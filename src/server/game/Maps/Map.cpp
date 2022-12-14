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
#include "Map.h"
#include "GridStates.h"
#include "ScriptMgr.h"
#include "VMapFactory.h"
#include "MMapFactory.h"
#include "MapInstanced.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Transport.h"
#include "InstanceScript.h"
#include "ObjectAccessor.h"
#include "MapManager.h"
#include "ObjectMgr.h"
#include "DynamicTree.h"
#include "Vehicle.h"

union u_map_magic
{
    char asChar[4];
    uint32 asUInt;
};

u_map_magic MapMagic        = { {'M','A','P','S'} };
u_map_magic MapVersionMagic = { {'v','1','.','3'} };
u_map_magic MapAreaMagic    = { {'A','R','E','A'} };
u_map_magic MapHeightMagic  = { {'M','H','G','T'} };
u_map_magic MapLiquidMagic  = { {'M','L','I','Q'} };

#define DEFAULT_GRID_EXPIRY     300
#define MAX_GRID_LOAD_TIME      50
#define MAX_CREATURE_ATTACK_RADIUS  (45.0f * sWorld->getRate(RATE_CREATURE_AGGRO))

GridState* si_GridStates[MAX_GRID_STATE];

Map::~Map()
{
    sScriptMgr->OnDestroyMap(this);

    UnloadAll();

    while (!i_worldObjects.empty())
    {
        WorldObject *obj = *i_worldObjects.begin();
        ASSERT(obj->m_isWorldObject);
        //ASSERT(obj->GetTypeId() == TYPEID_CORPSE);
        obj->RemoveFromWorld();
        obj->ResetMap();
    }

    if (!m_scriptSchedule.empty())
        sWorld->DecreaseScheduledScriptCount(m_scriptSchedule.size());

    MMAP::MMapFactory::createOrGetMMapManager()->unloadMapInstance(GetId(), i_InstanceId);
}

bool Map::ExistMap(uint32 mapid,int gx,int gy)
{
    int len = sWorld->GetDataPath().length()+strlen("maps/%03u%02u%02u.map")+1;
    char* tmp = new char[len];
    snprintf(tmp, len, (char *)(sWorld->GetDataPath()+"maps/%03u%02u%02u.map").c_str(),mapid,gx,gy);

    bool ret = false;
    FILE *pf=fopen(tmp,"rb");

    if (!pf)
        sLog->outError("Map file '%s': does not exist!",tmp);
    else
    {
        map_fileheader header;
        if (fread(&header, sizeof(header), 1, pf) == 1)
        {
            if (header.mapMagic != uint32(MAP_MAGIC) || (header.versionMagic != uint32(MAP_VERSION_MAGIC) && header.versionMagic != uint32(MAP_VERSION_MAGIC_ALT)))
                sLog->outError("Map file '%s' is from an incompatible clientversion. Please recreate using the mapextractor.",tmp);
            else
                ret = true;
        }
       fclose(pf);
    }
    delete [] tmp;
    return ret;
}

bool Map::ExistVMap(uint32 mapid,int gx,int gy)
{
    if (VMAP::IVMapManager* vmgr = VMAP::VMapFactory::createOrGetVMapManager())
    {
        if (vmgr->isMapLoadingEnabled())
        {
            bool exists = vmgr->existsMap((sWorld->GetDataPath()+ "vmaps").c_str(),  mapid, gx,gy);
            if (!exists)
            {
                std::string name = vmgr->getDirFileName(mapid,gx,gy);
                sLog->outError("VMap file '%s' is missing or points to wrong version of vmap file. Redo vmaps with latest version of vmap_assembler.exe.", (sWorld->GetDataPath()+"vmaps/"+name).c_str());
                return false;
            }
        }
    }

    return true;
}

void Map::LoadMMap(int gx, int gy)
{
    bool mmapLoadResult = MMAP::MMapFactory::createOrGetMMapManager()->loadMap((sWorld->GetDataPath() + "mmaps").c_str(), GetId(), gx, gy);

    if (mmapLoadResult)
        sLog->outDetail("MMAP loaded name:%s, id:%d, x:%d, y:%d (mmap rep.: x:%d, y:%d)", GetMapName(), GetId(), gx, gy, gx, gy);
    else
        sLog->outDetail("Could not load MMAP name:%s, id:%d, x:%d, y:%d (mmap rep.: x:%d, y:%d)", GetMapName(), GetId(), gx, gy, gx, gy);
}

void Map::LoadVMap(int gx, int gy)
{
    int vmapLoadResult = VMAP::VMapFactory::createOrGetVMapManager()->loadMap((sWorld->GetDataPath()+ "vmaps").c_str(),  GetId(), gx,gy);
    switch(vmapLoadResult)
    {
        case VMAP::VMAP_LOAD_RESULT_OK:
            sLog->outDetail("VMAP loaded name:%s, id:%d, x:%d, y:%d (vmap rep.: x:%d, y:%d)", GetMapName(), GetId(), gx,gy,gx,gy);
            break;
        case VMAP::VMAP_LOAD_RESULT_ERROR:
            sLog->outDetail("Could not load VMAP name:%s, id:%d, x:%d, y:%d (vmap rep.: x:%d, y:%d)", GetMapName(), GetId(), gx,gy,gx,gy);
            break;
        case VMAP::VMAP_LOAD_RESULT_IGNORED:
            sLog->outStaticDebug("Ignored VMAP name:%s, id:%d, x:%d, y:%d (vmap rep.: x:%d, y:%d)", GetMapName(), GetId(), gx,gy,gx,gy);
            break;
    }
}

void Map::LoadMap(int gx,int gy, bool reload)
{
    if (i_InstanceId != 0)
    {
        if (GridMaps[gx][gy])
            return;

        // load grid map for base map
        if (!m_parentMap->GridMaps[gx][gy])
            m_parentMap->EnsureGridCreated(GridPair(63-gx,63-gy));

        ((MapInstanced*)(m_parentMap))->AddGridMapReference(GridPair(gx,gy));
        GridMaps[gx][gy] = m_parentMap->GridMaps[gx][gy];
        return;
    }

    if (GridMaps[gx][gy] && !reload)
        return;

    //map already load, delete it before reloading (Is it necessary? Do we really need the ability the reload maps during runtime?)
    if (GridMaps[gx][gy])
    {
        sLog->outDetail("Unloading previously loaded map %u before reloading.",GetId());
        sScriptMgr->OnUnloadGridMap(this, GridMaps[gx][gy], gx, gy);

        delete (GridMaps[gx][gy]);
        GridMaps[gx][gy]=NULL;
    }

    // map file name
    char* tmp = NULL;
    int len = sWorld->GetDataPath().length() + strlen("maps/%03u%02u%02u.map") + 1;
    tmp = new char[len];
    snprintf(tmp, len, (char *)(sWorld->GetDataPath()+"maps/%03u%02u%02u.map").c_str(),GetId(),gx,gy);
    sLog->outDetail("Loading map %s",tmp);
    // loading data
    GridMaps[gx][gy] = new GridMap();
    dontDump(GridMaps[gx][gy], sizeof(GridMap));
    if (!GridMaps[gx][gy]->loadData(tmp))
    {
        sLog->outError("Error loading map file: \n %s\n", tmp);
    }
    delete [] tmp;

    sScriptMgr->OnLoadGridMap(this, GridMaps[gx][gy], gx, gy);
}

void Map::LoadMapAndVMap(int gx,int gy)
{
    LoadMap(gx, gy);
   // Only load the data for the base map
    if (i_InstanceId == 0)
    {
        LoadVMap(gx, gy);
        LoadMMap(gx, gy);
    }
}

void Map::InitStateMachine()
{
    si_GridStates[GRID_STATE_INVALID] = new InvalidState;
    si_GridStates[GRID_STATE_ACTIVE] = new ActiveState;
    si_GridStates[GRID_STATE_IDLE] = new IdleState;
    si_GridStates[GRID_STATE_REMOVAL] = new RemovalState;
}

void Map::DeleteStateMachine()
{
    delete si_GridStates[GRID_STATE_INVALID];
    delete si_GridStates[GRID_STATE_ACTIVE];
    delete si_GridStates[GRID_STATE_IDLE];
    delete si_GridStates[GRID_STATE_REMOVAL];
}

Map::Map(uint32 id, time_t expiry, uint32 InstanceId, uint8 SpawnMode, Map* _parent):
i_mapEntry (sMapStore.LookupEntry(id)), i_spawnMode(SpawnMode), i_InstanceId(InstanceId),
m_unloadTimer(0), m_VisibleDistance(DEFAULT_VISIBILITY_DISTANCE),
m_VisibilityNotifyPeriod(DEFAULT_VISIBILITY_NOTIFY_PERIOD),
m_activeNonPlayersIter(m_activeNonPlayers.end()), _transportsUpdateIter(_transports.end()),
i_gridExpiry(expiry), i_scriptLock(false)
{
    m_parentMap = (_parent ? _parent : this);
    for (unsigned int idx=0; idx < MAX_NUMBER_OF_GRIDS; ++idx)
    {
        for (unsigned int j=0; j < MAX_NUMBER_OF_GRIDS; ++j)
        {
            //z code
            GridMaps[idx][j] =NULL;
            setNGrid(NULL, idx, j);
        }
    }

    //lets initialize visibility distance for map
    Map::InitVisibilityDistance();

    sScriptMgr->OnCreateMap(this);
}

void Map::InitVisibilityDistance()
{
    //init visibility for continents
    m_VisibleDistance = World::GetMaxVisibleDistanceOnContinents();
    m_VisibilityNotifyPeriod = World::GetVisibilityNotifyPeriodOnContinents();
}

// Template specialization of utility methods
template<class T>
void Map::AddToGrid(T* obj, NGridType *grid, Cell const& cell)
{
    if (obj->m_isWorldObject)
        (*grid)(cell.CellX(), cell.CellY()).template AddWorldObject<T>(obj);
    else
        (*grid)(cell.CellX(), cell.CellY()).template AddGridObject<T>(obj);
}

template<>
void Map::AddToGrid(Creature* obj, NGridType *grid, Cell const& cell)
{
    if (obj->m_isWorldObject)
        (*grid)(cell.CellX(), cell.CellY()).AddWorldObject(obj);
    else
        (*grid)(cell.CellX(), cell.CellY()).AddGridObject(obj);

    obj->SetCurrentCell(cell);
}

template<>
void Map::AddToGrid(GameObject* obj, NGridType* grid, Cell const& cell)
{
    if (obj->m_isWorldObject)
        (*grid)(cell.CellX(), cell.CellY()).AddWorldObject(obj);
    else
        (*grid)(cell.CellX(), cell.CellY()).AddGridObject(obj);

    obj->SetCurrentCell(cell);
}

template<class T>
void Map::RemoveFromGrid(T* obj, NGridType *grid, Cell const& cell)
{
    if (obj->m_isWorldObject)
        (*grid)(cell.CellX(), cell.CellY()).template RemoveWorldObject<T>(obj);
    else
        (*grid)(cell.CellX(), cell.CellY()).template RemoveGridObject<T>(obj);
}

template<class T>
void Map::SwitchGridContainers(T* /*obj*/, bool /*on*/)
{
}

template<>
void Map::SwitchGridContainers(Creature* obj, bool on)
{
    CellPair p = Trinity::ComputeCellPair(obj->GetPositionX(), obj->GetPositionY());
    if (p.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || p.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
    {
        sLog->outError("Map::SwitchGridContainers: Object " UI64FMTD " has invalid coordinates X:%f Y:%f grid cell [%u:%u]", obj->GetGUID(), obj->GetPositionX(), obj->GetPositionY(), p.x_coord, p.y_coord);
        return;
    }

    Cell cell(p);
    if (!loaded(GridPair(cell.data.Part.grid_x, cell.data.Part.grid_y)))
        return;

    sLog->outStaticDebug("Switch object " UI64FMTD " from grid[%u,%u] %u", obj->GetGUID(), cell.data.Part.grid_x, cell.data.Part.grid_y, on);
    NGridType *ngrid = getNGrid(cell.GridX(), cell.GridY());
    ASSERT(ngrid != NULL);

    GridType &grid = (*ngrid)(cell.CellX(), cell.CellY());

    if (on)
    {
        grid.RemoveGridObject<Creature>(obj);
        grid.AddWorldObject<Creature>(obj);
        /*if (!grid.RemoveGridObject<T>(obj, obj->GetGUID())
            || !grid.AddWorldObject<T>(obj, obj->GetGUID()))
        {
            ASSERT(false);
        }*/
    }
    else
    {
        grid.RemoveWorldObject<Creature>(obj);
        grid.AddGridObject<Creature>(obj);
        /*if (!grid.RemoveWorldObject<T>(obj, obj->GetGUID())
            || !grid.AddGridObject<T>(obj, obj->GetGUID()))
        {
            ASSERT(false);
        }*/
    }
    obj->m_isWorldObject = on;
}

template<>
void Map::SwitchGridContainers(GameObject* obj, bool on)
{
    CellPair p = Trinity::ComputeCellPair(obj->GetPositionX(), obj->GetPositionY());
    if (p.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || p.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
    {
        sLog->outError("Map::SwitchGridContainers: Object " UI64FMTD " has invalid coordinates X:%f Y:%f grid cell [%u:%u]", obj->GetGUID(), obj->GetPositionX(), obj->GetPositionY(), p.x_coord, p.y_coord);
        return;
    }

    Cell cell(p);
    if (!loaded(GridPair(cell.data.Part.grid_x, cell.data.Part.grid_y)))
        return;

    sLog->outDebug("Switch object " UI64FMTD " from grid[%u, %u] %u", obj->GetGUID(), cell.data.Part.grid_x, cell.data.Part.grid_y, on);
    NGridType *ngrid = getNGrid(cell.GridX(), cell.GridY());
    ASSERT(ngrid != NULL);

    GridType &grid = ngrid->getGridType(cell.CellX(), cell.CellY());

    RemoveFromGrid<GameObject>(obj, ngrid, cell); //This step is not really necessary but we want to do ASSERT in remove/add

    if (on)
    {
        grid.AddWorldObject(obj);
        AddWorldObject(obj);
    }
    else
    {
        grid.AddGridObject(obj);
        RemoveWorldObject(obj);
    }
}

template void Map::SwitchGridContainers(Creature *, bool);
//template void Map::SwitchGridContainers(DynamicObject *, bool);

template<class T>
void Map::DeleteFromWorld(T* obj)
{
    // Note: In case resurrectable corpse and pet its removed from global lists in own destructor
    delete obj;
}

template<>
void Map::DeleteFromWorld(Player* pl)
{
    sObjectAccessor->RemoveObject(pl);
    delete pl;
}

void
Map::EnsureGridCreated(const GridPair &p)
{
    if (!getNGrid(p.x_coord, p.y_coord))
    {
        ACE_GUARD(ACE_Thread_Mutex, Guard, Lock);
        if (!getNGrid(p.x_coord, p.y_coord))
        {
            sLog->outDebug("Creating grid[%u,%u] for map %u instance %u", p.x_coord, p.y_coord, GetId(), i_InstanceId);

            setNGrid(new NGridType(p.x_coord*MAX_NUMBER_OF_GRIDS + p.y_coord, p.x_coord, p.y_coord, i_gridExpiry, sWorld->getBoolConfig(CONFIG_GRID_UNLOAD)),
                p.x_coord, p.y_coord);

            // build a linkage between this map and NGridType
            buildNGridLinkage(getNGrid(p.x_coord, p.y_coord));

            getNGrid(p.x_coord, p.y_coord)->SetGridState(GRID_STATE_IDLE);

            //z coord
            int gx = (MAX_NUMBER_OF_GRIDS - 1) - p.x_coord;
            int gy = (MAX_NUMBER_OF_GRIDS - 1) - p.y_coord;

            if (!GridMaps[gx][gy])
                LoadMapAndVMap(gx,gy);
        }
    }
}

void
Map::EnsureGridLoadedAtEnter(const Cell &cell, Player *player)
{
    EnsureGridLoaded(cell);
    NGridType *grid = getNGrid(cell.GridX(), cell.GridY());
    ASSERT(grid != NULL);

    // refresh grid state & timer
    if (grid->GetGridState() != GRID_STATE_ACTIVE)
    {
        if (player)
        {
            sLog->outStaticDebug("Player %s enter cell[%u,%u] triggers loading of grid[%u,%u] on map %u", player->GetName(), cell.CellX(), cell.CellY(), cell.GridX(), cell.GridY(), GetId());
        }
        else
        {
            sLog->outStaticDebug("Active object nearby triggers loading of grid [%u,%u] on map %u", cell.GridX(), cell.GridY(), GetId());
        }

        ResetGridExpiry(*grid, 0.1f);
        grid->SetGridState(GRID_STATE_ACTIVE);
    }
}

bool Map::EnsureGridLoaded(const Cell &cell)
{
    EnsureGridCreated(GridPair(cell.GridX(), cell.GridY()));
    NGridType *grid = getNGrid(cell.GridX(), cell.GridY());

    ASSERT(grid != NULL);
    if (!isGridObjectDataLoaded(cell.GridX(), cell.GridY()))
    {
        sLog->outDebug("Loading grid[%u,%u] for map %u instance %u", cell.GridX(), cell.GridY(), GetId(), i_InstanceId);

        setGridObjectDataLoaded(true,cell.GridX(), cell.GridY());

        ObjectGridLoader loader(*grid, this, cell);
        loader.LoadN();

        // Add resurrectable corpses to world object list in grid
        sObjectAccessor->AddCorpsesToGrid(GridPair(cell.GridX(),cell.GridY()),(*grid)(cell.CellX(), cell.CellY()), this);
        Balance();
        return true;
    }

    return false;
}

void Map::LoadGrid(float x, float y)
{
    CellPair pair = Trinity::ComputeCellPair(x, y);
    Cell cell(pair);
    EnsureGridLoaded(cell);
}

bool Map::Add(Player *player)
{
    // Check if we are adding to correct map
    ASSERT (player->GetMap() == this);
    CellPair p = Trinity::ComputeCellPair(player->GetPositionX(), player->GetPositionY());
    if (p.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || p.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
    {
        sLog->outError("Map::Add: Player (GUID: %u) has invalid coordinates X:%f Y:%f grid cell [%u:%u]", player->GetGUIDLow(), player->GetPositionX(), player->GetPositionY(), p.x_coord, p.y_coord);
        return false;
    }

    player->SetMap(this);

    Cell cell(p);
    EnsureGridLoadedAtEnter(cell, player);
    NGridType *grid = getNGrid(cell.GridX(), cell.GridY());
    ASSERT(grid != NULL);
    AddToGrid(player, grid, cell);

    player->AddToWorld();

    SendInitSelf(player);
    SendInitTransports(player);

    player->m_clientGUIDs.clear();
    player->UpdateObjectVisibility(true);

    sScriptMgr->OnPlayerEnterMap(this, player);
    return true;
}

template<>
void Map::Add(Transport* obj)
{
    //TODO: Needs clean up. An object should not be added to map twice.
    if (obj->IsInWorld())
        return;

    CellPair cellCoord = Trinity::ComputeCellPair(obj->GetPositionX(), obj->GetPositionY());
    if (cellCoord.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || cellCoord.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
    {
        sLog->outError("Map::Add: Object " UI64FMTD " has invalid coordinates X:%f Y:%f grid cell [%u:%u]", obj->GetGUID(), obj->GetPositionX(), obj->GetPositionY(), cellCoord.x_coord, cellCoord.y_coord);
        return; //Should delete object
    }

    obj->AddToWorld();
    _transports.insert(obj);

    // Broadcast creation to players
    if (!GetPlayers().isEmpty())
    {
        for (Map::PlayerList::const_iterator itr = GetPlayers().begin(); itr != GetPlayers().end(); ++itr)
        {
            if (itr->getSource()->GetTransport() != obj)
            {
                UpdateData data(GetId());
                obj->BuildCreateUpdateBlockForPlayer(&data, itr->getSource());
                WorldPacket packet;
                data.BuildPacket(&packet);
                itr->getSource()->SendDirectMessage(&packet);
            }
        }
    }
}

template<>
void Map::Add(GameObject *obj)
{
    CellPair p = Trinity::ComputeCellPair(obj->GetPositionX(), obj->GetPositionY());
    if (p.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || p.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
    {
        sLog->outError("Map::Add: Object " UI64FMTD " has invalid coordinates X:%f Y:%f grid cell [%u:%u]", obj->GetGUID(), obj->GetPositionX(), obj->GetPositionY(), p.x_coord, p.y_coord);
        return;
    }

    Cell cell(p);
    if (obj->IsInWorld()) // need some clean up later
    {
        obj->UpdateObjectVisibility(true);
        return;
    }

    if (obj->isActiveObject())
        EnsureGridLoadedAtEnter(cell);
    else
        EnsureGridCreated(GridPair(cell.GridX(), cell.GridY()));

    NGridType *grid = getNGrid(cell.GridX(), cell.GridY());
    ASSERT(grid != NULL);

    AddToGrid(obj, grid, cell);
    //obj->SetMap(this);
    obj->AddToWorld();

    if (obj->GetTypeId() == TYPEID_GAMEOBJECT)
        obj->_moveState = MAP_OBJECT_CELL_MOVE_NONE;

    if (obj->isActiveObject())
        AddToActive(obj);

    if (obj->GetGoType() == GAMEOBJECT_TYPE_TRANSPORT)
        _transports.insert(obj);

    sLog->outStaticDebug("Object %u enters grid[%u,%u]", GUID_LOPART(obj->GetGUID()), cell.GridX(), cell.GridY());

    //something, such as vehicle, needs to be update immediately
    //also, trigger needs to cast spell, if not update, cannot see visual
    obj->UpdateObjectVisibility(true);
}

template<class T>
void
Map::Add(T *obj)
{
    CellPair p = Trinity::ComputeCellPair(obj->GetPositionX(), obj->GetPositionY());
    if (p.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || p.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
    {
        sLog->outError("Map::Add: Object " UI64FMTD " has invalid coordinates X:%f Y:%f grid cell [%u:%u]", obj->GetGUID(), obj->GetPositionX(), obj->GetPositionY(), p.x_coord, p.y_coord);
        return;
    }

    Cell cell(p);
    if (obj->IsInWorld()) // need some clean up later
    {
        obj->UpdateObjectVisibility(true);
        return;
    }

    if (obj->isActiveObject())
        EnsureGridLoadedAtEnter(cell);
    else
        EnsureGridCreated(GridPair(cell.GridX(), cell.GridY()));

    NGridType *grid = getNGrid(cell.GridX(), cell.GridY());
    ASSERT(grid != NULL);

    AddToGrid(obj, grid, cell);
    //obj->SetMap(this);
    obj->AddToWorld();

    if (obj->isActiveObject())
        AddToActive(obj);

    sLog->outStaticDebug("Object %u enters grid[%u,%u]", GUID_LOPART(obj->GetGUID()), cell.GridX(), cell.GridY());

    //something, such as vehicle, needs to be update immediately
    //also, trigger needs to cast spell, if not update, cannot see visual
    obj->UpdateObjectVisibility(true);
}

bool Map::loaded(const GridPair &p) const
{
    return (getNGrid(p.x_coord, p.y_coord) && isGridObjectDataLoaded(p.x_coord, p.y_coord));
}

void Map::Update(const uint32 &t_diff)
{
    m_dyn_tree.update(t_diff);

    /// update players at tick
    for (m_mapRefIter = m_mapRefManager.begin(); m_mapRefIter != m_mapRefManager.end(); ++m_mapRefIter)
    {
        Player* plr = m_mapRefIter->getSource();
        if (plr && plr->IsInWorld())
            plr->Update(t_diff);
    }

    /// update active cells around players and active objects
    resetMarkedCells();

    Trinity::ObjectUpdater updater(t_diff);
    // for creature
    TypeContainerVisitor<Trinity::ObjectUpdater, GridTypeMapContainer  > grid_object_update(updater);
    // for pets
    TypeContainerVisitor<Trinity::ObjectUpdater, WorldTypeMapContainer > world_object_update(updater);

    // the player iterator is stored in the map object
    // to make sure calls to Map::Remove don't invalidate it
    for (m_mapRefIter = m_mapRefManager.begin(); m_mapRefIter != m_mapRefManager.end(); ++m_mapRefIter)
    {
        Player* plr = m_mapRefIter->getSource();

        if (!plr->IsInWorld())
            continue;

        CellPair standing_cell(Trinity::ComputeCellPair(plr->GetPositionX(), plr->GetPositionY()));

        // Check for correctness of standing_cell, it also avoids problems with update_cell
        if (standing_cell.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || standing_cell.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
            continue;

        // the overloaded operators handle range checking
        // so ther's no need for range checking inside the loop
        CellPair begin_cell(standing_cell), end_cell(standing_cell);
        //lets update mobs/objects in ALL visible cells around player!
        CellArea area = Cell::CalculateCellArea(*plr, GetVisibilityDistance());
        area.ResizeBorders(begin_cell, end_cell);

        for (uint32 x = begin_cell.x_coord; x <= end_cell.x_coord; ++x)
        {
            for (uint32 y = begin_cell.y_coord; y <= end_cell.y_coord; ++y)
            {
                // marked cells are those that have been visited
                // don't visit the same cell twice
                uint32 cell_id = (y * TOTAL_NUMBER_OF_CELLS_PER_MAP) + x;
                if (!isCellMarked(cell_id))
                {
                    markCell(cell_id);
                    CellPair pair(x,y);
                    Cell cell(pair);
                    cell.data.Part.reserved = CENTER_DISTRICT;
                    //cell.SetNoCreate();
                    cell.Visit(pair, grid_object_update,  *this);
                    cell.Visit(pair, world_object_update, *this);
                }
            }
        }
    }

    // non-player active objects
    if (!m_activeNonPlayers.empty())
    {
        for (m_activeNonPlayersIter = m_activeNonPlayers.begin(); m_activeNonPlayersIter != m_activeNonPlayers.end();)
        {
            // skip not in world
            WorldObject* obj = *m_activeNonPlayersIter;

            // step before processing, in this case if Map::Remove remove next object we correctly
            // step to next-next, and if we step to end() then newly added objects can wait next update.
            ++m_activeNonPlayersIter;

            if (!obj->IsInWorld())
                continue;

            CellPair standing_cell(Trinity::ComputeCellPair(obj->GetPositionX(), obj->GetPositionY()));

            // Check for correctness of standing_cell, it also avoids problems with update_cell
            if (standing_cell.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || standing_cell.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
                continue;

            // the overloaded operators handle range checking
            // so ther's no need for range checking inside the loop
            CellPair begin_cell(standing_cell), end_cell(standing_cell);
            begin_cell << 1; begin_cell -= 1;               // upper left
            end_cell >> 1; end_cell += 1;                   // lower right

            for (uint32 x = begin_cell.x_coord; x <= end_cell.x_coord; ++x)
            {
                for (uint32 y = begin_cell.y_coord; y <= end_cell.y_coord; ++y)
                {
                    // marked cells are those that have been visited
                    // don't visit the same cell twice
                    uint32 cell_id = (y * TOTAL_NUMBER_OF_CELLS_PER_MAP) + x;
                    if (!isCellMarked(cell_id))
                    {
                        markCell(cell_id);
                        CellPair pair(x,y);
                        Cell cell(pair);
                        cell.data.Part.reserved = CENTER_DISTRICT;
                        //cell.SetNoCreate();
                        cell.Visit(pair, grid_object_update,  *this);
                        cell.Visit(pair, world_object_update, *this);
                    }
                }
            }
        }
    }

    for (_transportsUpdateIter = _transports.begin(); _transportsUpdateIter != _transports.end();)
    {
        WorldObject* obj = *_transportsUpdateIter;
        ++_transportsUpdateIter;

        if (!obj->IsInWorld() || obj->isActiveObject())
            continue;

        obj->Update(t_diff);
    }

    ///- Process necessary scripts
    if (!m_scriptSchedule.empty())
    {
        i_scriptLock = true;
        ScriptsProcess();
        i_scriptLock = false;
    }

    MoveAllCreaturesInMoveList();
    MoveAllGameObjectsInMoveList();

    if (!m_mapRefManager.isEmpty() || !m_activeNonPlayers.empty())
        ProcessRelocationNotifies(t_diff);

    sScriptMgr->OnMapUpdate(this, t_diff);
}

struct ResetNotifier
{
    template<class T>inline void resetNotify(GridRefManager<T> &m)
    {
        for (typename GridRefManager<T>::iterator iter=m.begin(); iter != m.end(); ++iter)
            iter->getSource()->ResetAllNotifies();
    }
    template<class T> void Visit(GridRefManager<T> &) {}
    void Visit(CreatureMapType &m) { resetNotify<Creature>(m);}
    void Visit(PlayerMapType &m) { resetNotify<Player>(m);}
};

void Map::ProcessRelocationNotifies(const uint32 & diff)
{
    for (GridRefManager<NGridType>::iterator i = GridRefManager<NGridType>::begin(); i != GridRefManager<NGridType>::end(); ++i)
    {
        NGridType *grid = i->getSource();

        if (grid->GetGridState() != GRID_STATE_ACTIVE)
            continue;

        grid->getGridInfoRef()->getRelocationTimer().TUpdate(diff);
        if (!grid->getGridInfoRef()->getRelocationTimer().TPassed())
            continue;

        uint32 gx = grid->getX(), gy = grid->getY();

        CellPair cell_min(gx*MAX_NUMBER_OF_CELLS, gy*MAX_NUMBER_OF_CELLS);
        CellPair cell_max(cell_min.x_coord + MAX_NUMBER_OF_CELLS, cell_min.y_coord+MAX_NUMBER_OF_CELLS);

        for (uint32 x = cell_min.x_coord; x < cell_max.x_coord; ++x)
        {
            for (uint32 y = cell_min.y_coord; y < cell_max.y_coord; ++y)
            {
                uint32 cell_id = (y * TOTAL_NUMBER_OF_CELLS_PER_MAP) + x;
                if (!isCellMarked(cell_id))
                    continue;

                CellPair pair(x,y);
                Cell cell(pair);
                cell.SetNoCreate();

                Trinity::DelayedUnitRelocation cell_relocation(cell, pair, *this, GetVisibilityDistance());
                TypeContainerVisitor<Trinity::DelayedUnitRelocation, GridTypeMapContainer  > grid_object_relocation(cell_relocation);
                TypeContainerVisitor<Trinity::DelayedUnitRelocation, WorldTypeMapContainer > world_object_relocation(cell_relocation);
                Visit(cell, grid_object_relocation);
                Visit(cell, world_object_relocation);
            }
        }
    }

    ResetNotifier reset;
    TypeContainerVisitor<ResetNotifier, GridTypeMapContainer >  grid_notifier(reset);
    TypeContainerVisitor<ResetNotifier, WorldTypeMapContainer > world_notifier(reset);
    for (GridRefManager<NGridType>::iterator i = GridRefManager<NGridType>::begin(); i != GridRefManager<NGridType>::end(); ++i)
    {
        NGridType *grid = i->getSource();

        if (grid->GetGridState() != GRID_STATE_ACTIVE)
            continue;

        if (!grid->getGridInfoRef()->getRelocationTimer().TPassed())
            continue;

        grid->getGridInfoRef()->getRelocationTimer().TReset(diff, m_VisibilityNotifyPeriod);

        uint32 gx = grid->getX(), gy = grid->getY();

        CellPair cell_min(gx*MAX_NUMBER_OF_CELLS, gy*MAX_NUMBER_OF_CELLS);
        CellPair cell_max(cell_min.x_coord + MAX_NUMBER_OF_CELLS, cell_min.y_coord+MAX_NUMBER_OF_CELLS);

        for (uint32 x = cell_min.x_coord; x < cell_max.x_coord; ++x)
        {
            for (uint32 y = cell_min.y_coord; y < cell_max.y_coord; ++y)
            {
                uint32 cell_id = (y * TOTAL_NUMBER_OF_CELLS_PER_MAP) + x;
                if (!isCellMarked(cell_id))
                    continue;

                CellPair pair(x,y);
                Cell cell(pair);
                cell.SetNoCreate();
                Visit(cell, grid_notifier);
                Visit(cell, world_notifier);
            }
        }
    }
}

void Map::Remove(Player *player, bool remove)
{
    player->RemoveFromWorld();
    SendRemoveTransports(player);

    CellPair p = Trinity::ComputeCellPair(player->GetPositionX(), player->GetPositionY());
    if (p.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || p.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
        sLog->outCrash("Map::Remove: Player is in invalid cell!");
    else
    {
        Cell cell(p);
        if (!getNGrid(cell.data.Part.grid_x, cell.data.Part.grid_y))
            sLog->outError("Map::Remove() i_grids was NULL x:%d, y:%d",cell.data.Part.grid_x,cell.data.Part.grid_y);
        else
        {
            sLog->outStaticDebug("Remove player %s from grid[%u,%u]", player->GetName(), cell.GridX(), cell.GridY());
            NGridType *grid = getNGrid(cell.GridX(), cell.GridY());
            ASSERT(grid != NULL);

            UpdateObjectVisibility(player, cell, p);
            RemoveFromGrid(player,grid,cell);
        }
    }

    if (remove)
        DeleteFromWorld(player);

    sScriptMgr->OnPlayerLeaveMap(this, player);
}

template<>
void Map::Remove(Transport* obj, bool remove)
{
    obj->RemoveFromWorld();

    if (_transportsUpdateIter != _transports.end())
    {
        TransportsContainer::iterator itr = _transports.find(obj);
        if (itr == _transports.end())
            return;
        if (itr == _transportsUpdateIter)
            ++_transportsUpdateIter;
        _transports.erase(itr);
    }
    else
        _transports.erase(obj);

    obj->ResetMap();

    if (remove)
    {
        // if option set then object already saved at this moment
        if (!sWorld->getBoolConfig(CONFIG_SAVE_RESPAWN_TIME_IMMEDIATELY))
            obj->SaveRespawnTime();
        DeleteFromWorld(obj);
    }
}

template<class T>
void
Map::Remove(T *obj, bool remove)
{
    obj->RemoveFromWorld();
    if (obj->isActiveObject())
        RemoveFromActive(obj);

    CellPair p = Trinity::ComputeCellPair(obj->GetPositionX(), obj->GetPositionY());
    if (p.x_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP || p.y_coord >= TOTAL_NUMBER_OF_CELLS_PER_MAP)
        sLog->outError("Map::Remove: Object " UI64FMTD " has invalid coordinates X:%f Y:%f grid cell [%u:%u]", obj->GetGUID(), obj->GetPositionX(), obj->GetPositionY(), p.x_coord, p.y_coord);
    else
    {
        Cell cell(p);
        if (loaded(GridPair(cell.data.Part.grid_x, cell.data.Part.grid_y)))
        {
            sLog->outStaticDebug("Remove object " UI64FMTD " from grid[%u,%u]", obj->GetGUID(), cell.data.Part.grid_x, cell.data.Part.grid_y);
            NGridType *grid = getNGrid(cell.GridX(), cell.GridY());
            ASSERT(grid != NULL);

            UpdateObjectVisibility(obj, cell, p);
            RemoveFromGrid(obj,grid,cell);
        }
    }

    if (obj->ToGameObject())
    {
        if (_transportsUpdateIter != _transports.end())
        {
            TransportsContainer::iterator itr = _transports.find(obj->ToGameObject());
            if (itr == _transports.end())
                return;
            if (itr == _transportsUpdateIter)
                ++_transportsUpdateIter;
            _transports.erase(itr);
        }
        else
            _transports.erase(obj->ToGameObject());
    }

    obj->ResetMap();

    if (remove)
    {
        // if option set then object already saved at this moment
        if (!sWorld->getBoolConfig(CONFIG_SAVE_RESPAWN_TIME_IMMEDIATELY))
            obj->SaveRespawnTime();
        DeleteFromWorld(obj);
    }
}

void
Map::PlayerRelocation(Player *player, float x, float y, float z, float orientation)
{
    ASSERT(player);

    CellPair old_val = Trinity::ComputeCellPair(player->GetPositionX(), player->GetPositionY());
    CellPair new_val = Trinity::ComputeCellPair(x, y);

    Cell old_cell(old_val);
    Cell new_cell(new_val);

    player->Relocate(x, y, z, orientation);

    if (old_cell.DiffGrid(new_cell) || old_cell.DiffCell(new_cell))
    {
        sLog->outStaticDebug("Player %s relocation grid[%u,%u]cell[%u,%u]->grid[%u,%u]cell[%u,%u]", player->GetName(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY(), new_cell.GridX(), new_cell.GridY(), new_cell.CellX(), new_cell.CellY());

        NGridType* oldGrid = getNGrid(old_cell.GridX(), old_cell.GridY());
        RemoveFromGrid(player, oldGrid,old_cell);

        if (old_cell.DiffGrid(new_cell))
            EnsureGridLoadedAtEnter(new_cell, player);

        NGridType* newGrid = getNGrid(new_cell.GridX(), new_cell.GridY());
        AddToGrid(player, newGrid,new_cell);
    }

    player->UpdateObjectVisibility(false);
}

void
Map::CreatureRelocation(Creature *creature, float x, float y, float z, float ang, bool respawnRelocationOnFail)
{
    ASSERT(CheckGridIntegrity(creature,false));

    Cell old_cell = creature->GetCurrentCell();

    CellPair new_val = Trinity::ComputeCellPair(x, y);
    Cell new_cell(new_val);

    if (!respawnRelocationOnFail && !getNGrid(new_cell.GridX(), new_cell.GridY()))
        return;

    // delay creature move for grid/cell to grid/cell moves
    if (old_cell.DiffCell(new_cell) || old_cell.DiffGrid(new_cell))
    {
        #ifdef TRINITY_DEBUG
        if ((sLog->GetLogFilter() & LOG_FILTER_CREATURE_MOVES) == 0)
            sLog->outDebug("Creature (GUID: %u Entry: %u) added to moving list from grid[%u,%u]cell[%u,%u] to grid[%u,%u]cell[%u,%u].", creature->GetGUIDLow(), creature->GetEntry(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY(), new_cell.GridX(), new_cell.GridY(), new_cell.CellX(), new_cell.CellY());
        #endif
        AddCreatureToMoveList(creature, x, y, z, ang);
        // in diffcell/diffgrid case notifiers called at finishing move creature in Map::MoveAllCreaturesInMoveList
    }
    else
    {
        creature->Relocate(x, y, z, ang);
        creature->SetRelocatedFlag();
        creature->UpdateObjectVisibility(false);
    }

    ASSERT(CheckGridIntegrity(creature,true));
}

void Map::GameObjectRelocation(GameObject* go, float x, float y, float z, float orientation, bool respawnRelocationOnFail)
{
    Cell integrity_check(go->GetPositionX(), go->GetPositionY());
    Cell old_cell = go->GetCurrentCell();

    ASSERT(integrity_check == old_cell);
    Cell new_cell(x, y);

    if (!respawnRelocationOnFail && !getNGrid(new_cell.GridX(), new_cell.GridY()))
        return;

    // delay creature move for grid/cell to grid/cell moves
    if (old_cell.DiffCell(new_cell) || old_cell.DiffGrid(new_cell))
    {
#ifdef TRINITY_DEBUG
        sLog->outDebug("GameObject (GUID: %u Entry: %u) added to moving list from grid[%u, %u]cell[%u, %u] to grid[%u, %u]cell[%u, %u].", go->GetGUIDLow(), go->GetEntry(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY(), new_cell.GridX(), new_cell.GridY(), new_cell.CellX(), new_cell.CellY());
#endif
        AddGameObjectToMoveList(go, x, y, z, orientation);
        // in diffcell/diffgrid case notifiers called at finishing move go in Map::MoveAllGameObjectsInMoveList
    }
    else
    {
        go->Relocate(x, y, z, orientation);
        go->UpdateObjectVisibility(false);
        go->UpdateModelPosition(x, y, z, false);
        RemoveGameObjectFromMoveList(go);
    }

    old_cell = go->GetCurrentCell();
    integrity_check = Cell(go->GetPositionX(), go->GetPositionY());
    ASSERT(integrity_check == old_cell);
}

void Map::AddCreatureToMoveList(Creature *c, float x, float y, float z, float ang)
{
    if (!c)
        return;

    i_creaturesToMove[c] = CreatureMover(x, y, z, ang);
}

void Map::AddGameObjectToMoveList(GameObject* go, float x, float y, float z, float ang)
{
    if (!go)
        return;

    if (go->_moveState == MAP_OBJECT_CELL_MOVE_NONE)
        i_gameObjectsToMove.push_back(go);
    go->SetNewCellPosition(x, y, z, ang);
}

void Map::RemoveGameObjectFromMoveList(GameObject* go)
{
    if (!go)
        return;

    if (go->_moveState == MAP_OBJECT_CELL_MOVE_ACTIVE)
        go->_moveState = MAP_OBJECT_CELL_MOVE_INACTIVE;
}

void Map::MoveAllCreaturesInMoveList()
{
    while (!i_creaturesToMove.empty())
    {
        // get data and remove element;
        CreatureMoveList::iterator iter = i_creaturesToMove.begin();
        Creature* c = iter->first;
        CreatureMover cm = iter->second;
        i_creaturesToMove.erase(iter);

        // calculate cells
        CellPair new_val = Trinity::ComputeCellPair(cm.x, cm.y);
        Cell new_cell(new_val);
        EnsureGridLoaded(new_cell);

        // some checks, i think it's not necessary, but who knows..
        NGridType *grid = getNGrid(new_cell.GridX(), new_cell.GridY());

        // do move or do move to respawn or remove creature if previous all fail
        if (grid && CreatureCellRelocation(c,new_cell))
        {
            // update pos
            c->Relocate(cm.x, cm.y, cm.z, cm.ang);
            //CreatureRelocationNotify(c,new_cell,new_cell.cellPair());
            c->UpdateObjectVisibility(false);
            c->SetRelocatedFlag();
        }
        else
        {
            // if creature can't be move in new cell/grid (not loaded) move it to repawn cell/grid
            // creature coordinates will be updated and notifiers send
            if (!CreatureRespawnRelocation(c))
            {
                // ... or unload (if respawn grid also not loaded)
                #ifdef TRINITY_DEBUG
                if ((sLog->GetLogFilter() & LOG_FILTER_CREATURE_MOVES) == 0)
                    sLog->outDebug("Creature (GUID: %u Entry: %u) cannot be move to unloaded respawn grid.",c->GetGUIDLow(),c->GetEntry());
                #endif
                AddObjectToRemoveList(c);
            }
        }
    }
}

void Map::MoveAllGameObjectsInMoveList()
{
    for (std::vector<GameObject*>::iterator itr = i_gameObjectsToMove.begin(); itr != i_gameObjectsToMove.end(); ++itr)
    {
        GameObject* go = *itr;
        if (go->FindMap() != this) //transport is teleported to another map
            continue;

        if (go->_moveState != MAP_OBJECT_CELL_MOVE_ACTIVE)
        {
            go->_moveState = MAP_OBJECT_CELL_MOVE_NONE;
            continue;
        }

        go->_moveState = MAP_OBJECT_CELL_MOVE_NONE;
        if (!go->IsInWorld())
            continue;

        // do move or do move to respawn or remove creature if previous all fail
        if (GameObjectCellRelocation(go, Cell(go->_newPosition.m_positionX, go->_newPosition.m_positionY)))
        {
            // update pos
            go->Relocate(go->_newPosition);
            go->UpdateObjectVisibility(false);
        }
        else
        {
            // if GameObject can't be move in new cell/grid (not loaded) move it to repawn cell/grid
            // GameObject coordinates will be updated and notifiers send
            if (!GameObjectRespawnRelocation(go, false))
            {
                // ... or unload (if respawn grid also not loaded)
                AddObjectToRemoveList(go);
            }
        }

        // update dynamic vmap tree
        go->UpdateModelPosition(go->_newPosition.m_positionX, go->_newPosition.m_positionY, go->_newPosition.m_positionZ, true);
    }
    i_gameObjectsToMove.clear();
}

bool Map::CreatureCellRelocation(Creature *c, Cell new_cell)
{
    NGridType* grid = getNGrid(new_cell.GridX(), new_cell.GridY());
    if (!grid)
        return false;

    Cell const& old_cell = c->GetCurrentCell();
    if (!old_cell.DiffGrid(new_cell))                       // in same grid
    {
        // if in same cell then none do
        if (old_cell.DiffCell(new_cell))
        {
            #ifdef TRINITY_DEBUG
            if ((sLog->GetLogFilter() & LOG_FILTER_CREATURE_MOVES) == 0)
                sLog->outDebug("Creature (GUID: %u Entry: %u) moved in grid[%u,%u] from cell[%u,%u] to cell[%u,%u].", c->GetGUIDLow(), c->GetEntry(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY(), new_cell.CellX(), new_cell.CellY());
            #endif

            RemoveFromGrid(c,getNGrid(old_cell.GridX(), old_cell.GridY()),old_cell);
            AddToGrid(c,getNGrid(new_cell.GridX(), new_cell.GridY()),new_cell);
        }
        else
        {
            #ifdef TRINITY_DEBUG
            if ((sLog->GetLogFilter() & LOG_FILTER_CREATURE_MOVES) == 0)
                sLog->outDebug("Creature (GUID: %u Entry: %u) moved in same grid[%u,%u]cell[%u,%u].", c->GetGUIDLow(), c->GetEntry(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY());
            #endif
        }

        return true;
    }

    // in diff. grids but active creature
    if (c->isActiveObject())
    {
        EnsureGridLoadedAtEnter(new_cell);

        #ifdef TRINITY_DEBUG
        if ((sLog->GetLogFilter() & LOG_FILTER_CREATURE_MOVES) == 0)
            sLog->outDebug("Active creature (GUID: %u Entry: %u) moved from grid[%u,%u]cell[%u,%u] to grid[%u,%u]cell[%u,%u].", c->GetGUIDLow(), c->GetEntry(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY(), new_cell.GridX(), new_cell.GridY(), new_cell.CellX(), new_cell.CellY());
        #endif

        RemoveFromGrid(c,getNGrid(old_cell.GridX(), old_cell.GridY()),old_cell);
        AddToGrid(c,getNGrid(new_cell.GridX(), new_cell.GridY()),new_cell);

        return true;
    }

    // in diff. loaded grid normal creature
    if (loaded(GridPair(new_cell.GridX(), new_cell.GridY())))
    {
        #ifdef TRINITY_DEBUG
        if ((sLog->GetLogFilter() & LOG_FILTER_CREATURE_MOVES) == 0)
            sLog->outDebug("Creature (GUID: %u Entry: %u) moved from grid[%u,%u]cell[%u,%u] to grid[%u,%u]cell[%u,%u].", c->GetGUIDLow(), c->GetEntry(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY(), new_cell.GridX(), new_cell.GridY(), new_cell.CellX(), new_cell.CellY());
        #endif

        RemoveFromGrid(c,getNGrid(old_cell.GridX(), old_cell.GridY()),old_cell);
        EnsureGridCreated(GridPair(new_cell.GridX(), new_cell.GridY()));
        AddToGrid(c,getNGrid(new_cell.GridX(), new_cell.GridY()),new_cell);

        return true;
    }

    // fail to move: normal creature attempt move to unloaded grid
    #ifdef TRINITY_DEBUG
    if ((sLog->GetLogFilter() & LOG_FILTER_CREATURE_MOVES) == 0)
        sLog->outDebug("Creature (GUID: %u Entry: %u) attempted to move from grid[%u,%u]cell[%u,%u] to unloaded grid[%u,%u]cell[%u,%u].", c->GetGUIDLow(), c->GetEntry(), old_cell.GridX(), old_cell.GridY(), old_cell.CellX(), old_cell.CellY(), new_cell.GridX(), new_cell.GridY(), new_cell.CellX(), new_cell.CellY());
    #endif
    return false;
}

bool Map::GameObjectCellRelocation(GameObject* go, Cell new_cell)
{
    Cell const& old_cell = go->GetCurrentCell();
    if (!old_cell.DiffGrid(new_cell))                       // in same grid
    {
        // if in same cell then none do
        if (old_cell.DiffCell(new_cell))
        {
            RemoveFromGrid(go,getNGrid(old_cell.GridX(), old_cell.GridY()), old_cell);
            AddToGrid(go,getNGrid(new_cell.GridX(), new_cell.GridY()),new_cell);
        }

        return true;
    }

    // in diff. grids but active GameObject
    if (go->isActiveObject())
    {
        EnsureGridLoadedAtEnter(new_cell);

        RemoveFromGrid(go, getNGrid(old_cell.GridX(), old_cell.GridY()), old_cell);
        AddToGrid(go, getNGrid(new_cell.GridX(), new_cell.GridY()), new_cell);

        return true;
    }

    // in diff. loaded grid normal GameObject
    if (loaded(GridPair(new_cell.GridX(), new_cell.GridY())))
    {
        RemoveFromGrid(go, getNGrid(old_cell.GridX(), old_cell.GridY()), old_cell);
        EnsureGridCreated(GridPair(new_cell.GridX(), new_cell.GridY()));
        AddToGrid(go, getNGrid(new_cell.GridX(), new_cell.GridY()), new_cell);

        return true;
    }

    // fail to move: normal GameObject attempt move to unloaded grid
    return false;
}

bool Map::CreatureRespawnRelocation(Creature *c)
{
    float resp_x, resp_y, resp_z, resp_o;
    c->GetRespawnCoord(resp_x, resp_y, resp_z, &resp_o);

    CellPair resp_val = Trinity::ComputeCellPair(resp_x, resp_y);
    Cell resp_cell(resp_val);

    c->CombatStop();
    c->GetMotionMaster()->Clear();

    #ifdef TRINITY_DEBUG
    if ((sLog->GetLogFilter() & LOG_FILTER_CREATURE_MOVES) == 0)
        sLog->outDebug("Creature (GUID: %u Entry: %u) moved from grid[%u,%u]cell[%u,%u] to respawn grid[%u,%u]cell[%u,%u].", c->GetGUIDLow(), c->GetEntry(), c->GetCurrentCell().GridX(), c->GetCurrentCell().GridY(), c->GetCurrentCell().CellX(), c->GetCurrentCell().CellY(), resp_cell.GridX(), resp_cell.GridY(), resp_cell.CellX(), resp_cell.CellY());
    #endif

    // teleport it to respawn point (like normal respawn if player see)
    if (CreatureCellRelocation(c,resp_cell))
    {
        c->Relocate(resp_x, resp_y, resp_z, resp_o);
        c->GetMotionMaster()->Initialize();                 // prevent possible problems with default move generators
        //CreatureRelocationNotify(c,resp_cell,resp_cell.cellPair());
        c->UpdateObjectVisibility(false);
        return true;
    }
    else
        return false;
}

bool Map::GameObjectRespawnRelocation(GameObject* go, bool diffGridOnly)
{
    float resp_x, resp_y, resp_z, resp_o;
    go->GetRespawnPosition(resp_x, resp_y, resp_z, &resp_o);
    Cell resp_cell(resp_x, resp_y);

    //GameObject will be unloaded with grid
    if (diffGridOnly && !go->GetCurrentCell().DiffGrid(resp_cell))
        return true;

    // teleport it to respawn point (like normal respawn if player see)
    if (GameObjectCellRelocation(go, resp_cell))
    {
        go->Relocate(resp_x, resp_y, resp_z, resp_o);
        go->UpdateObjectVisibility(false);
        return true;
    }

    return false;
}

bool Map::UnloadGrid(const uint32 &x, const uint32 &y, bool unloadAll)
{
    NGridType *grid = getNGrid(x, y);
    ASSERT(grid != NULL);

    {
        if (!unloadAll)
        {
            //pets, possessed creatures (must be active), transport passengers
            if (grid->GetWorldObjectCountInNGrid<Creature>())
                return false;

            if (ActiveObjectsNearGrid(x, y))
                return false;
        }

        sLog->outDebug("Unloading grid[%u,%u] for map %u", x,y, GetId());

        ObjectGridUnloader unloader(*grid);

        if (!unloadAll)
        {
            // Finish creature moves, remove and delete all creatures with delayed remove before moving to respawn grids
            // Must know real mob position before move
            MoveAllCreaturesInMoveList();
            MoveAllGameObjectsInMoveList();

            // move creatures to respawn grids if this is diff.grid or to remove list
            unloader.MoveToRespawnN();

            // Finish creature moves, remove and delete all creatures with delayed remove before unload
            MoveAllCreaturesInMoveList();
            MoveAllGameObjectsInMoveList();
        }

        ObjectGridCleaner cleaner(*grid);
        cleaner.CleanN();

        RemoveAllObjectsInRemoveList();

        unloader.UnloadN();

        ASSERT(i_objectsToRemove.empty());

        delete grid;
        setNGrid(NULL, x, y);
    }
    int gx = (MAX_NUMBER_OF_GRIDS - 1) - x;
    int gy = (MAX_NUMBER_OF_GRIDS - 1) - y;

    // delete grid map, but don't delete if it is from parent map (and thus only reference)
    //+++if (GridMaps[gx][gy]) don't check for GridMaps[gx][gy], we might have to unload vmaps
    {
        if (i_InstanceId == 0)
        {
            if (GridMaps[gx][gy])
            {
                GridMaps[gx][gy]->unloadData();
                delete GridMaps[gx][gy];
            }
            VMAP::VMapFactory::createOrGetVMapManager()->unloadMap(GetId(), gx, gy);
            MMAP::MMapFactory::createOrGetMMapManager()->unloadMap(GetId(), gx, gy);
        }
        else
            ((MapInstanced*)m_parentMap)->RemoveGridMapReference(GridPair(gx, gy));

        GridMaps[gx][gy] = NULL;
    }
    sLog->outStaticDebug("Unloading grid[%u,%u] for map %u finished", x,y, GetId());
    return true;
}

void Map::RemoveAllPlayers()
{
    if (HavePlayers())
    {
        for (MapRefManager::iterator itr = m_mapRefManager.begin(); itr != m_mapRefManager.end(); ++itr)
        {
            Player* plr = itr->getSource();
            if (!plr->IsBeingTeleportedFar())
            {
                // this is happening for bg
                sLog->outError("Map::UnloadAll: player %s is still in map %u during unload, this should not happen!", plr->GetName(), GetId());
                plr->TeleportTo(plr->m_homebindMapId, plr->m_homebindX, plr->m_homebindY, plr->m_homebindZ, plr->GetOrientation());
            }
        }
    }
}

void Map::UnloadAll()
{
    // clear all delayed moves, useless anyway do this moves before map unload.
    i_creaturesToMove.clear();
    i_gameObjectsToMove.clear();

    for (GridRefManager<NGridType>::iterator i = GridRefManager<NGridType>::begin(); i != GridRefManager<NGridType>::end();)
    {
        NGridType &grid(*i->getSource());
        ++i;
        UnloadGrid(grid.getX(), grid.getY(), true);       // deletes the grid and removes it from the GridRefManager
    }
}

//*****************************
// Grid function
//*****************************
GridMap::GridMap()
{
    m_flags = 0;
    // Area data
    m_gridArea = 0;
    m_area_map = NULL;
    // Height level data
    m_gridHeight = INVALID_HEIGHT;
    m_gridGetHeight = &GridMap::getHeightFromFlat;
    m_V9 = NULL;
    m_V8 = NULL;
    // Liquid data
    m_liquidType    = 0;
    m_liquidOffX   = 0;
    m_liquidOffY   = 0;
    m_liquidWidth  = 0;
    m_liquidHeight = 0;
    m_liquidLevel = INVALID_HEIGHT;
    m_liquidEntry = NULL;
    m_liquidFlags = NULL;
    m_liquidMap  = NULL;
}

GridMap::~GridMap()
{
    unloadData();
}

bool GridMap::loadData(char* filename)
{
    // Unload old data if exist
    unloadData();

    map_fileheader header;
    // Not return error if file not found
    FILE *in = fopen(filename, "rb");
    if (!in)
        return true;

    if (fread(&header, sizeof(header),1,in) != 1)
    {
        fclose(in);
        return false;
    }

    if (header.mapMagic == uint32(MAP_MAGIC) && (header.versionMagic == uint32(MAP_VERSION_MAGIC) || header.versionMagic == uint32(MAP_VERSION_MAGIC_ALT)))
    {
        // loadup area data
        if (header.areaMapOffset && !loadAreaData(in, header.areaMapOffset, header.areaMapSize))
        {
            sLog->outError("Error loading map area data\n");
            fclose(in);
            return false;
        }
        // loadup height data
        if (header.heightMapOffset && !loadHeihgtData(in, header.heightMapOffset, header.heightMapSize))
        {
            sLog->outError("Error loading map height data\n");
            fclose(in);
            return false;
        }
        // loadup liquid data
        if (header.liquidMapOffset && !loadLiquidData(in, header.liquidMapOffset, header.liquidMapSize))
        {
            sLog->outError("Error loading map liquids data\n");
            fclose(in);
            return false;
        }
        fclose(in);
        return true;
    }
    sLog->outError("Map file '%s' is from an incompatible clientversion. Please recreate using the mapextractor.", filename);
    fclose(in);
    return false;
}

void GridMap::unloadData()
{
    delete[] m_area_map;
    delete[] m_V9;
    delete[] m_V8;
    delete[] m_liquidEntry;
    delete[] m_liquidFlags;
    delete[] m_liquidMap;
    m_area_map = NULL;
    m_V9 = NULL;
    m_V8 = NULL;
    m_liquidEntry = NULL;
    m_liquidFlags = NULL;
    m_liquidMap  = NULL;
    m_gridGetHeight = &GridMap::getHeightFromFlat;
}

bool GridMap::loadAreaData(FILE *in, uint32 offset, uint32 /*size*/)
{
    map_areaHeader header;
    fseek(in, offset, SEEK_SET);

    if (fread(&header, sizeof(header), 1, in) != 1 || header.fourcc != uint32(MAP_AREA_MAGIC))
        return false;

    m_gridArea = header.gridArea;
    if (!(header.flags & MAP_AREA_NO_AREA))
    {
        m_area_map = new uint16 [16*16];
        dontDump(m_area_map, sizeof(uint16)*16*16);
        if (fread(m_area_map, sizeof(uint16), 16*16, in) != 16*16)
            return false;
    }
    return true;
}

bool GridMap::loadHeihgtData(FILE *in, uint32 offset, uint32 /*size*/)
{
    map_heightHeader header;
    fseek(in, offset, SEEK_SET);

    if (fread(&header, sizeof(header), 1, in) != 1 || header.fourcc != uint32(MAP_HEIGHT_MAGIC))
        return false;

    m_gridHeight = header.gridHeight;
    if (!(header.flags & MAP_HEIGHT_NO_HEIGHT))
    {
        if ((header.flags & MAP_HEIGHT_AS_INT16))
        {
            m_uint16_V9 = new uint16 [129*129];
            m_uint16_V8 = new uint16 [128*128];
            dontDump(m_uint16_V9, sizeof(uint16) * 129 * 129);
            dontDump(m_uint16_V8, sizeof(uint16) * 128 * 128);
            if (fread(m_uint16_V9, sizeof(uint16), 129*129, in) != 129*129 ||
                fread(m_uint16_V8, sizeof(uint16), 128*128, in) != 128*128)
                return false;
            m_gridIntHeightMultiplier = (header.gridMaxHeight - header.gridHeight) / 65535;
            m_gridGetHeight = &GridMap::getHeightFromUint16;
        }
        else if ((header.flags & MAP_HEIGHT_AS_INT8))
        {
            m_uint8_V9 = new uint8 [129*129];
            m_uint8_V8 = new uint8 [128*128];
            dontDump(m_uint8_V9, sizeof(uint8) * 129 * 129);
            dontDump(m_uint8_V8, sizeof(uint8) * 128 * 128);
            if (fread(m_uint8_V9, sizeof(uint8), 129*129, in) != 129*129 ||
                fread(m_uint8_V8, sizeof(uint8), 128*128, in) != 128*128)
                return false;
            m_gridIntHeightMultiplier = (header.gridMaxHeight - header.gridHeight) / 255;
            m_gridGetHeight = &GridMap::getHeightFromUint8;
        }
        else
        {
            m_V9 = new float [129*129];
            m_V8 = new float [128*128];
            dontDump(m_V9, sizeof(float) * 129 * 129);
            dontDump(m_V8, sizeof(float) * 128 * 128);
            if (fread(m_V9, sizeof(float), 129*129, in) != 129*129 ||
                fread(m_V8, sizeof(float), 128*128, in) != 128*128)
                return false;
            m_gridGetHeight = &GridMap::getHeightFromFloat;
        }
    }
    else
        m_gridGetHeight = &GridMap::getHeightFromFlat;

    return true;
}

bool  GridMap::loadLiquidData(FILE *in, uint32 offset, uint32 /*size*/)
{
    map_liquidHeader header;
    fseek(in, offset, SEEK_SET);

    if (fread(&header, sizeof(header), 1, in) != 1 || header.fourcc != uint32(MAP_LIQUID_MAGIC))
        return false;

    m_liquidType  = header.liquidType;
    m_liquidOffX  = header.offsetX;
    m_liquidOffY  = header.offsetY;
    m_liquidWidth = header.width;
    m_liquidHeight= header.height;
    m_liquidLevel  = header.liquidLevel;

    if (!(header.flags & MAP_LIQUID_NO_TYPE))
    {
        m_liquidEntry = new uint16 [16*16];
        dontDump(m_liquidEntry, sizeof(uint16) * 16 * 16);
        if (fread(m_liquidEntry, sizeof(uint16), 16*16, in) != 16*16)
            return false;

        m_liquidFlags = new uint8[16*16];
        dontDump(m_liquidFlags, sizeof(uint8) * 16 * 16);
        if (fread(m_liquidFlags, sizeof(uint8), 16*16, in) != 16*16)
            return false;
    }
    if (!(header.flags & MAP_LIQUID_NO_HEIGHT))
    {
        m_liquidMap = new float [m_liquidWidth*m_liquidHeight];
        dontDump(m_liquidMap, sizeof(float) * m_liquidWidth * m_liquidHeight);
        if (fread(m_liquidMap, sizeof(float), m_liquidWidth*m_liquidHeight, in) != m_liquidWidth*m_liquidHeight)
            return false;
    }
    return true;
}

uint16 GridMap::getArea(float x, float y)
{
    if (!m_area_map)
        return m_gridArea;

    x = 16 * (CENTER_GRID_ID - x/SIZE_OF_GRIDS);
    y = 16 * (CENTER_GRID_ID - y/SIZE_OF_GRIDS);
    int lx = (int)x & 15;
    int ly = (int)y & 15;
    return m_area_map[lx*16 + ly];
}

float  GridMap::getHeightFromFlat(float /*x*/, float /*y*/) const
{
    return m_gridHeight;
}

float  GridMap::getHeightFromFloat(float x, float y) const
{
    if (!m_V8 || !m_V9)
        return m_gridHeight;

    x = MAP_RESOLUTION * (CENTER_GRID_ID - x/SIZE_OF_GRIDS);
    y = MAP_RESOLUTION * (CENTER_GRID_ID - y/SIZE_OF_GRIDS);

    int x_int = (int)x;
    int y_int = (int)y;
    x -= x_int;
    y -= y_int;
    x_int&=(MAP_RESOLUTION - 1);
    y_int&=(MAP_RESOLUTION - 1);

    // Height stored as: h5 - its v8 grid, h1-h4 - its v9 grid
    // +--------------> X
    // | h1-------h2     Coordinates is:
    // | | \  1  / |     h1 0,0
    // | |  \   /  |     h2 0,1
    // | | 2  h5 3 |     h3 1,0
    // | |  /   \  |     h4 1,1
    // | | /  4  \ |     h5 1/2,1/2
    // | h3-------h4
    // V Y
    // For find height need
    // 1 - detect triangle
    // 2 - solve linear equation from triangle points
    // Calculate coefficients for solve h = a*x + b*y + c

    float a,b,c;
    // Select triangle:
    if (x+y < 1)
    {
        if (x > y)
        {
            // 1 triangle (h1, h2, h5 points)
            float h1 = m_V9[(x_int)*129 + y_int];
            float h2 = m_V9[(x_int+1)*129 + y_int];
            float h5 = 2 * m_V8[x_int*128 + y_int];
            a = h2-h1;
            b = h5-h1-h2;
            c = h1;
        }
        else
        {
            // 2 triangle (h1, h3, h5 points)
            float h1 = m_V9[x_int*129 + y_int  ];
            float h3 = m_V9[x_int*129 + y_int+1];
            float h5 = 2 * m_V8[x_int*128 + y_int];
            a = h5 - h1 - h3;
            b = h3 - h1;
            c = h1;
        }
    }
    else
    {
        if (x > y)
        {
            // 3 triangle (h2, h4, h5 points)
            float h2 = m_V9[(x_int+1)*129 + y_int  ];
            float h4 = m_V9[(x_int+1)*129 + y_int+1];
            float h5 = 2 * m_V8[x_int*128 + y_int];
            a = h2 + h4 - h5;
            b = h4 - h2;
            c = h5 - h4;
        }
        else
        {
            // 4 triangle (h3, h4, h5 points)
            float h3 = m_V9[(x_int)*129 + y_int+1];
            float h4 = m_V9[(x_int+1)*129 + y_int+1];
            float h5 = 2 * m_V8[x_int*128 + y_int];
            a = h4 - h3;
            b = h3 + h4 - h5;
            c = h5 - h4;
        }
    }
    // Calculate height
    return a * x + b * y + c;
}

float  GridMap::getHeightFromUint8(float x, float y) const
{
    if (!m_uint8_V8 || !m_uint8_V9)
        return m_gridHeight;

    x = MAP_RESOLUTION * (CENTER_GRID_ID - x/SIZE_OF_GRIDS);
    y = MAP_RESOLUTION * (CENTER_GRID_ID - y/SIZE_OF_GRIDS);

    int x_int = (int)x;
    int y_int = (int)y;
    x -= x_int;
    y -= y_int;
    x_int&=(MAP_RESOLUTION - 1);
    y_int&=(MAP_RESOLUTION - 1);

    int32 a, b, c;
    uint8 *V9_h1_ptr = &m_uint8_V9[x_int*128 + x_int + y_int];
    if (x+y < 1)
    {
        if (x > y)
        {
            // 1 triangle (h1, h2, h5 points)
            int32 h1 = V9_h1_ptr[  0];
            int32 h2 = V9_h1_ptr[129];
            int32 h5 = 2 * m_uint8_V8[x_int*128 + y_int];
            a = h2-h1;
            b = h5-h1-h2;
            c = h1;
        }
        else
        {
            // 2 triangle (h1, h3, h5 points)
            int32 h1 = V9_h1_ptr[0];
            int32 h3 = V9_h1_ptr[1];
            int32 h5 = 2 * m_uint8_V8[x_int*128 + y_int];
            a = h5 - h1 - h3;
            b = h3 - h1;
            c = h1;
        }
    }
    else
    {
        if (x > y)
        {
            // 3 triangle (h2, h4, h5 points)
            int32 h2 = V9_h1_ptr[129];
            int32 h4 = V9_h1_ptr[130];
            int32 h5 = 2 * m_uint8_V8[x_int*128 + y_int];
            a = h2 + h4 - h5;
            b = h4 - h2;
            c = h5 - h4;
        }
        else
        {
            // 4 triangle (h3, h4, h5 points)
            int32 h3 = V9_h1_ptr[  1];
            int32 h4 = V9_h1_ptr[130];
            int32 h5 = 2 * m_uint8_V8[x_int*128 + y_int];
            a = h4 - h3;
            b = h3 + h4 - h5;
            c = h5 - h4;
        }
    }
    // Calculate height
    return (float)((a * x) + (b * y) + c)*m_gridIntHeightMultiplier + m_gridHeight;
}

float  GridMap::getHeightFromUint16(float x, float y) const
{
    if (!m_uint16_V8 || !m_uint16_V9)
        return m_gridHeight;

    x = MAP_RESOLUTION * (CENTER_GRID_ID - x/SIZE_OF_GRIDS);
    y = MAP_RESOLUTION * (CENTER_GRID_ID - y/SIZE_OF_GRIDS);

    int x_int = (int)x;
    int y_int = (int)y;
    x -= x_int;
    y -= y_int;
    x_int&=(MAP_RESOLUTION - 1);
    y_int&=(MAP_RESOLUTION - 1);

    int32 a, b, c;
    uint16 *V9_h1_ptr = &m_uint16_V9[x_int*128 + x_int + y_int];
    if (x+y < 1)
    {
        if (x > y)
        {
            // 1 triangle (h1, h2, h5 points)
            int32 h1 = V9_h1_ptr[  0];
            int32 h2 = V9_h1_ptr[129];
            int32 h5 = 2 * m_uint16_V8[x_int*128 + y_int];
            a = h2-h1;
            b = h5-h1-h2;
            c = h1;
        }
        else
        {
            // 2 triangle (h1, h3, h5 points)
            int32 h1 = V9_h1_ptr[0];
            int32 h3 = V9_h1_ptr[1];
            int32 h5 = 2 * m_uint16_V8[x_int*128 + y_int];
            a = h5 - h1 - h3;
            b = h3 - h1;
            c = h1;
        }
    }
    else
    {
        if (x > y)
        {
            // 3 triangle (h2, h4, h5 points)
            int32 h2 = V9_h1_ptr[129];
            int32 h4 = V9_h1_ptr[130];
            int32 h5 = 2 * m_uint16_V8[x_int*128 + y_int];
            a = h2 + h4 - h5;
            b = h4 - h2;
            c = h5 - h4;
        }
        else
        {
            // 4 triangle (h3, h4, h5 points)
            int32 h3 = V9_h1_ptr[  1];
            int32 h4 = V9_h1_ptr[130];
            int32 h5 = 2 * m_uint16_V8[x_int*128 + y_int];
            a = h4 - h3;
            b = h3 + h4 - h5;
            c = h5 - h4;
        }
    }
    // Calculate height
    return (float)((a * x) + (b * y) + c)*m_gridIntHeightMultiplier + m_gridHeight;
}

float  GridMap::getLiquidLevel(float x, float y)
{
    if (!m_liquidMap)
        return m_liquidLevel;

    x = MAP_RESOLUTION * (CENTER_GRID_ID - x/SIZE_OF_GRIDS);
    y = MAP_RESOLUTION * (CENTER_GRID_ID - y/SIZE_OF_GRIDS);

    int cx_int = ((int)x & (MAP_RESOLUTION-1)) - m_liquidOffY;
    int cy_int = ((int)y & (MAP_RESOLUTION-1)) - m_liquidOffX;

    if (cx_int < 0 || cx_int >=m_liquidHeight)
        return INVALID_HEIGHT;
    if (cy_int < 0 || cy_int >=m_liquidWidth)
        return INVALID_HEIGHT;

    return m_liquidMap[cx_int*m_liquidWidth + cy_int];
}

uint8  GridMap::getTerrainType(float x, float y)
{
    if (!m_liquidFlags)
        return 0;

    x = 16 * (CENTER_GRID_ID - x/SIZE_OF_GRIDS);
    y = 16 * (CENTER_GRID_ID - y/SIZE_OF_GRIDS);
    int lx = (int)x & 15;
    int ly = (int)y & 15;
    return m_liquidFlags[lx*16 + ly];
}

// Get water state on map
inline ZLiquidStatus GridMap::getLiquidStatus(float x, float y, float z, uint8 ReqLiquidType, LiquidData *data)
{
    // Check water type (if no water return)
    if (!m_liquidType && !m_liquidFlags)
        return LIQUID_MAP_NO_WATER;

    // Get cell
    float cx = MAP_RESOLUTION * (CENTER_GRID_ID - x/SIZE_OF_GRIDS);
    float cy = MAP_RESOLUTION * (CENTER_GRID_ID - y/SIZE_OF_GRIDS);

    int x_int = (int)cx & (MAP_RESOLUTION-1);
    int y_int = (int)cy & (MAP_RESOLUTION-1);

    // Check water type in cell
    int idx=(x_int>>3)*16 + (y_int>>3);
    uint8 type = m_liquidFlags ? m_liquidFlags[idx] : m_liquidType;
    uint32 entry = 0;
    if (m_liquidEntry)
    {
        if (LiquidTypeEntry const* liquidEntry = sLiquidTypeStore.LookupEntry(m_liquidEntry[idx]))
        {
            entry = liquidEntry->Id;
            type &= MAP_LIQUID_TYPE_DARK_WATER;
            uint32 liqTypeIdx = liquidEntry->Type;
            if (entry < 21)
            {
                if (AreaTableEntry const* area = GetAreaEntryByAreaFlagAndMap(getArea(x, y), MAPID_INVALID))
                {
                    uint32 overrideLiquid = area->LiquidTypeOverride[liquidEntry->Type];
                    if (!overrideLiquid && area->zone)
                    {
                        area = GetAreaEntryByAreaID(area->zone);
                        if (area)
                            overrideLiquid = area->LiquidTypeOverride[liquidEntry->Type];
                    }

                    if (LiquidTypeEntry const* liq = sLiquidTypeStore.LookupEntry(overrideLiquid))
                    {
                        entry = overrideLiquid;
                        liqTypeIdx = liq->Type;
                    }
                }
            }

            type |= 1 << liqTypeIdx;
        }
    }

    if (type == 0)
        return LIQUID_MAP_NO_WATER;

    // Check req liquid type mask
    if (ReqLiquidType && !(ReqLiquidType&type))
        return LIQUID_MAP_NO_WATER;

    // Check water level:
    // Check water height map
    int lx_int = x_int - m_liquidOffY;
    int ly_int = y_int - m_liquidOffX;
    if (lx_int < 0 || lx_int >= m_liquidHeight)
        return LIQUID_MAP_NO_WATER;
    if (ly_int < 0 || ly_int >= m_liquidWidth)
        return LIQUID_MAP_NO_WATER;

    // Get water level
    float liquid_level = m_liquidMap ? m_liquidMap[lx_int*m_liquidWidth + ly_int] : m_liquidLevel;
    // Get ground level (sub 0.2 for fix some errors)
    float ground_level = getHeight(x, y);

    // Check water level and ground level
    if (liquid_level < ground_level || z < ground_level - 2)
        return LIQUID_MAP_NO_WATER;

    // All ok in water -> store data
    if (data)
    {
        data->entry = entry;
        data->type_flags  = type;
        data->level = liquid_level;
        data->depth_level = ground_level;
    }

    // For speed check as int values
    float delta = liquid_level - z;

    if (delta > 2.0f)                   // Under water
        return LIQUID_MAP_UNDER_WATER;
    if (delta > 0.0f)                   // In water
        return LIQUID_MAP_IN_WATER;
    if (delta > -0.1f)                   // Walk on water
        return LIQUID_MAP_WATER_WALK;
                                      // Above water
    return LIQUID_MAP_ABOVE_WATER;
}

inline GridMap *Map::GetGrid(float x, float y)
{
    // half opt method
    int gx=(int)(CENTER_GRID_ID-x/SIZE_OF_GRIDS);                       //grid x
    int gy=(int)(CENTER_GRID_ID-y/SIZE_OF_GRIDS);                       //grid y

    // do not enter to invalid grid
    if (gx >= MAX_NUMBER_OF_GRIDS || gy >= MAX_NUMBER_OF_GRIDS || gx < 0 || gy < 0)
        return NULL;

    // ensure GridMap is loaded
    EnsureGridCreated(GridPair(63-gx,63-gy));

    return GridMaps[gx][gy];
}

float Map::GetWaterOrGroundLevel(float x, float y, float z, float* ground /*= NULL*/, bool swim /*= false*/) const
{
    if (const_cast<Map*>(this)->GetGrid(x, y))
    {
        // we need ground level (including grid height version) for proper return water level in point
        float ground_z = GetHeight(PHASEMASK_NORMAL, x, y, z+2.0f, true, 50.0f);
        if (ground)
            *ground = ground_z;

        LiquidData liquid_status;

        ZLiquidStatus res = getLiquidStatus(x, y, ground_z, MAP_ALL_LIQUIDS, &liquid_status);
        return res ? ( swim ? liquid_status.level - 2.0f : liquid_status.level) : ground_z;
    }

    return VMAP_INVALID_HEIGHT_VALUE;
}

GameObject* Map::GetGroundCollisionObject(float x, float y, float z, uint32 phaseMask)
{
    GameObjectModel* colmodel = m_dyn_tree.getFirstCollisionModel(x, y, z, DEFAULT_HEIGHT_SEARCH*3.0f, phaseMask);
    if (!colmodel)
        return NULL;

    return HashMapHolder<GameObject>::Find(colmodel->getOwnerGUID());
}

bool Map::isInLineOfSight(float x1, float y1, float z1, float x2, float y2, float z2, uint32 phasemask) const
{
    return VMAP::VMapFactory::createOrGetVMapManager()->isInLineOfSight(GetId(), x1, y1, z1, x2, y2, z2)
        && m_dyn_tree.isInLineOfSight(x1, y1, z1, x2, y2, z2, phasemask);
}

float Map::GetHeight(uint32 phasemask, float x, float y, float z, bool vmap/*=true*/, float maxSearchDist/*=DEFAULT_HEIGHT_SEARCH*/) const
{
    return std::max<float>(GetHeight(x, y, z, vmap, maxSearchDist), m_dyn_tree.getHeight(x, y, z, maxSearchDist, phasemask));
}

float Map::GetHeight(float x, float y, float z, bool pUseVmaps, float maxSearchDist) const
{
    // find raw .map surface under Z coordinates
    float mapHeight = VMAP_INVALID_HEIGHT_VALUE;
    if (GridMap *gmap = const_cast<Map*>(this)->GetGrid(x, y))
    {
        float _mapheight = gmap->getHeight(x,y);

        // look from a bit higher pos to find the floor, ignore under surface case
        if (z + 2.0f > _mapheight)
            mapHeight = _mapheight;
    }

    float vmapHeight = VMAP_INVALID_HEIGHT_VALUE;
    if (pUseVmaps)
    {
        VMAP::IVMapManager* vmgr = VMAP::VMapFactory::createOrGetVMapManager();
        if (vmgr->isHeightCalcEnabled())
        {
            // look from a bit higher pos to find the floor
            vmapHeight = vmgr->getHeight(GetId(), x, y, z + 2.0f, maxSearchDist);
        }
    }

    // mapHeight set for any above raw ground Z or <= INVALID_HEIGHT
    // vmapheight set for any under Z value or <= INVALID_HEIGHT

    if (vmapHeight > INVALID_HEIGHT)
    {
        if (mapHeight > INVALID_HEIGHT)
        {
            // we have mapheight and vmapheight and must select more appropriate

            // we are already under the surface or vmap height above map heigt
            // or if the distance of the vmap height is less the land height distance
            if (z < mapHeight || vmapHeight > mapHeight || fabs(mapHeight-z) > fabs(vmapHeight-z))
                return vmapHeight;
            else
                return mapHeight;                           // better use .map surface height

        }
        else
            return vmapHeight;                              // we have only vmapHeight (if have)
    }

    return mapHeight;                               // explicitly use map data (if have)
}

/* advanced, meaningful and not-bugged version of Map::GetHeight */
float Map::GetHeight2(float x, float y, float z) const
{
    /* input z is important for caves (or under-bridges, ..), because
     * one X,Y can have MULTIPLE Z points !!! (ie. under/on the bridge)
     * - make an algorithm that looks both up and down to find a nearest
     *   surface and return it's height */

    float height;

    /* use GetHeight when available to overcome VMap imperfections
       (GetHeight has algorithms to "detect" those) */
    height = GetHeight(x, y, z, true);
    if (height > INVALID_HEIGHT)
       return height;

    /* increase input Z a little, just enough for all the VMAP
     * inaccurate cases (tiny bumps) that would make the calculation wrong
     * -- this is because of the vmgr->getHeight algorithm that checks
     *    beneath current position and if there's a valid height point,
     *    it would use that one instead of the one we want (which is
     *    ie. 0.013yd above the current position */
    /* NOTE: this is ONLY for the calculation, returned height will be exact */
    z += 2.0f;

    /* use vmap object if it's available for this x,y */
    VMAP::IVMapManager* vmgr = VMAP::VMapFactory::createOrGetVMapManager();
    if (!vmgr)
        height = VMAP_INVALID_HEIGHT_VALUE;
    else
        height = vmgr->getHeight(GetId(), x, y, z, 5000.0f);

    /* use grid height if vmap object doesn't exist */
    if (height == VMAP_INVALID_HEIGHT_VALUE)
        if (GridMap *gmap = const_cast<Map*>(this)->GetGrid(x, y))
            height = gmap->getHeight(x,y);

    if (height == -500.000000f)
        height = INVALID_HEIGHT;

    return height;
}

inline bool IsOutdoorWMO(uint32 mogpFlags, int32 /*adtId*/, int32 /*rootId*/, int32 /*groupId*/, WMOAreaTableEntry const* wmoEntry, AreaTableEntry const* atEntry)
{
    bool outdoor = true;

    if(wmoEntry && atEntry)
    {
        if(atEntry->flags & AREA_FLAG_OUTSIDE)
            return true;
        if(atEntry->flags & AREA_FLAG_INSIDE)
            return false;
    }

    outdoor = mogpFlags&0x8;

    if(wmoEntry)
    {
        if(wmoEntry->Flags & 4)
            return true;
        if((wmoEntry->Flags & 2)!=0)
            outdoor = false;
    }
    return outdoor;
}

bool Map::IsOutdoors(float x, float y, float z) const
{
    uint32 mogpFlags;
    int32 adtId, rootId, groupId;

    // no wmo found? -> outside by default
    if(!GetAreaInfo(x, y, z, mogpFlags, adtId, rootId, groupId))
        return true;

    AreaTableEntry const* atEntry = 0;
    WMOAreaTableEntry const* wmoEntry= GetWMOAreaTableEntryByTripple(rootId, adtId, groupId);
    if(wmoEntry)
    {
        sLog->outStaticDebug("Got WMOAreaTableEntry! flag %u, areaid %u", wmoEntry->Flags, wmoEntry->areaId);
        atEntry = GetAreaEntryByAreaID(wmoEntry->areaId);
    }
    return IsOutdoorWMO(mogpFlags, adtId, rootId, groupId, wmoEntry, atEntry);
}

bool Map::GetAreaInfo(float x, float y, float z, uint32 &flags, int32 &adtId, int32 &rootId, int32 &groupId) const
{
    float vmap_z = z;
    VMAP::IVMapManager* vmgr = VMAP::VMapFactory::createOrGetVMapManager();
    if (vmgr->getAreaInfo(GetId(), x, y, vmap_z, flags, adtId, rootId, groupId))
    {
        // check if there's terrain between player height and object height
        if(GridMap *gmap = const_cast<Map*>(this)->GetGrid(x, y))
        {
            float _mapheight = gmap->getHeight(x,y);
            // z + 2.0f condition taken from GetHeight(), not sure if it's such a great choice...
            if(z + 2.0f > _mapheight &&  _mapheight > vmap_z)
                return false;
        }
        return true;
    }
    return false;
}

uint16 Map::GetAreaFlag(float x, float y, float z, bool *isOutdoors) const
{
    uint32 mogpFlags;
    int32 adtId, rootId, groupId;
    WMOAreaTableEntry const* wmoEntry = 0;
    AreaTableEntry const* atEntry = 0;
    bool haveAreaInfo = false;

    if (GetAreaInfo(x, y, z, mogpFlags, adtId, rootId, groupId))
    {
        haveAreaInfo = true;
        wmoEntry = GetWMOAreaTableEntryByTripple(rootId, adtId, groupId);
        if (wmoEntry)
            atEntry = GetAreaEntryByAreaID(wmoEntry->areaId);
    }

    uint16 areaflag;

    if (atEntry)
        areaflag = atEntry->exploreFlag;
    else
    {
        if (GridMap *gmap = const_cast<Map*>(this)->GetGrid(x, y))
            areaflag = gmap->getArea(x, y);
        // this used while not all *.map files generated (instances)
        else
            areaflag = GetAreaFlagByMapId(i_mapEntry->MapID);
    }

    if (isOutdoors)
    {
        if (haveAreaInfo)
            *isOutdoors = IsOutdoorWMO(mogpFlags, adtId, rootId, groupId, wmoEntry, atEntry);
        else
            *isOutdoors = true;
    }
    return areaflag;
 }

uint8 Map::GetTerrainType(float x, float y) const
{
    if (GridMap *gmap = const_cast<Map*>(this)->GetGrid(x, y))
        return gmap->getTerrainType(x, y);
    else
        return 0;
}

ZLiquidStatus Map::getLiquidStatus(float x, float y, float z, uint8 ReqLiquidType, LiquidData *data) const
{
    ZLiquidStatus result = LIQUID_MAP_NO_WATER;
    VMAP::IVMapManager* vmgr = VMAP::VMapFactory::createOrGetVMapManager();
    float liquid_level = INVALID_HEIGHT;
    float ground_level = INVALID_HEIGHT;
    uint32 liquid_type;
    if (vmgr->GetLiquidLevel(GetId(), x, y, z, ReqLiquidType, liquid_level, ground_level, liquid_type))
    {
        sLog->outDebug("getLiquidStatus(): vmap liquid level: %f ground: %f type: %u", liquid_level, ground_level, liquid_type);
        // Check water level and ground level
        if (liquid_level > ground_level && z > ground_level - 2)
        {
            // All ok in water -> store data
            if (data)
            {
                                // hardcoded in client like this
                if (GetId() == 530 && liquid_type == 2)
                    liquid_type = 15;

                uint32 liquidFlagType = 0;
                if (LiquidTypeEntry const* liq = sLiquidTypeStore.LookupEntry(liquid_type))
                    liquidFlagType = liq->Type;

                if (liquid_type && liquid_type < 21)
                {
                    if (AreaTableEntry const* area = GetAreaEntryByAreaFlagAndMap(GetAreaFlag(x, y, z), GetId()))
                    {
                        uint32 overrideLiquid = area->LiquidTypeOverride[liquidFlagType];
                        if (!overrideLiquid && area->zone)
                            if ((area = GetAreaEntryByAreaID(area->zone)) != NULL)
                                overrideLiquid = area->LiquidTypeOverride[liquidFlagType];

                        if (LiquidTypeEntry const* liq = sLiquidTypeStore.LookupEntry(overrideLiquid))
                        {
                            liquid_type = overrideLiquid;
                            liquidFlagType = liq->Type;
                        }
                    }
                }

                data->level = liquid_level;
                data->depth_level = ground_level;

                data->entry = liquid_type;
                data->type_flags = 1 << liquidFlagType;
            }

            // For speed check as int values
            float delta = liquid_level - z;

            // Get position delta
            if (delta > 2.0f)                   // Under water
                return LIQUID_MAP_UNDER_WATER;
            if (delta > 0.0f )                   // In water
                return LIQUID_MAP_IN_WATER;
            if (delta > -0.1f)                   // Walk on water
                return LIQUID_MAP_WATER_WALK;
            result = LIQUID_MAP_ABOVE_WATER;
        }
    }

    if(GridMap* gmap = const_cast<Map*>(this)->GetGrid(x, y))
    {
        LiquidData map_data;
        ZLiquidStatus map_result = gmap->getLiquidStatus(x, y, z, ReqLiquidType, &map_data);
        // Not override LIQUID_MAP_ABOVE_WATER with LIQUID_MAP_NO_WATER:
        if (map_result != LIQUID_MAP_NO_WATER && (map_data.level > ground_level))
        {
            if (data)
            {
                // hardcoded in client like this
                if (GetId() == 530 && map_data.entry == 2)
                    map_data.entry = 15;

                *data = map_data;
            }

            return map_result;
        }
    }
    return result;
}

float Map::GetWaterLevel(float x, float y) const
{
    if (GridMap* gmap = const_cast<Map*>(this)->GetGrid(x, y))
        return gmap->getLiquidLevel(x, y);
    else
        return 0;
}

uint32 Map::GetAreaIdByAreaFlag(uint16 areaflag,uint32 map_id)
{
    AreaTableEntry const *entry = GetAreaEntryByAreaFlagAndMap(areaflag,map_id);

    if (entry)
        return entry->ID;
    else
        return 0;
}

uint32 Map::GetZoneIdByAreaFlag(uint16 areaflag,uint32 map_id)
{
    AreaTableEntry const *entry = GetAreaEntryByAreaFlagAndMap(areaflag,map_id);

    if (entry)
        return (entry->zone != 0) ? entry->zone : entry->ID;
    else
        return 0;
}

void Map::GetZoneAndAreaIdByAreaFlag(uint32& zoneid, uint32& areaid, uint16 areaflag,uint32 map_id)
{
    AreaTableEntry const *entry = GetAreaEntryByAreaFlagAndMap(areaflag,map_id);

    areaid = entry ? entry->ID : 0;
    zoneid = entry ? ((entry->zone != 0) ? entry->zone : entry->ID) : 0;
}

bool Map::IsInWater(float x, float y, float pZ, LiquidData *data) const
{
    // Check surface in x, y point for liquid
    if (const_cast<Map*>(this)->GetGrid(x, y))
    {
        LiquidData liquid_status;
        LiquidData *liquid_ptr = data ? data : &liquid_status;
        if (getLiquidStatus(x, y, pZ, MAP_ALL_LIQUIDS, liquid_ptr))
                return true;
    }
    return false;
}

bool Map::IsUnderWater(float x, float y, float z) const
{
    if (const_cast<Map*>(this)->GetGrid(x, y))
    {
        if (getLiquidStatus(x, y, z, MAP_LIQUID_TYPE_WATER|MAP_LIQUID_TYPE_OCEAN)&LIQUID_MAP_UNDER_WATER)
            return true;
    }
    return false;
}

bool Map::CheckGridIntegrity(Creature* c, bool moved) const
{
    Cell const& cur_cell = c->GetCurrentCell();

    CellPair xy_val = Trinity::ComputeCellPair(c->GetPositionX(), c->GetPositionY());
    Cell xy_cell(xy_val);
    if (xy_cell != cur_cell)
    {
        sLog->outDebug("Creature (GUID: %u) X: %f Y: %f (%s) is in grid[%u,%u]cell[%u,%u] instead of grid[%u,%u]cell[%u,%u]",
            c->GetGUIDLow(),
            c->GetPositionX(),c->GetPositionY(),(moved ? "final" : "original"),
            cur_cell.GridX(), cur_cell.GridY(), cur_cell.CellX(), cur_cell.CellY(),
            xy_cell.GridX(),  xy_cell.GridY(),  xy_cell.CellX(),  xy_cell.CellY());
        return true;                                        // not crash at error, just output error in debug mode
    }

    return true;
}

const char* Map::GetMapName() const
{
    return i_mapEntry ? i_mapEntry->name : "UNNAMEDMAP\x0";
}

void Map::UpdateObjectVisibility(WorldObject* obj, Cell cell, CellPair cellpair)
{
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();
    Trinity::VisibleChangesNotifier notifier(*obj);
    TypeContainerVisitor<Trinity::VisibleChangesNotifier, WorldTypeMapContainer > player_notifier(notifier);
    cell.Visit(cellpair, player_notifier, *this, *obj, GetVisibilityDistance());
}

void Map::UpdateObjectsVisibilityFor(Player* player, Cell cell, CellPair cellpair)
{
    Trinity::VisibleNotifier notifier(*player);

    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();
    TypeContainerVisitor<Trinity::VisibleNotifier, WorldTypeMapContainer > world_notifier(notifier);
    TypeContainerVisitor<Trinity::VisibleNotifier, GridTypeMapContainer  > grid_notifier(notifier);
    cell.Visit(cellpair, world_notifier, *this, *player, GetVisibilityDistance());
    cell.Visit(cellpair, grid_notifier,  *this, *player, GetVisibilityDistance());

    // send data
    notifier.SendToSelf();
}

void Map::SendInitSelf(Player * player)
{
    sLog->outDetail("Creating player data for himself %u", player->GetGUIDLow());

    UpdateData data(player->GetMapId());

    // attach to player data current transport data
    if (TransportBase* transportBase = player->GetTransport())
    {
        if (GameObject* goBase = transportBase->ToGameObject())
            goBase->BuildCreateUpdateBlockForPlayer(&data, player);
    }

    // build data for self presence in world at own client (one time for map)
    player->BuildCreateUpdateBlockForPlayer(&data, player);

    // build other passengers at transport also (they always visible and marked as visible and will not send at visibility update at add to map
    if (TransportBase* transportBase = player->GetTransport())
    {
        if (Transport* transport = transportBase->ToGenericTransport())
        {
            for (std::set<WorldObject*>::const_iterator itr = transport->GetPassengers().begin(); itr != transport->GetPassengers().end(); ++itr)
            {
                if (player != (*itr) && player->HaveAtClient(*itr))
                    (*itr)->BuildCreateUpdateBlockForPlayer(&data, player);
            }
        }
    }

    WorldPacket packet;
    data.BuildPacket(&packet);
    player->GetSession()->SendPacket(&packet);
}

void Map::SendInitTransports(Player * player)
{
    UpdateData transData(player->GetMapId());

    for (TransportsContainer::const_iterator i = _transports.begin(); i != _transports.end(); ++i)
        if (!(*i)->isActiveObject() && (player->GetTransport() == NULL || *i != player->GetTransport()->ToGameObject()))
            (*i)->BuildCreateUpdateBlockForPlayer(&transData, player);

    WorldPacket packet;
    transData.BuildPacket(&packet);
    player->GetSession()->SendPacket(&packet);
}

void Map::SendRemoveTransports(Player * player)
{
    UpdateData transData(player->GetMapId());

    for (TransportsContainer::const_iterator i = _transports.begin(); i != _transports.end(); ++i)
        if (player->GetTransport() == NULL || *i != player->GetTransport()->ToGameObject())
            (*i)->BuildOutOfRangeUpdateBlock(&transData);

    WorldPacket packet;
    transData.BuildPacket(&packet);
    player->GetSession()->SendPacket(&packet);
}

inline void Map::setNGrid(NGridType *grid, uint32 x, uint32 y)
{
    if (x >= MAX_NUMBER_OF_GRIDS || y >= MAX_NUMBER_OF_GRIDS)
    {
        sLog->outError("map::setNGrid() Invalid grid coordinates found: %d, %d!",x,y);
        ASSERT(false);
    }
    i_grids[x][y] = grid;
}

void Map::DelayedUpdate(const uint32 t_diff)
{
    RemoveAllObjectsInRemoveList();

    // Don't unload grids if it's battleground, since we may have manually added GOs,creatures, those doesn't load from DB at grid re-load !
    // This isn't really bother us, since as soon as we have instanced BG-s, the whole map unloads as the BG gets ended
    if (!IsBattlegroundOrArena())
    {
        for (GridRefManager<NGridType>::iterator i = GridRefManager<NGridType>::begin(); i != GridRefManager<NGridType>::end();)
        {
            NGridType *grid = i->getSource();
            GridInfo *info = i->getSource()->getGridInfoRef();
            ++i;                                                // The update might delete the map and we need the next map before the iterator gets invalid
            ASSERT(grid->GetGridState() >= 0 && grid->GetGridState() < MAX_GRID_STATE);
            si_GridStates[grid->GetGridState()]->Update(*this, *grid, *info, grid->getX(), grid->getY(), t_diff);
        }
    }
}

void Map::AddObjectToRemoveList(WorldObject *obj)
{
    if (obj->GetMapId() != GetId() || obj->GetInstanceId() != GetInstanceId())
        return;

    obj->CleanupsBeforeDelete(false);                            // remove or simplify at least cross referenced links

    i_objectsToRemove.insert(obj);
    //sLog->outDebug("Object (GUID: %u TypeId: %u) added to removing list.",obj->GetGUIDLow(),obj->GetTypeId());
}

void Map::AddObjectToSwitchList(WorldObject *obj, bool on)
{
    ASSERT(obj->GetMapId() == GetId() && obj->GetInstanceId() == GetInstanceId());

    std::map<WorldObject*, bool>::iterator itr = i_objectsToSwitch.find(obj);
    if (itr == i_objectsToSwitch.end())
        i_objectsToSwitch.insert(itr, std::make_pair(obj, on));
    else if (itr->second != on)
        i_objectsToSwitch.erase(itr);
    else
        ASSERT(false);
}

void Map::RemoveAllObjectsInRemoveList()
{
    while (!i_objectsToSwitch.empty())
    {
        std::map<WorldObject*, bool>::iterator itr = i_objectsToSwitch.begin();
        WorldObject *obj = itr->first;
        bool on = itr->second;
        i_objectsToSwitch.erase(itr);

        if (obj->GetTypeId() == TYPEID_UNIT || (obj->GetTypeId() == TYPEID_GAMEOBJECT && !obj->m_isWorldObject))
            SwitchGridContainers(obj, on);
    }

    //sLog->outDebug("Object remover 1 check.");
    while (!i_objectsToRemove.empty())
    {
        std::set<WorldObject*>::iterator itr = i_objectsToRemove.begin();
        WorldObject* obj = *itr;

        switch(obj->GetTypeId())
        {
            case TYPEID_CORPSE:
            {
                Corpse* corpse = sObjectAccessor->GetCorpse(*obj, obj->GetGUID());
                if (!corpse)
                    sLog->outError("Tried to delete corpse/bones %u that is not in map.", obj->GetGUIDLow());
                else
                    Remove(corpse,true);
                break;
            }
        case TYPEID_DYNAMICOBJECT:
            Remove((DynamicObject*)obj,true);
            break;
        case TYPEID_AREATRIGGER:
            Remove((AreaTrigger*)obj, true);
            break;
        case TYPEID_GAMEOBJECT:
            Remove((GameObject*)obj,true);
            break;
        case TYPEID_UNIT:
            // in case triggered sequence some spell can continue casting after prev CleanupsBeforeDelete call
            // make sure that like sources auras/etc removed before destructor start
            obj->ToCreature()->CleanupsBeforeDelete();
            Remove(obj->ToCreature(),true);
            break;
        default:
            sLog->outError("Non-grid object (TypeId: %u) is in grid object remove list, ignored.",obj->GetTypeId());
            break;
        }

        i_objectsToRemove.erase(itr);
    }

    //sLog->outDebug("Object remover 2 check.");
}

uint32 Map::GetPlayersCountExceptGMs() const
{
    uint32 count = 0;
    for (MapRefManager::const_iterator itr = m_mapRefManager.begin(); itr != m_mapRefManager.end(); ++itr)
        if (!itr->getSource()->IsGameMaster())
            ++count;
    return count;
}

void Map::SendToPlayers(WorldPacket const* data) const
{
    for (MapRefManager::const_iterator itr = m_mapRefManager.begin(); itr != m_mapRefManager.end(); ++itr)
        itr->getSource()->GetSession()->SendPacket(data);
}

bool Map::ActiveObjectsNearGrid(uint32 x, uint32 y) const
{
    ASSERT(x < MAX_NUMBER_OF_GRIDS);
    ASSERT(y < MAX_NUMBER_OF_GRIDS);

    CellPair cell_min(x*MAX_NUMBER_OF_CELLS, y*MAX_NUMBER_OF_CELLS);
    CellPair cell_max(cell_min.x_coord + MAX_NUMBER_OF_CELLS, cell_min.y_coord+MAX_NUMBER_OF_CELLS);

    //we must find visible range in cells so we unload only non-visible cells...
    float viewDist = GetVisibilityDistance();
    int cell_range = (int)ceilf(viewDist / SIZE_OF_GRID_CELL) + 1;

    cell_min << cell_range;
    cell_min -= cell_range;
    cell_max >> cell_range;
    cell_max += cell_range;

    for (MapRefManager::const_iterator iter = m_mapRefManager.begin(); iter != m_mapRefManager.end(); ++iter)
    {
        Player* plr = iter->getSource();

        CellPair p = Trinity::ComputeCellPair(plr->GetPositionX(), plr->GetPositionY());
        if ((cell_min.x_coord <= p.x_coord && p.x_coord <= cell_max.x_coord) &&
            (cell_min.y_coord <= p.y_coord && p.y_coord <= cell_max.y_coord))
            return true;
    }

    for (ActiveNonPlayers::const_iterator iter = m_activeNonPlayers.begin(); iter != m_activeNonPlayers.end(); ++iter)
    {
        WorldObject* obj = *iter;

        CellPair p = Trinity::ComputeCellPair(obj->GetPositionX(), obj->GetPositionY());
        if ((cell_min.x_coord <= p.x_coord && p.x_coord <= cell_max.x_coord) &&
            (cell_min.y_coord <= p.y_coord && p.y_coord <= cell_max.y_coord))
            return true;
    }

    return false;
}

template<class T>
void Map::AddToActive(T* obj)
{
    AddToActiveHelper(obj);
}

template <>
void Map::AddToActive(Creature* c)
{
    AddToActiveHelper(c);

    // also not allow unloading spawn grid to prevent creating creature clone at load
    if (!c->IsPet() && c->GetDBTableGUIDLow())
    {
        float x,y,z;
        c->GetRespawnCoord(x,y,z);
        GridPair p = Trinity::ComputeGridPair(x, y);
        if (getNGrid(p.x_coord, p.y_coord))
            getNGrid(p.x_coord, p.y_coord)->incUnloadActiveLock();
        else
        {
            GridPair p2 = Trinity::ComputeGridPair(c->GetPositionX(), c->GetPositionY());
            sLog->outError("Active creature (GUID: %u Entry: %u) added to grid[%u,%u] but spawn grid[%u,%u] was not loaded.",
                c->GetGUIDLow(), c->GetEntry(), p.x_coord, p.y_coord, p2.x_coord, p2.y_coord);
        }
    }
}

template<class T>
void Map::RemoveFromActive(T* obj)
{
    RemoveFromActiveHelper(obj);
}

template <>
void Map::RemoveFromActive(Creature* c)
{
    RemoveFromActiveHelper(c);

    // also allow unloading spawn grid
    if (!c->IsPet() && c->GetDBTableGUIDLow())
    {
        float x,y,z;
        c->GetRespawnCoord(x,y,z);
        GridPair p = Trinity::ComputeGridPair(x, y);
        if (getNGrid(p.x_coord, p.y_coord))
            getNGrid(p.x_coord, p.y_coord)->decUnloadActiveLock();
        else
        {
            GridPair p2 = Trinity::ComputeGridPair(c->GetPositionX(), c->GetPositionY());
            sLog->outError("Active creature (GUID: %u Entry: %u) removed from grid[%u,%u] but spawn grid[%u,%u] was not loaded.",
                c->GetGUIDLow(), c->GetEntry(), p.x_coord, p.y_coord, p2.x_coord, p2.y_coord);
        }
    }
}

template void Map::Add(Corpse *);
template void Map::Add(Creature *);
template void Map::Add(GameObject *);
template void Map::Add(DynamicObject *);
template void Map::Add(AreaTrigger*);

template void Map::Remove(Corpse *,bool);
template void Map::Remove(Creature *,bool);
template void Map::Remove(GameObject *, bool);
template void Map::Remove(DynamicObject *, bool);
template void Map::Remove(AreaTrigger*, bool);

template void Map::AddToActive(Corpse*);
template void Map::AddToActive(GameObject*);
template void Map::AddToActive(DynamicObject*);

template void Map::RemoveFromActive(DynamicObject*);

/* ******* Dungeon Instance Maps ******* */

InstanceMap::InstanceMap(uint32 id, time_t expiry, uint32 InstanceId, uint8 SpawnMode, Map* _parent)
  : Map(id, expiry, InstanceId, SpawnMode, _parent),
    m_resetAfterUnload(false), m_unloadWhenEmpty(false),
    i_data(NULL), i_script_id(0)
{
    //lets initialize visibility distance for dungeons
    InstanceMap::InitVisibilityDistance();

    // set timer to check combat inside instanceable map to spread combat to every single member
    m_checkCombatTimer = DEFAULT_INSTANCE_COMBAT_CHECK_TIME;

    // the timer is started by default, and stopped when the first player joins
    // this make sure it gets unloaded if for some reason no player joins
    m_unloadTimer = std::max(sWorld->getIntConfig(CONFIG_INSTANCE_UNLOAD_DELAY), (uint32)MIN_UNLOAD_DELAY);
}

InstanceMap::~InstanceMap()
{
    delete i_data;
    i_data = NULL;
}

void InstanceMap::InitVisibilityDistance()
{
    //init visibility distance for instances
    m_VisibleDistance = World::GetMaxVisibleDistanceInInstances();
    m_VisibilityNotifyPeriod = World::GetVisibilityNotifyPeriodInInstances();

    // Allow some explicit visibility distances for instance maps (should be less than SIZE_OF_GRIDS ?)
    switch (GetId())
    {
        case 938:   // End Time
        case 939:   // Well Of Eternity
        case 940:   // Hour of Twilight
        case 754:   // Throne of the Four Winds
        case 607:   // Strand of the Ancients
        case 967:   // Dragon Soul
            m_VisibleDistance = SIZE_OF_GRIDS - 1.0f;
            break;
        default:
            break;
    }
}

/*
    Do map specific checks to see if the player can enter
*/
bool InstanceMap::CanEnter(Player *player)
{
    if (player->GetMapRef().getTarget() == this)
    {
        sLog->outError("InstanceMap::CanEnter - player %s(%u) already in map %d,%d,%d!", player->GetName(), player->GetGUIDLow(), GetId(), GetInstanceId(), GetSpawnMode());
        return false;
    }

    // allow GM's to enter
    if (player->IsGameMaster())
        return Map::CanEnter(player);

    // cannot enter if the instance is full (player cap), GMs don't count
    uint32 maxPlayers = GetMaxPlayers();
    if (GetPlayersCountExceptGMs() >= maxPlayers)
    {
        sLog->outDetail("MAP: Instance '%u' of map '%s' cannot have more than '%u' players. Player '%s' rejected", GetInstanceId(), GetMapName(), maxPlayers, player->GetName());
        player->SendTransferAborted(GetId(), TRANSFER_ABORT_MAX_PLAYERS);
        return false;
    }

    // cannot enter while an encounter is in progress on raids
    /*Group *pGroup = player->GetGroup();
    if (!player->IsGameMaster() && pGroup && pGroup->InCombatToInstance(GetInstanceId()) && player->GetMapId() != GetId())*/
    if ( player->GetSession()->GetSecurity() == SEC_PLAYER && IsRaid() && GetInstanceScript() && GetInstanceScript()->IsEncounterInProgress()) 
    {
        player->SendTransferAborted(GetId(), TRANSFER_ABORT_ZONE_IN_COMBAT);
        return false;
    }

    // cannot enter if instance is in use by another party/soloer that have a
    // permanent save in the same instance id

    PlayerList const &playerList = GetPlayers();

    if (!playerList.isEmpty())
        for (PlayerList::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
            if (Player *iPlayer = i->getSource())
            {
                if (iPlayer->IsGameMaster()) // bypass GMs
                    continue;
                if (!player->GetGroup()) // player has not group and there is someone inside, deny entry
                {
                    player->SendTransferAborted(GetId(), TRANSFER_ABORT_MAX_PLAYERS);
                    return false;
                }
                // player inside instance has no group or his groups is different to entering player's one, deny entry
                if (!iPlayer->GetGroup() || iPlayer->GetGroup() != player->GetGroup() )
                {
                    player->SendTransferAborted(GetId(), TRANSFER_ABORT_MAX_PLAYERS);
                    return false;
                }
                break;
            }

    return Map::CanEnter(player);
}

/*
    Do map specific checks and add the player to the map if successful.
*/
bool InstanceMap::Add(Player *player)
{
    // TODO: Not sure about checking player level: already done in HandleAreaTriggerOpcode
    // GMs still can teleport player in instance.
    // Is it needed?

    {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, Lock, false);
        // Check moved to void WorldSession::HandleMoveWorldportAckOpcode()
        //if (!CanEnter(player))
            //return false;

        // Dungeon only code
        if (IsDungeon())
        {
            // get or create an instance save for the map
            InstanceSave *mapSave = sInstanceSaveMgr->GetInstanceSave(GetInstanceId());
            if (!mapSave)
            {
                sLog->outDetail("InstanceMap::Add: creating instance save for map %d spawnmode %d with instance id %d", GetId(), GetSpawnMode(), GetInstanceId());
                mapSave = sInstanceSaveMgr->AddInstanceSave(GetId(), GetInstanceId(), Difficulty(GetSpawnMode()), 0, true);
            }

            // Not sure where put this
            // It lets player know, that if he kills boss, he will be bound to this instance
            if(player->showInstanceBindQuery == true)//show only, if needed
            {
                player->showInstanceBindQuery=false;
                uint32 count = sInstanceSaveMgr->getBossNumber(GetId());
                uint32 bossescompleted = 0;
                if (count)
                {
                    std::map<uint32,uint32> uiEnc = sInstanceSaveMgr->getInstanceSaveData(GetInstanceId());
                    for (uint32 i = 0; i < count; i++)
                        if (uiEnc[i] == DONE)
                            bossescompleted |= 1 << (count-1-i);
                }
                WorldPacket data(SMSG_INSTANCE_LOCK_WARNING_QUERY, 4+4+1+1, true);
                uint8 diff = 1;
                if(isHeroicRaid()) 
                {
                    diff = 0;
                    setPlayerSaveTimer(player->GetGUIDLow(), 60000);//set timer for autobind
                }
                data << uint32(60000);               // time
                data << bossescompleted; // completed boss mask
                data << uint8(0);                // unk
                data << diff;                // some kind of switch, 1-with timer and leave 0-only accept
                player->SendMessageToSet(&data,true);
            }

            // check for existing instance binds
            InstancePlayerBind *playerBind;
            if(IsRaid() && sInstanceSaveMgr->isFlexibleEnabled(GetId()))
                playerBind = player->GetBoundInstance(GetId(), FLEXIBLE_RAID_DIFFICULTY);
            else
                playerBind = player->GetBoundInstance(GetId(), Difficulty(GetSpawnMode()));
            if (playerBind && playerBind->perm)
            {
                // cannot enter other instances if bound permanently
                if (playerBind->save != mapSave)
                {
                    sLog->outError("InstanceMap::Add: player %s(%d) is permanently bound to instance %d,%d,%d,%d,%d,%d but he is being put into instance %d,%d,%d,%d,%d,%d", player->GetName(), player->GetGUIDLow(), playerBind->save->GetMapId(), playerBind->save->GetInstanceId(), playerBind->save->GetDifficulty(), playerBind->save->GetPlayerCount(), playerBind->save->GetGroupCount(), playerBind->save->CanReset(), mapSave->GetMapId(), mapSave->GetInstanceId(), mapSave->GetDifficulty(), mapSave->GetPlayerCount(), mapSave->GetGroupCount(), mapSave->CanReset());
                    return false;
                }
            }
            else
            {
                Group *pGroup = player->GetGroup();
                if (pGroup)
                {
                    // solo saves should be reset when entering a group
                    InstanceGroupBind *groupBind = pGroup->GetBoundInstance(this);
                    if (playerBind)
                    {
                        sLog->outError("InstanceMap::Add: player %s(%d) is being put into instance %d,%d,%d,%d,%d,%d but he is in group %d and is bound to instance %d,%d,%d,%d,%d,%d!", player->GetName(), player->GetGUIDLow(), mapSave->GetMapId(), mapSave->GetInstanceId(), mapSave->GetDifficulty(), mapSave->GetPlayerCount(), mapSave->GetGroupCount(), mapSave->CanReset(), GUID_LOPART(pGroup->GetLeaderGUID()), playerBind->save->GetMapId(), playerBind->save->GetInstanceId(), playerBind->save->GetDifficulty(), playerBind->save->GetPlayerCount(), playerBind->save->GetGroupCount(), playerBind->save->CanReset());
                        if (groupBind) sLog->outError("InstanceMap::Add: the group is bound to the instance %d,%d,%d,%d,%d,%d", groupBind->save->GetMapId(), groupBind->save->GetInstanceId(), groupBind->save->GetDifficulty(), groupBind->save->GetPlayerCount(), groupBind->save->GetGroupCount(), groupBind->save->CanReset());
                        //ASSERT(false);
                        return false;
                    }
                    // bind to the group or keep using the group save
                    if (!groupBind)
                        pGroup->BindToInstance(mapSave, false);
                    else
                    {
                        // cannot jump to a different instance without resetting it
                        if (groupBind->save != mapSave)
                        {
                            sLog->outError("InstanceMap::Add: player %s(%d) is being put into instance %d,%d,%d but he is in group %d which is bound to instance %d,%d,%d!", player->GetName(), player->GetGUIDLow(), mapSave->GetMapId(), mapSave->GetInstanceId(), mapSave->GetDifficulty(), GUID_LOPART(pGroup->GetLeaderGUID()), groupBind->save->GetMapId(), groupBind->save->GetInstanceId(), groupBind->save->GetDifficulty());
                            if (mapSave)
                                sLog->outError("MapSave players: %d, group count: %d", mapSave->GetPlayerCount(), mapSave->GetGroupCount());
                            else
                                sLog->outError("MapSave NULL");
                            if (groupBind->save)
                                sLog->outError("GroupBind save players: %d, group count: %d", groupBind->save->GetPlayerCount(), groupBind->save->GetGroupCount());
                            else
                                sLog->outError("GroupBind save NULL");
                            return false;
                        }
                        // if the group/leader is permanently bound to the instance
                        // players also become permanently bound when they enter
                        if (groupBind->perm)
                        {
                            WorldPacket data(SMSG_INSTANCE_SAVE_CREATED, 4);
                            data << uint32(0);
                            player->GetSession()->SendPacket(&data);
                            player->BindToInstance(mapSave, true);
                        }
                    }
                }
                else
                {
                    // set up a solo bind or continue using it
                    if (!playerBind)
                        player->BindToInstance(mapSave, false);
                    else
                        // cannot jump to a different instance without resetting it
                        ASSERT(playerBind->save == mapSave);
                }
            }
        }

        // for normal instances cancel the reset schedule when the
        // first player enters (no players yet)
        SetResetSchedule(false);

        sLog->outDetail("MAP: Player '%s' entered instance '%u' of map '%s'", player->GetName(), GetInstanceId(), GetMapName());
        // initialize unload state
        m_unloadTimer = 0;
        m_resetAfterUnload = false;
        m_unloadWhenEmpty = false;
    }

    // this will acquire the same mutex so it cannot be in the previous block
    Map::Add(player);

    if (i_data)
        i_data->OnPlayerEnter(player);

    return true;
}

void InstanceMap::Update(const uint32& t_diff)
{
    Map::Update(t_diff);

    // Here we will check whole InstanceMap group for combat, and if present, spread it to everyone
    if (m_checkCombatTimer <= t_diff)
    {
        m_checkCombatTimer = DEFAULT_INSTANCE_COMBAT_CHECK_TIME;

        Map::PlayerList const& plList = GetPlayers();
        if (!plList.isEmpty())
        {
            // At first we check raid group for combat and save threat list from at least one of them
            // We dont have to save a loads of references, if somebody doesnt have threat with that reference,
            // he would be surely "in combat" even without it, and thats the spirit
            bool combatPresent = false;
            HostileRefManager* threatList = NULL;
            for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
            {
                if (itr->getSource() && itr->getSource()->IsInCombat())
                {
                    combatPresent = true;
                    threatList = &(itr->getSource()->getHostileRefManager());
                    break;
                }
            }

            // And finally if combat is present
            if (combatPresent && threatList && !threatList->isEmpty())
            {
                for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
                {
                    // and somebody doesnt have combat
                    if (itr->getSource() && !itr->getSource()->IsInCombat() && !itr->getSource()->IsGameMaster())
                    {
                        // we will make (at least) one for him :)
                        for (HostileRefManager::iterator iter = threatList->begin(); iter != threatList->end(); ++iter)
                        {
                            if (iter->getSource() && iter->getSource()->getOwner() && itr->getSource()->IsWithinDistInMap(iter->getSource()->getOwner(), 200.0f))
                            {
                                itr->getSource()->SetInCombatWith(iter->getSource()->getOwner());
                                iter->getSource()->getOwner()->AddThreat(itr->getSource(), 0.0f);
                            }
                        }
                    }
                }
            }
        }
    }
    else
        m_checkCombatTimer -= t_diff;

    // Here we will if the player needs to be autosaved to the instance
    Map::PlayerList const& plList = GetPlayers();
    if (!plList.isEmpty())
    {
        for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
        {
            Player* player=itr->getSource();
            if(savePlayerTimers[player->GetGUIDLow()])//player has timer
            {
                if(savePlayerTimers[player->GetGUIDLow()] <= t_diff)//timer is ending
                {
                    removePlayerSaveTimer(player->GetGUIDLow());
                    copyDeadUnitsFromLeader(player, GetId(), GetInstanceId(),0);
                    uint32 mapId=GetId();
                    uint32 instanceId=player->getRaidId(mapId);
                    if(player->getRaidDiffProgr(mapId) < KILLED_HC)
                        CharacterDatabase.PExecute("UPDATE character_instance SET diffProgress = '%u' where guid = '%u' AND instance = '%u'", KILLED_HC, player->GetGUIDLow(), instanceId);
                }
                else//timer has some time
                    savePlayerTimers[player->GetGUIDLow()] -= t_diff;
            }
        }
    }

    if (i_data)
        i_data->Update(t_diff);
}

void InstanceMap::Remove(Player *player, bool remove)
{
    removePlayerSaveTimer(player->GetGUIDLow());//remove running timer for player
    sLog->outDetail("MAP: Removing player '%s' from instance '%u' of map '%s' before relocating to another map", player->GetName(), GetInstanceId(), GetMapName());
    //if last player set unload timer
    if (!m_unloadTimer && m_mapRefManager.getSize() == 1)
        m_unloadTimer = m_unloadWhenEmpty ? MIN_UNLOAD_DELAY : std::max(sWorld->getIntConfig(CONFIG_INSTANCE_UNLOAD_DELAY), (uint32)MIN_UNLOAD_DELAY);
    Map::Remove(player, remove);
    // for normal instances schedule the reset after all players have left
    SetResetSchedule(true);
}

void InstanceMap::CreateInstanceData(bool load)
{
    if (i_data != NULL)
        return;

    InstanceTemplate const* mInstance = sObjectMgr->GetInstanceTemplate(GetId());
    if (mInstance)
    {
        i_script_id = mInstance->script_id;
        i_data = sScriptMgr->CreateInstanceData(this);
    }

    if (!i_data)
        return;

    i_data->Initialize();

    if (load)
    {
        // TODO: make a global storage for this
        QueryResult result = CharacterDatabase.PQuery("SELECT data FROM instance WHERE map = '%u' AND id = '%u'", GetId(), i_InstanceId);
        if (result)
        {
            Field* fields = result->Fetch();
            std::string data = fields[0].GetString();
            if (data != "")
            {
                sLog->outDebug("Loading instance data for `%s` with id %u", sObjectMgr->GetScriptName(i_script_id), i_InstanceId);
                i_data->Load(data.c_str());
            }
        }
    }
}

/*
    Returns true if there are no players in the instance
*/
bool InstanceMap::Reset(uint8 method)
{
    // note: since the map may not be loaded when the instance needs to be reset
    // the instance must be deleted from the DB by InstanceSaveManager

    if (HavePlayers())
    {
        if (method == INSTANCE_RESET_ALL || method == INSTANCE_RESET_CHANGE_DIFFICULTY)
        {
            // notify the players to leave the instance so it can be reset
            for (MapRefManager::iterator itr = m_mapRefManager.begin(); itr != m_mapRefManager.end(); ++itr)
                itr->getSource()->SendResetFailedNotify(GetId());
        }
        else
        {
            if (method == INSTANCE_RESET_GLOBAL)
                // set the homebind timer for players inside (1 minute)
                for (MapRefManager::iterator itr = m_mapRefManager.begin(); itr != m_mapRefManager.end(); ++itr)
                    itr->getSource()->m_InstanceValid = false;

            // the unload timer is not started
            // instead the map will unload immediately after the players have left
            m_unloadWhenEmpty = true;
            m_resetAfterUnload = true;
        }
    }
    else
    {
        // unloaded at next update
        m_unloadTimer = MIN_UNLOAD_DELAY;
        m_resetAfterUnload = true;
    }

    return m_mapRefManager.isEmpty();
}

void InstanceMap::PermBindAllPlayers(Player *player, Unit* pUnit)
{
    if (!IsDungeon())
        return;

    if (!pUnit)
        return;

    InstanceSave *save = sInstanceSaveMgr->GetInstanceSave(GetInstanceId());
    if (!save)
    {
        sLog->outError("Cannot bind players, no instance save available for map!");
        return;
    }

    Group *group = player->GetGroup();
    // group members outside the instance group don't get bound
    for (MapRefManager::iterator itr = m_mapRefManager.begin(); itr != m_mapRefManager.end(); ++itr)
    {
        Player* plr = itr->getSource();
        // players inside an instance cannot be bound to other instances
        // some players may already be permanently bound, in this case nothing happens
        InstancePlayerBind *bind = plr->GetBoundInstance(save->GetMapId(), save->GetDifficulty());

        if (!bind || !bind->perm ||bind->save!=save)
        {
            plr->BindToInstance(save, true);
            WorldPacket data(SMSG_INSTANCE_SAVE_CREATED, 4);
            data << uint32(0);
            plr->GetSession()->SendPacket(&data);

            player->GetSession()->SendCalendarRaidLockout(save, true);
        }

        sLog->outChar("IP:(%s) account:(%u) character:(%s) action:(%s) entry:(%u) name:(%s) instance:(%s (id:(%u)) %s:(X:(%f) Y:(%f) Z:(%f) map:(%u))",
            plr->GetSession()->GetRemoteAddress().c_str(),
            plr->GetSession()->GetAccountId(),
            plr->GetName(),
            "save bind",
            pUnit->GetEntry(),
            pUnit->GetName(),
            GetMapName(),
            GetInstanceId(),
            "pos",
            plr->GetPositionX(),
            plr->GetPositionY(),
            plr->GetPositionZ(),
            plr->GetMapId());

        // if the leader is not in the instance the group will not get a perm bind
        if (group && group->GetLeaderGUID() == plr->GetGUID())
            group->BindToInstance(save, true);
    }
}

void InstanceMap::UnloadAll()
{
    ASSERT(!HavePlayers());

    if (m_resetAfterUnload == true)
        sObjectMgr->DeleteRespawnTimeForInstance(GetInstanceId());

    Map::UnloadAll();
}

void InstanceMap::SendResetWarnings(uint32 timeLeft) const
{
    for (MapRefManager::const_iterator itr = m_mapRefManager.begin(); itr != m_mapRefManager.end(); ++itr)
        itr->getSource()->SendInstanceResetWarning(GetId(), itr->getSource()->GetDifficulty(IsRaid()), timeLeft);
}

void InstanceMap::SetResetSchedule(bool on)
{
    // only for normal instances
    // the reset time is only scheduled when there are no payers inside
    // it is assumed that the reset time will rarely (if ever) change while the reset is scheduled
    if (IsDungeon() && !HavePlayers() && !IsRaidOrHeroicDungeon())
    {
        InstanceSave *save = sInstanceSaveMgr->GetInstanceSave(GetInstanceId());
        if (!save) sLog->outError("InstanceMap::SetResetSchedule: cannot turn schedule %s, no save available for instance %d of %d", on ? "on" : "off", GetInstanceId(), GetId());
        else sInstanceSaveMgr->ScheduleReset(on, save->GetResetTime(), InstanceSaveManager::InstResetEvent(0, GetId(), Difficulty(GetSpawnMode()), GetInstanceId()));
    }
}

void InstanceMap::doDifficultyStaff(Player* player, uint32 mapId, uint32 instanceId)
{
    uint32 insId=player->getRaidId(mapId);
    if(!insId)
        insId=instanceId;
    if(player->getRaidDiffProgr(mapId) != KILLED_HC && (player->GetDifficulty(true) == RAID_DIFFICULTY_10MAN_HEROIC || player->GetDifficulty(true) == RAID_DIFFICULTY_25MAN_HEROIC)) //killed boss on HC so cannot enter HC with other group
    {
        player->setRaidDiffProgr(mapId , KILLED_HC);
        CharacterDatabase.PExecute("UPDATE character_instance SET diffProgress = '%u' where guid = '%u' AND instance = '%u'", player->getRaidDiffProgr(mapId), player->GetGUIDLow(), insId);
    }
    if(player->getRaidDiffProgr(mapId) == KILLED_HC && (player->GetDifficulty(true) == RAID_DIFFICULTY_10MAN_NORMAL || player->GetDifficulty(true) == RAID_DIFFICULTY_25MAN_NORMAL)) //killed on HC and the on N so cannot enter any HC anymore
    {
        player->setRaidDiffProgr(mapId , KILLED_HC_N_MERGED);
        CharacterDatabase.PExecute("UPDATE character_instance SET diffProgress = '%u' where guid = '%u' AND instance = '%u'", player->getRaidDiffProgr(mapId), player->GetGUIDLow(), insId);
    }
}

void InstanceMap::copyDeadUnitsFromLeader(Player* player, uint32 mapId, uint32 instanceId, uint32 unitGuidDB/*=0*/)
{
    uint32 playId=player->getRaidId(mapId);
    if(playId!=instanceId)
    {
        /*Send saved to instance warning*/
        WorldPacket data(SMSG_INSTANCE_SAVE_CREATED, 4);
        data << uint32(0);
        player->GetSession()->SendPacket(&data);

        CharacterDatabase.PExecute("REPLACE INTO creature_respawn(guid,respawnTime,instance) SELECT guid,respawnTime,'%d' FROM creature_respawn WHERE instance = '%d'", playId, instanceId);
        QueryResult result = CharacterDatabase.PQuery("SELECT guid, respawnTime FROM creature_respawn WHERE instance = '%d'", instanceId);
        if (result)
        {
            do
            {
                Field* fields = result->Fetch();

                uint32 guid = fields[0].GetUInt32();
                uint32 time = fields[1].GetUInt32();
                sObjectMgr->SaveCreatureRespawnTimeWithoutDB(guid,playId,time);
            }
            while (result->NextRow());
        }

        if(GetInstanceScript())
        {
            std::string data=this->GetInstanceScript()->GetSaveData();
            CharacterDatabase.PExecute("UPDATE instance SET data='%s' WHERE id = '%d'",data.c_str(),playId);
            Map* map=sMapMgr->FindMap(mapId,playId);
            if(map)
            {
                map->ToInstanceMap()->GetInstanceScript()->Load(data.c_str());
                if(!map->HavePlayers())
                {
                    map->UnloadAll();
                }
            }
        }
    }
}

MapDifficulty const* Map::GetMapDifficulty() const
{
    return GetMapDifficultyData(GetId(),GetDifficulty());
}

uint32 InstanceMap::GetMaxPlayers() const
{
    if (MapDifficulty const* mapDiff = GetMapDifficulty())
    {
        if (mapDiff->maxPlayers || IsRegularDifficulty())    // Normal case (expect that regular difficulty always have correct maxplayers)
            return mapDiff->maxPlayers;
        else                                                // DBC have 0 maxplayers for heroic instances with expansion < 2
        {                                                   // The heroic entry exists, so we don't have to check anything, simply return normal max players
            MapDifficulty const* normalDiff = GetMapDifficultyData(GetId(), REGULAR_DIFFICULTY);
            return normalDiff ? normalDiff->maxPlayers : 0;
        }
    }
    else                                                    // I'd rather ASSERT(false);
        return 0;
}

uint32 InstanceMap::GetMaxResetDelay() const
{
    MapDifficulty const* mapDiff = GetMapDifficulty();
    return mapDiff ? mapDiff->resetTime : 0;
}

/* ******* Battleground Instance Maps ******* */

BattlegroundMap::BattlegroundMap(uint32 id, time_t expiry, uint32 InstanceId, Map* _parent, uint8 spawnMode)
  : Map(id, expiry, InstanceId, spawnMode, _parent)
{
    //lets initialize visibility distance for BG/Arenas
    BattlegroundMap::InitVisibilityDistance();
}

BattlegroundMap::~BattlegroundMap()
{
}

void BattlegroundMap::InitVisibilityDistance()
{
    //init visibility distance for BG/Arenas
    m_VisibleDistance = World::GetMaxVisibleDistanceInBGArenas();
    m_VisibilityNotifyPeriod = World::GetVisibilityNotifyPeriodInBGArenas();
}

bool BattlegroundMap::CanEnter(Player * player)
{
    if (player->GetMapRef().getTarget() == this)
    {
        sLog->outError("BGMap::CanEnter - player %u is already in map!", player->GetGUIDLow());
        return false;
    }

    if (player->GetBattlegroundId() != GetInstanceId())
        return false;

    // player number limit is checked in bgmgr, no need to do it here

    return Map::CanEnter(player);
}

bool BattlegroundMap::Add(Player * player)
{
    {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, Lock, false);
        //Check moved to void WorldSession::HandleMoveWorldportAckOpcode()
        //if (!CanEnter(player))
            //return false;
        // reset instance validity, battleground maps do not homebind
        player->m_InstanceValid = true;
    }
    return Map::Add(player);
}

void BattlegroundMap::Remove(Player *player, bool remove)
{
    sLog->outDetail("MAP: Removing player '%s' from bg '%u' of map '%s' before relocating to another map", player->GetName(), GetInstanceId(), GetMapName());
    Map::Remove(player, remove);
}

void BattlegroundMap::SetUnload()
{
    m_unloadTimer = MIN_UNLOAD_DELAY;
}

void BattlegroundMap::RemoveAllPlayers()
{
    if (HavePlayers())
        for (MapRefManager::iterator itr = m_mapRefManager.begin(); itr != m_mapRefManager.end(); ++itr)
            if (Player* plr = itr->getSource())
                if (!plr->IsBeingTeleportedFar())
                    plr->TeleportTo(plr->GetBattlegroundEntryPoint());

}

Creature*
Map::GetCreature(uint64 guid)
{
    return ObjectAccessor::GetObjectInMap(guid, this, (Creature*)NULL);
}

GameObject*
Map::GetGameObject(uint64 guid)
{
    return ObjectAccessor::GetObjectInMap(guid, this, (GameObject*)NULL);
}

Transport* Map::GetTransport(uint64 guid)
{
    if (GUID_HIPART(guid) != HIGHGUID_MO_TRANSPORT)
        return NULL;

    GameObject* go = GetGameObject(guid);
    return go ? go->ToTransport() : NULL;
}

DynamicObject*
Map::GetDynamicObject(uint64 guid)
{
    return ObjectAccessor::GetObjectInMap(guid, this, (DynamicObject*)NULL);
}

void Map::UpdateIteratorBack(Player *player)
{
    if (m_mapRefIter == player->GetMapRef())
        m_mapRefIter = m_mapRefIter->nocheck_prev();
}
