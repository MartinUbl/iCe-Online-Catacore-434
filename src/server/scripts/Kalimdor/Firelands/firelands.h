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

#ifndef INSTANCE_FIRELANDS_H
#define INSTANCE_FIRELANDS_H

enum Encounters
{
    TYPE_BETHTILAC = 0,
    TYPE_RHYOLITH  = 1,
    TYPE_ALYSRAZOR = 2,
    TYPE_SHANNOX   = 3,
    TYPE_BALEROC   = 4,
    TYPE_STAGHELM  = 5,
    TYPE_RAGNAROS  = 6,
    MAX_ENCOUNTER  = 7
};

enum AdditionNPCs
{
    DATA_RAGEFACE_GUID = 8,
    DATA_RIPLIMB_GUID = 9
};

#endif
