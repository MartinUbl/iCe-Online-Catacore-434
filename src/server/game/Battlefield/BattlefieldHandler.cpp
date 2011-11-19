/*
 * Copyright (C) 2010-2011 Project SkyFire <http://www.projectskyfire.org/>
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "gamePCH.h"
#include "Common.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include "Battlefield.h"
#include "BattlefieldMgr.h"
#include "Opcodes.h"

//This send to player windows for invite player to join the war
//Param1:(BattleId) the BattleId of Bf
//Param2:(ZoneId) the zone where the battle is (4197 for wg)
//Param3:(time) Time in second that the player have for accept
void WorldSession::SendBfInvitePlayerToWar(uint32 BattleId, uint32 ZoneId, uint32 p_time)
{
    //Send packet
    WorldPacket data(SMSG_BATTLEFIELD_MGR_ENTRY_INVITE, 12, true);
    data << uint32(time(NULL)+p_time);
    data << uint32(ZoneId);
    data << uint64(BattleId);

    //Sending the packet to player
    SendPacket(&data);
}

//This send invitation to player to join the queue
//Param1:(BattleId) the BattleId of Bf
void WorldSession::SendBfInvitePlayerToQueue(uint32 BattleId)
{
    WorldPacket data(SMSG_BATTLEFIELD_MGR_QUEUE_INVITE, 5, true);

    data << uint8(1);
    data << uint8(3);   // 0 = "Would you like to join?", 1 = "Battle is about to begin!"
    data << uint32(1);
    data << uint32(1);
    data << uint32(1);
    data << uint32(1);
    data << uint32(1);
    data << uint64(BattleId);

    // Whole packet nearly unknown. We know only 2 fields... thats not much
    // .. but it appears it is enough

    //Sending packet to player
    SendPacket(&data);
}

//This send packet for inform player that he join queue
//Param1:(BattleId) the BattleId of Bf
//Param2:(ZoneId) the zone where the battle is (4197 for wg)
void WorldSession::SendBfQueueInviteResponse(uint32 BattleId, uint32 ZoneId, bool inProgress, bool canJoin)
{
    WorldPacket data(SMSG_BATTLEFIELD_MGR_QUEUE_REQUEST_RESPONSE, 11, true);
    data << uint8(1);                     // unknown
    data << uint64(BattleId);             // BattleId... or that next 64bit field?
    data << uint32(ZoneId);               // ZoneId
    data << uint64(BattleId);             // BattleId, dont know
    data << uint8((canJoin ? 1 : 0));     // can queue?
    data << uint8((inProgress ? 0 : 1));  // 0 = "You are queued, please wait", 1 = "You are in queue for upcoming battle"

    SendPacket(&data);
}

//This is call when player accept to join war
//Param1:(BattleId) the BattleId of Bf
void WorldSession::SendBfEntered(uint32 BattleId)
{
//    m_PlayerInWar[player->GetTeamId()].insert(player->GetGUID());
    WorldPacket data(SMSG_BATTLEFIELD_MGR_ENTERED, 7, true);
    data << uint8(1);         // unknown
    data << uint64(BattleId); // BattleId

    SendPacket(&data);
}

//Send when player is kick from Battlefield
void WorldSession::SendBfLeaveMessage(uint32 BattleId)
{
    WorldPacket data(SMSG_BATTLEFIELD_MGR_EJECTED, 7, true);
    data << uint8(0);         // unknown (used to be Relocated?)
    data << uint8(2);         // BattleStatus?
    data << uint64(BattleId); // BattleId
    data << uint8(8);         // Reason?

    SendPacket(&data);
}

//Send by client when he click on accept for queue
void WorldSession::HandleBfQueueInviteResponse(WorldPacket & recv_data)
{
    uint16 BattleId;
    uint8 Accepted;

    recv_data >> Accepted >> BattleId;
    recv_data.read_skip<uint32>(); // always 2
    recv_data.read_skip<uint16>(); // constant

    Battlefield* Bf = sBattlefieldMgr.GetBattlefieldByBattleId(BattleId);
    if (!Bf)
        return;

    if (Accepted) // accepted = 1 << 7
    {
        Bf->PlayerAcceptInviteToQueue(_player);
    }
}

//Send by client on clicking in accept or refuse of invitation windows for join game
void WorldSession::HandleBfEntryInviteResponse(WorldPacket & recv_data)
{
    uint16 BattleId;
    uint8 Accepted;

    recv_data >> Accepted >> BattleId;
    recv_data.read_skip<uint32>(); // always 2
    recv_data.read_skip<uint16>(); // constant

    Battlefield* Bf = sBattlefieldMgr.GetBattlefieldByBattleId(BattleId);
    if (!Bf)
        return;

    //If player accept invitation
    if (Accepted) // 1 << 7 = accepted
    {
        Bf->PlayerAcceptInviteToWar(_player);
    }
    else
    {
        if (_player->GetZoneId() == Bf->GetZoneId())
            Bf->KickPlayerFromBf(_player->GetGUID());

        Bf->AskToLeaveQueue(_player);
    }
}

void WorldSession::HandleBfQueueRequest(WorldPacket &recv_data)
{
    uint16 battleId;

    recv_data >> battleId;
    recv_data.read_skip<uint32>(); // unknown, always 2
    recv_data.read_skip<uint16>(); // constant, same as in BG joining

    Battlefield* Bf = sBattlefieldMgr.GetBattlefieldByBattleId(battleId);
    if (!Bf)
        return;

    //SendBfInvitePlayerToWar(1,4197,20);
    Bf->InvitePlayerToQueue(GetPlayer());
}

void WorldSession::HandleBfExitRequest(WorldPacket & recv_data)
{
    if (recv_data.GetOpcode() == CMSG_BATTLEFIELD_MGR_LEAVE_REQUEST)
    {
        Battlefield* Bf = sBattlefieldMgr.GetBattlefieldToZoneId(_player->GetZoneId());
        if (!Bf)
            return;

        Bf->AskToLeaveQueue(_player);
        Bf->KickPlayerFromBf(_player->GetGUID());
        return;
    }

    uint16 BattleId;

    recv_data >> BattleId;
    recv_data.read_skip<uint32>(); // unknown, always 2
    recv_data.read_skip<uint16>(); // constant, same as in BG joining
    sLog->outError("HandleBfExitRequest: BattleID:%u ", BattleId);
    Battlefield* Bf = sBattlefieldMgr.GetBattlefieldByBattleId(BattleId);
    if (!Bf)
        return;

    Bf->AskToLeaveQueue(_player);
}
