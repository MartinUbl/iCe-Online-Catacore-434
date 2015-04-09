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

#include "Field.h"

Field::Field()
{
    data.value = NULL;
    data.type = MYSQL_TYPE_NULL;
    data.length = 0;
}

Field::~Field()
{
    CleanUp();
}

void Field::SetByteValue(const void* newValue, const size_t newSize, enum_field_types newType, uint32 length)
{
    if (data.value)
        CleanUp();

    // This value stores raw bytes that have to be explicitly casted later
    if (newValue)
    {
        data.value = new char[newSize];
        memcpy(data.value, newValue, newSize);
        data.length = length;
    }
    data.type = newType;
    data.raw = true;
}

void Field::SetStructuredValue(char* newValue, enum_field_types newType)
{
    if (data.value)
        CleanUp();

    // This value stores somewhat structured data that needs function style casting
    if (newValue)
    {
        size_t size = strlen(newValue);
        data.value = new char [size+1];
        strcpy((char*)data.value, newValue);
        data.length = size;
    }

    data.type = newType;
    data.raw = false;
}

bool Field::GetBool() const // Wrapper, actually gets integer
{
    return GetUInt8() == 1 ? true : false;
}

uint8 Field::GetUInt8() const
{
    if (!data.value)
        return 0;

#ifdef TRINITY_DEBUG
    if (!IsNumeric())
    {
        sLog->outSQLDriver("Error: GetUInt8() on non-numeric field.");
        return 0;
    }
#endif
    if (data.raw)
        return *reinterpret_cast<uint8*>(data.value);
    return static_cast<uint8>(atol((char*)data.value));
}

int8 Field::GetInt8() const
{
    if (!data.value)
        return 0;

#ifdef TRINITY_DEBUG
    if (!IsNumeric())
    {
        sLog->outSQLDriver("Error: GeInt8() on non-numeric field.");
        return 0;
    }
#endif
    if (data.raw)
        return *reinterpret_cast<int8*>(data.value);
    return static_cast<int8>(atol((char*)data.value));
}

uint16 Field::GetUInt16() const
{
    if (!data.value)
        return 0;

#ifdef TRINITY_DEBUG
    if (!IsNumeric())
    {
        sLog->outSQLDriver("Error: GetUInt16() on non-numeric field.");
        return 0;
    }
#endif
    if (data.raw)
        return *reinterpret_cast<uint16*>(data.value);
    return static_cast<uint16>(atol((char*)data.value));
}

int16 Field::GetInt16() const
{
    if (!data.value)
        return 0;

#ifdef TRINITY_DEBUG
    if (!IsNumeric())
    {
        sLog->outSQLDriver("Error: GetInt16() on non-numeric field.");
        return 0;
    }
#endif
    if (data.raw)
        return *reinterpret_cast<int16*>(data.value);
    return static_cast<int16>(atol((char*)data.value));
}

uint32 Field::GetUInt32() const
{
    if (!data.value)
        return 0;

#ifdef TRINITY_DEBUG
    if (!IsNumeric())
    {
        sLog->outSQLDriver("Error: GetUInt32() on non-numeric field.");
        return 0;
    }
#endif
    if (data.raw)
        return *reinterpret_cast<uint32*>(data.value);
    return static_cast<uint32>(atol((char*)data.value));
}

int32 Field::GetInt32() const
{
    if (!data.value)
        return 0;

#ifdef TRINITY_DEBUG
    if (!IsNumeric())
    {
        sLog->outSQLDriver("Error: GetInt32() on non-numeric field.");
        return 0;
    }
#endif
    if (data.raw)
        return *reinterpret_cast<int32*>(data.value);
    return static_cast<int32>(atol((char*)data.value));
}

uint64 Field::GetUInt64() const
{
    if (!data.value)
        return 0;

#ifdef TRINITY_DEBUG
    if (!IsNumeric())
    {
        sLog->outSQLDriver("Error: GetUInt64() on non-numeric field.");
        return 0;
    }
#endif
    if (data.raw)
        return *reinterpret_cast<uint64*>(data.value);
    return static_cast<uint64>(strtoull((char*)data.value, NULL, 0));
}

int64 Field::GetInt64() const
{
    if (!data.value)
        return 0;

#ifdef TRINITY_DEBUG
    if (!IsNumeric())
    {
        sLog->outSQLDriver("Error: GetInt64() on non-numeric field.");
        return 0;
    }
#endif
    if (data.raw)
        return *reinterpret_cast<int64*>(data.value);
    return static_cast<int64>(atoll((char*)data.value));
}

float Field::GetFloat() const
{
    if (!data.value)
        return 0.0f;

#ifdef TRINITY_DEBUG
    if (!IsNumeric())
    {
        sLog->outSQLDriver("Error: GetFloat() on non-numeric field.");
        return 0.0f;
    }
#endif
    if (data.raw)
        return *reinterpret_cast<float*>(data.value);
    return static_cast<float>(atof((char*)data.value));
}

double Field::GetDouble() const
{
    if (!data.value)
        return 0.0f;

#ifdef TRINITY_DEBUG
    if (!IsNumeric())
    {
        sLog->outSQLDriver("Error: GetDouble() on non-numeric field.");
        return 0.0f;
    }
#endif
    if (data.raw)
        return *reinterpret_cast<double*>(data.value);
    return static_cast<double>(atof((char*)data.value));
}

const char* Field::GetCString() const
{
    if (!data.value)
        return NULL;

#ifdef TRINITY_DEBUG
    if (IsNumeric())
    {
        sLog->outSQLDriver("Error: GetCString() on numeric field.");
        return NULL;
    }
#endif
    return static_cast<const char*>(data.value);
}

std::string Field::GetString() const
{
    if (!data.value)
        return "";

    if (data.raw)
    {
        const char* string = GetCString();
        if (!string)
            string = "";
        return std::string(string, data.length);
    }
    return std::string((char*)data.value);
}

void Field::CleanUp()
{
    delete[]((char*)data.value);
    data.value = NULL;
}

size_t Field::SizeForType(MYSQL_FIELD* field)
{
    switch (field->type)
    {
        case MYSQL_TYPE_NULL:
            return 0;
        case MYSQL_TYPE_TINY:
            return 1;
        case MYSQL_TYPE_YEAR:
        case MYSQL_TYPE_SHORT:
            return 2;
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_FLOAT:
            return 4;
        case MYSQL_TYPE_DOUBLE:
        case MYSQL_TYPE_LONGLONG:
        case MYSQL_TYPE_BIT:
            return 8;

        case MYSQL_TYPE_TIMESTAMP:
        case MYSQL_TYPE_DATE:
        case MYSQL_TYPE_TIME:
        case MYSQL_TYPE_DATETIME:
            return sizeof(MYSQL_TIME);

        case MYSQL_TYPE_TINY_BLOB:
        case MYSQL_TYPE_MEDIUM_BLOB:
        case MYSQL_TYPE_LONG_BLOB:
        case MYSQL_TYPE_BLOB:
        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_VAR_STRING:
            return field->max_length + 1;

        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_NEWDECIMAL:
            return 64;

        case MYSQL_TYPE_GEOMETRY:
            /*
            Following types are not sent over the wire:
            MYSQL_TYPE_ENUM:
            MYSQL_TYPE_SET:
            */
        default:
            sLog->outSQLDriver("SQL::SizeForType(): invalid field type %u", uint32(field->type));
            return 0;
    }
}

bool Field::IsNumeric() const
{
    return (data.type == MYSQL_TYPE_TINY ||
        data.type == MYSQL_TYPE_SHORT ||
        data.type == MYSQL_TYPE_INT24 ||
        data.type == MYSQL_TYPE_LONG ||
        data.type == MYSQL_TYPE_FLOAT ||
        data.type == MYSQL_TYPE_DOUBLE ||
        data.type == MYSQL_TYPE_LONGLONG);
}
