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

#include "gamePCH.h"
#include "Common.h"
#include "PlayerDump.h"
#include "DatabaseEnv.h"
#include "SQLStorage.h"
#include "UpdateFields.h"
#include "ObjectMgr.h"
#include "AccountMgr.h"

#define DUMP_TABLE_COUNT 29

struct DumpTable
{
    char const* name;
    DumpTableType type;
};

static DumpTable dumpTables[DUMP_TABLE_COUNT] =
{
    { "characters",                       DTT_CHARACTER  },
    { "character_account_data",           DTT_CHAR_TABLE },
    { "character_achievement",            DTT_CHAR_TABLE },
    { "character_achievement_progress",   DTT_CHAR_TABLE },
    { "character_action",                 DTT_CHAR_TABLE },
    { "character_aura",                   DTT_CHAR_TABLE },
    { "character_branchspec",             DTT_CHAR_TABLE },
    { "character_currency",               DTT_CHAR_TABLE },
    { "character_declinedname",           DTT_CHAR_TABLE },
    { "character_equipmentsets",          DTT_EQSET_TABLE},
    { "character_gifts",                  DTT_ITEM_GIFT  },
    { "character_glyphs",                 DTT_CHAR_TABLE },
    { "character_homebind",               DTT_CHAR_TABLE },
    { "character_inventory",              DTT_INVENTORY  },
    { "character_pet",                    DTT_PET        },
    { "character_pet_declinedname",       DTT_PET        },
    { "character_queststatus",            DTT_CHAR_TABLE },
    { "character_reputation",             DTT_CHAR_TABLE },
    { "character_skills",                 DTT_CHAR_TABLE },
    { "character_spell",                  DTT_CHAR_TABLE },
    { "character_spell_cooldown",         DTT_CHAR_TABLE },
    { "character_talent",                 DTT_CHAR_TABLE },
    { "item_instance",                    DTT_ITEM       },
    { "mail",                             DTT_MAIL       },
    { "mail_items",                       DTT_MAIL_ITEM  },
    { "pet_aura",                         DTT_PET_TABLE  },
    { "pet_spell",                        DTT_PET_TABLE  },
    { "pet_spell_cooldown",               DTT_PET_TABLE  },
};

// Low level functions
static bool findtoknth(std::string &str, int n, std::string::size_type &s, std::string::size_type &e)
{
    int i; s = e = 0;
    std::string::size_type size = str.size();
    for (i = 1; s < size && i < n; s++) if (str[s] == ' ') ++i;
    if (i < n)
        return false;

    e = str.find(' ', s);

    return e != std::string::npos;
}

std::string gettoknth(std::string &str, int n)
{
    std::string::size_type s = 0, e = 0;
    if (!findtoknth(str, n, s, e))
        return "";

    return str.substr(s, e-s);
}

bool findnth(std::string &str, int n, std::string::size_type &s, std::string::size_type &e)
{
    s = str.find("VALUES ('")+9;
    if (s == std::string::npos) return false;

    do
    {
        e = str.find("'",s);
        if (e == std::string::npos) return false;
    } while (str[e-1] == '\\');

    for (int i = 1; i < n; ++i)
    {
        do
        {
            s = e+4;
            e = str.find("'",s);
            if (e == std::string::npos) return false;
        } while (str[e-1] == '\\');
    }
    return true;
}

std::string gettablename(std::string &str)
{
    std::string::size_type s = 13;
    std::string::size_type e = str.find(_TABLE_SIM_, s);
    if (e == std::string::npos)
        return "";

    return str.substr(s, e-s);
}

bool changenth(std::string &str, int n, const char *with, bool insert = false, bool nonzero = false)
{
    std::string::size_type s, e;
    if (!findnth(str,n,s,e))
        return false;

    if (nonzero && str.substr(s,e-s) == "0")
        return true;                                        // not an error
    if (!insert)
        str.replace(s,e-s, with);
    else
        str.insert(s, with);

    return true;
}

std::string getnth(std::string &str, int n)
{
    std::string::size_type s, e;
    if (!findnth(str,n,s,e))
        return "";

    return str.substr(s, e-s);
}

bool changetoknth(std::string &str, int n, const char *with, bool insert = false, bool nonzero = false)
{
    std::string::size_type s = 0, e = 0;
    if (!findtoknth(str, n, s, e))
        return false;
    if (nonzero && str.substr(s,e-s) == "0")
        return true;                                        // not an error
    if (!insert)
        str.replace(s, e-s, with);
    else
        str.insert(s, with);

    return true;
}

uint32 registerNewGuid(uint32 oldGuid, std::map<uint32, uint32> &guidMap, uint32 hiGuid)
{
    std::map<uint32, uint32>::const_iterator itr = guidMap.find(oldGuid);
    if (itr != guidMap.end())
        return itr->second;

    uint32 newguid = hiGuid + guidMap.size();
    guidMap[oldGuid] = newguid;
    return newguid;
}

bool changeGuid(std::string &str, int n, std::map<uint32, uint32> &guidMap, uint32 hiGuid, bool nonzero = false)
{
    char chritem[20];
    uint32 oldGuid = atoi(getnth(str, n).c_str());
    if (nonzero && oldGuid == 0)
        return true;                                        // not an error

    uint32 newGuid = registerNewGuid(oldGuid, guidMap, hiGuid);
    snprintf(chritem, 20, "%d", newGuid);

    return changenth(str, n, chritem, false, nonzero);
}

bool changetokGuid(std::string &str, int n, std::map<uint32, uint32> &guidMap, uint32 hiGuid, bool nonzero = false)
{
    char chritem[20];
    uint32 oldGuid = atoi(gettoknth(str, n).c_str());
    if (nonzero && oldGuid == 0)
        return true;                                        // not an error

    uint32 newGuid = registerNewGuid(oldGuid, guidMap, hiGuid);
    snprintf(chritem, 20, "%d", newGuid);

    return changetoknth(str, n, chritem, false, nonzero);
}

std::string CreateDumpString(char const* tableName, QueryResult result)
{
    if (!tableName || !result) return "";
    std::ostringstream ss;
    ss << "INSERT INTO "<< _TABLE_SIM_ << tableName << _TABLE_SIM_ << " VALUES (";
    Field *fields = result->Fetch();
    for (uint32 i = 0; i < result->GetFieldCount(); ++i)
    {
        if (i == 0) ss << "'";
        else ss << ", '";

        std::string s = fields[i].GetString();
        CharacterDatabase.escape_string(s);
        ss << s;

        ss << "'";
    }
    ss << ");";
    return ss.str();
}

std::string PlayerDumpWriter::GenerateWhereStr(char const* field, uint32 guid)
{
    std::ostringstream wherestr;
    wherestr << field << " = '" << guid << "'";
    return wherestr.str();
}

std::string PlayerDumpWriter::GenerateWhereStr(char const* field, GUIDs const& guids, GUIDs::const_iterator& itr)
{
    std::ostringstream wherestr;
    wherestr << field << " IN ('";
    for (; itr != guids.end(); ++itr)
    {
        wherestr << *itr;

        if (wherestr.str().size() > MAX_QUERY_LEN - 50)      // near to max query
        {
            ++itr;
            break;
        }

        GUIDs::const_iterator itr2 = itr;
        if (++itr2 != guids.end())
            wherestr << "','";
    }
    wherestr << "')";
    return wherestr.str();
}

void StoreGUID(QueryResult result,uint32 field,std::set<uint32>& guids)
{
    Field* fields = result->Fetch();
    uint32 guid = fields[field].GetUInt32();
    if (guid)
        guids.insert(guid);
}

void StoreGUID(QueryResult result,uint32 data,uint32 field, std::set<uint32>& guids)
{
    Field* fields = result->Fetch();
    std::string dataStr = fields[data].GetString();
    uint32 guid = atoi(gettoknth(dataStr, field).c_str());
    if (guid)
        guids.insert(guid);
}

// Writing - High-level functions
bool PlayerDumpWriter::DumpTable(std::string& dump, uint32 guid, char const*tableFrom, char const*tableTo, DumpTableType type)
{
    GUIDs const* guids = NULL;
    char const* fieldname = NULL;

    switch (type)
    {
        case DTT_ITEM:      fieldname = "guid";      guids = &items; break;
        case DTT_ITEM_GIFT: fieldname = "item_guid"; guids = &items; break;
        case DTT_PET:       fieldname = "owner";                     break;
        case DTT_PET_TABLE: fieldname = "guid";      guids = &pets;  break;
        case DTT_MAIL:      fieldname = "receiver";                  break;
        case DTT_MAIL_ITEM: fieldname = "mail_id";   guids = &mails; break;
        default:            fieldname = "guid";                      break;
    }

    // for guid set stop if set is empty
    if (guids && guids->empty())
        return true;                                        // nothing to do

    // setup for guids case start position
    GUIDs::const_iterator guids_itr;
    if (guids)
        guids_itr = guids->begin();

    do
    {
        std::string wherestr;

        if (guids)                                           // set case, get next guids string
            wherestr = GenerateWhereStr(fieldname,*guids,guids_itr);
        else                                                // not set case, get single guid string
            wherestr = GenerateWhereStr(fieldname,guid);

        QueryResult result = CharacterDatabase.PQuery("SELECT * FROM %s WHERE %s", tableFrom, wherestr.c_str());
        if (!result)
            return true;

        do
        {
            // collect guids
            switch (type)
            {
                case DTT_INVENTORY:
                    StoreGUID(result,3,items); break;       // item guid collection (character_inventory.item)
                case DTT_PET:
                    StoreGUID(result,0,pets);  break;       // pet petnumber collection (character_pet.id)
                case DTT_MAIL:
                    StoreGUID(result,0,mails);              // mail id collection (mail.id)
                case DTT_MAIL_ITEM:
                    StoreGUID(result,1,items); break;       // item guid collection (mail_items.item_guid)
                case DTT_CHARACTER:
                {
                    if (result->GetFieldCount() <= 72)      // avoid crashes on next check
                        return true;
                    if (result->Fetch()[72].GetUInt32())    // characters.deleteInfos_Account - if filled error
                        return false;
                    break;
                }
                default:                       break;
            }

            dump += CreateDumpString(tableTo, result);
            dump += "\n";
        }
        while (result->NextRow());
    }
    while (guids && guids_itr != guids->end());              // not set case iterate single time, set case iterate for all guids
    return true;
}

bool PlayerDumpWriter::GetDump(uint32 guid, std::string &dump)
{
    dump = "";

    dump += "IMPORTANT NOTE: THIS DUMPFILE IS MADE FOR USE WITH THE 'PDUMP' COMMAND ONLY - EITHER THROUGH INGAME CHAT OR ON CONSOLE!\n";
    dump += "IMPORTANT NOTE: DO NOT apply it directly - it will irreversibly DAMAGE and CORRUPT your database! You have been warned!\n\n";

    for (int i = 0; i < DUMP_TABLE_COUNT; ++i)
        if (!DumpTable(dump, guid, dumpTables[i].name, dumpTables[i].name, dumpTables[i].type))
            return false;

    // TODO: Add instance/group..
    // TODO: Add a dump level option to skip some non-important tables

    return true;
}

DumpReturn PlayerDumpWriter::WriteDump(const std::string& file, uint32 guid)
{
    FILE *fout = fopen(file.c_str(), "w");
    if (!fout)
        return DUMP_FILE_OPEN_ERROR;

    DumpReturn ret = DUMP_SUCCESS;
    std::string dump;
    if (!GetDump(guid, dump))
        ret = DUMP_CHARACTER_DELETED;

    fprintf(fout, "%s\n", dump.c_str());
    fclose(fout);
    return ret;
}

// Reading - High-level functions
#define ROLLBACK(DR) {fclose(fin); return (DR);}

void fixNULLfields(std::string &line)
{
    std::string nullString("'NULL'");
    size_t pos = line.find(nullString);
    while (pos != std::string::npos)
    {
        line.replace(pos, nullString.length(), "NULL");
        pos = line.find(nullString);
    }
}

DumpReturn PlayerDumpReader::LoadDump(const std::string& file, uint32 account, std::string name, uint32 guid)
{
    uint32 charcount = sAccountMgr->GetCharactersCount(account);
    if (charcount >= 10)
        return DUMP_TOO_MANY_CHARS;

    FILE *fin = fopen(file.c_str(), "r");
    if (!fin)
        return DUMP_FILE_OPEN_ERROR;

    QueryResult result = QueryResult(NULL);
    char newguid[20], chraccount[20], newpetid[20], currpetid[20], lastpetid[20];

    // make sure the same guid doesn't already exist and is safe to use
    bool incHighest = true;
    if (guid != 0 && guid < sObjectMgr->m_hiCharGuid)
    {
        result = CharacterDatabase.PQuery("SELECT 1 FROM characters WHERE guid = '%d'", guid);
        if (result)
            guid = sObjectMgr->m_hiCharGuid;                     // use first free if exists
        else incHighest = false;
    }
    else
        guid = sObjectMgr->m_hiCharGuid;

    // normalize the name if specified and check if it exists
    if (!normalizePlayerName(name))
        name = "";

    if (ObjectMgr::CheckPlayerName(name,true) == CHAR_NAME_SUCCESS)
    {
        CharacterDatabase.escape_string(name);              // for safe, we use name only for sql quearies anyway
        result = CharacterDatabase.PQuery("SELECT 1 FROM characters WHERE name = '%s'", name.c_str());
        if (result)
            name = "";                                      // use the one from the dump
    }
    else
        name = "";

    // name encoded or empty

    snprintf(newguid, 20, "%d", guid);
    snprintf(chraccount, 20, "%d", account);
    snprintf(newpetid, 20, "%d", sObjectMgr->GeneratePetNumber());
    snprintf(lastpetid, 20, "%s", "");

    std::map<uint32,uint32> items;
    std::map<uint32,uint32> mails;
    char buf[32000] = "";

    typedef std::map<uint32, uint32> PetIds;                // old->new petid relation
    typedef PetIds::value_type PetIdsPair;
    PetIds petids;

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    while (!feof(fin))
    {
        if (!fgets(buf, 32000, fin))
        {
            if (feof(fin)) break;
            ROLLBACK(DUMP_FILE_BROKEN);
        }

        std::string line; line.assign(buf);

        // skip empty strings
        size_t nw_pos = line.find_first_not_of(" \t\n\r\7");
        if (nw_pos == std::string::npos)
            continue;

        // skip logfile-side dump start notice, the important notes and dump end notices
        if ((line.substr(nw_pos,16) == "== START DUMP ==") ||
            (line.substr(nw_pos,15) == "IMPORTANT NOTE:") ||
            (line.substr(nw_pos,14) == "== END DUMP =="))
            continue;

        // add required_ check
        /*
        if (line.substr(nw_pos,41) == "UPDATE character_db_version SET required_")
        {
            if (!CharacterDatabase.Execute(line.c_str()))
                ROLLBACK(DUMP_FILE_BROKEN);

            continue;
        }
        */

        // determine table name and load type
        std::string tn = gettablename(line);
        if (tn.empty())
        {
            sLog->outError("LoadPlayerDump: Can't extract table name from line: '%s'!", line.c_str());
            ROLLBACK(DUMP_FILE_BROKEN);
        }
        if (strcmp(tn.c_str(),"character_aura") == 0)
            continue;
        if (strcmp(tn.c_str(),"character_glyphs") == 0)
            continue;
        if (strcmp(tn.c_str(),"character_talent") == 0)
            continue;
        if (strcmp(tn.c_str(),"character_action") == 0)
            continue;
        if (strcmp(tn.c_str(),"character_pet") == 0)
            continue;

        DumpTableType type = DumpTableType(0);
        uint8 i;
        for (i = 0; i < DUMP_TABLE_COUNT; ++i)
        {
            if (tn == dumpTables[i].name)
            {
                type = dumpTables[i].type;
                break;
            }
        }

        if (i == DUMP_TABLE_COUNT)
        {
            sLog->outError("LoadPlayerDump: Unknown table: '%s'!", tn.c_str());
            ROLLBACK(DUMP_FILE_BROKEN);
        }

        // change the data to server values
        switch(type)
        {
            case DTT_CHARACTER:
            {
                if (!changenth(line, 1, newguid))           // characters.guid update
                    ROLLBACK(DUMP_FILE_BROKEN);

                if (!changenth(line, 2, chraccount))        // characters.account update
                    ROLLBACK(DUMP_FILE_BROKEN);

                if (name == "")
                {
                    // check if the original name already exists
                    name = getnth(line, 3);
                    CharacterDatabase.escape_string(name);

                    result = CharacterDatabase.PQuery("SELECT 1 FROM characters WHERE name = '%s'", name.c_str());
                    if (result)
                    {
                        if (!changenth(line, 37, "1"))       // characters.at_login set to "rename on login"
                            ROLLBACK(DUMP_FILE_BROKEN);
                    }
                }
                else if (!changenth(line, 3, name.c_str())) // characters.name
                    ROLLBACK(DUMP_FILE_BROKEN);

                const char null[5] = "NULL";
                if (!changenth(line, 71, null))             // characters.deleteInfos_Account
                    ROLLBACK(DUMP_FILE_BROKEN);
                if (!changenth(line, 72, null))             // characters.deleteInfos_Name
                    ROLLBACK(DUMP_FILE_BROKEN);
                if (!changenth(line, 73, null))             // characters.deleteDate
                    ROLLBACK(DUMP_FILE_BROKEN);
                break;
            }
            case DTT_CHAR_TABLE:
            {
                if (!changenth(line, 1, newguid))           // character_*.guid update
                    ROLLBACK(DUMP_FILE_BROKEN);
                break;
            }
            case DTT_EQSET_TABLE:
            {
                if (!changenth(line, 1, newguid))
                    ROLLBACK(DUMP_FILE_BROKEN);             // character_equipmentsets.guid

                char newSetGuid[24];
                snprintf(newSetGuid, 24, UI64FMTD, sObjectMgr->GenerateEquipmentSetGuid());
                if (!changenth(line, 2, newSetGuid))
                    ROLLBACK(DUMP_FILE_BROKEN);             // character_equipmentsets.setguid
                break;
            }
            case DTT_INVENTORY:
            {
                if (!changenth(line, 1, newguid))           // character_inventory.guid update
                    ROLLBACK(DUMP_FILE_BROKEN);

                if (!changeGuid(line, 2, items, sObjectMgr->m_hiItemGuid, true))
                    ROLLBACK(DUMP_FILE_BROKEN);             // character_inventory.bag update
                if (!changeGuid(line, 4, items, sObjectMgr->m_hiItemGuid))
                    ROLLBACK(DUMP_FILE_BROKEN);             // character_inventory.item update
                break;
            }
            case DTT_MAIL:                                  // mail
            {
                if (!changeGuid(line, 1, mails, sObjectMgr->m_mailid))
                    ROLLBACK(DUMP_FILE_BROKEN);             // mail.id update
                if (!changenth(line, 6, newguid))           // mail.receiver update
                    ROLLBACK(DUMP_FILE_BROKEN);
                break;
            }
            case DTT_MAIL_ITEM:                             // mail_items
            {
                if (!changeGuid(line, 1, mails, sObjectMgr->m_mailid))
                    ROLLBACK(DUMP_FILE_BROKEN);             // mail_items.id
                if (!changeGuid(line, 2, items, sObjectMgr->m_hiItemGuid))
                    ROLLBACK(DUMP_FILE_BROKEN);             // mail_items.item_guid
                if (!changenth(line, 3, newguid))           // mail_items.receiver
                    ROLLBACK(DUMP_FILE_BROKEN);
                break;
            }
            case DTT_ITEM:
            {
                // item, owner, data field:item, owner guid
                if (!changeGuid(line, 1, items, sObjectMgr->m_hiItemGuid))
                   ROLLBACK(DUMP_FILE_BROKEN);              // item_instance.guid update
                if (!changenth(line, 3, newguid))           // item_instance.owner_guid update
                    ROLLBACK(DUMP_FILE_BROKEN);
                break;
            }
            case DTT_ITEM_GIFT:
            {
                if (!changenth(line, 1, newguid))           // character_gifts.guid update
                    ROLLBACK(DUMP_FILE_BROKEN);
                if (!changeGuid(line, 2, items, sObjectMgr->m_hiItemGuid))
                    ROLLBACK(DUMP_FILE_BROKEN);             // character_gifts.item_guid update
                break;
            }
            case DTT_PET:
            {
                //store a map of old pet id to new inserted pet id for use by type 5 tables
                snprintf(currpetid, 20, "%s", getnth(line, 1).c_str());
                if (*lastpetid == '\0')
                    snprintf(lastpetid, 20, "%s", currpetid);
                if (strcmp(lastpetid,currpetid) != 0)
                {
                    snprintf(newpetid, 20, "%d", sObjectMgr->GeneratePetNumber());
                    snprintf(lastpetid, 20, "%s", currpetid);
                }

                std::map<uint32, uint32> :: const_iterator petids_iter = petids.find(atoi(currpetid));

                if (petids_iter == petids.end())
                {
                    petids.insert(PetIdsPair(atoi(currpetid), atoi(newpetid)));
                }

                if (!changenth(line, 1, newpetid))          // character_pet.id update
                    ROLLBACK(DUMP_FILE_BROKEN);
                if (!changenth(line, 3, newguid))           // character_pet.owner update
                    ROLLBACK(DUMP_FILE_BROKEN);

                break;
            }
            case DTT_PET_TABLE:                             // pet_aura, pet_spell, pet_spell_cooldown
            {
                snprintf(currpetid, 20, "%s", getnth(line, 1).c_str());

                // lookup currpetid and match to new inserted pet id
                std::map<uint32, uint32> :: const_iterator petids_iter = petids.find(atoi(currpetid));
                if (petids_iter == petids.end())             // couldn't find new inserted id
                    ROLLBACK(DUMP_FILE_BROKEN);

                snprintf(newpetid, 20, "%d", petids_iter->second);

                if (!changenth(line, 1, newpetid))
                    ROLLBACK(DUMP_FILE_BROKEN);

                break;
            }
            default:
                sLog->outError("Unknown dump table type: %u",type);
                break;
        }

        fixNULLfields(line);

        trans->Append(line.c_str());
    }

    char delst[10000];
    sprintf(delst,"DELETE FROM character_spell WHERE guid = %u AND spell NOT IN (48778, 60025, 43688, 16056, 66906, 63844, 67466, 66907, 61230, 60116, 60114, \
                    61229, 40192, 59567, 41514, 51412, 58953, 71342, 22719, 59650, 35022, 16055, 59976, 26656, 17461, 64977, 470  , 60118, \
                    60119, 48027, 22718, 59788, 59785, 22720, 22721, 22717, 22723, 22724, 64658, 74856, 72808, 61996, 59568, 35020, 10969, \
                    59996, 25953, 39803, 17463, 64656, 32244, 50869, 43899, 59569, 34406, 458  , 18990, 6899 , 17464, 6654 , 58615, 75614, \
                    43927, 6648 , 41515, 39315, 34896, 73313, 68188, 68187, 39316, 34790, 63635, 63637, 64927, 6653 , 23161, 32239, 8395 , \
                    63639, 5784 , 36702, 61451, 44153, 63643, 17460, 23509, 75596, 65439, 63638, 32235, 61467, 61465, 61469, 61470, 35710, \
                    18989, 6777 , 35713, 49379, 23249, 65641, 23248, 35712, 35714, 65637, 23247, 18991, 17453, 61294, 26056, 39798, 17465, \
                    32245, 48025, 59797, 59799, 72807, 17459, 72286, 63956, 63636, 17450, 65917, 61309, 55531, 60424, 44744, 63796, 16084, \
                    66846, 41513, 69395, 63640, 16082, 32345, 472  , 60021, 35711, 53018, 41516, 39801, 23246, 66090, 41252, 61997, 59570, \
                    34795, 10873, 59961, 26054, 39800, 17462, 22722, 16080, 67336, 30174, 17481, 63963, 64731, 66087, 39802, 39317, 34898, \
                    63642, 32240, 42776, 10789, 23510, 63232, 66847, 8394 , 10793, 23214, 34767, 13819, 34769, 66088, 66091, 68057, 32242, \
                    23241, 43900, 23238, 23229, 23250, 65646, 23221, 23239, 65640, 23252, 32290, 35025, 23225, 32295, 68056, 23219, 65638, \
                    37015, 23242, 23243, 23227, 33660, 32292, 35027, 65644, 32297, 24242, 32289, 65639, 32246, 42777, 23338, 23251, 65643, \
                    35028, 46628, 23223, 23240, 23228, 23222, 32296, 49322, 24252, 39318, 34899, 32243, 18992, 63641, 580  , 60002, 61425, \
                    61447, 44151, 65642, 10796, 59571, 17454, 49193, 64659, 41517, 41518, 60024, 10799, 64657, 15779, 54753, 6898 , 39319, \
                    65645, 16083, 34897, 54729, 16081, 17229, 59791, 59793, 74918, 71810, 46197, 46199, 75973, 26055,\
                    10713, 62562, 10685, 62746, 62609, 10696, 61855, 40549, 10714, 10675, 75134, 36031, 35907, 10673, 10709, 35239, 10716, \
                    65358, 75613, 46426, 54187, 61351, 10680, 10688, 69452, 10674, 10717, 10697, 65381, 65382, 10695, 67413, 94070, 67414, \
                    25162, 45127, 62508, 62513, 40614, 62516, 10698, 62564, 48408, 49964, 26533, 36034, 74932, 52615, 53316, 59250, 36027, \
                    45174, 10707, 10683, 66030, 69535, 67415, 27241, 10706, 30156, 10682, 66520, 23811, 61472, 67416, 19772, 69677, 95787, \
                    15049, 75906, 61991, 40405, 24988, 33050, 35156, 12243, 4055 , 62674, 17708, 78381, 53082, 39181, 43918, 95786, 95909, \
                    55068, 28739, 43698, 62542, 63318, 75936, 24696, 51716, 32298, 67417, 69002, 10676, 17707, 69541, 40634, 27570, 61357, \
                    70613, 15048, 46599, 44369, 61773, 28505, 61350, 67418, 67419, 67420, 36028, 35909, 45125, 89929, 45890, 63712, 10684, \
                    66096, 10677, 36029, 45175, 10678, 42609, 16450, 46425, 10711, 68810, 28738, 48406, 28871, 61725, 15067, 40990, 62561, \
                    89930, 62491, 61348, 23531, 23530, 26045, 45082, 62510, 43697, 71840, 26010, 10704, 68767, 51851, 65682, 13548, 89931, \
                    28740, 10679, 35911, 61349, 40613, 69536, 26529, 26541, 39709, 10703, 15999, 64351, 35910, 17709,\
                    691, 688, 712, 697, 982, 136, 6991, 2641,\
                    668, 671, 814, 29932, 672, 69270, 7340, 69269, 17737, 669, 670, 813, 816, 7341,\
                    107, 264, 5011, 1180, 204, 81, 674, 15590, 266, 196, 198, 201, 200, 3018, 5019, 227, 2764, 2567, 197, 199, 202, 203, \
                    5009,\
                    9078, 9077, 8737, 750, 52665, 27764, 27762, 27763, 9116,\
                    2259, 2018, 4036, 13262, 7411, 2383, 2366, 45357, 25229, 2108, 2580, 2575, 2656, 8613, 3908, 51005, 31252, 3101, 3100, \
                    4037, 7412, 2368, 45358, 25230, 3104, 2576, 8617, 3909, 3570, 11993, 28695, 50300, 74519, 3564, 10248, 29354, 50310, \
                    74517, 7413, 13920, 28029, 51313, 74258, 3464, 11611, 28596, 51304, 60893, 80731, 3538, 9785, 29844, 51300, 76666, 4038, \
                    12656, 30350, 51306, 49383, 61288, 45359, 45360, 45361, 45363, 28894, 28895, 28897, 51311, 3811, 10662, 32549, 51302, \
                    8613, 8617, 8618, 10768, 32678, 50305, 3910, 12180, 26790, 51309, 2550, 3102, 3413, 18260, 33359, 51296, 3273, 3274, \
                    7924, 10846, 27028, 45542, 3275, 3276, 3277, 3278, 7928, 7929, 10840, 10841, 18629, 18630, 27032, 27033, 45545, 45546, \
                    818, 7620, 7731, 43308, 7732, 18248, 33095, 51294, 33388, 33391, 34090, 34091, 54197);",newguid);

    trans->Append(delst);

    char flagst[500];
    sprintf(flagst,"UPDATE characters SET at_login=256,xp=0 WHERE guid = %u;",newguid);

    trans->Append(flagst);

    trans->Append("DELETE FROM character_inventory WHERE bag = 0 AND slot BETWEEN 118 AND 135");

    CharacterDatabase.CommitTransaction(trans);

    sObjectMgr->m_hiItemGuid += items.size();
    sObjectMgr->m_mailid     += mails.size();

    if (incHighest)
        ++sObjectMgr->m_hiCharGuid;

    fclose(fin);

    return DUMP_SUCCESS;
}

