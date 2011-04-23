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

#define SPELL_MORTAL_WOUND      25646
#define SPELL_ENRAGE            RAID_MODE(28371,54427)
#define SPELL_DECIMATE          RAID_MODE(28374,54426)
#define SPELL_BERSERK           26662
#define SPELL_INFECTED_WOUND    29306

#define SPELL_INTIMIDATING_ROAR 51467
#define SPELL_DEMORALIZING_ROAR 16244
#define SPELL_KNOCK_AWAY        64020

#define MOB_ZOMBIE  16360

const Position PosSummon[3] =
{
    {3267.9f, -3172.1f, 297.42f, 0.94f},
    {3253.2f, -3132.3f, 297.42f, 0},
    {3308.3f, -3185.8f, 297.42f, 1.58f},
};

enum Events
{
    EVENT_NONE,
    EVENT_WOUND,
    EVENT_ENRAGE,
    EVENT_DECIMATE,
    EVENT_BERSERK,
    EVENT_SUMMON,
	EVENT_ROAR1,
	EVENT_ROAR2,
	EVENT_KNOCKBACK,
};

#define EMOTE_NEARBY "Gluth spots a nearby zombie to devour!"

class boss_gluth : public CreatureScript
{
public:
    boss_gluth() : CreatureScript("boss_gluth") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_gluthAI (pCreature);
    }

    struct boss_gluthAI : public BossAI
    {
        boss_gluthAI(Creature *c) : BossAI(c, BOSS_GLUTH)
        {
            // Do not let Gluth be affected by zombies' debuff
            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_INFECTED_WOUND, true);
        }

        void MoveInLineOfSight(Unit *who)
        {
            if (who->GetEntry() == MOB_ZOMBIE && me->IsWithinDistInMap(who, 7))
            {
                SetGazeOn(who);
                // TODO: use a script text
                me->MonsterTextEmote(EMOTE_NEARBY, 0, true);
				who->AddThreat(me,10000000.0f);
            }
            else
                BossAI::MoveInLineOfSight(who);
        }

        void EnterCombat(Unit * /*who*/)
        {
            _EnterCombat();
            events.ScheduleEvent(EVENT_WOUND, 10000);
            events.ScheduleEvent(EVENT_ENRAGE, 15000);
            events.ScheduleEvent(EVENT_DECIMATE, 85000);
            events.ScheduleEvent(EVENT_BERSERK, 8*60000);
            events.ScheduleEvent(EVENT_SUMMON, 15000);
			events.ScheduleEvent(EVENT_ROAR1, 40000);
			events.ScheduleEvent(EVENT_ROAR2, 30000);
			events.ScheduleEvent(EVENT_KNOCKBACK, 18000);
        }

        void JustSummoned(Creature *summon)
        {
            if (summon->GetEntry() == MOB_ZOMBIE)
			{
                summon->AI()->AttackStart(me);
				summon->SetSpeed(MOVE_RUN,0.85f);
			}
            summons.Summon(summon);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictimWithGaze() || !CheckInRoom())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case EVENT_WOUND:
                        DoCast(me->getVictim(), SPELL_MORTAL_WOUND);
                        events.ScheduleEvent(EVENT_WOUND, 12000);
                        break;
                    case EVENT_ENRAGE:
                        // TODO : Add missing text
                        DoCast(me, SPELL_ENRAGE);
                        events.ScheduleEvent(EVENT_ENRAGE, 20000);
                        break;
                    case EVENT_DECIMATE:
						{
                        // TODO : Add missing text
                        DoCastAOE(SPELL_DECIMATE);
						std::list<Creature*> ZombieList;
						GetCreatureListWithEntryInGrid(ZombieList,me,MOB_ZOMBIE,200.0f);
						if(!ZombieList.empty())
						{
							for(std::list<Creature*>::iterator itr = ZombieList.begin(); itr != ZombieList.end(); ++itr)
							{
								if((*itr) && (*itr)->isAlive())
									(*itr)->DealDamage((*itr),(*itr)->GetHealth()-(*itr)->CountPctFromMaxHealth(5));
							}
						}
                        events.ScheduleEvent(EVENT_DECIMATE, 85000);
                        break;
						}
                    case EVENT_BERSERK:
                        DoCast(me, SPELL_BERSERK);
                        events.ScheduleEvent(EVENT_BERSERK, 5*60000);
                        break;
					case EVENT_ROAR1:
						DoCastAOE(SPELL_INTIMIDATING_ROAR);
						events.ScheduleEvent(EVENT_ROAR1,45000);
						break;
					case EVENT_ROAR2:
						DoCastAOE(SPELL_DEMORALIZING_ROAR);
						events.ScheduleEvent(EVENT_ROAR2,35000);
						break;
					case EVENT_KNOCKBACK:
						DoCastAOE(SPELL_KNOCK_AWAY);
						events.ScheduleEvent(EVENT_KNOCKBACK,25000);
						break;
                    case EVENT_SUMMON:
                        for (int32 i = 0; i < RAID_MODE(1, 2); ++i)
                            DoSummon(MOB_ZOMBIE, PosSummon[rand() % RAID_MODE(1,3)]);
                        events.ScheduleEvent(EVENT_SUMMON, 10000);
                        break;
                }
            }

            if (me->getVictim() && me->getVictim()->GetEntry() == MOB_ZOMBIE)
            {
                if (me->IsWithinMeleeRange(me->getVictim()))
                {
                    me->Kill(me->getVictim());
                    me->ModifyHealth(int32(me->CountPctFromMaxHealth(5)));
                }
            }
            else
                DoMeleeAttackIfReady();
        }
    };

};


void AddSC_boss_gluth()
{
    new boss_gluth();
}
