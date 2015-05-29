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
Encounter: Echo of Baine
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
    ECHO_OF_BAINE         = 54431,
    TOTEM                 = 54434,
};

enum Objects
{
    BAINE_PLATFORM_1            = 209693, // Platform where Baine stays
    BAINE_PLATFORM_2            = 209695, // On right side from Baine`s POV
    BAINE_PLATFORM_3            = 209670, // On left side from Baines POV
    BAINE_PLATFORM_4            = 209694, // Farest platform from Baine
};

// Spells
enum Spells 
{
        TOTEMS_AURA_BAINE        = 101624, // Two totems on his back
        VISUAL_MODEL_TOTEM       = 101594, // Visual effect of totem

        TOTEM_AIM                = 101601, // Aiming with totem + cast 101603 on selected place
        TOTEM2                   = 101602, // 5% HP + Stun
        TOTEM3                   = 101603, // Thorws totem back + triggers 101602
        TOTEM_BUFF               = 107837, // Triggers 101601, spell for players from totem - DB side

        THROW_TOTEM_MARK_TARGET  = 101613, // Who knows
        EFFECTS_OF_BAINES_TOTEM  = 101614, // Effects of Baine`s totem... knockback, spawn atc...
        THROW_TOTEM              = 101615, // Baine`s cast => Throw it

        PULVERIZE                = 101625, // Dummy aura target all enemy in area
        PULVERIZE_JUMP           = 101626, // Jump
        PULVERIZE_DMG            = 101627, // DMG + Trigger destroy game object
        PULVERIZE_DESTRUCTION    = 101815, // Destroy near game object 

        MOLTEN_AXE_BUFF          = 101834, // Buff on Baine
        MOLTEN_AXE               = 101836, // Via this applying debufs on target and checking when switch weapons
        //MOLTEN_MACE_BUFF         = 101865, Redesigned after Patch 4.3.0
        //MOLTEN_MACE              = 101867, Redesigned after Patch 4.3.0
        MOLTEN_FISTS             = 101866, // Buff for players gained from lava - DB

        MOLTENBLAST              = 101840, // Debuff on players
};

enum Weapons
{
        AXE_NORMAL               = 72814,
        AXE_FLAME_EFFECT         = 92169,
};

// Echo of Baine
class boss_echo_of_baine : public CreatureScript
{
public:
    boss_echo_of_baine() : CreatureScript("boss_echo_of_baine") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_echo_of_baineAI (pCreature);
    }

    struct boss_echo_of_baineAI : public ScriptedAI
    {
        boss_echo_of_baineAI(Creature *creature) : ScriptedAI(creature) 
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Totem_Timer;
        uint32 Pulverize_Timer;
        uint32 Pulverize_Destroy_Timer;
        uint32 Molten_Axe_Check;
        int Random_Kill_Text;

        void Reset() 
        {
            if (instance)
            {
                if(instance->GetData(TYPE_ECHO_OF_BAINE)!=DONE)
                    instance->SetData(TYPE_ECHO_OF_BAINE, NOT_STARTED);
            }

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);

            Totem_Timer             = 10000;
            Pulverize_Timer         = 30000;
            Pulverize_Destroy_Timer = 40000;
            Molten_Axe_Check        = 1000;

            me->CastSpell(me, MOLTEN_AXE_BUFF, false);   // Visual buff on Baine
            me->CastSpell(me, TOTEMS_AURA_BAINE, false); // Totems on Baine`s back

            SetEquipmentSlots(true); // Apply his default weapon

            // Reset all platforms
            if (GameObject* Platform1 = me->FindNearestGameObject(209693, 500.0f))
                    Platform1->SetDestructibleState(GO_DESTRUCTIBLE_REBUILDING, NULL, false);
            if (GameObject* Platfrom2 = me->FindNearestGameObject(209694, 500.0f))
                    Platfrom2->SetDestructibleState(GO_DESTRUCTIBLE_REBUILDING, NULL, false);
            if (GameObject* Platform3 = me->FindNearestGameObject(209695, 500.0f))
                    Platform3->SetDestructibleState(GO_DESTRUCTIBLE_REBUILDING, NULL, false);
            if (GameObject* Platform4 = me->FindNearestGameObject(209670, 500.0f))
                    Platform4->SetDestructibleState(GO_DESTRUCTIBLE_REBUILDING, NULL, false);

            // Despawn firewall barrier
            if (GameObject* Firewall = me->FindNearestGameObject(250301, 500.0f))
                    Firewall->Delete();
            if (GameObject* Firewall = me->FindNearestGameObject(250301, 500.0f))
                    Firewall->Delete();
        }

        void InEvadeMode() { }

        void EnterCombat(Unit * /*who*/)
        {
            if (instance)
            {
                instance->SetData(TYPE_ECHO_OF_BAINE, IN_PROGRESS);
            }

            me->MonsterYell("What dark horrors have you wrought in this place? By my ancestors' honor, I shall take you to task!", LANG_UNIVERSAL, 0);
            me->SendPlaySound(25909, true);

            // Summon firewall so players can`t take Baine anywhere else
            me->SummonGameObject(250301, 4374.13f, 1383.59f, 132.155f, 0.945964f, 0.0f, 0.0f, 0.455543f, 0.890214f, 300);
            me->SummonGameObject(250301, 4410.90f, 1393.75f, 130.615f, 1.656770f, 0.0f, 0.0f, 0.736840f, 0.676067f, 300);
        }

        void KilledUnit(Unit* /*victim*/)
        {
            Random_Kill_Text = urand(0,2);
            switch(Random_Kill_Text) {
            case 0:
                me->MonsterYell("This is the price you pay!", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25912, true); 
                    break;
            case 1:
                me->MonsterYell("A just punishment.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25913, true);
                    break;
            case 2:
                me->MonsterYell("Suffer for your arrogance!", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25914, true);
                    break;
            }
        }

        void JustDied(Unit* /*who*/)
        {
            if (instance)
            {
                instance->SetData(TYPE_ECHO_OF_BAINE, DONE);
                instance->SetData(DATA_ECHO_KILLED, 1);
            }

            me->MonsterYell("Where... is this place? What... have I done? Forgive me, my father...", LANG_UNIVERSAL, 0);
            me->SendPlaySound(25910, true);

            // Despawn firewall barrier
            if (GameObject* Firewall = me->FindNearestGameObject(250301, 500.0f))
                    Firewall->Delete();
            if (GameObject* Firewall = me->FindNearestGameObject(250301, 500.0f))
                    Firewall->Delete();
        }

        void SpellHitTarget(Unit* target, const SpellEntry* spell)
        {
            if (spell->Id == THROW_TOTEM)
            {
                if (!target) target = me->GetVictim();
                if (target)
                {
                    // Targeted player has to summon totem or it`s not clickable
                    target->SummonCreature(TOTEM, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(),0.0f,TEMPSUMMON_TIMED_DESPAWN, 10000);
                    me->CastSpell(target, EFFECTS_OF_BAINES_TOTEM, false); // Knockback and dmg 
                }
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            // Pulverize 
            if (Pulverize_Timer <= diff && !me->HasAura(TOTEM2))
            {
                me->MonsterYell("My wrath knows no bounds!", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25916, true);

                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true);
                if (target)
                {
                    me->CastSpell(target, PULVERIZE, false); // Dunno if needed but just for sure
                    me->CastSpell(target, PULVERIZE_JUMP, false); // Jump on random target
                }
                Pulverize_Destroy_Timer = 2000;
                Pulverize_Timer = 40000;
            }
            else Pulverize_Timer -= diff;

            // Destroy  platform after Pulverize
            if (Pulverize_Destroy_Timer <= diff)
            {
                me->CastSpell(me, PULVERIZE_DMG, false); // Destroy platform
                Pulverize_Destroy_Timer = 50000;
            }
            else Pulverize_Destroy_Timer -= diff;

            // Throw Totem
            if (Totem_Timer <= diff)
            {
                me->MonsterYell("There will be no escape!", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25915, true);

                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true);
                if (target)
                    me->CastSpell(target, THROW_TOTEM, false);

                Totem_Timer = 25000;
            }
            else Totem_Timer -= diff;

            // If in lava or Has Molten Axe buff
            if (Molten_Axe_Check <= diff)
            {
                Map * map = me->GetMap();
                if (map) 
                {
                    ZLiquidStatus res = me->GetMap()->getLiquidStatus(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), MAP_ALL_LIQUIDS);
                    if (res != 0 && !me->HasAura(MOLTEN_AXE))
                    {
                        me->CastSpell(me, MOLTEN_AXE, false);
                        Molten_Axe_Check = 1000;
                    } 
                    else Molten_Axe_Check = 1000;
                }
                else Molten_Axe_Check = 1000;

                // Switch weapons when he has a Molten Axe buff
                if (me->HasAura(MOLTEN_AXE))
                {
                    SetEquipmentSlots(false, AXE_FLAME_EFFECT, EQUIP_NO_CHANGE, EQUIP_NO_CHANGE); // Switch weapon to flame one
                    me->SetSheath(SHEATH_STATE_MELEE);
                }
                else SetEquipmentSlots(true);

                // Evade when too far away from spawn position so players can`t go with him on "beach"
                if (me->GetDistance(me->GetHomePosition()) >= 90)
                {
                    me->AI()->EnterEvadeMode();
                    return;
                }

            } else Molten_Axe_Check -= diff;

            DoMeleeAttackIfReady();
        }

        void MoveInLineOfSight(Unit* pWho)
        {
            // Aggro only when some player reaches his platform
            if(me->GetDistance2d(pWho) > 7.0f)
                return;

            ScriptedAI::MoveInLineOfSight(pWho);
        }
    };
};

// Baine`s Totem
class baines_totem : public CreatureScript
{
public:
    baines_totem() : CreatureScript("baines_totem") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new baines_totemAI (pCreature);
    }

    struct baines_totemAI : public ScriptedAI
    {
        baines_totemAI(Creature *c) : ScriptedAI(c) {}

        uint32 Check_Timer;

        void Reset() 
        {
            me->CastSpell(me, VISUAL_MODEL_TOTEM, false); // It has an invisible model so now players can see it
            Check_Timer = 1000;
        }

        void EnterCombat(Unit * /*who*/) { }
        void UpdateAI(const uint32 diff) 
        {
            // Check if some player clicks on totem
            if (Check_Timer <= diff)
            {
                Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                    if (Player* player = i->getSource())
                    {
                        if (player->HasAura(TOTEM_BUFF))
                            me->Kill(me, false);
                    }
                Check_Timer = 500;
            } else Check_Timer -= diff;
                
        }
    };
};

void AddSC_boss_echo_of_baine()
    {
        new boss_echo_of_baine();
        new baines_totem();
    }

//////////////////////////////////////////////////////////////////////////////////////
///////////////////////////         SQL QUERY          ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
/*
-- Echo of Baine
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('54431','0','0','0','0','0','38791','0','0','0','Echo of Baine','','','0','87','87','3','16','16','0','1','1.3','1','3','45000','55000','0','0','1','1500','2000','1','0','1','0','0','0','0','0','0','0','0','7','72','54431','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','20000','20000','','0','3','63.784487','1','1','0','0','0','0','0','0','0','152','1','54431','646922239','1','boss_echo_of_baine','15595');

-- Baine`s Totem
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES('54434','0','0','0','0','0','11686','0','0','0','Baine`s Totem',NULL,'Interact','0','87','87','3','35','35','16777216','1','1.14286','1','1','0','0','0','0','1','0','0','1','6','0','0','0','0','0','0','0','0','0','11','1048648','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','0','3','1','1','1','0','0','0','0','0','0','0','0','1','0','0','0','baines_totem','1');

-- Totem`s Spell on click
INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `quest_start`, `quest_start_active`, `quest_end`, `cast_flags`, `aura_required`, `aura_forbidden`, `user_type`) values('54434','107837','0','0','0','3','0','0','1');

-- Baine`s Equip
INSERT INTO `creature_equip_template` (`entry`, `equipentry1`, `equipentry2`, `equipentry3`) values('54431','72814','0','0');

-- Molten Fists buff
INSERT INTO `spell_linked_spell` (`spell_trigger`, `spell_effect`, `type`, `comment`) values('101619','101866','1','End Time - Molten Fists from Lava');

-- Loot
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54431','72814','67','1','1','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54431','72815','33','1','1','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54431','72801','4','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54431','72799','4','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54431','72804','4','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54431','72805','4','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54431','72802','3','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54431','72798','3','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54431','72806','3','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54431','72800','3','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54431','72803','3','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54431','72807','3','1','2','1','1');
*/
