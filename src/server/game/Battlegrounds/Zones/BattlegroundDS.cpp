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
#include "Battleground.h"
#include "BattlegroundDS.h"
#include "Language.h"
#include "Player.h"
#include "Object.h"
#include "ObjectMgr.h"
#include "WorldPacket.h"

BattlegroundDS::BattlegroundDS()
{
    m_BgObjects.resize(BG_DS_OBJECT_MAX);
    m_BgCreatures.resize(BG_DS_NPC_MAX);

    m_StartDelayTimes[BG_STARTING_EVENT_FIRST]  = BG_START_DELAY_1M;
    m_StartDelayTimes[BG_STARTING_EVENT_SECOND] = BG_START_DELAY_30S;
    m_StartDelayTimes[BG_STARTING_EVENT_THIRD]  = BG_START_DELAY_15S;
    m_StartDelayTimes[BG_STARTING_EVENT_FOURTH] = BG_START_DELAY_NONE;
    //we must set messageIds
    m_StartMessageIds[BG_STARTING_EVENT_FIRST]  = LANG_ARENA_ONE_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_ARENA_THIRTY_SECONDS;
    m_StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_ARENA_FIFTEEN_SECONDS;
    m_StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_ARENA_HAS_BEGUN;
}

BattlegroundDS::~BattlegroundDS()
{

}

void BattlegroundDS::Update(uint32 diff)
{
    Battleground::Update(diff);

    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    if (GetElapsedTime() >= 47*MINUTE*IN_MILLISECONDS)    // after 47 minutes without one team losing, the arena closes with no winner and no rating change
    {
        UpdateArenaWorldState();
        CheckArenaAfterTimerConditions();
    }

    if (getPipeKnockBackTimer() < diff)
    {
        if (getPipeKnockBackCount() < BG_DS_PIPE_KNOCKBACK_TOTAL_COUNT)
        {
            for (uint32 i = BG_DS_NPC_PIPE_KNOCKBACK_1; i <= BG_DS_NPC_PIPE_KNOCKBACK_2; ++i)
                if (Creature* waterSpout = GetBgMap()->GetCreature(m_BgCreatures[i]))
                    waterSpout->CastSpell(waterSpout, BG_DS_SPELL_FLUSH, true);

            setPipeKnockBackCount(getPipeKnockBackCount() + 1);
            setPipeKnockBackTimer(BG_DS_PIPE_KNOCKBACK_DELAY);

            // if next step would be teleporting player, then set 15s timer
            if (getPipeKnockBackCount() >= BG_DS_PIPE_KNOCKBACK_TOTAL_COUNT)
                setPipeKnockBackTimer(BG_DS_PIPE_TELEPORT_DELAY);
        }
        else
        {
            // only check for players in pipe, that haven't been kicked out by flush
            // 15s timer to repeat check
            setPipeKnockBackTimer(BG_DS_PIPE_TELEPORT_DELAY);

            for (BattlegroundPlayerMap::const_iterator itr = GetPlayers().begin(); itr != GetPlayers().end(); ++itr)
                if (Player* player = ObjectAccessor::FindPlayer(itr->first))
                    // z > 13 and X positions out of arena square bounds are considered as "in pipe"
                    if (player->GetPositionZ() > 13.0f && (player->GetPositionX() < 1250.3f || player->GetPositionX() > 1333.0f))
                        HandlePlayerUnderMap(player);
        }
    }
    else
        setPipeKnockBackTimer(getPipeKnockBackTimer() - diff);

    if (getWaterFallStatus() == BG_DS_WATERFALL_STATUS_ON) // Repeat knockback while the waterfall still active
    {
        if (getWaterFallKnockbackTimer() < diff)
        {
            if (Creature* waterSpout = GetBgMap()->GetCreature(m_BgCreatures[BG_DS_NPC_WATERFALL_KNOCKBACK]))
                waterSpout->CastSpell(waterSpout, BG_DS_SPELL_WATER_SPOUT, true);

            setWaterFallKnockbackTimer(BG_DS_WATERFALL_KNOCKBACK_TIMER);
        }
        else
            setWaterFallKnockbackTimer(getWaterFallKnockbackTimer() - diff);
    }

    if (getWaterFallTimer() < diff)
    {
        if (getWaterFallStatus() == BG_DS_WATERFALL_STATUS_OFF) // Add the water
        {
            DoorClose(BG_DS_OBJECT_WATER_2);
            setWaterFallTimer(BG_DS_WATERFALL_WARNING_DURATION);
            setWaterFallStatus(BG_DS_WATERFALL_STATUS_WARNING);
        }
        else if (getWaterFallStatus() == BG_DS_WATERFALL_STATUS_WARNING) // Active collision and start knockback timer
        {
            if (GameObject* gob = GetBgMap()->GetGameObject(m_BgObjects[BG_DS_OBJECT_WATER_1]))
            {
                gob->SetGoState(GO_STATE_READY);
                gob->EnableCollision(true);
            }

            setWaterFallTimer(BG_DS_WATERFALL_DURATION);
            setWaterFallStatus(BG_DS_WATERFALL_STATUS_ON);
            setWaterFallKnockbackTimer(BG_DS_WATERFALL_KNOCKBACK_TIMER);
        }
        else //if (getWaterFallStatus() == BG_DS_WATERFALL_STATUS_ON) // Remove collision and water
        {
            // turn off collision
            if (GameObject* gob = GetBgMap()->GetGameObject(m_BgObjects[BG_DS_OBJECT_WATER_1]))
            {
                gob->SetGoState(GO_STATE_ACTIVE);
                gob->EnableCollision(false);
            }

            DoorOpen(BG_DS_OBJECT_WATER_2);
            setWaterFallTimer(urand(BG_DS_WATERFALL_TIMER_MIN, BG_DS_WATERFALL_TIMER_MAX));
            setWaterFallStatus(BG_DS_WATERFALL_STATUS_OFF);
        }
    }
    else
        setWaterFallTimer(getWaterFallTimer() - diff);
}

void BattlegroundDS::StartingEventCloseDoors()
{
    for (uint32 i = BG_DS_OBJECT_DOOR_1; i <= BG_DS_OBJECT_DOOR_2; ++i)
        SpawnBGObject(i, RESPAWN_IMMEDIATELY);
}

void BattlegroundDS::StartingEventOpenDoors()
{
    for (uint32 i = BG_DS_OBJECT_DOOR_1; i <= BG_DS_OBJECT_DOOR_2; ++i)
        DoorOpen(i);

    for (uint32 i = BG_DS_OBJECT_BUFF_1; i <= BG_DS_OBJECT_BUFF_2; ++i)
        SpawnBGObject(i, 60);

    setWaterFallTimer(urand(BG_DS_WATERFALL_TIMER_MIN, BG_DS_WATERFALL_TIMER_MAX));
    setWaterFallStatus(BG_DS_WATERFALL_STATUS_OFF);

    setPipeKnockBackTimer(BG_DS_PIPE_KNOCKBACK_FIRST_DELAY);
    setPipeKnockBackCount(0);

    SpawnBGObject(BG_DS_OBJECT_WATER_2, RESPAWN_IMMEDIATELY);
    DoorOpen(BG_DS_OBJECT_WATER_2);

    // Turn off collision
    if (GameObject* gob = GetBgMap()->GetGameObject(m_BgObjects[BG_DS_OBJECT_WATER_1]))
        gob->SetGoState(GO_STATE_ACTIVE);

    // Remove effects of Demonic Circle Summon
    for (BattlegroundPlayerMap::const_iterator itr = GetPlayers().begin(); itr != GetPlayers().end(); ++itr)
        if (Player* player = ObjectAccessor::FindPlayer(itr->first))
            if (player->HasAura(48018))
                player->RemoveAurasDueToSpell(48018);
}

void BattlegroundDS::AddPlayer(Player *plr)
{
    Battleground::AddPlayer(plr);
    //create score and add it to map, default values are set in constructor
    BattlegroundDSScore* sc = new BattlegroundDSScore;

    m_PlayerScores[plr->GetGUID()] = sc;

    UpdateArenaWorldState();
}

void BattlegroundDS::RemovePlayer(Player * /*plr*/, uint64 /*guid*/)
{
    if (GetStatus() == STATUS_WAIT_LEAVE)
        return;

    UpdateArenaWorldState();
    CheckArenaWinConditions();
}

void BattlegroundDS::HandleKillPlayer(Player* player, Player* killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    if (!killer)
    {
        sLog->outError("BattlegroundDS: Killer player not found");
        return;
    }

    Battleground::HandleKillPlayer(player,killer);

    UpdateArenaWorldState();
    CheckArenaWinConditions();
}

void BattlegroundDS::HandleAreaTrigger(Player *Source, uint32 Trigger)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    switch(Trigger)
    {
        case 5347:
        case 5348:
            // Remove effects of Demonic Circle Summon
            if (Source->HasAura(48018))
                Source->RemoveAurasDueToSpell(48018);

            // Someone has get back into the pipes and the knockback has already been performed,
            // so we reset the knockback count for kicking the player again into the arena.
            if (getPipeKnockBackCount() >= BG_DS_PIPE_KNOCKBACK_TOTAL_COUNT)
            {
                setPipeKnockBackCount(0);
                setPipeKnockBackTimer(BG_DS_PIPE_KNOCKBACK_DELAY);
            }
            break;
        default:
            sLog->outError("WARNING: Unhandled AreaTrigger in Battleground: %u", Trigger);
            Source->GetSession()->SendAreaTriggerMessage("Warning: Unhandled AreaTrigger in Battleground: %u", Trigger);
            break;
    }
}

bool BattlegroundDS::GetUnderMapReturnPosition(Player* plr, Position& pos)
{
    pos.Relocate(1299.046f, 784.825f, 9.338f, 2.422f);
    return true;
}

void BattlegroundDS::FillInitialWorldStates(WorldPacket &data)
{
    data << uint32(3610) << uint32(1);                                              // 9 show
    UpdateArenaWorldState();
}

void BattlegroundDS::Reset()
{
    //call parent's class reset
    Battleground::Reset();
}

bool BattlegroundDS::SetupBattleground()
{
    // gates
    if (!AddObject(BG_DS_OBJECT_DOOR_1, BG_DS_OBJECT_TYPE_DOOR_1, 1350.95f, 817.2f, 20.8096f, 3.15f, 0, 0, 0.99627f, 0.0862864f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_DS_OBJECT_DOOR_2, BG_DS_OBJECT_TYPE_DOOR_2, 1232.65f, 764.913f, 20.0729f, 6.3f, 0, 0, 0.0310211f, -0.999519f, RESPAWN_IMMEDIATELY)
    // water
        || !AddObject(BG_DS_OBJECT_WATER_1, BG_DS_OBJECT_TYPE_WATER_1, 1291.56f, 790.837f, 7.1f, 3.14238f, 0, 0, 0.694215f, -0.719768f, 120)
        || !AddObject(BG_DS_OBJECT_WATER_2, BG_DS_OBJECT_TYPE_WATER_2, 1291.56f, 790.837f, 7.1f, 3.14238f, 0, 0, 0.694215f, -0.719768f, 120)
    // buffs
        || !AddObject(BG_DS_OBJECT_BUFF_1, BG_DS_OBJECT_TYPE_BUFF_1, 1291.7f, 813.424f, 7.11472f, 4.64562f, 0, 0, 0.730314f, -0.683111f, 120)
        || !AddObject(BG_DS_OBJECT_BUFF_2, BG_DS_OBJECT_TYPE_BUFF_2, 1291.7f, 768.911f, 7.11472f, 1.55194f, 0, 0, 0.700409f, 0.713742f, 120)
    // knockback creatures
        || !AddCreature(BG_DS_NPC_TYPE_WATER_SPOUT, BG_DS_NPC_WATERFALL_KNOCKBACK, 0, 1292.587f, 790.2205f, 7.19796f, 3.054326f, RESPAWN_IMMEDIATELY)
        || !AddCreature(BG_DS_NPC_TYPE_WATER_SPOUT, BG_DS_NPC_PIPE_KNOCKBACK_1, 0, 1369.977f, 817.2882f, 16.08718f, 3.106686f, RESPAWN_IMMEDIATELY)
        || !AddCreature(BG_DS_NPC_TYPE_WATER_SPOUT, BG_DS_NPC_PIPE_KNOCKBACK_2, 0, 1212.833f, 765.3871f, 16.09484f, 0.0f, RESPAWN_IMMEDIATELY))
    {
        sLog->outErrorDb("BatteGroundDS: Failed to spawn some object!");
        return false;
    }

    return true;
}
