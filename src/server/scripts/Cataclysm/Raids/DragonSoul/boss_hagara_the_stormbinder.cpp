/*
 * Copyright (C) 2006-2015 iCe Online <http://ice-wow.eu>
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
Encounter: Hagara the Stormbinder
Dungeon: Dragon Soul
Difficulty: Normal / Heroic
Mode: 10-man / 25-man
Autor: Lazik
*/

/* 95 % completed
TO DO:
1) Somehow enable mowing during Focused Assault channeled cast in Heroic version
*/

#include "ScriptPCH.h"
#include "dragonsoul.h"
#include "MapManager.h"
#include "TaskScheduler.h"

#include <stack>

const uint32 DATA_ICE_WAVE_SLOT         = 1;
const float CONDUIT_RANGE               = 10.0f;
const float WATERY_ENTRENCHMENT_RANGE   = 30.0f;
const uint32 FROSTFLAKE_MAX_STACK       = 10;
const float SEARCH_RANGE                = 300.0f;
const uint32 MAX_TRASH                  = 35;
const uint32 MAX_PLAYERS_25MAN          = 25;
const uint32 MAX_PLAYERS_10MAN          = 10;
const float HAGARA_MELEE_RANGE          = 7.0f;
const uint32 MAX_CONDUCTORS             = 4;
const uint32 MAX_CONDUCTORS_10HC        = 8;

// NPCs
enum NPC
{
    BOSS_HAGARA_THE_STORMBINDER     = 55689,
    NPC_ICE_TOMB                    = 55695,
    NPC_ICE_LANCE                   = 56108,
    NPC_ICE_WAVE                    = 56104,
    NPC_FROZEN_BINDING_CRYSTAL      = 56136,
    NPC_CRYSTAL_CONDUCTOR           = 56165,
    NPC_BOUND_LIGHNING_ELEMENTAL    = 56700,
    NPC_COLLAPSING_ICICLE           = 57867,
    NPC_HAGARA_TRASH_PORTAL         = 57809,
    NPC_HOVER_DISC_VEHICLE          = 57924,
    NPC_LIGHTNING_STORM_TEST        = 119950,
    NPC_FROSTFLAKE_SNARE            = 119556,
};

// Spells
enum Spells
{
    // Normal Phase
    SPELL_FOCUSED_ASSAULT           = 107850, // Focused Assault dmg
    SPELL_FOCUSED_ASSAULT_10N       = 107851, // 10N aura
    SPELL_FOCUSED_ASSAULT_25N       = 110900, // 25N aura
    SPELL_FOCUSED_ASSAULT_10HC      = 110899, // 10HC aura
    SPELL_FOCUSED_ASSAULT_25HC      = 110898, // 10HC aura

    SPELL_ICE_LANCE_DUMMY           = 105269, // Ice Lance - dummy aura
    SPELL_ICE_LANCE_SUMMON          = 105297, // Summon target dest dest
    SPELL_TARGET                    = 105285, // Target
    SPELL_ICE_LANCE                 = 105298, // ?
    SPELL_ICE_LANCE_MISSILE         = 105313, // Missile
    SPELL_ICE_LANCE_CONE            = 105287, // Cone enemy 104
    SPELL_ICE_LANCE_10N             = 105316, // 10N dmg 
    SPELL_ICE_LANCE_25N             = 107061, // 25N dmg
    SPELL_ICE_LANCE_10HC            = 107062, // 10HC dmg
    SPELL_ICE_LANCE_25HC            = 107063, // 25HC dmg

    SPELL_ICE_TOMB_DUMMY            = 104448, // Ice Tomb
    SPELL_ICE_TOMB_STUN             = 104451, // Stun + dmg
    SPELL_ICE_TOMB_MISSILE          = 104449, // Ice Tomb
    SPELL_ICE_TOMB_EFFECT           = 99837,  // Crystal Prison Trap Effect - used for heal immunity

    SPELL_SHATTERED_ICE_10N         = 105289, // Shattered Ice 10N
    SPELL_SHATTERED_ICE_25N         = 108567, // Shattered Ice 25N
    SPELL_SHATTERED_ICE_10HC        = 110888, // Shattered Ice 10HC
    SPELL_SHATTERED_ICE_25HC        = 110887, // Shattered Ice 25HC

    // Ice Phase
    SPELL_FROZEN_TEMPEST_10N        = 105256, // Frozen Tempest 10N
    SPELL_FROZEN_TEMPEST_25N        = 109552, // Frozen Tempest 25N
    SPELL_FROZEN_TEMPEST_10HC       = 109553, // Frozen Tempest 10HC
    SPELL_FROZEN_TEMPEST_25HC       = 109554, // Frozen Tempest 25HC

    SPELL_WATERY_ENTRENCHMENT       = 110317, // Watery Entrenchment - aura periodic dmg percent + snare
    SPELL_WATERY_ENTRENCHMENT_1     = 105259, // Watery Entrenchment - area trigger

    SPELL_ICICLE                    = 109315,
    SPELL_ICICLE_FALL_DUMMY         = 69428, // Couldn`t find correct ID, but this will do the trick
    SPELL_ICE_SHARDS                = 62457, // -//-

    SPELL_ICE_WAVE_AURA             = 105265, // Ice Wave aura
    SPELL_ICE_WAVE_DMG              = 105314, // Ice Wave dmg

    SPELL_CRYSTALLINE_OVERLOAD      = 105312, // Crystalline Overload - Description: Increases the damage taken by Hagara the Binder by $s1%.
    SPELL_CRYSTALLINE_TETHER        = 105311, // Crystalline Tether - dummy aura
    SPELL_CRYSTALLINE_TETHER_DUMMY  = 105482, // Crystalline Tether - dummy aura

    // Heroic ability
    SPELL_FROSTFLAKE                = 109325,
    SPELL_FROSTFLAKE_SNARE          = 72217, // Original 110316 - replaced by Frost Trap slow effect
    SPELL_FROSTFLAKE_SNARE_TRIGGER  = 109337,

    // Lightning Phase
    SPELL_WATER_SHIELD_10N          = 105409, // Water Shield 10N
    SPELL_WATER_SHIELD_25N          = 109560, // Water Shield 25N
    SPELL_WATER_SHIELD_10HC         = 109561, // Water Shield 10HC
    SPELL_WATER_SHIELD_25HC         = 109562, // Water Shield 25HC

    SPELL_LIGHTNING_STORM           = 105467, // Lightning Storm - force cast
    SPELL_LIGHTNING_STORM_10N       = 105465, // Lightning Storm 10N
    SPELL_LIGHTNING_STORM_25N       = 108568, // Lightning Storm 25N
    SPELL_LIGHTNING_STORM_10HC      = 110893, // Lightning Storm 10HC
    SPELL_LIGHTNING_STORM_25HC      = 110892, // Lightning Storm 25HC
    SPELL_LIGHTNING_STORM_COSMETIC  = 105466, // Lightning Storm Cosmetic

    SPELL_LIGHTNING_CONDUIT         = 105367, // Lightning Conduit aura
    SPELL_LIGHTNING_CONDUIT_DUMMY   = 105371, // Lightning Conduit dummy
    SPELL_LIGHTNING_CONDUIT_X       = 105377, // ?
    SPELL_LIGHTNING_CONDUIT_10N     = 105369, // Lightning Conduit 10N - aura and periodic dmg
    SPELL_LIGHTNING_CONDUIT_25N     = 108569, // Lightning Conduit 25N
    SPELL_LIGHTNING_CONDUIT_10HC    = 109201, // Lightning Conduit 10HC
    SPELL_LIGHTNING_CONDUIT_25HC    = 109202, // Lightning Conduit 25HC

    SPELL_LIGHTNING_ROD             = 105343, // Lightning Rod - dummy visual - visual aura on lightning rod
    SPELL_LIGHTNING_ROD_DUMMY       = 109180, // Lightning Rod - dummy visual - shining under lightning rod when overloaded

    // Heroic ability
    SPELL_STORM_PILLAR              = 109541, // Trigger circle under player
    SPELL_STORM_PILLAR_DUMMY        = 109557,
    SPELL_STORM_PILLAR_10N          = 109563, // 10N dmg
    SPELL_STORM_PILLAR_25N          = 109564, // 25N dmg
    SPELL_STORM_PILLAR_10HC         = 109565, // 10HC dmg
    SPELL_STORM_PILLAR_25HC         = 109566, // 25HC dmg

    // Other spells
    SPELL_OVERLOAD                  = 105481, // Overload
    SPELL_OVERLOAD_FORCE_STORM      = 105487, // Overload

    SPELL_PERMANENT_FEIGN_DEATH     = 70628,  // Permanent Feign Death

    SPELL_TELEPORT_VISUAL           = 101812, // Visual Port
    SPELL_ENRAGE                    = 64238,  // Enrage
    SPELL_FEEDBACK                  = 108934, // Feedback
    SPELL_KNOCKBACK                 = 105048,

    SPELL_HAGARA_LIGHTNING_AXES     = 109670,
    SPELL_HAGARA_FROST_AXES         = 109671,

    ACHIEVEMENT_HOLDING_HANDS       = 6175,
    ACHIEVEMENT_HEROIC_HAGARA       = 6112,
};

enum Phases
{
    NORMAL_PHASE       = 0,
    LIGHTNING_PHASE    = 1,
    ICE_PHASE          = 2,
};

enum HagaraGameobjects
{
    GO_FOCUSING_IRIS        = 210132,
};

enum HagaraActions
{
    ACTION_INTRO                = 0,
    ACTION_CRYSTAL_DIED         = 1,
    ACTION_PREPARE_FOR_FIGHT    = 2,
    ACTION_CONDUCTOR_CHARGED    = 3,
};

enum HagaraWaves
{
    INTRO_FIRST_WAVE        = 1,
    INTRO_SECOND_WAVE       = 2,
    INTRO_THIRD_WAVE        = 3,
    INTRO_FOURTH_WAVE       = 4,
};

enum ItemEntries
{
    // DB equipment_template entry 
    LIGHNING_AXE_ENTRY            = 55689,
    ICE_AXE_ENTRY                 = 57462,
    // DB item_template entry
    //ICE_AXE_ENTRY               = 92284,
    //LIGHTNING_AXE_ENTRY         = 92285,
};

enum PortalPositions
{
    FLY_POSITION                = 0,
    CENTER_PORTAL               = 1,
    LEFT_PORTAL_NEAR            = 2,
    RIGHT_PORTAL_NEAR           = 3,
    LEFT_PORTAL_FAR             = 4,
    RIGHT_PORTAL_FAR            = 5,
};

const uint8 MAX_PORTAL_POSTIONS             = 6;

const Position portalPos[MAX_PORTAL_POSTIONS] =
{
    { 13551.85f, 13612.07f, 129.94f, 6.20f }, // hagara fly position
    { 13545.51f, 13612.07f, 123.49f, 0.00f }, // center portal
    { 13557.44f, 13643.37f, 123.48f, 5.45f }, // 1st left portal
    { 13557.31f, 13580.67f, 123.48f, 0.78f }, // 1st rigt portal
    { 13594.21f, 13654.59f, 123.48f, 4.52f }, // 2nd left portal
    { 13596.83f, 13569.83f, 123.48f, 1.70f }, // 2nd right portal
};

const uint8 MAX_SPAWN_POSTIONS              = 6;

const float SpawnPos[MAX_SPAWN_POSTIONS][4] =
{
    // Frozen Binding Crystals
    { 13617.26f, 13643.15f, 123.48f, 3.922f },
    { 13557.48f, 13643.17f, 123.48f, 5.493f },
    { 13557.68f, 13580.76f, 123.48f, 0.812f },
    { 13617.60f, 13580.38f, 123.48f, 2.363f },
    // Hagara Spawn Position
    { 13587.70f, 13612.20f, 122.42f, 1.530f },
    // Bound Lightning Elemental
    { 13588.70f, 13648.67f, 123.45f, 4.677f },
};

struct Dist
{
    uint64 guid;
    float distance;
};

struct PlayableQuote aggro { 26227, "You cross the Stormbinder!I'll slaughter you all." };
struct PlayableQuote justDied { 26243, "Cowards! You pack of weakling... dogs..." };

const uint8 MAX_KILL_QUOTES            = 4;

struct PlayableQuote killedUnit[MAX_KILL_QUOTES]
{
    { 26255, "You should have run, dog." },
    { 26254, "Feh!" },
    { 26256, "Down, pup." },
    { 26257, "A waste of my time." },
};

const uint8 MAX_CRYSTAL_QUOTES          = 6;

struct PlayableQuote crystalDied[MAX_CRYSTAL_QUOTES]
{
    { 26235, "The time I spent binding that, WASTED!" },
    { 26236, "You'll PAY for that." },
    { 26237, "Enough!" },
    { 26239, "Again ?!" },
    { 26240, "Impudent pup!" },
    { 26241, "The one remaining is still enough to finish you." },
};

const uint8 MAX_INTRO_QUOTES            = 5;

struct PlayableQuote intro[MAX_INTRO_QUOTES]
{
    { 26223, "Even with the Aspect of Time on your side, you stumble foolishly into a trap?"},
    { 26224, "Don't preen just yet, little pups. We'll cleanse this world of your kind." },
    { 26225, "You'll not leave this place alive!" },
    { 26226, "Not one of you will live to see the final cataclysm! Finish them!" },
    { 26251, "Swagger all you like, you pups don't stand a chance. Flee now, while you can." },
};

const uint8 MAX_CONDUCTOR_QUOTES        = 6;

struct PlayableQuote conductorCharged[MAX_CONDUCTOR_QUOTES]
{
    { 26228, "What are you doing?"},
    { 26229, "You're toying with death." },
    { 26230, "You think you can play with my lightning?" },
    { 26232, "Enough of your games! You won't live to do it again." },
    { 26233, "No! More... lightning..." },
    { 26234, "I'll finish you now pups!" },
};

const uint8 MAX_ICE_LANCE_QUOTES         = 3;

struct PlayableQuote iceLance[MAX_ICE_LANCE_QUOTES]
{
    { 26244, "You face more than my axes, this close." },
    { 26245, "See what becomes of those who stand before me!" },
    { 26246, "Feel a chill up your spine...?" },
};

const uint8 MAX_OTHER_QUOTES              = 2;

struct PlayableQuote specialPhaseEnd[MAX_OTHER_QUOTES]
{
    { 26238, "Aughhhh!" },
    { 26231, "Impossible! Aughhhh!" },
};

struct PlayableQuote iceTomb[MAX_OTHER_QUOTES]
{
    { 26249, "Stay, pup."},
    { 26250, "Hold still." },
};

struct PlayableQuote iceWave[MAX_OTHER_QUOTES]
{
    { 26247, "You can't outrun the storm." },
    { 26248, "Die beneath the ice." },
};

struct PlayableQuote storm[MAX_OTHER_QUOTES]
{
    { 26252, "Suffer the storm's wrath!"},
    { 26253, "Thunder and lightning dance at my call!" },
};

// Hagara the Stormbinder
class boss_hagara_the_stormbinder : public CreatureScript
{
public:
    boss_hagara_the_stormbinder() : CreatureScript("boss_hagara_the_stormbinder") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_hagara_the_stormbinderAI(pCreature);
    }

    struct boss_hagara_the_stormbinderAI : public ScriptedAI
    {
        boss_hagara_the_stormbinderAI(Creature *creature) : ScriptedAI(creature),summons(creature)
        {
            instance = creature->GetInstanceScript();

            if (instance->GetData(DATA_HAGARA_INTRO_TRASH) < MAX_TRASH)
            {
                me->SummonGameObject(GO_FOCUSING_IRIS, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), 0, 0, 0, 0, 86400);
                HoverDisc();
                me->GetMotionMaster()->MovePoint(FLY_POSITION, portalPos[FLY_POSITION]);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_PASSIVE);
                me->SetVisible(false);
            }
        }

        InstanceScript* instance;
        TaskScheduler scheduler;
        // Timers
        uint32 randomText;
        uint32 phase;
        uint32 nextPhase;
        uint32 frozenCrystalKilled;
        uint32 chargedConductorCount;
        uint32 introYell;
        uint32 waveCount;
        uint32 achievReqCount;
        uint32 endSpecialPhaseTimer;

        uint32 enrageTimer;
        uint32 normalPhaseTimer;
        uint32 focusedAssaultTimer;
        uint32 iceLanceTimer;
        uint32 shatteredIceTimer;
        uint32 iceTombTimer;
        uint32 checkTimer;
        uint32 icicleTimer;
        uint32 frostflakeTimer;
        uint32 stormPillarTimer;
        float lastIcicleAngle;

        uint64 targetGUID;
        uint64 frostflakeGUID;

        SummonList summons;
        std::vector<Dist> dist;

        bool enrage;
        bool teleport;
        bool icePhaseEnded;

        void Reset() override
        {
            if (instance)
            {
                if (instance->GetData(TYPE_BOSS_HAGARA) != DONE)
                    instance->SetData(TYPE_BOSS_HAGARA, NOT_STARTED);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LIGHTNING_CONDUIT);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LIGHTNING_CONDUIT_10N);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LIGHTNING_CONDUIT_25N);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LIGHTNING_CONDUIT_10HC);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LIGHTNING_CONDUIT_25HC);
            }

            if (instance && instance->GetData(DATA_HAGARA_INTRO_TRASH) >= MAX_TRASH)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_AGGRESSIVE);
                me->SetVisible(true);
            }

            enrageTimer = 480000; // 8 mins
            phase = NORMAL_PHASE; // Start with normal phase
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            nextPhase = urand(LIGHTNING_PHASE, ICE_PHASE);
            EquipCorrectAxes(nextPhase);

            normalPhaseTimer = 30000;
            focusedAssaultTimer = 5000;
            checkTimer = 1000;
            icicleTimer = 0;
            iceLanceTimer = 10000;
            shatteredIceTimer = 2000;
            iceTombTimer = 60000;
            frostflakeTimer = 5000;
            stormPillarTimer = 5000;
            endSpecialPhaseTimer = 0;

            introYell = 0;
            frozenCrystalKilled = 0;
            chargedConductorCount = 0;
            waveCount = 0;
            frostflakeGUID = 0;
            achievReqCount = 0;

            teleport = false;
            enrage = false;
            icePhaseEnded = false;

            summons.DespawnAll();
            scheduler.CancelAll();
            LightningStorm(true);

            ScriptedAI::Reset();
        }

        void JustSummoned(Creature* summon) override
        {
            summons.push_back(summon->GetGUID());
        }

        void EnterCombat(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_HAGARA, IN_PROGRESS);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            }

            RunPlayableQuote(aggro);
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
            {
                randomText = urand(0, MAX_KILL_QUOTES-1);
                RunPlayableQuote(killedUnit[randomText]);
            }
        }

        void JustDied(Unit * /*who*/) override
        {
            RunPlayableQuote(justDied);
            summons.DespawnAll();
            LightningStorm(true);

            if (instance)
            {
                instance->SetData(TYPE_BOSS_HAGARA, DONE);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                if (IsHeroic())
                    instance->DoCompleteAchievement(ACHIEVEMENT_HEROIC_HAGARA);

                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LIGHTNING_CONDUIT);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LIGHTNING_CONDUIT_10N);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LIGHTNING_CONDUIT_25N);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LIGHTNING_CONDUIT_10HC);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LIGHTNING_CONDUIT_25HC);

                instance->DoModifyPlayerCurrencies(CURRENCY_MOTE_OF_DARKNESS, 1, CURRENCY_SOURCE_OTHER);
            }
        }

        void DoAction(const int32 param) override
        {
            switch (param)
            {
            case ACTION_CRYSTAL_DIED:
            {
                if (frozenCrystalKilled < 3)
                {
                    randomText = urand(0, MAX_CRYSTAL_QUOTES-1);
                    RunPlayableQuote(crystalDied[randomText]);
                }
                frozenCrystalKilled++;
                break;
            }
            case ACTION_PREPARE_FOR_FIGHT:
            {
                if (Creature * pHoverDisc = me->FindNearestCreature(NPC_HOVER_DISC_VEHICLE, SEARCH_RANGE, true))
                    pHoverDisc->GetMotionMaster()->MovePoint(0, me->GetHomePosition());
                me->SendMovementFlagUpdate();
                scheduler.Schedule(Seconds(10), [this](TaskContext /* task context */)
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    me->SetReactState(REACT_AGGRESSIVE);
                    HoverDisc(false);
                });
                break;
            }
            case ACTION_CONDUCTOR_CHARGED:
            {
                chargedConductorCount++;
                if (chargedConductorCount == ((IsHeroic() && !Is25ManRaid()) ? MAX_CONDUCTORS_10HC : MAX_CONDUCTORS))
                    CheckForAchievementCriteria();

                randomText = urand(0, MAX_CONDUCTOR_QUOTES-1);
                RunPlayableQuote(conductorCharged[randomText]);
                break;
            }
            case DATA_HAGARA_INTRO_TRASH:
            {
                if (Creature * pHoverDisc = me->FindNearestCreature(NPC_HOVER_DISC_VEHICLE, SEARCH_RANGE, true))
                    pHoverDisc->SetVisible(true);
                me->SetVisible(true);

                // Skip trash when already defeated once
                if (instance->GetData(DATA_HAGARA_INTRO_TRASH) >= MAX_TRASH)
                {
                    me->AI()->DoAction(ACTION_PREPARE_FOR_FIGHT);
                    return;
                }

                if (instance->GetData(DATA_HAGARA_INTRO_TRASH) == 0)
                    SummonNPC(INTRO_FIRST_WAVE);
                else if (instance->GetData(DATA_HAGARA_INTRO_TRASH) == MAX_TRASH-31)
                    SummonNPC(INTRO_SECOND_WAVE);
                else if (instance->GetData(DATA_HAGARA_INTRO_TRASH) == MAX_TRASH-17)
                    SummonNPC(INTRO_THIRD_WAVE);
                else if (instance->GetData(DATA_HAGARA_INTRO_TRASH) == MAX_TRASH-11)
                    SummonNPC(INTRO_FOURTH_WAVE);
                else if (instance->GetData(DATA_HAGARA_INTRO_TRASH) == MAX_TRASH)
                    me->AI()->DoAction(ACTION_PREPARE_FOR_FIGHT);

                RunPlayableQuote(intro[introYell]);
                introYell++;
                break;
            }
            default:
                break;
            }
        }

        void Tele()
        {
            me->GetMotionMaster()->MoveJump(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY(), me->GetHomePosition().GetPositionZ(), 200.0f, 200.0f);
            me->NearTeleportTo(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY(), me->GetHomePosition().GetPositionZ(), me->GetHomePosition().GetOrientation());

            // Stay and stop attack
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);
            me->GetMotionMaster()->MoveIdle();
            me->AttackStop();
            me->SendMovementFlagUpdate();

            teleport = true;
        }

        void HoverDisc(bool enter = true)
        {
            if (enter == true)
            {
                if (Creature * pHoverDisc = me->SummonCreature(NPC_HOVER_DISC_VEHICLE, portalPos[FLY_POSITION], TEMPSUMMON_MANUAL_DESPAWN))
                {
                    pHoverDisc->SetVisible(false);
                    pHoverDisc->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    pHoverDisc->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    if (Vehicle * veh = pHoverDisc->GetVehicleKit())
                        me->EnterVehicle(veh, 0, nullptr);
                }
            }
            else
            {
                me->ExitVehicle();
                if (Creature * pHoverDisc = me->FindNearestCreature(NPC_HOVER_DISC_VEHICLE, SEARCH_RANGE, true))
                    pHoverDisc->DespawnOrUnsummon();
            }
        }

        void EquipCorrectAxes(uint32 nextPhase)
        {
            if (nextPhase == ICE_PHASE)
            {
                me->CastSpell(me, SPELL_HAGARA_FROST_AXES, true);
                me->LoadEquipment(ICE_AXE_ENTRY, true);
                // For some reason this is buggy - won`t display anything in hand sometimes
                //SetEquipmentSlots(false, HAGARA_FROST_AXES, HAGARA_FROST_AXES, EQUIP_NO_CHANGE);
            }
            else if (nextPhase == LIGHTNING_PHASE)
            {
                me->CastSpell(me, SPELL_HAGARA_LIGHTNING_AXES, true);
                me->LoadEquipment(LIGHNING_AXE_ENTRY, true);
                //SetEquipmentSlots(false, HAGARA_LIGHTNING_AXES, HAGARA_LIGHTNING_AXES, EQUIP_NO_CHANGE);
            }
            me->SetSheath(SHEATH_STATE_MELEE);
        }

        void CastSpellOnRandomPlayers(uint32 spellId, uint32 size, uint32 excludeAuraId = 0, bool triggered = true, bool ignoreTanks = false, bool ignoreHealers = false)
        {
            std::list<Player*> target_list;
            target_list.clear();
            Map * map = me->GetMap();

            if (!map)
                return;

            Map::PlayerList const& plrList = map->GetPlayers();
            if (plrList.isEmpty())
                return;

            for (Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
            {
                if (Player* pPlayer = itr->getSource())
                {
                    if (!pPlayer->HasAura(excludeAuraId) || excludeAuraId == 0)
                    {
                        if (pPlayer->IsAlive() && !pPlayer->IsGameMaster())
                        {
                            if (ignoreHealers == true && ignoreTanks == true)
                            {
                                if (!pPlayer->HasTankSpec() && !pPlayer->HasAura(5487) && !pPlayer->HasHealingSpec())
                                    target_list.push_back(pPlayer);
                            }
                            else if (ignoreTanks == true)
                            {
                                if (!pPlayer->HasTankSpec() && !pPlayer->HasAura(5487)) // Or bear form
                                    target_list.push_back(pPlayer);
                            }
                            else if (ignoreHealers == true)
                            {
                                if (!pPlayer->HasHealingSpec())
                                    target_list.push_back(pPlayer);
                            }
                            else
                                target_list.push_back(pPlayer);
                        }
                    }
                }
            }

            if (spellId != SPELL_ICE_LANCE)
            {
                for (uint32 i = 0; i < size; i++)
                {
                    if (target_list.empty())
                        break;

                    std::list<Player*>::iterator j = target_list.begin();
                    advance(j, rand() % target_list.size()); // Pick random target

                    if ((*j) && (*j)->IsInWorld())
                    {
                        me->CastSpell((*j), spellId, triggered);
                        target_list.erase(j);
                    }
                }
            }
            else // Only Ice Lance stuff
            {
                float angle = (rand() % 628) / 100;
                float distance = 45.0f;
                float angleAddition = (rand() % 35 + 170) / 100;

                for (uint32 j = 0; j < size; j++)
                {
                    if (target_list.empty())
                        break;

                    std::list<Player*>::iterator i = target_list.begin();
                    advance(i, rand() % target_list.size());

                    angle += angleAddition;
                    angle = MapManager::NormalizeOrientation(angle);
                    (*i)->SummonCreature(NPC_ICE_LANCE, SpawnPos[4][0] + cos(angle)*distance, SpawnPos[4][1] + sin(angle)*distance, SpawnPos[4][2] + 2.0f, angle, TEMPSUMMON_TIMED_DESPAWN, 15500);

                    if (target_list.size() >= size)
                    {
                        target_list.erase(i);
                        i = target_list.begin();
                    }
                }
            }
        }

        void SummonNPC(uint32 entry)
        {
            switch (entry)
            {
            case NPC_FROZEN_BINDING_CRYSTAL:
                for (uint32 i = 0; i < 4; i++)
                    me->SummonCreature(NPC_FROZEN_BINDING_CRYSTAL, SpawnPos[i][0], SpawnPos[i][1], SpawnPos[i][2], SpawnPos[i][3], TEMPSUMMON_CORPSE_DESPAWN);
                break;
            case NPC_ICE_WAVE:
            {
                float angle = me->GetOrientation();
                double angleAddition = ((2 * M_PI) / 4); // 90 Digrees 
                //angle = MapManager::NormalizeOrientation(angle);
                float distance = 5.0f;

                for (uint32 i = 0; i < 4; i++)
                {
                    Creature * ice_wave = me->SummonCreature(NPC_ICE_WAVE, me->GetPositionX() + cos(angle)*distance, me->GetPositionY() + sin(angle)*distance, me->GetPositionZ(), angle, TEMPSUMMON_MANUAL_DESPAWN);
                    if (ice_wave)
                        ice_wave->AI()->SetData(DATA_ICE_WAVE_SLOT, waveCount);

                    waveCount++;
                    angle = angle + angleAddition;
                }
                break;
            }
            case NPC_CRYSTAL_CONDUCTOR:
            {
                float angle = me->GetOrientation();
                double angleAddition = ((2 * M_PI) / 4); // 90 Digrees 
                angle = MapManager::NormalizeOrientation(angle);
                float distance, height, max;

                max = (getDifficulty() == RAID_DIFFICULTY_10MAN_HEROIC) ? MAX_CONDUCTORS_10HC : MAX_CONDUCTORS;

                for (uint32 i = 0; i < max; i++)
                {
                    if (i < MAX_CONDUCTORS)
                    {
                        distance = (getDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC) ? 45.0 : 32.0f;
                        height = 2.0f;
                    }
                    else
                    {
                        distance = 45.0f;
                        height = 2.5f;
                    }

                    me->SummonCreature(NPC_CRYSTAL_CONDUCTOR, me->GetPositionX() + cos(angle)*distance, me->GetPositionY() + sin(angle)*distance, me->GetPositionZ() + height, angle, TEMPSUMMON_MANUAL_DESPAWN);
                    if (i == MAX_CONDUCTORS-1)
                    {
                        angle = angle + (M_PI / 4);
                        angle = MapManager::NormalizeOrientation(angle);
                    }
                    angle = angle + angleAddition;
                }
                break;
            }
            case NPC_COLLAPSING_ICICLE:
            {
                float angleAddition = frand(0.0f, M_PI / 4.0f);
                float angle = M_PI + angleAddition;
                angle = MapManager::NormalizeOrientation(angle);
                lastIcicleAngle += angle;
                lastIcicleAngle = MapManager::NormalizeOrientation(lastIcicleAngle);
                float distance = urand(37, 45);
                me->SummonCreature(NPC_COLLAPSING_ICICLE, me->GetPositionX() + cos(lastIcicleAngle)*distance, me->GetPositionY() + sin(lastIcicleAngle)*distance, me->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 8000);
                break;
            }
            case NPC_ICE_LANCE:
                CastSpellOnRandomPlayers(SPELL_ICE_LANCE, 3, SPELL_ICE_TOMB_STUN, false, false, false);
                break;
            case NPC_BOUND_LIGHNING_ELEMENTAL:
                me->SummonCreature(NPC_BOUND_LIGHNING_ELEMENTAL, SpawnPos[5][0], SpawnPos[5][1], SpawnPos[5][2], SpawnPos[5][3], TEMPSUMMON_DEAD_DESPAWN);
                break;
            case INTRO_FIRST_WAVE:
            case INTRO_FOURTH_WAVE:
                me->SummonCreature(NPC_HAGARA_TRASH_PORTAL, portalPos[CENTER_PORTAL], TEMPSUMMON_TIMED_DESPAWN, 30000);
                break;
            case INTRO_SECOND_WAVE:
                me->SummonCreature(NPC_HAGARA_TRASH_PORTAL, portalPos[LEFT_PORTAL_NEAR], TEMPSUMMON_TIMED_DESPAWN, 30000);
                me->SummonCreature(NPC_HAGARA_TRASH_PORTAL, portalPos[RIGHT_PORTAL_NEAR], TEMPSUMMON_TIMED_DESPAWN, 30000);
                break;
            case INTRO_THIRD_WAVE:
                me->SummonCreature(NPC_HAGARA_TRASH_PORTAL, portalPos[LEFT_PORTAL_FAR], TEMPSUMMON_TIMED_DESPAWN, 30000);
                me->SummonCreature(NPC_HAGARA_TRASH_PORTAL, portalPos[RIGHT_PORTAL_FAR], TEMPSUMMON_TIMED_DESPAWN, 30000);
                break;
            default:
                break;
            }
        }

        void LightningStorm(bool end = false)
        {
            Map::PlayerList const &playerList = me->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
            {
                if (Player * pPlayer = i->getSource())
                {
                    if (end == false)
                        pPlayer->SummonCreature(NPC_LIGHTNING_STORM_TEST, pPlayer->GetPositionX(), pPlayer->GetPositionY(), pPlayer->GetPositionZ() + 30.0f, 0, TEMPSUMMON_DEAD_DESPAWN);
                }
            }

            if (end == true)
            {
                if (instance)
                {
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LIGHTNING_CONDUIT_10N);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LIGHTNING_CONDUIT_10HC);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LIGHTNING_CONDUIT_25N);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LIGHTNING_CONDUIT_25HC);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LIGHTNING_CONDUIT);
                }

                std::list<Creature*> lightning_storm;
                me->GetCreatureListWithEntryInGrid(lightning_storm, NPC_LIGHTNING_STORM_TEST, SEARCH_RANGE);
                for (std::list<Creature*>::iterator itr = lightning_storm.begin(); itr != lightning_storm.end(); ++itr)
                {
                    if (*itr)
                        (*itr)->DespawnOrUnsummon();
                }
            }
        }

        void CheckDistance()
        {
            Map::PlayerList const &playerList = me->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
            {
                if (Player * pPlayer = i->getSource())
                {
                    if (me->GetExactDist2d(pPlayer) < WATERY_ENTRENCHMENT_RANGE)
                        pPlayer->CastSpell(pPlayer, SPELL_WATERY_ENTRENCHMENT, false);
                    else
                        pPlayer->RemoveAura(SPELL_WATERY_ENTRENCHMENT);

                    if (pPlayer->HasAura(SPELL_FROSTFLAKE))
                    {
                        if (Aura * pAura = pPlayer->GetAura(SPELL_FROSTFLAKE))
                        {
                            if (pAura->GetStackAmount() < FROSTFLAKE_MAX_STACK)
                                pAura->SetStackAmount(pAura->GetStackAmount() + 1);
                        }
                    }
                }
            }
        }

        void CheckForAchievementCriteria()
        {
            achievReqCount = 0;

            Map::PlayerList const &playerList = me->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
            {
                if (Player * pPlayer = i->getSource())
                {
                    if (pPlayer->HasAura(SPELL_LIGHTNING_CONDUIT))
                        achievReqCount++;
                }
            }

            if (instance && achievReqCount == (Is25ManRaid() ? MAX_PLAYERS_25MAN : MAX_PLAYERS_10MAN))
                instance->DoCompleteAchievement(ACHIEVEMENT_HOLDING_HANDS);
        }

        void SpellHitTarget(Unit* target, const SpellEntry* spell) override
        {
            if (spell->Id == SPELL_ICE_TOMB_MISSILE)
            {
                if (target)
                    me->CastSpell(target, SPELL_ICE_TOMB_STUN, true); // Stun and dmg
            }

            if (spell->Id == SPELL_ICE_TOMB_STUN)
            {
                if (target)
                {
                    if (Creature * pIceTomb = target->SummonCreature(NPC_ICE_TOMB, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0.0f))
                    {
                        target->NearTeleportTo(pIceTomb->GetPositionX(), pIceTomb->GetPositionY(), pIceTomb->GetPositionZ(), target->GetOrientation(), false);
                        pIceTomb->CastSpell(target, SPELL_ICE_TOMB_EFFECT, true);
                    }
                }
            }
        }

        void EndSpecialPhase()
        {
            me->RemoveAurasDueToSpell(SPELL_CRYSTALLINE_TETHER);
            me->RemoveAurasDueToSpell(SPELL_FROZEN_TEMPEST_10N);
            me->RemoveAurasDueToSpell(SPELL_FROZEN_TEMPEST_10HC);
            me->RemoveAurasDueToSpell(SPELL_FROZEN_TEMPEST_25N);
            me->RemoveAurasDueToSpell(SPELL_FROZEN_TEMPEST_25HC);
            me->RemoveAurasDueToSpell(SPELL_WATER_SHIELD_10N);
            me->RemoveAurasDueToSpell(SPELL_WATER_SHIELD_10HC);
            me->RemoveAurasDueToSpell(SPELL_WATER_SHIELD_25N);
            me->RemoveAurasDueToSpell(SPELL_WATER_SHIELD_25HC);
            summons.DespawnAll();

            normalPhaseTimer = 62000;
            focusedAssaultTimer = 16000;
            shatteredIceTimer = 17000;
            iceLanceTimer = 12000;
            iceTombTimer = 20000;
            teleport = false;

            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_AGGRESSIVE);
            me->GetMotionMaster()->MoveChase(me->GetVictim());
            me->Attack(me->GetVictim(), true);
            me->SendMovementFlagUpdate();

            if (instance)
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_WATERY_ENTRENCHMENT);

            LightningStorm(true);
            EquipCorrectAxes(nextPhase);
            phase = NORMAL_PHASE;
            me->CastSpell(me, SPELL_FEEDBACK, false);

            randomText = urand(0, MAX_OTHER_QUOTES-1);
            RunPlayableQuote(specialPhaseEnd[randomText]);
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (!UpdateVictim())
                return;

            // NORMAL PHASE
            if (phase == NORMAL_PHASE)
            {
                // Focused Assault
                if (focusedAssaultTimer <= diff)
                {
                    me->CastSpell(me->GetVictim(), SPELL_FOCUSED_ASSAULT_10N, true);
                    focusedAssaultTimer = 15000;
                    shatteredIceTimer += 5000;
                }
                else focusedAssaultTimer -= diff;

                // Ice Tomb
                if (iceTombTimer <= diff)
                {
                    uint32 max = RAID_MODE(2, 5, 2, 6);
                    CastSpellOnRandomPlayers(SPELL_ICE_TOMB_MISSILE, max, 0, true, true, false);

                    randomText = urand(0, MAX_OTHER_QUOTES-1);
                    RunPlayableQuote(iceTomb[randomText]);
                    
                    iceTombTimer = 60000;
                }
                else iceTombTimer -= diff;

                // Ice Lance
                if (iceLanceTimer <= diff)
                {
                    SummonNPC(NPC_ICE_LANCE);
                    iceLanceTimer = 30000;
                    randomText = urand(0, MAX_ICE_LANCE_QUOTES-1);
                    RunPlayableQuote(iceLance[randomText]);
                }
                else iceLanceTimer -= diff;

                // Shattered Ice
                if (shatteredIceTimer <= diff)
                {
                    Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, SEARCH_RANGE, true);
                    if (target)
                        me->CastSpell(target, SPELL_SHATTERED_ICE_10N, false);
                    shatteredIceTimer = urand(10500, 15000);
                }
                else shatteredIceTimer -= diff;

                // Melee attack
                DoMeleeAttackIfReady();

                // Switch Phase
                if (normalPhaseTimer <= diff)
                {
                    endSpecialPhaseTimer = 300000;
                    if (nextPhase == ICE_PHASE)
                    {
                        phase = ICE_PHASE;
                        waveCount = 0;
                        nextPhase = LIGHTNING_PHASE;
                    }
                    else if (nextPhase == LIGHTNING_PHASE)
                    {
                        phase = LIGHTNING_PHASE;
                        nextPhase = ICE_PHASE;
                    }
                }
                else normalPhaseTimer -= diff;
            }

            // ICE PHASE
            else if (phase == ICE_PHASE)
            {
                if (!teleport)
                {
                    Tele();
                    icicleTimer = 2000;
                    checkTimer = 5000;
                    frostflakeTimer = 5000;
                    icePhaseEnded = false;

                    me->CastSpell(me, SPELL_FROZEN_TEMPEST_10N, false);

                    scheduler.Schedule(Seconds(1), [this](TaskContext icePhase)
                    {
                        if (icePhase.GetRepeatCounter() == 0)
                        {
                            SummonNPC(NPC_FROZEN_BINDING_CRYSTAL);
                            me->CastSpell(me, SPELL_TELEPORT_VISUAL, true);
                            icePhase.Repeat(Seconds(9));
                        }
                        else if (icePhase.GetRepeatCounter() == 1)
                        {
                            randomText = urand(0, MAX_OTHER_QUOTES-1);
                            RunPlayableQuote(iceWave[randomText]);
                            SummonNPC(NPC_ICE_WAVE);
                            icePhase.Repeat(Seconds(1));
                        }
                        else if (icePhase.GetRepeatCounter() == 2 || icePhase.GetRepeatCounter() == 3)
                        {
                            SummonNPC(NPC_ICE_WAVE);
                            icePhase.Repeat(Seconds(1));
                        }
                    });
                }

                // Icicle
                if (icicleTimer <= diff)
                {
                    SummonNPC(NPC_COLLAPSING_ICICLE);
                    icicleTimer = 1500;
                }
                else icicleTimer -= diff;

                if (checkTimer <= diff)
                {
                    CheckDistance();
                    checkTimer = 1000;
                }
                else checkTimer -= diff;

                // Frostflake - heroic ability
                if (IsHeroic())
                {
                    if (frostflakeTimer <= diff)
                    {
                        frostflakeTimer = 5000;
                        CastSpellOnRandomPlayers(SPELL_FROSTFLAKE, 1, SPELL_FROSTFLAKE, true);
                    }
                    else frostflakeTimer -= diff;
                }

                // End of Ice Phase
                if ((frozenCrystalKilled % MAX_CONDUCTORS) == 0 && frozenCrystalKilled != 0)
                {
                    EndSpecialPhase();
                    frozenCrystalKilled = 0;
                }
            }

            // LIGHTNING PHASE
            else if (phase == LIGHTNING_PHASE)
            {
                if (!teleport)
                {
                    Tele();
                    LightningStorm(false);

                    me->CastSpell(me, SPELL_WATER_SHIELD_10N, false);

                    randomText = urand(0, MAX_OTHER_QUOTES-1);
                    RunPlayableQuote(storm[randomText]);

                    scheduler.Schedule(Seconds(1), [this](TaskContext /* Task context */)
                    {
                        SummonNPC(NPC_CRYSTAL_CONDUCTOR);
                        SummonNPC(NPC_BOUND_LIGHNING_ELEMENTAL);
                        me->CastSpell(me, SPELL_TELEPORT_VISUAL, true);
                    });
                }

                // Storm Pillar - heroic ability
                if (IsHeroic())
                {
                    if (stormPillarTimer <= diff)
                    {
                        Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, SEARCH_RANGE, true);
                        if (target)
                            me->CastSpell(target, SPELL_STORM_PILLAR, true);

                        stormPillarTimer = 5000;
                    }
                    else stormPillarTimer -= diff;
                }

                if (icePhaseEnded == false)
                {
                    // End of Lightning Phase
                    if (chargedConductorCount == ((IsHeroic() && !Is25ManRaid()) ? MAX_CONDUCTORS_10HC : MAX_CONDUCTORS))
                    {
                        // Delay phase end by 2s
                        scheduler.Schedule(Seconds(2), [this](TaskContext /* Task context */)
                        {
                            EndSpecialPhase();
                            chargedConductorCount = 0;
                        });
                        icePhaseEnded = true;
                    }
                }
            }

            // End special phase after 5 mins when players don`t kill or overload crystals
            if (phase == ICE_PHASE || phase == LIGHTNING_PHASE)
            {
                if (endSpecialPhaseTimer <= diff)
                {
                    EndSpecialPhase();
                }
                else endSpecialPhaseTimer -= diff;
            }

            // Enrage
            if (!enrage)
            {
                if (enrageTimer <= diff)
                {
                    me->CastSpell(me, SPELL_ENRAGE, false);
                    enrage = true;
                }
                else enrageTimer -= diff;
            }
        }
    };
};

class npc_ice_lance : public CreatureScript
{
public:
    npc_ice_lance() : CreatureScript("npc_ice_lance") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ice_lanceAI(pCreature);
    }

    struct npc_ice_lanceAI : public ScriptedAI
    {
        npc_ice_lanceAI(Creature *creature) : ScriptedAI(creature) {}

        uint32 dummyAuraTimer;
        uint32 iceLanceTimer;

        uint64 summonerGuid;
        std::vector<Dist> dist;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);
            dummyAuraTimer = 500;
            iceLanceTimer = 0;
            summonerGuid = 0;

            me->SetInCombatWithZone();
        }

        void SpellHitTarget(Unit* target, const SpellEntry* spell) override
        {
            if (spell->Id == SPELL_ICE_LANCE_MISSILE)
                if (target)
                    me->CastSpell(target, SPELL_ICE_LANCE_10N, true);
        }

        void IsSummonedBy(Unit* pSummoner) override
        {
            summonerGuid = pSummoner->GetGUID();
        }

        void UpdateAI(const uint32 diff) override
        {
            if (dummyAuraTimer <= diff)
            {
                me->CastSpell(me, SPELL_ICE_LANCE_DUMMY, true);
                dummyAuraTimer = 16000;

                if (Player* pPlayer = ObjectAccessor::GetPlayer(*me, summonerGuid))
                    me->CastSpell(pPlayer, SPELL_TARGET, true);
            }
            else dummyAuraTimer -= diff;

            if (iceLanceTimer <= diff)
            {
                Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                if (Player* pPlayer = ObjectAccessor::GetPlayer(*me, summonerGuid))
                {
                    for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                    {
                        if (Player* pl = i->getSource())
                        {
                            // Find out if any player is in between Ice Lance and player with Target buff
                            if ((pl)->IsInBetween(me, pPlayer, 2.0f) == true && pl->IsAlive())
                            {
                                // Save player's distance from Ice Lance and guid
                                Dist from_me;
                                from_me.distance = pl->GetExactDist2d(me);
                                from_me.guid = pl->GetGUID();
                                dist.push_back(from_me);
                            }
                        }
                    }
                    // If there are some players
                    if (dist.size() != 0)
                    {
                        float minimum = SEARCH_RANGE; // Player wouldn`t be farther than 100yd from boss
                        uint64 player_guid = 0;
                        for (uint8 i = 0; i < dist.size(); i++)
                        {
                            if (dist[i].distance < minimum)
                            {
                                // Find and save the smallest distance and player`s guid
                                minimum = dist[i].distance;
                                player_guid = dist[i].guid;
                            }
                        }
                        // Find correct player with right guid and cast Ice Lance
                        if (Player* pPlayer = ObjectAccessor::GetPlayer(*me, player_guid))
                            me->CastSpell(pPlayer, SPELL_ICE_LANCE_MISSILE, true);
                        dist.clear();
                    }
                    else
                        me->CastSpell(pPlayer, SPELL_ICE_LANCE_MISSILE, true);
                }
                iceLanceTimer = 1000;
            }
            else iceLanceTimer -= diff;
        }
    };
};

class npc_ice_tomb : public CreatureScript
{
public:
    npc_ice_tomb() : CreatureScript("npc_ice_tomb") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ice_tombAI(pCreature);
    }

    struct npc_ice_tombAI : public ScriptedAI
    {
        npc_ice_tombAI(Creature *creature) : ScriptedAI(creature) 
        {
            summonerGuid = 0;
        }

        uint64 summonerGuid;
        uint32 checkAliveTimer;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);
            checkAliveTimer = 5000;
        }

        void IsSummonedBy(Unit* pSummoner) override
        {
            summonerGuid = pSummoner->GetGUID();
        }

        void JustDied(Unit* who) override
        {
            if (Player* pPlayer = ObjectAccessor::GetPlayer(*me, summonerGuid))
            {
                if (pPlayer->HasAura(SPELL_ICE_TOMB_STUN))
                {
                    pPlayer->RemoveAura(SPELL_ICE_TOMB_STUN);
                    pPlayer->RemoveAura(SPELL_ICE_TOMB_EFFECT);
                    me->DespawnOrUnsummon();
                }
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (checkAliveTimer <= diff)
            {
                if (Player* pPlayer = ObjectAccessor::GetPlayer(*me, summonerGuid))
                {
                    if (!pPlayer->IsAlive())
                    {
                        pPlayer->RemoveAura(SPELL_ICE_TOMB_STUN);
                        pPlayer->RemoveAura(SPELL_ICE_TOMB_EFFECT);
                        me->DespawnOrUnsummon();
                    }
                }
                checkAliveTimer = 1000;
            }
            else checkAliveTimer -= diff;
        }
    };
};

class npc_frozen_binding_crystal : public CreatureScript
{
public:
    npc_frozen_binding_crystal() : CreatureScript("npc_frozen_binding_crystal") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_frozen_binding_crystalAI(pCreature);
    }

    struct npc_frozen_binding_crystalAI : public ScriptedAI
    {
        npc_frozen_binding_crystalAI(Creature *creature) : ScriptedAI(creature) {}

        void JustDied(Unit* /*who*/) override
        {
            Unit * hagara = me->FindNearestCreature(BOSS_HAGARA_THE_STORMBINDER, SEARCH_RANGE, true);
            if (hagara)
                hagara->GetAI()->DoAction(ACTION_CRYSTAL_DIED);
        }

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);

            Unit * hagara = me->FindNearestCreature(BOSS_HAGARA_THE_STORMBINDER, SEARCH_RANGE, true);
            if (hagara)
                me->CastSpell(hagara, SPELL_CRYSTALLINE_TETHER, false);
        }
    };
};

class npc_collapsing_icicle : public CreatureScript
{
public:
    npc_collapsing_icicle() : CreatureScript("npc_collapsing_icicle") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_collapsing_icicleAI(pCreature);
    }

    struct npc_collapsing_icicleAI : public ScriptedAI
    {
        npc_collapsing_icicleAI(Creature *creature) : ScriptedAI(creature) {}

        uint32 fallTimer;
        uint32 shatterTimer;
        uint32 knockbackTimer;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, SPELL_ICICLE, false);
            shatterTimer = 5000;
            knockbackTimer = 7000;
        }

        void UpdateAI(const uint32 diff) override
        {
            if (shatterTimer <= diff)
            {
                me->CastSpell(me, SPELL_ICICLE_FALL_DUMMY, false);
                shatterTimer = 10000;
            }
            else shatterTimer -= diff;

            if (knockbackTimer <= diff)
            {
                int bp0 = 100000; // Should deal 100k dmg
                me->CastCustomSpell(me, SPELL_ICE_SHARDS, &bp0, 0, 0, true);
                knockbackTimer = 10000;
            }
            else knockbackTimer -= diff;
        }
    };
};

class npc_ice_wave : public CreatureScript
{
public:
    npc_ice_wave() : CreatureScript("npc_ice_wave") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ice_waveAI(pCreature);
    }

    struct npc_ice_waveAI : public ScriptedAI
    {
        npc_ice_waveAI(Creature *creature) : ScriptedAI(creature)
        {
            initialize = false;
            slot = 0;

            if (Unit * pHagara = me->FindNearestCreature(BOSS_HAGARA_THE_STORMBINDER, SEARCH_RANGE, true))
            {
                sx = pHagara->GetPositionX();
                sy = pHagara->GetPositionY();
                i = pHagara->GetOrientation();
            }
            me->SetSpeed(MOVE_RUN, 1.0f);
        }

        float distance;
        float i;
        float sx;
        float sy;

        bool initialize;

        uint32 slot;
        uint32 moveTimer;
        bool firstPoint;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, SPELL_ICE_WAVE_AURA, false);
            firstPoint = false;
        }

        void SetData(uint32 Type, uint32 Data)
        {
            if (Type == DATA_ICE_WAVE_SLOT)
            {
                slot = Data;
                initialize = true;
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!initialize)
                return;

            if (initialize && !firstPoint)
            {
                switch (slot)
                {
                case 0:
                    distance = 51.0f;
                    moveTimer = 8000;
                    i = 0.0f;
                    break;
                case 1:
                    distance = 51.0f;
                    moveTimer = 8000;
                    i = M_PI / 2;
                    break;
                case 2:
                    distance = 51.0f;
                    moveTimer = 8000;
                    i = M_PI;
                    break;
                case 3:
                    distance = 51.0f;
                    moveTimer = 8000;
                    i = ((3 * M_PI) / 2);
                    break;
                case 4:
                    distance = 42.0f;
                    moveTimer = 7000;
                    i = 0.0f;
                    break;
                case 5:
                    distance = 42.0f;
                    moveTimer = 7000;
                    i = M_PI / 2;
                    break;
                case 6:
                    distance = 42.0f;
                    moveTimer = 7000;
                    i = M_PI;
                    break;
                case 7:
                    distance = 42.0f;
                    moveTimer = 7000;
                    i = ((3 * M_PI) / 2);
                    break;
                case 8:
                    distance = 34.0f;
                    moveTimer = 6000;
                    i = 0.0f;
                    break;
                case 9:
                    distance = 34.0f;
                    moveTimer = 6000;
                    i = M_PI / 2;
                    break;
                case 10:
                    distance = 34.0f;
                    moveTimer = 6000;
                    i = M_PI;
                    break;
                case 11:
                    distance = 34.0f;
                    moveTimer = 6000;
                    i = ((3 * M_PI) / 2);
                    break;
                }
                if (slot <= 3)
                    me->GetMotionMaster()->MovePoint(0, sx + (distance*cos(i)), sy + distance*sin(i), me->GetPositionZ()+1);
                else 
                    me->GetMotionMaster()->MovePoint(0, sx + (distance*cos(i)), sy + distance*sin(i), me->GetPositionZ());
                firstPoint = true;
            }

            if (moveTimer <= diff)
            {
                switch (uint32(distance))
                {
                case 34:
                    moveTimer = 500;
                    me->SetSpeed(MOVE_RUN, 1.0f);
                    break;
                case 42:
                    moveTimer = 500;
                    me->SetSpeed(MOVE_RUN, 1.20f);
                    break;
                case 51:
                    moveTimer = 500;
                    me->SetSpeed(MOVE_RUN, 1.40f);
                    break;
                default:
                    break;
                }
                i = i - (2 * M_PI / 76);
                me->GetMotionMaster()->MovePoint(0, sx + (distance*cos(i)), (sy + distance*sin(i)), me->GetPositionZ());
            }
            else moveTimer -= diff;
        }
    };
};

class npc_crystal_conductor : public CreatureScript
{
public:
    npc_crystal_conductor() : CreatureScript("npc_crystal_conductor") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_crystal_conductorAI(pCreature);
    }

    struct npc_crystal_conductorAI : public ScriptedAI
    {
        npc_crystal_conductorAI(Creature *creature) : ScriptedAI(creature) {}

        uint32 lightningConduitTimer;
        uint32 lightningConduitDmg;
        uint32 targetCount;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);

            me->CastSpell(me, SPELL_LIGHTNING_ROD, false);
            lightningConduitTimer = 1000;
            targetCount = 0;

            Unit * hagara = me->FindNearestCreature(BOSS_HAGARA_THE_STORMBINDER, SEARCH_RANGE, true);
            if (hagara)
                me->CastSpell(hagara, SPELL_CRYSTALLINE_TETHER, false);
        }

        void SwitchOn()
        {
            me->RemoveAura(SPELL_LIGHTNING_CONDUIT);

            targetCount = 0;

            std::set<Player*> traversedPlayers;
            std::stack<Player*> conduitStack;

            Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
            {
                if (Player* player = i->getSource())
                {
                    if (player->IsAlive() && player->GetExactDist2d(me) < CONDUIT_RANGE)
                    {
                        if (!player->HasAura(SPELL_LIGHTNING_CONDUIT) && traversedPlayers.find(player) == traversedPlayers.end())
                        {
                            me->CastSpell(player, SPELL_LIGHTNING_CONDUIT, true);
                            player->CastSpell(player, SPELL_LIGHTNING_CONDUIT_10N, true);
                            conduitStack.push(player);
                            traversedPlayers.insert(player);
                        }
                        targetCount++;
                    }
                }
            }

            Player* source;

            while (!conduitStack.empty())
            {
                source = conduitStack.top();
                conduitStack.pop();

                for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                {
                    if (Player* player = i->getSource())
                    {
                        if (traversedPlayers.find(player) != traversedPlayers.end())
                            continue;

                        if (player->HasAura(SPELL_LIGHTNING_CONDUIT))
                            continue;

                        if (player->GetExactDist2d(source) < CONDUIT_RANGE)
                        {
                            source->CastSpell(player, SPELL_LIGHTNING_CONDUIT, true);
                            player->CastSpell(player, SPELL_LIGHTNING_CONDUIT_10N, true);
                            traversedPlayers.insert(player);
                            conduitStack.push(player);
                        }
                    }
                }
            }

            if (targetCount == 0)
            {
                for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                {
                    if (Player* player = i->getSource())
                    {
                        if (player->GetExactDist2d(me) < (CONDUIT_RANGE * 3))
                        {
                            player->RemoveAura(SPELL_LIGHTNING_CONDUIT);
                            player->RemoveAura(SPELL_LIGHTNING_CONDUIT_10N);
                            player->RemoveAura(SPELL_LIGHTNING_CONDUIT_10HC);
                            player->RemoveAura(SPELL_LIGHTNING_CONDUIT_25N);
                            player->RemoveAura(SPELL_LIGHTNING_CONDUIT_25HC);
                        }
                    }
                }
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (me->HasAura(SPELL_LIGHTNING_ROD_DUMMY))
            {
                if (lightningConduitTimer <= diff)
                {
                    SwitchOn();
                    lightningConduitTimer = 1000;
                }
                else lightningConduitTimer -= diff;
            }
        }
    };
};

class npc_bound_lightning_elemental : public CreatureScript
{
public:
    npc_bound_lightning_elemental() : CreatureScript("npc_bound_lightning_elemental") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_bound_lightning_elementalAI(pCreature);
    }

    struct npc_bound_lightning_elementalAI : public ScriptedAI
    {
        npc_bound_lightning_elementalAI(Creature *creature) : ScriptedAI(creature) {}

        void JustDied(Unit* /*who*/) override
        {
            if (Unit * pConductor = me->FindNearestCreature(NPC_CRYSTAL_CONDUCTOR, CONDUIT_RANGE, true))
                pConductor->CastSpell(pConductor, SPELL_LIGHTNING_ROD_DUMMY, false);
            if (Creature * pHagara = me->FindNearestCreature(BOSS_HAGARA_THE_STORMBINDER, SEARCH_RANGE, true))
                pHagara->AI()->DoAction(ACTION_CONDUCTOR_CHARGED);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_test_lightning_storm : public CreatureScript
{
public:
    npc_test_lightning_storm() : CreatureScript("npc_test_lightning_storm") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_test_lightning_stormAI(pCreature);
    }

    struct npc_test_lightning_stormAI : public ScriptedAI
    {
        npc_test_lightning_stormAI(Creature *creature) : ScriptedAI(creature) {}

        uint64 summonerGuid;
        uint32 repeatTimer;
        uint32 moveTimer;
        uint32 unauraTimer;

        void IsSummonedBy(Unit* pSummoner) override
        {
            summonerGuid = pSummoner->GetGUID();
        }

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);

            summonerGuid = 0;
            repeatTimer = 4100;
            unauraTimer = 5100;
            moveTimer = 500;
        }

        void UpdateAI(const uint32 diff) override
        {
            if (repeatTimer <= diff)
            {
                if (Player * pPlayer = ObjectAccessor::GetPlayer(*me, summonerGuid))
                {
                    if (!pPlayer->HasAura(SPELL_LIGHTNING_CONDUIT))
                        me->CastSpell(pPlayer, SPELL_LIGHTNING_STORM_COSMETIC, false);
                }

                repeatTimer = 2100;
                unauraTimer = 1100;
            }
            else repeatTimer -= diff;

            if (unauraTimer <= diff)
            {
                if (Player * pPlayer = ObjectAccessor::GetPlayer(*me, summonerGuid))
                    pPlayer->RemoveAura(SPELL_LIGHTNING_STORM_COSMETIC);
                unauraTimer = 2100;
            }
            else unauraTimer -= diff;

            if (moveTimer <= diff)
            {
                if (Player * pPlayer = ObjectAccessor::GetPlayer(*me, summonerGuid))
                    me->GetMotionMaster()->MovePoint(0, pPlayer->GetPositionX(), pPlayer->GetPositionY(), pPlayer->GetPositionZ() + 30, true);
                moveTimer = 500;
            }
            else moveTimer -= diff;
        }
    };
};

class npc_frostflake_snare : public CreatureScript
{
public:
    npc_frostflake_snare() : CreatureScript("npc_frostflake_snare") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_frostflake_snareAI(pCreature);
    }

    struct npc_frostflake_snareAI : public ScriptedAI
    {
        npc_frostflake_snareAI(Creature *creature) : ScriptedAI(creature) {}

        uint32 snareTimer;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, SPELL_FROSTFLAKE_SNARE, false);
            snareTimer = 30000;
        }

        void UpdateAI(const uint32 diff) override
        {
            if (snareTimer <= diff)
            {
                me->CastSpell(me, SPELL_FROSTFLAKE_SNARE, false);
                snareTimer = 30000;
            }
            else snareTimer -= diff;
        }
    };
};

class spell_ds_frostflake : public SpellScriptLoader
{
public:
    spell_ds_frostflake() : SpellScriptLoader("spell_ds_frostflake") { }

    class spell_ds_frostflake_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_ds_frostflake_AuraScript);

        void RemoveBuff(AuraEffect const* aurEff, AuraEffectHandleModes mode)
        {
            Unit * pTarget = GetTarget();
            if (!pTarget)
                return;

            if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_DEFAULT)
            {
                if (!pTarget->HasAura(SPELL_WATERY_ENTRENCHMENT))
                {
                    GetCaster()->SummonCreature(NPC_FROSTFLAKE_SNARE, pTarget->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 60000);
                }
            }
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_ds_frostflake_AuraScript::RemoveBuff, EFFECT_0, SPELL_AURA_MOD_DECREASE_SPEED, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_ds_frostflake_AuraScript();
    }
};

class spell_ds_focused_assault : public SpellScriptLoader
{
public:
    spell_ds_focused_assault() : SpellScriptLoader("spell_ds_focused_assault") { }

    class spell_ds_focused_assault_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_ds_focused_assault_AuraScript);

        void OnPeriodic(AuraEffect const* /*aurEff*/)
        {
            Unit * caster = GetCaster();
            if (!caster)
                return;

            int32 bp0 = 0;
            if (caster->HasAura(SPELL_FOCUSED_ASSAULT_10N) || caster->HasAura(SPELL_FOCUSED_ASSAULT_25N))
                bp0 = urand(23000, 27000);
            if (caster->HasAura(SPELL_FOCUSED_ASSAULT_10HC) || caster->HasAura(SPELL_FOCUSED_ASSAULT_25HC))
                bp0 = urand(43000, 47000);

            if (Unit * pTarget = caster->GetVictim())
            {
                 if (pTarget->IsWithinMeleeRange(caster, HAGARA_MELEE_RANGE))
                     caster->CastCustomSpell(pTarget, SPELL_FOCUSED_ASSAULT, &bp0, 0, 0, true);
                 else
                 {
                     if (caster->HasAura(SPELL_FOCUSED_ASSAULT_10N) || caster->HasAura(SPELL_FOCUSED_ASSAULT_25N))
                         caster->InterruptNonMeleeSpells(false);
                 }
            }
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_ds_focused_assault_AuraScript::OnPeriodic, EFFECT_1, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_ds_focused_assault_AuraScript();
    }
};

class spell_ds_ice_lance_dmg : public SpellScriptLoader
{
public:
    spell_ds_ice_lance_dmg() : SpellScriptLoader("spell_ds_ice_lance_dmg") {}

    class spell_ds_ice_lance_dmg_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_ice_lance_dmg_SpellScript);

        void HandleScript()
        {
            Unit * hitUnit = GetHitUnit();
            Unit * caster = GetCaster();

            if (!hitUnit || !caster)
                return;

            uint32 stack = 0;
            uint32 auraId = 0;

            if (hitUnit->HasAura(SPELL_ICE_LANCE_10N))
                auraId = SPELL_ICE_LANCE_10N;
            else if (hitUnit->HasAura(SPELL_ICE_LANCE_25N))
                auraId = SPELL_ICE_LANCE_25N;
            else if (hitUnit->HasAura(SPELL_ICE_LANCE_10HC))
                auraId = SPELL_ICE_LANCE_10HC;
            else if (hitUnit->HasAura(SPELL_ICE_LANCE_25HC))
                auraId = SPELL_ICE_LANCE_25HC;

            Aura * pOldAura = hitUnit->GetAura(auraId);
            if (pOldAura)
            {
                stack = pOldAura->GetStackAmount();
                pOldAura->Remove(AURA_REMOVE_BY_DEFAULT);
                if (Aura * pNewAura = caster->AddAura(auraId, hitUnit))
                    pNewAura->SetStackAmount(stack);
            }
        }

        void Register()
        {
            BeforeHit += SpellHitFn(spell_ds_ice_lance_dmg_SpellScript::HandleScript);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_ice_lance_dmg_SpellScript();
    }
};

class HasLightningConduit
{
public:
    bool operator()(WorldObject* object) const
    {
        if (Player * player = object->ToPlayer())
        {
            if (player->HasAura(SPELL_LIGHTNING_CONDUIT_10N) ||
                player->HasAura(SPELL_LIGHTNING_CONDUIT_25N) ||
                player->HasAura(SPELL_LIGHTNING_CONDUIT_10HC) ||
                player->HasAura(SPELL_LIGHTNING_CONDUIT_25HC))
                return true;
        }
        return false;
    }
};

class spell_ds_hagara_lightning_storm : public SpellScriptLoader
{
public:
    spell_ds_hagara_lightning_storm() : SpellScriptLoader("spell_ds_hagara_lightning_storm") { }

    class spell_ds_hagara_lightning_storm_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_hagara_lightning_storm_SpellScript);

        void HandleSelection(std::list<Unit*>& unitList)
        {
            if (!GetCaster())
                return;

            unitList.remove_if(HasLightningConduit());
        }

        void Register() override
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_hagara_lightning_storm_SpellScript::HandleSelection, EFFECT_1, TARGET_UNIT_AREA_ENEMY_DST);
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_hagara_lightning_storm_SpellScript::HandleSelection, EFFECT_2, TARGET_UNIT_AREA_ENEMY_DST);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_ds_hagara_lightning_storm_SpellScript();
    }
};

class spell_ds_lightning_conduit : public SpellScriptLoader
{
public:
    spell_ds_lightning_conduit() : SpellScriptLoader("spell_ds_lightning_conduit") {}

    class spell_ds_lightning_conduit_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_lightning_conduit_SpellScript);

        void HandleScript()
        {
            Unit * hitUnit = GetHitUnit();
            Unit * caster = GetCaster();

            if (!caster || !hitUnit)
                return;

            if (hitUnit->GetTypeId() != TYPEID_PLAYER)
                return;

            if (Creature* pCrystal = hitUnit->FindNearestCreature(NPC_CRYSTAL_CONDUCTOR, CONDUIT_RANGE, true))
            {
                if (!pCrystal->HasAura(SPELL_LIGHTNING_ROD_DUMMY))
                {
                    hitUnit->CastSpell(pCrystal, SPELL_LIGHTNING_CONDUIT, true);
                    pCrystal->CastSpell(pCrystal, SPELL_LIGHTNING_ROD_DUMMY, false);

                    if (Creature * pHagara = hitUnit->FindNearestCreature(BOSS_HAGARA_THE_STORMBINDER, SEARCH_RANGE, true))
                        pHagara->AI()->DoAction(ACTION_CONDUCTOR_CHARGED);
                }
            }
        }

        void Register()
        {
            AfterHit += SpellHitFn(spell_ds_lightning_conduit_SpellScript::HandleScript);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_lightning_conduit_SpellScript();
    }
};

class spell_ds_lightning_conduit_aura : public SpellScriptLoader
{
public:
    spell_ds_lightning_conduit_aura() : SpellScriptLoader("spell_ds_lightning_conduit_aura") { }

    class spell_ds_lightning_conduit_aura_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_ds_lightning_conduit_aura_AuraScript);

        uint32 targetCount;

        void OnPeriodic(AuraEffect const* /*aurEff*/)
        {
            Unit * caster = GetCaster();
            if (!caster)
                return;

            targetCount = 0;

            // At frist drop the aura if original caster is 10yd away from me
            if (Aura * pAura = caster->GetAura(SPELL_LIGHTNING_CONDUIT))
            {
                if (Unit * pAuraCaster = pAura->GetCaster())
                {
                    if (caster->GetDistance2d(pAuraCaster) > CONDUIT_RANGE)
                        caster->RemoveAura(SPELL_LIGHTNING_CONDUIT);
                }
            }

            // Now check if conductor is in range
            if (Creature* pCrystal = caster->FindNearestCreature(NPC_CRYSTAL_CONDUCTOR, CONDUIT_RANGE, true))
            {
                // If not overloaded, overload it
                if (!pCrystal->HasAura(SPELL_LIGHTNING_ROD_DUMMY))
                {
                    caster->CastSpell(pCrystal, SPELL_LIGHTNING_CONDUIT, true);
                    pCrystal->CastSpell(pCrystal, SPELL_LIGHTNING_ROD_DUMMY, false);

                    if (Creature * pHagara = pCrystal->FindNearestCreature(BOSS_HAGARA_THE_STORMBINDER, SEARCH_RANGE, true))
                        pHagara->AI()->DoAction(ACTION_CONDUCTOR_CHARGED);
                }
                // Else count as charged target in conduit range
                else
                    targetCount++;
            }

            // Here check if another charged player is in conduit range
            Map::PlayerList const &PlList = GetCaster()->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
            {
                if (Player* pPlayer = i->getSource())
                {
                    if (pPlayer->GetDistance2d(caster) < CONDUIT_RANGE && pPlayer->GetGUID() != caster->GetGUID() && pPlayer->HasAura(SPELL_LIGHTNING_CONDUIT))
                        targetCount++;
                }
            }

            // If there is no charged player or conduit, drop the aura
            if (targetCount == 0)
            {
                caster->RemoveAura(SPELL_LIGHTNING_CONDUIT_10N);
                caster->RemoveAura(SPELL_LIGHTNING_CONDUIT_10HC);
                caster->RemoveAura(SPELL_LIGHTNING_CONDUIT_25N);
                caster->RemoveAura(SPELL_LIGHTNING_CONDUIT_25HC);
                caster->RemoveAura(SPELL_LIGHTNING_CONDUIT);
            }
            // If there is charged player or conduit, check for new players in conduit range and charge them
            else
            {
                for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                {
                    if (Player* pPlayer = i->getSource())
                    {
                        if (pPlayer->GetDistance2d(caster) < CONDUIT_RANGE && pPlayer->GetGUID() != caster->GetGUID() && !pPlayer->HasAura(SPELL_LIGHTNING_CONDUIT))
                        {
                            GetCaster()->CastSpell(pPlayer, SPELL_LIGHTNING_CONDUIT, true);
                            pPlayer->CastSpell(pPlayer, SPELL_LIGHTNING_CONDUIT_10N, true);
                        }
                    }
                }
            }
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_ds_lightning_conduit_aura_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_ds_lightning_conduit_aura_AuraScript();
    }
};

void AddSC_boss_hagara_the_stormbinder()
{
    // Boss
    new boss_hagara_the_stormbinder();          // 55689

    // Normal Phase
    new npc_ice_lance();                        // 56108
    new npc_ice_tomb();                         // 55695

    // Lightning Phase
    new npc_crystal_conductor();                // 56165
    new npc_bound_lightning_elemental();        // 56700
    new npc_test_lightning_storm();             // 119950

    // Ice Phase
    new npc_frozen_binding_crystal();           // 56136
    new npc_ice_wave();                         // 56104
    new npc_collapsing_icicle();                // 57867
    new npc_frostflake_snare();                 // 119556

    // Spells
    new spell_ds_frostflake();                  // 109325
    new spell_ds_focused_assault();             // 107851, 110900, 110899, 110898
    new spell_ds_ice_lance_dmg();               // 105316, 107061, 107062, 107063
    new spell_ds_hagara_lightning_storm();      // 105465, 108568, 110893, 110892
    new spell_ds_lightning_conduit();           // 105367
    new spell_ds_lightning_conduit_aura();      // 105369, 108569, 109201, 109202
}

//////////////////////////////////////////////////////////////////////////////////////
///////////////////////////         SQL QUERY          ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
/*
--NPC
--Hagara the Stormbinder
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`,
`exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`,
`trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`,
`spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`,
`questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('55689','57462','57955','57956','0','0','39318','0','0','0','Hagara the Stormbinder','','','0','88','88','3','14','14','0','1.57143','1','1','1','0',
'0','0','0','1','2000','2000','2','0','0','0','0','0','0','0','0','0','0','7','108','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','0','3','400','100','1','0','0','0','0','0','0','0','157','1','55689','0','1','boss_hagara_the_stormbinder','15595');

--Frostflake Snare
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`,
`exp`,`faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`,
`trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`,
`spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`,
`questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('119556','0','0','0','0','0','11686','0','0','0','Frostflake Snare',NULL,NULL,'0','85','85','3','14','14','0','1','1.14286','1','0','0','0',
'0','0','1','0','0','1','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','0','3','1','1','1','0','0','0','0','0','0','0','0','1','0','0','128','npc_frostflake_snare','1');

--Lightning Storm
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`,
`exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`,
`trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`,
`spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`,
`questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('119950','0','0','0','0','0','11686','0','0','0','Lightning Storm Test',NULL,NULL,'0','85','85','3','14','14','0','1','1.14286','1','0','0','0',
'0','0','1','0','0','1','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','0','3','1','1','1','0','0','0','0','0','0','0','0','1','0','0','0','npc_test_lightning_storm','1');

--EQUIPMENT TEMPLATE
REPLACE INTO `creature_equip_template` (`entry`, `equipentry1`, `equipentry2`, `equipentry3`) values('55689','75236','75236','0');
REPLACE INTO `creature_equip_template` (`entry`, `equipentry1`, `equipentry2`, `equipentry3`) values('57462','75237','75237','0');

--GAMEOBJECTS
REPLACE INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `faction`, `flags`, `size`, `questItem1`, `questItem2`, `questItem3`, `questItem4`,
`questItem5`, `questItem6`, `data0`, `data1`, `data2`, `data3`,`data4`, `data5`, `data6`, `data7`, `data8`, `data9`, `data10`, `data11`, `data12`, `data13`, `data14`, `data15`, `data16`, `data17`,
`data18`, `data19`, `data20`, `data21`, `data22`, `data23`, `data24`, `data25`, `data26`, `data27`, `data28`, `data29`,`data30`, `data31`, `AIName`, `ScriptName`, `WDBVerified`) values
('210132','10','7800','The Focusing Iris','','','','0','0','5','0','0','0','0','0','0','0','0','0','3000','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','go_ds_focusing_iris','15595');

--SPELL SCRIPTS
insert into `spell_script_names` (`spell_id`, `ScriptName`) values('109325','spell_ds_frostflake');
insert into `spell_script_names` (`spell_id`, `ScriptName`) values('105367','spell_ds_lightning_conduit');

insert into `spell_script_names` (`spell_id`, `ScriptName`) values
('107851','spell_ds_focused_assault'),
('110900','spell_ds_focused_assault'),
('110899','spell_ds_focused_assault'),
('110898','spell_ds_focused_assault');

insert into `spell_script_names` (`spell_id`, `ScriptName`) values
('105316','spell_ds_ice_lance_dmg'),
('107061','spell_ds_ice_lance_dmg'),
('107062','spell_ds_ice_lance_dmg'),
('107063','spell_ds_ice_lance_dmg');

insert into `spell_script_names` (`spell_id`, `ScriptName`) values
('105369','spell_ds_lightning_conduit_aura'),
('108569','spell_ds_lightning_conduit_aura'),
('109201','spell_ds_lightning_conduit_aura'),
('109202','spell_ds_lightning_conduit_aura');

insert into `spell_script_names` (`spell_id`, `ScriptName`) values
('105465','spell_ds_hagara_lightning_storm'),
('108568','spell_ds_hagara_lightning_storm'),
('110893','spell_ds_hagara_lightning_storm'),
('110892','spell_ds_hagara_lightning_storm');
*/