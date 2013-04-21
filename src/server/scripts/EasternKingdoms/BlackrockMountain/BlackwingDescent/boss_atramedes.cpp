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
#include "blackwing_descent.h"

enum NeededThings
{
    GO_ANCIENT_BELL = 459704,

    SPELL_SOUND_BAR_SHOW_PERIODIC = 89683,
    SPELL_SOUND_BAR_SHOW = 88824,
    SPELL_SOUND_INCREASE = 88827,
    SPELL_SOUND_DECREASE = 88834,

    SPELL_SONAR_PULSE_PERIODIC = 92416,
    SPELL_FLAME_BREATH_DUMMY = 74450,
};

static const Position BossPath[] = {
    {292.962677f, -224.477127f, 99.140099f, 0.0f},
    {259.621887f, -223.859009f, 95.056923f, 0.0f},
    {212.340851f, -225.293335f, 77.759232f, 0.0f}
};

class boss_atramedes: public CreatureScript
{
    public:
        boss_atramedes(): CreatureScript("boss_atramedes")
        {
        }

        struct boss_atramedesAI: public ScriptedAI
        {
            boss_atramedesAI(Creature* c): ScriptedAI(c)
            {
                pInstance = me->GetInstanceScript();
                Reset();
            }

            InstanceScript* pInstance;

            uint32 IntroTimer;
            uint8 IntroPhase;

            void Reset()
            {
                me->RemoveAurasDueToSpell(SPELL_SOUND_BAR_SHOW_PERIODIC);
                InstanceWideUnaura(SPELL_SOUND_BAR_SHOW);

                IntroTimer = 0;
                IntroPhase = 0;
            }

            void EnterCombat(Unit* pWho)
            {
            }

            void MovementInform(uint32 type, uint32 id)
            {
                if (id == 1)
                {
                    IntroTimer = 1500;
                    IntroPhase = 1;
                }
                else if (id == 2)
                {
                    me->SetFlying(false);
                    me->SetUnitMovementFlags(MOVEMENTFLAG_NONE);
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    me->CastSpell(me, SPELL_SOUND_BAR_SHOW_PERIODIC, true);
                }
            }

            void SpellHitTarget(Unit* pTarget, const SpellEntry* spell)
            {
                if (pTarget && pTarget->ToPlayer() && spell->Id == SPELL_SOUND_BAR_SHOW)
                    pTarget->SetMaxPower(POWER_SCRIPTED, 100);
            }

            void JustReachedHome()
            {
                me->RemoveAurasDueToSpell(SPELL_SOUND_BAR_SHOW_PERIODIC);
                InstanceWideUnaura(SPELL_SOUND_BAR_SHOW);

                GameObject* pGo = GetClosestGameObjectWithEntry(me, GO_ANCIENT_BELL, 100.0f);
                if (pGo)
                {
                    pGo->Respawn();
                    pGo->SetGoState(GO_STATE_READY);
                }
                me->SetVisibility(VISIBILITY_OFF);
                me->ForcedDespawn();
            }

            void InstanceWideUnaura(uint32 spell)
            {
                if (!pInstance)
                    return;

                Map::PlayerList const& plList = pInstance->instance->GetPlayers();
                if (!plList.isEmpty())
                {
                    for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
                        if ((*itr).getSource())
                            (*itr).getSource()->RemoveAurasDueToSpell(spell);
                }
            }

            void JustDied(Unit* pKiller)
            {
                me->RemoveAurasDueToSpell(SPELL_SOUND_BAR_SHOW_PERIODIC);
                InstanceWideUnaura(SPELL_SOUND_BAR_SHOW);
            }

            void UpdateAI(const uint32 diff)
            {
                if (IntroTimer)
                {
                    if (IntroTimer <= diff)
                    {
                        switch (IntroPhase)
                        {
                            case 0:
                            default:
                                IntroTimer = 0;
                                break;
                            case 1:
                            {
                                me->CastSpell(me, SPELL_FLAME_BREATH_DUMMY, false);
                                GameObject* pGo = GetClosestGameObjectWithEntry(me, GO_ANCIENT_BELL, 100.0f);
                                if (pGo)
                                {
                                    pGo->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                                }
                                me->SetSpeed(MOVE_RUN, 1.0f, true);
                                me->SetSpeed(MOVE_FLIGHT, 1.0f, true);
                                IntroTimer = 4500;
                                IntroPhase++;
                                break;
                            }
                            case 2:
                            {
                                me->GetMotionMaster()->MovePoint(2, BossPath[2]);
                                IntroTimer = 0;
                                break;
                            }
                        }
                    }
                    else
                        IntroTimer -= diff;
                }

                if (!UpdateVictim())
                {
                    if (!me->IsFlying())
                    {
                        me->SetFlying(true);
                        me->SetUnitMovementFlags(MOVEMENTFLAG_LEVITATING);
                    }
                    return;
                }

                me->SetFlying(false);
                me->SetUnitMovementFlags(MOVEMENTFLAG_NONE);

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new boss_atramedesAI(c);
        }
};

class go_ancient_bell_atramedes: public GameObjectScript
{
    public:
        go_ancient_bell_atramedes(): GameObjectScript("go_ancient_bell_atramedes") {}

        bool OnGossipHello(Player* pPlayer, GameObject* pObject)
        {
            Creature* pBoss = GetClosestCreatureWithEntry(pObject, NPC_ATRAMEDES, 50000.0f);
            if (pBoss)
                return true;

            pBoss = GetClosestCreatureWithEntry(pObject, NPC_ATRAMEDES, 50000.0f, false);
            if (pBoss && pBoss->GetVisibility() == VISIBILITY_ON)
                return true;

            pBoss = pObject->SummonCreature(NPC_ATRAMEDES, BossPath[0]);

            if (pBoss)
            {
                pBoss->SetSpeed(MOVE_RUN, 0.65f, true);
                pBoss->SetSpeed(MOVE_FLIGHT, 0.65f, true);
                pBoss->SetUnitMovementFlags(MOVEMENTFLAG_LEVITATING);
                pBoss->SetFlying(true);
                pBoss->SetReactState(REACT_PASSIVE);
                pBoss->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                pBoss->GetMotionMaster()->MovePoint(1, BossPath[1]);
            }

            return true;
        }
};

void AddSC_atramedes()
{
    new boss_atramedes;
    new go_ancient_bell_atramedes;
}

/**** SQL:

UPDATE creature_template SET ScriptName='boss_atramedes' WHERE entry=41442;
REPLACE INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `faction`, `flags`, `size`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `data0`, `data1`, `data2`, `data3`, `data4`, `data5`, `data6`, `data7`, `data8`, `data9`, `data10`, `data11`, `data12`, `data13`, `data14`, `data15`, `data16`, `data17`, `data18`, `data19`, `data20`, `data21`, `data22`, `data23`, `AIName`, `ScriptName`, `WDBVerified`) VALUES (459704, 1, 9704, 'Ancient Bell', '', '', '', 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, -1, 120, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 'go_ancient_bell_atramedes', 1);

*/
