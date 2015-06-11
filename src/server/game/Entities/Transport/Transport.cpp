/*
* Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
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
#include "Cell.h"
#include "Common.h"
#include "Transport.h"
#include "MapManager.h"
#include "ObjectMgr.h"
#include "Path.h"
#include "ScriptMgr.h"
#include "WorldPacket.h"
#include "DBCStores.h"
#include "World.h"
#include "GameObjectAI.h"
#include "Vehicle.h"
#include "MapReference.h"
#include "Player.h"

Transport::Transport() : GameObject(), TransportBase(TRANSPORT_TYPE_GENERIC),
_transportInfo(NULL), _isMoving(true), _pendingStop(false)
{
    m_updateFlag = (UPDATEFLAG_TRANSPORT | UPDATEFLAG_LOWGUID | UPDATEFLAG_STATIONARY_POSITION | UPDATEFLAG_ROTATION);
}

Transport::~Transport()
{
}

bool Transport::Create(uint32 guidlow, uint32 entry, uint32 mapid, float x, float y, float z, float ang, uint32 animprogress)
{
    Relocate(x, y, z, ang);

    if (!IsPositionValid())
    {
        sLog->outError("Transport (GUID: %u) not created. Suggested coordinates isn't valid (X: %f Y: %f)",
            guidlow, x, y);
        return false;
    }

    Object::_Create(guidlow, 0, HIGHGUID_MO_TRANSPORT);

    GameObjectInfo const* goinfo = sObjectMgr->GetGameObjectInfo(entry);

    if (!goinfo)
    {
        sLog->outError("Transport not created: entry in `gameobject_template` not found, guidlow: %u map: %u  (X: %f Y: %f Z: %f) ang: %f", guidlow, mapid, x, y, z, ang);
        return false;
    }

    m_goInfo = goinfo;

    TransportTemplate const* tInfo = sTransportMgr->GetTransportTemplate(entry);
    if (!tInfo)
    {
        sLog->outError("Transport %u (name: %s) will not be created, missing `transport_template` entry.", entry, goinfo->name);
        return false;
    }

    _transportInfo = tInfo;

    // initialize waypoints
    _nextFrame = tInfo->keyFrames.begin();
    _currentFrame = _nextFrame++;
    _triggeredArrivalEvent = false;
    _triggeredDepartureEvent = false;

    m_goValue->Transport.PathProgress = getMSTime();
    SetFloatValue(OBJECT_FIELD_SCALE_X, goinfo->size);
    SetUInt32Value(GAMEOBJECT_FACTION, goinfo->faction);

    // force transport and nodespawn flags (all mo transports must have them)
    uint32 flags = goinfo->flags;
    flags |= GO_FLAG_TRANSPORT | GO_FLAG_NODESPAWN;

    SetUInt32Value(GAMEOBJECT_FLAGS, goinfo->flags);
    SetPeriod(tInfo->pathTime);
    SetEntry(goinfo->id);
    SetDisplayId(goinfo->displayId);
    SetGoState(GO_STATE_READY);
    _pendingStop = goinfo->moTransport.canBeStopped != 0;
    SetGoType(GAMEOBJECT_TYPE_MO_TRANSPORT);
    SetGoAnimProgress(animprogress);
    SetName(goinfo->name);
    UpdateRotationFields(0.0f, 1.0f);

    m_model = GameObjectModel::Create(*this);

    return true;
}

void Transport::Update(uint32 diff)
{
    uint32 const positionUpdateDelay = 200;

    if (AI())
        AI()->UpdateAI(diff);
    else if (!AIM_Initialize())
        sLog->outError("Could not initialize GameObjectAI for Transport");

    if (GetKeyFrames().size() <= 1)
        return;

    if (IsMoving() || !_pendingStop)
        m_goValue->Transport.PathProgress += diff;

    uint32 timer = m_goValue->Transport.PathProgress % GetPeriod();

    // Set current waypoint
    // Desired outcome: _currentFrame->DepartureTime < timer < _nextFrame->ArriveTime
    // ... arrive | ... delay ... | departure
    //      event /         event /
    for (;;)
    {
        if (timer >= _currentFrame->ArriveTime)
        {
            if (!_triggeredArrivalEvent)
            {
                DoEventIfAny(*_currentFrame, false);
                _triggeredArrivalEvent = true;
            }

            if (timer < _currentFrame->DepartureTime)
            {
                SetMoving(false);
                if (_pendingStop)
                    SetGoState(GO_STATE_READY);
                break;  // its a stop frame and we are waiting
            }
        }

        if (_pendingStop && timer >= _currentFrame->DepartureTime && GetGoState() == GO_STATE_READY)
        {
            m_goValue->Transport.PathProgress = (m_goValue->Transport.PathProgress / GetPeriod());
            m_goValue->Transport.PathProgress *= GetPeriod();
            m_goValue->Transport.PathProgress += _currentFrame->ArriveTime;
            break;
        }

        if (timer >= _currentFrame->DepartureTime && !_triggeredDepartureEvent)
        {
            DoEventIfAny(*_currentFrame, true); // departure event
            _triggeredDepartureEvent = true;
        }

        if (timer >= _currentFrame->DepartureTime && timer < _currentFrame->NextArriveTime)
            break;  // found current waypoint

        MoveToNextWaypoint();

        // not waiting anymore
        SetMoving(true);

        // Enable movement
        if (GetGOInfo()->moTransport.canBeStopped)
            SetGoState(GO_STATE_ACTIVE);

        // Departure event
        if (_currentFrame->IsTeleportFrame())
            TeleportTransport(_nextFrame->Node->mapid, _nextFrame->Node->x, _nextFrame->Node->y, _nextFrame->Node->z);

        sScriptMgr->OnRelocate(this, _currentFrame->Node->index, _currentFrame->Node->mapid, _currentFrame->Node->x, _currentFrame->Node->y, _currentFrame->Node->z);

        sLog->outDebug("Transport %u (%s) moved to node %u %u %f %f %f", GetEntry(), GetName(), _currentFrame->Node->index, _currentFrame->Node->mapid, _currentFrame->Node->x, _currentFrame->Node->y, _currentFrame->Node->z);
    }

    // Set position
    _positionChangeTimer.Update(diff);
    if (_positionChangeTimer.Passed())
    {
        _positionChangeTimer.Reset(positionUpdateDelay);
        if (IsMoving())
        {
            float t = CalculateSegmentPos(float(timer) * 0.001f);
            G3D::Vector3 pos, dir;
            _currentFrame->Spline->evaluate_percent(_currentFrame->Index, t, pos);
            //_currentFrame->Spline->evaluate_derivative(_currentFrame->Index, t, dir);
            UpdatePosition(pos.x, pos.y, pos.z, 0.0f/*atan2(dir.x, dir.y)*/);
        }
    }

    sScriptMgr->OnTransportUpdate(this, diff);
}

bool Transport::AddPassenger(WorldObject* passenger, int8 seatId, bool byAura)
{
    if (_passengers.insert(passenger).second)
    {
        passenger->SetTransport(this);
        passenger->m_movementInfo.t_guid = GetGUID();
        sLog->outDetail("Object %s boarded transport %s.", passenger->GetName(), GetName());
    }

    if (Player* plr = passenger->ToPlayer())
        sScriptMgr->OnAddPassenger(this, plr);

    return true;
}

void Transport::RemovePassenger(WorldObject* passenger)
{
    if (_passengers.erase(passenger) || _staticPassengers.erase(passenger)) // static passenger can remove itself in case of grid unload
        sLog->outDetail("Object %s removed from transport %s.", passenger->GetName(), GetName());


    if (Player* plr = passenger->ToPlayer())
        sScriptMgr->OnRemovePassenger(this, plr);
}

Creature* Transport::CreateNPCPassenger(uint32 guid, CreatureData const* data)
{
    Map* map = GetMap();
    Creature* creature = new Creature();

    if (!creature->LoadFromDB(guid, map))
    {
        delete creature;
        return NULL;
    }

    float x = data->posX;
    float y = data->posY;
    float z = data->posZ;
    float o = data->orientation;

    creature->SetTransport(this);
    creature->AddUnitMovementFlag(MOVEMENTFLAG_LEVITATING);
    creature->m_movementInfo.t_guid = GetGUID();
    creature->m_movementInfo.t_pos.Relocate(x, y, z, o);
    CalculatePassengerPosition(x, y, z, &o);
    creature->Relocate(x, y, z, o);

    creature->SetHomePosition(creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ(), creature->GetOrientation());
    creature->SetTransportHomePosition(creature->m_movementInfo.t_pos);

    creature->AddUnitState(UNIT_STATE_IGNORE_PATHFINDING);

    if (!creature->IsPositionValid())
    {
        sLog->outError("Creature (guidlow %d, entry %d) not created. Suggested coordinates aren't valid (X: %f Y: %f)", creature->GetGUIDLow(), creature->GetEntry(), creature->GetPositionX(), creature->GetPositionY());
        delete creature;
        return NULL;
    }

    map->Add(creature);
    _staticPassengers.insert(creature);

    sScriptMgr->OnAddCreaturePassenger(this, creature);
    return creature;
}

Creature* Transport::SpawnNPCPassenger(uint32 entry, float x, float y, float z, float o, uint32 emoteId)
{
    Map* map = GetMap();

    CreatureInfo const *ci = sObjectMgr->GetCreatureTemplate(entry);

    if (!ci)
        return 0;

    Creature* pCreature = new Creature();

    if (!pCreature->Create(sObjectMgr->GenerateLowGuidForUnit(true), map, GetPhaseMask(), entry, 0, GetGOInfo()->faction, 0, 0, 0, 0))
    {
        delete pCreature;
        return NULL;
    }

    pCreature->SetTransport(this);
    pCreature->AddUnitMovementFlag(MOVEMENTFLAG_LEVITATING);
    pCreature->m_movementInfo.t_guid = GetGUID();
    pCreature->m_movementInfo.t_pos.Relocate(x, y, z, o);
    CalculatePassengerPosition(x, y, z, &o);
    pCreature->Relocate(x, y, z, o);

    pCreature->SetHomePosition(pCreature->GetPositionX(), pCreature->GetPositionY(), pCreature->GetPositionZ(), pCreature->GetOrientation());
    pCreature->SetTransportHomePosition(pCreature->m_movementInfo.t_pos);

    pCreature->AddUnitState(UNIT_STATE_IGNORE_PATHFINDING);

    if (emoteId)
        pCreature->SetUInt32Value(UNIT_NPC_EMOTESTATE, emoteId);

    if (!pCreature->IsPositionValid())
    {
        sLog->outError("Creature (guidlow %d, entry %d) not spawned. Suggested coordinates aren't valid (X: %f Y: %f)", pCreature->GetGUIDLow(), pCreature->GetEntry(), pCreature->GetPositionX(), pCreature->GetPositionY());
        delete pCreature;
        return NULL;
    }

    map->Add(pCreature);
    _passengers.insert(pCreature);

    sScriptMgr->OnAddCreaturePassenger(this, pCreature);

    return pCreature;
}

GameObject* Transport::CreateGOPassenger(uint32 guid, GameObjectData const* data)
{
    Map* map = GetMap();
    GameObject* go = new GameObject();

    if (!go->LoadFromDB(guid, map))
    {
        delete go;
        return NULL;
    }

    float x = data->posX;
    float y = data->posY;
    float z = data->posZ;
    float o = data->orientation;

    go->SetTransport(this);
    go->m_movementInfo.t_guid = GetGUID();
    go->m_movementInfo.t_pos.Relocate(x, y, z, o);
    CalculatePassengerPosition(x, y, z, &o);
    go->Relocate(x, y, z, o);

    if (!go->IsPositionValid())
    {
        sLog->outError("GameObject (guidlow %d, entry %d) not created. Suggested coordinates aren't valid (X: %f Y: %f)", go->GetGUIDLow(), go->GetEntry(), go->GetPositionX(), go->GetPositionY());
        delete go;
        return NULL;
    }

    map->Add(go);
    _staticPassengers.insert(go);

    //sScriptMgr->OnAddCreaturePassenger(this, go);
    return go;
}

void Transport::CalculatePassengerPosition(float& x, float& y, float& z, float* o /*= NULL*/) const
{
    float inx = x, iny = y, inz = z;
    if (o)
        *o = Position::NormalizeOrientation(GetOrientation() + *o);

    x = GetPositionX() + inx * std::cos(GetOrientation()) - iny * std::sin(GetOrientation());
    y = GetPositionY() + iny * std::cos(GetOrientation()) + inx * std::sin(GetOrientation());
    z = GetPositionZ() + inz;
}

void Transport::CalculatePassengerOffset(float& x, float& y, float& z, float* o /*= NULL*/) const
{
    if (o)
        *o = Position::NormalizeOrientation(*o - GetOrientation());

    z -= GetPositionZ();
    y -= GetPositionY();    // y = searchedY * std::cos(o) + searchedX * std::sin(o)
    x -= GetPositionX();    // x = searchedX * std::cos(o) + searchedY * std::sin(o + pi)
    float inx = x, iny = y;
    y = (iny - inx * std::tan(GetOrientation())) / (std::cos(GetOrientation()) + std::sin(GetOrientation()) * std::tan(GetOrientation()));
    x = (inx + iny * std::tan(GetOrientation())) / (std::cos(GetOrientation()) + std::sin(GetOrientation()) * std::tan(GetOrientation()));
}

void Transport::UpdatePosition(float x, float y, float z, float o)
{
    bool newActive = GetMap()->IsLoaded(x, y);

    float posX, posY;

    posX = GetPositionX();
    posY = GetPositionY();

    if (Cell(x, y).DiffGrid(Cell(posX, posY)))
        UpdateModelPosition(x, y, z, true);
    else
        UpdateModelPosition(x, y, z, false);

    Relocate(x, y, z, o);

    UpdatePassengerPositions(_passengers);

    /* There are four possible scenarios that trigger loading/unloading passengers:
    1. transport moves from inactive to active grid
    2. the grid that transport is currently in becomes active
    3. transport moves from active to inactive grid
    4. the grid that transport is currently in unloads
    */
    if (_staticPassengers.empty() && newActive) // 1. and 2.
        LoadStaticPassengers();
    else if (!_staticPassengers.empty() && !newActive && Cell(x, y).DiffGrid(Cell(posX, posY))) // 3.
        UnloadStaticPassengers();
    else
        UpdatePassengerPositions(_staticPassengers);
    // 4. is handed by grid unload
}

void Transport::LoadStaticPassengers()
{
    if (uint32 mapId = GetGOInfo()->moTransport.mapID)
    {
        CellObjectGuidsMap const& cells = sObjectMgr->GetMapObjectGuids(mapId, GetMap()->GetSpawnMode());
        CellGuidSet::const_iterator guidEnd;
        for (CellObjectGuidsMap::const_iterator cellItr = cells.begin(); cellItr != cells.end(); ++cellItr)
        {
            // Creatures on transport
            guidEnd = cellItr->second.creatures.end();
            for (CellGuidSet::const_iterator guidItr = cellItr->second.creatures.begin(); guidItr != guidEnd; ++guidItr)
                CreateNPCPassenger(*guidItr, sObjectMgr->GetCreatureData(*guidItr));

            // GameObjects on transport
            guidEnd = cellItr->second.gameobjects.end();
            for (CellGuidSet::const_iterator guidItr = cellItr->second.gameobjects.begin(); guidItr != guidEnd; ++guidItr)
                CreateGOPassenger(*guidItr, sObjectMgr->GetGOData(*guidItr));
        }
    }
}

void Transport::UnloadStaticPassengers()
{
    while (!_staticPassengers.empty())
    {
        WorldObject* obj = *_staticPassengers.begin();
        obj->AddObjectToRemoveList();   // also removes from _staticPassengers
    }
}

void Transport::EnableMovement(bool enabled)
{
    if (!GetGOInfo()->moTransport.canBeStopped)
        return;

    _pendingStop = !enabled;
}

void Transport::MoveToNextWaypoint()
{
    // Clear events flagging
    _triggeredArrivalEvent = false;
    _triggeredDepartureEvent = false;

    // Set frames
    _currentFrame = _nextFrame++;
    if (_nextFrame == GetKeyFrames().end())
        _nextFrame = GetKeyFrames().begin();
}

float Transport::CalculateSegmentPos(float now)
{
    KeyFrame const& frame = *_currentFrame;
    const float speed = float(m_goInfo->moTransport.moveSpeed);
    const float accel = float(m_goInfo->moTransport.accelRate);
    float timeSinceStop = frame.TimeFrom + (now - (1.0f / IN_MILLISECONDS) * frame.DepartureTime);
    float timeUntilStop = frame.TimeTo - (now - (1.0f / IN_MILLISECONDS) * frame.DepartureTime);
    float segmentPos, dist;
    float accelTime = _transportInfo->accelTime;
    float accelDist = _transportInfo->accelDist;
    // calculate from nearest stop, less confusing calculation...
    if (timeSinceStop < timeUntilStop)
    {
        if (timeSinceStop < accelTime)
            dist = 0.5f * accel * timeSinceStop * timeSinceStop;
        else
            dist = accelDist + (timeSinceStop - accelTime) * speed;
        segmentPos = dist - frame.DistSinceStop;
    }
    else
    {
        if (timeUntilStop < _transportInfo->accelTime)
            dist = 0.5f * accel * timeUntilStop * timeUntilStop;
        else
            dist = accelDist + (timeUntilStop - accelTime) * speed;
        segmentPos = frame.DistUntilStop - dist;
    }

    return segmentPos / frame.NextDistFromPrev;
}

void Transport::TeleportTransport(uint32 newMapid, float x, float y, float z)
{
    Map const* oldMap = GetMap();

    if (oldMap->GetId() != newMapid)
    {
        Map* newMap = (Map*)(sMapMgr->CreateBaseMap(newMapid));
        Map::PlayerList const& oldPlayers = GetMap()->GetPlayers();
        if (!oldPlayers.isEmpty())
        {
            UpdateData data(oldMap->GetId());
            BuildOutOfRangeUpdateBlock(&data);
            WorldPacket packet;
            data.BuildPacket(&packet);
            for (Map::PlayerList::const_iterator itr = oldPlayers.begin(); itr != oldPlayers.end(); ++itr)
                if (itr->getSource()->GetTransport() != this)
                    itr->getSource()->SendDirectMessage(&packet);
        }

        UnloadStaticPassengers();

        EnableCollision(false);
        RemoveModelFromMap();
        GetMap()->Remove<Transport>(this, false);
        SetMap(newMap);

        Map::PlayerList const& newPlayers = GetMap()->GetPlayers();
        if (!newPlayers.isEmpty())
        {
            for (Map::PlayerList::const_iterator itr = newPlayers.begin(); itr != newPlayers.end(); ++itr)
            {
                if (itr->getSource()->GetTransport() != this)
                {
                    UpdateData data(newMapid);
                    BuildCreateUpdateBlockForPlayer(&data, itr->getSource());
                    WorldPacket packet;
                    data.BuildPacket(&packet);
                    itr->getSource()->SendDirectMessage(&packet);
                }
            }
        }

        // Teleport passengers after everyone on destination map are sent create packet
        // but before transport itself is registered there and begins updating
        for (std::set<WorldObject*>::iterator itr = _staticPassengers.begin(); itr != _staticPassengers.end(); ++itr)
        {
            switch ((*itr)->GetTypeId())
            {
                case TYPEID_UNIT:
                {
                    (*itr)->ToCreature()->FarTeleportTo(newMap, x, y, z, (*itr)->GetOrientation());
                    break;
                }
                case TYPEID_GAMEOBJECT:
                {
                    GameObject* go = (*itr)->ToGameObject();
                    go->GetMap()->Remove(go, false);
                    Relocate(x, y, z, go->GetOrientation());
                    SetMap(newMap);
                    newMap->Add(go);
                    break;
                }
                default:
                    break;
            }
        }

        for (std::set<WorldObject*>::iterator itr = _passengers.begin(); itr != _passengers.end(); ++itr)
        {
            switch ((*itr)->GetTypeId())
            {
                case TYPEID_UNIT:
                {
                    if (!IS_PLAYER_GUID((*itr)->ToUnit()->GetOwnerGUID()))  // pets should be teleported with player
                        (*itr)->ToCreature()->FarTeleportTo(newMap, x, y, z, (*itr)->GetOrientation());
                    break;
                }
                case TYPEID_GAMEOBJECT:
                {
                    GameObject* go = (*itr)->ToGameObject();
                    go->GetMap()->Remove(go, false);
                    Relocate(x, y, z, go->GetOrientation());
                    SetMap(newMap);
                    newMap->Add(go);
                    break;
                }
                case TYPEID_PLAYER:
                {
                    (*itr)->ToPlayer()->TeleportTo(newMapid, x, y, z, (*itr)->GetOrientation(), TELE_TO_NOT_LEAVE_TRANSPORT);
                    break;
                }
                default:
                    break;
            }
        }

        GetMap()->Add<Transport>(this);
        AddModelToMap();
        EnableCollision(true);
    }
    else
    {
        // Teleport players, they need to know it
        for (std::set<WorldObject*>::iterator itr = _passengers.begin(); itr != _passengers.end(); ++itr)
            if ((*itr)->GetTypeId() == TYPEID_PLAYER)
                (*itr)->ToUnit()->NearTeleportTo(x, y, z, GetOrientation());
    }

    UpdatePosition(x, y, z, GetOrientation());
}

void Transport::UpdatePassengerPositions(std::set<WorldObject*>& passengers)
{
    for (std::set<WorldObject*>::iterator itr = passengers.begin(); itr != passengers.end(); ++itr)
    {
        WorldObject* passenger = *itr;
        // transport teleported but passenger not yet (can happen for players)
        if (passenger->GetMap() != GetMap())
            continue;

        // if passenger is on vehicle we have to assume the vehicle is also on transport
        // and its the vehicle that will be updating its passengers
        if (Unit* unit = passenger->ToUnit())
            if (unit->GetVehicle())
                continue;

        // Do not use Unit::UpdatePosition here, we don't want to remove auras
        // as if regular movement occurred
        float x, y, z, o;
        passenger->m_movementInfo.t_pos.GetPosition(x, y, z, o);
        CalculatePassengerPosition(x, y, z, &o);
        switch (passenger->GetTypeId())
        {
            case TYPEID_UNIT:
            {
                Creature* creature = passenger->ToCreature();
                GetMap()->CreatureRelocation(creature, x, y, z, o, false);
                creature->GetTransportHomePosition(x, y, z, o);
                CalculatePassengerPosition(x, y, z, &o);
                creature->SetHomePosition(x, y, z, o);
                break;
            }
            case TYPEID_PLAYER:
            {
                GetMap()->PlayerRelocation(passenger->ToPlayer(), x, y, z, o);
                break;
            }
            case TYPEID_GAMEOBJECT:
            {
                GetMap()->GameObjectRelocation(passenger->ToGameObject(), x, y, z, o, false);
                break;
            }
            default:
                break;
        }

        if (Unit* unit = passenger->ToUnit())
            if (Vehicle* vehicle = unit->GetVehicleKit())
                vehicle->RelocatePassengers();
    }
}

void Transport::DoEventIfAny(KeyFrame const& node, bool departure)
{
    if (uint32 eventid = departure ? node.Node->departureEventID : node.Node->arrivalEventID)
    {
        sLog->outDebug("Taxi %s event %u of node %u of %s path", departure ? "departure" : "arrival", eventid, node.Node->index, GetName());
        GetMap()->ScriptsStart(sEventScripts, eventid, this, this);
        EventInform(eventid);
    }
}

void Transport::BuildUpdate(UpdateDataMapType& data_map)
{
    Map::PlayerList const& players = GetMap()->GetPlayers();
    if (players.isEmpty())
        return;

    for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        BuildFieldsUpdate(itr->getSource(), data_map);

    ClearUpdateMask(true);
}
