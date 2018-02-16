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
                                if (!pPlayer->IsInCombat())
                                {
                                    WorldPacket data(SMSG_MESSAGECHAT, 200);
                                    pCreature->BuildMonsterChat(&data, CHAT_MSG_RAID_BOSS_EMOTE, "You will be transferred to your guild house in 5 seconds.", LANG_UNIVERSAL, pCreature->GetName(), pPlayer->GetGUID());
                                    pPlayer->GetSession()->SendPacket(&data);

                                    npc_gh::gh_teleporterAI* myAI = CAST_AI(npc_gh::gh_teleporterAI, pCreature->GetAI());
                                    if (myAI)
                                        myAI->AddTransfer(pPlayer->GetGUID(), map, x, y, z, phase);
                                    else
                                        pPlayer->GetSession()->SendNotification(LANG_ERROR);
                                }
                                else
                                    pPlayer->GetSession()->SendNotification(LANG_YOU_IN_COMBAT);
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
                        int64 cost = 500 * GOLD;
                        // Process
                        if (pPlayer->HasEnoughMoney(cost)) // Check, if player has 500 gold
                        {
                            if (cislo == -1)
                            {
                                ScriptDatabase.PExecute("UPDATE gh_system SET rank = %i WHERE guildid = %u", cislo, pPlayer->GetGuildId());
                                pPlayer->ModifyMoney(-cost);
                                ChatHandler(pPlayer).PSendSysMessage("Modified 500g money.");
                            }
                            else if (cislo > 0)
                            {
                                ScriptDatabase.PExecute("UPDATE gh_system SET rank = %i WHERE guildid = %u", cislo-1, pPlayer->GetGuildId());
                                pPlayer->ModifyMoney(-cost);
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
                        int64 cost = 1000 * GOLD;
                        if (pPlayer->HasEnoughMoney(cost)) // Check, if player has 1K gold
                        {
                            std::string old_pw = password.c_str();
                            ScriptDatabase.PExecute("UPDATE gh_system SET password = '%s' WHERE guildid = %u", code, pPlayer->GetGuildId());
                            pPlayer->ModifyMoney(-cost);
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

        struct gh_teleporterAI : public ScriptedAI
        {
            gh_teleporterAI(Creature *c) : ScriptedAI(c) {}

            const uint32 TRANSFER_TIME = 5000;

            struct TransferRecord
            {
                uint64 guid;
                uint32 mapId;
                float x, y, z;
                uint16 phase;

                uint32 teleportTimer;
            };

            std::list<TransferRecord> m_transferList;

            void Reset() override
            {
                m_transferList.clear();
            }

            void AddTransfer(uint64 guid, uint32 mapId, float x, float y, float z, uint16 phase)
            {
                m_transferList.push_back({
                    guid,
                    mapId,
                    x, y, z,
                    phase,
                    TRANSFER_TIME
                });

                if (auto ts = me->ToTempSummon())
                    ts->InitStats(TRANSFER_TIME * 2);
            }

            void UpdateAI(const uint32 diff) override
            {
                if (m_transferList.size() == 0)
                    return;

                for (auto itr = m_transferList.begin(); itr != m_transferList.end(); )
                {
                    auto& rec = *itr;

                    if (rec.teleportTimer <= diff)
                    {
                        Player* pl = sObjectMgr->GetPlayer(rec.guid);
                        if (pl)
                        {
                            if (!pl->IsInCombat())
                            {
                                if (pl->GetDistance2d(me) < 10.0f)
                                {
                                    pl->SetPhaseMask(rec.phase, true);
                                    pl->TeleportTo(rec.mapId, rec.x, rec.y, rec.z, 0.0f);
                                    pl->SetPhaseMask(rec.phase, true);
                                }
                                else
                                    pl->GetSession()->SendNotification("You are too far away from portal");
                            }
                            else
                                pl->GetSession()->SendNotification(LANG_YOU_IN_COMBAT);
                        }

                        itr = m_transferList.erase(itr);
                    }
                    else
                    {
                        rec.teleportTimer -= diff;
                        ++itr;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new gh_teleporterAI(pCreature);
        }
};

#define LOTTERY_WORLD_STATE 13331
#define LOTTERY_TICKET_ITEM 190000

class LotteryHelper: public WorldScript
{
    public:
        LotteryHelper(): WorldScript("lottery_world_script")
        {
            time_flush = 0;
        }

        time_t time_flush;

        void OnUpdate(void* dunno, uint32 diff)
        {
            if (time_flush == 0)
            {
                if (sWorld->getWorldState(LOTTERY_WORLD_STATE) == 0)
                    sWorld->setWorldState(LOTTERY_WORLD_STATE, (uint64)time(NULL)+WEEK);

                time_flush = sWorld->getWorldState(LOTTERY_WORLD_STATE);
            }

            if (time_flush < time(NULL))
            {
                sWorld->setWorldState(LOTTERY_WORLD_STATE, (uint64)time(NULL)+WEEK);
                time_flush = sWorld->getWorldState(LOTTERY_WORLD_STATE);

                QueryResult res = CharacterDatabase.PQuery("SELECT owner_guid, count FROM item_instance WHERE itemEntry = %u;", LOTTERY_TICKET_ITEM);
                if (res)
                {
                    std::vector<uint64> holderMap;
                    uint32 cnt = 0;
                    do
                    {
                        // the more tickets you have, the greater chance you have
                        for (uint32 i = 0; i < (*res)[1].GetUInt32(); i++)
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

                    QueryResult res = CharacterDatabase.PQuery("SELECT name FROM characters WHERE guid = %u", GUID_LOPART(target_guid));
                    if (res)
                        sLog->outChar("Goblin Lottery: player %s (%u) has won " UI64FMTD " gold (without vat: " UI64FMTD " gold)!", (*res)[0].GetString().c_str(), GUID_LOPART(target_guid), moneyamount / GOLD, moneyamount_notvat / GOLD);
                }
            }
        }
};

void AddSC_custom_events()
{
    new npc_gh;
    new LotteryHelper();
}
