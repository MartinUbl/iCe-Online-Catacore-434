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
#include <G3D/Quat.h>
#include "Common.h"
#include "QuestDef.h"
#include "GameObjectAI.h"
#include "ObjectMgr.h"
#include "PoolMgr.h"
#include "SpellMgr.h"
#include "Spell.h"
#include "UpdateMask.h"
#include "Opcodes.h"
#include "WorldPacket.h"
#include "World.h"
#include "DatabaseEnv.h"
#include "LootMgr.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "InstanceScript.h"
#include "Battleground.h"
#include "Util.h"
#include "OutdoorPvPMgr.h"
#include "BattlegroundAV.h"
#include "ScriptMgr.h"
#include "CreatureAISelector.h"
#include "GameObjectModel.h"
#include "DynamicTree.h"
#include "Transport.h"

GameObject::GameObject() : WorldObject(), m_model(NULL), m_goValue(new GameObjectValue), m_AI(NULL)
{
    m_objectType |= TYPEMASK_GAMEOBJECT;
    m_objectTypeId = TYPEID_GAMEOBJECT;

    m_updateFlag = (UPDATEFLAG_STATIONARY_POSITION | UPDATEFLAG_ROTATION);

    m_valuesCount = GAMEOBJECT_END;
    m_respawnTime = 0;
    m_respawnDelayTime = 300;
    m_lootState = GO_NOT_READY;
    m_spawnedByDefault = true;
    m_usetimes = 0;
    m_spellId = 0;
    m_cooldownTime = 0;
    m_goInfo = NULL;
    m_ritualOwner = NULL;
    m_goData = NULL;

    m_DBTableGuid = 0;
    m_rotation = 0;

    m_groupLootTimer = 0;
    lootingGroupLowGUID = 0;

    ResetLootMode(); // restore default loot mode
}

GameObject::~GameObject()
{
    delete m_goValue;
    delete m_AI;
    delete m_model;
    if (m_goInfo->type == GAMEOBJECT_TYPE_TRANSPORT)
        delete m_goValue->Transport.StopFrames;
    //if (m_uint32Values)                                      // field array can be not exist if GameOBject not loaded
    //    CleanupsBeforeDelete();
}

bool GameObject::AIM_Initialize()
{

    m_AI = FactorySelector::SelectGameObjectAI(this);
    if (!m_AI) return false;
    m_AI->InitializeAI();
    return true;
}

std::string GameObject::GetAIName() const
{
    return ObjectMgr::GetGameObjectInfo(GetEntry())->AIName;
}

void GameObject::CleanupsBeforeDelete(bool /*finalCleanup*/)
{
    if (IsInWorld())
        RemoveFromWorld();

    if (m_uint32Values)                                      // field array can be not exist if GameOBject not loaded
    {
        // Possible crash at access to deleted GO in Unit::m_gameobj
        if (uint64 owner_guid = GetOwnerGUID())
        {
            Unit* owner = ObjectAccessor::GetUnit(*this,owner_guid);

            if (owner)
                owner->RemoveGameObject(this,false);
            else
            {
                const char * ownerType = "creature";
                if (IS_PLAYER_GUID(owner_guid))
                    ownerType = "player";
                else if (IS_PET_GUID(owner_guid))
                    ownerType = "pet";

                sLog->outError("Delete GameObject (GUID: %u Entry: %u SpellId %u LinkedGO %u) that lost references to owner (GUID %u Type '%s') GO list. Crash possible later.",
                    GetGUIDLow(), GetGOInfo()->id, m_spellId, GetGOInfo()->GetLinkedGameObjectEntry(), GUID_LOPART(owner_guid), ownerType);
            }
        }
    }

    if (GetTransport() && !ToTransport())
        GetTransport()->RemovePassenger(this);
}

void GameObject::AddToWorld()
{
    ///- Register the gameobject for guid lookup
    if (!IsInWorld())
    {
        if (m_zoneScript)
            m_zoneScript->OnGameObjectCreate(this, true);

        sObjectAccessor->AddObject(this);
        bool startOpen = (GetGoType() == GAMEOBJECT_TYPE_DOOR || GetGoType() == GAMEOBJECT_TYPE_BUTTON ? GetGOInfo()->door.startOpen : false);
        // The state can be changed after GameObject::Create but before GameObject::AddToWorld
        bool toggledState = GetGOData() ? GetGOData()->go_state == GO_STATE_READY : false;
        if (m_model)
            GetMap()->Insert(*m_model);
        EnableCollision(startOpen ^ toggledState);
        WorldObject::AddToWorld();
    }
}

void GameObject::RemoveFromWorld()
{
    ///- Remove the gameobject from the accessor
    if (IsInWorld())
    {
        if (m_zoneScript)
            m_zoneScript->OnGameObjectCreate(this, false);

        // Possible crash at access to deleted GO in Unit::m_gameobj
        if (uint64 owner_guid = GetOwnerGUID())
        {
            if (Unit * owner = GetOwner())
                owner->RemoveGameObject(this,false);
            else
                sLog->outError("Delete GameObject (GUID: %u Entry: %u) that have references in not found creature %u GO list. Crash possible later.",GetGUIDLow(),GetGOInfo()->id,GUID_LOPART(owner_guid));
        }

        if (m_model)
            if (GetMap()->Contains(*m_model))
                GetMap()->Remove(*m_model);
        WorldObject::RemoveFromWorld();
        sObjectAccessor->RemoveObject(this);
    }
}

bool GameObject::Create(uint32 guidlow, uint32 name_id, Map *map, uint32 phaseMask, float x, float y, float z, float ang, float rotation0, float rotation1, float rotation2, float rotation3, uint32 animprogress, GOState go_state, uint32 artKit)
{
    ASSERT(map);
    SetMap(map);

    Relocate(x, y, z, ang);
    m_stationaryPosition.Relocate(x, y, z, ang);
    if (!IsPositionValid())
    {
        sLog->outError("Gameobject (GUID: %u Entry: %u) not created. Suggested coordinates isn't valid (X: %f Y: %f)",guidlow,name_id,x,y);
        return false;
    }

    SetPhaseMask(phaseMask,false);

    SetZoneScript();
    if (m_zoneScript)
    {
        name_id = m_zoneScript->GetGameObjectEntry(guidlow, name_id);
        if (!name_id)
            return false;
    }

    GameObjectInfo const* goinfo = sObjectMgr->GetGameObjectInfo(name_id);
    if (!goinfo)
    {
        sLog->outErrorDb("Gameobject (GUID: %u Entry: %u) not created: it have not exist entry in `gameobject_template`. Map: %u  (X: %f Y: %f Z: %f) ang: %f rotation0: %f rotation1: %f rotation2: %f rotation3: %f",guidlow, name_id, map->GetId(), x, y, z, ang, rotation0, rotation1, rotation2, rotation3);
        return false;
    }

    if (goinfo->type == GAMEOBJECT_TYPE_TRANSPORT)
        m_updateFlag = (m_updateFlag | UPDATEFLAG_TRANSPORT) & ~UPDATEFLAG_GO_TRANSPORT_POSITION;

    Object::_Create(guidlow, goinfo->id, HIGHGUID_GAMEOBJECT);

    m_goInfo = goinfo;

    if (goinfo->type >= MAX_GAMEOBJECT_TYPE)
    {
        sLog->outErrorDb("Gameobject (GUID: %u Entry: %u) not created: it have not exist GO type '%u' in `gameobject_template`. It's will crash client if created.",guidlow,name_id,goinfo->type);
        return false;
    }

    SetFloatValue(GAMEOBJECT_PARENTROTATION+0, rotation0);
    SetFloatValue(GAMEOBJECT_PARENTROTATION+1, rotation1);

    UpdateRotationFields(rotation2,rotation3);              // GAMEOBJECT_FACING, GAMEOBJECT_ROTATION, GAMEOBJECT_PARENTROTATION+2/3

    SetFloatValue(OBJECT_FIELD_SCALE_X, goinfo->size);

    SetUInt32Value(GAMEOBJECT_FACTION, goinfo->faction);
    SetUInt32Value(GAMEOBJECT_FLAGS, goinfo->flags);

    SetEntry(goinfo->id);

    SetName(goinfo->name);

    SetDisplayId(goinfo->displayId);

    m_model = GameObjectModel::Create(*this);

    // GAMEOBJECT_BYTES_1, index at 0, 1, 2 and 3
    SetGoType(GameobjectTypes(goinfo->type));
    SetGoState(go_state);

    SetGoArtKit(0);                                         // unknown what this is
    SetByteValue(GAMEOBJECT_BYTES_1, 2, artKit);

    switch(goinfo->type)
    {
        case GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING:
            m_goValue->building.health = goinfo->building.intactNumHits + goinfo->building.damagedNumHits;
            m_goValue->building.maxhealth = goinfo->building.intactNumHits + goinfo->building.damagedNumHits;
            SetGoAnimProgress(255);

            SetUInt32Value(GAMEOBJECT_PARENTROTATION, goinfo->building.destructibleData);
            SetDisplayId(goinfo->building.destructibleData);
            break;
        case GAMEOBJECT_TYPE_TRANSPORT:
        {
            m_goValue->Transport.AnimationInfo = sTransportMgr->GetTransportAnimInfo(goinfo->id);
            m_goValue->Transport.PathProgress = getMSTime();
            if (m_goValue->Transport.AnimationInfo)
                m_goValue->Transport.PathProgress -= m_goValue->Transport.PathProgress % GetTransportPeriod();    // align to period
            m_goValue->Transport.CurrentSeg = 0;
            m_goValue->Transport.StateUpdateTimer = 0;
            m_goValue->Transport.StopFrames = new std::vector<uint32>();
            if (goinfo->transport.stopFrame1 > 0)
                m_goValue->Transport.StopFrames->push_back(goinfo->transport.stopFrame1);
            if (goinfo->transport.stopFrame2 > 0)
                m_goValue->Transport.StopFrames->push_back(goinfo->transport.stopFrame2);
            if (goinfo->transport.stopFrame3 > 0)
                m_goValue->Transport.StopFrames->push_back(goinfo->transport.stopFrame3);
            if (goinfo->transport.stopFrame4 > 0)
                m_goValue->Transport.StopFrames->push_back(goinfo->transport.stopFrame4);
            if (goinfo->transport.startOpen)
                SetTransportState(GO_STATE_TRANSPORT_STOPPED, goinfo->transport.startOpen - 1);
            else
                SetTransportState(GO_STATE_TRANSPORT_ACTIVE);

            SetGoAnimProgress(animprogress);
            break;
        }
        case GAMEOBJECT_TYPE_FISHINGNODE:
            SetGoAnimProgress(0);
            break;
        default:
            SetGoAnimProgress(animprogress);
            break;
    }
    LastUsedScriptID = GetGOInfo()->ScriptId;
    AIM_Initialize();

    return true;
}

void GameObject::Update(uint32 diff)
{
    if (AI())
        AI()->UpdateAI(diff);
    else if (!AIM_Initialize())
        sLog->outError("Could not initialize gameobject AI");

    switch (m_lootState)
    {
        case GO_NOT_READY:
        {
            switch(GetGoType())
            {
                case GAMEOBJECT_TYPE_TRAP:
                {
                    // Arming Time for GAMEOBJECT_TYPE_TRAP (6)
                    GameObjectInfo const* goInfo = GetGOInfo();
                    // Bombs
                    if (goInfo->trap.charges == 2)
                        m_cooldownTime = time(NULL) + 10;   // Hardcoded tooltip value
                    else if (Unit* owner = GetOwner())
                    {
                        if (owner->IsInCombat())
                            m_cooldownTime = time(NULL) + goInfo->trap.startDelay;
                    }
                    m_lootState = GO_READY;
                    break;
                }
                case GAMEOBJECT_TYPE_TRANSPORT:
                {
                    if (!m_goValue->Transport.AnimationInfo)
                        break;

                    if (GetGoState() == GO_STATE_TRANSPORT_ACTIVE)
                    {
                        m_goValue->Transport.PathProgress += diff;
                        /* TODO: Fix movement in unloaded grid - currently GO will just disappear
                        uint32 timer = m_goValue.Transport.PathProgress % m_goValue.Transport.AnimationInfo->TotalTime;
                        TransportAnimationEntry const* node = m_goValue.Transport.AnimationInfo->GetAnimNode(timer);
                        if (node && m_goValue.Transport.CurrentSeg != node->TimeSeg)
                        {
                            m_goValue.Transport.CurrentSeg = node->TimeSeg;

                            G3D::Quat rotation = m_goValue.Transport.AnimationInfo->GetAnimRotation(timer);
                            G3D::Vector3 pos = rotation.toRotationMatrix()
                                             * G3D::Matrix3::fromEulerAnglesZYX(GetOrientation(), 0.0f, 0.0f)
                                             * G3D::Vector3(node->X, node->Y, node->Z);

                            pos += G3D::Vector3(GetStationaryX(), GetStationaryY(), GetStationaryZ());

                            G3D::Vector3 src(GetPositionX(), GetPositionY(), GetPositionZ());

                            sLog->outInfo(LOG_FILTER_GENERAL, "Src: %s Dest: %s", src.toString().c_str(), pos.toString().c_str());

                            GetMap()->GameObjectRelocation(this, pos.x, pos.y, pos.z, GetOrientation());
                        }
                        */

                        if (!m_goValue->Transport.StopFrames->empty())
                        {
                            uint32 visualStateBefore = (m_goValue->Transport.StateUpdateTimer / 20000) & 1;
                            m_goValue->Transport.StateUpdateTimer += diff;
                            uint32 visualStateAfter = (m_goValue->Transport.StateUpdateTimer / 20000) & 1;
                            if (visualStateBefore != visualStateAfter)
                            {
                                ForceValuesUpdateAtIndex(GAMEOBJECT_LEVEL);
                                ForceValuesUpdateAtIndex(GAMEOBJECT_BYTES_1);
                            }
                        }
                    }
                    break;
                }
                case GAMEOBJECT_TYPE_FISHINGNODE:
                {
                    // fishing code (bobber ready)
                    if (time(NULL) > m_respawnTime - FISHING_BOBBER_READY_TIME)
                    {
                        // This should prevent fishing bots usage - send custom anim before the bobber is set to "active" state (therefore "lootable")
                        if (!HasFlag(GAMEOBJECT_FLAGS, GO_FLAG_NODESPAWN))
                        {
                            SendCustomAnim(GetGoAnimProgress());
                            SetUInt32Value(GAMEOBJECT_FLAGS, GO_FLAG_NODESPAWN);

                            // delay the real "splash" by one update tick to confuse fishing bot
                            break;
                        }

                        // splash bobber (bobber ready now)
                        Unit* caster = GetOwner();
                        if (caster && caster->GetTypeId() == TYPEID_PLAYER)
                        {
                            SetGoState(GO_STATE_ACTIVE);

                            UpdateData udata(GetMapId());
                            WorldPacket packet;
                            BuildValuesUpdateBlockForPlayer(&udata,caster->ToPlayer());
                            udata.BuildPacket(&packet);
                            caster->ToPlayer()->GetSession()->SendPacket(&packet);

                            // Add some additional splash - this should help players to recognize the real catch
                            Creature* pVisual = SummonTrigger(GetPositionX(),GetPositionY(),GetPositionZ(),0.0f,2000);
                            if (pVisual)
                                pVisual->CastSpell(pVisual, 69657, true);

                            SendCustomAnim(GetGoAnimProgress());
                        }

                        m_lootState = GO_READY;                 // can be successfully open with some chance
                    }
                    return;
                }
                default:
                    m_lootState = GO_READY;                         // for other GOis same switched without delay to GO_READY
                    break;
            }
            // NO BREAK for switch (m_lootState)
        }
        case GO_READY:
        {
            if (m_respawnTime > 0)                          // timer on
            {
                if (m_respawnTime <= time(NULL))            // timer expired
                {
                    m_respawnTime = 0;
                    m_SkillupList.clear();
                    m_usetimes = 0;

                    switch (GetGoType())
                    {
                        case GAMEOBJECT_TYPE_FISHINGNODE:   //  can't fish now
                        {
                            Unit* caster = GetOwner();
                            if (caster && caster->GetTypeId() == TYPEID_PLAYER)
                            {
                                caster->FinishSpell(CURRENT_CHANNELED_SPELL);

                                WorldPacket data(SMSG_FISH_ESCAPED,0);
                                caster->ToPlayer()->GetSession()->SendPacket(&data);
                            }
                            // can be delete
                            m_lootState = GO_JUST_DEACTIVATED;
                            return;
                        }
                        case GAMEOBJECT_TYPE_DOOR:
                        case GAMEOBJECT_TYPE_BUTTON:
                            //we need to open doors if they are closed (add there another condition if this code breaks some usage, but it need to be here for battlegrounds)
                            if (GetGoState() != GO_STATE_READY)
                                ResetDoorOrButton();
                            //flags in AB are type_button and we need to add them here so no break!
                        default:
                            if (!m_spawnedByDefault)        // despawn timer
                            {
                                                            // can be despawned or destroyed
                                SetLootState(GO_JUST_DEACTIVATED);
                                return;
                            }
                                                            // respawn timer
                            uint32 poolid = GetDBTableGUIDLow() ? sPoolMgr->IsPartOfAPool<GameObject>(GetDBTableGUIDLow()) : 0;
                            if (poolid)
                                sPoolMgr->UpdatePool<GameObject>(poolid, GetDBTableGUIDLow());
                            else
                                GetMap()->Add(this);
                            break;
                    }
                }
            }

            if (isSpawned())
            {
                // traps can have time and can not have
                GameObjectInfo const* goInfo = GetGOInfo();
                if (goInfo->type == GAMEOBJECT_TYPE_TRAP)
                {
                    if (m_cooldownTime >= time(NULL))
                        return;

                    // Type 2 - Bomb (will go away after casting it's spell)
                    if (goInfo->trap.charges == 2)
                    {
                        if (goInfo->trap.spellId)
                            CastSpell(NULL, goInfo->trap.spellId);  // FIXME: null target won't work for target type 1
                        SetLootState(GO_JUST_DEACTIVATED);
                        break;
                    }
                    // Type 0 and 1 - trap (type 0 will not get removed after casting a spell)
                    Unit* owner = GetOwner();
                    Unit* ok = NULL;                            // pointer to appropriate target if found any

                    bool IsBattlegroundTrap = false;
                    //FIXME: this is activation radius (in different casting radius that must be selected from spell data)
                    //TODO: move activated state code (cast itself) to GO_ACTIVATED, in this place only check activating and set state
                    float radius = (float)(goInfo->trap.radius)/2; // TODO rename radius to diameter (goInfo->trap.radius) should be (goInfo->trap.diameter)
                    if (!radius)
                    {
                        if (goInfo->trap.cooldown != 3)            // cast in other case (at some triggering/linked go/etc explicit call)
                            return;
                        else
                        {
                            if (m_respawnTime > 0)
                                break;

                            radius = (float)goInfo->trap.cooldown;       // battlegrounds gameobjects has data2 == 0 && data5 == 3
                            IsBattlegroundTrap = true;

                            if (!radius)
                                return;
                        }
                    }

                    // Note: this hack with search required until GO casting not implemented
                    // search unfriendly creature
                    if (owner)                    // hunter trap
                    {
                        Trinity::AnyUnfriendlyNoTotemUnitInObjectRangeCheck checker(this, owner, radius);
                        Trinity::UnitSearcher<Trinity::AnyUnfriendlyNoTotemUnitInObjectRangeCheck> searcher(this, ok, checker);
                        VisitNearbyGridObject(radius, searcher);
                        if (!ok) VisitNearbyWorldObject(radius, searcher);
                    }
                    else                                        // environmental trap
                    {
                        // environmental damage spells already have around enemies targeting but this not help in case not existed GO casting support
                        // affect only players
                        Player* player = NULL;
                        Trinity::AnyPlayerInObjectRangeCheck checker(this, radius);
                        Trinity::PlayerSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(this, player, checker);
                        VisitNearbyWorldObject(radius, searcher);
                        ok = player;
                    }

                    if (ok)
                    {
                        // some traps do not have spell but should be triggered
                        if (goInfo->trap.spellId)
                            CastSpell(ok, goInfo->trap.spellId);

                        m_cooldownTime = time(NULL) + 4;        // 4 seconds

                        if (owner)  // || goInfo->trap.charges == 1)
                            SetLootState(GO_JUST_DEACTIVATED);

                        if (IsBattlegroundTrap && ok->GetTypeId() == TYPEID_PLAYER)
                        {
                            //Battleground gameobjects case
                            if (ok->ToPlayer()->InBattleground())
                                if (Battleground *bg = ok->ToPlayer()->GetBattleground())
                                    bg->HandleTriggerBuff(GetGUID());
                        }
                    }
                }
                else if (uint32 max_charges = goInfo->GetCharges())
                {
                    if (m_usetimes >= max_charges)
                    {
                        m_usetimes = 0;
                        SetLootState(GO_JUST_DEACTIVATED);      // can be despawned or destroyed
                    }
                }
            }

            break;
        }
        case GO_ACTIVATED:
        {
            switch(GetGoType())
            {
                case GAMEOBJECT_TYPE_DOOR:
                case GAMEOBJECT_TYPE_BUTTON:
                    if (GetGOInfo()->GetAutoCloseTime() && (m_cooldownTime < time(NULL)))
                        ResetDoorOrButton();
                    break;
                case GAMEOBJECT_TYPE_GOOBER:
                    if (m_cooldownTime < time(NULL))
                    {
                        RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);

                        SetLootState(GO_JUST_DEACTIVATED);
                        m_cooldownTime = 0;
                    }
                    break;
                case GAMEOBJECT_TYPE_CHEST:
                    if (m_groupLootTimer)
                    {
                        if (m_groupLootTimer <= diff)
                        {
                            Group* group = sObjectMgr->GetGroupByGUID(lootingGroupLowGUID);
                            if (group)
                                group->EndRoll(&loot);
                            m_groupLootTimer = 0;
                            lootingGroupLowGUID = 0;
                        }
                        else m_groupLootTimer -= diff;
                    }
                default:
                    break;
            }
            break;
        }
        case GO_JUST_DEACTIVATED:
        {
            //if Gameobject should cast spell, then this, but some GOs (type = 10) should be destroyed
            if (GetGoType() == GAMEOBJECT_TYPE_GOOBER)
            {
                uint32 spellId = GetGOInfo()->goober.spellId;

                if (spellId)
                {
                    std::set<uint32>::const_iterator it = m_unique_users.begin();
                    std::set<uint32>::const_iterator end = m_unique_users.end();
                    for (; it != end; ++it)
                    {
                        if (Unit* owner = Unit::GetUnit(*this, uint64(*it)))
                            owner->CastSpell(owner, spellId, false);
                    }

                    m_unique_users.clear();
                    m_usetimes = 0;
                }

                SetGoState(GO_STATE_READY);

                //any return here in case battleground traps
            }

            if (GetOwnerGUID())
            {
                if (Unit* owner = GetOwner())
                {
                    owner->RemoveGameObject(this, false);
                    SetRespawnTime(0);
                    Delete();
                }
                return;
            }

            //burning flags in some battlegrounds, if you find better condition, just add it
            if (GetGOInfo()->IsDespawnAtAction() || GetGoAnimProgress() > 0)
            {
                SendObjectDeSpawnAnim(GetGUID());
                //reset flags
                SetUInt32Value(GAMEOBJECT_FLAGS, GetGOInfo()->flags);
            }

            loot.clear();
            SetLootState(GO_READY);

            if (!m_respawnDelayTime)
                return;

            if (!m_spawnedByDefault)
            {
                m_respawnTime = 0;
                UpdateObjectVisibility();
                return;
            }

            m_respawnTime = time(NULL) + m_respawnDelayTime;

            // if option not set then object will be saved at grid unload
            if (sWorld->getBoolConfig(CONFIG_SAVE_RESPAWN_TIME_IMMEDIATELY))
                SaveRespawnTime();

            UpdateObjectVisibility();

            break;
        }
    }
    sScriptMgr->OnGameObjectUpdate(this, diff);
}

void GameObject::Refresh()
{
    // not refresh despawned not casted GO (despawned casted GO destroyed in all cases anyway)
    if (m_respawnTime > 0 && m_spawnedByDefault)
        return;

    if (isSpawned())
        GetMap()->Add(this);
}

void GameObject::AddUniqueUse(Player* player)
{
    AddUse();
    m_unique_users.insert(player->GetGUIDLow());
}

void GameObject::Delete()
{
    SetLootState(GO_NOT_READY);
    if (GetOwnerGUID())
        if (Unit * owner = GetOwner())
            owner->RemoveGameObject(this, false);

    ASSERT (!GetOwnerGUID());
    SendObjectDeSpawnAnim(GetGUID());

    SetGoState(GO_STATE_READY);
    SetUInt32Value(GAMEOBJECT_FLAGS, GetGOInfo()->flags);

    uint32 poolid = GetDBTableGUIDLow() ? sPoolMgr->IsPartOfAPool<GameObject>(GetDBTableGUIDLow()) : 0;
    if (poolid)
        sPoolMgr->UpdatePool<GameObject>(poolid, GetDBTableGUIDLow());
    else
        AddObjectToRemoveList();
}

void GameObject::getFishLoot(Loot *fishloot, Player* loot_owner)
{
    fishloot->clear();

    uint32 zone, subzone;
    GetZoneAndAreaId(zone,subzone);

    // if subzone loot exist use it
    if (!fishloot->FillLoot(subzone, LootTemplates_Fishing, loot_owner, true, true))
        // else use zone loot (must exist in like case)
        fishloot->FillLoot(zone, LootTemplates_Fishing, loot_owner,true);
}

void GameObject::SaveToDB()
{
    // this should only be used when the gameobject has already been loaded
    // preferably after adding to map, because mapid may not be valid otherwise
    GameObjectData const *data = sObjectMgr->GetGOData(m_DBTableGuid);
    if (!data)
    {
        sLog->outError("GameObject::SaveToDB failed, cannot get gameobject data!");
        return;
    }

    SaveToDB(GetMapId(), data->spawnMask, data->phaseMask);
}

void GameObject::SaveToDB(uint32 mapid, uint8 spawnMask, uint32 phaseMask)
{
    const GameObjectInfo *goI = GetGOInfo();

    if (!goI)
        return;

    if (!m_DBTableGuid)
        m_DBTableGuid = GetGUIDLow();
    // update in loaded data (changing data only in this place)
    GameObjectData& data = sObjectMgr->NewGOData(m_DBTableGuid);

    // data->guid = guid don't must be update at save
    data.id = GetEntry();
    data.mapid = mapid;
    data.phaseMask = phaseMask;
    data.posX = GetPositionX();
    data.posY = GetPositionY();
    data.posZ = GetPositionZ();
    data.orientation = GetOrientation();
    data.rotation0 = GetFloatValue(GAMEOBJECT_PARENTROTATION+0);
    data.rotation1 = GetFloatValue(GAMEOBJECT_PARENTROTATION+1);
    data.rotation2 = GetFloatValue(GAMEOBJECT_PARENTROTATION+2);
    data.rotation3 = GetFloatValue(GAMEOBJECT_PARENTROTATION+3);
    data.spawntimesecs = m_spawnedByDefault ? m_respawnDelayTime : -(int32)m_respawnDelayTime;
    data.animprogress = GetGoAnimProgress();
    data.go_state = GetGoState();
    data.spawnMask = spawnMask;
    data.artKit = GetGoArtKit();

    // updated in DB
    std::ostringstream ss;
    ss << "INSERT INTO gameobject VALUES ("
        << m_DBTableGuid << ", "
        << GetEntry() << ", "
        << mapid << ", "
        << uint32(spawnMask) << ","                         // cast to prevent save as symbol
        << uint16(GetPhaseMask()) << ","                    // prevent out of range error
        << GetPositionX() << ", "
        << GetPositionY() << ", "
        << GetPositionZ() << ", "
        << GetOrientation() << ", "
        << GetFloatValue(GAMEOBJECT_PARENTROTATION) << ", "
        << GetFloatValue(GAMEOBJECT_PARENTROTATION+1) << ", "
        << GetFloatValue(GAMEOBJECT_PARENTROTATION+2) << ", "
        << GetFloatValue(GAMEOBJECT_PARENTROTATION+3) << ", "
        << m_respawnDelayTime << ", "
        << uint32(GetGoAnimProgress()) << ", "
        << uint32(GetGoState()) << ")";

    SQLTransaction trans = WorldDatabase.BeginTransaction();
    trans->PAppend("DELETE FROM gameobject WHERE guid = '%u'", m_DBTableGuid);
    trans->Append(ss.str().c_str());
    WorldDatabase.CommitTransaction(trans);
}

bool GameObject::LoadFromDB(uint32 guid, Map *map)
{
    GameObjectData const* data = sObjectMgr->GetGOData(guid);

    if (!data)
    {
        sLog->outErrorDb("Gameobject (GUID: %u) not found in table `gameobject`, can't load. ",guid);
        return false;
    }

    uint32 entry = data->id;
    //uint32 map_id = data->mapid;                          // already used before call
    uint32 phaseMask = data->phaseMask;
    float x = data->posX;
    float y = data->posY;
    float z = data->posZ;
    float ang = data->orientation;

    float rotation0 = data->rotation0;
    float rotation1 = data->rotation1;
    float rotation2 = data->rotation2;
    float rotation3 = data->rotation3;

    uint32 animprogress = data->animprogress;
    GOState go_state = data->go_state;
    uint32 artKit = data->artKit;

    m_DBTableGuid = guid;
    if (map->GetInstanceId() != 0) guid = sObjectMgr->GenerateLowGuid(HIGHGUID_GAMEOBJECT);

    if (!Create(guid,entry, map, phaseMask, x, y, z, ang, rotation0, rotation1, rotation2, rotation3, animprogress, go_state, artKit))
        return false;

    if (data->spawntimesecs >= 0)
    {
        m_spawnedByDefault = true;

        if (!GetGOInfo()->GetDespawnPossibility() && !GetGOInfo()->IsDespawnAtAction())
        {
            SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NODESPAWN);
            m_respawnDelayTime = 0;
            m_respawnTime = 0;
        }
        else
        {
            m_respawnDelayTime = data->spawntimesecs;
            m_respawnTime = sObjectMgr->GetGORespawnTime(m_DBTableGuid, map->GetInstanceId());

            // ready to respawn
            if (m_respawnTime && m_respawnTime <= time(NULL))
            {
                m_respawnTime = 0;
                sObjectMgr->RemoveGORespawnTime(m_DBTableGuid, GetInstanceId());
            }
        }
    }
    else
    {
        m_spawnedByDefault = false;
        m_respawnDelayTime = -data->spawntimesecs;
        m_respawnTime = 0;
    }

    m_goData = data;

    return true;
}

void GameObject::DeleteFromDB()
{
    sObjectMgr->RemoveGORespawnTime(m_DBTableGuid, GetInstanceId());
    sObjectMgr->DeleteGOData(m_DBTableGuid);
    WorldDatabase.PExecute("DELETE FROM gameobject WHERE guid = '%u'", m_DBTableGuid);
    WorldDatabase.PExecute("DELETE FROM game_event_gameobject WHERE guid = '%u'", m_DBTableGuid);
}

GameObject* GameObject::GetGameObject(WorldObject& object, uint64 guid)
{
    return object.GetMap()->GetGameObject(guid);
}

/*********************************************************/
/***                    QUEST SYSTEM                   ***/
/*********************************************************/
bool GameObject::hasQuest(uint32 quest_id) const
{
    QuestRelationBounds qr = sObjectMgr->GetGOQuestRelationBounds(GetEntry());
    for (QuestRelations::const_iterator itr = qr.first; itr != qr.second; ++itr)
    {
        if (itr->second == quest_id)
            return true;
    }
    return false;
}

bool GameObject::hasInvolvedQuest(uint32 quest_id) const
{
    QuestRelationBounds qir = sObjectMgr->GetGOQuestInvolvedRelationBounds(GetEntry());
    for (QuestRelations::const_iterator itr = qir.first; itr != qir.second; ++itr)
    {
        if (itr->second == quest_id)
            return true;
    }
    return false;
}

bool GameObject::IsTransport() const
{
    // If something is marked as a transport, don't transmit an out of range packet for it.
    GameObjectInfo const * gInfo = GetGOInfo();
    if (!gInfo) return false;
    return gInfo->type == GAMEOBJECT_TYPE_TRANSPORT || gInfo->type == GAMEOBJECT_TYPE_MO_TRANSPORT;
}

// is Dynamic transport = non-stop Transport
bool GameObject::IsDynTransport() const
{
    // If something is marked as a transport, don't transmit an out of range packet for it.
    GameObjectInfo const * gInfo = GetGOInfo();
    if (!gInfo) return false;
    return gInfo->type == GAMEOBJECT_TYPE_MO_TRANSPORT || (gInfo->type == GAMEOBJECT_TYPE_TRANSPORT && m_goValue->Transport.StopFrames->empty());
}

bool GameObject::IsDestructibleBuilding() const
{
    GameObjectInfo const* gInfo = GetGOInfo();
    if (!gInfo) return false;
    return gInfo->type == GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING;
}

Unit* GameObject::GetOwner() const
{
    return ObjectAccessor::GetUnit(*this, GetOwnerGUID());
}

void GameObject::SaveRespawnTime()
{
    if (m_goData && m_goData->dbData && m_respawnTime > time(NULL) && m_spawnedByDefault)
        sObjectMgr->SaveGORespawnTime(m_DBTableGuid,GetInstanceId(),m_respawnTime);
}

bool GameObject::isVisibleForInState(Player const* u, bool inVisibleList) const
{
    // Not in world
    if (!IsInWorld() || !u->IsInWorld())
        return false;

    // Transport always visible at this step implementation
    if ((IsTransport() || IsDestructibleBuilding()) && IsInMap(u))
        return true;

    // quick check visibility false cases for non-GM-mode
    if (!u->IsGameMaster())
    {
        // despawned and then not visible for non-GM in GM-mode
        if (!isSpawned())
            return false;

        // special invisibility cases
        if (GetGOInfo()->type == GAMEOBJECT_TYPE_TRAP && GetGOInfo()->trap.stealthed)
        {
            Unit *owner = GetOwner();
            if (owner && u->IsHostileTo(owner) && !canDetectTrap(u, GetDistance(u)))
                return false;
        }
    }

    // check distance
    return IsWithinDistInMap(u->m_seer,World::GetMaxVisibleDistanceForObject() +
        (inVisibleList ? World::GetVisibleObjectGreyDistance() : 0.0f), false);
}

bool GameObject::canDetectTrap(Player const* u, float distance) const
{
    if (u->HasUnitState(UNIT_STATE_STUNNED))
        return false;
    if (distance < GetGOInfo()->size) //collision
        return true;
    if (!u->HasInArc(M_PI, this)) //behind
        return false;
    if (u->HasAuraType(SPELL_AURA_DETECT_STEALTH))
        return true;

    //Visible distance is modified by -Level Diff (every level diff = 0.25f in visible distance)
    float visibleDistance = (int32(u->getLevel()) - int32(GetOwner()->getLevel()))* 0.25f;
    //GetModifier for trap (miscvalue 1)
    //35y for aura 2836
    //WARNING: these values are guessed, may be not blizzlike
    visibleDistance += u->GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_DETECT, 1)* 0.5f;

    return distance < visibleDistance;
}

void GameObject::Respawn()
{
    if (m_spawnedByDefault && m_respawnTime > 0)
    {
        m_respawnTime = time(NULL);
        sObjectMgr->RemoveGORespawnTime(m_DBTableGuid, GetInstanceId());
    }
}

bool GameObject::ActivateToQuest(Player *pTarget) const
{
    if (pTarget->HasQuestForGO(GetEntry()))
        return true;

    if (!sObjectMgr->IsGameObjectForQuests(GetEntry()))
        return false;

    switch (GetGoType())
    {
        // scan GO chest with loot including quest items
        case GAMEOBJECT_TYPE_CHEST:
        {
            if (LootTemplates_Gameobject.HaveQuestLootForPlayer(GetGOInfo()->GetLootId(), pTarget))
            {
                //TODO: fix this hack
                //look for battlegroundAV for some objects which are only activated after mine gots captured by own team
                if (GetEntry() == BG_AV_OBJECTID_MINE_N || GetEntry() == BG_AV_OBJECTID_MINE_S)
                    if (Battleground *bg = pTarget->GetBattleground())
                        if (bg->GetTypeID(true) == BATTLEGROUND_AV && !(((BattlegroundAV*)bg)->PlayerCanDoMineQuest(GetEntry(),pTarget->GetTeam())))
                            return false;
                return true;
            }
            break;
        }
        case GAMEOBJECT_TYPE_GENERIC:
        {
            if (GetGOInfo()->_generic.questID == -1 || pTarget->GetQuestStatus(GetGOInfo()->_generic.questID) == QUEST_STATUS_INCOMPLETE)
                return true;
            break;
        }
        case GAMEOBJECT_TYPE_GOOBER:
        {
            if (GetGOInfo()->goober.questId == -1 || pTarget->GetQuestStatus(GetGOInfo()->goober.questId) == QUEST_STATUS_INCOMPLETE)
                return true;
            break;
        }
        default:
            break;
    }

    return false;
}

void GameObject::TriggeringLinkedGameObject(uint32 trapEntry, Unit* target)
{
    GameObjectInfo const* trapInfo = sGOStorage.LookupEntry<GameObjectInfo>(trapEntry);
    if (!trapInfo || trapInfo->type != GAMEOBJECT_TYPE_TRAP)
        return;

    SpellEntry const* trapSpell = sSpellStore.LookupEntry(trapInfo->trap.spellId);
    if (!trapSpell)                                          // checked at load already
        return;

    float range;
    SpellRangeEntry const * srentry = sSpellRangeStore.LookupEntry(trapSpell->rangeIndex);
    if (GetSpellMaxRangeForHostile(srentry) == GetSpellMaxRangeForFriend(srentry))
        range = GetSpellMaxRangeForHostile(srentry);
    else
        // get owner to check hostility of GameObject
        if (Unit *owner = GetOwner())
            range = (float)owner->GetSpellMaxRangeForTarget(target, srentry);
        else
            // if no owner assume that object is hostile to target
            range = GetSpellMaxRangeForHostile(srentry);

    // search nearest linked GO
    GameObject* trapGO = NULL;
    {
        // using original GO distance
        CellPair p(Trinity::ComputeCellPair(GetPositionX(), GetPositionY()));
        Cell cell(p);
        cell.data.Part.reserved = ALL_DISTRICT;

        Trinity::NearestGameObjectEntryInObjectRangeCheck go_check(*target,trapEntry,range);
        Trinity::GameObjectLastSearcher<Trinity::NearestGameObjectEntryInObjectRangeCheck> checker(this, trapGO,go_check);

        TypeContainerVisitor<Trinity::GameObjectLastSearcher<Trinity::NearestGameObjectEntryInObjectRangeCheck>, GridTypeMapContainer > object_checker(checker);
        cell.Visit(p, object_checker, *GetMap(), *target, range);
    }

    // found correct GO
    if (trapGO)
        trapGO->CastSpell(target, trapInfo->trap.spellId);
}

GameObject* GameObject::LookupFishingHoleAround(float range)
{
    GameObject* ok = NULL;

    CellPair p(Trinity::ComputeCellPair(GetPositionX(),GetPositionY()));
    Cell cell(p);
    cell.data.Part.reserved = ALL_DISTRICT;
    Trinity::NearestGameObjectFishingHole u_check(*this, range);
    Trinity::GameObjectSearcher<Trinity::NearestGameObjectFishingHole> checker(this, ok, u_check);

    TypeContainerVisitor<Trinity::GameObjectSearcher<Trinity::NearestGameObjectFishingHole>, GridTypeMapContainer > grid_object_checker(checker);
    cell.Visit(p, grid_object_checker, *GetMap(), *this, range);

    return ok;
}

void GameObject::ResetDoorOrButton()
{
    if (m_lootState == GO_READY || m_lootState == GO_JUST_DEACTIVATED)
        return;

    SwitchDoorOrButton(false);
    SetLootState(GO_JUST_DEACTIVATED);
    m_cooldownTime = 0;
}

void GameObject::UseDoorOrButton(uint32 time_to_restore, bool alternative /* = false */)
{
    if (m_lootState != GO_READY)
        return;

    if (!time_to_restore)
        time_to_restore = GetGOInfo()->GetAutoCloseTime();

    SwitchDoorOrButton(true,alternative);
    SetLootState(GO_ACTIVATED);

    m_cooldownTime = time(NULL) + time_to_restore;
}

void GameObject::SetGoArtKit(uint8 kit)
{
    SetByteValue(GAMEOBJECT_BYTES_1, 2, kit);
    GameObjectData *data = const_cast<GameObjectData*>(sObjectMgr->GetGOData(m_DBTableGuid));
    if (data)
        data->artKit = kit;
}

void GameObject::SetGoArtKit(uint8 artkit, GameObject *go, uint32 lowguid)
{
    const GameObjectData *data = NULL;
    if (go)
    {
        go->SetGoArtKit(artkit);
        data = go->GetGOData();
    }
    else if (lowguid)
        data = sObjectMgr->GetGOData(lowguid);

    if (data)
        const_cast<GameObjectData*>(data)->artKit = artkit;
}

void GameObject::SwitchDoorOrButton(bool activate, bool alternative /* = false */)
{
    if (activate)
        SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);
    else
        RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);

    if (GetGoState() == GO_STATE_READY)                      //if closed -> open
        SetGoState(alternative ? GO_STATE_ACTIVE_ALTERNATIVE : GO_STATE_ACTIVE);
    else                                                    //if open -> close
        SetGoState(GO_STATE_READY);
}

void GameObject::Use(Unit* user)
{
    // by default spell caster is user
    Unit* spellCaster = user;
    uint32 spellId = 0;
    bool triggered = false;

    if (Player* plr = user->ToPlayer())
    {
        if(plr->GetEmoteState())
            plr->SetEmoteState(0);
    }

    switch(GetGoType())
    {
        case GAMEOBJECT_TYPE_DOOR:                          //0
        case GAMEOBJECT_TYPE_BUTTON:                        //1
            //doors/buttons never really despawn, only reset to default state/flags
            UseDoorOrButton();

            // activate script
            GetMap()->ScriptsStart(sGameObjectScripts, GetDBTableGUIDLow(), spellCaster, this);
            return;

        case GAMEOBJECT_TYPE_QUESTGIVER:                    //2
        {
            if (user->GetTypeId() != TYPEID_PLAYER)
                return;

            Player* player = (Player*)user;

            player->PrepareGossipMenu(this, GetGOInfo()->questgiver.gossipID);
            player->SendPreparedGossip(this);
            return;
        }
        //Sitting: Wooden bench, chairs enzz
        case GAMEOBJECT_TYPE_CHAIR:                         //7
        {
            GameObjectInfo const* info = GetGOInfo();
            if (!info)
                return;

            if (user->GetTypeId() != TYPEID_PLAYER)
                return;

            if (!ChairListSlots.size())        // this is called once at first chair use to make list of available slots
            {
                if (info->chair.slots > 0)     // sometimes chairs in DB have error in fields and we dont know number of slots
                    for (uint32 i = 0; i < info->chair.slots; ++i)
                        ChairListSlots[i] = 0; // Last user of current slot set to 0 (none sit here yet)
                else
                    ChairListSlots[0] = 0;     // error in DB, make one default slot
            }

            Player* player = (Player*)user;

            // a chair may have n slots. we have to calculate their positions and teleport the player to the nearest one

            float lowestDist = DEFAULT_VISIBILITY_DISTANCE;

            uint32 nearest_slot = 0;
            float x_lowest = GetPositionX();
            float y_lowest = GetPositionY();

            // the object orientation + 1/2 pi
            // every slot will be on that straight line
            float orthogonalOrientation = GetOrientation()+M_PI*0.5f;
            // find nearest slot
            bool found_free_slot = false;
            for (ChairSlotAndUser::iterator itr = ChairListSlots.begin(); itr != ChairListSlots.end(); ++itr)
            {
                // the distance between this slot and the center of the go - imagine a 1D space
                float relativeDistance = (info->size*itr->first)-(info->size*(info->chair.slots-1)/2.0f);

                float x_i = GetPositionX() + relativeDistance * cos(orthogonalOrientation);
                float y_i = GetPositionY() + relativeDistance * sin(orthogonalOrientation);

                if (itr->second)
                {
                    if (Player* ChairUser = sObjectMgr->GetPlayer(itr->second))
                    {
                        if (ChairUser->IsSitState() && ChairUser->getStandState() != UNIT_STAND_STATE_SIT && ChairUser->GetExactDist2d(x_i, y_i) < 0.1f)
                            continue;        // This seat is already occupied by ChairUser. NOTE: Not sure if the ChairUser->getStandState() != UNIT_STAND_STATE_SIT check is required.
                        else
                            itr->second = 0; // This seat is unoccupied.
                    }
                    else
                        itr->second = 0;     // The seat may of had an occupant, but they're offline.
                }

                found_free_slot = true;

                // calculate the distance between the player and this slot
                float thisDistance = player->GetDistance2d(x_i, y_i);

                /* debug code. It will spawn a npc on each slot to visualize them.
                Creature* helper = player->SummonCreature(14496, x_i, y_i, GetPositionZ(), GetOrientation(), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 10000);
                std::ostringstream output;
                output << i << ": thisDist: " << thisDistance;
                helper->MonsterSay(output.str().c_str(), LANG_UNIVERSAL, 0);
                */

                if (thisDistance <= lowestDist)
                {
                    nearest_slot = itr->first;
                    lowestDist = thisDistance;
                    x_lowest = x_i;
                    y_lowest = y_i;
                }
            }

            if (found_free_slot)
            {
                ChairSlotAndUser::iterator itr = ChairListSlots.find(nearest_slot);
                if (itr != ChairListSlots.end())
                {
                    itr->second = player->GetGUID(); //this slot in now used by player
                    player->TeleportTo(GetMapId(), x_lowest, y_lowest, GetPositionZ(), GetOrientation(),TELE_TO_NOT_LEAVE_TRANSPORT | TELE_TO_NOT_LEAVE_COMBAT | TELE_TO_NOT_UNSUMMON_PET);
                    player->SetStandState(UNIT_STAND_STATE_SIT_LOW_CHAIR+info->chair.height);
                    return;
                }
            }
            //else
                //player->GetSession()->SendNotification("There's nowhere left for you to sit.");

            return;
        }
        //big gun, its a spell/aura
        case GAMEOBJECT_TYPE_GOOBER:                        //10
        {
            GameObjectInfo const* info = GetGOInfo();

            if (user->GetTypeId() == TYPEID_PLAYER)
            {
                Player* player = (Player*)user;

                if (info->goober.pageId)                    // show page...
                {
                    WorldPacket data(SMSG_GAMEOBJECT_PAGETEXT, 8);
                    data << GetGUID();
                    player->GetSession()->SendPacket(&data);
                }
                else if (info->goober.gossipID)
                {
                    player->PrepareGossipMenu(this, info->goober.gossipID);
                    player->SendPreparedGossip(this);
                }

                if (info->goober.eventId)
                {
                    sLog->outDebug("Goober ScriptStart id %u for GO entry %u (GUID %u).", info->goober.eventId, GetEntry(), GetDBTableGUIDLow());
                    GetMap()->ScriptsStart(sEventScripts, info->goober.eventId, player, this);
                    EventInform(info->goober.eventId);
                }

                // possible quest objective for active quests
                if (info->goober.questId && sObjectMgr->GetQuestTemplate(info->goober.questId))
                {
                    //Quest require to be active for GO using
                    if (player->GetQuestStatus(info->goober.questId) != QUEST_STATUS_INCOMPLETE)
                        break;
                }

                if (Battleground* bg = player->GetBattleground())
                    bg->EventPlayerUsedGO(player, this);

                player->CastedCreatureOrGO(info->id, GetGUID(), 0);
            }

            if (uint32 trapEntry = info->goober.linkedTrapId)
                TriggeringLinkedGameObject(trapEntry, user);

            SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);
            SetLootState(GO_ACTIVATED);

            uint32 time_to_restore = info->GetAutoCloseTime();

            // this appear to be ok, however others exist in addition to this that should have custom (ex: 190510, 188692, 187389)
            if (time_to_restore && info->goober.customAnim)
                SendCustomAnim(GetGoAnimProgress());
            else
                SetGoState(GO_STATE_ACTIVE);

            m_cooldownTime = time(NULL) + time_to_restore;

            // cast this spell later if provided
            spellId = info->goober.spellId;
            spellCaster = NULL;

            break;
        }
        case GAMEOBJECT_TYPE_CAMERA:                        //13
        {
            GameObjectInfo const* info = GetGOInfo();
            if (!info)
                return;

            if (user->GetTypeId() != TYPEID_PLAYER)
                return;

            Player* player = (Player*)user;

            if (info->camera.cinematicId)
                player->SendCinematicStart(info->camera.cinematicId);

            if (info->camera.eventID)
                GetMap()->ScriptsStart(sEventScripts, info->camera.eventID, player, this);

            return;
        }
        //fishing bobber
        case GAMEOBJECT_TYPE_FISHINGNODE:                   //17
        {
            if (user->GetTypeId() != TYPEID_PLAYER)
                return;

            Player* player = (Player*)user;

            if (player->GetGUID() != GetOwnerGUID())
                return;

            switch (getLootState())
            {
                case GO_READY:                              // ready for loot
                {
                    uint32 zone, subzone;
                    GetZoneAndAreaId(zone,subzone);

                    int32 zone_skill = sObjectMgr->GetFishingBaseSkillLevel(subzone);
                    if (!zone_skill)
                        zone_skill = sObjectMgr->GetFishingBaseSkillLevel(zone);

                    //provide error, no fishable zone or area should be 0
                    if (!zone_skill)
                        sLog->outErrorDb("Fishable areaId %u are not properly defined in `skill_fishing_base_level`.",subzone);

                    int32 skill = player->GetSkillValue(SKILL_FISHING);

                    int32 chance;
                    if (skill < zone_skill)
                    {
                        chance = int32(pow((double)skill/zone_skill,2) * 100);
                        if (chance < 1)
                            chance = 1;
                    }
                    else
                        chance = 100;

                    int32 roll = irand(1,100);

                    sLog->outStaticDebug("Fishing check (skill: %i zone min skill: %i chance %i roll: %i",skill,zone_skill,chance,roll);

                    // but you will likely cause junk in areas that require a high fishing skill (not yet implemented)
                    if (chance >= roll)
                    {
                        player->UpdateFishingSkill();

                        // prevent removing GO at spell cancel
                        player->RemoveGameObject(this,false);
                        SetOwnerGUID(player->GetGUID());

                        //TODO: find reasonable value for fishing hole search
                        GameObject* ok = LookupFishingHoleAround(20.0f + CONTACT_DISTANCE);
                        if (ok)
                        {
                            ok->Use(player);
                            SetLootState(GO_JUST_DEACTIVATED);

                            // accumulate pool catch progress
                            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_CATCH_FROM_POOL);
                        }
                        else
                            player->SendLoot(GetGUID(),LOOT_FISHING);
                    }
                    // TODO: else: junk

                    break;
                }
                case GO_JUST_DEACTIVATED:                   // nothing to do, will be deleted at next update
                    break;
                default:
                {
                    SetLootState(GO_JUST_DEACTIVATED);

                    // if has "nodespawn" flag and is not yet lootable (loot state "go_ready") but already splashed visually
                    // the real delay between splash and lootable flag setting is approx. 100ms, which is much smaller than
                    // most people reaction delay, therefore it indicates fishing bot usage
                    if (HasFlag(GAMEOBJECT_FLAGS, GO_FLAG_NODESPAWN))
                    {
                        sLog->outChar("IP:(%s) account:(%u) character:(%s) action:(%s) type:(%s))",
                        player->GetSession() ? player->GetSession()->GetRemoteAddress().c_str() : "none",
                        player->GetSession()->GetAccountId(),
                        player->GetName(),
                        "cheat",
                        "fishing bot");
                    }

                    WorldPacket data(SMSG_FISH_NOT_HOOKED, 0);
                    player->GetSession()->SendPacket(&data);
                    break;
                }
            }

            player->FinishSpell(CURRENT_CHANNELED_SPELL);
            return;
        }

        case GAMEOBJECT_TYPE_SUMMONING_RITUAL:              //18
        {
            if (user->GetTypeId() != TYPEID_PLAYER)
                return;

            Player* player = (Player*)user;

            Unit* owner = GetOwner();

            GameObjectInfo const* info = GetGOInfo();

            // ritual owner is set for GO's without owner (not summoned)
            if (!m_ritualOwner && !owner)
                m_ritualOwner = player;

            if (owner)
            {
                if (owner->GetTypeId() != TYPEID_PLAYER)
                    return;

                // accept only use by player from same group as owner, excluding owner itself (unique use already added in spell effect)
                if (player == owner->ToPlayer() || (info->summoningRitual.castersGrouped && !player->IsInSameRaidWith(owner->ToPlayer())))
                    return;

                // expect owner to already be channeling, so if not...
                if (!owner->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
                    return;

                // in case summoning ritual caster is GO creator
                spellCaster = owner;
            }
            else
            {
                if (player != m_ritualOwner && (info->summoningRitual.castersGrouped && !player->IsInSameRaidWith(m_ritualOwner)))
                    return;

                spellCaster = player;
            }

            AddUniqueUse(player);

            if (info->summoningRitual.animSpell)
            {
                player->CastSpell(player, info->summoningRitual.animSpell, true);

                // for this case, summoningRitual.spellId is always triggered
                triggered = true;
            }

            // full amount unique participants including original summoner
            if (GetUniqueUseCount() == info->summoningRitual.reqParticipants)
            {
                spellCaster = m_ritualOwner ? m_ritualOwner : spellCaster;

                spellId = info->summoningRitual.spellId;

                if (spellId == 62330)                       // GO store nonexistent spell, replace by expected
                {
                    // spell have reagent and mana cost but it not expected use its
                    // it triggered spell in fact casted at currently channeled GO
                    spellId = 61993;
                    triggered = true;
                }

                // finish owners spell
                if (owner)
                    owner->FinishSpell(CURRENT_CHANNELED_SPELL);

                // can be deleted now, if
                if (!info->summoningRitual.ritualPersistent)
                    SetLootState(GO_JUST_DEACTIVATED);
                else
                {
                    // reset ritual for this GO
                    m_ritualOwner = NULL;
                    m_unique_users.clear();
                    m_usetimes = 0;
                }
            }
            else
                return;

            // go to end function to spell casting
            break;
        }
        case GAMEOBJECT_TYPE_SPELLCASTER:                   //22
        {
            GameObjectInfo const* info = GetGOInfo();
            if (!info)
                return;

            if (info->spellcaster.partyOnly)
            {
                Unit* caster = GetOwner();
                if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
                    return;

                if (user->GetTypeId() != TYPEID_PLAYER || !user->ToPlayer()->IsInSameRaidWith(caster->ToPlayer()))
                    return;
            }

            user->RemoveAurasByType(SPELL_AURA_MOUNTED);
            spellId = info->spellcaster.spellId;

            AddUse();
            break;
        }
        case GAMEOBJECT_TYPE_MEETINGSTONE:                  //23
        {
            GameObjectInfo const* info = GetGOInfo();

            if (user->GetTypeId() != TYPEID_PLAYER)
                return;

            Player* player = (Player*)user;

            Player* targetPlayer = ObjectAccessor::FindPlayer(player->GetSelection());

            // accept only use by player from same raid as caster, except caster itself
            if (!targetPlayer || targetPlayer == player || !targetPlayer->IsInSameRaidWith(player))
                return;

            //required lvl checks!
            uint8 level = player->getLevel();
            if (level < info->meetingstone.minLevel)
                return;
            level = targetPlayer->getLevel();
            if (level < info->meetingstone.minLevel)
                return;

            if (info->id == 194097)
                spellId = 61994;                            // Ritual of Summoning
            else
                spellId = 59782;                            // Summoning Stone Effect

            break;
        }

        case GAMEOBJECT_TYPE_FLAGSTAND:                     // 24
        {
            if (user->GetTypeId() != TYPEID_PLAYER)
                return;

            Player* player = (Player*)user;

            player->RemoveAurasDueToBattlegroundObjectUse();

            if (player->CanUseBattlegroundObject())
            {
                // in battleground check
                Battleground *bg = player->GetBattleground();
                if (!bg)
                    return;
                if (player->GetVehicle())
                    return;
                // BG flag click
                // AB:
                // 15001
                // 15002
                // 15003
                // 15004
                // 15005
                bg->EventPlayerClickedOnFlag(player, this);
                return;                                     //we don;t need to delete flag ... it is despawned!
            }
            break;
        }

        case GAMEOBJECT_TYPE_FISHINGHOLE:                   // 25
        {
            if (user->GetTypeId() != TYPEID_PLAYER)
                return;

            Player* player = (Player*)user;

            player->SendLoot(GetGUID(), LOOT_FISHINGHOLE);
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT, GetGOInfo()->id);
            return;
        }

        case GAMEOBJECT_TYPE_FLAGDROP:                      // 26
        {
            if (user->GetTypeId() != TYPEID_PLAYER)
                return;

            Player* player = (Player*)user;

            player->RemoveAurasDueToBattlegroundObjectUse();

            if (player->CanUseBattlegroundObject())
            {
                // in battleground check
                Battleground *bg = player->GetBattleground();
                if (!bg)
                    return;
                if( player->GetVehicle())
                    return;
                // BG flag dropped
                // WS:
                // 179785 - Silverwing Flag
                // 179786 - Warsong Flag
                // EotS:
                // 184142 - Netherstorm Flag
                GameObjectInfo const* info = GetGOInfo();
                if (info)
                {
                    switch(info->id)
                    {
                        case 179785:                        // Silverwing Flag
                            // check if it's correct bg
                            if (bg->GetTypeID(true) == BATTLEGROUND_WS || bg->GetTypeID(true) == BATTLEGROUND_TP)
                                bg->EventPlayerClickedOnFlag(player, this);
                            break;
                        case 179786:                        // Warsong Flag
                            if (bg->GetTypeID(true) == BATTLEGROUND_WS || bg->GetTypeID(true) == BATTLEGROUND_TP)
                                bg->EventPlayerClickedOnFlag(player, this);
                            break;
                        case 184142:                        // Netherstorm Flag
                            if (bg->GetTypeID(bg->IsRandom()) == BATTLEGROUND_EY)
                                bg->EventPlayerClickedOnFlag(player, this);
                            break;
                    }
                }
                //this cause to call return, all flags must be deleted here!!
                spellId = 0;
                Delete();
            }
            break;
        }
        case GAMEOBJECT_TYPE_BARBER_CHAIR:                  //32
        {
            GameObjectInfo const* info = GetGOInfo();
            if (!info)
                return;

            if (user->GetTypeId() != TYPEID_PLAYER)
                return;

            Player* player = (Player*)user;

            // fallback, will always work
            player->TeleportTo(GetMapId(), GetPositionX(), GetPositionY(), GetPositionZ(), GetOrientation(),TELE_TO_NOT_LEAVE_TRANSPORT | TELE_TO_NOT_LEAVE_COMBAT | TELE_TO_NOT_UNSUMMON_PET);

            WorldPacket data(SMSG_ENABLE_BARBER_SHOP, 0);
            player->GetSession()->SendPacket(&data);

            player->SetStandState(UNIT_STAND_STATE_SIT_LOW_CHAIR+info->barberChair.chairheight);
            return;
        }
        default:
            sLog->outDebug("Unknown Object Type %u", GetGoType());
            break;
    }

    if (!spellId)
        return;

    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo)
    {
        if (user->GetTypeId() != TYPEID_PLAYER || !sOutdoorPvPMgr->HandleCustomSpell((Player*)user,spellId,this))
            sLog->outError("WORLD: unknown spell id %u at use action for gameobject (Entry: %u GoType: %u)", spellId,GetEntry(),GetGoType());
        else
            sLog->outDebug("WORLD: %u non-dbc spell was handled by OutdoorPvP", spellId);
        return;
    }

    if (spellCaster)
        spellCaster->CastSpell(user, spellInfo, triggered);
    else
        CastSpell(user, spellId);
}

void GameObject::CastSpell(Unit* target, uint32 spellId)
{
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo)
        return;

    bool self = false;
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (spellInfo->EffectImplicitTargetA[i] == TARGET_UNIT_CASTER)
        {
            self = true;
            break;
        }
    }

    if (self)
    {
        if (target)
            target->CastSpell(target, spellInfo, true);
        return;
    }

    //summon world trigger
    Creature* trigger = SummonTrigger(GetPositionX(), GetPositionY(), GetPositionZ(), 0, GetSpellCastTime(spellInfo) + 100);
    if (!trigger)
        return;

    if (Unit* owner = GetOwner())
    {
        trigger->setFaction(owner->getFaction());
        trigger->CastSpell(target ? target : trigger, spellInfo, true, 0, 0, owner->GetGUID());
    }
    else
    {
        trigger->setFaction(14);
        // Set owner guid for target if no owner avalible - needed by trigger auras
        // - trigger gets despawned and there's no caster avalible (see AuraEffect::TriggerSpell())
        trigger->CastSpell(target ? target : trigger, spellInfo, true, 0, 0, target ? target->GetGUID() : 0);
    }
}

void GameObject::SendCustomAnim(uint32 anim)
{
    WorldPacket data(SMSG_GAMEOBJECT_CUSTOM_ANIM,8+4);
    data << GetGUID();
    data << uint32(anim);
    SendMessageToSet(&data, true);
}

bool GameObject::IsInRange(float x, float y, float z, float radius) const
{
    uint32 displayId = GetUInt32Value(GAMEOBJECT_DISPLAYID);

    // Get native displayId for destructible buildings due to their indestructibility
    if (GetGOInfo()->type == GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING)
        displayId = GetGOInfo()->displayId;

    GameObjectDisplayInfoEntry const * info = sGameObjectDisplayInfoStore.LookupEntry(displayId);
    if (!info)
        return IsWithinDist3d(x, y, z, radius);

    float sinA = sin(GetOrientation());
    float cosA = cos(GetOrientation());
    float dx = x - GetPositionX();
    float dy = y - GetPositionY();
    float dz = z - GetPositionZ();
    float dist = sqrt(dx*dx + dy*dy);
    float sinB = dx / dist;
    float cosB = dy / dist;
    dx = dist * (cosA * cosB + sinA * sinB);
    dy = dist * (cosA * sinB - sinA * cosB);
    return dx < info->maxX + radius && dx > info->minX - radius
        && dy < info->maxY + radius && dy > info->minY - radius
        && dz < info->maxZ + radius && dz > info->minZ - radius;
}

void GameObject::TakenDamage(uint32 damage, Unit *who)
{
    if (!m_goValue->building.health)
        return;

    Player* pwho = NULL;
    if (who)
    {
        if (who->GetTypeId() == TYPEID_PLAYER)
            pwho = who->ToPlayer();
        else if (who->IsVehicle() && who->GetCharmerOrOwner())
            pwho = who->GetCharmerOrOwner()->ToPlayer();
    }

    if (m_goValue->building.health > damage)
        m_goValue->building.health -= damage;
    else
        m_goValue->building.health = 0;

    if (HasFlag(GAMEOBJECT_FLAGS, GO_FLAG_DAMAGED)) // from damaged to destroyed
    {
        uint8 hitType = BG_OBJECT_DMG_HIT_TYPE_HIGH_DAMAGED;
        if (!m_goValue->building.health)
        {
            RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_DAMAGED);
            SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);

            EventInform(m_goInfo->building.destroyedEvent);
            if (pwho)
                if (Battleground* bg = pwho->GetBattleground())
                    bg->DestroyGate(pwho, this, m_goInfo->building.destroyedEvent);
            hitType = BG_OBJECT_DMG_HIT_TYPE_JUST_DESTROYED;
            sScriptMgr->OnGameObjectDestroyed(pwho, this, m_goInfo->building.destroyedEvent);
        }
        if (pwho)
            if (Battleground* bg = pwho->GetBattleground())
                bg->EventPlayerDamagedGO(pwho, this, hitType, m_goInfo->building.destroyedEvent);
    }
    else // from intact to damaged
    {
        uint8 hitType = BG_OBJECT_DMG_HIT_TYPE_JUST_DAMAGED;
        if (m_goValue->building.health + damage < m_goInfo->building.intactNumHits + m_goInfo->building.damagedNumHits)
            hitType = BG_OBJECT_DMG_HIT_TYPE_DAMAGED;

        if (m_goValue->building.health <= m_goInfo->building.damagedNumHits)
        {
            if (!m_goInfo->building.destroyedDisplayId)
                m_goValue->building.health = m_goInfo->building.damagedNumHits;
            else if (!m_goValue->building.health)
                m_goValue->building.health = 1;

            SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_DAMAGED);

            EventInform(m_goInfo->building.damagedEvent);
            hitType = BG_OBJECT_DMG_HIT_TYPE_JUST_HIGH_DAMAGED;
        }
        if (pwho)
            if (Battleground* bg = pwho->GetBattleground())
                 bg->EventPlayerDamagedGO(pwho, this, hitType, m_goInfo->building.destroyedEvent);
    }
    SetGoAnimProgress(m_goValue->building.health*255/(m_goInfo->building.intactNumHits + m_goInfo->building.damagedNumHits));
}

void GameObject::Rebuild()
{
    RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_DAMAGED | GO_FLAG_DESTROYED);
    //SetUInt32Value(GAMEOBJECT_DISPLAYID, m_goInfo->displayId);
    m_goValue->building.health = m_goInfo->building.intactNumHits + m_goInfo->building.damagedNumHits;
    EventInform(m_goInfo->building.rebuildingEvent);
}

void GameObject::EventInform(uint32 eventId)
{
    if (eventId)
    {
        if (m_zoneScript)
            m_zoneScript->ProcessEvent(this, eventId);
        else if (Battlefield* bf = sBattlefieldMgr.GetBattlefieldToZoneId(GetZoneId()))
            bf->ProcessEvent(this, eventId);
    }
}

// overwrite WorldObject function for proper name localization
const char* GameObject::GetNameForLocaleIdx(LocaleConstant loc_idx) const
{
    if (loc_idx != DEFAULT_LOCALE)
    {
        uint8 uloc_idx = uint8(loc_idx);
        if (GameObjectLocale const *cl = sObjectMgr->GetGameObjectLocale(GetEntry()))
            if (cl->Name.size() > uloc_idx && !cl->Name[uloc_idx].empty())
                return cl->Name[uloc_idx].c_str();
    }

    return GetName();
}

void GameObject::UpdateRotationFields(float rotation2 /*=0.0f*/, float rotation3 /*=0.0f*/)
{
    static double const atan_pow = atan(pow(2.0f, -20.0f));

    double f_rot1 = sin(GetOrientation() / 2.0f);
    double f_rot2 = cos(GetOrientation() / 2.0f);

    int64 i_rot1 = int64(f_rot1 / atan_pow *(f_rot2 >= 0 ? 1.0f : -1.0f));
    int64 rotation = (i_rot1 << 43 >> 43) & 0x00000000001FFFFF;

    //float f_rot2 = sin(0.0f / 2.0f);
    //int64 i_rot2 = f_rot2 / atan(pow(2.0f, -20.0f));
    //rotation |= (((i_rot2 << 22) >> 32) >> 11) & 0x000003FFFFE00000;

    //float f_rot3 = sin(0.0f / 2.0f);
    //int64 i_rot3 = f_rot3 / atan(pow(2.0f, -21.0f));
    //rotation |= (i_rot3 >> 42) & 0x7FFFFC0000000000;

    m_rotation = rotation;

    if (rotation2 == 0.0f && rotation3 == 0.0f)
    {
        rotation2 = (float)f_rot1;
        rotation3 = (float)f_rot2;
    }

    SetFloatValue(GAMEOBJECT_PARENTROTATION+2, rotation2);
    SetFloatValue(GAMEOBJECT_PARENTROTATION+3, rotation3);
}


void GameObject::ModifyHealth(int32 change, Unit* attackerOrHealer /*= NULL*/, uint32 spellId /*= 0*/)
{
    if (!GetGOValue()->building.maxhealth || !change)
        return;

    // prevent double destructions of the same object
    if (change < 0 && !GetGOValue()->building.health)
        return;

    if (int32(GetGOValue()->building.health) + change <= 0)
        GetGOValue()->building.health = 0;
    else if (int32(GetGOValue()->building.health) + change >= int32(GetGOValue()->building.maxhealth))
        GetGOValue()->building.health = GetGOValue()->building.maxhealth;
    else
        GetGOValue()->building.health += change;

    // Set the health bar, value = 255 * healthPct;
    SetGoAnimProgress(GetGOValue()->building.health * 255 / GetGOValue()->building.maxhealth);

    Player* player = attackerOrHealer->GetCharmerOrOwnerPlayerOrPlayerItself();

    EventInform(GetGOInfo()->building.damageEvent);

    // dealing damage, send packet
    // TODO: is there any packet for healing?
    if (change < 0 && player)
    {
        WorldPacket data(SMSG_DESTRUCTIBLE_BUILDING_DAMAGE, 8 + 8 + 8 + 4 + 4);
        data.appendPackGUID(GetGUID());
        data.appendPackGUID(attackerOrHealer->GetGUID());
        data.appendPackGUID(player->GetGUID());
        data << uint32(-change);
        data << uint32(spellId);
        player->GetSession()->SendPacket(&data);
    }

    GameObjectDestructibleState newState = GetDestructibleState();

    if (!GetGOValue()->building.health)
        newState = GO_DESTRUCTIBLE_DESTROYED;
    else if (GetGOValue()->building.health <= GetGOInfo()->building.damagedNumHits)
        newState = GO_DESTRUCTIBLE_DAMAGED;
    else if (GetGOValue()->building.health == GetGOValue()->building.maxhealth)
        newState = GO_DESTRUCTIBLE_INTACT;

    if (newState == GetDestructibleState())
        return;

    SetDestructibleState(newState, player, false);
}

void GameObject::SetDestructibleState(GameObjectDestructibleState state, Player* eventInvoker /*= NULL*/, bool setHealth /*= false*/)
{
    // the user calling this must know he is already operating on destructible gameobject
    if (GetGoType() != GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING)
        return;

    switch (state)
    {
        case GO_DESTRUCTIBLE_INTACT:
            RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_DAMAGED | GO_FLAG_DESTROYED);
            SetDisplayId(m_goInfo->displayId);
            if (setHealth)
            {
                m_goValue->building.health = m_goValue->building.maxhealth;
                SetGoAnimProgress(255);
            }
            EnableCollision(true);
            break;
        case GO_DESTRUCTIBLE_DAMAGED:
        {
            EventInform(m_goInfo->building.damagedEvent);
            if (eventInvoker)
            {
                sScriptMgr->OnGameObjectDamaged(eventInvoker, this, 0);
                if (Battleground* bg = eventInvoker->GetBattleground())
                    bg->EventPlayerDamagedGO(eventInvoker, this, BG_OBJECT_DMG_HIT_TYPE_JUST_HIGH_DAMAGED, m_goInfo->building.damagedEvent);
            }

            RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);
            SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_DAMAGED);

            uint32 modelId = m_goInfo->building.damagedDisplayId;
            if (DestructibleModelDataEntry const* modelData = sDestructibleModelDataStore.LookupEntry(m_goInfo->building.destructibleData))
                if (modelData->DamagedDisplayId)
                    modelId = modelData->DamagedDisplayId;
            SetDisplayId(modelId);

            if (setHealth)
            {
                m_goValue->building.health = m_goInfo->building.damagedNumHits;
                uint32 maxHealth = m_goValue->building.maxhealth;
                // in this case current health is 0 anyway so just prevent crashing here
                if (!maxHealth)
                    maxHealth = 1;
                SetGoAnimProgress(m_goValue->building.health * 255 / maxHealth);
            }
            break;
        }
        case GO_DESTRUCTIBLE_DESTROYED:
        {
            EventInform(m_goInfo->building.destroyedEvent);
            if (eventInvoker)
            {
                sScriptMgr->OnGameObjectDestroyed(eventInvoker, this, 0);
                if (Battleground* bg = eventInvoker->GetBattleground())
                {
                    bg->EventPlayerDamagedGO(eventInvoker, this, BG_OBJECT_DMG_HIT_TYPE_JUST_DESTROYED, m_goInfo->building.destroyedEvent);
                    bg->DestroyGate(eventInvoker, this, m_goInfo->building.destroyedEvent);
                }
            }

            RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_DAMAGED);
            SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);

            uint32 modelId = m_goInfo->building.destroyedDisplayId;
            if (DestructibleModelDataEntry const* modelData = sDestructibleModelDataStore.LookupEntry(m_goInfo->building.destructibleData))
                if (modelData->DestroyedDisplayId)
                    modelId = modelData->DestroyedDisplayId;
            SetDisplayId(modelId);

            if (setHealth)
            {
                m_goValue->building.health = 0;
                SetGoAnimProgress(0);
            }
            EnableCollision(false);
            break;
        }
        case GO_DESTRUCTIBLE_REBUILDING:
        {
            EventInform(m_goInfo->building.rebuildingEvent);
            RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_DAMAGED | GO_FLAG_DESTROYED);

            uint32 modelId = m_goInfo->displayId;
            if (DestructibleModelDataEntry const* modelData = sDestructibleModelDataStore.LookupEntry(m_goInfo->building.destructibleData))
                if (modelData->RebuildingDisplayId)
                    modelId = modelData->RebuildingDisplayId;
            SetDisplayId(modelId);

            // restores to full health
            if (setHealth)
            {
                m_goValue->building.health = m_goValue->building.maxhealth;
                SetGoAnimProgress(255);
            }
            EnableCollision(true);
            break;
        }
    }
}

void GameObject::SetLootState(LootState s)
{
    m_lootState = s;
    if (m_model && GetGOData() && GetGOInfo())
    {
        // startOpen determines whether we are going to add or remove the LoS on activation
        bool startOpen = ((GetGoType() == GAMEOBJECT_TYPE_DOOR || GetGoType() == GAMEOBJECT_TYPE_BUTTON) ? (GetGOInfo()->door.startOpen != 0) : false);

        // Use the current go state
        if (GetGoState() != GO_STATE_READY)
            startOpen = !startOpen;

        if (s == GO_ACTIVATED || s == GO_JUST_DEACTIVATED)
            EnableCollision(startOpen);
        else if (s == GO_READY)
            EnableCollision(!startOpen);
    }
}

void GameObject::SetGoState(GOState state)
{
    SetByteValue(GAMEOBJECT_BYTES_1, 0, state);
    if (m_model && GetGOData() && GetGOInfo())
    {
        if (!IsInWorld())
            return;

        // startOpen determines whether we are going to add or remove the LoS on activation
        bool startOpen = ((GetGoType() == GAMEOBJECT_TYPE_DOOR || GetGoType() == GAMEOBJECT_TYPE_BUTTON) ? (GetGOInfo()->door.startOpen != 0) : false);

        if (GetGOData() && GetGOData()->go_state == GO_STATE_READY)
            startOpen = !startOpen;

        if (state == GO_STATE_ACTIVE || state == GO_STATE_ACTIVE_ALTERNATIVE)
            EnableCollision(startOpen);
        else if (state == GO_STATE_READY)
            EnableCollision(!startOpen);
    }
}

uint32 GameObject::GetTransportPeriod() const
{
    ASSERT(GetGOInfo()->type == GAMEOBJECT_TYPE_TRANSPORT);
    if (m_goValue->Transport.AnimationInfo)
        return m_goValue->Transport.AnimationInfo->TotalTime;

    // return something that will nicely divide for GAMEOBJECT_DYNAMIC value calculation
    return m_goValue->Transport.PathProgress;
}

void GameObject::SetTransportState(GOState state, uint32 stopFrame /*= 0*/)
{
    if (GetGoState() == state)
        return;

    ASSERT(GetGOInfo()->type == GAMEOBJECT_TYPE_TRANSPORT);
    ASSERT(state >= GO_STATE_TRANSPORT_ACTIVE);
    if (state == GO_STATE_TRANSPORT_ACTIVE)
    {
        m_goValue->Transport.StateUpdateTimer = 0;
        m_goValue->Transport.PathProgress = getMSTime();
        if (GetGoState() >= GO_STATE_TRANSPORT_STOPPED)
            m_goValue->Transport.StopFrames->at(GetGoState() - GO_STATE_TRANSPORT_STOPPED);
        SetGoState(GO_STATE_TRANSPORT_ACTIVE);
    }
    else
    {
        if (state < GO_STATE_TRANSPORT_STOPPED + MAX_GO_STATE_TRANSPORT_STOP_FRAMES && stopFrame < m_goValue->Transport.StopFrames->size())
        {
            m_goValue->Transport.PathProgress = getMSTime() + m_goValue->Transport.StopFrames->at(stopFrame);
            SetGoState(GOState(GO_STATE_TRANSPORT_STOPPED + stopFrame));
        }
    }
}

void GameObject::SetDisplayId(uint32 displayid)
{
    SetUInt32Value(GAMEOBJECT_DISPLAYID, displayid);
    UpdateModel();
}

void GameObject::SetPhaseMask(uint32 newPhaseMask, bool update)
{
    WorldObject::SetPhaseMask(newPhaseMask, update);
    EnableCollision(true);
}

void GameObject::GetRespawnPosition(float &x, float &y, float &z, float* ori /* = NULL*/) const
{
    if (m_DBTableGuid)
    {
        if (GameObjectData const* data = sObjectMgr->GetGOData(GetDBTableGUIDLow()))
        {
            x = data->posX;
            y = data->posY;
            z = data->posZ;
            if (ori)
                *ori = data->orientation;
            return;
        }
    }

    x = GetPositionX();
    y = GetPositionY();
    z = GetPositionZ();
    if (ori)
        *ori = GetOrientation();
}

void GameObject::EnableCollision(bool enable)
{
    if (!m_model)
        return;

    /*if (enable && !GetMap()->Contains(*m_model))
        GetMap()->Insert(*m_model);*/

    m_model->enable(enable ? GetPhaseMask() : 0);
}

void GameObject::UpdateModel()
{
    if (!IsInWorld())
        return;

    if (m_model)
        if (GetMap()->Contains(*m_model))
            GetMap()->Remove(*m_model);

    delete m_model;
    m_model = GameObjectModel::Create(*this);

    if (m_model)
        GetMap()->Insert(*m_model);
}

