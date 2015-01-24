/*
 * Copyright (C) 2006-2013 iCe Online <http://ice-wow.eu>
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

#include "ScriptPCH.h"
#include "well_of_eternity.h"

#define CAST_WOE_INSTANCE(i)     (dynamic_cast<instance_well_of_eternity::instance_well_of_eternity_InstanceMapScript*>(i))

/*static void PlayQuote (Creature * source, SimpleQuote quote, bool yell = false)
{
    source->PlayDirectSound(quote.soundID);

    if (yell)
        source->MonsterYell(quote.text, LANG_UNIVERSAL,0,200.0f);
    else
        source->MonsterSay(quote.text, LANG_UNIVERSAL,0,200.0f);
}*/

enum entries
{
    ENTRY_EMBEDDED_BLADE = 55838, // npc should equip blade ?
    ENTRY_FELHOUND = 56001,
    ENTRY_FELGUARD = 56002,
    ENTRY_DOOMGUARD_DEVASTATOR = 57410,
    ENTRY_INFERNAL = 1 // ???
};

enum spells
{
    SPELL_PORTAL_TO_TWISTING_NETHER = 102920, // freaking long cast tine -> just for anim :)
    SPELL_NETHER_TEAR = 105041, // TARGET_UNIT_NEARBY_ENTRY -> dummy aura missile

    SPELL_FIRESTORM = 103888, // summon 55502 triggers ...
    SPELL_FELBLADE  = 103966,
    SPELL_FEL_DRAIN = 104961, // imnstakill to TARGET_UNIT_NEARBY_ENTRY + self heal to 100%
    SPELL_DEBILITATING_FLAY = 104678, // dummy aura TARGET_UNIT_NEARBY_ENTRY

    SPELL_MAGISTRIKE_CHAIN_DAMAGE = 103669, // varothens

    SPELL_EMBEDDED_BLADE_VISUAL = 104823, // in mannoroths chest
    SPELL_EMBEDDED_BLADE_AURA = 104820, // with scream anim
    SPELL_EMBEDDED_BLADE_AURA2 = 109542, // same spell without anim kit
    SPELL_FEL_FIRE_NOVA_AOE = 105093,

    SPELL_NETHER_PORTAL_CAST = 104625, // apply aura, aoe, trigger missile
    SPELL_NETHER_PORTAL_AURA = 104648,

    SPELL_HAND_OF_ELUNE = 105072, // infinite channel like spell, periodically trigerring 105073 as AoE (3 max affected targets ...)
    SPELL_HAND_OF_ELUNE_DUMMY_AURA_BLESSING = 109546, // moon above head
    SPELL_MAGISTRIKE_ARC = 105524 , // trigering 105524=2 + 105523

    SPELL_GIFT_OF_SARGERAS = 104998, // 30 s cast ?
    SPELL_GIFT_OF_SARGERAS_INSTANT = 105009, // but cannot cast ?

    SPELL_ELUNES_WRATH = 103919, // moonfire like spell
};

class boss_mannoroth_woe : public CreatureScript
{
public:
    boss_mannoroth_woe() : CreatureScript("boss_mannoroth_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_mannoroth_woeAI(creature);
    }

    struct boss_mannoroth_woeAI : public ScriptedAI
    {
        boss_mannoroth_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = me->GetInstanceScript();
            me->CastSpell(me, SPELL_PORTAL_TO_TWISTING_NETHER, false);
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
        }

        InstanceScript * pInstance;

        void Reset() override
        {
            ScriptedAI::Reset();
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

class boss_captain_varothen_woe : public CreatureScript
{
public:
    boss_captain_varothen_woe() : CreatureScript("boss_captain_varothen_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_captain_varothen_woeAI(creature);
    }

    struct boss_captain_varothen_woeAI : public ScriptedAI
    {
        boss_captain_varothen_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = me->GetInstanceScript();
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
        }

        InstanceScript * pInstance;

        void Reset() override
        {
            ScriptedAI::Reset();
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_mannoroth()
{
    new boss_mannoroth_woe();
    new boss_captain_varothen_woe();
}