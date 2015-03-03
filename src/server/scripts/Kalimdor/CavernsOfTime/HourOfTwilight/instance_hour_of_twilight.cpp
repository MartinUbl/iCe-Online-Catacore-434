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
#include "hour_of_twilight.h"

const float ThrallMovePoints[6][4]=
{
    {4924.10f, 255.55f, 97.1204f, 2.132f}, // After two Frozen Servitors
    {4900.14f, 217.35f, 99.2130f, 4.122f}, // In the middle of the canyon

    {3024.83f, 518.80f, 22.2157f, 6.185f},
    {2989.04f, 506.10f, 26.4568f, 5.804f},
    {3006.73f, 486.72f, 25.2540f, 0.794f},
    {3005.81f, 539.14f, 27.4301f, 5.769f},
};

class instance_hour_of_twilight: public InstanceMapScript
{
public:
    instance_hour_of_twilight() : InstanceMapScript("instance_hour_of_twilight", 940) { }

    struct instance_hour_of_twilight_InstanceMapScript : public InstanceScript
    {
        instance_hour_of_twilight_InstanceMapScript(Map* pMap) : InstanceScript(pMap) {Initialize();}

        //InstanceScript* instance;
        uint32 m_auiEncounter[MAX_ENCOUNTER];
        uint32 currEnc[MAX_ENCOUNTER];

        uint64 arcurionGuid;
        uint64 thrallGuid;

        uint32 instance_progress;
        uint32 movement_progress;
        uint32 Move_Timer;

        std::string saveData;

        void Initialize()
        {
            instance_progress = 0;
            movement_progress = 0;

            Move_Timer = 0;

            memset(m_auiEncounter, 0, sizeof(uint32) * MAX_ENCOUNTER);
            GetCorrUiEncounter();
        }

        std::string GetSaveData()
        {
            OUT_SAVE_INST_DATA;

            std::ostringstream saveStream;
            saveStream << m_auiEncounter[0];
            for (uint8 i = 1; i < MAX_ENCOUNTER; i++)
                saveStream << " " << m_auiEncounter[i];

            saveStream << " " << instance_progress;
            saveStream << " " << movement_progress;

            OUT_SAVE_INST_DATA_COMPLETE;
            return saveStream.str();
        }

        void Load(const char* chrIn)
        {
            if (!chrIn)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(chrIn);

            std::istringstream loadStream(chrIn);
            for (uint8 i = 0; i < MAX_ENCOUNTER; i++)
                loadStream >> m_auiEncounter[i];

            loadStream >> instance_progress;
            loadStream >> movement_progress;

            for(uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                    m_auiEncounter[i] = NOT_STARTED;
            }

            GetCorrUiEncounter();
            OUT_LOAD_INST_DATA_COMPLETE;
        }

        void OnCreatureCreate(Creature* pCreature, bool add)
        {
            if (!add)
                return;

            switch (pCreature->GetEntry())
            {
                case 54590: // Arcurion
                    arcurionGuid = pCreature->GetGUID();
                    break;
                case 54548: // Thrall Entrance
                    thrallGuid = pCreature->GetGUID();
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go, bool add) { }

        uint64 GetData64(uint32 type)
        {
            switch (type)
            {
                case TYPE_BOSS_ARCURION:
                    return arcurionGuid;
                case TYPE_THRALL:
                    return thrallGuid;
            }
                return 0;
        }

        void SetData64(uint32 identifier, uint64 data) { }

        void Update(uint32 diff) 
        {
            if (!instance->HavePlayers())
                return;
        }

        uint32 GetData(uint32 DataId) 
        {
            if (DataId == DATA_INSTANCE_PROGRESS)
                return instance_progress;

            if (DataId == DATA_MOVEMENT_PROGRESS)
                return movement_progress;

            if (DataId < MAX_ENCOUNTER)
                return m_auiEncounter[DataId];

            return 0;
        }

        void SetData(uint32 type, uint32 data)
        {
            if (type < MAX_ENCOUNTER)
                m_auiEncounter[type] = data;

            if (type == DATA_INSTANCE_PROGRESS)
            {
                instance_progress += data;
                SaveToDB();
            }

            if (type == DATA_MOVEMENT_PROGRESS)
            {
                movement_progress += data;
                SaveToDB();
            }

            if (data == DONE)
            {
                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0];
                for (uint8 i = 1; i < MAX_ENCOUNTER; i++)
                    saveStream << " " << m_auiEncounter[i];

                GetCorrUiEncounter();
                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
            }
        }

        virtual uint32* GetCorrUiEncounter()
        {
            currEnc[0] = m_auiEncounter[TYPE_BOSS_ARCURION]; // Arcurion
            currEnc[1] = m_auiEncounter[TYPE_BOSS_ASIRA_DAWNSLAYER]; // Asira
            currEnc[2] = m_auiEncounter[TYPE_BOSS_ARCHBISHOP_BENEDICTUS]; // Benedictus

            sInstanceSaveMgr->setInstanceSaveData(instance->GetInstanceId(), currEnc, MAX_ENCOUNTER);

            return NULL;
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap* pMap) const
    {
        return new instance_hour_of_twilight_InstanceMapScript(pMap);
    }
};

////////////////////////////////
///// Other Instance Stuff /////
////////////////////////////////

// Thrall AIs
enum Thrall_NPC_IDs
{
    THRALL                = 54548,
    THRALL_1              = 55779,
    THRALL_2              = 54972,
    THRALL_3              = 54634,
    THRALL_4              = 54971,
};

// Thrall Spells
enum Thral_Spells
{
    THRALL_SPELL_LAVABURST         = 107980,
    THRALL_SPELL_HEALING_TOUCH     = 77067,
    THRALL_SPELL_GHOST_WOLF        = 2645,
    THRALL_SPELL_BLOODLUST         = 103834,
};

// List of gossip texts
#define GOSSIP_YES_THRALL     "Yes Thrall"

class npc_thrall_hour_of_twilight : public CreatureScript
{
public:
    npc_thrall_hour_of_twilight() : CreatureScript("npc_thrall_hour_of_twilight") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        if (pCreature->IsQuestGiver())
            pPlayer->PrepareQuestMenu(pCreature->GetGUID());

        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_YES_THRALL, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        pPlayer->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, pCreature->GetGUID());

        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF+1)
        {
            pCreature->AI()->DoAction();
            pPlayer->CLOSE_GOSSIP_MENU();
        }

        return true;
    }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_thrall_hour_of_twilightAI (pCreature);
    }

    struct npc_thrall_hour_of_twilightAI : public ScriptedAI
    {
        npc_thrall_hour_of_twilightAI(Creature *creature) : ScriptedAI(creature) 
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Arcurion_Yell_Timer;
        uint32 Lava_Burst_Timer;
        uint32 Thrall_Say_Timer;
        uint32 Move_Timer;
        bool Arcurion_Yell;
        bool Enemy_Spoted;

        void Reset() 
        {
            Arcurion_Yell = false;
            Enemy_Spoted = false;
            Thrall_Say_Timer = 0;
            Arcurion_Yell_Timer = 0;
            Lava_Burst_Timer = 0;
            Move_Timer = 30000;
        }

        void EnterCombat(Unit * /*who*/) { }

        void DoAction(const int32 /*param*/)
        {
            Arcurion_Yell_Timer = 10000;
            Arcurion_Yell = true;

            me->MonsterSay("Heroes, we have the Dragon Soul. The Aspects await us within Wyrmrest. Hurry - come with me!", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
            me->SendPlaySound(25870, true);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

            Thrall_Say_Timer = 25000;
        }

        void UpdateAI(const uint32 diff) 
        {
            if (Arcurion_Yell == true)
            {
                if (Arcurion_Yell_Timer <= diff)
                {
                    Creature * arcurion_dummy = me->FindNearestCreature(119508, 500.0, true);
                    if (arcurion_dummy)
                    {
                        // This should be only yell text not rly emote from some distance
                        //arcurion_dummy->MonsterYell("Shaman! The Dragon Soul is not yours. Give it up, and you may yet walk away with with your life", LANG_UNIVERSAL, 0);
                        me->SendPlaySound(25798, false);
                    }

                    // Set visible two nearby Frozen Servitors
                    std::list<Creature*> frozen_servitor;
                    GetCreatureListWithEntryInGrid(frozen_servitor, me, 54555, 50.0f);
                    for (std::list<Creature*>::const_iterator itr = frozen_servitor.begin(); itr != frozen_servitor.end(); ++itr)
                        if (*itr)
                        {
                            (*itr)->SetVisible(true);
                            (*itr)->setFaction(16);
                        }

                    Enemy_Spoted = true;
                    Arcurion_Yell = false;
                }
                else Arcurion_Yell_Timer -= diff;
            }

            if (Enemy_Spoted == true)
            {
                if (Thrall_Say_Timer <= diff)
                {
                    me->MonsterSay("How did they find us? Ready your weapons - we've got to get out of this canyon!", LANG_UNIVERSAL, me->GetGUID(), 100.0f);
                    me->SendPlaySound(25871, true);
                    me->GetMotionMaster()->MovePoint(0, 4926.0f, 289.0f, 96.75f);
                    Enemy_Spoted = false;
                }
                else Thrall_Say_Timer -= diff;
            }

            // Cast Lavaburst
            if (Lava_Burst_Timer <= diff)
            {
                Creature * frozen_servitor = me->FindNearestCreature(54555, 30.0, true);
                if (frozen_servitor)
                    me->CastSpell(frozen_servitor, THRALL_SPELL_LAVABURST, false);

                Lava_Burst_Timer = 3000;
            }
            else Lava_Burst_Timer -= diff;

            // Thrall movements update
            if (Move_Timer <= diff)
            {
                if (instance)
                {
                    if (instance->GetData(DATA_MOVEMENT_PROGRESS) == 2)
                    {
                        if (me->GetExactDist2d(ThrallMovePoints[0][0], ThrallMovePoints[0][1]) > 1)
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[0][0], ThrallMovePoints[0][1], ThrallMovePoints[0][2]);
                        else
                        {
                            instance->SetData(DATA_MOVEMENT_PROGRESS, 1);
                            Move_Timer = 500;
                            return;
                        }
                    }

                    if (instance->GetData(DATA_MOVEMENT_PROGRESS) == 3)
                    {
                        if (me->GetExactDist2d(ThrallMovePoints[1][0], ThrallMovePoints[1][1]) > 1)
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[1][0], ThrallMovePoints[1][1], ThrallMovePoints[1][2]);
                        else
                        {
                            instance->SetData(DATA_MOVEMENT_PROGRESS, 1);
                            Move_Timer = 1000;
                            return;
                        }
                    }

                    if (instance->GetData(DATA_MOVEMENT_PROGRESS) == 4)
                    {
                        if (me->GetExactDist2d(ThrallMovePoints[1][0], ThrallMovePoints[1][1]) > 1)
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[1][0], ThrallMovePoints[1][1], ThrallMovePoints[1][2]);
                        me->MonsterSay("What magic is this?", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
                        me->SendPlaySound(25872, true);
                        instance->SetData(DATA_MOVEMENT_PROGRESS, 1);
                        Move_Timer = 3000;
                        return;
                    }

                    if (instance->GetData(DATA_MOVEMENT_PROGRESS) == 5)
                    {
                        if (me->GetExactDist2d(ThrallMovePoints[1][0], ThrallMovePoints[1][1]) > 1)
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[1][0], ThrallMovePoints[1][1], ThrallMovePoints[1][2]);
                        me->MonsterSay("Look Out!", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
                        me->SendPlaySound(25873, true);
                        instance->SetData(DATA_MOVEMENT_PROGRESS, 1);
                    }

                    Move_Timer = 3000;
                }
            }
            else Move_Timer -= diff;
        }

    };
};

//////////////////////////////////////////////////////////////
////////////////        TRASH AI            //////////////////
//////////////////////////////////////////////////////////////

//////////////////////////
///// Arcurion Trash /////
//////////////////////////

enum Creatures
{
    FROZEN_SERVITOR           = 54555,
};
enum ArcurionTrashSpells
{
    ICY_BOULDER               = 105432,
    ICY_BOULDER_1             = 105433,
};

class npc_frozen_servitor : public CreatureScript
{
public:
    npc_frozen_servitor() : CreatureScript("npc_frozen_servitor") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_frozen_servitorAI (pCreature);
    }

    struct npc_frozen_servitorAI : public ScriptedAI
    {
        npc_frozen_servitorAI(Creature *creature) : ScriptedAI(creature) 
        {
            instance = creature->GetInstanceScript();

            me->SetVisible(false);
            me->setFaction(35);
        }

        InstanceScript* instance;

        void Reset() { }

        void JustDied(Unit* /*who*/)
        {
            if (InstanceScript *pInstance = me->GetInstanceScript())
                pInstance->SetData(DATA_MOVEMENT_PROGRESS, 1);
        }

        void EnterCombat()
        {
            
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_instance_hour_of_twilight()
{
    new instance_hour_of_twilight();

    new npc_thrall_hour_of_twilight();

    new npc_frozen_servitor();
}