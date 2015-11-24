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
1) Heroic abilities
2) 25 man mode
3) Plate animations 
all scripts for adds can be found in dragon_soul_trash.cpp - Spine of Deathwing part
*/

#include "ScriptPCH.h"
#include "dragonsoul.h"
#include "MapManager.h"
#include "TaskScheduler.h"

// NPCs
enum NPC
{
    BOSS_SPINE_OF_DEATHWING         = 53879,

    NPC_HIDEOUS_AMALGAMATION        = 53890,
    NPC_CORRUPTION                  = 53891,
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

    // Spawner
    SPELL_GRASPING_TENDRILS         = 105510,
    SPELL_GRASPING_TENDRILS_10N     = 105563,
    SPELL_GRASPING_TENDRILS_25N     = 109454,
    SPELL_GRASPING_TENDRILS_10HC    = 109455,
    SPELL_GRASPING_TENDRILS_25HC    = 109456,

    SPELL_BLOOD_CORRUPTION_DEATH    = 106199,
    SPELL_BLOOD_CORRUPTION_EARTH    = 106200,
    SPELL_BLOOD_OF_DEATHWING        = 106201,
    SPELL_BLOOD_OF_NELTHARION       = 106213,
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
    { -13862.8f, -13645.0f, 265.7f, 1.5708f }, // left 1
    { -13846.8f, -13644.7f, 265.7f, 1.5708f }, // right 1
    { -13862.6f, -13626.3f, 266.5f, 1.5708f }, // left 2
    { -13846.6f, -13626.0f, 266.6f, 1.5708f }, // right 2
    { -13862.6f, -13604.9f, 269.2f, 1.5708f }, // left 3
    { -13846.6f, -13604.7f, 269.1f, 1.5708f }, // right 3
};

const Position rollPos[2] =
{
    { -13916.4f, -13682.0f, 254.5f, 1.47f }, // left
    { -13789.2f, -13682.7f, 254.5f, 1.47f }, // right
};

enum SpineActions
{
    ACTION_CORRUPTED_DIED          = 0,
    ACTION_CORRUPTED_POSITION      = 1,
    ACTION_TENDONS_POSITION        = 3,
    ACTION_CHECK_ROLL              = 4,
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

static const Position teleMadnessPos = { -12099.48f, 12160.05f, 18.79f, 1.74f };
static const Position cacheSpawnPos = { -12076.60f, 12169.94f, -2.53f, 3.54f };

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
        uint32 rollCheckTimer;
        uint32 rollState;
        uint32 talkTimer;
        uint8 deathwingSay;
        uint8 previousRoll;
        uint8 achievementCount;
        bool achievement;

        void Reset() override
        {
            if (instance)
            {
                if (instance->GetData(TYPE_BOSS_SPINE_OF_DEATHWING) != DONE)
                    instance->SetData(TYPE_BOSS_SPINE_OF_DEATHWING, NOT_STARTED);
            }

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_AGGRESSIVE);

            corruptedPosition = 0;
            tendonsPosition = 0;
            deathwingSay = 0;
            previousRoll = 0;
            achievementCount = 0;
            achievement = false;
            playersPosition = PLAYERS_ARE_IN_MIDDLE;
            rollState = ROLL_NONE;
            rollCheckTimer = 10000;
            talkTimer = urand(3000, 5000);

            Summons.DespawnAll();
        }

        void JustSummoned(Creature* summon) override
        {
            Summons.push_back(summon->GetGUID());
        }

        void EnterCombat(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_SPINE_OF_DEATHWING, IN_PROGRESS);
            }
        }

        void SaySomething()
        {
            switch (deathwingSay)
            {
            case 0:
                me->MonsterYell("Your tenacity is admirable, but pointless.You ride into the jaws of the apocalypse.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(26346, true);
                break;
            case 1:
                me->MonsterYell("You are less than dust, fit only to be brushed from my back.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(26350, true);
                break;
            case 2:
                me->MonsterYell("Ha!I had not realize you fools were still there.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(26351, true);
                break;
            case 3:
                me->MonsterYell("Your efforts are insignificant.I carry you to your deaths.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(26352, true);
                break;
            case 4:
                me->MonsterYell("Cling while you can, heroes. You and your world are doomed.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(26515, true);
                break;
            default:
                break;
            }
            deathwingSay++;
            if (deathwingSay >= 5)
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

                if (achievement)
                    instance->DoCompleteAchievement(6133); // Maybe He'll Get Dizzy...
                if (IsHeroic())
                    instance->DoCompleteAchievement(6115); // Heroic: Spine of Deathwing 
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

        void DoAction(const int32 action) override
        {
            if (instance)
            {
                switch (action)
                {
                case DATA_PREPARE_SPINE_ENCOUNTER:
                {
                    for (uint32 i = 0; i < 8; i++)
                    {
                        if (i < 4) // Spawn 4 corruptions
                        {
                            if (Creature * pCorruption = me->SummonCreature(NPC_CORRUPTION, corruptionPos[i], TEMPSUMMON_DEAD_DESPAWN))
                                pCorruption->AI()->SetData(ACTION_CORRUPTED_POSITION, i);
                        }
                        if (i < 6) // Spawn 6 tendons
                        {
                            if (Creature* pTendons = me->SummonCreature((((i % 2) == 1) ? NPC_BURNING_TENDONS_RIGHT : NPC_BURNING_TENDONS_LEFT), tendonsPos[i], TEMPSUMMON_DEAD_DESPAWN))
                                pTendons->AI()->SetData(ACTION_TENDONS_POSITION, i);
                        }
                        // Spawn 8 spawners
                        me->SummonCreature(NPC_SPAWNER, corruptionPos[i], TEMPSUMMON_MANUAL_DESPAWN);
                    }

                    me->SetVisible(true);
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->SetInCombatWithZone();
                    break;
                }
                case ACTION_CORRUPTED_DIED:
                {
                    Creature * pCorruption = me->FindNearestCreature(NPC_CORRUPTION, 300.0f, true);
                    if (!pCorruption)
                    {
                        // For every plate away, we need to add two more spawn positions
                        uint32 maxPos = 3;
                        if (instance->GetData(DATA_SPINE_OF_DEATHWING_PLATES) == 1)
                            maxPos = 5;
                        else if (instance->GetData(DATA_SPINE_OF_DEATHWING_PLATES) == 2)
                            maxPos = 7;

                        // new corruption shouldn`t spawn at it`s last killed position
                        uint32 randPos = urand(0, maxPos);
                        if (randPos == corruptedPosition && randPos == maxPos)
                            randPos = urand(0, maxPos-1);
                        else if (randPos == corruptedPosition)
                            randPos++;

                        if (Creature * pCorruption = me->SummonCreature(NPC_CORRUPTION, corruptionPos[randPos], TEMPSUMMON_DEAD_DESPAWN))
                            pCorruption->AI()->SetData(ACTION_CORRUPTED_POSITION, randPos);
                    }

                    scheduler.Schedule(Seconds(5), [this](TaskContext /* Task context */)
                    {
                        me->SummonCreature(NPC_HIDEOUS_AMALGAMATION, corruptionPos[corruptedPosition], TEMPSUMMON_DEAD_DESPAWN);
                    });
                    scheduler.Schedule(Seconds(15), [this](TaskContext /* Task context */)
                    {
                        me->SummonCreature(NPC_CORRUPTED_BLOOD, corruptionPos[corruptedPosition], TEMPSUMMON_DEAD_DESPAWN);
                    });
                    break;
                }
                case ACTION_CHECK_ROLL:
                {
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
                            me->MonsterTextEmote("Deathwing feels players on his left side. He's about to roll left!", me->GetGUID(), true, 300.0f);
                            rollState = ROLL_PRE_LEFT_2;
                        }
                        else
                            rollState = ROLL_NONE;
                        rollCheckTimer = 5000;
                        break;
                    case ROLL_PRE_LEFT_2:
                        if (playersPosition == MORE_PLAYERS_ON_LEFT_SIDE)
                        {
                            me->MonsterTextEmote("Deathwing rolls left!", me->GetGUID(), true, 300.0f);
                            rollState = ROLLING_LEFT;
                        }
                        else
                        {
                            me->MonsterTextEmote("Deathwing levels out.", me->GetGUID(), true, 300.0f);
                            rollState = ROLL_NONE;
                        }
                        rollCheckTimer = 1000;
                        break;
                    case ROLLING_LEFT:
                        DoRoll(ROLL_LEFT);
                        rollState = ROLL_NONE;
                        rollCheckTimer = 10000;
                        break;
                    case ROLL_PRE_RIGHT_1:
                        if (playersPosition == MORE_PLAYERS_ON_RIGHT_SIDE)
                        {
                            me->MonsterTextEmote("Deathwing feels players on his right side. He's about to roll right!", me->GetGUID(), true, 300.0f);
                            rollState = ROLL_PRE_RIGHT_2;
                        }
                        else
                            rollState = ROLL_NONE;
                        rollCheckTimer = 5000;
                        break;
                    case ROLL_PRE_RIGHT_2:
                        if (playersPosition == MORE_PLAYERS_ON_RIGHT_SIDE)
                        {
                            me->MonsterTextEmote("Deathwing rolls right!", me->GetGUID(), true, 300.0f);
                            rollState = ROLLING_RIGHT;
                        }
                        else
                        {
                            me->MonsterTextEmote("Deathwing levels out.", me->GetGUID(), true, 300.0f);
                            rollState = ROLL_NONE;
                        }
                        rollCheckTimer = 1000;
                        break;
                    case ROLLING_RIGHT:
                        DoRoll(ROLL_RIGHT);
                        rollState = ROLL_NONE;
                        rollCheckTimer = 10000;
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
                        for (uint8 i = 4; i < 6; i++)
                        {
                            if (Creature* pCorruption = me->SummonCreature(NPC_CORRUPTION, corruptionPos[i], TEMPSUMMON_DEAD_DESPAWN))
                                pCorruption->AI()->SetData(ACTION_CORRUPTED_POSITION, i);
                        }
                    }
                    else if (instance->GetData(DATA_SPINE_OF_DEATHWING_PLATES) == 2)
                    {
                        for (uint8 i = 6; i < 8; i++)
                        {
                            if (Creature* pCorruption = me->SummonCreature(NPC_CORRUPTION, corruptionPos[i], TEMPSUMMON_DEAD_DESPAWN))
                                pCorruption->AI()->SetData(ACTION_CORRUPTED_POSITION, i);
                        }
                    }
                    else if (instance->GetData(DATA_SPINE_OF_DEATHWING_PLATES) == 3)
                    {
                        PlayMovieToPlayers(75);
                        scheduler.Schedule(Seconds(25), [this](TaskContext /* Task context */)
                        {
                            me->Kill(me);
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
                    if (pl && pl->IsInWorld() && pl->IsAlive() && !pl->IsGameMaster())
                    {
                        if (!pl->HasAura(SPELL_GRASPING_TENDRILS_10N) && !pl->HasAura(SPELL_GRASPING_TENDRILS_25N)
                            && !pl->HasAura(SPELL_GRASPING_TENDRILS_25HC) && !pl->HasAura(SPELL_GRASPING_TENDRILS_25HC))
                        {
                            if (side == ROLL_LEFT)
                                pl->GetMotionMaster()->MoveJump(rollPos[DEST_LEFT].GetPositionX(), rollPos[DEST_LEFT].GetPositionY(), rollPos[DEST_LEFT].GetPositionZ(), 20.0f, 10.0f);
                            if (side == ROLL_RIGHT)
                                pl->GetMotionMaster()->MoveJump(rollPos[DEST_RIGHT].GetPositionX(), rollPos[DEST_RIGHT].GetPositionY(), rollPos[DEST_RIGHT].GetPositionZ(), 20.0f, 10.0f);
                        }
                    }
                }
            }

            std::list<Creature*> hideous_amalgamation;
            GetCreatureListWithEntryInGrid(hideous_amalgamation, me, NPC_HIDEOUS_AMALGAMATION, 200.0f);
            for (std::list<Creature*>::const_iterator itr = hideous_amalgamation.begin(); itr != hideous_amalgamation.end(); ++itr)
            {
                if (side == ROLL_LEFT)
                    (*itr)->GetMotionMaster()->MoveJump(rollPos[DEST_LEFT].GetPositionX(), rollPos[DEST_LEFT].GetPositionY(), rollPos[DEST_LEFT].GetPositionZ(), 20.0f, 10.0f);
                if (side == ROLL_RIGHT)
                    (*itr)->GetMotionMaster()->MoveJump(rollPos[DEST_RIGHT].GetPositionX(), rollPos[DEST_RIGHT].GetPositionY(), rollPos[DEST_RIGHT].GetPositionZ(), 20.0f, 10.0f);

                (*itr)->DespawnOrUnsummon(5000);
            }
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
                        if (pl->GetPositionX() >= -13850)
                            rightSide++;
                        if (pl->GetPositionX() <= -13860)
                            leftSide++;
                        maxPlayers++;
                    }
                }
            }

            maxDifference = (maxPlayers > 3) ? 2 : 1;

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
                me->AI()->DoAction(ACTION_CORRUPTED_DIED);
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
        }
    };
};

void AddSC_boss_spine_of_deathwing()
{
    new boss_spine_of_deathwing();
}