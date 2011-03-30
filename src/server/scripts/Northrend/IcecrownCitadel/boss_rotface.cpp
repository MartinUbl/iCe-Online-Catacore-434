/*
 * Copyright (C) 2008-2010 TrinityCore <http://www.trinitycore.org/>
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

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellAuras.h"
#include "icecrown_citadel.h"

// KNOWN BUGS:
// ~ No Slime Spray animation directly at target spot

enum eTexts
{
    SAY_PRECIOUS_DIES           = -1631094,
    SAY_AGGRO                   = -1631095,
    EMOTE_SLIME_SPRAY           = -1631096,
    SAY_SLIME_SPRAY             = -1631097,
    EMOTE_UNSTABLE_EXPLOSION    = -1631098,
    SAY_UNSTABLE_EXPLOSION      = -1631099,
    SAY_KILL_1                  = -1631100,
    SAY_KILL_2                  = -1631101,
    SAY_BERSERK                 = -1631102,
    SAY_DEATH                   = -1631103
};

enum eSpells
{
    // Rotface
    SPELL_SLIME_SPRAY                       = 69508,    // every 20 seconds
    SPELL_MUTATED_INFECTION                 = 69674,    // hastens every 1:30

    // Oozes
    SPELL_LITTLE_OOZE_COMBINE               = 69537,    // combine 2 Small Oozes
    SPELL_LARGE_OOZE_COMBINE                = 69552,    // combine 2 Large Oozes
    SPELL_LARGE_OOZE_BUFF_COMBINE           = 69611,    // combine Large and Small Ooze
    SPELL_OOZE_MERGE                        = 69889,    // 2 Small Oozes summon a Large Ooze
    SPELL_WEAK_RADIATING_OOZE               = 69750,    // passive damage aura - small
    SPELL_RADIATING_OOZE                    = 69760,    // passive damage aura - large
    SPELL_UNSTABLE_OOZE                     = 69558,    // damage boost and counter for explosion
    SPELL_GREEN_ABOMINATION_HITTIN__YA_PROC = 70001,    // prevents getting hit by infection
    SPELL_UNSTABLE_OOZE_EXPLOSION           = 69839,
    SPELL_STICKY_OOZE                       = 69774,
    SPELL_UNSTABLE_OOZE_EXPLOSION_TRIGGER   = 69832,

    // Precious
    SPELL_MORTAL_WOUND                      = 71127,
    SPELL_DECIMATE                          = 71123
};

#define MUTATED_INFECTION RAID_MODE<int32>(69674,71224,73022,73023)

enum eEvents
{
    EVENT_SLIME_SPRAY       = 1,
    EVENT_HASTEN_INFECTIONS = 2,
    EVENT_MUTATED_INFECTION = 3,

    EVENT_DECIMATE          = 4,
    EVENT_MORTAL_WOUND      = 5,

    EVENT_STICKY_OOZE       = 6,
    EVENT_UNSTABLE_DESPAWN  = 7
};

class boss_rotface : public CreatureScript
{
    public:
        boss_rotface() : CreatureScript("boss_rotface") { }

        struct boss_rotfaceAI : public BossAI
        {
            boss_rotfaceAI(Creature* pCreature) : BossAI(pCreature, DATA_ROTFACE)
            {
                infectionStage = 0;
                infectionCooldown = 14000;
            }

            void InitializeAI()
            {
                if (!instance || static_cast<InstanceMap*>(me->GetMap())->GetScriptId() != GetScriptId(ICCSCRIPTNAME))
                    me->IsAIEnabled = false;
                else if (!me->isDead())
                    Reset();
            }

            void Reset()
            {
                events.Reset();
                events.ScheduleEvent(EVENT_SLIME_SPRAY, 20000);
                events.ScheduleEvent(EVENT_HASTEN_INFECTIONS, 90000);
                events.ScheduleEvent(EVENT_MUTATED_INFECTION, 14000);
                infectionStage = 0;
                infectionCooldown = 14000;
                summons.DespawnAll();

                instance->SetBossState(DATA_ROTFACE, NOT_STARTED);
            }

            void EnterCombat(Unit* /*who*/)
            {
                DoScriptText(SAY_AGGRO, me);
                if (Creature* professor = Unit::GetCreature(*me, instance->GetData64(DATA_PROFESSOR_PUTRICIDE)))
                    professor->AI()->DoAction(ACTION_ROTFACE_COMBAT);

                DoZoneInCombat(me);
            }

            void JustDied(Unit* /*killer*/)
            {
                DoScriptText(SAY_DEATH, me);
                instance->SetBossState(DATA_ROTFACE, DONE);
                if (Creature* professor = Unit::GetCreature(*me, instance->GetData64(DATA_PROFESSOR_PUTRICIDE)))
                    professor->AI()->DoAction(ACTION_ROTFACE_DEATH);
            }

            void JustReachedHome()
            {
                instance->SetBossState(DATA_ROTFACE, FAIL);
                instance->SetData(DATA_OOZE_DANCE_ACHIEVEMENT, uint32(true));   // reset
            }

            void KilledUnit(Unit *victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    DoScriptText(RAND(SAY_KILL_1, SAY_KILL_2), me);
            }

            void EnterEvadeMode()
            {
                ScriptedAI::EnterEvadeMode();
                if (Creature* professor = Unit::GetCreature(*me, instance->GetData64(DATA_PROFESSOR_PUTRICIDE)))
                    professor->AI()->EnterEvadeMode();
            }

            void SpellHitTarget(Unit* /*target*/, SpellEntry const* spell)
            {
                if (spell->Id == SPELL_SLIME_SPRAY)
                    DoScriptText(SAY_SLIME_SPRAY, me);
            }

            void MoveInLineOfSight(Unit* /*who*/)
            {
                // don't enter combat
            }

            Unit* GetAuraEffectTriggerTarget(uint32 spellId, uint8 /*effIndex*/)
            {
                if (spellId == SPELL_SLIME_SPRAY)
                {
                    for (std::list<uint64>::iterator itr = summons.begin(); itr != summons.end();)
                    {
                        Creature *summon = Unit::GetCreature(*me, *itr);
                        if (!summon)
                            summons.erase(itr++);
                        else if (summon->GetEntry() == NPC_OOZE_SPRAY_STALKER)
                            return summon;
                        else
                            ++itr;
                    }
                }

                return NULL;
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim() || !CheckInRoom())
                    return;

                events.Update(diff);

                if (me->hasUnitState(UNIT_STAT_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SLIME_SPRAY:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                            {
                                Position pos;
                                target->GetPosition(&pos);
                                DoSummon(NPC_OOZE_SPRAY_STALKER, pos, 8000, TEMPSUMMON_TIMED_DESPAWN);
                                DoScriptText(EMOTE_SLIME_SPRAY, me);
                                DoCastAOE(SPELL_SLIME_SPRAY);
                            }
                            events.ScheduleEvent(EVENT_SLIME_SPRAY, 20000);
                            break;
                        case EVENT_HASTEN_INFECTIONS:
                            if (infectionStage < 4)
                            {
                                infectionCooldown -= 2000;
                                events.ScheduleEvent(EVENT_HASTEN_INFECTIONS, 90000);
                            }
                            break;
                        case EVENT_MUTATED_INFECTION:
                        {
                            Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true, -MUTATED_INFECTION);
                            if (!target)
                                target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true, -MUTATED_INFECTION);
                            if (target)
                                me->CastCustomSpell(SPELL_MUTATED_INFECTION, SPELLVALUE_MAX_TARGETS, 1, target, false);
                            events.ScheduleEvent(EVENT_MUTATED_INFECTION, infectionCooldown);
                            break;
                        }
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            uint8 infectionStage;
            uint32 infectionCooldown;
        };

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new boss_rotfaceAI(pCreature);
        }
};

class npc_little_ooze : public CreatureScript
{
    public:
        npc_little_ooze() : CreatureScript("npc_little_ooze") { }

        struct npc_little_oozeAI : public ScriptedAI
        {
            npc_little_oozeAI(Creature* pCreature) : ScriptedAI(pCreature)
            {
            }

            void IsSummonedBy(Unit* summoner)
            {
                DoCast(me, SPELL_LITTLE_OOZE_COMBINE, true);
                DoCast(me, SPELL_WEAK_RADIATING_OOZE, true);
                events.ScheduleEvent(EVENT_STICKY_OOZE, 5000);
                me->AddThreat(summoner, 500000.0f);
                // register in Rotface's summons - not summoned with Rotface as owner
                if (InstanceScript* instance = me->GetInstanceScript())
                    if (Creature* rotface = Unit::GetCreature(*me, instance->GetData64(DATA_ROTFACE)))
                        rotface->AI()->JustSummoned(me);
            }

            void JustDied(Unit* /*killer*/)
            {
                if (TempSummon* summ = me->ToTempSummon())
                {
                    summ->UnSummon();
                    if (InstanceScript* instance = me->GetInstanceScript())
                        if (Creature* rotface = Unit::GetCreature(*me, instance->GetData64(DATA_ROTFACE)))
                            rotface->AI()->SummonedCreatureDespawn(me);
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (events.ExecuteEvent() == EVENT_STICKY_OOZE)
                {
                    DoCastVictim(SPELL_STICKY_OOZE);
                    events.ScheduleEvent(EVENT_STICKY_OOZE, 15000);
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap events;
        };

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_little_oozeAI(pCreature);
        }
};

class npc_big_ooze : public CreatureScript
{
    public:
        npc_big_ooze() : CreatureScript("npc_big_ooze") { }

        struct npc_big_oozeAI : public ScriptedAI
        {
            npc_big_oozeAI(Creature* pCreature) : ScriptedAI(pCreature), bExploded(false)
            {
            }

            void IsSummonedBy(Unit* /*summoner*/)
            {
                DoCast(me, SPELL_LARGE_OOZE_COMBINE, true);
                DoCast(me, SPELL_LARGE_OOZE_BUFF_COMBINE, true);
                DoCast(me, SPELL_RADIATING_OOZE, true);
                DoCast(me, SPELL_UNSTABLE_OOZE, true);
                DoCast(me, SPELL_GREEN_ABOMINATION_HITTIN__YA_PROC, true);
                events.ScheduleEvent(EVENT_STICKY_OOZE, 5000);
                // register in Rotface's summons - not summoned with Rotface as owner
                if (InstanceScript* instance = me->GetInstanceScript())
                    if (Creature* rotface = Unit::GetCreature(*me, instance->GetData64(DATA_ROTFACE)))
                        rotface->AI()->JustSummoned(me);
            }

            void JustDied(Unit* /*killer*/)
            {
                if (TempSummon* summ = me->ToTempSummon())
                {
                    summ->UnSummon();
                    if (InstanceScript* instance = me->GetInstanceScript())
                        if (Creature* rotface = Unit::GetCreature(*me, instance->GetData64(DATA_ROTFACE)))
                            rotface->AI()->SummonedCreatureDespawn(me);
                }
            }

            void DoAction(const int32 action)
            {
                if (action == EVENT_STICKY_OOZE)
                    events.CancelEvent(EVENT_STICKY_OOZE);
                else if (action == EVENT_UNSTABLE_DESPAWN)
                {
                    bExploded = true;
                    me->RemoveAllAuras();
                    me->SetVisibility(VISIBILITY_OFF);
                    events.Reset();
                    events.ScheduleEvent(EVENT_UNSTABLE_DESPAWN, 60000);
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_STICKY_OOZE:
                            DoCastVictim(SPELL_STICKY_OOZE);
                            events.ScheduleEvent(EVENT_STICKY_OOZE, 15000);
                            break;
                        case EVENT_UNSTABLE_DESPAWN:
                            me->Kill(me);
                            break;
                        default:
                            break;
                    }
                }

                if (!bExploded)
                    DoMeleeAttackIfReady();
            }

        private:
            EventMap events;
            bool bExploded;
        };

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_big_oozeAI(pCreature);
        }
};

class npc_precious_icc : public CreatureScript
{
    public:
        npc_precious_icc() : CreatureScript("npc_precious_icc") { }

        struct npc_precious_iccAI : public ScriptedAI
        {
            npc_precious_iccAI(Creature* pCreature) : ScriptedAI(pCreature)
            {
                pInstance = pCreature->GetInstanceScript();
            }

            void Reset()
            {
                events.Reset();
                events.ScheduleEvent(EVENT_DECIMATE, urand(20000, 25000));
                events.ScheduleEvent(EVENT_MORTAL_WOUND, urand(3000, 7000));
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->hasUnitState(UNIT_STAT_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_DECIMATE:
                            DoCastVictim(SPELL_DECIMATE);
                            events.ScheduleEvent(EVENT_DECIMATE, urand(20000, 25000));
                            break;
                        case EVENT_MORTAL_WOUND:
                            DoCastVictim(SPELL_MORTAL_WOUND);
                            events.ScheduleEvent(EVENT_MORTAL_WOUND, urand(10000, 12500));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

            void JustDied(Unit* /*who*/)
            {
                uint64 rotfaceGUID = pInstance ? pInstance->GetData64(DATA_ROTFACE) : 0;
                if (Creature *rotface = Unit::GetCreature(*me, rotfaceGUID))
                    if (rotface->isAlive())
                        DoScriptText(SAY_PRECIOUS_DIES, rotface);
            }

        private:
            EventMap events;
            InstanceScript* pInstance;
        };

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_precious_iccAI(pCreature);
        }
};

class spell_rotface_ooze_flood : public SpellScriptLoader
{
    public:
        spell_rotface_ooze_flood() : SpellScriptLoader("spell_rotface_ooze_flood") { }

        class spell_rotface_ooze_flood_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_ooze_flood_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (!GetHitUnit())
                    return;
                std::list<Creature*> list;
                GetHitUnit()->GetCreatureListWithEntryInGrid(list, GetHitUnit()->GetEntry(), 12.5f);
                list.sort(Trinity::ObjectDistanceOrderPred(GetHitUnit()));
                GetHitUnit()->CastSpell(list.back(), GetEffectValue(), false, NULL, NULL, GetOriginalCaster() ? GetOriginalCaster()->GetGUID() : 0);
            }

            void Register()
            {
                OnEffect += SpellEffectFn(spell_rotface_ooze_flood_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rotface_ooze_flood_SpellScript();
        }
};

class spell_rotface_little_ooze_combine : public SpellScriptLoader
{
    public:
        spell_rotface_little_ooze_combine() : SpellScriptLoader("spell_rotface_little_ooze_combine") { }

        class spell_rotface_little_ooze_combine_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_little_ooze_combine_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (!(GetHitUnit() && GetHitUnit()->isAlive()))
                    return;

                GetCaster()->RemoveAurasDueToSpell(SPELL_LITTLE_OOZE_COMBINE);
                GetHitUnit()->RemoveAurasDueToSpell(SPELL_LITTLE_OOZE_COMBINE);
                GetHitUnit()->CastSpell(GetCaster(), SPELL_OOZE_MERGE, true);
                if (TempSummon* summ = GetHitUnit()->ToTempSummon())
                    summ->UnSummon();
                else
                    GetHitCreature()->ForcedDespawn();
            }

            void Register()
            {
                OnEffect += SpellEffectFn(spell_rotface_little_ooze_combine_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rotface_little_ooze_combine_SpellScript();
        }
};

class spell_rotface_large_ooze_combine : public SpellScriptLoader
{
    public:
        spell_rotface_large_ooze_combine() : SpellScriptLoader("spell_rotface_large_ooze_combine") { }

        class spell_rotface_large_ooze_combine_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_large_ooze_combine_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (!(GetHitUnit() && GetHitUnit()->isAlive()))
                    return;

                if (Aura* unstable = GetCaster()->GetAura(SPELL_UNSTABLE_OOZE))
                {
                    if (Aura* targetAura = GetHitUnit()->GetAura(SPELL_UNSTABLE_OOZE))
                        unstable->ModStackAmount(targetAura->GetStackAmount());
                    else
                        unstable->ModStackAmount(1);

                    // no idea why, but this does not trigger explosion on retail (only small+large do)
                }

                // just for safety
                GetHitUnit()->RemoveAurasDueToSpell(SPELL_LARGE_OOZE_BUFF_COMBINE);
                GetHitUnit()->RemoveAurasDueToSpell(SPELL_LARGE_OOZE_COMBINE);

                if (TempSummon* summ = GetHitUnit()->ToTempSummon())
                    summ->UnSummon();
                else if (GetHitCreature())
                    GetHitCreature()->ForcedDespawn();
            }

            void Register()
            {
                OnEffect += SpellEffectFn(spell_rotface_large_ooze_combine_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rotface_large_ooze_combine_SpellScript();
        }
};

class spell_rotface_large_ooze_buff_combine : public SpellScriptLoader
{
    public:
        spell_rotface_large_ooze_buff_combine() : SpellScriptLoader("spell_rotface_large_ooze_buff_combine") { }

        class spell_rotface_large_ooze_buff_combine_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_large_ooze_buff_combine_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (!(GetHitUnit() && GetHitUnit()->isAlive()))
                    return;

                if (Aura* unstable = GetCaster()->GetAura(SPELL_UNSTABLE_OOZE))
                {
                    uint8 newStack = unstable->GetStackAmount()+1;
                    unstable->SetStackAmount(newStack);

                    // explode!
                    if (newStack >= 5)
                    {
                        GetCaster()->RemoveAurasDueToSpell(SPELL_LARGE_OOZE_BUFF_COMBINE);
                        GetCaster()->RemoveAurasDueToSpell(SPELL_LARGE_OOZE_COMBINE);
                        DoScriptText(EMOTE_UNSTABLE_EXPLOSION, GetCaster());
                        if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                            if (Creature* rotface = Unit::GetCreature(*GetCaster(), instance->GetData64(DATA_ROTFACE)))
                                if (rotface->isAlive())
                                    DoScriptText(SAY_UNSTABLE_EXPLOSION, rotface);

                        if (Creature* cre = GetCaster()->ToCreature())
                            cre->AI()->DoAction(EVENT_STICKY_OOZE);
                        GetCaster()->CastSpell(GetCaster(), SPELL_UNSTABLE_OOZE_EXPLOSION, false, NULL, NULL, GetCaster()->GetGUID());
                        if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                            instance->SetData(DATA_OOZE_DANCE_ACHIEVEMENT, uint32(false));
                    }
                }

                if (TempSummon* summ = GetHitUnit()->ToTempSummon())
                    summ->UnSummon();
                else if (GetHitCreature())
                    GetHitCreature()->ForcedDespawn();
            }

            void Register()
            {
                OnEffect += SpellEffectFn(spell_rotface_large_ooze_buff_combine_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rotface_large_ooze_buff_combine_SpellScript();
        }
};

class spell_rotface_unstable_ooze_explosion_init : public SpellScriptLoader
{
    public:
        spell_rotface_unstable_ooze_explosion_init() : SpellScriptLoader("spell_rotface_unstable_ooze_explosion_init") { }

        class spell_rotface_unstable_ooze_explosion_init_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_unstable_ooze_explosion_init_SpellScript);

            bool Validate(SpellEntry const* /*spell*/)
            {
                if (!sSpellStore.LookupEntry(SPELL_UNSTABLE_OOZE_EXPLOSION_TRIGGER))
                    return false;
                return true;
            }

            void HandleCast(SpellEffIndex effIndex)
            {
                PreventHitEffect(effIndex);
                if (!GetHitUnit())
                    return;

                float x, y, z;
                GetHitUnit()->GetPosition(x, y, z);
                Creature* dummy = GetCaster()->SummonCreature(NPC_UNSTABLE_EXPLOSION_STALKER, x, y, z, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 60000);
                GetCaster()->CastSpell(dummy, SPELL_UNSTABLE_OOZE_EXPLOSION_TRIGGER, true);
            }

            void Register()
            {
                OnEffect += SpellEffectFn(spell_rotface_unstable_ooze_explosion_init_SpellScript::HandleCast, EFFECT_0, SPELL_EFFECT_FORCE_CAST);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rotface_unstable_ooze_explosion_init_SpellScript();
        }
};

class spell_rotface_unstable_ooze_explosion : public SpellScriptLoader
{
    public:
        spell_rotface_unstable_ooze_explosion() : SpellScriptLoader("spell_rotface_unstable_ooze_explosion") { }

        class spell_rotface_unstable_ooze_explosion_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rotface_unstable_ooze_explosion_SpellScript);

            void CheckTarget(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(EFFECT_0);
                if (!GetTargetUnit())
                    return;

                uint32 triggered_spell_id = GetSpellInfo()->EffectTriggerSpell[effIndex];

                float x, y, z;
                GetTargetUnit()->GetPosition(x, y, z);
                // let Rotface handle the cast - caster dies before this executes
                if (InstanceScript* script = GetTargetUnit()->GetInstanceScript())
                    if (Creature* rotface = script->instance->GetCreature(script->GetData64(DATA_ROTFACE)))
                        rotface->CastSpell(x, y, z, triggered_spell_id, true, NULL, NULL, GetCaster()->GetGUID(), GetTargetUnit());
            }

            void Register()
            {
                OnEffect += SpellEffectFn(spell_rotface_unstable_ooze_explosion_SpellScript::CheckTarget, EFFECT_0, SPELL_EFFECT_TRIGGER_MISSILE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rotface_unstable_ooze_explosion_SpellScript();
        }
};

class spell_rotface_unstable_ooze_explosion_suicide : public SpellScriptLoader
{
    public:
        spell_rotface_unstable_ooze_explosion_suicide() : SpellScriptLoader("spell_rotface_unstable_ooze_explosion_suicide") { }

        class spell_rotface_unstable_ooze_explosion_suicide_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_rotface_unstable_ooze_explosion_suicide_AuraScript);

            void DespawnSelf(AuraEffect const* /*aurEff*/)
            {
                PreventDefaultAction();
                Unit* target = GetTarget();
                if (target->GetTypeId() != TYPEID_UNIT)
                    return;

                target->ToCreature()->AI()->DoAction(EVENT_UNSTABLE_DESPAWN);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_rotface_unstable_ooze_explosion_suicide_AuraScript::DespawnSelf, EFFECT_2, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_rotface_unstable_ooze_explosion_suicide_AuraScript();
        }
};

void AddSC_boss_rotface()
{
    new boss_rotface();
    new npc_little_ooze();
    new npc_big_ooze();
    new npc_precious_icc();
    new spell_rotface_ooze_flood();
    new spell_rotface_little_ooze_combine();
    new spell_rotface_large_ooze_combine();
    new spell_rotface_large_ooze_buff_combine();
    new spell_rotface_unstable_ooze_explosion_init();
    new spell_rotface_unstable_ooze_explosion();
    new spell_rotface_unstable_ooze_explosion_suicide();
}
