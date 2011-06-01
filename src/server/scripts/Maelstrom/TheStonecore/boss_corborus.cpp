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
#include "the_stonecore.h"

enum UsedThings
{
    SPELL_CRYSTAL_BARRAGE  = 86881,
    NPC_CRYSTAL_SHARD      = 49267,
    SPELL_DAMPENING_WAVE   = 82415,
    SPELL_DAMPENING_WAVE_H = 92650,
    NPC_ROCK_BORER         = 43917,
    SPELL_ROCK_BORE        = 80028,
    SPELL_ROCK_BORE_H      = 92630,
};

class boss_corborus: public CreatureScript
{
public:
    boss_corborus(): CreatureScript("boss_corborus") {};

    struct boss_corborusAI: public ScriptedAI
    {
        boss_corborusAI(Creature* c): ScriptedAI(c)
        {
            pInstance = me->GetInstanceScript();
            Reset();
        }

        InstanceScript* pInstance;

        void Reset()
        {
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new boss_corborusAI(c);
    }
};

void AddSC_corborus()
{
    new boss_corborus();
}
