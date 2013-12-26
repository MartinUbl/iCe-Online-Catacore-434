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

#include "Common.h"
#include "Configuration/Config.h"
#include "Database/DatabaseEnv.h"
#include "AccountMgr.h"
#include "Log.h"
#include "APISocket.h"
#include "Util.h"
#include "World.h"

APISocket::APISocket()
{
}

APISocket::~APISocket()
{
}

int APISocket::open(const char* host, const char* port)
{
    int rc;
    addrinfo *ainfo, hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    rc = getaddrinfo(host, port, &hints, &ainfo);
    if (rc != 0)
        return -1;

    m_fd = socket(ainfo->ai_family, ainfo->ai_socktype, ainfo->ai_protocol);
    if (m_fd < 0)
        return -1;

    rc = 1;

#ifdef _WIN32
    if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)(&rc), sizeof(rc)) == -1) {
        return -1;
    }
#else
    if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &rc, sizeof(rc)) == -1) {
        return -1;
    }
#endif

    if (bind(m_fd, ainfo->ai_addr, ainfo->ai_addrlen) == -1)
        return -1;

    m_claddrlen = ainfo->ai_addrlen;
    m_claddr = (sockaddr*)malloc(m_claddrlen);

    freeaddrinfo(ainfo);
    return 0;
}

void APISocket::destroy()
{
    close(m_fd);
}

int APISocket::send(const char* line)
{
    return sendto(m_fd, line, strlen(line), 0, m_claddr, m_claddrlen);
}

void APISocket::run()
{
    std::string cmd;
    std::vector<std::string> args;

    if (read(cmd, args) > 0)
        handle(cmd, args);
}

int APISocket::read(std::string &cmd, std::vector<std::string> &args)
{
    int readsize;

    readsize = recvfrom(m_fd, m_buff, API_REQUEST_BUFFER_SIZE, 0, m_claddr, &m_claddrlen);

    if (readsize <= 0)
        return 0;

    // fetch the first token (space or endline used as delimiter)
    cmd.clear();
    args.clear();
    int i = 0;
    for (i = 0; i < readsize; i++)
    {
        if (m_buff[i] == ' ' || m_buff[i] == '\n')
            break;

        cmd += m_buff[i];
    }

    std::string tmparg = "";
    for (int j = i+1; j < readsize; j++)
    {
        if (m_buff[j] == ' ' || m_buff[j] == '\n')
        {
            if (!tmparg.empty())
            {
                args.resize(args.size()+1);
                args[args.size()-1] = tmparg.c_str();
            }
            tmparg.clear();
        }
        else
        {
            tmparg += m_buff[j];
        }
    }

    if (!tmparg.empty())
    {
        args.resize(args.size()+1);
        args[args.size()-1] = tmparg.c_str();
    }

    return readsize;
}

int APISocket::handle(std::string &cmd, std::vector<std::string> &args)
{
    //std::string command = buffer;

    if (cmd.length() == 0)
        return 0;

    // exit
    if (cmd == "quit" || cmd == "exit")
        return -1;

    std::string response;

    if (cmd == "ping")
    {
        response += "pong";
    }
    else if (cmd == "onlineplayers")
    {
        char resp[10];
        if (args.empty())
        {
            sprintf(resp, "%u", sWorld->GetPlayerCount());
            response += resp;
        }
        else if (args.size() == 1)
        {
            if (args[0] == "max")
            {
                sprintf(resp, "%u", sWorld->GetMaxPlayerCount());
                response += resp;
            }
            else if (args[0] == "alliance")
            {
                sprintf(resp, "%u", sWorld->GetPlayerCountAlliance());
                response += resp;
            }
            else if (args[0] == "horde")
            {
                sprintf(resp, "%u", sWorld->GetPlayerCountHorde());
                response += resp;
            }
            else
            {
                response += "params?";
            }
        }
        else
            response += "params?";
    }
    else if (cmd == "uptime")
    {
        char resp[10];
        sprintf(resp, "%u", sWorld->GetUptime());
        response += resp;
    }
    else
    {
        response = "error";
    }

    response += '\n';

    send(response.c_str());

    return 0;
}
