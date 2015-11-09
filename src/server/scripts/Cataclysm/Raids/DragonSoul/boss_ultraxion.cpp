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
Encounter: Ultraxion
Dungeon: Dragon Soul
Difficulty: Normal / Heroic
Mode: 10-man / 25-man
Autor: Lazik
*/

/*
26323 - The final shred of light fades, and with it, your pitiful mortal existence!
---     26324,10,"VO_DS_ULTRAXION_SPELL_02
26325 - Through the pain and fire my hatred burns! SPELL_03
*/

#include "ScriptPCH.h"
#include "dragonsoul.h"
#include "TaskScheduler.h"

#define WAYPOINT_X (-1690.0f)
#define WAYPOINT_Y (-2384.0f)
#define WAYPOINT_Z (358.0f)

// NPCs
enum NPC
{
    BOSS_ULTRAXION                  = 55294,
    NPC_ULTRAXION_GAUNTLET          = 56305,

    NPC_GO_GIFT_OF_LIFE_SPAWN       = 119550,
    NPC_GO_ESSENCE_OF_DREAMS_SPAWN  = 119551,
    NPC_GO_SOURCE_OF_MAGIC_SPAWN    = 119552,
};

enum Gameobjescts
{
    GO_GIFT_OF_LIFE                 = 209873,
    GO_ESSENCE_OF_DREAMS            = 209874,
    GO_SOURCE_OF_MAGIC              = 209875,

    GO_LESSER_CACHE_OF_ASPECTS_10N  = 210160,
    GO_LESSER_CACHE_OF_ASPECTS_25N  = 210161,
    GO_LESSER_CACHE_OF_ASPECTS_10HC = 210162,
    GO_LESSER_CACHE_OF_ASPECTS_25HC = 210163,
};

// Spells
enum Spells
{
    // REUSE                        = 109192,
    ULTRAXION_NORM_REALM_COSMETIC   = 105929,
    ULTRAXION_COSMETIC              = 105211,
    ULTRAXION_COSMETIC_1            = 105214,

    ULTRAXION_ACHIEVEMENT_AURA      = 109188,
    ULTRAXION_AHCIEVEMENT_FAILED    = 109194,

    UNSTABLE_MONSTROSITY_6S         = 106372, // 6s trigger aura - 109176 / 106375
    UNSTABLE_MONSTROSITY_5S         = 106376, // 5s trigger aura
    UNSTABLE_MONSTROSITY_4S         = 106377, // 4s trigger aura
    UNSTABLE_MONSTROSITY_3S         = 106378, // 3s trigger aura
    UNSTABLE_MONSTROSITY_2S         = 106379, // 2s trigger aura
    UNSTABLE_MONSTROSITY_1S         = 106380, // 1s trigger aura
    UNSTABLE_MONSTROSITY_DUNNO      = 106390, // Prevents from casting these auras again for 4s
    UNSTABLE_MONSTROSITY_10N_EX     = 106381, // ? 8/6s normal/heroic
    UNSTABLE_MONSTROSITY_25N_EX     = 109418,
    UNSTABLE_MONSTROSITY_10HC_EX    = 109419,
    UNSTABLE_MONSTROSITY_25HC_EX    = 109420,

    TWILIGHT_INSTABILITY_DUMMY      = 106374, // Dummy effect
    TWILIGHT_INSTABILITY            = 109176, // Spell effect script effect?
    TWILIGHT_INSTABILITY_10N        = 106375, // 300 000 dmg 10N
    TWILIGHT_INSTABILITY_25N        = 109182, // 825 000 dmg 25N
    TWILIGHT_INSTABILITY_10HC       = 109183, // 400 000 dmg 10HC
    TWILIGHT_INSTABILITY_25HC       = 109184, // 1 100 000 dmg 25HC

    GROWING_INSTABILITY             = 106373,
    MAXIMUM_INSTABILITY             = 106395,

    TWILIGHT_SHIFT_COSMETIC_EFFECT  = 106368, // Twilight Phase - cosmetic visual
    TWILIGHT_SHIFT                  = 106369, // Force cast 106368

    HEROIC_WILL_ACTION_BUTTON       = 105554,
    HEROIC_WILL                     = 106108,
    HEROIC_WILL_COSMETIC_EFFECT     = 106175,

    FADING_LIGHT_TANK_10N           = 105925, // from boss to player, triggered by hour of twilight, tank only
    FADING_LIGHT_TANK_25N           = 110070,
    FADING_LIGHT_TANK_10HC          = 110069,
    FADING_LIGHT_TANK_25HC          = 110068,
    FADING_LIGHT_KILL_10N           = 105926, // kill player
    FADING_LIGHT_KILL_25N           = 110075,
    FADING_LIGHT_KILL_10HC          = 110074,
    FADING_LIGHT_KILL_25HC          = 110073,
    FADING_LIGHT_DPS_10N            = 109075, // from boss, triggered by 105925, dps
    FADING_LIGHT_DPS_25N            = 110080,
    FADING_LIGHT_DPS_10HC           = 110079,
    FADING_LIGHT_DPS_25HC           = 110078,
    FADING_LIGHT_DUMMY              = 109200, // dummy

    FADED_INTO_TWILIGHT_10N         = 105927,
    FADED_INTO_TWILIGHT_25N         = 109461,
    FADED_INTO_TWILIGHT_10HC        = 109462,
    FADED_INTO_TWILIGHT_25HC        = 109463,

    HOUR_OF_TWILIGHT_DMG            = 103327, // dmg + forse cast 109231 (Looming Darkness), force cast 106370
    HOUR_OF_TWILIGHT_REMOVE_WILL    = 106174, // remove heroic will
    HOUR_OF_TWILIGHT_ACHIEV_10N     = 106370, // from player, force cast achievement
    HOUR_OF_TWILIGHT_ACHIEV_25N     = 109172,
    HOUR_OF_TWILIGHT_ACHIEV_10HC    = 109173,
    HOUR_OF_TWILIGHT_ACHIEV_25HC    = 109174,
    HOUR_OF_TWILIGHT_10N            = 106371, // Eff1: 106174, Eff2: 103327, Eff3: 105925 (Fading Light)
    HOUR_OF_TWILIGHT_25N            = 109415,
    HOUR_OF_TWILIGHT_10HC           = 109416,
    HOUR_OF_TWILIGHT_25HC           = 109417,
    HOUR_OF_TWILIGHT_UNKNWON        = 106389, // Eff1: Twilight Shift, Eff2: 103327 HoT, Eff3: Dummy
    HOUR_OF_TWILIGHT_LAUNCH         = 109323, // Some Quest start? WTF

    TWILIGHT_BURST                  = 106415,

    TWILIGHT_ERUPTION               = 106388, // Berserk - kill all players

    LOOMING_DARKNESS_DUMMY          = 106498,
    LOOMING_DARKNESS_DMG            = 109231,

    // Alexstrasza
    GIFT_OF_LIFE_AURA               = 105896,
    GIFT_OF_LIFE_SUMMON_1           = 106044,
    GIFT_OF_LIFE_SUMMON_2           = 109340,
    GIFT_OF_LIFE_MISSILE_10N        = 106042,
    GIFT_OF_LIFE_MISSILE_25N        = 109349,
    GIFT_OF_LIFE_MISSILE_10HC       = 109350,
    GIFT_OF_LIFE_MISSILE_25HC       = 109351,
    GIFT_OF_LIFE_4                  = 109345, // triggered by 106042 in 25 ppl

    // Ysera
    ESSENCE_OF_DREAMS_AURA          = 105900,
    ESSENCE_OF_DREAMS_HEAL          = 105996,
    ESSENCE_OF_DREAMS_SUMMON_1      = 106047,
    ESSENCE_OF_DREAMS_SUMMON_2      = 109342,
    ESSENCE_OF_DREAMS_MISSILE_10N   = 106049,
    ESSENCE_OF_DREAMS_MISSILE_25N   = 109356,
    ESSENCE_OF_DREAMS_MISSILE_10HC  = 109357,
    ESSENCE_OF_DREAMS_MISSILE_25HC  = 109358,
    ESSENCE_OF_DREAMS_5             = 109344, // triggered by 106049 in 25 ppl

    // Kalecgos
    SOURCE_OF_MAGIC_AURA            = 105903,
    SOURCE_OF_MAGIC_SUMMON_1        = 106048,
    SOURCE_OF_MAGIC_SUMMON_2        = 109346,
    SOURCE_OF_MAGIC_MISSILE_10N     = 106050,
    SOURCE_OF_MAGIC_MISSILE_25N     = 109353,
    SOURCE_OF_MAGIC_MISSILE_10HC    = 109354,
    SOURCE_OF_MAGIC_MISSILE_25HC    = 109355,
    SOURCE_OF_MAGIC_4               = 109347, // triggered by 106050 in 25 ppl

    // Nozdormu
    TIMELOOP                        = 105984,
    TIMELOOP_HEAL                   = 105992,

    // Thrall
    LAST_DEFENDER_OF_AZEROTH        = 106182, // scale + force cast 110327
    LAST_DEFENDER_OF_AZEROTH_SCRIPT = 106218,
    LAST_DEFENDER_OF_AZEROTH_DUMMY  = 110327,
    LAST_DEFENDER_OF_AZEROTH_WARR   = 106080,
    LAST_DEFENDER_OF_AZEROTH_DRUID  = 106224,
    LAST_DEFENDER_OF_AZEROTH_PALA   = 106226,
    LAST_DEFENDER_OF_AZEROTH_DK     = 106227,
};

const Position cacheSpawn = { -1753.67f, -2368.67f, 340.84f, 4.788f };

// Ultraxion
class boss_ultraxion : public CreatureScript
{
public:
    boss_ultraxion() : CreatureScript("boss_ultraxion") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_ultraxionAI(pCreature);
    }

    struct boss_ultraxionAI : public ScriptedAI
    {
        boss_ultraxionAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        TaskScheduler scheduler;
        InstanceScript* instance;
        uint32 unstableMonstrosityTimer;
        uint32 unstableMonstrosityCount;
        uint32 hourOfTwilightTimer;

        void RemoveEncounterAuras()
        {
            if (instance)
            {
                instance->DoRemoveAurasDueToSpellOnPlayers(TWILIGHT_SHIFT);
                instance->DoRemoveAurasDueToSpellOnPlayers(TWILIGHT_SHIFT_COSMETIC_EFFECT);
                instance->DoRemoveAurasDueToSpellOnPlayers(HEROIC_WILL_ACTION_BUTTON);
                instance->DoRemoveAurasDueToSpellOnPlayers(GIFT_OF_LIFE_AURA);
                instance->DoRemoveAurasDueToSpellOnPlayers(ESSENCE_OF_DREAMS_AURA);
                instance->DoRemoveAurasDueToSpellOnPlayers(SOURCE_OF_MAGIC_AURA);
                instance->DoRemoveAurasDueToSpellOnPlayers(TIMELOOP);
                instance->DoRemoveAurasDueToSpellOnPlayers(LAST_DEFENDER_OF_AZEROTH_DK);
                instance->DoRemoveAurasDueToSpellOnPlayers(LAST_DEFENDER_OF_AZEROTH_PALA);
                instance->DoRemoveAurasDueToSpellOnPlayers(LAST_DEFENDER_OF_AZEROTH_DRUID);
                instance->DoRemoveAurasDueToSpellOnPlayers(LAST_DEFENDER_OF_AZEROTH_WARR);
            }
        }

        void Reset() override
        {
            if (instance)
            {
                if (instance->GetData(TYPE_BOSS_ULTRAXION) != DONE)
                    instance->SetData(TYPE_BOSS_ULTRAXION, NOT_STARTED);
            }

            me->SetVisible(false);
            me->SetFlying(true);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);

            unstableMonstrosityTimer = 60000;
            unstableMonstrosityCount = 0;
            hourOfTwilightTimer = 45000;

            RemoveEncounterAuras();
            if (GameObject* pGoGiftOfLife = me->FindNearestGameObject(GO_GIFT_OF_LIFE, 100.0f))
                pGoGiftOfLife->Delete();
            if (GameObject* pGoEssenceOfMagic = me->FindNearestGameObject(GO_ESSENCE_OF_DREAMS, 100.0f))
                pGoEssenceOfMagic->Delete();
            if (GameObject* pGoSourceOfMagic = me->FindNearestGameObject(GO_SOURCE_OF_MAGIC, 100.0f))
                pGoSourceOfMagic->Delete();

            scheduler.CancelAll();
        }

        void DoAction(const int32 action) override
        {
            switch (action)
            {
            case DATA_SUMMON_ULTRAXION:
                me->SetVisible(true);
                me->SetSpeed(MOVE_FLIGHT, 1.8f);
                me->GetMotionMaster()->MovePoint(0, WAYPOINT_X, WAYPOINT_Y, WAYPOINT_Z, false, true);
                me->MonsterYell("I am the beginning of the end...the shadow which blots out the sun...the bell which tolls your doom...", LANG_UNIVERSAL, 0, 300.0f);
                me->SendPlaySound(26317, false);
                scheduler.Schedule(Seconds(13), [this](TaskContext /*2nd Quote + Phase Switch*/)
                {
                    SwitchPlayersToTwilightPhase();
                    if (instance)
                        instance->SetData(DATA_ULTRAXION_DRAKES, 16);
                    me->MonsterYell("For this moment ALONE was I made. Look upon your death, mortals, and despair!", LANG_UNIVERSAL, 0, 300.0f);
                    me->SendPlaySound(26318, false);
                });
                scheduler.Schedule(Seconds(30), [this](TaskContext /*Remove Flags*/)
                {
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    me->SetReactState(REACT_AGGRESSIVE);
                });
                break;
            default:
                break;
            }
        }

        void SwitchPlayersToTwilightPhase()
        {
            Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
            {
                if (Player* player = i->getSource())
                {
                    if (!player->IsGameMaster())
                    {
                        player->CastSpell(player, TWILIGHT_SHIFT, true);
                        player->CastSpell(player, HEROIC_WILL_ACTION_BUTTON, true);
                    }
                }
            }
        }

        void EnterCombat(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_ULTRAXION, IN_PROGRESS);
            }

            me->MonsterYell("Now is the hour of twilight!", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
            me->SendPlaySound(26314, true);

            me->CastSpell(me, UNSTABLE_MONSTROSITY_6S, false);

            // Take all players to twilight phase
            SwitchPlayersToTwilightPhase();

            // Last Defender of Azeroth
            scheduler.Schedule(Seconds(5), [this](TaskContext /*context*/)
            {
                if (Creature * pThrall = me->FindNearestCreature(NPC_THRALL, 100.0f, true))
                    pThrall->GetAI()->DoAction(DATA_HELP_AGAINST_ULTRAXION);
            });
            // Alexstrasza - Gift of Life
            scheduler.Schedule(Seconds(80), [this](TaskContext /*context*/)
            {
                if (Creature* pAlexstrasza = me->FindNearestCreature(NPC_ALEXSTRASZA_THE_LIFE_BINDER, 100.0f))
                    pAlexstrasza->GetAI()->DoAction(DATA_HELP_AGAINST_ULTRAXION);
            });
            // Ysera - Essence of Dreams
            scheduler.Schedule(Seconds(155), [this](TaskContext /*context*/)
            {
                if (Creature* pYsera = me->FindNearestCreature(NPC_YSERA_THE_AWAKENED, 100.0f))
                    pYsera->AI()->DoAction(DATA_HELP_AGAINST_ULTRAXION);
            });
            // Kalecgos - Source of Magic
            scheduler.Schedule(Seconds(215), [this](TaskContext /*context*/)
            {
                if (Creature* pKalecgos = me->FindNearestCreature(NPC_KALECGOS, 100.0f))
                    pKalecgos->AI()->DoAction(DATA_HELP_AGAINST_ULTRAXION);
            });
            // Nozdormu - Timeloop
            scheduler.Schedule(Seconds(5*MINUTE), [this](TaskContext /*context*/)
            {
                if (Creature* pNozdormu = me->FindNearestCreature(NPC_NOZDORMU_THE_TIMELESS_ONE, 100.0f))
                    pNozdormu->AI()->DoAction(DATA_HELP_AGAINST_ULTRAXION);
            });
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim && victim->GetTypeId() == TYPEID_PLAYER)
            {
                switch (urand(0, 2))
                {
                case 0:
                    me->MonsterYell("Fall before Ultraxion!", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(26319, true);
                    break;
                case 1:
                    me->MonsterYell("You have no hope!", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(26320, true);
                    break;
                case 2:
                    me->MonsterYell("Hahahahaha!", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(26321, true);
                    break;
                default:
                    break;
                }
            }
        }

        void SummonCache()
        {
            uint32 cacheId = 0;
            switch (getDifficulty())
            {
            case RAID_DIFFICULTY_10MAN_NORMAL:
                cacheId = GO_LESSER_CACHE_OF_ASPECTS_10N;
                break;
            case RAID_DIFFICULTY_25MAN_NORMAL:
                cacheId = GO_LESSER_CACHE_OF_ASPECTS_25N;
                break;
            case RAID_DIFFICULTY_10MAN_HEROIC:
                cacheId = GO_LESSER_CACHE_OF_ASPECTS_10HC;
                break;
            case RAID_DIFFICULTY_25MAN_HEROIC:
                cacheId = GO_LESSER_CACHE_OF_ASPECTS_25N;
                break;
            default:
                break;
            }
            me->SummonGameObject(cacheId, cacheSpawn.GetPositionX(), cacheSpawn.GetPositionY(), cacheSpawn.GetPositionZ(), cacheSpawn.GetOrientation(), 0, 0, 0, 0, 86400);
        }

        void JustDied(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_ULTRAXION, DONE);
            }

            me->MonsterSay("But...but...I am...Ul...trax...ionnnnnn...", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
            me->SendPlaySound(26316, true);

            RemoveEncounterAuras();
            SummonCache();

            if (GameObject* pGoGiftOfLife = me->FindNearestGameObject(GO_GIFT_OF_LIFE, 100.0f))
                pGoGiftOfLife->Delete();
            if (GameObject* pGoEssenceOfMagic = me->FindNearestGameObject(GO_ESSENCE_OF_DREAMS, 100.0f))
                pGoEssenceOfMagic->Delete();
            if (GameObject* pGoSourceOfMagic = me->FindNearestGameObject(GO_SOURCE_OF_MAGIC, 100.0f))
                pGoSourceOfMagic->Delete();
        }

        void CastSpellOnRandomPlayers(uint32 spellId, uint32 size, bool triggered = true, bool ignoreTanks = false, bool ignoreHealers = false)
        {
            std::list<Player*> target_list;
            target_list.clear(); // For sure
            Map * map = me->GetMap();

            if (!map)
                return;

            Map::PlayerList const& plrList = map->GetPlayers();
            if (plrList.isEmpty())
                return;

            for (Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
            {
                if (Player* pPlayer = itr->getSource())
                {
                    if (pPlayer->IsAlive() && !pPlayer->IsGameMaster())
                    {
                        if (ignoreHealers == true && ignoreTanks == true)
                        {
                            if (!pPlayer->HasTankSpec() && !pPlayer->HasAura(5487) && !pPlayer->HasHealingSpec())
                                target_list.push_back(pPlayer);
                        }
                        else if (ignoreTanks == true)
                        {
                            if (!pPlayer->HasTankSpec() && !pPlayer->HasAura(5487)) // Or bear form
                                target_list.push_back(pPlayer);
                        }
                        else if (ignoreHealers == true)
                        {
                            if (!pPlayer->HasHealingSpec())
                                target_list.push_back(pPlayer);
                        } else target_list.push_back(pPlayer);
                    }
                }
            }

            for (uint32 i = 0; i < size; i++)
            {
                if (target_list.empty())
                    break;

                std::list<Player*>::iterator j = target_list.begin();
                advance(j, rand() % target_list.size()); // Pick random target

                if ((*j) && (*j)->IsInWorld())
                {
                    me->CastSpell((*j), spellId, triggered);
                    target_list.erase(j);
                }
            }
        }

        void FadingLight()
        {
            for (uint32 i = 0; i < 2; i++)
            {
                uint32 rand_time;
                rand_time = (i == 0) ? urand(10, 20) : urand(30, 40);

                scheduler.Schedule(Seconds(rand_time), [this](TaskContext /*context*/)
                {
                    int32 num_players;
                    switch (getDifficulty())
                    {
                    case RAID_DIFFICULTY_10MAN_HEROIC:
                        num_players = 2;
                        break;
                    case RAID_DIFFICULTY_25MAN_NORMAL:
                        num_players = 3;
                        break;
                    case RAID_DIFFICULTY_25MAN_HEROIC:
                        num_players = 6;
                        break;
                    default:
                        num_players = 1;
                        break;
                    }
                    me->CastSpell(me->GetVictim(), FADING_LIGHT_DPS_10N, true);
                    CastSpellOnRandomPlayers(FADING_LIGHT_DPS_10N, num_players, true, true, true);
                });
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (!UpdateVictim())
                return;

            // Unstable Monstrosity ability
            if (unstableMonstrosityTimer <= diff)
            {
                unstableMonstrosityCount++;
                unstableMonstrosityTimer = 60000;

                switch (unstableMonstrosityCount)
                {
                case 1: // 6s -> 5s
                    me->RemoveAura(UNSTABLE_MONSTROSITY_6S);
                    me->CastSpell(me, UNSTABLE_MONSTROSITY_5S, true);
                    break;
                case 2: // 5s -> 4s
                    me->RemoveAura(UNSTABLE_MONSTROSITY_5S);
                    me->CastSpell(me, UNSTABLE_MONSTROSITY_4S, true);
                    break;
                case 3: // 4s -> 3s
                    me->RemoveAura(UNSTABLE_MONSTROSITY_4S);
                    me->CastSpell(me, UNSTABLE_MONSTROSITY_3S, true);
                    break;
                case 4: // 3s -> 2s
                    me->RemoveAura(UNSTABLE_MONSTROSITY_3S);
                    me->CastSpell(me, UNSTABLE_MONSTROSITY_2S, true);
                    break;
                case 5: // 2s -> 1s
                    me->RemoveAura(UNSTABLE_MONSTROSITY_2S);
                    me->CastSpell(me, UNSTABLE_MONSTROSITY_1S, true);
                    break;
                case 6: // 1s -> Twilight Eruption
                    me->CastSpell(me, TWILIGHT_ERUPTION, false);
                    me->MonsterYell("I WILL DRAG YOU WITH ME INTO FLAME AND DARKNESS!", LANG_UNIVERSAL, me->GetGUID(), 150.0f);
                    me->SendPlaySound(26315, true);
                default:
                    break;
                }
            }
            else unstableMonstrosityTimer -= diff;

            // Hour Of Twilight
            if (hourOfTwilightTimer <= diff)
            {
                me->CastSpell(me, HOUR_OF_TWILIGHT_10N, false);
                hourOfTwilightTimer = 45000;
                FadingLight();
            }
            else hourOfTwilightTimer -= diff;

            // Melee attack ?
            // DoMeleeAttackIfReady();
        }
    };
};

class spell_ultraxion_heroic_will : public SpellScriptLoader
{
public:
    spell_ultraxion_heroic_will() : SpellScriptLoader("spell_ultraxion_heroic_will") { }

    class spell_ultraxion_heroic_will_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_ultraxion_heroic_will_AuraScript);

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetTarget())
                GetTarget()->CastSpell(GetTarget(), FADED_INTO_TWILIGHT_10N, true);
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_ultraxion_heroic_will_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_MOD_PACIFY_SILENCE, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_ultraxion_heroic_will_AuraScript();
    }
};

class spell_ultraxion_fading_light : public SpellScriptLoader
{
public:
    spell_ultraxion_fading_light() : SpellScriptLoader("spell_ultraxion_fading_light") { }

    class spell_ultraxion_fading_light_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_ultraxion_fading_light_AuraScript);

        void OnApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {
            if (!GetCaster())
                return;

            Aura * aura = aurEff->GetBase();

            uint32 duration = urand((GetCaster()->GetMap()->IsHeroic() ? 3000 : 5000), 9000);
            aura->SetDuration(duration);
            aura->SetMaxDuration(duration);
        }

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetTarget())
            {
                if (GetTarget()->HasAura(TWILIGHT_SHIFT))
                {
                    GetTarget()->CastSpell(GetTarget(), FADING_LIGHT_KILL_10N, true);
                    GetTarget()->Kill(GetTarget());
                }
                else if (GetTarget()->HasAura(HEROIC_WILL))
                    GetTarget()->CastSpell(GetTarget(), TWILIGHT_SHIFT, true);
            }
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_ultraxion_fading_light_AuraScript::OnApply, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            OnEffectRemove += AuraEffectRemoveFn(spell_ultraxion_fading_light_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);

        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_ultraxion_fading_light_AuraScript();
    }
};

class spell_ultraxion_twilight_instability : public SpellScriptLoader
{
public:
    spell_ultraxion_twilight_instability() : SpellScriptLoader("spell_ultraxion_twilight_instability") {}

    class spell_ultraxion_twilight_instability_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ultraxion_twilight_instability_SpellScript);

        void FilterTargets(std::list<Unit*>& targets)
        {
            if (!GetCaster())
                return;

            targets.remove_if(AuraCheck(HEROIC_WILL, true));
        }

        void HandleScript()
        {
            if (!GetCaster() || !GetHitUnit())
                return;

            if (GetCaster()->HasUnitState(UNIT_STATE_CASTING))
                return;

            GetCaster()->CastSpell(GetHitUnit(), TWILIGHT_INSTABILITY_10N, true);
        }

        void Register()
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ultraxion_twilight_instability_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_AREA_ENEMY_SRC);
            AfterHit += SpellHitFn(spell_ultraxion_twilight_instability_SpellScript::HandleScript);
        }

    private:
        class AuraCheck
        {
        public:
            AuraCheck(uint32 spellId, bool present) : _spellId(spellId), _present(present) {}

            bool operator()(WorldObject* unit)
            {
                if (!unit->ToUnit())
                    return true;

                if (_present)
                    return unit->ToUnit()->HasAura(_spellId);
                else
                    return !unit->ToUnit()->HasAura(_spellId);
            }

        private:
            uint32 _spellId;
            bool _present;
        };
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ultraxion_twilight_instability_SpellScript();
    }
};

class spell_ultraxion_hour_of_twilight_dmg : public SpellScriptLoader
{
public:
    spell_ultraxion_hour_of_twilight_dmg() : SpellScriptLoader("spell_ultraxion_hour_of_twilight_dmg") { }

    class spell_ultraxion_hour_of_twilight_dmg_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ultraxion_hour_of_twilight_dmg_SpellScript);

        void FilterTargetsDamage(std::list<Unit*>& targets)
        {
            if (!GetCaster())
                return;

            targets.remove_if(AuraCheck(HEROIC_WILL, true));

            uint32 min_players = 1;

            switch (GetCaster()->GetMap()->GetDifficulty())
            {
            case RAID_DIFFICULTY_10MAN_HEROIC:
                min_players = 2;
                break;
            case RAID_DIFFICULTY_25MAN_NORMAL:
                min_players = 3;
                break;
            case RAID_DIFFICULTY_25MAN_HEROIC:
                min_players = 5;
                break;
            default:
                min_players = 1;
                break;
            }

            if (targets.size() < min_players)
            {
                if (Creature* pUltraxion = GetCaster()->ToCreature())
                    pUltraxion->CastSpell(pUltraxion, TWILIGHT_ERUPTION, false);
            }
        }

        void FilterTargetsDarkness(std::list<Unit*>& targets)
        {
            if (!GetCaster())
                return;

            targets.remove_if(AuraCheck(HEROIC_WILL, true));
            targets.remove_if(AuraCheck(LOOMING_DARKNESS_DUMMY, false));
        }

        void FilterTargetsAchievement(std::list<Unit*>& targets)
        {
            if (!GetCaster())
                return;

            targets.remove_if(AuraCheck(HEROIC_WILL, true));
        }

        void Register()
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ultraxion_hour_of_twilight_dmg_SpellScript::FilterTargetsDamage, EFFECT_0, TARGET_UNIT_AREA_ENEMY_SRC);
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ultraxion_hour_of_twilight_dmg_SpellScript::FilterTargetsDarkness, EFFECT_1, TARGET_UNIT_AREA_ENEMY_SRC);
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ultraxion_hour_of_twilight_dmg_SpellScript::FilterTargetsAchievement, EFFECT_2, TARGET_UNIT_AREA_ENEMY_SRC);
        }

    private:
        class AuraCheck
        {
        public:
            AuraCheck(uint32 spellId, bool present) : _spellId(spellId), _present(present) {}

            bool operator()(WorldObject* unit)
            {
                if (!unit->ToUnit())
                    return true;

                if (_present)
                    return unit->ToUnit()->HasAura(_spellId);
                else
                    return !unit->ToUnit()->HasAura(_spellId);
            }

        private:
            uint32 _spellId;
            bool _present;
        };
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ultraxion_hour_of_twilight_dmg_SpellScript();
    }
};

void AddSC_boss_ultraxion()
{
    // Boss
    new boss_ultraxion();

    // Spells
    new spell_ultraxion_heroic_will();
    new spell_ultraxion_fading_light();
    new spell_ultraxion_twilight_instability();
    new spell_ultraxion_hour_of_twilight_dmg();
}