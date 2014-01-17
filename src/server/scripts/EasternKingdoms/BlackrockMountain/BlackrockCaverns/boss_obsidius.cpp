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

enum UsedThings
{
    SPELL_TRANSFORMATION_STUN      = 76274,
    SPELL_TRANSFORMATION_SIZE      = 76200,
    SPELL_STONE_BLOW               = 76185,
    SPELL_TWILIGHT_CORRUPTION      = 76188,
    SPELL_TWILIGHT_CORRUPTION_H    = 93613,
    SPELL_THUNDER_CLAP             = 76186,

    NPC_BOSS_OBSIDIUS              = 39705,
    NPC_SHADOW_OF_OBSIDIUS         = 40817,
    SPELL_TWITCHY                  = 76167,
    SPELL_CREPUSCULAR_VEIL         = 76189,
    SPELL_CREPUSCULAR_VEIL_TRIGGER = 76190,
};

/*
UPDATE creature_template SET ScriptName='boss_obsidius',AIName='' WHERE entry=39705;
UPDATE creature_template SET ScriptName='mob_shadow_of_obsidius',AIName='' WHERE entry=40817;
*/

class boss_obsidius: public CreatureScript
{
    public:
        boss_obsidius(): CreatureScript("boss_obsidius") {};

        struct boss_obsidiusAI: public ScriptedAI
        {
            boss_obsidiusAI(Creature* c): ScriptedAI(c)
            {
                ModifySpellRadius(SPELL_THUNDER_CLAP, 13, -1);
                Reset();
                pInstance = me->GetInstanceScript();
            }

            InstanceScript* pInstance;
            uint32 StoneBlowTimer;
            uint32 TwilightCorruptionTimer;
            uint32 ThunderClapTimer;
            uint8 TransformPhase;
            uint8 TransformTimedEvent;
            uint32 TransformTimer;

            std::list<Creature*> ShadowsList;
            Creature* pTransformTarget;

            void Reset()
            {
                StoneBlowTimer = urand(7000,9000);
                ThunderClapTimer = urand(8000,11000);
                TwilightCorruptionTimer = urand(5000,15000);
                TransformPhase = 0;
                TransformTimedEvent = 0;
                TransformTimer = 0;
                pTransformTarget = NULL;

                GetCreatureListWithEntryInGrid(ShadowsList, me, NPC_SHADOW_OF_OBSIDIUS, 100.0f);
            }

            void EnterCombat(Unit* pWho)
            {
                me->MonsterYell("You come seeking answers? Then have them! Look upon your answer to living!",LANG_UNIVERSAL,0);
            }

            void KilledUnit(Unit* pWho)
            {
                me->MonsterYell("Your kind has no place in the master's world!",LANG_UNIVERSAL,0);
            }

            void JustDied(Unit* pKiller)
            {
                me->MonsterYell("I cannot be destroyed... Only de... layed...",LANG_UNIVERSAL,0);
                // Zabit vsechny addy
                if (!ShadowsList.empty())
                {
                    for (std::list<Creature*>::const_iterator itr = ShadowsList.begin(); itr != ShadowsList.end(); ++itr)
                        (*itr)->Kill((*itr));
                }
                pInstance->SetData(TYPE_OBSIDIUS, DONE);
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->GetHealthPct() < (2-TransformPhase)*33.0f)
                {
                    TransformPhase++;
                    if (!ShadowsList.empty())
                    {
                        TransformTimedEvent = 1;
                        //TODO: emote
                        TransformTimer = 1000;
                    }
                }

                if (TransformTimedEvent > 0)
                {
                    if (TransformTimer <= diff)
                    {
                        switch (TransformTimedEvent)
                        {
                            case 1:
                                {
                                    me->MonsterYell("Earth can be shaped, molded... You cannot! You are useless!",LANG_UNIVERSAL,0);
                                    uint32 pos = urand(0,2);
                                    std::list<Creature*>::iterator itr = ShadowsList.begin();
                                    for(uint32 i = 0; i < pos; i++)
                                        ++itr;
                                    if (*itr)
                                    {
                                        me->CastSpell(*itr,SPELL_TRANSFORMATION_STUN,false);
                                        pTransformTarget = *itr;
                                    }
                                    TransformTimer = 2000;
                                    ++TransformTimedEvent;
                                    DoStopAttack();
                                    break;
                                }
                            case 2:
                                {
                                    me->CastSpell(pTransformTarget,SPELL_TRANSFORMATION_SIZE,true);
                                    TransformTimer = 2600;
                                    ++TransformTimedEvent;
                                    break;
                                }
                            case 3:
                                {
                                    Position my_pos, his_pos;
                                    me->GetPosition(&my_pos);
                                    pTransformTarget->GetPosition(&his_pos);
                                    me->NearTeleportTo(his_pos.GetPositionX(), his_pos.GetPositionY(), his_pos.GetPositionZ(), 0.0f);
                                    pTransformTarget->NearTeleportTo(my_pos.GetPositionX(), my_pos.GetPositionY(), my_pos.GetPositionZ(), 0.0f);
                                    TransformTimer = 1000;
                                    ++TransformTimedEvent;
                                    DoResetThreat();
                                    break;
                                }
                            case 4:
                                {
                                    // hm ?
                                    //Jen jako "timeout" pred dalsimi akcemi
                                    TransformTimer = 0;
                                    TransformTimedEvent = 0;
                                    break;
                                }
                        }
                    } else TransformTimer -= diff;
                }
                else
                {
                    if (StoneBlowTimer <= diff)
                    {
                        me->CastSpell(me->getVictim(),SPELL_STONE_BLOW,false);
                        StoneBlowTimer = urand(7000,9000);
                    } else StoneBlowTimer -= diff;

                    if (ThunderClapTimer <= diff)
                    {
                        me->CastSpell(me, SPELL_THUNDER_CLAP, false);
                        ThunderClapTimer = urand(8000,11000);
                    } else ThunderClapTimer -= diff;

                    if (TwilightCorruptionTimer <= diff)
                    {
                        Player* pTarget = SelectRandomPlayer(80.0f);
                        if (pTarget)
                            me->CastSpell(pTarget, DUNGEON_MODE(SPELL_TWILIGHT_CORRUPTION,SPELL_TWILIGHT_CORRUPTION_H), false);
                        TwilightCorruptionTimer = urand(5000,15000);
                    } else TwilightCorruptionTimer -= diff;
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new boss_obsidiusAI(c);
        }
};

class mob_shadow_of_obsidius: public CreatureScript
{
    public:
        mob_shadow_of_obsidius(): CreatureScript("mob_shadow_of_obsidius") {};

        struct mob_shadow_of_obsidiusAI: public ScriptedAI
        {
            mob_shadow_of_obsidiusAI(Creature* c): ScriptedAI(c)
            {
                Reset();
            }

            void Reset()
            {
            }

            void EnterCombat(Unit* pWho)
            {
                me->CastSpell(me, SPELL_CREPUSCULAR_VEIL_TRIGGER, true);
                me->CastSpell(me, SPELL_TWITCHY, true);
            }

            void DamageTaken(Unit* pAttacker, uint32& damage)
            {
                damage = 0;
                DoResetThreat();
                me->AddThreat(pAttacker,100.0f);
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->GetHealth() < me->GetMaxHealth())
                    me->SetHealth(me->GetMaxHealth());

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new mob_shadow_of_obsidiusAI(c);
        }
};

void AddSC_obsidius()
{
    new boss_obsidius();
    new mob_shadow_of_obsidius();
}
