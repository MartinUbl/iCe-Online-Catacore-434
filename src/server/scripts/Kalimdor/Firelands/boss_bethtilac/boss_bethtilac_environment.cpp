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
#include "boss_bethtilac_spiderAI.h"



// Spiderweb Filament dropped from the web above after killing Cinterweb Spinner
class filamentAI: public SpiderAI
{
public:
    explicit filamentAI(Creature *creature);
    virtual ~filamentAI();

    void Reset();
    void UpdateAI(const uint32 diff);
    void MoveInLineOfSight(Unit *who);
    void AttackStart(Unit *victim);
    void EnterEvadeMode();
    void MovementInform(uint32 type, uint32 id);
    void PassengerBoarded(Unit *unit, int8 seat, bool apply);
    void DoAction(const int32 event);

private:
    bool transporting;
};

class npc_filament: public CreatureScript
{
public:
    npc_filament(): CreatureScript("npc_spiderweb_filament") {}
    CreatureAI *GetAI(Creature *creature) const
    {
        return new filamentAI(creature);
    }
};

// Web Rip, created by Meteor Burn spell
class WebRipAI: public NullCreatureAI
{
public:
    explicit WebRipAI(Creature *creature);

private:
    virtual void IsSummonedBy(Unit *summoner);
    virtual void EnterEvadeMode();
    virtual void UpdateAI(const uint32 diff);

    InstanceScript *instance;
};

class npc_web_rip: public CreatureScript
{
public:
    npc_web_rip(): CreatureScript("npc_web_rip") {}
    CreatureAI *GetAI(Creature *creature) const
    {
        return new WebRipAI(creature);
    }
};

// spell Meteor Burn - need to summon temporary NPC above the target, which casts the meteor
class MeteorBurnScript: public SpellScript
{
private:
    PrepareSpellScript(MeteorBurnScript)
    bool Validate(SpellEntry const *spell);
    void Register();
    void HandleHit(SpellEffIndex effIndex);
};

class spell_meteor_burn: public SpellScriptLoader
{
public:
    spell_meteor_burn(): SpellScriptLoader("spell_meteor_burn") {}
    SpellScript *GetSpellScript() const
    {
        return new MeteorBurnScript();
    }
};

// Sticky Webbing - handles applying of slow fall when falling through the hole in the middle of the web
class StickyWebbingAI: public NullCreatureAI
{
public:
    StickyWebbingAI(Creature *creature);
private:
    void Reset();
};

class npc_sticky_webbing: public CreatureScript
{
public:
    npc_sticky_webbing(): CreatureScript("npc_sticky_webbing") {}
    CreatureAI *GetAI(Creature *creature) const
    {
        return new StickyWebbingAI(creature);
    }
};


void load_beth_environment()
{
    new npc_filament();
    new npc_web_rip();
    new spell_meteor_burn();
    new npc_sticky_webbing();
}




//////////////////////////////////////////////////////////////////////////
// implementation of environment scripts


// ----------------------------------------------------
// Spiderweb Filament
enum FilamentSpells
{
    SPELL_FILAMENT_VISUAL = 98149
};

enum FilamentEvents
{
    EVENT_TRANSFER_START,
    EVENT_DESPAWN,
};
filamentAI::filamentAI(Creature *creature)
    : SpiderAI(creature)
{
    transporting = false;
}
filamentAI::~filamentAI()
{
}
void filamentAI::Reset()
{
    SummonFilament();
    me->CastSpell(me, SPELL_FILAMENT_VISUAL, true);
    AddTimer(EVENT_DESPAWN, 30000, false);

    transporting = false;
}
void filamentAI::UpdateAI(const uint32 diff)
{
    if (instance && instance->GetData(TYPE_BETHTILAC) != IN_PROGRESS)
    {
        UnSummonFilament();
        me->DespawnOrUnsummon();
        return;
    }

    UpdateTimers(diff);
}
void filamentAI::MoveInLineOfSight(Unit *who)
{
}
void filamentAI::AttackStart(Unit *victim)
{
}
void filamentAI::EnterEvadeMode()
{
}
void filamentAI::MovementInform(uint32 type, uint32 id)
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
void filamentAI::PassengerBoarded(Unit *unit, int8 seat, bool apply)
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
void filamentAI::DoAction(const int32 event)
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


// Web Rip
static const uint32 SPELL_METEOR_BURN_VISUAL = 99071;

WebRipAI::WebRipAI(Creature *creature)
    : NullCreatureAI(creature)
{
}
void WebRipAI::IsSummonedBy(Unit *summoner)
{
    me->setFaction(14);
    me->SetReactState(REACT_PASSIVE);
    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
    me->CastSpell(me, SPELL_METEOR_BURN_VISUAL, true);

    instance = me->GetInstanceScript();
}
void WebRipAI::EnterEvadeMode()
{
}
void WebRipAI::UpdateAI(const uint32 diff)
{
    if (instance && instance->GetData(TYPE_BETHTILAC) != IN_PROGRESS)
        me->DespawnOrUnsummon();
}

// ----------------------------------------------------
// Meteor Burn
bool MeteorBurnScript::Validate(SpellEntry const *spell)
{
    return spell->Effect[EFFECT_0] == SPELL_EFFECT_DUMMY &&
           spell->Effect[EFFECT_1] == SPELL_EFFECT_NONE &&
           spell->Effect[EFFECT_2] == SPELL_EFFECT_NONE;
}
void MeteorBurnScript::Register()
{
    OnEffect += SpellEffectFn(MeteorBurnScript::HandleHit, EFFECT_0, SPELL_EFFECT_DUMMY);
}
void MeteorBurnScript::HandleHit(SpellEffIndex effIndex)
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


// ----------------------------------------------------
// Sticky Webbing
static const uint32 SPELL_STICKY_WEBBING = 99219;

StickyWebbingAI::StickyWebbingAI(Creature *creature)
    : NullCreatureAI(creature)
{
}
void StickyWebbingAI::Reset()
{
    if (!me->HasAura(SPELL_STICKY_WEBBING))
        me->CastSpell(me, SPELL_STICKY_WEBBING, false);    // triggers the channel of Sticky Webbing
}
