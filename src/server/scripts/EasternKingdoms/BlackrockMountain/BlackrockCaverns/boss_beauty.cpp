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
UPDATE creature_template SET ScriptName='boss_beauty', AIName='' WHERE entry=39700;
UPDATE creature_template SET ScriptName='mob_child_of_beauty', AIName='' WHERE entry IN (40008,40011,40013,40015);
UPDATE creature_template SET AIName='NullAI', modelid1=16946 WHERE entry=51445;
REPLACE INTO spell_script_names VALUES (76031, 'spell_beauty_magma_spit');
*/

enum
{
    NPC_BEAUTY = 39700,

    NPC_LUCKY  = 40008,
    NPC_SPOT   = 40011,
    NPC_BUSTER = 40013,
    NPC_RUNTY  = 40015,
    NPC_MAGMA_POOL = 51445,

    SPELL_LAVA_DROOL     = 76628,
    SPELL_LAVA_DROOL_H   = 93666,
    SPELL_FLAME_BREATH   = 76665,
    SPELL_FLAME_BREATH_H = 93667,

    SPELL_BERSERK           = 82395,
    SPELL_CHARGE            = 76030,
    SPELL_CHARGE_H          = 93580,
    SPELL_FLAMEBREAK        = 76032,
    SPELL_FLAMEBREAK_H      = 93583,
    SPELL_MAGMA_SPIT        = 76031,
    SPELL_TERRIFYING_ROAR   = 76028,
    SPELL_TERRIFYING_ROAR_H = 93586,
};

class boss_beauty: public CreatureScript
{
    public:
        boss_beauty(): CreatureScript("boss_beauty") {};

        struct boss_beautyAI: public ScriptedAI
        {
            boss_beautyAI(Creature* c): ScriptedAI(c)
            {
                pInstance = me->GetInstanceScript();
                Reset();
            }

            InstanceScript* pInstance;

            void Reset()
            {
                ChargeTimer = urand(6000,12000);
                FlamebreakTimer = urand(17000,20000);
                MagmaSpitTimer = urand(7000,13000);
                TerrifyingRoarTimer = urand(25000,32000);

                //Respawnovat addy
                //Lucky, Spot, Buster, Runty
                if (Creature* pLucky = GetClosestCreatureWithEntry(me,NPC_LUCKY,1000.0f, false))
                    pLucky->Respawn();
                if (Creature* pSpot = GetClosestCreatureWithEntry(me,NPC_SPOT,1000.0f, false))
                    pSpot->Respawn();
                if (Creature* pBuster = GetClosestCreatureWithEntry(me,NPC_BUSTER,1000.0f, false))
                    pBuster->Respawn();
                if (Creature* pRunty = GetClosestCreatureWithEntry(me,NPC_RUNTY,1000.0f, false))
                    pRunty->Respawn();
            }

            uint32 ChargeTimer;
            uint32 FlamebreakTimer;
            uint32 MagmaSpitTimer;
            uint32 TerrifyingRoarTimer;

            Player* SelectFurthestPlayer()
            {
                if (!pInstance)
                    return NULL;

                Map::PlayerList const& plList = pInstance->instance->GetPlayers();

                if (plList.isEmpty())
                    return NULL;

                float maxrange = 0.0f;
                Player* maxrangeplayer = NULL;
                for(Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
                {
                    if (me->GetDistance2d(itr->getSource()) > maxrange)
                        maxrangeplayer = itr->getSource();
                }

                return maxrangeplayer;
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (ChargeTimer <= diff)
                {
                    Player* pTarget = SelectFurthestPlayer();
                    if (pTarget)
                        me->CastSpell(pTarget, DUNGEON_MODE(SPELL_CHARGE,SPELL_CHARGE_H), false);
                    ChargeTimer = urand(16000,20000);
                } else ChargeTimer -= diff;

                if (FlamebreakTimer <= diff)
                {
                    me->CastSpell(me, DUNGEON_MODE(SPELL_FLAMEBREAK,SPELL_FLAMEBREAK_H), false);
                    FlamebreakTimer = urand(17000,20000);
                } else FlamebreakTimer -= diff;

                if (MagmaSpitTimer <= diff)
                {
                    Player* pTarget = SelectRandomPlayer(90.0f);
                    if (pTarget)
                        me->CastSpell(pTarget, SPELL_MAGMA_SPIT, true);
                    MagmaSpitTimer = urand(7000,13000);
                } else MagmaSpitTimer -= diff;

                if (TerrifyingRoarTimer <= diff)
                {
                    me->CastSpell(me, DUNGEON_MODE(SPELL_TERRIFYING_ROAR,SPELL_TERRIFYING_ROAR_H), false);
                    TerrifyingRoarTimer = urand(25000,32000);
                } else TerrifyingRoarTimer -= diff;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new boss_beautyAI(c);
        }
};

class mob_child_of_beauty: public CreatureScript
{
    public:
        mob_child_of_beauty(): CreatureScript("mob_child_of_beauty") {};

        struct mob_child_of_beautyAI: public ScriptedAI
        {
            mob_child_of_beautyAI(Creature* c): ScriptedAI(c)
            {
                Reset();
            }

            void Reset()
            {
                LavaDroolTimer = urand(170,200)*100;
                BreathTimer = urand(100,130)*100;
            }

            uint32 LavaDroolTimer;
            uint32 BreathTimer;

            void EnterCombat(Unit* pVictim)
            {
                // Sdilene aggro
                Creature* pMom = GetClosestCreatureWithEntry(me, NPC_BEAUTY, 5000.0f);
                Creature* pLucky = GetClosestCreatureWithEntry(me,NPC_LUCKY,1000.0f);
                Creature* pSpot = GetClosestCreatureWithEntry(me,NPC_SPOT,1000.0f);
                Creature* pBuster = GetClosestCreatureWithEntry(me,NPC_BUSTER,1000.0f);
                if (pMom && pMom->isAlive() && !pMom->isInCombat())
                    pMom->AI()->AttackStart(pVictim);
                if (pLucky && pLucky->isAlive() && !pLucky->isInCombat())
                    pLucky->AI()->AttackStart(pVictim);
                if (pSpot && pSpot->isAlive() && !pSpot->isInCombat())
                    pSpot->AI()->AttackStart(pVictim);
                if (pBuster && pBuster->isAlive() && !pBuster->isInCombat())
                    pBuster->AI()->AttackStart(pVictim);
            }

            void JustDied(Unit* pKiller)
            {
                if (me->GetEntry() == NPC_RUNTY)
                {
                    Creature* pMom = GetClosestCreatureWithEntry(me, NPC_BEAUTY, 5000.0f);
                    if (pMom && pMom->isAlive())
                        pMom->CastSpell(pMom, SPELL_BERSERK, true);
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (LavaDroolTimer <= diff)
                {
                    me->CastSpell(me, DUNGEON_MODE(SPELL_LAVA_DROOL,SPELL_LAVA_DROOL_H), false);
                    LavaDroolTimer = urand(170,200)*100;
                } else LavaDroolTimer -= diff;

                if (BreathTimer <= diff)
                {
                    me->CastSpell(me, DUNGEON_MODE(SPELL_FLAME_BREATH,SPELL_FLAME_BREATH_H), false);
                    BreathTimer = urand(100,130)*100;
                } else BreathTimer -= diff;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new mob_child_of_beautyAI(c);
        }
};

class spell_beauty_magma_spit : public SpellScriptLoader
{
    public:
        spell_beauty_magma_spit() : SpellScriptLoader("spell_beauty_magma_spit") { }

        class spell_beauty_magma_spitAuraScript : public AuraScript
        {
            PrepareAuraScript(spell_beauty_magma_spitAuraScript)
            void HandleEffectRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* pTarget = GetTarget();

                pTarget->CastSpell(pTarget, 76072, true);
                pTarget->CastSpell(pTarget, 76074, true);
                pTarget->CastSpell(pTarget, 76076, true);
                pTarget->CastSpell(pTarget, 76058, true);
            }

            void Register()
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_beauty_magma_spitAuraScript::HandleEffectRemove, EFFECT_1, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript *GetAuraScript() const
        {
            return new spell_beauty_magma_spitAuraScript();
        }
};

void AddSC_beauty()
{
    new boss_beauty();
    new mob_child_of_beauty();
    new spell_beauty_magma_spit();
}
