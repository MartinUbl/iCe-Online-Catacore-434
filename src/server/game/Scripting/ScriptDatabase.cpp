/*
 * Copyright (C) 2010-2011 iCe Online <http://www.ice-wow.eu/>
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
#include "ScriptPCH.h"
#include "ScriptSystem.h"
#include "ScriptDatabase.h"

#include "ObjectMgr.h"
#include "DatabaseEnv.h"

void ScriptDatabaseMgr::LoadDatabase()
{
    // Clear all storages for case of reload
    m_GOTeleCond.clear();
    // Cleared

    sLog->outString("");

    // Load our storages
    sLog->outString("Loading Gameobject Teleport Condition...");
    QueryResult res = ScriptDatabase.PQuery("SELECT * FROM gameobject_teleport_cond;");
    if (res)
    {
        Field* pField = NULL;
        uint32 count = 0;
        for (int i = 0; i < res->GetRowCount(); i++)
        {
            pField = res->Fetch();

            GameObjectTeleportCond* pTemp = new GameObjectTeleportCond;
            pTemp->Guid = pField[1].GetUInt64();
            pTemp->Quest = pField[2].GetUInt32();
            pTemp->Race = pField[3].GetUInt8();
            pTemp->Class = pField[4].GetUInt8();
            pTemp->Level = pField[5].GetUInt8();
            pTemp->Item = pField[6].GetUInt32();
            pTemp->Spell = pField[7].GetUInt32();
            pTemp->x = pField[8].GetFloat();
            pTemp->y = pField[9].GetFloat();
            pTemp->z = pField[10].GetFloat();
            pTemp->o = pField[11].GetFloat();
            pTemp->map = pField[12].GetUInt32();

            m_GOTeleCond[pField[0].GetUInt32()] = pTemp;
            ++count;

            res->NextRow();
        }
        sLog->outString("Loaded %u Gameobject Teleport Condition(s)",count);
    }
    else
        sLog->outString("Loaded 0 Gameobject Teleport Condition(s), table is empty or doesn't exist!");

    // Finished loading

    sLog->outString("");
}
