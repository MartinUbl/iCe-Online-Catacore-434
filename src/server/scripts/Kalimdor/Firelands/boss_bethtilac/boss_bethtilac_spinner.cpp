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
#include "boss_bethtilac_spiderAI.h"


namespace Bethtilac
{


class mob_spinnerAI: public SpiderAI
{
public:
    explicit mob_spinnerAI(Creature *creature);
    virtual ~mob_spinnerAI();

private:
    // virtual method overrides
    void EnterEvadeMode();
    void JustDied(Unit *killer);
    void UpdateAI(const uint32 diff);
    void DoAction(const int32 event);
    void MovementInform(uint32 type, uint32 id);
    void IsSummonedBy(Unit * summoner);
    void SpellHit(Unit *caster, const SpellEntry *spell);

    // attributes
    bool hanging;       // spider is hanging on one place on the filament
    bool onGround;      // spider is moving on the ground (not hanging)
    bool summoned;  // summoned a filament for the players (either after taunted or killed)
};


class mob_spinner: public CreatureScript
{
public:
    mob_spinner(): CreatureScript("mob_cinderweb_spinner") {}
    CreatureAI *GetAI(Creature *creature) const
    {
        if (creature->isSummon())
            return new mob_spinnerAI(creature);

        return NULL;
    }
};

void load_mob_Spinner()
{
    new mob_spinner();
}





//////////////////////////////////////////////////////////////////////////
// implementation of Cinderweb Spinner


static const int MOVE_POINT_DOWN2 = MOVE_POINT_DOWN + 10;

enum SpinnerSpells
{
    SPELL_BURNING_ACID = 98471,
};

enum SpinnerEvents
{
    EVENT_BURNING_ACID,
    EVENT_SUMMON_FILAMENT,
};




mob_spinnerAI::mob_spinnerAI(Creature *creature)
    : SpiderAI(creature)
    , hanging(false)
    , onGround(false)
    , summoned(false)
{
}


mob_spinnerAI::~mob_spinnerAI()
{
}


void mob_spinnerAI::EnterEvadeMode()
{
    // do nothing
}


void mob_spinnerAI::JustDied(Unit *killer)
{
    DoAction(EVENT_SUMMON_FILAMENT);
}


void mob_spinnerAI::UpdateAI(const uint32 diff)
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


void mob_spinnerAI::DoAction(const int32 event)
{
    switch (event)
    {
        case EVENT_BURNING_ACID:
            me->CastCustomSpell(SPELL_BURNING_ACID, SPELLVALUE_MAX_TARGETS, 1, me, false);
            break;
        case EVENT_SUMMON_FILAMENT:
            if (!summoned)
            {
                summoned = true;
                me->SummonCreature(NPC_SPIDERWEB_FILAMENT, me->GetPositionX(), me->GetPositionY(), webZPosition - 5.0f, 0);
            }
            break;
    }
}


void mob_spinnerAI::MovementInform(uint32 type, uint32 id)
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

                UnSummonFilament();

                me->SetFlying(false);
                me->SetReactState(REACT_AGGRESSIVE);
                AddTimer(EVENT_BURNING_ACID, 2000, true);
            }
        }
    }
}


void mob_spinnerAI::IsSummonedBy(Unit *summoner)
{
    onGround = false;
    hanging = true;
    summoned = false;

    // move halfway to the ground, remain hanging on the filament
    me->SetReactState(REACT_PASSIVE);
    me->SetFlying(true);
    me->GetMotionMaster()->MovePoint(MOVE_POINT_DOWN, me->GetPositionX(), me->GetPositionY(), 90.0f);

    SummonFilament();
    DoZoneInCombat();
}


void mob_spinnerAI::SpellHit(Unit *caster, const SpellEntry *spell)
{
    if (spell->HasSpellEffect(SPELL_EFFECT_ATTACK_ME))
    {
        if (hanging)
        {
            hanging = false;

            ClearTimers();
            MoveToGround(MOVE_POINT_DOWN2);

            // summon filament for players
            AddTimer(EVENT_SUMMON_FILAMENT, 2000, false);
        }
    }
}


}   // end of namespace
