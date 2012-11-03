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
#include "deadmines.h"

enum data
{
    DATA_GLUBTOK = 0,
    DATA_HELIX_GEARBREAKER,
    DATA_FOE_REAPER_5000,
    DATA_ADMIRAL_RIPSNARL,
    DATA_CAPTAIN_COOKIE,
    DATA_VANESSA_VANCLEEF
};

#define MAX_ENCOUNTER 6

class instance_the_deadmines : public InstanceMapScript
{
public:
    instance_the_deadmines() : InstanceMapScript("instance_the_deadmines", 36) { }

    struct instance_the_deadmines_InstanceScript : public InstanceScript
    {
        instance_the_deadmines_InstanceScript(Map* pMap) : InstanceScript(pMap) {Initialize();};

        uint32 auiEncounter[MAX_ENCOUNTER];

        uint64 GlubtokGUID;
        uint64 Helix_GearbreakerGUID;
        uint64 Foe_Reaper_5000GUID;
        uint64 Admiral_RipsnarlGUID;
        uint64 Captain_CookieGUID;
        uint64 Vanesa_VancleefGUID;

        void Initialize()
        {
            memset(&auiEncounter, 0, sizeof(auiEncounter));

            GlubtokGUID = 0;
            Helix_GearbreakerGUID = 0;
            Foe_Reaper_5000GUID = 0;
            Admiral_RipsnarlGUID = 0;
            Captain_CookieGUID = 0;
            Vanesa_VancleefGUID = 0;
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
                case 47162: // Glubtok
                    GlubtokGUID = pCreature->GetGUID();
                    break;
                case 47296: // Helix Gearbreaker
                    Helix_GearbreakerGUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_GLUBTOK] == DONE)
                        pCreature->setFaction(14);
                    break;
                case 43778: // Foe Reaper 5000
                    Foe_Reaper_5000GUID  = pCreature->GetGUID();
                    if (auiEncounter[DATA_HELIX_GEARBREAKER] == DONE)
                        pCreature->setFaction(14);
                    break;
                case 47626: // Admiral Ripsnarl
                    Admiral_RipsnarlGUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_FOE_REAPER_5000] == DONE)
                        pCreature->setFaction(14);
                    break;
                case 47739: // Captain Cookie
                    Captain_CookieGUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_ADMIRAL_RIPSNARL] == DONE)
                        pCreature->setFaction(14);
                    break;
                case 49541: // Vanessa VanCleef
                    Vanesa_VancleefGUID = pCreature->GetGUID();
                    if (auiEncounter[DATA_VANESSA_VANCLEEF] == DONE)
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
                case DATA_GLUBTOK:
                    return GlubtokGUID;
                case DATA_HELIX_GEARBREAKER:
                    return Helix_GearbreakerGUID;
                case DATA_FOE_REAPER_5000:
                    return Foe_Reaper_5000GUID;
                case DATA_ADMIRAL_RIPSNARL:
                    return Admiral_RipsnarlGUID;
                case DATA_CAPTAIN_COOKIE:
                    return Captain_CookieGUID;
                case DATA_VANESSA_VANCLEEF:
                    return Vanesa_VancleefGUID;
            }
            return 0;
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
                case DATA_GLUBTOK:
                case DATA_HELIX_GEARBREAKER:
                case DATA_FOE_REAPER_5000:
                case DATA_ADMIRAL_RIPSNARL:
                case DATA_CAPTAIN_COOKIE:
                case DATA_VANESSA_VANCLEEF:
                    auiEncounter[type] = data;
                    break;
            }

            if (auiEncounter[DATA_GLUBTOK] == DONE) // Glubtok
                if (Creature* helix = this->instance->GetCreature(Helix_GearbreakerGUID))
                    helix->setFaction(14);
            if (auiEncounter[DATA_HELIX_GEARBREAKER] == DONE) // Helix
                if (Creature* reaper = this->instance->GetCreature(Foe_Reaper_5000GUID))
                    reaper->setFaction(14);
            if (auiEncounter[DATA_FOE_REAPER_5000] == DONE) // Reaper
                if (Creature* admiral = this->instance->GetCreature(Admiral_RipsnarlGUID))
                    admiral->setFaction(14);
            if (auiEncounter[DATA_ADMIRAL_RIPSNARL] == DONE) // Admiral
                if (Creature* captain = this->instance->GetCreature(Captain_CookieGUID))
                    captain->setFaction(14);
            if (auiEncounter[DATA_CAPTAIN_COOKIE] == DONE) // Captain
                if (Creature* vanesa = this->instance->GetCreature(Vanesa_VancleefGUID))
                    vanesa->setFaction(14);

            if (data == DONE)
            {
                std::ostringstream saveStream;
                saveStream << auiEncounter[DATA_GLUBTOK];
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
            saveStream << auiEncounter[DATA_GLUBTOK];
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
        return new instance_the_deadmines_InstanceScript(map);
    }
};

void AddSC_instance_deadmines()
{
    new instance_the_deadmines();
}


/* SQL 
* UPDATE instance_template SET script='instance_the_deadmines' WHERE map=36;
* UPDATE creature_template SET faction_A=35, faction_H=35 WHERE entry in (47296, 43778, 47626, 47739, 49541, 48941, 48940, 48943, 48944);
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4716298, 47162, 6, 6, 34, 0, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4729698, 47296, 6, 6, 34, 1, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4377898, 43778, 6, 6, 34, 2, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4762698, 47626, 6, 6, 34, 3, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4773998, 47739, 6, 6, 34, 4, 3, 'instance script');
* INSERT INTO `creature_ai_scripts` (id, creature_id, event_type, event_flags, action1_type, action1_param1, action1_param2, comment) VALUES (4954198, 49541, 6, 6, 34, 5, 3, 'instance script');
*/