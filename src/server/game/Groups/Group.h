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

#ifndef TRINITYCORE_GROUP_H
#define TRINITYCORE_GROUP_H

#include "GroupReference.h"
#include "GroupRefManager.h"
#include "DBCEnums.h"
#include "Battleground.h"
#include "../../Battlefield/BattlefieldMgr.h"

#include <map>
#include <vector>

#define MAXGROUPSIZE 5
#define MAXRAIDSIZE 40
#define MAX_RAID_SUBGROUPS MAXRAIDSIZE/MAXGROUPSIZE
#define TARGETICONCOUNT 8

class InstanceSave;
class Player;
class Unit;
class WorldSession;

enum RollVote
{
    PASS              = 0,
    NEED              = 1,
    GREED             = 2,
    DISENCHANT        = 3,
    NOT_EMITED_YET    = 4,
    NOT_VALID         = 5
};

enum RaidMarkers
{
    MARKER_CYAN     = 0,
    MARKER_GREEN    = 1,
    MARKER_PURPLE   = 2,
    MARKER_RED      = 3,
    MARKER_YELLOW   = 4,
    MARKER_MAX      = 5
};

#define MARKER_KIT_START 19455
#define RAID_MARKER_KIT(a) ((a-MARKER_CYAN)+MARKER_KIT_START)

enum GroupMemberOnlineStatus
{
    MEMBER_STATUS_OFFLINE   = 0x0000,
    MEMBER_STATUS_ONLINE    = 0x0001,
    MEMBER_STATUS_PVP       = 0x0002,
    MEMBER_STATUS_UNK0      = 0x0004,                       // dead? (health=0)
    MEMBER_STATUS_UNK1      = 0x0008,                       // ghost? (health=1)
    MEMBER_STATUS_UNK2      = 0x0010,                       // never seen
    MEMBER_STATUS_UNK3      = 0x0020,                       // never seen
    MEMBER_STATUS_UNK4      = 0x0040,                       // appears with dead and ghost flags
    MEMBER_STATUS_UNK5      = 0x0080,                       // never seen
};

enum GroupMemberFlags
{
    MEMBER_FLAG_ASSISTANT   = 0x01,
    MEMBER_FLAG_MAINTANK    = 0x02,
    MEMBER_FLAG_MAINASSIST  = 0x04,
};

enum GroupType
{
    GROUPTYPE_NORMAL = 0x00,
    GROUPTYPE_BG     = 0x01,
    GROUPTYPE_RAID   = 0x02,
    GROUPTYPE_BGRAID = GROUPTYPE_BG | GROUPTYPE_RAID,       // mask
    GROUPTYPE_UNK1   = 0x04,
    GROUPTYPE_LFG    = 0x08,
    // 0x10, leave/change group?, I saw this flag when leaving group and after leaving BG while in group
};

enum GroupUpdateFlags
{
    GROUP_UPDATE_FLAG_NONE              = 0x00000000,       // nothing
    GROUP_UPDATE_FLAG_STATUS            = 0x00000001,       // uint16 (GroupMemberStatusFlag)
    GROUP_UPDATE_FLAG_CUR_HP            = 0x00000002,       // uint32 (HP)
    GROUP_UPDATE_FLAG_MAX_HP            = 0x00000004,       // uint32 (HP)
    GROUP_UPDATE_FLAG_POWER_TYPE        = 0x00000008,       // uint8 (PowerType)
    GROUP_UPDATE_FLAG_CUR_POWER         = 0x00000010,       // int16 (power value)
    GROUP_UPDATE_FLAG_MAX_POWER         = 0x00000020,       // int16 (power value)
    GROUP_UPDATE_FLAG_LEVEL             = 0x00000040,       // uint16 (level value)
    GROUP_UPDATE_FLAG_ZONE              = 0x00000080,       // uint16 (zone id)
    GROUP_UPDATE_FLAG_UNK100            = 0x00000100,       // int16 (unk)
    GROUP_UPDATE_FLAG_POSITION          = 0x00000200,       // uint16 (x), uint16 (y), uint16 (z)
    GROUP_UPDATE_FLAG_AURAS             = 0x00000400,       // uint8 (unk), uint64 (mask), uint32 (count), for each bit set: uint32 (spell id) + uint16 (AuraFlags)  (if has flags Scalable -> 3x int32 (bps))
    GROUP_UPDATE_FLAG_PET_GUID          = 0x00000800,       // uint64 (pet guid)
    GROUP_UPDATE_FLAG_PET_NAME          = 0x00001000,       // cstring (name, NULL terminated string)
    GROUP_UPDATE_FLAG_PET_MODEL_ID      = 0x00002000,       // uint16 (model id)
    GROUP_UPDATE_FLAG_PET_CUR_HP        = 0x00004000,       // uint32 (HP)
    GROUP_UPDATE_FLAG_PET_MAX_HP        = 0x00008000,       // uint32 (HP)
    GROUP_UPDATE_FLAG_PET_POWER_TYPE    = 0x00010000,       // uint8 (PowerType)
    GROUP_UPDATE_FLAG_PET_CUR_POWER     = 0x00020000,       // uint16 (power value)
    GROUP_UPDATE_FLAG_PET_MAX_POWER     = 0x00040000,       // uint16 (power value)
    GROUP_UPDATE_FLAG_PET_AURAS         = 0x00080000,       // [see GROUP_UPDATE_FLAG_AURAS]
    GROUP_UPDATE_FLAG_VEHICLE_SEAT      = 0x00100000,       // int32 (vehicle seat id)
    GROUP_UPDATE_FLAG_PHASE             = 0x00200000,       // int32 (unk), uint32 (phase count), for (count) uint16(phaseId)

    GROUP_UPDATE_PET = GROUP_UPDATE_FLAG_PET_GUID | GROUP_UPDATE_FLAG_PET_NAME | GROUP_UPDATE_FLAG_PET_MODEL_ID |
                       GROUP_UPDATE_FLAG_PET_CUR_HP | GROUP_UPDATE_FLAG_PET_MAX_HP | GROUP_UPDATE_FLAG_PET_POWER_TYPE |
                       GROUP_UPDATE_FLAG_PET_CUR_POWER | GROUP_UPDATE_FLAG_PET_MAX_POWER | GROUP_UPDATE_FLAG_PET_AURAS, // all pet flags
    GROUP_UPDATE_FULL = GROUP_UPDATE_FLAG_STATUS | GROUP_UPDATE_FLAG_CUR_HP | GROUP_UPDATE_FLAG_MAX_HP |
                        GROUP_UPDATE_FLAG_POWER_TYPE | GROUP_UPDATE_FLAG_CUR_POWER | GROUP_UPDATE_FLAG_MAX_POWER |
                        GROUP_UPDATE_FLAG_LEVEL | GROUP_UPDATE_FLAG_ZONE | GROUP_UPDATE_FLAG_POSITION |
                        GROUP_UPDATE_FLAG_AURAS | GROUP_UPDATE_PET | GROUP_UPDATE_FLAG_PHASE // all known flags, except UNK100 and VEHICLE_SEAT
};

enum RemoveMethod
{
    GROUP_REMOVEMETHOD_DEFAULT = 0,
    GROUP_REMOVEMETHOD_KICK    = 1,
    GROUP_REMOVEMETHOD_LEAVE   = 2,
};

enum GroupGuildProfit
{
    GROUP_MEMBERS_DUNGEON_PROFIT         = 3,
    GROUP_MEMBERS_DUNGEON_PROFIT_LOW_MIN = 3,
    GROUP_MEMBERS_DUNGEON_PROFIT_LOW_MAX = 3,
    GROUP_MEMBERS_DUNGEON_PROFIT_MED_MIN = 4,
    GROUP_MEMBERS_DUNGEON_PROFIT_MED_MAX = 4,
    GROUP_MEMBERS_DUNGEON_PROFIT_HIGH_MIN= 5,
    GROUP_MEMBERS_DUNGEON_PROFIT_HIGH_MAX= 5,
    GROUP_MEMBERS_10MAN_PROFIT           = 8,
    GROUP_MEMBERS_10MAN_PROFIT_LOW_MIN   = 8,
    GROUP_MEMBERS_10MAN_PROFIT_LOW_MAX   = 8,
    GROUP_MEMBERS_10MAN_PROFIT_MED_MIN   = 9,
    GROUP_MEMBERS_10MAN_PROFIT_MED_MAX   = 9,
    GROUP_MEMBERS_10MAN_PROFIT_HIGH_MIN  = 10,
    GROUP_MEMBERS_10MAN_PROFIT_HIGH_MAX  = 10,
    GROUP_MEMBERS_25MAN_PROFIT           = 20,
    GROUP_MEMBERS_25MAN_PROFIT_LOW_MIN   = 20,
    GROUP_MEMBERS_25MAN_PROFIT_LOW_MAX   = 21,
    GROUP_MEMBERS_25MAN_PROFIT_MED_MIN   = 22,
    GROUP_MEMBERS_25MAN_PROFIT_MED_MAX   = 23,
    GROUP_MEMBERS_25MAN_PROFIT_HIGH_MIN  = 24,
    GROUP_MEMBERS_25MAN_PROFIT_HIGH_MAX  = 25,
};

#define GROUP_GUILD_PROFIT_LOW   0.50f
#define GROUP_GUILD_PROFIT_MED   1.00f
#define GROUP_GUILD_PROFIT_HIGH  1.25f

#define GROUP_UPDATE_FLAGS_COUNT          20
                                                                // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,11,12,13,14,15,16,17,18,19
static const uint8 GroupUpdateLength[GROUP_UPDATE_FLAGS_COUNT] = { 0, 2, 2, 2, 1, 2, 2, 2, 2, 4, 8, 8, 1, 2, 2, 2, 1, 2, 2, 8};

class Roll : public LootValidatorRef
{
    public:
        Roll(uint64 _guid, LootItem const& li)
            : itemGUID(_guid), itemid(li.itemid), itemRandomPropId(li.randomPropertyId), itemRandomSuffix(li.randomSuffix), itemCount(li.count),
            totalPlayersRolling(0), totalNeed(0), totalGreed(0), totalPass(0), itemSlot(0), rollVoteMask(ROLL_ALL_TYPE_NO_DISENCHANT) {}
        ~Roll() { }
        void setLoot(Loot *pLoot) { link(pLoot, this); }
        Loot *getLoot() { return getTarget(); }
        void targetObjectBuildLink();

        uint64 itemGUID;
        uint32 itemid;
        int32  itemRandomPropId;
        uint32 itemRandomSuffix;
        uint8 itemCount;
        typedef std::map<uint64, RollVote> PlayerVote;
        PlayerVote playerVote;                              //vote position correspond with player position (in group)
        uint8 totalPlayersRolling;
        uint8 totalNeed;
        uint8 totalGreed;
        uint8 totalPass;
        uint8 itemSlot;
        uint8 rollVoteMask;
};

struct InstanceGroupBind
{
    InstanceSave *save;
    bool perm;
    /* permanent InstanceGroupBinds exist if the leader has a permanent
       PlayerInstanceBind for the same instance. */
    InstanceGroupBind() : save(NULL), perm(false) {}
};

/** request member stats checken **/
/** todo: uninvite people that not accepted invite **/
class Group
{
    public:
        struct MemberSlot
        {
            uint64      guid;
            std::string name;
            uint8       group;
            uint8       flags;
            uint8       roles;
        };
        typedef std::list<MemberSlot> MemberSlotList;
        typedef MemberSlotList::const_iterator member_citerator;

        typedef std::unordered_map< uint32 /*mapId*/, InstanceGroupBind> BoundInstancesMap;
    protected:
        typedef MemberSlotList::iterator member_witerator;
        typedef std::set<Player*> InvitesList;

        typedef std::vector<Roll*> Rolls;

    public:
        Group();
        ~Group();

        // group manipulation methods
        bool   Create(const uint64 &guid, const char * name);
        bool   LoadGroupFromDB(const uint32 &guid, QueryResult result, bool loadMembers = true);
        bool   LoadMemberFromDB(uint32 guidLow, uint8 memberFlags, uint8 subgroup, uint8 roles);
        bool   AddInvite(Player *player);
        void   RemoveInvite(Player *player);
        void   RemoveAllInvites();
        bool   AddLeaderInvite(Player *player);
        bool   AddMember(const uint64 &guid, const char* name);
        uint32 RemoveMember(const uint64 &guid, const RemoveMethod &method = GROUP_REMOVEMETHOD_DEFAULT, uint64 kicker = 0, const char* reason = NULL);
        void   ChangeLeader(const uint64 &guid);
        void   SetLootMethod(LootMethod method) { m_lootMethod = method; }
        void   SetLooterGuid(const uint64 &guid) { m_looterGuid = guid; }
        void   UpdateLooterGuid(WorldObject* pLootedObject, bool ifneed = false);
        void   SetLootThreshold(ItemQualities threshold) { m_lootThreshold = threshold; }
        void   Disband(bool hideDestroy=false);
        uint32 GetGuildMembersCount(uint32 guildId);
        bool   IsGuildGroup(uint32 guildId);
        float  GetGuildProfitCoef(uint32 guildId);

        void   OnGroupSlain(Unit* pVictim);

        // Dungeon Finder
        void   SetLfgRoles(uint64& guid, const uint8 roles)
        {
            member_witerator slot = _getMemberWSlot(guid);
            if (slot == m_memberSlots.end())
                return;

            slot->roles = roles;
            SendUpdate();
        }
        void   SetRoles(uint64 guid, const uint8 roles)
        {
            member_witerator slot = _getMemberWSlot(guid);
            if (slot == m_memberSlots.end())
                return;

            slot->roles = roles;
            SendUpdate();
        }  
        uint8  GetRoles(uint64 guid)
        {
            member_witerator slot = _getMemberWSlot(guid);
            if (slot == m_memberSlots.end())
                return 0;

            return slot->roles;
        }

        // properties accessories
        bool IsFull() const { return isRaidGroup() ? (m_memberSlots.size() >= MAXRAIDSIZE) : (m_memberSlots.size() >= MAXGROUPSIZE); }
        bool isLFGGroup()  const { return m_groupType & GROUPTYPE_LFG; }
        bool isRaidGroup() const { return m_groupType & GROUPTYPE_RAID; }
        bool isBGGroup()   const { return m_bgGroup != NULL; }
        bool isBFGroup()   const; 
        bool IsCreated()   const { return GetMembersCount() > 0; }
        const uint64& GetLeaderGUID() const { return m_leaderGuid; }
        const uint64& GetGUID() const { return m_guid; }
        Player *GetLeader();
        uint32 GetLowGUID() const { return GUID_LOPART(m_guid); }
        const char * GetLeaderName() const { return m_leaderName.c_str(); }
        LootMethod    GetLootMethod() const { return m_lootMethod; }
        const uint64& GetLooterGuid() const { return m_looterGuid; }
        ItemQualities GetLootThreshold() const { return m_lootThreshold; }

        // member manipulation methods
        bool IsMember(const uint64& guid) const { return _getMemberCSlot(guid) != m_memberSlots.end(); }
        bool IsLeader(const uint64& guid) const { return (GetLeaderGUID() == guid); }
        uint64 GetMemberGUID(const std::string& name)
        {
            for (member_citerator itr = m_memberSlots.begin(); itr != m_memberSlots.end(); ++itr)
            {
                if (itr->name == name)
                {
                    return itr->guid;
                }
            }
            return 0;
        }
        bool IsAssistant(uint64 guid) const
        {
            member_citerator mslot = _getMemberCSlot(guid);
            if (mslot == m_memberSlots.end())
                return false;

            return mslot->flags & MEMBER_FLAG_ASSISTANT;
        }
        Player* GetInvited(const uint64& guid) const;
        Player* GetInvited(const std::string& name) const;

        bool SameSubGroup(uint64 guid1,const uint64& guid2) const
        {
            member_citerator mslot2 = _getMemberCSlot(guid2);
            if (mslot2 == m_memberSlots.end())
                return false;

            return SameSubGroup(guid1,&*mslot2);
        }

        bool SameSubGroup(uint64 guid1, MemberSlot const* slot2) const
        {
            member_citerator mslot1 = _getMemberCSlot(guid1);
            if (mslot1 == m_memberSlots.end() || !slot2)
                return false;

            return (mslot1->group == slot2->group);
        }

        bool HasFreeSlotSubGroup(uint8 subgroup) const
        {
            return (m_subGroupsCounts && m_subGroupsCounts[subgroup] < MAXGROUPSIZE);
        }

        bool SameSubGroup(Player const* member1, Player const* member2) const;

        MemberSlotList const& GetMemberSlots() const { return m_memberSlots; }
        GroupReference* GetFirstMember() { return m_memberMgr.getFirst(); }
        uint32 GetMembersCount() const { return m_memberSlots.size(); }
        void GetDataForXPAtKill(Unit const* victim, uint32& count,uint32& sum_level, Player* & member_with_max_level, Player* & not_gray_member_with_max_level);
        uint8  GetMemberGroup(uint64 guid) const
        {
            member_citerator mslot = _getMemberCSlot(guid);
            if (mslot == m_memberSlots.end())
                return (MAX_RAID_SUBGROUPS+1);

            return mslot->group;
        }

        void ConvertToLFG();
        void ConvertToRaid();
        void ConvertToGroup();
        
        // some additional raid methods
        void SetBattlegroundGroup(Battleground *bg) { m_bgGroup = bg; }
        void SetBattlefieldGroup(Battlefield* bf);
        GroupJoinBattlegroundResult CanJoinBattlegroundQueue(Battleground const* bgOrTemplate, BattlegroundQueueTypeId bgQueueTypeId, uint32 MinPlayerCount, uint32 MaxPlayerCount, bool isRated, uint32 arenaSlot, bool isWarGame = false);

        void ChangeMembersGroup(const uint64 &guid, const uint8 &group);
        void ChangeMembersGroup(Player *player, const uint8 &group);

        void SetAssistant(uint64 guid, const bool &apply)
        {
            if (!isRaidGroup())
                return;
            if (_setAssistantFlag(guid, apply))
                SendUpdate();
        }
        void SetMainTank(uint64 guid, const bool &apply)
        {
            if (!isRaidGroup())
                return;

            if (_setMainTank(guid, apply))
                SendUpdate();
        }
        void SetMainAssistant(uint64 guid, const bool &apply)
        {
            if (!isRaidGroup())
                return;

            if (_setMainAssistant(guid, apply))
                SendUpdate();
        }

        void SetTargetIcon(uint8 id, uint64 whoGuid, uint64 targetGuid);

        Difficulty GetDifficulty(bool isRaid) const { return isRaid ? m_raidDifficulty : m_dungeonDifficulty; }
        Difficulty GetDungeonDifficulty() const { return m_dungeonDifficulty; }
        Difficulty GetRaidDifficulty() const { return m_raidDifficulty; }
        void SetDungeonDifficulty(Difficulty difficulty);
        void SetRaidDifficulty(Difficulty difficulty);
        uint16 InInstance();
        bool InCombatToInstance(uint32 instanceId);
        void ResetInstances(uint8 method, bool isRaid, Player* SendMsgTo);

        // -no description-
        //void SendInit(WorldSession *session);
        void SendTargetIconList(WorldSession *session);
        void SendUpdate();
        void UpdatePlayerOutOfRange(Player* pPlayer);
                                                            // ignore: GUID of player that will be ignored
        void BroadcastPacket(WorldPacket *packet, bool ignorePlayersInBGRaid, int group=-1, uint64 ignore=0);
        void BroadcastReadyCheck(WorldPacket *packet);
        void OfflineReadyCheck();

        /*********************************************************/
        /***                   LOOT SYSTEM                     ***/
        /*********************************************************/

        bool isRollLootActive() const { return !RollId.empty(); }
        void SendLootStartRoll(uint32 CountDown, uint32 mapid, const Roll &r);
        void SendLootRoll(const uint64& SourceGuid, const uint64& TargetGuid, uint32 RollNumber, uint8 RollType, const Roll &r);
        void SendLootRollWon(const uint64& SourceGuid, const uint64& TargetGuid, uint8 RollNumber, uint8 RollType, const Roll &r);
        void SendLootAllPassed(uint32 NumberOfPlayers, const Roll &r);
        void SendLooter(Creature *pCreature, Player *pLooter);
        void GroupLoot(Loot *loot, WorldObject* pLootedObject);
        void NeedBeforeGreed(Loot *loot, WorldObject* pLootedObject);
        void MasterLoot(Loot *loot, WorldObject* pLootedObject);
        Rolls::iterator GetRoll(uint64 Guid)
        {
            Rolls::iterator iter;
            for (iter=RollId.begin(); iter != RollId.end(); ++iter)
            {
                if ((*iter)->itemGUID == Guid && (*iter)->isValid())
                {
                    return iter;
                }
            }
            return RollId.end();
        }
        void CountTheRoll(Rolls::iterator roll, uint32 NumberOfPlayers);
        void CountRollVote(const uint64& playerGUID, const uint64& Guid, uint32 NumberOfPlayers, uint8 Choise);
        void EndRoll(Loot *loot);

        // related to disenchant rolls
        void ResetMaxEnchantingLevel();

        void LinkMember(GroupReference *pRef) { m_memberMgr.insertFirst(pRef); }
        void DelinkMember(GroupReference* /*pRef*/) { }

        InstanceGroupBind* BindToInstance(InstanceSave *save, bool permanent, bool load = false);
        InstanceGroupBind* BindToInstanceRaid(uint32 instanceId, uint32 mapId);
        void UnbindInstance(uint32 mapid, uint8 difficulty, bool unload = false);
        InstanceGroupBind* GetBoundInstance(Player* player);
        InstanceGroupBind* GetBoundInstance(Map* aMap);
        InstanceGroupBind* GetBoundInstance(MapEntry const* mapEntry);
        BoundInstancesMap& GetBoundInstances(Difficulty difficulty) { return m_boundInstances[difficulty]; }

        // FG: evil hacks
        void BroadcastGroupUpdate(void);

        uint32 GetAverageBattlegroundRating();
        uint8 GetAverageLevel();

        // raid markers
        void BuildMarkerVisualPacket(WorldPacket *data, uint64 source, RaidMarkers pos);
        void RefreshMarkerBySpellIdToPlayer(Player * pl,uint32 spellId);
        void RefreshMarkerBySpellIdToGroup(uint32 spellId);
        void RefreshAllMarkersTo(Player *pl);
        void RemoveAllMarkers(uint64 leaderGUID);
        void RemoveMarkerBySpellId(Player * player,uint32 spellId);

    protected:
        bool _addMember(const uint64 &guid, const char* name);
        bool _addMember(const uint64 &guid, const char* name, uint8 group);
        bool _removeMember(const uint64 &guid);             // returns true if leader has changed
        void _setLeader(const uint64 &guid);

        void _removeRolls(const uint64 &guid);

        bool _setMembersGroup(const uint64 &guid, const uint8 &group);
        bool _setAssistantFlag(const uint64 &guid, const bool &apply);
        bool _setMainTank(const uint64 &guid, const bool &apply);
        bool _setMainAssistant(const uint64 &guid, const bool &apply);

        void _homebindIfInstance(Player *player);

        void _initRaidSubGroupsCounter()
        {
            // Sub group counters initialization
            if (!m_subGroupsCounts)
                m_subGroupsCounts = new uint8[MAX_RAID_SUBGROUPS];

            memset((void*)m_subGroupsCounts, 0, (MAX_RAID_SUBGROUPS)*sizeof(uint8));

            for (member_citerator itr = m_memberSlots.begin(); itr != m_memberSlots.end(); ++itr)
                ++m_subGroupsCounts[itr->group];
        }

        member_citerator _getMemberCSlot(uint64 Guid) const
        {
            for (member_citerator itr = m_memberSlots.begin(); itr != m_memberSlots.end(); ++itr)
            {
                if (itr->guid == Guid)
                    return itr;
            }
            return m_memberSlots.end();
        }

        member_witerator _getMemberWSlot(uint64 Guid)
        {
            for (member_witerator itr = m_memberSlots.begin(); itr != m_memberSlots.end(); ++itr)
            {
                if (itr->guid == Guid)
                    return itr;
            }
            return m_memberSlots.end();
        }

        void SubGroupCounterIncrease(uint8 subgroup)
        {
            if (m_subGroupsCounts)
                ++m_subGroupsCounts[subgroup];
        }

        void SubGroupCounterDecrease(uint8 subgroup)
        {
            if (m_subGroupsCounts)
                --m_subGroupsCounts[subgroup];
        }

        void RemoveUniqueGroupMemberFlag(GroupMemberFlags flag)
        {
            for (member_witerator itr = m_memberSlots.begin(); itr != m_memberSlots.end(); ++itr)
            {
                if (itr->flags & flag)
                    itr->flags &= ~flag;
            }
        }

        void ToggleGroupMemberFlag(member_witerator slot, uint8 flag, bool apply)
        {
            if (apply)
                slot->flags |= flag;
            else
                slot->flags &= ~flag;
        }

        MemberSlotList      m_memberSlots;
        GroupRefManager     m_memberMgr;
        InvitesList         m_invitees;
        uint64              m_leaderGuid;
        std::string         m_leaderName;
        GroupType           m_groupType;
        Difficulty          m_dungeonDifficulty;
        Difficulty          m_raidDifficulty;
        Battleground*       m_bgGroup;
        Battlefield*        m_bfGroup; 
        uint64              m_targetIcons[TARGETICONCOUNT];
        LootMethod          m_lootMethod;
        uint64              m_looterGuid;
        Rolls               RollId;
        ItemQualities       m_lootThreshold;
        BoundInstancesMap   m_boundInstances[MAX_DIFFICULTY];
        uint8*              m_subGroupsCounts;
        uint64              m_guid;
        uint32              m_counter;                      // used only in SMSG_GROUP_LIST
        uint32              m_maxEnchantingLevel;
};
#endif
