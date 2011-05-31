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
#include "vortex_pinnacle.h"

/*
UPDATE instance_template SET script='instance_vortex_pinnacle' WHERE map=657;
*/

class instance_vortex_pinnacle: public InstanceMapScript
{
public:
    instance_vortex_pinnacle() : InstanceMapScript("instance_vortex_pinnacle", 657) { }

    InstanceScript* GetInstanceScript(InstanceMap* pMap) const
    {
        return new instance_vortex_pinnacle_InstanceMapScript(pMap);
    }

    struct instance_vortex_pinnacle_InstanceMapScript : public InstanceScript
    {
        instance_vortex_pinnacle_InstanceMapScript(Map* pMap): InstanceScript(pMap)
        {
            Initialize();
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string str_data;

        void Initialize()
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        bool IsEncounterInProgress() const
        {
            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                if (m_auiEncounter[i] == IN_PROGRESS) return true;

            return false;
        }

        void OnGameObjectCreate(GameObject* pGo, bool /*add*/)
        {
        }

        void OnCreatureCreate(Creature* pCreature, bool /*add*/)
        {
        }

        uint64 GetData64(uint32 identifier)
        {
            return 0;
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
                case TYPE_ERTAN:
                case TYPE_ALTAIRUS:
                case TYPE_ASAAD:
                    m_auiEncounter[type] = data;
                    break;
                default:
                    break;
            }

            if (data == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2];

                str_data = saveStream.str();

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
            std::istringstream loadStream(in);
            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2];
            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                if (m_auiEncounter[i] == IN_PROGRESS)                // Do not load an encounter as IN_PROGRESS - reset it instead.
                    m_auiEncounter[i] = NOT_STARTED;
            OUT_LOAD_INST_DATA_COMPLETE;
        }
    };
};

void AddSC_instance_vortex_pinnacle()
{
    new instance_vortex_pinnacle();
}
