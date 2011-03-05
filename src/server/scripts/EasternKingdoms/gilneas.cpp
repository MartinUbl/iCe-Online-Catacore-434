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

class npc_gwen_armstead: public CreatureScript
{
    public:
        npc_gwen_armstead(): CreatureScript("npc_gwen_armstead") {}

        bool OnGossipHello(Player* pl, Creature* cr)
        {
            pl->PrepareQuestMenu(cr->GetGUID());
            pl->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "I wanna take a ride", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            pl->SEND_GOSSIP_MENU(1,cr->GetGUID());

            return true;
        }

        bool OnGossipSelect(Player* pl, Creature* cr, uint32 sender, uint32 action)
        {
            if(action == GOSSIP_ACTION_INFO_DEF+1)
            {
                Creature* kun = cr->SummonCreature(42260,pl->GetPositionX(),pl->GetPositionY(),pl->GetPositionZ(),0,TEMPSUMMON_DEAD_DESPAWN,0);
                //pl->EnterVehicle(kun);
                WorldPacket data(SMSG_CLIENT_CONTROL_UPDATE, kun->GetPackGUID().size()+1);
                data.append(kun->GetPackGUID());
                data << uint8(0); //allow move
                pl->GetSession()->SendPacket(&data);
                //
                WorldPacket data2(SMSG_PET_SPELLS, 8+2+4+4+4*MAX_UNIT_ACTION_BAR_INDEX+1+1);
                data2 << uint64(cr->GetGUID());
                data2 << uint16(0);
                data2 << uint32(0);
                data2 << uint32(0);
                data2 << uint8(0);     // spells count
                data2 << uint8(0);     // cooldowns count
                pl->GetSession()->SendPacket(&data2);
                //
                WorldPacket data3(SMSG_MONSTER_MOVE_TRANSPORT, cr->GetPackGUID().size()+pl->GetPackGUID().size());
                data3.append(cr->GetPackGUID());
                data3.append(pl->GetPackGUID());
                data3 << int8(cr->GetTransSeat());
                data3 << uint8(0);
                data3 << cr->GetPositionX() - pl->GetPositionX();
                data3 << cr->GetPositionY() - pl->GetPositionY();
                data3 << cr->GetPositionZ() - pl->GetPositionZ();
                data3 << uint32(getMSTime());
                data3 << uint8(4);
                data3 << cr->GetTransOffsetO();
                data3 << uint32(SPLINEFLAG_TRANSPORT);
                data3 << uint32(0);// move time
                data3 << uint32(0);//GetTransOffsetX();
                data3 << uint32(0);//GetTransOffsetY();
                data3 << uint32(0);//GetTransOffsetZ();
                pl->SendMessageToSet(&data3, true);
                //
                WorldPacket data4(SMSG_ON_CANCEL_EXPECTED_RIDE_VEHICLE_AURA, 0);
                pl->GetSession()->SendPacket(&data4);
            }
            pl->CLOSE_GOSSIP_MENU();
            return true;
        }
};
/*
2011-03-05 10:03:49 received opcode 0xFF88 (CMSG_GOSSIP_SELECT_OPTION)
2011-03-05 10:03:49 sent opcode SMSG_COMPRESSED_UPDATE_OBJECT (0xEAC0)
2011-03-05 10:03:49 sent opcode SMSG_CLIENT_CONTROL_UPDATE (0x3C84)
2011-03-05 10:03:49 sent opcode SMSG_PET_SPELLS (0xB780)
2011-03-05 10:03:49 sent opcode SMSG_MONSTER_MOVE_TRANSPORT (0x248C)
2011-03-05 10:03:49 sent opcode SMSG_FORCE_MOVE_ROOT (0x2F88)
2011-03-05 10:03:49 sent opcode SMSG_ON_CANCEL_EXPECTED_RIDE_VEHICLE_AURA (0x3380)
2011-03-05 10:03:49 sent opcode SMSG_GOSSIP_COMPLETE (0xF0AC)
2011-03-05 10:03:49 received opcode 0x268C (CMSG_CREATURE_QUERY)
2011-03-05 10:03:49 sent opcode SMSG_CREATURE_QUERY_RESPONSE (0xE6AC)
2011-03-05 10:03:49 received opcode 0x69A0 (UNKNOWN)
2011-03-05 10:03:49 sent opcode SMSG_COMPRESSED_UPDATE_OBJECT (0xEAC0)
*/
/*
-- npc_gwen_armstead
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (36452, 0, 0, 0, 0, 0, 29290, 0, 0, 0, 'Gwen Armstead', 'Mayor of Duskhaven', '', 0, 1, 1, 0, 35, 35, 3, 1, 1.14286, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'npc_gwen_armstead', 1);
-- mob_horse_gilneas
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (42260, 0, 0, 0, 0, 0, 29284, 0, 0, 0, 'Stormwind Charger', '', '', 0, 1, 1, 0, 35, 35, 0, 1, 1.14286, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2056, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 486, 0, 0, '', 0, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 182, 1, 0, 0, 0, 'mob_horse_gilneas', 1);
*/

class npc_gilneas_homebinder: public CreatureScript
{
    public:
        npc_gilneas_homebinder(): CreatureScript("npc_gilneas_homebinder") {}

        bool OnQuestComplete(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
        {
            if(pQuest->GetQuestId() == 24438 && pCreature->GetEntry() == 37065)
            {
                WorldLocation wloc;
                wloc.Relocate(-2456.45f, 1552.06f, 18.692f);
                wloc.m_mapId = 654;
                pPlayer->SetHomebind(wloc,4731);
                return true;
            }
            if(pQuest->GetQuestId() == 14434 && pCreature->GetEntry() == 36616)
            {
                WorldLocation wloc;
                wloc.Relocate(-8867.25f, 671.881f, 97.9036f);
                wloc.m_mapId = 0;
                pPlayer->SetHomebind(wloc,5148);
                return true;
            }

            return false;
        }
};

void AddSC_gilneas()
{
    new npc_gwen_armstead();
    new npc_gilneas_homebinder();
}
