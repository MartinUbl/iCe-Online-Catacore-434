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

#include "HMACSHA1.h"
#include "BigNumber.h"

HmacHash::HmacHash(uint32 len, uint8 *seed)
{
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    m_pCtx = HMAC_CTX_new();
    HMAC_Init_ex(m_pCtx, seed, len, EVP_sha1(), NULL);
#else
    HMAC_CTX_init(&m_ctx);
    HMAC_Init_ex(&m_ctx, seed, len, EVP_sha1(), NULL);
#endif
}

HmacHash::~HmacHash()
{
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    HMAC_CTX_free(m_pCtx);
#else
    HMAC_CTX_cleanup(&m_ctx);
#endif
}

void HmacHash::UpdateBigNumber(BigNumber *bn)
{
    UpdateData(bn->AsByteArray(), bn->GetNumBytes());
}

void HmacHash::UpdateData(const uint8 *data, int length)
{
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    HMAC_Update(m_pCtx, data, length);
#else
    HMAC_Update(&m_ctx, data, length);
#endif
}

void HmacHash::UpdateData(const std::string &str)
{
    UpdateData((uint8 const*)str.c_str(), str.length());
}

void HmacHash::Finalize()
{
    uint32 length = 0;
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    HMAC_Final(m_pCtx, (uint8*)m_digest, &length);
#else
    HMAC_Final(&m_ctx, (uint8*)m_digest, &length);
#endif
    ASSERT(length == SHA_DIGEST_LENGTH)
}

uint8 *HmacHash::ComputeHash(BigNumber *bn)
{
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    HMAC_Update(m_pCtx, bn->AsByteArray(), bn->GetNumBytes());
#else
    HMAC_Update(&m_ctx, bn->AsByteArray(), bn->GetNumBytes());
#endif
    Finalize();
    return (uint8*)m_digest;
}
