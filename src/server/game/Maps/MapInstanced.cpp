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
#include "MapInstanced.h"
#include "ObjectMgr.h"
#include "MapManager.h"
#include "Battleground.h"
#include "VMapFactory.h"
#include "InstanceSaveMgr.h"
#include "World.h"
#include "InstanceScript.h"

MapInstanced::MapInstanced(uint32 id, time_t expiry) : Map(id, expiry, 0, DUNGEON_DIFFICULTY_NORMAL)
{
    // initialize instanced maps list
    m_InstancedMaps.clear();
    // fill with zero
    memset(&GridMapReference, 0, MAX_NUMBER_OF_GRIDS*MAX_NUMBER_OF_GRIDS*sizeof(uint16));
}

void MapInstanced::InitVisibilityDistance()
{
    if (m_InstancedMaps.empty())
        return;
    //initialize visibility distances for all instance copies
    for (InstancedMaps::iterator i = m_InstancedMaps.begin(); i != m_InstancedMaps.end(); ++i)
    {
        (*i).second->InitVisibilityDistance();
    }
}

void MapInstanced::Update(const uint32& t)
{
    // take care of loaded GridMaps (when unused, unload it!)
    Map::Update(t);

    // update the instanced maps
    InstancedMaps::iterator i = m_InstancedMaps.begin();

    while (i != m_InstancedMaps.end())
    {
        if (i->second->CanUnload(t))
        {
            if (!DestroyInstance(i))                             // iterator incremented
            {
                //m_unloadTimer
            }
        }
        else
        {
            // update only here, because it may schedule some bad things before delete
            if (sMapMgr->GetMapUpdater()->activated())
                sMapMgr->GetMapUpdater()->schedule_update(*i->second, t);
            else
                i->second->Update(t);
            ++i;
        }
    }
}

void MapInstanced::DelayedUpdate(const uint32 diff)
{
    for (InstancedMaps::iterator i = m_InstancedMaps.begin(); i != m_InstancedMaps.end(); ++i)
        i->second->DelayedUpdate(diff);

    Map::DelayedUpdate(diff); // this may be removed
}

/*
void MapInstanced::RelocationNotify()
{
    for (InstancedMaps::iterator i = m_InstancedMaps.begin(); i != m_InstancedMaps.end(); ++i)
        i->second->RelocationNotify();
}
*/

void MapInstanced::UnloadAll()
{
    // Unload instanced maps
    for (InstancedMaps::iterator i = m_InstancedMaps.begin(); i != m_InstancedMaps.end(); ++i)
        i->second->UnloadAll();

    // Delete the maps only after everything is unloaded to prevent crashes
    for (InstancedMaps::iterator i = m_InstancedMaps.begin(); i != m_InstancedMaps.end(); ++i)
        delete i->second;

    m_InstancedMaps.clear();

    // Unload own grids (just dummy(placeholder) grids, neccesary to unload GridMaps!)
    Map::UnloadAll();
}

/*
- return the right instance for the object, based on its InstanceId
- create the instance if it's not created already
- the player is not actually added to the instance (only in InstanceMap::Add)
*/
Map* MapInstanced::CreateInstance(const uint32 mapId, Player * player)
{
    if (GetId() != mapId || !player)
        return NULL;
 
    Map* map = NULL;
    uint32 NewInstanceId = 0;                       // instanceId of the resulting map

    if (IsBattlegroundOrArena())
    {
        // instantiate or find existing bg map for player
        // the instance id is set in battlegroundid
        NewInstanceId = player->GetBattlegroundId();
        if (!NewInstanceId) return NULL;
        map = _FindMap(NewInstanceId);
        if (!map && player->GetBattleground())
            map = CreateBattleground(NewInstanceId, player->GetBattleground());
    }
    else
    {
        Group *group = player->GetGroup();
        InstanceGroupBind *groupBind = NULL;
        InstancePlayerBind *pBind;
        bool raid=false;
        if(sInstanceSaveMgr->isFlexibleEnabled(mapId) && IsRaid()) //merge only if killed bosses are set
            raid=true;
        if(group)
            groupBind = group->GetBoundInstance(this);
        if(raid)
        {
            if(group)
            {
                if (!groupBind)//there is no group bind, so we need to create one
                {
                    NewInstanceId = sMapMgr->GenerateInstanceId();
                    map = CreateInstance(NewInstanceId, NULL, FLEXIBLE_RAID_DIFFICULTY);
                    if(map)//save created succesfully
                        groupBind=group->BindToInstanceRaid(NewInstanceId, mapId);
                }

            }
            pBind= player->GetBoundInstance(GetId(), FLEXIBLE_RAID_DIFFICULTY);
            if(pBind)
            {
                if(groupBind && (pBind->save->GetInstanceId() != groupBind->save->GetInstanceId() || pBind->save->GetDifficulty() != groupBind->save->GetDifficulty()))//Player enters another instance ID then before or another raid difficulty => need to show binding query, when teleported
                    player->showInstanceBindQuery=true;
            }

            player->_LoadBoundInstance(mapId);//load player alone bound, not bound of the last leader(its not up because of prevent disappearing ID from server somehow)
            pBind= player->GetBoundInstance(GetId(), FLEXIBLE_RAID_DIFFICULTY);

            if(!pBind)
            {
                NewInstanceId = sMapMgr->GenerateInstanceId();
                /*Player enters raid for the first time, so we need create his unique ID, which will be updated with leaders*/
                map = CreateInstance(NewInstanceId, NULL, FLEXIBLE_RAID_DIFFICULTY);
                pBind= player->BindToInstanceRaid(NewInstanceId, mapId);
                player->showInstanceBindQuery=true;
                std::ostringstream ssPrint;
                ssPrint << "Creating save for player: "<< player->GetGUIDLow() << " map id: " << mapId  << " instance id: " << NewInstanceId << " difficulty: " ;//log
                if(groupBind)
                    ssPrint << groupBind->save->GetDifficulty();
                else
                    ssPrint << player->GetDifficulty(true);
                sLog->outChar("%s", ssPrint.str().c_str());
            }
        }
        else
            pBind= player->GetBoundInstance(GetId(), player->GetDifficulty(IsRaid()));

        InstanceSave *pSave = pBind ? pBind->save : NULL;

        // the player's permanent player bind is taken into consideration first
        // then the player's group bind and finally the solo bind.
        if (!pBind || !pBind->perm)
        {
            // use the player's difficulty setting (it may not be the same as the group's)
            if (groupBind)
                pSave = groupBind->save;
        }
        //instance bound merging when same bosses killed(flexible id)
        if (player->GetSession()->GetSecurity() == SEC_PLAYER && group && pBind && groupBind && raid)
        {
            std::map<uint32,uint32> bossP = sInstanceSaveMgr->getInstanceSaveData(pSave->GetInstanceId());
            std::map<uint32,uint32> bossG = sInstanceSaveMgr->getInstanceSaveData(groupBind->save->GetInstanceId());
            uint32 count = bossP.size(); //we take size of map, not boss number from manager, because we need to compare iCe bosses too
            if (count && !bossP.empty() && !bossG.empty())
            {
                bool canMerge=true;
                std::ostringstream ssP;
                std::ostringstream ssG;
                std::ostringstream ssPrint;
                for (uint32 i = 0; i < count; i++)
                {
                    ssP << bossP[i] << ' ';
                    ssG << bossG[i] << ' ';
                    if (bossP[i] == DONE && bossG[i] != DONE)
                    {
                        canMerge=false;
                        break;
                    }
                }

                if(canMerge)
                {
                    ssPrint << "Merging save player: "<< player->GetGUIDLow() << " map id: " << mapId  << " instance id: " << pSave->GetInstanceId() << " difficulty " << groupBind->save->GetDifficulty() <<" instance data " << ssP.str().c_str() <<" INTO group: "<< group->GetLowGUID() <<" instance id: "<< groupBind->save->GetInstanceId() <<" instance data " << ssG.str().c_str();//log for instance merging
                    sLog->outChar("%s", ssPrint.str().c_str());
                    player->BindToInstance(groupBind->save,true,true);
                    pSave = groupBind->save;                             
                }
            }
        }

        if (pSave)
        {
            // solo/perm/group
            NewInstanceId = pSave->GetInstanceId();
            map = _FindMap(NewInstanceId);
            // it is possible that the save exists but the map doesn't
            if (!map)
            {
                if(raid)
                {
                    map = CreateInstance(NewInstanceId, pSave, FLEXIBLE_RAID_DIFFICULTY);
                }
                else
                    map = CreateInstance(NewInstanceId, pSave, pSave->GetDifficulty());
            }
            if (raid && map->ToInstanceMap()->GetInstanceScript() && !map->ToInstanceMap()->GetInstanceScript()->CorrectDiff(player))
            {
                if(GetMapDifficultyData(map->GetId(),Difficulty(map->ToInstanceMap()->GetInstanceScript()->getPlayerDifficulty(player))))
                    map->ToInstanceMap()->GetInstanceScript()->repairDifficulty(player);
            }
            if (raid && map->ToInstanceMap()->GetInstanceScript() && (player->getRaidDiffProgr(mapId) == KILLED_HC_N_MERGED) && (map->ToInstanceMap()->GetDifficulty() == RAID_DIFFICULTY_10MAN_HEROIC || map->ToInstanceMap()->GetDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC))//player somehow got into HC even if he couldnt
                 map->ToInstanceMap()->setSpawnMode(RAID_DIFFICULTY_10MAN_NORMAL);//swap to normal
        }
        else
        {
            // if no instanceId via group members or instance saves is found
            // the instance will be created for the first time
            NewInstanceId = sMapMgr->GenerateInstanceId();

            Difficulty diff = player->GetGroup() ? player->GetGroup()->GetDifficulty(IsRaid()) : player->GetDifficulty(IsRaid());
            if(raid)
            {
                map = CreateInstance(NewInstanceId, NULL, FLEXIBLE_RAID_DIFFICULTY);
                if(map->ToInstanceMap()->GetInstanceScript() && GetMapDifficultyData(map->GetId(),Difficulty(map->ToInstanceMap()->GetInstanceScript()->getPlayerDifficulty(player))))
                     map->ToInstanceMap()->GetInstanceScript()->repairDifficulty(player);
                if (map->ToInstanceMap()->GetInstanceScript() && (player->getRaidDiffProgr(mapId) == KILLED_HC_N_MERGED) && (map->ToInstanceMap()->GetDifficulty() == RAID_DIFFICULTY_10MAN_HEROIC || map->ToInstanceMap()->GetDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC))//player somehow got into HC even if he couldnt
                    map->ToInstanceMap()->setSpawnMode(RAID_DIFFICULTY_10MAN_NORMAL);//swap to normal
            }
            else
                map = CreateInstance(NewInstanceId, NULL, diff);
        }
    }

    return map;
}

InstanceMap* MapInstanced::CreateInstance(uint32 InstanceId, InstanceSave *save, Difficulty difficulty)
{
    // load/create a map
    ACE_GUARD_RETURN(ACE_Thread_Mutex, Guard, Lock, NULL);

    // make sure we have a valid map id
    const MapEntry* entry = sMapStore.LookupEntry(GetId());
    if (!entry)
    {
        sLog->outError("CreateInstance: no entry for map %d", GetId());
        ASSERT(false);
    }
    const InstanceTemplate * iTemplate = sObjectMgr->GetInstanceTemplate(GetId());
    if (!iTemplate)
    {
        sLog->outError("CreateInstance: no instance template for map %d", GetId());
        ASSERT(false);
    }

    // some instances only have one difficulty
    GetDownscaledMapDifficultyData(GetId(),difficulty);

    sLog->outDebug("MapInstanced::CreateInstance: %s map instance %d for %d created with difficulty %s", save?"":"new ", InstanceId, GetId(), difficulty?"heroic":"normal");

    InstanceMap *map = new InstanceMap(GetId(), GetGridExpiry(), InstanceId, difficulty, this);
    ASSERT(map->IsDungeon());

    bool load_data = save != NULL;
    map->CreateInstanceData(load_data);

    m_InstancedMaps[InstanceId] = map;
    return map;
}

BattlegroundMap* MapInstanced::CreateBattleground(uint32 InstanceId, Battleground* bg)
{
    if (!bg)
        return NULL;

    // load/create a map
    ACE_GUARD_RETURN(ACE_Thread_Mutex, Guard, Lock, NULL);

    sLog->outDebug("MapInstanced::CreateBattleground: map bg %d for %d created.", InstanceId, GetId());

    PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bg->GetMapId(),bg->GetMinLevel());

    uint8 spawnMode;

    if (bracketEntry)
        spawnMode = bracketEntry->difficulty;
    else
        spawnMode = REGULAR_DIFFICULTY;

    BattlegroundMap *map = new BattlegroundMap(GetId(), GetGridExpiry(), InstanceId, this, spawnMode);
    ASSERT(map->IsBattlegroundOrArena());
    map->SetBG(bg);
    bg->SetBgMap(map);

    m_InstancedMaps[InstanceId] = map;
    return map;
}

// increments the iterator after erase
bool MapInstanced::DestroyInstance(InstancedMaps::iterator &itr)
{
    itr->second->RemoveAllPlayers();
    if (itr->second->HavePlayers())
    {
        ++itr;
        return false;
    }

    itr->second->UnloadAll();
    // should only unload VMaps if this is the last instance and grid unloading is enabled
    if (m_InstancedMaps.size() <= 1 && sWorld->getBoolConfig(CONFIG_GRID_UNLOAD))
    {
        VMAP::VMapFactory::createOrGetVMapManager()->unloadMap(itr->second->GetId());
        // in that case, unload grids of the base map, too
        // so in the next map creation, (EnsureGridCreated actually) VMaps will be reloaded
        Map::UnloadAll();
    }
    // erase map
    delete itr->second;
    m_InstancedMaps.erase(itr++);
    return true;
}

bool MapInstanced::CanEnter(Player * /*player*/)
{
    //ASSERT(false);
    return true;
}
