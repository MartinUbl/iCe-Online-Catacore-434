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
 * implementation of AI for boss Beth'tilac in Firelands
 * based on script in Deadly boss mods addon
 * and http://www.icy-veins.com/beth-tilac-detailed-strategy-wow
 */


#include "ScriptPCH.h"
#include <math.h>
#include "../firelands.h"
#include "Spell.h"

#include "boss_bethtilac.h"
#include "boss_bethtilac_data.h"


// Beth'tilac implementation

enum BethtilacSpells
{
    // phase 1
    SPELL_SMOLDERING_DEVASTATION = 99052,
    SPELL_EMBER_FLARE = 98934,
    SPELL_VENOM_RAIN = 99333,
    // phase 2
    SPELL_FRENZY = 99497,
    SPELL_EMBER_FLARE_2 = 99859,
    SPELL_WIDOW_KISS = 99476,

    // summons
    SPELL_SUMMON_SPINNER = 98872,
};

enum BethtilacEvents
{
    POWER_DECAY,        // decay 100 energy each second

    // combat check
    COMBAT_CHECK_ENABLE,

    TRANSFER_TIMEOUT,     // transfer phases have limited duration - if they last longer, something went wrong

    // phase 1
    SD_ENABLE,            // Smoldering Devastation enabled
    SP_EMBER_FLARE,       // cast Ember Flare
    SP_VENOM_RAIN,        // cast Venom Rain (instant) if no one is in range
    SUMMON_FIRST_DRONE,   // first Cinderweb Drone is spawned earlier than others
    SUMMON_DRONE,
    SUMMON_SPINNERS,
    SUMMON_FIRST_SPIDERLINGS,
    SUMMON_SPIDERLINGS,

    // phase 2
    END_OF_PHASE_1,       // phase 1 ended
    SP_FRENZY,
    SP_EMBER_FLARE_2,
    SP_WIDOW_KISS,
};



boss_bethtilac::boss_bethtilacAI::boss_bethtilacAI(Creature *creature)
    : SpiderAI(creature)
{
    phase = PHASE_IDLE;
    devastationEnabled = true;
    devastationCounter = 0;

    summonCombatChecker = NULL;
}


boss_bethtilac::boss_bethtilacAI::~boss_bethtilacAI()
{
    //if (summonCombatChecker)
    //    summonCombatChecker->UnSummon();
}


void boss_bethtilac::boss_bethtilacAI::Reset()
{
    if (instance)
        instance->SetData(TYPE_BETHTILAC, NOT_STARTED);

    DebugOutput("R E S E T");

    ResetPower();
    ClearTimers();

    me->SetHealth(me->GetMaxHealth());
    phase = PHASE_IDLE;

    // teleport to starting position and set "flying"
    me->SetFlying(true);        // don't use hover, it would break the visual effect
    me->StopMoving();
    Position start = me->GetHomePosition();
    me->NearTeleportTo(start.GetPositionX(), start.GetPositionY(), start.GetPositionZ(), start.GetOrientation());
    me->GetMotionMaster()->MovePoint(0, start);

    // summon the filament
    SummonFilament();
}


void boss_bethtilac::boss_bethtilacAI::EnterCombat(Unit *who)
{
    if (instance)
        instance->SetData(TYPE_BETHTILAC, IN_PROGRESS);

    this->DoZoneInCombat();

    // summon combat checker
    summonCombatChecker = me->SummonTrigger(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, 0);
    summonCombatChecker->SetVisible(true);

    devastationEnabled = true;
    devastationCounter = 0;

    combatCheckEnabled = true;

    EnterPhase(PHASE_TRANSFER_1);
}


bool boss_bethtilac::boss_bethtilacAI::UpdateVictim()
{
    if (phase == PHASE_IDLE)
        return false;

    if (IsInTransfer() || ScriptedAI::UpdateVictim())
    {
        combatCheckEnabled = true;
        return true;
    }

    if (phase != PHASE_1 || !me->isInCombat())
        return false;

    me->AttackStop();
    return true;
}


void boss_bethtilac::boss_bethtilacAI::DamageTaken(Unit *attacker, uint32 &damage)
{
    if (phase == PHASE_IDLE)
    {
        EnterCombat(attacker);
    }
}


void boss_bethtilac::boss_bethtilacAI::EnterEvadeMode()
{
    if (!combatCheckEnabled)
        return;

    // don't evade when players are on another floor
    if (summonCombatChecker && summonCombatChecker->SelectNearestTarget())
        return;

    // really evade
    ScriptedAI::EnterEvadeMode();

    if (summonCombatChecker)
    {
        summonCombatChecker->DespawnOrUnsummon();
        summonCombatChecker = NULL;
    }

    if (instance)
        instance->SetData(TYPE_BETHTILAC, FAIL);
}


void boss_bethtilac::boss_bethtilacAI::KilledUnit(Unit *victim)
{
}


void boss_bethtilac::boss_bethtilacAI::JustDied(Unit *killer)
{
    SpiderAI::JustDied(killer);
    me->RemoveAllAuras();

    if (instance)
        instance->SetData(TYPE_BETHTILAC, DONE);
}


void boss_bethtilac::boss_bethtilacAI::UpdateAI(const uint32 diff)
{
    if (!UpdateVictim())
    {
        if (me->isInCombat())
            SpiderAI::EnterEvadeMode();
        return;
    }

    if (phase == PHASE_1 && me->GetPositionZ() < webZPosition)
    {
        me->AttackStop();
        me->StopMoving();
        me->DeleteThreatList();
        me->NearTeleportTo(me->GetPositionX() + 1.0f, me->GetPositionY(), webZPosition + 1.0f, me->GetOrientation());
    }

    if (!IsInTransfer() && !me->IsNonMeleeSpellCasted(false))
        if (Unit const *pVictim = me->getVictim())
            if (me->IsWithinMeleeRange(pVictim))
                DoMeleeAttackIfReady();


    UpdateTimers(diff);


    if (phase != oldPhase)
        EnterPhase(phase);
}


void boss_bethtilac::boss_bethtilacAI::SetPhase(BethtilacPhases newPhase)
{
    oldPhase = phase;
    phase = newPhase;
}


void boss_bethtilac::boss_bethtilacAI::EnterPhase(BethtilacPhases newPhase)
{
    if (newPhase != PHASE_1)
        ClearTimers();

    phase = newPhase;
    oldPhase = newPhase;

    ScheduleEventsForPhase(newPhase);

    switch (newPhase)
    {
        case PHASE_IDLE:
            ScriptedAI::EnterEvadeMode();
            break;

        case PHASE_TRANSFER_1:
        {
            DebugOutput("phase transfer 1");

            MoveToFilament(MOVE_POINT_UP);
            break;
        }

        case PHASE_1:
        {
            DebugOutput("phase 1");

            me->AttackStop();
            me->DeleteThreatList();
            me->SetReactState(REACT_AGGRESSIVE);

            UnSummonFilament();

            me->SetInCombatWithZone();
            break;
        }

        case PHASE_TRANSFER_2:
        {
            DebugOutput("phase transfer 2");

            me->NearTeleportTo(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() - 1.0f, me->GetOrientation());
            SummonFilament();

            MoveToGround(MOVE_POINT_DOWN);
            break;
        }

        case PHASE_2:
        {
            DebugOutput("phase 2");

            me->AttackStop();
            me->DeleteThreatList();
            me->SetReactState(REACT_AGGRESSIVE);

            UnSummonFilament();

            DoZoneInCombat();

            break;
        }
    }
}


void boss_bethtilac::boss_bethtilacAI::ScheduleEventsForPhase(BethtilacPhases phase)
{
    if (phase != PHASE_IDLE && phase != PHASE_1)
        AddTimer(POWER_DECAY, 1000, true);

    switch (phase)
    {
        case PHASE_TRANSFER_1:
            AddTimer(TRANSFER_TIMEOUT, 10000, false);
            // timers for phase 1 should start in the moment of entering to combat (except "auto-cast" spells)
            AddTimer(SUMMON_FIRST_DRONE, 45000, false);
            AddTimer(SUMMON_SPINNERS, 12000, false);
            AddTimer(SUMMON_FIRST_SPIDERLINGS, 12500, false);
            break;
        case PHASE_1:
            AddTimer(SP_EMBER_FLARE, 6000, true);
            AddTimer(SP_VENOM_RAIN, 3000, true);
            break;
        case PHASE_TRANSFER_2:
            AddTimer(TRANSFER_TIMEOUT, 10000, false);
            break;
        case PHASE_2:
            AddTimer(SP_FRENZY, 5000, true);
            AddTimer(SP_EMBER_FLARE_2, 6000, true);
            AddTimer(SP_WIDOW_KISS, 32000, true);
            break;
    }
}


void boss_bethtilac::boss_bethtilacAI::ResetPower()
{
    SetMaxPower(8200);
    SetPower(8200);
    me->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER);
}


bool boss_bethtilac::boss_bethtilacAI::IsInTransfer()
{
    return phase == PHASE_TRANSFER_1 || phase == PHASE_TRANSFER_2;
}


void boss_bethtilac::boss_bethtilacAI::DoAction(const int32 event)
{
    switch (event)
    {
        case POWER_DECAY:
            DepletePower();
            if (phase == PHASE_1 && GetPower() == 0)
                DoSmolderingDevastation();
            break;

        case COMBAT_CHECK_ENABLE:
            combatCheckEnabled = true;
            break;

        case TRANSFER_TIMEOUT:
            //me->CastSpell(me, 265, false);      // area death (only cheaters can make this to happen)
            if (IsInTransfer())
                SetPhase(PHASE_IDLE);
            break;

        case SD_ENABLE:
            devastationEnabled = true;
            break;

        case SP_VENOM_RAIN:
            if (!me->getVictim() && !me->IsNonMeleeSpellCasted(false))
                me->CastSpell((Unit*)NULL, SPELL_VENOM_RAIN, false);
            break;

        case SP_EMBER_FLARE:
            if (me->getVictim() && !me->IsNonMeleeSpellCasted(false))
                me->CastSpell((Unit*)NULL, SPELL_EMBER_FLARE, false);
            break;

        case SUMMON_FIRST_DRONE:
            AddTimer(SUMMON_DRONE, 60000, true);
        case SUMMON_DRONE:
            SummonDrone();
            break;

        case SUMMON_SPINNERS:
            SummonSpinners();
            break;

        case SUMMON_FIRST_SPIDERLINGS:
            AddTimer(SUMMON_SPIDERLINGS, 30000, true);
        case SUMMON_SPIDERLINGS:
            SummonSpiderlings();
            break;

        case END_OF_PHASE_1:
            SetPhase(PHASE_TRANSFER_2);
            break;

        case SP_FRENZY:
            me->CastSpell(me, SPELL_FRENZY, false);
            break;

        case SP_EMBER_FLARE_2:
            me->CastSpell((Unit*)NULL, SPELL_EMBER_FLARE_2, false);
            break;

        case SP_WIDOW_KISS:
            me->CastSpell(me->getVictim(), SPELL_WIDOW_KISS, false);
            break;

        /*
        case EVENT_SPELLCLICK:  // called when someone in GM mode enters me
            break;

        default:
            __debugbreak();
            break;
            */
    }
}


void boss_bethtilac::boss_bethtilacAI::MovementInform(uint32 type, uint32 id)
{
    ScriptedAI::MovementInform(type, id);

    switch (id)
    {
        case MOVE_POINT_UP:
        {
            if (type == POINT_MOTION_TYPE)
            {
                SetPhase(PHASE_1);
                me->NearTeleportTo(me->GetPositionX(), me->GetPositionY(), webZPosition + 1.0f, me->GetOrientation(), false);
                me->SetFlying(false);
            }

            break;
        }

        case MOVE_POINT_DOWN:
        {
            if (type == POINT_MOTION_TYPE)
            {
                SetPhase(PHASE_2);
                me->SetReactState(REACT_AGGRESSIVE);
                DoZoneInCombat();
                me->SetFlying(false);
                me->SendMovementFlagUpdate();
            }
            break;
        }
    }
}


void boss_bethtilac::boss_bethtilacAI::SummonedCreatureDespawn(Creature *creature)
{
    SpiderAI::SummonedCreatureDespawn(creature);

    if (summonCombatChecker == creature)
        summonCombatChecker = NULL;
}


void boss_bethtilac::boss_bethtilacAI::MoveInLineOfSight(Unit *who)
{
    if (me->isInCombat() && /*!me->getVictim() &&*/
        !IsInTransfer())
    {
        if ( me->canAttack(who, false))
            AttackStart(who);
        else
        {
            if (me->IsFriendlyTo(who) && !me->IsNonMeleeSpellCasted(false))
                SpiderAI::MoveInLineOfSight(who);   // eat spiderlings
        }
    }
}


void boss_bethtilac::boss_bethtilacAI::AttackStart(Unit *victim)
{
    if (phase != PHASE_IDLE && !IsInTransfer() &&
        me->canAttack(victim, false) && (phase != PHASE_1 || victim->GetPositionZ() >= webZPosition))
    {
        ScriptedAI::AttackStart(victim);
    }
}


void boss_bethtilac::boss_bethtilacAI::DepletePower()
{
    ModifyPower(-100);
}


void boss_bethtilac::boss_bethtilacAI::DoSmolderingDevastation()
{
    if (devastationEnabled)
    {
        ShowWarnText("Beth'tilac smoldering body begins to flicker and combust!");

        devastationEnabled = false;          // disable it temporarily to avoid double cast
        AddTimer(SD_ENABLE, 10000, false);   // re-enable it after 10 seconds

        me->CastSpell(me, SPELL_SMOLDERING_DEVASTATION, false);    // has cast time - can't be flagged as triggered
        devastationCounter++;   // increase counter - after third one transfer to next phase

        if (devastationCounter == 3)
        {
            Spell *spell = me->GetCurrentSpell(CURRENT_GENERIC_SPELL);
            ASSERT (spell);     // should be this Smoldering Devastation

            AddTimer(END_OF_PHASE_1, spell->GetRemainingCastTime() + 200, false);
        }
        else
        {
            // next Cinderweb Spinners in 15 seconds after start of cast
            AddTimer(SUMMON_SPINNERS, 15000, false);
        }
    }
}


void boss_bethtilac::boss_bethtilacAI::ShowWarnText(const char *text)
{
    me->MonsterTextEmote(text, 0, true);
}


void boss_bethtilac::boss_bethtilacAI::SummonDrone()
{
    DebugOutput("summoning Cinderweb Drone");

    me->SummonCreature(NPC_CINDERWEB_DRONE, 103.176773f, 454.915924f, 86.966888f, 3.578207f,
                       TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000);
}


void boss_bethtilac::boss_bethtilacAI::SummonSpinners()
{
    ShowWarnText("Cinderweb Spinners dangle from above!");
    DebugOutput("summoning Cinderweb Spinners");

    for (uint8 i = 0; i < 6; i++)
        me->CastSpell(me, SPELL_SUMMON_SPINNER, true);
}


void boss_bethtilac::boss_bethtilacAI::SummonSpiderlings()
{
    ShowWarnText("Spiderlings have been roused from their nest!");
    DebugOutput("summoning Cinderweb Spiderlings");

    // WRONG! TOTO: more spiderlings, and at correct location
    for (int i = 0; i < 6; i++)
    {
        float angle = 2 * 3.14f / 6 * i;
        me->SummonCreature(NPC_CINDERWEB_SPIDERLING,
                           98.0f + cos(angle), 452.0f + sin(angle), 86, 0,
                           TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000);
    }
}



// vehicle id = 1652

/*
// required SQL queries

UPDATE
    creature_template
  SET
    ScriptName = 'boss_bethtilac',
    VehicleId = 1652
  WHERE
    entry = 52498;


UPDATE
    creature_template
  SET
    ScriptName = 'mob_cinderweb_drone',
    AIName = '',
    VehicleId = 1652,
    difficulty_entry_1 = 53582,
    difficulty_entry_2 = 53583,
    difficulty_entry_3 = 53584
  WHERE
    entry = 52581;


UPDATE
    creature_template
  SET
    ScriptName = 'mob_cinderweb_spinner',
    AIName = '',
    difficulty_entry_1 = 53599,
    difficulty_entry_2 = 53600,
    difficulty_entry_3 = 53601
  WHERE
    entry = 52524;


UPDATE
    creature_template
  SET
    ScriptName = 'mob_cinderweb_spiderling',
    AIName = '',
    difficulty_entry_1 = 53579,
    difficulty_entry_2 = 53580,
    difficulty_entry_3 = 53581
  WHERE
    entry = 52447;


INSERT INTO
  conditions
    (SourceTypeOrReferenceId, SourceEntry, ConditionTypeOrReference, ConditionValue1, ConditionValue2, Comment)
  VALUES
    (13,                      99411,       18,                       1,               52498,           'Beth\'tilac - Leech Venom'),
    (13,                      99304,       18,                       1,               52447,           'Beth\'tilac - Consume');

 */
