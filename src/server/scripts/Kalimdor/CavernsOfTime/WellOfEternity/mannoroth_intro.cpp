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

typedef struct quote_event
{
    const uint32 nextEventTime;
    const uint32 entry;
    const char * yellQuote;
    const uint32 soundID;
}QUOTE_EVENTS;


enum spells
{

};

#define MAX_PRELUDE_QUOTES 7

QUOTE_EVENTS preludeQuotes [MAX_PRELUDE_QUOTES] =
{
    {5000, ENTRY_ILLIDAN_PRELUDE, "Can you close the portal brother?", 0},
    {5000, ENTRY_MALFURION_PRELUDE, "It is being maintain by the will of a power demon...Mannoroth.", 0},
    {5000, ENTRY_MALFURION_PRELUDE, "I cannot break his will alone.", 0},
    {5000, ENTRY_ILLIDAN_PRELUDE, "Very well, we shall break it for you.", 0},
    {5000, ENTRY_TYRANDE_PRELUDE, "He knows what we attempted. We have not much time. The forest crawls with his demons.", 0},
    {5000, ENTRY_ILLIDAN_PRELUDE, "Let them come.", 0},
    {MAX_TIMER, ENTRY_TYRANDE_PRELUDE, "Mother moon, guide us through this darkness.", 0}
};

#define MAX_SEQUEL_QUOTES 5

QUOTE_EVENTS sequelQuotes [MAX_SEQUEL_QUOTES] =
{
    {3000, ENTRY_ILLIDAN_PRELUDE, "Oh, this will be fun...", 0},
    {2000, ENTRY_ILLIDAN_PRELUDE, "Wait, I have an idea.", 0},
    {6000, ENTRY_TYRANDE_PRELUDE, "Illidan, what is in that vial? What are you doing?", 0},
    {9000, ENTRY_ILLIDAN_PRELUDE, "What our people could not.", 0},
    {MAX_TIMER,  ENTRY_ILLIDAN_PRELUDE, "Yes...YES. I can feel the raw power of the Well of Eternity! It is time to end this charade!", 0},
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

class boss_mannoroth_woe : public CreatureScript
{
public:
    boss_mannoroth_woe() : CreatureScript("boss_mannoroth_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_mannoroth_woeAI(creature);
    }

    struct boss_mannoroth_woeAI : public ScriptedAI
    {
        boss_mannoroth_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = me->GetInstanceScript();
        }

        InstanceScript * pInstance;

        void Reset() override
        {
        }

        void UpdateAI(const uint32 diff) override
        {
        }
    };
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
            pInstance = me->GetInstanceScript();
            preludeStep = 0;
            preQuotesTimer = MAX_TIMER;
        }

        InstanceScript * pInstance; // needed ?
        uint32 preQuotesTimer;
        uint32 preludeStep;

        void Reset() override
        {

        }

        void DoAction(const int32 action) override
        {
            if (action == ACTION_ILIDAN_PRELUDE_EVENT_START)
                preQuotesTimer = 10000;
        }

        void ReceiveEmote(Player* pPlayer, uint32 emote) // TESTING PURPOSE EVENT
        {
            if (pPlayer->IsGameMaster())
                preQuotesTimer = 5000;
        }

        void UpdateAI(const uint32 diff) override
        {
            if (preQuotesTimer <= diff)
            {
                QUOTE_EVENTS qEvent = preludeQuotes[preludeStep];

                if (me->GetEntry() == qEvent.entry) // Dont look for NPC if we are the npc :)
                {
                    me->MonsterYell(qEvent.yellQuote, 0, 0,me->GetMap()->GetVisibilityDistance());
                    me->PlayDirectSound(qEvent.soundID);
                }
                else if (Creature * pCreature = me->FindNearestCreature(qEvent.entry,me->GetMap()->GetVisibilityDistance(),true))
                {
                    pCreature->MonsterYell(qEvent.yellQuote, 0, 0,me->GetMap()->GetVisibilityDistance());
                    pCreature->PlayDirectSound(qEvent.soundID);
                }
                // Set timer and move to next Quote
                preQuotesTimer = qEvent.nextEventTime;
                preludeStep++;
            }
            else preQuotesTimer -= diff;
        }
    };
};

class npc_tyrande_mannoroth_woe : public CreatureScript
{
public:
    npc_tyrande_mannoroth_woe() : CreatureScript("npc_tyrande_mannoroth_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_tyrande_mannoroth_woeAI(creature);
    }

    struct npc_tyrande_mannoroth_woeAI : public ScriptedAI
    {
        npc_tyrande_mannoroth_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = me->GetInstanceScript();
        }

        InstanceScript * pInstance;

        void Reset() override
        {
        }

        void UpdateAI(const uint32 diff) override
        {
        }
    };
};

void AddSC_mannoroth_intro()
{
    new npc_illidan_mannoroth_woe();
    new npc_tyrande_mannoroth_woe();
}