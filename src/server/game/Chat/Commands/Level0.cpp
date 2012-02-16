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
#include "DatabaseEnv.h"
#include "World.h"
#include "Player.h"
#include "Opcodes.h"
#include "Chat.h"
#include "ObjectAccessor.h"
#include "Language.h"
#include "AccountMgr.h"
#include "SystemConfig.h"
#include "Util.h"
#include "SpellAuras.h"

// Here only due to size of this file - quick Edit and continue build
bool ChatHandler::HandleTestCommand(const char* args)
{
    // Fnction ONLY for DEBUG purposes and only for use under DEBUG BUILD !!
    // No other reason for this function
    Player* player = m_session->GetPlayer();
    if (!player)
        return false;

    return true;
}

bool ChatHandler::HandleHelpCommand(const char* args)
{
    char* cmd = strtok((char*)args, " ");
    if (!cmd)
    {
        ShowHelpForCommand(getCommandTable(), "help");
        ShowHelpForCommand(getCommandTable(), "");
    }
    else
    {
        if (!ShowHelpForCommand(getCommandTable(), cmd))
            SendSysMessage(LANG_NO_HELP_CMD);
    }

    return true;
}

bool ChatHandler::HandleCommandsCommand(const char* /*args*/)
{
    ShowHelpForCommand(getCommandTable(), "");
    return true;
}

bool ChatHandler::HandleAccountCommand(const char* /*args*/)
{
    AccountTypes gmlevel = m_session->GetSecurity();
    PSendSysMessage(LANG_ACCOUNT_LEVEL, uint32(gmlevel));
    return true;
}

bool ChatHandler::HandleUnstuckCommand(const char* /*args*/)
{
    Player *chr = m_session->GetPlayer();

    if (chr->isInFlight())
    {
        SendSysMessage(LANG_YOU_IN_FLIGHT);
        SetSentErrorMessage(true);
        return false;
    }

    if (chr->isInCombat())
    {
        SendSysMessage(LANG_YOU_IN_COMBAT);
        SetSentErrorMessage(true);
        return false;
    }

    // cast spell Stuck
    chr->CastSpell(chr, 7355, false);
    return true;
}

bool ChatHandler::HandlePetResetCommand(const char* /*args*/)
{
    // Pokud nejsme hunteri, fuck off
    if(m_session->GetPlayer()->getClass() != CLASS_HUNTER)
        return false;

    if(m_session->GetPlayer()->GetPet())
    {
        PSendSysMessage("Nejdrive dismissnete sveho peta!");
        return true;
    }

    PSendSysMessage("Reset hunter pet slotu...");

    // Nastavit specialni AT_LOGIN flagu pro reset currentPetSlot u hrace (nutne, nejde za behu)
    m_session->GetPlayer()->SetAtLoginFlag(AT_LOGIN_RESET_PET_SLOT);
    m_session->GetPlayer()->m_petSlotUsed = 0;
    PSendSysMessage("AT_LOGIN flag nastavena");

    // Vybrat vsechny pety
    QueryResult qr = CharacterDatabase.PQuery("SELECT id,name FROM character_pet WHERE owner = %u",m_session->GetPlayer()->GetGUID());
    if(!qr)
    {
        PSendSysMessage("Nemate zadneho peta, bude vam nejspis stacit jen relog. Pokud ne, kontaktujte spravce.");
        return true;
    }
    std::map<uint32,std::string> PetEntrys;
    Field* fd = qr->Fetch();
    // A seradit je do mapy entry+jmeno
    while(fd)
    {
        PetEntrys[fd[0].GetUInt32()] = fd[1].GetCString();
        if(!qr->NextRow())
            break;
        fd = qr->Fetch();
    }
    PSendSysMessage("Nalezeno %u petu",uint32(PetEntrys.size()));
    // Pokud jsme nenasli zadneho peta, vratime se, protoze by proces zfailoval (begin == end)
    if(PetEntrys.size() < 1)
    {
        PSendSysMessage("Nemate zadneho peta!");
        return true;
    }

    // Projdeme nasi mapu, seradime pety v databazi, masku petSlotUsed prenastavime na platnou hodnotu
    uint32 i = 0;
    for(std::map<uint32,std::string>::const_iterator itr = PetEntrys.begin(); itr != PetEntrys.end(); itr++)
    {
        CharacterDatabase.PQuery("UPDATE character_pet SET slot=%u WHERE id=%u",i,(*itr).first);
        PSendSysMessage("Pet %s -> slot %u",(*itr).second.c_str(),i);
        m_session->GetPlayer()->setPetSlotUsed((PetSlot)i,true);
        i++;
    }
    PSendSysMessage("Proces hotov, pro projeveni zmen provedte relog.");
    return true;
}

bool ChatHandler::HandleServerInfoCommand(const char* /*args*/)
{
    uint32 PlayersNum = sWorld->GetPlayerCount();
    uint32 MaxPlayersNum = sWorld->GetMaxPlayerCount();
    uint32 activeClientsNum = sWorld->GetActiveSessionCount();
    uint32 queuedClientsNum = sWorld->GetQueuedSessionCount();
    uint32 maxActiveClientsNum = sWorld->GetMaxActiveSessionCount();
    uint32 maxQueuedClientsNum = sWorld->GetMaxQueuedSessionCount();
    std::string uptime = secsToTimeString(sWorld->GetUptime());
    //uint32 updateTime = sWorld->GetUpdateTime();

    PSendSysMessage(LANG_CONNECTED_PLAYERS, PlayersNum, MaxPlayersNum);
    PSendSysMessage(LANG_CONNECTED_USERS, activeClientsNum, maxActiveClientsNum, queuedClientsNum, maxQueuedClientsNum);
    PSendSysMessage(LANG_UPTIME, uptime.c_str());
    //PSendSysMessage("Update time diff: %u.", updateTime); // This is useless

    return true;
}

bool ChatHandler::HandleDismountCommand(const char* /*args*/)
{
    //If player is not mounted, so go out :)
    if (!m_session->GetPlayer()->IsMounted() && !m_session->GetPlayer()->IsMountedShape())
    {
        SendSysMessage(LANG_CHAR_NON_MOUNTED);
        SetSentErrorMessage(true);
        return false;
    }

    if (m_session->GetPlayer()->isInFlight())
    {
        SendSysMessage(LANG_YOU_IN_FLIGHT);
        SetSentErrorMessage(true);
        return false;
    }

    m_session->GetPlayer()->Unmount();
    m_session->GetPlayer()->RemoveAurasByType(SPELL_AURA_MOUNTED);
    return true;
}

bool ChatHandler::HandleSaveCommand(const char* /*args*/)
{
    Player *player = m_session->GetPlayer();

    // save GM account without delay and output message
    if (m_session->GetSecurity() > SEC_PLAYER)
    {
        player->SaveToDB();
        SendSysMessage("Player saved (GM).");
        return true;
    }

    /* disallow player-induced save sooner than PlayerSaveInterval/2,
     * ie. PlayerSaveInterval (config) = 900sec,
     * allow manual save each 450sec + reset the timer */
    uint32 PlayerSaveInterval = sWorld->getIntConfig(CONFIG_INTERVAL_SAVE);
    if (PlayerSaveInterval > 0) {

        time_t now = time(NULL);
        time_t diff = now - player->GetLastManualSave();

        if (diff >= PlayerSaveInterval/2000) {  /* 2000 = x / 2 / 1000 (ms->sec) */
            player->SetLastManualSave(now);
            player->SaveToDB();                 /* SaveToDB also resets autosave timer */
            SendSysMessage("Player saved.");
            return true;

        } else {
            diff = PlayerSaveInterval/2000 - diff;
            PSendSysMessage("Player NOT saved, wait %lu seconds.", (unsigned long)diff);
            return true;
        }
    }

    return true;
}

bool ChatHandler::HandleGMListIngameCommand(const char* /*args*/)
{
    bool first = true;

    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, *HashMapHolder<Player>::GetLock(), true);
    HashMapHolder<Player>::MapType &m = sObjectAccessor->GetPlayers();
    for (HashMapHolder<Player>::MapType::const_iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        AccountTypes itr_sec = itr->second->GetSession()->GetSecurity();
        if ((itr->second->isGameMaster() || (itr_sec > SEC_PLAYER && itr_sec <= AccountTypes(sWorld->getIntConfig(CONFIG_GM_LEVEL_IN_GM_LIST)))) &&
            (!m_session || itr->second->IsVisibleGloballyFor(m_session->GetPlayer())))
        {
            if (first)
            {
                SendSysMessage(LANG_GMS_ON_SRV);
                first = false;
            }

            SendSysMessage(GetNameLink(itr->second).c_str());
        }
    }

    if (first)
        SendSysMessage(LANG_GMS_NOT_LOGGED);

    return true;
}

bool ChatHandler::HandleAccountPasswordCommand(const char* args)
{
    if (!*args)
    {
        SendSysMessage(LANG_CMD_SYNTAX);
        SetSentErrorMessage(true);
        return false;
    }

    char *old_pass = strtok((char*)args, " ");
    char *new_pass = strtok(NULL, " ");
    char *new_pass_c  = strtok(NULL, " ");

    if (!old_pass || !new_pass || !new_pass_c)
    {
        SendSysMessage(LANG_CMD_SYNTAX);
        SetSentErrorMessage(true);
        return false;
    }

    std::string password_old = old_pass;
    std::string password_new = new_pass;
    std::string password_new_c = new_pass_c;

    if (!sAccountMgr->CheckPassword(m_session->GetAccountId(), password_old))
    {
        SendSysMessage(LANG_COMMAND_WRONGOLDPASSWORD);
        SetSentErrorMessage(true);
        return false;
    }

    if (strcmp(new_pass, new_pass_c) != 0)
    {
        SendSysMessage(LANG_NEW_PASSWORDS_NOT_MATCH);
        SetSentErrorMessage(true);
        return false;
    }

    AccountOpResult result = sAccountMgr->ChangePassword(m_session->GetAccountId(), password_new);
    switch(result)
    {
        case AOR_OK:
            SendSysMessage(LANG_COMMAND_PASSWORD);
            break;
        case AOR_PASS_TOO_LONG:
            SendSysMessage(LANG_PASSWORD_TOO_LONG);
            SetSentErrorMessage(true);
            return false;
        default:
            SendSysMessage(LANG_COMMAND_NOTCHANGEPASSWORD);
            SetSentErrorMessage(true);
            return false;
    }

    return true;
}

bool ChatHandler::HandleAccountAddonCommand(const char* args)
{
    if (!*args)
    {
        SendSysMessage(LANG_CMD_SYNTAX);
        SetSentErrorMessage(true);
        return false;
    }

    char *szExp = strtok((char*)args, " ");

    uint32 account_id = m_session->GetAccountId();

    int expansion = atoi(szExp);                                    //get int anyway (0 if error)
    if (expansion < 0 || uint8(expansion) > sWorld->getIntConfig(CONFIG_EXPANSION))
    {
        SendSysMessage(LANG_IMPROPER_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    // No SQL injection
    LoginDatabase.PExecute("UPDATE account SET expansion = '%d' WHERE id = '%u'", expansion, account_id);
    PSendSysMessage(LANG_ACCOUNT_ADDON, expansion);
    return true;
}

bool ChatHandler::HandleAccountLockCommand(const char* args)
{
    if (!*args)
    {
        SendSysMessage(LANG_USE_BOL);
        SetSentErrorMessage(true);
        return false;
    }

    std::string argstr = (char*)args;
    if (argstr == "on")
    {
        LoginDatabase.PExecute("UPDATE account SET locked = '1' WHERE id = '%d'",m_session->GetAccountId());
        PSendSysMessage(LANG_COMMAND_ACCLOCKLOCKED);
        return true;
    }

    if (argstr == "off")
    {
        LoginDatabase.PExecute("UPDATE account SET locked = '0' WHERE id = '%d'",m_session->GetAccountId());
        PSendSysMessage(LANG_COMMAND_ACCLOCKUNLOCKED);
        return true;
    }

    SendSysMessage(LANG_USE_BOL);
    SetSentErrorMessage(true);
    return false;
}

/// Display the 'Message of the day' for the realm
bool ChatHandler::HandleServerMotdCommand(const char* /*args*/)
{
    PSendSysMessage(LANG_MOTD_CURRENT, sWorld->GetMotd());
    return true;
}

