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

#ifndef _FIELD_H
#define _FIELD_H

#include "Common.h"
#include "Log.h"

#include <mysql.h>

class Field
{
    friend class ResultSet;
    friend class PreparedResultSet;

    public:
        bool GetBool() const;
        uint8 GetUInt8() const;
        int8 GetInt8() const;
        uint16 GetUInt16() const;
        int16 GetInt16() const;
        uint32 GetUInt32() const;
        int32 GetInt32() const;
        uint64 GetUInt64() const;
        int64 GetInt64() const;
        float GetFloat() const;
        double GetDouble() const;
        const char* GetCString() const;
        std::string GetString() const;
    
    protected:
        Field();
        ~Field();

        struct 
        {
            enum_field_types type;  // Field type
            std::vector<char> value;// Actual data in memory
            bool raw;               // Raw bytes? (Prepared statement or adhoc)
            uint32 length;          // Length (prepared strings only)
        } data;

        void SetByteValue(const void* newValue, const size_t newSize, enum_field_types newType, uint32 length);
        void SetStructuredValue(char* newValue, enum_field_types newType);
        
        void CleanUp();

        static size_t SizeForType(MYSQL_FIELD* field);

        bool IsNumeric() const;
};

#endif

