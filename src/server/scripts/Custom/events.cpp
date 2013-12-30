/*
 * Copyright (C) 2006-2013 iCe Online <http://www.ice-wow.eu/>
 *
 * This program is not free software. iCe GM Team owns all of its content
 * Who won't obey that rule, i will kick his balls and twist his nipples.
 *
 */

#include "ScriptPCH.h"
#include "Guild.h"

/* Guild House System, fucked up by HyN3Q, fixed and maintained by Gregory */

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

            QueryResult res = ScriptDatabase.PQuery("SELECT rank, password, phase, map, x, y, z FROM gh_system WHERE guildid = %u", pPlayer->GetGuildId());

            uint32 phase;
            int32 rank;
            std::string password;
            uint32 map;
            float x, y, z;

            if (res) // Process
            {
                Field* field = res->Fetch();
                rank = field[0].GetInt32();
                password = field[1].GetString();
                phase = field[2].GetUInt32();
                map = field[3].GetUInt32();
                x = field[4].GetFloat();
                y = field[5].GetFloat();
                z = field[6].GetFloat();

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
                                pPlayer->SetPhaseMask(phase, true);
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
                        int32 cislo = atoi(code);
                        // Process
                        if (pPlayer->HasEnoughMoney(500*GOLD)) // Check, if player has 500 gold
                        {
                            if (cislo == -1)
                            {
                                ScriptDatabase.PExecute("UPDATE gh_system SET rank = %i WHERE guildid = %u", cislo, pPlayer->GetGuildId());
                                pPlayer->ModifyMoney(-500*GOLD);
                                ChatHandler(pPlayer).PSendSysMessage("Modified 500g money.");
                            }
                            else if (cislo > 0)
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

#define LOTTERY_WORLD_STATE 13331
#define LOTTERY_TICKET_ITEM 190000

class LotteryHelper: public WorldScript
{
    public:
        LotteryHelper(): WorldScript("lottery_world_script")
        {
            if (sWorld->getWorldState(LOTTERY_WORLD_STATE) == 0)
                sWorld->setWorldState(LOTTERY_WORLD_STATE, (uint64)time(NULL)+WEEK);

            time_flush = sWorld->getWorldState(LOTTERY_WORLD_STATE);
        }

        uint32 time_flush;

        void OnUpdate(void* dunno, uint32 diff)
        {
            if (time_flush < time(NULL))
            {
                sWorld->setWorldState(LOTTERY_WORLD_STATE, (uint64)time(NULL)+WEEK);
                time_flush = sWorld->getWorldState(LOTTERY_WORLD_STATE);

                QueryResult res = CharacterDatabase.PQuery("SELECT owner_guid FROM item_instance WHERE itemEntry = %u;", LOTTERY_TICKET_ITEM);
                if (res)
                {
                    std::vector<uint64> holderMap;
                    uint32 cnt = 0;
                    do
                    {
                        holderMap.push_back((*res)[0].GetUInt64());
                        cnt++;
                    } while (res->NextRow());

                    // Get rid of all tickets

                    // online players
                    SessionMap const& sessMap = sWorld->GetAllSessions();
                    Player* tmp = NULL;
                    for (SessionMap::const_iterator itr = sessMap.begin(); itr != sessMap.end(); ++itr)
                    {
                        tmp = itr->second->GetPlayer();
                        if (!tmp)
                            continue;

                        if (tmp->GetItemCount(LOTTERY_TICKET_ITEM, false) > 0)
                            tmp->DestroyItemCount(LOTTERY_TICKET_ITEM, tmp->GetItemCount(LOTTERY_TICKET_ITEM, false), true);
                    }
                    // offline players
                    CharacterDatabase.PExecute("DELETE FROM item_instance WHERE itemEntry = %u;", LOTTERY_TICKET_ITEM);

                    // Choose winner

                    if (holderMap.empty())
                        return;

                    uint32 winnerPos = urand(0, holderMap.size()-1);

                    uint64 target_guid = holderMap[winnerPos];
                    Player* target = sObjectMgr->GetPlayer(target_guid);

                    uint64 moneyamount = (cnt*100000)*0.8f;
                    uint64 moneyamount_notvat = (cnt*100000);

                    std::stringstream ss;
                    ss << "Congratulations! You won " << moneyamount/GOLD << " golds in goblin lottery!\n\nThat's " << moneyamount_notvat/GOLD << " golds original minus 20% vat.";

                    // send mail with money to our winner
                    MailSender sender(MAIL_NORMAL, 0, MAIL_STATIONERY_GM);
                    SQLTransaction trans = CharacterDatabase.BeginTransaction();
                    MailDraft* md = new MailDraft("Goblin Lottery", ss.str().c_str());
                    md->AddMoney(moneyamount);
                    md->SendMailTo(trans, MailReceiver(target,GUID_LOPART(target_guid)),sender);
                    delete md;

                    CharacterDatabase.CommitTransaction(trans);
                }
            }
        }
};

void AddSC_custom_events()
{
    new npc_gh;
    new LotteryHelper();
}
