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

// First demons Z coordinate
#define START_Z_COORD 189.53f

// Peroth'arn home position
#define COURTYARD_X 3339.75f
#define COURTYARD_Y -4897.18f
#define COURTYARD_Z 181.08f

// Coordinates of first escape waypoint for Perotharn and demons/arcanist (INTRO)
#define ESCAPE_X 3192.74f
#define ESCAPE_Y -4903.62f
#define ESCAPE_Z 194.35f

// These three enmus taken from https://github.com/WoWSource/WoWSource434/blob/master/src/server/scripts/Kalimdor/CavernsOfTime/WellOfEternity/well_of_eternity.h
// TODO: Make research and implement this

enum GameObjectIds
{
    GO_COURTYARD_DOOR_1         = 210084,
    GO_LARGE_FIREWALL_DOOR      = 210234,
    GO_SMALL_FIREWALL_DOOR      = 210130,
    GO_INVISIBLE_FIREWALL_DOOR  = 210097,
};

enum QuestIds
{
    QUEST_DOCUMENTING_THE_TIMEWAYS      = 30104,
    QUEST_IN_UNENDING_NUMBERS           = 30099,
    QUEST_THE_VAINGLORIOUS              = 30100,
    QUEST_THE_PATH_TO_THE_DRAGON_SOUL   = 30101,
};

enum QuestSpells
{
    SPELL_ARCHIVED_DEMON_1      = 109265,
    SPELL_ARCHIVED_DEMON_2      = 109266,
    SPELL_ARCHIVED_HANDMAIDEN_1 = 109270,
    SPELL_ARCHIVED_HANDMAIDEN_2 = 109271,
    SPELL_ARCHIVED_VAROTHEN_1   = 109273,
    SPELL_ARCHIVED_VAROTHEN_2   = 109274,
};


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
    WORLD_STATE_DEMONS_IN_COURTYARD = 6018,
    WORLD_STATE_DEMONS_IN_COURTYARD_COUNTER = 6017
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

    // Vehicle stuff creatures
    ILLIDAN_SHADOWCLOAK_VEHICLE_ENTRY = 56389,
    PLAYER_SHADOWCLOAK_VEHICLE_ENTRY = 55154
};

enum mannorothEntries
{
    // Friendly trio
    ENTRY_ILLIDAN_PRELUDE = 55532,
    ENTRY_MALFURION_PRELUDE = 55570,
    ENTRY_TYRANDE_PRELUDE = 55524,
    ENTRY_NOZDORMU_PRELUDE = 55624, // TODO : same entry as in entrance ? :)
    // Badasses
    ENTRY_DOOMGUARD_ANNIHILATOR = 55519,
    ENTRY_ABYSSAL_DOOMBRINGER = 55510,
    ENTRY_HIGHGUARD_ELITE = 55426,
    ENTRY_SHADOWBAT_HIGHGUARD = 55453,

    // Encounter mobs
    ENTRY_DDREADLORD_DEBILITATOR = 55762,
    ENTRY_DOOMGUARD_ANNIHILATOR_SUMMON = 55700
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

    SPELL_CRYSTAL_MELTDOWN = 105074, // intense green light
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

    SPELL_DEMON_GRIP = 102561,
    SPELL_DEMON_GRIP_ROOT = 102937,
    SPELL_INCINERATE = 102573
};

enum IntroData
{
    DATA_CRYSTAL_DESTROYED = 4000,
    DATA_SET_GUARDIAN_LANE,
    DATA_GET_GUARDIAN_LANE,
    DATA_GET_PEROTHARN_PHASE,
    DATA_ILLIDAN // for guid
};

enum Actions
{
    ACTION_PORTAL_CLOSING,
    ACTION_PORTAL_HURRY,
    ACTION_ILLIDAN_DELAY_MOVE, // force illidan to move to next WP point after few seconds
    ACTION_ON_ALL_DEFFENDERS_KILLED, // if all pack of deffedners was killed
    ACTION_PEROTHRAN_PREPARE_TO_AGGRO,
    ACTION_HIDE_AND_SEKK_END,
    ACTION_ILLIDAN_CREATE_VEHICLE,
    ACTION_ILLIDAN_REMOVE_VEHICLE,
    ACTION_AFTER_PEROTHARN_DEATH,
    ACTION_ILLIDAN_PRELUDE_EVENT_START,
    ACTION_ILLIDAN_PRELUDE_EVENT_CONTINUE,
    ACTION_ILLIDAN_PRECOMBAT_QUOTES_START,
    ACTION_ILLIDAN_HIGHGUARD_DIED,
    ACTION_TYRANDE_MOVE_TO_VAROTHEN,
    ACTION_MANNOROTH_ENCOUNTER_FAILED,
    ACTION_MANNOROTH_FIGHT_ENDED,
    ACTION_TYRANDE_PREPARE_WRATH_OF_ELUNE
};

enum SpecialActions
{
    SPECIAL_ACTION_NONE,
    SPECIAL_ACTION_MOVE_TO_DEFFENDERS,
    SPECIAL_ACTION_WAIT_INTRO,
};

enum IllidanSteps
{
    ILLIDAN_FIRST_STEP = 0,
    ILLIDAN_FIRST_LOOK_STEP,
    ILLIDAN_DISTRACT_STEP,
    ILLIDAN_FIRST_PACK_STEP,
    ILLIDAN_STAIRS_1_STEP,
    ILLIDAN_STAIRS_2_STEP,
    ILLIDAN_SECOND_PACK_STEP,
    ILLIDAN_WAIT_SECOND_PORTAL_STEP,
    ILLIDAN_BEFORE_STAIRS_STEP,
    ILLIDAN_UPSTAIRS_STEP,
    ILLIDAN_THIRD_PACK_STEP,
    ILLIDAN_BACK_STEP_1,
    ILLIDAN_BACK_STEP_2,
    ILLIDAN_TOO_EASY_STEP,
    ILLIDAN_PEROTHARN_STEP,
    ILLIDAN_MAX_STEPS,

    ILLIDAN_ATTACK_START_WP,
    ILLIDAN_ATTACK_END_WP,
    ILLIDAN_STEP_NULL,
};

static const Position illidanPos[ILLIDAN_MAX_STEPS] =
{
    { 3187.0f, -4891.0f, 194.36f, 5.08f },      // ILLIDAN_FIRST_STEP
    { 3246.0f, -4901.0f, COURTYARD_Z, 6.22f },      // ILLIDAN_FIRST_LOOK_STEP
    { 3297.0f, -4903.0f, COURTYARD_Z, 5.25f },  // ILLIDAN_DISTRACT_STEP
    { 3298.0f, -4959.0f, COURTYARD_Z, 4.50f },  // ILLIDAN_FIRST_PACK_STEP
    { 3380.0f, -4983.0f, COURTYARD_Z, 0.50f },  // ILLIDAN_STAIRS_1_STEP
    { 3398.0f, -4972.0f, COURTYARD_Z, 1.24f },  // ILLIDAN_STAIRS_2_STEP
    { 3425.0f, -4905.0f, COURTYARD_Z, 0.83f },  // ILLIDAN_SECOND_PACK_STEP
    { 3402.0f, -4884.0f, COURTYARD_Z, 2.10f },  // ILLIDAN_WAIT_SECOND_PORTAL_STEP
    { 3377.0f, -4816.0f, COURTYARD_Z, 1.07f },  // ILLIDAN_BEFORE_STAIRS_STEP
    { 3418.0f, -4765.0f, 194.15f, 5.61f },      // ILLIDAN_UPSTAIRS_STEP
    { 3458.0f, -4828.0f, 194.19f, 5.12f },      // ILLIDAN_THIRD_PACK_STEP
    { 3418.0f, -4765.0f, 194.15f, 5.61f },      // ILLIDAN_BACK_STEP_1
    { 3377.0f, -4816.0f, COURTYARD_Z, 1.07f },  // ILLIDAN_BACK_STEP_2
    { 3350.0f, -4866.0f, 181.29f, 3.92f },      // ILLIDAN_TOO_EASY_STEP
    { 3332.0f, -4884.0f, COURTYARD_Z, 5.40f }   // ILLIDAN_PEROTHARN_STEP
};

enum afterPerotharnDeathWP
{
    ILLIDAN_PEROTHARN_DEFEATED_STAIRS_WP = ILLIDAN_STEP_NULL + 1,
    ILLIDAN_HANDRAIL_WP,
    ILLIDAN_PALACE_JUMP_WP,
    ILLIDAN_RUNAWAY_WP
};

static const Position afterPerotharnDeathWP[4] =
{
    { 3373.0f, -4950.0f, COURTYARD_Z, 2.20f },  // ILLIDAN_PEROTHARN_DEFEATED_STEP
    { 3450.0f, -5080.0f, 213.6f,      3.70f },  // ILLIDAN_GOODBYE_STEP
    { 3418.0f, -5108.0f, COURTYARD_Z, 4.11f },  // ILLIDAN_PALACE_JUMP_STEP
    { 3402.0f, -5169.0f, COURTYARD_Z, 4.90f }   // ILLIDAN_RUNAWAY_STEP
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
    {3443.17f,-5067.00f, 213.6f, 2.12f},     //LEFT
    {3450.60f,-5064.36f, 213.6f, 2.12f},     //RIGHT
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

struct SimpleQuote
{
    uint32 soundID;
    const char * text;
};

typedef struct quote_event
{
    const uint32 nextEventTime;
    const uint32 entry;
    const char * yellQuote;
    const uint32 soundID;
}QUOTE_EVENTS;

enum miscData
{
    DATA_SET_WAVE_NUMBER,
    DATA_PORTAL_TO_TWISTING_NETHER = 56087
};

enum drakeActions
{
    ACTION_SPAWN_DRAKE_VEHICLES,
    ACTION_BRONZE_DRAKE_ARRIVED,
    ACTION_VEHICLE_MOVE
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

#define NIGH_ELF_ILLUSION_MALE 108465 
#define NIGH_ELF_ILLUSION_FEMALE 108466

// Path from forest to Mannoroth (shore path)
const Position pathStartPos = {3173.0f, -5595.0f, 17.65f,5.08f};
const Position pathMiddlePos = {3210.0f, -5678.0f, 18.02f,5.5f};

//Before abbysal
const Position illidanAbyssalPos = {3222.84f, -5680.7f, 18.7f,5.76f};
const Position tyrandeAbyssalPos = {3219.0f, -5688.0f, 18.1f,5.8f};
//Before highguards
const Position illidanHighGuardPos = {3250.0f, -5695.0f, 18.28f,6.12f};
const Position tyrandeHighGuardPos = {3249.0f, -5702.0f, 17.72f,6.12f};
//Before Varothen
const Position illidanVarothenPos = {3278.0f, -5705.0f, 16.35f,6.1f};
const Position tyrandeVarothenPos = {3284.0f, -5693.0f, 15.37f,6.1f};
// Tyrande combat position
const Position tyrandeCombatPos = {3308.0f, -5695.0f, 15.5f,5.6f};

class instance_well_of_eternity : public InstanceMapScript
{
public:
    instance_well_of_eternity() : InstanceMapScript("instance_well_of_eternity", 939) {}

    struct instance_well_of_eternity_InstanceMapScript : public InstanceScript, public MapScript
    {
        instance_well_of_eternity_InstanceMapScript(Map *pMap) : InstanceScript(pMap), MapScript(939) { Initialize(); }

        private:
        uint32 legionTimer;
        DemonWave waveCounter;

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        uint32 currEnc[MAX_ENCOUNTER];

        // Creature GUIDS
        uint64 illidanGUID;
        uint64 perotharnGUID;
        uint64 queenGUID;
        uint64 mannorothGUID;
        uint64 varothenGUID;

        uint64 illidanPreludeGUID;
        uint64 tyrandePreludeGUID;

        uint64 portalToTwistingNetherGUID;

        uint32 deffendersKilled;

        std::string saveData;
        // Perotharn intro
        std::list<CONNECTOR_INFO> connectors;
        std::list<uint64> guardians;
        std::list<uint64> crystalGUIDS;
        std::list<uint64> illidanVictims;
        // Mannoroth intro
        std::list<uint64> doomguards;

        uint32 crystals[MAX_CRYSTALS];
        IllidanSteps illidanStep;

        bool IsIlidanCurrentWPReached(uint64 victimGUID);
        void SummonAndInitDemon(Creature * summoner, Position summonPos, DemonDirection dir, DemonWave wave, Position movePos, WayPointStep wpID);
        void Initialize();
        void Update(uint32 diff);
        std::string GetSaveData();
        void Load(const char* chrIn);
        void OnCreatureCreate(Creature* pCreature, bool add);
        void OnGameObjectCreate(GameObject* go, bool add);
        virtual uint32* GetCorrUiEncounter();

        // Only this works ?
        void OnPlayerEnter(Player * p) override
        {
            if (p->getGender() == GENDER_MALE)
                p->CastSpell(p, NIGH_ELF_ILLUSION_MALE, true);
            else
                p->CastSpell(p, NIGH_ELF_ILLUSION_FEMALE, true);
        }

        public:
            //  Fuck it -> public
            uint32 crystalsRemaining;
            bool demonPulseDone;
            bool abyssalSlained;

            uint32 GetData(uint32 DataId);
            void SetData(uint32 type, uint32 data);
            uint64 GetData64(uint32 type);

            // Custom WoE methods
            void RegisterIllidanVictim(uint64 victimGUID);
            void UnRegisterIllidanVictim(uint64 victimGUID);
            Creature * GetIllidanVictim();
            void OnDeffenderDeath(Creature * deffender);
            void SetIllidanStep(IllidanSteps step) { this->illidanStep = step; }
            IllidanSteps GetIllidanMoveStep() { return illidanStep; }
            void TurnOffConnectors(uint32 connEntry, Creature * source);
            void CrystalDestroyed(uint32 crystalXCoord);
            bool IsPortalClosing(DemonDirection dir);
            Creature * GetGuardianByDirection(uint32 direction);

            // Mannoroth stuff
            void CallDoomGuardsForHelp(Unit * victim);
            bool PlayersWipedOnMannoroth();
    };

    InstanceScript* GetInstanceScript(InstanceMap* pMap) const
    {
        return new instance_well_of_eternity_InstanceMapScript(pMap);
    }
};

#endif