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
 * Scripts for spells with SPELLFAMILY_WARRIOR and SPELLFAMILY_GENERIC spells used by warrior players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_warr_".
 */

#include "ScriptPCH.h"

enum WarriorSpells
{
    WARRIOR_SPELL_LAST_STAND_TRIGGERED  = 12976,
    WARRIOR_2P_PROTECTION_SET_BONUS     = 105908,
    WARRIOR_SPELL_SHIELD_OF_FURY        = 105909
};

class spell_warr_last_stand : public SpellScriptLoader
{
    public:
        spell_warr_last_stand() : SpellScriptLoader("spell_warr_last_stand") { }

        class spell_warr_last_stand_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warr_last_stand_SpellScript)

            bool Validate(SpellEntry const * /*spellEntry*/) override
            {
                return sSpellStore.LookupEntry(WARRIOR_SPELL_LAST_STAND_TRIGGERED) != nullptr;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                int32 healthModSpellBasePoints0 = int32(GetCaster()->CountPctFromMaxHealth(30));
                GetCaster()->CastCustomSpell(GetCaster(), WARRIOR_SPELL_LAST_STAND_TRIGGERED, &healthModSpellBasePoints0, nullptr, nullptr, true, nullptr);
            }

            void Register() override
            {
                OnEffect += SpellEffectFn(spell_warr_last_stand_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript *GetSpellScript() const override
        {
            return new spell_warr_last_stand_SpellScript();
        }
};

class spell_warr_revenge : public SpellScriptLoader
{
public:
    spell_warr_revenge() : SpellScriptLoader("spell_warr_revenge") { }

    class spell_warr_revenge_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warr_revenge_SpellScript)

        bool Validate(SpellEntry const * /*spellEntry*/) override
        {
            return sSpellStore.LookupEntry(WARRIOR_SPELL_SHIELD_OF_FURY) != nullptr;
        }

        void HandleRevengeHit(SpellEffIndex /*effIndex*/)
        {
            if (Unit * caster = GetCaster())
            {
                if (AuraEffect * aurEff = caster->GetAuraEffect(WARRIOR_2P_PROTECTION_SET_BONUS, EFFECT_0))
                {
                    int32 bp0 = GetHitDamage() * aurEff->GetAmount() / 100;
                    caster->CastCustomSpell(caster, WARRIOR_SPELL_SHIELD_OF_FURY, &bp0, nullptr, nullptr, true, nullptr);
                }
            }
        }

        void Register() override
        {
            OnEffect += SpellEffectFn(spell_warr_revenge_SpellScript::HandleRevengeHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript *GetSpellScript() const override
    {
        return new spell_warr_revenge_SpellScript();
    }
};

void AddSC_warrior_spell_scripts()
{
    new spell_warr_last_stand();
    new spell_warr_revenge();   // 6572
}
