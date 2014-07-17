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
    SPELL_SHUTTING_DOWN             = 78746,
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
    SPELL_POISON_SOAKED_SHELL       = 79835,
    // MAGMATRON
    SPELL_INCINERATION              = 79023,
    SPELL_ACQUIRING_TARGET          = 79501,
    SPELL_BARRIER                   = 79582,
};

enum Misc
{
    // is that really related?
    MODEL_INACTIVE_GOLEM = 33492,

    // Visuals, needs to be replaced by correct ones with "beam" from the trigger
    // for now it's banish state spells
    SPELL_BANISH_STATE_BLUE   = 33344,
    SPELL_BANISH_STATE_RED    = 33343,
    SPELL_BANISH_STATE_GREEN  = 32567,
    SPELL_BANISH_STATE_PURPLE = 52241,
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

enum BossEncounters
{
    E_MIN        = 0,
    E_ARCANOTRON = 0,
    E_ELECTRON   = 1,
    E_TOXITRON   = 2,
    E_MAGMATRON  = 3,
    E_MAX        = 4
};

enum BossStates
{
    S_INACTIVE  = 0,
    S_PREPARING = 1,
    S_ACTIVE    = 2,
};

#define OmnoAI boss_omnotron_systemAI
#define OMNOTRONAI(c) ((OmnoAI*)c->AI())

struct boss_omnotron_systemAI: public ScriptedAI
{
    boss_omnotron_systemAI(Creature* c): ScriptedAI(c)
    {
        pInstance = c->GetInstanceScript();
        me->setPowerType(POWER_ENERGY);
        me->SetMaxPower(POWER_ENERGY, 100);
        me->SetPower(POWER_ENERGY, 100);
    }

    InstanceScript* pInstance;

    bool MeActive;
    bool LowEnergy;

    void Reset()
    {
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        me->SetPower(POWER_ENERGY, 100);

        me->CastSpell(me, SPELL_INACTIVE, true);

        MeActive = false;
        LowEnergy = false;
    }

    void SetInactiveInController()
    {
        Creature* pController = GetClosestCreatureWithEntry(me, 42186, 500.0f);
        if (!pController)
            return;

        switch (me->GetEntry())
        {
            case NPC_ARCANOTRON:
                pController->AI()->DoAction(100+E_ARCANOTRON);
                break;
            case NPC_TOXITRON:
                pController->AI()->DoAction(100+E_TOXITRON);
                break;
            case NPC_ELECTRON:
                pController->AI()->DoAction(100+E_ELECTRON);
                break;
            case NPC_MAGMATRON:
                pController->AI()->DoAction(100+E_MAGMATRON);
                break;
            default:
                break;
        }
    }

    void DamageTaken(Unit* who, uint32& uiDamage)
    {
        uint64 ElectronGUID = 0;
        uint64 ToxitronGUID = 0;
        uint64 ArcanotronGUID = 0;
        uint64 MagmatronGUID = 0;

        uiDamage = uiDamage / 4;

        if(pInstance)
        {
            ElectronGUID = pInstance->GetData64(DATA_ELECTRON_GUID);
            ToxitronGUID = pInstance->GetData64(DATA_TOXITRON_GUID);
            ArcanotronGUID = pInstance->GetData64(DATA_ARCANOTRON_GUID);
            MagmatronGUID = pInstance->GetData64(DATA_MAGMATRON_GUID);
        }

        if (!who || (who->GetTypeId() != TYPEID_PLAYER && !who->ToPet()))
            return;

        if (ElectronGUID != me->GetGUID())
        {
            if (Creature* pElectron = me->GetCreature(*me, ElectronGUID))
            {
                me->DealDamage(pElectron, uiDamage);
                pElectron->LowerPlayerDamageReq(uiDamage);
            }
        }

        if (ToxitronGUID != me->GetGUID())
        {
            if (Creature* pToxitron = me->GetCreature(*me, ToxitronGUID))
            {
                me->DealDamage(pToxitron, uiDamage);
                pToxitron->LowerPlayerDamageReq(uiDamage);
            }
        }

        if (ArcanotronGUID != me->GetGUID())
        {
            if (Creature* pArcanotron = me->GetCreature(*me, ArcanotronGUID))
            {
                me->DealDamage(pArcanotron, uiDamage);
                pArcanotron->LowerPlayerDamageReq(uiDamage);
            }
        }

        if (MagmatronGUID != me->GetGUID())
        {
            if (Creature* pMagmatron = me->GetCreature(*me,MagmatronGUID))
            {
                me->DealDamage(pMagmatron, uiDamage);
                pMagmatron->LowerPlayerDamageReq(uiDamage);
            }
        }
    }

    void ActivateSelf()
    {
        MeActive = true;
        me->RemoveAllAuras();
        me->CastSpell(me, SPELL_ACTIVE, false);
        me->SetReactState(REACT_AGGRESSIVE);
        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        me->ClearUnitState(UNIT_STATE_STUNNED | UNIT_STATE_ROOT);

        me->SetPower(POWER_ENERGY, 100);
    }

    void DeactivateSelf()
    {
        MeActive = false;
        SetInactiveInController();
        me->InterruptNonMeleeSpells(true);
        //me->CombatStop();
        me->SetReactState(REACT_PASSIVE);
        me->RemoveAurasDueToSpell(SPELL_ACTIVE);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        me->AddUnitState(UNIT_STATE_STUNNED | UNIT_STATE_ROOT);
        DoCast(me,SPELL_SHUTTING_DOWN);
    }

    void PrepareSelf()
    {
        // Funkce slouzici pro "pripravu" bossu, aneb kdo prijde na radu jako prvni
        switch (me->GetEntry())
        {
            case NPC_ARCANOTRON:
                DoCast(me, SPELL_BANISH_STATE_PURPLE, true);
                break;
            case NPC_TOXITRON:
                DoCast(me, SPELL_BANISH_STATE_GREEN, true);
                break;
            case NPC_ELECTRON:
                DoCast(me, SPELL_BANISH_STATE_BLUE, true);
                break;
            case NPC_MAGMATRON:
                DoCast(me, SPELL_BANISH_STATE_RED, true);
                break;
            default:
                break;
        }
    }

    void ShieldSelf()
    {
        switch (me->GetEntry())
        {
            case NPC_ARCANOTRON:
                DoCast(me, SPELL_POWER_CONVERSION, true);
                break;
            case NPC_TOXITRON:
                DoCast(me, SPELL_POISON_SOAKED_SHELL, true);
                break;
            case NPC_ELECTRON:
                DoCast(me, SPELL_UNSTABLE_SHIELD, true);
                break;
            case NPC_MAGMATRON:
                DoCast(me, SPELL_BARRIER, true);
                break;
            default:
                break;
        }
    }

    void UpdateEncounter()
    {
        if (me->GetPower(POWER_ENERGY) <= 50 && !LowEnergy)
        {
            LowEnergy = true;
            ShieldSelf();
        }
        if (me->GetPower(POWER_ENERGY) < 1 && MeActive)
        {
            DeactivateSelf();
        }
    }
};

class npc_omnotron_controller : public CreatureScript
{
public:
    npc_omnotron_controller() : CreatureScript("npc_omnotron_controller") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_omnotron_controllerAI (creature);
    }

    struct npc_omnotron_controllerAI : public Scripted_NoMovementAI
    {
        npc_omnotron_controllerAI(Creature *c) : Scripted_NoMovementAI(c)
        {
            pInstance = c->GetInstanceScript();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);

            eventPhase = 0;
            eventTimer = 0;

            Reset();
        }

        InstanceScript* pInstance;

        Creature* pBosses[E_MAX];
        uint8 bState[E_MAX];
        time_t uLastActive[E_MAX];

        uint8 eventPhase;
        uint32 eventTimer;

        bool overrideActivity;

        void Reset()
        {
            for (uint8 i = 0; i < E_MAX; i++)
            {
                bState[i] = S_INACTIVE;
                pBosses[i] = NULL;
                uLastActive[i] = 0;
            }
            eventPhase = 0;
            eventTimer = 0;

            GetBossPointers();
        }

        void DoAction(const int32 action)
        {
            if (action >= 100 && action <= 103)
            {
                sLog->outString("Setting %i inactive", action-100);
                bState[action-100] = S_INACTIVE;
            }
        }

        void GetBossPointers()
        {
            uint64 ArcanotronGUID = 0;
            uint64 ElectronGUID = 0;
            uint64 ToxitronGUID = 0;
            uint64 MagmatronGUID = 0;

            if(pInstance)
            {
                ArcanotronGUID = pInstance->GetData64(DATA_ARCANOTRON_GUID);
                ElectronGUID = pInstance->GetData64(DATA_ELECTRON_GUID);
                ToxitronGUID = pInstance->GetData64(DATA_TOXITRON_GUID);
                MagmatronGUID = pInstance->GetData64(DATA_MAGMATRON_GUID);
            }

            if (ArcanotronGUID)
                pBosses[E_ELECTRON] = me->GetCreature(*me, ArcanotronGUID);

            if (ElectronGUID)
                pBosses[E_ARCANOTRON] = me->GetCreature(*me, ElectronGUID);

            if (ToxitronGUID)
                pBosses[E_TOXITRON] = me->GetCreature(*me, ToxitronGUID);

            if (MagmatronGUID)
                pBosses[E_MAGMATRON] = me->GetCreature(*me, MagmatronGUID);
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (eventPhase != 0 || !me->canAttack(who) || !who->IsHostileTo(me) || who->GetDistance(me) > 10.0f || (who->ToPlayer() && who->ToPlayer()->isGameMaster()))
                return;

            AttackStart(who);

            overrideActivity = true;
            eventPhase = 1;
            eventTimer = 1000;
        }

        void ActivateBoss(uint32 pos)
        {
            sLog->outString("Activating: %u", pos);
            if (pos >= E_MAX)
                return;

            if (!pBosses[pos])
                return;

            OMNOTRONAI(pBosses[pos])->ActivateSelf();
            bState[pos] = S_ACTIVE;
            uLastActive[pos] = time(NULL);
        }

        void DeactivateBoss(uint32 pos)
        {
            if (pos >= E_MAX)
                return;

            if (!pBosses[pos])
                return;

            //OMNOTRONAI(pBosses[pos])->DeactivateSelf();
            bState[pos] = S_INACTIVE;
        }

        void PrepareBoss(uint32 pos)
        {
            sLog->outString("Preparing: %u", pos);
            if (pos >= E_MAX)
                return;

            if (!pBosses[pos])
                return;

            OMNOTRONAI(pBosses[pos])->PrepareSelf();
            bState[pos] = S_PREPARING;
        }

        BossEncounters GetRandomInactiveBoss()
        {
            std::vector<BossEncounters> available;
            for (uint8 i = 0; i < E_MAX; i++)
            {
                if (bState[i] == S_INACTIVE)
                    available.push_back(BossEncounters(i));
            }

            if (available.empty())
                return E_MAX;

            return available[urand(0,available.size()-1)];
        }

        BossEncounters GetOldestActive()
        {
            time_t theTime = time(NULL);
            BossEncounters whichOne = E_MAX;

            for (uint8 i = 0; i < E_MAX; i++)
            {
                if (uLastActive[i] < theTime && bState[i] == S_ACTIVE)
                {
                    theTime = uLastActive[i];
                    whichOne = BossEncounters(i);
                }
            }

            return whichOne;
        }

        BossEncounters GetPreparingBoss()
        {
            for (uint8 i = 0; i < E_MAX; i++)
            {
                if (bState[i] == S_PREPARING)
                    return BossEncounters(i);
            }

            return E_MAX;
        }

        void UpdateAI(const uint32 diff)
        {
            // Pokud bezi event a zadny z bossu neni v combatu, vypneme event
            if (eventPhase)
            {
                /*bool running = false;
                for (uint8 i = 0; i < E_MAX; i++)
                {
                    if (pBosses[i] && pBosses[i]->IsInCombat())
                        running = true;
                }

                if (!running && !overrideActivity)
                {
                    Reset();
                    return;
                }*/

                // Update eventu
                if (eventPhase > 0 && eventTimer > 0)
                {
                    if (eventTimer <= diff)
                    {
                        eventTimer = 0;
                        switch (eventPhase)
                        {
                            case 1:
                                ActivateBoss(urand(E_MIN,E_MAX-1));

                                eventPhase = 2;
                                eventTimer = 1000;
                                break;
                            case 2:
                                PrepareBoss(GetRandomInactiveBoss());

                                eventPhase = 3;
                                eventTimer = 30000;
                                break;
                            case 3:
                                ActivateBoss(GetPreparingBoss());
                                PrepareBoss(GetRandomInactiveBoss());
                                overrideActivity = false;

                                eventPhase = 4;
                                eventTimer = 30000;
                                break;
                            case 4:
                                //DeactivateBoss(GetOldestActive());
                                ActivateBoss(GetPreparingBoss());

                                eventPhase = 3;
                                eventTimer = 30000;
                                break;
                            default:
                                sLog->outError("npc_omnitron_controller: Nerozpoznana faze %u !", eventPhase);
                                eventPhase = 0;
                                break;
                        }
                    }
                    else
                        eventTimer -= diff;
                }
            }
        }
    };
};

class boss_Arcanotrom : public CreatureScript
{
public:
    boss_Arcanotrom() : CreatureScript("Boss_Arcanotron") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_ArcanotromAI (pCreature);
    }

    struct boss_ArcanotromAI : public boss_omnotron_systemAI
    {
        boss_ArcanotromAI(Creature *c) : boss_omnotron_systemAI(c)
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
            Reset();
        }

        uint32 Spawned;
        uint32 pElectron;
        uint32 pToxitron;
        uint32 uiArcaneAnnihilator;
        uint32 uiPowerGenerator;
        bool Shield;
        bool Recharge;

        void Reset()
        {
            boss_omnotron_systemAI::Reset();

            if (pInstance)
                pInstance->SetData(DATA_ELECTRON_GUID, NOT_STARTED);

            uiArcaneAnnihilator = 3000;
            uiPowerGenerator    = 15000;
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            UpdateEncounter();

            if (!MeActive)
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

    struct boss_ToxitronAI : public boss_omnotron_systemAI
    {
        boss_ToxitronAI(Creature *c) : boss_omnotron_systemAI(c)
        {
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
            Reset();
        }

        uint32 uiChemicalBomb;
        uint32 uiPoisonProtocol;
        uint32 uiPoisonSoakedShell;

        void Reset()
        {
            boss_omnotron_systemAI::Reset();

            if (pInstance)
                pInstance->SetData(DATA_TOXITRON_GUID, NOT_STARTED);

            uiChemicalBomb          = 4000;
            uiPoisonProtocol        = 40000;
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            UpdateEncounter();

            if (!MeActive)
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

    struct boss_ElectronAI : public boss_omnotron_systemAI
    {
        boss_ElectronAI(Creature *c) : boss_omnotron_systemAI(c)
        {
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
            Reset();
        }

        uint32 uiLightningConductor;
        uint32 uiUnstableShield;
        uint32 uiElectricalDischarge;

        void Reset()
        {
            boss_omnotron_systemAI::Reset();

            if (pInstance)
                pInstance->SetData(DATA_ARCANOTRON_GUID, NOT_STARTED);

            uiLightningConductor    = 3000;
            uiUnstableShield        = 30000;
            uiElectricalDischarge   = 35000;
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            UpdateEncounter();

            if (!MeActive)
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

    struct boss_MagmatronAI : public boss_omnotron_systemAI
    {
        boss_MagmatronAI(Creature *c) : boss_omnotron_systemAI(c)
        {
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
            Reset();
        }

        void Reset()
        {
            boss_omnotron_systemAI::Reset();

            if (pInstance)
                pInstance->SetData(DATA_ARCANOTRON_GUID, NOT_STARTED);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            UpdateEncounter();

            if (!MeActive)
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
    new npc_omnotron_controller();
    new boss_Toxitron();
    new boss_Arcanotrom();
    new boss_Electron();
    new boss_Magmatron();
}

/**** SQL:

UPDATE creature_template SET AIName='', ScriptName='Boss_Arcanotron' WHERE entry=42166;
UPDATE creature_template SET AIName='', ScriptName='Boss_Toxitron' WHERE entry=42180;
UPDATE creature_template SET AIName='', ScriptName='Boss_Electron' WHERE entry=42179;
UPDATE creature_template SET AIName='', ScriptName='Boss_Magmatron' WHERE entry=42178;

UPDATE creature_template SET AIName='', faction_A=14, faction_H=14, ScriptName='npc_omnotron_controller' WHERE entry=42186;

*/
