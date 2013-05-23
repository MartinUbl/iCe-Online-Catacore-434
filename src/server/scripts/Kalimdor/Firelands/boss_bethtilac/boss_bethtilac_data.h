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

/*
 * shared data used by creatures in Beth'tilac encounter
 */


#ifndef BOSS_BETHTILAC_DATA_H
#define BOSS_BETHTILAC_DATA_H


// Encounter data

static const float webZPosition = 111.7f;
//static const float groundZPosition = 74.042f;

enum BethtilacSpawns
{
    NPC_BETHTILAC = 52498,

    // spawns in waves
    NPC_CINDERWEB_DRONE = 52581,
    NPC_CINDERWEB_SPINNER = 52524,
    NPC_CINDERWEB_SPIDERLING = 52447,
};

// movement IDs
enum MovementId { MOVE_POINT_UP = 1, MOVE_POINT_DOWN };


enum SharedSpells
{
    SPELL_SPIDERWEB_FILAMENT = 98623,
    SPELL_CONSUME = 99304,
};


#endif // BOSS_BETHTILAC_DATA_H
