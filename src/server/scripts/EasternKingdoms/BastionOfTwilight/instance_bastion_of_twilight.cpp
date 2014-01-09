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
        uint32 currEnc[MAX_ENCOUNTER-1];

        uint64 HalfusGUID;
        uint64 ValionaGUID;
        uint64 CouncilGUID;
        uint64 ChogallGUID;
        uint64 SinestraGUID;
        uint64 GOfloorGUID;
        uint8  EmeraldWhelpFlag;
        uint64 EmeraldWhelp[8];
        uint64 Proto_BehemothGUID;
        uint32 StormRiderEvent;
        uint32 TimeWardenEvent;
        uint32 SlateDragonEvent;
        uint32 NetherScionEvent;
        uint32 EmeraldWhelpEvent;
        uint32 HalfusIntro;
        uint32 SinestraIntro;

        bool ChogallKilledOnHC;

        void Initialize()
        {
            HalfusGUID = 0;
            ValionaGUID = 0;
            CouncilGUID = 0;
            ChogallGUID = 0;
            SinestraGUID = 0;
            GOfloorGUID = 0;
            HalfusIntro = 0;
            SinestraIntro = 0;
            EmeraldWhelpFlag = 0;
            Proto_BehemothGUID = 0;
            TimeWardenEvent = 1;
            StormRiderEvent = 1;
            SlateDragonEvent = 1;
            NetherScionEvent = 1;
            EmeraldWhelpEvent = 1;

            for (int i = 0; i < 8; ++i)
                EmeraldWhelp[i] = 0;
                       
            switch (urand(1, 10))
            {
            case 1:
                NetherScionEvent = 0;
                SlateDragonEvent = 0;
                break;
            case 2:
                NetherScionEvent = 0;
                StormRiderEvent = 0;
                break;
            case 3:
                NetherScionEvent = 0;
                EmeraldWhelpEvent = 0;
                break;
            case 4:
                NetherScionEvent = 0;
                TimeWardenEvent = 0;
                break;
            case 5:
                SlateDragonEvent = 0;
                StormRiderEvent = 0;
                break;
            case 6:
                SlateDragonEvent = 0;
                EmeraldWhelpEvent = 0;
                break;
            case 7:
                SlateDragonEvent = 0;
                TimeWardenEvent = 0;
                break;
            case 8:
                StormRiderEvent = 0;
                EmeraldWhelpEvent = 0;
                break;
            case 9:
                StormRiderEvent = 0;
                TimeWardenEvent = 0;
                break;
            case 10:
                TimeWardenEvent = 0;
                EmeraldWhelpEvent = 0;
                break;
            };

            for(uint8 i=0; i < MAX_ENCOUNTER; ++i)
                auiEncounter[i] = NOT_STARTED;

            ChogallKilledOnHC=false;
        }


        void OnGameObjectCreate(GameObject* go, bool add)
        {
            if(!add)
                return;

            if (go->GetEntry() == 402097)
            {
                GOfloorGUID = go->GetGUID();
                if (auiEncounter[0] == DONE // Halfus
                    && auiEncounter[1] == DONE // Valiona
                    && auiEncounter[2] == DONE // Council
                    && auiEncounter[3] == DONE) // Chogall
                    {
                        if (this->instance->IsHeroic() && ChogallKilledOnHC)
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
                case 44687:
                    Proto_BehemothGUID = pCreature->GetGUID();
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
                case DATA_PROTO_BEHEMOTH:
                    return Proto_BehemothGUID;
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
                    auiEncounter[0] = data;
                    break;
                case DATA_VALIONA:
                    auiEncounter[1] = data;
                    break;
                case DATA_COUNCIL:
                    auiEncounter[2] = data;
                    break;
                case DATA_CHOGALL:
                    auiEncounter[3] = data;
                    if(this->instance->IsHeroic())
                        ChogallKilledOnHC=true;
                    break;
                case DATA_SINESTRA:
                    auiEncounter[4] = data;
                    break;
                case DATA_SINESTRA_INTRO:
                    SinestraIntro = data;
                    break;
            }

            if (auiEncounter[0] == DONE // Halfus
                && auiEncounter[1] == DONE // Valiona
                && auiEncounter[2] == DONE // Council
                && auiEncounter[3] == DONE) // Chogall
            {
                if (this->instance->IsHeroic() && ChogallKilledOnHC)
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

                GetCorrUiEncounter();
                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
            }
        }

       uint32 GetData(uint32 type)
       {
            if (type < MAX_ENCOUNTER)
                return auiEncounter[type];

            switch (type)
            {
                case DATA_HALFUS:                 return auiEncounter[0];
                case DATA_PROTO_BEHEMOTH:         return auiEncounter[0];
                case DATA_VALIONA:                return auiEncounter[1];
                case DATA_COUNCIL:                return auiEncounter[2];
                case DATA_CHOGALL:                return auiEncounter[3];
                case DATA_SINESTRA:               return auiEncounter[4];
                case DATA_STORM_RIDER:            return StormRiderEvent;
                case DATA_THE_TIME_WARDEN:        return TimeWardenEvent;
                case DATA_THE_SLATE_DRAGON:       return SlateDragonEvent;
                case DATA_NETHER_SCION:           return NetherScionEvent;
                case DATA_ORPHANED_EMERALD_WHELP: return EmeraldWhelpEvent;
                case DATA_HALFUS_INTRO:           return HalfusIntro;
                case DATA_SINESTRA_INTRO:         return SinestraIntro;
            }

            return 0;
       }

       std::string GetSaveData()
        {
            OUT_SAVE_INST_DATA;

            std::ostringstream saveStream;
            saveStream << auiEncounter[0];
            for (uint8 i = 1; i < MAX_ENCOUNTER; i++)
                saveStream << " " << auiEncounter[i];
            saveStream << " " << ChogallKilledOnHC;

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

            loadStream >> ChogallKilledOnHC;//load data, if Chogall was killed on hc

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                if (auiEncounter[i] == IN_PROGRESS)
                    auiEncounter[i] = NOT_STARTED;

            OUT_LOAD_INST_DATA_COMPLETE;
        }
        virtual uint32* GetUiEncounter(){return auiEncounter;}
        virtual uint32 GetMaxEncounter(){return MAX_ENCOUNTER;}
        virtual uint32* GetCorrUiEncounter()
        {
            uint32* uiEnc=GetUiEncounter();
            currEnc[0]=uiEnc[1];//valiona and theralion
            currEnc[1]=uiEnc[4];//sinestra
            currEnc[2]=uiEnc[0];//halfus
            currEnc[3]=uiEnc[3];//chogall
            currEnc[4]=uiEnc[2];//council
            sInstanceSaveMgr->setInstanceSaveData(instance->GetInstanceId(),currEnc,MAX_ENCOUNTER-1);
            sInstanceSaveMgr->setBossNumber(instance->GetId(),MAX_ENCOUNTER-1);

            return NULL;
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
