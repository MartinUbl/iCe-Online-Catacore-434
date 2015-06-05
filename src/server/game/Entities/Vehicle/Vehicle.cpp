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
#include "Log.h"
#include "ObjectMgr.h"
#include "Vehicle.h"
#include "Unit.h"
#include "Util.h"
#include "WorldPacket.h"
#include "ScriptMgr.h"
#include "CreatureAI.h"
#include "ZoneScript.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"

#include "Transport.h"
#include "DynamicTransport.h"

Transport* TransportBase::ToGenericTransport()
{
    if (m_transportType == TRANSPORT_TYPE_GENERIC)
        return dynamic_cast<Transport*>(this);
    return NULL;
}

DynamicTransport* TransportBase::ToDynamicTransport()
{
    if (m_transportType == TRANSPORT_TYPE_DYNAMIC)
        return dynamic_cast<DynamicTransport*>(this);
    return NULL;
}

Vehicle* TransportBase::ToVehicleTransport()
{
    if (m_transportType == TRANSPORT_TYPE_VEHICLE)
        return dynamic_cast<Vehicle*>(this);
    return NULL;
}

Transport const* TransportBase::ToGenericTransport() const
{
    if (m_transportType == TRANSPORT_TYPE_GENERIC)
        return dynamic_cast<Transport const*>(this);
    return NULL;
}

DynamicTransport const* TransportBase::ToDynamicTransport() const
{
    if (m_transportType == TRANSPORT_TYPE_DYNAMIC)
        return dynamic_cast<DynamicTransport const*>(this);
    return NULL;
}

Vehicle const* TransportBase::ToVehicleTransport() const
{
    if (m_transportType == TRANSPORT_TYPE_VEHICLE)
        return dynamic_cast<Vehicle const*>(this);
    return NULL;
}

WorldObject* TransportBase::ToWorldObject()
{
    return dynamic_cast<WorldObject*>(this);
}

WorldObject const* TransportBase::ToWorldObject() const
{
    return dynamic_cast<WorldObject const*>(this);
}

GameObject* TransportBase::ToGameObject()
{
    if (m_transportType == TRANSPORT_TYPE_GENERIC || m_transportType == TRANSPORT_TYPE_DYNAMIC)
        return dynamic_cast<GameObject*>(this);
    return NULL;
}

GameObject const* TransportBase::ToGameObject() const
{
    if (m_transportType == TRANSPORT_TYPE_GENERIC || m_transportType == TRANSPORT_TYPE_DYNAMIC)
        return dynamic_cast<GameObject const*>(this);
    return NULL;
}

Vehicle::Vehicle(Unit *unit, VehicleEntry const *vehInfo) : TransportBase(TRANSPORT_TYPE_VEHICLE), me(unit), m_vehicleInfo(vehInfo), m_usableSeatNum(0), m_bonusHP(0)
{
    for (uint32 i = 0; i < MAX_VEHICLE_SEATS; ++i)
    {
        if (uint32 seatId = m_vehicleInfo->m_seatID[i])
            if (VehicleSeatEntry const *veSeat = sVehicleSeatStore.LookupEntry(seatId))
            {
                m_Seats.insert(std::make_pair(i, VehicleSeat(veSeat)));
                if (veSeat->IsUsableByPlayer())
                    ++m_usableSeatNum;
            }
    }

    // HACKY WAY, We must found a more generic way to handle this
    // Set inmunities since db ones are rewritten with player's ones
    switch (GetVehicleInfo()->m_ID)
    {
        case 160: // Isle of Conquest Turret
        case 244: // Wintergrasp Turret
            me->SetControlled(true, UNIT_STATE_ROOT);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
        case 158:
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_HEAL, true);
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_FEAR, true);
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_PERIODIC_HEAL, true);
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_STUN, true);
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_ROOT, true);
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_DECREASE_SPEED, true);
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_CONFUSE, true);
            me->ApplySpellImmune(0, IMMUNITY_ID, 49560, true); // Death Grip jump effect
            break;
        default:
            break;
    }
}

Vehicle::~Vehicle()
{
    for (SeatMap::const_iterator itr = m_Seats.begin(); itr != m_Seats.end(); ++itr)
        ASSERT(!itr->second.passenger);
}

void Vehicle::Install()
{
    if (Creature *pCreature = me->ToCreature())
    {
        switch (m_vehicleInfo->m_powerType)
        {
            case POWER_STEAM:
            case POWER_HEAT:
            case POWER_BLOOD:
            case POWER_OOZE:
            case POWER_WRATH:
                me->setPowerType(POWER_ENERGY);
                me->SetMaxPower(POWER_ENERGY, 100);
                break;
            case POWER_PYRITE:
                me->setPowerType(POWER_ENERGY);
                me->SetMaxPower(POWER_ENERGY, 50);
                me->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER);
                break;
            case POWER_FIRE_ENERGY:
                me->setPowerType(POWER_MANA);
                me->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER);
                break;
            case POWER_ORANGE_POWER_2:
                me->setPowerType(POWER_MANA);
                me->SetMaxPower(POWER_MANA, 100);
                break;
            default:
                for (uint32 i = 0; i < MAX_SPELL_VEHICLE; ++i)
                {
                    if (!pCreature->m_spells[i])
                        continue;

                    SpellEntry const *spellInfo = sSpellStore.LookupEntry(pCreature->m_spells[i]);
                    if (!spellInfo)
                        continue;

                    if (spellInfo->powerType == POWER_MANA)
                        break;

                    if (spellInfo->powerType == POWER_ENERGY)
                    {
                        me->setPowerType(POWER_ENERGY);
                        me->SetMaxPower(POWER_ENERGY, 100);
                        break;
                    }
                }
                break;
        }
    }

    Reset();

    if (GetBase()->GetTypeId() == TYPEID_UNIT)
        sScriptMgr->OnInstall(this);
}

void Vehicle::InstallAllAccessories(uint32 entry)
{
    VehicleAccessoryList const* mVehicleList = sObjectMgr->GetVehicleAccessoryList(entry);
    if (!mVehicleList)
        return;

    for (VehicleAccessoryList::const_iterator itr = mVehicleList->begin(); itr != mVehicleList->end(); ++itr)
        InstallAccessory(itr->uiAccessory, itr->uiSeat, itr->bMinion);
}

void Vehicle::Uninstall()
{
    sLog->outDebug("Vehicle::Uninstall %u", me->GetEntry());
    for (SeatMap::iterator itr = m_Seats.begin(); itr != m_Seats.end(); ++itr)
        if (Unit *passenger = itr->second.passenger)
            if (passenger->HasUnitTypeMask(UNIT_MASK_ACCESSORY))
                passenger->ToTempSummon()->UnSummon();

    RemoveAllPassengers();

    if (GetBase()->GetTypeId() == TYPEID_UNIT)
        sScriptMgr->OnUninstall(this);
}

void Vehicle::Die()
{
    sLog->outDebug("Vehicle::Die %u", me->GetEntry());
    for (SeatMap::iterator itr = m_Seats.begin(); itr != m_Seats.end(); ++itr)
        if (Unit *passenger = itr->second.passenger)
            if (passenger->HasUnitTypeMask(UNIT_MASK_ACCESSORY))
                passenger->setDeathState(JUST_DIED);

    RemoveAllPassengers();

    if (GetBase()->GetTypeId() == TYPEID_UNIT)
        sScriptMgr->OnDie(this);
}

void Vehicle::Reset()
{
    sLog->outDebug("Vehicle::Reset");
    if (me->GetTypeId() == TYPEID_PLAYER)
    {
        if (m_usableSeatNum)
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_PLAYER_VEHICLE);
    }
    else
    {
        InstallAllAccessories(me->GetEntry());
        if (m_usableSeatNum)
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
    }

    if (GetBase()->GetTypeId() == TYPEID_UNIT)
        sScriptMgr->OnReset(this);
}

void Vehicle::RemoveAllPassengers()
{
    sLog->outDebug("Vehicle::RemoveAllPassengers");
    for (SeatMap::iterator itr = m_Seats.begin(); itr != m_Seats.end(); ++itr)
        if (Unit *passenger = itr->second.passenger)
        {
            if (passenger->IsVehicle())
                passenger->GetVehicleKit()->RemoveAllPassengers();
            if (passenger->GetVehicle() != this)
                sLog->outCrash("Vehicle %u has invalid passenger %u.", me->GetEntry(), passenger->GetEntry());
            passenger->ExitVehicle();
            if (itr->second.passenger)
            {
                sLog->outCrash("Vehicle %u cannot remove passenger %u. %u is still on vehicle.", me->GetEntry(), passenger->GetEntry(), itr->second.passenger->GetEntry());
                itr->second.passenger = NULL;
            }

            // creature passengers mounted on player mounts should be despawned at dismount
            if (GetBase()->GetTypeId() == TYPEID_PLAYER && passenger->ToCreature())
                passenger->ToCreature()->ForcedDespawn();
        }
}

bool Vehicle::HasEmptySeat(int8 seatId) const
{
    SeatMap::const_iterator seat = m_Seats.find(seatId);
    if (seat == m_Seats.end())
        return false;
    return !seat->second.passenger;
}

Unit *Vehicle::GetPassenger(int8 seatId) const
{
    SeatMap::const_iterator seat = m_Seats.find(seatId);
    if (seat == m_Seats.end())
        return NULL;
    return seat->second.passenger;
}

int8 Vehicle::GetNextEmptySeat(int8 seatId, bool next, bool byAura) const
{
    SeatMap::const_iterator seat = m_Seats.find(seatId);
    if (seat == m_Seats.end())
        return -1;

    while (seat->second.passenger || (!byAura && !seat->second.seatInfo->IsUsableByPlayer()) || (byAura && !seat->second.seatInfo->IsUsableByAura()))
    {
		sLog->outDebug("Vehicle::GetNextEmptySeat: m_flags: %u, m_flagsB:%u", seat->second.seatInfo->m_flags, seat->second.seatInfo->m_flagsB);
        if (next)
        {
            ++seat;
            if (seat == m_Seats.end())
                seat = m_Seats.begin();
        }
        else
        {
            if (seat == m_Seats.begin())
                seat = m_Seats.end();
            --seat;
        }

        if (seat->first == seatId)
            return -1; // no available seat
    }
    return seat->first;
}

void Vehicle::InstallAccessory(uint32 entry, int8 seatId, bool minion)
{
    if (Unit *passenger = GetPassenger(seatId))
    {
        // already installed
        if (passenger->GetEntry() == entry)
        {
            ASSERT(passenger->GetTypeId() == TYPEID_UNIT);
            if (me->GetTypeId() == TYPEID_UNIT && me->ToCreature()->IsInEvadeMode() && passenger->ToCreature()->IsAIEnabled)
                passenger->ToCreature()->AI()->EnterEvadeMode();
            return;
        }
        passenger->ExitVehicle(); // this should not happen
    }

    if (Creature *accessory = me->SummonCreature(entry, *me, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30000))
    {
        if (minion)
            accessory->AddUnitTypeMask(UNIT_MASK_ACCESSORY);

        accessory->EnterVehicle(this, seatId);
        // This is not good, we have to send update twice
        accessory->SendMovementFlagUpdate();

        if (GetBase()->GetTypeId() == TYPEID_UNIT)
            sScriptMgr->OnInstallAccessory(this, accessory);
    }
}

bool Vehicle::AddPassenger(WorldObject *obj, int8 seatId, bool byAura)
{
    Unit* unit = (Unit*)obj;

    if (unit->GetVehicle() != this)
        return false;

    SeatMap::iterator seat;
    if (seatId < 0) // no specific seat requirement
    {
        for (seat = m_Seats.begin(); seat != m_Seats.end(); ++seat)
            if (!seat->second.passenger && (((!byAura && seat->second.seatInfo->IsUsableByPlayer()) || (byAura && seat->second.seatInfo->IsUsableByAura()))))
                break;

        if (seat == m_Seats.end()) // no available seat
            return false;
    }
    else
    {
        seat = m_Seats.find(seatId);
        if (seat == m_Seats.end())
            return false;

        if (seat->second.passenger)
            seat->second.passenger->ExitVehicle();

        ASSERT(!seat->second.passenger);
    }

    sLog->outDebug("Unit %s enter vehicle entry %u id %u dbguid %u seat %d", unit->GetName(), me->GetEntry(), m_vehicleInfo->m_ID, me->GetGUIDLow(), (int32)seat->first);

    seat->second.passenger = unit;
    if (seat->second.seatInfo->IsUsableByPlayer())
    {
        ASSERT(m_usableSeatNum);
        --m_usableSeatNum;
        if (!m_usableSeatNum)
        {
            if (me->GetTypeId() == TYPEID_PLAYER)
                me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_PLAYER_VEHICLE);
            else
                me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        }
    }

    if (seat->second.seatInfo->m_flags && !(seat->second.seatInfo->m_flags & VEHICLE_SEAT_FLAG_ALLOW_TURNING))
        unit->AddUnitState(UNIT_STATE_ON_VEHICLE);

    VehicleSeatEntry const *veSeat = seat->second.seatInfo;
    unit->m_movementInfo.t_pos.m_positionX = veSeat->m_attachmentOffsetX;
    unit->m_movementInfo.t_pos.m_positionY = veSeat->m_attachmentOffsetY;
    unit->m_movementInfo.t_pos.m_positionZ = veSeat->m_attachmentOffsetZ;
    unit->m_movementInfo.t_pos.m_orientation = 0;
    unit->m_movementInfo.t_time = 0; // 1 for player
    unit->m_movementInfo.t_seat = seat->first;
    unit->m_movementInfo.t_guid = me->GetGUID();
    unit->m_movementInfo.t_vehicleId = m_vehicleInfo->m_ID;

    if (me->GetTypeId() == TYPEID_UNIT
        && unit->GetTypeId() == TYPEID_PLAYER
        && seat->first == 0 && seat->second.seatInfo->m_flags & VEHICLE_SEAT_FLAG_CAN_CONTROL)
    {
        if (!me->SetCharmedBy(unit, CHARM_TYPE_VEHICLE))
            ASSERT(false);

        if (VehicleScalingInfo const *scalingInfo = sObjectMgr->GetVehicleScalingInfo(m_vehicleInfo->m_ID))
        {
            Player *plr = unit->ToPlayer();
            float averageItemLevel = plr->GetAverageItemLevel();
            if (averageItemLevel < scalingInfo->baseItemLevel)
                averageItemLevel = scalingInfo->baseItemLevel;
            averageItemLevel -= scalingInfo->baseItemLevel;

            m_bonusHP = uint32(me->GetMaxHealth() * (averageItemLevel * scalingInfo->scalingFactor));
            me->SetMaxHealth(me->GetMaxHealth() + m_bonusHP);
            me->SetHealth(me->GetHealth() + m_bonusHP);
        }
    }

    if (me->IsInWorld())
    {
        Movement::MoveSplineInit init(unit);
        init.DisableTransportPathTransformations();
        init.MoveTo(veSeat->m_attachmentOffsetX, veSeat->m_attachmentOffsetY, veSeat->m_attachmentOffsetZ);
        init.SetFacing(0.0f);
        init.SetTransportEnter();
        init.Launch();

        if (me->GetTypeId() == TYPEID_UNIT)
        {
            if (me->ToCreature()->IsAIEnabled)
                me->ToCreature()->AI()->PassengerBoarded(unit, seat->first, true);

            // update all passenger's positions
            // spline system will relocate passengers automatically
            RelocatePassengers(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
        }

        if (seat->second.seatInfo->m_flags & VEHICLE_SEAT_FLAG_PASSENGER_NOT_SELECTABLE)
            unit->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
    }

    if (obj->GetTypeId() == TYPEID_UNIT)
        unit->DestroyForNearbyPlayers();

    unit->UpdateObjectVisibility(true);

    if (GetBase()->GetTypeId() == TYPEID_UNIT)
        sScriptMgr->OnAddPassenger(this, unit, seatId);

    return true;
}

void Vehicle::RemovePassenger(WorldObject* obj)
{
    Unit *unit = (Unit*)obj;

    if (unit->GetVehicle() != this)
        return;

    SeatMap::iterator seat;
    for (seat = m_Seats.begin(); seat != m_Seats.end(); ++seat)
        if (seat->second.passenger == unit)
            break;

    if (seat == m_Seats.end())
        return;

    sLog->outDebug("Unit %s exit vehicle entry %u id %u dbguid %u seat %d", unit->GetName(), me->GetEntry(), m_vehicleInfo->m_ID, me->GetGUIDLow(), (int32)seat->first);

    seat->second.passenger = NULL;
    if (seat->second.seatInfo->IsUsableByPlayer())
    {
        if (!m_usableSeatNum)
        {
            if (me->GetTypeId() == TYPEID_PLAYER)
                me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_PLAYER_VEHICLE);
            else
                me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        }
        ++m_usableSeatNum;
    }

    if (seat->second.seatInfo->m_flags & VEHICLE_SEAT_FLAG_PASSENGER_NOT_SELECTABLE)
        unit->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

    unit->ClearUnitState(UNIT_STATE_ON_VEHICLE);

    if (me->GetTypeId() == TYPEID_UNIT
        && unit->GetTypeId() == TYPEID_PLAYER
        && seat->first == 0 && seat->second.seatInfo->m_flags & VEHICLE_SEAT_FLAG_CAN_CONTROL)
    {
        me->RemoveCharmedBy(unit);

        if (m_bonusHP)
        {
            me->SetHealth(me->GetHealth() - m_bonusHP);
            me->SetMaxHealth(me->GetMaxHealth() - m_bonusHP);
            m_bonusHP = 0;
        }
    }

    if (me->GetTypeId() == TYPEID_UNIT && me->ToCreature()->IsAIEnabled)
        me->ToCreature()->AI()->PassengerBoarded(unit, seat->first, false);

    // only for flyable vehicles
    if (unit->HasUnitMovementFlag(MOVEMENTFLAG_FLYING))
        me->CastSpell(unit, VEHICLE_SPELL_PARACHUTE, true);
}

void Vehicle::RelocatePassengers(float x, float y, float z, float ang)
{
    Map *map = me->GetMap();
    ASSERT(map != NULL);

    // not sure that absolute position calculation is correct, it must depend on vehicle orientation and pitch angle
    for (SeatMap::const_iterator itr = m_Seats.begin(); itr != m_Seats.end(); ++itr)
    {
        if (Unit *passenger = itr->second.passenger)
        {
            float px, py, pz, po;
            passenger->m_movementInfo.t_pos.GetPosition(px, py, pz, po);
            CalculatePassengerPosition(px, py, pz, &po);

            passenger->SetPosition(px, py, pz, po);
        }
    }
}

void Vehicle::RelocatePassengers()
{
    Map *map = me->GetMap();
    ASSERT(map != NULL);

    // not sure that absolute position calculation is correct, it must depend on vehicle pitch angle
    for (SeatMap::const_iterator itr = m_Seats.begin(); itr != m_Seats.end(); ++itr)
    {
        if (Unit *passenger = itr->second.passenger)
        {
            ASSERT(passenger->IsInWorld());

            float px, py, pz, po;
            passenger->m_movementInfo.t_pos.GetPosition(px, py, pz, po);
            CalculatePassengerPosition(px, py, pz, &po);
            passenger->SetPosition(px, py, pz, po);
        }
    }
}

void Vehicle::Dismiss()
{
    sLog->outDebug("Vehicle::Dismiss %u", me->GetEntry());
    Uninstall();
    me->SendObjectDeSpawnAnim(me->GetGUID());
    me->CombatStop();
    me->AddObjectToRemoveList();
}

void Vehicle::TeleportVehicle(float x, float y, float z, float ang)
{
    vehiclePlayers.clear();
    for(int8 i = 0; i < 8; i++)
        if (Unit* player = GetPassenger(i))
            vehiclePlayers.insert(player->GetGUID());

    RemoveAllPassengers(); // this can unlink Guns from Siege Engines
    me->NearTeleportTo(x, y, z, ang);
    for (GuidSet::const_iterator itr = vehiclePlayers.begin(); itr != vehiclePlayers.end(); ++itr)
        if (Unit* plr = sObjectAccessor->FindUnit(*itr))
            plr->NearTeleportTo(x, y, z, ang);
}

uint16 Vehicle::GetExtraMovementFlagsForBase() const
{
    uint16 movementMask = MOVEMENTFLAG2_NONE;
    uint32 vehicleFlags = GetVehicleInfo()->m_flags;

    if (vehicleFlags & VEHICLE_FLAG_NO_STRAFE)
        movementMask |= MOVEMENTFLAG2_NO_STRAFE;
    if (vehicleFlags & VEHICLE_FLAG_NO_JUMPING)
        movementMask |= MOVEMENTFLAG2_NO_JUMPING;
    if (vehicleFlags & VEHICLE_FLAG_FULLSPEEDTURNING)
        movementMask |= MOVEMENTFLAG2_FULL_SPEED_TURNING;
    if (vehicleFlags & VEHICLE_FLAG_ALLOW_PITCHING)
        movementMask |= MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING;
    if (vehicleFlags & VEHICLE_FLAG_FULLSPEEDPITCHING)
        movementMask |= MOVEMENTFLAG2_FULL_SPEED_PITCHING;

    sLog->outDebug("Vehicle::GetExtraMovementFlagsForBase() returned %u", movementMask);
    return movementMask;
}

void Vehicle::CalculatePassengerPosition(float& x, float& y, float& z, float* o) const
{
    float inx = x, iny = y, inz = z, ino = *o;
    *o = MapManager::NormalizeOrientation(GetBase()->GetOrientation() + ino);
    x = GetBase()->GetPositionX() + inx * std::cos(GetBase()->GetOrientation()) - iny * std::sin(GetBase()->GetOrientation());
    y = GetBase()->GetPositionY() + iny * std::cos(GetBase()->GetOrientation()) + inx * std::sin(GetBase()->GetOrientation());
    z = GetBase()->GetPositionZ() + inz;
}

void Vehicle::CalculatePassengerOffset(float& x, float& y, float& z, float* o) const
{
    *o -= MapManager::NormalizeOrientation(GetBase()->GetOrientation());
    z -= GetBase()->GetPositionZ();
    y -= GetBase()->GetPositionY();    // y = searchedY * std::cos(o) + searchedX * std::sin(o)
    x -= GetBase()->GetPositionX();    // x = searchedX * std::cos(o) + searchedY * std::sin(o + pi)
    float inx = x, iny = y;
    y = (iny - inx * tan(GetBase()->GetOrientation())) / (cos(GetBase()->GetOrientation()) + std::sin(GetBase()->GetOrientation()) * tan(GetBase()->GetOrientation()));
    x = (inx + iny * tan(GetBase()->GetOrientation())) / (cos(GetBase()->GetOrientation()) + std::sin(GetBase()->GetOrientation()) * tan(GetBase()->GetOrientation()));
}
