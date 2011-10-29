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

/************************
* Quest: Fuel-ology 101 *
* Author: Labuz         *
************************/

/*
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (41666, 0, 0, 0, 0, 0, 32848, 0, 0, 0, 'Engineer Hexascrub', '', '', 0, 82, 82, 0, 35, 35, 2, 1, 1.14286, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 10, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, '', 1);
REPLACE INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `faction`, `flags`, `size`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `data0`, `data1`, `data2`, `data3`, `data4`, `data5`, `data6`, `data7`, `data8`, `data9`, `data10`, `data11`, `data12`, `data13`, `data14`, `data15`, `data16`, `data17`, `data18`, `data19`, `data20`, `data21`, `data22`, `data23`, `data24`, `data25`, `data26`, `data27`, `data28`, `data29`, `data30`, `data31`, `AIName`, `ScriptName`, `WDBVerified`) VALUES (203461, 2, 3491, 'Fuel Sampling Station', '', 'Using', '', 0, 0, 1, 0, 0, 0, 0, 0, 0, 1690, 0, 0, 11586, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 'go_Fuel_station', 13329);
INSERT INTO `script_texts` (`npc_entry`, `entry`, `content_default`, `content_loc1`, `content_loc2`, `content_loc3`, `content_loc4`, `content_loc5`, `content_loc6`, `content_loc7`, `content_loc8`, `sound`, `type`, `language`, `emote`, `comment`) VALUES (0, -1999959, 'We\'re getting close... but it\'s still a little too smoky. Don\'t put in any terrapin oil next time. <cough> Too much smoke. Give it another shot!', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0, NULL);
INSERT INTO `script_texts` (`npc_entry`, `entry`, `content_default`, `content_loc1`, `content_loc2`, `content_loc3`, `content_loc4`, `content_loc5`, `content_loc6`, `content_loc7`, `content_loc8`, `sound`, `type`, `language`, `emote`, `comment`) VALUES (0, -1999958, 'Bah! <cough> Far too smoky! <cough> Looks like you put in too much terrapin oil! Try again!', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0, NULL);
INSERT INTO `script_texts` (`npc_entry`, `entry`, `content_default`, `content_loc1`, `content_loc2`, `content_loc3`, `content_loc4`, `content_loc5`, `content_loc6`, `content_loc7`, `content_loc8`, `sound`, `type`, `language`, `emote`, `comment`) VALUES (0, -1999957, 'Yes, this will definitely burn too hot. Do you want to blow up the ship? Cut back on the remora oil next time.', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0, NULL);
INSERT INTO `script_texts` (`npc_entry`, `entry`, `content_default`, `content_loc1`, `content_loc2`, `content_loc3`, `content_loc4`, `content_loc5`, `content_loc6`, `content_loc7`, `content_loc8`, `sound`, `type`, `language`, `emote`, `comment`) VALUES (0, -1999956, 'You\'re right, far too weak. Needs more remora. Try it again!', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0, NULL);
INSERT INTO `script_texts` (`npc_entry`, `entry`, `content_default`, `content_loc1`, `content_loc2`, `content_loc3`, `content_loc4`, `content_loc5`, `content_loc6`, `content_loc7`, `content_loc8`, `sound`, `type`, `language`, `emote`, `comment`) VALUES (0, -1999955, 'This is it! The perfect bio-fuel! Good work !', NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0, NULL);
*/

/*######
##   FUEL STATION
######*/

#define GOSSIP_START        "Let's get started!"
#define GOSSIP_ERASEALL     "I want to start again"
#define GOSSIP_FLASKA1      "Terrapin Oil Sample"
#define GOSSIP_FLASKA2      "Remora Oil Sample"
#define GOSSIP_FLASKA3      "Hammerhead Oil Sample"

enum eFuel_station
{
    FLASKA1                         = 56824,
    FLASKA2                         = 56825,
    FLASKA3                         = 56826,
    Promising_Fuel_Sample           = 56833,
    Anemic_Fuel_Sample              = 56829,
    Atomic_Fuel_Sample              = 56828,
    Billowing_Fuel_Sample           = 56827,
    Smoky_Fuel_Sample               = 56831,
    QUEST_FUELOLOGY101              = 26106
};

class go_Fuel_Sampling_Station: public GameObjectScript
{
public:
    go_Fuel_Sampling_Station() : GameObjectScript("go_Fuel_station") { }

    bool CheckForChemistry(Player* pPlayer)
    {
        // Vraci true kdyz ma dost reagentu
        // nebo false pokud ma malo reagentu

        // Pro pripad neplatnyho pointeru, nemelo by se stat, ale buh vi
        if (!pPlayer)
            return false;

        uint32 sumcount = 0;
        uint32 flasks[] = {FLASKA1, FLASKA2, FLASKA3};

        for (uint32 i = 0; i < sizeof(flasks)/sizeof(uint32); i++)
            sumcount += pPlayer->GetItemCount(flasks[i], false);

        if (sumcount >= 5)
            return true;

        return false;
    }

    bool OnGossipHello(Player *pPlayer, GameObject *pGO)
    {
        if (pPlayer->HasItemCount(Promising_Fuel_Sample,1) || pPlayer->HasItemCount(Anemic_Fuel_Sample,1) || pPlayer->HasItemCount(Atomic_Fuel_Sample,1)
            || pPlayer->HasItemCount(Billowing_Fuel_Sample,1) || pPlayer->HasItemCount(Smoky_Fuel_Sample,1))
            pPlayer->SEND_GOSSIP_MENU(1, pGO->GetGUID());
        else
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Let's get started!", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
        pPlayer->SEND_GOSSIP_MENU(1, pGO->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player *pPlayer, GameObject *pGO, uint32 /*uiSender*/, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();

        switch(uiAction)
        {
            case GOSSIP_ACTION_INFO_DEF: // definice hlavniho menu
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Terrapin Oil Sample", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Remora Oil Sample", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Hammerhead Oil Sample", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "I want to start again", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Mix it!", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+5);
                pPlayer->SEND_GOSSIP_MENU(1, pGO->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF + 1: //Terrapin Oil
                pPlayer->AddItem(FLASKA1,1);
                if (!CheckForChemistry(pPlayer))
                {
                    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Terrapin Oil Sample", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
                    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Remora Oil Sample", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
                    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Hammerhead Oil Sample", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                }
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "I want to start again", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Mix it!", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+5);
                pPlayer->SEND_GOSSIP_MENU(1, pGO->GetGUID());
                break;
           case GOSSIP_ACTION_INFO_DEF + 2:  //Remora Oil
                pPlayer->AddItem(FLASKA2,1);
                if (!CheckForChemistry(pPlayer))
                {
                    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Terrapin Oil Sample", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
                    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Remora Oil Sample", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
                    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Hammerhead Oil Sample", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                }
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "I want to start again", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Mix it!", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+5);
                pPlayer->SEND_GOSSIP_MENU(1, pGO->GetGUID());
                break;
           case GOSSIP_ACTION_INFO_DEF + 3: //Hammerhead Oil
                pPlayer->AddItem(FLASKA3,1);
                if (!CheckForChemistry(pPlayer))
                {
                    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Terrapin Oil Sample", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
                    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Remora Oil Sample", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
                    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Hammerhead Oil Sample", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                }
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "I want to start again", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Mix it!", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+5);
                pPlayer->SEND_GOSSIP_MENU(1, pGO->GetGUID());
                break;
           case GOSSIP_ACTION_INFO_DEF + 4: // Smaze vsechny quest itemy co ma hrac u sebe
                pPlayer->DestroyItemCount(FLASKA1,10, true, false);
                pPlayer->DestroyItemCount(FLASKA2,10, true, false);
                pPlayer->DestroyItemCount(FLASKA3,10, true, false);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Terrapin Oil Sample", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Remora Oil Sample", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Hammerhead Oil Sample", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "I want to start again", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4);
                pPlayer->SEND_GOSSIP_MENU(1, pGO->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF + 5: //kontrola
                if (pPlayer->HasItemCount(FLASKA2,2) && pPlayer->HasItemCount(FLASKA3,3)) // Promising_Fuel_Sample
                {
                    pPlayer->AddItem(Promising_Fuel_Sample,1);
                    pPlayer->GetItemByEntry(Promising_Fuel_Sample);
                    pPlayer->CLOSE_GOSSIP_MENU();
                }
                else if (pPlayer->HasItemCount(FLASKA1,1) && pPlayer->HasItemCount(FLASKA2,2) && pPlayer->HasItemCount(FLASKA3,2))// Smoky_Fuel_Sample
                {
                    pPlayer->AddItem(Smoky_Fuel_Sample,1);
                    pPlayer->CLOSE_GOSSIP_MENU();
                }
                else if (pPlayer->HasItemCount(FLASKA1,5))// Billowing_Fuel_sample 5T
                {
                    pPlayer->AddItem(Billowing_Fuel_Sample,1);
                    pPlayer->CLOSE_GOSSIP_MENU();
                }
                else if (pPlayer->HasItemCount(FLASKA1,2) && pPlayer->HasItemCount(FLASKA2,3))//Billowing_Fuel_sample 2T+3R
                {
                    pPlayer->AddItem(Billowing_Fuel_Sample,1);
                    pPlayer->CLOSE_GOSSIP_MENU();
                }
                else if (pPlayer->HasItemCount(FLASKA1,2) && pPlayer->HasItemCount(FLASKA3,3))//Billowing_Fuel_sample 2T+3H
                {
                    pPlayer->AddItem(Billowing_Fuel_Sample,1);
                    pPlayer->CLOSE_GOSSIP_MENU();
                }
                else if (pPlayer->HasItemCount(FLASKA1,2) && pPlayer->HasItemCount(FLASKA2,1) && pPlayer->HasItemCount(FLASKA3,2))//Billowing_Fuel_sample 2T+1R+2H
                {
                    pPlayer->AddItem(Billowing_Fuel_Sample,1);
                    pPlayer->CLOSE_GOSSIP_MENU();
                }
                else if (pPlayer->HasItemCount(FLASKA1,2) && pPlayer->HasItemCount(FLASKA2,2) && pPlayer->HasItemCount(FLASKA3,1))//Billowing_Fuel_sample 2+T+1H+2R
                {
                    pPlayer->AddItem(Billowing_Fuel_Sample,1);
                    pPlayer->CLOSE_GOSSIP_MENU();
                }
                else if (pPlayer->HasItemCount(FLASKA2,3) && pPlayer->HasItemCount(FLASKA3,2))//Atomic_fuel_sample 3R+2H
                {
                    pPlayer->AddItem(Atomic_Fuel_Sample,1);
                    pPlayer->CLOSE_GOSSIP_MENU();
                }
                else if (pPlayer->HasItemCount(FLASKA2,3) && pPlayer->HasItemCount(FLASKA1,2))//Atomic_fuel_sample  3R+2T
                {
                    pPlayer->AddItem(Atomic_Fuel_Sample,1);
                    pPlayer->CLOSE_GOSSIP_MENU();
                }
                else if (pPlayer->HasItemCount(FLASKA2,3) && pPlayer->HasItemCount(FLASKA1,1) && pPlayer->HasItemCount(FLASKA3,1))//Atomic_fuel_sample 3R+1H+1T
                {
                    pPlayer->AddItem(Atomic_Fuel_Sample,1);
                    pPlayer->CLOSE_GOSSIP_MENU();
                }
                else if (pPlayer->HasItemCount(FLASKA2,3) && pPlayer->HasItemCount(FLASKA1,1) && pPlayer->HasItemCount(FLASKA3,3) && pPlayer->HasItemCount(FLASKA2,1))//Anemic 1T+3H+1R
                {
                    pPlayer->AddItem(Anemic_Fuel_Sample,1);
                    pPlayer->CLOSE_GOSSIP_MENU();
                }
                else if (pPlayer->HasItemCount(FLASKA1,1) && pPlayer->HasItemCount(FLASKA3,4))//Anemic 4H+1T
                {
                    pPlayer->AddItem(Anemic_Fuel_Sample,1);
                    pPlayer->CLOSE_GOSSIP_MENU();
                }
                else if (pPlayer->HasItemCount(FLASKA2,3) && pPlayer->HasItemCount(FLASKA1,1) && pPlayer->HasItemCount(FLASKA3,1))//4H+1R
                {
                    pPlayer->AddItem(Anemic_Fuel_Sample,1);
                    pPlayer->CLOSE_GOSSIP_MENU();
                }
                else if (pPlayer->HasItemCount(FLASKA3,5))//5h
                {
                    pPlayer->AddItem(Anemic_Fuel_Sample,1);
                    pPlayer->CLOSE_GOSSIP_MENU();
                }
                else // projistotu, vytvorit Billowing + sebrat vsechny reagenty, kdyz nedojde ani k jednej shode
                {
                    pPlayer->AddItem(Billowing_Fuel_Sample,1);
                    pPlayer->CLOSE_GOSSIP_MENU();
                }

                // Znicit vsechny mozny flasky
                pPlayer->DestroyItemCount(FLASKA3, 10, true, false);
                pPlayer->DestroyItemCount(FLASKA2, 10, true, false);
                pPlayer->DestroyItemCount(FLASKA1, 10, true, false);

                // Zavrit gossip okno jen pri zmixovani - to zabrani probliknuti okna
                pPlayer->PlayerTalkClass->CloseGossip();

                break;
        }
        return true;
    }
};

enum eEnginer
{
    say_smoky                           = -1999959,
    say_billowing                       = -1999958,
    say_atomic                          = -1999957,
    say_anemic                          = -1999956,
    say_complete                        = -1999955,
};

#define GOSSIP_ITEM_PROMISING   "Here I made a Promising Fuel Sample. Three parts hammerhead and two parts remora."
#define GOSSIP_ITEM_ANEMIC      "Here I made Anemic Fuel Sample."
#define GOSSIP_ITEM_ATOMIC      "Here I made Atomic Fuel Sample."
#define GOSSIP_ITEM_BILLOWING   "Here I made Billowing Fuel Sample."
#define GOSSIP_ITEM_SMOKY       "Here I made Smoky Fuel Sample"

class npc_Enginner : public CreatureScript
{
public:
    npc_Enginner() : CreatureScript("npc_Enginner") { }

    bool OnGossipHello(Player *pPlayer, Creature *pCreature)
    {
        pPlayer->PrepareQuestMenu(pCreature->GetGUID());

        if(pPlayer->GetQuestStatus(QUEST_FUELOLOGY101) == QUEST_STATUS_INCOMPLETE && pPlayer->HasItemCount(Promising_Fuel_Sample, 1))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_PROMISING, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

        if(pPlayer->GetQuestStatus(QUEST_FUELOLOGY101) == QUEST_STATUS_INCOMPLETE && pPlayer->HasItemCount(Anemic_Fuel_Sample, 1))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_ANEMIC, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);

        if(pPlayer->GetQuestStatus(QUEST_FUELOLOGY101) == QUEST_STATUS_INCOMPLETE && pPlayer->HasItemCount(Atomic_Fuel_Sample, 1))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_ATOMIC, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);

        if(pPlayer->GetQuestStatus(QUEST_FUELOLOGY101) == QUEST_STATUS_INCOMPLETE && pPlayer->HasItemCount(Billowing_Fuel_Sample, 1))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_BILLOWING, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4);

        if(pPlayer->GetQuestStatus(QUEST_FUELOLOGY101) == QUEST_STATUS_INCOMPLETE && pPlayer->HasItemCount(Smoky_Fuel_Sample, 1))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_SMOKY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+5);

        pPlayer->SEND_GOSSIP_MENU(1, pCreature->GetGUID());

        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->CloseGossip();

        switch(uiAction)
        {
            case GOSSIP_ACTION_INFO_DEF + 1:
            {
                pPlayer->CompleteQuest(QUEST_FUELOLOGY101);//promising
                pPlayer->DestroyItemCount(Promising_Fuel_Sample,1,true,false);
                DoScriptText(say_complete,pCreature);
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 2://anemic
            {
                pPlayer->DestroyItemCount(Anemic_Fuel_Sample,1,true,false);
                DoScriptText(say_anemic,pCreature);
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 3://atomic
            {
                pPlayer->DestroyItemCount(Atomic_Fuel_Sample,1,true,false);
                DoScriptText(say_atomic,pCreature);
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 4://billowing
            {
                pPlayer->DestroyItemCount(Billowing_Fuel_Sample,1,true,false);
                DoScriptText(say_billowing,pCreature);
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 5://smoky
            {
                pPlayer->DestroyItemCount(Smoky_Fuel_Sample,1,true,false);
                DoScriptText(say_smoky,pCreature);
                break;
            }
        }
        pPlayer->CLOSE_GOSSIP_MENU();
        return true;
    }
};

void AddSC_vashjir()
{
    new go_Fuel_Sampling_Station;
    new npc_Enginner;
}
