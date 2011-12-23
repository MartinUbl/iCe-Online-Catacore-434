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

enum Spells
{
    SPELL_MAGMA_SPIT = 78359,
    SPELL_MAGMA_SPIT_WITH_ENRAGE = 78068,
    SPELL_LAVA_SPEW = 77690,
    SPELL_PILLAR_OF_FLAME_EXPLOSION = 77970, // 4 seconds after spawning
    SPELL_PILLAR_OF_FLAME_VISUAL = 78017, //flaming circle
    SPELL_MANGLE = 89773,
    SPELL_IMPALE_SELF = 77907, // used for exposing self
    SPELL_LAVA_SPLASH_VISUAL = 79461,

    SPELL_POINT_OF_VULNERABILITY_SHARE_DMG = 79010,
    SPELL_POINT_OF_VULNERABILITY = 79011,

    // Lava Parasite
    SPELL_PARASITIC_INFECTION = 78941,
    SPELL_PARASITIC_INFECTION_10SEC_TRIGGER = 78079,
};

enum NPCs
{
    NPC_PILLAR_OF_FLAME = 41843,
    NPC_LAVA_PARASITE = 42321,
};

#define PINCER_COUNT 4
static const Position PincerPos[PINCER_COUNT] = {
    {-314.6449f, -43.6197f, 211.8921f, 0.8f},
    {-311.0326f, -46.6951f, 211.9754f, 0.8f},
    {-313.0673f, -48.5738f, 212.0910f, 0.8f},
    {-316.3784f, -46.3259f, 212.0263f, 0.8f},
};

#define PINCER_USERS_10M 3
#define PINCER_USERS_25M 6

#define PINCER_USERS RAID_MODE(PINCER_USERS_10M, PINCER_USERS_25M, PINCER_USERS_10M, PINCER_USERS_25M)

class boss_magmaw: public CreatureScript
{
public:
    boss_magmaw(): CreatureScript("boss_magmaw") {};

    struct boss_magmawAI: public Scripted_NoMovementAI
    {
        boss_magmawAI(Creature* c): Scripted_NoMovementAI(c)
        {
            // Aby nam nespadnul
            me->SetUnitMovementFlags(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING);
            Reset();
            if ((pHead = me->SummonCreature(42347, 0,0,0)) != NULL)
            {
                me->CastSpell(pHead, SPELL_POINT_OF_VULNERABILITY_SHARE_DMG, true);
                pHead->EnterVehicle(me, 1, false);
                pHead->addUnitState(UNIT_STAT_UNATTACKABLE);
                pHead->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            }
        }

        uint32 MagmaSpitTimer;
        uint32 LavaSpewTimer;
        uint32 PillarOfFlameTimer;
        uint32 SpawnPincerTimer;
        uint32 DespawnPincerTimer;
        GameObject* pPincers[PINCER_COUNT];
        Creature* pHead;
        uint32 DeExposeTimer;
        uint64 PincerUserGUIDs[PINCER_USERS_25M];

        void Reset()
        {
            // Just for being sure that we arrive at homepoint even visual
            me->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
        }

        void EnterCombat(Unit* pWho)
        {
            MagmaSpitTimer = 15000;
            LavaSpewTimer = 12000;
            PillarOfFlameTimer = 35000;
            DeExposeTimer = 0;
            SpawnPincerTimer = 5000;
            DespawnPincerTimer = 0;
            for (uint8 i = 0; i < PINCER_USERS_25M; i++)
                PincerUserGUIDs[i] = 0;
            for (uint8 i = 0; i < PINCER_COUNT; i++)
                pPincers[i] = NULL;
        }

        void JustDied(Unit* pWho)
        {
            if (pHead)
            {
                pHead->ExitVehicle();
                pHead->Relocate(-307.228363f,-45.881611f,212.010483f);
                pWho->Kill(pHead);
            }
        }

        bool HasUsedPincer(uint64 guid)
        {
            for (uint8 i = 0; i < PINCER_USERS_25M; i++)
            {
                if (PincerUserGUIDs[i] == guid)
                    return true;
            }
            return false;
        }

        uint8 UsedPincersNum()
        {
            uint8 result = 0;
            for (uint8 i = 0; i < PINCER_USERS_25M; i++)
            {
                if (PincerUserGUIDs[i] > 0)
                    result++;
            }
            return result;
        }

        uint8 GetFreePincerGUIDPos()
        {
            for (uint8 i = 0; i < PINCER_USERS_25M; i++)
            {
                if (PincerUserGUIDs[i] == 0)
                    return i;
            }
            return 255;
        }

        void CheckChains()
        {
            if (UsedPincersNum() >= PINCER_USERS)
            {
                DeExposeTimer = 38000;
                DespawnPincerTimer = 6000;
                me->CastSpell(me, SPELL_IMPALE_SELF, false);
                if (pHead)
                {
                    pHead->clearUnitState(UNIT_STAT_UNATTACKABLE);
                    pHead->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                }
            }
        }

        bool AddPincerUser(uint64 guid)
        {
            if (DeExposeTimer > 0)
                return false;
            if (HasUsedPincer(guid))
                return false;

            if (UsedPincersNum() < PINCER_USERS)
            {
                uint8 pos = GetFreePincerGUIDPos();
                if (pos >= PINCER_USERS_25M)
                    return false;

                PincerUserGUIDs[pos] = guid;
                CheckChains();
                return true;
            }
            CheckChains(); // projistotu
            return false;
        }

        void UpdateAI(const uint32 diff)
        {
            if (DespawnPincerTimer)
            {
                if (DespawnPincerTimer <= diff)
                {
                    for (uint8 i = 0; i < PINCER_COUNT; i++)
                    {
                        if (pPincers[i])
                            pPincers[i]->Delete();
                        pPincers[i] = NULL;
                    }
                    DespawnPincerTimer = 0;
                }
                else
                    DespawnPincerTimer -= diff;
            }

            if (!UpdateVictim())
                return;

            if (DeExposeTimer == 0)
            {
                if (MagmaSpitTimer <= diff)
                {
                    me->CastSpell(me->getVictim(), SPELL_MAGMA_SPIT, false);
                    MagmaSpitTimer = urand(12000, 15000);
                }
                else
                    MagmaSpitTimer -= diff;

                if (LavaSpewTimer <= diff)
                {
                    me->CastSpell(me, SPELL_LAVA_SPEW, false);
                    LavaSpewTimer = urand(12000, 15000);
                }
                else
                    LavaSpewTimer -= diff;

                if (PillarOfFlameTimer <= diff)
                {
                    Unit* pRandom = SelectUnit(SELECT_TARGET_RANDOM, 0);
                    if (pRandom)
                        me->SummonCreature(NPC_PILLAR_OF_FLAME, pRandom->GetPositionX(), pRandom->GetPositionY(), pRandom->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 8500);
                    PillarOfFlameTimer = urand(32000, 35000);
                }
                else
                    PillarOfFlameTimer -= diff;

                if (SpawnPincerTimer <= diff)
                {
                    for (uint8 i = 0; i < PINCER_COUNT; i++)
                        pPincers[i] = me->getVictim()->SummonGameObject(800001, PincerPos[i].m_positionX, PincerPos[i].m_positionY, PincerPos[i].m_positionZ, PincerPos[i].m_orientation, 0.0f, 0.0f, 0.0f, 0.0f, 0);
                    DespawnPincerTimer = 15000;
                    SpawnPincerTimer = 45000;
                }
                else
                    SpawnPincerTimer -= diff;
            }
            else
            {
                if (DeExposeTimer <= diff)
                {
                    if (pHead)
                    {
                        pHead->RemoveAllAuras();
                        me->CastSpell(pHead, SPELL_POINT_OF_VULNERABILITY_SHARE_DMG, true);
                        pHead->addUnitState(UNIT_STAT_UNATTACKABLE);
                        pHead->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                    }
                    DeExposeTimer = 0;
                } else DeExposeTimer -= diff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new boss_magmawAI(c);
    }
};

class mob_pillar_of_flame_magmaw: public CreatureScript
{
public:
    mob_pillar_of_flame_magmaw(): CreatureScript("mob_pillar_of_flame_magmaw") {};

    struct mob_pillar_of_flame_magmawAI: public Scripted_NoMovementAI
    {
        mob_pillar_of_flame_magmawAI(Creature* c): Scripted_NoMovementAI(c)
        {
            Reset();
        }

        uint32 ExplodeTimer;
        uint32 UnAuraTimer;

        void Reset()
        {
            ExplodeTimer = 4000;
            UnAuraTimer = 7500;
            me->CastSpell(me, SPELL_PILLAR_OF_FLAME_VISUAL, false);
        }

        void AttackStart(Unit* who)
        {
            return;
        }

        void UpdateAI(const uint32 diff)
        {
            if (ExplodeTimer <= diff)
            {
                me->CastSpell(me, SPELL_PILLAR_OF_FLAME_EXPLOSION, false);
                ExplodeTimer = 20000;
            }
            else
                ExplodeTimer -= diff;

            if (UnAuraTimer <= diff)
            {
                me->RemoveAurasDueToSpell(SPELL_PILLAR_OF_FLAME_VISUAL);
                UnAuraTimer = 20000;
            }
            else
                UnAuraTimer -= diff;
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new mob_pillar_of_flame_magmawAI(c);
    }
};

class mob_lava_worm_magmaw: public CreatureScript
{
public:
    mob_lava_worm_magmaw(): CreatureScript("mob_lava_worm_magmaw") {};

    struct mob_lava_worm_magmawAI: public ScriptedAI
    {
        mob_lava_worm_magmawAI(Creature* c): ScriptedAI(c)
        {
            Reset();
        }

        uint32 ApplyTimer;

        void Reset()
        {
            ApplyTimer = 0;
        }

        void DamageDealt(Unit* pVictim, uint32 &damage, DamageEffectType type)
        {
            if (type == DIRECT_DAMAGE && pVictim && pVictim->GetTypeId() == TYPEID_PLAYER)
            {
                if (ApplyTimer == 0)
                {
                    DoCastVictim(SPELL_PARASITIC_INFECTION);
                    DoCastVictim(SPELL_PARASITIC_INFECTION_10SEC_TRIGGER);
                    ApplyTimer = 1000;
                }
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
            {
                ApplyTimer = 0;
                return;
            }

            if (ApplyTimer <= diff)
            {
                ApplyTimer = 0;
            }
            else
                ApplyTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new mob_lava_worm_magmawAI(c);
    }
};

class go_magmaw_spike: public GameObjectScript
{
public:
    go_magmaw_spike(): GameObjectScript("go_magmaw_spike") { }

    bool OnGossipHello(Player *pPlayer, GameObject *pGO)
    {
        Creature* pBoss = GetClosestCreatureWithEntry(pPlayer, 41570, 100.0f, true);
        if (!pBoss)
            return false;

        if (((boss_magmaw::boss_magmawAI*)pBoss->GetAI())->AddPincerUser(pPlayer->GetGUID()))
        {
            // chain visual
            Creature* pTrigger = pPlayer->SummonCreature(90007, 0, 0, 0, 0, TEMPSUMMON_TIMED_DESPAWN, 10000);
            if (pTrigger)
            {
                pTrigger->SetReactState(REACT_PASSIVE);
                pTrigger->CastSpell(pBoss, 54324, false);
            }
        }

        return true;
    }
};

void AddSC_magmaw()
{
    new boss_magmaw();
    new mob_pillar_of_flame_magmaw();
    new mob_lava_worm_magmaw();
    new go_magmaw_spike();
}

/**** SQL:

UPDATE creature_template SET vehicleId=781,ScriptName='boss_magmaw' WHERE entry=41570;
UPDATE creature_template SET modelid1=16946,modelid2=0,modelid3=0,modelid4=0,unit_flags=2,faction_A=14,faction_H=14,ScriptName='mob_pillar_of_flame_magmaw' WHERE entry=41843;
UPDATE creature_template SET ScriptName='mob_lava_worm_magmaw' WHERE entry IN (41806,42321);
REPLACE INTO `creature_model_info` (`modelid`, `bounding_radius`, `combat_reach`, `gender`, `modelid_other_gender`) VALUES (32679, 15, 15, 2, 0);
DELETE FROM creature_template_addon WHERE entry=42347;

REPLACE INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `faction`, `flags`, `size`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `data0`, `data1`, `data2`, `data3`, `data4`, `data5`, `data6`, `data7`, `data8`, `data9`, `data10`, `data11`, `data12`, `data13`, `data14`, `data15`, `data16`, `data17`, `data18`, `data19`, `data20`, `data21`, `data22`, `data23`, `AIName`, `ScriptName`, `WDBVerified`) VALUES (800001, 10, 10268, 'Chain Spike', '', 'Chaining', '', 31, 6553600, 0.3, 0, 0, 0, 0, 0, 0, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1327, '', 'go_magmaw_spike', 13623);

*/
