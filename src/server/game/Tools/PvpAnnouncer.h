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

#ifndef GCORE_PVPANN_H
#define GCORE_PVPANN_H

#include <ace/Singleton.h>
#include <set>

enum PvPAnnounceType
{
    PVPANNOUNCE_NONE = 0,
    PVPANNOUNCE_BATTLEGROUND_STARTED = 1,
    PVPANNOUNCE_ARENA_STATUS_2v2 = 2,
    PVPANNOUNCE_ARENA_STATUS_3v3 = 3,
    PVPANNOUNCE_ARENA_STATUS_5v5 = 4,
    PVPANNOUNCE_BATTLEGROUND_RATED_TEAM_JOINED = 5,
    PVPANNOUNCE_BATTLEFIELD_IN_15MINS = 6,
    PVPANNOUNCE_BATTLEFIELD_IN_30MINS = 7,
    PVPANNOUNCE_BATTLEFIELD_STARTED = 8
};

enum DefaultPvPAnnouncerTimes
{
    PVPANNOUNCE_TIME_ARENA_STATUS       = 5 * MINUTE * IN_MILLISECONDS
};

class PvPAnnouncer
{
    public:
        PvPAnnouncer();

        void AddPlayer(Player* pl);
        void RemovePlayer(Player* pl, bool onLogout = false);
        bool IsPlayerListed(Player* pl);

        void Update(uint32 diff);

        void Announce(PvPAnnounceType type, BattlegroundTypeId bgTypeId, uint8 arenaType = 0, uint32 param1 = 0, uint32 param2 = 0);

    private:
        std::set<Player*> m_playerSet;

        uint32 m_arenaStatusAnnounceTimer;
};

#define sPvPAnnouncer ACE_Singleton<PvPAnnouncer, ACE_Null_Mutex>::instance()

#endif
