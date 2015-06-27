/*
* Copyright (C) 2006-2015 iCe Online <http://www.ice-wow.eu/>
*
* This part of program is not free software. iCe GM Team owns all of its content
* Who won't obey that rule, i will kick his balls and twist his nipples.
*
*/

#include "ScriptPCH.h"

struct soulwell_basic_record
{
    uint32 originalAccountId;
    std::string name;
    uint8 level;
    uint8 race;
    uint8 pclass;
    uint64 money;
    uint64 totaltime;
    uint64 leveltime;
    uint8 speccount;
    uint32 knownTitles[KNOWN_TITLES_SIZE * 2];
};

struct soulwell_item_transfer_record
{
    uint32 id;
    uint64 creatorGuid;
    uint32 count;
    uint32 duration;
    uint32 charges;
    uint32 flags;
    uint32 randomPropertyId;
    uint32 enchantments[MAX_ENCHANTMENT_SLOT];
    uint32 durability;
    std::string text;
};

struct soulwell_spell_transfer_record
{
    uint32 spell;
    // ?
};

struct soulwell_achievement_progress_record
{
    uint32 criteriaId;
    uint32 counter;
    uint32 date;
};

struct soulwell_achievement_record
{
    uint32 achievementId;
    uint32 date;
};

struct soulwell_currency_record
{
    uint32 currency;
    uint32 count;
    uint32 weekcount;
    uint32 seasoncount;
};

struct soulwell_skill_record
{
    uint32 skillId;
    uint32 value;
    uint32 max;
};

struct soulwell_reputation_record
{
    uint32 factionId;
    uint32 standing;
    uint32 flags;
};

class soulwell_transfer_npc : public CreatureScript
{
public:
    soulwell_transfer_npc() : CreatureScript("npc_soulwell_transfer")
    {
        //
    }

    enum SWTransferPhase
    {
        SWT_NONE = 0,

        SWT_BASIC = 1,
        SWT_ITEMS = 2,
        SWT_SPELLS = 3,
        SWT_ACHIEVEMENTS = 4,
        SWT_CURRENCY = 5,
        SWT_SKILLS = 6,
        SWT_REPUTATION = 7,

        SWT_END
    };

    struct soulwell_transfer_npcAI : public ScriptedAI
    {
        uint64 lockedPlayerGUID;
        bool loadingPhase;
        uint32 transferPhase = 0;

        std::string username;
        std::string password;
        std::string charactername;

        soulwell_basic_record basicInfo;
        std::list<soulwell_item_transfer_record*> items;
        std::list<soulwell_spell_transfer_record*> spells;
        std::list<soulwell_achievement_progress_record*> achievementProgress;
        std::list<soulwell_achievement_record*> achievements;
        std::list<soulwell_currency_record*> currency;
        std::list<soulwell_skill_record*> skills;
        std::list<soulwell_reputation_record*> reputation;

        std::list<soulwell_item_transfer_record*>::iterator itemsItr;
        std::list<soulwell_spell_transfer_record*>::iterator spellsItr;
        std::list<soulwell_achievement_progress_record*>::iterator achievementProgressItr;
        std::list<soulwell_achievement_record*>::iterator achievementsItr;
        std::list<soulwell_currency_record*>::iterator currencyItr;
        std::list<soulwell_skill_record*>::iterator skillsItr;
        std::list<soulwell_reputation_record*>::iterator reputationItr;

        soulwell_transfer_npcAI(Creature* c) : ScriptedAI(c)
        {
            lockedPlayerGUID = 0;
            loadingPhase = true;
            transferPhase = SWT_NONE;

            memset(&basicInfo, 0, sizeof(soulwell_basic_record));

            username = "";
            password = "";
            charactername = "";
        };

        void UsePlayerToTransfer(uint64 guid)
        {
            lockedPlayerGUID = guid;
        }

        bool LoadSoulwellCharacter()
        {
            QueryResult res = CharacterDatabase.PQuery("SELECT id FROM soulwell_auth.account WHERE username = '%s' AND sha_pass_hash = SHA1(CONCAT(UPPER('%s'), ':', UPPER('%s')))", username.c_str(), username.c_str(), password.c_str());
            if (!res)
                return false;

            Field* f = res->Fetch();
            if (!f)
                return false;

            basicInfo.originalAccountId = f[0].GetUInt32();

            res = CharacterDatabase.PQuery("SELECT level, race, class, money, totaltime, leveltime, speccount, knownTitles FROM soulwell_char.characters WHERE account = %u AND name = '%s'", basicInfo.originalAccountId, charactername.c_str());
            if (!res)
                return false;

            f = res->Fetch();
            if (!f)
                return false;

            basicInfo.level = f[0].GetUInt8();
            basicInfo.race = f[1].GetUInt8();
            basicInfo.pclass = f[2].GetUInt8();
            basicInfo.money = f[3].GetUInt64();
            basicInfo.totaltime = f[4].GetUInt64();
            basicInfo.leveltime = f[5].GetUInt64();
            basicInfo.speccount = f[6].GetUInt8();

            const char* knownTitles = f[7].GetCString();

            Tokens tokens(knownTitles, ' ', KNOWN_TITLES_SIZE * 2);

            // this may mean the player does not have any titles at all
            if (tokens.size() != KNOWN_TITLES_SIZE * 2)
                return true;

            for (uint32 index = 0; index < KNOWN_TITLES_SIZE * 2; ++index)
                basicInfo.knownTitles[index] = atol(tokens[index]);

            return true;
        }

        bool StartTransfer()
        {
            Player* dest = Player::GetPlayer(*me, lockedPlayerGUID);
            if (!dest)
                return false;

            if (dest->getRace() != basicInfo.race)
                return false;

            if (dest->getClass() != basicInfo.pclass)
                return false;

            // TODO: load everything needed to be transfered

            return true;
        }

        void UpdateAI(const uint32 diff)
        {
            //
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new soulwell_transfer_npcAI(c);
    }

    bool OnGossipHello(Player *player, Creature *creature)
    {
        soulwell_transfer_npcAI* pAI = CAST_AI(soulwell_transfer_npc::soulwell_transfer_npcAI, creature->AI());

        if (!pAI)
            return true;

        if (pAI->username.size() == 0)
            SendStageGossip(player, creature, 0);
        else if (pAI->password.size() == 0)
            SendStageGossip(player, creature, 1);
        else if (pAI->charactername.size() == 0)
            SendStageGossip(player, creature, 2);
        else
        {
            std::string outstr = "Begin transfer of character " + pAI->charactername;
            SendStageGossip(player, creature, 3, outstr.c_str());
        }

        return true;
    }

    void SendStageGossip(Player* player, Creature* creature, uint8 stage, const char* helperstr = nullptr)
    {
        switch (stage)
        {
            case 0:
                player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, "Enter username", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1, "", 0, true);
                break;
            case 1:
                player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, "Enter password (not hidden!)", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2, "", 0, true);
                break;
            case 2:
                player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, "Enter character name", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3, "", 0, true);
                break;
            case 3:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, helperstr, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                break;
        }

        if (stage != 0)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Reset values and start again", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 99);

        player->SEND_GOSSIP_MENU(1, creature->GetGUID());
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 uiSender, uint32 uiAction)
    {
        player->PlayerTalkClass->ClearMenus();
        player->CLOSE_GOSSIP_MENU();

        soulwell_transfer_npcAI* pAI = CAST_AI(soulwell_transfer_npc::soulwell_transfer_npcAI, creature->AI());

        if (!pAI)
            return false;

        if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
        {
            if (pAI->LoadSoulwellCharacter())
            {
                ChatHandler(player).PSendSysMessage("Sucessfully authenticated with username %s. Character selected: %s", pAI->username.c_str(), pAI->charactername.c_str());
                if (pAI->StartTransfer())
                    ChatHandler(player).SendSysMessage("Character transfer has been initiated...");
                else
                    ChatHandler(player).SendSysMessage("Race and class pair must match with newly created character!");
            }
            else
                ChatHandler(player).PSendSysMessage("Invalid username, password, or character name!");
        }
        else if (uiAction == GOSSIP_ACTION_INFO_DEF + 99)
        {
            pAI->username = "";
            pAI->password = "";
            pAI->charactername = "";
            SendStageGossip(player, creature, 0);
        }

        return true;
    }

    bool OnGossipSelectCode(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction, const char* code)
    {
        player->PlayerTalkClass->ClearMenus();
        soulwell_transfer_npcAI* pAI = CAST_AI(soulwell_transfer_npc::soulwell_transfer_npcAI, creature->AI());

        if (!pAI)
            return false;

        player->CLOSE_GOSSIP_MENU();

        if (pAI->lockedPlayerGUID != 0 && pAI->lockedPlayerGUID != player->GetGUID())
            return true;

        switch (uiAction)
        {
            case GOSSIP_ACTION_INFO_DEF + 1:
            {
                if (pAI->lockedPlayerGUID == 0)
                    pAI->UsePlayerToTransfer(player->GetGUID());

                pAI->username = code;
                SendStageGossip(player, creature, 1);
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 2:
            {
                pAI->password = code;
                SendStageGossip(player, creature, 2);
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 3:
            {
                pAI->charactername = code;

                std::string outstr = "Begin transfer of character " + pAI->charactername;
                SendStageGossip(player, creature, 3, outstr.c_str());
                break;
            }
            default:
                return false;
        }

        return true;
    }
};

void AddSC_soulwell_transfer()
{
    new soulwell_transfer_npc();
}
