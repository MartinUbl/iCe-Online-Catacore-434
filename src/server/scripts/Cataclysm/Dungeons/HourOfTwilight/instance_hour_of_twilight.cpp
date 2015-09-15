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
    {4322.37f, 483.20f, -8.7360f, 3.113f}, // Closer to the 2nd camp
    {4323.48f, 532.85f, -8.6174f, 1.492f}, // Before hill
    {4290.04f, 568.05f, -7.0952f, 2.964f}, // Asira`s clearing
    {4283.30f, 590.06f, -6.4609f, 1.498f}, // Revive Life-Warden
    {4284.70f, 600.89f, -4.5608f, 1.266f}, // Jump on Life Warden
    {4260.03f, 445.29f, 43.1593f, 4.431f}, // Fly Away
    // Benedictus Thrall 27-36
    {3916.99f, 275.17f,   8.1773f, 3.190f}, // First stop
    {3895.41f, 277.70f,   2.3380f, 3.092f}, // First trash
    {3837.12f, 280.85f, -20.5216f, 3.060f}, // Second trash
    {3809.39f, 290.78f, -38.8207f, 3.010f}, // Second stop
    {3762.31f, 289.02f, -64.3801f, 3.167f}, // Third trash
    {3738.73f, 289.87f, -84.0984f, 3.159f}, // Wait here for Archbishop
    {3668.17f, 284.15f, -119.399f, 3.170f}, // Run down the hill
    {3595.60f, 277.90f, -119.968f, 3.232f}, // Stop before stairs
    {3582.70f, 277.09f, -114.031f, 3.274f}, // Run up
    {3562.59f, 274.80f, -115.964f, 3.265f}, // Benedictus
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
        uint64 asiraGuid;
        uint64 benedictusGuid;
        uint64 thrallGuid;
        uint64 thrall1Guid;
        uint64 thrall2Guid;
        uint64 thrall3Guid;
        uint64 thrall4Guid;

        uint32 instance_progress;
        uint32 movement_progress;
        uint32 asira_intro;
        uint32 drakes;
        uint32 benedictus_intro;

        uint32 Check_Timer;

        std::string saveData;

        void Initialize()
        {
            instance_progress  = 0;
            movement_progress  = 0;
            asira_intro        = 0;
            drakes             = 0;
            benedictus_intro   = 0;

            arcurionGuid       = 0;
            asiraGuid          = 0;
            benedictusGuid     = 0;
            thrallGuid         = 0;
            thrall1Guid        = 0;
            thrall2Guid        = 0;
            thrall3Guid        = 0;
            thrall4Guid        = 0;

            Check_Timer         = 0;

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
            saveStream << " " << benedictus_intro;

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
            loadStream >> benedictus_intro;

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
                case 54968: // Asira Dawnslayer
                    asiraGuid = pCreature->GetGUID();
                    break;
                case 54938: // Archbishop Benedictus
                    benedictusGuid = pCreature->GetGUID();
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
                case TYPE_BOSS_ASIRA_DAWNSLAYER:
                    return asiraGuid;
                case TYPE_BOSS_ARCHBISHOP_BENEDICTUS:
                    return benedictusGuid;
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

            if (Check_Timer <= diff)
            {
                if (instance_progress > 1)
                {
                    Creature * thrall = this->instance->GetCreature(this->GetData64(TYPE_THRALL));
                    if (thrall)
                        thrall->SetVisible(false);
                }

                if (instance_progress == 2)
                {
                    Creature * thrall_1 = this->instance->GetCreature(this->GetData64(TYPE_THRALL1));
                    if (thrall_1)
                        thrall_1->SetVisible(true);
                }

                if (instance_progress == 3)
                {
                    Creature * thrall_2 = this->instance->GetCreature(this->GetData64(TYPE_THRALL2));
                    if (thrall_2)
                        thrall_2->SetVisible(true);
                }

                if (instance_progress == 5)
                {
                    Creature * thrall_3 = this->instance->GetCreature(this->GetData64(TYPE_THRALL3));
                    if (thrall_3)
                        thrall_3->SetVisible(false);

                    Creature * thrall_4 = this->instance->GetCreature(this->GetData64(TYPE_THRALL4));
                    if (thrall_4)
                        thrall_4->SetVisible(true);

                    if (benedictus_intro == 2)
                    {
                        Creature * benedictus = this->instance->GetCreature(this->GetData64(TYPE_BOSS_ARCHBISHOP_BENEDICTUS));
                        if (benedictus)
                        {
                            benedictus->SetVisible(true);
                            benedictus->setFaction(16);
                        }
                    }
                }

                if (movement_progress == 8)
                {
                    Creature * arcurion = this->instance->GetCreature(this->GetData64(TYPE_BOSS_ARCURION));
                    if (arcurion)
                    {
                        arcurion->SetVisible(true);
                        arcurion->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                        arcurion->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        arcurion->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        arcurion->SetReactState(REACT_AGGRESSIVE);
                    }
                }

                if (asira_intro >= 3)
                {
                    Creature * asira = this->instance->GetCreature(this->GetData64(TYPE_BOSS_ASIRA_DAWNSLAYER));
                    if (asira)
                    {
                        asira->SetVisible(true);
                        asira->setFaction(16);
                    }
                }

                Check_Timer = 60000;
            }
            else Check_Timer -= diff;
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

            if (DataId == DATA_BENEDICTUS_INTRO)
                return benedictus_intro;

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
                instance_progress = data;
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

            if (type == DATA_BENEDICTUS_INTRO)
            {
                benedictus_intro = data;
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
            currEnc[0] = m_auiEncounter[TYPE_BOSS_ASIRA_DAWNSLAYER]; // Asira 1
            currEnc[1] = m_auiEncounter[TYPE_BOSS_ARCHBISHOP_BENEDICTUS]; // Benedictus 2
            currEnc[2] = m_auiEncounter[TYPE_BOSS_ARCURION]; // Arcurion 0

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
        uint32 Lava_Burst_Timer;
        uint32 Move_Timer;
        uint32 Move_Point;
        bool Lavaburst;

        void Reset() 
        {
            Move_Point = 0;
            Lava_Burst_Timer = 0;
            Move_Timer = 0;
            Lavaburst = false;
        }

        void EnterCombat(Unit * /*who*/) { }

        void DoAction(const int32 /*param*/)
        {
            Move_Timer = 8000;

            if (instance)
                instance->SetData(DATA_INSTANCE_PROGRESS, 1); // 1

            me->MonsterSay("Heroes, we have the Dragon Soul. The Aspects await us within Wyrmrest. Hurry - come with me!", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
            me->SendPlaySound(25870, true);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }

        void CastLavaburst()
        {
            if (instance->GetData(TYPE_BOSS_ARCURION) == IN_PROGRESS)
            {
                Creature * arcurion = me->FindNearestCreature(54590, 200.0, true);
                if (arcurion && arcurion->HealthBelowPct(30))
                    me->CastSpell(arcurion, THRALL_SPELL_LAVABURST, false);
                else
                {
                    Creature * frozen_servitor = me->FindNearestCreature(119509, 150.0, true);
                    if (frozen_servitor && frozen_servitor->IsInCombat())
                        me->CastSpell(frozen_servitor, THRALL_SPELL_LAVABURST, false);
                }
            }
            else
            {
                Creature * frozen_servitor = me->FindNearestCreature(54555, 30.0, true);
                if (frozen_servitor && frozen_servitor->IsInCombat())
                    me->CastSpell(frozen_servitor, THRALL_SPELL_LAVABURST, false);
            }
        }

        void SetVisible()
        {
            if (instance->GetData(DATA_MOVEMENT_PROGRESS) < 5)
            {
                std::list<Creature*> frozen_servitor;
                GetCreatureListWithEntryInGrid(frozen_servitor, me, 54555, 50.0f);
                for (std::list<Creature*>::const_iterator itr = frozen_servitor.begin(); itr != frozen_servitor.end(); ++itr)
                    if (*itr)
                    {
                        (*itr)->SetVisible(true);
                        (*itr)->setFaction(16);
                    }
            }

            if (instance->GetData(DATA_MOVEMENT_PROGRESS) >= 5)
            {
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
            }
        }

        void UpdateAI(const uint32 diff) 
        {
            if (instance)
            {
                if (instance->GetData(DATA_INSTANCE_PROGRESS) == 1)
                {
                    // Thrall movements update
                    if (Move_Timer <= diff)
                    {
                        Move_Timer = 1000;

                        switch (Move_Point)
                        {
                        case 0: // Arcurion yell
                        {
                            Creature * arcurion = me->FindNearestCreature(54590, 500.0f, true);
                            if (arcurion)
                                arcurion->MonsterYell("Shaman! The Dragon Soul is not yours. Give it up, and you may yet walk away with your life", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25798, false);
                            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                            SetVisible();
                            Move_Point++;
                            Move_Timer = 20000;
                            break;
                        }
                        case 1:
                            me->MonsterSay("How did the Twilight Hammer find us? Ready your weapons - we've got to get out of this canyon!", LANG_UNIVERSAL, me->GetGUID(), 100.0f);
                            me->SendPlaySound(25871, true);
                            me->GetMotionMaster()->MovePoint(0, 4926.0f, 289.0f, 96.75f);
                            Move_Point++;
                            Lavaburst = true;
                            break;
                        case 2:
                            if (instance->GetData(DATA_MOVEMENT_PROGRESS) >= 2)
                            {
                                me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[0][0], ThrallMovePoints[0][1], ThrallMovePoints[0][2]);
                                Move_Timer = 5000;
                                Move_Point++;
                                Lavaburst = false;
                            }
                            else
                                Move_Timer = 1000;
                            break;
                        case 3:
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[1][0], ThrallMovePoints[1][1], ThrallMovePoints[1][2]);
                            Move_Point++;
                            Move_Timer = 8000;
                            break;
                        case 4:
                            me->MonsterSay("What magic is this?", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
                            me->SendPlaySound(25872, true);
                            Move_Point++;
                            Move_Timer = 3000;
                            break;
                        case 5:
                        {
                            me->MonsterSay("Look Out!", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
                            me->SendPlaySound(25873, true);

                            Creature * arcurion = me->FindNearestCreature(54590, 400.0f, true);
                            if (arcurion)
                                arcurion->MonsterYell("Destroy them all, but bring the Shaman to me!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25799, true);

                            SetVisible();
                            Move_Point++;
                            Lavaburst = true;
                            break;
                        }
                        case 6:
                            if (instance->GetData(DATA_MOVEMENT_PROGRESS) >= 5)
                            {
                                me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[2][0], ThrallMovePoints[2][1], ThrallMovePoints[2][2]);
                                Move_Timer = 9000;
                                Move_Point++;
                                Lavaburst = false;
                            }
                            else
                                Move_Timer = 1000;
                            break;
                        case 7:
                        {
                            Creature * arcurion = me->FindNearestCreature(54590, 400.0f, true);
                            if (arcurion)
                                arcurion->MonsterYell("You will go nowhere. Shaman.", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25800, true);

                            SetVisible();
                            Move_Point++;
                            Lavaburst = true;
                            break;
                        }
                        case 8:
                            if (instance->GetData(DATA_MOVEMENT_PROGRESS) >= 6)
                            {
                                me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[3][0], ThrallMovePoints[3][1], ThrallMovePoints[3][2]);
                                Move_Timer = 19000;
                                Move_Point++;
                                Lavaburst = false;
                            }
                            else
                                Move_Timer = 1000;
                            break;
                        case 9:
                        {
                            Creature * arcurion = me->FindNearestCreature(54590, 100.0f, true);
                            if (arcurion)
                            {
                                arcurion->SetVisible(true);
                                arcurion->CastSpell(arcurion, ARCURION_SPAWN_VISUAL, false);
                            }

                            if (instance->GetData(DATA_MOVEMENT_PROGRESS) == 6) // Avoid change counter when Thrall is running again after server crash
                                instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 7
                            me->MonsterSay("Show yourself!", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
                            me->SendPlaySound(25877, true);
                            Move_Point++;
                            break;
                        }
                        case 10:
                            if (instance->GetData(TYPE_BOSS_ARCURION) == DONE && !me->HasAura(103251) /*Icy Tomb*/)
                            {
                                Move_Point++;
                                Move_Timer = 5000;
                            }
                            else
                                Move_Timer = 1000;
                            break;
                        case 11:
                            me->MonsterSay("We've been discovered. I know you are tired but we cannot keep the aspects waiting!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25882, true);
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[4][0], ThrallMovePoints[4][1], ThrallMovePoints[4][2]);
                            Move_Timer = 5000;
                            Move_Point++;
                            break;
                        case 12: // Thrall destroys Skywall
                        {
                            Creature * Npc_Skywall = me->FindNearestCreature(119508, 100.0f, true);
                            if (Npc_Skywall)
                                me->CastSpell(Npc_Skywall, THRALL_SPELL_LAVABURST, false);
                            else
                            {
                                if (GameObject* Icewall = me->FindNearestGameObject(210049, 50.0f))
                                    Icewall->UseDoorOrButton();
                                instance->SetData(DATA_INSTANCE_PROGRESS, 2); // 2
                            }
                            Move_Point++;
                            break;
                        }
                        default:
                            break;
                        }

                    }
                    else Move_Timer -= diff;

                    if (Lavaburst == true || (instance->GetData(TYPE_BOSS_ARCURION) == IN_PROGRESS))
                    {
                        if (Lava_Burst_Timer <= diff)
                        {
                            CastLavaburst();
                            Lava_Burst_Timer = 3000;
                        }
                        else Lava_Burst_Timer -= diff;
                    }
                }
            }
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
        uint32 Move_Point;
        bool Timer_Set;
        bool Ghost_Wolf_Form;

        void Reset() 
        {
            Timer_Set = false;
            Ghost_Wolf_Form = false;
            Ghost_Wolf_Timer = 0;
            Move_Timer = 0;
            Move_Point = 5;
        }

        void EnterCombat(Unit * /*who*/) { }

        void UpdateAI(const uint32 diff)
        {
            if (instance->GetData(DATA_INSTANCE_PROGRESS) == 2)
            {
                if (!Timer_Set)
                {
                    // If instance progress 2, set 2s timer for start casting Wolf form
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
                    if (Move_Timer <= diff)
                    {
                        switch (Move_Point)
                        {
                        case 5:
                            Move_Timer = 10000;
                            break;
                        case 6:
                            Move_Timer = 8500;
                            break;
                        case 7:
                            Move_Timer = 6000;
                            break;
                        case 8:
                            Move_Timer = 4000;
                            break;
                        case 9:
                            Move_Timer = 9000;
                            break;
                        case 10:
                            Move_Timer = 10000;
                            break;
                        case 11:
                            Move_Timer = 4500;
                            break;
                        case 12:
                            Move_Timer = 4500;
                            break;
                        case 13:
                            Move_Timer = 12000;
                            break;
                        case 14:
                            Move_Timer = 8000;
                            break;
                        case 15:
                            {
                                me->SetVisible(false);
                                Creature * thrall_next = me->FindNearestCreature(THRALL_2, 50.0f, true);
                                if (thrall_next)
                                    thrall_next->SetVisible(true);
                                instance->SetData(DATA_INSTANCE_PROGRESS, 3); // 3
                            }
                            break;
                        default:
                            break;
                        }
                        if (instance->GetData(DATA_INSTANCE_PROGRESS) == 2)
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[Move_Point][0], ThrallMovePoints[Move_Point][1], ThrallMovePoints[Move_Point][2]);
                        Move_Point++;
                    }
                    else Move_Timer -= diff;
                }
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
        }

        InstanceScript* instance;
        uint32 Move_Timer;
        uint32 Move_Point;
        uint32 Lavaburst_Timer;
        uint32 Summon_Totem_Timer;
        uint32 Check_Timer;
        int Twilight;
        bool Lavaburst;

        void Reset() 
        {
            Twilight = 0;
            Lavaburst = false;
            Summon_Totem_Timer = 0;
            Lavaburst_Timer = 0;
            Check_Timer = 0;
            Move_Point = 0;
            Move_Timer = 0;
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
            if (instance)
            {
                if (instance->GetData(DATA_INSTANCE_PROGRESS) == 3)
                {
                    // Thrall movement update
                    if (Move_Timer <= diff)
                    {
                        Move_Timer = 1000;

                        switch (Move_Point)
                        {
                        case 0:
                            Move_Timer = 3000;
                            Move_Point++;
                            break;
                        case 1:
                            me->SetStandState(UNIT_STAND_STATE_STAND);
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[15][0], ThrallMovePoints[15][1], ThrallMovePoints[15][2]);
                            Move_Timer = 2000;
                            Move_Point++;
                            break;
                        case 2:
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[16][0], ThrallMovePoints[16][1], ThrallMovePoints[16][2]);
                            Move_Timer = 5000;
                            Move_Point++;
                            break;
                        case 3:
                            if (instance->GetData(DATA_MOVEMENT_PROGRESS) >= 10)
                            {
                                me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[17][0], ThrallMovePoints[17][1], ThrallMovePoints[17][2]);
                                Move_Timer = 6000;
                                Move_Point++;
                            }
                            else
                                Move_Timer = 1000;
                            break;
                        case 4:
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[18][0], ThrallMovePoints[18][1], ThrallMovePoints[18][2]);
                            Move_Timer = 5000;
                            Move_Point++;
                            break;
                        case 5:
                            me->MonsterYell("Be ware, enemies approach!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25883, true);
                            Move_Point++;
                            Summon_Totem_Timer = 5000 + urand(0, 10000);
                            Lavaburst = true;
                            break;
                        case 6:
                            if (instance->GetData(DATA_MOVEMENT_PROGRESS) >= 15)
                            {
                                me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[19][0], ThrallMovePoints[19][1], ThrallMovePoints[19][2]);
                                Move_Timer = 5500;
                                Move_Point++;
                                Lavaburst = false;
                            }
                            else
                                Move_Timer = 1000;
                            break;
                        case 7:
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[20][0], ThrallMovePoints[20][1], ThrallMovePoints[20][2]);
                            Move_Timer = 6000;
                            Move_Point++;
                            break;
                        case 8:
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[21][0], ThrallMovePoints[21][1], ThrallMovePoints[21][2]);
                            Move_Timer = 2500;
                            Move_Point++;
                            break;
                        case 9:
                            me->MonsterYell("Let none stand in our way!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25884, true);
                            Move_Point++;
                            Summon_Totem_Timer = 5000 + urand(0, 10000);
                            Lavaburst = true;
                            break;
                        case 10:
                            if (instance->GetData(DATA_MOVEMENT_PROGRESS) >= 20)
                            {
                                me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[22][0], ThrallMovePoints[22][1], ThrallMovePoints[22][2]);
                                Move_Timer = 7000;
                                Move_Point++;
                                Lavaburst = false;
                            }
                            else
                                Move_Timer = 1000;
                            break;
                        case 11:
                            me->MonsterYell("Twilight`s hammer returns!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25885, true);
                            Move_Point++;
                            Summon_Totem_Timer = 5000 + urand(0, 10000);
                            Lavaburst = true;
                            break;
                        case 12:
                            if (instance->GetData(DATA_MOVEMENT_PROGRESS) >= 25)
                            {
                                me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[23][0], ThrallMovePoints[23][1], ThrallMovePoints[23][2]);
                                Move_Timer = 7000;
                                Move_Point++;
                                Lavaburst = false;
                            }
                            else
                                Move_Timer = 1000;
                            break;
                        case 13:
                            me->GetMotionMaster()->MovePoint(0, 4286.44f, 568.85f, -6.925f);
                            Move_Timer = 1000;
                            Move_Point++;
                            break;
                        case 14:
                            me->MonsterYell("Let them come!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25886, true);
                            Move_Point++;
                            Summon_Totem_Timer = 5000 + urand(0, 10000);
                            Lavaburst = true;
                            break;
                        case 15:
                            if (instance->GetData(DATA_MOVEMENT_PROGRESS) >= 30)
                            {
                                me->MonsterSay("Alexstrasza's drakes should meet us here...", LANG_UNIVERSAL, 0);
                                me->SendPlaySound(25887, true);
                                Move_Timer = 5000;
                                Move_Point++;
                                Lavaburst = false;
                            }
                            else
                                Move_Timer = 1000;
                            break;
                        case 16:
                            if (instance->GetData(DATA_ASIRA_INTRO) == 0 || instance->GetData(DATA_ASIRA_INTRO) == 2)
                            {
                                Creature * life_warden = me->FindNearestCreature(LIFE_WARDEN, 300.0f, false);
                                if (life_warden)
                                    life_warden->Respawn(true);
                                instance->SetData(DATA_ASIRA_INTRO, 1); // 1
                            }
                            me->GetMotionMaster()->MovePoint(0, 4284.01f, 568.60f, -6.853f);
                            me->MonsterSay("Up there, above us!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25888, true);
                            Move_Timer = 20000;
                            Move_Point++;
                            break;
                        case 17:
                            me->GetMotionMaster()->MovePoint(0, 4281.76f, 568.75f, -6.761f);
                            Move_Point++;
                            break;
                        // OUTRO
                        case 18: // Walk to Life Warden
                            if (instance->GetData(TYPE_BOSS_ASIRA_DAWNSLAYER) == DONE)
                            {
                                me->SetWalk(true);
                                me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[24][0], ThrallMovePoints[24][1], ThrallMovePoints[24][2]);
                                Move_Timer = 4000;
                                Move_Point++;
                            }
                            else
                                Move_Timer = 1000;
                            break;
                        case 19: // Say
                            me->MonsterSay("Well done. Let us see to our friend...", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25891, true);
                            Move_Timer = 5000;
                            Move_Point++;
                            break;
                        case 20: // Cast Ancestral Spirit
                        {
                            me->CombatStop();
                            me->CastSpell(me, ANCESTRAL_SPIRIT, false);
                            Move_Timer = 3800;
                            Move_Point++;
                            break;
                        }
                        case 21: // Revive Life Warden
                        {
                            Creature * life_warden = me->FindNearestCreature(LIFE_WARDEN, 50.0f, false);
                            if (life_warden)
                            {
                                life_warden->setDeathState(JUST_ALIVED);
                                life_warden->CastSpell(life_warden, RESURRECTION, true);
                            }
                            Move_Timer = 1000;
                            Move_Point++;
                            break;
                        }
                        case 22: // Jump on Life Warden
                        {
                            Creature * life_warden = me->FindNearestCreature(LIFE_WARDEN, 50.0f, true);
                            if (life_warden)
                                me->GetMotionMaster()->MovePoint(0, life_warden->GetPositionX(), life_warden->GetPositionY(), life_warden->GetPositionZ());
                            Move_Timer = 5000;
                            Move_Point++;
                            break;
                        }
                        case 23: // Despawn Life Warden - Mount Thrall
                        {
                            Creature * life_warden = me->FindNearestCreature(LIFE_WARDEN, 50.0f, true);
                            if (life_warden)
                            {
                                life_warden->SetVisible(false);
                                life_warden->DespawnOrUnsummon();
                            }
                            me->CastSpell(me, RED_DRAKE, true);
                            me->MonsterSay("The rest of the drakes should be here shortly. I'll fly on ahead, catch up when you can.", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25892, true);
                            Move_Timer = 1000;
                            Move_Point++;
                            break;
                        }
                        case 24: // Fly Away
                            me->SetFlying(true);
                            Move_Timer = 15000;
                            Move_Point++;
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[26][0], ThrallMovePoints[26][1], ThrallMovePoints[26][2]);
                            break;
                        case 25: // Disappear
                            me->DespawnOrUnsummon();
                            instance->SetData(DATA_DRAKES, 1); // 1
                            Move_Point++;
                            break;
                        default:
                            break;
                        }
                    }
                    else Move_Timer -= diff;

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
                            int distance = urand(7, 15);
                            float angle = me->GetOrientation();
                            me->SummonCreature(RISING_FLAME_TOTEM, me->GetPositionX() + cos(angle)*distance, me->GetPositionY() + sin(angle)*distance, me->GetPositionZ() + 1, angle, TEMPSUMMON_TIMED_DESPAWN, 30000);
                            Summon_Totem_Timer = 35000 + urand(0, 15000);
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
                                    int distance = urand(10, 40);
                                    float angle = urand(22, 39) / 10;
                                    me->SummonCreature(RISING_FLAME_TOTEM, me->GetPositionX() + cos(angle)*distance, me->GetPositionY() + sin(angle)*distance, me->GetPositionZ() + 1, angle, TEMPSUMMON_TIMED_DESPAWN, 30000);
                                    Summon_Totem_Timer = 15000 + urand(0, 15000);
                                }
                                else Summon_Totem_Timer -= diff;
                            }
                        }
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
        uint32 Move_Point;
        bool Show_Trash;

        void Reset()
        {
            Move_Timer = 0;
            Move_Point = 0;
            Show_Trash = false;
        }

        void DoAction(const int32 /*param*/)
        {
            instance->SetData(DATA_INSTANCE_PROGRESS, 4); // 4
            Move_Timer = 1000;
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
            if (instance)
            {
                if (instance->GetData(DATA_INSTANCE_PROGRESS) == 4)
                {
                    // Thrall movement update
                    if (Move_Timer <= diff)
                    {
                        Move_Timer = 1000;

                        switch (Move_Point)
                        {
                        case 0:
                            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[27][0], ThrallMovePoints[27][1], ThrallMovePoints[27][2], true, true);
                            Move_Timer = 5000;
                            Move_Point++;
                            break;
                        case 1:
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[28][0], ThrallMovePoints[28][1], ThrallMovePoints[28][2], true);
                            Move_Timer = 3000;
                            Move_Point++;
                            break;
                        case 2:
                            ShowTrash();
                            Move_Point++;
                            break;
                        case 3:
                            if (instance->GetData(DATA_MOVEMENT_PROGRESS) >= 33)
                            {
                                me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[29][0], ThrallMovePoints[29][1], ThrallMovePoints[29][2], true);
                                Move_Timer = 9000;
                                Move_Point++;
                            }
                            else 
                                Move_Timer = 1000;
                            break;
                        case 4:
                            ShowTrash();
                            Move_Point++;
                            break;
                        case 5:
                            if (instance->GetData(DATA_MOVEMENT_PROGRESS) >= 36)
                            {
                                me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[30][0], ThrallMovePoints[30][1], ThrallMovePoints[30][2], true);
                                Move_Timer = 5000;
                                Move_Point++;
                            }
                            else
                                Move_Timer = 1000;
                            break;
                        case 6:
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[31][0], ThrallMovePoints[31][1], ThrallMovePoints[31][2], true);
                            Move_Timer = 8000;
                            Move_Point++;
                            break;
                        case 7:
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[32][0], ThrallMovePoints[32][1], ThrallMovePoints[32][2], true);
                            Move_Timer = 5000;
                            Move_Point++;
                            break;
                        case 8:
                            ShowTrash();
                            Move_Point++;
                            break;
                        case 9:
                            if (instance->GetData(DATA_MOVEMENT_PROGRESS) >= 39)
                            {
                                Move_Timer = 23000;
                                Move_Point++;
                                instance->SetData(DATA_BENEDICTUS_INTRO, 1); // 1
                            }
                            else
                                Move_Timer = 1000;
                            break;
                        case 10:
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[33][0], ThrallMovePoints[33][1], ThrallMovePoints[33][2], true);
                            Move_Timer = 11000;
                            Move_Point++;
                            break;
                        case 11:
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[34][0], ThrallMovePoints[34][1], ThrallMovePoints[34][2], true);
                            Move_Timer = 10500;
                            Move_Point++;
                            break;
                        case 12:
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[35][0], ThrallMovePoints[35][1], ThrallMovePoints[35][2], true);
                            Move_Timer = 3000;
                            Move_Point++;
                            break;
                        case 13:
                            me->GetMotionMaster()->MovePoint(0, ThrallMovePoints[36][0], ThrallMovePoints[36][1], ThrallMovePoints[36][2], true);
                            Move_Timer = 4000;
                            Move_Point++;
                            break;
                        case 14:
                        {
                            Creature * last_thrall = me->FindNearestCreature(THRALL_4, 10.0f, true);
                            if (last_thrall)
                                last_thrall->SetVisible(true);
                            me->ForcedDespawn(0);
                            instance->SetData(DATA_INSTANCE_PROGRESS, 5); // 5
                            break;
                        }
                        default:
                            break;
                        }
                    }
                    else Move_Timer -= diff;
                }
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
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            Visual_Spell = false;
            Size_Change = false;
            Icy_Boulder_Timer = urand(0,2000);
            Size_Timer = 0;
        }

        void JustDied(Unit* /*who*/)
        {
            if (instance)
                instance->SetData(DATA_MOVEMENT_PROGRESS, 1);
        }

        void EnterCombat(Unit * /*who*/)
        {
            me->SetInCombatWithZone();
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
            if (InstanceScript *pInstance = me->GetInstanceScript())
                pInstance->SetData(DATA_MOVEMENT_PROGRESS, 1);
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
            if (instance)
                instance->SetData(DATA_MOVEMENT_PROGRESS, 1);
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

        void JustDied(Unit* /*who*/)
        {
            if (instance)
                instance->SetData(DATA_MOVEMENT_PROGRESS, 1);
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

        void JustDied(Unit* /*who*/)
        {
            if (instance)
                instance->SetData(DATA_MOVEMENT_PROGRESS, 1);
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

///////////////////////////////////////
//////////// Drakes Event /////////////
///////////////////////////////////////

static const Position DrakePoint[40] =
{
    // 1st Drake
    {4287.72f, 501.17f,  9.24f, 0},
    {4314.09f, 528.44f,  0.10f, 0},
    {4308.13f, 550.56f, -4.07f, 0}, // Point before landing
    {4294.28f, 555.91f, -7.97f, 0}, // Land Point
    {4196.73f, 546.82f, 64.49f, 0},
    {4048.49f, 500.73f, 83.64f, 0},
    {3929.77f, 318.68f, 55.63f, 0},
    // 2nd Drake
    {4290.52f, 502.15f, 14.14f, 0},
    {4309.92f, 537.00f,  1.36f, 0},
    {4308.31f, 558.77f, -4.65f, 0}, // Point before landing
    {4296.77f, 562.69f, -7.66f, 0}, // Land Point
    {4195.51f, 555.06f, 70.30f, 0},
    {4040.02f, 510.73f, 95.19f, 0},
    {3945.68f, 314.75f, 55.63f, 0},
    // 3rd Drake
    {4286.70f, 503.97f, 10.02f, 0},
    {4322.38f, 534.14f, -3.02f, 0},
    {4310.94f, 566.24f, -4.14f, 0}, // Point before landing
    {4299.22f, 570.16f, -7.11f, 0}, // Land Point
    {4194.70f, 560.60f, 64.49f, 0},
    {4035.52f, 515.14f, 83.64f, 0},
    {3928.76f, 319.15f, 49.82f, 0},
    // 4th Drake
    {4298.37f, 512.10f, 15.84f, 0},
    {4314.56f, 542.10f,  6.09f, 0},
    {4315.82f, 573.50f, -3.03f, 0}, // Point before landing
    {4300.74f, 576.98f, -6.77f, 0}, // Land Point
    {4193.51f, 568.63f, 70.30f, 0},
    {4028.56f, 521.94f, 95.19f, 0},
    {3921.31f, 322.56f, 49.82f, 0},
    // 5th Drake
    {4295.91f, 492.10f, 17.80f, 0},
    {4329.42f, 536.24f,  5.95f, 0},
    {4319.26f, 580.12f, -3.24f, 0}, // Point before landing
    {4301.56f, 584.29f, -6.68f, 0}, // Land Point
    {4192.92f, 576.66f, 64.49f, 0},
    {4020.46f, 529.88f, 83.64f, 0},
    {3941.48f, 313.30f, 49.82f, 0},
    // Finish
    {3928.34f, 242.78f, 49.17f, 0},
};

// Life Wardens - 119511, 119517, 119518, 119519, 119520
class npc_life_warden_event_drake : public CreatureScript
{
public:
    npc_life_warden_event_drake() : CreatureScript("npc_life_warden_event_drake") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        pCreature->AI()->DoAction();
        return true;
    }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_life_warden_event_drake_AI(pCreature);
    }

    struct npc_life_warden_event_drake_AI : public ScriptedAI
    {
        npc_life_warden_event_drake_AI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Move_Timer;
        uint32 Move_Point;
        uint32 Action;
        uint32 Check_Timer;
        uint64 Guid;
        bool CanDie;

        void Reset()
        {
            me->SetVisible(false);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlying(true);
            Action = 0;
            Move_Timer = 1000;
            Check_Timer = 1000;
            CanDie = false;
        }

        void SpellHit(Unit* caster, const SpellEntry *spell)
        {
            if (spell->Id == 104230)
            {
                Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                    if (Player* player = i->getSource())
                    {
                        if (caster->GetDistance(player) < 55.0 && !player->HasAura(66131))
                        {
                            player->CastSpell(player, 66131, true); // Cast Parachute
                        }
                    }
                me->Kill(me);
            }
        }

        void DoAction(const int32 /*param*/)
        {
            Action = 5;
        }

        void UpdateAI(const uint32 diff)
        {
            if (instance)
            {
                if (instance->GetData(DATA_DRAKES) == 1)
                {
                    if (Move_Timer <= diff)
                    {
                        Move_Timer = 1000;

                        switch (Action)
                        {
                        case 0:
                            {
                                switch (me->GetEntry())
                                {
                                case 119511:
                                    Move_Point = 0;
                                    Move_Timer = 4800;
                                    break;
                                case 119517:
                                    Move_Point = 7;
                                    Move_Timer = 4800;
                                    break;
                                case 119518:
                                    Move_Point = 14;
                                    Move_Timer = 4800;
                                    break;
                                case 119519:
                                    Move_Point = 21;
                                    Move_Timer = 5000;
                                    break;
                                case 119520:
                                    Move_Point = 28;
                                    Move_Timer = 5000;
                                    break;
                                default:
                                    break;
                                }
                                me->SetVisible(true);
                                break;
                            }
                            break;
                        case 1:
                            {
                                switch (me->GetEntry())
                                {
                                case 119511:
                                    Move_Timer = 2700;
                                    break;
                                case 119517:
                                    Move_Timer = 2800;
                                    break;
                                case 119518:
                                    Move_Timer = 3000;
                                    break;
                                case 119519:
                                case 119520:
                                    Move_Timer = 3500;
                                    break;
                                default:
                                    break;
                                }
                            }
                            break;
                        case 2:
                            {
                                switch (me->GetEntry())
                                {
                                case 119511:
                                    Move_Timer = 1500;
                                    break;
                                case 119517:
                                    Move_Timer = 1800;
                                    break;
                                case 119518:
                                case 119519:
                                    Move_Timer = 2000;
                                    break;
                                case 119520:
                                    Move_Timer = 3000;
                                    break;
                                default:
                                    break;
                                }
                            }
                            break;
                        case 3:
                            Move_Timer = 1000;
                            break;
                        case 4:
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            break;
                        case 5:
                            {
                                me->SetSpeed(MOVE_FLIGHT, 4.0f, true);

                                switch (me->GetEntry())
                                {
                                case 119511:
                                    Move_Timer = 4200;
                                    break;
                                case 119517:
                                case 119518:
                                    Move_Timer = 4300;
                                    break;
                                case 119519:
                                case 119520:
                                    Move_Timer = 4500;
                                    break;
                                default:
                                    break;
                                }
                            }
                            break;
                        case 6:
                            {
                                switch (me->GetEntry())
                                {
                                case 119511:
                                    Move_Timer = 5000;
                                    break;
                                case 119517:
                                    Move_Timer = 5300;
                                    break;
                                case 119518:
                                    Move_Timer = 5600;
                                    break;
                                case 119519:
                                    Move_Timer = 6000;
                                    break;
                                case 119520:
                                    Move_Timer = 5000;
                                    break;
                                default:
                                    break;
                                }
                            }
                            break;
                        case 7:
                            {
                                switch (me->GetEntry())
                                {
                                case 119511:
                                case 119517:
                                case 119518:
                                    Move_Timer = 7500;
                                    break;
                                case 119519:
                                    Move_Timer = 8000;
                                    break;
                                case 119520:
                                    Move_Timer = 8300;
                                    break;
                                default:
                                    break;
                                }
                            }
                            break;
                        case 8:
                            Move_Timer = 3000;
                            Check_Timer = 500;
                            CanDie = true;
                            Move_Point = 35;
                            break;
                        default:
                            break;
                        }

                        if (Action != 4 && Action < 9)
                        {
                            me->GetMotionMaster()->MovePoint(0, DrakePoint[Move_Point], true);
                            Move_Point++;
                            Action++;
                        }
                    }
                    else Move_Timer -= diff;

                    if (CanDie)
                    {
                        if (Check_Timer <= diff)
                        {
                            Creature * twilight_drake = me->FindNearestCreature(55636, 80.0f, true);
                            if (twilight_drake)
                            {
                                twilight_drake->CastSpell(me, 104230, false); // Cast Twilight Breath
                                CanDie = false;
                            }
                            Check_Timer = 500;
                        }
                        else Check_Timer -= diff;
                    }
                }
            }
        }

    };
};

///////////////////////////
/// Time-transit Device ///
///////////////////////////

// Time Transit Device
#define ASIRA                 "Teleport to Galakrond`s Rest"      // Asira Dawnslayer
#define BENEDICTUS            "Teleport to Path of the Titans"    // Archbishop Benedictus

class go_hot_time_transit_device : public GameObjectScript
{
public:
    go_hot_time_transit_device() : GameObjectScript("go_hot_time_transit_device") { }

    bool OnGossipSelect(Player* pPlayer, GameObject* pGo, uint32 uiSender, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        switch (uiSender)
        {
            case GOSSIP_SENDER_MAIN:    SendActionMenu(pPlayer, pGo, uiAction); break;
        }
        return true;
    }

    bool OnGossipHello(Player* pPlayer, GameObject* pGo)
    {
        InstanceScript *pInstance = pGo->GetInstanceScript();
        if (((pInstance->GetData(TYPE_BOSS_ARCURION)) == DONE) && ((pInstance->GetData(TYPE_BOSS_ASIRA_DAWNSLAYER)) != DONE))
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ASIRA, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        }

        if ((pInstance->GetData(TYPE_BOSS_ASIRA_DAWNSLAYER)) == DONE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ASIRA, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BENEDICTUS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
        }

        pPlayer->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, pGo->GetGUID());
        return true;
    }

    void SendActionMenu(Player* pPlayer, GameObject* /*pGo*/, uint32 uiAction)
    {
        switch (uiAction)
        {
        case GOSSIP_ACTION_INFO_DEF + 1:
            pPlayer->TeleportTo(940, 4416.76f, 453.371f, 38.554f, 3.90f);
            pPlayer->CLOSE_GOSSIP_MENU();
            break;
        case GOSSIP_ACTION_INFO_DEF + 2:
            pPlayer->TeleportTo(940, 3932.045f, 303.857f, 12.63f, 3.16f);
            pPlayer->CLOSE_GOSSIP_MENU();
            break;
        }
    }
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
    new npc_life_warden_event_drake();

    new npc_shadow_borer();
    new npc_faceless_brute();
    new npc_faceless_shadow_weaver();
    new npc_fog();

    new go_hot_time_transit_device();
}