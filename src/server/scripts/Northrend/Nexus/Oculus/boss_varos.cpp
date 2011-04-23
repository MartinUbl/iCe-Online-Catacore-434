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
#include "oculus.h"

enum
{
	SPELL_ENERGIZE_CORES                   = 50785,
	H_SPELL_ENERGIZE_CORES                 = 59372,
	SPELL_ENERGIZE_CORES_VISUAL            = 61407,

	SPELL_ARCANE_BEAM_VISUAL               = 51024,
	SPELL_ARCANE_BEAM_PERIODIC             = 51019,
	SPELL_AMPLIFY_MAGIC                    = 51054,
	H_SPELL_AMPLIFY_MAGIC                  = 59371,

	SAY_INTRO                              = -1595013,
	SAY_AIRSTRIKE_1                        = -1595014,
	SAY_AIRSTRIKE_2                        = -1595015,
	SAY_AIRSTRIKE_3                        = -1595016,
	EMOTE_AIRSTRIKE                        = -1595018,
	SAY_DEATH                              = -1595017
};

#define X   1286
#define Y   1070
#define Z   440
#define R   45
#define PI  3.141f

class boss_varos : public CreatureScript
{
public:
    boss_varos() : CreatureScript("boss_varos_cloudstrider") { }

    struct boss_varos_cloudstriderAI : public ScriptedAI
    {
	boss_varos_cloudstriderAI(Creature* pCreature) : ScriptedAI(pCreature)
	{
		    m_pInstance = pCreature->GetInstanceScript();
		m_bIsRegularMode = pCreature->GetMap()->IsRegularDifficulty();
		    Reset();
	    }

	InstanceScript* m_pInstance;
	    bool m_bIsRegularMode;

	uint32 AzureRingCaptain_Timer;
	    uint32 AzureRingCaptain_Cooldown;
	uint32 EnergizeCoresVisual_Timer;
	    uint32 CastEnergizeCoresVisual_Timer;
	uint32 EnergizeCores_Timer;
	    uint32 BackToFight_Timer;
	uint32 direction_number;
	    bool Intro;
	bool AzureRingCaptain_80;
	    bool AzureRingCaptain_60;
	bool AzureRingCaptain_40;
	    bool AzureRingCaptain_20;
	Unit* pAggro;
	    Creature* pDummyAggro;
		Creature* pDummyVisual[2];

	void Reset()
	    {
		    Intro = false;
		AzureRingCaptain_Timer = 30000;
		AzureRingCaptain_Cooldown = 0;
		    EnergizeCoresVisual_Timer = 6000;
		direction_number = 1;
		CastEnergizeCoresVisual_Timer = 20000;
		    EnergizeCores_Timer = 20000;
			BackToFight_Timer = 20000;
			AzureRingCaptain_80 = false;
			AzureRingCaptain_60 = false;
			AzureRingCaptain_40 = false;
			AzureRingCaptain_20 = false;
			pAggro = NULL;
			pDummyAggro = NULL;
			for(int i = 0; i < 2; i++)
				pDummyVisual[i] = NULL;
			DeSpawnDummyVisual();
		}

		void MoveInLineOfSight(Unit *who)
		{
			if (!me->getVictim() && who->isTargetableForAttack() && me->IsHostileTo(who))
			{
				if (!me->canFly() && me->GetDistanceZ(who) > CREATURE_Z_ATTACK_RANGE)
					return;

				if (!Intro && me->IsWithinDistInMap(who, me->GetAttackDistance(who)))
				{
					DoScriptText(SAY_INTRO, me);
					Intro = true;

					float attackRadius = me->GetAttackDistance(who);
					if (me->IsWithinDistInMap(who, attackRadius) && me->IsWithinLOSInMap(who))
					{
						who->RemoveAurasByType(SPELL_AURA_MOD_STEALTH);
						AttackStart(who);
					}
				}
			}
		}

		void DamageTaken(Unit* pDoneBy, uint32 &damage)
		{
			if (!Intro)
			{
				DoScriptText(SAY_INTRO, me);
				Intro = true;
			}
		}

		void JustDied(Unit* Killer)
		{
			DoScriptText(SAY_DEATH, me);
		}

		void SpellHitTarget(Unit* target, const SpellEntry* spell)
		{
			if (spell->Id == 56251 && target->GetTypeId() == TYPEID_PLAYER)
			{
				target->RemoveAurasDueToSpell(56251);
			}
		}

		void CallAzureRingCaptain()
		{
			DoScriptText(EMOTE_AIRSTRIKE, me);
			switch(urand(0, 2))
			{
				default: DoScriptText(SAY_AIRSTRIKE_1, me); break;
				case 1: DoScriptText(SAY_AIRSTRIKE_2, me); break;
				case 2: DoScriptText(SAY_AIRSTRIKE_3, me); break;
			}
			float y, z;
			y = Y + 45;
			z = Z + 35;
			me->SummonCreature(28236, X, y, z, 2, TEMPSUMMON_DEAD_DESPAWN, 15000);
			AzureRingCaptain_Timer = 30000;
			AzureRingCaptain_Cooldown = 13000;
		}

		void SpawnDummyAggro(uint32 direction)
		{
			pDummyAggro = NULL;
			pAggro = me->getVictim();
	     //      DoStopAttack();
			switch(direction)
			{
				default: pDummyAggro = me->SummonCreature(10002, (X - R), (Y + R), Z, 2, TEMPSUMMON_TIMED_DESPAWN, 1000); break;
				case 2: pDummyAggro = me->SummonCreature(10002, (X - R), (Y - R), Z, 2, TEMPSUMMON_TIMED_DESPAWN, 1000); break;
				case 3: pDummyAggro = me->SummonCreature(10002, (X + R), (Y - R), Z, 2, TEMPSUMMON_TIMED_DESPAWN, 1000); break;
				case 4: pDummyAggro = me->SummonCreature(10002, (X + R), (Y + R), Z, 2, TEMPSUMMON_TIMED_DESPAWN, 1000); break;
			}
			if (pDummyAggro)
			{
				me->AddThreat(pDummyAggro, 1000000);
				me->Attack(pDummyAggro,false);
				//sLog->outError("dummy aggro ok");
			}
		}

		void DeSpawnDummyVisual()
		{
			for(int i=0;i<2;i++)
			{
				if(pDummyVisual[i])
				{
					pDummyVisual[i]->Kill(pDummyVisual[i]);
					pDummyVisual[i]->ForcedDespawn();
				}
			}
		}

		void SpawnDummyVisual(uint32 direction)
		{
			switch(direction)
			{
				default:
					pDummyVisual[0] = me->SummonCreature(10002, X, (Y + R), Z, 2, TEMPSUMMON_DEAD_DESPAWN, 1000);
					pDummyVisual[1] = me->SummonCreature(10002, (X - R), Y, Z, 2, TEMPSUMMON_DEAD_DESPAWN, 1000);
					break;
				case 2:
					pDummyVisual[0] = me->SummonCreature(10002, (X - R), Y, Z, 2, TEMPSUMMON_DEAD_DESPAWN, 1000);
					pDummyVisual[1] = me->SummonCreature(10002, X, (Y - R), Z, 2, TEMPSUMMON_DEAD_DESPAWN, 1000);
					break;
				case 3:
					pDummyVisual[0] = me->SummonCreature(10002, X, (Y - R), Z, 2, TEMPSUMMON_DEAD_DESPAWN, 1000);
					pDummyVisual[1] = me->SummonCreature(10002, (X + R), Y, Z, 2, TEMPSUMMON_DEAD_DESPAWN, 1000);
					break;
				case 4:
					pDummyVisual[0] = me->SummonCreature(10002, (X + R), Y, Z, 2, TEMPSUMMON_DEAD_DESPAWN, 1000);
					pDummyVisual[1] = me->SummonCreature(10002, X, (Y + R), Z, 2, TEMPSUMMON_DEAD_DESPAWN, 1000);
					break;
			}
			if(pDummyVisual[0])
				pDummyVisual[0]->CastSpell(me,45967,true);
			if(pDummyVisual[1])
				pDummyVisual[1]->CastSpell(me,45967,true);
		}

		void UpdateAI(const uint32 diff)
		{
			if (!UpdateVictim() || me->getVictim()->GetEntry() == 28183)
			{
				if (Intro)
				{
					EnterEvadeMode();
				}
				return;
			}

			if (CastEnergizeCoresVisual_Timer < diff)
			{
				//DoCast(me, SPELL_ENERGIZE_CORES_VISUAL, true);
				DeSpawnDummyVisual();
				SpawnDummyVisual(direction_number);
				if (pDummyAggro)
				{
					//me->DealDamage(pDummyAggro, pDummyAggro->GetHealth(), 0, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, 0, false);
					me->Kill(pDummyAggro);
					pDummyAggro = NULL;
				}
				CastEnergizeCoresVisual_Timer = 20000;
			} else CastEnergizeCoresVisual_Timer -= diff;

			if (BackToFight_Timer < diff)
			{
				if (pAggro)
					AttackStart(pAggro);
				BackToFight_Timer = 20000;
			} else BackToFight_Timer -= diff;

			if (EnergizeCoresVisual_Timer < diff)
			{
				SpawnDummyAggro(direction_number);
				CastEnergizeCoresVisual_Timer = 1;
				EnergizeCores_Timer = m_bIsRegularMode ? 4000 : 4500;
				BackToFight_Timer = 1000;
				EnergizeCoresVisual_Timer = 20000;
			} else EnergizeCoresVisual_Timer -= diff;

			if (EnergizeCores_Timer < diff)
			{
				SpawnDummyAggro(direction_number);
				(direction_number < 4) ? direction_number++ : direction_number = 1;
				DoCast(me, m_bIsRegularMode ? SPELL_ENERGIZE_CORES : H_SPELL_ENERGIZE_CORES);
				EnergizeCoresVisual_Timer = m_bIsRegularMode ? 1500 : 1000;
				BackToFight_Timer = (m_bIsRegularMode ? 1001 : 501);
				EnergizeCores_Timer = 20000;
			} else EnergizeCores_Timer -= diff;

			if (AzureRingCaptain_Cooldown < diff)
			{
				if (!AzureRingCaptain_80 && me->GetHealthPct() < 80.0f)
			{
				CallAzureRingCaptain();
				AzureRingCaptain_80 = true;
				return;
			}
			    if (!AzureRingCaptain_60 && me->GetHealthPct() < 60.0f)
			{
				CallAzureRingCaptain();
				AzureRingCaptain_60 = true;
				    return;
			    }
			if (!AzureRingCaptain_40 && me->GetHealthPct() < 40.0f)
			{
				CallAzureRingCaptain();
				    AzureRingCaptain_40 = true;
				return;
			}
			if (!AzureRingCaptain_20 && me->GetHealthPct() < 20.0f)
			{
				    CallAzureRingCaptain();
				AzureRingCaptain_20 = true;
				return;
			}
			if (AzureRingCaptain_Timer < diff)
			    {
				CallAzureRingCaptain();
			} else AzureRingCaptain_Timer -= diff;
		} else AzureRingCaptain_Cooldown -= diff;

		DoMeleeAttackIfReady();
	}
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_varos_cloudstriderAI(pCreature);
    }

};

class mob_centrifuge_core_varos: public CreatureScript
{
public:
	mob_centrifuge_core_varos(): CreatureScript("mob_centrifuge_core_varos") { }

	struct mob_centrifuge_core_varosAI : public ScriptedAI
	{
		mob_centrifuge_core_varosAI(Creature* pCreature) : ScriptedAI(pCreature)
		{
			Reset();
		}

		void DamageTaken(Unit* pDoneBy, uint32 &damage)
		{
			damage = 0;
		}

		void Reset()
		{
			me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_9);
			me->SetReactState(REACT_PASSIVE);
		}

		void UpdateAI(const uint32 diff)
		{
			if(UpdateVictim())
				EnterEvadeMode();
		}
	};

	CreatureAI* GetAI(Creature* pCreature) const
	{
		return new mob_centrifuge_core_varosAI(pCreature);
	}
};

class mob_azure_ring_captain: public CreatureScript
{
public:
	mob_azure_ring_captain(): CreatureScript("mob_azure_ring_captain_varos") {}

	struct mob_azure_ring_captain_varosAI : public ScriptedAI
	{
		mob_azure_ring_captain_varosAI(Creature* pCreature) : ScriptedAI(pCreature)
		{
			m_pInstance = pCreature->GetInstanceScript();
			m_bIsRegularMode = pCreature->GetMap()->IsRegularDifficulty();
			Reset();
		}

		InstanceScript* m_pInstance;
		bool m_bIsRegularMode;

		uint32 ArcaneBeam_Timer;
		uint32 SelfDestroy_Timer;
		bool Arcane_Beam;
		bool Self_Destroy;
		Unit* Dummy;

		void Reset()
		{
			me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
			me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_9);
			Arcane_Beam = false;
			Self_Destroy = false;
			ArcaneBeam_Timer = 4500;
			SelfDestroy_Timer = 15000;
			Dummy = me->SummonCreature(28239, X, Y, Z, 1, TEMPSUMMON_DEAD_DESPAWN, 15000);
			me->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
		}

		void UpdateAI(const uint32 diff)
		{
			if (!Arcane_Beam)
			{
				if (ArcaneBeam_Timer < diff)
				{
					DoCast(Dummy, SPELL_ARCANE_BEAM_VISUAL);
					Arcane_Beam = true;
				} else ArcaneBeam_Timer -= diff;
			}

			if (!Self_Destroy)
			{
				if (SelfDestroy_Timer < diff)
				{
					me->SetVisible(false);
					me->Kill(me);
					Self_Destroy = true;
				} else SelfDestroy_Timer -= diff;
			}
		}
	};

	CreatureAI* GetAI(Creature* pCreature) const
	{
		return new mob_azure_ring_captain_varosAI(pCreature);
	}
};

class mob_laser_dummy: public CreatureScript
{
public:
	mob_laser_dummy(): CreatureScript("mob_laser_dummy_varos") {}

	struct mob_laser_dummy_varosAI : public ScriptedAI
	{
		mob_laser_dummy_varosAI(Creature* pCreature) : ScriptedAI(pCreature)
		{
			m_pInstance = pCreature->GetInstanceScript();
			m_bIsRegularMode = pCreature->GetMap()->IsRegularDifficulty();
			Reset();
	    }

		InstanceScript* m_pInstance;
		bool m_bIsRegularMode;

		uint32 ArcaneBeam_Timer;
		uint32 SelfDestroy_Timer;
		bool Arcane_Beam;
		bool Self_Destroy;

		void Reset()
		{
			me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
			Arcane_Beam = false;
			Self_Destroy = false;
			ArcaneBeam_Timer = 5000;
			SelfDestroy_Timer = 15000;
		}

		void MoveInLineOfSight(Unit *who)
		{
			if (!me->getVictim() && who->isTargetableForAttack() && me->IsHostileTo(who))
			{
				if (!me->canFly() && me->GetDistanceZ(who) > CREATURE_Z_ATTACK_RANGE)
					return;

				float attackRadius = me->GetAttackDistance(who);
				if (me->IsWithinDistInMap(who, attackRadius) && me->IsWithinLOSInMap(who))
				{
					//who->RemoveSpellsCausingAura(SPELL_AURA_MOD_STEALTH);
					who->RemoveAurasByType(SPELL_AURA_MOD_STEALTH);
					DoCast(who, m_bIsRegularMode ? SPELL_AMPLIFY_MAGIC : H_SPELL_AMPLIFY_MAGIC);
					AttackStart(who);
				}
			}
		}

		void UpdateAI(const uint32 diff)
		{
			if (!Arcane_Beam)
			{
				if (ArcaneBeam_Timer < diff)
				{
					me->SetSpeed(MOVE_RUN, 0.35f); // unsure about the speed
					DoCast(me, SPELL_ARCANE_BEAM_PERIODIC);
					Arcane_Beam = true;
				} else ArcaneBeam_Timer -= diff;
			}

			if (!Self_Destroy)
			{
				if (SelfDestroy_Timer < diff)
				{
					me->Kill(me);
					Self_Destroy = true;
				} else SelfDestroy_Timer -= diff;
			}
		}
	};

	CreatureAI* GetAI(Creature* pCreature) const
	{
		return new mob_laser_dummy_varosAI(pCreature);
	}
};

void AddSC_boss_varos()
{
    new boss_varos();
	new mob_centrifuge_core_varos();
	new mob_azure_ring_captain();
	new mob_laser_dummy();
}
