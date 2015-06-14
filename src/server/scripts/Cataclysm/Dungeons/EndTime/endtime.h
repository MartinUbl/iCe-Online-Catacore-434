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

#ifndef INSTANCE_ENDTIME_H
#define INSTANCE_TIME_H

enum Encounters
{
    // Bosses
    TYPE_ECHO_OF_SYLVANAS  = 0,
    TYPE_ECHO_OF_JAINA     = 1,
    TYPE_MUROZOND          = 2,
    TYPE_ECHO_OF_BAINE     = 3,
    TYPE_ECHO_OF_TYRANDE   = 4, 
    MAX_ENCOUNTER          = 5,

    TYPE_FIRST_ECHO        = 6,
    TYPE_SECOND_ECHO       = 7,
};

enum AdditionalEvents
{
    DATA_CRYSTALS          = 10,
    DATA_TRASH_MUROZOND    = 11,
    DATA_TRASH_BAINE       = 12,
    DATA_TRASH_TYRANDE     = 13,
    DATA_ECHO_KILLED       = 14,

    TYPE_POOL_OF_MOONLIGHT_1  = 14,
    TYPE_POOL_OF_MOONLIGHT_2  = 15,
    TYPE_POOL_OF_MOONLIGHT_3  = 16,
    TYPE_POOL_OF_MOONLIGHT_4  = 17,
    TYPE_POOL_OF_MOONLIGHT_5  = 18,
};
#endif