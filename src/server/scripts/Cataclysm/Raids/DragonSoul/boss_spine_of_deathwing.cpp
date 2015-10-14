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

#include "ScriptPCH.h"
#include "dragonsoul.h"
#include "Spell.h"
#include "UnitAI.h"
#include "MapManager.h"
#include <stdlib.h>

// NPCs
enum NPC
{
    BOSS_SPINE_OF_DEATHWING         = 56173,
};

// Spells
enum Spells
{
    // Spells
};

// Spine of Deathwing
class boss_spine_of_deathwing : public CreatureScript
{
public:
    boss_spine_of_deathwing() : CreatureScript("boss_spine_of_deathwing") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_spine_of_deathwingAI(pCreature);
    }

    struct boss_spine_of_deathwingAI : public ScriptedAI
    {
        boss_spine_of_deathwingAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        void Reset() override
        {
            if (instance)
            {
                if (instance->GetData(TYPE_BOSS_SPINE_OF_DEATHWING) != DONE)
                    instance->SetData(TYPE_BOSS_SPINE_OF_DEATHWING, NOT_STARTED);
            }
        }

        void EnterCombat(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_SPINE_OF_DEATHWING, IN_PROGRESS);
            }
        }

        void KilledUnit(Unit* victim) override {}

        void JustDied(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_SPINE_OF_DEATHWING, DONE);
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            // Melee attack
            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_spine_of_deathwing()
{
    new boss_spine_of_deathwing();
}