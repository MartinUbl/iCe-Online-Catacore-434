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

//Stalagg
enum StalaggYells
{
    SAY_STAL_AGGRO          = -1533023, //not used
    SAY_STAL_SLAY           = -1533024, //not used
    SAY_STAL_DEATH          = -1533025  //not used
};

enum StalagSpells
{
    SPELL_POWERSURGE        = 28134,
    H_SPELL_POWERSURGE      = 54529,
    SPELL_MAGNETIC_PULL     = 28338,
    SPELL_STALAGG_TESLA     = 28097
};

//Feugen
enum FeugenYells
{
    SAY_FEUG_AGGRO          = -1533026, //not used
    SAY_FEUG_SLAY           = -1533027, //not used
    SAY_FEUG_DEATH          = -1533028 //not used
};

enum FeugenSpells
{
    SPELL_STATICFIELD       = 28135,
    H_SPELL_STATICFIELD     = 54528,
    SPELL_FEUGEN_TESLA      = 28109
};

// Thaddius DoAction
enum ThaddiusActions
{
    ACTION_FEUGEN_RESET,
    ACTION_FEUGEN_DIED,
    ACTION_STALAGG_RESET,
    ACTION_STALAGG_DIED
};

//generic
#define C_TESLA_COIL            16218           //the coils (emotes "Tesla Coil overloads!")

//Thaddius
enum ThaddiusYells
{
    SAY_GREET               = -1533029, //not used
    SAY_AGGRO_1             = -1533030,
    SAY_AGGRO_2             = -1533031,
    SAY_AGGRO_3             = -1533032,
    SAY_SLAY                = -1533033,
    SAY_ELECT               = -1533034, //not used
    SAY_DEATH               = -1533035,
    SAY_SCREAM1             = -1533036, //not used
    SAY_SCREAM2             = -1533037, //not used
    SAY_SCREAM3             = -1533038, //not used
    SAY_SCREAM4             = -1533039 //not used
};

enum ThaddiusSpells
{
    SPELL_POLARITY_SHIFT        = 28089,
    SPELL_BALL_LIGHTNING        = 28299,
    SPELL_CHAIN_LIGHTNING       = 28167,
    H_SPELL_CHAIN_LIGHTNING     = 54531,
    SPELL_BERSERK               = 27680,
	//61902 Heal 20%, +25%dmg inf.stack, /pod50% s polarity shift
	//43654 Visual random lightning
	//56327 Random periodic lightning
	SPELL_HEAL_DMG                = 61902,
	SPELL_VISUAL_LIGHTNING        = 43654,
	SPELL_LIGHTNING_PERIODIC      = 56327,
};

enum Events
{
    EVENT_NONE,
    EVENT_SHIFT,
    EVENT_CHAIN,
    EVENT_BERSERK,
};

class boss_thaddius : public CreatureScript
{
public:
    boss_thaddius() : CreatureScript("boss_thaddius") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_thaddiusAI (pCreature);
    }

    struct boss_thaddiusAI : public BossAI
    {
        boss_thaddiusAI(Creature *c) : BossAI(c, BOSS_THADDIUS)
        {
            // init is a bit tricky because thaddius shall track the life of both adds, but not if there was a wipe
            // and, in particular, if there was a crash after both adds were killed (should not respawn)

            // Moreover, the adds may not yet be spawn. So just track down the status if mob is spawn
            // and each mob will send its status at reset (meaning that it is alive)
            checkFeugenAlive = false;
            if (Creature *pFeugen = me->GetCreature(*me, instance->GetData64(DATA_FEUGEN)))
                checkFeugenAlive = pFeugen->IsAlive();

            checkStalaggAlive = false;
            if (Creature *pStalagg = me->GetCreature(*me, instance->GetData64(DATA_STALAGG)))
                checkStalaggAlive = pStalagg->IsAlive();

            if (!checkFeugenAlive && !checkStalaggAlive)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED);
                me->SetReactState(REACT_AGGRESSIVE);
            }
            else
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED);
                me->SetReactState(REACT_PASSIVE);
            }
        }

        bool checkStalaggAlive;
        bool checkFeugenAlive;
        uint32 uiAddsTimer;
		uint32 LightningTimer;

        void KilledUnit(Unit* /*victim*/)
        {
            if (!(rand()%5))
                DoScriptText(SAY_SLAY, me);
        }

        void JustDied(Unit* /*Killer*/)
        {
            _JustDied();
            DoScriptText(SAY_DEATH, me);
        }

        void DoAction(const int32 action)
        {
            switch(action)
            {
                case ACTION_FEUGEN_RESET:
                    checkFeugenAlive = true;
                    break;
                case ACTION_FEUGEN_DIED:
                    checkFeugenAlive = false;
                    break;
                case ACTION_STALAGG_RESET:
                    checkStalaggAlive = true;
                    break;
                case ACTION_STALAGG_DIED:
                    checkStalaggAlive = false;
                    break;
            }

            if (!checkFeugenAlive && !checkStalaggAlive)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED);
                // REACT_AGGRESSIVE only reset when he takes damage.
                DoZoneInCombat();
            }
            else
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED);
                me->SetReactState(REACT_PASSIVE);
            }
        }

		void Reset()
		{
            _Reset();
			me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED);
		}

        void EnterCombat(Unit * /*who*/)
        {
            _EnterCombat();
            DoScriptText(RAND(SAY_AGGRO_1,SAY_AGGRO_2,SAY_AGGRO_3), me);
            events.ScheduleEvent(EVENT_SHIFT, 30000);
            events.ScheduleEvent(EVENT_CHAIN, urand(10000,20000));
            events.ScheduleEvent(EVENT_BERSERK, 360000);
			LightningTimer = 0;
        }

        void DamageTaken(Unit * /*pDoneBy*/, uint32 & /*uiDamage*/)
        {
            me->SetReactState(REACT_AGGRESSIVE);
        }

        void UpdateAI(const uint32 diff)
        {
            if (checkFeugenAlive && checkStalaggAlive)
                uiAddsTimer = 0;

            if (checkStalaggAlive != checkFeugenAlive)
            {
                uiAddsTimer += diff;
                if (uiAddsTimer > 5000)
                {
                    if (!checkStalaggAlive)
                    {
                        if (instance)
                            if (Creature *pStalagg = me->GetCreature(*me, instance->GetData64(DATA_STALAGG)))
                                pStalagg->Respawn();
                    }
                    else
                    {
                        if (instance)
                            if (Creature *pFeugen = me->GetCreature(*me, instance->GetData64(DATA_FEUGEN)))
                                pFeugen->Respawn();
                    }
                }
            }

            if (!UpdateVictim())
                return;

			//iCelike!
			//Visuals
			if(me->GetHealth() < me->GetMaxHealth()*0.5 && !me->HasAura(SPELL_LIGHTNING_PERIODIC))
			{
				me->CastSpell(me, SPELL_LIGHTNING_PERIODIC, true);
				LightningTimer = 2000;
			}
			if(me->GetHealth() > me->GetMaxHealth()*0.5 && me->HasAura(SPELL_LIGHTNING_PERIODIC))
			{
				me->RemoveAurasDueToSpell(SPELL_LIGHTNING_PERIODIC);
				LightningTimer = 0;
			}

			//iCelike!
			//Visual no.2
			if(LightningTimer)
			{
				if(LightningTimer <= diff)
				{
					DoCast(me, SPELL_VISUAL_LIGHTNING, true);
					LightningTimer = 2000;
				} else LightningTimer -= diff;
			}

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case EVENT_SHIFT:
                        DoCastAOE(SPELL_POLARITY_SHIFT);
						//iCelike!
						if(me->GetHealth() < me->GetMaxHealth()*0.5)
						{
							//asi 40% na 10man, 30% na 25man
							if(urand(0,RAID_MODE(120,150)) < 50)
								DoCast(me, SPELL_HEAL_DMG, true);
							events.ScheduleEvent(EVENT_SHIFT, 35000);
						}
						else
							events.ScheduleEvent(EVENT_SHIFT, 30000);
                        return;
                    case EVENT_CHAIN:
                        DoCast(me->GetVictim(), RAID_MODE(SPELL_CHAIN_LIGHTNING, H_SPELL_CHAIN_LIGHTNING));
                        events.ScheduleEvent(EVENT_CHAIN, urand(10000,20000));
                        return;
                    case EVENT_BERSERK:
                        DoCast(me, SPELL_BERSERK);
                        return;
                }
            }

            if (events.GetTimer() > 15000 && !me->IsWithinMeleeRange(me->GetVictim()))
                DoCast(me->GetVictim(), SPELL_BALL_LIGHTNING);
            else
                DoMeleeAttackIfReady();
        }
    };

};


class mob_stalagg : public CreatureScript
{
public:
    mob_stalagg() : CreatureScript("mob_stalagg") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_stalaggAI(pCreature);
    }

    struct mob_stalaggAI : public ScriptedAI
    {
        mob_stalaggAI(Creature *c) : ScriptedAI(c)
        {
            pInstance = c->GetInstanceScript();
			JustSpawnedVisual = false;
			if(GameObject* NearTeslaCoil = GetClosestGameObjectWithEntry(me,181478,100.0f))
				NearTeslaCoil->ResetDoorOrButton();
        }

        InstanceScript* pInstance;

        uint32 powerSurgeTimer;
        uint32 magneticPullTimer;
		bool JustSpawnedVisual;
		bool isFeugenClose;
		bool JustEnraged;
		uint32 EnrageCastTimer;

        void Reset()
        {
            if (pInstance)
                if (Creature *pThaddius = me->GetCreature(*me, pInstance->GetData64(DATA_THADDIUS)))
                    if (pThaddius->AI())
                        pThaddius->AI()->DoAction(ACTION_STALAGG_RESET);
            powerSurgeTimer = urand(20000,25000);
            magneticPullTimer = 20000;
			if(GameObject* NearTeslaCoil = GetClosestGameObjectWithEntry(me,181478,100.0f))
				NearTeslaCoil->ResetDoorOrButton();
        }

        void EnterCombat(Unit * /*pWho*/)
        {
            DoCast(SPELL_STALAGG_TESLA);
			isFeugenClose = false;
			JustEnraged = false;
			EnrageCastTimer = 0;
        }

        void JustDied(Unit * /*killer*/)
        {
            if (pInstance)
                if (Creature *pThaddius = me->GetCreature(*me, pInstance->GetData64(DATA_THADDIUS)))
                    if (pThaddius->AI())
                        pThaddius->AI()->DoAction(ACTION_STALAGG_DIED);

			if(GameObject* NearTeslaCoil = GetClosestGameObjectWithEntry(me,181478,100.0f))
				NearTeslaCoil->UseDoorOrButton();
        }

        void UpdateAI(const uint32 uiDiff)
        {
			if(!JustSpawnedVisual)
			{
				if(GameObject* NearTeslaCoil = GetClosestGameObjectWithEntry(me,181478,100.0f))
				{
					float gx,gy,gz;
					NearTeslaCoil->GetPosition(gx,gy,gz);
					if(Creature* TeslaVisual = me->SummonCreature(C_TESLA_COIL,gx,gy,gz,0.0f,TEMPSUMMON_DEAD_DESPAWN,0))
					{
						TeslaVisual->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_UNK_9 + UNIT_FLAG_NOT_SELECTABLE + UNIT_FLAG_NON_ATTACKABLE);
						TeslaVisual->GetMotionMaster()->MovePoint(0,gx,gy,gz);
						TeslaVisual->CastSpell(me,45537,true);
						JustSpawnedVisual = true;
					}
				}
			}

            if (!UpdateVictim())
                return;

			if (Creature *Feugen = me->GetCreature(*me, pInstance->GetData64(DATA_FEUGEN)))
			{
				if(Feugen->IsWithinDist(me,20.0f))
					isFeugenClose = true;
				else
					isFeugenClose = false;
			}

			if( (me->GetPositionZ() < 309.58f) || isFeugenClose)
			{
				if(!JustEnraged)
				{
					JustEnraged = true;
				}
				else
				{
					if(EnrageCastTimer <= uiDiff)
					{
						me->CastSpell(me->GetVictim(),17364,true);
						EnrageCastTimer = 500;
					} else EnrageCastTimer -= uiDiff;
				}
			}
			else
				JustEnraged = false;

            if (magneticPullTimer <= uiDiff)
            {
                if (Creature *pFeugen = me->GetCreature(*me, pInstance->GetData64(DATA_FEUGEN)))
                {
                    Unit* pStalaggVictim = me->GetVictim();
                    Unit* pFeugenVictim = pFeugen->GetVictim();

                    if (pFeugenVictim && pStalaggVictim)
                    {
                        // magnetic pull is not working. So just jump.

                        // reset aggro to be sure that feugen will not follow the jump
                        pFeugen->getThreatManager().modifyThreatPercent(pFeugenVictim, -100);
                        pFeugenVictim->JumpTo(me, 0.3f);

                        me->getThreatManager().modifyThreatPercent(pStalaggVictim, -100);
                        pStalaggVictim->JumpTo(pFeugen, 0.3f);
                    }
                }

                magneticPullTimer = 20000;
            }
            else magneticPullTimer -= uiDiff;

            if (powerSurgeTimer <= uiDiff)
            {
                DoCast(me, RAID_MODE(SPELL_POWERSURGE, H_SPELL_POWERSURGE));
                powerSurgeTimer = urand(15000,20000);
            } else powerSurgeTimer -= uiDiff;

            DoMeleeAttackIfReady();
        }
    };

};


class mob_feugen : public CreatureScript
{
public:
    mob_feugen() : CreatureScript("mob_feugen") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_feugenAI(pCreature);
    }

    struct mob_feugenAI : public ScriptedAI
    {
        mob_feugenAI(Creature *c) : ScriptedAI(c)
        {
            pInstance = c->GetInstanceScript();
			JustSpawnedVisual = false;
			if(GameObject* NearTeslaCoil = GetClosestGameObjectWithEntry(me,181477,100.0f))
				NearTeslaCoil->ResetDoorOrButton();
        }

        InstanceScript* pInstance;

        uint32 staticFieldTimer;
		bool JustSpawnedVisual;
		bool isStalaggClose;
		bool JustEnraged;
		uint32 EnrageCastTimer;

        void Reset()
        {
            if (pInstance)
                if (Creature *pThaddius = me->GetCreature(*me, pInstance->GetData64(DATA_THADDIUS)))
                    if (pThaddius->AI())
                        pThaddius->AI()->DoAction(ACTION_FEUGEN_RESET);
            staticFieldTimer = 5000;
			if(GameObject* NearTeslaCoil = GetClosestGameObjectWithEntry(me,181477,100.0f))
				NearTeslaCoil->ResetDoorOrButton();
        }

        void EnterCombat(Unit * /*pWho*/)
        {
            DoCast(SPELL_FEUGEN_TESLA);
			isStalaggClose = false;
			JustEnraged = false;
			EnrageCastTimer = 0;
        }

        void JustDied(Unit * /*killer*/)
        {
            if (pInstance)
                if (Creature *pThaddius = me->GetCreature(*me, pInstance->GetData64(DATA_THADDIUS)))
                    if (pThaddius->AI())
                        pThaddius->AI()->DoAction(ACTION_FEUGEN_DIED);

			if(GameObject* NearTeslaCoil = GetClosestGameObjectWithEntry(me,181477,100.0f))
				NearTeslaCoil->UseDoorOrButton();
        }

        void UpdateAI(const uint32 uiDiff)
        {
			if(!JustSpawnedVisual)
			{
				if(GameObject* NearTeslaCoil = GetClosestGameObjectWithEntry(me,181477,100.0f))
				{
					float gx,gy,gz;
					NearTeslaCoil->GetPosition(gx,gy,gz);
					if(Creature* TeslaVisual = me->SummonCreature(C_TESLA_COIL,gx,gy,gz,0.0f,TEMPSUMMON_DEAD_DESPAWN,0))
					{
						TeslaVisual->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_UNK_9 + UNIT_FLAG_NOT_SELECTABLE + UNIT_FLAG_NON_ATTACKABLE);
						TeslaVisual->GetMotionMaster()->MovePoint(0,gx,gy,gz);
						TeslaVisual->CastSpell(me,45537,true);
						JustSpawnedVisual = true;
					}
				}
			}

            if (!UpdateVictim())
                return;

			if (Creature *Stalagg = me->GetCreature(*me, pInstance->GetData64(DATA_STALAGG)))
			{
				if(Stalagg->IsWithinDist(me,20.0f))
					isStalaggClose = true;
				else
					isStalaggClose = false;
			}

			if( (me->GetPositionZ() < 309.58f) || isStalaggClose)
			{
				if(!JustEnraged)
				{
					JustEnraged = true;
				}
				else
				{
					if(EnrageCastTimer <= uiDiff)
					{
						me->CastSpell(me->GetVictim(),17364,true);
						EnrageCastTimer = 500;
					} else EnrageCastTimer -= uiDiff;
				}
			}
			else
				JustEnraged = false;

            if (staticFieldTimer <= uiDiff)
            {
                DoCast(me, RAID_MODE(SPELL_STATICFIELD, H_SPELL_STATICFIELD));
                staticFieldTimer = 5000;
            } else staticFieldTimer -= uiDiff;

            DoMeleeAttackIfReady();
        }
    };

};


void AddSC_boss_thaddius()
{
    new boss_thaddius();
    new mob_stalagg();
    new mob_feugen();
}
