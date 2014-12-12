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

#ifndef _AUCTION_HOUSE_MGR_H
#define _AUCTION_HOUSE_MGR_H

#include <ace/Singleton.h>

#include "AuctionHouseObject.h"
#include "Common.h"
#include "DatabaseEnv.h"
#include "DBCStructure.h"

class Item;
class Player;
class WorldPacket;

#define MIN_AUCTION_TIME (12*HOUR)

enum AuctionError
{
    ERR_AUCTION_OK                  = 0,
    ERR_AUCTION_INVENTORY           = 1,
    ERR_AUCTION_DATABASE_ERROR      = 2,
    ERR_AUCTION_NOT_ENOUGHT_MONEY   = 3,
    ERR_AUCTION_ITEM_NOT_FOUND      = 4,
    ERR_AUCTION_HIGHER_BID          = 5,
    ERR_AUCTION_BID_INCREMENT       = 7,
    ERR_AUCTION_BID_OWN             = 10,
    ERR_RESTRICTED_ACCOUNT          = 13,
};

enum AuctionAction
{
    AUCTION_SELL_ITEM = 0,
    AUCTION_CANCEL = 1,
    AUCTION_PLACE_BID = 2
};

struct AuctionEntry
{
    uint32 Id;
    uint32 auctioneer;                                      // creature low guid
    uint32 item_guidlow;
    uint32 itemEntry;
    uint64 owner;
    uint64 startbid;                                        //maybe useless
    uint64 bid;
    uint32 buyout;
    time_t expire_time;
    uint64 bidder;
    uint32 deposit;                                         //deposit can be calculated only when creating auction
    AuctionHouseEntry const* auctionHouseEntry;             // in AuctionHouse.dbc
    uint32 factionTemplateId;

    // helpers
    uint32 GetHouseId() const { return auctionHouseEntry->houseId; }
    uint32 GetHouseFaction() const { return auctionHouseEntry->faction; }
    uint32 GetAuctionCut() const;
    uint64 GetAuctionOutBid() const;
    bool BuildAuctionInfo(WorldPacket & data) const;
    void DeleteFromDB(SQLTransaction& trans) const;
    void SaveToDB(SQLTransaction& trans) const;
    bool LoadFromDB(Field* fields);

    ItemQualities GetItemQuality() const;
    uint32 GetRequiredLevel() const;
    time_t GetExpireTime() const;
    std::string GetOwnerName() const;
    uint64 GetCurrentBid() const;
};

class AuctionHouseMgr
{
    friend class ACE_Singleton<AuctionHouseMgr, ACE_Null_Mutex>;
    AuctionHouseMgr();
    ~AuctionHouseMgr();

    public:

        typedef std::unordered_map<uint32, Item*> ItemMap;

        AuctionHouseObject* GetAuctionsMap(uint32 factionTemplateId);
        AuctionHouseObject* GetBidsMap(uint32 factionTemplateId);

        Item* GetAItem(uint32 id)
        {
            ItemMap::const_iterator itr = mAitems.find(id);
            if (itr != mAitems.end())
                return itr->second;

            return NULL;
        }

        //auction messages
        void SendAuctionWonMail(AuctionEntry * auction, SQLTransaction& trans);
        void SendAuctionSalePendingMail(AuctionEntry * auction, SQLTransaction& trans);
        void SendAuctionSuccessfulMail(AuctionEntry * auction, SQLTransaction& trans);
        void SendAuctionExpiredMail(AuctionEntry * auction, SQLTransaction& trans);
        void SendAuctionRemovedMail(AuctionEntry * auction, SQLTransaction& trans);
        void SendAuctionOutbiddedMail(AuctionEntry * auction, uint64 newPrice, Player* newBidder, SQLTransaction& trans);
        void SendAuctionCancelledToBidderMail(AuctionEntry* auction, SQLTransaction& trans, Item* item);

        static uint32 GetAuctionDeposit(AuctionHouseEntry const* entry, uint32 time, ItemPrototype const *pItem, uint32 count);
        static AuctionHouseEntry const* GetAuctionHouseEntry(uint32 factionTemplateId);

    public:

        //load first auction items, because of check if item exists, when loading
        void LoadAuctionItems();
        void LoadAuctions();

        void AddAItem(Item* it);
        bool RemoveAItem(uint32 id);

        void Update();

    private:

        AuctionHouseObject mHordeAuctions;
        AuctionHouseObject mAllianceAuctions;
        AuctionHouseObject mNeutralAuctions;

        ItemMap mAitems;
};

#define sAuctionMgr ACE_Singleton<AuctionHouseMgr, ACE_Null_Mutex>::instance()

#endif
