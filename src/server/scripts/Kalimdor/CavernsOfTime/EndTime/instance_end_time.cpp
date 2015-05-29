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
#include "endtime.h"

enum EndTimeWorldStates
{
    WORLD_STATE_FRAGMENTS                  = 6046,
    WORLD_STATE_FRAGMENTS_COUNT            = 6025,
};


class instance_end_time: public InstanceMapScript
{
public:
    instance_end_time() : InstanceMapScript("instance_end_time", 938) { }

    struct instance_end_time_InstanceMapScript : public InstanceScript
    {
        instance_end_time_InstanceMapScript(Map* pMap) : InstanceScript(pMap) {Initialize();}

        //InstanceScript* instance;
        uint32 m_auiEncounter[MAX_ENCOUNTER];
        uint32 currEnc[MAX_ENCOUNTER];

        uint32 crystals_clicked;
        uint32 trash_murozond;
        uint32 trash_baine;
        uint32 trash_tyrande;
        uint32 echo_killed;

        uint64 echo_of_sylvanasGuid;
        uint64 echo_of_jainaGuid;
        uint64 echo_of_baineGuid;
        uint64 echo_of_tyrandeGuid;
        uint64 murozondGuid;

        uint64 pool_of_moonlight_1_guid;
        uint64 pool_of_moonlight_2_guid;
        uint64 pool_of_moonlight_3_guid;
        uint64 pool_of_moonlight_4_guid;
        uint64 pool_of_moonlight_5_guid;

        uint32 Check_Timer;
        uint32 Pool_Check_Timer;
        bool Jaina_Welcome_Say;
        float Size;

        std::string saveData;

        void Initialize()
        {
            echo_of_sylvanasGuid = 0;
            echo_of_jainaGuid = 0;
            echo_of_baineGuid = 0;
            echo_of_tyrandeGuid = 0;
            murozondGuid = 0;
            echo_killed = 0;

            pool_of_moonlight_1_guid = 0;
            pool_of_moonlight_2_guid = 0;
            pool_of_moonlight_3_guid = 0;
            pool_of_moonlight_4_guid = 0;
            pool_of_moonlight_5_guid = 0;

            Check_Timer = 15000;
            Pool_Check_Timer = 1000;

            crystals_clicked = 0;
            trash_murozond = 0;
            trash_baine = 0;
            trash_tyrande = 0;
            Jaina_Welcome_Say = false;

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

            saveStream << " " << crystals_clicked;
            saveStream << " " << trash_murozond;
            saveStream << " " << trash_baine;
            saveStream << " " << trash_tyrande;
            saveStream << " " << echo_killed;

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

            loadStream >> crystals_clicked;
            loadStream >> trash_murozond;
            loadStream >> trash_baine;
            loadStream >> trash_tyrande;
            loadStream >> echo_killed;

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
                case 54123:
                    echo_of_sylvanasGuid = pCreature->GetGUID();
                    break;
                case 54445:
                    echo_of_jainaGuid = pCreature->GetGUID();
                    break;
                case 54431:
                    echo_of_baineGuid = pCreature->GetGUID();
                    break;
                case 54544:
                    echo_of_tyrandeGuid = pCreature->GetGUID();
                    break;
                case 54432:
                    murozondGuid = pCreature->GetGUID();
                    break;
                case 119503:
                    pool_of_moonlight_1_guid = pCreature->GetGUID();
                    break;
                case 119504:
                    pool_of_moonlight_2_guid = pCreature->GetGUID();
                    break;
                case 119505:
                    pool_of_moonlight_3_guid = pCreature->GetGUID();
                    break;
                case 119506:
                    pool_of_moonlight_4_guid = pCreature->GetGUID();
                    break;
                case 119507:
                    pool_of_moonlight_5_guid = pCreature->GetGUID();
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go, bool add) { }

        uint64 GetData64(uint32 type)
        {
            switch (type)
            {
                case TYPE_ECHO_OF_SYLVANAS:
                    return echo_of_sylvanasGuid;
                case TYPE_ECHO_OF_JAINA:
                    return echo_of_jainaGuid;
                case TYPE_ECHO_OF_BAINE:
                    return echo_of_baineGuid;
                case TYPE_ECHO_OF_TYRANDE:
                    return echo_of_tyrandeGuid;
                case TYPE_MUROZOND:
                    return murozondGuid;
                case TYPE_POOL_OF_MOONLIGHT_1:
                    return pool_of_moonlight_1_guid;
                case TYPE_POOL_OF_MOONLIGHT_2:
                    return pool_of_moonlight_2_guid;
                case TYPE_POOL_OF_MOONLIGHT_3:
                    return pool_of_moonlight_3_guid;
                case TYPE_POOL_OF_MOONLIGHT_4:
                    return pool_of_moonlight_4_guid;
                case TYPE_POOL_OF_MOONLIGHT_5:
                    return pool_of_moonlight_5_guid;
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
                if (Jaina_Welcome_Say == false)
                {
                    if (crystals_clicked >= 16)
                    {
                        DoUpdateWorldState(WORLD_STATE_FRAGMENTS, 0);

                        Creature * echo_of_jaina = this->instance->GetCreature(this->GetData64(TYPE_ECHO_OF_JAINA));
                        if (echo_of_jaina)
                        {
                            echo_of_jaina->SetVisible(true);
                            echo_of_jaina->setFaction(16);
                            echo_of_jaina->MonsterSay("I don't know who you are, but I'll defend this shrine with my life. Leave, now, before we come to blows.", LANG_UNIVERSAL, echo_of_jaina->GetGUID(), 150.0f);
                            echo_of_jaina->SendPlaySound(25920, true);
                            Jaina_Welcome_Say = true;
                        }
                    }
                }

                if (trash_murozond == 1)
                {
                    Creature * boss_murozond = this->instance->GetCreature(this->GetData64(TYPE_MUROZOND));
                    if (boss_murozond && boss_murozond->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE) 
                        && boss_murozond->IsAlive())
                    {
                        boss_murozond->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);
                        boss_murozond->SetReactState(REACT_AGGRESSIVE);
                        boss_murozond->GetMotionMaster()->MovePoint(0, 4170.6196f, -428.5562f, 144.295914f);
                        boss_murozond->SendMovementFlagUpdate();
                        boss_murozond->SetVisible(true);
                    }
                }

                if (trash_baine == 1)
                {
                    Creature * echo_of_baine = this->instance->GetCreature(this->GetData64(TYPE_ECHO_OF_BAINE));
                    if (echo_of_baine && echo_of_baine->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                    {
                        echo_of_baine->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);
                        echo_of_baine->SetReactState(REACT_AGGRESSIVE);
                    }
                }

                if (trash_tyrande >= 50)
                {
                    Creature * echo_of_tyrande = this->instance->GetCreature(this->GetData64(TYPE_ECHO_OF_TYRANDE));
                    if (echo_of_tyrande && echo_of_tyrande->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                    {
                        echo_of_tyrande->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);
                        echo_of_tyrande->SetReactState(REACT_AGGRESSIVE);
                        echo_of_tyrande->RemoveAura(101841); // In Shadow Aura
                    }
                }

                Check_Timer = 15000;
            }
            else Check_Timer -= diff;

            if (trash_tyrande > 0)
            {
                if (Pool_Check_Timer <= diff)
                {
                    Creature * pool_of_moonlight_1 = this->instance->GetCreature(this->GetData64(TYPE_POOL_OF_MOONLIGHT_1));
                    Creature * pool_of_moonlight_2 = this->instance->GetCreature(this->GetData64(TYPE_POOL_OF_MOONLIGHT_2));
                    Creature * pool_of_moonlight_3 = this->instance->GetCreature(this->GetData64(TYPE_POOL_OF_MOONLIGHT_3));
                    Creature * pool_of_moonlight_4 = this->instance->GetCreature(this->GetData64(TYPE_POOL_OF_MOONLIGHT_4));
                    Creature * pool_of_moonlight_5 = this->instance->GetCreature(this->GetData64(TYPE_POOL_OF_MOONLIGHT_5));

                    if (trash_tyrande <= 10)
                    {
                        Size = 2-(trash_tyrande*0.14);
                        if (pool_of_moonlight_1)
                            pool_of_moonlight_1->SetFloatValue(OBJECT_FIELD_SCALE_X, Size);
                    }

                    if (trash_tyrande > 10)
                    {
                        if (pool_of_moonlight_1 && pool_of_moonlight_1->IsAlive())
                        {
                            pool_of_moonlight_1->Kill(pool_of_moonlight_1);
                        }
                        else 
                        {
                            if (pool_of_moonlight_2 && !pool_of_moonlight_2->HasAura(102479))
                                pool_of_moonlight_2->CastSpell(pool_of_moonlight_2, 102479, true);
                        }

                        if (trash_tyrande <= 20)
                        {
                            Size = 2-((trash_tyrande-10)*0.14);
                            if (pool_of_moonlight_2)
                                pool_of_moonlight_2->SetFloatValue(OBJECT_FIELD_SCALE_X, Size);
                        }
                    }

                    if (trash_tyrande > 20)
                    {
                        if (pool_of_moonlight_2 && pool_of_moonlight_2->IsAlive())
                        {
                            pool_of_moonlight_2->Kill(pool_of_moonlight_2);
                        }
                        else
                        {
                            if (pool_of_moonlight_3 && !pool_of_moonlight_3->HasAura(102479))
                                pool_of_moonlight_3->CastSpell(pool_of_moonlight_3, 102479, true);
                        }

                        if (trash_tyrande <= 30)
                        {
                            Size = 2-((trash_tyrande-20)*0.14);
                            if (pool_of_moonlight_3)
                                pool_of_moonlight_3->SetFloatValue(OBJECT_FIELD_SCALE_X, Size);
                        }
                    }

                    if (trash_tyrande > 30)
                    {
                        if (pool_of_moonlight_3 && pool_of_moonlight_3->IsAlive())
                        {
                            pool_of_moonlight_3->Kill(pool_of_moonlight_3);
                        }
                        else 
                        {
                            if (pool_of_moonlight_4 && !pool_of_moonlight_4->HasAura(102479))
                                pool_of_moonlight_4->CastSpell(pool_of_moonlight_4, 102479, true);
                        }

                        if (trash_tyrande <= 40)
                        {
                            Size = 2-((trash_tyrande-30)*0.14);
                            if (pool_of_moonlight_4)
                                pool_of_moonlight_4->SetFloatValue(OBJECT_FIELD_SCALE_X, Size);
                        }
                    }

                    if (trash_tyrande > 40)
                    {
                        if (pool_of_moonlight_4 && pool_of_moonlight_4->IsAlive())
                        {
                            pool_of_moonlight_4->Kill(pool_of_moonlight_4);
                        }
                        else 
                        {
                            if (pool_of_moonlight_5 && !pool_of_moonlight_5->HasAura(102479))
                                pool_of_moonlight_5->CastSpell(pool_of_moonlight_5, 102479, true);
                        }

                        if (trash_tyrande <= 50)
                        {
                            Size = 2-((trash_tyrande-40)*0.14);
                            if (pool_of_moonlight_5)
                                pool_of_moonlight_5->SetFloatValue(OBJECT_FIELD_SCALE_X, Size);
                        }
                    }

                    if (trash_tyrande > 50)
                    {
                        if (pool_of_moonlight_5 && pool_of_moonlight_5->IsAlive())
                        {
                            pool_of_moonlight_5->Kill(pool_of_moonlight_5);
                        }
                    }

                    Pool_Check_Timer = 1000;
                }
                else Pool_Check_Timer -= diff;
            }
        }

        uint32 GetData(uint32 DataId) 
        {
            if (DataId < MAX_ENCOUNTER)
                return m_auiEncounter[DataId];

            if (DataId == DATA_CRYSTALS)
                return crystals_clicked;

            if (DataId == DATA_TRASH_MUROZOND)
                return trash_murozond;

            if (DataId == DATA_TRASH_BAINE)
                return trash_baine;

            if (DataId == DATA_TRASH_TYRANDE)
                return trash_tyrande;

            if (DataId == DATA_ECHO_KILLED)
                return echo_killed;

            return 0;
        }

        void SetData(uint32 type, uint32 data)
        {
            if (type < MAX_ENCOUNTER)
                m_auiEncounter[type] = data;

            if (type == DATA_CRYSTALS)
            {
                crystals_clicked += data;
                DoUpdateWorldState(WORLD_STATE_FRAGMENTS_COUNT, crystals_clicked);
                SaveToDB();
            }

            if (type == DATA_TRASH_MUROZOND)
            {
                trash_murozond = data;
                SaveToDB();
            }

            if (type == DATA_TRASH_BAINE)
            {
                trash_baine = data;
                SaveToDB();
            }

            if (type == DATA_TRASH_TYRANDE)
            {
                trash_tyrande += data;
                SaveToDB();
            }

            if (type == DATA_ECHO_KILLED)
            {
                echo_killed += data;
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
            currEnc[0] = m_auiEncounter[TYPE_MUROZOND]; // Murozond  
            currEnc[1] = m_auiEncounter[TYPE_ECHO_OF_SYLVANAS]; // Sylvanas
            currEnc[2] = m_auiEncounter[TYPE_ECHO_OF_JAINA]; // Jaina 
            currEnc[3] = m_auiEncounter[TYPE_ECHO_OF_BAINE]; // Baine
            currEnc[4] = m_auiEncounter[TYPE_ECHO_OF_TYRANDE]; // Tyrande

            sInstanceSaveMgr->setInstanceSaveData(instance->GetInstanceId(), currEnc, MAX_ENCOUNTER);

            return NULL;
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap* pMap) const
    {
        return new instance_end_time_InstanceMapScript(pMap);
    }
};

////////////////////////////////
///// Other Instance Stuff /////
////////////////////////////////

// Time Transit Device
#define ENTRANCE           "Teleport to Entrance"                // Entrance
#define SYLVANAS           "Teleport to Ruby Dragonshrine"       // Sylvanas
#define TYRANDE            "Teleport to Emerald Dragonshrine"    // Tyrande
#define JAINA              "Teleport to Azure Dragonshrine"      // Jaina
#define BAINE              "Teleport to Obsidian Dragonshrine"   // Baine
#define MUROZOND           "Teleport to Bronze Dragonshrine"     // Murozond

// Spells
enum Teleport_End_Time
{
    SPELL_ENTRANCE        = 102564,
    SPELL_SYLVANAS        = 102579,
    SPELL_JAINA           = 102126,
    SPELL_BAINE           = 103868,
    SPELL_TYRANDE         = 104761,
    SPELL_MUROZOND        = 104764,
};

class go_time_transit_device : public GameObjectScript
{
public:
    go_time_transit_device() : GameObjectScript("go_time_transit_device") { }

    bool OnGossipSelect(Player* pPlayer, GameObject* pGo, uint32 uiSender, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        switch(uiSender)
        {
            case GOSSIP_SENDER_MAIN:    SendActionMenu(pPlayer, pGo, uiAction); break;
        }
        return true;
    }

    bool OnGossipHello(Player* pPlayer, GameObject* pGo)
    {
        switch (pGo->GetEntry())
        {
        case 209438: // Sylvanas Timer transit device spawned after her death
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ENTRANCE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            break;
        case 209439: // Sylvanas and Jaina
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ENTRANCE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, SYLVANAS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, JAINA, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
            break;
        case 209440: // Tyrande and Baine
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ENTRANCE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, TYRANDE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BAINE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
            break;
        case 209441: // Sylvanas and Baine
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ENTRANCE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, SYLVANAS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BAINE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
            break;
        case 209442: // Jaina and Tyrande
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ENTRANCE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, JAINA, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, TYRANDE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
            break;
        case 209998: // Sylvanas and Tyrande
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ENTRANCE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, SYLVANAS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, TYRANDE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
            break;
        case 210000: // Jaina and Baine
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ENTRANCE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, JAINA, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BAINE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
            break;
        default:
            break;
        }

        if (InstanceScript * instance = pGo->GetInstanceScript())
            if (instance->GetData(DATA_ECHO_KILLED) >= 2)
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, MUROZOND, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 6);

        pPlayer->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, pGo->GetGUID());
        return true;
    }

    void SendActionMenu(Player* pPlayer, GameObject* /*pGo*/, uint32 uiAction)
    {
        switch(uiAction)
        {
            case GOSSIP_ACTION_INFO_DEF + 1:
                pPlayer->CastSpell(pPlayer,SPELL_ENTRANCE,false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 2:
                  pPlayer->CastSpell(pPlayer,SPELL_SYLVANAS,false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 3:
                pPlayer->CastSpell(pPlayer,SPELL_JAINA,false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 4:
                pPlayer->CastSpell(pPlayer,SPELL_TYRANDE,false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 5:
                pPlayer->CastSpell(pPlayer,SPELL_BAINE,false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 6:
                pPlayer->CastSpell(pPlayer,SPELL_MUROZOND,false);
                break;
        }
    }
};

// Image of Nozdormu
static const uint32 Bosses[4] = {54123, 54445, 54431, 54544}; // Sylvanas, Jaina, Baine, Tyrande (in this order)

enum ScriptTexts
{
    // Neutral
    SAY_NEUTRAL          = -1999926, // 25959 - You must give peace to this lands if you are to face Murozond

    // Sylvanas Encounter
    SAY_SYLVANAS_1       = -1999927, // 25958 - This is where she stood, heroes, and this is where she fell. The time-lost echo of Sylvanas Windrunner will reverberate through the rotting limbs of the Dragonshrine for all eternity.

    // Baine Encounter
    SAY_BAINE_1          = -1999928, // 25956 - The undying flames are all that remain of this sacred place. I sense much anger here... a seething rage, barely held in check. Be on your guard.
    YELL_BAINE_1         =    25911, // 25911 - You! Are you the ones responsible for what has happened here...?

    // Tyrande Encounter
    SAY_TYRANDE_1        = -1999929, // 25955 - There is an unnatural darkness to this place, a perpetual midnight. Take caution, heroes, and do not stray from the light.

    // Jaina Encounter
    SAY_JAINA_1          = -1999930, // 25957 - This is all that is left of the Blue Dragonshrine. A great battle shattered the timeways leading out of this forsaken place. You must reconstruct the fragments strewn across the ground and defeat the trapped spirit to proceed.
    JAINA_INTRO          = -1999941, // 25920 - I don't know who you are, but I'll defend this shrine with my life. Leave, now, before we come to blows.

    // Murozond Encounter
    YELL_MUROZOND_1      =    25934, // 25934 - The "End Time," I once called this place, this strand. I had not seen, by then; I did not know. You hope to... what? Stop me, here? Change the fate I worked so tirelessly to weave?
    YELL_MUROZOND_2      =    25935, // 25935 - You crawl unwitting, like a blind, writhing worm, towards endless madness and despair. I have witnessed the true End Time. This? This is a blessing you simply cannot comprehend.
};

class npc_image_of_nozdormu : public CreatureScript
{
public:
    npc_image_of_nozdormu() : CreatureScript("npc_image_of_nozdormu") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_image_of_nozdormuAI (pCreature);
    }

    struct npc_image_of_nozdormuAI : public ScriptedAI
    {
        npc_image_of_nozdormuAI(Creature *creature) : ScriptedAI(creature) 
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Check_Timer;
        uint32 Say_Neutral_Check;
        uint32 Say_Next;
        uint32 Baine_Welcome_Yell_Timer;
        bool Sylvanas_Say;
        bool Jaina_Say;
        bool Tyrande_Say;
        bool Baine_Say;
        bool Image_Of_Nozdormu_Say;
        bool Baine_Welcome_Yell;

        void Reset()
        {
            me->CastSpell(me, 102602, true); // Visual aura

            Check_Timer = 10000;
            Say_Neutral_Check = 10000;

            Sylvanas_Say = false;
            Jaina_Say = false;
            Tyrande_Say = false;
            Baine_Say = false;
            Image_Of_Nozdormu_Say = false;
            Baine_Welcome_Yell = false;
        }

        void EnterCombat(Unit * /*who*/) { }

        void UpdateAI(const uint32 diff) 
        {
            if (Check_Timer <= diff)
            {
                float distance;
                int count;
                count = 0;
                Map::PlayerList const &playerList = me->GetMap()->GetPlayers();
                if (!playerList.isEmpty())
                    for (Map::PlayerList::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
                        if (Player* pPlayer = i->getSource())
                        {
                            distance = me->GetExactDist2d(pPlayer);
                            if (distance<50)
                                count = count+1;
                        }

                if (count >= 4) // Say only when at least 4 memebrs of the group are near Image of Nozdormu, so most of them can enjoy this speech :D
                {
                    Creature * boss = NULL;
                    Creature * new_boss = NULL;
                    for (int i = 0; i<4; i++)
                    {
                        new_boss = me->FindNearestCreature(Bosses[i], 250.0f, true);
                        if (new_boss)
                            boss = new_boss;
                    }
                    
                    if (boss)
                    {
                        int entry = boss->GetEntry();
                        switch(entry)
                        {
                            // Sylvanas
                            case(54123):
                                {
                                    if (Sylvanas_Say == false)
                                    {
                                        DoScriptText(SAY_SYLVANAS_1,me);
                                        me->SendPlaySound(25958, true);
                                        Sylvanas_Say = true;
                                        Say_Next = 17000;
                                        Image_Of_Nozdormu_Say = true;
                                    }
                                }
                                break;
                            // Jaina
                            case(54445):
                                {
                                    if (Jaina_Say == false)
                                    {
                                        DoScriptText(SAY_JAINA_1,me);
                                        me->SendPlaySound(25957, true);
                                        Jaina_Say = true;
                                        Say_Next = 19000;
                                        Image_Of_Nozdormu_Say = true;
                                    }
                                }
                                break;
                            // Baine
                            case(54431):
                                {
                                    if (Baine_Say == false)
                                    {
                                        DoScriptText(SAY_BAINE_1,me);
                                        me->SendPlaySound(25956, true);
                                        Baine_Say = true;
                                        Say_Next = 17000;
                                        Image_Of_Nozdormu_Say = true;
                                        Baine_Welcome_Yell_Timer = 32000;
                                        Baine_Welcome_Yell = true;
                                    }
                                }
                                break;
                            // Tyrande
                            case(54544):
                                {
                                    if (Tyrande_Say == false)
                                    {
                                        DoScriptText(SAY_TYRANDE_1,me);
                                        me->SendPlaySound(25955, true);
                                        Tyrande_Say = true;
                                        Say_Next = 14000;
                                        Image_Of_Nozdormu_Say = true;
                                    }
                                }
                                break;
                            }
                        }
                    }
                    Check_Timer = 10000;
                } else Check_Timer -= diff;

                if (Image_Of_Nozdormu_Say == true)
                {
                    if (Say_Next <= diff)
                    {
                        DoScriptText(SAY_NEUTRAL,me);
                        me->SendPlaySound(25959, true);
                        Image_Of_Nozdormu_Say = false;
                    }
                    else Say_Next -= diff;
                }

                if (Baine_Welcome_Yell == true)
                {
                    if (Baine_Welcome_Yell_Timer <= diff)
                    {
                        Creature * baine = me->FindNearestCreature(54431, 300.0f, true);
                        if (baine)
                        {
                            baine->MonsterYell("You! Are you the ones responsible for what has happened here...?", LANG_UNIVERSAL, 0);
                            baine->SendPlaySound(25911, false);
                        }
                        Baine_Welcome_Yell = false;
                    }
                    else Baine_Welcome_Yell_Timer -= diff;
                }
            }

    };
};

//////////////////////////////////////////////////////////////
////////////////        TRASH AI            //////////////////
//////////////////////////////////////////////////////////////

/////////////////////////
// Bronze Dragonshrine //
/////////////////////////
enum Creatures
{
    INFINITE_SUPPRESSOR           = 54920,
    INFINITE_WARDEN               = 54923,
    MUROZOND_BOSS                 = 54432,
};
enum BronzeDragonshrineSpells
{
    ARCANE_WAVE                   = 102601,
    TEMPORAL_VORTEX               = 102600,

    VOID_SHIELD                   = 102599,
    VOID_STRIKE                   = 102598,
};

// Infinite Suppressor
class npc_infinite_suppressor : public CreatureScript
{
public:
    npc_infinite_suppressor() : CreatureScript("npc_infinite_suppressor") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_infinite_suppressorAI (pCreature);
    }

    struct npc_infinite_suppressorAI : public ScriptedAI
    {
        npc_infinite_suppressorAI(Creature *c) : ScriptedAI(c) {}

        uint32 Arcane_Wave;
        uint32 Temporal_Vortex;

        void Reset()
        {
            Arcane_Wave = 2000;
            Temporal_Vortex = 15000+urand(0,10000);
        }

        void UpdateAI(const uint32 diff) 
        {
            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

            if (Arcane_Wave <= diff)
            {
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true);
                if (target)
                    me->CastSpell(target, ARCANE_WAVE, false);
                Arcane_Wave = 4000;
            }
            else Arcane_Wave -= diff;

            if (Temporal_Vortex <= diff)
            {
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true);
                if (target)
                    me->CastSpell(target, TEMPORAL_VORTEX, false);
                Temporal_Vortex = 15000;
            }
            else Temporal_Vortex -= diff;

            DoMeleeAttackIfReady();
        }

    };
};

// Infinite Warden
class npc_infinite_warden : public CreatureScript
{
public:
    npc_infinite_warden() : CreatureScript("npc_infinite_warden") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_infinite_wardenAI (pCreature);
    }

    struct npc_infinite_wardenAI : public ScriptedAI
    {
        npc_infinite_wardenAI(Creature *c) : ScriptedAI(c) { }

        uint32 Void_Shield;
        uint32 Void_Strike;

        void Reset()
        {
            me->RemoveAura(VOID_SHIELD);

            Void_Shield = 5000;
            Void_Strike = 10000+urand(0,7000);
        }

        void JustDied(Unit * /*who*/)
        {
            int count;
            count = 0;
            std::list<Creature*> Infinite_Wardens;
            GetCreatureListWithEntryInGrid(Infinite_Wardens, me, INFINITE_WARDEN, 250.0f);
            for (std::list<Creature*>::const_iterator itr = Infinite_Wardens.begin(); itr != Infinite_Wardens.end(); ++itr)
                if ((*itr) && (*itr)->IsAlive())
                    count += 1;

            if (count == 2)
            {
                Creature * murozond_boss = me->FindNearestCreature(MUROZOND_BOSS, 250.0, true);
                if (murozond_boss)
                {
                    murozond_boss->SetVisible(true);
                    murozond_boss->GetMotionMaster()->MovePoint(0, 4170.6196f, -428.5562f, 144.295914f);
                    murozond_boss->SendMovementFlagUpdate();
                    murozond_boss->MonsterYell("The \"End Time\", I once called this place, this strand. I had not seen, by then; I did not know. You hope to... what? Stop me, here? Change the fate I worked so tirelessly to weave?", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(25934, false);
                }
            }

            if (count == 0)
            {
                Creature * murozond_boss = me->FindNearestCreature(MUROZOND_BOSS, 250.0, true);
                if (murozond_boss)
                {
                    murozond_boss->GetMotionMaster()->MovePoint(0, 4170.6196f, -428.5562f, 144.295914f);
                    murozond_boss->SendMovementFlagUpdate();
                    murozond_boss->MonsterYell("You crawl unwitting, like a blind, writhing worm, towards endless madness and despair. I have witnessed the true End Time. This? This is a blessing you simply cannot comprehend.", LANG_UNIVERSAL, 0);
                    murozond_boss->SendPlaySound(25935, false);

                    if (InstanceScript *pInstance = me->GetInstanceScript())
                        pInstance->SetData(DATA_TRASH_MUROZOND, 1);
                }
            }
        }

        void UpdateAI(const uint32 diff) 
        {
            if (!UpdateVictim()) 
                return;

            if (Void_Shield <= diff)
            {
                me->CastSpell(me, VOID_SHIELD, true);
                Void_Shield = 16000;
            }
            else Void_Shield -= diff;

            if (Void_Strike <= diff)
            {
                Unit * target = me->GetVictim();
                if (target)
                    me->CastSpell(target, VOID_STRIKE, false);
                Void_Strike = 10000+urand(0,3000);
            }
            else Void_Strike -= diff;

            DoMeleeAttackIfReady();
        }

    };
};

///////////////////////////
// Obsidian Dragonshrine //
///////////////////////////
enum CreaturesObsidian
{
    TIME_TWISTED_SEER             = 54553,
    TIME_TWISTED_BREAKER          = 54552,
    TIME_TWISTED_DRAKE            = 54543,

    RUPTURED_GROUND_NPC           = 54566,
    CALL_FLAMES_NPC               = 54585,
};

enum ObsidianDragonshrineSpells
{
    SEAR_FLESH                    = 102158,
    CALL_FLAMES                   = 102156,
    UNDYING_FLAME                 = 105238,

    BREAK_ARMOR                   = 102132,
    RUPTURE_GROUND                = 102124,
    RUPTURED_GROUND               = 102128,

    ENRAGE                        = 102134,
    FLAME_BREATH                  = 102135,
};

// Time-Twisted Seer
class npc_time_twisted_seer : public CreatureScript
{
public:
    npc_time_twisted_seer() : CreatureScript("npc_time_twisted_seer") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_time_twisted_seerAI (pCreature);
    }

    struct npc_time_twisted_seerAI : public ScriptedAI
    {
        npc_time_twisted_seerAI(Creature *c) : ScriptedAI(c) {}

        uint32 Sear_Flesh;
        uint32 Call_Flames;

        void Reset() 
        {
            Sear_Flesh = 8000;
            Call_Flames = 15000;
        }

        void UpdateAI(const uint32 diff) 
        {
            if (!UpdateVictim())
                return;

            if (Sear_Flesh <= diff)
            {
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true);
                    if (target)
                        me->CastSpell(target, SEAR_FLESH, false);
                Sear_Flesh = 10000;
            }
            else Sear_Flesh -= diff;

            if (Call_Flames <= diff)
            {
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
                me->CastSpell(me, CALL_FLAMES, false);
                Call_Flames = 15000;
            }
            else Call_Flames -= diff;

            DoMeleeAttackIfReady();
        }

    };
};

// Time-Twisted Breaker
class npc_time_twisted_breaker : public CreatureScript
{
public:
    npc_time_twisted_breaker() : CreatureScript("npc_time_twisted_breaker") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_time_twisted_breakerAI (pCreature);
    }

    struct npc_time_twisted_breakerAI : public ScriptedAI
    {
        npc_time_twisted_breakerAI(Creature *c) : ScriptedAI(c) {}

        uint32 Break_Armor;
        uint32 Rupture_Ground;

        void Reset() 
        {
            Break_Armor = 5000;
            Rupture_Ground = 10000;
        }

        void UpdateAI(const uint32 diff) 
        {
            if (!UpdateVictim())
                return;

            if (Break_Armor <= diff)
            {
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
                me->CastSpell(me, BREAK_ARMOR, false);
                Break_Armor = 15000;
            }
            else Break_Armor -= diff;

            if (Rupture_Ground <= diff)
            {
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
                me->CastSpell(me, RUPTURE_GROUND, false);
                Rupture_Ground = 20000;
            }
            else Rupture_Ground -= diff;

            DoMeleeAttackIfReady();
        }

    };
};

// Time-Twisted Drake
class npc_time_twisted_drake : public CreatureScript
{
public:
    npc_time_twisted_drake() : CreatureScript("npc_time_twisted_drake") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_time_twisted_drakeAI (pCreature);
    }

    struct npc_time_twisted_drakeAI : public ScriptedAI
    {
        npc_time_twisted_drakeAI(Creature *c) : ScriptedAI(c) {}

        uint32 Flame_Breath;
        uint32 Enrage;

        void Reset() 
        {
            Flame_Breath = 10000;
            Enrage = 15000;
        }

        void JustDied(Unit * /*who*/)
        {
            int count;
            count = 0;
            std::list<Creature*> Time_twisted_drakes;
            GetCreatureListWithEntryInGrid(Time_twisted_drakes, me, TIME_TWISTED_DRAKE, 150.0f);
            for (std::list<Creature*>::const_iterator itr = Time_twisted_drakes.begin(); itr != Time_twisted_drakes.end(); ++itr)
                if ((*itr) && (*itr)->IsAlive())
                    count += 1;

            if (count == 0)
            {
                if (InstanceScript *pInstance = me->GetInstanceScript())
                    pInstance->SetData(DATA_TRASH_BAINE, 1);
            }
        }

        void UpdateAI(const uint32 diff) 
        {
            if (!UpdateVictim())
                return;

            if (Enrage <= diff)
            {
                me->CastSpell(me, ENRAGE, false);
                Enrage = 30000;
            }
            else Enrage -= diff;

            if (Flame_Breath <= diff)
            {
                me->CastSpell(me, FLAME_BREATH, false);
                Flame_Breath = 14000;
            }
            else Flame_Breath -= diff;

            DoMeleeAttackIfReady();
        }

    };
};

// Ruptured Ground
class npc_ruptured_ground : public CreatureScript
{
public:
    npc_ruptured_ground() : CreatureScript("npc_ruptured_ground") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ruptured_groundAI (pCreature);
    }

    struct npc_ruptured_groundAI : public ScriptedAI
    {
        npc_ruptured_groundAI(Creature *c) : ScriptedAI(c) {}

        void Reset() 
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->CastSpell(me, RUPTURED_GROUND, false);
        }

    };
};

// Call Flames
class npc_call_flames : public CreatureScript
{
public:
    npc_call_flames() : CreatureScript("npc_call_flames") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_call_flamesAI (pCreature);
    }

    struct npc_call_flamesAI : public ScriptedAI
    {
        npc_call_flamesAI(Creature *c) : ScriptedAI(c) {}

        uint32 Undying_Flame;
        int count;

        void Reset() 
        {
            Undying_Flame = 500;
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);
            count = 0;
        }

        void UpdateAI(const uint32 diff) 
        {
            if (Undying_Flame <= diff)
            {
                me->CastSpell(me, UNDYING_FLAME, false);
                count = count + 1;

                if (count == 4)
                    me->Kill(me); 
                Undying_Flame = 1000;
            } 
            else Undying_Flame -= diff;
        }
    };
};

///////////////////////////
//   Ruby Dragonshrine   //
///////////////////////////
enum CreaturesRuby
{
    TIME_TWISTED_SCOURGE_BEAST    = 54507,
    TIME_TWISTED_GEIST            = 54511,
};

enum RubyDragonshrineSpells
{
    PUTRID_SPIT                   = 102063,
    FACE_KICK                     = 101888,
    WAIL_OF_THE_DEAD              = 101891,

    FLESH_RIP                     = 102066,
    CADAVER_TOSS                  = 109952,
    CANNIBALIZE                   = 109944,
};

// Time-Twisted Scourge Beast
class npc_time_twisted_scourge_beast : public CreatureScript
{
public:
    npc_time_twisted_scourge_beast() : CreatureScript("npc_time_twisted_scourge_beast") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_time_twisted_scourge_beastAI (pCreature);
    }

    struct npc_time_twisted_scourge_beastAI : public ScriptedAI
    {
        npc_time_twisted_scourge_beastAI(Creature *c) : ScriptedAI(c) {}

        uint32 Putrid_Spit;
        uint32 Face_Kick;
        uint32 Wail_Of_The_Dead;

        void Reset() 
        {
            Putrid_Spit = 7000+urand(0,5000);
            Face_Kick = 8000;
            Wail_Of_The_Dead = 12000;
        }

        void JustDied(Unit * /*who*/)
        {
            std::list<Creature*> Time_Twisted_Geists;
            GetCreatureListWithEntryInGrid(Time_Twisted_Geists, me, TIME_TWISTED_GEIST, 10.0f);
            for (std::list<Creature*>::const_iterator itr = Time_Twisted_Geists.begin(); itr != Time_Twisted_Geists.end(); ++itr)
                if (*itr)
                   (*itr)->CastSpell((*itr), CANNIBALIZE, true);
        }

        void UpdateAI(const uint32 diff) 
        {
            if (!UpdateVictim())
                return;

            if (Putrid_Spit <= diff)
            {
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true);
                if (target)
                    me->CastSpell(target, PUTRID_SPIT, true);
                Putrid_Spit = 7000+urand(0,5000);
            }
            else Putrid_Spit -= diff;

            if (Face_Kick <= diff)
            {
                me->CastSpell(me, FACE_KICK, true);
                Face_Kick = 10000+urand(0,5000);
            }
            else Face_Kick -= diff;

            if (Wail_Of_The_Dead <= diff)
            {
                me->CastSpell(me, WAIL_OF_THE_DEAD, true);
                Wail_Of_The_Dead = 45000;
            }
            else Wail_Of_The_Dead -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

// Time-Twisted Geist
class npc_time_twisted_geist : public CreatureScript
{
public:
    npc_time_twisted_geist() : CreatureScript("npc_time_twisted_geist") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_time_twisted_geistAI (pCreature);
    }

    struct npc_time_twisted_geistAI : public ScriptedAI
    {
        npc_time_twisted_geistAI(Creature *c) : ScriptedAI(c) {}

        uint32 Flesh_Rip;
        uint32 Cadaver_Toss;

        void Reset() 
        {
            Flesh_Rip = 10000+urand(0,10000);
            Cadaver_Toss = 5000+urand(0,35000);
        }

        void UpdateAI(const uint32 diff) 
        {
            if (!UpdateVictim())
                return;

            if (Flesh_Rip <= diff)
            {
                Unit * target = me->GetVictim();
                if (target)
                    me->CastSpell(target, FLESH_RIP, false);
                Flesh_Rip = 20000;
            }
            else Flesh_Rip -= diff;

            if (Cadaver_Toss <= diff)
            {
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true);
                if (target)
                    me->CastSpell(target, CADAVER_TOSS, true);
                Cadaver_Toss = 30000;
            }
            else Cadaver_Toss -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

//////////////////////////
// Emerald Dragonshrine //
//////////////////////////
enum CreaturesEmerald
{
    TIME_TWISTED_HUNTRESS         = 54701,
    TIME_TWISTED_NIGHT_SABER      = 54688,
    TIME_TWISTED_NIGHT_SABER2     = 54699,
    TIME_TWISTED_NIGHT_SABER3     = 54700,
    TIME_TWISTED_SENTINEL         = 54512,
};

enum EmeraldDragonshrineSpells
{
    IN_SHADOW_TRASH               = 101841,
    MOONLIT_TRASH                 = 101842,
    MOONLIGHT_TRASH               = 102479,
};

// Pools of Moonlight
static const uint32 Pools[5] = {119503, 119504, 119505, 119506, 119507};

// All npcs in Emerald Dragonshrine
class npc_time_twisted_emerald_trash : public CreatureScript
{
public:
    npc_time_twisted_emerald_trash() : CreatureScript("npc_time_twisted_emerald_trash") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_time_twisted_emerald_trashAI (pCreature);
    }

    struct npc_time_twisted_emerald_trashAI : public ScriptedAI
    {
        npc_time_twisted_emerald_trashAI(Creature *creature) : ScriptedAI(creature) 
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Check_Timer;

        void Reset() 
        {
            me->CastSpell(me, IN_SHADOW_TRASH, true);
            Check_Timer = 1000;
        }

        void JustDied(Unit * /*who*/)
        { 
            if (InstanceScript *pInstance = me->GetInstanceScript())
                        pInstance->SetData(DATA_TRASH_TYRANDE, 1);
        }

        void DamageDealt(Unit* victim, uint32& /*damage*/, DamageEffectType /*typeOfDamage*/)
        {
            if (victim->HasAura(102491)) // Tyrande Achievement Tracker
            {
                victim->RemoveAura(102491);
            }
        }

        void UpdateAI(const uint32 diff) 
        {
            if (!UpdateVictim())
                return;

            if (me->HasAura(IN_SHADOW_TRASH))
            {
                if (Check_Timer <= diff)
                {
                    Creature * pool_of_moonlight = NULL;
                    for (int i = 0; i < 5; i++)
                    {
                        pool_of_moonlight = me->FindNearestCreature(Pools[i], 5.0f, true);
                        if (pool_of_moonlight && pool_of_moonlight->HasAura(MOONLIGHT_TRASH))
                        {
                            me->RemoveAura(IN_SHADOW_TRASH);
                            me->AddAura(MOONLIT_TRASH, me);
                        }
                    }
                    Check_Timer = 3000;
                }
                else Check_Timer -= diff;
            }

            DoMeleeAttackIfReady();
        }
    };
};

//////////////////////////
//  Azure Dragonshrine  //
//////////////////////////
enum CreaturesAzure
{
    TIME_TWISTED_PRIEST           = 54690,
    TIME_TWISTED_RIFLEMAN         = 54693,
    TIME_TWISTED_FOOTMAN          = 54687,
    TIME_TWISTED_SORCERRER        = 54691,
    FOUNTAIN_OF_LIGHT             = 54795,
};

enum AzureDragonshrineSpells
{
    POWER_WORD_SHEILD             = 102409,
    FOINTAIN_OF_LIGHT             = 102405,

    SHOOT                         = 102410,
    MULTI_SHOT                    = 102411,

    SHIELD_BASH                   = 101817,
    SHIELD_WALL                   = 101811,
    THUNDERCLAP                   = 101820,

    ARCANE_BLAST                  = 101816,
    BLINK                         = 87925,

    LIGHT_RAIN                    = 102406,
};

// Time-Twisted Priest
class npc_time_twisted_priest : public CreatureScript
{
public:
    npc_time_twisted_priest() : CreatureScript("npc_time_twisted_priest") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_time_twisted_priestAI (pCreature);
    }

    struct npc_time_twisted_priestAI : public ScriptedAI
    {
        npc_time_twisted_priestAI(Creature *c) : ScriptedAI(c) {}

        uint32 Fountain_Of_Light;
        uint32 Power_Word_Shield;
        int Power_Word_Shield_Target;

        void Reset() 
        {
            Fountain_Of_Light = 7000+urand(0,5000);
            Power_Word_Shield = 5000+urand(0,7000);
            Power_Word_Shield_Target = 0;
        }

        void JustDied(Unit * /*who*/) { }

        void EnterCombat(Unit * /*who*/)
        {
            me->CastSpell(me, POWER_WORD_SHEILD, true);
        }

        void UpdateAI(const uint32 diff) 
        {
            if (!UpdateVictim())
                return;

            if (Fountain_Of_Light <= diff)
            {
                me->CastSpell(me, FOINTAIN_OF_LIGHT, true);
                Fountain_Of_Light = 60000;
            }
            else Fountain_Of_Light -= diff;

            // Cast Power World Shield on random target around Priest
            if (Power_Word_Shield <= diff)
            {
                Unit * target1 = me->FindNearestCreature(TIME_TWISTED_FOOTMAN, 15.0f, true);
                Unit * target2 = me->FindNearestCreature(TIME_TWISTED_RIFLEMAN, 15.0f, true);
                Unit * target3 = me->FindNearestCreature(TIME_TWISTED_SORCERRER, 15.0f, true);
                Unit * target4 = me->FindNearestCreature(FOUNTAIN_OF_LIGHT, 15.0f, true);

                Power_Word_Shield_Target = urand(0,3);

                switch(Power_Word_Shield_Target)
                {
                    case 0:
                        if (target1)
                            target1->CastSpell(target1, POWER_WORD_SHEILD, true);
                        else 
                            me->CastSpell(me, POWER_WORD_SHEILD, true);
                        break;
                    case 1:
                        if (target2)
                            target2->CastSpell(target2, POWER_WORD_SHEILD, true);
                        else
                            me->CastSpell(me, POWER_WORD_SHEILD, true);
                        break;
                    case 2:
                        if (target3)
                            target3->CastSpell(target3, POWER_WORD_SHEILD, true);
                        else
                            me->CastSpell(me, POWER_WORD_SHEILD, true);
                        break;
                    case 3:
                        if (target4)
                            target4->CastSpell(target4, POWER_WORD_SHEILD, true);
                        else
                            me->CastSpell(me, POWER_WORD_SHEILD, true);
                        break;
                    case 4:
                        me->CastSpell(me, POWER_WORD_SHEILD, true);
                        break;
                }

                Power_Word_Shield = 3000+urand(0,5000);
            }
            else Power_Word_Shield -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

// Time-Twisted Rifleman
class npc_time_twisted_rifleman : public CreatureScript
{
public:
    npc_time_twisted_rifleman() : CreatureScript("npc_time_twisted_rifleman") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_time_twisted_riflemanAI (pCreature);
    }

    struct npc_time_twisted_riflemanAI : public ScriptedAI
    {
        npc_time_twisted_riflemanAI(Creature *c) : ScriptedAI(c) {}

        uint32 Shoot;
        uint32 Multi_Shot;

        void Reset() 
        {
            Shoot = 5000;
            Multi_Shot = 10000;
        }

        void JustDied(Unit * /*who*/) { }

        void UpdateAI(const uint32 diff) 
        {
            if (!UpdateVictim())
                return;

            if (Shoot <= diff)
            {
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 150.0f, true);
                if (target)
                    me->CastSpell(target, SHOOT, true);
                Shoot = 3000+urand(0, 5000);
            }
            else Shoot -= diff;

            if (Multi_Shot <= diff)
            {
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 150.0f, true);
                if (target)
                    me->CastSpell(me, MULTI_SHOT, true);
                Multi_Shot = 10000+urand(0,5000);
            }
            else Multi_Shot -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

// Time-Twisted Footman
class npc_time_twisted_footman : public CreatureScript
{
public:
    npc_time_twisted_footman() : CreatureScript("npc_time_twisted_footman") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_time_twisted_footmanAI (pCreature);
    }

    struct npc_time_twisted_footmanAI : public ScriptedAI
    {
        npc_time_twisted_footmanAI(Creature *c) : ScriptedAI(c) {}

        uint32 Thunderclap;
        uint32 Shield_Wall;
        uint32 Shield_Bash;

        void Reset() 
        {
            Thunderclap = 7000+urand(0, 5000);
            Shield_Wall = 5000;
            Shield_Bash = 10000;
        }

        void JustDied(Unit * /*who*/) { }

        void UpdateAI(const uint32 diff) 
        {
            if (!UpdateVictim())
                return;

            if (Thunderclap <= diff)
            {
                me->CastSpell(me, THUNDERCLAP, true);
                Thunderclap = 15000+urand(0, 3000);
            }
            else Thunderclap -= diff;

            if (Shield_Wall <= diff)
            {
                me->CastSpell(me, SHIELD_WALL, true);
                Shield_Wall = 10000+urand(0, 2000);
            }
            else Shield_Wall -= diff;

            if (Shield_Bash <= diff)
            {
                Unit * target = me->GetVictim();
                if (target)
                    me->CastSpell(target, SHIELD_BASH, true);
                Shield_Bash = 10000;
            }
            else Shield_Bash -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

// Time-Twisted Sorceress
class npc_time_twisted_sorceress : public CreatureScript
{
public:
    npc_time_twisted_sorceress() : CreatureScript("npc_time_twisted_sorceress") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_time_twisted_sorceressAI (pCreature);
    }

    struct npc_time_twisted_sorceressAI : public ScriptedAI
    {
        npc_time_twisted_sorceressAI(Creature *c) : ScriptedAI(c) {}

        uint32 Arcane_Blast;
        uint32 Blink;

        void Reset() 
        {
            Arcane_Blast = 2500;
            Blink = 7000+urand(0, 3000);
        }

        void JustDied(Unit * /*who*/) { }
        void EnterCombat(Unit * /*who*/)
        {
            Unit * target = me->GetVictim();
            me->CastSpell(target, ARCANE_BLAST, false);
        }

        void UpdateAI(const uint32 diff) 
        {
            if (!UpdateVictim())
                return;

            if (Arcane_Blast <= diff)
            {
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true);
                if (target)
                    me->CastSpell(target, ARCANE_BLAST, false);
                Arcane_Blast = 2500;
            }
            else Arcane_Blast -= diff;

            if (Blink <= diff)
            {
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
                me->CastSpell(me, BLINK, false);
                Arcane_Blast = 1000;
                Blink = 10000+urand(0, 5000);
            }
            else Blink -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

// Fountain of Light
class npc_fountain_of_light : public CreatureScript
{
public:
    npc_fountain_of_light() : CreatureScript("npc_fountain_of_light") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_fountain_of_lightAI (pCreature);
    }

    struct npc_fountain_of_lightAI : public ScriptedAI
    {
        npc_fountain_of_lightAI(Creature *c) : ScriptedAI(c) {}

        void Reset() 
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->CastSpell(me, LIGHT_RAIN, true);
        }

    };
};

// Fragments of Jaina`s staff
class go_fragments_of_jainas_staff : public GameObjectScript
{
public:
    go_fragments_of_jainas_staff() : GameObjectScript("go_fragments_of_jainas_staff") { }

    bool OnGossipHello(Player* pPlayer, GameObject* pGo)
    {
        if (InstanceScript *pInstance = pGo->GetInstanceScript())
        {
            pInstance->SetData(DATA_CRYSTALS, 1);
            pInstance->DoUpdateWorldState(WORLD_STATE_FRAGMENTS, 1);
        }
        pGo->Delete();
        return true;
    }
};

// Alurmi
class npc_alurmi : public CreatureScript
{
public:
    npc_alurmi() : CreatureScript("npc_alurmi") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_alurmiAI (pCreature);
    }

    struct npc_alurmiAI : public ScriptedAI
    {
        npc_alurmiAI(Creature *c) : ScriptedAI(c) {}

        uint32 Check_Timer;
        uint32 Change_Timer;
        bool Change;
        bool Fly;

        void Reset() 
        {
            me->RemoveAura(109297);
            me->SetVisible(false);
            Check_Timer = 10000;
            Change_Timer = 4000;
            Change = false;
            Fly = false;
            me->SetSpeed(MOVE_FLIGHT, 0.3f);
            me->SetSpeed(MOVE_RUN, 0.3f);
            me->SetSpeed(MOVE_WALK, 0.3f);
        }

        void UpdateAI(const uint32 diff) 
        {
            if (Fly == false)
            {
                if (Check_Timer <= diff)
                {
                    float distance;
                    int count;
                    count = 0;
                    Map::PlayerList const &playerList = me->GetMap()->GetPlayers();
                    if (!playerList.isEmpty())
                        for (Map::PlayerList::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
                            if (Player* pPlayer = i->getSource())
                            {
                                distance = me->GetExactDist2d(pPlayer);
                                if (distance<30)
                                    count = count+1;
                            }

                    if (count >= 1)
                    {
                        me->SetVisible(true);
                        me->GetMotionMaster()->MovePoint(0, 3703.3f, -363.1712f, 113.942657f);
                        Change_Timer = 3500;
                        Change = true;
                        Fly = true;
                    }

                    Check_Timer = 10000;
                } else Check_Timer -= diff;
            }

            if (Change == true)
            {
                if (Change_Timer <= diff)
                {
                    me->CastSpell(me, 109297, true);
                    Change = false;
                }
                else Change_Timer -= diff;
            }
        }
    };
};

void AddSC_instance_end_time()
{
    new instance_end_time();

    new go_time_transit_device();
    new go_fragments_of_jainas_staff();

    new npc_image_of_nozdormu();

    new npc_infinite_suppressor();
    new npc_infinite_warden();
    new npc_time_twisted_seer();
    new npc_time_twisted_breaker();
    new npc_time_twisted_drake();
    new npc_ruptured_ground();
    new npc_call_flames();
    new npc_time_twisted_scourge_beast();
    new npc_time_twisted_geist();
    new npc_time_twisted_emerald_trash();
    new npc_time_twisted_priest();
    new npc_time_twisted_rifleman();
    new npc_time_twisted_footman();
    new npc_time_twisted_sorceress();
    new npc_fountain_of_light();
    new npc_alurmi();
}