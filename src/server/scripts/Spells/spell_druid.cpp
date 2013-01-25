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
 * Scripts for spells with SPELLFAMILY_DRUID and SPELLFAMILY_GENERIC spells used by druid players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_dru_".
 */

#include "ScriptPCH.h"
#include "SpellAuraEffects.h"

enum DruidSpells
{
    DRUID_INCREASED_MOONFIRE_DURATION   = 38414,
    DRUID_NATURES_SPLENDOR              = 57865
};

// 62606 - Savage Defense
class spell_dru_savage_defense : public SpellScriptLoader
{
public:
    spell_dru_savage_defense() : SpellScriptLoader("spell_dru_savage_defense") { }

    class spell_dru_savage_defense_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_dru_savage_defense_AuraScript);

        uint32 absorbPct;

            bool Load()
            {
                absorbPct = SpellMgr::CalculateSpellEffectAmount(GetSpellProto(), EFFECT_0, GetCaster());
                return true;
            }

            void CalculateAmount(AuraEffect const * /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
            {
                // Set absorbtion amount to unlimited
                amount = -1;
            }

            void Absorb(AuraEffect * aurEff, DamageInfo & /*dmgInfo*/, uint32 & absorbAmount)
            {
                absorbAmount = uint32(CalculatePctN(GetTarget()->GetTotalAttackPowerValue(BASE_ATTACK), absorbPct));
                aurEff->SetAmount(0);
            }

            void Register()
            {
                 DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_dru_savage_defense_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
                 OnEffectAbsorb += AuraEffectAbsorbFn(spell_dru_savage_defense_AuraScript::Absorb, EFFECT_0);
            }
        };

        AuraScript *GetAuraScript() const
        {
            return new spell_dru_savage_defense_AuraScript();
        }
};

class spell_dru_t10_restoration_4p_bonus : public SpellScriptLoader
{
    public:
        spell_dru_t10_restoration_4p_bonus() : SpellScriptLoader("spell_dru_t10_restoration_4p_bonus") { }

        class spell_dru_t10_restoration_4p_bonus_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_dru_t10_restoration_4p_bonus_SpellScript);

            void FilterTargets(std::list<Unit*>& unitList)
            {
                unitList.remove(GetTargetUnit());
                std::list<Unit*> tempTargets;
                std::list<Unit*>::iterator end = unitList.end(), itr = unitList.begin();
                for (; itr != end; ++itr)
                    if (GetCaster()->IsInRaidWith(*itr))
                        tempTargets.push_back(*itr);

                itr = tempTargets.begin();
                std::advance(itr, urand(0, tempTargets.size()-1));
                unitList.clear();
                unitList.push_back(*itr);
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_dru_t10_restoration_4p_bonus_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_AREA_ALLY_DST);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_dru_t10_restoration_4p_bonus_SpellScript();
        }
};

class spell_druid_blood_in_water : public SpellScriptLoader
{
    public:
        spell_druid_blood_in_water() : SpellScriptLoader("spell_druid_blood_in_water") { }

        class spell_druid_blood_in_water_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_druid_blood_in_water_SpellScript);

            enum spell
            {
                RIP = 1079,
            };

            bool Validate(SpellEntry const* /*spellEntry*/)
            {
               return (sSpellStore.LookupEntry(RIP));
            }

            bool Load()
            {
                _executed = false;
                return (GetCaster()->GetTypeId() == TYPEID_PLAYER && GetCaster()->getClass() == CLASS_DRUID);
            }

            void HandleAfterHit()
            {
                if (_executed)
                    return;

                _executed = true;

                    if( Unit* caster = GetCaster())
                        if (Unit* unitTarget = GetHitUnit())
                            if(unitTarget->HealthBelowPct(26)) // <= 25 %
                                if (unitTarget->HasAura(RIP))
                                {

                                    Unit::AuraApplicationMap const& auras = unitTarget->GetAppliedAuras();
                                    for (Unit::AuraApplicationMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                                    {
                                        Aura* aura = itr->second->GetBase();
                                        if(aura->GetCaster() != NULL && aura->GetCaster()->GetTypeId() == TYPEID_PLAYER
                                            && aura->GetCaster()->ToPlayer() == caster && aura->GetId() == RIP)
                                        {
                                            if(caster->HasAura(80318) && roll_chance_i(50)) // 50 % rank 1
                                            {
                                                aura->RefreshDuration();
                                                break;
                                            }
                                            else if(roll_chance_i(100) && caster->HasAura(80319)) // 100 % rank 2
                                            {
                                                aura->RefreshDuration();
                                                break;
                                            }
                                        }
                                    }
                                }
            }

            void Register()
            {
                AfterHit += SpellHitFn(spell_druid_blood_in_water_SpellScript::HandleAfterHit);
            }

            bool _executed;
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_druid_blood_in_water_SpellScript;
        }
};

class spell_druid_pulverize : public SpellScriptLoader
{
    public:
        spell_druid_pulverize() : SpellScriptLoader("spell_druid_pulverize") { }

        class spell_druid_pulverize_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_druid_pulverize_SpellScript);

            enum spell
            {
                PULVERIZE_BUFF  = 80951,
                LACERATE_DOT    = 33745,
            };

            int32 stackAmount;

            bool Validate(SpellEntry const* /*spellEntry*/)
            {
               return ( sSpellStore.LookupEntry(PULVERIZE_BUFF));
            }

            bool Load()
            {
                _executed = false;
                stackAmount = 0;
                return (GetCaster()->GetTypeId() == TYPEID_PLAYER && GetCaster()->getClass() == CLASS_DRUID);
            }

            void HandleAfterHit()
            {
                if (_executed)
                    return;

                _executed = true;

                if( Unit* caster = GetCaster())
                    if (Unit* unitTarget = GetHitUnit())
                        if (unitTarget->HasAura(LACERATE_DOT)) // Ak ma target dotku Lacerate
                        {
                            int32 bp = stackAmount * 3;
                            caster->CastCustomSpell(caster, PULVERIZE_BUFF, &bp, 0, 0, true); // Dostane crtical buff podla poctu stakov

                            //Remove lacerate stacks from target, only those which are bound with caster
                            Unit::AuraApplicationMap const& auras = GetHitUnit()->GetAppliedAuras();
                            for (Unit::AuraApplicationMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                            {
                                Aura* aura = itr->second->GetBase();

                                if(aura->GetCaster() != NULL && aura->GetCaster()->GetTypeId() == TYPEID_PLAYER
                                  && aura->GetCaster()->ToPlayer() == caster && aura->GetId() == LACERATE_DOT)
                                {
                                    aura->Remove();
                                    break;
                                }
                            }
                        }
            }

            void CalculateDamage(SpellEffIndex /*effIndex*/)
            {
                Unit::AuraApplicationMap const& auras = GetHitUnit()->GetAppliedAuras();
                for (Unit::AuraApplicationMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                {
                    Aura* aura = itr->second->GetBase();
                    if (aura && aura->GetCaster() && aura->GetCaster()->ToPlayer() != NULL &&  aura->GetCaster()->ToPlayer() == GetCaster() && aura->GetId() == LACERATE_DOT )
                    {
                        stackAmount = aura->GetStackAmount();
                        break;
                    }
                }
                    SetHitDamage(GetHitDamage() + (stackAmount * 361)); // Original formula
            }

            void Register()
            {
                AfterHit += SpellHitFn(spell_druid_pulverize_SpellScript::HandleAfterHit);
                OnEffect += SpellEffectFn(spell_druid_pulverize_SpellScript::CalculateDamage, EFFECT_2, SPELL_EFFECT_NORMALIZED_WEAPON_DMG);
            }

            bool _executed;
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_druid_pulverize_SpellScript;
        }
};

// Insect Swarm application proc Nature' grace
class spell_druid_insect_swarm : public SpellScriptLoader
{
public:
    spell_druid_insect_swarm() : SpellScriptLoader("spell_druid_insect_swarm") { }

    class spell_druid_insect_swarm_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_druid_insect_swarm_AuraScript);

        enum haste_buff
        {
            NATURES_GRACE_BUFF = 16886,
            NATURES_GRACE_R1   = 16880,
            NATURES_GRACE_R2   = 61345,
            NATURES_GRACE_R3   = 61346,
        };

        void OnApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {
            if(!GetCaster() || !GetCaster()->ToPlayer() || !GetTarget())
                return;

            Unit *caster = GetCaster();
            int32 bp;

            if(caster->ToPlayer()->HasSpellCooldown(16886))
                return;

            if(caster->HasAura(NATURES_GRACE_R1))
            {
                bp = 5;
                caster->CastCustomSpell(caster, NATURES_GRACE_BUFF, &bp, 0, 0, true);
                caster->ToPlayer()->AddSpellCooldown(16886,0,time(NULL) + 60);
            }
            else if(caster->HasAura(NATURES_GRACE_R2))
            {
                bp = 10;
                caster->CastCustomSpell(caster, NATURES_GRACE_BUFF, &bp, 0, 0, true);
                caster->ToPlayer()->AddSpellCooldown(16886,0,time(NULL) + 60);
            }
            else if(caster->HasAura(NATURES_GRACE_R3))
            {
                bp = 15;
                caster->CastCustomSpell(caster, NATURES_GRACE_BUFF, &bp, 0, 0, true);
                caster->ToPlayer()->AddSpellCooldown(16886,0,time(NULL) + 60);
            }
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_druid_insect_swarm_AuraScript::OnApply, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_druid_insect_swarm_AuraScript();
    }
};

void AddSC_druid_spell_scripts()
{
    new spell_dru_savage_defense();
    new spell_dru_t10_restoration_4p_bonus();
    new spell_druid_blood_in_water();
    new spell_druid_pulverize();
    new spell_druid_insect_swarm();
}
