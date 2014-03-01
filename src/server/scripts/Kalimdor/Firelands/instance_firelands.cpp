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


        uint32 unlockTimer;
        uint32 spawnBridgeTimer;
        uint32 unAuraTimer;

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        uint32 currEnc[MAX_ENCOUNTER];
        uint64 riplimbGuid;
        uint64 ragefaceGuid;
        uint64 shannoxGuid;
        uint64 bethtilacGUID;
        uint64 rhyolithGUID;
        uint64 alysrazorGUID;
        uint64 balerocGUID;
        uint64 staghelmGUID;
        uint64 ragnarosGUID;
        uint64 bridgeStalkerGUID;

        uint64 balerocDoorGUID;
        uint64 bridgeDoorGUID;
        uint64 bridgeGUID;

        std::string saveData;

        void Initialize()
        {
            riplimbGuid =       0;
            ragefaceGuid =      0;
            shannoxGuid =       0;
            alysrazorGUID =     0;
            bethtilacGUID =     0;
            rhyolithGUID =      0;
            balerocGUID =       0;
            staghelmGUID =      0;
            ragnarosGUID =      0;
            bridgeStalkerGUID = 0;
            balerocDoorGUID =   0;
            bridgeDoorGUID=     0;
            bridgeGUID =        0;

            unlockTimer         = 10000;
            spawnBridgeTimer    = 500;
            unAuraTimer         = 30000;
            memset(m_auiEncounter, 0, sizeof(uint32) * MAX_ENCOUNTER);
            GetCorrUiEncounter();
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

            GetCorrUiEncounter();
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
                case 52571:
                    staghelmGUID = pCreature->GetGUID();
                    break;
                case 52409:
                    ragnarosGUID = pCreature->GetGUID();
                    break;
                case 52977:
                    bridgeStalkerGUID = pCreature->GetGUID();
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go, bool add)
        {
            if(add == false)
                return;

            switch (go->GetEntry())
            {
                case 209066:
                    if (go->GetEntry() == 209066 && go->GetPositionX() < 0.0f ) // Baleroc's front door
                        balerocDoorGUID = go->GetGUID();
                    break;
                case 5010734:
                    bridgeGUID = go->GetGUID();
                    break;

                case 208906:
                    bridgeDoorGUID = go->GetGUID();
                    break;

                default:
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
                case TYPE_RHYOLITH:
                    return rhyolithGUID;
                case TYPE_BETHTILAC:
                    return bethtilacGUID;
                case TYPE_ALYSRAZOR:
                    return alysrazorGUID;
                case TYPE_BALEROC:
                    return balerocGUID;
                case TYPE_STAGHELM:
                    return staghelmGUID;
                case TYPE_RAGNAROS:
                    return ragnarosGUID;
                case DATA_BRIDGE_SPAWN:
                    return bridgeStalkerGUID;
                case DATA_BALEROC_FRONT_DOOR:
                    return balerocDoorGUID;
                case DATA_BRIDGE_DOOR:
                    return bridgeDoorGUID;
            }
                return 0;
        }

        void SetData64(uint32 identifier, uint64 data)
        {
            switch(identifier)
            {
                case DATA_BRIDGE_DOOR:
                    bridgeDoorGUID = data;
                break;

                default:
                    break;
            }
        }

        void Update(uint32 diff)
        {
            if (!instance->HavePlayers())
                return;

            if (unlockTimer < diff)
            {
                Creature * pBethtilac = this->instance->GetCreature(this->GetData64(TYPE_BETHTILAC));
                Creature * pShannox = this->instance->GetCreature(this->GetData64(TYPE_SHANNOX));
                Creature * pRhyolith = this->instance->GetCreature(this->GetData64(TYPE_RHYOLITH));
                Creature * pAlysrazor = this->instance->GetCreature(this->GetData64(TYPE_ALYSRAZOR));
                Creature * pBaleroc = this->instance->GetCreature(this->GetData64(TYPE_BALEROC));


                if (pBethtilac && pShannox && pRhyolith && pAlysrazor && pBaleroc)
                {
                    if (pBethtilac->isDead() && pShannox->isDead() && pRhyolith->isDead() && pAlysrazor->isDead())
                    {
                        if (pBaleroc->isAlive())
                            pBaleroc->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE);

                        if (GameObject * door1 = this->instance->GetGameObject(this->GetData64(DATA_BALEROC_FRONT_DOOR)))
                        {
                            door1->Delete();
                        }
                    }
                }

                unlockTimer = 60000; // Check it every minute
            }
            else unlockTimer -= diff;

            if (spawnBridgeTimer <= diff)
            {
                Creature * pBaleroc = this->instance->GetCreature(this->GetData64(TYPE_BALEROC));
                Creature * pBridgeStalker = this->instance->GetCreature(this->GetData64(DATA_BRIDGE_SPAWN));

                GameObject * bridge = NULL;

                if (pBridgeStalker && pBaleroc && pBaleroc->isDead() && bridgeGUID == 0)
                {
                    bridge = pBridgeStalker->SummonGameObject(5010734,247.0f,-64.0f,62.0f,3.15f,0,0,0,0,0);
                    if(bridge)
                        bridgeGUID = bridge->GetGUID();
                }

                if (GameObject * door2 = this->instance->GetGameObject(this->GetData64(DATA_BRIDGE_DOOR))) // Unlock  bridge door
                {
                    if(pBaleroc && pBaleroc->isDead())
                        door2->Delete();
                }
                spawnBridgeTimer = 23000;
            }
            else spawnBridgeTimer -= diff;

            if (unAuraTimer <= diff)
            {
                if (GetData(TYPE_SHANNOX) != IN_PROGRESS)
                    DoRemoveAurasDueToSpellOnPlayers(99837); // Crystal Prison Trap Effect

                if (GetData(TYPE_RHYOLITH) != IN_PROGRESS)
                    DoRemoveAurasDueToSpellOnPlayers(98226); // Balance Bar

                if (GetData(TYPE_ALYSRAZOR) != IN_PROGRESS)
                    DoRemoveAurasDueToSpellOnPlayers(97128); // Molten Feather Bar

                if (GetData(TYPE_BALEROC) != IN_PROGRESS)
                    DoRemoveAurasDueToSpellOnPlayers(99252); // Blaze of Glory

                if (GetData(TYPE_STAGHELM) != IN_PROGRESS && this->instance->IsHeroic())
                {
                    DoRemoveAurasDueToSpellOnPlayers(98229); // Concentration bar
                    // 4 kinds of concetration buffs
                    DoRemoveAurasDueToSpellOnPlayers(98254); // uncommon
                    DoRemoveAurasDueToSpellOnPlayers(98253); // rare
                    DoRemoveAurasDueToSpellOnPlayers(98252); // epic
                    DoRemoveAurasDueToSpellOnPlayers(98245); // legendary
                }

                if (GetData(TYPE_RAGNAROS) != IN_PROGRESS && this->instance->IsHeroic())
                {
                    //Deluge
                    DoRemoveAurasDueToSpellOnPlayers(100713);
                    DoRemoveAurasDueToSpellOnPlayers(101015);
                    //Superheated
                    DoRemoveAurasDueToSpellOnPlayers(100594);
                    DoRemoveAurasDueToSpellOnPlayers(100915);
                }
            }
            else unAuraTimer -= diff;
        }

        uint32 GetData(uint32 DataId)
        {
            if (DataId < MAX_ENCOUNTER)
                return m_auiEncounter[DataId];

            return 0;
        }

        void SetData(uint32 type, uint32 data)
        {
            if (type == DATA_BRIDGE_SPAWN)
            {
                spawnBridgeTimer = data;
                return;
            }

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

        virtual uint32* GetCorrUiEncounter()
        {
            currEnc[0]=m_auiEncounter[TYPE_ALYSRAZOR];//alysrazor
            currEnc[1]=m_auiEncounter[TYPE_SHANNOX];//shannox
            currEnc[2]=m_auiEncounter[TYPE_RHYOLITH];//rhyolit
            currEnc[3]=m_auiEncounter[TYPE_RAGNAROS];//ragnaros
            currEnc[4]=m_auiEncounter[TYPE_BALEROC];//baleroc
            currEnc[5]=m_auiEncounter[TYPE_BETHTILAC];//bethilac
            currEnc[6]=m_auiEncounter[TYPE_STAGHELM];//majordomo
            sInstanceSaveMgr->setInstanceSaveData(instance->GetInstanceId(),currEnc,MAX_ENCOUNTER);

            return NULL;
        }
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
