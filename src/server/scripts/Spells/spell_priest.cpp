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

/*
 * Scripts for spells with SPELLFAMILY_PRIEST and SPELLFAMILY_GENERIC spells used by priest players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_pri_".
 */

#include "ScriptPCH.h"
#include "SpellAuraEffects.h"

enum PriestSpells
{
    PRIEST_SPELL_PENANCE_R1                      = 47540,
    PRIEST_SPELL_PENANCE_R1_DAMAGE               = 47758,
    PRIEST_SPELL_PENANCE_R1_HEAL                 = 47757,
};

// Guardian Spirit
class spell_pri_guardian_spirit : public SpellScriptLoader
{
public:
    spell_pri_guardian_spirit() : SpellScriptLoader("spell_pri_guardian_spirit") { }

    class spell_pri_guardian_spirit_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_pri_guardian_spirit_AuraScript);

        uint32 healPct;

        enum Spell
        {
            PRI_SPELL_GUARDIAN_SPIRIT_HEAL = 48153,
        };

        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            return sSpellStore.LookupEntry(PRI_SPELL_GUARDIAN_SPIRIT_HEAL);
        }

        bool Load()
        {
            healPct = SpellMgr::CalculateSpellEffectAmount(GetSpellProto(), EFFECT_1);
            return true;
        }

        void CalculateAmount(AuraEffect const * /*aurEff*/, int32 & amount, bool & canBeRecalculated)
        {
            // Set absorbtion amount to unlimited
            amount = -1;
        }

        void Absorb(AuraEffect * aurEff, DamageInfo & dmgInfo, uint32 & absorbAmount)
        {
            Unit * target = GetTarget();
            if (dmgInfo.GetDamage() < target->GetHealth())
                return;

            int32 healAmount = int32(target->CountPctFromMaxHealth(healPct));
            // remove the aura now, we don't want 40% healing bonus
            Remove(AURA_REMOVE_BY_ENEMY_SPELL);
            target->CastCustomSpell(target, PRI_SPELL_GUARDIAN_SPIRIT_HEAL, &healAmount, NULL, NULL, true);
            absorbAmount = dmgInfo.GetDamage();
        }

        void Register()
        {
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_pri_guardian_spirit_AuraScript::CalculateAmount, EFFECT_1, SPELL_AURA_SCHOOL_ABSORB);
            OnEffectAbsorb += AuraEffectAbsorbFn(spell_pri_guardian_spirit_AuraScript::Absorb, EFFECT_1);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_pri_guardian_spirit_AuraScript();
    }
};

class spell_pri_mana_burn : public SpellScriptLoader
{
    public:
        spell_pri_mana_burn() : SpellScriptLoader("spell_pri_mana_burn") { }

        class spell_pri_mana_burn_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_mana_burn_SpellScript)
            bool Validate(SpellEntry const * /*spellEntry*/)
            {
                return true;
            }

            void HandleAfterHit()
            {
                Unit * unitTarget = GetHitUnit();
                if (!unitTarget)
                    return;

                unitTarget->RemoveAurasWithMechanic((1 << MECHANIC_FEAR) | (1 << MECHANIC_POLYMORPH));
            }

            void Register()
            {
                AfterHit += SpellHitFn(spell_pri_mana_burn_SpellScript::HandleAfterHit);
            }
        };

        SpellScript * GetSpellScript() const
        {
            return new spell_pri_mana_burn_SpellScript;
        }
};

class spell_pri_penance : public SpellScriptLoader
{
    public:
        spell_pri_penance() : SpellScriptLoader("spell_pri_penance") { }

        class spell_pri_penance_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_penance_SpellScript)
            bool Validate(SpellEntry const * spellEntry)
            {
                if (!sSpellStore.LookupEntry(PRIEST_SPELL_PENANCE_R1))
                    return false;
                // can't use other spell than this penance due to spell_ranks dependency
                if (sSpellMgr->GetFirstSpellInChain(PRIEST_SPELL_PENANCE_R1) != sSpellMgr->GetFirstSpellInChain(spellEntry->Id))
                    return false;

                uint8 rank = sSpellMgr->GetSpellRank(spellEntry->Id);
                if (!sSpellMgr->GetSpellWithRank(PRIEST_SPELL_PENANCE_R1_DAMAGE, rank, true))
                    return false;
                if (!sSpellMgr->GetSpellWithRank(PRIEST_SPELL_PENANCE_R1_HEAL, rank, true))
                    return false;

                return true;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit *unitTarget = GetHitUnit();
                if (!unitTarget || !unitTarget->isAlive())
                    return;

                Unit *caster = GetCaster();

                uint8 rank = sSpellMgr->GetSpellRank(GetSpellInfo()->Id);

                if (caster->IsFriendlyTo(unitTarget))
                {
                    caster->CastSpell(unitTarget, sSpellMgr->GetSpellWithRank(PRIEST_SPELL_PENANCE_R1_HEAL, rank), false, 0);
                    if (caster->HasAura(89911)) // T11 4p bonus
                        caster->CastSpell(caster, 89913, true);
                }
                else
                    caster->CastSpell(unitTarget, sSpellMgr->GetSpellWithRank(PRIEST_SPELL_PENANCE_R1_DAMAGE, rank), false, 0);
            }

            void Register()
            {
                // add dummy effect spell handler to Penance
                OnEffect += SpellEffectFn(spell_pri_penance_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript *GetSpellScript() const
        {
            return new spell_pri_penance_SpellScript;
        }
};

// Reflective Shield
class spell_pri_reflective_shield_trigger : public SpellScriptLoader
{
public:
    spell_pri_reflective_shield_trigger() : SpellScriptLoader("spell_pri_reflective_shield_trigger") { }

    class spell_pri_reflective_shield_trigger_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_pri_reflective_shield_trigger_AuraScript);

        enum Spells
        {
            SPELL_PRI_REFLECTIVE_SHIELD_TRIGGERED = 33619,
            SPELL_PRI_REFLECTIVE_SHIELD_R1 = 33201,
            SPELL_PRI_REFLECTIVE_SHIELD_R2 = 33202,
        };

        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            return sSpellStore.LookupEntry(SPELL_PRI_REFLECTIVE_SHIELD_TRIGGERED) && sSpellStore.LookupEntry(SPELL_PRI_REFLECTIVE_SHIELD_R1) && sSpellStore.LookupEntry(SPELL_PRI_REFLECTIVE_SHIELD_R2);
        }

        void Trigger(AuraEffect * aurEff, DamageInfo & dmgInfo, uint32 & absorbAmount)
        {
            Unit * target = GetTarget();
            if (dmgInfo.GetAttacker() == target)
                return;
            Unit * caster = GetCaster();
            if (!caster)
                return;

            AuraEffect* talentAurEff = NULL;
            if(!(talentAurEff = target->GetAuraEffect(SPELL_PRI_REFLECTIVE_SHIELD_R1, EFFECT_0))) // rank #1
                talentAurEff = target->GetAuraEffect(SPELL_PRI_REFLECTIVE_SHIELD_R2, EFFECT_0);   // rank #2

            if(talentAurEff)
            {
                // Cast damage spell on the attacker
                int32 bp = CalculatePctN(absorbAmount, talentAurEff->GetAmount());
                target->CastCustomSpell(dmgInfo.GetAttacker(), SPELL_PRI_REFLECTIVE_SHIELD_TRIGGERED, &bp, NULL, NULL, true, NULL, aurEff);
            }
        }

        void Register()
        {
             AfterEffectAbsorb += AuraEffectAbsorbFn(spell_pri_reflective_shield_trigger_AuraScript::Trigger, EFFECT_0);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_pri_reflective_shield_trigger_AuraScript();
    }
};

// Cure Disease
// INSERT INTO `spell_script_names` VALUES (528, "spell_pri_cure_disease");
class spell_pri_cure_disease : public SpellScriptLoader
{
    public:
        spell_pri_cure_disease() : SpellScriptLoader("spell_pri_cure_disease") { }

        class spell_pri_cure_disease_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_cure_disease_SpellScript);

            bool Validate(SpellEntry const * spellEntry)
            {
                return true;
            }

            void HandleDispel(SpellEffIndex /*effIndex*/)
            {
                // Body and Soul
                Unit* caster = GetCaster();
                Unit* target = GetHitUnit();
                if(caster && caster == target)
                {
                    if(caster->HasAura(64129))                       // rank #2
                        caster->CastSpell(caster, 64136, true);      // Remove poison effect
                    else if (caster->HasAura(64127))                 // rank #1
                    {
                        if(urand(0,1))                               // 50% chance
                            caster->CastSpell(caster, 64136, true);  // Remove poison effect
                    }
                }
            }

            void Register()
            {
                // Add handler to the Cure Disease SPELL_EFFECT_DISPEL
                OnEffect += SpellEffectFn(spell_pri_cure_disease_SpellScript::HandleDispel, EFFECT_0, SPELL_EFFECT_DISPEL);
            }
        };

        SpellScript *GetSpellScript() const
        {
            return new spell_pri_cure_disease_SpellScript;
        }
};

class mob_shadowy_apparition: public CreatureScript
{
    public:
        mob_shadowy_apparition(): CreatureScript("mob_shadowy_apparition") {};

        struct mob_shadowy_apparitionAI: public ScriptedAI
        {
            mob_shadowy_apparitionAI(Creature* c): ScriptedAI(c)
            {
            }

            void UpdateAI(const uint32 diff)
            {
                // If we doesn't have target, kill us
                if (!UpdateVictim())
                {
                    me->Kill(me);
                    me->ForcedDespawn(100);
                    return;
                }
                // If we are not moving towards our target, do it
                if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() != CHASE_MOTION_TYPE)
                    me->GetMotionMaster()->MoveChase(me->getVictim());

                if (me->isAttackReady())
                {
                    // If we are within range melee the target
                    if (me->IsWithinMeleeRange(me->getVictim()))
                    {
                        me->CastSpell(me, 87529, true);
                        if (me->GetCharmerOrOwnerPlayerOrPlayerItself())
                        {
                            me->CastSpell(me->getVictim(), 87532, true, 0, 0, me->GetCharmerOrOwnerPlayerOrPlayerItself()->GetGUID());
                            me->Kill(me);
                            me->ForcedDespawn(100);
                        }
                        me->resetAttackTimer();
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new mob_shadowy_apparitionAI(c);
        }
};

class mob_cauterizing_flame: public CreatureScript
{
    public:
        mob_cauterizing_flame(): CreatureScript("mob_cauterizing_flame") {};

        struct mob_cauterizing_flameAI: public ScriptedAI
        {
            mob_cauterizing_flameAI(Creature* c): ScriptedAI(c)
            {
                me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_DISABLE_MOVE|UNIT_FLAG_NON_ATTACKABLE);
                if (TempSummon * temp = me->ToTempSummon())
                {
                    if (temp->GetSummoner() && temp->GetSummoner()->ToPlayer())
                    {
                        Player * p = me->ToTempSummon()->GetSummoner()->ToPlayer();
                        if (p->IsInWorld())
                            me->SetUInt64Value(UNIT_FIELD_SUMMONEDBY, p->GetGUID());
                        me->CastSpell(me, 99153, true, 0, 0, p->GetGUID());
                    }
                }
                me->SetUInt32Value(UNIT_CREATED_BY_SPELL, 99136);
                me->CastSpell(me,102129,true); // Visual
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new mob_cauterizing_flameAI(c);
        }
};

class spell_pri_spirit_of_redemption : public SpellScriptLoader
{
public:
    spell_pri_spirit_of_redemption() : SpellScriptLoader("spell_pri_spirit_of_redemption") { }

    class spell_pri_spirit_of_redemption_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_pri_spirit_of_redemption_AuraScript);

        enum Spell
        {
            PRI_SPELL_SPIRIT_OF_REDEMPTION_SRC = 20711,
            PRI_SPELL_SPIRIT_OF_REDEMPTION_TRIGGER_0 = 27827, // shapeshift   + waterbreath             + heal self
            PRI_SPELL_SPIRIT_OF_REDEMPTION_TRIGGER_1 = 27792, // unattackable + zero mana cost          + root
            PRI_SPELL_SPIRIT_OF_REDEMPTION_TRIGGER_2 = 27795, // pacify       + spiritofredemption aura + interrupt regen
        };

        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            return sSpellStore.LookupEntry(PRI_SPELL_SPIRIT_OF_REDEMPTION_SRC);
        }

        bool Load()
        {
            return GetUnitOwner()->ToPlayer();
        }

        void CalculateAmount(AuraEffect const * /*aurEff*/, int32 & amount, bool & canBeRecalculated)
        {
            // Set absorbtion amount to unlimited
            amount = -1;
        }

        void Absorb(AuraEffect * aurEff, DamageInfo & dmgInfo, uint32 & absorbAmount)
        {
            Unit * target = GetTarget();
            if (dmgInfo.GetDamage() < target->GetHealth() || target->GetTypeId() != TYPEID_PLAYER)
                return;

            uint32 ressSpellId = target->GetUInt32Value(PLAYER_SELF_RES_SPELL);
            if (!ressSpellId)
                ressSpellId = target->ToPlayer()->GetResurrectionSpellId();
            //Remove all expected to remove at death auras (most important negative case like DoT or periodic triggers)
            target->RemoveAllAurasOnDeath();
            // restore for use at real death
            target->SetUInt32Value(PLAYER_SELF_RES_SPELL, ressSpellId);

            target->CastSpell(target, PRI_SPELL_SPIRIT_OF_REDEMPTION_TRIGGER_0, true);
            target->CastSpell(target, PRI_SPELL_SPIRIT_OF_REDEMPTION_TRIGGER_1, true);
            target->CastSpell(target, PRI_SPELL_SPIRIT_OF_REDEMPTION_TRIGGER_2, true);

            absorbAmount = dmgInfo.GetDamage();
        }

        void Register()
        {
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_pri_spirit_of_redemption_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
            OnEffectAbsorb += AuraEffectAbsorbFn(spell_pri_spirit_of_redemption_AuraScript::Absorb, EFFECT_0);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_pri_spirit_of_redemption_AuraScript();
    }
};

class spell_priest_inner_sanctum : public SpellScriptLoader
{
public:
    spell_priest_inner_sanctum() : SpellScriptLoader("spell_priest_inner_sanctum") { }

    class spell_priest_inner_sanctum_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_priest_inner_sanctum_AuraScript);

        enum Talents
        {
            INNER_SANCTUM_R1    = 14747,
            INNER_SANCTUM_R2    = 14770,
            INNER_SANCTUM_R3    = 14771,
            SPELL_WARDING       = 91724,
        };

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* owner = GetOwner()->ToPlayer();
            if (!owner)
                return;

            owner->RemoveAurasDueToSpell(SPELL_WARDING);
        }

        void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* target = GetTarget();
            if(target == NULL)
                return;

            if(target->HasAura(INNER_SANCTUM_R1))
                target->CastCustomSpell(SPELL_WARDING, SPELLVALUE_BASE_POINT0, -2, target, true, NULL,NULL);

            if(target->HasAura(INNER_SANCTUM_R2))
                target->CastCustomSpell(SPELL_WARDING, SPELLVALUE_BASE_POINT0, -4, target, true, NULL,NULL);

            if(target->HasAura(INNER_SANCTUM_R3))
                target->CastCustomSpell(SPELL_WARDING, SPELLVALUE_BASE_POINT0, -6, target, true, NULL,NULL);
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_priest_inner_sanctum_AuraScript::OnApply, EFFECT_0, SPELL_AURA_MOD_RESISTANCE_PCT, AURA_EFFECT_HANDLE_REAL);
            OnEffectRemove +=AuraEffectRemoveFn(spell_priest_inner_sanctum_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_MOD_RESISTANCE_PCT, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_priest_inner_sanctum_AuraScript();
    }
};

// 73510 - Mind spike
class spell_priest_mind_spike : public SpellScriptLoader
{
    public:
        spell_priest_mind_spike() : SpellScriptLoader("spell_priest_mind_spike") { }

        class spell_priest_mind_spike_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_priest_mind_spike_SpellScript);

            void HandleHit()
            {
                Unit * target = GetHitUnit();
                Unit * caster = GetCaster();

                if(!target || !caster)
                    return;

                target->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE,caster->GetGUID(),target->GetAura(14914,caster->GetGUID())); // Holy fire is excluded
                target->RemoveAurasByType(SPELL_AURA_PERIODIC_LEECH,caster->GetGUID());

            }

            void Register()
            {
                OnHit += SpellHitFn(spell_priest_mind_spike_SpellScript::HandleHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_priest_mind_spike_SpellScript();
        }
};


void AddSC_priest_spell_scripts()
{
    new spell_pri_guardian_spirit();
    new spell_pri_mana_burn;
    new spell_pri_penance;
    new spell_pri_reflective_shield_trigger();
    new spell_pri_cure_disease();
    new mob_shadowy_apparition;
    new mob_cauterizing_flame();
    new spell_pri_spirit_of_redemption();
    new spell_priest_inner_sanctum();
    new spell_priest_mind_spike();
}
