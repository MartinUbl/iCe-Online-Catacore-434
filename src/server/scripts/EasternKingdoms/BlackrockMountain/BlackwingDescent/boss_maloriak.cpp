#include "ScriptPCH.h"
#include "Spell.h"
#include "blackwing_descent.h"

/*
Authors: HyN3'Q, Gregory, Labuz

*/
enum Texts
{
    TEXT_AGGRO                  = -1999961,
    TEXT_RED_VIAL               = -1999962,
    TEXT_BLUE_VIAL              = -1999963,
    TEXT_GREEN_VIAL             = -1999964,
    TEXT_20HP                   = -1999965,
    TEXT_KILL1                  = -1999966,
    TEXT_KILL2                  = -1999967,
    TEXT_DEATH                  = -1999968,
    TEXT_DEATH2                 = -1999969,
    TEXT_UNKNOWN                = -1999970,
    TEXT_PRIME_SUBJECT          = -1999971,
    TEXT_PRIME_SUBJECT1         = -1999972,
    TEXT_EMOTE_RED              = -1999973,
    TEXT_EMOTE_BLUE             = -1999974,
    TEXT_EMOTE_GREEN            = -1999975,
    TEXT_EMOTE_DARK             = -1999976,
};

enum Phase
{
    PHASE_A                     = 1,
    PHASE_B                     = 2,
};

enum Action
{
    RECORD_DEATH                = 1,
    CHECK_COUNTER               = 2,
    ACTION_INTRO                = 3,
    CHECK_AURA_COUNT            = 4,
};

enum Npc
{
    NPC_PRIME_SUBJECT           = 41841,
    NPC_ABERRATION              = 41440,
    NPC_WORD_TRIGER             = 22515,
    NPC_FLASH_FREEZE            = 41576,
    NPC_ABSOLUTE_ZERO           = 41961,
    NPC_CAULDRON_TRIGGER        = 41505,
    NPC_LORD_VOICE_TRIGGER      = 415050,
    NPC_LORD_VICTOR_NEFARIUS    = 49799,
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
    SPELL_RELLEASE_ABERRATION    = 77569,
    SPELL_ABSOLUTE_ZERO          = 78223,
    SPELL_ABSOLUTE_ZERO_AURA     = 78201,
    SPELL_ABSOLUTE_ZERO_DMG      = 78208,
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

class npc_absolute_zero : public CreatureScript
{
public:
    npc_absolute_zero() : CreatureScript("npc_absolute_zero") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_absolute_zeroAI(pCreature);
    }

    struct npc_absolute_zeroAI : public ScriptedAI
    {
        npc_absolute_zeroAI(Creature *c) : ScriptedAI(c)
        {
        }
            uint32 uiPauseTimer; 
            uint32 uiDespawnTimer;
            bool CanExplode;
 
            void Reset()
            {
                uiPauseTimer = 3000;
                uiDespawnTimer = 15000;
                CanExplode = false;
            }

            void IsSummonedBy(Unit* owner)
            {
                DoCast(SPELL_ABSOLUTE_ZERO_AURA);
            }

            void UpdateAI(const uint32 diff)
            {
                if (!CanExplode)
                {
                    if(uiPauseTimer <= diff)
                    {
                        CanExplode = true;
                        if (Unit* target = me->SelectNearestTarget())
                        {
                            me->AddThreat(target, 100000.0f);
                            me->GetMotionMaster()->MoveFollow(target, 0.1f, 0.0f);
                        }
                    }
                    else
                        uiPauseTimer -= diff;
                }

                if (uiDespawnTimer <= diff)
                    me->DespawnOrUnsummon();
                else
                    uiDespawnTimer -= diff;

                if (Unit* target = me->SelectNearestTarget())
                {
                    if ((me->GetDistance(target) <= 4.0f) && CanExplode)
                    {
                        DoCast(SPELL_ABSOLUTE_ZERO_DMG);
                        me->DespawnOrUnsummon();
                    }
                }

                if(!UpdateVictim())
                    return;
        }
    };
};

class npc_flash_freeze : public CreatureScript
{
public:
    npc_flash_freeze() : CreatureScript("npc_flash_freeze") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_flash_freezeAI(pCreature);
    }

    struct npc_flash_freezeAI : public ScriptedAI
    {
        npc_flash_freezeAI(Creature *c) : ScriptedAI(c)
        {
            FlashFreezeGUID = 0;
        }

        uint64 FlashFreezeGUID;

        void SetPrison(Unit* uPrisoner)
        {
            FlashFreezeGUID = uPrisoner->GetGUID();
        }

        void Reset()
        { 
            FlashFreezeGUID = 0;
        }

        void EnterCombat(Unit* /*who*/) { }
        void AttackStart(Unit* /*who*/) { }
        void MoveInLineOfSight(Unit* /*who*/) { }

        void JustDied(Unit *killer)
        {
            if (FlashFreezeGUID)
            {
                Unit* FlashFreeze = Unit::GetUnit((*me),FlashFreezeGUID);
                if (FlashFreeze)
                    FlashFreeze->RemoveAurasDueToSpell(SPELL_FLASH_FREEZE);
            }
        }

        void UpdateAI(const uint32 /*diff*/) { }
    };

};

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
        uint32 uiDarkSludge;
        uint32 uiEngulfingDarkness;	
        uint32 uiVilleSwill;
        // uint32 pro faze
        uint32 uiSwitchPhaseTimer;
        uint32 uiSubPhaseTimer;
        uint32 phase1;
        uint32 phase2;
        uint32 phase3;
        // zbytek
        uint32 _AberrationGUID;
        uint32 Poradi;
        uint32 CastTimer;
        uint32 phase;
        uint32 Phase;
        uint32 SubPhase;
        uint32 subphase;
        uint32 diff;
        uint32 HeroicIntro;
        InstanceScript* pInstance;
        bool isEnraged;
        bool SummonAll;
        bool Block;
        bool PhaseOne;
        bool PhaseTwo;
        bool LastPhase;
        bool StopSlime;
        bool Jump;
        bool AwardAchiev;
        bool Intro;
        bool IntroDone;
        bool can_interrupt;
        uint8 move;
        uint8 uiphase;
        std::list<Creature*> mujList;
        GOState Blue;
        GOState Red;
        std::list<uint32> AberrationDeathTimeStamp;
        int Counter;
        int AberrationAchievementCounter;
        int SpellCounter;

        void Reset()
        {
            OpenCell();
            if (pInstance)
                pInstance->SetData(DATA_MALORIAK, NOT_STARTED);
            uiRemedy = 14000;
            uiphase = 0;
            Berserk = IsHeroic() ? 1200000 : 420000; // 7 minut
            move = 0;
            phase = 0;
            subphase = 0;
            SummonAll = 0;
            Poradi = 0;
            Counter = 0;
            SpellCounter = 0;
            _AberrationGUID = 0;
            mujList.clear();
            isEnraged = false;
            PhaseOne = false;
            PhaseTwo = false;
            LastPhase = false;
            StopSlime = false;
            AwardAchiev = false;
            IntroDone = false;
            Jump = false;
            can_interrupt = false;

        }

        void CloseCell()
        {
            if(GameObject* Cell= me->FindNearestGameObject(191722,100.0f))
            {
                Cell->SetGoState(GO_STATE_READY);
            }
        
        }
        
        void OpenCell()
        {
            if(GameObject* Cell= me->FindNearestGameObject(191722,100.0f))
            {
                Cell->SetGoState(GO_STATE_ACTIVE);
            }
        }

        void SpellHit(Unit* target, const SpellEntry* spell)
        {
            if(!spell)
                return;

            for(uint8 i = 0; i < 3; i++) {
                if(spell->Effect[i] == SPELL_EFFECT_INTERRUPT_CAST)
                {
                    if(can_interrupt)
                    {
                        me->InterruptNonMeleeSpells(true);
                        break;
                    }
                }
            }

            if (spell->Id == SPELL_RELLEASE_ABERRATION)
            {
                SpellCounter++;
                
                if(SpellCounter <= 6)
                {
                    switch(urand(0,1))
                    {
                        case 0:
                        {
                             for (uint32 i = 0; i < 3; i++)
                             {
                                 me->SummonCreature(NPC_ABERRATION,-77.633163f,-444.687256f,73.449394f,0.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                             }
                        }
                        break;
                        case 1:
                        {
                             for (uint32 i = 0; i < 3; i++)
                             {
                                me->SummonCreature(NPC_ABERRATION,-134.706467f,-444.746613f,73.447304f,0.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                             }
                        }
                        break;
                     }
                 }
            }
        }

        void FindPlayers()
        {
            float radius = 13.0f;
            std::list<Player*> players;
            Trinity::AnyPlayerInObjectRangeCheck checker(me, radius);
            Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(me, players, checker);
            me->VisitNearbyWorldObject(radius, searcher);
            for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
            {
                (*itr)->GetMotionMaster()->MoveJump(-102.292519f,-439.359955f,73.534279f,20.0f,20.0f);
            }
        }
        
 

        void ReleaseAll()
        {
           if (SpellCounter == 0)
           {
                switch(urand(0,1))
                {
                    case 0:
                    {
                        for (uint32 i = 0; i < 18; i++)
                        {
                            me->SummonCreature(NPC_ABERRATION,-77.633163f,-444.687256f,73.449394f,0.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                        }
                    }
                    break;
                    case 1:
                    {
                        for (uint32 i = 0; i < 18; i++)
                        {
                            me->SummonCreature(NPC_ABERRATION,-134.706467f,-444.746613f,73.447304f,0.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                        }
                    }
                    break;
                } 
           }
           else if(SpellCounter == 1)
           {
                switch(urand(0,1))
                {
                    case 0:
                    {
                        for (uint32 i = 0; i < 15; i++)
                        {
                            me->SummonCreature(NPC_ABERRATION,-77.633163f,-444.687256f,73.449394f,0.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                        }
                    }
                    break;
                    case 1:
                    {
                        for (uint32 i = 0; i < 15; i++)
                        {
                            me->SummonCreature(NPC_ABERRATION,-134.706467f,-444.746613f,73.447304f,0.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                        }
                    }
                    break;
                }                    
           }
           else if(SpellCounter == 2)
           {
                switch(urand(0,1))
                {
                    case 0:
                    {
                        for (uint32 i = 0; i < 12; i++)
                        {
                            me->SummonCreature(NPC_ABERRATION,-77.633163f,-444.687256f,73.449394f,0.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                        }
                    }
                    break;
                    case 1:
                    {
                        for (uint32 i = 0; i < 12; i++)
                        {
                            me->SummonCreature(NPC_ABERRATION,-134.706467f,-444.746613f,73.447304f,0.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                        }
                    }
                    break;
                }       
           }
           else if(SpellCounter == 3)
           {
                switch(urand(0,1))
                {
                    case 0:
                    {
                        for (uint32 i = 0; i < 9; i++)
                        {
                            me->SummonCreature(NPC_ABERRATION,-77.633163f,-444.687256f,73.449394f,0.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                        }
                    }
                    break;
                    case 1:
                    {
                        for (uint32 i = 0; i < 9; i++)
                        {
                            me->SummonCreature(NPC_ABERRATION,-134.706467f,-444.746613f,73.447304f,0.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                        }
                    }
                    break;
                }       
           }
           else if(SpellCounter == 4)
           {    
                 switch(urand(0,1))
                {
                    case 0:
                    {
                        for (uint32 i = 0; i < 6; i++)
                        {
                            me->SummonCreature(NPC_ABERRATION,-77.633163f,-444.687256f,73.449394f,0.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                        }
                    }
                    break;
                    case 1:
                    {
                        for (uint32 i = 0; i < 6; i++)
                        {
                            me->SummonCreature(NPC_ABERRATION,-134.706467f,-444.746613f,73.447304f,0.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                        }
                    }
                    break;
                }       
           }
           else if(SpellCounter == 5)
           {
                switch(urand(0,1))
                {
                    case 0:
                    {
                        for (uint32 i = 0; i < 3; i++)
                        {
                            me->SummonCreature(NPC_ABERRATION,-77.633163f,-444.687256f,73.449394f,0.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                        }
                    }
                    break;
                    case 1:
                    {
                        for (uint32 i = 0; i < 3; i++)
                        {
                            me->SummonCreature(NPC_ABERRATION,-134.706467f,-444.746613f,73.447304f,0.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                        }
                    }
                    break;
                }
           }
           else
                return;
           
        }

        void EnterCombat(Unit* /*who*/)
        {
           if(IsHeroic())
           {
                DoScriptText(TEXT_AGGRO,me);
                HeroicIntro = 8000;
           }
           else
                DoScriptText(TEXT_AGGRO,me);
            if (pInstance)
                pInstance->SetData(DATA_MALORIAK, IN_PROGRESS);
            SummonAll = false;
            Block = false;
            uiSubPhaseTimer = 1000;
            uiSwitchPhaseTimer = 15000;
            Phase = 0;
            SubPhase = 0;
            CloseCell();
        }
        
        void DoAction(const int Action)
        {
            switch (Action)
            {
                case RECORD_DEATH:
                {
                    AberrationDeathTimeStamp.push_back(getMSTime());
                    Counter++;
                }
                break;
                case CHECK_COUNTER:
                {
                    if (Counter >= 12)
                    {
                        uint32 m_uiTime = getMSTime();
                        Counter = 0;
                        for(std::list<uint32>::const_iterator itr = AberrationDeathTimeStamp.begin(); itr != AberrationDeathTimeStamp.end(); ++itr)
                        {
                            if (*itr >= (m_uiTime - 12000))
                            Counter++;
                        }
                        if (Counter == 12)
                            AwardAchiev = true;
                    }
                }
                break;
            }
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type != POINT_MOTION_TYPE)
                return;
 
            switch (id)
            {
                case 1:
                {
                     me->GetMotionMaster()->MoveChase(me->getVictim());
                     DoScriptText(TEXT_RED_VIAL,me);
                }
                break;
                case 2:
                {
                     me->GetMotionMaster()->MoveChase(me->getVictim());
                     DoScriptText(TEXT_BLUE_VIAL,me);
                }
                break;
                case 3:
                {
                     FindPlayers();
                     Jump = true;
                     DoScriptText(TEXT_GREEN_VIAL,me);
                }
                break;
                case 4:
                {
                    me->GetMotionMaster()->MoveChase(me->getVictim());
                    uiEngulfingDarkness = 2000;
                    uiVilleSwill = 4000;
                    if(Creature* Lord = me->FindNearestCreature(NPC_LORD_VOICE_TRIGGER,250.0f,true))
                    {
                        Lord->MonsterYell(" Your mixtures are weak, Maloriak! They need a bit more... kick!",0,0);
                        Lord->PlayDirectSound(23370,0);
                    }
                }
                break;
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;
                
            if (!IntroDone)
            {
                if (HeroicIntro <= diff)
                {
                    IntroDone = true;
                    if(Creature* Lord = me->FindNearestCreature(NPC_LORD_VOICE_TRIGGER,250.0f,true))
                    {
                        Lord->MonsterYell("Maloriak, try not to lose to these mortals! Semi-competent help is SO hard to create.",0,0);
                        Lord->PlayDirectSound(23372,0);
                    }
                }
                else
                    HeroicIntro -= diff;
            }
            
            if (Jump == true)
            {
                Jump = false;
                me->GetMotionMaster()->MoveJump(-102.292519f,-439.359955f,73.534279f,20.0f,20.0f);
                me->GetMotionMaster()->MoveChase(me->getVictim());
                if(Creature* Cauldron = me->FindNearestCreature(NPC_CAULDRON_TRIGGER,250.0f,true))
                {
                    Cauldron->CastSpell(Cauldron,77602,false);
                }
            }

            if (!isEnraged)
            {
                if (Berserk <= diff ) // Enrage
                {
                    DoCast(me,64238);
                    isEnraged = true;
                } else Berserk -= diff;
            }

            if(HealthBelowPct(25) && !SummonAll) // podminka co se stane kdyz boss bude mit 25%hp
            {
                LastPhase = true;
                me->InterruptNonMeleeSpells(true);
                DoScriptText(TEXT_20HP,me);
                uiphase = 4;
                SummonAll = true;
                uiAbsoluteZero = 15000;
                uiMagmaJets = 20000;
                uiAcid_Nova = 10000;
                uiRemedy = 14000;
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
                ReleaseAll();
            }

           if (!LastPhase && uiRemedy<= diff)
           {
               DoCast(me,SPELL_REMEDY);
               uiRemedy = urand(30*IN_MILLISECONDS,35*IN_MILLISECONDS);
           } else uiRemedy-= diff;

            if (!LastPhase && uiSwitchPhaseTimer <= diff)
            {
                if(IsHeroic())
                {
                    switch (Phase)
                    {
                        case 0:
                            Phase = 1;
                            break;
                        case 1:
                            Phase = 2;
                            uiSwitchPhaseTimer = 192000;
                            PhaseOne = true;
                            PhaseTwo = false;
                            break;
                        case 2:
                            Phase = 0;
                            uiSwitchPhaseTimer = 192000;
                            PhaseOne = false;
                            PhaseTwo = true;
                            break;
                    }
                }
                else
                {
                    switch (Phase)
                    {
                        case 0:
                            Phase = 1;
                            break;
                        case 1:
                            Phase = 2;
                            uiSwitchPhaseTimer = 120000;
                            PhaseOne = true;
                            PhaseTwo = false;
                        break;
                        case 2:
                            Phase = 0;
                            uiSwitchPhaseTimer = 120000;
                            PhaseOne = false;
                            PhaseTwo = true;
                        break;
                    }
                }
            }else
                uiSwitchPhaseTimer -= diff;

            if (!LastPhase && PhaseOne == true)
            {
                if(uiSubPhaseTimer <= diff)
                {
                    if (IsHeroic())
                    {
                        switch(SubPhase)
                        {
                            case 0:
                                SubPhase = 1;
                                break;
                            case 1:
                                SubPhase = 2;
                                uiphase = 5;
                                uiSubPhaseTimer = 72000;
                                me->TextEmote(TEXT_EMOTE_DARK,0,true);
                                me->GetMotionMaster()->MovePoint(4,-105.134102f,-482.974426f,73.456650f);
                                me->AddAura(92716,me);
                                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true); 
                            break;
                            case 2:
                                SubPhase = 3;
                                uiphase = 1;
                                uiSubPhaseTimer = 50000;
                                me->TextEmote(TEXT_EMOTE_RED,0,true);
                                me->GetMotionMaster()->MovePoint(1,-105.134102f,-482.974426f,73.456650f);
                                me->RemoveAurasDueToSpell(92716);
                                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, false);
                                CastTimer = 5000;
                                Poradi = 0;
                            break;
                            case 3:
                                SubPhase = 4;
                                uiphase = 2;
                                uiSubPhaseTimer = 50000;
                                me->TextEmote(TEXT_EMOTE_BLUE,0,true);
                                me->GetMotionMaster()->MovePoint(2,-105.134102f,-482.974426f,73.456650f);
                                CastTimer = 5000;
                                Poradi = 0;
                            break;
                            case 4:
                                SubPhase = 0;
                                uiphase = 3;
                                uiSubPhaseTimer = 50000;
                                StopSlime = false;
                                me->GetMotionMaster()->MovePoint(3,-105.134102f,-482.974426f,73.456650f);
                                me->TextEmote(TEXT_EMOTE_GREEN,0,true);
                            break;
                        }
                    }
                    else // !IsHeroic()
                    {
                        switch(SubPhase)
                        {
                            case 0:
                                SubPhase = 1;
                            break;
                            case 1:
                                CastTimer = 5000;
                                Poradi = 0;
                                SubPhase = 2;
                                uiphase = 1;
                                uiSubPhaseTimer = 40000;
                                me->TextEmote(TEXT_EMOTE_RED,0,true);
                                me->GetMotionMaster()->MovePoint(1,-105.134102f,-482.974426f,73.456650f);
                                break;
                            case 2:
                                CastTimer = 5000;
                                Poradi = 0;
                                SubPhase = 3;
                                uiphase = 2;
                                uiSubPhaseTimer = 40000;
                                me->TextEmote(TEXT_EMOTE_BLUE,0,true);
                                me->GetMotionMaster()->MovePoint(2,-105.134102f,-482.974426f,73.456650f);
                                break;
                            case 3:
                                SubPhase = 0;
                                uiphase = 3;
                                uiSubPhaseTimer = 40000;
                                StopSlime = false;
                                me->GetMotionMaster()->MovePoint(3,-105.134102f,-482.974426f,73.456650f);
                                me->TextEmote(TEXT_EMOTE_GREEN,0,true);
                                break;
                        }
                    }
                }else
                    uiSubPhaseTimer -= diff;
            }

            if (!LastPhase && PhaseTwo == true)
            {
                if(uiSubPhaseTimer <= diff)
                {
                    if (IsHeroic())
                    {
                        switch(SubPhase)
                        {
                            case 0:
                                SubPhase = 1;
                            break;
                            case 1:
                                SubPhase = 2;
                                uiphase = 5;
                                uiSubPhaseTimer = 72000;
                                me->TextEmote(TEXT_EMOTE_DARK,0,true);
                                me->GetMotionMaster()->MovePoint(4,-105.134102f,-482.974426f,73.456650f);
                                me->AddAura(92716,me);
                                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true); 
                            break;
                            case 2:
                                CastTimer = 5000;
                                Poradi = 0;
                                me->RemoveAurasDueToSpell(92716);
                                SubPhase = 3;
                                uiphase = 2;
                                uiSubPhaseTimer = 40000;
                                me->TextEmote(TEXT_EMOTE_BLUE,0,true);
                                me->GetMotionMaster()->MovePoint(2,-105.134102f,-482.974426f,73.456650f);
                                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, false);
                                break;
                            case 3:
                                CastTimer = 5000;
                                Poradi = 0;
                                SubPhase = 4;
                                uiphase = 1;
                                uiSubPhaseTimer = 40000;
                                me->TextEmote(TEXT_EMOTE_RED,0,true);
                                me->GetMotionMaster()->MovePoint(1,-105.134102f,-482.974426f,73.456650f);
                            break;
                            case 4:
                                SubPhase = 0;
                                uiphase = 3;
                                uiSubPhaseTimer = 40000;
                                StopSlime = false;
                                me->GetMotionMaster()->MovePoint(3,-105.134102f,-482.974426f,73.456650f);
                                me->TextEmote(TEXT_EMOTE_GREEN,0,true);
                                break;
                        }
                    }
                    else // !IsHeroic()
                    {
                        switch(SubPhase)
                        {
                            case 0:
                                SubPhase = 1;
                                break;
                            case 1:
                                CastTimer = 5000;
                                Poradi = 0;
                                SubPhase = 2;
                                uiphase = 2;
                                uiSubPhaseTimer = 40000;
                                me->TextEmote(TEXT_EMOTE_BLUE,0,true);
                                me->GetMotionMaster()->MovePoint(2,-105.134102f,-482.974426f,73.456650f);
                                break;
                            case 2:
                                CastTimer = 5000;
                                Poradi = 0;
                                SubPhase = 3;
                                uiphase = 1;
                                uiSubPhaseTimer = 40000;
                                me->TextEmote(TEXT_EMOTE_RED,0,true);
                                me->GetMotionMaster()->MovePoint(1,-105.134102f,-482.974426f,73.456650f);
                                break;
                            case 3:
                                SubPhase = 0;
                                uiphase = 3;
                                uiSubPhaseTimer = 40000;
                                StopSlime = false;
                                me->GetMotionMaster()->MovePoint(3,-105.134102f,-482.974426f,73.456650f);
                                me->TextEmote(TEXT_EMOTE_GREEN,0,true);
                                break;
                        }
                    }
                }else
                    uiSubPhaseTimer -= diff;
            }
            
            if (uiphase == 1) //Red Phase 
            {
                if (CastTimer <= diff)
                {
                    switch(Poradi)
                    {
                        case 0:
                        {
                            Poradi = 1;
                        }
                        break;
                        case 1: //5 SEC
                        {
                            Poradi = 2;
                            CastTimer = 7000; 
                            me->CastSpell(me,SPELL_ARCANE_STORM,false);
                            can_interrupt = true;
                        }
                        break;
                        case 2: //13
                        {
                            Poradi = 3;
                            CastTimer = 2000;
                            Unit* target = SelectTarget(SELECT_TARGET_RANDOM,0,100,true);
                            me->CastSpell(target,SPELL_CONSUMING_FLAMES,false);
                            can_interrupt = false;
                        }
                        break;
                        case 3: // 16
                        {
                            can_interrupt = false;
                            Poradi = 4;
                            CastTimer = 3000;
                            Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO, 0);
                            if (target && target->isAlive() && target->GetTypeId() == TYPEID_PLAYER)
                                me->CastSpell(target,SPELL_SCORCHING_BLAST10N,false);
                        }
                        break;
                        case 4: //19
                        {
                            Poradi = 5;
                            CastTimer = 6000; 
                            me->CastSpell(me,SPELL_RELLEASE_ABERRATION,false);
                            can_interrupt = true;
                        }
                        break;
                        case 5:// 25
                        {
                            Poradi = 6;
                            CastTimer = 7000;
                            me->CastSpell(me,SPELL_ARCANE_STORM,false);
                            can_interrupt = true;
                        }
                        break;
                        case 6://31
                        {
                            Poradi= 7;
                            CastTimer = 2000;
                            Unit* target = SelectTarget(SELECT_TARGET_RANDOM,0,100,true);
                            me->CastSpell(target,SPELL_CONSUMING_FLAMES,false);
                            can_interrupt = false;
                        }
                        break;
                        case 7://34
                        {
                            Poradi = 8;
                            CastTimer = 6000;
                            me->CastSpell(me,SPELL_RELLEASE_ABERRATION,false);
                            can_interrupt = true;
                        }
                        break;
                        case 8://40
                        {
                            can_interrupt = false;
                            CastTimer = 20000;
                            Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO, 0);
                            if (target && target->isAlive() && target->GetTypeId() == TYPEID_PLAYER)
                                me->CastSpell(target,SPELL_SCORCHING_BLAST10N,false);
                        }
                        break;
                    }
                }
                else 
                    CastTimer -= diff;
            }

            if (uiphase == 2) //Blue Phase 
            {
                if (CastTimer <= diff)
                {
                    switch(Poradi)
                    {
                        case 0:
                        {
                            Poradi = 1;
                        }
                        break;
                        case 1: //5 SEC
                        {
                            Poradi = 2;
                            CastTimer = 7000; 
                            me->CastSpell(me,SPELL_ARCANE_STORM,false);
                            can_interrupt = true;
                        }
                        break;
                        case 2: //13
                        {
                            can_interrupt = false;
                            Poradi = 3;
                            CastTimer = 2000;
                            if (SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                                DoCast (SPELL_BITTING_CHILL);
                        }
                        break;
                        case 3: // 16
                        {
                            can_interrupt = false;
                            Poradi = 4;
                            CastTimer = 5000;
                            if (Unit *pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                            {
                                if (pTarget->isAlive())
                                {
                                    if (Creature *pFlashFreeze = me->SummonCreature(NPC_FLASH_FREEZE, pTarget->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 30000))
                                    {
                                        CAST_AI(npc_flash_freeze::npc_flash_freezeAI, pFlashFreeze->AI())->SetPrison(pTarget);
                                        pFlashFreeze->CastSpell(pTarget, SPELL_FLASH_FREEZE, true);
                                    }
                                 }
                            }
                        }
                        break;
                        case 4: //19
                        {
                            can_interrupt = true;
                            Poradi = 5;
                            CastTimer = 6000; 
                            me->CastSpell(me,SPELL_RELLEASE_ABERRATION,false);
                        }
                        break;
                        case 5:// 25
                        {
                            can_interrupt = true;
                            Poradi = 6;
                            CastTimer = 7000;
                            me->CastSpell(me,SPELL_ARCANE_STORM,false);
                        }
                        break;
                        case 6://31
                        {
                            can_interrupt = false;
                            Poradi = 7;
                            CastTimer = 2000;
                            if (Unit *pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                            {
                                if (pTarget->isAlive())
                                {
                                    if (Creature *pFlashFreeze = me->SummonCreature(NPC_FLASH_FREEZE, pTarget->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 30000))
                                    {
                                        CAST_AI(npc_flash_freeze::npc_flash_freezeAI, pFlashFreeze->AI())->SetPrison(pTarget);
                                        pFlashFreeze->CastSpell(pTarget, SPELL_FLASH_FREEZE, true);
                                    }
                                 }
                            }
                        }
                        break;
                        case 7://34
                        {
                            can_interrupt = true;
                            Poradi = 8;
                            CastTimer = 6000;
                            me->CastSpell(me,SPELL_RELLEASE_ABERRATION,false);
                        }
                        break;
                        case 8://40
                        {
                            can_interrupt = false;
                            CastTimer = 20000;
                            if (SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                                DoCast (SPELL_BITTING_CHILL);
                        }
                        break;
                    }
                }
                else 
                    CastTimer -= diff;
            }

            if (uiphase == 3) // GreenPhase
            {
                if (uiReleaseAberrations <= diff )
                {
                    me->InterruptNonMeleeSpells(true);
                    me->CastSpell(me,SPELL_RELLEASE_ABERRATION,false);
                    uiReleaseAberrations = 17000;
                } else uiReleaseAberrations -= diff;

                if (!StopSlime && uiDebilitatingSlime<= diff)
                {
                    me->CastSpell(me,SPELL_DEBILITATING_SLIME,true);
                    StopSlime = true;
                } else uiDebilitatingSlime-= diff;
            }

            if (uiphase == 4) // Final Phase
            {
                if (uiAbsoluteZero <= diff)
                {
                    if (Unit *pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 300, true))
                    {
                        me->SummonCreature(NPC_ABSOLUTE_ZERO,pTarget->GetPositionX()+15,pTarget->GetPositionY(),pTarget->GetPositionZ(),0.0f,TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 35000);
                    }
                    uiAbsoluteZero = 30000;
                }else uiAbsoluteZero -= diff;

                if (uiAcid_Nova <= diff )
                {
                    DoCast(me, SPELL_ACID_NOVA);
                    uiAcid_Nova = urand(10000,12000);
                } else uiAcid_Nova -= diff;

                if (uiMagmaJets <= diff)
                {
                    Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO, 0);
                    if (target && target->isAlive() && target->GetTypeId() == TYPEID_PLAYER)
                    {
                        //DoCast(SPELL_MAGMA_JETS);
                        me->SummonCreature(41901,target->GetPositionX(),target->GetPositionY()+5,target->GetPositionZ(),0.0f,TEMPSUMMON_TIMED_DESPAWN,20000);
                        me->SummonCreature(41901,target->GetPositionX(),target->GetPositionY()+10,target->GetPositionZ(),0.0f,TEMPSUMMON_TIMED_DESPAWN,20000);
                        me->SummonCreature(41901,target->GetPositionX(),target->GetPositionY()+15,target->GetPositionZ(),0.0f,TEMPSUMMON_TIMED_DESPAWN,20000);
                        me->SummonCreature(41901,target->GetPositionX(),target->GetPositionY()+20,target->GetPositionZ(),0.0f,TEMPSUMMON_TIMED_DESPAWN,20000);
                    }
                    uiMagmaJets = 8000;
                } else uiMagmaJets -= diff;
            }

            if (uiphase == 5)
            {
                if(uiEngulfingDarkness <=diff)
                {
                    uiEngulfingDarkness = 10000;
                    Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO, 0);
                    if (target && target->isAlive() && target->GetTypeId() == TYPEID_PLAYER)
                    {
                        me->CastSpell(target,92982,false);
                    }
                }
                else
                    uiEngulfingDarkness -= diff;

                if(uiVilleSwill <= diff)
                {
                    uiVilleSwill = 5000;
                    me->SummonCreature(49811,me->GetPositionX(),me->GetPositionY(),me->GetPositionZ(),1.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT,2000);
                }
                else
                    uiVilleSwill -= diff;
            }
            DoMeleeAttackIfReady();
        }

        void JustSummoned (Creature* pSummon)
        {
            if (pSummon->GetEntry() == 41901)
            {
                pSummon->CastSpell(pSummon,78095,false);
                pSummon->CastSpell(pSummon,62680,false);
            }
            if (pSummon->GetEntry() == NPC_ABERRATION)
            {
                if (Unit *pTarget = SelectUnit(SELECT_TARGET_RANDOM,0))
                   if (pSummon->AI())
                       pSummon->AI()->AttackStart(pTarget);
            }
            if (pSummon->GetEntry() == NPC_FLASH_FREEZE)
            {
                pSummon->AddAura(77712,pSummon);
            }
        }


        void SummonedCreatureDies(Creature* pSummon, Unit* killer) 
        {
            if (pSummon->GetEntry() == NPC_ABERRATION)
            {
                me->AI()->DoAction(RECORD_DEATH);
                me->AI()->DoAction(CHECK_COUNTER);
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            OpenCell();
            DoScriptText(TEXT_DEATH,me);

            if (pInstance)
                pInstance->SetData(DATA_MALORIAK, DONE);

            if (AwardAchiev == true)
            {
                if(pInstance)
                {
                     if (getDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL)
                         pInstance->DoCompleteAchievement(1872);
                }
            }
        }

        void KilledUnit(Unit* victim)
        {
            DoScriptText(RAND(TEXT_KILL1,TEXT_KILL2),me);
        }
    };
};

class npc_Prime_Subject : public CreatureScript
{
public:
    npc_Prime_Subject() : CreatureScript("npc_prime_Subject") { }

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
        bool StopTimer;

        void Reset()
        {
            AggroTimer = 4000;
            StopTimer = false;
        }

        void EnterCombat(Unit * /*who*/)
        {
            DoZoneInCombat();
            me->AddAura(77987,me);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
            //timer pro aggro a taunt imunitu náhrada za spell fixate.
            if (!StopTimer)
            {
                if(AggroTimer <= diff)
                {
                    StopTimer = true;
                    me->AddThreat(me,10000);
                    me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true); 
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                } else AggroTimer -= diff;
            }
        }
    };
};

class npc_aberration: public CreatureScript
{
    public:
        npc_aberration() : CreatureScript("npc_aberration") { }

        struct npc_aberrationAI : public ScriptedAI
        {
            npc_aberrationAI(Creature* creature) : ScriptedAI(creature)
              
            {
            }
            uint32 timer;
            uint32 ListSize;
            void CheckAuraCount()
            {
                std::list<Creature*> AuraCount;
                me->GetCreatureListWithEntryInGrid(AuraCount, NPC_ABERRATION, 10.0f);
                if (!AuraCount.empty())
                {
                    
                    for (std::list<Creature*>::const_iterator itr = AuraCount.begin(); itr != AuraCount.end(); ++itr)
                    {
                        for (uint32 i = 0; i < AuraCount.size(); ++i)
                        {
                            (*itr)->AddAura(77987,me);
                        }
                    }
                }
            }
        
            void Reset()
            {
                me->RemoveAllAuras();
                timer = 3000;
            }

            void EnterCombat(Unit* /*target*/) { }


            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim())
                    return;

                if(timer <= diff)
                {
                    me->RemoveAllAuras();
                    CheckAuraCount();
                    timer = 2000;
                }
                else
                    timer -= diff;

               DoMeleeAttackIfReady();
            }

        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_aberrationAI(creature);
        }
};

void AddSC_maloriak()
{
    new npc_Prime_Subject();
    new boss_maloriak();
    new npc_flash_freeze();
    new npc_absolute_zero();
    new npc_aberration();
}