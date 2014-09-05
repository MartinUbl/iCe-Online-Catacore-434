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
Encounter: Murozond
Dungeon: End Time
Difficulty: Heroic
Mode: 5-man
Autor: Lazik
*/

#include "ScriptPCH.h"
#include "Spell.h"
#include "UnitAI.h"

#define NEVER (0xffffffff) // Used as "delayed" timer

enum NPC
{
    MUROZOND           = 54432,
    NOZDORMU           = 54751,
};

//define boss id`s to her name and her spells
enum Spells 
{
    TAIL_LASH              = 108589, // Dmg Spell
    INFINITE_BREATH        = 102569, // Dmg Spell
    TEMPORAL_BLAST         = 102381, // Dmg Spell + Increase dmg over time
    DISTORTION_BOMB        = 101983, // AoE
    TEMPORAL_SNAPSHOT      = 101592, // Save all players positions
    REWIND_TIME1           = 101590, // Rewind Time buff
    REWIND_TIME            = 101591, // Effect of Rewind Time
    BLESSING               = 102364, // Nozdormu`s blessing for players after using Houglass
    HOURS_COUNTDOWN_VISUAL = 102668, // Players can see the bar of Hourglass
    FADING                 = 107550, // Visual effect after his death
};

struct EnterCombatPosition
{
    uint64 guid;
    float x;
    float y;
    float z;
};
EnterCombatPosition Coordinates[5];

// Murozond
class boss_murozond : public CreatureScript
{
public:
    boss_murozond() : CreatureScript("boss_murozond") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_murozondAI (pCreature);
    }

    struct boss_murozondAI : public ScriptedAI
    {
        boss_murozondAI(Creature *c) : ScriptedAI(c) { }

        uint32 Random_Kill_Text;
        uint32 Tail_Lash_Timer;
        uint32 Temporal_Blast_Timer;
        uint32 Flame_Breath_Timer;
        uint32 Distortion_Bomb_Timer;
        uint32 Kill_Timer;
        uint32 Phase;
        uint32 Check_Timer;
        uint32 Rewind_Time_Check;
        uint32 RemoveDynamicObjects_Timer;
        bool Kill_Say;
        bool Ready_To_Die;

        void Reset() 
        {
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_AGGRESSIVE);
            me->SetFlying(true);
            me->SetVisible(true);

            Tail_Lash_Timer = 3000;
            Temporal_Blast_Timer = 12000;
            Flame_Breath_Timer = 15000;
            Distortion_Bomb_Timer = 5000;
            Check_Timer = 1000;
            Rewind_Time_Check = 500;
            Kill_Timer = NEVER;
            RemoveDynamicObjects_Timer = NEVER;
            Kill_Say = false;
            Ready_To_Die = false;
            Phase = 0;

            // Remove Hourglass bar
            Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                    if (Player* player = i->getSource())
                    {
                        if (player->HasAura(HOURS_COUNTDOWN_VISUAL))
                            player->RemoveAura(HOURS_COUNTDOWN_VISUAL);
                    }

            // Remove all DynamicObjects 
            me->RemoveAllDynObjects();

            // Disable interract with Hourglass 
            if (GameObject* Hourglass = me->FindNearestGameObject(209249, 500.0f))
                Hourglass->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
        }

        void EnterCombat(Unit * /*who*/)
        {
            uint32 Position;
            Position = 0;

            me->MonsterYell("So be it.", LANG_UNIVERSAL, 0);
            me->SendPlaySound(25927, true);
            
            me->CastSpell(me, TEMPORAL_SNAPSHOT, false);
            me->SetFlying(false);

            // Enable interract with Hourglass 
            if (GameObject* Hourglass = me->FindNearestGameObject(209249, 500.0f))
                Hourglass->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);

            // Remove all DynObjects if group wiped
            me->RemoveAllDynObjects();

            // Add Hourglass bar to players and save their position
            Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                    if (Player* player = i->getSource())
                    {
                        player->CastSpell(player, HOURS_COUNTDOWN_VISUAL, true);
                        player->SetMaxPower(POWER_SCRIPTED, 5);

                        Coordinates[Position].guid = player->GetGUID();
                        Coordinates[Position].x = player->GetPositionX();
                        Coordinates[Position].y = player->GetPositionY();
                        Coordinates[Position].z = player->GetPositionZ();
                        Position++; // Save data for next player to next Coordinates[Position]
                    }
        }

        void KilledUnit(Unit* /*victim*/)
        {
            Random_Kill_Text = urand(0,2);
            switch(Random_Kill_Text) {
                case 0:
                    me->MonsterYell("Your time has come.", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(25936, false);
                    break;
                case 1:
                    me->MonsterYell("The sand has run out.", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(25937, false);
                    break;
                case 2:
                    me->MonsterYell("Time ends.", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(25938, false);
                    break;
            }
        }

        void JustDied(Unit * /*who*/)
        {
            // Summon Chest with loot
            me->SummonGameObject(209547, 4189.15f, -447.247f, 121.01f, 2.80922f, 0.0f, 0.0f, 0.986223f, 0.165424f, 86400);

            // Remove Hourglass bar
            Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                    if (Player* player = i->getSource())
                    {
                        if (player->HasAura(HOURS_COUNTDOWN_VISUAL))
                            player->RemoveAura(HOURS_COUNTDOWN_VISUAL);
                    }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            // Tail Lash
            if (Tail_Lash_Timer <= diff)
            {
                me->CastSpell(me, TAIL_LASH, false);
                Tail_Lash_Timer = 15000;
            }
            else Tail_Lash_Timer -= diff;

            //Temporal Blast
            if (Temporal_Blast_Timer <= diff)
            {
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                me->CastSpell(me, TEMPORAL_BLAST, false);
                Temporal_Blast_Timer = 12000;
            }
            else Temporal_Blast_Timer -= diff;

            //Flame Breath
            if (Flame_Breath_Timer <= diff)
            {
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                me->CastSpell(me, INFINITE_BREATH, false);
                Flame_Breath_Timer = 20000+urand(0,5000);
            }
            else Flame_Breath_Timer -= diff;

            //Rewind Time
            if (Rewind_Time_Check <= diff)
            {
                Rewind_Time_Check = 500;

                Unit * target = me->GetVictim();
                if (target && target->HasAura(REWIND_TIME))
                {
                    Rewind_Time_Check = 5000;
                    Phase++;

                    switch(Phase)
                    {
                        case 1:
                            me->MonsterYell("The powers of the Hourglass do nothing to me!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25929, false);
                            break;
                        case 2:
                            me->MonsterYell("To repeat the same action and expect different results is madness.", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25930, false);
                            break;
                        case 3:
                            me->MonsterYell("Another chance will make no difference. You will fail.", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25931, false);
                            break;
                        case 4:
                            me->MonsterYell("Again...? Is this your plot, your scheme?", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25932, false);
                            break;
                        case 5:
                            me->MonsterYell("The Hourglass' power is exhausted. No more games, mortals. Relent, or perish.", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25933, false);

                            // Disable interract with Hourglass after 5th use
                            if (GameObject* Hourglass = me->FindNearestGameObject(209249, 500.0f))
                                Hourglass->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                            break;
                    }

                    // Remove all DynObjects at the end of Rewind Time spell
                    RemoveDynamicObjects_Timer = 500;

                    Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                    for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                        if (Player* player = i->getSource())
                            if (player->IsInWorld() && player->IsAlive() && !player->IsGameMaster())
                            {
                                // Resurrect dead players
                                if (!player->IsAlive())
                                    player->ResurrectPlayer(100.0f);

                                // Find correct player`s data
                                uint32 Position;
                                Position = 0;
                                for (Position; Position<5; Position++)
                                {
                                    if (player->GetGUID() == Coordinates[Position].guid)
                                        // Move player to "start encounter" position
                                        player->GetMotionMaster()->MovePoint(Coordinates[Position].guid, Coordinates[Position].x, Coordinates[Position].y, Coordinates[Position].z);
                                }

                                // Remove all debuffs
                                player->RemoveAllNegativeAuras();

                                // Immediately finishes all spell cooldowns
                                const SpellCooldowns& cm = player->ToPlayer()->GetSpellCooldownMap();
                                for (SpellCooldowns::const_iterator itr = cm.begin(); itr != cm.end();)
                                {
                                    SpellEntry const *spellInfo = sSpellStore.LookupEntry(itr->first);

                                    if (spellInfo && (GetSpellRecoveryTime(spellInfo) > 0))
                                    {
                                        player->ToPlayer()->RemoveSpellCooldown((itr++)->first, true);
                                    }
                                    else
                                        ++itr;
                                }

                                // Set correct charges in power bar 
                                if (player->HasAura(HOURS_COUNTDOWN_VISUAL))
                                    player->EnergizeBySpell(player, HOURS_COUNTDOWN_VISUAL, -1, POWER_SCRIPTED);
                            }
                }
            }
            else Rewind_Time_Check -= diff;

            // Distortion Bomb
            if (Distortion_Bomb_Timer <= diff) 
            {
                // Disable cast Distortion bombs during Rewind Time
                Unit * tank = me->GetVictim();
                if (tank)
                    if (tank && tank->HasAura(REWIND_TIME))
                        return;

                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 1, 200.0f, true);
                if (!target)
                    target = me->GetVictim();
                if (target) 
                {
                    me->CastSpell(target, DISTORTION_BOMB, true);
                    switch(Phase) 
                    {
                        case 0:
                            Distortion_Bomb_Timer = 5000;
                            break;
                        case 1:
                            Distortion_Bomb_Timer = 4000;
                            break;
                        case 2:
                            Distortion_Bomb_Timer = 3000;
                            break;
                        case 3:
                            Distortion_Bomb_Timer = 2000;
                            break;
                        case 4:
                            Distortion_Bomb_Timer = 1000;
                            break;
                        case 5:
                            Distortion_Bomb_Timer = 500;
                            break;
                    }
                }
            } else Distortion_Bomb_Timer -= diff;

            // Prepare for death, Say, Lie down and Fade
            if (Check_Timer <= diff)
            {
                if (me->GetHealth()<=100000 && Kill_Say == false)
                {
                    Phase = 6;
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                    me->SetReactState(REACT_PASSIVE);
                    me->SetStandState(UNIT_STAND_STATE_SLEEP);
                    me->AttackStop();
                    me->GetMotionMaster()->MovementExpired();
                    me->GetMotionMaster()->Clear();
                    me->GetMotionMaster()->MoveIdle();
                    me->SendMovementFlagUpdate();
                    me->RemoveAllAuras();
                    Kill_Timer = 5000;
                    Kill_Say = true;
                    Ready_To_Die = true;

                    me->CastSpell(me, FADING, true);

                    me->MonsterYell("You know not what you have done. Aman'Thul... What I... have... seen...", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(25928, false);
                }
                Check_Timer = 1000;
            } else Check_Timer -= diff;

            // Remove all DynObjects because of Rewind Time spell
            if (RemoveDynamicObjects_Timer <= diff)
            {
                me->RemoveAllDynObjects();
                RemoveDynamicObjects_Timer = NEVER;
            }
            else RemoveDynamicObjects_Timer -= diff;

            // Death
            if (Kill_Timer <= diff && Ready_To_Die == true)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->RemoveAllDynObjects();
                me->SetVisible(false);
                me->Kill(me, false);
                Ready_To_Die = false;
            }
            else Kill_Timer -= diff;

            DoMeleeAttackIfReady();
        } 

    };
};

void AddSC_boss_murozond()
    {
        new boss_murozond();
    }

//////////////////////////////////////////////////////////////////////////////////////
///////////////////////////         SQL QUERY          ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
/*
-- Murozond
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('54432','0','0','0','0','0','38931','0','0','0','Murozond','The Lord of the Infinite','','0','87','87','3','16','16','0','1.3','1.3','1','3','65000','95000','0','0','1','2000','2000','2','0','1','0','0','0','0','0','0','0','0','2','72','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','20000','20000','','0','3','163.168','192.993','1','0','0','0','0','0','0','0','144','1','0','646922239','0','boss_murozond','15595');

-- Nozdormu`s Blessing when Rewind Time
INSERT INTO `spell_linked_spell` (`spell_trigger`, `spell_effect`, `type`, `comment`) values('101591','102364','1','End Time - Murozond - Nozdormu`s Blessing when Rewind Time');

-- Murozond`s Temporal Cache
INSERT INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `faction`, `flags`, `size`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `data0`, `data1`, `data2`, `data3`, `data4`, `data5`, `data6`, `data7`, `data8`, `data9`, `data10`, `data11`, `data12`, `data13`, `data14`, `data15`, `data16`, `data17`, `data18`, `data19`, `data20`, `data21`, `data22`, `data23`, `data24`, `data25`, `data26`, `data27`, `data28`, `data29`, `data30`, `data31`, `AIName`, `ScriptName`, `WDBVerified`) values('209547','3','11020','Murozond\'s Temporal Cache','','','','0','0','2','0','0','0','0','0','0','93','209547','0','1','0','0','0','0','0','0','0','0','1','0','0','1','0','0','0','0','85','0','0','0','0','0','0','0','0','0','0','0','','','1');

-- Murozond`s Tempral Cache Loot
INSERT INTO `gameobject_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('209547','72897','15','1','1','1','1');
INSERT INTO `gameobject_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('209547','72820','14','1','1','1','1');
INSERT INTO `gameobject_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('209547','72821','14','1','1','1','1');
INSERT INTO `gameobject_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('209547','72826','12','1','1','1','1');
INSERT INTO `gameobject_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('209547','72824','12','1','1','1','1');
INSERT INTO `gameobject_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('209547','72822','10','1','1','1','1');
INSERT INTO `gameobject_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('209547','72818','9','1','1','1','1');
INSERT INTO `gameobject_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('209547','72816','9','1','1','1','1');
INSERT INTO `gameobject_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('209547','72819','9','1','1','1','1');
INSERT INTO `gameobject_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('209547','72817','9','1','1','1','1');
INSERT INTO `gameobject_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('209547','72825','8','1','1','1','1');
INSERT INTO `gameobject_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('209547','72823','8','1','1','1','1');
INSERT INTO `gameobject_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('209547','52078','64','1','0','1','1');
*/