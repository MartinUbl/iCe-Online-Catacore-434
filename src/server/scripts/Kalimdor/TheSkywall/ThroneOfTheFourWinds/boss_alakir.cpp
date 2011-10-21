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
#include "throne_of_the_four_winds.h"

enum Stuff
{
    // phase 1
    SPELL_LIGHTNING_STRIKE_DAMAGE = 88214,
    SPELL_LIGHTNING_STRIKE_DAMAGE_10H = 93255,
    SPELL_LIGHTNING_STRIKE_DAMAGE_25N = 93256,
    SPELL_LIGHTNING_STRIKE_DAMAGE_25H = 93257,
    SPELL_LIGHTNING_STRIKE_VISUAL = 88230,
    SPELL_STATIC_SHOCK            = 87873,
    SPELL_ICE_STORM_GROUND_AURA   = 87469,
    SPELL_ICE_STORM_TRIGGER       = 87472,
    SPELL_ICE_STORM_SUMMON        = 87055,
    SPELL_SQUALL_LINE_VISUAL      = 87621,
    SPELL_SQUALL_LINE_VEHICLE     = 87856,
    SPELL_WIND_BURST              = 87770,
    // phase 2
    SPELL_ACID_RAIN_TRIGGER       = 88290,
    SPELL_ACID_RAIN_DOT           = 88301,
    NPC_STORMLING                 = 47175,
    SPELL_FEEDBACK                = 87904, // casted after stormling death
    // phase 3
    SPELL_EYE_OF_THE_STORM        = 82724,
    SPELL_RELENTLESS_STORM_VEHICLE= 89104,
    SPELL_RELENTLESS_STORM_VISUAL = 88866, // visual effect, is this used in encounter?
    NPC_RELENTLESS_STORM          = 47807, // used for lighning?
    SPELL_LIGHTNING_ROD           = 89667,
    // lightning clouds?
};

#define SL_DIST_FIRST 29.0f
#define SL_DIST_NEXT  6.2f
#define SL_COUNT 7
#define SL_RAD_PER_STEP (M_PI/32.0f)

static uint16 vortexready[SL_COUNT];
float furthest_omega;

class boss_alakir: public CreatureScript
{
    public:
        boss_alakir(): CreatureScript("boss_alakir") {}

        struct boss_alakirAI: public Scripted_NoMovementAI
        {
            boss_alakirAI(Creature* c): Scripted_NoMovementAI(c)
            {
                Reset();
            }

            uint32 squallLineTimer;
            uint32 iceStormTimer;

            void Reset()
            {
                iceStormTimer = 5000;
                squallLineTimer = 15000;
                for (uint32 i = 0; i < SL_COUNT; i++)
                    vortexready[i] = 0;
            }

            void SpellHitTarget(Unit* pTarget, const SpellEntry* spell)
            {
                if (!spell || !pTarget)
                    return;

                // Cast visual when target hit by Lightning Strike
                if (spell->Id == SPELL_LIGHTNING_STRIKE_DAMAGE || spell->Id == SPELL_LIGHTNING_STRIKE_DAMAGE_10H
                    || spell->Id == SPELL_LIGHTNING_STRIKE_DAMAGE_25N || spell->Id == SPELL_LIGHTNING_STRIKE_DAMAGE_25H)
                    me->CastSpell(pTarget, 88230, true);
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (squallLineTimer <= diff)
                {
                    squallLineTimer = 40000 + 20000; // 40s duration, 20s delay

                    // 0.5f will be the speed of furthest squall
                    furthest_omega = 0.5f*(SL_DIST_FIRST+SL_DIST_NEXT * (SL_COUNT-1));

                    Creature* pTemp = NULL;
                    float mx,my,mz;
                    uint32 mstep = urand(0,(2*M_PI)/(SL_RAD_PER_STEP));
                    uint32 skip = urand(0,SL_COUNT-1);
                    for (uint32 i = 0; i < SL_COUNT; i++)
                    {
                        if (i == skip)
                            continue;

                        mx = me->GetPositionX()+(SL_DIST_FIRST+SL_DIST_NEXT * i) * cos(mstep * SL_RAD_PER_STEP);
                        my = me->GetPositionY()+(SL_DIST_FIRST+SL_DIST_NEXT * i) * sin(mstep * SL_RAD_PER_STEP);
                        mz = me->GetPositionZ();
                        pTemp = me->SummonCreature(48855, mx,my,mz,0,TEMPSUMMON_MANUAL_DESPAWN,0);
                        if (!pTemp)
                            continue;

                        pTemp->SetSpeed(MOVE_RUN, furthest_omega/(SL_DIST_FIRST+SL_DIST_NEXT * (SL_COUNT-i)), true);

                        mx = me->GetPositionX()+(SL_DIST_FIRST+SL_DIST_NEXT*i)*cos((mstep+1)*SL_RAD_PER_STEP);
                        my = me->GetPositionY()+(SL_DIST_FIRST+SL_DIST_NEXT*i)*sin((mstep+1)*SL_RAD_PER_STEP);

                        // Define actual point as 1000*position+step
                        pTemp->GetMotionMaster()->MovePoint(mstep+1000*i, mx, my, mz);
                    }
                } else squallLineTimer -= diff;

                if (iceStormTimer <= diff)
                {
                    Unit* pTargetOne = SelectUnit(SELECT_TARGET_RANDOM, 0);
                    Unit* pTargetTwo = SelectUnit(SELECT_TARGET_RANDOM, 0);
                    if (pTargetOne && pTargetTwo)
                    {
                        Creature* pStorm = me->SummonCreature(46973, pTargetOne->GetPositionX(), pTargetOne->GetPositionY(), pTargetOne->GetPositionZ());
                        if (pStorm)
                        {
                            pStorm->AddThreat(pTargetOne, 10000.0f);
                            pStorm->AddThreat(pTargetTwo, 50000.0f);
                            pStorm->Attack(pTargetTwo, true);
                            pStorm->GetMotionMaster()->MoveChase(pTargetTwo);
                        }
                    }
                    iceStormTimer = 25000;
                } else iceStormTimer -= diff;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new boss_alakirAI(c);
        }
};

class npc_ice_storm: public CreatureScript
{
    public:
        npc_ice_storm(): CreatureScript("npc_ice_storm") {}

        struct npc_ice_stormAI: public ScriptedAI
        {
            npc_ice_stormAI(Creature* c): ScriptedAI(c)
            {
                Reset();
            }

            uint32 disappearTimer;
            uint32 dropTimer;

            void Reset()
            {
                me->CastSpell(me, SPELL_ICE_STORM_TRIGGER, true);
                me->SetSpeed(MOVE_RUN, 0.5f, true);
                disappearTimer = 20000;
                dropTimer = 2000;
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (dropTimer <= diff)
                {
                    me->SummonCreature(46766, 0, 0, 0);
                    dropTimer = 2000;
                } else dropTimer -= diff;

                if (disappearTimer <= diff)
                {
                    disappearTimer = 10000;
                    me->Kill(me, false);
                    me->ForcedDespawn();
                } else disappearTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_ice_stormAI(c);
        }
};

class npc_ice_storm_ground: public CreatureScript
{
    public:
        npc_ice_storm_ground(): CreatureScript("npc_ice_storm_ground") {}

        struct npc_ice_storm_groundAI: public Scripted_NoMovementAI
        {
            npc_ice_storm_groundAI(Creature* c): Scripted_NoMovementAI(c)
            {
                Reset();
            }

            uint32 disappearTimer;

            void Reset()
            {
                me->CastSpell(me, SPELL_ICE_STORM_GROUND_AURA, false);
                disappearTimer = 20000;
            }

            void UpdateAI(const uint32 diff)
            {
                if (!me->HasAura(SPELL_ICE_STORM_GROUND_AURA) && !me->IsNonMeleeSpellCasted(true))
                    me->CastSpell(me, SPELL_ICE_STORM_GROUND_AURA, false);

                if (disappearTimer <= diff)
                {
                    disappearTimer = 10000;
                    me->Kill(me, false);
                    me->ForcedDespawn();
                } else disappearTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_ice_storm_groundAI(c);
        }
};

class npc_squall_vortex: public CreatureScript
{
    public:
        npc_squall_vortex(): CreatureScript("npc_squall_vortex") {}

        struct npc_squall_vortexAI: public ScriptedAI
        {
            npc_squall_vortexAI(Creature* c): ScriptedAI(c)
            {
                Reset();
            }

            uint32 despawnTimer;
            Player* pPassenger;
            Unit* pVehicle;

            float nextx,nexty,nextz,nextpoint;
            uint8 nextpos;
            bool needstomove;

            void Reset()
            {
                me->CastSpell(me, SPELL_SQUALL_LINE_VISUAL, true);
                despawnTimer = 40000;
                pPassenger = NULL;
                pVehicle = NULL;
                needstomove = false;
            }

            void MoveInLineOfSight(Unit* pWho)
            {
                if (pPassenger || !pWho || pWho->GetVehicle() || pWho->GetTypeId() != TYPEID_PLAYER || pWho->isDead() || pWho->GetDistance2d(me) > 5.0f)
                    return;

                pPassenger = pWho->ToPlayer();
                pVehicle = me->SummonCreature(48852, 0, 0, 0);
                pVehicle->SetDisplayId(me->GetDisplayId());
                Vehicle* pVeh = pVehicle->GetVehicleKit();
                if (pVeh)
                {
                    pPassenger->EnterVehicle(pVehicle);
                    pVehicle->GetMotionMaster()->MoveFollow(me, 0.0f, 0.0f);
                    pVehicle->SetSpeed(MOVE_RUN, me->GetSpeed(MOVE_RUN), true);
                    pVehicle->CastSpell(pPassenger, SPELL_SQUALL_LINE_VEHICLE, true);
                }
            }

            void MovementInform(uint32 type, uint32 id)
            {
                // Get actual position
                uint32 mypos = id/1000;
                uint32 mypoint = id%1000;

                float mx,my,mz;
                Creature* pBoss = GetClosestCreatureWithEntry(me, 46753, 500.0f);
                if (!pBoss)
                    return;

                // And do the trick
                mypoint += 1;

                mx = pBoss->GetPositionX()+(SL_DIST_FIRST+SL_DIST_NEXT*mypos)*cos(mypoint*SL_RAD_PER_STEP);
                my = pBoss->GetPositionY()+(SL_DIST_FIRST+SL_DIST_NEXT*mypos)*sin(mypoint*SL_RAD_PER_STEP);
                mz = pBoss->GetPositionZ();

                nextx = mx;
                nexty = my;
                nextz = mz;
                nextpos = mypos;
                nextpoint = mypoint;
                needstomove = true;
                vortexready[mypos] = mypoint;
            }

            void UpdateAI(const uint32 diff)
            {
                if (despawnTimer <= diff)
                {
                    me->Kill(me, false);
                    me->ForcedDespawn();
                    if (pVehicle)
                    {
                        pVehicle->Kill(pVehicle, false);
                        if (pVehicle->ToCreature())
                            pVehicle->ToCreature()->ForcedDespawn();
                    }
                    // For being sure when doing multithreading
                    // should not happen, but who knows
                    despawnTimer = 10000;
                }
                else despawnTimer -= diff;

                if (needstomove)
                {
                    uint32 sum = 0;
                    for (uint32 i = 0; i < SL_COUNT; i++)
                        sum += vortexready[i];

                    if (sum >= nextpoint*(SL_COUNT-1))
                    {
                        me->GetMotionMaster()->MovePoint(nextpos*1000+nextpoint, nextx, nexty, nextz);
                        if (pVehicle)
                            pVehicle->GetMotionMaster()->MovePoint(nextpos*1000+nextpoint, nextx, nexty, nextz);
                        needstomove = false;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_squall_vortexAI(c);
        }
};

void AddSC_alakir()
{
    new boss_alakir();
    new npc_ice_storm();
    new npc_ice_storm_ground();
    new npc_squall_vortex();
}

/*
SQL:

UPDATE creature_template SET vehicleId=116,AIName='NullAI' WHERE entry=48852;
UPDATE creature_template SET vehicleId=0,ScriptName='npc_squall_vortex' WHERE entry=48855;
UPDATE creature_template SET ScriptName='npc_ice_storm' WHERE entry=46973;
UPDATE creature_template SET ScriptName='npc_ice_storm_ground' WHERE entry=46766;
*/
