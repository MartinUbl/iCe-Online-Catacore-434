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
Encounter: Arcurion
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
    BOSS_ARCURION              =  54590,
    ARCURION_FROZEN_SERVITOR   = 119509,
    ICEWALL_DUMMY              = 119508,
    THRALL                     =  54548,
    THRALL_WOLF                =  55779,
    NPC_ICY_TOMB               =  54995,
};

enum Objects
{
    ICE_WALL              = 210048, // Entrance - summoned by Arcurion
    ICE_WALL2             = 210049, // Exit - destroyed by Thrall
};


const float SpawnPosition[14][4] =
{
    {4739.72f, 84.1997f, 107.230f, 5.30915f},
    {4756.07f, 103.248f, 114.950f, 5.74322f},
    {4750.13f, 97.3125f, 112.217f, 5.84370f},
    {4818.98f, 44.7500f, 106.324f, 3.22131f},
    {4771.36f, 110.743f, 121.498f, 5.00128f},
    {4838.94f, 90.1892f, 108.409f, 3.56877f},
    {4737.55f, 75.5538f, 105.757f, 5.68455f},
    {4842.17f, 110.130f, 107.272f, 4.03014f},
    {4777.38f, 30.8090f, 92.5167f, 1.58226f},
    {4788.47f, 125.670f, 129.112f, 4.73712f},
    {4796.06f, 131.432f, 132.468f, 4.60976f},
    {4827.03f, 50.5660f, 108.630f, 3.05247f},
    {4831.26f, 64.6198f, 108.553f, 3.28342f},
    {4810.14f, 31.5191f, 104.593f, 2.36920f},
};

// Spells
enum Spells 
{
    // Arcurion
    CHAINS_OF_FROST         = 102582, // Chains of Frost
    HAND_OF_FROST           = 102593, // Hand of Frost

    ICY_TOMB                = 103252, // Icy Tomb   Summons: Icy Tomb
    ICY_TOMB_1              = 103251, // Icy Tomb

    TORMENT_OF_FROST        = 104050, // Torrent of Frost
    TORMENT_OF_FROST_1      = 103904, // Torrent of Frost
    TORMENT_OF_FROST_2      = 103962, // Torrent of Frost

    //Frozen Servitor
    ICY_BOULDER             = 102198, // Icy Boulder
    ICY_BOULDER_1           = 102199, // Icy Boulder
    ICY_BOULDER_2           = 102480, // Icy Boulder
    FROZEN_SERVITOR_VISUAL  = 103595, // Visual spawn animation

    // Thrall
    BLOODLUST              = 103834, //Bloodlust
    MOLTEN_FURY            = 103905, // Molten Fury
    LAVA_BURST             = 103923, // Lava Burst
    LAVA_BURST_1           = 102475, // Lava Burst
    LAVA_BARRAGE           = 104540, // Lava Barrage
    GHOST_WOLF             = 2645,   // Ghost Wolf
};

enum Frozen_Doors
{
    FROZEN_DOORS           = 210048,
};

// Arcurion
class boss_arcurion : public CreatureScript
{
public:
    boss_arcurion() : CreatureScript("boss_arcurion") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_arcurionAI (pCreature);
    }

    struct boss_arcurionAI : public ScriptedAI
    {
        boss_arcurionAI(Creature *creature) : ScriptedAI(creature) 
        {
            instance = creature->GetInstanceScript();

            me->SetVisible(false);
        }

        InstanceScript* instance;
        uint32 Intro_Timer;
        uint32 Icy_Tomb_Timer;
        uint32 Hand_Of_Frost_Timer;
        uint32 Chain_Of_Frost_Timer;
        uint32 Pct;
        uint32 Check_Hp_Timer;
        int Random_Text;
        int Intro_Dialogue;
        int Phase;
        int Bloodlust;
        bool Intro;
        bool Torment_Of_Frost;

        void Reset() 
        {
            if (instance)
            {
                if(instance->GetData(TYPE_BOSS_ARCURION)!=DONE)
                    instance->SetData(TYPE_BOSS_ARCURION, NOT_STARTED);
            }

            if (GameObject* Icewall = me->FindNearestGameObject(ICE_WALL, 500.0f))
                Icewall->Delete();

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            Phase = 0;
            Icy_Tomb_Timer = 30000;
            Hand_Of_Frost_Timer = 0;
            Chain_Of_Frost_Timer = 17000;
            Check_Hp_Timer = 5000;
            Pct = 90;
            Intro_Timer = 0;
            Intro_Dialogue = 0;
            Intro = false;
            Torment_Of_Frost = false;
            Bloodlust = false;

            // Despawn all arcurion adds
            std::list<Creature*> arcurion_servitors;
            GetCreatureListWithEntryInGrid(arcurion_servitors, me, ARCURION_FROZEN_SERVITOR, 200.0f);
            for (std::list<Creature*>::const_iterator itr = arcurion_servitors.begin(); itr != arcurion_servitors.end(); ++itr)
                if (*itr)
                {
                    (*itr)->DespawnOrUnsummon(0);
                }

            Creature * icy_tomb = me->FindNearestCreature(NPC_ICY_TOMB, 200.0, true);
            if (icy_tomb)
                icy_tomb->Kill(icy_tomb);
        }

        void EnterCombat(Unit * /*who*/)
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_ARCURION, IN_PROGRESS);
            }

            // Summon Ice wall so players can`t take Arcurion anywhere else
            me->SummonGameObject(ICE_WALL, 4860.8f, 146.603f, 95.5939f, 3.60161f, 0.0f, 0.0f, 0.973664f, -0.227988f, 0);

            // Summon All Adds
            for (uint32 i = 0; i < 14;)
            {
                me->SummonCreature(ARCURION_FROZEN_SERVITOR, SpawnPosition[i][0], SpawnPosition[i][1], SpawnPosition[i][2], SpawnPosition[i][3], TEMPSUMMON_MANUAL_DESPAWN);
                i = i + 1;
            }

            // Set combat for adds
            std::list<Creature*> arcurion_servitors;
            GetCreatureListWithEntryInGrid(arcurion_servitors, me, ARCURION_FROZEN_SERVITOR, 200.0f);
            for (std::list<Creature*>::const_iterator itr = arcurion_servitors.begin(); itr != arcurion_servitors.end(); ++itr)
                if (*itr)
                {
                    (*itr)->SetInCombatWithZone();
                }
        }

        void KilledUnit(Unit* /*victim*/)
        {
            Random_Text = urand(0,2);
            switch(Random_Text) {
            case 0:
                me->MonsterYell("Mere mortals.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25803, true);
                    break;
            case 1:
                me->MonsterYell("Your shaman can't protect you.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25805, true);
                    break;
            case 2:
                me->MonsterYell("The aspects misplaced their trust.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25806, true);
                    break;
            }
        }

        void JustDied(Unit* /*who*/)
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_ARCURION, DONE);
            }

            me->MonsterSay("Nothing! Nothing....", LANG_UNIVERSAL, 0);
            me->SendPlaySound(25797, true);

            // Despawn all arcurion adds
            std::list<Creature*> arcurion_servitors;
            GetCreatureListWithEntryInGrid(arcurion_servitors, me, ARCURION_FROZEN_SERVITOR, 200.0f);
            for (std::list<Creature*>::const_iterator itr = arcurion_servitors.begin(); itr != arcurion_servitors.end(); ++itr)
                if (*itr)
                {
                    (*itr)->DespawnOrUnsummon(0);
                }

            // Despawn Icewall
            if (GameObject* Firewall = me->FindNearestGameObject(ICE_WALL, 500.0f))
                Firewall->Delete();
        }

        void UpdateAI(const uint32 diff)
        {
            if (instance->GetData(DATA_MOVEMENT_PROGRESS) == 7)
            {
                if (Intro_Timer <= diff)
                {
                    switch (Intro_Dialogue) {
                    case 0:
                        Intro_Timer = 1500;
                        break;
                    case 1:
                        {
                            me->MonsterYell("You're a fool if you think to take your place as the Aspect of Earth, shaman!", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
                            me->SendPlaySound(25802, true);
                            Intro_Timer = 10000;
                            break;
                        }
                    case 2:
                        {
                            Creature * thrall = me->FindNearestCreature(54548, 100.0f, true);
                            if (thrall)
                            {
                                thrall->MonsterSay("We're surrounded. Dispatch the ascendant while I keep the ambushers at bay!", LANG_UNIVERSAL, 0);
                                thrall->SendPlaySound(25878, true);
                            }
                            Intro_Timer = 4000;
                            break;
                        }
                    case 3:
                        {
                            me->MonsterYell("You're a mere mortal. It is time you died like one.", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25804, true);
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                            me->SetReactState(REACT_AGGRESSIVE);
                            instance->SetData(DATA_MOVEMENT_PROGRESS, 1); // 8
                            break;
                        }
                    }
                    ++Intro_Dialogue;
                }
                else Intro_Timer -= diff;
            }

            if (!UpdateVictim())
                return;

            if (!Torment_Of_Frost)
            {
                if (Chain_Of_Frost_Timer <= diff)
                {
                    Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0);
                    if (target)
                        me->CastSpell(target, CHAINS_OF_FROST, false);
                    Chain_Of_Frost_Timer = 26000;
                }
                else Chain_Of_Frost_Timer -= diff;

                if (Icy_Tomb_Timer <= diff)
                {
                    Creature * thrall = me->FindNearestCreature(54548, 100.0f, true);
                    if (thrall)
                    {
                        if (thrall->HasAura(ICY_TOMB_1)) // Delay new Icy Tomb timer if Thrall is already in Icy Tomb
                        {
                            Icy_Tomb_Timer = 10000;
                            return;
                        }

                        thrall->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE);
                        me->CastSpell(thrall, ICY_TOMB, false);
                        thrall->CastSpell(thrall, ICY_TOMB_1, false);

                        Random_Text = urand(0, 3);
                        switch (Random_Text) {
                        case 0:
                            me->MonsterYell("Enough, Shaman!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25807, true);
                            break;
                        case 1:
                            me->MonsterYell("None will survive!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25808, true);
                            break;
                        case 2:
                            me->MonsterYell("The Shaman is mine, focus on his companions!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25809, true);
                            break;
                        case 3:
                            me->MonsterYell("Freeze!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(25810, true);
                            break;
                        }
                    }

                    Icy_Tomb_Timer = 30000;
                }
                else
                {
                    if (Hand_Of_Frost_Timer <= diff)
                    {
                        Unit * player = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true);
                        if (player)
                            me->CastSpell(player, HAND_OF_FROST, false);
                        Hand_Of_Frost_Timer = 4000 + urand(0, 1000);
                    }
                    else Hand_Of_Frost_Timer -= diff;

                    Icy_Tomb_Timer -= diff;
                }
            }

            if (Check_Hp_Timer <= diff)
            {
                if (me->HealthBelowPct(Pct))
                {
                    if (Pct > 30)
                    {
                        std::list<Creature*> arcurion_servitors;
                        GetCreatureListWithEntryInGrid(arcurion_servitors, me, ARCURION_FROZEN_SERVITOR, 200.0f);
                        for (std::list<Creature*>::const_iterator itr = arcurion_servitors.begin(); itr != arcurion_servitors.end(); ++itr)
                            if (*itr)
                            {
                                (*itr)->GetAI()->DoAction();
                            }
                    }

                    Pct -= 10;
                }

                if (me->HealthBelowPct(35))
                {
                    if (!Bloodlust)
                    {
                        Creature * thrall = me->FindNearestCreature(THRALL, 100.0f, true);
                        if (thrall)
                        {
                            thrall->CastSpell(thrall, BLOODLUST, false);
                            thrall->MonsterYell("You've almost got him! Og'nor ka Lok'tar - Now we finish this!", LANG_UNIVERSAL, 0);
                            thrall->SendPlaySound(25881, true);
                        }
                        Bloodlust = true;
                    }
                }

                if (me->HealthBelowPct(30))
                {
                    if (!Torment_Of_Frost)
                    {
                        Torment_Of_Frost = true;
                        me->InterruptNonMeleeSpells(true, 0, true);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                        me->CastSpell(me, TORMENT_OF_FROST, false);

                        me->MonsterYell("The Hour of Twilight falls - the end of all things - you can't stop it. You are nothing. NOTHING!", LANG_UNIVERSAL, 0);
                        me->SendPlaySound(25801, true);
                    }
                }

                Check_Hp_Timer = 5000;
            }
            else Check_Hp_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_arcurion_frozen_servitor : public CreatureScript
{
public:
    npc_arcurion_frozen_servitor() : CreatureScript("npc_arcurion_frozen_servitor") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_arcurion_frozen_servitorAI(pCreature);
    }

    struct npc_arcurion_frozen_servitorAI : public ScriptedAI
    {
        npc_arcurion_frozen_servitorAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Icy_Boulder_Timer;
        uint32 Health;
        uint32 Cast_Faster_Timer;
        uint32 Current_Health;
        bool Cast;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            Icy_Boulder_Timer = urand(5000, 30000);
            Cast = true;
            Health = 100;
        }

        void JustDied(Unit* /*who*/) { }

        void DoAction(const int32 /*param*/)
        {
            if (Creature * arcurion = me->FindNearestCreature(54590, 250.0, true))
            {
                if (arcurion->HealthBelowPct(30))
                    Cast = false;

                Current_Health = arcurion->GetHealthPct();
                if (Current_Health > Health)
                {
                    int round_health_up = 0;
                    round_health_up = 10 - (Current_Health % 10);
                    Health = Current_Health + round_health_up;
                    Cast_Faster_Timer = 15000;
                }
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (Cast)
            {
                if (Icy_Boulder_Timer <= diff)
                {
                    if (Health < 40)
                        Health = 40;

                    Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true);
                    if (target)
                    {
                        me->CastSpell(target, ICY_BOULDER, false);
                        switch (Health)
                        {
                        case 100:
                            Icy_Boulder_Timer = 3000 + urand(0, 35000);
                            break;
                        case 90:
                            Icy_Boulder_Timer = 3000 + urand(0, 30000);
                            break;
                        case 80:
                            Icy_Boulder_Timer = 3000 + urand(0, 25000);
                            break;
                        case 70:
                            Icy_Boulder_Timer = 3000 + urand(0, 20000);
                            break;
                        case 60:
                            Icy_Boulder_Timer = 3000 + urand(0, 15000);
                            break;
                        case 50:
                            Icy_Boulder_Timer = 3000 + urand(0, 10000);
                            break;
                        case 40:
                            Icy_Boulder_Timer = 3000 + urand(0, 5000);
                            break;
                        default:
                            break;
                        }
                    }
                }
                else Icy_Boulder_Timer -= diff;
            }

            if (Cast_Faster_Timer <= diff)
            {
                Health -= 10;
                Cast_Faster_Timer = 15000;
            }
            else Cast_Faster_Timer -= diff;
        }
    };
};

class npc_icy_tomb : public CreatureScript
{
public:
    npc_icy_tomb() : CreatureScript("npc_icy_tomb") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_icy_tombAI(pCreature);
    }

    struct npc_icy_tombAI : public ScriptedAI
    {
        npc_icy_tombAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        void JustDied(Unit* /*who*/)
        {
            // Remove stun from Thrall
            Creature * thrall = me->FindNearestCreature(THRALL, 100.0f, true);
            if (thrall)
            {
                thrall->RemoveAura(ICY_TOMB_1);
                thrall->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                thrall->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            }
        }

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);
        }
    };
};

class npc_icewall : public CreatureScript
{
public:
    npc_icewall() : CreatureScript("npc_icewall") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_icewallAI(pCreature);
    }

    struct npc_icewallAI : public ScriptedAI
    {
        npc_icewallAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        void JustDied(Unit* /*who*/)
        {
            if (GameObject* Icewall = me->FindNearestGameObject(ICE_WALL2, 50.0f))
            {
                Icewall->UseDoorOrButton();

                Creature * thrall = me->FindNearestCreature(THRALL, 150.0f, true);
                Creature * thrall_ghost_wolf = me->FindNearestCreature(THRALL_WOLF, 150.0f, true);
                if (thrall)
                    thrall->SetVisible(false);

                if (thrall_ghost_wolf)
                    thrall_ghost_wolf->SetVisible(true);
            }

            if (instance)
                instance->SetData(DATA_INSTANCE_PROGRESS, 2); // 2
        }

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);
        }
    };
};

void AddSC_boss_arcurion()
    {
        new boss_arcurion();
        new npc_arcurion_frozen_servitor();
        new npc_icy_tomb();
        new npc_icewall();
    }