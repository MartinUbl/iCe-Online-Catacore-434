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
#include "Object.h"
//#include "MoveSplineInit.h"
#include "SpellScript.h"

#define DATA_TORNADO_SLOT           1
#define DATA_STAGE_THREE_TIMER      2
#define DATA_ACHIEVEMENTBF          4
#define DATA_ACHIEVEMENTIC          5
#define DATA_ACHIEVEMENTLS          6
#define DATA_ACHIEVEMENTFT          7
#define DATA_BOULDER                8

const uint32 DATA_IMPRINTED = 999;

# define NEVER  (0xffffffff) // used as "delayed" timer

enum Sounds
{
    SAY_AGGRO                   = 24426, //I serve a new master now, mortals!
    SAY_KILL_01                 = 24430, //BURN!
    SAY_KILL_02                 = 24431, //For his Glory!
    SAY_DEATH                   = 24429, //The light... mustn't... burn out...
    SAY_BURNOUT                 = 24436, //Fire... fire...
    SAY_REBORN                  = 24437, //Reborn in flame!
    SAY_SPIRAL_01               = 24434, //These skies are MINE!
    SAY_SPIRAL_02               = 24435, //I will burn you from the sky!
    SAY_FENDRAL_01              = 24466, //What have we here - visitors to our kingdom in the Firelands?
    SAY_FENDRAL_02              = 24467, //You mortals may remember Alysra, who spirited me to freedom in Mount Hyjal. She, too has been reborn. Born of flame!
    SAY_FENDRAL_03              = 24468, //I wish I could watch her reduce your pitiful band to cinders, but I am needed elsewhere. Farewell!
    SAY_DRUID_01F               = 24797, //We call upon you, Firelord!
    SAY_DRUID_02F               = 24798, //Behold His power!
    SAY_DRUID_03F               = 24799, //Let the unbelievers perish in fire!
    SAY_DRUID_01M               = 24808, //We call upon you, Firelord!
    SAY_DRUID_02M               = 24809, //Behold His power!
    SAY_DRUID_03M               = 24811, //Witness the majesty of flame!
    SAY_CLAWSHAPER              = 24800, //Together we call upon the lord of fire!
    SAY_METEOR                  = 24813, //None escape the rage of the Firelands!
    //SAY_UKNOWN                = 24440, //Unknown
};

Position const SpawnPositions[18] =
{
    {105.9f, -407.6f, 25.8f, 0.0f},     //Alysrazor
    {-10.7f, -269.7f, 55.3f, 0.0f},     //Worm 1-4
    {-37.0f, -309.99f, 55.3f, 0.0f},
    {-71.3f, -275.6f, 55.3f, 0.0f},
    {-41.6f, -246.8f, 55.3f, 0.0f},
    {-31.3f, -244.3f, 55.3f, 0.0f},     // Worm 4-8
    {-15.3f, -294.4f, 55.3f, 0.0f},
    {-69.3f, -264.3f, 55.3f, 0.0f},
    {-62.4f, -302.7f, 55.3f, 0.0f},
    {-22.9f, -324.42f, 55.3f, 0.0f},        // Initiate Spawns E
    {-57.99f, -236.71f, 55.3f, 0.0f},   // W
    {5.42f, -268.2f, 55.3f, 0.0f},      // W
    {-84.9f, -291.9f, 55.3f, 0.0f},     // E
    {-53.6f, -330.0f, 55.3f, 0.0f},     // E
    {-22.07f, -231.6f, 55.3f, 0.0f},    // W
    {-3.03f, -311.6f, 53.0f, 0.0f},     // Spawn Feather
    {-47.0f, -266.0f, 90.0f, 0.0f}, //Molten Eggs
    {-33.0f, -287.0f, 90.0f, 0.0f},
};

enum Spells
{
    // NPC SPELLS
    SPELL_SMOULDERING_ROOTS     = 100559,
    SPELL_SMOULDERING_ROOTS_BUFF= 100555,
    SPELL_REMOVE_ROOTS          = 100561,
    SPELL_FIGHT_START           = 100570, // Cosmetic earthquake
    SPELL_COSMETIC_DEATH        = 100564,
    SPELL_SACRIFICE             = 100557, // Cosmetic Instant Kill SACRIFICE OF THE FLAME
    SPELL_FENDRAL_TRANSFORM     = 100565,
    SPELL_FIRE_HAWK_SMOKE       = 100712, // Cosmetic
    SPELL_MOLTEN_FEATHER_COS    = 99507,  // Cosmetic
    SPELL_BLAZING_TALON_TRAN    = 99550,  // Transform
    SPELL_EXPLOSION_EGG         = 99943,
    SPELL_ERRUPTION             = 98402,
    SPELL_MOLTEN_EGG_VISUAL     = 98638,  // Visual Egg on back
    SPELL_BLAZING_PREVENTION    = 99565,  // BlazingAuraPrevention
    SPELL_BLAZING_POWER_EFFECT  = 99461,
    SPELL_FIERY_TORNADO         = 99817,

    // MoltenFeather Moving Cast
    MOLTEN_FEATHER_MAGE         = 98761,
    MOLTEN_FEATHER_WARRIOR      = 98762,
    MOLTEN_FEATHER_WARLOCK      = 98764,
    MOLTEN_FEATHER_PRIEST       = 98765,
    MOLTEN_FEATHER_DRUID        = 98766,
    MOLTEN_FEATHER_ROGUE        = 98767,
    MOLTEN_FEATHER_HUNTER       = 98768,
    MOLTEN_FEATHER_PALADIN      = 98769,
    MOLTEN_FEATHER_SHAMAN       = 98770,
    MOLTEN_FEATHER_DK           = 98771,

    // Alysrazor
    SPELL_FIRESTORM             = 99605,
    SPELL_FIRESTORM_HC          = 100744,
    SPELL_VOLCANIC_FIRE         = 98462, // Block
    SPELL_BLAZING_CLAW          = 99843,
    SPELL_MOLTING               = 99464,
    SPELL_MOLTEN_FEATHER        = 97128,
//    SPELL_FULL_POWER            = 99504,
    SPELL_FULL_POWER            = 99925,
    SPELL_WINGS_OF_FLAME        = 98624,
    SPELL_WINGS_OF_FLAME_FLY    = 98619, // buff
    SPELL_FEATHER_BAR           = 101410,
    SPELL_INCENDIARY_CLOUD      = 99426,
    SPELL_BLAZING_POWER         = 99462,
    SPELL_ALYSRAS_RAZOR         = 100029,
    SPELL_FIERY_VORTEX          = 99793,
    SPELL_HARSH_WINDS           = 100640,
    SPELL_BURNOUT               = 99432,
    SPELL_SPARK                 = 99921,
    SPELL_IGNITED               = 99922,

    // Blazing Talon Initiate
    SPELL_FIRE_IT_UP            = 100093,
    SPELL_FIEROBLAST            = 101223,
    SPELL_BRUSHFIRE             = 98868,
    SPELL_BRUSHFIRE_PERIODIC    = 98884,
    SPELL_BLAZING_SHIELD_HC     = 101484,

    // Voracious Hatchling
    SPELL_HUNGRY                = 99361,
    SPELL_IMPRINTED             = 99388,
    SPELL_IMPRINTED2            = 100358,
    SPELL_IMPRINTED_TAUNT       = 99390,
    SPELL_IMPRINTED_TAUNT2      = 100360,
    SPELL_SATIATED              = 99359,
    SPELL_TANTRUM               = 99362,
    SPELL_GUSHING_WOUND_Y10     = 99308,

    // Plump Lava Worm
    SPELL_LAVA_WORM_COSMETIC    = 99327,
    SPELL_LAVA_SPEW             = 99335,

    // Herald of the Burning End (HC)
    SPELL_SUMMON_GO_METEOR      = 100055,
    SPELL_MOLTEN_METOER_DEAD    = 100059, // visual
    SPELL_MOLTEN_METEOR         = 99215,
    SPELL_MOLTEN_BOULDER        = 99265,
    SPELL_CATACLYSM             = 102111,
    SPELL_METEOR_CRACK          = 99266,
    SPELL_METEORIC_IMPACT       = 99558,

    // Blazing Talon Clawshaper
    SPELL_IGNITION              = 99919,
};

enum Events
{
    // Alysrazor
    STAGE_ONE                   = 0, // I Will Burn You From the Sky!
    STAGE_TWO                   = 1, // The Skies are Mine!
    STAGE_THREE                 = 2, // Burnout!
    STAGE_FOUR                  = 3, // Reborn in Flame!
};

enum MiscData
{
    NPC_ALYSRAZOR               = 52530,
    NPC_PLUMP_LAVA_WORM         = 53520,
    NPC_VORACIOUS_HATCHLING     = 53898,
    NPC_BLAZING_TALON_INITIATE  = 53896,  // Bird
    NPC_MOLTEN_EGG              = 53681,
    NPC_CAPTIVE_DRUID           = 54019,
    NPC_MAJORDOMO_STAGHELM      = 54015,
    GAMEOBJECT_VOLCANO          = 209253,
    NPC_MOLTEN_METEOR           = 54563,
    NPC_MOLTEN_BOULDER          = 53498,
    NPC_EGG_SATCHEL             = 52528,
    NPC_BLAZING_TALON_CLAW      = 53734,  // Phase three
    NPC_MOLTEN_FEATHER          = 53089,
    NPC_HERALD_OF_BURNING       = 53375,
    NPC_BROODMOTHER_1           = 53680,
    NPC_BROODMOTHER_2           = 53900,

    // Spell NPC
    NPC_BLAZING_POWER           = 53554,
    NPC_INCENDIARY_CLOUD        = 53541,
    NPC_BRUSHFIRE               = 53372,
    NPC_LAVA_WORM_TARGET        = 53521,
    NPC_VOLCANO_FIRE_BUNNY      = 53158,
    NPC_FIERY_VORTEX            = 53693,
    NPC_FIERY_TORNADO           = 53698,
    NPC_FIRESTORM               = 53986,

    // Anim Kits
    ANIM_KIT_EMERGE             = 1490,
    ANIM_ATTACK_FEED            = 1488,
    ANIM_HATH                   = 1444,
    ANIM_KIT_FLY_OUT            = 1486,
    ANIM_KIT_WAKE_UP            = 1456,
};

/*********************ALYSRAZOR_BOSS_AI***********************/
class boss_Alysrazor : public CreatureScript
{
    public:
        boss_Alysrazor() : CreatureScript("boss_Alysrazor") { }

        struct boss_AlysrazorAI : public ScriptedAI
        {
            boss_AlysrazorAI(Creature* creature) : ScriptedAI(creature),Summons(creature)
            {
                me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                me->SetReactState(REACT_PASSIVE);
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_PERIODIC_MANA_LEECH, true);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_POWER_DRAIN, true);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_POWER_BURN, true);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, false);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, false);
                me->SetFlying(true);
                sx = -41.78f;
                sy = -275.97f;
                Phase = 4;
                me->SetVisible(false);
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;

            float sx;
            float sy;
            float i;
            float d;

            bool FlyOut;
            bool FlyDownFirst;
            bool SpawnWorms;
            bool FirstWormPositions;
            bool FlyUP;
            bool SummonInitiate;
            bool FieryVortex;
            bool MoveMiddle;
            bool FieryTornado;
            bool Check;
            bool Dying;
            bool switcher;

            uint8 wormsCount;

            uint32 AchievementBF;
            uint32 AchievementIC;
            uint32 AchievementLS;
            uint32 AchievementFT;
            uint32 HeraldTimer;
            uint32 Phase;
            uint32 Cycle;
            uint32 Rounds;
            uint32 Enrage;
            uint32 InitiatesFirst;
            uint32 HarshTimer;
            uint32 SummonInitiateTimer;
            uint32 SpawnWormsTimer;
            uint32 FlyFront;
            uint32 FlyTimer;
            uint32 EggSatchelTimer;
            uint32 IncendiaryCloudTimer;
            uint32 IncendiaryCloudFront;
            uint32 BlazingPowerTimer;
            uint32 StageTimer;
            uint32 FallingTimer;
            uint32 firstPhaseTimer;
            uint32 firestormTimer;
            uint32 feathersTimer;

            uint32 heraldCounter;

            uint32 C;
            uint32 u;
            uint32 y;

            uint64 TargetGUID;

            SummonList Summons;

            void JustSummoned(Creature* summon)
            {
                Summons.push_back(summon->GetGUID());
            }

            void SummonedCreatureDespawn(Creature* pSummon)
            {
                if (pSummon && pSummon->GetEntry() == NPC_VORACIOUS_HATCHLING)
                {
                    if (instance)
                        instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, pSummon);
                }
            }

            void DoUpdateGoForPlayers (GameObject * pGO)
            {
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
                        if (pGO)
                            pGO->SendUpdateToPlayer(pPlayer);
                    }
                }
            }

            void DoStopCombatForPlayers()
            {
                Map * map = me->GetMap();

                if (!map)
                    return;

                Map::PlayerList const& plrList = map->GetPlayers();
                if (plrList.isEmpty())
                    return;
                for(Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
                {
                    if(Player* pPlayer = itr->getSource())
                        pPlayer->CombatStop(true);
                }
            }

            void Reset()
            {
                if(instance)
                {
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                }

                IsHeroic() ? d=1.0f : d=-1.0f;
                AchievementBF = 0;
                AchievementIC = 0;
                AchievementLS = 0;
                AchievementFT = 0;

                me->SetMaxPower(POWER_MANA, 100);
                me->SetPower(POWER_MANA, 100);

                if (!me->GetAura(96301)) // Stop mana regen
                    me->CastSpell(me, 96301, false);

                me->SetPower(POWER_MANA, 100);
                instance->SetData(TYPE_ALYSRAZOR, NOT_STARTED);
                C = 1;
                u = 0;
                y = 0;
                wormsCount = 0;
                Rounds = 0;
                Cycle = 1;
                Enrage = 0;
                InitiatesFirst = 0;
                IncendiaryCloudFront = 4;
                heraldCounter = 0;
                Check = false;
                FlyUP = false;
                FirstWormPositions = true;
                SpawnWorms = false;
                FlyDownFirst = true;
                FlyOut = false;
                Dying = false;
                switcher = false;

                if (!me->FindNearestCreature(NPC_MAJORDOMO_STAGHELM, 300.0f))
                {
                    if (GameObject* Volcano = me->FindNearestGameObject(GAMEOBJECT_VOLCANO, 300.0f))
                    {
                        Volcano->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);
                        DoUpdateGoForPlayers(Volcano);
                    }
                    me->SummonCreature(NPC_MOLTEN_FEATHER, SpawnPositions[15].GetPositionX(), SpawnPositions[15].GetPositionY(), SpawnPositions[15].GetPositionZ(), 1.949f, TEMPSUMMON_MANUAL_DESPAWN);
                }
            }

            void EnterCombat(Unit* /*target*/)
            {
                Phase = 4;
                if(instance)
                    instance->SetData(TYPE_ALYSRAZOR, IN_PROGRESS);

                if (IsHeroic())
                    HeraldTimer = 32000;

                EggSatchelTimer = 26000;
                firestormTimer = 97000;
                feathersTimer = NEVER;
                SummonInitiate = true;
                SummonInitiateTimer = 20000;
                firstPhaseTimer = IsHeroic() ? 250000 : 175000;
                me->MonsterYell("I serve a new master now, mortals!", LANG_UNIVERSAL, 0);
                DoPlaySoundToSet(me, SAY_AGGRO);
                RemoveAuraFromAllPlayers(SPELL_MOLTEN_FEATHER, true, false);
                RemoveAuraFromAllPlayers(SPELL_FEATHER_BAR, false, false);
            }

            void DamageTaken(Unit* attacker, uint32& damage)
            {
                if(attacker == me) // Can kill self
                    return;

                if (damage >= me->GetHealth())
                {
                    damage = 0;
                    me->InterruptNonMeleeSpells(false);
                    me->RemoveAllAuras();
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
                    me->SetUInt64Value(UNIT_FIELD_TARGET, 0);
                    me->GetMotionMaster()->MoveFall(0);
                    me->HandleEmoteCommand(EMOTE_ONESHOT_FLYDEATH);
                    Dying = true;
                    FallingTimer = 2500;
                }
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                {
                    switch (urand(1, 2))
                    {
                        case 1: me->MonsterYell("BURN!", LANG_UNIVERSAL, 0);
                            DoPlaySoundToSet(me, SAY_KILL_01);
                            break;
                        case 2: me->MonsterYell("For his Glory!", LANG_UNIVERSAL, 0);
                            DoPlaySoundToSet(me, SAY_KILL_02);
                            break;
                    }
                }
            }

            void EnterEvadeMode()
            {
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, false);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, false);

                if(instance)
                {
                    instance->SetData(TYPE_ALYSRAZOR,NOT_STARTED);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                }

                RemoveAuraFromAllPlayers(0, false, true);
                RemoveAuraFromAllPlayers(SPELL_FEATHER_BAR, true, false);
                RemoveAuraFromAllPlayers(SPELL_MOLTEN_FEATHER, true, false);
                me->CombatStop(true);
                me->SetVisible(false);
                me->SetFullHealth();
                me->RemoveAllAuras();
                me->SetSpeed(MOVE_FLIGHT, 20.0f);
                me->GetMotionMaster()->MovePoint(3, me->GetHomePosition());
                Phase = 4;
                //Despawn meteors if any
                DespawnMeteors();
                ScriptedAI::EnterEvadeMode();
            }

            void DespawnMeteors(void)
            {
                DespawnCreatures(53784); // Viusal metoers NPCs
                std::list<GameObject*> objects; // LoS Meteors
                me->GetGameObjectListWithEntryInGrid(objects, 208966, 500.0f);
                if (!objects.empty())
                    for (std::list<GameObject*>::iterator iter = objects.begin(); iter != objects.end(); ++iter)
                        (*iter)->Delete();
            }

            void JustDied(Unit* /*Killer*/)
            {
                me->MonsterYell("The light... mustn't... burn out...", LANG_UNIVERSAL, 0);
                DoPlaySoundToSet(me, SAY_DEATH);
                RemoveAuraFromAllPlayers(SPELL_MOLTEN_FEATHER, true, false);
                RemoveAuraFromAllPlayers(SPELL_FEATHER_BAR, true, true);
                instance->SetData(TYPE_ALYSRAZOR, DONE);
                DoStopCombatForPlayers();
            }

            void SetData(uint32 Type, uint32 Data)
            {
                switch (Type)
                {
                    case DATA_STAGE_THREE_TIMER:
                    {
                        if (StageTimer > 1001)
                            StageTimer -= Data;
                        break;
                    }
                }
            }

            void DoAction(const int32 param)
            {
                switcher = !switcher;
            }

            uint32 GetData(uint32 type)
            {
                if( type == DATA_IMPRINTED)
                {
                    if(switcher)
                        return 1;
                    else
                        return 2;
                }

                switch (type)
                {
                    case DATA_STAGE_THREE_TIMER:
                        if (Phase == STAGE_FOUR)
                            return 5;
                        break;
                    case DATA_ACHIEVEMENTBF:
                        return AchievementBF;
                        break;
                    case DATA_ACHIEVEMENTIC:
                        return AchievementIC;
                        break;
                    case DATA_ACHIEVEMENTLS:
                        return AchievementLS;
                        break;
                    case DATA_ACHIEVEMENTFT:
                        return AchievementFT;
                        break;
                }
                return 0;
            }

            void SpellHit(Unit* caster, const SpellEntry* spell)
            {
                if(caster == me)
                    return;

                if(caster->ToCreature())
                    return;

                if (me->HasAura(SPELL_BURNOUT) && caster->getPowerType() == POWER_MANA)
                    me->CastSpell(caster,99433,true); // Energize mana ( Essence of the Green )
            }

            void FlyMovement(uint32 ID)
            {
                switch (ID)
                {
                    case 1: //FlyOut Intro
                        me->CastSpell(me, SPELL_FIGHT_START, false);
                        me->SetFloatValue(UNIT_FIELD_COMBATREACH, 35);
                        RemoveAuraFromAllPlayers(SPELL_SMOULDERING_ROOTS_BUFF, true, false);
                        FlyTimer = 2500;
                        FlyOut = true;
                        me->SetInCombatWithZone();
                        me->SetSpeed(MOVE_FLIGHT, 40.0f);
                        me->PlayOneShotAnimKit(ANIM_KIT_FLY_OUT);
                        me->SetFacingTo(2.40f);
                        //me->GetMotionMaster()->MovePoint(1, 105.9f, -407.6f, 150.0f);
                        me->GetMotionMaster()->MovePoint(1,45.0f,-348.0f,120.0f);
                        ++FlyFront;
                        break;
                    case 2: //FlyDown(Intro)
                    {
                        FlyTimer = 10000;
                        me->SetFacingTo(2.40f);
                        me->SetSpeed(MOVE_FLIGHT, 1.2f);
                        i = -4.5f*(M_PI/25);
                        me->GetMotionMaster()->MovePoint(3, sx + 70*cos(i), sy + 70*sin(i), 90.0f);
                        me->CastSpell(me, SPELL_FIRESTORM, true);
                        ++FlyFront;
                        break;
                     }
                    case 3://FlyDown2
                        FlyUP = false;
                        FlyTimer = !Check ? 1000 : 2500;
                        me->SetSpeed(MOVE_FLIGHT, 4.0f);
                        //me->SetFacingTo(me->GetAngle(sx, sy));
                        i = -4.5f*(M_PI/25);
                        me->GetMotionMaster()->MovePoint(3, sx + 55*cos(i), sy + 55*sin(i), 70.0);
                        ++FlyFront;
                        break;
                    case 4: //FlyCenter
                        FlyTimer = 9000;
                        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
                        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
                        me->SetFloatValue(UNIT_FIELD_COMBATREACH, 10);
                        me->SetSpeed(MOVE_FLIGHT, 1.0f);
                        i = 19.5f*(M_PI/25);
                        me->GetMotionMaster()->MovePoint(3, sx + 55*cos(i), sy + 55*sin(i), 70.0f);
                        me->CastSpell(me, 99844, true);
                        me->CastSpell(me, SPELL_MOLTING, false);
                        me->CastSpell(me, SPELL_BLAZING_CLAW, false);
                        ++FlyFront;
                        break;
                    case 5: //FlyUP1
                        me->SetFloatValue(UNIT_FIELD_COMBATREACH, 55);
                        FlyTimer = 2500;
                        me->SetSpeed(MOVE_FLIGHT, 4.0f);
                        me->SetFacingTo(me->GetAngle(sx, sy));
                        me->GetMotionMaster()->MovePoint(3, sx + 55*cos(i), sy + 55*sin(i), 130.0f);
                        me->RemoveAura(SPELL_BLAZING_CLAW);
                        FlyUP = true;
                        ++FlyFront;
                        break;
                    case 6: //FlyDown
                        FlyUP = false;
                        FlyTimer = 2500;
                        me->SetFacingTo(me->GetAngle(sx, sy));
                        me->SetSpeed(MOVE_FLIGHT, 4.0f);
                        i = 19.5f*(M_PI/25);
                        me->GetMotionMaster()->MovePoint(3, sx + 55*cos(i), sy + 55*sin(i), 70.0f);
                        ++FlyFront;
                        break;
                    case 7: //FlyCenter 2/2
                        FlyTimer = 9000;
                        me->SetSpeed(MOVE_FLIGHT, 1.4f);
                        me->SetFloatValue(UNIT_FIELD_COMBATREACH, 10);
                        i = -4.5f*(M_PI/25);
                        me->GetMotionMaster()->MovePoint(3, sx + 55*cos(i), sy + 55*sin(i), 70.0);
                        me->CastSpell(me, 99844, true);
                        me->CastSpell(me, SPELL_MOLTING, false);
                        me->CastSpell(me, SPELL_BLAZING_CLAW, false);
                        ++FlyFront;
                        break;
                    case 8: //FlyUP2
                        FlyUP = true;
                        FlyTimer = 2500;
                        me->SetFloatValue(UNIT_FIELD_COMBATREACH, 55);
                        me->SetFacingTo(me->GetAngle(sx, sy));
                        me->SetSpeed(MOVE_FLIGHT, 4.0f);
                         i = -4.5f*(M_PI/25);
                        me->GetMotionMaster()->MovePoint(3, sx + 55*cos(i), sy + 55*sin(i), 130.0);
                        me->RemoveAura(SPELL_BLAZING_CLAW);
                        ++FlyFront;
                        break;
                }
                me->SetUInt64Value(UNIT_FIELD_TARGET, 0);
            }

            void RemoveAuraFromAllPlayers(uint32 spellentry, bool remove, bool despawncreatures)
            {
                if (spellentry != 0)
                {
                    Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                    for (Map::PlayerList::const_iterator l = PlList.begin(); l != PlList.end(); ++l)
                        if (Player* player = l->getSource())
                        {
                            if (player->GetAura(97128) && player->GetPower(POWER_SCRIPTED))
                                if (player->GetAura(97128)->GetStackAmount() == 2 && player->GetPower(POWER_SCRIPTED) != 2)
                                    player->SetPower(POWER_SCRIPTED, 2);

                            if (remove)
                            {
                                if (spellentry == SPELL_MOLTEN_FEATHER)
                                    if (player->GetPower(POWER_SCRIPTED))
                                        player->SetPower(POWER_SCRIPTED, 0);
                                if (Aura* aura = player->GetAura(spellentry))
                                    player->RemoveAura(aura);
                            }
                            else me->AddAura(spellentry, player);
                        }
                }
                if (despawncreatures)
                {
                    Summons.DespawnAll();
                }
            }

            void SummonNPC(uint32 entry)
            {
                switch (entry)
                {
                case NPC_EGG_SATCHEL:
                {
                    Creature* pBroodMother = me->SummonCreature(NPC_BROODMOTHER_1,25.4f,-188.5f,113.0f,2.61f,TEMPSUMMON_MANUAL_DESPAWN,0);
                    if (pBroodMother)
                    {
                        if (Creature* pSatchel1 = me->SummonCreature(NPC_MOLTEN_EGG,pBroodMother->GetPositionX(),pBroodMother->GetPositionY(),pBroodMother->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 30000))
                        {
                            pBroodMother->ForcedDespawn(12000);
                            pSatchel1->EnterVehicle(pBroodMother, 0, false);
                            pBroodMother->GetMotionMaster()->MovePoint(0,-47.0f, -266.0f, 80.0f);
                        }
                    }

                    pBroodMother = NULL;

                    pBroodMother = me->SummonCreature(NPC_BROODMOTHER_2,-20.3f,-410.4f,113.0f,2.65f,TEMPSUMMON_MANUAL_DESPAWN,0);

                    if (pBroodMother)
                    {
                        if (Creature* pSatchel2 = me->SummonCreature(NPC_MOLTEN_EGG,pBroodMother->GetPositionX(),pBroodMother->GetPositionY(),pBroodMother->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 30000))
                        {
                            pBroodMother->ForcedDespawn(12000);
                            pSatchel2->EnterVehicle(pBroodMother, 0, false);
                            pBroodMother->GetMotionMaster()->MovePoint(0,-33.0f,-287.0f, 80.0f);
                        }
                    }
                }
                    break;
                case NPC_PLUMP_LAVA_WORM:
                {
                    uint8 max = IsHeroic() ? 3 : 2;
                    if (wormsCount >= max)
                        break;

                    wormsCount++;

                    if (FirstWormPositions)
                    {
                        for(uint8 i = 1; i < 5 ; i++)
                            me->SummonCreature(NPC_PLUMP_LAVA_WORM, SpawnPositions[i].GetPositionX(), SpawnPositions[i].GetPositionY(), 55.3f, 0, TEMPSUMMON_MANUAL_DESPAWN);
                        FirstWormPositions = false;
                    }
                    else
                    {
                        for(uint8 i = 5; i < 9 ; i++)
                            me->SummonCreature(NPC_PLUMP_LAVA_WORM, SpawnPositions[i].GetPositionX(), SpawnPositions[i].GetPositionY(), 55.3f, 0, TEMPSUMMON_MANUAL_DESPAWN);
                        FirstWormPositions = true;
                     }
                 }
                 break;
                case NPC_BLAZING_TALON_INITIATE:
                    if (InitiatesFirst == 0)
                    {
                        float CosA;
                        float SinA;
                        ++InitiatesFirst;
                        for (uint32 i=9; i<11 ; i++)
                        {
                            CosA = (SpawnPositions[i].GetPositionX() - sx)/35;
                            SinA = (SpawnPositions[i].GetPositionY() - sy)/35;
                            if (Creature* Initiate = me->SummonCreature(NPC_BLAZING_TALON_INITIATE, sx + 80*CosA, sy + 80*SinA, 140.0f, 0, TEMPSUMMON_MANUAL_DESPAWN))
                                Initiate->GetMotionMaster()->MovePoint(3, SpawnPositions[i].GetPositionX(), SpawnPositions[i].GetPositionY(), 60.0f);
                        }
                        if (IsHeroic())
                            SummonInitiateTimer = 22000;
                        return;
                    }
                    else if (InitiatesFirst == 1)
                            {
                                ++InitiatesFirst;
                                float CosA;
                                float SinA;
                                switch (urand(1,3))
                                    {
                                    case 1:
                                        y = 9;
                                        break;
                                    case 2:
                                        y = 12;
                                        break;
                                    case 3:
                                        y = 13;
                                        break;
                                    }
                                for (uint32 i=11; i<13 ; i++)
                                {
                                    CosA = (SpawnPositions[i].GetPositionX() - sx)/35;
                                    SinA = (SpawnPositions[i].GetPositionY() - sy)/35;
                                    if (Creature* Initiate = me->SummonCreature(NPC_BLAZING_TALON_INITIATE, sx + 80*CosA, sy + 80*SinA, 140.0f, 0, TEMPSUMMON_MANUAL_DESPAWN))
                                        Initiate->GetMotionMaster()->MovePoint(3, SpawnPositions[i].GetPositionX(), SpawnPositions[i].GetPositionY(), 60.0f);
                                }
                                if (IsHeroic())
                                    SummonInitiateTimer = 63000;
                                return;
                            }
                            else if (InitiatesFirst >= 2)
                                    {
                                        if (InitiatesFirst > 5)
                                            return;

                                        ++InitiatesFirst;
                                        SummonInitiateTimer = 22000;
                                        if (InitiatesFirst == 5 && IsHeroic())
                                            SummonInitiateTimer = 40000;
                                        float CosA;
                                        float SinA;

                                        if (u != 0)
                                        {
                                            CosA = (SpawnPositions[u].GetPositionX() - sx)/35;
                                            SinA = (SpawnPositions[u].GetPositionY() - sy)/35;
                                                if (Creature* Initiate = me->SummonCreature(NPC_BLAZING_TALON_INITIATE, sx + 80*CosA, sy + 80*SinA, 140.0f, 0, TEMPSUMMON_MANUAL_DESPAWN))
                                                {
                                                    switch (urand(1,3))
                                                        {
                                                        case 1:
                                                            y = 9;
                                                            break;
                                                        case 2:
                                                            y = 12;
                                                            break;
                                                        case 3:
                                                            y = 13;
                                                            break;
                                                        }
                                                    Initiate->GetMotionMaster()->MovePoint(3, SpawnPositions[u].GetPositionX(), SpawnPositions[u].GetPositionY(), 60.0f);
                                                    u = 0;
                                                    return;
                                                }
                                        }

                                        if (y != 0)
                                        {
                                            CosA = (SpawnPositions[y].GetPositionX() - sx)/35;
                                            SinA = (SpawnPositions[y].GetPositionY() - sy)/35;
                                            if (Creature* Initiate = me->SummonCreature(NPC_BLAZING_TALON_INITIATE, sx + 80*CosA, sy + 80*SinA, 140.0f, 0, TEMPSUMMON_MANUAL_DESPAWN))
                                            {
                                                switch (urand(1,3))
                                                    {
                                                    case 1:
                                                        u = 10;
                                                        break;
                                                    case 2:
                                                        u = 11;
                                                        break;
                                                    case 3:
                                                        u = 14;
                                                        break;
                                                    }
                                                Initiate->GetMotionMaster()->MovePoint(3, SpawnPositions[y].GetPositionX(), SpawnPositions[y].GetPositionY(), 60.0f);
                                            }
                                        }
                                    }
                        break;
                        case NPC_INCENDIARY_CLOUD:
                        {
                            float Z = 0;
                            float O = 0;

                            if (!IsHeroic())
                            {
                                // on normal summon BP + IC on 4 positions
                                switch(urand(0,3))
                                    {
                                case 0: // Summon on right side
                                    Z = 0;
                                    O = me->GetOrientation()+(M_PI/2);
                                        break;
                                case 1: // Summon on left side
                                    Z = 0;
                                    O = me->GetOrientation()-(M_PI/2);
                                        break;
                                case 2: // Summon up
                                    Z = 10.0f;
                                    O = 100.0f;
                                        break;
                                case 3: // Summon down
                                        Z = -10.0f;
                                        O = 100.0f;
                                        break;
                                    }
                            }
                            else 
                            {
                                // on heroic summon BP + IC on 8 positions
                                switch(urand(0,7))
                                {
                                case 0: // Summon on right side
                                    Z = 0;
                                    O = me->GetOrientation()+M_PI/2;
                                    break;
                                case 1: // Summon on left side
                                    Z = 0;
                                    O = me->GetOrientation()-M_PI/2;
                                    break;
                                case 2: // Summon up
                                    Z = 0;
                                    O = 100.0f;
                                    break;
                                case 3: // Summon up right
                                    Z = 10.0f;
                                    O = me->GetOrientation()+M_PI/2;
                                    break;
                                case 4: // Summon up left
                                    Z = 10.0f;
                                    O = me->GetOrientation()-M_PI/2;
                                    break;
                                case 5: // Summon down right
                                    Z = 10.0f;
                                    O = me->GetOrientation()+M_PI/2;
                                    break;
                                case 6: // Summon down left
                                    Z = 10.0f;
                                    O = me->GetOrientation()-M_PI/2;
                                    break;
                                case 7: // Summon down
                                    Z = -10.0f;
                                    O = 100.0f;
                                    break;
                                }
                            }

                            if (!FlyUP)
                            {
                                IncendiaryCloudFront = 1;
                                float O = 0;
                                float Distance = 0;
                                for(uint8 i=1; i<=3 ; i++)
                                {
                                    if (i == 1)
                                    {
                                        O = me->GetOrientation()+M_PI/2;
                                        Distance = 10;
                                    }
                                    if (i == 2)
                                    {
                                        O = 0.0;
                                        Distance = 0.0;
                                    }
                                    me->SummonCreature(NPC_INCENDIARY_CLOUD, me->GetPositionX() + Distance*cos(O), me->GetPositionY() + Distance*sin(O), me->GetPositionZ() + 10, 0, TEMPSUMMON_TIMED_DESPAWN, 3200);
                                }
                                me->SummonCreature(NPC_BLAZING_POWER, me->GetPositionX() + 10*cos(me->GetOrientation()-M_PI/2), me->GetPositionY() + 10*sin(me->GetOrientation()-M_PI/2), me->GetPositionZ() + 10, me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 3200);
                                return;
                            }

                            if (IncendiaryCloudFront < 4)
                            {
                                if (!IsHeroic())
                                    if (Cycle == IncendiaryCloudFront)
                                        IncendiaryCloudFront = 4;
                                    else ++IncendiaryCloudFront;
                                else if (IncendiaryCloudFront == 3)
                                    IncendiaryCloudFront = 4;
                                else ++IncendiaryCloudFront;

                                if (O != 100.0f)
                                {
                                    me->SummonCreature(NPC_INCENDIARY_CLOUD, me->GetPositionX() + 10*cos(O), me->GetPositionY() + 10*sin(O), me->GetPositionZ() + Z, 0, TEMPSUMMON_TIMED_DESPAWN, 3200);
                                }
                                else me->SummonCreature(NPC_INCENDIARY_CLOUD, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() + Z, 0, TEMPSUMMON_TIMED_DESPAWN, 3200);
                                return;
                            }
                            if (IncendiaryCloudFront == 4)
                            {
                                if (O != 100.0f)
                                {
                                    me->SummonCreature(NPC_BLAZING_POWER, me->GetPositionX() + 10 * cos(O), me->GetPositionY() + 10 * sin(O), me->GetPositionZ() + Z, me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 3200);
                                    RemoveAuraFromAllPlayers(SPELL_BLAZING_PREVENTION, true, false);
                                    IncendiaryCloudFront = 1;
                                }
                                else
                                {
                                    me->SummonCreature(NPC_BLAZING_POWER, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() + Z, me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 3200);
                                    RemoveAuraFromAllPlayers(SPELL_BLAZING_PREVENTION, true, false);
                                    IncendiaryCloudFront = 1;
                                }
                            }
                            break;
                        }
                        case NPC_FIERY_TORNADO:
                            if (Creature* Target = Unit::GetCreature(*me, TargetGUID))
                                for (uint32 i=1; i<18 ; i++)
                                    if (Creature* Tornado = me->SummonCreature(NPC_FIERY_TORNADO, Target->GetPositionX(), Target->GetPositionY(), Target->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 26000))
                                    {
                                        Tornado->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                        Tornado->AI()->SetData(DATA_TORNADO_SLOT, i);
                                    }
                            break;
                        case NPC_BLAZING_TALON_CLAW:
                            if (Creature* Clawshaper1 = me->SummonCreature(NPC_BLAZING_TALON_CLAW, me->GetPositionX() - 80*cos(M_PI-0.7f), me->GetPositionY() + 80*sin(M_PI-0.7f), 150.0f, 0, TEMPSUMMON_MANUAL_DESPAWN))
                                Clawshaper1->GetMotionMaster()->MovePoint(3, sx - 35*cos(M_PI-0.7f), sy + 35*sin(M_PI-0.7f), 57.0f);
                            if (Creature* Clawshaper2 = me->SummonCreature(NPC_BLAZING_TALON_CLAW, me->GetPositionX() - 80*cos(M_PI+1.6f), me->GetPositionY() + 80*sin(M_PI+1.6f), 150.0f, 0, TEMPSUMMON_MANUAL_DESPAWN))
                                Clawshaper2->GetMotionMaster()->MovePoint(3, sx - 35*cos(M_PI+1.6f), sy + 35*sin(M_PI+1.6f), 57.0f);
                            break;
                        case NPC_HERALD_OF_BURNING:
                            if (Creature* Herald = me->SummonCreature(NPC_HERALD_OF_BURNING, sx, sy, 54.5, 0, TEMPSUMMON_MANUAL_DESPAWN))
                            {
                                Herald->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                                Herald->SetReactState(REACT_PASSIVE);
                                Herald->CastSpell(Herald, SPELL_CATACLYSM, false);
                                Herald->MonsterYell("None escape the rage of the Firelands!", LANG_UNIVERSAL, 0);
                                DoPlaySoundToSet(Herald, SAY_METEOR);
                            }
                            break;
                        }
                }

            void DespawnCreatures(uint32 entry)
            {
                std::list<Creature*> creatures;
                GetCreatureListWithEntryInGrid(creatures, me, entry, 200.0f);

                if (creatures.empty())
                    return;

                for (std::list<Creature*>::iterator iter = creatures.begin(); iter != creatures.end(); ++iter)
                    (*iter)->ForcedDespawn();
            }

            void UpdateAI(const uint32 diff)
            {
                if(!UpdateVictim())
                    return;

                if(Dying && FallingTimer <= diff)
                {
                    me->DealDamage(me,me->GetHealth());
                    return;
                }
                else FallingTimer -= diff;

                if (Dying)
                    return;

                if (Phase == 4 && instance->GetData(TYPE_ALYSRAZOR) == IN_PROGRESS)
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    Phase = STAGE_ONE;
                    me->SetVisible(true);
                }

                if (instance->GetData(TYPE_ALYSRAZOR) == FAIL || instance->GetData(TYPE_ALYSRAZOR) == NOT_STARTED || Phase == 4)
                    return;

                if (firstPhaseTimer <= diff)
                {
                    DespawnMeteors();
                    me->MonsterYell("These skies are MINE!", LANG_UNIVERSAL, 0);
                    DoPlaySoundToSet(me, SAY_SPIRAL_01);
                    Phase = STAGE_TWO;
                    me->MonsterTextEmote("Alysrazor begins to fly in a rapid circle! The harsh winds will remove Wings of Flame!",0,true);
                    StageTimer = 30000;
                    FieryTornado = true;
                    MoveMiddle = false;
                    FieryVortex = false;
                    Cycle = 0;
                    Rounds = 0;
                    firstPhaseTimer = NEVER;
                }
                else firstPhaseTimer -= diff;

                if (Phase != 4)
                {
                    //Achievement
                    Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                    for (Map::PlayerList::const_iterator l = PlList.begin(); l != PlList.end(); ++l)
                        if (Player* player = l->getSource())
                        {
                            if (player->GetAura(99427) && AchievementIC == 0)
                                AchievementIC = 1;
                            if (player->GetAura(98885) && AchievementBF == 0)
                                AchievementBF = 1;
                            if (player->GetAura(99336) && AchievementLS == 0)
                                AchievementLS = 1;
                            if (player->GetAura(99816) && AchievementFT == 0)
                                AchievementFT = 1;
                        }
                }
                
                if (Phase == STAGE_ONE)
                {
                    // Fly movement
                    if (!FlyOut)
                    {
                        if (GameObject* Volcano = me->FindNearestGameObject(GAMEOBJECT_VOLCANO, 300.0f))
                        {
                            Volcano->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);
                            DoUpdateGoForPlayers(Volcano);
                        }
                        FlyFront = 1;
                        FlyMovement(FlyFront);
                    }

                    if (!FlyOut)
                        return;

                    if (FlyFront == 6 && !Check) //1. Fly Up
                    {
                        SpawnWormsTimer = 35000;
                        IncendiaryCloudTimer = 3000;
                        Check = true;
                        SpawnWorms = true;
                    }

                    if (FlyTimer <= diff)
                    {
                        if (FlyUP)
                        {
                            //Heroic
                            if (IsHeroic()) // On heroic and normal is different time of cycle (so different speed)
                            {
                                me->SetSpeed(MOVE_FLIGHT, 2.071f);
                                FlyTimer = 455; // Update timer - (distance/(7*Speed))*1000
                            }
                            else 
                            {
                                me->SetSpeed(MOVE_FLIGHT, 2.16f);
                                FlyTimer = 476; 
                            }
                            me->GetMotionMaster()->MovePoint(3, sx + 55*cos(i), sy + 55*sin(i), 130.0f);
                            i = i + ((M_PI/25)*d);
                            ++Rounds;
                            FlyTimer = 440;
                        }
                        if (FlyFront <= 5 || !FlyUP)
                        {
                            if (FlyFront <= 3)
                                me->SetFacingTo(me->GetAngle(sx, sy));
                            FlyMovement(FlyFront);
                        }
                    }
                    else FlyTimer -= diff;
                    
                    if (!IsHeroic())
                    {
                        if (Rounds == 101 && Cycle != 3)
                        {
                            me->SetFloatValue(UNIT_FIELD_COMBATREACH, 35);
                            Rounds = 0;
                            FlyUP = false;
                            FlyDownFirst ? FlyFront = 6 : FlyFront = 3;
                            FlyDownFirst = !FlyDownFirst;
                            ++Cycle;
                            FlyMovement(FlyFront);
                        }
                    } // Heroic
                    else if ((Rounds == 138 && Cycle == 1) || (Rounds == 151 && Cycle == 2))
                        {
                            d = urand(0,1) ? 1.0f : -1.0f;

                            Rounds = 0;
                            ++Cycle;
                            IncendiaryCloudTimer = 15000;
                        }
                    //if (Creature* Summon = me->FindNearestCreature(NPC_FIRESTORM, 200.0f, true))
                        //me->SetFacingToObject(Summon);

                    // Adds Timers
                    if (SummonInitiate && SummonInitiateTimer <= diff) //Initiate Timer
                    {
                        SummonInitiateTimer = 31000;
                        SummonNPC(NPC_BLAZING_TALON_INITIATE);
                    }
                    else SummonInitiateTimer -= diff;

                    if (Check && SpawnWorms && SpawnWormsTimer <= diff) //Worms Timer
                    {
                        me->MonsterTextEmote("Fiery Lava Worms erupt from the ground!",0,true);
                        SummonNPC(NPC_PLUMP_LAVA_WORM);

                        if(!IsHeroic())
                            SpawnWorms = false;
                        else
                            SpawnWormsTimer = NEVER; // Should spawn 8 seconds after eggs hatch
                    }
                    else SpawnWormsTimer -= diff;

                    if (!IsHeroic() && Check && !me->FindNearestCreature(NPC_PLUMP_LAVA_WORM,300.0f) && !SpawnWorms) //Worms Respawn
                    {
                        SpawnWorms = true;
                        SpawnWormsTimer = 15000;
                    }

                    if (EggSatchelTimer <= diff) //EggSatchel Timer
                    {
                        if(IsHeroic())
                            SpawnWormsTimer = 10000 + 2500;

                        SummonNPC(NPC_EGG_SATCHEL);

                        if(IsHeroic())
                            EggSatchelTimer = 60000 + 26000;
                        else
                            EggSatchelTimer = NEVER; // Spawn only one pack of Hatchlings in normal mode
                    }
                    else EggSatchelTimer -= diff;

                    if (firestormTimer <= diff && IsHeroic())
                    {
                        DespawnCreatures(NPC_BRUSHFIRE); // Despawn Brushfire on Firestorm
                        //me->GetMotionMaster()->MovePoint(3, sx + (55*cos(i)*d), sy + 55*sin(i), 135.0f);

                        if (me->FindNearestCreature(NPC_BLAZING_TALON_INITIATE, 200.0f, true))
                        {
                            std::list<Creature*> creatures;
                            GetCreatureListWithEntryInGrid(creatures, me, NPC_BLAZING_TALON_INITIATE, 100.0f);
                            if (!creatures.empty())
                                for (std::list<Creature*>::iterator iter = creatures.begin(); iter != creatures.end(); ++iter)
                                    if ((*iter)->IsInWorld() && (*iter)->isAlive())
                                        (*iter)->CastSpell((*iter), SPELL_BLAZING_SHIELD_HC, false);
                        }

                        if (Creature* Summon = me->SummonCreature(NPC_FIRESTORM, sx, sy, 70.0f, 0, TEMPSUMMON_TIMED_DESPAWN, 15000))
                        {
                            Summon->setFaction(35);
                            me->StopMoving();
                            me->SetFacingToObject(Summon);
                            me->SetUInt64Value(UNIT_FIELD_TARGET,Summon->GetGUID());
                            DoCast(SPELL_FIRESTORM_HC);
                        }
                        feathersTimer = 5000 + 5000 + 1000; // 5 second cast time + 5 second duration + 1s as small delay
                        FlyTimer = feathersTimer;
                        firestormTimer = 81000;
                    }
                    else firestormTimer -= diff;

                    if (feathersTimer <= diff)
                    {
                        DespawnMeteors();
                        // Summon molten feathers
                        uint8 max = Is25ManRaid() ? 22 : 9;
                        float z = me->GetMap()->GetHeight2(me->GetPositionX(),me->GetPositionY(),me->GetPositionZ());

                        for(uint8 l = 0; l < max ; l++)
                        {
                            float iCycleSLot = (M_PI/max)*l; // summon feather in cycle - only for visual effect
                            me->SummonCreature(53089, sx + 2*cos(iCycleSLot), sy + 2*sin(iCycleSLot), z, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 35000);
                        }
                        feathersTimer = NEVER;
                        FlyTimer = 100; // Continue flying
                        me->SetUInt64Value(UNIT_FIELD_TARGET,0);
                    }
                    else feathersTimer -= diff;

                    // FlyPhase Spells
                    if (Check && IncendiaryCloudTimer <= diff) //Clouds + ring
                    {
                        IsHeroic() ? IncendiaryCloudTimer = 1000 : IncendiaryCloudTimer = 4000/(Cycle + 1);
                        if (!FlyUP)
                            IncendiaryCloudTimer = 4000;
                        SummonNPC(NPC_INCENDIARY_CLOUD);
                    }
                    else IncendiaryCloudTimer -= diff;

                    //Heroic
                    if (HeraldTimer <= diff && IsHeroic())
                    {
                        heraldCounter++;
                        SummonNPC(NPC_HERALD_OF_BURNING);
                        // There are gaps in hearalds, due to Firestorm
                        if (heraldCounter == 2 || heraldCounter == 4)
                            HeraldTimer = 55000;
                        else
                            HeraldTimer = 32000;
                    }
                    else HeraldTimer -= diff;
                }

                if (Phase == STAGE_TWO)
                {
                    //Fiery Vortex Spells
                    if (!MoveMiddle)
                    {
                        MoveMiddle = true;
                        me->SetSpeed(MOVE_FLIGHT, 5.0f);
                        me->GetMotionMaster()->MovePoint(3, sx, sy, 60.0f);
                    }
                    else if (me->GetPositionY() == sy && !FieryVortex)
                        {
                            HarshTimer = 5001;
                            FieryVortex = true;
                            me->CastSpell(me, 99604, false);                                    //Fly Anim
                            if (Creature* Target = me->SummonCreature(NPC_FIERY_VORTEX, sx, sy, 55.0f, 0, TEMPSUMMON_TIMED_DESPAWN, 31000))
                            {
                                wormsCount = 0;
                                Target->CastSpell(Target, SPELL_FIERY_VORTEX, false);
                                TargetGUID = Target->GetGUID();
                                StageTimer = 30000; 
                                HarshTimer = 5000;
                                FlyTimer = 15;
                            }
                        }

                    if (FieryVortex && HarshTimer <= diff)
                    {
                        if (me->FindNearestCreature(NPC_FIERY_VORTEX, 200.0f))
                        {
                            if (FieryTornado)
                            {
                                SummonNPC(NPC_FIERY_TORNADO);
                                FieryTornado = false;
                            }

                            if (FieryTornado == false)
                                RemoveAuraFromAllPlayers(98619, true, false);
                        }

                        HarshTimer = 4900;
                    }
                    else HarshTimer -= diff;

                    // Boss flying
                    if (FlyTimer <= diff && MoveMiddle && FieryVortex)
                    {
                        FlyTimer = 3;
                        me->GetMotionMaster()->MovePoint(3, sx + 40*cos(i), sy + 40*sin(i), 90.0f);
                        i = i + (M_PI/50);
                    }
                    else FlyTimer -= diff;

                    if (FlyUP && BlazingPowerTimer <= diff) //Rings
                    {
                        //SummonNPC(NPC_BLAZING_POWER);
                        BlazingPowerTimer = 4000;
                    }
                    else BlazingPowerTimer -= diff;

                    // Change phase
                    if (StageTimer <= diff)
                    {
                        me->RemoveAura(99604);
                        RemoveAuraFromAllPlayers(SPELL_HARSH_WINDS, true, false);
                        Phase = STAGE_THREE;
                        MoveMiddle = false;
                    }
                    else StageTimer -= diff;
                }

                if (Phase == STAGE_THREE)
                {
                    if (!MoveMiddle)
                    {
                        me->SetFloatValue(UNIT_FIELD_COMBATREACH, 30.0f);
                        me->SetPower(POWER_MANA, 0);
                        RemoveAuraFromAllPlayers(SPELL_MOLTEN_FEATHER, true, false);
                        RemoveAuraFromAllPlayers(SPELL_FEATHER_BAR, true, false);
                        RemoveAuraFromAllPlayers(SPELL_FEATHER_BAR, false, false);
                        me->MonsterYell("Fire... fire...", LANG_UNIVERSAL, 0);
                        DoPlaySoundToSet(me, SAY_BURNOUT);
                        me->GetMotionMaster()->MovePoint(3, sx, sy, 54.3f);
                        SummonNPC(NPC_BLAZING_TALON_CLAW);
                        me->CastSpell(me, SPELL_BURNOUT, false);
                        me->CastSpell(me, SPELL_SPARK, false);
                        MoveMiddle = true;
                        StageTimer = 33333;
                    }

                    if (MoveMiddle && StageTimer <= diff) 
                    {
                        me->RemoveAura(SPELL_SPARK);
                        me->PlayOneShotAnimKit(ANIM_KIT_WAKE_UP);
                        me->CastSpell(me, SPELL_IGNITED, false);
                        me->SetSpeed(MOVE_FLIGHT, 2.0f);
                        me->SetPower(POWER_MANA, 50);
                        Phase = STAGE_FOUR;
                        me->SetFloatValue(UNIT_FIELD_COMBATREACH, 10);
                        FlyTimer = 3000;
                        FlyUP = false;
                    }
                    else StageTimer -= diff;
                }

                if (Phase == STAGE_FOUR)
                {
                    if (!FlyUP && FlyTimer <= diff)
                    {
                        me->SetSpeed(MOVE_FLIGHT, 1.5f);
                        me->RemoveAura(SPELL_BURNOUT);
                        me->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ()+7.0f);
                        me->MonsterYell("Reborn in flame!", LANG_UNIVERSAL, 0);
                        DoPlaySoundToSet(me, SAY_REBORN);
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->SetInCombatWithZone();
                        StageTimer = 23000;
                        FlyTimer = 2000;
                        FlyUP = true;
                        me->CastSpell(me, SPELL_BLAZING_CLAW, false);
                    }
                    else FlyTimer -= diff;

                    if (FlyUP && StageTimer <= diff && Enrage != 2)
                    {
                        me->SetUInt64Value(UNIT_FIELD_TARGET, 0);
                        me->SetPower(POWER_MANA, 100);
                        me->SetReactState(REACT_PASSIVE);
                        me->MonsterYell("I will burn you from the sky!", LANG_UNIVERSAL, 0);
                        firstPhaseTimer = IsHeroic() ? 250000 : 175000;
                        DoPlaySoundToSet(me, SAY_SPIRAL_02);
                        me->SetFloatValue(UNIT_FIELD_COMBATREACH, 55);
                        me->RemoveAura(SPELL_IGNITED);
                        me->RemoveAura(SPELL_BLAZING_CLAW);
                        me->CastSpell(me, SPELL_FULL_POWER, false);
                        // Summon molten feathers
                        uint8 max = Is25ManRaid() ? 22 : 9;

                        for(uint8 l= 0; l < max ; l++)
                        {
                            float iCycleSLot = (M_PI/max)*l; // summon feather in cycle - only for visual effect
                            float z = me->GetMap()->GetHeight2(me->GetPositionX() + 2 * cos(iCycleSLot), me->GetPositionY() + 2 * sin(iCycleSLot), me->GetPositionZ()) + 2.0f;
                            me->SummonCreature(53089, me->GetPositionX() + 2*cos(iCycleSLot), me->GetPositionY() + 2*sin(iCycleSLot), z, urand(0,6), TEMPSUMMON_TIMED_DESPAWN, 35000);
                        }

                        // Reset STAGE_ONE
                        Phase = 6;
                        IncendiaryCloudTimer = 10000;
                        HeraldTimer = 18000;
                        EggSatchelTimer = 22000;
                        firestormTimer = 70000;
                        heraldCounter = 0;
                        if (!me->FindNearestCreature(NPC_PLUMP_LAVA_WORM,200.0f))
                        {
                            SpawnWormsTimer = 35000;
                            SpawnWorms = true;
                        }
                        FlyUP = false;
                        FlyDownFirst = false;
                        InitiatesFirst = 0;
                        IsHeroic() ? FlyFront = 5 : FlyFront = 8;
                        IsHeroic() ? d=1.0f : d=-1.0f;
                        FlyMovement(FlyFront);
                        Cycle = 1;
                        me->SetSpeed(MOVE_FLIGHT, 3.0f);
                        ++Enrage;
                        FlyUP = false;
                    }
                    else StageTimer -= diff;

                    if (!UpdateVictim() || !FlyUP)
                        return;

                    if ((FlyTimer <= diff || me->GetMotionMaster()->GetCurrentMovementGeneratorType() == 0) && me->GetDistance(me->getVictim()) >= 2.0f)
                    {
                        me->GetMotionMaster()->MovePoint(0, me->getVictim()->GetPositionX(), me->getVictim()->GetPositionY(), me->getVictim()->GetPositionZ()+7.0f);
                        FlyTimer = 500;
                    }

                    if (me->GetDistance(me->getVictim()) < 1.0f)
                        me->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());

                    if (me->GetPositionZ() < 60.0f)
                        me->GetMotionMaster()->MovePoint(0, me->getVictim()->GetPositionX(), me->getVictim()->GetPositionY(), 65.0f);
                }

                if (Phase == 6 && me->GetMotionMaster()->GetCurrentMovementGeneratorType() == 0 && me->GetPositionZ() > 100.0f)
                {
                    FlyUP = true;
                    Phase = STAGE_ONE;
                    FlyTimer = 2000;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_AlysrazorAI(creature);
        }
};

/*********************MOLTEN_FEATHER_AI***********************/
class npc_Molten_Feather : public CreatureScript
{
    public:
        npc_Molten_Feather() : CreatureScript("npc_Molten_Feather") { }

        bool OnGossipHello(Player* player, Creature* creature)
            {
                InstanceScript* instance;
                instance = creature->GetInstanceScript();
                if (instance->GetData(TYPE_ALYSRAZOR) != IN_PROGRESS)
                {
                    Unit * alys = Unit::GetUnit(*player,instance->GetData64(TYPE_ALYSRAZOR));
                    if(alys && alys->ToCreature())
                    {
                        alys->ToCreature()->SetInCombatWithZone();
                    }
                }
                else 
                {
                    Powers Powere = (player->getPowerType());
                    if (!player->GetAura(SPELL_FEATHER_BAR))
                    {
                        player->AddAura(SPELL_FEATHER_BAR, player);
                    }
                    if (!player->GetPower(POWER_SCRIPTED))
                    {
                        player->setPowerType(POWER_SCRIPTED);
                        player->setPowerType(Powere);
                        player->SetMaxPower(POWER_SCRIPTED, 3);
                        player->SetPower(POWER_SCRIPTED, 0);
                    }
                    player->CastSpell(player, SPELL_MOLTEN_FEATHER, true);
                }
                creature->ForcedDespawn();
                return true;
            }

        struct npc_Molten_FeatherAI : public ScriptedAI
        {
            npc_Molten_FeatherAI(Creature* creature) : ScriptedAI(creature) 
            {
                me->SetFloatValue(UNIT_FIELD_COMBATREACH, 10);
                cast = false;
                FallTimer = 1500;
            }

            bool cast;
            uint32 FallTimer;

            void UpdateAI(const uint32 diff)
            {
                if (!cast && FallTimer <= diff)
                {
                    cast = true;
                    me->GetMotionMaster()->MoveFall();
                    me->CastSpell(me, SPELL_MOLTEN_FEATHER_COS, false);
                }
                else FallTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_Molten_FeatherAI(creature);
        }
};

class npc_Captive_Duid : public CreatureScript
{
    public:
        npc_Captive_Duid() : CreatureScript("npc_Captive_Duid") { }

        struct npc_Captive_DuidAI : public ScriptedAI
        {
            npc_Captive_DuidAI(Creature* creature) : ScriptedAI(creature)
            {
                me->CastSpell(me,100556,true);
            }

            void JustDied(Unit* /*Killer*/)
            {
                //if (Creature* Target = me->SummonCreature(NPC_LAVA_WORM_TARGET, 111.7f, -400.8f, 100.9f, 0, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 3000))
                    //me->CastSpell(Target, SPELL_COSMETIC_DEATH, false);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_Captive_DuidAI(creature);
        }
};

class npc_fiery_vortex : public CreatureScript
{
public:
    npc_fiery_vortex() : CreatureScript("npc_fiery_vortex") { }

    struct npc_fiery_vortexAI : public ScriptedAI
    {
        npc_fiery_vortexAI(Creature* creature) : ScriptedAI(creature){}

        uint32 harshWindsTimer;
        uint32 blazing_timer;

        void Reset()
        {
            me->SetReactState(REACT_AGGRESSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE|UNIT_FLAG_NOT_SELECTABLE);
            me->SetInCombatWithZone();
            //me->CastSpell(me, SPELL_FIERY_VORTEX, false); - > Why player can't see casting animation when casted here or in entercombat ? :D
            me->setFaction(14);
            harshWindsTimer = 6000;
            blazing_timer = 4000;
        }

        void UpdateAI(const uint32 diff)
        {
            if (harshWindsTimer <= diff)
            {
                me->CastSpell(me,SPELL_HARSH_WINDS,true);
                harshWindsTimer = 1000;
            }
            else harshWindsTimer -= diff;

            if (blazing_timer <= diff)
            {
                for (uint8 i = 0; i < 4 ; i++)
                {
                    float u; // Distance
                    float l; // RAD
                    l = (urand(1,628))/100;
                    uint32 t = urand(1,3);
                    u = 15+(t*9);
                    if (Creature* Ring = me->SummonCreature(NPC_BLAZING_POWER, me->GetPositionX() + u*cos(l), me->GetPositionY() + u*sin(l), 56.0f, 0, TEMPSUMMON_TIMED_DESPAWN, 3200))
                        Ring->GetMotionMaster()->MovePoint(3, me->GetPositionX() + u*cos(l+0.01f), me->GetPositionY() + u*sin(l+0.01f), 56.0f); //SetOrientation
                }
                blazing_timer = 5000;
            }
            else blazing_timer -= diff;
        }

        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_fiery_vortexAI(creature);
        }
};

/********************MAJORDOMO_START_EVENT********************/
class npc_Fendral : public CreatureScript
{
    public:
        npc_Fendral() : CreatureScript("npc_Fendral") { }

        struct npc_FendralAI : public ScriptedAI
        {
            npc_FendralAI(Creature* creature) : ScriptedAI(creature) 
            {
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
                instance = creature->GetInstanceScript();
                me->SetReactState(REACT_AGGRESSIVE);
            }

            uint32 KillTimer;
            uint32 Timer;

            bool StartIntro;
            bool FlyUp;

            InstanceScript* instance;

            void Reset()
            {
                StartIntro = false;
                FlyUp = false;
            }

            void EnterEvadeMode()
            {
                StartIntro = false;
                FlyUp = false;
                me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_NONE);
                ScriptedAI::EnterEvadeMode();
            }

            void MoveInLineOfSight(Unit* who)
            {
                if (who && who->ToPlayer() && !who->ToPlayer()->isGameMaster() && who->GetDistance(me) <= 40.0f)
                    if (!StartIntro)
                    {
                        me->SetInCombatWithZone();
                        me->CastSpell(me, SPELL_SMOULDERING_ROOTS, true);
                        me->HandleEmoteCommand(EMOTE_ONESHOT_TALK);

                        me->MonsterYell("What have we here - visitors to our kingdom in the Firelands?", LANG_UNIVERSAL, 0);
                        DoPlaySoundToSet(me, SAY_FENDRAL_01);
                        me->GetMotionMaster()->MovePoint(3, 29.02f, -329.64f, 52.4f);
                        Timer = 10000;
                        StartIntro = true;
                    }

                ScriptedAI::MoveInLineOfSight(who);
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (Timer <= diff && StartIntro && !FlyUp)
                {
                    me->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                    me->MonsterYell("You mortals may remember Alysra, who spirited me to freedom in Mount Hyjal. She, too has been reborn. Born of flame!", LANG_UNIVERSAL, 0);
                    DoPlaySoundToSet(me, SAY_FENDRAL_02);
                    Timer = 18000;
                    KillTimer = 10000;
                    FlyUp = true;
                }
                else Timer -= diff;

                if (KillTimer <= diff && FlyUp)
                {
                    KillTimer = 100000;
                    if (Creature* Target = me->FindNearestCreature(NPC_CAPTIVE_DRUID, 200.0f))
                        me->CastSpell(Target, SPELL_SACRIFICE, false);
                    me->CastSpell(me, SPELL_FIGHT_START, false);
                }
                else KillTimer -= diff;

                if (Timer <= diff && FlyUp)
                {
                    me->SetSpeed(MOVE_FLIGHT, 0.8f);
                    me->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                    me->MonsterYell("I wish I could watch her reduce your pitiful band to cinders, but I am needed elsewhere. Farewell!", LANG_UNIVERSAL, 0);
                    DoPlaySoundToSet(me, SAY_FENDRAL_03);
                    me->CastSpell(me, SPELL_FENDRAL_TRANSFORM, false);
                    me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                    me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
                    me->GetMotionMaster()->MovePoint(3, 29.02f, -329.64f, 140.0f);
                    Timer = 100000;
                }

                if (me->GetPositionZ() >= 110.0f && instance->GetData(TYPE_ALYSRAZOR) == NOT_STARTED)
                {
                    Unit * alys = Unit::GetUnit(*me,instance->GetData64(TYPE_ALYSRAZOR));
                    if(alys && alys->ToCreature())
                    {
                        alys->ToCreature()->SetInCombatWithZone();
                    }
                    me->ForcedDespawn();
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_FendralAI(creature);
        }
};

/*************************MOLTEN_EGG**************************/
class npc_Molten_Egg : public CreatureScript
{
    public:
        npc_Molten_Egg() : CreatureScript("npc_Molten_Egg") { }
        struct npc_Molten_EggAI : public ScriptedAI
        {
            npc_Molten_EggAI(Creature* creature) : ScriptedAI(creature) 
            {
                CastTimer = 10000;
                SpawnTimer = 15500;
                me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE|UNIT_FLAG_NON_ATTACKABLE);
                ALYSRAZOR_GUID = 0;
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
            }

            uint32 CastTimer;
            uint32 SpawnTimer;
            uint32 Imprinte;
            uint64 ALYSRAZOR_GUID;

            void IsSummonedBy(Unit* pSummoner)
            {
                ALYSRAZOR_GUID = pSummoner->GetGUID();
            }

            void UpdateAI(const uint32 diff)
            {
                if (CastTimer <= diff)
                {
                    me->CastSpell(me, SPELL_EXPLOSION_EGG, false);
                    if (me->GetDistance2d(-47.0f, -266.0f) < 1.0f)
                    if( Unit * pAlys = Unit::GetUnit(*me,ALYSRAZOR_GUID))
                    {
                        pAlys->MonsterTextEmote("The Molten Eggs begin to hatch!", 0, true);
                    }
                    CastTimer = 50000;
                }
                else CastTimer -= diff;

                if (SpawnTimer <= diff)
                {
                    if( Unit * pAlys = Unit::GetUnit(*me,ALYSRAZOR_GUID))
                    {
                        pAlys->SummonCreature(NPC_VORACIOUS_HATCHLING, me->GetPositionX(), me->GetPositionY(), 55.3f);
                    }

                    me->ForcedDespawn(500);
                    SpawnTimer = 50000;
                }
                else SpawnTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_Molten_EggAI(creature);
        }
};

/************************EGG_SATCHELS*************************/
class npc_Blazing_Broodmother  : public CreatureScript
{
    public:
        npc_Blazing_Broodmother() : CreatureScript("npc_Blazing_Broodmother") { }

        struct npc_Blazing_BroodmotherAI : public ScriptedAI
        {
            npc_Blazing_BroodmotherAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetFlying(true);
                me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                me->SetReactState(REACT_PASSIVE);
                me->SetSpeed(MOVE_FLIGHT, 2.5f);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
                SpawnEggTimer = 8000;
            }

            uint32 SpawnEggTimer;

             void UpdateAI(const uint32 diff)
            {
                if (SpawnEggTimer <= diff)
                {
                    SpawnEggTimer = 100000;

                    if (Creature* pEgg1 = me->FindNearestCreature(NPC_MOLTEN_EGG,20.0f,true))
                    {
                        pEgg1->ExitVehicle();
                        pEgg1->GetMotionMaster()->MoveFall();
                        pEgg1->SetReactState(REACT_PASSIVE);
                        //me->GetMotionMaster()->MovePoint(0,-111.0f,-277.0f,95.0f);
                        me->GetMotionMaster()->MoveTargetedHome();
                    }
                }
                else SpawnEggTimer -=diff;
            }

        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_Blazing_BroodmotherAI(creature);
        }
};

/************************MOLTEN_METEOR************************/
class npc_molten_meteor : public CreatureScript
{
    public:
        npc_molten_meteor() : CreatureScript("npc_molten_meteor") { }

        struct npc_molten_meteorAI : public ScriptedAI
        {
            npc_molten_meteorAI(Creature* creature) : ScriptedAI(creature)
            {
                if(Unit * pHerald = me->FindNearestCreature(NPC_HERALD_OF_BURNING,500.0f,true))
                    pHerald->CastSpell(pHerald,39656,true); // Kneel
                distance = 55.0f;
                me->setFaction(16);
                me->GetMotionMaster()->MoveFall();
                me->SetReactState(REACT_PASSIVE);
                me->SetSpeed(MOVE_FLIGHT, 5.0f,true);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
                i = urand(1,8);
                switch(uint32(i))
                {
                case 8:
                    distance = 61.0f;
                    break;
                case 1:
                    distance = 73.0f;
                    break;
                case 2:
                    distance = 55.0f;
                    break;
                case 3:
                    distance = 53.0f;
                    break;
                case 4:
                    distance = 55.0f;
                    break;
                case 5:
                    distance = 55.0f;
                    break;
                case 6:
                    distance = 62.5f;
                    break;
                case 7:
                    distance = 70.0f;
                    break;
                }
                i = i*(M_PI/4);
                Fall = false;
                FallTimer = 2200;
            }

            uint32 FallTimer;

            float i;
            float distance;

            bool Fall;

            void JustDied(Unit* /*Killer*/)
            {
                //DoCast(SPELL_SUMMON_GO_METEOR);

                float angle = 0.0f;
                if (Creature* pAlys = me->FindNearestCreature(NPC_ALYSRAZOR, 200.0f, true))
                    angle = me->GetAngle(pAlys);

                float z  = me->GetMap()->GetHeight2(me->GetPositionX(),me->GetPositionY(),me->GetPositionZ());

                //me->SummonGameObject(208966,me->GetPositionX(),me->GetPositionY(),z,angle,0,0,0,0,0); // Summon invis GO, only for LoS
                GameObject* pGameObj = new GameObject;

                if (pGameObj->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_GAMEOBJECT), 208966, me->GetMap(),
                    me->GetPhaseMask(), me->GetPositionX(), me->GetPositionY(), z,angle, 0.0f, 0.0f, 0.0f, 0.0f, 100, GO_STATE_READY))
                {
                    pGameObj->SetRespawnTime(300000/IN_MILLISECONDS);
                    pGameObj->SetSpellId(SPELL_SUMMON_GO_METEOR);
                    me->GetMap()->Add(pGameObj);
                    pGameObj->EnableCollision(true);
                }
                else
                    delete pGameObj;

                if (Creature* pMeteor = me->SummonCreature(53784,me->GetPositionX(),me->GetPositionY(),z,angle))
                    pMeteor->CastSpell(pMeteor, SPELL_MOLTEN_METOER_DEAD, false);
                me->ForcedDespawn();
            }

            void UpdateAI(const uint32 diff)
            {
                if (FallTimer <= diff && !Fall)
                {
                    if (!me->GetAura(99215))
                        me->AddAura(99215, me);

                    if (Creature* Herald = me->FindNearestCreature(NPC_HERALD_OF_BURNING, 200.0f, true))
                    {
                        Herald->setDeathState(JUST_DIED);
                        if (TempSummon* summon = Herald->ToTempSummon())
                            summon->ForcedDespawn(3000);
                    }
                    me->SetSpeed(MOVE_RUN, 0.5f,true);
                    DoCast(SPELL_METEORIC_IMPACT);
                    Fall = true;
                }
                else FallTimer -=diff;
                
                if (Fall && me->isAlive())
                {
                    me->GetMotionMaster()->MovePoint(0, me->GetPositionX() + 60*cos(i), me->GetPositionY() + 60*sin(i), 60.0f);
                    if (me->GetDistance(-49.34f, -277.97f, 70.0f) > distance)
                    {
                        DoCast(SPELL_METEOR_CRACK);
                        for(uint8 l=0; l<3; l++)
                        {
                            if (Creature* Boulder = me->FindNearestCreature((53496+l) ,5.0f,true))
                            {
                                Boulder->AI()->SetData(DATA_BOULDER, i);
                            }
                        }
                        me->ForcedDespawn(200);
                    }
                }
            }

        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_molten_meteorAI(creature);
        }
};

class npc_bouder : public CreatureScript
{
    public:
        npc_bouder() : CreatureScript("npc_bouder") { }

        struct npc_bouderAI : public ScriptedAI
        {
            npc_bouderAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                Initialize = false;
                Entry = me->GetEntry() - 53496;
                i = 0.0f;
                DoCast(SPELL_MOLTEN_BOULDER);
            }

            bool Initialize;

            float i;

            uint32 Entry;

            void SetData(uint32 Type, uint32 Data)
            {
                if (Type == DATA_BOULDER)
                {
                    i = Data;
                    i = i + M_PI;
                    i = i + Entry*0.78f;
                    Initialize = true;
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (Initialize && i != 0.0f)
                {
                    me->GetMotionMaster()->MovePoint(0, me->GetPositionX() + 200*cos(i), me->GetPositionY() + 200*sin(i), 56.0f);
                    if (me->GetDistance(me->GetHomePosition()) > 100.0f)
                        me->ForcedDespawn();
                }

                if (!Initialize)
                    me->ForcedDespawn();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_bouderAI(creature);
        }
};

/******************BLAZING_TALON_CLAWSHAPER_AI****************/
class npc_Blazing_Talon_Clawshaper : public CreatureScript
{
    public:
        npc_Blazing_Talon_Clawshaper() : CreatureScript("npc_Blazing_Talon_Clawshaper") { }

        struct npc_Blazing_Talon_ClawshaperAI : public ScriptedAI
        {
            npc_Blazing_Talon_ClawshaperAI(Creature* creature) : ScriptedAI(creature)
            {
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
                me->SetFlying(true);
                me->SetFloatValue(OBJECT_FIELD_SCALE_X, 2.0f);
                me->SetDisplayId(38317);
                ElfForm = false;
                FlyAway = false;
                me->SetReactState(REACT_DEFENSIVE);
                FlyTimer = 8000;
                me->SetSpeed(MOVE_FLIGHT, 3.0f);
            }

            uint32 IgnitionTimer;
            uint32 FlyTimer;

            bool FlyAway;
            bool ElfForm;

            void UpdateAI(const uint32 diff)
            {
                if (FlyTimer <= diff && !ElfForm)
                {
                    me->SetFlying(false);
                    me->SetFloatValue(OBJECT_FIELD_SCALE_X, 1.0f);
                    me->MonsterYell("Together we call upon the lord of fire!", LANG_UNIVERSAL, 0);
                    DoPlaySoundToSet(me, SAY_CLAWSHAPER);
                    ElfForm = true;
                    DoCast(SPELL_BLAZING_TALON_TRAN);
                    me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
                    IgnitionTimer = 2000;
                }
                else FlyTimer -= diff;

                if (IgnitionTimer <= diff && ElfForm)
                {
                    if (!me->hasUnitState(UNIT_STAT_CASTING))
                        if (Creature* Aly = me->FindNearestCreature(NPC_ALYSRAZOR, 200.0f))
                            me->CastSpell(Aly, SPELL_IGNITION, false);
                    IgnitionTimer = 5000;
                }
                else IgnitionTimer -= diff;

                if (Creature* Alysrazor = me->FindNearestCreature(NPC_ALYSRAZOR,200.0f, true))
                    if (Alysrazor->GetPower(POWER_MANA) >= 50)
                        FlyAway = true;

                if (FlyAway)
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
                    me->SetFlying(true);
                    FlyTimer = 1000000;
                    ElfForm = false;
                    FlyAway = false;
                    float sx = 41.78f;
                    float sy = -275.97f;
                    me->RemoveAura(SPELL_BLAZING_TALON_TRAN);
                    me->SetFloatValue(OBJECT_FIELD_SCALE_X, 2.0f);
                    me->ForcedDespawn(10000);
                    float Distance = me->GetDistance(sx, sy, 52.0f);
                    float SinA = (me->GetPositionY()-sy)/Distance;
                    float CosA = (me->GetPositionX()-sx)/Distance;
                    me->GetMotionMaster()->MovePoint(3, sx + 80*CosA, sy + 80*SinA, 150.0f);
                }

                if (ElfForm && !FlyAway)
                    DoMeleeAttackIfReady();
            }

        private:
            InstanceScript* _instance;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_Blazing_Talon_ClawshaperAI(creature);
        }
};

/*******************BLAZING_TALON_INITIATE_AI*****************/
class npc_Blazing_Talon_Initiate : public CreatureScript
{
    public:
        npc_Blazing_Talon_Initiate() : CreatureScript("npc_Blazing_Talon_Initiate") { }

        struct npc_Blazing_Talon_InitiateAI : public ScriptedAI
        {
            npc_Blazing_Talon_InitiateAI(Creature* creature) : ScriptedAI(creature)
            {
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
                me->SetFlying(true);
                me->SetDisplayId(38317);
                FlyTimer = 6000;
                Transform = false;
                me->SetReactState(REACT_PASSIVE);
                me->SetSpeed(MOVE_FLIGHT, 3.0f);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
                Male = false;
            }

            uint32 FlyTimer;

            uint32 CastTimer; // cast random spell

            bool Transform;
            bool Male;

            void UpdateAI(const uint32 diff)
            {
                if (FlyTimer <= diff && !Transform)
                {
                    me->SetFlying(false);

                    // !!!!!!!!!!!!!!!!!!!!!!!!!!
                    //me->SetDisableGravity(false);

                    me->GetMotionMaster()->MoveFall();
                    me->CastSpell(me, SPELL_BLAZING_TALON_TRAN, false);
                    switch (urand(1,2))
                    {
                    case 1:
                        me->SetDisplayId(38558);
                        Male = false;
                        break;
                    case 2:
                        Male = true;
                        break;
                    }
                    CastTimer = 3000;
                    Transform = true;
                    me->SetInCombatWithZone();
                    if (Male)
                        switch(urand(1,3))
                        {
                        case 1:
                            me->MonsterYell("We call upon you, Firelord!", LANG_UNIVERSAL, 0);
                            DoPlaySoundToSet(me, SAY_DRUID_01M);
                            break;
                        case 2:
                            me->MonsterYell("Behold His power!", LANG_UNIVERSAL, 0);
                            DoPlaySoundToSet(me, SAY_DRUID_02M);
                            break;
                        case 3:
                            me->MonsterYell("Witness the majesty of flame!", LANG_UNIVERSAL, 0);
                            DoPlaySoundToSet(me, SAY_DRUID_03M);
                            break;
                        }
                        else
                            switch(urand(1,3))
                            {
                            case 1:
                                me->MonsterYell("We call upon you, Firelord!", LANG_UNIVERSAL, 0);
                                DoPlaySoundToSet(me, SAY_DRUID_01F);
                                break;
                            case 2:
                                me->MonsterYell("Behold His power!", LANG_UNIVERSAL, 0);
                                DoPlaySoundToSet(me, SAY_DRUID_02F);
                                break;
                            case 3:
                                me->MonsterYell("Let the unbelievers perish in fire!", LANG_UNIVERSAL, 0);
                                DoPlaySoundToSet(me, SAY_DRUID_03F);
                                break;
                            }
                }
                else FlyTimer -= diff;

                if (CastTimer <= diff)
                {
                    if (!me->IsNonMeleeSpellCasted(false))
                    {
                        switch(urand(0,1))
                        {
                        case 0:
                            me->CastSpell(me, SPELL_BRUSHFIRE, false);
                            break;
                        case 1:
                            if (Unit* Target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true,-98619)) // Dont't target fly boys
                                me->CastSpell(Target, SPELL_FIEROBLAST, false);
                            break;
                        }
                       CastTimer = 3500;
                    }
                }
                else CastTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_Blazing_Talon_InitiateAI(creature);
        }
};

/********************VORACIOUS_HATCHLING_AI*******************/
class npc_Voracious_Hatchling : public CreatureScript
{
    public:
        npc_Voracious_Hatchling() : CreatureScript("npc_Voracious_Hatchling") { }

        struct npc_Voracious_HatchlingAI : public ScriptedAI
        {
            npc_Voracious_HatchlingAI(Creature* creature) : ScriptedAI(creature)
            {
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
                //me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true); - > cant be set due to Imprinted buff on Hatchling with this aura
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
                me->PlayOneShotAnimKit(ANIM_HATH);
                me->AddAura(SPELL_SATIATED, me);
                instance = me->GetInstanceScript();
                VICTIM_GUID = 0;
                me->SetInCombatWithZone();
                ALYSRAZOR_GUID = 0;
                me->SetSpeed(MOVE_RUN,2.0f,true);
            }

            uint32 GushingWoundTimer;
            uint32 losTimer;
            uint32 clearImprintedTimer;
            InstanceScript * instance;
            uint64 VICTIM_GUID;
            uint64 ALYSRAZOR_GUID;
            uint32 data;

            void Reset()
            {
                if(instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                data = 0;
            }

            void IsSummonedBy(Unit* pSummoner)
            {
                ALYSRAZOR_GUID = pSummoner->GetGUID();
            }

            void EnterCombat(Unit* /*target*/)
            {
                me->SetInCombatWithZone();
                me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS,10.0f);

                if(instance)
                     instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);

                GushingWoundTimer = 15000;
                clearImprintedTimer = 2000;
                losTimer = 5000;
            }

            void SpellHit(Unit* pCaster, const SpellEntry* spell)
            {
                if(pCaster == me)
                    return;

                if (spell->Id == 99390 || spell->Id == SPELL_IMPRINTED_TAUNT2) // Recursion check
                    return;

                if (spell->AppliesAuraType(SPELL_AURA_MOD_TAUNT)) // Dont try fuck up with me
                    Fixate();
            }

            void JustDied(Unit* /*Killer*/)
            {
                if(instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }

            void DamageTaken(Unit* attacker, uint32& damage)
            {
                if(damage >= me->GetHealth())
                {
                    ClearImprinted();
                }
            }

            void ClearImprinted(void)
            {
                Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                    if (Player* player = i->getSource())
                    {
                        if (me->HasAura(99390))
                            player->RemoveAura(99389);
                        else
                            player->RemoveAura(100359);
                    }
            }

            void Fixate(void)
            {
                ClearImprinted();
                me->RemoveAura(99390);
                me->RemoveAura(SPELL_IMPRINTED_TAUNT2);

                if (Unit* pVictim = SelectTarget(SELECT_TARGET_NEAREST, 0, 100.0f, true,-98619)) // Ignore players with wings
                {
                    if (Creature * alys = (Creature*)Unit::GetUnit(*me,ALYSRAZOR_GUID))
                    {
                        if(data == 0)
                        {
                            alys->AI()->DoAction(1);
                            data = alys->AI()->GetData(DATA_IMPRINTED);
                        }

                        if (data == 1)
                        {
                            me->AddAura(99389,pVictim); // Imprinted
                            pVictim->CastSpell(me,99390,true);
                        }
                        else
                        {
                            me->AddAura(100359,pVictim); // Imprinted
                            pVictim->CastSpell(me,SPELL_IMPRINTED_TAUNT2,true);
                        }
                    }
                    VICTIM_GUID = pVictim->GetGUID();
                }
            }

            bool VictimDiedOrInvalid(uint64 victimGUID)
            {
                if (Unit * player = Unit::GetUnit(*me,victimGUID))
                {
                    if(player->IsInWorld() == false)
                        return true;

                    if(player->isDead())
                        return true;

                    if (player->HasAura(98619)) // Wings of flame
                    {
                        player->RemoveAura(99389);
                        player->RemoveAura(100359);
                        return true;
                    }
                }
                else return true;

                return false;
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;


                if (losTimer <= diff) // Player can hide in LoS in meteor (GO) and hatchling can attack cause he is not in LoS
                {
                    if (Unit * vic = me->getVictim())
                    {
                        if (vic->IsInWorld() && !me->IsWithinLOSInMap(vic))
                            me->GetMotionMaster()->MovePoint(0, vic->GetPositionX(), vic->GetPositionY(), vic->GetPositionZ());
                    }
                    losTimer = 2000;
                }
                else losTimer -= diff;

                if (clearImprintedTimer <= diff)
                {
                    uint8 counter = 0;

                    Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                    for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                    if (Player* player = i->getSource())
                    {
                        if (player->HasAura(99389) || player->HasAura(100359))
                            counter++;

                        if( counter > 2)
                        {
                            player->RemoveAura(99389);
                            player->RemoveAura(100359);
                        }
                    }

                    clearImprintedTimer = 2000;
                }
                else clearImprintedTimer -= diff;

                bool hasSatiated = false;

                hasSatiated = me->HasAura(SPELL_SATIATED);
                hasSatiated = me->HasAura(SPELL_SATIATED);
                hasSatiated = me->HasAura(SPELL_SATIATED);
                hasSatiated = me->HasAura(SPELL_SATIATED);

                if (!hasSatiated && !me->GetAura(SPELL_HUNGRY))
                    me->CastSpell(me, SPELL_HUNGRY, false);

                if (VICTIM_GUID == 0 || VictimDiedOrInvalid(VICTIM_GUID))
                {
                    Fixate();
                    return;
                }

                if (GushingWoundTimer <= diff)
                {
                    me->CastSpell(me->getVictim(), SPELL_GUSHING_WOUND_Y10, false);
                    GushingWoundTimer = 60000;
                }
                else GushingWoundTimer -= diff;

                if (Creature* Worm = me->FindNearestCreature(NPC_PLUMP_LAVA_WORM,10.0f))
                    if (me->IsWithinDistInMap(Worm, 2))
                    {
                        me->RemoveAura(SPELL_HUNGRY);
                        me->RemoveAura(SPELL_TANTRUM);
                        me->RemoveAura(SPELL_SATIATED);
                        me->RemoveAura(100850);
                        me->RemoveAura(100851);
                        me->RemoveAura(100852);

                        me->AddAura(SPELL_SATIATED, me);
                        me->PlayOneShotAnimKit(ANIM_ATTACK_FEED);
                        Worm->setDeathState(JUST_DIED);
                        if (Creature* Target = me->FindNearestCreature(NPC_LAVA_WORM_TARGET, 20.0f))
                            Target->ForcedDespawn();
                        Worm->ForcedDespawn(2000);
                    }

                    DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_Voracious_HatchlingAI(creature);
        }
};

/**********************PLUMP_LAVA_WORM_AI*********************/
class npc_Plump_Lava_worm : public CreatureScript
{
    public:
        npc_Plump_Lava_worm() : CreatureScript("npc_Plump_Lava_worm") { }

        struct npc_Plump_Lava_wormAI : public ScriptedAI
        {
            npc_Plump_Lava_wormAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE|UNIT_FLAG_NOT_SELECTABLE);
                Emerge = false;
                Casting = false;
                Rotation = false;
            }

            bool Casting;
            bool Rotation;
            bool Emerge;

            float i;
            float O;

            uint32 RotationTimer;
            uint32 CastTimer;

            void UpdateAI(const uint32 diff)
            {
                if (!Emerge)
                {
                    CastTimer = 2000;
                    Emerge = true;
                    me->CastSpell(me, SPELL_LAVA_WORM_COSMETIC, false);
                    me->PlayOneShotAnimKit(ANIM_KIT_EMERGE);
                    switch(urand(1,2))
                    {
                    case 1:
                        O = -1.0f;
                        break;
                    case 2:
                        O = 1.0f;
                        break;
                    }
                }

                if (CastTimer <= diff && !Casting)
                {
                    if (Creature* Target = me->FindNearestCreature(NPC_LAVA_WORM_TARGET,50.0f))
                    {
                        uint32 t = urand(0,3);
                        i = t*M_PI/2;
                        Target->GetMotionMaster()->MovePoint(4, me->GetPositionX()+8*cos(i), me->GetPositionY()+8*sin(i), me->GetPositionZ());
                        Target->setFaction(35);
                        Target->SetSpeed(MOVE_RUN, 0.4f);
                        me->CastSpell(Target, SPELL_LAVA_SPEW, false);
                    }
                    Casting = true;
                    RotationTimer = 4000;
                }
                else CastTimer -= diff;

                if (Creature* Target = me->FindNearestCreature(NPC_LAVA_WORM_TARGET, 50.0f))
                {
                    me->SetUInt64Value(UNIT_FIELD_TARGET,Target->GetGUID());
                    me->SetFacingToObject(Target);
                }

                if (RotationTimer <= diff && Casting)
                {
                    i += (M_PI/15*O);
                    if (Creature* Target = me->FindNearestCreature(NPC_LAVA_WORM_TARGET, 50.0f))
                        Target->GetMotionMaster()->MovePoint(4, me->GetPositionX()+ 8*cos(i), me->GetPositionY()+8*sin(i), me->GetPositionZ());
                    RotationTimer = 500;
                }
                else RotationTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_Plump_Lava_wormAI(creature);
        }
};

/***********************FIERY_TORNADO_AI**********************/
class npc_Fiery_Tornado : public CreatureScript
{
    public:
       npc_Fiery_Tornado() : CreatureScript("npc_Fiery_Tornado") { }

        struct npc_Fiery_TornadoAI : public ScriptedAI
        {
            npc_Fiery_TornadoAI(Creature* creature) : ScriptedAI(creature)
            {
                Initialize = false;
                Direction = 0;
                Slot = 0;
                Duration = false;
                sx = me->GetPositionX();
                sy = me->GetPositionY();
                me->RemoveAllAuras();
                me->SetSpeed(MOVE_RUN, 5.0f);
            }

            float Distance;
            float i;
            float Direction;
            float sx;
            float sy;

            bool Initialize;
            bool Duration;

            uint32 Slot;
            uint32 MoveTimer;

            void SetData(uint32 Type, uint32 Data)
            {
                if (Type == DATA_TORNADO_SLOT)
                {
                    Slot = Data;
                    Initialize = true;
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!Initialize)
                    return;

                if (Initialize && Direction == 0)
                {
                    MoveTimer = 3000;
                    switch (Slot)
                        {
                        case 1:
                            Distance = 24.0f;
                            i = 0;
                            Direction = 1.0f;
                            break;
                        case 2:
                            Distance = 24.0f;
                            i = M_PI;
                            Direction = 1.0f;
                            break;
                        case 3:
                            Distance = 33.0f;
                            i = (2*M_PI)/3;
                            Direction = -1.0f;
                            break;
                        case 4:
                            Distance = 33.0f;
                            i = 2*((2*M_PI)/3);
                            Direction = -1.0f;
                            break;
                        case 5:
                            Distance = 33.0f;
                            i = 0.0f;
                            Direction = -1.0f;
                            break;
                        case 6:
                            Distance = 42.0f;
                            i = 0.0f;
                            Direction = 1.0f;
                            break;
                        case 7:
                            Distance = 42.0f;
                            i = M_PI/2;
                            Direction = 1.0f;
                            break;
                        case 8:
                            Distance = 42.0f;
                            i = M_PI;
                            Direction = 1.0f;
                            break;
                        case 9:
                            Distance = 42.0f;
                            i = 3*(M_PI/2);
                            Direction = 1.0f;
                            break;
                        case 10:
                            Distance = 51.0f;
                            i = 1.0f;
                            Direction = -1.0f;
                            break;
                        case 11:
                            Distance = 51.0f;
                            i = M_PI/2 + 1.0f;
                            Direction = -1.0f;
                            break;
                        case 12:
                            Distance = 51.0f;
                            i = M_PI + 1.0f;
                            Direction = -1.0f;
                            break;
                        case 13:
                            Distance = 51.0f;
                            i = 3*(M_PI/2) + 1.0f;
                            Direction = -1.0f;
                            break;
                        case 14:
                            Distance = 60.0f;
                            i = 0.0f;
                            Direction = 1.0f;
                            break;
                        case 15:
                            Distance = 60.0f;
                            i = M_PI/2;
                            Direction = 1.0f;
                            break;
                        case 16:
                            Distance = 60.0f;
                            i = 3*(M_PI/2);
                            Direction = 1.0f;
                            break;
                        case 17:
                            Distance = 60.0f;
                            i = M_PI;
                            Direction = 1.0f;
                            break;
                        }
                    me->RemoveAllAuras();
                    me->CastSpell(me, SPELL_FIERY_TORNADO, false);
                    me->GetMotionMaster()->MovePoint(0, sx +(Distance*cos(i)*Direction), sy + Distance*sin(i), me->GetPositionZ());
                }

                if (MoveTimer <= diff)
                {
                    if (!Duration)
                        if (Aura* Tornado = me->GetAura(SPELL_FIERY_TORNADO))
                        {
                            Tornado->SetDuration(25000);
                            Duration = true;
                        }

                    switch(uint32(Distance))
                    {
                        case 24:
                            MoveTimer = 200;
                            break;
                        case 33:
                            MoveTimer = 250;
                            break;
                        case 42:
                            MoveTimer = 350;
                            break;
                        case 51:
                            MoveTimer = 450;
                            break;
                        case 60:
                            MoveTimer = 500;
                            break;
                    }
                    me->SetSpeed(MOVE_RUN, 2.0f);
                    i = i + (2*M_PI/50);
                    me->GetMotionMaster()->MovePoint(0, sx +(Distance*cos(i)*Direction), sy + Distance*sin(i), me->GetPositionZ());
                }
                else MoveTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_Fiery_TornadoAI(creature);
        }
};

/************************SPELL_SCRIPTS************************/
class npc_Brushfire : public CreatureScript
{
    public:
        npc_Brushfire() : CreatureScript("npc_Brushfire") { }

        struct npc_BrushfireAI : public ScriptedAI
        {
            npc_BrushfireAI(Creature* creature) : ScriptedAI(creature)
            {
                i = M_PI/4;
                uint32 Times = urand(0,7);
                i *= Times;
                me->SetSpeed(MOVE_RUN, 0.6f);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->CastSpell(me,SPELL_BRUSHFIRE_PERIODIC,true);
                if (Aura* BurshFire = me->GetAura(SPELL_BRUSHFIRE_PERIODIC))
                    BurshFire->SetDuration(60000);
            }

            float i;

            void EnterCombat(Unit* /*who*/) { }
            void AttackStart(Unit* /*who*/) { }
            void MoveInLineOfSight(Unit* /*who*/) { }

            void UpdateAI(const uint32 diff)
            {
                me->GetMotionMaster()->MovePoint(0, me->GetPositionX() + 200*cos(i), me->GetPositionY() + 200*sin(i), me->GetPositionZ());
                if (me->GetDistance(-49.34f, -277.97f, 70.0f) > 70.0f)
                    me->ForcedDespawn();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_BrushfireAI(creature);
        }
};

class npc_Flying_Spells : public CreatureScript
{
    public:
        npc_Flying_Spells() : CreatureScript("npc_Flying_Spells") { }

        struct npc_Flying_SpellsAI : public ScriptedAI
        {
            npc_Flying_SpellsAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetFlying(true);
                me->SetLevel(88);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                me->SetReactState(REACT_AGGRESSIVE);
                me->SetInCombatWithZone();

                if (me->GetEntry() == NPC_INCENDIARY_CLOUD)
                {
                    me->CastSpell(me, SPELL_INCENDIARY_CLOUD, false);
                    if (Aura* Incendiary = me->GetAura(SPELL_INCENDIARY_CLOUD))
                        Incendiary->SetDuration(4000);
                 }
                if (me->GetEntry() == NPC_BLAZING_POWER)
                {
                    me->CastSpell(me, SPELL_BLAZING_POWER, false);
                    if (Aura* Incendiary = me->GetAura(SPELL_BLAZING_POWER))
                        Incendiary->SetDuration(4000);
                 }
            }

        void MoveInLineOfSight(Unit * who) 
        {
            if (who && who->ToPlayer() && who->GetDistance(me) <= 12 && me->GetEntry() == NPC_BLAZING_POWER)
            {
                if (Aura * aWings = who->GetAura(98619)) // Wings of flame
                {
                    if (aWings->GetDuration() <= 28000)
                        aWings->RefreshDuration();
                }

                if (Aura * aBlaze = who->GetAura(SPELL_BLAZING_POWER_EFFECT)) // Blazing power
                {
                    if (aBlaze->GetDuration() <= 38000)
                    {
                        who->AddAura(SPELL_BLAZING_POWER_EFFECT,who);
                    }
                }
                else
                    who->AddAura(SPELL_BLAZING_POWER_EFFECT,who);

            }
        }

        void UpdateAI(const uint32 diff){ }

        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_Flying_SpellsAI(creature);
        }
};

class npc_Volcanic_Fire : public CreatureScript
{
    public:
        npc_Volcanic_Fire() : CreatureScript("npc_Volcanic_Fire") { }

        struct npc_Volcanic_FireAI : public ScriptedAI
        {
            npc_Volcanic_FireAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_SELECTABLE);
                cast = true;
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;

            bool cast;

            uint32 Timer;

            void SpellHit(Unit* /*pCaster*/, const SpellEntry* spell)
            {
                if (spell->Id == SPELL_ERRUPTION)
                    me->AddAura(SPELL_VOLCANIC_FIRE, me);
            }

            void UpdateAI(const uint32 diff)
            {
                if (instance == NULL)
                    return;

                if (instance->GetData(TYPE_ALYSRAZOR) != IN_PROGRESS)
                    me->setFaction(35);
                else
                    me->setFaction(14);

                if (instance->GetData(TYPE_ALYSRAZOR) == DONE )
                {
                    me->RemoveAllAuras();
                    me->CombatStop(true);
                    me->SetReactState(REACT_PASSIVE);
                }

                if (instance->GetData(TYPE_ALYSRAZOR) == IN_PROGRESS)
                {
                    if (me->GetAura(SPELL_VOLCANIC_FIRE))
                        return;

                    if (cast)
                    {
                        Timer = 500;
                        cast = false;
                    }

                    if (!cast && Timer <= diff)
                    {
                        if (me->FindNearestCreature(NPC_ALYSRAZOR, 300.0f))
                            me->CastSpell(me, SPELL_ERRUPTION, false);
                        cast = true;
                        me->AddAura(SPELL_VOLCANIC_FIRE, me);
                        Timer = 1000000;
                    }
                    else Timer -= diff;
                }
                else if (Aura* Fire = me->GetAura(SPELL_VOLCANIC_FIRE))
                {
                    me->RemoveAura(Fire);
                    cast = true;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_Volcanic_FireAI(creature);
        }
};

class spell_ignition : public SpellScriptLoader
{
public:
    spell_ignition() : SpellScriptLoader("spell_ignition") { }

    class spell_ignition_AuraScript : public AuraScript //Ignition
    {
        PrepareAuraScript(spell_ignition_AuraScript);

        bool Validate(SpellEntry const* /*spellInfo*/)
        {
            return true;
        }

        void OnPeriodic(AuraEffect const* /*aurEff*/)
        {
            if (GetTarget()->GetEntry() == NPC_ALYSRAZOR)
                if (GetTarget()->GetPower(POWER_MANA) < 48)
                    GetTarget()->GetAI()->SetData(DATA_STAGE_THREE_TIMER, 666);
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_ignition_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_OBS_MOD_POWER);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_ignition_AuraScript();
    }
};

class spell_Gushing_Wound : public SpellScriptLoader
{
public:
    spell_Gushing_Wound() : SpellScriptLoader("spell_Gushing_Wound") { }

    class spell_Gushing_Wound_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_Gushing_Wound_AuraScript);

        void OnPeriodic(AuraEffect const* /*aurEff*/)
        {
            if (GetTarget()->GetHealthPct() < 50.0f) //remove when below 50%HP
            {
                Aura* Gushing = GetTarget()->GetAura(SPELL_GUSHING_WOUND_Y10);
                if(!Gushing)
                    Gushing = GetTarget()->GetAura(100718);
                if (!Gushing)
                    Gushing = GetTarget()->GetAura(100719);
                if (!Gushing)
                    Gushing = GetTarget()->GetAura(100720);

                if (Gushing)
                    GetTarget()->RemoveAura(Gushing);
            }

        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_Gushing_Wound_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_Gushing_Wound_AuraScript();
    }
};

class spell_Fieroblast_buff : public SpellScriptLoader
{
    public:
        spell_Fieroblast_buff() : SpellScriptLoader("spell_Fieroblast_buff") { }
        class spell_Fieroblast_buff_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_Fieroblast_buff_SpellScript);

            void HandleExtraEffect()
            {
                GetCaster()->CastSpell(GetCaster(), SPELL_FIRE_IT_UP, true);
            }

            void Register()
            {
               AfterHit += SpellHitFn(spell_Fieroblast_buff_SpellScript::HandleExtraEffect);
            }
        };

        SpellScript *GetSpellScript() const
        {
            return new spell_Fieroblast_buff_SpellScript();
        }
};

class spell_cataclysm : public SpellScriptLoader
{
    public:
        spell_cataclysm() : SpellScriptLoader("spell_cataclysm") { }
        class spell_cataclysm_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_cataclysm_SpellScript);

            void HandleExtraEffect()
            {
                if( Unit * pAlys = GetCaster()->FindNearestCreature(NPC_ALYSRAZOR,500.0f,true))
                if (Creature* Meteor = pAlys->SummonCreature(NPC_MOLTEN_METEOR, GetCaster()->GetPositionX(), GetCaster()->GetPositionY(), GetCaster()->GetPositionZ() + 40.0f, 0, TEMPSUMMON_MANUAL_DESPAWN))
                    Meteor->CastSpell(Meteor, SPELL_MOLTEN_METEOR, false);
            }

            void Register()
            {
               AfterHit += SpellHitFn(spell_cataclysm_SpellScript::HandleExtraEffect);
            }
        };

        SpellScript *GetSpellScript() const
        {
            return new spell_cataclysm_SpellScript();
        }
};

class spell_Molten_Feather : public SpellScriptLoader
{
    public:
        spell_Molten_Feather() : SpellScriptLoader("spell_Molten_Feather") { }

        class spell_Molten_Feather_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_Molten_Feather_AuraScript);

            void AddBuff(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Aura* MoltenFeather = GetTarget()->GetAura(SPELL_MOLTEN_FEATHER))
                {
                   if (MoltenFeather->GetStackAmount() == 3)
                       if (!GetTarget()->GetAura(SPELL_WINGS_OF_FLAME_FLY))
                       {
                            GetTarget()->CastSpell(GetTarget(), SPELL_WINGS_OF_FLAME, false);
                            GetTarget()->AddAura(SPELL_WINGS_OF_FLAME_FLY,GetTarget());
                       }
                }
            }

            void SetMovingCasting(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
                {
                switch (GetTarget()->getClass())
                    {
                    case CLASS_WARRIOR:
                        if (!GetTarget()->GetAura(MOLTEN_FEATHER_WARRIOR))
                            GetTarget()->AddAura(MOLTEN_FEATHER_WARRIOR, GetTarget());
                        break;
                    case CLASS_PALADIN:
                        if (!GetTarget()->GetAura(MOLTEN_FEATHER_PALADIN))
                            GetTarget()->AddAura(MOLTEN_FEATHER_PALADIN, GetTarget());
                        break;
                    case CLASS_HUNTER:
                        if (!GetTarget()->GetAura(MOLTEN_FEATHER_HUNTER))
                            GetTarget()->AddAura(MOLTEN_FEATHER_HUNTER, GetTarget());
                        break;
                    case CLASS_ROGUE:
                        if (!GetTarget()->GetAura(MOLTEN_FEATHER_ROGUE))
                            GetTarget()->AddAura(MOLTEN_FEATHER_ROGUE, GetTarget());
                        break;
                    case CLASS_PRIEST:
                        if (!GetTarget()->GetAura(MOLTEN_FEATHER_PRIEST))
                            GetTarget()->AddAura(MOLTEN_FEATHER_PRIEST, GetTarget());
                        break;
                    case CLASS_DEATH_KNIGHT:
                        if (!GetTarget()->GetAura(MOLTEN_FEATHER_DK))
                            GetTarget()->AddAura(MOLTEN_FEATHER_DK, GetTarget());
                        break;
                    case CLASS_SHAMAN:
                        if (!GetTarget()->GetAura(MOLTEN_FEATHER_SHAMAN))
                            GetTarget()->AddAura(MOLTEN_FEATHER_SHAMAN, GetTarget());
                        break;
                    case CLASS_MAGE:
                        if (!GetTarget()->GetAura(MOLTEN_FEATHER_MAGE))
                            GetTarget()->AddAura(MOLTEN_FEATHER_MAGE, GetTarget());
                        break;
                    case CLASS_WARLOCK:
                        if (!GetTarget()->GetAura(MOLTEN_FEATHER_WARLOCK))
                            GetTarget()->AddAura(MOLTEN_FEATHER_WARLOCK, GetTarget());
                        break;
                    case CLASS_DRUID:
                        if (!GetTarget()->GetAura(MOLTEN_FEATHER_DRUID))
                            GetTarget()->AddAura(MOLTEN_FEATHER_DRUID, GetTarget());
                        break;
                    default:
                        break;
                    }
                }

            void RemoveBuff(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (aurEff->GetCaster())
                if (aurEff->GetCaster()->GetPower(POWER_SCRIPTED))
                    aurEff->GetCaster()->SetPower(POWER_SCRIPTED, 0);

                switch (GetTarget()->getClass())
                   {
                    case CLASS_WARRIOR:
                        if (GetTarget()->GetAura(MOLTEN_FEATHER_WARRIOR))
                            GetTarget()->RemoveAura(MOLTEN_FEATHER_WARRIOR);
                        break;
                    case CLASS_PALADIN:
                        if (GetTarget()->GetAura(MOLTEN_FEATHER_PALADIN))
                            GetTarget()->RemoveAura(MOLTEN_FEATHER_PALADIN);
                        break;
                    case CLASS_HUNTER:
                        if (GetTarget()->GetAura(MOLTEN_FEATHER_HUNTER))
                            GetTarget()->RemoveAura(MOLTEN_FEATHER_HUNTER);
                        break;
                    case CLASS_ROGUE:
                        if (GetTarget()->GetAura(MOLTEN_FEATHER_ROGUE))
                            GetTarget()->RemoveAura(MOLTEN_FEATHER_ROGUE);
                        break;
                    case CLASS_PRIEST:
                        if (GetTarget()->GetAura(MOLTEN_FEATHER_PRIEST))
                            GetTarget()->RemoveAura(MOLTEN_FEATHER_PRIEST);
                        break;
                    case CLASS_DEATH_KNIGHT:
                        if (!GetTarget()->GetAura(MOLTEN_FEATHER_DK))
                            GetTarget()->RemoveAura(MOLTEN_FEATHER_DK);
                        break;
                    case CLASS_SHAMAN:
                        if (GetTarget()->GetAura(MOLTEN_FEATHER_SHAMAN))
                            GetTarget()->RemoveAura(MOLTEN_FEATHER_SHAMAN);
                        break;
                    case CLASS_MAGE:
                        if (GetTarget()->GetAura(MOLTEN_FEATHER_MAGE))
                            GetTarget()->RemoveAura(MOLTEN_FEATHER_MAGE);
                        break;
                    case CLASS_WARLOCK:
                        if (GetTarget()->GetAura(MOLTEN_FEATHER_WARLOCK))
                            GetTarget()->RemoveAura(MOLTEN_FEATHER_WARLOCK);
                        break;
                    case CLASS_DRUID:
                        if (GetTarget()->GetAura(MOLTEN_FEATHER_DRUID))
                            GetTarget()->RemoveAura(MOLTEN_FEATHER_DRUID);
                    default:
                        break;
                    }
            }

            void Register()
            {
               OnEffectApply += AuraEffectApplyFn(spell_Molten_Feather_AuraScript::SetMovingCasting, EFFECT_1, SPELL_AURA_MOD_INCREASE_SPEED, AURA_EFFECT_HANDLE_REAL);
               OnEffectApply += AuraEffectApplyFn(spell_Molten_Feather_AuraScript::AddBuff, EFFECT_1, SPELL_AURA_MOD_INCREASE_SPEED, AURA_EFFECT_HANDLE_REAL);
               OnEffectRemove += AuraEffectRemoveFn(spell_Molten_Feather_AuraScript::RemoveBuff, EFFECT_1, SPELL_AURA_MOD_INCREASE_SPEED, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_Molten_Feather_AuraScript();
        }
};

class spell_Blazing_Power : public SpellScriptLoader
{
    public:
        spell_Blazing_Power() : SpellScriptLoader("spell_Blazing_Power") { }

        class spell_Blazing_Power_AuraScript : public AuraScript
            {
                PrepareAuraScript(spell_Blazing_Power_AuraScript);

                void AddBuff(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
                {
                    if (Aura* Blazing = GetTarget()->GetAura(SPELL_BLAZING_POWER_EFFECT))
                    {
                        if (Blazing->GetStackAmount() >= 24)
                        {
                            if (Aura* Power = GetTarget()->GetAura(SPELL_ALYSRAS_RAZOR))
                                Power->SetDuration(40000);
                            else
                                GetTarget()->AddAura(SPELL_ALYSRAS_RAZOR, GetTarget());
                        }
                    }
                }

                 void Register()
                {
                    OnEffectApply += AuraEffectApplyFn(spell_Blazing_Power_AuraScript::AddBuff, EFFECT_0, SPELL_AURA_MELEE_SLOW, AURA_EFFECT_HANDLE_REAL);
                }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_Blazing_Power_AuraScript();
        }
};

class spell_gen_harsh_winds : public SpellScriptLoader
{
    public:
        spell_gen_harsh_winds() : SpellScriptLoader("spell_gen_harsh_winds") { }

        class spell_gen_harsh_winds_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_harsh_winds_SpellScript);

            void RemoveInvalidTargets(std::list<Unit*>& unitList)
            {
                if (!GetCaster())
                    return;

                for (std::list<Unit*>::iterator itr = unitList.begin() ; itr != unitList.end();)
                {
                    if ((*itr) && (*itr)->GetDistance(GetCaster()) < 60.0f)
                        itr = unitList.erase(itr);
                    else
                        ++itr;
                }
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_harsh_winds_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_AREA_ENTRY_SRC); // 7
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_harsh_winds_SpellScript();
        }
};

class spell_gen_firestorm_hc : public SpellScriptLoader
{
    public:
        spell_gen_firestorm_hc() : SpellScriptLoader("spell_gen_firestorm_hc") { }

        class spell_gen_firestorm_hc_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_firestorm_hc_SpellScript);

            void RemoveInvalidTargets(std::list<Unit*>& unitList)
            {
                if (Unit * caster = GetCaster())
                {
                    for (std::list<Unit*>::iterator itr = unitList.begin() ; itr != unitList.end();)
                    {
                        if (!(*itr) || !((*itr)->IsWithinLOS(caster->GetPositionX(),caster->GetPositionY(),caster->GetPositionZ())))
                            itr = unitList.erase(itr);
                        else
                            ++itr;
                    }
                }
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_firestorm_hc_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_AREA_ENTRY_SRC);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_firestorm_hc_SpellScript();
        }
};

class achievement_do_a_barrel_rollBF : public AchievementCriteriaScript
{
    public:
        achievement_do_a_barrel_rollBF() : AchievementCriteriaScript("achievement_do_a_barrel_rollBF") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            if (!target)
                return false;

            return target->GetAI()->GetData(DATA_ACHIEVEMENTBF) == 0;
        }
};

class achievement_do_a_barrel_rollIC : public AchievementCriteriaScript
{
    public:
        achievement_do_a_barrel_rollIC() : AchievementCriteriaScript("achievement_do_a_barrel_rollIC") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            if (!target)
                return false;

            return target->GetAI()->GetData(DATA_ACHIEVEMENTIC) == 0;
        }
};

class achievement_do_a_barrel_rollLS : public AchievementCriteriaScript
{
    public:
        achievement_do_a_barrel_rollLS() : AchievementCriteriaScript("achievement_do_a_barrel_rollLS") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            if (!target)
                return false;

            return target->GetAI()->GetData(DATA_ACHIEVEMENTLS) == 0;
        }
};

class achievement_do_a_barrel_rollFT : public AchievementCriteriaScript
{
    public:
        achievement_do_a_barrel_rollFT() : AchievementCriteriaScript("achievement_do_a_barrel_rollFT") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            if (!target)
                return false;

            return target->GetAI()->GetData(DATA_ACHIEVEMENTFT) == 0;
        }
};

void AddSC_boss_alysrazor()
{
    new boss_Alysrazor();
    new npc_Plump_Lava_worm();
    new npc_Voracious_Hatchling();
    new npc_Blazing_Talon_Initiate();
    new npc_Molten_Feather();
    new npc_Blazing_Broodmother();
    new npc_Volcanic_Fire();
    new npc_Blazing_Talon_Clawshaper();
    new npc_Brushfire();
    new npc_Fiery_Tornado();
    new npc_Molten_Egg();
    new npc_Fendral();
    new npc_Captive_Duid();
    new npc_Flying_Spells();
    new npc_molten_meteor();
    new npc_bouder();
    new npc_fiery_vortex();
    new spell_Blazing_Power();
    new spell_Molten_Feather();
    new spell_Fieroblast_buff();
    new spell_Gushing_Wound();
    new spell_ignition();
    new spell_cataclysm(); // 102111
    new spell_gen_harsh_winds();
    new spell_gen_firestorm_hc(); // 100745,101664,101665,101666
    new achievement_do_a_barrel_rollBF();
    new achievement_do_a_barrel_rollIC();
    new achievement_do_a_barrel_rollLS();
    new achievement_do_a_barrel_rollFT();
}

/* HEROIC SQLs
    
    DELETE FROM `spell_script_names` WHERE  `spell_id`=102111 AND `ScriptName`='spell_cataclysm' LIMIT 1;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (102111, 'spell_cataclysm');

    DELETE FROM `spell_script_names` WHERE  spell_id=100745 OR  spell_id=101664 OR  spell_id=101665 OR  spell_id=101666;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
    VALUES (100745, 'spell_gen_firestorm_hc'),
    (101664, 'spell_gen_firestorm_hc'),
    (101665, 'spell_gen_firestorm_hc'),
    (101666, 'spell_gen_firestorm_hc');

    REPLACE pre 
    select * from creature_template as ct where entry in (53496,53497,53498,54563)
*/

/*
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (52528, 0, 0, 0, 0, 0, 37935, 0, 0, 0, 'Egg Satchel', '', '', 0, 1, 1, 0, 35, 35, 0, 1, 1.14286, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 16778240, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'npc_Egg_Satchel', 15595);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (52530, 54044, 54045, 54046, 0, 0, 38446, 0, 0, 0, 'Alysrazor', '', '', 0, 88, 88, 3, 14, 14, 0, 2, 3.2, 1, 3, 40000, 43000, 0, 308, 1, 2000, 2000, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 108, 666, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3000000, 3500000, '', 0, 4, 450, 0.0199925, 1, 0, 0, 0, 0, 0, 0, 0, 187, 1, 0, 646922239, 1, 'boss_Alysrazor', 15595);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53089, 0, 0, 0, 0, 0, 38146, 0, 0, 0, 'Molten Feather', '', 'Interact', 0, 1, 1, 0, 2028, 2028, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 2000, 2000, 1, 33280, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 1096, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'npc_Molten_Feather', 15595);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53158, 0, 0, 0, 0, 0, 0, 11686, 0, 0, 'Volcano Fire Bunny', '', '', 0, 85, 85, 3, 14, 14, 0, 1.14286, 1, 1, 0, 0, 0, 0, 0, 1, 2000, 2000, 2, 33554432, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 1024, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'npc_Volcanic_Fire', 15595);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53372, 0, 0, 0, 0, 0, 0, 11686, 0, 0, 'Brushfire', '', '', 0, 88, 88, 0, 14, 14, 0, 1, 1.14286, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 1096, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'npc_Brushfire', 15595);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53375, 0, 0, 0, 0, 0, 38652, 0, 0, 0, 'Herald of the Burning End', '', '', 0, 87, 87, 3, 14, 14, 0, 1.14286, 1, 1, 0, 30000, 33000, 0, 308, 1, 2000, 2000, 1, 256, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 72, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'NullAI', 0, 3, 50, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, '', 15595);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53498, 0, 0, 0, 0, 0, 20324, 11686, 0, 0, 'Molten Boulder', '', '', 0, 1, 1, 0, 35, 35, 0, 1, 1.14286, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 188, 1, 0, 0, 0, '', 15595);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53520, 0, 0, 0, 0, 0, 37993, 0, 0, 0, 'Plump Lava Worm', '', '', 0, 87, 87, 0, 14, 14, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 2000, 2000, 1, 33554432, 0, 42, 0, 0, 0, 0, 0, 0, 0, 1, 72, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 50, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'npc_Plump_Lava_worm', 15595);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53521, 0, 0, 0, 0, 0, 0, 11686, 0, 0, 'Lava Worm Target', '', '', 0, 1, 1, 0, 35, 35, 0, 1, 1.14286, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 1024, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, '', 15595);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53541, 0, 0, 0, 0, 0, 0, 11686, 0, 0, 'Incindiary Cloud', '', '', 0, 85, 85, 3, 14, 14, 0, 1, 1.14286, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 1096, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 4, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'npc_Flying_Spells', 15595);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53554, 0, 0, 0, 0, 0, 0, 11686, 0, 0, 'Blazing Power', '', '', 0, 1, 1, 0, 35, 35, 0, 1, 1.14286, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 1096, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 4, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'npc_Flying_Spells', 15595);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53680, 0, 0, 0, 0, 0, 38443, 0, 0, 0, 'Blazing Broodmother', '', '', 0, 87, 87, 0, 14, 14, 0, 1.5873, 1.44444, 1, 1, 0, 0, 0, 0, 1, 2000, 2000, 1, 33587456, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 72, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1679, 0, 0, '', 0, 3, 40, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'npc_Blazing_Broodmother', 15595);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53681, 0, 0, 0, 0, 0, 38445, 0, 0, 0, 'Molten Egg', '', '', 0, 87, 87, 0, 16, 16, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 2000, 2000, 1, 33024, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 72, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 400, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'npc_Molten_Egg', 15595);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53693, 0, 0, 0, 0, 0, 0, 11686, 0, 0, 'Fiery Vortex', '', '', 0, 88, 88, 0, 14, 14, 0, 1.14286, 1, 1, 0, 0, 0, 0, 0, 1, 2000, 2000, 1, 33554432, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 1096, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'npc_fiery_vortex', 15595);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53698, 0, 0, 0, 0, 0, 0, 11686, 0, 0, 'Fiery Tornado', '', '', 0, 88, 88, 0, 14, 14, 0, 1.14286, 1, 1, 0, 0, 0, 0, 0, 1, 2000, 2000, 1, 33554432, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 1096, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'npc_Fiery_Tornado', 15595);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53734, 54055, 0, 0, 0, 0, 38013, 0, 0, 0, 'Blazing Talon Clawshaper', '', '', 0, 86, 86, 3, 14, 14, 0, 1.5873, 1.44444, 1, 1, 30000, 33000, 0, 308, 1, 2000, 2000, 1, 256, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 72, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'NullAI', 0, 3, 100, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'npc_Blazing_Talon_Clawshaper', 15595);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53896, 54063, 54064, 0, 0, 0, 38317, 0, 0, 0, 'Blazing Talon Initiate', '', '', 0, 86, 86, 3, 14, 14, 0, 1.5873, 1.44444, 1, 1, 30000, 33000, 0, 308, 1, 2000, 2000, 1, 256, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 72, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'NullAI', 0, 5, 4, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'npc_Blazing_Talon_Initiate', 15595);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53898, 0, 0, 0, 0, 0, 38372, 0, 0, 0, 'Voracious Hatchling', '', '', 0, 87, 87, 3, 14, 14, 0, 1, 1.14286, 1, 1, 30000, 33000, 0, 308, 1, 2000, 2000, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 72, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'NullAI', 0, 3, 168, 1, 1, 0, 0, 0, 0, 0, 0, 0, 259, 1, 0, 0, 0, 'npc_Voracious_Hatchling', 15595);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53900, 0, 0, 0, 0, 0, 38443, 0, 0, 0, 'Blazing Broodmother', '', '', 0, 87, 87, 0, 14, 14, 0, 1.5873, 1.44444, 1, 1, 0, 0, 0, 0, 1, 2000, 2000, 1, 33587456, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 72, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1697, 0, 0, '', 0, 3, 40, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'npc_Blazing_Broodmother', 15595);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53986, 0, 0, 0, 0, 0, 0, 11686, 0, 0, 'Firestorm', '', '', 0, 85, 85, 0, 14, 14, 0, 1.14286, 1, 1, 0, 0, 0, 0, 0, 1, 2000, 2000, 1, 33554432, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 1048576, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, '', 15595);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (54015, 54016, 54017, 0, 0, 0, 37953, 0, 0, 0, 'Majordomo Staghelm', 'Archdruid of the Flame', '', 0, 88, 88, 3, 14, 14, 0, 1, 1, 1, 1, 30000, 33000, 0, 308, 1, 2000, 2000, 4, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 108, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'NullAI', 0, 4, 330, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'npc_Fendral', 15595);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (54019, 0, 0, 0, 0, 0, 38603, 38604, 38605, 38606, 'Captive Druid of the Talon', '', '', 0, 85, 85, 3, 35, 35, 0, 1.14286, 1, 1, 1, 0, 0, 0, 0, 1, 2000, 2000, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 5, 10, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'npc_Captive_Duid', 15595);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (54563, 0, 0, 0, 0, 0, 20324, 38493, 0, 0, 'Molten Meteor', '', '', 0, 87, 87, 3, 14, 14, 0, 1, 1.14286, 1, 1, 30000, 33000, 0, 308, 1, 2000, 2000, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 72, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'EventAI', 0, 3, 1.7, 1, 1, 0, 0, 0, 0, 0, 0, 0, 188, 1, 0, 0, 0, '', 15595);

DELETE FROM `spell_script_names` WHERE  `spell_id`=100640 AND `ScriptName`='spell_gen_harsh_winds' LIMIT 1;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (100640, 'spell_gen_harsh_winds');

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_Fieroblast_buff';
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_Gushing_Wound';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(101294, 'spell_Fieroblast_buff'),
(101295, 'spell_Fieroblast_buff'),
(101296, 'spell_Fieroblast_buff'),
(100718, 'spell_Gushing_Wound'),
(100719, 'spell_Gushing_Wound'),
(100720, 'spell_Gushing_Wound');

DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_Blazing_Power';
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_Molten_Feather';
DELETE FROM `spell_script_names` WHERE `ScriptName`='spell_ignition';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(99461, 'spell_Blazing_Power'),
(97128, 'spell_Molten_Feather'),
(101223, 'spell_Fieroblast_buff'),
(99308, 'spell_Gushing_Wound'),
(99919, 'spell_ignition');


REPLACE INTO `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`,`position_y`,`position_z` , `orientation`,`spawntimesecs`,`spawndist`,`currentwaypoint`, `curhealth`,`curmana`,`MovementType`,`npcflag`,`unit_flags`,`dynamicflags`) VALUES
(220492,53158,720,15,1,0,0,-44.2135,-204.372,65.878,5.11381,7200,0,0,1,0,0,0,0,0),
(220493,53158,720,15,1,0,0,-24.3542,-191.929,77.8011,4.4855,7200,0,0,1,0,0,0,0,0),
(220494,53158,720,15,1,0,0,47.3455,-240.977,78.1981,3.33358,7200,0,0,1,0,0,0,0,0),
(220495,53158,720,15,1,0,0,39.5035,-211.217,77.8746,3.78736,7200,0,0,1,0,0,0,0,0),
(220496,53158,720,15,1,0,0,0.682292,-192.276,88.7189,4.24115,7200,0,0,1,0,0,0,0,0),
(220506,53158,720,15,1,0,0,50.8212,-269.667,72.2139,2.89725,7200,0,0,1,0,0,0,0,0),
(220507,53158,720,15,1,0,0,-104.804,-263.189,63.9482,5.55015,7200,0,0,1,0,0,0,0,0),
(220508,53158,720,15,1,0,0,-108.642,-269.795,56.2283,5.51524,7200,0,0,1,0,0,0,0,0),
(220509,53158,720,15,1,0,0,-107.259,-279.651,55.9602,6.26573,7200,0,0,1,0,0,0,0,0),
(220510,53158,720,15,1,0,0,28.1319,-298.859,51.843,2.77507,7200,0,0,1,0,0,0,0,0),
(220511,53158,720,15,1,0,0,-107.635,-288.804,56.1059,0.122173,7200,0,0,1,0,0,0,0,0),
(220512,53158,720,15,1,0,0,24.9931,-308.339,51.0899,2.70526,7200,0,0,1,0,0,0,0,0),
(220513,53158,720,15,1,0,0,-108.293,-298.653,56.1342,0.331613,7200,0,0,1,0,0,0,0,0),
(220514,53158,720,15,1,0,0,20.1319,-318.575,50.9641,2.49582,7200,0,0,1,0,0,0,0,0),
(220515,53158,720,15,1,0,0,14.1562,-328.361,51.0624,2.44346,7200,0,0,1,0,0,0,0,0),
(220516,53158,720,15,1,0,0,6.62326,-335.351,51.5699,2.18166,7200,0,0,1,0,0,0,0,0),
(220522,53158,720,15,1,0,0,-65.6997,-351.76,62.2697,1.27409,7200,0,0,1,0,0,0,0,0),
(220530,53158,720,15,1,0,0,-11.6198,-375.441,63.4142,1.72788,7200,0,0,1,0,0,0,0,0),
(220531,53158,720,15,1,0,0,-53.3646,-372.29,78.901,1.29154,7200,0,0,1,0,0,0,0,0),
(220532,53158,720,15,1,0,0,-30.8837,-386.681,77.1706,1.53589,7200,0,0,1,0,0,0,0,0);

REPLACE INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `ConditionTypeOrReference`, ConditionValue1, `ConditionValue2`, `Comment`) VALUES 
(13, 3, 99335, 31, 3, 53521, 'Spell Lava Spew ScriptTarget'),
(13, 3, 99919, 31, 3, 52530, 'Spell Cyclone Winds ScriptTarget'),
(13, 3, 100558, 31, 3, 54019, 'Spell Sacrifice ScriptTarget'),
(13, 3, 100564, 31, 3, 53521, 'Spell Cosmetic Death ScriptTarget'),
(13, 3, 100744, 31, 3, 53986, 'Spell Firestorm ScriptTarget');

REPLACE INTO `achievement_criteria_data` (`criteria_id`, `type`, `ScriptName`) VALUES (17533, 11, 'achievement_do_a_barrel_rollBF'),
(17538, 11, 'achievement_do_a_barrel_rollFT'),
(17536, 11, 'achievement_do_a_barrel_rollIC'),
(17535, 11, 'achievement_do_a_barrel_rollLS');

UPDATE `creature_template` SET `VehicleId`=1673 WHERE  `entry`=52530 LIMIT 1;
UPDATE `creature_template` SET `VehicleId`=1673 WHERE  `entry`=54044 LIMIT 1;
UPDATE `creature_template` SET `VehicleId`=1673 WHERE  `entry`=54045 LIMIT 1;
UPDATE `creature_template` SET `VehicleId`=1673 WHERE  `entry`=54046 LIMIT 1;

UPDATE `creature_template` SET `difficulty_entry_1`=54052 WHERE  `entry`=53898 LIMIT 1;
UPDATE `creature_template` SET `minlevel`=88, `maxlevel`=88 WHERE  `entry`=54044 LIMIT 1;
UPDATE `creature_template` SET `minlevel`=86, `maxlevel`=86 WHERE  `entry`=54055 LIMIT 1;
UPDATE `creature_template` SET `minlevel`=86, `maxlevel`=86 WHERE  `entry`=54063 LIMIT 1;
UPDATE `creature_template` SET `Mana_mod`=0.0199925 WHERE  `entry`=54044 LIMIT 1;
UPDATE `creature_template` SET `minlevel`=87, `maxlevel`=87 WHERE  `entry`=54052 LIMIT 1;

UPDATE `creature_template` set `exp` = 3,faction_A =14,faction_H =14 where entry in(52530,54044,54055,54063,53896,53898,53734,54052);

UPDATE `creature_template` SET `mechanic_immune_mask`=650854395 WHERE  `entry`=52530 LIMIT 1;
UPDATE `creature_template` SET `mechanic_immune_mask`=650854395 WHERE  `entry`=54044 LIMIT 1;
UPDATE `creature_template` SET `mechanic_immune_mask`=650854395 WHERE  `entry`=53898 LIMIT 1;
UPDATE `creature_template` SET `mechanic_immune_mask`=650854395 WHERE  `entry`=54052 LIMIT 1;
UPDATE `creature_template` SET `mechanic_immune_mask`=617297915 WHERE  `entry`=53734 LIMIT 1;
UPDATE `creature_template` SET `mechanic_immune_mask`=617297915 WHERE  `entry`=54055 LIMIT 1;
UPDATE `creature_template` SET `mechanic_immune_mask`=617299707 WHERE  `entry`=53896 LIMIT 1;
UPDATE `creature_template` SET `mechanic_immune_mask`=617299707 WHERE  `entry`=54063 LIMIT 1;

UPDATE `creature_template` SET `baseattacktime`=1500 WHERE  `entry`=53898 LIMIT 1;
UPDATE `creature_template` SET `mindmg`=35000, `maxdmg`=38000 WHERE  `entry`=53898 LIMIT 1;

UPDATE `creature_template` SET `Health_mod`=6.898 WHERE  `entry`=53896 LIMIT 1;
UPDATE `creature_template` SET `Health_mod`=20.694 WHERE  `entry`=54063 LIMIT 1;
UPDATE `creature_template` SET `Health_mod`=178.063 WHERE  `entry`=53898 LIMIT 1;
UPDATE `creature_template` SET `Health_mod`=222.6 WHERE  `entry`=54052 LIMIT 1;
UPDATE `creature_template` SET `Health_mod`=86.1385 WHERE  `entry`=53734 LIMIT 1;
UPDATE `creature_template` SET `Health_mod`=301.35 WHERE  `entry`=54055 LIMIT 1;
UPDATE `creature_template` SET `Health_mod`=383.44 WHERE  `entry`=52530 LIMIT 1;
UPDATE `creature_template` SET `Health_mod`=1198.24 WHERE  `entry`=54044 LIMIT 1;

*/
