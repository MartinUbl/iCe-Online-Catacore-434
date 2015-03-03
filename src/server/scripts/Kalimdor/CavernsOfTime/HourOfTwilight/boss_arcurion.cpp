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

/*
TO DO:
Boss Health: 4,979,625
*/

#include "ScriptPCH.h"
#include "Spell.h"
#include "UnitAI.h"
#include "MapManager.h"
#include "hour_of_twilight.h"

enum NPC
{
    BOSS_ARCURION         = 54590,
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

    // Thrall
    BLOODLUST              = 103834, //Bloodlust
    MOLTEN_FURY            = 103905, // Molten Fury
    LAVA_BURST             = 103923, // Lava Burst
    LAVA_BURST_1           = 102475, // Lava Burst
    LAVA_BARRAGE           = 104540, // Lava Barrage
    GHOST_WOLF             = 2645,   // Ghost Wolf
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
        }

        InstanceScript* instance;
        int Random_Kill_Text;

        void Reset() 
        {
            if (instance)
            {
                if(instance->GetData(TYPE_BOSS_ARCURION)!=DONE)
                    instance->SetData(TYPE_BOSS_ARCURION, NOT_STARTED);
            }
        }

        void InEvadeMode() { }

        void EnterCombat(Unit * /*who*/)
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_ARCURION, IN_PROGRESS);
            }

            me->MonsterYell("Suffer for your arrogance!", LANG_UNIVERSAL, 0);
            me->SendPlaySound(25914, true);
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
                instance->SetData(TYPE_BOSS_ARCURION, DONE);
            }

            me->MonsterYell("Suffer for your arrogance!", LANG_UNIVERSAL, 0);
            me->SendPlaySound(25914, true);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_arcurion()
    {
        new boss_arcurion();
    }