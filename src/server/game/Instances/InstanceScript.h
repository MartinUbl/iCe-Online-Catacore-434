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

#ifndef TRINITY_INSTANCE_DATA_H
#define TRINITY_INSTANCE_DATA_H

#include "ZoneScript.h"
#include "World.h"

#define OUT_SAVE_INST_DATA             sLog->outDebug("TSCR: Saving Instance Data for Instance %s (Map %d, Instance Id %d)", instance->GetMapName(), instance->GetId(), instance->GetInstanceId())
#define OUT_SAVE_INST_DATA_COMPLETE    sLog->outDebug("TSCR: Saving Instance Data for Instance %s (Map %d, Instance Id %d) completed.", instance->GetMapName(), instance->GetId(), instance->GetInstanceId())
#define OUT_LOAD_INST_DATA(a)          sLog->outDebug("TSCR: Loading Instance Data for Instance %s (Map %d, Instance Id %d). Input is '%s'", instance->GetMapName(), instance->GetId(), instance->GetInstanceId(), a)
#define OUT_LOAD_INST_DATA_COMPLETE    sLog->outDebug("TSCR: Instance Data Load for Instance %s (Map %d, Instance Id: %d) is complete.",instance->GetMapName(), instance->GetId(), instance->GetInstanceId())
#define OUT_LOAD_INST_DATA_FAIL        sLog->outError("TSCR: Unable to load Instance Data for Instance %s (Map %d, Instance Id: %d).",instance->GetMapName(), instance->GetId(), instance->GetInstanceId())

class Map;
class Unit;
class Player;
class GameObject;
class Creature;

typedef std::set<GameObject*> DoorSet;
typedef std::set<Creature*> MinionSet;

enum EncounterFrameType
{
    ENCOUNTER_FRAME_SET_COMBAT_RES_LIMIT    = 0,
    ENCOUNTER_FRAME_RESET_COMBAT_RES_LIMIT  = 1,
    ENCOUNTER_FRAME_ENGAGE                  = 2,
    ENCOUNTER_FRAME_DISENGAGE               = 3,
    ENCOUNTER_FRAME_UPDATE_PRIORITY         = 4,
    ENCOUNTER_FRAME_ADD_TIMER               = 5,
    ENCOUNTER_FRAME_ENABLE_OBJECTIVE        = 6,
    ENCOUNTER_FRAME_UPDATE_OBJECTIVE        = 7,
    ENCOUNTER_FRAME_DISABLE_OBJECTIVE       = 8,
    ENCOUNTER_FRAME_UNK7                    = 9,    // Seems to have something to do with sorting the encounter units
    ENCOUNTER_FRAME_ADD_COMBAT_RES_LIMIT    = 10
};

enum EncounterState
{
    NOT_STARTED   = 0,
    IN_PROGRESS   = 1,
    FAIL          = 2,
    DONE          = 3,
    SPECIAL       = 4,
    TO_BE_DECIDED = 5,
};

enum DoorType
{
    DOOR_TYPE_ROOM = 0,
    DOOR_TYPE_PASSAGE,
    MAX_DOOR_TYPES,
};

enum BoundaryType
{
    BOUNDARY_NONE = 0,
    BOUNDARY_N,
    BOUNDARY_S,
    BOUNDARY_E,
    BOUNDARY_W,
    BOUNDARY_NE,
    BOUNDARY_NW,
    BOUNDARY_SE,
    BOUNDARY_SW,
    BOUNDARY_MAX_X = BOUNDARY_N,
    BOUNDARY_MIN_X = BOUNDARY_S,
    BOUNDARY_MAX_Y = BOUNDARY_W,
    BOUNDARY_MIN_Y = BOUNDARY_E,
};

typedef std::map<BoundaryType, float> BossBoundaryMap;

struct DoorData
{
    uint32 entry, bossId;
    DoorType type;
    uint32 boundary;
};

struct MinionData
{
    uint32 entry, bossId;
};

struct BossInfo
{
    BossInfo() : state(TO_BE_DECIDED) {}
    EncounterState state;
    DoorSet door[MAX_DOOR_TYPES];
    MinionSet minion;
    BossBoundaryMap boundary;
};

struct DoorInfo
{
    explicit DoorInfo(BossInfo *_bossInfo, DoorType _type, BoundaryType _boundary)
        : bossInfo(_bossInfo), type(_type), boundary(_boundary) {}
    BossInfo *bossInfo;
    DoorType type;
    BoundaryType boundary;
};

struct MinionInfo
{
    explicit MinionInfo(BossInfo *_bossInfo) : bossInfo(_bossInfo) {}
    BossInfo *bossInfo;
};

typedef std::multimap<uint32 /*entry*/, DoorInfo> DoorInfoMap;
typedef std::map<uint32 /*entry*/, MinionInfo> MinionInfoMap;

class InstanceScript : public ZoneScript
{
    public:

        explicit InstanceScript(Map *map) : instance(map) {}
        virtual ~InstanceScript() {}

        Map *instance;

        //On creation, NOT load.
        virtual void Initialize() {}

        //On load
        virtual void Load(const char * data) { LoadBossState(data); }

        //When save is needed, this function generates the data
        virtual std::string GetSaveData() { return GetBossSaveData(); }

        void SaveToDB();

        virtual void Update(uint32 /*diff*/) {}

        //Used by the map's CanEnter function.
        //This is to prevent players from entering during boss encounters.
        virtual bool IsEncounterInProgress() const;

        //Called when a player successfully enters the instance.
        virtual void OnPlayerEnter(Player *) {}

        //Handle open / close objects
        //use HandleGameObject(0, boolen, GO); in OnObjectCreate in instance scripts
        //use HandleGameObject(GUID, boolen, NULL); in any other script
        void HandleGameObject(uint64 GUID, bool open, GameObject *go = NULL);

        //change active state of doors or buttons
        void DoUseDoorOrButton(uint64 uiGuid, uint32 uiWithRestoreTime = 0, bool bUseAlternativeState = false);

        //Respawns a GO having negative spawntimesecs in gameobject-table
        void DoRespawnGameObject(uint64 uiGuid, uint32 uiTimeToDespawn = MINUTE);

        //sends world state update to all players in instance
        void DoUpdateWorldState(uint32 uiStateId, uint32 uiStateData);

        // Send Notify to all players in instance
        void DoSendNotifyToInstance(const char *format,...);

        // Complete Achievement for all players in instance
        void DoCompleteAchievement(uint32 achievement);

        // Update Achievement Criteria for all players in instance
        void DoUpdateAchievementCriteria(AchievementCriteriaTypes type, uint32 miscvalue1=0, uint32 miscvalue2=0, Unit *unit=NULL, uint32 time=0);

        // Start/Stop Timed Achievement Criteria for all players in instance
        void DoStartTimedAchievement(AchievementCriteriaTimedTypes type, uint32 entry);
        void DoStopTimedAchievement(AchievementCriteriaTimedTypes type, uint32 entry);

        // Remove Auras due to Spell on all players in instance
        void DoRemoveAurasDueToSpellOnPlayers(uint32 spell);

        // Set scripted power to all players in instance
        void DoSetScriptedPowerToPlayers(int32 amount);
        void DoSetMaxScriptedPowerToPlayers(int32 amount);

        //add/remove boss frames (0=ADD,1=REMOVE)
        void SendEncounterUnit(uint32 type, Unit* unit, uint8 param1 = 0, uint8 param2 = 0);

        // Return wether server allow two side groups or not
        bool ServerAllowsTwoSideGroups() { return sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GROUP); }

        virtual bool SetBossState(uint32 id, EncounterState state);
        EncounterState GetBossState(uint32 id) const { return id < bosses.size() ? bosses[id].state : TO_BE_DECIDED; }
        const BossBoundaryMap * GetBossBoundary(uint32 id) const { return id < bosses.size() ? &bosses[id].boundary : NULL; }
        uint32 GetBossNumber() { return bosses.size(); }

        // Achievement criteria additional requirements check
        // NOTE: not use this if same can be checked existed requirement types from AchievementCriteriaRequirementType
        virtual bool CheckAchievementCriteriaMeet(uint32 /*criteria_id*/, Player const* /*source*/, Unit const* /*target*/ = NULL, uint32 /*miscvalue1*/ = 0);
        virtual uint32* GetUiEncounter(){return NULL;}
        virtual uint32* GetCorrUiEncounter(){return GetUiEncounter();}
        virtual uint32 GetMaxEncounter(){return 0;}
        virtual uint32 GetCorrMaxEncounter(){return GetMaxEncounter();}
        uint8 getPlayerDifficulty(Player* play)
        {
            if(play->GetGroup()&&play->GetGroup()->GetLeader())
                return play->GetGroup()->GetLeader()->GetRaidDifficulty();
            return play->GetRaidDifficulty();
        }
        void repairDifficulty(Player* play)
        {
            instance->setSpawnMode(getPlayerDifficulty(play));
        }
        bool CorrectDiff(Player* play)
        {
            if(instance->GetSpawnMode()!=getPlayerDifficulty(play))
               return false;
            return true;
        }
    protected:
        void SetBossNumber(uint32 number) { bosses.resize(number); }
        void LoadDoorData(const DoorData *data);
        void LoadMinionData(const MinionData *data);

        void AddDoor(GameObject *door, bool add);
        void AddMinion(Creature *minion, bool add);

        void UpdateDoorState(GameObject *door);
        void UpdateMinionState(Creature *minion, EncounterState state);

        std::string LoadBossState(const char * data);
        std::string GetBossSaveData();
    private:
        std::vector<BossInfo> bosses;
        DoorInfoMap doors;
        MinionInfoMap minions;
};
#endif

