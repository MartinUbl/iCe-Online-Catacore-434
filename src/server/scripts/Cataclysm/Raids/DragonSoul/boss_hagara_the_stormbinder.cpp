/*
 * Copyright (C) 2006-2015 iCe Online <http://ice-wow.eu>
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
Encounter: Hagara the Stormbinder
Dungeon: Dragon Soul
Difficulty: Normal / Heroic
Mode: 10-man / 25-man
Autor: Lazik
*/

/* 80 % completed
TO DO:
1) Snowflake - Freezing trap AoE, when dispelled outside of Frozen Tempest
2) Lightning Phase - Correctly end Lightning Phase, when all conducotrs are charged, also need to script charging system
3) Ice Lance debuff - This debuff should stack when ice lance from different summons hits the same target
4) Brutal Assault - Not working at all
5) Hagara Axes visual - Dummmy auras for axes are not working at all - this is only a minor thing
*/

/*
TO DO: Add correct sounds to spells
26251, 10, "VO_DS_HAGARA_INTRO_01", "vo_ds_hagara_intro_01.ogg"
26252, 10, "VO_DS_HAGARA_LIGHTNING_01", "vo_ds_hagara_lightning_01.ogg"
26253, 10, "VO_DS_HAGARA_LIGHTNING_02", "vo_ds_hagara_lightning_02.ogg"
26228, 10, "VO_DS_HAGARA_CIRCUIT_01", "vo_ds_hagara_circuit_01.ogg"
26229, 10, "VO_DS_HAGARA_CIRCUIT_02", "vo_ds_hagara_circuit_02.ogg"
26230, 10, "VO_DS_HAGARA_CIRCUIT_03", "vo_ds_hagara_circuit_03.ogg"
26231, 10, "VO_DS_HAGARA_CIRCUIT_04", "vo_ds_hagara_circuit_04.ogg"
26232, 10, "VO_DS_HAGARA_CIRCUIT_06", "vo_ds_hagara_circuit_06.ogg"
26233, 10, "VO_DS_HAGARA_CIRCUIT_05", "vo_ds_hagara_circuit_05.ogg"
26234, 10, "VO_DS_HAGARA_CIRCUIT_07", "vo_ds_hagara_circuit_07.ogg"
26235, 10, "VO_DS_HAGARA_CRYSTALDEAD_01", "vo_ds_hagara_crystaldead_01.ogg"
26236, 10, "VO_DS_HAGARA_CRYSTALDEAD_02", "vo_ds_hagara_crystaldead_02.ogg"
26237, 10, "VO_DS_HAGARA_CRYSTALDEAD_03", "vo_ds_hagara_crystaldead_03.ogg"
26238, 10, "VO_DS_HAGARA_CRYSTALDEAD_04", "vo_ds_hagara_crystaldead_04.ogg"
26239, 10, "VO_DS_HAGARA_CRYSTALDEAD_05", "vo_ds_hagara_crystaldead_05.ogg"
26240, 10, "VO_DS_HAGARA_CRYSTALDEAD_06", "vo_ds_hagara_crystaldead_06.ogg"
26241, 10, "VO_DS_HAGARA_CRYSTALDEAD_07", "vo_ds_hagara_crystaldead_07.ogg"
26242, 10, "VO_DS_HAGARA_CRYSTALHIT_01", "vo_ds_hagara_crystalhit_01.ogg"
26223, 10, "VO_DS_HAGARA_ADDS_01", "vo_ds_hagara_adds_01.ogg"
26224, 10, "VO_DS_HAGARA_ADDS_02", "vo_ds_hagara_adds_02.ogg"
26225, 10, "VO_DS_HAGARA_ADDS_03", "vo_ds_hagara_adds_03.ogg"
26226, 10, "VO_DS_HAGARA_ADDS_04", "vo_ds_hagara_adds_04.ogg"
*/

#include "ScriptPCH.h"
#include "dragonsoul.h"
#include "Spell.h"
#include "UnitAI.h"
#include "MapManager.h"
#include <stdlib.h>

#define DATA_ICE_WAVE_SLOT           1

// NPCs
enum NPC
{
    BOSS_HAGARA_THE_STORMBINDER     = 55689,
    NPC_ICE_TOMB                    = 55695,
    NPC_ICE_LANCE                   = 56108,
    NPC_ICE_WAVE                    = 56104,
    NPC_FROZEN_BINDING_CRYSTAL      = 56136,
    NPC_CRYSTAL_CONDUCTOR           = 56165,
    NPC_BOUND_LIGHNING_ELEMENTAL    = 56700,
    NPC_COLLAPSING_ICICLE           = 57867,

    NPC_LIGHTNING_STORM_TEST        = 119950,
};

// Spells
enum Spells
{
    // Normal Phase
    FOCUSED_ASSAULT                 = 107850, // Focused Assault aura
    FOCUSED_ASSAULT_10N             = 107851, // 10N dmg
    FOCUSED_ASSAULT_25N             = 110900, // 25N dmg
    FOCUSED_ASSAULT_10HC            = 110899, // 10HC dmg
    FOCUSED_ASSAULT_25HC            = 110898, // 10HC dmg

    ICE_LANCE_DUMMY                 = 105269, // Ice Lance - dummy aura
    ICE_LANCE_SUMMON                = 105297, // Summon target dest dest

    TARGET                          = 105285, // Target

    ICE_LANCE                       = 105298, // ?

    ICE_LANCE_MISSILE               = 105313, // Missile

    ICE_LANCE_CONE                  = 105287, // Cone enemy 104
    ICE_LANCE_10N                   = 105316, // 10N dmg 
    ICE_LANCE_25N                   = 107061, // 25N dmg
    ICE_LANCE_10HC                  = 107062, // 10HC dmg
    ICE_LANCE_25HC                  = 107063, // 25HC dmg

    ICE_TOMB_DUMMY                  = 104448, // Ice Tomb
    ICE_TOMB_STUN                   = 104451, // Stun + dmg
    ICE_TOMB_MISSILE                = 104449, // Ice Tomb

    SHATTERED_ICE_10N               = 105289, // Shattered Ice 10N
    SHATTERED_ICE_25N               = 108567, // Shattered Ice 25N
    SHATTERED_ICE_10HC              = 110888, // Shattered Ice 10HC
    SHATTERED_ICE_25HC              = 110887, // Shattered Ice 25HC

    // Ice Phase
    FROZEN_TEMPEST_10N              = 105256, // Frozen Tempest 10N
    FROZEN_TEMPEST_25N              = 109552, // Frozen Tempest 25N
    FROZEN_TEMPEST_10HC             = 109553, // Frozen Tempest 10HC
    FROZEN_TEMPEST_25HC             = 109554, // Frozen Tempest 25HC

    WATERY_ENTRENCHMENT             = 110317, // Watery Entrenchment - aura periodic dmg percent + snare
    WATERY_ENTRENCHMENT_1           = 105259, // Watery Entrenchment - area trigger

    ICICLE                          = 109315,

    ICE_WAVE_AURA                   = 105265, // Ice Wave aura
    ICE_WAVE_DMG                    = 105314, // Ice Wave dmg

    CRYSTALLINE_OVERLOAD            = 105312, // Crystalline Overload - Description: Increases the damage taken by Hagara the Binder by $s1%.
    CRYSTALLINE_TETHER              = 105311, // Crystalline Tether - dummy aura
    CRYSTALLINE_TETHER_1            = 105482, // Crystalline Tether - dummy aura 

    // Heroic ability
    FROSTFLAKE                      = 109325,
    FROSTFLAKE_SNARE                = 109337,
    FROSTFLAKE_SNARE_1              = 110316,

    // Lightning Phase
    WATER_SHIELD_10N                = 105409, // Water Shield 10N
    WATER_SHIELD_25N                = 109560, // Water Shield 25N
    WATER_SHIELD_10HC               = 109561, // Water Shield 10HC
    WATER_SHIELD_25HC               = 109562, // Water Shield 25HC

    LIGHTNING_STORM                 = 105467, // Lightning Storm - force cast
    LIGHTNING_STORM_10N             = 105465, // Lightning Storm 10N
    LIGHTNING_STORM_25N             = 108568, // Lightning Storm 25N
    LIGHTNING_STORM_10HC            = 110893, // Lightning Storm 10HC
    LIGHTNING_STORM_25HC            = 110892, // Lightning Storm 25HC
    LIGHTNING_STORM_COSMETIC        = 105466, // Lightning Storm Cosmetic

    LIGHTNING_CONDUIT               = 105367, // Lightning Conduit aura
    LIGHTNING_CONDUIT_DUMMY         = 105371, // Lightning Conduit dummy
    LIGHTNING_CONDUIT_DONTKNOW      = 105377, // ?
    LIGHTNING_CONDUIT_10N           = 105369, // Lightning Conduit 10N - aura and periodic dmg
    LIGHTNING_CONDUIT_25N           = 108569, // Lightning Conduit 25N
    LIGHTNING_CONDUIT_10HC          = 109201, // Lightning Conduit 10HC
    LIGHTNING_CONDUIT_25HC          = 109202, // Lightning Conduit 25HC

    LIGHTNING_ROD                   = 105343, // Lightning Rod - dummy visual - visual aura on lightning rod
    LIGHTNING_ROD_DUMMY             = 109180, // Lightning Rod - dummy visual - shining under lightning rod when overloaded

    // Heroic ability
    STORM_PILLAR                    = 109541, // Trigger circle under player
    STORM_PILLAR_1                  = 109557,
    STORM_PILLAR_10N                = 109563, // 10N dmg
    STORM_PILLAR_25N                = 109564, // 25N dmg
    STORM_PILLAR_10HC               = 109565, // 10Hc dmg
    STORM_PILLAR_25HC               = 109566, // 25HC dmg

    // Other spells
    OVERLOAD                        = 105481, // Overload
    OVERLOAD_FORCE_STORM            = 105487, // Overload

    PERMANENT_FEIGN_DEATH           = 70628,  // Permanent Feign Death

    TELEPORT_VISUAL                 = 101812, // Visual Port
    ENRAGE                          = 64238,  // Enrage
    FEEDBACK                        = 108934, // Feedback
    KNOCKBACK                       = 105048,

    HAGARA_LIGHTNING_AXES           = 109670,
    HAGARA_FROST_AXES               = 109671,
};

enum Phases
{
    NORMAL_PHASE       = 0,
    LIGHTNING_PHASE    = 1,
    ICE_PHASE          = 2,
};

const float SpawnPos[6][4] =
{
    // Frozen Binding Crystals
    { 13617.26f, 13643.15f, 123.48f, 3.922f },
    { 13557.48f, 13643.17f, 123.48f, 5.493f },
    { 13557.68f, 13580.76f, 123.48f, 0.812f },
    { 13617.60f, 13580.38f, 123.48f, 2.363f },
    // Hagara Spawn Position
    { 13587.70f, 13612.20f, 122.42f, 1.530f },
    // Bound Lightning Elemental
    { 13588.70f, 13648.67f, 123.45f, 4.677f },
};

struct Dist
{
    uint64 guid;
    float distance;
};

std::vector<Dist> dist;

// Hagara the Stormbinder
class boss_hagara_the_stormbinder : public CreatureScript
{
public:
    boss_hagara_the_stormbinder() : CreatureScript("boss_hagara_the_stormbinder") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_hagara_the_stormbinderAI(pCreature);
    }

    struct boss_hagara_the_stormbinderAI : public ScriptedAI
    {
        boss_hagara_the_stormbinderAI(Creature *creature) : ScriptedAI(creature),Summons(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Random_Text;
        uint32 Phase;
        uint32 Enrage_Timer;
        uint32 Normal_Phase_Timer;
        uint32 Next_Phase;
        uint32 Frozen_Crystal_Killed;
        uint32 Water_Shield_Timer;
        uint32 Ice_Wave_Timer;
        uint32 Focused_Assault_Timer;
        uint32 FA_Attack_Timer;
        uint32 Summon_Timer;
        uint32 Check_Timer;
        uint32 Icicle_Timer;
        uint32 Ice_Lance_Timer;
        uint32 Shattered_Ice_Timer;
        uint32 Ice_Tomb_Timer;
        uint32 Wave;
        uint32 Wave_Count;
        uint32 Frost_Flake_Timer;
        uint32 Storm_Pillar_Timer;

        uint32 Test_Sec_Timer;
        uint32 Test_Switch_Timer;

        uint64 TargetGUID;
        uint64 FrostflakeGUID;

        SummonList Summons;

        bool Enrage;
        bool Teleport;
        bool Focused_Assault;

        void Reset() override
        {
            if (instance)
            {
                if (instance->GetData(TYPE_BOSS_HAGARA) != DONE)
                    instance->SetData(TYPE_BOSS_HAGARA, NOT_STARTED);
            }

            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_AGGRESSIVE);

            Enrage_Timer = 480000; // 8 mins
            Phase = NORMAL_PHASE; // Start with normal phase
            Next_Phase = urand(1, 2);
            Normal_Phase_Timer = 30000;
            Focused_Assault_Timer = 15000;
            Summon_Timer = 60000;
            Check_Timer = 1000;
            Icicle_Timer = 0;
            Ice_Lance_Timer = 10000;
            Shattered_Ice_Timer = 3000;
            Ice_Tomb_Timer = 60000;
            Frost_Flake_Timer = 5000;
            Storm_Pillar_Timer = 5000;

            Frozen_Crystal_Killed = 0;
            Wave = 0;
            Wave_Count = 0;
            FrostflakeGUID = 0;

            Teleport = false;
            Focused_Assault = false;
            Enrage = false;

            Test_Sec_Timer = 1000;
            Test_Switch_Timer = 99999999;

            Summons.DespawnAll();
            DespawnLighntingStorm();
        }

        void JustSummoned(Creature* summon) override
        {
            Summons.push_back(summon->GetGUID());
        }

        void EnterCombat(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_HAGARA, IN_PROGRESS);
            }

            me->MonsterYell("You cross the Stormbinder!I'll slaughter you all.", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
            me->SendPlaySound(26227, true);

            if (Next_Phase == ICE_PHASE)
                me->CastSpell(me, HAGARA_FROST_AXES, true);
            else
                me->CastSpell(me, HAGARA_LIGHTNING_AXES, true);
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
            {
                Random_Text = urand(0, 3);
                switch (Random_Text)
                {
                    case 0:
                        me->MonsterYell("You should have run, dog.", LANG_UNIVERSAL, 0);
                        me->SendPlaySound(26255, true);
                        break;
                    case 1:
                        me->MonsterYell("Feh!", LANG_UNIVERSAL, 0);
                        me->SendPlaySound(26254, true);
                        break;
                    case 2:
                        me->MonsterYell("Down, pup.", LANG_UNIVERSAL, 0);
                        me->SendPlaySound(26256, true);
                        break;
                    case 3:
                        me->MonsterYell("A waste of my time.", LANG_UNIVERSAL, 0);
                        me->SendPlaySound(26257, true);
                        break;
                }
            }
        }

        void JustDied(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_HAGARA, DONE);
            }

            me->MonsterSay("Cowards! You pack of weakling... dogs...", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
            me->SendPlaySound(26243, true);

            Summons.DespawnAll();
            DespawnLighntingStorm();
        }

        void DoAction(const int32 param) override
        {
            switch (param)
            {
                case 0:
                {
                    switch (Frozen_Crystal_Killed)
                    {
                        case 0:
                            me->MonsterYell("You should have run, dog.", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(26255, true);
                            break;
                        case 1:
                            me->MonsterYell("Feh!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(26254, true);
                            break;
                        case 2:
                            me->MonsterYell("Down, pup.", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(26256, true);
                            break;
                        case 3:
                            me->MonsterYell("A waste of my time.", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(26257, true);
                            break;
                        default:
                            break;
                    }
                    Frozen_Crystal_Killed++;
                }
                default:
                    break;
            }
        }

        void Tele()
        {
            me->GetMotionMaster()->MoveJump(SpawnPos[4][0], SpawnPos[4][1], SpawnPos[4][2], 200.0f, 200.0f);
            me->NearTeleportTo(SpawnPos[4][0], SpawnPos[4][1], SpawnPos[4][2], SpawnPos[4][3]);

            // Stay and stop attack
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);
            me->GetMotionMaster()->MoveIdle();
            me->AttackStop();
            me->SendMovementFlagUpdate();

            Teleport = true;
        }

        void DespawnLighntingStorm()
        {
            std::list<Creature*> lightning_storm;
            me->GetCreatureListWithEntryInGrid(lightning_storm, NPC_LIGHTNING_STORM_TEST, 100.0f);
            for (std::list<Creature*>::iterator itr = lightning_storm.begin(); itr != lightning_storm.end(); ++itr)
            {
                if (*itr)
                    (*itr)->Kill((*itr));
            }
        }

        void SummonNPC(uint32 entry)
        {
            switch (entry)
            {
            case NPC_FROZEN_BINDING_CRYSTAL:
                {
                    for (uint32 i = 0; i < 4; i++)
                    {
                        me->SummonCreature(NPC_FROZEN_BINDING_CRYSTAL, SpawnPos[i][0], SpawnPos[i][1], SpawnPos[i][2], SpawnPos[i][3], TEMPSUMMON_CORPSE_DESPAWN);
                    }
                }
                break;
            case NPC_ICE_WAVE:
                {
                    float angle = me->GetOrientation();
                    double angleAddition = ((2 * M_PI) / 4); // 90 Digrees 
                    //angle = MapManager::NormalizeOrientation(angle);
                    float distance = 5.0f;

                    for (uint32 i = 0; i < 4; i++)
                    {
                        Creature * ice_wave = me->SummonCreature(NPC_ICE_WAVE, me->GetPositionX() + cos(angle)*distance, me->GetPositionY() + sin(angle)*distance, me->GetPositionZ(), angle, TEMPSUMMON_MANUAL_DESPAWN);
                        if (ice_wave)
                        {
                            ice_wave->AI()->SetData(DATA_ICE_WAVE_SLOT, Wave);
                        }
                        Wave++;
                        angle = angle + angleAddition;
                    }
                }
                break;
            case NPC_CRYSTAL_CONDUCTOR:
                {
                    float angle = me->GetOrientation();
                    double angleAddition = ((2 * M_PI) / 4); // 90 Digrees 
                    angle = MapManager::NormalizeOrientation(angle);
                    float distance, height, max;

                    max = IsHeroic() ? 8 : 4;

                    for (uint32 i = 0; i < max; i++)
                    {
                        if (i < 4)
                        {
                            distance = 32.0f;
                            height = 2.0f;
                        }
                        else
                        {
                            distance = 45.0f;
                            height = 2.5f;
                        }

                        me->SummonCreature(NPC_CRYSTAL_CONDUCTOR, me->GetPositionX() + cos(angle)*distance, me->GetPositionY() + sin(angle)*distance, me->GetPositionZ()+height, angle, TEMPSUMMON_MANUAL_DESPAWN);
                        if (i == 3)
                        {
                            angle = angle + (M_PI / 4);
                            angle = MapManager::NormalizeOrientation(angle);
                        }
                        angle = angle + angleAddition;
                    }
                }
                break;
            case NPC_COLLAPSING_ICICLE:
                {
                    float angle = (rand() % 628) / 100;
                    float distance = urand(37, 45);
                    
                    me->SummonCreature(NPC_COLLAPSING_ICICLE, me->GetPositionX() + cos(angle)*distance, me->GetPositionY() + sin(angle)*distance, me->GetPositionZ(), angle, TEMPSUMMON_TIMED_DESPAWN, 8000);
                }
                break;
            case NPC_ICE_LANCE:
                {
                    float angle = (rand() % 628) / 100;
                    float distance = 45.0f;

                    Map * map = me->GetMap();
                    if (!map)
                        return;

                    Map::PlayerList const& plrList = map->GetPlayers();
                    if (plrList.isEmpty())
                        return;

                    std::list<Player*> Ice_Lance_Targets;
                    Ice_Lance_Targets.clear();

                    for (Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
                    {
                        if (Player* pl = itr->getSource())
                        {
                            if (pl && pl->IsInWorld() && pl->IsAlive() /*&& !pl->IsGameMaster()*/)
                            {
                                Ice_Lance_Targets.push_back(pl);
                            }
                        }
                    }

                    if (Ice_Lance_Targets.size() >= 3)
                    {
                        std::list<Player*>::iterator i = Ice_Lance_Targets.begin();
                        for (uint32 j = 0; j < 3; j++)
                        {
                            advance(i, rand() % Ice_Lance_Targets.size());
                            if ((*i) && (*i)->IsInWorld())
                            {
                                float angleAddition = (rand() % 35 + 170) / 100;
                                angle += angleAddition;
                                angle = MapManager::NormalizeOrientation(angle);
                                (*i)->SummonCreature(NPC_ICE_LANCE, SpawnPos[4][0] + cos(angle)*distance, SpawnPos[4][1] + sin(angle)*distance, SpawnPos[4][2] + 2.0f, angle, TEMPSUMMON_TIMED_DESPAWN, 15000);

                                Ice_Lance_Targets.erase(i);
                                i = Ice_Lance_Targets.begin();
                            }
                        }
                    }
                    else
                    {
                        for (std::list<Player*>::iterator i = Ice_Lance_Targets.begin(); i != Ice_Lance_Targets.end(); ++i)
                        {
                            advance(i, rand() % Ice_Lance_Targets.size());
                            if (*i && (*i)->IsInWorld())
                            {
                                float angleAddition = (rand() % 35 + 170) / 100;
                                angle += angleAddition;
                                angle = MapManager::NormalizeOrientation(angle);
                                (*i)->SummonCreature(NPC_ICE_LANCE, SpawnPos[4][0] + cos(angle)*distance, SpawnPos[4][1] + sin(angle)*distance, SpawnPos[4][2] + 2.0f, angle, TEMPSUMMON_TIMED_DESPAWN, 15000);
                            }
                        }
                    }
                }
                break;
            case NPC_BOUND_LIGHNING_ELEMENTAL:
                me->SummonCreature(NPC_BOUND_LIGHNING_ELEMENTAL, SpawnPos[5][0], SpawnPos[5][1], SpawnPos[5][2], SpawnPos[5][3], TEMPSUMMON_DEAD_DESPAWN);
                break;
            default:
                break;
            }
        }

        void Check_Distance()
        {
            Map::PlayerList const &playerList = me->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
            {
                if (Player * pPlayer = i->getSource())
                {
                    if (me->GetExactDist2d(pPlayer) < 29.0)
                        pPlayer->CastSpell(pPlayer, WATERY_ENTRENCHMENT, false);
                    else
                        pPlayer->RemoveAura(WATERY_ENTRENCHMENT);

                    if (pPlayer->HasAura(FROSTFLAKE))
                    {
                        if (Aura * a = pPlayer->GetAura(FROSTFLAKE))
                        {
                            if (a->GetStackAmount() < 10)
                                a->SetStackAmount(a->GetStackAmount() + 1);
                        }
                    }
                }
            }
        }

        void SpellHitTarget(Unit* target, const SpellEntry* spell) override
        {
            if (spell->Id == ICE_TOMB_MISSILE)
            {
                if (target)
                {
                    me->CastSpell(target, ICE_TOMB_STUN, false); // Stun and dmg
                }
            }

            if (spell->Id == ICE_TOMB_STUN)
            {
                if (target)
                {
                    if (Creature * ice_tomb = target->SummonCreature(NPC_ICE_TOMB, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0.0f, TEMPSUMMON_DEAD_DESPAWN))
                        target->NearTeleportTo(ice_tomb->GetPositionX(), ice_tomb->GetPositionY(), ice_tomb->GetPositionZ(), target->GetOrientation(), false);
                }
            }
        }

        void EndSpecialPhase()
        {
            me->RemoveAllAuras();
            Summons.DespawnAll();

            Normal_Phase_Timer = 50000;
            Ice_Lance_Timer = 10000;
            Ice_Tomb_Timer = 20000;
            Teleport = false;

            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_AGGRESSIVE);
            me->GetMotionMaster()->MoveChase(me->GetVictim());
            me->Attack(me->GetVictim(), true);
            me->SendMovementFlagUpdate();

            DespawnLighntingStorm();

            Phase = NORMAL_PHASE;

            me->CastSpell(me, FEEDBACK, false);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (Summon_Timer <= diff)
            {
                switch(Phase)
                {
                case LIGHTNING_PHASE:
                    SummonNPC(NPC_CRYSTAL_CONDUCTOR);
                    SummonNPC(NPC_BOUND_LIGHNING_ELEMENTAL);
                    break;
                case ICE_PHASE:
                    SummonNPC(NPC_FROZEN_BINDING_CRYSTAL);
                    break;
                default:
                    break;
                }

                me->CastSpell(me, TELEPORT_VISUAL, true);
                Summon_Timer = 600000; // Delete this later and make it different way
            }
            else Summon_Timer -= diff;

            // NORMAL PHASE
            if (Phase == NORMAL_PHASE)
            {
                // Ice Tomb
                if (Ice_Tomb_Timer <= diff)
                {
                    uint32 max = 2;
                    if (getDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL)
                        max = 5;
                    else if (getDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC)
                        max = 6;

                    Map * map = me->GetMap();
                    if (!map)
                        return;

                    Map::PlayerList const& plrList = map->GetPlayers();
                    if (plrList.isEmpty())
                        return;

                    std::list<Player*> Ice_Tomb_Targets;
                    Ice_Tomb_Targets.clear();

                    for (Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
                    {
                        if (Player* pl = itr->getSource())
                        {
                            if (pl && pl->IsInWorld() && pl->IsAlive() && !pl->IsGameMaster())
                            {
                                Ice_Tomb_Targets.push_back(pl);
                            }
                        }
                    }

                    if (Ice_Tomb_Targets.size() >= max)
                    {
                        std::list<Player*>::iterator i = Ice_Tomb_Targets.begin();
                        for (uint32 j = 0; j < max; j++)
                        {
                            advance(i, rand() % Ice_Tomb_Targets.size());
                            if (*i && (*i)->IsInWorld())
                            {
                                me->CastSpell((*i), ICE_TOMB_MISSILE, true);
                            }

                            Ice_Tomb_Targets.erase(i);
                            i = Ice_Tomb_Targets.begin();
                        }
                    }
                    else
                    {
                        for (Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
                        {
                            if (Player* pl = itr->getSource())
                            {
                                if (pl && pl->IsInWorld() && pl->IsAlive() && !pl->IsGameMaster())
                                    me->CastSpell(pl, ICE_TOMB_MISSILE, true);
                            }
                        }
                    }

                    Random_Text = urand(0, 1);
                    switch (Random_Text)
                    {
                        case 0:
                            me->MonsterYell("Stay, pup.", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(26249, true);
                            break;
                        case 1:
                            me->MonsterYell("Hold still.", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(26250, true);
                            break;
                        default:
                            break;
                    }

                    Ice_Tomb_Timer = 20000;
                }
                else Ice_Tomb_Timer -= diff;

                // Ice Lance
                if (Ice_Lance_Timer <= diff)
                {
                    SummonNPC(NPC_ICE_LANCE);
                    Ice_Lance_Timer = 30000;

                    Random_Text = urand(0, 2);
                    switch (Random_Text) 
                    {
                        case 0:
                            me->MonsterYell("You face more than my axes, this close.", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(26244, true);
                            break;
                        case 1:
                            me->MonsterYell("See what becomes of those who stand before me!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(26245, true);
                            break;
                        case 2:
                            me->MonsterYell("Feel a chill up your spine...?", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(26246, true);
                            break;
                        default:
                            break;
                    }
                }
                else Ice_Lance_Timer -= diff;

                // Shattered Ice
                if (Shattered_Ice_Timer <= diff)
                {
                    Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true);
                    if (target)
                        me->CastSpell(target, SHATTERED_ICE_10N, false);
                    Shattered_Ice_Timer = 12000;
                }
                else Shattered_Ice_Timer -= diff;

                // Switch Phase
                if (Normal_Phase_Timer <= diff)
                {
                    if (Next_Phase == ICE_PHASE)
                    {
                        Phase = ICE_PHASE;
                        Wave = 0;
                        Wave_Count = 0;
                        Next_Phase = LIGHTNING_PHASE;
                    }
                    else if (Next_Phase == LIGHTNING_PHASE)
                    {
                        Phase = LIGHTNING_PHASE;
                        Next_Phase = ICE_PHASE;
                    }
                }
                else Normal_Phase_Timer -= diff;
            }

            // ICE PHASE
            if (Phase == ICE_PHASE)
            {
                if (!Teleport)
                {
                    Ice_Wave_Timer = 10000;
                    Tele();
                    Summon_Timer = 1000;
                    Check_Timer = 5000;
                    Frost_Flake_Timer = 5000;
                    me->CastSpell(me, FROZEN_TEMPEST_10N, false);
                }
                else
                {
                    if (Ice_Wave_Timer <= diff)
                    {
                        SummonNPC(NPC_ICE_WAVE);
                        if (Wave_Count <= 1)
                            Ice_Wave_Timer = 1000;
                        else
                            Ice_Wave_Timer = MAX_TIMER;
                        Wave_Count++;

                        if (Wave_Count == 1)
                        {
                            Random_Text = urand(0, 1);
                            switch (Random_Text) {
                            case 0:
                                me->MonsterYell("You can't outrun the storm.", LANG_UNIVERSAL, 0);
                                me->SendPlaySound(26247, true);
                                break;
                            case 1:
                                me->MonsterYell("Die beneath the ice.", LANG_UNIVERSAL, 0);
                                me->SendPlaySound(26248, true);
                                break;
                            }
                        }
                    }
                    else Ice_Wave_Timer -= diff;
                }

                if (Icicle_Timer <= diff)
                {
                    SummonNPC(NPC_COLLAPSING_ICICLE);
                    Icicle_Timer = 1000;
                }
                else Icicle_Timer -= diff;

                if (Check_Timer <= diff)
                {
                    Check_Distance();
                    Check_Timer = 1000;
                }
                else Check_Timer -= diff;

                if (IsHeroic())
                {
                    if (Frost_Flake_Timer <= diff)
                    {
                        Frost_Flake_Timer = 5000;

                        Map * map = me->GetMap();
                        if (!map)
                            return;

                        Map::PlayerList const& plrList = map->GetPlayers();
                        if (plrList.isEmpty())
                            return;

                        std::list<Player*> Frostflake_Targets;
                        Frostflake_Targets.clear();

                        for (Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
                        {
                            if (Player* pl = itr->getSource())
                            {
                                if (pl && pl->IsInWorld() && pl->IsAlive() && !pl->HasAura(FROSTFLAKE) && !pl->IsGameMaster())
                                {
                                    Frostflake_Targets.push_back(pl);
                                }
                            }
                        }

                        if (Frostflake_Targets.size() != 0)
                        {
                            std::list<Player*>::iterator i = Frostflake_Targets.begin();
                            advance(i, rand() % Frostflake_Targets.size());
                            if ((*i) && (*i)->IsInWorld())
                            {
                                me->CastSpell((*i), FROSTFLAKE, true);
                            }
                        }
                    }
                    else Frost_Flake_Timer -= diff;
                }

                // End of Ice Phase
                if ((Frozen_Crystal_Killed % 4) == 0 && Frozen_Crystal_Killed != 0)
                {
                    EndSpecialPhase();
                    Frozen_Crystal_Killed = 0;
                }
            }

            // LIGHTNING PHASE
            if (Phase == LIGHTNING_PHASE)
            {
                if (!Teleport)
                {
                    Tele();
                    Summon_Timer = 1000;

                    // Test Storm
                    Map::PlayerList const &playerList = me->GetMap()->GetPlayers();
                    for (Map::PlayerList::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
                        if (Player * pPlayer = i->getSource())
                            pPlayer->SummonCreature(NPC_LIGHTNING_STORM_TEST, pPlayer->GetPositionX(), pPlayer->GetPositionY(), pPlayer->GetPositionZ() + 30.0f, 0, TEMPSUMMON_DEAD_DESPAWN);

                    me->CastSpell(me, WATER_SHIELD_10N, false);
                    Test_Switch_Timer = 120000; // TO DO: Make proper ending - this is now for testing purpose only, phase should end when all conductors are charged
                }

                // Storm Pillar - heroic ability
                if (IsHeroic())
                {
                    if (Storm_Pillar_Timer <= diff)
                    {
                        Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true);
                        if (target)
                            me->CastSpell(target, STORM_PILLAR, true);

                        Storm_Pillar_Timer = 5000;
                    }
                    else Storm_Pillar_Timer -= diff;
                }

                // End of Lightning Phase
                if (Test_Switch_Timer <= diff)
                {
                    Test_Switch_Timer = 99999999;
                    EndSpecialPhase();
                }
                else Test_Switch_Timer -= diff;
            }

            // Enrage
            if (!Enrage)
            {
                if (Enrage_Timer <= diff)
                {
                    me->CastSpell(me, ENRAGE, false);
                    Enrage = true;
                }
                else Enrage_Timer -= diff;
            }

            // Melee attack
            DoMeleeAttackIfReady();
        }
    };
};

class npc_ice_lance : public CreatureScript
{
public:
    npc_ice_lance() : CreatureScript("npc_ice_lance") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ice_lanceAI(pCreature);
    }

    struct npc_ice_lanceAI : public ScriptedAI
    {
        npc_ice_lanceAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Dummy_Aura_Timer;
        uint32 Ice_Lance_Timer;

        uint64 summonerGuid;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);
            Dummy_Aura_Timer = 500;
            Ice_Lance_Timer = 1000;
            summonerGuid = 0;

            me->SetInCombatWithZone();
        }

        void SpellHitTarget(Unit* target, const SpellEntry* spell) override
        {
            if (spell->Id == ICE_LANCE_MISSILE)
                if (target)
                    me->CastSpell(target, ICE_LANCE_10N, true);
        }

        void IsSummonedBy(Unit* pSummoner) override
        {
            if (pSummoner)
                summonerGuid = pSummoner->GetGUID();
        }

        void UpdateAI(const uint32 diff) override
        {
            if (Dummy_Aura_Timer <= diff)
            {
                me->CastSpell(me, ICE_LANCE_DUMMY, true);
                Dummy_Aura_Timer = 16000;

                Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                    if (Player* player = i->getSource())
                    {
                        // Look for players who have Target
                        if (player->GetGUID() == summonerGuid)
                            me->CastSpell(player, TARGET, true);
                    }
            }
            else Dummy_Aura_Timer -= diff;

            if (Ice_Lance_Timer <= diff)
            {
                Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                Map::PlayerList const &PlList2 = me->GetMap()->GetPlayers();

                for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                    if (Player* player = i->getSource())
                    {
                        // Look for players who have Target
                        if (player->GetGUID() == summonerGuid)
                        {
                            for (Map::PlayerList::const_iterator i = PlList2.begin(); i != PlList2.end(); ++i)
                            {
                                if (Player* pl = i->getSource())
                                {
                                    // Find out if any player is in between Ice Lance and player with Target buff
                                    if ((pl)->IsInBetween(me, player) == true)
                                    {
                                        // Save players distances from boss and their guid
                                        Dist from_me;
                                        from_me.distance = pl->GetExactDist2d(me);
                                        from_me.guid = pl->GetGUID();
                                        dist.push_back(from_me);
                                    }
                                }
                            }

                            // If there are some players
                            if (dist.size() != 0)
                            {
                                float minimum = 500; // Player wouldn`t be farther than 500yd from boss
                                uint64 player_guid = 0;
                                for (unsigned int i = 0; i < dist.size(); i++)
                                {
                                    if (dist[i].distance < minimum)
                                    {
                                        // Find and save the smallest distance and player`s guid
                                        minimum = dist[i].distance;
                                        player_guid = dist[i].guid;
                                    }
                                }

                                for (Map::PlayerList::const_iterator i = PlList2.begin(); i != PlList2.end(); ++i)
                                {
                                    if (Player* pl = i->getSource())
                                    {
                                        // Find correct player with right guid and cast Ice Lance
                                        if (pl->GetGUID() == player_guid)
                                            me->CastSpell(pl, ICE_LANCE_MISSILE, true);
                                    }
                                }

                                dist.clear();
                            }
                            else
                                me->CastSpell(player, ICE_LANCE_MISSILE, true);
                        }
                    }
                Ice_Lance_Timer = 1000;
            }
            else Ice_Lance_Timer -= diff;
        }
    };
};


class npc_ice_tomb : public CreatureScript
{
public:
    npc_ice_tomb() : CreatureScript("npc_ice_tomb") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ice_tombAI(pCreature);
    }

    struct npc_ice_tombAI : public ScriptedAI
    {
        npc_ice_tombAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            summonerGuid = 0;
        }

        InstanceScript* instance;
        uint64 summonerGuid;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);
        }

        void IsSummonedBy(Unit* pSummoner) override
        {
            if (pSummoner)
                summonerGuid = pSummoner->GetGUID();
        }

        void JustDied(Unit* who) override
        {
            if (Player* p = ObjectAccessor::FindPlayer(summonerGuid))
                if (p->HasAura(ICE_TOMB_STUN))
                    p->RemoveAura(ICE_TOMB_STUN);
        }
    };
};

class npc_frozen_binding_crystal : public CreatureScript
{
public:
    npc_frozen_binding_crystal() : CreatureScript("npc_frozen_binding_crystal") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_frozen_binding_crystalAI(pCreature);
    }

    struct npc_frozen_binding_crystalAI : public ScriptedAI
    {
        npc_frozen_binding_crystalAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        void JustDied(Unit* /*who*/) override
        {
            Unit * hagara = me->FindNearestCreature(BOSS_HAGARA_THE_STORMBINDER, 200.0f, true);
            if (hagara)
                hagara->GetAI()->DoAction(0);
        }

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);

            Unit * hagara = me->FindNearestCreature(BOSS_HAGARA_THE_STORMBINDER, 200.0f, true);
            if (hagara)
                me->CastSpell(hagara, CRYSTALLINE_TETHER, false);
        }
    };
};

class npc_collapsing_icicle : public CreatureScript
{
public:
    npc_collapsing_icicle() : CreatureScript("npc_collapsing_icicle") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_collapsing_icicleAI(pCreature);
    }

    struct npc_collapsing_icicleAI : public ScriptedAI
    {
        npc_collapsing_icicleAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Fall_Timer;
        uint32 Shatter_Timer;
        uint32 Knockback_Timer;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, ICICLE, false);
            Shatter_Timer = 5000;
            Knockback_Timer = 7000;
        }

        void UpdateAI(const uint32 diff) override
        {
            if (Shatter_Timer <= diff)
            {
                me->CastSpell(me, 69428, false);
                Shatter_Timer = 10000;
            }
            else Shatter_Timer -= diff;

            if (Knockback_Timer <= diff)
            {
                int bp0 = 100000; // Should deal 100k dmg
                me->CastCustomSpell(me, 62457, &bp0, 0, 0, true);
                Knockback_Timer = 10000;
            }
            else Knockback_Timer -= diff;
        }
    };
};

class npc_ice_wave : public CreatureScript
{
public:
    npc_ice_wave() : CreatureScript("npc_ice_wave") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ice_waveAI(pCreature);
    }

    struct npc_ice_waveAI : public ScriptedAI
    {
        npc_ice_waveAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();

            Initialize = false;
            Slot = 0;

            Unit * hagara = me->FindNearestCreature(BOSS_HAGARA_THE_STORMBINDER, 200.0f, true);
            if (hagara)
            {
                sx = hagara->GetPositionX();
                sy = hagara->GetPositionY();
                i = hagara->GetOrientation();
            }
            me->SetSpeed(MOVE_RUN, 1.0f);
        }

        InstanceScript* instance;
        float Distance;
        float i;
        float sx;
        float sy;

        bool Initialize;

        uint32 Slot;
        uint32 MoveTimer;
        bool First_Point;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, ICE_WAVE_AURA, false);
            First_Point = false;
        }

        void SetData(uint32 Type, uint32 Data)
        {
            if (Type == DATA_ICE_WAVE_SLOT)
            {
                Slot = Data;
                Initialize = true;
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!Initialize)
                return;

            if (Initialize && !First_Point)
            {
                switch (Slot)
                {
                case 0:
                    Distance = 50.0f;
                    MoveTimer = 8000;
                    i = 0.0f;
                    break;
                case 1:
                    Distance = 50.0f;
                    MoveTimer = 8000;
                    i = M_PI / 2;
                    break;
                case 2:
                    Distance = 50.0f;
                    MoveTimer = 8000;
                    i = M_PI;
                    break;
                case 3:
                    Distance = 50.0f;
                    MoveTimer = 8000;
                    i = ((3 * M_PI) / 2);
                    break;
                case 4:
                    Distance = 42.0f;
                    MoveTimer = 7000;
                    i = 0.0f;
                    break;
                case 5:
                    Distance = 42.0f;
                    MoveTimer = 7000;
                    i = M_PI / 2;
                    break;
                case 6:
                    Distance = 42.0f;
                    MoveTimer = 7000;
                    i = M_PI;
                    break;
                case 7:
                    Distance = 42.0f;
                    MoveTimer = 7000;
                    i = ((3 * M_PI) / 2);
                    break;
                case 8:
                    Distance = 35.0f;
                    MoveTimer = 6000;
                    i = 0.0f;
                    break;
                case 9:
                    Distance = 35.0f;
                    MoveTimer = 6000;
                    i = M_PI / 2;
                    break;
                case 10:
                    Distance = 35.0f;
                    MoveTimer = 6000;
                    i = M_PI;
                    break;
                case 11:
                    Distance = 35.0f;
                    MoveTimer = 6000;
                    i = ((3 * M_PI) / 2);
                    break;
                }
                if (Slot <= 3)
                    me->GetMotionMaster()->MovePoint(0, sx + (Distance*cos(i)), sy + Distance*sin(i), me->GetPositionZ()+1);
                else 
                    me->GetMotionMaster()->MovePoint(0, sx + (Distance*cos(i)), sy + Distance*sin(i), me->GetPositionZ());
                First_Point = true;
            }

            if (MoveTimer <= diff)
            {
                switch (uint32(Distance))
                {
                case 35:
                    MoveTimer = 500;
                    me->SetSpeed(MOVE_RUN, 1.0f);
                    break;
                case 42:
                    MoveTimer = 500;
                    me->SetSpeed(MOVE_RUN, 1.2f);
                    break;
                case 50:
                    MoveTimer = 500;
                    me->SetSpeed(MOVE_RUN, 1.35f);
                    break;
                default:
                    break;
                }
                i = i - (2 * M_PI / 64);
                me->GetMotionMaster()->MovePoint(0, sx + (Distance*cos(i)), (sy + Distance*sin(i)), me->GetPositionZ());
            }
            else MoveTimer -= diff;
        }
    };
};

class npc_crystal_conductor : public CreatureScript
{
public:
    npc_crystal_conductor() : CreatureScript("npc_crystal_conductor") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_crystal_conductorAI(pCreature);
    }

    struct npc_crystal_conductorAI : public ScriptedAI
    {
        npc_crystal_conductorAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Lightning_Conduit_Timer;

        uint64 player_guid;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);

            me->CastSpell(me, LIGHTNING_ROD, false);
            Lightning_Conduit_Timer = 1000;

            player_guid = 0;

            Unit * hagara = me->FindNearestCreature(BOSS_HAGARA_THE_STORMBINDER, 200.0f, true);
            if (hagara)
                me->CastSpell(hagara, CRYSTALLINE_TETHER, false);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (me->HasAura(LIGHTNING_ROD_DUMMY))
            {
                if (Lightning_Conduit_Timer <= diff)
                {
                    Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                    for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                    {
                        if (Player* player = i->getSource())
                            player->RemoveAura(LIGHTNING_CONDUIT);
                    }

                    for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                    {
                        if (Player* player = i->getSource())
                        {
                            if (player->GetExactDist2d(me) < 10.0f && !player->HasAura(LIGHTNING_CONDUIT))
                            {
                                // Save players distances from conductor and their guid
                                Dist from_me;
                                from_me.distance = player->GetExactDist2d(me);
                                from_me.guid = player->GetGUID();
                                dist.push_back(from_me);

                                // If there are some players
                                if (dist.size() != 0)
                                {
                                    float minimum = 10; // Player can`t be further than 10yd
                                    for (unsigned int i = 0; i < dist.size(); i++)
                                    {
                                        if (dist[i].distance < minimum)
                                        {
                                            // Find and save the smallest distance and player`s guid
                                            minimum = dist[i].distance;
                                            player_guid = dist[i].guid;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (Player* p = ObjectAccessor::FindPlayer(player_guid))
                        me->CastSpell(p, LIGHTNING_CONDUIT, true);

                    player_guid = 0;
                    dist.clear();
                    Lightning_Conduit_Timer = 5000;
                }
                else Lightning_Conduit_Timer -= diff;
            }
        }
    };
};

class npc_bound_lightning_elemental : public CreatureScript
{
public:
    npc_bound_lightning_elemental() : CreatureScript("npc_bound_lightning_elemental") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_bound_lightning_elementalAI(pCreature);
    }

    struct npc_bound_lightning_elementalAI : public ScriptedAI
    {
        npc_bound_lightning_elementalAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Dummy_Aura_Timer;

        void JustDied(Unit* /*who*/) override
        {
            Unit * conductor = me->FindNearestCreature(NPC_CRYSTAL_CONDUCTOR, 10.0f, true);
            if (conductor)
                conductor->CastSpell(conductor, LIGHTNING_ROD_DUMMY, false);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_test_lightning_storm : public CreatureScript
{
public:
    npc_test_lightning_storm() : CreatureScript("npc_test_lightning_storm") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_test_lightning_stormAI(pCreature);
    }

    struct npc_test_lightning_stormAI : public ScriptedAI
    {
        npc_test_lightning_stormAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            summonerGuid = 0;
        }

        InstanceScript* instance;
        uint64 summonerGuid;
        uint32 Repeat_Timer;
        uint32 Move_Timer;
        uint32 Unaura_Timer;

        void IsSummonedBy(Unit* pSummoner) override
        {
            if (pSummoner)
            {
                summonerGuid = pSummoner->GetGUID();
            }
        }

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);

            Repeat_Timer = 4000;
            Unaura_Timer = 5000;
            Move_Timer = 500;
        }

        void UpdateAI(const uint32 diff) override
        {
            if (Repeat_Timer <= diff)
            {
                if (Player* p = ObjectAccessor::FindPlayer(summonerGuid))
                    if (!p->HasAura(LIGHTNING_CONDUIT))
                        me->CastSpell(p, 105466, false);

                Repeat_Timer = 2100;
                Unaura_Timer = 1000;
            }
            else Repeat_Timer -= diff;

            if (Unaura_Timer <= diff)
            {
                if (Player* p = ObjectAccessor::FindPlayer(summonerGuid))
                    p->RemoveAura(105466);
                Unaura_Timer = 2100;
            }
            else Unaura_Timer -= diff;

            if (Move_Timer <= diff)
            {
                if (Player* p = ObjectAccessor::FindPlayer(summonerGuid))
                    me->GetMotionMaster()->MovePoint(0, p->GetPositionX(), p->GetPositionY(), p->GetPositionZ() + 30, true);
                Move_Timer = 500;
            }
            else Move_Timer -= diff;
        }
    };
};

class npc_hagara_frost_trap : public CreatureScript
{
public:
    npc_hagara_frost_trap() : CreatureScript("npc_hagara_frost_trap") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_hagara_frost_trapAI(pCreature);
    }

    struct npc_hagara_frost_trapAI : public ScriptedAI
    {
        npc_hagara_frost_trapAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);
        }
    };
};

void AddSC_boss_hagara_the_stormbinder()
{
    // Boss
    new boss_hagara_the_stormbinder();

    // Normal Phase
    new npc_ice_lance();
    new npc_ice_tomb();

    // Lightning Phase
    new npc_crystal_conductor();
    new npc_bound_lightning_elemental();
    new npc_test_lightning_storm();
    //new spell_lightning_conduit();

    // Ice Phase
    new npc_frozen_binding_crystal();
    new npc_ice_wave();
    new npc_collapsing_icicle();
    new npc_hagara_frost_trap();
}

//////////////////////////////////////////////////////////////////////////////////////
///////////////////////////         SQL QUERY          ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////