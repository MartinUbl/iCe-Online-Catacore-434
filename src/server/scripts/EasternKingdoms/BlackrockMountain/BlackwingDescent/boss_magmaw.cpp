/*
 * Copyright (C) 2008-2010 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
#include "blackwing_descent.h"

enum Spells
{
    SPELL_MAGMA_SPIT = 78068,
    SPELL_LAVA_SPEW = 77690,
    SPELL_PILLAR_OF_FLAME_TRIGGER_AURA = 78006,
    SPELL_PILLAR_OF_FLAME_TRIGGERED = 78010,
    SPELL_PILLAR_OF_FLAME_THROW = 77971,
    SPELL_MANGLE = 89773,

    // Lava Parasite
    SPELL_PARASITIC_INFECTION_1 = 78941,
    SPELL_PARASITIC_INFECTION_2 = 91913,
    SPELL_PARASITIC_INFECTION_3 = 94678,
    SPELL_PARASITIC_INFECTION_4 = 94679,
    // TODO: on unapply spawn more small shits
};

enum NPCs
{
    NPC_MAGMAW = 41570,
    NPC_PILLAR_OF_FLAME = 41843,
    NPC_LAVA_PARASITE = 42321,
}

class boss_magmaw: public CreatureScript
{
public:
    boss_magmaw(): CreatureScript("boss_magmaw") {};

    struct boss_magmawAI: public Scripted_NoMovementAI
    {
        boss_magmawAI(Creature* c): Scripted_NoMovementAI(c)
        {
            me->SetUnitMovementFlags(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING);
            Reset();
        }

        void Reset()
        {
            // Just for being sure that we arrive at homepoint even visual
            me->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new boss_magmawAI(c);
    }
};

void AddSC_magmaw()
{
    new boss_magmaw();
}

/**** SQL:

UPDATE creature_template SET ScriptName='boss_magmaw' WHERE entry=41570;
REPLACE INTO `creature_model_info` (`modelid`, `bounding_radius`, `combat_reach`, `gender`, `modelid_other_gender`) VALUES (32679, 8, 0, 2, 0);

*/
