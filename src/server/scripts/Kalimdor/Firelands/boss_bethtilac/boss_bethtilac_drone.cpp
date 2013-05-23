/*
 * Copyright (C) 2006-2013 iCe Online <http://ice-wow.eu>
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

/*
 * implementation of AI for summon Cinderweb Drone
 * as part of the script of Beth'tilac
 */


#include "ScriptPCH.h"
#include "../firelands.h"
#include "boss_bethtilac_data.h"

#include "boss_bethtilac_drone.h"


enum DroneSpells
{
    SPELL_LEECH_VENOM = 99411,
};

enum DroneEvents
{
    POWER_DEPLETE,
    END_OF_LEECH_VENOM_CAST,
};



CreatureAI *mob_drone::GetAI(Creature *creature) const
{
    if (creature->isSummon())
        return new mob_droneAI(creature);

    return NULL;
}



mob_drone::mob_droneAI::mob_droneAI(Creature *creature)
    : SpiderAI(creature)
    , onGround(true)
    , onTop(false)
{
}


mob_drone::mob_droneAI::~mob_droneAI()
{
}


void mob_drone::mob_droneAI::Reset()
{
    SpiderAI::Reset();
}


void mob_drone::mob_droneAI::EnterCombat(Unit *who)
{
    SpiderAI::EnterCombat(who);
}


void mob_drone::mob_droneAI::EnterEvadeMode()
{
    // do nothing
}


void mob_drone::mob_droneAI::UpdateAI(const uint32 diff)
{
    if (instance && instance->GetData(TYPE_BETHTILAC) != IN_PROGRESS)
    {
        UnSummonFilament();
        me->DespawnOrUnsummon();
        return;
    }

    if (me->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
        return;

    if (UpdateVictim())
        DoMeleeAttackIfReady();

    UpdateTimers(diff);
}


void mob_drone::mob_droneAI::DoAction(const int32 event)
{
    switch (event)
    {
        case POWER_DEPLETE:
            ModifyPower(-1);

            if (GetPower() == 0)
            {
                // if Beth'tilac is on the web above, follow her and cast Leech Venom
                Unit *summoner = me->ToTempSummon()->GetSummoner();
                if (summoner->GetPositionZ() >= webZPosition)
                {
                    onGround = false;
                    SummonFilament();
                    MoveToFilament(MOVE_POINT_UP);
                }
                //else
                //     MovementInform(POINT_MOTION_TYPE, MOVE_POINT_UP);
            }
            break;

        case END_OF_LEECH_VENOM_CAST:
            onTop = false;

            if (me->GetPositionZ() >= webZPosition)
            {
                me->NearTeleportTo(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() - 5.0f, me->GetOrientation());
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                SummonFilament();
                MoveToGround(MOVE_POINT_DOWN);
            }
            else
                MovementInform(POINT_MOTION_TYPE, MOVE_POINT_DOWN);

            break;
    }
}


void mob_drone::mob_droneAI::MovementInform(uint32 type, uint32 id)
{
    if (type == POINT_MOTION_TYPE)
    {
        switch (id)
        {
            case MOVE_POINT_UP:
                onTop = true;
                UnSummonFilament();
                me->SetFlying(false);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                me->CastSpell(me->ToTempSummon()->GetSummoner(), SPELL_LEECH_VENOM, false);
                AddTimer(END_OF_LEECH_VENOM_CAST, 1, false);        // immediately after end of channel
                break;

            case MOVE_POINT_DOWN:
                onGround = true;
                UnSummonFilament();
                me->SetFlying(false);
                me->SetReactState(REACT_AGGRESSIVE);
                break;
        }
    }
}


void mob_drone::mob_droneAI::IsSummonedBy(Unit *summoner)
{
    SetMaxPower(85);
    SetPower(85);

    AddTimer(POWER_DEPLETE, 1000, true);
    me->SetInCombatWithZone();

    Position pos;
    summoner->GetPosition(&pos);
    pos.m_positionZ -= 10.0f;
    pos.m_positionZ = summoner->GetMap()->GetHeight(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ());
    me->GetMotionMaster()->MovePoint(0, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ());    // go underneath Beth'tilac

    onGround = true;
    onTop = false;
}
