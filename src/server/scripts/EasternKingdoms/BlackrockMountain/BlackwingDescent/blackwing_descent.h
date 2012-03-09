/*
 * Copyright (C) 2008-2010 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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


#ifndef DEF_BLACKWING_DESCENT_H
#define DEF_BLACKWING_DESCENT_H

enum SharedSpells
{
    SPELL_BERSERK1               = 26662,
    SPELL_BERSERK2               = 64238,
    // 10 Manka HC
};

enum Data
{
    //Encounters
    DATA_MAGMAW,
    DATA_OMNOTRON_DEFENSE_SYSTEM,
    DATA_MALORIAK,
    DATA_ATRAMEDES,
    DATA_CHIMAERON,
    DATA_NEFARIAN,
    //Additional Entities
    DATA_ARCANOTRON_GUID         = 6,
    DATA_ELECTRON_GUID           = 7,
    DATA_MAGMATRON_GUID          = 8,
    DATA_TOXITRON_GUID           = 9,
    DATA_ONYXIA_GUID             = 10,
    DATA_BILE_O_TRON_800         = 11,
    DATA_MALORIAK_GUID           = 41378,
    DATA_ATRAMEDES_GUID          = 41442,
    DATA_CHIMAERON_GUID          = 43296,
    DATA_MAGMAW_GUID            = 121050,
    DATA_DEFENSE_SYSTEM_GUID    = 121060,
    DATA_NEFARIAN1              = 41376,
    DATA_NEFARIAN2              = 41379
};

enum Creaturess
{
    NPC_MAGMAW               = 41570,
    NPC_ARCANOTRON           = 42166,
    NPC_ELECTRON             = 42179,
    NPC_MAGMATRON            = 42178,
    NPC_TOXITRON             = 42180,
    NPC_MALORIAK             = 41378,
    NPC_ATRAMEDES            = 41442,
    NPC_CHIMAERON            = 43296,
    NPC_BILE_O_TRON_800      = 44418,
    NPC_NEFARIAN             = 41376,
    NPC_ONYXIA               = 41270,
    NPC_LORD_VICTOR_NEFARIAN = 41379,
};

#endif
