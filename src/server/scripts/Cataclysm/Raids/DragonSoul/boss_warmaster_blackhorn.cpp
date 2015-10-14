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
    BOSS_WARMASTER_BLACKHORN           = 56427,
};

// Spells
enum Spells
{
    // Spells
};

// Warmaster Blackhorn
class boss_warmaster_blackhorn : public CreatureScript
{
public:
    boss_warmaster_blackhorn() : CreatureScript("boss_warmaster_blackhorn") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_warmaster_blackhornAI(pCreature);
    }

    struct boss_warmaster_blackhornAI : public ScriptedAI
    {
        boss_warmaster_blackhornAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        void Reset() override
        {
            if (instance)
            {
                if (instance->GetData(TYPE_BOSS_BLACKHORN) != DONE)
                    instance->SetData(TYPE_BOSS_BLACKHORN, NOT_STARTED);
            }
        }

        void EnterCombat(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_BLACKHORN, IN_PROGRESS);
            }
        }

        void KilledUnit(Unit* victim) override {}

        void JustDied(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_BLACKHORN, DONE);
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

void AddSC_boss_warmaster_blackhorn()
{
    new boss_warmaster_blackhorn();
}