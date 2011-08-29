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
#include "throne_of_the_four_winds.h"

enum Stuff
{
    // phase 1
    SPELL_LIGHTNING_STRIKE_DAMAGE = 88214,
    SPELL_LIGHTNING_STRIKE_VISUAL = 88230,
    SPELL_STATIC_SHOCK            = 87873,
    SPELL_ICE_STORM_TRIGGER       = 87469,
    SPELL_ICE_STORM_SUMMON        = 87055,
    SPELL_SQUALL_LINE_VEHICLE     = 87856,
    SPELL_WIND_BURST              = 87770,
    // phase 2
    SPELL_ACID_RAIN_TRIGGER       = 88290,
    SPELL_ACID_RAIN_DOT           = 88301,
    NPC_STORMLING                 = 47175,
    SPELL_FEEDBACK                = 87904, // casted after stormling death
    // phase 3
    SPELL_EYE_OF_THE_STORM        = 82724,
    SPELL_RELENTLESS_STORM_VEHICLE= 89104,
    SPELL_RELENTLESS_STORM_VISUAL = 88866, // visual effect, is this used in encounter?
    NPC_RELENTLESS_STORM          = 47807, // used for lighning?
    SPELL_LIGHTNING_ROD           = 89667,
    // lightning clouds?
};

void AddSC_alakir()
{
}
