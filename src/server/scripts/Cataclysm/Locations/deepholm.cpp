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

/*
UPDATE creature_template SET ScriptName='mob_spirit_totem',AIName='' WHERE entry = 42728;
*/

class mob_spirit_totem: public CreatureScript
{
public:
    mob_spirit_totem(): CreatureScript("mob_spirit_totem") {}

    struct mob_spirit_totemAI: public ScriptedAI
    {
        enum Creatures
        {
            NPC_LYING_1       = 42681,
            NPC_LYING_2       = 42682,
            NPC_SPIRIT_EVIL   = 42747,
            NPC_SPIRIT_FRIEND = 42757
        };

        mob_spirit_totemAI(Creature* pCreature): ScriptedAI(pCreature)
        {
            Reset();
        }

        uint32 RessTimer;
        uint8 RessPhase;
        Creature* selected;

        void Reset()
        {
            RessTimer = 1000;
            RessPhase = 1;
            selected = NULL;
        }

        void UpdateAI(const uint32 diff)
        {
            if (RessPhase)
            {
                if (RessTimer <= diff)
                {
                    switch (RessPhase)
                    {
                        case 1:
                            {
                            selected = NULL;
                            std::list<Creature*> AList;
                            GetCreatureListWithEntryInGrid(AList,me,NPC_LYING_1,5.0f);
                            if (!AList.empty())
                            {
                                for (std::list<Creature*>::const_iterator itr = AList.begin(); itr != AList.end(); ++itr)
                                    if (!(*itr)->HasAura(23452))
                                    {
                                        selected = *itr;
                                        break;
                                    }
                            }
                            if (!selected)
                            {
                                AList.clear();
                                GetCreatureListWithEntryInGrid(AList,me,NPC_LYING_2,5.0f);
                                if (!AList.empty())
                                {
                                    for (std::list<Creature*>::const_iterator itr = AList.begin(); itr != AList.end(); ++itr)
                                        if (!(*itr)->HasAura(23452))
                                        {
                                            selected = *itr;
                                            break;
                                        }
                                }
                            }
                            ++RessPhase;
                            RessTimer = 300;
                            break;
                            }
                        case 2:
                            selected->CastSpell(selected, 23452, true);
                            if (roll_chance_i(65))
                            {
                                Creature* pSpirit = me->SummonCreature(NPC_SPIRIT_FRIEND,selected->GetPositionX(),selected->GetPositionY(),selected->GetPositionZ(),selected->GetOrientation(),TEMPSUMMON_TIMED_DESPAWN,100000);
                                if (pSpirit)
                                {
                                    switch (urand(0,2))
                                    {
                                    case 0:
                                        pSpirit->MonsterSay("I heard them praying to their dark gods as everything went black... The Twilight's Hammer did this!",LANG_UNIVERSAL,0);
                                        break;
                                    case 1:
                                        pSpirit->MonsterSay("Everyone started dropping like flies... everyone who ate the rations",LANG_UNIVERSAL,0);
                                        break;
                                    default:
                                        pSpirit->MonsterSay("They tricked us into ambushing the Horde gunship... but why?",LANG_UNIVERSAL,0);
                                        break;
                                    }
                                }
                                Player* pOwner = me->GetCharmerOrOwnerPlayerOrPlayerItself();
                                if (pOwner)
                                    pOwner->KilledMonsterCredit(42758,0);
                            }
                            else
                            {
                                Creature* pSpirit = me->SummonCreature(NPC_SPIRIT_EVIL,selected->GetPositionX(),selected->GetPositionY(),selected->GetPositionZ(),selected->GetOrientation(),TEMPSUMMON_TIMED_DESPAWN,100000);
                                Player* pOwner = me->GetCharmerOrOwnerPlayerOrPlayerItself();
                                if (pSpirit && pOwner)
                                {
                                    pSpirit->MonsterSay("Twilight Scum! You did this to us!",LANG_UNIVERSAL,0);
                                    pSpirit->AI()->AttackStart(pOwner);
                                }
                            }
                            RessPhase = 0;
                            RessTimer = 0;
                            break;
                    }
                } else RessTimer -= diff;
            }
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new mob_spirit_totemAI(c);
    }
};

void AddSC_deepholm()
{
    new mob_spirit_totem();
}
