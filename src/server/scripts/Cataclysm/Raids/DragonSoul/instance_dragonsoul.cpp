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
        uint32 hagaraTrashKills;
        uint32 aspectsPrepareToChannel;
        uint32 drakesKilled;
        uint32 platesDestroyed;

        uint64 morchokGuid;
        uint64 yorsahjGuid;
        uint64 zonozzGuid;
        uint64 hagaraGuid;
        uint64 ultraxionGuid;
        uint64 blackhornGuid;
        uint64 deathwingSummitGuid;
        uint64 deathwingSpineGuid;
        uint64 deathwingMadnessGuid;

        uint64 thrallGuid;
        uint64 alexstraszaDragonGuid;
        uint64 nozdormuDragonGuid;
        uint64 yseraDragonGuid;
        uint64 kalecgosDragonGuid;

        uint64 thrallSummitGuid;
        uint64 alexstraszaSummitGuid;
        uint64 nozdormuSummitGuid;
        uint64 yseraSummitGuid;
        uint64 kalecgosSummitGuid;

        uint64 allianceShipGuid;

        uint64 valeeraGuid;
        uint64 eiendormiGuid;
        uint64 nethestraszGuid;
        std::vector<uint64> ultraxionDrakesGUIDs;
        std::vector<uint64> teleportGUIDs;
        std::string saveData;

        void Initialize()
        {
            morchokGuid             = 0;
            yorsahjGuid             = 0;
            zonozzGuid              = 0;
            hagaraGuid              = 0;
            ultraxionGuid           = 0;
            blackhornGuid           = 0;
            deathwingSummitGuid     = 0;
            deathwingSpineGuid      = 0;
            deathwingMadnessGuid    = 0;

            thrallGuid              = 0;
            alexstraszaDragonGuid   = 0;
            nozdormuDragonGuid      = 0;
            yseraDragonGuid         = 0;
            kalecgosDragonGuid      = 0;

            thrallGuid              = 0;
            alexstraszaDragonGuid   = 0;
            nozdormuDragonGuid      = 0;
            yseraDragonGuid         = 0;
            kalecgosDragonGuid      = 0;

            allianceShipGuid        = 0;

            heroicKills             = 0;
            hagaraTrashKills        = 0;
            drakesKilled            = 0;
            aspectsPrepareToChannel = 0;
            platesDestroyed         = 0;

            valeeraGuid             = 0;
            eiendormiGuid           = 0;
            nethestraszGuid         = 0;
            teleportGUIDs.clear();
            ultraxionDrakesGUIDs.clear();

            GetCorrUiEncounter();
        }

        std::string GetSaveData()
        {
            OUT_SAVE_INST_DATA;

            std::ostringstream saveStream;
            saveStream << m_auiEncounter[0];
            for (uint8 i = 1; i < MAX_ENCOUNTER; i++)
                saveStream << " " << m_auiEncounter[i];

            saveStream << " " << heroicKills;
            saveStream << " " << hagaraTrashKills;

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
            loadStream >> hagaraTrashKills;

            if (hagaraTrashKills < 35)
                hagaraTrashKills = 0;

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                    m_auiEncounter[i] = NOT_STARTED;
            }

            GetCorrUiEncounter();
            OUT_LOAD_INST_DATA_COMPLETE;
        }

        void OnPlayerEnter(Player * player) override
        {
            if (InstanceScript * pInstance = player->GetInstanceScript())
            {
                if (!player->IsGameMaster())
                {
                    if (pInstance->instance->IsRaid() && pInstance->IsEncounterInProgress())
                    {
                        player->RepopAtGraveyard();
                        return;
                    }
                }
            }
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
            case NPC_TRAVEL_TO_WYRMREST_TEMPLE:
            case NPC_TRAVEL_TO_WYRMREST_BASE:
            case NPC_TRAVEL_TO_WYRMREST_SUMMIT:
            case NPC_TRAVEL_TO_EYE_OF_ETERNITY:
            case NPC_TRAVEL_TO_DECK:
            case NPC_TRAVEL_TO_MAELSTORM:
            case NPC_ANDORGOS:
                teleportGUIDs.push_back(pCreature->GetGUID());
                break;
            case NPC_VALEERA:
                valeeraGuid = pCreature->GetGUID();
                break;
            case NPC_EIENDORMI:
                eiendormiGuid = pCreature->GetGUID();
                break;
            case NPC_NETHESTRASZ:
                nethestraszGuid = pCreature->GetGUID();
                break;
            case NPC_DEATHWING_WYRMREST_TEMPLE:
                deathwingSummitGuid = pCreature->GetGUID();
                break;
            case NPC_TWILIGHT_ASSAULTER:
            case NPC_TWILIGHT_ASSAULTERESS:
                ultraxionDrakesGUIDs.push_back(pCreature->GetGUID());
                break;
            case NPC_ALEXSTRASZA_THE_LIFE_BINDER:
                alexstraszaSummitGuid = pCreature->GetGUID();
                break;
            case NPC_YSERA_THE_AWAKENED:
                yseraSummitGuid = pCreature->GetGUID();
                break;
            case NPC_KALECGOS:
                kalecgosSummitGuid = pCreature->GetGUID();
                break;
            case NPC_NOZDORMU_THE_TIMELESS_ONE:
                nozdormuSummitGuid = pCreature->GetGUID();
                break;
            case NPC_THRALL:
                thrallSummitGuid = pCreature->GetGUID();
                break;
            default:
                break;
            }
        }

        void OnGameObjectCreate(GameObject* go, bool add)
        {
            if (add == false)
                return;

            if (go->GetEntry() == GO_ALLIANCE_SHIP)
            {
                allianceShipGuid = go->GetGUID();
                if (GetData(TYPE_BOSS_ULTRAXION) == DONE)
                    go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);
                else
                    go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);
                go->UpdateObjectVisibility();
            }
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
            else if (DataId == DATA_HAGARA_INTRO_TRASH)
                return hagaraTrashKills;
            else if (DataId == DATA_ASPECTS_PREPARE_TO_CHANNEL)
                return aspectsPrepareToChannel;
            else if (DataId == DATA_ULTRAXION_DRAKES)
                return drakesKilled;
            else if (DataId == DATA_SPINE_OF_DEATHWING_PLATES)
                return platesDestroyed;

            return 0;
        }

        void SetData(uint32 type, uint32 data)
        {
            if (type < MAX_ENCOUNTER)
                m_auiEncounter[type] = data;

            if (type == DATA_HAGARA_INTRO_TRASH)
            {
                hagaraTrashKills++;
                SaveToDB();

                Creature * pHagara = this->instance->GetCreature(hagaraGuid);
                if (hagaraTrashKills == 4 || hagaraTrashKills == 18 || hagaraTrashKills == 24 || hagaraTrashKills == 35)
                {
                    if (pHagara)
                        pHagara->AI()->DoAction(DATA_HAGARA_INTRO_TRASH);
                }

            }
            else if (type == DATA_ASPECTS_PREPARE_TO_CHANNEL)
                aspectsPrepareToChannel = data;
            else if (type == DATA_ULTRAXION_DRAKES)
            {
                if (data == 0 || data == 16)
                {
                    if (data == 16)
                    {
                        if (Creature * pDeathwingSummit = this->instance->GetCreature(deathwingSummitGuid))
                            pDeathwingSummit->SetVisible(false);
                    }

                    if (!ultraxionDrakesGUIDs.empty())
                    {
                        for (std::vector<uint64>::const_iterator itr = ultraxionDrakesGUIDs.begin(); itr != ultraxionDrakesGUIDs.end(); ++itr)
                        {
                            if (Creature* pTwilightAssaulter = instance->GetCreature((*itr)))
                            {
                                if (data == 0)
                                    pTwilightAssaulter->AI()->DoAction(DATA_ULTRAXION_DRAKES);
                                else
                                    pTwilightAssaulter->SetVisible(false);
                            }
                        }
                    }
                }

                drakesKilled += data;
                if (drakesKilled == 15)
                {
                    if (Creature * pDeathwingSummit = this->instance->GetCreature(deathwingSummitGuid))
                        pDeathwingSummit->AI()->DoAction(DATA_SUMMON_ULTRAXION);
                }
            }
            else if (type == DATA_SUMMON_ULTRAXION)
            {
                if (Creature * pUltraxion = this->instance->GetCreature(ultraxionGuid))
                    pUltraxion->AI()->DoAction(DATA_SUMMON_ULTRAXION);
            }
            else if (type == DATA_PREPARE_SPINE_ENCOUNTER)
            {
                if (Creature * pDeathwingSpine = this->instance->GetCreature(deathwingSpineGuid))
                    pDeathwingSpine->AI()->DoAction(DATA_PREPARE_SPINE_ENCOUNTER);
            }
            else if (type == DATA_SPINE_OF_DEATHWING_PLATES)
            {
                if (data == 0)
                {
                    platesDestroyed = 0;
                    return;
                }

                platesDestroyed++;

                if (Creature * pDeathwingSpine = this->instance->GetCreature(deathwingSpineGuid))
                    pDeathwingSpine->AI()->DoAction(DATA_SPINE_OF_DEATHWING_PLATES);
            }

            if (data == DONE)
            {
                if (this->instance->IsHeroic())
                    heroicKills++;

                if (type == TYPE_BOSS_MORCHOK)
                {
                    if (Creature * pValeera = this->instance->GetCreature(valeeraGuid))
                        pValeera->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

                    if (!teleportGUIDs.empty())
                    {
                        for (std::vector<uint64>::const_iterator itr = teleportGUIDs.begin(); itr != teleportGUIDs.end(); ++itr)
                        {
                            if (Creature* pTeleport = instance->GetCreature((*itr)))
                            {
                                if (pTeleport->GetEntry() != NPC_TRAVEL_TO_EYE_OF_ETERNITY)
                                    pTeleport->SetVisible(true);
                            }
                        }
                    }
                }
                else if (type == TYPE_BOSS_ZONOZZ)
                {
                    if (Creature * pEiendormi = this->instance->GetCreature(eiendormiGuid))
                        pEiendormi->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                }
                else if (type == TYPE_BOSS_YORSAHJ)
                {
                    if (Creature * pNethestrasz = this->instance->GetCreature(nethestraszGuid))
                        pNethestrasz->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                }
                else if (type == TYPE_BOSS_HAGARA)
                {
                    if (Creature * pKalecgos = this->instance->GetCreature(kalecgosSummitGuid))
                        pKalecgos->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                }
                else if (type == TYPE_BOSS_ULTRAXION)
                {
                    Creature * pAlexstrasza = this->instance->GetCreature(alexstraszaSummitGuid);
                    Creature * pYsera = this->instance->GetCreature(yseraSummitGuid);
                    Creature * pKalecgos = this->instance->GetCreature(kalecgosSummitGuid);
                    Creature * pNozdormu = this->instance->GetCreature(nozdormuSummitGuid);
                    Creature * pThrall = this->instance->GetCreature(thrallSummitGuid);
                    if (pAlexstrasza && pYsera && pKalecgos && pNozdormu && pThrall)
                    {
                        pAlexstrasza->AI()->DoAction(DATA_ULTRAXION_DEFEATED);
                        pYsera->AI()->DoAction(DATA_ULTRAXION_DEFEATED);
                        pKalecgos->AI()->DoAction(DATA_ULTRAXION_DEFEATED);
                        pNozdormu->AI()->DoAction(DATA_ULTRAXION_DEFEATED);
                        pThrall->AI()->DoAction(DATA_ULTRAXION_DEFEATED);
                    }
                }
            }

            if (data == IN_PROGRESS)
            {
                if (!teleportGUIDs.empty())
                {
                    for (std::vector<uint64>::const_iterator itr = teleportGUIDs.begin(); itr != teleportGUIDs.end(); ++itr)
                    {
                        if (Creature* pTeleport = instance->GetCreature((*itr)))
                        {
                            if (pTeleport->HasAura(SPELL_TELEPORT_VISUAL_ACTIVE))
                                pTeleport->RemoveAura(SPELL_TELEPORT_VISUAL_ACTIVE);
                            if (pTeleport->GetEntry() != NPC_ANDORGOS)
                            {
                                pTeleport->CastSpell(pTeleport, SPELL_TELEPORT_VISUAL_DISABLED, true);
                                if (pTeleport->GetEntry() != NPC_TRAVEL_TO_EYE_OF_ETERNITY || GetData(TYPE_BOSS_HAGARA) == DONE)
                                {
                                    uint32 gameObjectId = 0;
                                    switch (pTeleport->GetEntry())
                                    {
                                    case NPC_TRAVEL_TO_WYRMREST_BASE:
                                        gameObjectId = GO_TRAVEL_TO_WYRMREST_BASE;
                                        break;
                                    case NPC_TRAVEL_TO_WYRMREST_TEMPLE:
                                        gameObjectId = GO_TRAVEL_TO_WYRMREST_TEMPLE;
                                        break;
                                    case NPC_TRAVEL_TO_WYRMREST_SUMMIT:
                                        gameObjectId = GO_TRAVEL_TO_WYRMREST_SUMMIT;
                                        break;
                                    case NPC_TRAVEL_TO_EYE_OF_ETERNITY:
                                        gameObjectId = GO_TRAVEL_TO_EYE_OF_ETERNITY;
                                        break;
                                    case NPC_TRAVEL_TO_DECK:
                                        gameObjectId = GO_TRAVEL_TO_DECK;
                                        break;
                                    case NPC_TRAVEL_TO_MAELSTORM:
                                        gameObjectId = GO_TRAVEL_TO_MAELSTROM;
                                        break;
                                    default:
                                        break;
                                    }
                                    if (GameObject * pGoTeleport = pTeleport->FindNearestGameObject(gameObjectId, 5.0f))
                                        pGoTeleport->Delete();
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                if (!teleportGUIDs.empty())
                {
                    for (std::vector<uint64>::const_iterator itr = teleportGUIDs.begin(); itr != teleportGUIDs.end(); ++itr)
                    {
                        if (Creature* pTeleport = instance->GetCreature((*itr)))
                        {
                            if (pTeleport->HasAura(SPELL_TELEPORT_VISUAL_DISABLED))
                                pTeleport->RemoveAura(SPELL_TELEPORT_VISUAL_DISABLED);
                            if (pTeleport->GetEntry() != NPC_ANDORGOS)
                            {
                                pTeleport->CastSpell(pTeleport, SPELL_TELEPORT_VISUAL_ACTIVE, true);
                                if (pTeleport->GetVisibility() == VISIBILITY_ON)
                                {
                                    uint32 gameObjectId = 0;
                                    switch (pTeleport->GetEntry())
                                    {
                                    case NPC_TRAVEL_TO_WYRMREST_BASE:
                                        gameObjectId = GO_TRAVEL_TO_WYRMREST_BASE;
                                        break;
                                    case NPC_TRAVEL_TO_WYRMREST_TEMPLE:
                                        gameObjectId = GO_TRAVEL_TO_WYRMREST_TEMPLE;
                                        break;
                                    case NPC_TRAVEL_TO_WYRMREST_SUMMIT:
                                        gameObjectId = GO_TRAVEL_TO_WYRMREST_SUMMIT;
                                        break;
                                    case NPC_TRAVEL_TO_EYE_OF_ETERNITY:
                                        gameObjectId = GO_TRAVEL_TO_EYE_OF_ETERNITY;
                                        break;
                                    case NPC_TRAVEL_TO_DECK:
                                        gameObjectId = GO_TRAVEL_TO_DECK;
                                        break;
                                    case NPC_TRAVEL_TO_MAELSTORM:
                                        gameObjectId = GO_TRAVEL_TO_MAELSTROM;
                                        break;
                                    default:
                                        break;
                                    }
                                    GameObject * pGoTeleport = pTeleport->FindNearestGameObject(gameObjectId, 5.0f);
                                    if (pGoTeleport == nullptr)
                                        pTeleport->SummonGameObject(gameObjectId, pTeleport->GetPositionX(), pTeleport->GetPositionY(), pTeleport->GetPositionZ() - 3, pTeleport->GetOrientation(), 0, 0, 0, 0, 604800);
                                }
                            }
                        }
                    }
                }
            }

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