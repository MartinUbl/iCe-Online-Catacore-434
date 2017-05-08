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
#include "Battleground.h"
#include "BattlegroundWS.h"
#include "Creature.h"
#include "GameObject.h"
#include "Language.h"
#include "Object.h"
#include "ObjectMgr.h"
#include "BattlegroundMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldPacket.h"

// these variables aren't used outside of this file, so declare them only here
enum BG_WS_Rewards
{
    BG_WS_WIN = 0,
    BG_WS_FLAG_CAP,
    BG_WS_REWARD_NUM
};

uint32 BG_WS_Honor[BG_WS_REWARD_NUM]
    = {10,40};

BattlegroundWS::BattlegroundWS()
{
    m_BgObjects.resize(BG_WS_OBJECT_MAX);
    m_BgCreatures.resize(BG_CREATURES_MAX_WS);

    m_StartMessageIds[BG_STARTING_EVENT_FIRST]  = LANG_BG_WS_START_TWO_MINUTES;
    m_StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_WS_START_ONE_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_WS_START_HALF_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_WS_HAS_BEGUN;
}

BattlegroundWS::~BattlegroundWS()
{
}

void BattlegroundWS::Update(uint32 diff)
{
    Battleground::Update(diff);

    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        if (GetElapsedTime() >= 27*MINUTE*IN_MILLISECONDS)
        {
            if (GetTeamScore(ALLIANCE) == 0)
            {
                if (GetTeamScore(HORDE) == 0)        // No one scored - result is tie
                    EndBattleground(3);
                else                                 // Horde has more points and thus wins
                {
                    RewardHonorToTeam(BG_WS_Honor[BG_WS_WIN], HORDE);
                    EndBattleground(HORDE);
                }
            }

            else if (GetTeamScore(HORDE) == 0)
            {
                RewardHonorToTeam(BG_WS_Honor[BG_WS_WIN], ALLIANCE);
                EndBattleground(ALLIANCE);           // Alliance has > 0, Horde has 0, alliance wins
            }

            else if (GetTeamScore(HORDE) == GetTeamScore(ALLIANCE)) // Team score equal, winner is team that scored the last flag
            {
                RewardHonorToTeam(BG_WS_Honor[BG_WS_WIN], m_LastFlagCaptureTeam);
                EndBattleground(m_LastFlagCaptureTeam);
            }

            else if (GetTeamScore(HORDE) > GetTeamScore(ALLIANCE))  // Last but not least, check who has the higher score
            {
                RewardHonorToTeam(BG_WS_Honor[BG_WS_WIN], HORDE);
                EndBattleground(HORDE);
            }
            else
            {
                RewardHonorToTeam(BG_WS_Honor[BG_WS_WIN], ALLIANCE);
                EndBattleground(ALLIANCE);
            }
        }
        else if (GetElapsedTime() > uint32(m_minutesElapsed * MINUTE * IN_MILLISECONDS))
        {
            ++m_minutesElapsed;
            UpdateWorldState(BG_WS_STATE_TIMER, 27 - m_minutesElapsed);
        }

        if (m_FlagState[BG_TEAM_ALLIANCE] == BG_WS_FLAG_STATE_WAIT_RESPAWN)
        {
            m_FlagsTimer[BG_TEAM_ALLIANCE] -= diff;

            if (m_FlagsTimer[BG_TEAM_ALLIANCE] < 0)
            {
                m_FlagsTimer[BG_TEAM_ALLIANCE] = 0;
                RespawnFlag(ALLIANCE, true);
            }
        }
        if (m_FlagState[BG_TEAM_ALLIANCE] == BG_WS_FLAG_STATE_ON_GROUND)
        {
            m_FlagsDropTimer[BG_TEAM_ALLIANCE] -= diff;

            if (m_FlagsDropTimer[BG_TEAM_ALLIANCE] < 0)
            {
                m_FlagsDropTimer[BG_TEAM_ALLIANCE] = 0;
                RespawnFlagAfterDrop(ALLIANCE);
                m_BothFlagsKept = false;
            }
        }
        if (m_FlagState[BG_TEAM_HORDE] == BG_WS_FLAG_STATE_WAIT_RESPAWN)
        {
            m_FlagsTimer[BG_TEAM_HORDE] -= diff;

            if (m_FlagsTimer[BG_TEAM_HORDE] < 0)
            {
                m_FlagsTimer[BG_TEAM_HORDE] = 0;
                RespawnFlag(HORDE, true);
            }
        }
        if (m_FlagState[BG_TEAM_HORDE] == BG_WS_FLAG_STATE_ON_GROUND)
        {
            m_FlagsDropTimer[BG_TEAM_HORDE] -= diff;

            if (m_FlagsDropTimer[BG_TEAM_HORDE] < 0)
            {
                m_FlagsDropTimer[BG_TEAM_HORDE] = 0;
                RespawnFlagAfterDrop(HORDE);
                m_BothFlagsKept = false;
            }
        }

        if (m_BothFlagsKept)
        {
          m_FlagSpellForceTimer += diff;

          switch (m_FlagDebuffState)
          {
            case 0:
            {
                if (m_FlagSpellForceTimer >= 180000) // After 3 minutes of both teams having the flag
                {
                    if (Player * plr = sObjectMgr->GetPlayer(m_FlagKeepers[0]))
                      plr->CastSpell(plr,WS_SPELL_FOCUSED_ASSAULT,true);
                    if (Player * plr = sObjectMgr->GetPlayer(m_FlagKeepers[1]))
                      plr->CastSpell(plr,WS_SPELL_FOCUSED_ASSAULT,true);

                    m_FlagDebuffState++;
                    m_FlagSpellForceTimer = 0; // Reset timer for next stack
                }
                break;
            }
            case 1:
            case 2:
            case 3:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
            {
                if (m_FlagSpellForceTimer >= 60000)
                {
                    if (Player * plr = sObjectMgr->GetPlayer(m_FlagKeepers[0]))
                      plr->CastSpell(plr,m_FlagDebuffState < 4 ? WS_SPELL_FOCUSED_ASSAULT : WS_SPELL_BRUTAL_ASSAULT,true);
                    if (Player * plr = sObjectMgr->GetPlayer(m_FlagKeepers[1]))
                        plr->CastSpell(plr, m_FlagDebuffState < 4 ? WS_SPELL_FOCUSED_ASSAULT : WS_SPELL_BRUTAL_ASSAULT, true);

                    m_FlagDebuffState++;
                    m_FlagSpellForceTimer = 0;
                }
                break;
            }
            case 4: // After 7 minutes, Brutal Assault will mutate into Focused Assault
            {
                if (m_FlagSpellForceTimer >= 60000)
                {
                    // Set debuff state first for correct amount of stacks
                    m_FlagDebuffState++;
                    m_FlagSpellForceTimer = 0;

                    if (Player * plr = sObjectMgr->GetPlayer(m_FlagKeepers[0]))
                    {
                        plr->RemoveAurasDueToSpell(WS_SPELL_FOCUSED_ASSAULT);
                        for (size_t i = 0; i < m_FlagDebuffState; i++)
                        {
                            plr->CastSpell(plr, WS_SPELL_BRUTAL_ASSAULT, true);
                        }
                    }
                    if (Player * plr = sObjectMgr->GetPlayer(m_FlagKeepers[1]))
                    {
                        plr->RemoveAurasDueToSpell(WS_SPELL_FOCUSED_ASSAULT);
                        for (size_t i = 0; i < m_FlagDebuffState; i++)
                        {
                            plr->CastSpell(plr, WS_SPELL_BRUTAL_ASSAULT, true);
                        }
                    }
                }
                break;
            }
            default:
                break;
          }
        }
        else
        {
            if (Player * plr = sObjectMgr->GetPlayer(m_FlagKeepers[0]))
            {
                plr->RemoveAurasDueToSpell(WS_SPELL_FOCUSED_ASSAULT);
                plr->RemoveAurasDueToSpell(WS_SPELL_BRUTAL_ASSAULT);
            }
            if (Player * plr = sObjectMgr->GetPlayer(m_FlagKeepers[1]))
            {
                plr->RemoveAurasDueToSpell(WS_SPELL_FOCUSED_ASSAULT);
                plr->RemoveAurasDueToSpell(WS_SPELL_BRUTAL_ASSAULT);
            }
            m_FlagSpellForceTimer = 0; // Reset timer
            m_FlagDebuffState = 0;
        }
    }
}

void BattlegroundWS::StartingEventCloseDoors()
{
    for (uint32 i = BG_WS_OBJECT_DOOR_A_1; i <= BG_WS_OBJECT_DOOR_H_4; ++i)
    {
        //DoorClose(i);
        //SpawnBGObject(i, RESPAWN_IMMEDIATELY);
    }
    for (uint32 i = BG_WS_OBJECT_A_FLAG; i <= BG_WS_OBJECT_BERSERKBUFF_2; ++i)
        SpawnBGObject(i, RESPAWN_ONE_DAY);

    UpdateWorldState(BG_WS_STATE_TIMER_ACTIVE, 1);
    UpdateWorldState(BG_WS_STATE_TIMER, 25);
}

void BattlegroundWS::StartingEventOpenDoors()
{
    for (uint32 i = BG_WS_OBJECT_DOOR_A_1; i <= BG_WS_OBJECT_DOOR_A_6; ++i)
        DoorOpen(i);
    for (uint32 i = BG_WS_OBJECT_DOOR_H_1; i <= BG_WS_OBJECT_DOOR_H_4; ++i)
        DoorOpen(i);

    SpawnBGObject(BG_WS_OBJECT_DOOR_A_1, RESPAWN_ONE_DAY);
    SpawnBGObject(BG_WS_OBJECT_DOOR_A_2, RESPAWN_ONE_DAY);
    SpawnBGObject(BG_WS_OBJECT_DOOR_A_3, RESPAWN_ONE_DAY);
    SpawnBGObject(BG_WS_OBJECT_DOOR_A_4, RESPAWN_ONE_DAY);
    SpawnBGObject(BG_WS_OBJECT_DOOR_A_5, RESPAWN_ONE_DAY);
    SpawnBGObject(BG_WS_OBJECT_DOOR_A_6, RESPAWN_ONE_DAY);
    SpawnBGObject(BG_WS_OBJECT_DOOR_H_1, RESPAWN_ONE_DAY);
    SpawnBGObject(BG_WS_OBJECT_DOOR_H_2, RESPAWN_ONE_DAY);
    SpawnBGObject(BG_WS_OBJECT_DOOR_H_3, RESPAWN_ONE_DAY);
    SpawnBGObject(BG_WS_OBJECT_DOOR_H_4, RESPAWN_ONE_DAY);

    for (uint32 i = BG_WS_OBJECT_A_FLAG; i <= BG_WS_OBJECT_BERSERKBUFF_2; ++i)
        SpawnBGObject(i, RESPAWN_IMMEDIATELY);

    // players joining later are not egible
    StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, WS_EVENT_START_BATTLE);
}

void BattlegroundWS::AddPlayer(Player *plr)
{
    Battleground::AddPlayer(plr);
    //create score and add it to map, default values are set in constructor
    BattlegroundWGScore* sc = new BattlegroundWGScore;

    m_PlayerScores[plr->GetGUID()] = sc;
}

void BattlegroundWS::RespawnFlag(uint32 Team, bool captured)
{
    if (Team == ALLIANCE)
    {
        sLog->outDebug("Respawn Alliance flag");
        m_FlagState[BG_TEAM_ALLIANCE] = BG_WS_FLAG_STATE_ON_BASE;
    }
    else
    {
        sLog->outDebug("Respawn Horde flag");
        m_FlagState[BG_TEAM_HORDE] = BG_WS_FLAG_STATE_ON_BASE;
    }

    if (captured)
    {
        //when map_update will be allowed for battlegrounds this code will be useless
        SpawnBGObject(BG_WS_OBJECT_H_FLAG, RESPAWN_IMMEDIATELY);
        SpawnBGObject(BG_WS_OBJECT_A_FLAG, RESPAWN_IMMEDIATELY);
        SendMessageToAll(LANG_BG_WS_F_PLACED, CHAT_MSG_BG_SYSTEM_NEUTRAL);
        PlaySoundToAll(BG_WS_SOUND_FLAGS_RESPAWNED);        // flag respawned sound...
    }
    m_BothFlagsKept = false;
}

void BattlegroundWS::RespawnFlagAfterDrop(uint32 team)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    RespawnFlag(team,false);
    if (team == ALLIANCE)
    {
        SpawnBGObject(BG_WS_OBJECT_A_FLAG, RESPAWN_IMMEDIATELY);
        SendMessageToAll(LANG_BG_WS_ALLIANCE_FLAG_RESPAWNED, CHAT_MSG_BG_SYSTEM_NEUTRAL);
    }
    else
    {
        SpawnBGObject(BG_WS_OBJECT_H_FLAG, RESPAWN_IMMEDIATELY);
        SendMessageToAll(LANG_BG_WS_HORDE_FLAG_RESPAWNED, CHAT_MSG_BG_SYSTEM_NEUTRAL);
    }

    PlaySoundToAll(BG_WS_SOUND_FLAGS_RESPAWNED);

    GameObject *obj = GetBgMap()->GetGameObject(GetDroppedFlagGUID(team));
    if (obj)
        obj->Delete();
    else
        sLog->outError("unknown droped flag bg, guid: %u",GUID_LOPART(GetDroppedFlagGUID(team)));

    SetDroppedFlagGUID(0,team);
    m_BothFlagsKept = false;
}

void BattlegroundWS::EventPlayerCapturedFlag(Player *Source)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    uint32 winner = 0;

    Source->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
    if (Source->GetBGTeam() == ALLIANCE)
    {
        if (!this->IsHordeFlagPickedup())
            return;
        SetHordeFlagPicker(0);                              // must be before aura remove to prevent 2 events (drop+capture) at the same time
                                                            // horde flag in base (but not respawned yet)
        m_FlagState[BG_TEAM_HORDE] = BG_WS_FLAG_STATE_WAIT_RESPAWN;
                                                            // Drop Horde Flag and Debuffs from Player
        Source->RemoveAurasDueToSpell(BG_WS_SPELL_WARSONG_FLAG);
        Source->RemoveAurasDueToSpell(WS_SPELL_FOCUSED_ASSAULT);
        Source->RemoveAurasDueToSpell(WS_SPELL_BRUTAL_ASSAULT);

        if (GetTeamScore(ALLIANCE) < BG_WS_MAX_TEAM_SCORE)
            AddPoint(ALLIANCE, 1);
        PlaySoundToAll(BG_WS_SOUND_FLAG_CAPTURED_ALLIANCE);
        RewardReputationToTeamMixed(890, 889, m_ReputationCapture, ALLIANCE);
    }
    else
    {
        if (!this->IsAllianceFlagPickedup())
            return;
        SetAllianceFlagPicker(0);                           // must be before aura remove to prevent 2 events (drop+capture) at the same time
                                                            // alliance flag in base (but not respawned yet)
        m_FlagState[BG_TEAM_ALLIANCE] = BG_WS_FLAG_STATE_WAIT_RESPAWN;
                                                            // Drop Alliance Flag and Debuffs from Player
        Source->RemoveAurasDueToSpell(BG_WS_SPELL_SILVERWING_FLAG);
        Source->RemoveAurasDueToSpell(WS_SPELL_FOCUSED_ASSAULT);
        Source->RemoveAurasDueToSpell(WS_SPELL_BRUTAL_ASSAULT);

        if (GetTeamScore(HORDE) < BG_WS_MAX_TEAM_SCORE)
            AddPoint(HORDE, 1);
        PlaySoundToAll(BG_WS_SOUND_FLAG_CAPTURED_HORDE);
        RewardReputationToTeamMixed(890, 889, m_ReputationCapture, HORDE);
    }
    //for flag capture was reward 2 honorable kills
    //now special reward (40 honor points).. TODO: implement holiday
    RewardHonorToTeam(BG_WS_Honor[BG_WS_FLAG_CAP], Source->GetBGTeam());

    SpawnBGObject(BG_WS_OBJECT_H_FLAG, BG_WS_FLAG_RESPAWN_TIME);
    SpawnBGObject(BG_WS_OBJECT_A_FLAG, BG_WS_FLAG_RESPAWN_TIME);

    if (Source->GetBGTeam() == ALLIANCE)
        SendMessageToAll(LANG_BG_WS_CAPTURED_HF, CHAT_MSG_BG_SYSTEM_ALLIANCE, Source);
    else
        SendMessageToAll(LANG_BG_WS_CAPTURED_AF, CHAT_MSG_BG_SYSTEM_HORDE, Source);

    UpdateFlagState(Source->GetBGTeam(), 1);                  // flag state none
    UpdateTeamScore(Source->GetBGTeam());
    // only flag capture should be updated
    UpdatePlayerScore(Source, SCORE_FLAG_CAPTURES, 1);      // +1 flag captures

    // update last flag capture to be used if teamscore is equal
    SetLastFlagCapture(Source->GetBGTeam());

    if (GetTeamScore(ALLIANCE) == BG_WS_MAX_TEAM_SCORE)
        winner = ALLIANCE;

    if (GetTeamScore(HORDE) == BG_WS_MAX_TEAM_SCORE)
        winner = HORDE;

    if (winner)
    {
        UpdateWorldState(BG_WS_FLAG_UNK_ALLIANCE, 0);
        UpdateWorldState(BG_WS_FLAG_UNK_HORDE, 0);
        UpdateWorldState(BG_WS_FLAG_STATE_ALLIANCE, 1);
        UpdateWorldState(BG_WS_FLAG_STATE_HORDE, 1);
        UpdateWorldState(BG_WS_STATE_TIMER_ACTIVE, 0);

        RewardHonorToTeam(BG_WS_Honor[BG_WS_WIN], winner);
        EndBattleground(winner);
    }
    else
    {
        m_FlagsTimer[GetTeamIndexByTeamId(Source->GetBGTeam()) ? 0 : 1] = BG_WS_FLAG_RESPAWN_TIME;
    }
}

void BattlegroundWS::EventPlayerDroppedFlag(Player *Source)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
    {
        // if not running, do not cast things at the dropper player (prevent spawning the "dropped" flag), neither send unnecessary messages
        // just take off the aura
        if (Source->GetBGTeam() == ALLIANCE)
        {
            if (!this->IsHordeFlagPickedup())
                return;
            if (GetHordeFlagPickerGUID() == Source->GetGUID())
            {
                SetHordeFlagPicker(0);
                Source->RemoveAurasDueToSpell(BG_WS_SPELL_WARSONG_FLAG);
            }
        }
        else
        {
            if (!this->IsAllianceFlagPickedup())
                return;
            if (GetAllianceFlagPickerGUID() == Source->GetGUID())
            {
                SetAllianceFlagPicker(0);
                Source->RemoveAurasDueToSpell(BG_WS_SPELL_SILVERWING_FLAG);
            }
        }
        return;
    }

    bool set = false;

    if (Source->GetBGTeam() == ALLIANCE)
    {
        if (!IsHordeFlagPickedup())
            return;
        if (GetHordeFlagPickerGUID() == Source->GetGUID())
        {
            SetHordeFlagPicker(0);
            Source->RemoveAurasDueToSpell(BG_WS_SPELL_WARSONG_FLAG);
            Source->RemoveAurasDueToSpell(WS_SPELL_FOCUSED_ASSAULT);
            Source->RemoveAurasDueToSpell(WS_SPELL_BRUTAL_ASSAULT);
            m_FlagState[BG_TEAM_HORDE] = BG_WS_FLAG_STATE_ON_GROUND;
            Source->CastSpell(Source, BG_WS_SPELL_WARSONG_FLAG_DROPPED, true);
            set = true;
        }
    }
    else
    {
        if (!IsAllianceFlagPickedup())
            return;
        if (GetAllianceFlagPickerGUID() == Source->GetGUID())
        {
            SetAllianceFlagPicker(0);
            Source->RemoveAurasDueToSpell(BG_WS_SPELL_SILVERWING_FLAG);
            Source->RemoveAurasDueToSpell(WS_SPELL_FOCUSED_ASSAULT);
            Source->RemoveAurasDueToSpell(WS_SPELL_BRUTAL_ASSAULT);
            m_FlagState[BG_TEAM_ALLIANCE] = BG_WS_FLAG_STATE_ON_GROUND;
            Source->CastSpell(Source, BG_WS_SPELL_SILVERWING_FLAG_DROPPED, true);
            set = true;
        }
    }

    if (set)
    {
        Source->CastSpell(Source, SPELL_RECENTLY_DROPPED_FLAG, true);
        UpdateFlagState(Source->GetBGTeam(), 1);

        if (Source->GetBGTeam() == ALLIANCE)
        {
            SendMessageToAll(LANG_BG_WS_DROPPED_HF, CHAT_MSG_BG_SYSTEM_HORDE, Source);
            UpdateWorldState(BG_WS_FLAG_UNK_HORDE, uint32(-1));
        }
        else
        {
            SendMessageToAll(LANG_BG_WS_DROPPED_AF, CHAT_MSG_BG_SYSTEM_ALLIANCE, Source);
            UpdateWorldState(BG_WS_FLAG_UNK_ALLIANCE, uint32(-1));
        }

        m_FlagsDropTimer[GetTeamIndexByTeamId(Source->GetBGTeam()) ? 0 : 1] = BG_WS_FLAG_DROP_TIME;
    }
}

void BattlegroundWS::EventPlayerClickedOnFlag(Player *Source, GameObject* target_obj)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    int32 message_id = 0;
    ChatMsg type = CHAT_MSG_BG_SYSTEM_NEUTRAL;

    //alliance flag picked up from base
    if (Source->GetBGTeam() == HORDE && this->GetFlagState(ALLIANCE) == BG_WS_FLAG_STATE_ON_BASE
        && this->m_BgObjects[BG_WS_OBJECT_A_FLAG] == target_obj->GetGUID())
    {
        message_id = LANG_BG_WS_PICKEDUP_AF;
        type = CHAT_MSG_BG_SYSTEM_HORDE;
        PlaySoundToAll(BG_WS_SOUND_ALLIANCE_FLAG_PICKED_UP);
        SpawnBGObject(BG_WS_OBJECT_A_FLAG, RESPAWN_ONE_DAY);
        SetAllianceFlagPicker(Source->GetGUID());
        m_FlagState[BG_TEAM_ALLIANCE] = BG_WS_FLAG_STATE_ON_PLAYER;
        //update world state to show correct flag carrier
        UpdateFlagState(HORDE, BG_WS_FLAG_STATE_ON_PLAYER);
        UpdateWorldState(BG_WS_FLAG_UNK_ALLIANCE, 1);
        Source->CastSpell(Source, BG_WS_SPELL_SILVERWING_FLAG, true);
        Source->GetAchievementMgr().StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_SPELL_TARGET, BG_WS_SPELL_SILVERWING_FLAG_PICKED);
        if (m_FlagState[1] == BG_WS_FLAG_STATE_ON_PLAYER)
          m_BothFlagsKept = true;
    }

    //horde flag picked up from base
    if (Source->GetBGTeam() == ALLIANCE && this->GetFlagState(HORDE) == BG_WS_FLAG_STATE_ON_BASE
        && this->m_BgObjects[BG_WS_OBJECT_H_FLAG] == target_obj->GetGUID())
    {
        message_id = LANG_BG_WS_PICKEDUP_HF;
        type = CHAT_MSG_BG_SYSTEM_ALLIANCE;
        PlaySoundToAll(BG_WS_SOUND_HORDE_FLAG_PICKED_UP);
        SpawnBGObject(BG_WS_OBJECT_H_FLAG, RESPAWN_ONE_DAY);
        SetHordeFlagPicker(Source->GetGUID());
        m_FlagState[BG_TEAM_HORDE] = BG_WS_FLAG_STATE_ON_PLAYER;
        //update world state to show correct flag carrier
        UpdateFlagState(ALLIANCE, BG_WS_FLAG_STATE_ON_PLAYER);
        UpdateWorldState(BG_WS_FLAG_UNK_HORDE, 1);
        Source->CastSpell(Source, BG_WS_SPELL_WARSONG_FLAG, true);
        Source->GetAchievementMgr().StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_SPELL_TARGET, BG_WS_SPELL_WARSONG_FLAG_PICKED);
        if (m_FlagState[0] == BG_WS_FLAG_STATE_ON_PLAYER)
          m_BothFlagsKept = true;
    }

    //Alliance flag on ground(not in base) (returned or picked up again from ground!)
    if (GetFlagState(ALLIANCE) == BG_WS_FLAG_STATE_ON_GROUND && Source->IsWithinDistInMap(target_obj, 10))
    {
        if (Source->GetBGTeam() == ALLIANCE)
        {
            message_id = LANG_BG_WS_RETURNED_AF;
            type = CHAT_MSG_BG_SYSTEM_ALLIANCE;
            UpdateFlagState(HORDE, BG_WS_FLAG_STATE_WAIT_RESPAWN);
            RespawnFlag(ALLIANCE, false);
            SpawnBGObject(BG_WS_OBJECT_A_FLAG, RESPAWN_IMMEDIATELY);
            PlaySoundToAll(BG_WS_SOUND_FLAG_RETURNED);
            UpdatePlayerScore(Source, SCORE_FLAG_RETURNS, 1);
            m_BothFlagsKept = false;
        }
        else
        {
            message_id = LANG_BG_WS_PICKEDUP_AF;
            type = CHAT_MSG_BG_SYSTEM_HORDE;
            PlaySoundToAll(BG_WS_SOUND_ALLIANCE_FLAG_PICKED_UP);
            SpawnBGObject(BG_WS_OBJECT_A_FLAG, RESPAWN_ONE_DAY);
            SetAllianceFlagPicker(Source->GetGUID());
            Source->CastSpell(Source, BG_WS_SPELL_SILVERWING_FLAG, true);
            m_FlagState[BG_TEAM_ALLIANCE] = BG_WS_FLAG_STATE_ON_PLAYER;
            UpdateFlagState(HORDE, BG_WS_FLAG_STATE_ON_PLAYER);
            if (m_FlagDebuffState >= 1)
            {
                for (uint8 i = 0; i < m_FlagDebuffState; i++)
                    Source->CastSpell(Source, m_FlagDebuffState <= 4 ? WS_SPELL_FOCUSED_ASSAULT : WS_SPELL_BRUTAL_ASSAULT, true);
            }

            UpdateWorldState(BG_WS_FLAG_UNK_ALLIANCE, 1);
        }
        //called in HandleGameObjectUseOpcode:
        //target_obj->Delete();
    }
    //Horde flag on ground(not in base) (returned or picked up again)
    else if (GetFlagState(HORDE) == BG_WS_FLAG_STATE_ON_GROUND && Source->IsWithinDistInMap(target_obj, 10))
    {
        if (Source->GetBGTeam() == HORDE) // Flag was returned to base by enemy team
        {
            message_id = LANG_BG_WS_RETURNED_HF;
            type = CHAT_MSG_BG_SYSTEM_HORDE;
            UpdateFlagState(ALLIANCE, BG_WS_FLAG_STATE_WAIT_RESPAWN);
            RespawnFlag(HORDE, false);
            SpawnBGObject(BG_WS_OBJECT_H_FLAG, RESPAWN_IMMEDIATELY);
            PlaySoundToAll(BG_WS_SOUND_FLAG_RETURNED);
            UpdatePlayerScore(Source, SCORE_FLAG_RETURNS, 1);
            m_BothFlagsKept = false;
        }
        else // Flag was picked up again 
        {
            message_id = LANG_BG_WS_PICKEDUP_HF;
            type = CHAT_MSG_BG_SYSTEM_ALLIANCE;
            PlaySoundToAll(BG_WS_SOUND_HORDE_FLAG_PICKED_UP);
            SpawnBGObject(BG_WS_OBJECT_H_FLAG, RESPAWN_ONE_DAY);
            SetHordeFlagPicker(Source->GetGUID());
            Source->CastSpell(Source, BG_WS_SPELL_WARSONG_FLAG, true);
            m_FlagState[BG_TEAM_HORDE] = BG_WS_FLAG_STATE_ON_PLAYER;
            UpdateFlagState(ALLIANCE, BG_WS_FLAG_STATE_ON_PLAYER);
            if (m_FlagDebuffState >= 1)
            {
                for (uint8 i = 0; i < m_FlagDebuffState; i++)
                    Source->CastSpell(Source, m_FlagDebuffState <= 4 ? WS_SPELL_FOCUSED_ASSAULT : WS_SPELL_BRUTAL_ASSAULT,true);
            }

            UpdateWorldState(BG_WS_FLAG_UNK_HORDE, 1);
        }
        //called in HandleGameObjectUseOpcode:
        //target_obj->Delete();
    }

    if (!message_id)
        return;

    SendMessageToAll(message_id, type, Source);
    Source->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
}

void BattlegroundWS::RemovePlayer(Player *plr, uint64 guid)
{
    // sometimes flag aura not removed :(
    if (IsAllianceFlagPickedup() && m_FlagKeepers[BG_TEAM_ALLIANCE] == guid)
    {
        if (!plr)
        {
            sLog->outError("BattlegroundWS: Removing offline player who has the FLAG!!");
            this->SetAllianceFlagPicker(0);
            this->RespawnFlag(ALLIANCE, false);
        }
        else
            this->EventPlayerDroppedFlag(plr);
    }
    if (IsHordeFlagPickedup() && m_FlagKeepers[BG_TEAM_HORDE] == guid)
    {
        if (!plr)
        {
            sLog->outError("BattlegroundWS: Removing offline player who has the FLAG!!");
            this->SetHordeFlagPicker(0);
            this->RespawnFlag(HORDE, false);
        }
        else
            this->EventPlayerDroppedFlag(plr);
    }
}

void BattlegroundWS::UpdateFlagState(uint32 team, uint32 value)
{
    if (team == ALLIANCE)
        UpdateWorldState(BG_WS_FLAG_STATE_ALLIANCE, value);
    else
        UpdateWorldState(BG_WS_FLAG_STATE_HORDE, value);
}

void BattlegroundWS::UpdateTeamScore(uint32 team)
{
    if (team == ALLIANCE)
        UpdateWorldState(BG_WS_FLAG_CAPTURES_ALLIANCE, GetTeamScore(team));
    else
        UpdateWorldState(BG_WS_FLAG_CAPTURES_HORDE, GetTeamScore(team));
}

void BattlegroundWS::HandleAreaTrigger(Player *Source, uint32 Trigger)
{
    // this is wrong way to implement these things. On official it done by gameobject spell cast.
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    //uint32 SpellId = 0;
    //uint64 buff_guid = 0;
    switch(Trigger)
    {
        case 3686:                                          // Alliance elixir of speed spawn. Trigger not working, because located inside other areatrigger, can be replaced by IsWithinDist(object, dist) in Battleground::Update().
            //buff_guid = m_BgObjects[BG_WS_OBJECT_SPEEDBUFF_1];
            break;
        case 3687:                                          // Horde elixir of speed spawn. Trigger not working, because located inside other areatrigger, can be replaced by IsWithinDist(object, dist) in Battleground::Update().
            //buff_guid = m_BgObjects[BG_WS_OBJECT_SPEEDBUFF_2];
            break;
        case 3706:                                          // Alliance elixir of regeneration spawn
            //buff_guid = m_BgObjects[BG_WS_OBJECT_REGENBUFF_1];
            break;
        case 3708:                                          // Horde elixir of regeneration spawn
            //buff_guid = m_BgObjects[BG_WS_OBJECT_REGENBUFF_2];
            break;
        case 3707:                                          // Alliance elixir of berserk spawn
            //buff_guid = m_BgObjects[BG_WS_OBJECT_BERSERKBUFF_1];
            break;
        case 3709:                                          // Horde elixir of berserk spawn
            //buff_guid = m_BgObjects[BG_WS_OBJECT_BERSERKBUFF_2];
            break;
        case 3646:                                          // Alliance Flag spawn
            if (m_FlagState[BG_TEAM_HORDE] && !m_FlagState[BG_TEAM_ALLIANCE])
                if (GetHordeFlagPickerGUID() == Source->GetGUID())
                    EventPlayerCapturedFlag(Source);
            break;
        case 3647:                                          // Horde Flag spawn
            if (m_FlagState[BG_TEAM_ALLIANCE] && !m_FlagState[BG_TEAM_HORDE])
                if (GetAllianceFlagPickerGUID() == Source->GetGUID())
                    EventPlayerCapturedFlag(Source);
            break;
        case 3649:                                          // unk1
        case 3688:                                          // unk2
        case 4628:                                          // unk3
        case 4629:                                          // unk4
            break;
        default:
            sLog->outError("WARNING: Unhandled AreaTrigger in Battleground: %u", Trigger);
            Source->GetSession()->SendAreaTriggerMessage("Warning: Unhandled AreaTrigger in Battleground: %u", Trigger);
            break;
    }

    //if (buff_guid)
    //    HandleTriggerBuff(buff_guid,Source);
}

bool BattlegroundWS::SetupBattleground()
{
    // flags
    if (!AddObject(BG_WS_OBJECT_A_FLAG, BG_OBJECT_A_FLAG_WS_ENTRY, 1540.423f, 1481.325f, 351.8284f, 3.089233f, 0, 0, 0.9996573f, 0.02617699f, BG_WS_FLAG_RESPAWN_TIME/1000)
        || !AddObject(BG_WS_OBJECT_H_FLAG, BG_OBJECT_H_FLAG_WS_ENTRY, 916.0226f, 1434.405f, 345.413f, 0.01745329f, 0, 0, 0.008726535f, 0.9999619f, BG_WS_FLAG_RESPAWN_TIME/1000)
        // buffs
        || !AddObject(BG_WS_OBJECT_SPEEDBUFF_1, BG_OBJECTID_SPEEDBUFF_ENTRY, 1449.93f, 1470.71f, 342.6346f, -1.64061f, 0, 0, 0.7313537f, -0.6819983f, BUFF_RESPAWN_TIME)
        || !AddObject(BG_WS_OBJECT_SPEEDBUFF_2, BG_OBJECTID_SPEEDBUFF_ENTRY, 1005.171f, 1447.946f, 335.9032f, 1.64061f, 0, 0, 0.7313537f, 0.6819984f, BUFF_RESPAWN_TIME)
        || !AddObject(BG_WS_OBJECT_REGENBUFF_1, BG_OBJECTID_REGENBUFF_ENTRY, 1317.506f, 1550.851f, 313.2344f, -0.2617996f, 0, 0, 0.1305263f, -0.9914448f, BUFF_RESPAWN_TIME)
        || !AddObject(BG_WS_OBJECT_REGENBUFF_2, BG_OBJECTID_REGENBUFF_ENTRY, 1110.451f, 1353.656f, 316.5181f, -0.6806787f, 0, 0, 0.333807f, -0.9426414f, BUFF_RESPAWN_TIME)
        || !AddObject(BG_WS_OBJECT_BERSERKBUFF_1, BG_OBJECTID_BERSERKERBUFF_ENTRY, 1320.09f, 1378.79f, 314.7532f, 1.186824f, 0, 0, 0.5591929f, 0.8290376f, BUFF_RESPAWN_TIME)
        || !AddObject(BG_WS_OBJECT_BERSERKBUFF_2, BG_OBJECTID_BERSERKERBUFF_ENTRY, 1139.688f, 1560.288f, 306.8432f, -2.443461f, 0, 0, 0.9396926f, -0.3420201f, BUFF_RESPAWN_TIME)
        // alliance gates
        || !AddObject(BG_WS_OBJECT_DOOR_A_1, BG_OBJECT_DOOR_A_1_WS_ENTRY, 1503.335f, 1493.466f, 352.1888f, 3.115414f, 0, 0, 0.9999143f, 0.01308903f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_WS_OBJECT_DOOR_A_2, BG_OBJECT_DOOR_A_2_WS_ENTRY, 1492.478f, 1457.912f, 342.9689f, 3.115414f, 0, 0, 0.9999143f, 0.01308903f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_WS_OBJECT_DOOR_A_3, BG_OBJECT_DOOR_A_3_WS_ENTRY, 1468.503f, 1494.357f, 351.8618f, 3.115414f, 0, 0, 0.9999143f, 0.01308903f, RESPAWN_IMMEDIATELY)
        /*|| !AddObject(BG_WS_OBJECT_DOOR_A_4, BG_OBJECT_DOOR_A_4_WS_ENTRY, 1471.555f, 1458.778f, 362.6332f, 3.115414f, 0, 0, 0.9999143f, 0.01308903f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_WS_OBJECT_DOOR_A_5, BG_OBJECT_DOOR_A_5_WS_ENTRY, 1492.347f, 1458.34f, 342.3712f, -0.03490669f, 0, 0, 0.01745246f, -0.9998477f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_WS_OBJECT_DOOR_A_6, BG_OBJECT_DOOR_A_6_WS_ENTRY, 1503.466f, 1493.367f, 351.7352f, -0.03490669f, 0, 0, 0.01745246f, -0.9998477f, RESPAWN_IMMEDIATELY)
        */// horde gates
        || !AddObject(BG_WS_OBJECT_DOOR_H_1, BG_OBJECT_DOOR_H_1_WS_ENTRY, 949.1663f, 1423.772f, 345.6241f, -0.5756807f, -0.01673368f, -0.004956111f, -0.2839723f, 0.9586737f, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_WS_OBJECT_DOOR_H_2, BG_OBJECT_DOOR_H_2_WS_ENTRY, 953.0507f, 1459.842f, 340.6526f, -1.99662f, -0.1971825f, 0.1575096f, -0.8239487f, 0.5073641f, RESPAWN_IMMEDIATELY)
        /*|| !AddObject(BG_WS_OBJECT_DOOR_H_3, BG_OBJECT_DOOR_H_3_WS_ENTRY, 949.9523f, 1422.751f, 344.9273f, 0.0f, 0, 0, 0, 1, RESPAWN_IMMEDIATELY)
        || !AddObject(BG_WS_OBJECT_DOOR_H_4, BG_OBJECT_DOOR_H_4_WS_ENTRY, 950.7952f, 1459.583f, 342.1523f, 0.05235988f, 0, 0, 0.02617695f, 0.9996573f, RESPAWN_IMMEDIATELY)
*/)
    {
        sLog->outErrorDb("BatteGroundWS: Failed to spawn some object Battleground not created!");
        return false;
    }

    const uint32 toLockObjects[] = { BG_WS_OBJECT_DOOR_A_1, BG_WS_OBJECT_DOOR_A_2, BG_WS_OBJECT_DOOR_A_3, BG_WS_OBJECT_DOOR_H_1, BG_WS_OBJECT_DOOR_H_2 };
    GameObject* go;
    for (uint32 i = 0; i < sizeof(toLockObjects) / sizeof(uint32); i++)
    {
        go = GetBGObject(toLockObjects[i]);
        if (go)
            go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
    }

    WorldSafeLocsEntry const *sg = sWorldSafeLocsStore.LookupEntry(WS_GRAVEYARD_MAIN_ALLIANCE);
    if (!sg || !AddSpiritGuide(WS_SPIRIT_MAIN_ALLIANCE, sg->x, sg->y, sg->z, 3.124139f, ALLIANCE))
    {
        sLog->outErrorDb("BatteGroundWS: Failed to spawn Alliance spirit guide! Battleground not created!");
        return false;
    }

    sg = sWorldSafeLocsStore.LookupEntry(WS_GRAVEYARD_MAIN_HORDE);
    if (!sg || !AddSpiritGuide(WS_SPIRIT_MAIN_HORDE, sg->x, sg->y, sg->z, 3.193953f, HORDE))
    {
        sLog->outErrorDb("BatteGroundWS: Failed to spawn Horde spirit guide! Battleground not created!");
        return false;
    }

    sLog->outDebug("BatteGroundWS: BG objects and spirit guides spawned");

    return true;
}

void BattlegroundWS::Reset()
{
    //call parent's class reset
    Battleground::Reset();

    m_FlagKeepers[BG_TEAM_ALLIANCE]     = 0;
    m_FlagKeepers[BG_TEAM_HORDE]        = 0;
    m_DroppedFlagGUID[BG_TEAM_ALLIANCE] = 0;
    m_DroppedFlagGUID[BG_TEAM_HORDE]    = 0;
    m_FlagState[BG_TEAM_ALLIANCE]       = BG_WS_FLAG_STATE_ON_BASE;
    m_FlagState[BG_TEAM_HORDE]          = BG_WS_FLAG_STATE_ON_BASE;
    m_TeamScores[BG_TEAM_ALLIANCE]      = 0;
    m_TeamScores[BG_TEAM_HORDE]         = 0;
    bool isBGWeekend = sBattlegroundMgr->IsBGWeekend(GetTypeID());
    m_ReputationCapture = (isBGWeekend) ? 45 : 35;
    m_HonorWinKills = (isBGWeekend) ? 3 : 1;
    m_HonorEndKills = (isBGWeekend) ? 4 : 2;
    // For WorldState
    m_minutesElapsed                    = 0;
    m_LastFlagCaptureTeam               = 0;

    /* Spirit nodes is static at this BG and then not required deleting at BG reset.
    if (m_BgCreatures[WS_SPIRIT_MAIN_ALLIANCE])
        DelCreature(WS_SPIRIT_MAIN_ALLIANCE);
    if (m_BgCreatures[WS_SPIRIT_MAIN_HORDE])
        DelCreature(WS_SPIRIT_MAIN_HORDE);
    */
}

void BattlegroundWS::EndBattleground(uint32 winner)
{
    //there's no win reward sice 4.x.x

    Battleground::EndBattleground(winner);
}

void BattlegroundWS::HandleKillPlayer(Player *player, Player *killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    EventPlayerDroppedFlag(player);

    Battleground::HandleKillPlayer(player, killer);
}

void BattlegroundWS::UpdatePlayerScore(Player *Source, uint32 type, int32 value, bool doAddHonor)
{

    BattlegroundScoreMap::iterator itr = m_PlayerScores.find(Source->GetGUID());
    if (itr == m_PlayerScores.end())                         // player not found
        return;

    switch(type)
    {
        case SCORE_FLAG_CAPTURES:                           // flags captured
            ((BattlegroundWGScore*)itr->second)->FlagCaptures += value;
            Source->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, WS_OBJECTIVE_CAPTURE_FLAG);
            break;
        case SCORE_FLAG_RETURNS:                            // flags returned
            ((BattlegroundWGScore*)itr->second)->FlagReturns += value;
            Source->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, WS_OBJECTIVE_RETURN_FLAG);
            break;
        default:
            Battleground::UpdatePlayerScore(Source, type, value, doAddHonor);
            break;
    }
}

WorldSafeLocsEntry const* BattlegroundWS::GetClosestGraveYard(Player* player)
{
    //if status in progress, it returns main graveyards with spiritguides
    //else it will return the graveyard in the flagroom - this is especially good
    //if a player dies in preparation phase - then the player can't cheat
    //and teleport to the graveyard outside the flagroom
    //and start running around, while the doors are still closed
    if (player->GetBGTeam() == ALLIANCE)
    {
        if (GetStatus() == STATUS_IN_PROGRESS)
            return sWorldSafeLocsStore.LookupEntry(WS_GRAVEYARD_MAIN_ALLIANCE);
        else
            return sWorldSafeLocsStore.LookupEntry(WS_GRAVEYARD_FLAGROOM_ALLIANCE);
    }
    else
    {
        if (GetStatus() == STATUS_IN_PROGRESS)
            return sWorldSafeLocsStore.LookupEntry(WS_GRAVEYARD_MAIN_HORDE);
        else
            return sWorldSafeLocsStore.LookupEntry(WS_GRAVEYARD_FLAGROOM_HORDE);
    }
}

void BattlegroundWS::FillInitialWorldStates(WorldPacket& data)
{
    data << uint32(BG_WS_FLAG_CAPTURES_ALLIANCE) << uint32(GetTeamScore(ALLIANCE));
    data << uint32(BG_WS_FLAG_CAPTURES_HORDE) << uint32(GetTeamScore(HORDE));

    if (m_FlagState[BG_TEAM_ALLIANCE] == BG_WS_FLAG_STATE_ON_GROUND)
        data << uint32(BG_WS_FLAG_UNK_ALLIANCE) << uint32(-1);
    else if (m_FlagState[BG_TEAM_ALLIANCE] == BG_WS_FLAG_STATE_ON_PLAYER)
        data << uint32(BG_WS_FLAG_UNK_ALLIANCE) << uint32(1);
    else
        data << uint32(BG_WS_FLAG_UNK_ALLIANCE) << uint32(0);

    if (m_FlagState[BG_TEAM_HORDE] == BG_WS_FLAG_STATE_ON_GROUND)
        data << uint32(BG_WS_FLAG_UNK_HORDE) << uint32(-1);
    else if (m_FlagState[BG_TEAM_HORDE] == BG_WS_FLAG_STATE_ON_PLAYER)
        data << uint32(BG_WS_FLAG_UNK_HORDE) << uint32(1);
    else
        data << uint32(BG_WS_FLAG_UNK_HORDE) << uint32(0);

    data << uint32(BG_WS_FLAG_CAPTURES_MAX) << uint32(BG_WS_MAX_TEAM_SCORE);

    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        data << uint32(BG_WS_STATE_TIMER_ACTIVE) << uint32(1);
        data << uint32(BG_WS_STATE_TIMER) << uint32(25-m_minutesElapsed);
    }
    else
        data << uint32(BG_WS_STATE_TIMER_ACTIVE) << uint32(0);

    if (m_FlagState[BG_TEAM_HORDE] == BG_WS_FLAG_STATE_ON_PLAYER)
        data << uint32(BG_WS_FLAG_STATE_ALLIANCE) << uint32(2);
    else
        data << uint32(BG_WS_FLAG_STATE_ALLIANCE) << uint32(1);

    if (m_FlagState[BG_TEAM_ALLIANCE] == BG_WS_FLAG_STATE_ON_PLAYER)
        data << uint32(BG_WS_FLAG_STATE_HORDE) << uint32(2);
    else
        data << uint32(BG_WS_FLAG_STATE_HORDE) << uint32(1);

}

