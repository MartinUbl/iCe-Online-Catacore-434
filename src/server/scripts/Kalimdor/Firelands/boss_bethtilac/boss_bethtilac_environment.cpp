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
 * implementation of AI for environment of Beth'tilac encounter
 */

#include "ScriptPCH.h"
#include "../firelands.h"
#include "boss_bethtilac_data.h"

#include "boss_bethtilac_environment.h"



// Spiderweb Filament dropped from the web above after killing Cinterweb Spinner

enum FilamentSpells
{
    SPELL_FILAMENT_VISUAL = 98149
};

enum FilamentEvents
{
    EVENT_TRANSFER_START,
    EVENT_DESPAWN,
};


CreatureAI *npc_filament::GetAI(Creature *creature) const
{
    return new filamentAI(creature);
}


npc_filament::filamentAI::filamentAI(Creature *creature)
    : SpiderAI(creature)
{
    transporting = false;
}


npc_filament::filamentAI::~filamentAI()
{
}


void npc_filament::filamentAI::Reset()
{
    SummonFilament();
    me->CastSpell(me, SPELL_FILAMENT_VISUAL, true);
    AddTimer(EVENT_DESPAWN, 30000, false);

    transporting = false;
}


void npc_filament::filamentAI::UpdateAI(const uint32 diff)
{
    if (instance && instance->GetData(TYPE_BETHTILAC) != IN_PROGRESS)
    {
        UnSummonFilament();
        me->DespawnOrUnsummon();
        return;
    }

    UpdateTimers(diff);
}


void npc_filament::filamentAI::MoveInLineOfSight(Unit *who)
{
}


void npc_filament::filamentAI::AttackStart(Unit *victim)
{
}


void npc_filament::filamentAI::EnterEvadeMode()
{
}


void npc_filament::filamentAI::MovementInform(uint32 type, uint32 id)
{
    if (type == POINT_MOTION_TYPE && id == MOVE_POINT_UP)
    {
        if (me->IsVehicle())
        {
            UnSummonFilament();
            if (Vehicle *veh = me->GetVehicleKit())
                veh->RemoveAllPassengers();

            me->DespawnOrUnsummon();
        }
    }
}


void npc_filament::filamentAI::PassengerBoarded(Unit *unit, int8 seat, bool apply)
{
    transporting = apply;

    if (apply)
    {
        unit->addUnitState(UNIT_STAT_ONVEHICLE);        // makes the passenger unattackable
        AddTimer(EVENT_TRANSFER_START, 1500, false);

        me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
    }
    else
    {
        unit->clearUnitState(UNIT_STAT_ONVEHICLE);
        //unit->NearTeleportTo(unit->GetPositionX(), unit->GetPositionY(), webZPosition + 1.0f, unit->GetOrientation());
    }

    unit->RemoveAllAttackers();
    unit->DeleteThreatList();
}


void npc_filament::filamentAI::DoAction(const int32 event)
{
    switch (event)
    {
    case EVENT_TRANSFER_START:
        MoveToFilament(MOVE_POINT_UP);  // move the passenger
        break;
    case EVENT_DESPAWN:
        if (!transporting)
            me->DespawnOrUnsummon(0);
        break;
    }
}



// Web Rip, created by Meteor Burn spell

static const uint32 SPELL_METEOR_BURN_VISUAL = 99071;

CreatureAI *npc_web_rip::GetAI(Creature *creature) const
{
    return new AI(creature);
}


npc_web_rip::AI::AI(Creature *creature)
    : NullCreatureAI(creature)
{
}


void npc_web_rip::AI::IsSummonedBy(Unit *summoner)
{
    me->setFaction(14);
    me->SetReactState(REACT_PASSIVE);
    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
    me->CastSpell(me, SPELL_METEOR_BURN_VISUAL, true);
}


void npc_web_rip::AI::EnterEvadeMode()
{
}


// spell script for Meteor Burn

SpellScript *spell_meteor_burn::GetSpellScript() const
{
    return new spell_meteor_burn::Script();
}


bool spell_meteor_burn::Script::Validate(SpellEntry const *spell)
{
    return spell->Effect[EFFECT_0] == SPELL_EFFECT_DUMMY &&
           spell->Effect[EFFECT_1] == SPELL_EFFECT_NONE &&
           spell->Effect[EFFECT_2] == SPELL_EFFECT_NONE;
}


void spell_meteor_burn::Script::Register()
{
    OnEffect += SpellEffectFn(spell_meteor_burn::Script::HandleHit, EFFECT_0, SPELL_EFFECT_DUMMY);
}


void spell_meteor_burn::Script::HandleHit(SpellEffIndex effIndex)
{
    // summon a NPC above the victim, which casts the spell
    // the missile flies from the caster of the spell
    if (Unit *target = GetHitUnit())
    {
        Unit *caster = GetCaster();

        float posX = target->GetPositionX(),
              posY = target->GetPositionY(),
              posZ = target->GetPositionZ();

        if (TempSummon *summon = caster->SummonCreature(NPC_FILAMENT_CASTER,
                        posX, posY, posZ + 10, 0,
                        TEMPSUMMON_TIMED_DESPAWN, 10000))
        {
            summon->setFaction(14);     // enemy to players
            summon->CastSpell(posX, posY, posZ, 99073, false, NULL, NULL, caster->GetGUID(), target);
        }
    }
}


// creature script for Sticky Webbing

static const uint32 SPELL_STICKY_WEBBING = 99219;


npc_sticky_webbing::AI::AI(Creature *creature)
    : NullCreatureAI(creature)
{
}

void npc_sticky_webbing::AI::Reset()
{
    if (!me->HasAura(SPELL_STICKY_WEBBING))
        me->CastSpell(me, SPELL_STICKY_WEBBING, false);    // triggers the channel of Sticky Webbing
}
