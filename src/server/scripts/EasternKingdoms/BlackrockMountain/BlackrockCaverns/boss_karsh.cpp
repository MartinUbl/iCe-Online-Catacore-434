/*
 * Copyright (C) 2008-2010 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
#include "blackrock_caverns.h"

/*
UPDATE creature_template SET ScriptName='boss_karsh', AIName='',equipment_id=39698 WHERE entry=39698;
UPDATE creature_template SET ScriptName='mob_lava_pool', AIName='' WHERE entry=50423;
UPDATE creature_template SET ScriptName='mob_bound_flames', AIName='' WHERE entry=50417;

DELETE FROM `creature_equip_template` WHERE (`entry`=39698);
INSERT INTO `creature_equip_template` (`entry`, `equipentry1`, `equipentry2`, `equipentry3`) VALUES (39698, 55272, 55272, 0);
*/

//prostredni sloup tekouci lavy
#define CENTER_X 237.6216f
#define CENTER_Y 785.1566f
#define CENTER_Z  95.6717f
#define CENTER_DIST 4.243f
#define RING_DIST 29.5593f

enum
{
    SPELL_CLEAVE                      = 845,
    SPELL_QUICKSILVER_ARMOR           = 75842,
    SPELL_HEATED_ARMOR                = 75846,
    SPELL_HEATED_ARMOR_H              = 93567,
    SPELL_HEATED_ARMOR_DAMAGE_TRIGGER = 76015,
    SPELL_HEAT_WAVE                   = 75851,
    SPELL_LAVA_SPOUT                  = 76007,
    SPELL_LAVA_SPOUT_H                = 93565,
    SPELL_SUMMON_BOUND_FLAMES         = 93539,

    SPELL_LAVA_POOL_VISUAL = 90391,
    SPELL_LAVA_POOL_SUMMON = 93547,

    NPC_BOUND_FLAMES = 50417,
    NPC_LAVA_POOL    = 50423,
};

class boss_karsh: public CreatureScript
{
    public:
        boss_karsh(): CreatureScript("boss_karsh") {};

        struct boss_karshAI: public ScriptedAI
        {
            boss_karshAI(Creature* c): ScriptedAI(c)
            {
                Reset();
            }

            void Reset()
            {
                HeatedArmorApplyTimer = 1000;
                HeatWaveTimer = 1000;
                CleaveTimer = 5000;
                Zaznamenal = true;
            }

            uint32 HeatedArmorApplyTimer;
            uint32 HeatWaveTimer;
            uint32 CleaveTimer;
            bool Zaznamenal;

            void EnterCombat(Unit* pWho)
            {
                me->CastSpell(me, SPELL_QUICKSILVER_ARMOR, false);
            }

            bool IsInRangeOfPillar()
            {
                // Zkontrolovat hlavni sloup lavy
                if (me->IsWithinDist2d(CENTER_X,CENTER_Y,CENTER_DIST))
                    return true;

                // A pripadne lava spawny, ktere take slouzi jako sloup lavy
                Creature* pCokoliv = GetClosestCreatureWithEntry(me, NPC_LAVA_POOL, 3.8f);
                if (pCokoliv)
                    return true;
                else
                    return false;
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                // Jen pokud je v dosahu sloupu s lavou nebo Lava Poolu
                if (IsInRangeOfPillar())
                {
                    if (HeatedArmorApplyTimer <= diff)
                    {
                        // Nahodit stack
                        me->CastSpell(me, DUNGEON_MODE(75846,93567), true);
                        // A spell ktery pri kazdem melee utoku spusti damage spell
                        me->CastSpell(me, SPELL_HEATED_ARMOR_DAMAGE_TRIGGER, true);
                        Zaznamenal = false;
                        HeatedArmorApplyTimer = 2000;
                    } else HeatedArmorApplyTimer -= diff;

                    // A jeste Heat Wave
                    if (HeatWaveTimer <= diff)
                    {
                        me->CastSpell(me, SPELL_HEAT_WAVE, true);
                        HeatWaveTimer = 1000;
                    } else HeatWaveTimer -= diff;
                }

                // Pokud prave spadnul buff Heated Armor
                if (!me->HasAura(DUNGEON_MODE(75846,93567)) && !Zaznamenal)
                {
                    // Dokolecka naspawnit ohynek
                    int total = 16;
                    float x,y,z;
                    for(int i = 0; i < total; i++)
                    {
                        x = CENTER_X + RING_DIST*cos(i*2*M_PI/total);
                        y = CENTER_Y + RING_DIST*sin(i*2*M_PI/total);
                        z = me->GetPositionZ();
                        if (Creature* pTrigger = me->SummonCreature(90007,x,y,z,0,TEMPSUMMON_TIMED_DESPAWN,8000))
                            pTrigger->CastSpell(pTrigger,DUNGEON_MODE(76007,93565),true);
                    }

                    // Naspawnit tri addy
                    total = 3;
                    for(int i = 0; i < total; i++)
                        me->CastSpell(me, SPELL_SUMMON_BOUND_FLAMES, true);

                    Zaznamenal = true;
                }

                // Cleave, nic specialniho
                if (CleaveTimer <= diff)
                {
                    me->CastSpell(me->getVictim(), SPELL_CLEAVE, false);
                    CleaveTimer = urand(30,60)*100;
                } else CleaveTimer -= diff;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new boss_karshAI(c);
        }
};

class mob_bound_flames: public CreatureScript
{
    public:
        mob_bound_flames(): CreatureScript("mob_bound_flames") {};

        struct mob_bound_flamesAI: public ScriptedAI
        {
            mob_bound_flamesAI(Creature* c): ScriptedAI(c)
            {
            }

            // Pri smrti naspawnit lava pool
            void JustDied(Unit* pKiller)
            {
                me->CastSpell(me, SPELL_LAVA_POOL_SUMMON, true);
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new mob_bound_flamesAI(c);
        }
};

class mob_lava_pool: public CreatureScript
{
    public:
        mob_lava_pool(): CreatureScript("mob_lava_pool") {};

        struct mob_lava_poolAI: public Scripted_NoMovementAI
        {
            mob_lava_poolAI(Creature* c): Scripted_NoMovementAI(c)
            {
                Reset();
            }

            // Pri spawnu nahodit visual spell
            void Reset()
            {
                me->CastSpell(me, SPELL_LAVA_POOL_VISUAL, true);
            }

            // Zamezit utoku
            void AttackStart(Unit* pVictim)
            {
                return;
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new mob_lava_poolAI(c);
        }
};

void AddSC_karsh()
{
    new boss_karsh();
    new mob_lava_pool();
    new mob_bound_flames();
}
