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

#define SEARCH_RANGE                   300.0f
#define SEARCH_RANGE_FAR               500.0f
#define OUTRO_CINEMATIC                 76
#define ANGLE_RANGE                      0.28f

// NPCs
enum NPC
{
    BOSS_MADNESS_OF_DEATHWING           = 56173,
    // Parts of Deathwing
    NPC_WING_TENTACLE                   = 56168, // 1 & 4
    NPC_ARM_TENTACLE_LEFT               = 56846, // 2
    NPC_ARM_TENTACLE_RIGHT              = 56167, // 3

    NPC_WING_LEFT                       = 57696,
    NPC_WING_RIGHT                      = 57695,
    NPC_ARM_LEFT                        = 57694,
    NPC_ARM_RIGHT                       = 57686,
    NPC_HEAD                            = 119561,

    NPC_COSMETIC_TENTACLE               = 57693,
    NPC_TAIL_TENTACLE                   = 56844,

    // Phase 1 adds
    NPC_BLISTERING_TENTACLE             = 56188,
    NPC_PLATFORM                        = 56307,
    NPC_ELEMENTIUM_BOLT                 = 56262,
    NPC_MUTATED_CORRUPTION              = 56471,
    NPC_TIME_ZONE                       = 56311,
    NPC_REGENERATIVE_BLOOD              = 56263, // 105875

    NPC_CRUSH_TARGET                    = 56581,
    NPC_HEMORRHAGE_TARGET               = 56359, // 105853
    NPC_CLAWK_MARK                      = 56545,
    NPC_CORRUPTING_PARASITE_TENTACLE    = 57480,
    NPC_CORRUPTING_PARASITE             = 57479,

    // Phase 2 adds
    NPC_ELEMENTIUM_TERROR               = 56710, // 106767
    NPC_ELEMENTIUM_FRAGMENT             = 56724, // 106777
    NPC_CONGEALING_BLOOD                = 57798,

    NPC_DEATHWING_HEAD                  = 57962,
    NPC_AGGRA                           = 58211,
};

// Spells
enum Spells
{
    SPELL_SUMMON_TAIL_FORCE             = 106239,
    SPELL_SUMMON_TAIL                   = 106240,
    SPELL_IDLE                          = 106187, // tail tentacle has it
    SPELL_TRIGGER_ASPECT_BUFFS          = 106943, // casted by deathwing 56173
    SPELL_SHARE_HEALTH                  = 109548, // casted by deathwing 56173 on his head
    SPELL_ASSAULT_ASPECTS               = 107018, // casted by deathwing 56173
    SPELL_ELEMENTIUM_BOLT               = 105651,
    SPELL_ELEMENTIUM_METEOR_SCRIPT      = 105599,

    SPELL_ELEMENTIUM_BLAST              = 105723,
    SPELL_ELEMENTIUM_METEOR_TARGET      = 106242, // target mark
    //SPELL_ELEMENTIUM_METEOR_TRANSFORM_1 = 106991, // summon from sniffs ?
    //SPELL_ELEMENTIUM_METEOR_TRANSFORM_2 = 110663,
    SPELL_ELEMENTIUM_METEOR_AURA        = 110628,
    SPELL_ELEMENTIUM_METEOR_AURA_DMG    = 110632,

    SPELL_CATACLYSM                     = 106523,
    SPELL_CATACLYSM_25                  = 110044,
    SPELL_CATACLYSM_10H                 = 110043,
    SPELL_CATACLYSM_25H                 = 110042,

    SPELL_SLUMP                         = 106708, // Fall down with his head
    SPELL_SLUMP_2                       = 110062, // on death ?
    SPELL_EMERGE                        = 22445,

    SPELL_CORRUPTED_BLOOD_10N           = 106834,
    SPELL_CORRUPTED_BLOOD_25N           = 109592,
    SPELL_CORRUPTED_BLOOD_10HC          = 109593,
    SPELL_CORRUPTED_BLOOD_25HC          = 109594,
    SPELL_CORRUPTED_BLOOD_STACKER       = 106843,

    SPELL_DEATH                         = 110101,
    SPELL_BERSERK                       = 64238,
    SPELL_ACHIEVEMENT                   = 111533,

    // Limb Tentacle
    SPELL_LIMB_EMERGE_VISUAL            = 107991,
    SPELL_SUMMON_BLISTERING_TENTACLE    = 105549,
    SPELL_BURNING_BLOOD                 = 105401,
    SPELL_BURNING_BLOOD_25              = 109616,
    SPELL_BURNING_BLOOD_10H             = 109617,
    SPELL_BURNING_BLOOD_25H             = 109618,
    SPELL_TRIGGER_CONCENTRATION         = 106940,
    SPELL_AGONIZING_PAIN                = 106548,

    // Mutated Corruption
    SPELL_CRUSH_FORCE                   = 106382,
    SPELL_CRUSH_SUMMON                  = 106384,
    SPELL_CRUSH                         = 106385,
    SPELL_IMPALE                        = 106400,
    SPELL_IMPALE_DMG                    = 106444,
    SPELL_IMPALE_ASPECT_AOE             = 107026,
    SPELL_IMPALE_ASPECT                 = 107029,

    // Blistering Tentacle
    SPELL_BLISTERING_TENTACLE_VEHICLE   = 105550,
    SPELL_BLISTERING_HEAT               = 105444,
    SPELL_BLISTERING_HEAT_DMG           = 105445,
    SPELL_AVOIDANCE                     = 65220,

    // Hemorrhage
    SPELL_HEMORRHAGE_SUMMON_AOE         = 105853,
    SPELL_HEMORRHAGE_AURA               = 105861,
    SPELL_HEMORRHAGE_SCRIPT             = 105862,
    SPELL_HEMORRHAGE_MISSILE            = 105863,
    SPELL_HEMORRHAGE_SUMMON             = 105875,
    SPELL_REGENERATIVE_BLOOD_PERIODIC   = 105932,
    SPELL_REGENERATIVE_BLOOD_SCRIPT     = 105934,
    SPELL_REGENERATIVE_BLOOD_HEAL       = 105937,
    SPELL_REGENERATIVE_BLOOD_AURA       = 105969, // scale aura
    SPELL_DEGENERATIVE_BITE             = 105842,

    // Congealing Blood
    SPELL_CONGEALING_BLOOD_AOE          = 109082,
    SPELL_CONGEALING_BLOOD_SCRIPT       = 109087,
    SPELL_CONGEALING_BLOOD_MISSILE      = 109089,
    SPELL_CONGEALING_BLOOD_SUMMON       = 109091,
    SPELL_CONGEALING_BLOOD_HEAL         = 109102,

    // Elementium Terror
    SPELL_ELEMENTIUM_TERROR_SCRIPT      = 106765,
    SPELL_ELEMENTIUM_TERROR_MISSILE     = 106766,
    SPELL_ELEMENTIUM_TERROR_SUMMON      = 106767, // original name is hemorrhage

    // Impaling Tentacle
    SPELL_IMPALING_TENTACLE_SCRIPT      = 106775,
    SPELL_IMPALING_TENTACLE_MISSILE     = 106776,
    SPELL_IMPALING_TENTACLE_SUMMON      = 106777,

    // Elementium Terror
    SPELL_TETANUS_AURA                  = 106728,

    // Elementium Fragment
    SPELL_SHRAPNEL_AURA                 = 106818,
    SPELL_SHRAPNEL_AOE                  = 106789,
    SPELL_SHRAPNEL_TARGET               = 106794,
    SPELL_SHRAPNEL_TARGET_10HC          = 110140,
    SPELL_SHRAPNEL_TARGET_25N           = 110141,
    SPELL_SHRAPNEL_TARGET_25HC          = 110139,
    SPELL_SHRAPNEL_DMG                  = 106791,

    // Corrupting Parasite
    SPELL_CORRUPTING_PARASITE_AOE       = 108597,
    SPELL_CORRUPTING_PARASITE_DMG       = 108601,
    SPELL_CORRUPTING_PARASITE_AURA      = 108649,
    SPELL_PARASITIC_BACKSLASH           = 108787,
    SPELL_UNSTABLE_CORRUPTION           = 108813,

    // Aspects
    SPELL_CONCENTRATION_ALEXSTRASZA     = 106641,
    SPELL_CONCENTRATION_NOZDORMU        = 106642,
    SPELL_CONCENTRATION_YSERA           = 106643,
    SPELL_CONCENTRATION_KALECGOS        = 106644,

    SPELL_ALEXSTRASZA_PRESENCE          = 105825,
    SPELL_NOZDORMU_PRESENCE             = 105823,
    SPELL_YSERA_PRESENCE                = 106456,
    SPELL_KALECGOS_PRESENCE             = 106026,

    SPELL_EXPOSE_WEAKNESS_ALEXSTRASZA   = 106588,
    SPELL_EXPOSE_WEAKNESS_NOZDORMU      = 106600,
    SPELL_EXPOSE_WEAKNESS_YSERA         = 106613,
    SPELL_EXPOSE_WEAKNESS_KALECGOS      = 106624,

    SPELL_TIME_ZONE_MISSILE             = 105799,
    SPELL_TIME_ZONE_AURA_DUMMY          = 105831, // aura (debuff)
    SPELL_TIME_ZONE_AURA_2              = 108646, // for parasite
    SPELL_THE_DREAMER                   = 106463,
    SPELL_ENTER_THE_DREAM               = 106464,
    SPELL_SPELLWEAVER                   = 106039,
    SPELL_SPELLWEAVING                  = 106040,
    SPELL_SPELLWEAVING_DMG_10N          = 106043,
    SPELL_SPELLWEAVING_DMG_25N          = 109609,
    SPELL_SPELLWEAVING_DMG_10HC         = 109610,
    SPELL_SPELLWEAVING_DMG_25HC         = 109611,
    SPELL_FIRE_DRAGON_SOUL              = 109971,

    ACHIEVEMENT_DESTROYERS_END          = 6177,
    ACHIEVEMENT_HEORIC_MADNESS          = 6116,
    ACHIEVEMENT_REALM_FIRST_MADNESS     = 6126,
};

enum MadnesGameobjects
{
    GO_ELEMENTIUM_FRAGMENT_10N      = 210217,
    GO_ELEMENTIUM_FRAGMENT_25N      = 210218,
    GO_ELEMENTIUM_FRAGMENT_10HC     = 210219,
    GO_ELEMENTIUM_FRAGMENT_25HC     = 210220,
};

#define MAX_PLATFORMS                 4

enum Platforms
{
    PLATFORM_ALEXSTRASZA            = 0,
    PLATFORM_NOZDORMU               = 1,
    PLATFORM_YSERA                  = 2,
    PLATFORM_KALECGOS               = 3,
    // Summon Elementium Bolt Position
    ELEMENTIUM_BOLT_SPAWN_ALEX      = 4,
    ELEMENTIUM_BOLT_SPAWN_NOZDORMU  = 5,
    ELEMENTIUM_BOLT_SPAWN_YSERA     = 6,
    ELEMENTIUM_BOLT_SPAWN_KALECGOS  = 7,
};

enum MadnessActions
{
    ACTION_SET_PLATFORM                 = 0,
    ACTION_LIMB_DIED                    = 1,
    ACTION_TELEPORT_HOME                = 5,
    ACTION_SPAWN_HEAD                   = 4,
    ACTION_SPAWN_ELEMENT_TERRORS        = 7,
    ACTION_SPAWN_ELEMENT_FRAGMENTS      = 8,
    ACTION_SPAWN_CONGEALING_BLOODS      = 9,
    ACTION_ASPECTS_SECOND_PHASE         = 10,
    ACTION_ASPECTS_OUTRO                = 11,
    ACTION_ASPECTS_HIDE                 = 12,
    ACTION_CORRUPTING_PARASITE          = 13,
};

enum SeatPosition
{
    DEATHWING_RIGHT_ARM             = 0,
    DEATHWING_LEFT_ARM              = 1,
    DEATHWING_HEAD                  = 2,
    DEATHWING_RIGHT_WING            = 3,
    DEATHWING_LEFT_WING             = 4,
    FREE_SEAT_POSITION              = 5,
    MAX_SEAT_POSITIONS              = 6,
};

enum Phase
{
    PHASE_LIMBS                     = 0,
    PHASE_SLUMB                     = 1,
    PHASE_HEAD_DOWN                 = 2,
    PHASE_OUTRO                     = 3,
};

const Position limbsPos[MAX_PLATFORMS] =
{
    { -11941.2f, 12248.9f, 12.1f, 1.98f },
    { -12005.8f, 12190.3f, -6.5f, 2.12f },
    { -12065.0f, 12127.2f, -3.2f, 2.33f },
    { -12097.8f, 12067.4f, 13.4f, 2.21f },
};

const Position tailPos = { -11852.6f, 11844.2f, -64.9f,  2.17f };

const Position platformPos[MAX_PLATFORMS] =
{
    { -11958.7f, 12262.9f,  1.3f, 5.07f },
    { -12039.8f, 12224.3f, -6.1f, 5.32f },
    { -12099.0f, 12161.2f, -2.7f, 5.77f },
    { -12128.8f, 12077.4f,  2.3f, 2.21f },
};

#define MAX_BOLT_POSITIONS              8

const Position boltPos[MAX_BOLT_POSITIONS] =
{
    { -11961.2f, 12286.0f,  1.3f, 0.0f }, // Alexstrasza land position
    { -12055.0f, 12239.0f, -6.1f, 0.0f }, // Nozdormu land position
    { -12119.2f, 12174.9f, -2.7f, 0.0f }, // Ysera land position
    { -12149.8f, 12081.4f,  2.3f, 0.0f }, // Kalecgos land position
    { -11929.2f, 12187.1f, 17.5f, 0.2f }, // Summon Location Alexstrasza
    { -11992.1f, 12155.8f, 16.2f, 0.6f }, // Summon Location Nozdormu
    { -12027.6f, 12109.1f, 15.5f, 2.6f }, // Summon Location Ysera
    { -12050.0f, 12053.0f, 22.6f, 2.7f }, // Summon Location Kalecgos
};

const Position mutatedcorruptionPos[MAX_PLATFORMS] =
{
    { -11993.3f, 12286.3f, -2.5f, 5.91f }, // Alexstrasza platform
    { -12028.8f, 12265.6f, -6.2f, 4.13f }, // Nozdormu platform
    { -12107.4f, 12201.9f, -5.3f, 5.16f }, // Ysera platform
    { -12160.9f, 12057.0f,  2.4f, 0.73f }  // Kalecgos platform
};

#define MAX_HEMORRHAGE_ADDS             6

const Position hemorrhagePos[MAX_PLATFORMS] =
{
    { -11955.9f, 12281.7f,  1.30f, 0.0f },
    { -12048.0f, 12237.6f, -6.14f, 0.0f },
    { -12113.9f, 12166.7f, -2.72f, 0.0f },
    { -12146.3f, 12093.5f,  2.31f, 0.0f },
};

#define MAX_FRAGMENT_POSITIONS_10MAN         3
#define MAX_FRAGMENT_POSITIONS_25MAN         8

const Position fragmentTargetPos = { -12107.54f, 12168.70f, -2.73f, 0.61f };

#define MAX_TERROR_POSITIONS            2

const Position terrorPos[MAX_TERROR_POSITIONS] =
{
    { -12121.2f, 12162.7f, -2.7f, 0.0f },
    { -12117.7f, 12168.8f, -2.7f, 0.0f }
};

const uint8 MAX_CONGEALING_BLOODS           = 9;

#define MAX_CONGEALING_POSITIONS            3

const Position congealingPosSpawn[MAX_CONGEALING_POSITIONS] = 
{
    { -12074.78f, 12135.56f, -2.73f, 0.0f },
    { -12099.60f, 12144.50f, -2.73f, 0.0f },
    { -12126.18f, 12173.20f, -2.73f, 0.0f }
};

const Position congealingPosHeal  = { -12076.49f, 12173.56f, -2.83f, 0.0f };

#define MAX_ELEMENTIUM_BOLT_QUOTES             3 

static const PlayableQuote elementiumBoltQuotes[MAX_ELEMENTIUM_BOLT_QUOTES] =
{
    { 26354, "There's no shelter from my fury." },
    { 26355, "Your armor means nothing, your faith even less." },
    { 26356, "The sea will swallow your smoldering remains." },
};

const uint8 MAX_KILL_QUOTES = 3;

static const PlayableQuote killQuotes[MAX_KILL_QUOTES] =
{
    { 26528, "There is no hope, no future, all light is doomed!" },
    { 26529, "Like you, all Azeroth shall burn." },
    { 26530, "Know that your world will soon follow." },
};

const uint8 MAX_CATACLYSM_QUOTES = 3;

static const PlayableQuote cataclysmQuotes[MAX_CATACLYSM_QUOTES] =
{
    { 26357, "I shall tear this world apart!" },
    { 26358, "I shall show you a true CATACLYSM!" },
    { 26347, "Deathwing screams" } // Used only for audio effect
};

static const PlayableQuote aggroQuote = { 26527, "You have done NOTHING. I will tear your world APART." };

static const PlayableQuote slumbQuote = { 26353, "I AM DEATHWING, THE DESTROYER, THE END OF ALL THINGS, INEVITABLE, INDOMITABLE; I AM THE CATACLYSM!" };

const Position deathwingPos         = { -11903.9f, 11989.1f, -113.2f, 2.164f };
const Position deathwing2Pos        = { -12063.5f, 12198.9f,  -13.0f, 2.164f };

const Position alexstraszaendPos    = { -12077.3f, 12152.3f, -2.6f, 6.00f };
const Position nozdormuendPos       = { -12078.4f, 12147.5f, -2.6f, 0.17f };
const Position yseraendPos          = { -12073.8f, 12156.6f, -2.6f, 5.55f };
const Position kalecgosendPos       = { -12069.2f, 12159.9f, -2.6f, 5.23f };
const Position aggraendPos          = { -12066.1f, 12150.4f, -2.6f, 3.05f };
const Position thrallendPos         = { -12067.7f, 12146.4f, -2.6f, 2.31f };
const Position cacheSpawnPos        = { -12076.6f, 12169.9f, -2.5f, 3.54f };

struct AspectsEntries
{
    uint32 aspectEntryId;
    const char * textAssault;
    uint32 firstSoundId;
    const char * aspectFirstQuote;
    uint32 secondSoundId;
    const char * aspectSecondQuote;
    uint32 presenceId;
    uint32 concentrationId;
    uint32 exposeWeaknessId;
};

static AspectsEntries aspectsInfo[MAX_PLATFORMS] =
{
    { NPC_ALEXSTRASZA_DRAGON_FORM, "Deathwing assaults Alexstrasza", 
      26498, "I will cleanse whatever corruption I can; my fire will not harm you.",
      26500, "No! Such power! Deathwing's summoning of the final Cataclysm will destroy all life on Azeroth. Quickly, we must interrupt him!",
      SPELL_ALEXSTRASZA_PRESENCE, SPELL_CONCENTRATION_ALEXSTRASZA, SPELL_EXPOSE_WEAKNESS_ALEXSTRASZA },

    { NPC_NOZDORMU_DRAGON_FORM, "Deathwing assaults Nozdormu",
      25949, "I will slow the Destroyer's attacks when I can.",
      25951, "Hurry, heroes. In mere moments Deathwing's Cataclysm will complete what he begun and end the world. Join me in the attack, now!",
      SPELL_NOZDORMU_PRESENCE, SPELL_CONCENTRATION_NOZDORMU, SPELL_EXPOSE_WEAKNESS_NOZDORMU },

    { NPC_YSERA_DRAGON_FORM, "Deathwing assaults Ysera",
      26142, "I will bring you closer to the Emerald Dream. Seek safety there when the fight becomes too intense.",
      26144, "Deathwing is conjuring the final Cataclysm; even the Emerald Dream trembles. If we are to stop the spell, we must attack him together.",
      SPELL_YSERA_PRESENCE, SPELL_CONCENTRATION_YSERA, SPELL_EXPOSE_WEAKNESS_YSERA },
    
    { NPC_KALECGOS_DRAGON_FORM, "Deathwing assaults Kalecgos",
      26259, "I will charge you with arcane energy to blast your foes.",
      26261, "The Destroyer is gathering all his might for a blow that will split the world. Attack him, now! We must stop the final Cataclysm!",
      SPELL_KALECGOS_PRESENCE, SPELL_CONCENTRATION_KALECGOS, SPELL_EXPOSE_WEAKNESS_KALECGOS },
};

// Madness of Deathwing
class boss_madness_of_deathwing : public CreatureScript
{
public:
    boss_madness_of_deathwing() : CreatureScript("boss_madness_of_deathwing") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_madness_of_deathwingAI(pCreature);
    }

    struct boss_madness_of_deathwingAI : public ScriptedAI
    {
        boss_madness_of_deathwingAI(Creature *creature) : ScriptedAI(creature), summons(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        TaskScheduler scheduler;
        SummonList summons;

        uint64 limbGuids[MAX_PLATFORMS];
        uint64 platformGuids[MAX_PLATFORMS];

        uint32 assaultAspectsTimer;
        uint32 elementiumBoltTimer;
        uint32 cataclysmTimer;
        uint32 elementiumFragmentsTimer;
        uint32 elementiumTerrorsTimer;
        uint32 congealingTimer;
        uint32 congealingCounter;
        uint32 healthPctForNextSpawn;
        uint32 berserkTimer;

        uint8 platformKilled;
        uint8 activePlatform;
        uint8 phase;

        bool newAssault;
        bool canCataclysm;
        bool platformDestroyed[MAX_PLATFORMS];
        bool canSpawnCongealing;

        void Reset() override
        {
            if (instance)
            {
                if (instance->GetData(TYPE_BOSS_MADNESS_OF_DEATHWING) != DONE)
                    instance->SetData(TYPE_BOSS_MADNESS_OF_DEATHWING, NOT_STARTED);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

                if (Creature * pThrall = me->FindNearestCreature(NPC_THRALL_MADNESS_START, SEARCH_RANGE_FAR, true))
                    pThrall->AI()->DoAction(ACTION_TELEPORT_HOME);
            }

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetVisible(false);
            me->RemoveAura(SPELL_SLUMP);
            me->RemoveAura(SPELL_BERSERK);

            assaultAspectsTimer = 5000;
            newAssault = true;
            canCataclysm = false;
            canSpawnCongealing = false;
            platformKilled = 0;
            activePlatform = 0;
            elementiumFragmentsTimer = 0;
            elementiumTerrorsTimer = 0;
            congealingTimer = 0;
            healthPctForNextSpawn = 15;
            congealingCounter = 0;
            berserkTimer = 15 * MINUTE * IN_MILLISECONDS;
            phase = PHASE_LIMBS;

            for (uint8 i = PLATFORM_ALEXSTRASZA; i < MAX_PLATFORMS; i++)
            {
                platformDestroyed[i] = false;
            }

            summons.DespawnAll();
            scheduler.CancelAll();
            ScriptedAI::Reset();
        }

        void JustSummoned(Creature* summon) override
        {
            summons.push_back(summon->GetGUID());
        }

        void SummonedCreatureDies(Creature* unit, Unit* /*killer*/) override
        {
            me->SendPlaySound(cataclysmQuotes[MAX_CATACLYSM_QUOTES - 1].GetSoundId(), false);

            if (unit->GetEntry() == NPC_WING_TENTACLE || unit->GetEntry() == NPC_ARM_TENTACLE_LEFT || unit->GetEntry() == NPC_ARM_TENTACLE_RIGHT)
            {
                // Cancel Cataclysm Quote
                scheduler.CancelGroup(2);

                // Set platform destroyed
                platformDestroyed[activePlatform] = true;

                for (uint32 i = 0; i < MAX_PLATFORMS; i++)
                {
                    if (unit->GetGUID() == limbGuids[i])
                    {
                        if (Creature * pAspect = me->FindNearestCreature(aspectsInfo[i].aspectEntryId, SEARCH_RANGE_FAR, true))
                        {
                            pAspect->RemoveAurasDueToSpell(aspectsInfo[i].presenceId);

                            if (Vehicle * veh = me->GetVehicleKit())
                            {
                                if (Unit * vehiclePassenger = veh->GetPassenger(DEATHWING_HEAD))
                                    pAspect->CastSpell(vehiclePassenger, aspectsInfo[i].concentrationId, true);
                            }

                            if (instance)
                            {
                                if (activePlatform == PLATFORM_YSERA)
                                {
                                    pAspect->RemoveAurasDueToSpell(SPELL_THE_DREAMER);
                                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_ENTER_THE_DREAM);
                                }
                                else if (activePlatform == PLATFORM_KALECGOS)
                                {
                                    pAspect->RemoveAurasDueToSpell(SPELL_SPELLWEAVER);
                                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SPELLWEAVING);
                                }
                            }
                        }
                    }
                }
            }
        }

        void SpawnLimbs()
        {
            // Spawn Tentacles
            for (uint8 i = PLATFORM_ALEXSTRASZA; i < MAX_PLATFORMS; i++)
            {
                uint32 limbsEntryId = 0;
                switch (i)
                {
                    case PLATFORM_ALEXSTRASZA:
                    case PLATFORM_KALECGOS:
                        limbsEntryId = NPC_WING_TENTACLE;
                        break;
                    case PLATFORM_NOZDORMU:
                        limbsEntryId = NPC_ARM_TENTACLE_LEFT;
                        break;
                    case PLATFORM_YSERA:
                        limbsEntryId = NPC_ARM_TENTACLE_RIGHT;
                        break;
                    default:
                        break;
                }

                if (Creature* pLimb = me->SummonCreature(limbsEntryId, limbsPos[i], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5000))
                {
                    pLimb->SendPlaySpellVisualKit(SPELL_EMERGE, 0);
                    limbGuids[i] = pLimb->GetGUID();
                }

                // Spawn other parts of Deathwing's body
                if (Vehicle * veh = me->GetVehicleKit())
                {
                    uint8 seats = veh->m_Seats.size();

                    for (uint8 i = 0; i < seats; i++)
                    {
                        uint32 npcEntry = 0;
                        if (i == DEATHWING_RIGHT_ARM)
                            npcEntry = NPC_ARM_RIGHT;
                        else if (i == DEATHWING_LEFT_ARM)
                            npcEntry = NPC_ARM_LEFT;
                        else if (i == DEATHWING_RIGHT_WING)
                            npcEntry = NPC_WING_RIGHT;
                        else if (i == DEATHWING_LEFT_WING)
                            npcEntry = NPC_WING_LEFT;
                        else if (i == DEATHWING_HEAD)
                            npcEntry = NPC_HEAD;

                        if (i != FREE_SEAT_POSITION)
                        {
                            if (Creature * vehiclePassenger = me->SummonCreature(npcEntry, 0, 0, 0))
                            {
                                vehiclePassenger->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                vehiclePassenger->EnterVehicle(me, i);
                            }
                        }
                    }
                }

                // Spawn his tail
                if (Creature* pTail = me->SummonCreature(NPC_TAIL_TENTACLE, tailPos))
                    pTail->SendPlaySpellVisualKit(SPELL_EMERGE, 0);

                if (Creature* pPlatform = me->SummonCreature(NPC_PLATFORM, platformPos[i]))
                    platformGuids[i] = pPlatform->GetGUID();
            }
        }

        void EnterCombat(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_MADNESS_OF_DEATHWING, IN_PROGRESS);
                instance->DoRemoveAurasDueToSpellOnPlayers(106213); // Blood of Neltharion
            }

            me->SendPlaySpellVisualKit(SPELL_EMERGE, 0);
            me->SetVisible(true);
            SpawnLimbs();
            RunPlayableQuote(aggroQuote);
        }

        void SummonCache()
        {
            uint32 cacheId = 0;
            switch (getDifficulty())
            {
            case RAID_DIFFICULTY_10MAN_NORMAL:
                cacheId = GO_ELEMENTIUM_FRAGMENT_10N;
                break;
            case RAID_DIFFICULTY_25MAN_NORMAL:
                cacheId = GO_ELEMENTIUM_FRAGMENT_25N;
                break;
            case RAID_DIFFICULTY_10MAN_HEROIC:
                cacheId = GO_ELEMENTIUM_FRAGMENT_10HC;
                break;
            case RAID_DIFFICULTY_25MAN_HEROIC:
                cacheId = GO_ELEMENTIUM_FRAGMENT_25HC;
                break;
            default:
                break;
            }
            me->SummonGameObject(cacheId, cacheSpawnPos.GetPositionX(), cacheSpawnPos.GetPositionY(), cacheSpawnPos.GetPositionZ(), cacheSpawnPos.GetOrientation(), 0, 0, 0, 0, 86400);
        }

        void RewardRealmFirstAchievement()
        {
            AchievementEntry const* achievement = sAchievementStore.LookupEntry(ACHIEVEMENT_REALM_FIRST_MADNESS);

            if (!sAchievementMgr->IsRealmCompleted(achievement)) // If (Realm First! Madness of Deathwing achievement) is not already completed
            {
                Map::PlayerList const& plrList = me->GetMap()->GetPlayers();
                if (!plrList.isEmpty())
                {
                    if (Player * player = plrList.getFirst()->getSource())
                    {
                        if (player->GetGroup() && player->GetGroup()->IsGuildGroup(player->GetGuildId())) // Not in guild group
                        {
                            for (Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
                            {
                                if (Player* pl = itr->getSource())
                                {
                                    pl->CompletedAchievement(achievement); // Manually complete achievement for every player
                                }
                            }
                            sAchievementMgr->SetRealmCompleted(achievement); // Lock Achievement manually
                        }
                    }
                }
            }
        }

        void PlayMovieToPlayers(uint32 movieId)
        {
            if (!instance)
                return;

            Map::PlayerList const& plList = instance->instance->GetPlayers();
            if (plList.isEmpty())
                return;

            for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
            {
                if (Player * pPlayer = itr->getSource())
                    pPlayer->SendMovieStart(movieId);
            }
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim && victim->GetTypeId() == TYPEID_PLAYER)
            {
                uint32 rand = urand(0, MAX_KILL_QUOTES - 1);
                RunPlayableQuote(killQuotes[rand]);
            }
        }

        void Outro()
        {
            summons.DespawnAll();
            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_ENTER_THE_DREAM);
            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SPELLWEAVING);

            // Outro
            PlayMovieToPlayers(OUTRO_CINEMATIC);
            me->SummonCreature(NPC_ALEXSTRASZA_MADNESS, alexstraszaendPos, TEMPSUMMON_MANUAL_DESPAWN);
            me->SummonCreature(NPC_YSERA_MADNESS, yseraendPos, TEMPSUMMON_MANUAL_DESPAWN);
            me->SummonCreature(NPC_KALECGOS_MADNESS, kalecgosendPos, TEMPSUMMON_MANUAL_DESPAWN);
            me->SummonCreature(NPC_NOZDORMU_MADNESS, nozdormuendPos, TEMPSUMMON_MANUAL_DESPAWN);
            me->SummonCreature(NPC_THRALL_MADNESS, thrallendPos, TEMPSUMMON_MANUAL_DESPAWN);
            me->SummonCreature(NPC_AGGRA, aggraendPos, TEMPSUMMON_MANUAL_DESPAWN);

            if (Creature * pThrall = me->FindNearestCreature(NPC_THRALL_MADNESS_START, SEARCH_RANGE, true))
                pThrall->AI()->DoAction(ACTION_ASPECTS_HIDE);

            scheduler.CancelAll();
        }

        void JustDied(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_MADNESS_OF_DEATHWING, DONE);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

                if (IsHeroic())
                {
                    instance->DoCompleteAchievement(ACHIEVEMENT_HEORIC_MADNESS);
                    RewardRealmFirstAchievement();
                }
                instance->DoCompleteAchievement(ACHIEVEMENT_DESTROYERS_END);

                instance->DoModifyPlayerCurrencies(CURRENCY_VALOR_POINTS, (IsHeroic() ? 140 : 120), CURRENCY_SOURCE_OTHER);
                instance->DoModifyPlayerCurrencies(CURRENCY_ESSENCE_OF_CORRUPTED_DEATHWING, 1, CURRENCY_SOURCE_OTHER);

                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_ENTER_THE_DREAM);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SPELLWEAVING);
            }

            SummonCache();
        }

        void DamageTaken(Unit* who, uint32& damage)
        {
            if (phase == PHASE_HEAD_DOWN)
            {
                // Start Final Aspect Outro
                if (me->GetHealth() <= damage)
                {
                    me->SendPlaySound(cataclysmQuotes[MAX_CATACLYSM_QUOTES - 1].GetSoundId(), false);
                    damage = me->GetHealth() - 1;
                    me->RemoveAura(SPELL_SLUMP);
                    me->RemoveAura(SPELL_CORRUPTED_BLOOD_10N);
                    me->RemoveAura(SPELL_CORRUPTED_BLOOD_10HC);
                    me->RemoveAura(SPELL_CORRUPTED_BLOOD_25N);
                    me->RemoveAura(SPELL_CORRUPTED_BLOOD_25HC);
                    me->CastSpell(me, SPELL_SLUMP_2, false);

                    summons.DespawnAll();
                    phase = PHASE_OUTRO;

                    if (Creature * pThrall = me->FindNearestCreature(NPC_THRALL_MADNESS_START, 500.0f, true))
                        pThrall->AI()->DoAction(ACTION_ASPECTS_OUTRO);

                    // Schedule Death
                    scheduler.Schedule(Seconds(66), [this](TaskContext death)
                    {
                        if (death.GetRepeatCounter() == 0)
                        {
                            me->SendPlaySound(cataclysmQuotes[MAX_CATACLYSM_QUOTES - 1].GetSoundId(), false);
                            me->CastSpell(me, SPELL_DEATH, false);
                            death.Repeat(Seconds(7));
                        }
                        else if (death.GetRepeatCounter() == 1)
                        {
                            Outro();
                            me->SetVisible(false);
                            death.Repeat(Seconds(95));
                        }
                        else
                            me->Kill(me);
                    });
                }
            }
            else if (phase == PHASE_OUTRO)
                damage = 0;
        }

        void SpawnAdds(uint32 action)
        {
            if (Creature * pHemorrhageTarget = me->SummonCreature(NPC_HEMORRHAGE_TARGET, fragmentTargetPos, TEMPSUMMON_TIMED_DESPAWN, 10000))
            {
                if (Creature * pDeathwingHead = me->FindNearestCreature(NPC_DEATHWING_HEAD, 300.0f, true))
                {
                    if (action == ACTION_SPAWN_ELEMENT_FRAGMENTS)
                    {
                        for (uint8 i = 0; i < (Is25ManRaid() ? MAX_FRAGMENT_POSITIONS_25MAN : MAX_FRAGMENT_POSITIONS_10MAN); i++)
                        {
                            pDeathwingHead->CastSpell(pHemorrhageTarget, SPELL_IMPALING_TENTACLE_MISSILE, true);
                        }
                    }
                    else if (action == ACTION_SPAWN_ELEMENT_TERRORS)
                    {
                        for (uint8 i = 0; i < 2; i++)
                        {
                            pDeathwingHead->CastSpell(pHemorrhageTarget, SPELL_ELEMENTIUM_TERROR_MISSILE, true);
                        }
                    }
                    else if (action == ACTION_SPAWN_CONGEALING_BLOODS)
                    {
                        if (Creature * pCongealingTarget = me->SummonCreature(NPC_HEMORRHAGE_TARGET, congealingPosSpawn[urand(0, MAX_CONGEALING_POSITIONS-1)], TEMPSUMMON_TIMED_DESPAWN, 10000))
                        {
                            scheduler.Schedule(Seconds(0), [this, pCongealingTarget, pDeathwingHead](TaskContext congealingBlood)
                            {
                                pDeathwingHead->CastSpell(pCongealingTarget, SPELL_CONGEALING_BLOOD_MISSILE, true);
                                if (congealingBlood.GetRepeatCounter() < 9)
                                    congealingBlood.Repeat(Milliseconds(500));
                            });
                        }
                    }
                }
            }
        }

        void DoAction(const int32 action) override
        {
            if (instance)
            {
                switch (action)
                {
                    case ACTION_LIMB_DIED:
                        me->InterruptNonMeleeSpells(false);
                        scheduler.CancelGroup(1); // Cancel Elementium Meteor and Hemorrhage
                        platformKilled++;
                        if (platformKilled == MAX_PLATFORMS)
                            phase = PHASE_SLUMB;
                        me->DealDamage(me, me->GetMaxHealth()*0.2); // Decrease hp by 20%

                        if (platformKilled <= MAX_PLATFORMS)
                        {
                            assaultAspectsTimer = 15000;
                            newAssault = true;
                            canCataclysm = false;
                        }
                        break;
                    case ACTION_SPAWN_HEAD:
                        if (Creature * pDeathwingHead = me->SummonCreature(NPC_DEATHWING_HEAD, deathwing2Pos))
                        {
                            pDeathwingHead->SetMaxHealth(me->GetMaxHealth());
                            pDeathwingHead->SetHealth(me->GetHealth());

                            if (instance)
                            {
                                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, pDeathwingHead);
                                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, pDeathwingHead);
                                instance->SendEncounterUnit(ENCOUNTER_FRAME_UPDATE_OBJECTIVE, pDeathwingHead);
                                instance->SendEncounterUnit(ENCOUNTER_FRAME_UPDATE_PRIORITY, pDeathwingHead);
                            }

                            me->CastSpell(pDeathwingHead, SPELL_SHARE_HEALTH, false);
                            pDeathwingHead->CastSpell(me, SPELL_SHARE_HEALTH, false);
                        }
                        break;
                    case ACTION_CORRUPTING_PARASITE:
                    {
                        if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 500.0f, true))
                        {
                            me->CastSpell(pTarget, SPELL_CORRUPTING_PARASITE_AURA, true);

                            if (Vehicle * veh = pTarget->GetVehicleKit())
                            {
                                if (Creature * pTentacle = me->SummonCreature(NPC_CORRUPTING_PARASITE_TENTACLE, pTarget->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN))
                                {
                                    pTentacle->SendMovementFlagUpdate();
                                    pTentacle->AI()->SetGUID(pTarget->GetGUID());
                                    pTentacle->EnterVehicle(veh, 0);
                                    pTentacle->CastSpell(pTarget, SPELL_CORRUPTING_PARASITE_DMG, true);
                                    pTentacle->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                                }
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        void CheckPlayersPosition()
        {
            Map * map = me->GetMap();
            if (!map)
                return;

            Map::PlayerList const& plrList = map->GetPlayers();
            if (plrList.isEmpty())
                return;

            double orientation[MAX_PLATFORMS] = { 1.777, 2.174, 2.390, 2.881 };

            Position pos;
            pos.m_positionX = me->GetPositionX();
            pos.m_positionY = me->GetPositionY();
            pos.m_positionZ = me->GetPositionZ();

            uint32 maxPlayersInSegment = 0;

            Map::PlayerList const &playerList = me->GetMap()->GetPlayers();

            for (uint32 segmentIndex = 0; segmentIndex < MAX_PLATFORMS; segmentIndex++)
            {
                pos.m_orientation = orientation[segmentIndex];
                uint32 playersInAngle = 0;

                for (Map::PlayerList::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
                {
                    if (Player* pPlayer = i->getSource())
                    {
                        if (pos.HasInArc(ANGLE_RANGE, pPlayer))
                        {
                            playersInAngle++;
                        }
                    }
                }

                if (playersInAngle >= maxPlayersInSegment)
                {
                    maxPlayersInSegment = playersInAngle;
                    activePlatform = segmentIndex;
                }
            }
        }

        void AssaultAspect()
        {
            me->MonsterTextEmote("Deathwing begins to cast assault aspects!", me->GetGUID(), true, SEARCH_RANGE);
            me->CastSpell(me, SPELL_ASSAULT_ASPECTS, false);

            scheduler.Schedule(Seconds(10), [this](TaskContext /* Task context */)
            {
                CheckPlayersPosition();

                // If current platform is already destroyed find another one
                if (platformDestroyed[activePlatform] == true)
                {
                    std::vector<int> freePlatforms;
                    for (int i = PLATFORM_ALEXSTRASZA; i < MAX_PLATFORMS; i++)
                    {
                        if (platformDestroyed[i] == false) // Skip destroyed platforms
                            freePlatforms.push_back(i);
                    }
                    // Select new platform randomly
                    activePlatform = freePlatforms[urand(0, freePlatforms.size() - 1)];
                }

                // Deathwing's text emote which platform he's going to assault and aspects response
                me->MonsterTextEmote(aspectsInfo[activePlatform].textAssault, me->GetGUID(), true, SEARCH_RANGE);
                if (Creature * pAspect = me->FindNearestCreature(aspectsInfo[activePlatform].aspectEntryId, SEARCH_RANGE_FAR, true))
                {
                    pAspect->MonsterYell(aspectsInfo[activePlatform].aspectFirstQuote, LANG_UNIVERSAL, 0);
                    pAspect->SendPlaySound(aspectsInfo[activePlatform].firstSoundId, false);
                }

                // Activate Burning Blood on Limb tentacle
                if (Creature * pLimb = ObjectAccessor::GetObjectInMap(limbGuids[activePlatform], me->GetMap(), (Creature*)NULL))
                {
                    if (instance)
                        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, pLimb);
                    pLimb->CastSpell(pLimb, SPELL_BURNING_BLOOD, false);
                }

                // Spawn Mutated Corruption
                if (Creature * pMutatedCorruption = me->SummonCreature(NPC_MUTATED_CORRUPTION, mutatedcorruptionPos[activePlatform], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5000))
                {
                    if (instance)
                        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, pMutatedCorruption);
                }

                // Time Zone
                if (Creature * pPlatform = ObjectAccessor::GetObjectInMap(platformGuids[activePlatform], me->GetMap(), (Creature*)NULL))
                {
                    if (Creature * pNozdormu = pPlatform->FindNearestCreature(NPC_NOZDORMU_DRAGON_FORM, SEARCH_RANGE, true))
                    {
                        if (pNozdormu->HasAura(SPELL_NOZDORMU_PRESENCE))
                            pNozdormu->CastSpell(pPlatform, SPELL_TIME_ZONE_MISSILE, true);
                    }
                }

                // Corrupting Parasite
                if (IsHeroic())
                {
                    scheduler.Schedule(Seconds(5), 1, [this](TaskContext corruptingParasite)
                    {
                        me->AI()->DoAction(ACTION_CORRUPTING_PARASITE);
                        if (corruptingParasite.GetRepeatCounter() == 0)
                            corruptingParasite.Repeat(Seconds(60));
                    });
                }

                // Elementium Bolt
                scheduler.Schedule(Seconds(45), 1, [this](TaskContext /* Elementium Bolt */)
                {
                    if (Creature * pElementiumBoltTarget = me->SummonCreature(NPC_CLAWK_MARK, boltPos[activePlatform], TEMPSUMMON_TIMED_DESPAWN, 13000))
                        pElementiumBoltTarget->CastSpell(pElementiumBoltTarget, SPELL_ELEMENTIUM_METEOR_TARGET, false);

                    if (Creature * pElementiumBolt = me->SummonCreature(NPC_ELEMENTIUM_BOLT, boltPos[activePlatform + 4], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000))
                    {
                        pElementiumBolt->GetMotionMaster()->MovePoint(0, boltPos[activePlatform], false);
                        if (platformDestroyed[PLATFORM_NOZDORMU] == true)
                        {
                            pElementiumBolt->AI()->SetData(0, 0);
                        }
                    }
                    
                    uint32 randQuote = urand(0, MAX_ELEMENTIUM_BOLT_QUOTES - 1);
                    RunPlayableQuote(elementiumBoltQuotes[randQuote]);
                });

                // Hemorrhage
                scheduler.Schedule(Seconds(90), 1, [this](TaskContext hemorrhage)
                {
                    if (Creature * pLimb = ObjectAccessor::GetObjectInMap(limbGuids[activePlatform], me->GetMap(), (Creature*)NULL))
                    {
                        if (hemorrhage.GetRepeatCounter() == 0)
                        {
                            pLimb->SummonCreature(NPC_HEMORRHAGE_TARGET, hemorrhagePos[activePlatform], TEMPSUMMON_TIMED_DESPAWN, 10000);
                            hemorrhage.Repeat(Seconds(1));
                        }
                        else if (hemorrhage.GetRepeatCounter() <= 6)
                        {
                            if (Creature * pHemorrhageTarget = pLimb->FindNearestCreature(NPC_HEMORRHAGE_TARGET, 100.0f, true))
                                pLimb->CastSpell(pHemorrhageTarget, SPELL_HEMORRHAGE_MISSILE, true);

                            hemorrhage.Repeat(Seconds(1));
                        }
                    }
                });

                cataclysmTimer = 120000;
                canCataclysm = true;
            });
        }

        void ExposeWeakness()
        {
            scheduler.Schedule(Seconds(30), 2, [this](TaskContext hemorrhage)
            {
                uint8 rand = urand(0, MAX_CATACLYSM_QUOTES - 1);
                RunPlayableQuote(cataclysmQuotes[rand]);
            });

            if (Creature * pLimb = ObjectAccessor::GetObjectInMap(limbGuids[activePlatform], me->GetMap(), (Creature*)NULL))
            {
                if (Creature* pAspect = me->FindNearestCreature(aspectsInfo[activePlatform].aspectEntryId, 500.0f, true))
                {
                    pAspect->MonsterYell(aspectsInfo[activePlatform].aspectSecondQuote, LANG_UNIVERSAL, false);
                    me->SendPlaySound(aspectsInfo[activePlatform].secondSoundId, false);
                    pAspect->CastSpell(pLimb, aspectsInfo[activePlatform].exposeWeaknessId, false);
                }
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (!UpdateVictim())
                return;

            if (berserkTimer <= diff)
            {
                me->CastSpell(me, SPELL_BERSERK, true);
                if (Creature * pDeathwingHead = me->FindNearestCreature(NPC_DEATHWING_HEAD, 300.0f, true))
                    pDeathwingHead->CastSpell(pDeathwingHead, SPELL_BERSERK, true);
            }
            else berserkTimer -= diff;

            if (phase == PHASE_LIMBS)
            {
                // Assault Aspects
                if (newAssault)
                {
                    if (assaultAspectsTimer <= diff)
                    {
                        AssaultAspect();
                        newAssault = false;
                    }
                    else assaultAspectsTimer -= diff;
                }

                // Cataclysm
                if (canCataclysm)
                {
                    if (cataclysmTimer <= diff)
                    {
                        ExposeWeakness();
                        me->CastSpell(me, SPELL_CATACLYSM, false);
                        canCataclysm = false;
                    }
                    else cataclysmTimer -= diff;
                }
            }
            else if (phase == PHASE_SLUMB)
            {
                me->SetReactState(REACT_PASSIVE);
                me->AttackStop();
                me->GetMotionMaster()->MoveIdle();
                me->SetFacingTo(deathwingPos.GetOrientation());
                me->AI()->DoAction(ACTION_SPAWN_HEAD);

                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->CastSpell(me, SPELL_SLUMP, false);
                RunPlayableQuote(slumbQuote);

                elementiumFragmentsTimer = 10000;
                elementiumTerrorsTimer = 40000;
                canSpawnCongealing = true;
                congealingTimer = 0;
                congealingCounter = 0;
                healthPctForNextSpawn = 15;
                phase = PHASE_HEAD_DOWN;

                if (Creature * pThrall = me->FindNearestCreature(NPC_THRALL_MADNESS_START, 500.0f, true))
                    pThrall->AI()->DoAction(ACTION_ASPECTS_SECOND_PHASE);
            }
            else if (phase == PHASE_HEAD_DOWN)
            {
                if (elementiumFragmentsTimer <= diff)
                {
                    SpawnAdds(ACTION_SPAWN_ELEMENT_FRAGMENTS);
                    elementiumFragmentsTimer = 90000;
                }
                else elementiumFragmentsTimer -= diff;

                if (elementiumTerrorsTimer <= diff)
                {
                    SpawnAdds(ACTION_SPAWN_ELEMENT_TERRORS);
                    elementiumTerrorsTimer = 90000;
                }
                else elementiumTerrorsTimer -= diff;

                // Congealing Bloods should spawn at 15, 10 and 5 HealthPct or 60s into the phase if HealthPct hasn't been reached yet
                if (IsHeroic())
                {
                    congealingTimer += diff;

                    if (canSpawnCongealing == false)
                    {
                        if (congealingCounter == 1)
                        {
                            if (me->GetHealthPct() <= 15)
                            {
                                congealingTimer = 0;
                                canSpawnCongealing = true;
                            }
                        }
                        else if (congealingCounter == 2)
                        {
                            if (me->GetHealthPct() <= 10)
                            {
                                congealingTimer = 0;
                                canSpawnCongealing = true;
                            }
                        }
                    }

                    if (canSpawnCongealing == true)
                    {
                        if ((me->GetHealthPct() > healthPctForNextSpawn && congealingTimer >= 60000) || me->GetHealthPct() <= healthPctForNextSpawn)
                        {
                            SpawnAdds(ACTION_SPAWN_CONGEALING_BLOODS);
                            healthPctForNextSpawn -= 5;
                            canSpawnCongealing = false;
                            congealingCounter++;
                        }
                    }
                }
            }
        }
    };
};

class npc_ds_madness_deathwing_head : public CreatureScript
{
public:
    npc_ds_madness_deathwing_head() : CreatureScript("npc_ds_madness_deathwing_head") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_madness_deathwing_headAI(pCreature);
    }

    struct npc_ds_madness_deathwing_headAI : public ScriptedAI
    {
        npc_ds_madness_deathwing_headAI(Creature* pCreature) : ScriptedAI(pCreature), summons(pCreature)
        {
            instance = pCreature->GetInstanceScript();
        }

        InstanceScript * instance;
        SummonList summons;
        uint64 summonerGuid;
        uint32 checkTimer;

        void Reset() override
        {
            me->SetInCombatWithZone();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_AGGRESSIVE);
            me->CastSpell(me, SPELL_CORRUPTED_BLOOD_10N, false);
            checkTimer = 5000;

            summons.DespawnAll();
        }

        void CheckPlayers()
        {
            uint8 count = 0;

            Map::PlayerList const& plrList = me->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
            {
                if (Player* pPlayer = itr->getSource())
                {
                    if (pPlayer->IsAlive())
                        count++;
                }
            }

            if (count == 0)
            {
                if (Creature * pDeathwing = GetSummoner<Creature>())
                {
                    if (instance)
                        instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                    pDeathwing->AI()->EnterEvadeMode();
                }
            }
        }

        void JustSummoned(Creature* summon) override
        {
            summons.push_back(summon->GetGUID());
        }

        void IsSummonedBy(Unit* pSummoner) override
        {
            summonerGuid = pSummoner->GetGUID();
        }

        void JustDied(Unit * /*who*/) override
        {
            if (instance)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

            summons.DespawnAll();
        }

        void UpdateAI(const uint32 diff) override 
        {
            if (checkTimer <= diff)
            {
                CheckPlayers();
                checkTimer = 5000;
            }
            else checkTimer -= diff;
        }
    };
};

class npc_ds_madness_deathwing_limbs : public CreatureScript
{
public:
    npc_ds_madness_deathwing_limbs() : CreatureScript("npc_ds_madness_deathwing_limbs") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_madness_deathwing_limbsAI(pCreature);
    }

    struct npc_ds_madness_deathwing_limbsAI : public ScriptedAI
    {
        npc_ds_madness_deathwing_limbsAI(Creature* pCreature) : ScriptedAI(pCreature), Summons(pCreature) 
        {
            instance = pCreature->GetInstanceScript();
        }

        InstanceScript * instance;
        SummonList Summons;
        int32 spawnHealthPct;
        bool spawnBlisteringTentacles;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetInCombatWithZone();
            me->SetReactState(REACT_PASSIVE);
            spawnBlisteringTentacles = true;
            spawnHealthPct = 70;

            if (instance)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

            me->SendPlaySpellVisualKit(22446, 0);

            Summons.DespawnAll();
        }

        void JustSummoned(Creature* summon) override
        {
            Summons.push_back(summon->GetGUID());
        }

        void SpawnBlisteringTentacles()
        {
            if (Vehicle * veh = me->GetVehicleKit())
            {
                uint8 seats = veh->m_Seats.size();
                int bp0 = -100;

                for (uint8 i = 0; i < seats; i++)
                {
                    if (Creature * vehiclePassenger = me->SummonCreature(NPC_BLISTERING_TENTACLE, 0, 0, 0, 0, TEMPSUMMON_CORPSE_DESPAWN))
                    {
                        vehiclePassenger->CastSpell(vehiclePassenger, SPELL_BLISTERING_HEAT, false);
                        vehiclePassenger->CastCustomSpell(vehiclePassenger, SPELL_AVOIDANCE, &bp0, nullptr, nullptr, true);
                        vehiclePassenger->EnterVehicle(me, i);
                        vehiclePassenger->ClearUnitState(UNIT_STATE_UNATTACKABLE);
                        vehiclePassenger->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        vehiclePassenger->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                    }
                }
            }

            if (Creature * pAlexstrasza = me->FindNearestCreature(NPC_ALEXSTRASZA_DRAGON_FORM, SEARCH_RANGE, true))
            {
                if (pAlexstrasza->HasAura(SPELL_ALEXSTRASZA_PRESENCE))
                    pAlexstrasza->CastSpell(me, 105565, false);
            }
        }

        void JustDied(Unit * /*who*/) override
        {
            if (Creature * pDeathwing = GetSummoner<Creature>())
                pDeathwing->AI()->DoAction(ACTION_LIMB_DIED);

            if (instance)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

            if (Creature * pTimeZone = me->FindNearestCreature(NPC_TIME_ZONE, SEARCH_RANGE, true))
                pTimeZone->DespawnOrUnsummon();
        }

        void UpdateAI(const uint32 diff) override
        {
            if (spawnBlisteringTentacles)
            {
                if (me->GetHealthPct() <= spawnHealthPct)
                {
                    SpawnBlisteringTentacles();
                    spawnHealthPct -= 30;
                    if (spawnHealthPct < 40)
                        spawnBlisteringTentacles = false;
                }
            }
        }
    };
};

class npc_ds_madness_blistering_tentacle : public CreatureScript
{
public:
    npc_ds_madness_blistering_tentacle() : CreatureScript("npc_ds_madness_blistering_tentacle") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_madness_blistering_tentacleAI(pCreature);
    }

    struct npc_ds_madness_blistering_tentacleAI : public Scripted_NoMovementAI
    {
        npc_ds_madness_blistering_tentacleAI(Creature* pCreature) : Scripted_NoMovementAI(pCreature)
        {
            me->AddAura(SPELL_BLISTERING_HEAT, me);
        }

        void Reset() override
        {
            me->SetInCombatWithZone();
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->CastSpell(me, SPELL_BLISTERING_HEAT, false);
            // Avoidance for AoE dmg spells
            int32 bp0 = -100;
            me->CastCustomSpell(me, 65220, &bp0, 0, 0, true);
        }

        void UpdateAI(const uint32 diff) override {}
        void EnterEvadeMode() override { }
        void EnterCombat(Unit* /*enemy*/) override {}
        void AttackStart(Unit * who) override {}
        void MoveInLineOfSight(Unit* /*who*/) override { }
    };
};

class npc_ds_madness_elementium_bolt : public CreatureScript
{
public:
    npc_ds_madness_elementium_bolt() : CreatureScript("npc_ds_madness_elementium_bolt") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_madness_elementium_boltAI(pCreature);
    }

    struct npc_ds_madness_elementium_boltAI : public ScriptedAI
    {
        npc_ds_madness_elementium_boltAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        TaskScheduler scheduler;
        uint32 elementiumBlastTimer;
        bool canBlast;
        bool platformNozdormuDestroyed;

        void Reset() override
        {
            me->SetFlying(true);
            me->SetSpeed(MOVE_FLIGHT, 5.0f, true);
            me->SetSpeed(MOVE_WALK, 50.0f, true);
            me->SetSpeed(MOVE_RUN, 50.0f, true);
            me->SetReactState(REACT_PASSIVE);
            platformNozdormuDestroyed = false;
            // Slow
            scheduler.Schedule(Milliseconds(2000), [this](TaskContext slow)
            {
                if (platformNozdormuDestroyed == false)
                {
                    if (slow.GetRepeatCounter() == 0)
                    {
                        me->SetSpeed(MOVE_FLIGHT, 0.325f, true);
                        me->SetSpeed(MOVE_WALK, 2.5f, true);
                        me->SetSpeed(MOVE_RUN, 2.5f, true);
                        slow.Repeat(Seconds(7));
                    }
                    else
                    {
                        me->SetSpeed(MOVE_FLIGHT, 5.0f, true);
                        me->SetSpeed(MOVE_WALK, 50.0f, true);
                        me->SetSpeed(MOVE_RUN, 50.0f, true);
                        if (!canBlast)
                        {
                            elementiumBlastTimer = 1000;
                            canBlast = true;
                        }
                    }
                }
                else
                {
                    if (!canBlast)
                    {
                        elementiumBlastTimer = 1500;
                        canBlast = true;
                    }
                }
            });

            canBlast = false;
            me->CastSpell(me, SPELL_ELEMENTIUM_METEOR_AURA, false);
        }

        void SetData(uint32 type, uint32 data) override
        {
            platformNozdormuDestroyed = true;
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (canBlast)
            {
                if (elementiumBlastTimer <= diff)
                {
                    me->CastSpell(me, SPELL_ELEMENTIUM_BLAST, true);
                    elementiumBlastTimer = 5200;
                }
                else elementiumBlastTimer -= diff;
            }
        }
    };
};

class npc_ds_madness_mutated_corruption : public CreatureScript
{
public:
    npc_ds_madness_mutated_corruption() : CreatureScript("npc_ds_madness_mutated_corruption") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_madness_mutated_corruptionAI(pCreature);
    }

    struct npc_ds_madness_mutated_corruptionAI : public ScriptedAI
    {
        npc_ds_madness_mutated_corruptionAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            instance = pCreature->GetInstanceScript();
        }

        InstanceScript* instance;
        TaskScheduler scheduler;
        uint32 crushTimer;
        uint32 impaleTimer;

        inline bool IsCastingAllowed()
        {
            return !me->IsNonMeleeSpellCasted(false);
        }

        void Reset() override
        {
            me->SetFloatValue(UNIT_FIELD_COMBATREACH, 30.0f);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetInCombatWithZone();

            crushTimer = urand(10000, 15000);
            impaleTimer = 12000;
        }

        void JustDied(Unit * /*who*/) override
        {
            if (instance)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (crushTimer <= diff)
            {
                if (IsCastingAllowed())
                {
                    if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 300.0f, true))
                    {
                        me->SetFacingToObject(pTarget);
                        me->CastSpell(pTarget, SPELL_CRUSH, false);
                        me->PlayOneShotAnimKit(1558);
                    }
                    crushTimer = 8000;
                }
            }
            else crushTimer -= diff;

            if (!me->GetVictim())
                return;

            if (impaleTimer <= diff)
            {
                if (IsCastingAllowed())
                {
                    if (Unit* pTarget = SelectTarget(SELECT_TARGET_TOPAGGRO, 0, 300.0f, true))
                    {
                        if (pTarget && pTarget->IsAlive() && pTarget->GetTypeId() == TYPEID_PLAYER)
                        {
                            me->CastSpell(pTarget, SPELL_IMPALE, false);
                        }
                        impaleTimer = 35000;
                        crushTimer += 8000;
                    }
                }
            }
            else impaleTimer -= diff;

            //DoMeleeAttackIfReady();
        }
    };
};

class npc_ds_madness_regenerative_blood : public CreatureScript
{
public:
    npc_ds_madness_regenerative_blood() : CreatureScript("npc_ds_madness_regenerative_blood") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_madness_regenerative_bloodAI(pCreature);
    }

    struct npc_ds_madness_regenerative_bloodAI : public ScriptedAI
    {
        npc_ds_madness_regenerative_bloodAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 energizeTimer;

        void Reset() override
        {
            me->SetReactState(REACT_AGGRESSIVE);
            me->SetInCombatWithZone();
            me->CastSpell(me, SPELL_DEGENERATIVE_BITE, false);
            energizeTimer = 1000;
            me->setPowerType(POWER_ENERGY);
            me->SetMaxPower(POWER_ENERGY, 100);
            me->SetPower(POWER_ENERGY, 0);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (energizeTimer <= diff)
            {
                if (me->FindNearestCreature(NPC_TIME_ZONE, 11.0f, true))
                    me->AddAura(SPELL_TIME_ZONE_AURA_DUMMY - 1, me);

                me->ModifyPower(POWER_ENERGY, 10);
                if (me->GetPower(POWER_ENERGY) >= 100)
                {
                    me->SetHealth(me->GetMaxHealth());
                    me->SetPower(POWER_ENERGY, 0);
                }
                energizeTimer = 1000;
            }
            else energizeTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_ds_madness_corrupting_parasite : public CreatureScript
{
public:
    npc_ds_madness_corrupting_parasite() : CreatureScript("npc_ds_madness_corrupting_parasite") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_madness_corrupting_parasiteAI(pCreature);
    }

    struct npc_ds_madness_corrupting_parasiteAI : public ScriptedAI
    {
        npc_ds_madness_corrupting_parasiteAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        TaskScheduler scheduler;
        uint32 despawnTimer;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);

            scheduler.Schedule(Seconds(1), [this](TaskContext /* unstable corruption */)
            {
                if (me->FindNearestCreature(NPC_TIME_ZONE, 11.0f, true))
                {
                    me->AddAura(SPELL_TIME_ZONE_AURA_DUMMY - 1, me);
                    despawnTimer = 15500;
                }
                else despawnTimer = 10500;

                me->CastSpell(me, SPELL_UNSTABLE_CORRUPTION, false);
                me->DespawnOrUnsummon(despawnTimer);
            });
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);
        }
    };
};

class npc_ds_madness_platform : public CreatureScript
{
public:
    npc_ds_madness_platform() : CreatureScript("npc_ds_madness_platform") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_madness_platformAI(pCreature);
    }

    struct npc_ds_madness_platformAI : public ScriptedAI
    {
        npc_ds_madness_platformAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        void Reset() override
        {
            me->SetVisible(false);
        }
    };
};

class npc_ds_time_zone : public CreatureScript
{
public:
    npc_ds_time_zone() : CreatureScript("npc_ds_time_zone") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_time_zoneAI(pCreature);
    }

    struct npc_ds_time_zoneAI : public ScriptedAI
    {
        npc_ds_time_zoneAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        void Reset() override
        {
            me->CastSpell(me, SPELL_TIME_ZONE_AURA_DUMMY, false);
        }
    };
};

class npc_ds_madness_elementium_terror : public CreatureScript
{
public:
    npc_ds_madness_elementium_terror() : CreatureScript("npc_ds_madness_elementium_terror") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_madness_elementium_terrorAI(pCreature);
    }

    struct npc_ds_madness_elementium_terrorAI : public ScriptedAI
    {
        npc_ds_madness_elementium_terrorAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        void Reset() override
        {
            me->CastSpell(me, SPELL_TETANUS_AURA, false);
        }

        void UpdateAI(const uint32 diff) override
        {
            DoMeleeAttackIfReady();
        }
    };
};

class npc_ds_madness_elementium_fragment : public CreatureScript
{
public:
    npc_ds_madness_elementium_fragment() : CreatureScript("npc_ds_madness_elementium_fragment") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_madness_elementium_fragmentAI(pCreature);
    }

    struct npc_ds_madness_elementium_fragmentAI : public ScriptedAI
    {
        npc_ds_madness_elementium_fragmentAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 shrapnelTimer;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetInCombatWithZone();
            me->CastSpell(me, SPELL_SHRAPNEL_AURA, false);
            shrapnelTimer = urand(0, 8000);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (shrapnelTimer < diff)
            {
                int32 shrapnelAura = 0;
                switch (me->GetMap()->GetDifficulty())
                {
                case RAID_DIFFICULTY_10MAN_HEROIC: shrapnelAura = SPELL_SHRAPNEL_TARGET_10HC; break;
                case RAID_DIFFICULTY_25MAN_NORMAL: shrapnelAura = SPELL_SHRAPNEL_TARGET_25N; break;
                case RAID_DIFFICULTY_25MAN_HEROIC: shrapnelAura = SPELL_SHRAPNEL_TARGET_25HC; break;
                default:/*RAID_DIFFICULTY_10MAN N*/shrapnelAura = SPELL_SHRAPNEL_TARGET; break;
                }
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 300.0f, true, -shrapnelAura))
                {
                    pTarget->CastSpell(pTarget, SPELL_SHRAPNEL_TARGET, true);
                    me->CastSpell(pTarget, SPELL_SHRAPNEL_DMG, false);
                }
                shrapnelTimer = 8000;
            }
            else shrapnelTimer -= diff;
        }
    };
};

class npc_ds_madness_congealing_blood : public CreatureScript
{
public:
    npc_ds_madness_congealing_blood() : CreatureScript("npc_ds_madness_congealing_blood") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_madness_congealing_bloodAI(pCreature);
    }

    struct npc_ds_madness_congealing_bloodAI : public ScriptedAI
    {
        npc_ds_madness_congealing_bloodAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        bool canHeal;

        void Reset() override
        {
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
            canHeal = false;
            me->SetReactState(REACT_PASSIVE);
            me->GetMotionMaster()->MovePoint(0, congealingPosHeal.GetPositionX(), congealingPosHeal.GetPositionY(), congealingPosHeal.GetPositionZ());
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!canHeal)
            {
                if (me->GetDistance2d(congealingPosHeal.GetPositionX(), congealingPosHeal.GetPositionY()) <= 2.0f)
                {
                    canHeal = true;

                    // Heal Deathwing's head and Deathwing as well due to shared health
                    if (Creature* pHead = me->FindNearestCreature(NPC_DEATHWING_HEAD, 300.0f))
                        me->CastSpell(pHead, SPELL_CONGEALING_BLOOD_HEAL, false);
                    if (Creature* pDeathwing = me->FindNearestCreature(BOSS_MADNESS_OF_DEATHWING, 500.0f))
                        me->CastSpell(pDeathwing, SPELL_CONGEALING_BLOOD_HEAL, false);
                    me->DespawnOrUnsummon(1000);
                }
            }
        }
    };
};

class spell_ds_elementium_blast : public SpellScriptLoader
{
public:
    spell_ds_elementium_blast() : SpellScriptLoader("spell_ds_elementium_blast") {}

    class spell_ds_elementium_blast_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_elementium_blast_SpellScript);

        void HandleDamage(SpellEffIndex /*effIndex*/)
        {
            Unit * caster = GetCaster();
            Unit * hit_unit = GetHitUnit();

            if (!caster || !hit_unit)
                return;

            float damage = 0.0f;
            float distance = caster->GetExactDist2d(hit_unit->GetPositionX(), hit_unit->GetPositionY());

            damage = GetHitDamage() / (pow((double)(distance + 1), 0.65));

            SetHitDamage((int32)damage);
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_ds_elementium_blast_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_elementium_blast_SpellScript();
    }
};

class spell_ds_burning_blood : public SpellScriptLoader
{
public:
    spell_ds_burning_blood() : SpellScriptLoader("spell_ds_burning_blood") { }

    class spell_ds_burning_blood_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_ds_burning_blood_AuraScript);

        void OnPeriodic(AuraEffect const* /*aurEff*/)
        {
            if (!GetCaster())
                return;

            uint32 auraId = 0;

            if (GetCaster()->HasAura(SPELL_BURNING_BLOOD))
                auraId = SPELL_BURNING_BLOOD;
            else if (GetCaster()->HasAura(SPELL_BURNING_BLOOD_25))
                auraId = SPELL_BURNING_BLOOD_25;
            else if (GetCaster()->HasAura(SPELL_BURNING_BLOOD_10H))
                auraId = SPELL_BURNING_BLOOD_10H;
            else if (GetCaster()->HasAura(SPELL_BURNING_BLOOD_25H))
                auraId = SPELL_BURNING_BLOOD_25H;

            if (Aura* pCorruptedBlood = GetCaster()->GetAura(auraId))
            {
                uint32 currHealthPct = GetCaster()->GetHealthPct();
                uint32 stackAmount = 100 - currHealthPct;
                pCorruptedBlood->SetStackAmount(stackAmount);
            }
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_ds_burning_blood_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_ds_burning_blood_AuraScript();
    }
};

class spell_ds_burning_blood_dmg : public SpellScriptLoader
{
public:
    spell_ds_burning_blood_dmg() : SpellScriptLoader("spell_ds_burning_blood_dmg") {}

    class spell_ds_burning_blood_dmg_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_burning_blood_dmg_SpellScript);

        void HandleDamage(SpellEffIndex /*effIndex*/)
        {
            Unit * caster = GetCaster();
            Unit * hit_unit = GetHitUnit();
            if (!caster || !hit_unit)
                return;

            float damage = 0.0f;
            uint32 stackAmount = 0;
            uint32 auraId = 0;

            if (GetCaster()->HasAura(SPELL_BURNING_BLOOD))
                auraId = SPELL_BURNING_BLOOD;
            else if (GetCaster()->HasAura(SPELL_BURNING_BLOOD_25))
                auraId = SPELL_BURNING_BLOOD_25;
            else if (GetCaster()->HasAura(SPELL_BURNING_BLOOD_10H))
                auraId = SPELL_BURNING_BLOOD_10H;
            else if (GetCaster()->HasAura(SPELL_BURNING_BLOOD_25H))
                auraId = SPELL_BURNING_BLOOD_25H;

            if (Aura * pBurningBlood = caster->GetAura(auraId))
                stackAmount = pBurningBlood->GetStackAmount();

            damage = GetHitDamage() * stackAmount;

            SetHitDamage((int32)damage);
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_ds_burning_blood_dmg_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_burning_blood_dmg_SpellScript();
    }
};

class spell_ds_corrupted_blood_dmg : public SpellScriptLoader
{
public:
    spell_ds_corrupted_blood_dmg() : SpellScriptLoader("spell_ds_corrupted_blood_dmg") {}

    class spell_ds_corrupted_blood_dmg_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_corrupted_blood_dmg_SpellScript);

        void HandleDamage(SpellEffIndex /*effIndex*/)
        {
            Unit * caster = GetCaster();
            if (!caster)
                return;

            float damage = GetHitDamage();
            float dmg_multiplier = 0.0f;

            switch (GetCaster()->GetMap()->GetDifficulty())
            {
                case RAID_DIFFICULTY_10MAN_HEROIC:
                case RAID_DIFFICULTY_25MAN_HEROIC:
                    dmg_multiplier = 1.6f; break; // Possible nerf here in future
                default:/*RAID_DIFFICULTY_10MAN N*/dmg_multiplier = 1.3f; break;
            }

            if (caster->GetHealthPct() < 21)
                damage = GetHitDamage() * ((21 - caster->GetHealthPct()) * 0.5) * dmg_multiplier;

            SetHitDamage((int32)damage);
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_ds_corrupted_blood_dmg_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_corrupted_blood_dmg_SpellScript();
    }
};

class spell_ds_shrapnel : public SpellScriptLoader
{
public:
    spell_ds_shrapnel() : SpellScriptLoader("spell_ds_shrapnel") { }

    class spell_ds_shrapnel_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_ds_shrapnel_AuraScript);

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* owner = GetOwner()->ToPlayer();
            if (!owner)
                return;

            owner->CastSpell(owner, SPELL_SHRAPNEL_DMG, true);
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_ds_shrapnel_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_ds_shrapnel_AuraScript();
    }
};

class IsNotBlisteringTentacle
{
public:
    bool operator()(WorldObject* object) const
    {
        if (object->ToCreature() && object->ToCreature()->GetEntry() == NPC_BLISTERING_TENTACLE)
            return false;
        return true;
    }
};

class spell_ds_madness_cauterize : public SpellScriptLoader
{
public:
    spell_ds_madness_cauterize() : SpellScriptLoader("spell_ds_madness_cauterize") { }

    class spell_ds_madness_cauterize_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_madness_cauterize_SpellScript);

        void RemoveInvalidTargets(std::list<Unit*>& unitList)
        {
            unitList.remove_if(IsNotBlisteringTentacle());
        }

        void Register()
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_madness_cauterize_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_AREA_ENTRY_SRC);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_madness_cauterize_SpellScript();
    }
};

class IsNotSpellweaverTarget
{
public:
    bool operator()(WorldObject* object) const
    {
        if (object->ToCreature() && object->ToCreature()->GetEntry() == NPC_BLISTERING_TENTACLE)
            return true;
        return false;
    }
};

class spell_ds_madness_spellweave : public SpellScriptLoader
{
public:
    spell_ds_madness_spellweave() : SpellScriptLoader("spell_ds_madness_spellweave") { }

    class spell_ds_madness_spellweave_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_madness_spellweave_SpellScript);

        void RemoveInvalidTargets(std::list<Unit*>& unitList)
        {
            unitList.remove_if(IsNotSpellweaverTarget());
        }

        void Register()
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_madness_spellweave_SpellScript::RemoveInvalidTargets, EFFECT_1, TARGET_UNIT_AREA_ENEMY_DST);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_madness_spellweave_SpellScript();
    }
};

class IsNotMadnessNpc
{
public:
    bool operator()(WorldObject* object) const
    {
        if (object->ToCreature() && /*object->ToCreature()->GetEntry() == NPC_ELEMENTIUM_TERROR || */ (object->ToCreature()->GetEntry() == NPC_REGENERATIVE_BLOOD || object->ToCreature()->GetEntry() == NPC_CORRUPTING_PARASITE))
            return false;
        return true;
    }
};

class spell_ds_time_zone : public SpellScriptLoader
{
public:
    spell_ds_time_zone() : SpellScriptLoader("spell_ds_time_zone") { }

    class spell_ds_time_zone_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_time_zone_SpellScript);

        void RemoveInvalidTargets(std::list<Unit*>& unitList)
        {
            unitList.remove_if(IsNotMadnessNpc());
        }

        void Register()
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_time_zone_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_AREA_ENTRY_SRC);
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_time_zone_SpellScript::RemoveInvalidTargets, EFFECT_1, TARGET_UNIT_AREA_ENTRY_SRC);
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_time_zone_SpellScript::RemoveInvalidTargets, EFFECT_2, TARGET_UNIT_AREA_ENTRY_SRC);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_time_zone_SpellScript();
    }
};

class spell_ds_madness_corrupting_parasite : public SpellScriptLoader
{
public:
    spell_ds_madness_corrupting_parasite() : SpellScriptLoader("spell_ds_madness_corrupting_parasite") { }

    class spell_ds_madness_corrupting_parasite_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_ds_madness_corrupting_parasite_AuraScript);

        void RemoveBuff(AuraEffect const* aurEff, AuraEffectHandleModes mode)
        {
            Unit * pTarget = GetTarget();
            if (!pTarget)
                return;

            if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_DEFAULT || GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
            {
                pTarget->SummonCreature(NPC_CORRUPTING_PARASITE, pTarget->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ(), 0, TEMPSUMMON_DEAD_DESPAWN);
                pTarget->CastSpell(pTarget, SPELL_PARASITIC_BACKSLASH, true);
                pTarget->RemoveAura(SPELL_CORRUPTING_PARASITE_DMG);

                if (Vehicle * veh = pTarget->GetVehicleKit())
                {
                    if (Unit * vehiclePassenger = veh->GetPassenger(0))
                        vehiclePassenger->ToCreature()->DespawnOrUnsummon();
                }
            }
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_ds_madness_corrupting_parasite_AuraScript::RemoveBuff, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_ds_madness_corrupting_parasite_AuraScript();
    }
};

class spell_ds_unstable_corruption : public SpellScriptLoader
{
public:
    spell_ds_unstable_corruption() : SpellScriptLoader("spell_ds_unstable_corruption") {}

    class spell_ds_unstable_corruption_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_unstable_corruption_SpellScript);

        void HandleDamage(SpellEffIndex /*effIndex*/)
        {
            Unit * caster = GetCaster();
            if (!caster)
                return;

            float damage = caster->GetHealth() / 10;

            SetHitDamage((int32)damage);
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_ds_unstable_corruption_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_unstable_corruption_SpellScript();
    }
};

void AddSC_boss_madness_of_deathwing()
{
    new boss_madness_of_deathwing();                // 56173
    new npc_ds_madness_deathwing_head();            // 57962
    new npc_ds_madness_deathwing_limbs();           // 56168, 56846, 56167
    new npc_ds_madness_blistering_tentacle();       // 56188
    new npc_ds_madness_elementium_bolt();           // 56262
    new npc_ds_madness_mutated_corruption();        // 56471
    new npc_ds_madness_platform();                  // 56307
    new npc_ds_time_zone();                         // 56311
    new npc_ds_madness_regenerative_blood();        // 56263
    new npc_ds_madness_elementium_fragment();       // 56724
    new npc_ds_madness_elementium_terror();         // 56710
    new npc_ds_madness_congealing_blood();          // 57798
    new npc_ds_madness_corrupting_parasite();       // 57479

    new spell_ds_elementium_blast();                // 105723, 109600, 109601, 109602
    new spell_ds_burning_blood();                   // 105401, 109616, 109617, 109618
    new spell_ds_burning_blood_dmg();               // 105408, 105412, 105413, 105414
    new spell_ds_corrupted_blood_dmg();             // 106835, 109591, 109595, 109596
    new spell_ds_madness_cauterize();               // 105569, 109576, 109577, 109578
    new spell_ds_time_zone();                       // 105830
    new spell_ds_madness_corrupting_parasite();     // 108649
    new spell_ds_unstable_corruption();             // 108813
    new spell_ds_madness_spellweave();              // 106043, 109609, 109610, 109611
}

//////////////////////////////////////////////////////////////////////////////////////
///////////////////////////         SQL QUERY          ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
/*
--NPC

--SPELL SCRIPTS
insert into `spell_script_names` (`spell_id`, `ScriptName`) values('105401','spell_ds_burning_blood');

insert into `spell_script_names` (`spell_id`, `ScriptName`) values
('105723','spell_ds_elementium_blast'),
('109600','spell_ds_elementium_blast'),
('109601','spell_ds_elementium_blast'),
('109602','spell_ds_elementium_blast');

insert into `spell_script_names` (`spell_id`, `ScriptName`) values
('105408','spell_ds_burning_blood_dmg'),
('109612','spell_ds_burning_blood_dmg'),
('109613','spell_ds_burning_blood_dmg'),
('109614','spell_ds_burning_blood_dmg');

insert into `spell_script_names` (`spell_id`, `ScriptName`) values
('106835','spell_ds_corrupted_blood_dmg'),
('109591','spell_ds_corrupted_blood_dmg'),
('109595','spell_ds_corrupted_blood_dmg'),
('109596','spell_ds_corrupted_blood_dmg');

insert into `spell_script_names` (`spell_id`, `ScriptName`) values
('105569','spell_ds_madness_cauterize'),
('109576','spell_ds_madness_cauterize'),
('109577','spell_ds_madness_cauterize'),
('109578','spell_ds_madness_cauterize');
*/