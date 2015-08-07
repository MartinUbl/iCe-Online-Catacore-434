/*
* Copyright (C) 2010-2011 Project SkyFire <http://www.projectskyfire.org/>
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
#include "baradin_hold.h"

#define ENCOUNTERS 3

class instance_baradin_hold: public InstanceMapScript
{
public:
    instance_baradin_hold() : InstanceMapScript("instance_baradin_hold", 757) { }
    
    InstanceScript* GetInstanceScript(InstanceMap *map) const
    {
        return new instance_baradin_hold_InstanceMapScript(map);
    }
    
    struct instance_baradin_hold_InstanceMapScript: public InstanceScript
    {
        instance_baradin_hold_InstanceMapScript(InstanceMap *map) : InstanceScript(map) { }

        uint32 uiEncounter[ENCOUNTERS];
        uint32 currEnc[ENCOUNTERS];

        uint64 uiArgaloth;
        uint64 uiOccuthar;
        uint64 uiAlizabal;
 
        void Initialize()
        {
            uiArgaloth = 0;
            uiOccuthar = 0;
            uiAlizabal = 0;
            for(uint8 i=0; i < ENCOUNTERS; ++i)
                uiEncounter[i] = NOT_STARTED;
            GetCorrUiEncounter();
        }


        void OnCreatureCreate(Creature* pCreature, bool)
        {
            switch(pCreature->GetEntry())
            {
                case BOSS_ARGALOTH:
                    uiArgaloth = pCreature->GetGUID();
                    break;
                case BOSS_OCCUTHAR:
                    uiOccuthar = pCreature->GetGUID();
                    break;
                case BOSS_ALIZABAL:
                    uiAlizabal = pCreature->GetGUID();
                    break;
            }
        }

        uint64 getData64(uint32 identifier)
        {
            switch(identifier)
            {
                case DATA_ARGALOTH:
                    return uiArgaloth;
                case DATA_OCCUTHAR:
                    return uiOccuthar;
                case DATA_ALIZABAL:
                    return uiAlizabal;
            }
            return 0;
        }

        void SetData(uint32 type, uint32 data)
        {
            switch(type)
            {
                case DATA_ARGALOTH:
                    uiEncounter[0] = data;
                    break;
                case DATA_OCCUTHAR:
                    uiEncounter[1] = data;
                    break;
                case DATA_ALIZABAL:
                    uiEncounter[2] = data;
                    break;
            }

            if (data == DONE)
            {
                GetCorrUiEncounter();
                SaveToDB();
            }
        }

        std::string GetSaveData()
        {
            OUT_SAVE_INST_DATA;

            std::string str_data;
            std::ostringstream saveStream;
            saveStream << "B H" << " " << uiEncounter[0] << " " << uiEncounter[1] << " " << uiEncounter[2];
            str_data = saveStream.str();

            OUT_SAVE_INST_DATA_COMPLETE;
            return str_data;
        }

        void Load(const char* in)
        {
            if (!in)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(in);

            char dataHead1, dataHead2;
            uint16 data0, data1, data2;

            std::istringstream loadStream(in);
            loadStream >> dataHead1 >> dataHead2 >> data0 >> data1 >> data2;

            if (dataHead1 == 'B' && dataHead2 == 'H')
            {
                uiEncounter[0] = data0;
                uiEncounter[1] = data1;
                uiEncounter[2] = data2;
                       
                for(uint8 i=0; i < ENCOUNTERS; ++i)
                    if (uiEncounter[i] == IN_PROGRESS)
                        uiEncounter[i] = NOT_STARTED;
            }
            else OUT_LOAD_INST_DATA_FAIL;

            GetCorrUiEncounter();
            OUT_LOAD_INST_DATA_COMPLETE;
        }
        virtual uint32* GetCorrUiEncounter()
        {
            currEnc[0]=uiEncounter[1];//Argaloth
            currEnc[1]=uiEncounter[0];//Occuthar
            currEnc[2]=uiEncounter[2];//Alizabal
            sInstanceSaveMgr->setInstanceSaveData(instance->GetInstanceId(),currEnc,ENCOUNTERS);
            return NULL;
        }
    };
};

void AddSC_instance_baradin_hold()
{
    new instance_baradin_hold();
}
