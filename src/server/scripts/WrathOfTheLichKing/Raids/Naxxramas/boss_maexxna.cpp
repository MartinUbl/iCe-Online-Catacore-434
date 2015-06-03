/*
 * Copyright (C) 2008-2010 TrinityCore <http://www.trinitycore.org/>
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
#include "naxxramas.h"

enum Spells
{
    SPELL_WEB_WRAP              = 28622,
    SPELL_WEB_SPRAY_10          = 29484,
    SPELL_WEB_SPRAY_25          = 54125,
    SPELL_POISON_SHOCK_10       = 28741,
    SPELL_POISON_SHOCK_25       = 54122,
    SPELL_NECROTIC_POISON_10    = 28776,
    SPELL_NECROTIC_POISON_25    = 54121,
    SPELL_FRENZY_10             = 54123,
    SPELL_FRENZY_25             = 54124,
};

enum Creatures
{
    MOB_WEB_WRAP                = 16486,
    MOB_SPIDERLING              = 17055,
};

#define MAX_POS_WRAP            8
const Position PosWrap[MAX_POS_WRAP] =
{
	{3511.761f,-3836.391f,303.194f,0.0f},
	{3535.413f,-3847.061f,299.640f,0.0f},
	{3553.769f,-3872.762f,296.839f,0.0f},
	{3555.495f,-3907.812f,300.233f,0.0f},
	{3530.582f,-3940.827f,308.648f,0.0f},
	{3496.280f,-3947.832f,307.354f,0.0f},
	{3473.120f,-3941.538f,305.355f,0.0f},
	{3448.745f,-3912.884f,305.297f,0.0f},
};

enum Events
{
    EVENT_NONE,
    EVENT_SPRAY,
    EVENT_SHOCK,
    EVENT_POISON,
    EVENT_WRAP,
    EVENT_SUMMON,
    EVENT_FRENZY,
};

class boss_maexxna : public CreatureScript
{
public:
    boss_maexxna() : CreatureScript("boss_maexxna") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_maexxnaAI (pCreature);
    }

    struct boss_maexxnaAI : public BossAI
    {
        boss_maexxnaAI(Creature *c) : BossAI(c, BOSS_MAEXXNA) {}

        bool enraged;

        void EnterCombat(Unit * /*who*/)
        {
            _EnterCombat();
            enraged = false;
            events.ScheduleEvent(EVENT_WRAP, 25000);
            events.ScheduleEvent(EVENT_SPRAY, 45000);
            events.ScheduleEvent(EVENT_SHOCK, urand(5000,10000));
            events.ScheduleEvent(EVENT_POISON, urand(10000,15000));
            events.ScheduleEvent(EVENT_SUMMON, 15000);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim() || !CheckInRoom())
                return;

            if (!enraged && HealthBelowPct(30))
            {
                enraged = true;
                events.ScheduleEvent(EVENT_FRENZY, 0); // will be cast immediately
            }

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case EVENT_WRAP:
                        // TODO : Add missing text
                        for (uint8 i = 0; i < RAID_MODE(1,4); ++i)
                        {
                            if (Unit *pTarget = SelectTarget(SELECT_TARGET_RANDOM, 1, 0, true, -SPELL_WEB_WRAP))
                            {
								if(pTarget != me->GetVictim())
								{
									pTarget->RemoveAura(RAID_MODE(SPELL_WEB_SPRAY_10,SPELL_WEB_SPRAY_25));
									uint8 pos = rand()%MAX_POS_WRAP;
									pTarget->GetMotionMaster()->MoveJump(PosWrap[pos].GetPositionX(), PosWrap[pos].GetPositionY(), PosWrap[pos].GetPositionZ(), 20, 20);
									if (Creature *wrap = DoSummon(MOB_WEB_WRAP, PosWrap[pos], 0, TEMPSUMMON_CORPSE_DESPAWN))
										wrap->AI()->SetGUID(pTarget->GetGUID());
								}
								else
								{
									if (Unit *pTarget = SelectTarget(SELECT_TARGET_RANDOM, 1, 0, true, -SPELL_WEB_WRAP))
									{
										if(pTarget != me->GetVictim())
										{
											pTarget->RemoveAura(RAID_MODE(SPELL_WEB_SPRAY_10,SPELL_WEB_SPRAY_25));
											uint8 pos = rand()%MAX_POS_WRAP;
											pTarget->GetMotionMaster()->MoveJump(PosWrap[pos].GetPositionX(), PosWrap[pos].GetPositionY(), PosWrap[pos].GetPositionZ(), 20, 20);
											if (Creature *wrap = DoSummon(MOB_WEB_WRAP, PosWrap[pos], 0, TEMPSUMMON_CORPSE_DESPAWN))
												wrap->AI()->SetGUID(pTarget->GetGUID());
										}
									}
								}
                            }
                        }
                        events.ScheduleEvent(EVENT_WRAP, 45000);
                        break;
                    case EVENT_SPRAY:
                        DoCastAOE(RAID_MODE(SPELL_WEB_SPRAY_10,SPELL_WEB_SPRAY_25));
                        events.ScheduleEvent(EVENT_SPRAY, 30000);
                        break;
                    case EVENT_SHOCK:
                        DoCastAOE(RAID_MODE(SPELL_POISON_SHOCK_10,SPELL_POISON_SHOCK_25));
                        events.ScheduleEvent(EVENT_SHOCK, urand(8000,10000));
                        break;
                    case EVENT_POISON:
                        DoCast(me->GetVictim(), RAID_MODE(SPELL_NECROTIC_POISON_10,SPELL_NECROTIC_POISON_25));
                        events.ScheduleEvent(EVENT_POISON, urand(7000, 10000));
                        break;
                    case EVENT_FRENZY:
                        DoCast(me, RAID_MODE(SPELL_FRENZY_10,SPELL_FRENZY_25), true);
                        events.ScheduleEvent(EVENT_FRENZY, 600000);
                        break;
                    case EVENT_SUMMON:
                        // TODO : Add missing text
                        /*uint8 amount = urand(8,10);
                        for (uint8 i = 0; i < amount; ++i)
                            DoSummon(MOB_SPIDERLING, me, 0, TEMPSUMMON_CORPSE_DESPAWN);*/
						uint32 SpidersCount = urand(6,8);
						for(uint32 i = 0; i < SpidersCount; i++)
						{
							float x,y,z;
							switch(urand(0,6))
							{
							case 0:
								x = 3476.03f;
								y = -3847.177f;
								z = 304.81f;
								break;
							case 1:
								x = 3503.76f;
								y = -3838.37f;
								z = 303.11f;
								break;
							case 2:
								x = 3551.87f;
								y = -3874.447f;
								z = 296.45f;
								break;
							case 3:
								x = 3543.071f;
								y = -3919.83f;
								z = 302.241f;
								break;
							case 4:
								x = 3496.335f;
								y = -3941.557f;
								z = 304.48f;
								break;
							case 5:
								x = 3459.08f;
								y = -3925.74f;
								z = 303.083f;
								break;
							case 6:
							default:
								x = 3449.588f;
								y = -3878.03f;
								z = 305.055f;
								break;
							}
							Creature* pSpider = me->SummonCreature(17055,x,y,z,0.0f,TEMPSUMMON_DEAD_DESPAWN,0);
							if(pSpider)
							{
								if(Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM,0))
									pSpider->AI()->AttackStart(pTarget);
								//Paradicka
								pSpider->CastSpell(pSpider,37846,true);
								pSpider->SetSpeed(MOVE_RUN,0.65f,true);
							}
						}
                        events.ScheduleEvent(EVENT_SUMMON, 45000);
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };

};

class mob_webwrap : public CreatureScript
{
public:
    mob_webwrap() : CreatureScript("mob_webwrap") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_webwrapAI (pCreature);
    }

    struct mob_webwrapAI : public NullCreatureAI
    {
        mob_webwrapAI(Creature *c) : NullCreatureAI(c), victimGUID(0) {}

        uint64 victimGUID;

        void SetGUID(const uint64 &guid, int32 /*param*/)
        {
            victimGUID = guid;
            if (me->m_spells[0] && victimGUID)
                if (Unit *victim = Unit::GetUnit(*me, victimGUID))
                    victim->CastSpell(victim, me->m_spells[0], true, NULL, NULL, me->GetGUID());
        }

        void JustDied(Unit * /*killer*/)
        {
            if (me->m_spells[0] && victimGUID)
                if (Unit *victim = Unit::GetUnit(*me, victimGUID))
                    victim->RemoveAurasDueToSpell(me->m_spells[0], me->GetGUID());
        }
    };

};



void AddSC_boss_maexxna()
{
    new boss_maexxna();
    new mob_webwrap();
}
