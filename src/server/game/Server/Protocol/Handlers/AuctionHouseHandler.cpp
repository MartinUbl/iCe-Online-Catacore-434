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
#include "Player.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include "AuctionHouseMgr.h"
#include "Log.h"
#include "Opcodes.h"
#include "UpdateMask.h"
#include "Util.h"

//please DO NOT use iterator++, because it is slower than ++iterator!!!
//post-incrementation is always slower than pre-incrementation !

//void called when player click on auctioneer npc
void WorldSession::HandleAuctionHelloOpcode(WorldPacket & recv_data)
{
    uint64 guid;                                            //NPC guid
    recv_data >> guid;

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(guid,UNIT_NPC_FLAG_AUCTIONEER);
    if (!unit)
    {
        sLog->outDebug("WORLD: HandleAuctionHelloOpcode - Unit (GUID: %u) not found or you can't interact with him.", uint32(GUID_LOPART(guid)));
        return;
    }

    GetPlayer()->RemoveFakeDeath();

    SendAuctionHello(guid, unit);
}

//this void causes that auction window is opened
void WorldSession::SendAuctionHello(uint64 guid, Creature* unit)
{
    if (GetPlayer()->getLevel() < sWorld->getIntConfig(CONFIG_AUCTION_LEVEL_REQ))
    {
        SendNotification(GetTrinityString(LANG_AUCTION_REQ), sWorld->getIntConfig(CONFIG_AUCTION_LEVEL_REQ));
        return;
    }

    AuctionHouseEntry const* ahEntry = AuctionHouseMgr::GetAuctionHouseEntry(unit->getFaction());
    if (!ahEntry)
        return;

    WorldPacket data(MSG_AUCTION_HELLO, 12);
    data << uint64(guid);
    data << uint32(ahEntry->houseId);
    data << uint8(1);                                       // 3.3.3: 1 - AH enabled, 0 - AH disabled
    SendPacket(&data);
}

//call this method when player bids, creates, or deletes auction
void WorldSession::SendAuctionCommandResult(AuctionEntry* auction, uint32 Action, uint32 ErrorCode, uint64 bidError)
{
    WorldPacket data(SMSG_AUCTION_COMMAND_RESULT);
    data << uint32(auction ? auction->Id : 0);
    data << uint32(Action);
    data << uint32(ErrorCode);

    switch (ErrorCode)
    {
        case ERR_AUCTION_OK:
            if (Action == AUCTION_PLACE_BID)
                data << uint64(auction->bid ? auction->GetAuctionOutBid() : 0);
            break;
        case ERR_AUCTION_INVENTORY:
            data << uint32(bidError);
            break;
        case ERR_AUCTION_HIGHER_BID:
            data << uint64(auction->bidder);
            data << uint64(auction->bid);
            data << uint64(auction->bid ? auction->GetAuctionOutBid() : 0);
            break;
    }


    SendPacket(&data);
}

//this function sends notification, if bidder is online
void WorldSession::SendAuctionBidderNotification(uint32 location, uint32 auctionId, uint64 bidder, uint32 bidSum, uint32 diff, uint32 item_template)
{
    WorldPacket data(SMSG_AUCTION_BIDDER_NOTIFICATION, (8*4));
    data << uint32(location);
    data << uint32(auctionId);
    data << uint64(bidder);
    data << uint32(bidSum);
    data << uint32(diff);
    data << uint32(item_template);
    data << uint32(0);
    SendPacket(&data);
}

//this void causes on client to display: "Your auction sold"
void WorldSession::SendAuctionOwnerNotification(AuctionEntry* auction)
{
    WorldPacket data(SMSG_AUCTION_OWNER_NOTIFICATION, ((2 * 4) + (2 * 8) + (3 * 4)));
    data << auction->Id;
    data << auction->bid;
    data << uint32(0);                                     // unk
    data << uint64(0);                                     // unk
    data << auction->itemEntry;
    data << uint32(0);                                     // Something with item names
    data << float(0);                                      // unk
    SendPacket(&data);
}

void WorldSession::SendAuctionRemovedNotification(uint32 auctionId, uint32 itemEntry, int32 randomPropertyId)
{
    WorldPacket data(SMSG_AUCTION_REMOVED_NOTIFICATION, (4+4+4));
    data << uint32(auctionId);
    data << uint32(itemEntry);
    data << uint32(randomPropertyId);
    SendPacket(&data);
}

//this void creates new auction and adds auction to some auctionhouse
void WorldSession::HandleAuctionSellItem(WorldPacket & recv_data)
{
    uint64 auctioneer;
    uint64 bid, buyout;
    uint32 etime;
    uint32 countOfItems;        // count of items used to create stack (almost always 1, more if creation of stack requires more player's stacks)
    std::vector<uint64> itemGuids;
    std::vector<uint32> itemStacks;

    recv_data >> auctioneer;                                // uint64
    recv_data >> countOfItems;                              // uint32
    for (uint32 i = 0; i < countOfItems; i++)
    {
        uint64 item;
        recv_data >> item;                                  // uint64
        itemGuids.push_back(item);
        uint32 stack;
        recv_data >> stack;                                 // uint32
        itemStacks.push_back(stack);
    }
    recv_data >> bid;                                       // uint64
    recv_data >> buyout;                                    // uint64
    recv_data >> etime;                                     // uint32

    Player *pl = GetPlayer();

    if (!bid || !etime)
        return;                                             //check for cheaters
    if (itemGuids.empty())
        return;

    Creature *pCreature = GetPlayer()->GetNPCIfCanInteractWith(auctioneer, UNIT_NPC_FLAG_AUCTIONEER);
    if (!pCreature)
    {
        sLog->outDebug("WORLD: HandleAuctionSellItem - Unit (GUID: %u) not found or you can't interact with him.", uint32(GUID_LOPART(auctioneer)));
        return;
    }

    AuctionHouseEntry const* auctionHouseEntry = AuctionHouseMgr::GetAuctionHouseEntry(pCreature->getFaction());
    if (!auctionHouseEntry)
    {
        sLog->outDebug("WORLD: HandleAuctionSellItem - Unit (GUID: %u) has wrong faction.", uint32(GUID_LOPART(auctioneer)));
        return;
    }

    sLog->outDebug("WORLD: HandleAuctionSellItem - ETIME: %u", etime);

    // client send time in minutes, convert to common used sec time
    etime *= MINUTE;

    sLog->outDebug("WORLD: HandleAuctionSellItem - ETIME: %u", etime);

    // client understand only 3 auction time
    switch (etime)
    {
        case 1*MIN_AUCTION_TIME:
        case 2*MIN_AUCTION_TIME:
        case 4*MIN_AUCTION_TIME:
            break;
        default:
            return;
    }

    GetPlayer()->RemoveFakeDeath();

    std::vector<Item*> items;
    items.reserve(countOfItems);
    for (uint32 i = 0; i < countOfItems; i++)
    {
        Item *it = pl->GetItemByGuid(itemGuids[i]);
        // prevent sending bag with items (cheat: can be placed in bag after adding equiped empty bag to auction)
        if (!it)
        {
            SendAuctionCommandResult(0, AUCTION_SELL_ITEM, ERR_AUCTION_ITEM_NOT_FOUND);
            return;
        }

        //do not allow to sell already auctioned items
        if (sAuctionMgr->GetAItem(GUID_LOPART(itemGuids[i])))
        {
            sLog->outError("AuctionError, player %s is sending item id: %u, but item is already in another auction", pl->GetName(), it->GetGUIDLow());
            SendAuctionCommandResult(0, AUCTION_SELL_ITEM, ERR_AUCTION_DATABASE_ERROR);
            return;
        }

        if (!it->CanBeTraded())
        {
            SendAuctionCommandResult(0, AUCTION_SELL_ITEM, ERR_AUCTION_DATABASE_ERROR);
            return;
        }

        if (it->GetProto()->Flags & ITEM_PROTO_FLAG_CONJURED || it->GetUInt32Value(ITEM_FIELD_DURATION))
        {
            SendAuctionCommandResult(0, AUCTION_SELL_ITEM, ERR_AUCTION_DATABASE_ERROR);
            return;
        }

        if (it->IsBag() && !((Bag*)it)->IsEmpty())
        {
            SendAuctionCommandResult(0, AUCTION_SELL_ITEM, ERR_AUCTION_DATABASE_ERROR);
            return;
        }
        if (it->GetCount() < itemStacks[i])
        {
            SendAuctionCommandResult(0, AUCTION_SELL_ITEM, ERR_AUCTION_ITEM_NOT_FOUND);
            return;
        }
        items.push_back(it);
    }

    //  check prototypes of merged items
    ItemPrototype const *proto = items[0]->GetProto();
    for (uint32 i = 1; i < items.size(); i++)
    {
        if (items[i]->GetProto() != proto)
        {
            SendAuctionCommandResult(0, AUCTION_SELL_ITEM, ERR_AUCTION_DATABASE_ERROR);
            return;
        }
    }

    uint32 totalCount = 0;
    for (uint32 i = 0; i < itemStacks.size(); i++)
        totalCount += itemStacks[i];

    if (totalCount > proto->GetMaxStackSize())
    {
        SendAuctionCommandResult(0, AUCTION_SELL_ITEM, ERR_AUCTION_DATABASE_ERROR);
        return;
    }

    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap(pCreature->getFaction());

    //we have to take deposit :
    uint64 deposit = sAuctionMgr->GetAuctionDeposit(auctionHouseEntry, etime, proto, totalCount);
    if (!pl->HasEnoughMoney(deposit))
    {
        SendAuctionCommandResult(0, AUCTION_SELL_ITEM, ERR_AUCTION_NOT_ENOUGHT_MONEY);
        return;
    }

    if (GetSecurity() > SEC_PLAYER && sWorld->getBoolConfig(CONFIG_GM_LOG_TRADE))
    {
        sLog->outCommand(GetAccountId(),"GM %s (Account: %u) create auction: %s (Entry: %u Count: %u)",
            GetPlayerName(),GetAccountId(),proto->Name1,proto->ItemId,totalCount);
    }

    sLog->outChar("IP:(%s) account:(%u) character:(%s) action:(%s) %s:(name:(%s) entry:(%u) count:(%u))",
                 pl->GetSession() ? pl->GetSession()->GetRemoteAddress().c_str() : "none",
                 GetAccountId(),
                 GetPlayerName(),
                 "create auction",
                   "item",
                   proto->Name1,
                   proto->ItemId,
                   totalCount);

    pl->ModifyMoney(-int64(deposit));

    Item *it = NULL;
    for (uint32 i = 0; i < items.size(); i++)
    {
        Item *item = items[i];
        if (itemStacks[i] == item->GetCount())
        {
            if (it == NULL)
                it = item;
            else
                pl->DestroyItem(item->GetBagSlot(), item->GetSlot(), true, false);
        }
        else
            pl->DestroyItemCount(item, itemStacks[i], true);
    }

    bool itemIsCloned = false;
    if (it == NULL)
    {
        it = items[0]->CloneItem(1, pl);
        itemIsCloned = true;
    }

    it->SetCount(totalCount);
    it->SetState(ITEM_CHANGED, pl);
    it->RemoveFromUpdateQueueOf(pl);

    uint32 auction_time = uint32(etime * sWorld->getRate(RATE_AUCTION_TIME));

    AuctionEntry *AH = new AuctionEntry;
    AH->Id = sObjectMgr->GenerateAuctionID();
    if (sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
        AH->auctioneer = 258134;
    else
        AH->auctioneer = GUID_LOPART(auctioneer);
    AH->item_guidlow = it->GetGUIDLow();
    AH->itemEntry = it->GetEntry();
    AH->owner = pl->GetGUIDLow();
    AH->startbid = bid;
    AH->bidder = 0;
    AH->bid = 0;
    AH->buyout = buyout;
    AH->expire_time = time(NULL) + auction_time;
    AH->deposit = deposit;
    AH->auctionHouseEntry = auctionHouseEntry;

    sAuctionMgr->AddAItem(it);
    auctionHouse->AddAuction(AH);

    if (!itemIsCloned)
        pl->MoveItemFromInventory(it->GetBagSlot(), it->GetSlot(), true);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    it->DeleteFromInventoryDB(trans);
    it->SaveToDB(trans);                                         // recursive and not have transaction guard into self, not in inventiory and can be save standalone
    AH->SaveToDB(trans);
    pl->SaveInventoryAndGoldToDB(trans);
    CharacterDatabase.CommitTransaction(trans);

    SendAuctionCommandResult(AH, AUCTION_SELL_ITEM, ERR_AUCTION_OK);

    GetPlayer()->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_CREATE_AUCTION, 1);
}

//this function is called when client bids or buys out auction
void WorldSession::HandleAuctionPlaceBid(WorldPacket & recv_data)
{
    uint64 auctioneer;
    uint32 auctionId;
    uint64 price;
    recv_data >> auctioneer;
    recv_data >> auctionId;
    recv_data >> price;

    if (!auctionId || !price)
        return;                                             //check for cheaters

    Creature *pCreature = GetPlayer()->GetNPCIfCanInteractWith(auctioneer, UNIT_NPC_FLAG_AUCTIONEER);
    if (!pCreature)
    {
        sLog->outDebug("WORLD: HandleAuctionPlaceBid - Unit (GUID: %u) not found or you can't interact with him.", uint32(GUID_LOPART(auctioneer)));
        return;
    }

    GetPlayer()->RemoveFakeDeath();

    AuctionHouseObject *auctionHouse = sAuctionMgr->GetAuctionsMap(pCreature->getFaction());

    AuctionEntry *auction = auctionHouse->GetAuction(auctionId);
    Player *pl = GetPlayer();

    if (!auction || auction->owner == pl->GetGUIDLow())
    {
        //you cannot bid your own auction:
        SendAuctionCommandResult(0, AUCTION_PLACE_BID, ERR_AUCTION_BID_OWN);
        return;
    }

    // impossible have online own another character (use this for speedup check in case online owner)
    Player* auction_owner = sObjectMgr->GetPlayer(MAKE_NEW_GUID(auction->owner, 0, HIGHGUID_PLAYER));
    if (!auction_owner && sObjectMgr->GetPlayerAccountIdByGUID(MAKE_NEW_GUID(auction->owner, 0, HIGHGUID_PLAYER)) == pl->GetSession()->GetAccountId())
    {
        //you cannot bid your another character auction:
        SendAuctionCommandResult(0, AUCTION_PLACE_BID, ERR_AUCTION_BID_OWN);
        return;
    }

    // cheating
    if (price <= auction->bid || price < auction->startbid)
        return;

    // price too low for next bid if not buyout
    if ((price < auction->buyout || auction->buyout == 0) &&
        price < auction->bid + auction->GetAuctionOutBid())
    {
        //auction has already higher bid, client tests it!
        return;
    }

    if (!pl->HasEnoughMoney(price))
    {
        //you don't have enought money!, client tests!
        //SendAuctionCommandResult(auction->auctionId, AUCTION_PLACE_BID, ???);
        return;
    }

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    if (price < auction->buyout || auction->buyout == 0)
    {
        if (auction->bidder > 0)
        {
            if (auction->bidder == pl->GetGUIDLow())
                pl->ModifyMoney(-int64(price - auction->bid));
            else
            {
                // mail to last bidder and return money
                sAuctionMgr->SendAuctionOutbiddedMail(auction, price, GetPlayer(), trans);
                pl->ModifyMoney(-int64(price));
            }
        }
        else
            pl->ModifyMoney(-int64(price));

        auctionHouse->UpdateBidSorting(auction, std::max(auction->bid, auction->startbid), price);
        auction->bidder = pl->GetGUIDLow();
        auction->bid = price;
        GetPlayer()->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_BID, price);

        trans->PAppend("UPDATE auctionhouse SET buyguid = '%u',lastbid = '%u' WHERE id = '%u'", auction->bidder, auction->bid, auction->Id);

        SendAuctionCommandResult(auction, AUCTION_PLACE_BID, ERR_AUCTION_OK, 0);
    }
    else
    {
        //buyout:
        if (pl->GetGUIDLow() == auction->bidder)
            pl->ModifyMoney(-(int64(auction->buyout) - int64(auction->bid)));
        else
        {
            pl->ModifyMoney(-int64(auction->buyout));
            if (auction->bidder)                          //buyout for bidded auction ..
                sAuctionMgr->SendAuctionOutbiddedMail(auction, auction->buyout, GetPlayer(), trans);
        }
        auctionHouse->UpdateBidSorting(auction, std::max(auction->bid, auction->startbid), auction->buyout);
        auction->bidder = pl->GetGUIDLow();
        auction->bid = auction->buyout;
        GetPlayer()->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_BID, auction->buyout);

        //- Mails must be under transaction control too to prevent data loss
        sAuctionMgr->SendAuctionSalePendingMail(auction, trans);
        sAuctionMgr->SendAuctionSuccessfulMail(auction, trans);
        sAuctionMgr->SendAuctionWonMail(auction, trans);

        SendAuctionCommandResult(auction, AUCTION_PLACE_BID, ERR_AUCTION_OK);

        auction->DeleteFromDB(trans);

        uint32 itemEntry = auction->itemEntry;
        sAuctionMgr->RemoveAItem(auction->item_guidlow);
        auctionHouse->RemoveAuction(auction, itemEntry);
    }
    pl->SaveInventoryAndGoldToDB(trans);
    CharacterDatabase.CommitTransaction(trans);
}

//this void is called when auction_owner cancels his auction
void WorldSession::HandleAuctionRemoveItem(WorldPacket & recv_data)
{
    uint64 auctioneer;
    uint32 auctionId;
    recv_data >> auctioneer;
    recv_data >> auctionId;
    //sLog->outDebug("Cancel AUCTION AuctionID: %u", auctionId);

    Creature *pCreature = GetPlayer()->GetNPCIfCanInteractWith(auctioneer,UNIT_NPC_FLAG_AUCTIONEER);
    if (!pCreature)
    {
        sLog->outDebug("WORLD: HandleAuctionRemoveItem - Unit (GUID: %u) not found or you can't interact with him.", uint32(GUID_LOPART(auctioneer)));
        return;
    }

    GetPlayer()->RemoveFakeDeath();

    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap(pCreature->getFaction());

    AuctionEntry *auction = auctionHouse->GetAuction(auctionId);
    Player *pl = GetPlayer();

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    if (auction && auction->owner == pl->GetGUIDLow())
    {
        Item *pItem = sAuctionMgr->GetAItem(auction->item_guidlow);
        if (pItem)
        {
            if (auction->bidder > 0)                        // If we have a bidder, we have to send him the money he paid
            {
                int64 auctionCut = auction->GetAuctionCut();
                if (!pl->HasEnoughMoney(auctionCut))          //player doesn't have enough money, maybe message needed
                    return;
                //some auctionBidderNotification would be needed, but don't know that parts..
                sAuctionMgr->SendAuctionCancelledToBidderMail(auction, trans, pItem);
                pl->ModifyMoney(-int64(auctionCut));
            }
            // Return the item by mail
            std::ostringstream msgAuctionCanceledOwner;
            msgAuctionCanceledOwner << auction->itemEntry << ":0:" << AUCTION_CANCELED << ":0:0";

            // item will deleted or added to received mail list
            MailDraft(msgAuctionCanceledOwner.str(), "")    // TODO: fix body
                .AddItem(pItem)
                .SendMailTo(trans, pl, auction, MAIL_CHECK_MASK_COPIED);
        }
        else
        {
            sLog->outError("Auction id: %u has non-existed item (item guid : %u)!!!", auction->Id, auction->item_guidlow);
            SendAuctionCommandResult(0, AUCTION_CANCEL, ERR_AUCTION_DATABASE_ERROR);
            return;
        }
    }
    else
    {
        SendAuctionCommandResult(0, AUCTION_CANCEL, ERR_AUCTION_DATABASE_ERROR);
        //this code isn't possible ... maybe there should be assert
        sLog->outError("CHEATER : %u, he tried to cancel auction (id: %u) of another player, or auction is NULL", pl->GetGUIDLow(), auctionId);
        return;
    }

    //inform player, that auction is removed
    SendAuctionCommandResult(auction, AUCTION_CANCEL, ERR_AUCTION_OK);

    // Now remove the auction

    pl->SaveInventoryAndGoldToDB(trans);
    auction->DeleteFromDB(trans);
    CharacterDatabase.CommitTransaction(trans);

    uint32 itemEntry = auction->itemEntry;
    sAuctionMgr->RemoveAItem(auction->item_guidlow);
    auctionHouse->RemoveAuction(auction, itemEntry);
}

//called when player lists his bids
void WorldSession::HandleAuctionListBidderItems(WorldPacket & recv_data)
{
    uint64 guid;                                            //NPC guid
    uint32 listfrom;                                        //page of auctions
    uint32 outbiddedCount;                                  //count of outbidded auctions

    recv_data >> guid;
    recv_data >> listfrom;                                  // not used in fact (this list not have page control in client)
    recv_data >> outbiddedCount;
    if (recv_data.size() != (16 + outbiddedCount * 4))
    {
        sLog->outError("Client sent bad opcode!!! with count: %u and size : %lu (must be: %u)", outbiddedCount, (unsigned long)recv_data.size(),(16 + outbiddedCount * 4));
        outbiddedCount = 0;
    }

    Creature *pCreature = GetPlayer()->GetNPCIfCanInteractWith(guid,UNIT_NPC_FLAG_AUCTIONEER);
    if (!pCreature)
    {
        sLog->outDebug("WORLD: HandleAuctionListBidderItems - Unit (GUID: %u) not found or you can't interact with him.", uint32(GUID_LOPART(guid)));
        return;
    }

    GetPlayer()->RemoveFakeDeath();

    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap(pCreature->getFaction());

    WorldPacket data(SMSG_AUCTION_BIDDER_LIST_RESULT, (4+4+4));
    Player *pl = GetPlayer();
    data << (uint32) 0;                                     //add 0 as count
    uint32 count = 0;
    uint32 totalcount = 0;
    while (outbiddedCount > 0)                             //add all data, which client requires
    {
        --outbiddedCount;
        uint32 outbiddedAuctionId;
        recv_data >> outbiddedAuctionId;
        AuctionEntry * auction = auctionHouse->GetAuction(outbiddedAuctionId);
        if (auction && auction->BuildAuctionInfo(data))
        {
            ++totalcount;
            ++count;
        }
    }

    auctionHouse->BuildListBidderItems(data,pl,count,totalcount);
    data.put<uint32>(0, count);                           // add count to placeholder
    data << totalcount;
    data << (uint32)300;                                    //unk 2.3.0
    SendPacket(&data);
}

//this void sends player info about his auctions
void WorldSession::HandleAuctionListOwnerItems(WorldPacket & recv_data)
{
    uint32 listfrom;
    uint64 guid;

    recv_data >> guid;
    recv_data >> listfrom;                                  // not used in fact (this list not have page control in client)

    Creature *pCreature = GetPlayer()->GetNPCIfCanInteractWith(guid,UNIT_NPC_FLAG_AUCTIONEER);
    if (!pCreature)
    {
        sLog->outDebug("WORLD: HandleAuctionListOwnerItems - Unit (GUID: %u) not found or you can't interact with him.", uint32(GUID_LOPART(guid)));
        return;
    }

    GetPlayer()->RemoveFakeDeath();

    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap(pCreature->getFaction());

    WorldPacket data(SMSG_AUCTION_OWNER_LIST_RESULT, (4+4+4));
    data << (uint32) 0;                                     // amount place holder

    uint32 count = 0;
    uint32 totalcount = 0;

    auctionHouse->BuildListOwnerItems(data,_player,count,totalcount);
    data.put<uint32>(0, count);
    data << (uint32) totalcount;
    data << (uint32) 0;
    SendPacket(&data);
}

//this void is called when player clicks on search button
void WorldSession::HandleAuctionListItems(WorldPacket & recv_data)
{
    std::string searchedname;
    uint8 levelmin, levelmax, usable;
    uint32 listfrom, auctionSlotID, auctionMainCategory, auctionSubCategory, quality;
    uint64 guid;
    uint8 sortingCriterion, sortingDirection;

    recv_data >> guid;
    recv_data >> listfrom;                                  // start, used for page control listing by 50 elements
    recv_data >> searchedname;

    recv_data >> levelmin >> levelmax;
    recv_data >> auctionSlotID >> auctionMainCategory >> auctionSubCategory;
    recv_data >> quality >> usable;

    recv_data.read_skip<uint8>();                           // unk
    recv_data.read_skip<uint8>();                           // unk
    recv_data.read_skip<uint8>();                           // unk

    recv_data >> sortingCriterion;
    recv_data >> sortingDirection;

    // unknown data, size varies
    recv_data.rpos(recv_data.wpos());

    Creature *pCreature = GetPlayer()->GetNPCIfCanInteractWith(guid,UNIT_NPC_FLAG_AUCTIONEER);
    if (!pCreature)
    {
        sLog->outDebug("WORLD: HandleAuctionListItems - Unit (GUID: %u) not found or you can't interact with him.", uint32(GUID_LOPART(guid)));
        return;
    }

    GetPlayer()->RemoveFakeDeath();

    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap(pCreature->getFaction());

    //sLog->outDebug("Auctionhouse search (GUID: %u TypeId: %u)", , list from: %u, searchedname: %s, levelmin: %u, levelmax: %u, auctionSlotID: %u, auctionMainCategory: %u, auctionSubCategory: %u, quality: %u, usable: %u",
    //  GUID_LOPART(guid),GuidHigh2TypeId(GUID_HIPART(guid)), listfrom, searchedname.c_str(), levelmin, levelmax, auctionSlotID, auctionMainCategory, auctionSubCategory, quality, usable);

    WorldPacket data(SMSG_AUCTION_LIST_RESULT, (4+4+4));
    uint32 count = 0;
    uint32 totalcount = 0;
    data << uint32(0);

    // converting string that we try to find to lower case
    std::wstring wsearchedname;
    if (!Utf8toWStr(searchedname,wsearchedname))
        return;

    wstrToLower(wsearchedname);

    auctionHouse->BuildListAuctionItems(data,_player,
        wsearchedname, listfrom, levelmin, levelmax, usable,
        auctionSlotID, auctionMainCategory, auctionSubCategory, quality,
        count, totalcount, (AuctionSortingCriterion)sortingCriterion, (AuctionSortingDirection)sortingDirection);

    data.put<uint32>(0, count);
    data << (uint32) totalcount;
    data << (uint32) 300;                                   // unk 2.3.0 const, same in 4.0.6a
    SendPacket(&data);
}

void WorldSession::HandleAuctionListPendingSales(WorldPacket & recv_data)
{
    sLog->outDebug("CMSG_AUCTION_LIST_PENDING_SALES");

    recv_data.read_skip<uint64>();

    uint32 count = 0;

    WorldPacket data(SMSG_AUCTION_LIST_PENDING_SALES, 4);
    data << uint32(count);                                  // count
    /*for (uint32 i = 0; i < count; ++i)
    {
        data << "";                                         // string
        data << "";                                         // string
        data << uint64(0);
        data << uint32(0);
        data << float(0);
    }*/
    SendPacket(&data);
}
