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
#include "throne_of_the_four_winds.h"


// TODO:
//critical:
// extend bounding radius in phase 3
//to be done:
// correct timers based on test feedback
// Relentless Storm - not implemented yet
//obsolete:
// lightning clouds dummy visual range to cca 20 (spells: 89583, 89592)
// Squall Line: wrong Vehicle Kit ID; omega speed; direction?
// destroy platform animation
// Lightning Strike visuals
// Al'akir equip id: sword

enum Stuff
{
// phase 1
    SPELL_LIGHTNING_STRIKE_DAMAGE = 88214,
    SPELL_LIGHTNING_STRIKE_DAMAGE_10H = 93255,
    SPELL_LIGHTNING_STRIKE_DAMAGE_25N = 93256,
    SPELL_LIGHTNING_STRIKE_DAMAGE_25H = 93257,
    SPELL_LIGHTNING_STRIKE_VISUAL = 88230,
    SPELL_STATIC_SHOCK_HC_ONLY    = 87873,
    SPELL_ELECTROCUTE             = 88427,
    SPELL_WIND_BURST              = 87770,
    SPELL_ICE_STORM_GROUND_AURA   = 87469,
    SPELL_ICE_STORM_TRIGGER       = 87472,
    SPELL_ICE_STORM_SUMMON        = 87055,
    SPELL_SQUALL_LINE_VISUAL      = 87621,
    SPELL_SQUALL_LINE_VEHICLE     = 87856,
// phase 2
    //SPELL_ACID_RAIN_TRIGGER       = 88290,
    SPELL_ACID_RAIN_DOT           = 88301,
    NPC_STORMLING                 = 47175,
    SPELL_STORMLING_DAMAGE        = 87905,
    SPELL_STORMLING_VISUAL        = 87906,
    SPELL_FEEDBACK                = 87904,
// phase 3
    SPELL_LIGHTNING               = 89641,
    SPELL_LIGHTNING_ROD           = 89668,
    SPELL_WIND_BURST_THIRD_PHASE  = 88858,
    SPELL_EYE_OF_THE_STORM_AURA   = 82724,
    SPELL_EYE_OF_THE_STORM_TRIGGER= 89107,
    SPELL_RELENTLESS_STORM_CHANNEL= 88875,
    SPELL_RELENTLESS_STORM_VISUAL = 88866,
    SPELL_LIGHTNING_CLOUDS        = 89564,
    SPELL_LIGHTNING_CLOUDS_DMG    = 89587,

    // Relentless Storm ? - not implemented yet
    SPELL_RELENTLESS_STORM_VEHICLE= 89104, // ?
    NPC_RELENTLESS_STORM          = 47807, // ?


    GO_HEART_OF_WIND              = 207891,
};

#define SL_DIST_FIRST 29.0f
#define SL_DIST_NEXT  6.2f
#define SL_COUNT 7
#define SL_RAD_PER_STEP (M_PI/32.0f)

static uint16 vortexready[SL_COUNT];
float furthest_omega;

class boss_alakir: public CreatureScript
{
    public:
        boss_alakir(): CreatureScript("boss_alakir") {}

        struct boss_alakirAI: public Scripted_NoMovementAI
        {
            boss_alakirAI(Creature* c): Scripted_NoMovementAI(c)
            {
                c->SetReactState(REACT_DEFENSIVE);

                instance = c->GetInstanceScript();
                map = c->GetMap();

                m_stormlings.clear();
                m_relentless_guid = 0;
                Reset();
            }

            InstanceScript* instance;
            Map* map;
            uint32 m_phase;

            std::list<uint64> m_stormlings;

            uint32 checkHealthTimer;

            uint32 squallLineTimer;
            uint32 iceStormTimer;
            uint32 lightningStrikeTimer;
            uint32 windBurstTimer;
            uint32 staticShockTimer;
            uint32 acidRainTimer;
            uint32 stormlingTimer;
            uint32 lightningTimer;
            uint32 lightningRodTimer;
            uint32 lightningCloudTimer;

            uint64 m_relentless_guid;

            void Reset()
            {
                m_phase = 0;

                checkHealthTimer = 2000;

                iceStormTimer = 5000;
                squallLineTimer = 15000;
                lightningStrikeTimer = 8000;
                windBurstTimer = 11000;
                staticShockTimer = 2000;
                acidRainTimer = 2000;
                stormlingTimer = 6000;
                lightningTimer = 3000;
                lightningRodTimer = 9000;
                lightningCloudTimer = 15000;

                DespawnStormlings();

                for (uint32 i = 0; i < SL_COUNT; i++)
                    vortexready[i] = 0;

                if (instance)
                    instance->SetData(TYPE_ALAKIR, NOT_STARTED);

                me->SetFlying(false);

                me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, 35.0f); // implicit value of native model
                // client update
            }

            void EnterCombat(Unit* /*who*/)
            {
                if (instance)
                    instance->SetData(TYPE_ALAKIR, IN_PROGRESS);

                DoScriptText(-1850527, me);

                m_phase = 1;
            }

            void JustDied(Unit* /*pKiller*/)
            {
                if (instance)
                    instance->SetData(TYPE_ALAKIR, DONE);

                DespawnStormlings();

                DoScriptText(-1850534, me);

                // summon loot chest
                me->SummonGameObject(GO_HEART_OF_WIND, me->GetPositionX(), me->GetPositionY(), 240.0f, 0, 0, 0, 0, 0, 0);
            }

            void KilledUnit(Unit* /*victim*/)
            {
                DoScriptText((urand(0,1) ? -1850532 : -1850533), me);
            }

            void SpellHitTarget(Unit* pTarget, const SpellEntry* spell)
            {
                if (!spell || !pTarget)
                    return;

                // Cast visual when target hit by Lightning Strike
                if (spell->Id == SPELL_LIGHTNING_STRIKE_DAMAGE || spell->Id == SPELL_LIGHTNING_STRIKE_DAMAGE_10H
                    || spell->Id == SPELL_LIGHTNING_STRIKE_DAMAGE_25N || spell->Id == SPELL_LIGHTNING_STRIKE_DAMAGE_25H)
                    me->CastSpell(pTarget, SPELL_LIGHTNING_STRIKE_VISUAL, true);
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                // ######## Switch Phases ########
                if (checkHealthTimer <= diff)
                {
                    if (m_phase == 2 && me->GetHealthPct() < 25.0f)
                    {
                        DoScriptText(-1850531, me);
                        m_phase = 3;

                        windBurstTimer = 20000;

                        if(!map)
                            return;
                    // shatter platform, initialize
                        if (instance)
                            instance->SetData(TYPE_ALAKIR, SPECIAL);

                    // let players fly
                        me->CastSpell(me, SPELL_WIND_BURST_THIRD_PHASE, false);

                    // change model
                        me->AddAura(SPELL_RELENTLESS_STORM_CHANNEL, me);
                        me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, 400.0f);
                        // client update
                        //me->BuildUpdate
                        //me->BuildFieldsUpdate
                        //me->BuildValuesUpdateBlockForPlayer

                    // let myself fly
                        me->SetFlying(true);
                        me->GetMotionMaster()->MovePoint(666,me->GetPositionX(),me->GetPositionY(),me->GetPositionZ());
                        WorldPacket data;
                        me->BuildHeartBeatMsg(&data);
                        me->SendMessageToSet(&data, false);

                    // let Stormlings fly
                        if (!m_stormlings.empty())
                        {
                            for (std::list<uint64>::const_iterator itr = m_stormlings.begin(); itr != m_stormlings.end(); ++itr)
                            {
                                Creature *summon = map->GetCreature(*itr);
                                if (summon)
                                    me->AddAura(SPELL_EYE_OF_THE_STORM_AURA, summon);
                            }
                        }

                    // Relentless Storm visual on trigger npc
                        if (Creature* summon = me->SummonTrigger(0, 0, 0, 0, 0, 0))
                        {
                            m_relentless_guid = summon->GetGUID();

                            summon->SetFlying(true);
                            summon->GetMotionMaster()->MovePoint(666,me->GetPositionX(),me->GetPositionY(),me->GetPositionZ());
                            WorldPacket hbt;
                            me->BuildHeartBeatMsg(&hbt);
                            me->SendMessageToSet(&hbt, false);

                            summon->SetReactState(REACT_PASSIVE);
                            summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                            me->AddAura(SPELL_RELENTLESS_STORM_VISUAL, summon);
                        }

                    // do initial Lightning Clouds at the bottom
                        DoLightningCloud(185.0f);

                    }
                    else if (m_phase == 1 && me->GetHealthPct() < 80.0f)
                    {
                        DoScriptText(-1850529, me);
                        m_phase = 2;
                    }
                    checkHealthTimer = 2000;

                } else checkHealthTimer -= diff;

                // ######## Abilities Timers ########
                switch (m_phase)
                {
                case 1:
                    {
                    // Squall Line
                        if (squallLineTimer <= diff)
                        {
                            squallLineTimer = 40000 + 20000; // 40s duration, 20s delay
                            DoSquallLine();

                        } else squallLineTimer -= diff;

                    // Ice Storm
                        if (iceStormTimer <= diff)
                        {
                            DoIceStorm();
                            iceStormTimer = 25000;

                        } else iceStormTimer -= diff;

                    // Static Shock -- HC only
                        switch (getDifficulty())
                        {
                            case RAID_DIFFICULTY_10MAN_HEROIC:
                            case RAID_DIFFICULTY_25MAN_HEROIC:
                                if (staticShockTimer <= diff)
                                {
                                    me->InterruptSpell(CURRENT_CHANNELED_SPELL);
                                    if (me->hasUnitState(UNIT_STAT_CASTING))
                                        break;
                                    me->CastSpell(me, SPELL_STATIC_SHOCK_HC_ONLY, false);
                                    staticShockTimer = 2000;

                                } else staticShockTimer -= diff;
                                break;
                            default: break;
                        }

                    // Wind Burst
                        if (windBurstTimer <= diff)
                        {
                            DoScriptText(-1850528, me);

                            me->InterruptSpell(CURRENT_CHANNELED_SPELL);
                            if (me->hasUnitState(UNIT_STAT_CASTING))
                                break;
                            me->CastSpell(me, SPELL_WIND_BURST, false);

                            windBurstTimer = 20000;
                            lightningStrikeTimer += 5000;

                            return;

                        } else windBurstTimer -= diff;

                    // Lightning Strike
                        if (lightningStrikeTimer <= diff)
                        {
                            Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0);
                            if (target)
                            {
                                me->InterruptSpell(CURRENT_CHANNELED_SPELL);
                                if (me->hasUnitState(UNIT_STAT_CASTING))
                                    break;
                                me->CastSpell(target, SPELL_LIGHTNING_STRIKE_DAMAGE, false);
                            }
                            lightningStrikeTimer = 5000;

                            return;

                        } else lightningStrikeTimer -= diff;

                    // attack
                        if (me->IsWithinMeleeRange(me->getVictim()))
                            me->InterruptSpell(CURRENT_CHANNELED_SPELL);
                        DoMeleeAttackIfReady();
                        DoSpellAttackIfReady(SPELL_ELECTROCUTE); // case primary target is not in melee range

                    }
                    break;
                case 2:
                    {
                    // Squall Line
                        if (squallLineTimer <= diff)
                        {
                            squallLineTimer = 40000 + 4000; // 40s duration, 4s delay
                            DoSquallLine();

                        } else squallLineTimer -= diff;

                    // Static Shock -- HC only
                        switch (getDifficulty())
                        {
                            case RAID_DIFFICULTY_10MAN_HEROIC:
                            case RAID_DIFFICULTY_25MAN_HEROIC:
                                if (staticShockTimer <= diff)
                                {
                                    me->InterruptSpell(CURRENT_CHANNELED_SPELL);
                                    if (me->hasUnitState(UNIT_STAT_CASTING))
                                        break;
                                    me->CastSpell(me, SPELL_STATIC_SHOCK_HC_ONLY, false);
                                    staticShockTimer = 2000;

                                } else staticShockTimer -= diff;
                                break;
                            default: break;
                        }

                    // acid rain
                        if (acidRainTimer <= diff)
                        {
                            me->CastSpell(me, SPELL_ACID_RAIN_DOT, true);
                            acidRainTimer = 14500;

                        } else acidRainTimer -= diff;

                    // summon stormling
                        if (stormlingTimer <= diff)
                        {
                            Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0);
                            if (target)
                            {
                                Creature* summon = me->SummonCreature(NPC_STORMLING, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN, 0);
                                if (summon)
                                {
                                    summon->setFaction(14);
                                    summon->AddThreat(target, 1.0f);
                                    summon->AI()->AttackStart(target);
                                    summon->CastSpell(summon, SPELL_STORMLING_DAMAGE, true);
                                    summon->CastSpell(summon, SPELL_STORMLING_VISUAL, true);

                                    m_stormlings.push_back(summon->GetGUID());
                                }
                            }
                            stormlingTimer = 20000;

                        } else stormlingTimer -= diff;

                    // attack
                        if (me->IsWithinMeleeRange(me->getVictim()))
                            me->InterruptSpell(CURRENT_CHANNELED_SPELL);
                        DoMeleeAttackIfReady();
                        DoSpellAttackIfReady(SPELL_ELECTROCUTE); // case primary target is not in melee range

                    }
                    break;
                case 3:
                    {
                    // Lightning
                        if (lightningTimer <= diff)
                        {
                            Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0);
                            if (target)
                                me->CastSpell(target, SPELL_LIGHTNING, false);
                            lightningTimer = 3000;

                        } else lightningTimer -= diff;
                    // Wind Burst
                        if (windBurstTimer <= diff)
                        {
                            DoScriptText(-1850528, me);

                            me->CastSpell(me, SPELL_WIND_BURST_THIRD_PHASE, false);
                            windBurstTimer = 20000;

                        } else windBurstTimer -= diff;
                    // Lightning Rod
                        if (lightningRodTimer <= diff)
                        {
                            Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0);
                            if (target)
                                me->AddAura(SPELL_LIGHTNING_ROD, target);
                            lightningRodTimer = 15000;

                        } else lightningRodTimer -= diff;
                    // Lightning Cloud
                        if (lightningCloudTimer < diff)
                        {
                            Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0);
                            if (target)
                            {
                                float altitude = target->GetPositionZ();
                                DoLightningCloud(altitude);
                            }
                            lightningCloudTimer = 30000;

                        } else lightningCloudTimer -= diff;
                    }
                    break;
                }
            }

            void SummonedCreatureDies(Creature* summon, Unit* /*killer*/)
            {
                // Stomrling dies --> apply feedback debuff
                if (summon && summon->GetEntry() == NPC_STORMLING)
                {
                    me->CastSpell(me, SPELL_FEEDBACK, true);
                    summon->ForcedDespawn();

                    m_stormlings.remove(summon->GetGUID());
                }
            }

            void DoSquallLine()
            {
                DoScriptText(-1850530, me);

                // 0.5f will be the speed of furthest squall
                furthest_omega = 0.5f*(SL_DIST_FIRST+SL_DIST_NEXT * (SL_COUNT-1));

                Creature* pTemp = NULL;
                float mx,my,mz;
                uint32 mstep = urand(0,uint32((2*M_PI)/(SL_RAD_PER_STEP)));
                uint32 skip = urand(0,SL_COUNT-1);
                for (uint32 i = 0; i < SL_COUNT; i++)
                {
                    if (i == skip)
                        continue;

                    mx = me->GetPositionX()+(SL_DIST_FIRST+SL_DIST_NEXT * i) * cos(mstep * SL_RAD_PER_STEP);
                    my = me->GetPositionY()+(SL_DIST_FIRST+SL_DIST_NEXT * i) * sin(mstep * SL_RAD_PER_STEP);
                    mz = me->GetPositionZ();
                    pTemp = me->SummonCreature(48855, mx,my,mz,0,TEMPSUMMON_MANUAL_DESPAWN,0);
                    if (!pTemp)
                        continue;

                    pTemp->SetSpeed(MOVE_RUN, furthest_omega/(SL_DIST_FIRST+SL_DIST_NEXT * (SL_COUNT-i)), true);

                    mx = me->GetPositionX()+(SL_DIST_FIRST+SL_DIST_NEXT*i)*cos((mstep+1)*SL_RAD_PER_STEP);
                    my = me->GetPositionY()+(SL_DIST_FIRST+SL_DIST_NEXT*i)*sin((mstep+1)*SL_RAD_PER_STEP);

                    // Define actual point as 1000*position+step
                    pTemp->GetMotionMaster()->MovePoint(mstep+1000*i, mx, my, mz);
                }
            }

            void DoIceStorm()
            {
                Unit* pTargetOne = SelectUnit(SELECT_TARGET_RANDOM, 0);
                Unit* pTargetTwo = SelectUnit(SELECT_TARGET_RANDOM, 0);
                if (pTargetOne && pTargetTwo)
                {
                    Creature* pStorm = me->SummonCreature(46973, pTargetOne->GetPositionX(), pTargetOne->GetPositionY(), pTargetOne->GetPositionZ());
                    if (pStorm)
                    {
                        pStorm->AddThreat(pTargetOne, 10000.0f);
                        pStorm->AddThreat(pTargetTwo, 50000.0f);
                        pStorm->Attack(pTargetTwo, true);
                        pStorm->GetMotionMaster()->MoveChase(pTargetTwo);
                    }
                }
                // todo: prevent from crossing inside boss
            }

            void DoLightningCloud(float z)
            {
                // fill the given altitude with clouds
                for (int i = 0; i < 50; i++)
                {
                    float x, y, d, r, a;

                    // circle will be filled even
                    d = float(urand(0,100)) / 100.0f;
                    r = 125.0f * sqrt(d);

                    a = float(urand(0,628)) / 100.0f;

                    x = me->GetPositionX() + sin(a) * r;
                    y = me->GetPositionY() + cos(a) * r;
                    z += float(urand(0,8)) - 4.0f;

                    me->SummonCreature(SUMMON_LIGHTNING_CLOUDS, x, y, z, 0);
                }
            }

            void DespawnStormlings()
            {
                if (Map* map = me->GetMap())
                {
                    while (!m_stormlings.empty())
                    {
                        Creature *summon = map->GetCreature(*m_stormlings.begin());
                        if (summon)
                            summon->DisappearAndDie();
                        m_stormlings.erase(m_stormlings.begin());
                    }
                    if (m_relentless_guid)
                    {
                        Creature *trigger = map->GetCreature(m_relentless_guid);
                        if (trigger)
                            trigger->DisappearAndDie();
                        m_relentless_guid = 0;
                    }
                }
            }

        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new boss_alakirAI(c);
        }
};

class npc_ice_storm: public CreatureScript
{
    public:
        npc_ice_storm(): CreatureScript("npc_ice_storm") {}

        struct npc_ice_stormAI: public ScriptedAI
        {
            npc_ice_stormAI(Creature* c): ScriptedAI(c)
            {
                c->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                c->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                c->setFaction(14);
                Reset();
            }

            uint32 disappearTimer;
            uint32 dropTimer;

            void Reset()
            {
                me->CastSpell(me, SPELL_ICE_STORM_TRIGGER, true);
                me->SetSpeed(MOVE_RUN, 0.5f, true);
                disappearTimer = 20000;
                dropTimer = 2000;
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (dropTimer <= diff)
                {
                    Unit* summon = me->SummonCreature(46766, 0, 0, 0);
                    summon->SetDisplayId(me->GetDisplayId());
                    me->CastSpell(me, SPELL_ICE_STORM_TRIGGER, true); // visual
                    dropTimer = 2000;
                } else dropTimer -= diff;

                if (disappearTimer <= diff)
                {
                    disappearTimer = 10000;
                    me->Kill(me, false);
                    me->ForcedDespawn();
                } else disappearTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_ice_stormAI(c);
        }
};

class npc_ice_storm_ground: public CreatureScript
{
    public:
        npc_ice_storm_ground(): CreatureScript("npc_ice_storm_ground") {}

        struct npc_ice_storm_groundAI: public Scripted_NoMovementAI
        {
            npc_ice_storm_groundAI(Creature* c): Scripted_NoMovementAI(c)
            {
                c->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                c->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                c->SetReactState(REACT_PASSIVE);
                c->setFaction(14);
                Reset();
            }

            uint32 disappearTimer;

            void Reset()
            {
                me->CastSpell(me, SPELL_ICE_STORM_GROUND_AURA, false);
                disappearTimer = 20000;
            }

            void UpdateAI(const uint32 diff)
            {
                if (!me->HasAura(SPELL_ICE_STORM_GROUND_AURA) && !me->IsNonMeleeSpellCasted(true))
                    me->CastSpell(me, SPELL_ICE_STORM_GROUND_AURA, false);

                if (disappearTimer <= diff)
                {
                    disappearTimer = 10000;
                    me->Kill(me, false);
                    me->ForcedDespawn();
                } else disappearTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_ice_storm_groundAI(c);
        }
};

class npc_squall_vortex: public CreatureScript
{
    public:
        npc_squall_vortex(): CreatureScript("npc_squall_vortex") {}

        struct npc_squall_vortexAI: public ScriptedAI
        {
            npc_squall_vortexAI(Creature* c): ScriptedAI(c)
            {
                c->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                c->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                c->setFaction(14);
                Reset();
            }

            uint32 despawnTimer;
            std::list<Unit*> pVehicle_list;

            float nextx,nexty,nextz,nextpoint;
            uint8 nextpos;
            bool needstomove;

            void Reset()
            {
                me->CastSpell(me, SPELL_SQUALL_LINE_VISUAL, true);
                despawnTimer = 40000;
                pVehicle_list.clear();
                needstomove = false;
            }

            void AttackStart(Unit* /*who*/) { }

            void MoveInLineOfSight(Unit* pWho)
            {
                if (!pWho || pWho->GetVehicle() || pWho->GetTypeId() != TYPEID_PLAYER || pWho->isDead() || pWho->GetDistance2d(me) > 2.5f)
                    return;

                Player* pPassenger = pWho->ToPlayer();
                Unit* pVehicle = me->SummonCreature(48852, 0, 0, 0);
                pVehicle->SetDisplayId(me->GetDisplayId());
                pVehicle->setFaction(14);
                pVehicle->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_SCHOOL_DAMAGE, true);
                pVehicle->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                Vehicle* pVeh = pVehicle->GetVehicleKit();
                if (pVeh)
                {
                    if (Aura* aura = pPassenger->AddAura(SPELL_SQUALL_LINE_VEHICLE, pPassenger))
                        aura->SetDuration(despawnTimer);
                    pPassenger->EnterVehicle(pVehicle,0);
                    pPassenger->clearUnitState(UNIT_STAT_UNATTACKABLE); // applied when entering vehicle
                    pVehicle->GetMotionMaster()->MoveFollow(me, 0.0f, 0.0f);
                    pVehicle->SetSpeed(MOVE_RUN, me->GetSpeed(MOVE_RUN), true);
                }
                pVehicle_list.push_back(pVehicle);
            }

            void MovementInform(uint32 type, uint32 id)
            {
                // Get actual position
                uint32 mypos = id/1000;
                uint32 mypoint = id%1000;

                float mx,my,mz;
                Creature* pBoss = GetClosestCreatureWithEntry(me, 46753, 500.0f);
                if (!pBoss)
                    return;

                // And do the trick
                mypoint += 1;

                mx = pBoss->GetPositionX()+(SL_DIST_FIRST+SL_DIST_NEXT*mypos)*cos(mypoint*SL_RAD_PER_STEP);
                my = pBoss->GetPositionY()+(SL_DIST_FIRST+SL_DIST_NEXT*mypos)*sin(mypoint*SL_RAD_PER_STEP);
                mz = pBoss->GetPositionZ();

                nextx = mx;
                nexty = my;
                nextz = mz;
                nextpos = mypos;
                nextpoint = mypoint;
                needstomove = true;
                vortexready[mypos] = mypoint;
            }

            void UpdateAI(const uint32 diff)
            {
                if (despawnTimer <= diff)
                {
                    me->Kill(me, false);
                    me->ForcedDespawn();
                    for(std::list<Unit*>::const_iterator itr = pVehicle_list.begin(); itr != pVehicle_list.end(); ++itr)
                    {
                        (*itr)->Kill(*itr, false);
                        if ((*itr)->ToCreature())
                            (*itr)->ToCreature()->ForcedDespawn();
                    }
                    // For being sure when doing multithreading
                    // should not happen, but who knows
                    despawnTimer = 10000;
                }
                else despawnTimer -= diff;

                if (needstomove)
                {
                    uint32 sum = 0;
                    for (uint32 i = 0; i < SL_COUNT; i++)
                        sum += vortexready[i];

                    if (sum >= nextpoint*(SL_COUNT-1))
                    {
                        me->GetMotionMaster()->MovePoint(nextpos*1000+nextpoint, nextx, nexty, nextz);
                        for(std::list<Unit*>::const_iterator itr = pVehicle_list.begin(); itr != pVehicle_list.end(); ++itr)
                            (*itr)->GetMotionMaster()->MovePoint(nextpos*1000+nextpoint, nextx, nexty, nextz);
                        needstomove = false;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_squall_vortexAI(c);
        }
};

class npc_lightning_clouds: public CreatureScript
{
    public:
        npc_lightning_clouds(): CreatureScript("npc_lightning_clouds") {}

        struct npc_lightning_cloudsAI: public Scripted_NoMovementAI
        {
            npc_lightning_cloudsAI(Creature* c): Scripted_NoMovementAI(c)
            {
                c->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                c->SetReactState(REACT_PASSIVE);
                c->setFaction(14);

                c->SetFlying(true);
                c->GetMotionMaster()->MovePoint(666, c->GetPositionX(), c->GetPositionY(), c->GetPositionZ());
                WorldPacket data;
                c->BuildHeartBeatMsg(&data);
                c->SendMessageToSet(&data, false);

                c->CastSpell(me, SPELL_LIGHTNING_CLOUDS, true);

                Reset();
            }

            void Reset() { }
            void EnterEvadeMode() { }
            void DamageTaken(Unit* /*who*/, uint32 &damage) { damage = 0; }

            void SpellHit(Unit* /*caster*/, const SpellEntry* spell_info)
            {
                if (spell_info && spell_info->Id == 89567) // not working blizz aura
                {
                    me->AddAura(SPELL_LIGHTNING_CLOUDS_DMG, me);

                    for (int i = 0; i < 4; i++)
                    {
                        float x, y, z, d, r, a;

                        d = float(urand(0,100)) / 100.0f;
                        r = 9.75f * sqrt(d); // todo: increase along with range of visual dummy spells

                        a = float(urand(0,628)) / 100.0f;

                        x = me->GetPositionX() + sin(a) * r;
                        y = me->GetPositionY() + cos(a) * r;
                        z = me->GetPositionZ() + float(urand(0,4)) - 2.0f;

                        if (Creature * summon = me->SummonCreature(SUMMON_LIGHTNING_CLOUDS_EX, x, y, z, 0))
                        {
                            summon->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_SCHOOL_DAMAGE, true);
                            summon->SetReactState(REACT_PASSIVE);
                            summon->setFaction(14);

                            summon->SetFlying(true);
                            summon->GetMotionMaster()->MovePoint(666, summon->GetPositionX(), summon->GetPositionY(), summon->GetPositionZ());
                            WorldPacket heartbeat;
                            summon->BuildHeartBeatMsg(&heartbeat);
                            summon->SendMessageToSet(&heartbeat, false);
                        }
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_lightning_cloudsAI(c);
        }
};

void AddSC_alakir()
{
    new boss_alakir();
    new npc_ice_storm();
    new npc_ice_storm_ground();
    new npc_squall_vortex();
    new npc_lightning_clouds();
}

/*
SQL:

UPDATE creature_template SET vehicleId=342,AIName='NullAI' WHERE entry=48852;
UPDATE creature_template SET vehicleId=0,ScriptName='npc_squall_vortex' WHERE entry=48855;
UPDATE creature_template SET ScriptName='npc_ice_storm' WHERE entry=46973;
UPDATE creature_template SET ScriptName='npc_ice_storm_ground' WHERE entry=46766;
UPDATE creature_template SET ScriptName='boss_alakir' WHERE entry=46753;
UPDATE creature_template SET ScriptName='npc_lightning_clouds' WHERE entry=48190;
UPDATE creature_model_info SET bounding_radius=35.0, combat_reach=45.0 WHERE modelid=35248;
UPDATE creature_model_info SET bounding_radius=400.0, combat_reach=1.0 WHERE modelid=36062;


INSERT INTO `script_texts`
(`npc_entry`, `entry`, `content_default`, `content_loc1`, `content_loc2`, `content_loc3`, `content_loc4`, `content_loc5`, `content_loc6`, `content_loc7`, `content_loc8`, `sound`, `type`, `language`, `emote`, `comment`)
VALUES
(46753, -1850527, "Your challenge is accepted, mortals! Know that you face Al'Akir, Elemental Lord of Air! You have no hope of defeating me!", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_AGGRO'),
(46753, -1850528, "Winds! Obey my command!", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_WIND_BURST'),
(46753, -1850529, "Your futile persistence angers me.", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_PHASE_TWO'),
(46753, -1850530, "Storms! I summon you to my side!", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_SQUALL_LINE'),
(46753, -1850531, "ENOUGH! I WILL NO LONGER BE CONTAINED!", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_PHASE_THREE'),
(46753, -1850532, "Like swatting insects...", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_KILL_1'),
(46753, -1850533, "This little one will vex me no longer.", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_KILL_2'),
(46753, -1850534, "After every storm... comes the calm...", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_DEATH');


*/
