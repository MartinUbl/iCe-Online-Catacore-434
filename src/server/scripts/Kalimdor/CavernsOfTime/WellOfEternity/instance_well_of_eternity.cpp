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
#include "well_of_eternity.h"

#define MAX_ENCOUNTER      (3)

const Position legionDemonSpawnPositions[2] =
{
    // These are correct !!! , but i will use temporary points -> at stairs
    //{3442.0f,-5067.8f, 213.6f, 2.12f},     //LEFT
    //{3448.4f,-5065.2f, 213.6f, 2.12f},     //RIGHT

    {3373.7f,-4957.7f, 181.1f, 2.12f},     //LEFT
    {3379.2f,-4954.7f, 181.1f, 2.12f},     //RIGHT
};

const Position midPositions[2] =
{
    {3334.4f,-4896.5f, 181.1f, 2.12f},     //LEFT
    {3341.1f,-4892.4f, 181.1f, 2.12f},     //RIGHT
};

const Position midPositions2[2] =
{
    {3329.98f,-4892.0f, 181.1f, 2.12f},     //LEFT
    {3337.96f,-4886.74f, 181.1f, 2.12f},    //RIGHT
};

const Position frontEndPositions[2] =
{
    {3286.0f,-4821.0f, 181.5f, 2.12f},     //LEFT
    {3290.0f,-4818.0f, 181.5f, 2.12f},     //RIGHT
};

class instance_well_of_eternity : public InstanceMapScript
{
public:
    instance_well_of_eternity() : InstanceMapScript("instance_well_of_eternity", 939) { }

    struct instance_well_of_eternity_InstanceMapScript : public InstanceScript
    {
        instance_well_of_eternity_InstanceMapScript(Map *pMap) : InstanceScript(pMap) { Initialize(); }

        // Timers
        uint32 legionTimer;
        DemonWave waveCounter;

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        uint32 currEnc[MAX_ENCOUNTER];

        // Creature GUIDS
        uint64 perotharnGUID;
        uint64 queenGUID;
        uint64 mannorothGUID;
        uint64 varothenGUID;

        // Go GUIDS

        std::string saveData;

        void Initialize()
        {
            perotharnGUID = 0;
            queenGUID = 0;
            mannorothGUID = 0;
            varothenGUID = 0;

            legionTimer = 2000;
            waveCounter = WAVE_ONE;

            memset(m_auiEncounter, 0, sizeof(uint32)* MAX_ENCOUNTER);
            GetCorrUiEncounter();
        }

        void InitDemonSpeed(Creature * pDemon)
        {
            pDemon->SetWalk(true);
            pDemon->SetSpeed(MOVE_WALK, 1.28571f, true);
        }

        void Update(uint32 diff)
        {
            if (!instance->HavePlayers())
                return;
            
            if (legionTimer <= diff)
            {
                if (Creature * pPerotharn = this->instance->GetCreature(GetData64(DATA_PEROTHARN)))
                {
                    if (pPerotharn->isDead())
                        return;

                    Creature * pDemon = NULL;

                    switch (waveCounter)
                    {
                        case WAVE_ONE:
                            pDemon = pPerotharn->SummonCreature(LEGION_DEMON_ENTRY, legionDemonSpawnPositions[DIRECTION_LEFT]);
                            if (pDemon)
                            {
                                InitDemonSpeed(pDemon);
                                pDemon->AI()->SetData(DEMON_DATA_DIRECTION, DIRECTION_LEFT);
                                pDemon->AI()->SetData(DEMON_DATA_WAVE, waveCounter);
                                pDemon->GetMotionMaster()->MovePoint(WP_MID, midPositions[DIRECTION_LEFT]);
                            }

                            pDemon = pPerotharn->SummonCreature(LEGION_DEMON_ENTRY, legionDemonSpawnPositions[DIRECTION_RIGHT]);
                            if (pDemon)
                            {
                                InitDemonSpeed(pDemon);
                                pDemon->AI()->SetData(DEMON_DATA_DIRECTION, DIRECTION_RIGHT);
                                pDemon->AI()->SetData(DEMON_DATA_WAVE, waveCounter);
                                pDemon->GetMotionMaster()->MovePoint(WP_MID, midPositions[DIRECTION_RIGHT]);
                            }
                            waveCounter = WAVE_TWO;
                            break;
                        case WAVE_TWO:
                            pDemon = pPerotharn->SummonCreature(LEGION_DEMON_ENTRY, legionDemonSpawnPositions[DIRECTION_LEFT]);
                            if (pDemon)
                            {
                                InitDemonSpeed(pDemon);
                                pDemon->AI()->SetData(DEMON_DATA_DIRECTION, DIRECTION_LEFT);
                                pDemon->AI()->SetData(DEMON_DATA_WAVE, waveCounter);
                                pDemon->GetMotionMaster()->MovePoint(WP_MID, midPositions2[DIRECTION_LEFT]);
                            }

                            pDemon = pPerotharn->SummonCreature(LEGION_DEMON_ENTRY, legionDemonSpawnPositions[DIRECTION_RIGHT]);
                            if (pDemon)
                            {
                                InitDemonSpeed(pDemon);
                                pDemon->AI()->SetData(DEMON_DATA_DIRECTION, DIRECTION_RIGHT);
                                pDemon->AI()->SetData(DEMON_DATA_WAVE, waveCounter);
                                pDemon->GetMotionMaster()->MovePoint(WP_MID, midPositions2[DIRECTION_RIGHT]);
                            }
                            waveCounter = WAVE_THREE;
                            break;
                        case WAVE_THREE:
                            pDemon = pPerotharn->SummonCreature(LEGION_DEMON_ENTRY, legionDemonSpawnPositions[DIRECTION_LEFT]);
                            if (pDemon)
                            {
                                InitDemonSpeed(pDemon);
                                pDemon->AI()->SetData(DEMON_DATA_DIRECTION, DIRECTION_STRAIGHT);
                                pDemon->AI()->SetData(DEMON_DATA_WAVE, waveCounter);
                                pDemon->GetMotionMaster()->MovePoint(WP_END, frontEndPositions[DIRECTION_LEFT]);
                            }

                            pDemon = pPerotharn->SummonCreature(LEGION_DEMON_ENTRY, legionDemonSpawnPositions[DIRECTION_RIGHT]);
                            if (pDemon)
                            {
                                InitDemonSpeed(pDemon);
                                pDemon->AI()->SetData(DEMON_DATA_DIRECTION, DIRECTION_STRAIGHT);
                                pDemon->AI()->SetData(DEMON_DATA_WAVE, waveCounter);
                                pDemon->GetMotionMaster()->MovePoint(WP_END, frontEndPositions[DIRECTION_RIGHT]);
                            }
                            waveCounter = WAVE_ONE;
                            break;
                    }
                }
                legionTimer = 1500;
            }
            else legionTimer -= diff;
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
                case PEROTHARN_ENTRY:
                    perotharnGUID = pCreature->GetGUID();
                    break;
                case QUEEN_AZSHARA_ENTRY:
                    queenGUID = pCreature->GetGUID();
                    break;
                case MANNOROTH_ENTRY:
                    mannorothGUID = pCreature->GetGUID();
                    break;
                case VAROTHEN_ENTRY:
                    varothenGUID = pCreature->GetGUID();
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go, bool add)
        {
            if (add == false)
                return;

           /* switch (go->GetEntry())
            {
                default:
                    break;
            }*/
        }

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

            if (data == DONE)
            {
                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0];
                for (uint8 i = 1; i < MAX_ENCOUNTER; i++)
                    saveStream << " " << m_auiEncounter[i];

                GetCorrUiEncounter();
                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
            }
        }

        uint64 GetData64(uint32 type)
        {
            switch (type)
            {
                case DATA_PEROTHARN:
                    return perotharnGUID;
                case DATA_QUEEN_AZSHARA:
                    return queenGUID;
                case DATA_CAPTAIN_VAROTHEN:
                    return varothenGUID;
                case DATA_MANNOROTH:
                    return mannorothGUID;
            }
            return 0;
        }


        void SetData64(uint32 identifier, uint64 data)
        {
            /*switch (identifier)
            {
                default:
                    break;
            }*/
        }

        virtual uint32* GetCorrUiEncounter()
        {
            currEnc[0] = m_auiEncounter[DATA_PEROTHARN];
            currEnc[1] = m_auiEncounter[DATA_QUEEN_AZSHARA];
            currEnc[2] = m_auiEncounter[DATA_MANNOROTH];
            sInstanceSaveMgr->setInstanceSaveData(instance->GetInstanceId(), currEnc, MAX_ENCOUNTER);

            return NULL;
        }

    };

    InstanceScript* GetInstanceScript(InstanceMap* pMap) const
    {
        return new instance_well_of_eternity_InstanceMapScript(pMap);
    }
};

void AddSC_instance_well_of_eternity()
{
    new instance_well_of_eternity();
}

// UPDATE `instance_template` SET `script`='instance_well_of_eternity' WHERE  `map`=939 LIMIT 1;