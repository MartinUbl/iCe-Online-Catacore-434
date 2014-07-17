/*
 * Copyright (C) 2005-2011 MaNGOS <http://www.getmangos.com/>
 *
 * Copyright (C) 2008-2011 Trinity <http://www.trinitycore.org/>
 *
 * Copyright (C) 2010-2011 Project SkyFire <http://www.projectskyfire.org/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "gamePCH.h"
#include "InstanceScript.h"
#include "DatabaseEnv.h"
#include "Map.h"
#include "Player.h"
#include "GameObject.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "Log.h"
#include "Unit.h"

void InstanceScript::SaveToDB()
{
    std::string data = GetSaveData();
    if (data.empty())
        return;
    CharacterDatabase.escape_string(data);
    CharacterDatabase.PExecute("UPDATE instance SET data = '%s' WHERE id = '%d'", data.c_str(), instance->GetInstanceId());
}

void InstanceScript::HandleGameObject(uint64 GUID, bool open, GameObject *go)
{
    if (!go)
        go = instance->GetGameObject(GUID);
    if (go)
        go->SetGoState(open ? GO_STATE_ACTIVE : GO_STATE_READY);
    else
        sLog->outDebug("TSCR: InstanceScript: HandleGameObject failed");
}

bool InstanceScript::IsEncounterInProgress() const
{
    //for (std::vector<BossInfo>::const_iterator itr = bosses.begin(); itr != bosses.end(); ++itr)//
    //   if (itr->state == IN_PROGRESS)//                        OLD bosses is not filled
    //        return true;//

    Map::PlayerList const &PlayerList = instance->GetPlayers();
    if (!PlayerList.isEmpty())
        for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
        {
            Player* pPlayer = i->getSource();
            Unit::AttackerSet attackers = pPlayer->getAttackers();
            if (pPlayer->IsInCombat())
                for (Unit::AttackerSet::const_iterator k = attackers.begin(); k != attackers.end(); ++k)
                {
                    if ((*k) && (*k)->GetTypeId() == TYPEID_UNIT && (*k)->ToCreature()->GetCreatureInfo()->rank == 3) //is boss??
                        return true;
                }
        }
    return false;
}

uint32 InstanceScript::GetResurrectionData() const
{
    return ressCounter;
}

bool InstanceScript::CanUseCombatRessurrection() const
{
    if (this->instance && this->instance->IsRaid())
    {
        if ( this->instance->GetDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL || this->instance->GetDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC)
        {
            return (GetResurrectionData() <= 2) ? true : false;
        }
        else // 10 man difficulty
        {
            return (GetResurrectionData() == 0) ? true : false;
        }
    }
    return true;
}

void InstanceScript::LoadMinionData(const MinionData *data)
{
    while (data->entry)
    {
        if (data->bossId < bosses.size())
            minions.insert(std::make_pair(data->entry, MinionInfo(&bosses[data->bossId])));

        ++data;
    }
    sLog->outDebug("InstanceScript::LoadMinionData: " UI64FMTD " minions loaded.", uint64(minions.size()));
}

void InstanceScript::LoadDoorData(const DoorData *data)
{
    while (data->entry)
    {
        if (data->bossId < bosses.size())
            doors.insert(std::make_pair(data->entry, DoorInfo(&bosses[data->bossId], data->type, BoundaryType(data->boundary))));

        ++data;
    }
    sLog->outDebug("InstanceScript::LoadDoorData: " UI64FMTD " doors loaded.", uint64(doors.size()));
}

void InstanceScript::UpdateMinionState(Creature *minion, EncounterState state)
{
    switch (state)
    {
        case NOT_STARTED:
            if (!minion->IsAlive())
                minion->Respawn();
            else if (minion->IsInCombat())
                minion->AI()->EnterEvadeMode();
            break;
        case IN_PROGRESS:
            if (!minion->IsAlive())
                minion->Respawn();
            else if (!minion->GetVictim())
                minion->AI()->DoZoneInCombat();
            break;
        default:
            break;
    }
}

void InstanceScript::UpdateDoorState(GameObject *door)
{
    DoorInfoMap::iterator lower = doors.lower_bound(door->GetEntry());
    DoorInfoMap::iterator upper = doors.upper_bound(door->GetEntry());
    if (lower == upper)
        return;

    bool open = true;
    for (DoorInfoMap::iterator itr = lower; itr != upper; ++itr)
    {
        if (itr->second.type == DOOR_TYPE_ROOM)
        {
            if (itr->second.bossInfo->state == IN_PROGRESS)
            {
                open = false;
                break;
            }
        }
        else if (itr->second.type == DOOR_TYPE_PASSAGE)
        {
            if (itr->second.bossInfo->state != DONE)
            {
                open = false;
                break;
            }
        }
    }

    door->SetGoState(open ? GO_STATE_ACTIVE : GO_STATE_READY);
    //sLog->outError("Door %u is %s.", door->GetEntry(), open ? "opened" : "closed");
}

void InstanceScript::AddDoor(GameObject *door, bool add)
{
    DoorInfoMap::iterator lower = doors.lower_bound(door->GetEntry());
    DoorInfoMap::iterator upper = doors.upper_bound(door->GetEntry());
    if (lower == upper)
        return;

    for (DoorInfoMap::iterator itr = lower; itr != upper; ++itr)
    {
        if (add)
        {
            itr->second.bossInfo->door[itr->second.type].insert(door);
            switch (itr->second.boundary)
            {
                default:
                case BOUNDARY_NONE:
                    break;
                case BOUNDARY_N:
                case BOUNDARY_S:
                    itr->second.bossInfo->boundary[itr->second.boundary] = door->GetPositionX();
                    break;
                case BOUNDARY_E:
                case BOUNDARY_W:
                    itr->second.bossInfo->boundary[itr->second.boundary] = door->GetPositionY();
                    break;
                case BOUNDARY_NW:
                case BOUNDARY_SE:
                    itr->second.bossInfo->boundary[itr->second.boundary] = door->GetPositionX() + door->GetPositionY();
                    break;
                case BOUNDARY_NE:
                case BOUNDARY_SW:
                    itr->second.bossInfo->boundary[itr->second.boundary] = door->GetPositionX() - door->GetPositionY();
                    break;
            }
        }
        else
            itr->second.bossInfo->door[itr->second.type].erase(door);
    }

    if (add)
        UpdateDoorState(door);
}

void InstanceScript::AddMinion(Creature *minion, bool add)
{
    MinionInfoMap::iterator itr = minions.find(minion->GetEntry());
    if (itr == minions.end())
        return;

    if (add)
        itr->second.bossInfo->minion.insert(minion);
    else
        itr->second.bossInfo->minion.erase(minion);
}

bool InstanceScript::SetBossState(uint32 id, EncounterState state)
{
    if (id < bosses.size())
    {
        BossInfo *bossInfo = &bosses[id];
        if (bossInfo->state == TO_BE_DECIDED) // loading
        {
            bossInfo->state = state;
            //sLog->outError("Inialize boss %u state as %u.", id, (uint32)state);
            return false;
        }
        else
        {
            if (bossInfo->state == state)
                return false;

            if (state == DONE)
                for (MinionSet::iterator i = bossInfo->minion.begin(); i != bossInfo->minion.end(); ++i)
                    if ((*i)->isWorldBoss() && (*i)->IsAlive())
                        return false;

            bossInfo->state = state;
            SaveToDB();
        }

        for (uint32 type = 0; type < MAX_DOOR_TYPES; ++type)
            for (DoorSet::iterator i = bossInfo->door[type].begin(); i != bossInfo->door[type].end(); ++i)
                UpdateDoorState(*i);

        for (MinionSet::iterator i = bossInfo->minion.begin(); i != bossInfo->minion.end(); ++i)
            UpdateMinionState(*i, state);

        return true;
    }
    return false;
}

std::string InstanceScript::LoadBossState(const char * data)
{
    if (!data)
        return NULL;
    std::istringstream loadStream(data);
    uint32 buff;
    uint32 bossId = 0;
    for (std::vector<BossInfo>::iterator i = bosses.begin(); i != bosses.end(); ++i, ++bossId)
    {
        loadStream >> buff;
        if (buff < TO_BE_DECIDED)
            SetBossState(bossId, (EncounterState)buff);
    }
    return loadStream.str();
}

std::string InstanceScript::GetBossSaveData()
{
    std::ostringstream saveStream;
    for (std::vector<BossInfo>::iterator i = bosses.begin(); i != bosses.end(); ++i)
        saveStream << (uint32)i->state << " ";
    return saveStream.str();
}

void InstanceScript::DoUseDoorOrButton(uint64 uiGuid, uint32 uiWithRestoreTime, bool bUseAlternativeState)
{
    if (!uiGuid)
        return;

    GameObject* pGo = instance->GetGameObject(uiGuid);

    if (pGo)
    {
        if (pGo->GetGoType() == GAMEOBJECT_TYPE_DOOR || pGo->GetGoType() == GAMEOBJECT_TYPE_BUTTON)
        {
            if (pGo->getLootState() == GO_READY)
                pGo->UseDoorOrButton(uiWithRestoreTime,bUseAlternativeState);
            else if (pGo->getLootState() == GO_ACTIVATED)
                pGo->ResetDoorOrButton();
        }
        else
            sLog->outError("SD2: Script call DoUseDoorOrButton, but gameobject entry %u is type %u.",pGo->GetEntry(),pGo->GetGoType());
    }
}

void InstanceScript::DoRespawnGameObject(uint64 uiGuid, uint32 uiTimeToDespawn)
{
    if (GameObject* pGo = instance->GetGameObject(uiGuid))
    {
        //not expect any of these should ever be handled
        if (pGo->GetGoType() == GAMEOBJECT_TYPE_FISHINGNODE || pGo->GetGoType() == GAMEOBJECT_TYPE_DOOR ||
            pGo->GetGoType() == GAMEOBJECT_TYPE_BUTTON || pGo->GetGoType() == GAMEOBJECT_TYPE_TRAP)
            return;

        if (pGo->isSpawned())
            return;

        pGo->SetRespawnTime(uiTimeToDespawn);
    }
}

void InstanceScript::DoUpdateWorldState(uint32 uiStateId, uint32 uiStateData)
{
    Map::PlayerList const& lPlayers = instance->GetPlayers();

    if (!lPlayers.isEmpty())
    {
        for (Map::PlayerList::const_iterator itr = lPlayers.begin(); itr != lPlayers.end(); ++itr)
            if (Player *pPlayer = itr->getSource())
                pPlayer->SendUpdateWorldState(uiStateId, uiStateData);
    }
    else
        sLog->outDebug("TSCR: DoUpdateWorldState attempt send data but no players in map.");
}

// Send Notify to all players in instance
void InstanceScript::DoSendNotifyToInstance(const char *format, ...)
{
    InstanceMap::PlayerList const &PlayerList = instance->GetPlayers();
    InstanceMap::PlayerList::const_iterator i;

    if (!PlayerList.isEmpty())
    {
        va_list ap;
        va_start(ap, format);
        for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
        {
            if (Player *pPlayer = i->getSource())
                if (WorldSession *pSession = pPlayer->GetSession())
                    pSession->SendNotification(format, ap);
        }
        va_end(ap);
    }
}

// Complete Achievement for all players in instance
void InstanceScript::DoCompleteAchievement(uint32 achievement)
{
    AchievementEntry const* pAE = GetAchievementStore()->LookupEntry(achievement);
    Map::PlayerList const &PlayerList = instance->GetPlayers();

    if (!pAE)
    {
        sLog->outError("TSCR: DoCompleteAchievement called for not existing achievement %u", achievement);
        return;
    }

    if (!PlayerList.isEmpty())
        for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
            if (Player *pPlayer = i->getSource())
                pPlayer->CompletedAchievement(pAE);
}

// Update Achievement Criteria for all players in instance
void InstanceScript::DoUpdateAchievementCriteria(AchievementCriteriaTypes type, uint32 miscvalue1, uint32 miscvalue2, Unit *unit, uint32 time)
{
    Map::PlayerList const &PlayerList = instance->GetPlayers();

    if (!PlayerList.isEmpty())
        for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
            if (Player *pPlayer = i->getSource())
                pPlayer->UpdateAchievementCriteria(type, miscvalue1, miscvalue2, unit, time);
}

// Start timed achievement for all players in instance
void InstanceScript::DoStartTimedAchievement(AchievementCriteriaTimedTypes type, uint32 entry)
{
    Map::PlayerList const &PlayerList = instance->GetPlayers();

    if (!PlayerList.isEmpty())
        for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
            if (Player *pPlayer = i->getSource())
                pPlayer->GetAchievementMgr().StartTimedAchievement(type, entry);
}

// Stop timed achievement for all players in instance
void InstanceScript::DoStopTimedAchievement(AchievementCriteriaTimedTypes type, uint32 entry)
{
    Map::PlayerList const &PlayerList = instance->GetPlayers();

    if (!PlayerList.isEmpty())
        for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
            if (Player *pPlayer = i->getSource())
                pPlayer->GetAchievementMgr().RemoveTimedAchievement(type, entry);
}

// Remove Auras due to Spell on all players in instance
void InstanceScript::DoRemoveAurasDueToSpellOnPlayers(uint32 spell)
{
    Map::PlayerList const &PlayerList = instance->GetPlayers();

    if (!PlayerList.isEmpty())
        for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
            if (Player* pPlayer = i->getSource())
                pPlayer->RemoveAurasDueToSpell(spell);
}

// Add Aura on all players in instance
void InstanceScript::DoAddAuraOnPlayers(Unit * source, uint32 auraId)
{
    Map::PlayerList const &PlayerList = instance->GetPlayers();

    if (!PlayerList.isEmpty())
        for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
        if (Player* pPlayer = i->getSource())
            source->AddAura(auraId, pPlayer);
}

// Set scripted power to all players in instance
void InstanceScript::DoSetScriptedPowerToPlayers(int32 amount)
{
    Map::PlayerList const &PlayerList = instance->GetPlayers();

    if (!PlayerList.isEmpty())
        for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
            if (Player* pPlayer = i->getSource())
                pPlayer->SetPower(POWER_SCRIPTED, amount);
}

// Set max scripted power to all players in instance
void InstanceScript::DoSetMaxScriptedPowerToPlayers(int32 amount)
{
    Map::PlayerList const &PlayerList = instance->GetPlayers();

    if (!PlayerList.isEmpty())
        for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
            if (Player* pPlayer = i->getSource())
                pPlayer->SetMaxPower(POWER_SCRIPTED, amount);
}

bool InstanceScript::CheckAchievementCriteriaMeet(uint32 criteria_id, Player const* /*source*/, Unit const* /*target*/ /*= NULL*/, uint32 /*miscvalue1*/ /*= 0*/)
{
    sLog->outError("Achievement system call InstanceScript::CheckAchievementCriteriaMeet but instance script for map %u not have implementation for achievement criteria %u",
        instance->GetId(),criteria_id);
    return false;
}


void InstanceScript::SendEncounterUnit(uint32 type, Unit* unit /*= NULL*/, uint8 param1 /*= 0*/, uint8 param2 /*= 0*/)
{
    if (!unit)
        return;

    if (Creature * cr = unit->ToCreature()) // Some creature have rank 3 in DB and we dont want to send encounter frame for players
    {
        switch (cr->GetEntry())
        {
            case 52593: // Spirit of Flame ( Majordomo encounter)
            case 53619: // Druid of the Flame ( Majordomo thrash )
            case 53803:
                return;

            default:
                break;
        }
    }

    // size of this packet is at most 15 (usually less)
    WorldPacket data(SMSG_UPDATE_INSTANCE_ENCOUNTER_UNIT, 15);
    data << uint32(type);

    switch (type)
    {
    case ENCOUNTER_FRAME_ENGAGE:
    case ENCOUNTER_FRAME_DISENGAGE:
    case ENCOUNTER_FRAME_UPDATE_PRIORITY:
        data.append(unit->GetPackGUID());
        data << uint8(param1);
        break;
    case ENCOUNTER_FRAME_ADD_TIMER:
    case ENCOUNTER_FRAME_ENABLE_OBJECTIVE:
    case ENCOUNTER_FRAME_DISABLE_OBJECTIVE:
    case ENCOUNTER_FRAME_SET_COMBAT_RES_LIMIT:
        data << uint8(param1);
        break;
    case ENCOUNTER_FRAME_UPDATE_OBJECTIVE:
        data << uint8(param1);
        data << uint8(param2);
        break;
    case ENCOUNTER_FRAME_UNK7:
    case ENCOUNTER_FRAME_ADD_COMBAT_RES_LIMIT:
    case ENCOUNTER_FRAME_RESET_COMBAT_RES_LIMIT:
    default:
        break;
    }

    instance->SendToPlayers(&data);
}
