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
#include "Opcodes.h"
#include "Log.h"
#include "Corpse.h"
#include "Player.h"
#include "Vehicle.h"
#include "SpellAuras.h"
#include "MapManager.h"
#include "Transport.h"
#include "DynamicTransport.h"
#include "Battleground.h"
#include "WaypointMovementGenerator.h"
#include "InstanceSaveMgr.h"
#include "ObjectMgr.h"
#include "MovementStructures.h"
#include "G3D/g3dmath.h"

void WorldSession::HandleMoveWorldportAckOpcode(WorldPacket & /*recv_data*/)
{
    sLog->outDebug("WORLD: got MSG_MOVE_WORLDPORT_ACK.");
    HandleMoveWorldportAckOpcode();
}

void WorldSession::HandleMoveWorldportAckOpcode()
{
    // ignore unexpected far teleports
    if (!GetPlayer()->IsBeingTeleportedFar())
        return;

    // get the teleport destination
    WorldLocation &loc = GetPlayer()->GetTeleportDest();
    WorldLocation oldLoc;
    //we need to remember old position for stuck option
    oldLoc.m_mapId=0;
    oldLoc.m_positionX=GetPlayer()->GetPositionX()-10;
    oldLoc.m_positionY=GetPlayer()->GetPositionY()-10;
    oldLoc.m_positionZ=GetPlayer()->GetPositionZ()+20;
    oldLoc.m_orientation=-GetPlayer()->GetOrientation();
    // possible errors in the coordinate validity check
    if (!MapManager::IsValidMapCoord(loc))
    {
        GetPlayer()->SetSemaphoreTeleportFar(false); // to avoid recursion loop
        LogoutPlayer(false);
        return;
    }

    // get the destination map entry, not the current one, this will fix homebind and reset greeting
    MapEntry const* mEntry = sMapStore.LookupEntry(loc.GetMapId());
    InstanceTemplate const* mInstance = sObjectMgr->GetInstanceTemplate(loc.GetMapId());

    // reset instance validity, except if going to an instance inside an instance
    if (GetPlayer()->m_InstanceValid == false && !mInstance)
        GetPlayer()->m_InstanceValid = true;

    GetPlayer()->SetSemaphoreTeleportFar(false);

    Map * oldMap = GetPlayer()->GetMap();
    ASSERT(oldMap);
    if (GetPlayer()->IsInWorld())
    {
        sLog->outCrash("Player is still in world when teleported from map %u! to new map %u", oldMap->GetId(), loc.GetMapId());
        oldMap->Remove(GetPlayer(), false);
    }

    // relocate the player to the teleport destination
    Map * newMap = sMapMgr->CreateMap(loc.GetMapId(), GetPlayer(), 0);
    // the CanEnter checks are done in TeleporTo but conditions may change
    // while the player is in transit, for example the map may get full
    if (!newMap || !newMap->CanEnter(GetPlayer()))
    {
        sLog->outError("Map %d could not be created for player %d, porting player back to previous location", loc.GetMapId(), GetPlayer()->GetGUIDLow());
        GetPlayer()->TeleportTo(oldLoc, 0, true); //lets port player back
        return;
    }
    else
        GetPlayer()->Relocate(&loc);

    GetPlayer()->ResetMap();
    GetPlayer()->SetMap(newMap);

    GetPlayer()->SendInitialPacketsBeforeAddToMap();
    if (!GetPlayer()->GetMap()->Add(GetPlayer()))
    {
        sLog->outError("WORLD: failed to teleport player %s (%d) to map %d because of unknown reason!", GetPlayer()->GetName(), GetPlayer()->GetGUIDLow(), loc.GetMapId());
        GetPlayer()->ResetMap();
        GetPlayer()->SetMap(oldMap);
        GetPlayer()->TeleportTo(GetPlayer()->m_homebindMapId, GetPlayer()->m_homebindX, GetPlayer()->m_homebindY, GetPlayer()->m_homebindZ, GetPlayer()->GetOrientation());
        return;
    }

    // battleground state prepare (in case join to BG), at relogin/tele player not invited
    // only add to bg group and object, if the player was invited (else he entered through command)
    if (_player->InBattleground())
    {
        // cleanup setting if outdated
        if (!mEntry->IsBattlegroundOrArena())
        {
            // We're not in BG
            _player->SetBattlegroundId(0, BATTLEGROUND_TYPE_NONE);
            // reset destination bg team
            _player->SetBGTeam(0);
        }
        // join to bg case
        else if (Battleground *bg = _player->GetBattleground())
        {
            if (_player->IsInvitedForBattlegroundInstance(_player->GetBattlegroundId()))
                bg->AddPlayer(_player);
        }
    }

    GetPlayer()->SendInitialPacketsAfterAddToMap();

    // flight fast teleport case
    if (GetPlayer()->GetMotionMaster()->GetCurrentMovementGeneratorType() == FLIGHT_MOTION_TYPE)
    {
        if (!_player->InBattleground())
        {
            // short preparations to continue flight
            FlightPathMovementGenerator* flight = (FlightPathMovementGenerator*)(GetPlayer()->GetMotionMaster()->top());
            flight->Initialize(GetPlayer());
            return;
        }

        // battleground state prepare, stop flight
        GetPlayer()->GetMotionMaster()->MovementExpired();
        GetPlayer()->CleanupAfterTaxiFlight();
    }

    // resurrect character at enter into instance where his corpse exist after add to map
    Corpse *corpse = GetPlayer()->GetCorpse();
    if (corpse && corpse->GetType() != CORPSE_BONES && corpse->GetMapId() == GetPlayer()->GetMapId())
    {
        if (mEntry->IsDungeon())
        {
            GetPlayer()->ResurrectPlayer(0.5f,false);
            GetPlayer()->SpawnCorpseBones();
        }
    }

    bool allowMount = !mEntry->IsDungeon() || mEntry->IsBattlegroundOrArena();
    if (mInstance)
    {
        Difficulty diff = GetPlayer()->GetDifficulty(mEntry->IsRaid());
        if (MapDifficulty const* mapDiff = GetMapDifficultyData(mEntry->MapID,diff))
        {
            if (mapDiff->resetTime)
            {
                if (time_t timeReset = sInstanceSaveMgr->GetResetTimeFor(mEntry->MapID,diff))
                {
                    uint32 timeleft = uint32(timeReset - time(NULL));
                    GetPlayer()->SendInstanceResetWarning(mEntry->MapID, diff, timeleft);
                }
            }
        }
        allowMount = mInstance->allowMount;
    }

    // mount allow check
    if (!allowMount)
        _player->RemoveAurasByType(SPELL_AURA_MOUNTED);

    // update zone immediately, otherwise leave channel will cause crash in mtmap
    uint32 newzone, newarea;
    GetPlayer()->GetZoneAndAreaId(newzone, newarea);
    GetPlayer()->UpdateZone(newzone, newarea);

    // honorless target
    if (GetPlayer()->pvpInfo.inHostileArea)
        GetPlayer()->CastSpell(GetPlayer(), 2479, true);

    // in friendly area
    else if (GetPlayer()->IsPvP() && !GetPlayer()->HasFlag(PLAYER_FLAGS,PLAYER_FLAGS_IN_PVP))
        GetPlayer()->UpdatePvP(false, false);

    // resummon pet
    GetPlayer()->ResummonPetTemporaryUnSummonedIfAny();

    //lets process all delayed operations on successful teleport
    GetPlayer()->ProcessDelayedOperations();
}

void WorldSession::HandleMoveTeleportAck(WorldPacket& recv_data)
{
    sLog->outDebug("MSG_MOVE_TELEPORT_ACK");

    ObjectGuid guid;
    uint32 flags, time;
    recv_data >> flags >> time;

    guid[5] = recv_data.ReadBit();
    guid[0] = recv_data.ReadBit();
    guid[1] = recv_data.ReadBit();
    guid[6] = recv_data.ReadBit();
    guid[3] = recv_data.ReadBit();
    guid[7] = recv_data.ReadBit();
    guid[2] = recv_data.ReadBit();
    guid[4] = recv_data.ReadBit();

    recv_data.ReadByteSeq(guid[4]);
    recv_data.ReadByteSeq(guid[2]);
    recv_data.ReadByteSeq(guid[7]);
    recv_data.ReadByteSeq(guid[6]);
    recv_data.ReadByteSeq(guid[5]);
    recv_data.ReadByteSeq(guid[1]);
    recv_data.ReadByteSeq(guid[3]);
    recv_data.ReadByteSeq(guid[0]);

    sLog->outStaticDebug("Guid " UI64FMTD, (uint64)guid);
    sLog->outStaticDebug("Flags %u, time %u", flags, time/IN_MILLISECONDS);

    Unit *mover = _player->m_mover;
    Player *plMover = mover->GetTypeId() == TYPEID_PLAYER ? (Player*)mover : NULL;

    if (!plMover || !plMover->IsBeingTeleportedNear())
        return;

    if (guid != plMover->GetGUID())
        return;

    plMover->SetSemaphoreTeleportNear(false);

    uint32 old_zone = plMover->GetZoneId();

    WorldLocation const& dest = plMover->GetTeleportDest();

    plMover->SetPosition(dest,true);

    uint32 newzone, newarea;
    plMover->GetZoneAndAreaId(newzone, newarea);
    plMover->UpdateZone(newzone, newarea);

    // new zone
    if (old_zone != newzone)
    {
        // honorless target
        if (plMover->pvpInfo.inHostileArea)
            plMover->CastSpell(plMover, 2479, true);

        // in friendly area
        else if (plMover->IsPvP() && !plMover->HasFlag(PLAYER_FLAGS,PLAYER_FLAGS_IN_PVP))
            plMover->UpdatePvP(false, false);
    }

    // resummon pet
    GetPlayer()->ResummonPetTemporaryUnSummonedIfAny();

    //lets process all delayed operations on successful teleport
    GetPlayer()->ProcessDelayedOperations();
}

void WorldSession::HandleMovementOpcodes(WorldPacket & recv_data)
{
    uint32 opcode = recv_data.GetOpcode();
    recv_data.hexlike();

    Unit *mover = _player->m_mover;

    ASSERT(mover != NULL);                                  // there must always be a mover

    Player *plMover = mover->ToPlayer();

    // ignore, waiting processing in WorldSession::HandleMoveWorldportAckOpcode and WorldSession::HandleMoveTeleportAck
    if (plMover && plMover->IsBeingTeleported())
    {
        recv_data.rpos(recv_data.wpos());                   // prevent warnings spam
        return;
    }

    /* extract packet */
    MovementInfo movementInfo;
    ReadMovementInfo(recv_data, &movementInfo);

    recv_data.rpos(recv_data.wpos());                   // prevent warnings spam

    // prevent tampered movement data
    if (movementInfo.guid != mover->GetGUID())
        return;

    if (!movementInfo.pos.IsPositionValid())
    {
        recv_data.rpos(recv_data.wpos());                   // prevent warnings spam
        return;
    }

    // stop some emotes at player move
    if (plMover && (plMover->GetUInt32Value(UNIT_NPC_EMOTESTATE) != 0))
        plMover->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);

    /* handle special cases */
    if (movementInfo.t_guid)
    {
        // transports size limited
        // (also received at zeppelin leave by some reason with t_* as absolute in continent coordinates, can be safely skipped)
        if (movementInfo.t_pos.GetPositionX() > 50 || movementInfo.t_pos.GetPositionY() > 50 || movementInfo.t_pos.GetPositionZ() > 50)
        {
            recv_data.rpos(recv_data.wpos());                   // prevent warnings spam
            return;
        }

        if (!Trinity::IsValidMapCoord(movementInfo.pos.GetPositionX() + movementInfo.t_pos.GetPositionX(), movementInfo.pos.GetPositionY() + movementInfo.t_pos.GetPositionY(),
            movementInfo.pos.GetPositionZ() + movementInfo.t_pos.GetPositionZ(), movementInfo.pos.GetOrientation() + movementInfo.t_pos.GetOrientation()))
        {
            recv_data.rpos(recv_data.wpos());                   // prevent warnings spam
            return;
        }

        // if we boarded a transport, add us to it
        if (plMover)
        {
            TransportBase* transBase = plMover->GetTransport();
            if (!transBase)
            {
                // elevators also cause the client to send MOVEMENTFLAG_ONTRANSPORT - just dismount if the guid can be found in the transport list
                if (Transport* transport = plMover->GetMap()->GetTransport(movementInfo.t_guid))
                {
                    plMover->m_transport = transport;
                    transport->AddPassenger(plMover);
                }
                else if (GameObject* dynTransport = plMover->GetMap()->GetGameObject(movementInfo.t_guid))
                {
                    if (DynamicTransport* transport = dynTransport->ToDynamicTransport())
                    {
                        plMover->m_transport = transport;
                        transport->AddPassenger(plMover);
                    }
                }
            }
            else if (transBase->ToGameObject() && transBase->ToGameObject()->GetGUID() != movementInfo.t_guid)
            {
                bool foundNewTransport = false;
                plMover->m_transport->RemovePassenger(plMover);
                if (Transport* transport = plMover->GetMap()->GetTransport(movementInfo.t_guid))
                {
                    foundNewTransport = true;
                    plMover->m_transport = transport;
                    transport->AddPassenger(plMover);
                }
                else if (GameObject* dynTransport = plMover->GetMap()->GetGameObject(movementInfo.t_guid))
                {
                    DynamicTransport* transport = dynTransport->ToDynamicTransport();
                    if (dynTransport->IsDynamicTransport() && transport != NULL)
                    {
                        foundNewTransport = true;
                        plMover->m_transport = transport;
                        transport->AddPassenger(plMover);
                    }
                }

                if (!foundNewTransport)
                {
                    plMover->m_transport = NULL;
                    movementInfo.t_pos.Relocate(0.0f, 0.0f, 0.0f, 0.0f);
                    movementInfo.t_time = 0;
                    movementInfo.t_seat = -1;
                }
            }
        }

        if (!mover->GetTransport() && !mover->GetVehicle())
        {
            GameObject *go = mover->GetMap()->GetGameObject(movementInfo.t_guid);
            if (!go || go->GetGoType() != GAMEOBJECT_TYPE_TRANSPORT)
                movementInfo.t_guid = 0;
        }
    }
    else if (plMover && plMover->GetTransport())                // if we were on a transport, leave
    {
        plMover->m_transport->RemovePassenger(plMover);
        plMover->m_transport = NULL;
        movementInfo.t_pos.Relocate(0.0f, 0.0f, 0.0f, 0.0f);
        movementInfo.t_time = 0;
        movementInfo.t_seat = -1;
    }

    // fall damage generation (ignore in flight case that can be triggered also at lags in moment teleportation to another map).
    if (opcode == MSG_MOVE_FALL_LAND && plMover && !plMover->IsInFlight())
        plMover->HandleFall(movementInfo);

    if (plMover && ((movementInfo.flags & MOVEMENTFLAG_SWIMMING) != 0) != plMover->IsInWater())
    {
        // now client not include swimming flag in case jumping under water
        plMover->SetInWater(!plMover->IsInWater() || plMover->GetBaseMap()->IsUnderWater(movementInfo.pos.GetPositionX(), movementInfo.pos.GetPositionY(), movementInfo.pos.GetPositionZ()));
    }

    /*----------------------*/

    movementInfo.time = getMSTime();
    movementInfo.guid = mover->GetGUID();
    mover->m_movementInfo = movementInfo;

    /* process position-change */
    // this is almost never true (not sure why it is sometimes, but it is), normally use mover->IsVehicle()
    if (mover->GetVehicle())
    {
        mover->SetOrientation(movementInfo.pos.GetOrientation());
        return;
    }

    mover->SetPosition(movementInfo.pos);

    WorldPacket data(SMSG_PLAYER_MOVE, recv_data.size());
    WriteMovementInfo(data, &movementInfo);
    mover->SendMessageToSet(&data, _player);

    if (plMover)                                            // nothing is charmed, or player charmed
    {
        if (plMover->GetEmoteState() != 0 && opcode == MSG_MOVE_START_FORWARD && opcode != MSG_MOVE_SET_FACING &&
            opcode != MSG_MOVE_START_TURN_LEFT && opcode != MSG_MOVE_START_TURN_RIGHT &&
            opcode != MSG_MOVE_STOP_TURN)
            plMover->SetEmoteState(0);

        plMover->UpdateFallInformationIfNeed(movementInfo, opcode);

        AreaTableEntry const* zone = GetAreaEntryByAreaID(plMover->GetAreaId());
        float depth = zone ? zone->MaxDepth : -500.0f;

        if (movementInfo.pos.GetPositionZ() < depth)
        {
            if (!(plMover->InBattleground()
                && plMover->GetBattleground()
                && plMover->GetBattleground()->HandlePlayerUnderMap(_player)))
            {
                // NOTE: this is actually called many times while falling
                // even after the player has been teleported away
                // TODO: discard movement packets after the player is rooted
                if (plMover->IsAlive())
                {
                    plMover->EnvironmentalDamage(DAMAGE_FALL_TO_VOID, GetPlayer()->GetMaxHealth());
                    // pl can be alive if GM/etc
                    if (!plMover->IsAlive())
                    {
                        // change the death state to CORPSE to prevent the death timer from
                        // starting in the next player update
                        plMover->KillPlayer();
                        plMover->BuildPlayerRepop();
                    }
                }

                // cancel the death timer here if started
                plMover->RepopAtGraveyard();
            }
        }

        switch (opcode)
        {
            case MSG_MOVE_START_FORWARD:
            case MSG_MOVE_START_STRAFE_LEFT:
            case MSG_MOVE_START_STRAFE_RIGHT:
            case MSG_MOVE_START_BACKWARD:
                plMover->GetAntiHackServant()->CheckSpeedFrames(SPEED_CHECK_MOVE_START);
                break;
            case MSG_MOVE_HEARTBEAT:
                plMover->GetAntiHackServant()->CheckSpeedFrames(SPEED_CHECK_MOVE_UPDATE);
                break;
            case MSG_MOVE_STOP:
                plMover->GetAntiHackServant()->CheckSpeedFrames(SPEED_CHECK_MOVE_STOP);
                break;
        }
    }
}

void WorldSession::ReadMovementInfo(WorldPacket &data, MovementInfo *mi, ExtraMovementStatusElement* miextra)
{
    bool hasMovementFlags = false;
    bool hasMovementFlags2 = false;
    bool hasTimestamp = false;
    bool hasOrientation = false;
    bool hasTransportData = false;
    bool hasTransportTime2 = false;
    bool hasTransportVehicleId = false;
    bool hasPitch = false;
    bool hasFallData = false;
    bool hasFallDirection = false;
    bool hasSplineElevation = false;
    /*bool hasSpline = false;*/ // unused (for now)

    MovementStatusElements const* sequence = GetMovementStatusElementsSequence(data.GetOpcode());
    if (sequence == NULL)
        return;

    ObjectGuid guid;
    ObjectGuid tguid;

    for (uint32 i = 0; i < MSE_COUNT; ++i)
    {
        MovementStatusElements element = sequence[i];
        if (element == MSEEnd)
            break;

        if (element >= MSEHasGuidByte0 && element <= MSEHasGuidByte7)
        {
            guid[element - MSEHasGuidByte0] = data.ReadBit();
            continue;
        }

        if (element >= MSEHasTransportGuidByte0 &&
            element <= MSEHasTransportGuidByte7)
        {
            if (hasTransportData)
                tguid[element - MSEHasTransportGuidByte0] = data.ReadBit();
            continue;
        }

        if (element >= MSEGuidByte0 && element <= MSEGuidByte7)
        {
            data.ReadByteSeq(guid[element - MSEGuidByte0]);
            continue;
        }

        if (element >= MSETransportGuidByte0 &&
            element <= MSETransportGuidByte7)
        {
            if (hasTransportData)
                data.ReadByteSeq(tguid[element - MSETransportGuidByte0]);
            continue;
        }

        switch (element)
        {
            case MSEHasMovementFlags:
                hasMovementFlags = !data.ReadBit();
                break;
            case MSEHasMovementFlags2:
                hasMovementFlags2 = !data.ReadBit();
                break;
            case MSEHasTimestamp:
                hasTimestamp = !data.ReadBit();
                break;
            case MSEHasOrientation:
                hasOrientation = !data.ReadBit();
                break;
            case MSEHasTransportData:
                hasTransportData = data.ReadBit();
                break;
            case MSEHasTransportTime2:
                if (hasTransportData)
                    hasTransportTime2 = data.ReadBit();
                break;
            case MSEHasTransportTime3:
                if (hasTransportData)
                    hasTransportVehicleId = data.ReadBit();
                break;
            case MSEHasPitch:
                hasPitch = !data.ReadBit();
                break;
            case MSEHasFallData:
                hasFallData = data.ReadBit();
                break;
            case MSEHasFallDirection:
                if (hasFallData)
                    hasFallDirection = data.ReadBit();
                break;
            case MSEHasSplineElevation:
                hasSplineElevation = !data.ReadBit();
                break;
            case MSEHasSpline:
                /*hasSpline = */data.ReadBit();
                break;
            case MSEMovementFlags:
                if (hasMovementFlags)
                    mi->flags = data.ReadBits(30);
                break;
            case MSEMovementFlags2:
                if (hasMovementFlags2)
                    mi->flags2 = data.ReadBits(12);
                break;
            case MSETimestamp:
                if (hasTimestamp)
                    data >> mi->time;
                break;
            case MSEPositionX:
                data >> mi->pos.m_positionX;
                break;
            case MSEPositionY:
                data >> mi->pos.m_positionY;
                break;
            case MSEPositionZ:
                data >> mi->pos.m_positionZ;
                break;
            case MSEOrientation:
                if (hasOrientation)
                    mi->pos.SetOrientation(data.read<float>());
                break;
            case MSETransportPositionX:
                if (hasTransportData)
                    data >> mi->t_pos.m_positionX;
                break;
            case MSETransportPositionY:
                if (hasTransportData)
                    data >> mi->t_pos.m_positionY;
                break;
            case MSETransportPositionZ:
                if (hasTransportData)
                    data >> mi->t_pos.m_positionZ;
                break;
            case MSETransportOrientation:
                if (hasTransportData)
                    mi->t_pos.SetOrientation(data.read<float>());
                break;
            case MSETransportSeat:
                if (hasTransportData)
                    data >> mi->t_seat;
                break;
            case MSETransportTime:
                if (hasTransportData)
                    data >> mi->t_time;
                break;
            case MSETransportTime2:
                if (hasTransportData && hasTransportTime2)
                    data >> mi->t_time2;
                break;
            case MSETransportVehicleId:
                if (hasTransportData && hasTransportVehicleId)
                    data >> mi->t_vehicleId;
                break;
            case MSEPitch:
                if (hasPitch)
                    data >> mi->pitch;
                break;
            case MSEFallTime:
                if (hasFallData)
                    data >> mi->fallTime;
                break;
            case MSEFallVerticalSpeed:
                if (hasFallData)
                    data >> mi->j_zspeed;
                break;
            case MSEFallCosAngle:
                if (hasFallData && hasFallDirection)
                    data >> mi->j_cosAngle;
                break;
            case MSEFallSinAngle:
                if (hasFallData && hasFallDirection)
                    data >> mi->j_sinAngle;
                break;
            case MSEFallHorizontalSpeed:
                if (hasFallData && hasFallDirection)
                    data >> mi->j_xyspeed;
                break;
            case MSESplineElevation:
                if (hasSplineElevation)
                    data >> mi->splineElevation;
                break;
            case MSECounter:
                data.read_skip<uint32>();   /// @TODO: Maybe compare it with m_movementCounter to verify that packets are sent & received in order?
                break;
            case MSEZeroBit:
            case MSEOneBit:
                data.ReadBit();
                break;
            case MSEExtraElement:
                if (miextra)
                    miextra->ReadNextElement(data);
                break;
            default:
                ASSERT(false && "Incorrect sequence element detected at ReadMovementInfo");
                break;
        }
    }

    mi->guid = guid;
    mi->t_guid = tguid;

    //! Anti-cheat checks. Please keep them in seperate if() blocks to maintain a clear overview.
    //! Might be subject to latency, so just remove improper flags.
    #ifdef TRINITY_DEBUG
    #define REMOVE_VIOLATING_FLAGS(check, maskToRemove) \
    { \
        if (check) \
        { \
            sLog->outDebug("WorldSession::ReadMovementInfo: Violation of MovementFlags found (%s). " \
                "MovementFlags: %u, MovementFlags2: %u for player GUID: %u. Mask %u will be removed.", \
                STRINGIZE(check), mi->GetMovementFlags(), mi->GetExtraMovementFlags(), GetPlayer()->GetGUIDLow(), maskToRemove); \
            mi->RemoveMovementFlag((maskToRemove)); \
        } \
    }
    #else
    #define REMOVE_VIOLATING_FLAGS(check, maskToRemove) \
        if (check) \
            mi->RemoveMovementFlag((maskToRemove));
    #endif


    /*! This must be a packet spoofing attempt. MOVEMENTFLAG_ROOT sent from the client is not valid
        in conjunction with any of the moving movement flags such as MOVEMENTFLAG_FORWARD.
        It will freeze clients that receive this player's movement info.
    */
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_ROOT),
        MOVEMENTFLAG_ROOT);

    //! Cannot hover without SPELL_AURA_HOVER
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_HOVER) && !GetPlayer()->HasAuraType(SPELL_AURA_HOVER),
        MOVEMENTFLAG_HOVER);

    //! Cannot ascend and descend at the same time
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_ASCENDING) && mi->HasMovementFlag(MOVEMENTFLAG_DESCENDING),
        MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING);

    //! Cannot move left and right at the same time
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_LEFT) && mi->HasMovementFlag(MOVEMENTFLAG_RIGHT),
        MOVEMENTFLAG_LEFT | MOVEMENTFLAG_RIGHT);

    //! Cannot strafe left and right at the same time
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_STRAFE_LEFT) && mi->HasMovementFlag(MOVEMENTFLAG_STRAFE_RIGHT),
        MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT);

    //! Cannot pitch up and down at the same time
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_PITCH_UP) && mi->HasMovementFlag(MOVEMENTFLAG_PITCH_DOWN),
        MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN);

    //! Cannot move forwards and backwards at the same time
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_FORWARD) && mi->HasMovementFlag(MOVEMENTFLAG_BACKWARD),
        MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD);

    //! Cannot walk on water without SPELL_AURA_WATER_WALK
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_WATERWALKING) && !GetPlayer()->HasAuraType(SPELL_AURA_WATER_WALK),
        MOVEMENTFLAG_WATERWALKING);

    //! Cannot feather fall without SPELL_AURA_FEATHER_FALL
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_FALLING_SLOW) && !GetPlayer()->HasAuraType(SPELL_AURA_FEATHER_FALL),
        MOVEMENTFLAG_FALLING_SLOW);

    /*! Cannot fly if no fly auras present. Exception is being a GM.
        Note that we check for account level instead of Player::IsGameMaster() because in some
        situations it may be feasable to use .gm fly on as a GM without having .gm on,
        e.g. aerial combat.
    */

    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_MASK_MOVING_FLY | MOVEMENTFLAG_CAN_FLY) && GetSecurity() == SEC_PLAYER &&
        !GetPlayer()->m_mover->HasAuraType(SPELL_AURA_FLY) &&
        !GetPlayer()->m_mover->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED) &&
        !GetPlayer()->m_mover->HasAuraType(SPELL_AURA_MOD_MOUNTED_FLIGHT_SPEED_ALWAYS),
        MOVEMENTFLAG_MASK_MOVING_FLY | MOVEMENTFLAG_CAN_FLY);

    #undef REMOVE_VIOLATING_FLAGS
}

void WorldSession::WriteMovementInfo(WorldPacket &data, MovementInfo *mi, ExtraMovementStatusElement* miextra)
{
    bool hasMovementFlags = mi->GetMovementFlags() != 0;
    bool hasMovementFlags2 = mi->GetExtraMovementFlags() != 0;
    bool hasTimestamp = mi->time != 0;
    bool hasOrientation = !G3D::fuzzyEq(mi->pos.GetOrientation(), 0.0f);
    bool hasTransportData = mi->t_guid != 0;
    bool hasTransportTime2 = mi->HasExtraMovementFlag(MOVEMENTFLAG2_INTERPOLATED_MOVEMENT);
    bool hasTransportVehicleId = hasTransportData && mi->t_vehicleId != 0;
    bool hasPitch = mi->HasMovementFlag(MovementFlags(MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) || mi->HasExtraMovementFlag(MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING);
    bool hasFallData = mi->HasMovementFlag(MOVEMENTFLAG_FALLING);
    bool hasFallDirection = mi->HasMovementFlag(MOVEMENTFLAG_FALLING);
    bool hasSplineElevation = mi->HasMovementFlag(MOVEMENTFLAG_SPLINE_ELEVATION);
    bool hasSpline = false;

    MovementStatusElements const* sequence = GetMovementStatusElementsSequence(data.GetOpcode());
    if (!sequence)
    {
        sLog->outError("WorldSession::WriteMovementInfo: No movement sequence found for opcode 0x%04X", uint32(data.GetOpcode()));
        return;
    }

    ObjectGuid guid = mi->guid;
    ObjectGuid tguid = mi->t_guid;

    for(uint32 i = 0; i < MSE_COUNT; ++i)
    {
        MovementStatusElements element = sequence[i];
        if (element == MSEEnd)
            break;

        if (element >= MSEHasGuidByte0 && element <= MSEHasGuidByte7)
        {
            data.WriteBit(guid[element - MSEHasGuidByte0]);
            continue;
        }

        if (element >= MSEHasTransportGuidByte0 &&
            element <= MSEHasTransportGuidByte7)
        {
            if (hasTransportData)
                data.WriteBit(tguid[element - MSEHasTransportGuidByte0]);
            continue;
        }

        if (element >= MSEGuidByte0 && element <= MSEGuidByte7)
        {
            data.WriteByteSeq(guid[element - MSEGuidByte0]);
            continue;
        }

        if (element >= MSETransportGuidByte0 &&
            element <= MSETransportGuidByte7)
        {
            if (hasTransportData)
                data.WriteByteSeq(tguid[element - MSETransportGuidByte0]);
            continue;
        }

        switch (element)
        {
        case MSEHasMovementFlags:
            data.WriteBit(!hasMovementFlags);
            break;
        case MSEHasMovementFlags2:
            data.WriteBit(!hasMovementFlags2);
            break;
        case MSEHasTimestamp:
            data.WriteBit(!hasTimestamp);
            break;
        case MSEHasOrientation:
            data.WriteBit(!hasOrientation);
            break;
        case MSEHasTransportData:
            data.WriteBit(hasTransportData);
            break;
        case MSEHasTransportTime2:
            if (hasTransportData)
                data.WriteBit(hasTransportTime2);
            break;
        case MSEHasTransportTime3:
            if (hasTransportData)
                data.WriteBit(hasTransportVehicleId);
            break;
        case MSEHasPitch:
            data.WriteBit(!hasPitch);
            break;
        case MSEHasFallData:
            data.WriteBit(hasFallData);
            break;
        case MSEHasFallDirection:
            if (hasFallData)
                data.WriteBit(hasFallDirection);
            break;
        case MSEHasSplineElevation:
            data.WriteBit(!hasSplineElevation);
            break;
        case MSEHasSpline:
            data.WriteBit(hasSpline);
            break;
        case MSEMovementFlags:
            if (hasMovementFlags)
                data.WriteBits(mi->flags, 30);
            break;
        case MSEMovementFlags2:
            if (hasMovementFlags2)
                data.WriteBits(mi->flags2, 12);
            break;
        case MSETimestamp:
            if (hasTimestamp)
                data << mi->time;
            break;
        case MSEPositionX:
            data << mi->pos.m_positionX;
            break;
        case MSEPositionY:
            data << mi->pos.m_positionY;
            break;
        case MSEPositionZ:
            data << mi->pos.m_positionZ;
            break;
        case MSEOrientation:
            if (hasOrientation)
                data << mi->pos.GetOrientation();
            break;
        case MSETransportPositionX:
            if (hasTransportData)
                data << mi->t_pos.m_positionX;
            break;
        case MSETransportPositionY:
            if (hasTransportData)
                data << mi->t_pos.m_positionY;
            break;
        case MSETransportPositionZ:
            if (hasTransportData)
                data << mi->t_pos.m_positionZ;
            break;
        case MSETransportOrientation:
            if (hasTransportData)
                data << mi->t_pos.GetOrientation();
            break;
        case MSETransportSeat:
            if (hasTransportData)
                data << mi->t_seat;
            break;
        case MSETransportTime:
            if (hasTransportData)
                data << mi->t_time;
            break;
        case MSETransportTime2:
            if (hasTransportData && hasTransportTime2)
                data << mi->t_time2;
            break;
        case MSETransportVehicleId:
            if (hasTransportData && hasTransportVehicleId)
                data << mi->t_vehicleId;
            break;
        case MSEPitch:
            if (hasPitch)
                data << mi->pitch;
            break;
        case MSEFallTime:
            if (hasFallData)
                data << mi->fallTime;
            break;
        case MSEFallVerticalSpeed:
            if (hasFallData)
                data << mi->j_zspeed;
            break;
        case MSEFallCosAngle:
            if (hasFallData && hasFallDirection)
                data << mi->j_cosAngle;
            break;
        case MSEFallSinAngle:
            if (hasFallData && hasFallDirection)
                data << mi->j_sinAngle;
            break;
        case MSEFallHorizontalSpeed:
            if (hasFallData && hasFallDirection)
                data << mi->j_xyspeed;
            break;
        case MSESplineElevation:
            if (hasSplineElevation)
                data << mi->splineElevation;
            break;
        case MSECounter:
            data << mi->movementCounter++;
            break;
        case MSEZeroBit:
            data.WriteBit(0);
            break;
        case MSEOneBit:
            data.WriteBit(1);
            break;
        case MSEExtraElement:
            if (miextra)
                miextra->WriteNextElement(data);
            break;
        default:
            ASSERT(false && "Incorrect sequence element detected at ReadMovementInfo");
            break;
        }
    }
}

void WorldSession::HandleForceSpeedChangeAck(WorldPacket &recv_data)
{
    uint32 opcode = recv_data.GetOpcode();
    sLog->outDebug("WORLD: Recvd %s (%u, 0x%X) opcode", LookupOpcodeName(opcode), opcode, opcode);

    /* extract packet */
    uint64 guid;
    uint32 unk1;
    float  newspeed;

    recv_data.readPackGUID(guid);

    // now can skip not our packet
    if (_player->GetGUID() != guid)
    {
        recv_data.rpos(recv_data.wpos());                   // prevent warnings spam
        return;
    }

    // continue parse packet

    recv_data >> unk1;                                      // counter or moveEvent

    MovementInfo movementInfo;
    movementInfo.guid = guid;
    ReadMovementInfo(recv_data, &movementInfo);

    recv_data >> newspeed;
    /*----------------*/

    // client ACK send one packet for mounted/run case and need skip all except last from its
    // in other cases anti-cheat check can be fail in false case
    UnitMoveType move_type;
    UnitMoveType force_move_type;

    static char const* move_type_name[MAX_MOVE_TYPE] = {  "Walk", "Run", "RunBack", "Swim", "SwimBack", "TurnRate", "Flight", "FlightBack", "PitchRate" };

    switch(opcode)
    {
        case CMSG_FORCE_WALK_SPEED_CHANGE_ACK:          move_type = MOVE_WALK;          force_move_type = MOVE_WALK;        break;
        case CMSG_FORCE_RUN_SPEED_CHANGE_ACK:           move_type = MOVE_RUN;           force_move_type = MOVE_RUN;         break;
        case CMSG_FORCE_RUN_BACK_SPEED_CHANGE_ACK:      move_type = MOVE_RUN_BACK;      force_move_type = MOVE_RUN_BACK;    break;
        case CMSG_FORCE_SWIM_SPEED_CHANGE_ACK:          move_type = MOVE_SWIM;          force_move_type = MOVE_SWIM;        break;
        case CMSG_FORCE_SWIM_BACK_SPEED_CHANGE_ACK:     move_type = MOVE_SWIM_BACK;     force_move_type = MOVE_SWIM_BACK;   break;
        case CMSG_FORCE_TURN_RATE_CHANGE_ACK:           move_type = MOVE_TURN_RATE;     force_move_type = MOVE_TURN_RATE;   break;
        case CMSG_FORCE_FLIGHT_SPEED_CHANGE_ACK:        move_type = MOVE_FLIGHT;        force_move_type = MOVE_FLIGHT;      break;
        case CMSG_FORCE_FLIGHT_BACK_SPEED_CHANGE_ACK:   move_type = MOVE_FLIGHT_BACK;   force_move_type = MOVE_FLIGHT_BACK; break;
        case CMSG_FORCE_PITCH_RATE_CHANGE_ACK:          move_type = MOVE_PITCH_RATE;    force_move_type = MOVE_PITCH_RATE;  break;
        default:
            sLog->outError("WorldSession::HandleForceSpeedChangeAck: Unknown move type opcode: %u", opcode);
            return;
    }

    // skip all forced speed changes except last and unexpected
    // in run/mounted case used one ACK and it must be skipped.m_forced_speed_changes[MOVE_RUN} store both.
    if (_player->m_forced_speed_changes[force_move_type] > 0)
    {
        --_player->m_forced_speed_changes[force_move_type];
        if (_player->m_forced_speed_changes[force_move_type] > 0)
            return;
    }

    if (!_player->GetTransport() && fabs(_player->GetSpeed(move_type) - newspeed) > 0.01f)
    {
        if (_player->GetSpeed(move_type) > newspeed)         // must be greater - just correct
        {
            sLog->outError("%sSpeedChange player %s is NOT correct (must be %f instead %f), force set to correct value",
                move_type_name[move_type], _player->GetName(), _player->GetSpeed(move_type), newspeed);
            _player->SetSpeed(move_type,_player->GetSpeedRate(move_type),true);
            if (move_type == MOVE_TURN_RATE)
                _player->SetSpeed(MOVE_TURN_RATE,1.0f,true);
        }
        else                                                // must be lesser - cheating
        {
            sLog->outBasic("Player %s from account id %u kicked for incorrect speed (must be %f instead %f)",
                _player->GetName(),_player->GetSession()->GetAccountId(),_player->GetSpeed(move_type), newspeed);
            _player->GetSession()->KickPlayer();
        }
    }
}

void WorldSession::HandleSetActiveMoverOpcode(WorldPacket &recv_data)
{
    sLog->outDebug("WORLD: Recvd CMSG_SET_ACTIVE_MOVER");

    ObjectGuid guid;

    guid[7] = recv_data.ReadBit();
    guid[2] = recv_data.ReadBit();
    guid[1] = recv_data.ReadBit();
    guid[0] = recv_data.ReadBit();
    guid[4] = recv_data.ReadBit();
    guid[5] = recv_data.ReadBit();
    guid[6] = recv_data.ReadBit();
    guid[3] = recv_data.ReadBit();

    recv_data.ReadByteSeq(guid[3]);
    recv_data.ReadByteSeq(guid[2]);
    recv_data.ReadByteSeq(guid[4]);
    recv_data.ReadByteSeq(guid[0]);
    recv_data.ReadByteSeq(guid[5]);
    recv_data.ReadByteSeq(guid[1]);
    recv_data.ReadByteSeq(guid[6]);
    recv_data.ReadByteSeq(guid[7]);

    // do not re-set the active mover if it didn't change
    if (guid == _player->m_mover->GetGUID())
        return;
    // Anti-cheat check
    if (guid != _player->GetCharmGUID() && guid != _player->GetGUID())
    {
        sLog->outError("Player %s is trying to change mover to an invalid value!", _player->GetName());
        GetPlayer()->SetMover(GetPlayer());
        return;
    }

    if (GetPlayer()->IsInWorld())
    {
        if (Unit *mover = ObjectAccessor::GetUnit(*GetPlayer(), guid))
        {
            GetPlayer()->SetMover(mover);
            if (mover != GetPlayer() && mover->CanFly())
            {
                GetPlayer()->SetSendFlyPacket(true);
            }
        }
        else
        {
            sLog->outError("HandleSetActiveMoverOpcode: incorrect mover guid: mover is " UI64FMTD " and should be " UI64FMTD, (uint64)guid, _player->m_mover->GetGUID());
            GetPlayer()->SetMover(GetPlayer());
        }
    }
}

void WorldSession::HandleMoveNotActiveMover(WorldPacket &recv_data)
{
    sLog->outDebug("WORLD: Recvd CMSG_MOVE_NOT_ACTIVE_MOVER");

    MovementInfo mi;
    ReadMovementInfo(recv_data, &mi);
    _player->m_movementInfo = mi;
}

void WorldSession::HandleDismissControlledVehicle(WorldPacket &recv_data)
{
    sLog->outDebug("WORLD: Recvd CMSG_DISMISS_CONTROLLED_VEHICLE");
    recv_data.hexlike();

    uint64 vehicleGUID = _player->GetCharmGUID();

    if (!vehicleGUID)                                        // something wrong here...
    {
        recv_data.rpos(recv_data.wpos());                   // prevent warnings spam
        return;
    }

    MovementInfo mi;
    ReadMovementInfo(recv_data, &mi);
    _player->m_movementInfo = mi;

    _player->ExitVehicle();
}

void WorldSession::HandleChangeSeatsOnControlledVehicle(WorldPacket &recv_data)
{
    sLog->outDebug("WORLD: Recvd CMSG_CHANGE_SEATS_ON_CONTROLLED_VEHICLE");
    recv_data.hexlike();

    Unit* vehicle_base = GetPlayer()->GetVehicleBase();
    if (!vehicle_base)
        return;

    switch (recv_data.GetOpcode())
    {
        case CMSG_REQUEST_VEHICLE_PREV_SEAT:
            GetPlayer()->ChangeSeat(-1, false);
            break;
        case CMSG_REQUEST_VEHICLE_NEXT_SEAT:
            GetPlayer()->ChangeSeat(-1, true);
            break;
        case CMSG_CHANGE_SEATS_ON_CONTROLLED_VEHICLE:
            {
                float x, y, z;
                int8 seatId;
                uint64 accessory = 0;

                bool hasPosition = false, hasUnkInt1 = false, hasUnkInt2 = false, hasUnkInt3 = false;

                ObjectGuid vg, ag, ng;

                recv_data >> y >> x >> z;
                recv_data >> seatId;

                bool someGuid = recv_data.ReadBit(); // v2 + 10 !
                bool hasAdditionalData = recv_data.ReadBit(); // v2 + 72 // accessory?
                ag[2] = recv_data.ReadBit(); // v2 + 34
                ag[6] = recv_data.ReadBit(); // v2 + 38
                ag[4] = recv_data.ReadBit(); // v2 + 36
                vg[2] = recv_data.ReadBit(); // v2 + 18
                vg[4] = recv_data.ReadBit(); // v2 + 20
                recv_data.ReadBit(); // ??

                recv_data.ReadBit(); // v2 + 165
                vg[7] = recv_data.ReadBit(); // v2 + 23
                ag[7] = recv_data.ReadBit(); // v2 + 39
                vg[6] = recv_data.ReadBit(); // v2 + 22
                hasUnkInt3 = !recv_data.ReadBit(); // v2 + 12 !
                recv_data.ReadBit(); // ??
                ag[5] = recv_data.ReadBit(); // v2 + 37
                vg[5] = recv_data.ReadBit(); // v2 + 21

                recv_data.ReadBit(); // v2 + 22 !
                recv_data.ReadBit(); // ??
                vg[0] = recv_data.ReadBit(); // v2 + 16
                ag[0] = recv_data.ReadBit(); // v2 + 32
                vg[1] = recv_data.ReadBit(); // v2 + 17
                bool unkFlags = recv_data.ReadBit(); // v2 + 132
                ag[1] = recv_data.ReadBit(); // v2 + 33
                recv_data.ReadBit(); // v2 + 164

                ag[3] = recv_data.ReadBit(); // v2 + 35
                vg[3] = recv_data.ReadBit(); // v2 + 19

                if (!someGuid)
                {
                    for (uint32 i = 0; i < 24; i += 8)
                        recv_data.read_skip<uint8>();

                    //recv_data.read_skip<uint8>();
                }

                if (hasAdditionalData)
                {
                    // probably accessory data?

                    ng[3] = recv_data.ReadBit(); // v2 + 83
                    ng[0] = recv_data.ReadBit(); // v2 + 80
                    ng[7] = recv_data.ReadBit(); // v2 + 87
                    ng[5] = recv_data.ReadBit(); // v2 + 85
                    hasUnkInt2 = recv_data.ReadBit(); // v2 + 120
                    ng[1] = recv_data.ReadBit(); // v2 + 81
                    ng[2] = recv_data.ReadBit(); // v2 + 82
                    hasUnkInt1 = recv_data.ReadBit(); // v2 + 112
                    ng[4] = recv_data.ReadBit(); // v2 + 84
                    ng[6] = recv_data.ReadBit(); // v2 + 86

                    //recv_data.read_skip<uint8>();
                }

                recv_data.ReadBit(); // v2 + 81

                if (vg[6])
                {
                    recv_data.read_skip<uint8>(); // unk
                }

                if (unkFlags)
                {
                    hasPosition = recv_data.ReadBit(); // v2 + 144
                    recv_data.ReadBit(); // v13 ?
                }
                recv_data.FlushBits();

                recv_data.ReadByteSeq(ag[6]);
                recv_data.ReadByteSeq(vg[7]);
                recv_data.ReadByteSeq(vg[5]);
                recv_data.ReadByteSeq(ag[1]);
                recv_data.ReadByteSeq(ag[2]);
                recv_data.ReadByteSeq(vg[6]);
                recv_data.ReadByteSeq(ag[5]);
                recv_data.ReadByteSeq(ag[3]);
                recv_data.ReadByteSeq(vg[3]);
                recv_data.ReadByteSeq(ag[0]);
                recv_data.ReadByteSeq(vg[0]);
                recv_data.ReadByteSeq(ag[4]);
                recv_data.ReadByteSeq(vg[4]);
                recv_data.ReadByteSeq(vg[1]);
                recv_data.ReadByteSeq(ag[7]);
                recv_data.ReadByteSeq(vg[2]);

                if (unkFlags)
                {
                    if (hasPosition)
                    {
                        recv_data.read_skip<float>(); // y (v2 + 38)
                        recv_data.read_skip<float>(); // x (v2 + 37)
                        recv_data.read_skip<float>(); // z (v2 + 39)
                    }
                    recv_data.read_skip<uint32>(); // v2 + 34
                    recv_data.read_skip<float>();  // orientation? v2 + 35
                }

                if (hasAdditionalData)
                {
                    recv_data.ReadByteSeq(ng[2]);

                    if (hasUnkInt1)
                        recv_data.read_skip<uint32>(); // v2 + 29
                    if (hasUnkInt2)
                        recv_data.read_skip<uint32>(); // v2 + 31

                    recv_data.ReadByteSeq(ng[0]);

                    recv_data.read_skip<uint32>(); // v2 + 27
                    recv_data.read_skip<uint8>();  // v2 + 104
                    recv_data.read_skip<float>();  // v2 + 22 (x)
                    recv_data.read_skip<float>();  // v2 + 25 (orientation)

                    recv_data.ReadByteSeq(ng[7]);
                    recv_data.ReadByteSeq(ng[4]);
                    recv_data.ReadByteSeq(ng[3]);
                    recv_data.ReadByteSeq(ng[5]);

                    recv_data.read_skip<float>();  // v2 + 24 (z)

                    recv_data.ReadByteSeq(ng[1]);
                    recv_data.ReadByteSeq(ng[6]);

                    recv_data.read_skip<float>();  // v2 + 23 (y)
                }

                // condition for two floats? dont know how to deal with it
                //
                //

                if (hasUnkInt3)
                    recv_data.read_skip<uint32>();

                // Unfinished support for the rest of the packet, sorry
                recv_data.rfinish();

                if (vehicle_base->GetGUID() != vg)
                    return;

                // Due to received "move not active mover" packet, which sets the seatId by hard way,
                // we have to invalidate the seat to make us able to change it in the EnterVehicle/ChangeSeat functions
                if (GetPlayer()->GetTransSeat() == seatId)
                    GetPlayer()->m_movementInfo.t_seat = -1;

                accessory = ag;

                if (!accessory)
                    GetPlayer()->ChangeSeat(-1, seatId > 0); // prev/next
                else if (Unit *vehUnit = Unit::GetUnit(*GetPlayer(), accessory))
                {
                    if (Vehicle *vehicle = vehUnit->GetVehicleKit())
                        if (vehicle->HasEmptySeat(seatId))
                            GetPlayer()->EnterVehicle(vehicle, seatId);
                }

                // And if the change went wrong, change the seat back
                if (GetPlayer()->GetTransSeat() == -1)
                    GetPlayer()->m_movementInfo.t_seat = seatId;
            }
            break;
        case CMSG_REQUEST_VEHICLE_SWITCH_SEAT:
            {
                uint64 guid;        // current vehicle guid
                recv_data.readPackGUID(guid);

                int8 seatId;
                recv_data >> seatId;

                if (vehicle_base->GetGUID() == guid)
                    GetPlayer()->ChangeSeat(seatId);
                else if (Unit *vehUnit = Unit::GetUnit(*GetPlayer(), guid))
                    if (Vehicle *vehicle = vehUnit->GetVehicleKit())
                        if (vehicle->HasEmptySeat(seatId))
                            GetPlayer()->EnterVehicle(vehicle, seatId);
            }
            break;
        default:
            break;
    }
}

void WorldSession::HandleEnterPlayerVehicle(WorldPacket &data)
{
    // Read guid
    uint64 guid;
    data >> guid;

    if (Player* pl=ObjectAccessor::FindPlayer(guid))
    {
        if (!pl->GetVehicleKit())
            return;
        if (!pl->IsInRaidWith(_player))
            return;
        if (!pl->IsWithinDistInMap(_player,INTERACTION_DISTANCE))
            return;
        _player->EnterVehicle(pl);
    }
}

void WorldSession::HandleEjectPasenger(WorldPacket &data)
{
    if (_player->GetVehicleKit())
    {
        uint64 guid;
        data >> guid;
        if (Player *plr = ObjectAccessor::FindPlayer(guid))
            plr->ExitVehicle();
        else if (Unit *unit = ObjectAccessor::GetUnit(*_player, guid)) // creatures can be ejected too from player mounts
        {
            unit->ExitVehicle();
            unit->ToCreature()->ForcedDespawn(1000);
        }
    }
}

void WorldSession::HandleRequestVehicleExit(WorldPacket &recv_data)
{
    sLog->outDebug("WORLD: Recvd CMSG_REQUEST_VEHICLE_EXIT");
    recv_data.hexlike();
    GetPlayer()->ExitVehicle();
}

void WorldSession::HandleMountSpecialAnimOpcode(WorldPacket& /*recv_data*/)
{
    WorldPacket data(SMSG_MOUNTSPECIAL_ANIM, 8);
    data << uint64(GetPlayer()->GetGUID());

    GetPlayer()->SendMessageToSet(&data, false);
}

void WorldSession::HandleMoveKnockBackAck(WorldPacket & recv_data)
{
    sLog->outDebug("CMSG_MOVE_KNOCK_BACK_ACK");

    uint64 guid;
    recv_data.readPackGUID(guid);

    if (_player->m_mover->GetGUID() != guid)
        return;

    recv_data.read_skip<uint32>();                          // unk

    MovementInfo movementInfo;
    ReadMovementInfo(recv_data, &movementInfo);
    _player->m_movementInfo = movementInfo;

    WorldPacket data(SMSG_MOVE_KNOCK_BACK, 66);
    data.appendPackGUID(guid);
    _player->BuildMovementPacket(&data);

    // knockback specific info
    data << movementInfo.j_sinAngle;
    data << movementInfo.j_cosAngle;
    data << movementInfo.j_xyspeed;
    data << movementInfo.j_zspeed;

    _player->SendMessageToSet(&data, false);
}

void WorldSession::HandleMoveHoverAck(WorldPacket& recv_data)
{
    sLog->outDebug("CMSG_MOVE_HOVER_ACK");

    uint64 guid;                                            // guid - unused
    recv_data.readPackGUID(guid);

    recv_data.read_skip<uint32>();                          // unk

    MovementInfo movementInfo;
    ReadMovementInfo(recv_data, &movementInfo);

    recv_data.read_skip<uint32>();                          // unk2
}

void WorldSession::HandleMoveWaterWalkAck(WorldPacket& recv_data)
{
    sLog->outDebug("CMSG_MOVE_WATER_WALK_ACK");

    uint64 guid;                                            // guid - unused
    recv_data.readPackGUID(guid);

    recv_data.read_skip<uint32>();                          // unk

    MovementInfo movementInfo;
    ReadMovementInfo(recv_data, &movementInfo);

    recv_data.read_skip<uint32>();                          // unk2
}

void WorldSession::HandleSummonResponseOpcode(WorldPacket& recv_data)
{
    if (!_player->IsAlive() || _player->IsInCombat())
        return;

    uint64 summoner_guid;
    bool agree;
    recv_data >> summoner_guid;
    recv_data >> agree;

    _player->SummonIfPossible(agree);
}

