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
#include "halls_of_origination.h"

enum data
{
    DATA_TEMPLE_GUARDIAN_ANHUUR = 0,
    DATA_ANRAPHET,
    DATA_ISISET,
    DATA_AMMUNAE,
    DATA_SETESH,
    DATA_RAJH,
    DATA_PTAH,
    GUID_PTAH=39428,
};

#define MAX_ENCOUNTER 7

class instance_halls_of_origination : public InstanceMapScript
{
public:
    instance_halls_of_origination() : InstanceMapScript("instance_halls_of_origination", 644) { }

    struct instance_halls_of_origination_InstanceScript : public InstanceScript
    {
        instance_halls_of_origination_InstanceScript(Map* pMap) : InstanceScript(pMap) {Initialize();};

        uint32 auiEncounter[MAX_ENCOUNTER];
        uint32 currEnc[MAX_ENCOUNTER];

        uint64 Temple_Guardian_AnhuurGUID;
        uint64 AnraphetGUID;
        uint64 IsisetGUID;
        uint64 AmmunaeGUID;
        uint64 SeteshGUID;
        uint64 RajhGUID;

        void Initialize()
        {
            memset(&auiEncounter, 0, sizeof(auiEncounter));

            Temple_Guardian_AnhuurGUID = 0;
            AnraphetGUID = 0;
            IsisetGUID = 0;
            AmmunaeGUID = 0;
            SeteshGUID = 0;
            RajhGUID = 0;
            GetCorrUiEncounter();
        }


        void OnCreatureCreate(Creature* pCreature, bool add)
        {
            if (!add) // for safity
                return;

            switch (pCreature->GetEntry())
            {
                case 39425: // Temple Guardian Anhuur
                    Temple_Guardian_AnhuurGUID = pCreature->GetGUID();
                    break;
                case 39788: // Anraphet
                    AnraphetGUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_TEMPLE_GUARDIAN_ANHUUR] == DONE)
                        pCreature->setFaction(14);
                    break;
                case 39587: // Isiset
                    IsisetGUID  = pCreature->GetGUID();
                    if (auiEncounter[DATA_ANRAPHET] == DONE)
                        pCreature->setFaction(14);
                    break;
                case 39731: // Ammunae
                    AmmunaeGUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_ANRAPHET] == DONE)
                        pCreature->setFaction(14);
                    break;
                case 39732: // Setesh
                    SeteshGUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_ANRAPHET] == DONE)
                        pCreature->setFaction(14);
                    break;
                case 39378: // Rajh
                    RajhGUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_ANRAPHET] == DONE)
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
                case DATA_TEMPLE_GUARDIAN_ANHUUR:
                    return Temple_Guardian_AnhuurGUID;
                case DATA_ANRAPHET:
                    return AnraphetGUID;
                case DATA_ISISET:
                    return IsisetGUID;
                case DATA_AMMUNAE:
                    return AmmunaeGUID;
                case DATA_SETESH:
                    return SeteshGUID;
                case DATA_RAJH:
                    return RajhGUID;
            }
            return 0;
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
                case DATA_TEMPLE_GUARDIAN_ANHUUR:
                case DATA_ANRAPHET:
                case DATA_ISISET:
                case DATA_AMMUNAE:
                case DATA_SETESH:
                case DATA_RAJH:
                    auiEncounter[type] = data;
                    break;
                case GUID_PTAH:
                    auiEncounter[DATA_PTAH] = data;
                    break;
            }

            if (auiEncounter[DATA_TEMPLE_GUARDIAN_ANHUUR] == DONE) // Temple
                if (Creature* anraphet = this->instance->GetCreature(AnraphetGUID))
                    anraphet->setFaction(14);
            if (auiEncounter[DATA_ANRAPHET] == DONE) // Anraphet
            {
                if (Creature* isiset = this->instance->GetCreature(IsisetGUID))
                    isiset->setFaction(14);
                if (Creature* ammunae = this->instance->GetCreature(AmmunaeGUID))
                    ammunae->setFaction(14);
                if (Creature* setesh = this->instance->GetCreature(SeteshGUID))
                    setesh->setFaction(14);
                if (Creature* rajh = this->instance->GetCreature(RajhGUID))
                    rajh->setFaction(14);
            }

            if (data == DONE)
            {
                std::ostringstream saveStream;
                saveStream << auiEncounter[DATA_TEMPLE_GUARDIAN_ANHUUR];
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
            else if(type==GUID_PTAH)
                return auiEncounter[6];
            else
                return 0;
       }

       std::string GetSaveData()
        {
            OUT_SAVE_INST_DATA;

            std::ostringstream saveStream;
            saveStream << auiEncounter[DATA_TEMPLE_GUARDIAN_ANHUUR];
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
            currEnc[0]=auiEncounter[DATA_TEMPLE_GUARDIAN_ANHUUR];//0
            currEnc[1]=auiEncounter[DATA_SETESH];//4
            currEnc[2]=auiEncounter[DATA_RAJH];//5
            currEnc[3]=auiEncounter[DATA_ISISET];//2
            currEnc[4]=auiEncounter[DATA_PTAH];//6
            currEnc[5]=auiEncounter[DATA_ANRAPHET];//1
            currEnc[6]=auiEncounter[DATA_AMMUNAE];//3
            sInstanceSaveMgr->setInstanceSaveData(instance->GetInstanceId(),currEnc,MAX_ENCOUNTER);
            return currEnc;
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap *map) const
    {
        return new instance_halls_of_origination_InstanceScript(map);
    }
};

void AddSC_instance_halls_of_origination()
{
    new instance_halls_of_origination();
}

/* SQL 
* UPDATE instance_template SET script='instance_halls_of_origination' WHERE map=644;
* UPDATE creature_template SET faction_A=35, faction_H=35 WHERE entry in (39788, 39587, 39731, 39732, 39378, 48815, 48710, 48715, 48776, 48902);
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (3942598, 39425, 6, 6, 34, 0, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (3978898, 39788, 6, 6, 34, 1, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (3958798, 39587, 6, 6, 34, 2, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (3973198, 39731, 6, 6, 34, 3, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (3973298, 39732, 6, 6, 34, 4, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (3937898, 39378, 6, 6, 34, 5, 3, 'instance script');
*/