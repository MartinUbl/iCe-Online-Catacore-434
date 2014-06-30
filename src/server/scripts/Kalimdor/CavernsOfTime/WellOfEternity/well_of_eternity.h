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

#ifndef _WELL_OF_ETER_H_
#define _WELL_OF_ETER_H_

enum EncounterData
{
   DATA_PEROTHARN,
   DATA_QUEEN_AZSHARA,
   DATA_MANNOROTH,
   DATA_CAPTAIN_VAROTHEN
};

enum BossEntry
{
    PEROTHARN_ENTRY = 55085,
    QUEEN_AZSHARA_ENTRY = 54853,
    MANNOROTH_ENTRY = 54969,
    VAROTHEN_ENTRY = 55419
};

/***************** TRASH STUFF *********************/

enum TrashEntries
{
    LEGION_DEMON_ENTRY = 54500,
};

enum DemonData
{
    DEMON_DATA_DIRECTION,
    DEMON_DATA_WAVE
};

enum DemonDirection
{
    DIRECTION_LEFT,
    DIRECTION_RIGHT,
    DIRECTION_STRAIGHT
};

enum DemonWave
{
    WAVE_ONE,
    WAVE_TWO,
    WAVE_THREE
};

enum WayPointStep
{
    WP_MID = 0,
    WP_END
};

#endif