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
 * Scripts for spells with SPELLFAMILY_MAGE and SPELLFAMILY_GENERIC spells used by mage players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_mage_".
 */

#include "ScriptPCH.h"

enum MageSpells
{
    SPELL_MAGE_COLD_SNAP                         = 11958,
    SPELL_MAGE_SQUIRREL_FORM                     = 32813,
    SPELL_MAGE_GIRAFFE_FORM                      = 32816,
    SPELL_MAGE_SERPENT_FORM                      = 32817,
    SPELL_MAGE_DRAGONHAWK_FORM                   = 32818,
    SPELL_MAGE_WORGEN_FORM                       = 32819,
    SPELL_MAGE_SHEEP_FORM                        = 32820,
    SPELL_MAGE_GLYPH_OF_ETERNAL_WATER            = 70937,
    SPELL_MAGE_SUMMON_WATER_ELEMENTAL_PERMANENT  = 70908,
    SPELL_MAGE_SUMMON_WATER_ELEMENTAL_TEMPORARY  = 70907,
    SPELL_MAGE_GLYPH_OF_BLAST_WAVE               = 62126
};

class spell_mage_cold_snap : public SpellScriptLoader
{
    public:
        spell_mage_cold_snap() : SpellScriptLoader("spell_mage_cold_snap") { }

        class spell_mage_cold_snap_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_cold_snap_SpellScript)
            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit *caster = GetCaster();

                if (caster->GetTypeId() != TYPEID_PLAYER)
                    return;

                // immediately finishes the cooldown on Frost spells
                const SpellCooldowns& cm = caster->ToPlayer()->GetSpellCooldownMap();
                for (SpellCooldowns::const_iterator itr = cm.begin(); itr != cm.end();)
                {
                    SpellEntry const *spellInfo = sSpellStore.LookupEntry(itr->first);

                    if (spellInfo && spellInfo->SpellFamilyName == SPELLFAMILY_MAGE &&
                        ((GetSpellSchoolMask(spellInfo) & SPELL_SCHOOL_MASK_FROST) || (spellInfo->Id == 92283))  && 
                        spellInfo->Id != SPELL_MAGE_COLD_SNAP && GetSpellRecoveryTime(spellInfo) > 0)
                    {
                        caster->ToPlayer()->RemoveSpellCooldown((itr++)->first, true);
                    }
                    else
                        ++itr;
                }
            }

            void Register()
            {
                // add dummy effect spell handler to Cold Snap
                OnEffect += SpellEffectFn(spell_mage_cold_snap_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_cold_snap_SpellScript();
        }
};

// Frost Warding
class spell_mage_frost_warding_trigger : public SpellScriptLoader
{
public:
    spell_mage_frost_warding_trigger() : SpellScriptLoader("spell_mage_frost_warding_trigger") { }

    class spell_mage_frost_warding_trigger_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_mage_frost_warding_trigger_AuraScript);

        enum Spells
        {
            SPELL_MAGE_FROST_WARDING_TRIGGERED = 57776,
            SPELL_MAGE_FROST_WARDING_R1 = 28332,
        };

        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            return sSpellStore.LookupEntry(SPELL_MAGE_FROST_WARDING_TRIGGERED) 
                && sSpellStore.LookupEntry(SPELL_MAGE_FROST_WARDING_R1);
        }

        void Absorb(AuraEffect * aurEff, DamageInfo & dmgInfo, uint32 & absorbAmount)
        {
            Unit * target = GetTarget();
            if (AuraEffect * talentAurEff = target->GetAuraEffectOfRankedSpell(SPELL_MAGE_FROST_WARDING_R1, EFFECT_0))
            {
                int32 chance = SpellMgr::CalculateSpellEffectAmount(talentAurEff->GetSpellProto(), EFFECT_1);

                if (roll_chance_i(chance))
                {
                    absorbAmount = dmgInfo.GetDamage();
                    int32 bp = absorbAmount;
                    target->CastCustomSpell(target, SPELL_MAGE_FROST_WARDING_TRIGGERED, &bp, NULL, NULL, true, NULL, aurEff);
                }
            }
        }

        void Register()
        {
             OnEffectAbsorb += AuraEffectAbsorbFn(spell_mage_frost_warding_trigger_AuraScript::Absorb, EFFECT_0);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_mage_frost_warding_trigger_AuraScript();
    }
};

// Incanter's Absorption (Mage Ward)
class spell_mage_incanters_absorbtion_absorb : public SpellScriptLoader
{
public:
    spell_mage_incanters_absorbtion_absorb() : SpellScriptLoader("spell_mage_incanters_absorbtion_absorb") { }

class spell_mage_incanters_absorbtion_absorb_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_mage_incanters_absorbtion_absorb_AuraScript);

        enum Spells
        {
            SPELL_MAGE_INCANTERS_ABSORBTION_TRIGGERED = 44413,
            SPELL_MAGE_INCANTERS_ABSORBTION_R1 = 44394,
            SPELL_MAGE_INCANTERS_ABSORBTION_R2 = 44395,
        };

        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            return sSpellStore.LookupEntry(SPELL_MAGE_INCANTERS_ABSORBTION_TRIGGERED) 
                && sSpellStore.LookupEntry(SPELL_MAGE_INCANTERS_ABSORBTION_R1)
                && sSpellStore.LookupEntry(SPELL_MAGE_INCANTERS_ABSORBTION_R2);
        }

        void Trigger(AuraEffect * aurEff, DamageInfo & dmgInfo, uint32 & absorbAmount)
        {
            if(aurEff->GetId() != 543) // Mage Ward
                return;

            Unit * target = GetTarget();

            if (AuraEffect * talentAurEff = target->GetAuraEffectOfRankedSpell(SPELL_MAGE_INCANTERS_ABSORBTION_R1, EFFECT_0))
            {
                int32 bp = CalculatePctN(absorbAmount, talentAurEff->GetAmount());
                target->CastCustomSpell(target, SPELL_MAGE_INCANTERS_ABSORBTION_TRIGGERED, &bp, NULL, NULL, true, NULL, aurEff);
            }
            else if (AuraEffect * talentAurEff = target->GetAuraEffectOfRankedSpell(SPELL_MAGE_INCANTERS_ABSORBTION_R2, EFFECT_0))
            {
                int32 bp = CalculatePctN(absorbAmount, talentAurEff->GetAmount());
                target->CastCustomSpell(target, SPELL_MAGE_INCANTERS_ABSORBTION_TRIGGERED, &bp, NULL, NULL, true, NULL, aurEff);
            }
        }

        void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {
            if(!GetOwner() || !GetOwner()->ToPlayer() || !GetOwner()->ToPlayer()->HasAura(11094) ) // Molten shields talent
                return;

            if(GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_ENEMY_SPELL)
            {
                GetOwner()->ToPlayer()->CastSpell(GetOwner()->ToPlayer(),31643,true); // Blazing speed
            }
        }

        void Register()
        {
             AfterEffectAbsorb += AuraEffectAbsorbFn(spell_mage_incanters_absorbtion_absorb_AuraScript::Trigger, EFFECT_0);
             OnEffectRemove += AuraEffectRemoveFn(spell_mage_incanters_absorbtion_absorb_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_mage_incanters_absorbtion_absorb_AuraScript();
    }
};
// Incanter's Absorption (Mana Shield)
class spell_mage_incanters_absorbtion_manashield : public SpellScriptLoader
{
public:
    spell_mage_incanters_absorbtion_manashield() : SpellScriptLoader("spell_mage_incanters_absorbtion_manashield") { }

    class spell_mage_incanters_absorbtion_manashield_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_mage_incanters_absorbtion_manashield_AuraScript);

        enum Spells
        {
            SPELL_MAGE_INCANTERS_ABSORBTION_TRIGGERED = 44413,
            SPELL_MAGE_INCANTERS_ABSORBTION_R1 = 44394,
            SPELL_MAGE_INCANTERS_ABSORBTION_R2 = 44395,
        };

        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            return sSpellStore.LookupEntry(SPELL_MAGE_INCANTERS_ABSORBTION_TRIGGERED) 
                && sSpellStore.LookupEntry(SPELL_MAGE_INCANTERS_ABSORBTION_R1)
                && sSpellStore.LookupEntry(SPELL_MAGE_INCANTERS_ABSORBTION_R2);
        }

        void Trigger(AuraEffect * aurEff, DamageInfo & dmgInfo, uint32 & absorbAmount)
        {
            if(aurEff->GetId() != 1463) // Mana Shield
                return;

            Unit * target = GetTarget();

            if (AuraEffect * talentAurEff = target->GetAuraEffectOfRankedSpell(SPELL_MAGE_INCANTERS_ABSORBTION_R1, EFFECT_0))
            {
                int32 bp = CalculatePctN(absorbAmount, talentAurEff->GetAmount());
                target->CastCustomSpell(target, SPELL_MAGE_INCANTERS_ABSORBTION_TRIGGERED, &bp, NULL, NULL, true, NULL, aurEff);

                if (dmgInfo.GetDamage() != 0) // Mana Shield broken
                    target->CastSpell(target, 86261, true); // Knockback to nearby enemies
            }
            else if (AuraEffect * talentAurEff = target->GetAuraEffectOfRankedSpell(SPELL_MAGE_INCANTERS_ABSORBTION_R2, EFFECT_0))
            {
                int32 bp = CalculatePctN(absorbAmount, talentAurEff->GetAmount());
                target->CastCustomSpell(target, SPELL_MAGE_INCANTERS_ABSORBTION_TRIGGERED, &bp, NULL, NULL, true, NULL, aurEff);

                if (dmgInfo.GetDamage() != 0) // Mana Shield broken
                    target->CastSpell(target, 86261, true); // Knockback to nearby enemies
            }

        }

        void Register()
        {
             AfterEffectManaShield += AuraEffectManaShieldFn(spell_mage_incanters_absorbtion_manashield_AuraScript::Trigger, EFFECT_0);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_mage_incanters_absorbtion_manashield_AuraScript();
    }
};

class spell_mage_cauterize : public SpellScriptLoader
{
public:
    spell_mage_cauterize() : SpellScriptLoader("spell_mage_cauterize") { }

    class spell_mage_cauterize_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_mage_cauterize_AuraScript);

        uint32 absorbChance;

        enum Spels
        {
            SPELL_CAUTERIZE_DAMAGE = 87023,
        };
        
        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            return sSpellStore.LookupEntry(SPELL_CAUTERIZE_DAMAGE);
        }

        bool Load()
        {
            absorbChance = sSpellMgr->CalculateSpellEffectAmount(GetSpellProto(),EFFECT_0);
            return GetUnitOwner()->ToPlayer();
        }

        void CalculateAmount(AuraEffect const * /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
        {
            // Set absorbtion amount to unlimited
            amount = -1;
        }

        void Absorb(AuraEffect * /*aurEff*/, DamageInfo & dmgInfo, uint32 & absorbAmount)
        {
            Unit * target = GetTarget();
            if (dmgInfo.GetDamage() < target->GetHealth())
                return;
            if (target->ToPlayer()->HasSpellCooldown(SPELL_CAUTERIZE_DAMAGE))
                return;
            if (!roll_chance_i(absorbChance))
                return;

            int32 bp0 = 12, bp1 = target->CountPctFromMaxHealth(40);

            target->CastCustomSpell(target,SPELL_CAUTERIZE_DAMAGE,&bp0,&bp1,0,true);
            target->ToPlayer()->AddSpellCooldown(SPELL_CAUTERIZE_DAMAGE, 0, 60000);

            absorbAmount = dmgInfo.GetDamage();
        }

        void Register()
        {
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_mage_cauterize_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
            OnEffectAbsorb += AuraEffectAbsorbFn(spell_mage_cauterize_AuraScript::Absorb, EFFECT_0);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_mage_cauterize_AuraScript();
    }
};

// Impact
class spell_mage_impact : public SpellScriptLoader
{
public:
    spell_mage_impact() : SpellScriptLoader("spell_mage_impact") { }

    class spell_mage_impact_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_mage_impact_SpellScript);

        typedef struct auraRecord
        {
            uint32 aura_id;
            int32 aura_duration;
            int32 aura_per_timer;
            int16 auraEff_id;
            uint32 aura_bp;
            struct auraRecord *next;
        }DOT_RECORD;

        Unit * mainTarget;
        std::vector<DOT_RECORD> dots;
        uint32 living_bomb_counter;

        bool Load() override
        {
            if (GetCaster()->GetTypeId() != TYPEID_PLAYER)
                return false;

            mainTarget = nullptr;
            living_bomb_counter = 0;
            return true;
        }

        void HandleDirectEffect(SpellEffIndex /*effIndex*/)
        {
            Unit * caster = GetCaster();
            Unit * target = GetHitUnit();

            if (!caster || !target)
                return;

            mainTarget = target;

            Unit::AuraApplicationMap const& auras = target->GetAppliedAuras();
            for (Unit::AuraApplicationMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            {
                Aura* aura = itr->second->GetBase();

                if(aura && aura->GetCaster() != NULL && aura->GetCaster()->ToPlayer()
                  && (aura->GetCaster()->ToPlayer() == caster) && aura->GetSpellProto() && aura->GetSpellProto()->SchoolMask == SPELL_SCHOOL_MASK_FIRE &&
                  (aura->HasEffectType(SPELL_AURA_PERIODIC_DAMAGE) || aura->HasEffectType(SPELL_AURA_PERIODIC_TRIGGER_SPELL)))
                {
                    DOT_RECORD dot;

                    dot.aura_id = aura->GetId();
                    dot.aura_duration = aura->GetDuration();
                    dot.auraEff_id = -1; // For sure -1 means fail

                    for(uint8 i = 0; i < MAX_SPELL_EFFECTS ; i++) // Need to find right id of effect where needed aura' are
                    {
                        if (AuraEffect* aurEff = aura->GetEffect(i))
                        {
                            AuraType aurType = aurEff->GetAuraType();

                            if(aurType == SPELL_AURA_PERIODIC_DAMAGE || aurType == SPELL_AURA_PERIODIC_TRIGGER_SPELL)
                            {
                                dot.auraEff_id = i;
                                dot.aura_per_timer = aurEff->GetPeriodicTimer();
                                if (aurType == SPELL_AURA_PERIODIC_DAMAGE)
                                    dot.aura_bp = aurEff->GetDamage();
                                else
                                    dot.aura_bp = aurEff->GetAmount();
                                break;
                            }
                        }
                    }
                    dots.push_back(dot);
                }
            }
        }

        void HandleSpreadEffect(SpellEffIndex /*effIndex*/)
        {
            Unit * effectTarget = GetHitUnit();

            if (!effectTarget || effectTarget == mainTarget)
                return;

            if (!mainTarget)
                return;

            #define LIVING_BOMB 44457

            for (auto &dot : dots)
            {
                if(dot.aura_id == LIVING_BOMB)
                {
                    if (++living_bomb_counter > 3) // maximum 2 copies of living bomb
                        continue;
                }
                GetCaster()->AddAura(dot.aura_id,effectTarget);// Add according aura

                if(Aura *nova = effectTarget->GetAura(dot.aura_id,GetCaster()->GetGUID()))
                {
                    nova->SetDuration(dot.aura_duration); // Set according duration

                    if(dot.auraEff_id != -1)
                    {
                        if (AuraEffect* aurEff = nova->GetEffect(dot.auraEff_id))
                        {
                            aurEff->SetPeriodicTimer(dot.aura_per_timer); // Need synchronize periodic timer

                            if (aurEff->GetAuraType() == SPELL_AURA_PERIODIC_DAMAGE)
                                aurEff->SetDamage(dot.aura_bp);
                            else
                                aurEff->SetAmount(dot.aura_bp);
                        }
                    }
                }
            }
        }

        void Register() override
        {
            OnEffect += SpellEffectFn(spell_mage_impact_SpellScript::HandleDirectEffect, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
            OnEffect += SpellEffectFn(spell_mage_impact_SpellScript::HandleSpreadEffect, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_mage_impact_SpellScript();
    }
};

// Blizzard
class spell_mage_blizzard : public SpellScriptLoader
{
public:
    spell_mage_blizzard() : SpellScriptLoader("spell_mage_blizzard") { }

    class spell_mage_blizzard_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_mage_blizzard_SpellScript);

        Player *caster;

        enum spells
        {
            // Talents
            ICE_SHARDS_R1 = 11185,
            ICE_SHARDS_R2 = 12487,
            // Chill effects
            CHILLED_R1 = 12484,
            CHILLED_R2 = 12485,
        };

        bool Load()
        {
            if (GetCaster()->GetTypeId() != TYPEID_PLAYER || GetCaster()->ToPlayer()->getClass() != CLASS_MAGE )
                return false;

            caster = GetCaster()->ToPlayer();
            return true;
        }

        void HandleBlizzard(SpellEffIndex /*effIndex*/)
        {
            if (!GetHitUnit())
                return;

            if(caster->HasAura(11175)) // Permafrost (Rank 1)
            {
                if(caster->HasAura(ICE_SHARDS_R1))
                    caster->CastCustomSpell(CHILLED_R1, SPELLVALUE_BASE_POINT1, -3, GetHitUnit(), true); // - 3% healing
                else if(caster->HasAura(ICE_SHARDS_R2))
                    caster->CastCustomSpell(CHILLED_R2, SPELLVALUE_BASE_POINT1, -3, GetHitUnit(), true); // - 3% healing
            }

            else if(caster->HasAura(12569)) // Permafrost (Rank 2)
            {
                if(caster->HasAura(ICE_SHARDS_R1))
                    caster->CastCustomSpell(CHILLED_R1, SPELLVALUE_BASE_POINT1, -7, GetHitUnit(), true); // - 7% healing
                else if(caster->HasAura(ICE_SHARDS_R2))
                    caster->CastCustomSpell(CHILLED_R2, SPELLVALUE_BASE_POINT1, -7, GetHitUnit(), true); // - 7% healing
            }

            else if(caster->HasAura(12571)) // Permafrost (Rank 3)
            {
                if(caster->HasAura(ICE_SHARDS_R1))
                    caster->CastCustomSpell(CHILLED_R1, SPELLVALUE_BASE_POINT1, -10, GetHitUnit(), true); // - 10% healing
                else if(caster->HasAura(ICE_SHARDS_R2))
                    caster->CastCustomSpell(CHILLED_R2, SPELLVALUE_BASE_POINT1, -10, GetHitUnit(), true); // - 10% healing
            }
            else // Don't have talent Permafrost -> regular chill effect without healing reduction
            {
                if(caster->HasAura(ICE_SHARDS_R1))
                    caster->CastSpell(GetHitUnit(),CHILLED_R1,true);
                else if(caster->HasAura(ICE_SHARDS_R2))
                    caster->CastSpell(GetHitUnit(),CHILLED_R2,true);
            }
        }

            void Register()
            {
                OnEffect += SpellEffectFn(spell_mage_blizzard_SpellScript::HandleBlizzard, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

    SpellScript* GetSpellScript() const
    {
        return new spell_mage_blizzard_SpellScript();
    }
};

class spell_mage_invocation : public SpellScriptLoader
{
public:
    spell_mage_invocation() : SpellScriptLoader("spell_mage_invocation") { }

    class spell_mage_invocation_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_mage_invocation_SpellScript);

        Player *caster;
        bool success;

        enum talents
        {
            INVOCATION_RANK1 = 84722,
            INVOCATION_RANK2 = 84723,
            INVOCATION_BUFF  = 87098,
        };

        bool Load()
        {
            if (GetCaster()->GetTypeId() != TYPEID_PLAYER || GetCaster()->ToPlayer()->getClass() != CLASS_MAGE )
                return false;

            caster = GetCaster()->ToPlayer();
            return true;
        }

        void HandleInvocation(SpellEffIndex /*effIndex*/)
        {
            if (!GetHitUnit())
                return;

            success = false;

            if(GetHitUnit()->HasUnitState(UNIT_STATE_CASTING))
                success = true;
        }

        void HandleInterrupt(void)
        {
            if (!GetHitUnit())
                return;

            if(success)
            {
                if(caster->HasAura(INVOCATION_RANK1))
                    caster->CastCustomSpell(INVOCATION_BUFF, SPELLVALUE_BASE_POINT0, 5, caster, true);// 5 % dmg boost

                else if(caster->HasAura(INVOCATION_RANK2))
                    caster->CastCustomSpell(INVOCATION_BUFF, SPELLVALUE_BASE_POINT0, 10, caster, true);// 10 % dmg boost
            }
        }

            void Register()
            {
                OnEffect += SpellEffectFn(spell_mage_invocation_SpellScript::HandleInvocation, EFFECT_0, SPELL_EFFECT_INTERRUPT_CAST);
                AfterHit += SpellHitFn(spell_mage_invocation_SpellScript::HandleInterrupt);
            }
        };

    SpellScript* GetSpellScript() const
    {
        return new spell_mage_invocation_SpellScript();
    }
};

class spell_mage_reactive_barrier : public SpellScriptLoader
{
public:
    spell_mage_reactive_barrier() : SpellScriptLoader("spell_mage_reactive_barrier") { }

    class spell_mage_reactive_barrier_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_mage_reactive_barrier_AuraScript);

        Player * caster;

        enum Spels
        {
            ICE_BARRIER = 11426,
            REACTIVE_BARRIER_R1 =  86303,
            REACTIVE_BARRIER_R2 =  86304,
            RB_REDUCE_MANA_COST =  86347,
        };

        bool Load()
        {
            if(!GetCaster() || !GetCaster()->ToPlayer() || GetCaster()->ToPlayer()->getClass() != CLASS_MAGE )
                return false;
            else
            {
                caster = GetCaster()->ToPlayer();
                return true;
            }
        }

        void CalculateAmount(AuraEffect const * /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
        {
            amount = -1; // Set absorbtion amount to unlimited
        }

        void Absorb(AuraEffect * /*aurEff*/, DamageInfo & dmgInfo, uint32 & absorbAmount)
        {
            if (caster->GetHealth() < caster->GetMaxHealth() / 2 ) // must be above 50 % of health
                return;

            if (caster->GetHealth() - dmgInfo.GetDamage() > caster->GetMaxHealth() / 2 ) // Health after damage have to be less than 50 % of health
                return;

            if (caster->HasSpellCooldown(ICE_BARRIER)) // This talent obeys Ice Barrier's cooldown
                return;

            if(caster->HasAura(REACTIVE_BARRIER_R1))
            {
                if (roll_chance_i(50)) // 50 % chance for rank 1
                {
                    caster->CastSpell(caster, RB_REDUCE_MANA_COST, true);
                    caster->CastSpell(caster, ICE_BARRIER, true);
                }
            }
            else // 100 % chance for rank 2
            {
                caster->CastSpell(caster, RB_REDUCE_MANA_COST, true);
                caster->CastSpell(caster, ICE_BARRIER, true);
            }
        }

        void Register()
        {
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_mage_reactive_barrier_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
            OnEffectAbsorb += AuraEffectAbsorbFn(spell_mage_reactive_barrier_AuraScript::Absorb, EFFECT_0);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_mage_reactive_barrier_AuraScript();
    }
};



enum FlamestrikeSpells
{
    ICON_MAGE_IMPROVED_FLAMESTRIKE               = 37,
    SPELL_MAGE_FLAMESTRIKE                       = 2120
};

//Improved Flamestrike 
class spell_mage_blast_wave : public SpellScriptLoader
{
    public:
        spell_mage_blast_wave() : SpellScriptLoader("spell_mage_blast_wave") { }

        class spell_mage_blast_wave_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_blast_wave_SpellScript);

            uint32 _targetCount;
            bool done;

            bool Load()
            {
                _targetCount = 0;
                done = false;
                return true;
            }

            void CountTargets(std::list<Unit*>& targetList)
            {
                _targetCount = targetList.size();
            }

            void HandleImprovedFlamestrike(SpellEffIndex /*eff*/)
            {
                Unit * caster = GetCaster();

                if(!caster || done)
                    return;

                if (_targetCount >= 2)
                    if (AuraEffect* aurEff = caster->GetAuraEffect(SPELL_AURA_DUMMY, SPELLFAMILY_MAGE, ICON_MAGE_IMPROVED_FLAMESTRIKE, EFFECT_0))
                        if (roll_chance_i(aurEff->GetAmount()))
                        {
                            float x, y, z;
                            WorldLocation const* loc = GetTargetDest();
                            if (!loc)
                                return;

                            done = true;
                            loc->GetPosition(x, y, z);
                            caster->CastSpell(x, y, z, SPELL_MAGE_FLAMESTRIKE, true);
                        }
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_mage_blast_wave_SpellScript::CountTargets, EFFECT_0, TARGET_UNIT_AREA_ENEMY_DST);
                OnEffect += SpellEffectFn(spell_mage_blast_wave_SpellScript::HandleImprovedFlamestrike, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_blast_wave_SpellScript();
        }
};

/*######
## npc_flame_orb
######*/

enum eFlameOrb
{
    ENTRY_FLAME_ORB                    = 44214,
    ENTRY_FROSTFIRE_ORB                = 45322,

    SPELL_FLAME_ORB_DAMAGE_VISUAL      = 86719,
    SPELL_FLAME_ORB_DAMAGE             = 82739,
    SPELL_FLAME_ORB_FIRE_POWER_EXPLODE = 83619,
    FLAME_ORB_DISTANCE                 = 120,

    SPELL_FROSTFIRE_ORB_VISUAL         = 84716,
    SPELL_FROSTFIRE_ORB_DAMAGE         = 95969,
    SPELL_FROSTFIRE_ORB_DAMAGE_CHILL   = 84721,
};

class npc_flame_orb : public CreatureScript
{
public:
    npc_flame_orb() : CreatureScript("npc_flame_orb") {}

    struct npc_flame_orbAI : public ScriptedAI
    {
        npc_flame_orbAI(Creature *c) : ScriptedAI(c)
        {
            x = me->GetPositionX();
            y = me->GetPositionY();
            isChilling = false;
            if (Unit* owner = me->GetOwner())
            {
                Unit* target = me->SelectNearestTarget(20);
                if (target)
                    uiDespawnTimer = 15*IN_MILLISECONDS;
                else
                    uiDespawnTimer = 4*IN_MILLISECONDS;

                z = owner->GetPositionZ() + 3.0f;
                angle = owner->GetAngle(me);
                isChilling = owner->HasAura(84727);
            }

            o = me->GetOrientation();
            me->NearTeleportTo(x, y, z, o, true);
            newx = me->GetPositionX() + 5.0f * cos(angle);
            newy = me->GetPositionY() + 5.0f * sin(angle);
            z = me->GetMap()->GetHeight(newx, newy, z+3.0f) + 3.0f;
            SlowedDown = true;
            MoveCheck = false;

            isFrostfire = (c->GetEntry() == ENTRY_FROSTFIRE_ORB);
            if (isFrostfire)
                me->CastSpell(me, SPELL_FROSTFIRE_ORB_VISUAL, true);
        }

        bool isFrostfire;
        bool isChilling;
        bool isFrostfireCheck;

        float x,y,z,o,newx,newy,angle;
        bool SlowedDown;
        bool MoveCheck;
        uint32 uiHardDespawnTimer;
        uint32 uiDespawnTimer;
        uint32 uiDespawnCheckTimer;
        uint32 uiDamageTimer;

        void EnterCombat(Unit* target)
        {
            me->GetMotionMaster()->MoveCharge(newx, newy, z, 1.14286f); // Normal speed
            uiDespawnTimer = 15*IN_MILLISECONDS;
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type == 8)
            {
                newx = me->GetPositionX() + 5.0f * cos(angle);
                newy = me->GetPositionY() + 5.0f * sin(angle);
                z = me->GetMap()->GetHeight(newx, newy, z+3.0f) + 3.0f;
                MoveCheck = true;
            }
        }

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE);
            me->SetFlying(true);
            me->SetReactState(REACT_PASSIVE);

            isFrostfireCheck = false;

            uiHardDespawnTimer = 15*IN_MILLISECONDS;
            uiDamageTimer = 1*IN_MILLISECONDS;
            me->GetMotionMaster()->MoveCharge(newx, newy, z, 5.5f);
        }

        void UpdateAI(const uint32 diff)
        {
            if (isChilling && isFrostfireCheck == false) {
                    me->CastSpell(me, SPELL_FROSTFIRE_ORB_VISUAL, true);
                    isFrostfireCheck = true;
                }

            if (MoveCheck)
            {
                MoveCheck = false;
                if (SlowedDown)
                    me->GetMotionMaster()->MoveCharge(newx, newy, z, 1.14286f);
                else
                    me->GetMotionMaster()->MoveCharge(newx, newy, z, 5.5f);
            }

            if (!me->IsInCombat() && SlowedDown)
            {
                SlowedDown = false;
                me->SetSpeed(MOVE_RUN, 2.0f, true);
                me->SetSpeed(MOVE_FLIGHT, 2.0f, true);
            }

            if (uiDespawnTimer <= diff || uiHardDespawnTimer <= diff)
            {
                if (Unit* owner = me->GetOwner())
                    if (AuraEffect* aureff = owner->GetAuraEffect(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE,SPELLFAMILY_MAGE,31,EFFECT_0))
                        if (const SpellEntry* spellinfo = aureff->GetSpellProto())
                            if (roll_chance_i(spellinfo->GetProcChance()))
                                owner->CastSpell(me, SPELL_FLAME_ORB_FIRE_POWER_EXPLODE, true);
                me->SetVisibility(VISIBILITY_OFF);
                me->DisappearAndDie();
            }
            else
                uiDespawnTimer -= diff;

            if (uiDamageTimer <= diff)
            {
                if (Unit* target = me->SelectNearestTarget(20))
                {
                    if (!SlowedDown)
                    {
                        me->GetMotionMaster()->MovementExpired();
                        me->GetMotionMaster()->MoveCharge(newx, newy, z, 1.14286f);
                        SlowedDown = true;
                    }
                    if (target->HasNegativeAuraWithAttribute(SPELL_ATTR0_BREAKABLE_BY_DAMAGE,0))
                    {
                        uiDamageTimer = 1 * IN_MILLISECONDS;
                        return;
                    }

                    me->CastSpell(target, SPELL_FLAME_ORB_DAMAGE_VISUAL, false);
                    if (Unit* owner = me->GetOwner())
                        owner->CastSpell(target, isFrostfire ? (isChilling ? SPELL_FROSTFIRE_ORB_DAMAGE_CHILL : SPELL_FROSTFIRE_ORB_DAMAGE) : SPELL_FLAME_ORB_DAMAGE, true);
                    
                    if ((isChilling) && (target))
                    {
                        if (Unit* owner = me->GetOwner())
                        {
                            // Finger of Frost (44544) proc
                            if (owner->HasAura(44543)) // Finger of Frost (Rank 1) 7%
                            if (roll_chance_i(7))
                                owner->CastSpell(owner, 44544, true);
                            if (owner->HasAura(44544)) // Finger of Frost (Rank 2) 14%
                            if (roll_chance_i(14))
                                owner->CastSpell(owner, 44544, true);
                            if (owner->HasAura(83074)) // Finger of Frost (Rank 3) 20 %
                            if (roll_chance_i(20))
                                owner->CastSpell(owner, 44544, true);

                            // Brain Freeze (57761) proc
                            if (owner->HasAura(44546)) // Brain Freeze (Rank 1) 5%
                            if (roll_chance_i(5))
                                owner->CastSpell(owner, 57761, true);
                            if (owner->HasAura(44548)) // Brain Freeze (Rank 2) 10%
                            if (roll_chance_i(10))
                                owner->CastSpell(owner, 57761, true);
                            if (owner->HasAura(44549)) // Brain Freeze (Rank 3) 15%
                            if (roll_chance_i(15))
                                owner->CastSpell(owner, 57761, true);
                        }
                    }
                }
                uiDamageTimer = 1*IN_MILLISECONDS;
            }
            else
                uiDamageTimer -= diff;
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_flame_orbAI(creature);
    }
};

// Deep Freeze
class spell_mage_deep_freeze_dmg : public SpellScriptLoader
{
public:
    spell_mage_deep_freeze_dmg() : SpellScriptLoader("spell_mage_deep_freeze_dmg") { }

    class spell_mage_deep_freeze_dmg_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_mage_deep_freeze_dmg_SpellScript);

        void HandleEffect(SpellEffIndex /*effIndex*/)
        {
            Unit * caster = GetCaster();
            Unit * target = GetHitUnit();

            if (!caster || !target)
                return;

            if (target->GetTypeId() == TYPEID_UNIT && target->ToCreature()->isWorldBoss())
            {
                caster->CastSpell(target, 71757, true);
            }
        }

        void Register() override
        {
            OnEffect += SpellEffectFn(spell_mage_deep_freeze_dmg_SpellScript::HandleEffect, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_mage_deep_freeze_dmg_SpellScript();
    }
};

void AddSC_mage_spell_scripts()
{
    new spell_mage_cold_snap();
    new spell_mage_frost_warding_trigger();
    new spell_mage_incanters_absorbtion_absorb();
    new spell_mage_incanters_absorbtion_manashield();
    new spell_mage_cauterize();
    new spell_mage_impact();
    new spell_mage_blizzard();
    new spell_mage_invocation();
    new spell_mage_reactive_barrier();
    new spell_mage_blast_wave(); // 11113
    new npc_flame_orb();
    new spell_mage_deep_freeze_dmg();
}
