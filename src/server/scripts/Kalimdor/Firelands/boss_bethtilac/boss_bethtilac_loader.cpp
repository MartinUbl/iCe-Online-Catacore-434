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
#include "boss_bethtilac.h"
#include "boss_bethtilac_drone.h"
#include "boss_bethtilac_spiderling.h"
#include "boss_bethtilac_spinner.h"


// load scripts

void AddSC_boss_bethtilac()
{
    new boss_bethtilac();
    new mob_drone();
    new mob_spiderling();
    new mob_spinner();
}
