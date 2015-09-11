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
    uint32 charges[MAX_ITEM_PROTO_SPELLS];
    uint32 flags;
    uint32 randomPropertyId;
    uint32 enchantments[MAX_ENCHANTMENT_SLOT*3];
    uint32 durability;
    std::string text;

    uint64 guid;
};

struct soulwell_item_inventory_record
{
    uint64 bag;
    uint32 slot;
};

struct soulwell_spell_transfer_record
{
    uint32 spell;
    bool active;
    bool disabled;
};

struct soulwell_achievement_progress_record
{
    uint32 criteriaId;
    uint32 counter;
    time_t date;
};

struct soulwell_achievement_record
{
    uint32 achievementId;
    time_t date;
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

#define SOULWELL_TRANSFER_TICK_TIMER 1000
#define SOULWELL_TRANSFER_BATCH_SIZE 100
#define SOULWELL_TRANSFER_TICK_SAVE_COUNT 5

#define SOULWELL_AUTH_DB "soulwell_auth"
#define SOULWELL_CHAR_DB "soulwell_char"

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
        Player* lockedPlayer; /* DO NOT use without retrieving pointer in UpdateAI (player may be offline, etc.) */
        uint32 soulwellGUID;
        bool loadingPhase;
        uint32 transferPhase = 0;
        uint32 transferTimer = 0;
        uint32 transferTicks;

        std::string username;
        std::string password;
        std::string charactername;

        soulwell_basic_record basicInfo;
        std::list<soulwell_item_transfer_record*> items;
        std::map<uint64, soulwell_item_inventory_record*> itemInventory;
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
            transferTicks = 0;

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
            QueryResult res = CharacterDatabase.PQuery("SELECT id FROM " SOULWELL_AUTH_DB ".account WHERE username = '%s' AND sha_pass_hash = SHA1(CONCAT(UPPER('%s'), ':', UPPER('%s')))", username.c_str(), username.c_str(), password.c_str());
            if (!res)
                return false;

            Field* f = res->Fetch();
            if (!f)
                return false;

            basicInfo.originalAccountId = f[0].GetUInt32();

            res = CharacterDatabase.PQuery("SELECT level, race, class, money, totaltime, leveltime, speccount, knownTitles, guid FROM " SOULWELL_CHAR_DB ".characters WHERE account = %u AND name = '%s'", basicInfo.originalAccountId, charactername.c_str());
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
            soulwellGUID = f[8].GetUInt32();

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

            // basic info is already gathered, so start basic phase
            transferPhase = SWT_BASIC;
            transferTimer = SOULWELL_TRANSFER_TICK_TIMER;
            transferTicks = 0;
            ChatHandler(lockedPlayer).SendSysMessage("Starting basic stage...");

            return true;
        }

        void ProceedBasicStage()
        {
            // transfer all basic stuff
            lockedPlayer->GiveLevel(basicInfo.level);
            lockedPlayer->ModifyMoney(basicInfo.money);
            lockedPlayer->m_Played_time[PLAYED_TIME_TOTAL] = basicInfo.totaltime;
            lockedPlayer->m_Played_time[PLAYED_TIME_LEVEL] = basicInfo.leveltime;
            lockedPlayer->SetSpecsCount(basicInfo.speccount);
            for (uint32 i = 0; i < KNOWN_TITLES_SIZE * 2; ++i)
                lockedPlayer->SetUInt32Value(PLAYER__FIELD_KNOWN_TITLES + i, basicInfo.knownTitles[i]);

            lockedPlayer->SaveToDB();

            // load next stage
            LoadItemsStage();
        }

        void LoadItemsStage()
        {
            Field *f;
            QueryResult res;

            ChatHandler(lockedPlayer).SendSysMessage("Starting items stage...");

            // at first, select inventory to decide, where to put which item
            res = CharacterDatabase.PQuery("SELECT bag, slot, item FROM " SOULWELL_CHAR_DB ".character_inventory WHERE guid = %u", soulwellGUID);
            soulwell_item_inventory_record* rec;
            if (res)
            {
                uint64 guid;
                do
                {
                    f = res->Fetch();
                    if (!f)
                        break;

                    // there are only few fields we are fancy when looking for bags
                    rec = new soulwell_item_inventory_record;
                    guid = f[2].GetUInt64();

                    rec->bag = f[0].GetUInt64();
                    rec->slot = f[1].GetUInt32();

                    itemInventory[guid] = rec;

                } while (res->NextRow());
            }

            // select bags
            res = CharacterDatabase.PQuery("SELECT itemEntry, count, creatorGuid, flags, guid FROM " SOULWELL_CHAR_DB ".item_instance WHERE guid IN (SELECT bag FROM " SOULWELL_CHAR_DB ".character_inventory WHERE guid = %u AND bag != 0)", soulwellGUID);
            soulwell_item_transfer_record* itm;

            if (res)
            {
                do
                {
                    f = res->Fetch();
                    if (!f)
                        break;

                    // there are only few fields we are fancy when looking for bags
                    itm = new soulwell_item_transfer_record;
                    itm->id = f[0].GetUInt32();
                    itm->count = f[1].GetUInt32();
                    itm->creatorGuid = f[2].GetUInt32();
                    itm->flags = f[3].GetUInt32();
                    itm->guid = f[4].GetUInt64();

                    items.push_back(itm);

                } while (res->NextRow());
            }

            // select other items
            res = CharacterDatabase.PQuery("SELECT itemEntry, creatorGuid, count, duration, charges, flags, randomPropertyId, enchantments, durability, text, guid FROM " SOULWELL_CHAR_DB ".item_instance WHERE guid IN (SELECT item FROM " SOULWELL_CHAR_DB ".character_inventory WHERE guid = %u) AND guid NOT IN (SELECT bag FROM " SOULWELL_CHAR_DB ".character_inventory WHERE guid = %u)", soulwellGUID);

            if (res)
            {
                do
                {
                    f = res->Fetch();
                    if (!f)
                        break;

                    itm = new soulwell_item_transfer_record;
                    itm->id = f[0].GetUInt32();
                    itm->creatorGuid = f[1].GetUInt32();
                    itm->count = f[2].GetUInt32();
                    itm->duration = f[3].GetUInt32();

                    // parse charges string
                    std::string chargesStr = f[4].GetCString();
                    Tokens tk(chargesStr, ' ', MAX_ITEM_PROTO_SPELLS);
                    for (uint32 i = 0; i < MAX_ITEM_PROTO_SPELLS && i < tk.size(); i++)
                        itm->charges[i] = atol(tk[i]);

                    itm->flags = f[5].GetUInt32();
                    itm->randomPropertyId = f[6].GetUInt32();

                    // parse enchantments string
                    std::string enchStr = f[7].GetCString();
                    Tokens tke(enchStr, ' ');
                    for (uint32 i = 0; i < MAX_ENCHANTMENT_SLOT * 3 && i < tke.size(); i++)
                        itm->enchantments[i] = atol(tke[i]);

                    itm->durability = f[8].GetUInt32();

                    itm->text = f[9].GetCString();

                    itm->guid = f[10].GetUInt64();

                    items.push_back(itm);

                } while (res->NextRow());
            }

            // set items iterator to point at the beginning of list
            itemsItr = items.begin();
            transferPhase = SWT_ITEMS;
        }
        void ProceedItemsStage()
        {
            int procCount = 0;
            soulwell_item_transfer_record* it;
            ItemPosCountVec dest;
            uint32 nosp;
            while (itemsItr != items.end() && (procCount++) < SOULWELL_TRANSFER_BATCH_SIZE)
            {
                // select item from list
                it = *itemsItr;

                uint8 msg = lockedPlayer->CanStoreNewItem(itemInventory[it->guid]->bag, itemInventory[it->guid]->slot, dest, it->id, it->count, &nosp);
                if (msg == EQUIP_ERR_OK)
                {
                    Item* nitem = lockedPlayer->StoreNewItem(dest, it->id, false, it->randomPropertyId);

                    ItemPrototype const* proto = nitem->GetProto();

                    if (it->creatorGuid)
                        nitem->SetUInt64Value(ITEM_FIELD_CREATOR, MAKE_NEW_GUID(it->creatorGuid, 0, HIGHGUID_PLAYER));

                    if (it->duration)
                        nitem->SetUInt32Value(ITEM_FIELD_DURATION, it->duration);

                    for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
                        nitem->SetSpellCharges(i, it->charges[i]);

                    nitem->SetUInt32Value(ITEM_FIELD_FLAGS, it->flags);
                    if (nitem->IsSoulBound() && proto->Bonding == NO_BIND)
                        nitem->ApplyModFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_SOULBOUND, false);

                    for (int slot = 0; slot < MAX_ENCHANTMENT_SLOT; slot++)
                    {
                        if (it->enchantments[slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_ID_OFFSET] == 0)
                            continue;

                        nitem->SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_ID_OFFSET, it->enchantments[slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_ID_OFFSET]);
                        nitem->SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_DURATION_OFFSET, it->enchantments[slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_DURATION_OFFSET]);
                        nitem->SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_CHARGES_OFFSET, it->enchantments[slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_CHARGES_OFFSET]);
                    }

                    if (nitem->GetItemRandomPropertyId() < 0)
                        nitem->UpdateItemSuffixFactor();

                    if (it->durability)
                        nitem->SetUInt32Value(ITEM_FIELD_DURABILITY, it->durability);

                    if (it->text.length() > 0)
                        nitem->SetText(it->text);
                }
                else
                {
                    // TODO: log message!
                    continue;
                }

                // delete item transfer record (not list record! that would come later)
                delete it;
                // and move to next item in list
                ++itemsItr;
            }

            if (itemsItr == items.end())
            {
                LoadSpellsStage();
                items.clear();

                for (auto iter = itemInventory.begin(); iter != itemInventory.end(); ++iter)
                    delete iter->second;
                itemInventory.clear();
            }
        }

        void LoadSpellsStage()
        {
            Field *f;
            QueryResult res;

            ChatHandler(lockedPlayer).SendSysMessage("Starting spells stage...");

            // select all spells
            res = CharacterDatabase.PQuery("SELECT spell, active, disabled FROM " SOULWELL_CHAR_DB ".character_spell WHERE guid = %u", soulwellGUID);
            soulwell_spell_transfer_record* rec;
            if (res)
            {
                do
                {
                    f = res->Fetch();
                    if (!f)
                        break;

                    // there are only few fields we are fancy when looking for bags
                    rec = new soulwell_spell_transfer_record;
                    rec->spell = f[0].GetUInt32();
                    rec->active = f[1].GetBool();
                    rec->disabled = f[2].GetBool();

                    spells.push_back(rec);

                } while (res->NextRow());
            }

            // set spells iterator to point at the beginning of list
            spellsItr = spells.begin();
            transferPhase = SWT_SPELLS;
        }
        void ProceedSpellsStage()
        {
            int procCount = 0;
            soulwell_spell_transfer_record* sp;
            while (spellsItr != spells.end() && (procCount++) < SOULWELL_TRANSFER_BATCH_SIZE)
            {
                sp = *spellsItr;

                // is "learning" attribute correct?
                lockedPlayer->AddSpell(sp->spell, sp->active, true, false, sp->disabled);

                delete sp;
                ++spellsItr;
            }

            if (spellsItr == spells.end())
            {
                LoadAchievementsStage();
                spells.clear();
            }
        }

        void LoadAchievementsStage()
        {
            Field *f;
            QueryResult res;

            ChatHandler(lockedPlayer).SendSysMessage("Starting achievements stage...");

            // select all achievements
            res = CharacterDatabase.PQuery("SELECT achievement, date FROM " SOULWELL_CHAR_DB ".character_achievement WHERE guid = %u", soulwellGUID);
            soulwell_achievement_record* rec;
            if (res)
            {
                do
                {
                    f = res->Fetch();
                    if (!f)
                        break;

                    rec = new soulwell_achievement_record;
                    rec->achievementId = f[0].GetUInt32();
                    rec->date = f[1].GetUInt64();

                    achievements.push_back(rec);

                } while (res->NextRow());
            }

            // select all achievement progress
            res = CharacterDatabase.PQuery("SELECT criteria, counter, date FROM " SOULWELL_CHAR_DB ".character_achievement_progress WHERE guid = %u", soulwellGUID);
            soulwell_achievement_progress_record* prec;
            if (res)
            {
                do
                {
                    f = res->Fetch();
                    if (!f)
                        break;

                    prec = new soulwell_achievement_progress_record;
                    prec->criteriaId = f[0].GetUInt32();
                    prec->counter = f[1].GetUInt32();
                    prec->date = f[2].GetUInt64();

                    achievementProgress.push_back(prec);

                } while (res->NextRow());
            }

            achievementsItr = achievements.begin();
            achievementProgressItr = achievementProgress.begin();
            transferPhase = SWT_ACHIEVEMENTS;
        }
        void ProceedAchievementsStage()
        {
            int procCount = 0;
            soulwell_achievement_record* ar;
            while (achievementsItr != achievements.end() && (procCount++) < SOULWELL_TRANSFER_BATCH_SIZE)
            {
                ar = *achievementsItr;

                lockedPlayer->GetAchievementMgr().SetCompletedAchievement(ar->achievementId, ar->date);

                delete ar;
                ++achievementsItr;
            }

            soulwell_achievement_progress_record* apr;
            while (achievementProgressItr != achievementProgress.end() && (procCount++) < SOULWELL_TRANSFER_BATCH_SIZE)
            {
                apr = *achievementProgressItr;

                lockedPlayer->GetAchievementMgr().SetCriteriaCounter(apr->criteriaId, apr->counter, apr->date);

                delete apr;
                ++achievementProgressItr;
            }

            if (achievementsItr == achievements.end() && achievementProgressItr == achievementProgress.end())
            {
                LoadCurrencyStage();
                achievements.clear();
                achievementProgress.clear();
            }
        }

        void LoadCurrencyStage()
        {
            Field *f;
            QueryResult res;

            ChatHandler(lockedPlayer).SendSysMessage("Starting currency stage...");

            // select all currencies
            res = CharacterDatabase.PQuery("SELECT currency, total_count, week_count, season_count FROM " SOULWELL_CHAR_DB ".character_currency WHERE guid = %u", soulwellGUID);
            soulwell_currency_record* rec;
            if (res)
            {
                do
                {
                    f = res->Fetch();
                    if (!f)
                        break;

                    rec = new soulwell_currency_record;
                    rec->currency = f[0].GetUInt32();
                    rec->count = f[1].GetUInt32();
                    rec->weekcount = f[2].GetUInt32();
                    rec->seasoncount = f[3].GetUInt32();

                    currency.push_back(rec);

                } while (res->NextRow());
            }

            currencyItr = currency.begin();
            transferPhase = SWT_CURRENCY;
        }

        void ProceedCurrencyStage()
        {
            int procCount = 0;
            soulwell_currency_record* cu;
            while (currencyItr != currency.end() && (procCount++) < SOULWELL_TRANSFER_BATCH_SIZE)
            {
                cu = *currencyItr;

                lockedPlayer->SetCurrency(cu->currency, cu->count);
                lockedPlayer->SetCurrencyWeekCount(cu->currency, CURRENCY_SOURCE_ALL, cu->weekcount);
                lockedPlayer->SetCurrencySeasonCount(cu->currency, cu->seasoncount);

                delete cu;
                ++currencyItr;
            }

            if (currencyItr == currency.end())
            {
                LoadSkillsStage();
                currency.clear();
            }
        }

        void LoadSkillsStage()
        {
            Field *f;
            QueryResult res;

            ChatHandler(lockedPlayer).SendSysMessage("Starting skills stage...");

            res = CharacterDatabase.PQuery("SELECT skill, value, max FROM " SOULWELL_CHAR_DB ".character_skills WHERE guid = %u", soulwellGUID);
            soulwell_skill_record* rec;
            if (res)
            {
                do
                {
                    f = res->Fetch();
                    if (!f)
                        break;

                    rec = new soulwell_skill_record;
                    rec->skillId = f[0].GetUInt32();
                    rec->value = f[1].GetUInt32();
                    rec->max = f[2].GetUInt32();

                    skills.push_back(rec);

                } while (res->NextRow());
            }

            skillsItr = skills.begin();
            transferPhase = SWT_SKILLS;
        }
        void ProceedSkillsStage()
        {
            int procCount = 0;
            soulwell_skill_record* sr;
            uint32 step;
            while (skillsItr != skills.end() && (procCount++) < SOULWELL_TRANSFER_BATCH_SIZE)
            {
                sr = *skillsItr;

                SkillLineEntry const *pSkill = sSkillLineStore.LookupEntry(sr->skillId);
                step = 0;
                if (pSkill->categoryId == SKILL_CATEGORY_SECONDARY || pSkill->categoryId == SKILL_CATEGORY_PROFESSION)
                    step = sr->max / 75;

                lockedPlayer->SetSkill(sr->skillId, step, sr->value, sr->max);

                delete sr;
                ++skillsItr;
            }

            if (skillsItr == skills.end())
            {
                LoadReputationStage();
                skills.clear();
            }
        }

        void LoadReputationStage()
        {
            Field *f;
            QueryResult res;

            ChatHandler(lockedPlayer).SendSysMessage("Starting reputation stage...");

            res = CharacterDatabase.PQuery("SELECT faction, standing, flags FROM " SOULWELL_CHAR_DB ".character_reputation WHERE guid = %u", soulwellGUID);
            soulwell_reputation_record* rec;
            if (res)
            {
                do
                {
                    f = res->Fetch();
                    if (!f)
                        break;

                    rec = new soulwell_reputation_record;
                    rec->factionId = f[0].GetUInt32();
                    rec->standing = f[1].GetUInt32();
                    rec->flags = f[2].GetUInt32();

                    reputation.push_back(rec);

                } while (res->NextRow());
            }

            reputationItr = reputation.begin();
            transferPhase = SWT_REPUTATION;
        }
        void ProceedReputationStage()
        {
            int procCount = 0;
            soulwell_reputation_record* sr;
            while (reputationItr != reputation.end() && (procCount++) < SOULWELL_TRANSFER_BATCH_SIZE)
            {
                sr = *reputationItr;

                FactionEntry const* fe = sFactionStore.LookupEntry(sr->factionId);

                lockedPlayer->SetReputation(sr->factionId, sr->standing);

                if (sr->flags & FACTION_FLAG_VISIBLE)
                    lockedPlayer->GetReputationMgr().SetVisible(fe);

                if (sr->flags & FACTION_FLAG_INACTIVE)
                    lockedPlayer->GetReputationMgr().SetInactive(fe->reputationListID, true);

                if (sr->flags & FACTION_FLAG_AT_WAR)
                    lockedPlayer->GetReputationMgr().SetAtWar(fe->reputationListID, true);

                delete sr;
                ++reputationItr;
            }

            if (reputationItr == reputation.end())
            {
                FinishTransfer();
                reputation.clear();
            }
        }

        void FinishTransfer()
        {
            ChatHandler(lockedPlayer).SendSysMessage("Finishing transfer...");

            lockedPlayer->SaveToDB();

            lockedPlayer->SetRooted(true);

            ChatHandler(lockedPlayer).SendSysMessage("You now have to log out and then back in. Please, check if everything's fine, and report all bugs to authorities.");

            transferPhase = SWT_END;
        }

        void UpdateAI(const uint32 diff)
        {
            // no phase, or we've ended - do nothing
            if (transferPhase == SWT_NONE || transferPhase == SWT_END)
                return;

            // transfer is done in batches
            if (transferTimer > diff)
            {
                transferTimer -= diff;
                return;
            }
            transferTimer = SOULWELL_TRANSFER_TICK_TIMER;
            transferTicks++;

            Player* dest = Player::GetPlayer(*me, lockedPlayerGUID);
            if (!dest)
            {
                transferPhase = SWT_NONE;
                return;
            }
            lockedPlayer = dest;

            switch (transferPhase)
            {
                case SWT_BASIC:
                    ProceedBasicStage();
                    break;
                case SWT_ITEMS:
                    ProceedItemsStage();
                    break;
                case SWT_SPELLS:
                    ProceedSpellsStage();
                    break;
                case SWT_ACHIEVEMENTS:
                    ProceedAchievementsStage();
                    break;
                case SWT_CURRENCY:
                    ProceedCurrencyStage();
                    break;
                case SWT_SKILLS:
                    ProceedSkillsStage();
                    break;
                case SWT_REPUTATION:
                    ProceedReputationStage();
                    break;
            }

            // every N ticks save player to DB
            if ((transferTicks % SOULWELL_TRANSFER_TICK_SAVE_COUNT) == 0)
            {
                // save to DB
                lockedPlayer->SaveToDB();
            }
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
