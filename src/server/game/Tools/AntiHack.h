/*
 * Copyright (C) 2006-2013, iCe Online <http://ice-wow.eu>
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

#ifndef GCORE_ANTIHACK_H
#define GCORE_ANTIHACK_H

enum AntiHackConstants
{
    SPEEDHACK_FRAME_INTERVAL = 500,  // write move frame every X ms
    SPEEDHACK_CHECK_INTERVAL = 1500, // check for deviance every X ms
    SPEEDHACK_CHECK_DEVIANCE = 50,   // maximum of X % (+-) deviance, therefore max (100 + X) % speed is tolerated
    SPEEDHACK_CHECK_FRAME_COUNT = 5, // store maximum of X frames for comparsion
};

#define NORMAL_SECOND_DISTANCE 7.0f

enum SpeedCheckEvent
{
    SPEED_CHECK_INDEPENDENT = 0,
    SPEED_CHECK_MOVE_START  = 1,
    SPEED_CHECK_MOVE_STOP   = 2,
    SPEED_CHECK_MOVE_UPDATE = 3
};

struct SpeedCheckFrame
{
    // Coordinates in current frame
    float x;
    float y;
    float z;

    // Current unix timestamp of this frame
    uint32 time;

    // event of that frame
    SpeedCheckEvent checkEvent;
};

class AntiHackServant
{
    public:
        AntiHackServant(Player* source);
        ~AntiHackServant();

        // "speed hack" detection method
        void CheckSpeedFrames(SpeedCheckEvent ev);
        void DeleteData();

    private:

        SpeedCheckFrame* m_speedFrames[SPEEDHACK_CHECK_FRAME_COUNT];
        uint32 m_lastSpeedFrameCheck;
        uint32 m_lastSpeedCheck;

        Player* m_source;
};

#endif
