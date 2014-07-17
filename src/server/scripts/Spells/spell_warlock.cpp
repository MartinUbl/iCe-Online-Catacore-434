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
 * Scripts for spells with SPELLFAMILY_WARLOCK and SPELLFAMILY_GENERIC spells used by warlock players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_warl_".
 */

#include "ScriptPCH.h"
#include "Spell.h"
#include "SpellAuraEffects.h"

enum WarlockSpells
{
    WARLOCK_DEMONIC_EMPOWERMENT_SUCCUBUS    = 54435,
    WARLOCK_DEMONIC_EMPOWERMENT_VOIDWALKER  = 54443,
    WARLOCK_DEMONIC_EMPOWERMENT_FELGUARD    = 54508,
    WARLOCK_DEMONIC_EMPOWERMENT_FELHUNTER   = 54509,
    WARLOCK_DEMONIC_EMPOWERMENT_IMP         = 54444,
    //WARLOCK_IMPROVED_HEALTHSTONE_R1         = 18692,
    //WARLOCK_IMPROVED_HEALTHSTONE_R2         = 18693,
    WARLOCK_FELHUNTER_SHADOWBITE            = 54049,
};

// 47193 Demonic Empowerment
class spell_warl_demonic_empowerment : public SpellScriptLoader
{
public:
    spell_warl_demonic_empowerment() : SpellScriptLoader("spell_warl_demonic_empowerment") { }

    class spell_warl_demonic_empowerment_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warl_demonic_empowerment_SpellScript)
        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            if (!sSpellStore.LookupEntry(WARLOCK_DEMONIC_EMPOWERMENT_SUCCUBUS))
                return false;
            if (!sSpellStore.LookupEntry(WARLOCK_DEMONIC_EMPOWERMENT_VOIDWALKER))
                return false;
            if (!sSpellStore.LookupEntry(WARLOCK_DEMONIC_EMPOWERMENT_FELGUARD))
                return false;
            if (!sSpellStore.LookupEntry(WARLOCK_DEMONIC_EMPOWERMENT_FELHUNTER))
                return false;
            if (!sSpellStore.LookupEntry(WARLOCK_DEMONIC_EMPOWERMENT_IMP))
                return false;
            return true;
        }

        void HandleScriptEffect(SpellEffIndex /*effIndex*/)
        {
            if (Creature* targetCreature = GetHitCreature())
            {
                if (targetCreature->IsPet())
                {
                    CreatureInfo const * ci = sObjectMgr->GetCreatureTemplate(targetCreature->GetEntry());
                    switch (ci->family)
                    {
                    case CREATURE_FAMILY_SUCCUBUS:
                        targetCreature->CastSpell(targetCreature, WARLOCK_DEMONIC_EMPOWERMENT_SUCCUBUS, true);
                        break;
                    case CREATURE_FAMILY_VOIDWALKER:
                    {
                        SpellEntry const* spellInfo = sSpellStore.LookupEntry(WARLOCK_DEMONIC_EMPOWERMENT_VOIDWALKER);
                        int32 hp = int32(targetCreature->CountPctFromMaxHealth(GetCaster()->CalculateSpellDamage(targetCreature, spellInfo, 0)));
                        targetCreature->CastCustomSpell(targetCreature, WARLOCK_DEMONIC_EMPOWERMENT_VOIDWALKER, &hp, NULL, NULL, true);
                        //unitTarget->CastSpell(unitTarget, 54441, true);
                        break;
                    }
                    case CREATURE_FAMILY_FELGUARD:
                        targetCreature->CastSpell(targetCreature, WARLOCK_DEMONIC_EMPOWERMENT_FELGUARD, true);
                        break;
                    case CREATURE_FAMILY_FELHUNTER:
                        targetCreature->CastSpell(targetCreature, WARLOCK_DEMONIC_EMPOWERMENT_FELHUNTER, true);
                        break;
                    case CREATURE_FAMILY_IMP:
                        targetCreature->CastSpell(targetCreature, WARLOCK_DEMONIC_EMPOWERMENT_IMP, true);
                        break;
                    }
                }
            }
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_warl_demonic_empowerment_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_warl_demonic_empowerment_SpellScript();
    }
};

// 6201 Create Healthstone
class spell_warl_create_healthstone : public SpellScriptLoader
{
public:
    spell_warl_create_healthstone() : SpellScriptLoader("spell_warl_create_healthstone") { }

    class spell_warl_create_healthstone_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warl_create_healthstone_SpellScript)

        //deprecated. removed in cataclysm
        /*static uint32 const iTypes[8][3];

        bool Validate(SpellEntry const spellEntry)
        {
            if (!sSpellStore.LookupEntry(WARLOCK_IMPROVED_HEALTHSTONE_R1))
                return false;
            if (!sSpellStore.LookupEntry(WARLOCK_IMPROVED_HEALTHSTONE_R2))
                return false;
            return true;
        }*/

        void HandleScriptEffect(SpellEffIndex effIndex)
        {
            if (GetHitUnit())
            {
                //spell ranking deprecated in cataclysm
                /*uint32 rank = 0;
                // Improved Healthstone
                if (AuraEffect const * aurEff = unitTarget->GetDummyAuraEffect(SPELLFAMILY_WARLOCK, 284, 0))
                {
                    switch (aurEff->GetId())
                    {
                        case WARLOCK_IMPROVED_HEALTHSTONE_R1: rank = 1; break;
                        case WARLOCK_IMPROVED_HEALTHSTONE_R2: rank = 2; break;
                        default:
                            sLog->outError("Unknown rank of Improved Healthstone id: %d", aurEff->GetId());
                            break;
                    }
                }
                uint8 spellRank = sSpellMgr->GetSpellRank(GetSpellInfo()->Id);
                if (spellRank > 0 && spellRank <= 8)*/
                    CreateItem(effIndex, 5512);
            }
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_warl_create_healthstone_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_warl_create_healthstone_SpellScript();
    }
};

class spell_warl_seed_of_corruption : public SpellScriptLoader
{
    public:
        spell_warl_seed_of_corruption() : SpellScriptLoader("spell_warl_seed_of_corruption") { }

        class spell_warl_seed_of_corruption_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_seed_of_corruption_SpellScript);

            bool soulShardGained;

            bool Load()
            {
                soulShardGained = false;
                return true;
            }

            void HandleExtraEffect(SpellEffIndex effIndex)
            {
                Unit * caster = GetCaster();
                Unit * unit = GetHitUnit();

                if (!caster || !unit || !caster->HasAura(86664)) // Soulburn: Seed of Corruption ( talent )
                    return;

                if (GetSpellInfo()->Id == 27285 || GetSpellInfo()->Id == 32865) // Soulburn: Seed of Corruption
                {
                    caster->CastSpell(unit,172,true); // Corruption

                    if (soulShardGained == false)
                    {
                        soulShardGained = true;
                        caster->CastSpell(caster,87388,true); // Gain 1 soul shard
                    }
                }
            }

            void Register()
            {
                OnEffect += SpellEffectFn(spell_warl_seed_of_corruption_SpellScript::HandleExtraEffect, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript *GetSpellScript() const
        {
            return new spell_warl_seed_of_corruption_SpellScript();
        }
};

// 54049 Shadow Bite
class spell_warl_shadow_bite : public SpellScriptLoader
{
public:
    spell_warl_shadow_bite() : SpellScriptLoader("spell_warl_shadow_bite") { }

    class spell_warl_shadow_bite_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warl_shadow_bite_SpellScript)
        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            if (!sSpellStore.LookupEntry(WARLOCK_FELHUNTER_SHADOWBITE))
                return false;
            return true;
        }

        void HandleDamage(SpellEffIndex /*effIndex*/)
        {
            Unit *caster = GetCaster();

            if (!caster)
                return;

            Unit *owner = caster->GetOwner();

            if (!owner)
                return;

            int32 damage = GetHitDamage();

            if (AuraEffect * aurEff = owner->GetAuraEffect(18694,2))        // Dark Arts (Rank 1)
                damage *= ((aurEff)->GetAmount() + 100.0f) / 100.0f;
            else if (AuraEffect * aurEff = owner->GetAuraEffect(85283,2))   // Dark Arts (Rank 2)
                damage *= ((aurEff)->GetAmount() + 100.0f) / 100.0f;
            else if (AuraEffect * aurEff = owner->GetAuraEffect(85284,2))   // Dark Arts (Rank 3)
                damage *= ((aurEff)->GetAmount() + 100.0f) / 100.0f;

            SetHitDamage(damage);
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_warl_shadow_bite_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_warl_shadow_bite_SpellScript();
    }
};

class npc_hand_of_guldan: public CreatureScript
{
    public:
        npc_hand_of_guldan(): CreatureScript("npc_hand_of_guldan") { };

        struct npc_hand_of_guldanAI: public Scripted_NoMovementAI
        {
            npc_hand_of_guldanAI(Creature* c): Scripted_NoMovementAI(c)
            {
                Reset();
            }

            void InitializeAI()
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                //me->SetReactState(REACT_PASSIVE);

                me->CastSpell(me, 85526, true);
                me->CastSpell(me, 86000, true);

                me->SetRooted(true);
            }

            void Reset()
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                //me->SetReactState(REACT_PASSIVE);
            }

            void AttackStart(Unit* pWho) { return; };
            void Aggro(Unit* pWho) { return; };
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_hand_of_guldanAI(c);
        };
};

void AddSC_warlock_spell_scripts()
{
    new spell_warl_demonic_empowerment();
    new spell_warl_create_healthstone();
    new spell_warl_seed_of_corruption();
    new spell_warl_shadow_bite();
    new npc_hand_of_guldan();
}
