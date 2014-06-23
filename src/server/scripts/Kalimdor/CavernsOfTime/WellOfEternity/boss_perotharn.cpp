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


class boss_perotharn : public CreatureScript
{
public:
    boss_perotharn() : CreatureScript("boss_perotharn") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_perotharnAI(creature);
    }

    struct boss_perotharnAI : public ScriptedAI
    {
        boss_perotharnAI(Creature* creature) : ScriptedAI(creature), Summons(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        SummonList Summons;
        // Timers

        void JustSummoned(Creature* pSummoned)
        {
            Summons.Summon(pSummoned);
        }

        void Reset()
        {
            Summons.DespawnAll();
        }

        void PlayAndYell(uint32 soundId, const char * text)
        {
            DoPlaySoundToSet(me, soundId);
            me->MonsterYell(text, LANG_UNIVERSAL, 0);
        }

        void RemoveBeamAurasFromPlayers()
        {
            Map::PlayerList const& plList = me->GetMap()->GetPlayers();

            for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
            {
                if (Player * p = itr->getSource())
                {
                }
            }
        }

        void ApplyVortexAuraOnPlayers()
        {
            Map::PlayerList const& plList = me->GetMap()->GetPlayers();

            for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
            {
                if (Player * p = itr->getSource())
                {
                }
            }
        }

        void DoAction(const int32 action)
        {
        }

        void EnterCombat(Unit * /*who*/)
        {
        }

        void EnterEvadeMode()
        {
            ScriptedAI::EnterEvadeMode();
        }

        void KilledUnit(Unit* victim)
        {
            if (victim && victim->GetTypeId() == TYPEID_PLAYER)
            {
            }
        }

        void JustDied()
        {
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_perotharn()
{
    new boss_perotharn();
}