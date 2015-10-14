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

/*
Encounter: Ultraxion
Dungeon: Dragon Soul
Difficulty: Normal / Heroic
Mode: 10-man / 25-man
Autor: Lazik
*/

/*

26317 - I am the beginning of the end...the shadow which blots out the sun...the bell which tolls your doom... INTRO 1
26318 - For this moment ALONE was I made. Look upon your death, mortals, and despair! INTRO_02


26323 - The final shred of light fades, and with it, your pitiful mortal existence!
---     26324,10,"VO_DS_ULTRAXION_SPELL_02
26325 - Through the pain and fire my hatred burns! SPELL_03
*/

#include "ScriptPCH.h"
#include "dragonsoul.h"
#include "Spell.h"
#include "UnitAI.h"
#include "MapManager.h"
#include <stdlib.h>

// NPCs
enum NPC
{
    BOSS_ULTRAXION                  = 55294,
    NPC_ULTRAXION_GAUNTLET          = 56305,
};

enum Gameobjescts
{
    GO_GIFT_OF_LIFE                 = 209873,
    GO_ESSENCE_OF_DREAMS            = 209874,
    GO_SOURCE_OF_MAGIC              = 209875,
};

// Spells
enum Spells
{
    // REUSE                        = 109192,
    ULTRAXION_NORM_REALM_COSMETIC   = 105929,
    ULTRAXION_COSMETIC              = 105211,
    ULTRAXION_COSMETIC_1            = 105214,

    ULTRAXION_ACHIEVEMENT_AURA      = 109188,
    ULTRAXION_AHCIEVEMENT_FAILED    = 109194,

    UNSTABLE_MONSTROSITY_6S         = 106372, // 6s trigger aura - 109176 / 106375
    UNSTABLE_MONSTROSITY_5S         = 106376, // 5s trigger aura
    UNSTABLE_MONSTROSITY_4S         = 106377, // 4s trigger aura
    UNSTABLE_MONSTROSITY_3S         = 106378, // 3s trigger aura
    UNSTABLE_MONSTROSITY_2S         = 106379, // 2s trigger aura
    UNSTABLE_MONSTROSITY_1S         = 106380, // 1s trigger aura
    UNSTABLE_MONSTROSITY_DUNNO      = 106390, // Prevents from casting these auras again for 4s
    UNSTABLE_MONSTROSITY_10N_EX     = 106381, // ? 8/6s normal/heroic
    UNSTABLE_MONSTROSITY_25N_EX     = 109418,
    UNSTABLE_MONSTROSITY_10HC_EX    = 109419,
    UNSTABLE_MONSTROSITY_25HC_EX    = 109420,

    TWILIGHT_INSTABILITY_DUMMY      = 106374, // Dummy effect
    TWILIGHT_INSTABILITY            = 109176, // Spell effect script effect?
    TWILIGHT_INSTABILITY_10N        = 106375, // 300 000 dmg 10N
    TWILIGHT_INSTABILITY_25N        = 109182, // 825 000 dmg 25N
    TWILIGHT_INSTABILITY_10HC       = 109183, // 400 000 dmg 10HC
    TWILIGHT_INSTABILITY_25HC       = 109184, // 1 100 000 dmg 25HC

    TWILIGHT_SHIFT_AOE              = 106368, // Twilight Phase - cosmetic visual
    TWILIGHT_SHIFT                  = 106369, // Force cast 106368

    HEROIC_WILL_AOE                 = 105554,
    HEROIC_WILL                     = 106108,
    HEROIC_WILL_2                   = 106175,

    FADING_LIGHT                    = 105925, // from boss to player, triggered by hour of twilight, tank only
    FADING_LIGHT_KILL               = 105926, // kill player
    FADING_LIGHT_2                  = 109075, // from boss, triggered by 105925, dps
    FADING_LIGHT_3                  = 109200, // dummy
    FADING_LIGHT_4                  = 110068,
    FADING_LIGHT_5                  = 110069,
    FADING_LIGHT_6                  = 110070,
    FADING_LIGHT_7                  = 110073,
    FADING_LIGHT_8                  = 110074,
    FADING_LIGHT_9                  = 110075,
    FADING_LIGHT_10                 = 110078,
    FADING_LIGHT_11                 = 110079,
    FADING_LIGHT_12                 = 110080,

    FADED_INTO_TWILIGHT             = 105927,
    FADED_INTO_TWILIGHT_1           = 109461,
    FADED_INTO_TWILIGHT_2           = 109462,
    FADED_INTO_TWILIGHT_3           = 109463,

    HOUR_OF_TWILIGHT_DMG            = 103327, // dmg + forse cast 109231, force cast 106370
    HOUR_OF_TWILIGHT_REMOVE_WILL    = 106174, // remove heroic will
    HOUR_OF_TWILIGHT_ACHIEVEMENT    = 106370, // from player, force cast achievement
    HOUR_OF_TWILIGHT                = 106371,
    HOUR_OF_TWILIGHT_4              = 106389,
    HOUR_OF_TWILIGHT_5              = 109172,
    HOUR_OF_TWILIGHT_6              = 109173,
    HOUR_OF_TWILIGHT_7              = 109174,
    HOUR_OF_TWILIGHT_8              = 109323,
    HOUR_OF_TWILIGHT_9              = 109415,
    HOUR_OF_TWILIGHT_10             = 109416,
    HOUR_OF_TWILIGHT_11             = 109417,

    TWILIGHT_BURST                  = 106415,

    TWILIGHT_ERUPTION               = 106388, // Berserk - kill all players

    LOOMING_DARKNESS_DUMMY          = 106498,
    LOOMING_DARKNESS_DMG            = 109231,

    // Alexstrasza
    GIFT_OF_LIFE_AURA               = 105896,
    GIFT_OF_LIFE_1                  = 106042,
    GIFT_OF_LIFE_SUMMON_1           = 106044,
    GIFT_OF_LIFE_SUMMON_2           = 109340,
    GIFT_OF_LIFE_4                  = 109345, // triggered by 106042 in 25 ppl
    GIFT_OF_LIFE_5                  = 109349,
    GIFT_OF_LIFE_6                  = 109350,
    GIFT_OF_LIFE_7                  = 109351,

    // Ysera
    ESSENCE_OF_DREAMS_AURA          = 105900,
    ESSENCE_OF_DREAMS_HEAL          = 105996,
    ESSENCE_OF_DREAMS_SUMMON_1      = 106047,
    ESSENCE_OF_DREAMS_3             = 106049,
    ESSENCE_OF_DREAMS_SUMMON_2      = 109342,
    ESSENCE_OF_DREAMS_5             = 109344, // triggered by 106049 in 25 ppl
    ESSENCE_OF_DREAMS_6             = 109356,
    ESSENCE_OF_DREAMS_7             = 109357,
    ESSENCE_OF_DREAMS_8             = 109358,

    // Kalecgos
    SOURCE_OF_MAGIC_AURA            = 105903,
    SOURCE_OF_MAGIC_SUMMON_1        = 106048,
    SOURCE_OF_MAGIC_2               = 106050,
    SOURCE_OF_MAGIC_SUMMON_2        = 109346,
    SOURCE_OF_MAGIC_4               = 109347, // triggered by 106050 in 25 ppl
    SOURCE_OF_MAGIC_5               = 109353,
    SOURCE_OF_MAGIC_6               = 109354,
    SOURCE_OF_MAGIC_7               = 109355,

    // Nozdormu
    TIMELOOP                        = 105984,
    TIMELOOP_HEAL                   = 105992,

    // Thrall
    LAST_DEFENDER_OF_AZEROTH        = 106182, // scale + force cast 110327
    LAST_DEFENDER_OF_AZEROTH_SCRIPT = 106218,
    LAST_DEFENDER_OF_AZEROTH_DUMMY  = 110327,
    LAST_DEFENDER_OF_AZEROTH_WARR   = 106080,
    LAST_DEFENDER_OF_AZEROTH_DRUID  = 106224,
    LAST_DEFENDER_OF_AZEROTH_PALA   = 106226,
    LAST_DEFENDER_OF_AZEROTH_DK     = 106227,
};

// Ultraxion
class boss_ultraxion : public CreatureScript
{
public:
    boss_ultraxion() : CreatureScript("boss_ultraxion") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_ultraxionAI(pCreature);
    }

    struct boss_ultraxionAI : public ScriptedAI
    {
        boss_ultraxionAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 unstableMonstrosityTimer;
        uint32 unstableMonstrosityCount;
        uint32 phase;

        void Reset() override
        {
            if (instance)
            {
                if (instance->GetData(TYPE_BOSS_ULTRAXION) != DONE)
                    instance->SetData(TYPE_BOSS_ULTRAXION, NOT_STARTED);
            }

            unstableMonstrosityTimer = 60000;
            unstableMonstrosityCount = 0;

            me->SetFlying(true);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
        }

        void EnterCombat(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_ULTRAXION, IN_PROGRESS);
            }

            me->MonsterYell("Now is the hour of twilight!", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
            me->SendPlaySound(26314, true);

            me->CastSpell(me, UNSTABLE_MONSTROSITY_6S, false);

            // Take all players to twilight phase
            Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
            {
                if (Player* player = i->getSource())
                {
                    if (!player->IsGameMaster())
                    {
                        player->CastSpell(player, TWILIGHT_SHIFT, true);
                    }
                }
            }
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim && victim->GetTypeId() == TYPEID_PLAYER)
            {
                switch (urand(0, 2))
                {
                case 0:
                    me->MonsterYell("Fall before Ultraxion!", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(26319, true);
                    break;
                case 1:
                    me->MonsterYell("You have no hope!", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(26320, true);
                    break;
                case 2:
                    me->MonsterYell("Hahahahaha!", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(26321, true);
                    break;
                default:
                    break;
                }
            }
        }

        void JustDied(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_ULTRAXION, DONE);
            }

            me->MonsterSay("But...but...I am...Ul...trax...ionnnnnn...", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
            me->SendPlaySound(26316, true);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            // Unstable Monstrosity ability
            if (unstableMonstrosityTimer <= diff)
            {
                unstableMonstrosityCount++;
                unstableMonstrosityTimer = 60000;

                switch (unstableMonstrosityCount)
                {
                case 1: // 6s -> 5s
                    me->RemoveAura(UNSTABLE_MONSTROSITY_6S);
                    me->CastSpell(me, UNSTABLE_MONSTROSITY_5S, true);
                    break;
                case 2: // 5s -> 4s
                    me->RemoveAura(UNSTABLE_MONSTROSITY_5S);
                    me->CastSpell(me, UNSTABLE_MONSTROSITY_4S, true);
                    break;
                case 3: // 4s -> 3s
                    me->RemoveAura(UNSTABLE_MONSTROSITY_4S);
                    me->CastSpell(me, UNSTABLE_MONSTROSITY_3S, true);
                    break;
                case 4: // 3s -> 2s
                    me->RemoveAura(UNSTABLE_MONSTROSITY_3S);
                    me->CastSpell(me, UNSTABLE_MONSTROSITY_2S, true);
                    break;
                case 5: // 2s -> 1s
                    me->RemoveAura(UNSTABLE_MONSTROSITY_2S);
                    me->CastSpell(me, UNSTABLE_MONSTROSITY_1S, true);
                    break;
                case 6: // 1s -> Twilight Eruption
                    me->CastSpell(me, TWILIGHT_ERUPTION, false);
                    me->MonsterYell("I WILL DRAG YOU WITH ME INTO FLAME AND DARKNESS!", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
                    me->SendPlaySound(26315, true);
                default:
                    break;
                }
            }
            else unstableMonstrosityTimer -= diff;

            // Melee attack
            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_ultraxion()
{
    // Boss
    new boss_ultraxion();
}