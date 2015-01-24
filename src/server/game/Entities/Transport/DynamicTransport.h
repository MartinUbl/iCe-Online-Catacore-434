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

#ifndef DYNAMIC_TRANSPORTS_H
#define DYNAMIC_TRANSPORTS_H

#include "GameObject.h"
#include "TransportMgr.h"
#include "Transport.h"
#include "Vehicle.h"

// how often an active dynamic transport changes states - for now 20 seconds, seems to be right
#define DYNAMIC_TRANSPORT_ACTIVE_TIMESPAN 20000
// how often do we recalculate passenger positions between frames
#define DYNAMIC_TRANSPORT_INTERPOLATE_TIMER 500

class DynamicTransport : public GameObject, public TransportBase
{
    public:
        explicit DynamicTransport();
        ~DynamicTransport();

        bool Create(uint32 guidlow, uint32 name_id, Map *map, uint32 phaseMask, float x, float y, float z, float ang, float rotation0, float rotation1, float rotation2, float rotation3, uint32 animprogress, GOState go_state, uint32 artKit = 0);
        void Update(uint32 p_time);

        bool AddPassenger(WorldObject *passenger, int8 seatId = -1, bool byAura = false);
        void RemovePassenger(WorldObject *passenger);

        void UpdatePosition(float x, float y, float z, float o);

        void CalculatePassengerPosition(float& x, float& y, float& z, float* o /*= NULL*/) const;
        void CalculatePassengerOffset(float& x, float& y, float& z, float* o /*= NULL*/) const;

        float GetStationaryX() const { return m_stationaryPosition.GetPositionX(); }
        float GetStationaryY() const { return m_stationaryPosition.GetPositionY(); }
        float GetStationaryZ() const { return m_stationaryPosition.GetPositionZ(); }
        float GetStationaryO() const { return m_stationaryPosition.GetOrientation(); }

        void ResetPositionTimer();

    protected:

        void UpdatePassengerPositions(std::set<WorldObject*>& passengers);

        std::set<WorldObject*> _passengers;
        uint32 positionInterpolateTimer;
};

#endif
