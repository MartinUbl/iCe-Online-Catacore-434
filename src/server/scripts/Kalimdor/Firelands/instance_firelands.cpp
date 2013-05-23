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
        std::string saveData;

        void Initialize()
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        const char* Save()
        {
            OUT_SAVE_INST_DATA;
            std::ostringstream saveStream;
            saveStream << m_auiEncounter[0];
            for (uint8 i = 1; i < MAX_ENCOUNTER; i++)
                saveStream << " " << m_auiEncounter[i];

            saveData = saveStream.str();
            SaveToDB();
            OUT_SAVE_INST_DATA_COMPLETE;
            return saveData.c_str();
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
            }
                return 0;
        }

        virtual uint32 GetData(uint32 DataId)
        {
            if (DataId < MAX_ENCOUNTER && DataId >= 0)
                return m_auiEncounter[DataId];

            return 0;
        }

        virtual void SetData(uint32 DataId, uint32 Value)
        {
            if (DataId <= MAX_ENCOUNTER && DataId >= 0)
                m_auiEncounter[DataId] = Value;
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
