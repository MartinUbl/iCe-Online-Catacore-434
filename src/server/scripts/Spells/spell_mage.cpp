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

                    if (spellInfo->SpellFamilyName == SPELLFAMILY_MAGE &&
                        (GetSpellSchoolMask(spellInfo) & SPELL_SCHOOL_MASK_FROST) &&
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

class spell_mage_polymorph_cast_visual : public SpellScriptLoader
{
    public:
        spell_mage_polymorph_cast_visual() : SpellScriptLoader("spell_mage_polymorph_visual") { }

        class spell_mage_polymorph_cast_visual_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_polymorph_cast_visual_SpellScript)
            static const uint32 spell_list[6];

            bool Validate(SpellEntry const * /*spellEntry*/)
            {
                // check if spell ids exist in dbc
                for (int i = 0; i < 6; i++)
                    if (!sSpellStore.LookupEntry(spell_list[i]))
                        return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Unit *unitTarget = GetHitUnit())
                    if (unitTarget->GetTypeId() == TYPEID_UNIT)
                        unitTarget->CastSpell(unitTarget, spell_list[urand(0, 5)], true);
            }

            void Register()
            {
                // add dummy effect spell handler to Polymorph visual
                OnEffect += SpellEffectFn(spell_mage_polymorph_cast_visual_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_polymorph_cast_visual_SpellScript();
        }
};

const uint32 spell_mage_polymorph_cast_visual::spell_mage_polymorph_cast_visual_SpellScript::spell_list[6] =
{
    SPELL_MAGE_SQUIRREL_FORM,
    SPELL_MAGE_GIRAFFE_FORM,
    SPELL_MAGE_SERPENT_FORM,
    SPELL_MAGE_DRAGONHAWK_FORM,
    SPELL_MAGE_WORGEN_FORM,
    SPELL_MAGE_SHEEP_FORM
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

            target->CastSpell(target, SPELL_CAUTERIZE_DAMAGE, true);
            target->ToPlayer()->AddSpellCooldown(SPELL_CAUTERIZE_DAMAGE, 0, 60000);

            uint32 health40 = target->CountPctFromMaxHealth(40);

            // hp > 40% - absorb hp till 40%
            if (target->GetHealth() > health40)
                absorbAmount = dmgInfo.GetDamage() - target->GetHealth() + health40;
            // hp lower than 40% - absorb everything
            else
            {
                absorbAmount = dmgInfo.GetDamage();
                target->SetHealth(health40);
            }
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
            uint8 auraEff_id;
            uint32 aura_bp;
            struct auraRecord *next;
        }DOT;

        #define LIVING_BOMB 44457

        Player * caster;
        DOT* begin;
        Unit * exclude_target;
        uint32 living_bomb_counter;

        bool Load()
        {
            if (GetCaster()->GetTypeId() != TYPEID_PLAYER || GetCaster()->ToPlayer()->getClass() != CLASS_MAGE )
                return false;

            caster = GetCaster()->ToPlayer();
            begin = NULL;
            exclude_target = NULL;
            return true;
        }

        DOT * alloc(void)
        {
            DOT * temp;
            temp = (DOT*)malloc(sizeof(DOT));
            if(!temp)
                return NULL;
            else
            {
                temp->next = NULL;
                return temp;
            }
        }

        void Push(DOT *_new)
        {
            if(begin == NULL) // Inserting first time
            {
                begin =_new;
                return;
            }

            DOT* akt = begin;

            while(akt->next) // Look up end of SLL
            {
                akt= akt->next;
            }

            akt->next = _new; // And insert at the end  (cause there were trouble with inserting in beginning)
        }


        void HandleImpactEffect(SpellEffIndex /*effIndex*/)
        {
            living_bomb_counter = 0;
            exclude_target = GetHitUnit();

            if(!exclude_target)
                return;

            Unit::AuraApplicationMap const& auras = exclude_target->GetAppliedAuras();
            for (Unit::AuraApplicationMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            {
                Aura* aura = itr->second->GetBase();

                if(aura && aura->GetCaster() != NULL && aura->GetCaster()->ToPlayer()
                  && (aura->GetCaster()->ToPlayer() == caster) && aura->GetSpellProto() && aura->GetSpellProto()->SchoolMask == SPELL_SCHOOL_MASK_FIRE &&
                  (aura->HasEffectType(SPELL_AURA_PERIODIC_DAMAGE) || aura->HasEffectType(SPELL_AURA_PERIODIC_TRIGGER_SPELL)))
                {
                    DOT *current = alloc();
                    if(!current)
                        continue;

                    current->aura_id = aura->GetId();
                    current->aura_duration = aura->GetDuration();
                    current->auraEff_id = -1; // For sure -1 means fail

                    for(uint8 i = 0; i < MAX_SPELL_EFFECTS ; i++) // Need to find right id of effect where needed aura' are
                    {
                        AuraEffect* aurEff = aura->GetEffect(i);

                        if(aurEff && ( aurEff->GetAuraType() == SPELL_AURA_PERIODIC_DAMAGE || aurEff->GetAuraType() == SPELL_AURA_PERIODIC_TRIGGER_SPELL))
                        {
                            current->auraEff_id = i;
                            current->aura_per_timer = aurEff->GetPeriodicTimer();
                            current->aura_bp =aurEff->GetAmount();
                            break;
                        }
                    }

                    Push(current); // Push current aura record to SLL
                }
            }
        }

        void HandleEffectScriptEffect(SpellEffIndex /*effIndex*/)
        {
            if (!GetHitUnit() || GetHitUnit() == exclude_target || !begin)
                return;

            Unit * hit_unit = GetHitUnit();

            if(!exclude_target->IsWithinLOS(hit_unit->GetPositionX(),hit_unit->GetPositionY(),hit_unit->GetPositionZ())) // Blizz hotfix
                return;

            DOT* akt = begin;

            while(akt)
            {
                if(akt->aura_id == LIVING_BOMB)
                    living_bomb_counter++;

                if(akt->aura_id != LIVING_BOMB || living_bomb_counter < 3) // We can copy LIVING_BOMB max 2 times
                {
                    caster->AddAura(akt->aura_id,hit_unit);// Add according aura

                    if(Aura *nova = hit_unit->GetAura(akt->aura_id,caster->GetGUID()))
                    {
                        nova->SetDuration(akt->aura_duration); // Set according duration

                        if(akt->auraEff_id != -1)
                            if (AuraEffect* aurEff = nova->GetEffect(akt->auraEff_id))
                            {
                                aurEff->SetPeriodicTimer(akt->aura_per_timer); // Need synchronize periodic timer
                                aurEff->SetAmount(akt->aura_bp);
                            }
                    }
                 }

                akt = akt->next; // look for another aura in list
            }
        }


            void Register()
            {
                OnEffect += SpellEffectFn(spell_mage_impact_SpellScript::HandleImpactEffect, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
                OnEffect += SpellEffectFn(spell_mage_impact_SpellScript::HandleEffectScriptEffect, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

    SpellScript* GetSpellScript() const
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

            if(GetHitUnit()->hasUnitState(UNIT_STAT_CASTING))
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

void AddSC_mage_spell_scripts()
{
    new spell_mage_cold_snap;
    new spell_mage_frost_warding_trigger();
    new spell_mage_incanters_absorbtion_absorb();
    new spell_mage_incanters_absorbtion_manashield();
    new spell_mage_polymorph_cast_visual;
    new spell_mage_cauterize();
    new spell_mage_impact();
    new spell_mage_blizzard();
    new spell_mage_invocation();
    new spell_mage_reactive_barrier();
}
