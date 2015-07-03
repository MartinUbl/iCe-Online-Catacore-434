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
#include "well_of_eternity.h"

#define CAST_WOE_INSTANCE(i)     (dynamic_cast<instance_well_of_eternity::instance_well_of_eternity_InstanceMapScript*>(i))

enum spells
{
    SPELL_NATURE_CHANNELING = 28892, // dont know if correct but it has same effect
    SPELL_ABSYSSAL_FLAMETHROWER = 105218, // visual
    SPELL_TIME_SHIFT = 102602,
    SPELL_ABYSSAL_FLAMES_AOE = 103992,

    SPELL_WATERS_OF_ETERNITY = 103952,

    SPELL_SHADOWBAT_COSMETIC = 103714,
    SPELL_SHADOWBAT_DISPLACEMENT = 103763,

    SPELL_BLESSING_OF_ELUNE = 103917, // fixed aoe aura application to players
    SPELL_BLESSING_OF_ELUNE_AURA = 103918, // triggering 103919 -> 100k damage to single unit
};

enum illidanOffensiveSpells
{
    SPELL_DARKLANCE = 104394,
    SPELL_AURA_OF_IMMOLATION = 104379,
    SPELL_DEMON_RUSH = 104205,
    SPELL_DEMONIC_SIGHT_DODGE = 104746,
    SPELL_TAUNT_10_SECONDS = 104461,
};

#define MAX_PRELUDE_QUOTES 7

QUOTE_EVENTS preludeQuotes [MAX_PRELUDE_QUOTES] =
{
    {4000, ENTRY_ILLIDAN_PRELUDE, "Can you close the portal, brother?", 26084},
    {6000, ENTRY_MALFURION_PRELUDE, "It is being maintained by the will of a powerful demon, Mannoroth.", 26494},
    {4000, ENTRY_MALFURION_PRELUDE, "I cannot break his will alone.", 26495},
    {5000, ENTRY_ILLIDAN_PRELUDE, "Very well, we shall break it for you.", 26086},
    {6000, ENTRY_TYRANDE_PRELUDE, "He knows what we attempt. We have not much time. The forest crawls with his demons.", 25993},
    {3000, ENTRY_ILLIDAN_PRELUDE, "Let them come.", 26085},
    {MAX_TIMER, ENTRY_TYRANDE_PRELUDE, "Mother moon, guide us through this darkness.", 25994}
};
 
#define MAX_SEQUEL_QUOTES 5
 
QUOTE_EVENTS sequelQuotes [MAX_SEQUEL_QUOTES] =
{
    {4000, ENTRY_ILLIDAN_PRELUDE, "Oh, this will be fun...", 26089},
    {4000, ENTRY_ILLIDAN_PRELUDE, "Wait, I have an idea.", 26090 },
    {5000, ENTRY_TYRANDE_PRELUDE, "Illidan, what is in that vial? What are you doing?", 25995},
    {4000, ENTRY_ILLIDAN_PRELUDE, "What our people could not.", 26091 },
    {MAX_TIMER,  ENTRY_ILLIDAN_PRELUDE, "Yes...YES. I can feel the raw power of the Well of Eternity! It is time to end this charade!", 26092},
};
 
SimpleQuote afterDoomguardsDeath = {26088, "Weak, pitiful creatures. Hardly worth of being called a legion."};
 
#define MAX_PRECOMBAT_QUOTES 4
 
QUOTE_EVENTS preCombatQuotes [MAX_PRECOMBAT_QUOTES] =
{
    {5500, MANNOROTH_ENTRY, "Varo'then, see that I am not disrupted by this rabble!", 26480}, // yell
    {4000, VAROTHEN_ENTRY, "Highguard! To arms! For your queen! For Azshara!", 26137}, // yell
    {4000, ENTRY_TYRANDE_PRELUDE, "I cannot strike them! What is this demon magic?", 25996},
    {MAX_TIMER, ENTRY_ILLIDAN_PRELUDE, "They are not what they appear to be! Strike in an area, it is the only way to uncover the real one!", 26093}
};
#define CAST_WOE_INSTANCE(i)     (dynamic_cast<instance_well_of_eternity::instance_well_of_eternity_InstanceMapScript*>(i))

static void PlayQuote (Creature * source, SimpleQuote quote, bool yell = false)
{
    source->PlayDirectSound(quote.soundID);

    if (yell)
        source->MonsterYell(quote.text, LANG_UNIVERSAL,0,200.0f);
    else
        source->MonsterSay(quote.text, LANG_UNIVERSAL,0,200.0f);
}

enum pathWaypointsIds
{
    PATH_POINT1_WP = 6789,
    PATH_POINT2_WP,
    PATH_POINT3_WP,
    PATH_POINT_INVALID
};

class npc_illidan_mannoroth_woe : public CreatureScript
{
public:
    npc_illidan_mannoroth_woe() : CreatureScript("npc_illidan_mannoroth_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_illidan_mannoroth_woeAI(creature);
    }

    struct npc_illidan_mannoroth_woeAI : public ScriptedAI
    {
        npc_illidan_mannoroth_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            highGuardsKilled = 0;
            preludeStep = sequelStep = preCombatStep = 0;
            preQuotesTimer = sequelQuoteTimer = preCombatTimer = moveTimer = encounterStartTimer = MAX_TIMER;
            canAttackMannoroth = false;
            pathCompleted = true;

            darkLanceTimer = MAX_TIMER;
            tauntTimer = MAX_TIMER;
        }

        pathWaypointsIds currentWP = PATH_POINT_INVALID;
        bool pathCompleted;

        uint32 preQuotesTimer;
        uint32 sequelQuoteTimer;
        uint32 preCombatTimer;
        uint32 moveTimer;
        uint32 encounterStartTimer;

        uint32 preludeStep;
        uint32 sequelStep;
        uint32 preCombatStep;

        uint32 highGuardsKilled;

        bool canAttackMannoroth;

        // Combat timers
        uint32 darkLanceTimer;
        uint32 tauntTimer;

        void Reset() override
        {

        }

        void AttackStart(Unit* victim) override
        {
            if (victim->GetTypeId() == TYPEID_UNIT && victim->GetEntry() == MANNOROTH_ENTRY)
                ScriptedAI::AttackStart(victim);
        }

        void EnterCombat(Unit * victim) override
        {
            if (victim->GetTypeId() == TYPEID_UNIT && victim->GetEntry() == MANNOROTH_ENTRY)
                ScriptedAI::EnterCombat(victim);
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            pathCompleted = false;

            currentWP = (pathWaypointsIds)id;

            if (id == PATH_POINT3_WP)
            {
                currentWP = PATH_POINT_INVALID;
                sequelStep = 0; // just for sure
                sequelQuoteTimer = 5000;
            }
            else if (id == 2) // Arrived before varothen pos
            {
                encounterStartTimer = 4000;
                me->CastSpell(me, SPELL_AURA_OF_IMMOLATION, false);
            }
            else if (id == 2) // Arrived before varothen pos
            {
                encounterStartTimer = 4000;
                me->CastSpell(me, SPELL_AURA_OF_IMMOLATION, false);
            }
        }

        void DoAction(const int32 action) override
        {
            if (action == ACTION_ILLIDAN_PRELUDE_EVENT_START)
                preQuotesTimer = 10000;
            else if (action == ACTION_ILLIDAN_PRELUDE_EVENT_CONTINUE)
            {
                PlayQuote(me, afterDoomguardsDeath);
                moveTimer = 4000;
            }
            else if (action == ACTION_ILLIDAN_PRECOMBAT_QUOTES_START)
                preCombatTimer = 2000;
            else if (action == ACTION_ILLIDAN_HIGHGUARD_DIED)
            {
                if (++highGuardsKilled == 2)
                {
                    me->GetMotionMaster()->MovePoint(2, illidanVarothenPos, true, true);
                    if (Creature * pTyrande = me->FindNearestCreature(ENTRY_TYRANDE_PRELUDE, 250.0f, true))
                        pTyrande->AI()->DoAction(ACTION_TYRANDE_MOVE_TO_VAROTHEN);
                }
            }
            else if (action == ACTION_MANNOROTH_ENCOUNTER_FAILED)
            {
                me->DeleteThreatList();
                me->RemoveAllAuras();
                me->GetMotionMaster()->MoveTargetedHome();
                ScriptedAI::EnterEvadeMode();
            }
            else if (action == ACTION_MANNOROTH_FIGHT_ENDED)
            {
                me->DeleteThreatList();
                me->RemoveAllAuras();
                me->GetMotionMaster()->Clear(false);
                me->GetMotionMaster()->MovementExpired(false);

                canAttackMannoroth = false;
            }
        }

        void EnterEvadeMode() override
        {
            me->DeleteThreatList();
            me->RemoveAllAuras();
            //me->GetMotionMaster()->MoveTargetedHome();
            ScriptedAI::EnterEvadeMode();
        }

        void ReceiveEmote(Player* pPlayer, uint32 emote) // TESTING PURPOSE ONLY
        {
            if (!pPlayer->IsGameMaster())
                return;
            if (emote == TEXTEMOTE_DANCE)
            {
                me->AI()->DoAction(ACTION_ILLIDAN_PRELUDE_EVENT_CONTINUE);
                return;
            }
            preQuotesTimer = 5000;
        }

        void UpdateAI(const uint32 diff) override
        {
            if (pathCompleted == false && currentWP != PATH_POINT_INVALID)
            {
                pathCompleted = true;
                if (currentWP == PATH_POINT1_WP)
                {
                    me->GetMotionMaster()->MovePoint(currentWP + 1, pathMiddlePos, false, false);
                    if (Creature * pTyrande = me->FindNearestCreature(ENTRY_TYRANDE_PRELUDE, 100.0f, true))
                        pTyrande->GetMotionMaster()->MovePoint(currentWP + 1, pathMiddlePos, false, false);
                }
                else if (currentWP == PATH_POINT2_WP)
                {
                    me->GetMotionMaster()->MovePoint(currentWP + 1, illidanAbyssalPos, false, false);
                    if (Creature * pTyrande = me->FindNearestCreature(ENTRY_TYRANDE_PRELUDE, 100.0f, true))
                        pTyrande->GetMotionMaster()->MovePoint(currentWP + 1, tyrandeAbyssalPos, false,false);
                }
            }

            if (preQuotesTimer <= diff)
            {
                if (preludeStep < MAX_PRELUDE_QUOTES) // Should not happen
                {
                    QUOTE_EVENTS qEvent = preludeQuotes[preludeStep];

                    if (Creature * pCreature = me->FindNearestCreature(qEvent.entry,me->GetMap()->GetVisibilityDistance(),true))
                    {
                        if (preludeStep == MAX_PRELUDE_QUOTES - 1)
                            pCreature->CastSpell(pCreature, SPELL_BLESSING_OF_ELUNE, false); // Tyrande

                        pCreature->MonsterSay(qEvent.yellQuote, 0, 0,me->GetMap()->GetVisibilityDistance());
                        pCreature->PlayDirectSound(qEvent.soundID);
                    }
                    // Set timer and move to next Quote
                    preQuotesTimer = qEvent.nextEventTime;
                    preludeStep++;
                }
            }
            else preQuotesTimer -= diff;

            if (sequelQuoteTimer <= diff)
            {
                if (sequelStep < MAX_SEQUEL_QUOTES) // Should not happen
                {
                    QUOTE_EVENTS qEvent = sequelQuotes[sequelStep];

                    if (Creature * pCreature = me->FindNearestCreature(qEvent.entry,me->GetMap()->GetVisibilityDistance(),true))
                    {
                        if (sequelStep == 1)
                            me->CastSpell(me, 87071 , false); // Illidan alchemy fake
                        else if (sequelStep == 3)
                            me->CastSpell(me, SPELL_WATERS_OF_ETERNITY , false); // Illidan drink waters of eternity

                        pCreature->MonsterSay(qEvent.yellQuote, 0, 0,me->GetMap()->GetVisibilityDistance());
                        pCreature->PlayDirectSound(qEvent.soundID);
                    }
                    // Set timer and move to next Quote
                    sequelQuoteTimer = qEvent.nextEventTime;
                    sequelStep++;
                }
            }
            else sequelQuoteTimer -= diff;

            if (moveTimer <= diff)
            {
                me->GetMotionMaster()->MovePoint(PATH_POINT1_WP, pathStartPos, false);
                if (Creature * pTyrande = me->FindNearestCreature(ENTRY_TYRANDE_PRELUDE,100.0f,true))
                    pTyrande->GetMotionMaster()->MovePoint(PATH_POINT1_WP, pathStartPos, false);

                moveTimer = MAX_TIMER;
            }
            else moveTimer -= diff;

            if (preCombatTimer <= diff)
            {
                if (preCombatStep < MAX_PRECOMBAT_QUOTES) // Should not happen
                {
                    QUOTE_EVENTS qEvent = preCombatQuotes[preCombatStep];

                    if (Creature * pCreature = me->FindNearestCreature(qEvent.entry,me->GetMap()->GetVisibilityDistance(),true))
                    {
                        if (preCombatStep <= 1)
                            pCreature->MonsterYell(qEvent.yellQuote, 0, 0,me->GetMap()->GetVisibilityDistance());
                        else
                            pCreature->MonsterSay(qEvent.yellQuote, 0, 0,me->GetMap()->GetVisibilityDistance());
                        pCreature->PlayDirectSound(qEvent.soundID);
                    }

                    if (preCombatStep == 1)
                    {
                        std::list<Creature*> highguards;
                        me->GetCreatureListWithEntryInGrid(highguards, ENTRY_HIGHGUARD_ELITE, 250.0f);
                        for (std::list<Creature*>::iterator itr = highguards.begin(); itr != highguards.end(); ++itr)
                            (*itr)->SetInCombatWithZone();
                    }
                    // Set timer and move to next Quote
                    preCombatTimer = qEvent.nextEventTime;
                    preCombatStep++;
                }
            }
            else preCombatTimer -= diff;

            if (encounterStartTimer <= diff)
            {
                if (Creature * pMannoroth = me->FindNearestCreature(MANNOROTH_ENTRY, 250.0f, true))
                {
                    pMannoroth->SetReactState(REACT_AGGRESSIVE);
                    pMannoroth->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    pMannoroth->SetInCombatWithZone();
                    me->GetMotionMaster()->MoveChase(pMannoroth);
                    pMannoroth->AddThreat(me, 900000.0f); // is this safe and correct ? check factions ...
                    me->AddThreat(pMannoroth, 900000.0f); // dont attack varothen or somone else
                }
                if (Creature * pVarothen = me->FindNearestCreature(VAROTHEN_ENTRY, 250.0f, true))
                {
                    pVarothen->SetReactState(REACT_AGGRESSIVE);
                    pVarothen->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    pVarothen->SetInCombatWithZone();
                }
                me->CastSpell(me, SPELL_DEMONIC_SIGHT_DODGE, true);
                canAttackMannoroth = true; // Allow usage of combat abilities
                darkLanceTimer = 2000;
                tauntTimer = 6000;
                encounterStartTimer = MAX_TIMER;
            }
            else encounterStartTimer -= diff;

            // COMBAT ABILITIES TO MANNOROTH
            if (canAttackMannoroth)
            {
                if (darkLanceTimer <= diff && !me->IsNonMeleeSpellCasted(false))
                {
                    if (Creature * pMannoroth = me->FindNearestCreature(MANNOROTH_ENTRY, 250.0f, true))
                    {
                        if (me->IsWithinMeleeRange(pMannoroth))
                            me->CastSpell(pMannoroth, SPELL_DARKLANCE, false);
                        else
                            me->GetMotionMaster()->MoveChase(pMannoroth);
                    }
                    darkLanceTimer = urand(5000, 8000);
                }
                else darkLanceTimer -= diff;

                if (tauntTimer <= diff)
                {
                    if (Creature * pMannoroth = me->FindNearestCreature(MANNOROTH_ENTRY, 250.0f, true))
                        me->CastSpell(pMannoroth, SPELL_TAUNT_10_SECONDS, true);
                    tauntTimer = 6000;
                }
                else tauntTimer -= diff;

                DoMeleeAttackIfReady();
            }
        }
    };
};

class npc_malfurion_mannoroth_woe : public CreatureScript
{
public:
    npc_malfurion_mannoroth_woe() : CreatureScript("npc_malfurion_mannoroth_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_malfurion_mannoroth_woeAI(creature);
    }

    struct npc_malfurion_mannoroth_woeAI : public ScriptedAI
    {
        npc_malfurion_mannoroth_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            me->CastSpell(me, SPELL_NATURE_CHANNELING, false);
        }
    };
};

class npc_highguard_elite_woe : public CreatureScript
{
public:
    npc_highguard_elite_woe() : CreatureScript("npc_highguard_elite_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_highguard_elite_woeAI(creature);
    }

    struct npc_highguard_elite_woeAI : public ScriptedAI
    {
        npc_highguard_elite_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = me->GetInstanceScript();
            shadowbatGUID = 0;
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_DEFENSIVE);

            if (Creature * pShadowbat = me->SummonCreature(ENTRY_SHADOWBAT_HIGHGUARD, 0, 0, 0))
            {
                shadowbatGUID = pShadowbat->GetGUID();
                me->EnterVehicle(pShadowbat, 0);
                pShadowbat->ClearUnitState(UNIT_STATE_UNATTACKABLE);
                pShadowbat->CastSpell(pShadowbat, SPELL_SHADOWBAT_COSMETIC, true);
            }
        }

        uint64 shadowbatGUID;
        InstanceScript * pInstance;
        uint32 attackTimer;

        void Reset() override
        {
            attackTimer = 1000;
        }

        void EnterCombat(Unit * who) override
        {
            if (Creature * pShadowBat = ObjectAccessor::GetCreature(*me, shadowbatGUID))
                pShadowBat->CastSpell(pShadowBat, SPELL_SHADOWBAT_DISPLACEMENT, true);

            ScriptedAI::EnterCombat(who);
        }

        void EnterEvadeMode() override
        {
            // Not working .. but neh :)
            me->GetMotionMaster()->MoveTargetedHome();
            if (Creature * pShadowBat = ObjectAccessor::GetCreature(*me, shadowbatGUID))
                pShadowBat->GetMotionMaster()->MoveTargetedHome();
        }

        void JustDied(Unit*)
        {
            if (Creature * pIllidan = me->FindNearestCreature(ENTRY_ILLIDAN_PRELUDE, 250.0f, true))
                pIllidan->AI()->DoAction(ACTION_ILLIDAN_HIGHGUARD_DIED);
        }


        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (attackTimer <= diff)
            {
                me->GetMotionMaster()->MoveChase(me->GetVictim());

                if (Creature * pShadowBat = ObjectAccessor::GetCreature(*me, shadowbatGUID))
                {
                    if (pShadowBat->IsAlive())
                    {
                        pShadowBat->GetMotionMaster()->MoveChase(me->GetVictim());
                    }
                    else
                    {
                        if(me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    }
                }
                attackTimer = 500;
            }
            else attackTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_doomgurad_annihilator_woe : public CreatureScript
{
public:
    npc_doomgurad_annihilator_woe() : CreatureScript("npc_doomgurad_annihilator_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_doomgurad_annihilator_woeAI(creature);
    }

    struct npc_doomgurad_annihilator_woeAI : public ScriptedAI
    {
        npc_doomgurad_annihilator_woeAI(Creature* creature) : ScriptedAI(creature) { pInstance = me->GetInstanceScript(); }

        InstanceScript * pInstance;

        void Reset() override
        {
        }

        void JustDied(Unit *) override
        {
            // We killed last doomguard
            if (me->FindNearestCreature(ENTRY_DOOMGUARD_ANNIHILATOR,200.0f,true) == nullptr)
            {
                if (Creature * pTyrande = me->FindNearestCreature(ENTRY_TYRANDE_PRELUDE,250.0f,true))
                {
                    pTyrande->RemoveAllAuras();
                }

                if (Creature * pIllidan= me->FindNearestCreature(ENTRY_ILLIDAN_PRELUDE,250.0f,true))
                {
                    pIllidan->AI()->DoAction(ACTION_ILLIDAN_PRELUDE_EVENT_CONTINUE);
                }
            }
        }

        void EnterEvadeMode() override
        {
            if (!pInstance)
                return;

            CAST_WOE_INSTANCE(pInstance)->demonPulseDone = false;

            ScriptedAI::EnterEvadeMode();
        }

        void EnterCombat(Unit *who) override
        {
            if (!pInstance)
                return;

            if (CAST_WOE_INSTANCE(pInstance)->demonPulseDone)
                return;

            // Lock to prevent recursion
            // Because we enter to combat in CallDoomGuardsForHelp again ...
            CAST_WOE_INSTANCE(pInstance)->demonPulseDone = true;

            CAST_WOE_INSTANCE(pInstance)->CallDoomGuardsForHelp(who);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

#define SPELL_DEMON_PORTAL_PULL_AURA 105531

class npc_abyysal_elemental_woe : public CreatureScript
{
public:
    npc_abyysal_elemental_woe() : CreatureScript("npc_abyysal_elemental_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_abyysal_elemental_woeAI(creature);
    }

    struct npc_abyysal_elemental_woeAI : public ScriptedAI
    {
        npc_abyysal_elemental_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            pInst = me->GetInstanceScript();
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            pInst = me->GetInstanceScript();
            me->CastSpell(me, SPELL_ABSYSSAL_FLAMETHROWER, true);
            suckedToWell = false;
        }

        uint32 abyssalFlamesTimer;
        InstanceScript * pInst;
        bool suckedToWell;

        void Reset() override
        {
            if (me->HasAura(SPELL_DEMON_PORTAL_PULL_AURA))
                return;

            abyssalFlamesTimer = 2000;
            ScriptedAI::Reset();
        }

        void JustReachedHome() override
        {
            if (me->HasAura(SPELL_DEMON_PORTAL_PULL_AURA))
                return;

            me->CastSpell(me, SPELL_ABSYSSAL_FLAMETHROWER, true);
        }

        void AttackStart(Unit* victim) override
        {
            if (me->HasAura(SPELL_DEMON_PORTAL_PULL_AURA))
                return;

            if (victim->GetTypeId() == TYPEID_UNIT && victim->GetOwnerGUID() == 0)
                return;

            ScriptedAI::AttackStart(victim);
        }

        void EnterCombat(Unit * victim) override
        {
            if (me->HasAura(SPELL_DEMON_PORTAL_PULL_AURA))
                return;

            if (victim->GetTypeId() == TYPEID_UNIT && victim->GetOwnerGUID() == 0)
                return;

            me->SetInCombatWithZone();
            ScriptedAI::EnterCombat(victim);
        }

        void JustDied(Unit *) override
        {
            if (CAST_WOE_INSTANCE(pInst)->abyssalSlained == false)
            {
                //Abyssal killed -> move to next pos + start precombat quotes
                if (Creature * pIllidan = me->FindNearestCreature(ENTRY_ILLIDAN_PRELUDE, 100.0f, true))
                {
                    pIllidan->GetMotionMaster()->MovePoint(1, illidanHighGuardPos, true, true);
                    pIllidan->AI()->DoAction(ACTION_ILLIDAN_PRECOMBAT_QUOTES_START);
                }

                if (Creature * pTyrande = me->FindNearestCreature(ENTRY_TYRANDE_PRELUDE, 100.0f, true))
                {
                    pTyrande->GetMotionMaster()->MovePoint(1, tyrandeHighGuardPos, true, true);
                }

                CAST_WOE_INSTANCE(pInst)->abyssalSlained = true;
            }
            me->ForcedDespawn(2000);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (me->HasAura(SPELL_DEMON_PORTAL_PULL_AURA))
                return;

            if (!UpdateVictim())
                return;

            if (abyssalFlamesTimer <= diff)
            {
                me->CastSpell(me, SPELL_ABYSSAL_FLAMES_AOE, true);
                abyssalFlamesTimer = 3000;
            }
            else abyssalFlamesTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_mannoroth_intro()
{
    new npc_malfurion_mannoroth_woe();
    new npc_doomgurad_annihilator_woe();
    new npc_highguard_elite_woe();
    new npc_illidan_mannoroth_woe();
    new npc_abyysal_elemental_woe();
}