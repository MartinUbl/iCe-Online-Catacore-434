/*
* Copyright (C) 2010-2011 Project SkyFire <http://www.projectskyfire.org/>
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
Encounter: Alizabal
Dungeon: Baradin Hold
Difficulty: Normal
Mode: 10/25-man
Autor: Lazik
*/

#include "ScriptPCH.h"
#include "Spell.h"
#include "UnitAI.h"
#include "MapManager.h"
#include "baradin_hold.h"

enum NPC
{
    NPC_BOSS_ALIZABAL           = 55869,
};

// Spells
enum Spells
{
    BERSERK                 = 47008, // Berserk

    SKEWER                  = 104936, // Stun and dmg

    SEETHING_HATE           = 105067, // Priodic aura
    SEETHING_HATE_DUMMY     = 105065, // Dummy aura
    SEETHING_HATE_DUMMY_1   = 108090, // Dummy aura
    SEETHING_HATE_DMG_10    = 105069, // 10-Man dmg
    SEETHING_HATE_DMG_25    = 108094, // 25-Man dmg

    BLADE_DANCE_WHIRLWIND   = 105828, // Dummy, Pacify - force immune aura
    BLADE_DANCE_IMMUNE      = 105738, // Immune aura - forced by 105828
    BLADE_DANCE_CHARGE      = 105726, // Charge Destination
    BLADE_DANCE_ROOT        = 105784, // Root + trigger 104995
    BLADE_DANCE_SPIN        = 104995, // Periodic Aura - 104994
    BLADE_DANCE_SPIN_DMG    = 104994, // Dmg
    BLADE_DANCE_DUMMY       = 106248, // Dummy 105726
};

// Alizabal
class boss_alizabal : public CreatureScript
{
public:
    boss_alizabal() : CreatureScript("boss_alizabal") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_alizabalAI(pCreature);
    }

    struct boss_alizabalAI : public ScriptedAI
    {
        boss_alizabalAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Random_Text;
        uint32 SkewerOrSeething_Timer;
        uint32 SkewerOrSeething_Counter;
        uint32 SkewerOrSeething;
        uint32 Blade_Dance_Timer;
        uint32 Blade_Dance_Counter;
        uint32 Berserk_Timer;
        uint32 Charge_Timer;
        uint32 Spin_Timer;
        uint32 Check_Timer;
        bool Berserk;
        bool Blade_Dance;

        void Reset()
        {
            if (instance)
            {
                if (instance->GetData(DATA_ALIZABAL_EVENT) != DONE)
                    instance->SetData(DATA_ALIZABAL_EVENT, NOT_STARTED);
            }

            me->MonsterYell("I hate incompetent raiders.", LANG_UNIVERSAL, 0);
            me->SendPlaySound(25780, true);

            SkewerOrSeething_Timer = 8000;
            SkewerOrSeething_Counter = 0;
            SkewerOrSeething = 0;
            Blade_Dance_Timer = urand(25000, 35000);
            Blade_Dance_Counter = 0;
            Spin_Timer = 120000;
            Berserk_Timer = 300000;
            Check_Timer = 2000;

            Berserk = false;
            Blade_Dance = false;
        }

        void EnterCombat(Unit * /*who*/)
        {
            if (instance)
            {
                instance->SetData(DATA_ALIZABAL_EVENT, IN_PROGRESS);
            }

            me->MonsterYell("How I HATE this place. My captors may be long-dead, but don't think I won't take it all out on you miserable treasure - hunters.", LANG_UNIVERSAL, 0);
            me->SendPlaySound(25779, true);

            SkewerOrSeething = urand(0, 1);
        }

        void KilledUnit(Unit* /*victim*/)
        {
            Random_Text = urand(0, 3);
            switch (Random_Text) {
            case 0:
                me->MonsterYell("I still hate you.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25781, true);
                break;
            case 1:
                me->MonsterYell("Do you hate me? Good.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25782, true);
                break;
            case 2:
                me->MonsterYell("I hate mercy.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25783, true);
                break;
            case 3:
                me->MonsterYell("I didn't hate that.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25784, true);
                break;
            }
        }

        void RandomYell()
        {
            Random_Text = urand(0, 6);
            switch (Random_Text) {
            case 0:
                me->MonsterYell("I hate armor.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25785, true);
                break;
            case 1:
                me->MonsterYell("I hate martyrs.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25786, true);
                break;
            case 2:
                me->MonsterYell("Feel my hatred!", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25787, true);
                break;
            case 3:
                me->MonsterYell("My hatred burns!", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25788, true);
                break;
            case 4:
                me->MonsterYell("My hate will consume you!", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25789, true);
                break;
            case 5:
                me->MonsterYell("I hate you all!", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25790, true);
                break;
            case 6:
                me->MonsterYell("I hate standing still.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25791, true);
                break;
            }
        }

        void JustDied(Unit* /*who*/)
        {
            if (instance)
                instance->SetData(DATA_ALIZABAL_EVENT, DONE);

            me->MonsterYell("I hate... every one of you...", LANG_UNIVERSAL, 0);
            me->SendPlaySound(25778, true);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (SkewerOrSeething_Timer <= diff)
            {
                RandomYell();

                if (SkewerOrSeething == 0) // Cast Skewer on tank
                {
                    Unit * target = SelectTarget(SELECT_TARGET_TOPAGGRO, 0);
                    if (target)
                        me->CastSpell(target, SKEWER, false);
                    SkewerOrSeething = 1; // Switch for the other ability for next time
                }
                else // Cast Seething Hate on random player
                {
                    Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true);
                    if (target)
                        me->CastSpell(target, SEETHING_HATE, false);
                    SkewerOrSeething = 0; // Switch for the other ability for next time
                }

                SkewerOrSeething_Counter++;

                // Set timer for the next ability
                if (SkewerOrSeething_Counter < 2)
                    SkewerOrSeething_Timer = 8000;
                else
                    SkewerOrSeething_Timer = 20000;
            }
            else SkewerOrSeething_Timer -= diff;

            if (Blade_Dance_Timer <= diff)
            {
                RandomYell();
                SkewerOrSeething = urand(0, 1);
                SkewerOrSeething_Counter = 0;
                SkewerOrSeething_Timer = 15000 + 8000; // 15s is duration of Blade Dance

                me->CastSpell(me, BLADE_DANCE_WHIRLWIND, false);
                Blade_Dance = true;
                Charge_Timer = 1000;

                Blade_Dance_Counter = 0;
                Blade_Dance_Timer = 80000;
            }
            else Blade_Dance_Timer -= diff;

            if (Blade_Dance)
            {
                if (Charge_Timer <= diff)
                {
                    Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true);
                    if (target)
                        me->CastSpell(target, BLADE_DANCE_CHARGE, false);
                    Charge_Timer = 5000;
                    Spin_Timer = 1000;
                    Blade_Dance_Counter++;

                    if (Blade_Dance_Counter == 3)
                        Blade_Dance = false;
                }
                else Charge_Timer -= diff;

                if (Spin_Timer <= diff)
                {
                    me->CastSpell(me, BLADE_DANCE_SPIN, false);
                    me->CastSpell(me, BLADE_DANCE_ROOT, false);

                    if (Blade_Dance_Counter == 3)
                        Spin_Timer = 120000;
                    else Spin_Timer = 5000;
                }
                else Spin_Timer -= diff;
            }

            if (!Berserk)
            {
                if (Berserk_Timer <= diff)
                {
                    me->CastSpell(me, BERSERK, false);
                    Berserk = true;
                }
                else Berserk_Timer -= diff;
            }

            if (Check_Timer <= diff)
            {
                // Evade when too far away from spawn position so players can`t go with her anywhere in BH
                if (me->GetDistance(me->GetHomePosition()) >= 65)
                {
                    me->AI()->EnterEvadeMode();
                    return;
                }
                Check_Timer = 2000;
            }
            else Check_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_alizabal()
{
    new boss_alizabal();
}

