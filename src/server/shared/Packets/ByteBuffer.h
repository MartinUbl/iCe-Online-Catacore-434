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

#ifndef _BYTEBUFFER_H
#define _BYTEBUFFER_H

#include "Common.h"
#include "Debugging/Errors.h"
#include "Logging/Log.h"
#include "Utilities/ByteConverter.h"

class ByteBufferException
{
    public:
        ByteBufferException(bool _add, size_t _pos, size_t _esize, size_t _size)
            : add(_add), pos(_pos), esize(_esize), size(_size)
        {
            PrintPosError();
        }

        void PrintPosError() const
        {
            sLog->outError("Attempted to %s in ByteBuffer (pos: " SIZEFMTD " size: " SIZEFMTD ") value with size: " SIZEFMTD,
                (add ? "put" : "get"), pos, size, esize);
        }
    private:
        bool add;
        size_t pos;
        size_t esize;
        size_t size;
};

class BitStream
{
    public:
        BitStream(): _rpos(0), _wpos(0) {}

        BitStream(uint32 val, size_t len): _rpos(0), _wpos(0)
        {
            WriteBits(val, len);
        }

        BitStream(BitStream const& bs) : _data(bs._data), _rpos(bs._rpos), _wpos(bs._wpos) {}

        void Clear()
        {
            _data.clear();
            _rpos = _wpos = 0;
        }

        uint8 GetBit(uint32 bit)
        {
            ASSERT(_data.size() > bit);
            return _data[bit];
        }
        uint8 ReadBit()
        {
            ASSERT(_data.size() < _rpos);
            uint8 b = _data[_rpos];
            ++_rpos;
            return b;
        }
        void WriteBit(uint32 bit)
        {
            _data.push_back(bit ? uint8(1) : uint8(0));
            ++_wpos;
        }
        template <typename T> void WriteBits(T value, size_t bits)
        {
            for (int32 i = bits-1; i >= 0; --i)
                WriteBit((value >> i) & 1);
        }
        bool Empty()
        {
            return _data.empty();
        }
        void Reverse()
        {
            uint32 len = GetLength();
            std::vector<uint8> b = _data;
            Clear();

            for(uint32 i = len; i > 0; --i)
                WriteBit(b[i-1]);
        }
        void Print()
        {
            if (!sLog->IsOutDebug())
                return;
            std::stringstream ss;
            ss << "BitStream: ";
            for (uint32 i = 0; i < GetLength(); ++i)
                ss << uint32(GetBit(i)) << " ";

            sLog->outDebug("%s", ss.str().c_str());
        }

        size_t GetLength() { return _data.size();}
        uint32 GetReadPosition() { return _rpos; }
        uint32 GetWritePosition() { return _wpos; }
        void SetReadPos(uint32 pos) { _rpos = pos; }

        uint8 const& operator[](uint32 const pos) const
        {
            return _data[pos];
        }

        uint8& operator[] (uint32 const pos)
        {
            return _data[pos];
        }

    private:
        std::vector<uint8> _data;
        uint32 _rpos, _wpos;
};

class ByteBuffer
{
    public:
        const static size_t DEFAULT_SIZE = 0x1000;

        // constructor
        ByteBuffer(): _rpos(0), _wpos(0), _bitpos(8), _curbitval(0)
        {
            _storage.reserve(DEFAULT_SIZE);
        }

        // constructor
        ByteBuffer(size_t res, bool init = false): _rpos(0), _wpos(0), _bitpos(8), _curbitval(0)
        {
            if (init)
                _storage.resize(res, 0);
            else
                _storage.reserve(res);
        }

        // copy constructor
        ByteBuffer(const ByteBuffer &buf) : _rpos(buf._rpos), _wpos(buf._wpos),
            _bitpos(buf._bitpos), _curbitval(buf._curbitval), _storage(buf._storage)
        {
        }

        void clear()
        {
            _storage.clear();
            _rpos = _wpos = 0;
        }

        template <typename T> void append(T value)
        {
            FlushBits();
            EndianConvert(value);
            append((uint8 *)&value, sizeof(value));
        }

        void FlushBits()
        {
            if (_bitpos == 8)
                return;

            append((uint8 *)&_curbitval, sizeof(uint8));
            _curbitval = 0;
            _bitpos = 8;
        }

        void SetBitPoz()
        {
            if (_bitpos < 7)
            {
                _bitpos = 7;
            }
        }

        bool WriteBit(uint32 bit)
        {
            --_bitpos;
            if (bit)
                _curbitval |= (1 << (_bitpos));

            if (_bitpos == 0)
            {
                _bitpos = 8;
                append((uint8 *)&_curbitval, sizeof(_curbitval));
                _curbitval = 0;
            }

            return (bit != 0);
        }

        bool ReadBit()
        {
            ++_bitpos;
            if (_bitpos > 7)
            {
                _bitpos = 0;
                _curbitval = read<uint8>();
            }

            return ((_curbitval >> (7-_bitpos)) & 1) != 0;
        }

        template <typename T> void WriteBits(T value, size_t bits)
        {
            for (int32 i = bits-1; i >= 0; --i)
                WriteBit((value >> i) & 1);
        }

        uint32 ReadBits(size_t bits)
        {
            uint32 value = 0;
            for (int32 i = bits-1; i >= 0; --i)
                if (ReadBit())
                    value |= (1 << (i));

            return value;
        }

        BitStream ReadBitStream(uint32 len)
        {
            BitStream b;
            for (uint32 i = 0; i < len; ++i)
                b.WriteBit(ReadBit());
            return b;
        }

        void ReadByteMask(uint8& b)
        {
            b = ReadBit() ? 1 : 0;
        }

        void ReadByteSeq(uint8& b)
        {
            if (b != 0)
                b ^= read<uint8>();
        }

        uint8 ReadXorByte()
        {
            return ReadUInt8() ^ 1;
        }

        void ReadXorByte(uint32 bit, uint8& byte)
        {
            if (!bit)
                byte = 0;
            else
                byte = ReadUInt8() ^ bit;
        }

        void WriteByteMask(uint8 b)
        {
            WriteBit(b);
        }

        void WriteByteSeq(uint8 b)
        {
            if (b != 0)
                append<uint8>(b ^ 1);
        }

         void WriteGuidMask(uint64 guid, uint8* maskOrder, uint8 maskCount, uint8 maskPos = 0)
        {
            uint8* guidByte = ((uint8*)&guid);

            for (uint8 i = 0; i < maskCount; i++)
                WriteBit(guidByte[maskOrder[i + maskPos]]);
        }

        void WriteGuidBytes(uint64 guid, uint8* byteOrder, uint8 byteCount, uint8 bytePos)
        {
            uint8* guidByte = ((uint8*)&guid);

            for (uint8 i = 0; i < byteCount; i++)
                if (guidByte[byteOrder[i + bytePos]])
                    (*this) << uint8(guidByte[byteOrder[i + bytePos]] ^ 1);
        }

        // Method for writing strings that have their length sent separately in packet
        // without null-terminating the string
        void WriteString(std::string const& str)
        {
            if (size_t len = str.length())
                append(str.c_str(), len);
        }

        template <typename T> void put(size_t pos, T value)
        {
            EndianConvert(value);
            put(pos, (uint8 *)&value, sizeof(value));
        }

        template <typename T> void PutBits(size_t pos, T value, uint32 bitCount)
        {
            if (pos + bitCount > size() * 8)
                throw ByteBufferException(false, (pos + bitCount) / 8, size(), (bitCount - 1) / 8 + 1);

            for (uint32 i = 0; i < bitCount; ++i)
            {
                size_t wp = (pos + i) / 8;
                size_t bit = (pos + i) % 8;
                if ((value >> (bitCount - i - 1)) & 1)
                    _storage[wp] |= 1 << (7 - bit);
                else
                    _storage[wp] &= ~(1 << (7 - bit));
            }
        }

        ByteBuffer &operator<<(uint8 value)
        {
            append<uint8>(value);
            return *this;
        }

        ByteBuffer &operator<<(uint16 value)
        {
            append<uint16>(value);
            return *this;
        }

        ByteBuffer &operator<<(uint32 value)
        {
            append<uint32>(value);
            return *this;
        }

        ByteBuffer &operator<<(uint64 value)
        {
            append<uint64>(value);
            return *this;
        }

        // signed as in 2e complement
        ByteBuffer &operator<<(int8 value)
        {
            append<int8>(value);
            return *this;
        }

        ByteBuffer &operator<<(int16 value)
        {
            append<int16>(value);
            return *this;
        }

        ByteBuffer &operator<<(int32 value)
        {
            append<int32>(value);
            return *this;
        }

        ByteBuffer &operator<<(int64 value)
        {
            append<int64>(value);
            return *this;
        }

        // floating points
        ByteBuffer &operator<<(float value)
        {
            append<float>(value);
            return *this;
        }

        ByteBuffer &operator<<(double value)
        {
            append<double>(value);
            return *this;
        }

        ByteBuffer &operator<<(const std::string &value)
        {
            append((uint8 const*)value.c_str(), value.length());
            append((uint8)0);
            return *this;
        }

        ByteBuffer &operator<<(const char *str)
        {
            append((uint8 const*)str, str ? strlen(str) : 0);
            append((uint8)0);
            return *this;
        }

        ByteBuffer &operator>>(bool &value)
        {
            value = read<char>() > 0 ? true : false;
            return *this;
        }

        ByteBuffer &operator>>(uint8 &value)
        {
            value = read<uint8>();
            return *this;
        }

        ByteBuffer &operator>>(uint16 &value)
        {
            value = read<uint16>();
            return *this;
        }

        ByteBuffer &operator>>(uint32 &value)
        {
            value = read<uint32>();
            return *this;
        }

        ByteBuffer &operator>>(uint64 &value)
        {
            value = read<uint64>();
            return *this;
        }

        //signed as in 2e complement
        ByteBuffer &operator>>(int8 &value)
        {
            value = read<int8>();
            return *this;
        }

        ByteBuffer &operator>>(int16 &value)
        {
            value = read<int16>();
            return *this;
        }

        ByteBuffer &operator>>(int32 &value)
        {
            value = read<int32>();
            return *this;
        }

        ByteBuffer &operator>>(int64 &value)
        {
            value = read<int64>();
            return *this;
        }

        ByteBuffer &operator>>(float &value)
        {
            value = read<float>();
            return *this;
        }

        ByteBuffer &operator>>(double &value)
        {
            value = read<double>();
            return *this;
        }

        ByteBuffer &operator>>(std::string& value)
        {
            value.clear();
            while (rpos() < size())                         // prevent crash at wrong string format in packet
            {
                char c = read<char>();
                if (c == 0)
                    break;
                value += c;
            }
            return *this;
        }

        uint8& operator[](size_t const pos)
        {
            if (pos >= size())
                throw ByteBufferException(false, pos, 1, size());
            return _storage[pos];
        }

        uint8 const& operator[](size_t const pos) const
        {
            if (pos >= size())
                throw ByteBufferException(false, pos, 1, size());
            return _storage[pos];
        }

        size_t rpos() const { return _rpos; }

        size_t rpos(size_t rpos_)
        {
            _rpos = rpos_;
            return _rpos;
        }

        void rfinish()
        {
            _rpos = wpos();
        }

        size_t wpos() const { return _wpos; }

        size_t wpos(size_t wpos_)
        {
            _wpos = wpos_;
            return _wpos;
        }

        // position of last written bit
        size_t bitwpos() const { return _wpos * 8 + 8 - _bitpos; }

        size_t bitwpos(size_t newPos)
        {
            _wpos = newPos / 8;
            _bitpos = 8 - (newPos % 8);
            return _wpos * 8 + 8 - _bitpos;
        }

        template<typename T>
        void read_skip() { read_skip(sizeof(T)); }

        void read_skip(size_t skip)
        {
            if (_rpos + skip > size())
                throw ByteBufferException(false, _rpos, skip, size());
            _rpos += skip;
        }

        template <typename T> T read()
        {
            T r = read<T>(_rpos);
            _rpos += sizeof(T);
            return r;
        }

        template <typename T> T read(size_t pos) const
        {
            if (pos + sizeof(T) > size())
                throw ByteBufferException(false, pos, sizeof(T), size());
            T val = *((T const*)&_storage[pos]);
            EndianConvert(val);
            return val;
        }

        void read(uint8 *dest, size_t len)
        {
            if (_rpos  + len > size())
               throw ByteBufferException(false, _rpos, len, size());
            memcpy(dest, &_storage[_rpos], len);
            _rpos += len;
        }

        void readPackGUID(uint64& guid)
        {
            if (rpos() + 1 > size())
                throw ByteBufferException(false, _rpos, 1, size());

            guid = 0;

            uint8 guidmark = 0;
            (*this) >> guidmark;

            for (int i = 0; i < 8; ++i)
            {
                if (guidmark & (uint8(1) << i))
                {
                    if (rpos() + 1 > size())
                        throw ByteBufferException(false, _rpos, 1, size());

                    uint8 bit;
                    (*this) >> bit;
                    guid |= (uint64(bit) << (i * 8));
                }
            }
        }

        uint32 ReadPackedTime()
        {
            uint32 packedDate = read<uint32>();
            tm lt = tm();

            lt.tm_min = packedDate & 0x3F;
            lt.tm_hour = (packedDate >> 6) & 0x1F;
            //lt.tm_wday = (packedDate >> 11) & 7;
            lt.tm_mday = ((packedDate >> 14) & 0x3F) + 1;
            lt.tm_mon = (packedDate >> 20) & 0xF;
            lt.tm_year = ((packedDate >> 24) & 0x1F) + 100;

            #if _MSC_VER >= 1900
                #define __timezone _timezone
            #else
                #define __timezone timezone
            #endif

            return uint32(mktime(&lt) + __timezone);
        }

        ByteBuffer& ReadPackedTime(uint32& time)
        {
            time = ReadPackedTime();
            return *this;
        }

        uint8 ReadUInt8()
        {
            uint8 u = 0;
            (*this) >> u;
            return u;
        }

        uint16 ReadUInt16()
        {
            uint16 u = 0;
            (*this) >> u;
            return u;
        }

        uint32 ReadUInt32()
        {
            uint32 u = 0;
            (*this) >> u;
            return u;
        }

        uint64 ReadUInt64()
        {
            uint64 u = 0;
            (*this) >> u;
            return u;
        }

        int8 ReadInt8()
        {
            int8 u = 0;
            (*this) >> u;
            return u;
        }

        int16 ReadInt16()
        {
            int16 u = 0;
            (*this) >> u;
            return u;
        }

        int32 ReadInt32()
        {
            uint32 u = 0;
            (*this) >> u;
            return u;
        }

        int64 ReadInt64()
        {
            int64 u = 0;
            (*this) >> u;
            return u;
        }

        std::string ReadString()
        {
            std::string s = 0;
            (*this) >> s;
            return s;
        }

        std::string ReadString(uint32 length)
        {
            if (length == 0)
                return "";

            char* buffer = new char[length + 1];
            memset(buffer, 0, length + 1);
            read((uint8*)buffer, length);
            std::string retval = buffer;
            delete[] buffer;
            return retval;
        }

        bool ReadBoolean()
        {
            uint8 b = 0;
            (*this) >> b;
            return b > 0 ? true : false;
        }

        float ReadSingle()
        {
            float f = 0;
            (*this) >> f;
            return f;
        }

        const uint8 *contents() const { return &_storage[0]; }

        size_t size() const { return _storage.size(); }
        bool empty() const { return _storage.empty(); }

        void resize(size_t newsize)
        {
            _storage.resize(newsize);
            _rpos = 0;
            _wpos = size();
        }

        void reserve(size_t ressize)
        {
            if (ressize > size())
                _storage.reserve(ressize);
        }

        void append(const std::string& str)
        {
            append((uint8 const*)str.c_str(), str.size() + 1);
        }

        void append(const char *src, size_t cnt)
        {
            return append((const uint8 *)src, cnt);
        }

        template<class T> void append(const T *src, size_t cnt)
        {
            return append((const uint8 *)src, cnt * sizeof(T));
        }

        void append(const uint8 *src, size_t cnt)
        {
            if (!cnt)
                return;

            ASSERT(size() < 10000000);

            if (_storage.size() < _wpos + cnt)
                _storage.resize(_wpos + cnt);
            memcpy(&_storage[_wpos], src, cnt);
            _wpos += cnt;
        }

        void append(const ByteBuffer& buffer)
        {
            if (buffer.wpos())
                append(buffer.contents(), buffer.wpos());
        }

        // can be used in SMSG_MONSTER_MOVE opcode
        void appendPackXYZ(float x, float y, float z)
        {
            uint32 packed = 0;
            packed |= ((int)(x / 0.25f) & 0x7FF);
            packed |= ((int)(y / 0.25f) & 0x7FF) << 11;
            packed |= ((int)(z / 0.25f) & 0x3FF) << 22;
            *this << packed;
        }

        void appendPackGUID(uint64 guid)
        {
            uint8 packGUID[8+1];
            packGUID[0] = 0;
            size_t size = 1;
            for (uint8 i = 0;guid != 0;++i)
            {
                if (guid & 0xFF)
                {
                    packGUID[0] |= uint8(1 << i);
                    packGUID[size] =  uint8(guid & 0xFF);
                    ++size;
                }

                guid >>= 8;
            }
            append(packGUID, size);
        }

        void appendSpecialString(std::string string)
        {
            append(string.c_str(), string.length());
        }

        void appendSpecialString(char* string)
        {
            append(string, strlen(string));
        }

        void AppendPackedTime(time_t time)
        {
            tm* lt = localtime(&time);
            append<uint32>((lt->tm_year - 100) << 24 | lt->tm_mon  << 20 | (lt->tm_mday - 1) << 14 | lt->tm_wday << 11 | lt->tm_hour << 6 | lt->tm_min);
        }

        void put(size_t pos, const uint8 *src, size_t cnt)
        {
            if (pos + cnt > size())
               throw ByteBufferException(true, pos, cnt, size());
            memcpy(&_storage[pos], src, cnt);
        }

        void print_storage() const
        {
            if (!sLog->IsOutDebug())                          // optimize disabled debug output
                return;

            sLog->outDebug("STORAGE_SIZE: %lu", (unsigned long)size() );
            for (uint32 i = 0; i < size(); ++i)
                sLog->outDebugInLine("%u - ", read<uint8>(i) );
            sLog->outDebug(" ");
        }

        void textlike() const
        {
            if (!sLog->IsOutDebug())                          // optimize disabled debug output
                return;

            sLog->outDebug("STORAGE_SIZE: %lu", (unsigned long)size() );
            for (uint32 i = 0; i < size(); ++i)
                sLog->outDebugInLine("%c", read<uint8>(i) );
            sLog->outDebug(" ");
        }

        void hexlike() const
        {
            if (!sLog->IsOutDebug())                          // optimize disabled debug output
                return;

            uint32 j = 1, k = 1;
            sLog->outDebug("STORAGE_SIZE: %lu", (unsigned long)size() );

            for (uint32 i = 0; i < size(); ++i)
            {
                if ((i == (j * 8)) && ((i != (k * 16))))
                {
                    if (read<uint8>(i) < 0x10)
                    {
                        sLog->outDebugInLine("| 0%X ", read<uint8>(i) );
                    }
                    else
                    {
                        sLog->outDebugInLine("| %X ", read<uint8>(i) );
                    }
                    ++j;
                }
                else if (i == (k * 16))
                {
                    if (read<uint8>(i) < 0x10)
                    {
                        sLog->outDebugInLine("\n");

                        sLog->outDebugInLine("0%X ", read<uint8>(i) );
                    }
                    else
                    {
                        sLog->outDebugInLine("\n");

                        sLog->outDebugInLine("%X ", read<uint8>(i) );
                    }

                    ++k;
                    ++j;
                }
                else
                {
                    if (read<uint8>(i) < 0x10)
                    {
                        sLog->outDebugInLine("0%X ", read<uint8>(i) );
                    }
                    else
                    {
                        sLog->outDebugInLine("%X ", read<uint8>(i) );
                    }
                }
            }
            sLog->outDebugInLine("\n");
        }

    protected:
        size_t _rpos, _wpos, _bitpos;
        uint8 _curbitval;
        std::vector<uint8> _storage;
};

template <typename T>
inline ByteBuffer &operator<<(ByteBuffer &b, std::vector<T> v)
{
    b << (uint32)v.size();
    for (typename std::vector<T>::iterator i = v.begin(); i != v.end(); ++i)
    {
        b << *i;
    }
    return b;
}

template <typename T>
inline ByteBuffer &operator>>(ByteBuffer &b, std::vector<T> &v)
{
    uint32 vsize;
    b >> vsize;
    v.clear();
    while (vsize--)
    {
        T t;
        b >> t;
        v.push_back(t);
    }
    return b;
}

template <typename T>
inline ByteBuffer &operator<<(ByteBuffer &b, std::list<T> v)
{
    b << (uint32)v.size();
    for (typename std::list<T>::iterator i = v.begin(); i != v.end(); ++i)
    {
        b << *i;
    }
    return b;
}

template <typename T>
inline ByteBuffer &operator>>(ByteBuffer &b, std::list<T> &v)
{
    uint32 vsize;
    b >> vsize;
    v.clear();
    while (vsize--)
    {
        T t;
        b >> t;
        v.push_back(t);
    }
    return b;
}

template <typename K, typename V>
inline ByteBuffer &operator<<(ByteBuffer &b, std::map<K, V> &m)
{
    b << (uint32)m.size();
    for (typename std::map<K, V>::iterator i = m.begin(); i != m.end(); ++i)
    {
        b << i->first << i->second;
    }
    return b;
}

template <typename K, typename V>
inline ByteBuffer &operator>>(ByteBuffer &b, std::map<K, V> &m)
{
    uint32 msize;
    b >> msize;
    m.clear();
    while (msize--)
    {
        K k;
        V v;
        b >> k >> v;
        m.insert(make_pair(k, v));
    }
    return b;
}

// TODO: Make a ByteBuffer.cpp and move all this inlining to it.
template<> inline std::string ByteBuffer::read<std::string>()
{
    std::string tmp;
    *this >> tmp;
    return tmp;
}

template<>
inline void ByteBuffer::read_skip<char*>()
{
    std::string temp;
    *this >> temp;
}

template<>
inline void ByteBuffer::read_skip<char const*>()
{
    read_skip<char*>();
}

template<>
inline void ByteBuffer::read_skip<std::string>()
{
    read_skip<char*>();
}

class BitConverter
{
    public:
        static uint8 ToUInt8(ByteBuffer const& buff, size_t start = 0)
        {
            return buff.read<uint8>(start);
        }

        static uint16 ToUInt16(ByteBuffer const& buff, size_t start = 0)
        {
            return buff.read<uint16>(start);
        }

        static uint32 ToUInt32(ByteBuffer const& buff, size_t start = 0)
        {
            return buff.read<uint32>(start);
        }

        static uint64 ToUInt64(ByteBuffer const& buff, size_t start = 0)
        {
            return buff.read<uint64>(start);
        }

        static int16 ToInt16(ByteBuffer const& buff, size_t start = 0)
        {
            return buff.read<int16>(start);
        }

        static int32 ToInt32(ByteBuffer const& buff, size_t start = 0)
        {
            return buff.read<int32>(start);
        }

        static int64 ToInt64(ByteBuffer const& buff, size_t start = 0)
        {
            return buff.read<int64>(start);
        }
};

#endif

