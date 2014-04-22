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
Encounter: Echo of Tyrande
Dungeon: End Time
Difficulty: Heroic
Mode: 5-man
Autor: Lazik
Complete: 100%
*/

#include "ScriptPCH.h"
#include "Spell.h"
#include "UnitAI.h"
#include "MapManager.h" 

// NPCs
enum NPC
{
    ECHO_OF_TYRANDE         = 54544,
    EYE_OF_ELUNE            = 54941,
    MOONLANCE_MAIN_ADD      = 54574,
    MOONLANCE_ADD           = 54580,
    PURPLE_AURA_HOLDER      = 119501,
};

// Spells
enum Spells 
{
    DARK_MOONLIGHT              = 102414, // Aura slowing casting speed
    MOONBOLT                    = 102193, // Dmg Spell
    STARDUST                    = 102173, // Dmg Spell
    LUNAR_GUIDANCE              = 102472, // Buff
    TEARS_OF_ELUNE              = 102241, // Cast
    TEARS_RAIN_FALL             = 102243, // Dmg on target nearby destionation

    EYES_OF_THE_GODDESS         = 102181, // Visual cast of summons

    MOONLANCE                   = 102151, // 2s cast + Summon
    MOONLANCE_AURA              = 102150, // Buff
    MOONLANCE_EFFECT            = 102149, // Effect
    MOONLANCE_SUMMON_ADDS       = 102152, // Summon 3 adds

    PIERCING_GAZE_OF_ELUNE      = 102182, // Aura
    PIERCING_GAZE_EFFECT        = 102183, // Effect
};

// Echo of Tyrande
class boss_echo_of_tyrande : public CreatureScript
{
public:
    boss_echo_of_tyrande() : CreatureScript("boss_echo_of_tyrande") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_echo_of_tyrandeAI (pCreature);
    }

    struct boss_echo_of_tyrandeAI : public ScriptedAI
    {
        boss_echo_of_tyrandeAI(Creature *c) : ScriptedAI(c) {}

        uint32 Check_Timer;
        uint32 Eyes_Of_The_Goddess_Timer;
        uint32 Phase;
        uint32 Tears_Of_Elune_Timer;
        uint32 Moonbolt_Timer;
        uint32 Stardust_Timer;
        uint32 Moonlance_Timer;
        uint32 Summon_Eyes_Timer;
        uint32 Summon_Moonlance_Timer;
        bool First_Lunar_Guidance;
        bool Second_Lunar_Guidance;
        bool Tears_Of_Elune;
        bool Eyes_Of_The_Goddess;
        bool Stardust;
        bool Moonlance;
        bool Summon_Eyes_Successful;
        bool Summon_Moonlance_Successful;
        bool Say_Moonlance;

        void Reset() 
        {
            me->SetStandState(UNIT_STAND_STATE_KNEEL);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);

            Eyes_Of_The_Goddess_Timer = 30000;
            Check_Timer = 1000;
            Moonbolt_Timer = 1500;
            Stardust_Timer = 4400;
            Moonlance_Timer = 60000;

            Phase = 0;

            First_Lunar_Guidance = false;
            Second_Lunar_Guidance = false;
            Tears_Of_Elune = false;
            Eyes_Of_The_Goddess = false;
            Stardust = false;
            Moonlance = false;
            Summon_Eyes_Successful = false;
            Summon_Moonlance_Successful = false;
            Say_Moonlance = false;
        }

        void InEvadeMode() {}

        void EnterCombat(Unit * /*who*/)
        {
            me->SetStandState(UNIT_STAND_STATE_STAND);

            me->MonsterYell("Let the peaceful light of Elune soothe your souls in this dark time.", LANG_UNIVERSAL, 0);
            me->SendPlaySound(25972, true);

            me->SetInCombatWithZone();
            Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true);
            if (target)
                me->CastSpell(target, MOONBOLT, false);
        }

        void KilledUnit(Unit* /*victim*/)
        {
            me->MonsterYell("Elune guide you through the night.", LANG_UNIVERSAL, 0);
            me->SendPlaySound(25982, true);
        }

        void JustDied(Unit * /*who*/)
        {
            me->MonsterYell("I can...see the light of the moon...so clearly now. It is...beautiful...", LANG_UNIVERSAL, 0);
            me->SendPlaySound(25976, true);
        }

        void SummonEyesOfTheGoddess()
        {
            float angle = me->GetOrientation();
            double angleAddition = (M_PI); // 180 Digrees - 2 Eyes of Goddess 
            angle += (M_PI/2); // + 90 Digrees so it doesn`t go throught tank
            angle = MapManager::NormalizeOrientation(angle);
            float distance = 1.5f;

            for(uint32 i = 0; i < 2;)
            {
                me->SummonCreature(EYE_OF_ELUNE,me->GetPositionX()+cos(angle)*distance,me->GetPositionY()+sin(angle)*distance,me->GetPositionZ(),angle,TEMPSUMMON_TIMED_DESPAWN, 45000);
                angle += angleAddition;
                i = i + 1;
            }
        }

        void SummonMoonlance()
        {
            float angle = me->GetOrientation();
            float distance = 1.0f;

            me->SummonCreature(MOONLANCE_MAIN_ADD,me->GetPositionX()+cos(angle)*distance,me->GetPositionY()+sin(angle)*distance,me->GetPositionZ(),angle,TEMPSUMMON_TIMED_DESPAWN, 30000);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
            return;

            if (Check_Timer <= diff)
            {
                // First Lunar Gudance
                if (HealthBelowPct(80) && Phase == 0)
                {
                    First_Lunar_Guidance = true;

                    if (me->hasUnitState(UNIT_STAT_CASTING))
                        return;

                    me->MonsterYell("Moon goddess, your light dims! I am lost without your guidance!", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(25973, true);
                    me->CastSpell(me, LUNAR_GUIDANCE, false);
                    Stardust_Timer += 5000;
                    First_Lunar_Guidance = false;
                    Phase = 1;
                }
                // Second Lunar Guidance
                if (HealthBelowPct(55) && Phase == 1)
                {
                    Second_Lunar_Guidance = true;

                    if (me->hasUnitState(UNIT_STAT_CASTING))
                        return;

                    me->MonsterYell("The darkness closes in...my vision is clouded...", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(25974, true);
                    me->CastSpell(me, LUNAR_GUIDANCE, false);
                    Stardust_Timer += 4000;
                    Second_Lunar_Guidance = false;
                    Phase = 2;
                }
               // Tears of Elune
                if (HealthBelowPct(30) && Phase == 2)
                {
                    Tears_Of_Elune = true;

                    if (me->hasUnitState(UNIT_STAT_CASTING))
                        return;

                    me->MonsterYell("Mother moon, I can no longer see your light! Your daughter is alone in the darkness!", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(25975, true);
                    me->CastSpell(me, TEARS_OF_ELUNE, false);
                    Tears_Of_Elune_Timer = 6000;
                    Tears_Of_Elune = false;
                    Phase = 3;
                }
                Check_Timer = 1000;
            } else Check_Timer -= diff;

            // Final Rainfall of Stars
            if ((Tears_Of_Elune_Timer <= diff) && (Phase == 3))
            {
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM,0,200.0f,true);
                if (target) {
                    me->CastSpell(target, TEARS_RAIN_FALL, true);
                    Tears_Of_Elune_Timer = 700;
                }
            }
            else Tears_Of_Elune_Timer -= diff;

            // Visual cast of adds
            if ((Eyes_Of_The_Goddess_Timer <= diff) && (First_Lunar_Guidance == false) && (Second_Lunar_Guidance == false) && 
                (Tears_Of_Elune == false))
            {
                Eyes_Of_The_Goddess = true;

                if (me->hasUnitState(UNIT_STAT_CASTING))
                    return;

                me->CastSpell(me, EYES_OF_THE_GODDESS, false);
                me->MonsterYell("Eyes of night, pierce this darkness!", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25983, true);
                Summon_Eyes_Timer = 3000;
                Summon_Eyes_Successful = true;
                Eyes_Of_The_Goddess_Timer = 45000;
                Eyes_Of_The_Goddess = false;
            }
            else Eyes_Of_The_Goddess_Timer -= diff;

            // Summon Eyes of Goddess
            if ((Summon_Eyes_Timer <= diff) && (Summon_Eyes_Successful == true))
            {
                SummonEyesOfTheGoddess();
                Summon_Eyes_Successful = false;
            }
            else Summon_Eyes_Timer -= diff;

            // Summon Moonlance
            if ((Summon_Moonlance_Timer <= diff) && (Summon_Moonlance_Successful == true))
            {
                SummonMoonlance();
                Summon_Moonlance_Successful = false;
            }
            else Summon_Moonlance_Timer -= diff;

            // Stardust
            if ((Stardust_Timer <= diff) && (First_Lunar_Guidance == false) && (Second_Lunar_Guidance == false) && 
                (Tears_Of_Elune == false) && (Eyes_Of_The_Goddess == false))
            {
                Stardust = true;

                if (me->hasUnitState(UNIT_STAT_CASTING))
                    return;

                Unit * target = SelectUnit(SELECT_TARGET_TOPAGGRO, 0);
                if (!target) target = me->getVictim();
                if (target) {
                    me->CastSpell(target, STARDUST, false);
                    switch(Phase) {
                        case 0: 
                            Stardust_Timer = 6500;
                            Moonlance_Timer = 3000;
                            break;
                        case 1:
                            Stardust_Timer = 6400;
                            Moonlance_Timer = 2400;
                            break;
                        case 2:
                        case 3:
                            Stardust_Timer = 5333;
                            Moonlance_Timer = 2000;
                            break;
                    }
                    Stardust = false;
                }
            }
            else Stardust_Timer -= diff;

            // Moonlance
            if ((Moonlance_Timer <= diff) && (First_Lunar_Guidance == false) && (Second_Lunar_Guidance == false) && 
                (Tears_Of_Elune == false) && (Eyes_Of_The_Goddess == false) && (Stardust == false))
            {
                Moonlance = true;

                if (me->hasUnitState(UNIT_STAT_CASTING))
                    return;

                if (Say_Moonlance == false) {
                    me->MonsterYell("Spear of Elune, drive back the night!", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(25984, true);
                    Say_Moonlance = true;
                }

                me->CastSpell(me, MOONLANCE, false);
                Summon_Moonlance_Successful = true;

                switch(Phase) {
                    case 0: 
                        Moonbolt_Timer = 2000;
                        Summon_Moonlance_Timer = 2000;
                        break;
                    case 1:
                        Moonbolt_Timer = 1600;
                        Summon_Moonlance_Timer = 1600;
                        break;
                    case 2:
                    case 3:
                        Moonbolt_Timer = 1333;
                        Summon_Moonlance_Timer = 1333;
                        break;
                }
                Moonlance_Timer = 20000;
                Moonlance = false;
            }
            else Moonlance_Timer -= diff;

            // Moonbolt
            if ((Moonbolt_Timer <= diff) && (First_Lunar_Guidance == false) && (Second_Lunar_Guidance == false) && 
                (Tears_Of_Elune == false) && (Eyes_Of_The_Goddess == false) && (Stardust == false) && 
                (Moonlance == false))
            {
                if (me->hasUnitState(UNIT_STAT_CASTING))
                    return;

                Unit * target = SelectUnit(SELECT_TARGET_TOPAGGRO, 0);
                if (!target) target = me->getVictim();
                if (target) {
                    me->CastSpell(target, MOONBOLT, false);
                    switch(Phase) {
                        case 0: 
                            Moonbolt_Timer = 1500;
                            break;
                        case 1:
                            Moonbolt_Timer = 1200;
                            break;
                        case 2:
                        case 3:
                            Moonbolt_Timer = 1000;
                            break;
                    }
                }
            }
            else Moonbolt_Timer -= diff;
        }
    };
};

// Eye of Elune
class eye_of_elune : public CreatureScript
{
public:
    eye_of_elune() : CreatureScript("eye_of_elune") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new eye_of_eluneAI (pCreature);
    }

    struct eye_of_eluneAI : public ScriptedAI
    {
        eye_of_eluneAI(Creature *c) : ScriptedAI(c) {}

        uint32 Move;
        uint32 Move_Circle;
        uint32 Move_Seconds_Count;
        uint32 Waypoint;

        struct Waypoints
        {
            float x;
            float y;
        };
        Waypoints Coordinates[40];

        void MakeWaypoints()
        {
            float angle = me->GetOrientation();
            float angleAddition = (2*M_PI)/40;
            angle -= angleAddition;

            Unit * tyrande = me->FindNearestCreature(ECHO_OF_TYRANDE, 100.0, true);
            float distance = me->GetDistance(tyrande->GetPositionX(), tyrande->GetPositionY(), tyrande->GetPositionZ());

            angle = MapManager::NormalizeOrientation(angle);

            for(uint32 i = 0; i < 40; i++)
            {
                Coordinates[i].x = tyrande->GetPositionX()+cos(angle)*distance;
                Coordinates[i].y = tyrande->GetPositionY()+sin(angle)*distance;
                angle -= angleAddition;
            }
        }

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, PIERCING_GAZE_OF_ELUNE, false);

            Move = 300;
            Move_Seconds_Count = 0;
            Move_Circle = 10000;

            Waypoint = 0;
        }

        void EnterCombat(Unit * /*who*/) { }

        void StopMove() 
        {
            me->GetMotionMaster()->MovementExpired();
            me->GetMotionMaster()->Clear();
            me->GetMotionMaster()->MoveIdle();
            me->SendMovementFlagUpdate();

            MakeWaypoints();
            Move_Circle = 1;
        }

        void UpdateAI(const uint32 diff)
        {
            if ((Move <= diff) && (Move_Seconds_Count < 7))
            {
                float angle = me->GetOrientation();
                float distance;

                if (Move_Seconds_Count == 6) {
                    StopMove();
                    distance = 1;
                } 
                else
                    distance = 6;

                me->GetMotionMaster()->MovePoint(EYE_OF_ELUNE, me->GetPositionX()+cos(angle)*distance,me->GetPositionY()+sin(angle)*distance, me->GetPositionZ());
                Move = 1000;
                Move_Seconds_Count++;
            } 
            else Move -= diff;

            if (Move_Circle <= diff)
            {
                me->GetMotionMaster()->MovePoint(EYE_OF_ELUNE, Coordinates[Waypoint].x, Coordinates[Waypoint].y, me->GetPositionZ());
                if (Waypoint == 39) {
                    Waypoint = -1;
                }
                Waypoint++;
                Move_Circle = 550;
            } 
            else Move_Circle -= diff;
        }
    };
};

// Main Moonlance
class moonlance : public CreatureScript
{
public:
    moonlance() : CreatureScript("moonlance") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new moonlanceAI (pCreature);
    }

    struct moonlanceAI : public ScriptedAI
    {
        moonlanceAI(Creature *c) : ScriptedAI(c) {}

        uint32 Release_Timer;
        bool Kill;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, MOONLANCE_AURA, false);
            Release_Timer = 3000;
            Kill = false;
        }

        void EnterCombat(Unit * /*who*/) { }

        void SummonMoonlanceAdds()
        {
            Creature * moonlance = me->FindNearestCreature(MOONLANCE_MAIN_ADD, 300, true);

            float angle = moonlance->GetOrientation();
            double angleAddition = (2*M_PI)/12; // 30 Digries
            angle = angle-angleAddition;
            angle = MapManager::NormalizeOrientation(angle);
            float distance = 1.5f;

            for(uint32 i = 0; i < 3;)
            {
                moonlance->SummonCreature(MOONLANCE_ADD,moonlance->GetPositionX()+cos(angle)*distance,moonlance->GetPositionY()+sin(angle)*distance,moonlance->GetPositionZ(),angle,TEMPSUMMON_TIMED_DESPAWN, 8000);
                angle += angleAddition;
                i = i + 1;
            }
        }

        void UpdateAI(const uint32 diff)
        {
            float angle = me->GetOrientation();
            float distance = 5;
            me->GetMotionMaster()->MovePoint(MOONLANCE_MAIN_ADD, me->GetPositionX()+cos(angle)*distance,me->GetPositionY()+sin(angle)*distance, me->GetPositionZ());

            if (Release_Timer <= diff)
            {
                SummonMoonlanceAdds();
                Kill = true;
            } 
            else Release_Timer -= diff;

            if (Kill == true)
                me->Kill(me, false);
        }
    };
};

// Moonlance Adds
class moonlance_add : public CreatureScript
{
public:
    moonlance_add() : CreatureScript("moonlance_add") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new moonlance_addAI (pCreature);
    }

    struct moonlance_addAI : public ScriptedAI
    {
        moonlance_addAI(Creature *c) : ScriptedAI(c) {}

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, MOONLANCE_AURA, false);
        }

        void EnterCombat(Unit * /*who*/) { }

        void UpdateAI(const uint32 diff)
        {
            float angle = me->GetOrientation();
            float distance = 5;
            me->GetMotionMaster()->MovePoint(MOONLANCE_ADD, me->GetPositionX()+cos(angle)*distance,me->GetPositionY()+sin(angle)*distance, me->GetPositionZ());
        }
    };
};

// Purple Aura
class purple_aura_holder : public CreatureScript
{
public:
    purple_aura_holder() : CreatureScript("purple_aura_holder") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new purple_aura_holderAI (pCreature);
    }

    struct purple_aura_holderAI : public ScriptedAI
    {
        purple_aura_holderAI(Creature *c) : ScriptedAI(c) {}

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, DARK_MOONLIGHT, false);
        }

        void EnterCombat(Unit * /*who*/) { }
        void UpdateAI(const uint32 diff) { }
    };
};

void AddSC_boss_echo_of_tyrande()
    {
        new boss_echo_of_tyrande();
        new eye_of_elune();
        new moonlance();
        new moonlance_add();
        new purple_aura_holder();
    }

//////////////////////////////////////////////////////////////////////////////////////
///////////////////////////         SQL QUERY          ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
/*
--Echo of Tyrande
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES('54544','0','0','0','0','0','39617','0','0','0','Echo of Tyrande','','','0','87','87','3','16','16','0','1','1','1','3','0','0','0','0','1','2000','2000','2','0','0','0','0','0','0','0','0','0','0','7','72','54544','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','20000','20000','','0','3','59.33441','38.6','1','0','0','0','0','0','0','0','0','1','54544','613367551','1','boss_echo_of_tyrande','15595');

--Eyes of Elune
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('54941','0','0','0','0','0','11686','0','0','0','Eye of Elune','','','0','87','87','3','16','16','0','1','1','1','0','0','0','0','0','1','0','0','1','0','0','0','0','0','0','0','0','0','0','10','1074790400','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','0','3','1','1','1','0','0','0','0','0','0','0','169','1','0','0','0','eye_of_elune','15595');

--Moonlance Main
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('54574','0','0','0','0','0','11686','0','0','0','Moonlance','','','0','87','87','3','16','16','0','1','1','1','0','0','0','0','0','1','0','0','1','0','0','0','0','0','0','0','0','0','0','10','1074790400','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','0','3','1','1','1','0','0','0','0','0','0','0','169','1','0','0','0','moonlance','15595');

--Moonlance Add
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('54580','0','0','0','0','0','11686','0','0','0','Moonlance','','','0','87','87','3','16','16','0','1','1','1','0','0','0','0','0','1','0','0','1','0','0','0','0','0','0','0','0','0','0','10','1074790400','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','0','3','1','1','1','0','0','0','0','0','0','0','169','1','0','0','0','moonlance_add','15595');

--Purple Aura Holder
INSERT INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('119501','0','0','0','0','0','11686','0','0','0','Purple Aura Holder','','','0','88','88','3','16','16','0','1','1.14286','0.5','0','0','0','0','0','1','0','0','1','0','0','0','0','0','0','0','0','0','0','10','1074790400','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','0','3','1','1','1','0','0','0','0','0','0','0','169','1','0','0','0','purple_aura_holder','15595');

--Loot
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54544','72798','3','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54544','72806','3','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54544','72800','3','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54544','72803','3','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54544','72807','3','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54544','72804','4','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54544','72805','4','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54544','72799','4','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54544','72801','4','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54544','72802','4','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54544','72813','34','1','1','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54544','72812','66','1','1','1','1');
*/
