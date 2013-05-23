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
 * implementation of AI for summon Cinderweb Spinner
 * as part of the script of Beth'tilac
 */


#include "ScriptPCH.h"
#include "../firelands.h"
#include "boss_bethtilac_data.h"

#include "boss_bethtilac_spinner.h"


static const int MOVE_POINT_DOWN2 = MOVE_POINT_DOWN + 10;

enum SpinnerSpells
{
    SPELL_BURNING_ACID = 98471
};

enum SpinnerEvents
{
    EVENT_BURNING_ACID
};



CreatureAI *mob_spinner::GetAI(Creature *creature) const
{
    if (creature->isSummon())
        return new mob_spinnerAI(creature);

    return NULL;
}



mob_spinner::mob_spinnerAI::mob_spinnerAI(Creature *creature)
    : SpiderAI(creature)
    , hanging(false)
    , onGround(false)
{
}


mob_spinner::mob_spinnerAI::~mob_spinnerAI()
{
}


void mob_spinner::mob_spinnerAI::EnterEvadeMode()
{
    // do nothing
}


void mob_spinner::mob_spinnerAI::UpdateAI(const uint32 diff)
{
    if (instance && instance->GetData(TYPE_BETHTILAC) != IN_PROGRESS)
    {
        UnSummonFilament();
        me->DespawnOrUnsummon();
        return;
    }

    if (onGround)
        DoMeleeAttackIfReady();

    UpdateTimers(diff);
}


void mob_spinner::mob_spinnerAI::DoAction(const int32 event)
{
    switch (event)
    {
        case EVENT_BURNING_ACID:
            me->CastCustomSpell(SPELL_BURNING_ACID, SPELLVALUE_MAX_TARGETS, 1, NULL, false);
            break;
    }
}


void mob_spinner::mob_spinnerAI::MovementInform(uint32 type, uint32 id)
{
    switch (id)
    {
        case MOVE_POINT_DOWN:
        {
            // half of the way down (stay here after being summoned until taunted)
            if (type == POINT_MOTION_TYPE)
            {
                hanging = true;
                AddTimer(EVENT_BURNING_ACID, 2000, true);
            }
            break;
        }
        case MOVE_POINT_DOWN2:
        {
            // reached ground
            if (type == POINT_MOTION_TYPE)
            {
                hanging = false;
                onGround = true;

                me->SetFlying(false);
                me->SetReactState(REACT_AGGRESSIVE);
                AddTimer(EVENT_BURNING_ACID, 2000, true);
            }
        }
    }
}


void mob_spinner::mob_spinnerAI::IsSummonedBy(Unit *summoner)
{
    onGround = false;
    hanging = true;

    // move halfway to the ground, remain hanging on the filament
    me->SetReactState(REACT_PASSIVE);
    me->SetFlying(true);
    me->GetMotionMaster()->MovePoint(MOVE_POINT_DOWN, me->GetPositionX(), me->GetPositionY(), 90.0f);

    SummonFilament();
    DoZoneInCombat();
}


void mob_spinner::mob_spinnerAI::SpellHit(Unit *caster, const SpellEntry *spell)
{
    if (spell->HasSpellEffect(SPELL_EFFECT_ATTACK_ME))
    {
        if (hanging)
        {
            hanging = false;

            ClearTimers();
            MoveToGround(MOVE_POINT_DOWN2);
        }
    }
}
