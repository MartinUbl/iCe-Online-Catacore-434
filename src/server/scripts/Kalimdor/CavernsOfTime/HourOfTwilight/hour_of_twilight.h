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
    TYPE_BOSS_ARCURION                 = 0,
    TYPE_BOSS_ASIRA_DAWNSLAYER         = 1,
    TYPE_BOSS_ARCHBISHOP_BENEDICTUS    = 2,
    MAX_ENCOUNTER                      = 3,
};

enum AdditionalEvents 
{
    DATA_INSTANCE_PROGRESS             = 4,
    DATA_MOVEMENT_PROGRESS             = 5,
    DATA_ASIRA_INTRO                   = 6,
    DATA_DRAKES                        = 7,
    DATA_ECLIPSE_ACHIEVEMENT           = 8,
    DATA_BENEDICTUS_INTRO              = 9,
};

enum HourOfTwilightNpcs
{
    TYPE_THRALL                        = 10, 
    TYPE_THRALL1                       = 11,
    TYPE_THRALL2                       = 12,
    TYPE_THRALL3                       = 13,
    TYPE_THRALL4                       = 14,
};
#endif