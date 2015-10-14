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
#include "dragonsoul.h"

class instance_dragonsoul : public InstanceMapScript
{
public:
    instance_dragonsoul() : InstanceMapScript("instance_dragonsoul", 967) { }

    struct instance_dragonsoul_InstanceMapScript : public InstanceScript
    {
        instance_dragonsoul_InstanceMapScript(Map* pMap) : InstanceScript(pMap) { Initialize(); }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        uint32 currEnc[MAX_ENCOUNTER];
        uint32 heroicKills;

        uint64 morchokGuid;
        uint64 yorsahjGuid;
        uint64 zonozzGuid;
        uint64 hagaraGuid;
        uint64 ultraxionGuid;
        uint64 blackhornGuid;
        uint64 deathwingSpineGuid;
        uint64 deathwingMadnessGuid;

        uint64 thrallGuid;
        uint64 alexstraszaDragonGuid;
        uint64 nozdormuDragonGuid;
        uint64 yseraDragonGuid;
        uint64 kalecgosDragonGuid;

        std::string saveData;

        void Initialize()
        {
            morchokGuid             = 0;
            yorsahjGuid             = 0;
            zonozzGuid              = 0;
            hagaraGuid              = 0;
            ultraxionGuid           = 0;
            blackhornGuid           = 0;
            deathwingSpineGuid      = 0;
            deathwingMadnessGuid    = 0;

            thrallGuid              = 0;
            alexstraszaDragonGuid   = 0;
            nozdormuDragonGuid      = 0;
            yseraDragonGuid         = 0;
            kalecgosDragonGuid      = 0;

            heroicKills             = 0;
        }

        std::string GetSaveData()
        {
            OUT_SAVE_INST_DATA;

            std::ostringstream saveStream;
            saveStream << m_auiEncounter[0];
            for (uint8 i = 1; i < MAX_ENCOUNTER; i++)
                saveStream << " " << m_auiEncounter[i];

            saveStream << " " << heroicKills;

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

            loadStream >> heroicKills;

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                    m_auiEncounter[i] = NOT_STARTED;
            }

            GetCorrUiEncounter();
            OUT_LOAD_INST_DATA_COMPLETE;
        }

        void OnCreatureCreate(Creature* pCreature, bool add)
        {
            if (!add)
                return;

            switch (pCreature->GetEntry())
            {
            case 55265: // Morchok
                morchokGuid = pCreature->GetGUID();
                break;
            case 55312: // Yorsahj
                yorsahjGuid = pCreature->GetGUID();
                break;
            case 55308: // Zonozz
                zonozzGuid = pCreature->GetGUID();
                break;
            case 55689: // Hagara
                hagaraGuid = pCreature->GetGUID();
                break;
            case 55294: // Ultraxion
                ultraxionGuid = pCreature->GetGUID();
                break;
            case 56427: // Blackhorm
                blackhornGuid = pCreature->GetGUID();
                break;
            case 53879: // Spine of Deathwing
                deathwingSpineGuid = pCreature->GetGUID();
                break;
            case 56173: // Madness of Deathwing
                deathwingMadnessGuid = pCreature->GetGUID();
                break;
            }
        }

        void OnGameObjectCreate(GameObject* go, bool add)
        {
            if (add == false)
                return;
        }

        uint64 GetData64(uint32 type)
        {
            switch (type)
            {
            case TYPE_BOSS_MORCHOK:
                return morchokGuid;
            case TYPE_BOSS_YORSAHJ:
                return yorsahjGuid;
            case TYPE_BOSS_ZONOZZ:
                return zonozzGuid;
            case TYPE_BOSS_HAGARA:
                return hagaraGuid;
            case TYPE_BOSS_ULTRAXION:
                return ultraxionGuid;
            case TYPE_BOSS_BLACKHORN:
                return blackhornGuid;
            case TYPE_BOSS_SPINE_OF_DEATHWING:
                return deathwingSpineGuid;
            case TYPE_BOSS_MADNESS_OF_DEATHWING:
                return deathwingMadnessGuid;
            }
            return 0;
        }

        void SetData64(uint32 identifier, uint64 data)
        {

        }

        void Update(uint32 diff)
        {
            if (!instance->HavePlayers())
                return;
        }

        uint32 GetData(uint32 DataId)
        {
            if (DataId < MAX_ENCOUNTER)
                return m_auiEncounter[DataId];

            if (DataId == DATA_HEROIC_KILLS)
                return heroicKills;

            return 0;
        }

        void SetData(uint32 type, uint32 data)
        {
            if (type < MAX_ENCOUNTER)
                m_auiEncounter[type] = data;

            if (data == DONE)
                if (this->instance->IsHeroic())
                    heroicKills++;

            if (data == DONE)
            {
                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0];
                for (uint8 i = 1; i < MAX_ENCOUNTER; i++)
                    saveStream << " " << m_auiEncounter[i];

                saveStream << heroicKills;

                GetCorrUiEncounter();
                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
            }
        }

        virtual uint32* GetCorrUiEncounter()
        {
            currEnc[0] = m_auiEncounter[TYPE_BOSS_MADNESS_OF_DEATHWING];   // 7
            currEnc[1] = m_auiEncounter[TYPE_BOSS_BLACKHORN];              // 5
            currEnc[2] = m_auiEncounter[TYPE_BOSS_ULTRAXION];              // 4
            currEnc[3] = m_auiEncounter[TYPE_BOSS_HAGARA];                 // 3
            currEnc[4] = m_auiEncounter[TYPE_BOSS_YORSAHJ];                // 2
            currEnc[5] = m_auiEncounter[TYPE_BOSS_ZONOZZ];                 // 1
            currEnc[6] = m_auiEncounter[TYPE_BOSS_MORCHOK];                // 0
            currEnc[7] = m_auiEncounter[TYPE_BOSS_SPINE_OF_DEATHWING];     // 6
            sInstanceSaveMgr->setInstanceSaveData(instance->GetInstanceId(), currEnc, MAX_ENCOUNTER);

            return nullptr;
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap* pMap) const
    {
        return new instance_dragonsoul_InstanceMapScript(pMap);
    }
};

void AddSC_instance_dragonsoul()
{
    new instance_dragonsoul();
}