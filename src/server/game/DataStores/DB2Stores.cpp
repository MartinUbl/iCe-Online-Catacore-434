/*
 * Copyright (C) 2005-2011 MaNGOS <http://www.getmangos.com/>
 *
 * Copyright (C) 2008-2011 Trinity <http://www.trinitycore.org/>
 *
 * Copyright (C) 2010-2011 Strawberry-Pr0jcts <http://www.strawberry-pr0jcts.com/>
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

#include "gamePCH.h"
#include "Logging/Log.h"
#include "SharedDefines.h"
#include "SpellMgr.h"

#include "DB2Store.h"

#include "DB2fmt.h"

DB2Storage <ItemEntry>                    sItemStore(Itemfmt);
DB2Storage <ItemCurrencyCostEntry>        sItemCurrencyCostStore(ItemCurrencyCostEntryfmt);
DB2Storage <ItemExtendedCostEntry>        sItemExtendedCostStore(ItemExtendedCostEntryfmt);
DB2Storage <ItemSparseEntry>              sItemSparseStore (ItemSparsefmt);
DB2Storage <KeyChainEntry>                sKeyChainStore(KeyChainfmt);

#include "DB2Stores.h"

#include <map>

typedef std::list<std::string> StoreProblemList1;

uint32 DB2FileCount = 0;

static bool LoadDB2_assert_print(uint32 fsize,uint32 rsize, const std::string& filename)
{
    sLog->outError("Size of '%s' setted by format string (%u) not equal size of C++ structure (%u).", filename.c_str(), fsize, rsize);

    // ASSERT must fail after function call
    return false;
}

struct LocalDB2Data
{
    LocalDB2Data(LocaleConstant loc) : defaultLocale(loc), availableDb2Locales(0xFFFFFFFF) {}

    LocaleConstant defaultLocale;

    // bitmasks for index of fullLocaleNameList
    uint32 availableDb2Locales;
};

template<class T>
inline void LoadDB2(uint32& availableDb2Locales, StoreProblemList1& errlist, DB2Storage<T>& storage, const std::string& db2_path, const std::string& filename)
{
    // compatibility format and C++ structure sizes
    ASSERT(DB2FileLoader::GetFormatRecordSize(storage.GetFormat()) == sizeof(T) || LoadDB2_assert_print(DB2FileLoader::GetFormatRecordSize(storage.GetFormat()), sizeof(T), filename));

    ++DB2FileCount;
    std::string db2_filename = db2_path + filename;
    if (storage.Load(db2_filename.c_str()))
    {
        //bar.step();
    }
    else
    {
        // sort problematic db2 to (1) non compatible and (2) nonexistent
        FILE * f = fopen(db2_filename.c_str(), "rb");
        if (f)
        {
            char buf[100];
            snprintf(buf, 100,"(exist, but have %d fields instead " SIZEFMTD ") Wrong client version DBC file?", storage.GetFieldCount(), strlen(storage.GetFormat()));
            errlist.push_back(db2_filename + buf);
            fclose(f);
        }
        else
            errlist.push_back(db2_filename);
    }
}

void LoadDB2Stores(const std::string& dataPath)
{
    std::string db2Path = dataPath + "dbc/";
    
    StoreProblemList1 bad_db2_files;
    uint32 availableDb2Locales = 0xFFFFFFFF;

    LoadDB2(availableDb2Locales, bad_db2_files, sItemStore, db2Path, "Item.db2");
    LoadDB2(availableDb2Locales, bad_db2_files, sItemCurrencyCostStore, db2Path,"ItemCurrencyCost.db2");
    LoadDB2(availableDb2Locales, bad_db2_files, sItemExtendedCostStore, db2Path,"ItemExtendedCost.db2");
    LoadDB2(availableDb2Locales, bad_db2_files, sItemSparseStore, db2Path, "Item-sparse.db2");
    LoadDB2(availableDb2Locales, bad_db2_files, sKeyChainStore, db2Path, "KeyChain.db2");

    // error checks
    if (bad_db2_files.size() >= DB2FileCount)
    {
        sLog->outError("\nIncorrect DataDir value in worldserver.conf or ALL required *.db2 files (%d) not found by path: %sdb2", DB2FileCount, dataPath.c_str());
        exit(1);
    }
    else if (!bad_db2_files.empty())
    {
        std::string str;
        for (std::list<std::string>::iterator i = bad_db2_files.begin(); i != bad_db2_files.end(); ++i)
            str += *i + "\n";

        sLog->outError("\nSome required *.db2 files (%u from %d) not found or not compatible:\n%s", (uint32)bad_db2_files.size(), DB2FileCount,str.c_str());
        exit(1);
    }

    // Check loaded DBC files proper version
    if (!sItemStore.LookupEntry(79062)         ||            // last client known item added in 4.3.4
        !sItemExtendedCostStore.LookupEntry(3872))           // last item extended cost added in 4.3.4
    {
        sLog->outString(" ");
        sLog->outError("Please extract correct db2 files from client 4.0.6a 13623.");
        exit(1);
    }

    sLog->outString(">> Initialized %d data stores.", DB2FileCount);
    sLog->outString();

    sLog->outString("Loading custom DB2 entries...");

    // initialize

    // head
    ItemExtendedCostEntry* a = sItemExtendedCostStore.CreateEntry(78);
    *a = ItemExtendedCostEntry();
    a->ID = 78;
    a->RequiredCurrency[0] = 390;
    a->RequiredCurrencyCount[0] = 165000;

    // shoulders
    a = sItemExtendedCostStore.CreateEntry(79);
    *a = ItemExtendedCostEntry();
    a->ID = 79;
    a->RequiredCurrency[0] = 390;
    a->RequiredCurrencyCount[0] = 220000;

    // chest
    a = sItemExtendedCostStore.CreateEntry(80);
    *a = ItemExtendedCostEntry();
    a->ID = 80;
    a->RequiredCurrency[0] = 390;
    a->RequiredCurrencyCount[0] = 220000;

    // legs
    a = sItemExtendedCostStore.CreateEntry(81);
    *a = ItemExtendedCostEntry();
    a->ID = 81;
    a->RequiredCurrency[0] = 390;
    a->RequiredCurrencyCount[0] = 220000;

    // hands
    a = sItemExtendedCostStore.CreateEntry(82);
    *a = ItemExtendedCostEntry();
    a->ID = 82;
    a->RequiredCurrency[0] = 390;
    a->RequiredCurrencyCount[0] = 165000;

    sLog->outString(">> Custom entries successfully loaded.");
    sLog->outString();
}
