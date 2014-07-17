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
#include "firelands.h"
#include "float.h"
#include "MapManager.h"

enum Spells
{
    LAVA_FIRE_RING          = 99870,
    BURNING_WOUND           = 99401, // In DB seems that it should proc from melee attacks, but that's incorrect
    BURNING_WOUND_DOT       = 99399, // If player have this dot, melee attacks should damage player by Burning blast
    SUBMERGE_STATE_AURA     = 96725,
    BERSERK                 = 47008,
    BURROW_SULFURAS         = 98982,
    MAGMA                   = 99907,
    EMERGE                  = 81948,

    //PHASE 1
    SS_LAVA_POOLS           = 98712,
    SULFURUS_SMASH          = 98710,
    SULFURUSH_DMG           = 98708,
    LAVA_WAVE               = 98873,
    MAGMA_BLAST             = 98313, // If no one in melee range
    WRATH_OF_RAGNAROS       = 98263,
    HAND_OF_RAGNAROS        = 98237,
    MAGMA_TRAP_ERUPTION     = 98175,
    MAGMA_TRAP_BURNING      = 98179, // visual only
    MAGMA_TRAP_MARK_MISSILE = 98164,
    MAGMA_TRAP_VULNERABILITY= 100238,
    SUMMON_WAVE_N           = 98874,
    SUMMON_WAVE_E           = 98875,
    SUMMON_WAVE_W           = 98876,

    // PHASE 2
    SULFURUS_HAMMER_DMG     = 100456, // 45 second dmg aura
    SCORCHED_GROUND_DMG     = 100118, // correct aoe
    SPLITTING_BLOW          = 98951,
    INVOKE_SONS             = 99050, // visual lava spilled at son of flame birth location ( single target)
    BURNING_SPEED           = 99414,
    FLAME_PILLAR_TRANSFORM  = 98983,
    LAVA_BOLT               = 98981,
    SUPERNOVA               = 99112,
    EMGULFING_FLAMES        = 99172,
    ENGULFING_FLAMES_HC     = 99236,

    // PHASE 3

    ENGULFING_FLAME_VISUAL  = 99216,
    ENGULFING_FLAME_DMG     = 99225,
    MOLTEN_SEED             = 100888,
    MOLTEN_SEED_MISSILE     = 98495,
    MOLTEN_INFERNO          = 98518,
    MOLTEN_POWER            = 100157,
    BLAZING_HEAT_DMG_HEAL   = 99128,
    BLAZING_HEAT_SIGNALIZER = 100460,

    // PHASE 4
    LIVING_METEOR_MISSILE   = 99268, // + Spawn meteor at target location
    METEOR_DMG_REDUCTION    = 100904,
    COMBUSTION              = 99303,
    COMBUSTIBLE             = 99296,
    FIXATE                  = 99849,
    MATEOR_EXPLOSION        = 99287,
    DUMMY_AOE               = 99269, // Original for meteor explosion

    //HEROIC ABILITIES
    LEGS_HEAL                   = 100346, // 40% heal
    WORLD_IN_FLAMES             = 100171, // trigerrs 99171 every 3 seconds
    WORLD_IN_FLAMES_TRIGGERED   = 99171,
    MAGMA_GEYSER                = 100858, 
    SUMMON_GEYSER               = 100820, // summon at caster position
    EMPOWER_SULFURAS            = 100604,
    EMPOWER_SULFURAS_MISSILE    = 100606,
    SUPERHEATED                 = 100593,
    SUPERHEATED_DEBUFF          = 100594,
    DREADFLAME_MISSILE          = 100675,
    DREADFLAME_AOE              = 101361,
    DREADFLAME_CONTROL_AURA     = 100696, // trigger 100823
    // Cenarius spells
    CENARIUS_FROST_FREEZE       = 100345,
    SUMMON_BREADTH_OF_FROST     = 100476,
    BREADTH_OF_FROST_MISSILE    = 100479,
    BREADTH_OF_FROST_TRIGGERED  = 100478,
    //Arch Druid Hamuul Runetotem spells
    ROOT_BEAM                   = 100344,
    ENTRAPPING_ROOTS_SUMMON     = 100644,
    ENTRAPPING_ROOOTS_MISSILE   = 100646,
    ENTRAPPING_ROOTS_TRIGGERED  = 100647,
    // Malfurion Stormrage spells
    LIGHTNING_BEAM              = 100342,
    SUMMON_CLOUDBURST_MISSILE   = 100714,
    CLOUDBURST_VISUAL           = 100758,
    DELUGE                      = 100713,
    DELUGE_VISUAL_BLOB          = 100757,
    DELUGE_CANCEL               = 100771, // not needed
};

enum Npcs
{
    RAGNAROS                = 52409,
    LAVA_RING_NPC           = 540100, // Custom npc, needed for lava ring around Ragnaros
    SULFURAS_NPC            = 53420,
    LAVA_WAVE_NPC           = 53363,
    MAGMA_TRAP_NPC          = 53086,
    SPLITTING_SULFURAS      = 53419, // 38327 was original model id ( burning hamer anim - i dont like it :P)
    SON_OF_FLAME_NPC        = 53140,
    MOLTEN_ELEMENTAL        = 53189,
    ENGULFING_FLAME         = 53485,
    LAVA_SCION              = 53231,
    BLAZING_HEAT            = 53473,
    LIVING_METEOR           = 53500,
    MOLTEN_SEED_CASTER      = 53186,

    // Heroic
    HAMUUL_RUNETOTEM        = 53874,
    CENARIUS                = 53872,
    MALFURION_STORMRAGE     = 53873,
    CLOUDBURST              = 54147,
    BREADTH_OF_FROST        = 53953,
    ENTRAPPING_ROOTS        = 54074,
    // dreadflame npcs
    QUAD_STALKER            = 52850,
    DREADFLAME_SPAWN        = 54203,
    MAGMA_GEYSER_NPC        = 54184,

    // Trash
    MOLTEN_ERUPTER          = 53617,
    LAVA_WIELDER            = 53575,
    LAVA_BUNNY              = 53585
};

enum GameObjects
{
    CACHE_OF_FIRELORD_10N       = 208967,
    CACHE_OF_FIRELORD_25N       = 208968,
    RAGNAROS_PLATFORM           = 208835, // SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED); -> and middle of platfrom  should appeared
    FIREWALL_TRASH              = 208873,
    FIREWALL_RAGNAROS           = 209073
};

enum AnimationKits
{
    ANIM_KIT_HAMMER_PICKUP  = 1465,
    ANIM_KIT_ON_LEGS        = 1486,
    ANIM_KIT_EXCLAIM        = 1479,
    ANIM_KIT_EXCLAIM_SHORT  = 1468
};

enum Actions
{
    // Ragnaros
    APPLY_TAUNT_IMMUNITY    = 0,
    REMOVE_TAUNT_IMMUNITY   = 1,

    // Living meteor
    EXPLODE                 = 3,

    //Splitting blows npc
    LEFT                    = 4,
    RIGHT                   = 5,
    MIDDLE                  = 6,
};

enum Phases
{
    PHASE1                        ,
    PREPARING_FOR_INTERMISSION1   ,
    INTERMISSION1                 ,
    PHASE2                        ,
    INTERMISSION2                 ,
    PHASE3                        ,
    PHASE_NORMAL_DEATH            ,
    PHASE_HEROIC_INTERMISSION     ,
    PHASE4                        , // Only on Heroic
};

const Position Left_pos[8] =           // Spawn positions for sons of flames if hammer is burried LEFT
{
    {1074.53f,-13.48f,55.35f, 3.3f},
    {1053.49f,-7.67f,55.35f, 4.2f},
    {996.33f,-40.80f,55.49f, 0.0f},
    {996.68f,-75.46f,55.49f, 0.84f},
    {1011.43f,-89.09f,55.37f, 1.0f},
    {1030.00f,-98.31f,56.00f, 1.4f},
    {1055.01f,-106.59f,55.4f, 1.8f},
    {1073.60f,-100.20f,55.35f, 2.0f},
};

const Position Middle_pos[8] =           // Spawn positions for sons of flames if hammer is burried MIDDLE
{
    {1074.53f,-13.48f,55.35f, 3.3f},
    {1053.49f,-7.67f,55.35f, 4.2f},
    {1032.33f,-20.75f,55.40f, 4.87f},
    {1007.16f,-26.50f,55.36f, 5.4f},
    {1011.14f,-89.09f,55.37f, 1.0f},
    {1030.00f,-98.31f,56.00f, 1.4f},
    {1055.01f,-106.59f,55.4f, 1.8f},
    {1073.60f,-100.20f,55.35f, 2.0f},
};

const Position Right_pos[8] =           // Spawn positions for sons of flames if hammer is burried RIGHT
{
    {1074.53f,-13.48f,55.35f, 3.3f},
    {1053.49f,-7.67f,55.35f, 4.2f},
    {1035.77f,-21.35f,55.40f, 5.0f},
    {1005.50f,-29.70f,55.40f, 5.40f},
    {997.14f,-40.64f,55.39f, 5.7f},
    {997.14f,-75.15f,55.43f, 6.18f},
    {1055.01f,-106.59f,55.4f, 1.8f},
    {1073.60f,-100.20f,55.35f, 2.0f},
};

const Position Seeds_pos[6] =           // Molten seed casters positions
{
    {1057.0f,-133.48f,42.35f, 0.0f},
    {996.49f,-128.67f,42.35f, 0.0f},
    {967.77f,-93.35f ,42.35f, 0.0f},
    {960.50f,-25.70f ,42.35f, 0.0f},
    {983.14f, 13.04f ,42.35f, 0.0f},
    {1053.14f,25.64f ,42.35f, 0.0f},
};

struct Yells
{
    uint32 sound;
    const char * text;
};

static const Yells intro = {0,"Mortal insects! You dare trespass into MY domain? Your arrogance will be purged in living flame."}; // TODO : FOUND SOUND ID

static const Yells RandomAggro[3]= // DONE
{
    {24533, "Begone from my realm, insects."},
    {24536, "The realm of fire will consume you!"},
    {24535, "Be consumed by flame!"},
};

static const Yells Transition[4]= // DONE
{
    {24513, "Come forth, my servants, defend your master!"},
    {24514, "Minions of fire, purge the outsiders!"},
    {24515, "Denizens of flame, come to me!"},
    {24516, "Arise, servants of fire! Consume their flesh!"},
};

static const Yells Phase2[3]= // DONE
{
    {24523, "Enough! I will finish this!"},
    {24524, "Fall to your knees, mortals! This ends now!"},
    {24525, "Sulfuras will be your end!"},
};

static const Yells Phase3[3]= // DONE
{
    {24520, "You will be crushed!"},
    {24521, "Die!"},
    {24522, "Your judgement has come!"},
};

static const Yells Phase4[5]= // DONE
{
    {24528, "Too soon..."},
    {25159, "No, fiend. Your time is NOW."},
    {25169, "Heroes! He is bound. Finish him!"},
    {25427, "Arrggh, outsiders - this is not your realm!"},
    {24526, "When I finish this, your pathetic mortal world will burn with my vengeance!"},
};

static const Yells RandomKill[3]= // DONE
{
    {24531, "Die, insect!"},
    {24530, "Pathetic!"},
    {24529, "This is my realm!"},
};

static const Yells HeroicDeath[7]= // DONE
{
    {24518, "No, noooo- This was to be my hour of triumph..."},
    {25170, "It is finished then!"},
    {25160, "Perhaps..."},
    {25171, "Heroes, the world owns you a great debt."},
    {25158, "Ragnaros has perished. But the primal powers he represents can never be vanquished. Another will rise to power, someday."},
    {25168, "Yes, Cenarius. We must maintain a constant vigil over this realm. For let us celebrate this day, and the great victory we have earned here."},
    {25161, "Indeed."},
};

float engulfing_lengths[4] = {45.0f,58.9f,73.7f,89.5f}; // Distances for Engulfing flames

# define MINUTE 60000
# define NEVER  (0xffffffff) // used as "delayed" timer (10 minutes)

# define MIDDLE_X 1033.54f
# define MIDDLE_Y  -55.08f
# define MIDDLE_Z   55.92f
#define  MIDDLE_RADIUS   55.0f

typedef struct flamematrix
{
    bool valid; // if it's on platform
    bool free; // if there is no dreadflame active on this position
    float x,y,z; // world coordiantes
}FLAME_MATRIX;

#define MIN_X   981.0f
#define MAX_X   1112.0f
#define MIN_Y   17.0f
#define MAX_Y   -132.0f
#define STEP    6.0f

typedef struct dreadsPos
{
    int32 wx,wy; // x,y coordinate in world
    uint32 x,y; // x,y coordinate in matrix
}DREADSPOSITIONS;


class boss_ragnaros_firelands : public CreatureScript
{
public:
    boss_ragnaros_firelands() : CreatureScript("boss_ragnaros_firelands") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_ragnaros_firelandsAI (creature);
    }

    struct boss_ragnaros_firelandsAI : public ScriptedAI
    {
        boss_ragnaros_firelandsAI(Creature* creature) : ScriptedAI(creature),Summons(creature)
        {
            instance = creature->GetInstanceScript();
            Creature * lava_ring = me->SummonCreature(LAVA_RING_NPC,me->GetPositionX(),me->GetPositionY(),me->GetPositionZ() + 13.0f,me->GetOrientation()); // + 14
            if(lava_ring)
            {
                LAVA_RING_GUID = lava_ring->GetGUID();
                lava_ring->CastSpell(lava_ring,LAVA_FIRE_RING,true);
                lava_ring->SetFloatValue(OBJECT_FIELD_SCALE_X, 12.0f);
                lava_ring->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
                // TODO : Move this to DB
                lava_ring->SetPhaseMask(PHASEMASK_NORMAL,false);
            }

            me->SetRespawnDelay(7*DAY);
            me->SetPhaseMask(PHASEMASK_NORMAL,false);

            me->SetFloatValue(UNIT_FIELD_COMBATREACH,50.0f);

            GameObject * door = me->FindNearestGameObject(FIREWALL_RAGNAROS,500.0f); // Fire wall
            if (door)
                door->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);

            speech = false;
            SetEquipmentSlots(false, 69804, EQUIP_NO_CHANGE, EQUIP_NO_CHANGE);
            me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER); // Need to bet set, Ragnaros should "levitate" in lava, not standing on feets, lot of animations will not work without this
        }

        InstanceScript* instance;
        uint64 LAVA_RING_GUID;
        uint32 BurningWoundTimer;
        uint32 Sulfurus_timer;
        uint32 Magma_trap_timer;
        uint32 Magma_timer;
        uint32 Hand_of_ragnaros_timer;
        uint32 Wrath_timer;
        uint32 CheckTimer;
        uint32 PHASE;
        uint32 Burry_timer;
        uint32 reemerge;
        uint32 Lava_Bolt_timer;
        uint32 Molten_seeds_timer;
        uint32 Engulfing_flames_timer;
        uint32 Meteor_timer;
        uint32 Enrage_timer;
        uint32 AllowTurning_timer;
        uint32 Text_timer;
        uint32 Kill_timer;
        // Intermission 4 timers
        uint32 HeroicIntermissionTimer;
        uint32 IntermissionStep;
        //Heroic timers
        uint32 breadthTimer;
        uint32 dreadFlameTimer;
        uint32 entrappingRootsTimer;
        uint32 cloudBurstTimer;
        uint32 empowerSulfurasTimer;
        uint32 spreadFlamesTimer;
        uint32 geyserTimer;
        uint32 chaseTimer;
        bool speech;
        bool burried;

        SummonList Summons;

        FLAME_MATRIX flames[22][25];
        std::list<DREADSPOSITIONS> lastDreads;
        uint32 dreadFlameAct;

        void SetMatrix(void)
        {
            float xaddition = 0.0f;
            float yaddition = 0.0f;

            for (uint8 i = 0; i < 22; i++)
            {
                yaddition = 0.0f;

                for (uint8 j = 0; j < 25; j++)
                {
                    flames[i][j].free = true;
                    flames[i][j].x = MAX_X  - xaddition;
                    flames[i][j].y = MIN_Y - yaddition;

                    float z = me->GetBaseMap()->GetHeight(me->GetPhaseMask(),flames[i][j].x,flames[i][j].y,60.0f,true);
                    if ( z > 50.0f && z < 58.0f )
                    {
                        flames[i][j].valid = true;
                        flames[i][j].z = z + 2.0f;
                    }
                    else
                        flames[i][j].valid = false;

                    yaddition += STEP;
                }
                xaddition += STEP;
            }
        }

        FLAME_MATRIX GetCoord(uint32 x, uint32 y)
        {
            return flames[x][y];
        }

        void SetCoord(uint32 x, uint32 y,bool on)
        {
            flames[x][y].free = on;
        }

        void JustReachedHome()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
        }

        void Reset()
        {
            lastDreads.clear();
            SetMatrix();

            me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);

            if(instance)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

            dreadFlameAct           = 40000; // Every 40 seconds, decrement by 5 s each time till 15 sec minimum
            Magma_timer             = 1000;
            CheckTimer              = 3000;
            BurningWoundTimer       = urand(5000,8000);
            Magma_trap_timer        = 17000;
            Hand_of_ragnaros_timer  = 25000;
            Wrath_timer             = 30000;
            Sulfurus_timer          = 32000;
            Enrage_timer            = 15 * MINUTE;
            AllowTurning_timer      = NEVER;
            HeroicIntermissionTimer = NEVER;
            geyserTimer             = NEVER;
            spreadFlamesTimer       = NEVER;
            chaseTimer              = NEVER;
            IntermissionStep        = 0;
            PHASE = PHASE1;
            reemerge = 0;
            burried  = false;

            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
            me->ApplySpellImmune(0, IMMUNITY_ID, 49560, true); // Death Grip jump effect
            me->ApplySpellImmune(0, IMMUNITY_ID, 81261, true); // Solar Beam
            me->ApplySpellImmune(0, IMMUNITY_ID, 88625, true); // Chastise
            me->ApplySpellImmune(0, IMMUNITY_ID, 77606, true); // Dark Simulacrum 
            // Entrapping roots stun is allowed
            me->ApplySpellImmune(0, IMMUNITY_ID, 100653, false);
            me->ApplySpellImmune(0, IMMUNITY_ID, 101237, false);

            me->RemoveAura(BURROW_SULFURAS);
            me->RemoveAura(100295);
            me->RemoveAura(100296);
            me->RemoveAura(100297);

            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);

            if (instance)
                instance->SetData(TYPE_RAGNAROS, NOT_STARTED);

            Unit* unit = Unit::GetUnit(*me, LAVA_RING_GUID);

            if (unit)
            {
                if (IsHeroic() && me->getFaction() == 35)
                    unit->SetVisible(false);
                else
                    unit->SetVisible(true);
            }
            else
                unit->SetVisible(true);
        }

        void ReceiveEmote(Player* pPlayer, uint32 text_emote) // This is only for testing purpose ( for GMs only )
        {
            if (pPlayer && pPlayer->IsGameMaster() && text_emote == TEXTEMOTE_LAUGH)
            {
                me->setFaction(14);
                me->SetVisible(true);

                Unit* unit = Unit::GetUnit(*me, LAVA_RING_GUID);
                if (unit)
                    unit->SetVisible(true);
            }

            if (pPlayer && pPlayer->IsGameMaster() && text_emote == TEXTEMOTE_KNEEL)
            {
                me->setFaction(35);
                me->SetVisible(false);

                Unit* unit = Unit::GetUnit(*me, LAVA_RING_GUID);
                if (unit)
                    unit->SetVisible(false);
            }
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (who->ToPlayer() && !who->ToPlayer()->IsGameMaster() && speech == false && me->getFaction() == 14)
            {
                speech = true;
                PlayAndYell(intro.sound,intro.text);
                me->PlayOneShotAnimKit(ANIM_KIT_EXCLAIM);
                me->SetFloatValue(UNIT_FIELD_COMBATREACH,20.0f);
                me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS,20.0f);
                return;
            }
            ScriptedAI::MoveInLineOfSight(who);
        }

        void EnterCombat(Unit* /*who*/)
        {
            me->SetFloatValue(UNIT_FIELD_COMBATREACH,70.0f);

            if (instance)
            {
                instance->SetData(TYPE_RAGNAROS, IN_PROGRESS);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            }

            GameObject * door = me->FindNearestGameObject(FIREWALL_RAGNAROS,500.0f); // Fire wall
            if( door)
                door->SetGoState(GO_STATE_READY); // lock door

            uint8 rand_ = urand(0,2);
            PlayAndYell(RandomAggro[rand_].sound,RandomAggro[rand_].text);
        }

        void EnterEvadeMode()
        {
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);

            if (IsHeroic())
            {
                if (GameObject * goPlatform = me->FindNearestGameObject(RAGNAROS_PLATFORM,500.0f))
                {
                    goPlatform->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);
                    goPlatform->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_DAMAGED);
                }

                me->SetPosition(me->GetHomePosition(),true);
                me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);

                if (instance)
                {
                    // Superheated
                    instance->DoRemoveAurasDueToSpellOnPlayers(SUPERHEATED_DEBUFF);
                    instance->DoRemoveAurasDueToSpellOnPlayers(100915);
                    // Deluge
                    instance->DoRemoveAurasDueToSpellOnPlayers(DELUGE);
                    instance->DoRemoveAurasDueToSpellOnPlayers(101015);
                }
                // Dont forget about cloudburst, cause it's summoned by creature
                if (Creature * pCloud = me->FindNearestCreature(CLOUDBURST,500.0f,true))
                    pCloud->ForcedDespawn();

                // Despawn druids
                Creature * pCenarius = me->FindNearestCreature(CENARIUS,500.0f,true);
                Creature * pMalfurion = me->FindNearestCreature(MALFURION_STORMRAGE,500.0f,true);
                Creature * pHamuul = me->FindNearestCreature(HAMUUL_RUNETOTEM,500.0f,true);

                if (pCenarius && pMalfurion && pHamuul)
                {
                    pCenarius->ForcedDespawn();
                    pMalfurion->ForcedDespawn();
                    pHamuul->ForcedDespawn();
                }

            }

            me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE);
            me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE);
            me->SetFloatValue(UNIT_FIELD_COMBATREACH,20.0f);
            me->RemoveAllAuras();
            Summons.DespawnAll();
            if(instance)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

            GameObject * door = me->FindNearestGameObject(FIREWALL_RAGNAROS,500.0f);
            if( door)
                door->SetGoState(GO_STATE_ACTIVE); // unlock door
            ScriptedAI::EnterEvadeMode();
        }

        void JustDied(Unit* /*killer*/)
        {
            if(instance)
            {
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                instance->DoUpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET, 102237);

                if (IsHeroic())
                {
                    // Superheated
                    instance->DoRemoveAurasDueToSpellOnPlayers(SUPERHEATED_DEBUFF);
                    instance->DoRemoveAurasDueToSpellOnPlayers(100915);
                    // Deluge
                    instance->DoRemoveAurasDueToSpellOnPlayers(DELUGE);
                    instance->DoRemoveAurasDueToSpellOnPlayers(101015);
                }
            }

            if (IsHeroic() && instance)
            {
                PlayAndYell(HeroicDeath[0].sound, HeroicDeath[0].text);
                if (Creature * pCenarius = me->FindNearestCreature(CENARIUS, 500.0f, true))
                {
                    // Just start final conversation, Cenarius will handle all three conversations, it's waste of time and resources to create script for all of them
                    pCenarius->AI()->DoAction(0);
                }

                AchievementEntry const* achievement = sAchievementStore.LookupEntry(5985);

                if (!sAchievementMgr->IsRealmCompleted(achievement)) // If (Realm First! Ragnaros achievement) is not already completed
                {
                    Map * map = me->GetMap();
                    if (!map)
                        return;

                    Map::PlayerList const& plrList = map->GetPlayers();
                    if (plrList.isEmpty())
                        return;

                    Player * player = plrList.getFirst()->getSource();

                    if (player == NULL)
                        return;

                    if (!player->GetGroup() || !player->GetGroup()->IsGuildGroup(player->GetGuildId())) // Not in guild group
                        return;

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

            Summons.DespawnAll();
            DoAfterDied();
        }

        void DamageTaken(Unit* attacker, uint32& damage)
        {
            if (PHASE == PHASE_NORMAL_DEATH || PHASE == PHASE_HEROIC_INTERMISSION) // For sure
                damage = 0;

            if (IsHeroic() && damage == 123456 && attacker->GetMaxHealth() > 500000) // TODO : remove this once is no longer needed
                HideInHeroic();
        }

        void JustDiedInNormal() // In normal mode Ragnaors can't be killed only defeated, he will hide in lava and spawn cache of firelord
        {
            if(instance)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

            Summons.DespawnAll();
            me->InterruptNonMeleeSpells(false);
            DoAfterDied();
            PlayAndYell(24519,"Too soon! ... You have come too soon..."); // Only on normal difficulty
            me->RemoveAllAuras();
            me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me,SUBMERGE_STATE_AURA,true); // Hide in lava
            if (getDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL)
                me->SummonGameObject(CACHE_OF_FIRELORD_10N,1016.36f,-57.73f,55.34f,3.16f,0,0,0,0,0);
            else if (getDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL)
                me->SummonGameObject(CACHE_OF_FIRELORD_25N,1016.36f,-57.73f,55.34f,3.16f,0,0,0,0,0);
            PHASE = PHASE_NORMAL_DEATH;
            Kill_timer = 5000;
        }

        void HideInHeroic() // In heroic Ragnaros will hide in lava but powerfull druids draw out him
        {
            if(instance)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

            me->InterruptNonMeleeSpells(false);
            PlayAndYell(24528, "Too soon..."); // Only on normal difficulty
            me->RemoveAllAuras();
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me,SUBMERGE_STATE_AURA,true); // Hide in lava

            Unit* unit = Unit::GetUnit(*me, LAVA_RING_GUID);
            if(unit)
                unit->SetVisible(false);

            PHASE = PHASE_HEROIC_INTERMISSION;
            HeroicIntermissionTimer = 8000;

            // Summon druids
            me->SummonCreature(CENARIUS,987.32f,-56.22f,55.8f,0.067f);
            me->SummonCreature(HAMUUL_RUNETOTEM,982.25f,-39.0f,55.8f,0.067f);
            me->SummonCreature(MALFURION_STORMRAGE,984.56f,-73.42f,55.8f,0.3f);

            // Set timers for phase 4
            cloudBurstTimer = 50000 - 25000;
            entrappingRootsTimer = 56500 - 25000;
            empowerSulfurasTimer = entrappingRootsTimer + 15000;
            dreadFlameTimer = 48000 - 25000;
            // - 25000 due to intermission time
        }

        void DoAfterDied() // Do necessary things after encounter is done
        {
            GameObject * door = me->FindNearestGameObject(FIREWALL_RAGNAROS,500.0f);
            if( door)
                door->SetGoState(GO_STATE_ACTIVE); // unlock door

            Unit* unit = Unit::GetUnit(*me, LAVA_RING_GUID);
            if(unit && unit->ToTempSummon())
                unit->ToTempSummon()->ForcedDespawn();

            if (instance)
                instance->SetData(TYPE_RAGNAROS, DONE);
        }

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() != TYPEID_PLAYER)
                return;

            uint8 rand_ = urand(0,2);
            PlayAndYell(RandomKill[rand_].sound,RandomKill[rand_].text);
        }

        void JustSummoned(Creature* summon)
        {
            if(summon->GetGUID() == LAVA_RING_GUID)
                return;

            switch(summon->GetEntry())
            {
                case CENARIUS:
                case MALFURION_STORMRAGE:
                case HAMUUL_RUNETOTEM:
                    return;
            }

            Summons.push_back(summon->GetGUID());

            if (summon->GetEntry() == DREADFLAME_SPAWN)
            {
                for(std::list<DREADSPOSITIONS>::const_iterator itr = lastDreads.begin(); itr != lastDreads.end(); ++itr)
                {
                    if ((*itr).wx == (int32)summon->GetPositionX() && (*itr).wy == (int32)summon->GetPositionY())
                    {
                        summon->AI()->SetData(0,(*itr).x);
                        summon->AI()->SetData(1,(*itr).y);
                        break;
                    }
                }
            }
        }

        void CastSpellOnRandomPlayers(uint32 spellId, uint32 size, bool triggered = true, bool ignoreTanks = false)
        {
            std::list<Player*> target_list;
            target_list.clear(); // For sure
            Map * map = me->GetMap();

            if (!map)
                return;

            Map::PlayerList const& plrList = map->GetPlayers();
            if (plrList.isEmpty())
                return;

            for(Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
            {
                if(Player* pPlayer = itr->getSource())
                {
                    if (pPlayer->IsAlive() && !pPlayer->IsGameMaster())
                    {
                        if (ignoreTanks == true)
                        {
                            if (!pPlayer->HasTankSpec() && !pPlayer->HasAura(5487)) // Or bear form
                                target_list.push_back(pPlayer);
                        }
                        else target_list.push_back(pPlayer);
                    }
                }
            }

            for (uint32 i = 0; i < size ; i++)
            {
                if(target_list.empty())
                    break;

                std::list<Player*>::iterator j = target_list.begin();
                advance(j, rand()%target_list.size()); // Pick random target

                if ((*j) && (*j)->IsInWorld())
                {
                    me->CastSpell((*j),spellId,triggered);
                    target_list.erase(j);
                }
            }
        }


        void ExplodeSons() // If 45 secons passed through intermission, find rest of sons and make them explode
        {
            std::list<Creature*> sons;
            me->GetCreatureListWithEntryInGrid(sons, SON_OF_FLAME_NPC, 500.0f);

            for (std::list<Creature*>::iterator itr = sons.begin(); itr != sons.end(); ++itr)
            {
                    (*itr)->CastSpell((*itr),SUPERNOVA,true);
                    (*itr)->ForcedDespawn(1000);
            }
        }

        void DoSulfurasSmash() // Cast Sulfuras smash
        {
            if (!me->IsNonMeleeSpellCasted(false) && !me->HasAura(WORLD_IN_FLAMES) && !me->HasAura(100190))
            {
                Unit * player = SelectTarget(SELECT_TARGET_RANDOM,0,200.0f,true);

                float angle = (player) ? me->GetAngle(player) : (float)(urand(25,33)/10);

                Creature * sulf =me->SummonCreature(SULFURAS_NPC,me->GetPositionX() + cos(angle)*50.0f,me->GetPositionY() + sin(angle)* 50.0f,56.0f,angle,TEMPSUMMON_TIMED_DESPAWN,10000);
                me->SetFacingTo(angle);
                me->SetUInt64Value(UNIT_FIELD_TARGET,sulf->GetGUID());

                AllowTurning_timer = 6000;

                if (sulf)
                    me->CastSpell(sulf->GetPositionX(),sulf->GetPositionY(),sulf->GetPositionZ(),SULFURUS_SMASH,false);
                BurningWoundTimer += 2700;
                Sulfurus_timer = 40000;
            }
        }

        void ShiftToPhase2(void)
        {
            me->RemoveAura(BURROW_SULFURAS);
            me->RemoveAura(100295);
            me->RemoveAura(100296);
            me->RemoveAura(100297);
            me->SetReactState(REACT_AGGRESSIVE);

            CheckTimer = 3000;
            Sulfurus_timer = (IsHeroic()) ? 7000 : 15000;
            Molten_seeds_timer = (IsHeroic()) ? 17500 : 22000;
            Engulfing_flames_timer = (IsHeroic()) ? 42000 : 40000;

            PHASE = PHASE2; // Enter phase 2
            Text_timer = urand( 10000, 15000);

            me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE);

            Unit* unit = Unit::GetUnit(*me, LAVA_RING_GUID);
            if (unit)
                unit->SetVisible(true);
        }

        void ShiftToPhase3(void)
        {
            me->RemoveAura(BURROW_SULFURAS);
            me->RemoveAura(100295);
            me->RemoveAura(100296);
            me->RemoveAura(100297);
            me->SetReactState(REACT_AGGRESSIVE);

            CheckTimer = 3000;
            Sulfurus_timer = (IsHeroic()) ? 13000 : 15000;
            Engulfing_flames_timer = (IsHeroic()) ? 27000 : 31000;
            Meteor_timer = 45000;

            PHASE = PHASE3; // Enter phase 3
            Text_timer = urand( 10000, 15000);

            me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE);

            Unit* unit = Unit::GetUnit(*me, LAVA_RING_GUID);
            if (unit)
                unit->SetVisible(true);
        }

        void PlayAndYell(uint32 soundId, const char * text)
        {
            DoPlaySoundToSet(me,soundId);
            me->MonsterYell(text, LANG_UNIVERSAL, 0);
        }

        void CheckMeleeDistance(void) // If Ragnaros cannot reach his victim in melee range, he should cast magma blast on him
        {
            if (!me->IsWithinMeleeRange(me->GetVictim()))
                me->CastSpell(me->GetVictim(),MAGMA_BLAST,true);
            CheckTimer = 3000;
        }

        void SpawnEngulfingFlames(uint32 length, float start_angle, float end_angle)
        {
            float act_angle = start_angle;
            float lenght_of_arc = 2.0f*(end_angle - start_angle) * length;

            uint32 total_flames= (uint32) (lenght_of_arc / 25); // every 25 yards approx.

            for(uint32 i = 0; i < total_flames; i++ )
            {
                me->SummonCreature(ENGULFING_FLAME,me->GetPositionX()+cos(act_angle)*length,me->GetPositionY()+sin(act_angle)*length,55.4f,0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                act_angle += (float) ((end_angle-start_angle)/ total_flames);
            }
        }

        Position GetRandomPositionInRadius(float radius)// From middle of Ragnaros platform
        {
            float angle = urand(0,2*M_PI);
            Position pos;
            pos.m_positionX = MIDDLE_X + cos(angle)*radius;
            pos.m_positionY = MIDDLE_Y + sin(angle)*radius;
            pos.m_positionZ = me->GetBaseMap()->GetHeight(me->GetPhaseMask(),MIDDLE_X,MIDDLE_Y,MIDDLE_Z,true) + 1.0f;
            return pos;
        }

        inline bool CanCast(void) // We can't allow casting when we don't have our victim  in UNIT_FIELD_TARGET, due to spells animation ( Sulfuras smash,Splitting blow )
        {
            if (me->IsInWorld() && me->GetVictim() && me->GetVictim()->IsInWorld() )
                if (me->GetUInt64Value(UNIT_FIELD_TARGET) == me->GetVictim()->GetGUID())
                    return true;

            return false;
        }

        void SpawnSonsOfFlame (int32 side)
        {
            switch (side)
            {
                case LEFT:
                    for (uint8 i = 0; i < 8; i++)
                        me->SummonCreature(SON_OF_FLAME_NPC,Left_pos[i].GetPositionX(),Left_pos[i].GetPositionY(),Left_pos[i].GetPositionZ(),Left_pos[i].GetOrientation(),TEMPSUMMON_CORPSE_DESPAWN, 0);
                break;

                case MIDDLE:
                    for (uint8 i = 0; i < 8; i++)
                        me->SummonCreature(SON_OF_FLAME_NPC,Middle_pos[i].GetPositionX(),Middle_pos[i].GetPositionY(),Middle_pos[i].GetPositionZ(),Middle_pos[i].GetOrientation(),TEMPSUMMON_CORPSE_DESPAWN, 0);
                break;

                case RIGHT:
                    for (uint8 i = 0; i < 8; i++)
                        me->SummonCreature(SON_OF_FLAME_NPC,Right_pos[i].GetPositionX(),Right_pos[i].GetPositionY(),Right_pos[i].GetPositionZ(),Right_pos[i].GetOrientation(),TEMPSUMMON_CORPSE_DESPAWN, 0);
                break;

                default:
                    break;
            }
        }

        void SpawnSonsOfFlameHeroic(uint32 side) // Should spawn on random locations on HC
        {
            float angle;
            float distance;
            uint32 randNum;

            switch (side)
            {
                case LEFT:
                {
                    randNum = urand(2,3);
                    for(uint32 i = 0; i < randNum; i++ )
                    {
                        distance = urand(45,53);
                        angle = urand(160,200);
                        angle /= 100.0f;
                        me->SummonCreature(SON_OF_FLAME_NPC,me->GetPositionX()+cos(angle)*distance,me->GetPositionY()+sin(angle)*distance,55.4f,0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                    }
                    randNum = 8 - randNum;
                    for(uint32 i = 0; i < randNum; i++ )
                    {
                        distance = urand(45,53);
                        angle = urand(300,500);
                        angle /= 100.0f;
                        me->SummonCreature(SON_OF_FLAME_NPC,me->GetPositionX()+cos(angle)*distance,me->GetPositionY()+sin(angle)*distance,55.4f,0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                    }
                }
                break;
                case MIDDLE:
                {
                    randNum = urand(4,5);
                    for(uint32 i = 0; i < randNum; i++ )
                    {
                        distance = urand(45,53);
                        angle = urand(160,270);
                        angle /= 100.0f;
                        me->SummonCreature(SON_OF_FLAME_NPC,me->GetPositionX()+cos(angle)*distance,me->GetPositionY()+sin(angle)*distance,55.4f,0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                    }
                    randNum = 8 - randNum;
                    for(uint32 i = 0; i < randNum; i++ )
                    {
                        distance = urand(45,53);
                        angle = urand(350,500);
                        angle /= 100.0f;
                        me->SummonCreature(SON_OF_FLAME_NPC,me->GetPositionX()+cos(angle)*distance,me->GetPositionY()+sin(angle)*distance,55.4f,0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                    }
                }
                break;
                case RIGHT:
                {
                    randNum = urand(5,6);
                    for(uint32 i = 0; i < randNum; i++ )
                    {
                        distance = urand(45,53);
                        angle = urand(160,320);
                        angle /= 100.0f;
                        me->SummonCreature(SON_OF_FLAME_NPC,me->GetPositionX()+cos(angle)*distance,me->GetPositionY()+sin(angle)*distance,55.4f,0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                    }
                    randNum = 8 - randNum;
                    for(uint32 i = 0; i < randNum; i++ )
                    {
                        distance = urand(45,53);
                        angle = urand(430,500);
                        angle /= 100.0f;
                        me->SummonCreature(SON_OF_FLAME_NPC,me->GetPositionX()+cos(angle)*distance,me->GetPositionY()+sin(angle)*distance,55.4f,0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                    }
                }
                break;
            }
        }

        // If >= 4/10 (10 man / 25 man) people or more are clustered together, form Magma Geyser at location
        // This algorithm has time complexity O(n^2)
        // TODO : Maybe find better algorithm
        Player * RaidIsClusteredTogether(void)
        {
            uint32 counter = 0;
            uint32 maximum = (Is25ManRaid()) ? 10 : 4;

            Map * map = me->GetMap();
            if (!map)
                return NULL;

            Map::PlayerList const& plrList = map->GetPlayers();
            if (plrList.isEmpty())
                return NULL;

            std::list<Player*> players;
            // Copy players to second list
            for(Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
                if(Player* pl = itr->getSource())
                    if (pl->IsInWorld() && pl->IsAlive() && !pl->IsGameMaster())
                        players.push_back(itr->getSource());

            for(Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
            {
                if(Player* p = itr->getSource())
                {
                    counter = 0;

                    if (p->IsInWorld() && p->IsAlive() && !p->IsGameMaster())
                    {
                        // Measure distance for player to every player
                        for (std::list<Player*>::iterator j = players.begin(); j != players.end(); ++j)
                        {
                            if ((*j) && (*j)->IsInWorld())
                            {
                                if ((*j) == p) // Exclude self
                                    continue;
                                if (p->GetExactDist2d(*j) <= 7.5f)
                                    counter++;
                            }
                        }
                        if (counter >= maximum) // Stop looking
                            return p;
                    }
                }
            }
            return NULL;
        }

        void UpdateAI(const uint32 diff)
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            if (me->IsInWorld() && me->GetVictim() && me->GetVictim()->IsInWorld())
            {
                if (me->GetUInt64Value(UNIT_FIELD_TARGET) == me->GetVictim()->GetGUID())
                {
                    me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, false);
                    me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, false);
                }
                else
                {
                    if (PHASE != PHASE4) // Not in last phase on heroic
                    {
                        me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
                        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
                    }
                }
            }

            if (PHASE == INTERMISSION1 || PHASE == INTERMISSION2 || PHASE == PREPARING_FOR_INTERMISSION1)
            {
                Creature * sulfur = me->FindNearestCreature(SPLITTING_SULFURAS,80.0f,true);

                if (sulfur && sulfur->IsInWorld())
                {
                    me->SetUInt64Value(UNIT_FIELD_TARGET, sulfur->GetGUID());
                }
            }

            if (Magma_timer <= diff)
            {
                if (PHASE != PHASE4 && IntermissionStep < 4)
                    me->CastSpell(me,MAGMA,true);
                Magma_timer = 1000;
            }
            else Magma_timer -= diff;

            if (AllowTurning_timer <= diff) // Put again  our victim to  UNIT_FIELD_TARGET
            {
                if (me->GetVictim() && me->GetVictim()->IsInWorld())
                    me->SetUInt64Value(UNIT_FIELD_TARGET,me->GetVictim()->GetGUID());
                AllowTurning_timer = NEVER;
            }
            else AllowTurning_timer -= diff;

            if (Enrage_timer <= diff) // Enrage after 15 minutes
            {
                me->CastSpell(me,BERSERK,true);
                Enrage_timer = NEVER;
            }
            else Enrage_timer -= diff;

            if (BurningWoundTimer <= diff)
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    if (PHASE == PHASE1 || PHASE == PHASE2 || PHASE == PHASE3 || PHASE == PHASE4)
                        if (CanCast() && me->IsWithinMeleeRange(me->GetVictim()))
                            me->CastSpell(me->GetVictim(),BURNING_WOUND_DOT,true);

                    BurningWoundTimer = urand(4000,8000);
                }
            }
            else BurningWoundTimer -= diff;

            if (me->HealthBelowPct(10) && burried == false ) // On normal difficulty Ragnaros should die at 10 % of his health
            {
                burried = true;

                if (IsHeroic() == false)
                    JustDiedInNormal();
                else
                    HideInHeroic();
            }

/************************************************************ PHASE 1 *********************************************************************/
            if (PHASE == PHASE1)
            {
                if (me->HealthBelowPct(70) && !me->IsNonMeleeSpellCasted(false) && CanCast()) // Leave PHASE1 if we are below 70 % of our health
                {
                    PHASE = PREPARING_FOR_INTERMISSION1;
                    return;
                }

                if (CheckTimer <= diff)
                {
                    if(!me->IsNonMeleeSpellCasted(false) && CanCast())
                    {
                        CheckMeleeDistance();
                    }
                }
                 else CheckTimer -= diff;

                 if (Wrath_timer <= diff) // Cast Wrath of Ragnaros  on random player position
                {
                    if (!me->IsNonMeleeSpellCasted(false) && CanCast())
                    {
                        PlayAndYell(24532,"By fire be purged!");

                        uint8 max = 2;
                        if (getDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL || getDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC)
                            max = 6;

                        CastSpellOnRandomPlayers(WRATH_OF_RAGNAROS,max,true,true);

                        Wrath_timer = 22000;
                    }
                 }
                 else Wrath_timer -= diff;

                 if (Hand_of_ragnaros_timer <= diff) // Aoe dmg + knockback in melee range
                {
                    if (!me->IsNonMeleeSpellCasted(false) && CanCast())
                    {
                        me->CastSpell(me->GetVictim(),HAND_OF_RAGNAROS,false);
                        Hand_of_ragnaros_timer = 25000;
                    }
                 }
                 else Hand_of_ragnaros_timer -= diff;


                if (Magma_trap_timer <= diff) // Spawn Magma trap on random player position
                {
                    Unit * player;

                    player = SelectTarget(SELECT_TARGET_RANDOM, 2, 200.0f, true); // Not under tanks ? :P

                    if (player == NULL)
                        player = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true); // If we didn't find target, tanks are allowed

                    if (player)
                    {
                        //float x,y,z;
                        //player->GetPosition(x,y,z);
                        //player->UpdateGroundPositionZ(x,y,z);
                        //me->CastSpell(player,MAGMA_TRAP_MARK_MISSILE,true);
                        me->CastSpell(player,MAGMA_TRAP_MARK_MISSILE,true);
                    }

                    Magma_trap_timer = 25000;
                 }
                 else Magma_trap_timer -= diff;

                if (Sulfurus_timer <= diff)
                {
                    if (CanCast())
                        DoSulfurasSmash();
                }
                else Sulfurus_timer -= diff;
            }

/********************************************** PREPARING FOR INTERMISSION 1 PHASE *********************************************************************/

            if (PHASE == PREPARING_FOR_INTERMISSION1) // Burry to lava
            {
                float angle = 0.0f;

                switch (urand(0,2))
                {
                    case 0:
                        angle = 2.54f;
                        break;
                    case 1:
                        angle = 3.135f;
                        break;
                    case 2:
                        angle = 3.73f;
                        break;
                }

                Creature * sulfur = me->SummonCreature(SPLITTING_SULFURAS,me->GetPositionX() + cos(angle)*50.0f,me->GetPositionY() + sin(angle)* 50.0f,56.34f,angle,TEMPSUMMON_MANUAL_DESPAWN,0);

                if(sulfur && sulfur->IsInWorld())
                {
                    me->SetUInt64Value(UNIT_FIELD_TARGET,sulfur->GetGUID());
                }

                //if (IsHeroic() == false)
                //{
                    if (fabs(angle - 2.54f) < FLT_EPSILON )
                        SpawnSonsOfFlame(LEFT);
                    else if( fabs(angle - 3.135f) < FLT_EPSILON )
                        SpawnSonsOfFlame(MIDDLE);
                    else
                        SpawnSonsOfFlame(RIGHT);
                /*}
                else
                {
                    if (fabs(angle - 2.54f) < FLT_EPSILON )
                        SpawnSonsOfFlameHeroic(LEFT);
                    else if( fabs(angle - 3.135f) < FLT_EPSILON )
                        SpawnSonsOfFlameHeroic(MIDDLE);
                    else
                        SpawnSonsOfFlameHeroic(RIGHT);
                }*/

                me->CastSpell(me->GetVictim(),SPLITTING_BLOW,false); // Burry animation
                me->SetFacingTo(angle);
                AllowTurning_timer = 8000 + 45000 + 6000; // Casting time + 45 seconds

                Lava_Bolt_timer = 15000;
                Burry_timer = 11900;
                PHASE = INTERMISSION1;
            }

/**************************************************** INTERMISSION 1 PHASE *********************************************************************/

            if (PHASE == INTERMISSION1)
            {
                if (Lava_Bolt_timer <= diff)
                {
                    int32 max = 4;
                    if (getDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL || getDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC)
                        max += 6;

                    me->CastCustomSpell(LAVA_BOLT, SPELLVALUE_MAX_TARGETS,max, me, true);

                    Lava_Bolt_timer = 4000;
                }
                else Lava_Bolt_timer -= diff;


                if (Burry_timer <= diff )
                {
                    if (reemerge == 0 ) // Burry Ragnaros after casting splitting blow
                    {
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        uint8 rand_ = urand(0,3);
                        PlayAndYell(Transition[rand_].sound,Transition[rand_].text);
                        me->CastSpell(me,BURROW_SULFURAS,true); // This spell force him to leave hammer on the ground and force him to burry in lava
                        Unit* unit = Unit::GetUnit(*me, LAVA_RING_GUID);
                        if (unit)
                            unit->SetVisible(false);
                        Burry_timer = 45000;
                        reemerge++;
                    }
                    else // if 45 seconds passed allow turning and explode rest of sons
                    {
                        me->PlayOneShotAnimKit(ANIM_KIT_HAMMER_PICKUP);
                        if (Creature * sulfuras = me->FindNearestCreature(SPLITTING_SULFURAS,500.0f,true))
                            sulfuras->ForcedDespawn(6000);

                        ShiftToPhase2();
                        AllowTurning_timer = 6000;
                        ExplodeSons();
                    }

                }
                else Burry_timer -= diff;


                if (reemerge == 1) // Start looking for sons after burried in lava
                {
                    Creature * son = NULL;
                    son = me->FindNearestCreature(SON_OF_FLAME_NPC,500.0f,true);
                    if (son == NULL) // If all sons of flames are dead switch to the next phase
                    {
                        me->PlayOneShotAnimKit(ANIM_KIT_HAMMER_PICKUP);
                        if (Creature * sulfuras = me->FindNearestCreature(SPLITTING_SULFURAS,500.0f,true))
                            sulfuras->ForcedDespawn(6000);
                        ShiftToPhase2();
                        AllowTurning_timer = 6000;
                    }
                }
            }

/************************************************************ PHASE 2 *********************************************************************/

            if (PHASE == PHASE2)
            {
                if (Text_timer <= diff)
                {
                    uint8 rand_ = urand(0,2);
                    PlayAndYell(Phase2[rand_].sound,Phase2[rand_].text);
                    Text_timer = urand(15000, 20000);
                }
                else Text_timer -= diff;

                //Sulfurus Smash
                if (Sulfurus_timer <= diff)
                {
                    if (CanCast())
                        DoSulfurasSmash();
                }
                else Sulfurus_timer -= diff;


                if (CheckTimer <= diff)
                {
                    if (!me->IsNonMeleeSpellCasted(false) && CanCast())
                        CheckMeleeDistance();
                }
                 else CheckTimer -= diff;


                if (Engulfing_flames_timer <= diff)
                {
                    if (!me->IsNonMeleeSpellCasted(false) && CanCast())
                    {
                        if (IsHeroic() == false) // In normal version cast Engulfing flames
                        {
                            me->CastSpell(me,EMGULFING_FLAMES,false); // Visual casting of Engulfing flames
                            BurningWoundTimer += 3200;
                            uint32 randNum = urand(0,2); // Spawn on 3 random locations

                            switch(randNum)
                            {
                                case 0 :
                                    SpawnEngulfingFlames(engulfing_lengths[randNum],1.6f,5.2f);
                                break;

                                case 1 :
                                    SpawnEngulfingFlames(engulfing_lengths[randNum],1.6f,4.6f);
                                break;

                                default:
                                    SpawnEngulfingFlames(engulfing_lengths[2],2.12f,4.33f);
                                    SpawnEngulfingFlames(engulfing_lengths[3],2.12f,4.33f);
                                break;
                            }
                        }
                        else // in heroic is replaced by World in Flames handled via spellscript
                        {
                            me->CastSpell(me,WORLD_IN_FLAMES,false);
                        }

                        Engulfing_flames_timer = (IsHeroic()) ? 60000 : 42000;
                    }
                }
                else Engulfing_flames_timer -= diff;


                if (Molten_seeds_timer <= diff) // Spawn molten seeds under every player
                {
                    uint8 playerCounter = 0;
                    uint8 seedCounter   = 0;
                    uint64 seedsGUID[6];

                    memset(&seedsGUID, 0, sizeof(seedsGUID));

                    for (uint8  j = 0; j < 6 ; j++)
                    {
                        Creature * seed = me->SummonCreature(MOLTEN_SEED_CASTER,Seeds_pos[j].GetPositionX(),Seeds_pos[j].GetPositionY(),Seeds_pos[j].GetPositionZ(),0.0f);
                        if (seed)
                        {
                            seed->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
                            seed->ForcedDespawn(15000);
                            seedsGUID[j] = seed->GetGUID();
                        }
                    }

                    Map * map = me->GetMap();

                    if (!map)
                        return;

                    Map::PlayerList const& plrList = map->GetPlayers();
                    if (plrList.isEmpty())
                        return;

                    for(Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
                    {
                        if(Player* pPlayer = itr->getSource())
                        {
                            if ( pPlayer && pPlayer->IsInWorld() && pPlayer->IsAlive() && !pPlayer->IsGameMaster() && pPlayer->GetDistance(me) < 250.0f)
                            {
                                if (playerCounter == 20) // Max 20 seed on 25 man
                                    break;

                                seedCounter = (seedCounter == 6) ? 0 : seedCounter;

                                Creature * pSeed = (Creature*) Unit::GetUnit(*me,seedsGUID[seedCounter]);

                                if ( pSeed && pSeed->IsInWorld())
                                {
                                    Creature * pElemental = me->SummonCreature(MOLTEN_ELEMENTAL,pPlayer->GetPositionX(),pPlayer->GetPositionY(),pPlayer->GetPositionZ(),0.0f);
                                    if (pElemental)
                                    {
                                        pSeed->CastSpell(pPlayer,MOLTEN_SEED_MISSILE,true); // Cast initial Molten seed dmg + visual missile
                                        int32 travelTime = (pSeed->GetExactDist2d(pElemental) / 33.3f) * 1000;
                                        pElemental->AI()->DoAction(travelTime);
                                    }
                                }

                                playerCounter++;
                                seedCounter++;
                            }
                        }
                    }

                    Molten_seeds_timer = 60000;
                }
                 else Molten_seeds_timer -= diff;

/***************************************************** PREPARING FOR INTERMISSION 2 *********************************************************************/

                 if (me->HealthBelowPct(40) && !me->IsNonMeleeSpellCasted(false) && CanCast())
                 {
                    PHASE = INTERMISSION2;
                    reemerge = 0;

                    float angle = 0.0f;

                    if (urand(0,1))
                        angle = 2.54f; // LEFT
                    else
                        angle = 3.73f; // RIGHT

                    Creature * sulfur = me->SummonCreature(SPLITTING_SULFURAS,me->GetPositionX() + cos(angle)*50.0f,me->GetPositionY() + sin(angle)* 50.0f,55.34f,angle,TEMPSUMMON_MANUAL_DESPAWN,0);
                    if (sulfur && sulfur->IsInWorld())
                    {
                        me->SetUInt64Value(UNIT_FIELD_TARGET,sulfur->GetGUID());
                    }

                    //if (IsHeroic() == false)
                    //{
                        if (fabs(angle - 2.54f) < FLT_EPSILON )
                            SpawnSonsOfFlame(LEFT);
                        else if (fabs(angle - 3.135f) < FLT_EPSILON )
                            SpawnSonsOfFlame(MIDDLE);
                        else
                            SpawnSonsOfFlame(RIGHT);
                    /*}
                    else
                    {
                        if (fabs(angle - 2.54f) < FLT_EPSILON )
                            SpawnSonsOfFlameHeroic(LEFT);
                        else if( fabs(angle - 3.135f) < FLT_EPSILON )
                            SpawnSonsOfFlameHeroic(MIDDLE);
                        else
                            SpawnSonsOfFlameHeroic(RIGHT);
                    }*/

                    AllowTurning_timer = 8000 + 45000+ 6000; // Casting time + 45 seconds

                    me->CastSpell(me->GetVictim(),SPLITTING_BLOW,false); // Burry anim
                    me->SetFacingTo(angle);
                    Burry_timer = 11900;
                    Lava_Bolt_timer = 15000;
                 }
            }

/****************************************************** INTERMISSION 2 *********************************************************************/

            if(PHASE == INTERMISSION2)
            {
                if (Lava_Bolt_timer <= diff)
                {
                    int32 max = 4;
                    if (getDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL || getDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC)
                        max += 6;

                    me->CastCustomSpell(LAVA_BOLT, SPELLVALUE_MAX_TARGETS,max, me, true);

                    Lava_Bolt_timer = 4000;
                }
                else Lava_Bolt_timer -= diff;

                if (Burry_timer <= diff )
                {
                    if (reemerge ==0 ) // First time burry to lava
                    {
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        uint8 rand_ = urand(0,3);
                        PlayAndYell(Transition[rand_].sound,Transition[rand_].text);
                        me->CastSpell(me,BURROW_SULFURAS,true); // This spell force him to leave hammer on the ground and force him to burry in lava
                        Unit* unit = Unit::GetUnit(*me, LAVA_RING_GUID);
                        if(unit)
                            unit->SetVisible(false);
                        Burry_timer = 45000;
                        reemerge = 1;
                    }
                    else // Next time switch to phase 3, cause 45 seconds passed
                    {
                        me->PlayOneShotAnimKit(ANIM_KIT_HAMMER_PICKUP);
                        if (Creature * sulfuras = me->FindNearestCreature(SPLITTING_SULFURAS,500.0f,true))
                            sulfuras->ForcedDespawn(6000);
                        AllowTurning_timer = 6000;
                        ShiftToPhase3();
                        ExplodeSons();
                    }
                }
                else Burry_timer -= diff;

                if (reemerge == 1) // Start looking for sons after burried in lava
                {
                    Creature * son = NULL;
                    son = me->FindNearestCreature(SON_OF_FLAME_NPC,500.0f,true);
                    if (son == NULL) // If all sons of flames are dead switch to the next phase
                    {
                        me->PlayOneShotAnimKit(ANIM_KIT_HAMMER_PICKUP);
                        if (Creature * sulfuras = me->FindNearestCreature(SPLITTING_SULFURAS,500.0f,true))
                            sulfuras->ForcedDespawn(6000);
                        AllowTurning_timer = 6000;
                        ShiftToPhase3();
                    }
                }
            }

/************************************************************ PHASE 3 *********************************************************************/

            if( PHASE == PHASE3) // Last Phase on normal difficulty
            {
                if (Text_timer <= diff)
                {
                    uint8 rand_ = urand(0,2);
                    PlayAndYell(Phase3[rand_].sound,Phase3[rand_].text);
                    Text_timer = urand(15000, 20000);
                }
                else Text_timer -= diff;

                if (Meteor_timer <= diff)
                {
                    Unit * player;

                    player = SelectTarget(SELECT_TARGET_RANDOM, 2, 200.0f, true); // Not under tanks
                    if (player == NULL)
                        player = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true);

                    if (player)
                    {
                        me->CastSpell(player,LIVING_METEOR_MISSILE,true);
                    }
                    Meteor_timer = 45000;
                }
                else Meteor_timer -= diff;

                //Sulfurus Smash
                if (Sulfurus_timer <= diff)
                {
                    if(!me->IsNonMeleeSpellCasted(false) && CanCast())
                        DoSulfurasSmash();
                }
                else Sulfurus_timer -= diff;


                if (CheckTimer <= diff)
                {
                    if(!me->IsNonMeleeSpellCasted(false) && CanCast())
                        CheckMeleeDistance();
                }
                 else CheckTimer -= diff;


                 if (Engulfing_flames_timer <= diff)
                {
                    if (!me->IsNonMeleeSpellCasted(false) && CanCast())
                    {
                        if (IsHeroic() == false) // In normal version cast Engulfing flames
                        {
                            me->CastSpell(me,EMGULFING_FLAMES,false); // Visual casting of Engulfing flames
                            BurningWoundTimer += 3200;
                            uint32 randNum = urand(0,2); // Spawn on 3 random locations

                            switch(randNum)
                            {
                                case 0 :
                                    SpawnEngulfingFlames(engulfing_lengths[randNum],1.6f,5.2f);
                                break;

                                case 1 :
                                    SpawnEngulfingFlames(engulfing_lengths[randNum],1.6f,4.6f);
                                break;

                                default:
                                    SpawnEngulfingFlames(engulfing_lengths[2],2.12f,4.33f);
                                    SpawnEngulfingFlames(engulfing_lengths[3],2.12f,4.33f);
                                break;
                            }
                        }
                        else // in heroic is replaced by World in Flames handled via spellscript
                        {
                            me->CastSpell(me,WORLD_IN_FLAMES,false);
                        }

                        Engulfing_flames_timer = (IsHeroic()) ? 60000 : 42000;
                    }
                }
                else Engulfing_flames_timer -= diff;
            }

            if (PHASE == PHASE_NORMAL_DEATH) // on normal diffiuclty wait 5 secons and then die piecefully
            {
                if (Kill_timer <= diff)
                {
                    me->SetVisible(false);
                    me->Kill(me);
                    Kill_timer = NEVER;
                }
                else Kill_timer -= diff;
            }

            if (PHASE == PHASE_HEROIC_INTERMISSION)
            {
                if (HeroicIntermissionTimer <= diff)
                {
                    switch (IntermissionStep)
                    {
                        case 0:                   // Root beam
                            if (Creature *pCenarius = me->FindNearestCreature(CENARIUS,500.0f,true))
                            {
                                DoPlaySoundToSet(pCenarius,25159);
                                pCenarius->MonsterYell("No, fiend. Your time is NOW.", LANG_UNIVERSAL, 0);
                            }

                            me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE);
                            if (Creature *pRunetotem = me->FindNearestCreature(HAMUUL_RUNETOTEM,500.0f,true))
                                pRunetotem->CastSpell(me,ROOT_BEAM,false);
                            HeroicIntermissionTimer = 2000; 
                            break;
                        case 1:                   // Lightning beam 
                            if (Creature *pMalfurion = me->FindNearestCreature(MALFURION_STORMRAGE,500.0f,true))
                                pMalfurion->CastSpell(me,LIGHTNING_BEAM,false);
                            HeroicIntermissionTimer = 1000;
                            break;
                        case 2:                   // Frost freeze
                        {
                            if (Creature *pCenarius = me->FindNearestCreature(CENARIUS,500.0f,true))
                                pCenarius->CastSpell(pCenarius,CENARIUS_FROST_FREEZE,false);
                            me->AddAura(CENARIUS_FROST_FREEZE,me); // Workaround ..., from unknown reason npc can't cast this spell on rag
                            std::list<Creature*> meteorList;
                            GetCreatureListWithEntryInGrid(meteorList, me, LIVING_METEOR, 200.0f);
                            for (std::list<Creature*>::iterator iter = meteorList.begin(); iter != meteorList.end(); ++iter)
                            {
                                (*iter)->AddAura(100907,(*iter));
                                (*iter)->StopMoving();
                            }
                            HeroicIntermissionTimer = 3000;
                            break;
                        }
                        case 3:                   // Remove submerge state and heal to 50 %
                            me->RemoveAura(SUBMERGE_STATE_AURA);
                            me->CastSpell(me,LEGS_HEAL,true);
                            HeroicIntermissionTimer = 1000;
                            break;
                        case 4:                   // Make platform walkable
                            if (GameObject * goPlatform = me->FindNearestGameObject(RAGNAROS_PLATFORM,500.0f))
                                goPlatform->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_DAMAGED);
                            HeroicIntermissionTimer = 2000;
                            break;
                        case 5:                   // Exclaim animation + text
                            me->RemoveAllAuras(); // Beams
                            if (Creature *pCenarius = me->FindNearestCreature(CENARIUS,500.0f,true))
                                pCenarius->InterruptNonMeleeSpells(false);
                            me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE);
                            if(instance)
                                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                            PlayAndYell(25427, "Arrggh, outsiders - this is not your realm!");
                            me->PlayOneShotAnimKit(ANIM_KIT_EXCLAIM);
                            HeroicIntermissionTimer = 5000;
                            break;
                        case 6:
                            if (Creature *pMalfur = me->FindNearestCreature(MALFURION_STORMRAGE,500.0f,true))
                            {
                                DoPlaySoundToSet(pMalfur,25169);
                                pMalfur->MonsterYell("Heroes! He is bound. Finish him!", LANG_UNIVERSAL, 0);
                            }
                            HeroicIntermissionTimer = 5000;
                            break;
                        case 7:                     // Break free from stone magma
                            me->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                            if (GameObject * goPlatform = me->FindNearestGameObject(RAGNAROS_PLATFORM,500.0f))
                            {
                                goPlatform->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_DAMAGED);
                                goPlatform->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);
                            }
                            PlayAndYell(24526, "When I finish this, your pathetic mortal world will burn with my vengeance!");
                            me->PlayOneShotAnimKit(ANIM_KIT_ON_LEGS);
                            HeroicIntermissionTimer = 4000;
                            break;
                        case 8:                     // Move at correct z position
                        {
                            HeroicIntermissionTimer = 1500;
                            me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
                            me->SetFloatValue(UNIT_FIELD_COMBATREACH, 40.0f);
                            me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, 40.0f);
                            // Move him bit higher at correct position
                            float z = me->GetMap()->GetHeight2(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
                            me->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), z + 2.0f);
                            break;
                        }
                        case 9:                     // Melee phase
                            me->SetInternalCombatReach(18.0f); // Need to be called after SetFloatValue !!!
                            HeroicIntermissionTimer = NEVER;
                            PHASE = PHASE4;
                            breadthTimer = 3000;
                            geyserTimer = breadthTimer + 10000;
                            spreadFlamesTimer = dreadFlameTimer + 7000;
                            if (me->GetVictim())
                                me->GetMotionMaster()->MoveChase(me->GetVictim());
                            me->SetReactState(REACT_AGGRESSIVE);
                            me->CastSpell(me,SUPERHEATED,true);
                            break;
                    }

                    IntermissionStep++;
                }
                else HeroicIntermissionTimer -= diff;

            }

            if (PHASE == PHASE4) // Last phase on heroic mode
            {
                if (chaseTimer <= diff)
                {
                    if (!me->HasAura(100628) && !me->HasAuraType(SPELL_AURA_MOD_STUN)) // From unknowm reason channeling is interrupted, so for sure manually add aura after cast time if this happen
                        me->AddAura(100628, me);

                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);

                    if (!me->IsNonMeleeSpellCasted(false) && me->GetVictim())
                    {
                        me->GetMotionMaster()->MoveChase(me->GetVictim());
                        chaseTimer = NEVER;
                    }
                }
                else chaseTimer -= diff;

                if (breadthTimer <= diff)
                {
                    Position pos = GetRandomPositionInRadius(40.0f);
                    float x = pos.m_positionX, y = pos.m_positionY, z = pos.m_positionZ;

                    Creature * boF = me->SummonCreature(BREADTH_OF_FROST,x,y,z,0.0f,TEMPSUMMON_TIMED_DESPAWN,120000);
                    if (boF)
                    {
                        if (Creature * cen = me->FindNearestCreature(CENARIUS,500.0f,true))
                            cen->CastSpell(boF,BREADTH_OF_FROST_MISSILE,false); // 3.5 s cast time
                    }
                    breadthTimer = 50000;
                }
                else breadthTimer -= diff;

                if (cloudBurstTimer <= diff)
                {
                    Position pos = GetRandomPositionInRadius(35.0f);
                    float x = pos.m_positionX, y = pos.m_positionY, z = pos.m_positionZ;

                    if (Creature * pMalfurion = me->FindNearestCreature(MALFURION_STORMRAGE,500.0f,true))
                    {
                        Creature * pCloud = pMalfurion->SummonCreature(CLOUDBURST,x,y,z,0.0f);
                        if (pCloud)
                            pMalfurion->CastSpell(pCloud,SUMMON_CLOUDBURST_MISSILE,false);
                    }
                    cloudBurstTimer = NEVER;
                }
                else cloudBurstTimer -= diff;


                if (entrappingRootsTimer <= diff)
                {
                    Position pos = GetRandomPositionInRadius(35.0f);
                    float x = pos.m_positionX, y = pos.m_positionY, z = pos.m_positionZ;

                    Creature * pRoot = me->SummonCreature(ENTRAPPING_ROOTS,x,y,z,0.0f,TEMPSUMMON_TIMED_DESPAWN,90000);

                    if (pRoot)
                    {
                        if (Creature * runTotem = me->FindNearestCreature(CENARIUS,500.0f,true))
                            runTotem->CastSpell(pRoot,ENTRAPPING_ROOOTS_MISSILE,false); // 3 s cast time
                    }

                    entrappingRootsTimer = 56000;
                }
                else entrappingRootsTimer -= diff;

                if (empowerSulfurasTimer <= diff)
                {
                    if (!me->HasAuraType(SPELL_AURA_MOD_STUN))
                    {
                        me->StopMoving();
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                        me->CastSpell(me,EMPOWER_SULFURAS,false);
                        chaseTimer = 5100;
                        if(me->GetVictim())
                            me->GetMotionMaster()->MoveChase(me->GetVictim());
                        // Visual missiles during casting of Empower Sulfuras
                        for (uint8  j = 0; j < 6 ; j++)
                        {
                            Creature * seed = me->SummonCreature(MOLTEN_SEED_CASTER,Seeds_pos[j].GetPositionX(),Seeds_pos[j].GetPositionY(),Seeds_pos[j].GetPositionZ(),0.0f);
                            if (seed)
                            {
                                seed->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
                                seed->ForcedDespawn(15000);
                                seed->CastSpell(me,EMPOWER_SULFURAS_MISSILE,false);
                            }
                        }
                    }
                    empowerSulfurasTimer = 56000;
                }
                else empowerSulfurasTimer -= diff;

                if (geyserTimer <= diff)
                {
                    if (Player * p = RaidIsClusteredTogether())
                    {
                        if (p->IsInWorld()) // Only safety check
                        {
                            // Summom geyser at player location
                            me->SummonCreature(MAGMA_GEYSER_NPC, p->GetPositionX(), p->GetPositionY(), p->GetPositionZ(), 0.0f);

                            //Destroy any nearby Breadth of Frost if nearby of impact
                            if (Creature *boF = p->FindNearestCreature(BREADTH_OF_FROST, 50.0f, true))
                            {
                                // Probably this should be handled via magma geyser spell effect 3, but this is quicker and easier :P
                                if (boF->GetDistance2d(p) <= 5.0f) 
                                    boF->ForcedDespawn();
                            }
                        }
                        geyserTimer = 15000;
                        return;
                    }
                    geyserTimer = 4000;
                }
                else geyserTimer-= diff;

                if (dreadFlameTimer <= diff)
                {
                    uint8 max = Is25ManRaid() ? 6 : 2;
                    lastDreads.clear();

                    std::list<DREADSPOSITIONS> flameList;
                    flameList.clear();

                    // Fill vector with free spots for Dreadflames
                    for (uint32 i = 0; i < 22; i++)
                    for (uint32 j = 0; j < 25; j++)
                    {
                        if (flames[i][j].free && flames[i][j].valid)
                        {
                            DREADSPOSITIONS temp;
                            temp.wx = flames[i][j].x;
                            temp.wy = flames[i][j].y;
                            temp.x = i;
                            temp.y = j;
                            flameList.push_back(temp);
                        }
                    }

                    // If we have enoguh space
                    if (flameList.size() >= max)
                    {
                        for (uint32 i = 0; i < max; i++)
                        {
                            std::list<DREADSPOSITIONS>::iterator j = flameList.begin();
                            advance(j, rand()%flameList.size());

                            Creature * quad = me->SummonCreature(QUAD_STALKER,(*j).wx,(*j).wy,56.0f,0.0f);
                            if (quad)
                            {
                                // Save both coordinated dor dreadlfame
                                DREADSPOSITIONS d; 
                                d.wx = (uint32)quad->GetPositionX();
                                d.wy = (uint32)quad->GetPositionY();
                                d.x = (*j).x;
                                d.y = (*j).y;
                                lastDreads.push_back(d);

                                quad->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_DISABLE_MOVE);
                                quad->ForcedDespawn(10000);
                                me->CastSpell(quad,DREADFLAME_MISSILE,true);
                                flames[(*j).x][(*j).y].free = false; // todo remove this
                                flameList.erase(j);
                            }
                        }
                    }

                    dreadFlameAct = dreadFlameAct - 5000;

                    if (dreadFlameAct < 15000)
                        dreadFlameAct = 15000;
                    dreadFlameTimer = dreadFlameAct;
                }
                else dreadFlameTimer -= diff;

                if (spreadFlamesTimer <= diff)
                {
                    std::list<DREADSPOSITIONS> flameList;
                    flameList.clear();
                    uint32 counter = 0;

                    // Fill vector with free spots for new dreadflames
                    for (uint32 i = 1; i < 21; i++)
                    for (uint32 j = 1; j < 24; j++)
                    {
                        if (flames[i][j].free == false)
                            counter++;

                        if (flames[i][j].free && flames[i][j].valid )
                        {
                            // If this free spot has at least one burning neighbour, add this free spot to list
                            if (flames[i+1][j-1].free ==false || flames[i+1][j].free == false || flames[i+1][j+1].free == false
                            || flames[i][j-1].free ==false || flames[i][j+1].free == false
                            || flames[i-1][j-1].free ==false || flames[i-1][j].free == false || flames[i-1][j+1].free == false)
                            {
                                DREADSPOSITIONS temp;
                                temp.wx = flames[i][j].x;
                                temp.wy = flames[i][j].y;
                                temp.x = i;
                                temp.y = j;
                                flameList.push_back(temp);
                            }
                        }
                    }
                    uint8 max = (Is25ManRaid()) ? 10 : 3;

                    for ( uint8 i = 0; i < max ;i ++)
                    {
                        if (!flameList.empty())
                        {
                            std::list<DREADSPOSITIONS>::iterator j = flameList.begin();
                            advance(j, rand()%flameList.size()); // Pick random

                            Creature * dread = me->SummonCreature(DREADFLAME_SPAWN,(*j).wx,(*j).wy,56.0f,0.0f);
                            if (dread)
                            {
                                dread->AI()->SetData(0,(*j).x);
                                dread->AI()->SetData(1,(*j).y);

                                SetCoord((*j).x,(*j).y,false); // No longer free
                                flameList.erase(j);
                            }
                        }
                    }

                    spreadFlamesTimer = urand(3000,4000);
                }
                else spreadFlamesTimer -= diff;
            }

            if (PHASE == PHASE1 || PHASE == PHASE2 || PHASE == PHASE3 || PHASE == PHASE4) // Ragnaros can attack only during emerged phases
                if (CanCast())
                    DoMeleeAttackIfReady();
        }
    };

};


class npc_Sulfurus_smash : public CreatureScript
{
public:
   npc_Sulfurus_smash() : CreatureScript("npc_Sulfurus_smash") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_Sulfurus_smashAI (creature);
    }

    struct npc_Sulfurus_smashAI : public ScriptedAI
    {
        npc_Sulfurus_smashAI(Creature* creature) : ScriptedAI(creature) 
        {
        }

        uint32 Lava_timer;

        void Reset()
        {
            Lava_timer = 4000;
            me->SetReactState(REACT_AGGRESSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
            me->CastSpell(me,SS_LAVA_POOLS,true); // Lava pools visual
        }

        void Move(Creature * wave,float angle)
        {
            angle = MapManager::NormalizeOrientation(angle);

            if(wave == NULL)
                return;

            wave->CastSpell(wave,LAVA_WAVE,true);
            float x, y, z;
            wave->GetNearPoint2D(x, y, 200.0f, angle);
            z = wave->GetPositionZ();
            wave->GetMotionMaster()->MovePoint(0, x, y, z);
            wave->ForcedDespawn(10000);
            wave->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_PACIFIED);
        }

        void UpdateAI (const uint32 diff)
        {
            if (Lava_timer <= diff)
            {
                float distance = 5.0f;
                float angle = me->GetOrientation();
                Creature * lava_wave;

                me->RemoveAurasDueToSpell(SS_LAVA_POOLS); // visual lava pools
                me->CastSpell(me,SULFURUSH_DMG,false); // Impact damage

                //me->CastSpell(me->GetVictim(),SUMMON_WAVE_N,true); // NORTH
                lava_wave = me->SummonCreature(LAVA_WAVE_NPC,me->GetPositionX() + cos(angle)*distance,me->GetPositionY() + sin(angle)* distance,me->GetPositionZ(),angle,TEMPSUMMON_TIMED_DESPAWN,10000);
                Move(lava_wave,me->GetOrientation());

                //me->CastSpell(me->GetVictim(),SUMMON_WAVE_E,true); // EAST
                angle = me->GetOrientation() - (M_PI/2);
                lava_wave = me->SummonCreature(LAVA_WAVE_NPC,me->GetPositionX() + cos(angle)*distance,me->GetPositionY() + sin(angle)* distance,me->GetPositionZ(),angle,TEMPSUMMON_TIMED_DESPAWN,10000);
                Move(lava_wave,angle);

                //me->CastSpell(me->GetVictim(),SUMMON_WAVE_W,true); // WEST
                angle = me->GetOrientation() + (M_PI/2);
                lava_wave = me->SummonCreature(LAVA_WAVE_NPC,me->GetPositionX() + cos(angle)*distance,me->GetPositionY() + sin(angle)* distance,me->GetPositionZ(),angle,TEMPSUMMON_TIMED_DESPAWN,10000);
                Move(lava_wave,angle);

                me->CastSpell(me,SCORCHED_GROUND_DMG,true);

                me->RemoveAurasDueToSpell(SS_LAVA_POOLS);
                me->ForcedDespawn(11000);

                Lava_timer = NEVER;
            }
            else Lava_timer -= diff;
        }
    };
};


class npc_Lava_wave : public CreatureScript
{
public:
   npc_Lava_wave() : CreatureScript("npc_Lava_wave") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_Lava_waveAI (creature);
    }

    struct npc_Lava_waveAI : public ScriptedAI
    {
        npc_Lava_waveAI(Creature* creature) : ScriptedAI(creature) { }

        void DamageDealt(Unit* victim, uint32& /*damage*/, DamageEffectType typeOfDamage)
        {
            if(typeOfDamage == SPELL_DIRECT_DAMAGE)
                victim->JumpTo(8.0f,15.0f,false); // leap back
        }

        void EnterCombat(Unit* /*who*/) { }
        void AttackStart(Unit* /*who*/) { }
        void MoveInLineOfSight(Unit* /*who*/) { }
    };
};

class Magma_trap_npc : public CreatureScript
{
public:
   Magma_trap_npc() : CreatureScript("Magma_trap_npc") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new Magma_trap_npcAI (creature);
    }

    struct Magma_trap_npcAI : public ScriptedAI
    {
        Magma_trap_npcAI(Creature* creature) : ScriptedAI(creature) 
        {
            erupted = false;
            instance = me->GetInstanceScript();
        }

        bool erupted;
        InstanceScript * instance;

        void MoveInLineOfSight(Unit* who)
        {
            if (erupted == true) // If trap already boomed
                return;

            if (who->ToPlayer() == NULL)
                return;

            if (me->GetExactDist2d(who->GetPositionX(),who->GetPositionY()) <= 10.0f )
            {
                erupted = true;
                me->CastSpell(who,MAGMA_TRAP_ERUPTION,true);
                me->RemoveAurasDueToSpell(MAGMA_TRAP_BURNING);

                if (!who->HasAuraType(SPELL_AURA_DEFLECT_SPELLS))
                    who->KnockbackFrom(who->GetPositionX(),who->GetPositionY(),0.1f,55.1f);

                me->ForcedDespawn(2000);
            }
        }

        void Reset()
        {
            me->SetInCombatWithZone();
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_DISABLE_MOVE|UNIT_FLAG_NON_ATTACKABLE);
            me->SetUInt64Value(UNIT_FIELD_TARGET,0); // Stop turning
            me->CastSpell(me,MAGMA_TRAP_BURNING,true);
            //me->GetMotionMaster()->MoveFall(0);
        }

        void UpdateAI ( const uint32 diff)
        {
            if (instance && instance->GetData(TYPE_RAGNAROS) != IN_PROGRESS)
                me->ForcedDespawn();
        }
    };
};


class npc_Splitting_Sulfuras : public CreatureScript
{
public:
   npc_Splitting_Sulfuras() : CreatureScript("npc_Splitting_Sulfuras") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_Splitting_SulfurasAI (creature);
    }

    struct npc_Splitting_SulfurasAI : public ScriptedAI
    {
        npc_Splitting_SulfurasAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 Summon_sons_timer;

        void Reset()
        {
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
            Summon_sons_timer = 9000;
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_SELECTABLE);
        }

        void DamageTaken(Unit* attacker, uint32& damage)
        {
            damage = 0;
        }

        void UpdateAI (const uint32 diff)
        {
            if (Summon_sons_timer <= diff)
            {
                me->CastSpell(me,SULFURUS_HAMMER_DMG,true); // 45 s dmg aoe arround hammer

                std::list<Creature*> sons;
                me->GetCreatureListWithEntryInGrid(sons, SON_OF_FLAME_NPC, 500.0f);

                for (std::list<Creature*>::iterator itr = sons.begin(); itr != sons.end(); ++itr)
                {
                    me->CastSpell((*itr),INVOKE_SONS,true);
                }

                Creature * rag = NULL;
                rag = me->FindNearestCreature(RAGNAROS,500.0f,true);

                if (rag && rag->HealthBelowPct(40)) // second intermission -> summon 2 Lava Scions
                {
                    rag->SummonCreature(LAVA_SCION,1025.95f,3.5f,55.37f,5.1f,TEMPSUMMON_CORPSE_DESPAWN,0);
                    rag->SummonCreature(LAVA_SCION,1028.05f,-118.75f,55.37f,1.25f,TEMPSUMMON_CORPSE_DESPAWN,0);
                }

                Summon_sons_timer = NEVER;
            }
            else Summon_sons_timer -= diff;

        }
    };
};

class npc_Son_of_flame : public CreatureScript
{
public:
   npc_Son_of_flame() : CreatureScript("npc_Son_of_flame") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_Son_of_flameAI (creature);
    }

    struct npc_Son_of_flameAI : public ScriptedAI
    {
        npc_Son_of_flameAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 Transform_timer;
        uint32 GetPosition_timer;
        uint32 Path_correction_timer;
        bool transformed;
        float x,y,z;
        uint32 HPpercentage;
        float speed_coef;

        void Reset()
        {
            speed_coef = 0.10857132f;
            x = y = z = 0.0f;
            Creature * hammer = me->FindNearestCreature(SPLITTING_SULFURAS,500.0f,true);
            Transform_timer = 18000; // Default time in case we couldn't find ragnaros
            if (hammer)
                Transform_timer = 9000 +  (uint32)( floor( me->GetExactDist2d(hammer)) * 100.0f );
            GetPosition_timer = Transform_timer + 3500;
            Path_correction_timer = Transform_timer + 100;
            transformed = false;
            HPpercentage = 95;
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE| UNIT_FLAG_DISABLE_MOVE);
            //me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
            //me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, false);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);
            me->CastSpell(me,FLAME_PILLAR_TRANSFORM,true);
        }

        void UpdateAI ( const uint32 diff)
        {
            uint32 stacks = 0;
            Aura * a = me->GetAura(BURNING_SPEED);
            if (!a)
                a = me->GetAura(100306);
            if (!a)
                a = me->GetAura(100307);
            if (!a)
                a = me->GetAura(100308);

            if (a)
                stacks = a->GetStackAmount();

            /* WORKING BUT AS FAR AS I KNOW SONS SHOULD BE IMMUNE ALMOST FOR EVERY SLOW EFFECT, SO DONT COUNT WITH IT
            int32 speedReduction = me->GetTotalAuraModifier(SPELL_AURA_MOD_DECREASE_SPEED);
            speedReduction = (speedReduction < -50) ? -50 : speedReduction; // Maximum 50 % movement speed reduction
            speedReduction *= -1;
            */

            float speed = 0.142857 + (speed_coef * stacks); // Base speed + spell_coef * stacks
            //if (speedReduction)
                //speed = (speed * speedReduction) / 100;
            me->SetSpeed(MOVE_RUN, speed, true);

            if (me->HealthBelowPct(HPpercentage) && HPpercentage > 40)
            {
                me->RemoveAuraFromStack(BURNING_SPEED);
                me->RemoveAuraFromStack(100306);
                me->RemoveAuraFromStack(100307);
                me->RemoveAuraFromStack(100308);
                HPpercentage -= 5;
            }

            if (Transform_timer <= diff)
            {
                me->RemoveAura(FLAME_PILLAR_TRANSFORM);
                me->RemoveAura(100133);
                me->RemoveAura(100134);
                me->RemoveAura(100134);

                me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);

                for (uint8 i = 0; i < 10; i++) // At start Sons of flames starting with 10 stacks of burning speed
                {
                    me->CastSpell(me,BURNING_SPEED,true); // Burning speed
                }

                Transform_timer = NEVER;
            }
            else Transform_timer -= diff;

            if (GetPosition_timer <= diff)
            {
                transformed = true;
                me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);

                Creature * hammer = me->FindNearestCreature(SPLITTING_SULFURAS,500.0f,true);

                if (hammer)
                {
                    x = hammer->GetPositionX();
                    y = hammer->GetPositionY();
                    z = hammer->GetPositionZ();

                    if (!me->HasAuraType(SPELL_AURA_MOD_STUN))
                        me->GetMotionMaster()->MovePoint(0,x,y,z);
                }
                GetPosition_timer = NEVER;
            }
            else GetPosition_timer -= diff;


            if (transformed == true &&  x!= 0.0f )
            {
                if ((uint32)me->GetPositionX() == (uint32)x && (uint32)me->GetPositionY() == (uint32)y)
                {
                    me->CastSpell(me,SUPERNOVA,true);
                    me->ForcedDespawn(250);
                    x = 0.0f;
                }
            }

            if (Path_correction_timer <= diff)
            {
                if( x != 0.0f )
                {
                    if (!me->HasAuraType(SPELL_AURA_MOD_STUN))
                        me->GetMotionMaster()->MovePoint(0,x,y,z);
                }

                Path_correction_timer = 1000;
            }
            else Path_correction_timer -=diff;

        }
    };
};

class Engulfing_flame_npc : public CreatureScript
{
public:
   Engulfing_flame_npc() : CreatureScript("Engulfing_flame_npc") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new Engulfing_flame_npcAI (creature);
    }

    struct Engulfing_flame_npcAI : public ScriptedAI
    {
        Engulfing_flame_npcAI(Creature* creature) : ScriptedAI(creature) 
        {
        }

        uint32 Flame_timer;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_DISABLE_MOVE|UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->AddAura(ENGULFING_FLAME_VISUAL,me);
            Flame_timer = 3000;
        }

        void UpdateAI ( const uint32 diff)
        {
            if (Flame_timer <= diff)
            {
                if (IsHeroic())
                    me->ForcedDespawn(3000);
                else
                    me->ForcedDespawn(9000);

                me->CastSpell(me,ENGULFING_FLAME_DMG,true);
                Flame_timer = NEVER;
            }
            else Flame_timer -= diff;
        }
    };
};

class Molten_elemental_npc : public CreatureScript
{
public:
   Molten_elemental_npc() : CreatureScript("Molten_elemental_npc") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new Molten_elemental_npcAI (creature);
    }

    struct Molten_elemental_npcAI : public ScriptedAI
    {
        Molten_elemental_npcAI(Creature* creature) : ScriptedAI(creature) 
        {
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
        }

        uint32 Demorph_timer;
        uint32 morphTimer;

        void Reset()
        {
            morphTimer = NEVER;
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_DISABLE_MOVE|UNIT_FLAG_NON_ATTACKABLE);
            me->SetDisplayId(11686); // Invis model
            Demorph_timer = NEVER;
            me->SetInCombatWithZone();
            me->SetFloatValue(OBJECT_FIELD_SCALE_X,0.9f);
        }

        void DoAction(const int32 action)
        {
            morphTimer = (uint32)action;
        }

        void UpdateAI ( const uint32 diff)
        {
            if (morphTimer <= diff)
            {
                me->CastSpell(me,MOLTEN_SEED,true); // Molten seed efekt ( 10 s duration)
                morphTimer = NEVER;
                Demorph_timer = 10000;
            }
            else morphTimer -= diff;

            if (Demorph_timer <= diff)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_DISABLE_MOVE|UNIT_FLAG_NON_ATTACKABLE);
                me->SetFloatValue(OBJECT_FIELD_SCALE_X,2.0f);
                me->CastSpell(me,MOLTEN_INFERNO,true);
                me->CastSpell(me,MOLTEN_POWER,true);
                me->SetDisplayId(me->GetNativeDisplayId());// Back to Elemental Form

                if (Unit* player = SelectTarget(SELECT_TARGET_RANDOM, 0, 500.0f, true))
                {
                    me->AddThreat(player,5000000.0f);
                    me->GetMotionMaster()->MoveChase(player);
                }
                Demorph_timer = NEVER;
            }
            else Demorph_timer -= diff;

            if (!me->HasFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE)) // Dont attack if we havent morphed yet
                DoMeleeAttackIfReady();
        }
    };
};

class Lava_scion_npc : public CreatureScript
{
public:
   Lava_scion_npc() : CreatureScript("Lava_scion_npc") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new Lava_scion_npcAI (creature);
    }

    struct Lava_scion_npcAI : public ScriptedAI
    {
        Lava_scion_npcAI(Creature* creature) : ScriptedAI(creature) 
        {
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
            instance = me->GetInstanceScript();
        }

        uint32 Blazing_heat_timer;
        InstanceScript * instance;

        void Reset()
        {
            Blazing_heat_timer = 8000;
            me->SetInCombatWithZone();
            if (Unit* player = SelectTarget(SELECT_TARGET_RANDOM, 0, 500.0f, true))
            {
                me->GetMotionMaster()->MoveChase(player);
            }
        }

        void EnterCombat(Unit* who)
        {
            if (instance)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
        }

        void JustDied(Unit * /*victim*/)
        {
            if (instance)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        }

        void CastBlazingHeat(void)
        {
            std::list<Player*> heat_targets;

            std::list<HostileReference*> t_list = me->getThreatManager().getThreatList();
            for (std::list<HostileReference*>::const_iterator itr = t_list.begin(); itr!= t_list.end(); ++itr)
            {
                if ( Unit* unit = Unit::GetUnit(*me, (*itr)->getUnitGuid()))
                {
                    Player * p = unit->ToPlayer();
                    if (p && p->IsInWorld() == true)
                    {
                        if (!p->HasTankSpec() && !p->HasAura(5487)) // Bear form
                        {
                            if (!p->HasAura(BLAZING_HEAT_SIGNALIZER) && !p->HasAura(100981) && !p->HasAura(100982) && !p->HasAura(100983))
                                heat_targets.push_back(p);
                        }
                    }
                }
            }

            if(!heat_targets.empty())
            {
                std::list<Player*>::iterator j = heat_targets.begin();
                advance(j, rand()%heat_targets.size());
                if (*j && (*j)->IsInWorld() == true )
                    me->CastSpell(*j,BLAZING_HEAT_SIGNALIZER,false);
            }
            else
            {
                 Unit* player = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true);
                 if (player)
                    me->CastSpell(player,BLAZING_HEAT_SIGNALIZER,false);
            }

        }

        void UpdateAI ( const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (Blazing_heat_timer <= diff)
            {
                CastBlazingHeat();
                Blazing_heat_timer = 20000;
            }
            else Blazing_heat_timer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class Blazing_heat_npc : public CreatureScript
{
public:
   Blazing_heat_npc() : CreatureScript("Blazing_heat_npc") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new Blazing_heat_npcAI (creature);
    }

    struct Blazing_heat_npcAI : public ScriptedAI
    {
        Blazing_heat_npcAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 Blazing_heat_timer;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_DISABLE_MOVE|UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->ForcedDespawn(40000);
            Blazing_heat_timer = 300;
            me->CastSpell(me,BLAZING_HEAT_DMG_HEAL,true);
        }

        void UpdateAI ( const uint32 diff)
        {
            if (Blazing_heat_timer <= diff)
            {
                me->CastSpell(me,BLAZING_HEAT_DMG_HEAL,true);
                Blazing_heat_timer = NEVER;
            }
            else Blazing_heat_timer -= diff;
        }
    };
};

class Living_meteor_npc : public CreatureScript
{
public:
   Living_meteor_npc() : CreatureScript("Living_meteor_npc") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new Living_meteor_npcAI (creature);
    }

    struct Living_meteor_npcAI : public ScriptedAI
    {
        Living_meteor_npcAI(Creature* creature) : ScriptedAI(creature) 
        {
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(0, IMMUNITY_ID, 100567, false);
            me->ApplySpellImmune(0, IMMUNITY_ID, 100907, false);
        }

        Player * SelectRandomPlayer() // Need to filter Ragnaros tanks
        {
            Creature * rag;
            Unit * p;
            rag = me->FindNearestCreature(RAGNAROS,500.0f,true);
            if (rag == NULL)
                return NULL;

             p = rag->AI()->SelectTarget(SELECT_TARGET_NEAREST, 2, 500.0f, true); // Ignore rag's tank + offtank

            if (p == NULL)
                p = rag->AI()->SelectTarget(SELECT_TARGET_NEAREST, 0, 500.0f, true);

            if (p == NULL)
                return NULL;

            return p->ToPlayer();
        }

        uint32 Chasin_timer;
        uint32 Combustible_timer;
        uint32 ClearRoot_timer;

        void Reset()
        {
            ClearRoot_timer = Combustible_timer = NEVER; // Not now
            Chasin_timer = 1500;
            me->SetReactState(REACT_AGGRESSIVE);
            me->SetInCombatWithZone();
            me->CastSpell(me,METEOR_DMG_REDUCTION,true);
            me->CastSpell(me,COMBUSTIBLE,true);
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
        }

        void SpellHit(Unit* caster, SpellEntry const* spell)
        {
            if (spell->Id == 100907 || spell->Id == 100567) // Freeze through transmission or walking over BoF
                me->StopMoving();

            if (caster == me && spell->SpellIconID == 5520 && spell->SpellFamilyName == SPELLFAMILY_GENERIC  ) // Combustion
            {
                me->RemoveAura(100249);
                me->RemoveAura(100250);

                if (me->HasAura(COMBUSTIBLE) || me->HasAura(100282) ||me->HasAura(100283) ||me->HasAura(100284) ) // Combustible spell difficulties variants
                {
                    me->RemoveAura(COMBUSTIBLE);
                    me->RemoveAura(100282);
                    me->RemoveAura(100283);
                    me->RemoveAura(100284);

                    me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE); // few  seconds after knockback, meteor should stay on place

                    if (IsHeroic()) // In Heroic Difficulty Combustion inflicts 2000 Fire damage every second on the triggering player. This effect stacks.
                    {
                        if (Unit * vict = me->GetVictim())
                            vict->CastSpell(vict,100249,true);
                    }

                    if (Unit *victim = me->GetVictim())
                    {
                        uint32 dist = (uint32) me->GetDistance(victim);

                        bool canJump = (me->GetExactDist2d(MIDDLE_X,MIDDLE_Y) >= 58.0f ) ? false : true;

                        if (dist <= 10 && canJump)
                            me->JumpTo(8.0f, 5.0f,false);
                        else if (dist <= 20 && canJump)
                            me->JumpTo(15.0f, 8.0f,false);
                        else if (canJump)
                            me->JumpTo(15.0f, 8.0f,false);
                    }
                    me->InterruptNonMeleeSpells(false);
                    //me->GetMotionMaster()->MoveJump(me->GetPositionX()+cos(me->GetOrientation() + M_PI)*8,me->GetPositionY()+sin(me->GetOrientation() + M_PI)*8,me->GetPositionZ(),15.0f,10.0f);
                    //me->JumpTo(15.0f, 8.0f,false); // Leap back - spell effect 144 on Combustion effect didn't work properly
                    Combustible_timer = 5000; // Trigger Combustible effect again in 5 s
                    Chasin_timer = 4000;
                }
            }
        }

        void StartChasing()
        {
            me->CastSpell(me,DUMMY_AOE,true); // used in spellscript for explosion if someone is in melee range
            me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);

            Player * p = SelectRandomPlayer(); // Ignore Ragnaros tanks if possible

            if (p)
            {
                me->getThreatManager().resetAllAggro();
                me->AddThreat(p,99999999.0f);
                me->CastSpell(p,FIXATE,false);
                me->ClearUnitState(UNIT_STATE_CASTING);
            }
        }

        void DoAction(const int32 action) // This will be called if dummy aoe spell hits a player ( in melee range ) via spellscript
        {
            if (action == EXPLODE && !me->HasAura(100567)) // Not during stun caused by BoF
            {
                me->CastSpell(me,MATEOR_EXPLOSION,true);
                me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
                me->InterruptNonMeleeSpells(false);
                ClearRoot_timer = 2000;
                Chasin_timer = 1500;
            }
        }

        void UpdateAI ( const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (Unit * vic = me->GetVictim())
            {
                if (!me->HasFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE) && !me->HasAura(100907) && !me->HasAura(100567)) // Freeze through transmission
                    me->GetMotionMaster()->MovePoint(0,vic->GetPositionX(),vic->GetPositionY(),vic->GetPositionZ(),false,false);
            }

            if (Chasin_timer <= diff) // Few seconds after spawn Metoer should be passive
            {
                StartChasing(); 
                Chasin_timer = NEVER;
            }
            else Chasin_timer -= diff;

            if (Combustible_timer <= diff)
            {
                me->CastSpell(me,COMBUSTIBLE,true);
                Combustible_timer = NEVER;
            }
            else Combustible_timer-= diff;

            if (ClearRoot_timer <= diff)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
                ClearRoot_timer = NEVER;
            }
            else ClearRoot_timer -= diff;
        }
    };
};


enum TrashSpells
{
    MOLTEN_BLAST    = 99613,
    MOLTEN_BOLT     = 99579,
    RAISE_LAVA      = 99503
};

const Position telePositions[2] =           // Molten Erupter teleport positions
{
    {909.83f,-40.73f,45.60f, 0.0f},
    {910.28f,-74.80f,45.6f, 0.0f},
};

const Position LavaPosition[11] =           // Lava Flow position between pillars
{
    {775.0f,-47.00f,86.32f, 4.7f}, // Between pillars ( 0 - 2 )
    {740.0f,-47.00f,86.32f, 4.7f},
    {701.0f,-47.00f,86.32f, 4.7f},

    {748.0f,-47.00f,86.32f, 4.7f}, // On pillars ( 3 - 5 )
    {727.0f,-47.00f,86.32f, 4.7f},
    {707.0f,-47.00f,86.32f, 4.7f},

    {711.0f,-47.00f,86.32f, 4.7f}, // Corners ( 6 - 10 )
    {704.0f,-47.00f,86.32f, 4.7f},
    {697.0f,-47.00f,86.32f, 4.7f},
    {782.0f,-47.00f,86.32f, 4.7f},
    {772.0f,-47.00f,86.32f, 4.7f},
};

#define LEFT_ERUPTER  0
#define RIGHT_ERUPTER 1

class Lava_Wielder_npc : public CreatureScript
{
public:
   Lava_Wielder_npc() : CreatureScript("Lava_Wielder_npc") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new Lava_Wielder_npcAI (creature);
    }

    struct Lava_Wielder_npcAI : public ScriptedAI
    {
        Lava_Wielder_npcAI(Creature* creature) : ScriptedAI(creature) 
        {
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
        }

        uint32 raiseLavaTimer;

        void DestroyWalls(void)
        {
            GameObject * wall = NULL;

            wall = me->FindNearestGameObject(FIREWALL_TRASH,200.0f);

            if (wall)
                wall->Delete();

            wall = me->FindNearestGameObject(FIREWALL_TRASH,200.0f);

            if (wall)
                wall->Delete();
        }

        void SummonLava(uint32 from, uint32 to)
        {
            for (uint32 i = from; i <= to ; i++)
            {
                me->SummonCreature(LAVA_BUNNY,LavaPosition[i].GetPositionX(),LavaPosition[i].GetPositionY(),LavaPosition[i].GetPositionZ(),LavaPosition[i].GetOrientation(),TEMPSUMMON_TIMED_DESPAWN,17000);
            }
        }

        void SpawnLavaFlows(void)
        {
            switch(urand(0,2))
            {
                case 0:
                    SummonLava(0,2);
                    break;
                case 1:
                    SummonLava(3,5);
                    break;
                case 2:
                    SummonLava(6,10);
                    break;
                default:
                    break;
            }
        }

        void Reset()
        {
            raiseLavaTimer = 5000;
        }

        void EnterCombat(Unit * /*who*/)
        {
            //me->SummonGameObject(FIREWALL_TRASH,690.82f,-60.1f,86.32f,2.69f,0,0,0,0,0); // Wall to Majordomo
            //me->SummonGameObject(FIREWALL_TRASH,791.59f,-59.40f,86.32f,5.742f,0,0,0,0,0); // Wall to Ragnaros
            me->SetInCombatWithZone();
        }

        void EnterEvadeMode()
        {
            DestroyWalls();
            ScriptedAI::EnterEvadeMode();
        }

        void JustDied()
        {
            DestroyWalls();
        }

        void UpdateAI ( const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (raiseLavaTimer <= diff)
            {
                SpawnLavaFlows();
                me->CastSpell(me,RAISE_LAVA,false);
                raiseLavaTimer = 17000;
            }
            else raiseLavaTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class Lava_bunny_npc : public CreatureScript
{
public:
   Lava_bunny_npc() : CreatureScript("Lava_bunny_npc") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new Lava_bunny_npcAI (creature);
    }

    struct Lava_bunny_npcAI : public ScriptedAI
    {
        Lava_bunny_npcAI(Creature* creature) : ScriptedAI(creature) 
        {
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE|UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE);
        }

        uint32 flowTimer;

        void Reset()
        {
            flowTimer = 2000;
        }

        void EnterCombat(Unit* /*who*/) { }
        void AttackStart(Unit* /*who*/) { }
        void MoveInLineOfSight(Unit* /*who*/) { }

        void UpdateAI ( const uint32 diff)
        {
            if (flowTimer <= diff)
            {
                me->AddAura(RAISE_LAVA,me);
                flowTimer = NEVER;
            }
            else flowTimer -= diff;
        }
    };
};

class Molten_erupter_npc : public CreatureScript
{
public:
   Molten_erupter_npc() : CreatureScript("Molten_erupter_npc") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new Molten_erupter_npcAI (creature);
    }

    struct Molten_erupter_npcAI : public ScriptedAI
    {
        Molten_erupter_npcAI(Creature* creature) : ScriptedAI(creature) 
        {
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
            me->SetFloatValue(UNIT_FIELD_COMBATREACH,20.0f);
            me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS,15.0f);

            if (me->GetPositionY() < -50.0f)
                type = RIGHT_ERUPTER;
            else
                type = LEFT_ERUPTER;

            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
        }

        uint32 moltenBoltTimer;
        uint32 checkMeleeTimer;
        uint32 burryTimer;
        uint32 emergeTimer;
        uint32 teleTimer;

        bool inHome;

        uint32 type;

        void Reset()
        {
            me->SetReactState(REACT_AGGRESSIVE);
            inHome = true;
            me->SetVisible(true);

            moltenBoltTimer = 8000;
            checkMeleeTimer = 2500;
            burryTimer      = urand(10000,20000);
            teleTimer       = NEVER;
            emergeTimer     = NEVER;
        }

        void EnterCombat(Unit * who)
        {
            Creature * erupter = NULL;

            std::list<Creature*> erupters;
            me->GetCreatureListWithEntryInGrid(erupters, MOLTEN_ERUPTER, 100.0f);

            for (std::list<Creature*>::iterator itr = erupters.begin(); itr != erupters.end(); ++itr)
            {
                    if ( (*itr)->GetGUID() != me->GetGUID())
                        erupter = (*itr);
            }

            if (erupter)
                erupter->SetInCombatWithZone();
            me->SetInCombatWithZone();
        }

        void EnterEvadeMode()
        {
            me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
            ScriptedAI::EnterEvadeMode();
        }

        void JustReachedHome()
        {
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
        }

        void UpdateAI ( const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (moltenBoltTimer <= diff)
            {
                if (Unit * player = SelectTarget(SELECT_TARGET_RANDOM,0,100.0f,true))
                {
                    if (!me->HasAura(SUBMERGE_STATE_AURA))
                        me->CastSpell(player,MOLTEN_BOLT,false);
                }

                moltenBoltTimer = 15000;
            }
            else moltenBoltTimer -= diff;

            if (checkMeleeTimer <= diff)
            {
                if (!me->IsWithinMeleeRange(me->GetVictim()) || !me->IsWithinLOSInMap(me->GetVictim()))
                {
                    if (!me->HasAura(SUBMERGE_STATE_AURA))
                        me->CastSpell(me->GetVictim(),MOLTEN_BLAST,false);
                }
                checkMeleeTimer = 2000;
            }
            else checkMeleeTimer -= diff;

            if (burryTimer <= diff)
            {
                me->CastSpell(me,SUBMERGE_STATE_AURA,true);
                burryTimer  = urand(15000,25000);
                teleTimer = 2000;
            }
            else burryTimer -= diff;

            if (teleTimer <= diff)
            {
                me->SetReactState(REACT_PASSIVE);
                me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
                me->SetVisible(false);

                if (inHome == true)
                {
                    if (type == RIGHT_ERUPTER)
                        me->GetMotionMaster()->MovePoint(0,telePositions[0].GetPositionX(),telePositions[0].GetPositionY(),telePositions[0].GetPositionZ());
                    else
                        me->GetMotionMaster()->MovePoint(0,telePositions[1].GetPositionX(),telePositions[1].GetPositionY(),telePositions[1].GetPositionZ());

                    inHome = false;
                }
                else
                {
                    me->GetMotionMaster()->MoveTargetedHome();
                    inHome = true;
                }

                teleTimer = NEVER;
                emergeTimer = 2000;
            }
            else teleTimer -= diff;

            if (emergeTimer <= diff)
            {
                me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
                me->SetReactState(REACT_AGGRESSIVE);
                me->SetVisible(true);
                me->RemoveAura(SUBMERGE_STATE_AURA);
                emergeTimer = NEVER;
            }
            else emergeTimer -= diff;

            if (!me->HasAura(SUBMERGE_STATE_AURA))
                DoMeleeAttackIfReady();
        }
    };
};

class IsNotLavaNpc
{
    public:
        bool operator()(WorldObject* object) const
        {
            if (object->ToCreature() && object->ToCreature()->GetEntry() == LAVA_BUNNY)
                    return false;
            return true;
        }
};

class spell_gen_raise_lava : public SpellScriptLoader
{
    public:
        spell_gen_raise_lava() : SpellScriptLoader("spell_gen_raise_lava") { }

        class spell_gen_raise_lava_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_raise_lava_SpellScript);

            void RemoveInvalidTargets(std::list<Unit*>& unitList)
            {
                unitList.remove_if(IsNotLavaNpc());
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_raise_lava_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_AREA_ENTRY_SRC);// 7
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_raise_lava_SpellScript();
        }
};

class spell_gen_molten_inferno : public SpellScriptLoader
{
public:
    spell_gen_molten_inferno() : SpellScriptLoader("spell_gen_molten_inferno") {}

    class spell_gen_molten_inferno_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_gen_molten_inferno_SpellScript);

        void HandleDamage(SpellEffIndex /*effIndex*/)
        {
            Unit * caster = GetCaster();
            Unit * hit_unit = GetHitUnit();

            if (!caster || !hit_unit )
                return;

            float damage = 0.0f;
            float distance = caster->GetExactDist2d(hit_unit->GetPositionX(),hit_unit->GetPositionY());

            damage = GetHitDamage() / ( pow((double)(distance + 1),0.65) ); // Approx. is correct ( Thanks to Gregory :P )

            SetHitDamage((int32)damage);
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_gen_molten_inferno_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_gen_molten_inferno_SpellScript();
    }
};


class spell_gen_blazing_heat : public SpellScriptLoader
{
    public:
        spell_gen_blazing_heat() : SpellScriptLoader("spell_gen_blazing_heat") { }

        class spell_gen_blazing_heat_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_gen_blazing_heat_AuraScript);

            void HandleTick(AuraEffect const* aurEff)
            {
                Unit* target = GetTarget();

                if(target)
                {
                    target->SummonCreature(BLAZING_HEAT,target->GetPositionX(),target->GetPositionY(),target->GetPositionZ(),0.0f,TEMPSUMMON_CORPSE_DESPAWN,0);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_gen_blazing_heat_AuraScript::HandleTick, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_gen_blazing_heat_AuraScript();
        }
};


class spell_living_meteor_dummy : public SpellScriptLoader
{
    public:
        spell_living_meteor_dummy() : SpellScriptLoader("spell_living_meteor_dummy") { }

        class spell_living_meteor_dummy_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_living_meteor_dummy_SpellScript);

            void HandleDummyHitTarget(SpellEffIndex /*effIndex*/)
            {
                    Unit * caster = GetCaster();

                    if (!GetHitUnit()->ToPlayer()) // Only players
                        return;

                    if (caster && caster->ToCreature() && !caster->HasFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE) )
                        caster->ToCreature()->AI()->DoAction(EXPLODE);
            }

            void Register()
            {
                OnEffect += SpellEffectFn(spell_living_meteor_dummy_SpellScript::HandleDummyHitTarget, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_living_meteor_dummy_SpellScript();
        }
};


class IsNotMoltenElemental
{
    public:
        bool operator()(WorldObject* object) const
        {
            if (object->ToCreature() && object->ToCreature()->GetEntry() == MOLTEN_ELEMENTAL)
                    return false;
            return true;
        }
};

class spell_gen_molten_power : public SpellScriptLoader
{
    public:
        spell_gen_molten_power() : SpellScriptLoader("spell_gen_molten_power") { }

        class spell_gen_molten_power_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_molten_power_SpellScript);

            void RemoveInvalidTargets(std::list<Unit*>& unitList)
            {
                unitList.remove_if(IsNotMoltenElemental());
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_molten_power_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_AREA_ENTRY_SRC);// 7
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_molten_power_SpellScript::RemoveInvalidTargets, EFFECT_1, TARGET_UNIT_AREA_ENTRY_SRC);// 7
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_molten_power_SpellScript::RemoveInvalidTargets, EFFECT_2, TARGET_UNIT_AREA_ENTRY_SRC);// 7
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_molten_power_SpellScript();
        }
};

class IsNotLivingMeteor
{
    public:
        bool operator()(WorldObject* object) const
        {
            if (object->ToCreature() && object->ToCreature()->GetEntry() == LIVING_METEOR)
                    return false;
            return true;
        }
};

class spell_gen_lava_wave : public SpellScriptLoader
{
    public:
        spell_gen_lava_wave() : SpellScriptLoader("spell_gen_lava_wave") { }

        class spell_gen_lava_wave_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_lava_wave_SpellScript);

            void RemoveInvalidTargets(std::list<Unit*>& unitList)
            {
                unitList.remove_if(IsNotLivingMeteor());
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_lava_wave_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_AREA_ALLY_SRC);// 8
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_lava_wave_SpellScript();
        }
};


class IsNotSonOrLavaScion
{
    public:
        bool operator()(WorldObject* object) const
        {
            if (object->ToCreature())
                if(object->ToCreature()->GetEntry() == SON_OF_FLAME_NPC)
                    return false;
            return true;
        }
};

class spell_gen_blazing_heat_heal : public SpellScriptLoader
{
    public:
        spell_gen_blazing_heat_heal() : SpellScriptLoader("spell_gen_blazing_heat_heal") { }

        class spell_gen_blazing_heat_heal_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_blazing_heat_heal_SpellScript);

            void RemoveInvalidTargets(std::list<Unit*>& unitList)
            {
                unitList.remove_if(IsNotSonOrLavaScion()); // Spell should heal only Lava Scions and Sons of flame
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_blazing_heat_heal_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_AREA_ALLY_SRC);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_blazing_heat_heal_SpellScript();
        }
};

class spell_gen_world_in_flames : public SpellScriptLoader
{
    public:
        spell_gen_world_in_flames() : SpellScriptLoader("spell_gen_world_in_flames") { }

        class spell_gen_world_in_flames_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_gen_world_in_flames_AuraScript);

            uint32 lastNumber;
            uint32 flameCounter;

            bool Load()
            {
                lastNumber = 0;
                flameCounter = 0;
                return true;
            }

            void HandleFlames(AuraEffect const* aurEff)
            {
                PreventDefaultAction();
                Unit* caster = GetCaster();

                if (!caster || !caster->ToCreature())
                    return;

                if (flameCounter > 2) // Maximum 3 times
                    return;

                    uint32 randNum = urand(1,3); // Spawn on 3 random locations

                    if (randNum == lastNumber) // Dont spawn flames at same position
                    {
                        if (randNum == 1)
                            randNum = urand(2,3);
                        else
                        if (randNum == 2)
                            randNum = (urand(0,1)) ? 1 : 3;
                        else // 3
                            randNum = urand(1,2);
                    }

                    if (boss_ragnaros_firelands::boss_ragnaros_firelandsAI* pAI = (boss_ragnaros_firelands::boss_ragnaros_firelandsAI*)(caster->ToCreature()->GetAI()))
                    {
                        if (!caster->IsNonMeleeSpellCasted(false) && pAI->CanCast())
                            caster->CastSpell(caster, ENGULFING_FLAMES_HC, false); // Only for visual casting

                        switch(randNum)
                        {
                            case 1 :
                                pAI->SpawnEngulfingFlames(engulfing_lengths[randNum - 1],1.6f,5.2f);
                            break;

                            case 2 :
                                pAI->SpawnEngulfingFlames(engulfing_lengths[randNum - 1],1.6f,4.6f);
                            break;

                            case 3:
                                pAI->SpawnEngulfingFlames(engulfing_lengths[2],2.12f,4.33f);
                                pAI->SpawnEngulfingFlames(engulfing_lengths[3],2.12f,4.33f);
                            break;
                        }
                    }
                    lastNumber = randNum;
                    flameCounter++;
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_gen_world_in_flames_AuraScript::HandleFlames, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_gen_world_in_flames_AuraScript();
        }
};

class spell_gen_frost_freeze : public SpellScriptLoader
{
    public:
        spell_gen_frost_freeze() : SpellScriptLoader("spell_gen_frost_freeze") { }

        class spell_gen_frost_freeze_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_frost_freeze_SpellScript);

            void RemoveInvalidTargets(std::list<Unit*>& unitList)
            {
                unitList.remove_if(IsNotLivingMeteor());
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_frost_freeze_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_AREA_ENTRY_SRC);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_frost_freeze_SpellScript();
        }
};

class spell_gen_breadthOfFrostFreeze : public SpellScriptLoader
{
    public:
        spell_gen_breadthOfFrostFreeze() : SpellScriptLoader("spell_gen_breadthOfFrostFreeze") { }

        class spell_gen_breadthOfFrostFreeze_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_breadthOfFrostFreeze_SpellScript);

            void RemoveInvalidTargets(std::list<Unit*>& unitList)
            {
                unitList.remove_if(IsNotLivingMeteor());
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_breadthOfFrostFreeze_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_AREA_ENTRY_SRC);
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_breadthOfFrostFreeze_SpellScript::RemoveInvalidTargets, EFFECT_1, TARGET_UNIT_AREA_ENTRY_SRC);
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_breadthOfFrostFreeze_SpellScript::RemoveInvalidTargets, EFFECT_2, TARGET_UNIT_AREA_ENTRY_SRC);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_breadthOfFrostFreeze_SpellScript();
        }
};

class spell_gen_breadthOfFrostBarrier : public SpellScriptLoader
{
    public:
        spell_gen_breadthOfFrostBarrier() : SpellScriptLoader("spell_gen_breadthOfFrostBarrier") { }

        class spell_gen_breadthOfFrostBarrier_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_breadthOfFrostBarrier_SpellScript);

            void RefreshBarrier(std::list<Unit*>& unitList)
            {
                for (std::list<Unit*>::iterator itr = unitList.begin() ; itr != unitList.end();)
                {
                    if ((*itr)->IsInWorld())
                    {
                        if (Aura * a = (*itr)->GetAura(100503)) // If already has active buff, only refresh it
                        {
                            a->RefreshDuration();
                            itr = unitList.erase(itr);
                        }
                        else ++itr;
                    }
                    else
                        ++itr;
                }
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_breadthOfFrostBarrier_SpellScript::RefreshBarrier, EFFECT_0, TARGET_UNIT_AREA_ALLY_SRC);
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_breadthOfFrostBarrier_SpellScript::RefreshBarrier, EFFECT_1, TARGET_UNIT_AREA_ALLY_SRC);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_breadthOfFrostBarrier_SpellScript();
        }
};

class HasDeluge
{
    public:
        bool operator()(WorldObject* object) const
        {
            if (Player * pl = object->ToPlayer())
                if(pl->HasAura(DELUGE) || pl->HasAura(101015) || pl->HasAura(100503)) // deluge + breadth of frost
                {
                    pl->RemoveAura(SUPERHEATED_DEBUFF);
                    pl->RemoveAura(100915); // diff variant
                    return true;
                }
            return false;
        }
};

class spell_gen_superheated : public SpellScriptLoader
{
    public:
        spell_gen_superheated() : SpellScriptLoader("spell_gen_superheated") { }

        class spell_gen_superheated_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_superheated_SpellScript);

            void RemoveInvalidTargets(std::list<Unit*>& unitList)
            {
                unitList.remove_if(HasDeluge());
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_superheated_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_AREA_ENEMY_SRC);
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_superheated_SpellScript::RemoveInvalidTargets, EFFECT_1, TARGET_UNIT_AREA_ENEMY_SRC);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_superheated_SpellScript();
        }
};

class go_firelands_portal : public GameObjectScript
{
public:
    go_firelands_portal() : GameObjectScript("go_firelands_portal") { }

    bool OnGossipHello(Player *pPlayer, GameObject *pGO)
    {
        if (pPlayer == NULL || !pPlayer->IsInWorld())
            return true;

        InstanceScript * instance = pPlayer->GetInstanceScript();

        if (instance == NULL)
            return true;

        Creature * pBaleroc = instance->instance->GetCreature(instance->GetData64(TYPE_BALEROC));

        if (pBaleroc == NULL)
        {
            pPlayer->NearTeleportTo(360.63f,-63.93f,77.52f,0.06f);
            return true;
        }

        if (pBaleroc && pBaleroc->isDead())
            pPlayer->NearTeleportTo(360.63f,-63.93f,77.52f,0.06f);

        return true;
    }
};

class npc_cloudburst : public CreatureScript
{
public:
   npc_cloudburst() : CreatureScript("npc_cloudburst") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_cloudburstAI (creature);
    }

    struct npc_cloudburstAI : public ScriptedAI
    {
        npc_cloudburstAI(Creature* creature) : ScriptedAI(creature) 
        {
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
            instance = me->GetInstanceScript();
            maxBubbles = (Is25ManRaid()) ? 3 : 1;
            bubbles = 0;
        }

        InstanceScript * instance;
        uint32 transformTimer;
        uint32 maxBubbles;
        uint32 bubbles;

        void EnterEvadeMode() { }
        void EnterCombat(Unit* /*enemy*/) {}
        void DamageTaken(Unit* /*who*/, uint32 &damage) { damage = 0; }
        void Reset()
        {
            transformTimer = 500;
        }

        void UpdateAI (const uint32 diff)
        {
            if (transformTimer <= diff)
            {
                me->CastSpell(me,CLOUDBURST_VISUAL,true);
                me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                transformTimer = NEVER;
            }
            else transformTimer -= diff;
        }

        void SpellHit(Unit* caster, const SpellEntry* spell)
        {
            if (spell->Id == 110469) // Borrowed harmless spell
            {
                if (bubbles < maxBubbles)
                {
                    bubbles++;
                    caster->CastSpell(caster,DELUGE,true);
                    me->ForcedDespawn();
                }
            }
        }

    };
};

class npc_breadthOfFrost : public CreatureScript
{
public:
   npc_breadthOfFrost() : CreatureScript("npc_breadthOfFrost") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_breadthOfFrostAI (creature);
    }

    struct npc_breadthOfFrostAI : public ScriptedAI
    {
        npc_breadthOfFrostAI(Creature* creature) : ScriptedAI(creature) 
        {
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_DISABLE_MOVE);
        }

        bool morphed;

        void SpellHit(Unit* pSrc, const SpellEntry* spell)
        {
            if (spell->Id == BREADTH_OF_FROST_MISSILE)
            {
                me->CastSpell(me,BREADTH_OF_FROST_TRIGGERED,true);
                morphed = true;
            }
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (morphed && who && who->ToCreature() && me->GetExactDist2d(who) <= 6.0f)
            {
                if (who->ToCreature()->GetEntry() == LIVING_METEOR)
                {
                    who->AddAura(100567,who); // Root stun
                    who->StopMoving();
                    me->ForcedDespawn();
                }
            }
        }

        void Reset(){ morphed = false;}
        void EnterEvadeMode() { }
        void EnterCombat(Unit* /*enemy*/) {}
        void DamageTaken(Unit* /*who*/, uint32 &damage) { damage = 0; }

    };
};

class npc_entrappingRoots : public CreatureScript
{
public:
   npc_entrappingRoots() : CreatureScript("npc_entrappingRoots") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_entrappingRootsAI (creature);
    }

    struct npc_entrappingRootsAI : public ScriptedAI
    {
        npc_entrappingRootsAI(Creature* creature) : ScriptedAI(creature) 
        {
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_DISABLE_MOVE);
        }

        bool transformed;

        void Reset() { transformed = false;}

        void SpellHit(Unit* pSrc, const SpellEntry* spell)
        {
            if (spell->Id == ENTRAPPING_ROOOTS_MISSILE)
            {
                me->CastSpell(me,ENTRAPPING_ROOTS_TRIGGERED,true);
                transformed = true;
            }
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (transformed && who && who->ToCreature() && me->GetExactDist2d(who) <= 16.0f)
            {
                if (who->ToCreature()->GetEntry() == RAGNAROS)
                {
                    who->AddAura(100653,who); // Root stun
                    me->ForcedDespawn();
                }
            }
        }

        void EnterEvadeMode() { }
        void EnterCombat(Unit* /*enemy*/) {}
        void DamageTaken(Unit* /*who*/, uint32 &damage) { damage = 0; }

    };
};

typedef struct coord
{
    uint32 x;
    uint32 y;
}COORDINATE;

class npc_dreadFlame : public CreatureScript
{
public:
   npc_dreadFlame() : CreatureScript("npc_dreadFlame") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_dreadFlameAI (creature);
    }

    struct npc_dreadFlameAI : public ScriptedAI
    {
        npc_dreadFlameAI(Creature* creature) : ScriptedAI(creature) 
        {
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_DISABLE_MOVE);
            summonerGUID = 0;
        }
        uint32 mx,my;
        uint64 summonerGUID;

        void Reset()
        {
            me->CastSpell(me,DREADFLAME_AOE,true);
            me->CastSpell(me,DREADFLAME_CONTROL_AURA,true);
        }

        void SetData(uint32 type, uint32 data)
        {
            if (type == 0)
                mx = data;
            else if (type == 1)
                my = data;
            else
            {
                if (Unit * rag = Unit::GetUnit(*me,summonerGUID))
                    if (rag->ToCreature())
                        if (boss_ragnaros_firelands::boss_ragnaros_firelandsAI* pAI = (boss_ragnaros_firelands::boss_ragnaros_firelandsAI*)(rag->ToCreature()->GetAI()))
                        {
                            pAI->SetCoord(mx,my,true); // Free again
                            me->ForcedDespawn();
                        }
            }
        }

        void EnterEvadeMode() { }
        void EnterCombat(Unit* /*enemy*/) {}
        void DamageTaken(Unit* /*who*/, uint32 &damage) { damage = 0; }

        void IsSummonedBy(Unit* pSummoner)
        {
            if (pSummoner && pSummoner->ToCreature())
                summonerGUID = pSummoner->GetGUID();
        }

       void UpdateAI(const uint32 diff)
       {
       }

    };
};

class magma_geyser_npc : public CreatureScript
{
public:
   magma_geyser_npc() : CreatureScript("magma_geyser_npc") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new magma_geyser_npcAI (creature);
    }

    struct magma_geyser_npcAI : public ScriptedAI
    {
        magma_geyser_npcAI(Creature* creature) : ScriptedAI(creature) 
        {
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_DISABLE_MOVE|UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_AGGRESSIVE);
            me->ForcedDespawn(12000);
        }

        uint32 geyserTimer;

        void Reset()
        {
            geyserTimer = 500;
        }

        void UpdateAI ( const uint32 diff)
        {
            if (geyserTimer <= diff)
            {
                me->CastSpell(me,MAGMA_GEYSER,true);
                geyserTimer = NEVER;
            }
            else geyserTimer -= diff;
        }
    };
};

class npc_firelands_cenarius : public CreatureScript
{
public:
   npc_firelands_cenarius() : CreatureScript("npc_firelands_cenarius") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_firelands_cenariusAI (creature);
    }

    struct npc_firelands_cenariusAI : public ScriptedAI
    {
        npc_firelands_cenariusAI(Creature* creature) : ScriptedAI(creature) 
        {
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);
        }

        uint32 talkTimer;
        uint32 talks;
        bool canStart;

        void DoAction(const int32 param) // Start post intro
        {
            talkTimer = 15000;
            canStart = true;
        }

        void Reset()
        {
            talks = 0;
            canStart = false;
        }

        void UpdateAI ( const uint32 diff)
        {
            if (canStart == false)
                return;

            if(talkTimer <= diff)
            {
                switch (talks)
                {
                    case 0 :
                        if (Creature * pMalfurion = me->FindNearestCreature(MALFURION_STORMRAGE,500.0f,true))
                        {
                            pMalfurion->MonsterYell(HeroicDeath[1].text,LANG_UNIVERSAL,0);
                            DoPlaySoundToSet(pMalfurion,HeroicDeath[1].sound);
                        }
                        talkTimer = 3000;
                        break;
                    case 1:
                        me->MonsterYell(HeroicDeath[2].text,LANG_UNIVERSAL,0);
                        DoPlaySoundToSet(me,HeroicDeath[2].sound);
                        talkTimer = 2000;
                        break;
                    case 2:
                        if (Creature * pMalfurion = me->FindNearestCreature(MALFURION_STORMRAGE,500.0f,true))
                        {
                            pMalfurion->MonsterYell(HeroicDeath[3].text,LANG_UNIVERSAL,0);
                            DoPlaySoundToSet(pMalfurion,HeroicDeath[3].sound);
                        }
                        talkTimer = 6000;
                        break;
                    case 3:
                        me->MonsterYell(HeroicDeath[4].text,LANG_UNIVERSAL,0);
                        DoPlaySoundToSet(me,HeroicDeath[4].sound);
                        talkTimer = 10500;
                        break;
                    case 4:
                        if (Creature * pHamuul = me->FindNearestCreature(HAMUUL_RUNETOTEM,500.0f,true))
                        {
                            pHamuul->MonsterYell(HeroicDeath[5].text,LANG_UNIVERSAL,0);
                            DoPlaySoundToSet(pHamuul,HeroicDeath[5].sound);
                        }
                        talkTimer = 13500;
                        break;
                    case 5:
                        me->MonsterYell(HeroicDeath[6].text,LANG_UNIVERSAL,0);
                        DoPlaySoundToSet(me,HeroicDeath[6].sound);
                        talkTimer = NEVER;
                        canStart = false;
                        if (Creature * pHamuul = me->FindNearestCreature(HAMUUL_RUNETOTEM,500.0f,true))
                            pHamuul->ForcedDespawn(5000);
                        if (Creature * pMalfurion = me->FindNearestCreature(MALFURION_STORMRAGE,500.0f,true))
                            pMalfurion->ForcedDespawn(5000);
                        me->ForcedDespawn(5000);
                        break;
                }

                talks++;
            }
            else talkTimer -= diff;
        }
    };
};

void AddSC_boss_ragnaros_fl()
{
    // RAGNAROS NPCS
    new boss_ragnaros_firelands();
    new npc_Sulfurus_smash();
    new npc_Lava_wave();
    new Magma_trap_npc();
    new npc_Splitting_Sulfuras();
    new npc_Son_of_flame();
    new Engulfing_flame_npc();
    new Molten_elemental_npc();
    new Lava_scion_npc();
    new Blazing_heat_npc();
    new Living_meteor_npc();

    // TRASH NPCS
    new Molten_erupter_npc();//             53617
    new Lava_Wielder_npc();//               53575
    new Lava_bunny_npc();//                 53585

    //TRASH SPELL_SCRIPTS
    new spell_gen_raise_lava();//           99503

    // RAGNAROS SPELL_SCRIPTS
    new spell_gen_molten_inferno();//       98518,100252,100253,100254
    new spell_gen_blazing_heat(); //        99126,100984,100985,100986
    new spell_living_meteor_dummy();//      99279
    new spell_gen_blazing_heat_heal();//    99145
    new spell_gen_molten_power(); //        100158,100302
    new spell_gen_lava_wave(); //           101088,101102

    //HEROIC RAGNAROS NPCS
    new npc_cloudburst();
    new npc_breadthOfFrost();
    new npc_entrappingRoots();
    new npc_dreadFlame();
    new magma_geyser_npc();
    new npc_firelands_cenarius();

    // HEROIC RAGNAROS SPELL_SCRIPTS
    new spell_gen_world_in_flames(); // 100171 100190
    new spell_gen_frost_freeze(); // 100907
    new spell_gen_breadthOfFrostFreeze(); // 100567
    new spell_gen_breadthOfFrostBarrier(); // 100503
    new spell_gen_superheated(); // 100594,100915

    // GO SCRIPTS
    new go_firelands_portal();
}

/*  HEROIC SQLs

    DOPLNIT REPLACE INTO pre NPC :
    select * from creature_template where entry in (53874,53872,53873,54147)

    DELETE FROM `spell_script_names` WHERE  spell_id=100171 OR  spell_id=100190;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
    VALUES (100171, 'spell_gen_world_in_flames'),
    (100190, 'spell_gen_world_in_flames');

    DELETE FROM `spell_script_names` WHERE  spell_id=100907;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
    VALUES (100907, 'spell_gen_frost_freeze');

    DELETE FROM `spell_script_names` WHERE  spell_id=100567;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
    VALUES (100567, 'spell_gen_breadthOfFrostFreeze');

    DELETE FROM `spell_script_names` WHERE  spell_id=100503;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
    VALUES (100503, 'spell_gen_breadthOfFrostBarrier');

    DELETE FROM `spell_script_names` WHERE  spell_id=100594 OR  spell_id=100915;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
    VALUES (100594, 'spell_gen_superheated'),
    (100915, 'spell_gen_superheated');

    REPLACE INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `quest_start`, `cast_flags`, `user_type`) VALUES (54147, 110469, 0, 1, 1);



*/
/*
    DELETE FROM `spell_script_names` WHERE  spell_id=98518 OR  spell_id=100252 OR  spell_id=100253 OR  spell_id=100254;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
    VALUES (98518, 'spell_gen_molten_inferno'),
    (100252, 'spell_gen_molten_inferno'),
    (100253, 'spell_gen_molten_inferno'),
    (100254, 'spell_gen_molten_inferno');

    DELETE FROM `spell_script_names` WHERE  spell_id=99126 OR  spell_id=100984 OR  spell_id=100985 OR  spell_id=100986;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
    VALUES (99126, 'spell_gen_blazing_heat'),
    (100984, 'spell_gen_blazing_heat'),
    (100985, 'spell_gen_blazing_heat'),
    (100986, 'spell_gen_blazing_heat');

    DELETE FROM `spell_script_names` WHERE  spell_id=99503;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (99503, 'spell_gen_raise_lava');

    DELETE FROM `spell_script_names` WHERE  spell_id=99279;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
    VALUES (99279, 'spell_living_meteor_dummy');

    DELETE FROM `spell_script_names` WHERE  spell_id=99145;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
    VALUES (99145, 'spell_gen_blazing_heat_heal');

    DELETE FROM `spell_script_names` WHERE  spell_id=99510;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
    VALUES (99510, 'spell_gen_lava');

    DELETE FROM `spell_script_names` WHERE  spell_id=100158 OR  spell_id=100302;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
    VALUES (100158, 'spell_gen_molten_power'),
    (100302, 'spell_gen_molten_power');

    DELETE FROM `spell_script_names` WHERE  spell_id=101088 OR  spell_id=101102;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
    VALUES (101088, 'spell_gen_lava_wave'),
    (101102, 'spell_gen_lava_wave');


    DELETE FROM `creature_template` WHERE  `entry`=52409 LIMIT 1;
    DELETE FROM `creature_template` WHERE  `entry`=540100 LIMIT 1;
    DELETE FROM `creature_template` WHERE  `entry`=53420 LIMIT 1;
    DELETE FROM `creature_template` WHERE  `entry`=53363 LIMIT 1;
    DELETE FROM `creature_template` WHERE  `entry`=53086 LIMIT 1;
    DELETE FROM `creature_template` WHERE  `entry`=53419 LIMIT 1;
    DELETE FROM `creature_template` WHERE  `entry`=53140 LIMIT 1;
    DELETE FROM `creature_template` WHERE  `entry`=53189 LIMIT 1;
    DELETE FROM `creature_template` WHERE  `entry`=53485 LIMIT 1;
    DELETE FROM `creature_template` WHERE  `entry`=53231 LIMIT 1;
    DELETE FROM `creature_template` WHERE  `entry`=53473 LIMIT 1;
    DELETE FROM `creature_template` WHERE  `entry`=53500 LIMIT 1;

    INSERT INTO `creature_template` (`entry`, `modelid1`, `name`, `subname`, `IconName`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `speed_run`, `rank`, `mindmg`, `maxdmg`, `baseattacktime`, `rangeattacktime`, `unit_class`, `type`, `type_flags`, `Health_mod`, `movementId`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (52409, 37875, 'Ragnaros', '', '', 88, 88, 3, 2234, 2234, 1.42857, 1, 800, 1000, 1500, 2000, 1, 4, 268435564, 585, 144, 1, 'boss_ragnaros_firelands', 15595);
    INSERT INTO `creature_template` (`entry`, `modelid1`, `name`, `subname`, `IconName`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `type`, `type_flags`, `InhabitType`, `flags_extra`, `WDBVerified`) VALUES (540100, 11686, 'Lava_ring', '', '', 85, 85, 3, 35, 35, 2000, 2000, 1, 33554432, 10, 1048576, 4, 128, 15595);
    INSERT INTO `creature_template` (`entry`, `modelid2`, `name`, `subname`, `IconName`, `faction_A`, `faction_H`, `unit_class`, `type`, `type_flags`, `movementId`, `ScriptName`, `WDBVerified`) VALUES (53420, 11686, 'Sulfuras, Hand of Ragnaros', '', '', 35, 35, 1, 10, 16778276, 76, 'npc_Sulfurus_smash', 15595);
    INSERT INTO `creature_template` (`entry`, `modelid2`, `name`, `subname`, `IconName`, `faction_A`, `faction_H`, `unit_class`, `type`, `type_flags`, `movementId`, `ScriptName`, `WDBVerified`) VALUES (53363, 11686, 'Lava Wave', '', '', 14, 14, 1, 10, 16778276, 197, 'npc_Lava_wave', 15595);
    INSERT INTO `creature_template` (`entry`, `modelid1`, `name`, `subname`, `IconName`, `faction_A`, `faction_H`, `unit_class`, `type`, `type_flags`, `movementId`, `ScriptName`, `WDBVerified`) VALUES (53086, 11686, 'Magma Trap', '', '', 14, 14, 1, 10, 16778276, 76, 'Magma_trap_npc', 15595);
    INSERT INTO `creature_template` (`entry`, `modelid1`, `name`, `subname`, `IconName`, `faction_A`, `faction_H`, `unit_class`, `type`, `type_flags`, `movementId`, `ScriptName`, `WDBVerified`) VALUES (53419, 11686, 'Sulfuras, Hand of Ragnaros', '', '', 14, 14, 1, 10, 16778276, 76, 'npc_Splitting_Sulfuras', 15595);
    INSERT INTO `creature_template` (`entry`, `modelid1`, `name`, `subname`, `IconName`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `speed_run`, `rank`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `type`, `Health_mod`, `movementId`, `ScriptName`, `WDBVerified`) VALUES (53140, 1070, 'Son of Flame', '', '', 87, 87, 3, 14, 14, 0.142857, 1, 2000, 2000, 1, 33554432, 4, 1.5, 63, 'npc_Son_of_flame', 15595);
    INSERT INTO `creature_template` (`entry`, `modelid1`, `name`, `subname`, `IconName`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `scale`, `rank`, `unit_class`, `type`, `type_flags`, `Health_mod`, `movementId`, `ScriptName`, `WDBVerified`) VALUES (53189, 38520, 'Molten Elemental', '', '', 87, 87, 3, 14, 14, 2, 1, 1, 4, 72, 1.8, 144, 'Molten_elemental_npc', 15595);
    INSERT INTO `creature_template` (`entry`, `modelid1`, `modelid2`, `name`, `subname`, `IconName`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `speed_walk`, `speed_run`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `type`, `type_flags`, `movementId`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53485, 19963, 11686, 'Engulfing Flames', '', '', 88, 88, 3, 14, 14, 1.2, 0.428571, 2000, 2000, 1, 33554432, 10, 16778276, 76, 128, 'Engulfing_flame_npc', 15595);
    INSERT INTO `creature_template` (`entry`, `modelid1`, `name`, `subname`, `IconName`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `speed_walk`, `speed_run`, `rank`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `type`, `type_flags`, `Health_mod`, `movementId`, `ScriptName`, `WDBVerified`) VALUES (53231, 38019, 'Lava Scion', '', '', 87, 87, 3, 14, 14, 4, 1.42857, 1, 1500, 2000, 1, 32768, 4, 104, 18.75, 156, 'Lava_scion_npc', 15595);
    INSERT INTO `creature_template` (`entry`, `modelid1`, `name`, `subname`, `IconName`, `minlevel`, `maxlevel`, `faction_A`, `faction_H`, `unit_class`, `type`, `type_flags`, `movementId`, `ScriptName`, `WDBVerified`) VALUES (53473, 11686, 'Blazing Heat', '', '', 87, 87, 14, 14, 1, 10, 16778276, 76, 'Blazing_heat_npc', 15595);
    INSERT INTO `creature_template` (`entry`, `modelid1`, `name`, `subname`, `IconName`, `minlevel`, `maxlevel`, `faction_A`, `faction_H`, `speed_run`, `rank`, `unit_class`, `type`, `type_flags`, `Health_mod`, `movementId`, `ScriptName`, `WDBVerified`) VALUES (53500, 38475, 'Living Meteor', '', '', 87, 87, 14, 14, 0.88888, 1, 1, 10, 16778336, 1500, 106, 'Living_meteor_npc', 15595);

    UPDATE `creature_template` SET `minlevel`=87 WHERE  `entry`=53086 LIMIT 1;
    UPDATE `creature_template` SET `maxlevel`=87 WHERE  `entry`=53086 LIMIT 1;
    UPDATE `creature_template` SET `minlevel`=87, `maxlevel`=87 WHERE  `entry`=53363 LIMIT 1;
    UPDATE `creature_template` SET `minlevel`=87, `maxlevel`=87 WHERE  `entry`=53419 LIMIT 1;
    UPDATE `creature_template` SET `minlevel`=87, `maxlevel`=87 WHERE  `entry`=53420 LIMIT 1;
    */
