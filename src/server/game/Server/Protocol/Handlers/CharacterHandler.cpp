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
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "SystemConfig.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "DatabaseEnv.h"

#include "ArenaTeam.h"
#include "Chat.h"
#include "Group.h"
#include "Guild.h"
#include "Language.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "PlayerDump.h"
#include "SharedDefines.h"
#include "SocialMgr.h"
#include "UpdateMask.h"
#include "Util.h"
#include "ScriptMgr.h"
#include "Battleground.h"
#include "GuildFinderMgr.h"
#include "CalendarMgr.h"

class LoginQueryHolder : public SQLQueryHolder
{
    private:
        uint32 m_accountId;
        uint64 m_guid;
    public:
        LoginQueryHolder(uint32 accountId, uint64 guid)
            : m_accountId(accountId), m_guid(guid) { }
        uint64 GetGuid() const { return m_guid; }
        uint32 GetAccountId() const { return m_accountId; }
        bool Initialize();
};

bool LoginQueryHolder::Initialize()
{
    SetSize(MAX_PLAYER_LOGIN_QUERY);

    bool res = true;
    uint32 lowGuid = GUID_LOPART(m_guid);
    PreparedStatement* stmt = NULL;

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADFROM, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_GROUP);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADGROUP, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_BOUNDINSTANCES);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADBOUNDINSTANCES, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_AURAS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADAURAS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_SPELLS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADSPELLS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_QUESTSTATUS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADQUESTSTATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_DAILYQUESTSTATUS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADDAILYQUESTSTATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_WEKLYQUESTSTATUS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADWEKLYQUESTSTATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_REPUTATION);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADREPUTATION, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_INVENTORY);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADINVENTORY, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_VOID_STORAGE);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADVOIDSTORAGE, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_ACTIONS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADACTIONS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_MAILCOUNT);
    stmt->setUInt32(0, lowGuid);
    stmt->setUInt64(1, uint64(time(NULL)));
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADMAILCOUNT, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_MAILDATE);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADMAILDATE, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_SOCIALLIST);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADSOCIALLIST, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_HOMEBIND);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADHOMEBIND, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_SPELLCOOLDOWNS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADSPELLCOOLDOWNS, stmt);

    if (sWorld->getBoolConfig(CONFIG_DECLINED_NAMES_USED))
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_DECLINEDNAMES);
        stmt->setUInt32(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADDECLINEDNAMES, stmt);
    }

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_GUILD);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADGUILD, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_ARENAINFO);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADARENAINFO, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_ACHIEVEMENTS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADACHIEVEMENTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_CRITERIAPROGRESS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADCRITERIAPROGRESS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_EQUIPMENTSETS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADEQUIPMENTSETS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_BGDATA);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADBGDATA, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_GLYPHS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADGLYPHS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_TALENTBRANCHSPECS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADTALENTBRANCHSPECS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PET_SLOTS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADPETSLOT, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_CURRENCY);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CURRENCY, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_CURRENCY_WEEKCAP);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CURRENCY_WEEKCAP, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_TALENTS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADTALENTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_ACCOUNTDATA);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADACCOUNTDATA, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_SKILLS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADSKILLS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_RANDOMBG);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADRANDOMBG, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_ARENASTATS);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADARENASTATS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_PLAYER_BANNED);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOADBANNED, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_CUF_PROFILES);
    stmt->setUInt32(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CUF_PROFILES, stmt);

    return res;
}

void WorldSession::HandleCharEnum(QueryResult result)
{
    uint32 charCount = 0;
    ByteBuffer bitBuffer;
    ByteBuffer dataBuffer;

    bitBuffer.WriteBits(0, 23);
    bitBuffer.WriteBit(1);
    if (result)
    {
        _allowedCharsToLogin.clear();

        charCount = uint32(result->GetRowCount());
        bitBuffer.reserve(24 * charCount / 8);
        dataBuffer.reserve(charCount * 381);

        bitBuffer.WriteBits(charCount, 17);

        do
        {
            uint32 guidLow = (*result)[0].GetUInt32();

            sLog->outDetail("Loading char guid %u from account %u.", guidLow, GetAccountId());

            Player::BuildEnumData(result, &dataBuffer, &bitBuffer);

            _allowedCharsToLogin.insert(guidLow);
        } while (result->NextRow());

        bitBuffer.FlushBits();
    }
    else
        bitBuffer.WriteBits(0, 17);

    WorldPacket data(SMSG_CHAR_ENUM, 7 + bitBuffer.size() + dataBuffer.size());
    data.append(bitBuffer);
    if (charCount)
        data.append(dataBuffer);

    SendPacket(&data);
}

void WorldSession::HandleCharEnumOpcode(WorldPacket & /*recv_data*/)
{
    // remove expired bans
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_EXPIRED_BANS);
    CharacterDatabase.Execute(stmt);

    /// get all the data necessary for loading all characters (along with their pets) on the account
    m_charEnumCallback =
        CharacterDatabase.AsyncPQuery(
             !sWorld->getBoolConfig(CONFIG_DECLINED_NAMES_USED) ?
            //   ------- Query Without Declined Names --------
            //           0               1                2                3                 4                  5                       6                        7
                "SELECT characters.guid, characters.name, characters.race, characters.class, characters.gender, characters.playerBytes, characters.playerBytes2, characters.level, "
            //   8                9               10                     11                     12                     13                    14
                "characters.zone, characters.map, characters.position_x, characters.position_y, characters.position_z, guild_member.guildid, characters.playerFlags, "
            //  15                    16                         17                     18                         19
                "characters.at_login, characters.equipmentCache, character_banned.guid, characters.currentPetSlot, characters.charSlot "
                "FROM characters "
                "LEFT JOIN guild_member ON characters.guid = guild_member.guid "
                "LEFT JOIN character_banned ON characters.guid = character_banned.guid AND character_banned.active = 1 "
                "WHERE characters.account = '%u'"
                :
            //   --------- Query With Declined Names ---------
            //           0               1                2                3                 4                  5                       6                        7
                "SELECT characters.guid, characters.name, characters.race, characters.class, characters.gender, characters.playerBytes, characters.playerBytes2, characters.level, "
            //   8                9               10                     11                     12                     13                    14
                "characters.zone, characters.map, characters.position_x, characters.position_y, characters.position_z, guild_member.guildid, characters.playerFlags, "
            //  15                    16                         17                     18                         19                     20
                "characters.at_login, characters.equipmentCache, character_banned.guid, characters.currentPetSlot, characters.charSlot , character_declinedname.genitive "
                "FROM characters "
                "LEFT JOIN character_declinedname ON characters.guid = character_declinedname.guid "
                "LEFT JOIN guild_member ON characters.guid = guild_member.guid "
                "LEFT JOIN character_banned ON characters.guid = character_banned.guid AND character_banned.active = 1 "
                "WHERE characters.account = '%u'",
                GetAccountId()
            );
}

void WorldSession::HandleCharCreateOpcode(WorldPacket & recv_data)
{
    std::string name;
    uint8 race_,class_;

    recv_data >> name;

    recv_data >> race_;
    recv_data >> class_;

    WorldPacket data(SMSG_CHAR_CREATE, 1);                  // returned with diff.values in all cases

    if (GetSecurity() == SEC_PLAYER)
    {
        if (uint32 mask = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED))
        {
            bool disabled = false;

            uint32 team = Player::TeamForRace(race_);
            switch(team)
            {
                case ALLIANCE:
                    disabled = mask & (1<<0);
                    break;
                case HORDE:
                    disabled = mask & (1<<1);
                    break;
            }

            if (disabled)
            {
                data << (uint8)CHAR_CREATE_DISABLED;
                SendPacket(&data);
                return;
            }
        }
    }

    ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(class_);
    ChrRacesEntry const* raceEntry = sChrRacesStore.LookupEntry(race_);

    if (!classEntry || !raceEntry)
    {
        data << (uint8)CHAR_CREATE_FAILED;
        SendPacket(&data);
        sLog->outError("Class: %u or Race %u not found in DBC (Wrong DBC files?) or Cheater?", class_, race_);
        return;
    }

    // prevent character creating Expansion race without Expansion account
    if (raceEntry->expansion > Expansion())
    {
        data << (uint8)CHAR_CREATE_EXPANSION;
        sLog->outError("Expansion %u account:[%d] tried to Create character with expansion %u race (%u)",Expansion(),GetAccountId(),raceEntry->expansion,race_);
        SendPacket(&data);
        return;
    }

    // prevent character creating Expansion class without Expansion account
    if (classEntry->expansion > Expansion())
    {
        data << (uint8)CHAR_CREATE_EXPANSION_CLASS;
        sLog->outError("Expansion %u account:[%d] tried to Create character with expansion %u class (%u)",Expansion(),GetAccountId(),classEntry->expansion,class_);
        SendPacket(&data);
        return;
    }

    if (GetSecurity() == SEC_PLAYER)
    {
        uint32 raceMaskDisabled = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED_RACEMASK);
        if ((1 << (race_ - 1)) & raceMaskDisabled)
        {
            data << uint8(CHAR_CREATE_DISABLED);
            SendPacket(&data);
            return;
        }

        uint32 classMaskDisabled = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED_CLASSMASK);
        if ((1 << (class_ - 1)) & classMaskDisabled)
        {
            data << uint8(CHAR_CREATE_DISABLED);
            SendPacket(&data);
            return;
        }
    }

    // prevent character creating with invalid name
    if (!normalizePlayerName(name))
    {
        data << (uint8)CHAR_NAME_NO_NAME;
        SendPacket(&data);
        sLog->outError("Account:[%d] but tried to Create character with empty [name] ",GetAccountId());
        return;
    }

    // check name limitations
    uint8 res = ObjectMgr::CheckPlayerName(name,true);
    if (res != CHAR_NAME_SUCCESS)
    {
        data << uint8(res);
        SendPacket(&data);
        return;
    }

    if (GetSecurity() == SEC_PLAYER && sObjectMgr->IsReservedName(name))
    {
        data << (uint8)CHAR_NAME_RESERVED;
        SendPacket(&data);
        return;
    }

    if (sObjectMgr->GetPlayerGUIDByName(name))
    {
        data << (uint8)CHAR_CREATE_NAME_IN_USE;
        SendPacket(&data);
        return;
    }

    QueryResult resultacct = LoginDatabase.PQuery("SELECT IFNULL(SUM(numchars), 0) FROM realmcharacters WHERE acctid = '%d'", GetAccountId());
    if (resultacct)
    {
        Field *fields = resultacct->Fetch();
        uint32 acctcharcount = fields[0].GetUInt32();

        if (acctcharcount >= sWorld->getIntConfig(CONFIG_CHARACTERS_PER_ACCOUNT))
        {
            data << (uint8)CHAR_CREATE_ACCOUNT_LIMIT;
            SendPacket(&data);
            return;
        }
    }

    QueryResult petquery = CharacterDatabase.PQuery("SELECT id FROM character_pet WHERE id != 0 ORDER BY id DESC LIMIT 1");
	QueryResult result = CharacterDatabase.PQuery("SELECT COUNT(guid) FROM characters WHERE account = '%d'", GetAccountId());
    uint8 charcount = 0;
    if (result)
    {
        Field *fields=result->Fetch();
        charcount = fields[0].GetUInt8();

        if (charcount >= sWorld->getIntConfig(CONFIG_CHARACTERS_PER_REALM))
        {
            data << (uint8)CHAR_CREATE_SERVER_LIMIT;
            SendPacket(&data);
            return;
        }
    }

    // speedup check for heroic class disabled case
    uint32 heroic_free_slots = sWorld->getIntConfig(CONFIG_HEROIC_CHARACTERS_PER_REALM);
    if (heroic_free_slots == 0 && GetSecurity() == SEC_PLAYER && class_ == CLASS_DEATH_KNIGHT)
    {
        data << (uint8)CHAR_CREATE_UNIQUE_CLASS_LIMIT;
        SendPacket(&data);
        return;
    }

    // speedup check for heroic class disabled case
    uint32 req_level_for_heroic = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_MIN_LEVEL_FOR_HEROIC_CHARACTER);
    if (GetSecurity() == SEC_PLAYER && class_ == CLASS_DEATH_KNIGHT && req_level_for_heroic > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
    {
        data << (uint8)CHAR_CREATE_LEVEL_REQUIREMENT;
        SendPacket(&data);
        return;
    }

    bool AllowTwoSideAccounts = !sWorld->IsPvPRealm() || sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_ACCOUNTS) || GetSecurity() > SEC_PLAYER;
    uint32 skipCinematics = sWorld->getIntConfig(CONFIG_SKIP_CINEMATICS);

    bool have_same_race = false;

    // if 0 then allowed creating without any characters
    bool have_req_level_for_heroic = (req_level_for_heroic == 0);

    if (!AllowTwoSideAccounts || skipCinematics == 1 || class_ == CLASS_DEATH_KNIGHT)
    {
        QueryResult result2 = CharacterDatabase.PQuery("SELECT level,race,class FROM characters WHERE account = '%u' %s",
            GetAccountId(), (skipCinematics == 1 || class_ == CLASS_DEATH_KNIGHT) ? "" : "LIMIT 1");
        if (result2)
        {
            uint32 team_= Player::TeamForRace(race_);

            Field* field = result2->Fetch();
            uint8 acc_race  = field[1].GetUInt32();

            if (GetSecurity() == SEC_PLAYER && class_ == CLASS_DEATH_KNIGHT)
            {
                uint8 acc_class = field[2].GetUInt32();
                if (acc_class == CLASS_DEATH_KNIGHT)
                {
                    if (heroic_free_slots > 0)
                        --heroic_free_slots;

                    if (heroic_free_slots == 0)
                    {
                        data << (uint8)CHAR_CREATE_UNIQUE_CLASS_LIMIT;
                        SendPacket(&data);
                        return;
                    }
                }

                if (!have_req_level_for_heroic)
                {
                    uint32 acc_level = field[0].GetUInt32();
                    if (acc_level >= req_level_for_heroic)
                        have_req_level_for_heroic = true;
                }
            }

            // need to check team only for first character
            // TODO: what to if account already has characters of both races?
            if (!AllowTwoSideAccounts)
            {
                uint32 acc_team=0;
                if (acc_race > 0)
                    acc_team = Player::TeamForRace(acc_race);

                if (acc_team != team_)
                {
                    data << (uint8)CHAR_CREATE_PVP_TEAMS_VIOLATION;
                    SendPacket(&data);
                    return;
                }
            }

            // search same race for cinematic or same class if need
            // TODO: check if cinematic already shown? (already logged in?; cinematic field)
            while ((skipCinematics == 1 && !have_same_race) || class_ == CLASS_DEATH_KNIGHT)
            {
                if (!result2->NextRow())
                    break;

                field = result2->Fetch();
                acc_race = field[1].GetUInt32();

                if (!have_same_race)
                    have_same_race = race_ == acc_race;

                if (GetSecurity() == SEC_PLAYER && class_ == CLASS_DEATH_KNIGHT)
                {
                    uint8 acc_class = field[2].GetUInt32();
                    if (acc_class == CLASS_DEATH_KNIGHT)
                    {
                        if (heroic_free_slots > 0)
                            --heroic_free_slots;

                        if (heroic_free_slots == 0)
                        {
                            data << (uint8)CHAR_CREATE_UNIQUE_CLASS_LIMIT;
                            SendPacket(&data);
                            return;
                        }
                    }

                    if (!have_req_level_for_heroic)
                    {
                        uint32 acc_level = field[0].GetUInt32();
                        if (acc_level >= req_level_for_heroic)
                            have_req_level_for_heroic = true;
                    }
                }
            }
        }
    }

    if (GetSecurity() == SEC_PLAYER && class_ == CLASS_DEATH_KNIGHT && !have_req_level_for_heroic)
    {
        data << (uint8)CHAR_CREATE_LEVEL_REQUIREMENT;
        SendPacket(&data);
        return;
    }

    // extract other data required for player creating
    uint8 gender, skin, face, hairStyle, hairColor, facialHair, outfitId;
    recv_data >> gender >> skin >> face;
    recv_data >> hairStyle >> hairColor >> facialHair >> outfitId;

    if (recv_data.rpos() < recv_data.wpos())
    {
        uint8 unk;
        recv_data >> unk;
        sLog->outDebug("Character creation %s (account %u) has unhandled tail data: [%u]", name.c_str(), GetAccountId(), unk);
    }

    Player* pNewChar = new Player(this);

    ASSERT(pNewChar);

    pNewChar->setClass(class_);

    if (!pNewChar->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_PLAYER), name, race_, class_, gender, skin, face, hairStyle, hairColor, facialHair, outfitId))
    {
        // Player not create (race/class problem?)
        pNewChar->CleanupsBeforeDelete();
        delete pNewChar;

        data << (uint8)CHAR_CREATE_ERROR;
        SendPacket(&data);

        return;
    }

    if ((have_same_race && skipCinematics == 1) || skipCinematics == 2)
        pNewChar->setCinematic(1);                          // not show intro

    pNewChar->SetAtLoginFlag(AT_LOGIN_FIRST);               // First login

    // Player created, save it now by special method for creating (INSERT instead of UPDATE)
    pNewChar->CreateInDB();
    charcount+=1;

    LoginDatabase.PExecute("REPLACE INTO realmcharacters (numchars, acctid, realmid) VALUES (%u, %u, %u)",  charcount, GetAccountId(), realmID);

    uint32 pet_id = sObjectMgr->GeneratePetNumber();

    if (class_ == CLASS_WARLOCK)
    {
        const int petEntry = 416;
        CharacterDatabase.PExecute("REPLACE INTO character_pet (`id`, `entry`, `owner`, `modelid`, `CreatedBySpell`, `PetType`, `level`, `exp`, `Reactstate`, `name`, `renamed`, `slot`, `curhealth`, `curmana`, `curhappiness`, `savetime`, `resettalents_cost`, `resettalents_time`, `abdata`) VALUES (%u, %d, %u, 4449, 0, 0, 1, 0, 0, '%s', 1, 0, 282, 72, 0, 1295721046, 0, 0, '7 2 7 1 7 0 129 3110 1 0 1 0 1 0 6 2 6 1 6 0 ')", pet_id, petEntry, pNewChar->GetGUIDLow(), sObjectMgr->GeneratePetName(petEntry).c_str());
        CharacterDatabase.PExecute("UPDATE characters SET currentPetSlot = '0' WHERE guid = %u", pNewChar->GetGUIDLow());
        pNewChar->SetTemporaryUnsummonedPetNumber(pet_id);
    }
    if (class_ == CLASS_HUNTER)
    {
        switch(race_)
        {
        case RACE_HUMAN:
            CharacterDatabase.PExecute("REPLACE INTO character_pet (`id`, `entry`, `owner`, `modelid`, `CreatedBySpell`, `PetType`, `level`, `exp`, `Reactstate`, `name`, `renamed`, `slot`, `curhealth`, `curmana`, `curhappiness`, `savetime`, `resettalents_cost`, `resettalents_time`, `abdata`) VALUES (%u, 42717, %u, 903, 13481, 1, 1, 0, 0, 'Wolf', 0, 0, 192, 0, 166500, 1295727347, 0, 0, '7 2 7 1 7 0 129 2649 129 17253 1 0 1 0 6 2 6 1 6 0 ')",  pet_id, pNewChar->GetGUIDLow());
            break;
        case RACE_DWARF:
            CharacterDatabase.PExecute("REPLACE INTO character_pet (`id`, `entry`, `owner`, `modelid`, `CreatedBySpell`, `PetType`, `level`, `exp`, `Reactstate`, `name`, `renamed`, `slot`, `curhealth`, `curmana`, `curhappiness`, `savetime`, `resettalents_cost`, `resettalents_time`, `abdata`) VALUES (%u, 42713, %u, 822, 13481, 1, 1, 0, 0, 'Bear', 0, 0, 212, 0, 166500, 1295727650, 0, 0, '7 2 7 1 7 0 129 2649 129 16827 1 0 1 0 6 2 6 1 6 0 ')",  pet_id, pNewChar->GetGUIDLow());
            break;
        case RACE_ORC:
            CharacterDatabase.PExecute("REPLACE INTO character_pet (`id`, `entry`, `owner`, `modelid`, `CreatedBySpell`, `PetType`, `level`, `exp`, `Reactstate`, `name`, `renamed`, `slot`, `curhealth`, `curmana`, `curhappiness`, `savetime`, `resettalents_cost`, `resettalents_time`, `abdata`) VALUES (%u, 42719, %u, 744, 13481, 1, 1, 0, 0, 'Boar', 0, 0, 212, 0, 166500, 1295727175, 0, 0, '7 2 7 1 7 0 129 2649 129 17253 1 0 1 0 6 2 6 1 6 0 ')",  pet_id, pNewChar->GetGUIDLow());
            break;
        case RACE_NIGHTELF:
            CharacterDatabase.PExecute("REPLACE INTO character_pet (`id`, `entry`, `owner`, `modelid`, `CreatedBySpell`, `PetType`, `level`, `exp`, `Reactstate`, `name`, `renamed`, `slot`, `curhealth`, `curmana`, `curhappiness`, `savetime`, `resettalents_cost`, `resettalents_time`, `abdata`) VALUES (%u, 42718, %u,  17090, 13481, 1, 1, 0, 0, 'Cat', 0, 0, 192, 0, 166500, 1295727501, 0, 0, '7 2 7 1 7 0 129 2649 129 16827 1 0 1 0 6 2 6 1 6 0 ')",  pet_id, pNewChar->GetGUIDLow());
            break;
        case RACE_UNDEAD_PLAYER:
            CharacterDatabase.PExecute("REPLACE INTO character_pet (`id`, `entry`, `owner`, `modelid`, `CreatedBySpell`, `PetType`, `level`, `exp`, `Reactstate`, `name`, `renamed`, `slot`, `curhealth`, `curmana`, `curhappiness`, `savetime`, `resettalents_cost`, `resettalents_time`, `abdata`) VALUES (%u, 51107, %u,  368, 13481, 1, 1, 0, 0, 'Spider', 0, 0, 202, 0, 166500, 1295727821, 0, 0, '7 2 7 1 7 0 129 2649 129 17253 1 0 1 0 6 2 6 1 6 0 ')",  pet_id, pNewChar->GetGUIDLow());
            break;
        case RACE_TAUREN:
            CharacterDatabase.PExecute("REPLACE INTO character_pet (`id`, `entry`, `owner`, `modelid`, `CreatedBySpell`, `PetType`, `level`, `exp`, `Reactstate`, `name`, `renamed`, `slot`, `curhealth`, `curmana`, `curhappiness`, `savetime`, `resettalents_cost`, `resettalents_time`, `abdata`) VALUES (%u, 42720, %u,  29057, 13481, 1, 1, 0, 0, 'Tallstrider', 0, 0, 192, 0, 166500, 1295727912, 0, 0, '7 2 7 1 7 0 129 2649 129 16827 1 0 1 0 6 2 6 1 6 0 ')",  pet_id, pNewChar->GetGUIDLow());
            break;
        case RACE_TROLL:
            CharacterDatabase.PExecute("REPLACE INTO character_pet (`id`, `entry`, `owner`, `modelid`, `CreatedBySpell`, `PetType`, `level`, `exp`, `Reactstate`, `name`, `renamed`, `slot`, `curhealth`, `curmana`, `curhappiness`, `savetime`, `resettalents_cost`, `resettalents_time`, `abdata`) VALUES (%u, 42721, %u,  23518, 13481, 1, 1, 0, 0, 'Raptor', 0, 0, 192, 0, 166500, 1295727987, 0, 0, '7 2 7 1 7 0 129 2649 129 50498 129 16827 1 0 6 2 6 1 6 0 ')",  pet_id, pNewChar->GetGUIDLow());
            break;
        case RACE_GOBLIN:
            CharacterDatabase.PExecute("REPLACE INTO character_pet (`id`, `entry`, `owner`, `modelid`, `CreatedBySpell`, `PetType`, `level`, `exp`, `Reactstate`, `name`, `renamed`, `slot`, `curhealth`, `curmana`, `curhappiness`, `savetime`, `resettalents_cost`, `resettalents_time`, `abdata`) VALUES (%u, 42715, %u, 27692, 13481, 1, 1, 0, 0, 'Crab', 0, 0, 212, 0, 166500, 1295720595, 0, 0, '7 2 7 1 7 0 129 2649 129 16827 1 0 1 0 6 2 6 1 6 0 ')",  pet_id, pNewChar->GetGUIDLow());
            break;
        case RACE_BLOODELF:
            CharacterDatabase.PExecute("REPLACE INTO character_pet (`id`, `entry`, `owner`, `modelid`, `CreatedBySpell`, `PetType`, `level`, `exp`, `Reactstate`, `name`, `renamed`, `slot`, `curhealth`, `curmana`, `curhappiness`, `savetime`, `resettalents_cost`, `resettalents_time`, `abdata`) VALUES (%u, 42710, %u, 23515, 13481, 1, 1, 0, 0, 'Dragonhawk', 0, 0, 202, 0, 166500, 1295728068, 0, 0, '7 2 7 1 7 0 129 2649 129 17253 1 0 1 0 6 2 6 1 6 0 ')",  pet_id, pNewChar->GetGUIDLow());
            break;
        case RACE_DRAENEI:
            CharacterDatabase.PExecute("REPLACE INTO character_pet (`id`, `entry`, `owner`, `modelid`, `CreatedBySpell`, `PetType`, `level`, `exp`, `Reactstate`, `name`, `renamed`, `slot`, `curhealth`, `curmana`, `curhappiness`, `savetime`, `resettalents_cost`, `resettalents_time`, `abdata`) VALUES (%u, 42712, %u, 29056, 13481, 1, 1, 0, 0, 'Moth', 0, 0, 192, 0, 166500, 1295728128, 0, 0, '7 2 7 1 7 0 129 2649 129 49966 1 0 1 0 6 2 6 1 6 0 ')",  pet_id, pNewChar->GetGUIDLow());
            break;
        case RACE_WORGEN:
            CharacterDatabase.PExecute("REPLACE INTO character_pet (`id`, `entry`, `owner`, `modelid`, `CreatedBySpell`, `PetType`, `level`, `exp`, `Reactstate`, `name`, `renamed`, `slot`, `curhealth`, `curmana`, `curhappiness`, `savetime`, `resettalents_cost`, `resettalents_time`, `abdata`) VALUES (%u, 42722, %u, 30221, 13481, 1, 1, 0, 0, 'Dog', 0, 0, 192, 0, 166500, 1295728219, 0, 0, '7 2 7 1 7 0 129 2649 129 17253 1 0 1 0 6 2 6 1 6 0 ')",  pet_id, pNewChar->GetGUIDLow());
            break;
        }
        CharacterDatabase.PExecute("UPDATE characters SET currentPetSlot = '0' WHERE guid = %u", pNewChar->GetGUIDLow());
        pNewChar->SetTemporaryUnsummonedPetNumber(pet_id);
    }

    pNewChar->CleanupsBeforeDelete();

    data << (uint8)CHAR_CREATE_SUCCESS;
    SendPacket(&data);

    std::string IP_str = GetRemoteAddress();
    sLog->outDetail("Account: %d (IP: %s) Create Character:[%s] (GUID: %u)", GetAccountId(), IP_str.c_str(), name.c_str(), pNewChar->GetGUIDLow());
    sLog->outChar("IP:(%s) account:(%u) character:(%s) action:(%s) guid:(%u)",
                 IP_str.c_str(),
                 GetAccountId(),
                 name.c_str(),
                 "create",
                 pNewChar->GetGUIDLow());

    sScriptMgr->OnPlayerCreate(pNewChar);
    sWorld->AddCharacterNameData(pNewChar->GetGUIDLow(), pNewChar->GetName(), pNewChar->getGender(), pNewChar->getRace(), pNewChar->getClass(), pNewChar->getLevel());
    delete pNewChar;                                        // created only to call SaveToDB()

}

void WorldSession::HandleReorderCharacters(WorldPacket& recv_data)
{
    uint32 charCount = recv_data.ReadBits(10);

    std::vector<ObjectGuid> guid(charCount);
    uint8 pos;

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    for (uint8 i = 0; i < charCount; ++i)
    {
        guid[i][1] = recv_data.ReadBit();
        guid[i][4] = recv_data.ReadBit();
        guid[i][5] = recv_data.ReadBit();
        guid[i][3] = recv_data.ReadBit();
        guid[i][0] = recv_data.ReadBit();
        guid[i][7] = recv_data.ReadBit();
        guid[i][6] = recv_data.ReadBit();
        guid[i][2] = recv_data.ReadBit();
    }

    for (uint8 i = 0; i < charCount; ++i)
    {
        recv_data.ReadByteSeq(guid[i][6]);
        recv_data.ReadByteSeq(guid[i][5]);
        recv_data.ReadByteSeq(guid[i][1]);
        recv_data.ReadByteSeq(guid[i][4]);
        recv_data.ReadByteSeq(guid[i][0]);
        recv_data.ReadByteSeq(guid[i][3]);

        recv_data >> pos;

        // we need save only 1, 2, 3, 4 not 10, 20, 30, 40
        pos /= 10;

        recv_data.ReadByteSeq(guid[i][2]);
        recv_data.ReadByteSeq(guid[i][7]);

        trans->PAppend("UPDATE characters SET charSlot = %u WHERE guid = %u", pos, GUID_LOPART((uint64)guid[i]));
    }
    CharacterDatabase.CommitTransaction(trans);
}

void WorldSession::HandleCharDeleteOpcode(WorldPacket & recv_data)
{
    uint64 guid;
    recv_data >> guid;

    // can't delete loaded character
    if (sObjectMgr->GetPlayer(guid))
        return;

    uint32 accountId = 0;
    std::string name;

    // is guild leader
    if (sObjectMgr->GetGuildByLeader(guid))
    {
        WorldPacket data(SMSG_CHAR_DELETE, 1);
        data << (uint8)CHAR_DELETE_FAILED_GUILD_LEADER;
        SendPacket(&data);
        return;
    }

    // is arena team captain
    if (sObjectMgr->GetArenaTeamByCaptain(guid))
    {
        WorldPacket data(SMSG_CHAR_DELETE, 1);
        data << (uint8)CHAR_DELETE_FAILED_ARENA_CAPTAIN;
        SendPacket(&data);
        return;
    }

    QueryResult result = CharacterDatabase.PQuery("SELECT account,name FROM characters WHERE guid='%u'", GUID_LOPART(guid));
    if (result)
    {
        Field *fields = result->Fetch();
        accountId = fields[0].GetUInt32();
        name = fields[1].GetString();
    }

    // prevent deleting other players' characters using cheating tools
    if (accountId != GetAccountId())
        return;

    std::string IP_str = GetRemoteAddress();
    sLog->outDetail("Account: %d (IP: %s) Delete Character:[%s] (GUID: %u)",GetAccountId(),IP_str.c_str(),name.c_str(),GUID_LOPART(guid));
    sLog->outChar("IP:(%s) account:(%u) character:(%s) action:(%s) guid:(%u)",
                 IP_str.c_str(),
                 GetAccountId(),
                 name.c_str(),
                 "delete",
                 GUID_LOPART(guid));

    sScriptMgr->OnPlayerDelete(guid);

    sGuildFinderMgr->RemoveAllMembershipRequestsFromPlayer(guid);
    sWorld->DeleteCharacterNameData(GUID_LOPART(guid));

    if (sLog->IsOutCharDump())                                // optimize GetPlayerDump call
    {
        std::string dump;
        if (PlayerDumpWriter().GetDump(GUID_LOPART(guid), dump))
            sLog->outCharDump(dump.c_str(),GetAccountId(),GUID_LOPART(guid),name.c_str());
    }

    sCalendarMgr->RemoveAllPlayerEventsAndInvites(guid);
    Player::DeleteFromDB(guid, GetAccountId());

    WorldPacket data(SMSG_CHAR_DELETE, 1);
    data << (uint8)CHAR_DELETE_SUCCESS;
    SendPacket(&data);
}

void WorldSession::HandleWorldLoginOpcode(WorldPacket& recv_data)
{
    sLog->outDebug("WORLD: Recvd World Login Message");
    uint32 unk;
    uint8 loadingState; // 0x00 = loading end, 0x80 = loading started
    recv_data >> unk >> loadingState;

    // on loading end
    if (loadingState == 0x00)
    {
        if (GetPlayer() && GetPlayer()->IsInWorld())
        {
            Player* pl = GetPlayer();

            // If player has vehicle, then send everything about that
            // this avoids actionbar glitches after /reload
            if (pl->GetVehicle() && pl->GetVehicleBase())
            {
                Unit* veh = GetPlayer()->GetVehicleBase();

                pl->SetClientControl(veh, 1);
                pl->SetMover(veh);
                pl->SetViewpoint(veh, false);
                pl->SetViewpoint(veh, true);
                pl->VehicleSpellInitialize();
            }

            // Send update for players in arena
            if (pl->GetBattleground() && pl->GetBattleground()->isArena())
                pl->UpdateObjectVisibility(true);

            // Also resync runes and their types
            if (pl->getClass() == CLASS_DEATH_KNIGHT)
                pl->ResyncRunes(MAX_RUNES);
        }
    }
}

void WorldSession::HandlePlayerLoginOpcode(WorldPacket & recv_data)
{
    if (PlayerLoading() || GetPlayer() != NULL)
    {
        sLog->outError("Player tryes to login again, AccountId = %d",GetAccountId());
        return;
    }

    m_playerLoading = true;
    uint64 playerGuid = 0;

    sLog->outStaticDebug("WORLD: Recvd Player Logon Message");

    BitStream mask = recv_data.ReadBitStream(8);

    ByteBuffer bytes(8, true);
    //15595
    recv_data.ReadXorByte(mask[0], bytes[2]);
    recv_data.ReadXorByte(mask[7], bytes[7]);
    recv_data.ReadXorByte(mask[2], bytes[0]);
    recv_data.ReadXorByte(mask[1], bytes[3]);
    recv_data.ReadXorByte(mask[5], bytes[5]);
    recv_data.ReadXorByte(mask[3], bytes[6]);
    recv_data.ReadXorByte(mask[6], bytes[1]);
    recv_data.ReadXorByte(mask[4], bytes[4]);

    playerGuid = BitConverter::ToUInt64(bytes);

    if (!CharCanLogin(playerGuid))
    {
        KickPlayer();
        return;
    }

    LoginQueryHolder *holder = new LoginQueryHolder(GetAccountId(), playerGuid);
    if (!holder->Initialize())
    {
        delete holder;                                      // delete all unprocessed queries
        m_playerLoading = false;
        return;
    }

    m_charLoginCallback = CharacterDatabase.DelayQueryHolder((SQLQueryHolder*)holder);
}

void WorldSession::HandlePlayerLogin(LoginQueryHolder * holder)
{
    uint64 playerGuid = holder->GetGuid();

    // "GetAccountId() == db stored account id" checked in LoadFromDB (prevent login not own character using cheating tools)
    Player* pCurrChar = Player::LoadFromDB(GUID_LOPART(playerGuid), holder, this);

    if (!pCurrChar)
    {
        KickPlayer();                                       // disconnect client, player no set to session and it will not deleted or saved at kick
        delete pCurrChar;                                   // delete it manually
        delete holder;                                      // delete all unprocessed queries
        m_playerLoading = false;
        return;
    }

    // for send server info and strings (config)
    ChatHandler chH = ChatHandler(pCurrChar);
    
    pCurrChar->GetMotionMaster()->Initialize();

    SetPlayer(pCurrChar);

    pCurrChar->SendDungeonDifficulty(false);

    WorldPacket data(SMSG_LOGIN_VERIFY_WORLD, 20);
    data << pCurrChar->GetMapId();
    data << pCurrChar->GetPositionX();
    data << pCurrChar->GetPositionY();
    data << pCurrChar->GetPositionZ();
    data << pCurrChar->GetOrientation();
    SendPacket(&data);

    // load player specific part before send times
    LoadAccountData(holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOADACCOUNTDATA), PER_CHARACTER_CACHE_MASK);
    SendAccountDataTimes(PER_CHARACTER_CACHE_MASK);

    bool featureBit4 = true;
    bool featureBit5 = false;
    data.Initialize(SMSG_FEATURE_SYSTEM_STATUS, 2);         // added in 2.2.0
    data << uint8(2);                                       // unknown value
    data << uint32(1);
    data << uint32(1);
    data << uint32(2);
    data << uint32(0);
    data.WriteBit(1);
    data.WriteBit(1);
    data.WriteBit(0);
    data.WriteBit(featureBit4);
    data.WriteBit(0);
    data.WriteBit(0);
    data.FlushBits();
    if (featureBit4)
    {
        data << uint32(1);
        data << uint32(0);
        data << uint32(10);
        data << uint32(60);
    }

    if (featureBit5)
    {
        data << uint32(0);
        data << uint32(0);
        data << uint32(0);
    }
    SendPacket(&data);

    // Send MOTD
    {
        data.Initialize(SMSG_MOTD, 50);                     // new in 2.0.1
        data << (uint32)0;

        uint32 linecount=0;
        std::string str_motd = sWorld->GetMotd();
        std::string::size_type pos, nextpos;

        pos = 0;
        while ((nextpos= str_motd.find('@',pos)) != std::string::npos)
        {
            if (nextpos != pos)
            {
                data << str_motd.substr(pos,nextpos-pos);
                ++linecount;
            }
            pos = nextpos+1;
        }

        if (pos<str_motd.length())
        {
            data << str_motd.substr(pos);
            ++linecount;
        }

        data.put(0, linecount);

        SendPacket(&data);
        sLog->outStaticDebug("WORLD: Sent motd (SMSG_MOTD)");
    }

    //QueryResult *result = CharacterDatabase.PQuery("SELECT guildid,rank FROM guild_member WHERE guid = '%u'",pCurrChar->GetGUIDLow());
    if (PreparedQueryResult resultGuild = holder->GetPreparedResult(PLAYER_LOGIN_QUERY_LOADGUILD))
    {
        Field* fields = resultGuild->Fetch();
        pCurrChar->SetInGuild(fields[0].GetUInt32());
        pCurrChar->SetRank(fields[1].GetUInt8());
    }
    else if (pCurrChar->GetGuildId())                        // clear guild related fields in case wrong data about non existed membership
    {
        pCurrChar->SetInGuild(0);
        pCurrChar->SetRank(0);
    }

    if (pCurrChar->GetGuildId() != 0)
    {
        if (Guild* pGuild = sObjectMgr->GetGuildById(pCurrChar->GetGuildId()))
        {
            pGuild->SendLoginInfo(this);
            pCurrChar->SetUInt32Value(PLAYER_GUILDLEVEL, uint32(pGuild->GetLevel()));
            
            if(sWorld->getBoolConfig(CONFIG_GUILD_ADVANCEMENT_ENABLED))
            {
                pCurrChar->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_GLEVEL_ENABLED);

                /// Learn perks to him
                for(int i = 1; i < pGuild->GetLevel(); ++i)
                    if(const GuildPerksEntry* perk = sGuildPerksStore.LookupEntry(i))
                        pCurrChar->LearnSpell(perk->SpellId, true);
            }

            pGuild->UpdateMemberStats(pCurrChar);

            for (uint32 i = 0; i < 2; i++)
            {
                uint32 skillid = pCurrChar->GetUInt32Value(PLAYER_PROFESSION_SKILL_LINE_1 + i);
                pGuild->SetMemberProfessionData(pCurrChar->GetGUID(), skillid, pCurrChar->GetSkillValue(skillid), 0);
            }
        }
        else
        {
            // remove wrong guild data
            sLog->outError("Player %s (GUID: %u) marked as member of not existing guild (id: %u), removing guild membership for player.",pCurrChar->GetName(),pCurrChar->GetGUIDLow(),pCurrChar->GetGuildId());
            pCurrChar->SetInGuild(0);
        }
    }

    data.Initialize(SMSG_LEARNED_DANCE_MOVES, 4+4);
    data << uint64(0);
    SendPacket(&data);

    data.Initialize(SMSG_HOTFIX_INFO);
    data.WriteBits(0, 22); // size of hotfix info
    data.FlushBits();
    /*
    for (uint32 i = 0; i < hotfix.size(); ++i)
    {
        data << uint32(hotfix[i].Type);
        data << uint32(hotfix[i].Timestamp);
        data << uint32(hotfix[i].Entry);
    }
    */
    SendPacket(&data);

    pCurrChar->SendInitialPacketsBeforeAddToMap();

    //Show cinematic at the first time that player login
    if (!pCurrChar->getCinematic())
    {
        pCurrChar->setCinematic(1);

        if (ChrClassesEntry const* cEntry = sChrClassesStore.LookupEntry(pCurrChar->getClass()))
        {
            if (cEntry->CinematicSequence)
                pCurrChar->SendCinematicStart(cEntry->CinematicSequence);
            else if (ChrRacesEntry const* rEntry = sChrRacesStore.LookupEntry(pCurrChar->getRace()))
                pCurrChar->SendCinematicStart(rEntry->CinematicSequence);

            // send new char string if not empty
            if (!sWorld->GetNewCharString().empty())
                chH.PSendSysMessage("%s", sWorld->GetNewCharString().c_str());
        }
    }

    if (!pCurrChar->GetMap()->Add(pCurrChar) || !pCurrChar->CheckInstanceLoginValid())
    {
        AreaTriggerStruct const* at = sObjectMgr->GetGoBackTrigger(pCurrChar->GetMapId());
        if (at)
            pCurrChar->TeleportTo(at->target_mapId, at->target_X, at->target_Y, at->target_Z, pCurrChar->GetOrientation());
        else
            pCurrChar->RepopAtGraveyard();
    }

    sObjectAccessor->AddObject(pCurrChar);
    //sLog->outDebug("Player %s added to Map.",pCurrChar->GetName());

    pCurrChar->SendInitialPacketsAfterAddToMap();

    CharacterDatabase.PExecute("UPDATE characters SET online = 1 WHERE guid = '%u'", pCurrChar->GetGUIDLow());
    LoginDatabase.PExecute("UPDATE account SET online = '%u' WHERE id = '%u'", realmID, GetAccountId());
    pCurrChar->SetInGameTime(getMSTime());

    if (Player::TeamForRace(pCurrChar->getRace()) == ALLIANCE)
        sWorld->IncreasePlayerCountAlliance();

    // announce group about member online (must be after add to player list to receive announce to self)
    if (Group *group = pCurrChar->GetGroup())
    {
        //pCurrChar->groupInfo.group->SendInit(this); // useless
        group->SendUpdate();
        group->ResetMaxEnchantingLevel();
    }

    // friend status
    sSocialMgr->SendFriendStatus(pCurrChar, FRIEND_ONLINE, pCurrChar->GetGUIDLow(), true);

    // Place character in world (and load zone) before some object loading
    pCurrChar->LoadCorpse();

    // setting Ghost+speed if dead
    if (pCurrChar->m_deathState != ALIVE)
    {
        // not blizz like, we must correctly save and load player instead...
        if (pCurrChar->getRace() == RACE_NIGHTELF)
            pCurrChar->CastSpell(pCurrChar, 20584, true, 0);// auras SPELL_AURA_INCREASE_SPEED(+speed in wisp form), SPELL_AURA_INCREASE_SWIM_SPEED(+swim speed in wisp form), SPELL_AURA_TRANSFORM (to wisp form)
        pCurrChar->CastSpell(pCurrChar, 8326, true, 0);     // auras SPELL_AURA_GHOST, SPELL_AURA_INCREASE_SPEED(why?), SPELL_AURA_INCREASE_SWIM_SPEED(why?)

        // guild perk The Quick and the Dead
        if (pCurrChar->HasAura(83950))
            pCurrChar->CastSpell(pCurrChar, 84559, true);

        pCurrChar->SetWaterWalk(true);
    }

    pCurrChar->ContinueTaxiFlight();

    // Set FFA PvP for non GM in non-rest mode
    if (sWorld->IsFFAPvPRealm() && !pCurrChar->IsGameMaster() && !pCurrChar->HasFlag(PLAYER_FLAGS,PLAYER_FLAGS_RESTING))
        pCurrChar->SetByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP);

    if (pCurrChar->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_CONTESTED_PVP))
        pCurrChar->SetContestedPvP();

    // Apply at_login requests
    if (pCurrChar->HasAtLoginFlag(AT_LOGIN_RESET_SPELLS))
    {
        pCurrChar->ResetSpells();
        SendNotification(LANG_RESET_SPELLS);
    }

    if (pCurrChar->HasAtLoginFlag(AT_LOGIN_LEARN_DEFAULT))
    {
        pCurrChar->LearnDefaultSpells();
        pCurrChar->LearnQuestRewardedSpells();
        pCurrChar->RemoveAtLoginFlag(AT_LOGIN_LEARN_DEFAULT);
    }

    if (pCurrChar->HasAtLoginFlag(AT_LOGIN_RESET_PET_SLOT))
    {
        pCurrChar->m_currentPetSlot = PET_SLOT_NO_PET;
        pCurrChar->RemoveAtLoginFlag(AT_LOGIN_RESET_PET_SLOT);
    }

    // reset for all pets before pet loading
    if (pCurrChar->HasAtLoginFlag(AT_LOGIN_RESET_PET_TALENTS))
        Pet::resetTalentsForAllPetsOf(pCurrChar);

    // Load pet if any (if player not alive and in taxi flight or another then pet will remember as temporary unsummoned)
    pCurrChar->LoadPet();
    pCurrChar->GetSession()->SendStablePet(0);

    if (pCurrChar->HasAtLoginFlag(AT_LOGIN_RESET_TALENTS))
    {
        pCurrChar->ResetTalents(true);
        pCurrChar->SendTalentsInfoData(false);              // original talents send already in to SendInitialPacketsBeforeAddToMap, resend reset state
        SendNotification(LANG_RESET_TALENTS);
    }

    if (pCurrChar->HasAtLoginFlag(AT_LOGIN_FIRST))
        pCurrChar->RemoveAtLoginFlag(AT_LOGIN_FIRST);

    if (pCurrChar->HasAtLoginFlag(AT_LOGIN_REMOVE_PVP_TITLES))
    {
        CharTitlesEntry const* tit = sCharTitlesStore.LookupEntry(42);
        if (tit)
            pCurrChar->SetTitle(tit, true);

        tit = sCharTitlesStore.LookupEntry(43);
        if (tit)
            pCurrChar->SetTitle(tit, true);

        tit = sCharTitlesStore.LookupEntry(44);
        if (tit)
            pCurrChar->SetTitle(tit, true);

        tit = sCharTitlesStore.LookupEntry(45);
        if (tit)
            pCurrChar->SetTitle(tit, true);

        if (!pCurrChar->HasTitle(pCurrChar->GetUInt32Value(PLAYER_CHOSEN_TITLE)))
            pCurrChar->SetUInt32Value(PLAYER_CHOSEN_TITLE, 0);

        pCurrChar->RemoveAtLoginFlag(AT_LOGIN_REMOVE_PVP_TITLES);

        pCurrChar->SaveToDB();
    }

    // show time before shutdown if shutdown planned.
    if (sWorld->IsShutdowning())
        sWorld->ShutdownMsg(true,pCurrChar);

    if (pCurrChar)
        pCurrChar->SetTaxiCheater(true);

    if (pCurrChar->IsGameMaster())
        SendNotification(LANG_GM_ON);

    if (pCurrChar->getRace() == RACE_WORGEN /* is Worgen (:P) */
        && (pCurrChar->GetQuestStatus(14222) == QUEST_STATUS_COMPLETE) /* Has Quest Last Stand */ 
        && !pCurrChar->HasSpell(68996) /* Hasn't spell Two Forms */)
        pCurrChar->toggleWorgenForm();

    std::string IP_str = GetRemoteAddress();
    sLog->outChar("IP:(%s) account:(%u) character:(%s) action:(%s) guid:(%u)",
                 IP_str.c_str(),
                 GetAccountId(),
                 pCurrChar->GetName(),
                 "login",
                 pCurrChar->GetGUIDLow());

    if (!pCurrChar->IsStandState() && !pCurrChar->HasUnitState(UNIT_STATE_STUNNED))
        pCurrChar->SetStandState(UNIT_STAND_STATE_STAND);

    m_playerLoading = false;

    pCurrChar->ClearEclipseState();

    sScriptMgr->OnPlayerLogin(pCurrChar);

    delete holder;
}

void WorldSession::HandleSetFactionAtWar(WorldPacket & recv_data)
{
    sLog->outStaticDebug("WORLD: Received CMSG_SET_FACTION_ATWAR");

    uint32 repListID;
    uint8  flag;

    recv_data >> repListID;
    recv_data >> flag;

    GetPlayer()->GetReputationMgr().SetAtWar(repListID,flag);
}

//I think this function is never used :/ I dunno, but i guess this opcode not exists
void WorldSession::HandleSetFactionCheat(WorldPacket & /*recv_data*/)
{
    sLog->outError("WORLD SESSION: HandleSetFactionCheat, not expected call, please report.");
    /*
        uint32 FactionID;
        uint32 Standing;

        recv_data >> FactionID;
        recv_data >> Standing;

        std::list<struct Factions>::iterator itr;

        for (itr = GetPlayer()->factions.begin(); itr != GetPlayer()->factions.end(); ++itr)
        {
            if (itr->ReputationListID == FactionID)
            {
                itr->Standing += Standing;
                itr->Flags = (itr->Flags | 1);
                break;
            }
        }
    */
    GetPlayer()->GetReputationMgr().SendStates();
}

void WorldSession::HandleMeetingStoneInfo(WorldPacket & /*recv_data*/)
{
    sLog->outStaticDebug("WORLD: Received CMSG_MEETING_STONE_INFO");

    //SendLfgUpdate(0, 0, 0);
}

void WorldSession::HandleTutorialFlag(WorldPacket & recv_data)
{
    uint32 iFlag;
    recv_data >> iFlag;

    uint32 wInt = (iFlag / 32);
    if (wInt >= 8)
    {
        //sLog->outError("CHEATER? Account:[%d] Guid[%u] tried to send wrong CMSG_TUTORIAL_FLAG", GetAccountId(),GetGUID());
        return;
    }
    uint32 rInt = (iFlag % 32);

    uint32 tutflag = GetTutorialInt(wInt);
    tutflag |= (1 << rInt);
    SetTutorialInt(wInt, tutflag);

    //sLog->outDebug("Received Tutorial Flag Set {%u}.", iFlag);
}

void WorldSession::HandleTutorialClear(WorldPacket & /*recv_data*/)
{
    for (int i = 0; i < MAX_CHARACTER_TUTORIAL_VALUES; ++i)
        SetTutorialInt(i, 0xFFFFFFFF);
}

void WorldSession::HandleTutorialReset(WorldPacket & /*recv_data*/)
{
    for (int i = 0; i < MAX_CHARACTER_TUTORIAL_VALUES; ++i)
        SetTutorialInt(i, 0x00000000);
}

void WorldSession::HandleSetWatchedFactionOpcode(WorldPacket & recv_data)
{
    sLog->outStaticDebug("WORLD: Received CMSG_SET_WATCHED_FACTION");
    uint32 fact;
    recv_data >> fact;
    GetPlayer()->SetUInt32Value(PLAYER_FIELD_WATCHED_FACTION_INDEX, fact);
}

void WorldSession::HandleSetFactionInactiveOpcode(WorldPacket & recv_data)
{
    sLog->outStaticDebug("WORLD: Received CMSG_SET_FACTION_INACTIVE");
    uint32 replistid;
    uint8 inactive;
    recv_data >> replistid >> inactive;

    _player->GetReputationMgr().SetInactive(replistid, inactive);
}

void WorldSession::HandleShowingHelmOpcode(WorldPacket & /*recv_data*/)
{
    sLog->outStaticDebug("CMSG_SHOWING_HELM for %s", _player->GetName());
    _player->ToggleFlag(PLAYER_FLAGS, PLAYER_FLAGS_HIDE_HELM);
}

void WorldSession::HandleShowingCloakOpcode(WorldPacket & /*recv_data*/)
{
    sLog->outStaticDebug("CMSG_SHOWING_CLOAK for %s", _player->GetName());
    _player->ToggleFlag(PLAYER_FLAGS, PLAYER_FLAGS_HIDE_CLOAK);
}

void WorldSession::HandleCharRenameOpcode(WorldPacket& recv_data)
{
    uint64 guid;
    std::string newname;

    recv_data >> guid;
    recv_data >> newname;

    // prevent character rename to invalid name
    if (!normalizePlayerName(newname))
    {
        WorldPacket data(SMSG_CHAR_RENAME, 1);
        data << uint8(CHAR_NAME_NO_NAME);
        SendPacket(&data);
        return;
    }

    uint8 res = ObjectMgr::CheckPlayerName(newname,true);
    if (res != CHAR_NAME_SUCCESS)
    {
        WorldPacket data(SMSG_CHAR_RENAME, 1);
        data << uint8(res);
        SendPacket(&data);
        return;
    }

    // check name limitations
    if (GetSecurity() == SEC_PLAYER && sObjectMgr->IsReservedName(newname))
    {
        WorldPacket data(SMSG_CHAR_RENAME, 1);
        data << uint8(CHAR_NAME_RESERVED);
        SendPacket(&data);
        return;
    }

    std::string escaped_newname = newname;
    CharacterDatabase.escape_string(escaped_newname);

    // make sure that the character belongs to the current account, that rename at login is enabled
    // and that there is no character with the desired new name
    m_charRenameCallback.SetParam(newname);
    m_charRenameCallback.SetFutureResult(
        CharacterDatabase.AsyncPQuery(
        "SELECT guid, name FROM characters WHERE guid = %d AND account = %d AND (at_login & %d) = %d AND NOT EXISTS (SELECT NULL FROM characters WHERE name = '%s')",
        GUID_LOPART(guid), GetAccountId(), AT_LOGIN_RENAME, AT_LOGIN_RENAME, escaped_newname.c_str()
        )
    );
}

void WorldSession::HandleChangePlayerNameOpcodeCallBack(QueryResult result, std::string newname)
{
    if (!result)
    {
        WorldPacket data(SMSG_CHAR_RENAME, 1);
        data << uint8(CHAR_CREATE_ERROR);
        SendPacket(&data);
        return;
    }

    uint32 guidLow = result->Fetch()[0].GetUInt32();
    uint64 guid = MAKE_NEW_GUID(guidLow, 0, HIGHGUID_PLAYER);
    std::string oldname = result->Fetch()[1].GetString();

    CharacterDatabase.PExecute("UPDATE characters set name = '%s', at_login = at_login & ~ %u WHERE guid ='%u'", newname.c_str(), uint32(AT_LOGIN_RENAME), guidLow);
    CharacterDatabase.PExecute("DELETE FROM character_declinedname WHERE guid ='%u'", guidLow);

    sLog->outChar("IP:(%s) account:(%u) character:(%s) action:(%s) new_name:(%s) guid:(%u)",
                 GetRemoteAddress().c_str(),
                 GetAccountId(),
                 oldname.c_str(),
                 "rename",
                 newname.c_str(),
                 guidLow);

    WorldPacket data(SMSG_CHAR_RENAME, 1+8+(newname.size()+1));
    data << uint8(RESPONSE_SUCCESS);
    data << uint64(guid);
    data << newname;
    SendPacket(&data);

    sWorld->UpdateCharacterNameData(guidLow, newname);
}

void WorldSession::HandleSetPlayerDeclinedNames(WorldPacket& recv_data)
{
    uint64 guid;

    recv_data >> guid;

    // not accept declined names for unsupported languages
    std::string name;
    if (!sObjectMgr->GetPlayerNameByGUID(guid, name))
    {
        WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4+8);
        data << uint32(1);
        data << uint64(guid);
        SendPacket(&data);
        return;
    }

    std::wstring wname;
    if (!Utf8toWStr(name, wname))
    {
        WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4+8);
        data << uint32(1);
        data << uint64(guid);
        SendPacket(&data);
        return;
    }

    if (!isCyrillicCharacter(wname[0]))                      // name already stored as only single alphabet using
    {
        WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4+8);
        data << uint32(1);
        data << uint64(guid);
        SendPacket(&data);
        return;
    }

    std::string name2;
    DeclinedName declinedname;

    recv_data >> name2;

    if (name2 != name)                                       // character have different name
    {
        WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4+8);
        data << uint32(1);
        data << uint64(guid);
        SendPacket(&data);
        return;
    }

    for (int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
    {
        recv_data >> declinedname.name[i];
        if (!normalizePlayerName(declinedname.name[i]))
        {
            WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4+8);
            data << uint32(1);
            data << uint64(guid);
            SendPacket(&data);
            return;
        }
    }

    if (!ObjectMgr::CheckDeclinedNames(GetMainPartOfName(wname, 0), declinedname))
    {
        WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4+8);
        data << uint32(1);
        data << uint64(guid);
        SendPacket(&data);
        return;
    }

    for (int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
        CharacterDatabase.escape_string(declinedname.name[i]);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    trans->PAppend("DELETE FROM character_declinedname WHERE guid = '%u'", GUID_LOPART(guid));
    trans->PAppend("INSERT INTO character_declinedname (guid, genitive, dative, accusative, instrumental, prepositional) VALUES ('%u','%s','%s','%s','%s','%s')",
        GUID_LOPART(guid), declinedname.name[0].c_str(), declinedname.name[1].c_str(), declinedname.name[2].c_str(), declinedname.name[3].c_str(), declinedname.name[4].c_str());
    CharacterDatabase.CommitTransaction(trans);

    WorldPacket data(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, 4+8);
    data << uint32(0);                                      // OK
    data << uint64(guid);
    SendPacket(&data);
}

void WorldSession::HandleAlterAppearance(WorldPacket & recv_data)
{
    sLog->outDebug("CMSG_ALTER_APPEARANCE");

    uint32 Hair, Color, FacialHair, SkinColor;
    recv_data >> Hair >> Color >> FacialHair >> SkinColor;

    BarberShopStyleEntry const* bs_hair = sBarberShopStyleStore.LookupEntry(Hair);

    if (!bs_hair || bs_hair->type != 0 || bs_hair->race != _player->getRace() || bs_hair->gender != _player->getGender())
        return;

    BarberShopStyleEntry const* bs_facialHair = sBarberShopStyleStore.LookupEntry(FacialHair);

    if (!bs_facialHair || bs_facialHair->type != 2 || bs_facialHair->race != _player->getRace() || bs_facialHair->gender != _player->getGender())
        return;

    BarberShopStyleEntry const* bs_skinColor = sBarberShopStyleStore.LookupEntry(SkinColor);

    if (bs_skinColor && (bs_skinColor->type != 3 || bs_skinColor->race != _player->getRace() || bs_skinColor->gender != _player->getGender()))
        return;

    uint64 Cost = _player->GetBarberShopCost(bs_hair->hair_id, Color, bs_facialHair->hair_id, bs_skinColor);

    // 0 - ok
    // 1,3 - not enough money
    // 2 - you have to seat on barber chair
    if (!_player->HasEnoughMoney(Cost))
    {
        WorldPacket data(SMSG_BARBER_SHOP_RESULT, 4);
        data << uint32(1);                                  // no money
        SendPacket(&data);
        return;
    }
    else
    {
        WorldPacket data(SMSG_BARBER_SHOP_RESULT, 4);
        data << uint32(0);                                  // ok
        SendPacket(&data);
    }

    _player->ModifyMoney(-int64(Cost));                     // it isn't free
    _player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_AT_BARBER, Cost);

    _player->SetByteValue(PLAYER_BYTES, 2, uint8(bs_hair->hair_id));
    _player->SetByteValue(PLAYER_BYTES, 3, uint8(Color));
    _player->SetByteValue(PLAYER_BYTES_2, 0, uint8(bs_facialHair->hair_id));
    if (bs_skinColor)
        _player->SetByteValue(PLAYER_BYTES, 0, uint8(bs_skinColor->hair_id));

    _player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP, 1);

    _player->SetStandState(0);                              // stand up
}

void WorldSession::HandleRemoveGlyph(WorldPacket & recv_data)
{
    uint32 slot;
    recv_data >> slot;

    if (slot >= MAX_GLYPH_SLOT_INDEX)
    {
        sLog->outDebug("Client sent wrong glyph slot number in opcode CMSG_REMOVE_GLYPH %u", slot);
        return;
    }

    if (uint32 glyph = _player->GetGlyph(slot))
    {
        if (GlyphPropertiesEntry const *gp = sGlyphPropertiesStore.LookupEntry(glyph))
        {
            _player->RemoveAurasDueToSpell(gp->SpellId);
            _player->SetGlyph(slot, 0);
            _player->SendTalentsInfoData(false);
        }
    }
}

void WorldSession::HandleItemReforge(WorldPacket& recvPacket)
{
    uint32 slot;      // bag slot
    uint32 reforgeID; // ID of reforge entry in ItemReforge.dbc
    ObjectGuid guid;  // NPC guid
    uint32 bag;       // bag identifier

    recvPacket >> reforgeID >> slot >> bag;

    guid[2] = recvPacket.ReadBit();
    guid[6] = recvPacket.ReadBit();
    guid[3] = recvPacket.ReadBit();
    guid[4] = recvPacket.ReadBit();
    guid[1] = recvPacket.ReadBit();
    guid[0] = recvPacket.ReadBit();
    guid[7] = recvPacket.ReadBit();
    guid[5] = recvPacket.ReadBit();

    recvPacket.ReadByteSeq(guid[2]);
    recvPacket.ReadByteSeq(guid[3]);
    recvPacket.ReadByteSeq(guid[6]);
    recvPacket.ReadByteSeq(guid[4]);
    recvPacket.ReadByteSeq(guid[1]);
    recvPacket.ReadByteSeq(guid[0]);
    recvPacket.ReadByteSeq(guid[7]);
    recvPacket.ReadByteSeq(guid[5]);

    ItemReforgeEntry const* ref_info = sItemReforgeStore.LookupEntry(reforgeID);
    if(!ref_info && reforgeID != 0)
    {
        sLog->outError("Invalid reforge ID %u received in packet", reforgeID);
        return;
    }

    uint16 mslot = ((bag << 8) | slot);
    Item* dstItem = GetPlayer()->GetItemByPos(mslot >> 8, mslot & 255);
    if(!dstItem)
    {
        sLog->outError("Invalid bag slot %u received in reforge packet", slot);
        return;
    }

    const ItemPrototype* pProto = dstItem->GetProto();
    if (!pProto || pProto->ItemLevel < 200)
    {
        if (!pProto)
            sLog->outError("Invalid item for reforging - not exist or proto doesnt exist");
        SendNotification("The item level is not high enough!");
        return;
    }

    // Check for money
    if (pProto->SellPrice > GetPlayer()->GetMoney() && reforgeID != 0)
    {
        sLog->outDebug("Player doesn't have enough money for reforge item");
        SendNotification("You don't have enough money!");
        return;
    }

    // Only not-broken items could be reforged
    if (dstItem->IsBroken())
    {
        sLog->outChar("Exploit attempt: Player %s (%u) tried to reforge item %u with 0 durability", GetPlayer()->GetName(), GetPlayer()->GetGUIDLow(), pProto->ItemId);
        return;
    }

    // And take money equal to sell price
    if (reforgeID != 0)
        GetPlayer()->ModifyMoney(-int64(pProto->SellPrice));

    if (ref_info)
        sLog->outDebug("ID: %u, stat: %u, mod: %f, new: %u, newmod: %f",reforgeID, ref_info->source_stat, ref_info->source_mod, ref_info->new_stat, ref_info->new_mod);
    else
        sLog->outDebug("Removing reforge from item");
    sLog->outDebug("Item: %u - %s", dstItem->GetEntry(), dstItem->GetProto()->Name1);

    WorldPacket data(SMSG_REFORGE_RESULT,1);
    data.WriteBit(reforgeID);
    data.FlushBits();
    SendPacket(&data);

    dstItem->SetReforge(reforgeID);
}

void WorldSession::HandleCharCustomize(WorldPacket& recv_data)
{
    uint64 guid;
    std::string newname;

    recv_data >> guid;
    recv_data >> newname;

    uint8 gender, skin, face, hairStyle, hairColor, facialHair;
    recv_data >> gender >> skin >> hairColor >> hairStyle >> facialHair >> face;

    QueryResult result = CharacterDatabase.PQuery("SELECT at_login FROM characters WHERE guid ='%u'", GUID_LOPART(guid));
    if (!result)
    {
        WorldPacket data(SMSG_CHAR_CUSTOMIZE, 1);
        data << uint8(CHAR_CREATE_ERROR);
        SendPacket(&data);
        return;
    }

    Field *fields = result->Fetch();
    uint32 at_loginFlags = fields[0].GetUInt32();

    if (!(at_loginFlags & AT_LOGIN_CUSTOMIZE))
    {
        WorldPacket data(SMSG_CHAR_CUSTOMIZE, 1);
        data << uint8(CHAR_CREATE_ERROR);
        SendPacket(&data);
        return;
    }

    // prevent character rename to invalid name
    if (!normalizePlayerName(newname))
    {
        WorldPacket data(SMSG_CHAR_CUSTOMIZE, 1);
        data << uint8(CHAR_NAME_NO_NAME);
        SendPacket(&data);
        return;
    }

    uint8 res = ObjectMgr::CheckPlayerName(newname,true);
    if (res != CHAR_NAME_SUCCESS)
    {
        WorldPacket data(SMSG_CHAR_CUSTOMIZE, 1);
        data << uint8(res);
        SendPacket(&data);
        return;
    }

    // check name limitations
    if (GetSecurity() == SEC_PLAYER && sObjectMgr->IsReservedName(newname))
    {
        WorldPacket data(SMSG_CHAR_CUSTOMIZE, 1);
        data << uint8(CHAR_NAME_RESERVED);
        SendPacket(&data);
        return;
    }

    // character with this name already exist
    if (uint64 newguid = sObjectMgr->GetPlayerGUIDByName(newname))
    {
        if (newguid != guid)
        {
            WorldPacket data(SMSG_CHAR_CUSTOMIZE, 1);
            data << uint8(CHAR_CREATE_NAME_IN_USE);
            SendPacket(&data);
            return;
        }
    }

    CharacterDatabase.escape_string(newname);
    if (QueryResult result = CharacterDatabase.PQuery("SELECT name FROM characters WHERE guid ='%u'", GUID_LOPART(guid)))
    {
        std::string oldname = result->Fetch()[0].GetString();
        std::string IP_str = GetRemoteAddress();
        sLog->outChar("IP:(%s) account:(%u) character:(%s) action:(%s) new_name:(%s) guid:(%u)",
                     IP_str.c_str(),
                     GetAccountId(),
                     oldname.c_str(),
                     "customize",
                     newname.c_str(),
                     GUID_LOPART(guid));
    }
    Player::Customize(guid, gender, skin, face, hairStyle, hairColor, facialHair);
    CharacterDatabase.PExecute("UPDATE characters set name = '%s', at_login = at_login & ~ %u WHERE guid ='%u'", newname.c_str(), uint32(AT_LOGIN_CUSTOMIZE), GUID_LOPART(guid));
    CharacterDatabase.PExecute("DELETE FROM character_declinedname WHERE guid ='%u'", GUID_LOPART(guid));

    sWorld->UpdateCharacterNameData(GUID_LOPART(guid), newname, gender);

    WorldPacket data(SMSG_CHAR_CUSTOMIZE, 1+8+(newname.size()+1)+6);
    data << uint8(RESPONSE_SUCCESS);
    data << uint64(guid);
    data << newname;
    data << uint8(gender);
    data << uint8(skin);
    data << uint8(face);
    data << uint8(hairStyle);
    data << uint8(hairColor);
    data << uint8(facialHair);
    SendPacket(&data);
}

void WorldSession::HandleEquipmentSetSave(WorldPacket &recv_data)
{
    sLog->outDebug("CMSG_EQUIPMENT_SET_SAVE");

    uint64 setGuid;
    recv_data.readPackGUID(setGuid);

    uint32 index;
    recv_data >> index;
    if (index >= MAX_EQUIPMENT_SET_INDEX)                    // client set slots amount
        return;

    std::string name;
    recv_data >> name;

    std::string iconName;
    recv_data >> iconName;

    EquipmentSet eqSet;

    eqSet.Guid      = setGuid;
    eqSet.Name      = name;
    eqSet.IconName  = iconName;
    eqSet.state     = EQUIPMENT_SET_NEW;

    for (uint32 i = 0; i < EQUIPMENT_SLOT_END; ++i)
    {
        uint64 itemGuid;
        recv_data.readPackGUID(itemGuid);

        // equipment manager sends "1" (as raw GUID) for slots set to "ignore" (don't touch slot at equip set)
        if (itemGuid == 1)
        {
            // ignored slots saved as bit mask because we have no free special values for Items[i]
            eqSet.IgnoreMask |= 1 << i;
            continue;
        }

        Item *item = _player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);

        if (!item && itemGuid)                               // cheating check 1
            return;

        if (item && item->GetGUID() != itemGuid)             // cheating check 2
            return;

        eqSet.Items[i] = GUID_LOPART(itemGuid);
    }

    _player->SetEquipmentSet(index, eqSet);
}

void WorldSession::HandleEquipmentSetDelete(WorldPacket &recv_data)
{
    sLog->outDebug("CMSG_EQUIPMENT_SET_DELETE");

    uint64 setGuid;
    recv_data.readPackGUID(setGuid);

    _player->DeleteEquipmentSet(setGuid);
}

void WorldSession::HandleEquipmentSetUse(WorldPacket &recv_data)
{
    sLog->outDebug("CMSG_EQUIPMENT_SET_USE");
    recv_data.hexlike();

    for (uint32 i = 0; i < EQUIPMENT_SLOT_END; ++i)
    {
        uint64 itemGuid;
        recv_data.readPackGUID(itemGuid);

        uint8 srcbag, srcslot;
        recv_data >> srcbag >> srcslot;

        sLog->outDebug("Item " UI64FMTD ": srcbag %u, srcslot %u", itemGuid, srcbag, srcslot);

        Item *item = _player->GetItemByGuid(itemGuid);

        uint16 dstpos = i | (INVENTORY_SLOT_BAG_0 << 8);

        if (!item)
        {
            Item *uItem = _player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
            if (!uItem)
                continue;

            ItemPosCountVec sDest;
            uint8 msg = _player->CanStoreItem(NULL_BAG, NULL_SLOT, sDest, uItem, false);
            if (msg == EQUIP_ERR_OK)
            {
                _player->RemoveItem(INVENTORY_SLOT_BAG_0, i, true);
                _player->StoreItem(sDest, uItem, true);
            }
            else
                _player->SendEquipError(msg, uItem, NULL);

            continue;
        }

        if (item->GetPos() == dstpos)
            continue;

        _player->SwapItem(item->GetPos(), dstpos);
    }

    WorldPacket data(SMSG_EQUIPMENT_SET_USE_RESULT, 1);
    data << uint8(0);                                       // 4 - equipment swap failed - inventory is full
    SendPacket(&data);
}

void WorldSession::HandleCharFactionOrRaceChange(WorldPacket& recv_data)
{
    // TODO: Move queries to prepared statements
    uint64 guid;
    std::string newname;
    uint8 gender, skin, face, hairStyle, hairColor, facialHair, race;
    recv_data >> guid;
    recv_data >> newname;
    recv_data >> gender >> skin >> hairColor >> hairStyle >> facialHair >> face >> race;

    QueryResult result = CharacterDatabase.PQuery("SELECT class, level, at_login, name FROM characters WHERE guid ='%u'", GUID_LOPART(guid));
    if (!result)
    {
        WorldPacket data(SMSG_CHAR_FACTION_CHANGE, 1);
        data << uint8(CHAR_CREATE_ERROR);
        SendPacket( &data );
        return;
    }

    Field *fields = result->Fetch();
    uint32 playerClass = fields[0].GetUInt32();
    uint32 level = fields[1].GetUInt32();
    uint32 at_loginFlags = fields[2].GetUInt32();
    newname = fields[3].GetString();
    uint32 used_loginFlag = ((recv_data.GetOpcode() == CMSG_CHAR_RACE_CHANGE) ? AT_LOGIN_CHANGE_RACE : AT_LOGIN_CHANGE_FACTION);

    if (!sObjectMgr->GetPlayerInfo(race, playerClass))
    {
        WorldPacket data(SMSG_CHAR_FACTION_CHANGE, 1);
        data << uint8(CHAR_CREATE_ERROR);
        SendPacket( &data );
        return;
    }

    if (!(at_loginFlags & used_loginFlag))
    {
        WorldPacket data(SMSG_CHAR_FACTION_CHANGE, 1);
        data << uint8(CHAR_CREATE_ERROR);
        SendPacket( &data );
        return;
    }

    if (GetSecurity() == SEC_PLAYER)
    {
        uint32 raceMaskDisabled = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED_RACEMASK);
        if ((1 << (race - 1)) & raceMaskDisabled)
        {
            WorldPacket data(SMSG_CHAR_FACTION_CHANGE, 1);
            data << uint8(CHAR_CREATE_ERROR);
            SendPacket( &data );
            return;
        }
    }

    // prevent character rename to invalid name
    if (!normalizePlayerName(newname))
    {
        WorldPacket data(SMSG_CHAR_FACTION_CHANGE, 1);
        data << uint8(CHAR_NAME_NO_NAME);
        SendPacket( &data );
        return;
    }

    uint8 res = ObjectMgr::CheckPlayerName(newname,true);
    if (res != CHAR_NAME_SUCCESS)
    {
        WorldPacket data(SMSG_CHAR_FACTION_CHANGE, 1);
        data << uint8(res);
        SendPacket( &data );
        return;
    }

    // check name limitations
    if (GetSecurity() == SEC_PLAYER && sObjectMgr->IsReservedName(newname))
    {
        WorldPacket data(SMSG_CHAR_FACTION_CHANGE, 1);
        data << uint8(CHAR_NAME_RESERVED);
        SendPacket( &data );
        return;
    }

    // character with this name already exist
    if (uint64 newguid = sObjectMgr->GetPlayerGUIDByName(newname))
    {
        if (newguid != guid)
        {
            WorldPacket data(SMSG_CHAR_FACTION_CHANGE, 1);
            data << uint8(CHAR_CREATE_NAME_IN_USE);
            SendPacket( &data );
            return;
        }
    }

    CharacterDatabase.escape_string(newname);
    Player::Customize(guid, gender, skin, face, hairStyle, hairColor, facialHair);
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    trans->PAppend("UPDATE `characters` SET name = '%s', race = '%u', at_login = at_login & ~ %u WHERE guid ='%u'", newname.c_str(), race, uint32(used_loginFlag), GUID_LOPART(guid));
    trans->PAppend("DELETE FROM character_declinedname WHERE guid ='%u'", GUID_LOPART(guid));

    BattlegroundTeamId team;

    // Search each faction is targeted
    switch(race)
    {
    case RACE_ORC:
    case RACE_TAUREN:
    case RACE_UNDEAD_PLAYER:
    case RACE_TROLL:
    case RACE_BLOODELF:
    case RACE_GOBLIN:
        team = BG_TEAM_HORDE;
        break;
    default:
        team = BG_TEAM_ALLIANCE;
        break;
    }

    // Switch Languages
    // delete all languages first
    trans->PAppend("DELETE FROM `character_skills` WHERE `skill` IN (98, 113, 759, 111, 313, 109, 115, 315, 673, 137) AND `guid`='%u'", GUID_LOPART(guid));

    // now add them back
    if (team == BG_TEAM_ALLIANCE)
    {
        trans->PAppend("INSERT INTO `character_skills` (guid, skill, value, max) VALUES (%u, 98, 300, 300)", GUID_LOPART(guid));
        switch (race)
        {
        case RACE_DWARF:
            trans->PAppend("INSERT INTO `character_skills` (guid, skill, value, max) VALUES (%u, 111, 300, 300)", GUID_LOPART(guid));
            break;
        case RACE_DRAENEI:
            trans->PAppend("INSERT INTO `character_skills` (guid, skill, value, max) VALUES (%u, 759, 300, 300)", GUID_LOPART(guid));
            break;
        case RACE_GNOME:
            trans->PAppend("INSERT INTO `character_skills` (guid, skill, value, max) VALUES (%u, 313, 300, 300)", GUID_LOPART(guid));
            break;
        case RACE_NIGHTELF:
            trans->PAppend("INSERT INTO `character_skills` (guid, skill, value, max) VALUES (%u, 113, 300, 300)", GUID_LOPART(guid));
            break;
        case RACE_WORGEN:
            trans->PAppend("INSERT INTO `character_skills` (guid, skill, value, max) VALUES (%u, 791, 300, 300)", GUID_LOPART(guid));
            break;
        }
    }
    else if (team == BG_TEAM_HORDE)
    {
        trans->PAppend("INSERT INTO `character_skills` (guid, skill, value, max) VALUES (%u, 109, 300, 300)", GUID_LOPART(guid));
        switch (race)
        {
        case RACE_UNDEAD_PLAYER:
            trans->PAppend("INSERT INTO `character_skills` (guid, skill, value, max) VALUES (%u, 673, 300, 300)", GUID_LOPART(guid));
            break;
        case RACE_TAUREN:
            trans->PAppend("INSERT INTO `character_skills` (guid, skill, value, max) VALUES (%u, 115, 300, 300)", GUID_LOPART(guid));
            break;
        case RACE_TROLL:
            trans->PAppend("INSERT INTO `character_skills` (guid, skill, value, max) VALUES (%u, 315, 300, 300)", GUID_LOPART(guid));
            break;
        case RACE_BLOODELF:
            trans->PAppend("INSERT INTO `character_skills` (guid, skill, value, max) VALUES (%u, 137, 300, 300)", GUID_LOPART(guid));
            break;
        case RACE_GOBLIN:
            trans->PAppend("INSERT INTO `character_skills` (guid, skill, value, max) VALUES (%u, 792, 300, 300)", GUID_LOPART(guid));
            break;
        }
    }

    if(recv_data.GetOpcode() == CMSG_CHAR_FACTION_CHANGE)
    {
        // Delete all Flypaths
        trans->PAppend("UPDATE `characters` SET taxi_path = '' WHERE guid ='%u'",GUID_LOPART(guid));

        if (level > 7)
        {
            // Update Taxi path
            // this doesn't seem to be 100% blizzlike... but it can't really be helped.
            std::ostringstream taximaskstream;
            uint32 numFullTaximasks = level / 7;
            if (numFullTaximasks > 11)
                numFullTaximasks = 11;
            if (team == BG_TEAM_ALLIANCE)
            {
                if (playerClass != CLASS_DEATH_KNIGHT)
                {
                    for (uint8 i = 0; i < numFullTaximasks; ++i)
                        taximaskstream << uint32(sAllianceTaxiNodesMask[i]) << " ";
                }
                else
                {
                    for (uint8 i = 0; i < numFullTaximasks; ++i)
                        taximaskstream << uint32(sAllianceTaxiNodesMask[i] | sDeathKnightTaxiNodesMask[i]) << " ";
                }
            }
            else
            {
                if (playerClass != CLASS_DEATH_KNIGHT)
                {
                    for (uint8 i = 0; i < numFullTaximasks; ++i)
                        taximaskstream << uint32(sHordeTaxiNodesMask[i]) << " ";
                }
                else
                {
                    for (uint8 i = 0; i < numFullTaximasks; ++i)
                        taximaskstream << uint32(sHordeTaxiNodesMask[i] | sDeathKnightTaxiNodesMask[i]) << " ";
                }
            }

            uint32 numEmptyTaximasks = 11 - numFullTaximasks;
            for (uint8 i = 0; i < numEmptyTaximasks; ++i)
                taximaskstream << "0 ";
            taximaskstream << "0";
            std::string taximask = taximaskstream.str();
            trans->PAppend("UPDATE `characters` SET `taximask`= '%s' WHERE `guid` = '%u'", taximask.c_str(), GUID_LOPART(guid));
        }

        // Delete all current quests
        trans->PAppend("DELETE FROM `character_queststatus` WHERE `status` = 3 AND guid ='%u'",GUID_LOPART(guid));

        // Delete record of the faction old completed quests
        {
            std::ostringstream quests;
            ObjectMgr::QuestMap const& qTemplates = sObjectMgr->GetQuestTemplates();
            for (ObjectMgr::QuestMap::const_iterator iter = qTemplates.begin(); iter != qTemplates.end(); ++iter)
            {
                Quest *qinfo = iter->second;
                uint32 requiredRaces = qinfo->GetRequiredRaces();
                if (team == BG_TEAM_ALLIANCE)
                {
                    if (requiredRaces & RACEMASK_ALLIANCE)
                    {
                        quests << uint32(qinfo->GetQuestId());
                        quests << ",";
                    }

                    // Transaction for Portal quests
                    trans->PAppend("UPDATE character_queststatus SET quest = '262450' WHERE guid = '%u' AND quest = '26245'", GUID_LOPART(guid)); // Deepholm
                    trans->PAppend("UPDATE character_queststatus SET quest = '27537' WHERE guid = '%u' AND quest = '26784'", GUID_LOPART(guid)); // Twilight Highlands
                    trans->PAppend("UPDATE character_queststatus SET quest = '14482' WHERE guid = '%u' AND quest = '25924'", GUID_LOPART(guid)); // Vashj'ir
                }
                else // if (team == BG_TEAM_HORDE)
                {
                    if (requiredRaces & RACEMASK_HORDE)
                    {
                        quests << uint32(qinfo->GetQuestId());
                        quests << ",";
                    }

                    // Transaction for Portal quests
                    trans->PAppend("UPDATE character_queststatus SET quest = '26245' WHERE guid = '%u' AND quest = '262450'", GUID_LOPART(guid)); // Deepholm
                    trans->PAppend("UPDATE character_queststatus SET quest = '26784' WHERE guid = '%u' AND quest = '27537'", GUID_LOPART(guid)); // Twilight Highlands
                    trans->PAppend("UPDATE character_queststatus SET quest = '25924' WHERE guid = '%u' AND quest = '14482'", GUID_LOPART(guid)); // Vashj'ir
                }
            }

            std::string questsStr = quests.str();
            questsStr = questsStr.substr(0, questsStr.length() - 1);

            trans->PAppend("DELETE FROM `character_queststatus` WHERE guid= '%u' AND quest IN (%s)",GUID_LOPART(guid),questsStr.c_str());
        }

        if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD))
        {
            // Reset guild
            trans->PAppend("DELETE FROM `guild_member` WHERE `guid`= '%u'",GUID_LOPART(guid));
        }

        if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_ADD_FRIEND))
        {
            // Delete Friend List
            trans->PAppend("DELETE FROM `character_social` WHERE `guid`= '%u'",GUID_LOPART(guid));
            trans->PAppend("DELETE FROM `character_social` WHERE `friend`= '%u'",GUID_LOPART(guid));
        }

        // Leave Arena Teams
        Player::LeaveAllArenaTeams(GUID_LOPART(guid));

        // Reset homebind and position
        trans->PAppend("DELETE FROM `character_homebind` WHERE guid = '%u'",GUID_LOPART(guid));
        if(team == BG_TEAM_ALLIANCE)
        {
            trans->PAppend("INSERT INTO `character_homebind` VALUES (%u,0,1519,-8867.68,673.373,97.9034)",GUID_LOPART(guid));
            Player::SavePositionInDB(0, -8867.68f, 673.373f, 97.9034f, 0.0f, 1519, GUID_LOPART(guid));

            if (race == RACE_WORGEN) // for Worgen
            {
                trans->PAppend("INSERT INTO `character_homebind` VALUES (%u,654,4714,-1451.53,1403.35,35.5561)",GUID_LOPART(guid));
                Player::SavePositionInDB(654, -1451.53f, 1403.35f, 35.5561f, 0.0f, 4714, GUID_LOPART(guid));
            }
        }
        else
        {
            trans->PAppend("INSERT INTO `character_homebind` VALUES (%u,1,1637,1633.33,-4439.11,15.7588)",GUID_LOPART(guid));
            Player::SavePositionInDB(1, 1633.33f, -4439.11f, 15.7588f, 0.0f, 1637, GUID_LOPART(guid));

            if (race  == RACE_GOBLIN) // for Goblin
            {
                trans->PAppend("INSERT INTO `character_homebind` VALUES (%u,648,4737,-8423.81,1361.3,104.671)",GUID_LOPART(guid));
                Player::SavePositionInDB(648, -8423.81f, 1361.3f, 104.671f, 0.0f, 4737, GUID_LOPART(guid));
            }
        }

        // Achievement conversion
        for (std::map<uint32, uint32>::const_iterator it = sObjectMgr->factionchange_achievements.begin(); it != sObjectMgr->factionchange_achievements.end(); ++it)
        {
            uint32 achiev_alliance = it->first;
            uint32 achiev_horde = it->second;
            trans->PAppend("DELETE FROM `character_achievement` WHERE `achievement`=%u AND `guid`=%u",
                            team == BG_TEAM_ALLIANCE ? achiev_alliance : achiev_horde, GUID_LOPART(guid));
            trans->PAppend("UPDATE `character_achievement` SET achievement = '%u' where achievement = '%u' AND guid = '%u'",
                team == BG_TEAM_ALLIANCE ? achiev_alliance : achiev_horde, team == BG_TEAM_ALLIANCE ? achiev_horde : achiev_alliance, GUID_LOPART(guid));
        }

        // Item conversion
        for (std::map<uint32, uint32>::const_iterator it = sObjectMgr->factionchange_items.begin(); it != sObjectMgr->factionchange_items.end(); ++it)
        {
            uint32 item_alliance = it->first;
            uint32 item_horde = it->second;
            trans->PAppend("UPDATE `item_instance` ii, `character_inventory` ci SET ii.itemEntry = '%u' WHERE ii.itemEntry = '%u' AND ci.guid = '%u' AND ci.item = ii.guid",
                team == BG_TEAM_ALLIANCE ? item_alliance : item_horde, team == BG_TEAM_ALLIANCE ? item_horde : item_alliance, guid);
        }

        // Spell conversion
        for (std::map<uint32, uint32>::const_iterator it = sObjectMgr->factionchange_spells.begin(); it != sObjectMgr->factionchange_spells.end(); ++it)
        {
            uint32 spell_alliance = it->first;
            uint32 spell_horde = it->second;
            trans->PAppend("DELETE FROM `character_spell` WHERE `spell`=%u AND `guid`=%u",
                            team == BG_TEAM_ALLIANCE ? spell_alliance : spell_horde, GUID_LOPART(guid));
            trans->PAppend("UPDATE `character_spell` SET spell = '%u' where spell = '%u' AND guid = '%u'",
                team == BG_TEAM_ALLIANCE ? spell_alliance : spell_horde, team == BG_TEAM_ALLIANCE ? spell_horde : spell_alliance, GUID_LOPART(guid));
        }

        // Reputation conversion
        for (std::map<uint32, uint32>::const_iterator it = sObjectMgr->factionchange_reputations.begin(); it != sObjectMgr->factionchange_reputations.end(); ++it)
        {
            uint32 reputation_alliance = it->first;
            uint32 reputation_horde = it->second;
            trans->PAppend("DELETE FROM character_reputation WHERE faction = '%u' AND guid = '%u'",
                team == BG_TEAM_ALLIANCE ? reputation_alliance : reputation_horde, GUID_LOPART(guid));
            trans->PAppend("UPDATE `character_reputation` SET faction = '%u' where faction = '%u' AND guid = '%u'",
                team == BG_TEAM_ALLIANCE ? reputation_alliance : reputation_horde, team == BG_TEAM_ALLIANCE ? reputation_horde : reputation_alliance, GUID_LOPART(guid));
        }
    }

    CharacterDatabase.CommitTransaction(trans);

    std::string IP_str = GetRemoteAddress();
    sLog->outDebug("Account: %d (IP: %s), Character guid: %u Change Race/Faction to: %s", GetAccountId(), IP_str.c_str(), GUID_LOPART(guid), newname.c_str());

    sWorld->UpdateCharacterNameData(GUID_LOPART(guid), newname, gender, race);

    WorldPacket data(SMSG_CHAR_FACTION_CHANGE, 1 + 8 + (newname.size() + 1) + 1 + 1 + 1 + 1 + 1 + 1 + 1);
    data << uint8(RESPONSE_SUCCESS);
    data << uint64(guid);
    data << newname;
    data << uint8(gender);
    data << uint8(skin);
    data << uint8(face);
    data << uint8(hairStyle);
    data << uint8(hairColor);
    data << uint8(facialHair);
    data << uint8(race);
    SendPacket(&data);
}

void WorldSession::HandleOpeningCinematic(WorldPacket& /*recvData*/)
{
    Player* player = GetPlayer();
    // Only players that has not yet gained any experience can use this
    if (player->GetUInt32Value(PLAYER_XP))
        return;

    if (ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(player->getClass()))
    {
        if (classEntry->CinematicSequence)
            player->SendCinematicStart(classEntry->CinematicSequence);
        else if (ChrRacesEntry const* raceEntry = sChrRacesStore.LookupEntry(player->getRace()))
            player->SendCinematicStart(raceEntry->CinematicSequence);
    }
}

void WorldSession::HandleInstanceLockResponse(WorldPacket &recv_data)
{
    sLog->outDebug("CMSG_INSTANCE_LOCK_WARNING_RESPONSE");
    Player* player = GetPlayer();
    Map* map = player->GetMap();
    if(!map || !player)
        return;
    uint8 respond;
    recv_data >> respond;
    if(respond == 0) //Leave instance button
    {
        player->RepopAtGraveyard();
    }
    else //Accept
    {
        //if heroic, bind immediatelly
        if(player && map && (map->isHeroicRaid()) && map->ToInstanceMap())
        {
            map->ToInstanceMap()->copyDeadUnitsFromLeader(player, player->GetMapId(), map->GetInstanceId(),0);
            map->ToInstanceMap()->removePlayerSaveTimer(player->GetGUIDLow()); //remove automatic save timer (because just saved)
            uint32 mapId = map->GetId();
            uint32 instanceId = player->getRaidId(mapId);
            if(player->getRaidDiffProgr(mapId) < KILLED_HC && instanceId)
                CharacterDatabase.PExecute("UPDATE character_instance SET diffProgress = '%u' where guid = '%u' AND instance = '%u'", KILLED_HC, player->GetGUIDLow(), instanceId);
        }
    }
}
