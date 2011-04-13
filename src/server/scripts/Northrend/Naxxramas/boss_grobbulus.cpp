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

#define SPELL_BOMBARD_SLIME         28280

#define SPELL_POISON_CLOUD          28240
#define SPELL_MUTATING_INJECTION    28169
#define SPELL_SLIME_SPRAY           RAID_MODE(28157,54364)
#define SPELL_BERSERK               26662
#define SPELL_POISON_CLOUD_ADD      59116

#define SPELL_SLIME_PERIODIC        58809
#define SPELL_SLIME_PERIODIC_H      71125

#define SPELL_POISON_CLOUD_DAMAGE   30914
#define SPELL_POISON_CLOUD_ADDIT    38463

#define EVENT_BERSERK   1
#define EVENT_CLOUD     2
#define EVENT_INJECT    3
#define EVENT_SPRAY     4

#define MOB_FALLOUT_SLIME   16290

class boss_grobbulus : public CreatureScript
{
public:
    boss_grobbulus() : CreatureScript("boss_grobbulus") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_grobbulusAI (pCreature);
    }

    struct boss_grobbulusAI : public BossAI
    {
        boss_grobbulusAI(Creature *c) : BossAI(c, BOSS_GROBBULUS)
        {
            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_POISON_CLOUD_ADD, true);
        }

        void EnterCombat(Unit * /*who*/)
        {
            _EnterCombat();
            events.ScheduleEvent(EVENT_CLOUD, 15000);
            events.ScheduleEvent(EVENT_INJECT, 20000);
            events.ScheduleEvent(EVENT_SPRAY, 15000+rand()%15000); //not sure
            events.ScheduleEvent(EVENT_BERSERK, 12*60000);
        }

        void SpellHitTarget(Unit *pTarget, const SpellEntry *spell)
        {
            if (spell->Id == uint32(SPELL_SLIME_SPRAY))
            {
                if (TempSummon *slime = me->SummonCreature(MOB_FALLOUT_SLIME, pTarget->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 0))
                    DoZoneInCombat(slime);
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case EVENT_CLOUD:
                        DoCastAOE(SPELL_POISON_CLOUD);
                        events.ScheduleEvent(EVENT_CLOUD, 13000+urand(0,5000));
                        return;
                    case EVENT_BERSERK:
                        DoCastAOE(SPELL_BERSERK);
                        return;
                    case EVENT_SPRAY:
                        DoCastAOE(SPELL_SLIME_SPRAY);
                        events.ScheduleEvent(EVENT_SPRAY, 18000+rand()%4000);
                        return;
                    case EVENT_INJECT:
                        if (Unit *pTarget = SelectUnit(SELECT_TARGET_RANDOM, 1))
                            if (!pTarget->HasAura(SPELL_MUTATING_INJECTION))
                                DoCast(pTarget, SPELL_MUTATING_INJECTION);
                        events.ScheduleEvent(EVENT_INJECT, 11000 + uint32(100 * me->GetHealthPct()));
                        return;
                }
            }

            DoMeleeAttackIfReady();
        }
    };

};

class npc_grobbulus_poison_cloud : public CreatureScript
{
public:
    npc_grobbulus_poison_cloud() : CreatureScript("npc_grobbulus_poison_cloud") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_grobbulus_poison_cloudAI(pCreature);
    }

    struct npc_grobbulus_poison_cloudAI : public Scripted_NoMovementAI
    {
        npc_grobbulus_poison_cloudAI(Creature* pCreature) : Scripted_NoMovementAI(pCreature)
        {
            Reset();
        }

        uint32 Cloud_Timer;

        void Reset()
        {
            Cloud_Timer = 1000;
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }

        void UpdateAI(const uint32 diff)
        {
            if (Cloud_Timer <= diff)
            {
                DoCast(me, SPELL_POISON_CLOUD_ADD);
                Cloud_Timer = 10000;
            } else Cloud_Timer -= diff;
        }
    };

};

class mob_fallout_slime: public CreatureScript
{
public:
	mob_fallout_slime(): CreatureScript("mob_fallout_slime") {};

	struct mob_fallout_slimeAI: public ScriptedAI
	{
		mob_fallout_slimeAI(Creature* c): ScriptedAI(c)
		{
			Reset();
		}

		uint32 ResetTimer;
		uint32 StackTimer;

		void Reset()
		{
			ResetTimer = 11000;
			StackTimer = 1000;
		}

		void EnterCombat(Unit* pWho)
		{
			ResetTimer = 11000;
			StackTimer = 1000;
		}

		void UpdateAI(const uint32 diff)
		{
			if(!UpdateVictim())
			{
				if(me->GetHealth() < me->GetMaxHealth())
					EnterEvadeMode();
				return;
			}

			if(ResetTimer <= diff)
			{
			//	DoCast(m_creature->getVictim(), 41978, true);
				//DoCast(me->getVictim(), 41978, true);
				DoCast(me->getVictim(),32264,true);
				me->getThreatManager().clearReferences();
				Unit* pFutureTarget = SelectUnit(SELECT_TARGET_RANDOM,0);
				if(pFutureTarget)
					me->AddThreat(pFutureTarget,9000.0f);
				me->SetInCombatWithZone();
				ResetTimer = 3000;
			} else ResetTimer -= diff;

			if(StackTimer <= diff)
			{
				DoCast(me->getVictim(), 41978, true);
				//DoCast(me->getVictim(),32264,true);
				//if (me->getVictim())
				//	me->getVictim()->JustAddAura(32264, m_creature);
				StackTimer = 1000;
			} else StackTimer -= diff;
		}

	    Player* SelectRandomPlayer(float range = 0.0f)
		{
			Map *map = me->GetMap();
			if (map->IsDungeon())
			{
				Map::PlayerList const &PlayerList = map->GetPlayers();

				if (PlayerList.isEmpty())
					return NULL;

				for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
				{
					if((range == 0.0f || me->IsWithinDistInMap(i->getSource(), range))
						&& i->getSource()->isTargetableForAttack())
					return i->getSource();
				}
				return NULL;
			}
			else
				return NULL;
		}
	};

	CreatureAI* GetAI(Creature* pCreature) const
	{
		return new mob_fallout_slimeAI(pCreature);
	}
};

void AddSC_boss_grobbulus()
{
    new boss_grobbulus();
    new npc_grobbulus_poison_cloud();
	new mob_fallout_slime();
}
