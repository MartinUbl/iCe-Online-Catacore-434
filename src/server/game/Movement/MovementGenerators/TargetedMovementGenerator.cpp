/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
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
#include "ByteBuffer.h"
#include "TargetedMovementGenerator.h"
#include "Errors.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "World.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "Player.h"

template<class T, typename D>
bool TargetedMovementGeneratorMedium<T, D>::HasValidTargettedMovementPath(T* owner)
{
    // Verify, that the path is not consisted of 2 points that are equal (or have tiny difference due to floating point number storing method)
    if (i_path->GetPath().size() == 2)
    {
        G3D::Vector3 start = i_path->GetStartPosition();
        G3D::Vector3 end = i_path->GetActualEndPosition();

        // the "tiny difference" is being calculated using only horizontal plane projection
        if (fabs(start.x - end.x) < 0.0001f && fabs(start.y - end.y) < 0.0001f)
        {
            // if we are "stuck" in required distance without LoS, set indirect flag - will ignore pathfinding
            if (!i_target->IsWithinLOSInMap(owner))
                return false;
            // also when we are close, but in problematic height position (tangent too pitched), we also do not have valid path
            if (fabs(end.z - i_target->GetPositionZ()) > 1.0 && !i_target->IsFlying())
                return false;
        }
    }

    return true;
}

template<class T, typename D>
void TargetedMovementGeneratorMedium<T, D>::_setTargetLocation(T* owner, bool updateDestination)
{
    if (!i_target.isValid() || !i_target->IsInWorld())
        return;

    if (owner->hasUnitState(UNIT_STAT_NOT_MOVE))
        return;

    if (owner->GetTypeId() == TYPEID_UNIT && !i_target->isInAccessiblePlaceFor(owner->ToCreature()))
        return;

    float x, y, z;
    float dist, size = owner->GetObjectSize();

    if (updateDestination || !i_path)
    {
        if (!i_offset)
        {
            // to nearest contact position
            i_target->GetContactPoint(owner, x, y, z);
        }
        else
        {
            // Pets need special handling.
            // We need to subtract GetObjectSize() because it gets added back further down the chain
            //  and that makes pets too far away. Subtracting it allows pets to properly
            //  be (GetCombatReach() + i_offset) away.
            // Only applies when i_target is pet's owner otherwise pets and mobs end up
            //   doing a "dance" while fighting
            if (owner->isPet() && i_target->GetTypeId() == TYPEID_PLAYER)
            {
                dist = i_target->GetCombatReach();
                size = i_target->GetCombatReach() - i_target->GetObjectSize();
            }
            else
                dist = i_offset + 1.0f;

            if (i_target->IsWithinDistInMap(owner, dist))
                return;

            // to at i_offset distance from target and i_angle from target facing
            i_target->GetClosePoint(x, y, z, size, i_offset, i_angle);
        }
    }
    else
    {
        // the destination has not changed, we just need to refresh the path (usually speed change)
        G3D::Vector3 end = i_path->GetEndPosition();
        x = end.x;
        y = end.y;
        z = end.z;
    }

    if (!i_path)
        i_path = new PathGenerator(owner);

    i_path->UnsetForceSource();

    // allow pets to use shortcut if no path found when following their master
    bool forceDest = (owner->GetTypeId() == TYPEID_UNIT && owner->ToCreature()->isPet()
        && owner->hasUnitState(UNIT_STAT_FOLLOW));

    G3D::Vector3 startPos;


    bool result = i_path->CalculatePath(x, y, z, forceDest);
    if (!result || (i_path->GetPathType() & PATHFIND_NOPATH))
    {
        // Cant reach target
        i_recalculateTravel = true;

        // otherwise store target X/Y/Z
        storedDest.x = i_target->GetPositionX();
        storedDest.y = i_target->GetPositionY();
        storedDest.z = i_target->GetPositionZ();

        if (owner->GetTypeId() == TYPEID_UNIT)
            owner->GetMotionMaster()->setPathfindingState(PATHFIND_STATE_NOPATH);

        return;
    }

    D::_addUnitStateMove(owner);
    i_targetReached = false;
    i_recalculateTravel = false;
    owner->addUnitState(UNIT_STAT_CHASE);

    Movement::MoveSplineInit init(owner);

    // if some path has been found - store the destination
    const G3D::Vector3 &dest = i_path->GetActualEndPosition();
    storedDest.x = dest.x;
    storedDest.y = dest.y;
    storedDest.z = dest.z;

    // if the path generated is not direct (have to go through wall), we have to choose another path
    bool usepath = HasValidTargettedMovementPath(owner);
    if (!usepath && !(i_path->GetPathType() & PATHFIND_NOT_USING_PATH))
    {
        // if we have incomplete path, that doesn't mean we have to generate new path - the target is probably flying
        if (!(i_path->GetPathType() & PATHFIND_INCOMPLETE))
        {
            storedDest.x = i_target->GetPositionX();
            storedDest.y = i_target->GetPositionY();
            storedDest.z = i_target->GetPositionZ();

            float xt = x, yt = y, zt = z;
            float xo = owner->GetPositionX(), yo = owner->GetPositionY(), zo = owner->GetPositionZ();

            float addDist = 0.0f;
            // we will try to find path as if the target and source would be further away from themselves
            for (; addDist < 6.0f; addDist += 1.0f)
            {
                i_target->GetClosePoint(xt, yt, zt, size, i_offset + addDist / 2.0f, i_angle - M_PI);
                owner->GetClosePoint(xo, yo, zo, 0.0f, addDist, i_angle);

                i_path->SetForceSource(xo, yo, zo);
                bool result = i_path->CalculatePath(xt, yt, zt, forceDest);
                if (result && !(i_path->GetPathType() & PATHFIND_NOPATH) && HasValidTargettedMovementPath(owner))
                {
                    usepath = true;
                    break;
                }
            }
        }
        else
        {
            // incomplete path is valid in most cases
            usepath = true;

            if (owner->GetTypeId() == TYPEID_UNIT)
                owner->GetMotionMaster()->setPathfindingState(PATHFIND_STATE_NOPATH);
        }
    }
    else if ((i_path->GetPathType() & (PATHFIND_INCOMPLETE | PATHFIND_NOPATH) && !(i_path->GetPathType() & PATHFIND_NOT_USING_PATH)))
    {
        if (owner->GetTypeId() == TYPEID_UNIT)
            owner->GetMotionMaster()->setPathfindingState(PATHFIND_STATE_NOPATH);
    }

    // if direct path exists, move along
    if (usepath && !(i_path->GetPathType() & PATHFIND_NOT_USING_PATH))
    {
        init.MovebyPath(i_path->GetPath());

        // if valid path is found (not incomplete nor "no"), set pathfinding state to OK
        if (!(i_path->GetPathType() & (PATHFIND_INCOMPLETE | PATHFIND_NOPATH)) && owner->GetTypeId() == TYPEID_UNIT)
            owner->GetMotionMaster()->setPathfindingState(PATHFIND_STATE_OK);
    }
    else // if not, move straight through the thing, that breaks LoS, this is very special case (thin LoS breaking object with too complicated path around)
    {
        init.MoveTo(i_target->GetPositionX(), i_target->GetPositionY(), i_target->GetPositionZ(), false, true);

        if (owner->GetTypeId() == TYPEID_UNIT)
            owner->GetMotionMaster()->setPathfindingState(PATHFIND_STATE_OK);
    }

    init.SetWalk(((D*)this)->EnableWalking());
    // Using the same condition for facing target as the one that is used for SetInFront on movement end
    // - applies to ChaseMovementGenerator mostly
    if (i_angle == 0.f)
        init.SetFacing(i_target.getTarget());

    init.Launch();
}

template<class T, typename D>
bool TargetedMovementGeneratorMedium<T,D>::DoUpdate(T* owner, uint32 time_diff)
{
    if (!i_target.isValid() || !i_target->IsInWorld())
        return false;

    if (!owner || !owner->isAlive())
        return false;

    if (owner->hasUnitState(UNIT_STAT_NOT_MOVE))
    {
        D::_clearUnitStateMove(owner);
        return true;
    }

    // prevent movement while casting spells with cast time or channel time
    if (owner->hasUnitState(UNIT_STAT_CASTING))
    {
        if (!owner->IsStopped())
            owner->StopMoving();
        return true;
    }

    // prevent crash after creature killed pet
    if (static_cast<D*>(this)->_lostTarget(owner))
    {
        D::_clearUnitStateMove(owner);
        return true;
    }

    bool targetMoved = false;
    i_recheckDistance.Update(time_diff);
    if (i_recheckDistance.Passed())
    {
        i_recheckDistance.Reset(100);
        //More distance let have better performance, less distance let have more sensitive reaction at target move.
        float allowed_dist = owner->GetCombatReach() + sWorld->getRate(RATE_TARGET_POS_RECALCULATION_RANGE);
        G3D::Vector3 &dest = storedDest;
        float ori = 0.0f;
        if (owner->movespline->onTransport)
            if (TransportBase* transport = owner->GetDirectTransport())
                transport->CalculatePassengerPosition(dest.x, dest.y, dest.z, ori);

        // First check distance
        if ((owner->GetTypeId() == TYPEID_UNIT && owner->ToCreature()->canFly()) || fabs(i_target->GetPositionZ() - owner->GetPositionZ()) > 0.5f)
            targetMoved = !i_target->IsWithinDist3d(dest.x, dest.y, dest.z, allowed_dist);
        else
            targetMoved = !i_target->IsWithinDist2d(dest.x, dest.y, allowed_dist);

        // then, if the target is in range, check also Line of Sight.
        if (!targetMoved)
            targetMoved = !i_target->IsWithinLOSInMap(owner);
    }

    if (i_recalculateTravel || targetMoved)
        _setTargetLocation(owner, targetMoved);

    if (owner->movespline->Finalized())
    {
        static_cast<D*>(this)->MovementInform(owner);
        if (i_angle == 0.f && !owner->HasInArc(0.01f, i_target.getTarget()))
            owner->SetInFront(i_target.getTarget());

        if (!i_targetReached)
        {
            i_targetReached = true;
            static_cast<D*>(this)->_reachTarget(owner);
        }
    }

    return true;
}

//-----------------------------------------------//
template<class T>
void ChaseMovementGenerator<T>::_reachTarget(T* owner)
{
    if (owner->IsWithinMeleeRange(this->i_target.getTarget()))
        owner->Attack(this->i_target.getTarget(),true);
}

template<>
void ChaseMovementGenerator<Player>::DoInitialize(Player* owner)
{
    owner->addUnitState(UNIT_STAT_CHASE|UNIT_STAT_CHASE_MOVE);
    _setTargetLocation(owner, true);
}

template<>
void ChaseMovementGenerator<Creature>::DoInitialize(Creature* owner)
{
    owner->SetWalk(false);
    owner->addUnitState(UNIT_STAT_CHASE|UNIT_STAT_CHASE_MOVE);
    _setTargetLocation(owner, true);
}

template<class T>
void ChaseMovementGenerator<T>::DoFinalize(T* owner)
{
    owner->clearUnitState(UNIT_STAT_CHASE|UNIT_STAT_CHASE_MOVE);
}

template<class T>
void ChaseMovementGenerator<T>::DoReset(T* owner)
{
    DoInitialize(owner);
}

template<class T>
void ChaseMovementGenerator<T>::MovementInform(T* /*unit*/)
{
}

template<>
void ChaseMovementGenerator<Creature>::MovementInform(Creature* unit)
{
    // Pass back the GUIDLow of the target. If it is pet's owner then PetAI will handle
    if (unit->AI())
        unit->AI()->MovementInform(CHASE_MOTION_TYPE, i_target.getTarget()->GetGUIDLow());
}

//-----------------------------------------------//
template<>
bool FollowMovementGenerator<Creature>::EnableWalking() const
{
    return i_target.isValid() && i_target->IsWalking();
}

template<>
bool FollowMovementGenerator<Player>::EnableWalking() const
{
    return false;
}

template<>
void FollowMovementGenerator<Player>::_updateSpeed(Player* /*u*/)
{
    // nothing to do for Player
}

template<>
void FollowMovementGenerator<Creature>::_updateSpeed(Creature *owner)
{
    // pet only sync speed with owner
    // Make sure we are not in the process of a map change
    if (!owner || !owner->isPet() || !owner->IsInWorld() || !i_target.isValid() || i_target->GetGUID() != owner->GetOwnerGUID())
        return;

    owner->UpdateSpeed(MOVE_RUN, true);
    owner->UpdateSpeed(MOVE_WALK, true);
    owner->UpdateSpeed(MOVE_SWIM, true);
}

template<>
void FollowMovementGenerator<Player>::DoInitialize(Player* owner)
{
    owner->addUnitState(UNIT_STAT_FOLLOW|UNIT_STAT_FOLLOW_MOVE);
    _updateSpeed(owner);
    _setTargetLocation(owner, true);
}

template<>
void FollowMovementGenerator<Creature>::DoInitialize(Creature* owner)
{
    owner->addUnitState(UNIT_STAT_FOLLOW|UNIT_STAT_FOLLOW_MOVE);
    _updateSpeed(owner);
    _setTargetLocation(owner, true);
}

template<class T>
void FollowMovementGenerator<T>::DoFinalize(T* owner)
{
    owner->clearUnitState(UNIT_STAT_FOLLOW|UNIT_STAT_FOLLOW_MOVE);
    _updateSpeed(owner);
}

template<class T>
void FollowMovementGenerator<T>::DoReset(T* owner)
{
    DoInitialize(owner);
}

template<class T>
void FollowMovementGenerator<T>::MovementInform(T* /*unit*/)
{
}

template<>
void FollowMovementGenerator<Creature>::MovementInform(Creature* unit)
{
    // Pass back the GUIDLow of the target. If it is pet's owner then PetAI will handle
    if (unit->AI())
        unit->AI()->MovementInform(FOLLOW_MOTION_TYPE, i_target.getTarget()->GetGUIDLow());
}

//-----------------------------------------------//
template void TargetedMovementGeneratorMedium<Player,ChaseMovementGenerator<Player> >::_setTargetLocation(Player*, bool);
template void TargetedMovementGeneratorMedium<Player,FollowMovementGenerator<Player> >::_setTargetLocation(Player*, bool);
template void TargetedMovementGeneratorMedium<Creature,ChaseMovementGenerator<Creature> >::_setTargetLocation(Creature*, bool);
template void TargetedMovementGeneratorMedium<Creature,FollowMovementGenerator<Creature> >::_setTargetLocation(Creature*, bool);
template bool TargetedMovementGeneratorMedium<Player,ChaseMovementGenerator<Player> >::DoUpdate(Player*, uint32);
template bool TargetedMovementGeneratorMedium<Player,FollowMovementGenerator<Player> >::DoUpdate(Player*, uint32);
template bool TargetedMovementGeneratorMedium<Creature,ChaseMovementGenerator<Creature> >::DoUpdate(Creature*, uint32);
template bool TargetedMovementGeneratorMedium<Creature,FollowMovementGenerator<Creature> >::DoUpdate(Creature*, uint32);

template void ChaseMovementGenerator<Player>::_reachTarget(Player*);
template void ChaseMovementGenerator<Creature>::_reachTarget(Creature*);
template void ChaseMovementGenerator<Player>::DoFinalize(Player*);
template void ChaseMovementGenerator<Creature>::DoFinalize(Creature*);
template void ChaseMovementGenerator<Player>::DoReset(Player*);
template void ChaseMovementGenerator<Creature>::DoReset(Creature*);
template void ChaseMovementGenerator<Player>::MovementInform(Player*);

template void FollowMovementGenerator<Player>::DoFinalize(Player*);
template void FollowMovementGenerator<Creature>::DoFinalize(Creature*);
template void FollowMovementGenerator<Player>::DoReset(Player*);
template void FollowMovementGenerator<Creature>::DoReset(Creature*);
template void FollowMovementGenerator<Player>::MovementInform(Player*);
