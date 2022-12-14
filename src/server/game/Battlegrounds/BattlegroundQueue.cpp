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
#include "BattlegroundQueue.h"
#include "ArenaTeam.h"
#include "BattlegroundMgr.h"
#include "Chat.h"
#include "ObjectMgr.h"
#include "Log.h"


PlayerQueueInfo::~PlayerQueueInfo()
{
}

GroupQueueInfoPtr PlayerQueueInfo::GetGroupQueueInfo() const
{
    return GroupInfo.lock();
}



/*********************************************************/
/***            BATTLEGROUND QUEUE SYSTEM              ***/
/*********************************************************/

BattlegroundQueue::BattlegroundQueue()
{
    m_twink = 0;

    for (uint32 i = 0; i < BG_TEAMS_COUNT; ++i)
    {
        for (uint32 j = 0; j < MAX_BATTLEGROUND_BRACKETS; ++j)
        {
            m_SumOfWaitTimes[i][j] = 0;
            m_WaitTimeLastPlayer[i][j] = 0;
            for (uint32 k = 0; k < COUNT_OF_PLAYERS_TO_AVERAGE_WAIT_TIME; ++k)
                m_WaitTimes[i][j][k] = 0;
        }
    }
}

BattlegroundQueue::~BattlegroundQueue()
{
}

/*********************************************************/
/***      BATTLEGROUND QUEUE SELECTION POOLS           ***/
/*********************************************************/

// selection pool initialization, used to clean up from prev selection
void BattlegroundQueue::SelectionPool::Init()
{
    SelectedGroups.clear();
    PlayerCount = 0;
}

// remove group info from selection pool
// returns true when we need to try to add new group to selection pool
// returns false when selection pool is ok or when we kicked smaller group than we need to kick
// sometimes it can be called on empty selection pool
bool BattlegroundQueue::SelectionPool::KickGroup(uint32 size)
{
    //find maxgroup or LAST group with size == size and kick it
    bool found = false;
    GroupsQueueType::iterator groupToKick = SelectedGroups.begin();
    for (GroupsQueueType::iterator itr = groupToKick; itr != SelectedGroups.end(); ++itr)
    {
        if (abs((int32)((*itr)->Players.size() - size)) <= 1)
        {
            groupToKick = itr;
            found = true;
        }
        else if (!found && (*itr)->Players.size() >= (*groupToKick)->Players.size())
            groupToKick = itr;
    }
    //if pool is empty, do nothing
    if (GetPlayerCount())
    {
        //update player count
        GroupQueueInfoPtr ginfo = (*groupToKick);
        SelectedGroups.erase(groupToKick);
        PlayerCount -= ginfo->Players.size();
        //return false if we kicked smaller group or there are enough players in selection pool
        if (ginfo->Players.size() <= size + 1)
            return false;
    }
    return true;
}

// add group to selection pool
// used when building selection pools
// returns true if we can invite more players, or when we added group to selection pool
// returns false when selection pool is full
bool BattlegroundQueue::SelectionPool::AddGroup(const GroupQueueInfoPtr &ginfo, uint32 desiredCount)
{
    //if group is larger than desired count - don't allow to add it to pool
    if (!ginfo->IsInvitedToBGInstanceGUID && desiredCount >= PlayerCount + ginfo->Players.size())
    {
        SelectedGroups.push_back(ginfo);
        // increase selected players count
        PlayerCount += ginfo->Players.size();
        return true;
    }
    if (PlayerCount < desiredCount)
        return true;
    return false;
}

/*********************************************************/
/***               BATTLEGROUND QUEUES                 ***/
/*********************************************************/

// add group or player (grp == NULL) to bg queue with the given leader and bg specifications
GroupQueueInfoPtr BattlegroundQueue::AddGroup(Player *leader, Group* grp, BattlegroundTypeId BgTypeId, PvPDifficultyEntry const*  bracketEntry, uint8 ArenaType, bool isRated, bool isPremade, uint32 ArenaRating, uint32 MatchmakerRating, uint32 arenateamid)
{
    BattlegroundBracketId bracketId =  bracketEntry->GetBracketId();

    // create new ginfo
    GroupQueueInfoPtr ginfo = std::make_shared<GroupQueueInfo>();
    ginfo->BgTypeId                  = BgTypeId;
    ginfo->ArenaType                 = ArenaType;
    ginfo->ArenaTeamId               = arenateamid;
    ginfo->IsRated                   = isRated;
    ginfo->IsInvitedToBGInstanceGUID = 0;
    ginfo->JoinTime                  = getMSTime();
    ginfo->RemoveInviteTime          = 0;
    ginfo->Team                      = leader->GetTeam();
    ginfo->ArenaTeamRating           = ArenaRating;
    ginfo->ArenaMatchmakerRating     = MatchmakerRating;
    ginfo->OpponentsTeamRating       = 0;
    ginfo->OpponentsMatchmakerRating = 0;

    // balancing teams between queues - shared BG queues, mixed groups
    if (!isRated)
    {
        // balance only if there is different count of queued players

        if (m_QueuedGroups[bracketId][BG_QUEUE_NORMAL_ALLIANCE].size() > m_QueuedGroups[bracketId][BG_QUEUE_NORMAL_HORDE].size())
            ginfo->Team = HORDE;
        else if (m_QueuedGroups[bracketId][BG_QUEUE_NORMAL_ALLIANCE].size() < m_QueuedGroups[bracketId][BG_QUEUE_NORMAL_HORDE].size())
            ginfo->Team = ALLIANCE;
    }

    ginfo->Players.clear();

    //compute index (if group is premade or joined a rated match) to queues
    uint32 index = 0;
    if (!isRated && !isPremade)
        index += BG_TEAMS_COUNT;
    if (ginfo->Team == HORDE)
        index++;
    sLog->outDebug("Adding Group to BattlegroundQueue bgTypeId : %u, bracket_id : %u, index : %u", BgTypeId, bracketId, index);

    uint32 lastOnlineTime = getMSTime();

    //announce world (this don't need mutex)
    if (isRated && sWorld->getBoolConfig(CONFIG_ARENA_QUEUE_ANNOUNCER_ENABLE))
    {
        ArenaTeam *Team = sObjectMgr->GetArenaTeamById(arenateamid);
        if (Team)
            sWorld->SendWorldText(LANG_ARENA_QUEUE_ANNOUNCE_WORLD_JOIN, Team->GetName().c_str(), ginfo->ArenaType, ginfo->ArenaType, ginfo->ArenaTeamRating);
    }

    //add players from group to ginfo
    {
        //ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_Lock);
        if (grp)
        {
            for (GroupReference *itr = grp->GetFirstMember(); itr != NULL; itr = itr->next())
            {
                Player *member = itr->getSource();
                if (!member)
                    continue;   // this should never happen
                PlayerQueueInfo& pl_info = m_QueuedPlayers[member->GetGUID()];
                pl_info.LastOnlineTime   = lastOnlineTime;
                pl_info.GroupInfo        = ginfo;
                // add the pinfo to ginfo's list
                ginfo->Players[member->GetGUID()]  = &pl_info;
            }
        }
        else
        {
            PlayerQueueInfo& pl_info = m_QueuedPlayers[leader->GetGUID()];
            pl_info.LastOnlineTime   = lastOnlineTime;
            pl_info.GroupInfo        = ginfo;
            ginfo->Players[leader->GetGUID()]  = &pl_info;
        }

        //add GroupInfo to m_QueuedGroups
        if (m_twink != BATTLEGROUND_WARGAME)
            m_QueuedGroups[bracketId][index].push_back(ginfo);

        //announce to world, this code needs mutex
        if (!isRated && !isPremade && m_twink != BATTLEGROUND_WARGAME && sWorld->getBoolConfig(CONFIG_BATTLEGROUND_QUEUE_ANNOUNCER_ENABLE))
        {
            if (Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(ginfo->BgTypeId))
            {
                char const* bgName = bg->GetName();
                uint32 MinPlayers = bg->GetMinPlayersPerTeam();
                uint32 qHorde = 0;
                uint32 qAlliance = 0;
                uint32 q_min_level = bracketEntry->minLevel;
                uint32 q_max_level = bracketEntry->maxLevel;
                GroupsQueueType::const_iterator itr;
                for (itr = m_QueuedGroups[bracketId][BG_QUEUE_NORMAL_ALLIANCE].begin(); itr != m_QueuedGroups[bracketId][BG_QUEUE_NORMAL_ALLIANCE].end(); ++itr)
                    if (!(*itr)->IsInvitedToBGInstanceGUID)
                        qAlliance += (*itr)->Players.size();
                for (itr = m_QueuedGroups[bracketId][BG_QUEUE_NORMAL_HORDE].begin(); itr != m_QueuedGroups[bracketId][BG_QUEUE_NORMAL_HORDE].end(); ++itr)
                    if (!(*itr)->IsInvitedToBGInstanceGUID)
                        qHorde += (*itr)->Players.size();


                std::string bg_name = bgName;
                if (m_twink == BATTLEGROUND_TWINK)
                    bg_name += " (twink)";

                // Show queue status to player only (when joining queue)
                if (sWorld->getBoolConfig(CONFIG_BATTLEGROUND_QUEUE_ANNOUNCER_PLAYERONLY))
                {
                    ChatHandler(leader).PSendSysMessage(LANG_BG_QUEUE_ANNOUNCE_SELF, bg_name.c_str(), q_min_level, q_max_level,
                        qAlliance, (MinPlayers > qAlliance) ? MinPlayers - qAlliance : (uint32)0, qHorde, (MinPlayers > qHorde) ? MinPlayers - qHorde : (uint32)0);
                }
                // System message
                else
                {
                    sWorld->SendWorldText(LANG_BG_QUEUE_ANNOUNCE_WORLD, bg_name.c_str(), q_min_level, q_max_level,
                        qAlliance, (MinPlayers > qAlliance) ? MinPlayers - qAlliance : (uint32)0, qHorde, (MinPlayers > qHorde) ? MinPlayers - qHorde : (uint32)0);
                }
            }
        }
        //release mutex
    }

    return ginfo;
}

WargameQueueInfoPtr BattlegroundQueue::AddWargameGroups(Group* first, Group* second, BattlegroundTypeId bgTypeId, uint8 arenaType)
{
    WargameQueueInfoPtr wginfo = std::make_shared<WargameQueueInfo>();
    wginfo->BgTypeId                  = bgTypeId;
    wginfo->ArenaType                 = arenaType;
    wginfo->ArenaTeamId               = 0;
    wginfo->IsRated                   = false;
    wginfo->IsInvitedToBGInstanceGUID = 0;
    wginfo->JoinTime                  = getMSTime();
    wginfo->RemoveInviteTime          = 0;
    wginfo->Team                      = first->GetLeader()->GetTeam();
    wginfo->ArenaTeamRating           = 0;
    wginfo->ArenaMatchmakerRating     = 0;
    wginfo->OpponentsTeamRating       = 0;
    wginfo->OpponentsMatchmakerRating = 0;
    wginfo->queuedGroups[0]           = first;
    wginfo->queuedGroups[1]           = second;

    Player *member;
    for (GroupReference *itr = first->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        member = itr->getSource();
        if (!member)
            continue;   // this should never happen
        PlayerQueueInfo& pl_info = m_QueuedPlayers[member->GetGUID()];
        pl_info.LastOnlineTime   = getMSTime();
        pl_info.GroupInfo        = wginfo;
        // add the pinfo to ginfo's list
        wginfo->Players[member->GetGUID()]  = &pl_info;
    }

    for (GroupReference *itr = second->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        member = itr->getSource();
        if (!member)
            continue;   // this should never happen
        PlayerQueueInfo& pl_info = m_QueuedPlayers[member->GetGUID()];
        pl_info.LastOnlineTime   = getMSTime();
        pl_info.GroupInfo        = wginfo;
        // add the pinfo to ginfo's list
        wginfo->Players[member->GetGUID()]  = &pl_info;
    }

    m_wargameGroups.push_front(wginfo);

    return wginfo;
}

void BattlegroundQueue::PlayerInvitedToBGUpdateAverageWaitTime(const GroupQueueInfoPtr &ginfo, BattlegroundBracketId bracket_id)
{
    uint32 timeInQueue = getMSTimeDiff(ginfo->JoinTime, getMSTime());
    uint8 team_index = BG_TEAM_ALLIANCE;                    //default set to BG_TEAM_ALLIANCE - or non rated arenas!
    if (!ginfo->ArenaType)
    {
        if (ginfo->Team == HORDE)
            team_index = BG_TEAM_HORDE;
    }
    else
    {
        if (ginfo->IsRated)
            team_index = BG_TEAM_HORDE;                     //for rated arenas use BG_TEAM_HORDE
    }

    //store pointer to arrayindex of player that was added first
    uint32* lastPlayerAddedPointer = &(m_WaitTimeLastPlayer[team_index][bracket_id]);
    //remove his time from sum
    m_SumOfWaitTimes[team_index][bracket_id] -= m_WaitTimes[team_index][bracket_id][(*lastPlayerAddedPointer)];
    //set average time to new
    m_WaitTimes[team_index][bracket_id][(*lastPlayerAddedPointer)] = timeInQueue;
    //add new time to sum
    m_SumOfWaitTimes[team_index][bracket_id] += timeInQueue;
    //set index of last player added to next one
    (*lastPlayerAddedPointer)++;
    (*lastPlayerAddedPointer) %= COUNT_OF_PLAYERS_TO_AVERAGE_WAIT_TIME;
}

uint32 BattlegroundQueue::GetAverageQueueWaitTime(const GroupQueueInfoPtr &ginfo, BattlegroundBracketId bracket_id) const
{
    uint8 team_index = BG_TEAM_ALLIANCE;                    //default set to BG_TEAM_ALLIANCE - or non rated arenas!
    if (!ginfo->ArenaType)
    {
        if (ginfo->Team == HORDE)
            team_index = BG_TEAM_HORDE;
    }
    else
    {
        if (ginfo->IsRated)
            team_index = BG_TEAM_HORDE;                     //for rated arenas use BG_TEAM_HORDE
    }
    //check if there is enought values(we always add values > 0)
    if (m_WaitTimes[team_index][bracket_id][COUNT_OF_PLAYERS_TO_AVERAGE_WAIT_TIME - 1])
        return (m_SumOfWaitTimes[team_index][bracket_id] / COUNT_OF_PLAYERS_TO_AVERAGE_WAIT_TIME);
    else
        //if there aren't enough values return 0 - not available
        return 0;
}

//remove player from queue and from group info, if group info is empty then remove it too
void BattlegroundQueue::RemovePlayer(const uint64& guid, bool decreaseInvitedCount)
{
    //Player *plr = sObjectMgr->GetPlayer(guid);

    int32 bracket_id = -1;                                     // signed for proper for-loop finish
    QueuedPlayersMap::iterator itr;

    //remove player from map, if he's there
    itr = m_QueuedPlayers.find(guid);
    if (itr == m_QueuedPlayers.end())
    {
        sLog->outError("BattlegroundQueue: couldn't find player to remove GUID: %u", GUID_LOPART(guid));
        return;
    }

    GroupQueueInfoPtr group = itr->second.GetGroupQueueInfo();
    GroupsQueueType::iterator group_itr, group_itr_tmp;
    WargameQueueType::iterator wargame_itr, wargame_itr_tmp;
    // mostly people with the highest levels are in battlegrounds, thats why
    // we count from MAX_BATTLEGROUND_QUEUES - 1 to 0
    int32 index = -1;

    // Differ wargame queue from normal queues
    if (m_twink == BATTLEGROUND_WARGAME)
    {
        wargame_itr = m_wargameGroups.end();
        for (wargame_itr_tmp = m_wargameGroups.begin(); wargame_itr_tmp != m_wargameGroups.end(); ++wargame_itr_tmp)
        {
            if ((*wargame_itr_tmp) == group)
            {
                wargame_itr = wargame_itr_tmp;
                break;
            }
        }
    }
    else
    {
        for (int32 bracket_id_tmp = MAX_BATTLEGROUND_BRACKETS - 1; bracket_id_tmp >= 0 && bracket_id == -1; --bracket_id_tmp)
        {
            //we must check premade and normal team's queue - because when players from premade are joining bg,
            //they leave groupinfo so we can't use its players size to find out index
            for (uint32 j = 0; j < BG_QUEUE_GROUP_TYPES_COUNT; j++)
            {
                for (group_itr_tmp = m_QueuedGroups[bracket_id_tmp][j].begin(); group_itr_tmp != m_QueuedGroups[bracket_id_tmp][j].end(); ++group_itr_tmp)
                {
                    if ((*group_itr_tmp) == group)
                    {
                        bracket_id = bracket_id_tmp;
                        group_itr = group_itr_tmp;
                        //we must store index to be able to erase iterator
                        index = j;
                        break;
                    }
                }
            }
        }
    }
    //player can't be in queue without group, but just in case
    if ((m_twink == BATTLEGROUND_WARGAME && wargame_itr == m_wargameGroups.end()) || (m_twink != BATTLEGROUND_WARGAME && (bracket_id == -1 || index == -1)))
    {
        sLog->outError("BattlegroundQueue: ERROR Cannot find groupinfo for player GUID: %u", GUID_LOPART(guid));
        return;
    }
    sLog->outDebug("BattlegroundQueue: Removing player GUID %u, from bracket_id %u", GUID_LOPART(guid), (uint32)bracket_id);

    // ALL variables are correctly set
    // We can ignore leveling up in queue - it should not cause crash
    // remove player from group
    // if only one player there, remove group

    // remove player queue info from group queue info
    std::map<uint64, PlayerQueueInfo*>::iterator pitr = group->Players.find(guid);
    if (pitr != group->Players.end())
        group->Players.erase(pitr);

    // if invited to bg, and should decrease invited count, then do it
    if (decreaseInvitedCount && group->IsInvitedToBGInstanceGUID)
    {
        Battleground* bg = sBattlegroundMgr->GetBattleground(group->IsInvitedToBGInstanceGUID, group->BgTypeId == BATTLEGROUND_AA ? BATTLEGROUND_TYPE_NONE : group->BgTypeId);
        if (bg)
            bg->DecreaseInvitedCount(group->Team);
    }

    // remove player queue info
    m_QueuedPlayers.erase(itr);

    // announce to world if arena team left queue for rated match, show only once
    if (group->ArenaType && group->IsRated && group->Players.empty() && sWorld->getBoolConfig(CONFIG_ARENA_QUEUE_ANNOUNCER_ENABLE))
    {
        ArenaTeam *Team = sObjectMgr->GetArenaTeamById(group->ArenaTeamId);
        if (Team)
            sWorld->SendWorldText(LANG_ARENA_QUEUE_ANNOUNCE_WORLD_EXIT, Team->GetName().c_str(), group->ArenaType, group->ArenaType, group->ArenaTeamRating);
    }

    //if player leaves queue and he is invited to rated arena match, then he have to lose
    if (group->IsInvitedToBGInstanceGUID && group->IsRated && decreaseInvitedCount)
    {
        ArenaTeam * at = sObjectMgr->GetArenaTeamById(group->ArenaTeamId);
        if (at)
        {
            sLog->outDebug("UPDATING memberLost's personal arena rating for %u by opponents rating: %u", GUID_LOPART(guid), group->OpponentsTeamRating);
            Player *plr = sObjectMgr->GetPlayer(guid);
            if (plr)
                at->MemberLost(plr, at->GetAverageMMR(plr->GetGroup()));
            else
                at->OfflineMemberLost(guid, at->GetTeamRating());       // TODO: use average MMR of whole group as if he were online instead of team rating
            at->SaveToDB();
        }
    }

    // remove group queue info if needed
    if (group->Players.empty())
    {
        if (m_twink != BATTLEGROUND_WARGAME)
            m_QueuedGroups[bracket_id][index].erase(group_itr);
        else
            m_wargameGroups.erase(wargame_itr);
    }
    // if group wasn't empty, so it wasn't deleted, and player have left a rated
    // queue -> everyone from the group should leave too
    // don't remove recursively if already invited to bg!
    else if (!group->IsInvitedToBGInstanceGUID && group->IsRated)
    {
        // remove next player, this is recursive
        // first send removal information
        if (Player *plr2 = sObjectMgr->GetPlayer(group->Players.begin()->first))
        {
            Battleground * bg = sBattlegroundMgr->GetBattlegroundTemplate(group->BgTypeId);
            BattlegroundQueueTypeId bgQueueTypeId = BattlegroundMgr::BGQueueTypeId(group->BgTypeId, group->ArenaType);
            uint32 queueSlot = plr2->GetBattlegroundQueueIndex(bgQueueTypeId);
            plr2->RemoveBattlegroundQueueId(bgQueueTypeId); // must be called this way, because if you move this call to
                                                            // queue->removeplayer, it causes bugs
            WorldPacket data;
            sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, plr2, queueSlot, STATUS_NONE, plr2->GetBattlegroundQueueJoinTime(group->BgTypeId), 0, group->ArenaType);
            plr2->GetSession()->SendPacket(&data);
        }
        // then actually delete, this may delete the group as well!
        RemovePlayer(group->Players.begin()->first, decreaseInvitedCount);
    }
}

//returns true when player pl_guid is in queue and is invited to bgInstanceGuid
bool BattlegroundQueue::IsPlayerInvited(const uint64& pl_guid, const uint32 bgInstanceGuid, const uint32 removeTime)
{
    QueuedPlayersMap::const_iterator qItr = m_QueuedPlayers.find(pl_guid);
    return (qItr != m_QueuedPlayers.end()
        && qItr->second.GetGroupQueueInfo()->IsInvitedToBGInstanceGUID == bgInstanceGuid
        && qItr->second.GetGroupQueueInfo()->RemoveInviteTime == removeTime);
}

GroupQueueInfoPtr BattlegroundQueue::GetPlayerGroupInfoData(const uint64& guid)
{
    QueuedPlayersMap::const_iterator qItr = m_QueuedPlayers.find(guid);
    if (qItr == m_QueuedPlayers.end())
        return nullptr;
    return qItr->second.GetGroupQueueInfo();
}

bool BattlegroundQueue::InviteGroupToBG(const GroupQueueInfoPtr &ginfo, Battleground * bg, uint32 side, bool isWargame)
{
    // set side if needed
    if (side)
        ginfo->Team = side;

    if (!ginfo->IsInvitedToBGInstanceGUID)
    {
        // not yet invited
        // set invitation
        ginfo->IsInvitedToBGInstanceGUID = bg->GetInstanceID();

        if (bg->isRated() && !bg->isArena())
            sLog->outChar("RatedBG: battleground group (side: %u) invited to instance ID %u", ginfo->Team, bg->GetInstanceID());

        BattlegroundTypeId bgTypeId = bg->GetTypeID();
        BattlegroundQueueTypeId bgQueueTypeId = BattlegroundMgr::BGQueueTypeId(bgTypeId, bg->GetArenaType());
        BattlegroundBracketId bracket_id = bg->GetBracketId();

        // set ArenaTeamId for rated matches
        if (bg->isArena() && bg->isRated())
            bg->SetArenaTeamIdForTeam(ginfo->Team, ginfo->ArenaTeamId);

        ginfo->RemoveInviteTime = getMSTime() + INVITE_ACCEPT_WAIT_TIME;

        if (!isWargame)
        {
            WorldPacket data;
            // loop through the players
            for (std::map<uint64,PlayerQueueInfo*>::iterator itr = ginfo->Players.begin(); itr != ginfo->Players.end(); ++itr)
            {
                // get the player
                Player* plr = sObjectMgr->GetPlayer(itr->first);
                // if offline, skip him, this should not happen - player is removed from queue when he logs out
                if (!plr)
                    continue;

                plr->SetBGTeam(ginfo->Team);

                // invite the player
                PlayerInvitedToBGUpdateAverageWaitTime(ginfo, bracket_id);
                //sBattlegroundMgr->InvitePlayer(plr, bg, ginfo->Team);

                // set invited player counters
                bg->IncreaseInvitedCount(ginfo->Team);

                plr->SetInviteForBattlegroundQueueType(bgQueueTypeId, ginfo->IsInvitedToBGInstanceGUID);

                // create remind invite events
                BGQueueInviteEvent* inviteEvent = new BGQueueInviteEvent(plr->GetGUID(), ginfo->IsInvitedToBGInstanceGUID, bgTypeId, ginfo->ArenaType, ginfo->RemoveInviteTime);
                plr->m_Events.AddEvent(inviteEvent, plr->m_Events.CalculateTime(INVITATION_REMIND_TIME));
                // create automatic remove events
                BGQueueRemoveEvent* removeEvent = new BGQueueRemoveEvent(plr->GetGUID(), ginfo->IsInvitedToBGInstanceGUID, bgTypeId, bgQueueTypeId, ginfo->RemoveInviteTime);
                plr->m_Events.AddEvent(removeEvent, plr->m_Events.CalculateTime(INVITE_ACCEPT_WAIT_TIME));

                uint32 queueSlot = plr->GetBattlegroundQueueIndex(bgQueueTypeId);

                sLog->outDebug("Battleground: invited plr %s (%u) to BG instance %u queueindex %u bgtype %u, I can't help it if they don't press the enter battle button.",plr->GetName(),plr->GetGUIDLow(),bg->GetInstanceID(),queueSlot,bg->GetTypeID());

                // send status packet
                data.clear();
                sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, plr, queueSlot, STATUS_WAIT_JOIN, INVITE_ACCEPT_WAIT_TIME, plr->GetBattlegroundQueueJoinTime(bgTypeId), ginfo->ArenaType);
                plr->GetSession()->SendPacket(&data);
            }
        }
        else
        {
            uint32 team;
            WorldPacket data;
            // loop through the players
            for (std::map<uint64,PlayerQueueInfo*>::iterator itr = ginfo->Players.begin(); itr != ginfo->Players.end(); ++itr)
            {
                // get the player
                Player* plr = sObjectMgr->GetPlayer(itr->first);
                // if offline, skip him, this should not happen - player is removed from queue when he logs out
                if (!plr)
                    continue;

                team = (std::static_pointer_cast<WargameQueueInfo>(ginfo)->queuedGroups[0]->GetGUID() == plr->GetGroup()->GetGUID()) ? HORDE : ALLIANCE;

                plr->SetBGTeam(team);

                // invite the player
                PlayerInvitedToBGUpdateAverageWaitTime(ginfo, bracket_id);
                //sBattlegroundMgr->InvitePlayer(plr, bg, team);

                // set invited player counters
                bg->IncreaseInvitedCount(team);

                plr->SetInviteForBattlegroundQueueType(bgQueueTypeId, ginfo->IsInvitedToBGInstanceGUID);

                // create remind invite events
                BGQueueInviteEvent* inviteEvent = new BGQueueInviteEvent(plr->GetGUID(), ginfo->IsInvitedToBGInstanceGUID, bgTypeId, ginfo->ArenaType, ginfo->RemoveInviteTime);
                plr->m_Events.AddEvent(inviteEvent, plr->m_Events.CalculateTime(INVITATION_REMIND_TIME));
                // create automatic remove events
                BGQueueRemoveEvent* removeEvent = new BGQueueRemoveEvent(plr->GetGUID(), ginfo->IsInvitedToBGInstanceGUID, bgTypeId, bgQueueTypeId, ginfo->RemoveInviteTime);
                plr->m_Events.AddEvent(removeEvent, plr->m_Events.CalculateTime(INVITE_ACCEPT_WAIT_TIME));

                uint32 queueSlot = plr->GetBattlegroundQueueIndex(bgQueueTypeId);

                sLog->outDebug("Battleground: invited plr %s (%u) to BG instance %u queueindex %u bgtype %u, wargame.",plr->GetName(),plr->GetGUIDLow(),bg->GetInstanceID(),queueSlot,bg->GetTypeID());

                // send status packet
                data.clear();
                sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, plr, queueSlot, STATUS_WAIT_JOIN, INVITE_ACCEPT_WAIT_TIME, plr->GetBattlegroundQueueJoinTime(bgTypeId), ginfo->ArenaType);
                plr->GetSession()->SendPacket(&data);
            }
        }
        return true;
    }

    return false;
}

/*
This function is inviting players to already running battlegrounds

Teams are now joined, so no fraction-level balancing is present - all people are invited, and
their fraction is decided on teleport (see WorldSession::HandleBattleFieldPortOpcode)
*/
void BattlegroundQueue::FillPlayersToBG(Battleground* bg, BattlegroundBracketId bracket_id)
{
    uint32 aliFree = bg->GetMaxPlayersPerTeam() - bg->GetPlayersCountByTeam(ALLIANCE);
    uint32 hordeFree = bg->GetMaxPlayersPerTeam() - bg->GetPlayersCountByTeam(HORDE);

    GroupsQueueType::const_iterator Ali_itr = m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE].begin();
    GroupsQueueType::const_iterator Horde_itr = m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_HORDE].begin();
    uint32 aliCount = m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE].size();
    uint32 hordeCount = m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_HORDE].size();
    uint32 aliIndex = 0;
    uint32 hordeIndex = 0;

    for (; hordeIndex < hordeCount && m_SelectionPools[BG_TEAM_HORDE].AddGroup((*Horde_itr), hordeFree); hordeIndex++)
        ++Horde_itr;

    for (; aliIndex < aliCount && m_SelectionPools[BG_TEAM_ALLIANCE].AddGroup((*Ali_itr), aliFree); aliIndex++)
        ++Ali_itr;
}

// this method checks if premade versus premade battleground is possible
// then after 30 mins (default) in queue it moves premade group to normal queue
// it tries to invite as much players as it can - to MaxPlayersPerTeam, because premade groups have more than MinPlayersPerTeam players
bool BattlegroundQueue::CheckPremadeMatch(BattlegroundBracketId bracket_id, uint32 MinPlayersPerTeam, uint32 MaxPlayersPerTeam)
{
    //check match
    if (!m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].empty() && !m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].empty())
    {
        //start premade match
        //if groups aren't invited
        GroupsQueueType::const_iterator ali_group, horde_group;
        for (ali_group = m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].begin(); ali_group != m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].end(); ++ali_group)
            if (!(*ali_group)->IsInvitedToBGInstanceGUID)
                break;
        for (horde_group = m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].begin(); horde_group != m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].end(); ++horde_group)
            if (!(*horde_group)->IsInvitedToBGInstanceGUID)
                break;

        if (ali_group != m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].end() && horde_group != m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].end())
        {
            m_SelectionPools[BG_TEAM_ALLIANCE].AddGroup((*ali_group), MaxPlayersPerTeam);
            m_SelectionPools[BG_TEAM_HORDE].AddGroup((*horde_group), MaxPlayersPerTeam);
            //add groups/players from normal queue to size of bigger group
            uint32 maxPlayers = std::min(m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount(), m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount());
            GroupsQueueType::const_iterator itr;
            for (uint32 i = 0; i < BG_TEAMS_COUNT; i++)
            {
                for (itr = m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + i].begin(); itr != m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + i].end(); ++itr)
                {
                    //if itr can join BG and player count is less that maxPlayers, then add group to selectionpool
                    if (!(*itr)->IsInvitedToBGInstanceGUID && !m_SelectionPools[i].AddGroup((*itr), maxPlayers))
                        break;
                }
            }
            //premade selection pools are set
            return true;
        }
    }
    // now check if we can move group from Premade queue to normal queue (timer has expired) or group size lowered!!
    // this could be 2 cycles but i'm checking only first team in queue - it can cause problem -
    // if first is invited to BG and seconds timer expired, but we can ignore it, because players have only 80 seconds to click to enter bg
    // and when they click or after 80 seconds the queue info is removed from queue
    uint32 time_before = getMSTime() - sWorld->getIntConfig(CONFIG_BATTLEGROUND_PREMADE_GROUP_WAIT_FOR_MATCH);
    for (uint32 i = 0; i < BG_TEAMS_COUNT; i++)
    {
        if (!m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE + i].empty())
        {
            GroupsQueueType::iterator itr = m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE + i].begin();
            if (!(*itr)->IsInvitedToBGInstanceGUID && ((*itr)->JoinTime < time_before || (*itr)->Players.size() < MinPlayersPerTeam))
            {
                //we must insert group to normal queue and erase pointer from premade queue
                m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + i].push_front((*itr));
                m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE + i].erase(itr);
            }
        }
    }
    //selection pools are not set
    return false;
}

// this method tries to create battleground or arena with MinPlayersPerTeam against MinPlayersPerTeam
bool BattlegroundQueue::CheckNormalMatch(Battleground* bg_template, BattlegroundBracketId bracket_id, uint32 minPlayers, uint32 maxPlayers)
{
    GroupsQueueType::const_iterator itr_team[BG_TEAMS_COUNT];
    for (uint32 i = 0; i < BG_TEAMS_COUNT; i++)
    {
        itr_team[i] = m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + i].begin();
        for (; itr_team[i] != m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + i].end(); ++(itr_team[i]))
        {
            if (!(*(itr_team[i]))->IsInvitedToBGInstanceGUID)
            {
                m_SelectionPools[i].AddGroup(*(itr_team[i]), maxPlayers);
                if (m_SelectionPools[i].GetPlayerCount() >= minPlayers)
                    break;
            }
        }
    }
    //try to invite same number of players - this cycle may cause longer wait time even if there are enough players in queue, but we want ballanced bg
    uint32 j = BG_TEAM_ALLIANCE;
    if (m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount() < m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount())
        j = BG_TEAM_HORDE;
    if (sWorld->getIntConfig(CONFIG_BATTLEGROUND_INVITATION_TYPE) != 0
        && m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount() >= minPlayers && m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount() >= minPlayers)
    {
        //we will try to invite more groups to team with less players indexed by j
        ++(itr_team[j]);                                         //this will not cause a crash, because for cycle above reached break;
        for (; itr_team[j] != m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + j].end(); ++(itr_team[j]))
        {
            if (!(*(itr_team[j]))->IsInvitedToBGInstanceGUID)
                if (!m_SelectionPools[j].AddGroup(*(itr_team[j]), m_SelectionPools[(j + 1) % BG_TEAMS_COUNT].GetPlayerCount()))
                    break;
        }
        // do not allow to start bg with more than 2 players more on 1 faction
        if (abs((int32)(m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount() - m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount())) > 2)
            return false;
    }
    //allow 1v0 if debug bg
    if (sBattlegroundMgr->isTesting() && bg_template->isBattleground() && (m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount() || m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount()))
        return true;
    //return true if there are enough players in selection pools - enable to work .debug bg command correctly
    return m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount() >= minPlayers && m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount() >= minPlayers;
}

// this method will check if we can invite players to same faction skirmish match
bool BattlegroundQueue::CheckSkirmishForSameFaction(BattlegroundBracketId bracket_id, uint32 minPlayersPerTeam)
{
    if (m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount() < minPlayersPerTeam && m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount() < minPlayersPerTeam)
        return false;
    uint32 teamIndex = BG_TEAM_ALLIANCE;
    uint32 otherTeam = BG_TEAM_HORDE;
    uint32 otherTeamId = HORDE;
    if (m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount() == minPlayersPerTeam)
    {
        teamIndex = BG_TEAM_HORDE;
        otherTeam = BG_TEAM_ALLIANCE;
        otherTeamId = ALLIANCE;
    }
    //clear other team's selection
    m_SelectionPools[otherTeam].Init();
    //store last ginfo pointer
    GroupQueueInfoPtr ginfo = m_SelectionPools[teamIndex].SelectedGroups.back();
    //set itr_team to group that was added to selection pool latest
    GroupsQueueType::iterator itr_team = m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + teamIndex].begin();
    for (; itr_team != m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + teamIndex].end(); ++itr_team)
        if (ginfo == *itr_team)
            break;
    if (itr_team == m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + teamIndex].end())
        return false;
    GroupsQueueType::iterator itr_team2 = itr_team;
    ++itr_team2;
    //invite players to other selection pool
    for (; itr_team2 != m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + teamIndex].end(); ++itr_team2)
    {
        //if selection pool is full then break;
        if (!(*itr_team2)->IsInvitedToBGInstanceGUID && !m_SelectionPools[otherTeam].AddGroup(*itr_team2, minPlayersPerTeam))
            break;
    }
    if (m_SelectionPools[otherTeam].GetPlayerCount() != minPlayersPerTeam)
        return false;

    //here we have correct 2 selections and we need to change one teams team and move selection pool teams to other team's queue
    for (GroupsQueueType::iterator itr = m_SelectionPools[otherTeam].SelectedGroups.begin(); itr != m_SelectionPools[otherTeam].SelectedGroups.end(); ++itr)
    {
        //set correct team
        (*itr)->Team = otherTeamId;
        //add team to other queue
        m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + otherTeam].push_front(*itr);
        //remove team from old queue
        GroupsQueueType::iterator itr2 = itr_team;
        ++itr2;
        for (; itr2 != m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + teamIndex].end(); ++itr2)
        {
            if (*itr2 == *itr)
            {
                m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE + teamIndex].erase(itr2);
                break;
            }
        }
    }
    return true;
}

/*
this method is called when group is inserted, or player / group is removed from BG Queue - there is only one player's status changed, so we don't use while (true) cycles to invite whole queue
it must be called after fully adding the members of a group to ensure group joining
should be called from Battleground::RemovePlayer function in some cases
*/
void BattlegroundQueue::Update(BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket_id, uint8 arenaType, bool isRated, uint32 arenaRating)
{
    //if no players in queue - do nothing
    if (m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].empty() &&
        m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].empty() &&
        m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_ALLIANCE].empty() &&
        m_QueuedGroups[bracket_id][BG_QUEUE_NORMAL_HORDE].empty() &&
        m_wargameGroups.empty())
        return;

    //battleground with free slot for player should be always in the beggining of the queue
    // maybe it would be better to create bgfreeslotqueue for each bracket_id
    BGFreeSlotQueueType::iterator itr, next;
    for (itr = sBattlegroundMgr->BGFreeSlotQueue[bgTypeId][m_twink].begin(); itr != sBattlegroundMgr->BGFreeSlotQueue[bgTypeId][m_twink].end(); itr = next)
    {
        next = itr;
        ++next;
        // DO NOT allow queue manager to invite new player to arena and rated battlegrounds
        if ((*itr)->isBattleground() && (*itr)->GetTypeID() == bgTypeId && (*itr)->GetBracketId() == bracket_id &&
            (*itr)->GetStatus() > STATUS_WAIT_QUEUE && (*itr)->GetStatus() < STATUS_WAIT_LEAVE && !(*itr)->isRated() && m_twink != BATTLEGROUND_WARGAME)
        {
            Battleground* bg = *itr; //we have to store battleground pointer here, because when battleground is full, it is removed from free queue (not yet implemented!!)
            // and iterator is invalid

            // clear selection pools
            m_SelectionPools[BG_TEAM_ALLIANCE].Init();
            m_SelectionPools[BG_TEAM_HORDE].Init();

            // call a function that does the job for us
            FillPlayersToBG(bg, bracket_id);

            // now everything is set, invite players
            for (GroupsQueueType::const_iterator citr = m_SelectionPools[BG_TEAM_ALLIANCE].SelectedGroups.begin(); citr != m_SelectionPools[BG_TEAM_ALLIANCE].SelectedGroups.end(); ++citr)
                InviteGroupToBG((*citr), bg, (*citr)->Team);
            for (GroupsQueueType::const_iterator citr = m_SelectionPools[BG_TEAM_HORDE].SelectedGroups.begin(); citr != m_SelectionPools[BG_TEAM_HORDE].SelectedGroups.end(); ++citr)
                InviteGroupToBG((*citr), bg, (*citr)->Team);

            if (!bg->HasFreeSlots())
            {
                // remove BG from BGFreeSlotQueue
                bg->RemoveFromBGFreeSlotQueue();
            }
        }
    }

    // finished iterating through the bgs with free slots, maybe we need to create a new bg

    Battleground * bg_template = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId);
    if (!bg_template)
    {
        sLog->outError("Battleground: Update: bg template not found for %u", bgTypeId);
        return;
    }

    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketById(bg_template->GetMapId(),bracket_id);
    if (!bracketEntry)
    {
        sLog->outError("Battleground: Update: bg bracket entry not found for map %u bracket id %u", bg_template->GetMapId(), bracket_id);
        return;
    }

    // get the min. players per team, properly for larger arenas as well. (must have full teams for arena matches!)
    uint32 MinPlayersPerTeam = bg_template->GetMinPlayersPerTeam();
    uint32 MaxPlayersPerTeam = bg_template->GetMaxPlayersPerTeam();
    if (sBattlegroundMgr->isTesting())
        MinPlayersPerTeam = 1;
    if (bg_template->isArena())
    {
        if (sBattlegroundMgr->isArenaTesting())
        {
            MaxPlayersPerTeam = arenaType;
            MinPlayersPerTeam = 1;
        }
        else
        {
            //this switch can be much shorter
            MaxPlayersPerTeam = arenaType;
            MinPlayersPerTeam = arenaType;
            /*switch(arenaType)
            {
            case ARENA_TYPE_2v2:
                MaxPlayersPerTeam = 2;
                MinPlayersPerTeam = 2;
                break;
            case ARENA_TYPE_3v3:
                MaxPlayersPerTeam = 3;
                MinPlayersPerTeam = 3;
                break;
            case ARENA_TYPE_5v5:
                MaxPlayersPerTeam = 5;
                MinPlayersPerTeam = 5;
                break;
            }*/
        }
    }

    if (isRated && bg_template->isBattleground())
    {
        if (sBattlegroundMgr->isTesting())
        {
            MinPlayersPerTeam = 1;
            MaxPlayersPerTeam = arenaType;
        }
        else
        {
            MinPlayersPerTeam = arenaType;
            MaxPlayersPerTeam = arenaType;
        }
    }

    m_SelectionPools[BG_TEAM_ALLIANCE].Init();
    m_SelectionPools[BG_TEAM_HORDE].Init();

    // Wargames have their own handler here due to not look at ratings and conditions like if there is some group on the other side
    if (m_twink == BATTLEGROUND_WARGAME)
    {
        for (WargameQueueType::iterator itr = m_wargameGroups.begin(); itr != m_wargameGroups.end(); ++itr)
        {
            // groups already invited to fight
            if ((*itr)->IsInvitedToBGInstanceGUID)
                return;

            Battleground* wgame = sBattlegroundMgr->CreateNewBattleground(bgTypeId, bracketEntry, arenaType, false, true);
            if (!wgame)
            {
                sLog->outError("BattlegroundQueue::Update couldn't create bg/arena instance for wargame match!");
                return;
            }

            wgame->SetArenaMatchmakerRating(ALLIANCE, 0);
            wgame->SetArenaMatchmakerRating(HORDE, 0);
            InviteGroupToBG(*itr, wgame, ALLIANCE, true);

            wgame->StartBattleground(m_twink);
        }

        return;
    }

    if (bg_template->isBattleground() && !isRated)
    {
        //check if there is premade against premade match
        if (CheckPremadeMatch(bracket_id, MinPlayersPerTeam, MaxPlayersPerTeam))
        {
            //create new battleground
            Battleground * bg2 = sBattlegroundMgr->CreateNewBattleground(bgTypeId, bracketEntry, 0, false);
            if (!bg2)
            {
                sLog->outError("BattlegroundQueue::Update - Cannot create battleground: %u", bgTypeId);
                return;
            }
            //invite those selection pools
            for (uint32 i = 0; i < BG_TEAMS_COUNT; i++)
                for (GroupsQueueType::const_iterator citr = m_SelectionPools[BG_TEAM_ALLIANCE + i].SelectedGroups.begin(); citr != m_SelectionPools[BG_TEAM_ALLIANCE + i].SelectedGroups.end(); ++citr)
                    InviteGroupToBG((*citr), bg2, (*citr)->Team);
            //start bg
            bg2->StartBattleground(m_twink);
            //clear structures
            m_SelectionPools[BG_TEAM_ALLIANCE].Init();
            m_SelectionPools[BG_TEAM_HORDE].Init();
        }
    }

    // now check if there are in queues enough players to start new game of (normal battleground, or non-rated arena)
    if (!isRated)
    {
        // if there are enough players in pools, start new battleground or non rated arena
        if (CheckNormalMatch(bg_template, bracket_id, MinPlayersPerTeam, MaxPlayersPerTeam)
            || (bg_template->isArena() && CheckSkirmishForSameFaction(bracket_id, MinPlayersPerTeam)))
        {
            // we successfully created a pool
            Battleground * bg2 = sBattlegroundMgr->CreateNewBattleground(bgTypeId, bracketEntry, arenaType, false);
            if (!bg2)
            {
                sLog->outError("BattlegroundQueue::Update - Cannot create battleground: %u", bgTypeId);
                return;
            }

            // invite those selection pools
            for (uint32 i = 0; i < BG_TEAMS_COUNT; i++)
                for (GroupsQueueType::const_iterator citr = m_SelectionPools[BG_TEAM_ALLIANCE + i].SelectedGroups.begin(); citr != m_SelectionPools[BG_TEAM_ALLIANCE + i].SelectedGroups.end(); ++citr)
                    InviteGroupToBG((*citr), bg2, (*citr)->Team);
            // start bg
            bg2->StartBattleground(m_twink);
        }
    }
    else if (bg_template->isArena())
    {
        // found out the minimum and maximum ratings the newly added team should battle against
        // arenaRating is the rating of the latest joined team, or 0
        // 0 is on (automatic update call) and we must set it to team's with longest wait time
        if (!arenaRating)
        {
            GroupQueueInfoPtr front1 = nullptr;
            GroupQueueInfoPtr front2 = nullptr;
            if (!m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].empty())
            {
                front1 = m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].front();
                arenaRating = front1->ArenaMatchmakerRating;
            }
            if (!m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].empty())
            {
                front2 = m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].front();
                arenaRating = front2->ArenaMatchmakerRating;
            }
            if (front1 && front2)
            {
                if (front1->JoinTime < front2->JoinTime)
                    arenaRating = front1->ArenaMatchmakerRating;
            }
            else if (!front1 && !front2)
                return; //queues are empty
        }

        //set rating range
        uint32 arenaMinRating = (arenaRating <= sBattlegroundMgr->GetMaxRatingDifference()) ? 0 : arenaRating - sBattlegroundMgr->GetMaxRatingDifference();
        uint32 arenaMaxRating = arenaRating + sBattlegroundMgr->GetMaxRatingDifference();
        // if max rating difference is set and the time past since server startup is greater than the rating discard time
        // (after what time the ratings aren't taken into account when making teams) then
        // the discard time is current_time - time_to_discard, teams that joined after that, will have their ratings taken into account
        // else leave the discard time on 0, this way all ratings will be discarded
        uint32 discardTime = getMSTime() - sBattlegroundMgr->GetRatingDiscardTimer();

        // we need to find 2 teams which will play next game

        GroupsQueueType::iterator itr_team[BG_TEAMS_COUNT];

        //optimalization : --- we dont need to use selection_pools - each update we select max 2 groups

        for (uint32 i = BG_QUEUE_PREMADE_ALLIANCE; i < BG_QUEUE_NORMAL_ALLIANCE; i++)
        {
            // take the group that joined first
            itr_team[i] = m_QueuedGroups[bracket_id][i].begin();
            for (; itr_team[i] != m_QueuedGroups[bracket_id][i].end(); ++(itr_team[i]))
            {
                // if group match conditions, then add it to pool
                if (!(*itr_team[i])->IsInvitedToBGInstanceGUID
                    && (((*itr_team[i])->ArenaMatchmakerRating >= arenaMinRating && (*itr_team[i])->ArenaMatchmakerRating <= arenaMaxRating)
                        || (*itr_team[i])->JoinTime < discardTime))
                {
                    m_SelectionPools[i].AddGroup((*itr_team[i]), MaxPlayersPerTeam);
                    // break for cycle to be able to start selecting another group from same faction queue
                    break;
                }
            }
        }
        // now we are done if we have 2 groups - ali vs horde!
        // if we don't have, we must try to continue search in same queue
        // tmp variables are correctly set
        // this code isn't much userfriendly - but it is supposed to continue search for mathing group in HORDE queue
        if (m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount() == 0 && m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount())
        {
            itr_team[BG_TEAM_ALLIANCE] = itr_team[BG_TEAM_HORDE];
            ++itr_team[BG_TEAM_ALLIANCE];
            for (; itr_team[BG_TEAM_ALLIANCE] != m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].end(); ++(itr_team[BG_TEAM_ALLIANCE]))
            {
                if (!(*itr_team[BG_TEAM_ALLIANCE])->IsInvitedToBGInstanceGUID
                    && (((*itr_team[BG_TEAM_ALLIANCE])->ArenaMatchmakerRating >= arenaMinRating && (*itr_team[BG_TEAM_ALLIANCE])->ArenaMatchmakerRating <= arenaMaxRating)
                        || (*itr_team[BG_TEAM_ALLIANCE])->JoinTime < discardTime))
                {
                    m_SelectionPools[BG_TEAM_ALLIANCE].AddGroup((*itr_team[BG_TEAM_ALLIANCE]), MaxPlayersPerTeam);
                    break;
                }
            }
        }
        // this code isn't much userfriendly - but it is supposed to continue search for mathing group in ALLIANCE queue
        if (m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount() == 0 && m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount())
        {
            itr_team[BG_TEAM_HORDE] = itr_team[BG_TEAM_ALLIANCE];
            ++itr_team[BG_TEAM_HORDE];
            for (; itr_team[BG_TEAM_HORDE] != m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].end(); ++(itr_team[BG_TEAM_HORDE]))
            {
                if (!(*itr_team[BG_TEAM_HORDE])->IsInvitedToBGInstanceGUID
                    && (((*itr_team[BG_TEAM_HORDE])->ArenaMatchmakerRating >= arenaMinRating && (*itr_team[BG_TEAM_HORDE])->ArenaMatchmakerRating <= arenaMaxRating)
                        || (*itr_team[BG_TEAM_HORDE])->JoinTime < discardTime))
                {
                    m_SelectionPools[BG_TEAM_HORDE].AddGroup((*itr_team[BG_TEAM_HORDE]), MaxPlayersPerTeam);
                    break;
                }
            }
        }

        //if we have 2 teams, then start new arena and invite players!
        if (m_SelectionPools[BG_TEAM_ALLIANCE].GetPlayerCount() && m_SelectionPools[BG_TEAM_HORDE].GetPlayerCount())
        {
            Battleground* arena = sBattlegroundMgr->CreateNewBattleground(bgTypeId, bracketEntry, arenaType, true);
            if (!arena)
            {
                sLog->outError("BattlegroundQueue::Update couldn't create arena instance for rated arena match!");
                return;
            }

            (*(itr_team[BG_TEAM_ALLIANCE]))->OpponentsTeamRating = (*(itr_team[BG_TEAM_HORDE]))->ArenaTeamRating;
            (*(itr_team[BG_TEAM_ALLIANCE]))->OpponentsMatchmakerRating = (*(itr_team[BG_TEAM_HORDE]))->ArenaMatchmakerRating;
            sLog->outDebug("setting oposite teamrating for team %u to %u", (*(itr_team[BG_TEAM_ALLIANCE]))->ArenaTeamId, (*(itr_team[BG_TEAM_ALLIANCE]))->OpponentsTeamRating);
            (*(itr_team[BG_TEAM_HORDE]))->OpponentsTeamRating = (*(itr_team[BG_TEAM_ALLIANCE]))->ArenaTeamRating;
            (*(itr_team[BG_TEAM_HORDE]))->OpponentsMatchmakerRating = (*(itr_team[BG_TEAM_ALLIANCE]))->ArenaMatchmakerRating;
            sLog->outDebug("setting oposite teamrating for team %u to %u", (*(itr_team[BG_TEAM_HORDE]))->ArenaTeamId, (*(itr_team[BG_TEAM_HORDE]))->OpponentsTeamRating);
            // now we must move team if we changed its faction to another faction queue, because then we will spam log by errors in Queue::RemovePlayer
            if ((*(itr_team[BG_TEAM_ALLIANCE]))->Team != ALLIANCE)
            {
                // add to alliance queue
                m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].push_front(*(itr_team[BG_TEAM_ALLIANCE]));
                // erase from horde queue
                m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].erase(itr_team[BG_TEAM_ALLIANCE]);
                itr_team[BG_TEAM_ALLIANCE] = m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].begin();
            }
            if ((*(itr_team[BG_TEAM_HORDE]))->Team != HORDE)
            {
                m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].push_front(*(itr_team[BG_TEAM_HORDE]));
                m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].erase(itr_team[BG_TEAM_HORDE]);
                itr_team[BG_TEAM_HORDE] = m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].begin();
            }

            arena->SetArenaMatchmakerRating(ALLIANCE, (*(itr_team[BG_TEAM_ALLIANCE]))->ArenaMatchmakerRating);
            arena->SetArenaMatchmakerRating(HORDE, (*(itr_team[BG_TEAM_HORDE]))->ArenaMatchmakerRating);
            InviteGroupToBG(*(itr_team[BG_TEAM_ALLIANCE]), arena, ALLIANCE);
            InviteGroupToBG(*(itr_team[BG_TEAM_HORDE]), arena, HORDE);

            sLog->outDebug("Starting rated arena match!");

            arena->StartBattleground(m_twink);
        }
    }
    else if (bg_template->isBattleground() && isRated)
    {
        if (m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].size()+m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].size() <= 1)
            return;

        GroupsQueueType tmpQueue;

        if (!m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].empty())
            tmpQueue.insert(tmpQueue.begin(), m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].begin(), m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_ALLIANCE].end());
        if (!m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].empty())
            tmpQueue.insert(tmpQueue.begin(), m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].begin(), m_QueuedGroups[bracket_id][BG_QUEUE_PREMADE_HORDE].end());

        GroupQueueInfoPtr firstTeam = nullptr;
        GroupQueueInfoPtr opponentTeam = nullptr;

        uint32 ratingDiff = sWorld->getIntConfig(CONFIG_RATED_BATTLEGROUND_MAX_RATING_DIFFERENCE);
        uint32 minRating = std::max(int32(arenaRating - ratingDiff), 0);
        uint32 maxRating = arenaRating + ratingDiff;
        uint32 discardTime = sWorld->getIntConfig(CONFIG_RATED_BATTLEGROUND_RATING_DISCARD_TIMER);

        // if this is 0, it is called from regular update, if non zero, then it is called directly from join opcode and holds team rating
        if (!arenaRating)
        {
            // first find the group which is longest in queue
            for (GroupsQueueType::iterator itr = tmpQueue.begin(); itr != tmpQueue.end(); ++itr)
            {
                if (!(*itr)->IsInvitedToBGInstanceGUID)
                    if (!firstTeam || (*itr)->JoinTime < firstTeam->JoinTime)
                    {
                        firstTeam = *itr;
                        break;
                    }
            }
        }
        else
        {
            // find the team which is in range with this rating and joined first
            for (GroupsQueueType::iterator itr = tmpQueue.begin(); itr != tmpQueue.end(); ++itr)
            {
                if (!(*itr)->IsInvitedToBGInstanceGUID && ((*itr)->ArenaTeamRating >= minRating || (*itr)->ArenaTeamRating <= maxRating))
                    if (!firstTeam || (*itr)->JoinTime < firstTeam->JoinTime)
                    {
                        firstTeam = *itr;
                        break;
                    }
            }

            // compare it with the horde's team that is longest in queue and fits into rating requirement
            for (GroupsQueueType::iterator itr = tmpQueue.begin(); itr != tmpQueue.end(); ++itr)
            {
                if (!(*itr)->IsInvitedToBGInstanceGUID && ((*itr)->ArenaTeamRating >= minRating || (*itr)->ArenaTeamRating <= maxRating)
                    && (!firstTeam || (*itr)->JoinTime < firstTeam->JoinTime))
                {
                    firstTeam = *itr;
                    break;
                }
            }
        }

        // there is no one in queue
        if (!firstTeam)
            return;

        // recalculate min/max rating
        arenaRating = firstTeam->ArenaTeamRating;
        minRating = std::max(int32(arenaRating - ratingDiff), 0);
        maxRating = arenaRating + ratingDiff;

        // find the opponent for this team
        bool canIgnoreRatingDiff;

        for (GroupsQueueType::iterator itr = tmpQueue.begin(); itr != tmpQueue.end(); ++itr)
        {
            // only if both teams are long enough in queue, we can ignore the difference
            if ((canIgnoreRatingDiff = ((firstTeam->JoinTime + discardTime) < getMSTime())))
                canIgnoreRatingDiff = (((*itr)->JoinTime + discardTime) < getMSTime());

            if (*itr != firstTeam && !(*itr)->IsInvitedToBGInstanceGUID && ((*itr)->ArenaTeamRating >= minRating || (*itr)->ArenaTeamRating <= maxRating || canIgnoreRatingDiff))
            {
                opponentTeam = *itr;
                break;
            }
        }

        // no opponent found
        if (!opponentTeam)
            return;

        Battleground *bg = sBattlegroundMgr->CreateNewBattleground(bgTypeId, bracketEntry, arenaType, isRated);
        if (!bg)
        {
            sLog->outError("BattlegroundQueue::Update failed to create rated battleground!");
            return;
        }

        bg->SetArenaMatchmakerRating(ALLIANCE, firstTeam->ArenaMatchmakerRating);
        bg->SetArenaMatchmakerRating(HORDE, opponentTeam->ArenaMatchmakerRating);
        InviteGroupToBG(firstTeam, bg, ALLIANCE);
        InviteGroupToBG(opponentTeam, bg, HORDE);

        bg->StartBattleground(m_twink);
    }
}

/*********************************************************/
/***            BATTLEGROUND QUEUE EVENTS              ***/
/*********************************************************/

bool BGQueueInviteEvent::Execute(uint64 /*e_time*/, uint32 /*p_time*/)
{
    Player* plr = sObjectMgr->GetPlayer(m_PlayerGuid);
    // player logged off (we should do nothing, he is correctly removed from queue in another procedure)
    if (!plr)
        return true;

    Battleground* bg = sBattlegroundMgr->GetBattleground(m_BgInstanceGUID, m_BgTypeId);
    //if battleground ended and its instance deleted - do nothing
    if (!bg)
        return true;

    BattlegroundQueueTypeId bgQueueTypeId = BattlegroundMgr::BGQueueTypeId(bg->GetTypeID(), bg->GetArenaType());
    uint32 queueSlot = plr->GetBattlegroundQueueIndex(bgQueueTypeId);
    if (queueSlot < PLAYER_MAX_BATTLEGROUND_QUEUES)         // player is in queue or in battleground
    {
        // check if player is invited to this bg
        BattlegroundQueue &bgQueue = sBattlegroundMgr->m_BattlegroundQueues[bgQueueTypeId][plr->GetTwinkType()];
        if (bgQueue.IsPlayerInvited(m_PlayerGuid, m_BgInstanceGUID, m_RemoveTime))
        {
            WorldPacket data;
            //we must send remaining time in queue
            sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, plr, queueSlot, STATUS_WAIT_JOIN, INVITE_ACCEPT_WAIT_TIME - INVITATION_REMIND_TIME, plr->GetBattlegroundQueueJoinTime(m_BgTypeId), m_ArenaType);
            plr->GetSession()->SendPacket(&data);
        }
    }
    return true;                                            //event will be deleted
}

void BGQueueInviteEvent::Abort(uint64 /*e_time*/)
{
    //do nothing
}

/*
    this event has many possibilities when it is executed:
    1. player is in battleground (he clicked enter on invitation window)
    2. player left battleground queue and he isn't there any more
    3. player left battleground queue and he joined it again and IsInvitedToBGInstanceGUID = 0
    4. player left queue and he joined again and he has been invited to same battleground again -> we should not remove him from queue yet
    5. player is invited to bg and he didn't choose what to do and timer expired - only in this condition we should call queue::RemovePlayer
    we must remove player in the 5. case even if battleground object doesn't exist!
*/
bool BGQueueRemoveEvent::Execute(uint64 /*e_time*/, uint32 /*p_time*/)
{
    Player* plr = sObjectMgr->GetPlayer(m_PlayerGuid);
    if (!plr)
        // player logged off (we should do nothing, he is correctly removed from queue in another procedure)
        return true;

    Battleground* bg = sBattlegroundMgr->GetBattleground(m_BgInstanceGUID, m_BgTypeId);
    //battleground can be deleted already when we are removing queue info
    //bg pointer can be NULL! so use it carefully!

    uint32 queueSlot = plr->GetBattlegroundQueueIndex(m_BgQueueTypeId);
    if (queueSlot < PLAYER_MAX_BATTLEGROUND_QUEUES)         // player is in queue, or in Battleground
    {
        // check if player is in queue for this BG and if we are removing his invite event
        int twink = BATTLEGROUND_NOTWINK;
        if (plr->IsWargameForBattlegroundQueueType(m_BgQueueTypeId))
            twink = BATTLEGROUND_WARGAME;
        else
            twink = plr->GetTwinkType();

        BattlegroundQueue &bgQueue = sBattlegroundMgr->m_BattlegroundQueues[m_BgQueueTypeId][twink];
        if (bgQueue.IsPlayerInvited(m_PlayerGuid, m_BgInstanceGUID, m_RemoveTime))
        {
            sLog->outDebug("Battleground: removing player %u from bg queue for instance %u because of not pressing enter battle in time.",plr->GetGUIDLow(),m_BgInstanceGUID);

            uint8 arenaSize = 1;
            if (GroupQueueInfoPtr ginfo = bgQueue.GetPlayerGroupInfoData(m_PlayerGuid))
                arenaSize = ginfo->ArenaType;

            plr->RemoveBattlegroundQueueId(m_BgQueueTypeId);
            bgQueue.RemovePlayer(m_PlayerGuid, true);
            //update queues if battleground isn't ended
            if (bg && bg->isBattleground() && bg->GetStatus() != STATUS_WAIT_LEAVE)
                sBattlegroundMgr->ScheduleQueueUpdate(0, 0, m_BgQueueTypeId, m_BgTypeId, bg->GetBracketId(), twink);

            WorldPacket data;
            sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bg, plr, queueSlot, STATUS_NONE, plr->GetBattlegroundQueueJoinTime(m_BgTypeId), 0, arenaSize);
            plr->GetSession()->SendPacket(&data);
        }
    }

    //event will be deleted
    return true;
}

void BGQueueRemoveEvent::Abort(uint64 /*e_time*/)
{
    //do nothing
}
