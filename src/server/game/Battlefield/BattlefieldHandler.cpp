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

#define MAKE_BF_GUID(x) MAKE_NEW_GUID(x, 0, HIGHGUID_BATTLEGROUND)

//This send to player windows for invite player to join the war
//Param1:(BattleId) the BattleId of Bf
//Param2:(ZoneId) the zone where the battle is (4197 for wg)
//Param3:(time) Time in second that the player have for accept
void WorldSession::SendBfInvitePlayerToWar(uint32 BattleId, uint32 ZoneId, uint32 p_time)
{
    ObjectGuid guidBytes = MAKE_BF_GUID(BattleId);

    //Send packet
    WorldPacket data(SMSG_BATTLEFIELD_MGR_ENTRY_INVITE, 12, true);
    data.WriteBit(guidBytes[5]);
    data.WriteBit(guidBytes[3]);
    data.WriteBit(guidBytes[7]);
    data.WriteBit(guidBytes[2]);
    data.WriteBit(guidBytes[6]);
    data.WriteBit(guidBytes[4]);
    data.WriteBit(guidBytes[1]);
    data.WriteBit(guidBytes[0]);

    data.WriteByteSeq(guidBytes[6]);
    data << uint32(ZoneId);         // Zone Id
    data.WriteByteSeq(guidBytes[1]);
    data.WriteByteSeq(guidBytes[3]);
    data.WriteByteSeq(guidBytes[4]);
    data.WriteByteSeq(guidBytes[2]);
    data.WriteByteSeq(guidBytes[0]);
    data << uint32(time(NULL) + p_time); // Invite lasts until
    data.WriteByteSeq(guidBytes[7]);
    data.WriteByteSeq(guidBytes[5]);

    //Sending the packet to player
    SendPacket(&data);
}

//This send invitation to player to join the queue
//Param1:(BattleId) the BattleId of Bf
void WorldSession::SendBfInvitePlayerToQueue(uint32 BattleId)
{
    WorldPacket data(SMSG_BATTLEFIELD_MGR_QUEUE_INVITE, 5, true);

    ObjectGuid guid = MAKE_BF_GUID(BattleId);

    data.WriteBit(1); // v4 + 6 !
    data.WriteBit(0); // v4 + 36 ! // has warmup
    data.WriteBit(0); // v4 + 10
    data.WriteBit(guid[0]); // v4 + 16
    data.WriteBit(1); // v4 + 8
    data.WriteBit(guid[2]); // v4 + 18
    data.WriteBit(guid[6]); // v4 + 22
    data.WriteBit(guid[3]); // v4 + 19

    data.WriteBit(1); // v4 + 7 !
    data.WriteBit(0); // v4 + 48
    data.WriteBit(guid[1]); // v4 + 17
    data.WriteBit(guid[5]); // v4 + 21
    data.WriteBit(guid[4]); // v4 + 20
    data.WriteBit(1); // v4 + 11 !
    data.WriteBit(guid[7]); // v4 + 23

    data.FlushBits();

    data.WriteByteSeq(guid[2]);
    // v4 + 10, int
    data.WriteByteSeq(guid[3]);
    data.WriteByteSeq(guid[6]);
    data << uint8(0); // warmup
    data.WriteByteSeq(guid[5]);
    data.WriteByteSeq(guid[0]);
    // v4 + 6, int
    // v4 + 11, int
    // v4 + 8, int
    data.WriteByteSeq(guid[4]);
    // v4 + 7, int
    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[7]);

    SendPacket(&data);
}

//This send packet for inform player that he join queue
//Param1:(BattleId) the BattleId of Bf
//Param2:(ZoneId) the zone where the battle is (4197 for wg)
void WorldSession::SendBfQueueInviteResponse(uint32 BattleId, uint32 ZoneId, bool inProgress, bool canJoin)
{
    const bool hasSecondGuid = false;
    const bool warmup = true;
    ObjectGuid guidBytes = MAKE_BF_GUID(BattleId);

    WorldPacket data(SMSG_BATTLEFIELD_MGR_QUEUE_REQUEST_RESPONSE, 16);

    data.WriteBit(guidBytes[1]);
    data.WriteBit(guidBytes[6]);
    data.WriteBit(guidBytes[5]);
    data.WriteBit(guidBytes[7]);
    data.WriteBit(inProgress);  // Logging In, VERIFYME
    data.WriteBit(guidBytes[0]);
    data.WriteBit(!hasSecondGuid);
    data.WriteBit(guidBytes[4]);

    // if (hasSecondGuid) 7 3 0 4 2 6 1 5

    data.WriteBit(guidBytes[3]);
    data.WriteBit(guidBytes[2]);

    // if (hasSecondGuid) 2 5 3 0 4 6 1 7

    data.FlushBits();

    data << uint8(canJoin);  // Accepted

    data.WriteByteSeq(guidBytes[1]);
    data.WriteByteSeq(guidBytes[3]);
    data.WriteByteSeq(guidBytes[6]);
    data.WriteByteSeq(guidBytes[7]);
    data.WriteByteSeq(guidBytes[0]);

    data << uint8(warmup);

    data.WriteByteSeq(guidBytes[2]);
    data.WriteByteSeq(guidBytes[4]);
    data.WriteByteSeq(guidBytes[5]);

    data << uint32(ZoneId);

    SendPacket(&data);
}

//This is call when player accept to join war
//Param1:(BattleId) the BattleId of Bf
void WorldSession::SendBfEntered(uint32 BattleId)
{
    uint8 isAFK = GetPlayer()->isAFK() ? 1 : 0;
    ObjectGuid guidBytes = MAKE_BF_GUID(BattleId);

    WorldPacket data(SMSG_BATTLEFIELD_MGR_ENTERED, 11);

    data.WriteBit(0);               // unk
    data.WriteBit(isAFK);           // Clear AFK
    data.WriteBit(guidBytes[1]);
    data.WriteBit(guidBytes[4]);
    data.WriteBit(guidBytes[5]);
    data.WriteBit(guidBytes[0]);
    data.WriteBit(guidBytes[3]);
    data.WriteBit(0);               // unk
    data.WriteBit(guidBytes[6]);
    data.WriteBit(guidBytes[7]);
    data.WriteBit(guidBytes[2]);

    data.FlushBits();

    data.WriteByteSeq(guidBytes[5]);
    data.WriteByteSeq(guidBytes[3]);
    data.WriteByteSeq(guidBytes[0]);
    data.WriteByteSeq(guidBytes[4]);
    data.WriteByteSeq(guidBytes[1]);
    data.WriteByteSeq(guidBytes[7]);
    data.WriteByteSeq(guidBytes[2]);
    data.WriteByteSeq(guidBytes[6]);

    SendPacket(&data);
}

//Send when player is kick from Battlefield
void WorldSession::SendBfLeaveMessage(uint32 BattleId, BFLeaveReason reason)
{
    ObjectGuid guidBytes = MAKE_BF_GUID(BattleId);

    WorldPacket data(SMSG_BATTLEFIELD_MGR_EJECTED, 11);

    data.WriteBit(guidBytes[2]);
    data.WriteBit(guidBytes[5]);
    data.WriteBit(guidBytes[1]);
    data.WriteBit(guidBytes[0]);
    data.WriteBit(guidBytes[3]);
    data.WriteBit(guidBytes[6]);
    data.WriteBit(0);               // Relocated
    data.WriteBit(guidBytes[7]);
    data.WriteBit(guidBytes[4]);

    data.FlushBits();

    data << uint8(2);               // BattleStatus
    data.WriteByteSeq(guidBytes[1]);
    data.WriteByteSeq(guidBytes[7]);
    data.WriteByteSeq(guidBytes[4]);
    data.WriteByteSeq(guidBytes[2]);
    data.WriteByteSeq(guidBytes[3]);
    data << uint8(reason);          // Reason
    data.WriteByteSeq(guidBytes[6]);
    data.WriteByteSeq(guidBytes[0]);
    data.WriteByteSeq(guidBytes[5]);

    SendPacket(&data);
}

//Send by client when he click on accept for queue
void WorldSession::HandleBfQueueInviteResponse(WorldPacket & recv_data)
{
    uint16 BattleId;
    uint8 Accepted;

    ObjectGuid guid;

    guid[2] = recv_data.ReadBit();
    guid[0] = recv_data.ReadBit();
    guid[4] = recv_data.ReadBit();
    guid[3] = recv_data.ReadBit();
    guid[5] = recv_data.ReadBit();
    guid[7] = recv_data.ReadBit();
    Accepted = recv_data.ReadBit();
    guid[1] = recv_data.ReadBit();

    recv_data.ReadBits(16); // unk, but flushed

    recv_data.ReadByteSeq(guid[1]);
    recv_data.ReadByteSeq(guid[3]);
    recv_data.ReadByteSeq(guid[2]);
    recv_data.ReadByteSeq(guid[4]);
    recv_data.ReadByteSeq(guid[6]);
    recv_data.ReadByteSeq(guid[7]);
    recv_data.ReadByteSeq(guid[0]);
    recv_data.ReadByteSeq(guid[5]);

    BattleId = (uint16)(guid);

    Battlefield* Bf = sBattlefieldMgr.GetBattlefieldByBattleId(BattleId);
    if (!Bf)
        return;

    if (Accepted) // accepted = 1 << 7
    {
        Bf->PlayerAcceptInviteToQueue(_player);
    }
}

//Send by client on clicking in accept or refuse of invitation windows for join game
void WorldSession::HandleBfEntryInviteResponse(WorldPacket & recvData)
{
    uint8 Accepted;
    ObjectGuid guid;
    uint16 BattleId;

    guid[6] = recvData.ReadBit();
    guid[1] = recvData.ReadBit();
    Accepted = recvData.ReadBit();
    guid[5] = recvData.ReadBit();
    guid[3] = recvData.ReadBit();
    guid[2] = recvData.ReadBit();
    guid[0] = recvData.ReadBit();
    guid[7] = recvData.ReadBit();
    guid[4] = recvData.ReadBit();

    recvData.ReadByteSeq(guid[0]);
    recvData.ReadByteSeq(guid[3]);
    recvData.ReadByteSeq(guid[4]);
    recvData.ReadByteSeq(guid[2]);
    recvData.ReadByteSeq(guid[1]);
    recvData.ReadByteSeq(guid[6]);
    recvData.ReadByteSeq(guid[7]);
    recvData.ReadByteSeq(guid[5]);

    BattleId = (uint16)(guid);

    Battlefield* Bf = sBattlefieldMgr.GetBattlefieldByBattleId(BattleId);
    if (!Bf)
        return;

    //If player accept invitation
    if (Accepted)
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

    ObjectGuid guid;

    guid[0] = recv_data.ReadBit();
    guid[3] = recv_data.ReadBit();
    guid[7] = recv_data.ReadBit();
    guid[4] = recv_data.ReadBit();
    guid[6] = recv_data.ReadBit();
    guid[2] = recv_data.ReadBit();
    guid[1] = recv_data.ReadBit();
    guid[5] = recv_data.ReadBit();

    recv_data.ReadByteSeq(guid[6]);
    recv_data.ReadByteSeq(guid[3]);
    recv_data.ReadByteSeq(guid[2]);
    recv_data.ReadByteSeq(guid[4]);
    recv_data.ReadByteSeq(guid[7]);
    recv_data.ReadByteSeq(guid[1]);
    recv_data.ReadByteSeq(guid[5]);
    recv_data.ReadByteSeq(guid[0]);

    // we get battlefield guid from here
    // high guid is the same as battleground
    // probably have entry part, we skip this
    // we want only the last 2 bytes, which is battleId

    battleId = (uint16)(guid);

    Battlefield* Bf = sBattlefieldMgr.GetBattlefieldByBattleId(battleId);
    if (!Bf)
        return;

    Bf->InvitePlayerToQueue(GetPlayer());
}

void WorldSession::HandleBfExitRequest(WorldPacket & recvData)
{
    if (recvData.GetOpcode() == CMSG_BATTLEFIELD_MGR_LEAVE_REQUEST)
    {
        Battlefield* Bf = sBattlefieldMgr.GetBattlefieldToZoneId(_player->GetZoneId());
        if (!Bf)
            return;

        Bf->AskToLeaveQueue(_player);
        Bf->KickPlayerFromBf(_player->GetGUID());

        if (_player->GetGroup() && _player->GetGroup()->isBFGroup())
            _player->RemoveFromBattlegroundOrBattlefieldRaid();

        return;
    }

    uint16 BattleId;
    ObjectGuid guid;

    guid[2] = recvData.ReadBit();
    guid[0] = recvData.ReadBit();
    guid[3] = recvData.ReadBit();
    guid[7] = recvData.ReadBit();
    guid[4] = recvData.ReadBit();
    guid[5] = recvData.ReadBit();
    guid[6] = recvData.ReadBit();
    guid[1] = recvData.ReadBit();

    recvData.ReadByteSeq(guid[5]);
    recvData.ReadByteSeq(guid[2]);
    recvData.ReadByteSeq(guid[0]);
    recvData.ReadByteSeq(guid[1]);
    recvData.ReadByteSeq(guid[4]);
    recvData.ReadByteSeq(guid[3]);
    recvData.ReadByteSeq(guid[7]);
    recvData.ReadByteSeq(guid[6]);

    BattleId = (uint16)(guid);

    Battlefield* Bf = sBattlefieldMgr.GetBattlefieldByBattleId(BattleId);
    if (!Bf)
        return;

    Bf->AskToLeaveQueue(_player);
}
