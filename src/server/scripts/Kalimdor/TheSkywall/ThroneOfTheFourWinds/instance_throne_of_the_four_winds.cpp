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
#include "throne_of_the_four_winds.h"


class npc_four_winds_bridge : public CreatureScript
{
public:
    npc_four_winds_bridge() : CreatureScript("npc_four_winds_bridge") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_four_winds_bridgeAI (pCreature);
    }

    struct npc_four_winds_bridgeAI : public ScriptedAI
    {
        npc_four_winds_bridgeAI(Creature *c) : ScriptedAI(c)
        {
            m_pInstance = c->GetInstanceScript();
        }

        InstanceScript* m_pInstance;
        Position targetPos;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            //me->SetReactState(REACT_PASSIVE); // eliminates MILOS event
            me->SetVisibility(VISIBILITY_OFF);
            me->SetSpeed(MOVE_RUN, 0.0f);
        }

        void EnterCombat(Unit* /*who*/) {}
        void DamageTaken(Unit* /*who*/, uint32 &damage) { damage = 0; }

        void MoveInLineOfSight(Unit* who)
        {
            if(!who)
                return;

            if(me->GetDistance(who) > 13.0f)
                return;

            if(m_pInstance)
                if(m_pInstance->GetData(TYPE_CONCLAVE) == SPECIAL) // Ultimate ability in progress
                    return;

            if(who->ToPlayer() && !who->HasAura(89771))
            {
                who->CastSpell(who, 89771, true); // add aura not to retrigger jumping (8 sec)
                float x = targetPos.GetPositionX() + (who->GetPositionX() - me->GetPositionX())/2;
                float y = targetPos.GetPositionY() + (who->GetPositionY() - me->GetPositionY())/2;
                float z = targetPos.GetPositionZ();
                who->GetMotionMaster()->MoveJump(x, y, z, 30.0f, 9.0f);
            }
        }

        void DoAction(const int32 param)
        {
            targetPos = BridgeTarget[param];
        }
    };
};

class instance_throne_of_the_four_winds : public InstanceMapScript
{
public:
    instance_throne_of_the_four_winds() : InstanceMapScript("instance_throne_of_the_four_winds", 754) { }

    InstanceScript* GetInstanceScript(InstanceMap* pMap) const
    {
        return new instance_throne_of_the_four_winds_InstanceMapScript(pMap);
    }

    struct instance_throne_of_the_four_winds_InstanceMapScript : public InstanceScript
    {
        instance_throne_of_the_four_winds_InstanceMapScript(Map* pMap): InstanceScript(pMap) { }

        uint32 m_auiEncounter[MAX_ENCOUNTER];

        uint64 m_Bridge_GUID;
        uint64 m_Center_GUID;

        uint64 m_Control_GUID;
        uint64 m_Anshal_GUID;
        uint64 m_Nezir_GUID;
        uint64 m_Rohash_GUID;
        uint64 m_Alakir_GUID;

        void Initialize()
        {
            m_Bridge_GUID = 0;
            m_Center_GUID = 0;

            m_Control_GUID = 0;
            m_Anshal_GUID = 0;
            m_Nezir_GUID = 0;
            m_Rohash_GUID = 0;
            m_Alakir_GUID = 0;
        }

        bool IsEncounterInProgress() const
        {
            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                if (m_auiEncounter[i] == IN_PROGRESS) return true;

            return false;
        }

        void OnGameObjectCreate(GameObject* pGO, bool add)
        {
            switch(pGO->GetEntry())
            {
            case GO_BRIDGE:
                m_Bridge_GUID = pGO->GetGUID();
                break;
            case GO_CENTER:
                for(int i = 0; i < 8; i++) // Summon Bridge NPCs and set their target position
                {
                    if(TempSummon* pBridge = pGO->SummonCreature(NPC_BRIDGE, BridgePos[i]))
                        pBridge->AI()->DoAction(i);
                }
                m_Center_GUID = pGO->GetGUID();
                break;
            default: break;
            }
        }

        void OnCreatureCreate(Creature* pCreature, bool add)
        {
            switch(pCreature->GetEntry())
            {
            case NPC_CONCLAVE_CONTROL:  m_Control_GUID = pCreature->GetGUID();      break;
            case NPC_ANSHAL:            m_Anshal_GUID = pCreature->GetGUID();       break;
            case NPC_NEZIR:             m_Nezir_GUID = pCreature->GetGUID();        break;
            case NPC_ROHASH:            m_Rohash_GUID = pCreature->GetGUID();       break;
            case NPC_ALAKIR:            m_Alakir_GUID = pCreature->GetGUID();       break;
            default: break;
            }
        }

        void SetData(uint32 uiType, uint32 uiData)
        {
            switch(uiType)
            {
            case TYPE_CONCLAVE:
                m_auiEncounter[TYPE_CONCLAVE] = uiData;
                break;
            case TYPE_ALAKIR:
                m_auiEncounter[TYPE_ALAKIR] = uiData;
                break;
            default: break;
            }
        }

        uint32 GetData(uint32 uiType)
        {
            switch(uiType)
            {
            case TYPE_CONCLAVE:
                return m_auiEncounter[TYPE_CONCLAVE];
                break;
            case TYPE_ALAKIR:
                return m_auiEncounter[TYPE_ALAKIR];
                break;
            default: return 0; break;
            }
        }
    };
};

void AddSC_instance_throne_of_the_four_winds()
{
    new instance_throne_of_the_four_winds();
    new npc_four_winds_bridge();
}

/* ############## SQL ##############


UPDATE `instance_template` SET script="instance_throne_of_the_four_winds" WHERE map=754;
UPDATE `gameobject_template` SET data1='0' WHERE entry IN (4510135,4510136,4510137);

INSERT INTO creature_template
  (entry, difficulty_entry_1, difficulty_entry_2, difficulty_entry_3, KillCredit1, KillCredit2, modelid1, modelid2, modelid3, modelid4, name, subname, IconName, gossip_menu_id, minlevel, maxlevel, exp, faction_A, faction_H, npcflag, speed_walk, speed_run, scale, rank, mindmg, maxdmg, dmgschool, attackpower, dmg_multiplier, baseattacktime, rangeattacktime, unit_class, unit_flags, dynamicflags, family, trainer_type, trainer_spell, trainer_class, trainer_race, minrangedmg, maxrangedmg, rangedattackpower, type, type_flags, lootid, pickpocketloot, skinloot, resistance1, resistance2, resistance3, resistance4, resistance5, resistance6, spell1, spell2, spell3, spell4, spell5, spell6, spell7, spell8, PetSpellDataId, VehicleId, mingold, maxgold, AIName, MovementType, InhabitType, Health_mod, Mana_mod, Armor_mod, RacialLeader, questItem1, questItem2, questItem3, questItem4, questItem5, questItem6, movementId, RegenHealth, equipment_id, mechanic_immune_mask, flags_extra, ScriptName, WDBVerified)
VALUES
  (80000, 0, 0, 0, 0, 0, 987, 0, 0, 0, "Four Winds Bridge", "", "NULL", 0, 1, 1, 0, 21, 21, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 2000, 0, 1, 0, 2048, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "", 1, 3, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 8388624, 0, "npc_four_winds_bridge", 12340);

INSERT INTO gameobject
  (id, map, spawnMask, phaseMask, position_x, position_y, position_z, orientation, rotation0, rotation1, rotation2, rotation3, spawntimesecs, animprogress, state)
VALUES
  (4510369, 754, 1, 1, -274.184, 817.184, 200, 3.14018, 0, 0, 0.00141254, -0.999999, 300, 0, 0),
  (4510370, 754, 1, 1, -274.184, 817.184, 200, 3.14018, 0, 0, 0.00141254, -0.999999, 300, 0, 0),
  (4510136, 754, 1, 1, 185.588, 814.656, 200, 3.14662, 0, 0, 0.999997, -0.00251452, 300, 0, 1),
  (4510135, 754, 1, 1, -50.642, 1055.37, 200, 4.71113, 0, 0, 0.707551, -0.706663, 300, 0, 1),
  (4510137, 754, 1, 1, -54.9326, 578.428, 200, 1.55776, 0, 0, 0.702482, 0.711702, 300, 0, 1);

  */
