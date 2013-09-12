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

enum AdditionalEvents
{
    DATA_BALEROC_FRONT_DOOR = 10,
    DATA_BRIDGE_DOOR        = 11,
    DATA_BRIDGE_SPAWN       = 12
};

enum CreatureIds
{
    NPC_BLAZING_MONSTROSITY_LEFT    = 53786,
    NPC_BLAZING_MONSTROSITY_RIGHT   = 53791,
    NPC_EGG_PILE                    = 53795,
    NPC_HARBINGER_OF_FLAME          = 53793,
    NPC_MOLTEN_EGG_TRASH            = 53914,
    NPC_SMOULDERING_HATCHLING       = 53794,
};

class DelayedAttackStartEvent : public BasicEvent
{
    public:
        DelayedAttackStartEvent(Creature* owner) : _owner(owner) { }

        bool Execute(uint64 /*e_time*/, uint32 /*p_time*/)
        {
            _owner->AI()->DoZoneInCombat(_owner, 200.0f);
            return true;
        }

    private:
        Creature* _owner;
};

#endif
