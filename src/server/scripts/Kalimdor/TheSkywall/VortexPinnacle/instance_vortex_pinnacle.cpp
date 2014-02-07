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
#include "vortex_pinnacle.h"

enum data
{
    DATA_GRAND_VIZIER_ERTAN = 0,
    DATA_ALTARIUS,
    DATA_ASAAD
};

#define MAX_ENCOUNTER 3

class instance_vortex_pinnacle : public InstanceMapScript
{
public:
    instance_vortex_pinnacle() : InstanceMapScript("instance_vortex_pinnacle", 657) { }

    struct instance_vortex_pinnacle_InstanceScript : public InstanceScript
    {
        instance_vortex_pinnacle_InstanceScript(Map* pMap) : InstanceScript(pMap) {Initialize();};

        uint32 auiEncounter[MAX_ENCOUNTER];
        uint32 currEnc[MAX_ENCOUNTER];

        uint64 Grand_Vizier_ErrtanGUID;
        uint64 AltariusGUID;
        uint64 AsaadGUID;

        void Initialize()
        {
            memset(&auiEncounter, 0, sizeof(auiEncounter));

            Grand_Vizier_ErrtanGUID = 0;
            AltariusGUID = 0;
            AsaadGUID = 0;
            GetCorrUiEncounter();
        }


        void OnCreatureCreate(Creature* pCreature, bool add)
        {
            if (!add) // for safity
                return;

            switch (pCreature->GetEntry())
            {
                case 43878: // Grand Vizier Ertan
                    Grand_Vizier_ErrtanGUID = pCreature->GetGUID();
                    break;
                case 43873: // Altairus
                    AltariusGUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_GRAND_VIZIER_ERTAN] == DONE)
                        pCreature->setFaction(14);
                    break;
                case 43875: // Asaad
                    AsaadGUID  = pCreature->GetGUID();
                    if (auiEncounter[DATA_ALTARIUS] == DONE)
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
                case DATA_GRAND_VIZIER_ERTAN:
                    return Grand_Vizier_ErrtanGUID;
                case DATA_ALTARIUS:
                    return AltariusGUID;
                case DATA_ASAAD:
                    return AsaadGUID;
            }
            return 0;
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
                case DATA_GRAND_VIZIER_ERTAN:
                case DATA_ALTARIUS:
                case DATA_ASAAD:
                    auiEncounter[type] = data;
                    break;
            }

            if (auiEncounter[DATA_GRAND_VIZIER_ERTAN] == DONE) // Ertan
                if (Creature* altar = this->instance->GetCreature(AltariusGUID))
                    altar->setFaction(14);
            if (auiEncounter[DATA_ALTARIUS] == DONE) // Altarius
                if (Creature* asaad = this->instance->GetCreature(AsaadGUID))
                    asaad->setFaction(14);

            if (data == DONE)
            {
                std::ostringstream saveStream;
                saveStream << auiEncounter[DATA_GRAND_VIZIER_ERTAN];
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
            saveStream << auiEncounter[0];
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
        virtual uint32* GetCorrUiEncounter()
        {
            currEnc[0]=auiEncounter[DATA_GRAND_VIZIER_ERTAN];//0
            currEnc[1]=auiEncounter[DATA_ASAAD];//2
            currEnc[2]=auiEncounter[DATA_ALTARIUS];//1
            sInstanceSaveMgr->setInstanceSaveData(instance->GetInstanceId(),currEnc,MAX_ENCOUNTER);
            return currEnc;
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap *map) const
    {
        return new instance_vortex_pinnacle_InstanceScript(map);
    }
};

void AddSC_instance_vortex_pinnacle()
{
    new instance_vortex_pinnacle();
}


/* SQL 
* UPDATE instance_template SET script='instance_vortex_pinnacle' WHERE map=657;
* UPDATE creature_template SET faction_A=35, faction_H=35 WHERE entry in (43873, 43875, 43874, 43876);
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4387898, 43878, 6, 6, 34, 0, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4387398, 43873, 6, 6, 34, 1, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4387598, 43875, 6, 6, 34, 2, 3, 'instance script');
*/