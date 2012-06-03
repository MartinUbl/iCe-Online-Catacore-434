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
    DATA_HIGH_PRIESTESS_AZIL
};

#define MAX_ENCOUNTER 4

class instance_the_stonecore : public InstanceMapScript
{
public:
    instance_the_stonecore() : InstanceMapScript("instance_the_stonecore", 725) { }

    struct instance_the_stonecore_InstanceScript : public InstanceScript
    {
        instance_the_stonecore_InstanceScript(Map* pMap) : InstanceScript(pMap) {Initialize();};

        uint32 auiEncounter[MAX_ENCOUNTER];

        uint64 CorborusGUID;
        uint64 SlabhideGUID;
        uint64 OzrukGUID;
        uint64 High_Priestess_AzilGUID;

        void Initialize()
        {
            memset(&auiEncounter, 0, sizeof(auiEncounter));

            CorborusGUID = 0;
            SlabhideGUID = 0;
            OzrukGUID = 0;
            High_Priestess_AzilGUID = 0;
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
                case 43438: // Corborus
                    CorborusGUID = pCreature->GetGUID();
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
                case DATA_CORBORUS:
                    return CorborusGUID;
                case DATA_SLABHIDE:
                    return SlabhideGUID;
                case DATA_OZRUK:
                    return OzrukGUID;
                case DATA_HIGH_PRIESTESS_AZIL:
                    return High_Priestess_AzilGUID;
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

            if (data == DONE)
            {
                std::ostringstream saveStream;
                saveStream << auiEncounter[DATA_CORBORUS];
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

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                if (auiEncounter[i] == IN_PROGRESS)
                    auiEncounter[i] = NOT_STARTED;

            OUT_LOAD_INST_DATA_COMPLETE;
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