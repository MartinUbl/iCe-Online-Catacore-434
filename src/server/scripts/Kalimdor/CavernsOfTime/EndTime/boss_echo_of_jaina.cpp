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
Encounter: Echo of Jaina
Dungeon: End Time
Difficulty: Heroic
Mode: 5-man
Autor: Lazik
*/

#include "ScriptPCH.h"
#include "Spell.h"
#include "UnitAI.h"
#include "MapManager.h" 
#include "endtime.h"

enum NPC
{
    ECHO_OF_JAINA         = 54445,
    FROST_BLADE           = 54494,
    FLARECORE_EMBER       = 54446,
    BLUE_AURA_HOLDER      = 119502,
};

// Spells
enum Spells 
{
    PYROBLAST               = 101809, // Dmg spell
    BLINK                   = 101812, // Visual effect of Blink
    FROST_VOLLEY            = 101810, // Dmg spell

    SUMMON_FROST_VOLLEYS    = 101339, // Summon NPC
    FROST_BLADE_BUFF        = 101338, // Apply visual effect of Blade model ID
    FREEZE                  = 101337, // Triggered by 101338, freezes a nearby player

    FLARECORE_FLY           = 101927, // Visual effect of flarecore, fly part
    FLARECORE_SUMMON        = 101628, // Summon effect NPC Flareroce Ember
    FLARE_UP                = 101589, // Add scale
    UNSTABLE_FLARE          = 101980, // 5 000 - 50 000 dmg when someone detonates it
    FLARE                   = 101587, // Detonation after 10s, when noone detonates it
    FLARE_VISUAL            = 101588, // Visual effect of Flare Ember

    VISUAL_BLUE_AURA        = 102206, // Blue aura
};

const float TeleportPoint[6][4]=
{
    {3052.10f, 489.98f, 21.7684f, 2.132f},
    {3024.83f, 518.80f, 22.2157f, 6.185f},
    {2989.04f, 506.10f, 26.4568f, 5.804f},
    {3006.73f, 486.72f, 25.2540f, 0.794f},
    {3005.81f, 539.14f, 27.4301f, 5.769f},
    {3037.51f, 549.34f, 21.2855f, 4.638f},
};

// Echo of Jaina
class boss_echo_of_jaina : public CreatureScript
{
public:
    boss_echo_of_jaina() : CreatureScript("boss_echo_of_jaina") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_echo_of_jainaAI (pCreature);
    }

    struct boss_echo_of_jainaAI : public ScriptedAI
    {
        boss_echo_of_jainaAI(Creature *creature) : ScriptedAI(creature) 
        {
            instance = creature->GetInstanceScript();

            me->SetVisible(false);
            me->setFaction(35);
        }

        InstanceScript* instance;
        int Phase;
        int Pyroblast_Counter;
        int Flarecore_Counter;
        int Frost_Volley_Counter;
        int Random_Text;
        int Port_Location;
        int Last_Port_Spot;
        uint32 Pyroblast_Timer;
        uint32 Frost_Volley_Timer;
        uint32 Blink_Timer;
        uint32 Blades_Timer;
        bool Summon_Frost_Blades;

        void Reset() 
        {
            if (instance)
            {
                if(instance->GetData(TYPE_ECHO_OF_JAINA)!=DONE)
                    instance->SetData(TYPE_ECHO_OF_JAINA, NOT_STARTED);
            }

            Unit * blue_aura_holder = me->FindNearestCreature(BLUE_AURA_HOLDER, 500.0f, true);
            if (blue_aura_holder)
                blue_aura_holder->CastSpell(blue_aura_holder, VISUAL_BLUE_AURA, false);

            Summon_Frost_Blades = false;
            Phase = 1;
            Pyroblast_Counter = 0;
            Flarecore_Counter = 0;
            Pyroblast_Timer = 3200;
            Port_Location = 0;
            Last_Port_Spot = 6;
        }

        void InEvadeMode();

        void EnterCombat(Unit * /*who*/)
        {
            if (instance)
            {
                instance->SetData(TYPE_ECHO_OF_JAINA, IN_PROGRESS);
            }

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);

            Unit * blue_aura_holder = me->FindNearestCreature(BLUE_AURA_HOLDER, 100.0f, true);
            if (blue_aura_holder)
                blue_aura_holder->RemoveAura(VISUAL_BLUE_AURA);

            Unit * target = me->GetVictim();
            if (target)
                me->CastSpell(target, PYROBLAST, false); // First Pyroblast

            Random_Text = rand()%2;
            switch(Random_Text) {
                case 0:
                    me->MonsterYell("You asked for it.", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(25917, true);
                    break;
                case 1:
                    me->MonsterYell("I hate resorting to violence.", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(25918, true);
                    break;
            }
        }

        void KilledUnit(Unit* /*victim*/)
        {
            Random_Text = rand()%3;
            switch(Random_Text) {
                case 0:
                    me->MonsterYell("You forced my hand.", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(25921, true);
                    break;
                case 1:
                    me->MonsterYell("I didn't want to do that.", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(25922, true);
                    break;
                case 2:
                    me->MonsterYell("I wish you'd surrendered.", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(25923, true);
                    break;
            }
        }

        void JustDied(Unit * /*who*/)
        {
            if (instance)
            {
                instance->SetData(TYPE_ECHO_OF_JAINA, DONE);
                instance->SetData(DATA_ECHO_KILLED, 1);
            }

            me->MonsterYell("I understand, now. Farewell, and good luck.", LANG_UNIVERSAL, 0);
            me->SendPlaySound(25919, true);
        }

        void SummonFrostBlades()
        {
            Creature * jaina = me->FindNearestCreature(ECHO_OF_JAINA, 300, true);
            if (jaina) 
            {
                float angle = jaina->GetOrientation();
                double angleAddition = (2*M_PI)/14.4; // 25 Degrees
                angle = angle-angleAddition;
                angle = MapManager::NormalizeOrientation(angle);
                float distance = 1.5f;

                for(uint32 i = 0; i < 3; i++)
                {
                    jaina->SummonCreature(FROST_BLADE,jaina->GetPositionX()+cos(angle)*distance,jaina->GetPositionY()+sin(angle)*distance,jaina->GetPositionZ(),angle,TEMPSUMMON_TIMED_DESPAWN, 8000);
                    angle += angleAddition;
                    angle = MapManager::NormalizeOrientation(angle);
                }
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (Phase == 1) // Pyroblast phase
            {
                if (Pyroblast_Timer <= diff)
                {
                    // Cast 4 Pyroblasts
                    if (Pyroblast_Counter<4)
                    {
                        Unit * target = SelectRandomPlayer(SELECT_TARGET_TOPAGGRO);
                        if (!target)
                            target = me->GetVictim();
                        if (target)
                        {
                            me->CastSpell(target, PYROBLAST, false);
                            Pyroblast_Counter++;
                            Flarecore_Counter++;
                        }
                        // Flarecore trigger
                        if ((Flarecore_Counter-1)%3==0) // After every 3rd pyroblast 
                        { 
                            if (Flarecore_Counter != 1) // Not after 1st Pyroblast
                            {
                                Unit * fltarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true);
                                if (fltarget)
                                    me->CastSpell(fltarget, FLARECORE_FLY, true);
                            }
                        }
                        Pyroblast_Timer = 3200;
                    }
                    // 5 pyroblasts in total from start of the encounter or 4 during the repeating phase 1
                    else if (Pyroblast_Counter==4)
                    {
                        if (!me->HasUnitState(UNIT_STATE_CASTING))
                        {
                            Phase = 2;
                            me->CastSpell(me, BLINK, false); 
                            Blink_Timer = 1000; // Delay 2nd phase, so players can see visual effect of Blink
                        }
                    }
                }
                else Pyroblast_Timer -= diff;
            }

            if (Phase == 2) // Blink phase
            {
                if (Blink_Timer <= diff)
                {
                    // Port spot selection
                    std::vector<int> locations; 
                    for(int i = 0 ; i < 5; i++)
                    {
                        if (i != Last_Port_Spot) // Avoid port on the same spot
                            locations.push_back(i);
                    }
                    Port_Location = locations[urand(0, locations.size() - 1)]; // Select random port point
                    Last_Port_Spot = Port_Location; // Save the last port location

                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);

                    // Teleport
                    me->GetMotionMaster()->MoveJump(TeleportPoint[Port_Location][0], TeleportPoint[Port_Location][1], TeleportPoint[Port_Location][2], 100.0f, 100.0f);
                    me->NearTeleportTo(TeleportPoint[Port_Location][0], TeleportPoint[Port_Location][1], TeleportPoint[Port_Location][2], TeleportPoint[Port_Location][3]);
                    me->SendMovementFlagUpdate();

                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                    Frost_Volley_Counter = 0;
                    Blades_Timer = 1000;
                    Summon_Frost_Blades = false;
                    Phase = 3;

                } else Blink_Timer -= diff;
            }

            if (Phase == 3) // Frost Volley phase
            {
                if (!me->HasUnitState(UNIT_STATE_CASTING))
                {
                    // Cast 3 Frost Volleys
                    if (Frost_Volley_Counter<3)
                    {
                        me->CastSpell(me, FROST_VOLLEY, false);
                        Frost_Volley_Counter++;
                    }
                }
                // 3 Frost Volleys, so it`s time to go back to phase 1
                if (Frost_Volley_Counter == 3) 
                {
                    if (!me->HasUnitState(UNIT_STATE_CASTING))
                    {
                        Phase = 1;
                        Pyroblast_Counter = 0; // Reset counter before phase 1
                    }
                }
                // Release Frost Blades 
                if (Blades_Timer <= diff)
                {
                    if (Summon_Frost_Blades == false)
                    {
                        SummonFrostBlades();
                        Summon_Frost_Blades = true;
                    }
                }
                else Blades_Timer -= diff;

            }
        }

    };
};

// Frost Blades
class frost_blades : public CreatureScript
{
public:
    frost_blades() : CreatureScript("frost_blades") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new frost_bladesAI (pCreature);
    }

    struct frost_bladesAI : public ScriptedAI
    {
        frost_bladesAI(Creature *c) : ScriptedAI(c) {}

        void Reset() 
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, FROST_BLADE_BUFF, false); // Frost Blades visual
        }

        void EnterCombat(Unit * /*who*/) { }

        void UpdateAI(const uint32 diff)
        {
            float angle = me->GetOrientation();
            float distance = 5;
            me->GetMotionMaster()->MovePoint(FROST_BLADE, me->GetPositionX()+cos(angle)*distance,me->GetPositionY()+sin(angle)*distance, me->GetPositionZ());
        }
    };
};

// Flarecore Ember
class flarecore_ember : public CreatureScript
{
public:
    flarecore_ember() : CreatureScript("flarecore_ember") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new flarecore_emberAI (pCreature);
    }

    struct flarecore_emberAI : public ScriptedAI
    {
        flarecore_emberAI(Creature *c) : ScriptedAI(c) {}

        uint32 Detonation_Timer;
        uint32 FlareUp_Timer;
        uint32 Kill_Timer;
        uint32 Check_Timer;
        bool Detonation;
        bool Unstable_Detonation;
        bool Ready_For_Kill;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, FLARE_VISUAL, false); // Flarecore visual

            Detonation_Timer = 10000;
            FlareUp_Timer = 1000;
            Check_Timer = 500;
            Ready_For_Kill = false;
        }

        void EnterCombat(Unit * /*who*/) { }

        void UpdateAI(const uint32 diff)
        {
            // Detonation after 10s
            if (Detonation_Timer <= diff) 
            {
                if (Ready_For_Kill == false)
                {
                    me->CastSpell(me, FLARE, false);
                    Kill_Timer = 500;
                    Ready_For_Kill = true;
                }
            }
            else Detonation_Timer -= diff;

            // Grow Up every second
            if (FlareUp_Timer <= diff) 
            {
                me->CastSpell(me, FLARE_UP, false);
                FlareUp_Timer = 1000;
            }
            else FlareUp_Timer -= diff;

            // Detonation caused by player
            if (Check_Timer <= diff)
            {
                Unit * target = SelectRandomPlayer(2.0f);
                if (target)
                {
                    if (Ready_For_Kill == false)
                    {
                        me->CastSpell(me, UNSTABLE_FLARE, false);
                        Kill_Timer = 500;
                        Ready_For_Kill = true;
                    }
                }
                else Check_Timer = 500;
            } 
            else Check_Timer -= diff;

            // Have to do it this way, otherwise players won`t see visual effect of Flarecore detonation
            if (Kill_Timer <= diff) 
            {
                me->Kill(me, false);
            }
            else Kill_Timer -= diff;
        }
    };
};

// Blue Aura
class blue_aura_holder : public CreatureScript
{
public:
    blue_aura_holder() : CreatureScript("blue_aura_holder") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new blue_aura_holderAI (pCreature);
    }

    struct blue_aura_holderAI : public ScriptedAI
    {
        blue_aura_holderAI(Creature *c) : ScriptedAI(c) {}

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, VISUAL_BLUE_AURA, false);
        }

        void EnterCombat(Unit * /*who*/) { }
        void UpdateAI(const uint32 diff) { }
    };
};

void AddSC_boss_echo_of_jaina()
    {
        new boss_echo_of_jaina();
        new frost_blades();
        new flarecore_ember();
        new blue_aura_holder();
    }

//////////////////////////////////////////////////////////////////////////////////////
///////////////////////////         SQL QUERY          ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
/*
--Echo of Jaina
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('54445','0','0','0','0','0','38802','0','0','0','Echo of Jaina','','','0','87','87','3','16','16','0','1','1','1','3','0','0','0','0','1','2000','2000','2','0','1','0','0','0','0','0','0','0','0','7','72','54445','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','20000','20000','','0','3','55.626','38.5987','1','0','0','0','0','0','0','0','181','1','54445','613367807','1','boss_echo_of_jaina','15595');
 
--Frost Blades
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('54494','0','0','0','0','0','11686','0','0','0','Frost Blade','','','0','85','85','0','16','16','0','1','1.14286','1','3','0','0','0','0','1','0','0','1','0','0','0','0','0','0','0','0','0','0','10','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','0','3','1','1','1','0','0','0','0','0','0','0','0','1','0','0','0','frost_blades','15595');

--Flarecore
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('54446','0','0','0','0','0','11686','0','0','0','Flarecore Ember','','','0','85','85','0','16','16','0','1','1.14286','1','3','0','0','0','0','1','0','0','1','0','0','0','0','0','0','0','0','0','0','10','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','0','3','1','1','1','0','0','0','0','0','0','0','0','1','0','0','0','flarecore_ember','15595');

--Blue Aura Holder
INSERT INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('119501','0','0','0','0','0','11686','0','0','0','Purple Aura Holder','','','0','85','85','3','16','16','0','1','1.14286','1','0','0','0','0','0','1','0','0','1','0','0','0','0','0','0','0','0','0','0','10','1074790400','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','0','3','1','1','1','0','0','0','0','0','0','0','169','1','0','0','0','blue_aura_holder','15595');

--Loot
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54445','72808','60','1','1','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54445','72809','40','1','1','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54445','72805','5','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54445','72801','4','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54445','72802','4','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54445','72804','4','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54445','72799','4','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54445','72803','3','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54445','72800','3','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54445','72807','3','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54445','72798','3','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54445','72806','3','1','2','1','1');
 */
