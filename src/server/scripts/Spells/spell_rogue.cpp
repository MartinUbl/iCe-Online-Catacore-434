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
 * Scripts for spells with SPELLFAMILY_ROGUE and SPELLFAMILY_GENERIC spells used by rogue players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_rog_".
 */

#include "ScriptPCH.h"
#include "SpellAuraEffects.h"

enum RogueSpells
{
    ROGUE_SPELL_SHIV_TRIGGERED                   = 5940,
    ROGUE_SPELL_GLYPH_OF_PREPARATION             = 56819,
    ROGUE_SPELL_PREY_ON_THE_WEAK                 = 58670,
};

// Cheat Death
class spell_rog_cheat_death : public SpellScriptLoader
{
public:
    spell_rog_cheat_death() : SpellScriptLoader("spell_rog_cheat_death") { }

    class spell_rog_cheat_death_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_rog_cheat_death_AuraScript);

        uint32 absorbChance;

        enum Spell
        {
            ROG_SPELL_CHEAT_DEATH_COOLDOWN = 31231,
            ROG_SPELL_CHEAT_DEATH_DAMAGE_REDUCE = 45182,
        };

        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            return sSpellStore.LookupEntry(ROG_SPELL_CHEAT_DEATH_COOLDOWN);
        }

        bool Load()
        {
            absorbChance = SpellMgr::CalculateSpellEffectAmount(GetSpellProto(), EFFECT_0);
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
            if (dmgInfo.GetDamage() < target->GetHealth())
                return;
            if (target->ToPlayer()->HasSpellCooldown(ROG_SPELL_CHEAT_DEATH_COOLDOWN))
                return;
            if (!roll_chance_i(absorbChance))
                return;

            target->CastSpell(target, ROG_SPELL_CHEAT_DEATH_COOLDOWN, true);
            target->ToPlayer()->AddSpellCooldown(ROG_SPELL_CHEAT_DEATH_COOLDOWN, 0, 90000);

            target->CastSpell(target, ROG_SPELL_CHEAT_DEATH_DAMAGE_REDUCE, true);

            uint32 health10 = target->CountPctFromMaxHealth(10);

            // hp > 10% - absorb hp till 10%
            if (target->GetHealth() > health10)
                absorbAmount = dmgInfo.GetDamage() - target->GetHealth() + health10;
            // hp lower than 10% - absorb everything
            else
                absorbAmount = dmgInfo.GetDamage();
        }

        void Register()
        {
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_rog_cheat_death_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
            OnEffectAbsorb += AuraEffectAbsorbFn(spell_rog_cheat_death_AuraScript::Absorb, EFFECT_0);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_rog_cheat_death_AuraScript();
    }
};

class spell_rog_preparation : public SpellScriptLoader
{
    public:
        spell_rog_preparation() : SpellScriptLoader("spell_rog_preparation") { }

        class spell_rog_preparation_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_preparation_SpellScript)
            bool Validate(SpellEntry const * /*spellEntry*/)
            {
                if (!sSpellStore.LookupEntry(ROGUE_SPELL_GLYPH_OF_PREPARATION))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit *caster = GetCaster();
                if (caster->GetTypeId() != TYPEID_PLAYER)
                    return;

                // immediately finishes the cooldown on certain Rogue abilities

                // Shadowstep, Vanish, Sprint
                static const uint32 spellMap[] = {36554, 1856, 2983};
                // Kick, Dismantle, Smoke Bomb
                static const uint32 glyphSpellMap[] = {1766, 51722, 76577};

                for (uint32 i = 0; i < sizeof(spellMap)/sizeof(uint32); i++)
                    caster->ToPlayer()->RemoveSpellCooldown(spellMap[i], true);

                if (caster->HasAura(ROGUE_SPELL_GLYPH_OF_PREPARATION))
                {
                    for (uint32 i = 0; i < sizeof(glyphSpellMap)/sizeof(uint32); i++)
                        caster->ToPlayer()->RemoveSpellCooldown(glyphSpellMap[i], true);
                }
            }

            void Register()
            {
                // add dummy effect spell handler to Preparation
                OnEffect += SpellEffectFn(spell_rog_preparation_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript *GetSpellScript() const
        {
            return new spell_rog_preparation_SpellScript();
        }
};

// 51685-51689 Prey on the Weak
class spell_rog_prey_on_the_weak : public SpellScriptLoader
{
public:
    spell_rog_prey_on_the_weak() : SpellScriptLoader("spell_rog_prey_on_the_weak") { }

    class spell_rog_prey_on_the_weak_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_rog_prey_on_the_weak_AuraScript)
        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            if (!sSpellStore.LookupEntry(ROGUE_SPELL_PREY_ON_THE_WEAK))
                return false;
            return true;
        }

        void HandleEffectPeriodic(AuraEffect const * /*aurEff*/)
        {
            Unit* pTarget = GetTarget();
            Unit* pVictim = pTarget->GetVictim();
            if (pVictim && (pTarget->GetHealthPct() > pVictim->GetHealthPct()))
            {
                if (!pTarget->HasAura(ROGUE_SPELL_PREY_ON_THE_WEAK))
                {
                    int32 bp = SpellMgr::CalculateSpellEffectAmount(GetSpellProto(), 0);
                    pTarget->CastCustomSpell(pTarget, ROGUE_SPELL_PREY_ON_THE_WEAK, &bp, 0, 0, true);
                }
            }
            else
                pTarget->RemoveAurasDueToSpell(ROGUE_SPELL_PREY_ON_THE_WEAK);
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_rog_prey_on_the_weak_AuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_rog_prey_on_the_weak_AuraScript();
    }
};


class spell_rog_shiv : public SpellScriptLoader
{
    public:
        spell_rog_shiv() : SpellScriptLoader("spell_rog_shiv") { }

        class spell_rog_shiv_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_shiv_SpellScript)
            bool Validate(SpellEntry const * /*spellEntry*/)
            {
                if (!sSpellStore.LookupEntry(ROGUE_SPELL_SHIV_TRIGGERED))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit *caster = GetCaster();
                if (caster->GetTypeId() != TYPEID_PLAYER)
                    return;

                if (Unit *unitTarget = GetHitUnit())
                    caster->CastSpell(unitTarget, ROGUE_SPELL_SHIV_TRIGGERED, true);
            }

            void Register()
            {
                // add dummy effect spell handler to Shiv
                OnEffect += SpellEffectFn(spell_rog_shiv_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript *GetSpellScript() const
        {
            return new spell_rog_shiv_SpellScript();
        }
};

class spell_rog_deadly_poison : public SpellScriptLoader
{
public:
    spell_rog_deadly_poison() : SpellScriptLoader("spell_rog_deadly_poison") { }

    class spell_rog_deadly_poison_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_rog_deadly_poison_SpellScript)

    private:
        uint8 m_stackAmount;
    public:
        spell_rog_deadly_poison_SpellScript() : m_stackAmount(0) { }

        void HandleBeforeHit()
        {
            Player * player = GetCaster()->ToPlayer();
            Unit * target = GetHitUnit();

            if (!player || !target)
                return;

            // Deadly Poison
            if (AuraEffect const * aurEff = target->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_ROGUE, 0x10000, 0x80000, 0, player->GetGUID()))
                m_stackAmount = aurEff->GetBase()->GetStackAmount();
        }

        void HandleAfterHit()
        {
            if (m_stackAmount < 5)
                return;

            Player * player = GetCaster()->ToPlayer();
            Unit * target = GetHitUnit();
            Item * castItem = GetCastItem();

            if (!player || !target || !castItem)
                return;

            Item * item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);

            if (item == castItem)
                item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);

            if (!item)
                return;

            // item combat enchantments
            for (uint8 e_slot = 0; e_slot < MAX_ENCHANTMENT_SLOT; ++e_slot)
            {
                if (e_slot == REFORGING_ENCHANTMENT_SLOT || e_slot == TRANSMOGRIFY_ENCHANTMENT_SLOT)
                    continue;

                uint32 enchant_id = item->GetEnchantmentId(EnchantmentSlot(e_slot));
                SpellItemEnchantmentEntry const * pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);

                if (!pEnchant)
                    continue;

                for (uint8 s = 0; s < 3; ++s)
                {
                    if (pEnchant->type[s] != ITEM_ENCHANTMENT_TYPE_COMBAT_SPELL)
                        continue;

                    SpellEntry const *spellInfo = sSpellStore.LookupEntry(pEnchant->spellid[s]);

                    if (!spellInfo)
                    {
                        sLog->outError("Player::CastItemCombatSpell Enchant %i, cast unknown spell %i", pEnchant->ID, pEnchant->spellid[s]);
                        continue;
                    }

                    // Proc only rogue poisons
                    if ((spellInfo->SpellFamilyName != SPELLFAMILY_ROGUE) || (spellInfo->Dispel != DISPEL_POISON))
                        continue;

                    // Do not reproc deadly
                    if (spellInfo->SpellFamilyFlags.IsEqual(0x10000, 0x80000, 0))
                        continue;

                    if (IsPositiveSpell(pEnchant->spellid[s]))
                        player->CastSpell(player, pEnchant->spellid[s], true, item);
                    else
                        player->CastSpell(target, pEnchant->spellid[s], true, item);
                }
            }
        }

        void Register()
        {
            BeforeHit += SpellHitFn(spell_rog_deadly_poison_SpellScript::HandleBeforeHit);
            AfterHit += SpellHitFn(spell_rog_deadly_poison_SpellScript::HandleAfterHit);
        }
    };

    SpellScript * GetSpellScript() const
    {
       return new spell_rog_deadly_poison_SpellScript();
    }
};

class spell_rog_bandits_guile : public SpellScriptLoader
{
public:
    spell_rog_bandits_guile(): SpellScriptLoader("spell_rog_bandits_guile") {}

    class spell_rog_bandits_guile_AuraScript : public AuraScript
    {
    public:
        PrepareAuraScript(spell_rog_bandits_guile_AuraScript)
        enum Spells
        {
            BUFF1 = 84745,
            BUFF2 = 84746,
            BUFF3 = 84747
        };

        Unit *caster;
        Unit *target;
        int charges;
        int insight;
        Aura *buffIns;      // 84745 - 7
        Aura *buff;         // 84748
        int32 duration;

        bool Validate(SpellEntry const *)
        {
            return sSpellStore.LookupEntry(BUFF1) && sSpellStore.LookupEntry(BUFF2) && sSpellStore.LookupEntry(BUFF3);
        }

        bool Load()
        {
            Unit *owner = GetUnitOwner();
            caster = GetCaster();
            if(!caster || !owner)
                return false;

            buff = owner->GetAura(84748, caster->GetGUID());

            if (buff)
            {
                duration = buff->GetDuration();

                AuraEffect *aurEff = buff->GetEffect(0);
                if(!aurEff)
                    return false;
                switch(aurEff->GetAmount())
                {
                    case 10: insight = BUFF1; break;
                    case 20: insight = BUFF2; break;
                    case 30: insight = BUFF3; break;
                    default: insight = 0; break;
                }
                buffIns = caster->GetAura(insight);
                charges = buff->GetCharges();
            }
            else
            {
                buffIns = NULL;
                insight = 0;
                charges = 1;
            }

            return true;
        }

        void Unload()
        {
        }

        void HandleEffectApply(AuraEffect const * aurEff, AuraEffectHandleModes mode)
        {
            Aura *b = aurEff->GetBase();
            if (!b)
                return;

            target = aurEff->GetBase()->GetUnitOwner();
            AuraEffect *pFrst = NULL, *pScnd = NULL;

            pFrst = b->GetEffect(0);
            pScnd = b->GetEffect(1);

            if (!pFrst || !pScnd)
                return;

            if (!buff)
            {
                b->SetCharges(1);
                pFrst->ChangeAmount(0);
                pScnd->ChangeAmount(0);
                if (buffIns)
                    buffIns->Remove();
            }
            else if (insight == BUFF3)
            {
                pFrst->ChangeAmount(30);
                pScnd->ChangeAmount(30);
                this->SetDuration(duration);
            }
            else
            {
                charges++;

                if (charges == 4)
                {
                    charges = 0;
                    if (buffIns)
                        buffIns->Remove();

                    switch (insight)
                    {
                        case 0: insight = BUFF1; break;
                        case BUFF1: insight = BUFF2; break;
                        case BUFF2: insight = BUFF3; break;
                    }
                    caster->CastSpell(caster, insight, true);
                }
                else
                {
                    b->SetCharges(charges);
                    if (insight && buffIns)
                        buffIns->RefreshDuration();
                }
                int bonus = 0;
                if (insight)
                     bonus = (insight - BUFF1 + 1) * 10;

                pFrst->ChangeAmount(bonus);
                pScnd->ChangeAmount(bonus);
            }
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_rog_bandits_guile_AuraScript::HandleEffectApply, EFFECT_1, SPELL_AURA_MOD_AUTOATTACK_DAMAGE_1, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_rog_bandits_guile_AuraScript();
    }
};

void AddSC_rogue_spell_scripts()
{
    new spell_rog_cheat_death();
    new spell_rog_preparation();
    new spell_rog_prey_on_the_weak();
    new spell_rog_shiv();
    new spell_rog_deadly_poison();
    new spell_rog_bandits_guile();
}
