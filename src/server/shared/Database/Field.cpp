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
    data.type = MYSQL_TYPE_NULL;
    data.length = 0;
}

Field::~Field()
{
    CleanUp();
}

void Field::SetByteValue(const void* newValue, const size_t newSize, enum_field_types newType, uint32 length)
{
    // This value stores raw bytes that have to be explicitly casted later
    if (newValue)
    {
        data.value.resize(newSize);
        memcpy(data.value.data(), newValue, newSize);
        data.length = length;
    }
    else
    {
        CleanUp();
    }

    data.type = newType;
    data.raw = true;
}

void Field::SetStructuredValue(char* newValue, enum_field_types newType)
{
    // This value stores somewhat structured data that needs function style casting
    if (newValue)
    {
        size_t size = strlen(newValue);
        data.value.resize(size + 1);
        strcpy(data.value.data(), newValue);
        data.length = size;
    }
    else
    {
        CleanUp();
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
    if (data.value.empty())
        return 0;

#ifdef TRINITY_DEBUG
    if (!IsNumeric())
    {
        sLog->outSQLDriver("Error: GetUInt8() on non-numeric field.");
        return 0;
    }
#endif
    if (data.raw)
        return *reinterpret_cast<const uint8*>(&data.value[0]);
    return static_cast<uint8>(atol(&data.value[0]));
}

int8 Field::GetInt8() const
{
    if (data.value.empty())
        return 0;

#ifdef TRINITY_DEBUG
    if (!IsNumeric())
    {
        sLog->outSQLDriver("Error: GeInt8() on non-numeric field.");
        return 0;
    }
#endif
    if (data.raw)
        return *reinterpret_cast<const int8*>(&data.value[0]);
    return static_cast<int8>(atol(&data.value[0]));
}

uint16 Field::GetUInt16() const
{
    if (data.value.empty())
        return 0;

#ifdef TRINITY_DEBUG
    if (!IsNumeric())
    {
        sLog->outSQLDriver("Error: GetUInt16() on non-numeric field.");
        return 0;
    }
#endif
    if (data.raw)
        return *reinterpret_cast<const uint16*>(&data.value[0]);
    return static_cast<uint16>(atol(&data.value[0]));
}

int16 Field::GetInt16() const
{
    if (data.value.empty())
        return 0;

#ifdef TRINITY_DEBUG
    if (!IsNumeric())
    {
        sLog->outSQLDriver("Error: GetInt16() on non-numeric field.");
        return 0;
    }
#endif
    if (data.raw)
        return *reinterpret_cast<const int16*>(&data.value[0]);
    return static_cast<int16>(atol(&data.value[0]));
}

uint32 Field::GetUInt32() const
{
    if (data.value.empty())
        return 0;

#ifdef TRINITY_DEBUG
    if (!IsNumeric())
    {
        sLog->outSQLDriver("Error: GetUInt32() on non-numeric field.");
        return 0;
    }
#endif
    if (data.raw)
        return *reinterpret_cast<const uint32*>(&data.value[0]);
    return static_cast<uint32>(atol(&data.value[0]));
}

int32 Field::GetInt32() const
{
    if (data.value.empty())
        return 0;

#ifdef TRINITY_DEBUG
    if (!IsNumeric())
    {
        sLog->outSQLDriver("Error: GetInt32() on non-numeric field.");
        return 0;
    }
#endif
    if (data.raw)
        return *reinterpret_cast<const int32*>(&data.value[0]);
    return static_cast<int32>(atol(&data.value[0]));
}

uint64 Field::GetUInt64() const
{
    if (data.value.empty())
        return 0;

#ifdef TRINITY_DEBUG
    if (!IsNumeric())
    {
        sLog->outSQLDriver("Error: GetUInt64() on non-numeric field.");
        return 0;
    }
#endif
    if (data.raw)
        return *reinterpret_cast<const uint64*>(&data.value[0]);
    return static_cast<uint64>(strtoull(&data.value[0], NULL, 0));
}

int64 Field::GetInt64() const
{
    if (data.value.empty())
        return 0;

#ifdef TRINITY_DEBUG
    if (!IsNumeric())
    {
        sLog->outSQLDriver("Error: GetInt64() on non-numeric field.");
        return 0;
    }
#endif
    if (data.raw)
        return *reinterpret_cast<const int64*>(&data.value[0]);
    return static_cast<int64>(atoll(&data.value[0]));
}

float Field::GetFloat() const
{
    if (data.value.empty())
        return 0.0f;

#ifdef TRINITY_DEBUG
    if (!IsNumeric())
    {
        sLog->outSQLDriver("Error: GetFloat() on non-numeric field.");
        return 0.0f;
    }
#endif
    if (data.raw)
        return *reinterpret_cast<const float*>(&data.value[0]);
    return static_cast<float>(atof(&data.value[0]));
}

double Field::GetDouble() const
{
    if (data.value.empty())
        return 0.0f;

#ifdef TRINITY_DEBUG
    if (!IsNumeric())
    {
        sLog->outSQLDriver("Error: GetDouble() on non-numeric field.");
        return 0.0f;
    }
#endif
    if (data.raw)
        return *reinterpret_cast<const double*>(&data.value[0]);
    return static_cast<double>(atof(&data.value[0]));
}

const char* Field::GetCString() const
{
    if (data.value.empty())
        return NULL;

#ifdef TRINITY_DEBUG
    if (IsNumeric())
    {
        sLog->outSQLDriver("Error: GetCString() on numeric field.");
        return NULL;
    }
#endif
    return &data.value[0];
}

std::string Field::GetString() const
{
    if (data.value.empty())
        return "";

    if (data.raw)
    {
        const char* string = GetCString();
        if (!string)
            string = "";
        return std::string(string, data.length);
    }
    return &data.value[0];
}

void Field::CleanUp()
{
    data.value.clear();
    data.length = 0;
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
