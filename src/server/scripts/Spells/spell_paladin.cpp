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
 * Scripts for spells with SPELLFAMILY_PALADIN and SPELLFAMILY_GENERIC spells used by paladin players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_pal_".
 */

#include "ScriptPCH.h"
#include "SpellAuraEffects.h"

enum PaladinSpells
{
    PALADIN_SPELL_DIVINE_PLEA                    = 54428,
    
    PALADIN_SPELL_HOLY_SHOCK_R1                  = 20473,
    PALADIN_SPELL_HOLY_SHOCK_R1_DAMAGE           = 25912,
    PALADIN_SPELL_HOLY_SHOCK_R1_HEALING          = 25914,

    SPELL_BLESSING_OF_LOWER_CITY_DRUID           = 37878,
    SPELL_BLESSING_OF_LOWER_CITY_PALADIN         = 37879,
    SPELL_BLESSING_OF_LOWER_CITY_PRIEST          = 37880,
    SPELL_BLESSING_OF_LOWER_CITY_SHAMAN          = 37881
};

// 31850 - Ardent Defender
class spell_pal_ardent_defender : public SpellScriptLoader
{
public:
    spell_pal_ardent_defender() : SpellScriptLoader("spell_pal_ardent_defender") { }

    class spell_pal_ardent_defender_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_pal_ardent_defender_AuraScript);

        uint32 absorbPct, healPct;

        enum Spell
        {
            PAL_SPELL_ARDENT_DEFENDER_HEAL = 66235,
        };

        bool Load()
        {
            healPct = SpellMgr::CalculateSpellEffectAmount(GetSpellProto(), EFFECT_1);
            absorbPct = SpellMgr::CalculateSpellEffectAmount(GetSpellProto(), EFFECT_0);
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

            // we carry on
            if (dmgInfo.GetDamage() < target->GetHealth())
            {
                absorbAmount = CalculatePctN(dmgInfo.GetDamage(), absorbPct);
                return;
            }

            // damage kills us
            int32 healAmount = int32(target->CountPctFromMaxHealth(healPct));
            target->CastCustomSpell(target, PAL_SPELL_ARDENT_DEFENDER_HEAL, &healAmount, NULL, NULL, true);
            absorbAmount = dmgInfo.GetDamage();
            Remove(AURA_REMOVE_BY_ENEMY_SPELL);
        }

        void Register()
        {
             DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_pal_ardent_defender_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
             OnEffectAbsorb += AuraEffectAbsorbFn(spell_pal_ardent_defender_AuraScript::Absorb, EFFECT_0);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_pal_ardent_defender_AuraScript();
    }
};

class spell_pal_blessing_of_faith : public SpellScriptLoader
{
public:
    spell_pal_blessing_of_faith() : SpellScriptLoader("spell_pal_blessing_of_faith") { }

    class spell_pal_blessing_of_faith_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_pal_blessing_of_faith_SpellScript)
        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            if (!sSpellStore.LookupEntry(SPELL_BLESSING_OF_LOWER_CITY_DRUID))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_BLESSING_OF_LOWER_CITY_PALADIN))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_BLESSING_OF_LOWER_CITY_PRIEST))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_BLESSING_OF_LOWER_CITY_SHAMAN))
                return false;
            return true;
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            if (Unit *unitTarget = GetHitUnit())
            {
                uint32 spell_id = 0;
                switch(unitTarget->getClass())
                {
                    case CLASS_DRUID:   spell_id = SPELL_BLESSING_OF_LOWER_CITY_DRUID; break;
                    case CLASS_PALADIN: spell_id = SPELL_BLESSING_OF_LOWER_CITY_PALADIN; break;
                    case CLASS_PRIEST:  spell_id = SPELL_BLESSING_OF_LOWER_CITY_PRIEST; break;
                    case CLASS_SHAMAN:  spell_id = SPELL_BLESSING_OF_LOWER_CITY_SHAMAN; break;
                    default: return;                    // ignore for non-healing classes
                }

                GetCaster()->CastSpell(GetCaster(), spell_id, true);
            }
        }

        void Register()
        {
            // add dummy effect spell handler to Blessing of Faith
            OnEffect += SpellEffectFn(spell_pal_blessing_of_faith_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript *GetSpellScript() const
    {
        return new spell_pal_blessing_of_faith_SpellScript();
    }
};

class spell_pal_holy_shock : public SpellScriptLoader
{
public:
    spell_pal_holy_shock() : SpellScriptLoader("spell_pal_holy_shock") { }

    class spell_pal_holy_shock_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_pal_holy_shock_SpellScript)
        bool Validate(SpellEntry const *spellEntry)
        {
            if (!sSpellStore.LookupEntry(PALADIN_SPELL_HOLY_SHOCK_R1))
                return false;

            // can't use other spell than holy shock due to spell_ranks dependency
            if (sSpellMgr->GetFirstSpellInChain(PALADIN_SPELL_HOLY_SHOCK_R1) != sSpellMgr->GetFirstSpellInChain(spellEntry->Id))
                return false;

            uint8 rank = sSpellMgr->GetSpellRank(spellEntry->Id);
            if (!sSpellMgr->GetSpellWithRank(PALADIN_SPELL_HOLY_SHOCK_R1_DAMAGE, rank, true))
                return false;
            if (!sSpellMgr->GetSpellWithRank(PALADIN_SPELL_HOLY_SHOCK_R1_HEALING, rank, true))
                return false;

            return true;
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            if (Unit *unitTarget = GetHitUnit())
            {
                Unit *caster = GetCaster();

                uint8 rank = sSpellMgr->GetSpellRank(GetSpellInfo()->Id);

                if (caster->IsFriendlyTo(unitTarget))
                    caster->CastSpell(unitTarget, sSpellMgr->GetSpellWithRank(PALADIN_SPELL_HOLY_SHOCK_R1_HEALING, rank), true, 0);
                else
                    caster->CastSpell(unitTarget, sSpellMgr->GetSpellWithRank(PALADIN_SPELL_HOLY_SHOCK_R1_DAMAGE, rank), true, 0);
            }
        }

        void Register()
        {
            // add dummy effect spell handler to Holy Shock
            OnEffect += SpellEffectFn(spell_pal_holy_shock_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript *GetSpellScript() const
    {
        return new spell_pal_holy_shock_SpellScript();
    }
};

#define CONSECRATION_KIT_FRIENDLY 20251
#define CONSECRATION_KIT_ENEMY 16272

class npc_pal_consecration: public CreatureScript
{
public:
    npc_pal_consecration(): CreatureScript("npc_pal_consecration") {};

    struct consecrationAI: public Scripted_NoMovementAI
    {
        bool visualSent = false;

        consecrationAI(Creature* c): Scripted_NoMovementAI(c)
        {
            // Dummy visual
            // now sent by caster as spell visual kit in UpdateAI
            //me->CastSpell(me,81298,true);
            ticktimer = 1000;

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
        }

        uint32 ticktimer;

        void DamageTaken(Unit* pAttacker, uint32& damage)
        {
            damage = 0;

            // If we are still taking damage, something changed our flags and/or react state
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!visualSent && me->GetOwner())
            {
                visualSent = true;
                me->GetOwner()->SendPlaySpellVisualKitFactioned(CONSECRATION_KIT_FRIENDLY, CONSECRATION_KIT_ENEMY);
            }

            if(ticktimer <= diff)
            {
                Unit* creator = me->GetOwner();
                if (creator)
                    creator->CastSpell(me, 81297, true);

                ticktimer = 1000;
            } else ticktimer -= diff;
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new consecrationAI(c);
    }
};

void AddSC_paladin_spell_scripts()
{
    new spell_pal_ardent_defender();
    new spell_pal_blessing_of_faith();
    new spell_pal_holy_shock();
    new npc_pal_consecration();
}
