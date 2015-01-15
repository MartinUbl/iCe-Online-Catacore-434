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
#include "BattlegroundRV.h"
#include "ObjectAccessor.h"
#include "Language.h"
#include "Player.h"
#include "WorldPacket.h"
#include "GameObject.h"

BattlegroundRV::BattlegroundRV()
{
    m_BgObjects.resize(BG_RV_OBJECT_MAX);

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

BattlegroundRV::~BattlegroundRV()
{

}

void BattlegroundRV::Update(uint32 diff)
{
    BattlegroundArena::Update(diff);

    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    if (GetElapsedTime() >= 47*MINUTE*IN_MILLISECONDS)    // after 47 minutes without one team losing, the arena closes with no winner and no rating change
    {
        UpdateArenaWorldState();
        CheckArenaAfterTimerConditions();
    }

    if (getTimer() < diff)
    {
        uint32 i;
        switch (getState())
        {
            case BG_RV_STATE_CLOSE_FIRE:
                for (i = BG_RV_OBJECT_FIRE_1; i <= BG_RV_OBJECT_FIREDOOR_2; ++i)
                    DoorClose(i);
                setTimer(BG_RV_FIRE_TO_PILAR_TIMER);
                setState(BG_RV_STATE_OPEN_PILARS);
                break;
            case BG_RV_STATE_OPEN_PILARS:

                // change state of pillars
                for (i = 0; i < 4; i++)
                {
                    // activated by pairs, 1+3, 0+2
                    if (GameObject* gob = GetBgMap()->GetGameObject(m_BgObjects[BG_RV_OBJECT_PILAR_1 + i]))
                    {
                        gob->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_TRANSPORT);
                        gob->ForceValuesUpdateAtIndex(GAMEOBJECT_FLAGS);
                        gob->SetTransportState(GO_STATE_TRANSPORT_STOPPED, (i + 1) % 2/*, 6966*/);
                    }
                }

                for (i = BG_RV_OBJECT_PULLEY_1; i <= BG_RV_OBJECT_PULLEY_2; ++i)
                    DoorOpen(i);
                for (i = BG_RV_OBJECT_GEAR_1; i <= BG_RV_OBJECT_GEAR_2; ++i)
                    DoorClose(i);

                setTimer(BG_RV_PILAR_TO_FIRE_TIMER);
                setState(BG_RV_STATE_OPEN_FIRE);
                break;
            case BG_RV_STATE_OPEN_FIRE:
                for (i = BG_RV_OBJECT_FIRE_1; i <= BG_RV_OBJECT_FIREDOOR_2; ++i)
                    DoorOpen(i);
                setTimer(BG_RV_FIRE_TO_PILAR_TIMER);
                setState(BG_RV_STATE_CLOSE_PILARS);
                break;
            case BG_RV_STATE_CLOSE_PILARS:

                // change state of pillars
                for (i = 0; i < 4; i++)
                {
                    // activated by pairs, 1+3, 0+2; change to opposite state (i+1)%2
                    if (GameObject* gob = GetBgMap()->GetGameObject(m_BgObjects[BG_RV_OBJECT_PILAR_1 + i]))
                    {
                        gob->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_TRANSPORT);
                        gob->ForceValuesUpdateAtIndex(GAMEOBJECT_FLAGS);
                        gob->SetTransportState(GO_STATE_TRANSPORT_STOPPED, i % 2/*, 6966*/);
                    }
                }

                for (i = BG_RV_OBJECT_PULLEY_1; i <= BG_RV_OBJECT_PULLEY_2; ++i)
                    DoorClose(i);
                for (i = BG_RV_OBJECT_GEAR_1; i <= BG_RV_OBJECT_GEAR_2; ++i)
                    DoorOpen(i);

                setTimer(BG_RV_PILAR_TO_FIRE_TIMER);
                setState(BG_RV_STATE_CLOSE_FIRE);
                break;
        }
    }
    else
        setTimer(getTimer() - diff);
}

void BattlegroundRV::StartingEventCloseDoors()
{
}

void BattlegroundRV::StartingEventOpenDoors()
{
    // Buff respawn
    SpawnBGObject(BG_RV_OBJECT_BUFF_1, 90);
    SpawnBGObject(BG_RV_OBJECT_BUFF_2, 90);

    // Elevators
    DoorOpen(BG_RV_OBJECT_DOOR_1);
    DoorOpen(BG_RV_OBJECT_DOOR_2);

    setTimer(BG_RV_PILAR_TO_FIRE_TIMER);
    setState(BG_RV_STATE_CLOSE_FIRE);
}

void BattlegroundRV::AddPlayer(Player *plr)
{
    Battleground::AddPlayer(plr);
    //create score and add it to map, default values are set in constructor
    BattlegroundRVScore* sc = new BattlegroundRVScore;

    m_PlayerScores[plr->GetGUID()] = sc;

    UpdateWorldState(BG_RV_WORLD_STATE_A, GetAlivePlayersCountByTeam(ALLIANCE));
    UpdateWorldState(BG_RV_WORLD_STATE_H, GetAlivePlayersCountByTeam(HORDE));
}

void BattlegroundRV::RemovePlayer(Player * /*plr*/, uint64 /*guid*/)
{
    if (GetStatus() == STATUS_WAIT_LEAVE)
        return;

    UpdateWorldState(BG_RV_WORLD_STATE_A, GetAlivePlayersCountByTeam(ALLIANCE));
    UpdateWorldState(BG_RV_WORLD_STATE_H, GetAlivePlayersCountByTeam(HORDE));

    CheckArenaWinConditions();
}

void BattlegroundRV::HandleKillPlayer(Player *player, Player *killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    if (!killer)
    {
        sLog->outError("BattlegroundRV: Killer player not found");
        return;
    }

    Battleground::HandleKillPlayer(player, killer);

    UpdateWorldState(BG_RV_WORLD_STATE_A, GetAlivePlayersCountByTeam(ALLIANCE));
    UpdateWorldState(BG_RV_WORLD_STATE_H, GetAlivePlayersCountByTeam(HORDE));

    CheckArenaWinConditions();
}

bool BattlegroundRV::GetUnderMapLimitZPosition(float &z)
{
    z = 27.5f;
    return true;
}

bool BattlegroundRV::GetUnderMapReturnPosition(Player* plr, Position& pos)
{
    pos.Relocate(763.5f, -284, 28.276f, 2.422f);
    return true;
}

static bool circleLineIntersection(float ax, float ay, float bx, float by, float cpx, float cpy, float circleRadius)
{
    float a = ay - by;
    float b = bx - ax;
    float c1 = -a*ax - b*ay;
    float c2 = a*cpy - b*cpx;

    float in_y = (a*c2 - b*c1) / (a*a + b*b);
    float in_x = (-b*in_y - c1) / a;

    float dist = sqrt(std::pow(in_x - cpx, 2) + std::pow(in_y - cpy, 2));

    return (dist <= circleRadius);
}

bool BattlegroundRV::CheckSpecialLOS(float ax, float ay, float az, float bx, float by, float bz)
{
    for (int i = 0; i < 4; i++)
    {
        if (GameObject* gob = GetBgMap()->GetGameObject(m_BgObjects[BG_RV_OBJECT_PILAR_1 + i]))
        {
            if (gob->GetGoState() == GO_STATE_TRANSPORT_STOPPED + 1)
            {
                float rad = 4.3f;
                // pillars _1 and _3 have smaller radius
                if (i == 0 || i == 2)
                    rad = 2.24f;

                if (circleLineIntersection(ax, ay, bx, by, gob->GetPositionX(), gob->GetPositionY(), rad))
                    return false;
            }
        }
    }

    return true;
}

void BattlegroundRV::HandleAreaTrigger(Player *Source, uint32 Trigger)
{
    // We won't handle any areatriggers in this arena

    return;
}

void BattlegroundRV::FillInitialWorldStates(WorldPacket &data)
{
    data << uint32(BG_RV_WORLD_STATE_A) << uint32(GetAlivePlayersCountByTeam(ALLIANCE));
    data << uint32(BG_RV_WORLD_STATE_H) << uint32(GetAlivePlayersCountByTeam(HORDE));
    data << uint32(BG_RV_WORLD_STATE) << uint32(1);
}

void BattlegroundRV::Reset()
{
    //call parent's class reset
    Battleground::Reset();
}

bool BattlegroundRV::SetupBattleground()
{
    if (
    // doors
        !AddObject(BG_RV_OBJECT_DOOR_1, BG_RV_OBJECT_TYPE_PORTCULLIS, 818.889f, -297.079010f, 28.632999f, 2.647531f, 0, M_PI/2, M_PI/2, M_PI/2, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_DOOR_2, BG_RV_OBJECT_TYPE_PORTCULLIS, 708.281311f, -269.222046f, 28.626377f, 5.7000754f, 0, M_PI/2, M_PI/2, M_PI/2, RESPAWN_IMMEDIATELY)
    // buffs
        || !AddObject(BG_RV_OBJECT_BUFF_1, BG_RV_OBJECT_TYPE_BUFF_1, 735.551819f, -284.794678f, 28.276682f, 0.034906f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_BUFF_2, BG_RV_OBJECT_TYPE_BUFF_2, 791.224487f, -284.794464f, 28.276682f, 2.600535f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
    // fire
        || !AddObject(BG_RV_OBJECT_FIRE_1, BG_RV_OBJECT_TYPE_FIRE_1, 743.543457f, -283.799469f, 28.286655f, 3.141593f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_FIRE_2, BG_RV_OBJECT_TYPE_FIRE_2, 782.971802f, -283.799469f, 28.286655f, 3.141593f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_FIREDOOR_1, BG_RV_OBJECT_TYPE_FIREDOOR_1, 743.711060f, -284.099609f, 27.542587f, 3.141593f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_FIREDOOR_2, BG_RV_OBJECT_TYPE_FIREDOOR_2, 783.221252f, -284.133362f, 27.535686f, 0.000000f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
    // Gear
        || !AddObject(BG_RV_OBJECT_GEAR_1, BG_RV_OBJECT_TYPE_GEAR_1, 763.664551f, -261.872986f, 26.686588f, 0.000000f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_GEAR_2, BG_RV_OBJECT_TYPE_GEAR_2, 763.578979f, -306.146149f, 26.665222f, 3.141593f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
    // Pulley
        || !AddObject(BG_RV_OBJECT_PULLEY_1, BG_RV_OBJECT_TYPE_PULLEY_1, 700.722290f, -283.990662f, 39.517582f, 3.141593f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_PULLEY_2, BG_RV_OBJECT_TYPE_PULLEY_2, 826.303833f, -283.996429f, 39.517582f, 0.000000f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
    // Pilars
        || !AddObject(BG_RV_OBJECT_PILAR_1, BG_RV_OBJECT_TYPE_PILAR_1, 763.632385f, -306.162384f, 25.909504f, 3.141593f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_PILAR_2, BG_RV_OBJECT_TYPE_PILAR_2, 723.644287f, -284.493256f, 24.648525f, 3.141593f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_PILAR_3, BG_RV_OBJECT_TYPE_PILAR_3, 763.611145f, -261.856750f, 25.909504f, 0.000000f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_PILAR_4, BG_RV_OBJECT_TYPE_PILAR_4, 802.211609f, -284.493256f, 24.648525f, 0.000000f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
    // Pilars Collision
    // Do not spawn pillar collision objects, they are probably no longer needed
    /*
        || !AddObject(BG_RV_OBJECT_PILAR_COLLISION_1, BG_RV_OBJECT_TYPE_PILAR_COLLISION_1, 763.632385f, -306.162384f, 30.639660f, 3.141593f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_PILAR_COLLISION_2, BG_RV_OBJECT_TYPE_PILAR_COLLISION_2, 723.644287f, -284.493256f, 32.382710f, 0.000000f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_PILAR_COLLISION_3, BG_RV_OBJECT_TYPE_PILAR_COLLISION_3, 763.611145f, -261.856750f, 30.639660f, 0.000000f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_RV_OBJECT_PILAR_COLLISION_4, BG_RV_OBJECT_TYPE_PILAR_COLLISION_4, 802.211609f, -284.493256f, 32.382710f, 3.141593f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY)
    */
)
    {
        sLog->outErrorDb("BatteGroundRV: Failed to spawn some object!");
        return false;
    }

    // the pillars have to be stopped at the beginning
    for (uint8 i = BG_RV_OBJECT_PILAR_1; i <= BG_RV_OBJECT_PILAR_4; ++i)
    {
        if (GameObject* gob = GetBgMap()->GetGameObject(m_BgObjects[i]))
        {
            gob->SetTransportState(GO_STATE_TRANSPORT_STOPPED, 0);
            // disable collision - the LoS will be handled by our special method
            gob->EnableCollision(false);
        }
    }

    return true;
}

void BattlegroundRV::TogglePillarCollision(bool apply)
{
    for (uint8 i = BG_RV_OBJECT_PILAR_COLLISION_1; i <= BG_RV_OBJECT_PILAR_COLLISION_4; ++i)
    {
        if (GameObject* gob = GetBgMap()->GetGameObject(m_BgObjects[i]))
        {
            uint32 _state = GO_STATE_READY;
            if (gob->GetGOInfo()->door.startOpen)
                _state = GO_STATE_ACTIVE;
            gob->SetGoState(apply ? (GOState)_state : (GOState)(!_state));

            if (apply)
                gob->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE | GO_FLAG_TRANSPORT);
            else
                gob->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE | GO_FLAG_TRANSPORT);

            if (gob->GetGOInfo()->door.startOpen)
                gob->EnableCollision(apply); // Forced collision toggle

            for (BattlegroundPlayerMap::iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
                if (Player* player = ObjectAccessor::FindPlayer(MAKE_NEW_GUID(itr->first, 0, HIGHGUID_PLAYER)))
                    gob->SendUpdateToPlayer(player);
        }
    }
}
