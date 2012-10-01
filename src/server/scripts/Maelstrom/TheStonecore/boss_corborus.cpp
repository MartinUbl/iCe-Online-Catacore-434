/*
 * Copyright (C) 2006-2012 iCe Online <http://www.ice-wow.eu/>
 * Author: HyN3Q, Shadow27
 * THX to Gregory, Arti, Labuz for helping...this is my first boss AI :)
 */

#include "ScriptPCH.h"

enum Stuff
{
    // Corborus stuff
    DATA_CORBORUS          = 0,
    CORBORUS_MOVE          = 1,
    SPELL_CRYSTAL_BARRAGE  = 86881,
    NPC_CRYSTAL_SHARD      = 49267,
    SPELL_DAMPENING_WAVE   = 82415,
    NPC_ROCK_BORE          = 43917,
    SPELL_THRASHING_CHARGE = 81801,
    SPELL_THRASHING_CHARGE_2 = 81828,
    SPELL_SUBMERGE         = 81629,
    SPELL_EMERGE           = 81948,

    // Millhouse Manastorm stuff
    SPELL_BLUR             = 81216,
    SPELL_FEAR             = 81442,
    SPELL_FROSTBOLT_VOLEY = 81440,
    SPELL_IMPENDING_DOOOOOOM = 86830,
    SPELL_IMPENDING_DOOOOOOM_VISUAL = 86838,
    SPELL_SHADOW_BOLT        = 81439,
    SPELL_SHADOWFURY         = 81441,
    NPC_MILLHOUSE            = 43391,
    NPC_MILLHOUSE_2          = 43392,
    NPC_MILLHOUSE_3          = 433930,

    // Data Events
    DATA_MILLHOUSE_EVENT1    = 4,
    DATA_MILLHOUSE_EVENT2    = 5,
    DATA_MILLHOUSE_EVENT3    = 6,
    DATA_CORBORUS_EVENT      = 7
};

static const Position millhouse_spawn[] =
{
    {1160.714355f, 877.769165f, 285.126892f},
    {1162.036865f, 878.184082f, 285.117401f},
    {1163.528809f, 879.465759f, 285.062775f},
    {1164.461548f, 881.653015f, 284.960419f},
    {1164.192261f, 883.773499f, 284.961945f},
    {1163.039673f, 885.289001f, 284.961853f},
    {1161.582275f, 886.412842f, 284.961945f},
    {1160.212402f, 886.713745f, 284.961945f}
};

class boss_corborus: public CreatureScript
{
public:
    boss_corborus() : CreatureScript("boss_corborus") {};

    struct boss_corborusAI : public ScriptedAI
    {
        boss_corborusAI(Creature* c): ScriptedAI(c)
        {
            pInstance = c->GetInstanceScript();
            Reset();

            c->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            c->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
            c->ApplySpellImmune(0, IMMUNITY_ID, 49560, true); // Death Grip jump effect
            c->ApplySpellImmune(0, IMMUNITY_ID, 81261, true); // Solar Beam
            c->ApplySpellImmune(0, IMMUNITY_ID, 88625, true); // Chastise
        }

        InstanceScript* pInstance;
        uint32 phase;
        uint32 dampening_waveTimer;
        uint32 crystal_barrageTimer;
        uint32 crystal_shardTimer;
        uint32 submergeTimer;
        uint32 delay;
        uint32 submergeCounter;
        uint32 ShardsCounter;
        uint32 submergedelayTimer;
        bool BeginPhase;
        bool ShardSpawn;

        void Reset()
        {

            if (me->GetEntry() == 43438) {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_PASSIVE);
            }

            phase = 0;

            // Timers
            dampening_waveTimer = 5000;
            crystal_barrageTimer = 10000;
            crystal_shardTimer = 500;
            delay = 500;
            submergeTimer = 16000;
            submergedelayTimer = 3500;

            // Counters
            submergeCounter = 0;
            ShardsCounter = 0;

            // Boolean
            BeginPhase = false;
            ShardSpawn = false;

            // Safity...
            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE) && me->GetEntry() != 43438)
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
        }

        void SummonedCreatureDies(Creature* pSummon, Unit* pKiller)
        {
            if(pSummon->GetEntry() == NPC_ROCK_BORE)
                pSummon->ForcedDespawn();
        }

        void EnterCombat(Unit* pWho)
        {
            if (pInstance)
                pInstance->SetData(DATA_CORBORUS, IN_PROGRESS);

            phase = 1;
        }

        void SpellHitTarget(Unit* pTarget, const SpellEntry* pSpell)
        {
            if (pSpell->Id == SPELL_THRASHING_CHARGE)
                me->CastSpell(me->getVictim(), SPELL_THRASHING_CHARGE_2, false);
        }

        void DoAction(const int32 Action)
        {
            switch(Action)
            {
                case CORBORUS_MOVE:
                {
                    me->SetSpeed(MOVE_RUN, 6);
                    me->SetSpeed(MOVE_WALK, 6);
                    me->GetMotionMaster()->MovePoint(1, 1156.842407f, 881.401184f, 286.215546f);
                }
                break;
            }
        }

        void JustDied(Unit* pKiller)
        {
            if (pInstance)
                pInstance->SetData(DATA_CORBORUS, DONE);
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            switch(id)
            {
                case 1:
                {
                    if (pInstance)
                    {
                        pInstance->SetData(DATA_CORBORUS_EVENT, DONE);
                        pInstance->SetData(DATA_MILLHOUSE_EVENT3, DONE);
                    }

                    me->SetFacing(3.291410f, NULL);

                    me->SetSpeed(MOVE_RUN, 1);
                    me->SetSpeed(MOVE_WALK, 1);
                    me->StopMoving();
                    me->ForcedDespawn();
                    me->SetPhaseMask(2, false); // safity...
                }
                break;
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (phase == 1)
            {
                if (dampening_waveTimer <= diff)
                {
                    if (!me->IsNonMeleeSpellCasted(false))
                    {
                        DoCast(me->getVictim(), SPELL_DAMPENING_WAVE);
                        dampening_waveTimer = 12000;
                    }
                } else dampening_waveTimer -= diff;

                if (crystal_barrageTimer <= diff)
                {
                    if (!me->IsNonMeleeSpellCasted(false))
                    {
                        if (Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0))
                        {
                            DoCast(pTarget, SPELL_CRYSTAL_BARRAGE);
                            ShardSpawn = true;
                            crystal_shardTimer = 500;
                            ShardsCounter = 0;
                        }
                        crystal_barrageTimer = 10000;
                    }
                } else crystal_barrageTimer -= diff;

                if (IsHeroic() && crystal_shardTimer <= diff && ShardSpawn == true)
                {
                    if (ShardsCounter <= 7)
                    {
                        me->SummonCreature(NPC_CRYSTAL_SHARD, me->getVictim()->GetPositionX(), me->getVictim()->GetPositionY(), me->getVictim()->GetPositionZ(), 0, TEMPSUMMON_DEAD_DESPAWN);
                        ShardsCounter++;
                    }
                    crystal_shardTimer = 500;

                    if (ShardsCounter == 8)
                        ShardSpawn = false;

                } else crystal_shardTimer -= diff;

                if (submergeTimer <= diff) {
                    BeginPhase = true;
                    delay = 500;
                    phase = 2;
                    submergeTimer = 0;
                }
                else submergeTimer -= diff;
            }

            if (phase == 2)
            {
                if (BeginPhase == true)
                {
                    if (delay <= diff)
                    {
                        me->RemoveAllNegativeAuras();
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                        me->CastSpell(me, SPELL_SUBMERGE, false);
                        delay = 0;
                        BeginPhase = false;
                    } else delay -= diff;
                }

                if (submergedelayTimer <= diff)
                {
                    Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0);
                    if (!pTarget)
                        pTarget = me->getVictim();
                    if (pTarget)
                        DoCast(pTarget, SPELL_THRASHING_CHARGE);

                    submergeCounter++;
                    submergedelayTimer = 7000;
                } else submergedelayTimer -= diff;

                if (submergeCounter == 4)
                {
                    if (!me->IsNonMeleeSpellCasted(false))
                    {
                        me->CastSpell(me, SPELL_EMERGE, false);
                        me->RemoveAurasDueToSpell(SPELL_SUBMERGE);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                        dampening_waveTimer = 3300;
                        submergeCounter = 0;
                        submergeTimer = 20000;
                        phase = 1;
                    }
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new boss_corborusAI(c);
    }
};

class mob_millhouse : public CreatureScript
{
public:
    mob_millhouse() : CreatureScript("mob_millhouse") {};
    struct mob_millhouseAI : public ScriptedAI
    {
        mob_millhouseAI(Creature* c): ScriptedAI(c)
        {
            Reset();
            moving = false;
            map = c->GetMap();
            pInstance = c->GetInstanceScript();

            c->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            c->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
            c->ApplySpellImmune(0, IMMUNITY_ID, 49560, true); // Death Grip jump effect
            c->ApplySpellImmune(0, IMMUNITY_ID, 81261, true); // Solar Beam
            c->ApplySpellImmune(0, IMMUNITY_ID, 88625, true); // Chastise
        }

        InstanceScript* pInstance;

        uint32 FearTimer;
        uint32 ShadowBoltTimer;
        uint32 ShadowfuryTimer;
        uint32 FrostboltVoleyTimer;
        uint32 EventStartTimer;
        uint32 GO_removalTimer;
        uint32 millhouse_kill;

        uint32 actualWP;

        Map* map;

        std::list<uint64> millhouse_summon;

        bool moving;
        bool last_scene;
        bool last_move;
        bool start;
        bool yell;

        void Reset()
        {
            FearTimer = 2000;
            ShadowBoltTimer = 5000;
            ShadowfuryTimer = 17000;
            FrostboltVoleyTimer = 19000;
            EventStartTimer = 20000;
            GO_removalTimer = 23000;
            millhouse_kill = 25000;
            last_scene = false;
            last_move = false;
            start = false;
            yell = false;
            actualWP = 0;

            millhouse_summon.clear();
        }

        void DoRandomYells()
        {
            switch (urand(1,6))
            {
                case 1:
                    me->PlayDirectSound(21787);
                    me->MonsterYell("Ah-ha! I've got you right where I want you!", LANG_UNIVERSAL, 0);
                    break;
                case 2:
                    me->PlayDirectSound(21786);
                    me->MonsterYell("Don't say I didn't warn ya!", LANG_UNIVERSAL, 0);
                    break;
                case 3:
                    me->PlayDirectSound(21783);
                    me->MonsterYell("Follow me if you dare!", LANG_UNIVERSAL, 0);
                    break;
                case 4:
                    me->PlayDirectSound(21784);
                    me->MonsterYell("It's time for a tactical retreat!", LANG_UNIVERSAL, 0);
                    break;
                case 5:
                    me->PlayDirectSound(21789);
                    me->MonsterYell("Prison taught me one very important lesson, well, two if you count how to hold your soap, but yes! SURVIVAL!", LANG_UNIVERSAL, 0);
                    break;
                case 6:
                    me->PlayDirectSound(21785);
                    me->MonsterYell("You're gonna be sorry!", LANG_UNIVERSAL, 0);
                    break;
            }
        }

        void DoAction(const int32 action)
        {
            switch (action)
            {
                case 1:
                {
                    me->GetMotionMaster()->MoveJump(1178.472168f, 773.575684f, 315.928223f, 10, 10);
                    if (!millhouse_summon.empty())
                    {
                        for (std::list<uint64>::const_iterator itr = millhouse_summon.begin(); itr != millhouse_summon.end(); ++itr)
                        {
                            Creature* pSummons = NULL;
                            if ((pSummons = map->GetCreature((*itr))) != NULL) {
                                pSummons->GetMotionMaster()->MoveJump(1178.472168f, 773.575684f, 315.928223f, 10, 10);
                            }
                        }
                    }
                    break;
                }
                case 2:
                {
                    last_scene = false;
                    moving = true;
                    break;
                }
                default:
                    break;
            }
        }

        void DoCombatStop()
        {
            me->CombatStop();
            me->getHostileRefManager().deleteReferences();
            Map::PlayerList const& plrList = map->GetPlayers();
            if (plrList.isEmpty())
                return;

            for (Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
            {
                if (Player* pPlayer = itr->getSource())
                    pPlayer->getHostileRefManager().deleteReference(me);
            }
        }

        void DoDestroyForPlayers()
        {
            Map::PlayerList const& plrList = map->GetPlayers();
            if (plrList.isEmpty())
                return;

            for (Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
            {
                if (Player* pPlayer = itr->getSource())
                    me->DestroyForPlayer(pPlayer);
            }
        }

        void EnterCombat(Unit* pUnit)
        {
            if (moving == true)
                return;
            else
            {
                ScriptedAI::EnterCombat(pUnit);
                me->PlayDirectSound(21790);
            }
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            switch (id)
            {
                case 1: // First group
                    actualWP = 1;
                    break;
                case 2: // First group
                    if (pInstance)
                        pInstance->SetData(DATA_MILLHOUSE_EVENT1, DONE);
                    DoDestroyForPlayers();
                    me->SetPhaseMask(2, false);
                    me->Kill(me);
                    break;
                case 3: // Second group
                    actualWP = 2;
                    break;
                case 4: // Second group
                    actualWP = 3;
                    break;
                case 5: // Second group
                    if (pInstance)
                        pInstance->SetData(DATA_MILLHOUSE_EVENT2, DONE);
                    DoDestroyForPlayers();
                    me->SetPhaseMask(2, false);
                    me->Kill(me);
                    break;
               case 6: // third group - begin event
                    if (yell == false) {
                    me->SetFacing(3.291410f, NULL);
                    me->CastSpell(me, SPELL_IMPENDING_DOOOOOOM_VISUAL, false);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                    me->RemoveAurasDueToSpell(SPELL_BLUR);
                    me->SetReactState(REACT_AGGRESSIVE);
                    DoCombatStop();
                    me->PlayDirectSound(21788);
                    me->MonsterYell("Now... Witness the full power of Millhouse Manastorm!", LANG_UNIVERSAL, 0);
                    moving = false;
                    last_scene = true;
                    yell = true;
                    }
                    break;
                default:
                    break;
            }
        }

        void MoveInLineOfSight(Unit* pWho)
        {
            if (last_scene == true)
            {
                if(!pWho || pWho->GetDistance(me) >= 50.0f || pWho->isDead() || pWho->GetTypeId() != TYPEID_PLAYER)
                    return;

                start = true;
                me->CastSpell(me, SPELL_IMPENDING_DOOOOOOM, false);
                for (uint32 i = 0; i < 8; i++)
                {
                    Creature* pSummon = me->SummonCreature(43430, millhouse_spawn[i].GetPositionX(),millhouse_spawn[i].GetPositionY(),millhouse_spawn[i].GetPositionZ(),0,TEMPSUMMON_MANUAL_DESPAWN);
                    pSummon->SetFacing(me->GetOrientation(), NULL);
                    pSummon->SetReactState(REACT_PASSIVE);
                    millhouse_summon.push_back(pSummon->GetGUID());
                }

                last_scene = false;
                moving = true;
            }
            ScriptedAI::MoveInLineOfSight(pWho);
        }

        void DamageTaken(Unit* pTarget, uint32& damage)
        {
            if (damage >= me->GetHealth()) // Preserve Millhouse must be alive !
                damage = 0;
        }

        void UpdateAI(const uint32 diff)
        {
            if (actualWP > 0)
            {
                switch (actualWP)
                {
                    case 1:
                        me->GetMotionMaster()->MovePoint(2, 977.245544f, 895.039368f, 306.309204f);
                        break;
                    case 2:
                        me->GetMotionMaster()->MovePoint(4, 1011.411682f, 876.214539f, 299.778900f);
                        break;
                    case 3:
                        me->GetMotionMaster()->MovePoint(5, 1059.388672f, 867.483948f, 293.764282f);
                        break;
                }
                actualWP = 0;
            }

            if (start == true)
            {
                GameObject* pGO = me->FindNearestGameObject(4510265,60);
                if (EventStartTimer <= diff)
                {
                    if (pInstance)
                        pInstance->SetData(DATA_CORBORUS_EVENT, IN_PROGRESS);

                    if (pGO != NULL)
                        pGO->SetGoState(GO_STATE_ACTIVE);

                    EventStartTimer = 20000;

                } else EventStartTimer -= diff;

                if (GO_removalTimer <= diff)
                {

                    if (pGO != NULL) {
                        Map::PlayerList const& plrList = map->GetPlayers();
                        if (plrList.isEmpty())
                            return;

                        for (Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
                        {
                            if (Player* pPlayer = itr->getSource())
                            {
                                pGO->Delete();
                                pGO->DestroyForPlayer(pPlayer, true);
                            }
                        }
                        me->SetPhaseMask(2, false);
                    }

                    GO_removalTimer = 23000;
                } else GO_removalTimer -= diff;

                if (millhouse_kill <= diff)
                {
                    me->Kill(me);
                    for (std::list<uint64>::const_iterator itr = millhouse_summon.begin(); itr != millhouse_summon.end(); ++itr)
                    {
                        Creature* pSummons = NULL;
                        if ((pSummons = map->GetCreature((*itr))) != NULL)
                            pSummons->SetPhaseMask(2, false);
                    }
                    millhouse_kill = 25000;
                    start = false;
                } else millhouse_kill -= diff;
            }

            if (!UpdateVictim())
                return;

            if (FearTimer <= diff && (last_scene == false  || moving == false))
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0);
                    me->CastSpell(pTarget, SPELL_FEAR, false);
                    FearTimer = 20000;
                }
            } else FearTimer -= diff;

            if (ShadowBoltTimer <= diff && (last_scene == false  || moving == false))
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0);
                    me->CastSpell(pTarget, SPELL_SHADOW_BOLT, false);
                    ShadowBoltTimer = 8000;
                }
            } else ShadowBoltTimer -= diff;

            if (ShadowfuryTimer <= diff && (last_scene == false || moving == false))
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM,0);
                    me->CastSpell(pTarget, SPELL_SHADOWFURY, false);
                    ShadowfuryTimer = 15000;
                }
                else
                    ShadowfuryTimer += 500;
            } else ShadowfuryTimer -= diff;

            if (FrostboltVoleyTimer <= diff && (last_scene == false  || moving == false))
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    me->CastSpell(me, SPELL_FROSTBOLT_VOLEY, false);
                    FrostboltVoleyTimer = 18000;
                }
            } else FrostboltVoleyTimer -= diff;

            if (HealthBelowPct(50))
            {
                moving = true;

                if (me->GetCurrentSpell(CURRENT_GENERIC_SPELL))
                    me->InterruptSpell(CURRENT_GENERIC_SPELL);

                me->CastSpell(me, SPELL_BLUR, false);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_PASSIVE);
                switch (me->GetEntry())
                {
                    case NPC_MILLHOUSE:
                        me->GetMotionMaster()->MovePoint(1, 950.461121f, 947.477112f, 314.925171f);
                        break;
                    case NPC_MILLHOUSE_2:
                        me->GetMotionMaster()->MovePoint(3, 1009.400696f, 875.966431f, 300.089447f);
                        break;
                    case NPC_MILLHOUSE_3:
                        if (pInstance) {
                            pInstance->SetData(DATA_MILLHOUSE_EVENT3, IN_PROGRESS);
                        }

                        last_move = true;
                        break;
                }

                if (last_move == false)
                    DoRandomYells();
            }

            DoMeleeAttackIfReady();
        }

    };

        CreatureAI* GetAI(Creature* c) const
        {
            return new mob_millhouseAI(c);
        }
};

class mob_stonecore_earthshaper : public CreatureScript
{
public:
    mob_stonecore_earthshaper() : CreatureScript("mob_stonecore_earthshaper") {};
    struct mob_stonecore_earthshaperAI : public ScriptedAI
    {
        mob_stonecore_earthshaperAI(Creature* c): ScriptedAI(c)
        {
            Reset();
            pos = me->GetHomePosition();
        }

        uint32 ground_shockTimer;
        uint32 lava_burstTimer;
        uint32 dust_stormTimer;
        Position pos;

        void Reset()
        {
            ground_shockTimer = urand(8000, 12000);
            lava_burstTimer = urand(5000, 10000);
            dust_stormTimer = 2000;

            if (me->GetEntry() == 43552 || me->GetEntry() == 49649)
                me->SetHomePosition(pos);
        }

        void JustReachedHome()
        {
            if (me->GetEntry() == 43552)
                me->UpdateEntry(IsHeroic() ? 49649 : 43537);
        }

        void EnterCombat(Unit* who) { }

        void SpellHit(Unit* target, SpellEntry const* pSpell)
        {
            if (pSpell->Id == 81459)
                me->UpdateEntry(IsHeroic() ? 49649 : 43552);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (me->GetEntry() == 43552 || me->GetEntry() == 49649)
            {
                if (dust_stormTimer <= diff)
                {
                    if (!me->IsNonMeleeSpellCasted(true))
                    {
                        me->CastSpell(me, 81463, false);
                        dust_stormTimer = urand(10000, 12000);
                    }
                    else dust_stormTimer = 12000;
                }else dust_stormTimer -= diff;
            }
            else
            {
                if (HealthBelowPct(95) || HealthBelowPct(30))
                {
                    if (!me->IsNonMeleeSpellCasted(false))
                        me->CastSpell(me, 81459, false);
                }

                if (ground_shockTimer <= diff)
                {
                    if (!me->IsNonMeleeSpellCasted(false))
                        me->CastSpell(me, 81530, false);

                    ground_shockTimer = urand(15000, 19000);
                } else ground_shockTimer -= diff;

                if (lava_burstTimer <= diff)
                {
                    if (!me->IsNonMeleeSpellCasted(false))
                    {
                        Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0);
                        me->CastSpell(pTarget, 81576, false);
                        lava_burstTimer = urand(5000, 10000);
                    }
                } else lava_burstTimer -= diff;
            }

            DoMeleeAttackIfReady();
        }

    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new mob_stonecore_earthshaperAI(c);
    }
};
void AddSC_corborus()
{
    new boss_corborus();
    new mob_millhouse();
    new mob_stonecore_earthshaper();
}

/*
UPDATE creature_template SET scriptname='boss_corborus',AIName='' WHERE entry = 43438;
DELETE FROM creature_ai_scripts WHERE creature_id IN (43438,43391,43537,43552);
UPDATE gameobject_template SET name = 'DEEPHOLM_ROCKDOOR_BREAK', data1 = 0 WHERE entry = 4510265;
UPDATE creature_template SET scriptname='mob_stonecore_earthshaper',AIName='',faction_a=14,faction_h=14,minlevel = 81,maxlevel=81,health_mod='6.7282' WHERE entry IN (43552, 43537);
UPDATE creature_template SET scriptname='mob_stonecore_earthshaper', faction_a=14,faction_h=14,minlevel=85,maxlevel=85,health_mod='6.4166' where entry = 49649;
UPDATE creature_template SET scriptname='mob_millhouse', AIName='' WHERE entry = 43391;
UPDATE creature_template SET health_mod='3.9880' WHERE entry = 43552;
UPDATE creature_template SET health_mod='8.3362', faction_a=14,faction_h=14 WHERE entry = 49662;

INSERT INTO `creature_template` (`entry`,`difficulty_entry_1`,`modelid1`,`name`,`minlevel`,`maxlevel`,`exp`,`faction_A`,`faction_H`,`rank`,`mindmg`,`maxdmg`,`baseattacktime`,`unit_class`,`type`,`Health_mod`,`Mana_mod`,`ScriptName`)
    VALUES ('43392','49653','37234','Millhouse Manastorm','82','83','3','14','14','1','6000','8000','2000','2','7','8.6507','45.7143','mob_millhouse'),
        ('433930','49653','37234','Millhouse Manastorm','82','83','3','14','14','1','6000','8000','2000','2','7','8.6507','45.7143','mob_millhouse');
INSERT INTO creature_template
  (entry, difficulty_entry_1, difficulty_entry_2, difficulty_entry_3, KillCredit1, KillCredit2, modelid1, modelid2, modelid3, modelid4, name, subname, IconName, gossip_menu_id, minlevel, maxlevel, exp, faction_A, faction_H, npcflag, speed_walk, speed_run, scale, rank, mindmg, maxdmg, dmgschool, attackpower, dmg_multiplier, baseattacktime, rangeattacktime, unit_class, unit_flags, dynamicflags, family, trainer_type, trainer_spell, trainer_class, trainer_race, minrangedmg, maxrangedmg, rangedattackpower, type, type_flags, lootid, pickpocketloot, skinloot, resistance1, resistance2, resistance3, resistance4, resistance5, resistance6, spell1, spell2, spell3, spell4, spell5, spell6, spell7, spell8, PetSpellDataId, VehicleId, mingold, maxgold, AIName, MovementType, InhabitType, Health_mod, Mana_mod, Armor_mod, RacialLeader, questItem1, questItem2, questItem3, questItem4, questItem5, questItem6, movementId, RegenHealth, equipment_id, mechanic_immune_mask, flags_extra, ScriptName, WDBVerified)
VALUES
  (434380, 49642, 0, 0, 0, 0, 33477, 0, 0, 0, "Corborus", "", "", 0, 81, 81, 3, 14, 14, 0, 1, 1.14286, 1, 1, 13000, 15000, 0, 0, 1, 2000, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 72, 43438, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10002, 20002, "EventAI", 0, 3, 43.3592, 0, 1, 0, 52506, 0, 0, 0, 0, 0, 154, 1, 0, 646922239, 0, "boss_corborus", 1);

INSERT INTO creature
  (id, map, spawnMask, phaseMask, modelid, equipment_id, position_x, position_y, position_z, orientation, spawntimesecs, spawndist, currentwaypoint, curhealth, curmana, DeathState, MovementType, npcflag, unit_flags, dynamicflags)
VALUES
  (43392, 725, 3, 2, 0, 0, 977.227, 888.717, 305.362, 0.663324, 86400, 0, 0, 513246, 192380, 0, 0, 0, 0, 0), (433930, 725, 3, 2, 0, 0, 1062.55, 867.532, 293.469, 1.83473, 86400, 0, 0, 513246, 192380, 0, 0, 0, 0, 0),
  (434380, 725, 3, 2, 0, 0, 1159.725342, 881.786621, 284.963928,3.291410, 86400, 0, 0, 513246, 192380, 0, 0, 0, 0, 0);

INSERT INTO gameobject
  (id, map, spawnMask, phaseMask, position_x, position_y, position_z, orientation, rotation0, rotation1, rotation2, rotation3, spawntimesecs, animprogress, state)
VALUES
  (4510265, 725, 3, 1, 1154.06, 919.607, 284.439, 3.01255, 0, 0, 0.997919, 0.0644758, 300, 0, 1);
*/
