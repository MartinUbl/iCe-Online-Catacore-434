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

#ifndef __BATTLEGROUNDRV_H
#define __BATTLEGROUNDRV_H

class Battleground;

enum BattlegroundRVObjectTypes
{
    BG_RV_OBJECT_BUFF_1,
    BG_RV_OBJECT_BUFF_2,
    BG_RV_OBJECT_FIRE_1,
    BG_RV_OBJECT_FIRE_2,
    BG_RV_OBJECT_FIREDOOR_1,
    BG_RV_OBJECT_FIREDOOR_2,

    BG_RV_OBJECT_PILAR_1,
    BG_RV_OBJECT_PILAR_2,
    BG_RV_OBJECT_PILAR_3,
    BG_RV_OBJECT_PILAR_4,

    BG_RV_OBJECT_GEAR_1,
    BG_RV_OBJECT_GEAR_2,
    BG_RV_OBJECT_PULLEY_1,
    BG_RV_OBJECT_PULLEY_2,

    BG_RV_OBJECT_PILAR_COLLISION_1,
    BG_RV_OBJECT_PILAR_COLLISION_2,
    BG_RV_OBJECT_PILAR_COLLISION_3,
    BG_RV_OBJECT_PILAR_COLLISION_4,

    BG_RV_OBJECT_DOOR_1,
    BG_RV_OBJECT_DOOR_2,

    BG_RV_OBJECT_MAX,
};

enum BattlegroundRVObjects
{
    BG_RV_OBJECT_TYPE_BUFF_1                     = 184663,
    BG_RV_OBJECT_TYPE_BUFF_2                     = 184664,
    BG_RV_OBJECT_TYPE_FIRE_1                     = 192704,
    BG_RV_OBJECT_TYPE_FIRE_2                     = 192705,

    BG_RV_OBJECT_TYPE_FIREDOOR_2                 = 192387,
    BG_RV_OBJECT_TYPE_FIREDOOR_1                 = 192388,
    BG_RV_OBJECT_TYPE_PULLEY_1                   = 192389,
    BG_RV_OBJECT_TYPE_PULLEY_2                   = 192390,
    BG_RV_OBJECT_TYPE_GEAR_1                     = 192393,
    BG_RV_OBJECT_TYPE_GEAR_2                     = 192394,
    BG_RV_OBJECT_TYPE_PORTCULLIS                 = 203710,

    BG_RV_OBJECT_TYPE_PILAR_COLLISION_1          = 208466, // axe
    BG_RV_OBJECT_TYPE_PILAR_COLLISION_2          = 208465, // arena
    BG_RV_OBJECT_TYPE_PILAR_COLLISION_3          = 208467, // lightning
    BG_RV_OBJECT_TYPE_PILAR_COLLISION_4          = 208464, // ivory

    BG_RV_OBJECT_TYPE_PILAR_1                    = 208468, // axe
    BG_RV_OBJECT_TYPE_PILAR_2                    = 208469, // arena
    BG_RV_OBJECT_TYPE_PILAR_3                    = 208470, // lightning
    BG_RV_OBJECT_TYPE_PILAR_4                    = 208471, // ivory
};

enum BattlegroundRVData
{
    BG_RV_STATE_OPEN_PILARS,
    BG_RV_STATE_CLOSE_PILARS,
    BG_RV_STATE_OPEN_FIRE,
    BG_RV_STATE_CLOSE_FIRE,
    BG_RV_FIRE_TO_PILAR_TIMER                    = 20000,
    BG_RV_PILAR_TO_FIRE_TIMER                    =  5000,
    BG_RV_FIRST_TIMER                            = 20133,
    BG_RV_WORLD_STATE_A                          = 0xe10,
    BG_RV_WORLD_STATE_H                          = 0xe11,
    BG_RV_WORLD_STATE                            = 0xe1a,
};

class BattlegroundRVScore : public BattlegroundScore
{
    public:
        BattlegroundRVScore() {};
        virtual ~BattlegroundRVScore() {};
};

class BattlegroundRV : public BattlegroundArena
{
    friend class BattlegroundMgr;

    public:
        BattlegroundRV();
        ~BattlegroundRV();
        void Update(uint32 diff);

        /* inherited from Battleground class */
        virtual void AddPlayer(Player *plr);
        virtual void StartingEventCloseDoors();
        virtual void StartingEventOpenDoors();
        virtual void Reset();
        virtual void FillInitialWorldStates(WorldPacket &d);

        /* inherited from BattlegroundArena class */
        virtual bool GetUnderMapLimitZPosition(float &z);

        void RemovePlayer(Player *plr, uint64 guid);
        void HandleAreaTrigger(Player *Source, uint32 Trigger);
        bool SetupBattleground();
        void HandleKillPlayer(Player* player, Player *killer);
        bool GetUnderMapReturnPosition(Player* plr, Position& pos);

    private:
        uint32 Timer;
        uint32 State;

    protected:
        uint32 getTimer() { return Timer; };
        void setTimer(uint32 timer) { Timer = timer; };

        uint32 getState() { return State; };
        void setState(uint32 state) { State = state; };

        void TogglePillarCollision(bool apply);
};
#endif
