/*
* Copyright (C) 2006-2015, iCe Online <http://ice-wow.eu>
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
#include "Player.h"
#include "PvpAnnouncer.h"
#include "Chat.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"

PvPAnnouncer::PvPAnnouncer()
{
    m_arenaStatusAnnounceTimer = PVPANNOUNCE_TIME_ARENA_STATUS;
}

void PvPAnnouncer::AddPlayer(Player* pl)
{
    if (!IsPlayerListed(pl))
    {
        m_playerSet.insert(pl);
        pl->SetPvPAnnouncerFlag(true);

        ChatHandler(pl).SendSysMessage("You have been subscribed to PvP channel. To leave, use |cff00ff00.pvpmessage off|r");
    }
}

void PvPAnnouncer::RemovePlayer(Player* pl, bool onLogout)
{
    if (IsPlayerListed(pl))
    {
        m_playerSet.erase(pl);
        pl->SetPvPAnnouncerFlag(false);

        if (!onLogout)
            ChatHandler(pl).SendSysMessage("You have left PvP channel. To join again, use |cff00ff00.pvpmessage on|r");
    }
}

bool PvPAnnouncer::IsPlayerListed(Player* pl)
{
    return (m_playerSet.find(pl) != m_playerSet.end());
}

void PvPAnnouncer::Announce(PvPAnnounceType type, BattlegroundTypeId bgTypeId, uint8 arenaType, uint32 param1, uint32 param2)
{
    Battleground *bg = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId);

    uint8 levelMin = 0, levelMax = 0;

    std::string toannounce = "";

    switch (type)
    {
        case PVPANNOUNCE_NONE:
        default:
            break;
        case PVPANNOUNCE_BATTLEGROUND_STARTED:
            if (bg)
            {
                levelMin = param1;
                levelMax = param2;
                toannounce = std::string("Battleground ") + std::string(bg->GetName()) + " has just started!";
            }
            break;
        case PVPANNOUNCE_ARENA_STATUS_2v2:
        case PVPANNOUNCE_ARENA_STATUS_3v3:
        case PVPANNOUNCE_ARENA_STATUS_5v5:
            levelMin = DEFAULT_MAX_LEVEL;
            levelMax = DEFAULT_MAX_LEVEL;
            toannounce = std::string("|c0000ff00") + std::to_string(arenaType) + std::string("v") + std::to_string(arenaType)
                + std::string("|r teams in queue: ") + std::to_string(param1) + ", in arena: " + std::to_string(param2);
            break;
        case PVPANNOUNCE_BATTLEGROUND_RATED_TEAM_JOINED:
            levelMin = DEFAULT_MAX_LEVEL;
            levelMax = DEFAULT_MAX_LEVEL;
            toannounce = std::string("Team with average rating of ") + std::to_string(param1) + " joined rated BG queue";
            break;
        case PVPANNOUNCE_BATTLEFIELD_IN_30MINS:
            levelMin = GetBattlefieldMinLevel((BattlefieldIDs)param1);
            levelMax = DEFAULT_MAX_LEVEL;
            toannounce = std::string("The battle for ") + std::string(GetBattlefieldName((BattlefieldIDs)param1)) + std::string(" will begin in 30 minutes!");
            break;
        case PVPANNOUNCE_BATTLEFIELD_IN_15MINS:
            levelMin = GetBattlefieldMinLevel((BattlefieldIDs)param1);
            levelMax = DEFAULT_MAX_LEVEL;
            toannounce = std::string("The battle for ") + std::string(GetBattlefieldName((BattlefieldIDs)param1)) + std::string(" will begin in 15 minutes!");
            break;
        case PVPANNOUNCE_BATTLEFIELD_STARTED:
            levelMin = GetBattlefieldMinLevel((BattlefieldIDs)param1);
            levelMax = DEFAULT_MAX_LEVEL;
            toannounce = std::string("The battle for ") + std::string(GetBattlefieldName((BattlefieldIDs)param1)) + std::string(" has just begun!");
            break;
    }

    if (toannounce.length() == 0)
        return;

    toannounce = std::string("|c00ff0000(PvP)|r ") + toannounce;

    Player* pl;

    for (std::set<Player*>::iterator itr = m_playerSet.begin(); itr != m_playerSet.end(); ++itr)
    {
        pl = *itr;
        if (pl && pl->IsInWorld())
        {
            if (levelMax == 0 || (pl->getLevel() >= levelMin && pl->getLevel() <= levelMax))
                ChatHandler(pl).SendSysMessage(toannounce.c_str());
        }
    }
}

void PvPAnnouncer::Update(uint32 diff)
{
    if (m_arenaStatusAnnounceTimer <= diff)
    {
        sBattlegroundMgr->UpdateArenaQueueInfo();

        for (uint8 arenaSlot = ARENA_SLOT_2v2; arenaSlot < ARENA_SLOT_MAX; arenaSlot++)
        {
            uint8 qcount = sBattlegroundMgr->GetQueuedArenaTeamCount((ArenaSlot)arenaSlot);
            uint8 acount = sBattlegroundMgr->GetArenaTeamInArenaCount((ArenaSlot)arenaSlot);

            if (qcount != 0 || acount != 0)
            {
                Announce(PVPANNOUNCE_ARENA_STATUS_2v2, BATTLEGROUND_TYPE_NONE, ArenaTypeForSlot(arenaSlot),
                    qcount, acount);
            }
        }

        m_arenaStatusAnnounceTimer = PVPANNOUNCE_TIME_ARENA_STATUS;
    }
    else
        m_arenaStatusAnnounceTimer -= diff;
}
