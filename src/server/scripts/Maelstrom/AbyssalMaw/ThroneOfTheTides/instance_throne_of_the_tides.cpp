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
#include "throne_of_the_tides.h"

enum data
{
    DATA_COMANNDER_ULTHOK = 0,
    DATA_LADY_NAZJAR,
    DATA_ERUNAK_STONESPEAKER,
    DATA_OZUMAT
};

#define MAX_ENCOUNTER 4

class instance_throne_of_the_tides : public InstanceMapScript
{
public:
    instance_throne_of_the_tides() : InstanceMapScript("instance_throne_of_the_tides", 643) { }

    struct instance_throne_of_the_tides_InstanceScript : public InstanceScript
    {
        instance_throne_of_the_tides_InstanceScript(Map* pMap) : InstanceScript(pMap) {Initialize();};

        uint32 auiEncounter[MAX_ENCOUNTER];
        uint32 currEnc[MAX_ENCOUNTER];

        uint64 Commander_UlthokGUID;
        uint64 Lady_NazjarGUID;
        uint64 Erunak_StonespeakerGUID;
        uint64 OzumatGUID;

        void Initialize()
        {
            memset(&auiEncounter, 0, sizeof(auiEncounter));

            Commander_UlthokGUID = 0;
            Lady_NazjarGUID = 0;
            Erunak_StonespeakerGUID = 0;
            OzumatGUID = 0;
        }

        void OnCreatureCreate(Creature* pCreature, bool add)
        {
            if (!add) // for safity
                return;

            switch (pCreature->GetEntry())
            {
                case 40765: // Commander Ulthok
                    Commander_UlthokGUID = pCreature->GetGUID();
                    break;
                case 40586: // Lady Nazjar
                    Lady_NazjarGUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_COMANNDER_ULTHOK] == DONE)
                        pCreature->setFaction(14);
                    break;
                case 40788: // Erunak Stonespeaker
                    Erunak_StonespeakerGUID  = pCreature->GetGUID();
                    if (auiEncounter[DATA_LADY_NAZJAR] == DONE)
                        pCreature->setFaction(14);
                    break;
                case 42172: // Ozumat
                    OzumatGUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_ERUNAK_STONESPEAKER] == DONE)
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
                case DATA_COMANNDER_ULTHOK:
                    return Commander_UlthokGUID;
                case DATA_LADY_NAZJAR:
                    return Lady_NazjarGUID;
                case DATA_ERUNAK_STONESPEAKER:
                    return Erunak_StonespeakerGUID;
                case DATA_OZUMAT:
                    return OzumatGUID;
            }
            return 0;
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
                case DATA_COMANNDER_ULTHOK:
                case DATA_LADY_NAZJAR:
                case DATA_ERUNAK_STONESPEAKER:
                case DATA_OZUMAT:
                    auiEncounter[type] = data;
                    break;
            }

            if (auiEncounter[DATA_COMANNDER_ULTHOK] == DONE) // Commander
                if (Creature* lady = this->instance->GetCreature(Lady_NazjarGUID))
                    lady->setFaction(14);
            if (auiEncounter[DATA_LADY_NAZJAR] == DONE) // Lady
                if (Creature* erunak = this->instance->GetCreature(Erunak_StonespeakerGUID))
                    erunak->setFaction(14);
            if (auiEncounter[DATA_ERUNAK_STONESPEAKER] == DONE) // Erunak
                if (Creature* ozumat = this->instance->GetCreature(OzumatGUID))
                    ozumat->setFaction(14);

            if (data == DONE)
            {
                std::ostringstream saveStream;
                saveStream << auiEncounter[DATA_COMANNDER_ULTHOK];
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
            saveStream << auiEncounter[DATA_COMANNDER_ULTHOK];
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
        virtual uint32* GetCorrUiEncounter()
        {
            currEnc[0]=auiEncounter[DATA_OZUMAT];//3
            currEnc[1]=auiEncounter[DATA_ERUNAK_STONESPEAKER];//2
            currEnc[2]=auiEncounter[DATA_LADY_NAZJAR];//0
            currEnc[3]=auiEncounter[DATA_COMANNDER_ULTHOK];//1
            sInstanceSaveMgr->setInstanceSaveData(instance->GetInstanceId(),currEnc,MAX_ENCOUNTER);
            sInstanceSaveMgr->setBossNumber(instance->GetId(),MAX_ENCOUNTER);
            return currEnc;
        }

    };

    InstanceScript* GetInstanceScript(InstanceMap *map) const
    {
        return new instance_throne_of_the_tides_InstanceScript(map);
    }
};

void AddSC_instance_throne_of_the_tides()
{
    new instance_throne_of_the_tides();
}

/* SQL 
* UPDATE instance_template SET script='instance_throne_of_the_tides' WHERE map=643;
* UPDATE creature_template SET faction_A=35, faction_H=35 WHERE entry in (40586, 40788, 42172, 49080, 49082, 49096);
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4076598, 40765, 6, 6, 34, 0, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4058698, 40586, 6, 6, 34, 1, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (40825001, 40788, 6, 6, 34, 2, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4217298, 42172, 6, 6, 34, 3, 3, 'instance script');
*/