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
                caster->ToPlayer()->AddSpellCooldown(16886,0,60000);
            }
            else if(caster->HasAura(NATURES_GRACE_R2))
            {
                bp = 10;
                caster->CastCustomSpell(caster, NATURES_GRACE_BUFF, &bp, 0, 0, true);
                caster->ToPlayer()->AddSpellCooldown(16886,0,60000);
            }
            else if(caster->HasAura(NATURES_GRACE_R3))
            {
                bp = 15;
                caster->CastCustomSpell(caster, NATURES_GRACE_BUFF, &bp, 0, 0, true);
                caster->ToPlayer()->AddSpellCooldown(16886,0,60000);
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

class spell_dru_mush_detonate : public SpellScriptLoader
{
    public:
        spell_dru_mush_detonate() : SpellScriptLoader("spell_dru_mush_detonate") { }

        class spell_dru_mush_detonate_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_dru_mush_detonate_SpellScript);

            enum spells
            {
                EARTH_AND_MOON_TALENT   = 48506,
                EARTH_AND_MOON_DEBUFF   = 60433
            };

            void HandleEffect(SpellEffIndex effIndex)
            {
                Unit* player = GetOriginalCaster()->ToPlayer();
                if(player == NULL)
                    return;

                if (GetHitUnit() && player->HasAura(EARTH_AND_MOON_TALENT)) // Wild Mushrooms should apply Earth and moon debuff
                    player->CastSpell(GetHitUnit(),EARTH_AND_MOON_DEBUFF,true);
            }

            void Register()
            {
                OnEffect += SpellEffectFn(spell_dru_mush_detonate_SpellScript::HandleEffect, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_dru_mush_detonate_SpellScript();
        }
};

class spell_dru_efflo_periodic : public SpellScriptLoader
{
    public:
        spell_dru_efflo_periodic() : SpellScriptLoader("spell_dru_efflo_periodic") { }

        class spell_dru_efflo_periodic_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_dru_efflo_periodic_AuraScript)

            Position pos;
            bool done;

            bool Load()
            {
                done = false;
                return true;
            }

            void HandleEffectPeriodic(AuraEffect const * aurEff)
            {
                Unit* caster = aurEff->GetCaster();
                Unit *target = GetTarget();

                if (!caster || !target)
                    return;

                if(done == false) // Remember position of target (only once at begining)
                {
                    target->GetPosition(&pos);
                    done = true;
                }

                CustomSpellValues values;
                values.AddSpellMod(SPELLVALUE_BASE_POINT0, aurEff->GetAmount());
                caster->CastCustomSpell(81269, values, pos, true, NULL, aurEff, caster->GetGUID());
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_dru_efflo_periodic_AuraScript::HandleEffectPeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }

        };

        AuraScript* GetAuraScript() const
        {
            return new spell_dru_efflo_periodic_AuraScript();
        }
};

// Tranquility
class spell_dru_tranquility : public SpellScriptLoader
{
    public:
        spell_dru_tranquility() : SpellScriptLoader("spell_dru_tranquility") { }

        class spell_dru_tranquility_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_dru_tranquility_SpellScript);

            struct Record
            {
                float Health_pct;
                Unit * target;
            };

            struct Comp
            {
                bool operator()(const Record& i1, const Record& i2)
                {
                    return i1.Health_pct < i2.Health_pct;
                }
            };

            bool IsInPartyOrRaid(Unit *caster, Unit * u)
            {
                return ( caster->IsInRaidWith(u) || caster->IsInPartyWith(u) );
            }

            void FilterTargets(std::list<Unit*>& targets)
            {
                Unit * caster = GetCaster();
                if(caster == NULL ) return;

                std::vector<Record> temp_targets;

                for (std::list<Unit*>::const_iterator itr = targets.begin(); itr != targets.end(); ++itr)
                {
                    if ( (*itr)->ToUnit() == NULL || !IsInPartyOrRaid(caster,(*itr)->ToUnit()) ) // Security check + toolpit says
                        continue;                                                               // "Heals nearby lowest health party or raid targets" so this should  be correct solution

                    Record temp = {(*itr)->ToUnit()->GetHealthPct(),*itr }; // Fill new Structure

                    temp_targets.push_back(temp); // Push it to temp list
                }

                std::make_heap (temp_targets.begin(),temp_targets.end(),Comp());
                std::sort_heap (temp_targets.begin(),temp_targets.end(),Comp()); // This should make max heap ( due to Comp() function condition )

                targets.clear(); // Clean final target list

                std::vector<Record>::const_iterator itr = temp_targets.begin();
                uint8 counter = 0;
                const uint8 max_targets = 5; // Tranquility should affect only 5 targets with lowest HP

                for (std::vector<Record>::iterator itr = temp_targets.begin(); itr != temp_targets.end();++itr)
                {
                    if(counter == max_targets)
                        break;

                    if((*itr).target && (*itr).target->IsInWorld() == true)
                        targets.push_back((*itr).target); // Push correct target back to target list

                    counter++;
                }
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_dru_tranquility_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_AREA_ALLY_SRC);
                OnUnitTargetSelect += SpellUnitTargetFn(spell_dru_tranquility_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_AREA_ALLY_SRC);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_dru_tranquility_SpellScript();
        }
};

class spell_dru_solar_beam : public SpellScriptLoader
{
    public:
        spell_dru_solar_beam() : SpellScriptLoader("spell_dru_solar_beam") { }

        class spell_dru_solar_beam_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_dru_solar_beam_AuraScript)

            Position pos;
            bool done;

            bool Load()
            {
                done = false;
                return true;
            }

            void HandleEffectPeriodic(AuraEffect const * aurEff)
            {
                Unit* caster = aurEff->GetCaster();
                Unit *target = GetTarget();

                if (!caster || !target)
                    return;

                if(done == false) // Remember position of target (only once at begining)
                {
                    target->GetPosition(&pos);
                    done = true;
                }

                CustomSpellValues values;
                values.AddSpellMod(SPELLVALUE_BASE_POINT0, aurEff->GetAmount());
                caster->CastCustomSpell(81261, values, pos, true, NULL, aurEff, caster->GetGUID());
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_dru_solar_beam_AuraScript::HandleEffectPeriodic, EFFECT_2, SPELL_AURA_PERIODIC_DUMMY);
            }

        };

        AuraScript* GetAuraScript() const
        {
            return new spell_dru_solar_beam_AuraScript();
        }
};

class wild_mushroom_and_treant_npc : public CreatureScript
{
public:
   wild_mushroom_and_treant_npc() : CreatureScript("wild_mushroom_and_treant_npc") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new wild_mushroom_and_treant_npcAI (creature);
    }

    struct wild_mushroom_and_treant_npcAI : public ScriptedAI
    {
        wild_mushroom_and_treant_npcAI(Creature* creature) : ScriptedAI(creature) 
        {
            if(IsMushroom())
                me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
        }

        enum spells
        {
            FUNGAL_GROWTH_RANK1     = 78788,
            FUNGAL_GROWTH_RANK2     = 78789,

            TRIGGERING_AURA_R1      = 81289,
            TRIGGERING_AURA_R2      = 81282,

            FUNGAL_VISUAL           = 94339,
            FUNGAL_COOLDOWN         = 81283
        };

        uint32 Stealth_timer;

        bool IsMushroom(void)
        {
            return (me->GetEntry() == 47649) ? true : false ;
        }

        void Reset()
        {
            Stealth_timer = 6000;
        }

        void JustDied(Unit * victim)
        {
            if(IsMushroom() && victim != me) // if mushroom is killed by someone else, do nothing
                return;

            Player * pPlayer = NULL;

            if (me->ToTempSummon() && me->ToTempSummon()->GetSummoner())
                pPlayer = me->ToTempSummon()->GetSummoner()->ToPlayer();

            if(pPlayer == NULL)
                return;

            if(pPlayer->getClass() != CLASS_DRUID) // Only druids
                return;

            if (!pPlayer->HasAura(FUNGAL_GROWTH_RANK1) && !pPlayer->HasAura(FUNGAL_GROWTH_RANK2) ) // No talent no deal
                return;

            TempSummon* summon = pPlayer->SummonCreature(43484, me->GetPositionX(),me->GetPositionY(),me->GetPositionZ(),0.0f,TEMPSUMMON_MANUAL_DESPAWN,0);

            if (!summon)
                return;

            summon->SetLevel(pPlayer->getLevel());
            summon->ForcedDespawn(20000);
            summon->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
            summon->SetUInt64Value(UNIT_FIELD_SUMMONEDBY, pPlayer->GetGUID());
            summon->setFaction(pPlayer->getFaction());
            summon->SetUInt32Value(UNIT_CREATED_BY_SPELL, 78777);

            summon->CastSpell(summon,FUNGAL_VISUAL,true);
            Aura * aur = summon->GetAura(FUNGAL_VISUAL);
            if (aur)
                aur->SetDuration(20000); // Mistake in DB ?? ( 30s before )

            if(pPlayer->HasAura(FUNGAL_GROWTH_RANK1))
                summon->CastSpell(summon, TRIGGERING_AURA_R1, true);
            else
                summon->CastSpell(summon, TRIGGERING_AURA_R2, true);

            summon->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_PACIFIED); // Dont attack
        }

        void UpdateAI (const uint32 diff)
        {
            if(Stealth_timer <= diff)
            {
                if(IsMushroom())
                    me->CastSpell(me,92661,true); // Turn "Invisible" - > actually it is SPELL_AURA_MOD_STEALTH
                Stealth_timer = 60000;
            }
            else Stealth_timer -= diff;

            if(!IsMushroom())
                DoMeleeAttackIfReady();
        }
    };
};

void AddSC_druid_spell_scripts()
{
    new spell_dru_t10_restoration_4p_bonus();
    new spell_druid_blood_in_water();
    new spell_druid_pulverize();
    new spell_druid_insect_swarm();
    new spell_dru_mush_detonate(); // INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (78777, 'spell_dru_mush_detonate');

    new spell_dru_tranquility(); // INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (44203, 'spell_dru_tranquility');
    new spell_dru_solar_beam(); // INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (78675, 'spell_dru_solar_beam');
    new spell_dru_efflo_periodic(); // INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (81262, 'spell_dru_efflo_periodic');
    new wild_mushroom_and_treant_npc();
    //UPDATE `creature_template` SET `ScriptName`='wild_mushroom_and_treant_npc' WHERE  `entry`=1964 LIMIT 1;
    //UPDATE `creature_template` SET `ScriptName`='wild_mushroom_and_treant_npc' WHERE  `entry`=47649 LIMIT 1;
}
