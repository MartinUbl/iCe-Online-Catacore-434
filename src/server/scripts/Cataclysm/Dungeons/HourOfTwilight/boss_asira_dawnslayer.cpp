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
Encounter: Asira Dawnslayer
Dungeon: Hour of Twilight
Difficulty: Heroic
Mode: 5-man
Autor: Lazik
*/

#include "ScriptPCH.h"
#include "Spell.h"
#include "UnitAI.h"
#include "MapManager.h"
#include "hour_of_twilight.h"

enum NPC
{
    BOSS_ASIRA_DAWNSLAYER         =  54968,
    NPC_ASIRA_DAWNSLAYER_INTRO    = 119510,
    NPC_LIFE_WARDEN               =  55415,
    NPC_CHOKING_SMOKE_BOMB        = 119512,
};

// Spells
enum Spells
{
    THROW_KNIFE                = 103597,
    MARK_OF_SILENCE            = 102726,
    SILENCED                   = 103587,

    BLADE_BARRIER              = 103419,
    LESSER_BLADE_BARRIER       = 103562,
    DMG_REDUCTION              = 101386,

    CHOKING_SMOKE_BOMB         = 103558,
    CHOKING_SMOKE_BOMB_1       = 103790,

    AMBUSH                     = 103646,
};

const float AsiraMovePoints[8][4] =
{
    // Asira Intro Movement Points
    { 4233.86f, 501.51f, 47.0284f, 0.312f }, // 0 - 1st fly point
    { 4260.40f, 504.67f, 30.8544f, 1.280f }, // 1 - 2nd fly point
    { 4273.53f, 531.89f, 23.3096f, 1.453f }, // 2 - 3rd fly point
    { 4274.45f, 563.30f, 16.2441f, 1.476f }, // 3 - 4th fly point
    { 4284.85f, 601.66f,  8.1159f, 1.041f }, // 4 - 5th fly point
    { 4284.91f, 601.66f, -6.7400f, 1.233f }, // 5 - Death point
    { 4250.12f, 569.78f, -6.5263f, 0.086f }, // Asira Landing Point
    { 4251.84f, 570.08f, -6.5255f, 6.142f }, // Asira boss spawn location
};

struct Distance
{
    uint64 guid;
    float distance;
};

std::vector<Distance> distance;

// Asira Dawnslayer
class boss_asira_dawnslayer : public CreatureScript
{
public:
    boss_asira_dawnslayer() : CreatureScript("boss_asira_dawnslayer") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_asira_dawnslayerAI(pCreature);
    }

    struct boss_asira_dawnslayerAI : public ScriptedAI
    {
        boss_asira_dawnslayerAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();

            me->SetVisible(false);
            me->setFaction(35);
        }

        InstanceScript* instance;
        uint32 Choking_Smoke_Bomb_Timer;
        uint32 Cast_Check_Timer;
        int Random_Text;
        bool bladesUsed;

        void Reset()
        {
            if (instance)
            {
                if (instance->GetData(TYPE_BOSS_ASIRA_DAWNSLAYER) != DONE)
                    instance->SetData(TYPE_BOSS_ASIRA_DAWNSLAYER, NOT_STARTED);
            }

            me->setPowerType(POWER_ENERGY);
            me->SetMaxPower(POWER_ENERGY, 100);
            me->SetPower(POWER_ENERGY, 100);

            // Remove Mark of Silence
            Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                if (Player* player = i->getSource())
                {
                    if (player->HasAura(MARK_OF_SILENCE))
                        player->RemoveAura(MARK_OF_SILENCE);
                }

            me->MonsterYell("Ah. Well. That was even easier than I thought!", LANG_UNIVERSAL, 0, 30.0f);
            me->SendPlaySound(25820, true);

            Choking_Smoke_Bomb_Timer = 16000;
            Cast_Check_Timer = 1000;
            bladesUsed = false;
        }

        void EnterCombat(Unit * /*who*/)
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_ASIRA_DAWNSLAYER, IN_PROGRESS);
            }

            // Add Mark of Silence on spellcasters
            Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                if (Player* player = i->getSource())
                {
                    switch (player->GetActiveTalentBranchSpec())
                    {
                    case SPEC_DRUID_BALANCE:
                    case SPEC_DRUID_RESTORATION:
                    case SPEC_MAGE_ARCANE:
                    case SPEC_MAGE_FIRE:
                    case SPEC_MAGE_FROST:
                    case SPEC_PALADIN_HOLY:
                    case SPEC_PRIEST_DISCIPLINE:
                    case SPEC_PRIEST_HOLY:
                    case SPEC_PRIEST_SHADOW:
                    case SPEC_SHAMAN_ELEMENTAL:
                    case SPEC_SHAMAN_RESTORATION:
                    case SPEC_WARLOCK_AFFLICTION:
                    case SPEC_WARLOCK_DEMONOLOGY:
                    case SPEC_WARLOCK_DESTRUCTION:
                        me->CastSpell(player, MARK_OF_SILENCE, true);
                        break;
                    default:
                        break;
                    }
                }

            me->MonsterYell("Let's get to work, shall we?", LANG_UNIVERSAL, 0);
            me->SendPlaySound(25816, true);
        }

        void KilledUnit(Unit* /*victim*/)
        {
            Random_Text = urand(0, 3);
            switch (Random_Text) {
            case 0:
                me->MonsterYell("Ah, so soon", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25821, true);
                break;
            case 1:
                me->MonsterYell("I hope your friends can do better.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25822, true);
                break;
            case 2:
                me->MonsterYell("Mmm, too much fun!", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25823, true);
                break;
            case 3:
                me->MonsterYell("Good night...", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25824, true);
                break;
            }
        }

        void JustDied(Unit* /*who*/)
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_ASIRA_DAWNSLAYER, DONE);
            }

            // Remove Mark of Silence
            Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                if (Player* player = i->getSource())
                {
                    if (player->HasAura(MARK_OF_SILENCE))
                        player->RemoveAura(MARK_OF_SILENCE);
                }

            me->MonsterYell("You're... much better... than I thought...", LANG_UNIVERSAL, 0);
            me->SendPlaySound(25817, true);
        }

        void SpellHitTarget(Unit* target, const SpellEntry* spell)
        {
            if (spell->Id == THROW_KNIFE)
            {
                if (target && target->HasAura(MARK_OF_SILENCE) && target->HasUnitState(UNIT_STATE_CASTING))
                    me->CastSpell(target, SILENCED, true);
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (!bladesUsed && me->HealthBelowPct(30))
            {
                bladesUsed = true;
                me->CastSpell(me, BLADE_BARRIER, false);
            }

            if (Choking_Smoke_Bomb_Timer <= diff)
            {
                me->MonsterYell("Surprise.", LANG_UNIVERSAL, 0);
                me->PlayDirectSound(25827, nullptr);

                me->CastSpell(me, CHOKING_SMOKE_BOMB, false);
                me->SummonCreature(NPC_CHOKING_SMOKE_BOMB, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 20000);
                Choking_Smoke_Bomb_Timer = 24000;
            }
            else Choking_Smoke_Bomb_Timer -= diff;

            if (Cast_Check_Timer <= diff)
            {
                Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                Map::PlayerList const &PlList2 = me->GetMap()->GetPlayers();

                for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                    if (Player* player = i->getSource())
                    {
                        // Look for players who have Mark of Silence and are casting
                        if (player->HasAura(MARK_OF_SILENCE) && player->HasUnitState(UNIT_STATE_CASTING))
                        {
                            for (Map::PlayerList::const_iterator i = PlList2.begin(); i != PlList2.end(); ++i)
                            {
                                if (Player* pl = i->getSource())
                                {
                                    // Find out if any player is in between boss and player with Mark of Silence
                                    if ((pl)->IsInBetween(me, player) == true)
                                    {
                                        // Save players distances from boss and their guid
                                        Distance from_boss;
                                        from_boss.distance = pl->GetExactDist2d(me);
                                        from_boss.guid = pl->GetGUID();
                                        distance.push_back(from_boss);
                                    }
                                }
                            }

                            // If there are some players
                            if (distance.size() != 0)
                            {
                                float minimum = 500; // Player wouldn`t be farther than 500yd from boss
                                uint64 player_guid = 0;
                                for (unsigned int i = 0; i < distance.size(); i++)
                                    if (distance[i].distance < minimum)
                                    {
                                        // Find and save the smallest distance and player`s guid
                                        minimum = distance[i].distance;
                                        player_guid = distance[i].guid;
                                    }

                                for (Map::PlayerList::const_iterator i = PlList2.begin(); i != PlList2.end(); ++i)
                                    if (Player* pl = i->getSource())
                                    {
                                        // Find correct player with right guid and cast Throw Knife
                                        if (pl->GetGUID() == player_guid)
                                            me->CastSpell(pl, THROW_KNIFE, true);
                                    }

                                distance.clear();
                            }
                            else
                                me->CastSpell(player, THROW_KNIFE, true);
                        }
                    }
                Cast_Check_Timer = 500;
            }
            else Cast_Check_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

// Npc Asira Dawnslayer Intro - 119510
class npc_asira_dawnslayer_intro : public CreatureScript
{
public:
    npc_asira_dawnslayer_intro() : CreatureScript("npc_asira_dawnslayer_intro") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_asira_dawnslayer_introAI(pCreature);
    }

    struct npc_asira_dawnslayer_introAI : public ScriptedAI
    {
        npc_asira_dawnslayer_introAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Next_Say_Timer;
        uint32 Jump_Timer;
        uint32 Disappear_Timer;
        int Say_Text;
        bool Jump;
        bool Move;
        bool Next_Say;
        bool Disappear;

        void JustDied(Unit* /*who*/) { }

        void Reset()
        {
            me->setPowerType(POWER_ENERGY);
            me->SetMaxPower(POWER_ENERGY, 100);
            me->SetPower(POWER_ENERGY, 100);
            Next_Say_Timer = 150;
            Jump_Timer = 1000;
            Disappear_Timer = 0;
            Say_Text = 0;
            Next_Say = true;
            Jump = false;
            Move = false;
            Disappear = false;
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);

            me->GetMotionMaster()->MoveJump(AsiraMovePoints[5][0], AsiraMovePoints[5][1], AsiraMovePoints[5][2], 10.0f, 10.0f);
        }

        void UpdateAI(const uint32 diff)
        {
            if (Jump_Timer <= diff)
            {
                if (!Jump)
                {
                    me->GetMotionMaster()->MoveJump(AsiraMovePoints[6][0], AsiraMovePoints[6][1], AsiraMovePoints[6][2], 30.0f, 20.0f);
                    Jump = true;
                }
            }
            else Jump_Timer -= diff;

            if (Move == false)
            {
                if (me->GetExactDist2d(AsiraMovePoints[6][0], AsiraMovePoints[6][1]) < 1)
                {
                    me->GetMotionMaster()->MovePoint(0, AsiraMovePoints[7][0], AsiraMovePoints[7][1], AsiraMovePoints[7][2]);
                    Move = true;
                }
            }

            if (Next_Say)
            {
                if (Next_Say_Timer <= diff)
                {
                    switch (Say_Text)
                    {
                    case 0:
                        me->MonsterYell("Where do you think you're going, little lizard?", LANG_UNIVERSAL, 0);
                        me->PlayDirectSound(25818, nullptr);
                        Next_Say_Timer = 3500;
                        Say_Text++;
                        break;
                    case 1:
                        me->MonsterYell("...and with that out of the way, you and your flock of fumbling friends are next on my list. Mmm, I thought you'd never get here!", LANG_UNIVERSAL, 0);
                        me->SendPlaySound(25819, false);
                        Next_Say = false;
                        Disappear_Timer = 10000;
                        Disappear = true;
                        break;
                    }
                }
                else Next_Say_Timer -= diff;
            }

            if (Disappear)
            {
                if (Disappear_Timer <= diff)
                {
                    Creature * asira = me->FindNearestCreature(BOSS_ASIRA_DAWNSLAYER, 10.0f, true);
                    if (asira)
                    {
                        me->SetVisible(false);
                        me->setFaction(35);
                        asira->SetVisible(true);
                        asira->setFaction(16);
                        Disappear = false;
                        instance->SetData(DATA_ASIRA_INTRO, 3); // 3
                    }
                }
                else Disappear_Timer -= diff;
            }
        }
    };
};

// Npc Life Warden - 55415
class npc_life_warden : public CreatureScript
{
public:
    npc_life_warden() : CreatureScript("npc_life_warden") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_life_wardenAI(pCreature);
    }

    struct npc_life_wardenAI : public ScriptedAI
    {
        npc_life_wardenAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Move_Timer;
        int Move_Point;

        void JustDied(Unit* /*who*/) { }

        void Reset()
        {
            me->SetVisible(false);
            Move_Point = 0;
            Move_Timer = 0;

            me->SetFlying(true);
        }

        void UpdateAI(const uint32 diff)
        {
            if (instance->GetData(DATA_ASIRA_INTRO) == 1)
            {
                if (Move_Timer <= diff)
                {
                    Move_Timer = 1000;

                    switch (Move_Point)
                    {
                    case 0:
                        me->GetMotionMaster()->MovePoint(0, AsiraMovePoints[Move_Point][0], AsiraMovePoints[Move_Point][1], AsiraMovePoints[Move_Point][2]);
                        me->SetVisible(true);
                        me->CastSpell(me, AMBUSH, false);
                        Move_Timer = 3500;
                        Move_Point++;
                        break;
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                        me->CastSpell(me, AMBUSH, false);
                        me->GetMotionMaster()->MovePoint(0, AsiraMovePoints[Move_Point][0], AsiraMovePoints[Move_Point][1], AsiraMovePoints[Move_Point][2]);
                        if (Move_Point == 4)
                            Move_Timer = 5500;
                        else Move_Timer = 3500;
                        Move_Point++;
                        break;
                    case 5:
                        me->SummonCreature(NPC_ASIRA_DAWNSLAYER_INTRO, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN);
                        me->RemoveAura(AMBUSH);
                        me->SetFlying(false);
                        me->GetMotionMaster()->MoveJump(AsiraMovePoints[Move_Point][0], AsiraMovePoints[Move_Point][1], AsiraMovePoints[Move_Point][2], 10.0f, 10.0f);
                        Move_Timer = 2000;
                        Move_Point++;
                        break;
                    case 6:
                        instance->SetData(DATA_ASIRA_INTRO, 2); // 2
                        me->Kill(me);
                        Move_Point++;
                        break;
                    default:
                        break;
                    }
                }
                else Move_Timer -= diff;
            }
        }
    };
};

// Npc Choking Smoke Bomb - 119512
class npc_choking_smoke_bomb : public CreatureScript
{
public:
    npc_choking_smoke_bomb() : CreatureScript("npc_choking_smoke_bomb") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_choking_smoke_bombAI(pCreature);
    }

    struct npc_choking_smoke_bombAI : public ScriptedAI
    {
        npc_choking_smoke_bombAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Cast_Timer;

        void JustDied(Unit* /*who*/) { }

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            Cast_Timer = 500;
        }

        void UpdateAI(const uint32 diff)
        {
            if (Cast_Timer <= diff)
            {
                me->CastSpell(me, CHOKING_SMOKE_BOMB_1, false);
                Cast_Timer = 500;
            }
            else Cast_Timer -= diff;
        }
    };
};

class spell_gen_blade_barrier_hot : public SpellScriptLoader
{
    public: spell_gen_blade_barrier_hot() : SpellScriptLoader("spell_gen_blade_barrier_hot") { }

    class spell_gen_blade_barrier_hot_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_gen_blade_barrier_hot_AuraScript);

        void OnAbsorb(AuraEffect * aurEff, DamageInfo & dmgInfo, uint32 & absorbAmount)
        {
            uint32 damage = dmgInfo.GetDamage();

            if (damage > (uint32)GetSpellProto()->EffectBasePoints[EFFECT_0])
                aurEff->GetBase()->Remove();
            else
                absorbAmount = damage;
        }

        void OnRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetSpellProto()->Id == BLADE_BARRIER)
                GetCaster()->CastSpell(GetCaster(), GetSpellProto()->EffectBasePoints[EFFECT_1], true);
        }

        void Register() override
        {
            OnEffectAbsorb += AuraEffectAbsorbFn(spell_gen_blade_barrier_hot_AuraScript::OnAbsorb, EFFECT_0);
            OnEffectRemove += AuraEffectRemoveFn(spell_gen_blade_barrier_hot_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript *GetAuraScript() const override
    {
        return new spell_gen_blade_barrier_hot_AuraScript();
    }
};

void AddSC_boss_asira_dawnslayer()
{
    new boss_asira_dawnslayer();
    new npc_life_warden();
    new npc_asira_dawnslayer_intro();
    new npc_choking_smoke_bomb();
    new spell_gen_blade_barrier_hot();
}

/*
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (103562 , 'spell_gen_blade_barrier_hot');
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (103419 , 'spell_gen_blade_barrier_hot');
*/