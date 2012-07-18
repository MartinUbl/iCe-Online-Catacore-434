/*
 * Copyright (C) 2006-2012 iCe Online <http://www.ice-wow.eu/>
 *
 * This program is not free software. iCe GM Team owns all of its content
 * Who won't obey that rule, i will kick his balls and twist his nipples.
 *
 */

#include "ScriptPCH.h"
#include "Guild.h"

/* Guild House System, completed scripted by HyN3Q with cooperation Gregory. Ty ! */

class npc_gh: public CreatureScript
{
    public:
        npc_gh(): CreatureScript("npc_gh") {}

        bool OnGossipHello(Player* pPlayer, Creature* pCreature)
        {
            if (pPlayer->GetGuildId() == 0) // Check if in Guild
            {
                pPlayer->GetSession()->SendNotification("You don't have a guild.");
                pPlayer->CLOSE_GOSSIP_MENU();
            }
            else // if is in Guild
            {
                pPlayer->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, "I want to go to our Guild House", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1, "", 0, true);
                if (pPlayer->GetRank() == 0) // if is a Guild Master
                    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT,"Guild House Access Manager",GOSSIP_SENDER_MAIN,GOSSIP_ACTION_INFO_DEF+2);
            }
            pPlayer->SEND_GOSSIP_MENU(1,pCreature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 uiSender, uint32 uiAction)
        {
            pPlayer->PlayerTalkClass->ClearMenus();
            if (uiAction == GOSSIP_ACTION_INFO_DEF+2) // Define for Rank Access to GH
            {
                pPlayer->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, "Modify rank access to GH", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3, "", 0, true);
                pPlayer->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, "Change password", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4, "", 0, true);
            }
            pPlayer->SEND_GOSSIP_MENU(554,pCreature->GetGUID());
            return true;
        }

        bool OnGossipSelectCode(Player* pPlayer, Creature* pCreature, uint32 uiSender, uint32 uiAction, const char* code)
        {
            pPlayer->PlayerTalkClass->ClearMenus();

            QueryResult res = ScriptDatabase.PQuery("SELECT guildid, rank, password, phase, map, x, y, z FROM gh_system WHERE guildid = %u", pPlayer->GetGuildId());

            uint32 guildid;
            uint16 phase;
            int8 rank;
            std::string password;
            uint16 map;
            float x, y, z;

            if (res != NULL) // Process
            {
                Field* field = res->Fetch();
                guildid = field[0].GetUInt32();
                rank = field[1].GetInt8();
                password = field[2].GetString();
                phase = field[3].GetUInt16();
                map = field[4].GetUInt16();
                x = field[5].GetFloat();
                y = field[6].GetFloat();
                z = field[7].GetFloat();

                switch (uiAction)
                {
                    case GOSSIP_ACTION_INFO_DEF+1: // Entering to GH
                    {
                        pPlayer->CLOSE_GOSSIP_MENU();
                        const char* heslo = (const char*)password.c_str();
                        if (strcmp(heslo, code) == 0) // if is PW valid
                        {
                            if ((pPlayer->GetRank() <= rank) || (rank == -1)) // if is rank to access to enter ( -1 for all ranks)
                            {
                                pPlayer->SetPhaseMask(phase, true);
                                pPlayer->TeleportTo(map,x,y,z,0.0f); // Teleport To Dest where is GH
                            }
                            else // if not access.. bye
                                pPlayer->GetSession()->SendNotification("Your rank is too low.");
                        }
                        else // if typed an incorrect PW
                            pPlayer->GetSession()->SendNotification("You typed an incorrect password.");
                        break;
                    }
                    case GOSSIP_ACTION_INFO_DEF+3: // Define for Modify Rank Access
                    {
                        pPlayer->CLOSE_GOSSIP_MENU();
                        Guild* pGuild = sObjectMgr->GetGuildById(pPlayer->GetGuildId());
                        uint8 rank_size = pGuild->_GetRanksSize();
                        int8 cislo = atoi(code);
                        // Process
                        if (pPlayer->HasEnoughMoney(500*GOLD)) // Check, if player has 500 gold
                        {
                            if (cislo == -1)
                            {
                                ScriptDatabase.PExecute("UPDATE gh_system SET rank = %i WHERE guildid = %u", cislo, pPlayer->GetGuildId());
                                pPlayer->ModifyMoney(-500*GOLD);
                                ChatHandler(pPlayer).PSendSysMessage("Modified 500g money.");
                            }
                            else if (cislo > 0 && cislo <= rank_size)
                            {
                                ScriptDatabase.PExecute("UPDATE gh_system SET rank = %i WHERE guildid = %u", cislo-1, pPlayer->GetGuildId());
                                pPlayer->ModifyMoney(-500*GOLD);
                                ChatHandler(pPlayer).PSendSysMessage("Modified 500g money.");
                            }
                            else
                                pPlayer->GetSession()->SendNotification("You typed an invalid rank ID!");
                        }
                        else
                            pPlayer->GetSession()->SendNotification(LANG_NOT_ENOUGH_GOLD);
                        break;
                    }
                    case GOSSIP_ACTION_INFO_DEF+4: // Define for Change Password
                    {
                        pPlayer->CLOSE_GOSSIP_MENU();
                        // Process
                        if (pPlayer->HasEnoughMoney(1000*GOLD)) // Check, if player has 1K gold
                        {
                            std::string old_pw = password.c_str();
                            ScriptDatabase.PExecute("UPDATE gh_system SET password = '%s' WHERE guildid = %u", code, pPlayer->GetGuildId());
                            pPlayer->ModifyMoney(-1000*GOLD);
                            ChatHandler(pPlayer).PSendSysMessage("Modified 1000g money.");
                            pPlayer->GetSession()->SendNotification("Okay, password was changed from %s to %s", old_pw.c_str(), code);
                        }
                        else
                            pPlayer->GetSession()->SendNotification(LANG_NOT_ENOUGH_GOLD);
                        break;
                    }
                }
            }
            else // If Guild don't have a GH
            {
                pPlayer->CLOSE_GOSSIP_MENU();
                pPlayer->GetSession()->SendNotification("Your Guild don't have a Guild House.");
            }
            return true;
        }
};
void AddSC_custom_events()
{
    new npc_gh;
}
