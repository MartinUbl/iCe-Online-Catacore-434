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

const float ThrallMovePoints[20][4]=
{
    // Arcurion Thrall
    {4924.10f, 255.55f, 97.1204f, 2.132f}, // After two Frozen Servitors
    {4897.53f, 212.83f, 99.3030f, 4.155f}, // In the middle of the canyon
    {4867.65f, 162.53f, 98.0762f, 3.968f}, // Middle way in fron of Arcurion
    {4786.66f,  78.23f, 70.7800f, 3.847f}, // Stop before Arcurion
    {4762.56f,  61.55f, 66.3397f, 3.725f}, // Skywall
    // Ghost wolf Thrall
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
    // Asira Thrall
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
                case 55779: // Ghost wolf Thrall
                    thrall1Guid = pCreature->GetGUID();
                    break;
                case 54972: // Asira`s Thrall
                    thrall2Guid = pCreature->GetGUID();
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
    ARCURION_SPAWN_VISUAL          = 104767,
    GHOST_WOLF_FORM                = 2645,
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
                                instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 16-24
                            }
                            break;
                        case 25:
                            if (me->GetExactDist2d(ThrallMovePoints[14][0], ThrallMovePoints[14][1]) < 1)
                            {
                                me->SetVisible(false);
                                Creature * thrall_next = me->FindNearestCreature(THRALL_2, 50.0f, true);
                                if (thrall_next)
                                    thrall_next->SetVisible(true);
                                instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 25
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
        }

        InstanceScript* instance;

        void Reset() { }

        void EnterCombat(Unit * /*who*/) { }

        void UpdateAI(const uint32 diff)
        {
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


void AddSC_instance_hour_of_twilight()
{
    new instance_hour_of_twilight();

    new npc_thrall_hour_of_twilight();
    new npc_thrall_hour_of_twilight1();
    new npc_thrall_hour_of_twilight2();

    new npc_frozen_servitor();
    new npc_crystalline_elemental();
    new npc_frozen_shard();
}