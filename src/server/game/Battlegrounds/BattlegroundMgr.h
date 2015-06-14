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

#ifndef __BATTLEGROUNDMGR_H
#define __BATTLEGROUNDMGR_H

#include "Common.h"
#include "DBCEnums.h"
#include "Battleground.h"
#include "BattlegroundQueue.h"
#include "MersenneTwister.h"
#include <ace/Singleton.h>

typedef std::map<uint32, Battleground*> BattlegroundSet;
typedef std::set<uint32> BattlegroundClientIdsContainer;

typedef std::unordered_map<uint32, BattlegroundTypeId> BattleMastersMap;

#define BATTLEGROUND_ARENA_POINT_DISTRIBUTION_DAY 86400     // seconds in a day
#define WS_ARENA_DISTRIBUTION_TIME 20001                    // Custom worldstate

class BattlegroundMgr
{
    /// Todo: Thread safety?
    /* Construction */
    friend class ACE_Singleton<BattlegroundMgr, ACE_Null_Mutex>;
    BattlegroundMgr();

    public:
        ~BattlegroundMgr();
        void Update(uint32 diff);

        /* Packet Building */
        void BuildPlayerJoinedBattlegroundPacket(WorldPacket *data, Player *plr);
        void BuildPlayerLeftBattlegroundPacket(WorldPacket *data, const uint64& guid);
        void BuildBattlegroundListPacket(WorldPacket *data, const uint64& guid, Player *plr, BattlegroundTypeId bgtypeid);
        void BuildStatusFailedPacket(WorldPacket *data, Battleground *bg, Player *pPlayer, uint8 QueueSlot, GroupJoinBattlegroundResult result);
        void BuildUpdateWorldStatePacket(WorldPacket *data, uint32 field, uint32 value);
        void BuildPvpLogDataPacket(WorldPacket *data, Battleground *bg, Player *receiver);
        void BuildBattlegroundStatusPacket(WorldPacket *data, Battleground *bg, Player *player, uint8 QueueSlot, uint8 StatusID, uint32 Time1, uint32 Time2, uint8 arenatype, uint8 uiFrame = 1);
        void BuildPlaySoundPacket(WorldPacket *data, uint32 soundid);
        void SendAreaSpiritHealerQueryOpcode(Player *pl, Battleground *bg, const uint64& guid);

        /* Battlegrounds */
        void Update3v3ArenaInfo();
        Battleground* GetBattlegroundThroughClientInstance(uint32 instanceId, BattlegroundTypeId bgTypeId);
        Battleground* GetBattleground(uint32 InstanceID, BattlegroundTypeId bgTypeId); //there must be uint32 because MAX_BATTLEGROUND_TYPE_ID means unknown

        Battleground* GetBattlegroundTemplate(BattlegroundTypeId bgTypeId);
        Battleground* CreateNewBattleground(BattlegroundTypeId bgTypeId, PvPDifficultyEntry const* bracketEntry, uint8 arenaType, bool isRated, bool isWargame = false);

        uint32 CreateBattleground(BattlegroundTypeId bgTypeId, bool IsArena, uint32 MinPlayersPerTeam, uint32 MaxPlayersPerTeam, uint32 LevelMin, uint32 LevelMax, const char* BattlegroundName, uint32 MapID, float Team1StartLocX, float Team1StartLocY, float Team1StartLocZ, float Team1StartLocO, float Team2StartLocX, float Team2StartLocY, float Team2StartLocZ, float Team2StartLocO, uint32 scriptId);

        void AddBattleground(uint32 InstanceID, BattlegroundTypeId bgTypeId, Battleground* BG) { m_Battlegrounds[bgTypeId][InstanceID] = BG; };
        void RemoveBattleground(uint32 instanceID, BattlegroundTypeId bgTypeId) { m_Battlegrounds[bgTypeId].erase(instanceID); }
        uint32 CreateClientVisibleInstanceId(BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket_id);

        void GetSpectatableArenas(BattlegroundSet *target);
        bool AddArenaSpectator(Player* pl, uint32 instanceId);
        void RemoveArenaSpectator(Player* pl, bool teleport = true);

        void CreateInitialBattlegrounds();
        void DeleteAllBattlegrounds();

        void SendToBattleground(Player *pl, uint32 InstanceID, BattlegroundTypeId bgTypeId);

        void SetupWargame(Group* first, Group* second, BattlegroundTypeId bgTypeId);

        /* Battleground queues */
        //these queues are instantiated when creating BattlegroundMrg
        BattlegroundQueue m_BattlegroundQueues[MAX_BATTLEGROUND_QUEUE_TYPES][BATTLEGROUND_TWINK_TYPES]; // public, because we need to access them in BG handler code

        BGFreeSlotQueueType BGFreeSlotQueue[MAX_BATTLEGROUND_TYPE_ID][BATTLEGROUND_TWINK_TYPES];

        void ScheduleQueueUpdate(uint32 arenaMatchmakerRating, uint8 arenaType, BattlegroundQueueTypeId bgQueueTypeId, BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket_id, uint8 twink);
        uint32 GetMaxRatingDifference() const;
        uint32 GetRatingDiscardTimer()  const;
        uint32 GetPrematureFinishTime() const;

        static uint32 GetHighestArenaRating(uint32 lowguid);
        static uint32 CalculateArenaCap(uint32 rating);
        static uint32 CalculateRatedBattlegroundCap(uint32 rating);
        void InitAutomaticArenaPointDistribution();
        void DistributeArenaCurrency();
        void UpdateRatedBattlegroundCap();
        void ToggleArenaTesting();
        void ToggleTesting();

        void SetHolidayWeekends(uint32 mask);

        bool isArenaTesting() const { return m_ArenaTesting; }
        bool isTesting() const { return m_Testing; }

        void InitRatedBattlegrounds();
        uint32 GetRatedBattlegroundWeek() const { return m_ratedBgWeek; }
        void UpdateRatedBattlegroundWeek();
        BattlegroundTypeId GetRatedBattlegroundType();
        uint32 GetRatedBattlegroundSize();

        static bool IsArenaType(BattlegroundTypeId bgTypeId);
        static bool IsBattlegroundType(BattlegroundTypeId bgTypeId) { return !BattlegroundMgr::IsArenaType(bgTypeId); }
        static BattlegroundQueueTypeId BGQueueTypeId(BattlegroundTypeId bgTypeId, uint8 arenaType);
        static BattlegroundTypeId BGTemplateId(BattlegroundQueueTypeId bgQueueTypeId);
        static uint8 BGArenaType(BattlegroundQueueTypeId bgQueueTypeId);

        static HolidayIds BGTypeToWeekendHolidayId(BattlegroundTypeId bgTypeId);
        static BattlegroundTypeId WeekendHolidayIdToBGType(HolidayIds holiday);
        static bool IsBGWeekend(BattlegroundTypeId bgTypeId);
        void DoCompleteAchievement(uint32 achievement, Player * player = NULL);
private:
        BattlegroundTypeId SelectRandomBattleground(uint32 minBracketLevel);
        BattlegroundTypeId SelectRandomBattleground(std::vector<BattlegroundTypeId> &availBGs);

private:

        typedef std::map<BattlegroundTypeId, uint8> BattlegroundSelectionWeightMap; // TypeId and its selectionWeight
        /* Battlegrounds */
        BattlegroundSet m_Battlegrounds[MAX_BATTLEGROUND_TYPE_ID];
        BattlegroundSelectionWeightMap m_ArenaSelectionWeights;
        BattlegroundSelectionWeightMap m_BGSelectionWeights;
        std::vector<uint64> m_QueueUpdateScheduler;
        BattlegroundClientIdsContainer m_ClientBattlegroundIds[MAX_BATTLEGROUND_TYPE_ID][MAX_BATTLEGROUND_BRACKETS]; //the instanceids just visible for the client
        uint32 m_NextRatingDiscardUpdate;
        time_t m_NextAutoDistributionTime;
        uint32 m_AutoDistributionTimeChecker;
        bool   m_ArenaTesting;
        bool   m_Testing;

        uint32 m_ratedBgWeek;
        uint32 m_ratedBgNextWeek;
        uint32 m_ratedBgWeekCheckTimer;

        // random number generators
        MTRand m_ArenaGenerator, m_RandomBGGenerator;
};

#define sBattlegroundMgr ACE_Singleton<BattlegroundMgr, ACE_Null_Mutex>::instance()
#endif

