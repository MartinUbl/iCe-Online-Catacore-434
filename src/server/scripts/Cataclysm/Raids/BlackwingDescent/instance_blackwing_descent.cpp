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
#include "blackwing_descent.h"

#define MAX_ENCOUNTER    7
class instance_blackwing_descent : public InstanceMapScript
{
public:
    instance_blackwing_descent() : InstanceMapScript("instance_blackwing_descent", 669) { }

    struct instance_blackwing_descent_InstanceScript : public InstanceScript
    {
        instance_blackwing_descent_InstanceScript(Map* pMap) : InstanceScript(pMap) {Initialize();};

        uint32 auiEncounter[MAX_ENCOUNTER];
        uint32 currEnc[MAX_ENCOUNTER];

        uint64 MaloriakGUID;
        uint64 AtramedesGUID;
        uint64 ChimaeronGUID;
        uint64 MagmawGUID;
        uint64 OmnotronDefenceMatrixGUID;
        uint64 Nefarian1GUID;
        uint64 Nefarian2GUID;

        void Initialize()
        {
            memset(&auiEncounter, 0, sizeof(auiEncounter));

            MaloriakGUID = 0;
            AtramedesGUID = 0;
            ChimaeronGUID = 0;
            MagmawGUID = 0;
            OmnotronDefenceMatrixGUID = 0;
            Nefarian1GUID = 0;
            Nefarian2GUID = 0;
            GetCorrUiEncounter();
        }


        void OnCreatureCreate(Creature* pCreature, bool add)
        {
            if (!add)
                return;

            switch(pCreature->GetEntry())
            {
                case 41378: // Maloriak
                    MaloriakGUID = pCreature->GetGUID();
                    break;
                case 41442: // Atramedes
                    AtramedesGUID = pCreature->GetGUID();
                    break;
                case 43296: // Chimaeron
                    ChimaeronGUID = pCreature->GetGUID();
                    break;
                case 121050: // Magmaw
                    MagmawGUID = pCreature->GetGUID();
                    break;
                case 121060: // Omnotron Defence Matrix
                    OmnotronDefenceMatrixGUID = pCreature->GetGUID();
                    break;
                case 41376: // Nefarian 1
                    Nefarian1GUID = pCreature->GetGUID();
                    if (auiEncounter[1] == DONE
                        && auiEncounter[2] == DONE
                        && auiEncounter[3] == DONE
                        && auiEncounter[4] == DONE
                        && auiEncounter[5] == DONE)
                        pCreature->setFaction(14);
                    break;
                case 41379: // Nefarian 2
                    Nefarian2GUID = pCreature->GetGUID();
                    if (auiEncounter[1] == DONE
                        && auiEncounter[2] == DONE
                        && auiEncounter[3] == DONE
                        && auiEncounter[4] == DONE
                        && auiEncounter[5] == DONE)
                        pCreature->setFaction(14);
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go, bool add)
        {
            if(!add)
                return;
        }

        uint64 GetData64(uint32 type)
        {
            switch(type)
            {
                case DATA_MALORIAK_GUID:                return MaloriakGUID;
                case DATA_ATRAMEDES_GUID:               return AtramedesGUID;
                case DATA_CHIMAERON_GUID:               return ChimaeronGUID;
                case DATA_MAGMAW_GUID:                  return MagmawGUID;
                case DATA_DEFENSE_SYSTEM_GUID:          return OmnotronDefenceMatrixGUID;
                case DATA_NEFARIAN1:                    return Nefarian1GUID;
                case DATA_NEFARIAN2:                    return Nefarian2GUID;
            }
            return 0;
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
                case DATA_MALORIAK_GUID:
                    auiEncounter[1] = data;
                    break;
                case DATA_ATRAMEDES_GUID:
                    auiEncounter[2] = data;
                    break;
                case DATA_CHIMAERON_GUID:
                    auiEncounter[3] = data;
                    break;
                case DATA_DEFENSE_SYSTEM_GUID:
                    auiEncounter[4] = data;
                    break;
                case DATA_MAGMAW_GUID:
                    auiEncounter[5] = data;
                    break;
                case DATA_NEFARIAN1:
                case DATA_NEFARIAN2:
                    auiEncounter[6] = data;
                    break;
            }
            if (auiEncounter[1] == DONE
                && auiEncounter[2] == DONE
                && auiEncounter[3] == DONE
                && auiEncounter[4] == DONE
                && auiEncounter[5] == DONE)
            {
                if (Creature* nef = this->instance->GetCreature(Nefarian1GUID))
                    nef->setFaction(14);
                if (Creature* nef2 = this->instance->GetCreature(Nefarian2GUID))
                    nef2->setFaction(14);
            }
            if (data == DONE)
            {
                std::ostringstream saveStream;
                saveStream << auiEncounter[0];
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

            GetCorrUiEncounter();
            OUT_LOAD_INST_DATA_COMPLETE;
        }
        virtual uint32* GetCorrUiEncounter()
        {
            currEnc[0]=auiEncounter[4];//omnotron
            currEnc[1]=auiEncounter[6];//nefarian
            currEnc[2]=auiEncounter[1];//maloriak
            currEnc[3]=auiEncounter[5];//magmaw
            currEnc[4]=auiEncounter[3];//chimareon
            currEnc[5]=auiEncounter[2];//atramedes
            sInstanceSaveMgr->setInstanceSaveData(instance->GetInstanceId(),currEnc,MAX_ENCOUNTER-1);
            return currEnc;
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap *map) const
    {
        return new instance_blackwing_descent_InstanceScript(map);
    }
};

void AddSC_instance_blackwing_descent()
{
    new instance_blackwing_descent();
}

/**** SQL:

UPDATE instance_template SET script='instance_blackwing_descent' WHERE map=669;

*/
