/*
 * Copyright (C) 2008-2010 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
Encounter: High Prophet Barim
Dungeon: Lost City of the Tol'vir
Difficulty: Normal / Heroic
Mode: 5-man
Autor: Lazik
*/

#include "ScriptPCH.h"
#include "lost_city_of_the_tolvir.h"
#include "TaskScheduler.h"

const float LONG_RANGE_SEARCH_DISTANCE      = 300.0f;
const float SEARCH_DISTANCE                 = 100.0f;
const float MERGE_DISTANCE                  = 3.0f;

// NPCs
enum NPC
{
    BOSS_HIGH_PROPHET_BARIM                 = 43612,
    NPC_BLAZE_OF_THE_HEAVENS_SUMMONER       = 48904,
    NPC_BLAZE_OF_THE_HEAVENS                = 48906,
    NPC_BLAZE_OF_THE_HEAVENS_FIRE           = 48907,
    NPC_REPENTANCE_MIRROR                   = 43817,
    NPC_WAIL_OF_DARKNESS                    = 43927,
    NPC_WAIL_OF_DARKNESS_SUMMONER           = 43926,
    NPC_SOUL_FRAGMENT                       = 43934,
};

// Spells
enum Spells
{
    // High Prophet Barim
    SPELL_FIFTY_LASHINGS                    = 82506,
    SPELL_PLAGUE_OF_AGES                    = 82622,
    SPELL_HEAVENS_FURY                      = 81939,
    // Repentance
    SPELL_REPENTANCE_SUMMON_PLAYERS_MIRROR  = 81958,
    SPELL_REPENTANCE_CLONE_PLAYER           = 81961,
    SPELL_REPENTANCE_START                  = 82320,
    SPELL_REPENTANCE_PLAYER_PULL            = 82168,
    SPELL_REPENTANCE_PLAYER_KNEEL           = 81947,
    SPELL_REPENTANCE_PLAYER_KNOCK_BACK      = 82012,
    SPELL_REPENTANCE_PLAYER_CHANGE_PHASE    = 82139,
    SPELL_REPENTANCE_FINISH                 = 82430,
    // Harbinger of Darkness
    SPELL_WAIL_OF_DARKNESS_SUMMON_WAIL      = 82195,
    SPELL_WAIL_OF_DARKNESS_VISUAL           = 82197,
    SPELL_WAIL_OF_DARKNESS_SUMMON_HARBINGER = 82203,
    SPELL_WAIL_OF_DARKNESS_DAMAGE           = 82533,
    SPELL_WAIL_OF_DARKNESS_DEATH            = 82425,
    SPELL_SOUL_SEVER                        = 82255,
    SPELL_SOUL_SEVER_CHANNEL                = 82224,
    SPELL_MERGED_SOULS                      = 82263,
    SPELL_SOUL_SEVER_CLONE_PLAYER           = 82219,
    // Blaze of the Heavens
    SPELL_BLAZE_OF_THE_HEAVENS_SUMMONER     = 91181,
    SPELL_SUMMON_BLAZE_OF_THE_HEAVENS       = 91180,
    SPELL_BLAZE_OF_THE_HEAVENS_VISUAL       = 91179,
    SPELL_BLAZE_OF_THE_HEAVENS_TRANSFORM    = 95276,
    SPELL_BLAZE_OF_THE_HEAVENS_PERIODIC     = 95248,
    SPELL_BLAZE_OF_THE_HEAVENS_SUMMON_FIRE  = 91189,
    SPELL_BLAZE_OF_THE_HEAVENS_TRIGGER_FIRE = 91185,
    SPELL_BLAZE_OF_THE_HEAVENS_FIRE_DMG     = 91195,
    SPELL_BURNING_SOUL                      = 91277,
    // Achievement
    ACHIEVEMENT_KILL_IT_WITH_FIRE           = 5290,
};

enum Actions
{
    NORMAL_PHASE,
    REPENTANCE_PHASE,
    END_REPENTANCE_PHASE,

    SPAWN_BLAZE_OF_THE_HEAVENS,
    ACHIEVEMENT,
};

struct PlayableQuote barimDeath { 19733, "Death is only the beginning!" };
struct PlayableQuote barimAggro { 19735, "Begone infidels, you are not welcome here!" };
struct PlayableQuote barimRepentance { 19737, "Kneel before me and repent!" };
struct PlayableQuote barimKill { 19738, "May peace find you now." };
struct PlayableQuote barimKill1 { 19736, "The Heavens take you!" };

// High Prophet Barim
class boss_high_prophet_barim : public CreatureScript
{
public:
    boss_high_prophet_barim() : CreatureScript("boss_high_prophet_barim") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_high_prophet_barimAI(pCreature);
    }

    struct boss_high_prophet_barimAI : public ScriptedAI
    {
        boss_high_prophet_barimAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        TaskScheduler scheduler;
        // Timers
        uint32 fiftyLashingTimer;
        uint32 plagueOfAgesTimer;
        uint32 heavensFuryTimer;
        uint32 phase;
        uint8 achievementCounter;
        bool repentance;
        bool blazeOfTheHeavens;

        void Reset()
        {
            if (instance)
            {
                if (instance->GetData(DATA_HIGH_PROPHET_BARIM) != DONE)
                    instance->SetData(DATA_HIGH_PROPHET_BARIM, NOT_STARTED);

                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_REPENTANCE_PLAYER_CHANGE_PHASE);
            }

            DespawnCreatures(NPC_BLAZE_OF_THE_HEAVENS);
            DespawnCreatures(NPC_WAIL_OF_DARKNESS);
            DespawnCreatures(NPC_BLAZE_OF_THE_HEAVENS_FIRE);
            DespawnCreatures(NPC_REPENTANCE_MIRROR);
            phase = NORMAL_PHASE;
            repentance = false;
            blazeOfTheHeavens = false;
            achievementCounter = 0;
            plagueOfAgesTimer = 6000;
            fiftyLashingTimer = 5000;
            heavensFuryTimer = 7000;
            scheduler.CancelAll();
        }

        void DoAction(const int32 action) override
        {
            if (action == END_REPENTANCE_PHASE)
            {
                DespawnCreatures(NPC_REPENTANCE_MIRROR);
                DespawnCreatures(NPC_SOUL_FRAGMENT);
                me->RemoveAura(SPELL_REPENTANCE_START);
                if (instance)
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_REPENTANCE_PLAYER_CHANGE_PHASE);

                me->CastSpell(me, SPELL_REPENTANCE_PLAYER_PULL, true);

                plagueOfAgesTimer = 6000;
                fiftyLashingTimer = 5000;
                heavensFuryTimer = 7000;
                phase = NORMAL_PHASE;

                me->AI()->DoAction(SPAWN_BLAZE_OF_THE_HEAVENS);

                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                me->SetReactState(REACT_AGGRESSIVE);
                me->GetMotionMaster()->MoveChase(me->GetVictim());
            }
            else if (action == SPAWN_BLAZE_OF_THE_HEAVENS)
            {
                if (IsHeroic())
                {
                    scheduler.Schedule(Seconds(3), [this](TaskContext repentance)
                    {
                        me->CastSpell(me, SPELL_BLAZE_OF_THE_HEAVENS_SUMMONER, false);
                    });
                }
            }
            else if (action == ACHIEVEMENT)
            {
                achievementCounter++;

                if (instance && achievementCounter == 3)
                {
                    instance->DoCompleteAchievement(ACHIEVEMENT_KILL_IT_WITH_FIRE);
                }
            }
        }

        void EnterCombat(Unit * /*who*/)
        {
            if (instance)
            {
                instance->SetData(DATA_HIGH_PROPHET_BARIM, IN_PROGRESS);
            }

            me->GetAI()->DoAction(SPAWN_BLAZE_OF_THE_HEAVENS);
            RunPlayableQuote(barimAggro);
        }

        void KilledUnit(Unit* /*victim*/)
        {
            RunPlayableQuote(barimKill);
        }

        void JustDied(Unit * /*who*/)
        {
            if (instance)
            {
                instance->SetData(DATA_HIGH_PROPHET_BARIM, DONE);
            }

            DespawnCreatures(NPC_BLAZE_OF_THE_HEAVENS);
            DespawnCreatures(NPC_BLAZE_OF_THE_HEAVENS_FIRE);

            RunPlayableQuote(barimDeath);
        }

        void DespawnCreatures(uint32 entry)
        {
            std::list<Creature*> creatures;
            GetCreatureListWithEntryInGrid(creatures, me, entry, LONG_RANGE_SEARCH_DISTANCE);

            if (creatures.empty())
                return;

            for (std::list<Creature*>::iterator iter = creatures.begin(); iter != creatures.end(); ++iter)
                (*iter)->ForcedDespawn();
        }

        void UpdateAI(const uint32 diff)
        {
            scheduler.Update(diff);

            if (!UpdateVictim())
                return;

            if (phase == NORMAL_PHASE)
            {
                // Fifty Lashings Timer
                if (fiftyLashingTimer <= diff)
                {
                    me->CastSpell(me->GetVictim(), SPELL_FIFTY_LASHINGS, true);
                    fiftyLashingTimer = 24000;
                }
                else fiftyLashingTimer -= diff;

                // Plague of Ages Timer
                if (plagueOfAgesTimer <= diff)
                {
                    Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, SEARCH_DISTANCE, true);
                    if (target)
                        me->CastSpell(target, SPELL_PLAGUE_OF_AGES, true);
                    plagueOfAgesTimer = 18000;
                }
                else plagueOfAgesTimer -= diff;

                // Heavens Fury Timer
                if (heavensFuryTimer <= diff)
                {
                    Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, SEARCH_DISTANCE, true);
                    if (target)
                        me->CastSpell(target, SPELL_HEAVENS_FURY, true);
                    heavensFuryTimer = 15000;
                }
                else heavensFuryTimer -= diff;

                if (me->GetHealthPct() < 50 && repentance == false)
                {
                    phase = REPENTANCE_PHASE;
                }

                DoMeleeAttackIfReady();
            }

            if (phase == REPENTANCE_PHASE)
            {
                if (repentance == false)
                {
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                    me->SetReactState(REACT_PASSIVE);
                    me->GetMotionMaster()->MoveIdle();
                    me->AttackStop();
                    me->SendMovementFlagUpdate();

                    me->CastSpell(me, SPELL_REPENTANCE_START, true);
                    me->CastSpell(me, SPELL_REPENTANCE_PLAYER_PULL, true);
                    DespawnCreatures(NPC_BLAZE_OF_THE_HEAVENS);

                    scheduler.Schedule(Seconds(2), [this](TaskContext repentance)
                    {
                        if (repentance.GetRepeatCounter() == 0)
                        {
                            me->CastSpell(me, SPELL_REPENTANCE_PLAYER_KNEEL, true);
                            repentance.Repeat(Seconds(5));
                        }
                        else if (repentance.GetRepeatCounter() == 1)
                        {
                            me->CastSpell(me, SPELL_WAIL_OF_DARKNESS_SUMMON_WAIL, false);

                            me->CastSpell(me, SPELL_REPENTANCE_PLAYER_CHANGE_PHASE, true);
                            me->CastSpell(me, SPELL_REPENTANCE_PLAYER_KNOCK_BACK, true);
                        }
                    });

                    repentance = true;
                }
            }
        }

    };
};

class npc_blaze_of_the_heavens_summoner : public CreatureScript
{
public:
    npc_blaze_of_the_heavens_summoner() : CreatureScript("npc_blaze_of_the_heavens_summoner") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_blaze_of_the_heavens_summonerAI(pCreature);
    }

    struct npc_blaze_of_the_heavens_summonerAI : public ScriptedAI
    {
        npc_blaze_of_the_heavens_summonerAI(Creature *creature) : ScriptedAI(creature) {}

        TaskScheduler scheduler;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, SPELL_BLAZE_OF_THE_HEAVENS_VISUAL, false);

            scheduler.Schedule(Seconds(3), [this](TaskContext repentance)
            {
                me->CastSpell(me, SPELL_SUMMON_BLAZE_OF_THE_HEAVENS, false);
            });

            me->DespawnOrUnsummon(6000);
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);
        }
    };
};

class npc_blaze_of_the_heavens : public CreatureScript
{
public:
    npc_blaze_of_the_heavens() : CreatureScript("npc_blaze_of_the_heavens") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_blaze_of_the_heavensAI(pCreature);
    }

    struct npc_blaze_of_the_heavensAI : public ScriptedAI
    {
        npc_blaze_of_the_heavensAI(Creature *creature) : ScriptedAI(creature) 
        {
            me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, LONG_RANGE_SEARCH_DISTANCE);
        }

        void Reset() override
        {
            me->SetInCombatWithZone();
            me->CastSpell(me, SPELL_BLAZE_OF_THE_HEAVENS_PERIODIC, false);
            me->CastSpell(me, SPELL_BLAZE_OF_THE_HEAVENS_TRIGGER_FIRE, false);
        }

        void DamageTaken(Unit *, uint32 &damage) override
        {
            // Egg transformation
            if (damage >= me->GetHealth())
            {
                me->RemoveAllAuras();
                me->CastSpell(me, SPELL_BLAZE_OF_THE_HEAVENS_TRANSFORM, false);
                me->SetHealth(1);
                damage = 0;
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            // Rebirth
            if (me->GetHealthPct() == 100 && me->HasAura(SPELL_BLAZE_OF_THE_HEAVENS_TRANSFORM))
            {
                me->RemoveAura(SPELL_BLAZE_OF_THE_HEAVENS_TRANSFORM);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->CastSpell(me, SPELL_BLAZE_OF_THE_HEAVENS_PERIODIC, false);
                me->CastSpell(me, SPELL_BLAZE_OF_THE_HEAVENS_TRIGGER_FIRE, false);
            }

            DoMeleeAttackIfReady();
        }
    };
};

class npc_blaze_of_the_heavens_fire : public CreatureScript
{
public:
    npc_blaze_of_the_heavens_fire() : CreatureScript("npc_blaze_of_the_heavens_fire") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_blaze_of_the_heavens_fireAI(pCreature);
    }

    struct npc_blaze_of_the_heavens_fireAI : public ScriptedAI
    {
        npc_blaze_of_the_heavens_fireAI(Creature *creature) : ScriptedAI(creature) {}

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, SPELL_BLAZE_OF_THE_HEAVENS_FIRE_DMG, false);
        }

        void UpdateAI(const uint32 diff) override { }
    };
};

class npc_wail_of_darkness_summoner : public CreatureScript
{
public:
    npc_wail_of_darkness_summoner() : CreatureScript("npc_wail_of_darkness_summoner") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_wail_of_darkness_summonerAI(pCreature);
    }

    struct npc_wail_of_darkness_summonerAI : public ScriptedAI
    {
        npc_wail_of_darkness_summonerAI(Creature *creature) : ScriptedAI(creature) { }

        TaskScheduler scheduler;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, SPELL_WAIL_OF_DARKNESS_VISUAL, false);

            scheduler.Schedule(Seconds(3), [this](TaskContext repentance)
            {
                me->CastSpell(me, SPELL_WAIL_OF_DARKNESS_SUMMON_HARBINGER, false);
            });

            me->DespawnOrUnsummon(6000);
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);
        }
    };
};

class npc_wail_of_darkness : public CreatureScript
{
public:
    npc_wail_of_darkness() : CreatureScript("npc_wail_of_darkness") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_wail_of_darknessAI(pCreature);
    }

    struct npc_wail_of_darknessAI : public ScriptedAI
    {
        npc_wail_of_darknessAI(Creature *creature) : ScriptedAI(creature) 
        {
            me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, LONG_RANGE_SEARCH_DISTANCE);
        }

        uint32 wailOfDarknessTimer;
        uint32 soulSeverTimer;
        bool dead;

        void Reset() override
        {
            me->SetInCombatWithZone();
            wailOfDarknessTimer = 3000;
            soulSeverTimer = 7000;
            dead = false;
        }

        void DamageTaken(Unit *, uint32 &damage) override
        {
            if (damage >= me->GetHealth() && dead == false)
            {
                dead = true;
                if (Creature * bossBarim = me->FindNearestCreature(BOSS_HIGH_PROPHET_BARIM, LONG_RANGE_SEARCH_DISTANCE, true))
                    bossBarim->AI()->DoAction(END_REPENTANCE_PHASE);

                me->CastSpell(me, SPELL_WAIL_OF_DARKNESS_DEATH, false);
                me->SetHealth(1);
                damage = 0;
                me->DespawnOrUnsummon(0);
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (wailOfDarknessTimer <= diff)
            {
                me->CastSpell(me, SPELL_WAIL_OF_DARKNESS_DAMAGE, false);
                wailOfDarknessTimer = 1000;
            }
            else wailOfDarknessTimer -= diff;

            if (soulSeverTimer <= diff)
            {
                if (Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, LONG_RANGE_SEARCH_DISTANCE, true))
                    me->CastSpell(target, SPELL_SOUL_SEVER, false);

                soulSeverTimer = 7000;
            }
            else soulSeverTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_soul_fragment : public CreatureScript
{
public:
    npc_soul_fragment() : CreatureScript("npc_soul_fragment") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_soul_fragmentAI(pCreature);
    }

    struct npc_soul_fragmentAI : public ScriptedAI
    {
        npc_soul_fragmentAI(Creature *creature) : ScriptedAI(creature)
        {
            if (Creature * pHarbringer = me->FindNearestCreature(NPC_WAIL_OF_DARKNESS, SEARCH_DISTANCE, true))
                harbringerGuid = pHarbringer->GetGUID();
        }

        uint64 harbringerGuid;
        uint32 updateTimer;
        bool dead;

        void Reset() override
        {
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, SPELL_SOUL_SEVER_CHANNEL, false);
            updateTimer = 1000;
            dead = false;
        }

        void DamageTaken(Unit *, uint32 &damage) override
        {
            if (damage >= me->GetHealth() && dead == false)
            {
                dead = true;
                if (Creature * bossBarim = me->FindNearestCreature(BOSS_HIGH_PROPHET_BARIM, LONG_RANGE_SEARCH_DISTANCE, true))
                    bossBarim->AI()->DoAction(ACHIEVEMENT);

                damage = 0;
                me->DespawnOrUnsummon(0);
            }
        }

        void IsSummonedBy(Unit* pSummoner) override
        {
            if (Player * pPlayer = me->FindNearestPlayer(SEARCH_DISTANCE))
                pPlayer->CastSpell(me, SPELL_SOUL_SEVER_CLONE_PLAYER, false);

            if (Unit* pHarbringer = Unit::GetUnit(*me, harbringerGuid))
                me->CastSpell(pHarbringer, SPELL_SOUL_SEVER_CHANNEL, true);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (updateTimer <= diff)
            {
                if (Unit* pHarbringer = Unit::GetUnit(*me, harbringerGuid))
                {
                    me->GetMotionMaster()->MovePoint(0, pHarbringer->GetPositionX(), pHarbringer->GetPositionY(), pHarbringer->GetPositionZ());
                    if (pHarbringer->GetDistance(me) < MERGE_DISTANCE)
                    {
                        me->CastSpell(pHarbringer, SPELL_MERGED_SOULS, false);
                        me->DespawnOrUnsummon(500);
                    }
                }

                if (Creature * fire = me->FindNearestCreature(NPC_BLAZE_OF_THE_HEAVENS_FIRE, 1.5f, true))
                    me->AddAura(SPELL_BURNING_SOUL, me);

                updateTimer = 1000;
            }
            else updateTimer -= diff;
        }
    };
};

class npc_repentance_mirror : public CreatureScript
{
public:
    npc_repentance_mirror() : CreatureScript("npc_repentance_mirror") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_repentance_mirrorAI(pCreature);
    }

    struct npc_repentance_mirrorAI : public ScriptedAI
    {
        npc_repentance_mirrorAI(Creature *creature) : ScriptedAI(creature) { }

        void Reset() override 
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);
        }
        
        void IsSummonedBy(Unit* pSummoner) override
        {
            if (Player * pPlayer = me->FindNearestPlayer(SEARCH_DISTANCE))
                pPlayer->CastSpell(me, SPELL_REPENTANCE_CLONE_PLAYER, true);
        }
    };
};

void AddSC_boss_high_prophet_barim()
{
    new boss_high_prophet_barim();              // 43612, 48951
    new npc_blaze_of_the_heavens_summoner();    // 48904
    new npc_blaze_of_the_heavens();             // 48906
    new npc_blaze_of_the_heavens_fire();        // 48907
    new npc_wail_of_darkness_summoner();        // 43926
    new npc_wail_of_darkness();                 // 43927
    new npc_soul_fragment();                    // 43934
    new npc_repentance_mirror();                // 43817
}

//////////////////////////////////////////////////////////////////////////////////////
///////////////////////////         SQL QUERY          ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
/*
SELECT * FROM creature_template WHERE entry IN(43612, 48951, 48904, 48906, 48907, 43926, 43927, 43934, 43817)

-- High Prophet Barim
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('54123','0','0','0','0','0','39559','0','0','0','Echo of Sylvanas','','','0','87','87','3','16','16','0','1','1.3','1','3','50000','55000','0','0','1','1300','1300','2','0','1','0','0','0','0','0','0','0','0','7','72','54123','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','20000','20000','','0','3','59.35','38.5987','1','0','0','0','0','0','0','0','181','1','54123','646922239','0','boss_echo_of_sylvanas','15595');
*/