/*
 * Copyright (C) 2006-2012 iCe Online <http://www.ice-wow.eu/>
 * Author: HyN3Q, Shadow27
 */

#include "ScriptPCH.h"
#include "the_stonecore.h"

enum Stuff
{
    // Corborus stuff
    SPELL_CRYSTAL_BARRAGE  = 86881,
    NPC_CRYSTAL_SHARD      = 49267,
    SPELL_DAMPENING_WAVE   = 82415,
    NPC_ROCK_BORE          = 43917,
    SPELL_THRASHING_CHARGE = 81801,
    SPELL_TRASHING_CHARGE_2 = 81828,
    SPELL_SUBMERGE         = 81629,
    SPELL_EMERGE           = /*56864*/66949,
    // Millhouse Manastorm stuff
    SPELL_BLUR             = 81216,
    SPELL_FEAR             = 81442,
    SPELL_FROSTBOLT_VOLEY = 81440,
    SPELL_IMPENDING_DOOOOOOM = 86830,
    SPELL_IMPENDING_DOOOOOOM_VISUAL = 86838,
    SPELL_SHADOW_BOLT        = 81439,
    SPELL_SHADOWFURY         = 81441,
    SPELL_TIGULE_AND_FOROR   = 81220,
    NPC_MILLHOUSE            = 43391,
    NPC_MILLHOUSE_2          = 43392,
    NPC_MILLHOUSE_3          = 433930,
    NPC_UNBOUND_EARTH_RAGER  = 43662
};

bool EventMillhouse = false;
bool IsBossMoved = false;

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

/*
##################### enter combat #####################
po 5s dampening wave , vypnout timer
po 12 sec crystal barrage // na HC vysummonit navic kazdych 0.5 sec 1 kus crystal shard (max 8)
Prvni zahrabani 16 sec - do zeme SPELL_SUBMERGE
shodit dotky
 #################### v zemi ####################
 -------- opakovat 4x --------
 hodit untargetable
 0,5 sec prodleva
 nahodny target
 zacastit caru (3.5 sec)
 zacastit trashing charge (instant)
 po 5ti sec vyssummonit rock bore
 ------- opakovat 4x ---------
 vylezt == sundat auru SUBMERGE a ihned zacastit EMERGE
 sundat untargetable
 opakovat za 20 sec
 #################### vylezt ven ####################
 prvnich 3 sec vycastit SPELL_DAMPENING_WAVE, nastavit timer na 12 sec
 po 10 sec vyvolat crystal barrage // na HC vysummonit navic kazdych 0.5 sec 1 kus crystal shard (max 8)
 po 14 sec vycastit SPELL_DAMPENING_WAVE
 po 20 sec opakovat #################### v zemi ####################
*/

class boss_corborus: public CreatureScript
{
public:
    boss_corborus() : CreatureScript("boss_corborus") {};

    struct boss_corborusAI : public ScriptedAI
    {
        boss_corborusAI(Creature* c): ScriptedAI(c)
        {
            pInstance = me->GetInstanceScript();
            Reset();
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
        uint32 damage;
        bool BeginPhase;
        bool ShardSpawn;

        void Reset()
        {
            if (pInstance)
                pInstance->SetData(0, NOT_STARTED);

            phase = 0;

            // Timers
            dampening_waveTimer = 5000;
            crystal_barrageTimer = 10000;
            crystal_shardTimer = 500;
            delay = 500;
            submergeTimer = 16000;
            submergeCounter = 0;
            ShardsCounter = 0;
            submergedelayTimer = 3500;
            BeginPhase = false;
            ShardSpawn = false;
            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE))
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);

            if (IsBossMoved == false)
                me->SetHomePosition(1148.062134f, 939.537964f, 284.398804f, 4.921068f);
        }

        void SummonedCreatureDies(Creature* pSummon, Unit* pKiller)
        {
            if(pSummon->GetEntry() == NPC_ROCK_BORE)
                pSummon->ForcedDespawn();
        }

        void EnterCombat(Unit* pWho)
        {
            if (pInstance)
                pInstance->SetData(0, IN_PROGRESS);

            phase = 1;
        }

        void SpellHitTarget(Unit* pTarget, const SpellEntry* spell)
        {
            if (!spell || !pTarget)
                return;

            if (spell->Id == SPELL_THRASHING_CHARGE)
                me->CastSpell(me, SPELL_TRASHING_CHARGE_2, false);
        }

        void JustDied(Unit* pKiller)
        {
            if (pInstance)
                pInstance->SetData(0, DONE);
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == 1)
            {
                IsBossMoved = true;
                me->MonsterSay("Do daneho pointu dobehnu", LANG_UNIVERSAL, 0);
                me->SetFacing(3.291410f, NULL);
                me->SetHomePosition(me->GetPositionX(),me->GetPositionY(),me->GetPositionZ(),me->GetOrientation());
                me->SetSpeed(MOVE_RUN, 1);
                Creature* millhouse = me->GetMap()->GetCreature(me->GetInstanceScript()->GetData64(4));
                if (millhouse != NULL)
                    millhouse->MonsterSay("Wazaaaaaaaaaaaaaaap!", LANG_UNIVERSAL, 0);
                    //millhouse->GetMotionMaster()->MoveKnockbackFrom(1115.535034f,763.702759f,7,7);
            }

        }

        void UpdateAI(const uint32 diff)
        {

            if (EventMillhouse == true)
            {
                me->SetSpeed(MOVE_RUN, 6);
                me->GetMotionMaster()->MovePoint(1, 1156.842407f, 881.401184f, 286.215546f);
                EventMillhouse = false;
            }

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
                        if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM,0, 1000,true))
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
                        me->SummonCreature(NPC_CRYSTAL_SHARD,me->getVictim()->GetPositionX(),me->getVictim()->GetPositionY(),me->getVictim()->GetPositionZ(),0, TEMPSUMMON_DEAD_DESPAWN);
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
                if (delay <= diff && BeginPhase == true)
                {
                    me->RemoveAllNegativeAuras();
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                    me->CastSpell(me, SPELL_SUBMERGE, false);
                    delay = 0;
                    BeginPhase = false;
                } else delay -= diff;

                if (submergedelayTimer <= diff)
                {
                    Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM,0, 1000,true);
                    if (!pTarget)
                        pTarget = me->getVictim();
                    if (pTarget)
                        DoCast(pTarget, SPELL_THRASHING_CHARGE);

                    submergeCounter++;
                    submergedelayTimer = 3800;
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
            Moving = false;
            evade = false;
            map = c->GetMap();
            pInstance = c->GetInstanceScript();
        }

        InstanceScript* pInstance;

        uint32 FearTimer;
        uint32 ShadowBoltTimer;
        uint32 ShadowfuryTimer;
        uint32 FrostboltVoleyTimer;
        uint32 BeginEvent;
        uint32 cavein_delete;
        uint32 actualWP;
        Map* map;
        bool begin_event;
        bool Moving;
        bool last_scene;
        bool last_move;
        bool move;
        bool evade;

        void Reset()
        {
            FearTimer = 2000;
            ShadowBoltTimer = 5000;
            ShadowfuryTimer = 15000;
            FrostboltVoleyTimer = 18000;
            BeginEvent = 20000;
            cavein_delete = 23000;
            last_scene = false;
            last_move = false;
            move = false;
            begin_event = false;
            actualWP = 0;
        }

        void RandomYells()
        {
            switch (urand(1,6))
            {
                case 1:
                    me->MonsterYell("Ah-ha! I've got you right where I want you!", LANG_UNIVERSAL, 0);
                    break;
                case 2:
                    me->MonsterYell("Don't say I didn't warn ya!", LANG_UNIVERSAL, 0);
                    break;
                case 3:
                    me->MonsterYell("Follow me if you dare!", LANG_UNIVERSAL, 0);
                    break;
                case 4:
                    me->MonsterYell("It's time for a tactical retreat!", LANG_UNIVERSAL, 0);
                    break;
                case 5:
                    me->MonsterYell("Prison taught me one very important lesson, well, two if you count how to hold your soap, but yes! SURVIVAL!", LANG_UNIVERSAL, 0);
                    break;
                case 6:
                    me->MonsterYell("You're gonna be sorry!", LANG_UNIVERSAL, 0);
                    break;
            }
        }

       void CombatStop()
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

        void MoveInLineOfSight(Unit* pWho)
        {
            Player* pTarget = pWho->ToPlayer();
            if(!pTarget)
                return;

            if (last_scene == true)
            {
                if (me->GetDistance(pTarget) >= 50)
                    return;

                    me->CastSpell(me, SPELL_IMPENDING_DOOOOOOM, false);
                    for (uint32 i = 0; i < 8; i++)
                    {
                        Creature* pSummon = me->SummonCreature(NPC_UNBOUND_EARTH_RAGER, millhouse_spawn[i].GetPositionX(),millhouse_spawn[i].GetPositionY(),millhouse_spawn[i].GetPositionZ(),0,TEMPSUMMON_MANUAL_DESPAWN);
                        pSummon->SetReactState(REACT_PASSIVE);
                    }
                    begin_event = true;
                    last_scene = false;
            }
            ScriptedAI::MoveInLineOfSight(pWho);
        }

        void EnterCombat(Unit* pUnit)
        {
            if (Moving == true)
                return;
            else
            {
                ScriptedAI::EnterCombat(pUnit);
                RandomYells();
            }
        }

        void EnterEvadeMode()
        {
            return;
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
                    me->SummonCreature(NPC_MILLHOUSE_2, 976.619202f, 889.058838f, 305.502899f,0,TEMPSUMMON_MANUAL_DESPAWN);
                    me->ForcedDespawn();
                    break;
                case 3: // Second group
                    actualWP = 2;
                    break;
                case 4: // Second group
                    actualWP = 3;
                    break;
                case 5: // Second group
                    me->SummonCreature(NPC_MILLHOUSE_3, 1062.914185f, 867.131470f, 293.436096f,1.788949f,TEMPSUMMON_MANUAL_DESPAWN);
                    me->ForcedDespawn();
                    break;
               case 6: // third group - begin event
                    me->SetFacing(3.291410f, NULL);
                    me->CastSpell(me, SPELL_IMPENDING_DOOOOOOM_VISUAL, false);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                    me->RemoveAurasDueToSpell(SPELL_BLUR);
                    me->SetReactState(REACT_AGGRESSIVE);
                    CombatStop();
                    Moving = false;
                    last_scene = true;
                    break;
                default:
                    break;
            }
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

            if (BeginEvent <= diff && begin_event == true)
            {
                EventMillhouse = true;
                GameObject* pGO = me->FindNearestGameObject(4510265,100);

                if (pGO != NULL)
                    pGO->SetGoState(GO_STATE_ACTIVE);

                BeginEvent = 0;
                begin_event = false;
            } else BeginEvent -= diff;

            /*if (cavein_delete <= diff && begin_event == true)
            {
                GameObject* pGO = me->FindNearestGameObject(452230,100);

                if (pGO != NULL)
                    pGO->Delete();

                cavein_delete = 0;
                begin_event = false;
            } else cavein_delete -= diff;*/

            if (!UpdateVictim())
                return;

            if (FearTimer <= diff && (last_scene == false  || Moving == false))
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM,0, 80,true);
                    me->CastSpell(pTarget, SPELL_FEAR, false);
                    FearTimer = 20000;
                }
            } else FearTimer -= diff;

            if (ShadowBoltTimer <= diff && (last_scene == false  || Moving == false))
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM,0, 80,true);
                    me->CastSpell(pTarget, SPELL_SHADOW_BOLT, false);
                    ShadowBoltTimer = 8000;
                }
            } else ShadowBoltTimer -= diff;

            if (ShadowfuryTimer <= diff && (last_scene == false || Moving == false))
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM,0, 80,true);
                    me->CastSpell(pTarget, SPELL_SHADOWFURY, false);
                    ShadowfuryTimer = 15000;
                }
            } else ShadowfuryTimer -= diff;

            if (FrostboltVoleyTimer <= diff && (last_scene == false  || Moving == false))
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    me->CastSpell(me, SPELL_FROSTBOLT_VOLEY, false);
                    FrostboltVoleyTimer = 18000;
                }
            } else FrostboltVoleyTimer -= diff;

            if (HealthBelowPct(50))
                evade = true;

            if (evade == true)
            {
                Moving = true;

                if (me->GetCurrentSpell(CURRENT_GENERIC_SPELL))
                    me->InterruptSpell(CURRENT_GENERIC_SPELL);

                me->CastSpell(me, SPELL_BLUR, false);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_PASSIVE);
                //me->CastSpell(me, SPELL_TIGULE_AND_FOROR, false);
                switch (me->GetEntry())
                {
                    case NPC_MILLHOUSE:
                        me->GetMotionMaster()->MovePoint(1, 950.461121f, 947.477112f, 314.925171f);
                        break;
                    case NPC_MILLHOUSE_2:
                        me->GetMotionMaster()->MovePoint(3, 1009.400696f, 875.966431f, 300.089447f);
                        break;
                    case NPC_MILLHOUSE_3:
                        me->GetMotionMaster()->MovePoint(6, 1159.725342f, 881.786621f, 284.963928f);
                        last_move = true;
                        break;
                }

                if (last_move == false)
                    RandomYells();
                else
                    me->MonsterYell("Now... Witness the full power of Millhouse Manastorm!", LANG_UNIVERSAL, 0);

                evade = false;

            }

            DoMeleeAttackIfReady();
        }

    };

        CreatureAI* GetAI(Creature* c) const
        {
            return new mob_millhouseAI(c);
        }
};

void AddSC_corborus()
{
    new boss_corborus();
    new mob_millhouse();
}

/*
UPDATE creature_template SET scriptname='boss_corborus',AIName='' WHERE entry = 43438;
DELETE FROM creature_ai_scripts WHERE creature_id IN (43438,43391);
UPDATE creature_template SET scriptname='mob_millhouse', AIName='' WHERE entry = 43391;
INSERT INTO `creature_template` (`entry`,`difficulty_entry_1`,`modelid1`,`name`,`minlevel`,`maxlevel`,`exp`,`faction_A`,`faction_H`,`rank`,`mindmg`,`maxdmg`,`baseattacktime`,`unit_class`,`type`,`Health_mod`,`Mana_mod`,`movementId`,`ScriptName`)
    VALUES ('43392','49653','37234','Millhouse Manastorm','82','83','3','14','14','1','6000','8000','2000','2','7','8.6507','45.7143','841','mob_millhouse'),
        ('433930','49653','37234','Millhouse Manastorm','82','83','3','14','14','1','6000','8000','2000','2','7','8.6507','45.7143','841','mob_millhouse');
*/
