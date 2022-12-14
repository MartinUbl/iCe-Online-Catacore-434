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
#include "SharedDefines.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Log.h"
#include "World.h"
#include "Object.h"
#include "Creature.h"
#include "Player.h"
#include "Vehicle.h"
#include "ObjectMgr.h"
#include "UpdateData.h"
#include "UpdateMask.h"
#include "Util.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include "Transport.h"
#include "DynamicTransport.h"
#include "TargetedMovementGenerator.h"
#include "WaypointMovementGenerator.h"
#include "VMapFactory.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "SpellAuraEffects.h"
#include "BattlefieldMgr.h"
#include "UpdateFieldFlags.h"
#include "TemporarySummon.h"
#include "Totem.h"
#include "OutdoorPvPMgr.h"
#include "MovementPacketBuilder.h"
#include "Group.h"
#include "G3D/g3dmath.h"
#include "DynamicTree.h"

uint32 GuidHigh2TypeId(uint32 guid_hi)
{
    switch(guid_hi)
    {
        case HIGHGUID_ITEM:         return TYPEID_ITEM;
        //case HIGHGUID_CONTAINER:    return TYPEID_CONTAINER; HIGHGUID_CONTAINER == HIGHGUID_ITEM currently
        case HIGHGUID_UNIT:         return TYPEID_UNIT;
        case HIGHGUID_PET:          return TYPEID_UNIT;
        case HIGHGUID_PLAYER:       return TYPEID_PLAYER;
        case HIGHGUID_GAMEOBJECT:   return TYPEID_GAMEOBJECT;
        case HIGHGUID_DYNAMICOBJECT:return TYPEID_DYNAMICOBJECT;
        case HIGHGUID_CORPSE:       return TYPEID_CORPSE;
        case HIGHGUID_AREATRIGGER:  return TYPEID_AREATRIGGER;
        case HIGHGUID_MO_TRANSPORT: return TYPEID_GAMEOBJECT;
        case HIGHGUID_VEHICLE:      return TYPEID_UNIT;
    }
    return NUM_CLIENT_OBJECT_TYPES;                         // unknown
}

Object::Object() : m_PackGUID(sizeof(uint64)+1)
{
    m_objectTypeId      = TYPEID_OBJECT;
    m_objectType        = TYPEMASK_OBJECT;

    m_uint32Values      = NULL;
    m_valuesCount       = 0;
    _fieldNotifyFlags   = UF_FLAG_DYNAMIC;

    m_inWorld           = false;
    m_objectUpdated     = false;

    m_PackGUID.appendPackGUID(0);
}

WorldObject::~WorldObject()
{
    // this may happen because there are many !create/delete
    if (m_isWorldObject && m_currMap)
    {
        if (GetTypeId() == TYPEID_CORPSE)
        {
            sLog->outCrash("Object::~Object Corpse guid=" UI64FMTD ", type=%d, entry=%u deleted but still in map!!", GetGUID(), ((Corpse*)this)->GetType(), GetEntry());
            ASSERT(false);
        }
        ResetMap();
    }
}

Object::~Object()
{
    if (IsInWorld())
    {
        sLog->outCrash("Object::~Object - guid=" UI64FMTD ", typeid=%d, entry=%u deleted but still in world!!", GetGUID(), GetTypeId(), GetEntry());
        if (isType(TYPEMASK_ITEM))
            sLog->outCrash("Item slot %u", ((Item*)this)->GetSlot());
        ASSERT(false);
        RemoveFromWorld();
    }

    if (m_objectUpdated)
    {
        sLog->outCrash("Object::~Object - guid=" UI64FMTD ", typeid=%d, entry=%u deleted but still in update list!!", GetGUID(), GetTypeId(), GetEntry());
        ASSERT(false);
        sObjectAccessor->RemoveUpdateObject(this);
    }

    delete [] m_uint32Values;

}

void Object::_InitValues()
{
    m_uint32Values = new uint32[m_valuesCount];
    memset(m_uint32Values, 0, m_valuesCount*sizeof(uint32));

    _changesMask.SetCount(m_valuesCount);

    m_objectUpdated = false;
}

void Object::_Create(uint32 guidlow, uint32 entry, HighGuid guidhigh)
{
    if (!m_uint32Values) _InitValues();

    uint64 guid = MAKE_NEW_GUID(guidlow, entry, guidhigh);
    SetUInt64Value(OBJECT_FIELD_GUID, guid);
    SetUInt16Value(OBJECT_FIELD_TYPE, 0, m_objectType);
    m_PackGUID.clear();
    m_PackGUID.appendPackGUID(GetGUID());
}

void Object::BuildMovementUpdateBlock(UpdateData * data, uint32 flags) const
{
    /*ByteBuffer buf(500);

    buf << uint8(UPDATETYPE_MOVEMENT);
    buf.append(GetPackGUID());

    _BuildMovementUpdate(&buf, flags);

    data->AddUpdateBlock(buf);*/
}

void Object::BuildCreateUpdateBlockForPlayer(UpdateData* data, Player* target) const
{
    if (!target)
        return;

    uint8  updateType = UPDATETYPE_CREATE_OBJECT;
    uint16 flags      = m_updateFlag;

    uint32 valCount = m_valuesCount;

    /** lower flag1 **/
    if (target == this)                                      // building packet for yourself
        flags |= UPDATEFLAG_SELF;
    else if (GetTypeId() == TYPEID_PLAYER)
        valCount = PLAYER_END_NOT_SELF;

    switch (GetGUIDHigh())
    {
        case HIGHGUID_PLAYER:
        case HIGHGUID_PET:
        case HIGHGUID_CORPSE:
        case HIGHGUID_DYNAMICOBJECT:
        case HIGHGUID_AREATRIGGER:
            updateType = UPDATETYPE_CREATE_OBJECT2;
            break;
        case HIGHGUID_UNIT:
        {
            TempSummon* ts = ((Unit*)ToUnit())->ToTempSummon();
            if (ts && IS_PLAYER_GUID(ts->GetSummonerGUID()))
                updateType = UPDATETYPE_CREATE_OBJECT2;
            break;
        }
        case HIGHGUID_GAMEOBJECT:
            if (IS_PLAYER_GUID(ToGameObject()->GetOwnerGUID()))
                updateType = UPDATETYPE_CREATE_OBJECT2;
            break;
    }

    if (flags & UPDATEFLAG_STATIONARY_POSITION)
    {
        // UPDATETYPE_CREATE_OBJECT2 for some gameobject types...
        if (isType(TYPEMASK_GAMEOBJECT))
        {
            switch (((GameObject*)this)->GetGoType())
            {
                case GAMEOBJECT_TYPE_TRAP:
                case GAMEOBJECT_TYPE_DUEL_ARBITER:
                case GAMEOBJECT_TYPE_FLAGSTAND:
                case GAMEOBJECT_TYPE_FLAGDROP:
                    updateType = UPDATETYPE_CREATE_OBJECT2;
                    break;
                default:
                    break;
            }
        }
    }

    if (Unit const* unit = ToUnit())
    {
        if (ToUnit()->GetVictim())
            flags |= UPDATEFLAG_HAS_TARGET;

        if (unit->GetAIAnimKitId() || unit->GetMovementAnimKitId() || unit->GetMeleeAnimKitId())
            flags |= UPDATEFLAG_ANIMKITS;
    }

    ByteBuffer buf(500);
    buf << uint8(updateType);
    buf.append(GetPackGUID());
    buf << uint8(m_objectTypeId);

    _BuildMovementUpdate(&buf, flags);

    UpdateMask updateMask;
    updateMask.SetCount(valCount);
    _SetCreateBits(&updateMask, target);
    _BuildValuesUpdate(updateType, &buf, &updateMask, target);
    data->AddUpdateBlock(buf);
}

void Object::SendUpdateToPlayer(Player* player)
{
    // send create update to player
    UpdateData upd(player->GetMapId());
    WorldPacket packet;

    BuildCreateUpdateBlockForPlayer(&upd, player);
    upd.BuildPacket(&packet);
    player->GetSession()->SendPacket(&packet);
}

void Object::BuildValuesUpdateBlockForPlayer(UpdateData* data, Player* target) const
{
    ByteBuffer buf(500);

    buf << (uint8) UPDATETYPE_VALUES;
    buf.append(GetPackGUID());

    UpdateMask updateMask;
    uint32 valCount = m_valuesCount;
    if (GetTypeId() == TYPEID_PLAYER && target != this)
        valCount = PLAYER_END_NOT_SELF;

    updateMask.SetCount(valCount);

    _SetUpdateBits(&updateMask, target);
    _BuildValuesUpdate(UPDATETYPE_VALUES, &buf, &updateMask, target);

    data->AddUpdateBlock(buf);
}

void Object::BuildOutOfRangeUpdateBlock(UpdateData * data) const
{
    data->AddOutOfRangeGUID(GetGUID());
}

void Object::DestroyForPlayer(Player *target, bool anim) const
{
    ASSERT(target);

    if (isType(TYPEMASK_UNIT) || isType(TYPEMASK_PLAYER))
    {
        if (Battleground *bg = target->GetBattleground())
        {
            if (bg->isArena())
            {
                WorldPacket data(SMSG_ARENA_UNIT_DESTROYED, 8);
                data << uint64(GetGUID());
                target->GetSession()->SendPacket(&data);
            }
        }
    }

    WorldPacket data(SMSG_DESTROY_OBJECT, 8 + 1);
    data << uint64(GetGUID());
    data << uint8(anim ? 1 : 0);                            // WotLK (bool), may be despawn animation
    target->GetSession()->SendPacket(&data);
}

void Object::HideForPlayer(Player *target) const
{
    UpdateData data(target->GetMapId());
    data.AddOutOfRangeGUID(GetGUID());
    WorldPacket packet;
    data.BuildPacket(&packet);
    target->GetSession()->SendPacket(&packet);
}

void Object::_BuildMovementUpdate(ByteBuffer * data, uint16 flags) const
{
    uint32 stopFrameCount = 0;
    if (GameObject const* go = ToGameObject())
        if (go->GetGoType() == GAMEOBJECT_TYPE_TRANSPORT)
            stopFrameCount = go->GetGOValue()->Transport.StopFrames->size();

    uint32 transportTime2 = (ToUnit() ? ToUnit()->m_movementInfo.t_time2 : 0);
    bool hasTransportTime2 = (transportTime2 != 0);

    uint32 transportVehicleId = (ToUnit() ? ToUnit()->m_movementInfo.t_vehicleId : 0);
    bool hasTransportVehicleId = (transportVehicleId != 0);

    // Bit content
    data->WriteBit(0);
    data->WriteBit(0);
    data->WriteBit(flags & UPDATEFLAG_ROTATION);
    data->WriteBit(flags & UPDATEFLAG_ANIMKITS);
    data->WriteBit(flags & UPDATEFLAG_HAS_TARGET);
    data->WriteBit(flags & UPDATEFLAG_SELF);
    data->WriteBit(flags & UPDATEFLAG_VEHICLE);
    data->WriteBit(flags & UPDATEFLAG_LIVING);
    data->WriteBits(stopFrameCount, 24);
    data->WriteBit(0);
    data->WriteBit(flags & UPDATEFLAG_GO_TRANSPORT_POSITION);
    data->WriteBit(flags & UPDATEFLAG_STATIONARY_POSITION);
    data->WriteBit(flags & UPDATEFLAG_UNK5);
    data->WriteBit(0);
    data->WriteBit(flags & UPDATEFLAG_TRANSPORT);

    if (flags & UPDATEFLAG_LIVING)
    {
        Unit* self = ((Unit*)this);
        ObjectGuid guid = GetGUID();
        uint32 movementFlags = self->m_movementInfo.GetMovementFlags() & ~MOVEMENTFLAG_FALLING;
        uint16 movementFlagsExtra = self->m_movementInfo.GetExtraMovementFlags() & ~MOVEMENTFLAG2_INTERPOLATED_PITCHING;
        if (GetTypeId() == TYPEID_UNIT)
            movementFlags &= MOVEMENTFLAG_MASK_CREATURE_ALLOWED;

        data->WriteBit(!movementFlags);
        data->WriteBit(G3D::fuzzyEq(self->GetOrientation(), 0.0f));             // Has Orientation
        data->WriteBit(guid[7]);
        data->WriteBit(guid[3]);
        data->WriteBit(guid[2]);
        if (movementFlags)
            data->WriteBits(movementFlags, 30);

        data->WriteBit(0);
        data->WriteBit(!((movementFlags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) ||
            (movementFlagsExtra & MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING)));       // Has pitch
        data->WriteBit(self->IsSplineEnabled());                                // Has spline data
        data->WriteBit(movementFlagsExtra & MOVEMENTFLAG2_INTERPOLATED_TURNING);// Has fall data
        data->WriteBit(!(movementFlags & MOVEMENTFLAG_SPLINE_ELEVATION));       // Has spline elevation
        data->WriteBit(guid[5]);
        data->WriteBit(self->m_movementInfo.t_guid);                            // Has transport data
        data->WriteBit(0);                                                      // Is missing time
        if (self->m_movementInfo.t_guid)
        {
            ObjectGuid transGuid = self->m_movementInfo.t_guid;

            data->WriteBit(transGuid[1]);
            data->WriteBit(hasTransportTime2);                                                  // Has transport time 2
            data->WriteBit(transGuid[4]);
            data->WriteBit(transGuid[0]);
            data->WriteBit(transGuid[6]);
            data->WriteBit(hasTransportVehicleId);                                              // Has transport time 3
            data->WriteBit(transGuid[7]);
            data->WriteBit(transGuid[5]);
            data->WriteBit(transGuid[3]);
            data->WriteBit(transGuid[2]);
        }

        data->WriteBit(guid[4]);
        if (self->IsSplineEnabled())
            Movement::PacketBuilder::WriteCreateBits(*self->movespline, *data);

        data->WriteBit(guid[6]);
        if (movementFlagsExtra & MOVEMENTFLAG2_INTERPOLATED_TURNING)
            data->WriteBit(movementFlags & MOVEMENTFLAG_FALLING);

        data->WriteBit(guid[0]);
        data->WriteBit(guid[1]);
        data->WriteBit(0);
        data->WriteBit(!movementFlagsExtra);
        if (movementFlagsExtra)
            data->WriteBits(movementFlagsExtra, 12);
    }

    if (flags & UPDATEFLAG_GO_TRANSPORT_POSITION)
    {
        WorldObject const* self = static_cast<WorldObject const*>(this);
        ObjectGuid transGuid = self->m_movementInfo.t_guid;
        data->WriteBit(transGuid[5]);
        data->WriteBit(hasTransportVehicleId);                                                      // Has GO transport time 3
        data->WriteBit(transGuid[0]);
        data->WriteBit(transGuid[3]);
        data->WriteBit(transGuid[6]);
        data->WriteBit(transGuid[1]);
        data->WriteBit(transGuid[4]);
        data->WriteBit(transGuid[2]);
        data->WriteBit(hasTransportTime2);                                                      // Has GO transport time 2
        data->WriteBit(transGuid[7]);
    }

    if (flags & UPDATEFLAG_HAS_TARGET)
    {
        ObjectGuid victimGuid = ToUnit()->GetVictim()->GetGUID();   // checked in BuildCreateUpdateBlockForPlayer
        data->WriteBit(victimGuid[2]);
        data->WriteBit(victimGuid[7]);
        data->WriteBit(victimGuid[0]);
        data->WriteBit(victimGuid[4]);
        data->WriteBit(victimGuid[5]);
        data->WriteBit(victimGuid[6]);
        data->WriteBit(victimGuid[1]);
        data->WriteBit(victimGuid[3]);
    }

    if (flags & UPDATEFLAG_ANIMKITS)
    {
        Unit const* unit = ToUnit();
        data->WriteBit(unit->GetAIAnimKitId() == 0);
        data->WriteBit(unit->GetMovementAnimKitId() == 0);
        data->WriteBit(unit->GetMeleeAnimKitId() == 0);
    }

    data->FlushBits();

    // Transport object stop frame data
    if (stopFrameCount > 0)
    {
        GameObject const* go = ToGameObject();

        if (go && go->GetGOValue()->Transport.StopFrames)
        {
            for (uint32 i = 0; i < stopFrameCount; ++i)
                *data << uint32(go->GetGOValue()->Transport.StopFrames->at(i));
        }
        else
        {
            for (uint32 i = 0; i < stopFrameCount; ++i)
                *data << uint32(0);
        }
    }

    if (flags & UPDATEFLAG_LIVING)
    {
        Unit* self = ((Unit*)this);
        ObjectGuid guid = GetGUID();
        uint32 movementFlags = self->m_movementInfo.GetMovementFlags() & ~MOVEMENTFLAG_FALLING;
        uint16 movementFlagsExtra = self->m_movementInfo.GetExtraMovementFlags() & ~MOVEMENTFLAG2_INTERPOLATED_PITCHING;
        if (GetTypeId() == TYPEID_UNIT)
            movementFlags &= MOVEMENTFLAG_MASK_CREATURE_ALLOWED;

        data->WriteByteSeq(guid[4]);
        *data << self->GetSpeed(MOVE_RUN_BACK);
        if (movementFlagsExtra & MOVEMENTFLAG2_INTERPOLATED_TURNING)
        {
            if (movementFlags & MOVEMENTFLAG_FALLING)
            {
                *data << float(self->m_movementInfo.j_xyspeed);
                *data << float(self->m_movementInfo.j_sinAngle);
                *data << float(self->m_movementInfo.j_cosAngle);
            }

            *data << uint32(self->m_movementInfo.fallTime);
            *data << float(self->m_movementInfo.j_zspeed);
        }

        *data << self->GetSpeed(MOVE_SWIM_BACK);
        if (movementFlags & MOVEMENTFLAG_SPLINE_ELEVATION)
            *data << float(self->m_movementInfo.splineElevation);

        if (self->IsSplineEnabled())
            Movement::PacketBuilder::WriteCreateData(*self->movespline, *data);

        *data << float(self->GetPositionZMinusOffset());
        data->WriteByteSeq(guid[5]);
        if (self->m_movementInfo.t_guid)
        {
            ObjectGuid transGuid = self->m_movementInfo.t_guid;

            data->WriteByteSeq(transGuid[5]);
            data->WriteByteSeq(transGuid[7]);
            *data << uint32(self->GetTransTime());
            *data << float(self->GetTransOffsetO());
            if (hasTransportTime2)
                *data << uint32(transportTime2);

            *data << float(self->GetTransOffsetY());
            *data << float(self->GetTransOffsetX());
            data->WriteByteSeq(transGuid[3]);
            *data << float(self->GetTransOffsetZ());
            data->WriteByteSeq(transGuid[0]);
            if (hasTransportVehicleId)
                *data << uint32(transportVehicleId);

            *data << int8(self->GetTransSeat());
            data->WriteByteSeq(transGuid[1]);
            data->WriteByteSeq(transGuid[6]);
            data->WriteByteSeq(transGuid[2]);
            data->WriteByteSeq(transGuid[4]);
        }

        *data << float(self->GetPositionX());
        *data << self->GetSpeed(MOVE_PITCH_RATE);
        data->WriteByteSeq(guid[3]);
        data->WriteByteSeq(guid[0]);
        *data << self->GetSpeed(MOVE_SWIM);
        *data << float(self->GetPositionY());
        data->WriteByteSeq(guid[7]);
        data->WriteByteSeq(guid[1]);
        data->WriteByteSeq(guid[2]);
        *data << self->GetSpeed(MOVE_WALK);

        //if (true)   // Has time, controlled by bit just after HasTransport
        *data << uint32(getMSTime());

        *data << self->GetSpeed(MOVE_FLIGHT_BACK);
        data->WriteByteSeq(guid[6]);
        *data << self->GetSpeed(MOVE_TURN_RATE);
        if (!G3D::fuzzyEq(self->GetOrientation(), 0.0f))
            *data << float(self->GetOrientation());

        *data << self->GetSpeed(MOVE_RUN);
        if ((movementFlags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) ||
            (movementFlagsExtra & MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING))
            *data << float(self->m_movementInfo.pitch);

        *data << self->GetSpeed(MOVE_FLIGHT);
    }

    if (flags & UPDATEFLAG_VEHICLE)
    {
        Unit const* self = ToUnit();
        *data << float(self->GetOrientation());
        *data << uint32(self->GetVehicleKit()->GetVehicleInfo()->m_ID);
    }

    if (flags & UPDATEFLAG_GO_TRANSPORT_POSITION)
    {
        WorldObject const* self = static_cast<WorldObject const*>(this);
        ObjectGuid transGuid = self->m_movementInfo.t_guid;

        data->WriteByteSeq(transGuid[0]);
        data->WriteByteSeq(transGuid[5]);
        if (hasTransportVehicleId)
            *data << uint32(transportVehicleId);

        data->WriteByteSeq(transGuid[3]);
        *data << float(self->GetTransOffsetX());
        data->WriteByteSeq(transGuid[4]);
        data->WriteByteSeq(transGuid[6]);
        data->WriteByteSeq(transGuid[1]);
        *data << uint32(self->GetTransTime());
        *data << float(self->GetTransOffsetY());
        data->WriteByteSeq(transGuid[2]);
        data->WriteByteSeq(transGuid[7]);
        *data << float(self->GetTransOffsetZ());
        *data << int8(self->GetTransSeat());
        *data << float(self->GetTransOffsetO());
        if (hasTransportTime2)
            *data << uint32(transportTime2);
    }

    if (flags & UPDATEFLAG_ROTATION)
        *data << uint64(ToGameObject()->GetRotation());

    if (flags & UPDATEFLAG_UNK5)
    {
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << uint8(0);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
    }

    if (flags & UPDATEFLAG_STATIONARY_POSITION)
    {
        WorldObject const* self = static_cast<WorldObject const*>(this);
        *data << float(self->GetStationaryO());
        *data << float(self->GetStationaryX());
        *data << float(self->GetStationaryY());
        if (const Unit* unit = this->ToUnit())
            *data << float(unit->GetPositionZMinusOffset());
        else
            *data << float(self->GetStationaryZ());
    }

    if (flags & UPDATEFLAG_HAS_TARGET)
    {
        ObjectGuid victimGuid = ToUnit()->GetVictim()->GetGUID();   // checked in BuildCreateUpdateBlockForPlayer
        data->WriteByteSeq(victimGuid[4]);
        data->WriteByteSeq(victimGuid[0]);
        data->WriteByteSeq(victimGuid[3]);
        data->WriteByteSeq(victimGuid[5]);
        data->WriteByteSeq(victimGuid[7]);
        data->WriteByteSeq(victimGuid[6]);
        data->WriteByteSeq(victimGuid[2]);
        data->WriteByteSeq(victimGuid[1]);
    }

    if (flags & UPDATEFLAG_ANIMKITS)
    {
        Unit const* unit = ToUnit();
        if (unit->GetAIAnimKitId())
            *data << uint16(unit->GetAIAnimKitId());
        if (unit->GetMovementAnimKitId())
            *data << uint16(unit->GetMovementAnimKitId());
        if (unit->GetMeleeAnimKitId())
            *data << uint16(unit->GetMeleeAnimKitId());
    }

    if (flags & UPDATEFLAG_TRANSPORT)
    {
        GameObject const* go = ToGameObject();
        /**
        Currently grid objects are not updated if there are no nearby players,
        this causes clients to receive different PathProgress
        resulting in players seeing the object in a different position
        - this may therefore lead to desynchronization
        */
        if (go && go->IsTransport())
            *data << uint32(go->GetGOValue()->Transport.PathProgress);
        else
            *data << uint32(getMSTime());
    }
}

void Object::_BuildValuesUpdate(uint8 updatetype, ByteBuffer* data, UpdateMask* updateMask, Player* target) const
{
    if (!target)
        return;

    bool IsActivateToQuest = false;
    if (updatetype == UPDATETYPE_CREATE_OBJECT || updatetype == UPDATETYPE_CREATE_OBJECT2)
    {
        if (isType(TYPEMASK_GAMEOBJECT) && !((GameObject*)this)->IsStaticTransport())
        {
            if (((GameObject*)this)->ActivateToQuest(target) || target->IsGameMaster())
                IsActivateToQuest = true;

            if (((GameObject*)this)->GetGoArtKit())
                updateMask->SetBit(GAMEOBJECT_BYTES_1);
        }
        else if (isType(TYPEMASK_UNIT))
        {
            if (((Unit*)this)->HasFlag(UNIT_FIELD_AURASTATE, PER_CASTER_AURA_STATE_MASK))
                updateMask->SetBit(UNIT_FIELD_AURASTATE);
        }
    }
    else                                                    // case UPDATETYPE_VALUES
    {
        if (isType(TYPEMASK_GAMEOBJECT))
        {
            if (!((GameObject*)this)->IsTransport())
            {
                if (((GameObject*)this)->ActivateToQuest(target) || target->IsGameMaster())
                    IsActivateToQuest = true;

                updateMask->SetBit(GAMEOBJECT_BYTES_1);

                if (ToGameObject()->GetGoType() == GAMEOBJECT_TYPE_CHEST && ToGameObject()->GetGOInfo()->chest.groupLootRules/* &&
                    ToGameObject()->HasLootRecipient()*/)
                    updateMask->SetBit(GAMEOBJECT_FLAGS);
            }
        }
        else if (isType(TYPEMASK_UNIT))
        {
            if (((Unit*)this)->HasFlag(UNIT_FIELD_AURASTATE, PER_CASTER_AURA_STATE_MASK))
                updateMask->SetBit(UNIT_FIELD_AURASTATE);
        }
    }

    if (isType(TYPEMASK_GAMEOBJECT))
    {
        if (((GameObject*)this)->IsTransport())
        {
            // We have to always send flags in update packet for all transports
            updateMask->SetBit(GAMEOBJECT_FLAGS);
            updateMask->SetBit(GAMEOBJECT_LEVEL);
            updateMask->SetBit(GAMEOBJECT_DYNAMIC);
        }
    }

    uint32 valCount = m_valuesCount;
    if (GetTypeId() == TYPEID_PLAYER && target != this)
        valCount = PLAYER_END_NOT_SELF;

    WPAssert(updateMask && updateMask->GetCount() == valCount);

    *data << (uint8)updateMask->GetBlockCount();
    updateMask->AppendToPacket(data);

    // 2 specialized loops for speed optimization in non-unit case
    if (isType(TYPEMASK_UNIT))                               // unit (creature/player) case
    {
        for (uint16 index = 0; index < valCount; ++index)
        {
            if (updateMask->GetBit(index))
            {
                if (index == UNIT_NPC_FLAGS)
                {
                    // remove custom flag before sending
                    uint32 appendValue = m_uint32Values[index];

                    if (GetTypeId() == TYPEID_UNIT)
                    {
                        if (!target->CanSeeSpellClickOn(this->ToCreature()))
                            appendValue &= ~UNIT_NPC_FLAG_SPELLCLICK;

                        if (appendValue & UNIT_NPC_FLAG_TRAINER)
                        {
                            if (!this->ToCreature()->isCanTrainingOf(target, false))
                                appendValue &= ~(UNIT_NPC_FLAG_TRAINER | UNIT_NPC_FLAG_TRAINER_CLASS | UNIT_NPC_FLAG_TRAINER_PROFESSION);
                        }
                    }

                    *data << uint32(appendValue);
                }
                else if (index == UNIT_FIELD_AURASTATE)
                {
                    // Check per caster aura states to not enable using a pell in client if specified aura is not by target
                    *data << ((Unit*)this)->BuildAuraStateUpdateForTarget(target);
                }
                // FIXME: Some values at server stored in float format but must be sent to client in uint32 format
                else if (index >= UNIT_FIELD_BASEATTACKTIME && index <= UNIT_FIELD_RANGEDATTACKTIME)
                {
                    // convert from float to uint32 and send
                    *data << uint32(m_floatValues[index] < 0 ? 0 : m_floatValues[index]);
                }
                // there are some float values which may be negative or can't get negative due to other checks
                else if ((index >= UNIT_FIELD_NEGSTAT0   && index <= UNIT_FIELD_NEGSTAT4) ||
                    (index >= UNIT_FIELD_RESISTANCEBUFFMODSPOSITIVE  && index <= (UNIT_FIELD_RESISTANCEBUFFMODSPOSITIVE + 6)) ||
                    (index >= UNIT_FIELD_RESISTANCEBUFFMODSNEGATIVE  && index <= (UNIT_FIELD_RESISTANCEBUFFMODSNEGATIVE + 6)) ||
                    (index >= UNIT_FIELD_POSSTAT0   && index <= UNIT_FIELD_POSSTAT4))
                {
                    *data << uint32(m_floatValues[index]);
                }
                // Gamemasters should be always able to select units - remove not selectable flag
                else if (index == UNIT_FIELD_FLAGS)
                {
                    if (target->IsGameMaster())
                        *data << (m_uint32Values[index] & ~UNIT_FLAG_NOT_SELECTABLE);
                    else
                        *data << m_uint32Values[index];
                }
                // use modelid_a if not gm, _h if gm for CREATURE_FLAG_EXTRA_TRIGGER creatures
                else if (index == UNIT_FIELD_DISPLAYID)
                {
                    if (GetTypeId() == TYPEID_UNIT)
                    {
                        CreatureInfo const* cinfo = ToCreature()->GetCreatureInfo();

                        // this also applies for transform auras
                        if (SpellEntry const* transform = sSpellStore.LookupEntry(ToUnit()->getTransForm()))
                            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                                if (transform->EffectApplyAuraName[i] == SPELL_AURA_TRANSFORM)
                                    if (CreatureInfo const* transformInfo = sObjectMgr->GetCreatureTemplate(transform->EffectMiscValue[i]))
                                    {
                                        cinfo = transformInfo;
                                        break;
                                    }

                        uint32 modelId = m_uint32Values[index];

                        // several spells should send different visuals depending on faction
                        switch (modelId)
                        {
                            case 34997: // Ring of Frost
                                if (ToUnit()->IsFriendlyTo(target))
                                    modelId = 38203;
                                break;
                        }

                        if (cinfo->flags_extra & CREATURE_FLAG_EXTRA_TRIGGER)
                        {
                            if (target->IsGameMaster())
                            {
                                if (cinfo->Modelid1)
                                    modelId = cinfo->Modelid1;//Modelid1 is a visible model for gms
                                else
                                    modelId = 17519; // world invisible trigger's model
                            }
                            else
                            {
                                if (cinfo->Modelid2)
                                    modelId = cinfo->Modelid2;//Modelid2 is an invisible model for players
                                else
                                    modelId = 11686; // world invisible trigger's model
                            }
                        }

                        *data << modelId;
                    }
                    else
                        *data << m_uint32Values[index];
                }
                // hide lootable animation for unallowed players
                else if (index == UNIT_DYNAMIC_FLAGS)
                {
                    uint32 dynamicFlags = m_uint32Values[index];

                    if (Creature const* creature = ToCreature())
                    {
                        if (creature->hasLootRecipient())
                        {
                            if (creature->isTappedBy(target))
                            {
                                dynamicFlags |= (UNIT_DYNFLAG_TAPPED | UNIT_DYNFLAG_TAPPED_BY_PLAYER);
                            }
                            else
                            {
                                dynamicFlags |= UNIT_DYNFLAG_TAPPED;
                                dynamicFlags &= ~UNIT_DYNFLAG_TAPPED_BY_PLAYER;
                            }
                        }
                        else
                        {
                            dynamicFlags &= ~UNIT_DYNFLAG_TAPPED;
                            dynamicFlags &= ~UNIT_DYNFLAG_TAPPED_BY_PLAYER;
                        }

                        if (!target->isAllowedToLoot(creature))
                            dynamicFlags &= ~UNIT_DYNFLAG_LOOTABLE;
                    }

                    // unit UNIT_DYNFLAG_TRACK_UNIT should only be sent to caster of SPELL_AURA_MOD_STALKED auras
                    /*if (Unit const* unit = ToUnit())
                        if (dynamicFlags & UNIT_DYNFLAG_TRACK_UNIT)
                            if (!unit->HasAuraTypeWithCaster(SPELL_AURA_MOD_STALKED, target->GetGUID()))
                                dynamicFlags &= ~UNIT_DYNFLAG_TRACK_UNIT;*/
                    *data << dynamicFlags;
                }
                // FG: pretend that OTHER players in own group are friendly ("blue")
                else if (index == UNIT_FIELD_BYTES_2 || index == UNIT_FIELD_FACTIONTEMPLATE)
                {
                    Unit const* unit = ToUnit();
                    if (unit->IsControlledByPlayer() && target != this && sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GROUP) && unit->IsInRaidWith(target))
                    {
                        FactionTemplateEntry const* ft1 = unit->getFactionTemplateEntry();
                        FactionTemplateEntry const* ft2 = target->getFactionTemplateEntry();
                        if (ft1 && ft2 && !ft1->IsFriendlyTo(*ft2))
                        {
                            if (index == UNIT_FIELD_BYTES_2)
                            {
                                // Allow targetting opposite faction in party when enabled in config
                                *data << (m_uint32Values[index] & ((UNIT_BYTE2_FLAG_SANCTUARY /*| UNIT_BYTE2_FLAG_AURAS | UNIT_BYTE2_FLAG_UNK5*/) << 8)); // this flag is at uint8 offset 1 !!
                            }
                            else
                            {
                                // pretend that all other HOSTILE players have own faction, to allow follow, heal, rezz (trade wont work)
                                uint32 faction = target->getFaction();
                                *data << uint32(faction);
                            }
                        }
                        else
                            *data << m_uint32Values[index];
                    }
                    else
                        *data << m_uint32Values[index];
                }
                else
                {
                    // send in current format (float as float, uint32 as uint32)
                    *data << m_uint32Values[index];
                }
            }
        }
    }
    else if (isType(TYPEMASK_GAMEOBJECT))                    // gameobject case
    {
        GameObjectValue const* goValue = ToGameObject()->GetGOValue();

        for (uint16 index = 0; index < valCount; ++index)
        {
            if (updateMask->GetBit(index))
            {
                // send in current format (float as float, uint32 as uint32)
                if (index == GAMEOBJECT_DYNAMIC)
                {
                    uint16 dynFlags = 0;
                    uint16 pathProgress = uint16(-1);

                    switch (ToGameObject()->GetGoType())
                    {
                        case GAMEOBJECT_TYPE_CHEST:
                            if (IsActivateToQuest)
                            {
                                if (target->IsGameMaster())
                                    dynFlags |= GO_DYNFLAG_LO_ACTIVATE;
                                else
                                    dynFlags |= GO_DYNFLAG_LO_ACTIVATE | GO_DYNFLAG_LO_SPARKLE;
                            }
                            break;
                        case GAMEOBJECT_TYPE_GENERIC:
                            if (!target->IsGameMaster() && IsActivateToQuest)
                                dynFlags |= GO_DYNFLAG_LO_SPARKLE;
                            break;
                        case GAMEOBJECT_TYPE_GOOBER:
                            if (IsActivateToQuest)
                            {
                                if (target->IsGameMaster())
                                    dynFlags |= GO_DYNFLAG_LO_ACTIVATE;
                                else
                                    dynFlags |= GO_DYNFLAG_LO_ACTIVATE | GO_DYNFLAG_LO_SPARKLE;
                            }
                            break;
                        case GAMEOBJECT_TYPE_TRANSPORT:
                        {
                            if (goValue->Transport.StateChangeStartProgress > 0)
                            {
                                uint32 diff = getMSTimeDiff(goValue->Transport.StateChangeStartProgress, goValue->Transport.PathProgress);
                                if (diff < goValue->Transport.StateChangeTime)
                                    pathProgress = uint16(65535.0f * float(diff) / float(goValue->Transport.StateChangeTime));
                                else
                                    pathProgress = 0;
                            }
                            else
                                pathProgress = 0;
                            break;
                        }
                        case GAMEOBJECT_TYPE_MO_TRANSPORT:
                        {
                            uint32 lvl = GetUInt32Value(GAMEOBJECT_LEVEL);
                            // when MO_TRANSPORT object does not have period set, it is implicitly
                            // set to zero - to avoid division by zero, apply something that will
                            // nicely divide path progress itself
                            if (lvl == 0)
                                lvl = goValue->Transport.PathProgress != 0 ? goValue->Transport.PathProgress : 1;

                            float timer = float(goValue->Transport.PathProgress % lvl);
                            pathProgress = uint16((timer / float(lvl)) * 65535.0f);
                            break;
                        }
                        default:
                            // unknown and other
                            break;
                    }

                    *data << uint16(dynFlags);
                    *data << uint16(pathProgress);
                }
                else if (index == GAMEOBJECT_FLAGS)
                {
                    uint32 flags = m_uint32Values[index];

                    GameObject const* go = ToGameObject();

                    if (go->GetGoType() == GAMEOBJECT_TYPE_CHEST)
                        if (go->GetGOInfo()->chest.groupLootRules)
                            flags |= GO_FLAG_LOCKED | GO_FLAG_NOT_SELECTABLE;

                    if (go->IsTransport())
                    {
                        if (go->IsStaticTransport())
                            flags = GO_FLAG_NODESPAWN | GO_FLAG_TRANSPORT;
                        else
                            flags |= GO_FLAG_TRANSPORT;
                    }

                    *data << flags;
                }
                else if (index == GAMEOBJECT_LEVEL)
                {
                    if (ToGameObject()->IsDynamicTransport())
                        *data << uint32(goValue->Transport.StateChangeStartProgress + goValue->Transport.StateChangeTime);
                    else
                        *data << m_uint32Values[index];
                }
                else if (index == GAMEOBJECT_BYTES_1)
                {
                    uint32 bytes1 = m_uint32Values[index];
                    if (ToGameObject()->IsDynamicTransport() && ToGameObject()->GetGoState() == GO_STATE_TRANSPORT_ACTIVE)
                    {
                        bytes1 &= 0xFFFFFF00;
                        bytes1 |= GO_STATE_TRANSPORT_STOPPED + goValue->Transport.VisualState;
                    }

                    *data << bytes1;
                }
                else
                    *data << m_uint32Values[index];                // other cases
            }
        }
    }
    else if (isType(TYPEMASK_DYNAMICOBJECT))
    {
        DynamicObject const* dynob = ToDynObject();

        for (uint16 index = 0; index < valCount; ++index)
        {
            if (updateMask->GetBit(index))
            {
                if (index == DYNAMICOBJECT_BYTES)
                {
                    uint32 sendBytes = m_uint32Values[DYNAMICOBJECT_BYTES];

                    Unit* owner = dynob->GetCaster();
                    if (owner)
                        owner = owner->GetCharmerOrOwnerOrSelf();

                    // if caster and target teams does not match, we will send different visual
                    // for the player for several spells
                    if (owner && owner->IsHostileTo(target))
                    {
                        uint32 visual = sendBytes & 0xFFFFFFF; // cut 28bits
                        switch (visual)
                        {
                            // Flare
                            case 19814: visual = 20730; break;
                            // Desecration
                            case 8506: visual = 20722; break;
                            // Smoke Bomb
                            case 16163: visual = 20733; break;
                            // Consecration
                            case 20720: visual = 17387; break;
                            // Frost trap aura
                            case 3759:
                                if (owner->GetEntry() != 119556) // exception for Hagara Encounter
                                    visual = 20731;
                                break;
                            // Power Word: Barrier
                            case 13210: visual = 20732; break;
                            // Hand of Gul'Dan
                            case 18896: visual = 20737; break;
                            // Fungal Growth
                            case 19762: visual = 22670; break;
                        }

                        sendBytes = (sendBytes & 0xFF000000) | visual;
                    }

                    *data << sendBytes;
                }
                else
                {
                    // send in current format (float as float, uint32 as uint32)
                    *data << m_uint32Values[index];
                }
            }
        }
    }
    else                                                    // other objects case (no special index checks)
    {
        for (uint16 index = 0; index < valCount; ++index)
        {
            if (updateMask->GetBit(index))
            {
                // send in current format (float as float, uint32 as uint32)
                *data << m_uint32Values[index];
            }
        }
    }
}

void Object::ClearUpdateMask(bool remove)
{
    _changesMask.Clear();

    if (m_objectUpdated)
    {
        if (remove)
            sObjectAccessor->RemoveUpdateObject(this);
        m_objectUpdated = false;
    }
}

void Object::BuildFieldsUpdate(Player *pl, UpdateDataMapType &data_map) const
{
    UpdateDataMapType::iterator iter = data_map.find(pl);

    if (iter == data_map.end())
    {
        UpdateData udata(pl->GetMapId());
        std::pair<UpdateDataMapType::iterator, bool> p = data_map.insert(UpdateDataMapType::value_type(pl, udata));
        ASSERT(p.second);
        iter = p.first;
    }

    BuildValuesUpdateBlockForPlayer(&iter->second, iter->first);
}

bool Object::LoadValues(const char* data)
{
    if (!m_uint32Values) _InitValues();

    Tokens tokens(data, ' ');

    if (tokens.size() != m_valuesCount)
        return false;

    for (uint16 index = 0; index < m_valuesCount; ++index)
        m_uint32Values[index] = atol(tokens[index]);

    return true;
}

void Object::_LoadIntoDataField(char const* data, uint32 startOffset, uint32 count)
{
    if (!data)
        return;

    Tokens tokens(data, ' ', count);

    if (tokens.size() != count)
        return;

    for (uint32 index = 0; index < count; ++index)
    {
        m_uint32Values[startOffset + index] = atol(tokens[index]);
        _changesMask.SetBit(startOffset + index);
    }
}

uint32 Object::GetUpdateFieldData(Player const* target, uint32*& flags) const
{
    uint32 visibleFlag = UF_FLAG_PUBLIC;

    if (target == this)
        visibleFlag |= UF_FLAG_PRIVATE;

    switch (GetTypeId())
    {
        case TYPEID_ITEM:
        case TYPEID_CONTAINER:
            flags = ItemUpdateFieldFlags;
            if (((Item*)this)->GetOwnerGUID() == target->GetGUID())
                visibleFlag |= UF_FLAG_OWNER | UF_FLAG_ITEM_OWNER;
            break;
        case TYPEID_UNIT:
        case TYPEID_PLAYER:
        {
            Player* plr = ToUnit()->GetCharmerOrOwnerPlayerOrPlayerItself();
            flags = UnitUpdateFieldFlags;
            if (ToUnit()->GetOwnerGUID() == target->GetGUID())
                visibleFlag |= UF_FLAG_OWNER;

            if (ToUnit()->HasAuraTypeWithCaster(SPELL_AURA_EMPATHY, target->GetGUID()))
                visibleFlag |= UF_FLAG_SPECIAL_INFO;

            if (plr && plr->IsInSameGroupWith(target))
                visibleFlag |= UF_FLAG_PARTY_MEMBER;
            break;
        }
        case TYPEID_GAMEOBJECT:
            flags = GameObjectUpdateFieldFlags;
            if (ToGameObject()->GetOwnerGUID() == target->GetGUID())
                visibleFlag |= UF_FLAG_OWNER;
            break;
        case TYPEID_DYNAMICOBJECT:
            flags = DynamicObjectUpdateFieldFlags;
            if (((DynamicObject*)this)->GetCasterGUID() == target->GetGUID())
                visibleFlag |= UF_FLAG_OWNER;
            break;
        case TYPEID_CORPSE:
            flags = CorpseUpdateFieldFlags;
            if (ToCorpse()->GetOwnerGUID() == target->GetGUID())
                visibleFlag |= UF_FLAG_OWNER;
            break;
        case TYPEID_AREATRIGGER:
            flags = AreaTriggerUpdateFieldFlags;
            break;
        case TYPEID_OBJECT:
            // This should NEVER happen!
            break;
    }

    return visibleFlag;
}

void Object::_SetUpdateBits(UpdateMask* updateMask, Player* target) const
{
    uint32* flags = NULL;

    uint32 visibleFlag = GetUpdateFieldData(target, flags);

    uint32 valuesCount = m_valuesCount;

    if(GetTypeId() == TYPEID_PLAYER && target != this)
        valuesCount = PLAYER_END_NOT_SELF;

    for (uint16 index = 0; index < valuesCount; ++index)
        if (_fieldNotifyFlags & flags[index] || ((flags[index] & visibleFlag) & UF_FLAG_SPECIAL_INFO) || (_changesMask.GetBit(index) && (flags[index] & visibleFlag)))
            updateMask->SetBit(index);
}

void Object::_SetCreateBits(UpdateMask* updateMask, Player* target) const
{
    uint32* value = m_uint32Values;
    uint32* flags = NULL;

    uint32 valuesCount = m_valuesCount;
    if(GetTypeId() == TYPEID_PLAYER && target != this)
        valuesCount = PLAYER_END_NOT_SELF;

    uint32 visibleFlag = GetUpdateFieldData(target, flags);

    for (uint16 index = 0; index < valuesCount; ++index, ++value)
        if (_fieldNotifyFlags & flags[index] || ((flags[index] & visibleFlag) & UF_FLAG_SPECIAL_INFO) || (*value && (flags[index] & visibleFlag)))
            updateMask->SetBit(index);
}

void Object::SetInt32Value(uint16 index, int32 value)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));

    if (m_int32Values[index] != value)
    {
        m_int32Values[index] = value;
        _changesMask.SetBit(index);

        if (m_inWorld)
        {
            if (!m_objectUpdated)
            {
                sObjectAccessor->AddUpdateObject(this);
                m_objectUpdated = true;
            }
        }
    }
}

void Object::SetUInt32Value(uint16 index, uint32 value)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));

    if (m_uint32Values[index] != value)
    {
        m_uint32Values[index] = value;
        _changesMask.SetBit(index);

        if (m_inWorld)
        {
            if (!m_objectUpdated)
            {
                sObjectAccessor->AddUpdateObject(this);
                m_objectUpdated = true;
            }
        }
    }
}

void Object::UpdateUInt32Value(uint16 index, uint32 value)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));

    m_uint32Values[index] = value;
    _changesMask.SetBit(index);
}

void Object::SetUInt64Value(uint16 index, const uint64 &value)
{
    ASSERT(index + 1 < m_valuesCount || PrintIndexError(index, true));
    if (*((uint64*)&(m_uint32Values[index])) != value)
    {
        m_uint32Values[index] = *((uint32*)&value);
        m_uint32Values[index + 1] = *(((uint32*)&value) + 1);
        _changesMask.SetBit(index);
        _changesMask.SetBit(index + 1);

        if (m_inWorld)
        {
            if (!m_objectUpdated)
            {
                sObjectAccessor->AddUpdateObject(this);
                m_objectUpdated = true;
            }
        }
    }
}

bool Object::AddUInt64Value(uint16 index, const uint64 &value)
{
    ASSERT(index + 1 < m_valuesCount || PrintIndexError(index , true));
    if (value && !*((uint64*)&(m_uint32Values[index])))
    {
        m_uint32Values[index] = *((uint32*)&value);
        m_uint32Values[index + 1] = *(((uint32*)&value) + 1);
        _changesMask.SetBit(index);
        _changesMask.SetBit(index + 1);

        if (m_inWorld)
        {
            if (!m_objectUpdated)
            {
                sObjectAccessor->AddUpdateObject(this);
                m_objectUpdated = true;
            }
        }
        return true;
    }
    return false;
}

bool Object::RemoveUInt64Value(uint16 index, const uint64 &value)
{
    ASSERT(index + 1 < m_valuesCount || PrintIndexError(index , true));
    if (value && *((uint64*)&(m_uint32Values[index])) == value)
    {
        m_uint32Values[index] = 0;
        m_uint32Values[index + 1] = 0;
        _changesMask.SetBit(index);
        _changesMask.SetBit(index + 1);

        if (m_inWorld)
        {
            if (!m_objectUpdated)
            {
                sObjectAccessor->AddUpdateObject(this);
                m_objectUpdated = true;
            }
        }
        return true;
    }
    return false;
}

void Object::SetFloatValue(uint16 index, float value)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));

    if (GetTypeId() == TYPEID_UNIT && index == UNIT_FIELD_COMBATREACH)
        ToCreature()->SaveBackupCombatReach(value);

    if (m_floatValues[index] != value)
    {
        m_floatValues[index] = value;
        _changesMask.SetBit(index);

        if (m_inWorld)
        {
            if (!m_objectUpdated)
            {
                sObjectAccessor->AddUpdateObject(this);
                m_objectUpdated = true;
            }
        }
    }
}

void Object::SetByteValue(uint16 index, uint8 offset, uint8 value)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));

    if (offset > 4)
    {
        sLog->outError("Object::SetByteValue: wrong offset %u", offset);
        return;
    }

    if (uint8(m_uint32Values[index] >> (offset * 8)) != value)
    {
        m_uint32Values[index] &= ~uint32(uint32(0xFF) << (offset * 8));
        m_uint32Values[index] |= uint32(uint32(value) << (offset * 8));
        _changesMask.SetBit(index);

        if (m_inWorld)
        {
            if (!m_objectUpdated)
            {
                sObjectAccessor->AddUpdateObject(this);
                m_objectUpdated = true;
            }
        }
    }
}

void Object::SetUInt16Value(uint16 index, uint8 offset, uint16 value)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));

    if (offset > 2)
    {
        sLog->outError("Object::SetUInt16Value: wrong offset %u", offset);
        return;
    }

    if (uint16(m_uint32Values[index] >> (offset * 16)) != value)
    {
        m_uint32Values[index] &= ~uint32(uint32(0xFFFF) << (offset * 16));
        m_uint32Values[index] |= uint32(uint32(value) << (offset * 16));
        _changesMask.SetBit(index);

        if (m_inWorld)
        {
            if (!m_objectUpdated)
            {
                sObjectAccessor->AddUpdateObject(this);
                m_objectUpdated = true;
            }
        }
    }
}

void Object::SetStatFloatValue(uint16 index, float value)
{
    if (value < 0)
        value = 0.0f;

    SetFloatValue(index, value);
}

void Object::SetStatInt32Value(uint16 index, int32 value)
{
    if (value < 0)
        value = 0;

    SetUInt32Value(index, uint32(value));
}

void Object::ApplyModUInt32Value(uint16 index, int32 val, bool apply)
{
    int32 cur = GetUInt32Value(index);
    cur += (apply ? val : -val);
    if (cur < 0)
        cur = 0;
    SetUInt32Value(index, cur);
}

void Object::ApplyModInt32Value(uint16 index, int32 val, bool apply)
{
    int32 cur = GetInt32Value(index);
    cur += (apply ? val : -val);
    SetInt32Value(index, cur);
}

void Object::ApplyModSignedFloatValue(uint16 index, float  val, bool apply)
{
    float cur = GetFloatValue(index);
    cur += (apply ? val : -val);
    SetFloatValue(index, cur);
}

void Object::ApplyModPositiveFloatValue(uint16 index, float  val, bool apply)
{
    float cur = GetFloatValue(index);
    cur += (apply ? val : -val);
    if (cur < 0)
        cur = 0;
    SetFloatValue(index, cur);
}

void Object::SetFlag(uint16 index, uint32 newFlag)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));
    uint32 oldval = m_uint32Values[index];
    uint32 newval = oldval | newFlag;

    if (oldval != newval)
    {
        m_uint32Values[index] = newval;
        _changesMask.SetBit(index);

        if (m_inWorld)
        {
            if (!m_objectUpdated)
            {
                sObjectAccessor->AddUpdateObject(this);
                m_objectUpdated = true;
            }
        }
    }
}

void Object::RemoveFlag(uint16 index, uint32 oldFlag)
{
    ASSERT(m_uint32Values);
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));

    uint32 oldval = m_uint32Values[index];
    uint32 newval = oldval & ~oldFlag;

    if (oldval != newval)
    {
        m_uint32Values[index] = newval;
        _changesMask.SetBit(index);

        if (m_inWorld)
        {
            if (!m_objectUpdated)
            {
                sObjectAccessor->AddUpdateObject(this);
                m_objectUpdated = true;
            }
        }
    }
}

void Object::SetByteFlag(uint16 index, uint8 offset, uint8 newFlag)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));

    if (offset > 4)
    {
        sLog->outError("Object::SetByteFlag: wrong offset %u", offset);
        return;
    }

    if (!(uint8(m_uint32Values[index] >> (offset * 8)) & newFlag))
    {
        m_uint32Values[index] |= uint32(uint32(newFlag) << (offset * 8));
        _changesMask.SetBit(index);

        if (m_inWorld)
        {
            if (!m_objectUpdated)
            {
                sObjectAccessor->AddUpdateObject(this);
                m_objectUpdated = true;
            }
        }
    }
}

void Object::RemoveByteFlag(uint16 index, uint8 offset, uint8 oldFlag)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));

    if (offset > 4)
    {
        sLog->outError("Object::RemoveByteFlag: wrong offset %u", offset);
        return;
    }

    if (uint8(m_uint32Values[index] >> (offset * 8)) & oldFlag)
    {
        m_uint32Values[index] &= ~uint32(uint32(oldFlag) << (offset * 8));
        _changesMask.SetBit(index);

        if (m_inWorld)
        {
            if (!m_objectUpdated)
            {
                sObjectAccessor->AddUpdateObject(this);
                m_objectUpdated = true;
            }
        }
    }
}

bool Object::PrintIndexError(uint32 index, bool set) const
{
    sLog->outError("Attempt %s non-existed value field: %u (count: %u) for object typeid: %u type mask: %u",(set ? "set value to" : "get value from"),index,m_valuesCount,GetTypeId(),m_objectType);

    // ASSERT must fail after function call
    return false;
}

void WorldObject::SetTransport(TransportBase* t)
{
    m_transport = t;

    // gameobjects needs to have transport position sent separatelly
    if (ToGameObject())
    {
        if (t != NULL)
            m_updateFlag |= UPDATEFLAG_GO_TRANSPORT_POSITION;
        else
            m_updateFlag &= ~UPDATEFLAG_GO_TRANSPORT_POSITION;
    }
}

bool Position::HasInLine(const Unit * const target, float distance, float width) const
{
    if (!HasInArc(M_PI, target) || !target->IsWithinDist3d(m_positionX, m_positionY, m_positionZ, distance))
        return false;
    width += target->GetObjectSize();
    float angle = GetRelativeAngle(target);
    return fabs(sin(angle)) * GetExactDist2d(target->GetPositionX(), target->GetPositionY()) < width;
}

std::string Position::ToString() const
{
    std::stringstream sstr;
    sstr << "X: " << m_positionX << " Y: " << m_positionY << " Z: " << m_positionZ << " O: " << m_orientation;
    return sstr.str();
}

ByteBuffer &operator>>(ByteBuffer& buf, Position::PositionXYZOStreamer const & streamer)
{
    float x, y, z, o;
    buf >> x >> y >> z >> o;
    streamer.m_pos->Relocate(x, y, z, o);
    return buf;
}
ByteBuffer & operator<<(ByteBuffer& buf, Position::PositionXYZStreamer const & streamer)
{
    float x, y, z;
    streamer.m_pos->GetPosition(x, y, z);
    buf << x << y << z;
    return buf;
}

ByteBuffer &operator>>(ByteBuffer& buf, Position::PositionXYZStreamer const & streamer)
{
    float x, y, z;
    buf >> x >> y >> z;
    streamer.m_pos->Relocate(x, y, z);
    return buf;
}

ByteBuffer & operator<<(ByteBuffer& buf, Position::PositionXYZOStreamer const & streamer)
{
    float x, y, z, o;
    streamer.m_pos->GetPosition(x, y, z, o);
    buf << x << y << z << o;
    return buf;
}

void MovementInfo::OutDebug()
{
    sLog->outString("MOVEMENT INFO");
    sLog->outString("guid " UI64FMTD, guid);
    sLog->outString("flags %u", flags);
    sLog->outString("flags2 %u", flags2);
    sLog->outString("time %u current time " UI64FMTD "", flags2, uint64(::time(NULL)));
    sLog->outString("position: `%s`", pos.ToString().c_str());
    sLog->outString("MOVEMENT INFO");
    sLog->outString("guid " UI64FMTD, guid);
    sLog->outString("flags %u", flags);
    sLog->outString("flags2 %u", flags2);
    sLog->outString("time %u current time " UI64FMTD "", flags2, uint64(::time(NULL)));
    sLog->outString("position: `%s`", pos.ToString().c_str());
    if (t_guid)
    {
        sLog->outString("TRANSPORT:");
        sLog->outString("guid: " UI64FMTD, t_guid);
        sLog->outString("position: `%s`", t_pos.ToString().c_str());
        sLog->outString("seat: %i", t_seat);
        sLog->outString("time: %u", t_time);
        if (flags2 & MOVEMENTFLAG2_INTERPOLATED_MOVEMENT)
            sLog->outString("time2: %u", t_time2);
    }
    if ((flags & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) || (flags2 & MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING))
    {
        sLog->outString("pitch: %f", pitch);
    }
    sLog->outString("fallTime: %u", fallTime);
    if (flags & MOVEMENTFLAG_FALLING)
    {
        sLog->outString("j_zspeed: %f j_sinAngle: %f j_cosAngle: %f j_xyspeed: %f", j_zspeed, j_sinAngle, j_cosAngle, j_xyspeed);
    }
    if (flags & MOVEMENTFLAG_SPLINE_ELEVATION)
    {
        sLog->outString("splineElevation: %f", splineElevation);
    }
}

WorldObject::WorldObject(): WorldLocation(),
m_isWorldObject(false), m_name(""), m_isActive(false), m_zoneScript(NULL),
m_transport(NULL), m_currMap(NULL), m_InstanceId(0),
m_phaseMask(PHASEMASK_NORMAL), m_notifyflags(0), m_executed_notifies(0)
{}

void WorldObject::SetWorldObject(bool on)
{
    if (!IsInWorld())
        return;

    GetMap()->AddObjectToSwitchList(this, on);
}

void WorldObject::setActive(bool on)
{
    if (m_isActive == on)
        return;

    if (GetTypeId() == TYPEID_PLAYER)
        return;

    m_isActive = on;

    if (!IsInWorld())
        return;

    Map *map = FindMap();
    if (!map)
        return;

    if (on)
    {
        if (GetTypeId() == TYPEID_UNIT)
            map->AddToActive(this->ToCreature());
        else if (GetTypeId() == TYPEID_DYNAMICOBJECT)
            map->AddToActive((DynamicObject*)this);
    }
    else
    {
        if (GetTypeId() == TYPEID_UNIT)
            map->RemoveFromActive(this->ToCreature());
        else if (GetTypeId() == TYPEID_DYNAMICOBJECT)
            map->RemoveFromActive((DynamicObject*)this);
    }
}

void WorldObject::CleanupsBeforeDelete(bool /*finalCleanup*/)
{
}

void WorldObject::_Create(uint32 guidlow, HighGuid guidhigh, uint32 phaseMask)
{
    Object::_Create(guidlow, 0, guidhigh);
    m_phaseMask = phaseMask;
}

uint32 WorldObject::GetZoneId() const
{
    return GetBaseMap()->GetZoneId(m_positionX, m_positionY, m_positionZ);
}

uint32 WorldObject::GetAreaId() const
{
    return GetBaseMap()->GetAreaId(m_positionX, m_positionY, m_positionZ);
}

void WorldObject::GetZoneAndAreaId(uint32& zoneid, uint32& areaid) const
{
    GetBaseMap()->GetZoneAndAreaId(zoneid, areaid, m_positionX, m_positionY, m_positionZ);
}

InstanceScript* WorldObject::GetInstanceScript()
{
    Map *map = GetMap();
    return map->IsDungeon() ? ((InstanceMap*)map)->GetInstanceScript() : NULL;
}

float WorldObject::GetDistanceZ(const WorldObject* obj) const
{
    float dz = fabs(GetPositionZ() - obj->GetPositionZ());
    float sizefactor = GetObjectSize() + obj->GetObjectSize();
    float dist = dz - sizefactor;
    return (dist > 0 ? dist : 0);
}

bool WorldObject::_IsWithinDist(WorldObject const* obj, float dist2compare, bool is3D) const
{
    float dx, dy, dz, distsq;

    /* add size of the objects themselves, resulting in real max distance */
    dist2compare += GetObjectSize() + obj->GetObjectSize();

    if (m_transport && obj->GetTransport() && obj->GetTransport() == m_transport)
    {
        dx = m_movementInfo.t_pos.m_positionX - obj->m_movementInfo.t_pos.m_positionX;
        dy = m_movementInfo.t_pos.m_positionY - obj->m_movementInfo.t_pos.m_positionY;
        distsq = dx * dx + dy * dy;
        if (is3D)
        {
            dz = m_movementInfo.t_pos.m_positionZ - obj->m_movementInfo.t_pos.m_positionZ;
            distsq += dz * dz;
        }
    }
    else
    {
        dx = GetPositionX() - obj->GetPositionX();
        dy = GetPositionY() - obj->GetPositionY();
        distsq = dx*dx + dy*dy;
        if (is3D)
        {
            dz = GetPositionZ() - obj->GetPositionZ();
            distsq += dz*dz;
        }
    }

    return distsq < dist2compare * dist2compare;
}

bool WorldObject::IsWithinLOSInMap(const WorldObject* obj, bool reducedHeight) const
{
    if (!IsInMap(obj))
        return false;

    float ox,oy,oz;
    obj->GetPosition(ox,oy,oz);

    if (reducedHeight)
        return (IsWithinLOS(ox, oy, oz));
    else
        return (!IsInWorld() || IsWithinLOS_raw(ox, oy, oz, 1.5f));
}

bool WorldObject::IsWithinLOS(float ox, float oy, float oz) const
{
    if (IsInWorld())
        return IsWithinLOS_raw(ox, oy, oz, 2.0f);

    return true;
}

bool WorldObject::IsWithinLOS_raw(float ox, float oy, float oz, float heightCorrection) const
{
    return GetMap()->isInLineOfSight(GetPositionX(), GetPositionY(), GetPositionZ() + heightCorrection, ox, oy, oz + heightCorrection, GetPhaseMask());
}

bool WorldObject::GetDistanceOrder(WorldObject const* obj1, WorldObject const* obj2, bool is3D /* = true */) const
{
    float dx1 = GetPositionX() - obj1->GetPositionX();
    float dy1 = GetPositionY() - obj1->GetPositionY();
    float distsq1 = dx1*dx1 + dy1*dy1;
    if (is3D)
    {
        float dz1 = GetPositionZ() - obj1->GetPositionZ();
        distsq1 += dz1*dz1;
    }

    float dx2 = GetPositionX() - obj2->GetPositionX();
    float dy2 = GetPositionY() - obj2->GetPositionY();
    float distsq2 = dx2*dx2 + dy2*dy2;
    if (is3D)
    {
        float dz2 = GetPositionZ() - obj2->GetPositionZ();
        distsq2 += dz2*dz2;
    }

    return distsq1 < distsq2;
}

bool WorldObject::IsInRange(WorldObject const* obj, float minRange, float maxRange, bool is3D /* = true */) const
{
    float dx = GetPositionX() - obj->GetPositionX();
    float dy = GetPositionY() - obj->GetPositionY();
    float distsq = dx*dx + dy*dy;
    if (is3D)
    {
        float dz = GetPositionZ() - obj->GetPositionZ();
        distsq += dz*dz;
    }

    float sizefactor = GetObjectSize() + obj->GetObjectSize();

    // check only for real range
    if (minRange > 0.0f)
    {
        float mindist = minRange + sizefactor;
        if (distsq < mindist * mindist)
            return false;
    }

    float maxdist = maxRange + sizefactor;
    return distsq < maxdist * maxdist;
}

bool WorldObject::IsInRange2d(float x, float y, float minRange, float maxRange) const
{
    float dx = GetPositionX() - x;
    float dy = GetPositionY() - y;
    float distsq = dx*dx + dy*dy;

    float sizefactor = GetObjectSize();

    // check only for real range
    if (minRange > 0.0f)
    {
        float mindist = minRange + sizefactor;
        if (distsq < mindist * mindist)
            return false;
    }

    float maxdist = maxRange + sizefactor;
    return distsq < maxdist * maxdist;
}

bool WorldObject::IsInRange3d(float x, float y, float z, float minRange, float maxRange) const
{
    float dx = GetPositionX() - x;
    float dy = GetPositionY() - y;
    float dz = GetPositionZ() - z;
    float distsq = dx*dx + dy*dy + dz*dz;

    float sizefactor = GetObjectSize();

    // check only for real range
    if (minRange > 0.0f)
    {
        float mindist = minRange + sizefactor;
        if (distsq < mindist * mindist)
            return false;
    }

    float maxdist = maxRange + sizefactor;
    return distsq < maxdist * maxdist;
}

void Position::RelocateOffset(const Position & offset)
{
    m_positionX = GetPositionX() + (offset.GetPositionX() * cos(GetOrientation()) + offset.GetPositionY() * sin(GetOrientation() + M_PI));
    m_positionY = GetPositionY() + (offset.GetPositionY() * cos(GetOrientation()) + offset.GetPositionX() * sin(GetOrientation()));
    m_positionZ = GetPositionZ() + offset.GetPositionZ();
    SetOrientation(GetOrientation() + offset.GetOrientation());
}

void Position::GetPositionOffsetTo(const Position & endPos, Position & retOffset) const
{
    float dx = endPos.GetPositionX() - GetPositionX();
    float dy = endPos.GetPositionY() - GetPositionY();

    retOffset.m_positionX = dx * cos(GetOrientation()) + dy * sin(GetOrientation());
    retOffset.m_positionY = dy * cos(GetOrientation()) - dx * sin(GetOrientation());
    retOffset.m_positionZ = endPos.GetPositionZ() - GetPositionZ();
    retOffset.SetOrientation(endPos.GetOrientation() - GetOrientation());
}

float Position::GetAngle(const Position *obj) const
{
    if (!obj) return 0;
    return GetAngle(obj->GetPositionX(), obj->GetPositionY());
}

// Return angle in range 0..2*pi
float Position::GetAngle(const float x, const float y) const
{
    float dx = x - GetPositionX();
    float dy = y - GetPositionY();

    float ang = atan2(dy, dx);
    ang = (ang >= 0) ? ang : 2 * M_PI + ang;
    return ang;
}

void Position::GetSinCos(const float x, const float y, float &vsin, float &vcos) const
{
    float dx = GetPositionX() - x;
    float dy = GetPositionY() - y;

    if (fabs(dx) < 0.001f && fabs(dy) < 0.001f)
    {
        float angle = (float)rand_norm()*static_cast<float>(2*M_PI);
        vcos = cos(angle);
        vsin = sin(angle);
    }
    else
    {
        float dist = sqrt((dx*dx) + (dy*dy));
        vcos = dx / dist;
        vsin = dy / dist;
    }
}

bool Position::HasInArc(float arc, const Position *obj) const
{
    // always have self in arc
    if (obj == this)
        return true;

    // move arc to range 0.. 2*pi
    arc = MapManager::NormalizeOrientation(arc);

    float angle = GetAngle(obj);
    angle -= m_orientation;

    // move angle to range -pi ... +pi
    angle = MapManager::NormalizeOrientation(angle);
    if(angle > M_PI)
        angle -= 2.0f*M_PI;

    float lborder =  -1 * (arc/2.0f);                       // in range -pi..0
    float rborder = (arc/2.0f);                             // in range 0..pi
    return ((angle >= lborder) && (angle <= rborder));
}

bool WorldObject::IsInBetween(const WorldObject *obj1, const WorldObject *obj2, float size) const
{
    if (GetPositionX() > std::max(obj1->GetPositionX(), obj2->GetPositionX())
        || GetPositionX() < std::min(obj1->GetPositionX(), obj2->GetPositionX())
        || GetPositionY() > std::max(obj1->GetPositionY(), obj2->GetPositionY())
        || GetPositionY() < std::min(obj1->GetPositionY(), obj2->GetPositionY()))
        return false;

    if (!size)
        size = GetObjectSize() / 2;

    float angle = obj1->GetAngle(this) - obj1->GetAngle(obj2);
    return fabs(sin(angle)) * GetExactDist2d(obj1->GetPositionX(), obj1->GetPositionY()) < size;
}

bool WorldObject::HasEdgeOnPathTo(Position *dst)
{
    /**
     * Determines, whether there is some edge in the way or not
     *
     * The basic idea is to choose points along the path 0.33 yard distant from each other
     * then get the height of every of them and determine, if there is more than 45 degrees
     * height angle. If yes, then it's considered an edge.
     */

    /**
     * pointDistance = distance of points along the path. Also used for checking the height difference
     * startX, startY, startZ = starting coordinates
     * deltaX, deltaY, deltaZ = difference between starting and end point (vector-like, with polarity)
     */

    static const float pointDistance = 0.33f;

    float distance = GetExactDist2d(dst);
    uint32 numpoints = distance / pointDistance; // check on every third of yard

    float startX = GetPositionX();
    float deltaX = dst->m_positionX - startX;
    float startY = GetPositionY();
    float deltaY = dst->m_positionY - startY;
    float startZ = GetPositionZ();
    float deltaZ = dst->m_positionZ - startZ;

    // starting height is our Z position
    float prevHeight = startZ;
    float tmpHeight;
    for (uint32 i = 1; i <= numpoints; i++)
    {
        tmpHeight = GetMap()->GetHeight(GetPhaseMask(), startX + ((float)i)/((float)numpoints)*deltaX, startY + ((float)i)/((float)numpoints)*deltaY, startZ + ((float)i)/((float)numpoints)*deltaZ, true);
        if (fabs(prevHeight - tmpHeight) > pointDistance)
            return true;
    }

    return false;
}

bool WorldObject::HasFlatPathTo(Position *dst)
{
    /* on a path between this object and dst position, select a series
     * of N points (N being distance/1yd) and determine whether
     * the path is flat enough for whatever we need this function to */

    /* use only x,y distance, z is irrelevant */
    float distance = GetExactDist2d(dst);

    /* just a number of points (int)
     * for distance ie. 2-2.9yd, use 4 points (incl. start + end) */
    int numpoints = (int)distance + 2;

    /* start + path[total-2] + end */
    std::vector<float> pointX(numpoints);
    std::vector<float> pointY(numpoints);
    std::vector<float> pointZ(numpoints);
    /* distance between points can therefore be only < 1.0f,
     * it's extremes shrinking with larger distances */
    float pointdist = distance / (numpoints-1);

    int i;

    for (i = 0; i < numpoints; i++) {
        pointX[i] = GetPositionX() + ((dst->GetPositionX() - GetPositionX()) * ((float)i/(float)(numpoints-1)));
        pointY[i] = GetPositionY() + ((dst->GetPositionY() - GetPositionY()) * ((float)i/(float)(numpoints-1)));
        pointZ[i] = GetPositionZ() + ((dst->GetPositionZ() - GetPositionZ()) * ((float)i/(float)(numpoints-1)));
    }

    /* adjust height to nearest ground level
     * - be smart, leave source/destination intact to avoid cheating */
    for (i = 1; i < numpoints-1; i++)
        pointZ[i] = GetMap()->GetHeight2(pointX[i], pointY[i], pointZ[i]);

    /* if any of the points has invalid height, fail */
    for (i = 0; i < numpoints; i++)
        if (pointZ[i] == INVALID_HEIGHT)
            return false;

    /* excluded algorithms:
     *
     * --- height difference ---
     * 1: find minimum and maximum and compare them against algo 2
     * 2: use height difference (calculated from an angle, say, 45 degrees)
     *    between source and destination
     * 3: if the height diff between any two points on the path is larger than
     *    the x,y distance between those points (eg. if the angle between
     *    point N and point N+1 is 45 (or more) degrees up/down), remember that
     *    - if it happens again, the path is not really flat
     *
     * --- valley detection ---
     * 4: remember the amount of increased/decreased height between points
     *    and if both (and only both) of those totals are above 1/5 of the total
     *    distance between start/end, then there's a valley on the path
     * 5: look for a point A with a negative fatal (>4yd) height difference against
     *    a previous one - if such point is found, remember it and look for
     *    another point B, which is at most 3yd below A (and still below A+0.5)
     *    - if point B is found, compare it's distance to point A with a regular
     *      player jump distance (around 5yd)
     *      - if within that range, look for another fatal point,
     *      - else ... well, the valley is just too wide
     */

    /* used algorithm: very simplistic, allow any negative jump (down)
     * if no point N on the path is higher than point N-1 by more than
     * the x,y distance between those points (eg. 45 degrees)
     * - apply the same rule on positive jumps */
    for (i = 1; i < numpoints; i++)
        if (pointZ[i] - pointZ[i-1] > pointdist)
            return false;

    return true;
}

bool WorldObject::isInFront(WorldObject const* target, float distance,  float arc) const
{
    return IsWithinDist(target, distance) && HasInArc(arc, target);
}

bool WorldObject::isInBack(WorldObject const* target, float distance, float arc) const
{
    return IsWithinDist(target, distance) && !HasInArc(2 * M_PI - arc, target);
}

void WorldObject::GetRandomPoint(const Position &pos, float distance, float &rand_x, float &rand_y, float &rand_z) const
{
    if (!distance)
    {
        pos.GetPosition(rand_x, rand_y, rand_z);
        return;
    }

    // angle to face `obj` to `this`
    float angle = (float)rand_norm()*static_cast<float>(2*M_PI);
    float new_dist = (float)rand_norm()*static_cast<float>(distance);

    rand_x = pos.m_positionX + new_dist * cos(angle);
    rand_y = pos.m_positionY + new_dist * sin(angle);
    rand_z = pos.m_positionZ;

    Trinity::NormalizeMapCoord(rand_x);
    Trinity::NormalizeMapCoord(rand_y);
    UpdateGroundPositionZ(rand_x,rand_y,rand_z);            // update to LOS height if available
}

void WorldObject::UpdateGroundPositionZ(float x, float y, float &z) const
{
    float new_z = GetBaseMap()->GetHeight(GetPhaseMask(),x,y,z,true);
    if (new_z > INVALID_HEIGHT)
        z = new_z+ 0.05f;                                   // just to be sure that we are not a few pixel under the surface
}

bool Position::IsPositionValid() const
{
    return Trinity::IsValidMapCoord(m_positionX,m_positionY,m_positionZ,m_orientation);
}

void WorldObject::MonsterSay(const char* text, uint32 language, uint64 TargetGuid, float customRadius)
{
    WorldPacket data(SMSG_MESSAGECHAT, 200);
    BuildMonsterChat(&data,CHAT_MSG_MONSTER_SAY,text,language,GetName(),TargetGuid);
    SendMessageToSetInRange(&data,customRadius == 0.0f ? sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY) : customRadius,true);
}

void WorldObject::MonsterYell(const char* text, uint32 language, uint64 TargetGuid, float customRadius)
{
    WorldPacket data(SMSG_MESSAGECHAT, 200);
    BuildMonsterChat(&data,CHAT_MSG_MONSTER_YELL,text,language,GetName(),TargetGuid);
    SendMessageToSetInRange(&data,customRadius == 0.0f ? sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_YELL) : customRadius,true);
}

void WorldObject::MonsterTextEmote(const char* text, uint64 TargetGuid, bool IsBossEmote, float customRadius)
{
    WorldPacket data(SMSG_MESSAGECHAT, 200);
    BuildMonsterChat(&data,IsBossEmote ? CHAT_MSG_RAID_BOSS_EMOTE : CHAT_MSG_MONSTER_EMOTE,text,LANG_UNIVERSAL,GetName(),TargetGuid);
    SendMessageToSetInRange(&data,customRadius == 0.0f ? sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_TEXTEMOTE) : customRadius,true);
}

void WorldObject::MonsterWhisper(const char* text, uint64 receiver, bool IsBossWhisper)
{
    Player *player = sObjectMgr->GetPlayer(receiver);
    if (!player || !player->GetSession())
        return;

    WorldPacket data(SMSG_MESSAGECHAT, 200);
    BuildMonsterChat(&data,IsBossWhisper ? CHAT_MSG_RAID_BOSS_WHISPER : CHAT_MSG_MONSTER_WHISPER,text,LANG_UNIVERSAL,GetName(),receiver);

    player->GetSession()->SendPacket(&data);
}

void WorldObject::SendPlaySound(uint32 Sound, bool OnlySelf)
{
    WorldPacket data(SMSG_PLAY_SOUND, 4);
    data << Sound;
    if (OnlySelf && GetTypeId() == TYPEID_PLAYER)
        this->ToPlayer()->GetSession()->SendPacket(&data);
    else
        SendMessageToSet(&data, true); // ToSelf ignored in this case
}

void Object::ForceValuesUpdateAtIndex(uint32 i)
{
    _changesMask.SetBit(i);
    if (m_inWorld)
    {
        if (!m_objectUpdated)
        {
            sObjectAccessor->AddUpdateObject(this);
            m_objectUpdated = true;
        }
    }
}

namespace Trinity
{
    class MonsterChatBuilder
    {
        public:
            MonsterChatBuilder(WorldObject const& obj, ChatMsg msgtype, int32 textId, uint32 language, uint64 targetGUID)
                : i_object(obj), i_msgtype(msgtype), i_textId(textId), i_language(language), i_targetGUID(targetGUID) {}
            void operator()(WorldPacket& data, LocaleConstant loc_idx)
            {
                char const* text = sObjectMgr->GetTrinityString(i_textId,loc_idx);

                // TODO: i_object.GetName() also must be localized?
                i_object.BuildMonsterChat(&data,i_msgtype,text,i_language,i_object.GetNameForLocaleIdx(loc_idx),i_targetGUID);
            }

        private:
            WorldObject const& i_object;
            ChatMsg i_msgtype;
            int32 i_textId;
            uint32 i_language;
            uint64 i_targetGUID;
    };
}                                                           // namespace Trinity

void WorldObject::MonsterSay(int32 textId, uint32 language, uint64 TargetGuid)
{
    CellPair p = Trinity::ComputeCellPair(GetPositionX(), GetPositionY());

    Cell cell(p);
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();

    Trinity::MonsterChatBuilder say_build(*this, CHAT_MSG_MONSTER_SAY, textId,language,TargetGuid);
    Trinity::LocalizedPacketDo<Trinity::MonsterChatBuilder> say_do(say_build);
    Trinity::PlayerDistWorker<Trinity::LocalizedPacketDo<Trinity::MonsterChatBuilder> > say_worker(this,sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY),say_do);
    TypeContainerVisitor<Trinity::PlayerDistWorker<Trinity::LocalizedPacketDo<Trinity::MonsterChatBuilder> >, WorldTypeMapContainer > message(say_worker);
    cell.Visit(p, message, *GetMap(), *this, sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY));
}

void WorldObject::MonsterYell(int32 textId, uint32 language, uint64 TargetGuid)
{
    CellPair p = Trinity::ComputeCellPair(GetPositionX(), GetPositionY());

    Cell cell(p);
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();

    Trinity::MonsterChatBuilder say_build(*this, CHAT_MSG_MONSTER_YELL, textId,language,TargetGuid);
    Trinity::LocalizedPacketDo<Trinity::MonsterChatBuilder> say_do(say_build);
    Trinity::PlayerDistWorker<Trinity::LocalizedPacketDo<Trinity::MonsterChatBuilder> > say_worker(this,sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_YELL),say_do);
    TypeContainerVisitor<Trinity::PlayerDistWorker<Trinity::LocalizedPacketDo<Trinity::MonsterChatBuilder> >, WorldTypeMapContainer > message(say_worker);
    cell.Visit(p, message, *GetMap(), *this, sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_YELL));
}

void WorldObject::MonsterYellToZone(int32 textId, uint32 language, uint64 TargetGuid)
{
    Trinity::MonsterChatBuilder say_build(*this, CHAT_MSG_MONSTER_YELL, textId,language,TargetGuid);
    Trinity::LocalizedPacketDo<Trinity::MonsterChatBuilder> say_do(say_build);

    uint32 zoneid = GetZoneId();

    Map::PlayerList const& pList = GetMap()->GetPlayers();
    for (Map::PlayerList::const_iterator itr = pList.begin(); itr != pList.end(); ++itr)
        if (itr->getSource()->GetZoneId() == zoneid)
            say_do(itr->getSource());
}

void WorldObject::MonsterTextEmote(int32 textId, uint64 TargetGuid, bool IsBossEmote)
{
    CellPair p = Trinity::ComputeCellPair(GetPositionX(), GetPositionY());

    Cell cell(p);
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();

    Trinity::MonsterChatBuilder say_build(*this, IsBossEmote ? CHAT_MSG_RAID_BOSS_EMOTE : CHAT_MSG_MONSTER_EMOTE, textId,LANG_UNIVERSAL,TargetGuid);
    Trinity::LocalizedPacketDo<Trinity::MonsterChatBuilder> say_do(say_build);
    Trinity::PlayerDistWorker<Trinity::LocalizedPacketDo<Trinity::MonsterChatBuilder> > say_worker(this,sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_TEXTEMOTE),say_do);
    TypeContainerVisitor<Trinity::PlayerDistWorker<Trinity::LocalizedPacketDo<Trinity::MonsterChatBuilder> >, WorldTypeMapContainer > message(say_worker);
    cell.Visit(p, message, *GetMap(), *this, sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_TEXTEMOTE));
}

void WorldObject::MonsterWhisper(int32 textId, uint64 receiver, bool IsBossWhisper)
{
    Player *player = sObjectMgr->GetPlayer(receiver);
    if (!player || !player->GetSession())
        return;

    LocaleConstant loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
    char const* text = sObjectMgr->GetTrinityString(textId, loc_idx);

    WorldPacket data(SMSG_MESSAGECHAT, 200);
    BuildMonsterChat(&data,IsBossWhisper ? CHAT_MSG_RAID_BOSS_WHISPER : CHAT_MSG_MONSTER_WHISPER,text,LANG_UNIVERSAL,GetNameForLocaleIdx(loc_idx),receiver);

    player->GetSession()->SendPacket(&data);
}

void WorldObject::BuildMonsterChat(WorldPacket *data, uint8 msgtype, char const* text, uint32 language, char const* name, uint64 targetGuid) const
{
    *data << (uint8)msgtype;
    *data << (uint32)language;
    *data << (uint64)GetGUID();
    *data << (uint32)0;                                     // 2.1.0
    *data << (uint32)(strlen(name)+1);
    *data << name;
    *data << (uint64)targetGuid;                            // Unit Target
    if (targetGuid && !IS_PLAYER_GUID(targetGuid))
    {
        *data << (uint32)1;                                 // target name length
        *data << (uint8)0;                                  // target name
    }
    *data << (uint32)(strlen(text)+1);
    *data << text;
    *data << (uint8)0;                                      // ChatTag
    if (msgtype == CHAT_MSG_RAID_BOSS_EMOTE)
    {
        *data << float(0);
        *data << uint8(0);
    }
}

void Unit::BuildHeartBeatMsg(WorldPacket *data) const
{
    data->Initialize(MSG_MOVE_HEARTBEAT, 32);
    data->append(GetPackGUID());
    BuildMovementPacket(data);
}

void WorldObject::SendMessageToSet(WorldPacket *data, bool /*fake*/)
{
    Trinity::MessageDistDeliverer notifier(this, data, GetMap()->GetVisibilityDistance());
    VisitNearbyWorldObject(GetMap()->GetVisibilityDistance(), notifier);
}

void WorldObject::SendMessageToSetInRange(WorldPacket *data, float dist, bool /*bToSelf*/)
{
    Trinity::MessageDistDeliverer notifier(this, data, dist);
    VisitNearbyWorldObject(dist, notifier);
}

void WorldObject::SendMessageToSet(WorldPacket *data, Player const* skipped_rcvr)
{
    Trinity::MessageDistDeliverer notifier(this, data, GetMap()->GetVisibilityDistance(), false, skipped_rcvr);
    VisitNearbyWorldObject(GetMap()->GetVisibilityDistance(), notifier);
}

void WorldObject::SendMessageToSetByStanding(WorldPacket *data_friendly, WorldPacket *data_hostile)
{
    if (GetTypeId() != TYPEID_UNIT && GetTypeId() != TYPEID_PLAYER)
        return;

    Trinity::MessageDistFactionDeliverer notifier(ToUnit(), data_hostile, data_friendly, GetMap()->GetVisibilityDistance());
    VisitNearbyWorldObject(GetMap()->GetVisibilityDistance(), notifier);
}

void WorldObject::SendObjectDeSpawnAnim(uint64 guid)
{
    WorldPacket data(SMSG_GAMEOBJECT_DESPAWN_ANIM, 8);
    data << uint64(guid);
    SendMessageToSet(&data, true);
}

void WorldObject::SetMap(Map * map)
{
    ASSERT(map);
    ASSERT(!IsInWorld() || GetTypeId() == TYPEID_CORPSE);
    if (m_currMap == map) // command add npc: first create, than loadfromdb
        return;
    if (m_currMap)
    {
        sLog->outCrash("WorldObject::SetMap: obj %u new map %u %u, old map %u %u", (uint32)GetTypeId(), map->GetId(), map->GetInstanceId(), m_currMap->GetId(), m_currMap->GetInstanceId());
        ASSERT(false);
    }
    m_currMap = map;
    m_mapId = map->GetId();
    m_InstanceId = map->GetInstanceId();
    if (m_isWorldObject)
        m_currMap->AddWorldObject(this);
}

void WorldObject::ResetMap()
{
    ASSERT(m_currMap);
    ASSERT(!IsInWorld());
    if (m_isWorldObject)
        m_currMap->RemoveWorldObject(this);
    m_currMap = NULL;
    //maybe not for corpse
    //m_mapId = 0;
    //m_InstanceId = 0;
}

Map const* WorldObject::GetBaseMap() const
{
    ASSERT(m_currMap);
    return m_currMap->GetParent();
}

void WorldObject::AddObjectToRemoveList()
{
    ASSERT(m_uint32Values);

    Map* map = FindMap();
    if (!map)
    {
        sLog->outError("Object (TypeId: %u Entry: %u GUID: %u) at attempt add to move list not have valid map (Id: %u).",GetTypeId(),GetEntry(),GetGUIDLow(),GetMapId());
        return;
    }

    map->AddObjectToRemoveList(this);
}

TempSummon *Map::SummonCreature(uint32 entry, const Position &pos, SummonPropertiesEntry const *properties, uint32 duration, Unit *summoner, uint32 vehId, uint32 lowGUID)
{
    uint32 mask = UNIT_MASK_SUMMON;
    if (properties)
    {
        switch(properties->Category)
        {
            case SUMMON_CATEGORY_PET:       mask = UNIT_MASK_GUARDIAN;  break;
            case SUMMON_CATEGORY_PUPPET:    mask = UNIT_MASK_PUPPET;    break;
            case SUMMON_CATEGORY_VEHICLE:   mask = UNIT_MASK_MINION;    break;
            default:
                switch(properties->Type)
                {
                    case SUMMON_TYPE_MINION:
                    case SUMMON_TYPE_GUARDIAN:
                    case SUMMON_TYPE_GUARDIAN2:
                        mask = UNIT_MASK_GUARDIAN;  break;
                    case SUMMON_TYPE_TOTEM:
                        mask = UNIT_MASK_TOTEM;     break;
                    case SUMMON_TYPE_VEHICLE:
                    case SUMMON_TYPE_VEHICLE2:
                        mask = UNIT_MASK_SUMMON;    break;
                    case SUMMON_TYPE_MINIPET:
                        mask = UNIT_MASK_MINION;    break;
                    default:
                        if (properties->Flags & 512) // Mirror Image, Summon Gargoyle
                            mask = UNIT_MASK_GUARDIAN;
                        break;
                }
                break;
        }
    }

    uint32 phase = PHASEMASK_NORMAL, team = 0;
    if (summoner)
    {
        phase = summoner->GetPhaseMask();
        if (summoner->GetTypeId() == TYPEID_PLAYER)
            team = summoner->ToPlayer()->GetTeam();
    }

    TempSummon *summon = NULL;
    switch(mask)
    {
        case UNIT_MASK_SUMMON:    summon = new TempSummon (properties, summoner);  break;
        case UNIT_MASK_GUARDIAN:  summon = new Guardian   (properties, summoner);  break;
        case UNIT_MASK_PUPPET:    summon = new Puppet     (properties, summoner);  break;
        case UNIT_MASK_TOTEM:     summon = new Totem      (properties, summoner);  break;
        case UNIT_MASK_MINION:    summon = new Minion     (properties, summoner);  break;
        default:    return NULL;
    }
    if(!lowGUID)
    {
        lowGUID = sObjectMgr->GenerateLowGuidForUnit(true);
    }

    if (!summon->Create(lowGUID, this, phase, entry, vehId, team, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation()))
    {
        delete summon;
        return NULL;
    }

    summon->SetHomePosition(pos);

    summon->InitStats(duration);
    Add(summon->ToCreature());
    summon->InitSummon();

    //ObjectAccessor::UpdateObjectVisibility(summon);

    return summon;
}

void WorldObject::SetZoneScript(ZoneScript* src)
{
    if (src)
    {
        m_zoneScript = src;
        return;
    }

    if (Map *map = FindMap())
    {
        if (map->IsDungeon())
            m_zoneScript = (ZoneScript*)((InstanceMap*)map)->GetInstanceScript();
        else if (!map->IsBattlegroundOrArena())
        {
            if (Battlefield* bf = sBattlefieldMgr.GetBattlefieldToZoneId(GetZoneId()))
                m_zoneScript = bf;
            else
                m_zoneScript = sOutdoorPvPMgr->GetZoneScript(GetZoneId());
        }
    }
}

TempSummon* WorldObject::SummonCreature(uint32 entry, const Position &pos, TempSummonType spwtype, uint32 duration, uint32 vehId) const
{
    if (Map *map = FindMap())
    {
        if (TempSummon *summon = map->SummonCreature(entry, pos, NULL, duration, isType(TYPEMASK_UNIT) ? (Unit*)this : NULL, vehId))
        {
            summon->SetTempSummonType(spwtype);
            return summon;
        }
    }

    return NULL;
}

Pet* Player::SummonPet(uint32 entry, float x, float y, float z, float ang, PetType petType, uint32 duration, PetSlot slotID)
{
    Pet* pet = new Pet(this, petType);
    pet->SetSlot(slotID);

    if (petType == SUMMON_PET && pet->LoadPetFromDB(this, entry, 0, slotID != PET_SLOT_UNK_SLOT, slotID))
    {
        // Remove Demonic Sacrifice auras (known pet)
        Unit::AuraEffectList const& auraClassScripts = GetAuraEffectsByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
        for (Unit::AuraEffectList::const_iterator itr = auraClassScripts.begin(); itr != auraClassScripts.end();)
        {
            if ((*itr)->GetMiscValue() == 2228)
            {
                RemoveAurasDueToSpell((*itr)->GetId());
                itr = auraClassScripts.begin();
            }
            else
                ++itr;
        }

        if (duration > 0)
            pet->SetDuration(duration);

        if (pet)
        {
            if (pet->IsHunterPet())
            {
                // Spirit Bond effect
                if (HasAura(19578) && !HasAura(19579))
                    CastSpell(this, 19579, true);
                if (HasAura(20895) && !HasAura(24529))
                    CastSpell(this, 24529, true);

                // Ferocious Inspiration
                if (HasAura(34460))
                    pet->CastSpell(pet, 75447, true);
            }
            else
            {
                // Regenerate health and mana of non-hunter pets
                pet->SetHealth(pet->GetMaxHealth());
                pet->SetPower(pet->getPowerType(), pet->GetMaxPower(pet->getPowerType()));
            }
        }

        return NULL;
    }

    // petentry == 0 for hunter "call pet" (current pet summoned if any)
    if (!entry)
    {
        delete pet;
        return NULL;
    }

    pet->Relocate(x, y, z, ang);
    if (!pet->IsPositionValid())
    {
        sLog->outError("Pet (guidlow %d, entry %d) not summoned. Suggested coordinates isn't valid (X: %f Y: %f)",pet->GetGUIDLow(),pet->GetEntry(),pet->GetPositionX(),pet->GetPositionY());
        delete pet;
        return NULL;
    }

    Map *map = GetMap();
    uint32 pet_number = sObjectMgr->GeneratePetNumber();
    if (!pet->Create(pet_number, map, GetPhaseMask(), entry))
    {
        sLog->outError("no such creature entry %u", entry);
        delete pet;
        return NULL;
    }

    pet->SetCreatorGUID(GetGUID());
    pet->SetUInt32Value(UNIT_FIELD_FACTIONTEMPLATE, getFaction());

    pet->setPowerType(POWER_MANA);
    pet->SetUInt32Value(UNIT_NPC_FLAGS , 0);
    pet->SetUInt32Value(UNIT_FIELD_BYTES_1,0);
    pet->InitStatsForLevel(getLevel());

    SetMinion(pet, true);

    switch(petType)
    {
        case SUMMON_PET:
            // this enables pet details window (Shift+P)
            pet->GetCharmInfo()->SetPetNumber(pet_number, true);

            if (pet->IsPetGhoul())
                pet->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_ROGUE); // technically, it's a rogue (has energy)
            else
                pet->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_MAGE);

            pet->SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, 0);
            pet->SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, 1000);
            pet->SetFullHealth();
            pet->SetPower(POWER_MANA, pet->GetMaxPower(POWER_MANA));
            pet->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(time(NULL))); // cast can't be helped in this case
            break;
        default:
            break;
    }

    map->Add(pet->ToCreature());

    switch(petType)
    {
        case SUMMON_PET:
            pet->InitPetCreateSpells();
            pet->InitTalentForLevel();
            pet->SavePetToDB();
            PetSpellInitialize();
            break;
        default:
            break;
    }

    if (petType == SUMMON_PET)
    {
        // Remove Demonic Sacrifice auras (known pet)
        Unit::AuraEffectList const& auraClassScripts = GetAuraEffectsByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
        for (Unit::AuraEffectList::const_iterator itr = auraClassScripts.begin(); itr != auraClassScripts.end();)
        {
            if ((*itr)->GetMiscValue() == 2228)
            {
                RemoveAurasDueToSpell((*itr)->GetId());
                itr = auraClassScripts.begin();
            }
            else
                ++itr;
        }
    }

    if (duration > 0)
        pet->SetDuration(duration);

    //ObjectAccessor::UpdateObjectVisibility(pet);

    return pet;
}

GameObject* WorldObject::SummonGameObject(uint32 entry, float x, float y, float z, float ang, float rotation0, float rotation1, float rotation2, float rotation3, uint32 respawnTime)
{
    if (!IsInWorld())
        return NULL;

    GameObjectInfo const* goinfo = sObjectMgr->GetGameObjectInfo(entry);
    if (!goinfo)
    {
        sLog->outErrorDb("Gameobject template %u not found in database!", entry);
        return NULL;
    }
    Map *map = GetMap();
    GameObject *go = (goinfo->type == GAMEOBJECT_TYPE_TRANSPORT) ? new DynamicTransport() : new GameObject();
    if (!go->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_GAMEOBJECT), entry, map, GetPhaseMask(), x,y,z,ang,rotation0,rotation1,rotation2,rotation3,100,GO_STATE_READY))
    {
        delete go;
        return NULL;
    }
    go->SetRespawnTime(respawnTime);
    if (GetTypeId() == TYPEID_PLAYER || GetTypeId() == TYPEID_UNIT) //not sure how to handle this
        ((Unit*)this)->AddGameObject(go);
    else
        go->SetSpawnedByDefault(false);
    map->Add(go);

    return go;
}

Creature* WorldObject::SummonTrigger(float x, float y, float z, float ang, uint32 duration, CreatureAI* (*GetAI)(Creature*))
{
    TempSummonType summonType = (duration == 0) ? TEMPSUMMON_DEAD_DESPAWN : TEMPSUMMON_TIMED_DESPAWN;
    Creature* summon = SummonCreature(WORLD_TRIGGER, x, y, z, ang, summonType, duration);
    if (!summon)
        return NULL;

    //summon->SetName(GetName());
    if (GetTypeId() == TYPEID_PLAYER || GetTypeId() == TYPEID_UNIT)
    {
        summon->setFaction(((Unit*)this)->getFaction());
        summon->SetLevel(((Unit*)this)->getLevel());
    }

    if (GetAI)
        summon->AIM_Initialize(GetAI(summon));
    return summon;
}

Player* WorldObject::FindNearestPlayer(float range, bool alive)
{
    Player *pl = NULL;
    Trinity::NearestPlayerWithLiveStateInObjectRangeCheck checker(*this, alive, range);
    Trinity::PlayerLastSearcher<Trinity::NearestPlayerWithLiveStateInObjectRangeCheck> searcher(this, pl, checker);
    VisitNearbyObject(range, searcher);
    return pl;
}

Creature* WorldObject::FindNearestCreature(uint32 entry, float range, bool alive)
{
    Creature *creature = NULL;
    Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck checker(*this, entry, alive, range);
    Trinity::CreatureLastSearcher<Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck> searcher(this, creature, checker);
    VisitNearbyObject(range, searcher);
    return creature;
}

Creature* WorldObject::FindNearestCreatureByDBGuid(uint32 dbGuid, float range, bool alive)
{
    Creature *creature = NULL;
    Trinity::NearestCreatureDBGuidWithLiveStateInObjectRangeCheck checker(*this, dbGuid, alive, range);
    Trinity::CreatureLastSearcher<Trinity::NearestCreatureDBGuidWithLiveStateInObjectRangeCheck> searcher(this, creature, checker);
    VisitNearbyObject(range, searcher);
    return creature;
}

GameObject* WorldObject::FindNearestGameObject(uint32 entry, float range) const
{
    GameObject *go = NULL;
    Trinity::NearestGameObjectEntryInObjectRangeCheck checker(*this, entry, range);
    Trinity::GameObjectLastSearcher<Trinity::NearestGameObjectEntryInObjectRangeCheck> searcher(this, go, checker);
    VisitNearbyGridObject(range, searcher);
    return go;
}

void WorldObject::GetGameObjectListWithEntryInGrid(std::list<GameObject*>& lList, uint32 uiEntry, float fMaxSearchRange)
{
    CellPair pair(Trinity::ComputeCellPair(this->GetPositionX(), this->GetPositionY()));
    Cell cell(pair);
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();

    Trinity::AllGameObjectsWithEntryInRange check(this, uiEntry, fMaxSearchRange);
    Trinity::GameObjectListSearcher<Trinity::AllGameObjectsWithEntryInRange> searcher(this, lList, check);
    TypeContainerVisitor<Trinity::GameObjectListSearcher<Trinity::AllGameObjectsWithEntryInRange>, GridTypeMapContainer> visitor(searcher);

    cell.Visit(pair, visitor, *(this->GetMap()));
}

void WorldObject::GetCreatureListWithEntryInGrid(std::list<Creature*>& lList, uint32 uiEntry, float fMaxSearchRange)
{
    CellPair pair(Trinity::ComputeCellPair(this->GetPositionX(), this->GetPositionY()));
    Cell cell(pair);
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();

    Trinity::AllCreaturesOfEntryInRange check(this, uiEntry, fMaxSearchRange);
    Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(this, lList, check);
    TypeContainerVisitor<Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange>, GridTypeMapContainer> visitor(searcher);

    cell.Visit(pair, visitor, *(this->GetMap()));
}

/*
namespace Trinity
{
    class NearUsedPosDo
    {
        public:
            NearUsedPosDo(WorldObject const& obj, WorldObject const* searcher, float angle, ObjectPosSelector& selector)
                : i_object(obj), i_searcher(searcher), i_angle(angle), i_selector(selector) {}

            void operator()(Corpse*) const {}
            void operator()(DynamicObject*) const {}

            void operator()(Creature* c) const
            {
                // skip self or target
                if (c == i_searcher || c == &i_object)
                    return;

                float x,y,z;

                if (!c->IsAlive() || c->HasUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED | UNIT_STATE_DISTRACTED) ||
                    !c->GetMotionMaster()->GetDestination(x,y,z))
                {
                    x = c->GetPositionX();
                    y = c->GetPositionY();
                }

                add(c,x,y);
            }

            template<class T>
                void operator()(T* u) const
            {
                // skip self or target
                if (u == i_searcher || u == &i_object)
                    return;

                float x,y;

                x = u->GetPositionX();
                y = u->GetPositionY();

                add(u,x,y);
            }

            // we must add used pos that can fill places around center
            void add(WorldObject* u, float x, float y) const
            {
                // u is too nearest/far away to i_object
                if (!i_object.IsInRange2d(x,y,i_selector.m_dist - i_selector.m_size,i_selector.m_dist + i_selector.m_size))
                    return;

                float angle = i_object.GetAngle(u)-i_angle;

                // move angle to range -pi ... +pi
                while (angle > M_PI)
                    angle -= 2.0f * M_PI;
                while (angle < -M_PI)
                    angle += 2.0f * M_PI;

                // dist include size of u
                float dist2d = i_object.GetDistance2d(x,y);
                i_selector.AddUsedPos(u->GetObjectSize(),angle,dist2d + i_object.GetObjectSize());
            }
        private:
            WorldObject const& i_object;
            WorldObject const* i_searcher;
            float              i_angle;
            ObjectPosSelector& i_selector;
    };
}                                                           // namespace Trinity
*/

//===================================================================================================

void WorldObject::GetNearPoint2D(float &x, float &y, float distance2d, float absAngle) const
{
    x = GetPositionX() + (GetObjectSize() + distance2d) * cos(absAngle);
    y = GetPositionY() + (GetObjectSize() + distance2d) * sin(absAngle);

    Trinity::NormalizeMapCoord(x);
    Trinity::NormalizeMapCoord(y);
}

void WorldObject::GetNearPoint(WorldObject const* /*searcher*/, float &x, float &y, float &z, float searcher_size, float distance2d, float absAngle) const
{
    GetNearPoint2D(x,y,distance2d+searcher_size,absAngle);
    z = GetPositionZ();
    UpdateAllowedPositionZ(x,y,z);

    /*
    // if detection disabled, return first point
    if (!sWorld->getIntConfig(CONFIG_DETECT_POS_COLLISION))
    {
        UpdateGroundPositionZ(x,y,z);                       // update to LOS height if available
        return;
    }

    // or remember first point
    float first_x = x;
    float first_y = y;
    bool first_los_conflict = false;                        // first point LOS problems

    // prepare selector for work
    ObjectPosSelector selector(GetPositionX(),GetPositionY(),GetObjectSize(),distance2d+searcher_size);

    // adding used positions around object
    {
        CellPair p(Trinity::ComputeCellPair(GetPositionX(), GetPositionY()));
        Cell cell(p);
        cell.data.Part.reserved = ALL_DISTRICT;
        cell.SetNoCreate();

        Trinity::NearUsedPosDo u_do(*this,searcher,absAngle,selector);
        Trinity::WorldObjectWorker<Trinity::NearUsedPosDo> worker(this,u_do);

        TypeContainerVisitor<Trinity::WorldObjectWorker<Trinity::NearUsedPosDo>, GridTypeMapContainer  > grid_obj_worker(worker);
        TypeContainerVisitor<Trinity::WorldObjectWorker<Trinity::NearUsedPosDo>, WorldTypeMapContainer > world_obj_worker(worker);

        CellLock<GridReadGuard> cell_lock(cell, p);
        cell_lock->Visit(cell_lock, grid_obj_worker,  *GetMap(), *this, distance2d);
        cell_lock->Visit(cell_lock, world_obj_worker, *GetMap(), *this, distance2d);
    }

    // maybe can just place in primary position
    if (selector.CheckOriginal())
    {
        UpdateGroundPositionZ(x,y,z);                       // update to LOS height if available

        if (IsWithinLOS(x,y,z))
            return;

        first_los_conflict = true;                          // first point have LOS problems
    }

    float angle;                                            // candidate of angle for free pos

    // special case when one from list empty and then empty side preferred
    if (selector.FirstAngle(angle))
    {
        GetNearPoint2D(x,y,distance2d,absAngle+angle);
        z = GetPositionZ();
        UpdateGroundPositionZ(x,y,z);                       // update to LOS height if available

        if (IsWithinLOS(x,y,z))
            return;
    }

    // set first used pos in lists
    selector.InitializeAngle();

    // select in positions after current nodes (selection one by one)
    while (selector.NextAngle(angle))                        // angle for free pos
    {
        GetNearPoint2D(x,y,distance2d,absAngle+angle);
        z = GetPositionZ();
        UpdateGroundPositionZ(x,y,z);                       // update to LOS height if available

        if (IsWithinLOS(x,y,z))
            return;
    }

    // BAD NEWS: not free pos (or used or have LOS problems)
    // Attempt find _used_ pos without LOS problem

    if (!first_los_conflict)
    {
        x = first_x;
        y = first_y;

        UpdateGroundPositionZ(x,y,z);                       // update to LOS height if available
        return;
    }

    // special case when one from list empty and then empty side preferred
    if (selector.IsNonBalanced())
    {
        if (!selector.FirstAngle(angle))                     // _used_ pos
        {
            GetNearPoint2D(x,y,distance2d,absAngle+angle);
            z = GetPositionZ();
            UpdateGroundPositionZ(x,y,z);                   // update to LOS height if available

            if (IsWithinLOS(x,y,z))
                return;
        }
    }

    // set first used pos in lists
    selector.InitializeAngle();

    // select in positions after current nodes (selection one by one)
    while (selector.NextUsedAngle(angle))                    // angle for used pos but maybe without LOS problem
    {
        GetNearPoint2D(x,y,distance2d,absAngle+angle);
        z = GetPositionZ();
        UpdateGroundPositionZ(x,y,z);                       // update to LOS height if available

        if (IsWithinLOS(x,y,z))
            return;
    }

    // BAD BAD NEWS: all found pos (free and used) have LOS problem :(
    x = first_x;
    y = first_y;

    UpdateGroundPositionZ(x,y,z);                           // update to LOS height if available
    */
}

void WorldObject::MovePosition(Position &pos, float dist, float angle)
{
    angle += m_orientation;
    pos.m_positionX += dist * cos(angle);
    pos.m_positionY += dist * sin(angle);
    Trinity::NormalizeMapCoord(pos.m_positionX);
    Trinity::NormalizeMapCoord(pos.m_positionY);
    UpdateAllowedPositionZ(pos.m_positionX, pos.m_positionY, pos.m_positionZ);
    pos.m_orientation = m_orientation;
}

void WorldObject::MovePositionToFirstCollision(Position &pos, float dist, float angle)
{
    angle += m_orientation;
    float destx, desty, destz, ground, floor;

    destx = pos.m_positionX + dist * cos(angle);
    desty = pos.m_positionY + dist * sin(angle);
    ground = GetMap()->GetHeight(GetPhaseMask(), destx, desty, MAX_HEIGHT, true);
    floor = pos.m_positionZ;
    UpdateAllowedPositionZ(destx, desty, floor);
    destz = fabs(ground - pos.m_positionZ) <= fabs(floor - pos.m_positionZ) ? ground : floor;

    bool col = VMAP::VMapFactory::createOrGetVMapManager()->getObjectHitPos(GetMapId(),pos.m_positionX,pos.m_positionY,pos.m_positionZ+0.5f,destx,desty,destz+0.5f,destx,desty,destz,-0.5f);

    // collision occured
    if (col)
    {
        // move back a bit
        destx -= CONTACT_DISTANCE * cos(angle);
        desty -= CONTACT_DISTANCE * sin(angle);
        dist = sqrt((pos.m_positionX - destx)*(pos.m_positionX - destx) + (pos.m_positionY - desty)*(pos.m_positionY - desty));
    }

    float step = dist/10.0f;

    for (uint8 j = 0; j < 10; ++j)
    {
        // do not allow too big z changes
        if (fabs(pos.m_positionZ - destz) > 6)
        {
            destx -= step * cos(angle);
            desty -= step * sin(angle);
            ground = GetMap()->GetHeight(GetPhaseMask(), destx, desty, MAX_HEIGHT, true);
            floor = destz+2.0f;
            UpdateAllowedPositionZ(destx, desty, floor);
            destz = fabs(ground - pos.m_positionZ) <= fabs(floor - pos.m_positionZ) ? ground : floor;
        }
        // we have correct destz now
        else
        {
            pos.Relocate(destx, desty, destz);
            break;
        }
    }

    Trinity::NormalizeMapCoord(pos.m_positionX);
    Trinity::NormalizeMapCoord(pos.m_positionY);
    UpdateAllowedPositionZ(pos.m_positionX, pos.m_positionY, pos.m_positionZ);
    pos.m_orientation = m_orientation;
}

void WorldObject::MovePositionToFirstCollisionIncrementally(Position &pos, float dist, float angle)
{
    angle += m_orientation;
    float destx, desty, destz, tmpx, tmpy, startz, lastz, ground, floor;

    destx = pos.m_positionX;
    desty = pos.m_positionY;
    startz = pos.m_positionZ;
    lastz = startz;

    bool startOutdoors = GetMap()->IsOutdoors(pos.m_positionX, pos.m_positionY, pos.m_positionZ);

    ground = GetMap()->GetHeight(GetPhaseMask(), destx, desty, MAX_HEIGHT, true);

    floor = pos.m_positionZ;
    UpdateAllowedPositionZ(destx, desty, floor);
    destz = fabs(ground - pos.m_positionZ) <= fabs(floor - pos.m_positionZ) ? ground : floor;

    float step = dist/10.0f;

    for (uint8 j = 0; j < 10; ++j)
    {
        tmpx = destx + step * cos(angle);
        tmpy = desty + step * sin(angle);

        ground = GetMap()->GetHeight(GetPhaseMask(), tmpx, tmpy, MAX_HEIGHT, true);
        floor = destz+2.0f;
        UpdateAllowedPositionZ(tmpx, tmpy, floor);
        destz = fabs(ground - pos.m_positionZ) <= fabs(floor - pos.m_positionZ) ? ground : floor;

        // collision occured
        if (VMAP::VMapFactory::createOrGetVMapManager()->getObjectHitPos(GetMapId(),destx,desty,lastz+0.5f,tmpx,tmpy,destz+0.5f,destx,desty,destz,-0.5f)
            // or we are changing indoor/outdoor state
            || startOutdoors != GetMap()->IsOutdoors(tmpx, tmpy, destz)
            // or we are dealing with greater Z difference (6 yards, needs adjust?)
            || (fabs(destz-startz) > 6.0f)
            // or if we are dealing with some gameobject in way
            || !GetMap()->isInLineOfSight(destx, desty, lastz+2.0f, tmpx, tmpy, destz+2.0f, GetPhaseMask()))
        {
            // move back a bit
            destx -= CONTACT_DISTANCE * cos(angle);
            desty -= CONTACT_DISTANCE * sin(angle);
            dist = sqrt((pos.m_positionX - destx)*(pos.m_positionX - destx) + (pos.m_positionY - desty)*(pos.m_positionY - desty));
            break;
        }

        destx = tmpx;
        desty = tmpy;
        lastz = destz;
    }

    pos.Relocate(destx, desty, destz);

    Trinity::NormalizeMapCoord(pos.m_positionX);
    Trinity::NormalizeMapCoord(pos.m_positionY);
    UpdateAllowedPositionZ(pos.m_positionX, pos.m_positionY, pos.m_positionZ);
    pos.m_orientation = m_orientation;
}

void WorldObject::SetPhaseMask(uint32 newPhaseMask, bool update)
{
    m_phaseMask = newPhaseMask;

    if (update && IsInWorld())
        UpdateObjectVisibility();
}

void WorldObject::PlayDistanceSound(uint32 sound_id, Player* target /*= NULL*/)
{
    WorldPacket data(SMSG_PLAY_OBJECT_SOUND,4+8);
    data << uint32(sound_id);
    data << uint64(GetGUID());
    if (target)
        target->SendDirectMessage(&data);
    else
        SendMessageToSet(&data, true);
}

void WorldObject::PlayDirectSound(uint32 sound_id, Player* target /*= NULL*/)
{
    WorldPacket data(SMSG_PLAY_SOUND, 4);
    data << uint32(sound_id);
    if (target)
        target->SendDirectMessage(&data);
    else
        SendMessageToSet(&data, true);
}

void WorldObject::DestroyForNearbyPlayers(uint64 exceptGUID)
{
    if (!IsInWorld())
        return;

    std::list<Player*> targets;
    Trinity::AnyPlayerInObjectRangeCheck check(this, GetMap()->GetVisibilityDistance());
    Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(this, targets, check);
    VisitNearbyWorldObject(GetMap()->GetVisibilityDistance(), searcher);
    for (std::list<Player*>::const_iterator iter = targets.begin(); iter != targets.end(); ++iter)
    {
        Player *plr = (*iter);

        if (plr == this)
            continue;

        if (!plr->HaveAtClient(this))
            continue;

        if (isType(TYPEMASK_UNIT) && ((Unit*)this)->GetCharmerGUID() == plr->GetGUID()) // TODO: this is for puppet
            continue;

        // TODO: Remove this hack after rewriting of stealth/invisibility system !
        if (plr->GetGUID() == exceptGUID)
            continue;

        DestroyForPlayer(plr);
        plr->m_clientGUIDs.erase(GetGUID());
    }
}

void WorldObject::UpdateObjectVisibility(bool /*forced*/)
{
    //updates object's visibility for nearby players
    Trinity::VisibleChangesNotifier notifier(*this);
    VisitNearbyWorldObject(GetMap()->GetVisibilityDistance(), notifier);
}

struct WorldObjectChangeAccumulator
{
    UpdateDataMapType &i_updateDatas;
    WorldObject &i_object;
    std::set<uint64> plr_list;
    WorldObjectChangeAccumulator(WorldObject &obj, UpdateDataMapType &d) : i_updateDatas(d), i_object(obj) {}
    void Visit(PlayerMapType &m)
    {
        for (PlayerMapType::iterator iter = m.begin(); iter != m.end(); ++iter)
        {
            BuildPacket(iter->getSource());
            if (!iter->getSource()->GetSharedVisionList().empty())
            {
                SharedVisionList::const_iterator it = iter->getSource()->GetSharedVisionList().begin();
                for (; it != iter->getSource()->GetSharedVisionList().end(); ++it)
                    BuildPacket(*it);
            }
        }
    }

    void Visit(CreatureMapType &m)
    {
        for (CreatureMapType::iterator iter = m.begin(); iter != m.end(); ++iter)
        {
            if (!iter->getSource()->GetSharedVisionList().empty())
            {
                SharedVisionList::const_iterator it = iter->getSource()->GetSharedVisionList().begin();
                for (; it != iter->getSource()->GetSharedVisionList().end(); ++it)
                    BuildPacket(*it);
            }
        }
    }
    void Visit(DynamicObjectMapType &m)
    {
        for (DynamicObjectMapType::iterator iter = m.begin(); iter != m.end(); ++iter)
        {
            uint64 guid = iter->getSource()->GetCasterGUID();
            if (IS_PLAYER_GUID(guid))
            {
                //Caster may be NULL if DynObj is in removelist
                if (Player *caster = ObjectAccessor::FindPlayer(guid))
                    if (caster->GetUInt64Value(PLAYER_FARSIGHT) == iter->getSource()->GetGUID())
                        BuildPacket(caster);
            }
        }
    }
    void BuildPacket(Player* plr)
    {
        // Only send update once to a player
        if (plr_list.find(plr->GetGUID()) == plr_list.end() && plr->HaveAtClient(&i_object))
        {
            i_object.BuildFieldsUpdate(plr, i_updateDatas);
            plr_list.insert(plr->GetGUID());
        }
    }

    template<class SKIP> void Visit(GridRefManager<SKIP> &) {}
};

void WorldObject::BuildUpdate(UpdateDataMapType& data_map)
{
    CellPair p = Trinity::ComputeCellPair(GetPositionX(), GetPositionY());
    Cell cell(p);
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();
    WorldObjectChangeAccumulator notifier(*this, data_map);
    TypeContainerVisitor<WorldObjectChangeAccumulator, WorldTypeMapContainer > player_notifier(notifier);
    Map& map = *GetMap();
    //we must build packets for all visible players
    cell.Visit(p, player_notifier, map, *this, map.GetVisibilityDistance());

    ClearUpdateMask(false);
}

uint64 WorldObject::GetTransGUID() const
{
    if (m_transport && m_transport->ToGameObject())
        return m_transport->ToGameObject()->GetGUID();
    return 0;
}

void WorldObject::UpdateAllowedPositionZ(float x, float y, float &z) const
{
    // TODO: Allow transports to be part of dynamic vmap tree
    if (GetTransport())
        return;

    switch (GetTypeId())
    {
        case TYPEID_UNIT:
            if (!((Creature const*)this)->CanFly())
            {
                bool canSwim = ((Creature const*)this)->CanSwim();
                float ground_z = z;
                float max_z = canSwim
                    ? GetBaseMap()->GetWaterOrGroundLevel(x, y, z, &ground_z, !((Unit const*)this)->HasAuraType(SPELL_AURA_WATER_WALK))
                    : ((ground_z = GetBaseMap()->GetHeight(GetPhaseMask(), x, y, z+2.0f, true)));

                if (max_z > INVALID_HEIGHT)
                {
                    if (z > max_z)
                        z = max_z;
                    else if (z < ground_z)
                        z = ground_z;
                }
            }
            else
            {
                float ground_z = GetBaseMap()->GetHeight(GetPhaseMask(), x, y, z+2.0f, true);

                if (z < ground_z)
                    z = ground_z;
            }
            break;
        case TYPEID_PLAYER:
            if (!((Player const*)this)->CanFly())
            {
                float ground_z = z;
                float max_z = GetBaseMap()->GetWaterOrGroundLevel(x, y, z, &ground_z, !((Unit const*)this)->HasAuraType(SPELL_AURA_WATER_WALK));

                if (max_z > INVALID_HEIGHT)
                {
                    if (z > max_z)
                        z = max_z;
                    else if (z < ground_z)
                        z = ground_z;
                }
            }
            else
            {
                float ground_z = GetBaseMap()->GetHeight(GetPhaseMask(), x, y, z+2.0f, true);

                if (z < ground_z)
                    z = ground_z;
            }
            break;
        default:
        {
            float ground_z = GetBaseMap()->GetHeight(GetPhaseMask(), x, y, z+2.0f, true);

            if(ground_z > INVALID_HEIGHT)
                z = ground_z;

            break;
        }
    }


}
