/*
 * Copyright (C) 2005-2011 MaNGOS <http://www.getmangos.com/>
 *
 * Copyright (C) 2008-2011 Trinity <http://www.trinitycore.org/>
 *
 * Copyright (C) 2010-2011 Project SkyFire <http://www.projectskyfire.org/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "gamePCH.h"
#include "DatabaseEnv.h"
#include "Guild.h"
#include "GuildFinderMgr.h"
#include "ScriptMgr.h"
#include "Chat.h"
#include "Config.h"
#include "SocialMgr.h"
#include "Log.h"
#include "DisableMgr.h"
#include "BattlegroundMgr.h"

#define MAX_GUILD_BANK_TAB_TEXT_LEN 500
#define EMBLEM_PRICE 10 * GOLD

inline uint32 _GetGuildBankTabPrice(uint8 tabId)
{
    switch (tabId)
    {
        case 0: return 100;
        case 1: return 250;
        case 2: return 500;
        case 3: return 1000;
        case 4: return 2500;
        case 5: return 5000;
        // Slot 6 and 7 are added by item, which can be bought from vendors
        case 6: return 0;
        case 7: return 0;
        default: return 0;
    }
}

void Guild::SendCommandResult(WorldSession* session, GuildCommandType type, GuildCommandError errCode, const std::string& param)
{
    WorldPacket data(SMSG_GUILD_COMMAND_RESULT, 8 + param.size() + 1);
    data << uint32(type);
    data << param;
    data << uint32(errCode);
    session->SendPacket(&data);

    sLog->outDebug("WORLD: Sent (SMSG_GUILD_COMMAND_RESULT)");
}

void Guild::SendSaveEmblemResult(WorldSession* session, GuildEmblemError errCode)
{
    WorldPacket data(MSG_SAVE_GUILD_EMBLEM, 4);
    data << uint32(errCode);
    session->SendPacket(&data);

    sLog->outDebug("WORLD: Sent (MSG_SAVE_GUILD_EMBLEM)");
}

///////////////////////////////////////////////////////////////////////////////
// GuildAchievementMgr
GuildAchievementMgr::GuildAchievementMgr(Guild* pGuild)
{
    m_guild = pGuild;
    achievementPoints = 0;
}

GuildAchievementMgr::~GuildAchievementMgr()
{
}

void GuildAchievementMgr::SaveToDB()
{
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    if (!m_completedAchievements.empty())
    {
        for (CompletedAchievementMap::iterator itr = m_completedAchievements.begin(); itr != m_completedAchievements.end(); ++itr)
        {
            if (itr->second.changed)
            {
                trans->PAppend("DELETE FROM guild_achievement WHERE achievement = %u AND guildid = %u;",itr->first,m_guild->GetId());
                trans->PAppend("INSERT INTO guild_achievement (guildid, achievement, date) VALUES (%u, %u, %u);",m_guild->GetId(),itr->first,itr->second.date);
                itr->second.changed = false;
            }
        }
    }
    if (!m_criteriaProgress.empty())
    {
        for (CriteriaProgressMap::iterator itr = m_criteriaProgress.begin(); itr != m_criteriaProgress.end(); ++itr)
        {
            if (itr->second.changed)
            {
                trans->PAppend("DELETE FROM guild_achievement_progress WHERE criteria = %u AND guildid = %u;",itr->first,m_guild->GetId());
                trans->PAppend("INSERT INTO guild_achievement_progress (guildid, criteria, counter, date) VALUES (%u, %u, %u, %u);",m_guild->GetId(),itr->first,itr->second.counter,itr->second.date);
                itr->second.changed = false;
            }
        }
    }

    if (trans->GetSize() > 0)
        CharacterDatabase.CommitTransaction(trans);
}

void GuildAchievementMgr::LoadFromDB()
{
    QueryResult achievementResult = CharacterDatabase.PQuery("SELECT achievement,date FROM guild_achievement WHERE guildid=%u",GetGuild()->GetId());
    QueryResult criteriaResult = CharacterDatabase.PQuery("SELECT criteria,counter,date FROM guild_achievement_progress WHERE guildid=%u",GetGuild()->GetId());

    if (achievementResult)
    {
        do
        {
            Field* fields = achievementResult->Fetch();

            uint32 achievement_id = fields[0].GetUInt32();

            // don't must happen: cleanup at server startup in sAchievementMgr->LoadCompletedAchievements()
            if (!sAchievementStore.LookupEntry(achievement_id))
                continue;

            CompletedAchievementData& ca = m_completedAchievements[achievement_id];
            ca.date = time_t(fields[1].GetUInt64());
            ca.changed = false;
            
            if (AchievementEntry const* pAchievement = sAchievementStore.LookupEntry(achievement_id))
                achievementPoints += pAchievement->points;
        }
        while (achievementResult->NextRow());
    }

    if (criteriaResult)
    {
        do
        {
            Field* fields = criteriaResult->Fetch();

            uint32 id      = fields[0].GetUInt32();
            uint32 counter = fields[1].GetUInt32();
            time_t date    = time_t(fields[2].GetUInt64());

            AchievementCriteriaEntry const* criteria = sAchievementCriteriaStore.LookupEntry(id);
            if (!criteria)
            {
                // we will remove not existed criteria for all characters
                sLog->outError("Non-existing achievement criteria %u data removed from table `character_achievement_progress`.",id);
                CharacterDatabase.PExecute("DELETE FROM guild_achievement_progress WHERE criteria = %u",id);
                continue;
            }

            if (criteria->timeLimit && time_t(date + criteria->timeLimit) < time(NULL))
                continue;

            CriteriaProgress& progress = m_criteriaProgress[id];
            progress.counter = counter;
            progress.date    = date;
            progress.changed = false;
        } 
        while (criteriaResult->NextRow());
    }
}

void GuildAchievementMgr::UpdateAchievementCriteria(AchievementCriteriaTypes type, uint64 miscvalue1, uint64 miscvalue2, Unit *unit, uint32 time, Player* player)
{
    AchievementCriteriaEntryList const& achievementCriteriaList = sAchievementMgr->GetGuildAchievementCriteriaByType(type);
    for (AchievementCriteriaEntryList::const_iterator i = achievementCriteriaList.begin(); i != achievementCriteriaList.end(); ++i)
    {
        AchievementCriteriaEntry const *achievementCriteria = (*i);
        if (sDisableMgr->IsDisabledFor(DISABLE_TYPE_ACHIEVEMENT_CRITERIA, achievementCriteria->ID, NULL))
            continue;

        AchievementEntry const *achievement = sAchievementStore.LookupEntry(achievementCriteria->referredAchievement);
        if (!achievement)
            continue;

        if (!(achievement->flags & ACHIEVEMENT_FLAG_GUILD_ACHIEVEMENT))
            continue;

        // TODO: implement faction dependent achievements based on guild faction !
        if ((player && achievement->factionFlag == ACHIEVEMENT_FACTION_HORDE    && player->GetTeam() != HORDE) ||
            (player && achievement->factionFlag == ACHIEVEMENT_FACTION_ALLIANCE && player->GetTeam() != ALLIANCE))
            continue;

        // don't update already completed criteria
        if (IsCompletedCriteria(achievementCriteria,achievement) && HasAchieved(achievement->ID))
            continue;

        // Check for guild group if needed
        if (achievement->flags & ACHIEVEMENT_FLAG_NEEDS_GUILD_GROUP)
        {
            // If not valid or not in group
            if (!player || !player->GetGroup())
                continue;

            // If not in guild group, gtfo of this handler
            if (!player->GetGroup()->IsGuildGroup(player->GetGuildId()))
                continue;
        }

        // Check for universal morerequirement types and values
        bool moreReqMeets = true;
        for (uint8 i = 0; i < 3; i++)
        {
            // guild reputation
            if (player && achievementCriteria->moreRequirement[i] == ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_GUILD_REP
                && player->GetReputation(FACTION_GUILD) < achievementCriteria->moreRequirementValue[i])
            {
                moreReqMeets = false;
                continue;
            }
        }
        if (!moreReqMeets)
            continue;

        switch (type)
        {
            case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
                {
                if (!miscvalue1)
                    continue;
                if (achievementCriteria->kill_creature.creatureID != miscvalue1)
                    continue;

                AchievementCriteriaDataSet const* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria);
                if (!data)
                {
                    if (!unit || unit->GetTypeId() != TYPEID_UNIT)
                        continue;
                }
                else if (!data->Meets(player,unit))
                    continue;

                SetCriteriaProgress(achievementCriteria, miscvalue2, PROGRESS_ACCUMULATE);
                break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_REACH_GUILD_LEVEL:
                SetCriteriaProgress(achievementCriteria, m_guild->GetLevel());
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_EARN_GUILD_ACHIEVEMENT_POINTS:
                SetCriteriaProgress(achievementCriteria, achievementPoints, PROGRESS_SET);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_BUY_GUILD_TABARD:
                SetCriteriaProgress(achievementCriteria, 1);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
                if (m_completedAchievements.find(achievementCriteria->complete_achievement.linkedAchievement) != m_completedAchievements.end())
                    SetCriteriaProgress(achievementCriteria, 1);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_GUILD:
            case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILLS_GUILD:
                SetCriteriaProgress(achievementCriteria, miscvalue1, PROGRESS_ACCUMULATE);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD:
                // for testing - "kill critters" - actually it is stored in morerequirement
                if (unit && unit->GetCreatureType() == CREATURE_TYPE_CRITTER)
                    SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL:
                if (miscvalue1 && miscvalue1 != achievementCriteria->reach_skill_level.skillID)
                    continue;
                if (!miscvalue2)
                    miscvalue2 = player ? player->GetSkillValue(miscvalue1) : 0;
                SetCriteriaProgress(achievementCriteria, miscvalue2);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_CRAFT_ITEMS_GUILD:
            {
                ItemPrototype const* proto = sItemStorage.LookupEntry<ItemPrototype>((uint32) miscvalue1);
                if (!proto)
                    break;

                // Check for item requirements listed in moreRequirements
                bool fits = true;
                for (uint8 i = 0; i < 3; i++)
                {
                    if (achievementCriteria->moreRequirement[i] == 0)
                        continue;

                    // check for quality
                    if (achievementCriteria->moreRequirement[i] == ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_ITEM_QUALITY_EQUIPPED
                        && proto->Quality < achievementCriteria->moreRequirementValue[i])
                        fits = false;

                    // item level
                    if (achievementCriteria->moreRequirement[i] == ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_ITEM_LEVEL
                        && proto->ItemLevel < achievementCriteria->moreRequirementValue[i])
                        fits = false;

                    // item class (must be equal)
                    if (achievementCriteria->moreRequirement[i] == ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_ITEM_CLASS
                        && proto->Class != achievementCriteria->moreRequirementValue[i])
                        fits = false;

                    // item subclass (also must be equal)
                    if (achievementCriteria->moreRequirement[i] == ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_ITEM_SUBCLASS
                        && proto->SubClass != achievementCriteria->moreRequirementValue[i])
                        fits = false;
                }

                if (fits)
                    SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);

                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY:
                if (!miscvalue1)
                    continue;
                SetCriteriaProgress(achievementCriteria, miscvalue1, PROGRESS_ACCUMULATE);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_HK_RACE:
            {
                // HKs will count only for player kills
                if (!unit || !unit->ToPlayer())
                    continue;

                // check for morerequirement
                bool fits = true;
                for (uint8 i = 0; i < 3; i++)
                {
                    if (achievementCriteria->moreRequirement[i] == 0)
                        continue;

                    if (achievementCriteria->moreRequirement[i] == ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_PLAYER_CLASS2
                        && unit->ToPlayer()->getClass() != achievementCriteria->moreRequirementValue[i])
                        fits = false;

                    if (achievementCriteria->moreRequirement[i] == ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_PLAYER_LEVEL2
                        && unit->ToPlayer()->getLevel() < achievementCriteria->moreRequirementValue[i])
                        fits = false;
                }

                if (fits)
                    SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);

                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
                // Wierd, but true. We need to check more reqs for class and race
                if (!player || player->getLevel() < achievementCriteria->reach_level.level)
                    continue;

                // check for morerequirement
                for (uint8 i = 0; i < 3; i++)
                {
                    if (achievementCriteria->moreRequirement[i] == ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_PLAYER_CLASS
                        && player->ToPlayer()->getClass() != achievementCriteria->moreRequirementValue[i])
                        continue;
                    if (achievementCriteria->moreRequirement[i] == ACHIEVEMENT_CRITERIA_MORE_REQ_TYPE_PLAYER_RACE2
                        && player->ToPlayer()->getRace() != achievementCriteria->moreRequirementValue[i])
                        continue;
                }
                SetCriteriaProgress(achievementCriteria, player->getLevel());
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
            case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
            {
                if (!miscvalue1 || miscvalue1 != achievementCriteria->be_spell_target.spellID)
                    continue;

                // those requirements couldn't be found in the dbc
                AchievementCriteriaDataSet const* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria);
                if (!data)
                    continue;
                if (!data->Meets(player,unit))
                    continue;

                SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_GUILD_CHALLENGE_GENERIC:
            {
                // no additional checks
                SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_GUILD_CHALLENGE_SPECIFIC:
            {
                if (!miscvalue1 || miscvalue1 != achievementCriteria->guild_challenge.type)
                    continue;

                SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                break;
            }
            case ACHIEVEMENT_CRITERIA_TYPE_CATCH_FROM_POOL:
            {
                // no special conditions - called after successfull fishing "cycle" on fishing pool
                SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                break;
            }
            default:
                // Not implemented, sorry
                continue;
        }

        if (IsCompletedCriteria(achievementCriteria,achievement))
            CompletedCriteriaFor(achievement);

        // check again the completeness for SUMM and REQ COUNT achievements,
        // as they don't depend on the completed criteria but on the sum of the progress of each individual criteria
        if (achievement->flags & ACHIEVEMENT_FLAG_SUMM)
        {
            if (IsCompletedAchievement(achievement))
                CompletedAchievement(achievement);
        }

        if (AchievementEntryList const* achRefList = sAchievementMgr->GetAchievementByReferencedId(achievement->ID))
        {
            for (AchievementEntryList::const_iterator itr = achRefList->begin(); itr != achRefList->end(); ++itr)
                if (IsCompletedAchievement(*itr))
                    CompletedAchievement(*itr);
        }
    }

    // Commit immediately
    SaveToDB();
}

void GuildAchievementMgr::SendCriteriaUpdate(AchievementCriteriaEntry const* entry, CriteriaProgress const* progress)
{
    if (!entry)
        return;

    if (!progress)
        return;

    m_guild->SendGuildCriteriaUpdate(entry, progress);
}

void GuildAchievementMgr::ResetAchievementCriteria(AchievementCriteriaTypes type, uint64 miscvalue1, uint64 miscvalue2, bool evenIfCriteriaComplete)
{
    AchievementCriteriaEntryList const& achievementCriteriaList = sAchievementMgr->GetGuildAchievementCriteriaByType(type);
    for (AchievementCriteriaEntryList::const_iterator i = achievementCriteriaList.begin(); i != achievementCriteriaList.end(); ++i)
    {
        AchievementCriteriaEntry const *achievementCriteria = (*i);

        AchievementEntry const *achievement = sAchievementStore.LookupEntry(achievementCriteria->referredAchievement);
        if (!achievement)
            continue;

        if (!(achievement->flags & ACHIEVEMENT_FLAG_GUILD_ACHIEVEMENT))
            continue;

        // don't update already completed criteria if not forced or achievement already complete
        if ((IsCompletedCriteria(achievementCriteria, achievement) && !evenIfCriteriaComplete) || HasAchieved(achievement))
            continue;

        for (uint8 j = 0; j < MAX_CRITERIA_REQUIREMENTS; ++j)
            if (achievementCriteria->additionalRequirements[j].additionalRequirement_type == miscvalue1 &&
                (!achievementCriteria->additionalRequirements[j].additionalRequirement_value ||
                achievementCriteria->additionalRequirements[j].additionalRequirement_value == miscvalue2))
            {
                RemoveCriteriaProgress(achievementCriteria);
                break;
            }
    }
}

bool GuildAchievementMgr::HasAchieved(AchievementEntry const* achievement) const
{
    return m_completedAchievements.find(achievement->ID) != m_completedAchievements.end();
}

bool GuildAchievementMgr::HasAchieved(uint32 achievement) const
{
    return m_completedAchievements.find(achievement) != m_completedAchievements.end();
}

void GuildAchievementMgr::CompletedAchievement(AchievementEntry const* achievement)
{
    sLog->outDetail("GuildAchievementMgr::CompletedAchievement(%u)", achievement->ID);

    if (achievement->flags & ACHIEVEMENT_FLAG_COUNTER || HasAchieved(achievement))
        return;

    // Guilds can earn only guild achievements..obviously..
    if (!(achievement->flags & ACHIEVEMENT_FLAG_GUILD_ACHIEVEMENT))
        return;

    CompletedAchievementData& ca = m_completedAchievements[achievement->ID];
    ca.date = time(NULL);
    ca.changed = true;

    if (AchievementEntry const* pAchievement = sAchievementStore.LookupEntry(achievement->ID))
        achievementPoints += pAchievement->points;

    m_guild->AddGuildNews(GUILD_NEWS_GUILD_ACHIEVEMENT, achievement->ID);

    SendAchievementEarned(achievement);

    // don't insert for ACHIEVEMENT_FLAG_REALM_FIRST_KILL since otherwise only the first group member would reach that achievement
    // TODO: where do set this instead?
    if (!(achievement->flags & ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
        sAchievementMgr->SetRealmCompleted(achievement);

    UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT);
    UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_EARN_GUILD_ACHIEVEMENT_POINTS, achievement->points);
}

void GuildAchievementMgr::SendAchievementEarned(AchievementEntry const* achievement)
{
    if (achievement->flags & ACHIEVEMENT_FLAG_HIDDEN)
        return;

    m_guild->SendGuildAchievementEarned(achievement);
}

CriteriaProgress* GuildAchievementMgr::GetCriteriaProgress(AchievementCriteriaEntry const* entry)
{
    CriteriaProgressMap::iterator iter = m_criteriaProgress.find(entry->ID);

    if (iter == m_criteriaProgress.end())
        return NULL;

    return &(iter->second);
}

void GuildAchievementMgr::SetCriteriaProgress(AchievementCriteriaEntry const* entry, uint32 changeValue, ProgressType ptype)
{
    CriteriaProgress* progress = GetCriteriaProgress(entry);
    if (!progress)
    {
        // not create record for 0 counter but allow it for timed achievements
        // we will need to send 0 progress to client to start the timer
        if (changeValue == 0 && !entry->timeLimit)
            return;

        progress = &m_criteriaProgress[entry->ID];
        progress->counter = changeValue;
    }
    else
    {
        uint32 newValue = 0;
        switch (ptype)
        {
            case PROGRESS_SET:
                newValue = changeValue;
                break;
            case PROGRESS_ACCUMULATE:
            {
                // avoid overflow
                uint32 max_value = std::numeric_limits<uint32>::max();
                newValue = max_value - progress->counter > changeValue ? progress->counter + changeValue : max_value;
                break;
            }
            case PROGRESS_HIGHEST:
                newValue = progress->counter < changeValue ? changeValue : progress->counter;
                break;
        }

        // not update (not mark as changed) if counter will have same value
        if (progress->counter == newValue && !entry->timeLimit)
            return;

        progress->counter = newValue;
    }

    progress->changed = true;
    progress->date = time(NULL); // set the date to the latest update.

    SendCriteriaUpdate(entry, progress);
}

void GuildAchievementMgr::RemoveCriteriaProgress(const AchievementCriteriaEntry *entry)
{
    CriteriaProgressMap::iterator criteriaProgress = m_criteriaProgress.find(entry->ID);
    if (criteriaProgress == m_criteriaProgress.end())
        return;

    // TODO: implement sending criteria delete to client
    //WorldPacket data(SMSG_CRITERIA_DELETED,4);
    //data << uint32(entry->ID);
    //m_player->SendDirectMessage(&data);

    m_criteriaProgress.erase(criteriaProgress);
}

void GuildAchievementMgr::CompletedCriteriaFor(AchievementEntry const* achievement)
{
    // counter can never complete
    if (achievement->flags & ACHIEVEMENT_FLAG_COUNTER)
        return;

    // already completed and stored
    if (HasAchieved(achievement))
        return;

    if (IsCompletedAchievement(achievement))
        CompletedAchievement(achievement);
}

bool GuildAchievementMgr::IsCompletedCriteria(AchievementCriteriaEntry const* achievementCriteria, AchievementEntry const* achievement)
{
    // counter can never complete
    if (achievement->flags & ACHIEVEMENT_FLAG_COUNTER)
        return false;

    if (achievement->flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
    {
        // someone on this realm has already completed that achievement
        if (sAchievementMgr->IsRealmCompleted(achievement))
            return false;
    }

    CriteriaProgress const* progress = GetCriteriaProgress(achievementCriteria);
    if (!progress)
        return false;

    switch (achievementCriteria->requiredType)
    {
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
            return progress->counter >= achievementCriteria->kill_creature.creatureCount;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
            return progress->counter >= 1;
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL:
            return progress->counter >= achievementCriteria->reach_skill_level.skillLevel;
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE_GUILD:
            return progress->counter >= achievementCriteria->raw.count;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
            return progress->counter >= achievementCriteria->complete_daily_quest.questCount;
        case ACHIEVEMENT_CRITERIA_TYPE_CRAFT_ITEMS_GUILD:
            return progress->counter >= achievementCriteria->raw.count;
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY:
            return progress->counter >= achievementCriteria->loot_money.goldInCopper;
        case ACHIEVEMENT_CRITERIA_TYPE_HK_RACE:
            return progress->counter >= achievementCriteria->hk_race.count;
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_GUILD_LEVEL:
            return progress->counter >= achievementCriteria->raw.count;
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
            return progress->counter >= achievementCriteria->reach_level.level;
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
            return progress->counter >= achievementCriteria->be_spell_target.spellCount;
        case ACHIEVEMENT_CRITERIA_TYPE_GUILD_CHALLENGE_SPECIFIC:
        case ACHIEVEMENT_CRITERIA_TYPE_GUILD_CHALLENGE_GENERIC:
            return progress->counter >= achievementCriteria->guild_challenge.count;
        case ACHIEVEMENT_CRITERIA_TYPE_CATCH_FROM_POOL:
            return progress->counter >= achievementCriteria->raw.count;
        default:
            break;
    }
    return false;
}

bool GuildAchievementMgr::IsCompletedAchievement(AchievementEntry const* entry)
{
    // counter can never complete
    if (entry->flags & ACHIEVEMENT_FLAG_COUNTER)
        return false;

    // for achievement with referenced achievement criterias get from referenced and counter from self
    uint32 achievmentForTestId = entry->refAchievement ? entry->refAchievement : entry->ID;
    uint32 achievmentForTestCount = entry->count;

    AchievementCriteriaEntryList const* cList = sAchievementMgr->GetGuildAchievementCriteriaByAchievement(achievmentForTestId);
    if (!cList)
        return false;
    uint32 count = 0;

    // For SUMM achievements, we have to count the progress of each criteria of the achievement.
    // Oddly, the target count is NOT countained in the achievement, but in each individual criteria
    if (entry->flags & ACHIEVEMENT_FLAG_SUMM)
    {
        for (AchievementCriteriaEntryList::const_iterator itr = cList->begin(); itr != cList->end(); ++itr)
        {
            AchievementCriteriaEntry const* criteria = *itr;

            CriteriaProgress const* progress = GetCriteriaProgress(criteria);
            if (!progress)
                continue;

            count += progress->counter;

            // for counters, field4 contains the main count requirement
            if (count >= criteria->raw.count)
                return true;
        }
        return false;
    }

    // Default case - need complete all or
    bool completed_all = true;
    for (AchievementCriteriaEntryList::const_iterator itr = cList->begin(); itr != cList->end(); ++itr)
    {
        AchievementCriteriaEntry const* criteria = *itr;

        bool completed = IsCompletedCriteria(criteria,entry);

        // found an uncompleted criteria, but DONT return false yet - there might be a completed criteria with ACHIEVEMENT_CRITERIA_COMPLETE_FLAG_ALL
        if (completed)
            ++count;
        else
            completed_all = false;

        // completed as have req. count of completed criterias
        if (achievmentForTestCount > 0 && achievmentForTestCount <= count)
           return true;
    }

    // all criterias completed requirement
    if (completed_all && achievmentForTestCount == 0)
        return true;

    return false;
}

void GuildAchievementMgr::SendAllAchievementData(Player *receiver) const
{
    WorldPacket data(SMSG_GUILD_ACHIEVEMENT_DATA, m_completedAchievements.size() * (4 + 4) + 3);
    data.WriteBits(m_completedAchievements.size(), 23);
    for (CompletedAchievementMap::const_iterator itr = m_completedAchievements.begin(); itr != m_completedAchievements.end(); ++itr)
    {
        data.AppendPackedTime(itr->second.date);
        data << uint32(itr->first);
    }

    receiver->GetSession()->SendPacket(&data);
}

void Guild::SendGuildAchievementEarned(const AchievementEntry* achievement)
{
    if (!achievement)
        return;

    for(Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
    {
        if(Player* pMember = (*itr).second->FindPlayer())
        {
            WorldPacket data(SMSG_ACHIEVEMENT_EARNED);
            data.appendPackGUID(pMember->GetGUID());
            data << uint32(achievement->ID);
            data << uint32(secsToTimeBitFields(time(NULL)));
            data << uint32(0);

            pMember->GetSession()->SendPacket(&data);
        }
    }
}

void Guild::SendGuildCriteriaUpdate(AchievementCriteriaEntry const* entry, CriteriaProgress const* progress)
{
    if (!entry || !progress)
        return;

    WorldPacket data(SMSG_CRITERIA_UPDATE);

    for(Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
    {
        if(Player* pMember = (*itr).second->FindPlayer())
        {
            data.clear();

            data << uint32(entry->ID);

            // the counter is packed like a packed Guid
            data.appendPackGUID(progress->counter);

            data.append(pMember->GetPackGUID());
            data << uint32(0);              // this are some flags, 1 is for keeping the counter at 0 in client
            data << uint32(secsToTimeBitFields(progress->date));
            data << uint32(0);              // time elapsed in seconds
            data << uint32(0);              // unk

            pMember->GetSession()->SendPacket(&data);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// LogHolder
Guild::LogHolder::~LogHolder()
{
    // Cleanup
    for (GuildLog::iterator itr = m_log.begin(); itr != m_log.end(); ++itr)
        delete (*itr);
}

// Adds event loaded from database to collection
inline void Guild::LogHolder::LoadEvent(LogEntry* entry)
{
    if (m_nextGUID == uint32(GUILD_EVENT_LOG_GUID_UNDEFINED))
        m_nextGUID = entry->GetGUID();
    m_log.push_front(entry);
}

// Adds new event happened in game.
// If maximum number of events is reached, oldest event is removed from collection.
inline void Guild::LogHolder::AddEvent(SQLTransaction& trans, LogEntry* entry)
{
    // Check max records limit
    if (m_log.size() >= m_maxRecords)
    {
        LogEntry* oldEntry = m_log.front();
        delete oldEntry;
        m_log.pop_front();
    }
    // Add event to list
    m_log.push_back(entry);
    // Save to DB
    entry->SaveToDB(trans);
}

// Writes information about all events into packet.
inline void Guild::LogHolder::WritePacket(WorldPacket& data) const
{
    ByteBuffer buffer;
    data.WriteBits(m_log.size(), 23);
    for (GuildLog::const_iterator itr = m_log.begin(); itr != m_log.end(); ++itr)
        (*itr)->WritePacket(data, buffer);

    data.FlushBits();
    data.append(buffer);
}

inline uint32 Guild::LogHolder::GetNextGUID()
{
    // Next guid was not initialized. It means there are no records for this holder in DB yet.
    // Start from the beginning.
    if (m_nextGUID == uint32(GUILD_EVENT_LOG_GUID_UNDEFINED))
        m_nextGUID = 0;
    else
        m_nextGUID = (m_nextGUID + 1) % m_maxRecords;
    return m_nextGUID;
}

///////////////////////////////////////////////////////////////////////////////
// EventLogEntry
void Guild::EventLogEntry::SaveToDB(SQLTransaction& trans) const
{
    PreparedStatement* stmt = NULL;

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_EVENTLOG);
    stmt->setUInt32(0, m_guildId);
    stmt->setUInt32(1, m_guid);
    CharacterDatabase.ExecuteOrAppend(trans, stmt);

    uint8 index = 0;
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_ADD_GUILD_EVENTLOG);
    stmt->setUInt32(  index, m_guildId);
    stmt->setUInt32(++index, m_guid);
    stmt->setUInt8 (++index, uint8(m_eventType));
    stmt->setUInt32(++index, m_playerGuid1);
    stmt->setUInt32(++index, m_playerGuid2);
    stmt->setUInt8 (++index, m_newRank);
    stmt->setUInt64(++index, m_timestamp);
    CharacterDatabase.ExecuteOrAppend(trans, stmt);
}

void Guild::EventLogEntry::WritePacket(WorldPacket& data, ByteBuffer& content) const
{
    ObjectGuid guid1 = MAKE_NEW_GUID(m_playerGuid1, 0, HIGHGUID_PLAYER);
    ObjectGuid guid2 = MAKE_NEW_GUID(m_playerGuid2, 0, HIGHGUID_PLAYER);

    data.WriteBit(guid1[2]);
    data.WriteBit(guid1[4]);
    data.WriteBit(guid2[7]);
    data.WriteBit(guid2[6]);
    data.WriteBit(guid1[3]);
    data.WriteBit(guid2[3]);
    data.WriteBit(guid2[5]);
    data.WriteBit(guid1[7]);
    data.WriteBit(guid1[5]);
    data.WriteBit(guid1[0]);
    data.WriteBit(guid2[4]);
    data.WriteBit(guid2[2]);
    data.WriteBit(guid2[0]);
    data.WriteBit(guid2[1]);
    data.WriteBit(guid1[1]);
    data.WriteBit(guid1[6]);

    content.WriteByteSeq(guid2[3]);
    content.WriteByteSeq(guid2[2]);
    content.WriteByteSeq(guid2[5]);

    // New Rank
    content << uint8(m_newRank);

    content.WriteByteSeq(guid2[4]);
    content.WriteByteSeq(guid1[0]);
    content.WriteByteSeq(guid1[4]);

    // Event timestamp
    content << uint32(::time(NULL) - m_timestamp);

    content.WriteByteSeq(guid1[7]);
    content.WriteByteSeq(guid1[3]);
    content.WriteByteSeq(guid2[0]);
    content.WriteByteSeq(guid2[6]);
    content.WriteByteSeq(guid2[7]);
    content.WriteByteSeq(guid1[5]);

    // Event type
    content << uint8(m_eventType);

    content.WriteByteSeq(guid2[1]);
    content.WriteByteSeq(guid1[2]);
    content.WriteByteSeq(guid1[6]);
    content.WriteByteSeq(guid1[1]);
}

///////////////////////////////////////////////////////////////////////////////
// BankEventLogEntry
void Guild::BankEventLogEntry::SaveToDB(SQLTransaction& trans) const
{
    PreparedStatement* stmt = NULL;
    uint8 index = 0;

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_BANK_EVENTLOG);
    stmt->setUInt32(  index, m_guildId);
    stmt->setUInt32(++index, m_guid);
    stmt->setUInt8 (++index, m_bankTabId);
    CharacterDatabase.ExecuteOrAppend(trans, stmt);

    index = 0;
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_ADD_GUILD_BANK_EVENTLOG);
    stmt->setUInt32(  index, m_guildId);
    stmt->setUInt32(++index, m_guid);
    stmt->setUInt8 (++index, m_bankTabId);
    stmt->setUInt8 (++index, uint8(m_eventType));
    stmt->setUInt32(++index, m_playerGuid);
    stmt->setUInt32(++index, m_itemOrMoney);
    stmt->setUInt16(++index, m_itemStackCount);
    stmt->setUInt8 (++index, m_destTabId);
    stmt->setUInt64(++index, m_timestamp);
    CharacterDatabase.ExecuteOrAppend(trans, stmt);
}

void Guild::BankEventLogEntry::WritePacket(WorldPacket& data, ByteBuffer& content) const
{
    ObjectGuid logGuid = MAKE_NEW_GUID(m_playerGuid, 0, HIGHGUID_PLAYER);

    bool hasItem = m_eventType == GUILD_BANK_LOG_DEPOSIT_ITEM || m_eventType == GUILD_BANK_LOG_WITHDRAW_ITEM ||
        m_eventType == GUILD_BANK_LOG_MOVE_ITEM || m_eventType == GUILD_BANK_LOG_MOVE_ITEM2;

    bool itemMoved = (m_eventType == GUILD_BANK_LOG_MOVE_ITEM || m_eventType == GUILD_BANK_LOG_MOVE_ITEM2);

    bool hasStack = (hasItem && m_itemStackCount > 1) || itemMoved;

    data.WriteBit(IsMoneyEvent());
    data.WriteBit(logGuid[4]);
    data.WriteBit(logGuid[1]);
    data.WriteBit(hasItem);
    data.WriteBit(hasStack);
    data.WriteBit(logGuid[2]);
    data.WriteBit(logGuid[5]);
    data.WriteBit(logGuid[3]);
    data.WriteBit(logGuid[6]);
    data.WriteBit(logGuid[0]);
    data.WriteBit(itemMoved);
    data.WriteBit(logGuid[7]);

    content.WriteByteSeq(logGuid[6]);
    content.WriteByteSeq(logGuid[1]);
    content.WriteByteSeq(logGuid[5]);
    if (hasStack)
        content << uint32(m_itemStackCount);

    content << uint8(m_eventType);
    content.WriteByteSeq(logGuid[2]);
    content.WriteByteSeq(logGuid[4]);
    content.WriteByteSeq(logGuid[0]);
    content.WriteByteSeq(logGuid[7]);
    content.WriteByteSeq(logGuid[3]);
    if (hasItem)
        content << uint32(m_itemOrMoney);

    content << uint32(time(NULL) - m_timestamp);

    if (IsMoneyEvent())
        content << uint64(m_itemOrMoney);

    if (itemMoved)
        content << uint8(m_destTabId);
}

///////////////////////////////////////////////////////////////////////////////
// RankInfo
bool Guild::RankInfo::LoadFromDB(Field* fields)
{
    m_rankId            = fields[1].GetUInt8();
    m_name              = fields[2].GetString();
    m_rights            = fields[3].GetUInt32();
    m_bankMoneyPerDay   = fields[4].GetUInt32();
    if (m_rankId == GR_GUILDMASTER)                     // Prevent loss of leader rights
        m_rights |= GR_RIGHT_ALL;
    return true;
}

void Guild::RankInfo::SaveToDB(SQLTransaction& trans) const
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_ADD_GUILD_RANK);
    stmt->setUInt32(0, m_guildId);
    stmt->setUInt8 (1, m_rankId);
    stmt->setString(2, m_name);
    stmt->setUInt32(3, m_rights);
    CharacterDatabase.ExecuteOrAppend(trans, stmt);
}

void Guild::RankInfo::WritePacket(WorldPacket& data) const
{
    data << uint32(m_rights);
    data << uint32(m_bankMoneyPerDay);                  // In game set in gold, in packet set in bronze.
    for (uint8 i = 0; i < GUILD_BANK_MAX_TABS; ++i)
    {
        data << uint32(m_bankTabRightsAndSlots[i].rights);
        data << uint32(m_bankTabRightsAndSlots[i].slots);
    }
}

void Guild::RankInfo::SetName(const std::string& name)
{
    if (m_name == name)
        return;

    m_name = name;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SET_GUILD_RANK_NAME);
    stmt->setString(0, m_name);
    stmt->setUInt8 (1, m_rankId);
    stmt->setUInt32(2, m_guildId);
    CharacterDatabase.Execute(stmt);
}

void Guild::RankInfo::SetRights(uint32 rights)
{
    if (m_rankId == GR_GUILDMASTER)                     // Prevent loss of leader rights
        rights = GR_RIGHT_ALL;

    if (m_rights == rights)
        return;

    m_rights = rights;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SET_GUILD_RANK_RIGHTS);
    stmt->setUInt32(0, m_rights);
    stmt->setUInt8 (1, m_rankId);
    stmt->setUInt32(2, m_guildId);
    CharacterDatabase.Execute(stmt);
}

void Guild::RankInfo::SetBankMoneyPerDay(uint32 money)
{
    if (m_rankId == GR_GUILDMASTER)                     // Prevent loss of leader rights
        money = GUILD_WITHDRAW_MONEY_UNLIMITED;

    if (m_bankMoneyPerDay == money)
        return;

    m_bankMoneyPerDay = money;

    PreparedStatement* stmt = NULL;
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SET_GUILD_RANK_BANK_MONEY);
    stmt->setUInt32(0, money);
    stmt->setUInt8 (1, m_rankId);
    stmt->setUInt32(2, m_guildId);
    CharacterDatabase.Execute(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_RESET_GUILD_RANK_BANK_RESET_TIME);
    stmt->setUInt32(0, m_guildId);
    stmt->setUInt8 (1, m_rankId);
    CharacterDatabase.Execute(stmt);
}

void Guild::RankInfo::SetBankTabSlotsAndRights(uint8 tabId, GuildBankRightsAndSlots rightsAndSlots, bool saveToDB)
{
    if (m_rankId == GR_GUILDMASTER)                     // Prevent loss of leader rights
        rightsAndSlots.SetGuildMasterValues();

    if (m_bankTabRightsAndSlots[tabId].IsEqual(rightsAndSlots))
        return;

    m_bankTabRightsAndSlots[tabId] = rightsAndSlots;

    if (saveToDB)
    {
        PreparedStatement* stmt = NULL;

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_BANK_RIGHT);
        stmt->setUInt32(0, m_guildId);
        stmt->setUInt8 (1, tabId);
        stmt->setUInt8 (2, m_rankId);
        CharacterDatabase.Execute(stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_ADD_GUILD_BANK_RIGHT);
        stmt->setUInt32(0, m_guildId);
        stmt->setUInt8 (1, tabId);
        stmt->setUInt8 (2, m_rankId);
        stmt->setUInt8 (3, m_bankTabRightsAndSlots[tabId].rights);
        stmt->setUInt32(4, m_bankTabRightsAndSlots[tabId].slots);
        CharacterDatabase.Execute(stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_RESET_GUILD_RANK_BANK_TIME0 + tabId);
        stmt->setUInt32(0, m_guildId);
        stmt->setUInt8 (1, m_rankId);
        CharacterDatabase.Execute(stmt);
    }
}

///////////////////////////////////////////////////////////////////////////////
// BankTab
bool Guild::BankTab::LoadFromDB(Field* fields)
{
    m_name = fields[2].GetString();
    m_icon = fields[3].GetString();
    m_text = fields[4].GetString();
    return true;
}

bool Guild::BankTab::LoadItemFromDB(Field* fields)
{
    uint8 slotId = fields[12].GetUInt8();
    uint32 itemGuid = fields[13].GetUInt32();
    uint32 itemEntry = fields[14].GetUInt32();
    if (slotId >= GUILD_BANK_MAX_SLOTS)
    {
        sLog->outError("Invalid slot for item (GUID: %u, id: %u) in guild bank, skipped.", itemGuid, itemEntry);
        return false;
    }

    ItemPrototype const* proto = sObjectMgr->GetItemPrototype(itemEntry);
    if (!proto)
    {
        sLog->outError("Unknown item (GUID: %u, id: %u) in guild bank, skipped.", itemGuid, itemEntry);
        return false;
    }

    Item *pItem = NewItemOrBag(proto);
    if (!pItem->LoadFromDB(itemGuid, 0, fields, itemEntry))
    {
        sLog->outError("Item (GUID %u, id: %u) not found in item_instance, deleting from guild bank!", itemGuid, itemEntry);

        PreparedStatement *stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_NONEXISTENT_GUILD_BANK_ITEM);
        stmt->setUInt32(0, m_guildId);
        stmt->setUInt8 (1, m_tabId);
        stmt->setUInt8 (2, slotId);
        CharacterDatabase.Execute(stmt);

        delete pItem;
        return false;
    }

    pItem->AddToWorld();
    m_items[slotId] = pItem;
    return true;
}

// Deletes contents of the tab from the world (and from DB if necessary)
void Guild::BankTab::Delete(SQLTransaction& trans, bool removeItemsFromDB)
{
    for (uint8 slotId = 0; slotId < GUILD_BANK_MAX_SLOTS; ++slotId)
        if (Item* pItem = m_items[slotId])
        {
            pItem->RemoveFromWorld();
            if (removeItemsFromDB)
                pItem->DeleteFromDB(trans);
            delete pItem;
            pItem = NULL;
        }
}

inline void Guild::BankTab::WritePacket(WorldPacket& data) const
{
    data << uint8(GUILD_BANK_MAX_SLOTS);
    for (uint8 slotId = 0; slotId < GUILD_BANK_MAX_SLOTS; ++slotId)
        WriteSlotPacket(data, slotId);
}

// Writes information about contents of specified slot into packet.
void Guild::BankTab::WriteSlotPacket(WorldPacket& data, uint8 slotId) const
{
    Item *pItem = GetItem(slotId);
    uint32 itemEntry = pItem ? pItem->GetEntry() : 0;

    data << uint8(slotId);
    data << uint32(itemEntry);
    if (itemEntry)
    {
        data << uint32(0);                                  // 3.3.0 (0x00018020, 0x00018000)
        data << uint32(pItem->GetItemRandomPropertyId());   // Random item property id

        if (pItem->GetItemRandomPropertyId())
            data << uint32(pItem->GetItemSuffixFactor());   // SuffixFactor

        data << uint32(pItem->GetCount());                  // ITEM_FIELD_STACK_COUNT
        data << uint32(0);
        data << uint8(abs(pItem->GetSpellCharges()));       // Spell charges

        // Something new in cataclysm. MIGHT be related to reforging.
        // No research yet done...
        data << uint32(0);
        data << uint32(0); 

        uint8 enchCount = 0;
        size_t enchCountPos = data.wpos();

        data << uint8(enchCount);                           // Number of enchantments
        for (uint32 i = PERM_ENCHANTMENT_SLOT; i < MAX_ENCHANTMENT_SLOT; ++i)
            if (uint32 enchId = pItem->GetEnchantmentId(EnchantmentSlot(i)))
            {
                data << uint8(i);
                data << uint32(enchId);
                ++enchCount;
            }
        data.put<uint8>(enchCountPos, enchCount);
    }
}

void Guild::BankTab::SetInfo(const std::string& name, const std::string& icon)
{
    if (m_name == name && m_icon == icon)
        return;

    m_name = name;
    m_icon = icon;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SET_GUILD_BANK_TAB_INFO);
    stmt->setString(0, m_name);
    stmt->setString(1, m_icon);
    stmt->setUInt32(2, m_guildId);
    stmt->setUInt8 (3, m_tabId);
    CharacterDatabase.Execute(stmt);
}

void Guild::BankTab::SetText(const std::string& text)
{
    if (m_text == text)
        return;

    m_text = text;
    utf8truncate(m_text, MAX_GUILD_BANK_TAB_TEXT_LEN);          // DB and client size limitation

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SET_GUILD_BANK_TAB_TEXT);
    stmt->setString(0, m_text);
    stmt->setUInt32(1, m_guildId);
    stmt->setUInt8 (2, m_tabId);
    CharacterDatabase.Execute(stmt);
}

// Sets/removes contents of specified slot.
// If pItem == NULL contents are removed.
bool Guild::BankTab::SetItem(SQLTransaction& trans, uint8 slotId, Item* pItem)
{
    if (slotId >= GUILD_BANK_MAX_SLOTS)
        return false;

    m_items[slotId] = pItem;

    PreparedStatement* stmt = NULL;

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_BANK_ITEM);
    stmt->setUInt32(0, m_guildId);
    stmt->setUInt8 (1, m_tabId);
    stmt->setUInt8 (2, slotId);
    CharacterDatabase.ExecuteOrAppend(trans, stmt);

    if (pItem)
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_ADD_GUILD_BANK_ITEM);
        stmt->setUInt32(0, m_guildId);
        stmt->setUInt8 (1, m_tabId);
        stmt->setUInt8 (2, slotId);
        stmt->setUInt32(3, pItem->GetGUIDLow());
        CharacterDatabase.ExecuteOrAppend(trans, stmt);

        pItem->SetUInt64Value(ITEM_FIELD_CONTAINED, 0);
        pItem->SetUInt64Value(ITEM_FIELD_OWNER, 0);
        pItem->FSetState(ITEM_NEW);
        pItem->SaveToDB(trans);                                 // Not in inventory and can be saved standalone
    }
    return true;
}

void Guild::BankTab::SendText(const Guild* pGuild, WorldSession* session) const
{
    WorldPacket data(SMSG_QUERY_GUILD_BANK_TEXT, 6 + m_text.size());
    data.WriteBits(m_text.length(), 14);
    data << uint32(m_tabId);
    data.WriteString(m_text);

    if (session)
        session->SendPacket(&data);
    else
        pGuild->BroadcastPacket(&data);
}

///////////////////////////////////////////////////////////////////////////////
// Member
void Guild::Member::SetStats(Player* player)
{
    m_name      = player->GetName();
    m_level     = player->getLevel();
    m_class     = player->getClass();
    m_zoneId    = player->GetZoneId();
    m_accountId = player->GetSession()->GetAccountId();
    m_achievementPoints = player->GetAchievementMgr().GetAchievementPoints();

    // propagate profession spells to guild info
    uint32 val = 0;
    if (player->GetUInt32Value(PLAYER_PROFESSION_SKILL_LINE_1))
    {
        val = player->GetSkillValue(player->GetUInt32Value(PLAYER_PROFESSION_SKILL_LINE_1));
        SetProfessionData(player->GetUInt32Value(PLAYER_PROFESSION_SKILL_LINE_1), val, 0);
    }
    if (player->GetUInt32Value(PLAYER_PROFESSION_SKILL_LINE_1 + 1))
    {
        val = player->GetSkillValue(player->GetUInt32Value(PLAYER_PROFESSION_SKILL_LINE_1 + 1));
        SetProfessionData(player->GetUInt32Value(PLAYER_PROFESSION_SKILL_LINE_1 + 1), val, 0);
    }
}

void Guild::UpdateMemberStats(Player* player)
{
    if (!player)
        return;

    if (Member* member = GetMember(player->GetGUID()))
        member->SetStats(player);
}

void Guild::Member::SetStats(const std::string& name, uint8 level, uint8 _class, uint32 zoneId, uint32 accountId, uint32 achievementPoints)
{
    m_name      = name;
    m_level     = level;
    m_class     = _class;
    m_zoneId    = zoneId;
    m_accountId = accountId;
    m_achievementPoints = achievementPoints;
}

void Guild::Member::SetPublicNote(const std::string& publicNote)
{
    if (m_publicNote == publicNote)
        return;

    m_publicNote = publicNote;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SET_GUILD_MEMBER_PNOTE);
    stmt->setString(0, publicNote);
    stmt->setUInt32(1, GUID_LOPART(m_guid));
    CharacterDatabase.Execute(stmt);
}

void Guild::Member::SetOfficerNote(const std::string& officerNote)
{
    if (m_officerNote == officerNote)
        return;

    m_officerNote = officerNote;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SET_GUILD_MEMBER_OFFNOTE);
    stmt->setString(0, officerNote);
    stmt->setUInt32(1, GUID_LOPART(m_guid));
    CharacterDatabase.Execute(stmt);
}

void Guild::Member::ChangeRank(uint8 newRank)
{
    m_rankId = newRank;

    // Update rank information in player's field, if he is online.
    if (Player *player = FindPlayer())
        player->SetRank(newRank);

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SET_GUILD_MEMBER_RANK);
    stmt->setUInt8 (0, newRank);
    stmt->setUInt32(1, GUID_LOPART(m_guid));
    CharacterDatabase.Execute(stmt);
}

void Guild::SwitchRank(uint32 oldID, uint32 newID)
{
    if (oldID == GR_GUILDMASTER || newID == GR_GUILDMASTER)
        return;
    
    if (oldID == newID)
        return;
    
    if (oldID > GUILD_RANKS_MIN_COUNT || newID > GUILD_RANKS_MIN_COUNT)
        return;
    
    RankInfo old = m_ranks[oldID];//swap rank position
    m_ranks[oldID] = m_ranks[newID];
    m_ranks[newID] = old;
    m_ranks[oldID].SetId(oldID);//we need to swap their ids too
    m_ranks[newID].SetId(newID);
   
    CharacterDatabase.PExecute("UPDATE guild_rank SET rid = 11 WHERE rid = '%u' AND guildid='%u'", oldID, m_id);
    CharacterDatabase.PExecute("UPDATE guild_rank SET rid = '%u' WHERE rid = '%u' AND guildid='%u'", oldID, newID, m_id);
    CharacterDatabase.PExecute("UPDATE guild_rank SET rid = '%u' WHERE rid = 11 AND guildid='%u'", newID, m_id);
    
    CharacterDatabase.PExecute("UPDATE guild_bank_right SET rid = 11 WHERE rid = '%u' AND guildid='%u'", oldID, m_id);
    CharacterDatabase.PExecute("UPDATE guild_bank_right SET rid = '%u' WHERE rid = '%u' AND guildid='%u'", oldID, newID, m_id);
    CharacterDatabase.PExecute("UPDATE guild_bank_right SET rid = '%u' WHERE rid = 11 AND guildid='%u'", newID, m_id);
}

void Guild::Member::SaveToDB(SQLTransaction& trans) const
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_ADD_GUILD_MEMBER);
    stmt->setUInt32(0, m_guildId);
    stmt->setUInt32(1, GUID_LOPART(m_guid));
    stmt->setUInt8 (2, m_rankId);
    stmt->setString(3, m_publicNote);
    stmt->setString(4, m_officerNote);
    CharacterDatabase.ExecuteOrAppend(trans, stmt);
}

void Guild::UpdateMemberInDB(uint64 guid)
{
    if (Member* pMember = GetMember(guid))
    {
        CharacterDatabase.PExecute("UPDATE guild_member SET achievementPoints = %u, skillId1 = %u, skillValue1 = %u, skillId2 = %u, skillValue2 = %u, activityWeek = %u, activityTotal = %u"
                                   " WHERE guildid = %u AND guid = %u;",
                                   pMember->GetAchievementPoints(), pMember->professions[0].skillId, pMember->professions[0].skillValue, pMember->professions[1].skillId, pMember->professions[1].skillValue,
                                   pMember->m_activityWeek, pMember->m_activityTotal,
                                   m_id, GUID_LOPART(guid));
    }
}

// Loads member's data from database.
// If member has broken fields (level, class) returns false. 
// In this case member has to be removed from guild.
bool Guild::Member::LoadFromDB(Field* fields)
{
    /*
    //          0        1        2     3      4        5                   6
        "SELECT guildid, gm.guid, rank, pnote, offnote, BankResetTimeMoney, BankRemMoney,"
    //   7                  8                 9                  10                11                 12
        "BankResetTimeTab0, BankRemSlotsTab0, BankResetTimeTab1, BankRemSlotsTab1, BankResetTimeTab2, BankRemSlotsTab2,"
    //   13                 14                15                 16                17                 18
        "BankResetTimeTab3, BankRemSlotsTab3, BankResetTimeTab4, BankRemSlotsTab4, BankResetTimeTab5, BankRemSlotsTab5,"
    //   19                 20                21                 22
        "BankResetTimeTab6, BankRemSlotsTab6, BankResetTimeTab7, BankRemSlotsTab7,"
    //   23      24       25       26      27         28
        "c.name, c.level, c.class, c.zone, c.account, c.logout_time, "
    //   29                 30        31           32        33           34            35
        "achievementPoints, skillId1, skillValue1, skillId2, skillValue2, activityWeek, activityTotal "
    */

    m_publicNote    = fields[3].GetString();
    m_officerNote   = fields[4].GetString();
    m_bankRemaining[GUILD_BANK_MAX_TABS].resetTime  = fields[5].GetUInt32();
    m_bankRemaining[GUILD_BANK_MAX_TABS].value      = fields[6].GetUInt32();
    for (uint8 i = 0; i < GUILD_BANK_MAX_TABS; ++i)
    {
        m_bankRemaining[i].resetTime                = fields[7 + i * 2].GetUInt32();
        m_bankRemaining[i].value                    = fields[8 + i * 2].GetUInt32();
    }

    SetStats(fields[23].GetString(),
             fields[24].GetUInt8(),
             fields[25].GetUInt8(),
             fields[26].GetUInt16(),
             fields[27].GetUInt32(),
             fields[29].GetUInt32());
    m_logoutTime = fields[28].GetUInt64();

    professions[0].skillId = fields[30].GetUInt32();
    professions[0].skillValue = fields[31].GetUInt32();
    professions[0].title = 0;
    professions[1].skillId = fields[32].GetUInt32();
    professions[1].skillValue = fields[33].GetUInt32();
    professions[1].title = 0;

    m_activityWeek = fields[34].GetUInt32();
    m_activityTotal = fields[35].GetUInt32();

    if (!CheckStats())
        return false;

    if (!m_zoneId)
    {
        sLog->outError("Player (GUID: %u) has broken zone-data", GUID_LOPART(m_guid));
        m_zoneId = Player::GetZoneIdFromDB(m_guid);
    }
    return true;
}

// Validate player fields. Returns false if corrupted fields are found.
bool Guild::Member::CheckStats() const
{
    if (m_level < 1)
    {
        sLog->outError("Player (GUID: %u) has a broken data in field `characters`.`level`, deleting him from guild!", GUID_LOPART(m_guid));
        return false;
    }
    if (m_class < CLASS_WARRIOR || m_class >= MAX_CLASSES)
    {
        sLog->outError("Player (GUID: %u) has a broken data in field `characters`.`class`, deleting him from guild!", GUID_LOPART(m_guid));
        return false;
    }
    return true;
}

// Decreases amount of money/slots left for today.
// If (tabId == GUILD_BANK_MAX_TABS) decrease money amount.
// Otherwise decrease remaining items amount for specified tab.
void Guild::Member::DecreaseBankRemainingValue(SQLTransaction& trans, uint8 tabId, uint32 amount)
{
    m_bankRemaining[tabId].value -= amount;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(
        tabId == GUILD_BANK_MAX_TABS ? 
        CHAR_SET_GUILD_MEMBER_BANK_REM_MONEY : 
        CHAR_SET_GUILD_MEMBER_BANK_REM_SLOTS0 + tabId);
    stmt->setUInt32(0, m_bankRemaining[tabId].value);
    stmt->setUInt32(1, m_guildId);
    stmt->setUInt32(2, GUID_LOPART(m_guid));
    CharacterDatabase.ExecuteOrAppend(trans, stmt);
}

// Get amount of money/slots left for today.
// If (tabId == GUILD_BANK_MAX_TABS) return money amount.
// Otherwise return remaining items amount for specified tab.
// If reset time was more than 24 hours ago, renew reset time and reset amount to maximum value.
uint32 Guild::Member::GetBankRemainingValue(uint8 tabId, const Guild* pGuild) const
{
    // Guild master has unlimited amount.
    if (IsRank(GR_GUILDMASTER))
        return tabId == GUILD_BANK_MAX_TABS ? GUILD_WITHDRAW_MONEY_UNLIMITED : GUILD_WITHDRAW_SLOT_UNLIMITED;

    // Check rights for non-money tab.
    if (tabId != GUILD_BANK_MAX_TABS)
        if ((pGuild->_GetRankBankTabRights(m_rankId, tabId) & GUILD_BANK_RIGHT_VIEW_TAB) != GUILD_BANK_RIGHT_VIEW_TAB)
            return 0;

    uint32 curTime = uint32(::time(NULL) / MINUTE); // minutes
    if (curTime > m_bankRemaining[tabId].resetTime + 24 * HOUR / MINUTE)
    {
        RemainingValue& rv = const_cast <RemainingValue&> (m_bankRemaining[tabId]);
        rv.resetTime = curTime;
        rv.value = tabId == GUILD_BANK_MAX_TABS ?
            pGuild->_GetRankBankMoneyPerDay(m_rankId) :
            pGuild->_GetRankBankTabSlotsPerDay(m_rankId, tabId);

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(
            tabId == GUILD_BANK_MAX_TABS ?
            CHAR_SET_GUILD_MEMBER_BANK_TIME_MONEY : 
            CHAR_SET_GUILD_MEMBER_BANK_TIME_REM_SLOTS0 + tabId);
        stmt->setUInt32(0, m_bankRemaining[tabId].resetTime);
        stmt->setUInt32(1, m_bankRemaining[tabId].value);
        stmt->setUInt32(2, m_guildId);
        stmt->setUInt32(3, GUID_LOPART(m_guid));
        CharacterDatabase.Execute(stmt);
    }
    return m_bankRemaining[tabId].value;
}

inline void Guild::Member::ResetTabTimes()
{
    for (uint8 tabId = 0; tabId < GUILD_BANK_MAX_TABS; ++tabId)
        m_bankRemaining[tabId].resetTime = 0;
}

inline void Guild::Member::ResetMoneyTime()
{
    m_bankRemaining[GUILD_BANK_MAX_TABS].resetTime = 0;
}

///////////////////////////////////////////////////////////////////////////////
// EmblemInfo
void EmblemInfo::LoadFromDB(Field* fields)
{
    m_style             = fields[3].GetUInt8();
    m_color             = fields[4].GetUInt8();
    m_borderStyle       = fields[5].GetUInt8();
    m_borderColor       = fields[6].GetUInt8();
    m_backgroundColor   = fields[7].GetUInt8();
}

void EmblemInfo::WritePacket(WorldPacket& data) const
{
    data << uint32(m_style);
    data << uint32(m_color);
    data << uint32(m_borderStyle);
    data << uint32(m_borderColor);
    data << uint32(m_backgroundColor);
}

void EmblemInfo::SaveToDB(uint32 guildId) const
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SET_GUILD_EMBLEM_INFO);
    stmt->setUInt32(0, m_style);
    stmt->setUInt32(1, m_color);
    stmt->setUInt32(2, m_borderStyle);
    stmt->setUInt32(3, m_borderColor);
    stmt->setUInt32(4, m_backgroundColor);
    stmt->setUInt32(5, guildId);
    CharacterDatabase.Execute(stmt);
}

///////////////////////////////////////////////////////////////////////////////
// MoveItemData
bool Guild::MoveItemData::CheckItem(uint32& splitedAmount)
{
    ASSERT(m_pItem);
    if (splitedAmount > m_pItem->GetCount())
        return false;
    if (splitedAmount == m_pItem->GetCount())
        splitedAmount = 0;
    return true;
}

uint8 Guild::MoveItemData::CanStore(Item* pItem, bool swap, bool sendError)
{
    m_vec.clear();
    uint8 msg = _CanStore(pItem, swap);
    if (sendError && msg != EQUIP_ERR_OK)
        m_pPlayer->SendEquipError(msg, pItem);
    return (msg == EQUIP_ERR_OK);
}

bool Guild::MoveItemData::CloneItem(uint32 count)
{
    ASSERT(m_pItem);
    m_pClonedItem = m_pItem->CloneItem(count);
    if (!m_pClonedItem)
    {
        m_pPlayer->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, m_pItem);
        return false;
    }
    return true;
}

void Guild::MoveItemData::LogAction(MoveItemData* pFrom) const
{
    ASSERT(pFrom->GetItem());

    sScriptMgr->OnGuildItemMove(m_pGuild, m_pPlayer, pFrom->GetItem(), 
        pFrom->IsBank(), pFrom->GetContainer(), pFrom->GetSlotId(),
        IsBank(), GetContainer(), GetSlotId());
}

inline void Guild::MoveItemData::CopySlots(SlotIds& ids) const
{
    for (ItemPosCountVec::const_iterator itr = m_vec.begin(); itr != m_vec.end(); ++itr)
        ids.insert(uint8(itr->pos));
}

///////////////////////////////////////////////////////////////////////////////
// PlayerMoveItemData
bool Guild::PlayerMoveItemData::InitItem()
{
    m_pItem = m_pPlayer->GetItemByPos(m_container, m_slotId);
    if (m_pItem)
    {
        // Anti-WPE protection. Do not move non-empty bags to bank.
        if (m_pItem->IsBag() && !((Bag*)m_pItem)->IsEmpty())
        {
            m_pPlayer->SendEquipError(EQUIP_ERR_CAN_ONLY_DO_WITH_EMPTY_BAGS, m_pItem);
            m_pItem = NULL;
        }
        // Bound items cannot be put into bank.
        else if (!m_pItem->CanBeTraded())
        {
            m_pPlayer->SendEquipError(EQUIP_ERR_ITEMS_CANT_BE_SWAPPED, m_pItem);
            m_pItem = NULL;
        }
    }
    return (m_pItem != NULL);
}

void Guild::PlayerMoveItemData::RemoveItem(SQLTransaction& trans, MoveItemData* /*pOther*/, uint32 splitedAmount)
{
    if (splitedAmount)
    {
        m_pItem->SetCount(m_pItem->GetCount() - splitedAmount);
        m_pItem->SetState(ITEM_CHANGED, m_pPlayer);
        m_pPlayer->SaveInventoryAndGoldToDB(trans);
    }
    else
    {
        m_pPlayer->MoveItemFromInventory(m_container, m_slotId, true);
        m_pItem->DeleteFromInventoryDB(trans);
        m_pItem = NULL;
    }
}

Item* Guild::PlayerMoveItemData::StoreItem(SQLTransaction& trans, Item* pItem)
{
    ASSERT(pItem);
    m_pPlayer->MoveItemToInventory(m_vec, pItem, true);
    m_pPlayer->SaveInventoryAndGoldToDB(trans);
    return pItem;
}

void Guild::PlayerMoveItemData::LogBankEvent(SQLTransaction& trans, MoveItemData* pFrom, uint32 count) const
{
    ASSERT(pFrom);
    // Bank -> Char
    m_pGuild->_LogBankEvent(trans, GUILD_BANK_LOG_WITHDRAW_ITEM, pFrom->GetContainer(), m_pPlayer->GetGUIDLow(), 
        pFrom->GetItem()->GetEntry(), count);

    sLog->outChar("IP:(%s) account:(%u) character:(%s) action:(%s) %s:(name:(%s) entry:(%u) count:(%u)) guild_id:(%u)",
                 m_pPlayer->GetSession()->GetRemoteAddress().c_str(),
                 m_pPlayer->GetSession()->GetAccountId(),
                 m_pPlayer->GetName(),
                 "guildbank withdraw",
                   "item",
                   pFrom->GetItem()->GetProto()->Name1,
                   pFrom->GetItem()->GetEntry(),
                   pFrom->GetItem()->GetCount(),
                 m_pGuild->GetId());
}

inline uint8 Guild::PlayerMoveItemData::_CanStore(Item* pItem, bool swap)
{
    return m_pPlayer->CanStoreItem(m_container, m_slotId, m_vec, pItem, swap);
}

///////////////////////////////////////////////////////////////////////////////
// BankMoveItemData
bool Guild::BankMoveItemData::InitItem()
{
    m_pItem = m_pGuild->_GetItem(m_container, m_slotId);
    return (m_pItem != NULL);
}

bool Guild::BankMoveItemData::HasStoreRights(MoveItemData* pOther) const
{
    ASSERT(pOther);
    // Do not check rights if item is being swapped within the same bank tab
    if (pOther->IsBank() && pOther->GetContainer() == m_container)
        return true;
    return m_pGuild->_MemberHasTabRights(m_pPlayer->GetGUID(), m_container, GUILD_BANK_RIGHT_DEPOSIT_ITEM);
}

bool Guild::BankMoveItemData::HasWithdrawRights(MoveItemData* pOther) const
{
    ASSERT(pOther);
    // Do not check rights if item is being swapped within the same bank tab
    if (pOther->IsBank() && pOther->GetContainer() == m_container)
        return true;
    return (m_pGuild->_GetMemberRemainingSlots(m_pPlayer->GetGUID(), m_container) != 0);
}

void Guild::BankMoveItemData::RemoveItem(SQLTransaction& trans, MoveItemData* pOther, uint32 splitedAmount)
{
    ASSERT(m_pItem);
    if (splitedAmount)
    {
        m_pItem->SetCount(m_pItem->GetCount() - splitedAmount);
        m_pItem->FSetState(ITEM_CHANGED);
        m_pItem->SaveToDB(trans);
    }
    else
    {
        m_pGuild->_RemoveItem(trans, m_container, m_slotId);
        m_pItem = NULL;
    }
    // Decrease amount of player's remaining items (if item is moved to different tab or to player)
    if (!pOther->IsBank() || pOther->GetContainer() != m_container)
        m_pGuild->_DecreaseMemberRemainingSlots(trans, m_pPlayer->GetGUID(), m_container);
}

Item* Guild::BankMoveItemData::StoreItem(SQLTransaction& trans, Item* pItem)
{
    if (!pItem)
        return NULL;

    BankTab* pTab = m_pGuild->GetBankTab(m_container);
    if (!pTab)
        return NULL;

    Item* pLastItem = pItem;
    for (ItemPosCountVec::const_iterator itr = m_vec.begin(); itr != m_vec.end(); )
    {
        ItemPosCount pos(*itr);
        ++itr;

        sLog->outDebug("GUILD STORAGE: StoreItem tab = %u, slot = %u, item = %u, count = %u", 
            m_container, m_slotId, pItem->GetEntry(), pItem->GetCount());
        pLastItem = _StoreItem(trans, pTab, pItem, pos, itr != m_vec.end());
    }
    return pLastItem;
}

void Guild::BankMoveItemData::LogBankEvent(SQLTransaction& trans, MoveItemData* pFrom, uint32 count) const
{
    ASSERT(pFrom->GetItem());
    if (pFrom->IsBank())
        // Bank -> Bank
        m_pGuild->_LogBankEvent(trans, GUILD_BANK_LOG_MOVE_ITEM, pFrom->GetContainer(), m_pPlayer->GetGUIDLow(), 
            pFrom->GetItem()->GetEntry(), count, m_container);
    else
    {
        // Char -> Bank
        m_pGuild->_LogBankEvent(trans, GUILD_BANK_LOG_DEPOSIT_ITEM, m_container, m_pPlayer->GetGUIDLow(), 
            pFrom->GetItem()->GetEntry(), count);

        sLog->outChar("IP:(%s) account:(%u) character:(%s) action:(%s) %s:(name:(%s) entry:(%u) count:(%u)) guild_id:(%u)",
                     m_pPlayer->GetSession()->GetRemoteAddress().c_str(),
                     m_pPlayer->GetSession()->GetAccountId(),
                     m_pPlayer->GetName(),
                     "guildbank deposit",
                       "item",
                       pFrom->GetItem()->GetProto()->Name1,
                       pFrom->GetItem()->GetEntry(),
                       pFrom->GetItem()->GetCount(),
                     m_pGuild->GetId());
    }
}

void Guild::BankMoveItemData::LogAction(MoveItemData* pFrom) const
{
    MoveItemData::LogAction(pFrom);
    if (!pFrom->IsBank() && sWorld->getBoolConfig(CONFIG_GM_LOG_TRADE) && m_pPlayer->GetSession()->GetSecurity() > SEC_PLAYER)       // TODO: move to scripts
        sLog->outCommand(m_pPlayer->GetSession()->GetAccountId(),
            "GM %s (Account: %u) deposit item: %s (Entry: %d Count: %u) to guild bank (Guild ID: %u)",
            m_pPlayer->GetName(), m_pPlayer->GetSession()->GetAccountId(),
            pFrom->GetItem()->GetProto()->Name1, pFrom->GetItem()->GetEntry(), pFrom->GetItem()->GetCount(),
            m_pGuild->GetId());
}

Item* Guild::BankMoveItemData::_StoreItem(SQLTransaction& trans, BankTab* pTab, Item *pItem, ItemPosCount& pos, bool clone) const
{
    uint8 slotId = uint8(pos.pos);
    uint32 count = pos.count;
    if (Item* pItemDest = pTab->GetItem(slotId))
    {
        pItemDest->SetCount(pItemDest->GetCount() + count);
        pItemDest->FSetState(ITEM_CHANGED);
        pItemDest->SaveToDB(trans);
        if (!clone)
        {
            pItem->RemoveFromWorld();
            pItem->DeleteFromDB(trans);
            delete pItem;
        }
        return pItemDest;
    }

    if (clone)
        pItem = pItem->CloneItem(count);
    else
        pItem->SetCount(count);

    if (pItem && pTab->SetItem(trans, slotId, pItem))
        return pItem;

    return NULL;
}

// Tries to reserve space for source item.
// If item in destination slot exists it must be the item of the same entry
// and stack must have enough space to take at least one item.
// Returns false if destination item specified and it cannot be used to reserve space.
bool Guild::BankMoveItemData::_ReserveSpace(uint8 slotId, Item* pItem, Item* pItemDest, uint32& count)
{
    uint32 requiredSpace = pItem->GetMaxStackCount();
    if (pItemDest)
    {
        // Make sure source and destination items match and destination item has space for more stacks.
        if (pItemDest->GetEntry() != pItem->GetEntry() || pItemDest->GetCount() >= pItem->GetMaxStackCount())
            return false;
        requiredSpace -= pItemDest->GetCount();
    }
    // Let's not be greedy, reserve only required space
    requiredSpace = std::min(requiredSpace, count);

    // Reserve space
    ItemPosCount pos(slotId, requiredSpace);
    if (!pos.isContainedIn(m_vec))
    {
        m_vec.push_back(pos);
        count -= requiredSpace;
    }
    return true;
}

void Guild::BankMoveItemData::_CanStoreItemInTab(Item* pItem, uint8 skipSlotId, bool merge, uint32& count)
{
    for (uint8 slotId = 0; (slotId < GUILD_BANK_MAX_SLOTS) && (count > 0); ++slotId)
    {
        // Skip slot already processed in _CanStore (when destination slot was specified)
        if (slotId == skipSlotId)
            continue;

        Item* pItemDest = m_pGuild->_GetItem(m_container, slotId);
        if (pItemDest == pItem)
            pItemDest = NULL;

        // If merge skip empty, if not merge skip non-empty
        if ((pItemDest != NULL) != merge)
            continue;

        _ReserveSpace(slotId, pItem, pItemDest, count);
    }
}

uint8 Guild::BankMoveItemData::_CanStore(Item* pItem, bool swap)
{
    sLog->outDebug("GUILD STORAGE: CanStore() tab = %u, slot = %u, item = %u, count = %u", 
        m_container, m_slotId, pItem->GetEntry(), pItem->GetCount());

    uint32 count = pItem->GetCount();
    // Soulbound items cannot be moved
    if (pItem->IsSoulBound())
        return EQUIP_ERR_CANT_DROP_SOULBOUND;

    // Make sure destination bank tab exists
    if (m_container >= m_pGuild->_GetPurchasedTabsSize())
        return EQUIP_ERR_ITEM_DOESNT_GO_INTO_BAG;

    // Slot explicitely specified. Check it.
    if (m_slotId != NULL_SLOT)
    {
        Item* pItemDest = m_pGuild->_GetItem(m_container, m_slotId);
        // Ignore swapped item (this slot will be empty after move)
        if ((pItemDest == pItem) || swap)
            pItemDest = NULL;

        if (!_ReserveSpace(m_slotId, pItem, pItemDest, count))
            return EQUIP_ERR_ITEM_CANT_STACK;

        if (count == 0)
            return EQUIP_ERR_OK;
    }

    // Slot was not specified or it has not enough space for all the items in stack
    // Search for stacks to merge with
    if (pItem->GetMaxStackCount() > 1)
    {
        _CanStoreItemInTab(pItem, m_slotId, true, count);
        if (count == 0)
            return EQUIP_ERR_OK;
    }

    // Search free slot for item
    _CanStoreItemInTab(pItem, m_slotId, false, count);
    if (count == 0)
        return EQUIP_ERR_OK;

    return EQUIP_ERR_BANK_FULL;
}

///////////////////////////////////////////////////////////////////////////////
// Guild
Guild::Guild() : m_id(0), m_leaderGuid(0), m_createdDate(0), m_accountsNumber(0), m_bankMoney(0), m_achievementMgr(this), m_lastXPSave(0), m_eventLog(NULL)
{
    memset(&m_bankEventLog, 0, (GUILD_BANK_MAX_TABS + 1) * sizeof(LogHolder*));
    memset(&m_guildChallenges, 0, GUILD_CHALLENGE_ARRAY_SIZE*sizeof(GuildChallenge));
}

Guild::~Guild()
{
    SQLTransaction temp(NULL);
    _DeleteBankItems(temp);

    // Cleanup
    if (m_eventLog)
        delete m_eventLog;
    for (uint8 tabId = 0; tabId <= GUILD_BANK_MAX_TABS; ++tabId)
        if (m_bankEventLog[tabId])
            delete m_bankEventLog[tabId];
    for (Members::iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        delete itr->second;
}

// Creates new guild with default data and saves it to database.
bool Guild::Create(Player* pLeader, const std::string& name)
{
    // Check if guild with such name already exists
    if (sObjectMgr->GetGuildByName(name))
        return false;

    WorldSession* pLeaderSession = pLeader->GetSession();
    if (!pLeaderSession)
        return false;

    m_id = sObjectMgr->GenerateGuildId();
    m_leaderGuid = pLeader->GetGUID();
    m_name = name;
    m_info = "";
    m_motd = "No message set.";
    m_bankMoney = 0;
    m_createdDate = ::time(NULL);
    m_level = 1;
    m_xp = 0;
    m_nextLevelXP = sObjectMgr->GetXPForGuildLevel(m_level);
    _CreateLogHolders();

    sLog->outDebug("GUILD: creating guild [%s] for leader %s (%u)", 
        name.c_str(), pLeader->GetName(), GUID_LOPART(m_leaderGuid));

    PreparedStatement* stmt = NULL;
    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_MEMBERS);
    stmt->setUInt32(0, m_id);
    trans->Append(stmt);

    uint8 index = 0;
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_ADD_GUILD);
    stmt->setUInt32(  index, m_id);
    stmt->setString(++index, name);
    stmt->setUInt32(++index, GUID_LOPART(m_leaderGuid));
    stmt->setString(++index, m_info);
    stmt->setString(++index, m_motd);
    stmt->setUInt64(++index, uint64(m_createdDate));
    stmt->setUInt32(++index, m_emblemInfo.GetStyle());
    stmt->setUInt32(++index, m_emblemInfo.GetColor());
    stmt->setUInt32(++index, m_emblemInfo.GetBorderStyle());
    stmt->setUInt32(++index, m_emblemInfo.GetBorderColor());
    stmt->setUInt32(++index, m_emblemInfo.GetBackgroundColor());
    stmt->setUInt64(++index, m_bankMoney);
    stmt->setUInt64(++index, m_xp);
    stmt->setUInt32(++index, m_level);
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
    // Create default ranks
    _CreateDefaultGuildRanks(pLeaderSession->GetSessionDbLocaleIndex());
    // Add guildmaster
    bool ret = AddMember(m_leaderGuid, GR_GUILDMASTER);
    if (ret)
        // Call scripts on successful create
        sScriptMgr->OnGuildCreate(this, pLeader, name);

    return ret;
}

// Disbands guild and deletes all related data from database
void Guild::Disband()
{
    // Call scripts before guild data removed from database
    sScriptMgr->OnGuildDisband(this);

    _BroadcastEvent(GE_DISBANDED, 0);
    // Remove all members
    uint64 guid;
    while (!m_members.empty())
    {
        Members::const_iterator itr = m_members.begin();
        guid = itr->second->GetGUID();
        DeleteMember(guid, true);

        Player* target = sObjectMgr->GetPlayer(guid);
        if (target)
            target->SetLeaveGuildData(GetId());
        else
            Player::SetOfflineLeaveGuildData(guid, GetId());
    }

    PreparedStatement* stmt = NULL;
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD);
    stmt->setUInt32(0, m_id);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_RANKS);
    stmt->setUInt32(0, m_id);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_BANK_TABS);
    stmt->setUInt32(0, m_id);
    trans->Append(stmt);

    // Free bank tab used memory and delete items stored in them
    _DeleteBankItems(trans, true);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_BANK_ITEMS);
    stmt->setUInt32(0, m_id);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_BANK_RIGHTS);
    stmt->setUInt32(0, m_id);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_BANK_EVENTLOGS);
    stmt->setUInt32(0, m_id);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_EVENTLOGS);
    stmt->setUInt32(0, m_id);
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
    sGuildFinderMgr->DeleteGuild(m_id);
    sObjectMgr->RemoveGuild(m_id);
}

void Guild::UpdateMemberData(Player* plr, uint8 dataid, uint32 value)
{
    if (Member* pMember = GetMember(plr->GetGUID()))
    {
        switch(dataid)
        {
        case GUILD_MEMBER_DATA_ZONEID:
            pMember->SetZoneID(value);
            break;
        case GUILD_MEMBER_DATA_ACHIEVEMENT_POINTS:
            pMember->SetAchievementPoints(value);
            break;
        case GUILD_MEMBER_DATA_LEVEL:
            pMember->SetLevel(value);
            break;
        default: 
            sLog->outError("Guild::UpdateMemberData: Called with incorrect DATAID %u (value %u)", dataid, value);
            break;
        }
    }
}

void Guild::OnPlayerStatusChange(Player* plr, uint32 flag, bool state)
{
    if (Member* pMember = GetMember(plr->GetGUID()))
    {
        if(state)
            pMember->AddFlag(flag);
        else pMember->RemFlag(flag);
    }
}

///////////////////////////////////////////////////////////////////////////////
// HANDLE CLIENT COMMANDS
void Guild::HandleRoster(WorldSession *session /*= NULL*/)
{
    ByteBuffer memberData(100);
    // Guess size
    WorldPacket data(SMSG_GUILD_ROSTER, 100);
    data.WriteBits(m_motd.length(), 11);
    data.WriteBits(m_members.size(), 18);

    for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
    {
        Member* member = itr->second;
        size_t pubNoteLength = member->GetPublicNote().length();
        size_t offNoteLength = member->GetOfficerNote().length();

        ObjectGuid guid = member->GetGUID();
        data.WriteBit(guid[3]);
        data.WriteBit(guid[4]);
        data.WriteBit(0); // Has Authenticator
        data.WriteBit(0); // Can Scroll of Ressurect
        data.WriteBits(pubNoteLength, 8);
        data.WriteBits(offNoteLength, 8);
        data.WriteBit(guid[0]);
        data.WriteBits(member->GetName().length(), 7);
        data.WriteBit(guid[1]);
        data.WriteBit(guid[2]);
        data.WriteBit(guid[6]);
        data.WriteBit(guid[5]);
        data.WriteBit(guid[7]);

        memberData << uint8(member->GetClass());
        memberData << uint32(0);      // total guild reputation of member
        memberData.WriteByteSeq(guid[0]);
        memberData << uint64(member->m_activityWeek); // week activity
        memberData << uint32(member->GetRankId());
        memberData << uint32(member->GetAchievementPoints());

        for (uint32 i = 0; i < 2; i++)
            memberData << uint32(member->professions[i].title) << uint32(member->professions[i].skillValue) << uint32(member->professions[i].skillId);

        memberData.WriteByteSeq(guid[2]);
        memberData << uint8(member->GetFlags());
        memberData << uint32(member->GetZoneId());
        memberData << uint64(member->m_activityTotal); // total activity
        memberData.WriteByteSeq(guid[7]);
        memberData << uint32(0);    // remaining reputation the member can gain

        if (pubNoteLength)
            memberData.WriteString(member->GetPublicNote());

        memberData.WriteByteSeq(guid[3]);
        memberData << uint8(member->GetLevel());
        memberData << int32(0);                                     // unk
        memberData.WriteByteSeq(guid[5]);
        memberData.WriteByteSeq(guid[4]);
        memberData << uint8(0);                                     // unk
        memberData.WriteByteSeq(guid[1]);
        memberData << float(member->IsOnline() ? 0.0f : float(::time(NULL) - member->GetLogoutTime()) / DAY);

        if (offNoteLength)
            memberData.WriteString(member->GetOfficerNote());

        memberData.WriteByteSeq(guid[6]);
        memberData.WriteString(member->GetName());
    }

    size_t infoLength = m_info.length();
    data.WriteBits(infoLength, 12);

    data.FlushBits();
    data.append(memberData);

    if (infoLength)
        data.WriteString(m_info);

    data.WriteString(m_motd);
    data << uint32(m_accountsNumber);
    data << uint32(0);      // weekly reputation cap
    data.AppendPackedTime(m_createdDate);
    data << uint32(0);

    if (session)
    {
        session->SendPacket(&data);
        UpdateGuildNews(session);
    }
    else
        BroadcastPacket(&data);
}

void Guild::UpdateGuildNews(WorldSession* session)
{
    if (!sWorld->getBoolConfig(CONFIG_GUILD_ADVANCEMENT_ENABLED))
        return;

    /*
    Event Types:
     0 = guild achievement,   param = achievementID
     1 = player achievement,  param = achievementID
     2 = boss encounter,      param = ID from DungeonEncounter.dbc
     3 = epic item loot,      param = item ID
     4 = epic item craft,     param = item ID
     5 = epic item purchase,  param = item ID
     6 = guild level,         param = level
    */

    ObjectGuid guid;
    uint32 count = m_guildNews.size();

    WorldPacket data(SMSG_GUILD_NEWS_UPDATE, 4+(5*4+2*8)*count);
    data.WriteBits(count, 21);

    for (GuildNewsList::const_iterator itr = m_guildNews.begin(); itr != m_guildNews.end(); ++itr)
    {
        data.WriteBits(0, 26); // Not yet implemented used for guild achievements
        guid = (*itr).guid;

        data.WriteBit(guid[7]);
        data.WriteBit(guid[0]);
        data.WriteBit(guid[6]);
        data.WriteBit(guid[5]);
        data.WriteBit(guid[4]);
        data.WriteBit(guid[3]);
        data.WriteBit(guid[1]);
        data.WriteBit(guid[2]);
    }

    data.FlushBits();

    for (GuildNewsList::const_iterator itr = m_guildNews.begin(); itr != m_guildNews.end(); ++itr)
    {
        guid = (*itr).guid;
        data.WriteByteSeq(guid[5]);

        data << uint32(0);            // flags, 1 sticky
        data << uint32((*itr).param);
        data << uint32(0);            // unk

        data.WriteByteSeq(guid[7]);
        data.WriteByteSeq(guid[6]);
        data.WriteByteSeq(guid[2]);
        data.WriteByteSeq(guid[3]);
        data.WriteByteSeq(guid[0]);
        data.WriteByteSeq(guid[4]);
        data.WriteByteSeq(guid[1]);

        data << uint32((*itr).id);
        data << uint32((*itr).type);
        data << uint32((*itr).date);
    }

    if (session)
        session->SendPacket(&data);
}

void Guild::CompleteChallenge(Group* pSource, GuildChallengeType type)
{
    if (type == GUILD_CHALLENGE_DUNGEON && m_guildChallenges[type-1].count >= GUILD_CHALLENGE_WEEK_DUNGEON_COUNT)
        return;
    if (type == GUILD_CHALLENGE_RAID && m_guildChallenges[type-1].count >= GUILD_CHALLENGE_WEEK_RAID_COUNT)
        return;
    if (type == GUILD_CHALLENGE_BG && m_guildChallenges[type-1].count >= GUILD_CHALLENGE_WEEK_BG_COUNT)
        return;

    m_guildChallenges[type-1].count++;

    for (GroupReference* itr = pSource->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        if (itr->getSource()->GetGuildId() == GetId())
        {
            SendChallengeCompleted(itr->getSource()->GetSession(), type);
            SendChallengeUpdate(itr->getSource()->GetSession());
        }
    }

    uint32 xprew = 0, moneyrew = 0;
    // choose reward
    if (m_level >= 5 && m_level <= 24)
    {
        if (type == GUILD_CHALLENGE_DUNGEON)
        {
            xprew = GCH_REWARD_XP_DUNGEON;
            moneyrew = GCH_REWARD_MONEY_DUNGEON;
        }
        else if (type == GUILD_CHALLENGE_RAID)
        {
            xprew = GCH_REWARD_XP_RAID;
            moneyrew = GCH_REWARD_MONEY_RAID;
        }
        else if (type == GUILD_CHALLENGE_BG)
        {
            xprew = GCH_REWARD_XP_BG;
            moneyrew = GCH_REWARD_MONEY_BG;
        }
    }
    else if (m_level == 25)
    {
        xprew = 0;

        if (type == GUILD_CHALLENGE_DUNGEON)
            moneyrew = GCH_REWARD_MONEY_DUNGEON_25;
        else if (type == GUILD_CHALLENGE_RAID)
            moneyrew = GCH_REWARD_MONEY_RAID_25;
        else if (type == GUILD_CHALLENGE_BG)
            moneyrew = GCH_REWARD_MONEY_BG_25;
    }

    if (xprew > 0)
        GainXP(xprew);
    DepositBankMoney(moneyrew);

    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GUILD_CHALLENGE_GENERIC);
    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GUILD_CHALLENGE_SPECIFIC, type);

    CharacterDatabase.PExecute("REPLACE INTO guild_week_challenge VALUES (%u, %u, %u, %u);", GetId(), m_guildChallenges[GUILD_CHALLENGE_DUNGEON-1].count,
        m_guildChallenges[GUILD_CHALLENGE_RAID-1].count, m_guildChallenges[GUILD_CHALLENGE_BG-1].count);
}

void Guild::SendChallengeCompleted(WorldSession* session, GuildChallengeType type)
{
    WorldPacket data(SMSG_GUILD_CHALLENGE_COMPLETED);

    uint32 xprew = 0;
    uint32 moneyrew = 0;
    uint32 totalCount = 0;

    if (m_level >= 5 && m_level <= 24)
    {
        if (type == GUILD_CHALLENGE_DUNGEON)
        {
            xprew = GCH_REWARD_XP_DUNGEON;
            moneyrew = GCH_REWARD_MONEY_DUNGEON;
        }
        else if (type == GUILD_CHALLENGE_RAID)
        {
            xprew = GCH_REWARD_XP_RAID;
            moneyrew = GCH_REWARD_MONEY_RAID;
        }
        else if (type == GUILD_CHALLENGE_BG)
        {
            xprew = GCH_REWARD_XP_BG;
            moneyrew = GCH_REWARD_MONEY_BG;
        }
    }
    else if (m_level == 25)
    {
        xprew = 0;

        if (type == GUILD_CHALLENGE_DUNGEON)
            moneyrew = GCH_REWARD_MONEY_DUNGEON_25;
        else if (type == GUILD_CHALLENGE_RAID)
            moneyrew = GCH_REWARD_MONEY_RAID_25;
        else if (type == GUILD_CHALLENGE_BG)
            moneyrew = GCH_REWARD_MONEY_BG_25;
    }

    if (type == GUILD_CHALLENGE_DUNGEON)
        totalCount = GUILD_CHALLENGE_WEEK_DUNGEON_COUNT;
    else if (type == GUILD_CHALLENGE_RAID)
        totalCount = GUILD_CHALLENGE_WEEK_RAID_COUNT;
    else if (type == GUILD_CHALLENGE_BG)
        totalCount = GUILD_CHALLENGE_WEEK_BG_COUNT;

    data << uint32(type);                             // type, 1 dung, 2 raid, 3 rbg
    data << uint32(moneyrew / GOLD);                  // gold reward
    data << uint32(m_guildChallenges[type-1].count);  // count
    data << uint32(xprew);                            // XP reward
    data << uint32(totalCount);                       // total count

    if (session)
        session->SendPacket(&data);
}

void Guild::SendChallengeUpdate(WorldSession* session)
{
    WorldPacket data(SMSG_GUILD_CHALLENGE_UPDATED);

    // guilds with level lower than two don't have challenges active
    if (m_level < 5)
    {
        for (uint32 i = 0; i < 5; i++)
            for (uint32 j = 0; j < 4; j++)
                data << uint32(0);

        if (session)
            session->SendPacket(&data);

        return;
    }

    // indexes are moved by one (array indexes 0-2, client types 1-3)

    data << uint32(0);                                      // unk (MoP challenge mode dungeon?)
    data << uint32(GCH_REWARD_XP_DUNGEON);                  // XP reward dungeons
    data << uint32(GCH_REWARD_XP_RAID);                     // XP reward raids
    data << uint32(GCH_REWARD_XP_BG);                       // XP reward rbgs

    data << uint32(0);                                      // unk (MoP challenge mode dungeon lvl 25?)
    data << uint32(GCH_REWARD_MONEY_DUNGEON_25 / GOLD);     // gold reward dungeons lvl 25
    data << uint32(GCH_REWARD_MONEY_RAID_25 / GOLD);        // gold reward raids lvl 25
    data << uint32(GCH_REWARD_MONEY_BG_25 / GOLD);          // gold reward rbgs lvl 25

    data << uint32(0);                                      // unk (MoP challenge mode dungeon?)
    data << uint32(GUILD_CHALLENGE_WEEK_DUNGEON_COUNT);     // total dungeons
    data << uint32(GUILD_CHALLENGE_WEEK_RAID_COUNT);        // total raids
    data << uint32(GUILD_CHALLENGE_WEEK_BG_COUNT);          // total rbgs

    data << uint32(0);                                      // unk (MoP challenge mode dungeon?)
    data << uint32(GCH_REWARD_MONEY_DUNGEON / GOLD);        // gold reward dungeons
    data << uint32(GCH_REWARD_MONEY_RAID / GOLD);           // gold reward raids
    data << uint32(GCH_REWARD_MONEY_BG / GOLD);             // gold reward rbgs

    data << uint32(0);                                                  // unk (MoP challenge mode dungeon?)
    data << uint32(m_guildChallenges[GUILD_CHALLENGE_DUNGEON-1].count); // dungeons done
    data << uint32(m_guildChallenges[GUILD_CHALLENGE_RAID-1].count);    // raids done
    data << uint32(m_guildChallenges[GUILD_CHALLENGE_BG-1].count);      // rbgs done

    if (session)
        session->SendPacket(&data);
}

void Guild::ClearChallenges()
{
    memset(&m_guildChallenges, 0, GUILD_CHALLENGE_ARRAY_SIZE*sizeof(GuildChallenge));
}

void Guild::AddMemberNews(Player* pPlayer, GuildNewsType type, uint64 param)
{
    if (!sWorld->getBoolConfig(CONFIG_GUILD_ADVANCEMENT_ENABLED))
        return;

    if (!pPlayer || type > GUILD_NEWS_GUILD_LEVEL || type < GUILD_NEWS_GUILD_ACHIEVEMENT)
        return;

    uint32 new_id = sObjectMgr->GenerateGuildNewsID();
    uint32 date = secsToTimeBitFields(time(NULL));

    CharacterDatabase.PExecute("INSERT INTO guild_news (id, guildid, event_type, param, date, playerguid) VALUES (%u,%u,%u,%u,%u,%u)",
        uint32(new_id), uint32(m_id), uint32(type), uint32(param), uint32(date), uint32(pPlayer->GetGUID()));

    // And save it to guild news list
    GuildNewsEntry gn;
    gn.id = new_id;
    gn.type = type;
    gn.param = param;
    gn.date = date;
    gn.guid = pPlayer->GetGUID();
    m_guildNews.push_back(gn);
}

void Guild::AddGuildNews(GuildNewsType type, uint64 param)
{
    if (!sWorld->getBoolConfig(CONFIG_GUILD_ADVANCEMENT_ENABLED))
        return;

    if (type > GUILD_NEWS_GUILD_LEVEL || type < GUILD_NEWS_GUILD_ACHIEVEMENT)
        return;

    uint32 new_id = sObjectMgr->GenerateGuildNewsID();
    uint32 date = secsToTimeBitFields(time(NULL));

    CharacterDatabase.PExecute("INSERT INTO guild_news (id, guildid, event_type, param, date, playerguid) VALUES (%u,%u,%u,%u,%u,%u)",
        uint32(new_id), uint32(m_id), uint32(type), uint32(param), uint32(date), 0);

    // And save it to guild news list
    GuildNewsEntry gn;
    gn.id = new_id;
    gn.type = type;
    gn.param = param;
    gn.date = date;
    gn.guid = 0;
    m_guildNews.push_back(gn);
}

void Guild::HandleQuery(WorldSession *session)
{
    WorldPacket data(SMSG_GUILD_QUERY_RESPONSE, 8 * 32 + 200);      // Guess size

    uint64 guid = MAKE_NEW_GUID(m_id, 0, HIGHGUID_GUILD);
    data << uint64(guid);
    data << m_name;

    for (uint8 i = 0 ; i < GUILD_RANKS_MAX_COUNT; ++i)              // Always show 10 ranks
    {
        if (i < _GetRanksSize())
            data << m_ranks[i].GetName();
        else
            data << uint8(0);                                       // Empty string
    }

    for(uint8 i = 0; i < GUILD_RANKS_MAX_COUNT; ++i)
    {
        if (i < _GetRanksSize())
            data << uint32(i);
        else
            data << uint32(0);
    }
    for(uint8 i = 0; i < GUILD_RANKS_MAX_COUNT; ++i)
    {
        if (i < _GetRanksSize())
            data << uint32(i);
        else
            data << uint32(0);
    }

    m_emblemInfo.WritePacket(data);
    data << uint32(7);                                              // Something new in WotLK

    session->SendPacket(&data);
    sLog->outDebug("WORLD: Sent (SMSG_GUILD_QUERY_RESPONSE)");
}

void Guild::SendGuildRankInfo(WorldSession* session) const
{
    ByteBuffer rankData(100);
    WorldPacket data(SMSG_GUILD_RANK, 100);

    data.WriteBits(_GetRanksSize(), 18);

    for (uint8 i = 0; i < _GetRanksSize(); i++)
    {
        RankInfo const* rankInfo = GetRankInfo(i);
        if (!rankInfo)
            continue;

        data.WriteBits(rankInfo->GetName().length(), 7);

        rankData << uint32(rankInfo->GetId());

        for (uint8 j = 0; j < GUILD_BANK_MAX_TABS; ++j)
        {
            if (i == 0)
            {
                rankData << uint32(-1);
                rankData << uint32(-1);
            }
            else
            {
                rankData << uint32(rankInfo->GetBankTabSlotsPerDay(j));
                rankData << uint32(rankInfo->GetBankTabRights(j));
            }
        }

        if (i == 0)
        {
            rankData << uint32(-1);
            rankData << uint32(-1);
        }
        else
        {
            rankData << uint32(rankInfo->GetBankMoneyPerDay() / GOLD);
            rankData << uint32(rankInfo->GetRights());
        }

        if (rankInfo->GetName().length())
            rankData.WriteString(rankInfo->GetName());

        rankData << uint32(i);
    }

    data.FlushBits();
    data.append(rankData);
    session->SendPacket(&data);
}

void Guild::HandleSetMOTD(WorldSession* session, const std::string& motd)
{
    if (m_motd == motd)
        return;

    // Player must have rights to set MOTD
    if (!_HasRankRight(session->GetPlayer(), GR_RIGHT_SETMOTD))
        SendCommandResult(session, GUILD_COMMAND_CREATE, ERR_GUILD_PERMISSIONS);
    else
    {
        m_motd = motd;

        sScriptMgr->OnGuildMOTDChanged(this, motd);

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SET_GUILD_MOTD);
        stmt->setString(0, motd);
        stmt->setUInt32(1, m_id);
        CharacterDatabase.Execute(stmt);

        _BroadcastEvent(GE_MOTD, 0, motd.c_str());
    }
}

void Guild::HandleSetInfo(WorldSession* session, const std::string& info)
{
    if (m_info == info)
        return;

    // Player must have rights to set guild's info
    if (!_HasRankRight(session->GetPlayer(), GR_RIGHT_MODIFY_GUILD_INFO))
        SendCommandResult(session, GUILD_COMMAND_CREATE, ERR_GUILD_PERMISSIONS);
    else
    {
        m_info = info;

        sScriptMgr->OnGuildInfoChanged(this, info);

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SET_GUILD_INFO);
        stmt->setString(0, info);
        stmt->setUInt32(1, m_id);
        CharacterDatabase.Execute(stmt);
    }
}

void Guild::HandleSetEmblem(WorldSession* session, const EmblemInfo& emblemInfo)
{
    Player* player = session->GetPlayer();
    if (!_IsLeader(player))
        // "Only pGuild leaders can create emblems."
        SendSaveEmblemResult(session, ERR_GUILDEMBLEM_NOTGUILDMASTER);
    else if (!player->HasEnoughMoney(EMBLEM_PRICE))
        // "You can't afford to do that."
        SendSaveEmblemResult(session, ERR_GUILDEMBLEM_NOTENOUGHMONEY);
    else
    {
        player->ModifyMoney(-int32(EMBLEM_PRICE));

        m_emblemInfo = emblemInfo;
        m_emblemInfo.SaveToDB(m_id);

        // "Guild Emblem saved."
        SendSaveEmblemResult(session, ERR_GUILDEMBLEM_SUCCESS);

        HandleQuery(session);
    }
}

void Guild::HandleSetLeader(WorldSession* session, const std::string& name)
{
    Player* player = session->GetPlayer();
    // Only leader can assign new leader
    if (!_IsLeader(player))
        SendCommandResult(session, GUILD_COMMAND_CREATE, ERR_GUILD_PERMISSIONS);
    // Old leader must be a member of guild
    else if (Member* pOldLeader = GetMember(player->GetGUID()))
    {
        // New leader must be a member of guild
        if (Member* pNewLeader = GetMember(session, name))
        {
            _SetLeaderGUID(pNewLeader);
            pOldLeader->ChangeRank(GR_OFFICER);
            _BroadcastEvent(GE_LEADER_CHANGED, 0, player->GetName(), name.c_str());
        }
    }
    else
        SendCommandResult(session, GUILD_COMMAND_CREATE, ERR_GUILD_PERMISSIONS);
}

void Guild::HandleSetBankTabInfo(WorldSession* session, uint8 tabId, const std::string& name, const std::string& icon)
{
    if (BankTab* pTab = GetBankTab(tabId))
    {
        char aux[2];
        sprintf(aux, "%u", tabId);

        pTab->SetInfo(name, icon);
        _BroadcastEvent(GE_BANK_TAB_UPDATED, 0, aux, name.c_str(), icon.c_str());
    }
}

void Guild::HandleSetMemberNote(WorldSession* session, uint64 guid, const std::string& note, bool officer)
{
    // Player must have rights to set public/officer note
    if (!_HasRankRight(session->GetPlayer(), officer ? GR_RIGHT_EOFFNOTE : GR_RIGHT_EPNOTE))
        SendCommandResult(session, GUILD_COMMAND_CREATE, ERR_GUILD_PERMISSIONS);
    // Noted player must be a member of guild
    else if (Member* pMember = GetMember(guid))
    {
        if (officer)
            pMember->SetOfficerNote(note);
        else
            pMember->SetPublicNote(note);
        HandleRoster(session);
    }
}

void Guild::HandleSetRankInfo(WorldSession* session, uint8 rankId, const std::string& name, uint32 rights, uint32 moneyPerDay, GuildBankRightsAndSlotsVec rightsAndSlots)
{
    // Only leader can modify ranks
    if (!_IsLeader(session->GetPlayer()))
        SendCommandResult(session, GUILD_COMMAND_CREATE, ERR_GUILD_PERMISSIONS);
    else if (RankInfo* rankInfo = GetRankInfo(rankId))
    {
        sLog->outDebug("WORLD: Changed RankName to '%s', rights to 0x%08X", name.c_str(), rights);

        rankInfo->SetName(name);
        rankInfo->SetRights(rights);
        _SetRankBankMoneyPerDay(rankId, moneyPerDay);

        uint8 tabId = 0;
        for (GuildBankRightsAndSlotsVec::const_iterator itr = rightsAndSlots.begin(); itr != rightsAndSlots.end(); ++itr)
            _SetRankBankTabRightsAndSlots(rankId, tabId++, *itr);

        char aux[2];
        sprintf(aux, "%u", rankId);
        _BroadcastEvent(GE_RANK_UPDATED, 0, aux);
    }
}

void Guild::HandleBuyBankTab(WorldSession* session, uint8 tabId)
{
    if (tabId != _GetPurchasedTabsSize())
        return;

    uint32 tabCost = _GetGuildBankTabPrice(tabId) * GOLD;
    // tabs 6 and 7 (7 and 8 in game numbering) is bought by items and has no cost at all)
    if (!tabCost && tabId != 6 && tabId != 7)
        return;

    Player* player = session->GetPlayer();
    if (!player->HasEnoughMoney(tabCost))                   // Should not happen, this is checked by client
        return;

    if (!_CreateNewBankTab())
        return;

    player->ModifyMoney(-int32(tabCost));

    _BroadcastEvent(GE_BANK_TAB_PURCHASED, 0);
    SendPermissions(session); /// Hack to force client to update permissions
}

void Guild::HandleInviteMember(WorldSession* session, const std::string& name)
{
    Player* pInvitee = sObjectAccessor->FindPlayerByName(name.c_str());
    if (!pInvitee)
    {
        SendCommandResult(session, GUILD_COMMAND_INVITE, ERR_GUILD_PLAYER_NOT_FOUND_S, name);
        session->SendPlayerNotFoundNotice(name);
        return;
    }

    Player* player = session->GetPlayer();
    // Do not show invitations from ignored players
    if (pInvitee->GetSocial()->HasIgnore(player->GetGUIDLow()))
        return;
    if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD) && pInvitee->GetTeam() != player->GetTeam())
    {
        SendCommandResult(session, GUILD_COMMAND_CREATE, ERR_GUILD_NOT_ALLIED, name);
        return;
    }

    // Invited player cannot be in another guild
    if (pInvitee->GetGuildId())
    {
        SendCommandResult(session, GUILD_COMMAND_CREATE, ERR_ALREADY_IN_GUILD_S, name);
        return;
    }

    // Invited player cannot be invited
    if (pInvitee->GetGuildIdInvited())
    {
        SendCommandResult(session, GUILD_COMMAND_CREATE, ERR_ALREADY_INVITED_TO_GUILD_S, name);
        return;
    }
    // Inviting player must have rights to invite
    if (!_HasRankRight(player, GR_RIGHT_INVITE))
    {
        SendCommandResult(session, GUILD_COMMAND_CREATE, ERR_GUILD_PERMISSIONS);
        return;
    }

    sLog->outDebug("Player %s invited %s to join his Guild", player->GetName(), name.c_str());

    pInvitee->SetGuildIdInvited(m_id);
    _LogEvent(GUILD_EVENT_LOG_INVITE_PLAYER, player->GetGUIDLow(), pInvitee->GetGUIDLow());

    WorldPacket data(SMSG_GUILD_INVITE, 100);
    data << uint32(GetLevel());
    data << uint32(m_emblemInfo.GetBorderStyle());
    data << uint32(m_emblemInfo.GetBorderColor());
    data << uint32(m_emblemInfo.GetStyle());
    data << uint32(m_emblemInfo.GetBackgroundColor());
    data << uint32(m_emblemInfo.GetColor());

    ObjectGuid oldGuildGuid = MAKE_NEW_GUID(pInvitee->GetGuildId(), 0, pInvitee->GetGuildId() ? uint32(HIGHGUID_GUILD) : 0);
    ObjectGuid newGuildGuid = GetGUID();

    data.WriteBit(newGuildGuid[3]);
    data.WriteBit(newGuildGuid[2]);
    data.WriteBits(pInvitee->GetGuildName().length(), 8);
    data.WriteBit(newGuildGuid[1]);
    data.WriteBit(oldGuildGuid[6]);
    data.WriteBit(oldGuildGuid[4]);
    data.WriteBit(oldGuildGuid[1]);
    data.WriteBit(oldGuildGuid[5]);
    data.WriteBit(oldGuildGuid[7]);
    data.WriteBit(oldGuildGuid[2]);
    data.WriteBit(newGuildGuid[7]);
    data.WriteBit(newGuildGuid[0]);
    data.WriteBit(newGuildGuid[6]);
    data.WriteBits(m_name.length(), 8);
    data.WriteBit(oldGuildGuid[3]);
    data.WriteBit(oldGuildGuid[0]);
    data.WriteBit(newGuildGuid[5]);
    data.WriteBits(strlen(player->GetName()), 7);
    data.WriteBit(newGuildGuid[4]);

    data.FlushBits();

    data.WriteByteSeq(newGuildGuid[1]);
    data.WriteByteSeq(oldGuildGuid[3]);
    data.WriteByteSeq(newGuildGuid[6]);
    data.WriteByteSeq(oldGuildGuid[2]);
    data.WriteByteSeq(oldGuildGuid[1]);
    data.WriteByteSeq(newGuildGuid[0]);

    if (!pInvitee->GetGuildName().empty())
        data.WriteString(pInvitee->GetGuildName());

    data.WriteByteSeq(newGuildGuid[7]);
    data.WriteByteSeq(newGuildGuid[2]);

    data.WriteString(player->GetName());

    data.WriteByteSeq(oldGuildGuid[7]);
    data.WriteByteSeq(oldGuildGuid[6]);
    data.WriteByteSeq(oldGuildGuid[5]);
    data.WriteByteSeq(oldGuildGuid[0]);
    data.WriteByteSeq(newGuildGuid[4]);

    data.WriteString(m_name);

    data.WriteByteSeq(newGuildGuid[5]);
    data.WriteByteSeq(newGuildGuid[3]);
    data.WriteByteSeq(oldGuildGuid[4]);
    pInvitee->GetSession()->SendPacket(&data);

    sLog->outDebug("WORLD: Sent (SMSG_GUILD_INVITE)");
}

void Guild::HandleAcceptMember(WorldSession* session)
{
    Player* player = session->GetPlayer();
    if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD) && 
        player->GetTeam() != sObjectMgr->GetPlayerTeamByGUID(GetLeaderGUID()))
        return;

    if (AddMember(player->GetGUID()))
    {
        _LogEvent(GUILD_EVENT_LOG_JOIN_GUILD, player->GetGUIDLow());
        _BroadcastEvent(GE_JOINED, player->GetGUID(), player->GetName());
        sGuildFinderMgr->RemoveAllMembershipRequestsFromPlayer(player->GetGUIDLow());
    }
}

void Guild::HandleLeaveMember(WorldSession* session)
{
    Player* player = session->GetPlayer();
    // If leader is leaving
    if (_IsLeader(player))
    {
        if (m_members.size() > 1)
            // Leader cannot leave if he is not the last member
            SendCommandResult(session, GUILD_COMMAND_QUIT, ERR_GUILD_LEADER_LEAVE);
        else
            // Guild is disbanded if leader leaves.
            Disband();
    }
    else
    {
        DeleteMember(player->GetGUID(), false, false);

        _LogEvent(GUILD_EVENT_LOG_LEAVE_GUILD, player->GetGUIDLow());
        _BroadcastEvent(GE_LEFT, player->GetGUID(), player->GetName());

        SendCommandResult(session, GUILD_COMMAND_QUIT, ERR_GUILD_COMMAND_SUCCESS, m_name);

        player->SetLeaveGuildData(GetId());
    }
}

void Guild::HandleRemoveMember(WorldSession* session, uint64 guid)
{
    Player* player = session->GetPlayer();
    // Player must have rights to remove members
    if (!_HasRankRight(player, GR_RIGHT_REMOVE))
        SendCommandResult(session, GUILD_COMMAND_CREATE, ERR_GUILD_PERMISSIONS);
    // Removed player must be a member of guild
    else if (Member* pMember = GetMember(guid))
    {
        std::string name = pMember->GetName();

        // Leader cannot be removed
        if (pMember->IsRank(GR_GUILDMASTER))
            SendCommandResult(session, GUILD_COMMAND_QUIT, ERR_GUILD_LEADER_LEAVE);
        // Do not allow to remove player with the same rank or higher
        else if (pMember->IsRankNotLower(player->GetRank()))
            SendCommandResult(session, GUILD_COMMAND_QUIT, ERR_GUILD_RANK_TOO_HIGH_S, name);
        else
        {
            // After call to DeleteMember pointer to member becomes invalid
            DeleteMember(guid, false, true);
            _LogEvent(GUILD_EVENT_LOG_UNINVITE_PLAYER, player->GetGUIDLow(), GUID_LOPART(guid));
            _BroadcastEvent(GE_REMOVED, 0, name.c_str(), player->GetName());

            Player* target = sObjectMgr->GetPlayer(guid);
            if (target)
                target->SetLeaveGuildData(GetId());
            else
                Player::SetOfflineLeaveGuildData(guid, GetId());
        }
    }
}

void Guild::HandleUpdateMemberRank(WorldSession* session, uint64 guid, bool demote)
{
    Player* player = session->GetPlayer();
    // Player must have rights to promote
    if (!_HasRankRight(player, demote ? GR_RIGHT_DEMOTE : GR_RIGHT_PROMOTE))
        SendCommandResult(session, GUILD_COMMAND_CREATE, ERR_GUILD_PERMISSIONS);
    // Promoted player must be a member of guild
    else if (Member* pMember = GetMember(guid))
    {
        std::string name = pMember->GetName();
        // Player cannot promote himself
        if (pMember->IsSamePlayer(player->GetGUID()))
        {
            SendCommandResult(session, GUILD_COMMAND_CREATE, ERR_GUILD_NAME_INVALID);
            return;
        }

        if (demote)
        {
            // Player can demote only lower rank members
            if (pMember->IsRankNotLower(player->GetRank()))
            {
                SendCommandResult(session, GUILD_COMMAND_CREATE, ERR_GUILD_RANK_TOO_HIGH_S, name);
                return;
            }
            // Lowest rank cannot be demoted
            if (pMember->GetRankId() >= _GetLowestRankId())
            {
                SendCommandResult(session, GUILD_COMMAND_CREATE, ERR_GUILD_RANK_TOO_LOW_S, name);
                return;
            }
        }
        else
        {
            // Allow to promote only to lower rank than member's rank
            // pMember->GetRank() + 1 is the highest rank that current player can promote to
            if (pMember->IsRankNotLower(player->GetRank() + 1))
            {
                SendCommandResult(session, GUILD_COMMAND_CREATE, ERR_GUILD_RANK_TOO_HIGH_S, name);
                return;
            }
        }

        // When promoting player, rank is decreased, when demoting - increased
        uint32 newRankId = pMember->GetRankId() + (demote ? 1 : -1);
        pMember->ChangeRank(newRankId);
        _LogEvent(demote ? GUILD_EVENT_LOG_DEMOTE_PLAYER : GUILD_EVENT_LOG_PROMOTE_PLAYER, player->GetGUIDLow(), GUID_LOPART(pMember->GetGUID()), newRankId);
        _BroadcastEvent(demote ? GE_DEMOTION : GE_PROMOTION, 0, player->GetName(), name.c_str(), _GetRankName(newRankId).c_str());
        HandleRoster();
    }
}

void Guild::HandleSetMemberRank(WorldSession *session, uint64 targetGuid, uint64 setterGuid, uint32 rank)
{
    Player *player = session->GetPlayer();
    Member *member = GetMember(targetGuid);
    GuildRankRights rights = GR_RIGHT_PROMOTE;
    GuildCommandType type = GUILD_COMMAND_PROMOTE;
    if(!member)
    {
        return;
    }

    if (rank > member->GetRankId())
    {
        rights = GR_RIGHT_DEMOTE;
        type = GUILD_COMMAND_DEMOTE;
    }

    // Promoted player must be a member of guild
    if (!_HasRankRight(player, rights))
    {
        SendCommandResult(session, type, ERR_GUILD_PERMISSIONS);
        return;
    }

    // Player cannot promote himself
    if (member->IsSamePlayer(player->GetGUID()))
    {
        SendCommandResult(session, type, ERR_GUILD_NAME_INVALID);
        return;
    }

    SendGuildRanksUpdate(setterGuid, targetGuid, rank);
}

void Guild::HandleAddNewRank(WorldSession* session, const std::string& name)
{
    if (_GetRanksSize() >= GUILD_RANKS_MAX_COUNT)
        return;

    // Only leader can add new rank
    if (!_IsLeader(session->GetPlayer()))
        SendCommandResult(session, GUILD_COMMAND_CREATE, ERR_GUILD_PERMISSIONS);
    else
    {
        if (_CreateRank(name, GR_RIGHT_GCHATLISTEN | GR_RIGHT_GCHATSPEAK))
            _BroadcastEvent(GE_RANK_CREATED, 0);
    }
}

void Guild::HandleSwitchRank(WorldSession* session, uint8 rankId, bool up)
{
    /*if (rankId == 0 && up)
        return;

    if (rankId > _GetRanksSize() && !up)
        return;

    RankInfo* src = GetRankInfo((uint8)rankId);
    RankInfo* dst = up ? GetRankInfo((uint8)(rankId - 1)) : GetRankInfo((uint8)(rankId + 1));

    if (src && dst)
    {
        uint8 srcPos = src->GetRankPos();
        dst->SetRankPos(srcPos);
        src->SetRankPos(up ? (srcPos - 1) : (srcPos + 1));
    }*/
    if(up)
        SwitchRank(rankId,rankId-1);
    else
        SwitchRank(rankId,rankId+1);
    SendGuildRankInfo(session);
}

void Guild::HandleRemoveRank(WorldSession* session, uint8 rankId)
{
    // Cannot remove rank if total count is minimum allowed by the client
    if (_GetRanksSize() <= GUILD_RANKS_MIN_COUNT || rankId >= _GetRanksSize())
        return;

    // Only leader can delete ranks
    if (!_IsLeader(session->GetPlayer()))
        SendCommandResult(session, GUILD_COMMAND_CREATE, ERR_GUILD_PERMISSIONS);
    else
    {
        SQLTransaction trans = CharacterDatabase.BeginTransaction();

        //uint8 rankId = _GetLowestRankId();
        // Delete bank rights for rank
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_BANK_RIGHTS_FOR_RANK);
        stmt->setUInt32(0, m_id);
        stmt->setUInt8 (1, rankId);
        trans->Append(stmt);
        // Delete rank
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_LOWEST_RANK);
        stmt->setUInt32(0, m_id);
        stmt->setUInt8 (1, rankId);
        trans->Append(stmt);
        //Move members to the new correct position
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_MOVE_GUILD_BANK_RIGHT);
        stmt->setUInt32(0, m_id);
        stmt->setUInt8 (1, rankId);
        trans->Append(stmt);
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_MOVE_GUILD_LOWEST_RANK);
        stmt->setUInt32(0, m_id);
        stmt->setUInt8 (1, rankId);
        trans->Append(stmt);
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_MOVE_GUILD_MEMBER);
        stmt->setUInt32(0, m_id);
        stmt->setUInt8 (1, rankId);
        trans->Append(stmt);

        CharacterDatabase.CommitTransaction(trans);

        m_ranks.erase(m_ranks.begin()+rankId);
        Members::const_iterator itr;
        for(itr = m_members.begin(); itr != m_members.end(); itr++)
        {
            if(itr->second->GetRankId()>rankId)
                itr->second->ChangeRank(itr->second->GetRankId()-1);
        }
        Ranks::iterator itr2;
        for(itr2 = m_ranks.begin(); itr2 != m_ranks.end(); itr2++)
        {
            if(itr2->GetId()>rankId)
                itr2->SetId(itr2->GetId()-1);
        }
        _BroadcastEvent(GE_RANK_DELETED, rankId);
    }
}

void Guild::DepositBankMoney(uint32 amount)
{
    if (!_GetPurchasedTabsSize())
        return;

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    // Add money to bank
    _ModifyBankMoney(trans, amount, true);

    CharacterDatabase.CommitTransaction(trans);
}

void Guild::HandleMemberDepositMoney(WorldSession* session, uint32 amount)
{
    if (!_GetPurchasedTabsSize())
        return;                                                     // No guild bank tabs - no money in bank

    Player* player = session->GetPlayer();

    // Call script after validation and before money transfer.
    sScriptMgr->OnGuildMemberDepositMoney(this, player, amount);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    // Add money to bank
    _ModifyBankMoney(trans, amount, true);
    // Remove money from player
    player->ModifyMoney(-int32(amount));
    player->SaveGoldToDB(trans);
    // Log GM action (TODO: move to scripts)
    if (player->GetSession()->GetSecurity() > SEC_PLAYER && sWorld->getBoolConfig(CONFIG_GM_LOG_TRADE))
    {
        sLog->outCommand(player->GetSession()->GetAccountId(),
            "GM %s (Account: %u) deposit money (Amount: %u) to pGuild bank (Guild ID %u)",
            player->GetName(), player->GetSession()->GetAccountId(), amount, m_id);
    }
    sLog->outChar("IP:(%s) account:(%u) character:(%s) action:(%s) amount:(%u) guild_id:(%u)",
                 player->GetSession()->GetRemoteAddress().c_str(),
                 player->GetSession()->GetAccountId(),
                 player->GetName(),
                 "guildbank deposit money",
                 amount,
                 m_id);

    // Log guild bank event
    _LogBankEvent(trans, GUILD_BANK_LOG_DEPOSIT_MONEY, uint8(0), player->GetGUIDLow(), amount);

    CharacterDatabase.CommitTransaction(trans);

    std::string aux = ByteArrayToHexStr(reinterpret_cast<uint8*>(&amount), 8, true);
    _BroadcastEvent(GE_BANK_MONEY_CHANGED, 0, aux.c_str());
}

bool Guild::HandleMemberWithdrawMoney(WorldSession* session, uint32 amount, bool repair)
{
    if (!_GetPurchasedTabsSize())
        return false;                                       // No guild bank tabs - no money

    if (m_bankMoney < amount)                               // Not enough money in bank
        return false;

    Player* player = session->GetPlayer();
    if (!_HasRankRight(player, repair ? GR_RIGHT_WITHDRAW_REPAIR : GR_RIGHT_WITHDRAW_GOLD))
        return false;

    uint32 remainingMoney = _GetMemberRemainingMoney(player->GetGUID());
    if (!remainingMoney)
        return false;

    if (remainingMoney < amount)
        return false;

    // Call script after validation and before money transfer.
    sScriptMgr->OnGuildMemberWitdrawMoney(this, player, amount, repair);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    // Update remaining money amount
    if (remainingMoney < uint32(GUILD_WITHDRAW_MONEY_UNLIMITED))
        if (Member* pMember = GetMember(player->GetGUID()))
            pMember->DecreaseBankRemainingValue(trans, GUILD_BANK_MAX_TABS, amount);
    // Remove money from bank
    _ModifyBankMoney(trans, amount, false);
    // Add money to player (if required)
    if (!repair)
    {
        player->ModifyMoney(amount);
        player->SaveGoldToDB(trans);
    }
    // Log guild bank event
    _LogBankEvent(trans, repair ? GUILD_BANK_LOG_REPAIR_MONEY : GUILD_BANK_LOG_WITHDRAW_MONEY, uint8(0), player->GetGUIDLow(), amount);
    CharacterDatabase.CommitTransaction(trans);

    sLog->outChar("IP:(%s) account:(%u) character:(%s) action:(%s) amount:(%u) guild_id:(%u)",
                 player->GetSession()->GetRemoteAddress().c_str(),
                 player->GetSession()->GetAccountId(),
                 player->GetName(),
                 "guildbank withdraw money",
                 amount,
                 m_id);

    std::string aux = ByteArrayToHexStr(reinterpret_cast<uint8*>(&amount), 8, true);
    _BroadcastEvent(GE_BANK_MONEY_CHANGED, 0, aux.c_str());

    return true;
}

void Guild::HandleMemberLogout(WorldSession* session)
{
    Player* player = session->GetPlayer();
    if (Member* pMember = GetMember(player->GetGUID()))
    {
        pMember->SetStats(player);
        pMember->UpdateLogoutTime();
        pMember->ResetFlags();
    }
    _BroadcastEvent(GE_SIGNED_OFF, player->GetGUID(), player->GetName());
}

void Guild::HandleDisband(WorldSession* session)
{
    // Only leader can disband guild
    if (!_IsLeader(session->GetPlayer()))
        Guild::SendCommandResult(session, GUILD_COMMAND_CREATE, ERR_GUILD_PERMISSIONS);
    else
    {
        Disband();
        sLog->outDebug("WORLD: Guild Successfully Disbanded");
    }
}

void Guild::HandleGuildPartyRequest(WorldSession* session)
{
    Player *player = session->GetPlayer();
    Group *group = player->GetGroup();

    // Make sure player is a member of the guild and that he is in a group.
    if (!IsMember(player->GetGUID()) || !group)
        return;

    WorldPacket data(SMSG_GUILD_PARTY_STATE_RESPONSE, 13);
    data.WriteBit(group->IsGuildGroup(player->GetGuildId()));            // Is guild group
    data.FlushBits();
    data << float(group->GetGuildProfitCoef(player->GetGuildId()));      // Guild XP multiplier
    data << uint32(group->GetGuildMembersCount(player->GetGuildId()));   // Current guild members

    uint32 neededcount = 0;
    if (group->isRaidGroup())
    {
        if (group->GetRaidDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL || group->GetRaidDifficulty() == RAID_DIFFICULTY_10MAN_HEROIC)
            neededcount = GROUP_MEMBERS_10MAN_PROFIT;
        else
            neededcount = GROUP_MEMBERS_25MAN_PROFIT;
    }
    else if (group->isBGGroup())
    {
        if (sBattlegroundMgr->GetRatedBattlegroundSize() == 10)
            neededcount = GROUP_MEMBERS_10MAN_PROFIT;
        else
            neededcount = 12; // TODO: verify
    }
    else
        neededcount = GROUP_MEMBERS_DUNGEON_PROFIT;

    data << uint32(neededcount);                                         // Needed guild members

    session->SendPacket(&data);
}

///////////////////////////////////////////////////////////////////////////////
// Send data to client
void Guild::SendInfo(WorldSession* session) const
{
    WorldPacket data(SMSG_GUILD_INFO, m_name.size() + 4 + 4 + 4);
    data << m_name;
    data << secsToTimeBitFields(m_createdDate);     // 3.x (prev. year + month + day)
    data << uint32(m_members.size());               // Number of members
    data << m_accountsNumber;                       // Number of accounts

    session->SendPacket(&data);
    sLog->outDebug("WORLD: Sent (SMSG_GUILD_INFO)");
}

void Guild::SendEventLog(WorldSession *session) const
{
    WorldPacket data(SMSG_GUILD_EVENT_LOG_QUERY_RESULT, 1 + m_eventLog->GetSize() * (1 + 8 + 4));
    m_eventLog->WritePacket(data);
    session->SendPacket(&data);
    sLog->outDebug("WORLD: Sent (MSG_GUILD_EVENT_LOG_QUERY)");
}

void Guild::SendBankLog(WorldSession *session, uint8 tabId) const
{
    // GUILD_BANK_MAX_TABS send by client for money log
    if (tabId < _GetPurchasedTabsSize() || tabId == GUILD_BANK_MAX_TABS)
    {
        const LogHolder* pLog = m_bankEventLog[tabId];
        WorldPacket data(SMSG_GUILD_BANK_LOG_QUERY_RESULT, pLog->GetSize() * (4 * 4 + 1) + 1 + 1);
        data.WriteBit(GetLevel() >= 5 && tabId == GUILD_BANK_MAX_TABS);     // has Cash Flow perk
        pLog->WritePacket(data);
        data << uint32(tabId);
        session->SendPacket(&data);
        sLog->outDebug("WORLD: Sent (SMSG_GUILD_BANK_LOG_QUERY_RESULT)");
    }
}

void Guild::SendBankList(WorldSession *session, uint8 tabId, bool withContent, bool withTabInfo) const
{
    Member const *member = GetMember(session->GetPlayer()->GetGUID());
    if (!member) // Shouldn't happen, just in case
        return;

    ByteBuffer tabData;
    WorldPacket data(SMSG_GUILD_BANK_LIST, 500);
    data.WriteBit(0);
    uint32 itemCount = 0;
    if (withContent && _MemberHasTabRights(session->GetPlayer()->GetGUID(), tabId, GUILD_BANK_RIGHT_VIEW_TAB))
        if (BankTab const* tab = GetBankTab(tabId))
            for (uint8 slotId = 0; slotId < GUILD_BANK_MAX_SLOTS; ++slotId)
                if (tab->GetItem(slotId))
                    ++itemCount;

    data.WriteBits(itemCount, 20);
    data.WriteBits(withTabInfo ? _GetPurchasedTabsSize() : 0, 22);
    if (withContent && _MemberHasTabRights(session->GetPlayer()->GetGUID(), tabId, GUILD_BANK_RIGHT_VIEW_TAB))
    {
        if (BankTab const* tab = GetBankTab(tabId))
        {
            for (uint8 slotId = 0; slotId < GUILD_BANK_MAX_SLOTS; ++slotId)
            {
                if (Item* tabItem = tab->GetItem(slotId))
                {
                    data.WriteBit(0);

                    uint32 enchants = 0;
                    for (uint32 ench = 0; ench < MAX_ENCHANTMENT_SLOT; ++ench)
                    {
                        if (uint32 enchantId = tabItem->GetEnchantmentId(EnchantmentSlot(ench)))
                        {
                            tabData << uint32(enchantId);
                            tabData << uint32(ench);
                            ++enchants;
                        }
                    }

                    data.WriteBits(enchants, 23);

                    tabData << uint32(0);
                    tabData << uint32(0);
                    tabData << uint32(0);
                    tabData << uint32(tabItem->GetCount());                 // ITEM_FIELD_STACK_COUNT
                    tabData << uint32(slotId);
                    tabData << uint32(0);
                    tabData << uint32(tabItem->GetEntry());
                    tabData << uint32(tabItem->GetItemRandomPropertyId());
                    tabData << uint32(abs(tabItem->GetSpellCharges()));     // Spell charges
                    tabData << uint32(tabItem->GetItemSuffixFactor());      // SuffixFactor
                }
            }
        }
    }

    if (withTabInfo)
    {
        for (uint8 i = 0; i < _GetPurchasedTabsSize(); ++i)
        {
            data.WriteBits(m_bankTabs[i]->GetIcon().length(), 9);
            data.WriteBits(m_bankTabs[i]->GetName().length(), 7);
        }
    }

    data.FlushBits();

    if (withTabInfo)
    {
        for (uint8 i = 0; i < _GetPurchasedTabsSize(); ++i)
        {
            data.WriteString(m_bankTabs[i]->GetIcon());
            data << uint32(i);
            data.WriteString(m_bankTabs[i]->GetName());
        }
    }

    data << uint64(m_bankMoney);
    if (!tabData.empty())
        data.append(tabData);

    data << uint32(tabId);
    data << uint32(_GetMemberRemainingSlots(member->GetGUID(), tabId));

    session->SendPacket(&data);

    sLog->outDebug("WORLD: Sent (SMSG_GUILD_BANK_LIST)");
}

void Guild::SendGuildRanksUpdate(uint64 setterGuid, uint64 targetGuid, uint32 rank)
{
    ObjectGuid tarGuid = targetGuid;
    ObjectGuid setGuid = setterGuid;

    Member* member = GetMember(targetGuid);
    ASSERT(member);

    WorldPacket data(SMSG_GUILD_RANKS_UPDATE, 100);
    data.WriteBit(setGuid[7]);
    data.WriteBit(setGuid[2]);
    data.WriteBit(tarGuid[2]);
    data.WriteBit(setGuid[1]);
    data.WriteBit(tarGuid[1]);
    data.WriteBit(tarGuid[7]);
    data.WriteBit(tarGuid[0]);
    data.WriteBit(tarGuid[5]);
    data.WriteBit(tarGuid[4]);
    data.WriteBit(rank < member->GetRankId()); // 1 == higher, 0 = lower?
    data.WriteBit(setGuid[5]);
    data.WriteBit(setGuid[0]);
    data.WriteBit(tarGuid[6]);
    data.WriteBit(setGuid[3]);
    data.WriteBit(setGuid[6]);
    data.WriteBit(tarGuid[3]);
    data.WriteBit(setGuid[4]);

    data.FlushBits();

    data << uint32(rank);
    data.WriteByteSeq(setGuid[3]);
    data.WriteByteSeq(tarGuid[7]);
    data.WriteByteSeq(setGuid[6]);
    data.WriteByteSeq(setGuid[2]);
    data.WriteByteSeq(tarGuid[5]);
    data.WriteByteSeq(tarGuid[0]);
    data.WriteByteSeq(setGuid[7]);
    data.WriteByteSeq(setGuid[5]);
    data.WriteByteSeq(tarGuid[2]);
    data.WriteByteSeq(tarGuid[1]);
    data.WriteByteSeq(setGuid[0]);
    data.WriteByteSeq(setGuid[4]);
    data.WriteByteSeq(setGuid[1]);
    data.WriteByteSeq(tarGuid[3]);
    data.WriteByteSeq(tarGuid[6]);
    data.WriteByteSeq(tarGuid[4]);
    BroadcastPacket(&data);

    member->ChangeRank(rank);

    sLog->outDebug("SMSG_GUILD_RANKS_UPDATE [Broadcast] Target: %u, Issuer: %u, RankId: %u",
        GUID_LOPART(targetGuid), GUID_LOPART(setterGuid), rank);
}

void Guild::SendBankTabsInfo(WorldSession *session) const
{
    WorldPacket data(SMSG_GUILD_BANK_LIST, 500);

    data << uint64(m_bankMoney);
    data << uint8(0);                                       // TabInfo packet must be for tabId 0
    data << uint32(_GetMemberRemainingSlots(session->GetPlayer()->GetGUID(), 0));
    data << uint8(1);                                       // Tell client that this packet includes tab info

    data << uint8(_GetPurchasedTabsSize());                  // Number of tabs
    for (uint8 i = 0; i < _GetPurchasedTabsSize(); ++i)
        m_bankTabs[i]->WriteInfoPacket(data);

    data << uint8(0);                                       // Do not send tab content
    session->SendPacket(&data);

    sLog->outDebug("WORLD: Sent (SMSG_GUILD_BANK_LIST)");
}

void Guild::SendBankTabText(WorldSession *session, uint8 tabId) const
{
    if (const BankTab* pTab = GetBankTab(tabId))
        pTab->SendText(this, session);
}

void Guild::SendPermissions(WorldSession *session) const
{
    Member const *member = GetMember(session->GetPlayer()->GetGUID());
    if (!member)
        return;

    uint8 rankId = member->GetRankId();

    WorldPacket data(SMSG_GUILD_PERMISSIONS_QUERY_RESULTS, 4 * 15 + 1);
    data << uint32(rankId);
    data << uint32(_GetPurchasedTabsSize());

    if (rankId == 0)
        data << uint32(-1);
    else
        data << uint32(_GetRankRights(rankId));

    data << uint32(_GetMemberRemainingMoney(member));
    data.WriteBits(GUILD_BANK_MAX_TABS, 23);
    for (uint8 tabId = 0; tabId < GUILD_BANK_MAX_TABS; ++tabId)
    {
        if (rankId == 0)
        {
            data << uint32(-1);
            data << uint32(-1);
        }
        else
        {
            data << uint32(_GetRankBankTabRights(rankId, tabId));
            data << uint32(_GetMemberRemainingSlots(member->GetGUID(), tabId));
        }
    }

    session->SendPacket(&data);
    sLog->outDebug("WORLD: Sent (MSG_GUILD_PERMISSIONS)");
}

void Guild::SendMoneyInfo(WorldSession *session) const
{
    WorldPacket data(SMSG_GUILD_BANK_MONEY_WITHDRAWN, 4);
    data << uint64(_GetMemberRemainingMoney(session->GetPlayer()->GetGUID()));
    session->SendPacket(&data);
    sLog->outDebug("WORLD: Sent MSG_GUILD_BANK_MONEY_WITHDRAWN");
}

void Guild::SendLoginInfo(WorldSession* session)
{
    Player *player = session->GetPlayer();
    Member *member = GetMember(player->GetGUID());
    if (!member)
        return;

    /*
        Login sequence:
          SMSG_GUILD_EVENT - GE_MOTD
          SMSG_GUILD_RANK
          SMSG_GUILD_EVENT - GE_SIGNED_ON
          -- learn perks
          SMSG_GUILD_REPUTATION_WEEKLY_CAP
          SMSG_GUILD_ACHIEVEMENT_DATA
          SMSG_GUILD_MEMBER_DAILY_RESET // bank withdrawal reset
    */

    WorldPacket data(SMSG_GUILD_EVENT, 1 + 1 + m_motd.size() + 1);
    data << uint8(GE_MOTD);
    data << uint8(1);
    data << m_motd;
    session->SendPacket(&data);
    sLog->outDebug("WORLD: Sent guild MOTD (SMSG_GUILD_EVENT)");

    SendBankList(session, 0, true, true);
    _BroadcastEvent(GE_SIGNED_ON, player->GetGUID(), player->GetName());

    // Send to self separately, player is not in world yet and is not found by _BroadcastEvent
    data.Initialize(SMSG_GUILD_EVENT, 1 + 1 + strlen(player->GetName()) + 8);
    data << uint8(GE_SIGNED_ON);
    data << uint8(1);
    data << player->GetName();
    data << uint64(player->GetGUID());
    session->SendPacket(&data);

    data.Initialize(SMSG_GUILD_MEMBER_DAILY_RESET, 0);  // tells the client to request bank withdrawal limit
    session->SendPacket(&data);

    //SendGuildReputationWeeklyCap(session, member->GetWeekReputation());
    SendGuildReputationWeeklyCap(session, 10);

    m_achievementMgr.SendAllAchievementData(player);

    member->SetStats(player);
    member->AddFlag(GUILD_MEMBER_FLAG_ONLINE);
}

///////////////////////////////////////////////////////////////////////////////
// Loading methods
bool Guild::LoadFromDB(Field* fields)
{
/*
 //          0          1       2             3              4              5              6
 "SELECT g.guildid, g.name, g.leaderguid, g.EmblemStyle, g.EmblemColor, g.BorderStyle, g.BorderColor,"
 //   7                  8       9       10            11           12                  13  14
 "g.BackgroundColor, g.info, g.motd, g.createdate, g.BankMoney, COUNT(gbt.guildid), xp, level "
 "FROM guild g LEFT JOIN guild_bank_tab gbt ON g.guildid = gbt.guildid GROUP BY g.guildid ORDER BY g.guildid ASC", CONNECTION_SYNCH);
*/
    m_id            = fields[0].GetUInt32();
    m_name          = fields[1].GetString();
    m_leaderGuid    = MAKE_NEW_GUID(fields[2].GetUInt32(), 0, HIGHGUID_PLAYER);
    m_emblemInfo.LoadFromDB(fields);
    m_info          = fields[8].GetString();
    m_motd          = fields[9].GetString();
    m_createdDate   = fields[10].GetUInt64();
    m_bankMoney     = fields[11].GetUInt64();

    uint8 purchasedTabs = uint8(fields[12].GetUInt32());
    if (purchasedTabs > GUILD_BANK_MAX_TABS)
        purchasedTabs = GUILD_BANK_MAX_TABS;

    m_bankTabs.resize(purchasedTabs);
    for (uint8 i = 0; i < purchasedTabs; ++i)
        m_bankTabs[i] = new BankTab(m_id, i);

    m_xp = fields[13].GetUInt64();
    m_level = fields[14].GetUInt32();
    if (m_level == 0)
        m_level = 1;

    m_nextLevelXP = sObjectMgr->GetXPForGuildLevel(m_level);

    m_todayXP = 0;

    QueryResult xpcapquery = CharacterDatabase.PQuery("SELECT today_xp FROM guild_advancement WHERE guildid = '%u'",m_id);
    if (xpcapquery && xpcapquery->GetRowCount() > 0)
    {
        Field* xpfields = xpcapquery->Fetch();
        if (xpfields)
        {
            // Load today XP cap to variable
            m_todayXP = xpfields[0].GetUInt64();
        }
    }

    QueryResult newsquery = CharacterDatabase.PQuery("SELECT id, event_type, param, date, playerguid FROM guild_news WHERE guildid = '%u' ORDER BY id DESC LIMIT 100;", m_id);
    if (newsquery && newsquery->GetRowCount() > 0)
    {
        uint32 tmptype;
        do
        {
            Field* newsfields = newsquery->Fetch();

            tmptype = newsfields[1].GetUInt32();
            if (tmptype >= GUILD_NEWS_MAX)
                continue;

            GuildNewsEntry gn;

            gn.id = newsfields[0].GetUInt32();
            gn.type = (GuildNewsType)tmptype;
            gn.param = newsfields[2].GetUInt64();
            gn.date = newsfields[3].GetUInt32();
            gn.guid = MAKE_NEW_GUID(newsfields[4].GetUInt32(), 0, HIGHGUID_PLAYER);

            m_guildNews.push_back(gn);
        } while (newsquery->NextRow());
    }

    QueryResult challengequery = CharacterDatabase.PQuery("SELECT dungeon, raid, battleground FROM guild_week_challenge WHERE guildid = %u;", m_id);
    if (challengequery && challengequery->GetRowCount() > 0)
    {
        Field* chfields = challengequery->Fetch();
        if (chfields)
        {
            m_guildChallenges[GUILD_CHALLENGE_DUNGEON-1].count = chfields[0].GetUInt16();
            m_guildChallenges[GUILD_CHALLENGE_RAID-1].count = chfields[1].GetUInt16();
            m_guildChallenges[GUILD_CHALLENGE_BG-1].count = chfields[2].GetUInt16();
        }
    }

    // id, type, date, param, guid

    _CreateLogHolders();
    return true;
}

bool Guild::LoadRankFromDB(Field* fields)
{
    RankInfo rankInfo(m_id);
    if (!rankInfo.LoadFromDB(fields))
        return false;
    m_ranks.push_back(rankInfo);
    return true;
}

bool Guild::LoadMemberFromDB(Field* fields)
{
    uint32 lowguid = fields[1].GetUInt32();
    Member *pMember = new Member(m_id, MAKE_NEW_GUID(lowguid, 0, HIGHGUID_PLAYER), fields[2].GetUInt8());
    if (!pMember->LoadFromDB(fields))
    {
        _DeleteMemberFromDB(lowguid);
        delete pMember;
        return false;
    }
    m_members[lowguid] = pMember;
    return true;
}

bool Guild::LoadBankRightFromDB(Field* fields)
{
                                           // rights             slots
    GuildBankRightsAndSlots rightsAndSlots(fields[3].GetUInt8(), fields[4].GetUInt32());
                                  // rankId             tabId
    _SetRankBankTabRightsAndSlots(fields[2].GetUInt8(), fields[1].GetUInt8(), rightsAndSlots, false);
    return true;
}

bool Guild::LoadEventLogFromDB(Field* fields)
{
    /*
     //          0        1        2          3            4            5        6
     "SELECT guildid, LogGuid, EventType, PlayerGuid1, PlayerGuid2, NewRank, TimeStamp FROM guild_eventlog ORDER BY TimeStamp DESC, LogGuid DESC", CONNECTION_SYNCH);
    */
    
    if (m_eventLog->CanInsert())
    {
        m_eventLog->LoadEvent(new EventLogEntry(
            m_id,                                       // guild id
            fields[1].GetUInt32(),                      // guid
            fields[6].GetUInt32(),                      // timestamp //64 bits?
            GuildEventLogTypes(fields[2].GetUInt8()),   // event type
            fields[3].GetUInt32(),                      // player guid 1
            fields[4].GetUInt32(),                      // player guid 2
            fields[5].GetUInt8()));                     // rank
        return true;
    }
    return false;
}

bool Guild::LoadBankEventLogFromDB(Field* fields)
{
    /*
     //          0        1      2        3          4           5            6               7          8
     "SELECT guildid, TabId, LogGuid, EventType, PlayerGuid, ItemOrMoney, ItemStackCount, DestTabId, TimeStamp FROM guild_bank_eventlog ORDER BY TimeStamp DESC, LogGuid DESC", 
     */
    uint8 dbTabId = fields[1].GetUInt8();
    bool isMoneyTab = (dbTabId == GUILD_BANK_MONEY_LOGS_TAB);
    if (dbTabId < _GetPurchasedTabsSize() || isMoneyTab)
    {
        uint8 tabId = isMoneyTab ? uint8(GUILD_BANK_MAX_TABS) : dbTabId;
        LogHolder* pLog = m_bankEventLog[tabId];
        if (pLog->CanInsert())
        {
            uint32 guid = fields[2].GetUInt32();
            GuildBankEventLogTypes eventType = GuildBankEventLogTypes(fields[3].GetUInt8());
            if (BankEventLogEntry::IsMoneyEvent(eventType))
            {
                if (!isMoneyTab)
                {
                    sLog->outError("GuildBankEventLog ERROR: MoneyEvent(LogGuid: %u, Guild: %u) does not belong to money tab (%u), ignoring...", guid, m_id, dbTabId);
                    return false;
                }
            }
            else if (isMoneyTab)
            {
                sLog->outError("GuildBankEventLog ERROR: non-money event (LogGuid: %u, Guild: %u) belongs to money tab, ignoring...", guid, m_id);
                return false;
            }
            pLog->LoadEvent(new BankEventLogEntry(
                m_id,                                   // guild id
                guid,                                   // guid
                fields[8].GetUInt32(),                  // timestamp //64 bits?
                dbTabId,                                // tab id
                eventType,                              // event type
                fields[4].GetUInt32(),                  // player guid
                fields[5].GetUInt32(),                  // item or money
                fields[6].GetUInt16(),                  // itam stack count
                fields[7].GetUInt8()));                 // dest tab id
        }
    }
    return true;
}

bool Guild::LoadBankTabFromDB(Field* fields)
{
    uint32 tabId = fields[1].GetUInt8();
    if (tabId >= _GetPurchasedTabsSize())
    {
        sLog->outError("Invalid tab (tabId: %u) in guild bank, skipped.", tabId);
        return false;
    }
    return m_bankTabs[tabId]->LoadFromDB(fields);
}

bool Guild::LoadBankItemFromDB(Field* fields)
{
    uint8 tabId = fields[11].GetUInt8();
    if (tabId >= _GetPurchasedTabsSize())
    {
        sLog->outError("Invalid tab for item (GUID: %u, id: #%u) in guild bank, skipped.", 
            fields[13].GetUInt32(), fields[14].GetUInt32());
        return false;
    }
    return m_bankTabs[tabId]->LoadItemFromDB(fields);
}

// Validates guild data loaded from database. Returns false if guild should be deleted.
bool Guild::Validate()
{
    // Validate ranks data
    // GUILD RANKS represent a sequence starting from 0 = GUILD_MASTER (ALL PRIVILEGES) to max 9 (lowest privileges).
    // The lower rank id is considered higher rank - so promotion does rank-- and demotion does rank++
    // Between ranks in sequence cannot be gaps - so 0,1,2,4 is impossible
    // Min ranks count is 5 and max is 10.
    bool broken_ranks = false;
    if (_GetRanksSize() < GUILD_RANKS_MIN_COUNT || _GetRanksSize() > GUILD_RANKS_MAX_COUNT)
    {
        sLog->outError("Guild %u has invalid number of ranks, creating new...", m_id);
        broken_ranks = true;
    }
    else
    {
        for (uint8 rankId = 0; rankId < _GetRanksSize(); ++rankId)
        {
            RankInfo* rankInfo = GetRankInfo(rankId);
            if (rankInfo->GetId() != rankId)
            {
                sLog->outError("Guild %u has broken rank id %u, creating default set of ranks...", m_id, rankId);
                broken_ranks = true;
            }
        }
    }
    if (broken_ranks)
    {
        m_ranks.clear();
        _CreateDefaultGuildRanks(DEFAULT_LOCALE);
    }

    // Validate members' data
    for (Members::iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        if (itr->second->GetRankId() > _GetRanksSize())
            itr->second->ChangeRank(_GetLowestRankId());

    // Repair the structure of the guild.
    // If the guildmaster doesn't exist or isn't member of the guild
    // attempt to promote another member.
    Member* pLeader = GetMember(m_leaderGuid);
    if (!pLeader)
    {
        DeleteMember(m_leaderGuid);
        // If no more members left, disband guild
        if (m_members.empty())
        {
            Disband();
            return false;
        }
    }
    else if (!pLeader->IsRank(GR_GUILDMASTER))
        _SetLeaderGUID(pLeader);

    // Check config if multiple guildmasters are allowed
    if (!sConfig->GetBoolDefault("Guild.AllowMultipleGuildMaster", 0))
        for (Members::iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
            if (itr->second->GetRankId() == GR_GUILDMASTER && !itr->second->IsSamePlayer(m_leaderGuid))
                itr->second->ChangeRank(GR_OFFICER);

    _UpdateAccountsNumber();
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Broadcasts
void Guild::BroadcastToGuild(WorldSession *session, bool officerOnly, const std::string& msg, uint32 language, const char* addonPrefix) const
{
    if (session && session->GetPlayer() && _HasRankRight(session->GetPlayer(), officerOnly ? GR_RIGHT_OFFCHATSPEAK : GR_RIGHT_GCHATSPEAK))
    {
        WorldPacket data;
        ChatHandler::FillMessageData(&data, session, officerOnly ? CHAT_MSG_OFFICER : CHAT_MSG_GUILD, language, NULL, 0, msg.c_str(), NULL, addonPrefix);
        for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
            if (Player *player = itr->second->FindPlayer())
                if (player->GetSession() && _HasRankRight(player, officerOnly ? GR_RIGHT_OFFCHATLISTEN : GR_RIGHT_GCHATLISTEN) && 
                    !player->GetSocial()->HasIgnore(session->GetPlayer()->GetGUIDLow()))
                    player->GetSession()->SendPacket(&data);
    }
}

void Guild::BroadcastPacketToRank(WorldPacket *packet, uint8 rankId) const
{
    for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        if (itr->second->IsRank(rankId))
            if (Player *player = itr->second->FindPlayer())
                player->GetSession()->SendPacket(packet);
}

void Guild::BroadcastPacket(WorldPacket *packet) const
{
    for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        if (Player *player = itr->second->FindPlayer())
            player->GetSession()->SendPacket(packet);
}

///////////////////////////////////////////////////////////////////////////////
// Members handling
bool Guild::AddMember(const uint64& guid, uint8 rankId)
{
    Player* player = sObjectMgr->GetPlayer(guid);
    // Player cannot be in guild
    if (player)
    {
        if (player->GetGuildId() != 0)
            return false;
    }
    else if (Player::GetGuildIdFromDB(guid) != 0)
        return false;

    // Remove all player signs from another petitions
    // This will be prevent attempt to join many guilds and corrupt guild data integrity
    Player::RemovePetitionsAndSigns(guid, GUILD_CHARTER_TYPE);

    uint32 lowguid = GUID_LOPART(guid);

    // If rank was not passed, assing lowest possible rank
    if (rankId == GUILD_RANK_NONE)
        rankId = _GetLowestRankId();

    Member* pMember = new Member(m_id, guid, rankId);
    if (player)
    {
        pMember->AddFlag(GUILD_MEMBER_FLAG_ONLINE);
        pMember->SetStats(player);

        // If player wasn't in this guild before, or his leave time is greater than one month, reduce reputation
        if (player->m_lastGuildId != m_id || player->m_guildLeaveTime < (time(NULL) - GUILD_REPUTATION_KICK_TIME_LIMIT))
        {
            ReputationRank currRank = player->GetReputationRank(FACTION_GUILD);
            if (currRank > REP_NEUTRAL)
                player->SetReputation(FACTION_GUILD, ReputationMgr::ReputationAmountRankRange(REP_NEUTRAL, (ReputationRank)(currRank - 1)));
            else
                player->SetReputation(FACTION_GUILD, 0);
        }

        // Learn our perks to him
        for(int i = 0; i < m_level-1; ++i)
            if(const GuildPerksEntry* perk = sGuildPerksStore.LookupEntry(i))
                player->learnSpell(perk->SpellId, true);
    }
    else
    {
        pMember->ResetFlags();

        bool ok = false;
        // Player must exist
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_CHAR_DATA_FOR_GUILD);
        stmt->setUInt32(0, lowguid);
        if (PreparedQueryResult result = CharacterDatabase.Query(stmt))
        {
            Field *fields = result->Fetch();
            pMember->SetStats(
                fields[0].GetString(), 
                fields[1].GetUInt8(), 
                fields[2].GetUInt8(),
                fields[3].GetUInt32(), 
                fields[4].GetUInt32());

            ok = pMember->CheckStats();
        }
        if (!ok)
        {
            delete pMember;
            return false;
        }
    }
    m_members[lowguid] = pMember;

    if (m_members.size() > 1)
        HandleRoster();

    SQLTransaction trans(NULL);
    pMember->SaveToDB(trans);
    // If player not in game data in will be loaded from guild tables, so no need to update it!
    if (player)
    {
        player->SetInGuild(m_id);
        player->SetRank(rankId);
        player->SetGuildIdInvited(0);
    }

    UpdateMemberInDB(guid);

    _UpdateAccountsNumber();

    // Call scripts if member was succesfully added (and stored to database)
    sScriptMgr->OnGuildAddMember(this, player, rankId);

    return true;
}

void Guild::DeleteMember(const uint64& guid, bool isDisbanding, bool isKicked)
{
    uint32 lowguid = GUID_LOPART(guid);
    Player *player = sObjectMgr->GetPlayer(guid);

    // Guild master can be deleted when loading guild and guid doesn't exist in characters table
    // or when he is removed from guild by gm command
    if (m_leaderGuid == guid && !isDisbanding)
    {
        Member* oldLeader = NULL;
        Member* newLeader = NULL;
        for (Guild::Members::iterator i = m_members.begin(); i != m_members.end(); ++i)
        {
            if (i->first == lowguid)
                oldLeader = i->second;
            else if (!newLeader || newLeader->GetRankId() > i->second->GetRankId())
                newLeader = i->second;
        }
        if (!newLeader)
        {
            Disband();
            return;
        }

        _SetLeaderGUID(newLeader);

        // If player not online data in data field will be loaded from guild tabs no need to update it !!
        if (Player *newLeaderPlayer = newLeader->FindPlayer())
            newLeaderPlayer->SetRank(GR_GUILDMASTER);

        // If leader does not exist (at guild loading with deleted leader) do not send broadcasts
        if (oldLeader)
        {
            _BroadcastEvent(GE_LEADER_CHANGED, 0, oldLeader->GetName().c_str(), newLeader->GetName().c_str());
            _BroadcastEvent(GE_LEFT, guid, oldLeader->GetName().c_str());
        }
    }
    // Call script on remove before member is acutally removed from guild (and database)
    sScriptMgr->OnGuildRemoveMember(this, player, isDisbanding, isKicked);

    if (Member* pMember = GetMember(guid))
        delete pMember;
    m_members.erase(lowguid);

    if (!m_members.empty())
        HandleRoster();

    // If player not online data in data field will be loaded from guild tabs no need to update it !!
    if (player)
    {
        player->SetInGuild(0);
        player->SetRank(0);

        // Delete guild perks
        // remove all spells that related to this skill
        for (uint32 i = 0; i < sGuildPerksStore.GetNumRows(); ++i)
            if (GuildPerksEntry const *pAbility = sGuildPerksStore.LookupEntry(i))
                    player->removeSpell(pAbility->SpellId);
    }

    _DeleteMemberFromDB(lowguid);
    if (!isDisbanding)
        _UpdateAccountsNumber();
}

bool Guild::ChangeMemberRank(const uint64& guid, uint8 newRank)
{
    if (newRank <= _GetLowestRankId())                    // Validate rank (allow only existing ranks)
        if (Member* pMember = GetMember(guid))
        {
            pMember->ChangeRank(newRank);
            return true;
        }
    return false;
}

void Guild::SetMemberProfessionData(const uint64& guid, uint32 skillId, uint32 skillValue, uint32 title)
{
    if (Member* pMember = GetMember(guid))
        pMember->SetProfessionData(skillId, skillValue, title);
}

void Guild::Member::SetProfessionData(uint32 skillId, uint32 skillValue, uint32 title)
{
    for (uint32 i = 0; i < 2; i++)
    {
        if (professions[i].skillId == skillId)
        {
            professions[i].skillValue = skillValue;
            professions[i].title = title;
            return;
        }
    }

    // this means that profession hasn't been found
    for (uint32 i = 0; i < 2; i++)
    {
        if (professions[i].skillId == 0)
        {
            professions[i].skillId = skillId;
            professions[i].skillValue = skillValue;
            professions[i].title = title;
            return;
        }
    }
}

void Guild::RemoveMemberProfession(const uint64& guid, uint32 skillId)
{
    if (Member* pMember = GetMember(guid))
        pMember->RemoveProfession(skillId);
}

void Guild::Member::RemoveProfession(uint32 skillId)
{
    for (uint32 i = 0; i < 2; i++)
    {
        if (professions[i].skillId == skillId)
        {
            professions[i].skillId = 0;
            professions[i].skillValue = 0;
            professions[i].title = 0;
        }
    }
}

bool Guild::IsMember(uint64 guid) const
{
    Members::const_iterator itr = m_members.find(GUID_LOPART(guid));
    return itr != m_members.end();
}

///////////////////////////////////////////////////////////////////////////////
// Bank (items move)
void Guild::SwapItems(Player* player, uint8 tabId, uint8 slotId, uint8 destTabId, uint8 destSlotId, uint32 splitedAmount)
{
    if (tabId >= _GetPurchasedTabsSize() || slotId >= GUILD_BANK_MAX_SLOTS || 
        destTabId >= _GetPurchasedTabsSize() || destSlotId >= GUILD_BANK_MAX_SLOTS)
        return;

    if (tabId == destTabId && slotId == destSlotId)
        return;

    BankMoveItemData from(this, player, tabId, slotId);
    BankMoveItemData to(this, player, destTabId, destSlotId);
    _MoveItems(&from, &to, splitedAmount);
}

void Guild::SwapItemsWithInventory(Player* player, bool toChar, uint8 tabId, uint8 slotId, uint8 playerBag, uint8 playerSlotId, uint32 splitedAmount)
{
    if ((slotId >= GUILD_BANK_MAX_SLOTS && slotId != NULL_SLOT) || tabId >= _GetPurchasedTabsSize())
        return;

    BankMoveItemData bankData(this, player, tabId, slotId);
    PlayerMoveItemData charData(this, player, playerBag, playerSlotId);
    if (toChar)
        _MoveItems(&bankData, &charData, splitedAmount);
    else
        _MoveItems(&charData, &bankData, splitedAmount);
}

///////////////////////////////////////////////////////////////////////////////
// Bank tabs
void Guild::SetBankTabText(uint8 tabId, const std::string& text)
{
    if (BankTab* pTab = GetBankTab(tabId))
    {
        pTab->SetText(text);
        pTab->SendText(this, NULL);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Private methods
void Guild::_CreateLogHolders()
{
    m_eventLog = new LogHolder(m_id, sWorld->getIntConfig(CONFIG_GUILD_EVENT_LOG_COUNT));
    for (uint8 tabId = 0; tabId <= GUILD_BANK_MAX_TABS; ++tabId)
        m_bankEventLog[tabId] = new LogHolder(m_id, sWorld->getIntConfig(CONFIG_GUILD_BANK_EVENT_LOG_COUNT));
}

bool Guild::_CreateNewBankTab()
{
    if (_GetPurchasedTabsSize() >= GUILD_BANK_MAX_TABS)
        return false;

    uint8 tabId = _GetPurchasedTabsSize();                      // Next free id
    m_bankTabs.push_back(new BankTab(m_id, tabId));

    PreparedStatement* stmt = NULL;
    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_BANK_TAB);
    stmt->setUInt32(0, m_id);
    stmt->setUInt8 (1, tabId);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_ADD_GUILD_BANK_TAB);
    stmt->setUInt32(0, m_id);
    stmt->setUInt8 (1, tabId);
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
    return true;
}

void Guild::_CreateDefaultGuildRanks(LocaleConstant loc)
{
    PreparedStatement* stmt = NULL;

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_RANKS);
    stmt->setUInt32(0, m_id);
    CharacterDatabase.Execute(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GUILD_BANK_RIGHTS);
    stmt->setUInt32(0, m_id);
    CharacterDatabase.Execute(stmt);

    _CreateRank(sObjectMgr->GetTrinityString(LANG_GUILD_MASTER,   loc), GR_RIGHT_ALL);
    _CreateRank(sObjectMgr->GetTrinityString(LANG_GUILD_OFFICER,  loc), GR_RIGHT_ALL);
    _CreateRank(sObjectMgr->GetTrinityString(LANG_GUILD_VETERAN,  loc), GR_RIGHT_GCHATLISTEN | GR_RIGHT_GCHATSPEAK);
    _CreateRank(sObjectMgr->GetTrinityString(LANG_GUILD_MEMBER,   loc), GR_RIGHT_GCHATLISTEN | GR_RIGHT_GCHATSPEAK);
    _CreateRank(sObjectMgr->GetTrinityString(LANG_GUILD_INITIATE, loc), GR_RIGHT_GCHATLISTEN | GR_RIGHT_GCHATSPEAK);
}

bool Guild::_CreateRank(const std::string& name, uint32 rights)
{
    if (_GetRanksSize() >= GUILD_RANKS_MAX_COUNT)
        return false;

    // Ranks represent sequence 0,1,2,... where 0 means guildmaster
    uint8 newRankId = _GetRanksSize();

    RankInfo info(m_id, newRankId, name, rights, 0);
    m_ranks.push_back(info);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    for (uint8 i = 0; i < _GetPurchasedTabsSize(); ++i)
    {
        // Create bank rights with default values
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_ADD_GUILD_BANK_RIGHT_DEFAULT);
        stmt->setUInt32(0, m_id);
        stmt->setUInt8 (1, i);
        stmt->setUInt8 (2, newRankId);
        trans->Append(stmt);
    }
    info.SaveToDB(trans);
    CharacterDatabase.CommitTransaction(trans);

    return true;
}

// Updates the number of accounts that are in the guild
// Player may have many characters in the guild, but with the same account
void Guild::_UpdateAccountsNumber()
{
    // We use a set to be sure each element will be unique
    std::set<uint32> accountsIdSet;
    for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        accountsIdSet.insert(itr->second->GetAccountId());

    m_accountsNumber = accountsIdSet.size();
}

// Detects if player is the guild master.
// Check both leader guid and player's rank (otherwise the feature with 
// multiple guild masters won't work)
bool Guild::_IsLeader(Player* player) const
{
    if (player->GetGUID() == m_leaderGuid)
        return true;
    if (const Member* pMember = GetMember(player->GetGUID()))
        return pMember->IsRank(GR_GUILDMASTER);
    return false;
}

void Guild::_DeleteBankItems(SQLTransaction& trans, bool removeItemsFromDB)
{
    for (uint8 tabId = 0; tabId < _GetPurchasedTabsSize(); ++tabId)
    {
        m_bankTabs[tabId]->Delete(trans, removeItemsFromDB);
        delete m_bankTabs[tabId];
        m_bankTabs[tabId] = NULL;
    }
    m_bankTabs.clear();
}

bool Guild::_ModifyBankMoney(SQLTransaction& trans, const uint64& amount, bool add)
{
    if (add)
        m_bankMoney += amount;
    else
    {
        // Check if there is enough money in bank.
        if (m_bankMoney < amount)
            return false;
        m_bankMoney -= amount;
    }

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SET_GUILD_BANK_MONEY);
    stmt->setUInt64(0, m_bankMoney);
    stmt->setUInt32(1, m_id);
    trans->Append(stmt);
    return true;
}

void Guild::_SetLeaderGUID(Member* pLeader)
{
    if (!pLeader)
        return;

    m_leaderGuid = pLeader->GetGUID();
    pLeader->ChangeRank(GR_GUILDMASTER);

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SET_GUILD_LEADER);
    stmt->setUInt32(0, GUID_LOPART(m_leaderGuid));
    stmt->setUInt32(1, m_id);
    CharacterDatabase.Execute(stmt);
}

void Guild::_SetRankBankMoneyPerDay(uint8 rankId, uint32 moneyPerDay)
{
    if (RankInfo* rankInfo = GetRankInfo(rankId))
    {
        for (Members::iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
            if (itr->second->IsRank(rankId))
                itr->second->ResetMoneyTime();

        rankInfo->SetBankMoneyPerDay(moneyPerDay);
    }
}

void Guild::_SetRankBankTabRightsAndSlots(uint8 rankId, uint8 tabId, GuildBankRightsAndSlots rightsAndSlots, bool saveToDB)
{
    if (tabId >= _GetPurchasedTabsSize())
        return;

    if (RankInfo* rankInfo = GetRankInfo(rankId))
    {
        for (Members::iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
            if (itr->second->IsRank(rankId))
                itr->second->ResetTabTimes();

        rankInfo->SetBankTabSlotsAndRights(tabId, rightsAndSlots, saveToDB);
    }
}

inline std::string Guild::_GetRankName(uint8 rankId) const
{
    if (const RankInfo* rankInfo = GetRankInfo(rankId))
        return rankInfo->GetName();
    return "<unknown>";
}

inline uint32 Guild::_GetRankRights(uint8 rankId) const
{
    if (const RankInfo* rankInfo = GetRankInfo(rankId))
        return rankInfo->GetRights();
    return 0;
}

inline uint32 Guild::_GetRankBankMoneyPerDay(uint8 rankId) const
{
    if (const RankInfo* rankInfo = GetRankInfo(rankId))
        return rankInfo->GetBankMoneyPerDay();
    return 0;
}

inline uint32 Guild::_GetRankBankTabSlotsPerDay(uint8 rankId, uint8 tabId) const
{
    if (tabId < _GetPurchasedTabsSize())
        if (const RankInfo* rankInfo = GetRankInfo(rankId))
            return rankInfo->GetBankTabSlotsPerDay(tabId);
    return 0;
}

inline uint8 Guild::_GetRankBankTabRights(uint8 rankId, uint8 tabId) const
{
    if (const RankInfo* rankInfo = GetRankInfo(rankId))
        return rankInfo->GetBankTabRights(tabId);
    return 0;
}

inline uint32 Guild::_GetMemberRemainingSlots(const uint64& guid, uint8 tabId) const
{
    if (const Member* pMember = GetMember(guid))
        return pMember->GetBankRemainingValue(tabId, this);
    return 0;
}

inline uint32 Guild::_GetMemberRemainingMoney(const Member *member) const
{
    if (member)
        return member->GetBankRemainingValue(GUILD_BANK_MAX_TABS, this);
    return 0;
}

inline uint32 Guild::_GetMemberRemainingMoney(const uint64& guid) const
{
    if (const Member* pMember = GetMember(guid))
        return _GetMemberRemainingMoney(pMember);
    return 0;
}

inline void Guild::_DecreaseMemberRemainingSlots(SQLTransaction& trans, const uint64& guid, uint8 tabId)
{
    // Remaining slots must be more then 0
    if (uint32 remainingSlots = _GetMemberRemainingSlots(guid, tabId))
        // Ignore guild master
        if (remainingSlots < GUILD_WITHDRAW_SLOT_UNLIMITED)
            if (Member* pMember = GetMember(guid))
                pMember->DecreaseBankRemainingValue(trans, tabId, 1);
}

inline bool Guild::_MemberHasTabRights(const uint64& guid, uint8 tabId, uint32 rights) const
{
    if (const Member* pMember = GetMember(guid))
    {
        // Leader always has full rights
        if (pMember->IsRank(GR_GUILDMASTER) || m_leaderGuid == guid)
            return true;
        return (_GetRankBankTabRights(pMember->GetRankId(), tabId) & rights) == rights;
    }
    return false;
}

// Add new event log record
inline void Guild::_LogEvent(GuildEventLogTypes eventType, uint32 playerGuid1, uint32 playerGuid2, uint8 newRank)
{
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    m_eventLog->AddEvent(trans, new EventLogEntry(m_id, m_eventLog->GetNextGUID(), eventType, playerGuid1, playerGuid2, newRank));
    CharacterDatabase.CommitTransaction(trans);

    sScriptMgr->OnGuildEvent(this, uint8(eventType), playerGuid1, playerGuid2, newRank);
}

// Add new bank event log record
void Guild::_LogBankEvent(SQLTransaction& trans, GuildBankEventLogTypes eventType, uint8 tabId, uint32 lowguid, uint32 itemOrMoney, uint16 itemStackCount, uint8 destTabId)
{
    if (tabId > GUILD_BANK_MAX_TABS)
        return;

    uint8 dbTabId = tabId;
    if (BankEventLogEntry::IsMoneyEvent(eventType))
    {
        tabId = GUILD_BANK_MAX_TABS;
        dbTabId = GUILD_BANK_MONEY_LOGS_TAB;
    }
    LogHolder* pLog = m_bankEventLog[tabId];
    pLog->AddEvent(trans, new BankEventLogEntry(m_id, pLog->GetNextGUID(), eventType, dbTabId, lowguid, itemOrMoney, itemStackCount, destTabId));

    sScriptMgr->OnGuildBankEvent(this, uint8(eventType), tabId, lowguid, itemOrMoney, itemStackCount, destTabId);
}

inline Item* Guild::_GetItem(uint8 tabId, uint8 slotId) const
{
    if (const BankTab* tab = GetBankTab(tabId))
        return tab->GetItem(slotId);
    return NULL;
}

inline void Guild::_RemoveItem(SQLTransaction& trans, uint8 tabId, uint8 slotId)
{
    if (BankTab* pTab = GetBankTab(tabId))
        pTab->SetItem(trans, slotId, NULL);
}

void Guild::_MoveItems(MoveItemData* pSrc, MoveItemData* pDest, uint32 splitedAmount)
{
    // 1. Initialize source item
    if (!pSrc->InitItem())
        return; // No source item

    // 2. Check source item
    if (!pSrc->CheckItem(splitedAmount))
        return; // Source item or splited amount is invalid
    /*
    if (pItemSrc->GetCount() == 0)
    {
        sLog->outCrash("Guild::SwapItems: Player %s(GUIDLow: %u) tried to move item %u from tab %u slot %u to tab %u slot %u, but item %u has a stack of zero!",
            player->GetName(), player->GetGUIDLow(), pItemSrc->GetEntry(), tabId, slotId, destTabId, destSlotId, pItemSrc->GetEntry());
        //return; // Commented out for now, uncomment when it's verified that this causes a crash!!
    }
    //*/

    // 3. Check destination rights
    if (!pDest->HasStoreRights(pSrc))
        return; // Player has no rights to store item in destination

    // 4. Check source withdraw rights
    if (!pSrc->HasWithdrawRights(pDest))
        return; // Player has no rights to withdraw items from source

    // 5. Check split
    if (splitedAmount)
    {
        // 5.1. Clone source item
        if (!pSrc->CloneItem(splitedAmount))
            return; // Item could not be cloned

        // 5.2. Move splited item to destination
        _DoItemsMove(pSrc, pDest, true, splitedAmount);
    }
    else // 6. No split
    {
        // 6.1. Try to merge items in destination (pDest->GetItem() == NULL)
        if (!_DoItemsMove(pSrc, pDest, false)) // Item could not be merged
        {
            // 6.2. Try to swap items
            // 6.2.1. Initialize destination item
            if (!pDest->InitItem())
                return;

            // 6.2.2. Check rights to store item in source (opposite direction)
            if (!pSrc->HasStoreRights(pDest))
                return; // Player has no rights to store item in source (opposite direction)

            if (!pDest->HasWithdrawRights(pSrc))
                return; // Player has no rights to withdraw item from destination (opposite direction)

            // 6.2.3. Swap items (pDest->GetItem() != NULL)
            _DoItemsMove(pSrc, pDest, true);
        }
    }
    // 7. Send changes
    _SendBankContentUpdate(pSrc, pDest);
}

bool Guild::_DoItemsMove(MoveItemData* pSrc, MoveItemData* pDest, bool sendError, uint32 splitedAmount)
{
    Item* pDestItem = pDest->GetItem();
    bool swap = (pDestItem != NULL);

    Item* pSrcItem = pSrc->GetItem(splitedAmount);
    // 1. Can store source item in destination
    if (!pDest->CanStore(pSrcItem, swap, sendError))
        return false;

    // 2. Can store destination item in source
    if (swap)
        if (!pSrc->CanStore(pDestItem, true, true))
            return false;

    // GM LOG (TODO: move to scripts)
    pDest->LogAction(pSrc);
    if (swap)
        pSrc->LogAction(pDest);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    // 3. Log bank events
    pDest->LogBankEvent(trans, pSrc, pSrcItem->GetCount());
    if (swap)
        pSrc->LogBankEvent(trans, pDest, pDestItem->GetCount());

    // 4. Remove item from source
    pSrc->RemoveItem(trans, pDest, splitedAmount);

    // 5. Remove item from destination
    if (swap)
        pDest->RemoveItem(trans, pSrc);

    // 6. Store item in destination
    pDest->StoreItem(trans, pSrcItem);

    // 7. Store item in source
    if (swap)
        pSrc->StoreItem(trans, pDestItem);

    CharacterDatabase.CommitTransaction(trans);
    return true;
}

/*
void Guild::_SendBankContent(WorldSession *session, uint8 tabId) const
{
    const uint64& guid = session->GetPlayer()->GetGUID();
    if (_MemberHasTabRights(guid, tabId, GUILD_BANK_RIGHT_VIEW_TAB))
        if (const BankTab* pTab = GetBankTab(tabId))
        {
            WorldPacket data(SMSG_GUILD_BANK_LIST, 1200);

            data << uint64(m_bankMoney);
            data << uint8(tabId);
            data << uint32(_GetMemberRemainingSlots(guid, tabId));
            data << uint8(0);                                   // Tell client that there's no tab info in this packet

            pTab->WritePacket(data);

            session->SendPacket(&data);

            sLog->outDebug("WORLD: Sent (SMSG_GUILD_BANK_LIST)");
        }
}
*/

/*
void Guild::_SendBankMoneyUpdate(WorldSession *session) const
{
    WorldPacket data(SMSG_GUILD_BANK_LIST, 8 + 1 + 4 + 1 + 1);

    data.WriteBit(0);
    data.WriteBits(0, 20);                                            // Item count
    data.WriteBits(0, 22);                                            // Tab count

    data.FlushBits();

    data << uint64(m_bankMoney);

    data << uint8(0);                                                 // tabId, default 0

    data << uint32(_GetMemberRemainingSlots(session->GetPlayer()->GetGUID(), 0));

    BroadcastPacket(&data);

    sLog->outDebug("WORLD: Sent (SMSG_GUILD_BANK_LIST)");
}
*/

void Guild::_SendBankContentUpdate(MoveItemData* pSrc, MoveItemData* pDest) const
{
    ASSERT(pSrc->IsBank() || pDest->IsBank());

    uint8 tabId = 0;
    SlotIds slots;
    if (pSrc->IsBank()) // B ->
    {
        tabId = pSrc->GetContainer();
        slots.insert(pSrc->GetSlotId());
        if (pDest->IsBank()) // B -> B
        {
            // Same tab - add destination slots to collection
            if (pDest->GetContainer() == pSrc->GetContainer())
                pDest->CopySlots(slots);
            else // Different tabs - send second message
            {
                SlotIds destSlots;
                pDest->CopySlots(destSlots);
                _SendBankContentUpdate(pDest->GetContainer(), destSlots);
            }
        }
    }
    else if (pDest->IsBank()) // C -> B
    {
        tabId = pDest->GetContainer();
        pDest->CopySlots(slots);
    }
    _SendBankContentUpdate(tabId, slots);
}

void Guild::_SendBankContentUpdate(uint8 tabId, SlotIds slots) const
{
    if (BankTab const* tab = GetBankTab(tabId))
    {
        ByteBuffer tabData;
        WorldPacket data(SMSG_GUILD_BANK_LIST, 1200);
        data.WriteBit(0);
        data.WriteBits(slots.size(), 20);                                           // Item count
        data.WriteBits(0, 22);                                                      // Tab count

        for (SlotIds::const_iterator itr = slots.begin(); itr != slots.end(); ++itr)
        {
            data.WriteBit(0);

            Item const* tabItem = tab->GetItem(*itr);
            uint32 enchantCount = 0;
            if (tabItem)
            {
                for (uint32 enchSlot = 0; enchSlot < MAX_ENCHANTMENT_SLOT; ++enchSlot)
                {
                    if (uint32 enchantId = tabItem->GetEnchantmentId(EnchantmentSlot(enchSlot)))
                    {
                        tabData << uint32(enchantId);
                        tabData << uint32(enchSlot);
                        ++enchantCount;
                    }
                }
            }

            data.WriteBits(enchantCount, 23);                                       // enchantment count

            tabData << uint32(0);
            tabData << uint32(0);
            tabData << uint32(0);
            tabData << uint32(tabItem ? tabItem->GetCount() : 0);                   // ITEM_FIELD_STACK_COUNT
            tabData << uint32(*itr);
            tabData << uint32(0);
            tabData << uint32(tabItem ? tabItem->GetEntry() : 0);
            tabData << uint32(tabItem ? tabItem->GetItemRandomPropertyId() : 0);
            tabData << uint32(tabItem ? abs(tabItem->GetSpellCharges()) : 0);       // Spell charges
            tabData << uint32(tabItem ? tabItem->GetItemSuffixFactor() : 0);        // SuffixFactor
        }

        data.FlushBits();

        data << uint64(m_bankMoney);
        if (!tabData.empty())
            data.append(tabData);

        data << uint32(tabId);

        size_t rempos = data.wpos();
        data << uint32(0);                                      // Item withdraw amount, will be filled later

        for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
            if (_MemberHasTabRights(itr->second->GetGUID(), tabId, GUILD_BANK_RIGHT_VIEW_TAB))
                if (Player* player = itr->second->FindPlayer())
                {
                    data.put<uint32>(rempos, uint32(_GetMemberRemainingSlots(itr->second->GetGUID(), tabId)));
                    player->GetSession()->SendPacket(&data);
                }
    }
}

void Guild::_BroadcastEvent(GuildEvents guildEvent, const uint64& guid, const char* param1, const char* param2, const char* param3) const
{
    uint8 count = !param3 ? (!param2 ? (!param1 ? 0 : 1) : 2) : 3;

    WorldPacket data(SMSG_GUILD_EVENT, 1 + 1 + count + (guid ? 8 : 0));
    data << uint8(guildEvent);
    data << uint8(count);

    if (param3)
        data << param1 << param2 << param3;
    else if (param2)
        data << param1 << param2;
    else if (param1)
        data << param1;

    if (guid)
        data << uint64(guid);

    BroadcastPacket(&data);

    sLog->outDebug("WORLD: Sent SMSG_GUILD_EVENT");
}

bool Guild::IsListedAsGuildReward(uint32 item)
{
    ObjectMgr::GuildRewardsVector const& vec = sObjectMgr->GetGuildRewards();
    if (vec.find(item) != vec.end())
        return true;

    return false;
}

bool Guild::IsRewardReachable(Player* pPlayer, uint32 item)
{
    if (!pPlayer || !item)
        return false;

    ObjectMgr::GuildRewardsVector const& vec = sObjectMgr->GetGuildRewards();
    ObjectMgr::GuildRewardsVector::const_iterator itr = vec.find(item);
    // Not found in guild rewards, no reason for disallowing buying. This shouldn't happen
    if (itr == vec.end() || !itr->second)
        return true;

    // If player doesn't have enough guild reputation
    if (pPlayer->GetReputationRank(FACTION_GUILD) < ReputationRank(itr->second->standing))
        return false;

    // If guild has't achieved required achievement
    if (itr->second->achievement > 0 && !GetAchievementMgr().HasAchieved(itr->second->achievement))
        return false;

    return true;
}

GuildRewardsEntry* Guild::GetRewardData(uint32 item)
{
    ObjectMgr::GuildRewardsVector const& vec = sObjectMgr->GetGuildRewards();
    ObjectMgr::GuildRewardsVector::const_iterator itr = vec.find(item);
    if (itr != vec.end() && itr->second)
        return vec.find(item)->second;

    return NULL;
}

// Guild Advancement
void Guild::GainXP(uint64 xp)
{
    if (!xp)
        return;
    if (!sWorld->getBoolConfig(CONFIG_GUILD_ADVANCEMENT_ENABLED))
        return;
    if (GetLevel() >= sWorld->getIntConfig(CONFIG_GUILD_ADVANCEMENT_MAX_LEVEL))
        return;
    if (m_todayXP >= GUILD_DAILY_XP_CAP)
        return;
    if ((m_todayXP + xp) > GUILD_DAILY_XP_CAP)
    {
        xp = (GUILD_DAILY_XP_CAP - m_todayXP);
        if (xp == 0)
            return;
    }

    uint64 new_xp = m_xp + xp;
    uint64 nextLvlXP = GetNextLevelXP();
    uint8 level = GetLevel();
    while (new_xp >= nextLvlXP && level < sWorld->getIntConfig(CONFIG_GUILD_ADVANCEMENT_MAX_LEVEL))
    {
        new_xp -= nextLvlXP;

        if (level < sWorld->getIntConfig(CONFIG_GUILD_ADVANCEMENT_MAX_LEVEL))
        {
            LevelUp();
            ++level;
            nextLvlXP = GetNextLevelXP();
        }
    }

    m_todayXP += xp;
    m_xp = new_xp;
    SaveXP();
}

void Guild::ResetDailyXPCap()
{
    m_todayXP = 0;
    SaveXP();
}

void Guild::LevelUp()
{
    if (!sWorld->getBoolConfig(CONFIG_GUILD_ADVANCEMENT_ENABLED))
        return;

    uint8 level = m_level + 1;
    m_level = level;
    m_nextLevelXP = sObjectMgr->GetXPForGuildLevel(level);

    // Send guild news about new level
    AddGuildNews(GUILD_NEWS_GUILD_LEVEL, m_level);
    // And update achievement criteria - mgr will take guild level, no need to supply it as miscvalue
    m_achievementMgr.UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_REACH_GUILD_LEVEL);

    WorldPacket data(SMSG_GUILD_XP_UPDATE, 8*5);
    data << uint64(0x37);             // max daily xp
    data << uint64(GetNextLevelXP()); // next level xp
    data << uint64(0x37);             // weekly xp
    data << uint64(GetCurrentXP());   // Current xp
    data << uint64(GetTodayXP());     // Today xp

    // Find perk to gain
    uint32 spellId = 0;
    if(const GuildPerksEntry* perk = sGuildPerksStore.LookupEntry(level-1))
        spellId = perk->SpellId;

    // Notify players of level change
    for (Members::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        if (Player *player = itr->second->FindPlayer())
        {
            player->SetUInt32Value(PLAYER_GUILDLEVEL, level);
            player->GetSession()->SendPacket(&data);

            if (spellId)
                player->learnSpell(spellId, true);
        }
}

void Guild::SaveXP()
{
    if (getMSTime() - m_lastXPSave >= 60000) // 1 minute. Hardcoded value for now
    {
        m_lastXPSave = getMSTime();

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_GUILD_SAVE_XP);
        stmt->setUInt64(0, m_xp);
        stmt->setUInt32(1, uint32(m_level));
        stmt->setUInt32(2, m_id);
        CharacterDatabase.Execute(stmt);

        CharacterDatabase.PExecute("REPLACE INTO guild_advancement VALUES (%u," UI64FMTD ")",m_id,m_todayXP);
    }
}

void Guild::SendGuildReputationWeeklyCap(WorldSession *session, uint32 reputation) const
{
    WorldPacket data(SMSG_GUILD_REPUTATION_WEEKLY_CAP, 4);
    data << uint32(0);      // (reputation cap) - reputation
    session->SendPacket(&data);
}

void Guild::SendGuildXP(WorldSession* session /* = NULL */) const
{
    //Member const* member = GetMember(session->GetGuidLow());

    WorldPacket data(SMSG_GUILD_XP, 40);
    data << uint64(/*member ? member->GetTotalActivity() :*/ 0);
    data << uint64(GetNextLevelXP());         // XP missing for next level
    data << uint64(GetTodayXP());
    data << uint64(/*member ? member->GetWeeklyActivity() :*/ 0);
    data << uint64(GetCurrentXP());
    session->SendPacket(&data);
}
