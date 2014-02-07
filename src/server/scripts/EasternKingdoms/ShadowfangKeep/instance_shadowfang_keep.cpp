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

enum data
{
    DATA_BARON_ASHBURY = 0,
    DATA_BARON_SILVERLAINE,
    DATA_COMMANDER_SPRINGVALE,
    DATA_LORD_VALDEN,
    DATA_LORD_GODFREY
};

#define MAX_ENCOUNTER 5

class instance_shadowfang_keep : public InstanceMapScript
{
public:
    instance_shadowfang_keep() : InstanceMapScript("instance_shadowfang_keep", 33) { }

    struct instance_shadowfang_keep_InstanceScript : public InstanceScript
    {
        instance_shadowfang_keep_InstanceScript(Map* pMap) : InstanceScript(pMap) {Initialize();};

        uint32 auiEncounter[MAX_ENCOUNTER];
        uint32 currEnc[MAX_ENCOUNTER];

        uint64 Baron_AshburyGUID;
        uint64 Baron_SilverlaineGUID;
        uint64 Commander_SpringvaleGUID;
        uint64 Lord_ValdenGUID;
        uint64 Lord_GodfreyGUID;

        void Initialize()
        {
            memset(&auiEncounter, 0, sizeof(auiEncounter));

            Baron_AshburyGUID = 0;
            Baron_SilverlaineGUID = 0;
            Commander_SpringvaleGUID = 0;
            Lord_ValdenGUID = 0;
            Lord_GodfreyGUID = 0;
            GetCorrUiEncounter();
        }


        void OnCreatureCreate(Creature* pCreature, bool add)
        {
            if (!add) // for safity
                return;

            switch (pCreature->GetEntry())
            {
                case 46962: // Baron Ashbury
                    Baron_AshburyGUID = pCreature->GetGUID();
                    break;
                case 3887: // Baron Silverlaine
                    Baron_SilverlaineGUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_BARON_ASHBURY] == DONE)
                        pCreature->setFaction(14);
                    break;
                case 4278: // Commander Springvale
                    Commander_SpringvaleGUID  = pCreature->GetGUID();
                    if (auiEncounter[DATA_BARON_SILVERLAINE] == DONE)
                        pCreature->setFaction(14);
                    break;
                case 46963: // Lord Walden
                    Lord_ValdenGUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_COMMANDER_SPRINGVALE] == DONE)
                        pCreature->setFaction(14);
                    break;
                case 46964: // Lord Godfrey
                    Lord_GodfreyGUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_LORD_VALDEN] == DONE)
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
                case DATA_BARON_ASHBURY:
                    return Baron_AshburyGUID;
                case DATA_BARON_SILVERLAINE:
                    return Baron_SilverlaineGUID;
                case DATA_COMMANDER_SPRINGVALE:
                    return Commander_SpringvaleGUID;
                case DATA_LORD_VALDEN:
                    return Lord_ValdenGUID;
                case DATA_LORD_GODFREY:
                    return Lord_GodfreyGUID;
            }
            return 0;
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
                case DATA_BARON_ASHBURY:
                case DATA_BARON_SILVERLAINE:
                case DATA_COMMANDER_SPRINGVALE:
                case DATA_LORD_VALDEN:
                case DATA_LORD_GODFREY:
                    auiEncounter[type] = data;
                    break;
            }

            if (auiEncounter[DATA_BARON_ASHBURY] == DONE) // Asbury
                if (Creature* silverlaine = this->instance->GetCreature(Baron_SilverlaineGUID))
                    silverlaine->setFaction(14);
            if (auiEncounter[DATA_BARON_SILVERLAINE] == DONE) // Silverlaine
                if (Creature* springvale = this->instance->GetCreature(Commander_SpringvaleGUID))
                    springvale->setFaction(14);
            if (auiEncounter[DATA_COMMANDER_SPRINGVALE] == DONE) // Springvale
                if (Creature* valden = this->instance->GetCreature(Lord_ValdenGUID))
                    valden->setFaction(14);
            if (auiEncounter[DATA_LORD_VALDEN] == DONE) // Valden
                if (Creature* godfrey = this->instance->GetCreature(Lord_GodfreyGUID))
                    godfrey->setFaction(14);

            if (data == DONE)
            {
                std::ostringstream saveStream;
                saveStream << auiEncounter[DATA_BARON_ASHBURY];
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
            saveStream << auiEncounter[DATA_BARON_ASHBURY];
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
            currEnc[0]=auiEncounter[DATA_LORD_VALDEN];//3
            currEnc[1]=auiEncounter[DATA_LORD_GODFREY];//4
            currEnc[2]=auiEncounter[DATA_COMMANDER_SPRINGVALE];//2
            currEnc[3]=auiEncounter[DATA_BARON_SILVERLAINE];//1
            currEnc[4]=auiEncounter[DATA_BARON_ASHBURY];//0
            sInstanceSaveMgr->setInstanceSaveData(instance->GetInstanceId(),currEnc,MAX_ENCOUNTER);
            return currEnc;
        }

    };

    InstanceScript* GetInstanceScript(InstanceMap *map) const
    {
        return new instance_shadowfang_keep_InstanceScript(map);
    }
};

void AddSC_instance_shadowfang_keep()
{
    new instance_shadowfang_keep();
}


/* SQL 
* UPDATE instance_template SET script='instance_shadowfang_keep' WHERE map=33;
* UPDATE creature_template SET faction_A=35, faction_H=35 WHERE entry in (3887, 4278, 46963, 46964, 388711, 427811, 469631, 469641);
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4696298, 46962, 6, 6, 34, 0, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (388798, 3887, 6, 6, 34, 1, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (427898, 4278, 6, 6, 34, 2, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4696398, 46963, 6, 6, 34, 3, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4696498, 46964, 6, 6, 34, 4, 3, 'instance script');
*/