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
#include "Common.h"
#include "SQLStorage.h"
#include "Player.h"
#include "GridNotifiers.h"
#include "Log.h"
#include "GridStates.h"
#include "CellImpl.h"
#include "Map.h"
#include "MapManager.h"
#include "MapInstanced.h"
#include "InstanceSaveMgr.h"
#include "Timer.h"
#include "GridNotifiersImpl.h"
#include "Config.h"
#include "Transport.h"
#include "ObjectMgr.h"
#include "World.h"
#include "Group.h"
#include "InstanceScript.h"


uint16 InstanceSaveManager::ResetTimeDelay[] = {3600, 900, 300, 60};

InstanceSaveManager::~InstanceSaveManager()
{
    // it is undefined whether this or objectmgr will be unloaded first
    // so we must be prepared for both cases
    lock_instLists = true;
    for (InstanceSaveHashMap::iterator itr = m_instanceSaveById.begin(); itr != m_instanceSaveById.end(); ++itr)
    {
        InstanceSave *save = itr->second;

        for (InstanceSave::PlayerListType::iterator itr2 = save->m_playerList.begin(), next = itr2; itr2 != save->m_playerList.end(); itr2 = next)
        {
            ++next;
            (*itr2)->UnbindInstance(save->GetMapId(), save->GetDifficulty(), true);
        }
        save->m_playerList.clear();

        for (InstanceSave::GroupListType::iterator itr2 = save->m_groupList.begin(), next = itr2; itr2 != save->m_groupList.end(); itr2 = next)
        {
            ++next;
            (*itr2)->UnbindInstance(save->GetMapId(), save->GetDifficulty(), true);
        }
        save->m_groupList.clear();
        delete save;
    }
}

/*
- adding instance into manager
- called from InstanceMap::Add, _LoadBoundInstances, LoadGroups
*/
InstanceSave* InstanceSaveManager::AddInstanceSave(uint32 mapId, uint32 instanceId, Difficulty difficulty, time_t resetTime, bool canReset, bool load)
{
    if (InstanceSave *old_save = GetInstanceSave(instanceId))
        return old_save;

    const MapEntry* entry = sMapStore.LookupEntry(mapId);
    if (!entry)
    {
        sLog->outError("InstanceSaveManager::AddInstanceSave: wrong mapid = %d, instanceid = %d!", mapId, instanceId);
        return NULL;
    }

    if (instanceId == 0)
    {
        sLog->outError("InstanceSaveManager::AddInstanceSave: mapid = %d, wrong instanceid = %d!", mapId, instanceId);
        return NULL;
    }

    if (difficulty >= (entry->IsRaid() ? MAX_RAID_DIFFICULTY : MAX_DUNGEON_DIFFICULTY))
    {
        sLog->outError("InstanceSaveManager::AddInstanceSave: mapid = %d, instanceid = %d, wrong dificalty %u!", mapId, instanceId, difficulty);
        return NULL;
    }

    if (!resetTime)
    {
        // initialize reset time
        // for normal instances if no creatures are killed the instance will reset in two hours
        if (entry->map_type == MAP_RAID || difficulty > DUNGEON_DIFFICULTY_NORMAL)
            resetTime = GetResetTimeFor(mapId,difficulty);
        else
        {
            resetTime = time(NULL) + 2 * HOUR;
            // normally this will be removed soon after in InstanceMap::Add, prevent error
            ScheduleReset(true, resetTime, InstResetEvent(0, mapId, difficulty, instanceId));
        }
    }

    sLog->outDebug("InstanceSaveManager::AddInstanceSave: mapid = %d, instanceid = %d", mapId, instanceId);

    InstanceSave *save = new InstanceSave(mapId, instanceId, difficulty, resetTime, canReset);
    if (!load)
        save->SaveToDB();

    m_instanceSaveById[instanceId] = save;
    return save;
}

InstanceSave *InstanceSaveManager::GetInstanceSave(uint32 InstanceId)
{
    InstanceSaveHashMap::iterator itr = m_instanceSaveById.find(InstanceId);
    return itr != m_instanceSaveById.end() ? itr->second : NULL;
}

void InstanceSaveManager::DeleteInstanceFromDB(uint32 instanceid)
{
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    trans->PAppend("DELETE FROM instance WHERE id = '%u'", instanceid);
    trans->PAppend("DELETE FROM character_instance WHERE instance = '%u'", instanceid);
    trans->PAppend("DELETE FROM group_instance WHERE instance = '%u'", instanceid);
    CharacterDatabase.CommitTransaction(trans);
    // respawn times should be deleted only when the map gets unloaded
}

void InstanceSaveManager::RemoveInstanceSave(uint32 InstanceId)
{
    InstanceSaveHashMap::iterator itr = m_instanceSaveById.find(InstanceId);
    if (itr != m_instanceSaveById.end())
    {
        // save the resettime for normal instances only when they get unloaded
        if (time_t resettime = itr->second->GetResetTimeForDB())
            CharacterDatabase.PExecute("UPDATE instance SET resettime = '" UI64FMTD "' WHERE id = '%u'", (uint64)resettime, InstanceId);

        delete itr->second;
        m_instanceSaveById.erase(itr);
    }
}

InstanceSave::InstanceSave(uint16 MapId, uint32 InstanceId, Difficulty difficulty, time_t resetTime, bool canReset)
: m_resetTime(resetTime), m_instanceid(InstanceId), m_mapid(MapId),
  m_difficulty(difficulty), m_canReset(canReset)
{
}

InstanceSave::~InstanceSave()
{
    // the players and groups must be unbound before deleting the save
    ASSERT(m_playerList.empty() && m_groupList.empty());
}

/*
    Called from AddInstanceSave
*/
void InstanceSave::SaveToDB()
{
    // save instance data too
    std::string data;

    Map *map = sMapMgr->FindMap(GetMapId(),m_instanceid);
    if (map)
    {
        ASSERT(map->IsDungeon());
        if (InstanceScript *iData = ((InstanceMap*)map)->GetInstanceScript())
        {
            data = iData->GetSaveData();
            if (!data.empty())
                CharacterDatabase.escape_string(data);
        }
    }

    CharacterDatabase.PExecute("INSERT INTO instance VALUES ('%u', '%u', '" UI64FMTD "', '%u', '%s')", m_instanceid, GetMapId(), (uint64)GetResetTimeForDB(), GetDifficulty(), data.c_str());
}

time_t InstanceSave::GetResetTimeForDB()
{
    // only save the reset time for normal instances
    const MapEntry *entry = sMapStore.LookupEntry(GetMapId());
    if (!entry || entry->map_type == MAP_RAID || GetDifficulty() == DUNGEON_DIFFICULTY_HEROIC)
        return 0;
    else
        return GetResetTime();
}

// to cache or not to cache, that is the question
InstanceTemplate const* InstanceSave::GetTemplate()
{
    return sObjectMgr->GetInstanceTemplate(m_mapid);
}

MapEntry const* InstanceSave::GetMapEntry()
{
    return sMapStore.LookupEntry(m_mapid);
}

void InstanceSave::DeleteFromDB()
{
    InstanceSaveManager::DeleteInstanceFromDB(GetInstanceId());
}

/* true if the instance save is still valid */
bool InstanceSave::UnloadIfEmpty()
{
    if (m_playerList.empty() && m_groupList.empty())
    {
        if (!sInstanceSaveMgr->lock_instLists)
            sInstanceSaveMgr->RemoveInstanceSave(GetInstanceId());

        return false;
    }
    else
        return true;
}

void InstanceSaveManager::_DelHelper(const char *fields, const char *table, const char *queryTail,...)
{
    Tokens fieldTokens(fields, ',');
    ASSERT(fieldTokens.size() != 0);

    va_list ap;
    char szQueryTail [MAX_QUERY_LEN];
    va_start(ap, queryTail);
    vsnprintf(szQueryTail, MAX_QUERY_LEN, queryTail, ap);
    va_end(ap);

    QueryResult result = CharacterDatabase.PQuery("SELECT %s FROM %s %s", fields, table, szQueryTail);
    if (result)
    {
        do
        {
            Field *fields = result->Fetch();
            std::ostringstream ss;
            for (size_t i = 0; i < fieldTokens.size(); i++)
            {
                std::string fieldValue = fields[i].GetString();
                CharacterDatabase.escape_string(fieldValue);
                ss << (i != 0 ? " AND " : "") << fieldTokens[i] << " = '" << fieldValue << "'";
            }
            CharacterDatabase.DirectPExecute("DELETE FROM %s WHERE %s", table, ss.str().c_str());
        } while (result->NextRow());
    }
}

void InstanceSaveManager::CleanupAndPackInstances()
{
    // load reset times and clean expired instances
    sInstanceSaveMgr->LoadResetTimes();

    // Delete invalid character_instance and group_instance references
    CharacterDatabase.DirectExecute("DELETE ci.* FROM character_instance AS ci LEFT JOIN characters AS c ON ci.guid = c.guid WHERE c.guid IS NULL");
    CharacterDatabase.DirectExecute("DELETE gi.* FROM group_instance     AS gi LEFT JOIN groups     AS g ON gi.guid = g.guid WHERE g.guid IS NULL");

    // Delete invalid instance references
    CharacterDatabase.DirectExecute("DELETE i.* FROM instance AS i LEFT JOIN character_instance AS ci ON i.id = ci.instance LEFT JOIN group_instance AS gi ON i.id = gi.instance WHERE ci.guid IS NULL AND gi.guid IS NULL");

    // Delete invalid references to instance
    CharacterDatabase.DirectExecute("DELETE tmp.* FROM creature_respawn   AS tmp LEFT JOIN instance ON tmp.instance = instance.id WHERE tmp.instance > 0 AND instance.id IS NULL");
    CharacterDatabase.DirectExecute("DELETE tmp.* FROM gameobject_respawn AS tmp LEFT JOIN instance ON tmp.instance = instance.id WHERE tmp.instance > 0 AND instance.id IS NULL");
    CharacterDatabase.DirectExecute("DELETE tmp.* FROM character_instance AS tmp LEFT JOIN instance ON tmp.instance = instance.id WHERE tmp.instance > 0 AND instance.id IS NULL");
    CharacterDatabase.DirectExecute("DELETE tmp.* FROM group_instance     AS tmp LEFT JOIN instance ON tmp.instance = instance.id WHERE tmp.instance > 0 AND instance.id IS NULL");

    // Clean invalid references to instance
    CharacterDatabase.DirectExecute("UPDATE corpse     AS tmp LEFT JOIN instance ON tmp.instance    = instance.id SET tmp.instance    = 0 WHERE tmp.instance    > 0 AND instance.id IS NULL");
    CharacterDatabase.DirectExecute("UPDATE characters AS tmp LEFT JOIN instance ON tmp.instance_id = instance.id SET tmp.instance_id = 0 WHERE tmp.instance_id > 0 AND instance.id IS NULL");

    // Create new index
    CharacterDatabase.DirectExecute("ALTER TABLE instance ADD newid INT UNSIGNED AUTO_INCREMENT, ADD INDEX(newid)");

    // Update old ids
    CharacterDatabase.DirectExecute("UPDATE corpse                  AS tmp LEFT JOIN instance ON tmp.instance    = instance.id SET tmp.instance    = instance.newid WHERE tmp.instance    > 0");
    CharacterDatabase.DirectExecute("UPDATE character_instance      AS tmp LEFT JOIN instance ON tmp.instance    = instance.id SET tmp.instance    = instance.newid WHERE tmp.instance    > 0");
    CharacterDatabase.DirectExecute("UPDATE group_instance          AS tmp LEFT JOIN instance ON tmp.instance    = instance.id SET tmp.instance    = instance.newid WHERE tmp.instance    > 0");
    CharacterDatabase.DirectExecute("UPDATE characters              AS tmp LEFT JOIN instance ON tmp.instance_id = instance.id SET tmp.instance_id = instance.newid WHERE tmp.instance_id > 0");
    CharacterDatabase.DirectExecute("UPDATE creature_respawn        AS tmp LEFT JOIN instance ON tmp.instance    = instance.id SET tmp.instance    = instance.newid WHERE tmp.instance    > 0");
    CharacterDatabase.DirectExecute("UPDATE gameobject_respawn      AS tmp LEFT JOIN instance ON tmp.instance    = instance.id SET tmp.instance    = instance.newid WHERE tmp.instance    > 0");

    // Update instance too
    CharacterDatabase.Query("UPDATE instance SET id = newid");

    // Finally drop the no longer needed column
    CharacterDatabase.Query("ALTER TABLE instance DROP COLUMN newid");

    // Bake some cookies for click
    sLog->outString(">> Cleaned up and packed instances");
}

void InstanceSaveManager::LoadResetTimes()
{
    time_t now = time(NULL);
    time_t today = (now / DAY) * DAY;

    // NOTE: Use DirectPExecute for tables that will be queried later

    // get the current reset times for normal instances (these may need to be updated)
    // these are only kept in memory for InstanceSaves that are loaded later
    // resettime = 0 in the DB for raid/heroic instances so those are skipped
    typedef std::pair<uint32 /*PAIR32(map,difficulty)*/, time_t> ResetTimeMapDiffType;
    typedef std::map<uint32, ResetTimeMapDiffType> InstResetTimeMapDiffType;
    InstResetTimeMapDiffType instResetTime;

    // index instance ids by map/difficulty pairs for fast reset warning send
    typedef std::multimap<uint32 /*PAIR32(map,difficulty)*/, uint32 /*instanceid*/ > ResetTimeMapDiffInstances;
    ResetTimeMapDiffInstances mapDiffResetInstances;

    QueryResult result = CharacterDatabase.Query("SELECT id, map, difficulty, resettime FROM instance WHERE resettime > 0");
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();

            if (time_t resettime = time_t(fields[3].GetUInt64()))
            {
                uint32 id = fields[0].GetUInt32();
                uint32 mapid = fields[1].GetUInt32();
                uint32 difficulty = fields[2].GetUInt32();
                instResetTime[id] = ResetTimeMapDiffType(MAKE_PAIR32(mapid, difficulty), resettime);
                mapDiffResetInstances.insert(ResetTimeMapDiffInstances::value_type(MAKE_PAIR32(mapid, difficulty), id));
            }
        }
        while (result->NextRow());

        // update reset time for normal instances with the max creature respawn time + X hours
        result = CharacterDatabase.Query("SELECT MAX(respawntime), instance FROM creature_respawn WHERE instance > 0 GROUP BY instance");
        if (result)
        {
            do
            {
                Field *fields = result->Fetch();
                uint32 instance = fields[1].GetUInt32();
                time_t resettime = time_t(fields[0].GetUInt64() + 2 * HOUR);
                InstResetTimeMapDiffType::iterator itr = instResetTime.find(instance);
                if (itr != instResetTime.end() && itr->second.second != resettime)
                {
                    CharacterDatabase.DirectPExecute("UPDATE instance SET resettime = '" UI64FMTD "' WHERE id = '%u'", uint64(resettime), instance);
                    itr->second.second = resettime;
                }
            }
            while (result->NextRow());
        }

        // schedule the reset times
        for (InstResetTimeMapDiffType::iterator itr = instResetTime.begin(); itr != instResetTime.end(); ++itr)
            if (itr->second.second > now)
                ScheduleReset(true, itr->second.second, InstResetEvent(0, PAIR32_LOPART(itr->second.first), Difficulty(PAIR32_HIPART(itr->second.first)), itr->first));
    }

    // load the global respawn times for raid/heroic instances
    uint32 diff = sWorld->getIntConfig(CONFIG_INSTANCE_RESET_TIME_HOUR) * HOUR;
    result = CharacterDatabase.Query("SELECT mapid, difficulty, resettime FROM instance_reset");
    if (result)
    {
        do
        {
            Field *fields = result->Fetch();
            uint32 mapid = fields[0].GetUInt32();
            Difficulty difficulty = Difficulty(fields[1].GetUInt32());
            uint64 oldresettime = fields[2].GetUInt64();

            MapDifficulty const* mapDiff = GetMapDifficultyData(mapid, difficulty);
            if (!mapDiff)
            {
                sLog->outError("InstanceSaveManager::LoadResetTimes: invalid mapid(%u)/difficulty(%u) pair in instance_reset!", mapid, difficulty);
                CharacterDatabase.DirectPExecute("DELETE FROM instance_reset WHERE mapid = '%u' AND difficulty = '%u'", mapid,difficulty);
                continue;
            }

            // update the reset time if the hour in the configs changes
            uint64 newresettime = (oldresettime / DAY) * DAY + diff;
            if (oldresettime != newresettime)
                CharacterDatabase.DirectPExecute("UPDATE instance_reset SET resettime = '" UI64FMTD "' WHERE mapid = '%u' AND difficulty = '%u'", newresettime, mapid, difficulty);

            SetResetTimeFor(mapid, difficulty, newresettime);
        } while (result->NextRow());
    }

    // clean expired instances, references to them will be deleted in CleanupInstances
    // must be done before calculating new reset times
    _DelHelper("id, map, instance.difficulty", "instance", "LEFT JOIN instance_reset ON mapid = map AND instance.difficulty =  instance_reset.difficulty WHERE (instance.resettime < '" UI64FMTD "' AND instance.resettime > '0') OR (NOT instance_reset.resettime IS NULL AND instance_reset.resettime < '" UI64FMTD "')",  (uint64)now, (uint64)now);

    ResetTimeMapDiffInstances::const_iterator in_itr;

    // calculate new global reset times for expired instances and those that have never been reset yet
    // add the global reset times to the priority queue
    for (MapDifficultyMap::const_iterator itr = sMapDifficultyMap.begin(); itr != sMapDifficultyMap.end(); ++itr)
    {
        uint32 map_diff_pair = itr->first;
        uint32 mapid = PAIR32_LOPART(map_diff_pair);
        Difficulty difficulty = Difficulty(PAIR32_HIPART(map_diff_pair));
        MapDifficulty const* mapDiff = &itr->second;
        if (!mapDiff->resetTime)
            continue;

        // the reset_delay must be at least one day
        uint32 period = uint32(((mapDiff->resetTime * sWorld->getRate(RATE_INSTANCE_RESET_TIME))/DAY) * DAY);
        if (period < DAY)
            period = DAY;

        time_t t = GetResetTimeFor(mapid,difficulty);
        if (!t)
        {
            // initialize the reset time
            t = today + period + diff;
            CharacterDatabase.DirectPExecute("INSERT INTO instance_reset VALUES ('%u','%u','" UI64FMTD "')", mapid, difficulty, (uint64)t);
        }

        if (t < now)
        {
            // assume that expired instances have already been cleaned
            // calculate the next reset time
            t = (t / DAY) * DAY;
            t += ((today - t) / period + 1) * period + diff;
            CharacterDatabase.DirectPExecute("UPDATE instance_reset SET resettime = '" UI64FMTD "' WHERE mapid = '%u' AND difficulty= '%u'", (uint64)t, mapid, difficulty);
        }

        SetResetTimeFor(mapid,difficulty,t);

        // schedule the global reset/warning
        uint8 type;
        for (type = 1; type < 4; ++type)
            if (t - ResetTimeDelay[type-1] > now)
                break;

        ScheduleReset(true, t - ResetTimeDelay[type-1], InstResetEvent(type, mapid, difficulty, 0));

        for (in_itr = mapDiffResetInstances.lower_bound(map_diff_pair); in_itr != mapDiffResetInstances.upper_bound(map_diff_pair); ++in_itr)
            ScheduleReset(true, t - ResetTimeDelay[type-1], InstResetEvent(type, mapid, difficulty, in_itr->second));
    }
}

void InstanceSaveManager::ScheduleReset(bool add, time_t time, InstResetEvent event)
{
    if (!add)
    {
        // find the event in the queue and remove it
        ResetTimeQueue::iterator itr;
        std::pair<ResetTimeQueue::iterator, ResetTimeQueue::iterator> range;
        range = m_resetTimeQueue.equal_range(time);
        for (itr = range.first; itr != range.second; ++itr)
        {
            if (itr->second == event)
            {
                m_resetTimeQueue.erase(itr);
                return;
            }
        }

        // in case the reset time changed (should happen very rarely), we search the whole queue
        if (itr == range.second)
        {
            for (itr = m_resetTimeQueue.begin(); itr != m_resetTimeQueue.end(); ++itr)
            {
                if (itr->second == event)
                {
                    m_resetTimeQueue.erase(itr);
                    return;
                }
            }

            if (itr == m_resetTimeQueue.end())
                sLog->outError("InstanceSaveManager::ScheduleReset: cannot cancel the reset, the event(%d,%d,%d) was not found!", event.type, event.mapid, event.instanceId);
        }
    }
    else
        m_resetTimeQueue.insert(std::pair<time_t, InstResetEvent>(time, event));
}

void InstanceSaveManager::Update()
{
    time_t now = time(NULL);
    time_t t;

    while (!m_resetTimeQueue.empty())
    {
        t = m_resetTimeQueue.begin()->first;
        if (t >= now)
            break;

        InstResetEvent &event = m_resetTimeQueue.begin()->second;
        if (event.type == 0)
        {
            // for individual normal instances, max creature respawn + X hours
            _ResetInstance(event.mapid, event.instanceId);
            m_resetTimeQueue.erase(m_resetTimeQueue.begin());
        }
        else
        {
            // global reset/warning for a certain map
            time_t resetTime = GetResetTimeFor(event.mapid,event.difficulty);
            _ResetOrWarnAll(event.mapid, event.difficulty, event.type != 4, resetTime);
            if (event.type != 4)
            {
                // schedule the next warning/reset
                ++event.type;
                ScheduleReset(true, resetTime - ResetTimeDelay[event.type-1], event);
            }
            m_resetTimeQueue.erase(m_resetTimeQueue.begin());
        }
    }
}

void InstanceSaveManager::_ResetSave(InstanceSaveHashMap::iterator &itr)
{
    // unbind all players bound to the instance
    // do not allow UnbindInstance to automatically unload the InstanceSaves
    lock_instLists = true;

    InstanceSave::PlayerListType &pList = itr->second->m_playerList;
    while (!pList.empty())
    {
        Player *player = *(pList.begin());
        player->UnbindInstance(itr->second->GetMapId(), itr->second->GetDifficulty(), true);
    }

    InstanceSave::GroupListType &gList = itr->second->m_groupList;
    while (!gList.empty())
    {
        Group *group = *(gList.begin());
        group->UnbindInstance(itr->second->GetMapId(), itr->second->GetDifficulty(), true);
    }

    delete itr->second;
    m_instanceSaveById.erase(itr++);

    lock_instLists = false;
}

void InstanceSaveManager::_ResetInstance(uint32 mapid, uint32 instanceId)
{
    sLog->outDebug("InstanceSaveMgr::_ResetInstance %u, %u", mapid, instanceId);
    Map *map = (MapInstanced*)sMapMgr->CreateBaseMap(mapid);
    if (!map->Instanceable())
        return;

    InstanceSaveHashMap::iterator itr = m_instanceSaveById.find(instanceId);
    if (itr != m_instanceSaveById.end())
        _ResetSave(itr);

    DeleteInstanceFromDB(instanceId);                       // even if save not loaded

    Map* iMap = ((MapInstanced*)map)->FindMap(instanceId);

    if (iMap && iMap->IsDungeon())
        ((InstanceMap*)iMap)->Reset(INSTANCE_RESET_RESPAWN_DELAY);
    else
        sObjectMgr->DeleteRespawnTimeForInstance(instanceId);   // even if map is not loaded
}

void InstanceSaveManager::_ResetOrWarnAll(uint32 mapid, Difficulty difficulty, bool warn, time_t resetTime)
{
    // global reset for all instances of the given map
    MapEntry const *mapEntry = sMapStore.LookupEntry(mapid);
    if (!mapEntry->Instanceable())
        return;

    time_t now = time(NULL);

    if (!warn)
    {
        MapDifficulty const* mapDiff = GetMapDifficultyData(mapid, difficulty);
        if (!mapDiff || !mapDiff->resetTime)
        {
            sLog->outError("InstanceSaveManager::ResetOrWarnAll: not valid difficulty or no reset delay for map %d", mapid);
            return;
        }

        // remove all binds to instances of the given map
        for (InstanceSaveHashMap::iterator itr = m_instanceSaveById.begin(); itr != m_instanceSaveById.end();)
        {
            if (itr->second->GetMapId() == mapid && itr->second->GetDifficulty() == difficulty)
                _ResetSave(itr);
            else
                ++itr;
        }

        // delete them from the DB, even if not loaded
        SQLTransaction trans = CharacterDatabase.BeginTransaction();
        trans->PAppend("DELETE FROM character_instance USING character_instance LEFT JOIN instance ON character_instance.instance = id WHERE map = '%u' and difficulty='%u'", mapid, difficulty);
        trans->PAppend("DELETE FROM group_instance USING group_instance LEFT JOIN instance ON group_instance.instance = id WHERE map = '%u' and difficulty='%u'", mapid, difficulty);
        trans->PAppend("DELETE FROM instance WHERE map = '%u' and difficulty='%u'", mapid, difficulty);
        CharacterDatabase.CommitTransaction(trans);

        // calculate the next reset time
        uint32 diff = sWorld->getIntConfig(CONFIG_INSTANCE_RESET_TIME_HOUR) * HOUR;

        uint32 period = uint32(((mapDiff->resetTime * sWorld->getRate(RATE_INSTANCE_RESET_TIME))/DAY) * DAY);
        if (period < DAY)
            period = DAY;

        uint64 next_reset = ((resetTime + MINUTE) / DAY * DAY) + period + diff;

        SetResetTimeFor(mapid, difficulty, next_reset);
        ScheduleReset(true, time_t(next_reset-3600), InstResetEvent(1, mapid, difficulty, 0));

        // update it in the DB
        CharacterDatabase.PExecute("UPDATE instance_reset SET resettime = '" UI64FMTD "' WHERE mapid = '%d' AND difficulty = '%d'", next_reset, mapid, difficulty);
    }

    // note: this isn't fast but it's meant to be executed very rarely
    Map const *map = sMapMgr->CreateBaseMap(mapid);          // _not_ include difficulty
    MapInstanced::InstancedMaps &instMaps = ((MapInstanced*)map)->GetInstancedMaps();
    MapInstanced::InstancedMaps::iterator mitr;
    uint32 timeLeft;

    for (mitr = instMaps.begin(); mitr != instMaps.end(); ++mitr)
    {
        Map *map2 = mitr->second;
        if (!map2->IsDungeon())
            continue;

        if (warn)
        {            
            if (now <= resetTime)
                timeLeft = 0;
            else
                timeLeft = uint32(now - resetTime);

            ((InstanceMap*)map2)->SendResetWarnings(timeLeft);
        }
        else
            ((InstanceMap*)map2)->Reset(INSTANCE_RESET_GLOBAL);
    }

    // TODO: delete creature/gameobject respawn times even if the maps are not loaded
}

uint32 InstanceSaveManager::GetNumBoundPlayersTotal()
{
    uint32 ret = 0;
    for (InstanceSaveHashMap::iterator itr = m_instanceSaveById.begin(); itr != m_instanceSaveById.end(); ++itr)
        ret += itr->second->GetPlayerCount();

    return ret;
}

uint32 InstanceSaveManager::GetNumBoundGroupsTotal()
{
    uint32 ret = 0;
    for (InstanceSaveHashMap::iterator itr = m_instanceSaveById.begin(); itr != m_instanceSaveById.end(); ++itr)
        ret += itr->second->GetGroupCount();

    return ret;
}

/*This is gona be HUUUUUUUUUUUGE. Sorry, everybody is saving boss data as he wants"-.-
  Loads Raid boss data into data structure when server starts, so no map must be created later
*/
void InstanceSaveManager::loadRaidEncounter()
{
    uint32 count = 0;
    QueryResult result = CharacterDatabase.Query("SELECT id, data, map FROM instance");
    uint32 instanceId;
    std::string data;
    uint32 mapId;
    int bossNum=0;
    char DummyReadCh;
    uint32 DummyReadInt;
    /*Set boss numbers for all needed maps*/
    setBossNumber(671, 5); //BoT
    setBossNumber(669, 6); //BwD
    setBossNumber(757, 2); //BH
    setBossNumber(754, 2); //ToFW
    setBossNumber(720, 7); //Firelands

    setBossNumber(33, 5); //SfK
    setBossNumber(645, 5); //BrC
    setBossNumber(670, 4); //GB
    setBossNumber(644, 7); //HoO
    setBossNumber(755, 4); //Tolvir
    setBossNumber(657, 3);//VP
    setBossNumber(725, 4);//SC
    setBossNumber(643, 4);//TotT
    setBossNumber(36, 6);//DM
    setBossNumber(568, 6);//ZA
    setBossNumber(938, 5); // End Time

    if (!result)
    {
        sLog->outString();
        sLog->outErrorDb(">> Loaded 0 raid encounter boss data.");
        return;
    }

    Field* fields = NULL;
    do
    {
        fields = result->Fetch();
        instanceId = fields[0].GetUInt32();
        data = fields[1].GetString();
        mapId = fields[2].GetUInt32();

        switch(mapId)
        {
            case 671://The Bastion of Twilight
            {
                bossNum=5;
                uint32 dataEnc[5];               
                std::istringstream loadStream(data);
                for (uint8 i = 0; i < bossNum; i++)
                {
                    switch(i)
                    {
                        case 0:
                            loadStream >> dataEnc[2];//halfus
                            break;
                        case 1:
                            loadStream >> dataEnc[0];//valiona and theralion
                            break;
                        case 2:
                            loadStream >> dataEnc[4];//council
                            break;
                        case 3:                            
                            loadStream >> dataEnc[3];//chogall
                            break;
                        case 4:
                            loadStream >> dataEnc[1];//sinestra
                            break;
                    }
                }
                setInstanceSaveData(instanceId,dataEnc,bossNum);
                break;
            }
            case 669://Blackwing Descent
            {
                bossNum=6;
                uint32 dataEnc[6];
                std::istringstream loadStream(data);
                for (uint8 i = 0; i < bossNum+1; i++)
                {
                    switch(i)
                    {
                    case 0:
                        loadStream >> DummyReadInt;
                        break;
                    case 1:
                        loadStream >> dataEnc[2];//Maloriak
                        break;
                    case 2:
                        loadStream >> dataEnc[5];//Atramedes
                        break;
                    case 3:                            
                        loadStream >> dataEnc[4];//Chimareon
                        break;
                    case 4:
                        loadStream >> dataEnc[0];//Omnotron
                        break;
                    case 5:
                        loadStream >> dataEnc[3];//Magmaw
                        break;
                    case 6:
                        loadStream >> dataEnc[1];//Nefarian
                        break;
                    }
                }
                setInstanceSaveData(instanceId,dataEnc,bossNum);
                break;
            }
            case 757://Baradin Hold
            {
                bossNum=2;
                uint32 dataEnc[2];
                std::istringstream loadStream(data);
                loadStream >> DummyReadCh >>DummyReadCh;
                for (uint8 i = 0; i < bossNum; i++)
                {
                    switch(i)
                    {
                    case 0:
                        loadStream >> dataEnc[1];//Argaloth
                        break;
                    case 1:
                        loadStream >> dataEnc[0];//Occuthar
                        break;
                    }
                }
                setInstanceSaveData(instanceId,dataEnc,bossNum);
                break;
            }
            case 754://Throne of the Four Winds
            {
                bossNum=2;
                uint32 dataEnc[2];
                std::istringstream loadStream(data);
                for (uint8 i = 0; i < bossNum; i++)
                {
                    switch(i)
                    {
                    case 0:
                        loadStream >> dataEnc[0];//Conclave
                        break;
                    case 1:
                        loadStream >> dataEnc[1];//Alakir
                        break;
                    }
                }
                setInstanceSaveData(instanceId,dataEnc,bossNum);
                break;
            }
            case 720://Firelands
            {
                bossNum=7;
                uint32 dataEnc[7];
                std::istringstream loadStream(data);
                for (uint8 i = 0; i < bossNum; i++)
                {
                    switch(i)
                    {
                    case 0:
                        loadStream >> dataEnc[5];//Bethilac
                        break;
                    case 1:
                        loadStream >> dataEnc[2];//Rhyolit
                        break;
                    case 2:
                        loadStream >> dataEnc[0];//Alysrazor
                        break;
                    case 3:                            
                        loadStream >> dataEnc[1];//Shannox
                        break;
                    case 4:
                        loadStream >> dataEnc[4];//Baleroc
                        break;
                    case 5:
                        loadStream >> dataEnc[6];//Staghelm
                        break;
                    case 6:
                        loadStream >> dataEnc[3];//Ragnaros
                        break;
                    }
                }
                setInstanceSaveData(instanceId,dataEnc,bossNum);
                break;
            }
            case 33://Shadowfang keep
            {
                bossNum=5;
                uint32 dataEnc[5];
                std::istringstream loadStream(data);
                for (uint8 i = 0; i < bossNum; i++)
                {
                    switch(i)
                    {
                    case 0:
                        loadStream >> dataEnc[4];//Ashbury
                        break;
                    case 1:
                        loadStream >> dataEnc[3];//Silverlaine
                        break;
                    case 2:                            
                        loadStream >> dataEnc[2];//Springvale
                        break;
                    case 3:
                        loadStream >> dataEnc[0];//Valden
                        break;
                    case 4:
                        loadStream >> dataEnc[1];//Godfrey
                        break;
                    }
                }
                setInstanceSaveData(instanceId,dataEnc,bossNum);
                break;
            }
            case 645://Blackrock caverns
            {
                bossNum=5;
                uint32 dataEnc[5];
                std::istringstream loadStream(data);
                for (uint8 i = 0; i < bossNum; i++)
                {
                    switch(i)
                    {
                    case 0:
                        loadStream >> dataEnc[0];//Romogg
                        break;
                    case 1:
                        loadStream >> dataEnc[2];//Corla
                        break;
                    case 2:                            
                        loadStream >> dataEnc[1];//Karsh
                        break;
                    case 3:
                        loadStream >> dataEnc[3];//Beauty
                        break;
                    case 4:
                        loadStream >> dataEnc[4];//Obsidius
                        break;
                    }
                }
                setInstanceSaveData(instanceId,dataEnc,bossNum);
                break;
            }
            case 670://Grim batol
            {
                bossNum=4;
                uint32 dataEnc[4];
                std::istringstream loadStream(data);
                for (uint8 i = 0; i < bossNum; i++)
                {
                    switch(i)
                    {
                    case 0:
                        loadStream >> dataEnc[0];//Umbriss
                        break;
                    case 1:
                        loadStream >> dataEnc[1];//Throngus
                        break;
                    case 2:                            
                        loadStream >> dataEnc[3];//Dragha
                        break;
                    case 3:
                        loadStream >> dataEnc[2];//Edurax
                        break;
                    }
                }
                setInstanceSaveData(instanceId,dataEnc,bossNum);
                break;
            }
            case 644://Halls of Origination
            {
                bossNum=7;
                uint32 dataEnc[7];
                std::istringstream loadStream(data);
                for (uint8 i = 0; i < bossNum; i++)
                {
                    switch(i)
                    {
                    case 0:
                        loadStream >> dataEnc[0];//Anhuur
                        break;
                    case 1:
                        loadStream >> dataEnc[5];//Anarphet
                        break;
                    case 2:                            
                        loadStream >> dataEnc[3];//Isiset
                        break;
                    case 3:
                        loadStream >> dataEnc[6];//Ammunae
                        break;
                    case 4:
                        loadStream >> dataEnc[1];//Setesh
                        break;
                    case 5:
                        loadStream >> dataEnc[2];//Rajh
                        break;
                    case 6:
                        loadStream >> dataEnc[4];//Ptah
                        break;
                    }
                }
                setInstanceSaveData(instanceId,dataEnc,bossNum);
                break;
            }
            case 755://Lost city of Tolvir
            {
                bossNum=4;
                uint32 dataEnc[4];
                std::istringstream loadStream(data);
                for (uint8 i = 0; i < bossNum; i++)
                {
                    switch(i)
                    {
                    case 0:
                        loadStream >> dataEnc[3];//Husam
                        break;
                    case 1:
                        loadStream >> dataEnc[2];//Barim
                        break;
                    case 2:                            
                        loadStream >> dataEnc[1];//Lockmaw
                        break;
                    case 3:
                        loadStream >> dataEnc[0];//Siamat
                        break;
                    }
                }
                setInstanceSaveData(instanceId,dataEnc,bossNum);
                break;
            }
            case 657://Vortex pinnacle
            {
                bossNum=3;
                uint32 dataEnc[3];
                std::istringstream loadStream(data);
                for (uint8 i = 0; i < bossNum; i++)
                {
                    switch(i)
                    {
                    case 0:
                        loadStream >> dataEnc[0];//Ertan
                        break;
                    case 1:
                        loadStream >> dataEnc[2];//Altarius
                        break;
                    case 2:                            
                        loadStream >> dataEnc[1];//Asaad
                        break;
                    }
                }
                setInstanceSaveData(instanceId,dataEnc,bossNum);
                break;
            }
            case 725://The Stonecore
            {
                bossNum=4;
                uint32 dataEnc[4];
                std::istringstream loadStream(data);
                for (uint8 i = 0; i < bossNum; i++)
                {
                    switch(i)
                    {
                    case 0:
                        loadStream >> dataEnc[3];//Corborus
                        break;
                    case 1:
                        loadStream >> dataEnc[0];//Slabhide
                        break;
                    case 2:                            
                        loadStream >> dataEnc[1];//Ozruk
                        break;
                    case 3:
                        loadStream >> dataEnc[2];//Azil
                        break;
                    }
                }
                setInstanceSaveData(instanceId,dataEnc,bossNum);
                break;
            }
            case 643://Throne of the Tides
            {
                bossNum=4;
                uint32 dataEnc[4];
                std::istringstream loadStream(data);
                for (uint8 i = 0; i < bossNum; i++)
                {
                    switch(i)
                    {
                    case 0:
                        loadStream >> dataEnc[2];//Nazjar
                        break;
                    case 1:
                        loadStream >> dataEnc[3];//Ulthok
                        break;
                    case 2:                            
                        loadStream >> dataEnc[1];//Stonespeaker
                        break;
                    case 3:
                        loadStream >> dataEnc[0];//Ozumat
                        break;
                    }
                }
                setInstanceSaveData(instanceId,dataEnc,bossNum);
                break;
            }
            case 36://Deadmines
            {
                bossNum=6;
                uint32 dataEnc[6];
                std::istringstream loadStream(data);
                for (uint8 i = 0; i < bossNum; i++)
                {
                    switch(i)
                    {
                    case 0:
                        loadStream >> dataEnc[2];//Glubtok
                        break;
                    case 1:
                        loadStream >> dataEnc[1];//Gearbreaker
                        break;
                    case 2:                            
                        loadStream >> dataEnc[3];//Reaper 5000
                        break;
                    case 3:
                        loadStream >> dataEnc[4];//Ripsnarl
                        break;
                    case 4:
                        loadStream >> dataEnc[5];//Cookie
                        break;
                    case 5:
                        loadStream >> dataEnc[0];//Vancleef
                        break;
                    }
                }
                setInstanceSaveData(instanceId,dataEnc,bossNum);
                break;
            }
            case 568://Zulaman
            {
                bossNum=6;
                uint32 dataEnc[6];
                std::istringstream loadStream(data);
                loadStream >> DummyReadCh >> DummyReadInt >> DummyReadInt >> DummyReadInt;
                for (uint8 i = 0; i < bossNum; i++)
                {
                    switch(i)
                    {
                    case 0:
                        loadStream >> dataEnc[4];//Nalorakk
                        break;
                    case 1:
                        loadStream >> dataEnc[5];//Akilzon
                        break;
                    case 2:                            
                        loadStream >> dataEnc[3];//Janalai
                        break;
                    case 3:
                        loadStream >> dataEnc[2];//Halazzi
                        break;
                    case 4:
                        loadStream >> dataEnc[1];//Hexlord
                        break;
                    case 5:
                        loadStream >> dataEnc[0];//Zuljin
                        break;
                    }
                }
                setInstanceSaveData(instanceId,dataEnc,bossNum);
                break;
            }
            case 938: // End Time
            {
                bossNum=5;
                uint32 dataEnc[5];
                std::istringstream loadStream(data);
                for (uint8 i = 0; i < bossNum; i++)
                {
                    switch(i)
                    {
                    case 0:
                        loadStream >> dataEnc[0]; // Murozond
                        break;
                    case 1:
                        loadStream >> dataEnc[1]; // Sylvanas
                        break;
                    case 2:
                        loadStream >> dataEnc[2]; // Jaina
                        break;
                    case 3:
                        loadStream >> dataEnc[3]; // Baine
                        break;
                    case 4:
                        loadStream >> dataEnc[4]; // Tyrande
                        break;
                    }
                }
                setInstanceSaveData(instanceId,dataEnc,bossNum);
                break;
            }

            default://raids for which Raid info does not work yet
            {
                bossNum=0;
                uint32 dataEnc[1];
                dataEnc[0]=0;
                setBossNumber(mapId,bossNum);
                setInstanceSaveData(instanceId,dataEnc,bossNum);
                break;
                /*
                case mapid://
                {
                bossNum=0;
                uint32 dataEnc[5];
                std::istringstream loadStream(data);
                for (uint8 i = 0; i < bossNum; i++)
                {
                    switch(i)
                    {
                    case 0:
                    loadStream >> dataEnc[4];//
                    break;
                    case 1:
                    loadStream >> dataEnc[3];//
                    break;
                    case 2:                            
                    loadStream >> dataEnc[2];//
                    break;
                    case 3:
                    loadStream >> dataEnc[0];//
                    break;
                    case 4:
                    loadStream >> dataEnc[1];//
                    break;
                    }
                }
                setInstanceSaveData(instanceId,dataEnc,bossNum);
                break;
                }
                */
            }               
        
        }
        ++count;
    } while (result->NextRow());
    sLog->outString();
    sLog->outString(">> Loaded %u raid encounter boss data.", count);
}
