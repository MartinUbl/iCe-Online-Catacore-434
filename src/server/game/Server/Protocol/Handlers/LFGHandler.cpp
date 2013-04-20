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
#include "WorldSession.h"
#include "WorldPacket.h"
#include "DBCStores.h"
#include "Player.h"
#include "Group.h"
#include "LFGMgr.h"
#include "ObjectMgr.h"


void BuildPlayerLockDungeonBlock(WorldPacket& data, const LfgLockMap& lock)
{
    data << uint32(lock.size());                           // Size of lock dungeons
    for (LfgLockMap::const_iterator it = lock.begin(); it != lock.end(); ++it)
    {
        data << uint32(it->first);                          // Dungeon entry (id + type)
        data << uint32(it->second);                         // Lock status
        data << uint32(0);                                  // 4.2.2
        data << uint32(0);                                  // 4.2.2
    }
}

void BuildPartyLockDungeonBlock(WorldPacket& data, const LfgLockPartyMap& lockMap)
{
    data << uint8(lockMap.size());
    for (LfgLockPartyMap::const_iterator it = lockMap.begin(); it != lockMap.end(); ++it)
    {
        data << uint64(it->first);                         // Player guid
        BuildPlayerLockDungeonBlock(data, it->second);
    }
}

void WorldSession::HandleLfgJoinOpcode(WorldPacket& recv_data)
{
    if (!sWorld->getBoolConfig(CONFIG_DUNGEON_FINDER_ENABLE) ||
        (GetPlayer()->GetGroup() && GetPlayer()->GetGroup()->GetLeaderGUID() != GetPlayer()->GetGUID()))
    {
        recv_data.rpos(recv_data.wpos());
        return;
    }

    uint32 i;
    uint32 roles, unk[3], dungeonCount;
    uint8 commentLength;
    std::string comment;

    recv_data >> roles;

    for (i = 0; i < 3; i++)
        recv_data >> unk[i];

    commentLength = recv_data.ReadBits(9);

    dungeonCount = recv_data.ReadBits(24);
    recv_data.FlushBits();

    if (commentLength)
        recv_data.ReadString(commentLength);

    uint32 dungeon;
    LfgDungeonSet newDungeons;
    for (uint8 i = 0 ; i < dungeonCount; ++i)
    {
        recv_data >> dungeon;
        newDungeons.insert((dungeon & 0x00FFFFFF));       // remove the type from the dungeon entry
    }

    recv_data.read_skip<uint8>(); // almost always set to 128

    sLog->outDebug("CMSG_LFG_JOIN [" UI64FMTD "] roles: %u, Dungeons: %u", GetPlayer()->GetGUID(), roles, uint8(newDungeons.size()));
    sLFGMgr->Join(GetPlayer(), uint8(roles), newDungeons, "");
}

void WorldSession::HandleLfgLeaveOpcode(WorldPacket&  /*recv_data*/)
{
    Group* grp = GetPlayer()->GetGroup();

    sLog->outDebug("CMSG_LFG_LEAVE [" UI64FMTD "] in group: %u", GetPlayer()->GetGUID(), grp ? 1 : 0);

    // Check cheating - only leader can leave the queue
    if (!grp || grp->GetLeaderGUID() == GetPlayer()->GetGUID())
        sLFGMgr->Leave(GetPlayer(), grp);

    SendLfgQueueStatus(0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

void WorldSession::HandleLfgProposalResultOpcode(WorldPacket& recv_data)
{
    uint32 lfgGroupID;                                     // Internal lfgGroupID
    bool accept;                                           // Accept to join?

    recv_data >> lfgGroupID;
    recv_data.read_skip<int32>();   // time
    recv_data.read_skip<uint32>();  // roles
    recv_data.read_skip<uint32>();  // unk

    ObjectGuid guid1;
    guid1[4] = recv_data.ReadBit();
    guid1[5] = recv_data.ReadBit();
    guid1[0] = recv_data.ReadBit();
    guid1[6] = recv_data.ReadBit();
    guid1[2] = recv_data.ReadBit();
    guid1[7] = recv_data.ReadBit();
    guid1[1] = recv_data.ReadBit();
    guid1[3] = recv_data.ReadBit();

    recv_data.ReadByteSeq(guid1[7]);
    recv_data.ReadByteSeq(guid1[4]);
    recv_data.ReadByteSeq(guid1[3]);
    recv_data.ReadByteSeq(guid1[2]);
    recv_data.ReadByteSeq(guid1[6]);
    recv_data.ReadByteSeq(guid1[0]);
    recv_data.ReadByteSeq(guid1[1]);
    recv_data.ReadByteSeq(guid1[5]);

    ObjectGuid guid2;
    guid2[7] = recv_data.ReadBit();
    accept = recv_data.ReadBit();
    guid2[1] = recv_data.ReadBit();
    guid2[3] = recv_data.ReadBit();
    guid2[0] = recv_data.ReadBit();
    guid2[5] = recv_data.ReadBit();
    guid2[4] = recv_data.ReadBit();
    guid2[6] = recv_data.ReadBit();
    guid2[2] = recv_data.ReadBit();

    recv_data.ReadByteSeq(guid2[7]);
    recv_data.ReadByteSeq(guid2[1]);
    recv_data.ReadByteSeq(guid2[5]);
    recv_data.ReadByteSeq(guid2[6]);
    recv_data.ReadByteSeq(guid2[3]);
    recv_data.ReadByteSeq(guid2[4]);
    recv_data.ReadByteSeq(guid2[0]);
    recv_data.ReadByteSeq(guid2[2]);

    sLog->outDebug("CMSG_LFG_PROPOSAL_RESULT [" UI64FMTD "] proposal: %u accept: %u", GetPlayer()->GetGUID(), lfgGroupID, accept ? 1 : 0);
    sLFGMgr->UpdateProposal(lfgGroupID, GetPlayer()->GetGUID(), accept);
}

void WorldSession::HandleLfgSetRolesOpcode(WorldPacket& recv_data)
{
    uint8 roles;
    recv_data >> roles;                                    // Player Group Roles
    uint64 guid = GetPlayer()->GetGUID();
    Group* grp = GetPlayer()->GetGroup();
    if (!grp)
    {
        sLog->outDebug("CMSG_LFG_SET_ROLES [" UI64FMTD "] Not in group", guid);
        return;
    }
    uint64 gguid = grp->GetGUID();
    sLog->outDebug("CMSG_LFG_SET_ROLES: Group [" UI64FMTD "], Player [" UI64FMTD "], Roles: %u", gguid, guid, roles);
    sLFGMgr->UpdateRoleCheck(gguid, guid, roles);
}

void WorldSession::HandleLfgSetCommentOpcode(WorldPacket&  recv_data)
{
    uint32 length;
    std::string comment;

    length = recv_data.ReadBits(9);
    if (length)
        comment = recv_data.ReadString(length);

    uint64 guid = GetPlayer()->GetGUID();
    sLog->outDebug("CMSG_SET_LFG_COMMENT [" UI64FMTD "] comment: %s", guid, comment.c_str());

    sLFGMgr->SetComment(guid, comment);
}

void WorldSession::HandleLfgSetBootVoteOpcode(WorldPacket& recv_data)
{
    bool agree;                                            // Agree to kick player
    recv_data >> agree;

    sLog->outDebug("CMSG_LFG_SET_BOOT_VOTE [" UI64FMTD "] agree: %u", GetPlayer()->GetGUID(), agree ? 1 : 0);
    sLFGMgr->UpdateBoot(GetPlayer(), agree);
}

void WorldSession::HandleLfgTeleportOpcode(WorldPacket& recv_data)
{
    uint8 out;
    recv_data >> out;

    sLog->outDebug("CMSG_LFG_TELEPORT [" UI64FMTD "] out: %u", GetPlayer()->GetGUID(), out ? 1 : 0);
    sLFGMgr->TeleportPlayer(GetPlayer(), out, true);
}

void WorldSession::HandleLfgPlayerLockInfoRequestOpcode(WorldPacket& recv_data)
{
    uint64 guid = GetPlayer()->GetGUID();
    sLog->outDebug("CMSG_LFD_PLAYER_LOCK_INFO_REQUEST [" UI64FMTD "]", guid);

    // Get Random dungeons that can be done at a certain level and expansion
    // FIXME - Should return seasonals (when not disabled)
    LfgDungeonSet randomDungeons;
    uint8 level = GetPlayer()->getLevel();
    uint8 expansion = GetPlayer()->GetSession()->Expansion();
    for (uint32 i = 0; i < sLFGDungeonStore.GetNumRows(); ++i)
    {
        LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(i);
        if (dungeon && dungeon->type == LFG_TYPE_RANDOM && dungeon->expansion <= expansion &&
            dungeon->minlevel <= level && level <= dungeon->maxlevel)
            randomDungeons.insert(dungeon->Entry());
    }

    // Get player locked Dungeons
    LfgLockMap lock = sLFGMgr->GetLockedDungeons(guid);
    uint32 rsize = uint32(randomDungeons.size());
    uint32 lsize = uint32(lock.size());

    sLog->outDebug("SMSG_LFG_PLAYER_INFO [" UI64FMTD "]", guid);
    WorldPacket data(SMSG_LFG_PLAYER_INFO, 1 + rsize * (4 + 1 + 4 + 4 + 4 + 4 + 1 + 4 + 4 + 4) + 4 + lsize * (1 + 4 + 4 + 4 + 4 + 1 + 4 + 4 + 4));

    data << uint8(randomDungeons.size());                  // Random Dungeon count
    for (LfgDungeonSet::const_iterator it = randomDungeons.begin(); it != randomDungeons.end(); ++it)
    {
        data << uint32(*it);                               // Dungeon Entry (id + type)
        LfgReward const* reward = sLFGMgr->GetRandomDungeonReward(*it, level);
        Quest const* qRew = NULL;
        uint8 done = 0;
        if (reward)
        {
            qRew = sObjectMgr->GetQuestTemplate(reward->reward[0].questId);
            if (qRew)
            {
                done = !GetPlayer()->CanRewardQuest(qRew, false);
                if (done)
                    qRew = sObjectMgr->GetQuestTemplate(reward->reward[1].questId);
            }
        }

        // We don't know exactly, what is the first par of the packet for. It seems to work fine only with the second part
        // of packet implemented. If you uncomment this block, you will probably get wrong currency/item rewards by every
        // dungeon.

        /*if (qRew)
        {
            data << uint8(done);
            data << uint32(0);
            data << uint32(0);

            data << uint32(0); // uint32(reward->reward[done].variableMoney)
            data << uint32(0); // uint32(reward->reward[done].variableXP)

            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);

            uint8 currCount = 0;
            for (uint8 k = 0; k < QUEST_REWARDS_COUNT; k++)
            {
                if (qRew->RewCurrencyId[k] > 0)
                    currCount++;
                if (qRew->RewItemId[k] > 0)
                    currCount++;
            }

            data << uint8(0); // some kind of boolean field

            for (uint32 j = 0; j < 3; j++)
            {
                data << uint32(currCount);
                if (currCount)
                {
                    data << uint32(qRew->GetRewOrReqMoney());
                    data << uint32(qRew->XPValue(GetPlayer()));

                    size_t pos = data.wpos();
                    uint8 rewCount = 0;
                    data << uint8(0);

                    for (uint8 i = 0; i < QUEST_REWARDS_COUNT; ++i)
                    {
                        if (qRew->RewCurrencyId[i])
                        {
                            data << uint32(qRew->RewCurrencyId[i]);
                            data << uint32(qRew->RewCurrencyId[i]);
                            data << uint32(qRew->RewCurrencyCount[i] * GetCurrencyPrecision(qRew->RewCurrencyId[i]));

                            data << uint8(1);  // is currency

                            rewCount++;
                        }
                        if (qRew->RewItemId[i])
                        {
                            data << uint32(qRew->RewItemId[i]);
                            data << uint32(qRew->RewItemId[i]);
                            data << uint32(qRew->RewItemCount[i]);

                            data << uint8(0);  // is currency

                            rewCount++;
                        }
                    }

                    data.put<uint8>(pos, rewCount);
                }
            }
        }
        else*/
        {
            data << uint8(done);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint32(0);
            data << uint8(0);

            for (uint32 j = 0; j < 3; j++)
            {
                data << uint32(0);
            }
        }

        uint8 secRewCount = 0;

        if (qRew)
        {
            data << uint32(qRew->GetRewOrReqMoney());
            data << uint32(qRew->XPValue(GetPlayer()));
        }
        else
        {
            data << uint32(0);
            data << uint32(0);
        }

        uint8 pos = data.wpos();
        data << uint8(secRewCount);

        if (qRew)
        {
            for (uint8 i = 0; i < QUEST_REWARDS_COUNT; ++i)
            {
                if (qRew->RewCurrencyId[i])
                {
                    data << uint32(qRew->RewCurrencyId[i]);
                    data << uint32(qRew->RewCurrencyId[i]);
                    data << uint32(qRew->RewCurrencyCount[i] * GetCurrencyPrecision(qRew->RewCurrencyId[i]));

                    data << uint8(1);  // is currency

                    secRewCount++;
                }
                if (qRew->RewItemId[i])
                {
                    data << uint32(qRew->RewItemId[i]);
                    data << uint32(qRew->RewItemId[i]);
                    data << uint32(qRew->RewItemCount[i]);

                    data << uint8(0);  // is currency

                    secRewCount++;
                }
            }
        }

        data.put<uint8>(pos, secRewCount);
    }
    BuildPlayerLockDungeonBlock(data, lock);
    SendPacket(&data);
}

void WorldSession::HandleLfgPartyLockInfoRequestOpcode(WorldPacket&  /*recv_data*/)
{
    uint64 guid = GetPlayer()->GetGUID();
    sLog->outDebug("CMSG_LFD_PARTY_LOCK_INFO_REQUEST [" UI64FMTD "]", guid);

    Group* grp = GetPlayer()->GetGroup();
    if (!grp)
        return;

    // Get the locked dungeons of the other party members
    LfgLockPartyMap lockMap;
    for (GroupReference* itr = grp->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* plrg = itr->getSource();
        if (!plrg)
            continue;

        uint64 pguid = plrg->GetGUID();
        if (pguid == guid)
            continue;

        lockMap[pguid] = sLFGMgr->GetLockedDungeons(pguid);
    }

    uint32 size = 0;
    for (LfgLockPartyMap::const_iterator it = lockMap.begin(); it != lockMap.end(); ++it)
        size += 8 + 4 + uint32(it->second.size()) * (4 + 4);
    
    sLog->outDebug("SMSG_LFG_PARTY_INFO [" UI64FMTD "]", guid);
    WorldPacket data(SMSG_LFG_PARTY_INFO, 1 + size);
    BuildPartyLockDungeonBlock(data, lockMap);
    SendPacket(&data);
}

void WorldSession::HandleLfrSearchOpcode(WorldPacket& recv_data)
{
    uint32 entry;                                          // Raid id to search
    recv_data >> entry;
    sLog->outDebug("CMSG_SEARCH_LFG_JOIN [" UI64FMTD "] dungeon entry: %u", GetPlayer()->GetGUID(), entry);
    //SendLfrUpdateListOpcode(entry);
}

void WorldSession::HandleLfrLeaveOpcode(WorldPacket& recv_data)
{
    uint32 dungeonId;                                      // Raid id queue to leave
    recv_data >> dungeonId;
    sLog->outDebug("CMSG_SEARCH_LFG_LEAVE [" UI64FMTD "] dungeonId: %u", GetPlayer()->GetGUID(), dungeonId);
    //sLFGMgr->LeaveLfr(GetPlayer(), dungeonId);
}

void WorldSession::SendLfgUpdateStatus(const LfgUpdateData &updateData)
{
    bool join = false;
    bool queued = false;
    bool lfgjoined = false;
    bool silent = false;

    switch(updateData.updateType)
    {
        case LFG_UPDATETYPE_JOIN_PROPOSAL:
            join = true;
            lfgjoined = true;
            queued = true;
            break;
        case LFG_UPDATETYPE_ADDED_TO_QUEUE:
            join = true;
            queued = true;
            break;
        case LFG_UPDATETYPE_CLEAR_LOCK_LIST:
            // join = true;  // TODO: Sometimes queued and extrainfo - Check ocurrences...
            queued = true;
            break;
        case LFG_UPDATETYPE_PROPOSAL_BEGIN:
            join = true;
            lfgjoined = true;
            queued = true;
            break;
        default:
            break;
    }

    ObjectGuid guid = GetPlayer()->GetGUID();
    uint8 size = uint8(updateData.dungeons.size());

    WorldPacket data(SMSG_LFG_UPDATE_STATUS);

    data.WriteBit(guid[1]);
    data.WriteBit(silent);       // unk, "silent" ? If true, doesn't show minimap icon
    data.WriteBits(size, 24);
    data.WriteBit(guid[6]);
    data.WriteBit(join);
    data.WriteBits(updateData.comment.size(), 9);
    data.WriteBit(guid[4]);
    data.WriteBit(guid[7]);
    data.WriteBit(guid[2]);
    data.WriteBit(lfgjoined);
    data.WriteBit(guid[0]);
    data.WriteBit(guid[3]);
    data.WriteBit(guid[5]);
    data.WriteBit(queued);
    data.FlushBits();

    data << uint8(-1);          // unk

    if (updateData.comment.size() > 0)
        data.WriteString(updateData.comment);

    data << uint32(1);         // queue id
    data << int32(time(NULL)); // join date

    data.WriteByteSeq(guid[6]);

    for (uint32 i = 0; i < 3; i++)
        data << uint8(-1);

    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[4]);
    data.WriteByteSeq(guid[3]);
    data.WriteByteSeq(guid[5]);
    data.WriteByteSeq(guid[0]);

    data << uint32(3);             // unk

    data.WriteByteSeq(guid[7]);

    if (size)
        for (LfgDungeonSet::const_iterator it = updateData.dungeons.begin(); it != updateData.dungeons.end(); ++it)
            data << uint32(*it);

    SendPacket(&data);
}

void WorldSession::SendLfgRoleChosen(uint64 guid, uint8 roles)
{
    sLog->outDebug("SMSG_LFG_ROLE_CHOSEN [" UI64FMTD "] guid: [" UI64FMTD "] roles: %u", GetPlayer()->GetGUID(), guid, roles);

    WorldPacket data(SMSG_LFG_ROLE_CHOSEN, 8 + 1 + 4);
    data << uint64(guid);                                  // Guid
    data << uint8(roles > 0);                              // Ready
    data << uint32(roles);                                 // Roles
    SendPacket(&data);
}

void WorldSession::SendLfgRoleCheckUpdate(const LfgRoleCheck* pRoleCheck)
{
    ASSERT(pRoleCheck);
    LfgDungeonSet dungeons;
    if (pRoleCheck->rDungeonId)
        dungeons.insert(pRoleCheck->rDungeonId);
    else
        dungeons = pRoleCheck->dungeons;

    sLog->outDebug("SMSG_LFG_ROLE_CHECK_UPDATE [" UI64FMTD "]", GetPlayer()->GetGUID());
    WorldPacket data(SMSG_LFG_ROLE_CHECK_UPDATE, 4 + 1 + 1 + dungeons.size() * 4 + 1 + pRoleCheck->roles.size() * (8 + 1 + 4 + 1));

    data << uint32(pRoleCheck->state);                     // Check result
    data << uint8(pRoleCheck->state == LFG_ROLECHECK_INITIALITING);
    data << uint8(dungeons.size());                        // Number of dungeons
    if (dungeons.size())
    {
        for (LfgDungeonSet::iterator it = dungeons.begin(); it != dungeons.end(); ++it)
        {
            LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(*it);
            data << uint32(dungeon ? dungeon->Entry() : 0); // Dungeon
        }
    }

    data << uint8(pRoleCheck->roles.size());               // Players in group
    if (pRoleCheck->roles.size())
    {
        // Leader info MUST be sent 1st :S
        uint64 guid = pRoleCheck->leader;
        uint8 roles = pRoleCheck->roles.find(guid)->second;
        data << uint64(guid);                              // Guid
        data << uint8(roles > 0);                          // Ready
        data << uint32(roles);                             // Roles
        Player* plr = sObjectMgr->GetPlayer(guid);
        data << uint8(plr ? plr->getLevel() : 0);          // Level

        for (LfgRolesMap::const_iterator it = pRoleCheck->roles.begin(); it != pRoleCheck->roles.end(); ++it)
        {
            if (it->first == pRoleCheck->leader)
                continue;

            guid = it->first;
            roles = it->second;
            data << uint64(guid);                          // Guid
            data << uint8(roles > 0);                      // Ready
            data << uint32(roles);                         // Roles
            plr = sObjectMgr->GetPlayer(guid);
            data << uint8(plr ? plr->getLevel() : 0);      // Level
        }
    }
    SendPacket(&data);
}

void WorldSession::SendLfgJoinResult(const LfgJoinResultData& joinData)
{
    uint32 size = 0;
    for (LfgLockPartyMap::const_iterator it = joinData.lockmap.begin(); it != joinData.lockmap.end(); ++it)
        size += 8 + 4 + uint32(it->second.size()) * (4 + 4);

    // IDA address: 0x006F7C20

    sLog->outDebug("SMSG_LFG_JOIN_RESULT [" UI64FMTD "] checkResult: %u checkValue: %u", GetPlayer()->GetGUID(), joinData.result, joinData.state);
    WorldPacket data(SMSG_LFG_JOIN_RESULT, 4 + 4 + size);

    data << uint32(3); // unk
    data << uint8(joinData.result);
    data << uint32(1); // queue id
    data << uint8(joinData.state);
    data << uint32(time(NULL));

    ObjectGuid plGuid = GetPlayer()->GetGUID();
    data.WriteBit(plGuid[2]);
    data.WriteBit(plGuid[7]);
    data.WriteBit(plGuid[3]);
    data.WriteBit(plGuid[0]);

    uint32 dsize = 0;//joinData.lockmap.size();

    // temp and hack and shit
    // please, fix me
    // without it, it causes ByteBuffer to overflow over 10000000 bytes allocated, and thats wrong
    if (dsize > 50)
        dsize = 50;

    data.WriteBits(dsize, 24);

    /*ObjectGuid tmpGuid;

    uint32 limit = 0;

    for (LfgLockPartyMap::const_iterator it = joinData.lockmap.begin(); it != joinData.lockmap.end(); ++it)
    {
        tmpGuid = it->first;

        data.WriteBit(tmpGuid[7]);
        data.WriteBit(tmpGuid[5]);
        data.WriteBit(tmpGuid[3]);
        data.WriteBit(tmpGuid[6]);
        data.WriteBit(tmpGuid[0]);
        data.WriteBit(tmpGuid[2]);
        data.WriteBit(tmpGuid[4]);
        data.WriteBit(tmpGuid[1]);

        data.WriteBits(it->second.size(), 22);

        limit++;

        if (limit >= dsize)
            break;
    }*/

    data.WriteBit(plGuid[4]);
    data.WriteBit(plGuid[5]);
    data.WriteBit(plGuid[1]);
    data.WriteBit(plGuid[6]);

    /*limit = 0;

    for (LfgLockPartyMap::const_iterator it = joinData.lockmap.begin(); it != joinData.lockmap.end(); ++it)
    {
        tmpGuid = it->first;

        for (LfgLockMap::const_iterator itr = it->second.begin(); itr != it->second.end(); ++it)
        {
            data << uint32(itr->first);                          // Dungeon entry (id + type)
            data << uint32(itr->second);                         // Lock status
            data << uint32(0);                                   // needed avg item level to enter
            data << uint32(0);                                   // player avg item level
        }

        data.WriteByteSeq(tmpGuid[2]);
        data.WriteByteSeq(tmpGuid[5]);
        data.WriteByteSeq(tmpGuid[1]);
        data.WriteByteSeq(tmpGuid[0]);
        data.WriteByteSeq(tmpGuid[4]);
        data.WriteByteSeq(tmpGuid[3]);
        data.WriteByteSeq(tmpGuid[6]);
        data.WriteByteSeq(tmpGuid[7]);

        limit++;

        if (limit >= dsize)
            break;
    }*/

    data.WriteByteSeq(plGuid[1]);
    data.WriteByteSeq(plGuid[4]);
    data.WriteByteSeq(plGuid[3]);
    data.WriteByteSeq(plGuid[5]);
    data.WriteByteSeq(plGuid[0]);
    data.WriteByteSeq(plGuid[7]);
    data.WriteByteSeq(plGuid[2]);
    data.WriteByteSeq(plGuid[6]);

    SendPacket(&data);
}

void WorldSession::SendLfgQueueStatus(uint32 dungeon, int32 waitTime, int32 avgWaitTime, int32 waitTimeTanks, int32 waitTimeHealer, int32 waitTimeDps, uint32 queuedTime, uint8 tanks, uint8 healers, uint8 dps)
{
    sLog->outDebug("SMSG_LFG_QUEUE_STATUS [" UI64FMTD "] dungeon: %u - waitTime: %d - avgWaitTime: %d - waitTimeTanks: %d - waitTimeHealer: %d - waitTimeDps: %d - queuedTime: %u - tanks: %u - healers: %u - dps: %u", GetPlayer()->GetGUID(), dungeon, waitTime, avgWaitTime, waitTimeTanks, waitTimeHealer, waitTimeDps, queuedTime, tanks, healers, dps);

    ObjectGuid guid = GetPlayer()->GetGUID();

    WorldPacket data(SMSG_LFG_QUEUE_STATUS, 52, true);

    data.WriteBit(guid[3]);
    data.WriteBit(guid[0]);
    data.WriteBit(guid[2]);
    data.WriteBit(guid[6]);
    data.WriteBit(guid[5]);
    data.WriteBit(guid[7]);
    data.WriteBit(guid[1]);
    data.WriteBit(guid[4]);

    data.WriteByteSeq(guid[0]);

    data << uint8(tanks);                                  // Tanks needed
    data << int32(waitTimeTanks);                          // Wait Tanks
    data << uint8(healers);                                // Healers needed
    data << int32(waitTimeHealer);                         // Wait Healers
    data << uint8(dps);                                    // Dps needed
    data << int32(waitTimeDps);                            // Wait Dps

    data.WriteByteSeq(guid[4]);
    data.WriteByteSeq(guid[6]);

    data << int32(avgWaitTime);                            // Average Wait time
    data << uint32(time(NULL));                            // join time
    data << uint32(dungeon);                               // Dungeon

    data << uint32(queuedTime);                            // Player wait time in queue

    data.WriteByteSeq(guid[5]);
    data.WriteByteSeq(guid[7]);
    data.WriteByteSeq(guid[3]);

    data << uint32(1);                                     // queue ID ?

    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[2]);

    data << int32(waitTime);                               // Wait Time

    data << uint32(3);                                     // unknown...

    SendPacket(&data);
}

void WorldSession::SendLfgPlayerReward(uint32 rdungeonEntry, uint32 sdungeonEntry, uint8 done, const LfgReward* reward, const Quest* qRew)
{
    if (!rdungeonEntry || !sdungeonEntry || !qRew)
        return;

    uint8 currCount = 0;

    sLog->outDebug("SMSG_LFG_PLAYER_REWARD [" UI64FMTD "] rdungeonEntry: %u - sdungeonEntry: %u - done: %u", GetPlayer()->GetGUID(), rdungeonEntry, sdungeonEntry, done);
    WorldPacket data(SMSG_LFG_PLAYER_REWARD, 4 + 4 + 1 + 4 + 4 + 4 + 4 + 4 + 1 + currCount * (4 + 4 + 4));
    data << uint32(rdungeonEntry);                         // Random Dungeon Finished
    data << uint32(sdungeonEntry);                         // Dungeon Finished
    //data << uint8(done);
    //data << uint32(1);
    data << uint32(qRew->GetRewOrReqMoney());
    data << uint32(qRew->XPValue(GetPlayer()));
    //data << uint32(reward->reward[done].variableMoney);
    //data << uint32(reward->reward[done].variableXP);

    size_t pos = data.wpos();
    data << uint8(currCount);

    for (uint8 i = 0; i < QUEST_REWARDS_COUNT; ++i)
    {
        if (qRew->RewCurrencyId[i])
        {
            data << uint32(qRew->RewCurrencyId[i]);
            data << uint32(0); // displayid
            data << uint32(qRew->RewCurrencyCount[i] * GetCurrencyPrecision(qRew->RewCurrencyId[i]));

            data << uint8(1); // is currency

            currCount++;
        }
        if (qRew->RewItemId[i])
        {
            data << uint32(qRew->RewItemId[i]);
            data << uint32(0); // displayid
            data << uint32(qRew->RewItemCount[i]);

            data << uint8(0); // is currency

            currCount++;
        }
    }

    data.put<uint8>(pos, currCount);

    SendPacket(&data);
}

void WorldSession::SendLfgBootPlayer(const LfgPlayerBoot* pBoot)
{
    uint64 guid = GetPlayer()->GetGUID();
    LfgAnswer playerVote = pBoot->votes.find(guid)->second;
    uint8 votesNum = 0;
    uint8 agreeNum = 0;
    uint32 secsleft = uint8((pBoot->cancelTime - time(NULL)) / 1000);
    for (LfgAnswerMap::const_iterator it = pBoot->votes.begin(); it != pBoot->votes.end(); ++it)
    {
        if (it->second != LFG_ANSWER_PENDING)
        {
            ++votesNum;
            if (it->second == LFG_ANSWER_AGREE)
                ++agreeNum;
        }
    }
    sLog->outDebug("SMSG_LFG_BOOT_PROPOSAL_UPDATE [" UI64FMTD "] inProgress: %u - didVote: %u - agree: %u - victim: [" UI64FMTD "] votes: %u - agrees: %u - left: %u - needed: %u - reason %s",
        guid, uint8(pBoot->inProgress), uint8(playerVote != LFG_ANSWER_PENDING), uint8(playerVote == LFG_ANSWER_AGREE), pBoot->victim, votesNum, agreeNum, secsleft, pBoot->votedNeeded, pBoot->reason.c_str());
    WorldPacket data(SMSG_LFG_BOOT_PROPOSAL_UPDATE, 1 + 1 + 1 + 8 + 4 + 4 + 4 + 4 + pBoot->reason.length());
    data << uint8(pBoot->inProgress);                      // Vote in progress
    data << uint8(playerVote != LFG_ANSWER_PENDING);       // Did Vote
    data << uint8(playerVote == LFG_ANSWER_AGREE);         // Agree
    data << uint8(0);                                      // 4.2.2
    data << uint64(pBoot->victim);                         // Victim GUID
    data << uint32(votesNum);                              // Total Votes
    data << uint32(agreeNum);                              // Agree Count
    data << uint32(secsleft);                              // Time Left
    data << uint32(pBoot->votedNeeded);                    // Needed Votes
    data << pBoot->reason.c_str();                         // Kick reason
    SendPacket(&data);
}

void WorldSession::SendLfgUpdateProposal(uint32 proposalId, const LfgProposal* pProp)
{
    if (!pProp)
        return;

    uint64 guid = GetPlayer()->GetGUID();
    LfgProposalPlayerMap::const_iterator itPlayer = pProp->players.find(guid);
    if (itPlayer == pProp->players.end())                  // Player MUST be in the proposal
        return;

    LfgProposalPlayer* ppPlayer = itPlayer->second;
    uint32 pLowGroupGuid = ppPlayer->groupLowGuid;
    uint32 dLowGuid = pProp->groupLowGuid;
    uint32 dungeonId = pProp->dungeonId;
    bool isSameDungeon = false;
    bool isContinue = false;
    Group* grp = dLowGuid ? sObjectMgr->GetGroupByGUID(dLowGuid) : NULL;
    if (grp)
    {
        uint64 gguid = grp->GetGUID();
        isContinue = grp->isLFGGroup() && sLFGMgr->GetState(gguid) != LFG_STATE_FINISHED_DUNGEON;
        isSameDungeon = GetPlayer()->GetGroup() == grp && isContinue;
    }

    if (!isContinue)                                       // Only show proposal dungeon if it's continue
    {
        LfgDungeonSet playerDungeons = sLFGMgr->GetSelectedDungeons(guid);
        if (playerDungeons.size() == 1)
            dungeonId = (*playerDungeons.begin());
    }
    if (LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(dungeonId))
        dungeonId = dungeon->Entry();

    sLog->outDebug("SMSG_LFG_PROPOSAL_UPDATE [" UI64FMTD "] state: %u", GetPlayer()->GetGUID(), pProp->state);
    WorldPacket data(SMSG_LFG_PROPOSAL_UPDATE, 4 + 1 + 4 + 4 + 1 + 1 + pProp->players.size() * (4 + 1 + 1 + 1 + 1 +1));

    data << uint32(time(NULL));
    data << uint32(0);                                     // Bosses killed - FIXME
    data << int32(0);                                      // unk
    data << uint32(0);                                     // unk
    data << uint32(dungeonId);                             // Dungeon
    data << uint32(proposalId);                            // probably proposalId - client sends it back
    data << uint8(pProp->state);                           // Result state

    // why two guids?!
    ObjectGuid guid1 = GetPlayer()->GetGUID();
    ObjectGuid guid2 = GetPlayer()->GetGUID();

    data.WriteBit(guid2[4]);

    data.WriteBit(guid1[3]);
    data.WriteBit(guid1[7]);
    data.WriteBit(guid1[0]);
    data.WriteBit(guid2[1]);
    data.WriteBit(isSameDungeon); // silend (show client window)
    data.WriteBit(guid1[4]);
    data.WriteBit(guid1[5]);
    data.WriteBit(guid2[3]);

    data.WriteBits(pProp->players.size(), 23);

    data.WriteBit(guid2[7]);

    for (itPlayer = pProp->players.begin(); itPlayer != pProp->players.end(); ++itPlayer)
    {
        ppPlayer = itPlayer->second;

        data.WriteBit(ppPlayer->groupLowGuid == dLowGuid);      // in dungeon
        data.WriteBit(ppPlayer->groupLowGuid == pLowGroupGuid); // same group
        data.WriteBit(ppPlayer->accept == LFG_ANSWER_AGREE);    // accepted
        data.WriteBit(ppPlayer->accept != LFG_ANSWER_PENDING);  // answered
        data.WriteBit(itPlayer->first == guid);                 // self
    }

    data.WriteBit(guid2[5]);
    data.WriteBit(guid1[6]);
    data.WriteBit(guid2[2]);
    data.WriteBit(guid2[6]);
    data.WriteBit(guid1[2]);
    data.WriteBit(guid1[1]);
    data.WriteBit(guid2[0]);

    data.WriteByteSeq(guid1[5]);
    data.WriteByteSeq(guid2[3]);
    data.WriteByteSeq(guid2[6]);
    data.WriteByteSeq(guid1[6]);
    data.WriteByteSeq(guid1[0]);
    data.WriteByteSeq(guid2[5]);
    data.WriteByteSeq(guid1[1]);

    for (itPlayer = pProp->players.begin(); itPlayer != pProp->players.end(); ++itPlayer)
        data << uint32(itPlayer->second->role);

    data.WriteByteSeq(guid2[7]);
    data.WriteByteSeq(guid1[4]);
    data.WriteByteSeq(guid2[0]);
    data.WriteByteSeq(guid2[1]);
    data.WriteByteSeq(guid1[2]);
    data.WriteByteSeq(guid1[7]);
    data.WriteByteSeq(guid2[2]);
    data.WriteByteSeq(guid1[3]);
    data.WriteByteSeq(guid2[4]);

    SendPacket(&data);
}

void WorldSession::SendLfgUpdateSearch(bool update)
{
    sLog->outDebug("SMSG_LFG_UPDATE_LIST [" UI64FMTD "] update: %u", GetPlayer()->GetGUID(), update ? 1 : 0);
    WorldPacket data(SMSG_LFG_UPDATE_LIST, 1);
    data << uint8(update);                                 // In Lfg Queue?
    SendPacket(&data);
}

void WorldSession::SendLfgDisabled()
{
    sLog->outDebug("SMSG_LFG_DISABLED [" UI64FMTD "]", GetPlayer()->GetGUID());
    WorldPacket data(SMSG_LFG_DISABLED, 0);
    SendPacket(&data);
}

void WorldSession::SendLfgOfferContinue(uint32 dungeonEntry)
{
    sLog->outDebug("SMSG_LFG_OFFER_CONTINUE [" UI64FMTD "] dungeon entry: %u", GetPlayer()->GetGUID(), dungeonEntry);
    WorldPacket data(SMSG_LFG_OFFER_CONTINUE, 4);
    data << uint32(dungeonEntry);
    SendPacket(&data);
}

void WorldSession::SendLfgTeleportError(uint8 err)
{
    sLog->outDebug("SMSG_LFG_TELEPORT_DENIED [" UI64FMTD "] reason: %u", GetPlayer()->GetGUID(), err);
    WorldPacket data(SMSG_LFG_TELEPORT_DENIED, 4);
    data << uint32(err);                                   // Error
    SendPacket(&data);
}

/*
void WorldSession::SendLfrUpdateListOpcode(uint32 dungeonEntry)
{
    sLog->outDebug("SMSG_UPDATE_LFG_LIST [" UI64FMTD "] dungeon entry: %u", GetPlayer()->GetGUID(), dungeonEntry);
    WorldPacket data(SMSG_UPDATE_LFG_LIST);
    SendPacket(&data);
}
*/
