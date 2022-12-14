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
#include "utgarde_pinnacle.h"

enum Spells
{
    SPELL_CALL_FLAMES                             = 48258,
    SPELL_RITUAL_OF_THE_SWORD                     = 48276, //Effect #1 Teleport,  Effect #2 Dummy
    SPELL_SINSTER_STRIKE                          = 15667,
    H_SPELL_SINSTER_STRIKE                        = 59409,
    SPELL_SVALA_TRANSFORMING1                     = 54140,
    SPELL_SVALA_TRANSFORMING2                     = 54205
};
//not in db
enum Yells
{
    SAY_DIALOG_WITH_ARTHAS_1                      = -1575000,
    SAY_DIALOG_WITH_ARTHAS_2                      = -1575002,
    SAY_DIALOG_WITH_ARTHAS_3                      = -1575004,
    SAY_AGGRO                                     = -1575005,
    SAY_SLAY_1                                    = -1575006,
    SAY_SLAY_2                                    = -1575007,
    SAY_SLAY_3                                    = -1575008,
    SAY_DEATH                                     = -1575014,
    SAY_SACRIFICE_PLAYER_1                        = -1575009,
    SAY_SACRIFICE_PLAYER_2                        = -1575010,
    SAY_SACRIFICE_PLAYER_3                        = -1575011,
    SAY_SACRIFICE_PLAYER_4                        = -1575012,
    SAY_SACRIFICE_PLAYER_5                        = -1575013,
    SAY_DIALOG_OF_ARTHAS_1                        = -1575001,
    SAY_DIALOG_OF_ARTHAS_2                        = -1575003
};
enum Creatures
{
    CREATURE_ARTHAS                               = 24266, // Image of Arthas
    CREATURE_SVALA_SORROWGRAVE                    = 26668, // Svala after transformation
    CREATURE_SVALA                                = 29281, // Svala before transformation
    CREATURE_RITUAL_CHANNELER                     = 27281
};
enum ChannelerSpells
{
    //ritual channeler's spells
    SPELL_PARALYZE                                = 48278,
    SPELL_SHADOWS_IN_THE_DARK                     = 59407
};
enum Misc
{
    DATA_SVALA_DISPLAY_ID                         = 25944
};
enum IntroPhase
{
    IDLE,
    INTRO,
    FINISHED
};
enum CombatPhase
{
    NORMAL,
    SACRIFICING
};

static Position RitualChannelerPos[]=
{
    {296.42f, -355.01f, 90.94f, 0.0f},
    {302.36f, -352.01f, 90.54f, 0.0f},
    {291.39f, -350.89f, 90.54f, 0.0f}
};
static Position ArthasPos = { 295.81f, -366.16f, 92.57f, 1.58f };
static Position SvalaPos = { 296.632f, -346.075f, 90.6307f, 1.58f };

class boss_svala : public CreatureScript
{
public:
    boss_svala() : CreatureScript("boss_svala") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_svalaAI (pCreature);
    }

    struct boss_svalaAI : public ScriptedAI
    {
        boss_svalaAI(Creature *c) : ScriptedAI(c)
        {
            pInstance = c->GetInstanceScript();
        }

        uint32 uiIntroTimer;

        uint8 uiIntroPhase;

        IntroPhase Phase;

        TempSummon* pArthas;
        uint64 uiArthasGUID;

        InstanceScript* pInstance;

        void Reset()
        {
            Phase = IDLE;
            uiIntroTimer = 1 * IN_MILLISECONDS;
            uiIntroPhase = 0;
            uiArthasGUID = 0;

            if (pInstance)
                pInstance->SetData(DATA_SVALA_SORROWGRAVE_EVENT, NOT_STARTED);
        }

        void MoveInLineOfSight(Unit* pWho)
        {
            if (!pWho)
                return;

            if (Phase == IDLE && pWho->isTargetableForAttack() && me->IsHostileTo(pWho) && me->IsWithinDistInMap(pWho, 40))
            {
                Phase = INTRO;
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

                if (Creature *pArthas = me->SummonCreature(CREATURE_ARTHAS, ArthasPos, TEMPSUMMON_MANUAL_DESPAWN))
                {
                    pArthas->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);
                    pArthas->SetFloatValue(OBJECT_FIELD_SCALE_X, 9);
                    pArthas->CastSpell(pArthas, 54134, true);
                    uiArthasGUID = pArthas->GetGUID();
                }
            }
        }

        void AttackStart(Unit* /*who*/) {}

		void DoMoveToPosition()
	    {
		    float fX, fZ, fY;
	        me->GetRespawnCoord(fX, fY, fZ);

	        //me->AddSplineFlag(SPLINEFLAG_FLYING);
			me->AddUnitMovementFlag(MOVEMENTFLAG_FLYING | MOVEMENTFLAG_LEVITATING);

			//me->SendMonsterMoveWithSpeed(fX, fY, fZ + 5.0f, 6000);
			me->MonsterMoveWithSpeed(fX,fY,fZ+5.0f,1.0f);
		    me->GetMap()->CreatureRelocation(me, fX, fY, fZ + 5.0f, me->GetOrientation());
	    }

        void UpdateAI(const uint32 diff)
        {
            if (Phase != INTRO)
                return;

            if (uiIntroTimer <= diff)
            {
                Creature *pArthas = Unit::GetCreature(*me, uiArthasGUID);
                if (!pArthas)
                    return;

                switch (uiIntroPhase)
                {
                    case 0:
                        DoScriptText(SAY_DIALOG_WITH_ARTHAS_1, me);
                        ++uiIntroPhase;
                        uiIntroTimer = 3500;
                        break;
                    case 1:
                        DoScriptText(SAY_DIALOG_OF_ARTHAS_1, pArthas);
                        ++uiIntroPhase;
                        uiIntroTimer = 3500;
                        break;
                    case 2:
                        DoScriptText(SAY_DIALOG_WITH_ARTHAS_2, me);
                        ++uiIntroPhase;
                        uiIntroTimer = 3500;
                        break;
                    case 3:
                        DoScriptText(SAY_DIALOG_OF_ARTHAS_2, pArthas);
                        ++uiIntroPhase;
                        uiIntroTimer = 3500;
                        break;
                    case 4:
                        DoScriptText(SAY_DIALOG_WITH_ARTHAS_3, me);
						pArthas->CastSpell(me, 54142, false);
                        DoCast(me, SPELL_SVALA_TRANSFORMING1);
						DoMoveToPosition();
                        ++uiIntroPhase;
                        uiIntroTimer = 6000;//2800;
                        break;
                    case 5:
                        DoCast(me, SPELL_SVALA_TRANSFORMING2);
                        ++uiIntroPhase;
                        uiIntroTimer = 200;
                        break;
                    case 6:
                        if (Creature* pSorrow = me->SummonCreature(CREATURE_SVALA_SORROWGRAVE, SvalaPos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60*IN_MILLISECONDS))
                        {
                            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                            me->SetDisplayId(DATA_SVALA_DISPLAY_ID);
							me->SetVisible(false);
							me->SetReactState(REACT_PASSIVE);
							pSorrow->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);
                            pArthas->ToTempSummon()->UnSummon();
                            uiArthasGUID = 0;
                            Phase = FINISHED;
                        }
                        else
                            Reset();
                        break;
                }
            } else uiIntroTimer -= diff;
        }
    };

};

class mob_ritual_channeler : public CreatureScript
{
public:
    mob_ritual_channeler() : CreatureScript("mob_ritual_channeler") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_ritual_channelerAI(pCreature);
    }

    struct mob_ritual_channelerAI : public Scripted_NoMovementAI
    {
        mob_ritual_channelerAI(Creature *c) :Scripted_NoMovementAI(c)
        {
            pInstance = c->GetInstanceScript();
        }

        InstanceScript* pInstance;

        void Reset()
        {
            DoCast(me, SPELL_SHADOWS_IN_THE_DARK);
        }

        // called by svala sorrowgrave to set guid of victim
        void DoAction(const int32 /*action*/)
        {
            if (pInstance)
                if (Unit *pVictim = me->GetUnit(*me, pInstance->GetData64(DATA_SACRIFICED_PLAYER)))
                    DoCast(pVictim, SPELL_PARALYZE);
        }

        void EnterCombat(Unit* /*who*/)
        {
        }
    };

};

class boss_svala_sorrowgrave : public CreatureScript
{
public:
    boss_svala_sorrowgrave() : CreatureScript("boss_svala_sorrowgrave") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_svala_sorrowgraveAI(pCreature);
    }

    struct boss_svala_sorrowgraveAI : public ScriptedAI
    {
        boss_svala_sorrowgraveAI(Creature *c) : ScriptedAI(c), summons(c)
        {
            pInstance = c->GetInstanceScript();
        }

        uint32 uiSinsterStrikeTimer;
        uint32 uiCallFlamesTimer;
        uint32 uiRitualOfSwordTimer;
        uint32 uiSacrificeTimer;

        CombatPhase Phase;

        SummonList summons;

        bool bSacrificed;

        InstanceScript* pInstance;

        void Reset()
        {
            uiSinsterStrikeTimer = 7 * IN_MILLISECONDS;
            uiCallFlamesTimer = 10 * IN_MILLISECONDS;
            uiRitualOfSwordTimer = 20 * IN_MILLISECONDS;
            uiSacrificeTimer = 8 * IN_MILLISECONDS;

            bSacrificed = false;

            Phase = NORMAL;

            DoTeleportTo(296.632f, -346.075f, 90.6307f);
            me->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);

            summons.DespawnAll();

            if (pInstance)
            {
                pInstance->SetData(DATA_SVALA_SORROWGRAVE_EVENT, NOT_STARTED);
                pInstance->SetData64(DATA_SACRIFICED_PLAYER,0);
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoScriptText(SAY_AGGRO, me);

            if (pInstance)
                pInstance->SetData(DATA_SVALA_SORROWGRAVE_EVENT, IN_PROGRESS);
        }

        void JustSummoned(Creature *summon)
        {
            summons.Summon(summon);
        }

        void SummonedCreatureDespawn(Creature *summon)
        {
            summons.Despawn(summon);
        }

        void UpdateAI(const uint32 diff)
        {
            if (Phase == NORMAL)
            {
                //Return since we have no target
                if (!UpdateVictim())
                    return;

                if (uiSinsterStrikeTimer <= diff)
                {
                    DoCast(me->GetVictim(), SPELL_SINSTER_STRIKE);
                    uiSinsterStrikeTimer = urand(5 * IN_MILLISECONDS, 9 * IN_MILLISECONDS);
                } else uiSinsterStrikeTimer -= diff;

                if (uiCallFlamesTimer <= diff)
                {
                    if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                    {
                        DoCast(pTarget, pInstance->instance->IsRegularDifficulty()?46551:20203, true);
                        uiCallFlamesTimer = urand(8*IN_MILLISECONDS,12*IN_MILLISECONDS);
                    }
                } else uiCallFlamesTimer -= diff;

                if (!bSacrificed)
                {
                    if (uiRitualOfSwordTimer <= diff)
                    {
                        if (Unit* pSacrificeTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                        {
                            DoScriptText(RAND(SAY_SACRIFICE_PLAYER_1, SAY_SACRIFICE_PLAYER_2, SAY_SACRIFICE_PLAYER_3, SAY_SACRIFICE_PLAYER_4, SAY_SACRIFICE_PLAYER_5), me);
                            DoCast(pSacrificeTarget, SPELL_RITUAL_OF_THE_SWORD);
                            //Spell doesn't teleport
                            DoTeleportPlayer(pSacrificeTarget, 296.632f, -346.075f, 90.63f, 4.6f);
                            me->SetUnitMovementFlags(MOVEMENTFLAG_CAN_FLY);
                            DoTeleportTo(296.632f, -346.075f, 120.85f);
                            Phase = SACRIFICING;
                            if (pInstance)
                            {
                                pInstance->SetData64(DATA_SACRIFICED_PLAYER,pSacrificeTarget->GetGUID());

                                for (uint8 i = 0; i < 3; ++i)
                                    if (Creature* pSummon = me->SummonCreature(CREATURE_RITUAL_CHANNELER, RitualChannelerPos[i], TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 360000))
                                        pSummon->AI()->DoAction(0);
                            }

                            bSacrificed = true;
                        }
                    } else uiRitualOfSwordTimer -= diff;
                }

                DoMeleeAttackIfReady();
            }
            else  //SACRIFICING
            {
                if (uiSacrificeTimer <= diff)
                {
                    Unit* pSacrificeTarget = pInstance ? Unit::GetUnit(*me, pInstance->GetData64(DATA_SACRIFICED_PLAYER)) : NULL;
                    if (pInstance && !summons.empty() && pSacrificeTarget && pSacrificeTarget->IsAlive())
                        me->Kill(pSacrificeTarget, false); // durability damage?

                    //go down
                    Phase = NORMAL;
                    pSacrificeTarget = NULL;
                    me->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);
                    if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                        me->GetMotionMaster()->MoveChase(pTarget);

                    uiSacrificeTimer = 8 * IN_MILLISECONDS;
                }
                else uiSacrificeTimer -= diff;
            }
        }

        void KilledUnit(Unit* /*pVictim*/)
        {
            DoScriptText(RAND(SAY_SLAY_1, SAY_SLAY_2, SAY_SLAY_3), me);
        }

        void JustDied(Unit* pKiller)
        {
            if (pInstance)
            {
                Creature* pSvala = Unit::GetCreature((*me), pInstance->GetData64(DATA_SVALA));
                if (pSvala && pSvala->IsAlive())
                    pKiller->Kill(pSvala);

                pInstance->SetData(DATA_SVALA_SORROWGRAVE_EVENT, DONE);
            }
            DoScriptText(SAY_DEATH, me);
        }
    };

};

void AddSC_boss_svala()
{
    new boss_svala();
    new mob_ritual_channeler();
    new boss_svala_sorrowgrave();
}
