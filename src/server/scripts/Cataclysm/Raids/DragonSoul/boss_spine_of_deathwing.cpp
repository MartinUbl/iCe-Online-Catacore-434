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
Encounter: Spine of Deathwing
Dungeon: Dragon Soul
Difficulty: Normal / Heroic
Mode: 10-man / 25-man
Autor: Lazik
*/

/*
TO DO:
1) 25 man mode
2) Fiery Grip sometime targets two different players and Effect 0 is used on player 1 and Effect 1 is used on player 2
3) Searing Plasma - still sometimes targets player who already has Searing Plasma debuff
*/

#include "ScriptPCH.h"
#include "dragonsoul.h"
#include "MapManager.h"
#include "TaskScheduler.h"

#define CENTRAL_SPINE_AXIS -13855.0
#define SEARCH_RANGE 300.0f

// NPCs
enum NPC
{
    BOSS_SPINE_OF_DEATHWING         = 53879,

    NPC_HIDEOUS_AMALGAMATION        = 53890,
    NPC_CORRUPTION                  = 53891,
    NPC_CORRUPTION_2                = 56161, // ?
    NPC_CORRUPTION_3                = 56162, // ?
    NPC_SPAWNER                     = 53888,
    NPC_CORRUPTED_BLOOD             = 53889,
    NPC_BURNING_TENDONS_LEFT        = 56341, // Left
    NPC_BURNING_TENDONS_RIGHT       = 56575, // Right
};

// Spells
enum Spells
{
    // Deathwing
    SPELL_SUMMON_SLIME_AOE          = 105537,
    SPELL_SUMMON_SLIME              = 104999, // cast on spawner

    // Corruption
    SPELL_FIERY_GRIP                = 105490,
    SPELL_FIERY_GRIP_25             = 109457,
    SPELL_FIERY_GRIP_10H            = 109458,
    SPELL_FIERY_GRIP_25H            = 109459,
    SPELL_SEARING_PLASMA_AOE        = 109379,
    SPELL_SEARING_PLASMA            = 105479,
    SPELL_SEARING_PLASMA_25N        = 109362,
    SPELL_SEARING_PLASMA_10HC       = 109363,
    SPELL_SEARING_PLASMA_25HC       = 109364,

    // Hideous Amalgamation
    SPELL_ZERO_REGEN                = 109121,
    SPELL_ABSORB_BLOOD_BAR          = 109329,
    SPELL_DEGRADATION               = 106005,
    SPELL_NUCLEAR_BLAST             = 105845,
    SPELL_NUCLEAR_BLAST_SCRIPT      = 105846,
    SPELL_SUPERHEATED_NUCLEUS       = 105834,
    SPELL_SUPERHEATED_NUCLEUS_DMG   = 106264,
    SPELL_ABSORB_BLOOD              = 105244,
    SPELL_ABSORB_BLOOD_DUMMY        = 105241, // target on 105223
    SPELL_ABSORBED_BLOOD            = 105248,

    // Spawner
    SPELL_GRASPING_TENDRILS         = 105510,
    SPELL_GRASPING_TENDRILS_10N     = 105563,
    SPELL_GRASPING_TENDRILS_25N     = 109454,
    SPELL_GRASPING_TENDRILS_10HC    = 109455,
    SPELL_GRASPING_TENDRILS_25HC    = 109456,

    // Corrupted Blood
    SPELL_RESIDUE                   = 105223,
    SPELL_BURST                     = 105219,

    // Burning Tendons
    SPELL_SEAL_ARMOR_BREACH_LEFT    = 105847, // 1698
    SPELL_SEAL_ARMOR_BREACH_RIGHT   = 105848, // 1699
    SPELL_BREACH_ARMOR_LEFT         = 105363, // 1677
    SPELL_BREACH_ARMOR_RIGHT        = 105385, // 1681
    SPELL_PLATE_FLY_OFF_LEFT        = 105366, // 1678
    SPELL_PLATE_FLY_OFF_RIGHT       = 105384, // 1680
    SPELL_SLOW                      = 110907,

    SPELL_BLOOD_CORRUPTION_DEATH    = 106199,
    SPELL_BLOOD_CORRUPTION_EARTH    = 106200,
    SPELL_BLOOD_OF_DEATHWING        = 106201,
    SPELL_BLOOD_OF_NELTHARION       = 106213,

    ACHIEVEMENT_DIZZY               = 6133,
    ACHIEVEMENT_HEROIC_SPINE        = 6115,
};

enum LootCache
{
    GO_GREATER_CACHE_OF_THE_ASPECTS_10N     = 209894,
    GO_GREATER_CACHE_OF_THE_ASPECTS_25N     = 209895,
    GO_GREATER_CACHE_OF_THE_ASPECTS_10HC    = 209896,
    GO_GREATER_CACHE_OF_THE_ASPECTS_25HC    = 209897,
};

const Position corruptionPos[8] =
{
    { -13868.6f, -13667.3f, 262.8f, 0.06f }, // left 1
    { -13841.1f, -13666.9f, 262.7f, 3.01f }, // right 1
    { -13868.5f, -13654.1f, 262.9f, 0.01f }, // left 2
    { -13842.8f, -13654.1f, 263.9f, 5.84f }, // right 2
    { -13867.1f, -13638.5f, 264.7f, 2.33f }, // left 3
    { -13841.2f, -13635.2f, 265.2f, 0.78f }, // right 3
    { -13870.5f, -13614.4f, 266.8f, 1.95f }, // left 4
    { -13839.3f, -13614.8f, 266.3f, 6.23f }, // right 4
};

const Position tendonsPos[6] =
{
    { -13867.5f, -13641.0f, 265.7f, 1.5708f }, // left 1
    { -13844.1f, -13639.7f, 265.7f, 1.5708f }, // right 1
    { -13867.8f, -13622.7f, 266.5f, 1.5708f }, // left 2
    { -13841.0f, -13621.7f, 266.6f, 1.5708f }, // right 2
    { -13867.1f, -13600.3f, 269.7f, 1.5708f }, // left 3
    { -13840.8f, -13600.6f, 269.1f, 1.5708f }, // right 3
};

const Position rollPos[2] =
{
    { -13916.4f, -13682.0f, 254.5f, 1.47f }, // left
    { -13789.2f, -13682.7f, 254.5f, 1.47f }, // right
};

enum SpineActions
{
    ACTION_CORRUPTION_DIED          = 0,
    ACTION_CORRUPTED_POSITION       = 1,
    ACTION_TENDONS_POSITION         = 3,
    ACTION_OPEN_PLATE               = 4,
    ACTION_CHECK_ROLL               = 5,
    ACTION_SPAWN_CORRUPTED_BLOOD    = 6,
    ACTION_ABSORB_BLOOD             = 7,
};

enum RollState
{
    ROLL_NONE                       = 0,
    ROLL_PRE_LEFT_1                 = 1,
    ROLL_PRE_RIGHT_1                = 2,
    ROLL_PRE_LEFT_2                 = 3,
    ROLL_PRE_RIGHT_2                = 4,
    ROLLING_LEFT                    = 5,
    ROLLING_RIGHT                   = 6,
};

enum RollSides
{
    ROLL_LEFT                       = 1,
    ROLL_RIGHT                      = 2,
};

enum JumpRollDest
{
    DEST_LEFT                       = 0,
    DEST_RIGHT                      = 1,
};

enum PlayersPosition
{
    PLAYERS_ARE_IN_MIDDLE           = 0,
    MORE_PLAYERS_ON_LEFT_SIDE       = 1,
    MORE_PLAYERS_ON_RIGHT_SIDE      = 2,
};

#define MAX_SPINE_OF_DEATHWING_YELL   5

struct PlayableQuote randomYell[MAX_SPINE_OF_DEATHWING_YELL]
{
    { 26346, "Your tenacity is admirable, but pointless.You ride into the jaws of the apocalypse." },
    { 26350, "You are less than dust, fit only to be brushed from my back." },
    { 26351, "Ha! I had not realize you fools were still there." },
    { 26352, "Your efforts are insignificant.I carry you to your deaths." },
    { 26515, "Cling while you can, heroes. You and your world are doomed." },
};

static const Position teleMadnessPos = { -12099.48f, 12160.05f, 18.79f, 1.74f };
static const Position cacheSpawnPos = { -12076.60f, 12169.94f, -2.53f, 3.54f };

#define ZERO_PLATE_OFF_MAX_SPAWN_COUNT          4
#define FIRST_PLATE_OFF_MAX_SPAWN_COUNT         6
#define SECOND_PLATE_OFF_MAX_SPAWN_COUNT        8
#define SPINE_OF_DEATHWING_DEFEAT_CINEMATIC    75
#define MAX_SPAWNER_COUNT                       8
#define MAX_CORRUPTION_COUNT                    4
#define MAX_TENDRONS_COUNT                      6

// Spine of Deathwing
class boss_spine_of_deathwing : public CreatureScript
{
public:
    boss_spine_of_deathwing() : CreatureScript("boss_spine_of_deathwing") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_spine_of_deathwingAI(pCreature);
    }

    struct boss_spine_of_deathwingAI : public ScriptedAI
    {
        boss_spine_of_deathwingAI(Creature *creature) : ScriptedAI(creature), Summons(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        TaskScheduler scheduler;
        SummonList Summons;
        uint32 corruptedPosition;
        uint32 tendonsPosition;
        uint32 playersPosition;
        uint32 nextSpawnTimer;
        uint32 rollCheckTimer;
        uint32 rollState;
        uint32 talkTimer;
        uint8 deathwingSay;
        uint8 previousRoll;
        uint8 achievementCount;
        bool achievement;
        bool firstCorruptionKilled;

        void Reset() override
        {
            if (instance)
            {
                if (instance->GetData(TYPE_BOSS_SPINE_OF_DEATHWING) != DONE)
                    instance->SetData(TYPE_BOSS_SPINE_OF_DEATHWING, NOT_STARTED);

                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                instance->SetData(DATA_SPINE_OF_DEATHWING_PLATES, 0);

                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_DEGRADATION);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SEARING_PLASMA);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SEARING_PLASMA_25N);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SEARING_PLASMA_10HC);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SEARING_PLASMA_25HC);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLOOD_OF_NELTHARION);
            }

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_AGGRESSIVE);
            me->SetFloatValue(UNIT_FIELD_COMBATREACH, 100.0f);
            me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, 100.0f);

            corruptedPosition = 0;
            tendonsPosition = 0;
            deathwingSay = 0;
            previousRoll = 0;
            achievementCount = 0;
            achievement = false;
            firstCorruptionKilled = false;
            playersPosition = PLAYERS_ARE_IN_MIDDLE;
            rollState = ROLL_NONE;
            nextSpawnTimer = 15000;
            rollCheckTimer = 10000;
            talkTimer = urand(3000, 5000);

            DeleteGameObjects(GO_DEATHWING_BACK_PLATE_1);
            DeleteGameObjects(GO_DEATHWING_BACK_PLATE_2);
            DeleteGameObjects(GO_DEATHWING_BACK_PLATE_3);

            scheduler.CancelAll();
            Summons.DespawnAll();

            ScriptedAI::Reset();
        }

        void JustSummoned(Creature* summon) override
        {
            Summons.push_back(summon->GetGUID());
        }

        void MoveInLineOfSight(Unit *who) override
        {
            if (who->GetTypeId() == TYPEID_PLAYER && !who->ToPlayer()->IsGameMaster())
            {
                if (instance)
                {
                    if (instance->GetData(TYPE_BOSS_SPINE_OF_DEATHWING) != IN_PROGRESS)
                    {
                        instance->SetData(DATA_PREPARE_SPINE_ENCOUNTER, 0);
                        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                    }
                }
            }
            ScriptedAI::MoveInLineOfSight(who);
        }

        void EnterCombat(Unit * /*who*/) override
        {
            if (instance)
                instance->SetData(TYPE_BOSS_SPINE_OF_DEATHWING, IN_PROGRESS);

            me->SetFullHealth();
        }

        void SaySomething()
        {
            RunPlayableQuote(randomYell[deathwingSay]);

            deathwingSay++;
            if (deathwingSay >= MAX_SPINE_OF_DEATHWING_YELL)
                deathwingSay = 0;
        }

        void TeleportAllPlayers()
        {
            Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
            {
                if (Player* player = i->getSource())
                {
                    player->NearTeleportTo(teleMadnessPos.GetPositionX(), teleMadnessPos.GetPositionY(), teleMadnessPos.GetPositionZ(), teleMadnessPos.GetOrientation());
                    player->AddAura(SPELL_PARACHUTE, player);
                }
            }
        }

        void PlayMovieToPlayers(uint32 movieId)
        {
            if (!instance)
                return;

            Map::PlayerList const& plList = instance->instance->GetPlayers();
            if (plList.isEmpty())
                return;

            for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
            {
                if (Player * pPlayer = itr->getSource())
                    pPlayer->SendMovieStart(movieId);
            }
        }

        void JustDied(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_SPINE_OF_DEATHWING, DONE);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_DEGRADATION);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SEARING_PLASMA);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SEARING_PLASMA_25N);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SEARING_PLASMA_10HC);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SEARING_PLASMA_25HC);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLOOD_OF_NELTHARION);

                if (achievement)
                    instance->DoCompleteAchievement(ACHIEVEMENT_DIZZY);
                if (IsHeroic())
                    instance->DoCompleteAchievement(ACHIEVEMENT_HEROIC_SPINE);

                instance->DoModifyPlayerCurrencies(CURRENCY_VALOR_POINTS, IsHeroic() ? 140 : 120, CURRENCY_SOURCE_OTHER);
                instance->DoModifyPlayerCurrencies(CURRENCY_MOTE_OF_DARKNESS, 1, CURRENCY_SOURCE_OTHER);
            }

            TeleportAllPlayers();
            SummonCache();
            Summons.DespawnAll();
        }

        void SummonCache()
        {
            uint32 cacheId = 0;
            switch (getDifficulty())
            {
            case RAID_DIFFICULTY_10MAN_NORMAL:
                cacheId = GO_GREATER_CACHE_OF_THE_ASPECTS_10N;
                break;
            case RAID_DIFFICULTY_25MAN_NORMAL:
                cacheId = GO_GREATER_CACHE_OF_THE_ASPECTS_25N;
                break;
            case RAID_DIFFICULTY_10MAN_HEROIC:
                cacheId = GO_GREATER_CACHE_OF_THE_ASPECTS_10HC;
                break;
            case RAID_DIFFICULTY_25MAN_HEROIC:
                cacheId = GO_GREATER_CACHE_OF_THE_ASPECTS_25HC;
                break;
            default:
                break;
            }
            me->SummonGameObject(cacheId, cacheSpawnPos.GetPositionX(), cacheSpawnPos.GetPositionY(), cacheSpawnPos.GetPositionZ(), cacheSpawnPos.GetOrientation(), 0, 0, 0, 0, 86400);
        }

        void DeleteGameObjects(uint32 entry)
        {
            std::list<GameObject*> gameobjects;
            me->GetGameObjectListWithEntryInGrid(gameobjects, entry, SEARCH_RANGE);
            if (!gameobjects.empty())
            {
                for (std::list<GameObject*>::iterator itr = gameobjects.begin(); itr != gameobjects.end(); ++itr)
                {
                    (*itr)->Delete();
                }
            }
        }

        void DoAction(const int32 action) override
        {
            if (instance)
            {
                switch (action)
                {
                case DATA_PREPARE_SPINE_ENCOUNTER:
                {
                    for (uint32 i = 0; i < MAX_SPAWNER_COUNT; i++)
                    {
                        if (i < MAX_CORRUPTION_COUNT) // Spawn 4 corruptions
                        {
                            if (Creature * pCorruption = me->SummonCreature(NPC_CORRUPTION, corruptionPos[i], TEMPSUMMON_DEAD_DESPAWN))
                                pCorruption->AI()->SetData(ACTION_CORRUPTED_POSITION, i);
                        }
                        if (i < MAX_TENDRONS_COUNT) // Spawn 6 tendons
                        {
                            if (Creature* pTendons = me->SummonCreature((((i % 2) == 1) ? NPC_BURNING_TENDONS_RIGHT : NPC_BURNING_TENDONS_LEFT), tendonsPos[i], TEMPSUMMON_DEAD_DESPAWN))
                                pTendons->AI()->SetData(ACTION_TENDONS_POSITION, i);
                        }
                        // Spawn 8 spawners
                        me->SummonCreature(NPC_SPAWNER, corruptionPos[i], TEMPSUMMON_MANUAL_DESPAWN);
                    }

                    // Spawn plates
                    me->SummonGameObject(GO_DEATHWING_BACK_PLATE_1, -13855.0f, -13638.9f, 267.685f, 1.530f, 0, 0, 0, 0, 86400);
                    me->SummonGameObject(GO_DEATHWING_BACK_PLATE_2, -13855.0f, -13618.4f, 269.402f, 1.550f, 0, 0, 0, 0, 86400);
                    me->SummonGameObject(GO_DEATHWING_BACK_PLATE_3, -13854.0f, -13599.3f, 272.333f, 1.558f, 0, 0, 0, 0, 86400);

                    me->SetVisible(true);
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->SetInCombatWithZone();
                    break;
                }
                case ACTION_SPAWN_CORRUPTED_BLOOD:
                {
                    // For every plate away, we need to add two more spawn positions
                    uint32 maxPos = ZERO_PLATE_OFF_MAX_SPAWN_COUNT;
                    nextSpawnTimer = IsHeroic() ? 9500 : 11000;
                    if (instance->GetData(DATA_SPINE_OF_DEATHWING_PLATES) == 1)
                    {
                        maxPos = FIRST_PLATE_OFF_MAX_SPAWN_COUNT;
                        nextSpawnTimer = IsHeroic() ? 7500 : 9000;
                    }
                    else if (instance->GetData(DATA_SPINE_OF_DEATHWING_PLATES) == 2)
                    {
                        maxPos = SECOND_PLATE_OFF_MAX_SPAWN_COUNT;
                        nextSpawnTimer = IsHeroic() ? 5500 : 7000;
                    }

                    uint32 randPos = urand(0, maxPos-1);
                    if (Creature * pCorruption = me->SummonCreature(NPC_CORRUPTED_BLOOD, corruptionPos[randPos], TEMPSUMMON_DEAD_DESPAWN))
                        pCorruption->AI()->SetData(ACTION_CORRUPTED_POSITION, randPos);
                    break;
                }
                case ACTION_CORRUPTION_DIED:
                {
                    firstCorruptionKilled = true;
                    nextSpawnTimer = 9000;

                    Creature * pCorruption = me->FindNearestCreature(NPC_CORRUPTION, SEARCH_RANGE, true);
                    if (!pCorruption)
                    {
                        // For every plate away, we need to add two more spawn positions
                        uint32 maxPos = ZERO_PLATE_OFF_MAX_SPAWN_COUNT;
                        if (instance->GetData(DATA_SPINE_OF_DEATHWING_PLATES) == 1)
                            maxPos = FIRST_PLATE_OFF_MAX_SPAWN_COUNT;
                        else if (instance->GetData(DATA_SPINE_OF_DEATHWING_PLATES) == 2)
                            maxPos = SECOND_PLATE_OFF_MAX_SPAWN_COUNT;

                        // new corruption shouldn`t spawn at it`s last killed position
                        uint32 randPos = urand(0, maxPos-1);
                        if (randPos == corruptedPosition && randPos == maxPos-1)
                            randPos = urand(0, maxPos-2);
                        else if (randPos == corruptedPosition)
                            randPos++;

                        if (Creature * pCorruption = me->SummonCreature(NPC_CORRUPTION, corruptionPos[randPos], TEMPSUMMON_DEAD_DESPAWN))
                            pCorruption->AI()->SetData(ACTION_CORRUPTED_POSITION, randPos);
                    }

                    // Spawn Hideous Amalgamation and Corrupted Blood 
                    uint32 lastCorruptedPosition = corruptedPosition;
                    scheduler.Schedule(Seconds(5), [this, lastCorruptedPosition](TaskContext spawn)
                    {
                        if (spawn.GetRepeatCounter() == 0)
                        {
                            me->SummonCreature(NPC_HIDEOUS_AMALGAMATION, corruptionPos[lastCorruptedPosition], TEMPSUMMON_DEAD_DESPAWN);
                            spawn.Repeat(Seconds(10));
                        }
                        else
                            me->SummonCreature(NPC_CORRUPTED_BLOOD, corruptionPos[lastCorruptedPosition], TEMPSUMMON_DEAD_DESPAWN);
                    });
                    break;
                }
                case ACTION_CHECK_ROLL:
                {
                    // Don`t roll when players are watching cinematic
                    if (instance && instance->GetData(DATA_SPINE_OF_DEATHWING_PLATES) == 3)
                        return;

                    CheckPlayersPosition();
                    switch (rollState)
                    {
                    case ROLL_NONE:
                        if (playersPosition == MORE_PLAYERS_ON_LEFT_SIDE)
                            rollState = ROLL_PRE_LEFT_1;
                        else if (playersPosition == MORE_PLAYERS_ON_RIGHT_SIDE)
                            rollState = ROLL_PRE_RIGHT_1;
                        rollCheckTimer = 2000;
                        break;
                    case ROLL_PRE_LEFT_1:
                        if (playersPosition == MORE_PLAYERS_ON_LEFT_SIDE)
                        {
                            me->MonsterTextEmote("Deathwing feels players on his left side. He's about to roll left!", me->GetGUID(), true, SEARCH_RANGE);
                            rollState = ROLL_PRE_LEFT_2;
                        }
                        else
                            rollState = ROLL_NONE;
                        rollCheckTimer = 5000;
                        break;
                    case ROLL_PRE_LEFT_2:
                        if (playersPosition == MORE_PLAYERS_ON_LEFT_SIDE)
                        {
                            me->MonsterTextEmote("Deathwing rolls left!", me->GetGUID(), true, SEARCH_RANGE);
                            rollState = ROLLING_LEFT;
                        }
                        else
                        {
                            me->MonsterTextEmote("Deathwing levels out.", me->GetGUID(), true, SEARCH_RANGE);
                            rollState = ROLL_NONE;
                        }
                        rollCheckTimer = 1000;
                        break;
                    case ROLLING_LEFT:
                        DoRoll(ROLL_LEFT);
                        rollState = ROLL_NONE;
                        rollCheckTimer = 1000;
                        break;
                    case ROLL_PRE_RIGHT_1:
                        if (playersPosition == MORE_PLAYERS_ON_RIGHT_SIDE)
                        {
                            me->MonsterTextEmote("Deathwing feels players on his right side. He's about to roll right!", me->GetGUID(), true, SEARCH_RANGE);
                            rollState = ROLL_PRE_RIGHT_2;
                        }
                        else
                            rollState = ROLL_NONE;
                        rollCheckTimer = 5000;
                        break;
                    case ROLL_PRE_RIGHT_2:
                        if (playersPosition == MORE_PLAYERS_ON_RIGHT_SIDE)
                        {
                            me->MonsterTextEmote("Deathwing rolls right!", me->GetGUID(), true, SEARCH_RANGE);
                            rollState = ROLLING_RIGHT;
                        }
                        else
                        {
                            me->MonsterTextEmote("Deathwing levels out.", me->GetGUID(), true, SEARCH_RANGE);
                            rollState = ROLL_NONE;
                        }
                        rollCheckTimer = 1000;
                        break;
                    case ROLLING_RIGHT:
                        DoRoll(ROLL_RIGHT);
                        rollState = ROLL_NONE;
                        rollCheckTimer = 1000;
                        break;
                    default:
                        rollCheckTimer = 2000;
                        break;
                    }
                    break;
                }
                case DATA_SPINE_OF_DEATHWING_PLATES:
                    if (instance->GetData(DATA_SPINE_OF_DEATHWING_PLATES) == 1)
                    {
                        me->DealDamage(me, me->GetMaxHealth() * 0.3);
                        for (uint8 i = MAX_CORRUPTION_COUNT; i < MAX_CORRUPTION_COUNT + 2; i++)
                        {
                            if (Creature* pCorruption = me->SummonCreature(NPC_CORRUPTION, corruptionPos[i], TEMPSUMMON_DEAD_DESPAWN))
                                pCorruption->AI()->SetData(ACTION_CORRUPTED_POSITION, i);
                        }
                    }
                    else if (instance->GetData(DATA_SPINE_OF_DEATHWING_PLATES) == 2)
                    {
                        me->DealDamage(me, me->GetMaxHealth() * 0.3);
                        for (uint8 i = MAX_CORRUPTION_COUNT + 2; i < MAX_CORRUPTION_COUNT + 4; i++)
                        {
                            if (Creature* pCorruption = me->SummonCreature(NPC_CORRUPTION, corruptionPos[i], TEMPSUMMON_DEAD_DESPAWN))
                                pCorruption->AI()->SetData(ACTION_CORRUPTED_POSITION, i);
                        }
                    }
                    else if (instance->GetData(DATA_SPINE_OF_DEATHWING_PLATES) == 3)
                    {
                        nextSpawnTimer = 30000;
                        rollCheckTimer = 30000;
                        scheduler.CancelAll();
                        Summons.DespawnAll();
                        PlayMovieToPlayers(SPINE_OF_DEATHWING_DEFEAT_CINEMATIC);
                        scheduler.Schedule(Seconds(25), [this](TaskContext /* Task context */)
                        {
                            me->DealDamage(me, me->GetHealth());
                        });
                    }
                    break;
                default:
                    break;
                }
            }
        }

        void DoRoll(uint8 side /* 1 - left, 2 - right */)
        {
            // Don`t roll when players are watching cinematic
            if (instance && instance->GetData(DATA_SPINE_OF_DEATHWING_PLATES) == 3)
                return;

            if (previousRoll != side)
                achievementCount++;
            else
                achievementCount = 0;
            if (achievementCount == 4)
                achievement = true;
            previousRoll = side;

            Map * map = me->GetMap();
            if (!map)
                return;

            Map::PlayerList const& plrList = map->GetPlayers();
            if (plrList.isEmpty())
                return;

            for (Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
            {
                if (Player* pl = itr->getSource())
                {
                    if (pl && pl->IsInWorld() && pl->IsAlive() && !pl->IsGameMaster() && pl->GetPositionZ() >= rollPos[DEST_LEFT].GetPositionZ())
                    {
                        if (!pl->HasAura(SPELL_GRASPING_TENDRILS_10N) && !pl->HasAura(SPELL_GRASPING_TENDRILS_25N)
                            && !pl->HasAura(SPELL_GRASPING_TENDRILS_10HC) && !pl->HasAura(SPELL_GRASPING_TENDRILS_25HC)
                            && !pl->HasAuraType(SPELL_AURA_MOD_STUN))
                        {
                            if (side == ROLL_LEFT)
                                pl->GetMotionMaster()->MoveJump(rollPos[DEST_LEFT].GetPositionX(), rollPos[DEST_LEFT].GetPositionY(), rollPos[DEST_LEFT].GetPositionZ(), 20.0f, 10.0f);
                            if (side == ROLL_RIGHT)
                                pl->GetMotionMaster()->MoveJump(rollPos[DEST_RIGHT].GetPositionX(), rollPos[DEST_RIGHT].GetPositionY(), rollPos[DEST_RIGHT].GetPositionZ(), 20.0f, 10.0f);

                            scheduler.Schedule(Seconds(5), [this, pl](TaskContext /* Task context */)
                            {
                                pl->Kill(pl, true);
                                pl->RepopAtGraveyard();
                            });
                        }
                    }
                }
            }

            Summons.DoAction(NPC_HIDEOUS_AMALGAMATION, side);
        }

        void CheckPlayersPosition()
        {
            Map * map = me->GetMap();
            if (!map)
                return;

            Map::PlayerList const& plrList = map->GetPlayers();
            if (plrList.isEmpty())
                return;

            uint8 maxDifference = 0;
            uint8 leftSide = 0;
            uint8 rightSide = 0;
            uint8 maxPlayers = 0;

            for (Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
            {
                if (Player* pl = itr->getSource())
                {
                    if (pl && pl->IsInWorld() && pl->IsAlive() && !pl->IsGameMaster())
                    {
                        if (pl->GetPositionX() >= CENTRAL_SPINE_AXIS + 4)
                            rightSide++;
                        else if (pl->GetPositionX() <= CENTRAL_SPINE_AXIS - 4)
                            leftSide++;
                        maxPlayers++;
                    }
                }
            }

            maxDifference = (maxPlayers > 1) ? 1 : 0;

            if (leftSide - maxDifference > rightSide)
                playersPosition = MORE_PLAYERS_ON_LEFT_SIDE;
            else if (rightSide - maxDifference > leftSide)
                playersPosition = MORE_PLAYERS_ON_RIGHT_SIDE;
            else
                playersPosition = PLAYERS_ARE_IN_MIDDLE;
        }

        void SetData(uint32 type, uint32 data) override
        {
            if (type == ACTION_CORRUPTED_POSITION)
            {
                corruptedPosition = data;
                me->AI()->DoAction(ACTION_CORRUPTION_DIED);
            }

            if (type == ACTION_TENDONS_POSITION)
                tendonsPosition = data;
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (!UpdateVictim())
                return;

            if (talkTimer <= diff)
            {
                SaySomething();
                talkTimer = urand(35000, 45000);
            }
            else talkTimer -= diff;

            if (rollCheckTimer <= diff)
            {
                me->AI()->DoAction(ACTION_CHECK_ROLL);
            }
            else rollCheckTimer -= diff;

            if (firstCorruptionKilled == true)
            {
                if (nextSpawnTimer <= diff)
                {
                    me->AI()->DoAction(ACTION_SPAWN_CORRUPTED_BLOOD);
                }
                else nextSpawnTimer -= diff;
            }
        }
    };
};

class npc_ds_spine_corruption : public CreatureScript
{
public:
    npc_ds_spine_corruption() : CreatureScript("npc_ds_spine_corruption") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_spine_corruptionAI(pCreature);
    }

    struct npc_ds_spine_corruptionAI : public ScriptedAI
    {
        npc_ds_spine_corruptionAI(Creature *creature) : ScriptedAI(creature) { }

        uint32 searingPlasmaTimer;
        uint32 fieryGripTimer;
        uint32 damageCounter;
        uint32 corruptedPosition;
        bool isGrip;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_AGGRESSIVE);

            isGrip = false;
            damageCounter = 0;
            corruptedPosition = 0;
            searingPlasmaTimer = urand(5000, 12000);
            fieryGripTimer = urand(10000, 25000);
        }

        void JustDied(Unit * /*who*/) override
        {
            if (Creature * pBoss = GetSummoner<Creature>())
                pBoss->AI()->SetData(ACTION_CORRUPTED_POSITION, corruptedPosition);
        }

        void SetData(uint32 type, uint32 data)
        {
            if (type == ACTION_CORRUPTED_POSITION)
                corruptedPosition = data;
        }

        void DamageTaken(Unit* /*who*/, uint32 &damage)
        {
            if (!isGrip)
                return;

            if (damageCounter <= damage)
            {
                damageCounter = 0;
                isGrip = false;
                fieryGripTimer = urand(30000, 35000);
                me->InterruptSpell(CURRENT_CHANNELED_SPELL);
            }
            else
                damageCounter -= damage;
        }

        void SpellHitTarget(Unit* target, const SpellEntry* spell) override
        {
            if (spell->Id == SPELL_SEARING_PLASMA_AOE)
            {
                if (!target->HasAura(SPELL_SEARING_PLASMA))
                    me->CastSpell(target, SPELL_SEARING_PLASMA, true);
            }
        }

        inline bool IsCastingAllowed()
        {
            return !me->IsNonMeleeSpellCasted(false) && me->GetReactState() == REACT_AGGRESSIVE;
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (searingPlasmaTimer <= diff)
            {
                if (IsCastingAllowed())
                {
                    me->CastSpell(me, SPELL_SEARING_PLASMA_AOE, false);
                    searingPlasmaTimer = 9000;
                }
            }
            else searingPlasmaTimer -= diff;

            if (fieryGripTimer <= diff)
            {
                if (IsCastingAllowed())
                {
                    if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 300.0f, true, -SPELL_FIERY_GRIP))
                    {
                        me->CastSpell(pTarget, SPELL_FIERY_GRIP, false);
                        damageCounter = me->CountPctFromMaxHealth(20);
                        isGrip = true;
                    }
                    fieryGripTimer = urand(30000, 35000);
                }
            }
            else fieryGripTimer -= diff;
        }
    };
};

static GameObject *GetNearestPlate(const Unit *caster)
{
    GameObject *nearestPlate = nullptr;
    float minDistance = std::numeric_limits<float>::max();
    for (auto plateId : {GO_DEATHWING_BACK_PLATE_1, GO_DEATHWING_BACK_PLATE_2, GO_DEATHWING_BACK_PLATE_3})
    {
        if (GameObject *plate = caster->FindNearestGameObject(plateId, 10))
        {
            auto distance = caster->GetDistance(*plate);
            if (distance > minDistance)
                continue;
            minDistance = distance;
            nearestPlate = plate;
        }
    }

    return nearestPlate;
}

class npc_ds_spine_burning_tendons : public CreatureScript
{
public:
    npc_ds_spine_burning_tendons() : CreatureScript("npc_ds_spine_burning_tendons") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_spine_burning_tendonsAI(pCreature);
    }

    class npc_ds_spine_burning_tendonsAI : public ScriptedAI
    {
        InstanceScript* instance = nullptr;
        uint32 openTimer = 0;
        uint32 tendonsPosition = 0;
        bool isOpen = false;

    public:
        npc_ds_spine_burning_tendonsAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset() override
        {
            tendonsPosition = 0;
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            ClosePlate();
        }

        void DamageTaken(Unit* /*who*/, uint32 &damage) override
        {
            if (me->GetHealth() > damage)
                return;

            if (me->GetEntry() == NPC_BURNING_TENDONS_LEFT)
            {
                if (Creature * pRightTendons = me->FindNearestCreature(NPC_BURNING_TENDONS_RIGHT, SEARCH_RANGE, true))
                    pRightTendons->DespawnOrUnsummon();
            }
            else if (me->GetEntry() == NPC_BURNING_TENDONS_RIGHT)
            {
                if (Creature * pLeftTendons = me->FindNearestCreature(NPC_BURNING_TENDONS_LEFT, SEARCH_RANGE, true))
                    pLeftTendons->DespawnOrUnsummon();
            }

            // can't cast spells in JustDied because I am dead in that function
            me->CastSpell(me, ((tendonsPosition % 2) == 1) ? SPELL_PLATE_FLY_OFF_RIGHT : SPELL_PLATE_FLY_OFF_LEFT, false);

            if (instance)
            {
                instance->SetData(DATA_SPINE_OF_DEATHWING_PLATES, 1);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }

            HidePlate();
        }

        void DoAction(const int32 action) override
        {
            if (action == ACTION_OPEN_PLATE)
            {
                if (instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                OpenPlate();
            }
        }

        void SetData(uint32 type, uint32 data)
        {
            if (type == ACTION_TENDONS_POSITION)
                tendonsPosition = data;
        }

        void UpdateAI(const uint32 diff) override
        {
            if (isOpen)
            {
                if (openTimer <= diff)
                {
                    if (instance)
                        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                    ClosePlate();
                }
                else openTimer -= diff;
            }
        }

    private:
        void OpenPlate()
        {
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetVisible(true);
            me->SetInCombatWithZone();
            me->CastSpell(me, ((tendonsPosition % 2) == 0) ? SPELL_BREACH_ARMOR_LEFT : SPELL_BREACH_ARMOR_RIGHT, false);
            me->CastSpell(me, ((tendonsPosition % 2) == 0) ? SPELL_SEAL_ARMOR_BREACH_LEFT : SPELL_SEAL_ARMOR_BREACH_RIGHT, false);
            openTimer = 23000;
            isOpen = true;
        }

        void ClosePlate()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetVisible(false);
            me->RemoveAllAuras();
            isOpen = false;
            if (auto *plate = GetNearestPlate(me))
            {
                plate->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_INTERACT_COND);
                plate->SetGoState(GO_STATE_READY);
            }
        }

        void HidePlate()
        {
            if (auto *plate = GetNearestPlate(me))
            {
                plate->Delete();
            }

        }
    };
};

struct spell_ds_spine_plate_activate_loader : SpellScriptLoader
{
    spell_ds_spine_plate_activate_loader() : SpellScriptLoader("spell_ds_spine_plate_activate") { }

    SpellScript *GetSpellScript() const override
    {
        return new spell_ds_spine_plate_activate();
    }

private:
    struct spell_ds_spine_plate_activate : SpellScript
    {
        PrepareSpellScript(spell_ds_spine_plate_activate);

    public:
        void Register() override
        {
            OnEffect += SpellEffectFn(spell_ds_spine_plate_activate::HandleActivate, 0, SPELL_EFFECT_ACTIVATE_OBJECT);
        }

    private:
        void HandleActivate(SpellEffIndex effectIndex)
        {
            if (auto *plate = GetNearestPlate(GetCaster()))
            {
                plate->SendAnimKit(GetSpellInfo()->EffectMiscValueB[effectIndex]);
            }
        }
    };
};

class npc_ds_spine_corrupted_blood : public CreatureScript
{
public:
    npc_ds_spine_corrupted_blood() : CreatureScript("npc_ds_spine_corrupted_blood") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_spine_corrupted_bloodAI(pCreature);
    }

    struct npc_ds_spine_corrupted_bloodAI : public ScriptedAI
    {
        npc_ds_spine_corrupted_bloodAI(Creature *creature) : ScriptedAI(creature) { }

        uint32 searingPlasmaTimer;
        uint32 fieryGripTimer;
        uint32 moveBackTimer;
        float x, y, z;
        bool isDead;

        void IsSummonedBy(Unit* pSummoner) override
        {
            x = me->GetPositionX();
            y = me->GetPositionY();
            z = me->GetPositionZ();
        }

        void Reset() override
        {
            isDead = false;
            moveBackTimer = 0;
            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            me->SetInCombatWithZone();
        }

        void DamageTaken(Unit* /*who*/, uint32 & damage)
        {
            if (me->GetHealth() < damage)
            {
                damage = 0;
                if (!isDead)
                {
                    moveBackTimer = 5000;
                    isDead = true;
                    me->SetReactState(REACT_PASSIVE);
                    me->AttackStop();
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                    me->CastSpell(me, SPELL_BURST, true);
                    me->CastSpell(me, SPELL_RESIDUE, true);
                    me->SetWalk(true);
                    me->SetSpeed(MOVE_WALK, 0.1f, true);
                }
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (isDead)
            {
                if (moveBackTimer <= diff)
                {
                    me->GetMotionMaster()->MovePoint(0, x, y, z);
                    isDead = false;
                }
                else moveBackTimer -= diff;
            }
            else if (me->GetDistance(me->GetHomePosition()) <= 1.0f)
            {
                isDead = false;
                DoResetThreat();
                me->SetReactState(REACT_AGGRESSIVE);
                me->RemoveAura(SPELL_RESIDUE);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            }

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_ds_spine_hideous_amalgamation : public CreatureScript
{
public:
    npc_ds_spine_hideous_amalgamation() : CreatureScript("npc_ds_spine_hideous_amalgamation") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_spine_hideous_amalgamationAI(pCreature);
    }

    struct npc_ds_spine_hideous_amalgamationAI : public ScriptedAI
    {
        npc_ds_spine_hideous_amalgamationAI(Creature *creature) : ScriptedAI(creature) 
        {
            isExplode = false;
        }

        TaskScheduler scheduler;
        uint32 dispelCount;
        bool isExplode;
        bool mutated;
        int absorbedBloodCount = 0;

        void Reset() override 
        {
            dispelCount = 0;
            mutated = false;

            if (IsHeroic())
            {
                scheduler.Schedule(Seconds(30), [this](TaskContext /* Task context */)
                {
                    if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, SEARCH_RANGE, true))
                        me->CastSpell(pTarget, SPELL_BLOOD_CORRUPTION_DEATH, false);
                });
            }
        }

        void DoAction(const int32 action)
        {
            if (action == ACTION_ABSORB_BLOOD)
            {
                me->CastSpell(me, SPELL_ABSORBED_BLOOD, true);
                // the spell SPELL_ABSORBED_BLOOD has a travel time - can't look at its aura
                absorbedBloodCount++;
                if (absorbedBloodCount == 9)
                    me->CastSpell(me, SPELL_SUPERHEATED_NUCLEUS, true);
            }
            else if (action == ROLL_LEFT)
            {
                me->GetMotionMaster()->MoveJump(rollPos[DEST_LEFT].GetPositionX(), rollPos[DEST_LEFT].GetPositionY(), rollPos[DEST_LEFT].GetPositionZ(), 20.0f, 10.0f);
                me->DespawnOrUnsummon(3000);
            }
            else if (action == ROLL_RIGHT)
            {
                me->GetMotionMaster()->MoveJump(rollPos[DEST_RIGHT].GetPositionX(), rollPos[DEST_RIGHT].GetPositionY(), rollPos[DEST_RIGHT].GetPositionZ(), 20.0f, 10.0f);
                me->DespawnOrUnsummon(3000);
            }
            // Spread Blood Corruption: Death
            else if (action)
            {
                dispelCount++;

                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, SEARCH_RANGE, true, -SPELL_BLOOD_CORRUPTION_DEATH))
                {
                    // 50% chance after 3rd dispel or 100% on 4th dispel to mutate into corruption earth
                    if ((roll_chance_i(50) && dispelCount == 3) || (dispelCount == 4 && mutated == false))
                    {
                        mutated = true;
                        me->CastSpell(pTarget, SPELL_BLOOD_CORRUPTION_EARTH, false);
                        if (Aura * pAura = pTarget->GetAura(SPELL_BLOOD_CORRUPTION_EARTH))
                            pAura->SetDuration(action);
                    }
                    // otherwise just jump corrupted blood death
                    if (mutated == false)
                    {
                        me->CastSpell(pTarget, SPELL_BLOOD_CORRUPTION_DEATH, false);
                        if (Aura * pAura = pTarget->GetAura(SPELL_BLOOD_CORRUPTION_DEATH))
                            pAura->SetDuration(action);
                    }
                }
            }
        }

        void MoveInLineOfSight(Unit *who) override
        {
            if (who->GetExactDist2d(me) < 3.0f)
            {
                if (who->IsAlive() && who->HasAura(SPELL_RESIDUE))
                {
                    me->AI()->DoAction(ACTION_ABSORB_BLOOD);
                    who->ToCreature()->DespawnOrUnsummon();
                }
            }
            ScriptedAI::MoveInLineOfSight(who);
        }

        void JustDied(Unit * /*who*/) override
        {
            if (IsHeroic())
            {
                Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                if (!PlList.isEmpty())
                {
                    for (Map::PlayerList::const_iterator itr = PlList.begin(); itr != PlList.end(); ++itr)
                    {
                        if (Player* pPlayer = itr->getSource())
                            pPlayer->AddAura(SPELL_DEGRADATION, pPlayer);
                    }
                }
            }

            scheduler.CancelAll();
        }

        void DamageTaken(Unit* who, uint32& damage)
        {
            if (me->GetHealth() <= damage)
            {
                if (me->HasAura(SPELL_SUPERHEATED_NUCLEUS))
                {
                    damage = me->GetHealth() - 1;
                    if (!isExplode)
                    {
                        isExplode = true;
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        me->CastSpell(me, SPELL_NUCLEAR_BLAST, false);
                        scheduler.Schedule(Milliseconds(5100), [this](TaskContext /* Task context */)
                        {
                            Creature * pBurningTendonsLeft = me->FindNearestCreature(NPC_BURNING_TENDONS_LEFT, 25.0f, true);
                            Creature * pBurningTendonsRight = me->FindNearestCreature(NPC_BURNING_TENDONS_RIGHT, 25.0f, true);
                            if (pBurningTendonsLeft && pBurningTendonsRight)
                            {
                                if (me->GetDistance2d(pBurningTendonsLeft->GetPositionX(), pBurningTendonsLeft->GetPositionY()) <=
                                    me->GetDistance2d(pBurningTendonsRight->GetPositionX(), pBurningTendonsRight->GetPositionY()))
                                {
                                    pBurningTendonsLeft->AI()->DoAction(ACTION_OPEN_PLATE);
                                }
                                else
                                    pBurningTendonsRight->AI()->DoAction(ACTION_OPEN_PLATE);
                            }
                            me->Kill(me);
                        });
                    }
                }
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_ds_spine_spawner : public CreatureScript
{
public:
    npc_ds_spine_spawner() : CreatureScript("npc_ds_spine_spawner") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_spine_spawnerAI(pCreature);
    }

    struct npc_ds_spine_spawnerAI : public ScriptedAI
    {
        npc_ds_spine_spawnerAI(Creature *creature) : ScriptedAI(creature) { }

        uint32 checkTimer;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            checkTimer = 1000;
        }

        void UpdateAI(const uint32 diff) override
        {
            if (checkTimer <= diff)
            {
                if (me->FindNearestCreature(NPC_CORRUPTION, 5.0f, true))
                    me->RemoveAura(SPELL_GRASPING_TENDRILS);
                else
                    me->CastSpell(me, SPELL_GRASPING_TENDRILS, false);
                checkTimer = 2500;
            }
            else checkTimer -= diff;
        }
    };
};

class spell_ds_spine_searing_plasma : public SpellScriptLoader
{
public:
    spell_ds_spine_searing_plasma() : SpellScriptLoader("spell_ds_spine_searing_plasma") {}

    class spell_ds_spine_searing_plasma_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_spine_searing_plasma_SpellScript);

        void HandleScript()
        {
            Unit * hitUnit = GetHitUnit();

            if (!hitUnit)
                return;

            if (!hitUnit->HasAura(SPELL_SEARING_PLASMA))
                GetCaster()->CastSpell(GetHitUnit(), SPELL_SEARING_PLASMA, false);
        }

        void Register()
        {
            OnHit += SpellHitFn(spell_ds_spine_searing_plasma_SpellScript::HandleScript);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_spine_searing_plasma_SpellScript();
    }
};

class spell_ds_spine_blood_corruption_death : public SpellScriptLoader
{
public:
    spell_ds_spine_blood_corruption_death() : SpellScriptLoader("spell_ds_spine_blood_corruption_death") { }

    class spell_ds_spine_blood_corruption_death_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_ds_spine_blood_corruption_death_AuraScript);

        void OnDispel(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {

            if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_STACK)
                return;

            if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_EXPIRE)
            {
                Unit* caster = aurEff->GetCaster();
                if (!caster)
                    return;

                if (Creature * pAmalgamation = caster->ToCreature())
                {
                    pAmalgamation->AI()->DoAction(aurEff->GetBase()->GetDuration());
                }
            }
            else
            {
                if (Unit* target = GetTarget())
                    target->CastSpell(target, SPELL_BLOOD_OF_DEATHWING, true);
            }
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_ds_spine_blood_corruption_death_AuraScript::OnDispel, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_ds_spine_blood_corruption_death_AuraScript();
    }
};

class spell_ds_spine_blood_corruption_earth : public SpellScriptLoader
{
public:
    spell_ds_spine_blood_corruption_earth() : SpellScriptLoader("spell_ds_spine_blood_corruption_earth") { }

    class spell_ds_spine_blood_corruption_earth_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_ds_spine_blood_corruption_earth_AuraScript);

        void OnDispel(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {

            if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_STACK)
                return;

            if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_EXPIRE)
            {
                Unit* caster = aurEff->GetCaster();
                if (!caster)
                    return;

                if (Creature * pAmalgamation = caster->ToCreature())
                {
                    pAmalgamation->AI()->DoAction(aurEff->GetBase()->GetDuration());
                }
            }
            else
            {
                if (Unit* target = GetTarget())
                {
                    target->CastSpell(target, SPELL_BLOOD_OF_NELTHARION, true);
                    if (roll_chance_i(50)) // 50% chance to apply second stack
                        target->CastSpell(target, SPELL_BLOOD_OF_NELTHARION, true);
                }
            }

        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_ds_spine_blood_corruption_earth_AuraScript::OnDispel, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_ds_spine_blood_corruption_earth_AuraScript();
    }
};

void AddSC_boss_spine_of_deathwing()
{
    new boss_spine_of_deathwing();
    new npc_ds_spine_corruption();
    new npc_ds_spine_corrupted_blood();
    new npc_ds_spine_hideous_amalgamation();
    new npc_ds_spine_burning_tendons();
    new npc_ds_spine_spawner();
    new spell_ds_spine_blood_corruption_death();
    new spell_ds_spine_blood_corruption_earth();

    new spell_ds_spine_plate_activate_loader();
    new spell_ds_spine_searing_plasma();
}

/*
insert into spell_script_names(spell_id, ScriptName) values
(105363, 'spell_ds_spine_plate_activate'),
(105366, 'spell_ds_spine_plate_activate'),
(105384, 'spell_ds_spine_plate_activate'),
(105385, 'spell_ds_spine_plate_activate'),
(105847, 'spell_ds_spine_plate_activate'),
(105848, 'spell_ds_spine_plate_activate');
update gameobject_template set type = 5, IconName = '' where entry in(209623, 209631, 209632);
*/
