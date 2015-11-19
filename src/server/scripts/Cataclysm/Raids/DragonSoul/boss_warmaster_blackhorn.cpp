/*
* Copyright (C) 2006-2013 iCe Online <http://ice-wow.eu>
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
#include "dragonsoul.h"
#include "MapManager.h"
#include "TaskScheduler.h"

// NPCs
enum NPC
{
    BOSS_WARMASTER_BLACKHORN            = 56427,
    NPC_GORIONA                         = 56781,
    NPC_TWILIGHT_ASSAULT_DRAKE_LEFT     = 56855,
    NPC_TWILIGHT_ASSAULT_DRAKE_RIGHT    = 56587,
    NPC_TWILIGHT_ELITE_DREADBLADE       = 56854,
    NPC_TWILIGHT_ELITE_SLAYER           = 56848,
    NPC_TWILIGHT_INFILTRATOR            = 56922,
    NPC_SKYFIRE                         = 56598,
    NPC_TWILIGHT_FLAMES                 = 57268,
    NPC_MASSIVE_EXPLOSION               = 57297,
    NPC_GUNSHIP_PURSUIT_CONTROLLER      = 56599,
    NPC_ONSLAUGHT_TARGET                = 57238,
    NPC_ENGINE_STALKER                  = 57190,
    NPC_FIRE_STALKER                    = 57852,
    NPC_SKYFIRE_HARPOON_GUN             = 56681,
    NPC_SKYFIRE_CANNON                  = 57260,
    NPC_SKYFIRE_COMMANDO                = 57264,
};

// Spells
enum Spells
{
    // Warmaster Blackhorn
    SPELL_BERSERK                       = 26662,
    SPELL_VENGEANCE                     = 108045,
    SPELL_DEVASTATE                     = 108042,
    SPELL_DISRUPTING_ROAR               = 108044,
    SPELL_SHOCKWAVE_AOE                 = 110137,
    SPELL_SHOCKWAVE                     = 108046,
    SPELL_SIPHON_VITALITY               = 110312,

    // Goriona
    SPELL_TWILIGHT_ONSLAUGHT_DUMMY      = 107927,
    SPELL_TWILIGHT_ONSLAUGHT_DUMMY_2    = 107586,
    SPELL_TWILIGHT_ONSLAUGHT            = 107588,
    SPELL_TWILIGHT_ONSLAUGHT_DMG        = 106401,
    SPELL_TWILIGHT_ONSLAUGHT_DMG_2      = 107589,
    SPELL_BROADSIDE_AOE                 = 110153,
    SPELL_BROADSIDE_DMG                 = 110157,
    SPELL_TWILIGHT_BREATH               = 110212,
    SPELL_CONSUMING_SHROUD              = 110214,
    SPELL_CONSUMING_SHROUD_DMG          = 110215,
    SPELL_TWILIGHT_FLAMES               = 108051,
    SPELL_TWILIGHT_FLAMES_AURA          = 108053,

    // Skifire
    SPELL_ENGINE_FIRE                   = 107799,
    SPELL_DECK_FIRE                     = 109445, // 109245
    SPELL_DECK_FIRE_DMG                 = 110095,
    SPELL_DECK_FIRE_PERIODIC            = 110092,
    SPELL_DECK_FIRE_SPAWN               = 109470,
    SPELL_MASSIVE_EXPLOSION             = 108132,
    SPELL_HEAVY_SLUG                    = 108010,
    SPELL_ARTILLERY_BARRAGE             = 108040,
    SPELL_ARTILLERY_BARRAGE_DMG         = 108041,
    SPELL_HARPOON                       = 108038,
    SPELL_RELOADING                     = 108039,
    SPELL_RIDE_VEHICLE                  = 43671, // commandos on harpoon guns and cannons

    // Twilight Assault Drake
    SPELL_TWILIGHT_BARRAGE              = 107286,
    SPELL_TWILIGHT_BARRAGE_DMG          = 107439,
    SPELL_TWILIGHT_BARRAGE_DUMMY        = 107287,
    SPELL_TWILIGHT_BARRAGE_DMG_2        = 107501,
};

enum EncounterActions
{
    ACTION_START_ENCOUNTER              = 0,
    ACTION_GORIONA_LEAVE_SCENE          = 1,
    ACTION_PHASE_TWO                    = 2,
};

enum Phase
{
    PHASE_ZERO                          = 0,
    PHASE_ONE                           = 1,
    PHASE_TWO                           = 2,
    PHASE_LAND                          = 3,
};

// Warmaster Blackhorn
class boss_warmaster_blackhorn : public CreatureScript
{
public:
    boss_warmaster_blackhorn() : CreatureScript("boss_warmaster_blackhorn") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_warmaster_blackhornAI(pCreature);
    }

    struct boss_warmaster_blackhornAI : public ScriptedAI
    {
        boss_warmaster_blackhornAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        TaskScheduler scheduler;
        uint32 disruptingRoarTimer;
        uint32 shockwaveTimer;
        uint32 devastateTimer;
        uint32 vengeanceTimer;
        uint32 phase;
        bool siphonVitality;

        void Reset() override
        {
            if (instance)
            {
                if (instance->GetData(TYPE_BOSS_BLACKHORN) != DONE)
                    instance->SetData(TYPE_BOSS_BLACKHORN, NOT_STARTED);
            }

            me->CastSpell(me, SPELL_VENGEANCE, false);
            me->SetVisible(false);
            me->SetReactState(REACT_PASSIVE);
            phase = PHASE_ONE;

            scheduler.Schedule(Seconds(5), [this](TaskContext greetings)
            {
                if (Creature * pGoriona = me->FindNearestCreature(NPC_GORIONA, 200.0f, true))
                    me->EnterVehicle(pGoriona, 0, nullptr);
            });

            disruptingRoarTimer = 20000;
            shockwaveTimer = 25000;
            devastateTimer = 8000;
            vengeanceTimer = 5000;
            siphonVitality = false;

            if (Creature* pShip = me->FindNearestCreature(NPC_SKYFIRE, 300.0f))
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, pShip);
        }

        void EnterCombat(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_BLACKHORN, IN_PROGRESS);
            }
        }

        void KilledUnit(Unit* victim) override 
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
            {
                switch (urand(0, 3))
                {
                case 0:
                    me->MonsterYell("Ha, ha, ha... mess with the bull!", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(26215, true);
                    break;
                case 1:
                    me->MonsterYell("HA-HA-HA-HA!", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(26216, true);
                    break;
                case 2:
                    me->MonsterYell("Down you go!", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(26217, true);
                    break;
                case 3:
                    me->MonsterYell("Get up! Oh, weakling...", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(26218, true);
                    break;
                }
            }
        }

        void JustDied(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_BLACKHORN, DONE);
                if (Creature* pShip = me->FindNearestCreature(NPC_SKYFIRE, 300.0f))
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, pShip);
            }

            if (Creature * pGoriona = me->FindNearestCreature(NPC_GORIONA, 200.0f, true))
                pGoriona->AI()->DoAction(ACTION_GORIONA_LEAVE_SCENE);

            me->MonsterYell("Well... done, heh. But I wonder if you're good enough... to best him.", LANG_UNIVERSAL, 0);
            me->SendPlaySound(26211, false);
        }

        void DoAction(const int32 action) override
        {
            if (action == ACTION_PHASE_TWO)
            {
                phase = PHASE_TWO;
                me->SetReactState(REACT_AGGRESSIVE);
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (!UpdateVictim())
                return;

            if (phase == PHASE_TWO)
            {
                if (disruptingRoarTimer <= diff)
                {
                    me->CastSpell(me, SPELL_DISRUPTING_ROAR, false);
                    disruptingRoarTimer = 20000;
                }
                else disruptingRoarTimer -= diff;

                if (shockwaveTimer <= diff)
                {
                    if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                        me->CastSpell(pTarget, SPELL_SHOCKWAVE, false);
                    shockwaveTimer = 25000;
                }
                else shockwaveTimer -= diff;

                if (devastateTimer <= diff)
                {
                    me->CastSpell(me->GetVictim(), SPELL_DEVASTATE, false);
                    devastateTimer = 8000;
                }
                else devastateTimer -= diff;

                if (vengeanceTimer <= diff)
                {
                    uint32 value = 100.0f - me->GetHealthPct();
                    me->CastCustomSpell(SPELL_VENGEANCE, SPELLVALUE_BASE_POINT0, value, me, true);
                    vengeanceTimer = 5000;
                }
                else vengeanceTimer -= diff;

                if (!siphonVitality)
                {
                    if (IsHeroic() && me->HealthBelowPct(20))
                    {
                        if (Creature* pGoriona = me->FindNearestCreature(NPC_GORIONA, 300.0f))
                            me->CastSpell(pGoriona, SPELL_SIPHON_VITALITY, false);
                        siphonVitality = true;
                    }
                }

                DoMeleeAttackIfReady();
            }
        }
    };
};

enum GorionaMoves
{
    MOVE_LEFT_POSITION              = 0,
    MOVE_RIGHT_POSITION             = 1,
};

const Position gorionaPos[7] =
{
    { 13400.00f, -12087.48f, 182.50f, 3.77f }, // Second Engine
    { 13367.23f, -12128.88f, 182.50f, 6.23f }, // Front of the ship
    { 13395.59f, -12175.28f, 182.50f, 0.88f }, // Third Engine
    { 13421.95f, -11930.60f, 183.00f, 1.17f }, // Fly Away position
    { 13426.70f, -12132.45f, 151.20f, 6.21f }, // Heroic land position
};

enum GorionaDrakesPosition
{
    LEFT_DRAKE_SPAWN_POS              = 0,
    RIGHT_DRAKE_SPAWN_POS             = 1,
    LEFT_DRAKE_DROP_POS               = 2,
    RIGHT_DRAKE_DROP_POS              = 3,
    LEFT_DRAKE_END_FLY_POS            = 4,
    RIGHT_DRAKE_END_FLY_POS           = 5,
};

const Position assaultDrakePos[6] =
{
    { 13441.83f, -12184.15f, 172.05f, 1.49f }, // Left Spawn Drake Pos
    { 13447.38f, -12083.55f, 172.05f, 4.45f }, // Right Spawn Drake Pos
    { 13431.26f, -12125.28f, 172.05f, 3.08f }, // Left drake drop add position
    { 13430.35f, -12140.22f, 172.05f, 3.08f }, // Right drake drop add position
    { 13433.40f, -12082.54f, 172.05f, 4.65f }, // Left drake end fly position
    { 13429.12f, -12182.25f, 172.05f, 1.46f }, // Right drake end fly position
};

const Position infiltratorPos[3] =
{
    { 13352.60f, -12150.22f, 181.30f, 0.03f }, // Left side Start fly position
    { 13353.51f, -12109.58f, 181.30f, 6.10f }, // Right side Start fly position
    { 13504.39f, -12135.67f, 185.00f, 6.22f }, // End fly position
};

// Goriona
class npc_ds_goriona : public CreatureScript
{
public:
    npc_ds_goriona() : CreatureScript("npc_ds_goriona") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_gorionaAI(pCreature);
    }

    struct npc_ds_gorionaAI : public ScriptedAI
    {
        npc_ds_gorionaAI(Creature *creature) : ScriptedAI(creature), Summons(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        TaskScheduler scheduler;
        SummonList Summons;

        uint32 twilightOnsloughtTimer;
        uint32 twilightFlamesTimer;
        uint32 twilightInfiltratorTimer;
        uint32 twilightBreathTimer;
        uint32 consumingShroudTimer;
        uint32 waveTimer;
        uint32 waveCount;
        uint32 phase;
        bool nextWave;
        bool destroyEngines;
        bool flyAway;
        bool heroicLand;

        void Reset() override
        {
            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->GetMotionMaster()->MoveIdle();
            me->SetFlying(true);
            me->SetFloatValue(UNIT_FIELD_COMBATREACH, 5.0f);
            me->SetVisible(false);
            me->SetSpeed(MOVE_FLIGHT, 3.0f);

            twilightOnsloughtTimer = 35000;
            twilightFlamesTimer = 8000;
            twilightInfiltratorTimer = 70000;

            phase = PHASE_ZERO;
            waveCount = 0;

            nextWave = false;
            destroyEngines = false;
            flyAway = false;
            heroicLand = false;

            Summons.DespawnAll();
        }

        void JustSummoned(Creature* summon) override
        {
            switch (summon->GetEntry())
            {
            case NPC_TWILIGHT_INFILTRATOR:
                summon->GetMotionMaster()->MovePoint(0, infiltratorPos[2], true);
                break;
            case NPC_TWILIGHT_ASSAULT_DRAKE_LEFT:
                if (Creature * pPassenger = summon->SummonCreature(NPC_TWILIGHT_ELITE_SLAYER, summon->GetPositionX(), summon->GetPositionY(), summon->GetPositionZ(), summon->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN))
                    pPassenger->EnterVehicle(summon, 0, nullptr);
                summon->GetMotionMaster()->MovePoint(LEFT_DRAKE_DROP_POS, assaultDrakePos[LEFT_DRAKE_DROP_POS], true);
                break;
            case NPC_TWILIGHT_ASSAULT_DRAKE_RIGHT:
                if (Creature * pPassenger = me->SummonCreature(NPC_TWILIGHT_ELITE_DREADBLADE, summon->GetPositionX(), summon->GetPositionY(), summon->GetPositionZ(), summon->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN))
                    pPassenger->EnterVehicle(summon, 0, nullptr);
                summon->GetMotionMaster()->MovePoint(RIGHT_DRAKE_DROP_POS, assaultDrakePos[RIGHT_DRAKE_DROP_POS], true);
                break;
            default:
                break;
            }

            Summons.push_back(summon->GetGUID());
        }

        void EnterCombat(Unit * /*who*/) override
        {
            phase = PHASE_ONE;
        }

        void KilledUnit(Unit* victim) override {}

        void SummonNPC(uint32 entry)
        {
            switch (entry)
            {
            case NPC_TWILIGHT_ASSAULT_DRAKE_LEFT:
                me->SummonCreature(entry, assaultDrakePos[LEFT_DRAKE_SPAWN_POS], TEMPSUMMON_DEAD_DESPAWN);
                break;
            case NPC_TWILIGHT_ASSAULT_DRAKE_RIGHT:
                me->SummonCreature(entry, assaultDrakePos[RIGHT_DRAKE_SPAWN_POS], TEMPSUMMON_DEAD_DESPAWN);
                break;
            case NPC_TWILIGHT_INFILTRATOR:
                me->SummonCreature(entry, infiltratorPos[urand(0, 1)], TEMPSUMMON_TIMED_DESPAWN, 8000);
                break;
            default:
                break;
            }
        }

        void DoAction(const int32 action) override
        {
            if (instance)
            {
                switch (action)
                {
                case ACTION_START_ENCOUNTER:
                {
                    scheduler.Schedule(Seconds(7), [this](TaskContext intro)
                    {
                        Creature * pSkyfireEngine = me->FindNearestCreature(NPC_ENGINE_STALKER, 100.0f, true);
                        Creature* pShip = me->FindNearestCreature(NPC_SKYFIRE, 300.0f, true);
                        Creature * pWarmasterBlackhorn = me->FindNearestCreature(BOSS_WARMASTER_BLACKHORN, 300.0f, true);

                        if (intro.GetRepeatCounter() == 0)
                        {
                            me->SetVisible(true);
                            if (pWarmasterBlackhorn && pShip)
                            {
                                pWarmasterBlackhorn->SetVisible(true);
                                pWarmasterBlackhorn->MonsterYell("Hah! I was hoping you'd make it this far. You'd best be ready for a real fight.", LANG_UNIVERSAL, 0);
                                pWarmasterBlackhorn->SendPlaySound(26214, false);
                                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, pShip);
                            }
                            me->GetMotionMaster()->MovePoint(0, gorionaPos[0], true);
                            if (pSkyfireEngine)
                                me->CastSpell(pSkyfireEngine, 99148, true);
                            intro.Repeat(Seconds(8));
                        }
                        else if (intro.GetRepeatCounter() == 1)
                        {
                            me->GetMotionMaster()->MovePoint(0, gorionaPos[1], true);
                            if (pSkyfireEngine)
                                me->CastSpell(pSkyfireEngine, 99148, true);
                            intro.Repeat(Seconds(3));
                        }
                        else if (intro.GetRepeatCounter() == 2)
                        {
                            me->GetMotionMaster()->MovePoint(0, gorionaPos[2], true);
                            intro.Repeat(Seconds(3));
                        }
                        else if (intro.GetRepeatCounter() == 3)
                        {
                            if (pSkyfireEngine)
                                me->CastSpell(pSkyfireEngine, 99148, true);
                            intro.Repeat(Seconds(7));
                        }
                        else if (intro.GetRepeatCounter() == 4)
                        {
                            me->SetInCombatWithZone();
                            if (pShip)
                            {
                                pShip->SetInCombatWith(me);
                                me->SetInCombatWith(pShip);
                                pShip->AddThreat(me, 50.0f);
                            }

                            SummonNPC(NPC_TWILIGHT_ASSAULT_DRAKE_LEFT);
                            SummonNPC(NPC_TWILIGHT_ASSAULT_DRAKE_RIGHT);
                            waveTimer = 60000;
                            nextWave = true;
                        }
                    });
                    break;
                }
                case ACTION_GORIONA_LEAVE_SCENE:
                    me->SetReactState(REACT_PASSIVE);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    me->GetMotionMaster()->MovePoint(0, gorionaPos[5], true);
                    flyAway = true;
                    break;
                case ACTION_PHASE_TWO:
                    twilightFlamesTimer = 8000;
                    phase = PHASE_TWO;
                    me->GetMotionMaster()->MovePoint(0, gorionaPos[4], true);
                    scheduler.Schedule(Seconds(3), [this](TaskContext /*task context*/)
                    {
                        if (Vehicle * veh = me->GetVehicleKit())
                        {
                            if (Unit * passenger = veh->GetPassenger(0))
                                passenger->ExitVehicle();
                        }
                        me->GetMotionMaster()->MovePoint(0, gorionaPos[0], true);
                        if (Creature * pBlackhorn = me->FindNearestCreature(BOSS_WARMASTER_BLACKHORN, 300.0f, true))
                            pBlackhorn->AI()->DoAction(ACTION_PHASE_TWO);
                    });
                    break;
                default:
                    break;
                }
            }
        }

        void SpellHitTarget(Unit* pTarget, const SpellEntry* spell) 
        {
            if (!spell || !pTarget)
                return;

            if (spell->Id == 99148)
                pTarget->CastSpell(pTarget, SPELL_ENGINE_FIRE, true);
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (nextWave)
            {
                if (waveTimer <= diff)
                {
                    waveCount++;

                    if (waveCount <= 2)
                    {
                        SummonNPC(NPC_TWILIGHT_ASSAULT_DRAKE_LEFT);
                        SummonNPC(NPC_TWILIGHT_ASSAULT_DRAKE_RIGHT);
                        waveTimer = 60000;
                    }
                    else nextWave = false;

                    if (waveCount == 3)
                        me->AI()->DoAction(ACTION_PHASE_TWO);
                }
                else waveTimer -= diff;
            }

            if (!UpdateVictim())
                return;

            if (phase == PHASE_ONE)
            {
                if (twilightOnsloughtTimer <= diff)
                {
                    uint32 randPos = urand(0, 24);
                    if (Creature * pOnslaughtTarget = me->SummonCreature(NPC_ONSLAUGHT_TARGET, warmasterDamagePos[randPos].GetPositionX(), warmasterDamagePos[randPos].GetPositionY(), warmasterDamagePos[randPos].GetPositionZ(), me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 10000))
                        pOnslaughtTarget->CastSpell(pOnslaughtTarget, SPELL_TWILIGHT_ONSLAUGHT_DUMMY, false);
                    me->CastSpell(warmasterDamagePos[randPos].GetPositionX(), warmasterDamagePos[randPos].GetPositionY(), warmasterDamagePos[randPos].GetPositionZ(), SPELL_TWILIGHT_ONSLAUGHT, false);
                    twilightOnsloughtTimer = 30000;
                }
                else twilightOnsloughtTimer -= diff;

                if (twilightInfiltratorTimer <= diff)
                {
                    SummonNPC(NPC_TWILIGHT_INFILTRATOR);
                    twilightInfiltratorTimer = 40000;
                }
                else twilightInfiltratorTimer -= diff;
            }
            else if (phase == PHASE_TWO)
            {
                if (IsHeroic())
                {
                    if (me->GetHealthPct() <= 80)
                        phase = PHASE_LAND;
                }

                if (twilightFlamesTimer <= diff)
                {
                    if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                        me->CastSpell(pTarget, SPELL_TWILIGHT_FLAMES, false);
                    twilightFlamesTimer = 8000;
                }
                else twilightFlamesTimer -= diff;
            }
            else if (phase == PHASE_LAND)
            {
                if (!heroicLand)
                {
                    if (me->GetHealthPct() <= 80)
                    {
                        me->GetMotionMaster()->MovePoint(0, gorionaPos[6], true, true);
                        twilightBreathTimer = 15000;
                        consumingShroudTimer = 20000;
                        heroicLand = true;
                    }
                }
                else
                {
                    if (twilightBreathTimer <= diff)
                    {
                        me->CastSpell(me->GetVictim(), SPELL_TWILIGHT_BREATH, false);
                        twilightBreathTimer = 20000;
                    }
                    else twilightBreathTimer -= diff;

                    if (consumingShroudTimer <= diff)
                    {
                        if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                            me->CastSpell(pTarget, SPELL_CONSUMING_SHROUD, false);
                        consumingShroudTimer = 20000;
                    }
                    else consumingShroudTimer -= diff;

                    DoMeleeAttackIfReady();
                }
            }

            // Fuck it, I'm out of here
            if (!flyAway && me->GetHealthPct() <= 20)
            {
                me->AI()->DoAction(ACTION_GORIONA_LEAVE_SCENE);
            }
        }
    };
};

class spell_ds_twilight_barrage_dmg : public SpellScriptLoader
{
public:
    spell_ds_twilight_barrage_dmg() : SpellScriptLoader("spell_ds_twilight_barrage_dmg") { }

    class spell_ds_twilight_barrage_dmg_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_twilight_barrage_dmg_SpellScript);

        void FilterTargetsDamage(std::list<Unit*>& targets)
        {
            if (!GetCaster())
                return;

            if (targets.empty())
            {
                if (Creature* pShip = GetCaster()->FindNearestCreature(NPC_SKYFIRE, 300.0f))
                {
                    int32 bp0 = GetSpellInfo()->EffectBasePoints[EFFECT_0];
                    bp0 *= 1.5f;
                    GetCaster()->CastCustomSpell(pShip, SPELL_TWILIGHT_BARRAGE_DMG, &bp0, NULL, NULL, true);
                }
            }
        }

        void Register()
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_twilight_barrage_dmg_SpellScript::FilterTargetsDamage, EFFECT_0, TARGET_UNIT_AREA_ENEMY_SRC);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_twilight_barrage_dmg_SpellScript();
    }
};

class spell_ds_twilight_onslaught_dmg : public SpellScriptLoader
{
public:
    spell_ds_twilight_onslaught_dmg() : SpellScriptLoader("spell_ds_twilight_onslaught_dmg") { }

    class spell_ds_twilight_onslaught_dmg_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_twilight_onslaught_dmg_SpellScript);

        void FilterTargetsDamage(std::list<Unit*>& targets)
        {
            if (!GetCaster())
                return;

            if (Creature* pShip = GetCaster()->FindNearestCreature(NPC_SKYFIRE, 300.0f))
            {
                int32 bp0 = GetSpellInfo()->EffectBasePoints[EFFECT_0];
                bp0 /= targets.size() + 1;
                GetCaster()->CastCustomSpell(pShip, SPELL_TWILIGHT_ONSLAUGHT_DMG, &bp0, NULL, NULL, true);
            }
        }

        void Register()
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_twilight_onslaught_dmg_SpellScript::FilterTargetsDamage, EFFECT_0, TARGET_UNIT_AREA_ENEMY_SRC);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_twilight_onslaught_dmg_SpellScript();
    }
};

class spell_ds_warmaster_blackhorn_vengeance : public SpellScriptLoader
{
public:
    spell_ds_warmaster_blackhorn_vengeance() : SpellScriptLoader("spell_ds_warmaster_blackhorn_vengeance") { }

    class spell_ds_warmaster_blackhorn_vengeance_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_ds_warmaster_blackhorn_vengeance_AuraScript);

        void HandlePeriodicTick(AuraEffect const* aurEff /*aurEff*/)
        {
            if (!GetUnitOwner())
                return;

            uint32 val = int32(100.0f - GetUnitOwner()->GetHealthPct());

            if (AuraEffect * aurEff = GetAura()->GetEffect(EFFECT_0))
                aurEff->SetAmount(val);
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_ds_warmaster_blackhorn_vengeance_AuraScript::HandlePeriodicTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_ds_warmaster_blackhorn_vengeance_AuraScript();
    }
};

void AddSC_boss_warmaster_blackhorn()
{
    new boss_warmaster_blackhorn();
    new npc_ds_goriona();
    new spell_ds_twilight_barrage_dmg();
    new spell_ds_twilight_onslaught_dmg();
    new spell_ds_warmaster_blackhorn_vengeance();
}