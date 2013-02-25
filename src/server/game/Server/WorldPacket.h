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

#ifndef TRINITYCORE_WORLDPACKET_H
#define TRINITYCORE_WORLDPACKET_H

#include "Common.h"
#include "ByteBuffer.h"

struct z_stream_s;

class WorldPacket : public ByteBuffer
{
    public:
        // just container for later use
        WorldPacket();
        WorldPacket(uint32 opcode, size_t res = 200, bool hack = false);

        // copy constructor
        WorldPacket(const WorldPacket &packet);

        void Initialize(uint32 opcode, size_t newres = 200, bool hack = false);

        uint32 GetOpcode() const { return m_opcode; }
        void SetOpcode(uint32 opcode) { m_opcode = opcode; m_realOpcode = opcode; }

        uint32 GetRealOpcode() const { return m_realOpcode; }

        void Compress(uint32 opcode);
        void Compress(z_stream_s* compressionStream);
        void Compress(z_stream_s* compressionStream, WorldPacket const* source);

    protected:

        uint32 m_opcode;
        uint32 m_realOpcode;

        void Compress(void* dst, uint32 *dst_size, const void* src, int src_size);

        z_stream_s* _compressionStream;
};
#endif

