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
#define DATA_IMPRINTED              3
#define DATA_ACHIEVEMENTBF          4
#define DATA_ACHIEVEMENTIC          5
#define DATA_ACHIEVEMENTLS          6
#define DATA_ACHIEVEMENTFT          7
#define DATA_BOULDER                8

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
    SPELL_FULL_POWER            = 99504,
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

uint64 ALYSRAZOR_GUID = 0;

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
                //me->SetPower(POWER_MANA, 100);
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_PERIODIC_MANA_LEECH, true);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_POWER_DRAIN, true);
                me->SetFlying(true);
                sx = -41.78f;
                sy = -275.97f;
                Phase = 4;
                me->SetVisible(false);
                instance = creature->GetInstanceScript();
                ALYSRAZOR_GUID = me->GetGUID();
            }

            InstanceScript* instance;

            float sx;
            float sy;
            float i;
            float d;

            bool FlyOut;
            bool FlyDownFirst;
            bool EggSatchels;
            bool SpawnWorms;
            bool FirstWormPositions;
            bool FlyUP;
            bool SummonInitiate;
            bool FieryVortex;
            bool MoveMiddle;
            bool FieryTornado;
            bool Check;
            bool Dying;

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

                if (!me->GetAura(96301))
                    me->CastSpell(me, 96301, false);

                me->SetPower(POWER_MANA, 100);
                instance->SetData(TYPE_ALYSRAZOR, NOT_STARTED);
                C = 1;
                u = 0;
                y = 0;
                Rounds = 0;
                Cycle = 1;
                Enrage = 0;
                InitiatesFirst = 0;
                IncendiaryCloudFront = 4;
                Check = false;
                EggSatchels = false;
                FlyUP = false;
                FirstWormPositions = true;
                SpawnWorms = false;
                FlyDownFirst = true;
                FlyOut = false;
                Dying = false;

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
                //me->SetUInt64Value(UNIT_FIELD_TARGET, 0);
                HeraldTimer = 35000;
                SummonInitiate = true;
                SummonInitiateTimer = 20000;
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
                if(instance)
                {
                    instance->SetData(TYPE_ALYSRAZOR,NOT_STARTED);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                }
                ScriptedAI::EnterEvadeMode();
            }

            void JustDied(Unit* /*Killer*/)
            {
                me->MonsterYell("The light... mustn't... burn out...", LANG_UNIVERSAL, 0);
                DoPlaySoundToSet(me, SAY_DEATH);
                RemoveAuraFromAllPlayers(SPELL_MOLTEN_FEATHER, true, false);
                RemoveAuraFromAllPlayers(SPELL_FEATHER_BAR, true, true);
                instance->SetData(TYPE_ALYSRAZOR, DONE);
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

            uint32 GetData(uint32 type) const
            {
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

                if(caster->ToCreature() && caster->GetEntry() == NPC_BLAZING_TALON_CLAW)
                    return;

                if (caster && me->HasAura(SPELL_BURNOUT) && caster->getPowerType() == POWER_MANA)
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
                        me->SetFacingTo(me->GetAngle(sx, sy));
                        i = -4.5f*(M_PI/25);
                        me->GetMotionMaster()->MovePoint(3, sx + 55*cos(i), sy + 55*sin(i), 70.0);
                        ++FlyFront;
                        break;
                    case 4: //FlyCenter
                        FlyTimer = 9000;
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
                        /*DespawnCreatures(NPC_BRUSHFIRE);
                        DespawnCreatures(NPC_LAVA_WORM_TARGET);
                        DespawnCreatures(NPC_MOLTEN_FEATHER);
                        DespawnCreatures(NPC_BLAZING_TALON_CLAW);
                        DespawnCreatures(NPC_FIERY_TORNADO);
                        DespawnCreatures(NPC_FIERY_VORTEX);
                        DespawnCreatures(NPC_EGG_SATCHEL);
                        DespawnCreatures(NPC_MOLTEN_EGG);
                        DespawnCreatures(NPC_BLAZING_TALON_INITIATE);
                        DespawnCreatures(NPC_VORACIOUS_HATCHLING);
                        DespawnCreatures(NPC_PLUMP_LAVA_WORM);*/
                    }
                }

            void SummonNPC(uint32 entry)
            {
                switch (entry)
                {
                case NPC_EGG_SATCHEL:
                {
                    Creature* pBroodMother = me->SummonCreature(NPC_BROODMOTHER_1,30.0f,-307.0f,95.0f,2.61f,TEMPSUMMON_MANUAL_DESPAWN,0);
                    if (pBroodMother)
                    {
                        if (Creature* pSatchel1 = me->SummonCreature(NPC_MOLTEN_EGG,pBroodMother->GetPositionX(),pBroodMother->GetPositionY(),pBroodMother->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 30000))
                        {
                            pBroodMother->ForcedDespawn(12000);
                            pSatchel1->EnterVehicle(pBroodMother, 0, false);
                            pSatchel1->AI()->SetData(DATA_IMPRINTED, 1);
                            pBroodMother->GetMotionMaster()->MovePoint(0,-47.0f,-253.0f,54.8f + 18);
                        }
                    }

                    pBroodMother = NULL;

                    pBroodMother = me->SummonCreature(NPC_BROODMOTHER_2,19.0f,-327.0f,95.0f,2.65f,TEMPSUMMON_MANUAL_DESPAWN,0);

                    if (pBroodMother)
                    {
                        if (Creature* pSatchel2 = me->SummonCreature(NPC_MOLTEN_EGG,pBroodMother->GetPositionX(),pBroodMother->GetPositionY(),pBroodMother->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 30000))
                        {
                            pBroodMother->ForcedDespawn(12000);
                            pSatchel2->EnterVehicle(pBroodMother, 0, false);
                            pSatchel2->AI()->SetData(DATA_IMPRINTED, 2);
                            pBroodMother->GetMotionMaster()->MovePoint(0,-59.0f,-279.0f,55.0f + 18);
                        }
                    }
                }
                    break;
                case NPC_PLUMP_LAVA_WORM:
                    if (FirstWormPositions)
                    {
                        for(uint8 i=1; i<5 ; i++)
                            me->SummonCreature(NPC_PLUMP_LAVA_WORM, SpawnPositions[i].GetPositionX(), SpawnPositions[i].GetPositionY(), 55.3f, 0, TEMPSUMMON_MANUAL_DESPAWN);
                        FirstWormPositions = false;
                    }
                    else
                    {
                        for(uint8 i=5; i<9 ; i++)
                            me->SummonCreature(NPC_PLUMP_LAVA_WORM, SpawnPositions[i].GetPositionX(), SpawnPositions[i].GetPositionY(), 55.3f, 0, TEMPSUMMON_MANUAL_DESPAWN);
                        FirstWormPositions = true;
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

                            switch (C)
                                {
                                case 1:
                                    Z = 10.0f;
                                    O = me->GetOrientation()+M_PI/2;
                                    C = urand(2,4);
                                    break;
                                case 2:
                                    Z = 10.0f;
                                    O = me->GetOrientation()-M_PI/2;
                                    C = urand(3,4);
                                    break;
                                case 3:
                                    Z = -10.0f;
                                    O = me->GetOrientation()-M_PI/2;
                                    C = urand(1,2);
                                    break;
                                case 4:
                                    Z = -10.0f;
                                    O = me->GetOrientation()+M_PI/2;
                                    C = urand(1,3);
                                    break;
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

                                me->SummonCreature(NPC_INCENDIARY_CLOUD, me->GetPositionX() + 10*cos(O), me->GetPositionY() + 10*sin(O), me->GetPositionZ() + Z, 0, TEMPSUMMON_TIMED_DESPAWN, 3200);
                                return;
                            }
                            if (IncendiaryCloudFront == 4)
                                if (me->SummonCreature(NPC_BLAZING_POWER, me->GetPositionX() + 10*cos(O), me->GetPositionY() + 10*sin(O), me->GetPositionZ() + Z, me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 3200))
                                    {
                                        RemoveAuraFromAllPlayers(SPELL_BLAZING_PREVENTION, true, false);
                                        IncendiaryCloudFront = 1;
                                    }
                            break;
                        }
                        case NPC_BLAZING_POWER:
                            for (uint32 i=0; i<4 ; i++)
                            {
                                float u; // Distance
                                float l; // RAD
                                l = (urand(1,628))/100;
                                uint32 t = urand(1,3);
                                u = 15+(t*9);
                                if (Creature* Ring = me->SummonCreature(NPC_BLAZING_POWER, sx + u*cos(l), sy + u*sin(l), 56.0f, 0, TEMPSUMMON_TIMED_DESPAWN, 3200))
                                    Ring->GetMotionMaster()->MovePoint(3, sx + u*cos(l+0.01f), sy + u*sin(l+0.01f), 56.0f); //SetOrientation
                            }
                            break;
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
                if(Dying && FallingTimer <= diff)
                {
                    me->DealDamage(me,me->GetHealth());
                    return;
                }
                else FallingTimer -= diff;

                if (Dying)
                    return;

                if (instance->GetData(TYPE_ALYSRAZOR) == FAIL && Phase != 4)
                {
                    RemoveAuraFromAllPlayers(0, false, true);
                    RemoveAuraFromAllPlayers(SPELL_FEATHER_BAR, true, false);
                    RemoveAuraFromAllPlayers(SPELL_MOLTEN_FEATHER, true, false);
                    me->CombatStop(true);
                    me->SetVisible(false);
                    me->SetHealth(me->GetMaxHealth());
                    me->RemoveAllAuras();
                    me->SetSpeed(MOVE_FLIGHT, 20.0f);
                    me->GetMotionMaster()->MovePoint(3, me->GetHomePosition());
                    Phase = 4;
                    Reset();
                }

                if (Phase == 4 && instance->GetData(TYPE_ALYSRAZOR) == IN_PROGRESS)
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    Phase = STAGE_ONE;
                    me->SetVisible(true);
                }
                
                if (instance->GetData(TYPE_ALYSRAZOR) == FAIL || instance->GetData(TYPE_ALYSRAZOR) == NOT_STARTED || Phase == 4)
                    return;

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
                        EggSatchelTimer = 10000;
                        Check = true;
                        SpawnWorms = true;
                        EggSatchels = true;
                    }

                    if (FlyTimer <= diff)
                    {
                        if (FlyUP)
                        {
                            //Heroic
                            if ((Cycle == 2 || Cycle == 3) && Rounds == 0 && IsHeroic())
                            {
                                std::list<GameObject*> objects;
                                me->GetGameObjectListWithEntryInGrid(objects, 208966, 200.0f);
                                if (!objects.empty())
                                    for (std::list<GameObject*>::iterator iter = objects.begin(); iter != objects.end(); ++iter)
                                        if ((*iter)->IsInWorld())
                                            (*iter)->RemoveFromWorld();

                                DespawnCreatures(53784);
                                if (Is25ManRaid())
                                {
                                    for(uint8 i=0; i<22 ; i++)
                                        DoCast(100839);
                                }
                                else for(uint8 i=0; i<9 ; i++)
                                        DoCast(100839);
                            }
                            me->SetSpeed(MOVE_FLIGHT, 2.1f);
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
                        else if (Rounds == 101 && Cycle == 3)
                        {
                            DespawnCreatures(NPC_LAVA_WORM_TARGET);
                            DespawnCreatures(NPC_PLUMP_LAVA_WORM);
                            me->MonsterYell("These skies are MINE!", LANG_UNIVERSAL, 0);
                            DoPlaySoundToSet(me, SAY_SPIRAL_01);
                            Phase = STAGE_TWO;
                            StageTimer = 30000;
                            FieryTornado = true;
                            MoveMiddle = false;
                            FieryVortex = false;
                            Cycle = 0;
                            Rounds = 0;
                        }
                    }
                    else if ((Rounds == 138 && Cycle == 1) || (Rounds == 151 && Cycle == 2))
                        {
                            me->GetMotionMaster()->MovePoint(3, sx + (55*cos(i)*d), sy + 55*sin(i), 135.0f);
                            if (me->FindNearestCreature(NPC_BLAZING_TALON_INITIATE, 200.0f, true))
                            {
                                std::list<Creature*> creatures;
                                GetCreatureListWithEntryInGrid(creatures, me, NPC_BLAZING_TALON_INITIATE, 100.0f);
                                if (!creatures.empty())
                                    for (std::list<Creature*>::iterator iter = creatures.begin(); iter != creatures.end(); ++iter)
                                        if ((*iter)->IsInWorld() && (*iter)->isAlive())
                                            (*iter)->CastSpell((*iter), SPELL_BLAZING_SHIELD_HC, false);
                            }
                            switch(urand(0,1))
                            {
                            case 0:
                                d = -1.0f;
                                break;
                            case 1:
                                d = 1.0f;
                                break;
                            }
                            Rounds = 0;
                            if (Creature* Summon = me->SummonCreature(NPC_FIRESTORM, sx, sy, 70.0f, 0, TEMPSUMMON_TIMED_DESPAWN, 15000))
                            {
                                Summon->setFaction(35);
                                DoCast(SPELL_FIRESTORM_HC);
                            }
                            ++Cycle;
                            EggSatchelTimer = 5000;
                            EggSatchels = true;
                            IncendiaryCloudTimer = 15000;
                            FlyTimer = 15000;
                        } else if (Rounds == 276 && Cycle == 3)
                        {
                            me->MonsterYell("These skies are MINE!", LANG_UNIVERSAL, 0);
                            DoPlaySoundToSet(me, SAY_SPIRAL_01);
                            Phase = STAGE_TWO;
                            StageTimer = 30000;
                            FieryTornado = true;
                            MoveMiddle = false;
                            FieryVortex = false;
                            Cycle = 0;
                            Rounds = 0;
                        }

                    if (Creature* Summon = me->FindNearestCreature(NPC_FIRESTORM, 200.0f, true))
                        me->SetFacingToObject(Summon);

                    // Adds Timers
                    if (SummonInitiate && SummonInitiateTimer <= diff) //Initiate Timer
                    {
                        SummonInitiateTimer = 31000;
                        SummonNPC(NPC_BLAZING_TALON_INITIATE);
                    }
                    else SummonInitiateTimer -= diff;

                    if (Check && SpawnWorms && SpawnWormsTimer <= diff) //Worms Timer
                    {
                        SummonNPC(NPC_PLUMP_LAVA_WORM);
                        SpawnWorms = false;
                    }
                    else SpawnWormsTimer -= diff;

                    if (Check && !me->FindNearestCreature(NPC_PLUMP_LAVA_WORM,300.0f) && !SpawnWorms) //Worms Respawn
                    {
                        SpawnWorms = true;
                        SpawnWormsTimer = 15000;
                    }

                    if (Check && EggSatchelTimer <= diff && EggSatchels) //EggSatchel Timer
                    {
                        SummonNPC(NPC_EGG_SATCHEL);
                        EggSatchels = false;
                    }
                    else EggSatchelTimer -= diff;

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
                    if (HeraldTimer <= diff && IsHeroic()) //Initiate Timer
                    {
                        HeraldTimer = 31000;
                        SummonNPC(NPC_HERALD_OF_BURNING);
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
                        SummonNPC(NPC_BLAZING_POWER);
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
                        //me->SetUInt64Value(UNIT_FIELD_TARGET, me->getVictim()->GetGUID());
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
                        DoPlaySoundToSet(me, SAY_SPIRAL_02);
                        me->SetFloatValue(UNIT_FIELD_COMBATREACH, 55);
                        me->RemoveAura(SPELL_IGNITED);
                        me->RemoveAura(SPELL_BLAZING_CLAW);
                        me->CastSpell(me, SPELL_FULL_POWER, false);
                        // Reset STAGE_ONE
                        Phase = 6;
                        IncendiaryCloudTimer = 10000;
                        EggSatchelTimer = 10000;
                        if (!me->FindNearestCreature(NPC_PLUMP_LAVA_WORM,200.0f))
                        {
                            SpawnWormsTimer = 35000;
                            SpawnWorms = true;
                        }
                        EggSatchels = true;
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
                    instance->SetData(TYPE_ALYSRAZOR, IN_PROGRESS);
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
            npc_Captive_DuidAI(Creature* creature) : ScriptedAI(creature){}

            void JustDied(Unit* /*Killer*/)
            {
                if (Creature* Target = me->SummonCreature(NPC_LAVA_WORM_TARGET, 111.7f, -400.8f, 100.9f, 0, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 3000))
                    me->CastSpell(Target, SPELL_COSMETIC_DEATH, false);
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

        void Reset()
        {
            me->SetReactState(REACT_AGGRESSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
            me->SetInCombatWithZone();
            //me->CastSpell(me, SPELL_FIERY_VORTEX, false);
            me->setFaction(14);
            harshWindsTimer = 6000;
        }

        void EnterCombat(Unit* /*who*/)
        {
            me->CastSpell(me, SPELL_FIERY_VORTEX, false);
        }

        void UpdateAI(const uint32 diff)
        {
            if (harshWindsTimer <= diff)
            {
                me->CastSpell(me,SPELL_HARSH_WINDS,true);
                harshWindsTimer = 1000;
            }
            else harshWindsTimer -= diff;
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
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
                instance = creature->GetInstanceScript();
                me->SetReactState(REACT_AGGRESSIVE);
                me->SetFlying(false);
                StartIntro = false;
                FlyUp = false;
            }

            InstanceScript* instance;

            bool StartIntro;
            bool FlyUp;

            uint32 KillTimer;
            uint32 Timer;


        void MoveInLineOfSight(Unit* who)
        {
            if (who && who->ToPlayer() && !who->ToPlayer()->isGameMaster() && who->GetDistance(me) <= 40.0f)
            if (!StartIntro)
            {
                me->CastSpell(me, SPELL_SMOULDERING_ROOTS, true);
                me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_TALK);
                me->MonsterYell("What have we here - visitors to our kingdom in the Firelands?", LANG_UNIVERSAL, 0);
                DoPlaySoundToSet(me, SAY_FENDRAL_01);
                me->GetMotionMaster()->MovePoint(3, 29.02f, -329.64f, 50.4f);
                Timer = 10000;
                StartIntro = true;
            }
            ScriptedAI::MoveInLineOfSight(who);
        }

            void UpdateAI(const uint32 diff)
            {
                if (Timer <= diff && StartIntro && !FlyUp)
                {
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
                    instance->SetData(TYPE_ALYSRAZOR, IN_PROGRESS);
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
                me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
            }

            uint32 CastTimer;
            uint32 SpawnTimer;
            uint32 Imprinte;

            void SetData(uint32 Type, uint32 Data)
            {
                if (Type == DATA_IMPRINTED)
                    Imprinte = Data;
            }

            void UpdateAI(const uint32 diff)
            {
                if (CastTimer <= diff)
                {
                    me->CastSpell(me, SPELL_EXPLOSION_EGG, false);
                    CastTimer = 50000;
                }
                else CastTimer -= diff;

                if (SpawnTimer <= diff)
                {
                    if( Unit * pAlys = Unit::GetUnit(*me,ALYSRAZOR_GUID))
                    if (Creature* HATCHLING = pAlys->SummonCreature(NPC_VORACIOUS_HATCHLING, me->GetPositionX(), me->GetPositionY(), 55.3f))
                        HATCHLING->AI()->SetData(DATA_IMPRINTED, Imprinte);
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
                SpawnEggTimer = 6000;
            }

            uint32 SpawnEggTimer;
            uint32 Imprinte;

            void SetData(uint32 Type, uint32 Data)
            {
                if (Type == DATA_IMPRINTED)
                    Imprinte = Data;
            }

             void UpdateAI(const uint32 diff)
            {
                if (SpawnEggTimer <= diff)
                {
                    SpawnEggTimer = 100000;

                    if (Creature* pEgg1 = me->FindNearestCreature(NPC_MOLTEN_EGG,20.0f,true))
                    {
                        pEgg1->ExitVehicle();
                        pEgg1->GetMotionMaster()->MoveFall();
                        pEgg1->AI()->SetData(DATA_IMPRINTED, Imprinte);
                        pEgg1->SetReactState(REACT_PASSIVE);
                        me->GetMotionMaster()->MovePoint(0,-111.0f,-277.0f,95.0f);
                    }

                    /*if( Unit * pAlys = Unit::GetUnit(*me,ALYSRAZOR_GUID))
                    if (Creature* Egg = pAlys->SummonCreature(NPC_MOLTEN_EGG, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ()))
                    {
                        Egg->AI()->SetData(DATA_IMPRINTED, Imprinte);
                        Egg->SetReactState(REACT_PASSIVE);
                    }
                    me->GetMotionMaster()->MovePoint(3, me->GetHomePosition());*/
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
                distance = 55.0f;
                me->setFaction(16);
                me->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), 53.0f);
                me->SetReactState(REACT_PASSIVE);
                me->SetSpeed(MOVE_FLIGHT, 50.2f);
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
                FallTimer = 5000;
            }

            uint32 FallTimer;

            float i;
            float distance;

            bool Fall;

            void JustDied(Unit* /*Killer*/)
            {
                DoCast(SPELL_SUMMON_GO_METEOR);
                if (Creature* Meteor = me->FindNearestCreature(53784, 30.0f, true))
                    Meteor->CastSpell(Meteor, SPELL_MOLTEN_METOER_DEAD, false);
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
                    me->SetSpeed(MOVE_FLIGHT, 2.0f);
                    DoCast(SPELL_METEORIC_IMPACT);
                    Fall = true;
                }
                else FallTimer -=diff;
                
                if (Fall && me->isAlive())
                {
                    me->GetMotionMaster()->MovePoint(0, me->GetPositionX() + 60*cos(i), me->GetPositionY() + 60*sin(i), 53.0f);
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
                        if (TempSummon* summon = me->ToTempSummon())
                            summon->ForcedDespawn(200);
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
                if (i != 0.0f && Initialize)
                {
                    me->GetMotionMaster()->MovePoint(0, me->GetPositionX() + 200*cos(i), me->GetPositionY() + 200*sin(i), 55.0f);
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
                    //if (Alysrazor->AI()->GetData(DATA_STAGE_THREE_TIMER) == 5)
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

            uint32 FieroblastTimer;
            uint32 BrushfireTimer;
            
            bool Transform;
            bool Male;

            void UpdateAI(const uint32 diff)
            {
                if (FlyTimer <= diff && !Transform)
                {
                    me->SetFlying(false);

                    // !!!!!!!!!!!!!!!!!!!!!!!!!!
                    //me->SetDisableGravity(false);

                    me->GetMotionMaster()->MovePoint(3, me->GetPositionX(), me->GetPositionY(), 55.0f);
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
                    FieroblastTimer = 10000;
                    BrushfireTimer = 3000;
                    Transform = true;
                    //DoZoneInCombat(me);
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

                if (BrushfireTimer <= diff)
                {
                    if (!me->IsNonMeleeSpellCasted(false))
                    {
                        me->CastSpell(me, SPELL_BRUSHFIRE, false);
                        BrushfireTimer = 12000;
                        FieroblastTimer = 4000;
                    }
                }
                else BrushfireTimer -= diff;

                if (FieroblastTimer <= diff)
                {
                    if (!me->IsNonMeleeSpellCasted(false))
                    {
                        if (Unit* Target = SelectTarget(SELECT_TARGET_NEAREST, 0, 100.0f, true))
                            me->CastSpell(Target, SPELL_FIEROBLAST, false);
                        FieroblastTimer = 10000;
                        BrushfireTimer = 4000;
                    }
                }
                else FieroblastTimer -= diff;
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
                Imprinte = 0;
                Passive = false;
                me->PlayOneShotAnimKit(ANIM_HATH);
                me->addUnitState(UNIT_STAT_ROOT);
                me->AddAura(SPELL_SATIATED, me);
                me->SetUInt64Value(UNIT_FIELD_TARGET, 0);
                HathTimer = 3000;
                Imprinted = true;
                instance = me->GetInstanceScript();

            }

            uint32 GushingWoundTimer;
            uint32 PassiveTimer;
            uint32 HathTimer;
            uint32 Threat;
            uint32 Imprinte;
            InstanceScript * instance;

            bool Imprinted;
            bool Passive;


            void Reset()
            {
                if(instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }

            void EnterCombat(Unit* /*target*/)
            {
                me->SetInCombatWithZone();

                if(instance)
                     instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);

                GushingWoundTimer = 15000;
            }

            void SetData(uint32 Type, uint32 Data)
            {
                if (Type == DATA_IMPRINTED)
                    Imprinte = Data;
            }

            void JustDied(Unit* /*Killer*/)
            {
                if(instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

                Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                    if (Player* player = i->getSource())
                    {
                        if (Imprinte == 1)
                            if (Aura* Imprinted = player->GetAura(99389))
                                player->RemoveAura(Imprinted);
                            if (Imprinte == 2)
                                if (Aura* Imprinted = player->GetAura(100359))
                                    player->RemoveAura(Imprinted);
                    }
            }

            void DeleteFromThreatList(uint64 TargetGUID)
            {
                std::list<HostileReference*> const& threatlist = me->getThreatManager().getThreatList();
                for (std::list<HostileReference*>::const_iterator itr = threatlist.begin(); itr != threatlist.end(); ++itr)
                {
                    if ((*itr)->getUnitGuid() == TargetGUID)
                    {
                        (*itr)->removeReference();
                        break;
                    }
                }
            }

            bool CheckImprinted()
            {
                Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                    if (Player* player = i->getSource())
                    {
                        if (Imprinted)
                        {
                            if (player->GetAura(99389) && Imprinte == 1)
                                return true;
                            if (player->GetAura(100359) && Imprinte == 2)
                                return true;
                        }
                    }
                return false;
            }

            void UpdateAI(const uint32 diff)
            {
                if (!me->GetAura(SPELL_SATIATED) && !me->GetAura(SPELL_HUNGRY))
                    me->CastSpell(me, SPELL_HUNGRY, false);

                if (CheckImprinted() && !Imprinted)
                    {
                        HathTimer = 1000;
                        Imprinted = true;
                    }

                if (HathTimer <= diff && Imprinted)
                {
                    if (Unit* Target = SelectTarget(SELECT_TARGET_NEAREST, 0, 20.0f, true))
                    {
                        if (Target->GetPositionZ() > me->GetPositionZ()+10.0f)
                            return;
                        me->SetInCombatWithZone();
                        me->AddThreat(Target, 5000.0f);
                        me->clearUnitState(UNIT_STAT_ROOT);
                        if (Imprinte == 1)
                        {
                            Target->AddAura(SPELL_IMPRINTED_TAUNT, me);
                            me->CastSpell(Target, SPELL_IMPRINTED, false);
                        }
                        else if (Imprinte == 2)
                                {
                                    Target->AddAura(SPELL_IMPRINTED_TAUNT2, me);
                                    me->CastSpell(Target, SPELL_IMPRINTED2, false);
                                }
                        Imprinted = false;
                    }
                }
                else HathTimer -= diff;

                if (!me->getVictim() && (me->GetAura(SPELL_IMPRINTED2) || me->GetAura(SPELL_IMPRINTED)))
                {
                    me->RemoveAura(SPELL_IMPRINTED2);
                    me->RemoveAura(SPELL_IMPRINTED);
                }

                if (!me->getVictim())
                    return;

                if (Imprinte == 1)
                    if (!me->getVictim()->GetAura(99389) && !Imprinted)
                    {
                        DeleteFromThreatList(me->getVictim()->GetGUID());
                        me->SetInCombatWithZone();
                    }

                if (Imprinte == 2)
                    if (!me->getVictim()->GetAura(100359) && !Imprinted)
                    {
                        DeleteFromThreatList(me->getVictim()->GetGUID());
                        me->SetInCombatWithZone();
                    }

                if ((me->getVictim()->GetPositionZ() >= me->GetPositionZ()+10.0f) && !Imprinted)
                {
                    Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                    for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                        if (Player* player = i->getSource())
                        {
                            if (!Imprinted)
                            {
                                if (player->GetAura(99389) && Imprinte == 1)
                                {
                                    me->RemoveAura(SPELL_IMPRINTED_TAUNT);
                                    player->RemoveAura(99389);
                                }
                                if (player->GetAura(100359) && Imprinte == 2)
                                {
                                    me->RemoveAura(SPELL_IMPRINTED_TAUNT2);
                                    player->RemoveAura(100359);
                                }
                            }
                        }
                    HathTimer = 1000;
                    Imprinted = true;
                    me->CombatStop(true);
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
                        PassiveTimer = 1500;
                        Passive = true;
                        me->RemoveAura(SPELL_HUNGRY);
                        me->RemoveAura(SPELL_TANTRUM);
                        me->RemoveAura(SPELL_SATIATED);
                        me->AddAura(SPELL_SATIATED, me);
                        me->addUnitState(UNIT_STAT_ROOT);
                        me->SetFacingToObject(Worm);
                        me->PlayOneShotAnimKit(ANIM_ATTACK_FEED);
                        Worm->setDeathState(JUST_DIED);
                        if (Creature* Target = me->FindNearestCreature(NPC_LAVA_WORM_TARGET, 50.0f))
                            Target->ForcedDespawn();
                        Worm->ForcedDespawn(2000);
                    }

                if (PassiveTimer <= diff && Passive)
                {
                    Passive = false;
                    me->clearUnitState(UNIT_STAT_ROOT);
                }
                else PassiveTimer -= diff;

                if (!Passive)
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
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
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

                if (Creature* Target = me->FindNearestCreature(NPC_LAVA_WORM_TARGET,50.0f))
                    me->SetFacingToObject(Target);

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
                me->SetSpeed(MOVE_RUN, 4.0f);
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
                if (Aura * a = who->GetAura(98619))
                {
                    if (a->GetDuration() <= 28000)
                    {
                        a->SetDuration(30000);
                        who->AddAura(SPELL_BLAZING_POWER_EFFECT,who);
                    }
                }
            }
        }

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
                if (Aura* Gushing = GetTarget()->GetAura(SPELL_GUSHING_WOUND_Y10))
                    GetTarget()->RemoveAura(Gushing);
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
                if( Unit * pAlys = Unit::GetUnit(*GetCaster(),ALYSRAZOR_GUID))
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
                            GetTarget()->AddAura(SPELL_WINGS_OF_FLAME_FLY,GetTarget()); // triggered spell was bugged due to ( You are in wrong zone message )
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
    new spell_cataclysm();
    new spell_gen_harsh_winds();
    new achievement_do_a_barrel_rollBF();
    new achievement_do_a_barrel_rollIC();
    new achievement_do_a_barrel_rollLS();
    new achievement_do_a_barrel_rollFT();
}

/*
UPDATE `creature_template` SET `InhabitType`=4 WHERE  `entry`=52530 LIMIT 1;
UPDATE `creature_template` SET `InhabitType`=4 WHERE  `entry`=54015 LIMIT 1;

UPDATE `creature_template` SET `modelid1`=0 WHERE  `entry`=53554 LIMIT 1;
UPDATE `creature_template` SET `InhabitType`=4 WHERE  `entry`=53554 LIMIT 1;

UPDATE `creature_template` SET `modelid1`=0, `InhabitType`=4 WHERE  `entry`=53541 LIMIT 1;
UPDATE `creature_template` SET `modelid1`=0 WHERE  `entry`=53372 LIMIT 1;

UPDATE `creature_template` SET `modelid1`=0 WHERE  `entry`=53158 LIMIT 1;
UPDATE `creature_template` SET `modelid1`=0 WHERE  `entry`=53986 LIMIT 1;
UPDATE `creature_template` SET `modelid1`=0 WHERE  `entry`=53693 LIMIT 1;
UPDATE `creature_template` SET `modelid1`=0 WHERE  `entry`=53521 LIMIT 1;

UPDATE `creature_template` SET `faction_A`=14, `faction_H`=14 WHERE  `entry`=53372 LIMIT 1;
UPDATE `creature_template` SET `faction_A`=14, `faction_H`=14 WHERE  `entry`=53541 LIMIT 1;

UPDATE `creature_template` SET `minlevel`=85, `maxlevel`=85, `exp`=3 WHERE  `entry`=53541 LIMIT 1;
UPDATE `creature_template` SET `modelid1`=0 WHERE  `entry`=53698 LIMIT 1;

UPDATE `creature_template` SET `ScriptName`='npc_fiery_vortex' WHERE  `entry`=53693 LIMIT 1;
UPDATE `creature_template` SET `Mana_mod`=0.01999245 WHERE  `entry`=52530 LIMIT 1;

DELETE FROM `spell_script_names` WHERE  `spell_id`=100640 AND `ScriptName`='spell_gen_harsh_winds' LIMIT 1;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (100640, 'spell_gen_harsh_winds');

UPDATE `creature_template` SET `InhabitType`=4 WHERE  `entry`=53896 LIMIT 1;
UPDATE `creature_template` SET `InhabitType`=4 WHERE  `entry`=54063 LIMIT 1;
UPDATE `creature_template` SET `InhabitType`=4 WHERE  `entry`=54064 LIMIT 1;

UPDATE `creature_template` SET `exp`=3, `faction_A`=14, `faction_H`=14 WHERE  `entry`=53158 LIMIT 1;

UPDATE `creature_template` SET `ScriptName`='npc_Blazing_Broodmother' WHERE  `entry`=53680 LIMIT 1;
UPDATE `creature_template` SET `ScriptName`='npc_Blazing_Broodmother' WHERE  `entry`=53900 LIMIT 1;

INSERT INTO `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`,`position_y`,`position_z` , `orientation`,`spawntimesecs`,`spawndist`,`currentwaypoint`, `curhealth`,`curmana`,`MovementType`,`npcflag`,`unit_flags`,`dynamicflags`) VALUES
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
*/
