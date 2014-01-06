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
#include "Common.h"
#include "ObjectMgr.h"
#include "World.h"
#include "WorldPacket.h"


#include "ArenaTeam.h"
#include "BattlegroundMgr.h"
#include "BattlegroundAV.h"
#include "BattlegroundAB.h"
#include "BattlegroundEY.h"
#include "BattlegroundWS.h"
#include "BattlegroundNA.h"
#include "BattlegroundBE.h"
#include "BattlegroundAA.h"
#include "BattlegroundRL.h"
#include "BattlegroundSA.h"
#include "BattlegroundDS.h"
#include "BattlegroundRV.h"
#include "BattlegroundIC.h"
#include "BattlegroundBG.h"
#include "BattlegroundTP.h"
#include "BattlegroundRB.h"
#include "BattlegroundTP.h"
#include "Chat.h"
#include "Map.h"
#include "MapInstanced.h"
#include "MapManager.h"
#include "Player.h"
#include "GameEventMgr.h"
#include "SharedDefines.h"
#include "Formulas.h"
#include "DisableMgr.h"

/*********************************************************/
/***            BATTLEGROUND MANAGER                   ***/
/*********************************************************/

BattlegroundMgr::BattlegroundMgr() : m_AutoDistributionTimeChecker(0), m_ArenaTesting(false)
{
    for(int type = 0; type < MAX_BATTLEGROUND_QUEUE_TYPES; type++)
        for(uint8 tw = 0; tw < BATTLEGROUND_TWINK_TYPES; tw++)
            m_BattlegroundQueues[type][tw].setTwink(tw);

    for (uint32 i = BATTLEGROUND_TYPE_NONE; i < MAX_BATTLEGROUND_TYPE_ID; i++)
        m_Battlegrounds[i].clear();
    m_NextRatingDiscardUpdate = sWorld->getIntConfig(CONFIG_ARENA_RATING_DISCARD_TIMER);
    m_Testing=false;

    m_ratedBgWeek = RATED_BATTLEGROUND_WEEK_NONE;
    m_ratedBgNextWeek = 0;
    m_ratedBgWeekCheckTimer = 0;

    m_ArenaGenerator.Seed(urand(0, 0xffffffff));
    m_RandomBGGenerator.Seed(urand(0, 0xffffffff));
}

BattlegroundMgr::~BattlegroundMgr()
{
    DeleteAllBattlegrounds();
}

void BattlegroundMgr::DeleteAllBattlegrounds()
{
    for (uint32 i = BATTLEGROUND_TYPE_NONE; i < MAX_BATTLEGROUND_TYPE_ID; ++i)
    {
        for (BattlegroundSet::iterator itr = m_Battlegrounds[i].begin(); itr != m_Battlegrounds[i].end();)
        {
            Battleground * bg = itr->second;
            m_Battlegrounds[i].erase(itr++);
            if (!m_ClientBattlegroundIds[i][bg->GetBracketId()].empty())
                m_ClientBattlegroundIds[i][bg->GetBracketId()].erase(bg->GetClientInstanceID());
            delete bg;
        }
    }

    // destroy template battlegrounds that listed only in queues (other already terminated)
    for (uint32 bgTypeId = 0; bgTypeId < MAX_BATTLEGROUND_TYPE_ID; ++bgTypeId)
    {
        // ~Battleground call unregistring BG from queue
        for(uint8 tw = 0; tw < BATTLEGROUND_TWINK_TYPES; tw++)
            while (!BGFreeSlotQueue[bgTypeId][tw].empty())
                delete BGFreeSlotQueue[bgTypeId][tw].front();
    }
}

// used to update running battlegrounds, and delete finished ones
void BattlegroundMgr::Update(uint32 diff)
{
    BattlegroundSet::iterator itr, next;
    for (uint32 i = BATTLEGROUND_TYPE_NONE; i < MAX_BATTLEGROUND_TYPE_ID; ++i)
    {
        itr = m_Battlegrounds[i].begin();
        // skip updating battleground template
        if (itr != m_Battlegrounds[i].end())
            ++itr;
        for (; itr != m_Battlegrounds[i].end(); itr = next)
        {
            next = itr;
            ++next;
            itr->second->Update(diff);
            // use the SetDeleteThis variable
            // direct deletion caused crashes
            if (itr->second->m_SetDeleteThis)
            {
                Battleground * bg = itr->second;
                m_Battlegrounds[i].erase(itr);
                if (!m_ClientBattlegroundIds[i][bg->GetBracketId()].empty())
                    m_ClientBattlegroundIds[i][bg->GetBracketId()].erase(bg->GetClientInstanceID());
                delete bg;
            }
        }
    }

    // update scheduled queues
    if (!m_QueueUpdateScheduler.empty())
    {
        std::vector<uint64> scheduled;
        {
            //copy vector and clear the other
            scheduled = std::vector<uint64>(m_QueueUpdateScheduler);
            m_QueueUpdateScheduler.clear();
            //release lock
        }

        for (uint8 i = 0; i < scheduled.size(); i++)
        {
            uint32 arenaMMRating = (scheduled[i] >> 32) & 0x00FFFFFF;
            uint8 arenaType = (scheduled[i] >> 24) & 255;
            BattlegroundQueueTypeId bgQueueTypeId = BattlegroundQueueTypeId(scheduled[i] >> 16 & 255);
            BattlegroundTypeId bgTypeId = BattlegroundTypeId((scheduled[i] >> 8) & 255);
            BattlegroundBracketId bracket_id = BattlegroundBracketId(scheduled[i] & 255);
            bool isRated = (arenaMMRating > 0) || (bgTypeId == BATTLEGROUND_RA_BG_10) || (bgTypeId == BATTLEGROUND_RA_BG_15);
            uint8 twink = (scheduled[i] >> 56) & 0xFF;
            m_BattlegroundQueues[bgQueueTypeId][twink].Update(bgTypeId, bracket_id, arenaType, isRated, arenaMMRating);
        }
    }

    // if rating difference counts, maybe force-update queues
    if (sWorld->getIntConfig(CONFIG_ARENA_MAX_RATING_DIFFERENCE) && sWorld->getIntConfig(CONFIG_ARENA_RATING_DISCARD_TIMER))
    {
        // it's time to force update
        if (m_NextRatingDiscardUpdate < diff)
        {
            // forced update for rated arenas (scan all, but skipped non rated)
            sLog->outDebug("BattlegroundMgr: UPDATING ARENA QUEUES");
            for (int qtype = BATTLEGROUND_QUEUE_2v2; qtype <= BATTLEGROUND_QUEUE_5v5; ++qtype)
                for (int bracket = BG_BRACKET_ID_FIRST; bracket < MAX_BATTLEGROUND_BRACKETS; ++bracket)
                    for(int tw = 0; tw < BATTLEGROUND_TWINK_TYPES; tw++)
                        m_BattlegroundQueues[qtype][tw].Update(
                            BATTLEGROUND_AA, BattlegroundBracketId(bracket),
                            BattlegroundMgr::BGArenaType(BattlegroundQueueTypeId(qtype)), true, 0);

            m_NextRatingDiscardUpdate = sWorld->getIntConfig(CONFIG_ARENA_RATING_DISCARD_TIMER);
        }
        else
            m_NextRatingDiscardUpdate -= diff;
    }
    if (sWorld->getBoolConfig(CONFIG_ARENA_AUTO_DISTRIBUTE_POINTS))
    {
        if (m_AutoDistributionTimeChecker < diff)
        {
            if (time(NULL) > m_NextAutoDistributionTime)
            {
                DistributeArenaCurrency();
                m_NextAutoDistributionTime = m_NextAutoDistributionTime + BATTLEGROUND_ARENA_POINT_DISTRIBUTION_DAY * sWorld->getIntConfig(CONFIG_ARENA_AUTO_DISTRIBUTE_INTERVAL_DAYS);
                sWorld->setWorldState(WS_ARENA_DISTRIBUTION_TIME, uint64(m_NextAutoDistributionTime));
            }
            m_AutoDistributionTimeChecker = 600000; // check 10 minutes
        }
        else
            m_AutoDistributionTimeChecker -= diff;
    }

    if (m_ratedBgNextWeek)
    {
        if (m_ratedBgWeekCheckTimer < diff)
        {
            if (m_ratedBgNextWeek <= time(NULL))
                UpdateRatedBattlegroundWeek();

            m_ratedBgWeekCheckTimer = 600000; // 10 minutes
        }
        else
            m_ratedBgWeekCheckTimer -= diff;
    }
}

void BattlegroundMgr::BuildBattlegroundStatusPacket(WorldPacket *data, Battleground *bg, Player *player, uint8 QueueSlot, uint8 StatusID, uint32 Time1, uint32 Time2, uint8 arenatype, uint8 uiFrame)
{
    ObjectGuid playerGuid = player->GetGUID();
    ObjectGuid bgGuid;

    if (!bg)
        StatusID = STATUS_NONE;
    else
        bgGuid = bg->GetGUID();

    switch (StatusID)
    {
        case STATUS_NONE:
        {
            // STATUS1 resets clients' BG Info
            data->Initialize(SMSG_BATTLEFIELD_STATUS1);

            data->WriteBit(playerGuid[0]);
            data->WriteBit(playerGuid[4]);
            data->WriteBit(playerGuid[7]);
            data->WriteBit(playerGuid[1]);
            data->WriteBit(playerGuid[6]);
            data->WriteBit(playerGuid[3]);
            data->WriteBit(playerGuid[5]);
            data->WriteBit(playerGuid[2]);

            data->WriteByteSeq(playerGuid[5]);
            data->WriteByteSeq(playerGuid[6]);
            data->WriteByteSeq(playerGuid[7]);
            data->WriteByteSeq(playerGuid[2]);
            *data << uint32((bg && bg->isArena()) ? arenatype : 1);  // 1 for battlegrounds, team size for arenas
            data->WriteByteSeq(playerGuid[3]);
            data->WriteByteSeq(playerGuid[1]);
            *data << uint32(QueueSlot);                 // Queue slot
            *data << uint32(Time1);                     // Join Time
            data->WriteByteSeq(playerGuid[0]);
            data->WriteByteSeq(playerGuid[4]);
            break;
        }
        case STATUS_WAIT_QUEUE:
        {
            // The client will set STATUS_WAIT_QUEUE at BGInfo once it receives this packet
            data->Initialize(SMSG_GROUP_JOINED_BATTLEGROUND);

            data->WriteBit(playerGuid[3]);
            data->WriteBit(playerGuid[0]);
            data->WriteBit(bgGuid[3]);
            data->WriteBit(playerGuid[2]);
            data->WriteBit(1);                          // Eligible In Queue
            data->WriteBit(bg->isRated());
            data->WriteBit(bgGuid[2]);
            data->WriteBit(playerGuid[1]);
            data->WriteBit(bgGuid[0]);
            data->WriteBit(bgGuid[6]);
            data->WriteBit(bgGuid[4]);
            data->WriteBit(playerGuid[6]);
            data->WriteBit(playerGuid[7]);
            data->WriteBit(bgGuid[7]);
            data->WriteBit(bgGuid[5]);
            data->WriteBit(playerGuid[4]);
            data->WriteBit(playerGuid[5]);
            data->WriteBit(bg->isRated());              // Is Rated
            data->WriteBit(0);                          // Waiting On Other Activity
            data->WriteBit(bgGuid[1]);

            data->FlushBits();

            data->WriteByteSeq(playerGuid[0]);
            *data << uint32(bg->isArena() ? arenatype : 1); // Player count, 1 for bgs, 2-3-5 for arena (2v2, 3v3, 5v5)
            data->WriteByteSeq(bgGuid[5]);
            data->WriteByteSeq(playerGuid[3]);
            *data << uint32(Time1);                     // Estimated Wait Time
            data->WriteByteSeq(bgGuid[7]);
            data->WriteByteSeq(bgGuid[1]);
            data->WriteByteSeq(bgGuid[2]);
            *data << uint8(0);                          // unk
            data->WriteByteSeq(bgGuid[4]);
            data->WriteByteSeq(playerGuid[2]);
            *data << uint8(bg->isArena() && bg->isRated() ? arenatype : 0);                // non-zero on rated arenas (team size)
            data->WriteByteSeq(bgGuid[6]);
            data->WriteByteSeq(playerGuid[7]);
            data->WriteByteSeq(bgGuid[3]);
            data->WriteByteSeq(playerGuid[6]);
            data->WriteByteSeq(bgGuid[0]);
            *data << uint32(Time2);                     // Join Time
            *data << uint32(QueueSlot);                 // Queue slot
            *data << uint8(bg->GetMinLevel());          // Min Level
            *data << uint32(getMSTimeDiff(Time2, getMSTime())); // Time since joined
            data->WriteByteSeq(playerGuid[1]);
            data->WriteByteSeq(playerGuid[5]);
            *data << uint32(bg->GetClientInstanceID()); // Client Instance ID
            data->WriteByteSeq(playerGuid[4]);
            break;
        }
        case STATUS_WAIT_JOIN:
        {
            // The client will set STATUS_WAIT_JOIN at BGInfo once it receives this packet
            data->Initialize(SMSG_BATTLEFIELD_STATUS3);

            *data << uint32(bg->GetClientInstanceID()); // Client Instance ID
            *data << uint32(Time1);                     // Time until closed
            *data << uint8(0);                          // unk
            *data << uint32(QueueSlot);                 // Queue slot
            *data << uint32(Time2);                     // Join Time
            *data << uint8(bg->GetMinLevel());          // Min Level
            *data << uint32(bg->isArena() ? arenatype : 1); // Player count, 1 for bgs, 2-3-5 for arena (2v2, 3v3, 5v5)
            *data << uint32(bg->GetMapId());            // Map Id
            *data << uint8(bg->isArena() && bg->isRated() ? arenatype : 0);                // non-zero on rated arenas (team size)

            data->WriteBit(playerGuid[5]);
            data->WriteBit(playerGuid[2]);
            data->WriteBit(playerGuid[1]);
            data->WriteBit(bgGuid[2]);
            data->WriteBit(playerGuid[4]);
            data->WriteBit(bgGuid[6]);
            data->WriteBit(bgGuid[3]);
            data->WriteBit(bg->isRated());              // Is Rated
            data->WriteBit(playerGuid[7]);
            data->WriteBit(playerGuid[3]);
            data->WriteBit(bgGuid[7]);
            data->WriteBit(bgGuid[0]);
            data->WriteBit(bgGuid[4]);
            data->WriteBit(playerGuid[6]);
            data->WriteBit(bgGuid[1]);
            data->WriteBit(bgGuid[5]);
            data->WriteBit(playerGuid[0]);

            data->FlushBits();

            data->WriteByteSeq(bgGuid[6]);
            data->WriteByteSeq(bgGuid[5]);
            data->WriteByteSeq(bgGuid[7]);
            data->WriteByteSeq(bgGuid[2]);
            data->WriteByteSeq(playerGuid[0]);
            data->WriteByteSeq(playerGuid[7]);
            data->WriteByteSeq(bgGuid[4]);
            data->WriteByteSeq(playerGuid[1]);
            data->WriteByteSeq(bgGuid[0]);
            data->WriteByteSeq(playerGuid[4]);
            data->WriteByteSeq(bgGuid[1]);
            data->WriteByteSeq(playerGuid[5]);
            data->WriteByteSeq(bgGuid[3]);
            data->WriteByteSeq(playerGuid[6]);
            data->WriteByteSeq(playerGuid[2]);
            data->WriteByteSeq(playerGuid[3]);
            break;
        }
        case STATUS_IN_PROGRESS:
        {
            data->Initialize(SMSG_BATTLEFIELD_STATUS2);

            data->WriteBit(playerGuid[2]);
            data->WriteBit(playerGuid[7]);
            data->WriteBit(bgGuid[7]);
            data->WriteBit(bgGuid[1]);
            data->WriteBit(playerGuid[5]);
            if (bg->isArena())
                data->WriteBit(bg->GetStatus() == STATUS_IN_PROGRESS);      // show unit frames
            else
                data->WriteBit(player->GetTeam() == ALLIANCE);
            data->WriteBit(bgGuid[0]);
            data->WriteBit(playerGuid[1]);
            data->WriteBit(bgGuid[3]);
            data->WriteBit(playerGuid[6]);
            data->WriteBit(bgGuid[5]);
            data->WriteBit(bg->isRated());              // Is Rated
            data->WriteBit(playerGuid[4]);
            data->WriteBit(bgGuid[6]);
            data->WriteBit(bgGuid[4]);
            data->WriteBit(bgGuid[2]);
            data->WriteBit(playerGuid[3]);
            data->WriteBit(playerGuid[0]);

            data->FlushBits();

            data->WriteByteSeq(bgGuid[4]);
            data->WriteByteSeq(bgGuid[5]);
            data->WriteByteSeq(playerGuid[5]);
            data->WriteByteSeq(bgGuid[1]);
            data->WriteByteSeq(bgGuid[6]);
            data->WriteByteSeq(bgGuid[3]);
            data->WriteByteSeq(bgGuid[7]);
            data->WriteByteSeq(playerGuid[6]);

            *data << uint32(Time1);                     // Join Time
            *data << uint8(0);                          // unk

            data->WriteByteSeq(playerGuid[4]);
            data->WriteByteSeq(playerGuid[1]);

            *data << uint32(QueueSlot);                 // Queue slot
            *data << uint8(bg->isArena() && bg->isRated() ? arenatype : 0);        // non-zero on rated arenas (team size)
            *data << uint32(bg->isArena() ? arenatype : 1); // Player count, 1 for bgs, 2-3-5 for arena (2v2, 3v3, 5v5)
            *data << uint32(bg->GetMapId());            // Map Id
            *data << uint8(bg->GetMinLevel());          // Min Level
            *data << uint32(Time2);                     // Elapsed Time

            data->WriteByteSeq(playerGuid[2]);
            *data << uint32(getMSTimeDiff(bg->GetEndTime(), Time2));    // Remaining Time

            data->WriteByteSeq(playerGuid[0]);
            data->WriteByteSeq(playerGuid[3]);
            data->WriteByteSeq(playerGuid[2]);

            *data << uint32(bg->GetClientInstanceID()); // Client Instance ID or faction ?

            data->WriteByteSeq(bgGuid[0]);
            data->WriteByteSeq(playerGuid[7]);
            break;
        }
        case STATUS_WAIT_LEAVE:
            break;
    }
}

void BattlegroundMgr::BuildPvpLogDataPacket(WorldPacket *data, Battleground *bg, Player *receiver)
{
    uint8 isRated = (bg->isRated() ? 1 : 0);               // type (normal=0/rated=1) -- ATM arena or bg, RBG NYI
    uint8 isArena = (bg->isArena() ? 1 : 0);               // Arena names

    data->Initialize(MSG_PVP_LOG_DATA, (1+1+4+40*bg->GetPlayerScoresSize()));
    data->WriteBit(isArena);
    data->WriteBit(isRated);

    uint8 playerTeam = 0;
    uint8 oppositeTeam = 1;

    if (isArena)
    {
        for (int8 i = 0; i < 2; ++i)
        {
            uint32 at_id = bg->m_ArenaTeamIds[i];
            if (ArenaTeam* at = sObjectMgr->GetArenaTeamById(at_id))
                data->WriteBits(at->GetName().length(), 8);
            else
                data->WriteBits(0, 8);
        }
    }

    size_t count_pos = data->bitwpos();

    data->WriteBits(0, 21);     // Placeholder

    int32 count = 0;
    ByteBuffer buff;

    Battleground::BattlegroundScoreMap::const_iterator itr2 = bg->GetPlayerScoresBegin();
    for (Battleground::BattlegroundScoreMap::const_iterator itr = itr2; itr != bg->GetPlayerScoresEnd();)
    {
        itr2 = itr++;
        if (!bg->IsPlayerInBattleground(itr2->first))
        {
            sLog->outError("Player " UI64FMTD " has scoreboard entry for battleground %u but is not in battleground!", itr->first, bg->GetTypeID(true));
            continue;
        }

        ObjectGuid guid = itr2->first;
        Player* player = ObjectAccessor::FindPlayer(itr2->first);
        if (!player)
            continue;

        // for arenas which didn't start yet, do not send opponents data
        if (isArena && bg->GetStatus() <= STATUS_WAIT_JOIN)
        {
            if (bg->GetPlayerTeam(player->GetGUID()) != bg->GetPlayerTeam(receiver->GetGUID()))
                continue;
        }

        data->WriteBit(0); // Unk 1
        data->WriteBit(0); // Unk 2
        data->WriteBit(guid[2]);
        data->WriteBit(!isArena); // Unk 3 -- Prolly if (bg)
        data->WriteBit(0); // Unk 4
        data->WriteBit(isRated);    // send personal rating change
        data->WriteBit(0); // Unk 6
        data->WriteBit(guid[3]);
        data->WriteBit(guid[0]);
        data->WriteBit(guid[5]);
        data->WriteBit(guid[1]);
        data->WriteBit(guid[6]);
        data->WriteBit(player->GetBGTeam() == ALLIANCE);
        data->WriteBit(guid[7]);

        buff << uint32(itr2->second->HealingDone);             // healing done
        buff << uint32(itr2->second->DamageDone);              // damage done

        if (!isArena) // Unk 3 prolly is (bg)
        {
            buff << uint32(itr2->second->BonusHonor);
            buff << uint32(itr2->second->Deaths);
            buff << uint32(itr2->second->HonorableKills);
        }

        buff.WriteByteSeq(guid[4]);
        buff << uint32(itr2->second->KillingBlows);

        if (isRated)
        {
            uint8 team = bg->GetPlayerTeam(player->GetGUID()) == ALLIANCE ? TEAM_ALLIANCE : TEAM_HORDE;
            buff << int32(itr2->second->PersonalRatingChange);      // personal rating change

            if (receiver == player)
            {
                playerTeam = team;
                oppositeTeam = (playerTeam == TEAM_ALLIANCE) ? TEAM_HORDE : TEAM_ALLIANCE;
            }
        }

        buff.WriteByteSeq(guid[5]);

        // if (unk 6) << uint32() unk
        // if (unk 2) << uint32() unk

        buff.WriteByteSeq(guid[1]);
        buff.WriteByteSeq(guid[6]);

        buff << int32(player->GetTalentBranchSpec(player->GetActiveSpec()));

        switch (bg->GetTypeID(true))                             // Custom values
        {
        case BATTLEGROUND_RB:
            switch (bg->GetMapId())
            {
            case 489:
                data->WriteBits(0x00000002, 24);
                buff << uint32(((BattlegroundWGScore*)itr2->second)->FlagCaptures);        // flag captures
                buff << uint32(((BattlegroundWGScore*)itr2->second)->FlagReturns);         // flag returns
                break;
            case 566:
                data->WriteBits(0x00000001, 24);
                buff << uint32(((BattlegroundEYScore*)itr2->second)->FlagCaptures);        // flag captures
                break;
            case 529:
                data->WriteBits(0x00000002, 24);
                buff << uint32(((BattlegroundABScore*)itr2->second)->BasesAssaulted);      // bases asssulted
                buff << uint32(((BattlegroundABScore*)itr2->second)->BasesDefended);       // bases defended
                break;
            case 30:
                data->WriteBits(0x00000005, 24);
                buff << uint32(((BattlegroundAVScore*)itr2->second)->GraveyardsAssaulted); // GraveyardsAssaulted
                buff << uint32(((BattlegroundAVScore*)itr2->second)->GraveyardsDefended);  // GraveyardsDefended
                buff << uint32(((BattlegroundAVScore*)itr2->second)->TowersAssaulted);     // TowersAssaulted
                buff << uint32(((BattlegroundAVScore*)itr2->second)->TowersDefended);      // TowersDefended
                buff << uint32(((BattlegroundAVScore*)itr2->second)->MinesCaptured);       // MinesCaptured
                break;
            case 607:
                data->WriteBits(0x00000002, 24);
                buff << uint32(((BattlegroundSAScore*)itr2->second)->demolishers_destroyed);
                buff << uint32(((BattlegroundSAScore*)itr2->second)->gates_destroyed);
                break;
            case 628:                                   // IC
                data->WriteBits(0x00000002, 24);
                buff << uint32(((BattlegroundICScore*)itr2->second)->BasesAssaulted);       // bases asssulted
                buff << uint32(((BattlegroundICScore*)itr2->second)->BasesDefended);        // bases defended
                break;
            case 726:
                data->WriteBits(0x00000002, 24);
                buff << uint32(((BattlegroundTPScore*)itr2->second)->FlagCaptures);         // flag captures
                buff << uint32(((BattlegroundTPScore*)itr2->second)->FlagReturns);          // flag returns
                break;
            case 761:
                data->WriteBits(0x00000002, 24);
                buff << uint32(((BattlegroundBGScore*)itr2->second)->BasesAssaulted);      // bases asssulted
                buff << uint32(((BattlegroundBGScore*)itr2->second)->BasesDefended);       // bases defended
                break;
            default:
                data->WriteBits(0, 24);
                break;
            }
            break;
        case BATTLEGROUND_AV:
            data->WriteBits(0x00000005, 24);
            buff << uint32(((BattlegroundAVScore*)itr2->second)->GraveyardsAssaulted); // GraveyardsAssaulted
            buff << uint32(((BattlegroundAVScore*)itr2->second)->GraveyardsDefended);  // GraveyardsDefended
            buff << uint32(((BattlegroundAVScore*)itr2->second)->TowersAssaulted);     // TowersAssaulted
            buff << uint32(((BattlegroundAVScore*)itr2->second)->TowersDefended);      // TowersDefended
            buff << uint32(((BattlegroundAVScore*)itr2->second)->MinesCaptured);       // MinesCaptured
            break;
        case BATTLEGROUND_WS:
            data->WriteBits(0x00000002, 24);
            buff << uint32(((BattlegroundWGScore*)itr2->second)->FlagCaptures);        // flag captures
            buff << uint32(((BattlegroundWGScore*)itr2->second)->FlagReturns);         // flag returns
            break;
        case BATTLEGROUND_AB:
            data->WriteBits(0x00000002, 24);
            buff << uint32(((BattlegroundABScore*)itr2->second)->BasesAssaulted);      // bases asssulted
            buff << uint32(((BattlegroundABScore*)itr2->second)->BasesDefended);       // bases defended
            break;
        case BATTLEGROUND_EY:
            data->WriteBits(0x00000001, 24);
            buff << uint32(((BattlegroundEYScore*)itr2->second)->FlagCaptures);        // flag captures
            break;
        case BATTLEGROUND_SA:
            data->WriteBits(0x00000002, 24);
            buff << uint32(((BattlegroundSAScore*)itr2->second)->demolishers_destroyed);
            buff << uint32(((BattlegroundSAScore*)itr2->second)->gates_destroyed);
            break;
        case BATTLEGROUND_IC:
            data->WriteBits(0x00000002, 24);
            buff << uint32(((BattlegroundICScore*)itr2->second)->BasesAssaulted);       // bases asssulted
            buff << uint32(((BattlegroundICScore*)itr2->second)->BasesDefended);        // bases defended
            break;
        case BATTLEGROUND_TP:
            data->WriteBits(0x00000002, 24);
            buff << uint32(((BattlegroundTPScore*)itr2->second)->FlagCaptures);         // flag captures
            buff << uint32(((BattlegroundTPScore*)itr2->second)->FlagReturns);          // flag returns
            break;
        case BATTLEGROUND_BG:
            data->WriteBits(0x00000002, 24);
            buff << uint32(((BattlegroundBGScore*)itr2->second)->BasesAssaulted);      // bases asssulted
            buff << uint32(((BattlegroundBGScore*)itr2->second)->BasesDefended);       // bases defended
            break;
        case BATTLEGROUND_NA:
        case BATTLEGROUND_BE:
        case BATTLEGROUND_AA:
        case BATTLEGROUND_RL:
        case BATTLEGROUND_DS:
        case BATTLEGROUND_RV:
            data->WriteBits(0, 24);
            break;
        default:
            data->WriteBits(0, 24);
            break;
        }
        data->WriteBit(guid[4]);

        buff.WriteByteSeq(guid[0]);
        buff.WriteByteSeq(guid[3]);

        // if (unk 4) << uint32() unk

        buff.WriteByteSeq(guid[7]);
        buff.WriteByteSeq(guid[2]);

        ++count;
    }

    data->WriteBit(bg->GetStatus() == STATUS_WAIT_LEAVE);    // If Ended
    data->FlushBits();
    data->PutBits<int32>(count_pos, count, 21);              // Number of Players

    if (isRated)                                             // arena TODO : Fix Order on Rated Implementation
    {
        *data << uint32(bg->m_ArenaTeamMMR[playerTeam]);
        *data << uint32(0);
        *data << uint32(0);
        *data << uint32(bg->m_ArenaTeamMMR[oppositeTeam]);
        *data << uint32(0);
        *data << uint32(0);
    }

    data->append(buff);

    if (isArena)
        for (int8 i = 0; i < 2; ++i)
            if (ArenaTeam* at = sObjectMgr->GetArenaTeamById(bg->m_ArenaTeamIds[i]))
                data->WriteString(at->GetName());

    *data << uint8(bg->GetPlayersCountByTeam(HORDE));

    if (bg->GetStatus() == STATUS_WAIT_LEAVE)
        *data << uint8(bg->GetWinner());                    // who win

    *data << uint8(bg->GetPlayersCountByTeam(ALLIANCE));
}

void BattlegroundMgr::BuildStatusFailedPacket(WorldPacket *data, Battleground *bg, Player *pPlayer, uint8 QueueSlot, GroupJoinBattlegroundResult result)
{
    ObjectGuid guidBytes1 = pPlayer ? pPlayer->GetGUID() : 0; // player who caused the error
    ObjectGuid guidBytes2 = bg->GetGUID();
    ObjectGuid unkGuid3 = 0;

    data->Initialize(SMSG_BATTLEFIELD_STATUS_FAILED);

    data->WriteBit(guidBytes2[3]);
    data->WriteBit(unkGuid3[3]);
    data->WriteBit(guidBytes1[3]);
    data->WriteBit(unkGuid3[0]);
    data->WriteBit(guidBytes2[6]);
    data->WriteBit(guidBytes1[5]);
    data->WriteBit(guidBytes1[6]);
    data->WriteBit(guidBytes1[4]);

    data->WriteBit(guidBytes1[2]);
    data->WriteBit(unkGuid3[1]);
    data->WriteBit(guidBytes2[1]);
    data->WriteBit(unkGuid3[5]);
    data->WriteBit(unkGuid3[6]);
    data->WriteBit(guidBytes1[1]);
    data->WriteBit(guidBytes2[7]);
    data->WriteBit(unkGuid3[4]);

    data->WriteBit(guidBytes2[2]);
    data->WriteBit(guidBytes2[5]);
    data->WriteBit(unkGuid3[7]);
    data->WriteBit(guidBytes2[4]);
    data->WriteBit(guidBytes2[0]);
    data->WriteBit(guidBytes1[0]);
    data->WriteBit(unkGuid3[2]);
    data->WriteBit(guidBytes1[7]);

    data->WriteByteSeq(guidBytes2[1]);

    *data << uint32(bg->GetStatus());           // Status
    *data << uint32(QueueSlot);                 // Queue slot

    data->WriteByteSeq(guidBytes1[6]);
    data->WriteByteSeq(guidBytes1[3]);
    data->WriteByteSeq(guidBytes1[7]);
    data->WriteByteSeq(guidBytes1[4]);
    data->WriteByteSeq(guidBytes2[0]);
    data->WriteByteSeq(guidBytes1[5]);
    data->WriteByteSeq(guidBytes2[7]);
    data->WriteByteSeq(guidBytes2[6]);
    data->WriteByteSeq(guidBytes2[2]);
    data->WriteByteSeq(unkGuid3[6]);
    data->WriteByteSeq(unkGuid3[3]);
    data->WriteByteSeq(guidBytes1[1]);
    data->WriteByteSeq(guidBytes2[3]);
    data->WriteByteSeq(unkGuid3[0]);
    data->WriteByteSeq(unkGuid3[1]);
    data->WriteByteSeq(unkGuid3[4]);
    data->WriteByteSeq(guidBytes1[0]);
    data->WriteByteSeq(guidBytes2[5]);
    data->WriteByteSeq(unkGuid3[7]);
    data->WriteByteSeq(guidBytes2[4]);
    data->WriteByteSeq(guidBytes1[2]);

    *data << uint32(result);                    // Result

    data->WriteByteSeq(unkGuid3[2]);

    *data << uint32(pPlayer->GetBattlegroundQueueJoinTime(bg->GetTypeID())); // Join Time

    data->WriteByteSeq(unkGuid3[5]);
}

void BattlegroundMgr::BuildUpdateWorldStatePacket(WorldPacket *data, uint32 field, uint32 value)
{
    data->Initialize(SMSG_UPDATE_WORLD_STATE, 4+4);
    *data << uint32(field);
    *data << uint32(value);
    *data << uint8(0);
}

void BattlegroundMgr::BuildPlaySoundPacket(WorldPacket *data, uint32 soundid)
{
    data->Initialize(SMSG_PLAY_SOUND, 4);
    *data << uint32(soundid);
}

void BattlegroundMgr::BuildPlayerLeftBattlegroundPacket(WorldPacket *data, const uint64& guid)
{
    data->Initialize(SMSG_BATTLEGROUND_PLAYER_LEFT, 8);
    *data << uint64(guid);
}

void BattlegroundMgr::BuildPlayerJoinedBattlegroundPacket(WorldPacket *data, Player *plr)
{
    data->Initialize(SMSG_BATTLEGROUND_PLAYER_JOINED, 8);
    *data << uint64(plr->GetGUID());
}

Battleground * BattlegroundMgr::GetBattlegroundThroughClientInstance(uint32 instanceId, BattlegroundTypeId bgTypeId)
{
    //cause at HandleBattlegroundJoinOpcode the clients sends the instanceid he gets from
    //SMSG_BATTLEFIELD_LIST we need to find the battleground with this clientinstance-id
    Battleground* bg = GetBattlegroundTemplate(bgTypeId);
    if (!bg)
        return NULL;

    if (bg->isArena())
        return GetBattleground(instanceId, bgTypeId);

    for (BattlegroundSet::iterator itr = m_Battlegrounds[bgTypeId].begin(); itr != m_Battlegrounds[bgTypeId].end(); ++itr)
    {
        if (itr->second->GetClientInstanceID() == instanceId)
            return itr->second;
    }
    return NULL;
}

Battleground * BattlegroundMgr::GetBattleground(uint32 InstanceID, BattlegroundTypeId bgTypeId)
{
    if (!InstanceID)
        return NULL;
    //search if needed
    BattlegroundSet::iterator itr;
    if (bgTypeId == BATTLEGROUND_TYPE_NONE)
    {
        for (uint32 i = BATTLEGROUND_AV; i < MAX_BATTLEGROUND_TYPE_ID; i++)
        {
            itr = m_Battlegrounds[i].find(InstanceID);
            if (itr != m_Battlegrounds[i].end())
                return itr->second;
        }
        return NULL;
    }
    itr = m_Battlegrounds[bgTypeId].find(InstanceID);
    return ((itr != m_Battlegrounds[bgTypeId].end()) ? itr->second : NULL);
}

Battleground * BattlegroundMgr::GetBattlegroundTemplate(BattlegroundTypeId bgTypeId)
{
    //map is sorted and we can be sure that lowest instance id has only BG template
    return m_Battlegrounds[bgTypeId].empty() ? NULL : m_Battlegrounds[bgTypeId].begin()->second;
}

// static const field of battlegrounds capable to spectating
static const BattlegroundTypeId spectableArenaTypes[] = {
    BATTLEGROUND_AA,
    BATTLEGROUND_NA,
    BATTLEGROUND_BE,
    BATTLEGROUND_RL,
    BATTLEGROUND_DS,
    BATTLEGROUND_RV};

void BattlegroundMgr::GetSpectatableArenas(BattlegroundSet* target)
{
    if (!target)
        return;

    target->clear();

    for (uint32 i = 0; i < sizeof(spectableArenaTypes)/sizeof(BattlegroundTypeId); i++)
    {
        for (BattlegroundSet::iterator itr = m_Battlegrounds[spectableArenaTypes[i]].begin(); itr != m_Battlegrounds[spectableArenaTypes[i]].end(); ++itr)
        {
            if (itr->second && itr->second->GetStatus() == STATUS_IN_PROGRESS)
                (*target)[itr->first] = itr->second;
        }
    }
}

bool BattlegroundMgr::AddArenaSpectator(Player* pl, uint32 instanceID)
{
    if (!pl || instanceID == 0)
        return false;

    if (pl->isInCombat() || pl->InBattleground() || pl->isDead())
    {
        pl->GetSession()->SendNotification("Couldn't teleport to arena as spectator due combat/battleground/flight/death.");
        return false;
    }

    for (uint32 i = 0; i < sizeof(spectableArenaTypes)/sizeof(BattlegroundTypeId); i++)
    {
        BattlegroundSet::iterator itr = m_Battlegrounds[spectableArenaTypes[i]].find(instanceID);
        if (itr != m_Battlegrounds[spectableArenaTypes[i]].end())
        {
            if (itr->second && itr->second->GetStatus() == STATUS_IN_PROGRESS)
            {
                itr->second->AddSpectator(pl);
                return true;
            }
            else
            {
                // process errors
                return false;
            }
            break;
        }
    }

    return false;
}

void BattlegroundMgr::RemoveArenaSpectator(Player* pl)
{
    if (!pl)
        return;

    uint32 instanceID = pl->GetSpectatorInstanceId();

    // attempt to remove from nowhere?
    if (instanceID == 0)
        return;

    for (uint32 i = 0; i < sizeof(spectableArenaTypes)/sizeof(BattlegroundTypeId); i++)
    {
        BattlegroundSet::iterator itr = m_Battlegrounds[spectableArenaTypes[i]].find(instanceID);
        if (itr != m_Battlegrounds[spectableArenaTypes[i]].end())
        {
            if (itr->second)
            {
                itr->second->RemoveSpectator(pl);
                return;
            }
        }
    }
}

uint32 BattlegroundMgr::CreateClientVisibleInstanceId(BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket_id)
{
    if (IsArenaType(bgTypeId))
        return 0;                                           //arenas don't have client-instanceids

    // we create here an instanceid, which is just for
    // displaying this to the client and without any other use..
    // the client-instanceIds are unique for each battleground-type
    // the instance-id just needs to be as low as possible, beginning with 1
    // the following works, because std::set is default ordered with "<"
    // the optimalization would be to use as bitmask std::vector<uint32> - but that would only make code unreadable
    uint32 lastId = 0;
    for (std::set<uint32>::iterator itr = m_ClientBattlegroundIds[bgTypeId][bracket_id].begin(); itr != m_ClientBattlegroundIds[bgTypeId][bracket_id].end();)
    {
        if ((++lastId) != *itr)                             //if there is a gap between the ids, we will break..
            break;
        lastId = *itr;
    }
    m_ClientBattlegroundIds[bgTypeId][bracket_id].insert(lastId + 1);
    return lastId + 1;
}

// create a new battleground that will really be used to play
Battleground * BattlegroundMgr::CreateNewBattleground(BattlegroundTypeId bgTypeId, PvPDifficultyEntry const* bracketEntry, uint8 arenaType, bool isRated, bool isWargame)
{
    // get the template BG
    Battleground *bg_template = GetBattlegroundTemplate(bgTypeId);
    BattlegroundSelectionWeightMap *selectionWeights = NULL;
    BattlegroundTypeId originalBgTypeId = bgTypeId;

    if (!bg_template)
    {
        sLog->outError("Battleground: CreateNewBattleground - bg template not found for %u", bgTypeId);
        return NULL;
    }
    bool isRandom = false;

    if (bg_template->isArena())
        selectionWeights = &m_ArenaSelectionWeights;
    else if (bgTypeId == BATTLEGROUND_RB)
    {
        selectionWeights = &m_BGSelectionWeights;
        isRandom = true;
    }
    else if (bg_template->isBattleground() && isRated)
    {
        std::vector<BattlegroundTypeId> possibleBgs;

        switch (bgTypeId)
        {
            case BATTLEGROUND_RA_BG_10:
                possibleBgs.push_back(BATTLEGROUND_WS);
                possibleBgs.push_back(BATTLEGROUND_BG);
                possibleBgs.push_back(BATTLEGROUND_TP);
                possibleBgs.push_back(BATTLEGROUND_AB);
                break;
            case BATTLEGROUND_RA_BG_15:
                possibleBgs.push_back(BATTLEGROUND_AB);
                possibleBgs.push_back(BATTLEGROUND_EY);
                break;
            default:
                break;
        }

        if (possibleBgs.empty())
        {
            sLog->outError("Battleground: CreateNewBattleground - no rated battleground choosen for %u", bgTypeId);
            return NULL;
        }
        
        bgTypeId = possibleBgs[irand(0, possibleBgs.size() - 1)];
        bg_template = GetBattlegroundTemplate(bgTypeId);

        if (!bg_template)
        {
            sLog->outError("Battleground: CreateNewBattleground - bg template not found for rated %u", bgTypeId);
            return NULL;
        }
    }

    if (selectionWeights && (!isWargame || bgTypeId == BATTLEGROUND_AA))
    {
#if 0
        if (!selectionWeights->size())
           return NULL;
        uint32 Weight = 0;
        uint32 selectedWeight = 0;
        bgTypeId = BATTLEGROUND_TYPE_NONE;
        // Get sum of all weights
        for (BattlegroundSelectionWeightMap::const_iterator it = selectionWeights->begin(); it != selectionWeights->end(); ++it)
            Weight += it->second;
        if (!Weight)
            return NULL;
        // Select a random value
        selectedWeight = urand(0, Weight-1);

        // Select the correct bg (if we have in DB A(10), B(20), C(10), D(15) --> [0---A---9|10---B---29|30---C---39|40---D---54])
        Weight = 0;
        for (BattlegroundSelectionWeightMap::const_iterator it = selectionWeights->begin(); it != selectionWeights->end(); ++it)
        {
            Weight += it->second;
            if (selectedWeight < Weight)
            {
                bgTypeId = it->first;
                break;
            }
        }
#endif
        // We must differ arenas, because of they are using the same function to create instance
        if (!bg_template->isArena())
        {
            // Twin Peaks and Battle for Gilneas exists only for level 85 (PvpDifficulty.dbc)
            // We must select different list of BGs in our custom system
            if (bracketEntry->minLevel == 85)
            {
                BattlegroundTypeId avail_RBGs[] = {
                    BATTLEGROUND_WS,
                    BATTLEGROUND_AB,
                    BATTLEGROUND_EY,
                    BATTLEGROUND_TP,
                    BATTLEGROUND_BG,
                    BATTLEGROUND_TYPE_NONE,
                };

                time_t secs = time(NULL);
                tm* date = localtime(&secs);

                // Allow Alterac Valley only from 16:00 to 22:00 every day
                if (date && date->tm_hour >= 16 && date->tm_hour <= 22)
                {
                    for (uint8 i = 0; i < (sizeof(avail_RBGs)/sizeof(BattlegroundTypeId)); i++)
                    {
                        if (avail_RBGs[i] == BATTLEGROUND_TYPE_NONE)
                        {
                            avail_RBGs[i] = BATTLEGROUND_AV;
                            break;
                        }
                    }
                }

                uint32 bgCount = sizeof(avail_RBGs)/sizeof(BattlegroundTypeId);

                do {
                    bgTypeId = avail_RBGs[m_RandomBGGenerator.Generate(bgCount)];
                } while (bgTypeId == BATTLEGROUND_TYPE_NONE);
            }
            else
            {
                BattlegroundTypeId avail_RBGs[] = {
                    BATTLEGROUND_WS,
                    BATTLEGROUND_AB,
                    BATTLEGROUND_EY,
                };

                uint32 bgCount = sizeof(avail_RBGs)/sizeof(BattlegroundTypeId);
                bgTypeId = avail_RBGs[m_RandomBGGenerator.Generate(bgCount)];
            }
        }
        else
        {
            BattlegroundTypeId avail_RBGs[] = {
                BATTLEGROUND_NA, // Nagrand Arena
                BATTLEGROUND_BE, // Blade's Edge Arena
                BATTLEGROUND_RL, // Ruins of Lordaeron
                //BATTLEGROUND_RV, // Ring of Valor
                BATTLEGROUND_DS, // Dalaran Severs
            };

            uint32 bgCount = sizeof(avail_RBGs)/sizeof(BattlegroundTypeId);
            bgTypeId = avail_RBGs[m_ArenaGenerator.Generate(bgCount)];
        }

        bg_template = GetBattlegroundTemplate(bgTypeId);
        if (!bg_template)
        {
            sLog->outError("Battleground: CreateNewBattleground - bg template not found for %u", bgTypeId);
            return NULL;
        }
    }

    Battleground *bg = NULL;
    // create a copy of the BG template
    switch(bgTypeId)
    {
        case BATTLEGROUND_AV:
            bg = new BattlegroundAV(*(BattlegroundAV*)bg_template);
            break;
        case BATTLEGROUND_WS:
            bg = new BattlegroundWS(*(BattlegroundWS*)bg_template);
            break;
        case BATTLEGROUND_AB:
            bg = new BattlegroundAB(*(BattlegroundAB*)bg_template);
            break;
        case BATTLEGROUND_NA:
            bg = new BattlegroundNA(*(BattlegroundNA*)bg_template);
            break;
        case BATTLEGROUND_BE:
            bg = new BattlegroundBE(*(BattlegroundBE*)bg_template);
            break;
        case BATTLEGROUND_AA:
            bg = new BattlegroundAA(*(BattlegroundAA*)bg_template);
            break;
        case BATTLEGROUND_EY:
            bg = new BattlegroundEY(*(BattlegroundEY*)bg_template);
            break;
        case BATTLEGROUND_RL:
            bg = new BattlegroundRL(*(BattlegroundRL*)bg_template);
            break;
        case BATTLEGROUND_SA:
            bg = new BattlegroundSA(*(BattlegroundSA*)bg_template);
            break;
        case BATTLEGROUND_DS:
            bg = new BattlegroundDS(*(BattlegroundDS*)bg_template);
            break;
        case BATTLEGROUND_RV:
            bg = new BattlegroundRV(*(BattlegroundRV*)bg_template);
            break;
        case BATTLEGROUND_IC:
            bg = new BattlegroundIC(*(BattlegroundIC*)bg_template);
            break;
        case BATTLEGROUND_BG:
            bg = new BattlegroundBG(*(BattlegroundBG*)bg_template);
            break;
        case BATTLEGROUND_TP:
            bg = new BattlegroundTP(*(BattlegroundTP*)bg_template);
            break;
        case BATTLEGROUND_RB:
            bg = new BattlegroundRB(*(BattlegroundRB*)bg_template);
            break;
        default:
            //error, but it is handled few lines above
            return 0;
    }

    // set battelground difficulty before initialization
    bg->SetBracket(bracketEntry);

    // generate a new instance id
    bg->SetInstanceID(sMapMgr->GenerateInstanceId()); // set instance id
    if (isRandom)
        bg->SetClientInstanceID(CreateClientVisibleInstanceId(BATTLEGROUND_RB, bracketEntry->GetBracketId()));
    else if (bg->isBattleground() && isRated)
        bg->SetClientInstanceID(CreateClientVisibleInstanceId(originalBgTypeId, bracketEntry->GetBracketId()));
    else
        bg->SetClientInstanceID(CreateClientVisibleInstanceId(bgTypeId, bracketEntry->GetBracketId()));

    // reset the new bg (set status to status_wait_queue from status_none)
    bg->Reset();

    // start the joining of the bg
    bg->SetStatus(STATUS_WAIT_JOIN);
    bg->SetArenaType(arenaType);
    bg->SetRated(isRated);
    bg->SetWargame(isWargame);
    bg->SetRandom(isRandom);
    if (isRandom)
        bg->SetTypeID(BATTLEGROUND_RB);
    else if (bg->isBattleground() && isRated)
        bg->SetTypeID(originalBgTypeId);
    else
        bg->SetTypeID(bgTypeId);
    bg->SetRandomTypeID(bgTypeId);
    bg->SetGUID(MAKE_NEW_GUID(bg->GetTypeID(false), 0, HIGHGUID_BATTLEGROUND));

    return bg;
}

// used to create the BG templates
uint32 BattlegroundMgr::CreateBattleground(BattlegroundTypeId bgTypeId, bool IsArena, uint32 MinPlayersPerTeam, uint32 MaxPlayersPerTeam, uint32 LevelMin, uint32 LevelMax, const char* BattlegroundName, uint32 MapID, float Team1StartLocX, float Team1StartLocY, float Team1StartLocZ, float Team1StartLocO, float Team2StartLocX, float Team2StartLocY, float Team2StartLocZ, float Team2StartLocO, uint32 scriptId)
{
    // Create the BG
    Battleground *bg = NULL;
    switch(bgTypeId)
    {
        case BATTLEGROUND_AV: bg = new BattlegroundAV; break;
        case BATTLEGROUND_WS: bg = new BattlegroundWS; break;
        case BATTLEGROUND_AB: bg = new BattlegroundAB; break;
        case BATTLEGROUND_NA: bg = new BattlegroundNA; break;
        case BATTLEGROUND_BE: bg = new BattlegroundBE; break;
        case BATTLEGROUND_AA: bg = new BattlegroundAA; break;
        case BATTLEGROUND_EY: bg = new BattlegroundEY; break;
        case BATTLEGROUND_RL: bg = new BattlegroundRL; break;
        case BATTLEGROUND_SA: bg = new BattlegroundSA; break;
        case BATTLEGROUND_DS: bg = new BattlegroundDS; break;
        case BATTLEGROUND_RV: bg = new BattlegroundRV; break;
        case BATTLEGROUND_IC: bg = new BattlegroundIC; break;
        case BATTLEGROUND_BG: bg = new BattlegroundBG; break;
        case BATTLEGROUND_TP: bg = new BattlegroundTP; break;
        case BATTLEGROUND_RB: bg = new BattlegroundRB; break;
        default:
            bg = new Battleground;
            break;
    }

    bg->SetMapId(MapID);
    bg->SetTypeID(bgTypeId);
    bg->SetInstanceID(0);
    bg->SetArenaorBGType(IsArena);
    bg->SetMinPlayersPerTeam(MinPlayersPerTeam);
    bg->SetMaxPlayersPerTeam(MaxPlayersPerTeam);
    bg->SetMinPlayers(MinPlayersPerTeam * 2);
    bg->SetMaxPlayers(MaxPlayersPerTeam * 2);
    bg->SetName(BattlegroundName);
    bg->SetTeamStartLoc(ALLIANCE, Team1StartLocX, Team1StartLocY, Team1StartLocZ, Team1StartLocO);
    bg->SetTeamStartLoc(HORDE,    Team2StartLocX, Team2StartLocY, Team2StartLocZ, Team2StartLocO);
    bg->SetLevelRange(LevelMin, LevelMax);
    bg->SetScriptId(scriptId);
    bg->SetGUID(MAKE_NEW_GUID(bgTypeId, 0, HIGHGUID_BATTLEGROUND));

    // add bg to update list
    AddBattleground(bg->GetInstanceID(), bg->GetTypeID(), bg);

    // return some not-null value, bgTypeId is good enough for me
    return bgTypeId;
}

void BattlegroundMgr::CreateInitialBattlegrounds()
{
    float AStartLoc[4];
    float HStartLoc[4];
    uint32 MaxPlayersPerTeam, MinPlayersPerTeam, MinLvl, MaxLvl, start1, start2;
    uint8 selectionWeight;
    BattlemasterListEntry const *bl;
    WorldSafeLocsEntry const *start;
    bool IsArena;
    uint32 scriptId = 0;
    uint32 count = 0;

    //                                                       0   1                 2                 3      4      5                6              7             8           9      10
    QueryResult result = WorldDatabase.Query("SELECT id, MinPlayersPerTeam,MaxPlayersPerTeam,MinLvl,MaxLvl,AllianceStartLoc,AllianceStartO,HordeStartLoc,HordeStartO,Weight,ScriptName FROM battleground_template");

    if (!result)
    {
        sLog->outString();
        sLog->outErrorDb(">> Loaded 0 battlegrounds. DB table `battleground_template` is empty.");
        return;
    }
    do
    {
        Field *fields = result->Fetch();
        uint32 bgTypeID_ = fields[0].GetUInt32();
        if (sDisableMgr->IsDisabledFor(DISABLE_TYPE_BATTLEGROUND, bgTypeID_, NULL))
            continue;

        // can be overwrite by values from DB
        bl = sBattlemasterListStore.LookupEntry(bgTypeID_);
        if (!bl)
        {
            sLog->outError("Battleground ID %u not found in BattlemasterList.dbc. Battleground not created.", bgTypeID_);
            continue;
        }

        BattlegroundTypeId bgTypeID = BattlegroundTypeId(bgTypeID_);

        // bl->rated attribute is set to 2 when it's rated battleground. Arenas are rated, but have this field set to 0 !
        IsArena = (bl->type == TYPE_ARENA) && (bl->rated == 0);
        MinPlayersPerTeam = fields[1].GetUInt32();
        MaxPlayersPerTeam = fields[2].GetUInt32();
        MinLvl = fields[3].GetUInt32();
        MaxLvl = fields[4].GetUInt32();
        //check values from DB
        if (MaxPlayersPerTeam == 0 || MinPlayersPerTeam == 0 || MinPlayersPerTeam > MaxPlayersPerTeam)
        {
            MinPlayersPerTeam = 0;                          // by default now expected strong full bg requirement
            MaxPlayersPerTeam = 40;
        }
        if (MinLvl == 0 || MaxLvl == 0 || MinLvl > MaxLvl)
        {
            //TO-DO: FIX ME
            MinLvl = 0;//bl->minlvl;
            MaxLvl = 85;//bl->maxlvl;
        }

        start1 = fields[5].GetUInt32();

        start = sWorldSafeLocsStore.LookupEntry(start1);
        if (start)
        {
            AStartLoc[0] = start->x;
            AStartLoc[1] = start->y;
            AStartLoc[2] = start->z;
            AStartLoc[3] = fields[6].GetFloat();
        }
        else if (bgTypeID == BATTLEGROUND_AA || bgTypeID == BATTLEGROUND_RB || bl->rated)
        {
            AStartLoc[0] = 0;
            AStartLoc[1] = 0;
            AStartLoc[2] = 0;
            AStartLoc[3] = fields[6].GetFloat();
        }
        else
        {
            sLog->outErrorDb("Table `battleground_template` for id %u have non-existed WorldSafeLocs.dbc id %u in field `AllianceStartLoc`. BG not created.", bgTypeID, start1);
            continue;
        }

        start2 = fields[7].GetUInt32();

        start = sWorldSafeLocsStore.LookupEntry(start2);
        if (start)
        {
            HStartLoc[0] = start->x;
            HStartLoc[1] = start->y;
            HStartLoc[2] = start->z;
            HStartLoc[3] = fields[8].GetFloat();
        }
        else if (bgTypeID == BATTLEGROUND_AA || bgTypeID == BATTLEGROUND_RB || bl->rated)
        {
            HStartLoc[0] = 0;
            HStartLoc[1] = 0;
            HStartLoc[2] = 0;
            HStartLoc[3] = fields[8].GetFloat();
        }
        else
        {
            sLog->outErrorDb("Table `battleground_template` for id %u have non-existed WorldSafeLocs.dbc id %u in field `HordeStartLoc`. BG not created.", bgTypeID, start2);
            continue;
        }

        selectionWeight = fields[9].GetUInt8();
        scriptId = sObjectMgr->GetScriptId(fields[10].GetCString());
        //sLog->outDetail("Creating battleground %s, %u-%u", bl->name[sWorld->GetDBClang()], MinLvl, MaxLvl);
        if (!CreateBattleground(bgTypeID, IsArena, MinPlayersPerTeam, MaxPlayersPerTeam, MinLvl, MaxLvl, bl->name, bl->mapid[0], AStartLoc[0], AStartLoc[1], AStartLoc[2], AStartLoc[3], HStartLoc[0], HStartLoc[1], HStartLoc[2], HStartLoc[3], scriptId))
            continue;

        if (IsArena)
        {
            if (bgTypeID != BATTLEGROUND_AA)
                m_ArenaSelectionWeights[bgTypeID] = selectionWeight;
        }
        else if (bgTypeID != BATTLEGROUND_RB)
            m_BGSelectionWeights[bgTypeID] = selectionWeight;
        ++count;
    } while (result->NextRow());

    sLog->outString();
    sLog->outString(">> Loaded %u battlegrounds", count);
}

void BattlegroundMgr::InitAutomaticArenaPointDistribution()
{
    if (!sWorld->getBoolConfig(CONFIG_ARENA_AUTO_DISTRIBUTE_POINTS))
        return;

    time_t wstime = time_t(sWorld->getWorldState(WS_ARENA_DISTRIBUTION_TIME));
    time_t curtime = time(NULL);
    sLog->outDebug("Initializing Automatic Arena Point Distribution");
    if (wstime < curtime)
    {
        m_NextAutoDistributionTime = curtime;           // reset will be called in the next update
        sLog->outDebug("Battleground: Next arena point distribution time in the past, reseting it now.");
    }
    else
        m_NextAutoDistributionTime = wstime;
    sLog->outDebug("Automatic Arena Point Distribution initialized.");
}

uint32 BattlegroundMgr::CalculateArenaCap(uint32 rating)
{
    float cap = 0;

    // cap is higher than 1350 only if player has rating > 1500
    if (rating > 1500 && rating <= 2985)
    {
        cap = -1.00844494413448 * pow(10.0,-12) * pow(double(rating),5) + 1.2356986230482 * pow(10.0,-8) * pow(double(rating),4)
                    + -5.94771172066112 * pow(10.0,-5) * pow(double(rating),3) + 0.139443656834417 * pow(double(rating),2) + -156.936920229832 * rating + 68836.3;
        /* we reuse original pre-4.1 formula
         * The original formula ranged from 1343 to 3000. We move it to the beginning (-1343),
         * shrink the difference (3000-1343 = 1657, 2700-1350 = 1350, 1350/1657 = ~0,8147254),
         * and move it to the real start (+1350)
         */

        cap -= 1343.0f;
        cap = cap*(1350.0f/1657.0f);
        cap += 1350.0f;
    }
    // formula has maximum in 2985, avoid lower cap for higher ratings
    else if (rating > 2985)
        cap = 2700.0f;
    else
        cap = 1350.0f;

    return (uint32)cap;
}

uint32 BattlegroundMgr::CalculateRatedBattlegroundCap(uint32 rating)
{
    // Rated battlegrounds has 22.2% higher cap than arenas
    return (uint32)ceil(((float)CalculateArenaCap(rating))*1.222f);
}

void BattlegroundMgr::UpdateRatedBattlegroundCap()
{
    Player* tmp = NULL;
    uint32 lowguid;
    uint32 rating, cap;
    Field* fields = NULL;

    // at first, update online players BG conquest cap
    for (SessionMap::const_iterator itr = sWorld->GetAllSessions().begin(); itr != sWorld->GetAllSessions().end(); ++itr)
    {
        tmp = itr->second->GetPlayer();
        if (!itr->second || !tmp)
            continue;

        rating = tmp->GetRatedBattlegroundRating();
        cap = CalculateRatedBattlegroundCap(rating)*GetCurrencyPrecision(CURRENCY_TYPE_CONQUEST_POINTS);

        tmp->SetCurrencyWeekCap(CURRENCY_TYPE_CONQUEST_POINTS, CURRENCY_SOURCE_BG, cap);
    }

    QueryResult result = CharacterDatabase.PQuery("SELECT guid, rating FROM character_rated_bg_stats");
    if (result)
    {
        do
        {
            fields = result->Fetch();
            lowguid = fields[0].GetUInt32();
            rating = fields[1].GetUInt32();

            cap = CalculateRatedBattlegroundCap(rating)*GetCurrencyPrecision(CURRENCY_TYPE_CONQUEST_POINTS);

            CharacterDatabase.PExecute("REPLACE INTO character_currency_weekcap VALUES (%u, %u, %u, %u, 0)",
                lowguid, CURRENCY_TYPE_CONQUEST_POINTS, CURRENCY_SOURCE_BG, cap);
        } while (result->NextRow());
    }
}

uint32 BattlegroundMgr::GetHighestArenaRating(uint32 lowguid)
{
    uint32 tmprat = 0;
    uint64 fullguid = MAKE_NEW_GUID(lowguid, 0, HIGHGUID_PLAYER);

    for (ObjectMgr::ArenaTeamMap::iterator team_itr = sObjectMgr->GetArenaTeamMapBegin(); team_itr != sObjectMgr->GetArenaTeamMapEnd(); ++team_itr)
        if (ArenaTeam* at = team_itr->second)
            if (ArenaTeamMember* atm = at->GetMember(fullguid))
                if (atm->personal_rating > tmprat)
                    tmprat = atm->personal_rating;

    return tmprat;
}

void BattlegroundMgr::DistributeArenaCurrency()
{
    // used to distribute arena points based on last week's stats
    sWorld->SendWorldText(LANG_DIST_ARENA_POINTS_START);

    sWorld->SendWorldText(LANG_DIST_ARENA_POINTS_ONLINE_START);

    //calculate new conquest point cap for each player
    for (ObjectMgr::ArenaTeamMap::iterator team_itr = sObjectMgr->GetArenaTeamMapBegin(); team_itr != sObjectMgr->GetArenaTeamMapEnd(); ++team_itr)
        if (ArenaTeam* at = team_itr->second)
            at->UpdateMembersConquestPointCap();

    UpdateRatedBattlegroundCap();

    sWorld->SendWorldText(LANG_DIST_ARENA_POINTS_ONLINE_END);

    sWorld->SendWorldText(LANG_DIST_ARENA_POINTS_TEAM_START);
    for (ObjectMgr::ArenaTeamMap::iterator titr = sObjectMgr->GetArenaTeamMapBegin(); titr != sObjectMgr->GetArenaTeamMapEnd(); ++titr)
    {
        if (ArenaTeam * at = titr->second)
        {
            at->FinishWeek();                              // set played this week etc values to 0 in memory, too
            at->SaveToDB();                                // save changes
            at->NotifyStatsChanged();                      // notify the players of the changes
        }
    }

    sWorld->SendWorldText(LANG_DIST_ARENA_POINTS_TEAM_END);

    sWorld->SendWorldText(LANG_DIST_ARENA_POINTS_END);
}

void BattlegroundMgr::BuildBattlegroundListPacket(WorldPacket *data, const uint64& guid, Player* plr, BattlegroundTypeId bgTypeId)
{
    if (!plr)
        return;

    uint32 winner_conquest, winner_honor, loser_honor;

    if (plr->getLevel() >= 85)
    {
        winner_conquest = plr->GetRandomWinner() ? BG_REWARD_WINNER_CONQUEST_FIRST_85 : BG_REWARD_WINNER_CONQUEST_85;
        winner_honor    = plr->GetRandomWinner() ? BG_REWARD_WINNER_HONOR_FIRST_85 : BG_REWARD_WINNER_HONOR_85;
        loser_honor     = BG_REWARD_LOSER_HONOR_85;
    }
    else
    {
        winner_conquest = 0;
        winner_honor    = plr->GetRandomWinner() ? BG_REWARD_WINNER_FIRST : BG_REWARD_WINNER_HONOR_85;
        loser_honor     = BG_REWARD_LOSER;
    }

    ObjectGuid guidBytes = guid;

    data->Initialize(SMSG_BATTLEFIELD_LIST);
    *data << uint32(winner_conquest);                       // Winner Conquest Reward or Random Winner Conquest Reward
    *data << uint32(winner_conquest);                       // Winner Conquest Reward or Random Winner Conquest Reward
    *data << uint32(loser_honor);                           // Loser Honor Reward or Random Loser Honor Reward
    *data << uint32(bgTypeId);                              // battleground id
    *data << uint32(loser_honor);                           // Loser Honor Reward or Random Loser Honor Reward
    *data << uint32(winner_honor);                          // Winner Honor Reward or Random Winner Honor Reward
    *data << uint32(winner_honor);                          // Winner Honor Reward or Random Winner Honor Reward
    *data << uint8(0);                                      // max level
    *data << uint8(0);                                      // min level

    data->WriteBit(guidBytes[0]);
    data->WriteBit(guidBytes[1]);
    data->WriteBit(guidBytes[7]);
    data->WriteBit(0);                                      // unk
    data->WriteBit(0);                                      // unk

    data->FlushBits();
    size_t count_pos = data->bitwpos();
    data->WriteBits(0, 24);                                 // placeholder

    data->WriteBit(guidBytes[6]);
    data->WriteBit(guidBytes[4]);
    data->WriteBit(guidBytes[2]);
    data->WriteBit(guidBytes[3]);
    data->WriteBit(0);                                      // unk
    data->WriteBit(guidBytes[5]);
    data->WriteBit(0);                                      // unk

    data->FlushBits();

    data->WriteByteSeq(guidBytes[6]);
    data->WriteByteSeq(guidBytes[1]);
    data->WriteByteSeq(guidBytes[7]);
    data->WriteByteSeq(guidBytes[5]);

    BattlegroundSet &bSet = m_Battlegrounds[bgTypeId];
    if (!bSet.empty())
    {
        // expected bracket entry
        if (PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bSet.begin()->second->GetMapId(), plr->getLevel()))
        {
            uint32 count = 0;
            BattlegroundBracketId bracketId = bracketEntry->GetBracketId();
            BattlegroundClientIdsContainer &clientIds = m_ClientBattlegroundIds[bgTypeId][bracketId];
            for (BattlegroundClientIdsContainer::const_iterator itr = clientIds.begin(); itr != clientIds.end(); ++itr)
            {
                *data << uint32(*itr);
                ++count;
            }
            data->PutBits(count_pos, count, 24);            // bg instance count
        }
    }

    data->WriteByteSeq(guidBytes[0]);
    data->WriteByteSeq(guidBytes[2]);
    data->WriteByteSeq(guidBytes[4]);
    data->WriteByteSeq(guidBytes[3]);
}

void BattlegroundMgr::SendToBattleground(Player *pl, uint32 instanceId, BattlegroundTypeId bgTypeId)
{
    Battleground *bg = GetBattleground(instanceId, bgTypeId);
    if (bg)
    {
        uint32 mapid = bg->GetMapId();
        float x, y, z, O;
        uint32 team = pl->GetBGTeam();
        if (team == 0)
            team = pl->GetTeam();
        bg->GetTeamStartLoc(team, x, y, z, O);

        sLog->outDetail("BATTLEGROUND: Sending %s to map %u, X %f, Y %f, Z %f, O %f", pl->GetName(), mapid, x, y, z, O);
        pl->TeleportTo(mapid, x, y, z, O);
    }
    else
    {
        sLog->outError("player %u trying to port to non-existent bg instance %u",pl->GetGUIDLow(), instanceId);
    }
}

void BattlegroundMgr::SendAreaSpiritHealerQueryOpcode(Player *pl, Battleground *bg, const uint64& guid)
{
    WorldPacket data(SMSG_AREA_SPIRIT_HEALER_TIME, 12);
    uint32 time_ = 30000 - bg->GetLastResurrectTime();      // resurrect every 30 seconds
    if (time_ == uint32(-1))
        time_ = 0;
    data << guid << time_;
    pl->GetSession()->SendPacket(&data);
}

bool BattlegroundMgr::IsArenaType(BattlegroundTypeId bgTypeId)
{
    return (bgTypeId == BATTLEGROUND_AA ||
        bgTypeId == BATTLEGROUND_BE ||
        bgTypeId == BATTLEGROUND_NA ||
        bgTypeId == BATTLEGROUND_DS ||
        bgTypeId == BATTLEGROUND_RV ||
        bgTypeId == BATTLEGROUND_RL ||
        bgTypeId == BATTLEGROUND_DS);
}

BattlegroundQueueTypeId BattlegroundMgr::BGQueueTypeId(BattlegroundTypeId bgTypeId, uint8 arenaType)
{
    switch(bgTypeId)
    {
        case BATTLEGROUND_WS:
            return BATTLEGROUND_QUEUE_WS;
        case BATTLEGROUND_AB:
            return BATTLEGROUND_QUEUE_AB;
        case BATTLEGROUND_AV:
            return BATTLEGROUND_QUEUE_AV;
        case BATTLEGROUND_EY:
            return BATTLEGROUND_QUEUE_EY;
        case BATTLEGROUND_SA:
            return BATTLEGROUND_QUEUE_SA;
        case BATTLEGROUND_IC:
            return BATTLEGROUND_QUEUE_IC;
        case BATTLEGROUND_BG:
            return BATTLEGROUND_QUEUE_BG;
        case BATTLEGROUND_TP:
            return BATTLEGROUND_QUEUE_TP;
        case BATTLEGROUND_RB:
            return BATTLEGROUND_QUEUE_RB;
        case BATTLEGROUND_AA:
        case BATTLEGROUND_NA:
        case BATTLEGROUND_RL:
        case BATTLEGROUND_BE:
        case BATTLEGROUND_DS:
        case BATTLEGROUND_RV:
            switch(arenaType)
            {
                case ARENA_TYPE_2v2:
                    return BATTLEGROUND_QUEUE_2v2;
                case ARENA_TYPE_3v3:
                    return BATTLEGROUND_QUEUE_3v3;
                case ARENA_TYPE_5v5:
                    return BATTLEGROUND_QUEUE_5v5;
                default:
                    return BATTLEGROUND_QUEUE_NONE;
            }
        case BATTLEGROUND_RA_BG_10:
            return BATTLEGROUND_QUEUE_RA_BG_10;
        case BATTLEGROUND_RA_BG_15:
            return BATTLEGROUND_QUEUE_RA_BG_15;
        default:
            return BATTLEGROUND_QUEUE_NONE;
    }
}

BattlegroundTypeId BattlegroundMgr::BGTemplateId(BattlegroundQueueTypeId bgQueueTypeId)
{
    switch(bgQueueTypeId)
    {
        case BATTLEGROUND_QUEUE_WS:
            return BATTLEGROUND_WS;
        case BATTLEGROUND_QUEUE_AB:
            return BATTLEGROUND_AB;
        case BATTLEGROUND_QUEUE_AV:
            return BATTLEGROUND_AV;
        case BATTLEGROUND_QUEUE_EY:
            return BATTLEGROUND_EY;
        case BATTLEGROUND_QUEUE_SA:
            return BATTLEGROUND_SA;
        case BATTLEGROUND_QUEUE_IC:
            return BATTLEGROUND_IC;
        case BATTLEGROUND_QUEUE_BG:
            return BATTLEGROUND_BG;
        case BATTLEGROUND_QUEUE_TP:
            return BATTLEGROUND_TP;
        case BATTLEGROUND_QUEUE_RB:
            return BATTLEGROUND_RB;
        case BATTLEGROUND_QUEUE_2v2:
        case BATTLEGROUND_QUEUE_3v3:
        case BATTLEGROUND_QUEUE_5v5:
            return BATTLEGROUND_AA;
        case BATTLEGROUND_QUEUE_RA_BG_10:
            return BATTLEGROUND_RA_BG_10;
        case BATTLEGROUND_QUEUE_RA_BG_15:
            return BATTLEGROUND_RA_BG_15;
        default:
            return BattlegroundTypeId(0);                   // used for unknown template (it existed and do nothing)
    }
}

uint8 BattlegroundMgr::BGArenaType(BattlegroundQueueTypeId bgQueueTypeId)
{
    switch(bgQueueTypeId)
    {
        case BATTLEGROUND_QUEUE_2v2:
            return ARENA_TYPE_2v2;
        case BATTLEGROUND_QUEUE_3v3:
            return ARENA_TYPE_3v3;
        case BATTLEGROUND_QUEUE_5v5:
            return ARENA_TYPE_5v5;
        default:
            return 0;
    }
}

void BattlegroundMgr::ToggleTesting()
{
    m_Testing = !m_Testing;
    if (m_Testing)
        sWorld->SendWorldText(LANG_DEBUG_BG_ON);
    else
        sWorld->SendWorldText(LANG_DEBUG_BG_OFF);
}

void BattlegroundMgr::ToggleArenaTesting()
{
    m_ArenaTesting = !m_ArenaTesting;
    if (m_ArenaTesting)
        sWorld->SendWorldText(LANG_DEBUG_ARENA_ON);
    else
        sWorld->SendWorldText(LANG_DEBUG_ARENA_OFF);
}

void BattlegroundMgr::SetHolidayWeekends(uint32 mask)
{
    for (uint32 bgtype = 1; bgtype < MAX_BATTLEGROUND_TYPE_ID; ++bgtype)
    {
        if (Battleground * bg = GetBattlegroundTemplate(BattlegroundTypeId(bgtype)))
        {
            bg->SetHoliday(mask & (1 << bgtype));
        }
    }
}

void BattlegroundMgr::ScheduleQueueUpdate(uint32 arenaMatchmakerRating, uint8 arenaType, BattlegroundQueueTypeId bgQueueTypeId, BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket_id, uint8 twink)
{
    //This method must be atomic, TODO add mutex
    //we will use only 1 number created of bgTypeId and bracket_id
    uint64 schedule_id = ((uint64)twink << 56) | ((uint64)(arenaMatchmakerRating & 0x00FFFFFF) << 32) | (arenaType << 24) | (bgQueueTypeId << 16) | (bgTypeId << 8) | bracket_id;
    bool found = false;
    for (uint8 i = 0; i < m_QueueUpdateScheduler.size(); i++)
    {
        if (m_QueueUpdateScheduler[i] == schedule_id)
        {
            found = true;
            break;
        }
    }
    if (!found)
        m_QueueUpdateScheduler.push_back(schedule_id);
}

uint32 BattlegroundMgr::GetMaxRatingDifference() const
{
    // this is for stupid people who can't use brain and set max rating difference to 0
    uint32 diff = sWorld->getIntConfig(CONFIG_ARENA_MAX_RATING_DIFFERENCE);
    if (diff == 0)
        diff = 5000;
    return diff;
}

uint32 BattlegroundMgr::GetRatingDiscardTimer() const
{
    return sWorld->getIntConfig(CONFIG_ARENA_RATING_DISCARD_TIMER);
}

uint32 BattlegroundMgr::GetPrematureFinishTime() const
{
    return sWorld->getIntConfig(CONFIG_BATTLEGROUND_PREMATURE_FINISH_TIMER);
}

HolidayIds BattlegroundMgr::BGTypeToWeekendHolidayId(BattlegroundTypeId bgTypeId)
{
    switch (bgTypeId)
    {
        case BATTLEGROUND_AV: return HOLIDAY_CALL_TO_ARMS_AV;
        case BATTLEGROUND_EY: return HOLIDAY_CALL_TO_ARMS_EY;
        case BATTLEGROUND_WS: return HOLIDAY_CALL_TO_ARMS_WS;
        case BATTLEGROUND_SA: return HOLIDAY_CALL_TO_ARMS_SA;
        case BATTLEGROUND_AB: return HOLIDAY_CALL_TO_ARMS_AB;
        case BATTLEGROUND_IC: return HOLIDAY_CALL_TO_ARMS_IC;
        case BATTLEGROUND_BG: return HOLIDAY_CALL_TO_ARMS_BG;
        case BATTLEGROUND_TP: return HOLIDAY_CALL_TO_ARMS_TP;
        default: return HOLIDAY_NONE;
    }
}

BattlegroundTypeId BattlegroundMgr::WeekendHolidayIdToBGType(HolidayIds holiday)
{
    switch (holiday)
    {
        case HOLIDAY_CALL_TO_ARMS_AV: return BATTLEGROUND_AV;
        case HOLIDAY_CALL_TO_ARMS_EY: return BATTLEGROUND_EY;
        case HOLIDAY_CALL_TO_ARMS_WS: return BATTLEGROUND_WS;
        case HOLIDAY_CALL_TO_ARMS_SA: return BATTLEGROUND_SA;
        case HOLIDAY_CALL_TO_ARMS_AB: return BATTLEGROUND_AB;
        case HOLIDAY_CALL_TO_ARMS_IC: return BATTLEGROUND_IC;
        case HOLIDAY_CALL_TO_ARMS_BG: return BATTLEGROUND_BG;
        case HOLIDAY_CALL_TO_ARMS_TP: return BATTLEGROUND_TP;
        default: return BATTLEGROUND_TYPE_NONE;
    }
}

bool BattlegroundMgr::IsBGWeekend(BattlegroundTypeId bgTypeId)
{
    return IsHolidayActive(BGTypeToWeekendHolidayId(bgTypeId));
}

void BattlegroundMgr::DoCompleteAchievement(uint32 achievement, Player * player)
{
    AchievementEntry const* AE = GetAchievementStore()->LookupEntry(achievement);

    if (!player)
    {
        //Map::PlayerList const &PlayerList = this->GetPlayers();
        //GroupsQueueType::iterator group = SelectedGroups.begin();

        //if (!PlayerList.isEmpty())
            //for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
        //    for (GroupsQueueType::iterator itr = group; itr != SelectedGroups.end(); ++itr)
        //        if (Player *pPlayer = itr->getSource())
        //            pPlayer->CompletedAchievement(AE);
    }
    else
    {
        player->CompletedAchievement(AE);
    }
}

void BattlegroundMgr::SetupWargame(Group* first, Group* second, BattlegroundTypeId bgTypeId)
{
    if (!first || !second || !first->GetLeader() || !second->GetLeader())
        return;

    // wargames must have equal sized groups
    if (first->GetMembersCount() != second->GetMembersCount())
        return;

    Player* initiator = first->GetLeader();
    Player* receiver = second->GetLeader();

    Battleground *bg = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId);

    uint8 arenaType = 0;

    switch (bgTypeId)
    {
        case BATTLEGROUND_AA:
        case BATTLEGROUND_NA:
        case BATTLEGROUND_RL:
        case BATTLEGROUND_BE:
        case BATTLEGROUND_DS:
        case BATTLEGROUND_RV:
        {
            arenaType = first->GetMembersCount();

            // only valid member count
            if (arenaType != 2 && arenaType != 3 && arenaType != 5)
                arenaType = 0;
            break;
        }
        default:
            break;
    }

    BattlegroundQueueTypeId bgQueueTypeId = BGQueueTypeId(bgTypeId, arenaType);

    if (!bg)
    {
        sLog->outError("Battleground template for BG Type ID %u not found!", (uint32)bgTypeId);
        return;
    }

    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(),initiator->getLevel());
    if (!bracketEntry)
    {
        sLog->outError("Unexpected level branch branch for player level %u",initiator->getLevel());
        return;
    }

    // check if the group is sized to fit the battleground they chose and so
    GroupJoinBattlegroundResult err1 = first->CanJoinBattlegroundQueue(bg, bgQueueTypeId, first->GetMembersCount(), bg->GetMaxPlayersPerTeam(), false, arenaType, true);
    GroupJoinBattlegroundResult err2 = second->CanJoinBattlegroundQueue(bg, bgQueueTypeId, first->GetMembersCount(), bg->GetMaxPlayersPerTeam(), false, arenaType, true);

    if (err1 != ERR_BATTLEGROUND_NONE || err2 != ERR_BATTLEGROUND_NONE)
    {
        // Notify initiator (first leader)
        WorldPacket data;
        sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, initiator, 0, err1);
        initiator->GetSession()->SendPacket(&data);

        // and also receiver (second leader)
        data.clear();
        sBattlegroundMgr->BuildStatusFailedPacket(&data, bg, receiver, 0, err1);
        receiver->GetSession()->SendPacket(&data);
        return;
    }

    BattlegroundQueue& bgQueue = sBattlegroundMgr->m_BattlegroundQueues[bgQueueTypeId][BATTLEGROUND_WARGAME];

    /*GroupQueueInfo* ginfo1 = bgQueue.AddGroup(initiator, first, bgTypeId, bracketEntry, arenaType, false, true, 0, 0, 0);
    GroupQueueInfo* ginfo2 = bgQueue.AddGroup(receiver, second, bgTypeId, bracketEntry, arenaType, false, true, 0, 0, 0);*/

    WargameQueueInfo* wginfo = bgQueue.AddWargameGroups(first, second, bgTypeId, arenaType);

    uint32 queueSlot;
    Player *member;

    for (GroupReference *itr = first->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        member = itr->getSource();
        if (!member) 
            continue;

        WorldPacket data;

        // add to queue
        queueSlot = member->AddBattlegroundQueueId(bgQueueTypeId, true);

        // add joined time data
        member->AddBattlegroundQueueJoinTime(bgTypeId, wginfo->JoinTime);

        // send status packet (in queue)
        sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, member, queueSlot, STATUS_WAIT_QUEUE, 0, wginfo->JoinTime, wginfo->ArenaType);
        member->GetSession()->SendPacket(&data);
    }
    for (GroupReference *itr = second->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        member = itr->getSource();
        if (!member) 
            continue;

        WorldPacket data;

        // add to queue
        queueSlot = member->AddBattlegroundQueueId(bgQueueTypeId, true);

        // add joined time data
        member->AddBattlegroundQueueJoinTime(bgTypeId, wginfo->JoinTime);

        // send status packet (in queue)
        sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, member, queueSlot, STATUS_WAIT_QUEUE, 0, wginfo->JoinTime, wginfo->ArenaType);
        member->GetSession()->SendPacket(&data);
    }

    sBattlegroundMgr->ScheduleQueueUpdate(0, arenaType, bgQueueTypeId, bgTypeId, bracketEntry->GetBracketId(), BATTLEGROUND_WARGAME);
}

void BattlegroundMgr::InitRatedBattlegrounds()
{
    // even Rated BGs are disabled, lets load it
    QueryResult result = CharacterDatabase.Query("SELECT week, next_week FROM rated_battleground");

    if (!result)
    {
        sLog->outString(">> DB table `rated_battleground` is empty!");
        sLog->outString();
        return;
    }

    Field* fields = result->Fetch();
    m_ratedBgWeek = fields[0].GetUInt32();
    m_ratedBgNextWeek = fields[1].GetUInt32();

    if (!sWorld->getBoolConfig(CONFIG_RATED_BATTLEGROUND_ENABLED))
        m_ratedBgWeek = RATED_BATTLEGROUND_WEEK_NONE;
}

void BattlegroundMgr::UpdateRatedBattlegroundWeek()
{
    uint32 possibleWeeks = sWorld->getIntConfig(CONFIG_RATED_BATTLEGROUND_WEEKS_IN_ROTATION);
    
    if (sWorld->getBoolConfig(CONFIG_RATED_BATTLEGROUND_ENABLED))
    {
        do
        {
            if (m_ratedBgWeek == RATED_BATTLEGROUND_WEEK_NONE || m_ratedBgWeek == RATED_BATTLEGROUND_WEEK_15v15)
                m_ratedBgWeek = RATED_BATTLEGROUND_WEEK_10v10;
            else if (m_ratedBgWeek == RATED_BATTLEGROUND_WEEK_10v10)
                m_ratedBgWeek = RATED_BATTLEGROUND_WEEK_15v15;
            else
                m_ratedBgWeek = RATED_BATTLEGROUND_WEEK_NONE;
        }
        while (!(possibleWeeks & m_ratedBgWeek));
    }
    else
        m_ratedBgWeek = RATED_BATTLEGROUND_WEEK_NONE;

    do
    {
        m_ratedBgNextWeek += 7*DAY;
    }
    while (time(NULL) > m_ratedBgNextWeek);

    CharacterDatabase.PExecute("UPDATE rated_battleground SET week = %u, next_week = %u", m_ratedBgWeek, m_ratedBgNextWeek);

    //TRINITY_READ_GUARD(HashMapHolder<Player>::LockType, *HashMapHolder<Player>::GetLock());
    HashMapHolder<Player>::MapType const& m = sObjectAccessor->GetPlayers();
    for (HashMapHolder<Player>::MapType::const_iterator itr = m.begin(); itr != m.end(); ++itr)
        itr->second->SendUpdateWorldState(RATED_BATTLEGROUND_WEEK_WORLDSTATE, m_ratedBgWeek);
}

BattlegroundTypeId BattlegroundMgr::GetRatedBattlegroundType()
{
    switch (m_ratedBgWeek)
    {
        case RATED_BATTLEGROUND_WEEK_10v10:
            return BATTLEGROUND_RA_BG_10;
        case RATED_BATTLEGROUND_WEEK_15v15:
            return BATTLEGROUND_RA_BG_15;
        case RATED_BATTLEGROUND_WEEK_NONE:
        default:
            return BATTLEGROUND_TYPE_NONE;
    }
}

uint32 BattlegroundMgr::GetRatedBattlegroundSize()
{
    switch (m_ratedBgWeek)
    {
        case RATED_BATTLEGROUND_WEEK_10v10:
            return 10;
        case RATED_BATTLEGROUND_WEEK_15v15:
            return 15;
        case RATED_BATTLEGROUND_WEEK_NONE:
        default:
            return 0;
    }
}
