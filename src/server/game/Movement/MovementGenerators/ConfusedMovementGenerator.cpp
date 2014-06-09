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
#include "Creature.h"
#include "MapManager.h"
#include "ConfusedMovementGenerator.h"
#include "PathGenerator.h"
#include "VMapFactory.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"

#ifdef MAP_BASED_RAND_GEN
#define rand_norm() unit.rand_norm()
#define urand(a,b) unit.urand(a,b)
#endif

template<class T>
void ConfusedMovementGenerator<T>::DoInitialize(T* unit)
{
    unit->addUnitState(UNIT_STAT_CONFUSED);
    unit->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_CONFUSED);
    unit->GetPosition(i_x, i_y, i_z);

    if (!unit->isAlive() || unit->IsStopped())
        return;

    unit->StopMoving();
    unit->addUnitState(UNIT_STAT_CONFUSED_MOVE);
}

template<class T>
void ConfusedMovementGenerator<T>::DoReset(T* unit)
{
    i_nextMoveTime.Reset(0);

    if (!unit->isAlive() || unit->IsStopped())
        return;

    unit->StopMoving();
}

template<class T>
bool ConfusedMovementGenerator<T>::DoUpdate(T* unit, uint32 diff)
{
    if (unit->hasUnitState(UNIT_STAT_ROOT | UNIT_STAT_STUNNED | UNIT_STAT_DISTRACTED))
        return true;

    if (i_nextMoveTime.Passed())
    {
        // currently moving, update location
        unit->addUnitState(UNIT_STAT_CONFUSED_MOVE);

        if (unit->movespline->Finalized())
            i_nextMoveTime.Reset(urand(800, 1500)); // Guessed
    }
    else
    {
        // waiting for next move
        i_nextMoveTime.Update(diff);
        if (i_nextMoveTime.Passed())
        {
            // start moving
            unit->addUnitState(UNIT_STAT_CONFUSED_MOVE);

            float dest = 4.0f * (float)rand_norm() - 2.0f;

            Position pos;
            pos.Relocate(i_x, i_y, i_z);
            unit->MovePositionToFirstCollision(pos, dest, 0.0f);

            PathGenerator path(unit);
            path.SetPathLengthLimit(30.0f);
            bool result = path.CalculatePath(pos.m_positionX, pos.m_positionY, pos.m_positionZ);
            if (!result || (path.GetPathType() & PATHFIND_NOPATH))
            {
                i_nextMoveTime.Reset(100);
                return true;
            }

            Movement::MoveSplineInit init(unit);
            init.MovebyPath(path.GetPath());
            init.SetWalk(true);
            init.Launch();
        }
    }

    return true;
}

template<>
void ConfusedMovementGenerator<Player>::DoFinalize(Player* unit)
{
    unit->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_CONFUSED);
    unit->clearUnitState(UNIT_STAT_CONFUSED | UNIT_STAT_CONFUSED_MOVE);
    unit->StopMoving();
}

template<>
void ConfusedMovementGenerator<Creature>::DoFinalize(Creature* unit)
{
    unit->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_CONFUSED);
    unit->clearUnitState(UNIT_STAT_CONFUSED | UNIT_STAT_CONFUSED_MOVE);
    if (unit->getVictim())
        unit->SetUInt64Value(UNIT_FIELD_TARGET, unit->getVictim()->GetGUID());
}

template void ConfusedMovementGenerator<Player>::DoInitialize(Player*);
template void ConfusedMovementGenerator<Creature>::DoInitialize(Creature*);
template void ConfusedMovementGenerator<Player>::DoReset(Player*);
template void ConfusedMovementGenerator<Creature>::DoReset(Creature*);
template bool ConfusedMovementGenerator<Player>::DoUpdate(Player*, uint32 diff);
template bool ConfusedMovementGenerator<Creature>::DoUpdate(Creature*, uint32 diff);
