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
#include "DruidPlayer.h"

DruidPlayer::DruidPlayer(WorldSession * session): Player(session)
{
    m_eclipseDriverLeft = false;
}

void DruidPlayer::TurnEclipseDriver(bool left)
{
    m_eclipseDriverLeft = left;

    if(left)
    {
        RemoveAurasDueToSpell(67483);
        CastSpell(this,67484,true);
    }
    else
    {
        RemoveAurasDueToSpell(67484);
        CastSpell(this,67483,true);
    }
}

void DruidPlayer::ClearEclipseState()
{
    // clear power
    SetPower(POWER_ECLIPSE,0);

    // we are moving to solar eclipse
    TurnEclipseDriver(false);

    // remove all eclipse spells
    RemoveAurasDueToSpell(48517);
    RemoveAurasDueToSpell(48518);

    // and hardly set eclipse power to zero
    uint32 powerIndex = GetPowerIndexByClass(POWER_ECLIPSE, getClass());
    if (powerIndex != MAX_POWERS)
        SetUInt32Value(UNIT_FIELD_POWER1 + powerIndex, 0);
}
