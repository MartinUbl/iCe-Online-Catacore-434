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
    TYPE_FIRST_ECHO        = 0,
    TYPE_SECOND_ECHO       = 1,
    TYPE_MUROZOND          = 2,
    MAX_ENCOUNTER          = 3,
    // Bosses
    TYPE_ECHO_OF_SYLVANAS  = 4,
    TYPE_ECHO_OF_JAINA     = 5,
    TYPE_ECHO_OF_BAINE     = 6,
    TYPE_ECHO_OF_TYRANDE   = 7,
};

enum AdditionalEvents
{
    DATA_CRYSTALS          = 8,
    DATA_TRASH_MUROZOND    = 9,
    DATA_TRASH_BAINE       = 10,
};
#endif