/*
 * Copyright (C) 2006-2013 iCe Online <http://www.ice-wow.eu/>
 * Author: HyN3Q
 * THX to Gregory, Ojaaa, ZabakTwist for helping..
 */

#include "ScriptPCH.h"
#include "zulaman.h"

/*
to-do:
write AI for amani kiddnaper
fix Plucked (if needed)
*/

enum Spells
{
    SPELL_STATIC_DISRUPTION     = 43622,
    SPELL_STATIC_VISUAL         = 45265,
    SPELL_CALL_LIGHTNING        = 43661,
    SPELL_GUST_OF_WIND          = 43621,
    SPELL_ELECTRICAL_STORM      = 43648,
    SPELL_BERSERK               = 45078,
    SPELL_ELECTRICAL_DAMAGE     = 43657,
    SPELL_ELECTRICAL_OVERLOAD   = 43658,
    SPELL_EAGLE_SWOOP           = 44732,
    SPELL_PLUCKED               = 97318
};

//"Your death gonna be quick, strangers. You shoulda never have come to this place..."
#define SAY_ONAGGRO "I be da predator! You da prey..."
#define SAY_ONDEATH "You can't... kill... me spirit!"
#define SAY_ONSLAY1 "Ya got nothin'!"
#define SAY_ONSLAY2 "Stop your cryin'!"
#define SAY_ONSUMMON "Feed, me bruddahs!"
#define SAY_ONENRAGE "All you be doing is wasting my time!"
#define SOUND_ONAGGRO 12013
#define SOUND_ONDEATH 12019
#define SOUND_ONSLAY1 12017
#define SOUND_ONSLAY2 12018
#define SOUND_ONSUMMON 12014
#define SOUND_ONENRAGE 12016

enum NPCs
{
    npc_soaring_eagle = 24858,
    npc_electrical_storm_vehicle = 509449,
    npc_amani_kiddnaper = 52638
};

class boss_akilzon : public CreatureScript
{
    public:

        boss_akilzon()
            : CreatureScript("boss_akilzon") { }

        struct boss_akilzonAI : public ScriptedAI
        {
            boss_akilzonAI(Creature *c) : ScriptedAI(c)
            {
                SpellEntry *TempSpell = GET_SPELL(SPELL_ELECTRICAL_DAMAGE);

                if (TempSpell)
                    TempSpell->EffectBasePoints[1] = 49; //disable bugged lightning until fixed in core

                pInstance = c->GetInstanceScript();
            }

            InstanceScript *pInstance;

            uint32 CallLightningCounter;
            uint32 SummonEaglesCounter;
            uint32 AmaniKiddnaperCounter;
            uint64 BirdGUIDs[6];
            uint64 AmaniKiddnaperGUID;

            uint32 StaticDisruptionTimer;
            uint32 ElectricalStormTimer;

            uint32 EnrageTimer;
            uint32 sayTick;

            void Reset()
            {
                if (pInstance)
                    pInstance->SetData(DATA_AKILZONEVENT, NOT_STARTED);

                StaticDisruptionTimer = 10000;
                ElectricalStormTimer = 48000; // 48 seconds(bosskillers)
                sayTick = 500;
                EnrageTimer = 600000;
                SummonEaglesCounter = 0;
                CallLightningCounter = 0;
                AmaniKiddnaperCounter = 0;

                DespawnSummons();

                for (uint8 i = 0; i < 6; ++i)
                    BirdGUIDs[i] = 0;

                AmaniKiddnaperGUID = 0;
            }

            void EnterCombat(Unit * /*who*/)
            {

                me->MonsterYell(SAY_ONAGGRO, LANG_UNIVERSAL, 0);
                DoPlaySoundToSet(me, SOUND_ONAGGRO);

                if (pInstance)
                    pInstance->SetData(DATA_AKILZONEVENT, IN_PROGRESS);
            }

            void JustDied(Unit* /*Killer*/)
            {
                me->MonsterYell(SAY_ONDEATH,LANG_UNIVERSAL,0);
                DoPlaySoundToSet(me, SOUND_ONDEATH);

                if (pInstance)
                    pInstance->SetData(DATA_AKILZONEVENT, DONE);

                DespawnSummons();
            }

            void KilledUnit(Unit* /*victim*/)
            {
                switch (urand(0, 1))
                {
                    case 0:
                        me->MonsterYell(SAY_ONSLAY1, LANG_UNIVERSAL, 0);
                        DoPlaySoundToSet(me, SOUND_ONSLAY1);
                        break;
                    case 1:
                        me->MonsterYell(SAY_ONSLAY2, LANG_UNIVERSAL, 0);
                        DoPlaySoundToSet(me, SOUND_ONSLAY2);
                        break;
                }
            }

            void DespawnSummons()
            {
                for (uint8 i = 0; i < 6; ++i)
                {
                    Unit* bird = Unit::GetUnit(*me,BirdGUIDs[i]);
                    if (bird && bird->isAlive())
                    {
                        bird->SetVisibility(VISIBILITY_OFF);
                        bird->setDeathState(JUST_DIED);
                    }
                }

                if (Unit* pBird = Unit::GetUnit(*me, AmaniKiddnaperGUID))
                {
                    pBird->SetVisibility(VISIBILITY_OFF);
                    pBird->setDeathState(JUST_DIED);
                }
            }

            void MovementInform(uint32 type, uint32 id)
            {
                if (type != POINT_MOTION_TYPE)
                    return;

                if (id == 0)
                {
                    me->GetMotionMaster()->MoveChase(me->getVictim());
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->getVictim()->GetPositionZMinusOffset() > me->GetPositionZ()+5)
                {
                    if (!me->IsNonMeleeSpellCasted(false))
                    {
                        me->GetMotionMaster()->Clear();
                        me->GetMotionMaster()->MoveFall();
                        float x,y,z;
                        me->getVictim()->GetPosition(x,y,z);
                        me->GetMotionMaster()->MovePoint(0, x, y, me->GetPositionZ());
                     }
                }

                if (EnrageTimer <= diff)
                {
                    me->MonsterYell(SAY_ONENRAGE, LANG_UNIVERSAL, 0);
                    DoPlaySoundToSet(me, SOUND_ONENRAGE);
                    DoCast(me, SPELL_BERSERK, true);
                    EnrageTimer = 600000;
                } else EnrageTimer -= diff;

                if (StaticDisruptionTimer <= diff) // blizzlike status: done
                {
                    Unit *pTarget = SelectUnit(SELECT_TARGET_RANDOM, 1);

                    if (!pTarget)
                        pTarget = me->getVictim();

                    DoCast(pTarget, SPELL_STATIC_DISRUPTION, false);
                    me->SetInFront(me->getVictim());

                    StaticDisruptionTimer = 10000;
                } else StaticDisruptionTimer -= diff;

                if (100.0f - me->GetHealthPct() > CallLightningCounter * 20) // blizzlike status: done
                {
                    // Call Lightning
                    CallLightningCounter++;

                    DoCast(me->getVictim(), SPELL_CALL_LIGHTNING);
                }

                if (ElectricalStormTimer <= diff) // blizzlike status: done
                {
                    Unit *pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 50, true);

                    if (pTarget)
                    {
                        float x,y,z;
                        pTarget->GetPosition(x,y,z);

                        Unit* pVehicle = me->SummonCreature(npc_electrical_storm_vehicle, x, y, me->GetPositionZ(), pTarget->GetOrientation(), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 80000);

                        if (pVehicle)
                        {
                            pTarget->StopMoving();
                            pVehicle->SetUnitMovementFlags(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING);
                            pVehicle->setFaction(pTarget->getFaction());
                            pVehicle->SetDisplayId(11686);
                            pVehicle->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            pTarget->EnterVehicle(pVehicle, 0);
                            Vehicle* pVeh = pVehicle->GetVehicleKit();

                            if (pVeh)
                            {
                                pVehicle->GetMotionMaster()->MoveJump(x,y,me->GetPositionZ()+7,5,1);
                                pVehicle->SetFacingTo(pTarget->GetOrientation());
                                pVeh->GetPassenger(0)->clearUnitState(UNIT_STAT_UNATTACKABLE);
                            }

                            DoCast(pTarget, SPELL_ELECTRICAL_STORM, false);
                        }
                    }

                    ElectricalStormTimer = 50000;
                } else ElectricalStormTimer -= diff;

                if (100.0f - me->GetHealthPct() > SummonEaglesCounter * 5) // blizzlike status: done
                {
                    if (sayTick <= diff)
                    {
                        me->MonsterYell(SAY_ONSUMMON, LANG_UNIVERSAL, 0);
                        DoPlaySoundToSet(me, SOUND_ONSUMMON);
                        int32 new_timer = 500 - (diff - sayTick); // Should not get delayed
                        sayTick = (new_timer > 0) ? (uint32)new_timer : 0;
                    } else sayTick -= diff;

                    SummonEaglesCounter++;

                    float x, y, z;
                    me->GetPosition(x, y, z);

                    for (uint8 i = 0; i < urand(3, 6); i++)
                    {
                        Unit* bird = Unit::GetUnit(*me,BirdGUIDs[i]);

                        if (!bird) //they despawned on die
                        {
                            if (Unit *pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0))
                            {
                                x = pTarget->GetPositionX() + irand(-10,10);
                                y = pTarget->GetPositionY() + irand(-10,10);
                                z = pTarget->GetPositionZ() + urand(16,20);
                                if (z > 95)
                                    z = 95.0f - urand(0,5);
                            }

                            Creature *pCreature = me->SummonCreature(npc_soaring_eagle, x, y, z, 0, TEMPSUMMON_CORPSE_DESPAWN, 0);
                            if (pCreature)
                            {
                                pCreature->AddThreat(me->getVictim(), 1.0f);
                                pCreature->AI()->AttackStart(me->getVictim());
                                BirdGUIDs[i] = pCreature->GetGUID();
                            }
                        }
                    }
                }

                if (100.0f - me->GetHealthPct() > AmaniKiddnaperCounter * 10)
                {
                    AmaniKiddnaperCounter++;

                    float x, y, z;
                    me->GetPosition(x, y, z);

                    Unit* bird = Unit::GetUnit(*me, AmaniKiddnaperGUID);

                    if (!bird) //they despawned on die
                    {
                        if (Unit *pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0))
                        {
                            x = pTarget->GetPositionX() + irand(-10,10);
                            y = pTarget->GetPositionY() + irand(-10,10);
                            z = pTarget->GetPositionZ() + urand(16,20);
                            if (z > 95)
                                z = 95.0f - urand(0,5);
                        }

                        Creature *pCreature = me->SummonCreature(npc_amani_kiddnaper, x, y, z, 0, TEMPSUMMON_CORPSE_DESPAWN, 0);
                        if (pCreature)
                        {
                            pCreature->AddThreat(me->getVictim(), 1.0f);
                            pCreature->AI()->AttackStart(me->getVictim());
                            AmaniKiddnaperGUID = pCreature->GetGUID();
                        }
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_akilzonAI(creature);
        }
};

class mob_akilzon_eagle : public CreatureScript
{
    public:

        mob_akilzon_eagle() : CreatureScript("mob_akilzon_eagle") { }

        struct mob_akilzon_eagleAI : public ScriptedAI
        {
            mob_akilzon_eagleAI(Creature *c) : ScriptedAI(c) {}

            uint32 EagleSwoop_Timer;
            bool arrived;
            uint64 TargetGUID;

            void Reset()
            {
                EagleSwoop_Timer = 5000 + rand()%5000;
                arrived = true;
                TargetGUID = 0;
                me->SetUnitMovementFlags(MOVEMENTFLAG_LEVITATING);
            }

            void EnterCombat(Unit * /*who*/) {DoZoneInCombat();}

            void MoveInLineOfSight(Unit* /*who*/) {}

            void MovementInform(uint32, uint32)
            {
                arrived = true;
                if (TargetGUID)
                {
                    if (Unit *pTarget = Unit::GetUnit(*me, TargetGUID))
                        DoCast(pTarget, SPELL_EAGLE_SWOOP, true);
                    TargetGUID = 0;
                    me->SetSpeed(MOVE_RUN, 1.2f);
                    EagleSwoop_Timer = 5000 + rand()%5000;
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (EagleSwoop_Timer <= diff)
                    EagleSwoop_Timer = 0;
                else
                    EagleSwoop_Timer -= diff;

                if (arrived)
                {
                    if (Unit *pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0))
                    {
                        float x, y, z;
                        if (EagleSwoop_Timer)
                        {
                            x = pTarget->GetPositionX() + irand(-10,10);
                            y = pTarget->GetPositionY() + irand(-10,10);
                            z = pTarget->GetPositionZ() + urand(10,15);
                            if (z > 95)
                                z = 95.0f - urand(0,5);
                        }
                        else
                        {
                            pTarget->GetContactPoint(me, x, y, z);
                            z += 2;
                            me->SetSpeed(MOVE_RUN, 5.0f);
                            TargetGUID = pTarget->GetGUID();
                        }
                        me->GetMotionMaster()->MovePoint(0, x, y, z);
                        arrived = false;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_akilzon_eagleAI(creature);
        }
};

class mob_amani_kiddnaper : public CreatureScript
{
    public:

        mob_amani_kiddnaper() : CreatureScript("mob_amani_kiddnaper") { }

        struct mob_amani_kiddnaperAI : public ScriptedAI
        {
            mob_amani_kiddnaperAI(Creature *c) : ScriptedAI(c) {}


            void UpdateAI(const uint32 diff)
            {
            
            }
            
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_amani_kiddnaperAI(creature);
        }
};

void AddSC_boss_akilzon()
{
    new boss_akilzon();
    new mob_akilzon_eagle();
    new mob_amani_kiddnaper();
}
