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
#include "blackrock_caverns.h"

/*
UPDATE instance_template SET script='instance_blackrock_caverns' WHERE map=645;
*/

class instance_blackrock_caverns : public InstanceMapScript
{
public:
    instance_blackrock_caverns() : InstanceMapScript("instance_blackrock_caverns", 645) { }

    InstanceScript* GetInstanceScript(InstanceMap* pMap) const
    {
        return new instance_blackrock_caverns_InstanceMapScript(pMap);
    }

    struct instance_blackrock_caverns_InstanceMapScript : public InstanceScript
    {
        instance_blackrock_caverns_InstanceMapScript(Map* pMap): InstanceScript(pMap)
        {
            Initialize();
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        uint32 currEnc[MAX_ENCOUNTER];
        std::string str_data;
        uint64 m_auiCorla_ZealotGUID[3];
        uint64 m_uiCorlaGUID;

        void Initialize()
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
            memset(&m_auiCorla_ZealotGUID, 0, sizeof(m_auiCorla_ZealotGUID));
            m_uiCorlaGUID = 0;
        }

        void OnGameObjectCreate(GameObject* pGo, bool /*add*/)
        {
        }

        void OnCreatureCreate(Creature* pCreature, bool /*add*/)
        {
            if(!pCreature)
                return;

            if(pCreature->GetEntry() == 39679) // Corla
                m_uiCorlaGUID = pCreature->GetGUID();

            if(pCreature->GetEntry() == 50284) // Corla's Zealot
                m_auiCorla_ZealotGUID[0] ? (m_auiCorla_ZealotGUID[1] ? m_auiCorla_ZealotGUID[2] = pCreature->GetGUID() : m_auiCorla_ZealotGUID[1] = pCreature->GetGUID()) : m_auiCorla_ZealotGUID[0] = pCreature->GetGUID();
        }

        uint64 GetData64(uint32 identifier)
        {
            switch(identifier)
            {
            case DATA_CORLA:
                return m_uiCorlaGUID;
                break;
            case DATA_CORLA_ZEALOT_1:
                return m_auiCorla_ZealotGUID[0];
                break;
            case DATA_CORLA_ZEALOT_2:
                return m_auiCorla_ZealotGUID[1];
                break;
            case DATA_CORLA_ZEALOT_3:
                return m_auiCorla_ZealotGUID[2];
                break;
            default:
                return 0;
                break;
            }
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
                case TYPE_ROMOGG:
                case TYPE_CORLA:
                case TYPE_KARSH:
                case TYPE_BEAUTY:
                case TYPE_OBSIDIUS:
                    m_auiEncounter[type] = data;
                    break;
                default:
                    break;
            }

            if (data == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2] << " "
                    << m_auiEncounter[3] << " " << m_auiEncounter[4];

                str_data = saveStream.str();

                GetCorrUiEncounter();
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
            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2] >> m_auiEncounter[3] >> m_auiEncounter[4];
            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                if (m_auiEncounter[i] == IN_PROGRESS)                // Do not load an encounter as IN_PROGRESS - reset it instead.
                    m_auiEncounter[i] = NOT_STARTED;
            OUT_LOAD_INST_DATA_COMPLETE;
        }
        virtual uint32* GetUiEncounter(){return m_auiEncounter;}
        virtual uint32 GetMaxEncounter(){return MAX_ENCOUNTER;}
        virtual uint32* GetCorrUiEncounter()
        {
            currEnc[0]=m_auiEncounter[TYPE_ROMOGG];//0
            currEnc[1]=m_auiEncounter[TYPE_KARSH];//2
            currEnc[2]=m_auiEncounter[TYPE_CORLA];//1
            currEnc[3]=m_auiEncounter[TYPE_BEAUTY];//3
            currEnc[4]=m_auiEncounter[TYPE_OBSIDIUS];//4
            sInstanceSaveMgr->setInstanceSaveData(instance->GetInstanceId(),currEnc,MAX_ENCOUNTER);
            sInstanceSaveMgr->setBossNumber(instance->GetId(),MAX_ENCOUNTER);
            return currEnc;
        }
    };
};

void AddSC_instance_blackrock_caverns()
{
    new instance_blackrock_caverns();
}
