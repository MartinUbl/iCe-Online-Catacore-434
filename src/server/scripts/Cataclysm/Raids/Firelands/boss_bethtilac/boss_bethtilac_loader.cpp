/*
 * Copyright (C) 2006-2013 iCe Online <http://ice-wow.eu>
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

/*
 * loader of Beth'tilac's and her minions' AI
 * disable the calls to make the AI inactive
 */


#include "ScriptPCH.h"  // not needed, only to make compiler happy


namespace Bethtilac
{
    // forward declarations of script loaders
    void load_boss_Bethtilac();
    void load_npc_CinderwebDrone();
    void load_mob_Spiderling();
    void load_mob_Spinner();
    void load_beth_environment();
}


// load scripts

void AddSC_boss_bethtilac()
{
    using namespace Bethtilac;

    load_boss_Bethtilac();
    load_npc_CinderwebDrone();
    load_mob_Spiderling();
    load_mob_Spinner();
    load_beth_environment();
}
