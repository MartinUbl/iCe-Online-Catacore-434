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
#include "HomeMovementGenerator.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "WorldPacket.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"

void HomeMovementGenerator<Creature>::DoInitialize(Creature* owner)
{
    owner->SetWalk(false);
    owner->AddUnitState(UNIT_STATE_EVADE);
    _setTargetLocation(owner);
}

void HomeMovementGenerator<Creature>::DoFinalize(Creature* owner)
{
    owner->ClearUnitState(UNIT_STATE_EVADE);
    if (arrived)
    {
        owner->SetWalk(true);
        owner->LoadCreaturesAddon(true);
        owner->AI()->JustReachedHome();
    }
}

void HomeMovementGenerator<Creature>::DoReset(Creature*)
{
}

void HomeMovementGenerator<Creature>::_setTargetLocation(Creature* owner)
{
    if (owner->HasUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED | UNIT_STATE_DISTRACTED))
    {
        arrived = false;
        DoFinalize(owner);
        return;
    }

    Movement::MoveSplineInit init(owner);
    float x, y, z, o;
    // at apply we can select more nice return points base at current movegen
    if (owner->GetMotionMaster()->empty() || !owner->GetMotionMaster()->top()->GetResetPosition(*owner, x, y, z))
    {
        owner->GetHomePosition(x, y, z, o);
        init.SetFacing(o);
    }
    init.MoveTo(x,y,z);
    init.SetWalk(false);
    init.Launch();

    arrived = false;
    owner->ClearUnitState(UNIT_STATE_ALL_STATE & ~(UNIT_STATE_EVADE | UNIT_STATE_IGNORE_PATHFINDING));
}

bool HomeMovementGenerator<Creature>::DoUpdate(Creature* owner, const uint32 /*time_diff*/)
{
    arrived = owner->movespline->Finalized();
    return !arrived;
}
