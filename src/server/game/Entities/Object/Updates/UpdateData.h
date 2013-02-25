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

#ifndef __UPDATEDATA_H
#define __UPDATEDATA_H

#include "ByteBuffer.h"
class WorldPacket;

enum OBJECT_UPDATE_TYPE
{
    UPDATETYPE_VALUES               = 0,
    //UPDATETYPE_MOVEMENT             = 0xFF, //deleted in 4.0.3.
    UPDATETYPE_CREATE_OBJECT        = 1,
    UPDATETYPE_CREATE_OBJECT2       = 2,
    UPDATETYPE_OUT_OF_RANGE_OBJECTS = 3,
    //UPDATETYPE_NEAR_OBJECTS         = 0xFF //deleted in 4.0.3.
};

/*enum OBJECT_UPDATE_FLAGS
{
    UPDATEFLAG_NONE         = 0x0000,
    UPDATEFLAG_SELF         = 0x0001,
    UPDATEFLAG_TRANSPORT    = 0x0002,
    UPDATEFLAG_HAS_TARGET   = 0x0004,
    //UPDATEFLAG_LOWGUID      = 0x0008, deleted in 4.0.3
    //UPDATEFLAG_HIGHGUID     = 0x0010, deleted in 4.0.3
    UPDATEFLAG_LIVING       = 0x0020,
    UPDATEFLAG_HAS_POSITION = 0x0040,
    UPDATEFLAG_VEHICLE      = 0x0080,
    UPDATEFLAG_POSITION     = 0x0100,
    UPDATEFLAG_ROTATION     = 0x0200,
    UPDATEFLAG_UNK1         = 0x0400, //not found.
    UPDATEFLAG_UNK2         = 0x0800, //added in 4.0.3
    UPDATEFLAG_UNK3         = 0x1000, //added in 4.0.3
};*/

enum OBJECT_UPDATE_FLAGS
{
    UPDATEFLAG_HAS_NONE                    = 0x0000,
    UPDATEFLAG_HAS_SELF                    = 0x0001,
    UPDATEFLAG_HAS_GO_TRANSPORT_TIME       = 0x0002,
    UPDATEFLAG_HAS_ATTACKING_TARGET        = 0x0004,
    UPDATEFLAG_HAS_ORIENTATION             = 0x0008,
    UPDATEFLAG_HAS_LOWGUID                 = 0x0010,
    UPDATEFLAG_HAS_LIVING                  = 0x0020,
    UPDATEFLAG_HAS_STATIONARY_POSITION     = 0x0040,
    UPDATEFLAG_HAS_VEHICLE                 = 0x0080,
    UPDATEFLAG_HAS_GO_POSITION             = 0x0100,
    UPDATEFLAG_HAS_GO_ROTATION             = 0x0200,
    UPDATEFLAG_HAS_UNK                     = 0x0400,
    UPDATEFLAG_HAS_ANIMKITS                = 0x0800,
    UPDATEFLAG_HAS_GO_TRANSPORT_TIME_ARRAY = 0x1000,
    UPDATEFLAG_HAS_ENABLE_PORTAL           = 0x2000,
    UPDATEFLAG_HAS_UNK2                    = 0x4000,
};

class UpdateData
{
    public:
        UpdateData(uint32 map);

        void AddOutOfRangeGUID(std::set<uint64>& guids);
        void AddOutOfRangeGUID(const uint64 &guid);
        void AddUpdateBlock(const ByteBuffer &block);
        bool BuildPacket(WorldPacket *packet);
        bool HasData() { return m_blockCount > 0 || !m_outOfRangeGUIDs.empty(); }
        uint32 GetBlockCount() { return m_blockCount; }
        size_t GetDataSize() { return m_data.size(); };
        void Clear();

        std::set<uint64> const& GetOutOfRangeGUIDs() const { return m_outOfRangeGUIDs; }
        uint16 m_map;

    protected:
        uint32 m_blockCount;
        std::set<uint64> m_outOfRangeGUIDs;
        ByteBuffer m_data;
};
#endif

