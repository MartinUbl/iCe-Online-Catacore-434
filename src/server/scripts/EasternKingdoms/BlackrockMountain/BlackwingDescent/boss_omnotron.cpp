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

#define ERROR_INST_DATA           "Problem s Instanci."

enum Texts
{
    TEXT_START                      = -1999970,
    TEXT_AGGRO                      = -1999971,
    TEXT_DEATH                      = -1999972
};

enum Spells
{
    // spolecne spelly
    SPELL_ACTIVE                    = 78740,
    SPELL_INACTIVE                  = 78726,
    // ARCANOTRON
    SPELL_POWER_GENERATOR           = 79624,//(NPC id 42733, spell , aura 79628)
    SPELL_ARCANE_ANNIHILATOR        = 79710,
    SPELL_POWER_CONVERSION          = 79729,
    // ELECTRON
    SPELL_UNSTABLE_SHIELD           = 79900,
    SPELL_ELECTRICIAL_DISCHARGE     = 81055,
    SPELL_LIGHTNING_CONSDUCTOR      = 79889,
    // TOXITRON
    SPELL_CHEMICAL_BOMB             = 80157,
    SPELL_POISON_PROTOCOL           = 80053,
    SPELL_POISON_SOAKED_SHELL       = 79835
};


enum Creatures
{
    NPC_POWER_GENERATOR = 42733,
};

enum Events
{
    EVENT_INTRO,
    EVENT_ACTIVEAI,
};

/*
class npc_omnotrion_triger : public CreatureScript
{
public:
    npc_omnotrion_triger() : CreatureScript("npc_omnotrion_triger") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_omnotrion_trigerAI (creature);
    }

    struct npc_omnotrion_trigerAI : public BossAI
    {
        npc_omnotrion_trigerAI(Creature *c) : BossAI(c, BOSS_OMNOTRION_DEFENSE_SYSTEM)
        {
        }

        bool Intro;

        void MoveInLineOfSight(Unit* who)
        {
        }

        void Reset()
        {
            Intro = false;
            events.ScheduleEvent(EVENT_INTRO,2000);
            events.ScheduleEvent(EVENT_ACTIVEAI,5000);
        }

        void UpdateAI(const uint32 diff)
        {
            events.Update(diff);
            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_INTRO:
                        me->MonsterSay("Hello",0,0);
                        break;
                    case EVENT_ACTIVEAI:
                        me->MonsterSay("This is Intro",0,0);
                        std::list<Creature*> StartBossList;
                        GetCreatureListWithEntryInGrid(StartBossList, me, NPC_ARCANOTRON, 100.0f);
                        if (!StartBossList.empty())
                        {
                            for (std::list<Creature*>::iterator itr = StartBossList.begin(); itr !=StartBossList.end(); ++itr)
                            {
                                (*itr)->MonsterSay("Hello",0,0);
                                (*itr)->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                            }
                        }
                        break;
                }
            }
        }
    };
};
*/


class boss_Arcanotrom : public CreatureScript
{
public:
    boss_Arcanotrom() : CreatureScript("Boss_Arcanotron") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_ArcanotromAI (pCreature);
    }

    struct boss_ArcanotromAI : public ScriptedAI
    {
        boss_ArcanotromAI(Creature *c) : ScriptedAI(c)
        {
            pInstance = c->GetInstanceScript();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
            me->setPowerType(POWER_ENERGY);
        }
        InstanceScript* pInstance;
        uint32 Spawned;
        uint32 pElectron;
        uint32 pToxitron;
        uint32 uiArcaneAnnihilator;
        uint32 uiPowerGenerator;
        bool Shield;
        bool Recharge;

        void Reset()
        {
            if (pInstance)
            pInstance->SetData(DATA_ELECTRON_GUID, NOT_STARTED);
            me->SetPower(POWER_ENERGY,100);
            uiArcaneAnnihilator = 3000;
            uiPowerGenerator    = 15000;
            Shield = false;
            Recharge = false;
        }

        void DamageTaken(Unit* who, uint32& uiDamage)
        {
            std::list<Creature*> BossList;
            GetCreatureListWithEntryInGrid(BossList, me, NPC_ARCANOTRON, 100.0f);
            GetCreatureListWithEntryInGrid(BossList, me, NPC_ELECTRON, 100.0f);
            GetCreatureListWithEntryInGrid(BossList, me, NPC_TOXITRON, 100.0f);
            GetCreatureListWithEntryInGrid(BossList, me, NPC_MAGMATRON, 100.0f);
            if (!BossList.empty())
            {
                for (std::list<Creature*>::iterator itr = BossList.begin(); itr !=BossList.end(); ++itr)
                {
                    uint64 ElectronGUID = 0;
                    uint64 ToxitronGUID = 0;
                    uint64 ArcanotronGUID = 0;
                    uint64 MagmatronGUID = 0;

                    if(pInstance)
                    {
                        ElectronGUID = pInstance->GetData64(DATA_ELECTRON_GUID);
                        ToxitronGUID = pInstance->GetData64(DATA_TOXITRON_GUID);
                        ArcanotronGUID = pInstance->GetData64(DATA_ARCANOTRON_GUID);
                        MagmatronGUID = pInstance->GetData64(DATA_MAGMATRON_GUID);
                    }

                    if (!who || (who->GetTypeId() != TYPEID_PLAYER && !who->ToPet()))
                        return;

                    if (Creature* pToxitron = me->GetCreature(*me, ToxitronGUID))
                    {
                        uiDamage /= 4;
                        me->DealDamage(pToxitron, uiDamage);
                        me->LowerPlayerDamageReq(uiDamage);
                    }

                    if (Creature* pElectron = me->GetCreature(*me, ElectronGUID))
                    {
                        uiDamage /= 4;
                        me->DealDamage(pElectron, uiDamage);
                        me->LowerPlayerDamageReq(uiDamage);
                    }

                    if (Creature* pMagmatron = me->GetCreature(*me,MagmatronGUID))
                    {
                        uiDamage /= 4;
                        me->DealDamage(pMagmatron, uiDamage);
                        me->LowerPlayerDamageReq(uiDamage);
                    }
                }
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            me->RemoveAura(SPELL_INACTIVE);
            DoCast(SPELL_ACTIVE);
            if (pInstance)
                pInstance->SetData(DATA_ARCANOTRON_GUID, IN_PROGRESS);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (uiArcaneAnnihilator <= diff)
            {
                if (Unit *pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                    DoCast(pTarget, SPELL_ARCANE_ANNIHILATOR);
                uiArcaneAnnihilator = urand(12000,15000);
            }
            else
                uiArcaneAnnihilator -= diff;

            if (uiPowerGenerator <= diff)
            {
                if (Unit *pTarget = SelectTarget(SELECT_TARGET_TOPAGGRO, 0, 100, true))
                    if (Creature *pGenerator = me->SummonCreature(NPC_POWER_GENERATOR, pTarget->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 30000))
                        pGenerator->CastSpell(pGenerator,SPELL_POWER_GENERATOR,true);

                uiPowerGenerator = urand(35000,39000);
            }
            else
                uiPowerGenerator -= diff;

            if (me->GetPower(POWER_ENERGY) > 50 && Shield)
            {
                DoCast(me,SPELL_POWER_CONVERSION);
                Shield = true;
            }

            if (me->GetPower(POWER_ENERGY) >0&& !Recharge)
            {
                DoCast(me,SPELL_POWER_CONVERSION);
                Recharge = true;
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            DoScriptText(TEXT_DEATH,me);
            if (pInstance)
                pInstance->SetData(DATA_ARCANOTRON_GUID, DONE);
        }

        void KilledUnit(Unit * victim)
        {
        }
    };
};


class boss_Toxitron : public CreatureScript
{
public:
    boss_Toxitron () : CreatureScript("Boss_Toxitron") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_ToxitronAI (pCreature);
    }

    struct boss_ToxitronAI : public ScriptedAI
    {
        boss_ToxitronAI(Creature *c) : ScriptedAI(c)
        {
            pInstance = c->GetInstanceScript();
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
            me->setPowerType(POWER_ENERGY);
        }
        InstanceScript* pInstance;

        uint32 uiChemicalBomb;
        uint32 uiPoisonProtocol;
        uint32 uiPoisonSoakedShell;

        void Reset()
        {
            if (pInstance)
                pInstance->SetData(DATA_TOXITRON_GUID, NOT_STARTED);
            me->SetPower(POWER_ENERGY,100);
            uiChemicalBomb          = 4000;
            uiPoisonProtocol        = 40000;
        }

        void DamageTaken(Unit* who, uint32& uiDamage)
        {
            std::list<Creature*> BossList;
            GetCreatureListWithEntryInGrid(BossList, me, NPC_ARCANOTRON, 100.0f);
            GetCreatureListWithEntryInGrid(BossList, me, NPC_ELECTRON, 100.0f);
            GetCreatureListWithEntryInGrid(BossList, me, NPC_TOXITRON, 100.0f);
            GetCreatureListWithEntryInGrid(BossList, me, NPC_MAGMATRON, 100.0f);
            if (!BossList.empty())
            {
                for (std::list<Creature*>::iterator itr = BossList.begin(); itr !=BossList.end(); ++itr)
                {
                    uint64 ElectronGUID = 0;
                    uint64 ToxitronGUID = 0;
                    uint64 ArcanotronGUID = 0;
                    uint64 MagmatronGUID = 0;

                    if(pInstance)
                    {
                        ElectronGUID = pInstance->GetData64(DATA_ELECTRON_GUID);
                        ToxitronGUID = pInstance->GetData64(DATA_TOXITRON_GUID);
                        ArcanotronGUID = pInstance->GetData64(DATA_ARCANOTRON_GUID);
                        MagmatronGUID = pInstance->GetData64(DATA_MAGMATRON_GUID);
                    }

                    if (!who || (who->GetTypeId() != TYPEID_PLAYER && !who->ToPet()))
                        return;

                    if (Creature* pElectron = me->GetCreature(*me, ElectronGUID))
                    {
                        uiDamage /= 4;
                        me->DealDamage(pElectron, uiDamage);
                        me->LowerPlayerDamageReq(uiDamage);
                    }

                    if (Creature* pArcanotron = me->GetCreature(*me, ArcanotronGUID))
                    {
                        uiDamage /= 4;
                        me->DealDamage(pArcanotron, uiDamage);
                        me->LowerPlayerDamageReq(uiDamage);
                    }

                    if (Creature* pMagmatron = me->GetCreature(*me,MagmatronGUID))
                    {
                        uiDamage /= 4;
                        me->DealDamage(pMagmatron, uiDamage);
                        me->LowerPlayerDamageReq(uiDamage);
                    }
                }
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            me->RemoveAura(SPELL_INACTIVE);
            DoCast(SPELL_ACTIVE);

            if (pInstance)
                pInstance->SetData(DATA_ARCANOTRON_GUID, IN_PROGRESS);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (uiChemicalBomb <= diff)
            {
                if (Unit *pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                    DoCast(pTarget, SPELL_CHEMICAL_BOMB);
                uiChemicalBomb = urand(9000,10000);
            }
            else
                uiChemicalBomb -= diff;

            if (uiPoisonProtocol <= diff)
            {
                if (Unit *pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                    DoCast(pTarget, SPELL_POISON_PROTOCOL);
                uiPoisonProtocol = urand(15000,19000);
            }
            else
                uiPoisonProtocol -= diff;

            DoMeleeAttackIfReady();
        }


        void JustDied(Unit* /*killer*/)
        {
            DoScriptText(TEXT_DEATH,me);
            if (pInstance)
                pInstance->SetData(DATA_TOXITRON_GUID, DONE);
        }

        void KilledUnit(Unit * victim)
        {
        }
    };
};

class boss_Electron : public CreatureScript
{
public:
    boss_Electron () : CreatureScript("Boss_Electron") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_ElectronAI (pCreature);
    }

    struct boss_ElectronAI : public ScriptedAI
    {
        boss_ElectronAI(Creature *c) : ScriptedAI(c)
        {
            pInstance = c->GetInstanceScript();
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
            me->setPowerType(POWER_ENERGY);

        }
        InstanceScript* pInstance;

        uint32 uiLightningConductor;
        uint32 uiUnstableShield;
        uint32 uiElectricalDischarge;

        void Reset()
        {
            if (pInstance)
                pInstance->SetData(DATA_ARCANOTRON_GUID, NOT_STARTED);
            me->SetPower(POWER_ENERGY,100);
            uiLightningConductor    = 3000;
            uiUnstableShield        = 30000;
            uiElectricalDischarge   = 35000;
        }

        void DamageTaken(Unit* who, uint32& uiDamage)
        {
            std::list<Creature*> BossList;
            GetCreatureListWithEntryInGrid(BossList, me, NPC_ARCANOTRON, 100.0f);
            GetCreatureListWithEntryInGrid(BossList, me, NPC_ELECTRON, 100.0f);
            GetCreatureListWithEntryInGrid(BossList, me, NPC_TOXITRON, 100.0f);
            GetCreatureListWithEntryInGrid(BossList, me, NPC_MAGMATRON, 100.0f);
            if (!BossList.empty())
            {
                for (std::list<Creature*>::iterator itr = BossList.begin(); itr !=BossList.end(); ++itr)
                {
                    uint64 ElectronGUID = 0;
                    uint64 ToxitronGUID = 0;
                    uint64 ArcanotronGUID = 0;
                    uint64 MagmatronGUID = 0;

                    if(pInstance)
                    {
                        ElectronGUID = pInstance->GetData64(DATA_ELECTRON_GUID);
                        ToxitronGUID = pInstance->GetData64(DATA_TOXITRON_GUID);
                        ArcanotronGUID = pInstance->GetData64(DATA_ARCANOTRON_GUID);
                        MagmatronGUID = pInstance->GetData64(DATA_MAGMATRON_GUID);
                    }

                    if (!who || who->GetTypeId() != TYPEID_PLAYER)
                        return;

                    if (Creature* pToxitron = me->GetCreature(*me, ToxitronGUID))
                    {
                        uiDamage /= 4;
                        me->DealDamage(pToxitron, uiDamage);
                        me->LowerPlayerDamageReq(uiDamage);
                    }

                    if (Creature* pArcanotron = me->GetCreature(*me, ArcanotronGUID))
                    {
                        uiDamage /= 4;
                        me->DealDamage(pArcanotron, uiDamage);
                        me->LowerPlayerDamageReq(uiDamage);
                    }

                    if (Creature* pMagmatron = me->GetCreature(*me,MagmatronGUID))
                    {
                        uiDamage /= 4;
                        me->DealDamage(pMagmatron, uiDamage);
                        me->LowerPlayerDamageReq(uiDamage);
                    }
                }
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            me->RemoveAura(SPELL_INACTIVE);
            DoCast(SPELL_ACTIVE);
            if (pInstance)
                pInstance->SetData(DATA_ELECTRON_GUID, IN_PROGRESS);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/)
        {
            DoScriptText(TEXT_DEATH,me);
            if (pInstance)
                pInstance->SetData(DATA_ELECTRON_GUID, DONE);
        }

        void KilledUnit(Unit * victim)
        {
        }
    };
};

class boss_Magmatron : public CreatureScript
{
public:
    boss_Magmatron () : CreatureScript("Boss_Magmatron") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_MagmatronAI (pCreature);
    }

    struct boss_MagmatronAI : public ScriptedAI
    {
        boss_MagmatronAI(Creature *c) : ScriptedAI(c)
        {
            pInstance = c->GetInstanceScript();
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
            me->setPowerType(POWER_ENERGY);
        }
        InstanceScript* pInstance;

        void Reset()
        {
            if (pInstance)
                pInstance->SetData(DATA_ARCANOTRON_GUID, NOT_STARTED);
            me->SetPower(POWER_ENERGY,100);
        }

        void DamageTaken(Unit* who, uint32& uiDamage)
        {
            std::list<Creature*> BossList;
            GetCreatureListWithEntryInGrid(BossList, me, NPC_ARCANOTRON, 100.0f);
            GetCreatureListWithEntryInGrid(BossList, me, NPC_ELECTRON, 100.0f);
            GetCreatureListWithEntryInGrid(BossList, me, NPC_TOXITRON, 100.0f);
            GetCreatureListWithEntryInGrid(BossList, me, NPC_MAGMATRON, 100.0f);
            if (!BossList.empty())
            {
                for (std::list<Creature*>::iterator itr = BossList.begin(); itr !=BossList.end(); ++itr)
                {
                    uint64 ElectronGUID = 0;
                    uint64 ToxitronGUID = 0;
                    uint64 ArcanotronGUID = 0;
                    uint64 MagmatronGUID = 0;

                    if(pInstance)
                    {
                        ElectronGUID = pInstance->GetData64(DATA_ELECTRON_GUID);
                        ToxitronGUID = pInstance->GetData64(DATA_TOXITRON_GUID);
                        ArcanotronGUID = pInstance->GetData64(DATA_ARCANOTRON_GUID);
                        MagmatronGUID = pInstance->GetData64(DATA_MAGMATRON_GUID);
                    }

                    if (!who || who->GetTypeId() != TYPEID_PLAYER)
                        return;

                    if (Creature* pToxitron = me->GetCreature(*me, ToxitronGUID))
                    {
                        uiDamage /= 4;
                        me->DealDamage(pToxitron, uiDamage);
                        me->LowerPlayerDamageReq(uiDamage);
                    }

                    if (Creature* pArcanotron = me->GetCreature(*me, ArcanotronGUID))
                    {
                        uiDamage /= 4;
                        me->DealDamage(pArcanotron, uiDamage);
                        me->LowerPlayerDamageReq(uiDamage);
                    }

                    if (Creature* pElectron= me->GetCreature(*me,ElectronGUID))
                    {
                        uiDamage /= 4;
                        me->DealDamage(pElectron, uiDamage);
                        me->LowerPlayerDamageReq(uiDamage);
                    }
                }
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            me->RemoveAura(SPELL_INACTIVE);
            DoCast(SPELL_ACTIVE);
            if (pInstance)
                pInstance->SetData(DATA_MAGMATRON_GUID, IN_PROGRESS);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/)
        {
            DoScriptText(TEXT_DEATH,me);
            if (pInstance)
                pInstance->SetData(DATA_MAGMATRON_GUID, DONE);
        }

        void KilledUnit(Unit * victim)
        {
        }
    };
};

void AddSC_boss_defensing_system()
{
//    new npc_omnotrion_triger();
    new boss_Toxitron();
    new boss_Arcanotrom();
    new boss_Electron();
    new boss_Magmatron();
}
