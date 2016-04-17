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

static const FacelessQuote introQuote =
{
    26328,
    "Ak'agthshi ma uhnish, ak'uq shg'cul vwahuhn! H'iwn iggksh Phquathi gag OOU KAAXTH SHUUL!",
    "Our numbers are endless, our power beyond reckoning! All who oppose the Destroyer will DIE A THOUSAND DEATHS!"
};

static const FacelessQuote aggroQuote =
{
    26326,
    "Iilth qi'uothk shn'ma yeh'glu Shath'Yar! H'IWN IILTH!",
    "You will drown in the blood of the Old Gods! ALL OF YOU!"
};

static const FacelessQuote deathQuote =
{
    26327,
    "Ez, Shuul'wah! Sk'woth'gl yu'gaz yog'ghyl ilfah!",
    "O, Deathwing! Your faithful servant has failed you!"
};

#define MAX_BLOOD_QUOTES 2

static const FacelessQuote bloodQuotes[MAX_BLOOD_QUOTES] =
{
    { 26332, "KYTH ag'xig yyg'far IIQAATH ONGG!", "SEE how we pour from the CURSED EARTH!" },
    { 26333, "UULL lwhuk H'IWN!", "The DARKNESS devours ALL!" },
};

#define MAX_KILL_QUOTES 3

static const FacelessQuote killQuotes[MAX_KILL_QUOTES] =
{
    { 26329, "Sk'yahf qi'plahf PH'MAGG!", "Your soul will know ENDLESS TORMENT!" },
    { 26330, "H'iwn zaix Shuul'wah, PHQUATHI!", "All praise Deathwing, THE DESTROYER!" },
    { 26331, "Shkul an'zig qvsakf KSSH'GA, ag'THYZAK agthu!", "From its BLEAKEST DEPTHS, we RECLAIM this world!" },
};

enum whisperSpells : uint32
{
    SPELL_WHISPER_INTRO             = 109894,
    SPELL_WHISPER_AGGRO             = 109895,
    SPELL_WHISPER_DEATH             = 109896,
    SPELL_WHISPER_SLAY              = 109897,
    SPELL_WHISPER_MINIONS           = 109898,
    SPELL_WHISPER_CORRUPTION        = 109899
};

enum bloodAuraSpells
{
    SPELL_BLACK_BLOOD_OF_SHUMA      = 104894,
    SPELL_SHADOWED_BLOOD_OF_SHUMA   = 104896,
    SPELL_CRIMSON_BLOOD_OF_SHUMA    = 104897,
    SPELL_ACIDIC_BLOOD_OF_SHUMA     = 104898,
    SPELL_COBALT_BLOOD_OF_SHUMA     = 105027,
    SPELL_GLOWING_BLOOD_OF_SHUMA    = 104901
};

enum spells
{
    SPELL_VOID_BOLT                 = 104849,
    SPELL_VOID_BOLT_AOE             = 105416,
    SPELL_CHANNEL_ANIM_KIT          = 89176,
    SPELL_BERSERK                   = 26662,

    SPELL_COSMETIC_ROAR             = 105533,

    // Green phase
    SPELL_DIGESTIVE_ACID_VISUAL     = 105562,
    SPELL_DIGESTIVE_ACID_DUMMY      = 105571, // Used as selector aoe for players to cast SPELL_DIGESTIVE_ACID_MISSILE on ?
    SPELL_DIGESTIVE_ACID_MISSILE    = 105573,

    // Dark phase
    SPELL_SPAWNING_POOL_VISUAL_1    = 105600,
    SPELL_SPAWNING_POOL_VISUAL_2    = 105601,
    SPELL_SPAWNING_POOL_VISUAL_3    = 105603,
    SPELL_PSYCHIC_SLICE             = 105671,
    SPELL_FIXATE                    = 105695, // aoe SPELL_AURA_MOD_POSSESS_PET -> wtf blizz

    // Purple phase
    SPELL_DEEP_CORRUPTION_PROC_AURA = 105171,
    SPELL_DDEP_CORRUPTION_STACK     = 103628,
    SPELL_DEEP_CORRUPTION_DAMAGE    = 105173,

    // Red phase
    SPELL_SEARING_BLOOD             = 105033, // AoE

    // Blue phase
    SPELL_MANA_VOID_VISUAL_BUBBLE   = 105527,

    SPELL_MANA_VOID_SUMMON          = 105034, // Caster Aura Spell (105027) Cobalt Blood of Shu'ma
    SPELL_MANA_VOID_DUMMY_CASTING   = 105505, // 3s dummy aura trggering casting of mana void channel anim
    SPELL_MANA_VOID_MANA_LEACH      = 108222, // + force cast of SPELL_MANA_VOID_LEACH_VISUAL to affected players
    SPELL_MANA_LEACH_PERIODIC       = 105530,
    SPELL_MANA_VOID_LEACH_VISUAL    = 105534, // TARGET_UNIT_NEARBY_ENTRY

    SPELL_MANA_DIFFUSION_PCT        = 108228,
    SPELL_MANA_DIFFUSION_LFR        = 110742,
    SPELL_MANA_DIFFUSION_FLAT       = 105539,

    ACHIEVEMENT_HEROIC_YORSAHJ      = 6111,
    ACHIEVEMENT_TASTE_THE_RAINBOW   = 6129,
};

enum colorCombinationsNormal : uint32
{
    SPELL_COMB_PURPLE_GREEN_BLUE    = 105420,
    SPELL_COMB_GREEN_RED_BLACK      = 105435,
    SPELL_COMB_GREEN_YELLOW_RED     = 105436,
    SPELL_COMB_PURPLE_BLUE_YELLOW   = 105437,
    SPELL_COMB_BLUE_BLACK_YELLLOW   = 105439,
    SPELL_COMB_PURPLE_RED_BLACK     = 105440
};

enum colorCombinationsHeroic : uint32
{
    SPELL_COMB_PURPLE_GREEN_BLUE_BLACK      = 105420,
    SPELL_COMB_GREEN_RED_BLACK_BLUE         = 105435,
    SPELL_COMB_GREEN_YELLOW_RED_BLACK       = 105436,
    SPELL_COMB_PURPLE_BLUE_YELLOW_GREEN     = 105437,
    SPELL_COMB_BLUE_BLACK_YELLLOW_PURPLE    = 105439,
    SPELL_COMB_PURPLE_RED_BLACK_YELLOW      = 105440
};

enum colorBeamsAuras : uint32
{
    AURA_TALL_BLUE                  = 105473,
    AURA_TALL_RED                   = 105474,
    AURA_TALL_GREEN                 = 105475,
    AURA_TALL_YELLOW                = 105476,
    AURA_TALL_PURPLE                = 105477,
    AURA_TALL_BLACK                 = 105478
};

enum globuleBloodAuras
{
    SPELL_BLACK_BLOOD_OF_SHUMA_UNLIMITED    = 110746,
    SPELL_SHADOWED_BLOOD_OF_SHUMA_UNLIMITED = 110748,
    SPELL_CRIMSON_BLOOD_OF_SHUMA_UNLIMITED  = 110750,
    SPELL_ACIDIC_BLOOD_OF_SHUMA_UNLIMITED   = 110743,
    SPELL_COBALT_BLOOD_OF_SHUMA_UNLIMITED   = 110747,
    SPELL_GLOWING_BLOOD_OF_SHUMA_UNLIMITED  = 110753
};

enum globuleSpells : uint32
{
    SPELL_FUSING_VAPOR_HEAL_PCT     = 108233, // (TARGET_UNIT_SRC_AREA_ENTRY) Emits a fusing vapor when damaged below 50% health that heals all other active globules for 5% of their maximum life.
    SPELL_COMPLETELY_FUSED          = 105904, // make all glubules imprevious to damage (SPELL_AURA_SCHOOL_IMMUNITY) + removes 103968 -> proc aura whic triggers 103635
    SPELL_FUSING_VAPORS_PROC_AURA   = 103968,
    SPELL_BESTOW_BLOOD              = 103969 // transfom caster to invisible form and cast scripted effect to target
};

enum entries : uint32
{
     YORSAH_ENTRY                   = 55312,
     MANA_VOID_ENTRY                = 56231,
     FORGOTTEN_ONE_ENTRY            = 56265
};

enum globuleEntry : uint32
{
    ACID_GLOBULE_ENTRY              = 55862, // green
    SHADOWED_GLOBULE_ENTRY          = 55863, // purple
    GLOWING_GLOBULE_ENTRY           = 55864, // yellow
    CRIMSON_GLOBULE_ENTRY           = 55865, // red
    COBALT_GLOBULE_ENTRY            = 55866, // blue
    DARK_GLOBULE_ENTRY              = 55867  // black
};

enum waypointId : uint32
{
    WP_YORSAHJ_POS                  = 0,
    WP_MIDDDLE_POS                  = 1
};

enum datas : uint32
{
    DATA_MANA_LEACHED               = 0
};

enum actions : int32
{
    ACTION_GLOBULE_FRAME_ENGAGED    = 0
};

#define MAX_GLOBULES_NORMAL             3
#define MAX_GLOBULES_HEROIC             4
#define MAX_SPELL_COMBINATIONS          6

struct GlobuleData
{
    Position pos;
    uint32 globuleEntry;
};

static const GlobuleData greenGlobePos     = { {-1721.0f, -2928.0f, -172.0f, 4.27f},    ACID_GLOBULE_ENTRY };
static const GlobuleData blueGlobePos      = { {-1721.0f, -3140.0f, -172.0f, 2.00f},    COBALT_GLOBULE_ENTRY };
static const GlobuleData purpleGlobePos    = { {-1660.0f, -3078.0f, -172.0f, 2.70f},    SHADOWED_GLOBULE_ENTRY };
static const GlobuleData yellowGlobePos    = { {-1870.0f, -2990.0f, -172.0f, 5.87f},    GLOWING_GLOBULE_ENTRY };
static const GlobuleData blackGlobePos     = { {-1809.0f, -3141.0f, -172.0f, 1.15f},    DARK_GLOBULE_ENTRY };
static const GlobuleData redGlobePos       = { {-1659.0f, -2990.0f, -172.0f, 3.55f},    CRIMSON_GLOBULE_ENTRY };

struct SpellCombinationNormal
{
    const uint32 spellId;
    const GlobuleData globules[MAX_GLOBULES_NORMAL];
};

struct SpellCombinationHeroic
{
    const uint32 spellId;
    const GlobuleData globules[MAX_GLOBULES_HEROIC];
};

static SpellCombinationNormal spellCombinationsNormal[MAX_SPELL_COMBINATIONS] =
{
    {   SPELL_COMB_PURPLE_GREEN_BLUE,   {purpleGlobePos,    greenGlobePos,  blueGlobePos    }},
    {   SPELL_COMB_GREEN_RED_BLACK,     {greenGlobePos,     redGlobePos,    blackGlobePos   }},
    {   SPELL_COMB_GREEN_YELLOW_RED,    {greenGlobePos,     yellowGlobePos, redGlobePos     }},
    {   SPELL_COMB_PURPLE_BLUE_YELLOW,  {purpleGlobePos,    blueGlobePos,   yellowGlobePos  }},
    {   SPELL_COMB_BLUE_BLACK_YELLLOW,  {blueGlobePos,      blackGlobePos,  yellowGlobePos  }},
    {   SPELL_COMB_PURPLE_RED_BLACK,    {purpleGlobePos,    redGlobePos,    blackGlobePos   }},
};

static SpellCombinationHeroic spellCombinationsHeroic[MAX_SPELL_COMBINATIONS] =
{
    { SPELL_COMB_PURPLE_GREEN_BLUE_BLACK,   { purpleGlobePos,    greenGlobePos,  blueGlobePos,      blackGlobePos   } },
    { SPELL_COMB_GREEN_RED_BLACK_BLUE,      { greenGlobePos,     redGlobePos,    blackGlobePos,     blueGlobePos    } },
    { SPELL_COMB_GREEN_YELLOW_RED_BLACK,    { greenGlobePos,     yellowGlobePos, redGlobePos,       blackGlobePos   } },
    { SPELL_COMB_PURPLE_BLUE_YELLOW_GREEN,  { purpleGlobePos,    blueGlobePos,   yellowGlobePos,    greenGlobePos   } },
    { SPELL_COMB_BLUE_BLACK_YELLLOW_PURPLE, { blueGlobePos,      blackGlobePos,  yellowGlobePos,    purpleGlobePos  } },
    { SPELL_COMB_PURPLE_RED_BLACK_YELLOW,   { purpleGlobePos,    redGlobePos,    blackGlobePos,     yellowGlobePos  } },
};

static const Position MIDDLE_POSITION = { -1763.7f, -3032.5f, -182.40f, 0.0f };

class boss_yorsahj_the_unsleeping : public CreatureScript
{
public:
    boss_yorsahj_the_unsleeping() : CreatureScript("boss_yorsahj_the_unsleeping") { }

    CreatureAI* GetAI(Creature* creature) const  override
    {
        return new boss_yorsahj_the_unsleepingAI(creature);
    }

    struct boss_yorsahj_the_unsleepingAI : public ScriptedAI
    {
        boss_yorsahj_the_unsleepingAI(Creature* creature) : ScriptedAI(creature), summons(me)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        SummonList summons;
        TaskScheduler scheduler;

        uint32 oozeTimer;
        uint32 voidBoltTimer;

        uint32 globulesActive;
        uint32 quoteIndex;

        bool introDone = false;
        bool bestowedBlood = false;

        inline bool IsCasting()
        {
            return me->IsNonMeleeSpellCasted(false) || me->HasAura(SPELL_CHANNEL_ANIM_KIT) || me->HasAura(SPELL_MANA_VOID_DUMMY_CASTING);
        }

        void Reset() override
        {
            if (instance)
            {
                if (instance->GetData(TYPE_BOSS_YORSAHJ) != DONE)
                    instance->SetData(TYPE_BOSS_YORSAHJ, NOT_STARTED);
            }

            me->SetFloatValue(UNIT_FIELD_COMBATREACH, 10.0f);
            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveAura(SPELL_CHANNEL_ANIM_KIT);

            quoteIndex = 0;

            summons.DespawnAll();
            scheduler.CancelAll();
        }

        void EnterCombat(Unit * who) override
        {
            globulesActive = 0;
            oozeTimer = 22000;
            voidBoltTimer = 6000;
            bestowedBlood = false;

            if (instance)
            {
                instance->SetData(TYPE_BOSS_YORSAHJ, IN_PROGRESS);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            }

            // Enrage after 10 minutes
            scheduler.Schedule(Minutes(10), [this](TaskContext)
            {
                me->CastSpell(me, SPELL_BERSERK, true);
            });

            RunPlayableQuote(aggroQuote);
            me->CastSpell(me, SPELL_WHISPER_AGGRO, true);

            ScriptedAI::EnterCombat(who);
        }

        void JustSummoned(Creature *pSummon) override
        {
            summons.Summon(pSummon);
        }

        void SummonedCreatureDespawn(Creature *pSummon) override
        {
            switch (pSummon->GetEntry())
            {
                case ACID_GLOBULE_ENTRY:
                case SHADOWED_GLOBULE_ENTRY:
                case GLOWING_GLOBULE_ENTRY:
                case CRIMSON_GLOBULE_ENTRY:
                case COBALT_GLOBULE_ENTRY:
                case DARK_GLOBULE_ENTRY:
                {
                    if (instance)
                    {
                        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, pSummon);
                    }
                    break;
                }
                default:
                    break;
            }
            summons.Despawn(pSummon);
        }

        void MoveInLineOfSight(Unit *who) override
        {
            if (!introDone && who->GetTypeId() == TYPEID_PLAYER && !who->ToPlayer()->IsGameMaster() && who->GetExactDist2d(me) < 60.0f)
            {
                RunPlayableQuote(introQuote);
                me->CastSpell(me, SPELL_WHISPER_INTRO, true);

                introDone = true;
                me->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
            }

            if (who->GetExactDist2d(me) < 15.0f)
                ScriptedAI::MoveInLineOfSight(who);
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetTypeId() != TYPEID_PLAYER)
                return;

            quoteIndex = urand(0, MAX_KILL_QUOTES - 1);
            RunPlayableQuote(killQuotes[quoteIndex]);
            me->CastSpell(me, SPELL_WHISPER_SLAY, true);
        }

        void JustDied(Unit *) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_YORSAHJ, DONE);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

                if (IsHeroic())
                    instance->DoCompleteAchievement(ACHIEVEMENT_HEROIC_YORSAHJ);

                instance->DoModifyPlayerCurrencies(CURRENCY_MOTE_OF_DARKNESS, 1, CURRENCY_SOURCE_OTHER);
            }

            summons.DespawnAll();
            scheduler.CancelAll();

            RunPlayableQuote(deathQuote);
            me->CastSpell(me, SPELL_WHISPER_DEATH, true);
            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_FIXATE);
        }

        void EnterEvadeMode() override
        {
            if (instance)
            {
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }

            scheduler.CancelAll();
            summons.DespawnAll();
            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_FIXATE);

            ScriptedAI::EnterEvadeMode();
        }

        void SpellHitTarget(Unit *target, const SpellEntry *spell) override
        {
            switch (spell->Id)
            {
                case SPELL_DIGESTIVE_ACID_DUMMY:
                    me->CastSpell(target, SPELL_DIGESTIVE_ACID_MISSILE, true);
                    break;
                case SPELL_WHISPER_INTRO:
                    me->MonsterWhisper(introQuote.GetWhisperText(), target->GetGUID());
                    break;
                case SPELL_WHISPER_AGGRO:
                    me->MonsterWhisper(aggroQuote.GetWhisperText(), target->GetGUID());
                    break;
                case SPELL_WHISPER_DEATH:
                    me->MonsterWhisper(deathQuote.GetWhisperText(), target->GetGUID());
                    break;
                case SPELL_WHISPER_SLAY:
                    if (quoteIndex < MAX_KILL_QUOTES)
                        me->MonsterWhisper(killQuotes[quoteIndex].GetWhisperText(), target->GetGUID());
                    break;
                case SPELL_WHISPER_MINIONS:
                    if (quoteIndex < MAX_BLOOD_QUOTES)
                        me->MonsterWhisper(bloodQuotes[quoteIndex].GetWhisperText(), target->GetGUID());
                    break;
                case SPELL_WHISPER_CORRUPTION:
                    // Not used at all ?
                    break;
                default:
                    break;
            }
        }

        void SpellHit(Unit* caster, const SpellEntry* spell) override
        {
            if (spell->Id == SPELL_BESTOW_BLOOD)
            {
                // Count only first blood consumed
                if (!bestowedBlood)
                {
                    bestowedBlood = true;
                    voidBoltTimer = 4000;
                    me->RemoveAura(SPELL_CHANNEL_ANIM_KIT);
                    me->CastSpell(me, SPELL_COSMETIC_ROAR, true);

                    // Wait few 2-4 seconds to play roaring anim, then continue attacking
                    scheduler.Schedule(Seconds(2), Seconds(4), [this](TaskContext)
                    {
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->SetInCombatWithZone();
                        me->Attack(me->GetVictim(), true);
                        me->GetMotionMaster()->MoveChase(me->GetVictim());
                    });
                }

                switch (caster->GetEntry())
                {
                    case CRIMSON_GLOBULE_ENTRY:
                    {
                        // Cast searing blood every 6 seconds
                        scheduler.Schedule(Seconds(6), [this](TaskContext ctx)
                        {
                            me->CastCustomSpell(SPELL_SEARING_BLOOD, SPELLVALUE_MAX_TARGETS, Is25ManRaid() ? 8 : 3, me, true);

                            if (me->HasAura(SPELL_CRIMSON_BLOOD_OF_SHUMA))
                                ctx.Repeat();
                        });
                        break;
                    }
                    case COBALT_GLOBULE_ENTRY:
                    {
                        // Spawn Mana Void after 6 seconds
                        scheduler.Schedule(Seconds(3), [this](TaskContext)
                        {
                            me->CastSpell(me, SPELL_MANA_VOID_SUMMON, true);
                        });

                        break;
                    }
                    case DARK_GLOBULE_ENTRY:
                    {
                        me->CastSpell(me, SPELL_SPAWNING_POOL_VISUAL_1, true);
                        me->CastSpell(me, SPELL_SPAWNING_POOL_VISUAL_2, true);
                        me->CastSpell(me, SPELL_SPAWNING_POOL_VISUAL_3, true);

                        // Spawn Forgotten ones
                        scheduler.Schedule(Seconds(6), [this](TaskContext ctx)
                        {
                            // Max 10 forgottens in 25 man -> 5 in 10 man
                            if (ctx.GetRepeatCounter() == (Is25ManRaid() ? 10 : 5))
                                return;

                            float x, y, z;

                            me->GetNearPoint(me, x, y, z, 1.0f, 12.0f, frand(0.0f, 2.0f * M_PI));
                            me->SummonCreature(FORGOTTEN_ONE_ENTRY, x, y, z, MapManager::NormalizeOrientation(me->GetAngle(x, y) + M_PI), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000);

                            ctx.Repeat(Seconds(1));
                        });

                        break;
                    }
                    case ACID_GLOBULE_ENTRY:
                    {
                        me->CastSpell(me, SPELL_DIGESTIVE_ACID_VISUAL, true);

                        // Cast digestive acid every 10 seconds
                        scheduler.Schedule(Seconds(10), [this](TaskContext ctx)
                        {
                            me->CastCustomSpell(SPELL_DIGESTIVE_ACID_DUMMY, SPELLVALUE_MAX_TARGETS, Is25ManRaid() ? 12 : 5, me, true);

                            if (me->HasAura(SPELL_ACIDIC_BLOOD_OF_SHUMA))
                                ctx.Repeat();
                        });
                        break;
                    }
                    case GLOWING_GLOBULE_ENTRY:
                    {
                        // handled in voidBoltTimer
                        break;
                    }
                    case SHADOWED_GLOBULE_ENTRY:
                    {
                        scheduler.Schedule(Seconds(6), [this](TaskContext ctx)
                        {
                            if (instance)
                                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_DDEP_CORRUPTION_STACK);

                            me->CastSpell(me, SPELL_DEEP_CORRUPTION_PROC_AURA, true);

                            if (ctx.GetRepeatCounter() == 0)
                            {
                                ctx.Repeat(Seconds(26));
                            }
                        });
                        break;
                    }
                }
            }
        }

        void DoAction(const int32 action) override
        {
            if (action == ACTION_GLOBULE_FRAME_ENGAGED)
            {
                if (++globulesActive == (IsHeroic() ? MAX_GLOBULES_HEROIC : MAX_GLOBULES_NORMAL))
                {
                    /* 
                        Max encounter frame number which client can display is 4.
                        In heroic there are 4 blood globules so Yor'sahj encounter frame should be hidden while there are all 4 active
                    */
                    if (instance)
                    {
                        if (IsHeroic())
                        {
                            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                        }
                        else
                        {
                            // Update Yor'sahj encounter frame on normal to be at first position
                            instance->SendEncounterUnit(ENCOUNTER_FRAME_UPDATE_PRIORITY, me);
                            instance->SendEncounterUnit(ENCOUNTER_FRAME_UPDATE_PRIORITY, me);
                        }
                    }
                }
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type == POINT_MOTION_TYPE && id == WP_MIDDDLE_POS)
            {
                me->GetMotionMaster()->MoveIdle(MOTION_SLOT_IDLE);

                // Pick random color combination
                uint32 ranColorCombination = urand(0, MAX_SPELL_COMBINATIONS - 1);
                // Spawn globules by color combination
                SpawnBloodGlobules(ranColorCombination);

                // Cast appropriate combination spell (normal / heroic spellIds are shared)
                me->CastSpell(me, spellCombinationsNormal[ranColorCombination].spellId, false);
                me->CastSpell(me, SPELL_CHANNEL_ANIM_KIT, true);

                // Yell and whisper
                quoteIndex = urand(0, MAX_BLOOD_QUOTES - 1);
                RunPlayableQuote(bloodQuotes[quoteIndex]);

                me->CastSpell(me, SPELL_WHISPER_MINIONS, true);
            }
        }

        void SpawnBloodGlobules(uint32 colorCombination)
        {
            // Spawn globules by color combination
            if (IsHeroic())
            {
                for (uint32 i = 0; i < MAX_GLOBULES_HEROIC; i++)
                {
                    uint32 entry = spellCombinationsHeroic[colorCombination].globules[i].globuleEntry;
                    Position pos = spellCombinationsHeroic[colorCombination].globules[i].pos;
                    me->SummonCreature(entry, pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000);
                }
            }
            else
            {
                for (uint32 i = 0; i < MAX_GLOBULES_NORMAL; i++)
                {
                    uint32 entry = spellCombinationsNormal[colorCombination].globules[i].globuleEntry;
                    Position pos = spellCombinationsNormal[colorCombination].globules[i].pos;
                    me->SummonCreature(entry, pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000);
                }
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            scheduler.Update(diff);

            if (voidBoltTimer <= diff)
            {
                if (!IsCasting())
                {
                    voidBoltTimer = urand(5000,6000);

                    if (me->HasAura(SPELL_GLOWING_BLOOD_OF_SHUMA))
                    {
                        me->CastSpell(me, SPELL_VOID_BOLT_AOE, true);
                        voidBoltTimer /= 2;
                    }
                    else
                        me->CastSpell(me->GetVictim(), SPELL_VOID_BOLT, true);
                }
            }
            else voidBoltTimer -= diff;

            if (oozeTimer <= diff)
            {
                if (!IsCasting())
                {
                    // Reset bestowing
                    bestowedBlood = false;

                    // Stop attacking
                    me->SetReactState(REACT_PASSIVE);
                    me->AttackStop();
                    // And go to middle position, casting is handled at the arrival to WP
                    me->GetMotionMaster()->MovePoint(WP_MIDDDLE_POS, MIDDLE_POSITION, true, false);
                    oozeTimer = 90000;
                }
            }
            else oozeTimer -= diff;

            if (!IsCasting())
                DoMeleeAttackIfReady();
        }
    };
};

class npc_yorsahj_blood_globule : public CreatureScript
{
public:
    npc_yorsahj_blood_globule() : CreatureScript("npc_yorsahj_blood_globule") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_yorsahj_blood_globuleAI(creature);
    }

    typedef struct globuleInfo
    {
        public:
        uint32 beamAura = 0;
        uint32 empoweringAura = 0;
        uint32 passiveAura = 0;

        void InitGlobuleInfo(uint32 entry)
        {
            switch (entry)
            {
                case CRIMSON_GLOBULE_ENTRY:
                    Init(SPELL_CRIMSON_BLOOD_OF_SHUMA_UNLIMITED, AURA_TALL_RED, SPELL_CRIMSON_BLOOD_OF_SHUMA);
                    break;
                case COBALT_GLOBULE_ENTRY:
                    Init(SPELL_COBALT_BLOOD_OF_SHUMA_UNLIMITED, AURA_TALL_BLUE, SPELL_COBALT_BLOOD_OF_SHUMA);
                    break;
                case GLOWING_GLOBULE_ENTRY:
                    Init(SPELL_GLOWING_BLOOD_OF_SHUMA_UNLIMITED, AURA_TALL_YELLOW, SPELL_GLOWING_BLOOD_OF_SHUMA);
                    break;
                case DARK_GLOBULE_ENTRY:
                    Init(SPELL_BLACK_BLOOD_OF_SHUMA_UNLIMITED, AURA_TALL_BLACK, SPELL_BLACK_BLOOD_OF_SHUMA);
                    break;
                case ACID_GLOBULE_ENTRY:
                    Init(SPELL_ACIDIC_BLOOD_OF_SHUMA_UNLIMITED, AURA_TALL_GREEN, SPELL_ACIDIC_BLOOD_OF_SHUMA);
                    break;
                case SHADOWED_GLOBULE_ENTRY:
                    Init(SPELL_SHADOWED_BLOOD_OF_SHUMA_UNLIMITED, AURA_TALL_PURPLE, SPELL_SHADOWED_BLOOD_OF_SHUMA);
                    break;
                default:
                    break;
            }
        }

        private:
        void Init(uint32 passiveAura, uint32 beamAura, uint32 empoweringAura)
        {
            this->passiveAura = passiveAura;
            this->beamAura = beamAura;
            this->empoweringAura = empoweringAura;
        }

    } GLOBULE_INFO;

    struct npc_yorsahj_blood_globuleAI : public ScriptedAI
    {
        npc_yorsahj_blood_globuleAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            globuleInfo.InitGlobuleInfo(creature->GetEntry());
        }

        InstanceScript* instance;
        GLOBULE_INFO globuleInfo;

        uint32 updateMovePointTimer;

        void Reset() override
        {
            me->CastSpell(me, SPELL_FUSING_VAPORS_PROC_AURA, true);
            me->CastSpell(me, globuleInfo.beamAura, true);
            me->CastSpell(me, globuleInfo.passiveAura, true);

            me->SetReactState(REACT_PASSIVE);
            me->SetSpeed(MOVE_RUN, 0.695f, true);
            updateMovePointTimer = 10000;
        }

        void JustDied(Unit *) override
        {
            // Make other globules immmune to any damage
            me->CastSpell(me, SPELL_COMPLETELY_FUSED, true);

            if (instance)
            {
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

                if (IsHeroic())
                {
                    if (Creature * pBoss = GetSummoner<Creature>())
                        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, pBoss);
                }
            }
        }

        void SpellHit(Unit* caster, const SpellEntry* spell) override
        {
            if (SPELL_COMB_PURPLE_GREEN_BLUE    == spell->Id ||
                SPELL_COMB_GREEN_RED_BLACK      == spell->Id ||
                SPELL_COMB_GREEN_YELLOW_RED     == spell->Id ||
                SPELL_COMB_PURPLE_BLUE_YELLOW   == spell->Id ||
                SPELL_COMB_BLUE_BLACK_YELLLOW   == spell->Id ||
                SPELL_COMB_PURPLE_RED_BLACK     == spell->Id)
            {
                UpdateMovePoint();

                if (instance)
                {
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_UPDATE_PRIORITY, me);

                    if (Creature * pBoss = GetSummoner<Creature>())
                    {
                        pBoss->AI()->DoAction(ACTION_GLOBULE_FRAME_ENGAGED);
                    }
                }

                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            }
        }

        void UpdateMovePoint()
        {
            if (Creature * pBoss = GetSummoner<Creature>())
                me->GetMotionMaster()->MovePoint(WP_YORSAHJ_POS, pBoss->GetPositionX(), pBoss->GetPositionY(), pBoss->GetPositionZ(), false, false);

            updateMovePointTimer = 500;
        }

        bool BossReached()
        {
            Creature * pBoss = GetSummoner<Creature>();

            if (!pBoss)
                return false;

            return (me->GetExactDist2d(pBoss) <= 3.0f) ? true : false;
        }

        void EnterCombat(Unit * ) override {}
        void MoveInLineOfSight(Unit * ) override {}
        void EnterEvadeMode() override {}

        void UpdateAI(const uint32 diff) override
        {
            if (!me->IsSummon())
                return;

            if (updateMovePointTimer <= diff)
            {
                // Check if we arrived to his position
                if (BossReached())
                {
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

                    if (instance)
                    {
                        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                    }

                    if (Creature * pBoss = GetSummoner<Creature>())
                    {
                        // First cast empower aura, cause some spells has CasterAuraSpell condition
                        pBoss->CastSpell(pBoss, globuleInfo.empoweringAura, true);
                        // Bestow blood as second
                        me->CastSpell(pBoss, SPELL_BESTOW_BLOOD, true);
                    }

                    me->ForcedDespawn(1000);
                    updateMovePointTimer = MAX_TIMER;
                    return;
                }

                // Move to current boss position
                UpdateMovePoint();
            }
            else updateMovePointTimer -= diff;
        }
    };
};

class npc_mana_void_DS : public CreatureScript
{
public:
    npc_mana_void_DS() : CreatureScript("npc_mana_void_DS") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_mana_void_DSAI(creature);
    }

    struct npc_mana_void_DSAI : public ScriptedAI
    {
        npc_mana_void_DSAI(Creature* creature) : ScriptedAI(creature) { }

        TaskScheduler scheduler;

        uint32 manaLeached = 0;
        uint32 ticks = 0;

        void Reset() override
        {
            me->CastSpell(me, SPELL_MANA_VOID_VISUAL_BUBBLE, true);
            me->CastSpell(me, SPELL_MANA_LEACH_PERIODIC, true);

            me->SetSpeed(MOVE_FLIGHT, 0.2f, true);
            me->SetReactState(REACT_PASSIVE);

            me->GetMotionMaster()->MoveFall();

            scheduler.Schedule(Seconds(4), [this](TaskContext ctx)
            {
                me->SetFlying(false);
                me->SetSpeed(MOVE_RUN, 0.2f, true);

                float x, y, z;
                me->GetNearPoint(me, x, y, z, 1.0f, 10.0f, frand(0.0f, 2.0f * M_PI));

                me->GetMotionMaster()->MovePoint(0, x, y, z, true, false);
                ctx.Repeat(Seconds(6), Seconds(8));
            });
        }

        void EnterCombat(Unit *) override {}
        void MoveInLineOfSight(Unit *) override {}
        void EnterEvadeMode() override {}
        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);
        }

        void SetData(uint32 type, uint32 data) override
        {
            if (type == DATA_MANA_LEACHED)
            {
                ++ticks;
                manaLeached += data;
            }
        }

        void DamageTaken(Unit* /*pDoneBy*/, uint32 &damage) override
        {
            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
            {
                damage = 0;
                return;
            }

            if (damage > me->GetHealth())
            {
                damage = me->GetHealth() - 1;

                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->ForcedDespawn(1500);

                me->GetMotionMaster()->Clear(false);
                me->GetMotionMaster()->MoveIdle(MOTION_SLOT_IDLE);

                uint32 sharedMana = ticks == 0 ? 100 : manaLeached / (ticks / 4); // 4 ticks from each periodic aura
                int32 bp0 = int32(sharedMana);

                me->CastCustomSpell(me, SPELL_MANA_DIFFUSION_FLAT, &bp0, 0, 0, true);
            }
        }
    };
};

class npc_forgotten_one_DS : public CreatureScript
{
public:
    npc_forgotten_one_DS() : CreatureScript("npc_forgotten_one_DS") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_forgotten_one_DSAI(creature);
    }

    struct npc_forgotten_one_DSAI : public ScriptedAI
    {
        npc_forgotten_one_DSAI(Creature* creature) : ScriptedAI(creature) { }

        TaskScheduler scheduler;
        uint64 fixatePlayerGUID = 0;

        void Reset() override
        {
            // Be passive or few seconds
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NON_ATTACKABLE);

            // Turn aggressive after 3 - 5 seconds
            scheduler.Schedule(Milliseconds(3000), Milliseconds(5000), [this] (TaskContext)
            {
                me->SetReactState(REACT_AGGRESSIVE);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NON_ATTACKABLE);
                me->SetInCombatWithZone();

                if (Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    me->GetMotionMaster()->MoveChase(target);

                me->CastSpell(me, SPELL_FIXATE, true);
            });

            // Cast slicer to fixate target
            scheduler.Schedule(Seconds(7), Seconds(12), [this](TaskContext ctx)
            {
                if (me->GetVictim())
                    me->CastSpell(me->GetVictim(), SPELL_PSYCHIC_SLICE, false);

                ctx.Repeat(Seconds(12));
            });
        }

        void SpellHitTarget(Unit* target, const SpellEntry* spell) override
        {
            if (spell->Id == SPELL_FIXATE)
            {
                fixatePlayerGUID = target->GetGUID();
                me->AddThreat(target, float(me->GetHealth() * 2.0f));
            }
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetTypeId() != TYPEID_PLAYER)
                return;

            if (Creature * pBoss = GetSummoner<Creature>())
                pBoss->AI()->KilledUnit(victim);
        }

        void JustDied(Unit * killer) override
        {
            if (Player * player = ObjectAccessor::GetPlayer(*me, fixatePlayerGUID))
                player->RemoveAura(SPELL_FIXATE);
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            DoMeleeAttackIfReady();
        }
    };
};

class spell_gen_color_combination : public SpellScriptLoader
{
    public:
        spell_gen_color_combination() : SpellScriptLoader("spell_gen_color_combination") { }

        class spell_gen_color_combination_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_color_combination_SpellScript);

            void RemoveInvalidTargets(std::list<Unit*>& unitList)
            {
                unitList.clear();

                for (uint32 entry = ACID_GLOBULE_ENTRY; entry <= DARK_GLOBULE_ENTRY; entry++)
                {
                    if (Creature * cr = GetCaster()->FindNearestCreature(entry, 200.0f, true))
                        unitList.push_back(cr);
                }
            }

            void Register() override
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_color_combination_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_AREA_ENTRY_SRC);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_color_combination_SpellScript();
        }
};

class spell_gen_ooze_selector : public SpellScriptLoader
{
public:
    spell_gen_ooze_selector() : SpellScriptLoader("spell_gen_ooze_selector") { }

    class spell_gen_ooze_selector_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_gen_ooze_selector_SpellScript);

        void SelectBloodGlobules(std::list<Unit*>& unitList)
        {
            unitList.clear();

            for (uint32 entry = ACID_GLOBULE_ENTRY; entry <= DARK_GLOBULE_ENTRY; entry++)
            {
                // Skip caster globule
                if (GetCaster()->GetEntry() == entry)
                    continue;

                if (Creature * cr = GetCaster()->FindNearestCreature(entry, 200.0f, true))
                    unitList.push_back(cr);
            }
        }

        void Register() override
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_ooze_selector_SpellScript::SelectBloodGlobules, EFFECT_0, TARGET_UNIT_AREA_ENTRY_SRC);
            OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_ooze_selector_SpellScript::SelectBloodGlobules, EFFECT_1, TARGET_UNIT_AREA_ENTRY_SRC);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_gen_ooze_selector_SpellScript();
    }
};

class spell_gen_searing_blood : public SpellScriptLoader
{
public:
    spell_gen_searing_blood() : SpellScriptLoader("spell_gen_searing_blood") {}

    class spell_gen_searing_blood_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_gen_searing_blood_SpellScript);

        void HandleDamage(SpellEffIndex effIndex)
        {
            Unit * hitUnit = GetHitUnit();

            float distance = GetCaster()->GetExactDist2d(hitUnit->GetPositionX(), hitUnit->GetPositionY());

            if (distance <= 10.0f)
                return;

           if (const SpellRadiusEntry *radiusEntry = sSpellRadiusStore.LookupEntry(GetSpellInfo()->EffectRadiusIndex[effIndex]))
                SetHitDamage(int32(GetHitDamage() * (1.0f + distance / radiusEntry->RadiusMax)));
        }

        void Register() override
        {
            OnEffect += SpellEffectFn(spell_gen_searing_blood_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_gen_searing_blood_SpellScript();
    }
};

class spell_gen_mana_void_summon : public SpellScriptLoader
{
public:
    spell_gen_mana_void_summon() : SpellScriptLoader("spell_gen_mana_void_summon") { }

    class spell_gen_mana_void_summon_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_gen_mana_void_summon_SpellScript);

        void ChangeSummonPos(SpellEffIndex /*effIndex*/)
        {
            WorldLocation* summonPos = GetTargetDest();
            Unit * caster = GetCaster();

            // Relocate summon position (increase z coord by 20.0f)
            float x, y, z;
            caster->GetNearPoint(caster, x, y, z, 1.0f, 10.0f, frand(0.0f, 2.0f * M_PI));
            summonPos->Relocate(x, y, z + 20.0f);
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_gen_mana_void_summon_SpellScript::ChangeSummonPos, EFFECT_0, SPELL_EFFECT_SUMMON);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_gen_mana_void_summon_SpellScript();
    }
};

class spell_gen_deep_corruption : public SpellScriptLoader
{
public:
    spell_gen_deep_corruption() : SpellScriptLoader("spell_gen_deep_corruption") { }

    class spell_gen_deep_corruption_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_gen_deep_corruption_AuraScript);

        void HandleDummyTick(AuraEffect const* aurEff)
        {
            if (Unit* target = GetTarget())
            {
                if (!target->HasAura(SPELL_DEEP_CORRUPTION_PROC_AURA)) // Remove proc stack aura if proc aura faded
                {
                    aurEff->GetBase()->Remove();
                    return;
                }
            }
        }

        void OnApply(AuraEffect const* aurEff, AuraEffectHandleModes)
        {
            if (Unit* target = GetTarget())
            {
                if (aurEff->GetBase()->GetStackAmount() == aurEff->GetSpellProto()->StackAmount) // Cast damage spell at max stacks
                {
                    target->CastSpell(target, SPELL_DEEP_CORRUPTION_DAMAGE, true);
                    aurEff->GetBase()->Remove();
                }
            }
        }

        void Register() override
        {
            OnEffectApply += AuraEffectApplyFn(spell_gen_deep_corruption_AuraScript::OnApply, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_gen_deep_corruption_AuraScript::HandleDummyTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_gen_deep_corruption_AuraScript();
    }
};

void AddSC_boss_yorsahj_the_unsleeping()
{
    new boss_yorsahj_the_unsleeping();      // 55312
    new npc_yorsahj_blood_globule();        // 55862, 55863, 55864, 55865, 55866, 55867
    new npc_forgotten_one_DS();             // 56265
    new npc_mana_void_DS();                 // 56231

    new spell_gen_color_combination();      // 105420, 105435, 105436, 105437, 105439, 105440
    new spell_gen_searing_blood();          // 105033, 108356, 108357, 108358
    new spell_gen_mana_void_summon();       // 105034
    new spell_gen_ooze_selector();          // 103635, 105904
    new spell_gen_deep_corruption();        // 103628
}

// select * from creature_template where entry in (55312,55862,55863,55864,55865,55866,55867,56265,56231)

/*
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105420, 'spell_gen_color_combination');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105435, 'spell_gen_color_combination');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105436, 'spell_gen_color_combination');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105437, 'spell_gen_color_combination');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105439, 'spell_gen_color_combination');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105440, 'spell_gen_color_combination');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105033, 'spell_gen_searing_blood');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (108356, 'spell_gen_searing_blood');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (108357, 'spell_gen_searing_blood');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (108358, 'spell_gen_searing_blood');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105034, 'spell_gen_mana_void_summon');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (103635, 'spell_gen_ooze_selector');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105904, 'spell_gen_ooze_selector');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (103628, 'spell_gen_deep_corruption');

INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ErrorTextId`, `NegativeCondition`, `ScriptName`, `Comment`) VALUES ('13', '0', '108224', '0', '18', '1', '56231', '0', '0', '0', '', 'Spell 108224 targets npc 56231 only');
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ErrorTextId`, `NegativeCondition`, `ScriptName`, `Comment`) VALUES ('13', '0', '105536', '0', '18', '1', '56231', '0', '0', '0', '', 'Spell 105536  targets npc 56231 only');

INSERT INTO `spell_proc_event` (`entry`, `Cooldown`) VALUES ('103968', '1');
*/