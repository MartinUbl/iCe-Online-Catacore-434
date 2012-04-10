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
#include "bastion_of_twilight.h"

#define MAX_ENCOUNTER    6

class instance_bastion_of_twilight : public InstanceMapScript
{
public:
    instance_bastion_of_twilight() : InstanceMapScript("instance_bastion_of_twilight", 671) { }

    struct instance_bastion_of_twilight_InstanceScript : public InstanceScript
    {
        instance_bastion_of_twilight_InstanceScript(Map* pMap) : InstanceScript(pMap) {Initialize();};

        uint32 auiEncounter[MAX_ENCOUNTER];

        uint64 HalfusGUID;
        uint64 ValionaGUID;
        uint64 CouncilGUID;
        uint64 ChogallGUID;
        uint64 SinestraGUID;
        uint64 GOfloorGUID;

        void Initialize()
        {
            HalfusGUID = 0;
            ValionaGUID = 0;
            CouncilGUID = 0;
            ChogallGUID = 0;
            SinestraGUID = 0;
            GOfloorGUID = 0;

            for(uint8 i=0; i < MAX_ENCOUNTER; ++i)
                auiEncounter[i] = NOT_STARTED;
        }

        bool IsEncounterInProgress() const
        {
            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                if (auiEncounter[i] == IN_PROGRESS) return true;

            return false;
        }

        void OnGameObjectCreate(GameObject* go, bool add)
        {
            if(!add)
                return;

            if (go->GetEntry() == 402097)
            {
                GOfloorGUID = go->GetGUID();
                if (auiEncounter[1] == DONE // Halfus
                    && auiEncounter[2] == DONE // Valiona
                    && auiEncounter[3] == DONE // Council
                    && auiEncounter[4] == DONE) // Chogall
                    {
                        if (this->instance->IsHeroic())
                            go->Delete();
                    }
            }
        }

        void OnCreatureCreate(Creature* pCreature, bool add)
        {
            if (!add)
                return;

            switch (pCreature->GetEntry())
            {
                case 44600: // Halfus
                    HalfusGUID = pCreature->GetGUID();
                    break;
                case 45992: // Valiona
                    ValionaGUID = pCreature->GetGUID();
                    break;
                case 43735: // Council
                    CouncilGUID = pCreature->GetGUID();
                    break;
                case 41379: // Chogall
                    ChogallGUID = pCreature->GetGUID();
                    break;
                case 45213: // Sinestra
                    SinestraGUID = pCreature->GetGUID();
                    break;
            }

        }

        uint64 GetData64(uint32 type)
        {
            switch(type)
            {
                case DATA_HALFUS:
                    return HalfusGUID;
                    break;
                case DATA_VALIONA:
                    return ValionaGUID;
                    break;
                case DATA_COUNCIL:
                    return CouncilGUID;
                    break;
                case DATA_CHOGALL:
                    return ChogallGUID;
                    break;
                case DATA_SINESTRA:
                    return SinestraGUID;
                    break;
            }
            return 0;
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
                case DATA_HALFUS:
                    auiEncounter[1] = data;
                    break;
                case DATA_VALIONA:
                    auiEncounter[2] = data;
                    break;
                case DATA_COUNCIL:
                    auiEncounter[3] = data;
                    break;
                case DATA_CHOGALL:
                    auiEncounter[4] = data;
                    break;
                case DATA_SINESTRA:
                    auiEncounter[5] = data;
                    break;
            }

            if (auiEncounter[1] == DONE // Halfus
                && auiEncounter[2] == DONE // Valiona
                && auiEncounter[3] == DONE // Council
                && auiEncounter[4] == DONE) // Chogall
            {
                if (this->instance->IsHeroic())
                {
                    if (GameObject* go = this->instance->GetGameObject(GOfloorGUID))
                        go->Delete();
                }
            }

            if (data == DONE)
            {
                std::ostringstream saveStream;
                saveStream << auiEncounter[0];
                for (uint8 i = 1; i < MAX_ENCOUNTER; i++)
                    saveStream << " " << auiEncounter[i];

                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
            }
        }

        uint32 GetData(uint32 type)
        {
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
    };

    InstanceScript* GetInstanceScript(InstanceMap *map) const
    {
        return new instance_bastion_of_twilight_InstanceScript(map);
    }
};

void AddSC_instance_bastion_of_twilight()
{
    new instance_bastion_of_twilight();
}
