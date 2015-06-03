/*
 * Copyright (C) 2008-2010 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

class npc_attitudes_troll : CreatureScript
{
    public:
        npc_attitudes_troll() : CreatureScript("npc_attitudes_troll") { }

    bool OnGossipHello(Player* pShocker, Creature* pShocked)
    {
        if (pShocker->GetQuestStatus(14069) == QUEST_STATUS_INCOMPLETE)
            pShocker->CastSpell(pShocked, 66306, false);
        else {
            pShocker->GetSession()->SendNotification("The Goblin Al-In-1-Der Belt\'s battery is depleted.");
        }
        pShocker->SEND_GOSSIP_MENU(1, pShocked->GetGUID());
        pShocker->CLOSE_GOSSIP_MENU();
        return true;
    }

    struct npc_attitudes_trollAI : public ScriptedAI
    {
        npc_attitudes_trollAI(Creature* c) : ScriptedAI(c)
        {
            c->CastSpell(me, 45111, true); // visual "Enrage"
            c->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, 10.0f);
            c->SetFloatValue(UNIT_FIELD_COMBATREACH, 10.0f);
        }

        bool RunAwayStart;

        void Reset()
        {
            if (roll_chance_i(50))
                me->CastSpell(me, 62248, true);

            RunAwayStart = false;
        }

        void SpellHit(Unit* pTarget, const SpellEntry* spell)
        {
            if (!spell)
                return;

            if (spell->Id == 66306)
            {
                me->RemoveAurasDueToSpell(45111);
                me->RemoveAurasDueToSpell(62248);
                me->SetUInt32Value(UNIT_NPC_EMOTESTATE, 0);

                switch (urand(0,7))
                {
                    case 0:
                        me->MonsterSay("I\'m going. I\'m going!", LANG_UNIVERSAL, 0);
                        break;
                    case 1:
                        me->MonsterSay("Ouch! Dat hurt!", LANG_UNIVERSAL, 0);
                        break;
                    case 2:
                        me->MonsterSay("Work was bettah in da Undermine!", LANG_UNIVERSAL, 0);
                        break;
                    case 3:
                        me->MonsterSay("Don\'t tase me, mon!", LANG_UNIVERSAL, 0);
                        break;
                    case 4:
                        me->MonsterSay("What I doin\' wrong? Don't I get a lunch and two breaks a day, mon?", LANG_UNIVERSAL, 0);
                        break;
                    case 5:
                        me->MonsterSay("I report you to HR!", LANG_UNIVERSAL, 0);
                        break;
                    case 6:
                        me->MonsterSay("Oops, break\'s over.", LANG_UNIVERSAL, 0);
                        break;
                    case 7:
                        me->MonsterSay("Sorry, mon. It won\'t happen again.", LANG_UNIVERSAL, 0);
                        break;
                }
                me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                RunAwayStart = true;
            }
        }

        void UpdateAI(const uint32 /*diff*/)
        {
            if (RunAwayStart)
            {
                me->GetMotionMaster()->MoveRandom(30);
                me->SetSpeed(MOVE_RUN, 5.0f);
                me->ForcedDespawn(5000);
                RunAwayStart = false;
            }
        }
    };
    CreatureAI* GetAI(Creature* c) const
    {
        return new npc_attitudes_trollAI(c);
    }
};

void AddSC_kezan()
{
    new npc_attitudes_troll();
}
