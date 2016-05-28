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
Encounter: Echo of Sylvanas
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

// NPCs
enum NPC
{
    ECHO_OF_SYLVANAS        = 54123,
    TRIGGER_WITH_MIST       = 54197,
    RISEN_GHOUL             = 54191,
    BRITTLE_GHOUL           = 54952,
    BLIGHTED_ARROW_TRIGGER  = 54403,
};

// Spells
enum Spells 
{
    // Echo of Sylvanas
    UNHOLY_SHOT                           = 101411, // Dmg Spell
    BLACK_ARROW                           = 101404, // Smg Spell
    SHRIEK_OF_THE_HIGHBORNE               = 101412, // Dmg Spell

    CALLING_OF_THE_HIGHBORNE_AURA         = 102581, // Aura
    CALLING_OF_THE_HIGHBORNE              = 100686, // Buff for Immune + Circle under Sylvanas
    CALLING_OF_THE_HIGHBORNE_MARK_POINT   = 100867, // Circle under Ghouls at their spawn point locations
    CALLING_OF_THE_HIGHBORNE_LINK         = 100862, // Triggers Wracking Pain Link

    WRACKING_PAIN_IN_MIST                 = 101257, // Dmg in purple mist
    WRACKING_PAIN_LINK                    = 100865, // Link between ghouls
    WRACKING_PAIN_LINK_DMG                = 101221, // Dmg when crossing link

    BLIGHTED_ARROWS_SUMMON_SPELL          = 100547, // Summon 54403 trigger
    BLIGHTED_ARROWS_BOILING_AURA          = 101552, // Purple swamp aura
    BLIGHTED_ARROWS_BOOM                  = 100763, // Explosion
    BLIGHTED_ARROWS_VISUAL_ANIMATION      = 101401, // Visual animation

    SACRIFICE                             = 101348, // Kill all nearby players when sacrificing ghouls
    DEATH_GRIP                            = 101987, // Grip player to Sylvanas
    DEATH_GRIP_VISUAL                     = 101397, // Beam to all griping players
    SUMMON_GHOULS                         = 102603, // Summmoning ghouls

    // Ghouls (triggers) with Mist
    CALLING_OF_THE_HIGHBORNE_VISUAL       = 105766, // Purple Mist
    SHRINK_AURA                           = 101318, // Aura
    SHRINK_MOD_SCALE                      = 101271, // Mod scale -10%
    SUMMON_RISEN_GHOUL                    = 101200, // Summon Risen Ghoul

    // Risen Ghouls
    SEEPING_SHADOWS                       = 103182, // Increase dmg taken for every missing 5% HP

    // Brightle Ghouls
};

// Echo of Sylvanas
class boss_echo_of_sylvanas : public CreatureScript
{
public:
    boss_echo_of_sylvanas() : CreatureScript("boss_echo_of_sylvanas") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_echo_of_sylvanasAI (pCreature);
    }

    struct boss_echo_of_sylvanasAI : public ScriptedAI
    {
        boss_echo_of_sylvanasAI(Creature *creature) : ScriptedAI(creature) 
        {
            instance = creature->GetInstanceScript();
            me->CastSpell(me, CALLING_OF_THE_HIGHBORNE_AURA, true);
        }

        InstanceScript* instance;
        // Timers
        uint32 Port_Timer;
        uint32 Summon_Purple_Mist_Timer;
        uint32 Move_Again;
        uint32 Blink_Timer;
        uint32 Unholy_Shot_Timer;
        uint32 Black_Arrow_Timer;
        uint32 Shriek_Timer;
        uint32 Blighted_Arrows_Timer;
        uint32 Calling_Of_The_Highborne;
        uint32 Sacrifice_Timer;
        uint32 Wracking_Check;
        uint32 Wracking_Pain_Link_Timer;
        uint32 Wracking_Check_Counter;
        uint32 Grip_Timer;
        uint32 Blighted_Arrows_Visual_Cast;
        uint32 Brittle_Despawn_Timer;
        int Phase;
        int achievement_counter;
        int unholy_shot_counter;
        float freeAngle;
        float maxRange;
        bool Blighted_Arrows;
        bool Blighted_Arrows_Visual_Check;
        bool Ghoul_Died;
        bool Achievement_Gain;
        bool Brittle_Despawn;

        void JustReachedHome()
        {
            me->CastSpell(me, CALLING_OF_THE_HIGHBORNE_AURA, true);
        }

        void Reset()
        {
            if (instance)
            {
                if(instance->GetData(TYPE_ECHO_OF_SYLVANAS)!=DONE)
                    instance->SetData(TYPE_ECHO_OF_SYLVANAS, NOT_STARTED);
            }

            me->MonsterYell("And so ends your story.", LANG_UNIVERSAL, 0, 50.0f);
            me->SendPlaySound(25968, true);

            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->AddUnitMovementFlag(MOVEMENTFLAG_LEVITATING);
            me->SetReactState(REACT_AGGRESSIVE);
            me->CastSpell(me, SUMMON_GHOULS, true); // Summon ghouls around Sylvanas

            Phase = 0;
            achievement_counter = 0;
            Ghoul_Died = false;
            Achievement_Gain = false;
            Brittle_Despawn = false;
            KillGhouls();

            // Event Timers
            Blink_Timer = 34500;
            Port_Timer = 35000;
            Summon_Purple_Mist_Timer = 36000;
            Grip_Timer = 42000;
            Calling_Of_The_Highborne = 38000; 
            Sacrifice_Timer = 65000;
            Wracking_Pain_Link_Timer = 47000;
            Wracking_Check = 47000;
            Wracking_Check_Counter = 0;
            Brittle_Despawn_Timer = 1500;

            maxRange = 67.0f;

            // Dmg spell timers
            Blighted_Arrows_Timer = 17000;
            Unholy_Shot_Timer = 10000;
            Black_Arrow_Timer = 28000;
            Shriek_Timer = 14000;
            unholy_shot_counter = 0;
        }

        void InEvadeMode() {}

        void EnterCombat(Unit * /*who*/) 
        {
            if (instance)
            {
                instance->SetData(TYPE_ECHO_OF_SYLVANAS, IN_PROGRESS);
            }

            me->MonsterYell("Another band of Deathwing's converts? I'll be sure your death is especially painful.", LANG_UNIVERSAL, 0);
            me->SendPlaySound(25966, true);

            me->RemoveAura(SUMMON_GHOULS);
            me->RemoveAura(CALLING_OF_THE_HIGHBORNE_AURA);
            me->RemoveUnitMovementFlag(MOVEMENTFLAG_LEVITATING);
        }

        void KilledUnit(Unit* /*victim*/)
        {
            me->MonsterYell("Cry havoc!", LANG_UNIVERSAL, 0);
            me->SendPlaySound(25969, true);
        }

        void JustDied(Unit * /*who*/)
        {
            if (instance)
            {
                instance->SetData(TYPE_ECHO_OF_SYLVANAS, DONE);
                instance->SetData(DATA_ECHO_KILLED, 1);
            }

            me->MonsterYell("This... isn't how it's supposed to... end.", LANG_UNIVERSAL, 0);
            me->SendPlaySound(25967, true);
            KillGhouls();

            if (Achievement_Gain == true)
            {
                Map::PlayerList const &playerList = me->GetMap()->GetPlayers();
                if (!playerList.isEmpty())
                    for (Map::PlayerList::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
                        if (Player* pPlayer = i->getSource())
                            pPlayer->GetAchievementMgr().CompletedAchievement(sAchievementStore.LookupEntry(6130), true); // Severed Ties
            }

            // Sumon Time transit device
            Map::PlayerList const &playerList = me->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
                if (Player * pPlayer = i->getSource())
                    pPlayer->SummonGameObject(209438, 3821.3f, 935.617f, 55.8123f, 5.50447f, 0.0f, 0.0f, 0.379594f, -0.925153f, 86400);
            return;
        }

        void KillGhouls()
        {
            std::list<Creature*> trigger_mist;
            me->GetCreatureListWithEntryInGrid(trigger_mist, TRIGGER_WITH_MIST, 150.0f);
            for (std::list<Creature*>::iterator itr = trigger_mist.begin(); itr != trigger_mist.end(); ++itr)
            {
                if (*itr)
                    (*itr)->Kill((*itr));
            }

            std::list<Creature*> risen_ghouls;
            me->GetCreatureListWithEntryInGrid(risen_ghouls, RISEN_GHOUL, 150.0f);
            for (std::list<Creature*>::iterator itr = risen_ghouls.begin(); itr != risen_ghouls.end(); ++itr)
            {
                if (*itr)
                    (*itr)->Kill((*itr));
            }
        }

        // Summon triggers with Mist
        void SummonPurpleMist()
        {
            float angle = me->GetOrientation();
            double angleAddition = ((2*M_PI)/8); // 45 Digrees - 8 Adds 
            angle = MapManager::NormalizeOrientation(angle);
            float distance = 27.0f;

            for(uint32 i = 0; i < 8;)
            {
                me->SummonCreature(TRIGGER_WITH_MIST,me->GetPositionX()+cos(angle)*distance,me->GetPositionY()+sin(angle)*distance,me->GetPositionZ(),angle,TEMPSUMMON_TIMED_DESPAWN, 31000);
                angle += angleAddition;
                i = i + 1;
            }

            Summon_Purple_Mist_Timer = 65000;
        }

        // Port
        void Port() 
        {
            me->CastSpell(me, 101812, true); // Visual Port

            freeAngle = -1.0f;
            Ghoul_Died = false;
            maxRange = 67.0f; // Reset Max range for dmg in mist

            // Port Sylvanas
            me->GetMotionMaster()->MoveJump(3844.3298f, 910.2124f, 56.146f, 200.0f, 200.0f);
            me->NearTeleportTo(3844.3298f, 910.2124f, 56.146f, 2.472f);

            // Stay and stop attack
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);
            me->GetMotionMaster()->MoveIdle();
            me->AttackStop();
            me->SendMovementFlagUpdate();

            Wracking_Check = 12000;
            Wracking_Check_Counter = 0;
            achievement_counter = 0;

            me->MonsterYell("Watch, heathens, as death surrounds you!", LANG_UNIVERSAL, 0);
            me->SendPlaySound(25970, true);

            Port_Timer = 65000; // Set new time
            Summon_Purple_Mist_Timer = 1000;

            Unholy_Shot_Timer = 42000;
            Black_Arrow_Timer = 58000;
            Shriek_Timer = 48000;
            Blighted_Arrows_Timer = 34000;
            unholy_shot_counter = 0;

            Calling_Of_The_Highborne = 3000;
            Grip_Timer = 7000;
            Sacrifice_Timer = 31000;
            Wracking_Pain_Link_Timer = 12000;
        }

        // Sacrifice and start attacking again
        void Sacrifice()
        {
            me->CastSpell(me, SACRIFICE, true); // Visual explosion of Sacrifice + Dmg nearby players
            me->RemoveAura(CALLING_OF_THE_HIGHBORNE); // Remove aura from sylvanas

            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_AGGRESSIVE);
            me->GetMotionMaster()->MoveChase(me->GetVictim());

            Sacrifice_Timer = 65000;
        }

        // All Ghouls cast beam on nearby ghoul
        void WrackingPainLink() 
        {
            std::list<Creature*> risen_ghouls;
            me->GetCreatureListWithEntryInGrid(risen_ghouls, RISEN_GHOUL, 100.0f);

            for (std::list<Creature*>::iterator itr = risen_ghouls.begin(); itr != risen_ghouls.end(); ++itr)
            {
                (*itr)->CastSpell((*itr), WRACKING_PAIN_LINK, true); // Link Ghouls with beam
            }
        }

        void SetGhoulAngle(float angle)
        {
            // Uhel vzhledem k Sylvanas pod kterym umrel risen ghoul
            freeAngle = angle;
            Ghoul_Died = true;
            achievement_counter = achievement_counter + 1;

            if (achievement_counter == 2)
                Achievement_Gain = true;
        }

        // Deal dmg to players in purple Mist
        void CastWrackingPainToPlayers(void)
        {
            Creature * ghoul = me->FindNearestCreature(TRIGGER_WITH_MIST, 100.0f, true);
            if (!ghoul)
                return;

            const float MAX_GHOULS = 8.0f;

            if (Wracking_Check_Counter%5 == 0)
            {
                maxRange = maxRange - 8.0f;
            }

            Map::PlayerList const &playerList = me->GetMap()->GetPlayers();
            if (!playerList.isEmpty())
            for (Map::PlayerList::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
                if (Player* pPlayer = i->getSource())
                {
                    // Hrac je za ghoulami
                    if ((me->GetExactDist2d(pPlayer) > me->GetExactDist2d(ghoul)) && (me->GetExactDist2d(pPlayer) < maxRange))
                    {
                        Position pos;
                        me->GetPosition(&pos);

                        float arcAngle = ((2 * M_PI) / MAX_GHOULS);

                        pos.m_orientation = freeAngle;

                        if ((pos.HasInArc(arcAngle,pPlayer) == false) || (Ghoul_Died == false))
                            pPlayer->CastSpell(pPlayer, WRACKING_PAIN_IN_MIST, true, NULL, NULL, me->GetGUID());
                    }
                }
        }

        // Blighted Arrows
        void BlightedArrows()
        {
            Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true);
            if (target)
            {
                float distance = me->GetExactDist2d(target);
                distance -= 6.0;
                float angle = me->GetOrientation();

                for(uint32 i = 0; i < 5;)
                {
                    me->SummonCreature(BLIGHTED_ARROW_TRIGGER,me->GetPositionX()+cos(angle)*distance,me->GetPositionY()+sin(angle)*distance,me->GetPositionZ(),angle,TEMPSUMMON_TIMED_DESPAWN, 4000);
                    distance += 6.0;
                    i = i + 1;
                }
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
            return;

            if (Brittle_Despawn == false)
            {
                if (Brittle_Despawn_Timer <= diff)
                {
                    std::list<Creature*> brittle_ghouls;
                    GetCreatureListWithEntryInGrid(brittle_ghouls, me, BRITTLE_GHOUL, 100.0f);
                    for (std::list<Creature*>::const_iterator itr = brittle_ghouls.begin(); itr != brittle_ghouls.end(); ++itr)
                        if (*itr)
                            (*itr)->ForcedDespawn();

                    Brittle_Despawn = true;
                } else Brittle_Despawn_Timer -= diff;
            }

            if (Blink_Timer <= diff)
            {
                me->CastSpell(me, 101398, true);
                Blink_Timer = 65000;
            }
            else Blink_Timer -= diff;

            // Enable moving after Blighted Arrows
            if (Blighted_Arrows == true)
            {
                if (Move_Again <= diff)
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                    me->GetMotionMaster()->MoveJump(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ()-6, 9.0f, 9.0f);
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->GetMotionMaster()->MoveChase(me->GetVictim());
                    Blighted_Arrows = false;
                }
                else Move_Again -= diff;
            }

            // Cast Visual Arrows animation
            if (Blighted_Arrows_Visual_Check == true)
            {
                if (Blighted_Arrows_Visual_Cast <= diff)
                {
                    me->CastSpell(me, BLIGHTED_ARROWS_VISUAL_ANIMATION, true);
                    Blighted_Arrows_Visual_Check = false;
                }
                else Blighted_Arrows_Visual_Cast -= diff;
            }

            // Blighted Arrows
            if (Blighted_Arrows_Timer <= diff)
            {
                BlightedArrows();
                me->GetMotionMaster()->MoveJump(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ()+6, 9.0f, 9.0f);

                // Stay and stop attack
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                me->SetReactState(REACT_PASSIVE);
                me->GetMotionMaster()->MoveIdle();
                me->AttackStop();
                me->SendMovementFlagUpdate();

                Blighted_Arrows = true;
                Move_Again = 3500;
                Blighted_Arrows_Visual_Check = true;
                Blighted_Arrows_Visual_Cast = 2000;
                Blighted_Arrows_Timer = 65000;
            }
            else Blighted_Arrows_Timer -= diff;

            // Unholy Shot
            if (Unholy_Shot_Timer <= diff)
            {
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true);
                if (target)
                    me->CastSpell(target, UNHOLY_SHOT, false);

                unholy_shot_counter = unholy_shot_counter + 1;

                if (Phase == 0)
                    Unholy_Shot_Timer = 22000;
                else Unholy_Shot_Timer = 10000;

                if (unholy_shot_counter == 2)
                    Unholy_Shot_Timer = 65000;
            }
            else Unholy_Shot_Timer -= diff;

            // Black Arrow
            if (Black_Arrow_Timer <= diff)
            {
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true);
                if (target)
                    me->CastSpell(target, BLACK_ARROW, false);
                Black_Arrow_Timer = 65000;
            }
            else Black_Arrow_Timer -= diff;

            // Shriek of the Highborne
            if (Shriek_Timer <= diff)
            {
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true);
                if (target)
                    me->CastSpell(target, SHRIEK_OF_THE_HIGHBORNE, true);
                Shriek_Timer = 65000;
            } 
            else Shriek_Timer -= diff;

            // Encounter Start Timers
            if (Phase == 0)
            {
                // Port
                if (Port_Timer <= diff)
                {
                    Port();
                    Phase = 1;
                }
                else Port_Timer -= diff;
            }

            // Phase 1 Timers
            if (Phase == 1)
            {
                // Port
                if (Port_Timer <= diff)
                    Port();
                else Port_Timer -= diff;

                // Sacrifice
                if (Sacrifice_Timer <= diff)
                    Sacrifice();
                else Sacrifice_Timer -= diff;

                // Purple Mist triggers
                if (Summon_Purple_Mist_Timer <= diff)
                    SummonPurpleMist(); // Summon ghoul triggers
                else Summon_Purple_Mist_Timer -= diff;

                // Calling Of The Highborne
                if (Calling_Of_The_Highborne <= diff)
                {
                    me->CastSpell(me, CALLING_OF_THE_HIGHBORNE, true);
                    me->GetMotionMaster()->MoveJump(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ()+4, 4.0f, 4.0f);
                    Calling_Of_The_Highborne = 65000;
                }
                else Calling_Of_The_Highborne -= diff;

                // Wracking Pain Check 
                if (me->HasAura(CALLING_OF_THE_HIGHBORNE))
                {
                    if (Wracking_Check <= diff)
                    {
                        CastWrackingPainToPlayers();
                        Wracking_Check_Counter = Wracking_Check_Counter +1;
                        Wracking_Check = 500;
                    }
                    else Wracking_Check -= diff;
                }

                // Cast Wracking Pain Link
                if (Wracking_Pain_Link_Timer <= diff)
                {
                    WrackingPainLink();
                    Wracking_Pain_Link_Timer = 65000;
                }
                else Wracking_Pain_Link_Timer -= diff;

                // Grip all players to Sylvanas
                if (Grip_Timer <= diff)
                {
                    Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                        for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                            if (Player* player = i->getSource())
                            {
                                Creature * sylvanas = me->FindNearestCreature(ECHO_OF_SYLVANAS, 100.0f, true);
                                if (sylvanas)
                                    player->CastSpell(sylvanas, DEATH_GRIP, true);
                            }

                    me->CastSpell(me, DEATH_GRIP_VISUAL, false); // Visual effect - beams 

                    Grip_Timer = 65000;
                }
                else Grip_Timer -= diff;
            }

            DoMeleeAttackIfReady();
        }

    };
};

// Ghoul With Mist
class npc_ghoul_with_mist : public CreatureScript
{
public:
    npc_ghoul_with_mist() : CreatureScript("npc_ghoul_with_mist") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ghoul_with_mistAI (pCreature);
    }

    struct npc_ghoul_with_mistAI : public ScriptedAI
    {
        npc_ghoul_with_mistAI(Creature *c) : ScriptedAI(c) {}

        uint32 Walk_Timer;
        uint32 Mist_Timer;
        uint32 Visual_Summon_Timer;
        uint32 Mark_Point_Timer;
        bool Walk;

        void Reset()
        {
            // Turn them 
            float angle = me->GetOrientation();
            double angleAddition = (M_PI);
            angle += angleAddition; 
            angle = MapManager::NormalizeOrientation(angle);
            me->SetOrientation(angle); // Turn them to Sylvanas

            Walk = false;
            Walk_Timer = 12000; // Maybe 12500 would be better
            Mist_Timer = 11000;
            Mark_Point_Timer = 4000;
            Visual_Summon_Timer = 6000;

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);

            me->SetVisible(true);
            me->SetWalk(true);
        }

        void EnterCombat(Unit * /*who*/) { }

        void UpdateAI(const uint32 diff) 
        { 
            // Walk to Sylvanas
            if (Walk_Timer <= diff)
            {
                if (Walk == false)
                {
                    Creature * sylvanas = me->FindNearestCreature(ECHO_OF_SYLVANAS, 300, true);
                    if (sylvanas)
                        me->GetMotionMaster()->MovePoint(0, sylvanas->GetPositionX(), sylvanas->GetPositionY(), sylvanas->GetPositionZ()-4);
                    Walk = true;
                }
            }
            else Walk_Timer -= diff;

            if (Mist_Timer <= diff)
            {
                me->SetVisible(true);
                me->CastSpell(me, CALLING_OF_THE_HIGHBORNE_VISUAL, true);
                me->CastSpell(me, SHRINK_AURA, false); // Set Better Scale !!!
                Mist_Timer = 65000;
            }
            else Mist_Timer -= diff;

            if (Visual_Summon_Timer <= diff)
            {
                me->CastSpell(me, SUMMON_RISEN_GHOUL, false); // Summon Risen Ghoul
                Visual_Summon_Timer = 65000;
            }
            else Visual_Summon_Timer -= diff;

            if (Mark_Point_Timer <= diff)
            {
                if (Aura * a = me->AddAura(CALLING_OF_THE_HIGHBORNE_MARK_POINT,me)) 
                    a->SetDuration(4000);
                Mark_Point_Timer = 65000;
            }
            else Mark_Point_Timer -= diff;
        }

    };
};

// Risen Ghoul
class risen_ghoul : public CreatureScript
{
public:
    risen_ghoul() : CreatureScript("risen_ghoul") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new risen_ghoulAI (pCreature);
    }

    struct risen_ghoulAI : public ScriptedAI
    {
        risen_ghoulAI(Creature *c) : ScriptedAI(c) {}

        uint32 Walk_Timer;
        uint32 Link_Check_Dmg_Timer;
        uint32 Selectable_Timer;
        float grid_search;
        bool Walk;
        bool Selectable;

        void Reset()
        {
            Walk_Timer = 6000;
            Link_Check_Dmg_Timer = 5000;
            Selectable_Timer = 4000;
            Walk = false;
            Selectable = false;
            grid_search = 27.0;

            me->PlayOneShotAnimKit(1490); // Raise from death animation
            me->CastSpell(me, SEEPING_SHADOWS, true);

            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            me->SetWalk(true);
            me->SetLevel(85);
            me->SetMaxHealth(1084860);
        }

        // Ghoul umre
        void JustDied(Unit * killer)
        {
            Creature * pSylvanas = me->FindNearestCreature(ECHO_OF_SYLVANAS, 100.0f, true);

            if (pSylvanas == NULL)
                return;

            if (boss_echo_of_sylvanas::boss_echo_of_sylvanasAI* pAI = (boss_echo_of_sylvanas::boss_echo_of_sylvanasAI*)(pSylvanas->GetAI()))
                pAI->SetGhoulAngle(pSylvanas->GetAngle(me));

            // Despawn Mist trigger + Ghoul`s corpose so visual effect of wracking link is interrupted
            me->RemoveCorpse();
            Creature * ghoul_with_mist = me->FindNearestCreature(TRIGGER_WITH_MIST, 2.0f, true);
            if (ghoul_with_mist)
                ghoul_with_mist->DespawnOrUnsummon(0);
        }

        void EnterCombat(Unit * /*who*/) { }
        void UpdateAI(const uint32 diff) 
        { 
            // Walk to Sylvanas
            if (Walk_Timer <= diff)
            {
                if ((Walk == false) && (Selectable == true))
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                    Creature * sylvanas = me->FindNearestCreature(ECHO_OF_SYLVANAS, 300, true);
                    if (sylvanas)
                        me->GetMotionMaster()->MovePoint(0, sylvanas->GetPositionX(), sylvanas->GetPositionY(), sylvanas->GetPositionZ()-4);
                    Walk = true;
                }
            }
            else Walk_Timer -= diff;

            if (Selectable_Timer <= diff)
            {
                if (Selectable == false)
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                    Selectable = true;
                }
            } 
            else Selectable_Timer -= diff;

            // Wracking Pain dmg if player is in link
            if (Link_Check_Dmg_Timer <= diff)
            {
                Link_Check_Dmg_Timer = 500;
                grid_search = (grid_search-(2*0.3375)); // grid_search-(2*((13.5yd/20s)*0.5))

                Map::PlayerList const &playerList = me->GetMap()->GetPlayers();

                std::list<Creature*> another_ghouls;
                me->GetCreatureListWithEntryInGrid(another_ghouls, RISEN_GHOUL, grid_search);

                for (std::list<Creature*>::iterator itr = another_ghouls.begin(); itr != another_ghouls.end(); ++itr)
                {
                    for (Map::PlayerList::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
                    if (Player* player = i->getSource())
                    {
                        float distance1 = me->GetDistance(player);
                        float distance2 = (*itr)->GetDistance(player);
                        if (((player)->IsInBetween(me, (*itr), (*itr)->GetObjectSize()) == true)
                            && (distance1 < distance2))
                            {
                                player->CastSpell(player, WRACKING_PAIN_LINK_DMG, true);
                            }
                    }
                }
            }
            else Link_Check_Dmg_Timer -= diff;

            // Set correct stack amount for Seeping Shadows
            for (uint32 i = 0 ; i < 1; i++)
            {
                uint32 percentage;
                uint32 stack;
                percentage = me->GetHealthPct();
                stack = 20-(percentage/5);
                if (me->HasAura(SEEPING_SHADOWS))
                    me->SetAuraStack(SEEPING_SHADOWS, me, stack);
            }
        }

    };
};

// Blighted Arrows 
class npc_blighted_arrows : public CreatureScript
{
public:
    npc_blighted_arrows() : CreatureScript("npc_blighted_arrows") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_blighted_arrowsAI (pCreature);
    }

    struct npc_blighted_arrowsAI : public ScriptedAI
    {
        npc_blighted_arrowsAI(Creature *c) : ScriptedAI(c) {}

        uint32 Boom_Timer;
        bool Boom;

        void Reset() 
        {
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, BLIGHTED_ARROWS_BOILING_AURA, false); // Purple boiling aura

            Boom = false;
            Boom_Timer = 3000;
        }

        void EnterCombat(Unit * /*who*/) { }

        void UpdateAI(const uint32 diff) 
        {
            if (Boom_Timer <= diff)
            {
                if (Boom == false)
                {
                    me->CastSpell(me, BLIGHTED_ARROWS_BOOM, true);
                    Boom = true;
                }
            }
            else Boom_Timer -= diff;
        }
    };
};

// Brittle Ghoul
class npc_brittle_ghoul : public CreatureScript
{
public:
    npc_brittle_ghoul() : CreatureScript("npc_brittle_ghoul") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_brittle_ghoulAI (pCreature);
    }

    struct npc_brittle_ghoulAI : public ScriptedAI
    {
        npc_brittle_ghoulAI(Creature *c) : ScriptedAI(c) {}

        uint32 Despawn_Timer;

        void Reset() 
        {
            me->SetReactState(REACT_PASSIVE);
            Despawn_Timer = 20000;
        }

        void EnterCombat(Unit * /*who*/) { }
        void UpdateAI(const uint32 diff)  
        {
            if (Despawn_Timer <= diff)
            {
                me->DespawnOrUnsummon(0);
                Despawn_Timer = 20000;
            } 
            else Despawn_Timer -= diff;
        }
    };
};

class IsNotRisenGhoul
{
    public:
        bool operator()(Unit* target) const
        {
            if (target && target->GetEntry() == RISEN_GHOUL)
                    return false;
            return true;
        }
};

// Wracking Pain Link
class spell_wracking_pain_link : public SpellScriptLoader
{
    public:
        spell_wracking_pain_link() : SpellScriptLoader("spell_wracking_pain_link") {}

        class spell_wracking_pain_linkSpellScript : public SpellScript
        {
            PrepareSpellScript(spell_wracking_pain_linkSpellScript)

            void RemoveInvalidTargets(std::list<Unit*>& unitList)
            {
                unitList.remove_if(IsNotRisenGhoul());
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_wracking_pain_linkSpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_AREA_ALLY_SRC);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_wracking_pain_linkSpellScript();
        }
};

// Blighted Arrows
class spell_blighted_arrows : public SpellScriptLoader
{
    public:
        spell_blighted_arrows() : SpellScriptLoader("spell_blighted_arrows") {}

        class spell_blighted_arrowsSpellScript : public SpellScript
        {
            PrepareSpellScript(spell_blighted_arrowsSpellScript)

            void RemoveInvalidTargets(std::list<Unit*>& unitList)
            {
                unitList.clear();
                std::list<Creature*> blighted_arrows;
                GetCreatureListWithEntryInGrid(blighted_arrows, GetCaster(), BLIGHTED_ARROW_TRIGGER, 100.0f);
                for (std::list<Creature*>::const_iterator itr = blighted_arrows.begin(); itr != blighted_arrows.end(); ++itr)
                    if (*itr)
                        unitList.push_back(*itr);
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_blighted_arrowsSpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_AREA_ALLY_SRC);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_blighted_arrowsSpellScript();
        }
};

void AddSC_boss_echo_of_sylvanas()
    {
        new boss_echo_of_sylvanas();
        new npc_ghoul_with_mist();
        new risen_ghoul();
        new npc_brittle_ghoul();
        new npc_blighted_arrows();

        new spell_blighted_arrows();
        new spell_wracking_pain_link();
    }

//////////////////////////////////////////////////////////////////////////////////////
///////////////////////////         SQL QUERY          ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
/*
-- Echo of Sylvanas
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('54123','0','0','0','0','0','39559','0','0','0','Echo of Sylvanas','','','0','87','87','3','16','16','0','1','1.3','1','3','50000','55000','0','0','1','1300','1300','2','0','1','0','0','0','0','0','0','0','0','7','72','54123','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','20000','20000','','0','3','59.35','38.5987','1','0','0','0','0','0','0','0','181','1','54123','646922239','0','boss_echo_of_sylvanas','15595');

-- Risen Ghoul
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('54191','0','0','0','0','0','38671','0','0','0','Risen Ghoul','','','0','85','85','3','16','16','0','0.5','1','1','3','0','0','0','0','1','0','0','2','0','0','0','0','0','0','0','0','0','0','6','72','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','0','3','14.0322','1','1','0','0','0','0','0','0','0','66','1','0','646922239','0','risen_ghoul','15595');

-- Ghouls with Mist
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('54197','0','0','0','0','0','11686','0','0','0','Ghoul','','','0','85','85','3','16','16','0','0.5','1','0.8','0','0','0','0','0','1','0','0','1','0','0','0','0','0','0','0','0','0','0','10','1611661312','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','0','3','1','1','1','0','0','0','0','0','0','0','66','1','0','0','0','npc_ghoul_with_mist','15595');

-- Brittle Ghoul
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('54952','0','0','0','0','0','38671','0','0','0','Brittle Ghoul','','','0','85','85','3','16','16','0','1','1.14286','1','0','0','0','0','0','1','0','0','1','0','0','0','0','0','0','0','0','0','0','6','1024','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','1','3','1','1','1','0','0','0','0','0','0','0','115','1','0','0','0','npc_brittle_ghoul','15595');

-- Blighted Arrows triggers
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('54403','0','0','0','0','0','11686','0','0','0','Blighted Arrows','','','0','85','85','3','16','16','0','1','1.14286','1','0','0','0','0','0','1','0','0','1','0','0','0','0','0','0','0','0','0','0','10','1024','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','0','3','40','7','1','0','0','0','0','0','0','0','98','1','0','0','128','npc_blighted_arrows','15595');

-- Spells
insert into `spell_script_names` (`spell_id`, `ScriptName`) values('100865','spell_wracking_pain_link');
insert into `spell_script_names` (`spell_id`, `ScriptName`) values('101401','spell_blighted_arrows');
insert into `spell_script_names` (`spell_id`, `ScriptName`) values('101549','spell_blighted_arrows');
insert into `spell_script_names` (`spell_id`, `ScriptName`) values('101567','spell_blighted_arrows');

-- Loot
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54123','72798','3','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54123','72799','4','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54123','72800','3','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54123','72801','4','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54123','72802','4','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54123','72803','3','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54123','72804','4','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54123','72805','4','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54123','72806','3','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54123','72807','3','1','2','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54123','72810','37','1','1','1','1');
INSERT INTO `creature_loot_template` (`entry`, `item`, `ChanceOrQuestChance`, `lootmode`, `groupid`, `mincountOrRef`, `maxcount`) values('54123','72811','63','1','1','1','1');
*/