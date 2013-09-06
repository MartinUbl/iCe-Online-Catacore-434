/*
 * Copyright (C) 2006-2013 iCe Online <http://ice-wow.eu>
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
#include "firelands.h"

class instance_firelands: public InstanceMapScript
{
public:
    instance_firelands() : InstanceMapScript("instance_firelands", 720) { }

    struct instance_firelands_InstanceMapScript : public InstanceScript
    {
        instance_firelands_InstanceMapScript(Map* pMap) : InstanceScript(pMap) {Initialize();}

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        uint64 riplimbGuid;
        uint64 ragefaceGuid;
        uint64 shannoxGuid;
        uint64 bethtilacGUID;
        uint64 rhyolithGUID;
        uint64 alysrazorGUID;
        uint64 balerocGUID;
        uint64 balerocDoorGUID;

        std::string saveData;

        void Initialize()
        {
            riplimbGuid =       0;
            ragefaceGuid =      0;
            shannoxGuid =       0;
            alysrazorGUID =     0;
            bethtilacGUID =     0;
            rhyolithGUID=       0;
            balerocGUID =       0;
            balerocDoorGUID =   0;
            memset(m_auiEncounter, 0, sizeof(uint32) * MAX_ENCOUNTER);
        }

        std::string GetSaveData()
        {
            OUT_SAVE_INST_DATA;

            std::ostringstream saveStream;
            saveStream << m_auiEncounter[0];
            for (uint8 i = 1; i < MAX_ENCOUNTER; i++)
                saveStream << " " << m_auiEncounter[i];

            OUT_SAVE_INST_DATA_COMPLETE;
            return saveStream.str();
        }

        void Load(const char* chrIn)
        {
            if (!chrIn)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(chrIn);

            std::istringstream loadStream(chrIn);
            for (uint8 i = 0; i < MAX_ENCOUNTER; i++)
                loadStream >> m_auiEncounter[i];

            for(uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                    m_auiEncounter[i] = NOT_STARTED;
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

        void OnCreatureCreate(Creature* pCreature, bool add)
        {
            if (!add)
                return;

            switch (pCreature->GetEntry())
            {
                case 53694:
                    riplimbGuid = pCreature->GetGUID();
                    break;
                case 53695:
                    ragefaceGuid = pCreature->GetGUID();
                    break;
                case 53691:
                    shannoxGuid = pCreature->GetGUID();
                    break;
                case 52530:
                    alysrazorGUID = pCreature->GetGUID();
                    break;
                case 52558:
                    rhyolithGUID = pCreature->GetGUID();
                    break;
                case 52498:
                    bethtilacGUID = pCreature->GetGUID();
                    break;
                case 53494:
                    balerocGUID = pCreature->GetGUID();
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go, bool add)
        {
            if(add == false)
                return;

            if (go->GetEntry() == 209066 && go->GetPositionX() < 0.0f ) // Baleroc's front door
            {
                balerocDoorGUID = go->GetGUID();
            }
        }

        uint64 GetData64(uint32 type)
        {
            switch (type)
            {
                case TYPE_SHANNOX:
                    return shannoxGuid;
                case DATA_RAGEFACE_GUID:
                    return ragefaceGuid;
                case DATA_RIPLIMB_GUID:
                    return riplimbGuid;
                case TYPE_RHYOLITH:
                    return rhyolithGUID;
                case TYPE_BETHTILAC:
                    return bethtilacGUID;
                case TYPE_ALYSRAZOR:
                    return alysrazorGUID;
                case TYPE_BALEROC:
                    return balerocGUID;
                case DATA_BALEROC_FRONT_DOOR:
                    return balerocDoorGUID;
            }
                return 0;
        }

        bool CheckWipe()
        {
            Map::PlayerList const &players = instance->GetPlayers();
            for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
            {
                Player* player = itr->getSource();
                if (player->isGameMaster())
                    continue;

                if (player->isAlive())
                    return false;
            }

            return true;
        }

        void Update(uint32 diff)
        {
            if (CheckWipe() && m_auiEncounter[TYPE_ALYSRAZOR] == IN_PROGRESS)
                m_auiEncounter[TYPE_ALYSRAZOR] = FAIL;
        };

        uint32 GetData(uint32 DataId)
        {
            if (DataId < MAX_ENCOUNTER)
                return m_auiEncounter[DataId];

            return 0;
        }

        void SetData(uint32 type, uint32 data)
        {
            if (type < MAX_ENCOUNTER)
                m_auiEncounter[type] = data;

               if (/*m_auiEncounter[0] == DONE // TYPE_BETHTILAC
                && */m_auiEncounter[1] == DONE // TYPE_RHYOLITH
                && m_auiEncounter[2] == DONE // TYPE_ALYSRAZOR
                && m_auiEncounter[3] == DONE ) // TYPE_SHANNOX
                {
                    if (GameObject* go = this->instance->GetGameObject(balerocDoorGUID))
                        go->Delete();

                    if (Creature* baleroc = this->instance->GetCreature(this->GetData64(TYPE_BALEROC)))
                        baleroc->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE);
                }

            if (data == DONE)
            {
                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0];
                for (uint8 i = 1; i < MAX_ENCOUNTER; i++)
                    saveStream << " " << m_auiEncounter[i];

                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
            }
        }

        virtual uint32* GetUiEncounter(){ return m_auiEncounter; }
        virtual uint32 GetMaxEncounter(){ return MAX_ENCOUNTER; }
    };

    InstanceScript* GetInstanceScript(InstanceMap* pMap) const
    {
        return new instance_firelands_InstanceMapScript(pMap);
    }
};

void AddSC_instance_firelands()
{
    new instance_firelands();
}
