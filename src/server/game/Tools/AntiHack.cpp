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

#include "gamePCH.h"
#include "AntiHack.h"

AntiHackServant::AntiHackServant(Player* source)
{
    m_source = source;
    m_lastSpeedFrameCheck = 0;
    m_lastSpeedCheck = 0;
    memset(m_speedFrames, 0, sizeof(m_speedFrames));
}

AntiHackServant::~AntiHackServant()
{
}

void AntiHackServant::CheckSpeedFrames(SpeedCheckEvent ev)
{
    if (!m_source->IsInWorld())
        return;

    // Just for GM - WE ARE TESTING!
    if (m_source->GetSession()->GetSecurity() == SEC_PLAYER)
        return;

    // check if it's time to insert a new speed frame
    if (getMSTimeDiff(m_lastSpeedFrameCheck, getMSTime()) < SPEEDHACK_FRAME_INTERVAL)
        return;

    m_lastSpeedFrameCheck = getMSTime();

    // Insert new frame

    if (m_speedFrames[SPEEDHACK_CHECK_FRAME_COUNT - 1] != NULL)
        delete m_speedFrames[SPEEDHACK_CHECK_FRAME_COUNT - 1];

    for (int32 i = SPEEDHACK_CHECK_FRAME_COUNT - 2; i >= 0; i--)
    {
        if (m_speedFrames[i] == NULL)
            continue;

        m_speedFrames[i+1] = m_speedFrames[i];
    }

    SpeedCheckFrame* fr = new SpeedCheckFrame();

    m_source->GetPosition(fr->x, fr->y, fr->z);
    fr->time = getMSTime();
    fr->checkEvent = ev;

    m_speedFrames[0] = fr;

    // Check for allowed deviation once per specified period
    if (getMSTimeDiff(m_lastSpeedCheck, getMSTime()) < SPEEDHACK_CHECK_INTERVAL)
        return;

    m_lastSpeedCheck = getMSTime();

    float dist_x = 0;
    float dist_y = 0;
    float dist_z = 0;
    float dist_plane = 0;

    uint32 timeSpent = 0;
    for (uint32 i = 1; i < SPEEDHACK_CHECK_FRAME_COUNT; i++)
    {
        if (m_speedFrames[i] == NULL)
            continue;

        if (m_speedFrames[i-1]->checkEvent == SPEED_CHECK_MOVE_START || m_speedFrames[i]->checkEvent == SPEED_CHECK_MOVE_STOP)
            break;

        dist_x += fabs(m_speedFrames[i]->x - m_speedFrames[i-1]->x);
        dist_y += fabs(m_speedFrames[i]->y - m_speedFrames[i-1]->y);
        dist_z += fabs(m_speedFrames[i]->z - m_speedFrames[i-1]->z);

        timeSpent += getMSTimeDiff(m_speedFrames[i]->time, m_speedFrames[i-1]->time);
    }

    dist_plane = sqrt(pow(dist_x, 2) + pow(dist_y, 2));

    // we don't care about not-moving players
    if (dist_plane < 1.0f || timeSpent == 0)
        return;

    float deviance = 100.0f*((1000.0f*dist_plane/(float)timeSpent)/(NORMAL_SECOND_DISTANCE*m_source->GetSpeedRate(MOVE_RUN)));
    if (!m_source->GetTransport() && !m_source->GetVehicle() && deviance >= 100.0f+SPEEDHACK_CHECK_DEVIANCE)
    {
        m_source->GetSession()->SendNotification("Caution, your speed deviates by %i%% !", (int32)(deviance-100.0f));
    }
}
