/*
 * Copyright (C) 2006-2013 iCe Online <http://ice-wow.eu/>
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
#include "Config.h"
#include "Log.h"
#include "APIRunnable.h"
#include "World.h"

#include <ace/Reactor_Impl.h>
#include <ace/TP_Reactor.h>
#include <ace/Dev_Poll_Reactor.h>
#include <ace/Acceptor.h>
#include <ace/SOCK_Acceptor.h>

#include "APISocket.h"

APIRunnable::APIRunnable() : m_Reactor(NULL)
{
    ACE_Reactor_Impl* imp = 0;

#if defined (ACE_HAS_EVENT_POLL) || defined (ACE_HAS_DEV_POLL)

    imp = new ACE_Dev_Poll_Reactor();

    imp->max_notify_iterations (128);
    imp->restart (1);

#else

    imp = new ACE_TP_Reactor();
    imp->max_notify_iterations (128);

#endif

    m_Reactor = new ACE_Reactor (imp, 1);
}

APIRunnable::~APIRunnable()
{
    delete m_Reactor;
}

void APIRunnable::run()
{
    if (!sConfig->GetBoolDefault("API.Enable", false))
        return;

    std::string ip = sConfig->GetStringDefault("API.IP", "127.0.0.1");
    std::string port = sConfig->GetStringDefault("API.Port", "3444");

    APISocket api;

    if (api.open(ip.c_str(), port.c_str()) == -1)
    {
        sLog->outError("API runnable can not bind to port %s on %s", port.c_str(), ip.c_str());
        return;
    }

    sLog->outString("Starting API server on port %s on %s", port.c_str(), ip.c_str());

    while (!World::IsStopped())
    {
        api.run();
    }

    api.destroy();

    sLog->outStaticDebug("API server thread exiting");
}
