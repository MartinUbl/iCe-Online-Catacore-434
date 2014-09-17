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
#include "endtime.h"

// Time Transit Device
#define ENTRANCE           "Teleport to Entrance"                // Entrance
#define SYLVANAS           "Teleport to Ruby Dragonshrine"       // Sylvanas
#define TYRANDE            "Teleport to Emerald Dragonsshrine"   // Tyrande
#define JAINA              "Teleport to Azure Dragonshrine"      // Jaina
#define BAINE              "Teleport to Obsidian Dragonshrine"   // Baine
#define MUROZOND           "Teleport to Bronze Dragonshrine"     // Murozond

// Quests
enum Quests
{
    SYLVANAS_AND_JAINA         = 158700,
    BAINE_AND_TYRANDE          = 158701,
    SYLVANAS_AND_BAINE         = 158702,
    JAINA_AND_TYRANDE          = 158703,
    SYLVANAS_AND_TYRANDE       = 158704,
    JAINA_AND_BAINE            = 158705,
};

// Spells
enum NPC
{
    SPELL_ENTRANCE        = 102564,
    SPELL_SYLVANAS        = 102579,
    SPELL_JAINA           = 102126,
    SPELL_BAINE           = 103868,
    SPELL_TYRANDE         = 104761,
    SPELL_MUROZOND        = 104764,
};

class go_time_transit_device : public GameObjectScript
{
public:
    go_time_transit_device() : GameObjectScript("go_time_transit_device") { }

    bool OnGossipSelect(Player* pPlayer, GameObject* pGo, uint32 uiSender, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        switch(uiSender)
        {
            case GOSSIP_SENDER_MAIN:    SendActionMenu(pPlayer, pGo, uiAction); break;
        }
        return true;
    }

    bool OnGossipHello(Player* pPlayer, GameObject* pGo)
    {
        // Sylvanas and Jaina
        if (pPlayer->GetQuestStatus(SYLVANAS_AND_JAINA) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ENTRANCE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, SYLVANAS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, JAINA, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        }
        // Baine and Tyrande
        if (pPlayer->GetQuestStatus(BAINE_AND_TYRANDE) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ENTRANCE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, TYRANDE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BAINE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
        }
        // Sylvanas and Baine
        if (pPlayer->GetQuestStatus(SYLVANAS_AND_BAINE) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ENTRANCE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, SYLVANAS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BAINE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
        }
        // Jaina and Tyrande
        if (pPlayer->GetQuestStatus(JAINA_AND_TYRANDE) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ENTRANCE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, JAINA, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, TYRANDE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
        }
        // Sylvanas and Tyrande
        if (pPlayer->GetQuestStatus(SYLVANAS_AND_TYRANDE) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ENTRANCE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, SYLVANAS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, TYRANDE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
        }
        // Jaina and Baine
        if (pPlayer->GetQuestStatus(JAINA_AND_BAINE) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ENTRANCE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, JAINA, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BAINE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
        }
        // Enable port to Murozond if Q for Echoes complete (resets every day)
        if ((pPlayer->GetQuestStatus(SYLVANAS_AND_JAINA) == QUEST_STATUS_COMPLETE) || (pPlayer->GetQuestStatus(BAINE_AND_TYRANDE) == QUEST_STATUS_COMPLETE)
            || (pPlayer->GetQuestStatus(SYLVANAS_AND_BAINE) == QUEST_STATUS_COMPLETE) || (pPlayer->GetQuestStatus(JAINA_AND_TYRANDE) == QUEST_STATUS_COMPLETE)
            || (pPlayer->GetQuestStatus(SYLVANAS_AND_TYRANDE) == QUEST_STATUS_COMPLETE) || (pPlayer->GetQuestStatus(JAINA_AND_BAINE) == QUEST_STATUS_COMPLETE))
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ENTRANCE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, MUROZOND, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 6);
        }

        pPlayer->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, pGo->GetGUID());
        return true;
    }

    void SendActionMenu(Player* pPlayer, GameObject* /*pGo*/, uint32 uiAction)
    {
        switch(uiAction)
        {
            case GOSSIP_ACTION_INFO_DEF + 1:
                pPlayer->CastSpell(pPlayer,SPELL_ENTRANCE,false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 2:
                  pPlayer->CastSpell(pPlayer,SPELL_SYLVANAS,false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 3:
                pPlayer->CastSpell(pPlayer,SPELL_JAINA,false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 4:
                pPlayer->CastSpell(pPlayer,SPELL_BAINE,false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 5:
                pPlayer->CastSpell(pPlayer,SPELL_TYRANDE,false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 6:
                pPlayer->CastSpell(pPlayer,SPELL_MUROZOND,false);
                break;
        }
    }
};

void AddSC_instance_end_time()
{
    new go_time_transit_device();
}