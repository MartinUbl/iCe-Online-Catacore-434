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
#include "ObjectMgr.h"                                      // for normalizePlayerName
#include "ChannelMgr.h"

void WorldSession::HandleJoinChannel(WorldPacket& recvPacket)
{
    uint32 channelId;
    uint32 channelLength, passLength;
    std::string channelName, password;

    recvPacket >> channelId;
    uint8 unknown1 = recvPacket.ReadBit();   // unknowns
    uint8 unknown2 = recvPacket.ReadBit();
    channelLength = recvPacket.ReadBits(8);
    passLength = recvPacket.ReadBits(8);
    channelName = recvPacket.ReadString(channelLength);
    password = recvPacket.ReadString(passLength);

    if (channelId)
    {
        ChatChannelsEntry const* channel = sChatChannelsStore.LookupEntry(channelId);
        if (!channel)
            return;

        AreaTableEntry const* zone = GetAreaEntryByAreaID(GetPlayer()->GetZoneId());
        if (!zone || !GetPlayer()->CanJoinConstantChannelInZone(channel, zone))
            return;
    }

    if (channelName.empty())
        return;

    if (ChannelMgr* cMgr = channelMgr(GetPlayer()->GetTeam()))
    {
        cMgr->team = GetPlayer()->GetTeam();
        Channel *chn = cMgr->GetJoinChannel(channelName, channelId);
        if (chn)
            chn->Join(GetPlayer()->GetGUID(), password.c_str());
    }
}

void WorldSession::HandleLeaveChannel(WorldPacket& recvPacket)
{
    sLog->outDebug("Opcode %u", recvPacket.GetOpcode());
    //recvPacket.hexlike();

    uint32 unk0; //unknown 4 bytes, read as uint32 (4.0.3)
    uint8 channelNameSize;
    std::string channelname;

    recvPacket >> unk0;
    recvPacket >> channelNameSize;

    if (channelNameSize)
        channelname = recvPacket.ReadString(channelNameSize);

    if (channelname.empty())
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
    {
        if (Channel *chn = cMgr->GetChannel(channelname, _player))
            chn->Leave(_player->GetGUID(), true);
        cMgr->LeftChannel(channelname);
    }
}

void WorldSession::HandleChannelList(WorldPacket& recvPacket)
{
    sLog->outDebug("Opcode %u", recvPacket.GetOpcode());
    //recvPacket.hexlike();
    uint8 channelsize;
    std::string channelname;

    recvPacket >> channelsize;

    if (channelsize)
        channelname = recvPacket.ReadString(channelsize);

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel *chn = cMgr->GetChannel(channelname, _player))
            chn->List(_player);
}

void WorldSession::HandleChannelPassword(WorldPacket& recvPacket)
{
    uint32 nameLength = recvPacket.ReadBits(8);
    uint32 passLength = recvPacket.ReadBits(7);

    std::string channelName = recvPacket.ReadString(nameLength);
    std::string password = recvPacket.ReadString(passLength);

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel *chn = cMgr->GetChannel(channelName, _player))
            chn->Password(_player->GetGUID(), password.c_str());
}

void WorldSession::HandleChannelSetOwner(WorldPacket& recvPacket)
{
    uint32 channelLength = recvPacket.ReadBits(8);
    uint32 nameLength = recvPacket.ReadBits(7);

    std::string newp = recvPacket.ReadString(nameLength);
    std::string channelName = recvPacket.ReadString(channelLength);

    if (!normalizePlayerName(newp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel *chn = cMgr->GetChannel(channelName, _player))
            chn->SetOwner(_player->GetGUID(), newp.c_str());
}

void WorldSession::HandleChannelOwner(WorldPacket& recvPacket)
{
    sLog->outDebug("Opcode %u", recvPacket.GetOpcode());
    //recvPacket.hexlike();
    uint8 channelnameSize;
    std::string channelname;

    recvPacket >> channelnameSize;

    if (channelnameSize)
        channelname = recvPacket.ReadString(channelnameSize);

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel *chn = cMgr->GetChannel(channelname, _player))
            chn->SendWhoOwner(_player->GetGUID());
}

void WorldSession::HandleChannelModerator(WorldPacket& recvPacket)
{
    sLog->outDebug("Opcode %u", recvPacket.GetOpcode());
    //recvPacket.hexlike();
    uint16 otpsize, channelnamesize;
    std::string channelname, otp;

    otpsize = recvPacket.ReadBits(9);
    channelnamesize = recvPacket.ReadBits(7);

    if (otpsize > 0)
        otp = recvPacket.ReadString(otpsize);
    if (channelnamesize > 0)
        channelname = recvPacket.ReadString(channelnamesize);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel *chn = cMgr->GetChannel(channelname, _player))
            chn->SetModerator(_player->GetGUID(), otp.c_str());
}

void WorldSession::HandleChannelUnmoderator(WorldPacket& recvPacket)
{
    sLog->outDebug("Opcode %u", recvPacket.GetOpcode());
    //recvPacket.hexlike();
    uint8 otpsize, channelsize;
    std::string channelname, otp;

    otpsize = recvPacket.ReadBits(7);
    channelsize = recvPacket.ReadBits(8);

    recvPacket.FlushBits();

    if (channelsize)
        channelname = recvPacket.ReadString(channelsize);
    if (otpsize)
        otp = recvPacket.ReadString(otpsize);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel *chn = cMgr->GetChannel(channelname, _player))
            chn->UnsetModerator(_player->GetGUID(), otp.c_str());
}

void WorldSession::HandleChannelMute(WorldPacket& recvPacket)
{
    sLog->outDebug("Opcode %u", recvPacket.GetOpcode());
    //recvPacket.hexlike();
    uint8 otpsize, channelsize;
    std::string channelname, otp;

    channelsize = recvPacket.ReadBits(8);
    otpsize = recvPacket.ReadBits(7);
    recvPacket.FlushBits();

    if (channelsize)
        channelname = recvPacket.ReadString(channelsize);

    if (otpsize)
        otp = recvPacket.ReadString(otpsize);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel *chn = cMgr->GetChannel(channelname, _player))
            chn->SetMute(_player->GetGUID(), otp.c_str());
}

void WorldSession::HandleChannelUnmute(WorldPacket& recvPacket)
{
    sLog->outDebug("Opcode %u", recvPacket.GetOpcode());
    //recvPacket.hexlike();

    uint8 otpsize, channelsize;
    std::string channelname, otp;

    channelsize = recvPacket.ReadBits(8);
    otpsize = recvPacket.ReadBits(7);
    recvPacket.FlushBits();

    if (channelsize)
        channelname = recvPacket.ReadString(channelsize);

    if (otpsize)
        otp = recvPacket.ReadString(otpsize);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel *chn = cMgr->GetChannel(channelname, _player))
            chn->UnsetMute(_player->GetGUID(), otp.c_str());
}

void WorldSession::HandleChannelInvite(WorldPacket& recvPacket)
{
    sLog->outDebug("Opcode %u", recvPacket.GetOpcode());
    //recvPacket.hexlike();

    uint8 otpsize, channelsize;
    std::string channelname, otp;

    otpsize = recvPacket.ReadBits(7);
    channelsize = recvPacket.ReadBits(8);
    recvPacket.FlushBits();

    if (otpsize)
        otp = recvPacket.ReadString(otpsize);

    if (channelsize)
        channelname = recvPacket.ReadString(channelsize);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel *chn = cMgr->GetChannel(channelname, _player))
            chn->Invite(_player->GetGUID(), otp.c_str());
}

void WorldSession::HandleChannelKick(WorldPacket& recvPacket)
{
    sLog->outDebug("Opcode %u", recvPacket.GetOpcode());
    //recvPacket.hexlike();

    uint8 otpsize, channelsize;
    std::string channelname, otp;

    channelsize = recvPacket.ReadBits(8);
    otpsize = recvPacket.ReadBits(7);
    recvPacket.FlushBits();

    if (channelsize)
        channelname = recvPacket.ReadString(channelsize);

    if (otpsize)
        otp = recvPacket.ReadString(otpsize);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel *chn = cMgr->GetChannel(channelname, _player))
            chn->Kick(_player->GetGUID(), otp.c_str());
}

void WorldSession::HandleChannelBan(WorldPacket& recvPacket)
{
    sLog->outDebug("Opcode %u", recvPacket.GetOpcode());
    //recvPacket.hexlike();
    uint8 otpsize, channelsize;
    std::string channelname, otp;

    channelsize = recvPacket.ReadBits(8);
    otpsize = recvPacket.ReadBits(7);
    recvPacket.FlushBits();

    if (otpsize)
        otp = recvPacket.ReadString(otpsize);

    if (channelsize)
        channelname = recvPacket.ReadString(channelsize);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel *chn = cMgr->GetChannel(channelname, _player))
            chn->Ban(_player->GetGUID(), otp.c_str());
}

void WorldSession::HandleChannelUnban(WorldPacket& recvPacket)
{
    sLog->outDebug("Opcode %u", recvPacket.GetOpcode());
    //recvPacket.hexlike();

    uint8 otpsize, channelsize;
    std::string channelname, otp;

    otpsize = recvPacket.ReadBits(7);
    channelsize = recvPacket.ReadBits(8);
    recvPacket.FlushBits();

    if (channelsize)
        channelname = recvPacket.ReadString(channelsize);

    if (otpsize)
        otp = recvPacket.ReadString(otpsize);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel *chn = cMgr->GetChannel(channelname, _player))
            chn->UnBan(_player->GetGUID(), otp.c_str());
}

void WorldSession::HandleChannelAnnouncements(WorldPacket& recvPacket)
{
    sLog->outDebug("Opcode %u", recvPacket.GetOpcode());
    //recvPacket.hexlike();
    uint8 channelsize;
    std::string channelname;

    recvPacket >> channelsize;
    if (channelsize)
        channelname = recvPacket.ReadString(channelsize);

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel *chn = cMgr->GetChannel(channelname, _player))
            chn->Announce(_player->GetGUID());
}

void WorldSession::HandleChannelModerate(WorldPacket& recvPacket)
{
    sLog->outDebug("Opcode %u", recvPacket.GetOpcode());
    //recvPacket.hexlike();
    std::string channelname;
    recvPacket >> channelname;
    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel *chn = cMgr->GetChannel(channelname, _player))
            chn->Moderate(_player->GetGUID());
}

void WorldSession::HandleChannelDisplayListQuery(WorldPacket &recvPacket)
{
    // this should be OK because the 2 function _were_ the same
    HandleChannelList(recvPacket);
}

void WorldSession::HandleGetChannelMemberCount(WorldPacket &recvPacket)
{
    sLog->outDebug("Opcode %u", recvPacket.GetOpcode());
    //recvPacket.hexlike();
    std::string channelname;
    recvPacket >> channelname;
    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
    {
        if (Channel *chn = cMgr->GetChannel(channelname, _player))
        {
            WorldPacket data(SMSG_CHANNEL_MEMBER_COUNT, chn->GetName().size()+1+1+4);
            data << chn->GetName();
            data << uint8(chn->GetFlags());
            data << uint32(chn->GetNumPlayers());
            SendPacket(&data);
        }
    }
}

void WorldSession::HandleSetChannelWatch(WorldPacket &recvPacket)
{
    sLog->outDebug("Opcode %u", recvPacket.GetOpcode());
    //recvPacket.hexlike();
    std::string channelname;
    recvPacket >> channelname;

    // And send player list for player, which started watching channel
    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel *chn = cMgr->GetChannel(channelname, _player))
            chn->List(_player);

    // Why would be this here?
    /*if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel *chn = cMgr->GetChannel(channelname, _player))
            chn->JoinNotify(_player->GetGUID());*/
}

