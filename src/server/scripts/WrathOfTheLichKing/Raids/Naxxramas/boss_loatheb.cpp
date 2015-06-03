/*
 * Copyright (C) 2008-2010 TrinityCore <http://www.trinitycore.org/>
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
#include "naxxramas.h"

enum Spells
{
    SPELL_NECROTIC_AURA                                    = 55593,
    SPELL_SUMMON_SPORE                                     = 29234,
    SPELL_DEATHBLOOM                                       = 29865,
    H_SPELL_DEATHBLOOM                                     = 55053,
    SPELL_INEVITABLE_DOOM                                  = 29204,
    H_SPELL_INEVITABLE_DOOM                                = 55052,

	SPELL_DRAINMANA = 46453,
	//visuals
	//34168
	//35394 - zluta
	//69279 - spore dotka - 10 lidi pod 5%
	SPELL_VISUAL_1 = 34168,
	SPELL_VISUAL_2 = 35394,
	SPELL_SPORE    = 69279,
};

enum Events
{
    EVENT_NONE,
    EVENT_AURA,
    EVENT_BLOOM,
    EVENT_DOOM,
	EVENT_DRAIN,
	EVENT_SPORES,
};

class boss_loatheb : public CreatureScript
{
public:
    boss_loatheb() : CreatureScript("boss_loatheb") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_loathebAI (pCreature);
    }

    struct boss_loathebAI : public BossAI
    {
		boss_loathebAI(Creature *c) : BossAI(c, BOSS_LOATHEB) { pInstance = me->GetInstanceScript(); }

		bool SubCast;
		uint32 AuraCnt;
		InstanceScript* pInstance;

        void EnterCombat(Unit * /*who*/)
        {
            _EnterCombat();
            events.ScheduleEvent(EVENT_AURA, 10000);
            events.ScheduleEvent(EVENT_BLOOM, 5000);
            events.ScheduleEvent(EVENT_DOOM, 120000);
			events.ScheduleEvent(EVENT_DRAIN, 13000);
			SubCast = false;
			AuraCnt = 0;
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

			if(me->GetHealthPct() < 5.1f && !SubCast)
			{
				SubCast = true;
				events.ScheduleEvent(EVENT_SPORES,1000);
			}

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case EVENT_AURA:
						switch(AuraCnt)
						{
						case 0:
							me->MonsterTextEmote("An aura of necrotic energy blocks all healing!",0,true);
							DoCastAOE(SPELL_NECROTIC_AURA);
							events.ScheduleEvent(EVENT_AURA, 14000);
							break;
						case 1:
							me->MonsterTextEmote("The aura's power begins to wane!",0,true);
							events.ScheduleEvent(EVENT_AURA, 3000);
							break;
						case 2:
							me->MonsterTextEmote("The aura fades away, allowing healing once more!",0,true);
							events.ScheduleEvent(EVENT_AURA, 2800);
							break;
						}
						(AuraCnt < 2)?(AuraCnt += 1):(AuraCnt = 0);
                        break;
                    case EVENT_BLOOM:
                        // TODO : Add missing text
                        DoCastAOE(SPELL_SUMMON_SPORE, true);
                        DoCastAOE(RAID_MODE(SPELL_DEATHBLOOM,H_SPELL_DEATHBLOOM));
                        events.ScheduleEvent(EVENT_BLOOM, 30000);
                        break;
                    case EVENT_DOOM:
                        DoCastAOE(RAID_MODE(SPELL_INEVITABLE_DOOM,H_SPELL_INEVITABLE_DOOM));
                        events.ScheduleEvent(EVENT_DOOM, events.GetTimer() < 5*60000 ? 30000 : 15000);
                        break;
					case EVENT_DRAIN:
						DoCastAOE(SPELL_DRAINMANA, true);
						events.ScheduleEvent(EVENT_DRAIN, RAID_MODE(urand(15000,19000),urand(25000,30000)));
						break;
					case EVENT_SPORES:
						Map::PlayerList const &plList = pInstance->instance->GetPlayers();
						for(Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
						{
							if(itr->getSource())
							{
								itr->getSource()->CastSpell(itr->getSource(),SPELL_SPORE,true);
								if(urand(0,6) > 3)
									itr->getSource()->CastSpell(itr->getSource(),SPELL_VISUAL_1,true);
								else
									itr->getSource()->CastSpell(itr->getSource(),SPELL_VISUAL_2,true);
							}
						}
						events.ScheduleEvent(EVENT_SPORES,15000);
						break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };

};


enum SporeSpells
{
    SPELL_FUNGAL_CREEP                                     = 29232
};

class mob_loatheb_spore : public CreatureScript
{
public:
    mob_loatheb_spore() : CreatureScript("mob_loatheb_spore") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_loatheb_sporeAI (pCreature);
    }

    struct mob_loatheb_sporeAI : public ScriptedAI
    {
        mob_loatheb_sporeAI(Creature *c) : ScriptedAI(c) {
			for(int i = 0; i <= 4; i++)
			{
				Closest[i].distance = 200.0f;
				Closest[i].playerGUID = 0;
			}
		}

		struct closest {
		public:
			uint64 playerGUID;
			float distance;
		} Closest[5];

		void DamageTaken(Unit* pDoneBy, uint32 &uiDamage)
		{
			if(uiDamage > me->GetHealth())
				me->CastSpell(me, SPELL_FUNGAL_CREEP, true);
		}

        void JustDied(Unit* killer)
        {
			Map* pMap = me->GetMap();
		    if(!pMap)
				return;

			Map::PlayerList const &lPlayers = pMap->GetPlayers();
			for(Map::PlayerList::const_iterator itr = lPlayers.begin(); itr != lPlayers.end(); ++itr)
			{
				if(Player* pPlayer = itr->getSource())
				{
					uint64 playerGUID = pPlayer->GetGUID();
					for(int i = 0; i <= 4; i++)
					{
						if(playerGUID == Closest[i].playerGUID)
							break;

						if(i == 4)
							pPlayer->RemoveAurasDueToSpell(SPELL_FUNGAL_CREEP, me->GetGUID());
					}
				}
			}
		}

		void SpellHitTarget(Unit* pPlayer, const SpellEntry* spellEntry)
		{
			if(pPlayer && pPlayer->GetTypeId() == TYPEID_PLAYER)
			{
				float dist = me->GetDistance(pPlayer);
				if(dist < 10.0f)
				{
					for(int i = 0; i <= 4; i++)
					{
						if(dist < Closest[i].distance)
						{
							for(int j = 3; j >= i; j--)
								Closest[j+1] = Closest[j];

							Closest[i].distance = dist;
							Closest[i].playerGUID = pPlayer->GetGUID();
							return;
						}
					}
				}
			}
		}
    };

};


void AddSC_boss_loatheb()
{
    new boss_loatheb();
    new mob_loatheb_spore();
}
