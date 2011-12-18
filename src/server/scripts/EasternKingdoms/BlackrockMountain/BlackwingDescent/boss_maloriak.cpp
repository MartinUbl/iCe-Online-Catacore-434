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
#include "blackwing_descent.h"

/*
To DO:
Zkontrolovat spelly
Fixnout Acid novu
Zkusit efektivnìji mìnit fáze.
Dodìlat cyklus se spawny
Dodìlat efekty k bossovy . (chùze, aktivování gameobjectu)
Do green fáze zviditelnit Aberrationy a vybrat jim náhodný target.
Prosím rád bych aby se ten script ke mì ještì dostal abych ho mohl doladit.
*/

enum Texts
{
    TEXT_AGGRO           = -1999961,
    TEXT_RED_VIAL        = -1999962,
    TEXT_BLUE_VIAL       = -1999963,
    TEXT_GREEN_VIAL      = -1999964,
    TEXT_20HP            = -1999965,
    TEXT_KILL1           = -1999966,
    TEXT_KILL2           = -1999967,
    TEXT_DEATH           = -1999968,
    TEXT_DEATH2          = -1999969,
    TEXT_UNKNOWN         = -1999970, // Netuším co je to za text ale byl v dbc
    TEXT_PRIME_SUBJECT   = -1999971,
    TEXT_PRIME_SUBJECT1  = -1999972  // neznámý text zatím
};

enum Npc
{
    NPC_PRIME_SUBJECT   = 41841,
    NPC_ABERRATION      = 41440,
    NPC_WORD_TRIGER     = 22515
};

enum Spells
{
    SPELL_MAGMA_JETS             = 78194,
    SPELL_ACID_NOVA              = 93013,
    SPELL_REMEDY                 = 77912,
    SPELL_DEBILITATING_SLIME     = 77615,
    SPELL_FLASH_FREEZE           = 77699,
    //Consuming flames
    SPELL_CONSUMING_FLAMES       = 77786,
    SPELL_CONSUMING_FLAMES10HC   = 92972,
    SPELL_CONSUMING_FLAMES25N    = 92971,
    SPELL_CONSUMING_FLAMES25HC   = 92973,
    //
    SPELL_BITTING_CHILL          = 77760,
    SPELL_ARCANE_STORM           = 77896,
    // Scorching blast
    SPELL_SCORCHING_BLAST10N     = 77679,
    SPELL_SCORCHING_BLAST10HC    = 92969,
    SPELL_SCORCHING_BLAST25N     = 92968,
    SPELL_SCORCHING_BLAST25HC    = 92970,
    SPELL_RELEASE_ALL_MINION     = 77991,
    SPELL_FAKE_CAST              = 92703
};

static const Position TriggerSpawnPos[4] =
{
//levá strana
    {-106.218002f, -471.330994f, 73.453796f},//1.
    {-75.352234f, -441.131531f, 73.574654f}, //2.
    {-72.794563f, -459.245331f, 73.548965f}, //3.
    {-71.485229f, -468.842987f, 73.243889f}, //4.
};

static const Position SpawnPos[2] =
{
    {-105.692007f, -435.626007f, 73.331398f},
    {-105.692007f, -435.626007f, 73.331398f},
};

static const Position AddSpawnPos[2] =
{
    {-105.692007f, -435.626007f, 73.331398f,0.0f},
    {-105.692007f, -435.626007f, 73.331398f,0.0f},
};

const Position Kotel = {594.317f, -136.953f, 391.516998f, 4.544f};

class boss_maloriak : public CreatureScript
{
public:
    boss_maloriak() : CreatureScript("boss_maloriak") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_maloriakAI (pCreature);
    }

    struct boss_maloriakAI : public ScriptedAI
    {
        boss_maloriakAI(Creature *c) : ScriptedAI(c)
        {
            pInstance = c->GetInstanceScript();
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
            Reset();
        }
        // uint32 pro spelly
        uint32 uiDebilitatingSlime;
        uint32 uiAbsoluteZero;
        uint32 uiArcaneStorm;
        uint32 uiBitingChill;
        uint32 uiMagmaJets;
        uint32 uiReleaseAberrations;
        uint32 uiReleaseAllMinions;
        uint32 Berserk;
        uint32 uiAcid_Nova;
        uint32 uiConsuming_Flames; 
        uint32 uiFlashFreeze;
        uint32 uiRemedy;
        uint32 uiScorchingBlast;
        // uint32 pro fáze
        uint32 phase1;
        uint32 phase2;
        uint32 phase3;
        // zbytek
        uint32 _AberrationGUID;
        InstanceScript* pInstance;
        bool SummonAll;
        bool Block;
        uint8 move;
        uint8 uiphase;
        std::list<Creature*> mujList;

        void Reset()
        {
            if (pInstance)
                pInstance->SetData(DATA_MALORIAK, NOT_STARTED);
            uiMagmaJets = 10000;
            uiReleaseAberrations = 2000;
            uiDebilitatingSlime =17000;
            uiBitingChill=  27000; 
            uiFlashFreeze=  22000;
            uiScorchingBlast = 22000;
            uiConsuming_Flames = 12000; 
            uiAcid_Nova = 10000;
            uiRemedy = 14000;
            uiArcaneStorm = urand (10000,15000); // 10-15 sec od pullu
            uiphase = 0;
            phase1= 18500;
            phase2= 58500;
            phase3= 95500;
            Berserk = IsHeroic() ? 1200000 : 420000; // 7 minut
            move = 0;
            SummonAll = 0;
            _AberrationGUID = 0;
            mujList.clear();

            for (int i = 0; i < 4; i++)
            {
                Creature* mujtrigger = me->SummonCreature(NPC_WORD_TRIGER,TriggerSpawnPos[i],TEMPSUMMON_DEAD_DESPAWN, 10000);
                mujList.push_back(mujtrigger);
            }
        }

        void SpellHit(Unit* target, const SpellEntry* spell)
        {
            if (spell->Id == SPELL_FAKE_CAST)
            {
                sLog->outString("test funkce");

                for (std::list<Creature*>::iterator itr = mujList.begin(); itr != mujList.end(); ++itr)
                {
                    (*itr)->SummonCreature(NPC_ABERRATION,SpawnPos[1],TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 300000);
                }
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoScriptText(TEXT_AGGRO,me);
            if (pInstance)
                pInstance->SetData(DATA_MALORIAK, IN_PROGRESS);

            SummonAll = false;
            Block = false;
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (Berserk <= diff ) // Enrage
            {
                sLog->outString("Bersker normal");
                DoCast(me,SPELL_BERSERK2);
                Berserk = 120000; // jen neco navic, aby se berserk nespoustel furt
            } else Berserk -= diff;

            if(HealthBelowPct(25) && !SummonAll) // podmínka co se stane když boss bude mít 25%hp
            {
                Block = false;
                me->InterruptNonMeleeSpells(true);
                DoScriptText(TEXT_20HP,me);
                uiphase = 4;
                SummonAll = true;

                // 1.prime subject
                if (Creature* Spawned = me->SummonCreature(NPC_PRIME_SUBJECT,SpawnPos[1],TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000))
                    if (Unit *pTarget = SelectUnit(SELECT_TARGET_RANDOM,0))
                        if (Spawned->AI())
                            Spawned->AI()->AttackStart(pTarget);
                // 2.prime Subject
                if (Creature* Spawned = me->SummonCreature(NPC_PRIME_SUBJECT,SpawnPos[1],TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000))
                    if (Unit *pTarget = SelectUnit(SELECT_TARGET_RANDOM,0))
                        if (Spawned->AI())
                            Spawned->AI()->AttackStart(pTarget);

                DoScriptText(TEXT_PRIME_SUBJECT,me);
                //
                // tøeba vyøešit workAraundem a ne pøes spell
                DoCast(me, SPELL_RELEASE_ALL_MINION, true);
                //

                if (uiAcid_Nova <= diff )
                {
                    DoCast(me, SPELL_ACID_NOVA);
                    uiAcid_Nova = urand(10000,12000);
                } else uiAcid_Nova -= diff;

                if (uiMagmaJets <= diff)
                {
                    Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO, 0);
                    if (target && target->isAlive() && target->GetTypeId() == TYPEID_PLAYER)
                        DoCast (SPELL_MAGMA_JETS);
                    uiMagmaJets = 20000;
                } else uiMagmaJets -= diff;
            }

            // tady zaèinají normální diffy pro ostatní fáze
            if (uiReleaseAberrations <= diff )
            {
                me->InterruptNonMeleeSpells(true);
                DoCast(me, SPELL_FAKE_CAST);
                uiReleaseAberrations = 19000;
            } else uiReleaseAberrations -= diff;

            if (uiArcaneStorm <= diff)
            {
                DoCast(me,SPELL_ARCANE_STORM);
                uiArcaneStorm = urand(12*IN_MILLISECONDS,14*IN_MILLISECONDS);
            } else uiArcaneStorm -= diff;

           if (uiRemedy<= diff)
           {
               DoCast(me,SPELL_REMEDY);
               uiRemedy = urand(30*IN_MILLISECONDS,35*IN_MILLISECONDS);
           } else uiRemedy-= diff;

            if (phase1 <= diff)
            {
                me->InterruptNonMeleeSpells(true);
                DoScriptText(TEXT_RED_VIAL,me);
                uiphase = 1;
                phase1 = 120000;
            } else phase1 -= diff;

            if (uiphase == 1 && phase2 <= diff)
            {
                me->InterruptNonMeleeSpells(true);
                DoScriptText(TEXT_BLUE_VIAL,me);
                uiphase = 2;
                phase2 = 120000;
            } else phase2 -= diff;

            if (uiphase == 2 && phase3 <= diff)
            {
                me->InterruptNonMeleeSpells(true);
                DoScriptText(TEXT_GREEN_VIAL,me);
                uiphase = 3;
                phase3 = 120000;
            }else phase3 -= diff;

            if (uiphase == 1) //Red Phase 
            {
                if (uiConsuming_Flames <= diff)
                {
                    if (SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                        DoCast( SPELL_CONSUMING_FLAMES);
                    uiConsuming_Flames = 15000;
                } else uiConsuming_Flames -= diff;

                if (uiScorchingBlast <= diff)
                {
                    Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO, 0);
                    if (target && target->isAlive() && target->GetTypeId() == TYPEID_PLAYER)
                    DoCast (SPELL_SCORCHING_BLAST10N);
                    uiScorchingBlast = 20000;
                } else uiScorchingBlast -= diff;

            }

            if (uiphase == 2) //Blue Phase 
            {
                if (uiBitingChill<= diff)
                {
                    if (SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                        DoCast (SPELL_BITTING_CHILL);
                    uiBitingChill= 17000;
                } else uiBitingChill-= diff;

                if (uiFlashFreeze<= diff)
                {
                    if (SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                        DoCast (SPELL_FLASH_FREEZE);
                    uiFlashFreeze= 19000;
                } else uiFlashFreeze-= diff;

            }

            if (uiphase == 3) // GreenPhase
            {
                if (Creature* Aberration = ObjectAccessor::GetCreature(*me, _AberrationGUID))
                {
                    if (Aberration->isAlive())
                    {
                        sLog->outString("experiment");
                        Aberration->setFaction(19);
                    }
                }

                if (uiDebilitatingSlime<= diff)
                {
                    if (SelectTarget(SELECT_TARGET_RANDOM, 0,0,false))
                        DoCast (SPELL_DEBILITATING_SLIME);
                    uiDebilitatingSlime= 18000;
                } else uiDebilitatingSlime-= diff;
            }

            // definice pohybù bosse
            if(move == 1)
            {
                me->GetMotionMaster()->MovePoint(0, -106.081688f, -487.253876f, 73.456802f);
                move = 0;
            }
            if(move == 2)
            {
                me->GetMotionMaster()->MovePoint(0, -108.081688f, -487.253876f, 73.456802f);
                move = 0;
            }
            // konec
            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/)
        {
            DoScriptText(TEXT_DEATH,me);
            if (pInstance)
                pInstance->SetData(DATA_MALORIAK, DONE);
        }

        void KilledUnit(Unit * victim)
        {
            DoScriptText(RAND(TEXT_KILL1,TEXT_KILL2),me);
        }
    };
};

class npc_Prime_Subject : public CreatureScript
{
public:
    npc_Prime_Subject() : CreatureScript("npc_Prime_Subject") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_Prime_SubjectAI (pCreature);
    }

    struct npc_Prime_SubjectAI : public ScriptedAI
    {
        npc_Prime_SubjectAI(Creature *c) : ScriptedAI(c)
        {
        }
        uint32 AggroTimer;

        void Reset()
        {
            AggroTimer = 4000;
        }

        void EnterCombat(Unit * /*who*/)
        {
            DoZoneInCombat();
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;
            //timer pro aggro a taunt imunitu náhrada za spell fixate.
            if (AggroTimer <= diff)
            {
                sLog->outString("test ai prime subject");
                me->AddThreat(me,10000);
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true); 
            } else AggroTimer -= diff;
        }
    };
};

void AddSC_maloriak()
{
    new npc_Prime_Subject();
    new boss_maloriak();
}
