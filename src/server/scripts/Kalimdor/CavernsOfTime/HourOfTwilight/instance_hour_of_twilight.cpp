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

const float ThrallMovePoints[37][4]=
{
    // Arcurion Thrall 0-4
    {4924.10f, 255.55f, 97.1204f, 2.132f}, // After two Frozen Servitors
    {4897.53f, 212.83f, 99.3030f, 4.155f}, // In the middle of the canyon
    {4867.65f, 162.53f, 98.0762f, 3.968f}, // Middle way in fron of Arcurion
    {4786.66f,  78.23f, 70.7800f, 3.847f}, // Stop before Arcurion
    {4762.56f,  61.55f, 66.3397f, 3.725f}, // Skywall
    // Ghost wolf Thrall 5-14
    {4684.05f,   6.57f, 65.5976f, 2.008f}, // Turn right point
    {4651.16f,  72.70f, 80.9788f, 1.419f}, // Another turn right point
    {4659.99f, 131.99f, 94.4264f, 1.973f}, // Turn again and run to the hill
    {4646.11f, 168.75f, 98.3476f, 2.072f}, // Hill peak
    {4614.72f, 238.88f, 94.9064f, 1.718f}, // Skeleton
    {4604.16f, 327.35f, 96.2448f, 2.354f}, // After Skeleton
    {4567.76f, 364.20f, 90.6261f, 2.927f}, // Turn Left
    {4536.76f, 371.36f, 81.8557f, 2.205f}, // Turn Right
    {4472.79f, 460.27f, 54.9014f, 3.116f}, // One last stop
    {4406.05f, 462.09f, 35.7199f, 5.370f}, // 3rd Thrall position
    // Asira Thrall 15-26
    {4410.55f, 451.16f, 35.3942f, 4.074f}, // Start of the road leading down
    {4389.73f, 423.24f, 16.3965f, 4.026f}, // Middle of the hill
    {4348.91f, 401.17f, -6.4098f, 3.226f}, // Bottom of the hill
    {4321.85f, 402.56f, -8.0832f, 3.545f}, // Middle of the 1st camp
    {4342.15f, 433.38f, -7.6375f, 1.541f}, // Turn left
    {4337.53f, 480.67f, -8.4055f, 2.256f}, // Stop before 2nd camp
    {4322.06f, 485.36f, -8.8445f, 3.151f}, // Closer to the 2nd camp
    {4323.48f, 532.85f, -8.6174f, 1.492f}, // Before hill
    {4290.04f, 568.05f, -7.0952f, 2.964f}, // Asira`s clearing
    {4283.30f, 590.06f, -6.4609f, 1.498f}, // Revive Life-Warden
    {4284.70f, 600.89f, -4.5608f, 1.266f}, // Jump on Life Warden
    {4260.03f, 445.29f, 43.1593f, 4.431f}, // Fly Away
    // Benedictus Thrall 27-36
    { 3916.99f, 275.17f, 8.1773f, 3.190f }, // 
    { 3895.41f, 277.70f, 2.3380f, 3.092f }, // First trash
    { 3837.12f, 280.85f, -20.5216f, 3.060f }, // Second trash
    { 3809.39f, 290.78f, -38.8207f, 3.010f },
    { 3762.31f, 289.02f, -64.3801f, 3.167f }, // Third trash
    { 3738.73f, 289.87f, -84.0984f, 3.159f },
    { 3668.17f, 284.15f, -119.399f, 3.170f },
    { 3595.60f, 277.90f, -119.968f, 3.232f },
    { 3582.70f, 277.09f, -114.031f, 3.274f },
    { 3562.59f, 274.80f, -115.964f, 3.265f }, // Benedictus
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
        uint64 thrall1Guid;
        uint64 thrall2Guid;
        uint64 thrall3Guid;
        uint64 thrall4Guid;

        uint32 instance_progress;
        uint32 movement_progress;
        uint32 asira_intro;
        uint32 drakes;

        std::string saveData;

        void Initialize()
        {
            instance_progress  = 0;
            movement_progress  = 0;
            asira_intro        = 0;
            drakes             = 0;

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
            saveStream << " " << asira_intro;
            saveStream << " " << drakes;

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
            loadStream >> asira_intro;
            loadStream >> drakes;

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
                case 55779: // Ghost wolf Thrall
                    thrall1Guid = pCreature->GetGUID();
                    break;
                case 54972: // Asira`s Thrall
                    thrall2Guid = pCreature->GetGUID();
                    break;
                case 54634: // Benedictus trash Thrall
                    thrall3Guid = pCreature->GetGUID();
                    break;
                case 54971: // Benedictus Thrall
                    thrall4Guid = pCreature->GetGUID();
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
                case TYPE_THRALL1:
                    return thrall1Guid;
                case TYPE_THRALL2:
                    return thrall2Guid;
                case TYPE_THRALL3:
                    return thrall3Guid;
                case TYPE_THRALL4:
                    return thrall4Guid;
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

            if (DataId == DATA_ASIRA_INTRO)
                return asira_intro;

            if (DataId == DATA_DRAKES)
                return drakes;

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

            if (type == DATA_ASIRA_INTRO)
            {
                asira_intro = data;
                SaveToDB();
            }

            if (type == DATA_DRAKES)
            {
                drakes = data;
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
    THRALL                       = 54548,
    THRALL_1                     = 55779,
    THRALL_2                     = 54972,
    THRALL_3                     = 54634,
    THRALL_4                     = 54971,
    RISING_FLAME_TOTEM           = 55474,
    LIFE_WARDEN                  = 55415,
    ASIRA_DOWNSLAYER             = 54968,
    NPC_FOG                      = 119513,
    BOSS_ARCHBISHOP_BENEDICTUS   = 54938,
    NPC_WATER_SHELL              = 55447,
};

// Thrall Spells
enum Thral_Spells
{
    THRALL_SPELL_LAVABURST         = 107980,
    THRALL_SPELL_HEALING_TOUCH     =  77067,
    THRALL_SPELL_BLOODLUST         = 103834,
    ARCURION_SPAWN_VISUAL          = 104767,
    GHOST_WOLF_FORM                =   2645,
    RISING_FLAME                   = 103813,
    ANCESTRAL_SPIRIT               = 103947,
    RESURRECTION                   =   2006,
    RED_DRAKE                      =  59570,
    THRALL_SPELL_LAVABURST_1       = 108442,
    WATER_SHELL                    = 103688,
};

// List of gossip texts
#define GOSSIP_YES_THRALL     "Yes Thrall"

// ENTRANCE THRALL - 54548
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
        uint32 Lookout_Timer;
        uint32 Move_Timer;
        uint32 Epilogue_Timer;
        uint32 Farewell_Timer;
        bool Arcurion_Yell;
        bool Enemy_Spoted;
        bool Say_Something;
        bool Epilogue;
        bool Farewell;
        bool Lookout;
        bool Go_Nowhere;

        void Reset() 
        {
            Arcurion_Yell = false;
            Enemy_Spoted = false;
            Say_Something = false;
            Epilogue = false;
            Farewell = false;
            Lookout = false;
            Go_Nowhere = false;
            Lookout_Timer = 0;
            Epilogue_Timer = 0;
            Thrall_Say_Timer = 0;
            Arcurion_Yell_Timer = 0;
            Lava_Burst_Timer = 0;
            Farewell_Timer = 0;
            Move_Timer = 30000;
        }

        void EnterCombat(Unit * /*who*/) { }

        void DoAction(const int32 /*param*/)
        {
            Arcurion_Yell_Timer = 8000;
            Arcurion_Yell = true;

            me->MonsterSay("Heroes, we have the Dragon Soul. The Aspects await us within Wyrmrest. Hurry - come with me!", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
            me->SendPlaySound(25870, true);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

            Thrall_Say_Timer = 20000;
        }

        void UpdateAI(const uint32 diff) 
        {
            if (Arcurion_Yell == true)
            {
                if (Arcurion_Yell_Timer <= diff)
                {
                    Creature * arcurion = me->FindNearestCreature(54590, 500.0f, true);
                    if (arcurion)
                        arcurion->MonsterYell("Shaman! The Dragon Soul is not yours. Give it up, and you may yet walk away with your life", LANG_UNIVERSAL, 0);
                        me->SendPlaySound(25798, false);

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

            if (Lookout)
            {
                if (Lookout_Timer <= diff)
                {
                    me->MonsterSay("Look Out!", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
                    me->SendPlaySound(25873, true);

                    Creature * arcurion = me->FindNearestCreature(54590, 400.0f, true);
                    if (arcurion)
                        arcurion->MonsterYell("Destroy them all, but bring the Shaman to me!", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(25799, true);

                    // Set visible three nearby Frozen Servitors
                    std::list<Creature*> frozen_servitor;
                    GetCreatureListWithEntryInGrid(frozen_servitor, me, 54555, 50.0f);
                    for (std::list<Creature*>::const_iterator itr = frozen_servitor.begin(); itr != frozen_servitor.end(); ++itr)
                        if (*itr)
                        {
                            (*itr)->SetVisible(true);
                            (*itr)->setFaction(16);
                        }

                    Lookout = false;
                }
                else Lookout_Timer -= diff;
            }

            if (Epilogue)
            {
                if (Epilogue_Timer <= diff)
                {
                    Epilogue = false;
                    me->MonsterSay("We've been discovered. I know you are tired but we cannot keep the aspects waiting!", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(25882, true);
                    me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[4][0], ThrallMovePoints[4][1], ThrallMovePoints[4][2]);
                }
                else Epilogue_Timer -= diff;
            }

            if (Enemy_Spoted == true)
            {
                if (Thrall_Say_Timer <= diff)
                {
                    me->MonsterSay("How did the Twilight Hammer find us? Ready your weapons - we've got to get out of this canyon!", LANG_UNIVERSAL, me->GetGUID(), 100.0f);
                    me->SendPlaySound(25871, true);
                    me->GetMotionMaster()->MovePoint(0, 4926.0f, 289.0f, 96.75f);
                    Enemy_Spoted = false;
                }
                else Thrall_Say_Timer -= diff;
            }

            // Cast Lavaburst during trash phase
            if (Lava_Burst_Timer <= diff)
            {
                Creature * frozen_servitor = me->FindNearestCreature(54555, 30.0, true);
                if (frozen_servitor && frozen_servitor->GetVisibility() == VISIBILITY_ON) // Cast only if theay are visible
                    me->CastSpell(frozen_servitor, THRALL_SPELL_LAVABURST, false);

                Lava_Burst_Timer = 3000;
            }
            else Lava_Burst_Timer -= diff;

            // Cast Lava Burst during Arcurion encounter
            if (instance->GetData(DATA_MOVEMENT_PROGRESS) >= 11)
            {
                // Cast Lavaburst
                if (Lava_Burst_Timer <= diff)
                {
                    Creature * frozen_servitor = me->FindNearestCreature(119509, 150.0, true);
                    if (frozen_servitor)
                        me->CastSpell(frozen_servitor, THRALL_SPELL_LAVABURST, false);

                    Lava_Burst_Timer = 3000;
                }
                else Lava_Burst_Timer -= diff;
            }

            // Thrall movements update
            if (Move_Timer <= diff)
            {
                if (instance)
                {
                    switch ((instance->GetData(DATA_MOVEMENT_PROGRESS)))
                    {
                    case 2:
                        me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[0][0], ThrallMovePoints[0][1], ThrallMovePoints[0][2]);
                        instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 3
                        break;
                    case 3:
                        if (me->GetExactDist2d(ThrallMovePoints[0][0], ThrallMovePoints[0][1]) < 1)
                        {
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[1][0], ThrallMovePoints[1][1], ThrallMovePoints[1][2]);
                            instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 4
                        }
                        break;
                    case 4:
                        if (me->GetExactDist2d(ThrallMovePoints[1][0], ThrallMovePoints[1][1]) < 1)
                        {
                            if (Say_Something == false)
                            {
                                me->MonsterSay("What magic is this?", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
                                me->SendPlaySound(25872, true);

                                Lookout = true;
                                Lookout_Timer = 3000;
                                Say_Something = true;
                            }
                        }
                        break;
                    case 7:
                        me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[2][0], ThrallMovePoints[2][1], ThrallMovePoints[2][2]);
                        instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 8
                        break;
                    case 8:
                        if (me->GetExactDist2d(ThrallMovePoints[2][0], ThrallMovePoints[2][1]) < 1)
                        {
                            if (!Go_Nowhere)
                            {
                                Creature * arcurion = me->FindNearestCreature(54590, 400.0f, true);
                                if (arcurion)
                                    arcurion->MonsterYell("You will go nowhere. Shaman.", LANG_UNIVERSAL, 0);
                                me->SendPlaySound(25800, true);

                                // Set visible three nearby Frozen Shards and Crystalline Elemental
                                std::list<Creature*> frozen_trash;
                                GetCreatureListWithEntryInGrid(frozen_trash, me, 55559, 200.0f);
                                for (std::list<Creature*>::const_iterator itr = frozen_trash.begin(); itr != frozen_trash.end(); ++itr)
                                    if (*itr)
                                    {
                                        (*itr)->SetVisible(true);
                                        (*itr)->setFaction(16);
                                    }

                                std::list<Creature*> frozen_boulders;
                                GetCreatureListWithEntryInGrid(frozen_boulders, me, 55563, 200.0f);
                                for (std::list<Creature*>::const_iterator itr = frozen_boulders.begin(); itr != frozen_boulders.end(); ++itr)
                                    if (*itr)
                                    {
                                        (*itr)->SetVisible(true);
                                        (*itr)->setFaction(16);
                                    }

                                Go_Nowhere = true;
                            }
                        }
                        break;
                    case 9:
                        me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[3][0], ThrallMovePoints[3][1], ThrallMovePoints[3][2]);
                        instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 10
                        break;
                    case 10:
                        if (me->GetExactDist2d(ThrallMovePoints[3][0], ThrallMovePoints[3][1]) < 1)
                        {
                            Creature * arcurion = me->FindNearestCreature(54590, 100.0f, true);
                            if (arcurion)
                            {
                                arcurion->SetVisible(true);
                                arcurion->CastSpell(arcurion, ARCURION_SPAWN_VISUAL, false);
                            }

                            me->MonsterSay("Show yourself", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
                            me->SendPlaySound(25877, true);

                            instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 11
                        }
                        break;
                    case 13:
                        // Say epilogue and move to Skywall
                        Epilogue_Timer = 5000;
                        Epilogue = true;
                        instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 14
                        break;
                    case 14:
                        if (me->GetExactDist2d(ThrallMovePoints[4][0], ThrallMovePoints[4][1]) < 1)
                        {
                            // Thrall destroys Skywall
                            Creature * Npc_Skywall = me->FindNearestCreature(119508, 100.0f, true);
                            if (Npc_Skywall)
                            {
                                me->CastSpell(Npc_Skywall, THRALL_SPELL_LAVABURST, false);
                                instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 15
                            }
                        }
                        break;
                    }
                    Move_Timer = 1000;
                }
            }
            else Move_Timer -= diff;
        }

    };
};

// GHOST WOLF THRALL - 55779
class npc_thrall_hour_of_twilight1 : public CreatureScript
{
public:
    npc_thrall_hour_of_twilight1() : CreatureScript("npc_thrall_hour_of_twilight1") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_thrall_hour_of_twilight1AI(pCreature);
    }

    struct npc_thrall_hour_of_twilight1AI : public ScriptedAI
    {
        npc_thrall_hour_of_twilight1AI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            
            me->SetVisible(false);
        }

        InstanceScript* instance;
        uint32 Move_Timer;
        uint32 Ghost_Wolf_Timer;
        int Instance_Progress;
        bool Timer_Set;
        bool Ghost_Wolf_Form;

        void Reset() 
        {
            Timer_Set = false;
            Ghost_Wolf_Form = false;
            Ghost_Wolf_Timer = 0;
            Move_Timer = 0;
        }

        void EnterCombat(Unit * /*who*/) { }

        void UpdateAI(const uint32 diff)
        {
            if (!Timer_Set)
            {
                // If visible, set 2s timer for start casting Wolf form
                if (me->GetVisibility() == VISIBILITY_ON)
                {
                    Ghost_Wolf_Timer = 2000;
                    Timer_Set = true;
                }
            }
            else
            {
                if (!Ghost_Wolf_Form)
                {
                    if (Ghost_Wolf_Timer <= diff)
                    {
                        me->CastSpell(me, GHOST_WOLF_FORM, false);
                        me->MonsterSay("Follow me!", LANG_UNIVERSAL, 0);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                        Ghost_Wolf_Form = true;
                    }
                    else Ghost_Wolf_Timer -= diff;
                }
            }

            if (me->HasAura(GHOST_WOLF_FORM))
            {
                // Ghost wolf Thrall movements update
                if (Move_Timer <= diff)
                {
                    if (instance)
                    {
                        switch ((instance->GetData(DATA_MOVEMENT_PROGRESS)))
                        {
                        case 15:
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[5][0], ThrallMovePoints[5][1], ThrallMovePoints[5][2]);
                            instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 16
                            break;
                        case 16:
                        case 17:
                        case 18:
                        case 19:
                        case 20:
                        case 21:
                        case 22:
                        case 23:
                        case 24:
                            if (me->GetExactDist2d(ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))-11][0], ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))-11][1]) < 1)
                            {
                                me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))-10][0], ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))-10][1], ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))-10][2]);
                                instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 16-25
                            }
                            break;
                        case 25:
                            if (me->GetExactDist2d(ThrallMovePoints[14][0], ThrallMovePoints[14][1]) < 1)
                            {
                                me->SetVisible(false);
                                Creature * thrall_next = me->FindNearestCreature(THRALL_2, 50.0f, true);
                                if (thrall_next)
                                    thrall_next->SetVisible(true);
                                instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 26
                            }
                            break;
                        default:
                            break;
                        }

                        Move_Timer = 1000;
                    }
                }
                else Move_Timer -= diff;
            }
        }

    };
};

// ASIRA THRALL - 54972
class npc_thrall_hour_of_twilight2 : public CreatureScript
{
public:
    npc_thrall_hour_of_twilight2() : CreatureScript("npc_thrall_hour_of_twilight2") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_thrall_hour_of_twilight2AI(pCreature);
    }

    struct npc_thrall_hour_of_twilight2AI : public ScriptedAI
    {
        npc_thrall_hour_of_twilight2AI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();

            me->SetVisible(false);
            me->SetStandState(UNIT_STAND_STATE_KNEEL);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
        }

        InstanceScript* instance;
        uint32 Move_Timer;
        uint32 Lavaburst_Timer;
        uint32 Say_Next_Timer;
        uint32 Summon_Totem_Timer;
        uint32 Outro_Timer;
        uint32 Check_Timer;
        int Twilight;
        int Outro_Action;
        bool Set_Timer;
        bool Lavaburst;
        bool Say_Next;
        bool Outro;

        void Reset() 
        {
            Twilight = 0;
            Set_Timer = false;
            Lavaburst = false;
            Say_Next = false;
            Outro = false;
            Summon_Totem_Timer = 0;
            Lavaburst_Timer = 0;
            Outro_Action = 0;
            Outro_Timer = 0;
            Check_Timer = 0;
        }

        void FindCorrectTarget()
        {
            Creature * twilight_assassin = me->FindNearestCreature(55106, 30.0f, true);       // TWILIGHT_ASSASSIN
            Creature * twilight_shadow_walker = me->FindNearestCreature(55109, 30.0f, true);  // TWILIGHT_SHADOW_WALKER
            Creature * twilight_bruiser = me->FindNearestCreature(55112, 30.0f, true);        // TWILIGHT_BRUISER
            Creature * twilight_thug = me->FindNearestCreature(55111, 30.0f, true);           // TWILIGHT_THUG
            Creature * twilight_ranger = me->FindNearestCreature(55107, 30.0f, true);         // TWILIGHT_RANGER

            std::vector<int> targets;
            targets.clear();

            if (twilight_assassin && twilight_assassin->IsInCombat())
                targets.push_back(0);
            if (twilight_shadow_walker && twilight_shadow_walker->IsInCombat())
                targets.push_back(1);
            if (twilight_bruiser && twilight_bruiser->IsInCombat())
                targets.push_back(2);
            if (twilight_thug && twilight_thug->IsInCombat())
                targets.push_back(3);
            if (twilight_ranger && twilight_ranger->IsInCombat())
                targets.push_back(4);

            if (!targets.empty())
            {
                Twilight = targets[urand(0, targets.size() - 1)];

                switch (Twilight)
                {
                case 0:
                    if (twilight_assassin)
                        me->CastSpell(twilight_assassin, THRALL_SPELL_LAVABURST, false);
                    break;
                case 1:
                    if (twilight_shadow_walker)
                        me->CastSpell(twilight_shadow_walker, THRALL_SPELL_LAVABURST, false);
                    break;
                case 2:
                    if (twilight_bruiser)
                        me->CastSpell(twilight_bruiser, THRALL_SPELL_LAVABURST, false);
                    break;
                case 3:
                    if (twilight_thug)
                        me->CastSpell(twilight_thug, THRALL_SPELL_LAVABURST, false);
                    break;
                case 4:
                    if (twilight_ranger)
                        me->CastSpell(twilight_ranger, THRALL_SPELL_LAVABURST, false);
                    break;
                default:
                    break;
                }
            }

            Lavaburst_Timer = 3000;
        }

        void EnterCombat(Unit * /*who*/) { }

        void UpdateAI(const uint32 diff)
        {
            // Change this later, Move_Timer won`t work after server crash
            if ((instance->GetData(DATA_MOVEMENT_PROGRESS)) == 1)
                if (Set_Timer == false)
                {
                    Move_Timer = 3000;
                    Set_Timer = true;
                }

            // Trash actions
            if (Lavaburst)
            {
                if (Lavaburst_Timer <= diff)
                {
                    FindCorrectTarget();
                }
                else Lavaburst_Timer -= diff;

                if (Summon_Totem_Timer <= diff)
                {
                    int distance = 7;
                    float angle = me->GetOrientation();
                    me->SummonCreature(RISING_FLAME_TOTEM, me->GetPositionX() + cos(angle)*distance, me->GetPositionY() + sin(angle)*distance, me->GetPositionZ()+1, angle, TEMPSUMMON_TIMED_DESPAWN, 30000);
                    Summon_Totem_Timer = 35000+urand(0, 15000);
                }
                else Summon_Totem_Timer -= diff;
            }

            // Boss Asira Actions
            if (Check_Timer <= diff)
            {
                if (instance)
                {
                    if ((instance->GetData(TYPE_BOSS_ASIRA_DAWNSLAYER)) == IN_PROGRESS)
                    {
                        // Cast Lavaburst
                        if (Lavaburst_Timer <= diff)
                        {
                            Creature * asira = me->FindNearestCreature(ASIRA_DOWNSLAYER, 40.0f, true);
                            if (asira)
                                me->CastSpell(asira, THRALL_SPELL_LAVABURST, false);
                            Lavaburst_Timer = 5000;
                        }
                        else Lavaburst_Timer -= diff;

                        // Summon Totem
                        if (Summon_Totem_Timer <= diff)
                        {
                            int distance = urand(10,40);
                            float angle = urand(22, 39) / 10;
                            me->SummonCreature(RISING_FLAME_TOTEM, me->GetPositionX() + cos(angle)*distance, me->GetPositionY() + sin(angle)*distance, me->GetPositionZ()+1, angle, TEMPSUMMON_TIMED_DESPAWN, 30000);
                            Summon_Totem_Timer = 15000+urand(0, 15000);
                        }
                        else Summon_Totem_Timer -= diff;
                    }
                }
            }

            if (Say_Next)
            {
                if (Say_Next_Timer <= diff)
                {
                    me->GetMotionMaster()->MovePoint(0, 4287.48f, 565.78f, -7.1822f);
                    me->MonsterSay("Up there, above us!", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(25888, true);
                    Say_Next = false;
                }
                else Say_Next_Timer -= diff;
            }

            // Thrall movement update
            if (Move_Timer <= diff)
            {
                if (instance)
                {
                    switch ((instance->GetData(DATA_MOVEMENT_PROGRESS)))
                    {
                    case 1: // Start from 26 later
                        me->RemoveStandFlags(UNIT_STAND_STATE_KNEEL);
                        me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[15][0], ThrallMovePoints[15][1], ThrallMovePoints[15][2]);
                        instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 27 (2)
                        break;
                    case 2:
                        if (me->GetExactDist2d(ThrallMovePoints[15][0], ThrallMovePoints[15][1]) < 1)
                        {
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[16][0], ThrallMovePoints[16][1], ThrallMovePoints[16][2]);
                            instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 28 (3)
                            Summon_Totem_Timer = 5000;
                            Lavaburst = true;
                        }
                        break;
                    case 5/*28*/: // After group kills 2 Assassins 28 na 16
                    case 6/*29*/:
                        //if (me->GetExactDist2d(ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))-12][0], ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))-12][1]) < 1)
                        if (me->GetExactDist2d(ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))+11][0], ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))+11][1]) < 1)
                        {
                            // me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))-11][0], ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))-11][1], ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))-11][2]);
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))+12][0], ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))+12][1], ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))+12][2]);
                            instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 30 (7)
                        }
                        Lavaburst = false;
                        break;
                    case 7/*30*/:
                        if (me->GetExactDist2d(ThrallMovePoints[18][0], ThrallMovePoints[18][1]) < 1)
                        {
                            me->MonsterYell("Be ware, enemies approach!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25883, true);
                            instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 31 (8) + 5 Dead enemies => 36 (13)
                            Summon_Totem_Timer = 10000 + urand(0, 10000);
                        }
                        Lavaburst = true;
                        break;
                    case 13/*36*/:
                    case 14/*37*/:
                    case 15/*38*/:
                        //if (me->GetExactDist2d(ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))-18][0], ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))-18][1]) < 1)
                        if (me->GetExactDist2d(ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))+5][0], ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))+5][1]) < 1)
                        {
                            // me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))-17][0], ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))-17][1], ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))-17][2]);
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))+6][0], ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))+6][1], ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS))+6][2]);
                            instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 13->[19], 14->[20], 15->[21]
                        }
                        Lavaburst = false;
                        break;
                    case 16/*39*/:
                        if (me->GetExactDist2d(ThrallMovePoints[21][0], ThrallMovePoints[21][1]) < 1)
                        {
                            me->MonsterYell("Let none stand in our way!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25884, true);
                            instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 40 (17) + 5 Dead enemies => 45 (22)
                            Summon_Totem_Timer = 10000 + urand(0, 10000);
                        }
                        Lavaburst = true;
                        break;
                    case 22/*45*/:
                        if (me->GetExactDist2d(ThrallMovePoints[21][0], ThrallMovePoints[21][1]) < 1)
                        {
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[22][0], ThrallMovePoints[22][1], ThrallMovePoints[22][2]);
                            instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 46 (23)
                        }
                        Lavaburst = false;
                        break;
                    case 23/*46*/:
                        if (me->GetExactDist2d(ThrallMovePoints[22][0], ThrallMovePoints[22][1]) < 1)
                        {
                            me->MonsterYell("Twilight`s hammer returns!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25885, true);
                            instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 47 (24) + 5 => 52 (29)
                            Summon_Totem_Timer = 10000 + urand(0, 10000);
                        }
                        Lavaburst = true;
                        break;
                    case 29/*52*/:
                        if (me->GetExactDist2d(ThrallMovePoints[22][0], ThrallMovePoints[22][1]) < 1)
                        {
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[23][0], ThrallMovePoints[23][1], ThrallMovePoints[23][2]);
                            instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 53 (30)
                        }
                        Lavaburst = false;
                        break;
                    case 30/*53*/:
                        if (me->GetExactDist2d(ThrallMovePoints[23][0], ThrallMovePoints[23][1]) < 1)
                        {
                            me->MonsterYell("Let them come", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25886, true);
                            instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 54 (31) + 5 => 59 (36)
                            Summon_Totem_Timer = 10000 + urand(0, 10000);
                        }
                        Lavaburst = true;
                        break;
                    case 36/*59*/:
                        me->MonsterSay("Alexstrasza's drakes should meet us here...", LANG_UNIVERSAL, 0);
                        me->SendPlaySound(25887, true);
                        me->SetOrientation(3.089f);
                        Lavaburst = false;
                        Say_Next_Timer = 5000;
                        Say_Next = true;
                        instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 55

                    default:
                        break;
                    }

                    Move_Timer = 1000;
                }
            }
            else Move_Timer -= diff;

            // Thrall Asira Outro
            if (instance)
            {
                if ((instance->GetData(DATA_DRAKES)) == 1)
                {
                    if (!Outro)
                    {
                        if (Outro_Timer <= diff)
                        {
                            switch (Outro_Action) {
                            case 0: // Walk to Life Warden
                                me->SetWalk(true);
                                me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[24][0], ThrallMovePoints[24][1], ThrallMovePoints[24][2]);
                                Outro_Timer = 4000;
                                Outro_Action++;
                                break;
                            case 1: // Say
                                me->MonsterSay("Well done. Let us see to our friend...", LANG_UNIVERSAL, 0);
                                me->SendPlaySound(25891, true);
                                Outro_Action++;
                                Outro_Timer = 10000;
                                break;
                            case 2: // Cast Ancestral Spirit
                            {
                                Creature * life_warden = me->FindNearestCreature(LIFE_WARDEN, 50.0f, false);
                                if (life_warden)
                                {
                                    me->CastSpell(life_warden, ANCESTRAL_SPIRIT, false);
                                    me->MonsterSay("Nicco je Noob :D proto to nejde", LANG_UNIVERSAL, 0);
                                }
                                Outro_Timer = 3800;
                                Outro_Action++;
                                break;
                            }
                            case 3: // Revive Life Warden
                            {
                                Creature * life_warden = me->FindNearestCreature(LIFE_WARDEN, 50.0f, false);
                                if (life_warden)
                                {
                                    life_warden->setDeathState(JUST_ALIVED);
                                    life_warden->CastSpell(life_warden, RESURRECTION, true);
                                }
                                Outro_Timer = 1000;
                                Outro_Action++;
                                break;
                            }
                            case 4: // Jump on Life Warden
                            {
                                Creature * life_warden = me->FindNearestCreature(LIFE_WARDEN, 50.0f, true);
                                if (life_warden)
                                    me->GetMotionMaster()->MovePoint(0, life_warden->GetPositionX(), life_warden->GetPositionY(), life_warden->GetPositionZ());
                                Outro_Timer = 3000;
                                Outro_Action++;
                                break;
                            }
                            case 5: // Despawn Life Warden - Mount Thrall
                            {
                                Creature * life_warden = me->FindNearestCreature(LIFE_WARDEN, 50.0f, true);
                                if (life_warden)
                                {
                                    life_warden->SetVisible(false);
                                    life_warden->DespawnOrUnsummon();
                                    me->CastSpell(me, RED_DRAKE, true);
                                    me->MonsterSay("The rest of the drakes should be here shortly. I'll fly on ahead, catch up when you can.", LANG_UNIVERSAL, 0);
                                    me->SendPlaySound(25892, true);
                                    Outro_Timer = 1000;
                                    Outro_Action++;
                                }
                                break;
                            }
                            case 6: // Fly Away
                                me->SetFlying(true);
                                me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[26][0], ThrallMovePoints[26][1], ThrallMovePoints[26][2]);
                                Outro_Action++;
                                Outro_Timer = 15000;
                                break;
                            case 7: // Disappear
                                me->DespawnOrUnsummon();
                                Outro = true;
                                break;
                            default:
                                break;
                            }
                        }
                        else Outro_Timer -= diff;
                    }
                }
            }
        }

    };
};

// List of gossip texts
#define GOSSIP_YES_LETS_DO_THIS_THRALL     "Yes Thrall, let`s do this!"

// BENEDICTUS TRASH THRALL - 54634
class npc_thrall_hour_of_twilight3 : public CreatureScript
{
public:
    npc_thrall_hour_of_twilight3() : CreatureScript("npc_thrall_hour_of_twilight3") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        if (pCreature->IsQuestGiver())
            pPlayer->PrepareQuestMenu(pCreature->GetGUID());

        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_YES_LETS_DO_THIS_THRALL, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        pPlayer->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, pCreature->GetGUID());

        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
        {
            pCreature->AI()->DoAction();
            pPlayer->CLOSE_GOSSIP_MENU();
        }

        return true;
    }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_thrall_hour_of_twilight3AI(pCreature);
    }

    struct npc_thrall_hour_of_twilight3AI : public ScriptedAI
    {
        npc_thrall_hour_of_twilight3AI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Move_Timer;
        bool Gossip;
        bool Show_Trash;

        void Reset()
        {
            Gossip = false;
            Show_Trash = false;
            Move_Timer = 0;
        }

        void DoAction(const int32 /*param*/)
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            Gossip = true;
            Move_Timer = 1000;

            if (InstanceScript *pInstance = me->GetInstanceScript())
                pInstance->SetData(DATA_MOVEMENT_PROGRESS, 1);

            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[27][0], ThrallMovePoints[27][1], ThrallMovePoints[27][2], true, true);
        }

        void ShowTrash()
        {
            Creature * fog = me->FindNearestCreature(NPC_FOG, 50.0f, true);
            if (fog)
                fog->Kill(fog);
            me->MonsterTextEmote("Spawn of the Old Gods materialize nearby!", LANG_UNIVERSAL, true, 150.0f);
            Show_Trash = true;
        }

        void EnterCombat(Unit * /*who*/) { }

        void UpdateAI(const uint32 diff)
        {
            if (Gossip == true)
            {
                // Thrall movement update
                if (Move_Timer <= diff)
                {
                    if (instance)
                    {
                        switch ((instance->GetData(DATA_MOVEMENT_PROGRESS)))
                        {
                        case 1:
                            if (me->GetExactDist2d(ThrallMovePoints[27][0], ThrallMovePoints[27][1]) < 1)
                            {
                                me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[28][0], ThrallMovePoints[28][1], ThrallMovePoints[28][2], true);
                                instance->SetData(DATA_MOVEMENT_PROGRESS, 1);
                            }
                            break;
                        case 2: //
                            if (me->GetExactDist2d(ThrallMovePoints[28][0], ThrallMovePoints[28][1]) < 1)
                                if (Show_Trash == false)
                                    ShowTrash();
                            break;
                        case 3:
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[29][0], ThrallMovePoints[29][1], ThrallMovePoints[29][2], true);
                            Show_Trash = false;
                            instance->SetData(DATA_MOVEMENT_PROGRESS, 1);
                            break;
                        case 4: // 
                            if (me->GetExactDist2d(ThrallMovePoints[29][0], ThrallMovePoints[29][1]) < 1)
                                if (Show_Trash == false)
                                    ShowTrash();
                            break;
                        case 5:
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[30][0], ThrallMovePoints[30][1], ThrallMovePoints[30][2], true);
                            Show_Trash = false;
                            instance->SetData(DATA_MOVEMENT_PROGRESS, 1);
                            break;
                        case 6:
                            if (me->GetExactDist2d(ThrallMovePoints[30][0], ThrallMovePoints[30][1]) < 1)
                            {
                                me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[31][0], ThrallMovePoints[31][1], ThrallMovePoints[31][2], true);
                                instance->SetData(DATA_MOVEMENT_PROGRESS, 1);
                            }
                            break;
                        case 7:
                            if (me->GetExactDist2d(ThrallMovePoints[31][0], ThrallMovePoints[31][1]) < 1)
                                if (Show_Trash == false)
                                    ShowTrash();
                            break;
                        case 8:
                        case 9:
                        case 10:
                        case 11:
                            if (me->GetExactDist2d(ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS)) + 23][0], ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS)) + 23][1]) < 1)
                            {
                                me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS)) + 24][0], ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS)) + 24][1], ThrallMovePoints[(instance->GetData(DATA_MOVEMENT_PROGRESS)) + 24][2]);
                                instance->SetData(DATA_MOVEMENT_PROGRESS, 1);
                            }
                            break;
                        case 12:
                            if (me->GetExactDist2d(ThrallMovePoints[35][0], ThrallMovePoints[35][1]) < 1)
                            {
                                me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[36][0], ThrallMovePoints[36][1], ThrallMovePoints[36][2]);
                                instance->SetData(DATA_MOVEMENT_PROGRESS, 1);
                            }
                            break;
                        case 13:
                            if (me->GetExactDist2d(ThrallMovePoints[36][0], ThrallMovePoints[36][1]) < 1)
                            {
                                Creature * last_thrall = me->FindNearestCreature(THRALL_4, 10.0f, true);
                                if (last_thrall)
                                {
                                    last_thrall->SetVisible(true);
                                }
                                me->ForcedDespawn(0);
                            }
                            break;
                        default:
                            break;
                        }
                    }
                    Move_Timer = 1000;
                }
                else Move_Timer -= diff;
            }
        }

    };
};

// BENEDICTUS BOSS THRALL - 54971
class npc_thrall_hour_of_twilight4 : public CreatureScript
{
public:
    npc_thrall_hour_of_twilight4() : CreatureScript("npc_thrall_hour_of_twilight4") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_thrall_hour_of_twilight4AI(pCreature);
    }

    struct npc_thrall_hour_of_twilight4AI : public ScriptedAI
    {
        npc_thrall_hour_of_twilight4AI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Check_Timer;
        uint32 Dmg_Timer;
        uint32 Water_Shell_Timer;
        bool Damage;

        void Reset()
        {
            me->SetVisible(false);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            Check_Timer = 1000;
            Dmg_Timer = 1000;
            Water_Shell_Timer = 60000;
            Damage = false;
        }

        void EnterCombat(Unit * /*who*/) { }

        void DoAction(const int32 /*param*/)
        {
            Dmg_Timer = 5000;
            Water_Shell_Timer = 6000;
        }

        void UpdateAI(const uint32 diff)
        {
            if (Check_Timer <= diff)
            {
                if (instance && Damage == false)
                    if (instance->GetData(TYPE_BOSS_ARCHBISHOP_BENEDICTUS) == IN_PROGRESS)
                        Damage = true;

                if (instance && Damage == true)
                    if (instance->GetData(TYPE_BOSS_ARCHBISHOP_BENEDICTUS) == NOT_STARTED ||
                        instance->GetData(TYPE_BOSS_ARCHBISHOP_BENEDICTUS) == DONE)
                        Damage = false;

                Check_Timer = 5000;
            }
            else Check_Timer -= diff;

            if (Damage == true && !me->HasAura(103762)) // Engulfing Twilight aura
            {
                if (Dmg_Timer <= diff)
                {
                    Creature * archbishop = me->FindNearestCreature(BOSS_ARCHBISHOP_BENEDICTUS, 100.0f, true);
                    if (archbishop)
                        me->CastSpell(archbishop, THRALL_SPELL_LAVABURST_1, false);
                    Dmg_Timer = 3000;
                }
                else Dmg_Timer -= diff;

                if (Water_Shell_Timer <= diff)
                {
                    int distance = urand(10, 27);
                    float angle = urand(22, 42) / 10; // 3.22
                    me->SummonCreature(NPC_WATER_SHELL, me->GetPositionX() + cos(angle)*distance, me->GetPositionY() + sin(angle)*distance, me->GetPositionZ() + 1.5, angle, TEMPSUMMON_TIMED_DESPAWN, 30000);
                    me->InterruptNonMeleeSpells(false);
                    Creature * water_shell = me->FindNearestCreature(NPC_WATER_SHELL, 100.0f, true);
                    if (water_shell)
                        me->CastSpell(water_shell, WATER_SHELL, false);
                    Water_Shell_Timer = 60000;
                    Dmg_Timer = 15000;
                }
                else Water_Shell_Timer -= diff;
            }
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
    CRYSTALLINE_ELEMENTAL     = 55559,
    FROZEN_SHARD              = 55563,
};
enum ArcurionTrashSpells
{
    ICY_BOULDER               = 105432,
    ICY_BOULDER_1             = 105433,
    FROZEN_SERVITOR_VISUAL    = 103595,
    IMPALE                    = 104019,
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
        uint32 Icy_Boulder_Timer;
        uint32 Size_Timer;
        float Size;
        bool Visual_Spell;
        bool Size_Change;

        void Reset() 
        {
            Visual_Spell = false;
            Size_Change = false;
            Icy_Boulder_Timer = urand(0,2000);
            Size_Timer = 0;
        }

        void JustDied(Unit* /*who*/)
        {
            if (InstanceScript *pInstance = me->GetInstanceScript())
                pInstance->SetData(DATA_MOVEMENT_PROGRESS, 1);
        }

        void EnterCombat() { }

        void UpdateAI(const uint32 diff)
        {
            if (me->GetVisibility() == VISIBILITY_ON)
            {
                if (Visual_Spell == false)
                {
                    me->CastSpell(me, FROZEN_SERVITOR_VISUAL, false);

                    Size_Timer = 1000;
                    Visual_Spell = true;
                }
            }

            if (Visual_Spell == true)
            {
                if (Size_Change == false)
                {
                    if (Size_Timer <= diff)
                    {
                        Size = 1.00;
                        me->SetFloatValue(OBJECT_FIELD_SCALE_X, Size);

                        Size_Change = true;
                    }
                    else Size_Timer -= diff;
                }
            }

            if (!UpdateVictim())
                return;

            if (Icy_Boulder_Timer <= diff)
            {
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true);
                if (target)
                {
                    me->CastSpell(target, ICY_BOULDER_1, false);
                    Icy_Boulder_Timer = 4000+urand(0,3000);
                }
            }
            else Icy_Boulder_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_crystalline_elemental : public CreatureScript
{
public:
    npc_crystalline_elemental() : CreatureScript("npc_crystalline_elemental") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_crystalline_elementalAI(pCreature);
    }

    struct npc_crystalline_elementalAI : public ScriptedAI
    {
        npc_crystalline_elementalAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();

            me->SetVisible(false);
            me->setFaction(35);
        }

        InstanceScript* instance;
        uint32 Size_Timer;
        uint32 Impale_Timer;
        float Size;
        bool Visual_Spell;
        bool Size_Change;

        void Reset()
        {
            Visual_Spell = false;
            Size_Change = false;
            Size_Timer = 0;
        }

        void JustDied(Unit* /*who*/)
        {
            if (InstanceScript *pInstance = me->GetInstanceScript())
                pInstance->SetData(DATA_MOVEMENT_PROGRESS, 1);
        }

        void EnterCombat() { }

        void UpdateAI(const uint32 diff)
        {
            if (me->GetVisibility() == VISIBILITY_ON)
            {
                if (Visual_Spell == false)
                {
                    me->CastSpell(me, FROZEN_SERVITOR_VISUAL, false);

                    Size_Timer = 1000;
                    Visual_Spell = true;
                }
            }

            if (Visual_Spell == true)
            {
                if (Size_Change == false)
                {
                    if (Size_Timer <= diff)
                    {
                        Size = 1.00;
                        me->SetFloatValue(OBJECT_FIELD_SCALE_X, Size);

                        Size_Change = true;
                    }
                    else Size_Timer -= diff;
                }
            }

            if (Impale_Timer <= diff)
            {
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 30.0f, true);
                if (target)
                    me->CastSpell(target, IMPALE, false);

                Impale_Timer = 6000 + urand(0, 4000);
            }
            else Impale_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_frozen_shard : public CreatureScript
{
public:
    npc_frozen_shard() : CreatureScript("npc_frozen_shard") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_frozen_shardAI(pCreature);
    }

    struct npc_frozen_shardAI : public ScriptedAI
    {
        npc_frozen_shardAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();

            me->SetVisible(false);
            me->setFaction(35);
        }

        InstanceScript* instance;
        uint32 Size_Timer;
        float Size;
        bool Visual_Spell;
        bool Size_Change;

        void Reset() 
        {
            Visual_Spell = false;
            Size_Change = false;
            Size_Timer = 0;
        }

        void UpdateAI(const uint32 diff)
        {
            if (me->GetVisibility() == VISIBILITY_ON)
            {
                if (Visual_Spell == false)
                {
                    me->CastSpell(me, FROZEN_SERVITOR_VISUAL, false);

                    Size_Timer = 1000;
                    Visual_Spell = true;
                }
            }

            if (Visual_Spell == true)
            {
                if (Size_Change == false)
                {
                    if (Size_Timer <= diff)
                    {
                        Size = 1.00;
                        me->SetFloatValue(OBJECT_FIELD_SCALE_X, Size);

                        Size_Change = true;
                        me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[2][0], ThrallMovePoints[2][1], ThrallMovePoints[2][2], true);
                        me->SetWalk(true);
                    }
                    else Size_Timer -= diff;
                }
            }

            DoMeleeAttackIfReady();
        }
    };
};

//////////////////////////
/////  Asira  Trash  /////
//////////////////////////
enum CreaturesAsiraTrash
{
    TWILIGHT_ASSASSIN          = 55106,
    TWILIGHT_SHADOW_WALKER     = 55109,
    TWILIGHT_BRUISER           = 55112,
    TWILIGHT_THUG              = 55111,
    TWILIGHT_RANGER            = 55107,
};
enum AsiraTrashSpells
{
    STEALTH                    = 102921,
    GARROTE                    = 102925,
    GARROTE_SILENCE            = 102926,

    HUNGERING_SHADOWS          = 103021,
    SHADOWFORM                 = 107903,
    MIND_FLAY                  = 103024,

    CLEAVE                     = 103001,
    MORTAL_STRIKE              = 103002,
    STAGGERING_BLOW            = 103000,

    BEATDOWN                   = 102989,
    BASH                       = 102990,

    DISENGAGE                  = 102975,
    ICE_ARROW                  = 108443,
    SHOOT                      = 102410,
};

class npc_twilight_assassin : public CreatureScript
{
public:
    npc_twilight_assassin() : CreatureScript("npc_twilight_assassin") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_twilight_assassinAI(pCreature);
    }

    struct npc_twilight_assassinAI : public ScriptedAI
    {
        npc_twilight_assassinAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Garrote_Timer;
        bool Garrote;

        void JustDied(Unit* /*who*/)
        {
            //if (InstanceScript *pInstance = me->GetInstanceScript())
            //    pInstance->SetData(DATA_MOVEMENT_PROGRESS, 1);

            if (InstanceScript *pInstance = me->GetInstanceScript())
                pInstance->SetData(DATA_ASIRA_INTRO, 1);
        }

        void Reset() 
        {
            me->setPowerType(POWER_ENERGY);
            me->SetMaxPower(POWER_ENERGY, 100);
            me->SetPower(POWER_ENERGY, 100);

            me->CastSpell(me, STEALTH, false);
            Garrote = false;
            Garrote_Timer = 10000+urand(0,3000);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (Garrote == false)
                if (!me->HasAura(STEALTH))
                {
                    me->CastSpell(me->GetVictim(), GARROTE, true);
                    Garrote = true;
                }

            if (Garrote_Timer <= diff)
            {
                me->CastSpell(me->GetVictim(), GARROTE, true);
                Garrote_Timer = 10000+urand(0,3000);
            }
            else Garrote_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_twilight_shadow_walker : public CreatureScript
{
public:
    npc_twilight_shadow_walker() : CreatureScript("npc_twilight_shadow_walker") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_twilight_shadow_walkerAI(pCreature);
    }

    struct npc_twilight_shadow_walkerAI : public ScriptedAI
    {
        npc_twilight_shadow_walkerAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Mind_Flay_Timer;
        uint32 Hungering_Shadows_Timer;

        void Reset() 
        {
            Mind_Flay_Timer = 1000+urand(0,3000);
            Hungering_Shadows_Timer = 8000+urand(0,4000);
            me->CastSpell(me, SHADOWFORM, true);
        }

        void JustDied(Unit* /*who*/)
        {
            if (InstanceScript *pInstance = me->GetInstanceScript())
                pInstance->SetData(DATA_MOVEMENT_PROGRESS, 1);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (Hungering_Shadows_Timer <= diff)
            {
                me->CastSpell(me, HUNGERING_SHADOWS, true);
                Hungering_Shadows_Timer = 15000+urand(0, 3000);
            }
            else Hungering_Shadows_Timer -= diff;

            if (Mind_Flay_Timer <= diff)
            {
                if (!me->HasUnitState(UNIT_STATE_CASTING))
                {
                    Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true);
                    if (target)
                        me->CastSpell(target, MIND_FLAY, true);
                    Mind_Flay_Timer = 7000 + urand(0, 1500);
                }
            }
            else Mind_Flay_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_twilight_bruiser : public CreatureScript
{
public:
    npc_twilight_bruiser() : CreatureScript("npc_twilight_bruiser") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_twilight_bruiserAI(pCreature);
    }

    struct npc_twilight_bruiserAI : public ScriptedAI
    {
        npc_twilight_bruiserAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Special_Attack_Timer;
        int Random;

        void Reset() 
        {
            Special_Attack_Timer = 3000+urand(0, 5000);
        }

        void JustDied(Unit* /*who*/)
        {
            if (InstanceScript *pInstance = me->GetInstanceScript())
                pInstance->SetData(DATA_MOVEMENT_PROGRESS, 1);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (Special_Attack_Timer <= diff)
            {
                Random = urand(0, 2);
                switch (Random)
                {
                case 0:
                    me->CastSpell(me->GetVictim(), CLEAVE, false);
                    break;
                case 1:
                    me->CastSpell(me->GetVictim(), MORTAL_STRIKE, false);
                    break;
                case 2:
                    me->CastSpell(me->GetVictim(), STAGGERING_BLOW, false);
                    break;
                }
                Special_Attack_Timer = 3000+urand(0, 3000);
            }
            else Special_Attack_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_twilight_thug : public CreatureScript
{
public:
    npc_twilight_thug() : CreatureScript("npc_twilight_thug") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_twilight_thugAI(pCreature);
    }

    struct npc_twilight_thugAI : public ScriptedAI
    {
        npc_twilight_thugAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Bash_Timer;
        uint32 Beatdown_Timer;

        void Reset() 
        {
            Bash_Timer = 15000;
            Beatdown_Timer = 15000;
        }

        void JustDied(Unit* /*who*/)
        {
            if (InstanceScript *pInstance = me->GetInstanceScript())
                pInstance->SetData(DATA_MOVEMENT_PROGRESS, 1);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (Bash_Timer <= diff)
            {
                me->CastSpell(me->GetVictim(), BASH, false);
                Bash_Timer = 15000+urand(0,5000);
            }
            else Bash_Timer -= diff;

            if (Beatdown_Timer <= diff)
            {
                me->CastSpell(me, BEATDOWN, false);
                Beatdown_Timer = 20000+urand(0,5000);
            }
            else Beatdown_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_twilight_ranger : public CreatureScript
{
public:
    npc_twilight_ranger() : CreatureScript("npc_twilight_ranger") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_twilight_rangerAI(pCreature);
    }

    struct npc_twilight_rangerAI : public ScriptedAI
    {
        npc_twilight_rangerAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Disengage_Timer;
        uint32 Shoot_Timer;
        uint32 Ice_Arrow_Timer;

        void Reset() 
        {
            Disengage_Timer = 30000+urand(0, 10000);
            Ice_Arrow_Timer = 0;
            Shoot_Timer = 0;
        }

        void JustDied(Unit* /*who*/)
        {
            if (InstanceScript *pInstance = me->GetInstanceScript())
                pInstance->SetData(DATA_MOVEMENT_PROGRESS, 1);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (Disengage_Timer <= diff)
            {
                me->CastSpell(me, DISENGAGE, false);
                Disengage_Timer = 30000;
            }
            else Disengage_Timer -= diff;

            if (Shoot_Timer <= diff)
            {
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true);
                if (target)
                    me->CastSpell(target, SHOOT, false);
                Shoot_Timer = 15000+urand(0,5000);
            }
            else
            {
                Shoot_Timer -= diff;

                if (Ice_Arrow_Timer <= diff)
                {
                    Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true);
                    if (target)
                        me->CastSpell(target, ICE_ARROW, false);
                    Ice_Arrow_Timer = 5000+urand(0, 3000);
                }
                else Ice_Arrow_Timer -= diff;
            }

            DoMeleeAttackIfReady();
        }
    };
};

class npc_rising_flame_totem : public CreatureScript
{
public:
    npc_rising_flame_totem() : CreatureScript("npc_rising_flame_totem") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_rising_flame_totemAI(pCreature);
    }

    struct npc_rising_flame_totemAI : public ScriptedAI
    {
        npc_rising_flame_totemAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        void Reset() 
        {
            me->CastSpell(me, RISING_FLAME, false);
        }

        void UpdateAI(const uint32 diff) { }
    };
};

////////////////////////////
///// Benedictus Trash /////
////////////////////////////

enum CreaturesBenedictusTrash
{
    NCP_SHADOW_BORER = 54686,
    NPC_FACELESS_BRUTE = 54632,
    NPC_FACELESS_SHADOW_WEAVER = 54633,
};
enum BenedictusTrashSpells
{
    SHADOW_BORE = 102995,

    SQUEEZE_LIFELESS = 102860,
    SQUEEZE_LIFELESS_1 = 102861,
    TENTACLE_SMASH = 102848,

    SEEKING_SHADOWS = 102983,
    SEEKING_SHADOWS_1 = 102984,
    SHADOW_VOLLEY = 102992,

    FOG = 88783,
};

class npc_shadow_borer : public CreatureScript
{
public:
    npc_shadow_borer() : CreatureScript("npc_shadow_borer") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_shadow_borerAI(pCreature);
    }

    struct npc_shadow_borerAI : public ScriptedAI
    {
        npc_shadow_borerAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Shadow_Bore_Timer;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            Shadow_Bore_Timer = 3000;
        }

        void JustDied(Unit* /*who*/)
        {
            if (InstanceScript *pInstance = me->GetInstanceScript())
                pInstance->SetData(DATA_MOVEMENT_PROGRESS, 1);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;


            if (Shadow_Bore_Timer <= diff)
            {
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true);
                if (target)
                    me->CastSpell(target, SHADOW_BORE, false);
                Shadow_Bore_Timer = 17000;
            }
            else Shadow_Bore_Timer -= diff;
        }
    };
};

class npc_faceless_brute : public CreatureScript
{
public:
    npc_faceless_brute() : CreatureScript("npc_faceless_brute") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_faceless_bruteAI(pCreature);
    }

    struct npc_faceless_bruteAI : public ScriptedAI
    {
        npc_faceless_bruteAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Tentacle_Smash_Timer;
        uint32 Squeeze_Lifeless_Timer;

        void Reset()
        {
            Tentacle_Smash_Timer = 10000 + urand(0, 5000);
            Squeeze_Lifeless_Timer = 20000 + urand(0, 10000);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (Tentacle_Smash_Timer <= diff)
            {
                me->CastSpell(me->GetVictim(), TENTACLE_SMASH, true);
                Tentacle_Smash_Timer = 25000 + urand(0, 5000);
            }
            else Tentacle_Smash_Timer -= diff;

            if (Squeeze_Lifeless_Timer <= diff)
            {
                me->CastSpell(me, SQUEEZE_LIFELESS, true);
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true);
                if (target)
                    me->CastSpell(target, SQUEEZE_LIFELESS_1, true);
                Squeeze_Lifeless_Timer = 35000 + urand(0, 10000);
            }
            else Squeeze_Lifeless_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_faceless_shadow_weaver : public CreatureScript
{
public:
    npc_faceless_shadow_weaver() : CreatureScript("npc_faceless_shadow_weaver") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_faceless_shadow_weaverAI(pCreature);
    }

    struct npc_faceless_shadow_weaverAI : public ScriptedAI
    {
        npc_faceless_shadow_weaverAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Seeking_Shadows_Timer;
        uint32 Shadow_Volley_Timer;

        void Reset()
        {
            Seeking_Shadows_Timer = 10000 + urand(0, 10000);
            Shadow_Volley_Timer = 5000 + urand(0, 5000);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (Seeking_Shadows_Timer <= diff)
            {
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 60.0f, true);
                if (target)
                {
                    me->CastSpell(target, SEEKING_SHADOWS, false);
                    me->CastSpell(target, SEEKING_SHADOWS_1, false);
                }
                Seeking_Shadows_Timer = 20000 + urand(0, 10000);
            }
            else Seeking_Shadows_Timer -= diff;

            if (Shadow_Volley_Timer <= diff)
            {
                me->CastSpell(me, SHADOW_VOLLEY, false);
                Shadow_Volley_Timer = 15000 + urand(0, 5000);
            }
            else Shadow_Volley_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

// Npc Fog - 119513
class npc_fog : public CreatureScript
{
public:
    npc_fog() : CreatureScript("npc_fog") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_fogAI(pCreature);
    }

    struct npc_fogAI : public ScriptedAI
    {
        npc_fogAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->CastSpell(me, FOG, false);
        }

        void JustDied(Unit* /*who*/)
        {
            Creature * fog = me->FindNearestCreature(NPC_FOG, 20.0f, true);
            if (fog)
                fog->Kill(fog);
        }

    };
};

void AddSC_instance_hour_of_twilight()
{
    new instance_hour_of_twilight();

    new npc_thrall_hour_of_twilight();
    new npc_thrall_hour_of_twilight1();
    new npc_thrall_hour_of_twilight2();
    new npc_thrall_hour_of_twilight3();
    new npc_thrall_hour_of_twilight4();

    new npc_frozen_servitor();
    new npc_crystalline_elemental();
    new npc_frozen_shard();

    new npc_twilight_assassin();
    new npc_twilight_shadow_walker();
    new npc_twilight_bruiser();
    new npc_twilight_thug();
    new npc_twilight_ranger();
    new npc_rising_flame_totem();

    new npc_shadow_borer();
    new npc_faceless_brute();
    new npc_faceless_shadow_weaver();
    new npc_fog();
}