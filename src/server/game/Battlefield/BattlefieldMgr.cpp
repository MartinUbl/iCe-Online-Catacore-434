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

#include "gamePCH.h"
#include "BattlefieldMgr.h"
#include "Zones/BattlefieldWG.h"
#include "ObjectMgr.h"
#include "Player.h"

BattlefieldMgr::BattlefieldMgr()
{
    m_UpdateTimer = 0;
    //sLog->outDebug(LOG_FILTER_BATTLEFIELD, "Instantiating BattlefieldMgr");
}

BattlefieldMgr::~BattlefieldMgr()
{
    //sLog->outDebug(LOG_FILTER_BATTLEFIELD, "Deleting BattlefieldMgr");
    for (BattlefieldSet::iterator itr = m_BattlefieldSet.begin(); itr != m_BattlefieldSet.end(); ++itr)
        delete *itr;
}

void BattlefieldMgr::InitBattlefield()
{
    Battlefield* pBf = new BattlefieldWG;
    // respawn, init variables
    if (!pBf->SetupBattlefield())
    {
        sLog->outString();
        sLog->outString("Battlefield : Wintergrasp init failed.");
        delete pBf;
    }
    else
    {
        m_BattlefieldSet.push_back(pBf);
        sLog->outString();
        sLog->outString("Battlefield : Wintergrasp successfully initiated.");
    }

    /* For Cataclysm: Tol Barad
       pBf = new BattlefieldTB;
       // respawn, init variables
       if(!pBf->SetupBattlefield())
       {
       sLog->outDebug(LOG_FILTER_BATTLEFIELD, "Battlefield : Tol Barad init failed.");
       delete pBf;
       }
       else
       {
       m_BattlefieldSet.push_back(pBf);
       sLog->outDebug(LOG_FILTER_BATTLEFIELD, "Battlefield : Tol Barad successfully initiated.");
       } */
}

void BattlefieldMgr::AddZone(uint32 zoneid, Battlefield *handle)
{
    m_BattlefieldMap[zoneid] = handle;
}

void BattlefieldMgr::HandlePlayerEnterZone(Player* player, uint32 zoneid)
{
    BattlefieldMap::iterator itr = m_BattlefieldMap.find(zoneid);
    if (itr == m_BattlefieldMap.end())
        return;

    if (itr->second->HasPlayer(player))
        return;
    if (itr->second->GetEnable() == false)
        return;
    itr->second->HandlePlayerEnterZone(player, zoneid);
}

void BattlefieldMgr::HandlePlayerLeaveZone(Player* player, uint32 zoneid)
{
    BattlefieldMap::iterator itr = m_BattlefieldMap.find(zoneid);
    if (itr == m_BattlefieldMap.end())
        return;

    // teleport: remove once in removefromworld, once in updatezone
    if (!itr->second->HasPlayer(player))
        return;
    itr->second->HandlePlayerLeaveZone(player, zoneid);
}

void BattlefieldMgr::HandlePlayerLogout(Player* player)
{
    for (BattlefieldSet::iterator itr = m_BattlefieldSet.begin(); itr != m_BattlefieldSet.end(); ++itr)
    {
        (*itr)->AskToLeaveQueue(player);
    }
}

Battlefield *BattlefieldMgr::GetBattlefieldToZoneId(uint32 zoneid)
{
    BattlefieldMap::iterator itr = m_BattlefieldMap.find(zoneid);
    if (itr == m_BattlefieldMap.end())
    {
        // no handle for this zone, return
        return NULL;
    }
    if (itr->second->GetEnable() == false)
        return NULL;
    return itr->second;
}

Battlefield *BattlefieldMgr::GetBattlefieldByBattleId(uint32 battleid)
{
    for (BattlefieldSet::iterator itr = m_BattlefieldSet.begin(); itr != m_BattlefieldSet.end(); ++itr)
    {
        if ((*itr)->GetBattleId() == battleid)
            return (*itr);
    }
    return NULL;
}

void BattlefieldMgr::Update(uint32 diff)
{
    m_UpdateTimer += diff;
    if (m_UpdateTimer > BATTLEFIELD_OBJECTIVE_UPDATE_INTERVAL)
    {
        for (BattlefieldSet::iterator itr = m_BattlefieldSet.begin(); itr != m_BattlefieldSet.end(); ++itr)
            if ((*itr)->GetEnable())
                (*itr)->Update(m_UpdateTimer);
        m_UpdateTimer = 0;
    }
}

void BattlefieldMgr::UpdateBattlefieldState()
{
    Battlefield* bf;

    bf = GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG);
    WorldPacket wgprogress;
    BuildWorldStateUpdate(wgprogress, WS_WG_INPROGRESS, bf->IsWarTime() ? 1 : 0);

    WorldPacket wgtimer;
    BuildWorldStateUpdate(wgtimer, WS_WG_TIMER, bf->IsWarTime() ? 0 : time(nullptr) + bf->GetTimer() / 1000);

    sWorld->SendGlobalMessage(&wgprogress);
    sWorld->SendGlobalMessage(&wgtimer);
}

void BattlefieldMgr::UpdateBattlefieldStateFor(Player* plr)
{
    Battlefield* bf;

    bf = GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG);
    WorldPacket wgprogress;
    BuildWorldStateUpdate(wgprogress, WS_WG_INPROGRESS, bf->IsWarTime() ? 1 : 0);

    WorldPacket wgtimer;
    BuildWorldStateUpdate(wgtimer, WS_WG_TIMER, bf->IsWarTime() ? 0 : time(nullptr) + bf->GetTimer() / 1000);

    plr->GetSession()->SendPacket(&wgprogress);
    plr->GetSession()->SendPacket(&wgtimer);
}

void BattlefieldMgr::BuildWorldStateUpdate(WorldPacket &pkt, uint32 state, uint32 value)
{
    pkt.SetOpcode(SMSG_UPDATE_WORLD_STATE);
    pkt << state;
    pkt << value;
    pkt << uint8(0);
}

ZoneScript *BattlefieldMgr::GetZoneScript(uint32 zoneId)
{
    BattlefieldMap::iterator itr = m_BattlefieldMap.find(zoneId);
    if (itr != m_BattlefieldMap.end())
        return itr->second;
    else
        return NULL;
}
