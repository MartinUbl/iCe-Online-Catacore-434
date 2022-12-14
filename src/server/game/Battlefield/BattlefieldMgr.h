/*
 * Copyright (C) 2010-2011 Project SkyFire <http://www.projectskyfire.org/>
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BATTLEFIELD_MGR_H_
#define BATTLEFIELD_MGR_H_

#include "Battlefield.h"
#include "ace/Singleton.h"

enum BattlefieldWorldStates
{
    WS_WG_INPROGRESS        = 3781,
    WS_WG_TIMER             = 4354,
    WS_TB_INPROGRESS        = 5333,
    WS_TB_TIMER             = 5332,
};

class Player;
class GameObject;
class Creature;
class ZoneScript;
struct GossipMenuItems;

// class to handle player enter / leave / areatrigger / GO use events
class BattlefieldMgr
{
  public:
    // ctor
    BattlefieldMgr();
    // dtor
    ~BattlefieldMgr();

    // create battlefield events
    void InitBattlefield();
    // called when a player enters an battlefield area
    void HandlePlayerEnterZone(Player* player, uint32 areaflag);
    // called when player leaves an battlefield area
    void HandlePlayerLeaveZone(Player* player, uint32 areaflag);
    // called when player resurrects
    void HandlePlayerResurrects(Player* player, uint32 areaflag);
    // called when player logout
    void HandlePlayerLogout(Player* player);
    // return assigned battlefield
    Battlefield *GetBattlefieldToZoneId(uint32 zoneid);
    Battlefield *GetBattlefieldByBattleId(uint32 battleid);

    ZoneScript *GetZoneScript(uint32 zoneId);

    void AddZone(uint32 zoneid, Battlefield * handle);

    void Update(uint32 diff);

    void UpdateBattlefieldState();
    void UpdateBattlefieldStateFor(Player* plr);

    void HandleGossipOption(Player* player, uint64 guid, uint32 gossipid);

    bool CanTalkTo(Player* player, Creature * creature, GossipMenuItems gso);

    void HandleDropFlag(Player* player, uint32 spellId);

    typedef std::vector < Battlefield * >BattlefieldSet;
    typedef std::map < uint32 /* zoneid */ , Battlefield * >BattlefieldMap;
  private:
      void BuildWorldStateUpdate(WorldPacket &pkt, uint32 state, uint32 value);
    // contains all initiated battlefield events
    // used when initing / cleaning up
      BattlefieldSet m_BattlefieldSet;
    // maps the zone ids to an battlefield event
    // used in player event handling
    BattlefieldMap m_BattlefieldMap;
    // update interval
    uint32 m_UpdateTimer;
};

#define sBattlefieldMgr (*ACE_Singleton<BattlefieldMgr, ACE_Null_Mutex>::instance())

#endif
