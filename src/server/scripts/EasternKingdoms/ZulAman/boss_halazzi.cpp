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

/* ScriptData
SDName: boss_Halazzi
SD%Complete: 80
SDComment:
SDCategory: Zul'Aman
EndScriptData */

#include "ScriptPCH.h"
#include "zulaman.h"

#define YELL_AGGRO "Get on your knees and bow to da fang and claw!"
#define SOUND_AGGRO                    12020
#define YELL_SABER_ONE "You gonna leave in pieces!"
#define YELL_SABER_TWO "Me gonna carve ya now!"
#define YELL_SPLIT "Me gonna carve ya now!"
#define SOUND_SPLIT                    12021
#define YELL_MERGE "Spirit, come back to me!"
#define SOUND_MERGE                    12022
#define YELL_KILL_ONE "You cant fight the power!"
#define SOUND_KILL_ONE                12026
#define YELL_KILL_TWO "You gonna fail!"
#define SOUND_KILL_TWO                12027
#define YELL_DEATH "Chaga... choka'jinn."
#define SOUND_DEATH                    12028
#define YELL_BERSERK "Whatch you be doing? Pissin' yourselves..."
#define SOUND_BERSERK                12025

enum Spells
{
    SPELL_DUAL_WIELD              = 29651,
    SPELL_FRENZY                  = 43139,
    SPELL_FLAMESHOCK              = 43303,
    SPELL_EARTHSHOCK              = 43305,
    SPELL_TRANSFORM_SPLIT         = 43142,
    SPELL_TRANSFORM_SPLIT2        = 43573,
    SPELL_TRANSFORM_MERGE         = 43271,

    SPELL_SUMMON_LYNX             = 43143,
    SPELL_SUMMON_LIGHTING_TOTEM   = 43302,
    SPELL_SUMMON_WATER_TOTEM      = 97499,

    SPELL_BERSERK                 = 45078,
    SPELL_LYNX_FRENZY             = 43290,
    SPELL_SHRED_ARMOR             = 43243,
    SPELL_LIGHTNING               = 43301,
    SPELL_ENRAGE                  = 22428
};

enum NPCs
{
    MOB_SPIRIT_LYNX               = 24143,
    MOB_TOTEM                     = 24224
};

enum PhaseHalazzi
{
    PHASE_NONE = 0,
    PHASE_LYNX = 1,
    PHASE_SPLIT = 2,
    PHASE_HUMAN = 3,
    PHASE_MERGE = 4,
    PHASE_ENRAGE = 5
};

class boss_halazzi : public CreatureScript
{
    public:
        boss_halazzi() : CreatureScript("boss_halazzi") { }

        struct boss_halazziAI : public ScriptedAI
        {
            boss_halazziAI(Creature *c) : ScriptedAI(c)
            {
                pInstance = c->GetInstanceScript();
                // need to find out what controls totem's spell cooldown
                SpellEntry *TempSpell = GET_SPELL(SPELL_LIGHTNING);
                if (TempSpell && TempSpell->CastingTimeIndex != 5)
                    TempSpell->CastingTimeIndex = 5; // 2000 ms casting time
            }

            InstanceScript *pInstance;

            uint32 ShockFlameTimer;
            uint32 LightingTotemTimer;
            uint32 CheckTimer;
            uint32 MergeTimer;
            uint32 BerserkTimer;
            uint32 EnrageTimer;
            uint32 WaterTotemTimer;
            uint8 TransformCount;

            PhaseHalazzi Phase;

            uint64 LynxGUID;

            void Reset()
            {
                if (pInstance)
                    pInstance->SetData(DATA_HALAZZIEVENT, NOT_STARTED);

                BerserkTimer = 600000;
                EnrageTimer = 10000;

                WaterTotemTimer = 5000;

                LightingTotemTimer = 10000;
                CheckTimer = 500;
                ShockFlameTimer = 5000;
                MergeTimer = 10000;
                TransformCount = 0;

                if (Creature* pCreature = Unit::GetCreature(*me, LynxGUID))
                    pCreature->DisappearAndDie();

                LynxGUID = 0;

                Phase = PHASE_NONE;
                EnterPhase(PHASE_NONE);
            }

            void EnterCombat(Unit * /*who*/)
            {
                if (pInstance)
                    pInstance->SetData(DATA_HALAZZIEVENT, IN_PROGRESS);

                me->MonsterYell(YELL_AGGRO, LANG_UNIVERSAL, 0);
                DoPlaySoundToSet(me, SOUND_AGGRO);

                EnterPhase(PHASE_ENRAGE);
            }

            void JustSummoned(Creature* summon)
            {
                summon->AI()->AttackStart(me->GetVictim());
                if (summon->GetEntry() == MOB_SPIRIT_LYNX)
                    LynxGUID = summon->GetGUID();
            }

            void DamageTaken(Unit * /*done_by*/, uint32 &damage)
            {
                if (damage >= me->GetHealth() && Phase != PHASE_ENRAGE)
                    damage = 0;
            }

            void SpellHit(Unit*, const SpellEntry *spell)
            {
                if (!spell)
                    return;

                if (spell->Id == SPELL_TRANSFORM_SPLIT2)
                    EnterPhase(PHASE_SPLIT);
            }

            void EnterPhase(PhaseHalazzi NextPhase)
            {
                switch(NextPhase)
                {
                case PHASE_NONE:
                    me->SetMaxHealth(4149700);
                    break;
                case PHASE_ENRAGE:
                    if (Phase == PHASE_MERGE)
                    {
                        me->SetMaxHealth(4149700);
                        if (TransformCount == 1)
                            me->SetHealth(me->GetMaxHealth()*0.60f);
                        else if (TransformCount == 2)
                            me->SetHealth(me->GetMaxHealth()*0.30f);
                        me->RemoveAurasDueToSpell(SPELL_TRANSFORM_SPLIT2);
                        DoCast(me, SPELL_TRANSFORM_MERGE, true);
                        me->Attack(me->GetVictim(), true);
                        me->GetMotionMaster()->MoveChase(me->GetVictim());
                    }
                    if (Creature *Lynx = Unit::GetCreature(*me, LynxGUID))
                    {
                        Lynx->ForcedDespawn();
                        LynxGUID = 0;
                    }
                    break;
                case PHASE_LYNX:
                    me->MonsterYell(YELL_SPLIT, LANG_UNIVERSAL, 0);
                    DoPlaySoundToSet(me, SOUND_SPLIT);
                    DoCast(me, SPELL_TRANSFORM_SPLIT, true);
                    break;
                case PHASE_MERGE:
                    if (Unit *pLynx = Unit::GetUnit(*me, LynxGUID))
                    {
                        me->MonsterYell(YELL_MERGE, LANG_UNIVERSAL, 0);
                        DoPlaySoundToSet(me, SOUND_MERGE);
                        pLynx->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        pLynx->GetMotionMaster()->Clear();
                        pLynx->GetMotionMaster()->MoveFollow(me, 0, 0);
                    }
                    break;
                default:
                    break;
                }
                Phase = NextPhase;
            }

             void UpdateAI(const uint32 diff)
             {
                if (!UpdateVictim())
                    return;

                if (BerserkTimer <= diff)
                {
                    me->MonsterYell(YELL_BERSERK, LANG_UNIVERSAL, 0);
                    DoPlaySoundToSet(me, SOUND_BERSERK);
                    DoCast(me, SPELL_BERSERK, true);
                    BerserkTimer = 60000;
                } else BerserkTimer -= diff;

                if (Phase == PHASE_ENRAGE) // done
                {
                    if (EnrageTimer <= diff)// blizzlike status: done
                    {
                        me->CastSpell(me, SPELL_ENRAGE, true);
                        me->MonsterYell("You gonna leave in pieces!", LANG_UNIVERSAL, 0);
                        me->PlayDirectSound(12024);
                        EnrageTimer = 20000;
                    } else EnrageTimer -= diff;

                    if (WaterTotemTimer <= diff) // blizzlike status: done
                    {
                        me->CastSpell(me, SPELL_SUMMON_WATER_TOTEM, false); // visual
                        WaterTotemTimer = 20000;
                    } else WaterTotemTimer -= diff;

                    if (HealthBelowPct(60) && TransformCount == 0)
                    {
                        EnterPhase(PHASE_LYNX);
                        TransformCount++;
                    }

                    if (HealthBelowPct(30) && TransformCount == 1)
                    {
                        EnterPhase(PHASE_LYNX);
                        TransformCount++;
                    }
                }

                if (Phase == PHASE_SPLIT)
                {
                    if (CheckTimer <= diff)
                    {
                        me->SetMaxHealth(me->GetMaxHealth()*0.50f);
                        me->CastSpell(me, SPELL_SUMMON_LYNX, true);
                        CheckTimer = 500;
                        EnterPhase(PHASE_HUMAN);
                    } else CheckTimer-= diff;
                }

                if (Phase == PHASE_HUMAN)
                {
                    if (LightingTotemTimer <= diff)
                    {
                        me->CastSpell(me, SPELL_SUMMON_LIGHTING_TOTEM, false);
                        LightingTotemTimer = 20000;
                    } else LightingTotemTimer -= diff;

                    if (ShockFlameTimer <= diff)
                    {
                        if (!me->IsNonMeleeSpellCasted(false))
                        {
                            if (Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0))
                            {
                                switch (urand(0,1))
                                {
                                    case 0: // flame shock
                                        me->CastSpell(pTarget, SPELL_FLAMESHOCK, false);
                                        break;
                                    case 1: // earth shock
                                        me->CastSpell(pTarget, SPELL_EARTHSHOCK, false);
                                        break;
                                }
                            }
                        }
                        ShockFlameTimer = 5000;
                    } else ShockFlameTimer -= diff;

                    if (HealthBelowPct(20))
                        EnterPhase(PHASE_MERGE);
                }

                if (Phase == PHASE_MERGE)
                {
                    if (MergeTimer <= diff)
                    {
                        EnterPhase(PHASE_ENRAGE);
                        MergeTimer = 10000;
                    } else MergeTimer -= diff;
                }

                DoMeleeAttackIfReady();
            }

            void KilledUnit(Unit* /*victim*/)
            {
                switch (urand(0,1))
                {
                    case 0:
                        me->MonsterYell(YELL_KILL_ONE, LANG_UNIVERSAL, 0);
                        DoPlaySoundToSet(me, SOUND_KILL_ONE);
                        break;

                    case 1:
                        me->MonsterYell(YELL_KILL_TWO, LANG_UNIVERSAL, 0);
                        DoPlaySoundToSet(me, SOUND_KILL_TWO);
                        break;
                }
            }

            void JustDied(Unit* /*Killer*/)
            {
                if (pInstance)
                    pInstance->SetData(DATA_HALAZZIEVENT, DONE);

                me->MonsterYell(YELL_DEATH, LANG_UNIVERSAL, 0);
                DoPlaySoundToSet(me, SOUND_DEATH);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_halazziAI(creature);
        }
};

class mob_halazzi_lynx : public CreatureScript
{
    public:

        mob_halazzi_lynx() : CreatureScript("mob_halazzi_lynx") { }

        struct mob_halazzi_lynxAI : public ScriptedAI
        {
            mob_halazzi_lynxAI(Creature *c) : ScriptedAI(c)
            {
                pInstance = c->GetInstanceScript();
            }

            InstanceScript* pInstance;
            uint32 FrenzyTimer;
            uint32 fixatetimer;
            bool beginphase;
            uint32 shredder_timer;

            void Reset()
            {
                FrenzyTimer = urand(30000, 50000);  //frenzy every 30-50 seconds
                beginphase = true;
                fixatetimer = 500;
                shredder_timer = 4000;
            }

            void DamageTaken(Unit * done_by, uint32 &damage)
            {
                if (damage >= me->GetHealth())
                    damage = 0;

                if (done_by->HasAura(97486))
                {
                    me->GetMotionMaster()->MoveChase(done_by);
                    me->AI()->AttackStart(done_by);
                }
            }

            void EnterCombat(Unit* who)
            {
            }

            void SpellHitTarget(Unit* pTarget, const SpellEntry* pSpell)
            {
                if (!pTarget || !pSpell)
                    return;

                if (pSpell->Id == 97486)
                {
                    me->GetMotionMaster()->MoveChase(pTarget);
                    me->AI()->AttackStart(pTarget);
                }

            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

               if (fixatetimer <= diff)
               {
                    if (Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0))
                        me->CastSpell(pTarget, 97486, false);
                    fixatetimer = 6000;
               } else fixatetimer -= diff;

                if (FrenzyTimer <= diff)
                {
                    DoCast(me, SPELL_LYNX_FRENZY);
                    FrenzyTimer = urand(30000, 50000);  //frenzy every 30-50 seconds
                } else FrenzyTimer -= diff;

                if (shredder_timer <= diff)
                {
                    DoCast(me->GetVictim(), SPELL_SHRED_ARMOR);
                    shredder_timer = 4000;
                } else shredder_timer -= diff;

                if (HealthBelowPct(20) && beginphase)
                {
                    if (pInstance)
                    {
                        if (Creature* pBoss = pInstance->instance->GetCreature(pInstance->GetData64(DATA_HALAZZIEVENT)))
                            CAST_AI(boss_halazzi::boss_halazziAI, pBoss->AI())->EnterPhase(PHASE_MERGE);
                        beginphase = false;
                    }
                }

                DoMeleeAttackIfReady();
            }

        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_halazzi_lynxAI(creature);
        }
};

class npc_halazzi_water_totem: public CreatureScript
{
   public:
       npc_halazzi_water_totem(): CreatureScript("npc_halazzi_water_totem") {};

       struct npc_halazzi_water_totemAI: public ScriptedAI
       {
           npc_halazzi_water_totemAI(Creature* c): ScriptedAI(c)
           {
                c->CastSpell(c, 97502, false);
                c->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NON_ATTACKABLE);
           }

            void Reset()
            {

            }

            void EnterCombat(Unit* /*target*/) { }

            void DamageTaken(Unit * /*done_by*/, uint32 &damage)
            {
                damage = 0;
            }


           void UpdateAI(const uint32 diff)
           {
           }
       };

       CreatureAI* GetAI(Creature* c) const
       {
           return new npc_halazzi_water_totemAI(c);
       }
};

void AddSC_boss_halazzi()
{
    new boss_halazzi();
    new mob_halazzi_lynx();
    new npc_halazzi_water_totem();
}
