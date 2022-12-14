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
#include "ObjectMgr.h"
#include "WorldPacket.h"
#include "BattlegroundMgr.h"
#include "ArenaTeam.h"
#include "World.h"

void ArenaTeamMember::ModifyPersonalRating(Player* plr, int32 mod, uint32 slot)
{
    if (int32(personal_rating) + mod < 0)
        personal_rating = 0;
    else
        personal_rating += mod;
    if (plr)
    {
        CharacterDatabase.PExecute("INSERT INTO arena_log_personal VALUES ('%u', '%s', '%d', '%d', '%s', NOW())",
                                     plr->GetGUIDLow(),
                                     plr->GetName(),
                                     int32(personal_rating),
                                     mod,
                                     plr->GetSession() ? plr->GetSession()->GetRemoteAddress().c_str() : "none");
        plr->SetArenaTeamInfoField(slot, ARENA_TEAM_PERSONAL_RATING, personal_rating);
    }

    if (personal_rating > highest_week_rating)
        highest_week_rating = personal_rating;
}

void ArenaTeamMember::ModifyMatchmakerRating(int32 mod, uint32 /*slot*/)
{
    if (int32(matchmaker_rating) + mod < 0)
        matchmaker_rating = 0;
    else
        matchmaker_rating += mod;
}


void ArenaTeamMember::SetMatchmakerRating(int32 value/*, uint32 slot*/)
{
    matchmaker_rating = value;
}

ArenaTeam::ArenaTeam()
{
    m_TeamId              = 0;
    m_Type                = 0;
    m_Name                = "";
    m_CaptainGuid         = 0;
    m_BackgroundColor     = 0;                              // background
    m_EmblemStyle         = 0;                              // icon
    m_EmblemColor         = 0;                              // icon color
    m_BorderStyle         = 0;                              // border
    m_BorderColor         = 0;                              // border color
    m_stats.games_week    = 0;
    m_stats.games_season  = 0;
    m_stats.rank          = 0;
    m_stats.rating        = sWorld->getIntConfig(CONFIG_ARENA_START_RATING);
    m_stats.wins_week     = 0;
    m_stats.wins_season   = 0;
}

ArenaTeam::~ArenaTeam()
{
}

bool ArenaTeam::Create(Player *captain, uint32 type, std::string ArenaTeamName)
{
    uint64 captainGuid = captain->GetGUID();

    if (sObjectMgr->GetArenaTeamByName(ArenaTeamName))            // arena team with this name already exist
    {
        captain->GetSession()->SendArenaTeamCommandResult(ERR_ARENA_TEAM_INVITE_SS, "", captain->GetName(), ERR_ARENA_TEAM_NAME_EXISTS_S);
        return false;
    }

    uint32 captainLowGuid = GUID_LOPART(captainGuid);
    sLog->outDebug("GUILD: creating arena team %s to leader: %u", ArenaTeamName.c_str(), captainLowGuid);

    m_CaptainGuid = captainGuid;
    m_Name = ArenaTeamName;
    m_Type = type;

    m_TeamId = sObjectMgr->GenerateArenaTeamId();

    // ArenaTeamName already assigned to ArenaTeam::name, use it to encode string for DB
    CharacterDatabase.escape_string(ArenaTeamName);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    // CharacterDatabase.PExecute("DELETE FROM arena_team WHERE arenateamid='%u'", m_TeamId); - MAX(arenateam)+1 not exist
    trans->PAppend("DELETE FROM arena_team_member WHERE arenateamid='%u'", m_TeamId);
    trans->PAppend("INSERT INTO arena_team (arenateamid,name,captainguid,type,BackgroundColor,EmblemStyle,EmblemColor,BorderStyle,BorderColor) "
        "VALUES('%u','%s','%u','%u','%u','%u','%u','%u','%u')",
        m_TeamId, ArenaTeamName.c_str(), captainLowGuid, m_Type, m_BackgroundColor, m_EmblemStyle, m_EmblemColor, m_BorderStyle, m_BorderColor);
    trans->PAppend("INSERT INTO arena_team_stats (arenateamid, rating, games, wins, played, wins2, rank) VALUES "
        "('%u', '%u', '%u', '%u', '%u', '%u', '%u')", m_TeamId, m_stats.rating, m_stats.games_week, m_stats.wins_week, m_stats.games_season, m_stats.wins_season, m_stats.rank);

    CharacterDatabase.CommitTransaction(trans);

    AddMember(m_CaptainGuid);
    sLog->outArena("New ArenaTeam created [Id: %u] [Type: %u] [Captain low GUID: %u]", GetId(), GetType(), captainLowGuid);
    return true;
}

bool ArenaTeam::AddMember(const uint64& PlayerGuid)
{
    std::string plName;
    uint8 plClass;
    uint32 plPRating;
    uint32 plMMRating;
    uint32 plPCap;

    // arena team is full (can't have more than type * 2 players!)
    if (GetMembersSize() >= GetType() * 2)
        return false;

    Player *pl = sObjectMgr->GetPlayer(PlayerGuid);
    if (pl)
    {
        if (pl->GetArenaTeamId(GetSlot()))
        {
            sLog->outError("Arena::AddMember() : player already in this sized team");
            return false;
        }

        plClass = pl->getClass();
        plName = pl->GetName();
    }
    else
    {
        //                                                     0     1
        QueryResult result = CharacterDatabase.PQuery("SELECT name, class FROM characters WHERE guid='%u'", GUID_LOPART(PlayerGuid));
        if (!result)
            return false;

        plName = (*result)[0].GetString();
        plClass = (*result)[1].GetUInt8();

        // check if player already in arenateam of that size
        if (Player::GetArenaTeamIdFromDB(PlayerGuid, GetType()) != 0)
        {
            sLog->outError("Arena::AddMember() : player already in this sized team");
            return false;
        }
    }

    plMMRating = sWorld->getIntConfig(CONFIG_ARENA_START_MATCHMAKER_RATING);
    plPRating = 0;
    plPCap = 1350;

    if (sWorld->getIntConfig(CONFIG_ARENA_START_PERSONAL_RATING) > 0)
        plPRating = sWorld->getIntConfig(CONFIG_ARENA_START_PERSONAL_RATING);
    else if (GetTeamRating() >= 1000)
        plPRating = 1000;

    sWorld->getIntConfig(CONFIG_ARENA_START_PERSONAL_RATING);

    QueryResult result = CharacterDatabase.PQuery("SELECT matchmaker_rating FROM character_arena_stats WHERE guid='%u' AND slot='%u'", GUID_LOPART(PlayerGuid), GetSlot());
    if (result)
        plMMRating = (*result)[0].GetUInt32();

    QueryResult capresult = CharacterDatabase.PQuery("SELECT cap FROM character_currency_weekcap WHERE guid = '%u' AND currency = '%u' AND source = '%u';", GUID_LOPART(PlayerGuid), CURRENCY_TYPE_CONQUEST_POINTS, CURRENCY_SOURCE_ARENA);
    if (capresult)
        plPCap = (*capresult)[0].GetUInt32() / GetCurrencyPrecision(CURRENCY_TYPE_CONQUEST_POINTS);

    // remove all player signs from another petitions
    // this will be prevent attempt joining player to many arenateams and corrupt arena team data integrity
    Player::RemovePetitionsAndSigns(PlayerGuid, GetType());

    ArenaTeamMember newmember;
    newmember.name              = plName;
    newmember.playerGuid        = PlayerGuid;
    newmember.Class             = plClass;
    newmember.games_season      = 0;
    newmember.games_week        = 0;
    newmember.wins_season       = 0;
    newmember.wins_week         = 0;
    newmember.personal_rating   = plPRating;
    newmember.matchmaker_rating = plMMRating;
    newmember.highest_week_rating = plPRating; //same as personal rating
    newmember.conquest_point_cap = plPCap;

    m_members.push_back(newmember);

    CharacterDatabase.PExecute("INSERT INTO arena_team_member (arenateamid, guid) VALUES ('%u', '%u')", m_TeamId, GUID_LOPART(newmember.playerGuid));

    if (pl)
    {
        pl->SetInArenaTeam(m_TeamId, GetSlot(), GetType());
        pl->SetArenaTeamIdInvited(0);

        // hide promote/remove buttons
        if (m_CaptainGuid != PlayerGuid)
            pl->SetArenaTeamInfoField(GetSlot(), ARENA_TEAM_MEMBER, 1);
        sLog->outArena("Player: %s [GUID: %u] joined arena team type: %u [Id: %u].", pl->GetName(), pl->GetGUIDLow(), GetType(), GetId());
    }
    return true;
}

bool ArenaTeam::LoadArenaTeamFromDB(QueryResult arenaTeamDataResult)
{
    if (!arenaTeamDataResult)
        return false;

    Field *fields = arenaTeamDataResult->Fetch();

    m_TeamId             = fields[0].GetUInt32();
    m_Name               = fields[1].GetString();
    m_CaptainGuid        = MAKE_NEW_GUID(fields[2].GetUInt32(), 0, HIGHGUID_PLAYER);
    m_Type               = fields[3].GetUInt32();
    m_BackgroundColor    = fields[4].GetUInt32();
    m_EmblemStyle        = fields[5].GetUInt32();
    m_EmblemColor        = fields[6].GetUInt32();
    m_BorderStyle        = fields[7].GetUInt32();
    m_BorderColor        = fields[8].GetUInt32();
    //load team stats
    m_stats.rating       = fields[9].GetUInt32();
    m_stats.games_week   = fields[10].GetUInt32();
    m_stats.wins_week    = fields[11].GetUInt32();
    m_stats.games_season = fields[12].GetUInt32();
    m_stats.wins_season  = fields[13].GetUInt32();
    m_stats.rank         = fields[14].GetUInt32();

    return true;
}

bool ArenaTeam::LoadMembersFromDB(QueryResult arenaTeamMembersResult)
{
    if (!arenaTeamMembersResult)
        return false;

    bool captainPresentInTeam = false;

    do
    {
        Field *fields = arenaTeamMembersResult->Fetch();
        //prevent crash if db records are broken, when all members in result are already processed and current team hasn't got any members
        if (!fields)
            break;
        uint32 arenaTeamId        = fields[0].GetUInt32();
        if (arenaTeamId < m_TeamId)
        {
            //there is in table arena_team_member record which doesn't have arenateamid in arena_team table, report error
            sLog->outErrorDb("ArenaTeam %u does not exist but it has record in arena_team_member table, deleting it!", arenaTeamId);
            CharacterDatabase.PExecute("DELETE FROM arena_team_member WHERE arenateamid = '%u'", arenaTeamId);
            continue;
        }

        if (arenaTeamId > m_TeamId)
            //we loaded all members for this arena_team already, break cycle
            break;

        uint32 player_guid = fields[1].GetUInt32();

        QueryResult result = CharacterDatabase.PQuery(
            "SELECT personal_rating, matchmaker_rating, highest_week_rating FROM character_arena_stats WHERE guid = '%u' AND slot = '%u'", player_guid, GetSlot());

        QueryResult capresult = CharacterDatabase.PQuery("SELECT cap FROM character_currency_weekcap WHERE guid = '%u' AND currency = '%u' AND source = '%u';", player_guid, CURRENCY_TYPE_CONQUEST_POINTS, CURRENCY_SOURCE_ARENA);

        uint32 personalrating = 0;
        uint32 matchmakerrating = 1500;
        uint32 highestweekrating = 0;

        uint32 conquestpointcap = 1350;

        if (result)
        {
            personalrating = (*result)[0].GetUInt32();
            matchmakerrating = (*result)[1].GetUInt32();
            highestweekrating = (*result)[2].GetUInt32();
        }

        if (capresult)
            conquestpointcap = (*capresult)[0].GetUInt32() / GetCurrencyPrecision(CURRENCY_TYPE_CONQUEST_POINTS);

        ArenaTeamMember newmember;
        newmember.playerGuid        = MAKE_NEW_GUID(player_guid, 0, HIGHGUID_PLAYER);
        newmember.games_week        = fields[2].GetUInt32();
        newmember.wins_week         = fields[3].GetUInt32();
        newmember.games_season      = fields[4].GetUInt32();
        newmember.wins_season       = fields[5].GetUInt32();
        newmember.name              = fields[6].GetString();
        newmember.Class             = fields[7].GetUInt8();
        newmember.personal_rating   = personalrating;
        newmember.matchmaker_rating = matchmakerrating;
        newmember.highest_week_rating = highestweekrating;
        newmember.conquest_point_cap = conquestpointcap;

        //check if member exists in characters table
        if (newmember.name.empty())
        {
            sLog->outErrorDb("ArenaTeam %u has member with empty name - probably player %u doesn't exist, deleting him from memberlist!", arenaTeamId, GUID_LOPART(newmember.playerGuid));
            this->DelMember(newmember.playerGuid);
            continue;
        }

        if (newmember.playerGuid == GetCaptain())
            captainPresentInTeam = true;

        m_members.push_back(newmember);
    }while (arenaTeamMembersResult->NextRow());

    if (Empty() || !captainPresentInTeam)
    {
        // arena team is empty or captain is not in team, delete from db
        sLog->outErrorDb("ArenaTeam %u does not have any members or its captain is not in team, disbanding it...", m_TeamId);
        return false;
    }

    return true;
}

void ArenaTeam::SetCaptain(const uint64& guid)
{
    // disable remove/promote buttons
    Player *oldcaptain = sObjectMgr->GetPlayer(GetCaptain());
    if (oldcaptain)
        oldcaptain->SetArenaTeamInfoField(GetSlot(), ARENA_TEAM_MEMBER, 1);

    // set new captain
    m_CaptainGuid = guid;

    // update database
    CharacterDatabase.PExecute("UPDATE arena_team SET captainguid = '%u' WHERE arenateamid = '%u'", GUID_LOPART(guid), GetId());

    // enable remove/promote buttons
    Player *newcaptain = sObjectMgr->GetPlayer(guid);
    if (newcaptain)
    {
        newcaptain->SetArenaTeamInfoField(GetSlot(), ARENA_TEAM_MEMBER, 0);
        sLog->outArena("Player: %s [GUID: %u] promoted player: %s [GUID: %u] to leader of arena team [Id: %u] [Type: %u].", oldcaptain->GetName(), oldcaptain->GetGUIDLow(), newcaptain->GetName(), newcaptain->GetGUIDLow(), GetId(), GetType());
    }
}

void ArenaTeam::DelMember(uint64 guid)
{
    for (MemberList::iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        if (itr->playerGuid == guid)
        {
            m_members.erase(itr);
            break;
        }

    if (Player *player = sObjectMgr->GetPlayer(guid))
    {
        player->GetSession()->SendArenaTeamCommandResult(ERR_ARENA_TEAM_QUIT_S, GetName(), "", 0);
        // delete all info regarding this team
        for (uint32 i = 0; i < ARENA_TEAM_END; ++i)
            player->SetArenaTeamInfoField(GetSlot(), ArenaTeamInfoType(i), 0);
        sLog->outArena("Player: %s [GUID: %u] left arena team type: %u [Id: %u].", player->GetName(), player->GetGUIDLow(), GetType(), GetId());
    }
    CharacterDatabase.PExecute("DELETE FROM arena_team_member WHERE arenateamid = '%u' AND guid = '%u'", GetId(), GUID_LOPART(guid));
}

void ArenaTeam::Disband(WorldSession *session)
{
    // event
    if (session)
        // probably only 1 string required...
        BroadcastEvent(ERR_ARENA_TEAM_DISBANDED_S, 0, 2, session->GetPlayerName(), GetName(), "");

    while (!m_members.empty())
        // Removing from members is done in DelMember.
        DelMember(m_members.front().playerGuid);

    if (session)
        if (Player *player = session->GetPlayer())
            sLog->outArena("Player: %s [GUID: %u] disbanded arena team type: %u [Id: %u].", player->GetName(), player->GetGUIDLow(), GetType(), GetId());

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    trans->PAppend("DELETE FROM arena_team WHERE arenateamid = '%u'", m_TeamId);
    trans->PAppend("DELETE FROM arena_team_member WHERE arenateamid = '%u'", m_TeamId); //< this should be alredy done by calling DelMember(memberGuids[j]); for each member
    trans->PAppend("DELETE FROM arena_team_stats WHERE arenateamid = '%u'", m_TeamId);
    CharacterDatabase.CommitTransaction(trans);
    sObjectMgr->RemoveArenaTeam(m_TeamId);
}

void ArenaTeam::Roster(WorldSession *session)
{
    Player *pl = NULL;
    uint8 unk308 = 0;

    WorldPacket data(SMSG_ARENA_TEAM_ROSTER, 100);
    data << uint32(GetId());                                // team id
    data << uint8(unk308);                                  // 308 unknown value but affect packet structure
    data << uint32(GetMembersSize());                       // members count
    data << uint32(GetType());                              // arena team type?

    for (MemberList::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
    {
        pl = sObjectMgr->GetPlayer(itr->playerGuid);

        data << uint64(itr->playerGuid);
        data << uint8((pl ? 1 : 0));                        // online flag
        data << itr->name;                                  // member name
        data << uint32((itr->playerGuid == GetCaptain() ? 0 : 1));  // captain flag 0 captain 1 member
        data << uint8((pl ? pl->getLevel() : 0));           // unknown, level?
        data << uint8(itr->Class);                          // class
        data << uint32(itr->games_week);                    // played this week
        data << uint32(itr->wins_week);                     // wins this week
        data << uint32(itr->games_season);                  // played this season
        data << uint32(itr->wins_season);                   // wins this season
        data << uint32(itr->personal_rating);               // personal rating
        if (unk308)
        {
            data << float(0.0);                             // 308 unk
            data << float(0.0);                             // 308 unk
        }
    }

    session->SendPacket(&data);
    sLog->outDebug("WORLD: Sent SMSG_ARENA_TEAM_ROSTER");
}

void ArenaTeam::Query(WorldSession *session)
{
    WorldPacket data(SMSG_ARENA_TEAM_QUERY_RESPONSE, 4*7+GetName().size()+1);
    data << uint32(GetId());                                // team id
    data << GetName();                                      // team name
    data << uint32(GetType());                              // arena team type (2=2x2, 3=3x3 or 5=5x5)
    data << uint32(m_BackgroundColor);                      // background color
    data << uint32(m_EmblemStyle);                          // emblem style
    data << uint32(m_EmblemColor);                          // emblem color
    data << uint32(m_BorderStyle);                          // border style
    data << uint32(m_BorderColor);                          // border color
    session->SendPacket(&data);
    sLog->outDebug("WORLD: Sent SMSG_ARENA_TEAM_QUERY_RESPONSE");
}

void ArenaTeam::Stats(WorldSession *session)
{
    WorldPacket data(SMSG_ARENA_TEAM_STATS, 4*7);
    data << uint32(GetId());                                // team id
    data << uint32(m_stats.rating);                         // rating
    data << uint32(m_stats.games_week);                     // games this week
    data << uint32(m_stats.wins_week);                      // wins this week
    data << uint32(m_stats.games_season);                   // played this season
    data << uint32(m_stats.wins_season);                    // wins this season
    data << uint32(m_stats.rank);                           // rank
    session->SendPacket(&data);
}

void ArenaTeam::NotifyStatsChanged()
{
    // this is called after a rated match ended
    // updates arena team stats for every member of the team (not only the ones who participated!)
    for (MemberList::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
    {
        Player * plr = sObjectMgr->GetPlayer(itr->playerGuid);
        if (plr)
            Stats(plr->GetSession());
    }
}

void ArenaTeam::InspectStats(WorldSession *session, uint64 guid)
{
    ArenaTeamMember* member = GetMember(guid);
    if (!member)
        return;

    WorldPacket data(MSG_INSPECT_ARENA_TEAMS, 8+1+4*6);
    data << uint64(guid);                                   // player guid
    data << uint8(GetSlot());                               // slot (0...2)
    data << uint32(GetId());                                // arena team id
    data << uint32(m_stats.rating);                         // rating
    data << uint32(m_stats.games_season);                   // season played
    data << uint32(m_stats.wins_season);                    // season wins
    data << uint32(member->games_season);                   // played (count of all games, that the inspected member participated...)
    data << uint32(member->personal_rating);                // personal rating
    session->SendPacket(&data);
}

void ArenaTeam::SetEmblem(uint32 backgroundColor, uint32 emblemStyle, uint32 emblemColor, uint32 borderStyle, uint32 borderColor)
{
    m_BackgroundColor = backgroundColor;
    m_EmblemStyle = emblemStyle;
    m_EmblemColor = emblemColor;
    m_BorderStyle = borderStyle;
    m_BorderColor = borderColor;

    CharacterDatabase.PExecute("UPDATE arena_team SET BackgroundColor='%u', EmblemStyle='%u', EmblemColor='%u', BorderStyle='%u', BorderColor='%u' WHERE arenateamid='%u'", m_BackgroundColor, m_EmblemStyle, m_EmblemColor, m_BorderStyle, m_BorderColor, m_TeamId);
}

void ArenaTeam::SetStats(uint32 stat_type, uint32 value)
{
    switch(stat_type)
    {
        case STAT_TYPE_RATING:
            m_stats.rating = value;
            CharacterDatabase.PExecute("UPDATE arena_team_stats SET rating = '%u' WHERE arenateamid = '%u'", value, GetId());
            break;
        case STAT_TYPE_GAMES_WEEK:
            m_stats.games_week = value;
            CharacterDatabase.PExecute("UPDATE arena_team_stats SET games = '%u' WHERE arenateamid = '%u'", value, GetId());
            break;
        case STAT_TYPE_WINS_WEEK:
            m_stats.wins_week = value;
            CharacterDatabase.PExecute("UPDATE arena_team_stats SET wins = '%u' WHERE arenateamid = '%u'", value, GetId());
            break;
        case STAT_TYPE_GAMES_SEASON:
            m_stats.games_season = value;
            CharacterDatabase.PExecute("UPDATE arena_team_stats SET played = '%u' WHERE arenateamid = '%u'", value, GetId());
            break;
        case STAT_TYPE_WINS_SEASON:
            m_stats.wins_season = value;
            CharacterDatabase.PExecute("UPDATE arena_team_stats SET wins2 = '%u' WHERE arenateamid = '%u'", value, GetId());
            break;
        case STAT_TYPE_RANK:
            m_stats.rank = value;
            CharacterDatabase.PExecute("UPDATE arena_team_stats SET rank = '%u' WHERE arenateamid = '%u'", value, GetId());
            break;
        default:
            sLog->outDebug("unknown stat type in ArenaTeam::SetStats() %u", stat_type);
            break;
    }
}

void ArenaTeam::BroadcastPacket(WorldPacket *packet)
{
    for (MemberList::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
    {
        Player *player = sObjectMgr->GetPlayer(itr->playerGuid);
        if (player)
            player->GetSession()->SendPacket(packet);
    }
}

void ArenaTeam::BroadcastEvent(ArenaTeamEvents event, uint64 guid, uint8 strCount, std::string str1, std::string str2, std::string str3)
{
    WorldPacket data(SMSG_ARENA_TEAM_EVENT, 1+1+1);
    data << uint8(event);
    data << uint8(strCount);
    switch (strCount)
    {
        case 0:
            break;
        case 1:
            data << str1;
            break;
        case 2:
            data << str1 << str2;
            break;
        case 3:
            data << str1 << str2 << str3;
            break;
        default:
            sLog->outError("Unhandled strCount %u in ArenaTeam::BroadcastEvent", strCount);
            return;
    }

    if (guid)
        data << uint64(guid);

    BroadcastPacket(&data);

    sLog->outDebug("WORLD: Sent SMSG_ARENA_TEAM_EVENT");
}

void ArenaTeam::MassInviteToEvent(WorldSession* session)
{
    WorldPacket data(SMSG_CALENDAR_ARENA_TEAM, (m_members.size() - 1) * (4 + 8 + 1));
    data << uint32(m_members.size() - 1);

    for (MemberList::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
    {
        if (itr->playerGuid != session->GetPlayer()->GetGUID())
        {
            data.appendPackGUID(itr->playerGuid);
            data << uint8(0); // unk
        }
    }

    session->SendPacket(&data);
}

uint8 ArenaTeam::GetSlotByType(uint32 type)
{
    switch(type)
    {
        case ARENA_TEAM_2v2: return 0;
        case ARENA_TEAM_3v3: return 1;
        case ARENA_TEAM_5v5: return 2;
        default:
            break;
    }
    sLog->outError("FATAL: Unknown arena team type %u for some arena team", type);
    return 0xFF;
}

uint8 ArenaTeam::GetTypeBySlot(uint8 slot)
{
    switch (slot)
    {
        case 0: return ARENA_TEAM_2v2;
        case 1: return ARENA_TEAM_3v3;
        case 2: return ARENA_TEAM_5v5;
        default:
            break;
    }
    sLog->outError("FATAL: Unknown arena team slot %u for some arena team", slot);
    return 0xFF;
}

bool ArenaTeam::HaveMember(const uint64& guid) const
{
    for (MemberList::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        if (itr->playerGuid == guid)
            return true;

    return false;
}

uint32 ArenaTeam::GetPoints(uint32 MemberRating)
{
    // returns how many points would be awarded with this team type with this rating
    float points;

    uint32 rating = MemberRating + 150 < m_stats.rating ? MemberRating : m_stats.rating;

    if (rating <= 1500)
    {
        if (sWorld->getIntConfig(CONFIG_ARENA_SEASON_ID) < 6)
            points = (float)rating * 0.22f + 14.0f;
        else
            points = 344;
    }
    else
        points = 1511.26f / (1.0f + 1639.28f * exp(-0.00412f * (float)rating));

    // type penalties for <5v5 teams
    if  (m_Type == ARENA_TEAM_2v2)
        points *= 0.76f;
    else if (m_Type == ARENA_TEAM_3v3)
        points *= 0.88f;

    return (uint32) points;
}

uint32 ArenaTeam::GetAverageMMR(Group *group) const
{
    if (!group) //should never happen
        return 0;

    uint32 matchmakerrating = 0;
    uint32 player_divider = 0;
    for (MemberList::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
    {
        // If player not online
        if (!ObjectAccessor::FindPlayer(itr->playerGuid))
            continue;

        // If not in group
        if (!group->IsMember(itr->playerGuid))
            continue;

        matchmakerrating += itr->matchmaker_rating;
        ++player_divider;
    }

    //- x/0 = crash
    if (player_divider == 0)
        player_divider = 1;

    matchmakerrating /= player_divider;

    return matchmakerrating;
}

float ArenaTeam::GetChanceAgainst(int32 own_rating, int32 enemy_rating)
{
    // returns the chance to win against a team with the given rating, used in the rating adjustment calculation
    // ELO system

    float chance = 1.0f / ( 1.0f + pow(10.0f, float(enemy_rating - own_rating) / 400.0f) );
    if (chance < 0.0f)
        return 0.0f;
    if (chance > 1.0f)
        return 1.0f;
    return chance;
}

int32 ArenaTeam::GetMatchMakerRatingMod(uint32 own_rating, uint32 enemy_rating, bool won)
{
    // 'chance' calculation - to beat the opponent
    float chance = GetChanceAgainst(own_rating, enemy_rating);
    float won_mod = (won) ? 1.0f : 0.0f;
    float mod = 46.0f * (won_mod - chance);     // with the value 46 team needs to play 40 games at an average to get from 1300 to 1650 if they win 50% of games against 1650

    // can't drop below 1
    if (own_rating + mod < 1)
        mod = -float(own_rating) + 1;

    // this condition makes sure that the average MMR remains 1500
    // (values of rating gain and loss of opponent team are equal)
    // TODO: it is needed to compute it for only one team, the second one has the same with opposite sign (+/-)
    if (won)
        return (int32)ceil(mod);
    else
        return (int32)floor(mod);
}

int32 ArenaTeam::GetRatingMod(uint32 own_rating, uint32 enemy_matchmaker_rating, bool won)
{
    float chance = GetChanceAgainst(own_rating, enemy_matchmaker_rating);
    float won_mod = (won) ? 1.0f : 0.0f;
    float mod = 46.0f * (won_mod - chance);

    if (own_rating < 1400)
    {
        if (!won)
            mod = 0.0f;
        else
            mod *= 2.0f;
    }
    else if (own_rating < 1500)
    {
        if (!won)
            mod *= 0.5f;
    }

    if (won)
        return (int32)ceil(mod);
    else
        return (int32)floor(mod);
}

void ArenaTeam::FinishGame(int32 teamRatingMod)
{
    if (int32(m_stats.rating) + teamRatingMod < 0)
        m_stats.rating = 0;
    else
    {
        m_stats.rating += teamRatingMod;
        for (MemberList::iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        {
            if (Player* member = ObjectAccessor::FindPlayer(itr->playerGuid))
                member->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_TEAM_RATING, m_stats.rating, m_Type);
        }
    }

    m_stats.games_week += 1;
    m_stats.games_season += 1;
    // update team's rank
    m_stats.rank = 1;
    ObjectMgr::ArenaTeamMap::const_iterator i = sObjectMgr->GetArenaTeamMapBegin();
    for (; i != sObjectMgr->GetArenaTeamMapEnd(); ++i)
    {
        if (i->second->GetType() == m_Type && i->second->GetStats().rating > m_stats.rating)
            ++m_stats.rank;
    }
}

int32 ArenaTeam::WonAgainst(uint32 selfMatchmakerRating, uint32 opponentMatchmakerRating)
{
    // called when the team has won
    // own team rating versus opponents matchmaker rating
    int32 matchmakerRatingMod = GetMatchMakerRatingMod(selfMatchmakerRating, opponentMatchmakerRating, true);
    int32 teamRatingMod = GetRatingMod(GetTeamRating(), opponentMatchmakerRating, true); // when the value will be clearly wrong, try to compare it with opponent's matchmaker instead - but it will stop following own matchmaker rating

    // modify the team stats accordingly
    FinishGame(teamRatingMod);
    m_stats.wins_week += 1;
    m_stats.wins_season += 1;

    // return the rating change, used to display it on the results screen
    return matchmakerRatingMod;
}

void ArenaTeam::MemberWon(Player * plr, uint32 opponentMatchmakerRating)
{
    // called for each participant after winning a match
    for (MemberList::iterator itr = m_members.begin(); itr !=  m_members.end(); ++itr)
    {
        if (itr->playerGuid == plr->GetGUID())
        {
            // update personal rating
            int32 mod = GetRatingMod(itr->personal_rating, opponentMatchmakerRating, true);
            itr->ModifyPersonalRating(plr, mod, GetSlot());
            if (Battleground *bg = plr->GetBattleground())
                bg->UpdatePlayerScore(plr, SCORE_PERSONAL_RATING_CHANGE, mod);

            // reward conquest points (new in 4.x)
            if (plr)
                plr->ModifyCurrency(CURRENCY_TYPE_CONQUEST_POINTS, CONQUEST_REWARD_ARENA, CURRENCY_SOURCE_ARENA, false);

            // update personal played stats
            itr->games_week +=1;
            itr->games_season +=1;
            itr->wins_week += 1;
            itr->wins_season += 1;
            // update the unit fields
            plr->SetArenaTeamInfoField(GetSlot(), ARENA_TEAM_GAMES_WEEK,  itr->games_week);
            plr->SetArenaTeamInfoField(GetSlot(), ARENA_TEAM_GAMES_SEASON,  itr->games_season);
            return;
        }
    }
}

void ArenaTeam::OfflineMemberWon(uint64 guid, uint32 opponentMatchmakerRating)
{
    // called for offline player after ending rated arena match!
    for (MemberList::iterator itr = m_members.begin(); itr !=  m_members.end(); ++itr)
    {
        if (itr->playerGuid == guid)
        {
            // update personal rating
            int32 mod = GetRatingMod(itr->personal_rating, opponentMatchmakerRating, true);
            itr->ModifyPersonalRating(NULL, mod, GetSlot());

            // update personal played stats
            itr->games_week +=1;
            itr->games_season +=1;
            itr->wins_week += 1;
            itr->wins_season += 1;
            return;
        }
    }
}

int32 ArenaTeam::LostAgainst(uint32 selfMatchmakerRating, uint32 opponentMatchmakerRating)
{
    // called when the team has lost
    // own team rating versus opponents matchmaker rating
    int32 matchmakerRatingMod = GetMatchMakerRatingMod(selfMatchmakerRating, opponentMatchmakerRating, false);
    int32 teamRatingMod = GetRatingMod(GetTeamRating(), opponentMatchmakerRating, false);

    // modify the team stats accordingly
    FinishGame(teamRatingMod);

    // return the rating change, used to display it on the results screen
    return matchmakerRatingMod;
}

void ArenaTeam::MemberLost(Player * plr, uint32 opponentMatchmakerRating)
{
    // called for each participant of a match after losing
    for (MemberList::iterator itr = m_members.begin(); itr !=  m_members.end(); ++itr)
    {
        if (itr->playerGuid == plr->GetGUID())
        {
            // update personal rating
            int32 mod = GetRatingMod(itr->personal_rating, opponentMatchmakerRating, false);
            itr->ModifyPersonalRating(plr, mod, GetSlot());
            if (Battleground *bg = plr->GetBattleground())
                bg->UpdatePlayerScore(plr, SCORE_PERSONAL_RATING_CHANGE, mod);

            // update personal played stats
            itr->games_week +=1;
            itr->games_season +=1;
            // update the unit fields
            plr->SetArenaTeamInfoField(GetSlot(), ARENA_TEAM_GAMES_WEEK,  itr->games_week);
            plr->SetArenaTeamInfoField(GetSlot(), ARENA_TEAM_GAMES_SEASON,  itr->games_season);
            return;
        }
    }
}

void ArenaTeam::OfflineMemberLost(uint64 guid, uint32 opponentMatchmakerRating)
{
    // called for offline player after ending rated arena match!
    for (MemberList::iterator itr = m_members.begin(); itr !=  m_members.end(); ++itr)
    {
        if (itr->playerGuid == guid)
        {
            // update personal rating
            int32 mod = GetRatingMod(itr->personal_rating, opponentMatchmakerRating, false);
            itr->ModifyPersonalRating(NULL, mod, GetSlot());

            return;
        }
    }
}

void ArenaTeam::UpdateArenaPointsHelper(std::map<uint32, uint32>& PlayerPoints)
{
    // called after a match has ended and the stats are already modified
    // helper function for arena point distribution (this way, when distributing, no actual calculation is required, just a few comparisons)
    // 10 played games per week is a minimum
    if (m_stats.games_week < 10)
        return;
    // to get points, a player has to participate in at least 30% of the matches
    uint32 min_plays = (uint32) ceil(m_stats.games_week * 0.3);
    for (MemberList::const_iterator itr = m_members.begin(); itr !=  m_members.end(); ++itr)
    {
        // the player participated in enough games, update his points
        uint32 points_to_add = 0;
        if (itr->games_week >= min_plays)
            points_to_add = GetPoints(itr->personal_rating);
        // OBSOLETE : CharacterDatabase.PExecute("UPDATE arena_team_member SET points_to_add = '%u' WHERE arenateamid = '%u' AND guid = '%u'", points_to_add, m_TeamId, itr->guid);

        std::map<uint32, uint32>::iterator plr_itr = PlayerPoints.find(GUID_LOPART(itr->playerGuid));
        if (plr_itr != PlayerPoints.end())
        {
            //check if there is already more points
            if (plr_itr->second < points_to_add)
                PlayerPoints[GUID_LOPART(itr->playerGuid)] = points_to_add;
        }
        else
            PlayerPoints[GUID_LOPART(itr->playerGuid)] = points_to_add;
    }
}

void ArenaTeam::UpdateMembersConquestPointCap()
{
    uint32 newcap = 0;
    uint32 oldcap = 0;
    Player* pSource = NULL;
    for (MemberList::iterator itr = m_members.begin(); itr !=  m_members.end(); ++itr)
    {
        // Calculate the cap for the highest rating only
        if (BattlegroundMgr::GetHighestArenaRating(itr->playerGuid) != itr->personal_rating)
            continue;

        newcap = BattlegroundMgr::CalculateArenaCap(itr->personal_rating);

        oldcap = itr->conquest_point_cap;

        itr->conquest_point_cap = newcap;

        // And notify also player class (if new cap is different from old cap)
        pSource = sObjectMgr->GetPlayerByLowGUID(GUID_LOPART(itr->playerGuid));
        if (pSource && newcap != oldcap)
        {
            pSource->SetCurrencyWeekCap(CURRENCY_TYPE_CONQUEST_POINTS, CURRENCY_SOURCE_ARENA, newcap * GetCurrencyPrecision(CURRENCY_TYPE_CONQUEST_POINTS));
            pSource->SendUpdateCurrencyWeekCap(CURRENCY_TYPE_CONQUEST_POINTS, newcap);
        }
        else
        {
            // Also update database data if the player is not online
            // (online players will save theirs cap automatically on next logout / autosave / .save
            CharacterDatabase.PExecute("REPLACE INTO character_currency_weekcap VALUES ('%u', '%u', '%u', '%u', '%u');", GUID_LOPART(itr->playerGuid), CURRENCY_TYPE_CONQUEST_POINTS, CURRENCY_SOURCE_ARENA, newcap * GetCurrencyPrecision(CURRENCY_TYPE_CONQUEST_POINTS), 0);
        }
    }
}

void ArenaTeam::SaveToDB()
{
    // save team and member stats to db
    // called after a match has ended, or when calculating arena_points
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    trans->PAppend("UPDATE arena_team_stats SET rating = '%u',games = '%u',played = '%u',rank = '%u',wins = '%u',wins2 = '%u' WHERE arenateamid = '%u'", m_stats.rating, m_stats.games_week, m_stats.games_season, m_stats.rank, m_stats.wins_week, m_stats.wins_season, GetId());
    for (MemberList::const_iterator itr = m_members.begin(); itr !=  m_members.end(); ++itr)
    {
        trans->PAppend("UPDATE arena_team_member SET played_week = '%u', wons_week = '%u', played_season = '%u', wons_season = '%u' WHERE arenateamid = '%u' AND guid = '%u'", itr->games_week, itr->wins_week, itr->games_season, itr->wins_season, m_TeamId, GUID_LOPART(itr->playerGuid));
        trans->PAppend("UPDATE character_arena_stats SET personal_rating = '%u',matchmaker_rating = '%u',highest_week_rating = '%u' WHERE guid = '%u' AND slot = '%u';", itr->personal_rating, itr->matchmaker_rating, itr->highest_week_rating, GUID_LOPART(itr->playerGuid), GetSlot());
    }
    CharacterDatabase.CommitTransaction(trans);
}

void ArenaTeam::FinishWeek()
{
    m_stats.games_week = 0;                                   // played this week
    m_stats.wins_week = 0;                                    // wins this week
    for (MemberList::iterator itr = m_members.begin(); itr !=  m_members.end(); ++itr)
    {
        itr->games_week = 0;
        itr->wins_week = 0;
    }
}

bool ArenaTeam::IsFighting() const
{
    for (MemberList::const_iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
        if (Player *p = sObjectMgr->GetPlayer(itr->playerGuid))
            if (p->GetMap()->IsBattleArena())
                return true;
    return false;
}
