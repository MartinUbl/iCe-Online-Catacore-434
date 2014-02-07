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
#include "the_stonecore.h"

enum data
{
    DATA_CORBORUS = 0,
    DATA_SLABHIDE,
    DATA_OZRUK,
    DATA_HIGH_PRIESTESS_AZIL,
    DATA_MILLHOUSE_EVENT1,
    DATA_MILLHOUSE_EVENT2,
    DATA_MILLHOUSE_EVENT3,
    DATA_CORBORUS_EVENT,
};

#define MAX_ENCOUNTER 8

class instance_the_stonecore : public InstanceMapScript
{
public:
    instance_the_stonecore() : InstanceMapScript("instance_the_stonecore", 725) { }

    struct instance_the_stonecore_InstanceScript : public InstanceScript
    {
        instance_the_stonecore_InstanceScript(Map* pMap) : InstanceScript(pMap) {Initialize();};

        uint32 auiEncounter[MAX_ENCOUNTER];
        uint32 currEnc[MAX_ENCOUNTER];

        uint64 CorborusGUID;
        uint64 Corborus2GUID;
        uint64 SlabhideGUID;
        uint64 OzrukGUID;
        uint64 High_Priestess_AzilGUID;
        uint64 Millhouse1_GUID;
        uint64 Millhouse2_GUID;
        uint64 wallGUID;

        void Initialize()
        {
            memset(&auiEncounter, 0, sizeof(auiEncounter));

            CorborusGUID = 0;
            Corborus2GUID = 0;
            SlabhideGUID = 0;
            OzrukGUID = 0;
            High_Priestess_AzilGUID = 0;
            Millhouse1_GUID = 0;
            Millhouse2_GUID = 0;
            wallGUID = 0;
            GetCorrUiEncounter();
        }


        void OnCreatureCreate(Creature* pCreature, bool add)
        {
            if (!add) // for safity
                return;

            switch (pCreature->GetEntry())
            {
                case 434380:
                    Corborus2GUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_CORBORUS_EVENT] == DONE)
                        pCreature->SetPhaseMask(PHASEMASK_NORMAL, false);
                    break;
                case 43438: // Corborus
                    CorborusGUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_CORBORUS_EVENT] == DONE)
                        pCreature->SetPhaseMask(2, false);
                    break;
                case 43214: // Slabhide
                    SlabhideGUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_CORBORUS] == DONE)
                        pCreature->setFaction(14);
                    break;
                case 42188: // Ozruk
                    OzrukGUID  = pCreature->GetGUID();
                    if (auiEncounter[DATA_SLABHIDE] == DONE)
                        pCreature->setFaction(14);
                    break;
                case 42333: // High Priestess Azil
                    High_Priestess_AzilGUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_OZRUK] == DONE)
                        pCreature->setFaction(14);
                    break;
                case 43392:
                    Millhouse1_GUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_MILLHOUSE_EVENT1] == DONE)
                        pCreature->SetPhaseMask(PHASEMASK_NORMAL, false);
                     break;
                case 433930:
                    Millhouse2_GUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_MILLHOUSE_EVENT2] == DONE)
                        pCreature->SetPhaseMask(PHASEMASK_NORMAL, false);
                     if (auiEncounter[DATA_MILLHOUSE_EVENT3] == IN_PROGRESS) {
                        SetData(DATA_MILLHOUSE_EVENT3, NOT_STARTED);
                     }
                     break;
            }
        }

        void OnGameObjectCreate(GameObject* pGO, bool add)
        {
            if (!add)
                return;

            if (pGO->GetEntry() == 4510265)
            {
                if (auiEncounter[DATA_CORBORUS_EVENT] == DONE) {
                    pGO->SetPhaseMask(2,false);
                    pGO->Delete();
                }
            }

        }

        uint64 GetData64(uint32 type)
        {
            switch (type)
            {
                case DATA_CORBORUS:
                case DATA_CORBORUS_EVENT:
                    return CorborusGUID;
                case DATA_SLABHIDE:
                    return SlabhideGUID;
                case DATA_OZRUK:
                    return OzrukGUID;
                case DATA_HIGH_PRIESTESS_AZIL:
                    return High_Priestess_AzilGUID;
                case DATA_MILLHOUSE_EVENT1:
                    return Millhouse1_GUID;
                case DATA_MILLHOUSE_EVENT2:
                case DATA_MILLHOUSE_EVENT3:
                    return Millhouse2_GUID;
            }
            return 0;
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
                case DATA_CORBORUS:
                case DATA_SLABHIDE:
                case DATA_OZRUK:
                case DATA_HIGH_PRIESTESS_AZIL:
                case DATA_MILLHOUSE_EVENT1:
                case DATA_MILLHOUSE_EVENT2:
                case DATA_MILLHOUSE_EVENT3:
                case DATA_CORBORUS_EVENT:
                    auiEncounter[type] = data;
                    break;
            }

            if (auiEncounter[DATA_CORBORUS] == DONE) // Corborus
                if (Creature* slabhide = this->instance->GetCreature(SlabhideGUID))
                    slabhide->setFaction(14);
            if (auiEncounter[DATA_SLABHIDE] == DONE) // Slabhide
                if (Creature* ozruk = this->instance->GetCreature(OzrukGUID))
                    ozruk->setFaction(14);
            if (auiEncounter[DATA_OZRUK] == DONE) // Ozruk
                if (Creature* azil = this->instance->GetCreature(High_Priestess_AzilGUID))
                    azil->setFaction(14);
            if (auiEncounter[DATA_MILLHOUSE_EVENT1] == DONE) // Millhouse event
                if (Creature* millhouse = this->instance->GetCreature(Millhouse1_GUID))
                    millhouse->SetPhaseMask(PHASEMASK_NORMAL, false);

            if (Creature* millhouse2 = this->instance->GetCreature(Millhouse2_GUID))
            {
                if (auiEncounter[DATA_MILLHOUSE_EVENT2] == DONE) // Millhouse event 2
                    millhouse2->SetPhaseMask(PHASEMASK_NORMAL, false);
                if (auiEncounter[DATA_MILLHOUSE_EVENT3] == IN_PROGRESS) // behani
                {
                    millhouse2->GetMotionMaster()->MovePoint(6, 1159.725342f, 881.786621f, 284.963928f);
                }
                if (auiEncounter[DATA_MILLHOUSE_EVENT3] == DONE) {
                    millhouse2->AI()->DoAction(1);
                }
            }

            if (Creature* corborus = this->instance->GetCreature(CorborusGUID))
            {
                if (auiEncounter[DATA_CORBORUS_EVENT] == IN_PROGRESS)
                    corborus->AI()->DoAction(1);
                if (auiEncounter[DATA_CORBORUS_EVENT] == DONE)
                    corborus->SetPhaseMask(2, false);
            }
            if (Creature* corborus_new = this->instance->GetCreature(Corborus2GUID))
            {
                if (auiEncounter[DATA_CORBORUS_EVENT] == DONE) {
                    corborus_new->SetPhaseMask(PHASEMASK_NORMAL, false);
                    if (GameObject* go = this->instance->GetGameObject(wallGUID)) {
                        go->SetPhaseMask(2, false);
                        go->Delete();
                    }
                }
            }

            if (data == DONE)
            {
                std::ostringstream saveStream;
                saveStream << auiEncounter[DATA_CORBORUS];
                for (uint8 i = 1; i < MAX_ENCOUNTER; i++)
                    saveStream << " " << auiEncounter[i];

                GetCorrUiEncounter();
                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
            }
        }

       uint32 GetData(uint32 type)
       {
            if (type < MAX_ENCOUNTER)
                return auiEncounter[type];
            else
                return 0;
       }

       std::string GetSaveData()
        {
            OUT_SAVE_INST_DATA;

            std::ostringstream saveStream;
            saveStream << auiEncounter[DATA_CORBORUS];
            for (uint8 i = 1; i < MAX_ENCOUNTER; i++)
                saveStream << " " << auiEncounter[i];

            OUT_SAVE_INST_DATA_COMPLETE;
            return saveStream.str();
        }

        void Load(const char* in)
        {
            if (!in)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(in);

            std::istringstream loadStream(in);
            for (uint8 i = 0; i < MAX_ENCOUNTER; i++)
                loadStream >> auiEncounter[i];

            for (uint8 i = 0; i < 3; ++i)
                if (auiEncounter[i] == IN_PROGRESS)
                    auiEncounter[i] = NOT_STARTED;

            OUT_LOAD_INST_DATA_COMPLETE;
        }
        virtual uint32* GetCorrUiEncounter()
        {
            currEnc[0]=auiEncounter[DATA_SLABHIDE];//1
            currEnc[1]=auiEncounter[DATA_OZRUK];//2
            currEnc[2]=auiEncounter[DATA_HIGH_PRIESTESS_AZIL];//3
            currEnc[3]=auiEncounter[DATA_CORBORUS];//0
            sInstanceSaveMgr->setInstanceSaveData(instance->GetInstanceId(),currEnc,MAX_ENCOUNTER-4);
            return currEnc;
        }

    };

    InstanceScript* GetInstanceScript(InstanceMap *map) const
    {
        return new instance_the_stonecore_InstanceScript(map);
    }
};

void AddSC_instance_the_stonecore()
{
    new instance_the_stonecore();
}


/* SQL 
* UPDATE instance_template SET script='instance_the_stonecore' WHERE map=725;
* UPDATE creature_template SET faction_A=35, faction_H=35 WHERE entry in (43214, 42188, 42333, 49654, 49624, 49538);
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4343898, 43438, 6, 6, 34, 0, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4321498, 43214, 6, 6, 34, 1, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4218898, 42188, 6, 6, 34, 2, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4233398, 42333, 6, 6, 34, 3, 3, 'instance script');
*/