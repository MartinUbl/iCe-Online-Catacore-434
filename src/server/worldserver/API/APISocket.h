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

#ifndef _APISOCKET_H
#define _APISOCKET_H

#include "Common.h"

#define API_REQUEST_BUFFER_SIZE 4096

class APISocket
{
    public:
        APISocket();
        virtual ~APISocket();

        void run();
        int open(const char* host, const char* port);
        void destroy();

    private:
        int read(std::string &cmd, std::vector<std::string> &args);
        int handle(std::string &cmd, std::vector<std::string> &args);
        int send(const char* line);

        int m_fd;
        sockaddr *m_claddr;
        socklen_t m_claddrlen;
        char m_buff[API_REQUEST_BUFFER_SIZE];
};
#endif
/// @}
