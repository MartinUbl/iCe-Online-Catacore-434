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
    //NPC_CORRUPTION_2                = 56161, // ?
    //NPC_CORRUPTION_3                = 56162, // ?
    //NPC_SPAWNER                     = 53888,
    NPC_CORRUPTED_BLOOD             = 53889,
    NPC_BURNING_TENDONS_LEFT        = 56341, // Left
    NPC_BURNING_TENDONS_RIGHT       = 56575, // Right
};

// Spells
enum Spells
{
    // Deathwing
    SPELL_ROLL_CONTROL              = 105036, // check aura
    SPELL_ROLL_CONTROL_AOE_1        = 104621, // triggered by check aura
    SPELL_ROLL_CONTROL_FORCE        = 105740, // 
    SPELL_ROLL_CONTROL_AOE_2        = 105777, // triggered by force spell
    SPELL_ROLL_CONTROL_AOE_3        = 105773, // ?
    SPELL_SUMMON_SLIME_AOE          = 105537, // 
    SPELL_SUMMON_SLIME              = 104999, // cast on spawner

    // Corruption
    SPELL_FIERY_GRIP                = 105490,
    SPELL_FIERY_GRIP_25             = 109457,
    SPELL_FIERY_GRIP_10H            = 109458,
    SPELL_FIERY_GRIP_25H            = 109459,
    SPELL_SEARING_PLASMA_AOE        = 109379,
    SPELL_SEARING_PLASMA            = 105479,

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

    // Spawner ?
    SPELL_GRASPING_TENDRILS         = 105510,
    SPELL_GRASPING_TENDRILS_10      = 105563,
    SPELL_GRASPING_TENDRILS_25      = 109454,
    SPELL_GRASPING_TENDRILS_10H     = 109455,
    SPELL_GRASPING_TENDRILS_25H     = 109456,

    // Corrupted Blood
    SPELL_RESIDUE                   = 105223,
    SPELL_BURST                     = 105219,

    // Burning Tendons
    SPELL_SEAL_ARMOR_BREACH_1       = 105847,
    SPELL_SEAL_ARMOR_BREACH_2       = 105848,
    SPELL_BREACH_ARMOR_1            = 105363,
    SPELL_BREACH_ARMOR_2            = 105385,
    SPELL_PLATE_FLY_OFF_LEFT        = 105366,
    SPELL_PLATE_FLY_OFF_RIGHT       = 105384,
    SPELL_SLOW                      = 110907, // ?

    SPELL_BLOOD_CORRUPTION_DEATH    = 106199,
    SPELL_BLOOD_CORRUPTION_EARTH    = 106200,
    SPELL_BLOOD_OF_DEATHWING        = 106201,
    SPELL_BLOOD_OF_NELTHARION       = 106213,
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

enum PlayersPosition
{
    PLAYERS_ARE_IN_MIDDLE           = 0,
    MORE_PLAYERS_ON_LEFT_SIDE       = 1,
    MORE_PLAYERS_ON_RIGHT_SIDE      = 2,
};

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

        void Reset() override
        {
            if (instance)
            {
                if (instance->GetData(TYPE_BOSS_SPINE_OF_DEATHWING) != DONE)
                    instance->SetData(TYPE_BOSS_SPINE_OF_DEATHWING, NOT_STARTED);
            }

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            //me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            //me->SetVisible(false);
            me->SetReactState(REACT_AGGRESSIVE);

            corruptedPosition = 0;
            tendonsPosition = 0;
            playersPosition = PLAYERS_ARE_IN_MIDDLE;
            rollState = ROLL_NONE;
            rollCheckTimer = 10000;

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

        void KilledUnit(Unit* victim) override {}

        void JustDied(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_SPINE_OF_DEATHWING, DONE);
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
                    for (uint32 i = 0; i < 4; i++)
                    {
                        if (Creature * pCorruption = me->SummonCreature(NPC_CORRUPTION, corruptionPos[i], TEMPSUMMON_DEAD_DESPAWN))
                            pCorruption->AI()->SetData(ACTION_CORRUPTED_POSITION, i);
                    }

                    for (uint8 i = 0; i < 6; i++)
                    {
                        if (Creature* pTendons = me->SummonCreature((((i % 2) == 1) ? NPC_BURNING_TENDONS_RIGHT : NPC_BURNING_TENDONS_LEFT), tendonsPos[i], TEMPSUMMON_DEAD_DESPAWN))
                            pTendons->AI()->SetData(ACTION_TENDONS_POSITION, i);
                    }

                    scheduler.Schedule(Seconds(30), [this](TaskContext /*task context*/)
                    {
                        me->SetVisible(true);
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->SetInCombatWithZone();

                        std::list<Creature*> corruption;
                        GetCreatureListWithEntryInGrid(corruption, me, NPC_CORRUPTION, 300.0f);
                        for (std::list<Creature*>::const_iterator itr = corruption.begin(); itr != corruption.end(); ++itr)
                        {
                            if (*itr)
                            {
                                (*itr)->SetReactState(REACT_AGGRESSIVE);
                                (*itr)->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                            }
                        }
                    });
                    break;
                }
                case ACTION_CORRUPTED_DIED:
                {
                    Creature * pCorruption = me->FindNearestCreature(NPC_CORRUPTION, 300.0f, true);
                    if (!pCorruption)
                    {
                        uint32 randPos = urand(0, 3);
                        if (randPos == corruptedPosition && randPos == 3)
                            randPos--;
                        if (randPos == corruptedPosition && randPos == 0)
                            randPos++;
                        me->SummonCreature(NPC_CORRUPTION, corruptionPos[randPos], TEMPSUMMON_DEAD_DESPAWN);
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
                        me->MonsterYell("ROLL_NONE", LANG_UNIVERSAL, 0);
                        if (playersPosition == MORE_PLAYERS_ON_LEFT_SIDE)
                        {
                            me->MonsterYell("ROLL_PRE_LEFT", LANG_UNIVERSAL, 0);
                            rollState = ROLL_PRE_LEFT_1;
                        }
                        else if (playersPosition == MORE_PLAYERS_ON_RIGHT_SIDE)
                        {
                            rollState = ROLL_PRE_RIGHT_1;
                            me->MonsterYell("ACTION_ROLL_RIGHT", LANG_UNIVERSAL, 0);
                        }
                        me->MonsterYell("ACTION_ROLL_MIDDLE", LANG_UNIVERSAL, 0);
                        rollCheckTimer = 2000;
                        break;
                    case ROLL_PRE_LEFT_1:
                        if (playersPosition == MORE_PLAYERS_ON_LEFT_SIDE)
                        {
                            rollState = ROLL_PRE_LEFT_2;
                            me->MonsterTextEmote("Deathwing is about to roll left", me->GetGUID(), true, 300.0f);
                        }
                        else
                        {
                            me->MonsterTextEmote("Uff, povedlo se vam naskladat na prostredek", me->GetGUID(), true, 300.0f);
                            rollState = ROLL_NONE;
                        }
                        rollCheckTimer = 5000;
                        break;
                    case ROLL_PRE_LEFT_2:
                        if (playersPosition == MORE_PLAYERS_ON_LEFT_SIDE)
                        {
                            me->MonsterTextEmote("Deathwing is going to roll left", me->GetGUID(), true, 300.0f);
                            rollState = ROLLING_LEFT;
                        }
                        else
                        {
                            me->MonsterTextEmote("Uff, povedlo se vam naskladat na prostredek", me->GetGUID(), true, 300.0f);
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
                            me->MonsterTextEmote("Deathwing is about to roll right", me->GetGUID(), true, 300.0f);
                            rollState = ROLL_PRE_RIGHT_2;
                        }
                        else
                        {
                            me->MonsterTextEmote("Uff, povedlo se vam naskladat na prostredek", me->GetGUID(), true, 300.0f);
                            rollState = ROLL_NONE;
                        }
                        rollCheckTimer = 5000;
                        break;
                    case ROLL_PRE_RIGHT_2:
                        if (playersPosition == MORE_PLAYERS_ON_RIGHT_SIDE)
                        {
                            me->MonsterTextEmote("Deathwing is going to roll right", me->GetGUID(), true, 300.0f);
                            rollState = ROLLING_RIGHT;
                        }
                        else
                        {
                            me->MonsterTextEmote("Uff, povedlo se vam naskladat na prostredek", me->GetGUID(), true, 300.0f);
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
                default:
                    break;
                }
            }
        }

        void DoRoll(uint8 side /* 1 - left, 2 - right*/)
        {
            if (side == ROLL_LEFT)
            {
            }

            if (side == ROLL_RIGHT)
            {
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
                        {
                            rightSide++;
                            pl->MonsterSay("Pridan do seznamu hracu na prava strane", LANG_UNIVERSAL, 0);
                        }
                        if (pl->GetPositionX() <= -13860)
                        {
                            pl->MonsterSay("Pridan do seznamu hracu na leve strane", LANG_UNIVERSAL, 0);
                            leftSide++;
                        }
                        maxPlayers++;
                    }
                }
            }

            maxDifference = (maxPlayers > 3) ? 2 : 1;

            if (leftSide - maxDifference > rightSide)
            {
                me->MonsterYell("MORE PLAYERS ON THE LEFT SIDE", LANG_UNIVERSAL, 0);
                playersPosition = MORE_PLAYERS_ON_LEFT_SIDE;
            }
            else if (rightSide - maxDifference > leftSide)
            {
                me->MonsterYell("MORE PLAYERS ON THE RIGH SIDE", LANG_UNIVERSAL, 0);
                playersPosition = MORE_PLAYERS_ON_RIGHT_SIDE;
            }
            else
            {
                me->MonsterYell("PLAYERS ARE IN MIDDLE", LANG_UNIVERSAL, 0);
                playersPosition = PLAYERS_ARE_IN_MIDDLE;
            }
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