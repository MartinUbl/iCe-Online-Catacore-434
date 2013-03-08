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
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Log.h"
#include "Opcodes.h"
#include "Guild.h"
#include "GossipDef.h"
#include "SocialMgr.h"

// Cataclysm TODO:
// Proper update of internal events:
// Promote, Demote, Add or Remove member
// Tabard/Emblem update (so that player doesn't have to relog to see changes)
// Guild Rank Management (adding, deleting ranks, editing ranks' permissions)
// Professions in guild roster, its dynamic updates
// GuildBankRightsAndSlots
//
// Guild Advancement system is currently disabled. Make its disable state configurable and fix it

// Helper for getting guild object of session's player.
// If guild does not exist, sends error (if necessary).
inline Guild* _GetPlayerGuild(WorldSession* session, bool sendError = false)
{
    if (uint32 guildId = session->GetPlayer()->GetGuildId())    // If guild id = 0, player is not in guild
        if (Guild* pGuild = sObjectMgr->GetGuildById(guildId))   // Find guild by id
            return pGuild;
    if (sendError)
        Guild::SendCommandResult(session, GUILD_COMMAND_CREATE, ERR_GUILD_PLAYER_NOT_IN_GUILD);
    return NULL;
}

// Cata Status: Done
void WorldSession::HandleGuildQueryOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug("WORLD: Received CMSG_GUILD_QUERY");

    uint64 guildId;
    uint64 player; //4.0.6a
    recvPacket >> guildId;
    recvPacket >> player;
    // Use received guild id to access guild method (not player's guild id)
    uint32 lowGuildId = GUID_LOPART(guildId);
    if (Guild *pGuild = sObjectMgr->GetGuildById(lowGuildId))
        pGuild->HandleQuery(this);
    else
        Guild::SendCommandResult(this, GUILD_COMMAND_CREATE, ERR_GUILD_PLAYER_NOT_IN_GUILD);
}

// Cata status: Not used.
void WorldSession::HandleGuildCreateOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug("WORLD: Received CMSG_GUILD_CREATE");

    std::string name;
    recvPacket >> name;

    if (!GetPlayer()->GetGuildId())             // Player cannot be in guild
    {
        Guild* pGuild = new Guild();
        if (pGuild->Create(GetPlayer(), name))
            sObjectMgr->AddGuild(pGuild);
        else
            delete pGuild;
    }
}

// Cata Status: Done
// Functional, but there are many unknown values in the SMSG_GUILD_INVITE
// However, until guild leveling system finds some use in emulating, those aren't needed
void WorldSession::HandleGuildInviteOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug("WORLD: Received CMSG_GUILD_INVITE");

    uint32 nameLength = recvPacket.ReadBits(7);
    std::string invitedName = recvPacket.ReadString(nameLength);

    if (normalizePlayerName(invitedName))
        if (Guild* pGuild = GetPlayer()->GetGuild())
            pGuild->HandleInviteMember(this, invitedName);
}

// Cata Status: Done
void WorldSession::HandleGuildRemoveOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug("WORLD: Received CMSG_GUILD_REMOVE");

    ObjectGuid guid;

    guid[6] = recvPacket.ReadBit();
    guid[5] = recvPacket.ReadBit();
    guid[4] = recvPacket.ReadBit();
    guid[0] = recvPacket.ReadBit();
    guid[1] = recvPacket.ReadBit();
    guid[3] = recvPacket.ReadBit();
    guid[7] = recvPacket.ReadBit();
    guid[2] = recvPacket.ReadBit();

    recvPacket.ReadByteSeq(guid[2]);
    recvPacket.ReadByteSeq(guid[6]);
    recvPacket.ReadByteSeq(guid[5]);
    recvPacket.ReadByteSeq(guid[7]);
    recvPacket.ReadByteSeq(guid[1]);
    recvPacket.ReadByteSeq(guid[4]);
    recvPacket.ReadByteSeq(guid[3]);
    recvPacket.ReadByteSeq(guid[0]);

    if (Guild* pGuild = _GetPlayerGuild(this, true))
        pGuild->HandleRemoveMember(this, guid);
}

// Cata Status: Done
void WorldSession::HandleGuildAcceptOpcode(WorldPacket& /*recvPacket*/)
{
    sLog->outDebug("WORLD: Received CMSG_GUILD_ACCEPT");

    // Player cannot be in guild
    if (!GetPlayer()->GetGuildId())
        // Guild where player was invited must exist
        if (Guild* pGuild = sObjectMgr->GetGuildById(GetPlayer()->GetGuildIdInvited()))
            pGuild->HandleAcceptMember(this);
}

// Cata Status: Done
void WorldSession::HandleGuildDeclineOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug("WORLD: Received CMSG_GUILD_DECLINE");

    GetPlayer()->SetGuildIdInvited(0);
    GetPlayer()->SetInGuild(0);
}

// Cata Status: Done
void WorldSession::HandleGuildInfoOpcode(WorldPacket& /*recvPacket*/)
{
    sLog->outDebug("WORLD: Received CMSG_GUILD_INFO");

    if (Guild* pGuild = _GetPlayerGuild(this, true))
        pGuild->SendInfo(this);
}

// CATA Status: Done
void WorldSession::HandleGuildRosterOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug("WORLD: Received CMSG_GUILD_ROSTER");
    recvPacket.rfinish();

    if (Guild* pGuild = _GetPlayerGuild(this))
        pGuild->HandleRoster(this);
}

// Cata Status: Done
// TODO!!! The update is improper. Shouldn't re-send the roster - it causes visual sprinkles
void WorldSession::HandleGuildPromoteOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug("WORLD: Received CMSG_GUILD_PROMOTE");

    uint64 guid;
    recvPacket >> guid; // target guid
    recvPacket.read_skip<uint64>(); // command issuer's guid?

    if (Guild* pGuild = _GetPlayerGuild(this, true))
        pGuild->HandleUpdateMemberRank(this, guid, false);
}

// Cata Status: Done
// TODO!!! The update is improper. Shouldn't re-send the roster - it causes visual sprinkles
void WorldSession::HandleGuildDemoteOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug("WORLD: Received CMSG_GUILD_DEMOTE");

    uint64 guid;
    recvPacket >> guid; // target guid
    recvPacket.read_skip<uint64>(); // command issuer's guid?
       
    if (Guild* pGuild = _GetPlayerGuild(this, true))
        pGuild->HandleUpdateMemberRank(this, guid, true);
}

// Cata Status: Done
void WorldSession::HandleGuildLeaveOpcode(WorldPacket& /*recvPacket*/)
{
    sLog->outDebug("WORLD: Received CMSG_GUILD_LEAVE");

    if (Guild* pGuild = _GetPlayerGuild(this, true))
        pGuild->HandleLeaveMember(this);
}

void WorldSession::HandleGuildAssignRankOpcode(WorldPacket& recvPacket)
{
    ObjectGuid targetGuid;
    ObjectGuid setterGuid;

    uint32 rankId;
    recvPacket >> rankId;

    targetGuid[1] = recvPacket.ReadBit();
    targetGuid[7] = recvPacket.ReadBit();
    setterGuid[4] = recvPacket.ReadBit();
    setterGuid[2] = recvPacket.ReadBit();
    targetGuid[4] = recvPacket.ReadBit();
    targetGuid[5] = recvPacket.ReadBit();
    targetGuid[6] = recvPacket.ReadBit();
    setterGuid[1] = recvPacket.ReadBit();
    setterGuid[7] = recvPacket.ReadBit();
    targetGuid[2] = recvPacket.ReadBit();
    targetGuid[3] = recvPacket.ReadBit();
    targetGuid[0] = recvPacket.ReadBit();
    setterGuid[6] = recvPacket.ReadBit();
    setterGuid[3] = recvPacket.ReadBit();
    setterGuid[0] = recvPacket.ReadBit();
    setterGuid[5] = recvPacket.ReadBit();

    recvPacket.ReadByteSeq(targetGuid[0]);
    recvPacket.ReadByteSeq(setterGuid[1]);
    recvPacket.ReadByteSeq(setterGuid[3]);
    recvPacket.ReadByteSeq(setterGuid[5]);
    recvPacket.ReadByteSeq(targetGuid[7]);
    recvPacket.ReadByteSeq(targetGuid[3]);
    recvPacket.ReadByteSeq(setterGuid[0]);
    recvPacket.ReadByteSeq(targetGuid[1]);
    recvPacket.ReadByteSeq(setterGuid[6]);
    recvPacket.ReadByteSeq(targetGuid[2]);
    recvPacket.ReadByteSeq(targetGuid[5]);
    recvPacket.ReadByteSeq(targetGuid[4]);
    recvPacket.ReadByteSeq(setterGuid[2]);
    recvPacket.ReadByteSeq(setterGuid[4]);
    recvPacket.ReadByteSeq(targetGuid[6]);
    recvPacket.ReadByteSeq(setterGuid[7]);

    if (Guild *guild = GetPlayer()->GetGuild())
        guild->HandleSetMemberRank(this, targetGuid, setterGuid, rankId);
}

// Cata Status: Done
void WorldSession::HandleGuildDisbandOpcode(WorldPacket& /*recvPacket*/)
{
    sLog->outDebug("WORLD: Received CMSG_GUILD_DISBAND");

    if (Guild* pGuild = _GetPlayerGuild(this, true))
        pGuild->HandleDisband(this);
}

// Cata Status: Done
void WorldSession::HandleGuildLeaderOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug("WORLD: Received CMSG_GUILD_LEADER");

    uint8 nameLength = recvPacket.ReadBits(7);
    /*bool isDethrone = */recvPacket.ReadBit();
    std::string name = recvPacket.ReadString(nameLength);

    if (normalizePlayerName(name))
        if (Guild* pGuild = _GetPlayerGuild(this, true))
            pGuild->HandleSetLeader(this, name);
}

// Cata Status: Done
void WorldSession::HandleGuildChangeInfoTextOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug("WORLD: Received CMSG_GUILD_INFO_TEXT");

    uint32 length = recvPacket.ReadBits(12);
    std::string info = recvPacket.ReadString(length);

    if (Guild* pGuild = _GetPlayerGuild(this, true))
        pGuild->HandleSetInfo(this, info);
}

// Cata Status: Done
void WorldSession::HandleGuildMOTDOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug("WORLD: Received CMSG_GUILD_MOTD");

    uint32 motdLength = recvPacket.ReadBits(11);
    std::string motd = recvPacket.ReadString(motdLength);

    if (Guild* pGuild = _GetPlayerGuild(this, true))
        pGuild->HandleSetMOTD(this, motd);
}


void WorldSession::HandleGuildExperienceOpcode(WorldPacket& recvPacket)
{
    ObjectGuid guildGuid;

    guildGuid[2] = recvPacket.ReadBit();
    guildGuid[1] = recvPacket.ReadBit();
    guildGuid[0] = recvPacket.ReadBit();
    guildGuid[5] = recvPacket.ReadBit();
    guildGuid[4] = recvPacket.ReadBit();
    guildGuid[7] = recvPacket.ReadBit();
    guildGuid[6] = recvPacket.ReadBit();
    guildGuid[3] = recvPacket.ReadBit();

    recvPacket.ReadByteSeq(guildGuid[7]);
    recvPacket.ReadByteSeq(guildGuid[2]);
    recvPacket.ReadByteSeq(guildGuid[3]);
    recvPacket.ReadByteSeq(guildGuid[6]);
    recvPacket.ReadByteSeq(guildGuid[1]);
    recvPacket.ReadByteSeq(guildGuid[5]);
    recvPacket.ReadByteSeq(guildGuid[0]);
    recvPacket.ReadByteSeq(guildGuid[4]);

    if (Guild* guild = sObjectMgr->GetGuildByGuid(guildGuid))
        if (guild->IsMember(_player->GetGUID()))
            guild->SendGuildXP(this);
}

void WorldSession::HandleGuildMaxExperienceOpcode(WorldPacket& recvPacket)
{
    recvPacket.read_skip<uint64>();

    WorldPacket data(SMSG_GUILD_MAX_DAILY_XP, 8);
    data << uint64(GUILD_DAILY_XP_CAP);
    SendPacket(&data);
}

void WorldSession::HandleGuildRewardsOpcode(WorldPacket& recvPacket)
{
    recvPacket.read_skip<uint32>(); // Unk

    if (sObjectMgr->GetGuildById(_player->GetGuildId()))
    {
        ObjectMgr::GuildRewardsVector const& rewards = sObjectMgr->GetGuildRewards();

        WorldPacket data(SMSG_GUILD_REWARDS_LIST, 3 + rewards.size() * (4 + 4 + 4 + 8 + 4 + 4));
        data.WriteBits(rewards.size(), 21);
        data.FlushBits();

        for (ObjectMgr::GuildRewardsVector::const_iterator itr = rewards.begin(); itr != rewards.end(); ++itr)
        {
            GuildRewardsEntry *entry = itr->second;
            data << uint32(entry->standing);
            //data << int32(rewards[i].Racemask);
            data << uint32(-1); // race mask (NYI)
            data << uint32(entry->item);
            data << uint64(entry->price);
            data << uint32(0); // Unused
            data << uint32(entry->achievement);
        }
        data << uint32(time(NULL));
        SendPacket(&data);
    }
}

void WorldSession::HandleGuildRequestPartyState(WorldPacket &recvPacket)
{
    sLog->outDebug("WORLD: Received CMSG_GUILD_REQUEST_PARTY_STATE");

    ObjectGuid guildGuid;

    guildGuid[0] = recvPacket.ReadBit();
    guildGuid[6] = recvPacket.ReadBit();
    guildGuid[7] = recvPacket.ReadBit();
    guildGuid[3] = recvPacket.ReadBit();
    guildGuid[5] = recvPacket.ReadBit();
    guildGuid[1] = recvPacket.ReadBit();
    guildGuid[2] = recvPacket.ReadBit();
    guildGuid[4] = recvPacket.ReadBit();

    recvPacket.ReadByteSeq(guildGuid[6]);
    recvPacket.ReadByteSeq(guildGuid[3]);
    recvPacket.ReadByteSeq(guildGuid[2]);
    recvPacket.ReadByteSeq(guildGuid[1]);
    recvPacket.ReadByteSeq(guildGuid[5]);
    recvPacket.ReadByteSeq(guildGuid[0]);
    recvPacket.ReadByteSeq(guildGuid[7]);
    recvPacket.ReadByteSeq(guildGuid[4]);

    if (Guild *guild = sObjectMgr->GetGuildByGuid(guildGuid))
        guild->HandleGuildPartyRequest(this);
}

// Cata Status: Done
void WorldSession::HandleGuildSetNoteOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug("WORLD: Received CMSG_GUILD_SET_NOTE");

    ObjectGuid playerGuid;

    playerGuid[1] = recvPacket.ReadBit();
    playerGuid[4] = recvPacket.ReadBit();
    playerGuid[5] = recvPacket.ReadBit();
    playerGuid[3] = recvPacket.ReadBit();
    playerGuid[0] = recvPacket.ReadBit();
    playerGuid[7] = recvPacket.ReadBit();
    bool ispublic = recvPacket.ReadBit();      // 0 == Officer, 1 == Public
    playerGuid[6] = recvPacket.ReadBit();
    uint32 noteLength = recvPacket.ReadBits(8);
    playerGuid[2] = recvPacket.ReadBit();

    recvPacket.ReadByteSeq(playerGuid[4]);
    recvPacket.ReadByteSeq(playerGuid[5]);
    recvPacket.ReadByteSeq(playerGuid[0]);
    recvPacket.ReadByteSeq(playerGuid[3]);
    recvPacket.ReadByteSeq(playerGuid[1]);
    recvPacket.ReadByteSeq(playerGuid[6]);
    recvPacket.ReadByteSeq(playerGuid[7]);
    std::string note = recvPacket.ReadString(noteLength);
    recvPacket.ReadByteSeq(playerGuid[2]);

    if (Guild* pGuild = _GetPlayerGuild(this, true))
        pGuild->HandleSetMemberNote(this, playerGuid, note, !ispublic);
}

void WorldSession::HandleGuildQueryRanksOpcode(WorldPacket& recvPacket)
{
    ObjectGuid guildGuid;

    guildGuid[2] = recvPacket.ReadBit();
    guildGuid[3] = recvPacket.ReadBit();
    guildGuid[0] = recvPacket.ReadBit();
    guildGuid[6] = recvPacket.ReadBit();
    guildGuid[4] = recvPacket.ReadBit();
    guildGuid[7] = recvPacket.ReadBit();
    guildGuid[5] = recvPacket.ReadBit();
    guildGuid[1] = recvPacket.ReadBit();

    recvPacket.ReadByteSeq(guildGuid[3]);
    recvPacket.ReadByteSeq(guildGuid[4]);
    recvPacket.ReadByteSeq(guildGuid[5]);
    recvPacket.ReadByteSeq(guildGuid[7]);
    recvPacket.ReadByteSeq(guildGuid[1]);
    recvPacket.ReadByteSeq(guildGuid[0]);
    recvPacket.ReadByteSeq(guildGuid[6]);
    recvPacket.ReadByteSeq(guildGuid[2]);

    if (Guild* guild = sObjectMgr->GetGuildByGuid(guildGuid))
        if (guild->IsMember(_player->GetGUID()))
            guild->SendGuildRankInfo(this);
}

// Cata Status: Done
void WorldSession::HandleGuildAddRankOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug("WORLD: Received CMSG_GUILD_ADD_RANK");

    uint32 rankId;
    recvPacket >> rankId;   // unused, value is computed by server

    uint32 rankNameLength;
    rankNameLength = recvPacket.ReadBits(7);
    std::string rankName;
    rankName = recvPacket.ReadString(rankNameLength);

    if (Guild* pGuild = _GetPlayerGuild(this, true))
        pGuild->HandleAddNewRank(this, rankName);
}

void WorldSession::HandleGuildSwitchRankOpcode(WorldPacket &recvPacket)
{
    sLog->outDebug("WORLD: Received CMSG_GUILD_SWITCH_RANK");

    uint32 rankId = 0;
    uint8 direction = 0;

    recvPacket >> rankId;
    recvPacket >> direction; // 0 = down, 128 = up

    if (Guild* pGuild = _GetPlayerGuild(this, true))
        pGuild->HandleSwitchRank(this, rankId, (direction != 0));
}

void WorldSession::HandleGuildSetRankPermissionsOpcode(WorldPacket& recvPacket)
{
    Guild* guild = GetPlayer()->GetGuild();
    if (!guild)
    {
        recvPacket.rfinish();
        return;
    }

    uint32 oldRankId;
    uint32 newRankId;
    uint32 oldRights;
    uint32 newRights;
    uint32 moneyPerDay;

    recvPacket >> oldRankId;
    recvPacket >> oldRights;
    recvPacket >> newRights;

    GuildBankRightsAndSlotsVec rightsAndSlots(GUILD_BANK_MAX_TABS);
    for (uint8 tabId = 0; tabId < GUILD_BANK_MAX_TABS; ++tabId)
    {
        uint32 bankRights;
        uint32 slots;

        recvPacket >> bankRights;
        recvPacket >> slots;

        rightsAndSlots[tabId] = GuildBankRightsAndSlots(uint8(bankRights), slots);
    }

    recvPacket >> moneyPerDay;
    recvPacket >> newRankId;
    uint32 nameLength = recvPacket.ReadBits(7);
    std::string rankName = recvPacket.ReadString(nameLength);

    sLog->outDebug("CMSG_GUILD_SET_RANK_PERMISSIONS [%s]: Rank: %s (%u)", GetPlayer()->GetName(), rankName.c_str(), newRankId);

    guild->HandleSetRankInfo(this, newRankId, rankName, newRights, moneyPerDay, rightsAndSlots);
}

void WorldSession::HandleGuildRequestChallenges(WorldPacket& recvPacket)
{
    if (GetPlayer() && GetPlayer()->GetGuildId())
    {
        Guild* pGuild = GetPlayer()->GetGuild();
        if (pGuild)
            pGuild->SendChallengeUpdate(this);
    }
}

// Cata Status: Done
void WorldSession::HandleGuildDelRankOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug("WORLD: Received CMSG_GUILD_DEL_RANK");
    uint32 rankid;
    recvPacket >> rankid;

    if (Guild* pGuild = _GetPlayerGuild(this, true))
        pGuild->HandleRemoveRank(this, rankid);
}

// Cata Status: Done
// TODO!!! Doesn't update tabard, guild tab until relog
void WorldSession::HandleSaveGuildEmblemOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug("WORLD: Received MSG_SAVE_GUILD_EMBLEM");

    uint64 vendorGuid;
    recvPacket >> vendorGuid;

    EmblemInfo emblemInfo;
    emblemInfo.ReadPacket(recvPacket);

    if (GetPlayer()->GetNPCIfCanInteractWith(vendorGuid, UNIT_NPC_FLAG_TABARDDESIGNER))
    {
        // Remove fake death
        if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
            GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

        if (Guild* pGuild = _GetPlayerGuild(this))
            pGuild->HandleSetEmblem(this, emblemInfo);
        else
            // "You are not part of a pGuild!";
            Guild::SendSaveEmblemResult(this, ERR_GUILDEMBLEM_NOGUILD);
    }
    else
    {
        // "That's not an emblem vendor!"
        Guild::SendSaveEmblemResult(this, ERR_GUILDEMBLEM_INVALIDVENDOR);
        sLog->outDebug("WORLD: HandleSaveGuildEmblemOpcode - Unit (GUID: %u) not found or you can't interact with him.", GUID_LOPART(vendorGuid));
    }
}

// Cata Status: Done
void WorldSession::HandleGuildEventLogQueryOpcode(WorldPacket& /* recvPacket */)
{
    sLog->outDebug("WORLD: Received (MSG_GUILD_EVENT_LOG_QUERY)");

    if (Guild* pGuild = _GetPlayerGuild(this))
        pGuild->SendEventLog(this);
}

// Cata Status: Done
void WorldSession::HandleGuildBankMoneyWithdrawn(WorldPacket & /* recv_data */)
{
    sLog->outDebug("WORLD: Received (MSG_GUILD_BANK_MONEY_WITHDRAWN)");

    if (Guild* pGuild = _GetPlayerGuild(this))
        pGuild->SendMoneyInfo(this);
}

// Cata Status: Done
void WorldSession::HandleGuildPermissions(WorldPacket& /* recv_data */)
{
    sLog->outDebug("WORLD: Received (MSG_GUILD_PERMISSIONS)");

    if (Guild* pGuild = _GetPlayerGuild(this))
        pGuild->SendPermissions(this);
}

// Called when clicking on Guild bank gameobject
// Cata Status: Done
void WorldSession::HandleGuildBankerActivate(WorldPacket & recv_data)
{
    sLog->outDebug("WORLD: Received (CMSG_GUILD_BANKER_ACTIVATE)");

    uint64 GoGuid;
    recv_data >> GoGuid;

    uint8 unk;
    recv_data >> unk;

    if (GameObject* pGO = GetPlayer()->GetGameObjectIfCanInteractWith(GoGuid, GAMEOBJECT_TYPE_GUILD_BANK))
    {
        // Exception for Mobile Banking gameobject Guild Chest
        // Only members of the same guild as caster can access
        if (pGO->GetEntry() == 206602 || pGO->GetEntry() == 206603)
        {
            if (pGO->GetOwner() && pGO->GetOwner()->ToPlayer() && pGO->GetOwner()->ToPlayer()->GetGuildId())
            {
                if (pGO->GetOwner()->ToPlayer()->GetGuildId() != GetPlayer()->GetGuildId())
                    return;
            }
            else
                return;
        }

        if (Guild* pGuild = _GetPlayerGuild(this))
            pGuild->SendBankList(this, 0, true, true);
        else 
            Guild::SendCommandResult(this, GUILD_COMMAND_VIEW_TAB, ERR_GUILD_PLAYER_NOT_IN_GUILD);
    }
}

// Called when opening pGuild bank tab only (first one)
// Cata Status: Done
void WorldSession::HandleGuildBankQueryTab(WorldPacket & recv_data)
{
    sLog->outDebug("WORLD: Received (CMSG_GUILD_BANK_QUERY_TAB)");

    uint64 GoGuid;
    recv_data >> GoGuid;

    uint8 tabId;
    recv_data >> tabId;

    bool sendAllSlots;
    recv_data >> sendAllSlots;

    if (GetPlayer()->GetGameObjectIfCanInteractWith(GoGuid, GAMEOBJECT_TYPE_GUILD_BANK))
        if (Guild* pGuild = _GetPlayerGuild(this))
            pGuild->SendBankList(this, tabId, true, false);
}

// Cata Status: Done
void WorldSession::HandleGuildBankDepositMoney(WorldPacket & recv_data)
{
    sLog->outDebug("WORLD: Received (CMSG_GUILD_BANK_DEPOSIT_MONEY)");

    uint64 GoGuid;
    recv_data >> GoGuid;

    uint64 money;
    recv_data >> money;

    if (GetPlayer()->GetGameObjectIfCanInteractWith(GoGuid, GAMEOBJECT_TYPE_GUILD_BANK))
        if (money && GetPlayer()->HasEnoughMoney(uint32(money)))
            if (Guild* pGuild = _GetPlayerGuild(this))
                pGuild->HandleMemberDepositMoney(this, money);
}

// Cata Status: Done
void WorldSession::HandleGuildBankWithdrawMoney(WorldPacket & recv_data)
{
    sLog->outDebug("WORLD: Received (CMSG_GUILD_BANK_WITHDRAW_MONEY)");

    uint64 GoGuid;
    recv_data >> GoGuid;

    uint64 money;
    recv_data >> money;

    if (money)
        if (GetPlayer()->GetGameObjectIfCanInteractWith(GoGuid, GAMEOBJECT_TYPE_GUILD_BANK))
            if (Guild* pGuild = _GetPlayerGuild(this))
                pGuild->HandleMemberWithdrawMoney(this, money);
}

// Cata Status: Done
void WorldSession::HandleGuildBankSwapItems(WorldPacket & recv_data)
{
    sLog->outDebug("WORLD: Received (CMSG_GUILD_BANK_SWAP_ITEMS)");

    uint64 GoGuid;
    recv_data >> GoGuid;

    if (!GetPlayer()->GetGameObjectIfCanInteractWith(GoGuid, GAMEOBJECT_TYPE_GUILD_BANK))
    {
        recv_data.rpos(recv_data.wpos());                   // Prevent additional spam at rejected packet
        return;
    }

    Guild* pGuild = _GetPlayerGuild(this);
    if (!pGuild)
    {
        recv_data.rpos(recv_data.wpos());                   // Prevent additional spam at rejected packet
        return;
    }

    uint8 bankToBank;
    recv_data >> bankToBank;

    uint8 tabId;
    uint8 slotId;
    uint32 itemEntry;
    uint32 splitedAmount = 0;

    if (bankToBank)
    {
        uint8 destTabId;
        recv_data >> destTabId;

        uint8 destSlotId;
        recv_data >> destSlotId;
        recv_data.read_skip<uint32>();                      // Always 0

        recv_data >> tabId;
        recv_data >> slotId;
        recv_data >> itemEntry;
        recv_data.read_skip<uint8>();                       // Always 0

        recv_data >> splitedAmount;

        pGuild->SwapItems(GetPlayer(), tabId, slotId, destTabId, destSlotId, splitedAmount);
    }
    else
    {
        uint8 playerBag = NULL_BAG;
        uint8 playerSlotId = NULL_SLOT;
        uint8 toChar = 1;

        recv_data >> tabId;
        recv_data >> slotId;
        recv_data >> itemEntry;

        uint8 autoStore;
        recv_data >> autoStore;
        if (autoStore)
        {
            recv_data.read_skip<uint32>();                  // autoStoreCount
            recv_data.read_skip<uint8>();                   // ToChar (?), always and expected to be 1 (autostore only triggered in Bank -> Char)
            recv_data.read_skip<uint32>();                  // Always 0
        }
        else
        {
            recv_data >> playerBag;
            recv_data >> playerSlotId;
            recv_data >> toChar;
            recv_data >> splitedAmount;
        }

        // Player <-> Bank
        // Allow to work with inventory only
        if (!Player::IsInventoryPos(playerBag, playerSlotId) && !(playerBag == NULL_BAG && playerSlotId == NULL_SLOT))
            GetPlayer()->SendEquipError(EQUIP_ERR_NONE, NULL);
        else
            pGuild->SwapItemsWithInventory(GetPlayer(), toChar, tabId, slotId, playerBag, playerSlotId, splitedAmount);
    }
}

// Cata Status: Done
void WorldSession::HandleGuildBankBuyTab(WorldPacket & recv_data)
{
    sLog->outDebug("WORLD: Received (CMSG_GUILD_BANK_BUY_TAB)");

    uint64 GoGuid;
    recv_data >> GoGuid;

    uint8 tabId;
    recv_data >> tabId;

    if (GetPlayer()->GetGameObjectIfCanInteractWith(GoGuid, GAMEOBJECT_TYPE_GUILD_BANK))
        if (Guild* pGuild = _GetPlayerGuild(this))
            pGuild->HandleBuyBankTab(this, tabId);
}

// Cata Status: Done
void WorldSession::HandleGuildBankUpdateTab(WorldPacket & recv_data)
{
    sLog->outDebug("WORLD: Received (CMSG_GUILD_BANK_UPDATE_TAB)");

    uint64 GoGuid;
    recv_data >> GoGuid;

    uint8 tabId;
    recv_data >> tabId;

    std::string name;
    recv_data >> name;

    std::string icon;
    recv_data >> icon;

    if (!name.empty() && !icon.empty())
        if (GetPlayer()->GetGameObjectIfCanInteractWith(GoGuid, GAMEOBJECT_TYPE_GUILD_BANK))
            if (Guild* pGuild = _GetPlayerGuild(this))
                pGuild->HandleSetBankTabInfo(this, tabId, name, icon);
}

// Cata Status: Done
void WorldSession::HandleGuildBankLogQuery(WorldPacket & recv_data)
{
    sLog->outDebug("WORLD: Received (MSG_GUILD_BANK_LOG_QUERY)");

    uint8 tabId;
    recv_data >> tabId;

    if (Guild* pGuild = _GetPlayerGuild(this))
        pGuild->SendBankLog(this, tabId);
}

// Cata Status: Done
void WorldSession::HandleQueryGuildBankTabText(WorldPacket &recv_data)
{
    sLog->outDebug("WORLD: Received MSG_QUERY_GUILD_BANK_TEXT");

    uint8 tabId;
    recv_data >> tabId;

    if (Guild* pGuild = _GetPlayerGuild(this))
        pGuild->SendBankTabText(this, tabId);
}

// Cata Status: Done
void WorldSession::HandleSetGuildBankTabText(WorldPacket &recv_data)
{
    sLog->outDebug("WORLD: Received CMSG_SET_GUILD_BANK_TEXT");

    uint32 tabId;
    recv_data >> tabId;

    uint16 textLen;
    textLen = recv_data.ReadBits(14);
    std::string text;
    text = recv_data.ReadString(textLen);

    if (Guild* pGuild = _GetPlayerGuild(this))
        pGuild->SetBankTabText(tabId, text);
}
