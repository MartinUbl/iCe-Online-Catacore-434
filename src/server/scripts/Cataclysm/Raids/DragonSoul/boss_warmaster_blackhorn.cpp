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

/*
Encounter: Warmaster Blackhorn
Dungeon: Dragon Soul
Difficulty: Normal / Heroic
Mode: 10-man normal/ 10-man heroic
Autor: Lazik
*/

#include "ScriptPCH.h"
#include "dragonsoul.h"
#include "MapManager.h"
#include "TaskScheduler.h"

#define SEARCH_RANGE        300.0f

// NPCs
enum NPC
{
    BOSS_WARMASTER_BLACKHORN            = 56427,
    NPC_GORIONA                         = 56781,
    NPC_TWILIGHT_ASSAULT_DRAKE_LEFT     = 56855,
    NPC_TWILIGHT_ASSAULT_DRAKE_RIGHT    = 56587,
    NPC_TWILIGHT_ELITE_DREADBLADE       = 56854,
    NPC_TWILIGHT_ELITE_SLAYER           = 56848,
    NPC_TWILIGHT_INFILTRATOR            = 56922,
    NPC_TWILIGHT_SAPPER                 = 56923,
    NPC_SKYFIRE                         = 56598,
    NPC_TWILIGHT_FLAMES                 = 57268,
    NPC_MASSIVE_EXPLOSION               = 57297,
    NPC_GUNSHIP_PURSUIT_CONTROLLER      = 56599,
    NPC_BROADSIDE_TARGET                = 119559,
    NPC_SHOCKWAVE_TARGET                = 119560,
    NPC_ONSLAUGHT_TARGET                = 57238,
    NPC_ENGINE_STALKER                  = 57190,
    NPC_FIRE_STALKER                    = 57852,
    NPC_HARPOON                         = 56681,
    NPC_SKYFIRE_CANNON                  = 57260,
    NPC_SKYFIRE_COMMANDO                = 57264,
};

// Spells
enum Spells
{
    // Warmaster Blackhorn
    SPELL_BERSERK                       = 26662,
    SPELL_VENGEANCE                     = 108045,
    SPELL_DEVASTATE                     = 108042,
    SPELL_DISRUPTING_ROAR               = 108044,
    SPELL_SHOCKWAVE_AOE                 = 110137,
    SPELL_SHOCKWAVE                     = 108046,
    SPELL_SIPHON_VITALITY               = 110312,
    SPELL_SIPHON_VITALITY_VISUAL        = 110315,

    // Goriona
    SPELL_TWILIGHT_ONSLAUGHT_DUMMY      = 107927,
    SPELL_TWILIGHT_ONSLAUGHT_DUMMY_2    = 107586,
    SPELL_TWILIGHT_ONSLAUGHT            = 107588,
    SPELL_TWILIGHT_ONSLAUGHT_DMG        = 106401,
    SPELL_TWILIGHT_ONSLAUGHT_DMG_2      = 107589,
    SPELL_BROADSIDE_AOE                 = 110153,
    SPELL_BROADSIDE_DMG                 = 110157,
    SPELL_TWILIGHT_BREATH               = 110212,
    SPELL_CONSUMING_SHROUD              = 110214,
    SPELL_CONSUMING_SHROUD_DMG          = 110215,
    SPELL_TWILIGHT_FLAMES_MISSILE       = 108050,
    SPELL_TWILIGHT_FLAMES               = 108051,
    SPELL_TWILIGHT_FLAMES_AURA          = 108053,
    SPELL_TWILIGHT_BLAST                = 107583,
    SPELL_FORCE_FLYING                  = 74426, // For some reason she's not flying, this helps

    // Skifire
    SPELL_ENGINE_FIRE                   = 107799,
    SPELL_DECK_FIRE_PERSISTENT_AURA     = 109445,
    SPELL_DECK_FIRE_DMG                 = 110095,
    SPELL_DECK_FIRE_PERIODIC            = 110092, // Periodically trigger damage 
    SPELL_DECK_FIRE_SPAWN               = 109470, // Explosion and fire spread
    SPELL_SHIP_FIRE_VISUAL              = 109245,
    SPELL_HEAVY_SLUG                    = 108010,
    SPELL_ARTILLERY_BARRAGE             = 108040,
    SPELL_ARTILLERY_BARRAGE_DMG         = 108041,
    SPELL_RELOADING                     = 108039,
    SPELL_RIDE_VEHICLE                  = 43671, // commandos on harpoon guns and cannons

    // Twilight Assault Drake
    SPELL_TWILIGHT_BARRAGE              = 107286,
    SPELL_HARPOON                       = 108038,

    // Achievements
    ACHIEVEMENT_HEROIC_WARMASTER        = 6114,
    ACHIEVEMENT_DECK_DEFENDER           = 6105,
};

enum EncounterActions
{
    ACTION_START_ENCOUNTER              = 0,
    ACTION_GORIONA_LEAVES_SCENE         = 1,
    ACTION_WARMASTER_JUMPS_ON_BOARD     = 2,
    ACTION_ACHIEVEMENT_FAILED           = 3,
};

enum Phase
{
    PHASE_INTRO                         = 0,
    PHASE_GORIONA_AIR_ATTACK            = 1,
    PHASE_WARMASTER_ON_BOARD            = 2,
    PHASE_GORIONA_LANDS_ON_BOARD        = 3,
};

enum Engines
{
    RIGHT_BACK_ENGINE                   = 0,
    RIGHT_FRONT_ENGINE                  = 1,
    LEFT_FRONT_ENGINE                   = 2,
    LEFT_BACK_ENGINE                    = 3,
    MAX_ENGINES                         = 4,
};

const uint8 MAX_HARPOONS      = 2;

const Position harpoonPos[MAX_HARPOONS] =
{
    { 13430.10f, -12161.81f, 154.11f, 1.51f }, // Left harpoon drake position
    { 13432.68f, -12103.32f, 154.11f, 4.66f }, // Right harpoon drake position
};

enum drakeSide
{
    LEFT_HARPOON_POSITION           = 0,
    RIGHT_HARPOON_POSITION          = 1,
};

const Position skyfireEnginePositions[MAX_ENGINES] =
{
    { 13481.2f, -12094.4f, 143.8f, 3.07f }, // Right Back
    { 13397.0f, -12099.2f, 146.7f, 3.07f }, // Right Front
    { 13394.4f, -12162.5f, 146.4f, 3.08f }, // Left Front
    { 13477.4f, -12174.7f, 144.3f, 3.10f }, // Left Back
};

struct PlayableQuote warmasterIntro { 26214, "Hah! I was hoping you'd make it this far. You'd best be ready for a real fight." };

#define MAX_WARMASTER_KILL_QUOTES 4

struct PlayableQuote warmasterKilledUnit[MAX_WARMASTER_KILL_QUOTES]
{
    { 26215, "Ha, ha, ha... mess with the bull!" },
    { 26216, "HA-HA-HA-HA!" },
    { 26217, "Down you go!" },
    { 26218, "Get up! Oh, weakling..." },
};

#define MAX_WARMASTER_SHOCKWAVE_QUOTES 2

struct PlayableQuote warmasterShockwave[MAX_WARMASTER_SHOCKWAVE_QUOTES]
{
    { 26221, "How's THIS?" },
    { 26222, "Come on!" },
};

struct PlayableQuote warmasterCombat { 26210, "You won't get near the Master. Dragonriders, attack!" };

struct PlayableQuote warmasterDeath { 26211, "Well... done, heh. But I wonder if you're good enough... to best him." };

struct PlayableQuote warmasterErage { 26213, "We're flying a little too close. It's been a good fight, but I'm ending it, now." };

struct PlayableQuote warmasterOnBoardPhase { 26212, "Looks like I'm doing this myself. Good!" };

struct PlayableQuote warmasterGorionaOnslaught { 26219, "Goriona! Give them hell!" };

struct PlayableQuote captainSwayze { 26295, "Concentrate everything on the armored drake!" };

// Warmaster Blackhorn
class boss_warmaster_blackhorn : public CreatureScript
{
public:
    boss_warmaster_blackhorn() : CreatureScript("boss_warmaster_blackhorn") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_warmaster_blackhornAI(pCreature);
    }

    struct boss_warmaster_blackhornAI : public ScriptedAI
    {
        boss_warmaster_blackhornAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        TaskScheduler scheduler;
        uint32 disruptingRoarTimer;
        uint32 shockwaveTimer;
        uint32 devastateTimer;
        uint32 vengeanceTimer;
        uint32 berserkTimer;
        uint32 allowTurningTimer;
        uint32 phase;
        bool siphonVitality;
        bool berserk;
        bool achievement;

        void Reset() override
        {
            if (instance)
            {
                if (instance->GetData(TYPE_BOSS_BLACKHORN) != DONE)
                    instance->SetData(TYPE_BOSS_BLACKHORN, NOT_STARTED);
            }

            me->CastSpell(me, SPELL_VENGEANCE, false);
            me->SetVisible(false);
            me->SetReactState(REACT_PASSIVE);
            phase = PHASE_GORIONA_AIR_ATTACK;

            disruptingRoarTimer = 20000;
            shockwaveTimer = 25000;
            devastateTimer = 8000;
            vengeanceTimer = 5000;
            allowTurningTimer = 0;
            berserkTimer = 0;
            achievement = true;
            berserk = false;
            siphonVitality = false;

            if (Creature* pGoriona = me->FindNearestCreature(NPC_GORIONA, SEARCH_RANGE))
            {
                if (!pGoriona->IsAlive())
                    pGoriona->Respawn(true);
                pGoriona->AI()->Reset();
            }

            if (Creature* pCaptainSwayze = me->FindNearestCreature(NPC_SKY_CAPTAIN_SWAYZE, SEARCH_RANGE))
                pCaptainSwayze->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

            ScriptedAI::Reset();
            DespawnFireStalkers();
        }

        void EnterCombat(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_BLACKHORN, IN_PROGRESS);
            }
        }

        void KilledUnit(Unit* victim) override 
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
            {
                uint32 randYell = urand(0, MAX_WARMASTER_KILL_QUOTES - 1);
                RunPlayableQuote(warmasterKilledUnit[randYell]);
            }
        }

        void JustDied(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_BLACKHORN, DONE);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

                if (Creature* pShip = me->FindNearestCreature(NPC_SKYFIRE, SEARCH_RANGE))
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, pShip);

                if (IsHeroic())
                    instance->DoCompleteAchievement(ACHIEVEMENT_HEROIC_WARMASTER);
                if (achievement)
                    instance->DoCompleteAchievement(ACHIEVEMENT_DECK_DEFENDER);

                instance->DoModifyPlayerCurrencies(CURRENCY_MOTE_OF_DARKNESS, 1, CURRENCY_SOURCE_OTHER);
            }

            if (Creature * pGoriona = me->FindNearestCreature(NPC_GORIONA, SEARCH_RANGE, true))
                pGoriona->DespawnOrUnsummon();

            if (Creature* pCaptainSwayze = me->FindNearestCreature(NPC_SKY_CAPTAIN_SWAYZE, SEARCH_RANGE))
                pCaptainSwayze->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

            RunPlayableQuote(warmasterDeath);
        }

        void DespawnFireStalkers()
        {
            scheduler.Schedule(Seconds(10), [this](TaskContext firestalkerDespawn)
            {
                if (Creature * pFireStalker = me->FindNearestCreature(NPC_FIRE_STALKER, SEARCH_RANGE, true))
                    pFireStalker->DespawnOrUnsummon();
                if (firestalkerDespawn.GetRepeatCounter() < 20)
                    firestalkerDespawn.Repeat(Seconds(2));
            });
        }

        void DoAction(const int32 action) override
        {
            if (action == ACTION_WARMASTER_JUMPS_ON_BOARD)
            {
                // Ship won`t take any other damage when Warmaster jumps on board
                if (Creature* pShip = me->FindNearestCreature(NPC_SKYFIRE, SEARCH_RANGE))
                    pShip->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                phase = PHASE_WARMASTER_ON_BOARD;
                allowTurningTimer = 60000;
                me->SetReactState(REACT_AGGRESSIVE);
                RunPlayableQuote(warmasterOnBoardPhase);
                berserkTimer = 240 * IN_MILLISECONDS;
                berserk = true;
                DespawnFireStalkers();
            }
            else if (action == ACTION_ACHIEVEMENT_FAILED)
            {
                achievement = false;
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (!UpdateVictim())
                return;

            if (phase == PHASE_WARMASTER_ON_BOARD)
            {
                // Disrupting Roar
                if (disruptingRoarTimer <= diff)
                {
                    me->CastSpell(me, SPELL_DISRUPTING_ROAR, false);
                    disruptingRoarTimer = 20000;
                }
                else disruptingRoarTimer -= diff;

                // Shockwave
                if (shockwaveTimer <= diff)
                {
                    if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, SEARCH_RANGE, true))
                    {
                        if (Creature * pShockwaveTarget = me->SummonCreature(NPC_SHOCKWAVE_TARGET, pTarget->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 3000))
                        {
                            pShockwaveTarget->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE|UNIT_FLAG_NOT_SELECTABLE);
                            me->SetFacingTo(me->GetAngle(pShockwaveTarget));
                            me->SetUInt64Value(UNIT_FIELD_TARGET, pShockwaveTarget->GetGUID());
                            me->CastSpell(pShockwaveTarget, SPELL_SHOCKWAVE, false);
                        }
                    }
                    allowTurningTimer = 3500;

                    uint32 randQuote = urand(0, MAX_WARMASTER_SHOCKWAVE_QUOTES - 1);
                    RunPlayableQuote(warmasterShockwave[randQuote]);
                    shockwaveTimer = 25000;
                }
                else shockwaveTimer -= diff;

                // Devastate
                if (devastateTimer <= diff)
                {
                    me->CastSpell(me->GetVictim(), SPELL_DEVASTATE, false);
                    devastateTimer = 8000;
                }
                else devastateTimer -= diff;

                // Berserk
                if (berserk == true)
                {
                    if (berserkTimer <= diff)
                    {
                        berserk = false;
                        RunPlayableQuote(warmasterErage);
                        me->CastSpell(me, SPELL_BERSERK, false);
                    }
                    else berserkTimer -= diff;
                }

                // Heroic Ability - Spihon Vitality
                if (IsHeroic() && me->HealthBelowPct(20))
                {
                    if (!siphonVitality)
                    {
                        if (Creature* pGoriona = me->FindNearestCreature(NPC_GORIONA, SEARCH_RANGE))
                        {
                            pGoriona->CastSpell(me, SPELL_SIPHON_VITALITY_VISUAL, true);
                            int32 bp0 = (pGoriona->GetHealth() * 0.2); // 20% of her remaining health
                            me->CastCustomSpell(pGoriona, SPELL_SIPHON_VITALITY, &bp0, 0, 0, true);
                            me->DealDamage(pGoriona, bp0);
                        }
                        siphonVitality = true;
                    }
                }

                // Change target after Shockwave - Don`t even ask :D
                if (allowTurningTimer <= diff) // Put again our victim to  UNIT_FIELD_TARGET
                {
                    if (me->GetVictim() && me->GetVictim()->IsInWorld())
                        me->SetUInt64Value(UNIT_FIELD_TARGET, me->GetVictim()->GetGUID());
                    allowTurningTimer = 25000;
                }
                else allowTurningTimer -= diff;

                DoMeleeAttackIfReady();
            }
        }
    };
};

enum GorionaMoves
{
    MOVE_LEFT_POSITION              = 0,
    MOVE_RIGHT_POSITION             = 1,
};


const uint32 EMPTY_SPOT = 0;
const uint32 FULL_SPOT = 1;

const uint32 MAX_WARMASTER_DAMAGE_POSITIONS = 6;

const Position warmasterDamagePos[MAX_WARMASTER_DAMAGE_POSITIONS] =
{
    { 13456.34f, -12139.04f, 151.17f, 4.67f },
    { 13442.35f, -12138.85f, 150.82f, 4.70f },
    { 13427.95f, -12138.62f, 150.87f, 4.71f },
    { 13456.39f, -12127.44f, 151.17f, 4.67f },
    { 13442.41f, -12127.45f, 150.82f, 4.65f },
    { 13428.14f, -12127.19f, 150.87f, 4.70f },
};

enum GorionaDrakesPosition
{
    LEFT_DRAKE_SPAWN_POS                = 0,
    RIGHT_DRAKE_SPAWN_POS               = 1,
    LEFT_DRAKE_DROP_POS                 = 2,
    RIGHT_DRAKE_DROP_POS                = 3,
    LEFT_DRAKE_END_FLY_POS              = 4,
    RIGHT_DRAKE_END_FLY_POS             = 5,
};

#define MAX_ASSAULT_DRAKE_POSITIONS       6

const Position assaultDrakePos[MAX_ASSAULT_DRAKE_POSITIONS] =
{
    { 13441.83f, -12184.15f, 172.05f, 1.49f }, // Left Spawn Drake Pos
    { 13447.38f, -12083.55f, 172.05f, 4.45f }, // Right Spawn Drake Pos
    { 13431.26f, -12125.28f, 172.05f, 3.08f }, // Left drake drop add position
    { 13430.35f, -12140.22f, 172.05f, 3.08f }, // Right drake drop add position
    { 13433.40f, -12082.54f, 172.05f, 4.65f }, // Left drake end fly position
    { 13429.12f, -12182.25f, 172.05f, 1.46f }, // Right drake end fly position
};

#define MAX_GORIONA_POSITIONS        5

const Position gorionaPos[MAX_GORIONA_POSITIONS] =
{
    { 13400.00f, -12087.48f, 182.50f, 3.77f }, // Second Engine
    { 13367.23f, -12128.88f, 182.50f, 6.23f }, // Front of the ship
    { 13395.59f, -12175.28f, 182.50f, 0.88f }, // Third Engine
    { 13421.95f, -11930.60f, 183.00f, 1.17f }, // Fly Away position
    { 13426.70f, -12132.45f, 151.20f, 6.21f }, // Heroic land position
};

enum GorionaPositions
{
    WP_RIGHT_FRONT_ENGINE    = 0,
    WP_GUNSHIP_FRONT         = 1,
    WP_LEFT_FRONT_ENGINE     = 2,
    WP_FLY_AWAY              = 3,
    WP_WARMASTER_TAKE_OFF    = 4,
    WP_LAND_POSITION         = 5,
    WP_GORIONA_SECOND_PHASE  = 6,
};

#define MAX_INFILTRATIOR_POSITIONS         3

const Position infiltratorPos[MAX_INFILTRATIOR_POSITIONS] =
{
    { 13352.60f, -12150.22f, 181.30f, 0.03f }, // Left side Start fly position
    { 13353.51f, -12109.58f, 181.30f, 6.10f }, // Right side Start fly position
    { 13504.39f, -12135.67f, 185.00f, 6.22f }, // End fly position
};

const Position skyfirePosition { 13500.0f, -12135.6f, 128.6f, 3.03f };

typedef struct targetSpots
{
    bool free;
    float x, y, z;
}TARGET_SPOTS;

struct OnslaughtSpot
{
    uint8 spot;
    uint8 minX, maxX;
    uint8 minY, maxY;
};

const uint32 MAX_ONSLAUGHT_POSITIONS                    = 6;

const OnslaughtSpot onslaughtSpot[MAX_ONSLAUGHT_POSITIONS]
{
    { 0, 1, 2, 2, 3 },
    { 1, 3, 4, 2, 3 },
    { 2, 5, 6, 2, 3 },
    { 3, 1, 2, 4, 5 },
    { 4, 3, 4, 4, 5 },
    { 5, 5, 6, 4, 5 },
};

const uint32 MAX_FIRE_POSITIONS = 20;

struct FireSpot
{
    uint8 spotX;
    uint8 spotY;
};

const FireSpot fireSpot[MAX_FIRE_POSITIONS]
{
    { 2, 3 },
    { 2, 2 },
    { 1, 2 },
    { 1, 3 },
    { 1, 4 },
    { 0, 2 },
    { 0, 3 },
    { 1, 1 },
    { 0, 1 },
    { 0, 4 },
    { 1, 0 },
    { 1, 5 },
    { 0, 5 },
    { 0, 0 },
    { 2, 1 },
    { 2, 0 },
    { 3, 0 },
    { 2, 4 },
    { 2, 5 },
    { 3, 5 },
};

enum Status
{
    CHECK              = 0,
    SET_FULL           = 1,
    SET_EMPTY          = 2,
};

const uint32 MAX_HORIZONTAL_LINES     = 6;
const uint32 MAX_VERTICAL_LINES       = 6;
const float START_X                   =  13460.0f;
const float START_Y                   = -12147.0f;
const float START_Z                   = 150.84f;
const float STEP_X                    = -7.0f;
const float STEP_Y                    = 5.5f;

// Goriona
class npc_ds_goriona : public CreatureScript
{
public:
    npc_ds_goriona() : CreatureScript("npc_ds_goriona") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_gorionaAI(pCreature);
    }

    struct npc_ds_gorionaAI : public ScriptedAI
    {
        npc_ds_gorionaAI(Creature *creature) : ScriptedAI(creature), Summons(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        TaskScheduler scheduler;
        SummonList Summons;

        uint64 engineGuids[MAX_ENGINES];
        uint32 twilightOnsloughtTimer;
        uint32 twilightFlamesTimer;
        uint32 twilightInfiltratorTimer;
        uint32 twilightBreathTimer;
        uint32 consumingShroudTimer;
        uint32 broadsideTimer;
        uint32 waveTimer;
        uint32 waveCount;
        uint32 phase;
        uint32 nextWP;
        bool canMoveNextPoint;
        bool nextWave;
        bool flyAway;
        bool heroicLand;
        bool spreadFire;
        TARGET_SPOTS boardSpots[MAX_VERTICAL_LINES][MAX_HORIZONTAL_LINES];

        void Reset() override
        {
            me->SetReactState(REACT_PASSIVE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlying(true);
            me->SetVisible(false);
            me->SetSpeed(MOVE_FLIGHT, 2.2f);

            twilightOnsloughtTimer = 35000;
            twilightFlamesTimer = 8000;
            twilightInfiltratorTimer = 50000;
            broadsideTimer = 40000;

            phase = PHASE_INTRO;
            waveCount = 0;
            nextWP = 0;

            canMoveNextPoint = false;
            nextWave = false;
            flyAway = false;
            heroicLand = false;
            spreadFire = true;

            if (Creature* pShip = me->FindNearestCreature(NPC_SKYFIRE, SEARCH_RANGE))
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, pShip);

            scheduler.Schedule(Seconds(5), [this](TaskContext greetings)
            {
                if (Creature * pWarmaster = me->FindNearestCreature(BOSS_WARMASTER_BLACKHORN, SEARCH_RANGE, true))
                {
                    pWarmaster->EnterVehicle(me, 0, nullptr);
                    pWarmaster->SetVisible(false);
                }
            });

            if (Creature* pCaptainSwayze = me->FindNearestCreature(NPC_SKY_CAPTAIN_SWAYZE, SEARCH_RANGE))
                pCaptainSwayze->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

            Summons.DespawnAll();
            DespawnNPC(NPC_TWILIGHT_ELITE_DREADBLADE);
            DespawnNPC(NPC_TWILIGHT_ELITE_SLAYER);
            DespawnNPC(NPC_TWILIGHT_SAPPER);

            scheduler.CancelGroup(1);
            ScriptedAI::Reset();
        }

        void SetBoardSpots()
        {
            float startX = START_X;
            float startY = START_Y;

            for (uint8 i = 0; i < MAX_VERTICAL_LINES; i++)
            {
                startY = START_Y;

                for (uint8 j = 0; j < MAX_HORIZONTAL_LINES; j++)
                {
                    boardSpots[i][j].free = true;
                    boardSpots[i][j].z = START_Z;
                    boardSpots[i][j].x = startX;
                    boardSpots[i][j].y = startY;

                    startY += STEP_Y;
                }
                startX += STEP_X;
            }
        }

        bool ValidateFreeSpot(uint32 type, uint32 spotX, uint32 spotY)
        {
            if (type == DATA_WARMASTER_BOARD_SPOT)
            {
                if (boardSpots[spotX][spotY].free == true)
                {
                    boardSpots[spotX][spotY].free = false;

                    scheduler.Schedule(Seconds(5), [this, spotX, spotY](TaskContext greetings)
                    {
                        UnlockBoardSpot(spotX, spotY);
                    });
                    return EMPTY_SPOT;
                }
            }
            return FULL_SPOT;
        }

        void UnlockBoardSpot(uint32 spotX, uint32 spotY)
        {
            boardSpots[spotX][spotY].free = true;
        }

        void SetBoardSpotFull(uint32 spotX, uint32 spotY)
        {
            boardSpots[spotX][spotY].free = false;
        }

        bool OnslaughtBoardSpotStatus(uint32 onslaughtPosition, uint32 status)
        {
            for (uint8 i = onslaughtSpot[onslaughtPosition].minX; i < onslaughtSpot[onslaughtPosition].maxX; i++)
            {
                for (uint8 j = onslaughtSpot[onslaughtPosition].minY; j < onslaughtSpot[onslaughtPosition].maxY; j++)
                {
                    if (status == CHECK)
                    {
                        if (boardSpots[i][j].free == false)
                        {
                            return FULL_SPOT;
                        }
                    }
                    else if (status == SET_EMPTY)
                        boardSpots[i][j].free = true;
                    else if (status == SET_FULL)
                        boardSpots[i][j].free = false;
                }
            }
            return EMPTY_SPOT;
        }

        void JustSummoned(Creature* summon) override
        {
            switch (summon->GetEntry())
            {
            case NPC_TWILIGHT_INFILTRATOR:
                summon->GetMotionMaster()->MovePoint(0, infiltratorPos[MAX_INFILTRATIOR_POSITIONS - 1], true);
                break;
            case NPC_TWILIGHT_ASSAULT_DRAKE_LEFT:
                if (Creature * pPassenger = summon->SummonCreature(NPC_TWILIGHT_ELITE_SLAYER, summon->GetPositionX(), summon->GetPositionY(), summon->GetPositionZ(), summon->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN))
                    pPassenger->EnterVehicle(summon, 0, nullptr);
                summon->GetMotionMaster()->MovePoint(LEFT_DRAKE_DROP_POS, assaultDrakePos[LEFT_DRAKE_DROP_POS], true);
                break;
            case NPC_TWILIGHT_ASSAULT_DRAKE_RIGHT:
                if (Creature * pPassenger = me->SummonCreature(NPC_TWILIGHT_ELITE_DREADBLADE, summon->GetPositionX(), summon->GetPositionY(), summon->GetPositionZ(), summon->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN))
                    pPassenger->EnterVehicle(summon, 0, nullptr);
                summon->GetMotionMaster()->MovePoint(RIGHT_DRAKE_DROP_POS, assaultDrakePos[RIGHT_DRAKE_DROP_POS], true);
                break;
            default:
                break;
            }

            Summons.push_back(summon->GetGUID());
        }

        void DespawnNPC(uint32 entry)
        {
            std::list<Creature*> crList;
            me->GetCreatureListWithEntryInGrid(crList, entry, SEARCH_RANGE);
            for (std::list<Creature*>::iterator itr = crList.begin(); itr != crList.end(); ++itr)
            {
                if (*itr)
                    (*itr)->DespawnOrUnsummon();
            }
        }

        void EnterCombat(Unit * /*who*/) override
        {
            phase = PHASE_GORIONA_AIR_ATTACK;
            SetBoardSpots();
        }

        void KilledUnit(Unit* victim) override {}

        void SummonNPC(uint32 entry)
        {
            switch (entry)
            {
            case NPC_TWILIGHT_ASSAULT_DRAKE_LEFT:
                me->SummonCreature(entry, assaultDrakePos[LEFT_DRAKE_SPAWN_POS], TEMPSUMMON_DEAD_DESPAWN);
                break;
            case NPC_TWILIGHT_ASSAULT_DRAKE_RIGHT:
                me->SummonCreature(entry, assaultDrakePos[RIGHT_DRAKE_SPAWN_POS], TEMPSUMMON_DEAD_DESPAWN);
                break;
            case NPC_TWILIGHT_INFILTRATOR:
                me->SummonCreature(entry, infiltratorPos[urand(MAX_INFILTRATIOR_POSITIONS - 3, MAX_INFILTRATIOR_POSITIONS - 2)], TEMPSUMMON_TIMED_DESPAWN, 8000);
                break;
            case NPC_ENGINE_STALKER:
                for (uint32 i = 0; i < MAX_ENGINES; i++)
                {
                    if (Creature * engineStalker = me->SummonCreature(NPC_ENGINE_STALKER, skyfireEnginePositions[i]))
                        engineGuids[i] = engineStalker->GetGUID();
                }
                break;
            default:
                break;
            }
        }

        void RunPlayableQuote(PlayableQuote quote, uint32 npcEntry)
        {
            if (Creature * pNpcEntry = me->FindNearestCreature(npcEntry, SEARCH_RANGE))
            {
                pNpcEntry->MonsterYell(quote.GetText(), LANG_UNIVERSAL, 0);
                pNpcEntry->SendPlaySound(quote.GetSoundId(), false);
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (type == POINT_MOTION_TYPE)
            {
                if (id == WP_RIGHT_FRONT_ENGINE || id == WP_GUNSHIP_FRONT || id == WP_LEFT_FRONT_ENGINE)
                {
                    if (Creature* pEngineStalker = Unit::GetCreature(*me, engineGuids[++id]))
                        me->CastSpell(pEngineStalker, SPELL_TWILIGHT_BLAST, true);

                    nextWP++;
                    canMoveNextPoint = true;

                    if (id == WP_LEFT_FRONT_ENGINE)
                    {
                        RunPlayableQuote(warmasterCombat, BOSS_WARMASTER_BLACKHORN);
                        me->SetInCombatWithZone();

                        // Spawn 1st wave of Assault drakes
                        SummonNPC(NPC_TWILIGHT_ASSAULT_DRAKE_LEFT);
                        SummonNPC(NPC_TWILIGHT_ASSAULT_DRAKE_RIGHT);
                        waveTimer = 60000;
                        nextWave = true;

                        me->AddAura(SPELL_FORCE_FLYING, me);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->SetOrientation(0.706f);
                    }
                }
                else if (id == WP_LAND_POSITION)
                {
                    me->GetMotionMaster()->MoveChase(me->GetVictim());
                }
                else if (id == WP_FLY_AWAY)
                {
                    me->SetVisible(false);
                }
            }
        }

        void DoAction(const int32 action) override
        {
            if (instance)
            {
                switch (action)
                {
                case ACTION_START_ENCOUNTER:
                {
                    me->SetVisible(true);

                    Creature * pShip = me->SummonCreature(NPC_SKYFIRE, skyfirePosition);
                    Creature * pWarmasterBlackhorn = me->FindNearestCreature(BOSS_WARMASTER_BLACKHORN, SEARCH_RANGE, true);
                    if (pWarmasterBlackhorn && pShip)
                    {
                        pWarmasterBlackhorn->SetVisible(true);
                        pWarmasterBlackhorn->EnterVehicle(me, 0, nullptr);
                        RunPlayableQuote(warmasterIntro, BOSS_WARMASTER_BLACKHORN);                        
                        pShip->SetInCombatWithZone();
                        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, pShip, 0, 0);
                    }
                    me->GetMotionMaster()->MovePoint(WP_RIGHT_FRONT_ENGINE, gorionaPos[WP_RIGHT_FRONT_ENGINE], true);

                    SummonNPC(NPC_ENGINE_STALKER);
                    if (Creature* pEngineStalker = Unit::GetCreature(*me, engineGuids[nextWP]))
                        me->CastSpell(pEngineStalker, SPELL_TWILIGHT_BLAST, true);
                    break;
                }
                case ACTION_GORIONA_LEAVES_SCENE:
                    phase = PHASE_INTRO;
                    me->SetReactState(REACT_PASSIVE);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    me->SetFlying(true);
                    me->GetMotionMaster()->MovePoint(WP_FLY_AWAY, gorionaPos[WP_FLY_AWAY], true);
                    flyAway = true;
                    break;
                case ACTION_WARMASTER_JUMPS_ON_BOARD:
                    twilightFlamesTimer = 8000;
                    phase = PHASE_WARMASTER_ON_BOARD;
                    me->GetMotionMaster()->MovePoint(WP_WARMASTER_TAKE_OFF, gorionaPos[WP_WARMASTER_TAKE_OFF], true);
                    RunPlayableQuote(captainSwayze, NPC_SKY_CAPTAIN_SWAYZE);

                    scheduler.Schedule(Seconds(4), [this](TaskContext /*task context*/)
                    {
                        if (Vehicle * veh = me->GetVehicleKit())
                        {
                            if (Unit * passenger = veh->GetPassenger(0))
                                passenger->ExitVehicle();
                        }
                        me->GetMotionMaster()->MovePoint(WP_GORIONA_SECOND_PHASE, gorionaPos[WP_RIGHT_FRONT_ENGINE], true);
                        if (Creature * pBlackhorn = me->FindNearestCreature(BOSS_WARMASTER_BLACKHORN, 300.0f, true))
                            pBlackhorn->AI()->DoAction(ACTION_WARMASTER_JUMPS_ON_BOARD);
                    });
                    break;
                default:
                    break;
                }
            }
        }

        void SpellHitTarget(Unit* pTarget, const SpellEntry* spell) 
        {
            if (!spell || !pTarget)
                return;

            if (spell->Id == SPELL_TWILIGHT_BLAST)
                pTarget->CastSpell(pTarget, SPELL_ENGINE_FIRE, true);
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            // Intro Flight
            if (canMoveNextPoint)
            {
                canMoveNextPoint = false;

                if (nextWP == WP_GUNSHIP_FRONT)
                    me->GetMotionMaster()->MovePoint(WP_GUNSHIP_FRONT, gorionaPos[WP_GUNSHIP_FRONT], true);
                else if (nextWP == LEFT_FRONT_ENGINE)
                    me->GetMotionMaster()->MovePoint(WP_LEFT_FRONT_ENGINE, gorionaPos[WP_LEFT_FRONT_ENGINE], true);
            }

            // Twilight Assault drakes spawn part
            if (nextWave)
            {
                if (waveTimer <= diff)
                {
                    waveCount++;

                    if (waveCount <= 2)
                    {
                        SummonNPC(NPC_TWILIGHT_ASSAULT_DRAKE_LEFT);
                        SummonNPC(NPC_TWILIGHT_ASSAULT_DRAKE_RIGHT);
                        waveTimer = 60000;
                    }
                    else if (waveCount == 3)
                    {
                        nextWave = false;
                        me->AI()->DoAction(ACTION_WARMASTER_JUMPS_ON_BOARD);
                    }
                }
                else waveTimer -= diff;
            }

            if (!UpdateVictim())
                return;

            if (phase == PHASE_GORIONA_AIR_ATTACK)
            {
                // Twilight Onslaught
                if (twilightOnsloughtTimer <= diff)
                {
                    RunPlayableQuote(warmasterGorionaOnslaught, BOSS_WARMASTER_BLACKHORN);
                    twilightOnsloughtTimer = 30000;

                    std::vector<int> locations;
                    for (uint32 i = !IsHeroic() ? 0 : 2; i < MAX_WARMASTER_DAMAGE_POSITIONS; i++)
                    {
                        if (OnslaughtBoardSpotStatus(i, CHECK) == EMPTY_SPOT)
                            locations.push_back(i);
                    }

                    if (locations.size() != 0)
                    {
                        uint32 randPos = locations[urand(0, locations.size() - 1)];

                        if (OnslaughtBoardSpotStatus(randPos, CHECK) == EMPTY_SPOT)
                        {
                            if (Creature * pOnslaughtTarget = me->SummonCreature(NPC_ONSLAUGHT_TARGET, warmasterDamagePos[randPos].GetPositionX(), warmasterDamagePos[randPos].GetPositionY(), warmasterDamagePos[randPos].GetPositionZ(), me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 10000))
                            {
                                OnslaughtBoardSpotStatus(randPos, SET_FULL);
                                pOnslaughtTarget->CastSpell(pOnslaughtTarget, SPELL_TWILIGHT_ONSLAUGHT_DUMMY, false);
                                me->CastSpell(warmasterDamagePos[randPos].GetPositionX(), warmasterDamagePos[randPos].GetPositionY(), warmasterDamagePos[randPos].GetPositionZ(), SPELL_TWILIGHT_ONSLAUGHT, false);

                                scheduler.Schedule(Seconds(5), [this, randPos](TaskContext /*task context*/)
                                {
                                    OnslaughtBoardSpotStatus(randPos, SET_EMPTY);
                                });
                            }
                        }
                    }
                    else twilightOnsloughtTimer = 5000;
                }
                else twilightOnsloughtTimer -= diff;

                // Twilight Infiltrator
                if (twilightInfiltratorTimer <= diff)
                {
                    SummonNPC(NPC_TWILIGHT_INFILTRATOR);
                    twilightInfiltratorTimer = 40000;
                }
                else twilightInfiltratorTimer -= diff;

                // Heroic ability - Broadside
                if (IsHeroic())
                {
                    if (broadsideTimer <= diff)
                    {
                        // Ship should take 20% of its current health
                        me->CastSpell(me, SPELL_BROADSIDE_AOE, false);
                        if (Creature* pShip = me->FindNearestCreature(NPC_SKYFIRE, SEARCH_RANGE))
                        {
                            uint32 damage = pShip->GetHealth() * 0.2;
                            me->DealDamage(pShip, damage);
                        }
                        broadsideTimer = 100000;

                        if (spreadFire == true)
                        {
                            scheduler.Schedule(Seconds(5), 1,[this](TaskContext spreadFire)
                            {
                                Creature * pFireStalker = me->SummonCreature(NPC_FIRE_STALKER, boardSpots[fireSpot[spreadFire.GetRepeatCounter()].spotX][fireSpot[spreadFire.GetRepeatCounter()].spotY].x, boardSpots[fireSpot[spreadFire.GetRepeatCounter()].spotX][fireSpot[spreadFire.GetRepeatCounter()].spotY].y, START_Z, 0, TEMPSUMMON_TIMED_DESPAWN, 300000);
                                if (pFireStalker && spreadFire.GetRepeatCounter() == 0)
                                    pFireStalker->CastSpell(pFireStalker, SPELL_DECK_FIRE_SPAWN, false);
                                SetBoardSpotFull(fireSpot[spreadFire.GetRepeatCounter()].spotX, fireSpot[spreadFire.GetRepeatCounter()].spotY);
                                if (spreadFire.GetRepeatCounter() < 19)
                                    spreadFire.Repeat(Seconds(6));
                            });
                        }
                        spreadFire = false;
                    }
                    else broadsideTimer -= diff;
                }
            }
            else if (phase == PHASE_WARMASTER_ON_BOARD)
            {
                // She should land on board in heroic mode
                if (IsHeroic())
                {
                    if (me->GetHealthPct() <= 90)
                        phase = PHASE_GORIONA_LANDS_ON_BOARD;
                }

                // Twilight Flames
                if (twilightFlamesTimer <= diff)
                {
                    if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, SEARCH_RANGE, true))
                        me->CastSpell(pTarget, SPELL_TWILIGHT_FLAMES_MISSILE, false);
                    twilightFlamesTimer = 8000;
                }
                else twilightFlamesTimer -= diff;
            }
            else if (phase == PHASE_GORIONA_LANDS_ON_BOARD)
            {
                if (!heroicLand)
                {
                    me->GetMotionMaster()->MovePoint(WP_LAND_POSITION, gorionaPos[WP_WARMASTER_TAKE_OFF], true, true);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->SetFlying(false);
                    me->RemoveAura(SPELL_FORCE_FLYING);
                    twilightBreathTimer = 15000;
                    consumingShroudTimer = 20000;
                    heroicLand = true;
                }
                else
                {
                    // Twilight Breath
                    if (twilightBreathTimer <= diff)
                    {
                        me->CastSpell(me->GetVictim(), SPELL_TWILIGHT_BREATH, false);
                        twilightBreathTimer = 20000;
                    }
                    else twilightBreathTimer -= diff;

                    // Consuming Shroud
                    if (consumingShroudTimer <= diff)
                    {
                        if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, SEARCH_RANGE, true))
                            me->CastSpell(pTarget, SPELL_CONSUMING_SHROUD, false);
                        consumingShroudTimer = 30000;
                    }
                    else consumingShroudTimer -= diff;

                    DoMeleeAttackIfReady();
                }
            }

            // Fuck it, I'm out of here
            if (!flyAway && me->GetHealthPct() <= 20)
            {
                me->AI()->DoAction(ACTION_GORIONA_LEAVES_SCENE);
            }
        }
    };
};

class npc_ds_twilight_assault_drake : public CreatureScript
{
public:
    npc_ds_twilight_assault_drake() : CreatureScript("npc_ds_twilight_assault_drake") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_twilight_assault_drakeAI(pCreature);
    }

    struct npc_ds_twilight_assault_drakeAI : public ScriptedAI
    {
        npc_ds_twilight_assault_drakeAI(Creature *creature) : ScriptedAI(creature) { }

        TaskScheduler scheduler;
        uint32 harpoonTimer;
        uint32 releaseTimer;
        uint32 twilightBarrageTimer;
        bool harpoon;
        TARGET_SPOTS boardSpots[MAX_VERTICAL_LINES][MAX_HORIZONTAL_LINES];

        void Reset() override
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetFlying(true);
            me->SetSpeed(MOVE_FLIGHT, 1.5f);
            me->SetInCombatWithZone();
            harpoonTimer = 25000;
            twilightBarrageTimer = urand(8000, 16000);
            harpoon = false;

            scheduler.Schedule(Seconds(7), [this](TaskContext /*task context*/)
            {
                if (Vehicle * veh = me->GetVehicleKit())
                {
                    if (Unit * passenger = veh->GetPassenger(0))
                    {
                        passenger->ExitVehicle();
                        passenger->GetMotionMaster()->MoveFall();
                    }
                }

                if (me->GetEntry() == NPC_TWILIGHT_ASSAULT_DRAKE_LEFT)
                    me->GetMotionMaster()->MovePoint(LEFT_DRAKE_END_FLY_POS, assaultDrakePos[LEFT_DRAKE_END_FLY_POS], true, false);
                else
                    me->GetMotionMaster()->MovePoint(RIGHT_DRAKE_END_FLY_POS, assaultDrakePos[RIGHT_DRAKE_END_FLY_POS], true, false);

                // Set correct facing to raid
                scheduler.Schedule(Seconds(5), [this](TaskContext /*task context*/)
                {
                    if (me->GetEntry() == NPC_TWILIGHT_ASSAULT_DRAKE_LEFT)
                        me->SetFacingTo(assaultDrakePos[LEFT_DRAKE_END_FLY_POS].GetOrientation());
                    else
                        me->SetFacingTo(assaultDrakePos[RIGHT_DRAKE_END_FLY_POS].GetOrientation());
                });
            });
        }

        void SetBoardSpots()
        {
            float startX = START_X;
            float startY = START_Y;

            for (uint8 i = 0; i < MAX_VERTICAL_LINES; i++)
            {
                startY = START_Y;

                for (uint8 j = 0; j < MAX_HORIZONTAL_LINES; j++)
                {
                    boardSpots[i][j].free = true;
                    boardSpots[i][j].z = START_Z;
                    boardSpots[i][j].x = startX;
                    boardSpots[i][j].y = startY;
                    startY += STEP_Y;
                }
                startX += STEP_X;
            }
        }

        void SpellHit(Unit* /*caster*/, SpellEntry const* spell) override
        {
            if (spell->Id == SPELL_HARPOON)
            {
                me->SetSpeed(MOVE_FLIGHT, 1.0f);
                if (me->GetEntry() == NPC_TWILIGHT_ASSAULT_DRAKE_LEFT)
                    me->GetMotionMaster()->MovePoint(0, harpoonPos[RIGHT_HARPOON_POSITION], true, true);
                else
                    me->GetMotionMaster()->MovePoint(0, harpoonPos[LEFT_HARPOON_POSITION], true, true);
            }
        }

        void EnterCombat(Unit * /*who*/) override
        {
            SetBoardSpots();
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            // Harpoons
            if (!harpoon)
            {
                if (harpoonTimer <= diff)
                {
                    if (Creature * pHarpoon = me->FindNearestCreature(NPC_HARPOON, 200.0f, true))
                        pHarpoon->CastSpell(me, SPELL_HARPOON, true);

                    uint32 releaseTimer = IsHeroic() ? 20 : 25;
                    scheduler.Schedule(Seconds(releaseTimer), [this](TaskContext harpoon)
                    {
                        if (harpoon.GetRepeatCounter() == 0)
                        {
                            if (Creature * pHarpoon = me->FindNearestCreature(NPC_HARPOON, 200.0f, true))
                            {
                                pHarpoon->InterruptNonMeleeSpells(true);
                                me->SetSpeed(MOVE_FLIGHT, 2.0f);
                                if (me->GetEntry() == NPC_TWILIGHT_ASSAULT_DRAKE_LEFT)
                                    me->GetMotionMaster()->MovePoint(LEFT_DRAKE_END_FLY_POS, assaultDrakePos[LEFT_DRAKE_END_FLY_POS], true);
                                else me->GetMotionMaster()->MovePoint(RIGHT_DRAKE_END_FLY_POS, assaultDrakePos[RIGHT_DRAKE_END_FLY_POS], true);
                            }
                            harpoon.Repeat(Seconds(3));
                        }
                        else
                        {
                            if (me->GetEntry() == NPC_TWILIGHT_ASSAULT_DRAKE_LEFT)
                                me->SetFacingTo(assaultDrakePos[LEFT_DRAKE_END_FLY_POS].GetOrientation());
                            else
                                me->SetFacingTo(assaultDrakePos[RIGHT_DRAKE_END_FLY_POS].GetOrientation());
                        }
                    });
                    harpoon = true;
                }
                else harpoonTimer -= diff;
            }

            // Twilight Barrage
            if (twilightBarrageTimer <= diff)
            {
                uint32 randX = urand(0, MAX_VERTICAL_LINES-1);
                uint32 randY = urand(0, MAX_HORIZONTAL_LINES - 1);

                Creature * pGoriona = me->FindNearestCreature(NPC_GORIONA, 300.0f, true);
                if (pGoriona == NULL)
                    return;

                if (npc_ds_goriona::npc_ds_gorionaAI* pAI = (npc_ds_goriona::npc_ds_gorionaAI*)(pGoriona->GetAI()))
                {
                    if (pAI->ValidateFreeSpot(DATA_WARMASTER_BOARD_SPOT, randX, randY) == EMPTY_SPOT)
                        me->CastSpell(boardSpots[randX][randY].x, boardSpots[randX][randY].y, boardSpots[randX][randY].z, SPELL_TWILIGHT_BARRAGE, false);
                }
                twilightBarrageTimer = 8000;
            }
            else twilightBarrageTimer -= diff;

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

class spell_ds_twilight_barrage_dmg : public SpellScriptLoader
{
public:
    spell_ds_twilight_barrage_dmg() : SpellScriptLoader("spell_ds_twilight_barrage_dmg") { }

    class spell_ds_twilight_barrage_dmg_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_twilight_barrage_dmg_SpellScript);

        uint32 hitUnitCount;

        bool Load()
        {
            hitUnitCount = 0;
            return true;
        }

        void FilterTargetsDamage(std::list<Unit*>& targets)
        {
            if (!GetCaster())
                return;

            hitUnitCount = targets.size();

            if (targets.empty())
            {
                // If there is no player to soak dmg, ship should take 1,5* more dmg itself
                if (Creature* pShip = GetCaster()->FindNearestCreature(NPC_SKYFIRE, SEARCH_RANGE))
                {
                    int32 bp0 = GetSpellInfo()->EffectBasePoints[EFFECT_0];
                    bp0 *= 1.5f; 
                    GetCaster()->DealDamage(pShip, bp0);
                }

                // If ship takes damage from Twilight Barrage
                // send information that achievement failed to Warmaster Blackhorn
                if (Creature* pWarmaster = GetCaster()->FindNearestCreature(BOSS_WARMASTER_BLACKHORN, SEARCH_RANGE))
                    pWarmaster->AI()->DoAction(ACTION_ACHIEVEMENT_FAILED);
            } 
        }

        void HandleDamage(SpellEffIndex /*effIndex*/)
        {
            Unit * hitUnit = GetHitUnit();

            if (!hitUnit || hitUnitCount == 0)
                return;

            int32 damage = GetHitDamage() / (hitUnitCount); // Shared damage

            SetHitDamage(damage);
        }

        void Register()
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_twilight_barrage_dmg_SpellScript::FilterTargetsDamage, EFFECT_0, TARGET_UNIT_AREA_ENEMY_DST);
            OnEffect += SpellEffectFn(spell_ds_twilight_barrage_dmg_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_twilight_barrage_dmg_SpellScript();
    }
};

class spell_ds_twilight_onslaught_dmg : public SpellScriptLoader
{
public:
    spell_ds_twilight_onslaught_dmg() : SpellScriptLoader("spell_ds_twilight_onslaught_dmg") { }

    class spell_ds_twilight_onslaught_dmg_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_twilight_onslaught_dmg_SpellScript);

        uint32 hitUnitCount;

        bool Load()
        {
            hitUnitCount = 0;
            return true;
        }

        void FilterTargetsDamage(std::list<Unit*>& targets)
        {
            hitUnitCount = targets.size();

            if (targets.empty())
            {
                // If there is no player to soak dmg, ship should take full dmg itself
                if (Creature* pShip = GetCaster()->FindNearestCreature(NPC_SKYFIRE, SEARCH_RANGE))
                {
                    int32 bp0 = GetSpellInfo()->EffectBasePoints[EFFECT_0];
                    GetCaster()->DealDamage(pShip, bp0);
                }
            }

            int32 damage = GetSpellInfo()->EffectBasePoints[EFFECT_0] / (hitUnitCount + 1); // Shared damage + ship
            // Deal damage to the ship
            if (Creature* pShip = GetCaster()->FindNearestCreature(NPC_SKYFIRE, 300.0f))
                GetCaster()->DealDamage(pShip, damage);
            // Despawn trigger
            if (Creature* pOnslaught = GetCaster()->FindNearestCreature(NPC_ONSLAUGHT_TARGET, 300.0f))
                pOnslaught->DespawnOrUnsummon();
        }

        void HandleDamage(SpellEffIndex /*effIndex*/)
        {
            int32 damage = GetHitDamage() / (hitUnitCount + 1); // Shared damage + ship
            SetHitDamage(damage);
        }

        void Register()
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_twilight_onslaught_dmg_SpellScript::FilterTargetsDamage, EFFECT_0, TARGET_UNIT_AREA_ENEMY_DST);
            OnEffect += SpellEffectFn(spell_ds_twilight_onslaught_dmg_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_twilight_onslaught_dmg_SpellScript();
    }
};

class spell_ds_warmaster_blackhorn_vengeance : public SpellScriptLoader
{
public:
    spell_ds_warmaster_blackhorn_vengeance() : SpellScriptLoader("spell_ds_warmaster_blackhorn_vengeance") { }

    class spell_ds_warmaster_blackhorn_vengeance_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_ds_warmaster_blackhorn_vengeance_AuraScript);

        void HandlePeriodicTick(AuraEffect const* aurEff /*aurEff*/)
        {
            if (!GetUnitOwner())
                return;

            uint32 val = int32(100.0f - GetUnitOwner()->GetHealthPct());

            if (AuraEffect * aurEff = GetAura()->GetEffect(EFFECT_0))
                aurEff->SetAmount(val);
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_ds_warmaster_blackhorn_vengeance_AuraScript::HandlePeriodicTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_ds_warmaster_blackhorn_vengeance_AuraScript();
    }
};

class IsNotBroadsideTarget
{
public:
    bool operator()(WorldObject* object) const
    {
        if (object->ToCreature())
            if (object->ToCreature()->GetEntry() == NPC_BROADSIDE_TARGET)
                return false;
        return true;
    }
};

class spell_ds_warmaster_broadside : public SpellScriptLoader
{
public:
    spell_ds_warmaster_broadside() : SpellScriptLoader("spell_ds_warmaster_broadside") { }

    class spell_ds_warmaster_broadside_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_warmaster_broadside_SpellScript);

        void FilterTargetsDamage(std::list<Unit*>& targets)
        {
            targets.remove_if(IsNotBroadsideTarget());
        }

        void Register()
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_warmaster_broadside_SpellScript::FilterTargetsDamage, EFFECT_0, TARGET_UNIT_AREA_ENTRY_SRC);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_warmaster_broadside_SpellScript();
    }
};

void AddSC_boss_warmaster_blackhorn()
{
    new boss_warmaster_blackhorn();
    new npc_ds_goriona();
    new npc_ds_twilight_assault_drake();

    new spell_ds_twilight_barrage_dmg();
    new spell_ds_twilight_onslaught_dmg();
    new spell_ds_warmaster_blackhorn_vengeance();
    new spell_ds_warmaster_broadside();
}

//////////////////////////////////////////////////////////////////////////////////////
///////////////////////////         SQL QUERY          ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
/*
--NPC
--Warmaster Blackhorn
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`,
`dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`,
`resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`,
`equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('56427','0','0','0','0','0','39399','0','0','0','Warmaster Blackhorn','','','0','88','88','3','14','14','0','2','2','1','1','40000','45000','0','0','1','1500','2000','1','32832','0','0','0','0','0','0','0','0','0','7','76','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0',
'0','0','0','0','0','0','','0','3','240','100','1','0','0','0','0','0','0','0','184','1','56427','0','1','boss_warmaster_blackhorn','15595');

--Goriona
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`,
`dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`,
`resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`,
`equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('56781','0','0','0','0','0','39691','0','0','0','Goriona','','','0','88','88','3','14','14','0','1.5873','1.2','1','1','0','0','0','0','1','1500','2000','1','0','0','0','0','0','0','0','0','0','0','2','72','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','1920',
'0','0','','0','7','155','1','1','0','0','0','0','0','0','0','127','1','0','0','0','npc_ds_goriona','15595');

--Broadside spawn positions
insert into `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`, `curhealth`, `curmana`, `DeathState`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`) values('218817','119559','967','15','1','0','0','13438.4','-12162','149.402','1.50303','300','0','0','77491','0','0','0','0','0','0');
insert into `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`, `curhealth`, `curmana`, `DeathState`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`) values('218815','119559','967','15','1','0','0','13417.5','-12159.5','149.402','1.40878','300','0','0','77491','0','0','0','0','0','0');
insert into `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`, `curhealth`, `curmana`, `DeathState`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`) values('218816','119559','967','15','1','0','0','13428.5','-12160.9','149.402','1.44805','300','0','0','77491','0','0','0','0','0','0');
insert into `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`, `curhealth`, `curmana`, `DeathState`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`) values('218818','119559','967','15','1','0','0','13447.2','-12162.6','149.402','1.53052','300','0','0','77491','0','0','0','0','0','0');
insert into `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`, `curhealth`, `curmana`, `DeathState`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`) values('218820','119559','967','15','1','0','0','13458.2','-12163','149.402','1.53445','300','0','0','77491','0','0','0','0','0','0');


--SPELL SCRIPTS
insert into `spell_script_names` (`spell_id`, `ScriptName`) values('110153','spell_ds_warmaster_broadside');
insert into `spell_script_names` (`spell_id`, `ScriptName`) values('108045','spell_ds_warmaster_blackhorn_vengeance');

insert into `spell_script_names` (`spell_id`, `ScriptName`) values
('106401','spell_ds_twilight_onslaught_dmg'),
('109226','spell_ds_twilight_onslaught_dmg'),
('109227','spell_ds_twilight_onslaught_dmg'),
('108862','spell_ds_twilight_onslaught_dmg');

insert into `spell_script_names` (`spell_id`, `ScriptName`) values
('109205','spell_ds_twilight_barrage_dmg'),
('107439','spell_ds_twilight_barrage_dmg'),
('109204','spell_ds_twilight_barrage_dmg'),
('109203','spell_ds_twilight_barrage_dmg');
*/