/*
 * Copyright (C) 2005-2011 MaNGOS <http://www.getmangos.com/>
 *
 * Copyright (C) 2008-2011 Trinity <http://www.trinitycore.org/>
 *
 * Copyright (C) 2010-2011 Project SkyFire <http://www.projectskyfire.org/>
 *
 * Copyright (C) 2010-2011 G-Core, Project iCe Online <http://ice-wow.eu/>
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

#ifndef __BattlegroundBG_H
#define __BattlegroundBG_H

class Battleground;

enum BG_BG_WorldStates
{
    BG_BG_OP_OCCUPIED_BASES_HORDE       = 1778,
    BG_BG_OP_OCCUPIED_BASES_ALLY        = 1779,
    BG_BG_OP_RESOURCES_ALLY             = 1776,
    BG_BG_OP_RESOURCES_HORDE            = 1777,
    BG_BG_OP_RESOURCES_MAX              = 1780,
    BG_BG_OP_RESOURCES_WARNING          = 1955
};

const uint32 BG_BG_OP_NODESTATES[3] =    {1767, 1782, 1772};
const uint32 BG_BG_OP_NODEICONS[3]  =    {1842, 1846, 1845};

enum BG_BG_NodeObjectId
{
    BG_BG_OBJECTID_NODE_BANNER_0    = 180087,       // Lighthouse banner
    BG_BG_OBJECTID_NODE_BANNER_1    = 180088,       // Waterworks banner
    BG_BG_OBJECTID_NODE_BANNER_2    = 180089,       // Mine banner
    /*
    BG_BG_OBJECTID_NODE_BANNER_0    = 205557,       // Lighthouse banner
    BG_BG_OBJECTID_NODE_BANNER_1    = 208782,       // Mine banner
    BG_BG_OBJECTID_NODE_BANNER_2    = 208785,       // Watterworks banner
    */
};

// Difference between each node banner object entry
// its due to for loop for spawning base blank node banners
const uint32 NodeEntryNumDiff[3] = {0,1,2};
// We use banner IDs from Arathi Basin, original BfG banners and entry diffs commented out
//const uint32 NodeEntryNumDiff[3] = {0,3225,3228};

enum BG_BG_ObjectType
{
    // 8 objects for each base, 3 bases, 3*8 objects = 24
    BG_BG_OBJECT_BANNER_NEUTRAL          = 0,
    BG_BG_OBJECT_BANNER_CONT_A           = 1,
    BG_BG_OBJECT_BANNER_CONT_H           = 2,
    BG_BG_OBJECT_BANNER_ALLY             = 3,
    BG_BG_OBJECT_BANNER_HORDE            = 4,
    BG_BG_OBJECT_AURA_ALLY               = 5,
    BG_BG_OBJECT_AURA_HORDE              = 6,
    BG_BG_OBJECT_AURA_CONTESTED          = 7,

    // buffs
    BG_BG_OBJECT_SPEEDBUFF_LIGHTHOUSE    = 24,
    BG_BG_OBJECT_REGENBUFF_LIGHTHOUSE    = 25,
    BG_BG_OBJECT_BERSERKBUFF_LIGHTHOUSE  = 26,
    BG_BG_OBJECT_SPEEDBUFF_WATERWORKS    = 27,
    BG_BG_OBJECT_REGENBUFF_WATERWORKS    = 28,
    BG_BG_OBJECT_BERSERKBUFF_WATERWORKS  = 29,
    BG_BG_OBJECT_SPEEDBUFF_MINE          = 30,
    BG_BG_OBJECT_REGENBUFF_MINE          = 31,
    BG_BG_OBJECT_BERSERKBUFF_MINE        = 32,

    BG_BG_OBJECT_GATE_A                  = 33,
    BG_BG_OBJECT_GATE_H                  = 34,
    BG_BG_OBJECT_MAX                     = 35,
};

/* Object id templates from DB */
enum BG_BG_ObjectTypes
{
    BG_BG_OBJECTID_BANNER_A             = 180058,
    BG_BG_OBJECTID_BANNER_CONT_A        = 180059,
    BG_BG_OBJECTID_BANNER_H             = 180060,
    BG_BG_OBJECTID_BANNER_CONT_H        = 180061,
    /*
    BG_BG_OBJECTID_BANNER_A             = 208718,
    BG_BG_OBJECTID_BANNER_CONT_A        = 208724,
    BG_BG_OBJECTID_BANNER_H             = 208727,
    BG_BG_OBJECTID_BANNER_CONT_H        = 208754,
    */

    // Same as in AB
    BG_BG_OBJECTID_AURA_A               = 180100,
    BG_BG_OBJECTID_AURA_H               = 180101,
    BG_BG_OBJECTID_AURA_C               = 180102,

    BG_BG_OBJECTID_GATE_A               = 205496,
    BG_BG_OBJECTID_GATE_H               = 207178
};

enum BG_BG_Timers
{
    BG_BG_FLAG_CAPTURING_TIME   = 60000,
};

enum BG_BG_Score
{
    BG_BG_WARNING_NEAR_VICTORY_SCORE    = 1800,
    BG_BG_MAX_TEAM_SCORE                = 2000
};

/* do NOT change the order, else wrong behaviour */
enum BG_BG_BattlegroundNodes
{
    BG_BG_NODE_LIGHTHOUSE       = 0,
    BG_BG_NODE_WATERWORKS       = 1,
    BG_BG_NODE_MINE             = 2,

    BG_BG_DYNAMIC_NODES_COUNT   = 3,                        // dynamic nodes that can be captured

    BG_BG_SPIRIT_ALIANCE        = 3,
    BG_BG_SPIRIT_HORDE          = 4,

    BG_BG_ALL_NODES_COUNT       = 5,                        // all nodes (dynamic and static)
};

enum BG_BG_NodeStatus
{
    BG_BG_NODE_TYPE_NEUTRAL             = 0,
    BG_BG_NODE_TYPE_CONTESTED           = 1,
    BG_BG_NODE_STATUS_ALLY_CONTESTED    = 1,
    BG_BG_NODE_STATUS_HORDE_CONTESTED   = 2,
    BG_BG_NODE_TYPE_OCCUPIED            = 3,
    BG_BG_NODE_STATUS_ALLY_OCCUPIED     = 3,
    BG_BG_NODE_STATUS_HORDE_OCCUPIED    = 4
};

enum BG_BG_Sounds
{
    BG_BG_SOUND_NODE_CLAIMED            = 8192,
    BG_BG_SOUND_NODE_CAPTURED_ALLIANCE  = 8173,
    BG_BG_SOUND_NODE_CAPTURED_HORDE     = 8213,
    BG_BG_SOUND_NODE_ASSAULTED_ALLIANCE = 8212,
    BG_BG_SOUND_NODE_ASSAULTED_HORDE    = 8174,
    BG_BG_SOUND_NEAR_VICTORY            = 8456
};

// Old AB ones: 122 assault, 123 defend
enum BG_BG_Objectives
{
    BG_OBJECTIVE_ASSAULT_BASE           = 370,
    BG_OBJECTIVE_DEFEND_BASE            = 371,
};

// x, y, z, o
const float BG_BG_NodePositions[BG_BG_DYNAMIC_NODES_COUNT][4] = {
    {1057.805176f, 1278.395630f, 3.1747f, 1.907216f},                 // lighthouse
    {980.161499f, 948.649292f, 12.720972f, 5.903179f},                // waterworks
    {1251.07886f, 958.236f, 5.656674f, 2.826648f},                    // mine
};

// x, y, z, o, rot0, rot1, rot2, rot3
const float BG_BG_DoorPositions[2][8] = {
    {918.876f, 1336.56f, 27.6195f, 2.77481f, 0.0f, 0.0f, 0.983231f, 0.182367f},
    {1396.15f, 977.014f, 7.43169f, 6.27043f, 0.0f, 0.0f, 0.006378f, -0.99998f}
};

const uint32 BG_BG_TickIntervals[4] = {0, 12000, 6000, 1000};
const uint32 BG_BG_TickPoints[4]    = {0, 10, 10, 30};

// WorldSafeLocs ids for 3 nodes, and for ally, and horde starting location
const uint32 BG_BG_GraveyardIds[BG_BG_ALL_NODES_COUNT] = {1736, 1738, 1735, 1740, 1739};

// x, y, z, o
const float BG_BG_BuffPositions[BG_BG_DYNAMIC_NODES_COUNT][4] = {
    {1097.806885f, 1293.072876f, 6.328900f, 3.776195f},               // lighthouse
    {990.798767f, 984.346558f, 13.008142f, 3.988254f},                // waterworks
    {1205.644409f, 1014.332764f, 6.314735f, 5.756184f},               // mine
};

// x, y, z, o
const float BG_BG_SpiritGuidePos[BG_BG_ALL_NODES_COUNT][4] = {
    {1036.422607f, 1341.375854f, 11.536129f, 4.885175f},              // lighthouse
    {885.384888f, 936.318176f, 24.379227f, 0.490875f},                // waterworks
    {1252.441895f, 831.479004f, 27.789499f, 1.582577f},               // mine
    {911.930969f, 1345.247681f, 27.939629f, 4.05266f},                // alliance starting base
    {1399.523804f, 971.404175f, 7.441012f, 1.083854f}                 // horde starting base
};

struct BG_BG_BannerTimer
{
    uint32      timer;
    uint8       type;
    uint8       teamIndex;
};

#define BG_BG_NotBGBGWeekendHonorTicks      330
#define BG_BG_BGBGWeekendHonorTicks         200
#define BG_BG_NotBGBGWeekendReputationTicks 200
#define BG_BG_BGBGWeekendReputationTicks    150

class BattlegroundBGScore : public BattlegroundScore
{
    public:
        BattlegroundBGScore(): BasesAssaulted(0), BasesDefended(0) {};
        virtual ~BattlegroundBGScore() {};
        uint32 BasesAssaulted;
        uint32 BasesDefended;
};

class BattlegroundBG : public Battleground
{
    friend class BattlegroundMgr;

    public:
        BattlegroundBG();
        ~BattlegroundBG();
        void Update(uint32 diff);

        /* inherited from BattlegroundClass */
        virtual void AddPlayer(Player *plr);
        virtual void StartingEventCloseDoors();
        virtual void StartingEventOpenDoors();

        void RemovePlayer(Player *plr, uint64 guid);
        void HandleAreaTrigger(Player *Source, uint32 Trigger);
        bool SetupBattleground();
        void EndBattleground(uint32 winner);
        virtual void Reset();
        virtual WorldSafeLocsEntry const* GetClosestGraveYard(Player* player);

        /* Scorekeeping */
        void UpdatePlayerScore(Player *Source, uint32 type, uint32 value, bool doAddHonor = true);

        virtual void FillInitialWorldStates(WorldPacket& data);

        /* Nodes occupying */
        virtual void EventPlayerClickedOnFlag(Player *source, GameObject* target_obj);

    private:
        /* Gameobject spawning/despawning */
        void _CreateBanner(uint8 node, uint8 type, uint8 teamIndex, bool delay);
        void _DelBanner(uint8 node, uint8 type, uint8 teamIndex);
        void _SendNodeUpdate(uint8 node);

        /* Creature spawning/despawning */
        void _NodeOccupied(uint8 node,Team team);
        void _NodeDeOccupied(uint8 node);

        int32 _GetNodeNameId(uint8 node);

        /* Nodes info:
            0: neutral
            1: ally contested
            2: horde contested
            3: ally occupied
            4: horde occupied     */
        uint8               m_Nodes[BG_BG_DYNAMIC_NODES_COUNT];
        uint8               m_prevNodes[BG_BG_DYNAMIC_NODES_COUNT];
        BG_BG_BannerTimer   m_BannerTimers[BG_BG_DYNAMIC_NODES_COUNT];
        uint32              m_NodeTimers[BG_BG_DYNAMIC_NODES_COUNT];
        uint32              m_lastTick[BG_TEAMS_COUNT];
        uint32              m_HonorScoreTics[BG_TEAMS_COUNT];
        bool                m_IsInformedNearVictory;
        uint32              m_HonorTics;
};
#endif
