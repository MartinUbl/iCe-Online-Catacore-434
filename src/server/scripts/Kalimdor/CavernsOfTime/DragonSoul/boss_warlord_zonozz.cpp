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
#include "dragonsoul.h"

enum spells
{
    /* BOSS SPELLS */
    SPELL_FOCUSED_ANGER                 = 104543, // OK
    SPELL_PSYCHIC_DRAIN                 = 104323, // OK
    SPELL_DISRUPTING_SHADOWS            = 103434, // OK (maybe cast with max affected targets)
    SPELL_DISRUPTING_SHADOWS_KNOCKBACK  = 103948, // this should be triggered after dispeling spell above
  //SPELL_BLACK_BLOOD_INFINTY           = 104377, // infinite duration ?
    SPELL_BLACK_BLOOD                   = 104378, // higher damage + 30 s duration
    SPELL_VOID_OF_THE_UNMAKING_SUMMON   = 103571, // summon sphere in front of the caster
    SPELL_DARKNESS                      = 109413, // damn, hate looking up correct spells

    /* VOID SPHERE SPELLS */
    SPELL_VOID_DIFFUSION_DAMAGE         = 103527, // should split damage
    SPELL_VOID_DIFFUSION_TAKEN          = 104031, // stackable damage taken aura (TARGET_UNIT_SRC_AREA_ENTRY) -> just use AddAura on boss instead
    SPELL_VOID_OF_THE_UNMAKING_CHANNEL  = 103946, // dummy channel
    SPELL_VOID_OF_THE_UNMAKING_VISUAL   = 109187, // visual purple aura (infinite)
    SPELL_VOID_OF_THE_UNMAKING_REMOVE   = 105336, // should remove aura above
    SPELL_BLACK_BLOOD_ERUPTION          = 110382, // if sphere hits the edge of the room -> TARGET_UNIT_CASTER -> cast by players themsleves probably

    /* SUMMONS SPELLS*/
    SPELL_OOZE_SPIT                     = 109396, // Casted by Claw of Go'rath when no one in melee range
    SPELL_SHADOW_GAZE                   = 104347, // Casted by Eye of Go'rath to random player
    SPELL_WILD_FLAIL                    = 109199, // Casted be Flail of Go'rath (melee AoE)
    SPELL_SLUDGE_SPEW                   = 110102, // Casted be Flail of Go'rath to random player
};

enum entries
{
    WARLORD_ENTRY               = 55308,
    CLAW_OF_GORATH_ENTRY        = 55418,
    EYE_OF_GORATH_ENTRY         = 57875,
    FLAIL_OF_GORATH_ENTRY       = 55417,
    VOID_SPHERE                 = 55334
};

void AddSC_boss_warlord_zonozz()
{
}