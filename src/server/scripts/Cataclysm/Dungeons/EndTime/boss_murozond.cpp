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
#include "endtime.h"

enum NPC
{
    MUROZOND           = 54432,
    NOZDORMU_INTRO     = 54476,
    NOZDORMU_OUTRO     = 54751,
};

enum ScriptTexts
{
    // Murozond encounter
    SAY_NOZDORMU_START   = -1999931, // 25943 - Mortals! I cannot follow you any further - accept my blessing and use the Hourglass of Time to defeat Murozond!
    SAY_NOZDORMU_END_1   = -1999932, // 25944 - At last it has come to pass. The moment of my demise. The loop is closed. My future self will cause no more harm.
    SAY_NOZDORMU_END_2   = -1999933, // 25945 - Still, in time, I will... fall to madness. And you, heroes... will vanquish me. The cycle will repeat. So it goes.
    SAY_NOZDORMU_END_3   = -1999934, // 25946 - What matters is that Azeroth did not fall; that we survived to fight another day.
    SAY_NOZDORMU_END_4   = -1999935, // 25947 - All that matters... is this moment.
};

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
        boss_murozondAI(Creature *creature) : ScriptedAI(creature) 
        {
            instance = creature->GetInstanceScript();
            me->GetMotionMaster()->MoveJump(4215.0f, -428.6582f, 150.0f, 100.0f, 100.0f);

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

            me->SetFlying(true);
            me->SetVisible(false);
        }

        InstanceScript* instance;
        uint32 Random_Kill_Text;
        uint32 Tail_Lash_Timer;
        uint32 Temporal_Blast_Timer;
        uint32 Flame_Breath_Timer;
        uint32 Distortion_Bomb_Timer;
        uint32 Kill_Timer;
        uint32 Phase;
        uint32 Rewind_Time_Check;
        uint32 RemoveDynamicObjects_Timer;
        uint32 Nozdormu_Say_Timer;
        bool Kill_Say;
        bool Ready_To_Die;
        bool Nozdormu_Say;

        void Reset() 
        {
            if (instance)
            {
                if(instance->GetData(TYPE_MUROZOND)!=DONE)
                    instance->SetData(TYPE_MUROZOND, NOT_STARTED);
            }

            me->SetStandState(UNIT_STAND_STATE_STAND);
            me->SetReactState(REACT_AGGRESSIVE);

            Tail_Lash_Timer = 3000;
            Temporal_Blast_Timer = 12000;
            Flame_Breath_Timer = 15000;
            Distortion_Bomb_Timer = 5000;
            Rewind_Time_Check = 500;
            Nozdormu_Say_Timer = 3000;
            Kill_Timer = MAX_TIMER;
            RemoveDynamicObjects_Timer = MAX_TIMER;
            Kill_Say = false;
            Ready_To_Die = false;
            Nozdormu_Say = false;
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

        void JustReachedHome()
        {
            me->SetVisible(true);
            me->SetFlying(false);
        }

        void EnterCombat(Unit * /*who*/)
        {
            if (instance)
            {
                instance->SetData(TYPE_MUROZOND, IN_PROGRESS);
            }

            uint32 Position;
            Position = 0;

            me->MonsterYell("So be it.", LANG_UNIVERSAL, 0);
            me->SendPlaySound(25927, true);

            me->CastSpell(me, TEMPORAL_SNAPSHOT, false);
            me->SetFlying(false);
            me->GetMotionMaster()->MoveJump(4170.6196f, -428.5562f, 119.296f, 10.0f, 10.0f);

            // Enable interract with Hourglass 
            if (GameObject* Hourglass = me->FindNearestGameObject(209249, 500.0f))
                Hourglass->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);

            // Remove all DynObjects if group wiped
            me->RemoveAllDynObjects();

            // Add Hourglass bar to players and save their position
            Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                    if (Player* player = i->getSource())
                        if (!player->IsGameMaster())
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
            if (instance)
            {
                instance->SetData(TYPE_MUROZOND, DONE);
            }

            // Remove Hourglass bar
            Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                    if (Player* player = i->getSource())
                    {
                        if (player->HasAura(HOURS_COUNTDOWN_VISUAL))
                            player->RemoveAura(HOURS_COUNTDOWN_VISUAL);
                        player->CombatStop();
                        player->GetAchievementMgr().CompletedAchievement(sAchievementStore.LookupEntry(6117), true); // Heroic: End Time
                    }

            // Summon Chest with loot
            Map::PlayerList const &playerList = me->GetMap()->GetPlayers();
                if (!playerList.isEmpty())
                    for (Map::PlayerList::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
                        if (Player* pPlayer = i->getSource())
                        {
                            pPlayer->SummonGameObject(209547, 4189.15f, -447.247f, 121.01f, 2.80922f, 0.0f, 0.0f, 0.986223f, 0.165424f, 86400);
                            return;
                        }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            // Nozdormu Say Intro
            if (Nozdormu_Say == false)
            {
                if (Nozdormu_Say_Timer <= diff)
                {
                    Creature * nozdormu_intro = me->FindNearestCreature(NOZDORMU_INTRO, 200.0, true);
                    if (nozdormu_intro)
                    {
                        nozdormu_intro->MonsterSay("Mortals! I cannot follow you any further - accept my blessing and use the Hourglass of Time to defeat Murozond!", LANG_UNIVERSAL, nozdormu_intro->GetGUID(), 150.0f);
                        me->SendPlaySound(25943, true);
                    }
                    Nozdormu_Say = true;
                } 
                else Nozdormu_Say_Timer -= diff;
            }

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
                            if (player->IsInWorld() && player && !player->IsGameMaster())
                            {
                                // Resurrect dead players
                                player->ResurrectPlayer(100.0f);

                                // Find correct player`s data
                                uint32 position = 0;
                                for (; position < 5; position++)
                                {
                                    if (player->GetGUID() == Coordinates[position].guid)
                                        // Move player to "start encounter" position
                                        player->GetMotionMaster()->MovePoint(Coordinates[position].guid, Coordinates[position].x, Coordinates[position].y, Coordinates[position].z);
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
                            Distortion_Bomb_Timer = 9000;
                            break;
                        case 1:
                            Distortion_Bomb_Timer = 7500;
                            break;
                        case 2:
                            Distortion_Bomb_Timer = 6000;
                            break;
                        case 3:
                            Distortion_Bomb_Timer = 4500;
                            break;
                        case 4:
                            Distortion_Bomb_Timer = 3000;
                            break;
                        case 5:
                            Distortion_Bomb_Timer = 1500;
                            break;
                    }
                }
            } else Distortion_Bomb_Timer -= diff;

            // Prepare for death, Say, Lie down and Fade
            if (me->GetHealth()<=250000)
            {
                if (Kill_Say == false)
                {
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

                    if (InstanceScript *pInstance = me->GetInstanceScript())
                        pInstance->SetData(DATA_TRASH_MUROZOND, 2);
                }
            }

            // Remove all DynObjects because of Rewind Time spell
            if (RemoveDynamicObjects_Timer <= diff)
            {
                me->RemoveAllDynObjects();
                RemoveDynamicObjects_Timer = MAX_TIMER;
            }
            else RemoveDynamicObjects_Timer -= diff;

            // Death
            if (Kill_Timer <= diff)
            {
                if (Ready_To_Die == true)
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    me->RemoveAllDynObjects();

                    Unit * player = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true);
                    if (player)
                        player->Kill(me, false);

                    me->SetVisible(false);
                    Ready_To_Die = false;

                    // Hide Nozdormu Intro
                    Creature * nozdormu_intro = me->FindNearestCreature(NOZDORMU_INTRO, 250.0, true);
                    if (nozdormu_intro)
                        nozdormu_intro->SetVisible(false);

                    // Spawn Nozdormu Outro
                    me->SummonCreature(NOZDORMU_OUTRO, 4133.0f, -423.77f, 122.75, 0.00f);
                }
            }
            else Kill_Timer -= diff;

            DoMeleeAttackIfReady();
        } 

    };
};

class npc_nozdormu_outro : public CreatureScript
{
public:
    npc_nozdormu_outro() : CreatureScript("npc_nozdormu_outro") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_nozdormu_outroAI (pCreature);
    }

    struct npc_nozdormu_outroAI : public ScriptedAI
    {
        npc_nozdormu_outroAI(Creature *c) : ScriptedAI(c) {}

        uint32 Say_Timer;
        int Say;
        bool Say_Last;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->CastSpell(me, 102602, true); // Visual aura
            Say = 0;
            Say_Timer = 8000;
            Say_Last = false;
        }

        void EnterCombat(Unit * /*who*/) { }
        void UpdateAI(const uint32 diff) 
        {
            if (Say_Last == false)
            {
                if (Say_Timer <= diff)
                {
                    switch (Say)
                    {
                        case 0:
                            {
                                me->MonsterSay("At last it has come to pass. The moment of my demise. The loop is closed. My future self will cause no more harm.", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
                                me->SendPlaySound(25944, true);
                                Say += 1;
                                Say_Timer = 16000;
                                break;
                            }
                        case 1:
                            {
                                me->MonsterSay("Still, in time, I will... fall to madness. And you, heroes... will vanquish me. The cycle will repeat. So it goes.", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
                                me->SendPlaySound(25945, true);
                                Say += 1;
                                Say_Timer = 18000;
                                break;
                            }
                        case 2:
                            {
                                me->MonsterSay("What matters is that Azeroth did not fall; that we survived to fight another day.", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
                                me->SendPlaySound(25946, true);
                                Say += 1;
                                Say_Timer = 12000;
                                break;
                            }
                        case 3:
                            {
                                me->MonsterSay("All that matters... is this moment.", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
                                me->SendPlaySound(25947, true);
                                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                Say = 0;
                                Say_Last = true;
                                break;
                            }
                    }
                }
                else Say_Timer -= diff;
            }
        }

    };
};

void AddSC_boss_murozond()
    {
        new boss_murozond();
        new npc_nozdormu_outro();
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