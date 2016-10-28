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

 /*
 Encounter: Morchok
 Dungeon: Dragon Soul
 Difficulty: Normal / Heroic
 Mode: 10-man normal/ 10-man heroic
 Autor: Artisan
 */

#include "ScriptPCH.h"
#include "dragonsoul.h"
#include "MapManager.h"
#include "TaskScheduler.h"

static const PlayableQuote firstIntroQuotes[6] = 
{
    { 26282, "No mortal shall turn me from my task!" }, // Morchok
    { 26531, "Wyrmrest Accord, attack!" }, // Lord Afrasastrasz
    { 26305, "They have broken our defenses! The very earth turns against us in Deathwing's name." }, // Image of Tyrygosa
    { 26306, "You must hurry...Wyrmrest falls as we speak. All...is lost." }, // Image of Tyrygosa
    { 26532, "Tyrygosa yet lives! We must press on, to the Temple!" }, // Lord Afrasastrasz
    { 26270, "Cowards! Weaklings! Come down and fight or I will bring you down!" }, // Morchok
};

static const PlayableQuote secondIntroQuotes[5] =
{
    { 26271, "You cannot hide in this temple forever, shaman!" }, // Morchok
    { 26273, "Wyrmrest will fall. All will be dust." }, // Morchok
    { 26533, "Advance to the front!" }, // Lord Afrasastrasz
    { 26534, "The siege must be broken! Wyrmrest Accord, defend the line!" }, // Lord Afrasastrasz
    { 26272, " I will turn this tower to rubble and scatter it across the wastes." }, // Morchok
};

static const PlayableQuote aggroQuote = { 26268, "You seek to halt an avalanche. I will bury you." };

static const PlayableQuote summonMorchokQuote = { 26288, "You thought to fight me alone? The earth splits to swallow and crush you." };

#define MAX_SUMMON_CRYSTAL_QUOTES 2

static const PlayableQuote summonCrystalQuotes[MAX_SUMMON_CRYSTAL_QUOTES] =
{
    { 26283, "Flee and die!" },
    { 26284, "Run, and perish." },
};

#define MAX_VORTEX_QUOTES 4

static const PlayableQuote vortexQuotes[MAX_VORTEX_QUOTES] =
{
    { 26274, "The stone calls..." },
    { 26275, "The ground shakes..." },
    { 26276, "The rocks tremble..." },
    { 26277, "The surface quakes..." },
};

#define MAX_BLACK_BLOOD_QUOTES 4

static const PlayableQuote blackBloodQuotes[MAX_BLACK_BLOOD_QUOTES] =
{
    { 26278, "...and the black blood of the earth consumes you." },
    { 26279, "...and there is no escape from the old gods." },
    { 26280, "...and the rage of the true gods follows." },
    { 26281, "...and you drown in the hate of The Master." },
};

#define MAX_KILL_QUOTES 3

static const PlayableQuote killQuotes[MAX_KILL_QUOTES] =
{
    { 26285, "I am unstoppable." },
    { 26286, "It was inevitable." },
    { 26287, "Ground to dust." },
};

#define MAX_DEATH_QUOTES 2
#define DEATH_QUOTE_MORCHOK_INDEX 0
#define DEATH_QUOTE_LORD_AFASASTRASZ_INDEX 1

static const PlayableQuote deathQuotes[MAX_DEATH_QUOTES] =
{
    { 26269, "Impossible. This cannot be. The tower...must...fall." }, // Morchok
    { 26535, "The Twilight's Hammer is retreating! The temple is ours; fortify your positions within!" }, // Lord Afasastrasz
};

enum Spells : uint32
{
    SPELL_MORCHOK_SIEGE_MISSILE         = 110307,
    SPELL_STOMP                         = 103414,
    SPELL_CRUSH_ARMOR                   = 103687,
    SPELL_RESONATING_CRYSTAL_MISSILE    = 103640,   // missile which spawn crystal on players nearby location
    SPELL_FURIOUS                       = 103846,
    SPELL_EARTH_VENGEANCE               = 103176,   // summon falling Fragments GOs SPELL_AURA_PERIODIC_TRIGGER_SPELL
    SPELL_EARTH_VORTEX                  = 103821,   // (pull) teleport players in front of the boss + summon npc ( Vehicle ??? )
    SPELL_EARTH_VORTEX_VEHICLE_AURA     = 109615,
    SPELL_FALLING_FRAGMENT_MISSILE      = 103177,   // spawn fragment
    SPELL_BLACK_BLOOD_VISUAL_MISSILE    = 103180,   // visual black blood on destination position
    SPELL_BLACK_BLOOD_CHANNEL           = 103851,   // trigger SPELL_BLACK_BLOOD_DAMAGE periodicaly
    SPELL_BLACK_BLOOD_DAMAGE            = 103785,   // radius must be calculated manually
    SPELL_BURROW                        = 108246,
    SPELL_BERSERK                       = 47008,

    // HC spells
    SPELL_SUMMON_KOHCROM                = 109017,
    SPELL_JUMP                          = 109070,
    SPELL_SHRINK                        = 96112,    // - 10 % scale
    SPELL_SHARE_HEALTH                  = 109548,

    ACHIEVEMENT_HEROIC_MORCHOK            = 6109,
    ACHIEVEMENT_DONT_STAY_SO_CLOSE_TO_ME  = 6174,
};

enum resonatingCrystalSpells : uint32
{
    SPELL_RESONATING_CRYSTAL_DAMAGE     = 103545,
    SPELL_RESONATING_CRYSTAL_10_HC      = 110041,
    SPELL_RESONATING_CRYSTAL_VISUAL     = 103494,

    SPELL_CRYSTAL_TARGET_SELECTION_10   = 103528, // triggered by SPELL_RESONATING_CRYSTAL_VISUAL
    SPELL_CRYSTAL_TARGET_SELECTION_25   = 104573, // triggered by SPELL_RESONATING_CRYSTAL_VISUAL

    DANGER_BEAM                         = 103534,
    WARNING_BEAM                        = 103536,
    SAFE_BEAM                           = 103541
};

enum creatureEntries : uint32
{
    WYRMEST_PROTECTOR_ENTRY             = 27953,
    MORCHOK_ENTRY                       = 55265,
    KOHCROM_ENTRY                       = 57773, // Heroic twin 
    EARTHEN_VORTEX_ENTRY                = 55723,
    RESONATING_CRYSTAL_ENTRY            = 55346,
    EARTHEN_VORTEX_VEHICLE_ENTRY        = 55723
};

enum actions : int32
{
    ACTION_BLACK_BLOOD_TICK             = 1,
    MAX_BLACK_BLOOD_TICKS               = 15,
    ACTION_SPAWN_FRAGMENT,
    ACTION_CAST_BLACK_BLOOD
};

enum goEntries : uint32
{
    GO_FALLING_FRAGMENT_ENTRY           = 209596
};

#define GRID_SEARCH_DISTANCE        250.0f
#define WYRMEST_TEMPLE_ORIENTATION  0.21f
#define KOHCROM_TIMERS_OFFSET_MILIS 6000

#define SAFE_BEAM_DISTANCE          5.0f
#define WARNING_BEAM_DISTANCE       20.0f

#define MAX_CRYSTAL_TARGETS_10_MAN  3
#define MAX_CRYSTAL_TARGETS_25_MAN  7

namespace MorchokHelpers
{
    static void RemoveBeamAuraFromPlayers(InstanceScript * instance)
    {
        if (!instance)
            return;

        instance->DoRemoveAurasDueToSpellOnPlayers(SAFE_BEAM);
        instance->DoRemoveAurasDueToSpellOnPlayers(WARNING_BEAM);
        instance->DoRemoveAurasDueToSpellOnPlayers(DANGER_BEAM);
    }
}

struct ElementalAI : public ScriptedAI
{
    ElementalAI(Creature* c) : ScriptedAI(c), summonList(c)
    {
        instance = c->GetInstanceScript();
        me->SetFloatValue(UNIT_FIELD_COMBATREACH, 10.0f);
    }

    void RunPlayableQuote(PlayableQuote quote, bool yell = true) override
    {
        if (me->GetEntry() == MORCHOK_ENTRY)
        {
            ScriptedAI::RunPlayableQuote(quote);
        }
    }

    void DeactivateFragments()
    {
        for (std::list<uint64>::const_iterator itr = goGUIDs.cbegin(); itr != goGUIDs.cend(); itr++)
        {
            if (GameObject *go = ObjectAccessor::GetGameObject(*me, *itr))
            {
                go->SetGoState(GO_STATE_ACTIVE);
                go->EnableCollision(false);
            }
        }
        goGUIDs.clear();
    }

    InstanceScript* instance;
    SummonList summonList;
    TaskScheduler scheduler;
    std::list<uint64> goGUIDs;

    uint32 stompTimer;
    uint32 crushArmorTimer;
    uint32 crystalTimer;
    uint32 vortexTimer;
    uint32 bloodTicks;

    bool canCastBlackBlood;

    void EnterCombat(Unit * who) override
    {
        if (me->GetEntry() == MORCHOK_ENTRY)
        {
            crushArmorTimer = 6000;
            stompTimer = 11000;
            crystalTimer = 18000;
            vortexTimer = 55000;

            me->GetMotionMaster()->MoveChase(who);
            MorchokHelpers::RemoveBeamAuraFromPlayers(instance);

            scheduler.Schedule(Minutes(7), [this](TaskContext)
            {
                me->CastSpell(me, SPELL_BERSERK, true);

                if (Creature * pKohcrom = me->FindNearestCreature(KOHCROM_ENTRY, GRID_SEARCH_DISTANCE, true))
                    pKohcrom->CastSpell(pKohcrom, SPELL_BERSERK, true);
            });
        }

        bloodTicks = 0;
        canCastBlackBlood = false;

        if (instance)
        {
            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);

            if (me->GetEntry() == KOHCROM_ENTRY)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_UPDATE_PRIORITY, me);
        }

        ScriptedAI::EnterCombat(who);
    }

    void EnterEvadeMode() override
    {
        me->SetReactState(REACT_AGGRESSIVE);

        if (instance)
        {
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        }

        me->GetMotionMaster()->MoveTargetedHome();

        summonList.DespawnAll();
        scheduler.CancelAll();
        DeactivateFragments();
        MorchokHelpers::RemoveBeamAuraFromPlayers(instance);

        ScriptedAI::EnterEvadeMode();
    }

    virtual void KilledUnit(Unit* victim) override = 0;

    void JustDied(Unit * killer) override
    {
        if (instance)
        {
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            if (IsHeroic())
                instance->DoCompleteAchievement(ACHIEVEMENT_HEROIC_MORCHOK);

            instance->DoModifyPlayerCurrencies(CURRENCY_MOTE_OF_DARKNESS, 1, CURRENCY_SOURCE_OTHER);
        }

        MorchokHelpers::RemoveBeamAuraFromPlayers(instance);
        DeactivateFragments();
        summonList.DespawnAll();

        ScriptedAI::JustDied(killer);
    }

    /*
        SHRINKING (applied only in heroic):
        - shrinking aura can be stacked max 8 times
        - first cast of shrink is cast at 80 % HP
        - after every 10 % missing health, one stack of SPELL_SHRINK is applied
        - 80, 70, 60, 50, 40, 30, 20, 10 -> total 8 times
    */
    void DamageTaken(Unit *, uint32 &damage) override
    {
        if (!IsHeroic())
            return;

        uint32 hpPCT = uint32(me->GetHealthPct());

        if (hpPCT < 80 && !me->HasAura(SPELL_SHRINK))
            me->CastSpell(me, SPELL_SHRINK, true);

        if (Aura * shrinkAura = me->GetAura(SPELL_SHRINK))
        {
            if (hpPCT % 10 == 0 && hpPCT != 0)
            {
                if (shrinkAura->GetStackAmount() == 8 - (hpPCT / 10))
                    me->CastSpell(me, SPELL_SHRINK, true);
            }
        }
    }

    void OnWildGameObjectSummon(GameObject* go) override
    {
        if (go->GetGOInfo()->id == GO_FALLING_FRAGMENT_ENTRY)
        {
            goGUIDs.push_back(go->GetGUID());
            go->EnableCollision(true);
            go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
        }
    }

    void JustSummoned(Creature* pSummon) override
    {
        summonList.Summon(pSummon);
    }

    void DoAction(const int32 action) override
    {
        switch (action)
        {
            case ACTION_SPAWN_FRAGMENT:
            {
                float angle = frand(0.0f, 2.0f * M_PI);
                const uint32 distance = urand(35, 42);
                float x = me->GetPositionX() + cos(angle) * float(distance);
                float y = me->GetPositionY() + sin(angle) * float(distance);
                float z = me->GetMap()->GetHeight(me->GetPhaseMask(), x, y, me->GetPositionZ() + 10.0f, true);

                me->CastSpell(x, y, z, SPELL_FALLING_FRAGMENT_MISSILE, true);
                break;
            }
            case ACTION_CAST_BLACK_BLOOD:
            {
                me->InterruptNonMeleeSpells(false);
                bloodTicks = 0;
                canCastBlackBlood = true;
                break;
            }
            default:
                break;
        }

        /*
        Ticks from [Black Blood of the Earth] spell should play black blood animation
        And again blizzard bullshit starting (fasten your seatbelts please)
        - Black blood on ground is only animation from [SPELL_BLACK_BLOOD_VISUAL_MISSILE] spell, which takes 4 seconds
        - it has visual radius cca 10 yards and should spread from boss with bigger nad bigger circles
        - so we must cast missiles in these circles every periodic tick
        - black blood should not be casted behind LoS from boss (behind falling fragments)
        - BIG BUT !!! Black blood channeling last for 15 seconds with 1 second tick, which menas that we need to recast missiles to hold animation at place till channeling ends
        - All this should be handled in SpellScript but, if we did that, we could not use TaskSheduler to easily capture x,y,z location and recast it multiple times (missing Update call)
        - so we are doing it in AI ...
        */
        if (action >= ACTION_BLACK_BLOOD_TICK && action <= MAX_BLACK_BLOOD_TICKS)
        {
            bloodTicks++;
            uint32 bloods = (2.0f * M_PI * float(action));
            float angle = 0.0f;

            for (uint32 i = 0; i < bloods; i++)
            {
                angle += (2.0f * M_PI) / float(bloods);
                angle = MapManager::NormalizeOrientation(angle);
                const uint32 distance = bloodTicks * 10;

                if (float(distance) >= 100.0f)
                    break;

                float x = me->GetPositionX() + cos(angle) * float(distance);
                float y = me->GetPositionY() + sin(angle) * float(distance);
                float z = me->GetMap()->GetHeight(me->GetPhaseMask(), x, y, me->GetPositionZ() + 10.0f, true);

                // Don't cast black blood behind fragments
                if (me->IsWithinLOS(x, y, z))
                {
                    me->CastSpell(x, y, z, SPELL_BLACK_BLOOD_VISUAL_MISSILE, true);

                    // Capture coordinates of missile to repeat animation to same place after 4 seconds
                    scheduler.Schedule(Seconds(4), [this, x, y, z](TaskContext context)
                    {
                        me->CastSpell(x, y, z, SPELL_BLACK_BLOOD_VISUAL_MISSILE, true);

                        // Repeat total 3 times ( first time + scheduled + repeat schedule)
                        if (context.GetRepeatCounter() == 0)
                            context.Repeat();
                    });
                }
            }
        }
    }

    void StopMoving()
    {
        // This is kinda hacky, but nothing else wont work, to immediately stop movement
        me->GetMotionMaster()->MovePoint(0,me->GetPositionX(),me->GetPositionY(),me->GetPositionZ());
        me->GetMotionMaster()->MoveIdle(MOTION_SLOT_IDLE);
        me->SetReactState(REACT_PASSIVE);
        me->AttackStop();

        // Schedule attacking again in 4 seconds
        scheduler.Schedule(Seconds(4), [this](TaskContext)
        {
            me->SetInCombatWithZone();
            me->Attack(me->GetVictim(), true);
            me->SetReactState(REACT_AGGRESSIVE);
            me->GetMotionMaster()->MoveChase(me->GetVictim());
        });
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type == EFFECT_MOTION_TYPE) // At jump movement finished
        {
            StopMoving();
        }
    }

    inline bool IsCastingAllowed()
    {
        return !me->IsNonMeleeSpellCasted(false) && me->GetReactState() == REACT_AGGRESSIVE;
    }

    void UpdateAI(const uint32 diff) override
    {
        scheduler.Update(diff);

        if (!UpdateVictim())
            return;

        if (canCastBlackBlood)
        {
            if (!IsCastingAllowed())
                return;

            uint32 randInt = urand(0, MAX_BLACK_BLOOD_QUOTES - 1);
            RunPlayableQuote(blackBloodQuotes[randInt]);

            // Start channeling after summonning of fragments
            me->CastSpell(me, SPELL_BLACK_BLOOD_CHANNEL, false);

            scheduler.Schedule(Seconds(16), [this](TaskContext)
            {
                me->GetMotionMaster()->MoveChase(me->GetVictim());
            });

            canCastBlackBlood = false;
            return;
        }

        // Crush armor available only in heroic
        if (!IsHeroic() && crushArmorTimer <= diff)
        {
            if (IsCastingAllowed())
            {
                me->CastSpell(me->GetVictim(), SPELL_CRUSH_ARMOR, true);
                crushArmorTimer = 6000;
            }
        }
        else crushArmorTimer -= diff;

        if (stompTimer <= diff)
        {
            if (IsCastingAllowed())
            {
                me->CastSpell(me, SPELL_STOMP, false);
                stompTimer = 12000;
            }
        }
        else stompTimer -= diff;

        if (crystalTimer <= diff)
        {
            if (IsCastingAllowed() && vortexTimer > 20000) // 12 till explosion + 1 s to spawn + few seconds to recover
            {
                uint32 randInt = urand(0, MAX_SUMMON_CRYSTAL_QUOTES - 1);
                RunPlayableQuote(summonCrystalQuotes[randInt]);

                uint32 pos = me->getThreatManager().getThreatList().size() == 1 ? 0 : 1;

                if (Unit * plr = SelectTarget(SELECT_TARGET_RANDOM, pos, 30.0f, true))
                    me->CastSpell(plr->GetPositionX(), plr->GetPositionY(), plr->GetPositionZ(), SPELL_RESONATING_CRYSTAL_MISSILE, true);

                crystalTimer = 14000;
            }
        }
        else crystalTimer -= diff;

        if (vortexTimer <= diff)
        {
            if (IsCastingAllowed())
            {
                uint32 randInt = urand(0, MAX_VORTEX_QUOTES - 1);
                RunPlayableQuote(vortexQuotes[randInt]);

                me->CastSpell(me, SPELL_EARTH_VORTEX, true); // pull players
                me->CastSpell(me, SPELL_EARTH_VENGEANCE, false); // start summoning fragments

                // Despawn fragments after 22 seconds
                scheduler.Schedule(Seconds(22), [this](TaskContext context)
                {
                    DeactivateFragments();
                });

                stompTimer = 40000;
                crystalTimer = 47000;
                crushArmorTimer = 30000;

                vortexTimer = 97000;
            }
        }
        else vortexTimer -= diff;

        // get furious at 20 % of health
        if (me->HealthBelowPct(20) && !me->HasAura(SPELL_FURIOUS))
            me->CastSpell(me, SPELL_FURIOUS, true);

        if (me->HasReactState(REACT_AGGRESSIVE))
            DoMeleeAttackIfReady();
    }
};

class boss_morchok_dragon_soul : public CreatureScript
{
public:
    boss_morchok_dragon_soul() : CreatureScript("boss_morchok_dragon_soul") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_morchok_dragon_soulAI(creature);
    }

    struct boss_morchok_dragon_soulAI : public ElementalAI
    {
        boss_morchok_dragon_soulAI(Creature* creature) : ElementalAI(creature) 
        {
            me->SetFacingTo(WYRMEST_TEMPLE_ORIENTATION);
        }

        bool kohcromSummoned = false;

        void SummonedCreatureDespawn(Creature *pSummon) override
        {
            if (pSummon->GetEntry() == KOHCROM_ENTRY)
            {
                if (instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, pSummon);
            }
        }

        void ScheduleWyrmestSiege()
        {
            // Every 10 seconds throw crystal at Wyrmest temple
            scheduler.Schedule(Seconds(10), [this](TaskContext context)
            {
                Creature * protector = me->FindNearestCreature(WYRMEST_PROTECTOR_ENTRY, GRID_SEARCH_DISTANCE, true);

                if (protector && !me->IsInCombat())
                    me->CastSpell(protector, SPELL_MORCHOK_SIEGE_MISSILE, false);

                context.Repeat();
            });
        }

        void Reset() override
        {
            kohcromSummoned = false;

            if (instance)
            {
                if (instance->GetData(TYPE_BOSS_MORCHOK) != DONE)
                    instance->SetData(TYPE_BOSS_MORCHOK, NOT_STARTED);
            }

            ScheduleWyrmestSiege();
        }

        void EnterEvadeMode() override
        {
            ElementalAI::EnterEvadeMode();
            ScheduleWyrmestSiege();
            kohcromSummoned = false;
            Reset();
        }

        void JustReachedHome() override
        {
            me->SetFacingTo(WYRMEST_TEMPLE_ORIENTATION);
        }

        void EnterCombat(Unit * who) override
        {
            ElementalAI::EnterCombat(who);

            if (instance)
            {
                instance->SetData(TYPE_BOSS_MORCHOK, IN_PROGRESS);
            }

            RunPlayableQuote(aggroQuote);
            Reset();
        }

        void KilledUnit(Unit* victim) override
        {
            ScriptedAI::EnterCombat(victim);

            if (victim->GetTypeId() == TYPEID_PLAYER)
            {
                uint32 randInt = urand(0, MAX_KILL_QUOTES - 1);
                RunPlayableQuote(killQuotes[randInt]);
            }
        }

        void JustDied(Unit * killer) override
        {
            ElementalAI::JustDied(killer);

            if (instance)
            {
                instance->SetData(TYPE_BOSS_MORCHOK, DONE);
            }

            RunPlayableQuote(deathQuotes[DEATH_QUOTE_MORCHOK_INDEX]);
        }

        void UpdateAI(const uint32 diff) override
        {
            ElementalAI::UpdateAI(diff);

            if (!UpdateVictim())
                return;

            if (IsCastingAllowed() && me->HealthBelowPct(90) && !kohcromSummoned && IsHeroic() && instance)
            {
                kohcromSummoned = true;

                // Handled in spellscript
                me->CastSpell(me, SPELL_SUMMON_KOHCROM, true);
                RunPlayableQuote(summonMorchokQuote);

                instance->SendEncounterUnit(ENCOUNTER_FRAME_UPDATE_PRIORITY, me);
            }
        }
    };
};

class boss_kohcrom_dragon_soul : public CreatureScript
{
public:
    boss_kohcrom_dragon_soul() : CreatureScript("boss_kohcrom_dragon_soul") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_kohcrom_dragon_soulAI(creature);
    }

    struct boss_kohcrom_dragon_soulAI : public ElementalAI
    {
        boss_kohcrom_dragon_soulAI(Creature* creature) : ElementalAI(creature) 
        {
            if (Creature * pMorchok = me->FindNearestCreature(MORCHOK_ENTRY, GRID_SEARCH_DISTANCE, true))
                summonerGUID = pMorchok->GetGUID();
        }

        uint64 summonerGUID = 0;

        void Reset() override
        {
            if (Creature * pMorchok = ObjectAccessor::GetCreature(*me, summonerGUID))
            {
                me->CastSpell(pMorchok, SPELL_SHARE_HEALTH, true);
                pMorchok->CastSpell(me, SPELL_SHARE_HEALTH, true);

                ElementalAI* pAI = (ElementalAI*)(pMorchok->GetAI());

                // Delay all kohcrom timers by 5 second
                if (pAI)
                {
                    stompTimer = pAI->stompTimer + KOHCROM_TIMERS_OFFSET_MILIS;
                    crystalTimer = pAI->crystalTimer + KOHCROM_TIMERS_OFFSET_MILIS;
                    vortexTimer = pAI->vortexTimer + KOHCROM_TIMERS_OFFSET_MILIS + 18000;
                }
            }
        }

        void KilledUnit(Unit* victim) override
        {
            if (Creature * pMorchok = ObjectAccessor::GetCreature(*me, summonerGUID))
                pMorchok->AI()->KilledUnit(victim);
        }
    };
};

class npc_ds_resonating_crystal : public CreatureScript
{
public:
    npc_ds_resonating_crystal() : CreatureScript("npc_ds_resonating_crystal") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_ds_resonating_crystalAI(creature);
    }

    struct npc_ds_resonating_crystalAI : public ScriptedAI
    {
        npc_ds_resonating_crystalAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
        }

        uint32 explodeTimer;
        uint32 sizeTimer;

        void Reset() override
        {
            explodeTimer = 12000;
            sizeTimer = 3000;
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, SPELL_RESONATING_CRYSTAL_VISUAL, true);
        }

        void EnterEvadeMode() override {}

        void UpdateAI(const uint32 diff) override
        {
            if (explodeTimer <= diff)
            {
                me->CastSpell(me, SPELL_RESONATING_CRYSTAL_DAMAGE, false);
                explodeTimer = MAX_TIMER;
                me->RemoveAllAuras();
                MorchokHelpers::RemoveBeamAuraFromPlayers(me->GetInstanceScript());
                me->ForcedDespawn(1000);
            }
            else explodeTimer -= diff;

            if (sizeTimer <= diff) // Shards should shrink by time
            {
                if (AuraEffect * aurEff = me->GetAuraEffect(SPELL_RESONATING_CRYSTAL_VISUAL, EFFECT_1))
                {
                    if (aurEff->GetAmount() >= 50)
                    {
                        aurEff->ChangeAmount(aurEff->GetAmount() - 50);
                    }
                }
                sizeTimer = 2000;
            }
            else sizeTimer -= diff;
        }
    };
};

class npc_ds_earthen_vortex : public CreatureScript
{
public:
    npc_ds_earthen_vortex() : CreatureScript("npc_ds_earthen_vortex") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_ds_earthen_vortexAI(creature);
    }

    struct npc_ds_earthen_vortexAI : public PassiveAI
    {
        npc_ds_earthen_vortexAI(Creature* creature) : PassiveAI(creature)
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            me->CastSpell(me, SPELL_BURROW, true);
            me->ForcedDespawn(5000); // Duration of SPELL_EARTH_VORTEX aura
        }

        uint32 delayTimer = 500;

        void EnterCombat(Unit *) {}
        void EnterEvadeMode() override {}
        /*
            Overriding OnCharmed is crucial ! When player enters vehicle, he charms vehicle, which will cause deleting the whole AI and not respoding to anything
            But we want move with vehicle to boss position (handled in spellscript)
            Instead of this, we will inherit from PassiveAI + override some methods which could lead to reacting of npc to some actions.
        */
        void OnCharmed(bool) override {}

        void HandleVehicleMove()
        {
            // This vehicle should be player's minion
            if (TempSummon * summon = me->ToTempSummon())
            {
                if (Unit * playerSummoner = summon->GetSummoner())
                {
                    if (AuraEffect * vortexAuraEff = playerSummoner->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE_PERCENT, SPELLFAMILY_GENERIC, 5098, EFFECT_2))
                    {
                        // This is the smartest way to get correct boss elemental pointer (Morchok/Kohcrom)
                        if (Unit * elemental = vortexAuraEff->GetCaster())
                        {
                            Position pos;
                            elemental->GetNearPosition(pos, 2.0f, elemental->GetAngle(me));
                            me->SetSpeed(MOVE_RUN, 4.0f, true);
                            me->GetMotionMaster()->MovePoint(0, pos, true, true);
                        }
                    }
                }
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (delayTimer <= diff)
            {
                HandleVehicleMove();
                delayTimer = MAX_TIMER;
            }
            else delayTimer -= diff;
        }
    };
};

class spell_ds_resonating_blast : public SpellScriptLoader
{
public:
    spell_ds_resonating_blast() : SpellScriptLoader("spell_ds_resonating_blast") {}

    class spell_ds_resonating_blast_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_resonating_blast_SpellScript);

        uint32 unitsAffected = 0;

        void FilterTargets(std::list<Unit*>& unitList)
        {
            Unit * caster = GetCaster();

            // Exclude players not affected by beams
            for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end();)
            {
                if ((*itr)->HasAura(SAFE_BEAM, caster->GetGUID())
                || (*itr)->HasAura(WARNING_BEAM, caster->GetGUID())
                || (*itr)->HasAura(DANGER_BEAM, caster->GetGUID()))
                    itr++;
                else
                    itr = unitList.erase(itr);
            }

            unitsAffected = unitList.size();
        }

        void HandleDamage(SpellEffIndex /*effIndex*/)
        {
            Unit * caster = GetCaster();
            Unit * hitUnit = GetHitUnit();

            if (!caster || !hitUnit || unitsAffected == 0)
                return;

            int32 damage = GetHitDamage() / unitsAffected; // Split damage between beam targets

            // The total damage increases the further the targets are from the explosion
            if (hitUnit->HasAura(SAFE_BEAM))
                damage = damage * 150 / 100;
            else if (hitUnit->HasAura(WARNING_BEAM))
                damage = damage * 3;
            else if (hitUnit->HasAura(DANGER_BEAM))
                damage = damage * 5;

            SetHitDamage(damage);
        }

        void Register() override
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_resonating_blast_SpellScript::FilterTargets, EFFECT_ALL, TARGET_UNIT_AREA_ENEMY_DST);
            OnEffect += SpellEffectFn(spell_ds_resonating_blast_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_ds_resonating_blast_SpellScript();
    }
};

class spell_ds_morchok_stomp : public SpellScriptLoader
{
public:
    spell_ds_morchok_stomp() : SpellScriptLoader("spell_ds_morchok_stomp") {}

    class spell_ds_morchok_stomp_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_morchok_stomp_SpellScript);

        #define FIRST_CLOSEST_TARGET_INDEX      0
        #define SECOND_CLOSEST_TARGET_INDEX     1
        #define MAX_CLOSEST_UNITS               2

        uint64 guids[MAX_CLOSEST_UNITS];
        uint32 unitsAffected;

        bool Load() override
        {
            unitsAffected = 0;
            guids[FIRST_CLOSEST_TARGET_INDEX] = 0;
            guids[SECOND_CLOSEST_TARGET_INDEX] = 0;
            return true;
        }

        void FindClosestTargets(std::list<Unit*>& unitList)
        {
            Unit * caster = GetCaster();

            // Order affected units by distance descending
            unitList.sort(Trinity::ObjectDistanceOrderPred(caster));

            uint32 max = MAX_CLOSEST_UNITS;

            for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end(); ++itr)
            {
                if (max-- <= 0)
                    break;

                guids[max] = (*itr)->GetGUID();
            }

            unitsAffected = unitList.size();
        }

        void HandleDamage(SpellEffIndex /*effIndex*/)
        {
            Unit * hitUnit = GetHitUnit();

            if (!hitUnit || unitsAffected == 0)
                return;

            int32 damage = GetHitDamage() / unitsAffected; // Shared damage

            // Two closest targets take a double share of the damage
            if (hitUnit->GetGUID() == guids[FIRST_CLOSEST_TARGET_INDEX] || hitUnit->GetGUID() == guids[SECOND_CLOSEST_TARGET_INDEX])
                SetHitDamage(damage * 2);
            else
                SetHitDamage(damage);
        }

        void FilterNonTankPlayers(std::list<Unit*>& unitList)
        {
            Unit * caster = GetCaster();

            for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end();)
            {
                // Remove all targets except main tank
                if (caster->GetVictim() != *itr)
                {
                    itr = unitList.erase(itr);
                }
                else
                    itr++;
            }
        }

        void Register() override
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_morchok_stomp_SpellScript::FindClosestTargets, EFFECT_0, TARGET_UNIT_AREA_ENEMY_DST);
            OnEffect += SpellEffectFn(spell_ds_morchok_stomp_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_morchok_stomp_SpellScript::FilterNonTankPlayers, EFFECT_2, TARGET_UNIT_AREA_ENEMY_DST);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_ds_morchok_stomp_SpellScript();
    }
};

class spell_ds_summon_fragments : public SpellScriptLoader
{
public:
    spell_ds_summon_fragments() : SpellScriptLoader("spell_ds_summon_fragments") { }

    class spell_ds_summon_fragments_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_ds_summon_fragments_AuraScript);

        void HandleEffectPeriodic(AuraEffect const * aurEff)
        {
            Unit *caster = aurEff->GetBase()->GetCaster();

            if (!caster || caster->GetTypeId() != TYPEID_UNIT)
                return;

            Creature * morchok = caster->ToCreature();

            morchok->AI()->DoAction(ACTION_SPAWN_FRAGMENT);

            // Immediately cast black blood after spawning all fragments
            if (aurEff->GetTickNumber() == (uint32)aurEff->GetTotalTicks())
                morchok->AI()->DoAction(ACTION_CAST_BLACK_BLOOD);
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_ds_summon_fragments_AuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
        }
    };

    AuraScript *GetAuraScript() const override
    {
        return new spell_ds_summon_fragments_AuraScript();
    }
};

class spell_ds_summon_black_blood : public SpellScriptLoader
{
public:
    spell_ds_summon_black_blood() : SpellScriptLoader("spell_ds_summon_black_blood") { }

    class spell_ds_summon_black_blood_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_ds_summon_black_blood_AuraScript);

        void HandleEffectPeriodic(AuraEffect const * aurEff)
        {
            Unit *caster = aurEff->GetBase()->GetCaster();

            if (!caster || caster->GetTypeId() != TYPEID_UNIT)
                return;

            caster->ToCreature()->AI()->DoAction(ACTION_BLACK_BLOOD_TICK + aurEff->GetTickNumber()); // start + tick offset
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_ds_summon_black_blood_AuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
        }
    };

    AuraScript *GetAuraScript() const override
    {
        return new spell_ds_summon_black_blood_AuraScript();
    }
};

class spell_ds_black_blood_damage : public SpellScriptLoader
{
public:
    spell_ds_black_blood_damage() : SpellScriptLoader("spell_ds_black_blood_damage") {}

    class spell_ds_black_blood_damage_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_black_blood_damage_SpellScript);

        #define BLACK_BLOOD_MISSILE_VISUAL_RADIUS 7.0f
        #define BLACK_BLOOD_MAX_RADIUS  100.0f

        void RemoveFarTargets(std::list<Unit*>& unitList)
        {
            Creature * elemental = GetCaster()->ToCreature();

            if (!elemental)
                return;

            ElementalAI* pAI = (ElementalAI*)(elemental->GetAI());

            if (!pAI)
                return;

            // Remove targets that are further than actual black blood and targets behind fragments
            for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end();)
            {
                float maxDistance = BLACK_BLOOD_MISSILE_VISUAL_RADIUS * pAI->bloodTicks;

                // In heroic there will be also black blood from Kohcrom, so dont hit other players from back :)
                if (maxDistance >= BLACK_BLOOD_MAX_RADIUS)
                    maxDistance = BLACK_BLOOD_MAX_RADIUS;

                if (elemental->GetDistance(*itr) > maxDistance || !(*itr)->IsWithinLOSInMap(elemental))
                {
                    itr = unitList.erase(itr);
                }
                else
                    itr++;
            }
        }

        void Register() override
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_black_blood_damage_SpellScript::RemoveFarTargets, EFFECT_0, TARGET_UNIT_AREA_ENEMY_DST);
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_black_blood_damage_SpellScript::RemoveFarTargets, EFFECT_1, TARGET_UNIT_AREA_ENEMY_DST);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_ds_black_blood_damage_SpellScript();
    }
};

class spell_ds_target_selection : public SpellScriptLoader
{
public:
    spell_ds_target_selection() : SpellScriptLoader("spell_ds_target_selection") { }

    class spell_ds_target_selection_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_target_selection_SpellScript);

        inline uint32 GetBeamSpellId(float distance)
        {
            if (distance < SAFE_BEAM_DISTANCE)
                return SAFE_BEAM;

            if (distance < WARNING_BEAM_DISTANCE)
                return WARNING_BEAM;

            return DANGER_BEAM;
        }

        void HandleSelection(std::list<Unit*>& unitList)
        {
            Unit * me = GetCaster();

            // Order by distance ascending
            unitList.sort(Trinity::ObjectDistanceOrderPred(me));

            // Get count of players wh an be affected by beams
            int32 beamPlayersCount = GetSpellInfo()->Id == SPELL_CRYSTAL_TARGET_SELECTION_10 ? MAX_CRYSTAL_TARGETS_10_MAN : MAX_CRYSTAL_TARGETS_25_MAN;

            // Apply beam spell on them
            for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end(); itr++)
            {
                Unit * player = *itr;

                if (beamPlayersCount-- > 0)
                {
                    uint32 spellId = GetBeamSpellId(me->GetDistance(player));

                    if (!player->HasAura(spellId))
                    {
                        // Cast appropriate beam
                        me->CastSpell(player, spellId, true);

                        // Drop all beam auras, except casted one
                        if (player->HasAura(SAFE_BEAM) && SAFE_BEAM != spellId)
                            player->RemoveAura(SAFE_BEAM);

                        if (player->HasAura(WARNING_BEAM) && WARNING_BEAM != spellId)
                            player->RemoveAura(WARNING_BEAM);

                        if (player->HasAura(DANGER_BEAM) && DANGER_BEAM != spellId)
                            player->RemoveAura(DANGER_BEAM);
                    }
                }
                else
                {
                    player->RemoveAurasDueToSpell(SAFE_BEAM);
                    player->RemoveAurasDueToSpell(WARNING_BEAM);
                    player->RemoveAurasDueToSpell(DANGER_BEAM);
                }
            }
        }

        void Register() override
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_target_selection_SpellScript::HandleSelection, EFFECT_1, TARGET_UNIT_AREA_ENEMY_DST);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_ds_target_selection_SpellScript();
    }
};

class spell_ds_summon_kohcrom : public SpellScriptLoader
{
public:
    spell_ds_summon_kohcrom() : SpellScriptLoader("spell_ds_summon_kohcrom") { }

    class spell_ds_summon_kohcrom_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_summon_kohcrom_SpellScript);

        void HandleKohcromSummon(SpellEffIndex effIndex)
        {
            // Need special handling, so turn of default spawning handling
            PreventHitDefaultEffect(effIndex);

            Unit * caster = GetCaster();

            #define ANGLE_JUMP_OFFSET   (M_PI / 4)
            #define JUMP_DISTANCE       (45.0f)

            float o = caster->GetOrientation();

            Position morchokPos;
            float morchokAngle = o + ANGLE_JUMP_OFFSET;

            caster->GetNearPosition(morchokPos, JUMP_DISTANCE, MapManager::NormalizeOrientation(morchokAngle));

            if (Creature * pKohcrom = caster->SummonCreature(KOHCROM_ENTRY, caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ(), caster->GetOrientation()))
            {
                pKohcrom->SetMaxHealth(caster->GetMaxHealth());
                pKohcrom->SetHealth(caster->GetHealth());

                Position kohcromPos;
                float kohcromAngle = o - ANGLE_JUMP_OFFSET;
                caster->GetNearPosition(kohcromPos, JUMP_DISTANCE, MapManager::NormalizeOrientation(kohcromAngle));

                pKohcrom->AttackStop();
                pKohcrom->GetMotionMaster()->MoveJump(kohcromPos.GetPositionX(), kohcromPos.GetPositionY(), kohcromPos.GetPositionZ(), 15.0f, 10.0f);
            }

            caster->AttackStop();
            caster->GetMotionMaster()->MoveJump(morchokPos.GetPositionX(), morchokPos.GetPositionY(), morchokPos.GetPositionZ(), 15.0f, 10.0f);
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_ds_summon_kohcrom_SpellScript::HandleKohcromSummon, EFFECT_0, SPELL_EFFECT_SUMMON);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_ds_summon_kohcrom_SpellScript();
    }
};

class spell_ds_earthen_vortex : public SpellScriptLoader
{
public:
    spell_ds_earthen_vortex() : SpellScriptLoader("spell_ds_earthen_vortex") { }

    class spell_ds_earthen_vortex_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_earthen_vortex_SpellScript);

        void HandleTeleport(SpellEffIndex effIndex)
        {
            PreventHitEffect(effIndex); // Teleport is bullshit, be honest blizz :)
        }

        void HandleExtraVehicleMove(std::list<Unit*>& unitList)
        {
            // In this time, player does not have vehicle pointer set yet
            // Handled in npc_ds_earthen_vortex AI -> HandleVehicleMove()
        }

        void Register() override
        {
            OnEffect += SpellEffectFn(spell_ds_earthen_vortex_SpellScript::HandleTeleport, EFFECT_1, SPELL_EFFECT_TELEPORT_UNITS_FACE_CASTER);
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_earthen_vortex_SpellScript::HandleExtraVehicleMove, EFFECT_2, TARGET_UNIT_AREA_ENEMY_DST);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_ds_earthen_vortex_SpellScript();
    }
};

void AddSC_boss_morchok()
{
    new boss_morchok_dragon_soul();     // 55265
    new boss_kohcrom_dragon_soul();     // 57773
    new npc_ds_resonating_crystal();    // 55346
    new npc_ds_earthen_vortex();        // 55723

    new spell_ds_summon_fragments();    // 103176
    new spell_ds_summon_black_blood();  // 103851
    new spell_ds_black_blood_damage();  // 103785, 108570, 110288, 110287
    new spell_ds_resonating_blast();    // 103545, 108572, 110041, 110040
    new spell_ds_morchok_stomp();       // 103414, 108571, 109033, 109034
    new spell_ds_target_selection();    // 103528, 104573
    new spell_ds_summon_kohcrom();      // 109017
    new spell_ds_earthen_vortex();      // 103821, 110047, 110046, 110045
}

/*
    UPDATE `creature_template` SET `ScriptName`='boss_morchok_dragon_soul' WHERE  `entry`=55265 LIMIT 1;
    UPDATE `creature_template` SET `ScriptName`='boss_kohcrom_dragon_soul' WHERE  `entry`=57773 LIMIT 1;
    UPDATE `creature_template` SET `ScriptName`='npc_ds_resonating_crystal' WHERE  `entry`=55346 LIMIT 1;
    UPDATE `creature_template` SET `ScriptName`='npc_ds_earthen_vortex' WHERE  `entry`=55723 LIMIT 1;

    -- fuck blizzard hidden vehicleId
    UPDATE `creature_template` SET `VehicleId`=1896 WHERE  `entry`=55723;

    REPLACE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (103176, 'spell_ds_summon_fragments');

    REPLACE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (103851, 'spell_ds_summon_black_blood');

    REPLACE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (103785, 'spell_ds_black_blood_damage'),
    (108570, 'spell_ds_black_blood_damage'),
    (110288, 'spell_ds_black_blood_damage'),
    (110287, 'spell_ds_black_blood_damage');

    REPLACE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (103545, 'spell_ds_resonating_blast'),
    (108572, 'spell_ds_resonating_blast'),
    (110041, 'spell_ds_resonating_blast'),
    (110040, 'spell_ds_resonating_blast');

    REPLACE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (103414, 'spell_ds_morchok_stomp'),
    (108571, 'spell_ds_morchok_stomp'),
    (109033, 'spell_ds_morchok_stomp'),
    (109034, 'spell_ds_morchok_stomp');

    REPLACE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (103528, 'spell_ds_target_selection'),
    (104573, 'spell_ds_target_selection');

    REPLACE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (109017, 'spell_ds_summon_kohcrom');

    REPLACE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (103821, 'spell_ds_earthen_vortex'),
    (110047, 'spell_ds_earthen_vortex'),
    (110046, 'spell_ds_earthen_vortex'),
    (110045, 'spell_ds_earthen_vortex');

*/