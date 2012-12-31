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
    SET_TIMER                   = 5,
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
    NPC_VILE_SWILL_PREEFECT     = 49812,
    NPC_VILE_SWILL              = 49811,
    NPC_MAGMA_JETS              = 41901,
};

enum Spells
{
    // Maloriak ====>
    SPELL_MAGMA_JETS             = 78194,
    SPELL_ACID_NOVA              = 93013,
    SPELL_REMEDY                 = 77912,
    SPELL_BITTING_CHILL          = 77760,
    SPELL_ARCANE_STORM           = 77896,
    SPELL_ENFULGING_DARKNESS     = 92754,

    //Consuming flames
    SPELL_CONSUMING_FLAMES       = 77786,
    SPELL_CONSUMING_FLAMES10HC   = 92972,
    SPELL_CONSUMING_FLAMES25N    = 92971,
    SPELL_CONSUMING_FLAMES25HC   = 92973,

    // Scorching blast
    SPELL_SCORCHING_BLAST10N     = 77679,
    SPELL_SCORCHING_BLAST10HC    = 92969,
    SPELL_SCORCHING_BLAST25N     = 92968,
    SPELL_SCORCHING_BLAST25HC    = 92970,

    SPELL_RELEASE_ALL_MINION     = 77991,
    SPELL_RELEASE_ABERRATION     = 77569,
    // <==== Maloriak

    // Aberration ====>
    SPELL_GROWTH_CATACLYST       = 77987,
    // <==== Aberration

    // Cauldron ====>
    SPELL_DEBILITATING_SLIME     = 77615,
    SPELL_DEBILITATING_SLIME_VISUAL = 77602,
    // <==== Cauldron

    // Flash Freeze ====>
    SPELL_FLASH_FREEZE_10M          = 77699,
    SPELL_FLASH_FREEZE_25M       = 92978,
    SPELL_FLASH_FREEZE_10M_HC    = 92979,
    SPELL_FLASH_FREEZE_25M_HC    = 92980,
    // <==== Flash Freeze


    // Absolute Zero ====>
    SPELL_ABSOLUTE_ZERO          = 78223,
    SPELL_ABSOLUTE_ZERO_AURA     = 78201,
    SPELL_ABSOLUTE_ZERO_DMG      = 78208,
    // <==== Absolute Zero


    // Vile Swill ====>
    SPELL_DARK_SLUDGE            = 92987,
    SPELL_VILE_SWILL_PREEFECT    = 92737,
    // <==== Vile Swill
};

static const Position SpawnPos[2] =
{
    {-105.692007f, -435.626007f, 73.331398f},
    {-105.692007f, -435.626007f, 73.331398f}
};

static const Position AberrationSpawnPos[3] =
{
    {-149.28f, -443.74f, 89.931f, 0},
    {-148.25f, -439.79f, 89.137f, 0},
    {-60.197f, -449.86f, 85.557f, 0}
};

class npc_vile_swill_preeffect: public CreatureScript
{
    public:
        npc_vile_swill_preeffect() : CreatureScript("npc_vile_swill_preeffect") { }

        struct npc_vile_swill_preeffectAI : public ScriptedAI
        {
            npc_vile_swill_preeffectAI(Creature* creature) : ScriptedAI(creature){}
            
            uint32 Release_timer;
            int Count;

            void Reset()
            {
                Release_timer = 2000;
                Count = 0;
            }

            void EnterCombat(Unit* /*target*/) { }

            void DamageTaken(Unit* pTarget, uint32& damage)
            {
                damage = 0;
            }
            
            void UpdateAI(uint32 const diff)
            {
                    
                if (Count >= 5)
                {
                    me->DespawnOrUnsummon();
                }

                if(Release_timer <= diff)
                {
                    Count++;
                    me->SummonCreature(NPC_VILE_SWILL,me->GetPositionX(),me->GetPositionY(),me->GetPositionZ(),0.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                    me->CastSpell(me,92720,false);
                    Release_timer = 2000;
                }
                else
                    Release_timer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_vile_swill_preeffectAI(creature);
        }
};

class npc_vile_swill: public CreatureScript
{
    public:
        npc_vile_swill() : CreatureScript("npc_vile_swill") { }

        struct npc_vile_swillAI : public ScriptedAI
        {
            npc_vile_swillAI(Creature* creature) : ScriptedAI(creature){}
            uint32 Dark_Sludge;

            void Reset()
            {
                Dark_Sludge = 2000;
            }

            void EnterCombat(Unit* /*target*/) { }


            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim())
                    return;

                if(Dark_Sludge <= diff)
                {
                    Unit* target = SelectTarget(SELECT_TARGET_RANDOM,0,100,true);
                    me->CastSpell(target,SPELL_DARK_SLUDGE,false);
                    Dark_Sludge = 2000;
                }
                else
                    Dark_Sludge -= diff;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_vile_swillAI(creature);
        }
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
        npc_absolute_zeroAI(Creature *c) : ScriptedAI(c) { }

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
                    FlashFreeze->RemoveAurasDueToSpell(RAID_MODE(SPELL_FLASH_FREEZE_10M,SPELL_FLASH_FREEZE_25M,SPELL_FLASH_FREEZE_10M_HC,SPELL_FLASH_FREEZE_25M_HC));
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
            c->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            c->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
            c->ApplySpellImmune(0, IMMUNITY_ID, 49560, true); // Death Grip jump effect
            c->ApplySpellImmune(0, IMMUNITY_ID, 81261, true); // Solar Beam
            c->ApplySpellImmune(0, IMMUNITY_ID, 88625, true); // Chastise
            c->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
            Reset();
        }
        // uint32 pro spelly
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
        uint32 maxspawncount;

        // uint32 pro faze
        uint32 uiSwitchPhaseTimer;
        uint32 uiSubPhaseTimer;

        // zbytek
        uint32 Poradi;
        uint32 CastTimer;
        uint32 Phase;
        uint32 SubPhase;
        uint32 AuraCheckTimer;

        uint32 HeroicIntro;

        InstanceScript* pInstance;
        bool isEnraged;
        bool SummonAll;
        bool StopTimer;
        bool PhaseOne;
        bool PhaseTwo;
        bool LastPhase;
        bool StopSlime;
        bool Jump;
        bool AwardAchiev;
        bool IntroDone;
        bool can_interrupt;

        uint8 uiphase;
        GameObject* pCauldron;
        Creature* pLord;
        Creature* pCauldronTrigger;
        uint32 counter;
        uint32 AberrationAchievementCounter;
        uint32 SpellCounter;

        std::list<uint32> AberrationDeathTimeStamp;
        std::vector<uint64> PrimeSubject;

        void Reset()
        {
            OpenCell();
            if (pInstance)
                pInstance->SetData(DATA_MALORIAK_GUID, NOT_STARTED);
            uiRemedy = 14000;
            uiphase = 0;
            Berserk = IsHeroic() ? 1200000 : 420000; // 7 minut
            SummonAll = false;
            Poradi = 0;
            counter = 0;
            maxspawncount = 0;
            SpellCounter = 0;

            pCauldron = NULL;
            pLord = NULL;
            pCauldronTrigger = NULL;

            isEnraged = false;
            PhaseOne = false;
            PhaseTwo = false;
            LastPhase = false;
            StopSlime = false;
            AwardAchiev = false;
            IntroDone = false;
            Jump = false;
            can_interrupt = false;
            //StopTimer = false;
        }

        void CloseCell()
        {
            if(GameObject* Cell= me->FindNearestGameObject(191722,100.0f))
            {
                Cell->SetGoState(GO_STATE_READY);
            }
        }

        void ChangeCauldronColor(uint8 color) {
            // 0 - red, 1 - blue, 2 - green, 3 - dark
            if (pCauldron)
                pCauldron->SendCustomAnim(color);
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

            if (spell->Id == SPELL_RELEASE_ABERRATION)
            {
                SpellCounter++;

                if(SpellCounter <= 6)
                {
                    for (uint32 i = 0; i < 3; i++)
                    {
                        me->SummonCreature(NPC_ABERRATION,AberrationSpawnPos[i],TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
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
                for (uint32 i = 0; i < 15; i++) // 18 (15 + 3 = 18)
                {
                    for (uint32 o = 0; o < 3; o++)
                    {
                        me->SummonCreature(NPC_ABERRATION,AberrationSpawnPos[i],TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                    }
                }
            }
            else if(SpellCounter == 1)
            {
                for (uint32 i = 0; i < 12; i++) // 15 (12 + 3 = 15)
                {
                    for (uint32 o = 0; o < 3; o++)
                    {
                        me->SummonCreature(NPC_ABERRATION,AberrationSpawnPos[i],TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                    }
                }
            }
            else if(SpellCounter == 2)
            {
                for (uint32 i = 0; i < 9; i++) // 12 (9 + 3 = 12)
                {
                    for (uint32 o = 0; o < 3; o++)
                    {
                        me->SummonCreature(NPC_ABERRATION,AberrationSpawnPos[i],TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                    }
                }
            }
            else if(SpellCounter == 3)
            {
                for (uint32 i = 0; i < 6; i++) // 9 (6 + 3 = 9)
                {
                    for (uint32 o = 0; o < 3; o++)
                    {
                        me->SummonCreature(NPC_ABERRATION,AberrationSpawnPos[i],TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                    }
                }
            }
            else if(SpellCounter == 4)
            {
                for (uint32 i = 0; i < 3; i++) // 6 (3 + 3 = 6)
                {
                    for (uint32 o = 0; o < 3; o++)
                    {
                        me->SummonCreature(NPC_ABERRATION,AberrationSpawnPos[i],TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                    }
                 }
            }
            else if(SpellCounter == 5)
            {
                for (uint32 i = 0; i < 3; i++)
                {
                    me->SummonCreature(NPC_ABERRATION,AberrationSpawnPos[i],TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                }
            }
            else
                return;
        }

        void EnterCombat(Unit* /*who*/)
        {
           if(IsHeroic())
                HeroicIntro = 8000;

            DoScriptText(TEXT_AGGRO,me);

            if (pInstance)
                pInstance->SetData(DATA_MALORIAK_GUID, IN_PROGRESS);
            SummonAll = false;

            uiSubPhaseTimer = 1000;
            uiSwitchPhaseTimer = 15000;
            Phase = 0;
            SubPhase = 0;
            CloseCell();
            AuraCheckTimer = 1000;

            if (pCauldron == NULL)
                pCauldron = me->FindNearestGameObject(459554, 100.0f);

            if (pCauldronTrigger == NULL)
                pCauldronTrigger = me->FindNearestCreature(NPC_CAULDRON_TRIGGER, 250.0f);

            if (pLord == NULL)
                pLord = me->FindNearestCreature(NPC_LORD_VOICE_TRIGGER, 100.0f);

        }

        void DoAction(const int Action)
        {
            switch (Action)
            {
                case RECORD_DEATH:
                {
                    AberrationDeathTimeStamp.push_back(getMSTime());
                    counter++;
                }
                break;
                case CHECK_COUNTER:
                {
                    if (counter >= 12)
                    {
                        uint32 m_uiTime = getMSTime();
                        counter = 0;
                        for(std::list<uint32>::const_iterator itr = AberrationDeathTimeStamp.begin(); itr != AberrationDeathTimeStamp.end(); ++itr)
                        {
                            if (*itr >= (m_uiTime - 12000))
                            counter++;
                        }
                        if (counter == 12)
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
                    DoScriptText(TEXT_RED_VIAL, me);
                    me->GetMotionMaster()->MoveChase(me->getVictim());

                    ChangeCauldronColor(0);
                }
                break;
                case 2:
                {
                    DoScriptText(TEXT_BLUE_VIAL,me);
                    me->GetMotionMaster()->MoveChase(me->getVictim());

                    ChangeCauldronColor(1);
                }
                break;
                case 3:
                {
                     FindPlayers();
                     DoScriptText(TEXT_GREEN_VIAL,me);
                     Jump = true;
                     ChangeCauldronColor(2);
                }
                break;
                case 4:
                {
                    ChangeCauldronColor(3);
                    uiEngulfingDarkness = 6000;
                    uiVilleSwill = 8000;
                    me->GetMotionMaster()->MoveChase(me->getVictim());
                    if(pLord)
                    {
                        pLord->MonsterYell("Your mixtures are weak, Maloriak! They need a bit more... kick!",0,0);
                        pLord->PlayDirectSound(23370,0);
                    }
                }
                break;
            }
        }

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
                        if (!me->HasAura(SPELL_DEBILITATING_SLIME))
                            (*itr)->AddAura(SPELL_GROWTH_CATACLYST,me);
                    }
                }
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if(AuraCheckTimer <= diff)
            {
                me->RemoveAurasDueToSpell(SPELL_GROWTH_CATACLYST);
                CheckAuraCount();
                AuraCheckTimer = 1000;
            }
            else
                AuraCheckTimer -= diff;

            if (!IntroDone)
            {
                if (HeroicIntro <= diff)
                {
                    IntroDone = true;
                    if (pLord)
                    {
                        pLord->MonsterYell("Maloriak, try not to lose to these mortals! Semi-competent help is SO hard to create.",0,0);
                        pLord->PlayDirectSound(23372,0);
                    }
                }
                else
                    HeroicIntro -= diff;
            }

            if (Jump == true && !me->hasUnitState(UNIT_STAT_CASTING))
            {
                Jump = false;
                me->GetMotionMaster()->MoveJump(-102.292519f,-439.359955f,73.534279f,20.0f,20.0f);
                me->GetMotionMaster()->MoveChase(me->getVictim());
                if(pCauldronTrigger)
                    pCauldronTrigger->CastSpell(pCauldronTrigger, SPELL_DEBILITATING_SLIME_VISUAL, false);
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
                {
                    PrimeSubject.push_back(Spawned->GetGUID());
                    if (Unit *pTarget = SelectUnit(SELECT_TARGET_RANDOM,0))
                        if (Spawned->AI())
                            Spawned->AI()->AttackStart(pTarget);
                }
                // 2.prime Subject
                if (Creature* Spawned = me->SummonCreature(NPC_PRIME_SUBJECT,SpawnPos[1],TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000))
                {
                    PrimeSubject.push_back(Spawned->GetGUID());
                    if (Unit *pTarget = SelectUnit(SELECT_TARGET_RANDOM,0))
                        if (Spawned->AI())
                            Spawned->AI()->AttackStart(pTarget);
                }

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
                            uiSwitchPhaseTimer = 194000;
                            PhaseOne = true;
                            PhaseTwo = false;
                            break;
                        case 2:
                            Phase = 0;
                            uiSwitchPhaseTimer = 194000;
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
                            uiSwitchPhaseTimer = 122000;
                            PhaseOne = true;
                            PhaseTwo = false;
                        break;
                        case 2:
                            Phase = 0;
                            uiSwitchPhaseTimer = 122000;
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
                                StopTimer = false;
                                SubPhase = 2;
                                uiphase = 5;
                                uiSubPhaseTimer = 72000;
                                maxspawncount = 0;
                                me->GetMotionMaster()->MovePoint(4,-105.134102f,-482.974426f,73.456650f);
                                me->AddAura(92716,me);
                                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true); 
                            break;
                            case 2:
                                SubPhase = 3;
                                uiphase = 1;
                                uiSubPhaseTimer = 50000;
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
                                me->GetMotionMaster()->MovePoint(2,-105.134102f,-482.974426f,73.456650f);
                                CastTimer = 5000;
                                Poradi = 0;
                            break;
                            case 4:
                                uiReleaseAberrations = 4000;
                                SubPhase = 0;
                                uiphase = 3;
                                uiSubPhaseTimer = 50000;
                                StopSlime = false;
                                me->GetMotionMaster()->MovePoint(3,-105.134102f,-482.974426f,73.456650f);
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
                                me->GetMotionMaster()->MovePoint(1,-105.134102f,-482.974426f,73.456650f);
                                break;
                            case 2:
                                CastTimer = 5000;
                                Poradi = 0;
                                SubPhase = 3;
                                uiphase = 2;
                                uiSubPhaseTimer = 40000;
                                me->GetMotionMaster()->MovePoint(2,-105.134102f,-482.974426f,73.456650f);
                                break;
                            case 3:
                                uiReleaseAberrations = 4000;
                                SubPhase = 0;
                                uiphase = 3;
                                uiSubPhaseTimer = 40000;
                                StopSlime = false;
                                me->GetMotionMaster()->MovePoint(3,-105.134102f,-482.974426f,73.456650f);
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
                                StopTimer = false;
                                SubPhase = 2;
                                uiphase = 5;
                                maxspawncount = 0;
                                uiSubPhaseTimer = 72000;
                                ChangeCauldronColor(3);
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
                                me->GetMotionMaster()->MovePoint(2,-105.134102f,-482.974426f,73.456650f);
                                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, false);
                                break;
                            case 3:
                                CastTimer = 5000;
                                Poradi = 0;
                                SubPhase = 4;
                                uiphase = 1;
                                uiSubPhaseTimer = 40000;
                                me->GetMotionMaster()->MovePoint(1,-105.134102f,-482.974426f,73.456650f);
                            break;
                            case 4:
                                SubPhase = 0;
                                uiphase = 3;
                                uiReleaseAberrations = 4000;
                                uiSubPhaseTimer = 42000;
                                StopSlime = false;
                                me->GetMotionMaster()->MovePoint(3,-105.134102f,-482.974426f,73.456650f);
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
                                me->GetMotionMaster()->MovePoint(2,-105.134102f,-482.974426f,73.456650f);
                                break;
                            case 2:
                                CastTimer = 5000;
                                Poradi = 0;
                                SubPhase = 3;
                                uiphase = 1;
                                uiSubPhaseTimer = 40000;
                                me->GetMotionMaster()->MovePoint(1,-105.134102f,-482.974426f,73.456650f);
                                break;
                            case 3:
                                SubPhase = 0;
                                uiphase = 3;
                                uiReleaseAberrations = 4000;
                                uiSubPhaseTimer = 42000;
                                StopSlime = false;
                                me->GetMotionMaster()->MovePoint(3,-105.134102f,-482.974426f,73.456650f);
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
                            Poradi = 1;
                            break;
                        case 1: //5 SEC
                            Poradi = 2;
                            CastTimer = 7000;
                            me->CastSpell(me,SPELL_ARCANE_STORM,false);
                            can_interrupt = true;
                            break;
                        case 2: //13
                        {
                            Poradi = 3;
                            CastTimer = 2000;
                            Unit* target = SelectTarget(SELECT_TARGET_RANDOM,0,100,true);
                            me->CastSpell(target,SPELL_CONSUMING_FLAMES,false);
                            can_interrupt = false;
                            break;
                        }
                        case 3: // 16
                        {
                            can_interrupt = false;
                            Poradi = 4;
                            CastTimer = 3000;
                            Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO, 0);
                            if (target && target->isAlive() && target->GetTypeId() == TYPEID_PLAYER)
                                me->CastSpell(target,SPELL_SCORCHING_BLAST10N,false);
                            break;
                        }
                        case 4: //19
                            Poradi = 5;
                            CastTimer = 6000; 
                            me->CastSpell(me,SPELL_RELEASE_ABERRATION,false);
                            can_interrupt = true;
                            break;
                        case 5:// 25
                            Poradi = 6;
                            CastTimer = 7000;
                            me->CastSpell(me,SPELL_ARCANE_STORM,false);
                            can_interrupt = true;
                            break;
                        case 6://31
                        {
                            Poradi= 7;
                            CastTimer = 2000;
                            Unit* target = SelectTarget(SELECT_TARGET_RANDOM,0,100,true);
                            me->CastSpell(target,SPELL_CONSUMING_FLAMES,false);
                            can_interrupt = false;
                            break;
                        }
                        case 7://34
                            Poradi = 8;
                            CastTimer = 5000;
                            me->CastSpell(me,SPELL_RELEASE_ABERRATION,false);
                            can_interrupt = true;
                            break;
                        case 8://40
                        {
                            can_interrupt = false;
                            CastTimer = 20000;
                            Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO, 0);
                            if (target && target->isAlive() && target->GetTypeId() == TYPEID_PLAYER)
                                me->CastSpell(target,SPELL_SCORCHING_BLAST10N,false);
                        break;
                        }
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
                                DoCast(SPELL_BITTING_CHILL);
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
                                        pFlashFreeze->CastSpell(pTarget, SPELL_FLASH_FREEZE_10M, true);
                                        pTarget->ToPlayer()->TeleportTo(pFlashFreeze->GetMapId(), pFlashFreeze->GetPositionX(),pFlashFreeze->GetPositionY(),pFlashFreeze->GetPositionZ(),pTarget->GetOrientation()); // Protoze nechcem aby to nestalo vizualne za houby
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
                            me->CastSpell(me,SPELL_RELEASE_ABERRATION,false);
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
                                        pFlashFreeze->CastSpell(pTarget, SPELL_FLASH_FREEZE_10M, true);
                                        pTarget->ToPlayer()->TeleportTo(pFlashFreeze->GetMapId(), pFlashFreeze->GetPositionX(),pFlashFreeze->GetPositionY(),pFlashFreeze->GetPositionZ(),pTarget->GetOrientation()); // Protoze nechcem aby to nestalo vizualne za houby
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
                            me->CastSpell(me,SPELL_RELEASE_ABERRATION,false);
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
                    me->CastSpell(me,SPELL_RELEASE_ABERRATION,false);
                    uiReleaseAberrations = 17000;
                } else uiReleaseAberrations -= diff;

                if (StopSlime == false)
                {
                    me->CastSpell(me,SPELL_DEBILITATING_SLIME,false);
                    StopSlime = true;
                }
            }

            if (uiphase == 4) // Final Phase
            {
                if (uiAbsoluteZero <= diff)
                {
                    if (Unit *pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 300, true))
                        me->SummonCreature(NPC_ABSOLUTE_ZERO,pTarget->GetPositionX()+15,pTarget->GetPositionY(),pTarget->GetPositionZ(),0.0f,TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 35000);

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
                        me->SetFacing(target->GetOrientation());
                        //DoCast(SPELL_MAGMA_JETS);
                        me->SummonCreature(NPC_MAGMA_JETS,target->GetPositionX()+5,target->GetPositionY(),target->GetPositionZ(),0.0f,TEMPSUMMON_TIMED_DESPAWN,20000);
                        me->SummonCreature(NPC_MAGMA_JETS,target->GetPositionX()+10,target->GetPositionY(),target->GetPositionZ(),0.0f,TEMPSUMMON_TIMED_DESPAWN,20000);
                        me->SummonCreature(NPC_MAGMA_JETS,target->GetPositionX()+15,target->GetPositionY(),target->GetPositionZ(),0.0f,TEMPSUMMON_TIMED_DESPAWN,20000);
                        me->SummonCreature(NPC_MAGMA_JETS,target->GetPositionX()+20,target->GetPositionY(),target->GetPositionZ(),0.0f,TEMPSUMMON_TIMED_DESPAWN,20000);
                        me->SummonCreature(NPC_MAGMA_JETS,target->GetPositionX()+25,target->GetPositionY(),target->GetPositionZ(),0.0f,TEMPSUMMON_TIMED_DESPAWN,20000);
                        me->SummonCreature(NPC_MAGMA_JETS,target->GetPositionX()+30,target->GetPositionY(),target->GetPositionZ(),0.0f,TEMPSUMMON_TIMED_DESPAWN,20000);                    }
                    uiMagmaJets = 8000;
                } else uiMagmaJets -= diff;
            }

            if (uiphase == 5)
            {
                if(uiEngulfingDarkness <=diff)
                {
                    uiEngulfingDarkness = 12000;
                    Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO, 0);
                    if (target && target->isAlive() && target->GetTypeId() == TYPEID_PLAYER)
                    {
                        me->StopMoving();
                        me->CastSpell(target, SPELL_ENFULGING_DARKNESS, false);
                    }
                }
                else
                    uiEngulfingDarkness -= diff;
                
                if(!StopTimer)
                {
                    if(uiVilleSwill <= diff)
                    {
                        StopTimer = true;
                        uiVilleSwill = 150000;
                        me->SummonCreature(NPC_VILE_SWILL_PREEFECT,me->GetPositionX()+5,me->GetPositionY(),me->GetPositionZ()+1,TEMPSUMMON_MANUAL_DESPAWN);
                    }
                    else
                        uiVilleSwill -= diff;   
                }
            }
            DoMeleeAttackIfReady();
        }

        void JustSummoned (Creature* pSummon)
        {
            switch(pSummon->GetEntry())
            {
                case NPC_VILE_SWILL_PREEFECT:
                    pSummon->CastSpell(pSummon,SPELL_VILE_SWILL_PREEFECT,true);
                    break;
                case NPC_MAGMA_JETS:
                    pSummon->CastSpell(pSummon,78095,false);
                    break;
                case NPC_ABERRATION:
                    pSummon->GetMotionMaster()->MoveJump(-106.564323f, -455.508728f, 73.458221f,1.0f,1.0f);
                    if (Unit *pTarget = SelectUnit(SELECT_TARGET_RANDOM,0)) 
                       if (pSummon->AI())
                           pSummon->AI()->AttackStart(pTarget);
                    break;
                case NPC_FLASH_FREEZE:
                    pSummon->AddAura(77712,pSummon);
                    break;
            }
        }

        void SummonedCreatureDies(Creature* pSummon, Unit* killer) 
        {
            if (pSummon->GetEntry() == NPC_ABERRATION)
            {
                /*me->AI()->DoAction(RECORD_DEATH);
                me->AI()->DoAction(CHECK_COUNTER);*/
            }
        }

        void CleanPrimeSubject()
        {
             Unit* Prime = NULL;
             for (std::vector<uint64>::const_iterator itr = PrimeSubject.begin(); itr != PrimeSubject.end(); ++itr) {
                Prime = me->GetMap()->GetCreature((*itr));
                if (Prime) {
                    if (Prime->isAlive())
                        Prime->ToCreature()->ForcedDespawn();
                }
             }
             PrimeSubject.clear();
        }

        void JustDied(Unit* /*killer*/)
        {
            CleanPrimeSubject();
            OpenCell();
            if(IsHeroic())
            {
                if(pLord)
                {
                    pLord->MonsterYell("Congratulations! Allow me to grant you a title befitting the amazing achievement you just performed! Henceforth, you shall be known as the Slayer of Incompetent, Stupid and Disappointing Minions!", LANG_UNIVERSAL, 0);
                    pLord->PlayDirectSound(23371,0);
                }
            }
            DoScriptText(TEXT_DEATH,me);

            if (pInstance)
                pInstance->SetData(DATA_MALORIAK_GUID, DONE);

            if (AwardAchiev == true)
            {
                if(pInstance)
                {
                     if (getDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL)
                         pInstance->DoCompleteAchievement(5310);
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
            me->AddAura(SPELL_GROWTH_CATACLYST,me);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();

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
            npc_aberrationAI(Creature *c) : ScriptedAI(c){}

            uint32 ListSize;
            uint32 timer;
 
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
                            if (!me->HasAura(SPELL_DEBILITATING_SLIME))
                                (*itr)->AddAura(SPELL_GROWTH_CATACLYST,me);
                        }
                    }
                }
            }

            void Reset()
            {
                me->RemoveAllAuras();
                timer = 1000;
            }

            void EnterCombat(Unit* /*target*/) 
            { 
                me->AddUnitMovementFlag(MOVEMENTFLAG_WALKING);
                me->GetMotionMaster()->Clear(true);
                me->GetMotionMaster()->MoveChase(me->getVictim());
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim())
                    return;

                if(timer <= diff)
                {
                    me->RemoveAurasDueToSpell(SPELL_GROWTH_CATACLYST);
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

class spell_gen_maloriak_remedy : public SpellScriptLoader
{
public:
    spell_gen_maloriak_remedy() : SpellScriptLoader("spell_gen_maloriak_remedy") { }

    class spell_gen_maloriak_remedy_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_gen_maloriak_remedy_AuraScript);
        void HandleTick(AuraEffect const* aurEff)
        {
            Unit* target = GetTarget();
            if (target && target->GetTypeId() == TYPEID_UNIT)
            {
                int32 baseAmount = aurEff->GetBaseAmount();
                if (baseAmount > 0)
                    const_cast<AuraEffect*>(aurEff)->SetAmount(baseAmount * aurEff->GetTickNumber());
            }
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_gen_maloriak_remedy_AuraScript::HandleTick, EFFECT_0, SPELL_AURA_PERIODIC_HEAL);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_gen_maloriak_remedy_AuraScript();
    }
};

void AddSC_maloriak()
{
    new npc_vile_swill_preeffect();
    new npc_Prime_Subject();
    new boss_maloriak();
    new npc_flash_freeze();
    new npc_absolute_zero();
    new npc_aberration();
    new npc_vile_swill();
    new spell_gen_maloriak_remedy();
}