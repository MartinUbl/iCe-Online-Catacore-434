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
#include "DynamicTransport.h"

DynamicTransport::DynamicTransport() : GameObject(), TransportBase(TRANSPORT_TYPE_DYNAMIC)
{
    //
}

DynamicTransport::~DynamicTransport()
{
    //
}

bool DynamicTransport::Create(uint32 guidlow, uint32 name_id, Map *map, uint32 phaseMask, float x, float y, float z, float ang, float rotation0, float rotation1, float rotation2, float rotation3, uint32 animprogress, GOState go_state, uint32 artKit)
{
    if (!GameObject::Create(guidlow, name_id, map, phaseMask, x, y, z, ang, rotation0, rotation1, rotation2, rotation3, animprogress, go_state, artKit))
        return false;

    // goinfo is always existant if parent Create method returned true
    GameObjectInfo const* goinfo = sObjectMgr->GetGameObjectInfo(name_id);

    m_updateFlag |= UPDATEFLAG_TRANSPORT;

    // insert first stopframe, if needed
    if (goinfo->transport.stopFrame1 > 0)
    {
        m_goValue->Transport.StopFrames->push_back(goinfo->transport.stopFrame1);
        m_goValue->Transport.MaxStopFrameTime = goinfo->transport.stopFrame1;
    }

    // go through other stopframe fields, they are defined like this
    // - second stopframe is at position 6, and each next stopframe is +2 from current, up to 20 (including)
    for (uint32 i = 6; i <= 20; i += 2)
    {
        if (goinfo->raw.data[i] > 0)
        {
            // stopframes has to be in ascending order, otherwise the client would not handle it
            if (goinfo->raw.data[i] <= m_goValue->Transport.MaxStopFrameTime)
            {
                sLog->outError("DynamicTransport::Create: error when creating dynamic transport object (entry %u) - stopframes are not in ascending order!", goinfo->id);
                break;
            }

            m_goValue->Transport.StopFrames->push_back(goinfo->raw.data[i]);
            m_goValue->Transport.MaxStopFrameTime = goinfo->raw.data[i];
        }
        else
            break;
    }

    // the nullth frame should always be here
    if (!m_goValue->Transport.StopFrames->empty())
    {
        bool nullPresent = false;
        for (uint32 i = 0; i < m_goValue->Transport.StopFrames->size(); i++)
        {
            if (m_goValue->Transport.StopFrames->at(i) == 0)
            {
                nullPresent = true;
                break;
            }
        }

        if (!nullPresent)
            m_goValue->Transport.StopFrames->insert(m_goValue->Transport.StopFrames->begin(), 0);
    }

    // set proper transport state only when dealing with non-custom transports
    if (m_goValue->Transport.AnimationInfo)
    {
        if (goinfo->transport.startOpen)
        {
            if (goinfo->transport.startOpen > 0 && goinfo->transport.startOpen <= m_goValue->Transport.StopFrames->size())
                SetTransportState(GO_STATE_TRANSPORT_STOPPED, goinfo->transport.startOpen - 1);
            else
                SetTransportState(GO_STATE_TRANSPORT_STOPPED, 0);
        }
        else
            SetTransportState(GO_STATE_TRANSPORT_ACTIVE);
    }

    SetGoAnimProgress(animprogress);

    // we have to set those objects as active, to always update them
    // this may be needed only for objects, which are changing position across grids/cells
    // but for now, enable it for all dynamic transports
    setActive(true);

    return true;
}

void DynamicTransport::Update(uint32 p_time)
{
    // at fisrt, call parent method
    GameObject::Update(p_time);

    // if this object does not have any animation info, we don't care
    if (!m_goValue->Transport.AnimationInfo)
        return;

    // path progress is always 1:1 with some 100% accurate and synchronnous number
    // getMSTime provides such source, and is perfect for us
    m_goValue->Transport.PathProgress = getMSTime();

    // if the transport is active, make stopframes rotate by script
    if (GetGoState() == GO_STATE_TRANSPORT_ACTIVE)
    {
        // this is interesting only if some stopframes are present
        if (!m_goValue->Transport.StopFrames->empty())
        {
            // this causes active transport to change stop frame every DYNAMIC_TRANSPORT_ACTIVE_TIMESPAN milliseconds
            uint32 visualStateAfter = (m_goValue->Transport.PathProgress / DYNAMIC_TRANSPORT_ACTIVE_TIMESPAN) % m_goValue->Transport.StopFrames->size();

            // if it's time to change frame
            if (m_goValue->Transport.VisualState != visualStateAfter)
            {
                // calculate how long will it take, save progress start time and set visual state
                m_goValue->Transport.StateChangeTime = abs((int32)m_goValue->Transport.StopFrames->at(visualStateAfter) - (int32)m_goValue->Transport.StopFrames->at(m_goValue->Transport.VisualState));
                m_goValue->Transport.StateChangeStartProgress = m_goValue->Transport.PathProgress;
                ResetPositionTimer();
                m_goValue->Transport.OldVisualState = m_goValue->Transport.VisualState;
                m_goValue->Transport.VisualState = visualStateAfter;

                // this forces client update
                ForceValuesUpdateAtIndex(GAMEOBJECT_LEVEL);
                ForceValuesUpdateAtIndex(GAMEOBJECT_BYTES_1);
            }
        }
    }

    // if the transport is moving (not reached destination point yet), then we should move it by animation frames defined in DBC
    if (m_goValue->Transport.StateChangeTime && (getMSTimeDiff(m_goValue->Transport.StateChangeStartProgress, m_goValue->Transport.PathProgress) < m_goValue->Transport.StateChangeTime ||
        m_goValue->Transport.OldVisualState != m_goValue->Transport.VisualState))
    {
        bool movingToNext = m_goValue->Transport.OldVisualState < m_goValue->Transport.VisualState;

        float progress = ((float)getMSTimeDiff(m_goValue->Transport.StateChangeStartProgress, m_goValue->Transport.PathProgress)) / ((float)(m_goValue->Transport.StateChangeTime));
        if (progress > 1.0f)
            progress = 1.0f;

        // this evaluates time segment the transport is currently in, and retrieve specific node
        uint32 timer = m_goValue->Transport.StopFrames->at(m_goValue->Transport.OldVisualState) +
            progress * ((float)m_goValue->Transport.StopFrames->at(m_goValue->Transport.VisualState) - (float)m_goValue->Transport.StopFrames->at(m_goValue->Transport.OldVisualState));

        TransportAnimationEntry const* node = m_goValue->Transport.AnimationInfo->GetAnimNode(timer, movingToNext);

        // if we moved to next node
        if (node)
        {
            // reached next node
            if (m_goValue->Transport.CurrentSeg != node->TimeSeg)
            {
                m_goValue->Transport.CurrentSeg = node->TimeSeg;

                // evaluate position
                // note we are not including GO orientation in those fields - we should not, the moving process is independent
                G3D::Quat rotation = m_goValue->Transport.AnimationInfo->GetAnimRotation(timer, movingToNext);
                G3D::Vector3 pos = rotation.toRotationMatrix() * G3D::Vector3(-node->X, -node->Y, node->Z);
                // also note the negative coordinates here - Blizzard decided to save "return offset" which would, by subtraction, help us retrieve
                // the original position. But since we use different model (stationary position plus offset instead of dynamic position minus offset),
                // we have to consider it negative. This applies only to X and Y coordinates, not the Z one

                pos += G3D::Vector3(m_stationaryPosition.GetPositionX(), m_stationaryPosition.GetPositionY(), m_stationaryPosition.GetPositionZ());

                // this will cause also grid/cell relocation if needed
                GetMap()->GameObjectRelocation(this, pos.x, pos.y, pos.z, GetOrientation());

                UpdatePosition(pos.x, pos.y, pos.z, GetOrientation());
            }
            else // not reached next node, interpolate movement between current and next node
            {
                if (positionInterpolateTimer <= p_time)
                {
                    TransportAnimationEntry const* nextnode = m_goValue->Transport.AnimationInfo->GetAnimNode(m_goValue->Transport.CurrentSeg + (movingToNext ? 1 : -1), !movingToNext);

                    G3D::Quat oldrotation = m_goValue->Transport.AnimationInfo->GetAnimRotation(timer, movingToNext);
                    G3D::Vector3 oldpos = oldrotation.toRotationMatrix() * G3D::Vector3(-node->X, -node->Y, node->Z);

                    oldpos += G3D::Vector3(m_stationaryPosition.GetPositionX(), m_stationaryPosition.GetPositionY(), m_stationaryPosition.GetPositionZ());

                    G3D::Quat newrotation = m_goValue->Transport.AnimationInfo->GetAnimRotation(m_goValue->Transport.CurrentSeg + (movingToNext ? 1 : -1), !movingToNext);
                    G3D::Vector3 newpos = newrotation.toRotationMatrix() * G3D::Vector3(-nextnode->X, -nextnode->Y, nextnode->Z);

                    newpos += G3D::Vector3(m_stationaryPosition.GetPositionX(), m_stationaryPosition.GetPositionY(), m_stationaryPosition.GetPositionZ());

                    // use linear interpolation between points, since we can afford it - client does the same

                    G3D::Vector3 interpolated(oldpos.x + (newpos.x - oldpos.x)*progress, oldpos.y + (newpos.y - oldpos.y)*progress, oldpos.z + (newpos.z - oldpos.z)*progress);

                    // this will cause also grid/cell relocation if needed
                    GetMap()->GameObjectRelocation(this, interpolated.x, interpolated.y, interpolated.z, GetOrientation());

                    UpdatePosition(interpolated.x, interpolated.y, interpolated.z, GetOrientation());

                    ResetPositionTimer();
                }
                else
                    positionInterpolateTimer -= p_time;
            }
        }

        // if we reached destination node, set old state to current
        if (getMSTimeDiff(m_goValue->Transport.StateChangeStartProgress, getMSTime()) >= m_goValue->Transport.StateChangeTime)
        {
            m_goValue->Transport.OldVisualState = m_goValue->Transport.VisualState;

            // TODO: maybe some "reached node" event hook?
        }
    }
}

void DynamicTransport::ResetPositionTimer()
{
    positionInterpolateTimer = DYNAMIC_TRANSPORT_INTERPOLATE_TIMER;
}

bool DynamicTransport::AddPassenger(WorldObject* passenger, int8 seatId, bool byAura)
{
    if (_passengers.insert(passenger).second)
    {
        passenger->SetTransport(this);
        passenger->m_movementInfo.t_guid = GetGUID();
    }

    return true;
}

void DynamicTransport::RemovePassenger(WorldObject* passenger)
{
    _passengers.erase(passenger);
}

void DynamicTransport::CalculatePassengerPosition(float& x, float& y, float& z, float* o /*= NULL*/) const
{
    float inx = x, iny = y, inz = z;
    if (o)
        *o = Position::NormalizeOrientation(GetOrientation() + *o);

    x = GetPositionX() + inx * std::cos(GetOrientation()) - iny * std::sin(GetOrientation());
    y = GetPositionY() + iny * std::cos(GetOrientation()) + inx * std::sin(GetOrientation());
    z = GetPositionZ() + inz;
}

void DynamicTransport::CalculatePassengerOffset(float& x, float& y, float& z, float* o /*= NULL*/) const
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

void DynamicTransport::UpdatePosition(float x, float y, float z, float o)
{
    // This will activate the next cell if not activated yet
    GetMap()->IsLoaded(x, y);

    Relocate(x, y, z, o);

    UpdatePassengerPositions(_passengers);

    // we force sending some other fields in BGs, since there are the only occurency, where we definitelly need to have
    // everything synced across whole map, which would be brutal overhead in outside world
    if (GetMap()->IsBattleground())
    {
        ForceValuesUpdateAtIndex(GAMEOBJECT_DYNAMIC);
        ForceValuesUpdateAtIndex(GAMEOBJECT_LEVEL);
    }
}

void DynamicTransport::ForcePositionUpdate()
{
    // if the transport is not moving, no need to recalculate - it's already been done in Update function
    if (!m_goValue->Transport.StateChangeTime ||
        (getMSTimeDiff(m_goValue->Transport.StateChangeStartProgress, m_goValue->Transport.PathProgress) >= m_goValue->Transport.StateChangeTime &&
        m_goValue->Transport.OldVisualState == m_goValue->Transport.VisualState))
        return;

    bool movingToNext = m_goValue->Transport.OldVisualState < m_goValue->Transport.VisualState;

    float progress = ((float)getMSTimeDiff(m_goValue->Transport.StateChangeStartProgress, m_goValue->Transport.PathProgress)) / ((float)(m_goValue->Transport.StateChangeTime));
    if (progress > 1.0f)
        progress = 1.0f;

    // this evaluates time segment the transport is currently in, and retrieve specific node
    uint32 timer = m_goValue->Transport.StopFrames->at(m_goValue->Transport.OldVisualState) +
        progress * ((float)m_goValue->Transport.StopFrames->at(m_goValue->Transport.VisualState) - (float)m_goValue->Transport.StopFrames->at(m_goValue->Transport.OldVisualState));

    TransportAnimationEntry const* node = m_goValue->Transport.AnimationInfo->GetAnimNode(timer, movingToNext);
    TransportAnimationEntry const* nextnode = m_goValue->Transport.AnimationInfo->GetAnimNode(m_goValue->Transport.CurrentSeg + (movingToNext ? 1 : -1), !movingToNext);

    G3D::Quat oldrotation = m_goValue->Transport.AnimationInfo->GetAnimRotation(timer, movingToNext);
    G3D::Vector3 oldpos = oldrotation.toRotationMatrix() * G3D::Vector3(-node->X, -node->Y, node->Z);

    oldpos += G3D::Vector3(m_stationaryPosition.GetPositionX(), m_stationaryPosition.GetPositionY(), m_stationaryPosition.GetPositionZ());

    G3D::Quat newrotation = m_goValue->Transport.AnimationInfo->GetAnimRotation(m_goValue->Transport.CurrentSeg + (movingToNext ? 1 : -1), !movingToNext);
    G3D::Vector3 newpos = newrotation.toRotationMatrix() * G3D::Vector3(-nextnode->X, -nextnode->Y, nextnode->Z);

    newpos += G3D::Vector3(m_stationaryPosition.GetPositionX(), m_stationaryPosition.GetPositionY(), m_stationaryPosition.GetPositionZ());

    // use linear interpolation between points, since we can afford it - client does the same

    G3D::Vector3 interpolated(oldpos.x + (newpos.x - oldpos.x)*progress, oldpos.y + (newpos.y - oldpos.y)*progress, oldpos.z + (newpos.z - oldpos.z)*progress);

    // this will cause also grid/cell relocation if needed
    GetMap()->GameObjectRelocation(this, interpolated.x, interpolated.y, interpolated.z, GetOrientation());

    UpdatePosition(interpolated.x, interpolated.y, interpolated.z, GetOrientation());

    ResetPositionTimer();
}

void DynamicTransport::UpdateSinglePassengerPosition(WorldObject* passenger)
{
    // if passenger is on vehicle we have to assume the vehicle is also on transport
    // and its the vehicle that will be updating its passengers
    if (Unit* unit = passenger->ToUnit())
        if (unit->GetVehicle())
            return;

    // Do not use Unit::UpdatePosition here, we don't want to remove auras
    // as if regular movement occurred
    float x, y, z, o;
    passenger->m_movementInfo.t_pos.GetPosition(x, y, z, o);
    CalculatePassengerPosition(x, y, z, &o);
    // for now we accept only player passengers, the other cases are there for future use
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

void DynamicTransport::UpdatePassengerPositions(std::set<WorldObject*>& passengers)
{
    for (std::set<WorldObject*>::iterator itr = passengers.begin(); itr != passengers.end(); ++itr)
    {
        WorldObject* passenger = *itr;
        UpdateSinglePassengerPosition(passenger);
    }
}

