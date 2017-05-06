/*
 * Copyright (C) 2006-2016 iCe Online <http://www.ice-wow.eu/>
 *
 * This part of program is not free software. iCe GM Team owns all of its content
 * Who won't obey that rule, i will kick his balls and twist his nipples.
 *
 */

#include "ScriptPCH.h"
#include "TaskScheduler.h"
#include "WaypointMovementGenerator.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"

// QUEST A HIDDEN MESSAGE
enum npc_altha_and_rafir_stuff
{
    QUEST_A_HIDDEN_MESSAGE          = 29802,

    ITEM_CRYPTOMANCERS_DECODER_RING = 74256,
    ITEM_CHARGING_DECODER_RING      = 74749,
    ITEM_SINGED_CIPHER              = 74750,
};

#define THAUMATURGE_TEXT_ID_1 800
#define THAUMATURGE_TEXT_ID_2 801
#define THAUMATURGE_TEXT_ID_3 802
#define THAUMATURGE_GOSSIP_1 "Can you charge my Cryptomancer's Decoder Ring?"
#define THAUMATURGE_GOSSIP_2 "<Pay the 10.000 gold.>"
#define THAUMATURGE_GOSSIP_3 "Ten thousand?! Outrageous! I'll kill you while you sleep!"

class npc_thaumaturge_altha_and_rafir : public CreatureScript
{
public:
    npc_thaumaturge_altha_and_rafir() : CreatureScript("npc_thaumaturge_altha_and_rafir") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        if (pCreature->IsQuestGiver())
            pPlayer->PrepareQuestMenu(pCreature->GetGUID());

        if (pPlayer->GetQuestStatus(QUEST_A_HIDDEN_MESSAGE) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, THAUMATURGE_GOSSIP_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->SEND_GOSSIP_MENU(THAUMATURGE_TEXT_ID_1, pCreature->GetGUID());
        }
        else
            pPlayer->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, pCreature->GetGUID());

        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*sender*/, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
        {
            pPlayer->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, THAUMATURGE_GOSSIP_2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2, "Are you sure you want to pay?", 10000 * GOLD, 0);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, THAUMATURGE_GOSSIP_3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
            pPlayer->SEND_GOSSIP_MENU(THAUMATURGE_TEXT_ID_2, pCreature->GetGUID());
        }
        if (uiAction == GOSSIP_ACTION_INFO_DEF + 2)
        {
            if (pPlayer->GetItemCount(ITEM_CRYPTOMANCERS_DECODER_RING, false) == 1 && pPlayer->GetMoney() >= 10000 * GOLD)
            {
                pPlayer->DestroyItemCount(ITEM_CRYPTOMANCERS_DECODER_RING, 1, true, true);
                pPlayer->ModifyMoney(-10000 * GOLD);
                pPlayer->AddItem(ITEM_CHARGING_DECODER_RING, 1);
                pCreature->MonsterSay("Very good! Remember, your ring will not reach its full charge for many hours.", LANG_UNIVERSAL, 0);
                pPlayer->CLOSE_GOSSIP_MENU();
            }
            else
            {
                if (pPlayer->GetSession())
                    pPlayer->GetSession()->SendNotification("You don't have Cryptomancer's Ring or enough money!");
                pPlayer->CLOSE_GOSSIP_MENU();
            }
        }
        if (uiAction == GOSSIP_ACTION_INFO_DEF + 3)
        {
            pPlayer->SEND_GOSSIP_MENU(THAUMATURGE_TEXT_ID_3, pCreature->GetGUID());
        }
        return true;
    }
};

class npc_corastrasza_rogue_quest : public CreatureScript
{
public:
    npc_corastrasza_rogue_quest() : CreatureScript("npc_corastrasza_rogue_quest") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        if (pCreature->IsQuestGiver())
            pPlayer->PrepareQuestMenu(pCreature->GetGUID());

        if (pPlayer->GetQuestStatus(QUEST_A_HIDDEN_MESSAGE) == QUEST_STATUS_INCOMPLETE || pPlayer->GetQuestStatus(QUEST_A_HIDDEN_MESSAGE) == QUEST_STATUS_COMPLETE)
        {
            if (pPlayer->GetItemCount(ITEM_SINGED_CIPHER, true) == 0)
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Lord Afrasastrasz sent me. He said you have a message to decode?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        }
        pPlayer->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, pCreature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*sender*/, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
        {
            pPlayer->AddItem(ITEM_SINGED_CIPHER, 1);
            pPlayer->AddQuestObjectiveProgress(QUEST_A_HIDDEN_MESSAGE, 1, 1);
            pCreature->MonsterSay("Here's what's left of the message we found. Can you make any sense of it?", LANG_UNIVERSAL, 0);
            pPlayer->CLOSE_GOSSIP_MENU();
        }
        return true;
    }
};

enum RavenholdtSpells
{
    SPELL_INFILTRATING_RAVENHOLDT     = 106067,
    SPELL_INFILTRATING_GILNEAS_CITY   = 109157,
    SPELL_INFILTRATING_KARAZHAN       = 109376,
    SPELL_MOSTRASZ_VISION             = 106015,
    SPELL_EYE_OF_ZAZZO                = 109156,
    SPELL_EYE_OF_ZAZZO_KARAZHAN       = 109374,
    SPELL_HIGH_ALERT                  = 106198,
    SPELL_EYE_OF_THE_ARCANE           = 109366,
    SPELL_SAP                         = 6770,
    SPELL_STEALTH                     = 1784,

    // Lord Hiram Creed
    SPELL_CONSUMING_DARKNESS          = 109196,
    SPELL_BLACKHOWLS_WILL             = 109664,
    SPELL_SHADOW_BREATH               = 109669,
    SPELL_SHADOW_BREATH_NPC           = 109673,
    SPELL_BLACKHOWLS_RAGE             = 109706,
    SPELL_ENRAGE                      = 110055,
    SPELL_TRANSFORM_DRAGONKIN         = 109232,

    NPC_SHADOW_BREATH                 = 58002,
};

static const Position MostraszPos = { 14.9f, -1447.1f, 175.3f, 0.04f };
static const Position ZazzoPos = { -1392.0f , 1232.1f, 35.5f,  0.83f };
static const Position ZazzoPosKarazhan = { -11155.2f, -2143.2f, 62.0f, 1.28f };

class npc_ravenholdt_guards : public CreatureScript
{
public:
    npc_ravenholdt_guards() : CreatureScript("npc_ravenholdt_guards") { }

    CreatureAI* GetAI(Creature *_Creature) const
    {
        return new npc_ravenholdt_guardsAI(_Creature);
    }

    struct npc_ravenholdt_guardsAI : public ScriptedAI
    {
        npc_ravenholdt_guardsAI(Creature *c) : ScriptedAI(c)
        {
            m_startMovementTimer = 0;
        }

        TaskScheduler scheduler;

        uint32_t m_startMovementTimer;

        void Reset()
        {
            me->CastSpell(me, SPELL_HIGH_ALERT, true);
        }

        void MoveInLineOfSight(Unit *who) override
        {
            if (who->GetTypeId() == TYPEID_PLAYER && !who->ToPlayer()->IsGameMaster() && (who->GetPhaseMask() & me->GetPhaseMask()) != 0 && who->IsWithinLOSInMap(me, false))
            {
                if (!me->HasAura(SPELL_SAP) && who->HasAura(SPELL_MOSTRASZ_VISION))
                {
                    if (me->GetExactDist2d(who) < 7.0f)
                    {
                        who->RemoveAurasByType(SPELL_AURA_MOD_STEALTH);
                        who->RemoveAura(SPELL_INFILTRATING_RAVENHOLDT);
                        who->RemoveAura(SPELL_MOSTRASZ_VISION);
                        me->canAttack(who, true);
                        me->GetMotionMaster()->MoveChase(who);
                        me->Attack(who, true);
                    }
                }
            }
        }

        void DropCombatEngagement(uint64 targetGUID)
        {
            std::list<Unit*> nearUnits;
            Trinity::AnyUnitInObjectRangeCheck u_check(me, 30.0f);
            Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(me, nearUnits, u_check);
            me->VisitNearbyObject(30.0f, searcher);

            for (Unit* un : nearUnits)
            {
                if (un->GetTypeId() != TYPEID_UNIT)
                    continue;

                if (un->GetVictim() && un->GetVictim()->GetGUID() == targetGUID)
                {
                    un->CombatStop(true);
                    un->getThreatManager().clearReferences();
                    un->GetMotionMaster()->MoveTargetedHome();
                }
            }
        }

        void EnterCombat(Unit * /*who*/) override
        {
            Unit * target = me->GetVictim();
            if (!target)
                return;

            target->Unmount();
            target->RemoveAurasByType(SPELL_AURA_MOUNTED);
            target->RemoveAurasByType(SPELL_AURA_FLY);
            target->RemoveAurasByType(SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED);
            target->RemoveAurasByType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED);

            uint64 targetGUID = target->GetGUID();

            scheduler.Schedule(Seconds(6), [this, targetGUID](TaskContext /*context*/)
            {
                Unit* target = sObjectAccessor->FindUnit(targetGUID);

                DropCombatEngagement(targetGUID);

                me->CombatStop(true);
                if (target)
                    target->CombatStop(true);
                me->getThreatManager().clearReferences();
                me->GetMotionMaster()->MoveTargetedHome();

                if (target && me->GetDistance2d(target) < 10.0f)
                {
                    target->NearTeleportTo(MostraszPos.GetPositionX(), MostraszPos.GetPositionY(), MostraszPos.GetPositionZ(), MostraszPos.GetOrientation());
                    target->ToPlayer()->RemoveSpellCooldown(SPELL_STEALTH);
                    target->CastSpell(target, SPELL_STEALTH, true);
                    target->CastSpell(target, SPELL_MOSTRASZ_VISION, true);
                    target->CastSpell(target, SPELL_INFILTRATING_RAVENHOLDT, true);
                }
            });
        }

        void UpdateAI(const uint32 diff)
        {
            scheduler.Update(diff);

            if (!me->isMoving())
            {
                if (!me->IsInCombat() && me->CanFreeMove() && m_startMovementTimer == 0)
                    m_startMovementTimer = 5000;
                else if (m_startMovementTimer != 0)
                {
                    if (m_startMovementTimer <= diff)
                    {
                        if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == WAYPOINT_MOTION_TYPE)
                            ((WaypointMovementGenerator<Creature>*)me->GetMotionMaster()->top())->StartMoveNow(me);
                        m_startMovementTimer = 0;
                    }
                    else
                        m_startMovementTimer -= diff;
                }
            }

            if (!me->GetVictim())
                return;

            if (me->GetVictim()->GetDistance(me) > 30.0f)
            {
                me->CombatStop(true);
                me->getThreatManager().clearReferences();
                me->GetMotionMaster()->MoveTargetedHome();
                return;
            }

            DoMeleeAttackIfReady();
        }
    };
};

#define GILNEAS_SHOUT_COUNT 8

static const char* guardShouts[GILNEAS_SHOUT_COUNT] = {
    "What have we here ?",
    "I see you...",
    "Protect Lord Creed!",
    "Die, assassin!",
    "Over here - an assassin!",
    "Take him out!",
    "Intruder!",
    "Show yourself!"
};

class npc_gilneas_guards : public CreatureScript
{
public:
    npc_gilneas_guards() : CreatureScript("npc_gilneas_guards") { }

    CreatureAI* GetAI(Creature *_Creature) const
    {
        return new npc_gilneas_guardsAI(_Creature);
    }

    struct npc_gilneas_guardsAI : public ScriptedAI
    {
        npc_gilneas_guardsAI(Creature *c) : ScriptedAI(c)
        {
            m_startMovementTimer = 0;
        }

        TaskScheduler scheduler;

        uint32_t m_startMovementTimer;

        void Reset()
        {
            me->CastSpell(me, SPELL_HIGH_ALERT, true);
        }

        void MoveInLineOfSight(Unit *who) override
        {
            if (who->GetTypeId() == TYPEID_PLAYER && !who->ToPlayer()->IsGameMaster() && (who->GetPhaseMask() & me->GetPhaseMask()) != 0 && who->IsWithinLOSInMap(me, false))
            {
                if (!me->HasAura(SPELL_SAP) && who->HasAura(SPELL_EYE_OF_ZAZZO))
                {
                    if (me->GetExactDist2d(who) < 7.0f)
                    {
                        who->RemoveAurasByType(SPELL_AURA_MOD_STEALTH);
                        who->RemoveAura(SPELL_INFILTRATING_GILNEAS_CITY);
                        who->RemoveAura(SPELL_EYE_OF_ZAZZO);
                        me->canAttack(who, true);
                        me->GetMotionMaster()->MoveChase(who);
                        me->Attack(who, true);
                    }
                }
            }
        }

        void DropCombatEngagement(uint64 targetGUID)
        {
            std::list<Unit*> nearUnits;
            Trinity::AnyUnitInObjectRangeCheck u_check(me, 30.0f);
            Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(me, nearUnits, u_check);
            me->VisitNearbyObject(30.0f, searcher);

            for (Unit* un : nearUnits)
            {
                if (un->GetTypeId() != TYPEID_UNIT)
                    continue;

                if (un->GetVictim() && un->GetVictim()->GetGUID() == targetGUID)
                {
                    un->CombatStop(true);
                    un->getThreatManager().clearReferences();
                    un->GetMotionMaster()->MoveTargetedHome();
                }
            }
        }

        void EnterCombat(Unit * /*who*/) override
        {
            Unit * target = me->GetVictim();
            if (!target)
                return;
            
            // if we are not a dog, shout some random aggro stuff
            if (me->GetEntry() != 57861)
                me->MonsterSay(guardShouts[urand(0, GILNEAS_SHOUT_COUNT - 1)], LANG_UNIVERSAL, 0);

            target->Unmount();
            target->RemoveAurasByType(SPELL_AURA_MOUNTED);
            target->RemoveAurasByType(SPELL_AURA_FLY);
            target->RemoveAurasByType(SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED);
            target->RemoveAurasByType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED);

            uint64 targetGUID = target->GetGUID();

            scheduler.Schedule(Seconds(6), [this, targetGUID](TaskContext /*context*/)
            {
                Unit* target = sObjectAccessor->FindUnit(targetGUID);

                DropCombatEngagement(targetGUID);

                me->CombatStop(true);
                if (target)
                    target->CombatStop(true);
                me->getThreatManager().clearReferences();
                me->GetMotionMaster()->MoveTargetedHome();

                if (target && me->GetDistance2d(target) < 10.0f)
                {
                    target->NearTeleportTo(ZazzoPos.GetPositionX(), ZazzoPos.GetPositionY(), ZazzoPos.GetPositionZ(), ZazzoPos.GetOrientation());
                    target->ToPlayer()->RemoveSpellCooldown(SPELL_STEALTH);
                    target->CastSpell(target, SPELL_STEALTH, true);
                    target->CastSpell(target, SPELL_EYE_OF_ZAZZO, true);
                    target->CastSpell(target, SPELL_INFILTRATING_GILNEAS_CITY, true);
                }
            });
        }

        void UpdateAI(const uint32 diff)
        {
            scheduler.Update(diff);

            if (!me->isMoving())
            {
                if (!me->IsInCombat() && me->CanFreeMove() && m_startMovementTimer == 0)
                    m_startMovementTimer = 5000;
                else if (m_startMovementTimer != 0)
                {
                    if (m_startMovementTimer <= diff)
                    {
                        if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == WAYPOINT_MOTION_TYPE)
                            ((WaypointMovementGenerator<Creature>*)me->GetMotionMaster()->top())->StartMoveNow(me);
                        m_startMovementTimer = 0;
                    }
                    else
                        m_startMovementTimer -= diff;
                }
            }

            if (!me->GetVictim())
                return;

            if (me->GetVictim()->GetDistance(me) > 30.0f)
            {
                me->CombatStop(true);
                me->getThreatManager().clearReferences();
                me->GetMotionMaster()->MoveTargetedHome();
                return;
            }

            DoMeleeAttackIfReady();
        }
    };
};

class npc_lord_hiram_creed : public CreatureScript
{
public:
    npc_lord_hiram_creed() : CreatureScript("npc_lord_hiram_creed") {}

    CreatureAI * GetAI(Creature * pCreature) const
    {
        return new npc_lord_hiram_creedAI(pCreature);
    }

    struct npc_lord_hiram_creedAI : public ScriptedAI
    {
        npc_lord_hiram_creedAI(Creature * creature) : ScriptedAI(creature) {}

        TaskScheduler scheduler;
        uint32 consumingDarknessTimer;
        uint32 blackhowlsWillTimer;
        uint32 shadowBreathTimer;
        uint32 blackhowlsRageTimer;
        bool phaseChange;
        bool enrage;
        bool shadowBreath;

        inline bool IsCastingAllowed()
        {
            return !me->IsNonMeleeSpellCasted(false);
        }

        void Reset() override
        {
            consumingDarknessTimer = 7000;
            blackhowlsWillTimer = 10000;
            blackhowlsRageTimer = 120000;
            enrage = false;
            phaseChange = false;
            me->RemoveAura(SPELL_TRANSFORM_DRAGONKIN);
            me->RemoveAura(SPELL_ENRAGE);
        }

        void EnterCombat(Unit * /*who*/) override
        {
            me->MonsterSay("An assassin, eh ? Who sent you ? Speak, or I will carve the answer from your skull!", LANG_UNIVERSAL, 0);
        }

        void JustDied(Unit * /*who*/) override
        {
            // get players within 50y
            float radius = 50.0f;
            std::list<Player*> players;
            Trinity::AnyPlayerInObjectRangeCheck checker(me, radius);
            Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(me, players, checker);
            me->VisitNearbyWorldObject(radius, searcher);

            // cast Teleport Back Timing Aura
            for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                (*itr)->CastSpell(*itr, 109383, true);

            me->MonsterSay("Who... ordered... this?", LANG_UNIVERSAL, 0);
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (!UpdateVictim())
                return;

            if (me->GetHealthPct() <= 75 && phaseChange == false)
            {
                phaseChange = true;
                shadowBreathTimer = 10000;
                shadowBreath = true;
                me->CastSpell(me, SPELL_TRANSFORM_DRAGONKIN, false);
                me->MonsterSay("I tire of this disguise. Now we finish this!", LANG_UNIVERSAL, 0);
            }

            // Timers
            if (consumingDarknessTimer <= diff)
            {
                if (IsCastingAllowed())
                {
                    me->CastSpell(me->GetVictim(), SPELL_CONSUMING_DARKNESS, false);
                    consumingDarknessTimer = 10000;
                }
            }
            else consumingDarknessTimer -= diff;

            if (blackhowlsWillTimer <= diff)
            {
                if (IsCastingAllowed())
                {
                    me->CastSpell(me->GetVictim(), SPELL_BLACKHOWLS_WILL, false);
                    blackhowlsWillTimer = 12000;
                }
            }
            else blackhowlsWillTimer -= diff;

            if (blackhowlsRageTimer <= diff)
            {
                if (!enrage)
                {
                    me->CastSpell(me, SPELL_BLACKHOWLS_RAGE, true);
                    enrage = true;
                }
                else
                    me->CastSpell(me, SPELL_ENRAGE, true);
                blackhowlsRageTimer = 120000;
            }
            else blackhowlsRageTimer -= diff;

            if (phaseChange)
            {
                if (shadowBreathTimer <= diff)
                {
                    if (IsCastingAllowed())
                    {
                        me->CastSpell(me->GetVictim(), SPELL_SHADOW_BREATH_NPC, false);
                        consumingDarknessTimer += 1500;
                        blackhowlsWillTimer += 1500;
                        scheduler.Schedule(Milliseconds(1100), [this](TaskContext /*context*/)
                        {
                            if (Creature * pShadowBreath = me->FindNearestCreature(NPC_SHADOW_BREATH, 200.0f, true))
                            {
                                pShadowBreath->SetOrientation(me->GetOrientation());
                                me->AddThreat(pShadowBreath, 10000000.0f);
                                me->CastSpell(pShadowBreath, SPELL_SHADOW_BREATH, false);
                            }
                        });
                        scheduler.Schedule(Milliseconds(4100), [this](TaskContext /*context*/)
                        {
                            if (Creature * pShadowBreath = me->FindNearestCreature(NPC_SHADOW_BREATH, 200.0f, true))
                                me->Kill(pShadowBreath);
                        });
                        shadowBreathTimer = 10000;
                    }
                }
                else shadowBreathTimer -= diff;
            }

            DoMeleeAttackIfReady();
        }
    };
};

// SHADOW BREATH
class npc_lord_hiram_creed_shadow_breath : public CreatureScript
{
public:
    npc_lord_hiram_creed_shadow_breath() : CreatureScript("npc_lord_hiram_creed_shadow_breath") {}

    CreatureAI * GetAI(Creature * pCreature) const
    {
        return new npc_lord_hiram_creed_shadow_breathAI(pCreature);
    }

    struct npc_lord_hiram_creed_shadow_breathAI : public ScriptedAI
    {
        npc_lord_hiram_creed_shadow_breathAI(Creature * creature) : ScriptedAI(creature) {}

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);
        }
    };
};

enum NaliceSpells
{
    SPELL_GROUND_ARCANE_PRE_EXPLOSION = 109836, // casted by ground triggers two/three times before explosion
    SPELL_GROUND_ARCANE_EXPLOSION = 110001,     // casted by ground triggers after few seconds
    SPELL_ARCANE_DESTRUCTION_PRECAST = 110038,  // 1.5s cast leading to channeled spell
    SPELL_ARCANE_SHIELD = 109912,               // casted when channelling pillars
    SPELL_ARCANE_DESTRUCTION_CHANNEL = 109951,  // channelling pillars (18s), if successfully channelled, destroy players
    SPELL_ARCANE_DESTRUCTION_CHANNEL_2 = 109909,// channelling pillars (20s), if successfully channelled, destroy players
    SPELL_ARCANE_DESTRUCTION_DESTROY = 109931,  // destroys all nearby players
    SPELL_NALICE_ENRAGE_PRECAST = 109890,       // 1.5s cast of enrage
    SPELL_NALICE_ENRAGE = 109889,               // casted every approx 17-19s
    SPELL_ARCANE_INFUSED_WEAPON_PRECAST = 109887,   // 1.5s cast leading to aura apply
    SPELL_ARCANE_INFUSED_WEAPON = 109886,       // casted after 1.5s cast, every approx. 18s (8s from start of the fight)
    SPELL_NALICE_ARCANE_MISSILES = 109901,      // casted 5-10s after enrage
    SPELL_ARCANE_INFUSED_ARMOR_PRECAST = 109893,    // 1.5s cast leading to aura apply
    SPELL_ARCANE_INFUSED_ARMOR = 109892,        // casted every approx. 30s in stage 2

    SPELL_DESTR_TELEPORT_LEFT = 109903,         // casted when starting channelling left side
    SPELL_DESTR_TELEPORT_RIGHT = 109953,        // casted when starting channelling right side
    SPELL_DESTR_TELEPORT_SELF_LEFT = 108497,    // casted on self when starting channelling left side
    SPELL_DESTR_TELEPORT_SELF_RIGHT = 108498,    // casted on self when starting channelling left side

    SPELL_PILLAR_DESTROYED = 109940,            // when the pillar gets destroyed, this is casted on stalker
    SPELL_ARCANE_POWER = 109965,                // casted on player after destroying pillar?

    SPELL_BLAZING_SHADOWS_PRECAST = 109988,     // 1.5s cast leading to aura apply
    SPELL_BLAZING_SHADOWS_PLDEBUFF = 109970,    // casted on player after precast is done
    SPELL_BLAZING_SHADOWS_DMG = 109981,         // casted by invisible NPCs as damaging aura

    SPELL_NALICE_TRANSFORM_DEATH = 110464,      // casted when killed (transform to dragon)
};

enum NaliceEncounterActions
{
    NACTION_PILLAR_CLICKED = 1,                 // pillar GO -> Nalice : pillar has been clicked
    NACTION_DO_NOT_EXPLODE = 2,                 // Nalice -> arcane explosions stalkers : do not cause damage, it's over
};

enum NaliceNPCs
{
    NPC_NALICE = 57910,
    NPC_BLAZING_SHADOWS = 58148,
    NPC_ARCANE_PILLAR_STALKER_LEFT = 310000,
    NPC_ARCANE_PILLAR_STALKER_RIGHT = 310001,
    NPC_ARCANE_PILLAR_STALKER_FLAMES = 310002,
    NPC_ARCANE_EXPLOSION_GROUND = 310003,
    NPC_NALICE_QUEST_CHECKER = 310004,
};

class npc_nalice : public CreatureScript
{
    public:
        npc_nalice() : CreatureScript("npc_nalice") {}

        CreatureAI * GetAI(Creature * pCreature) const
        {
            return new npc_naliceAI(pCreature);
        }

        struct npc_naliceAI : public ScriptedAI
        {
            npc_naliceAI(Creature * creature) : ScriptedAI(creature)
            {
                Reset();
            }

            uint32 arcaneInfusedWeaponTimer;
            uint32 enrageTimer;
            uint32 arcaneMissilesTimer;
            uint32 arcaneInfusedArmorTimer;
            uint32 blazingShadowsTimer;

            int32 arcaneDestructionPhase;
            uint32 arcaneDestructionFloorTimer;

            bool pauseRegulars;
            bool destructionLeft;
            bool floorTilesStyle;
            bool destroyEverythingAfterCast;
            bool beforeDeathSpeak;

            uint8 pillarClicks;

            void Reset() override
            {
                arcaneInfusedWeaponTimer = 8000;
                enrageTimer = 17000;
                arcaneMissilesTimer = 30000;
                arcaneInfusedArmorTimer = 30000;
                arcaneDestructionPhase = 0;
                arcaneDestructionFloorTimer = 0;
                blazingShadowsTimer = 6000;
                pauseRegulars = false;
                destructionLeft = true;
                floorTilesStyle = true;
                destroyEverythingAfterCast = false;
                pillarClicks = 0;
                beforeDeathSpeak = false;

                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK, false);
                me->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_MASK_ALL, false);
            }

            void ResetPillars()
            {
                std::list<Creature*> stalkerList;
                GetCreatureListWithEntryInGrid(stalkerList, me, NPC_ARCANE_PILLAR_STALKER_FLAMES, 500.0f);
                for (Creature* cr : stalkerList)
                    cr->ForcedDespawn();
                GetCreatureListWithEntryInGrid(stalkerList, me, NPC_ARCANE_PILLAR_STALKER_LEFT, 500.0f);
                for (Creature* cr : stalkerList)
                    cr->ForcedDespawn();
                GetCreatureListWithEntryInGrid(stalkerList, me, NPC_ARCANE_PILLAR_STALKER_RIGHT, 500.0f);
                for (Creature* cr : stalkerList)
                    cr->ForcedDespawn();

                std::list<GameObject*> pillarList;
                GetGameObjectListWithEntryInGrid(pillarList, me, 310000, 500.0f);
                for (GameObject* go : pillarList)
                {
                    go->UseDoorOrButton(0);
                    go->EnableCollision(false);
                    // left side
                    if (go->GetPositionY() > -2184.0f)
                        me->SummonCreature(310000, go->GetPositionX(), go->GetPositionY(), go->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN, 0);
                    else // right side
                        me->SummonCreature(310001, go->GetPositionX(), go->GetPositionY(), go->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN, 0);
                }
            }

            void ClearGroundExplosions()
            {
                std::list<Creature*> stalkerList;
                GetCreatureListWithEntryInGrid(stalkerList, me, NPC_ARCANE_EXPLOSION_GROUND, 500.0f);
                for (Creature* cr : stalkerList)
                    cr->GetAI()->DoAction(NACTION_DO_NOT_EXPLODE);
            }

            void EnterCombat(Unit* who) override
            {
                ResetPillars();

                me->MonsterSay("An assassin! Did the little whelpling send you after me? Come, rogue. Let's dance.", LANG_UNIVERSAL, 0);
            }

            void JustDied(Unit* killer) override
            {
                me->CastSpell(me, SPELL_NALICE_TRANSFORM_DEATH, true);
                me->MonsterSay("You cling to a shattered world... Your time comes... soon enough...", LANG_UNIVERSAL, 0);

                std::list<Creature*> flameList;
                GetCreatureListWithEntryInGrid(flameList, me, NPC_BLAZING_SHADOWS, 100.0f);
                for (Creature* cr : flameList)
                    cr->ForcedDespawn();

                me->SummonCreature(NPC_NALICE_QUEST_CHECKER, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 60000);
            }

            void UpdateAI(const uint32 diff) override
            {
                ScriptedAI::UpdateAI(diff);

                if (!UpdateVictim())
                    return;

                if (!beforeDeathSpeak && me->GetHealthPct() < 9.0f)
                {
                    me->MonsterSay("This isn't how it ends. Not for me. I'm a survivor.", LANG_UNIVERSAL, 0);
                    beforeDeathSpeak = true;
                }

                if (2 - arcaneDestructionPhase > me->GetHealthPct() / 33.3f)
                {
                    if (!me->IsNonMeleeSpellCasted(false))
                    {
                        if (pauseRegulars)
                        {
                            std::list<GameObject*> pillarList;
                            GetGameObjectListWithEntryInGrid(pillarList, me, 310000, 500.0f);
                            for (GameObject* go : pillarList)
                            {
                                // left side
                                if (go->GetPositionY() > -2184.0f)
                                {
                                    if (destructionLeft)
                                        go->ResetDoorOrButton();
                                }
                                // right side
                                else if (!destructionLeft)
                                    go->ResetDoorOrButton();
                            }

                            TeleportToChannelPos(destructionLeft);
                            me->CastSpell(me, SPELL_ARCANE_SHIELD, true);
                            me->CastSpell(me, destructionLeft ? SPELL_ARCANE_DESTRUCTION_CHANNEL_2 : SPELL_ARCANE_DESTRUCTION_CHANNEL, false);
                            destructionLeft = !destructionLeft;
                            arcaneDestructionPhase++;
                            arcaneDestructionFloorTimer = 1000;
                            pillarClicks = 0;
                            destroyEverythingAfterCast = true;
                        }
                        else
                        {
                            me->RemoveAllNegativeAuras();
                            me->MonsterTextEmote("|TInterface\\ICONS\\Spell_arcane_arcaneresilience:16|t Nalice begins to cast |cffff0000|Hspell:109909|h[Arcane Destruction]|h|r! Activate the pillars to stop her!", 0, true);
                            if (destructionLeft)
                                me->MonsterSay("I've got some secrets of my own, rogue.", LANG_UNIVERSAL, 0);
                            else
                                me->MonsterSay("I ... I need more time!", LANG_UNIVERSAL, 0);

                            pillarClicks = 0;
                            pauseRegulars = true;
                            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK, true);
                            me->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_MASK_ALL, true);
                            me->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_MASK_MAGIC, true);
                            me->CastSpell(me, SPELL_ARCANE_DESTRUCTION_PRECAST, false);
                        }
                    }
                }

                if (arcaneDestructionFloorTimer)
                {
                    if (arcaneDestructionFloorTimer <= diff)
                    {
                        SpawnFloorTiles(!destructionLeft, floorTilesStyle);
                        floorTilesStyle = !floorTilesStyle;
                        arcaneDestructionFloorTimer = 2600;
                    }
                    else
                        arcaneDestructionFloorTimer -= diff;
                }

                if (destroyEverythingAfterCast)
                {
                    if (!me->IsNonMeleeSpellCasted(false))
                    {
                        me->RemoveAurasDueToSpell(SPELL_ARCANE_SHIELD);
                        me->CastSpell(me, SPELL_ARCANE_DESTRUCTION_DESTROY, false);
                        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK, false);
                        me->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_MASK_ALL, false);
                        me->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_MASK_MAGIC, false);
                        destroyEverythingAfterCast = false;
                        pauseRegulars = false;
                        arcaneDestructionFloorTimer = 0;
                    }
                }

                // should we pause regulars due to pillar channelling?
                if (!pauseRegulars)
                {
                    // Arcane Infused Weapon
                    if (arcaneInfusedWeaponTimer <= diff)
                    {
                        if (CanCastRegular(me, SPELL_ARCANE_INFUSED_WEAPON_PRECAST, false))
                        {
                            me->MonsterTextEmote("|TInterface\\ICONS\\Inv_weapon_shortblade_45:16|t Nalice begins to cast |cffa335ee|Hspell:109887|h[Arcane Infused Weapon]|h|r! Avoid her attacks!", 0, true);
                            me->CastSpell(me, SPELL_ARCANE_INFUSED_WEAPON, true);
                            me->CastSpell(me, SPELL_ARCANE_INFUSED_WEAPON_PRECAST, false);
                            arcaneInfusedWeaponTimer = 19000 + urand(0, 3000);
                        }
                    }
                    else
                        arcaneInfusedWeaponTimer -= diff;

                    // Enrage
                    if (enrageTimer <= diff)
                    {
                        if (CanCastRegular(me, SPELL_NALICE_ENRAGE_PRECAST, false))
                        {
                            me->CastSpell(me, SPELL_NALICE_ENRAGE, true);
                            me->CastSpell(me, SPELL_NALICE_ENRAGE_PRECAST, false);
                            enrageTimer = 17000 + urand(0, 3000);

                            arcaneMissilesTimer = 5000 + urand(0, 5000);
                        }
                    }
                    else
                        enrageTimer -= diff;

                    // Arcane Missiles
                    if (arcaneMissilesTimer <= diff)
                    {
                        if (CanCastRegular(me->GetVictim(), SPELL_NALICE_ARCANE_MISSILES, false))
                        {
                            me->CastSpell(me->GetVictim(), SPELL_NALICE_ARCANE_MISSILES, false);
                            arcaneMissilesTimer = 30000;
                        }
                    }
                    else
                        arcaneMissilesTimer -= diff;

                    // -- Stage 2 spells
                    if (arcaneDestructionPhase >= 1)
                    {
                        // Arcane Infused Armor
                        if (arcaneInfusedArmorTimer <= diff)
                        {
                            if (CanCastRegular(me, SPELL_ARCANE_INFUSED_ARMOR_PRECAST, false))
                            {
                                me->MonsterTextEmote("|TInterface\\ICONS\\Ability_warrior_riposte:16|t Nalice begins to cast |cffff0000|Hspell:109893|h[Arcane Infused Armor]|h|r! Expose her armor!", 0, true);
                                me->CastSpell(me, SPELL_ARCANE_INFUSED_ARMOR, true);
                                me->CastSpell(me, SPELL_ARCANE_INFUSED_ARMOR_PRECAST, false);
                                arcaneInfusedArmorTimer = 28000 + urand(0, 4000);
                            }
                        }
                        else
                            arcaneInfusedArmorTimer -= diff;
                    }

                    // -- Stage 3 spells
                    if (arcaneDestructionPhase >= 2)
                    {
                        // Blazing Shadows
                        if (blazingShadowsTimer <= diff)
                        {
                            if (CanCastRegular(me->GetVictim(), SPELL_BLAZING_SHADOWS_PRECAST, false))
                            {
                                me->CastSpell(me->GetVictim(), SPELL_BLAZING_SHADOWS_PRECAST, false);
                                blazingShadowsTimer = 30000 + urand(0, 3000);
                            }
                        }
                        else
                            blazingShadowsTimer -= diff;
                    }

                    DoMeleeAttackIfReady();
                }
            }

            void DoAction(const int32 action) override
            {
                if (action == NACTION_PILLAR_CLICKED)
                {
                    pillarClicks++;

                    if (pillarClicks >= 2)
                    {
                        me->RemoveAurasDueToSpell(SPELL_ARCANE_SHIELD);
                        destroyEverythingAfterCast = false;
                        me->InterruptNonMeleeSpells(true);
                        pauseRegulars = false;
                        arcaneDestructionFloorTimer = 0;
                        ClearGroundExplosions();
                        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK, false);
                        me->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_MASK_ALL, false);
                        me->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_MASK_MAGIC, false);
                    }
                }
            }

            void DamageTaken(Unit* who, uint32 &damage) override
            {
                if (me->HasAura(SPELL_ARCANE_SHIELD))
                    damage = 0;
            }

            void SpellCastFailed(const SpellEntry* spell) override
            {
                // when interrupted cast of Blazing Shadows (Kick, stun, ..), recast after stun duration
                if (spell && spell->Id == SPELL_BLAZING_SHADOWS_PRECAST)
                    blazingShadowsTimer = 1000;
            }

            void SpellHitTarget(Unit* target, const SpellEntry* spell) override
            {
                if (spell && spell->Id == SPELL_BLAZING_SHADOWS_PRECAST)
                    me->CastSpell(target, SPELL_BLAZING_SHADOWS_PLDEBUFF, true);
            }

            void SpellHit(Unit* caster, const SpellEntry* spell) override
            {
                // Expose Armor should drop Arcane Infused Armor effect
                if (spell && spell->Id == 8647)
                    me->RemoveAurasDueToSpell(SPELL_ARCANE_INFUSED_ARMOR);
            }

            void TeleportToChannelPos(bool left)
            {
                if (left)
                {
                    me->CastSpell(me, SPELL_DESTR_TELEPORT_LEFT, true);
                    //me->NearTeleportTo(-11384.595703f, -2208.664551f, 23.097633f, 1.457203f);
                    me->CastSpell(me, SPELL_DESTR_TELEPORT_SELF_LEFT, false);
                }
                else
                {
                    me->CastSpell(me, SPELL_DESTR_TELEPORT_RIGHT, true);
                    //me->NearTeleportTo(-11378.373047f, -2157.664551f, 23.131664f, 4.600522f);
                    me->CastSpell(me, SPELL_DESTR_TELEPORT_SELF_RIGHT, false);
                }
            }

            void SpawnFloorTiles(bool left, bool style)
            {
                float startX, startY, vectorX_a, vectorY_a;
                float vectorX_b, vectorY_b;
                const float zpos = 23.1f;

                // left
                if (left)
                {
                    startX = -11398.418945f;
                    startY = -2150.076904f;
                    vectorX_a = (-11403.284180f - (-11398.418945f));
                    vectorY_a = (-2192.479248f - (-2150.076904f));
                    vectorX_b = (-11355.325195f - (-11398.418945f));
                    vectorY_b = (-2152.714111f - (-2150.076904f));
                }
                // right
                else
                {
                    startX = -11361.044922f;
                    startY = -2218.927002f;
                    vectorX_a = (-11358.394531f - (-11361.044922f));
                    vectorY_a = (-2172.589844f - (-2218.927002f));
                    vectorX_b = (-11405.269531f - (-11361.044922f));
                    vectorY_b = (-2213.935303f - (-2218.927002f));
                }

                const uint32 count_a = 8;
                const uint32 count_b = 8;

                float stepX_a = vectorX_a / (float)count_a;
                float stepY_a = vectorY_a / (float)count_a;

                float stepX_b = vectorX_b / (float)count_b;
                float stepY_b = vectorY_b / (float)count_b;

                float posX, posY;
                for (uint32 i = 0; i < count_a; i++)
                {
                    posX = startX + stepX_a * (float)i;
                    posY = startY + stepY_a * (float)i;

                    for (uint32 j = 0; j < count_b; j++)
                    {
                        if ((i % 2 == j % 2) ^ style)
                            me->SummonCreature(NPC_ARCANE_EXPLOSION_GROUND, posX + stepX_b * (float)j, posY + stepY_b * (float)j, zpos, 0.0f, TEMPSUMMON_DEAD_DESPAWN, 0);
                    }
                }
            }
        };
};

class go_nalice_arcane_pillar : public GameObjectScript
{
    public:
        go_nalice_arcane_pillar() : GameObjectScript("go_nalice_arcane_pillar") { }

        bool OnGossipHello(Player *pPlayer, GameObject *pGo)
        {
            if (pPlayer)
                pPlayer->CastSpell(pPlayer, SPELL_ARCANE_POWER, true);

            Creature* cr = pGo->SummonCreature(NPC_ARCANE_PILLAR_STALKER_FLAMES, pGo->GetPositionX(), pGo->GetPositionY(), pGo->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN, 0);
            if (cr)
                cr->CastSpell(cr, SPELL_PILLAR_DESTROYED, true);

            Creature* boss = GetClosestCreatureWithEntry(pGo, NPC_NALICE, 100.0f);
            if (boss)
                boss->GetAI()->DoAction(NACTION_PILLAR_CLICKED);

            pGo->UseDoorOrButton(0);

            return true;
        }
};

class npc_nalice_ground_explosion : public CreatureScript
{
    public:
        npc_nalice_ground_explosion() : CreatureScript("npc_nalice_ground_explosion") {}

        CreatureAI * GetAI(Creature * pCreature) const
        {
            return new npc_nalice_ground_explosionAI(pCreature);
        }

        struct npc_nalice_ground_explosionAI : public Scripted_NoMovementAI
        {
            npc_nalice_ground_explosionAI(Creature * creature) : Scripted_NoMovementAI(creature)
            {
                generalPhase = 0;
                generalTimer = 300;
                doNotExplode = false;
            }

            uint32 generalTimer;
            uint8 generalPhase;
            bool doNotExplode;

            void DoAction(const int32 action) override
            {
                if (action == NACTION_DO_NOT_EXPLODE)
                    doNotExplode = true;
            }

            void UpdateAI(const uint32 diff) override
            {
                if (generalTimer <= diff)
                {
                    switch (generalPhase)
                    {
                        case 0: // first warning
                            me->CastSpell(me, SPELL_GROUND_ARCANE_PRE_EXPLOSION, true);
                            generalTimer = 150;
                            generalPhase++;
                            break;
                        case 1: // second warning
                            me->CastSpell(me, SPELL_GROUND_ARCANE_PRE_EXPLOSION, true);
                            generalTimer = 150;
                            generalPhase++;
                            break;
                        case 2: // third warning
                            me->CastSpell(me, SPELL_GROUND_ARCANE_PRE_EXPLOSION, true);
                            generalTimer = 500;
                            generalPhase++;
                            break;
                        case 3: // boom!
                            if (!doNotExplode)
                                me->CastSpell(me, SPELL_GROUND_ARCANE_EXPLOSION, true);
                            generalTimer = 800;
                            generalPhase++;
                            break;
                        case 4: // despawn
                        default:
                            me->Kill(me);
                            me->ForcedDespawn();
                            break;
                    }
                }
                else
                    generalTimer -= diff;
            }
        };
};

class npc_nalice_blazing_shadows : public CreatureScript
{
    public:
        npc_nalice_blazing_shadows() : CreatureScript("npc_nalice_blazing_shadows") {}

        CreatureAI * GetAI(Creature * pCreature) const
        {
            return new npc_nalice_blazing_shadowsAI(pCreature);
        }

        struct npc_nalice_blazing_shadowsAI : public Scripted_NoMovementAI
        {
            npc_nalice_blazing_shadowsAI(Creature * creature) : Scripted_NoMovementAI(creature)
            {
                debuffTimer = 1000;
            }

            uint32 debuffTimer;

            void UpdateAI(const uint32 diff) override
            {
                if (debuffTimer)
                {
                    if (debuffTimer <= diff)
                    {
                        me->CastSpell(me, SPELL_BLAZING_SHADOWS_DMG, true);
                        debuffTimer = 0;
                    }
                    else
                        debuffTimer -= diff;
                }
            }
        };
};

class npc_nalice_quest_checker : public CreatureScript
{
    public:
        npc_nalice_quest_checker() : CreatureScript("npc_nalice_quest_checker") {}

        CreatureAI * GetAI(Creature * pCreature) const
        {
            return new npc_nalice_quest_checkerAI(pCreature);
        }

        struct npc_nalice_quest_checkerAI : public Scripted_NoMovementAI
        {
            npc_nalice_quest_checkerAI(Creature * creature) : Scripted_NoMovementAI(creature)
            {
                lootCheckTimer = 3000;
            }

            uint32 lootCheckTimer;

            void UpdateAI(const uint32 diff) override
            {
                if (lootCheckTimer <= diff)
                {
                    // get players within 100y
                    float radius = 100.0f;
                    std::list<Player*> players;
                    Trinity::AnyPlayerInObjectRangeCheck checker(me, radius);
                    Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(me, players, checker);
                    me->VisitNearbyWorldObject(radius, searcher);

                    uint8 teleported = 0;
                    for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    {
                        if ((*itr)->HasItemCount(77954, 1))
                        {
                            (*itr)->NearTeleportTo(-11158.28f, -2149.3506f, 64.9f, 2.28f);
                            (*itr)->CastSpell(*itr, 51347, true);
                        }
                    }

                    // if all players has been teleported, despawn; also occurs when no players in range
                    if (players.size() == teleported)
                        me->ForcedDespawn();

                    lootCheckTimer = 3000;
                }
                else
                    lootCheckTimer -= diff;
            }
        };
};

class npc_wrathion_quest_checker : public CreatureScript
{
public:
    npc_wrathion_quest_checker() : CreatureScript("npc_wrathion_quest_checker") {}

    CreatureAI * GetAI(Creature * pCreature) const
    {
        return new npc_wrathion_quest_checkerAI(pCreature);
    }

    struct npc_wrathion_quest_checkerAI : public Scripted_NoMovementAI
    {
        npc_wrathion_quest_checkerAI(Creature * creature) : Scripted_NoMovementAI(creature) 
        {
            checkTimer = 1000;
        }

        uint32 checkTimer;

        void UpdateAI(const uint32 diff) override
        {
            if (checkTimer <= diff)
            {
                // get players within 300y
                float radius = 300.0f;
                std::list<Player*> players;
                Trinity::AnyPlayerInObjectRangeCheck checker(me, radius);
                Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(me, players, checker);
                me->VisitNearbyWorldObject(radius, searcher);

                for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                {
                    // If player has quest Patricide! add aura for correct phase 
                    if (((*itr)->GetQuestStatus(30118) == QUEST_STATUS_COMPLETE) && ((*itr)->GetItemCount(77949, true) == 0))
                    {
                        if (!(*itr)->HasAura(69078))
                        {
                            // Switch phase
                            (*itr)->RemoveAura(59074);
                            (*itr)->AddAura(69078, *itr);
                        }
                    }
                }
                checkTimer = 3000;
            }
            else checkTimer -= diff;
        }
    };
};

const uint8 KARAZHAN_SHOUT_COUNT = 6;

static const char* guardKarazhanShouts[KARAZHAN_SHOUT_COUNT] = {
    "Dirty sneak, you will be destroyed!",
    "I see you...",
    "I can smell you, thief!",
    "Over here - an assassin!",
    "Fool!",
    "Terminate him!",
};

class npc_karazhan_guards : public CreatureScript
{
public:
    npc_karazhan_guards() : CreatureScript("npc_karazhan_guards") { }

    CreatureAI* GetAI(Creature *_Creature) const
    {
        return new npc_karazhan_guardsAI(_Creature);
    }

    struct npc_karazhan_guardsAI : public ScriptedAI
    {
        npc_karazhan_guardsAI(Creature *c) : ScriptedAI(c)
        {
            m_startMovementTimer = 0;
        }

        TaskScheduler scheduler;
        uint32_t m_startMovementTimer;

        void Reset()
        {
            me->CastSpell(me, SPELL_EYE_OF_THE_ARCANE, true);
        }

        void MoveInLineOfSight(Unit *who) override
        {
            if (who->GetTypeId() == TYPEID_PLAYER && !who->ToPlayer()->IsGameMaster() && (who->GetPhaseMask() & me->GetPhaseMask()) != 0 && who->IsWithinLOSInMap(me, false))
            {
                if (!me->HasAura(SPELL_SAP) && who->HasAura(SPELL_EYE_OF_ZAZZO_KARAZHAN))
                {
                    if (me->GetExactDist2d(who) < 7.0f)
                    {
                        who->RemoveAurasByType(SPELL_AURA_MOD_STEALTH);
                        who->RemoveAura(SPELL_INFILTRATING_KARAZHAN);
                        who->RemoveAura(SPELL_EYE_OF_ZAZZO_KARAZHAN);
                        me->canAttack(who, true);
                        me->GetMotionMaster()->MoveChase(who);
                        me->Attack(who, true);
                    }
                }
            }
        }

        void DropCombatEngagement(uint64 targetGUID)
        {
            std::list<Unit*> nearUnits;
            Trinity::AnyUnitInObjectRangeCheck u_check(me, 30.0f);
            Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(me, nearUnits, u_check);
            me->VisitNearbyObject(30.0f, searcher);

            for (Unit* un : nearUnits)
            {
                if (un->GetTypeId() != TYPEID_UNIT)
                    continue;

                if (un->GetVictim() && un->GetVictim()->GetGUID() == targetGUID)
                {
                    un->CombatStop(true);
                    un->getThreatManager().clearReferences();
                    un->GetMotionMaster()->MoveTargetedHome();
                }
            }
        }

        void EnterCombat(Unit * /*who*/) override
        {
            Unit * target = me->GetVictim();
            if (!target)
                return;

            me->MonsterSay(guardKarazhanShouts[urand(0, KARAZHAN_SHOUT_COUNT - 1)], LANG_UNIVERSAL, 0);

            target->Unmount();
            target->RemoveAurasByType(SPELL_AURA_MOUNTED);
            target->RemoveAurasByType(SPELL_AURA_FLY);
            target->RemoveAurasByType(SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED);
            target->RemoveAurasByType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED);

            uint64 targetGUID = target->GetGUID();

            scheduler.Schedule(Seconds(6), [this, targetGUID](TaskContext /*context*/)
            {
                Unit* target = sObjectAccessor->FindUnit(targetGUID);

                DropCombatEngagement(targetGUID);

                me->CombatStop(true);
                if (target)
                    target->CombatStop(true);
                me->getThreatManager().clearReferences();
                me->GetMotionMaster()->MoveTargetedHome();

                if (target && me->GetDistance2d(target) < 10.0f)
                {
                    target->NearTeleportTo(ZazzoPosKarazhan.GetPositionX(), ZazzoPosKarazhan.GetPositionY(), ZazzoPosKarazhan.GetPositionZ(), ZazzoPosKarazhan.GetOrientation());
                    target->ToPlayer()->RemoveSpellCooldown(SPELL_STEALTH);
                    target->CastSpell(target, SPELL_STEALTH, true);
                    target->CastSpell(target, SPELL_EYE_OF_ZAZZO, true);
                    target->CastSpell(target, SPELL_INFILTRATING_GILNEAS_CITY, true);

                    if (Creature * pZazzo = me->FindNearestCreature(57770, 50.0f, true))
                        pZazzo->MonsterSay("You call that sneaking?", LANG_UNIVERSAL, 0);
                }
            });
        }

        void UpdateAI(const uint32 diff)
        {
            scheduler.Update(diff);

            if (!me->isMoving())
            {
                if (!me->IsInCombat() && me->CanFreeMove() && m_startMovementTimer == 0)
                    m_startMovementTimer = 5000;
                else if (m_startMovementTimer != 0)
                {
                    if (m_startMovementTimer <= diff)
                    {
                        if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == WAYPOINT_MOTION_TYPE)
                            ((WaypointMovementGenerator<Creature>*)me->GetMotionMaster()->top())->StartMoveNow(me);
                        m_startMovementTimer = 0;
                    }
                    else
                        m_startMovementTimer -= diff;
                }
            }

            if (!me->GetVictim())
                return;

            if (me->GetVictim()->GetDistance(me) > 30.0f)
            {
                me->CombatStop(true);
                me->getThreatManager().clearReferences();
                me->GetMotionMaster()->MoveTargetedHome();
                return;
            }

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_questline_fangs_of_the_father()
{
    new npc_thaumaturge_altha_and_rafir;
    new npc_corastrasza_rogue_quest;
    new npc_ravenholdt_guards;
    new npc_gilneas_guards;
    new npc_lord_hiram_creed();
    new npc_lord_hiram_creed_shadow_breath();
    new npc_nalice();
    new go_nalice_arcane_pillar();
    new npc_nalice_ground_explosion();
    new npc_nalice_blazing_shadows();
    new npc_nalice_quest_checker();
    new npc_wrathion_quest_checker();
    new npc_karazhan_guards();
}
