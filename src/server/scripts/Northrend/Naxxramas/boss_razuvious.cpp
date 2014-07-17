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
#include "Spell.h"

//Razuvious - NO TEXT sound only
//8852 aggro01 - Hah hah, I'm just getting warmed up!
//8853 aggro02 Stand and fight!
//8854 aggro03 Show me what you've got!
//8861 slay1 - You should've stayed home!
//8863 slay2-
//8858 cmmnd3 - You disappoint me, students!
//8855 cmmnd1 - Do as I taught you!
//8856 cmmnd2 - Show them no mercy!
//8859 cmmnd4 - The time for practice is over! Show me what you've learned!
//8861 Sweep the leg! Do you have a problem with that?
//8860 death - An honorable... death...
//8947 - Aggro Mixed? - ?

#define SOUND_AGGRO     RAND(8852,8853,8854)
#define SOUND_SLAY      RAND(8861,8863)
#define SOUND_COMMND    RAND(8855,8856,8858,8859,8861)
#define SOUND_DEATH     8860
#define SOUND_AGGROMIX  8847

#define SPELL_UNBALANCING_STRIKE    26613
#define SPELL_DISRUPTING_SHOUT      RAID_MODE(29107,55543)
#define SPELL_JAGGED_KNIFE          55550
#define SPELL_HOPELESS              29125

#define NPC_DEATH_KNIGHT_UNDERSTUDY 16803

enum Events
{
    EVENT_NONE,
    EVENT_STRIKE,
    EVENT_SHOUT,
    EVENT_KNIFE,
    EVENT_COMMAND,
	EVENT_SUMMON,
};

class mob_razuvious_image: public CreatureScript
{
public:
	mob_razuvious_image(): CreatureScript("mob_razuvious_image") {}

	struct mob_razuvious_imageAI: public ScriptedAI
	{
		mob_razuvious_imageAI(Creature* c): ScriptedAI(c)
		{
			Reset();
		}

		void Reset()
		{
		}

		void GodSaidAttackHim(Player* pWho)
		{
			if(!pWho)
				return;

			DoResetThreat();
			AttackStart(pWho);
			me->AddThreat(pWho,1000000000.0f);
			float tx,ty,tz;
			pWho->GetPosition(tx,ty,tz);
			me->GetMotionMaster()->MoveJump(tx,ty,tz,10,10);
		}

		void UpdateAI(const uint32 diff)
		{
			if(!UpdateVictim())
			{
				if(me->GetHealth() < me->GetMaxHealth())
					EnterEvadeMode();
				return;
			}

			DoMeleeAttackIfReady();
		}
	};

	CreatureAI* GetAI(Creature* pCreature) const
	{
		return new mob_razuvious_imageAI(pCreature);
	}
};

class boss_razuvious : public CreatureScript
{
public:
    boss_razuvious() : CreatureScript("boss_razuvious") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_razuviousAI (pCreature);
    }

    struct boss_razuviousAI : public BossAI
    {
        boss_razuviousAI(Creature *c) : BossAI(c, BOSS_RAZUVIOUS) { Reset(); }

		void Reset()
		{
			DespawnAdds();
			SpawnAdds();
            _Reset();
		}

        void KilledUnit(Unit* /*victim*/)
        {
            if (!(rand()%3))
                DoPlaySoundToSet(me, SOUND_SLAY);
        }

        void DamageTaken(Unit* pDone_by, uint32& uiDamage)
        {
            // Damage done by the controlled Death Knight understudies should also count toward damage done by players
            if (pDone_by->GetTypeId() == TYPEID_UNIT && (pDone_by->GetEntry() == 16803 || pDone_by->GetEntry() == 29941))
            {
                me->LowerPlayerDamageReq(uiDamage);
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            _JustDied();
            DoPlaySoundToSet(me, SOUND_DEATH);
            me->CastSpell(me, SPELL_HOPELESS, true); // TODO: this may affect other creatures
        }

        void EnterCombat(Unit * /*who*/)
        {
            _EnterCombat();
            DoPlaySoundToSet(me, SOUND_AGGRO);
            events.ScheduleEvent(EVENT_STRIKE, 25000);
            events.ScheduleEvent(EVENT_SHOUT, 15000);
            events.ScheduleEvent(EVENT_COMMAND, 40000);
            events.ScheduleEvent(EVENT_KNIFE, 10000);
			events.ScheduleEvent(EVENT_SUMMON, 8000);
        }

	    void DespawnAdds()
		{
			std::list<Creature*> m_pDeathKnight;
	        GetCreatureListWithEntryInGrid(m_pDeathKnight, me, NPC_DEATH_KNIGHT_UNDERSTUDY, DEFAULT_VISIBILITY_INSTANCE);

	        if (!m_pDeathKnight.empty())
		        for(std::list<Creature*>::iterator itr = m_pDeathKnight.begin(); itr != m_pDeathKnight.end(); ++itr)
			        (*itr)->ForcedDespawn();
		}

	    void SpawnAdds()
		{
			me->SummonCreature(NPC_DEATH_KNIGHT_UNDERSTUDY, 2757.48f, -3111.52f, 267.77f, 3.93f, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 3000000);
	        me->SummonCreature(NPC_DEATH_KNIGHT_UNDERSTUDY, 2762.05f, -3084.47f, 267.77f, 2.13f, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 3000000);

			if(!me->GetInstanceScript()->instance->IsRegularDifficulty())
		    {
			    me->SummonCreature(NPC_DEATH_KNIGHT_UNDERSTUDY, 2781.99f, -3087.81f, 267.68f, 0.61f, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 3000000);
				me->SummonCreature(NPC_DEATH_KNIGHT_UNDERSTUDY, 2779.13f, -3112.39f, 267.68f, 5.1f, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 3000000);
	        }
		}

		void JustSummoned(Creature* pCreature)
		{
			if(pCreature->GetEntry() == NPC_DEATH_KNIGHT_UNDERSTUDY)
				pCreature->HandleEmoteCommand(EMOTE_ONESHOT_ATTACK1H);
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
                    case EVENT_STRIKE:
                        DoCast(me->GetVictim(), SPELL_UNBALANCING_STRIKE);
                        events.ScheduleEvent(EVENT_STRIKE, 28000);
                        return;
                    case EVENT_SHOUT:
                        DoCastAOE(SPELL_DISRUPTING_SHOUT);
                        events.ScheduleEvent(EVENT_SHOUT, 21000);
                        return;
                    case EVENT_KNIFE:
                        if (Unit *pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 45.0f))
                            DoCast(pTarget, SPELL_JAGGED_KNIFE);
                        events.ScheduleEvent(EVENT_KNIFE, 12000);
                        return;
                    case EVENT_COMMAND:
                        DoPlaySoundToSet(me, SOUND_COMMND);
                        events.ScheduleEvent(EVENT_COMMAND, 40000);
                        return;
					case EVENT_SUMMON:
						Creature* pImage = me->SummonCreature(99954,0,0,0,0,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT,10000);
						mob_razuvious_image::mob_razuvious_imageAI* mAI = (mob_razuvious_image::mob_razuvious_imageAI*)pImage->AI();
						Player* pRandomPlayer = SelectRandomPlayer(100.0f);
						if(pImage && mAI && pRandomPlayer)
						{
							pImage->SetInCombatWithZone();
							mAI->GodSaidAttackHim(pRandomPlayer);
						}
						events.ScheduleEvent(EVENT_SUMMON, 22000+urand(0,3000));
						return;
                }
            }

            DoMeleeAttackIfReady();
        }
    };

};

class npc_obedience_crystal: public CreatureScript
{
public:
	npc_obedience_crystal(): CreatureScript("npc_obedience_crystal") {}

	struct npc_obedience_crystalAI: public ScriptedAI
	{
		npc_obedience_crystalAI(Creature* c): ScriptedAI(c)
		{
			Possessor = NULL;
			Possessed = NULL;
			UnpossessTimer = 120000;
			Reset();
		}

		Player* Possessor;
		Unit* Possessed;
		uint32 UnpossessTimer;

		void Reset()
		{
		}

		void SetPossessor(Player* pPlayer)
		{
			Possessor = pPlayer;
			UnpossessTimer = 120000;
		}

		void SetPossessed(Unit* pCreature)
		{
			Possessed = pCreature;
			if(pCreature)
				pCreature->HandleEmoteCommand(EMOTE_ONESHOT_NONE);
		}

		Player* GetPossessor()
		{
			return Possessor;
		}

		void UpdateAI(const uint32 diff)
		{
			if(Possessor)
			{
				Spell* pSpell = NULL;
				pSpell = Possessor->GetCurrentSpell(CURRENT_CHANNELED_SPELL);

				if(pSpell)
				{
					if(pSpell->m_spellInfo->Id != 55479)
					{
						if(Possessed && Possessed->IsAlive())
							Possessed->RemoveAurasDueToSpell(530);
						if(Possessor && Possessor->IsAlive())
							Possessor->InterruptNonMeleeSpells(false);
						Possessor = NULL;
						Possessed = NULL;
						me->InterruptNonMeleeSpells(false);
					}
				}
				else if(Possessor->isDead())
				{
					if(Possessed && Possessed->IsAlive())
						Possessed->RemoveAurasDueToSpell(530);
					if(Possessor && Possessor->IsAlive())
						Possessor->InterruptNonMeleeSpells(false);
					Possessor = NULL;
					Possessed = NULL;
					me->InterruptNonMeleeSpells(false);
				}
			}

			if(UnpossessTimer <= diff)
			{
				if(Possessed && Possessed->IsAlive())
					Possessed->RemoveAurasDueToSpell(530);
				if(Possessor && Possessor->IsAlive())
					Possessor->InterruptNonMeleeSpells(false);
				Possessor = NULL;
				Possessed = NULL;
				me->InterruptNonMeleeSpells(false);
			} else UnpossessTimer -= diff;
		}
	};

	CreatureAI* GetAI(Creature* pCreature) const
	{
		return new npc_obedience_crystalAI(pCreature);
	}

	bool OnGossipHello(Player* pPlayer, Creature* pCreature)
	{
		if(((npc_obedience_crystalAI*)pCreature->AI())->GetPossessor())
			return true;

		Unit* target = NULL;
		std::list<Creature*> DKList;
		GetCreatureListWithEntryInGrid(DKList,pPlayer,NPC_DEATH_KNIGHT_UNDERSTUDY,200.0f);
		for(std::list<Creature*>::const_iterator itr = DKList.begin(); itr != DKList.end(); ++itr)
		{
			if((*itr))
			{
				if( !(*itr)->HasAura(530) )
				{
					target = *itr;
					break;
				}
			}
		}

		if (target)
		{
			SpellEntry* sp = (SpellEntry*)GetSpellStore()->LookupEntry(55479);
			sp->Effect[0] = sp->Effect[1];
			pPlayer->CastSpell(target, sp, true);
			pPlayer->CastSpell(target, 530, true);
			((npc_obedience_crystalAI*)pCreature->AI())->SetPossessor(pPlayer);
			((npc_obedience_crystalAI*)pCreature->AI())->SetPossessed(target);
			return true;
		}
		return false;
	}
};

class mob_dk_understudy: public CreatureScript
{
public:
	mob_dk_understudy(): CreatureScript("mob_dk_understudy") {}

	struct mob_dk_understudyAI: public ScriptedAI
	{
		mob_dk_understudyAI(Creature* c): ScriptedAI(c)
		{
			Reset();
		}

		void Reset()
		{
		}

		void JustReachedHome()
		{
			me->HandleEmoteCommand(EMOTE_ONESHOT_ATTACK1H);
		}

		void MoveInLineOfSight(Unit* pWho)
		{
			return;
		}

		void UpdateAI(const uint32 diff)
		{
			if(!UpdateVictim())
			{
				if(me->GetHealth() < me->GetMaxHealth())
					EnterEvadeMode();
				return;
			}

			DoMeleeAttackIfReady();
		}
	};

	CreatureAI* GetAI(Creature* pCreature) const
	{
		return new mob_dk_understudyAI(pCreature);
	}
};

void AddSC_boss_razuvious()
{
    new boss_razuvious();
	new mob_razuvious_image();
	new mob_dk_understudy();
	new npc_obedience_crystal();
}
