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
Encounter: Archbishop Benedictus
Dungeon: Hour of Twilight
Difficulty: Heroic
Mode: 5-man
Autor: Lazik
*/

#include "ScriptPCH.h"
#include "Spell.h"
#include "UnitAI.h"
#include "MapManager.h"
#include "hour_of_twilight.h"

enum NPC
{
    BOSS_ARCHBISHOP_BENEDICTUS        =  54938,
    THRALL                            =  54971,
    NPC_WAVE_Of_VIRTUE                =  55441,
    NPC_WAVE_OF_TWILIGHT              =  55469,
    NPC_PURIFYING_LIGHT               =  55377,
    NPC_PURIFYING_BLAST               =  55427,
    NPC_TWILIGHT_BOLT                 =  55468,
    NPC_CORRUPTING_TWILIGHT           =  55467,
    NPC_WATER_SHELL                   =  55447,
    NPC_TWILIGHT_SPARKLE              =  55466,
    NPC_ARCHBISHOP_BENEDICTUS_INTRO   = 119514,
    NPC_HOLLY_WALL                    = 119515,
    NPC_PLATFORM_AURA                 = 119516,
};

// Spells
enum Spells
{
    // Intro
    HOLY_WALL                        = 102629, // Summon Barrier

    // 1st phase
    SMITE                            = 104503, // Dmg Spell

    PURIFYING_LIGHT_CHANNEL          = 103565, // Chanelling
    PURIFYING_TARGETING              = 103600, 
    PURIFYING_LIGHT_SHINE            = 103578, // Shining ball aura
    PURIFYING_LIGHT_GLOW             = 103579, // Glowing - size change

    PURIFYING_BLAST                  = 103648, // Jump dest target
    PURIFYING_BLAST_1                = 103651, // Impact dmg + knockback + summon
    PURIFIED                         = 103654, // Aura on the ground
    PURIFIED_1                       = 103653, // Dmg

    RIGHTEOUS_SHEAR                  = 103151, // Dmg Aura
    RIGHTEOUS_SHEAR_1                = 103161, // Dmg
    RIGHTEOUS_SHEAR_2                = 103149, // Dummy aura

    WAVE_OF_VIRTUE                   = 103676, // ?
    WAVE_OF_VIRTUE_1                 = 103677, // Summon Wave of Virtue
    WAVE_OF_VIRTUE_2                 = 103678, // Visual aura
    WAVE_OF_VIRTUE_3                 = 103680, // Summon Wave of Virtue
    WAVE_OF_VIRTUE_4                 = 103681, // Summon Wave of Virute
    WAVE_OF_VIRTUE_5                 = 103683, // Glow - mod scale
    WAVE_OF_VIRTUE_6                 = 103684, // Dmg + knockback

    // Transition
    TWILIGHT_EPIPHANY                = 103754, // Channel
    TWILIGHT_EPIPHANY_1              = 103755, // Explosion

    ENGULFING_TWILIGHT               = 103762, // Shackle Thrall
    
    TRANSFORM                        = 103765, // Change visual skin of Benedictus

    // 2nd phase
    TWILIGHT_BLAST                   = 104504, // Dmg Spell

    TWILIGHT_SHEAR                   = 103363, // Dmg Aura
    TWILIGHT_SHEAR_1                 = 103362, // Dmg
    TWILIGHT_SHEAR_2                 = 103526, // Dummy Aura

    CORRUPTING_TWILIGHT_CHANNEL      = 103767, // Chanelling
    CORRUPTING_TARGETING             = 103768,
    CORRUPTING_TWILIGHT_SHINE        = 103769, // Shining ball aura
    CORRUPTING_TWILIGHT_GLOW         = 103773, // Glowing - size change

    TWILIGHT_BOLT                    = 103776, // Jump dest target
    TWILIGHT_BOLT_1                  = 103777, // Impact dmg + knockback + summon
    TWILIGHT                         = 103774, // Aura on the ground
    TWILIGHT_1                       = 103775, // Dmg

    WAVE_OF_TWILIGHT                 = 103778, // ?
    WAVE_OF_TWILIGHT_1               = 103780, // Visual aura
    WAVE_OF_TWILIGHT_2               = 103781, // Dmg + knockback
    WAVE_OF_TWILIGHT_3               = 103782, // Summon
    WAVE_OF_TWILIGHT_4               = 103783, // Summon
    WAVE_OF_TWILIGHT_5               = 103784, // Summon

    UNSTABLE_TWILIGHT                = 103766, // Unstable Twilight

    // Platform boundaries
    SEAPING_LIGHT                    = 104516, // Aura
    SEAPING_LIGHT_1                  = 104528, // Dmg
    SEAPING_TWILIGHT                 = 104534, // Aura
    SEAPING_TWILIGHT_1               = 104537, // Dmg

    // Thrall`s spells
    PYROBLAST                        = 108442, // Dmg
    CLEANSE_SPIRIT                   = 103550, // Dispell
    CHAIN_LIGHTNING                  = 103637, // Dmg
    CHAIN_LIGHTNING_1                = 103638,
    WATER_SHELL                      = 103688,
};

const float BenedictusMovePoints[7][4] =
{
    // Benedictus Intro Movement Points
    {3670.13f, 285.29f, -119.129f, 0.019f}, // 0
    {3724.11f, 286.66f,  -92.899f, 3.179f}, // 1
    {3668.17f, 284.15f, -119.399f, 3.170f}, // 2
    {3639.66f, 282.44f, -120.144f, 3.241f}, // 3
    {3595.60f, 277.90f, -119.968f, 3.232f}, // 4
    {3582.70f, 277.09f, -114.031f, 3.274f}, // 5
    {3541.93f, 272.55f, -115.964f, 0.265f}, // 6
};

// Archbishop Benedictus
class boss_archbishop_benedictus : public CreatureScript
{
public:
    boss_archbishop_benedictus() : CreatureScript("boss_archbishop_benedictus") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_archbishop_benedictusAI(pCreature);
    }

    struct boss_archbishop_benedictusAI : public ScriptedAI
    {
        boss_archbishop_benedictusAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetVisible(false);
        }

        InstanceScript* instance;
        uint32 Spell_Timer;
        uint32 Engulfing_Twilight_Timer;
        uint32 Wave_Timer;
        uint32 Shear_Timer;
        uint32 Light_Or_Twilight_Timer;
        uint32 Thrall_Clean_Timer;
        uint32 Thrall_Lightning_Timer;
        uint32 Jump_Timer;
        int Random_Text;
        int Phase;
        int Achievement_Count;
        int Clean_Count;
        int Lightning_Count;
        bool Engulfing_Twilight;
        bool Phase_Change;
        bool Thrall_Lightning;
        bool Thrall_Clean;

        void Reset()
        {
            if (instance)
            {
                if (instance->GetData(TYPE_BOSS_ARCHBISHOP_BENEDICTUS) != DONE)
                    instance->SetData(TYPE_BOSS_ARCHBISHOP_BENEDICTUS, NOT_STARTED);
            }

            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveAura(103765); // Transform aura
            Random_Text = 0;
            Phase = 0;
            Achievement_Count = 0;
            Phase_Change = false;
            Engulfing_Twilight = false;
            Spell_Timer = 10000;
            Wave_Timer = 30000;
            Shear_Timer = 45000;
            Light_Or_Twilight_Timer = 15000;
            Jump_Timer = 300000;
            Lightning_Count = 0;
            Clean_Count = 0;
            Thrall_Clean = false;
            Thrall_Lightning = false;

            Creature * thrall = me->FindNearestCreature(THRALL, 100.0f, true);
            if (thrall)
            {
                thrall->RemoveAura(ENGULFING_TWILIGHT);
                thrall->InterruptNonMeleeSpells(false);
                thrall->CombatStop();
            }

            // Remove Twilight epiphany from players
            Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                if (Player* player = i->getSource())
                {
                if (player->HasAura(TWILIGHT_EPIPHANY_1))
                    player->RemoveAura(TWILIGHT_EPIPHANY_1);
                }

            Creature * platform_aura = me->FindNearestCreature(NPC_PLATFORM_AURA, 100.0f, true);
            if (platform_aura)
            {
                if (platform_aura->HasAura(SEAPING_LIGHT))
                    platform_aura->RemoveAura(SEAPING_LIGHT);
                if (platform_aura->HasAura(SEAPING_TWILIGHT))
                    platform_aura->RemoveAura(SEAPING_TWILIGHT);
            }

            std::list<Creature*> twilight_bolt;
            GetCreatureListWithEntryInGrid(twilight_bolt, me, NPC_TWILIGHT_BOLT, 100.0f);
            for (std::list<Creature*>::const_iterator itr = twilight_bolt.begin(); itr != twilight_bolt.end(); ++itr)
                if (*itr)
                    (*itr)->ForcedDespawn();

            std::list<Creature*> purifying_blast;
            GetCreatureListWithEntryInGrid(purifying_blast, me, NPC_PURIFYING_BLAST, 100.0f);
            for (std::list<Creature*>::const_iterator itr = purifying_blast.begin(); itr != purifying_blast.end(); ++itr)
                if (*itr)
                    (*itr)->ForcedDespawn();

            std::list<Creature*> twilight_sparkle;
            GetCreatureListWithEntryInGrid(twilight_sparkle, me, NPC_TWILIGHT_SPARKLE, 100.0f);
            for (std::list<Creature*>::const_iterator itr = twilight_sparkle.begin(); itr != twilight_sparkle.end(); ++itr)
                if (*itr)
                    (*itr)->ForcedDespawn();
        }

        void EnterCombat(Unit * /*who*/)
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_ARCHBISHOP_BENEDICTUS, IN_PROGRESS);
            }

            Creature * platform_aura = me->FindNearestCreature(NPC_PLATFORM_AURA, 100.0f, true);
            if (platform_aura)
                platform_aura->CastSpell(platform_aura, SEAPING_LIGHT, false);
        }

        void KilledUnit(Unit* /*victim*/) { }

        void JustDied(Unit* /*who*/)
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_ARCHBISHOP_BENEDICTUS, DONE);
            }

            // Remove Twilight epiphany from players
            Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                if (Player* player = i->getSource())
                {
                    // Remove Debuff
                    if (player->HasAura(TWILIGHT_EPIPHANY_1))
                        player->RemoveAura(TWILIGHT_EPIPHANY_1);
                    // Add Achievement Heroic: Hour of Twilight
                    player->GetAchievementMgr().CompletedAchievement(sAchievementStore.LookupEntry(6119), true);
                    // Add Achievement Eclipse
                    if (Achievement_Count >= 10)
                        player->GetAchievementMgr().CompletedAchievement(sAchievementStore.LookupEntry(6132), true);
                }

            Creature * thrall = me->FindNearestCreature(THRALL, 100.0f, true);
            if (thrall)
                thrall->RemoveAura(ENGULFING_TWILIGHT);

            Creature * platform_aura = me->FindNearestCreature(NPC_PLATFORM_AURA, 100.0f, true);
            if (platform_aura)
            {
                if (platform_aura->HasAura(SEAPING_LIGHT))
                    platform_aura->RemoveAura(SEAPING_LIGHT);
                if (platform_aura->HasAura(SEAPING_TWILIGHT))
                    platform_aura->RemoveAura(SEAPING_TWILIGHT);
            }

            std::list<Creature*> twilight_sparkle;
            GetCreatureListWithEntryInGrid(twilight_sparkle, me, NPC_TWILIGHT_SPARKLE, 100.0f);
            for (std::list<Creature*>::const_iterator itr = twilight_sparkle.begin(); itr != twilight_sparkle.end(); ++itr)
                if (*itr)
                    (*itr)->ForcedDespawn();

            std::list<Creature*> twilight_bolt;
            GetCreatureListWithEntryInGrid(twilight_bolt, me, NPC_TWILIGHT_BOLT, 100.0f);
            for (std::list<Creature*>::const_iterator itr = twilight_bolt.begin(); itr != twilight_bolt.end(); ++itr)
                if (*itr)
                    (*itr)->ForcedDespawn();

            me->MonsterYell("I looked into the eyes of The Dragon, and despaired...", LANG_UNIVERSAL, 0);
            me->SendPlaySound(25865, true);
        }

        void DoAction(const int32 /*param*/)
        {
            Achievement_Count++;
        }

        // Twilight Sparkles for achievement
        void SummonTwilightSparkle()
        {
            float angle = me->GetOrientation();
            double angleAddition = (M_PI / 5); // 36 Digrees 
            angle = MapManager::NormalizeOrientation(angle);

            for (uint32 i = 0; i < 10; i++)
            {
                float distance = urand(55, 70);
                float height = urand(0, 15);
                me->SummonCreature(NPC_TWILIGHT_SPARKLE, me->GetPositionX() + cos(angle)*distance, me->GetPositionY() + sin(angle)*distance, me->GetPositionZ() + height, angle, TEMPSUMMON_MANUAL_DESPAWN);
                angle += angleAddition;
            }
        }

        // Puryfiyng light and Corrupting Twilight summons
        void SummonGlowingBalls()
        {
            float angle = me->GetOrientation()-(M_PI/2); // They suppose to make triangle frmation above Archbishop
            double angleAddition = (M_PI/2); // 90 Digrees 
            angle = MapManager::NormalizeOrientation(angle);
            float distance;
            float height;

            for (uint32 i = 0; i < 3; i++)
            {
                if (i == 1) // if 2nd (middle/top)
                {
                    distance = 0;
                    height = 5;
                }
                else // 1st and 3rd
                {
                    distance = 2.5;
                    height = 2;
                }

                if (Phase == 0)
                    me->SummonCreature(NPC_PURIFYING_LIGHT, me->GetPositionX() + cos(angle)*distance, me->GetPositionY() + sin(angle)*distance, me->GetPositionZ()+height, angle, TEMPSUMMON_TIMED_DESPAWN, 10000);
                else
                    me->SummonCreature(NPC_CORRUPTING_TWILIGHT, me->GetPositionX() + cos(angle)*distance, me->GetPositionY() + sin(angle)*distance, me->GetPositionZ()+height, angle, TEMPSUMMON_TIMED_DESPAWN, 10000);
                angle += angleAddition;
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            // Purifying Light and Corrupting Twilight
            if (Light_Or_Twilight_Timer <= diff)
            {
                Spell_Timer = Spell_Timer + 8000;

                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                me->SetReactState(REACT_PASSIVE);
                me->AttackStop();
                me->GetMotionMaster()->MoveIdle();
                me->SendMovementFlagUpdate();

                if (Phase == 0)
                    me->CastSpell(me, PURIFYING_LIGHT_CHANNEL, false);
                else
                    me->CastSpell(me, CORRUPTING_TWILIGHT_CHANNEL, false);
                SummonGlowingBalls();
                Thrall_Lightning_Timer = 4000;
                Thrall_Lightning = true;
                Light_Or_Twilight_Timer = 60000;
                Jump_Timer = 8000;
            }
            else Light_Or_Twilight_Timer -= diff;

            if (Jump_Timer <= diff)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                me->SetReactState(REACT_AGGRESSIVE);
                me->GetMotionMaster()->MoveChase(me->GetVictim());
                me->SendMovementFlagUpdate();

                // Force Twilight bolts to target 3 different players, so they don`t jump on the same player
                if (Phase == 1)
                {
                    uint32 target_pl = 1; // Target 3 players - probably better exclude tank

                    std::list<Creature*> corrupting_twilight;
                    GetCreatureListWithEntryInGrid(corrupting_twilight, me, NPC_CORRUPTING_TWILIGHT, 100.0f);
                    for (std::list<Creature*>::const_iterator itr = corrupting_twilight.begin(); itr != corrupting_twilight.end(); ++itr)
                        if (*itr && ((*itr)->GetPositionZ() > me->GetPositionZ()+5))
                        {
                            Unit * target = SelectTarget(SELECT_TARGET_TOPAGGRO, target_pl, 200.0f, true);
                            if (target)
                            {
                                (*itr)->CastSpell(target, PURIFYING_BLAST, false);
                                target_pl++; // Change next target in topaggro list
                            }
                            else // Just in case there is less players alive
                            {
                                (*itr)->CastSpell(me->GetVictim(), PURIFYING_BLAST, false);
                                target_pl++;
                            }
                        }
                }
                Jump_Timer = 60000;
            }
            else Jump_Timer -= diff;

            // Smite / Twilight blast timer
            if (Spell_Timer <= diff)
            {
                Unit * target = me->GetVictim();
                if (target)
                {
                    if (Phase == 0)
                        me->CastSpell(target, SMITE, false);
                    else
                        me->CastSpell(target, TWILIGHT_BLAST, false);
                }
                Spell_Timer = 10000;
            }
            else Spell_Timer -= diff;

            // Additional phase changes
            if (Engulfing_Twilight == true)
            {
                if (Engulfing_Twilight_Timer <= diff)
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->GetMotionMaster()->MoveChase(me->GetVictim());
                    me->SendMovementFlagUpdate();

                    // Stun Thrall
                    Creature * thrall = me->FindNearestCreature(THRALL, 100.0f, true);
                    if (thrall)
                        thrall->CastSpell(thrall, ENGULFING_TWILIGHT, false);

                    // Change platform aura visual aura
                    Creature * platform_aura = me->FindNearestCreature(NPC_PLATFORM_AURA, 100.0f, true);
                    if (platform_aura && platform_aura->HasAura(SEAPING_LIGHT))
                    {
                        platform_aura->RemoveAura(SEAPING_LIGHT);
                        platform_aura->CastSpell(platform_aura, SEAPING_TWILIGHT, false);
                    }

                    // Despawn purifying Lights
                    std::list<Creature*> purifying_blast;
                    GetCreatureListWithEntryInGrid(purifying_blast, me, NPC_PURIFYING_BLAST, 100.0f);
                    for (std::list<Creature*>::const_iterator itr = purifying_blast.begin(); itr != purifying_blast.end(); ++itr)
                        if (*itr)
                            (*itr)->ForcedDespawn();

                    SummonTwilightSparkle(); // Spawn Twilight Sparkle for achievemenet
                    Engulfing_Twilight = false;
                    me->CastSpell(me, TRANSFORM, false); // Change model ID
                }
                else Engulfing_Twilight_Timer -= diff;
            }

            // Phase Change
            if (Phase_Change == false)
            {
                if (me->HealthBelowPct(60))
                {
                    if (!me->HasUnitState(UNIT_STATE_CASTING))
                    {
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                        me->SetReactState(REACT_PASSIVE);
                        me->AttackStop();
                        me->GetMotionMaster()->MoveIdle();
                        me->SendMovementFlagUpdate();

                        me->CastSpell(me, TWILIGHT_EPIPHANY, false);

                        me->MonsterYell("Witness my POWER!", LANG_UNIVERSAL, 0);
                        me->SendPlaySound(21416, true);

                        Light_Or_Twilight_Timer = 15000;
                        Jump_Timer = 23000;
                        Engulfing_Twilight = true;
                        Engulfing_Twilight_Timer = 5000;
                        Spell_Timer = 10000;
                        Shear_Timer = 35000;
                        Phase_Change = true;
                        Phase = 1;
                    }
                }
            }

            // Wave of Light and Wave of Twilight
            if (Wave_Timer <= diff)
            {
                float angle = me->GetOrientation();
                float orientation = angle + (M_PI); // + 180 Digrees so it goes to 
                orientation = MapManager::NormalizeOrientation(orientation);
                float distance = 30.0f;
                if (Phase == 0)
                {
                    me->SummonCreature(NPC_WAVE_Of_VIRTUE, 3546.97f + cos(angle)*distance, 273.10 + sin(angle)*distance, -114.03f, orientation, TEMPSUMMON_TIMED_DESPAWN, 13000);
                    me->MonsterYell("The Light will consume you!", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(25868, true);
                }
                else
                {
                    me->SummonCreature(NPC_WAVE_OF_TWILIGHT, 3546.97f + cos(angle)*distance, 273.10 + sin(angle)*distance, -114.03f, orientation, TEMPSUMMON_TIMED_DESPAWN, 13000);
                    me->MonsterYell("Drown in Shadow!", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(25869, true);
                }
                Wave_Timer = 60000;
            }
            else Wave_Timer -= diff;

            // Righteous Shear and Twilight Shear
            if (Shear_Timer <= diff)
            {
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true);
                if (target)
                {
                    if (Phase == 0)
                    {
                        me->CastSpell(target, RIGHTEOUS_SHEAR, true);
                        me->CastSpell(target, RIGHTEOUS_SHEAR, true);
                        Clean_Count = 0;
                        Thrall_Clean_Timer = 2000;
                        Thrall_Clean = true;
                    }
                    else
                    {
                        me->CastSpell(target, TWILIGHT_SHEAR, true);
                        me->CastSpell(target, TWILIGHT_SHEAR, true);
                    }
                }
                Shear_Timer = 45000;
            }
            else Shear_Timer -= diff;

            // Thrall`s spells
            if (Thrall_Lightning == true)
            {
                // Help destroy 2 purifying lights in phase 0
                if (Thrall_Lightning_Timer <= diff)
                {
                    if (Lightning_Count == 0)
                    {
                        Creature * thrall = me->FindNearestCreature(THRALL, 100.0f, true);
                        if (thrall)
                        {
                            Creature * purifying_light = me->FindNearestCreature(NPC_PURIFYING_LIGHT, 100.0f, true);
                            if (purifying_light)
                            {
                                thrall->InterruptNonMeleeSpells(false, 0, true);
                                thrall->CastSpell(purifying_light, CHAIN_LIGHTNING, false);
                                thrall->GetAI()->DoAction();
                            }
                        }
                        Thrall_Lightning_Timer = 2600;
                        Lightning_Count++;
                    }
                    else
                    {
                        Creature * thrall = me->FindNearestCreature(THRALL, 100.0f, true);
                        if (thrall)
                        {
                            Creature * purifying_light = me->FindNearestCreature(NPC_PURIFYING_LIGHT, 100.0f, true);
                            if (purifying_light)
                                thrall->CastSpell(purifying_light, CHAIN_LIGHTNING, true);
                        }
                        Thrall_Lightning = false;
                        Lightning_Count = 0;
                    }
                }
                else Thrall_Lightning_Timer -= diff;
            }

            if (Thrall_Clean == true)
            {
                // Help players dispell Righteous Shear in phase 0
                if (Thrall_Clean_Timer <= diff)
                {
                    // Remove debuff from players
                    Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                    for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                        if (Player* player = i->getSource())
                        {
                            if (player->HasAura(RIGHTEOUS_SHEAR))
                            {
                                Creature * thrall = me->FindNearestCreature(THRALL, 100.0f, true);
                                if (thrall)
                                    thrall->CastSpell(player, CLEANSE_SPIRIT, true);
                            }
                        }

                    Clean_Count++;
                    Thrall_Clean_Timer = 3000;

                    if (Clean_Count == 2)
                        Thrall_Clean = false;
                }
                else Thrall_Clean_Timer -= diff;
            }

            DoMeleeAttackIfReady();
        }
    };
};

// Npc Archbishop Benedictus Intro - 119514
class npc_archbishop_benedictus_intro : public CreatureScript
{
public:
    npc_archbishop_benedictus_intro() : CreatureScript("npc_archbishop_benedictus_intro") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_archbishop_benedictus_introAI(pCreature);
    }

    struct npc_archbishop_benedictus_introAI : public ScriptedAI
    {
        npc_archbishop_benedictus_introAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Intro_Timer;
        int Intro_Action;

        void JustDied(Unit* /*who*/) { }

        void Reset()
        {
            Intro_Timer = 0;
            Intro_Action = 0;
            me->SetVisible(false);
        }

        void UpdateAI(const uint32 diff)
        {
            // Benedictus Intro
            if (instance)
            {
                if ((instance->GetData(DATA_BENEDICTUS_INTRO)) == 1)
                {
                    if (Intro_Timer <= diff)
                    {
                        Intro_Timer = 1000;

                        switch (Intro_Action) {
                        case 0: // Appear and go
                            me->GetMotionMaster()->MovePoint(0, BenedictusMovePoints[0][0], BenedictusMovePoints[0][1], BenedictusMovePoints[0][2]);
                            me->SetVisible(true);
                            Intro_Timer = 8000;
                            Intro_Action++;
                            break;
                        case 1: // Run towards Thrall
                            me->GetMotionMaster()->MovePoint(0, BenedictusMovePoints[1][0], BenedictusMovePoints[1][1], BenedictusMovePoints[1][2]);
                            Intro_Timer = 10000;
                            Intro_Action++;
                            break;
                        case 2: // Say
                            me->MonsterSay("Get inside, quickly! I'll hold them off!", LANG_UNIVERSAL, 0, 100.0f);
                            me->SendPlaySound(25866, true);
                            Intro_Action++;
                            Intro_Timer = 10000;
                            break;
                        case 3: // Run back
                            me->GetMotionMaster()->MovePoint(0, BenedictusMovePoints[2][0], BenedictusMovePoints[2][1], BenedictusMovePoints[2][2]);
                            Intro_Timer = 8000;
                            Intro_Action++;
                            break;
                        case 4: // Move
                            me->GetMotionMaster()->MovePoint(0, BenedictusMovePoints[3][0], BenedictusMovePoints[3][1], BenedictusMovePoints[3][2]);
                            Intro_Timer = 5000;
                            Intro_Action++;
                            break;
                        case 5: // Cast Holly Wall
                            {
                                Creature * holly_wall = me->FindNearestCreature(NPC_HOLLY_WALL, 50.0f, true);
                                if (holly_wall)
                                    me->CastSpell(holly_wall, HOLY_WALL, false);
                                Intro_Timer = 5000;
                                Intro_Action++;
                            }
                            break;
                        case 6: // Move
                        case 7: // Move
                        case 8: // Move
                            if (me->GetExactDist2d(BenedictusMovePoints[Intro_Action-3][0], BenedictusMovePoints[Intro_Action-3][1]) < 1)
                            {
                                me->GetMotionMaster()->MovePoint(0, BenedictusMovePoints[Intro_Action-2][0], BenedictusMovePoints[Intro_Action-2][1], BenedictusMovePoints[Intro_Action-2][2]);
                                if (Intro_Action == 8)
                                    Intro_Timer = 5000;
                                Intro_Action++;
                            }
                            break;
                        case 9: // Move
                            if (me->GetExactDist2d(BenedictusMovePoints[6][0], BenedictusMovePoints[6][1]) < 1)
                            {
                                Creature * benedictus = me->FindNearestCreature(BOSS_ARCHBISHOP_BENEDICTUS, 10.0f, true);
                                if (benedictus)
                                {
                                    me->GetMotionMaster()->MovePoint(0, benedictus->GetPositionX(), benedictus->GetPositionY(), benedictus->GetPositionZ());
                                    Intro_Action++;
                                    Intro_Timer = 2000;
                                }
                            }
                            break;
                        case 10: // Say
                            me->MonsterSay("And now, Shaman, you will give the Dragon Soul to ME.", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25867, true);
                            Intro_Timer = 5000;
                            Intro_Action++;
                            break;
                        case 11: // Thrall Say
                            {
                                Creature * thrall = me->FindNearestCreature(THRALL, 50.0f, true);
                                if (thrall)
                                {
                                    thrall->MonsterSay("I will NOT, Archbishop. It will never be yours.", LANG_UNIVERSAL, 0);
                                    me->SendPlaySound(25893, true);
                                }
                                Intro_Timer = 6000;
                                Intro_Action++;
                            }
                            break;
                        case 12: // Say
                            me->MonsterSay("I suppose it has to be this way, then. If only you'd seen what I've seen. THEN you'd understand.", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(21410, true);
                            Intro_Timer = 8000;
                            Intro_Action++;
                            break;
                        case 13: // Thrall Say
                            {
                                Creature * thrall = me->FindNearestCreature(THRALL, 50.0f, true);
                                if (thrall)
                                {
                                    thrall->MonsterSay("You were a figurehead of the Light, Benedictus. How could you betray your own people?", LANG_UNIVERSAL, 0);
                                    me->SendPlaySound(25894, true);
                                }
                                Intro_Timer = 8000;
                                Intro_Action++;
                            }
                            break;
                        case 14: // Say
                            me->MonsterSay("There is no good. No evil. No Light. There is only POWER!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(21411, true);
                            Intro_Timer = 9000;
                            Intro_Action++;
                            break;
                        case 15: // Say
                            me->MonsterSay("We serve the world's TRUE masters! When their rule begins, we will share in their glory!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(21412, true);
                            Intro_Timer = 10000;
                            Intro_Action++;
                            break;
                        case 16: // Say
                            me->MonsterSay("And YOU! We will FEAST on your ashes! Now, DIE!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(21413, true);
                            Intro_Timer = 9000;
                            Intro_Action++;
                            break;
                        case 17: // Disappear
                            {
                                Creature * benedictus = me->FindNearestCreature(BOSS_ARCHBISHOP_BENEDICTUS, 50.0f, true);
                                if (benedictus)
                                {
                                    benedictus->SetVisible(true);
                                    benedictus->setFaction(16);
                                }
                                me->SetVisible(false);
                                instance->SetData(DATA_BENEDICTUS_INTRO, 2); // 2
                            }
                            break;
                        default:
                            break;
                        }
                    }
                    else Intro_Timer -= diff;
                }
            }
        }
    };
};

// Npc Wave of Twilight - 55469
class npc_wave_of_twilight : public CreatureScript
{
public:
    npc_wave_of_twilight() : CreatureScript("npc_wave_of_twilight") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_wave_of_twilightAI(pCreature);
    }

    struct npc_wave_of_twilightAI : public ScriptedAI
    {
        npc_wave_of_twilightAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        void JustDied(Unit* /*who*/) { }

        void DamageDealt(Unit* victim, uint32& /*damage*/, DamageEffectType typeOfDamage)
        {
            if (typeOfDamage == SPELL_DIRECT_DAMAGE)
                victim->JumpTo(8.0f, 15.0f, false); // leap back
        }

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, WAVE_OF_TWILIGHT_1, false);

            float angle = me->GetOrientation();
            float distance = 70.0;
            me->GetMotionMaster()->MovePoint(0, me->GetPositionX()+cos(angle)*distance, me->GetPositionY()+sin(angle)*distance, me->GetPositionZ(), true);
        }

        void UpdateAI(const uint32 diff) { }
    };
};

// Npc Wave of Virtue - 55441
class npc_wave_of_virtue : public CreatureScript
{
public:
    npc_wave_of_virtue() : CreatureScript("npc_wave_of_virtue") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_wave_of_virtueAI(pCreature);
    }

    struct npc_wave_of_virtueAI : public ScriptedAI
    {
        npc_wave_of_virtueAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        void JustDied(Unit* /*who*/) { }

        void DamageDealt(Unit* victim, uint32& /*damage*/, DamageEffectType typeOfDamage)
        {
            if (typeOfDamage == SPELL_DIRECT_DAMAGE)
                victim->JumpTo(8.0f, 15.0f, false); // leap back
        }

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, WAVE_OF_VIRTUE_2, false);

            float angle = me->GetOrientation();
            float distance = 70.0;
            me->GetMotionMaster()->MovePoint(0, me->GetPositionX() + cos(angle)*distance, me->GetPositionY() + sin(angle)*distance, me->GetPositionZ(), true);
        }

        void UpdateAI(const uint32 diff) { }
    };
};

// Npc Platform Aura - 55441
class npc_platform_aura : public CreatureScript
{
public:
    npc_platform_aura() : CreatureScript("npc_platform_aura") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_platform_auraAI(pCreature);
    }

    struct npc_platform_auraAI : public ScriptedAI
    {
        npc_platform_auraAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Check_Timer;
        uint32 Dmg_Timer;
        bool Damage;

        void JustDied(Unit* /*who*/) { }

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);
            Check_Timer = 5000;
            Dmg_Timer = 1000;
            Damage = false;
        }

        void UpdateAI(const uint32 diff) 
        {
            if (Check_Timer <= diff)
            {
                if (instance && Damage == false)
                    if (instance->GetData(TYPE_BOSS_ARCHBISHOP_BENEDICTUS) == IN_PROGRESS)
                        Damage = true;

                if (instance && Damage == true)
                    if (instance->GetData(TYPE_BOSS_ARCHBISHOP_BENEDICTUS) == NOT_STARTED ||
                        instance->GetData(TYPE_BOSS_ARCHBISHOP_BENEDICTUS) == DONE)
                        Damage = false;

                Check_Timer = 5000;
            }
            else Check_Timer -= diff;

            if (Damage == true)
            {
                if (Dmg_Timer <= diff)
                {
                    Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                    for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                        if (Player* player = i->getSource())
                        {
                            if (me->GetExactDist2d(player) > 40)
                            {
                                if (me->HasAura(SEAPING_LIGHT))
                                    me->CastSpell(player, SEAPING_LIGHT_1, true);

                                if (me->HasAura(SEAPING_TWILIGHT))
                                    me->CastSpell(player, SEAPING_TWILIGHT_1, true);
                            }
                        }
                    Dmg_Timer = 1000;
                }
                else Dmg_Timer -= diff;
            }
        }
    };
};

// Npc Purifying Light - 55377
class npc_purifying_light : public CreatureScript
{
public:
    npc_purifying_light() : CreatureScript("npc_purifying_light") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_purifying_lightAI(pCreature);
    }

    struct npc_purifying_lightAI : public ScriptedAI
    {
        npc_purifying_lightAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Jump_Timer;
        uint32 Kick_Timer;

        void JustDied(Unit* /*who*/) { }

        void Reset()
        {
            me->SetFlying(true);
            me->SetWalk(true);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            //me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, PURIFYING_LIGHT_SHINE, false);
            me->CastSpell(me, PURIFYING_LIGHT_GLOW, false);
            me->SetInCombatWithZone();
            Jump_Timer = 8000;
            Kick_Timer = 9000;
            me->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ()+6, true);
        }

        void SpellHit(Unit* /*caster*/, SpellEntry const* spell)
        {
            if (spell->Id == CHAIN_LIGHTNING)
            {
                me->ForcedDespawn(500);
            }
        }

        void UpdateAI(const uint32 diff) 
        {
            if (Jump_Timer <= diff)
            {
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true);
                if (target)
                    me->CastSpell(target, PURIFYING_BLAST, false);
                Jump_Timer = 10000;
            }
            else Jump_Timer -= diff;

            if (Kick_Timer <= diff)
            {
                me->CastSpell(me, PURIFYING_BLAST_1, false);
                Kick_Timer = 10000;
            }
            else Kick_Timer -= diff;
        }
    };
};

// Npc Purifying Blast - 55427
class npc_purifying_blast : public CreatureScript
{
public:
    npc_purifying_blast() : CreatureScript("npc_purifying_blast") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_purifying_blastAI(pCreature);
    }

    struct npc_purifying_blastAI : public ScriptedAI
    {
        npc_purifying_blastAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        void JustDied(Unit* /*who*/) { }

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, PURIFIED, false);
        }

        void UpdateAI(const uint32 diff) { }
    };
};

// Npc Corrupting Twilight - 55467
class npc_corrupting_twilight : public CreatureScript
{
public:
    npc_corrupting_twilight() : CreatureScript("npc_corrupting_twilight") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_corrupting_twilightAI(pCreature);
    }

    struct npc_corrupting_twilightAI : public ScriptedAI
    {
        npc_corrupting_twilightAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Kick_Timer;

        void JustDied(Unit* /*who*/) { }

        void Reset()
        {
            me->SetFlying(true);
            me->SetWalk(true);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, CORRUPTING_TWILIGHT_SHINE, false);
            me->CastSpell(me, CORRUPTING_TWILIGHT_GLOW, false);
            me->SetInCombatWithZone();
            Kick_Timer = 9000;
            me->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() + 6, true);
        }

        void UpdateAI(const uint32 diff)
        {
            if (Kick_Timer <= diff)
            {
                me->CastSpell(me, TWILIGHT_BOLT_1, false);
                Kick_Timer = 10000;
            }
            else Kick_Timer -= diff;
        }
    };
};

// Npc Twilight Bolt - 55468
class npc_twilight_bolt : public CreatureScript
{
public:
    npc_twilight_bolt() : CreatureScript("npc_twilight_bolt") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_twilight_boltAI(pCreature);
    }

    struct npc_twilight_boltAI : public ScriptedAI
    {
        npc_twilight_boltAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        void JustDied(Unit* /*who*/) { }

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, TWILIGHT, false);
        }

        void UpdateAI(const uint32 diff) { }
    };
};

// Npc Water Shell - 55447
class npc_water_shell : public CreatureScript
{
public:
    npc_water_shell() : CreatureScript("npc_water_shell") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_water_shellAI(pCreature);
    }

    struct npc_water_shellAI : public ScriptedAI
    {
        npc_water_shellAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        void JustDied(Unit* /*who*/) { }

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->setFaction(35);
        }

        void UpdateAI(const uint32 diff) { }
    };
};

// Npc Twilight Sparkle - 55466
class npc_twilight_sparkle : public CreatureScript
{
public:
    npc_twilight_sparkle() : CreatureScript("npc_twilight_sparkle") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_twilight_sparkleAI(pCreature);
    }

    struct npc_twilight_sparkleAI : public ScriptedAI
    {
        npc_twilight_sparkleAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        void JustDied(Unit* /*who*/) 
        { 
            Creature * benedictus = me->FindNearestCreature(BOSS_ARCHBISHOP_BENEDICTUS, 200.0f, true);
            if (benedictus)
                benedictus->GetAI()->DoAction();
        }

        void Reset()
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetFlying(true);
            me->CastSpell(me, UNSTABLE_TWILIGHT, false);
        }

        void UpdateAI(const uint32 diff) { }
    };
};

void AddSC_boss_archbishop_benedictus()
{
    new boss_archbishop_benedictus();
    new npc_archbishop_benedictus_intro();
    new npc_wave_of_twilight();
    new npc_wave_of_virtue();
    new npc_platform_aura();
    new npc_purifying_light();
    new npc_purifying_blast();
    new npc_twilight_bolt();
    new npc_corrupting_twilight();
    new npc_water_shell();
    new npc_twilight_sparkle();
}