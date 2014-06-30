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
#include "well_of_eternity.h"

# define NEVER  (0xffffffff)

const Position leftEndPositions[2] =
{
    {3260.16f,-4943.4f, 181.7f, 3.7f},     //LEFT
    {3257.5f,-4938.6f, 181.7f, 3.7f},     //RIGHT
};

const Position rightEndPositions[2] =
{
    {3411.3f,-4841.3f, 181.7f, 0.65f},     //LEFT
    {3413.7f,-4845.3f, 181.7f, 0.65f},     //RIGHT
};

class Legion_demon_WoE : public CreatureScript
{
public:
    Legion_demon_WoE() : CreatureScript("legion_demon_WoE") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new Legion_demon_WoEAI(creature);
    }

    struct Legion_demon_WoEAI : public ScriptedAI
    {
        Legion_demon_WoEAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = me->GetInstanceScript();
            canMoveToNextPoint = false;
        }

        InstanceScript * pInstance;
        DemonDirection direction;
        DemonWave waveNumber;
        bool canMoveToNextPoint;

        uint32 waitTimer;

        void SetData(uint32 type, uint32 data)
        {
            if (type == DEMON_DATA_DIRECTION)
            {
                direction = (DemonDirection)data;
            }
            else if (type == DEMON_DATA_WAVE)
            {
                waveNumber = (DemonWave)data;
            }
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (!pInstance || type != POINT_MOTION_TYPE)
                return;

            if (id == WP_END)
            {
                me->ForcedDespawn();
                return;
            }

            switch (direction)
            {
                case DIRECTION_LEFT: // Going to Left Corner
                {
                    if (waveNumber == WAVE_ONE)
                        waitTimer = 1900;
                    else if (waveNumber == WAVE_TWO)
                        canMoveToNextPoint = true;

                    break;
                }
                case DIRECTION_RIGHT: // Going to Right Corner
                {
                    if (waveNumber == WAVE_ONE)
                        waitTimer = 1900;
                    else if (waveNumber == WAVE_TWO)
                        canMoveToNextPoint = true;
                    break;
                }
                case DIRECTION_STRAIGHT: // Just go straight till end
                {
                    break;
                }
            }
        }

        void Reset()
        {
            me->SetReactState(REACT_DEFENSIVE); // Temporary
            waitTimer = NEVER;
        }

        void UpdateAI(const uint32 diff)
        {
            if (canMoveToNextPoint)
            {
                canMoveToNextPoint = false;

                if (direction == DIRECTION_LEFT)
                    me->GetMotionMaster()->MovePoint(WP_END, leftEndPositions[DIRECTION_RIGHT]);
                else
                    me->GetMotionMaster()->MovePoint(WP_END, rightEndPositions[DIRECTION_RIGHT]);
            }

            if (waitTimer <= diff)
            {
                waitTimer = NEVER;

                if (direction == DIRECTION_STRAIGHT || waveNumber == WAVE_THREE)
                    return;

                if (direction == DIRECTION_LEFT)
                    me->GetMotionMaster()->MovePoint(WP_END, leftEndPositions[DIRECTION_LEFT]);
                else
                    me->GetMotionMaster()->MovePoint(WP_END, rightEndPositions[DIRECTION_LEFT]);
            }
            else waitTimer -= diff;


            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

enum TrashSpells
{
    SPELL_DEMON_GRIP = 105720,
};

class Guardian_Demon_WoE : public CreatureScript
{
public:
    Guardian_Demon_WoE() : CreatureScript("guardian_demon_WoE") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new Guardian_Demon_WoEAI(creature);
    }

    struct Guardian_Demon_WoEAI : public ScriptedAI
    {
        Guardian_Demon_WoEAI(Creature* creature) : ScriptedAI(creature)
        {
            me->AddUnitMovementFlag(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING | MOVEMENTFLAG_LEVITATING);
        }

        uint32 gripTimer;

        void Reset()
        {
            gripTimer = 1000;
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (gripTimer <= diff)
            {
                if (me->getVictim())
                    me->CastSpell(me->getVictim(), SPELL_DEMON_GRIP, true);
                gripTimer = 2000;
            }
            else gripTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

// BASIC TEMPLATE FOR CREATURE SCRIPTS
/*class Legion_demon_WoE : public CreatureScript
{
public:
    Legion_demon_WoE() : CreatureScript("Legion_demon_WoE") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new Legion_demon_WoEAI(creature);
    }

    struct Legion_demon_WoEAI : public ScriptedAI
    {
        Legion_demon_WoEAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = me->GetInstanceScript();
        }

        InstanceScript * pInstance;

        void Reset()
        {
        }

        void UpdateAI(const uint32 diff)
        {
        }
    };
};*/

void AddSC_well_of_eternity_trash()
{
    new Legion_demon_WoE(); // 54500
    new Guardian_Demon_WoE(); // 54927
}