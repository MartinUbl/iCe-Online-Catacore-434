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
    BOSS_MADNESS_OF_DEATHWING           = 56173,

    NPC_WING_TENTACLE                   = 56168, // 1 & 4
    NPC_ARM_TENTACLE_LEFT               = 56846, // 2
    NPC_ARM_TENTACLE_RIGHT              = 56167, // 3
    NPC_COSMETIC_TENTACLE               = 57693,
    NPC_TAIL_TENTACLE                   = 56844,
    NPC_BLISTERING_TENTACLE             = 56188,
    NPC_PLATFORM                        = 56307,
    NPC_ELEMENTIUM_BOLT                 = 56262,
    NPC_MUTATED_CORRUPTION              = 56471,
    NPC_TIME_ZONE                       = 56311,
    NPC_REGENERAIVE_BLOOD               = 56263,


    NPC_CRUSH_TARGET                    = 56581,

    NPC_HEMORRHAGE_TARGET               = 56359,
    NPC_CLAWK_MARK                      = 56545,
    NPC_CORRUPTING_PARASITE             = 57479,

    NPC_IMPALING_TENTACLE               = 56724,
    NPC_ELEMENTIUM_TERROR               = 56710,
    NPC_CONGEALING_BLOOD                = 57798,

    NPC_DEATHWING_1                     = 57962, // invisible ?
};

// Spells
enum Spells
{
    SPELL_IDLE                          = 106187, // tail tentacle has it
    SPELL_TRIGGER_ASPECT_BUFFS          = 106943, // casted by deathwing 56173
    SPELL_SHARE_HEALTH_1                = 109547, // casted by deathwing 56173 on self ?
    SPELL_SHARE_HEALTH_2                = 109548, // casted by deathwing 56173 on 57962
    SPELL_ASSAULT_ASPECTS               = 107018, // casted by deathwing 56173
    SPELL_ELEMENTIUM_BOLT               = 105651,
    SPELL_ELEMENTIUM_METEOR_SCRIPT      = 105599,

    SPELL_ELEMENTIUM_BLAST              = 105723,
    //SPELL_ELEMENTIUM_METEOR_TARGET      = 106242, // target mark
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

    SPELL_CORRUPTED_BLOOD               = 106834,
    SPELL_CORRUPTED_BLOOD_25            = 109592,
    SPELL_CORRUPTED_BLOOD_10H           = 109593,
    SPELL_CORRUPTED_BLOOD_25H           = 109594,
    SPELL_CORRUPTED_BLOOD_STACKER       = 106843,

    SPELL_CORRUPTING_PARASITE_AOE       = 108597,
    SPELL_CORRUPTING_PARASITE_DMG       = 108601,
    SPELL_CORRUPTING_PARASITE_AURA      = 108649,
    SPELL_PARASITIC_BACKSLASH           = 108787,
    SPELL_UNSTABLE_CORRUPTION           = 108813,
    SPELL_DEATH                         = 110101,
    SPELL_BERSERK                       = 64238,
    SPELL_ACHIEVEMENT                   = 111533,

    SPELL_TIME_ZONE_MISSILE             = 105799,
    SPELL_TIME_ZONE_AURA_DUMMY          = 105831, // aura (debuff)
    SPELL_TIME_ZONE_AURA_2              = 108646, // for parasite
    SPELL_ENTER_THE_DREAM               = 106464,

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
    SPELL_SUMMON_TAIL_FORCE             = 106239,
    SPELL_SUMMON_TAIL                   = 106240,
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
    SPELL_SHRAPNEL_DMG                  = 106791,

    SPELL_CONCENTRATION_ALEXSTRASZA     = 106641,
    SPELL_CONCENTRATION_NOZDORMU        = 106642,
    SPELL_CONCENTRATION_YSERA           = 106643,
    SPELL_CONCENTRATION_KALECGOS        = 106644,
    SPELL_CONCENTRATION_ALEXSTRASZA_END = 110071,
    SPELL_CONCENTRATION_NOZDORMU_END    = 110072,
    SPELL_CONCENTRATION_YSERA_END       = 110076,
    SPELL_CONCENTRATION_KALECGOS_END    = 110077,

    SPELL_ALEXSTRASZA_PRESENCE          = 105825,
    SPELL_NOZDORMU_PRESENCE             = 105823,
    SPELL_YSERA_PRESENCE                = 106456,
    SPELL_KALECGOS_PRESENCE             = 106026,

    SPELL_EXPOSE_WEAKNESS_ALEXSTRASZA   = 106588,
    SPELL_EXPOSE_WEAKNESS_NOZDORMU      = 106600,
    SPELL_EXPOSE_WEAKNESS_YSERA         = 106613,
    SPELL_EXPOSE_WEAKNESS_KALECGOS      = 106624,
};

enum MadnesGameobjects
{
    GO_ELEMENTIUM_FRAGMENT_10N      = 210217,
    GO_ELEMENTIUM_FRAGMENT_25N      = 210218,
    GO_ELEMENTIUM_FRAGMENT_10HC     = 210219,
    GO_ELEMENTIUM_FRAGMENT_25HC     = 210220,
};

enum Platforms
{
    PLATFORM_ALEXSTRASZA            = 0,
    PLATFORM_NOZDORMU               = 1,
    PLATFORM_YSERA                  = 2,
    PLATFORM_KALECGOS               = 3,
    // Summon Elementium Bolt Position
    ELEMENTIUM_BOLT                 = 4,
};

enum MadnessActions
{
    ACTION_SET_PLATFORM             = 0,
    ACTION_LIMB_DIED                = 1,
    ACTION_TELEPORT_HOME            = 5,
    ACTION_CAUTERIZE                = 6,
};

enum Phase
{
    PHASE_LIMBS                     = 0,
    PHASE_SLUMB                     = 1,
    PHASE_HEAD_DOWN                 = 2,
};

const Position limbsPos[4] =
{
    { -11941.2f, 12248.9f, 12.1f, 1.98f },
    { -12005.8f, 12190.3f, -6.5f, 2.12f },
    { -12065.0f, 12127.2f, -3.2f, 2.33f },
    { -12097.8f, 12067.4f, 13.4f, 2.21f },
};

const Position platformPos[4] =
{
    { -11957.2f, 12267.9f,  1.2f, 5.07f },
    { -12039.8f, 12224.3f, -6.1f, 5.32f },
    { -12099.0f, 12161.2f, -2.7f, 5.77f },
    { -12128.8f, 12077.4f,  2.3f, 2.21f },
};

const Position boltPos[5] =
{
    { -11961.2f, 12286.0f,  1.3f, 0.0f }, // Alexstrasza land position
    { -12055.0f, 12239.0f, -6.1f, 0.0f }, // Nozdormu land position
    { -12112.8f, 12170.2f, -2.7f, 0.0f }, // Ysera land position
    { -12149.8f, 12081.4f,  2.3f, 0.0f }, // Kalecgos land position
    { -11929.8f, 12035.6f, 35.4f, 0.0f }, // Summon Location
};

const Position mutatedcorruptionPos[4] =
{
    { -11993.3f, 12286.3f, -2.5f, 5.91f }, // Alexstrasza platform
    { -12028.8f, 12265.6f, -6.2f, 4.13f }, // Nozdormu platform
    { -12107.4f, 12201.9f, -5.3f, 5.16f }, // Ysera platform
    { -12160.9f, 12057.0f,  2.4f, 0.73f }  // Kalecgos platform
};

const Position hemorrhagePos[4] =
{
    { -11955.9f, 12281.7f,  1.30f, 0.0f },
    { -12048.0f, 12237.6f, -6.14f, 0.0f },
    { -12113.9f, 12166.7f, -2.72f, 0.0f },
    { -12146.3f, 12093.5f,  2.31f, 0.0f },
};

//const Position blisteringPos[4] =
//{
//    {-11942.116211f, 12249.538086f, 1.37f, 0.0f},
//    {-12025.414063f, 12213.312500f, -6.14f, 0.0f},
//    {-12084.824219f, 12146.507813f, -2.72f, 0.0f},
//    {-12102.188477f, 12067.497070f, 2.31f, 0.0f},
//};

const Position impalingPos[8] =
{
    { -12117.3f, 12185.7f, -2.7f, 0.0f },
    { -12115.9f, 12177.0f, -2.7f, 0.0f },
    { -12122.9f, 12176.0f, -2.7f, 0.0f },
    { -12120.6f, 12170.0f, -2.7f, 0.0f },
    { -12115.1f, 12168.3f, -2.7f, 0.0f },
    { -12120.2f, 12165.8f, -2.7f, 0.0f },
    { -12113.3f, 12165.1f, -2.7f, 0.0f },
    { -12112.7f, 12172.5f, -2.7f, 0.0f }
};

const Position terrorPos[2] =
{
    { -12121.2f, 12162.7f, -2.7f, 0.0f },
    { -12117.7f, 12168.8f, -2.7f, 0.0f }
};

const Position congealingPos[2] =
{
    { -12119.4f, 12162.6f, -2.7f, 0.0f },
    { -12079.5f, 12169.6f, -2.7f, 0.0f } // healing pos
};

const Position deathwingPos         = { -11903.9f, 11989.1f, -113.2f, 2.164f };
const Position deathwing2Pos        = { -12063.5f, 12198.9f,  -13.0f, 2.164f };

const Position alexstraszaendPos    = { -12077.3f, 12152.3f, -2.6f, 6.00f };
const Position nozdormuendPos       = { -12078.4f, 12147.5f, -2.6f, 0.17f };
const Position yseraendPos          = { -12073.8f, 12156.6f, -2.6f, 5.55f };
const Position kalecgosendPos       = { -12069.2f, 12159.9f, -2.6f, 5.23f };
const Position aggraendPos          = { -12066.1f, 12150.4f, -2.6f, 3.05f };
const Position thrallendPos         = { -12067.7f, 12146.4f, -2.6f, 3.05f };
const Position cacheSpawnPos        = { -12076.6f, 12169.9f, -2.5f, 3.54f };

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
        boss_madness_of_deathwingAI(Creature *creature) : ScriptedAI(creature), Summons(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        TaskScheduler scheduler;
        SummonList Summons;
        uint64 limbsGuids[4];
        uint64 platformGuids[4];
        uint32 assaultAspectsTimer;
        uint32 elementiumBoltTimer;
        uint32 cataclysmTimer;
        uint8 platformKilled;
        uint8 maxPlayersOnPlatform;
        uint8 phase;
        bool newAssault;
        bool canCataclysm;
        bool platformDestroyed[4];

        void Reset() override
        {
            if (instance)
            {
                if (instance->GetData(TYPE_BOSS_MADNESS_OF_DEATHWING) != DONE)
                    instance->SetData(TYPE_BOSS_MADNESS_OF_DEATHWING, NOT_STARTED);

                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_ENTER_THE_DREAM);
                if (Creature * pThrall = me->FindNearestCreature(NPC_THRALL_MADNESS_START, 300.0f, true))
                    pThrall->AI()->DoAction(ACTION_TELEPORT_HOME);
            }

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetVisible(false);
            me->RemoveAura(SPELL_SLUMP);

            Summons.DespawnAll();
            assaultAspectsTimer = 5000;
            newAssault = true;
            canCataclysm = false;
            platformKilled = 0;
            maxPlayersOnPlatform = 0;
            phase = PHASE_LIMBS;

            for (uint8 i = 0; i < 4; i++)
            {
                platformDestroyed[i] = false;
            }

            std::list<Creature*> time_zone;
            GetCreatureListWithEntryInGrid(time_zone, me, NPC_TIME_ZONE, 500.0f);
            for (std::list<Creature*>::const_iterator itr = time_zone.begin(); itr != time_zone.end(); ++itr)
            {
                (*itr)->DespawnOrUnsummon();
            }

            scheduler.CancelAll();
        }

        void JustSummoned(Creature* summon) override
        {
            Summons.push_back(summon->GetGUID());
        }

        void SummonedCreatureDies(Creature* unit, Unit* /*killer*/) override
        {
            if (unit->GetEntry() == NPC_WING_TENTACLE || unit->GetEntry() == NPC_ARM_TENTACLE_LEFT || unit->GetEntry() == NPC_ARM_TENTACLE_RIGHT)
            {
                for (uint32 i = 0; i < 4; i++)
                {
                    if (unit->GetGUID() == limbsGuids[i])
                    {
                        platformDestroyed[i] = true;
                        uint32 aspectsEntry = 0;
                        uint32 spellId = 0;
                        uint32 concentrationId = 0;

                        switch (i)
                        {
                        case PLATFORM_ALEXSTRASZA:
                            aspectsEntry = NPC_ALEXSTRASZA_DRAGON_FORM;
                            spellId = SPELL_ALEXSTRASZA_PRESENCE;
                            concentrationId = SPELL_CONCENTRATION_ALEXSTRASZA;
                            break;
                        case PLATFORM_NOZDORMU:
                            aspectsEntry = NPC_NOZDORMU_DRAGON_FORM;
                            spellId = SPELL_NOZDORMU_PRESENCE;
                            concentrationId = SPELL_CONCENTRATION_NOZDORMU;
                            break;
                        case PLATFORM_YSERA:
                            aspectsEntry = NPC_YSERA_DRAGON_FORM;
                            spellId = SPELL_YSERA_PRESENCE;
                            concentrationId = SPELL_CONCENTRATION_YSERA;
                            break;
                        case PLATFORM_KALECGOS:
                            aspectsEntry = NPC_KALECGOS_DRAGON_FORM;
                            spellId = SPELL_KALECGOS_PRESENCE;
                            concentrationId = SPELL_CONCENTRATION_KALECGOS;
                            break;
                        default:
                            break;
                        }

                        if (Creature * pAspect = me->FindNearestCreature(aspectsEntry, 500.0f, true))
                        {
                            pAspect->RemoveAurasDueToSpell(spellId);
                            pAspect->CastSpell(me, concentrationId, true);
                        }
                    }
                }
            }
        }

        void EnterCombat(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_MADNESS_OF_DEATHWING, IN_PROGRESS);
            }

            me->SendPlaySpellVisualKit(22445, 0);
            me->SetVisible(true);

            for (uint8 i = 0; i < 4; i++)
            {
                uint32 limbsEntryId = 0;
                switch (i)
                {
                case 0:
                case 3:
                    limbsEntryId = NPC_WING_TENTACLE;
                    break;
                case 1:
                    limbsEntryId = NPC_ARM_TENTACLE_LEFT;
                    break;
                case 2:
                    limbsEntryId = NPC_ARM_TENTACLE_RIGHT;
                    break;
                default:
                    break;
                }

                if (Creature* pLimb = me->SummonCreature(limbsEntryId, limbsPos[i]))
                {
                    pLimb->SendPlaySpellVisualKit(22445, 0);
                    limbsGuids[i] = pLimb->GetGUID();
                }

                if (Creature* pPlatform = me->SummonCreature(NPC_PLATFORM, platformPos[i]))
                    platformGuids[i] = pPlatform->GetGUID();
            }

            me->MonsterYell("You have done NOTHING. I will tear your world APART.", LANG_UNIVERSAL, 0);
            me->SendPlaySound(26527, false);
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

        void ElementiumBoltYell()
        {
            uint32 randYell = urand(0, 2);
            switch (randYell) {
            case 0:
                me->MonsterYell("There's no shelter from my fury.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(26354, false);
                break;
            case 1:
                me->MonsterYell("Your armor means nothing, your faith even less.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(26355, false);
                break;
            case 2:
                me->MonsterYell("The sea will swallow your smoldering remains.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(26356, false);
                break;
            default:
                break;
            }
        }

        void KilledUnit(Unit* victim) override {}

        void JustDied(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_MADNESS_OF_DEATHWING, DONE);
            }

            // Outro
            PlayMovieToPlayers(76);
            me->SummonCreature(NPC_ALEXSTRASZA_MADNESS, alexstraszaendPos, TEMPSUMMON_MANUAL_DESPAWN);
            me->SummonCreature(NPC_YSERA_MADNESS, yseraendPos, TEMPSUMMON_MANUAL_DESPAWN);
            me->SummonCreature(NPC_KALECGOS_MADNESS, kalecgosendPos, TEMPSUMMON_MANUAL_DESPAWN);
            me->SummonCreature(NPC_NOZDORMU_MADNESS, nozdormuendPos, TEMPSUMMON_MANUAL_DESPAWN);
            me->SummonCreature(NPC_THRALL_MADNESS, thrallendPos, TEMPSUMMON_MANUAL_DESPAWN);

            SummonCache();
            scheduler.CancelAll();
        }

        void DamageTaken(Unit* who, uint32& damage)
        {
            if (me->GetHealth() <= damage)
            {
                damage = 0;
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->RemoveAura(SPELL_SLUMP);
                me->CastSpell(me, SPELL_SLUMP_2, false);
            }
        }

        void DoAction(const int32 action) override
        {
            if (instance)
            {
                switch (action)
                {
                case ACTION_LIMB_DIED:
                    platformKilled++;
                    if (platformKilled == 4)
                        phase = PHASE_SLUMB;
                    me->DealDamage(me, me->GetMaxHealth()*0.2); // Decrease hp by 20%
                    scheduler.CancelGroup(1); // Cancel Elementium Meteor
                    assaultAspectsTimer = 15000;
                    newAssault = true;
                    canCataclysm = false;
                    me->InterruptNonMeleeSpells(false);
                    break;
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

            uint8 alexstraszaPlatform = 0;
            uint8 nozdormuPlatform = 0;
            uint8 yseraPlatform = 0;
            uint8 kalecgosPlatform = 0;
            uint64 platformGuid = 0;

            for (Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
            {
                if (Player* pl = itr->getSource())
                {
                    if (pl && pl->IsInWorld() && pl->IsAlive() && !pl->IsGameMaster())
                    {
                        if (Creature * pPlatform = pl->FindNearestCreature(NPC_PLATFORM, 300.0f, true))
                            platformGuid = pPlatform->GetGUID();
                        
                        if (platformGuid == platformGuids[PLATFORM_ALEXSTRASZA])
                            alexstraszaPlatform++;
                        else if (platformGuid == platformGuids[PLATFORM_NOZDORMU])
                            nozdormuPlatform++;
                        else if (platformGuid == platformGuids[PLATFORM_YSERA])
                            yseraPlatform++;
                        else if (platformGuid == platformGuids[PLATFORM_KALECGOS])
                            kalecgosPlatform++;
                    }
                }
            }

            if (alexstraszaPlatform > nozdormuPlatform && alexstraszaPlatform > yseraPlatform && alexstraszaPlatform > kalecgosPlatform)
                maxPlayersOnPlatform = PLATFORM_ALEXSTRASZA;
            else if (nozdormuPlatform > alexstraszaPlatform && nozdormuPlatform > yseraPlatform && nozdormuPlatform > kalecgosPlatform)
                maxPlayersOnPlatform = PLATFORM_NOZDORMU;
            else if (yseraPlatform > alexstraszaPlatform && yseraPlatform > nozdormuPlatform && yseraPlatform > kalecgosPlatform)
                maxPlayersOnPlatform = PLATFORM_YSERA;
            else maxPlayersOnPlatform = PLATFORM_KALECGOS;
        }

        void AssaultAspect()
        {
            me->MonsterTextEmote("Deathwing begins to cast assault aspects!", me->GetGUID(), true, 300.0f);
            me->CastSpell(me, SPELL_ASSAULT_ASPECTS, false);
            CheckPlayersPosition();

            if (platformDestroyed[maxPlayersOnPlatform] == true)
            {
                std::vector<int> freePlatforms;
                for (int i = 0; i < 4; i++)
                {
                    if (platformDestroyed[i] == false) // Avoid new assaulted platform to be the same as the killed one
                        freePlatforms.push_back(i);
                }
                maxPlayersOnPlatform = freePlatforms[urand(0, freePlatforms.size() - 1)];
            }

            scheduler.Schedule(Seconds(10), [this](TaskContext /* Task context */)
            {
                const char * text;
                const char * aspectYell;
                uint32 npcId = 0;
                uint32 soundId = 0;

                switch (maxPlayersOnPlatform)
                {
                case PLATFORM_ALEXSTRASZA:
                    npcId = NPC_ALEXSTRASZA_DRAGON_FORM;
                    text = "Deathwing assaults Alexstrasza";
                    aspectYell = "I will cleanse whatever corruption I can; my fire will not harm you.";
                    soundId = 26498;
                    break;
                case PLATFORM_NOZDORMU:
                    npcId = NPC_NOZDORMU_DRAGON_FORM;
                    text = "Deathwing assaults Nozdormu";
                    aspectYell = "I will slow the Destroyer's attacks when I can.";
                    soundId = 25949;
                    break;
                case PLATFORM_YSERA:
                    npcId = NPC_YSERA_DRAGON_FORM;
                    text = "Deathwing assaults Ysera";
                    aspectYell = "I will bring you closer to the Emerald Dream. Seek safety there when the fight becomes too intense.";
                    soundId = 26142;
                    break;
                case PLATFORM_KALECGOS:
                    npcId = NPC_KALECGOS_DRAGON_FORM;
                    text = "Deathwing assaults Kalecgos";
                    aspectYell = "I will charge you with arcane energy to blast your foes.";
                    soundId = 26259;
                    break;
                default:
                    break;
                }
                me->MonsterTextEmote(text, me->GetGUID(), true, 300.0f);
                if (Creature * pAspect = me->FindNearestCreature(npcId, 500.0f, true))
                {
                    pAspect->MonsterYell(aspectYell, LANG_UNIVERSAL, 0);
                    pAspect->SendPlaySound(soundId, true);
                }

                // Spawn Mutated Corruption
                me->SummonCreature(NPC_MUTATED_CORRUPTION, mutatedcorruptionPos[maxPlayersOnPlatform], TEMPSUMMON_MANUAL_DESPAWN);
                // Activate Burning Blood on Limb tentacle
                if (Creature * pLimb = ObjectAccessor::GetObjectInMap(limbsGuids[maxPlayersOnPlatform], me->GetMap(), (Creature*)NULL))
                    pLimb->CastSpell(pLimb, SPELL_BURNING_BLOOD, false);
                // Time Zone
                if (Creature * pPlatform = ObjectAccessor::GetObjectInMap(platformGuids[maxPlayersOnPlatform], me->GetMap(), (Creature*)NULL))
                {
                    if (Creature * pNozdormu = pPlatform->FindNearestCreature(NPC_NOZDORMU_DRAGON_FORM, 300.0f, true))
                        pNozdormu->CastSpell(pPlatform, SPELL_TIME_ZONE_MISSILE, false);
                }
                // Elementium Bolt
                scheduler.Schedule(Seconds(55), 1, [this](TaskContext elementiumBolt)
                {
                    if (Creature * pElementiumBolt = me->SummonCreature(NPC_ELEMENTIUM_BOLT, boltPos[ELEMENTIUM_BOLT], TEMPSUMMON_DEAD_DESPAWN))
                        pElementiumBolt->GetMotionMaster()->MovePoint(0, boltPos[maxPlayersOnPlatform], true);
                    ElementiumBoltYell();
                });
                // Hemorrhage
                scheduler.Schedule(Seconds(90), 1, [this](TaskContext hemorrhage)
                {
                    if (hemorrhage.GetRepeatCounter() < 6)
                    {
                        if (Creature * pLimb = ObjectAccessor::GetObjectInMap(limbsGuids[maxPlayersOnPlatform], me->GetMap(), (Creature*)NULL))
                        {
                            pLimb->SummonCreature(NPC_REGENERAIVE_BLOOD, hemorrhagePos[maxPlayersOnPlatform], TEMPSUMMON_MANUAL_DESPAWN);
                        }
                        hemorrhage.Repeat(Seconds(1));
                    }
                });

                cataclysmTimer = 120000;
                canCataclysm = true;
            });
        }

        void ExposeWeakness()
        {
            if (Creature * pLimb = ObjectAccessor::GetObjectInMap(limbsGuids[maxPlayersOnPlatform], me->GetMap(), (Creature*)NULL))
            {
                switch (maxPlayersOnPlatform)
                {
                case PLATFORM_ALEXSTRASZA:
                    if (Creature* pAspect = me->FindNearestCreature(NPC_ALEXSTRASZA_DRAGON_FORM, 500.0f, true))
                    {
                        pAspect->MonsterYell("No! Such power! Deathwing's summoning of the final Cataclysm will destroy all life on Azeroth. Quickly, we must interrupt him!", LANG_UNIVERSAL, false);
                        me->SendPlaySound(26500, false);
                        pAspect->CastSpell(pLimb, SPELL_EXPOSE_WEAKNESS_ALEXSTRASZA, false);
                    }
                    break;
                case PLATFORM_NOZDORMU:
                    if (Creature* pAspect = me->FindNearestCreature(NPC_NOZDORMU_DRAGON_FORM, 500.0f, true))
                    {
                        pAspect->MonsterYell("Hurry, heroes. In mere moments Deathwing's Cataclysm will complete what he begun and end the world. Join me in the attack, now!", LANG_UNIVERSAL, false);
                        me->SendPlaySound(25951, false);
                        pAspect->CastSpell(pLimb, SPELL_EXPOSE_WEAKNESS_NOZDORMU, false);
                    }
                    break;
                case PLATFORM_YSERA:
                    if (Creature* pAspect = me->FindNearestCreature(NPC_YSERA_DRAGON_FORM, 500.0f, true))
                    {
                        pAspect->MonsterYell("Deathwing is conjuring the final Cataclysm; even the Emerald Dream trembles. If we are to stop the spell, we must attack him together.", LANG_UNIVERSAL, false);
                        me->SendPlaySound(26144, false);
                        pAspect->CastSpell(pLimb, SPELL_EXPOSE_WEAKNESS_YSERA, false);
                    }
                    break;
                case PLATFORM_KALECGOS:
                    if (Creature* pAspect = me->FindNearestCreature(NPC_KALECGOS_DRAGON_FORM, 500.0f, true))
                    {
                        pAspect->MonsterYell("The Destroyer is gathering all his might for a blow that will split the world. Attack him, now! We must stop the final Cataclysm!", LANG_UNIVERSAL, false);
                        me->SendPlaySound(26261, false);
                        pAspect->CastSpell(pLimb, SPELL_EXPOSE_WEAKNESS_KALECGOS, false);
                    }
                    break;
                default:
                    break;
                }
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (!UpdateVictim())
                return;

            if (phase == PHASE_LIMBS)
            {
                if (newAssault)
                {
                    if (assaultAspectsTimer <= diff)
                    {
                        AssaultAspect();
                        newAssault = false;
                    }
                    else assaultAspectsTimer -= diff;
                }

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
                me->CastSpell(me, SPELL_SLUMP, false);
                me->CastSpell(me, SPELL_CORRUPTED_BLOOD, false);
                me->MonsterYell("I AM DEATHWING, THE DESTROYER, THE END OF ALL THINGS, INEVITABLE, INDOMITABLE; I AM THE CATACLYSM!", LANG_UNIVERSAL, 0);
                me->SendPlaySound(26353, true);
                phase = PHASE_HEAD_DOWN;
            }
            else if (phase == PHASE_HEAD_DOWN)
            {
                
            }
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
        npc_ds_madness_deathwing_limbsAI(Creature* pCreature) : ScriptedAI(pCreature), Summons(pCreature) {}

        SummonList Summons;
        uint32 platformPosition;
        uint32 checkTimer;
        bool spawnBlisteringTentacles;
        bool secondSpawn;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetInCombatWithZone();
            me->SetReactState(REACT_PASSIVE);
            spawnBlisteringTentacles = false;
            secondSpawn = false;
            checkTimer = 1000;

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

                for (uint8 i = 0; i < seats; i++)
                {
                    if (Creature * vehiclePassenger = me->SummonCreature(NPC_BLISTERING_TENTACLE, 0, 0, 0))
                    {
                        vehiclePassenger->EnterVehicle(me, i);
                        vehiclePassenger->ClearUnitState(UNIT_STATE_UNATTACKABLE);
                        vehiclePassenger->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    }
                }
            }

            if (Creature * pAlexstrasza = me->FindNearestCreature(NPC_ALEXSTRASZA_DRAGON_FORM, 300.0f, true))
            {
                pAlexstrasza->CastSpell(me, 105565, false);
                //pAlexstrasza->AI()->DoAction(ACTION_CAUTERIZE);
            }
        }

        void JustDied(Unit * /*who*/) override
        {
            if (Creature * pDeathwing = me->FindNearestCreature(BOSS_MADNESS_OF_DEATHWING, 300.0f, true))
                pDeathwing->AI()->DoAction(ACTION_LIMB_DIED);

            if (Creature * pTimeZone = me->FindNearestCreature(NPC_TIME_ZONE, 100.0f, true))
                pTimeZone->DespawnOrUnsummon();

            Summons.DespawnAll();
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!spawnBlisteringTentacles)
            {
                if (!secondSpawn)
                {
                    if (me->GetHealthPct() <= 70)
                    {
                        SpawnBlisteringTentacles();
                        secondSpawn = true;
                    }
                }
                else if (me->GetHealthPct() <= 40)
                {
                    SpawnBlisteringTentacles();
                    spawnBlisteringTentacles = true;
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

    struct npc_ds_madness_blistering_tentacleAI : public ScriptedAI
    {
        npc_ds_madness_blistering_tentacleAI(Creature* pCreature) : ScriptedAI(pCreature) 
        {
            me->AddAura(SPELL_BLISTERING_HEAT, me);
        }

        void Reset() override
        {
            me->SetInCombatWithZone();
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->CastSpell(me, SPELL_BLISTERING_HEAT, false);
        }

        void UpdateAI(const uint32 diff) override {}
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

        uint32 elementiumBlastTimer;
        bool canBlast;
        bool isSlow;

        void Reset() override
        {
            me->SetFlying(true);
            me->SetSpeed(MOVE_FLIGHT, 5.0f, true);
            me->SetSpeed(MOVE_WALK, 50.0f, true);
            me->SetSpeed(MOVE_RUN, 50.0f, true);
            me->SetReactState(REACT_PASSIVE);

            isSlow = false;
            canBlast = false;
            me->CastSpell(me, SPELL_ELEMENTIUM_METEOR_AURA, false);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!isSlow && me->FindNearestCreature(NPC_TIME_ZONE, 10.0f, true))
            {
                isSlow = true;
                me->SetSpeed(MOVE_FLIGHT, 0.225f, true);
                me->SetSpeed(MOVE_WALK, 2.5f, true);
                me->SetSpeed(MOVE_RUN, 2.5f, true);
            }
            else if (isSlow && !me->FindNearestCreature(NPC_PLATFORM, 10.0f, true))
            {
                me->SetSpeed(MOVE_FLIGHT, 5.0f, true);
                me->SetSpeed(MOVE_WALK, 50.0f, true);
                me->SetSpeed(MOVE_RUN, 50.0f, true);
                if (!canBlast)
                {
                    elementiumBlastTimer = 500;
                    canBlast = true;
                }
            }

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
        npc_ds_madness_mutated_corruptionAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 crushTimer;
        uint32 impaleTimer;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetInCombatWithZone();

            crushTimer = urand(10000, 15000);
            impaleTimer = 12000;
        }

        void UpdateAI(const uint32 diff) override
        {
            if (crushTimer <= diff)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                {
                    me->SetFacingToObject(pTarget);
                    me->CastSpell(pTarget, SPELL_CRUSH, false);
                }
                crushTimer = 8000;
            }
            else crushTimer -= diff;

            if (impaleTimer <= diff)
            {
                Unit* pTarget = me->GetVictim();
                if (!me->IsWithinMeleeRange(pTarget))
                {
                    Unit* pNearest = SelectTarget(SELECT_TARGET_NEAREST, 0, 50.0f, true);
                    if (pNearest)
                        pTarget = pNearest;
                }
                me->CastSpell(pTarget, SPELL_IMPALE, false);
                impaleTimer = 35000;
            }
            else impaleTimer -= diff;
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
            shrapnelTimer = 8000;
        }

        void UpdateAI(const uint32 diff) override
        {
            if (shrapnelTimer)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                    me->CastSpell(pTarget, SPELL_SHRAPNEL_TARGET, false);
                shrapnelTimer = 8000;
            }
            else shrapnelTimer -= diff;

            DoMeleeAttackIfReady();
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

            if (Aura* pCorruptedBlood = GetCaster()->GetAura(SPELL_BURNING_BLOOD))
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

            if (Aura * pBurningBlood = caster->GetAura(SPELL_BURNING_BLOOD))
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
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_madness_cauterize_SpellScript::RemoveInvalidTargets, EFFECT_1, TARGET_UNIT_AREA_ENTRY_SRC);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_madness_cauterize_SpellScript();
    }
};

void AddSC_boss_madness_of_deathwing()
{
    new boss_madness_of_deathwing();
    new npc_ds_madness_deathwing_limbs();
    new npc_ds_madness_blistering_tentacle();
    new npc_ds_madness_elementium_bolt();
    new npc_ds_madness_mutated_corruption();
    new npc_ds_madness_platform();
    new npc_ds_time_zone();
    new npc_ds_madness_regenerative_blood();
    new npc_ds_madness_elementium_fragment();
    new npc_ds_madness_elementium_terror();

    new spell_ds_elementium_blast();
    new spell_ds_burning_blood();
    new spell_ds_burning_blood_dmg();
    new spell_ds_madness_cauterize();
}