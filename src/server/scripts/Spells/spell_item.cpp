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
 * Scripts for spells with SPELLFAMILY_GENERIC spells used by items.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_item_".
 */

#include "ScriptPCH.h"

// Generic script for handling item dummy effects which trigger another spell.
class spell_item_trigger_spell : public SpellScriptLoader
{
private:
    uint32 _triggeredSpellId;

public:
    spell_item_trigger_spell(const char* name, uint32 triggeredSpellId) : SpellScriptLoader(name), _triggeredSpellId(triggeredSpellId) { }

    class spell_item_trigger_spell_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_item_trigger_spell_SpellScript)
    private:
        uint32 _triggeredSpellId;

    public:
        spell_item_trigger_spell_SpellScript(uint32 triggeredSpellId) : SpellScript(), _triggeredSpellId(triggeredSpellId) { }

        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            if (!sSpellStore.LookupEntry(_triggeredSpellId))
                return false;
            return true;
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            if (Item* pItem = GetCastItem())
                GetCaster()->CastSpell(GetCaster(), _triggeredSpellId, true, pItem);
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_item_trigger_spell_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_item_trigger_spell_SpellScript(_triggeredSpellId);
    }
};

// http://www.wowhead.com/item=6522 Deviate Fish
// 8063 Deviate Fish
enum eDeviateFishSpells
{
    SPELL_SLEEPY            = 8064,
    SPELL_INVIGORATE        = 8065,
    SPELL_SHRINK            = 8066,
    SPELL_PARTY_TIME        = 8067,
    SPELL_HEALTHY_SPIRIT    = 8068,
};

class spell_item_deviate_fish : public SpellScriptLoader
{
public:
    spell_item_deviate_fish() : SpellScriptLoader("spell_item_deviate_fish") { }

    class spell_item_deviate_fish_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_item_deviate_fish_SpellScript)
    public:
        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            for (uint32 spellId = SPELL_SLEEPY; spellId <= SPELL_HEALTHY_SPIRIT; ++spellId)
                if (!sSpellStore.LookupEntry(spellId))
                    return false;
            return true;
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* pCaster = GetCaster();
            if (pCaster->GetTypeId() != TYPEID_PLAYER)
                return;

            uint32 spellId = urand(SPELL_SLEEPY, SPELL_HEALTHY_SPIRIT);
            pCaster->CastSpell(pCaster, spellId, true, NULL);
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_item_deviate_fish_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_item_deviate_fish_SpellScript();
    }
};

// http://www.wowhead.com/item=47499 Flask of the North
// 67019 Flask of the North
enum eFlaskOfTheNorthSpells
{
    SPELL_FLASK_OF_THE_NORTH_SP = 67016,
    SPELL_FLASK_OF_THE_NORTH_AP = 67017,
    SPELL_FLASK_OF_THE_NORTH_STR = 67018,
};

class spell_item_flask_of_the_north : public SpellScriptLoader
{
public:
    spell_item_flask_of_the_north() : SpellScriptLoader("spell_item_flask_of_the_north") { }

    class spell_item_flask_of_the_north_SpellScript : public SpellScript
    {
    public:
        PrepareSpellScript(spell_item_flask_of_the_north_SpellScript)
        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            if (!sSpellStore.LookupEntry(SPELL_FLASK_OF_THE_NORTH_SP))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_FLASK_OF_THE_NORTH_AP))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_FLASK_OF_THE_NORTH_STR))
                return false;
            return true;
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* pCaster = GetCaster();
            if (pCaster->GetTypeId() != TYPEID_PLAYER)
                return;
            
            std::vector<uint32> possibleSpells;
            switch (pCaster->getClass())
            {
                case CLASS_WARLOCK:
                case CLASS_MAGE:
                case CLASS_PRIEST:
                    possibleSpells.push_back(SPELL_FLASK_OF_THE_NORTH_SP);
                    break;
                case CLASS_DEATH_KNIGHT:
                case CLASS_WARRIOR:
                    possibleSpells.push_back(SPELL_FLASK_OF_THE_NORTH_STR);
                    break;
                case CLASS_ROGUE:
                case CLASS_HUNTER:
                    possibleSpells.push_back(SPELL_FLASK_OF_THE_NORTH_AP);
                    break;
                case CLASS_DRUID:
                case CLASS_PALADIN:
                    possibleSpells.push_back(SPELL_FLASK_OF_THE_NORTH_SP);
                    possibleSpells.push_back(SPELL_FLASK_OF_THE_NORTH_STR);
                    break;
                case CLASS_SHAMAN:
                    possibleSpells.push_back(SPELL_FLASK_OF_THE_NORTH_SP);
                    possibleSpells.push_back(SPELL_FLASK_OF_THE_NORTH_AP);
                    break;
            }

            pCaster->CastSpell(pCaster, possibleSpells[irand(0, (possibleSpells.size() - 1))], true, NULL);
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_item_flask_of_the_north_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_item_flask_of_the_north_SpellScript();
    }
};

// http://www.wowhead.com/item=58149 Flask of Enhancement
// 79637 Flask of Enhancement
enum eFlaskOfEnhancementSpells
{
    SPELL_FLASK_OF_ENHANCEMENT_STR = 79638,
    SPELL_FLASK_OF_ENHANCEMENT_AGI = 79639,
    SPELL_FLASK_OF_ENHANCEMENT_INT = 79640,
};

class spell_item_flask_of_enhancement : public SpellScriptLoader
{
public:
    spell_item_flask_of_enhancement() : SpellScriptLoader("spell_item_flask_of_enhancement") { }

    class spell_item_flask_of_enhancement_SpellScript : public SpellScript
    {
    public:
        PrepareSpellScript(spell_item_flask_of_enhancement_SpellScript)
        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            if (!sSpellStore.LookupEntry(SPELL_FLASK_OF_ENHANCEMENT_STR))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_FLASK_OF_ENHANCEMENT_AGI))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_FLASK_OF_ENHANCEMENT_INT))
                return false;
            return true;
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* pCaster = GetCaster();
            if (pCaster->GetTypeId() != TYPEID_PLAYER)
                return;
            float agi = pCaster->ToPlayer()->GetTotalStatValue(STAT_AGILITY);
            float str = pCaster->ToPlayer()->GetTotalStatValue(STAT_STRENGTH);
            float inte = pCaster->ToPlayer()->GetTotalStatValue(STAT_INTELLECT);

            uint32 spellId = 0;

            if (agi>=str && agi>=inte)
            {
                spellId = 79639;
            }
            else if (str>=agi && str>=inte)
            {
                spellId = 79638;
            }
            else if (inte>=agi && inte>=str)
            {
                spellId = 79640;
            }
            if (spellId)
                pCaster->CastSpell(pCaster, spellId, true, NULL);
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_item_flask_of_enhancement_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_item_flask_of_enhancement_SpellScript();
    }
};

class go_food_feast_cataclysm : public GameObjectScript
{
public:
    go_food_feast_cataclysm() : GameObjectScript("go_food_feast_cataclysm") { uses = 0;}

    uint32 uses;

    bool OnGossipHello(Player * pPlayer, GameObject* pGo)
    {
        if(pPlayer && !pPlayer->isInCombat() && uses < 50)
        {
            pPlayer->CastSpell(pPlayer,87544,true); // Food
            uses++;
        }
        return false;
    }
};

// http://www.wowhead.com/item=10645 Gnomish Death Ray
// 13280 Gnomish Death Ray
enum eGnomishDeathRay
{
    SPELL_GNOMISH_DEATH_RAY_SELF = 13493,
    SPELL_GNOMISH_DEATH_RAY_TARGET = 13279,
};

class spell_item_gnomish_death_ray : public SpellScriptLoader
{
public:
    spell_item_gnomish_death_ray() : SpellScriptLoader("spell_item_gnomish_death_ray") { }

    class spell_item_gnomish_death_ray_SpellScript : public SpellScript
    {
    public:
        PrepareSpellScript(spell_item_gnomish_death_ray_SpellScript)
        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            if (!sSpellStore.LookupEntry(SPELL_GNOMISH_DEATH_RAY_SELF))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_GNOMISH_DEATH_RAY_TARGET))
                return false;
            return true;
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            if (Unit* pTarget = GetHitUnit())
            {
                Unit* pCaster = GetCaster();
                if (urand(0, 99) < 15)
                    pCaster->CastSpell(pCaster, SPELL_GNOMISH_DEATH_RAY_SELF, true, NULL);    // failure
                else
                    pCaster->CastSpell(pTarget, SPELL_GNOMISH_DEATH_RAY_TARGET, true, NULL);
            }
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_item_gnomish_death_ray_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_item_gnomish_death_ray_SpellScript();
    }
};

// http://www.wowhead.com/item=27388 Mr. Pinchy
// 33060 Make a Wish
enum eMakeAWish
{
    SPELL_MR_PINCHYS_BLESSING       = 33053,
    SPELL_SUMMON_MIGHTY_MR_PINCHY   = 33057,
    SPELL_SUMMON_FURIOUS_MR_PINCHY  = 33059,
    SPELL_TINY_MAGICAL_CRAWDAD      = 33062,
    SPELL_MR_PINCHYS_GIFT           = 33064,
};

class spell_item_make_a_wish : public SpellScriptLoader
{
public:
    spell_item_make_a_wish() : SpellScriptLoader("spell_item_make_a_wish") { }

    class spell_item_make_a_wish_SpellScript : public SpellScript
    {
    public:
        PrepareSpellScript(spell_item_make_a_wish_SpellScript)
        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            if (!sSpellStore.LookupEntry(SPELL_MR_PINCHYS_BLESSING))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_SUMMON_MIGHTY_MR_PINCHY))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_SUMMON_FURIOUS_MR_PINCHY))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_TINY_MAGICAL_CRAWDAD))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_MR_PINCHYS_GIFT))
                return false;
            return true;
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* pCaster = GetCaster();
            if (pCaster->GetTypeId() != TYPEID_PLAYER)
                return;

            uint32 spellId = SPELL_MR_PINCHYS_GIFT;
            switch (urand(1, 5))
            {
                case 1: spellId = SPELL_MR_PINCHYS_BLESSING; break;
                case 2: spellId = SPELL_SUMMON_MIGHTY_MR_PINCHY; break;
                case 3: spellId = SPELL_SUMMON_FURIOUS_MR_PINCHY; break;
                case 4: spellId = SPELL_TINY_MAGICAL_CRAWDAD; break;
            }
            pCaster->CastSpell(pCaster, spellId, true, NULL);
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_item_make_a_wish_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_item_make_a_wish_SpellScript();
    }
};

// http://www.wowhead.com/item=32686 Mingo's Fortune Giblets
// 40802 Mingo's Fortune Generator
class spell_item_mingos_fortune_generator : public SpellScriptLoader
{
public:
    spell_item_mingos_fortune_generator() : SpellScriptLoader("spell_item_mingos_fortune_generator") { }

    class spell_item_mingos_fortune_generator_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_item_mingos_fortune_generator_SpellScript)
        void HandleDummy(SpellEffIndex effIndex)
        {
            // Selecting one from Bloodstained Fortune item
            uint32 newitemid;
            switch (urand(1, 20))
            {
                case 1:  newitemid = 32688; break;
                case 2:  newitemid = 32689; break;
                case 3:  newitemid = 32690; break;
                case 4:  newitemid = 32691; break;
                case 5:  newitemid = 32692; break;
                case 6:  newitemid = 32693; break;
                case 7:  newitemid = 32700; break;
                case 8:  newitemid = 32701; break;
                case 9:  newitemid = 32702; break;
                case 10: newitemid = 32703; break;
                case 11: newitemid = 32704; break;
                case 12: newitemid = 32705; break;
                case 13: newitemid = 32706; break;
                case 14: newitemid = 32707; break;
                case 15: newitemid = 32708; break;
                case 16: newitemid = 32709; break;
                case 17: newitemid = 32710; break;
                case 18: newitemid = 32711; break;
                case 19: newitemid = 32712; break;
                case 20: newitemid = 32713; break;
                default:
                    return;
            }

            CreateItem(effIndex, newitemid);
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_item_mingos_fortune_generator_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_item_mingos_fortune_generator_SpellScript();
    }
};

// http://www.wowhead.com/item=10720 Gnomish Net-o-Matic Projector
// 13120 Net-o-Matic
enum eNetOMaticSpells
{
    SPELL_NET_O_MATIC_TRIGGERED1 = 16566,
    SPELL_NET_O_MATIC_TRIGGERED2 = 13119,
    SPELL_NET_O_MATIC_TRIGGERED3 = 13099,
};

class spell_item_net_o_matic : public SpellScriptLoader
{
public:
    spell_item_net_o_matic() : SpellScriptLoader("spell_item_net_o_matic") { }

    class spell_item_net_o_matic_SpellScript : public SpellScript
    {
    public:
        PrepareSpellScript(spell_item_net_o_matic_SpellScript)
        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            if (!sSpellStore.LookupEntry(SPELL_NET_O_MATIC_TRIGGERED1))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_NET_O_MATIC_TRIGGERED2))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_NET_O_MATIC_TRIGGERED3))
                return false;
            return true;
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            if (Unit* pTarget = GetHitUnit())
            {
                uint32 spellId = SPELL_NET_O_MATIC_TRIGGERED3;
                uint32 roll = urand(0, 99);
                if (roll < 2)                            // 2% for 30 sec self root (off-like chance unknown)
                    spellId = SPELL_NET_O_MATIC_TRIGGERED1;
                else if (roll < 4)                       // 2% for 20 sec root, charge to target (off-like chance unknown)
                    spellId = SPELL_NET_O_MATIC_TRIGGERED2;

                GetCaster()->CastSpell(pTarget, spellId, true, NULL);
            }
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_item_net_o_matic_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_item_net_o_matic_SpellScript();
    }
};

// http://www.wowhead.com/item=8529 Noggenfogger Elixir
// 16589 Noggenfogger Elixir
enum eNoggenfoggerElixirSpells
{
    SPELL_NOGGENFOGGER_ELIXIR_TRIGGERED1 = 16595,
    SPELL_NOGGENFOGGER_ELIXIR_TRIGGERED2 = 16593,
    SPELL_NOGGENFOGGER_ELIXIR_TRIGGERED3 = 16591,
};

class spell_item_noggenfogger_elixir : public SpellScriptLoader
{
public:
    spell_item_noggenfogger_elixir() : SpellScriptLoader("spell_item_noggenfogger_elixir") { }

    class spell_item_noggenfogger_elixir_SpellScript : public SpellScript
    {
    public:
        PrepareSpellScript(spell_item_noggenfogger_elixir_SpellScript)
        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            if (!sSpellStore.LookupEntry(SPELL_NOGGENFOGGER_ELIXIR_TRIGGERED1))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_NOGGENFOGGER_ELIXIR_TRIGGERED2))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_NOGGENFOGGER_ELIXIR_TRIGGERED3))
                return false;
            return true;
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* pCaster = GetCaster();
            if (pCaster->GetTypeId() != TYPEID_PLAYER)
                return;

            uint32 spellId = SPELL_NOGGENFOGGER_ELIXIR_TRIGGERED3;
            switch (urand(1, 3))
            {
                case 1: spellId = SPELL_NOGGENFOGGER_ELIXIR_TRIGGERED1; break;
                case 2: spellId = SPELL_NOGGENFOGGER_ELIXIR_TRIGGERED2; break;
            }

            pCaster->CastSpell(pCaster, spellId, true, NULL);
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_item_noggenfogger_elixir_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_item_noggenfogger_elixir_SpellScript();
    }
};

// http://www.wowhead.com/item=6657 Savory Deviate Delight
// 8213 Savory Deviate Delight
enum eSavoryDeviateDelight
{
    SPELL_FLIP_OUT_MALE     = 8219,
    SPELL_FLIP_OUT_FEMALE   = 8220,
    SPELL_YAAARRRR_MALE     = 8221,
    SPELL_YAAARRRR_FEMALE   = 8222,
};

class spell_item_savory_deviate_delight : public SpellScriptLoader
{
public:
    spell_item_savory_deviate_delight() : SpellScriptLoader("spell_item_savory_deviate_delight") { }

    class spell_item_savory_deviate_delight_SpellScript : public SpellScript
    {
    public:
        PrepareSpellScript(spell_item_savory_deviate_delight_SpellScript)
        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            for (uint32 spellId = SPELL_FLIP_OUT_MALE; spellId <= SPELL_YAAARRRR_FEMALE; ++spellId)
                if (!sSpellStore.LookupEntry(spellId))
                    return false;
            return true;
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* pCaster = GetCaster();
            if (pCaster->GetTypeId() != TYPEID_PLAYER)
                return;

            uint32 spellId = 0;
            switch (urand(1, 2))
            {
                // Flip Out - ninja
                case 1: spellId = (pCaster->getGender() == GENDER_MALE ? SPELL_FLIP_OUT_MALE : SPELL_FLIP_OUT_FEMALE); break;
                // Yaaarrrr - pirate
                case 2: spellId = (pCaster->getGender() == GENDER_MALE ? SPELL_YAAARRRR_MALE : SPELL_YAAARRRR_FEMALE); break;
            }
            pCaster->CastSpell(pCaster, spellId, true, NULL);
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_item_savory_deviate_delight_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_item_savory_deviate_delight_SpellScript();
    }
};

// http://www.wowhead.com/item=7734 Six Demon Bag
// 14537 Six Demon Bag
enum eSixDemonBagSpells
{
    SPELL_FROSTBOLT                 = 11538,
    SPELL_POLYMORPH                 = 14621,
    SPELL_SUMMON_FELHOUND_MINION    = 14642,
    SPELL_FIREBALL                  = 15662,
    SPELL_CHAIN_LIGHTNING           = 21179,
    SPELL_ENVELOPING_WINDS          = 25189,
};

class spell_item_six_demon_bag : public SpellScriptLoader
{
public:
    spell_item_six_demon_bag() : SpellScriptLoader("spell_item_six_demon_bag") { }

    class spell_item_six_demon_bag_SpellScript : public SpellScript
    {
    public:
        PrepareSpellScript(spell_item_six_demon_bag_SpellScript)
        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            if (!sSpellStore.LookupEntry(SPELL_FROSTBOLT))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_POLYMORPH))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_SUMMON_FELHOUND_MINION))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_FIREBALL))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_CHAIN_LIGHTNING))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_ENVELOPING_WINDS))
                return false;
            return true;
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            if (Unit* pTarget = GetHitUnit())
            {
                Unit* pCaster = GetCaster();

                uint32 spellId = 0;
                uint32 rand = urand(0, 99);
                if (rand < 25)                      // Fireball (25% chance)
                    spellId = SPELL_FIREBALL;
                else if (rand < 50)                 // Frostball (25% chance)
                    spellId = SPELL_FROSTBOLT;
                else if (rand < 70)                 // Chain Lighting (20% chance)
                    spellId = SPELL_CHAIN_LIGHTNING;
                else if (rand < 80)                 // Polymorph (10% chance)
                {
                    spellId = SPELL_POLYMORPH;
                    if (urand(0, 100) <= 30)        // 30% chance to self-cast
                        pTarget = pCaster;
                }
                else if (rand < 95)                 // Enveloping Winds (15% chance)
                    spellId = SPELL_ENVELOPING_WINDS;
                else                                // Summon Felhund minion (5% chance)
                {
                    spellId = SPELL_SUMMON_FELHOUND_MINION;
                    pTarget = pCaster;
                }

                pCaster->CastSpell(pTarget, spellId, true, GetCastItem());
            }
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_item_six_demon_bag_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_item_six_demon_bag_SpellScript();
    }
};

// http://www.wowhead.com/item=44012 Underbelly Elixir
// 59640 Underbelly Elixir
enum eUnderbellyElixirSpells
{
    SPELL_UNDERBELLY_ELIXIR_TRIGGERED1 = 59645,
    SPELL_UNDERBELLY_ELIXIR_TRIGGERED2 = 59831,
    SPELL_UNDERBELLY_ELIXIR_TRIGGERED3 = 59843,
};

class spell_item_underbelly_elixir : public SpellScriptLoader
{
public:
    spell_item_underbelly_elixir() : SpellScriptLoader("spell_item_underbelly_elixir") { }

    class spell_item_underbelly_elixir_SpellScript : public SpellScript
    {
    public:
        PrepareSpellScript(spell_item_underbelly_elixir_SpellScript)
        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            if (!sSpellStore.LookupEntry(SPELL_UNDERBELLY_ELIXIR_TRIGGERED1))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_UNDERBELLY_ELIXIR_TRIGGERED2))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_UNDERBELLY_ELIXIR_TRIGGERED3))
                return false;
            return true;
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* pCaster = GetCaster();
            if (pCaster->GetTypeId() != TYPEID_PLAYER)
                return;

            uint32 spellId = SPELL_UNDERBELLY_ELIXIR_TRIGGERED3;
            switch (urand(1, 3))
            {
                case 1: spellId = SPELL_UNDERBELLY_ELIXIR_TRIGGERED1; break;
                case 2: spellId = SPELL_UNDERBELLY_ELIXIR_TRIGGERED2; break;
            }
            pCaster->CastSpell(pCaster, spellId, true, NULL);
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_item_underbelly_elixir_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_item_underbelly_elixir_SpellScript();
    }
};

enum eShadowmourneVisuals
{
    SPELL_SHADOWMOURNE_CHAOS_BANE_DAMAGE = 71904,
    SPELL_SHADOWMOURNE_SOUL_FRAGMENT     = 71905,
    SPELL_SHADOWMOURNE_VISUAL_LOW        = 72521,
    SPELL_SHADOWMOURNE_VISUAL_HIGH       = 72523,
    SPELL_SHADOWMOURNE_CHAOS_BANE_BUFF   = 73422,
};

// 71903 - Item - Shadowmourne Legendary
class spell_item_shadowmourne : public SpellScriptLoader
{
    public:
        spell_item_shadowmourne() : SpellScriptLoader("spell_item_shadowmourne") { }

        class spell_item_shadowmourne_AuraScript : public AuraScript
        {
        public:
            PrepareAuraScript(spell_item_shadowmourne_AuraScript);

            bool Validate(SpellEntry const*)
            {
                if (!sSpellStore.LookupEntry(SPELL_SHADOWMOURNE_CHAOS_BANE_DAMAGE))
                    return false;
                if (!sSpellStore.LookupEntry(SPELL_SHADOWMOURNE_SOUL_FRAGMENT))
                    return false;
                if (!sSpellStore.LookupEntry(SPELL_SHADOWMOURNE_CHAOS_BANE_BUFF))
                    return false;
                return true;
            }

            void HandleDummy(AuraEffect const *, AuraEffectHandleModes)
            {
                GetCaster()->CastSpell(GetCaster(), SPELL_SHADOWMOURNE_SOUL_FRAGMENT, true, NULL);

                if (Aura* soulFragments = GetCaster()->GetAura(SPELL_SHADOWMOURNE_SOUL_FRAGMENT))
                {
                    if (soulFragments->GetStackAmount() >= 10)
                    {
                        GetCaster()->CastSpell(GetCaster(), SPELL_SHADOWMOURNE_CHAOS_BANE_DAMAGE, true, NULL);
                        soulFragments->Remove();
                    }
                }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_item_shadowmourne_AuraScript::HandleDummy, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuracript() const
        {
            return new spell_item_shadowmourne_AuraScript();
        }
};

// 71905 - Soul Fragment
class spell_item_shadowmourne_soul_fragment : public SpellScriptLoader
{
    public:
        spell_item_shadowmourne_soul_fragment() : SpellScriptLoader("spell_item_shadowmourne_soul_fragment") { }

        class spell_item_shadowmourne_soul_fragment_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_item_shadowmourne_soul_fragment_AuraScript);

            bool Validate(SpellEntry const*)
            {
                if (!sSpellStore.LookupEntry(SPELL_SHADOWMOURNE_VISUAL_LOW) || !sSpellStore.LookupEntry(SPELL_SHADOWMOURNE_VISUAL_HIGH) || !sSpellStore.LookupEntry(SPELL_SHADOWMOURNE_CHAOS_BANE_BUFF))
                    return false;
                return true;
            }

            void OnStackChange(AuraEffect const*, AuraEffectHandleModes)
            {
                Unit* target = GetTarget();

                switch (GetStackAmount())
                {
                    case 1:
                        target->CastSpell(target, SPELL_SHADOWMOURNE_VISUAL_LOW, true);
                        break;
                    case 6:
                        target->RemoveAurasDueToSpell(SPELL_SHADOWMOURNE_VISUAL_LOW);
                        target->CastSpell(target, SPELL_SHADOWMOURNE_VISUAL_HIGH, true);
                        break;
                    case 10:
                        target->RemoveAurasDueToSpell(SPELL_SHADOWMOURNE_VISUAL_HIGH);
                        target->CastSpell(target, SPELL_SHADOWMOURNE_CHAOS_BANE_BUFF, true);
                        break;
                    default:
                        break;
                }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_item_shadowmourne_soul_fragment_AuraScript::OnStackChange, EFFECT_0, SPELL_AURA_MOD_STAT, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_item_shadowmourne_soul_fragment_AuraScript();
        }
};

class spell_item_apparatus_of_khazgoroth : public SpellScriptLoader
{
public:
    spell_item_apparatus_of_khazgoroth() : SpellScriptLoader("spell_item_apparatus_of_khazgoroth") { }

    class spell_item_apparatus_of_khazgoroth_SpellScript : public SpellScript
    {
    public:
        PrepareSpellScript(spell_item_apparatus_of_khazgoroth_SpellScript)
        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* pCaster = GetCaster();
            if (pCaster->GetTypeId() != TYPEID_PLAYER)
                return;
            uint32 spellId=0;
            float crit=pCaster->ToPlayer()->GetRatingBonusValue(CR_CRIT_MELEE);
            float haste=pCaster->ToPlayer()->GetRatingBonusValue(CR_HASTE_MELEE);
            float mastery=pCaster->ToPlayer()->GetRatingBonusValue(CR_MASTERY);
            uint32 stacks=pCaster->GetAuraCount(96923);
            if(crit>=haste&&crit>=mastery)
            {
                spellId = 96928;
            }
            if(haste>=crit&&haste>=mastery)
            {
                spellId = 96927;
            }
            if(mastery>=crit&&mastery>=haste)
            {
                spellId = 96929;
            }

            pCaster->CastSpell(pCaster, spellId, true, NULL);
            pCaster->RemoveAurasDueToSpell(96923,pCaster->GetGUID(),true);
            pCaster->SetAuraStack(spellId,pCaster,stacks);
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_item_apparatus_of_khazgoroth_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_item_apparatus_of_khazgoroth_SpellScript();
    }
};

class spell_item_apparatus_of_khazgoroth_hc : public SpellScriptLoader
{
public:
    spell_item_apparatus_of_khazgoroth_hc() : SpellScriptLoader("spell_item_apparatus_of_khazgoroth_hc") { }

    class spell_item_apparatus_of_khazgoroth_hc_SpellScript : public SpellScript
    {
    public:
        PrepareSpellScript(spell_item_apparatus_of_khazgoroth_hc_SpellScript)
            void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* pCaster = GetCaster();
            if (pCaster->GetTypeId() != TYPEID_PLAYER)
                return;
            uint32 spellId=0;
            float crit=pCaster->ToPlayer()->GetRatingBonusValue(CR_CRIT_MELEE);
            float haste=pCaster->ToPlayer()->GetRatingBonusValue(CR_HASTE_MELEE);
            float mastery=pCaster->ToPlayer()->GetRatingBonusValue(CR_MASTERY);
            uint32 stacks=pCaster->GetAuraCount(96923);
            if(crit>=haste&&crit>=mastery)
            {
                spellId = 96928;
            }
            if(haste>=crit&&haste>=mastery)
            {
                spellId = 96927;
            }
            if(mastery>=crit&&mastery>=haste)
            {
                spellId = 96929;
            }

            pCaster->CastCustomSpell(spellId,SPELLVALUE_BASE_POINT0,575,pCaster,true,NULL);
            pCaster->RemoveAurasDueToSpell(96923,pCaster->GetGUID(),true);
            pCaster->SetAuraStack(spellId,pCaster,stacks);
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_item_apparatus_of_khazgoroth_hc_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_item_apparatus_of_khazgoroth_hc_SpellScript();
    }
};

enum eGenericData
{
    SPELL_ARCANITE_DRAGONLING           = 19804,
    SPELL_BATTLE_CHICKEN                = 13166,
    SPELL_MECHANICAL_DRAGONLING         = 4073,
    SPELL_MITHRIL_MECHANICAL_DRAGONLING = 12749,
};

void AddSC_item_spell_scripts()
{
    // 23074 Arcanite Dragonling
    new spell_item_trigger_spell("spell_item_arcanite_dragonling", SPELL_ARCANITE_DRAGONLING);
    // 23133 Gnomish Battle Chicken
    new spell_item_trigger_spell("spell_item_gnomish_battle_chicken", SPELL_BATTLE_CHICKEN);
    // 23076 Mechanical Dragonling
    new spell_item_trigger_spell("spell_item_mechanical_dragonling", SPELL_MECHANICAL_DRAGONLING);
    // 23075 Mithril Mechanical Dragonling
    new spell_item_trigger_spell("spell_item_mithril_mechanical_dragonling", SPELL_MITHRIL_MECHANICAL_DRAGONLING);

    new spell_item_deviate_fish();
    new spell_item_flask_of_the_north();
    new spell_item_flask_of_enhancement();
    new spell_item_gnomish_death_ray();
    new spell_item_make_a_wish();
    new spell_item_mingos_fortune_generator();
    new spell_item_net_o_matic();
    new spell_item_noggenfogger_elixir();
    new spell_item_savory_deviate_delight();
    new spell_item_six_demon_bag();
    new spell_item_underbelly_elixir();
    new spell_item_shadowmourne();
    new spell_item_shadowmourne_soul_fragment();
    new spell_item_apparatus_of_khazgoroth();
    new spell_item_apparatus_of_khazgoroth_hc();
    new go_food_feast_cataclysm();
}