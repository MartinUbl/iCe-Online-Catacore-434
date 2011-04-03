/*
 * Copyright (C) 2005-2011 MaNGOS <http://www.getmangos.com/>
 *
 * Copyright (C) 2008-2011 Trinity <http://www.trinitycore.org/>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __BATTLEGROUNDTP_H
#define __BATTLEGROUNDTP_H

class Battleground;

enum BG_TP_TimerOrScore
{
    BG_TP_MAX_TEAM_SCORE    = 3,
    BG_TP_FLAG_RESPAWN_TIME = 25000,
    BG_TP_FLAG_DROP_TIME    = 10000,
};

enum BG_TP_Sound
{
    BG_TP_SOUND_FLAG_CAPTURED_ALLIANCE  = 8173,
    BG_TP_SOUND_FLAG_CAPTURED_HORDE     = 8213,
    BG_TP_SOUND_FLAG_PLACED             = 8232,
    BG_TP_SOUND_FLAG_RETURNED           = 8192,
    BG_TP_SOUND_HORDE_FLAG_PICKED_UP    = 8212,
    BG_TP_SOUND_ALLIANCE_FLAG_PICKED_UP = 8174,
    BG_TP_SOUND_FLAGS_RESPAWNED         = 8232,
};

enum BG_TP_SpellId
{
    BG_TP_SPELL_HORDE_FLAG            = 23333,
    BG_TP_SPELL_HORDE_FLAG_DROPPED    = 23334,
    BG_TP_SPELL_HORDE_FLAG_PICKED     = 61266,    // fake spell, does not exist but used as timer start event
    BG_TP_SPELL_ALLIANCE_FLAG         = 23335,
    BG_TP_SPELL_ALLIANCE_FLAG_DROPPED = 23336,
    BG_TP_SPELL_ALLIANCE_FLAG_PICKED  = 61265,    // fake spell, does not exist but used as timer start event
};

enum BG_TP_WorldStates
{
    TP_FLAG_ALLIANCE_UP    = 1545,
    TP_FLAG_HORDE_UP       = 1546,
    TP_FLAGS_TOTAL         = 1601,
    TP_ALLIANCE_FLAGS      = 1581,
    TP_ALLIANCE_FLAGS_SHOW = 2339,
    TP_HORDE_FLAGS         = 1582,
    TP_HORDE_FLAGS_SHOW    = 2338,
    TP_TIME_DISPLAY        = 4247,
    TP_TIME_VALUE          = 4248, //minutes
};

enum BG_TP_ObjectTypes
{
    BG_TP_OBJECT_DOOR_A_1       = 0,
    BG_TP_OBJECT_DOOR_A_2       = 1,
    BG_TP_OBJECT_DOOR_A_3       = 2,
    BG_TP_OBJECT_DOOR_H_1       = 3,
    BG_TP_OBJECT_DOOR_H_2       = 4,
    BG_TP_OBJECT_DOOR_H_3       = 5,
    BG_TP_OBJECT_A_FLAG         = 6,
    BG_TP_OBJECT_H_FLAG         = 7,
    BG_TP_OBJECT_REGENBUFF_1    = 8,
    BG_TP_OBJECT_REGENBUFF_2    = 9,
    BG_TP_OBJECT_SPEEDBUFF_1    = 10,
    BG_TP_OBJECT_SPEEDBUFF_2    = 11,
    BG_TP_OBJECT_BERSERKBUFF_1  = 12,
    BG_TP_OBJECT_BERSERKBUFF_2  = 13,
    BG_TP_OBJECT_MAX            = 14
};

enum BG_TP_ObjectEntry
{
    BG_OBJECT_DOOR_A_1_TP_ENTRY          = 206653,
    BG_OBJECT_DOOR_A_2_TP_ENTRY          = 206654,
    BG_OBJECT_DOOR_A_3_TP_ENTRY          = 206655,
    BG_OBJECT_DOOR_H_TP_ENTRY            = 203710,
    BG_OBJECT_A_FLAG_TP_ENTRY            = 179830,
    BG_OBJECT_H_FLAG_TP_ENTRY            = 179831,
    BG_OBJECT_A_FLAG_GROUND_TP_ENTRY     = 179785,
    BG_OBJECT_H_FLAG_GROUND_TP_ENTRY     = 179786
};

enum BG_TP_FlagState
{
    BG_TP_FLAG_STATE_ON_BASE      = 0,
    BG_TP_FLAG_STATE_WAIT_RESPAWN = 1,
    BG_TP_FLAG_STATE_ON_PLAYER    = 2,
    BG_TP_FLAG_STATE_ON_GROUND    = 3
};

enum BG_TP_Graveyards
{
    TP_GRAVEYARD_FLAGROOM_ALLIANCE = 1726,
    TP_GRAVEYARD_FLAGROOM_HORDE    = 1727,
    TP_GRAVEYARD_BASE_ALLIANCE     = 1729,
    TP_GRAVEYARD_BASE_HORDE        = 1728,
    TP_GRAVEYARD_CENTER_ALLIANCE   = 1749,
    TP_GRAVEYARD_CENTER_HORDE      = 1750,
};

enum BG_TP_CreatureTypes
{
    TP_SPIRIT_MAIN_ALLIANCE   = 0,
    TP_SPIRIT_MAIN_HORDE      = 1,
    TP_SPIRIT_CENTER_ALLIANCE = 2,
    TP_SPIRIT_CENTER_HORDE    = 3,

    BG_CREATURES_MAX_TP       = 4
};

enum BG_TP_Objectives
{
    TP_OBJECTIVE_FLAG_CAPTURE   = 290,
    TP_OBJECTIVE_FLAG_RETURN    = 291,
};

class BattlegroundTPScore : public BattlegroundScore
{
    public:
        BattlegroundTPScore() : FlagCaptures(0), FlagReturns(0) {};
        virtual ~BattlegroundTPScore() {};
        uint32 FlagCaptures;
        uint32 FlagReturns;
};

class BattlegroundTP : public Battleground
{
    friend class BattlegroundMgr;

    public:
        BattlegroundTP();
        ~BattlegroundTP();
        void Update(uint32 diff);

        /* inherited from BattlegroundClass */
        virtual void AddPlayer(Player *plr);
        virtual void StartingEventCloseDoors();
        virtual void StartingEventOpenDoors();

        void RemovePlayer(Player *plr,uint64 guid);
        void HandleAreaTrigger(Player *Source, uint32 Trigger);
        bool SetupBattleground();
        virtual void Reset();
        void HandleKillPlayer(Player *player, Player *killer);
        void EndBattleground(uint32 winner);
        virtual WorldSafeLocsEntry const* GetClosestGraveYard(Player* player);

        // Events
        virtual void EventPlayerDroppedFlag(Player *Source);
        virtual void EventPlayerClickedOnFlag(Player *Source, GameObject* target_obj);
        virtual void EventPlayerCapturedFlag(Player *Source);

        // Flags
        uint64 GetAllianceFlagPickerGUID() const    { return m_FlagKeepers[BG_TEAM_ALLIANCE]; }
        uint64 GetHordeFlagPickerGUID() const       { return m_FlagKeepers[BG_TEAM_HORDE]; }
        void SetAllianceFlagPicker(uint64 guid)     { m_FlagKeepers[BG_TEAM_ALLIANCE] = guid; }
        void SetHordeFlagPicker(uint64 guid)        { m_FlagKeepers[BG_TEAM_HORDE] = guid; }
        bool IsAllianceFlagPickedup() const         { return m_FlagKeepers[BG_TEAM_ALLIANCE] != 0; }
        bool IsHordeFlagPickedup() const            { return m_FlagKeepers[BG_TEAM_HORDE] != 0; }
        void RespawnFlag(uint32 Team, bool captured);
        void RespawnFlagAfterDrop(uint32 Team);
        uint8 GetFlagState(uint32 team)             { return m_FlagState[GetTeamIndexByTeamId(team)]; }

        void UpdateFlagState(uint32 team, uint32 value);
        void UpdateTeamScore(uint32 team);
        void UpdatePlayerScore(Player *Source, uint32 type, uint32 value, bool doAddHonor = true);
        void SetLastFlagCapture(uint32 team)                { m_LastFlagCaptureTeam = team; }
        void SetDroppedFlagGUID(uint64 guid, uint32 TeamID)  { m_DroppedFlagGUID[GetTeamIndexByTeamId(TeamID)] = guid;}
        uint64 GetDroppedFlagGUID(uint32 TeamID)             { return m_DroppedFlagGUID[GetTeamIndexByTeamId(TeamID)];}
        virtual void FillInitialWorldStates(WorldPacket& data);

        /* Scorekeeping */
        uint32 GetTeamScore(uint32 TeamID) const            { return m_TeamScores[GetTeamIndexByTeamId(TeamID)]; }
        void AddPoint(uint32 TeamID, uint32 Points = 1)     { m_TeamScores[GetTeamIndexByTeamId(TeamID)] += Points; }
        void SetTeamPoint(uint32 TeamID, uint32 Points = 0) { m_TeamScores[GetTeamIndexByTeamId(TeamID)] = Points; }
        void RemovePoint(uint32 TeamID, uint32 Points = 1)  { m_TeamScores[GetTeamIndexByTeamId(TeamID)] -= Points; }

    private:
        uint64 m_FlagKeepers[2];                            // 0 - alliance, 1 - horde
        uint64 m_DroppedFlagGUID[2];
        uint8 m_FlagState[2];                               // for checking flag state
        int32 m_FlagsTimer[2];
        int32 m_FlagsDropTimer[2];
        uint32 m_LastFlagCaptureTeam;                       // Winner is based on this if score is equal

        uint32 m_ReputationCapture;
        uint32 m_HonorWinKills;
        uint32 m_HonorEndKills;
        bool m_BothFlagsKept;
        uint8 m_minutesElapsed;
};
#endif
