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

#ifndef TRINITY_DB2STORES_H
#define TRINITY_DB2STORES_H

#include "Common.h"
#include "DB2Store.h"
#include "DB2Structure.h"

#include <list>

extern DB2Storage <ItemEntry> sItemStore;
extern DB2Storage <ItemCurrencyCostEntry> sItemCurrencyCostStore;
extern DB2Storage <ItemExtendedCostEntry> sItemExtendedCostStore;
extern DB2Storage <ItemSparseEntry> sItemSparseStore;
extern DB2Storage <KeyChainEntry> sKeyChainStore;

void LoadDB2Stores(const std::string& dataPath);

#endif
