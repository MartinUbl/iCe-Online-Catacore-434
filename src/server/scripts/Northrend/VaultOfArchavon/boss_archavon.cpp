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
#include "vault_of_archavon.h"

#define EMOTE_BERSERK           -1590002

//Spells Archavon
#define SPELL_ROCK_SHARDS        58678
#define SPELL_CRUSHING_LEAP      RAID_MODE(58960,60894)//Instant (10-80yr range) -- Leaps at an enemy, inflicting 8000 Physical damage, knocking all nearby enemies away, and creating a cloud of choking debris.
#define SPELL_STOMP              RAID_MODE(58663,60880)
#define SPELL_IMPALE             RAID_MODE(58666,60882) //Lifts an enemy off the ground with a spiked fist, inflicting 47125 to 52875 Physical damage and 9425 to 10575 additional damage each second for 8 sec.
#define SPELL_BERSERK            47008
//Spells Archavon Warders
#define SPELL_ROCK_SHOWER        RAID_MODE(60919,60923)
#define SPELL_SHIELD_CRUSH       RAID_MODE(60897,60899)
#define SPELL_WHIRL              RAID_MODE(60902,60916)

//4 Warders spawned
#define ARCHAVON_WARDER          32353 //npc 32353


enum Events
{
    // Archavon

    //mob
    EVENT_ROCK_SHOWER       = 6,    // set = 20s cd,unkown cd
    EVENT_SHIELD_CRUSH      = 7,    // set = 30s cd
    EVENT_WHIRL             = 8,    // set= 10s cd
};

class boss_archavon : public CreatureScript
{
    public:
        boss_archavon() : CreatureScript("boss_archavon") { }

		struct boss_archavonAI : public BossAI
		{
			boss_archavonAI(Creature* creature) : BossAI(creature, DATA_ARCHAVON), vehicle(creature->GetVehicleKit())
			{
				assert(vehicle);
			//	me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true); // blocks spell Crushing Leap
				me->ApplySpellImmune(0, IMMUNITY_ID, 49560, true); // Death Grip jump effect
			}

			Vehicle* vehicle;
			uint32 m_uiRockShards_Timer;
			uint32 m_uiCrushingLeapJump_Timer;
			uint32 m_uiCrushingLeapSpell_Timer;
			uint32 m_uiStomp_Timer;
			uint32 m_uiImpale_Timer;
			uint32 m_uiDropPlayer_Timer;
			uint32 m_uiBerserk_Timer;
			bool m_bCrushingLeapSpell_Countdown;
			bool m_bImpale_Countdown;
			bool m_bDropPlayer_Countdown;
			bool m_bJumpOne;
			bool m_bBerserk;
			uint64 m_uiImpaledGUID;

			void Reset()
			{
				m_uiRockShards_Timer = 14000;
				m_uiCrushingLeapJump_Timer = 25000;
				m_uiCrushingLeapSpell_Timer = 1200;
				m_uiStomp_Timer = 50000;
				m_uiImpale_Timer = 1000;
				m_uiDropPlayer_Timer = 8000;
				m_uiBerserk_Timer = 300000;
				m_bCrushingLeapSpell_Countdown = false;
				m_bImpale_Countdown = false;
				m_bJumpOne = true;
				m_bBerserk = false;

				_Reset();
			}

			void JustDied(Unit* /*pKiller*/)
			{
				if(instance)
					instance->DoCompleteAchievement(RAID_MODE(1722,1721));

				_JustDied();
			}

			void EnterCombat(Unit * /*pWho*/)
            {
				m_bDropPlayer_Countdown = false;
				m_uiImpaledGUID = 0; // must not be in Reset() since it is called from EnterEvadeMode()
				_EnterCombat();
			}

			void DamageTaken(Unit* /*pDoneBy*/, uint32 &damage)
			{
				if(damage >= me->GetHealth())
					DropPlayer();
			}

			void JustReachedHome()
			{
				_JustReachedHome();
			}

			Unit* GetLeapTarget()
			{
				// blizzlike should be 10+ yd (would have to sort playerlist)
				// increase chance to select more distant unit slightly
				Unit* pUnit1 = SelectUnit(SELECT_TARGET_RANDOM, 0);
				Unit* pUnit2 = SelectUnit(SELECT_TARGET_RANDOM, 0);
				return me->GetDistanceOrder(pUnit1, pUnit2, false) ? pUnit1 : pUnit2;
			}

			void DropPlayer()
			{
				if(m_bDropPlayer_Countdown)
				{
					if(Player* pPlayer = me->GetPlayer(*me, m_uiImpaledGUID))
					{
						pPlayer->ExitVehicle();
						pPlayer->KnockbackFrom(me->GetPositionX(), me->GetPositionY(), 40, 10);
					}
				}

				m_bDropPlayer_Countdown = false;
				m_uiImpaledGUID = 0;
			}

			void UpdateAI(const uint32 diff)
			{
				if (!UpdateVictim())
				{
					DropPlayer();
					return;
				}

				if (me->HasUnitState(UNIT_STATE_CASTING))
					return;

				// jump. jumps twice - iCelike, every 40 sec
				if(m_uiCrushingLeapJump_Timer < diff)
				{
					if(Unit* pJumpTarget = GetLeapTarget())
					{
						// stop moving
						me->GetMotionMaster()->MoveIdle();

						// the jump to last 1 sec (since speed = dist per sec)
						float speedXY = me->GetDistance2d(pJumpTarget);
						me->GetMotionMaster()->MoveJump(pJumpTarget->GetPositionX(), pJumpTarget->GetPositionY(), pJumpTarget->GetPositionZ(), speedXY, 18);

					//	me->CastSpell(me->getVictim(), SPELL_CRUSHING_LEAP, false); // trigger missile effect handled wrong?

						// boss emote to raid. $N = Name of victim
						me->MonsterTextEmote("Archavon the Stone Watcher lunges for $N!", pJumpTarget->GetGUID(), true);

						m_bCrushingLeapSpell_Countdown = true;
						m_uiRockShards_Timer += 4000; // not to cast rock shards while jumping
					}

					m_uiCrushingLeapJump_Timer = m_bJumpOne ? 3500 : 35300;
					return;
				} else m_uiCrushingLeapJump_Timer -= diff;

				// after jump
				if(m_uiCrushingLeapSpell_Timer < diff)
				{
					// triggers damage + choking cloud. spell range 10-80yd
					me->CastSpell(me->SummonTrigger(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() + 25, 1.0f, 400), RAID_MODE(58963,60895), true);

					if(m_bJumpOne)
						m_bJumpOne = false;
					else
					{
						// go back to victim
						me->GetMotionMaster()->MoveChase(me->getVictim());
						m_bJumpOne = true;
					}

					m_uiCrushingLeapSpell_Timer = 1200;
					m_bCrushingLeapSpell_Countdown = false;
				}
				else if(m_bCrushingLeapSpell_Countdown)
					m_uiCrushingLeapSpell_Timer -= diff;

				// stomp. every 40 sec, 25 sec after jump
				if(m_uiStomp_Timer < diff)
				{
					me->CastSpell(me->getVictim(), SPELL_STOMP, false);

					m_bImpale_Countdown = true;
					m_uiRockShards_Timer += 3000; // not to cast rock shards before impale

					m_uiStomp_Timer = 30000;
					return;
				} else m_uiStomp_Timer -= diff;

				// impale after stomp
				if(m_uiImpale_Timer < diff)
				{
					me->CastSpell(me->getVictim(), SPELL_IMPALE, false);

					// apply iCelike DOT
					if(SpellEntry* customSpell = (SpellEntry *)sSpellStore.LookupEntry(26548))
					{
						customSpell->EffectBasePoints[0] = customSpell->EffectBasePoints[0] * RAID_MODE(8,11);
						me->CastSpell(me->getVictim(), customSpell, true);

						customSpell->EffectBasePoints[0] = customSpell->EffectBasePoints[0] / RAID_MODE(8,11);
						customSpell = NULL;
					}

					// enter vehicle manually
					if(Player* pImpaled = (Player *)me->getVictim())
					{
						pImpaled->EnterVehicle(me, 0);
						me->getThreatManager().modifyThreatPercent(pImpaled, -99);
						m_uiImpaledGUID = pImpaled->GetGUID();
					}

					m_uiImpale_Timer = 1000;
					m_bDropPlayer_Countdown = true;
					m_bImpale_Countdown = false;
				}
				else if(m_bImpale_Countdown)
					m_uiImpale_Timer -= diff;

				// leave vehicle manually
				if(m_uiDropPlayer_Timer < diff)
				{
					DropPlayer();

					m_uiDropPlayer_Timer = 8000;
				}
				else if(m_bDropPlayer_Countdown)
					m_uiDropPlayer_Timer -= diff;

				// enrage
				if(!m_bBerserk && m_uiBerserk_Timer < diff)
				{
					me->CastSpell(me, SPELL_BERSERK, false);
					DoScriptText(EMOTE_BERSERK, me);

					m_bBerserk = true;
				} else if(!m_bBerserk)
					m_uiBerserk_Timer -= diff;

				// rock shards
				if(m_uiRockShards_Timer < diff)
				{
					if(Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0))
						DoCast(pTarget, SPELL_ROCK_SHARDS);

					m_uiRockShards_Timer = 12000;
				} else m_uiRockShards_Timer -= diff;

				DoMeleeAttackIfReady();
			}
		};

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_archavonAI(creature);
        }
};

/*######
##  Mob Archavon Warder
######*/
class mob_archavon_warder : public CreatureScript
{
    public:
        mob_archavon_warder() : CreatureScript("mob_archavon_warder") { }

        struct mob_archavon_warderAI : public ScriptedAI //npc 32353
        {
            mob_archavon_warderAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            EventMap events;

            void Reset()
            {
                events.Reset();
                events.ScheduleEvent(EVENT_ROCK_SHOWER, 2000);
                events.ScheduleEvent(EVENT_SHIELD_CRUSH, 20000);
                events.ScheduleEvent(EVENT_WHIRL, 7500);
            }

            void EnterCombat(Unit* /*who*/)
            {
                DoZoneInCombat();
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_ROCK_SHOWER:
                            if (Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0))
                                DoCast(target, SPELL_ROCK_SHOWER);
                            events.ScheduleEvent(EVENT_ROCK_SHOWER, 6000);
                            break;
                        case EVENT_SHIELD_CRUSH:
                            DoCastVictim(SPELL_SHIELD_CRUSH);
                            events.ScheduleEvent(EVENT_SHIELD_CRUSH, 20000);
                            break;
                        case EVENT_WHIRL:
                            DoCastVictim(SPELL_WHIRL);
                            events.ScheduleEvent(EVENT_WHIRL, 8000);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_archavon_warderAI(creature);
        }
};

void AddSC_boss_archavon()
{
    new boss_archavon();
    new mob_archavon_warder();
}
