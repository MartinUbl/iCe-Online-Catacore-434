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
    SORT_ASC = 1,
    SORT_DESC = 0,
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

    void FinishAuctionOnTime(uint32 auctionId);
};


#endif // _AUCTION_HOUSE_OBJECT_H
