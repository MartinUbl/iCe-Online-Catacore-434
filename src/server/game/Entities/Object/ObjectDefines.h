/*
 * Copyright (C) 2005-2011 MaNGOS <http://www.getmangos.com/>
 *
 * Copyright (C) 2008-2011 Trinity <http://www.trinitycore.org/>
 *
 * Copyright (C) 2010-2011 Project SkyFire <http://www.projectskyfire.org/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef TRINITY_OBJECTDEFINES_H
#define TRINITY_OBJECTDEFINES_H

#include "Define.h"

// used for creating values for respawn for example
#define MAKE_PAIR64(l, h)  uint64(uint32(l) | (uint64(h) << 32))
#define PAIR64_HIPART(x)   (uint32)((uint64(x) >> 32) & UI64LIT(0x00000000FFFFFFFF))
#define PAIR64_LOPART(x)   (uint32)(uint64(x)         & UI64LIT(0x00000000FFFFFFFF))

#define MAKE_PAIR16(l, h)  uint16(uint8(l) | (uint16(h) << 8))
#define MAKE_PAIR32(l, h)  uint32(uint16(l) | (uint32(h) << 16))
#define PAIR32_HIPART(x)   (uint16)((uint32(x) >> 16) & 0x0000FFFF)
#define PAIR32_LOPART(x)   (uint16)(uint32(x)         & 0x0000FFFF)

enum HighGuid
{
    HIGHGUID_ITEM           = 0x400,                       // blizz 4000
    HIGHGUID_CONTAINER      = 0x400,                       // blizz 4000
    HIGHGUID_PLAYER         = 0x000,                       // blizz 0000
    HIGHGUID_GAMEOBJECT     = 0xF11,                       // blizz F110
    HIGHGUID_TRANSPORT      = 0xF12,                       // blizz F120 (for GAMEOBJECT_TYPE_TRANSPORT)
    HIGHGUID_UNIT           = 0xF13,                       // blizz F130
    HIGHGUID_PET            = 0xF14,                       // blizz F140
    HIGHGUID_VEHICLE        = 0xF15,                       // blizz F550
    HIGHGUID_DYNAMICOBJECT  = 0xF10,                       // blizz F100
    HIGHGUID_CORPSE         = 0xF101,                      // blizz F100
    HIGHGUID_AREATRIGGER    = 0xF102,                      // blizz F100
    HIGHGUID_BATTLEGROUND   = 0x1F1,                       // new 4.x
    HIGHGUID_MO_TRANSPORT   = 0x1FC,                       // blizz 1FC0 (for GAMEOBJECT_TYPE_MO_TRANSPORT)
    HIGHGUID_GROUP          = 0x1F5,
    HIGHGUID_GUILD          = 0x1FF5,                      // new 4.x
};

#define IS_EMPTY_GUID(Guid)          (Guid == 0)

#define IS_CREATURE_GUID(Guid)       (GUID_HIPART(Guid) == HIGHGUID_UNIT)
#define IS_PET_GUID(Guid)            (GUID_HIPART(Guid) == HIGHGUID_PET)
#define IS_VEHICLE_GUID(Guid)        (GUID_HIPART(Guid) == HIGHGUID_VEHICLE)
#define IS_CRE_OR_VEH_GUID(Guid)     (IS_CREATURE_GUID(Guid) || IS_VEHICLE_GUID(Guid))
#define IS_CRE_OR_VEH_OR_PET_GUID(Guid)(IS_CRE_OR_VEH_GUID(Guid) || IS_PET_GUID(Guid))
#define IS_PLAYER_GUID(Guid)         (GUID_HIPART(Guid) == HIGHGUID_PLAYER && Guid != 0)
#define IS_GUILD_GUID(Guid)          (GUID_HIPART(Guid) == HIGHGUID_GUILD && Guid != 0)
#define IS_UNIT_GUID(Guid)           (IS_CRE_OR_VEH_OR_PET_GUID(Guid) || IS_PLAYER_GUID(Guid))
                                                            // special case for empty guid need check
#define IS_ITEM_GUID(Guid)           (GUID_HIPART(Guid) == HIGHGUID_ITEM)
#define IS_GAMEOBJECT_GUID(Guid)     (GUID_HIPART(Guid) == HIGHGUID_GAMEOBJECT)
#define IS_DYNAMICOBJECT_GUID(Guid)  (GUID_HIPART(Guid) == HIGHGUID_DYNAMICOBJECT)
#define IS_CORPSE_GUID(Guid)         (GUID_HIPART(Guid) == HIGHGUID_CORPSE)
#define IS_TRANSPORT(Guid)           (GUID_HIPART(Guid) == HIGHGUID_TRANSPORT)
#define IS_MO_TRANSPORT(Guid)        (GUID_HIPART(Guid) == HIGHGUID_MO_TRANSPORT)
#define IS_GROUP(Guid)               (GUID_HIPART(Guid) == HIGHGUID_GROUP)
#define IS_GUILD(Guid)               (GUID_HIPART(Guid) == HIGHGUID_GUILD)
#define IS_AREATRIGGER_GUID(Guid)    (GUID_HIPART(guid) == HIGHGUID_AREATRIGGER)

// l - OBJECT_FIELD_GUID
// e - OBJECT_FIELD_ENTRY for GO (except GAMEOBJECT_TYPE_MO_TRANSPORT) and creatures or UNIT_FIELD_PETNUMBER for pets
// h - OBJECT_FIELD_GUID + 1
#define MAKE_NEW_GUID(l, e, h)   uint64(uint64(l) | (uint64(e) << 32) | (uint64(h) << ((h == HIGHGUID_GUILD || h == HIGHGUID_CORPSE || h == HIGHGUID_AREATRIGGER) ? 48 : 52)))

//#define GUID_HIPART(x)   (uint32)((uint64(x) >> 52)) & 0x0000FFFF)
inline uint32 GUID_HIPART(uint64 guid)
{
    uint32 t = ((uint64(guid) >> 48) & 0x0000FFFF);
    return (t == HIGHGUID_GUILD || t == HIGHGUID_CORPSE || t == HIGHGUID_AREATRIGGER) ? t : ((uint32(t) >> 4) & 0x00000FFF);
}

// We have different low and middle part size for different guid types
#define _GUID_ENPART_2(x) 0
#define _GUID_ENPART_3(x) (uint32)((uint64(x) >> 32) & UI64LIT(0x00000000000FFFFF))
#define _GUID_LOPART_2(x) (uint32)(uint64(x)         & UI64LIT(0x00000000FFFFFFFF))
#define _GUID_LOPART_3(x) (uint32)(uint64(x)         & UI64LIT(0x00000000FFFFFFFF))

inline bool IsGuidHaveEnPart(uint64 guid)
{
    switch (GUID_HIPART(guid))
    {
        case HIGHGUID_ITEM:
        case HIGHGUID_PLAYER:
        case HIGHGUID_DYNAMICOBJECT:
        case HIGHGUID_CORPSE:
        case HIGHGUID_GROUP:
        case HIGHGUID_GUILD:
            return false;
        case HIGHGUID_GAMEOBJECT:
        case HIGHGUID_TRANSPORT:
        case HIGHGUID_UNIT:
        case HIGHGUID_PET:
        case HIGHGUID_VEHICLE:
        case HIGHGUID_MO_TRANSPORT:
        case HIGHGUID_AREATRIGGER:
        default:
            return true;
    }
}

#define GUID_ENPART(x) (IsGuidHaveEnPart(x) ? _GUID_ENPART_3(x) : _GUID_ENPART_2(x))
#define GUID_LOPART(x) (IsGuidHaveEnPart(x) ? _GUID_LOPART_3(x) : _GUID_LOPART_2(x))

inline char const* GetLogNameForGuid(uint64 guid)
{
    switch (GUID_HIPART(guid))
    {
        case HIGHGUID_ITEM:         return "item";
        case HIGHGUID_PLAYER:       return guid ? "player" : "none";
        case HIGHGUID_GAMEOBJECT:   return "gameobject";
        case HIGHGUID_TRANSPORT:    return "transport";
        case HIGHGUID_UNIT:         return "creature";
        case HIGHGUID_PET:          return "pet";
        case HIGHGUID_VEHICLE:      return "vehicle";
        case HIGHGUID_DYNAMICOBJECT:return "dynobject";
        case HIGHGUID_CORPSE:       return "corpse";
        case HIGHGUID_MO_TRANSPORT: return "mo_transport";
        case HIGHGUID_GROUP:        return "group";
        case HIGHGUID_GUILD:        return "guild";
        case HIGHGUID_AREATRIGGER:  return "areatrigger";
        default:
            return "<unknown>";
    }
}
#endif

