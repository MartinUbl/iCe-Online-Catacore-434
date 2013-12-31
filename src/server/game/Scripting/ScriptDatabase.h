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

#ifndef SC_DATABASE_H
#define SC_DATABASE_H

// Database data structures
struct GameObjectTeleportCond
{
    //uint32 Id;
    uint64 Guid;
    uint32 Quest;
    uint8 Race;
    uint8 Class;
    uint8 Level;
    uint32 Item;
    uint32 Spell;
    float x,y,z,o;
    uint32 map;
};

struct CreaturePersonal
{
    //uint32 Id;
    uint32 Group;
};

class ScriptDatabaseMgr
{
public:
    ScriptDatabaseMgr() {};
    ~ScriptDatabaseMgr() {};

    void LoadDatabase();

    GameObjectTeleportCond* GetGOTeleCond(uint32 entry, uint64 guid = 0, uint8 race = 0, uint8 Class = 0)
    {
        for (GOTeleCondMap::const_iterator itr = m_GOTeleCond.begin(); itr != m_GOTeleCond.end(); ++itr)
        {
            if ((*itr).first == entry
                && ((*itr).second->Guid == guid || (*itr).second->Guid == 0)
                && ((*itr).second->Race == race || (*itr).second->Race == 0)
                && ((*itr).second->Class == Class || (*itr).second->Class == 0))
                return (*itr).second;
        }
        return NULL;
    }

    bool IsPersonalCreature(uint32 entry);

private:
    //Typedefs HERE !
    typedef UNORDERED_MAP<uint32, GameObjectTeleportCond*> GOTeleCondMap;
    typedef UNORDERED_MAP<uint32, CreaturePersonal*> CreaturePersonalMap;

    //Storages HERE !
    GOTeleCondMap m_GOTeleCond;
    CreaturePersonalMap m_personalCreatures;
};

#define sScriptDatabase ACE_Singleton<ScriptDatabaseMgr, ACE_Null_Mutex>::instance()

#endif
