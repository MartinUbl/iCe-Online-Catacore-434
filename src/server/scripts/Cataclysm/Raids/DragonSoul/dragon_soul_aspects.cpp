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

#include "ScriptPCH.h"
#include "dragonsoul.h"
#include "TaskScheduler.h"

#define DEFAULT_Z_POS     341.355f
#define SEARCH_RANGE      300.0f

enum aspectsNPC
{
    NPC_GO_GIFT_OF_LIFE_SPAWN           = 119550,
    NPC_GO_ESSENCE_OF_DREAMS_SPAWN      = 119551,
    NPC_GO_SOURCE_OF_MAGIC_SPAWN        = 119552,
};

enum aspectsGameobjescts
{
    GO_GIFT_OF_LIFE                     = 209873,
    GO_ESSENCE_OF_DREAMS                = 209874,
    GO_SOURCE_OF_MAGIC                  = 209875,
};

enum aspectsSpells
{
    // Alexstrasza
    GIFT_OF_LIFE_AURA                   = 105896,
    GIFT_OF_LIFE_SUMMON_1               = 106044,
    GIFT_OF_LIFE_SUMMON_2               = 109340,
    GIFT_OF_LIFE_MISSILE_10N            = 106042,
    GIFT_OF_LIFE_MISSILE_25N            = 109349,
    GIFT_OF_LIFE_MISSILE_10HC           = 109350,
    GIFT_OF_LIFE_MISSILE_25HC           = 109351,
    GIFT_OF_LIFE_4                      = 109345,
    // Ysera
    ESSENCE_OF_DREAMS_AURA              = 105900,
    ESSENCE_OF_DREAMS_HEAL              = 105996,
    ESSENCE_OF_DREAMS_SUMMON_1          = 106047,
    ESSENCE_OF_DREAMS_SUMMON_2          = 109342,
    ESSENCE_OF_DREAMS_MISSILE_10N       = 106049,
    ESSENCE_OF_DREAMS_MISSILE_25N       = 109356,
    ESSENCE_OF_DREAMS_MISSILE_10HC      = 109357,
    ESSENCE_OF_DREAMS_MISSILE_25HC      = 109358,
    ESSENCE_OF_DREAMS_5                 = 109344,
    // Kalecgos
    SOURCE_OF_MAGIC_AURA                = 105903,
    SOURCE_OF_MAGIC_SUMMON_1            = 106048,
    SOURCE_OF_MAGIC_SUMMON_2            = 109346,
    SOURCE_OF_MAGIC_MISSILE_10N         = 106050,
    SOURCE_OF_MAGIC_MISSILE_25N         = 109353,
    SOURCE_OF_MAGIC_MISSILE_10HC        = 109354,
    SOURCE_OF_MAGIC_MISSILE_25HC        = 109355,
    SOURCE_OF_MAGIC_4                   = 109347,
    // Nozdormu
    TIMELOOP                            = 105984,
    TIMELOOP_HEAL                       = 105992,
    // Thrall
    LAST_DEFENDER_OF_AZEROTH            = 106182,
    LAST_DEFENDER_OF_AZEROTH_SCRIPT     = 106218,
    LAST_DEFENDER_OF_AZEROTH_DUMMY      = 110327,
    LAST_DEFENDER_OF_AZEROTH_WARR       = 106080,
    LAST_DEFENDER_OF_AZEROTH_DRUID      = 106224,
    LAST_DEFENDER_OF_AZEROTH_PALA       = 106226,
    LAST_DEFENDER_OF_AZEROTH_DK         = 106227,

    SPELL_CHARGING_UP_LIFE              = 108490,
    SPELL_CHARGING_UP_MAGIC             = 108491,
    SPELL_CHARGING_UP_EARTH             = 108492,
    SPELL_CHARGING_UP_TIME              = 108493,
    SPELL_CHARGING_UP_DREAMS            = 108494,

    SPELL_WARD_OF_TIME                  = 108160,
    SPELL_WARD_OF_EARTH                 = 108161,
    SPELL_WARD_OF_MAGIC                 = 108162,
    SPELL_WARD_OF_LIFE                  = 108163,
    SPELL_WARD_OF_DREAMS                = 108164,

    SPELL_CHARGE_DRAGONSOUL_LIFE        = 108242,
    SPELL_CHARGE_DRAGONSOUL_EARTH       = 108471,
    SPELL_CHARGE_DRAGONSOUL_MAGIC       = 108243,
    SPELL_CHARGE_DRAGONSOUL_TIME        = 108472,
    SPELL_CHARGE_DRAGONSOUL_DREAMS      = 108473,

    SPELL_DRAGON_SOUL_CHARGED_COSMETIC  = 110489,
    SPELL_DRAGON_SOUL_CHARGED_DARK      = 108531,
    SPELL_DRAGON_SOUL_CHARGED_LIGHT     = 108530,
    SPELL_DRAGON_SOUL_BULWARK           = 108035,
};

enum AspectsSteps
{
    ASPECTS_BASE_STEP                   = 0,
    ASPECTS_FIRST_STEP                  = 1,
    ASPECTS_SECOND_STEP                 = 2,
    ASPECTS_THIRD_STEP                  = 3,
    ASPECTS_MAX_STEPS                   = 4,
};

static const Position alexstraszaPos[ASPECTS_MAX_STEPS] =
{
    { -1707.57f, -2387.85f, 340.10f, 0.0f },
    { -1684.49f, -2412.19f, 340.10f, 0.0f },
    { -1690.63f, -2426.18f, 342.74f, 0.0f },
    { -1690.63f, -2426.19f, 342.77f, 0.0f },
};

static const Position yseraPos[ASPECTS_MAX_STEPS] =
{
    { -1707.57f, -2387.85f, 340.10f, 0.0f },
    { -1684.49f, -2412.19f, 340.10f, 0.0f },
    { -1690.63f, -2426.18f, 342.74f, 0.0f },
    { -1691.19f, -2433.78f, 342.74f, 0.0f },
};

static const Position kalecgosPos[ASPECTS_MAX_STEPS] =
{
    { -1717.11f, -2389.02f, 340.10f, 0.0f },
    { -1711.17f, -2412.43f, 339.75f, 0.0f },
    { -1702.76f, -2424.50f, 342.72f, 0.0f },
    { -1702.34f, -2426.99f, 342.95f, 0.0f },
};

static const Position nozdormuPos[ASPECTS_MAX_STEPS] =
{
    { -1717.11f, -2389.02f, 340.10f, 0.0f },
    { -1711.17f, -2412.43f, 339.75f, 0.0f },
    { -1702.12f, -2426.70f, 342.75f, 0.0f },
    { -1699.94f, -2434.31f, 342.72f, 0.0f },
};

static const Position thrallPos[ASPECTS_MAX_STEPS] =
{
    { -1717.11f, -2389.02f, 340.10f, 0.0f },
    { -1711.17f, -2412.43f, 339.75f, 0.0f },
    { -1705.48f, -2422.57f, 342.66f, 0.0f },
    { -1696.85f, -2422.11f, 342.79f, 0.0f },
};

const Position pickDragonSoulPos = { -1788.93f, -2391.44f, 341.35f, 5.60f }; // Thrall Pick up Dragon Soul
const Position dragonSoulShipPos = { -1696.34f, -2428.96f, 342.78f, 4.83f }; // Dragon Soul position on alliance ship

// NPC_ALEXTRASZA_THE_LIFE_BINDER  56630
class npc_ds_alexstrasza_the_life_binder : public CreatureScript
{
public:
    npc_ds_alexstrasza_the_life_binder() : CreatureScript("npc_ds_alexstrasza_the_life_binder") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_alexstrasza_the_life_binderAI(pCreature);
    }

    struct npc_ds_alexstrasza_the_life_binderAI : public ScriptedAI
    {
        npc_ds_alexstrasza_the_life_binderAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        TaskScheduler scheduler;

        void Reset() override
        {
            if (instance && (instance->GetData(DATA_ASPECTS_PREPARE_TO_CHANNEL) == 2))
                me->AI()->DoAction(DATA_CHANNEL_DRAGONSOUL);
        }

        void DoAction(const int32 action) override
        {
            switch (action)
            {
            case DATA_PLAYERS_SUMMIT_ARRIVAL:
                scheduler.Schedule(Seconds(13), [this](TaskContext /*Alexstrasza 1st pre-Hagara quote*/)
                {
                    me->MonsterSay("Then truly, we are lost.", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(26503, true);
                });
                break;
            case DATA_ASPECTS_PREPARE_TO_CHANNEL:
                me->MonsterSay("The ritual will take much from us, as it requires a very piece of our being. We will be weakened greatly.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(26504, true);
                scheduler.Schedule(Seconds(10), [this](TaskContext /*Alexstrazsa starts channeling*/)
                {
                    me->CastSpell(me, SPELL_CHARGING_UP_LIFE, false);
                    if (Creature* pThrall = me->FindNearestCreature(NPC_THRALL, SEARCH_RANGE))
                        pThrall->AI()->DoAction(DATA_ASPECTS_PREPARE_TO_CHANNEL);
                });
                break;
            case DATA_CHANNEL_DRAGONSOUL:
                scheduler.Schedule(Seconds(43), [this](TaskContext /*Alexstrasza's Quote*/)
                {
                    if (instance && (instance->GetData(DATA_ULTRAXION_DRAKES) <= 14))
                    {
                        me->MonsterSay("They... are my clutch no longer. Bring them down.", LANG_UNIVERSAL, 0);
                        me->SendPlaySound(26505, true);

                        if (Creature* pDeathwing = me->FindNearestCreature(NPC_DEATHWING_WYRMREST_TEMPLE, SEARCH_RANGE))
                            pDeathwing->AI()->DoAction(DATA_ULTRAXION_DRAKES);
                    }
                });
                scheduler.Schedule(Seconds(48), [this](TaskContext /*Start channel*/)
                {
                    me->CastSpell(me, SPELL_WARD_OF_LIFE, true);
                    me->GetMotionMaster()->MoveJump(me->GetPositionX(), me->GetPositionY(), DEFAULT_Z_POS + 2, 2.0f, 2.0f);
                    if (Creature* pDragonSoul = me->FindNearestCreature(NPC_THE_DRAGON_SOUL, SEARCH_RANGE))
                        me->CastSpell(pDragonSoul, SPELL_CHARGE_DRAGONSOUL_LIFE, true);
                });
                scheduler.Schedule(Seconds(50), [this](TaskContext /*Ascend*/)
                {
                    me->GetMotionMaster()->MoveJump(me->GetPositionX(), me->GetPositionY(), DEFAULT_Z_POS + 4, 0.2f, 0.2f);
                });
                break;
            case DATA_HELP_AGAINST_ULTRAXION:
                me->MonsterYell("Take heart, heroes, life will always blossom from the darkest soil!", LANG_UNIVERSAL, 0);
                me->SendPlaySound(26506, true);
                if (Creature* pNpcGiftOfLife = me->FindNearestCreature(NPC_GO_GIFT_OF_LIFE_SPAWN, SEARCH_RANGE))
                    me->CastSpell(pNpcGiftOfLife, GIFT_OF_LIFE_MISSILE_10N, true);
                break;
            case DATA_ULTRAXION_RESET:
                me->InterruptNonMeleeSpells(true, 0, true);
                me->RemoveAllAuras();
                me->GetMotionMaster()->MoveFall();
                break;
            case DATA_ULTRAXION_DEFEATED:
                me->SetWalk(true);
                me->SetSpeed(MOVE_WALK, 1.2f);
                scheduler.Schedule(Seconds(90), [this](TaskContext /*Jump Down*/)
                {
                    me->InterruptNonMeleeSpells(true, 0, true);
                    me->RemoveAllAuras();
                    me->GetMotionMaster()->MoveFall();

                    me->MonsterSay("It is done! Our power now resides within the Dragon Soul! Our fate lies with you, Earth-Warder!", LANG_UNIVERSAL, 0, 100.0f);
                    me->SendPlaySound(26507, true);

                    if (Creature* pDragonSoul = me->FindNearestCreature(NPC_THE_DRAGON_SOUL, SEARCH_RANGE))
                        pDragonSoul->CastSpell(pDragonSoul, SPELL_DRAGON_SOUL_CHARGED_COSMETIC, true);
                });
                scheduler.Schedule(Seconds(121), [this](TaskContext moveSteps)
                {
                    uint32 repeatCount = moveSteps.GetRepeatCounter();
                    uint32 repeatSeconds = 0;

                    switch (repeatCount)
                    {
                    case ASPECTS_BASE_STEP:
                        repeatSeconds = 26;
                        break;
                    case ASPECTS_FIRST_STEP:
                        repeatSeconds = 13;
                        break;
                    case ASPECTS_SECOND_STEP:
                        repeatSeconds = 8;
                        break;
                    case ASPECTS_THIRD_STEP:
                        repeatSeconds = 3;
                        break;
                    default:
                        break;
                    }

                    if (repeatCount < ASPECTS_MAX_STEPS)
                    {
                        moveSteps.Repeat(Seconds(repeatSeconds));
                        me->GetMotionMaster()->MovePoint(repeatCount, alexstraszaPos[repeatCount], true, true);
                    }
                    else
                    {
                        me->SetFacingTo(3.79f);
                        me->CastSpell(me, SPELL_CHARGING_UP_LIFE, true);
                    }
                });
                break;
            default:
                break;
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);
        }
    };
};

// NPC_YSERA_THE_AWAKENED 56665
class npc_ds_ysera_the_awekened : public CreatureScript
{
public:
    npc_ds_ysera_the_awekened() : CreatureScript("npc_ds_ysera_the_awekened") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_ysera_the_awekenedAI(pCreature);
    }

    struct npc_ds_ysera_the_awekenedAI : public ScriptedAI
    {
        npc_ds_ysera_the_awekenedAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        TaskScheduler scheduler;

        void Reset() override
        {
            if (instance && (instance->GetData(DATA_ASPECTS_PREPARE_TO_CHANNEL) == 2))
                me->AI()->DoAction(DATA_CHANNEL_DRAGONSOUL);
        }

        void DoAction(const int32 action) override
        {
            switch (action)
            {
            case DATA_PLAYERS_SUMMIT_ARRIVAL:
                scheduler.Schedule(Seconds(46), [this](TaskContext /*Ysera 1st pre-Hagara*/)
                {
                    me->MonsterSay("I sense danger...it is a trap...carefully laid for us at this crucial moment.", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(26147, true);
                });
                break;
            case DATA_ASPECTS_PREPARE_TO_CHANNEL:
                me->MonsterSay("Praise the Titans, they have returned!", LANG_UNIVERSAL, 0);
                me->SendPlaySound(26145, true);
                scheduler.Schedule(Seconds(5), [this](TaskContext /*Ysera starts channeling*/)
                {
                    me->CastSpell(me, SPELL_CHARGING_UP_DREAMS, false);
                    if (Creature* pNozdormu = me->FindNearestCreature(NPC_NOZDORMU_THE_TIMELESS_ONE, SEARCH_RANGE))
                        pNozdormu->AI()->DoAction(DATA_ASPECTS_PREPARE_TO_CHANNEL);
                });
                break;
            case DATA_CHANNEL_DRAGONSOUL:
                scheduler.Schedule(Seconds(48), [this](TaskContext /*Start channel*/)
                {
                    me->CastSpell(me, SPELL_WARD_OF_DREAMS, true);
                    me->GetMotionMaster()->MoveJump(me->GetPositionX(), me->GetPositionY(), DEFAULT_Z_POS + 2, 2.0f, 2.0f);
                    if (Creature* pDragonSoul = me->FindNearestCreature(NPC_THE_DRAGON_SOUL, SEARCH_RANGE))
                        me->CastSpell(pDragonSoul, SPELL_CHARGE_DRAGONSOUL_DREAMS, true);
                });
                scheduler.Schedule(Seconds(50), [this](TaskContext /*Ascend*/)
                {
                    me->GetMotionMaster()->MoveJump(me->GetPositionX(), me->GetPositionY(), DEFAULT_Z_POS + 4, 0.2f, 0.2f);
                });
                break;
            case DATA_HELP_AGAINST_ULTRAXION:
                me->MonsterYell("In dreams, we may overcome any obstacle.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(26149, true);
                if (Creature* pNpcEssenceOfDreams = me->FindNearestCreature(NPC_GO_ESSENCE_OF_DREAMS_SPAWN, SEARCH_RANGE))
                    me->CastSpell(pNpcEssenceOfDreams, ESSENCE_OF_DREAMS_MISSILE_10N, true);
                break;
            case DATA_ULTRAXION_RESET:
                // They have failed us sister!
                me->InterruptNonMeleeSpells(true, 0, true);
                me->RemoveAllAuras();
                me->GetMotionMaster()->MoveFall();
                break;
            case DATA_ULTRAXION_DEFEATED:
                me->SetWalk(true);
                me->SetSpeed(MOVE_WALK, 1.2f);
                scheduler.Schedule(Seconds(90), [this](TaskContext /*Jump Down*/)
                {
                    me->InterruptNonMeleeSpells(true, 0, true);
                    me->RemoveAllAuras();
                    me->GetMotionMaster()->MoveFall();
                });
                scheduler.Schedule(Seconds(115), [this](TaskContext moveSteps)
                {
                    uint32 repeatCount = moveSteps.GetRepeatCounter();
                    uint32 repeatSeconds = 0;

                    switch (repeatCount)
                    {
                    case ASPECTS_BASE_STEP:
                        repeatSeconds = 24;
                        break;
                    case ASPECTS_FIRST_STEP:
                        repeatSeconds = 12;
                        break;
                    case ASPECTS_SECOND_STEP:
                        repeatSeconds = 6;
                        break;
                    case ASPECTS_THIRD_STEP:
                        repeatSeconds = 14;
                        break;
                    default:
                        break;
                    }

                    if (repeatCount < ASPECTS_MAX_STEPS)
                    {
                        moveSteps.Repeat(Seconds(repeatSeconds));
                        me->GetMotionMaster()->MovePoint(repeatCount, yseraPos[repeatCount], true, true);
                    }
                    else
                    {
                        me->SetFacingTo(2.43f);
                        me->CastSpell(me, SPELL_CHARGING_UP_DREAMS, false);
                    }
                });
                break;
            default:
                break;
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);
        }
    };
};

// NPC_NOZDORMU_THE_TIMELESS_ONE 56666
class npc_ds_nozdormu_the_timeless_one : public CreatureScript
{
public:
    npc_ds_nozdormu_the_timeless_one() : CreatureScript("npc_ds_nozdormu_the_timeless_one") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_nozdormu_the_timeless_oneAI(pCreature);
    }

    struct npc_ds_nozdormu_the_timeless_oneAI : public ScriptedAI
    {
        npc_ds_nozdormu_the_timeless_oneAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        TaskScheduler scheduler;

        void Reset() override
        {
            if (instance && (instance->GetData(DATA_ASPECTS_PREPARE_TO_CHANNEL) == 2))
                me->AI()->DoAction(DATA_CHANNEL_DRAGONSOUL);
        }

        void DoAction(const int32 action) override
        {
            switch (action)
            {
            case DATA_ASPECTS_PREPARE_TO_CHANNEL:
                me->MonsterSay("Not entirely unexpected.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25953, true);
                scheduler.Schedule(Seconds(4), [this](TaskContext /*Nozdormu starts channeling*/)
                {
                    me->CastSpell(me, SPELL_CHARGING_UP_TIME, false);
                    if (Creature* pKalecgos = me->FindNearestCreature(NPC_KALECGOS, SEARCH_RANGE))
                        pKalecgos->AI()->DoAction(DATA_ASPECTS_PREPARE_TO_CHANNEL);
                });
                break;
            case DATA_CHANNEL_DRAGONSOUL:
                scheduler.Schedule(Seconds(48), [this](TaskContext /*Start channel*/)
                {
                    me->CastSpell(me, SPELL_WARD_OF_TIME, true);
                    me->GetMotionMaster()->MoveJump(me->GetPositionX(), me->GetPositionY(), DEFAULT_Z_POS + 2, 2.0f, 2.0f);
                    if (Creature* pDragonSoul = me->FindNearestCreature(NPC_THE_DRAGON_SOUL, SEARCH_RANGE))
                        me->CastSpell(pDragonSoul, SPELL_CHARGE_DRAGONSOUL_TIME, true);
                });
                scheduler.Schedule(Seconds(50), [this](TaskContext /*Ascend*/)
                {
                    me->GetMotionMaster()->MoveJump(me->GetPositionX(), me->GetPositionY(), DEFAULT_Z_POS + 4, 0.2f, 0.2f);
                });
                break;
            case DATA_HELP_AGAINST_ULTRAXION:
            {
                me->MonsterYell("The cycle of time brings an end to all things.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(25954, true);
                me->CastSpell(me, TIMELOOP, true);
                break;
            }
            case DATA_ULTRAXION_RESET:
                me->InterruptNonMeleeSpells(true, 0, true);
                me->RemoveAllAuras();
                me->GetMotionMaster()->MoveFall();
                break;
            case DATA_ULTRAXION_DEFEATED:
                me->SetWalk(true);
                me->SetSpeed(MOVE_WALK, 1.2f);
                scheduler.Schedule(Seconds(90), [this](TaskContext /*Jump Down*/)
                {
                    me->InterruptNonMeleeSpells(true, 0, true);
                    me->RemoveAllAuras();
                    me->GetMotionMaster()->MoveFall();
                });
                scheduler.Schedule(Seconds(115), [this](TaskContext moveSteps)
                {
                    uint32 repeatCount = moveSteps.GetRepeatCounter();
                    uint32 repeatSeconds = 0;

                    switch (repeatCount)
                    {
                    case ASPECTS_BASE_STEP:
                        repeatSeconds = 24;
                        break;
                    case ASPECTS_FIRST_STEP:
                        repeatSeconds = 8;
                        break;
                    case ASPECTS_SECOND_STEP:
                        repeatSeconds = 6;
                        break;
                    case ASPECTS_THIRD_STEP:
                        repeatSeconds = 18;
                        break;
                    default:
                        break;
                    }

                    if (repeatCount < ASPECTS_MAX_STEPS)
                    {
                        moveSteps.Repeat(Seconds(repeatSeconds));
                        me->GetMotionMaster()->MovePoint(repeatCount, nozdormuPos[repeatCount], true, true);
                    }
                    else
                    {
                        me->SetFacingTo(1.02f);
                        me->AddAura(SPELL_CHARGING_UP_TIME, me);
                    }
                });
                break;
            default:
                break;
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);
        }
    };
};

// List of gossip texts
#define GOSSIP_SUCCESS     "We were successfull, Kalecgos"

// NPC_KALECGOS 56664
class npc_ds_kalecgos : public CreatureScript
{
public:
    npc_ds_kalecgos() : CreatureScript("npc_ds_kalecgos") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        if (pCreature->IsQuestGiver())
            pPlayer->PrepareQuestMenu(pCreature->GetGUID());

        if (pCreature->GetInstanceScript()->GetData(TYPE_BOSS_HAGARA) == DONE
            && pCreature->GetInstanceScript()->GetData(DATA_ASPECTS_PREPARE_TO_CHANNEL) == 0)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SUCCESS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, pCreature->GetGUID());
        }

        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
        {
            pCreature->AI()->DoAction(DATA_START_ULTRAXION_ASPECTS_EVENT);
            pPlayer->CLOSE_GOSSIP_MENU();
        }

        return true;
    }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_kalecgosAI(pCreature);
    }

    struct npc_ds_kalecgosAI : public ScriptedAI
    {
        npc_ds_kalecgosAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        TaskScheduler scheduler;

        void Reset() override
        {
            if (instance->GetData(TYPE_BOSS_HAGARA) != DONE || instance->GetData(TYPE_BOSS_ULTRAXION) == DONE)
                me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

            if (instance && (instance->GetData(DATA_ASPECTS_PREPARE_TO_CHANNEL) == 2))
                me->AI()->DoAction(DATA_CHANNEL_DRAGONSOUL);
        }

        void DoAction(const int32 action) override
        {
            switch (action)
            {
            case DATA_PLAYERS_SUMMIT_ARRIVAL:
                scheduler.Schedule(Seconds(18), [this](TaskContext /*Kalecgos 1st pre-Hagara*/)
                {
                    me->MonsterSay("Not necessarily. I believe...yes...it just might work.", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(26547, true);
                });
                scheduler.Schedule(Seconds(26), [this](TaskContext /*Kalecgos 2nd pre-Hagara*/)
                {
                    me->MonsterSay("The Focusing Iris within the Eye of Eternity may allow us to converge the magical matrix contained within the Dragon Soul.", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(26265, true);
                });
                scheduler.Schedule(Seconds(61), [this](TaskContext /*Kalecgos 3rd pre-Hagara*/)
                {
                    me->MonsterSay("Then I will open the way into the Eye of Eternity. You must not fail, heroes. The future of Azeroth hinges on your actions.", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(26266, true);

                    if (Creature * pEyeOfEternityPortal = me->FindNearestCreature(NPC_TRAVEL_TO_EYE_OF_ETERNITY, SEARCH_RANGE, true))
                        me->CastSpell(pEyeOfEternityPortal, SPELL_OPEN_EYE_OF_ETERNITY_PORTAL, false);
                });
                break;
            case DATA_START_ULTRAXION_ASPECTS_EVENT:
                me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                if (Creature* pDragonSoul = me->FindNearestCreature(NPC_THE_DRAGON_SOUL, SEARCH_RANGE))
                    me->SetFacingToObject(pDragonSoul);
                if (Creature* pYsera = me->FindNearestCreature(NPC_YSERA_THE_AWAKENED, SEARCH_RANGE))
                {
                    pYsera->AI()->DoAction(DATA_ASPECTS_PREPARE_TO_CHANNEL);
                    if (instance)
                        instance->SetData(DATA_ASPECTS_PREPARE_TO_CHANNEL, 1);
                }
                break;
            case DATA_ASPECTS_PREPARE_TO_CHANNEL:
                me->MonsterSay("Excellent! We will begin the ritual at once!", LANG_UNIVERSAL, 0);
                me->SendPlaySound(26262, true);
                scheduler.Schedule(Seconds(4), [this](TaskContext /*Kalecgos starts channeling*/)
                {
                    me->CastSpell(me, SPELL_CHARGING_UP_MAGIC, false);
                    if (Creature* pAlexstrasza = me->FindNearestCreature(NPC_ALEXSTRASZA_THE_LIFE_BINDER, SEARCH_RANGE))
                        pAlexstrasza->AI()->DoAction(DATA_ASPECTS_PREPARE_TO_CHANNEL);
                });
                break;
            case DATA_CHANNEL_DRAGONSOUL:
                scheduler.Schedule(Seconds(48), [this](TaskContext /*Start channel*/)
                {
                    me->CastSpell(me, SPELL_WARD_OF_MAGIC, true);
                    me->GetMotionMaster()->MoveJump(me->GetPositionX(), me->GetPositionY(), DEFAULT_Z_POS + 2, 2.0f, 2.0f);
                    if (Creature* pDragonSoul = me->FindNearestCreature(NPC_THE_DRAGON_SOUL, SEARCH_RANGE))
                        me->CastSpell(pDragonSoul, SPELL_CHARGE_DRAGONSOUL_MAGIC, true);
                });
                scheduler.Schedule(Seconds(50), [this](TaskContext /*Ascend*/)
                {
                    me->GetMotionMaster()->MoveJump(me->GetPositionX(), me->GetPositionY(), DEFAULT_Z_POS + 4, 0.2f, 0.2f);
                });
                break;
            case DATA_HELP_AGAINST_ULTRAXION:
                me->MonsterYell("Winds of the arcane be at their backs, and refresh them in this hour of darkness!", LANG_UNIVERSAL, 0);
                me->SendPlaySound(26267, true);
                if (Creature* pNpcSourceOfMagic = me->FindNearestCreature(NPC_GO_SOURCE_OF_MAGIC_SPAWN, SEARCH_RANGE))
                    me->CastSpell(pNpcSourceOfMagic, SOURCE_OF_MAGIC_MISSILE_10N, true);
                break;
            case DATA_ULTRAXION_RESET:
                me->InterruptNonMeleeSpells(true, 0, true);
                me->RemoveAllAuras();
                me->GetMotionMaster()->MoveFall();
                break;
            case DATA_ULTRAXION_DEFEATED:
                me->SetWalk(true);
                me->SetSpeed(MOVE_WALK, 1.2f);
                scheduler.Schedule(Seconds(90), [this](TaskContext /*Jump Down*/)
                {
                    me->InterruptNonMeleeSpells(true, 0, true);
                    me->RemoveAllAuras();
                    me->GetMotionMaster()->MoveFall();
                });
                scheduler.Schedule(Seconds(121), [this](TaskContext moveSteps)
                {
                    uint32 repeatCount = moveSteps.GetRepeatCounter();
                    uint32 repeatSeconds = 0;

                    switch (repeatCount)
                    {
                    case ASPECTS_BASE_STEP:
                        repeatSeconds = 25;
                        break;
                    case ASPECTS_FIRST_STEP:
                        repeatSeconds = 9;
                        break;
                    case ASPECTS_SECOND_STEP:
                        repeatSeconds = 7;
                        break;
                    case ASPECTS_THIRD_STEP:
                        repeatSeconds = 9;
                        break;
                    default:
                        break;
                    }

                    if (repeatCount < ASPECTS_MAX_STEPS)
                    {
                        moveSteps.Repeat(Seconds(repeatSeconds));
                        me->GetMotionMaster()->MovePoint(repeatCount, kalecgosPos[repeatCount], true, true);
                    }
                    else
                    {
                        me->SetFacingTo(6.13f);
                        me->CastSpell(me, SPELL_CHARGING_UP_MAGIC, true);
                    }
                });
                break;
            default:
                break;
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);
        }
    };
};

// List of gossip texts
#define GOSSIP_START_RITUAL     "You may begin your ritual, we will defend you."

// NPC_THRALL 56667
class npc_ds_thrall : public CreatureScript
{
public:
    npc_ds_thrall() : CreatureScript("npc_ds_thrall") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        if (pCreature->IsQuestGiver())
            pPlayer->PrepareQuestMenu(pCreature->GetGUID());

        if ((pCreature->GetInstanceScript()->GetData(DATA_ASPECTS_PREPARE_TO_CHANNEL) == 2) ||
            (pCreature->GetInstanceScript()->GetData(DATA_ASPECTS_PREPARE_TO_CHANNEL) == 4))
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_START_RITUAL, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, pCreature->GetGUID());
        }

        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction) override
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
        {
            pCreature->AI()->DoAction(DATA_CHANNEL_DRAGONSOUL);
            pCreature->GetInstanceScript()->SetData(DATA_ASPECTS_PREPARE_TO_CHANNEL, 4);
            pPlayer->CLOSE_GOSSIP_MENU();
        }

        return true;
    }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_thrallAI(pCreature);
    }

    struct npc_ds_thrallAI : public ScriptedAI
    {
        npc_ds_thrallAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        TaskScheduler scheduler;
        uint32 checkTimer;
        bool playersArrived;

        void Reset() override
        {
            checkTimer = 30000;
            playersArrived = false;
            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

            if (instance && (instance->GetData(DATA_ASPECTS_PREPARE_TO_CHANNEL) == 2))
                me->AI()->DoAction(DATA_CHANNEL_DRAGONSOUL);
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

        void DoAction(const int32 action) override
        {
            if (instance)
            {
                switch (action)
                {
                case DATA_PLAYERS_SUMMIT_ARRIVAL:
                {
                    if (Creature* pAlexstrasza = me->FindNearestCreature(NPC_ALEXSTRASZA_THE_LIFE_BINDER, SEARCH_RANGE))
                        pAlexstrasza->AI()->DoAction(DATA_PLAYERS_SUMMIT_ARRIVAL);
                    if (Creature* pKalecgos = me->FindNearestCreature(NPC_KALECGOS, SEARCH_RANGE))
                        pKalecgos->AI()->DoAction(DATA_PLAYERS_SUMMIT_ARRIVAL);
                    if (Creature* pYsera = me->FindNearestCreature(NPC_YSERA_THE_AWAKENED, SEARCH_RANGE))
                        pYsera->AI()->DoAction(DATA_PLAYERS_SUMMIT_ARRIVAL);

                    me->MonsterSay("It is no use, the power of the Dragon Soul is too great. I cannot wield it safely, the raging forces contained within it may be the doom of us all.", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(25904, true);
                    scheduler.Schedule(Seconds(38), [this](TaskContext /*Thrall's 2nd pre-Hagara quote*/)
                    {
                        me->MonsterSay("Yes, I see. We can finally turn Deathwing's own power back against him.", LANG_UNIVERSAL, 0);
                        me->SendPlaySound(25905, true);
                    });
                    scheduler.Schedule(Seconds(54), [this](TaskContext /*Thrall's 3rd pre-Hagara quote*/)
                    {
                        me->MonsterSay("We have no choice. We must have faith in our allies.", LANG_UNIVERSAL, 0);
                        me->SendPlaySound(25906, true);
                    });
                    break;
                }
                case DATA_ASPECTS_PREPARE_TO_CHANNEL:
                    me->MonsterSay("Heroes, this burden falls to you once again.", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(25898, true);
                    scheduler.Schedule(Seconds(6), [this](TaskContext /*Thrall's 2nd pre-Ultraxion quote*/)
                    {
                        me->MonsterSay("You must protect us from Deathwing's forces while we imbue the Dragon Soul with the power of the Aspects.", LANG_UNIVERSAL, 0);
                        me->SendPlaySound(25899, true);
                    });
                    scheduler.Schedule(Seconds(16), [this](TaskContext /*Thrall's 3rd pre-Ultraxion quote*/)
                    {
                        me->MonsterSay("Speak to me when you are ready to begin.", LANG_UNIVERSAL, 0);
                        me->SendPlaySound(25900, true);
                        me->CastSpell(me, SPELL_CHARGING_UP_EARTH, false);
                        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        instance->SetData(DATA_ASPECTS_PREPARE_TO_CHANNEL, 2);
                    });
                    break;
                case DATA_CHANNEL_DRAGONSOUL:
                    me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

                    if (Creature* pAlexstrasza = me->FindNearestCreature(NPC_ALEXSTRASZA_THE_LIFE_BINDER, SEARCH_RANGE, true))
                        pAlexstrasza->AI()->DoAction(DATA_CHANNEL_DRAGONSOUL);
                    if (Creature* pKalecgos = me->FindNearestCreature(NPC_KALECGOS, SEARCH_RANGE, true))
                        pKalecgos->AI()->DoAction(DATA_CHANNEL_DRAGONSOUL);
                    if (Creature* pNozdormu = me->FindNearestCreature(NPC_NOZDORMU_THE_TIMELESS_ONE, SEARCH_RANGE, true))
                        pNozdormu->AI()->DoAction(DATA_CHANNEL_DRAGONSOUL);
                    if (Creature* pYsera = me->FindNearestCreature(NPC_YSERA_THE_AWAKENED, SEARCH_RANGE, true))
                        pYsera->AI()->DoAction(DATA_CHANNEL_DRAGONSOUL);

                    if (Creature* pDeathwing = me->FindNearestCreature(NPC_DEATHWING_WYRMREST_TEMPLE, 500.0f, true))
                    {
                        if (instance->GetData(DATA_ULTRAXION_DRAKES) <= 14)
                            pDeathwing->AI()->DoAction(DATA_DEATHWING_GREETINGS);
                    }

                    scheduler.Schedule(Seconds(48), [this](TaskContext /*Start channel*/)
                    {
                        me->CastSpell(me, SPELL_WARD_OF_EARTH, true);
                        me->GetMotionMaster()->MoveJump(me->GetPositionX(), me->GetPositionY(), DEFAULT_Z_POS + 2, 2.0f, 2.0f);
                        if (Creature* pDragonSoul = me->FindNearestCreature(NPC_THE_DRAGON_SOUL, SEARCH_RANGE))
                            me->CastSpell(pDragonSoul, SPELL_CHARGE_DRAGONSOUL_EARTH, true);
                    });
                    scheduler.Schedule(Seconds(50), [this](TaskContext /*Ascend*/)
                    {
                        if (instance->GetData(DATA_ULTRAXION_DRAKES) >= 15)
                            instance->SetData(DATA_SUMMON_ULTRAXION, 0);

                        me->GetMotionMaster()->MoveJump(me->GetPositionX(), me->GetPositionY(), DEFAULT_Z_POS + 4, 0.2f, 0.2f);
                        if (Creature* pDragonSoul = me->FindNearestCreature(NPC_THE_DRAGON_SOUL, SEARCH_RANGE))
                            pDragonSoul->GetMotionMaster()->MoveJump(pDragonSoul->GetPositionX(), pDragonSoul->GetPositionY(), 352.0f, 0.4f, 0.4f);
                    });
                    break;
                case DATA_HELP_AGAINST_ULTRAXION:
                    me->MonsterYell("Strength of the Earth, hear my call! Shield them in this dark hour, the last defenders of Azeroth!", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(25907, true);
                    me->CastSpell(me, LAST_DEFENDER_OF_AZEROTH, true);
                    break;
                case DATA_ULTRAXION_RESET:
                    me->InterruptNonMeleeSpells(true, 0, true);
                    me->RemoveAllAuras();
                    me->GetMotionMaster()->MoveJump(me->GetPositionX(), me->GetPositionY(), DEFAULT_Z_POS, 5.0f, 5.0f);
                    break;
                case DATA_ULTRAXION_DEFEATED:
                    scheduler.Schedule(Seconds(90), [this](TaskContext /*Jump Down*/)
                    {
                        me->InterruptNonMeleeSpells(true, 0, true);
                        me->RemoveAllAuras();
                        me->GetMotionMaster()->MoveJump(me->GetPositionX(), me->GetPositionY(), DEFAULT_Z_POS, 5.0f, 5.0f);

                        if (Creature* pDragonSoul = me->FindNearestCreature(NPC_THE_DRAGON_SOUL, SEARCH_RANGE))
                            pDragonSoul->GetMotionMaster()->MoveJump(pDragonSoul->GetPositionX(), pDragonSoul->GetPositionY(), pDragonSoul->GetPositionZ() - 8, 1.0f, 1.0f);
                    });
                    scheduler.Schedule(Seconds(100), [this](TaskContext /*Walk towards Dragon Soul*/)
                    {
                        me->SetSpeed(MOVE_WALK, 0.4f);
                        me->SetWalk(true);
                        me->GetMotionMaster()->MovePoint(0, pickDragonSoulPos, true);
                        me->MonsterSay("Taretha... Cairne... Aggra... I will not fail you. I will not fail this world!", LANG_UNIVERSAL, 0, SEARCH_RANGE);
                        me->SendPlaySound(25908, true);
                    });
                    scheduler.Schedule(Seconds(112), [this](TaskContext /*Show Movie and ship*/)
                    {
                        me->SetSpeed(MOVE_WALK, 1.2f);
                        PlayMovieToPlayers(73);
                        if (Creature* pDragonSoul = me->FindNearestCreature(NPC_THE_DRAGON_SOUL, SEARCH_RANGE))
                            pDragonSoul->SetVisible(false);
                        if (GameObject * allianceShip = me->FindNearestGameObject(GO_ALLIANCE_SHIP, SEARCH_RANGE))
                        {
                            allianceShip->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);
                            allianceShip->UpdateObjectVisibility();
                            if (Creature * pSwayze = me->FindNearestCreature(NPC_SKY_CAPTAIN_SWAYZE, SEARCH_RANGE, true))
                                pSwayze->SetVisible(true);
                            if (Creature * pKaanuReevs = me->FindNearestCreature(NPC_KAANU_REEVS, SEARCH_RANGE, true))
                                pKaanuReevs->SetVisible(true);
                        }
                    });
                    scheduler.Schedule(Seconds(117), [this](TaskContext moveSteps)
                    {
                        uint32 repeatCount = moveSteps.GetRepeatCounter();
                        uint32 repeatSeconds = 0;

                        switch (repeatCount)
                        {
                        case ASPECTS_BASE_STEP:
                            repeatSeconds = 24;
                            break;
                        case ASPECTS_FIRST_STEP:
                            repeatSeconds = 10;
                            break;
                        case ASPECTS_SECOND_STEP:
                            repeatSeconds = 5;
                            break;
                        case ASPECTS_THIRD_STEP:
                            repeatSeconds = 15;
                            break;
                        default:
                            break;
                        }

                        if (repeatCount < ASPECTS_MAX_STEPS)
                        {
                            moveSteps.Repeat(Seconds(repeatSeconds));
                            me->GetMotionMaster()->MovePoint(repeatCount, thrallPos[repeatCount], true, true);
                        }
                        else
                        {
                            instance->SetData(DATA_ASPECTS_PREPARE_TO_CHANNEL, 3);

                            if (Unit * pDragonSoul = me->SummonCreature(NPC_THE_DRAGON_SOUL, dragonSoulShipPos.GetPositionX(), dragonSoulShipPos.GetPositionY(), dragonSoulShipPos.GetPositionZ() + 1.5f, dragonSoulShipPos.GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN))
                            {
                                me->SetFacingTo(4.77f);
                                me->CastSpell(me, SPELL_CHARGING_UP_EARTH, true);
                                pDragonSoul->CastSpell(pDragonSoul, SPELL_DRAGON_SOUL_BULWARK, true);
                            }
                        }
                    });
                    break;
                default:
                    break;
                }
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (playersArrived == false && instance && instance->GetData(TYPE_BOSS_HAGARA) != DONE)
            {
                if (checkTimer <= diff)
                {
                    float distance;
                    uint32 playersCount = 0;

                    Map::PlayerList const &playerList = me->GetMap()->GetPlayers();
                    if (!playerList.isEmpty())
                    {
                        for (Map::PlayerList::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
                        {
                            if (Player* pPlayer = i->getSource())
                            {
                                distance = me->GetExactDist(pPlayer);
                                if (distance < 20)
                                    playersCount++;
                            }
                        }
                    }

                    uint32 minPlayers;
                    switch (getDifficulty())
                    {
                    case RAID_DIFFICULTY_25MAN_NORMAL:
                    case RAID_DIFFICULTY_25MAN_HEROIC:
                        minPlayers = 20;
                        break;
                    case RAID_DIFFICULTY_10MAN_NORMAL:
                    case RAID_DIFFICULTY_10MAN_HEROIC:
                        minPlayers = 8;
                        break;
                    default:
                        break;
                    }

                    // ATM For Testing purpose
                    minPlayers = 1;

                    if (playersCount >= minPlayers)
                    {
                        me->AI()->DoAction(DATA_PLAYERS_SUMMIT_ARRIVAL);
                        playersArrived = true;
                    }

                    checkTimer = 30000;
                }
                else checkTimer -= diff;
            }
        }
    };
};

void AddSC_dragon_soul_aspects()
{
    new npc_ds_alexstrasza_the_life_binder();
    new npc_ds_ysera_the_awekened();
    new npc_ds_nozdormu_the_timeless_one();
    new npc_ds_kalecgos();
    new npc_ds_thrall();
}