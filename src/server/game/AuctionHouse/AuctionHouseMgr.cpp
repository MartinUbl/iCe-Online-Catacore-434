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
#include "ObjectMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "DatabaseEnv.h"
#include "SQLStorage.h"
#include "DBCStores.h"
#include "ScriptMgr.h"
#include "AccountMgr.h"
#include "AuctionHouseMgr.h"
#include "Item.h"
#include "Language.h"
#include "Logging/Log.h"
#include <vector>

enum eAuctionHouse
{
    AH_MINIMUM_DEPOSIT = 100,
};

AuctionHouseMgr::AuctionHouseMgr()
{
}

AuctionHouseMgr::~AuctionHouseMgr()
{
    for (ItemMap::iterator itr = mAitems.begin(); itr != mAitems.end(); ++itr)
        delete itr->second;
}

AuctionHouseObject * AuctionHouseMgr::GetAuctionsMap(uint32 factionTemplateId)
{
    if (sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
        return &mNeutralAuctions;

    // team have linked auction houses
    FactionTemplateEntry const* u_entry = sFactionTemplateStore.LookupEntry(factionTemplateId);
    if (!u_entry)
        return &mNeutralAuctions;
    else if (u_entry->ourMask & FACTION_MASK_ALLIANCE)
        return &mAllianceAuctions;
    else if (u_entry->ourMask & FACTION_MASK_HORDE)
        return &mHordeAuctions;
    else
        return &mNeutralAuctions;
}

uint32 AuctionHouseMgr::GetAuctionDeposit(AuctionHouseEntry const* entry, uint32 time, ItemPrototype const *pItem, uint32 count)
{
    uint32 MSV = pItem->SellPrice;

    if (MSV <= 0)
        return AH_MINIMUM_DEPOSIT;

    uint32 timeHr = (((time / 60) / 60) /12);
    float multiplier = (float)(entry->depositPercent * 3) / 100.0f;
    uint32 deposit = ((uint32)((float)MSV * multiplier * (float)count)/3) * 3 * timeHr;

    sLog->outDebug("MSV:        %u", MSV);
    sLog->outDebug("Items:      %u", count);
    sLog->outDebug("Multiplier: %f", multiplier);
    sLog->outDebug("Deposit:    %u", deposit);

    if (deposit < AH_MINIMUM_DEPOSIT)
        return AH_MINIMUM_DEPOSIT;
    else
        return deposit;
}

//does not clear ram
void AuctionHouseMgr::SendAuctionWonMail(AuctionEntry *auction, SQLTransaction& trans)
{
    Item *pItem = GetAItem(auction->item_guidlow);
    if (!pItem)
        return;

    uint32 bidder_accId = 0;
    uint64 bidder_guid = MAKE_NEW_GUID(auction->bidder, 0, HIGHGUID_PLAYER);
    Player *bidder = sObjectMgr->GetPlayer(bidder_guid);
    // data for gm.log
    if (sWorld->getBoolConfig(CONFIG_GM_LOG_TRADE))
    {
        uint32 bidder_security = 0;
        std::string bidder_name;
        if (bidder)
        {
            bidder_accId = bidder->GetSession()->GetAccountId();
            bidder_security = bidder->GetSession()->GetSecurity();
            bidder_name = bidder->GetName();
        }
        else
        {
            bidder_accId = sObjectMgr->GetPlayerAccountIdByGUID(bidder_guid);
            bidder_security = sAccountMgr->GetSecurity(bidder_accId);

            if (bidder_security > SEC_PLAYER) // not do redundant DB requests
            {
                if (!sObjectMgr->GetPlayerNameByGUID(bidder_guid,bidder_name))
                    bidder_name = sObjectMgr->GetTrinityStringForDBCLocale(LANG_UNKNOWN);
            }
        }
        {
            std::string owner_name;
            if (!sObjectMgr->GetPlayerNameByGUID(auction->owner,owner_name))
                owner_name = sObjectMgr->GetTrinityStringForDBCLocale(LANG_UNKNOWN);

            uint32 owner_accid = sObjectMgr->GetPlayerAccountIdByGUID(auction->owner);

            if (bidder_security > SEC_PLAYER)
                sLog->outCommand(bidder_accId,"GM %s (Account: %u) won item in auction: %s (Entry: %u Count: %u) and pay money: " UI64FMTD ". Original owner %s (Account: %u)",
                    bidder_name.c_str(),bidder_accId,pItem->GetProto()->Name1,pItem->GetEntry(),pItem->GetCount(),auction->bid,owner_name.c_str(),owner_accid);

            sLog->outChar("IP:(%s) account:(%u) character:(%s) action:(%s) %s:(name:(%s) entry:(%u) count:(%u)) cost:(" UI64FMTD ") %s:(name:(%s) account:(%u))",
                         bidder ? bidder->GetSession()->GetRemoteAddress().c_str() : "none",
                         bidder_accId,
                         bidder_name.c_str(),
                         "won auction",
                           "item",
                           pItem->GetProto()->Name1,
                           pItem->GetEntry(),
                           pItem->GetCount(),
                         auction->bid,
                           "seller",
                           owner_name.c_str(),
                           owner_accid);
        }
    }

    // receiver exist
    if (bidder || bidder_accId)
    {
        std::ostringstream msgAuctionWonSubject;
        msgAuctionWonSubject << auction->itemEntry << ":0:" << AUCTION_WON;

        std::ostringstream msgAuctionWonBody;
        msgAuctionWonBody.width(16);
        msgAuctionWonBody << std::right << std::hex << auction->owner;
        msgAuctionWonBody << std::dec << ":" << auction->bid << ":" << auction->buyout;
        sLog->outDebug("AuctionWon body string : %s", msgAuctionWonBody.str().c_str());

        // set owner to bidder (to prevent delete item with sender char deleting)
        // owner in `data` will set at mail receive and item extracting
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SET_ITEM_OWNER);
        stmt->setUInt32(0, auction->bidder);
        stmt->setUInt32(1, pItem->GetGUIDLow());
        trans->Append(stmt);

        if (bidder)
        {
            bidder->GetSession()->SendAuctionBidderNotification(auction->GetHouseId(), auction->Id, bidder_guid, 0, 0, auction->itemEntry);
            // FIXME: for offline player need also
            bidder->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_WON_AUCTIONS, 1);
        }

        MailDraft(msgAuctionWonSubject.str(), msgAuctionWonBody.str())
            .AddItem(pItem)
            .SendMailTo(trans, MailReceiver(bidder, auction->bidder), auction, MAIL_CHECK_MASK_COPIED);
    }
}

void AuctionHouseMgr::SendAuctionSalePendingMail(AuctionEntry * auction, SQLTransaction& trans)
{
    uint64 owner_guid = MAKE_NEW_GUID(auction->owner, 0, HIGHGUID_PLAYER);
    Player *owner = sObjectMgr->GetPlayer(owner_guid);
    uint32 owner_accId = sObjectMgr->GetPlayerAccountIdByGUID(owner_guid);
    // owner exist (online or offline)
    if (owner || owner_accId)
    {
        std::ostringstream msgAuctionSalePendingSubject;
        msgAuctionSalePendingSubject << auction->itemEntry << ":0:" << AUCTION_SALE_PENDING;

        std::ostringstream msgAuctionSalePendingBody;
        uint32 auctionCut = auction->GetAuctionCut();

        time_t distrTime = time(NULL) + sWorld->getIntConfig(CONFIG_MAIL_DELIVERY_DELAY);

        msgAuctionSalePendingBody.width(16);
        msgAuctionSalePendingBody << std::right << std::hex << auction->bidder;
        msgAuctionSalePendingBody << std::dec << ":" << auction->bid << ":" << auction->buyout;
        msgAuctionSalePendingBody << ":" << auction->deposit << ":" << auctionCut << ":0:";
        msgAuctionSalePendingBody << secsToTimeBitFields(distrTime);

        sLog->outDebug("AuctionSalePending body string : %s", msgAuctionSalePendingBody.str().c_str());

        MailDraft(msgAuctionSalePendingSubject.str(), msgAuctionSalePendingBody.str())
            .SendMailTo(trans, MailReceiver(owner,auction->owner), auction, MAIL_CHECK_MASK_COPIED);
    }
}

//call this method to send mail to auction owner, when auction is successful, it does not clear ram
void AuctionHouseMgr::SendAuctionSuccessfulMail(AuctionEntry * auction, SQLTransaction& trans)
{
    uint64 owner_guid = MAKE_NEW_GUID(auction->owner, 0, HIGHGUID_PLAYER);
    Player *owner = sObjectMgr->GetPlayer(owner_guid);
    uint32 owner_accId = sObjectMgr->GetPlayerAccountIdByGUID(owner_guid);
    // owner exist
    if (owner || owner_accId)
    {
        std::ostringstream msgAuctionSuccessfulSubject;
        msgAuctionSuccessfulSubject << auction->itemEntry << ":0:" << AUCTION_SUCCESSFUL;

        std::ostringstream auctionSuccessfulBody;
        uint32 auctionCut = auction->GetAuctionCut();

        auctionSuccessfulBody.width(16);
        auctionSuccessfulBody << std::right << std::hex << auction->bidder;
        auctionSuccessfulBody << std::dec << ":" << auction->bid << ":" << auction->buyout;
        auctionSuccessfulBody << ":" << auction->deposit << ":" << auctionCut;

        sLog->outDebug("AuctionSuccessful body string : %s", auctionSuccessfulBody.str().c_str());

        uint64 profit = auction->bid + auction->deposit - auctionCut;

        //FIXME: what do if owner offline
        if (owner)
        {
            owner->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS, profit);
            owner->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_SOLD, auction->bid);
            //send auction owner notification, bidder must be current!
            owner->GetSession()->SendAuctionOwnerNotification(auction);
        }
        MailDraft(msgAuctionSuccessfulSubject.str(), auctionSuccessfulBody.str())
            .AddMoney(profit)
            .SendMailTo(trans, MailReceiver(owner,auction->owner), auction, MAIL_CHECK_MASK_COPIED, sWorld->getIntConfig(CONFIG_MAIL_DELIVERY_DELAY));
    }
}

//does not clear ram
void AuctionHouseMgr::SendAuctionExpiredMail(AuctionEntry * auction, SQLTransaction& trans)
{
    //return an item in auction to its owner by mail
    Item *pItem = GetAItem(auction->item_guidlow);
    if (!pItem)
        return;

    uint64 owner_guid = MAKE_NEW_GUID(auction->owner, 0, HIGHGUID_PLAYER);
    Player *owner = sObjectMgr->GetPlayer(owner_guid);
    uint32 owner_accId = sObjectMgr->GetPlayerAccountIdByGUID(owner_guid);
    // owner exist
    if (owner || owner_accId)
    {
        std::ostringstream subject;
        subject << auction->itemEntry << ":0:" << AUCTION_EXPIRED << ":0:0";

        if (owner)
            owner->GetSession()->SendAuctionOwnerNotification(auction);

        MailDraft(subject.str(), "")                        // TODO: fix body
            .AddItem(pItem)
            .SendMailTo(trans, MailReceiver(owner,auction->owner), auction, MAIL_CHECK_MASK_COPIED, 0);
    }
}

void AuctionHouseMgr::SendAuctionRemovedMail(AuctionEntry * auction, SQLTransaction& trans)
{
    uint64 owner_guid = MAKE_NEW_GUID(auction->owner, 0, HIGHGUID_PLAYER);
    Player *owner = sObjectMgr->GetPlayer(owner_guid);

    Item* pItem = sAuctionMgr->GetAItem(auction->item_guidlow);

    if (owner)
    {
        std::ostringstream subject;
        subject << auction->itemEntry << ":0:" << AUCTION_CANCELED << ":0:0";

        if (owner)
            owner->GetSession()->SendAuctionRemovedNotification(auction->Id,auction->itemEntry,pItem->GetItemRandomPropertyId());

        MailDraft(subject.str(), "").SendMailTo(trans, MailReceiver(owner, auction->owner), auction, MAIL_CHECK_MASK_COPIED, 0);
    }
}

//this function sends mail to old bidder
void AuctionHouseMgr::SendAuctionOutbiddedMail(AuctionEntry *auction, uint64 newPrice, Player* newBidder, SQLTransaction& trans)
{
    uint64 oldBidder_guid = MAKE_NEW_GUID(auction->bidder,0, HIGHGUID_PLAYER);
    Player *oldBidder = sObjectMgr->GetPlayer(oldBidder_guid);

    uint32 oldBidder_accId = 0;
    if (!oldBidder)
        oldBidder_accId = sObjectMgr->GetPlayerAccountIdByGUID(oldBidder_guid);

    // old bidder exist
    if (oldBidder || oldBidder_accId)
    {
        std::ostringstream msgAuctionOutbiddedSubject;
        msgAuctionOutbiddedSubject << auction->itemEntry << ":0:" << AUCTION_OUTBIDDED << ":0:0";

        if (oldBidder && newBidder)
            oldBidder->GetSession()->SendAuctionBidderNotification(auction->GetHouseId(), auction->Id, newBidder->GetGUID(), newPrice, auction->GetAuctionOutBid(), auction->itemEntry);

        MailDraft(msgAuctionOutbiddedSubject.str(), "")     // TODO: fix body
            .AddMoney(auction->bid)
            .SendMailTo(trans, MailReceiver(oldBidder, auction->bidder), auction, MAIL_CHECK_MASK_COPIED);
    }
}

//this function sends mail, when auction is cancelled to old bidder
void AuctionHouseMgr::SendAuctionCancelledToBidderMail(AuctionEntry* auction, SQLTransaction& trans, Item* item)
{
    uint64 bidder_guid = MAKE_NEW_GUID(auction->bidder, 0, HIGHGUID_PLAYER);
    Player *bidder = sObjectMgr->GetPlayer(bidder_guid);

    uint32 bidder_accId = 0;
    if (!bidder)
        bidder_accId = sObjectMgr->GetPlayerAccountIdByGUID(bidder_guid);

    if (bidder)
        bidder->GetSession()->SendAuctionRemovedNotification(auction->Id, auction->itemEntry, item->GetItemRandomPropertyId());

    // bidder exist
    if (bidder || bidder_accId)
    {
        std::ostringstream msgAuctionCancelledSubject;
        msgAuctionCancelledSubject << auction->itemEntry << ":0:" << AUCTION_CANCELLED_TO_BIDDER << ":0:0";

        MailDraft(msgAuctionCancelledSubject.str(), "")     // TODO: fix body
            .AddMoney(auction->bid)
            .SendMailTo(trans, MailReceiver(bidder, auction->bidder), auction, MAIL_CHECK_MASK_COPIED);
    }
}


void AuctionHouseMgr::LoadAuctionItems()
{
    // data needs to be at first place for Item::LoadFromDB
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_AUCTION_ITEMS);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (!result)
    {
        sLog->outString();
        sLog->outString(">> Loaded 0 auction items");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 item_guid        = fields[10].GetUInt32();
        uint32 itemEntry    = fields[11].GetUInt32();

        ItemPrototype const *proto = sObjectMgr->GetItemPrototype(itemEntry);
        if (!proto)
        {
            sLog->outError("AuctionHouseMgr::LoadAuctionItems: Unknown item (GUID: %u id: #%u) in auction, skipped.", item_guid,itemEntry);
            continue;
        }

        Item *item = NewItemOrBag(proto);
        if (!item->LoadFromDB(item_guid, 0, fields, itemEntry))
        {
            delete item;
            continue;
        }
        AddAItem(item);

        ++count;
    }
    while (result->NextRow());

    sLog->outString();
    sLog->outString(">> Loaded %u auction items", count);
}

void AuctionHouseMgr::LoadAuctions()
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_LOAD_AUCTIONS);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> Loaded 0 auctions. DB table `auctionhouse` is empty.");
        return;
    }


    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    uint32 count = 0;
    AuctionEntry *aItem;
    do
    {
        Field* fields = result->Fetch();

        aItem = new AuctionEntry;
        aItem->Id = fields[0].GetUInt32();
        aItem->auctioneer = fields[1].GetUInt32();
        aItem->item_guidlow = fields[2].GetUInt32();
        aItem->itemEntry = fields[3].GetUInt32();
        aItem->owner = fields[4].GetUInt32();
        aItem->buyout = fields[5].GetUInt32();
        aItem->expire_time = fields[6].GetUInt32();
        aItem->bidder = fields[7].GetUInt32();
        aItem->bid = fields[8].GetUInt32();
        aItem->startbid = fields[9].GetUInt32();
        aItem->deposit = fields[10].GetUInt32();

        aItem = new AuctionEntry();
        if (!aItem->LoadFromDB(fields))
        {
            aItem->DeleteFromDB(trans);
            delete aItem;
            continue;
        }

        GetAuctionsMap(aItem->factionTemplateId)->AddAuction(aItem);
        count++;
    } while (result->NextRow());

    CharacterDatabase.CommitTransaction(trans);

    sLog->outString();
    sLog->outString(">> Loaded %u auctions", count);
}

void AuctionHouseMgr::AddAItem(Item* it)
{
    ASSERT(it);
    ASSERT(mAitems.find(it->GetGUIDLow()) == mAitems.end());
    mAitems[it->GetGUIDLow()] = it;
}

bool AuctionHouseMgr::RemoveAItem(uint32 id)
{
    ItemMap::iterator i = mAitems.find(id);
    if (i == mAitems.end())
        return false;

    mAitems.erase(i);
    return true;
}

void AuctionHouseMgr::Update()
{
    mHordeAuctions.Update();
    mAllianceAuctions.Update();
    mNeutralAuctions.Update();
}

AuctionHouseEntry const* AuctionHouseMgr::GetAuctionHouseEntry(uint32 factionTemplateId)
{
    uint32 houseid = 7; // goblin auction house

    if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
    {
        //FIXME: found way for proper auctionhouse selection by another way
        // AuctionHouse.dbc have faction field with _player_ factions associated with auction house races.
        // but no easy way convert creature faction to player race faction for specific city
        switch(factionTemplateId)
        {
            case   12: houseid = 1; break; // human
            case   29: houseid = 6; break; // orc, and generic for horde
            case   55: houseid = 2; break; // dwarf, and generic for alliance
            case   68: houseid = 4; break; // undead
            case   80: houseid = 3; break; // n-elf
            case  104: houseid = 5; break; // trolls
            case  120: houseid = 7; break; // booty bay, neutral
            case  474: houseid = 7; break; // gadgetzan, neutral
            case  855: houseid = 7; break; // everlook, neutral
            case 1604: houseid = 6; break; // b-elfs,
            default:                       // for unknown case
            {
                FactionTemplateEntry const* u_entry = sFactionTemplateStore.LookupEntry(factionTemplateId);
                if (!u_entry)
                    houseid = 7; // goblin auction house
                else if (u_entry->ourMask & FACTION_MASK_ALLIANCE)
                    houseid = 1; // human auction house
                else if (u_entry->ourMask & FACTION_MASK_HORDE)
                    houseid = 6; // orc auction house
                else
                    houseid = 7; // goblin auction house
                break;
            }
        }
    }

    return sAuctionHouseStore.LookupEntry(houseid);
}

//this function inserts to WorldPacket auction's data
bool AuctionEntry::BuildAuctionInfo(WorldPacket & data) const
{
    Item *pItem = sAuctionMgr->GetAItem(item_guidlow);
    if (!pItem)
    {
        sLog->outError("auction to item, that doesn't exist !!!!");
        return false;
    }
    data << uint32(Id);
    data << uint32(pItem->GetEntry());

    for (uint8 i = 0; i < MAX_INSPECTED_ENCHANTMENT_SLOT; ++i)
    {
        data << uint32(pItem->GetEnchantmentId(EnchantmentSlot(i)));
        data << uint32(pItem->GetEnchantmentDuration(EnchantmentSlot(i)));
        data << uint32(pItem->GetEnchantmentCharges(EnchantmentSlot(i)));
    }

    data << int32(pItem->GetItemRandomPropertyId());        //random item property id
    data << uint32(pItem->GetItemSuffixFactor());           //SuffixFactor
    data << uint32(pItem->GetCount());                      //item->count
    data << uint32(pItem->GetSpellCharges());               //item->charge FFFFFFF
    data << uint32(0);                                      //Unknown
    data << uint64(owner);                                  //Auction->owner
    data << uint64(startbid);                               //Auction->startbid (not sure if useful)
    data << uint64(bid ? GetAuctionOutBid() : 0);           //minimal outbid
    data << uint64(buyout);                                 //auction->buyout
    data << uint32((expire_time-time(NULL))*IN_MILLISECONDS);//time left
    data << uint64(bidder) ;                                //auction->bidder current
    data << uint64(bid);                                    //current bid
    return true;
}

uint32 AuctionEntry::GetAuctionCut() const
{
    int32 cut = int32(((double)auctionHouseEntry->cutPercent / 100.0f) * (double)sWorld->getRate(RATE_AUCTION_CUT)) * bid;
    if (cut > 0)
        return cut;
    else
        return 0;
}

/// the sum of outbid is (1% from current bid)*5, if bid is very small, it is 1c
uint64 AuctionEntry::GetAuctionOutBid() const
{
    uint64 outbid = (uint32)((double)bid / 100.0f) * 5;
    if (!outbid)
        outbid = 1;
    return outbid;
}

void AuctionEntry::DeleteFromDB(SQLTransaction& trans) const
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_AUCTION);
    stmt->setUInt32(0, Id);
    trans->Append(stmt);
}

void AuctionEntry::SaveToDB(SQLTransaction& trans) const
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_ADD_AUCTION);
    stmt->setUInt32(0, Id);
    stmt->setUInt32(1, auctioneer);
    stmt->setUInt32(2, item_guidlow);
    stmt->setUInt64(3, owner);
    stmt->setInt32 (4, int32(buyout));
    stmt->setUInt64(5, uint64(expire_time));
    stmt->setUInt64(6, bidder);
    stmt->setInt32 (7, int32(bid));
    stmt->setInt32 (8, int32(startbid));
    stmt->setInt32 (9, int32(deposit));
    trans->Append(stmt);
}

bool AuctionEntry::LoadFromDB(Field* fields)
{
    Id = fields[0].GetUInt32();
    auctioneer = fields[1].GetUInt32();
    item_guidlow = fields[2].GetUInt32();
    itemEntry = fields[3].GetUInt32();
    owner = fields[4].GetUInt32();
    buyout = fields[5].GetUInt32();
    expire_time = fields[6].GetUInt32();
    bidder = fields[7].GetUInt32();
    bid = fields[8].GetUInt32();
    startbid = fields[9].GetUInt32();
    deposit = fields[10].GetUInt32();

    CreatureData const* auctioneerData = sObjectMgr->GetCreatureData(auctioneer);
    if (!auctioneerData && auctioneer != 258134)
    {
        sLog->outError("Auction %u has not a existing auctioneer (GUID : %u)", Id, auctioneer);
        return false;
    }

    CreatureInfo const* auctioneerInfo = sObjectMgr->GetCreatureTemplate((auctioneer == 258134)?8661:auctioneerData->id);
    if (!auctioneerInfo)
    {
        sLog->outError("Auction %u has not a existing auctioneer (GUID : %u Entry: %u)", Id, auctioneer, auctioneerData->id);
        return false;
    }

    factionTemplateId = auctioneerInfo->faction_A;
    auctionHouseEntry = AuctionHouseMgr::GetAuctionHouseEntry(factionTemplateId);
    if (!auctionHouseEntry)
    {
        sLog->outError("Auction %u has auctioneer (GUID : %u Entry: %u) with wrong faction %u", Id, auctioneer, auctioneerData->id, factionTemplateId);
        return false;
    }

    // check if sold item exists for guid
    // and itemEntry in fact (GetAItem will fail if problematic in result check in AuctionHouseMgr::LoadAuctionItems)
    if (!sAuctionMgr->GetAItem(item_guidlow))
    {
        sLog->outError("Auction %u has not a existing item : %u", Id, item_guidlow);
        return false;
    }
    return true;
}
