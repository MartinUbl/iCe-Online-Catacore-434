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
#include "grim_batol.h"

enum data
{
    DATA_GENERAL_UMBRISS = 0,
    DATA_FORGEMASTER_THRONGUS,
    DATA_DRAHGA_SHADOWBURNER,
    DATA_EDURAX
};

#define MAX_ENCOUNTER 4

class instance_grim_batol : public InstanceMapScript
{
public:
    instance_grim_batol() : InstanceMapScript("instance_grim_batol",670) { }

    struct instance_grim_batol_InstanceScript : public InstanceScript
    {
        instance_grim_batol_InstanceScript(Map* pMap) : InstanceScript(pMap) {Initialize();};

        uint32 auiEncounter[MAX_ENCOUNTER];

        uint64 General_UmbrissGUID;
        uint64 Forgemaster_ThorngusGUID;
        uint64 Drahga_ShadowburnerGUID;
        uint64 EduraxGUID;

        void Initialize()
        {
            memset(&auiEncounter, 0, sizeof(auiEncounter)); 

            General_UmbrissGUID = 0;
            Forgemaster_ThorngusGUID = 0;
            Drahga_ShadowburnerGUID = 0;
            EduraxGUID = 0;
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
                case 39625: // General Umbriss
                    General_UmbrissGUID = pCreature->GetGUID();
                    break;
                case 40177: // Forgemaster Throngus
                    Forgemaster_ThorngusGUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_GENERAL_UMBRISS] == DONE)
                        pCreature->setFaction(14);
                    break;
                case 40319: // Drahga Shadowburner
                    Drahga_ShadowburnerGUID  = pCreature->GetGUID();
                    if (auiEncounter[DATA_FORGEMASTER_THRONGUS] == DONE)
                        pCreature->setFaction(14);
                    break;
                case 40484: // Erudax
                    EduraxGUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_DRAHGA_SHADOWBURNER] == DONE)
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
                case DATA_GENERAL_UMBRISS:
                    return General_UmbrissGUID;
                case DATA_FORGEMASTER_THRONGUS:
                    return Forgemaster_ThorngusGUID;
                case DATA_DRAHGA_SHADOWBURNER:
                    return Drahga_ShadowburnerGUID;
                case DATA_EDURAX:
                    return EduraxGUID;
            }
            return 0;
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
                case DATA_GENERAL_UMBRISS:
                case DATA_FORGEMASTER_THRONGUS:
                case DATA_DRAHGA_SHADOWBURNER:
                case DATA_EDURAX:
                    auiEncounter[type] = data;
                    break;
            }

            if (auiEncounter[DATA_GENERAL_UMBRISS] == DONE) // General
                if (Creature* forgemaster = this->instance->GetCreature(Forgemaster_ThorngusGUID))
                    forgemaster->setFaction(14);
            if (auiEncounter[DATA_FORGEMASTER_THRONGUS] == DONE) // Forgemaster
                if (Creature* drahga = this->instance->GetCreature(Drahga_ShadowburnerGUID))
                    drahga->setFaction(14);
            if (auiEncounter[DATA_DRAHGA_SHADOWBURNER] == DONE) // Dragha
                if (Creature* edurax = this->instance->GetCreature(EduraxGUID))
                    edurax->setFaction(14);

            if (data == DONE)
            {
                std::ostringstream saveStream;
                saveStream << auiEncounter[DATA_GENERAL_UMBRISS];
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
            saveStream << auiEncounter[DATA_GENERAL_UMBRISS];
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
        return new instance_grim_batol_InstanceScript(map);
    }
};

void AddSC_instance_grim_batol()
{
    new instance_grim_batol();
}


/* SQL 
* UPDATE instance_template SET script='instance_grim_batol' WHERE map=670;
* UPDATE creature_template SET faction_A=35, faction_H=35 WHERE entry in (40177, 40319, 40484, 48702, 48784, 48822);
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (3962598, 39625, 6, 6, 34, 0, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4017798, 40177, 6, 6, 34, 1, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4031998, 40319, 6, 6, 34, 2, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4048498, 40484, 6, 6, 34, 3, 3, 'instance script');
*/