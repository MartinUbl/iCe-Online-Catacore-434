/*
 * Copyright (C) 2008-2010 TrinityCore <http://www.trinitycore.org/>
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
#include "eye_of_eternity.h"
#include "WorldPacket.h"
 
#define DISABLED_ENTER_MESSAGE "You cannot enter Eye of Eternity now"
#define EXIT_MAP 571
#define EXIT_X 3864
#define EXIT_Z 6987
#define EXIT_Y 152
 
enum {
	GO_PLATFORM			= 193070,
	GO_FOCUSING_IRIS	= 193958,
	GO_EXIT_PORTAL		= 193908
};

class instance_eye_of_eternity : public InstanceMapScript
{
public:
    instance_eye_of_eternity() : InstanceMapScript("instance_eye_of_eternity", 616) { }

    InstanceScript* GetInstanceScript(InstanceMap* pMap) const
    {
        return new instance_eye_of_eternity_InstanceMapScript(pMap);
    }

    struct instance_eye_of_eternity_InstanceMapScript : public InstanceScript
    {
        instance_eye_of_eternity_InstanceMapScript(Map* pMap) : InstanceScript(pMap) {Initialize();}
     
        std::string strInstData;
        uint32 m_auiEncounter[MAX_ENCOUNTER];
        uint32 m_uiOutroCheck;
        uint32 m_uiMalygosPlatformData;
     
        uint64 m_uiMalygosPlatformGUID;
        uint64 m_uiFocusingIrisGUID;
        uint64 m_uiExitPortalGUID;
     
        uint64 m_uiMalygosGUID;
        uint64 m_uiPlayerCheckGUID;

        std::list<uint64> m_flyingPlayers;
        
        bool m_bVortex;
        
     
        void Initialize()
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
                    
            m_uiMalygosGUID = 0;
            m_uiOutroCheck = 0; 
            m_uiMalygosPlatformData = 0;
            m_uiMalygosPlatformGUID = 0;
            m_uiFocusingIrisGUID = 0;
            m_uiExitPortalGUID = 0;
            m_uiPlayerCheckGUID = 0;
            m_bVortex = false;
        }
        
        void OnCreatureCreate(Creature* pCreature, bool add)
        {
            switch(pCreature->GetEntry())
            {
                case NPC_MALYGOS:
                    m_uiMalygosGUID = pCreature->GetGUID();
                    break;
                default:
                    break;
            }
        }
        
        void OnGameObjectCreate(GameObject *pGo, bool add)
        {
            switch(pGo->GetEntry())
            {
                case 193070: m_uiMalygosPlatformGUID = pGo->GetGUID(); break;
                case 193958: //normal, hero 
                case 193960: m_uiFocusingIrisGUID = pGo->GetGUID(); break;
                case 193908: m_uiExitPortalGUID = pGo->GetGUID(); break;
                default:
                    break;
            }
        }
     
     
        void SetData(uint32 uiType, uint32 uiData)
        {
            switch(uiType)
            {
                case TYPE_MALYGOS:
                    if(uiData == IN_PROGRESS)
                    {
                        if(GameObject* pExitPortal = instance->GetGameObject(m_uiExitPortalGUID))
							pExitPortal->SetPhaseMask(2, true);

                        if(GameObject* pFocusingIris = instance->GetGameObject(m_uiFocusingIrisGUID))
							pFocusingIris->SetPhaseMask(2, true);
                    }
					if(uiData == NOT_STARTED)
					{
						Creature* pBoss = instance->GetCreature(m_uiMalygosGUID);
						if(!pBoss)
							return;
						//Summon Platform
						if(GameObject* pGo = GetClosestGameObjectWithEntry(pBoss, GO_PLATFORM, 200.0f))
						{
							pGo->Respawn();
							SetData(TYPE_DESTROY_PLATFORM, NOT_STARTED);
						}
						else 
							pBoss->SummonGameObject(GO_PLATFORM, 754.346f, 1300.87f, 256.249f, 3.14159f, 0, 0, 0, 0, 0);
     
						//Summon focusing iris
						if(GameObject* pGo = instance->GetGameObject(m_uiFocusingIrisGUID))
							pGo->SetPhaseMask(1, true);
						else
							pBoss->SummonGameObject(GO_FOCUSING_IRIS, 754.731f, 1300.12f, 266.171f, 5.01343f, 0, 0, 0, 0, 0);
     
						//Summon exit portal
						if(GameObject* pGo = instance->GetGameObject(m_uiExitPortalGUID))
							pGo->SetPhaseMask(1, true);
						else
							pBoss->SummonGameObject(GO_EXIT_PORTAL, 724.684f, 1332.92f, 267.234f, -0.802851f, 0, 0, 0, 0, 0);
					}
					if(uiData == DONE)
					{
						//exit portal
						if(GameObject* pGo = instance->GetGameObject(m_uiExitPortalGUID))
							pGo->SetPhaseMask(1, true);
					}
                    m_auiEncounter[0] = uiData;
                    break;
                case TYPE_OUTRO_CHECK:
                    m_uiOutroCheck = uiData;
                    break;
                case TYPE_DESTROY_PLATFORM:
                    if(uiData == IN_PROGRESS)
                    {
                        if(GameObject* pMalygosPlatform = instance->GetGameObject(m_uiMalygosPlatformGUID))
                            pMalygosPlatform->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);
                    }
                    else if(uiData == NOT_STARTED)
                    {
                        if(GameObject* pMalygosPlatform = instance->GetGameObject(m_uiMalygosPlatformGUID))
                        {
                            pMalygosPlatform->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);
                            pMalygosPlatform->Respawn();
                        }
                    }
                    m_uiMalygosPlatformData = uiData;
                    break;
                case TYPE_VORTEX:
                    if(uiData)
                        m_bVortex = true;
                    else
                        m_bVortex = false;
                    break;
                case TYPE_PLAYER_HOVER:
                    if(uiData == DATA_DROP_PLAYERS)
                        dropAllPlayers();
                    break;
            }
        }
     
        const char* Save()
        {
            OUT_SAVE_INST_DATA;
            std::ostringstream saveStream;
            saveStream << m_auiEncounter[0] << " " << m_uiOutroCheck;
     
            strInstData = saveStream.str();
            SaveToDB();
            OUT_SAVE_INST_DATA_COMPLETE;
            return strInstData.c_str();
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
            loadStream >> m_auiEncounter[0] >> m_uiOutroCheck;
     
            for(uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                    m_auiEncounter[i] = NOT_STARTED;
            }
     
            OUT_LOAD_INST_DATA_COMPLETE;
        }
     
        uint32 GetData(uint32 uiType)
        {
            switch(uiType)
            {
                case TYPE_MALYGOS:
                    return m_auiEncounter[0];
                case TYPE_OUTRO_CHECK:
                    return m_uiOutroCheck;
                case TYPE_DESTROY_PLATFORM:
                    return m_uiMalygosPlatformData;
                case TYPE_VORTEX:
                    return m_bVortex;
                case TYPE_CHECK_PLAYER_FLYING:
                    return isFlying(m_uiPlayerCheckGUID);
            }
            return 0;
        }
     
        uint64 GetData64(uint32 uiData)
        {
            switch(uiData)
            {
                case NPC_MALYGOS:
                    return m_uiMalygosGUID;
                default:
                    return 0;
            }
            return 0;
        }

        void SetData64(uint32 uiType, uint64 uiData)
        {
            switch(uiType)
            {
            case TYPE_PLAYER_HOVER:
                if(!isFlying(uiData))
                {
                    Unit* pUnit = ObjectAccessor::GetObjectInMap(uiData, instance, (Unit*)NULL);
                    if(pUnit && pUnit->GetTypeId() == TYPEID_PLAYER)
                    {
                        m_flyingPlayers.push_back(uiData);

                        //Using packet workaround
                        WorldPacket data(12);
                        data.SetOpcode(SMSG_MOVE_SET_CAN_FLY);
                        data.append(pUnit->GetPackGUID());
                        data << uint32(0);
                        pUnit->SendMessageToSet(&data, true);
                    }
                }
                break;

            case TYPE_SET_PLAYER_TO_CHECK:
                m_uiPlayerCheckGUID = uiData;
                break;
            }
        }

        bool isFlying(uint64 GUID)
        {
            if(m_flyingPlayers.empty())
                return false;

            for(std::list<uint64>::iterator itr = m_flyingPlayers.begin(); itr != m_flyingPlayers.end(); ++itr)
            {
                if(Player* pPlayer = (Player*)Unit::GetUnit(*(instance->GetCreature(m_uiMalygosGUID)), (*itr)))
                {
                    if(pPlayer->GetGUID() == GUID)
                        return true;
                }
            }
            return false;
        }

        void dropAllPlayers()
        {
            for(std::list<uint64>::iterator itr = m_flyingPlayers.begin(); itr != m_flyingPlayers.end(); ++itr)
            {
                if(Unit* pUnit = Unit::GetUnit(*(instance->GetCreature(m_uiMalygosGUID)), (*itr)))
                {
                    //Using packet workaround
                    WorldPacket data(12);
                    data.SetOpcode(SMSG_MOVE_UNSET_CAN_FLY);
                    data.append(pUnit->GetPackGUID());
                    data << uint32(0);
                    pUnit->SendMessageToSet(&data, true);
                }
            }
            m_flyingPlayers.clear();
        }

        void OnPlayerEnter(Player* pPlayer)
        {
            if(GetData(TYPE_MALYGOS) == DONE)
            {
                Creature *pTemp = pPlayer->SummonCreature(NPC_WYRMREST_SKYTALON, pPlayer->GetPositionX(), pPlayer->GetPositionY(), pPlayer->GetPositionZ() - 5, 0);
                if(pTemp)
                {
                    pTemp->SetCreatorGUID(pPlayer->GetGUID());
                    pPlayer->EnterVehicle(pTemp, 0);
                }
            }
        }
    };
};

void AddSC_instance_eye_of_eternity()
{
    new instance_eye_of_eternity();
}

/*
-- EYE_OF_ETERNITY.SQL

-- Set instance script
UPDATE instance_template SET script = 'instance_eye_of_eternity' WHERE map = 616;

-- Update flags for NPCs/Vehicles
UPDATE creature_template SET flags_extra = flags_extra | 2 WHERE entry = 30090; -- Vortex  'Arcane Overload', 'Hover Disk');
UPDATE creature_template SET flags_extra = flags_extra | 2, faction_A = 35, faction_H = 35, VehicleId = 264 WHERE entry IN (30234, 30248); -- Hover Disk (VehicleId 264)
UPDATE creature_template SET flags_extra = flags_extra | 2, faction_A = 35, faction_H = 35 WHERE entry = 30118; -- Portal (Malygos)
UPDATE creature_template SET flags_extra = flags_extra | 2 WHERE entry = 30282; -- Arcane Overload
UPDATE creature_template SET mindmg = 1, maxdmg = 1, dmg_multiplier = 1 WHERE entry = 30592; -- Static Field
UPDATE creature_template SET modelid1 = 11686, modelid2 = 11686 WHERE entry = 22517; -- Some world trigger

-- Set scriptnames and some misc data to bosses and GOs
UPDATE gameobject_template SET flags = 4, data0 = 43 WHERE gameobject_template.entry in (193967, 193905);
UPDATE creature_template SET ScriptName = 'boss_malygos', unit_flags = unit_flags & ~256 WHERE entry = 28859;
UPDATE creature SET MovementType = 0, spawndist = 0 WHERE id = 28859; -- Malygos, don't move
UPDATE creature_template SET ScriptName = 'mob_nexus_lord' WHERE entry = 30245; -- Nexus Lord
UPDATE creature_template SET ScriptName = 'mob_scion_of_eternity' WHERE entry = 30249; -- Scion of Eternity
UPDATE creature_template SET faction_A = 14, faction_H = 14, ScriptName = 'mob_power_spark' WHERE entry = 30084; -- Power Spark
UPDATE creature_template SET ScriptName = 'npc_arcane_overload' WHERE entry = 30282; -- Arcane Overload

-- Fix Wyrmrest drakes creature info
UPDATE creature_template SET spell1 = 56091, spell2 = 56092, spell3 = 57090, spell4 = 57143, spell5 = 57108, spell6 = 57403, VehicleId = 165 WHERE entry IN (30161, 31752);

-- Delete faulty Alextrasza spawn
DELETE FROM creature WHERE guid = 132302;
DELETE FROM creature_addon WHERE guid = 132302;

-- And Surge of Power
DELETE FROM creature WHERE guid = 132303;
DELETE FROM creature_addon WHERE guid = 132303;

-- Fix Loot caches being not selectable
UPDATE gameobject_template SET faction = 35, flags = 0 WHERE entry IN (193967, 193905);

-- Fix Surge of Power targeting
DELETE FROM conditions WHERE SourceTypeOrReferenceId = 13 AND SourceEntry = 56505;
INSERT INTO conditions (SourceTypeOrReferenceId, SourceGroup, SourceEntry, ElseGroup, ConditionTypeOrReference, ConditionValue1, ConditionValue2, ConditionValue3, ErrorTextId, COMMENT) VALUES
(13, 0, 56505, 0, 18, 1, 22517, 0, 0, "Surge of Power - cast only on World Trigger (Large AOI)");

-- Fix loot for Malygos (Alexstrasza's Gift)
DELETE FROM reference_loot_template WHERE entry = 50008;
INSERT INTO reference_loot_template (entry, item, ChanceOrQuestChance, lootmode, groupid, mincountOrRef, maxcount) VALUES
(50008, 40474, 0, 1, 1, 1, 1), -- Surge Needle Ring
(50008, 40497, 0, 1, 1, 1, 1), -- Black Ice
(50008, 40489, 0, 1, 1, 1, 1), -- Greatstaff of the Nexus
(50008, 40526, 0, 1, 1, 1, 1), -- Gown of the Spell-Weaver
(50008, 40511, 0, 1, 1, 1, 1), -- Focusing Energy Epaulets
(50008, 40475, 0, 1, 1, 1, 1), -- Barricade of Eternity
(50008, 40488, 0, 1, 1, 1, 1), -- Ice Spire Scepter
(50008, 40491, 0, 1, 1, 1, 1), -- Hailstorm
(50008, 40519, 0, 1, 1, 1, 1), -- Footsteps of Malygos
(50008, 40486, 0, 1, 1, 1, 1); -- Necklace of the Glittering Chamber

DELETE FROM reference_loot_template WHERE entry = 50009;
INSERT INTO reference_loot_template (entry, item, ChanceOrQuestChance, lootmode, groupid, mincountOrRef, maxcount) VALUES
(50009, 40592, 0, 1, 1, 1, 1), -- Boots of Healing Energies
(50009, 40566, 0, 1, 1, 1, 1), -- Unravelling Strands of Sanity
(50009, 40194, 0, 1, 1, 1, 1), -- Blanketing Robes of Snow
(50009, 40543, 0, 1, 1, 1, 1), -- Blue Aspect Helm
(50009, 40590, 0, 1, 1, 1, 1), -- Elevated Lair Pauldrons
(50009, 40560, 0, 1, 1, 1, 1), -- Leggings of the Wanton Spellcaster
(50009, 40589, 0, 1, 1, 1, 1), -- Legplates of Sovereignty
(50009, 40555, 0, 1, 1, 1, 1), -- Mantle of Dissemination
(50009, 40591, 0, 1, 1, 1, 1), -- Melancholy Sabatons
(50009, 40594, 0, 1, 1, 1, 1), -- Spaulders of Catatonia
(50009, 40588, 0, 1, 1, 1, 1), -- Tunic of the Artifact Guardian
(50009, 40549, 0, 1, 1, 1, 1), -- Boots of the Renewed Flight
(50009, 40539, 0, 1, 1, 1, 1), -- Chestguard of the Recluse
(50009, 40541, 0, 1, 1, 1, 1), -- Frosted Adroit Handguards
(50009, 40562, 0, 1, 1, 1, 1), -- Hood of Rationality
(50009, 40561, 0, 1, 1, 1, 1), -- Leash of Heedless Magic
(50009, 40532, 0, 1, 1, 1, 1), -- Living Ice Crystals
(50009, 40531, 0, 1, 1, 1, 1), -- Mark of Norgannon
(50009, 40564, 0, 1, 1, 1, 1), -- Winter Spectacle Gloves
(50009, 40558, 0, 1, 1, 1, 1); -- Arcanic Tramplers

DELETE FROM gameobject_loot_template WHERE entry IN (26094, 26097);
INSERT INTO gameobject_loot_template (entry, item, ChanceOrQuestChance, lootmode, groupid, mincountOrRef, maxcount) VALUES
(26094, 40752, 100, 1, 0, 2, 2), -- Emblem of Heroism x2
(26094, 	1, 100, 1, 0, -50008, 2), -- 2 items ilevel 213
(26094, 44650, 100, 1, 0, 1, 1), -- Quest item, Judgement at the Eye of Eternity
(26094, 43953, 1, 1, 0, 1, 1), -- Reins of the Blue Drake 	
-- End of 10m Malygos loot

(26097, 40753, 100, 1, 0, 2, 2), -- Emblem of Valor x2
(26097, 	1, 100, 1, 0, -50009, 4), -- 4 items ilevel 226
(26097, 44651, 100, 1, 0, 1, 1), -- Quest item, Heroic Judgement at the Eye of Eternity
(26097, 43952, 1, 1, 0, 1, 1); -- Reins of the Azure Drake
-- End of 25m Malygos loot

-- Fix Malygos and his adds' damage
UPDATE creature_template SET mindmg = 3684, maxdmg = 4329, dmg_multiplier = 7.5, mechanic_immune_mask = 1072918979 WHERE entry = 30245; -- Nexus Lord
UPDATE creature_template SET mindmg = 3684, maxdmg = 4329, dmg_multiplier = 13,  mechanic_immune_mask = 1072918979 WHERE entry = 31750; -- Nexus Lord (1)
UPDATE creature_template SET mechanic_immune_mask = 1072918979 WHERE entry IN (30249, 31751);

-- Create entry for Heroic Malygos
DELETE FROM creature_template WHERE entry = 50000;
INSERT INTO creature_template (entry, difficulty_entry_1, difficulty_entry_2, difficulty_entry_3, KillCredit1, KillCredit2, modelid1, modelid2, 
modelid3, modelid4, name, subname, IconName, gossip_menu_id, minlevel, maxlevel, exp, faction_A, faction_H, npcflag, speed_walk, speed_run, scale, 
rank, mindmg, maxdmg, dmgschool, attackpower, dmg_multiplier, baseattacktime, rangeattacktime, unit_class, unit_flags, dynamicflags, family, 
trainer_type, trainer_spell, trainer_class, trainer_race, minrangedmg, maxrangedmg, rangedattackpower, type, type_flags, lootid, pickpocketloot, 
skinloot, resistance1, resistance2, resistance3, resistance4, resistance5, resistance6, spell1, spell2, spell3, spell4, spell5, spell6, spell7, 
spell8, PetSpellDataId, VehicleId, mingold, maxgold, AIName, MovementType, InhabitType, Health_mod, Mana_mod, Armor_mod, RacialLeader, questItem1, 
questItem2, questItem3, questItem4, questItem5, questItem6, movementId, RegenHealth, equipment_id, mechanic_immune_mask, flags_extra, ScriptName, WDBVerified) VALUES 
(50000, 0, 0, 0, 0, 0, 26752, 0, 0, 0, 'Malygos', '', '', 0, 83, 83, 2, 16, 16, 0, 1, 1.14286, 1, 3, 496, 674, 0, 783, 35, 2000, 0, 2, 64, 8, 0, 0, 0, 0, 0, 365, 529, 98, 2, 108, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 5, 500, 50, 1, 0, 44650, 0, 0, 0, 0, 0, 227, 1, 0, 0, 1, 'boss_malygos', 1);

UPDATE creature_template SET Health_mod = 1400, questItem1 = 44651, mechanic_immune_mask = 617299803, ScriptName = '', WDBVerified = 1 WHERE entry = 50000;

UPDATE creature_template SET mindmg = 4602, maxdmg = 5502, dmg_multiplier = 7.5, difficulty_entry_1 = 50000, mechanic_immune_mask = 617299803 WHERE entry = 28859;
UPDATE creature_template SET mindmg = 4602, maxdmg = 5502, dmg_multiplier = 13 WHERE entry = 50000;

UPDATE creature_template SET mechanic_immune_mask = 1072902595 WHERE entry IN (30245, 31750);

-- Fix sound entries for Malygos encounter
DELETE FROM script_texts WHERE entry BETWEEN -1616034 AND -1616000;
INSERT INTO script_texts (npc_entry, entry, content_default, sound, type, language, emote, comment) VALUES
(28859, -1616000, 'Lesser beings, intruding here! A shame that your excess courage does not compensate for your stupidity!', 14512, 1, 0, 0, 'Malygos INTRO 1'),
(28859, -1616001, 'None but the blue dragonflight are welcome here! Perhaps this is the work of Alexstrasza? Well then, she has sent you to your deaths.', 14513, 1, 0, 0, 'Malygos INTRO 2'),
(28859, -1616002, 'What could you hope to accomplish, to storm brazenly into my domain? To employ MAGIC? Against ME?', 14514, 1, 0, 0, 'Malygos INTRO 3'),
(28859, -1616003, 'I am without limits here... the rules of your cherished reality do not apply... In this realm, I am in control...', 14515, 1, 0, 0, 'Malygos INTRO 4'),
(28859, -1616004, 'I give you one chance. Pledge fealty to me, and perhaps I won\'t slaughter you for your insolence!', 14516, 1, 0, 0, 'Malygos INTRO 5'),
(28859, -1616005, 'My patience has reached its limit, I WILL BE RID OF YOU!', 14517, 1, 0, 0, 'Malygos AGGRO 1'),
(28859, -1616006, 'Watch helplessly as your hopes are swept away...', 14525, 1, 0, 0, 'Malygos VORTEX'),
(28859, -1616007, 'I AM UNSTOPPABLE!', 14533, 1, 0, 0, 'Malygos SPARK BUFF'),
(28859, -1616008, 'Your stupidity has finally caught up to you!', 14519, 1, 0, 0, 'Malygos SLAY 1-1'),
(28859, -1616009, 'More artifacts to confiscate...', 14520, 1, 0, 0, 'Malygos SLAY 1-2'),
(28859, -1616010, 'How very... naive...', 14521, 1, 0, 0, 'Malygos SLAY 1-3'),
(28859, -1616012, 'I had hoped to end your lives quickly, but you have proven more...resilient then I had anticipated. Nonetheless, your efforts are in vain, it is you reckless, careless mortals who are to blame for this war! I do what I must...And if it means your...extinction...THEN SO BE IT!', 14522, 1, 0, 0, 'Malygos PHASEEND 1'),
(28859, -1616013, 'Few have experienced the pain I will now inflict upon you!', 14523, 1, 0, 0, 'Malygos AGGRO 2'),
(28859, -1616014, 'YOU WILL NOT SUCCEED WHILE I DRAW BREATH!', 14518, 1, 0, 0, 'Malygos DEEP BREATH'),
(28859, -1616015, 'Malygos takes a deep breath.', 0, 3, 0, 0, 'Malygos DEEP BREATH WARN'),
(28859, -1616016, 'I will teach you IGNORANT children just how little you know of magic...', 14524, 1, 0, 0, 'Malygos ARCANE OVERLOAD'),
(28859, -1616020, 'Your energy will be put to good use!', 14526, 1, 0, 0, 'Malygos SLAY 2-1'),
(28859, -1616021, 'I AM THE SPELL-WEAVER! My power is INFINITE!', 14527, 1, 0, 0, 'Malygos SLAY 2-2'),
(28859, -1616022, 'Your spirit will linger here forever!', 14528, 1, 0, 0, 'Malygos SLAY 2-3'),
(28859, -1616017, 'ENOUGH! If you intend to reclaim Azeroth\'s magic, then you shall have it...', 14529, 1, 0, 0, 'Malygos PHASEEND 2'),
(28859, -1616018, 'Now your benefactors make their appearance...But they are too late. The powers contained here are sufficient to destroy the world ten times over! What do you think they will do to you?', 14530, 1, 0, 0, 'Malygos PHASE 3 INTRO'),
(28859, -1616019, 'SUBMIT!', 14531, 1, 0, 0, 'Malygos AGGRO 3'),
(28859, -1616026, 'The powers at work here exceed anything you could possibly imagine!', 14532, 1, 0, 0, 'Malygos STATIC FIELD'),
(28859, -1616023, 'Alexstrasza! Another of your brood falls!', 14534, 1, 0, 0, 'Malygos SLAY 3-1'),
(28859, -1616024, 'Little more then gnats!', 14535, 1, 0, 0, 'Malygos SLAY 3-2'),
(28859, -1616025, 'Your red allies will share your fate...', 14536, 1, 0, 0, 'Malygos SLAY 3-3'),
(28859, -1616027, 'Still standing? Not for long...', 14537, 1, 0, 0, 'Malygos SPELL 1'),
(28859, -1616028, 'Your cause is lost!', 14538, 1, 0, 0, 'Malygos SPELL 1'),
(28859, -1616029, 'Your fragile mind will be shattered!', 14539, 1, 0, 0, 'Malygos SPELL 1'),
(28859, -1616030, 'UNTHINKABLE! The mortals will destroy... e-everything... my sister... what have you-', 14540, 1, 0, 0, 'Malygos DEATH'),
(32295, -1616031, 'I did what I had to, brother. You gave me no alternative.', 14406, 1, 0, 0, 'Alexstrasza OUTRO 1'),
(32295, -1616032, 'And so ends the Nexus War.', 14407, 1, 0, 0, 'Alexstrasza OUTRO 2'),
(32295, -1616033, 'This resolution pains me deeply, but the destruction, the monumental loss of life had to end. Regardless of Malygos\' recent transgressions, I will mourn his loss. He was once a guardian, a protector. This day, one of the world\'s mightiest has fallen.', 14408, 1, 0, 0, 'Alexstrasza OUTRO 3'),
(32295, -1616034, 'The red dragonflight will take on the burden of mending the devastation wrought on Azeroth. Return home to your people and rest. Tomorrow will bring you new challenges, and you must be ready to face them. Life...goes on.', 14409, 1, 0, 0, 'Alexstrasza OUTRO 4');

--Focusing Iris AI
UPDATE `gameobject_template` SET ScriptName = 'go_focusing_iris' WHERE entry = 193958;
--Focusing Iris Spawn
INSERT INTO `gameobject` (id, map, spawnMask, phaseMask, position_x, position_y, position_z, orientation, rotation0, rotation1, rotation2, rotation3, spawntimesecs, animprogress, state)
VALUES (193958, 616, 1, 1, 754.508, 1301.67, 266.171, 0.81895, 0, 0, 0.398128, 0.91733, 300, 0, 1);

--iCelike platform
DELETE FROM `gameobject_template` WHERE (`entry`=590011);
INSERT INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `faction`, `flags`, `size`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `data0`, `data1`, `data2`, `data3`, `data4`, `data5`, `data6`, `data7`, `data8`, `data9`, `data10`, `data11`, `data12`, `data13`, `data14`, `data15`, `data16`, `data17`, `data18`, `data19`, `data20`, `data21`, `data22`, `data23`, `ScriptName`, `WDBVerified`) VALUES (590011, 5, 6651, 'Deska', '', '', '', 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 1);

*/
