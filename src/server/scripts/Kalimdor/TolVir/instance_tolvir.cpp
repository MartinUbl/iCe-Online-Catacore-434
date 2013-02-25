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
#include "tolvir.h"

enum data
{
    DATA_GENERAL_HUSAM = 0,
    DATA_LOCKMAW,
    DATA_HIGH_PROPHET_BARIM,
    DATA_SIAMAT
};

#define MAX_ENCOUNTER 4

class instance_lost_city_of_the_tolvir : public InstanceMapScript
{
public:
    instance_lost_city_of_the_tolvir() : InstanceMapScript("instance_lost_city_of_the_tolvir", 755) { }

    struct instance_lost_city_of_the_tolvir_InstanceScript : public InstanceScript
    {
        instance_lost_city_of_the_tolvir_InstanceScript(Map* pMap) : InstanceScript(pMap) {Initialize();};

        uint32 auiEncounter[MAX_ENCOUNTER];

        uint64 General_HusamGUID;
        uint64 LockmawGUID;
        uint64 High_Prophet_BarimGUID;
        uint64 SiamatGUID;

        void Initialize()
        {
            memset(&auiEncounter, 0, sizeof(auiEncounter));

            General_HusamGUID = 0;
            LockmawGUID = 0;
            High_Prophet_BarimGUID = 0;
            SiamatGUID = 0;
        }

        bool IsEncounterInProgress() const // not avaiable for this instance script
        {
            return false;
        }

        void OnCreatureCreate(Creature* pCreature, bool add)
        {
            if (!add) // for safity
                return;

            switch (pCreature->GetEntry())
            {
                case 44577: // General Husam
                    General_HusamGUID = pCreature->GetGUID();
                    break;
                case 43614: // Lockmaw
                    LockmawGUID  = pCreature->GetGUID();
                    if (auiEncounter[DATA_GENERAL_HUSAM] == DONE)
                        pCreature->setFaction(14);
                    break;
                case 43612: // High Prophet Barim
                    High_Prophet_BarimGUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_LOCKMAW] == DONE)
                        pCreature->setFaction(14);
                    break;
                case 44819: // Siamat
                    SiamatGUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_HIGH_PROPHET_BARIM] == DONE)
                        pCreature->setFaction(14);
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* pGO, bool add)
        {
            if (!add)
                return;
        }

        uint64 GetData64(uint32 type)
        {
            switch (type)
            {
                case DATA_GENERAL_HUSAM:
                    return General_HusamGUID;
                case DATA_LOCKMAW:
                    return LockmawGUID;
                case DATA_HIGH_PROPHET_BARIM:
                    return High_Prophet_BarimGUID;
                case DATA_SIAMAT:
                    return SiamatGUID;
            }
            return 0;
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
                case DATA_GENERAL_HUSAM:
                case DATA_LOCKMAW:
                case DATA_HIGH_PROPHET_BARIM:
                case DATA_SIAMAT:
                    auiEncounter[type] = data;
                    break;
            }

            if (auiEncounter[DATA_GENERAL_HUSAM] == DONE) // General
                if (Creature* lockmaw = this->instance->GetCreature(LockmawGUID))
                    lockmaw->setFaction(14);
            if (auiEncounter[DATA_LOCKMAW] == DONE) // Lockmaw
                if (Creature* barim = this->instance->GetCreature(High_Prophet_BarimGUID))
                    barim->setFaction(14);
            if (auiEncounter[DATA_HIGH_PROPHET_BARIM] == DONE) // Barim
                if (Creature* siamat = this->instance->GetCreature(SiamatGUID))
                    siamat->setFaction(14);

            if (data == DONE)
            {
                std::ostringstream saveStream;
                saveStream << auiEncounter[DATA_GENERAL_HUSAM];
                for (uint8 i = 1; i < MAX_ENCOUNTER; i++)
                    saveStream << " " << auiEncounter[i];

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
            saveStream << auiEncounter[DATA_GENERAL_HUSAM];
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

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                if (auiEncounter[i] == IN_PROGRESS)
                    auiEncounter[i] = NOT_STARTED;

            OUT_LOAD_INST_DATA_COMPLETE;
        }
        virtual uint32* GetUiEncounter(){return auiEncounter;}
        virtual uint32 GetMaxEncounter(){return MAX_ENCOUNTER;}
    };

    InstanceScript* GetInstanceScript(InstanceMap *map) const
    {
        return new instance_lost_city_of_the_tolvir_InstanceScript(map);
    }
};

void AddSC_instance_lost_city_of_the_tolvir()
{
    new instance_lost_city_of_the_tolvir();
}


/* SQL 
* UPDATE instance_template SET script='instance_lost_city_of_the_tolvir' WHERE map=755;
* UPDATE creature_template SET faction_A=35, faction_H=35 WHERE entry in (43612, 43614, 44819, 48951, 49043, 51088);
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4457798, 44577, 6, 6, 34, 0, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4361407, 43614, 6, 6, 34, 1, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4361298, 43612, 6, 6, 34, 2, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4481998, 44819, 6, 6, 34, 3, 3, 'instance script');
*/