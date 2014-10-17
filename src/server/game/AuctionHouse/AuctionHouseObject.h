#ifndef _AUCTION_HOUSE_OBJECT_H
#define _AUCTION_HOUSE_OBJECT_H


// sorting values used by client in WorldSession::HandleAuctionListItems
enum AuctionSortingCriterion
{
    SC_RARITY = 1,
    SC_LEVEL = 0,
    SC_TIME_LEFT = 3,
    SC_SELLER = 7,
    SC_CURRENT_BID = 8,
};

enum AuctionSortingDirection
{
    SORT_ASC = 0,
    SORT_DESC = 1,
};


struct AuctionSearch
{
    Player* const m_player;
    std::wstring const& m_wsearchedname;
    uint32 m_listfrom;
    uint8 m_levelmin;
    uint8 m_levelmax;
    uint8 m_usable;
    uint32 m_inventoryType;
    uint32 m_itemClass;
    uint32 m_itemSubClass;
    uint32 m_quality;
    AuctionSortingCriterion m_sortingCriterion;
    AuctionSortingDirection m_sortingDirection;
    uint32& m_count;
    uint32& m_totalcount;

    AuctionSearch(Player* const player, std::wstring const& wsearchedname, uint32 listfrom, uint8 levelmin, uint8 levelmax, uint8 usable,
        uint32 inventoryType, uint32 itemClass, uint32 itemSubClass, uint32 quality,
        AuctionSortingCriterion sortingCriterion, AuctionSortingDirection sortingDirection, uint32& count, uint32& totalcount);
};



//this class is used as auctionhouse instance
class AuctionHouseObject
{
public:
    // Initialize storage
    AuctionHouseObject();
    ~AuctionHouseObject();

    uint32 Getcount() const;

    AuctionEntry* GetAuction(uint32 id) const;

    void AddAuction(AuctionEntry *auction);

    bool RemoveAuction(AuctionEntry *auction, uint32 itemEntry);

    void Update();

    void BuildListBidderItems(WorldPacket& data, Player* player, uint32& count, uint32& totalcount);
    void BuildListOwnerItems(WorldPacket& data, Player* player, uint32& count, uint32& totalcount);
    void BuildListAuctionItems(WorldPacket& data, Player* player,
        std::wstring const& searchedname, uint32 listfrom, uint8 levelmin, uint8 levelmax, uint8 usable,
        uint32 inventoryType, uint32 itemClass, uint32 itemSubClass, uint32 quality,
        uint32& count, uint32& totalcount, AuctionSortingCriterion sortingCriterion, AuctionSortingDirection sortingDirection);

private:
    typedef std::map<uint32, AuctionEntry*> AuctionEntryMap;
    AuctionEntryMap AuctionsMap;
    typedef std::list<const AuctionEntry*> AuctionList;

    std::multimap<ItemQualities, const AuctionEntry*> AuctionsMapByRarity;
    std::multimap<uint32 /*level*/, const AuctionEntry*> AuctionsMapByLevel;
    std::multimap<time_t, const AuctionEntry*> AuctionsMapByTimeLeft;
    std::multimap<std::string, const AuctionEntry*> AuctionsMapBySeller;
    std::multimap<uint64 /*bid*/, const AuctionEntry*> AuctionsMapByCurrentBid;

    void FinishAuctionOnTime(uint32 auctionId);
    bool ItemMatchesSearchCriteria(AuctionEntry const *Aentry, AuctionSearch const& search) const;

    template <class TKey, class TContainer>
    void RemoveFromAuctionMap(TContainer &container, const TKey &key, const AuctionEntry *auction);

    AuctionList GetAuctionsBySearchCriteria(const AuctionSearch &search) const;

    template <class TContainer>
    AuctionList GetAuctionsBySearchCriteria(TContainer &container, const AuctionSearch &search) const;

    template <class TIterator>
    AuctionList GetAuctionsBySearchCriteria(const TIterator &begin, const TIterator &end, const AuctionSearch &search) const;
};


#endif // _AUCTION_HOUSE_OBJECT_H
