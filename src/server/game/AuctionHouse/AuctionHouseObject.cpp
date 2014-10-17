#include "gamePCH.h"
#include "AuctionHouseObject.h"

#include "AuctionHouseMgr.h"
#include "ScriptMgr.h"



AuctionSearch::AuctionSearch(Player* const player, std::wstring const& wsearchedname, uint32 listfrom, uint8 levelmin, uint8 levelmax, uint8 usable,
    uint32 inventoryType, uint32 itemClass, uint32 itemSubClass, uint32 quality, AuctionSortingCriterion sortingCriterion, AuctionSortingDirection sortingDirection,
    uint32& count, uint32& totalcount)
    : m_player(player), m_wsearchedname(wsearchedname), m_listfrom(listfrom), m_levelmin(levelmin), m_levelmax(levelmax), m_usable(usable),
    m_inventoryType(inventoryType), m_itemClass(itemClass), m_itemSubClass(itemSubClass), m_quality(quality),
    m_sortingCriterion(sortingCriterion), m_sortingDirection(sortingDirection),
    m_count(count), m_totalcount(totalcount)
{
}




AuctionHouseObject::AuctionHouseObject()
{
}

AuctionHouseObject::~AuctionHouseObject()
{
    for (AuctionEntryMap::iterator itr = AuctionsMap.begin(); itr != AuctionsMap.end(); ++itr)
        delete itr->second;
}

uint32 AuctionHouseObject::Getcount() const
{
    return AuctionsMap.size();
}

AuctionEntry* AuctionHouseObject::GetAuction(uint32 id) const
{
    AuctionEntryMap::const_iterator itr = AuctionsMap.find(id);
    return itr != AuctionsMap.end() ? itr->second : NULL;
}

void AuctionHouseObject::AddAuction(AuctionEntry *auction)
{
    ASSERT(auction);

    AuctionsMap[auction->Id] = auction;
    AuctionsMapByRarity.insert(std::make_pair(auction->GetItemQuality(), auction));
    AuctionsMapByLevel.insert(std::make_pair(auction->GetRequiredLevel(), auction));
    AuctionsMapByTimeLeft.insert(std::make_pair(auction->GetExpireTime(), auction));
    AuctionsMapBySeller.insert(std::make_pair(auction->GetOwnerName(), auction));
    AuctionsMapByCurrentBid.insert(std::make_pair(auction->startbid, auction));

    sScriptMgr->OnAuctionAdd(this, auction);
}

bool AuctionHouseObject::RemoveAuction(AuctionEntry *auction, uint32 /*itemEntry*/)
{
    bool wasInMap = AuctionsMap.erase(auction->Id) ? true : false;
    if (wasInMap)
    {
        RemoveFromAuctionMap(AuctionsMapByRarity, auction->GetItemQuality(), auction);
        RemoveFromAuctionMap(AuctionsMapByLevel, auction->GetRequiredLevel(), auction);
        RemoveFromAuctionMap(AuctionsMapByTimeLeft, auction->GetExpireTime(), auction);
        RemoveFromAuctionMap(AuctionsMapBySeller, auction->GetOwnerName(), auction);
        RemoveFromAuctionMap(AuctionsMapByCurrentBid, auction->GetCurrentBid(), auction);
    }

    sScriptMgr->OnAuctionRemove(this, auction);

    // we need to delete the entry, it is not referenced any more
    delete auction;
    return wasInMap;
}

template <class TKey, class TContainer>
void AuctionHouseObject::RemoveFromAuctionMap(TContainer &container, const TKey &key, const AuctionEntry *auction)
{
    typedef typename std::multimap<TKey, const AuctionEntry *>::iterator iterator;
    std::pair<iterator, iterator> mapIterRange = container.equal_range(key);

    for (iterator itr = mapIterRange.first; itr != mapIterRange.second; itr++)
    {
        if (auction == itr->second)
        {
            container.erase(itr);
            return;
        }
    }

    // auction not found by key - perform full search
    for (iterator itr = container.begin(); itr != container.end(); itr++)
    {
        if (auction == itr->second)
        {
            container.erase(itr);
            return;
        }
    }
}

void AuctionHouseObject::Update()
{
    time_t curTime = sWorld->GetGameTime();
    if (AuctionsMap.empty())
        return;

    QueryResult result = CharacterDatabase.PQuery("SELECT id FROM auctionhouse WHERE time <= %u ORDER BY TIME ASC", (uint32)curTime + 60);

    if (!result)
        return;

    do
    {
        FinishAuctionOnTime(result->Fetch()->GetUInt32());
    } while (result->NextRow());
}

void AuctionHouseObject::FinishAuctionOnTime(uint32 auctionId)
{
    AuctionEntry* auction = GetAuction(auctionId);
    if (!auction)
        return;

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    ///- Either cancel the auction if there was no bidder
    if (auction->bidder == 0)
    {
        sAuctionMgr->SendAuctionExpiredMail(auction, trans);
        sScriptMgr->OnAuctionExpire(this, auction);
    }
    ///- Or perform the transaction
    else
    {
        //we should send an "item sold" message if the seller is online
        //we send the item to the winner
        //we send the money to the seller
        sAuctionMgr->SendAuctionSuccessfulMail(auction, trans);
        sAuctionMgr->SendAuctionWonMail(auction, trans);
        sScriptMgr->OnAuctionSuccessful(this, auction);
    }

    uint32 itemEntry = auction->itemEntry;

    ///- In any case clear the auction
    auction->DeleteFromDB(trans);
    CharacterDatabase.CommitTransaction(trans);

    RemoveAuction(auction, itemEntry);
    sAuctionMgr->RemoveAItem(auction->item_guidlow);
}

void AuctionHouseObject::BuildListBidderItems(WorldPacket& data, Player* player, uint32& count, uint32& totalcount)
{
    for (AuctionEntryMap::const_iterator itr = AuctionsMap.begin(); itr != AuctionsMap.end(); ++itr)
    {
        AuctionEntry *Aentry = itr->second;
        if (Aentry && Aentry->bidder == player->GetGUIDLow())
        {
            if (itr->second->BuildAuctionInfo(data))
                ++count;

            ++totalcount;
        }
    }
}

void AuctionHouseObject::BuildListOwnerItems(WorldPacket& data, Player* player, uint32& count, uint32& totalcount)
{
    for (AuctionEntryMap::const_iterator itr = AuctionsMap.begin(); itr != AuctionsMap.end(); ++itr)
    {
        AuctionEntry *Aentry = itr->second;
        if (Aentry && Aentry->owner == player->GetGUIDLow())
        {
            if (Aentry->BuildAuctionInfo(data))
                ++count;

            ++totalcount;
        }
    }
}

void AuctionHouseObject::BuildListAuctionItems(WorldPacket& data, Player* player,
    std::wstring const& wsearchedname, uint32 listfrom, uint8 levelmin, uint8 levelmax, uint8 usable,
    uint32 inventoryType, uint32 itemClass, uint32 itemSubClass, uint32 quality,
    uint32& count, uint32& totalcount, AuctionSortingCriterion sortingCriterion, AuctionSortingDirection sortingDirection)
{
    if (sortingCriterion == SC_RARITY)
    {
        // from unknown reason client sends opposite sorting direction for Rarity column than it internally sorts items inside a page
        sortingDirection = (sortingDirection == SORT_ASC) ? SORT_DESC : SORT_ASC;
    }

    AuctionSearch search(player, wsearchedname, listfrom, levelmin, levelmax, usable, inventoryType, itemClass, itemSubClass, quality,
        sortingCriterion, sortingDirection, count, totalcount);

    AuctionList auctionsList = GetAuctionsBySearchCriteria(search);

    for (AuctionList::const_iterator itr = auctionsList.begin(); itr != auctionsList.end(); ++itr)
    {
        const AuctionEntry *Aentry = *itr;
        Aentry->BuildAuctionInfo(data);
    }
}

bool AuctionHouseObject::ItemMatchesSearchCriteria(AuctionEntry const *Aentry, AuctionSearch const& search) const
{
    Item *item = sAuctionMgr->GetAItem(Aentry->item_guidlow);
    if (!item)
        return false;

    ItemPrototype const *proto = item->GetProto();

    if (search.m_itemClass != 0xffffffff && proto->Class != search.m_itemClass)
        return false;

    if (search.m_itemSubClass != 0xffffffff && proto->SubClass != search.m_itemSubClass)
        return false;

    if (search.m_inventoryType != 0xffffffff && proto->InventoryType != search.m_inventoryType)
    {
        // let's join chests with robes, as it should be
        if (search.m_inventoryType != INVTYPE_CHEST || proto->InventoryType != INVTYPE_ROBE)
            return false;
    }

    if (search.m_quality != 0xffffffff && proto->Quality != search.m_quality)
        return false;

    if (search.m_levelmin != 0x00 && (proto->RequiredLevel < search.m_levelmin || (search.m_levelmax != 0x00 && proto->RequiredLevel > search.m_levelmax)))
        return false;

    if (search.m_usable != 0x00 && search.m_player->CanUseItem(item) != EQUIP_ERR_OK)
        return false;

    // Allow search by suffix (ie: of the Monkey) or partial name (ie: Monkey)
    // No need to do any of this if no search term was entered
    if (!search.m_wsearchedname.empty())
    {
        std::string name = proto->Name1;
        if (name.empty())
            return false;

        // local name
        int loc_idx = search.m_player->GetSession()->GetSessionDbLocaleIndex();
        if (loc_idx >= 0)
            if (ItemLocale const *il = sObjectMgr->GetItemLocale(proto->ItemId))
                sObjectMgr->GetLocaleString(il->Name, loc_idx, name);

        // DO NOT use GetItemEnchantMod(proto->RandomProperty) as it may return a result
        //  that matches the search but it may not equal item->GetItemRandomPropertyId()
        //  used in BuildAuctionInfo() which then causes wrong items to be listed
        int32 propRefID = item->GetItemRandomPropertyId();

        if (propRefID)
        {
            // Append the suffix to the name (ie: of the Monkey) if one exists
            // These are found in ItemRandomProperties.dbc, not ItemRandomSuffix.dbc
            //  even though the DBC names seem misleading
            const ItemRandomPropertiesEntry *itemRandProp = sItemRandomPropertiesStore.LookupEntry(propRefID);

            if (itemRandProp)
            {
                DBCString temp = itemRandProp->nameSuffix;
                //char* temp = itemRandProp->nameSuffix;

                // dbc local name
                if (temp)
                {
                    int locdbc_idx = search.m_player->GetSession()->GetSessionDbcLocale();
                    if (locdbc_idx >= 0)
                    {
                        // Append the suffix (ie: of the Monkey) to the name using localization
                        name += " ";
                        name += temp;
                    }
                    else
                    {
                        // Invalid localization? Append the suffix using default enUS
                        name += " ";
                        name += temp;
                    }
                }
            }
        }

        // Perform the search (with or without suffix)
        if (!Utf8FitTo(name, search.m_wsearchedname))
            return false;
    }

    return true;
}

AuctionHouseObject::AuctionList AuctionHouseObject::GetAuctionsBySearchCriteria(const AuctionSearch &search) const
{
    switch (search.m_sortingCriterion)
    {
    case SC_RARITY:
        return GetAuctionsBySearchCriteria(AuctionsMapByRarity, search);
    case SC_LEVEL:
        return GetAuctionsBySearchCriteria(AuctionsMapByLevel, search);
    case SC_TIME_LEFT:
        return GetAuctionsBySearchCriteria(AuctionsMapByTimeLeft, search);
    case SC_SELLER:
        return GetAuctionsBySearchCriteria(AuctionsMapBySeller, search);
    case SC_CURRENT_BID:
        return GetAuctionsBySearchCriteria(AuctionsMapByCurrentBid, search);
    default:
        return GetAuctionsBySearchCriteria(AuctionsMap, search);
    }
}

template <class TContainer>
AuctionHouseObject::AuctionList AuctionHouseObject::GetAuctionsBySearchCriteria(TContainer &container, const AuctionSearch &search) const
{
    if (search.m_sortingDirection == SORT_ASC)
        return GetAuctionsBySearchCriteria(container.begin(), container.end(), search);
    else
        return GetAuctionsBySearchCriteria(container.rbegin(), container.rend(), search);
}

template <class TIterator>
AuctionHouseObject::AuctionList AuctionHouseObject::GetAuctionsBySearchCriteria(const TIterator &begin, const TIterator &end, const AuctionSearch &search) const
{
    AuctionList result;

    for (TIterator itr = begin; itr != end; itr++)
    {
        const AuctionEntry *entry = itr->second;
        if (!ItemMatchesSearchCriteria(entry, search))
            continue;

        if (search.m_count < 50 && search.m_totalcount >= search.m_listfrom)
        {
            ++search.m_count;
            result.push_back(entry);
        }
        ++search.m_totalcount;
    }

    return result;
}

void AuctionHouseObject::UpdateBidSorting(const AuctionEntry *auction, uint64 oldBid, uint64 newBid)
{
    RemoveFromAuctionMap(AuctionsMapByCurrentBid, oldBid, auction);
    AuctionsMapByCurrentBid.insert(std::make_pair(newBid, auction));
}
