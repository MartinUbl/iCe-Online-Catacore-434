/*
 * Copyright (C) 2006-2013 iCe Online <http://ice-wow.eu>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _WELL_OF_ETER_H_
#define _WELL_OF_ETER_H_

// Max timer
#define NEVER  (0xffffffff)

// First demons Z coordinate
#define START_Z_COORD (189.53f)

// Middle of Perotharn room
#define COURTYARD_X (3339.75f)
#define COURTYARD_Y (-4897.18f)
#define COURTYARD_Z (181.08f)

// Coordinates of first escape waypoint for Perotharn and demons/arcanist (INTRO)
#define ESCAPE_X (3192.74f)
#define ESCAPE_Y (-4903.62f)
#define ESCAPE_Z (194.35f)

enum EncounterData
{
   DATA_PEROTHARN,
   DATA_QUEEN_AZSHARA,
   DATA_MANNOROTH,
   DATA_CAPTAIN_VAROTHEN
};

enum BossEntry
{
    PEROTHARN_ENTRY = 55085,
    QUEEN_AZSHARA_ENTRY = 54853,
    MANNOROTH_ENTRY = 54969,
    VAROTHEN_ENTRY = 55419
};

enum WorldState
{
    WORLD_STATE_DEMONS_IN_COURTYARD = 6018
};

enum goEntry
{
    TIME_TRANSIT_DEVICE_ENTRY = 210000,
    PORTAL_ENERGY_FOCUS_ENTRY = 209366
};

enum TrashEntry
{
    ILLIDAN_STORMRAGE_ENTRY = 55500,
    NOZDORMU_ENTRY = 55624,

    FIREWALL_ENTRY = 250301, // not sure if correct
    FIREWALL_INVIS_ENTRY = 210097,

    PORTAL_CONNECTOR_1_ENTRY = 55541,
    PORTAL_CONNECTOR_2_ENTRY = 55542,
    PORTAL_CONNECTOR_3_ENTRY = 55543,

    LEGION_PORTAL_DUMMY = 54513,

    LEGION_DEMON_ENTRY = 55503, // static demons
    GUARDIAN_DEMON_ENTRY = 54927, // black guys

    CORRUPTED_ARCANIST_ENTRY = 55654,
    DREADLORD_DEFFENDER_ENTRY = 55656,

    FEL_CRYSTAL_ENTRY = 55917,
    FEL_CRYSTAL_STALKER_ENTRY = 55965,

    VIGILANT_SHIVAN_ENTRY = 55663,
    FELHOUND_ENTRY = 55662,

    LEGION_PORTAL_DEMON = 54500,
    DISTRACION_STALKER_ENTRY = 58200,
};

enum CrystalOrientations
{
    FIRST_CRYSTAL_X_COORD = 3288,
    SECOND_CRYSTAL_X_COORD = 3446,
    THIRD_CRYSTAL_X_COORD = 3478
};

enum TrashSpells
{
    SPELL_DISTRACTION_AURA = 110082,

    SPELL_PORTAL_STATUS_ACTIVE = 102456,
    SPELL_PORTAL_STATUS_SHUTTING_DOWN = 102457,

    SPELL_CRYSTAL_MELTDOWN = 105074, // intesne green light
    SPELL_SHATTER_CRYSTALS = 105004, // some kind of explosion -> some kind of die anim kit and then freeze anim
    SPELL_CRYSTAL_DESTRUCTION = 105079, // another explosion ?
    SPELL_CRYSTAL_FEL_GROUND = 105119,
    SPELL_CRYSTAL_PERIODIC = 105055, // periodic green lightining
    SPELL_CRYSTAL_PERIODIC_TRIGGERED = 105018, // triggered by spell above
    SPELL_FEL_CRYSTAL_CONNECTOR_COSMETIC = 105192, // visual green crystal under unit
    SPELL_FEL_CRYSTAL_STALKER_GLOW = 105046,

    SPELL_PORTAL_BEAM_CONNECTOR_GREEN = 104034,
    SPELL_CONNECTOR_DEAD = 105201,

    SPELL_FIREWALL_PERIODIC_DUMMY = 105247, // this periodic dummy aura should be trigger for casting spell below to correct positions 
                                            //-> https://www.youtube.com/watch?v=DF-ALycXVww 1:42
    SPELL_FIREWALL_COSMETIC = 105250,

    SPELL_SUMMON_FIREWALL_DUMMY = 105243,
    SPELL_CRUSHING_LEAP_JUMP = 108474,
    SPELL_CRUSHING_LEAP_DAMAGE = 108479,
    SPELL_STRIKE_FEAR = 103913,

    SPELL_INFINITE_MANA = 107880,
    SPELL_ARCANE_ANNIHILATION = 107865,

    SPELL_CARRION_SWARM = 107840, // frontal cone AoE
    SPELL_DEMONIC_WARDING = 107900, // absorb aura

    SPELL_DEMON_GRIP = 105720,
};

enum IntroData
{
    DATA_CRYSTAL_DESTROYED = 4000,
    DATA_SET_GUARDIAN_WAVE,
    DATA_GET_GUARDIAN_WAVE
};

enum Actions
{
    ACTION_PORTAL_CLOSING,
};

enum CrystalPosition
{
    FIRST_CRYSTAL = 0,
    SECOND_CRSYTAL = 1,
    THIRD_CRYSTAL = 2,
    MAX_CRYSTALS
};

// PORTAL DEMONS STUFF

enum DemonAuras // -> not used actually
{
    DEMON_AURA_LEFT = 102176,
    DEMON_AURA_RIGHT = 102146,
    DEMON_AURA_FORWARD = 102250,
};

enum DemonData
{
    DEMON_DATA_DIRECTION,
    DEMON_DATA_WAVE
};

enum DemonDirection
{
    DIRECTION_LEFT = 0,
    DIRECTION_RIGHT,
    DIRECTION_STRAIGHT
};

enum DemonWave
{
    WAVE_ONE = 0,
    WAVE_TWO = 1,
    WAVE_THREE = 2
};

enum WayPointStep
{
    WP_MID = 0,
    WP_END
};

static const Position legionDemonSpawnPositions[2] =
{
    // These are correct !!! , but i will use temporary points -> at stairs
    //{3442.0f,-5067.8f, 213.6f, 2.12f},     //LEFT
    //{3448.4f,-5065.2f, 213.6f, 2.12f},     //RIGHT

    {3373.7f,-4957.7f, 181.1f, 2.12f},     //LEFT
    {3379.2f,-4954.7f, 181.1f, 2.12f},     //RIGHT
};

static const Position midPositions[2] =
{
    {3334.4f,-4896.5f, 181.1f, 2.12f},     //LEFT
    {3341.1f,-4892.4f, 181.1f, 2.12f},     //RIGHT
};

static const Position midPositions2[2] =
{
    {3329.98f,-4892.0f, 181.1f, 2.12f},     //LEFT
    {3337.96f,-4886.74f, 181.1f, 2.12f},    //RIGHT
};

static const Position frontEndPositions[2] =
{
    {3286.0f,-4821.0f, 181.5f, 2.12f},     //LEFT
    {3290.0f,-4818.0f, 181.5f, 2.12f},     //RIGHT
};

#define PORTAL_Z (COURTYARD_Z + 0.7f)

static const Position portalPostions[3] =
{
    {3254.0f,-4943.0f, PORTAL_Z, 0.0f}, // first
    {3416.0f,-4841.0f, PORTAL_Z, 0.0f}, // second
    {3286.0f,-4814.0f, PORTAL_Z, 0.0f}  // third
};

struct guardianWPPositions
{
    Position first;
    Position last;
};

static const guardianWPPositions guardPos [MAX_CRYSTALS] =
{
    {{3320.52f,-4900.9f,COURTYARD_Z,4.03f}, {3258.66f,-4941.16f,PORTAL_Z,3.7f} }, // WAVE_ONE -> left
    {{3358.01f,-4877.5f,COURTYARD_Z,0.57f}, {3412.28f,-4842.74f,PORTAL_Z,0.5f} }, // WAVE_TWO -> right
    {{3305.9f,-4845.24f,COURTYARD_Z,2.52f}, {3289.08f,-4819.37f,PORTAL_Z,2.18f} }, // WAVE_THREE -> straight
};

enum miscData
{
    DATA_SET_WAVE_NUMBER,
};

#define MAX_ENCOUNTER      (3)

struct CONNECTOR_INFO
{
    uint64 guid;
    uint32 entry;

    CONNECTOR_INFO(uint64 guid, uint32 entry)
    {
        this->guid = guid;
        this->entry = entry;
    }
};

class instance_well_of_eternity : public InstanceMapScript
{
public:
    instance_well_of_eternity() : InstanceMapScript("instance_well_of_eternity", 939) { }

    struct instance_well_of_eternity_InstanceMapScript : public InstanceScript
    {
        instance_well_of_eternity_InstanceMapScript(Map *pMap) : InstanceScript(pMap) { Initialize(); }

        // Timers
        uint32 legionTimer;
        DemonWave waveCounter;

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        uint32 currEnc[MAX_ENCOUNTER];

        // Creature GUIDS
        uint64 perotharnGUID;
        uint64 queenGUID;
        uint64 mannorothGUID;
        uint64 varothenGUID;

        uint32 crystalsRemaining;

        std::string saveData;
        std::list<CONNECTOR_INFO> connectors;
        std::list<uint64> guardians;
        std::list<uint64> crystalGUIDS;

        uint32 crystals[MAX_CRYSTALS];

        void Initialize();
        void Update(uint32 diff);
        std::string GetSaveData();
        void Load(const char* chrIn);
        void OnCreatureCreate(Creature* pCreature, bool add);
        void OnGameObjectCreate(GameObject* go, bool add);
        uint32 GetData(uint32 DataId);
        void SetData(uint32 type, uint32 data);
        uint64 GetData64(uint32 type);
        void SetData64(uint32 identifier, uint64 data);
        virtual uint32* GetCorrUiEncounter();

        // Custom methods
        void TurnOffConnectors(uint32 connEntry, Creature * source);
        void CrystalDestroyed(uint32 crystalXCoord);
        bool IsPortalClosing(DemonDirection dir);
        Creature * GetGuardianByDirection(uint32 wave, Creature * source);

        private:
            void SummonAndInitDemon(Creature * summoner, Position summonPos, DemonDirection dir, DemonWave wave, Position movePos, WayPointStep wpID);
    };

    InstanceScript* GetInstanceScript(InstanceMap* pMap) const
    {
        return new instance_well_of_eternity_InstanceMapScript(pMap);
    }
};

#endif