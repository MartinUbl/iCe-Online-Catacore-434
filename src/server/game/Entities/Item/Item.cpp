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
 #include <ace/Auto_Ptr.h>

#include "Common.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "WorldPacket.h"
#include "DatabaseEnv.h"
#include "ItemEnchantmentMgr.h"
#include "SpellMgr.h"
#include "ScriptMgr.h"
#include "ConditionMgr.h"

void AddItemsSetItem(Player*player,Item *item)
{
    ItemPrototype const *proto = item->GetProto();
    uint32 setid = proto->ItemSet;

    ItemSetEntry const *set = sItemSetStore.LookupEntry(setid);

    if (!set)
    {
        sLog->outErrorDb("Item set %u for item (id %u) not found, mods not applied.",setid,proto->ItemId);
        return;
    }

    if (set->required_skill_id && player->GetSkillValue(set->required_skill_id) < set->required_skill_value)
        return;

    ItemSetEffect *eff = NULL;

    for (size_t x = 0; x < player->ItemSetEff.size(); ++x)
    {
        if (player->ItemSetEff[x] && player->ItemSetEff[x]->setid == setid)
        {
            eff = player->ItemSetEff[x];
            break;
        }
    }

    if (!eff)
    {
        eff = new ItemSetEffect;
        memset(eff,0,sizeof(ItemSetEffect));
        eff->setid = setid;

        size_t x = 0;
        for (; x < player->ItemSetEff.size(); x++)
            if (!player->ItemSetEff[x])
                break;

        if (x < player->ItemSetEff.size())
            player->ItemSetEff[x]=eff;
        else
            player->ItemSetEff.push_back(eff);
    }

    ++eff->item_count;

    for (uint32 x = 0; x < MAX_ITEM_SET_SPELLS; x++)
    {
        if (!set->spells [x])
            continue;
        //not enough for  spell
        if (set->items_to_triggerspell[x] > eff->item_count)
            continue;

        uint32 z = 0;
        for (; z < MAX_ITEM_SET_SPELLS; z++)
            if (eff->spells[z] && eff->spells[z]->Id == set->spells[x])
                break;

        if (z < MAX_ITEM_SET_SPELLS)
            continue;

        //new spell
        for (uint32 y = 0; y < MAX_ITEM_SET_SPELLS; y++)
        {
            if (!eff->spells[y])                             // free slot
            {
                SpellEntry const *spellInfo = sSpellStore.LookupEntry(set->spells[x]);
                if (!spellInfo)
                {
                    sLog->outError("WORLD: unknown spell id %u in items set %u effects", set->spells[x],setid);
                    break;
                }

                // spell casted only if fit form requirement, in other case will casted at form change
                player->ApplyEquipSpell(spellInfo,NULL,true);
                eff->spells[y] = spellInfo;
                break;
            }
        }
    }
}

void RemoveItemsSetItem(Player*player,ItemPrototype const *proto)
{
    uint32 setid = proto->ItemSet;

    ItemSetEntry const *set = sItemSetStore.LookupEntry(setid);

    if (!set)
    {
        sLog->outErrorDb("Item set #%u for item #%u not found, mods not removed.",setid,proto->ItemId);
        return;
    }

    ItemSetEffect *eff = NULL;
    size_t setindex = 0;
    for (; setindex < player->ItemSetEff.size(); setindex++)
    {
        if (player->ItemSetEff[setindex] && player->ItemSetEff[setindex]->setid == setid)
        {
            eff = player->ItemSetEff[setindex];
            break;
        }
    }

    // can be in case now enough skill requirement for set appling but set has been appliend when skill requirement not enough
    if (!eff)
        return;

    --eff->item_count;

    for (uint32 x = 0; x < MAX_ITEM_SET_SPELLS; x++)
    {
        if (!set->spells[x])
            continue;

        // enough for spell
        if (set->items_to_triggerspell[x] <= eff->item_count)
            continue;

        for (uint32 z = 0; z < MAX_ITEM_SET_SPELLS; z++)
        {
            if (eff->spells[z] && eff->spells[z]->Id == set->spells[x])
            {
                // spell can be not active if not fit form requirement
                player->ApplyEquipSpell(eff->spells[z],NULL,false);
                eff->spells[z]=NULL;
                break;
            }
        }
    }

    if (!eff->item_count)                                    //all items of a set were removed
    {
        ASSERT(eff == player->ItemSetEff[setindex]);
        delete eff;
        player->ItemSetEff[setindex] = NULL;
    }
}

bool ItemCanGoIntoBag(ItemPrototype const *pProto, ItemPrototype const *pBagProto)
{
    if (!pProto || !pBagProto)
        return false;

    switch(pBagProto->Class)
    {
        case ITEM_CLASS_CONTAINER:
            switch(pBagProto->SubClass)
            {
                case ITEM_SUBCLASS_CONTAINER:
                    return true;
                case ITEM_SUBCLASS_SOUL_CONTAINER:
                    if (!(pProto->BagFamily & BAG_FAMILY_MASK_SOUL_SHARDS))
                        return false;
                    return true;
                case ITEM_SUBCLASS_HERB_CONTAINER:
                    if (!(pProto->BagFamily & BAG_FAMILY_MASK_HERBS))
                        return false;
                    return true;
                case ITEM_SUBCLASS_ENCHANTING_CONTAINER:
                    if (!(pProto->BagFamily & BAG_FAMILY_MASK_ENCHANTING_SUPP))
                        return false;
                    return true;
                case ITEM_SUBCLASS_MINING_CONTAINER:
                    if (!(pProto->BagFamily & BAG_FAMILY_MASK_MINING_SUPP))
                        return false;
                    return true;
                case ITEM_SUBCLASS_ENGINEERING_CONTAINER:
                    if (!(pProto->BagFamily & BAG_FAMILY_MASK_ENGINEERING_SUPP))
                        return false;
                    return true;
                case ITEM_SUBCLASS_GEM_CONTAINER:
                    if (!(pProto->BagFamily & BAG_FAMILY_MASK_GEMS))
                        return false;
                    return true;
                case ITEM_SUBCLASS_LEATHERWORKING_CONTAINER:
                    if (!(pProto->BagFamily & BAG_FAMILY_MASK_LEATHERWORKING_SUPP))
                        return false;
                    return true;
                case ITEM_SUBCLASS_INSCRIPTION_CONTAINER:
                    if (!(pProto->BagFamily & BAG_FAMILY_MASK_INSCRIPTION_SUPP))
                        return false;
                    return true;
                case ITEM_SUBCLASS_FISHING_CONTAINER:
                    if (!(pProto->BagFamily & BAG_FAMILY_MASK_FISHING))
                        return false;
                    return true;
                default:
                    return false;
            }
        case ITEM_CLASS_QUIVER:
            switch(pBagProto->SubClass)
            {
                case ITEM_SUBCLASS_QUIVER:
                    if (!(pProto->BagFamily & BAG_FAMILY_MASK_ARROWS))
                        return false;
                    return true;
                case ITEM_SUBCLASS_AMMO_POUCH:
                    if (!(pProto->BagFamily & BAG_FAMILY_MASK_BULLETS))
                        return false;
                    return true;
                default:
                    return false;
            }
    }
    return false;
}

uint32 ItemPrototype::GetArmor() const
{
    if(Quality >= ITEM_QUALITY_HEIRLOOM)                    // heirlooms have it's own dbc...
        return 0;
    
    if(Class == ITEM_CLASS_ARMOR && SubClass == ITEM_SUBCLASS_ARMOR_SHIELD)
    {
        if(ItemArmorShieldEntry const* ias = sItemArmorShieldStore.LookupEntry(ItemLevel))
        {
            return uint32(floor(ias->Value[Quality] + 0.5f));
        }
        return 0;
    }
    
    ItemArmorQualityEntry const* iaq = sItemArmorQualityStore.LookupEntry(ItemLevel);
    ItemArmorTotalEntry const* iat = sItemArmorTotalStore.LookupEntry(ItemLevel);
    
    if(!iaq || !iat)
        return 0;
    
    if(InventoryType != INVTYPE_HEAD && InventoryType != INVTYPE_CHEST && InventoryType != INVTYPE_SHOULDERS
       && InventoryType != INVTYPE_LEGS && InventoryType != INVTYPE_FEET && InventoryType != INVTYPE_WRISTS
       && InventoryType != INVTYPE_HANDS && InventoryType != INVTYPE_WAIST && InventoryType != INVTYPE_CLOAK
       && InventoryType != INVTYPE_ROBE)
        return 0;
    
    ArmorLocationEntry const* al = NULL;
    
    if(InventoryType == INVTYPE_ROBE)
        al = sArmorLocationStore.LookupEntry(INVTYPE_CHEST);
    else
        al = sArmorLocationStore.LookupEntry(InventoryType);
    
    if(!al)
        return 0;
    
    float iatMult, alMult;
    
    switch(SubClass)
    {
        case ITEM_SUBCLASS_ARMOR_CLOTH:
            iatMult = iat->Value[0];
            alMult = al->Value[0];
            break;
        case ITEM_SUBCLASS_ARMOR_LEATHER:
            iatMult = iat->Value[1];
            alMult = al->Value[1];
            break;
        case ITEM_SUBCLASS_ARMOR_MAIL:
            iatMult = iat->Value[2];
            alMult = al->Value[2];
            break;
        case ITEM_SUBCLASS_ARMOR_PLATE:
            iatMult = iat->Value[3];
            alMult = al->Value[3];
            break;
        default:
            return 0;
    }
    
    return uint32(floor(iaq->Value[Quality] * iatMult * alMult + 0.5f));
}

ItemDamageEntry const * ItemPrototype::getItemDamageEntry() const
{
    if(Class == ITEM_CLASS_WEAPON)
    {
        if(Quality >= ITEM_QUALITY_HEIRLOOM)                // heirlooms have it's own dbc...
            return NULL;
        
        ItemDamageEntry const* id = NULL;
        
        switch(InventoryType)
        {
            case INVTYPE_WEAPON:
            case INVTYPE_WEAPONMAINHAND:
            case INVTYPE_WEAPONOFFHAND:
                if(Flags2 & ITEM_FLAGS_EXTRA_CASTER_WEAPON)      // caster weapon flag
                    id = sItemDamageOneHandCasterStore.LookupEntry(ItemLevel);
                else
                    id = sItemDamageOneHandStore.LookupEntry(ItemLevel);
                break;
            case INVTYPE_2HWEAPON:
                if(Flags2 & ITEM_FLAGS_EXTRA_CASTER_WEAPON)      // caster weapon flag
                    id = sItemDamageTwoHandCasterStore.LookupEntry(ItemLevel);
                else
                    id = sItemDamageTwoHandStore.LookupEntry(ItemLevel);
                break;
            case INVTYPE_AMMO:
                id = sItemDamageAmmoStore.LookupEntry(ItemLevel);
                break;
            case INVTYPE_RANGED:
            case INVTYPE_THROWN:
            case INVTYPE_RANGEDRIGHT:
                switch(SubClass)
            {
                case ITEM_SUBCLASS_WEAPON_BOW:
                case ITEM_SUBCLASS_WEAPON_GUN:
                case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                    id = sItemDamageRangedStore.LookupEntry(ItemLevel);
                    break;
                case ITEM_SUBCLASS_WEAPON_THROWN:
                    id = sItemDamageThrownStore.LookupEntry(ItemLevel);
                    break;
                case ITEM_SUBCLASS_WEAPON_WAND:
                    id = sItemDamageWandStore.LookupEntry(ItemLevel);
                    break;
                default:
                    break;
            }
                break;
            default:
                break;
        }
        
        if(id)
            return id;
    }
    return NULL;
}

float ItemPrototype::getDPS() const
{
    ItemDamageEntry const* id = getItemDamageEntry();
    if(id)
        return id->Value[Quality];
    return 0.0f;
}

float ItemPrototype::GetDamageRange() const
{
    ItemSparseEntry const *sparse = sItemSparseStore.LookupEntry(ItemId);
    if (!sparse)
        return 0.3f;   // this value is accurate for many existing weapons

    return sparse->DamageRangeScale / 2.0f;
}

float ItemPrototype::GetMinDamage() const
{
    float avgDamage = getDPS() * float(Delay) / 1000.0f;
    return avgDamage * (1.0f - GetDamageRange());
}

float ItemPrototype::GetMaxDamage() const
{
    float avgDamage = getDPS() * float(Delay) / 1000.0f;
    return avgDamage * (1.0f + GetDamageRange());
}


Item::Item()
{
    m_objectType |= TYPEMASK_ITEM;
    m_objectTypeId = TYPEID_ITEM;

    m_updateFlag = 0;

    m_valuesCount = ITEM_END;
    m_slot = 0;
    uState = ITEM_NEW;
    uQueuePos = -1;
    m_container = NULL;
    m_lootGenerated = false;
    mb_in_trade = false;
    m_lastPlayedTimeUpdate = time(NULL);
    m_playedTime = 0;

    m_refundRecipient = 0;
    m_paidMoney = 0;
    m_paidExtendedCost = 0;
}

bool Item::Create(uint32 guidlow, uint32 itemid, Player const* owner)
{
    Object::_Create(guidlow, 0, HIGHGUID_ITEM);

    SetEntry(itemid);
    SetFloatValue(OBJECT_FIELD_SCALE_X, 1.0f);

    SetUInt64Value(ITEM_FIELD_OWNER, owner ? owner->GetGUID() : 0);
    SetUInt64Value(ITEM_FIELD_CONTAINED, owner ? owner->GetGUID() : 0);

    ItemPrototype const *itemProto = sObjectMgr->GetItemPrototype(itemid);
    if (!itemProto)
        return false;

    SetUInt32Value(ITEM_FIELD_STACK_COUNT, 1);
    SetUInt32Value(ITEM_FIELD_MAXDURABILITY, itemProto->MaxDurability);
    SetUInt32Value(ITEM_FIELD_DURABILITY, itemProto->MaxDurability);

    for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
        SetSpellCharges(i,itemProto->Spells[i].SpellCharges);

    SetUInt32Value(ITEM_FIELD_DURATION, abs(itemProto->Duration));
    SetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME, owner ? owner->GetTotalPlayedTime() : 0);
    return true;
}

void Item::UpdateDuration(Player* owner, uint32 diff)
{
    if (!GetUInt32Value(ITEM_FIELD_DURATION))
        return;

    sLog->outDebug("Item::UpdateDuration Item (Entry: %u Duration %u Diff %u)",GetEntry(),GetUInt32Value(ITEM_FIELD_DURATION),diff);

    if (GetUInt32Value(ITEM_FIELD_DURATION) <= diff)
    {
        sScriptMgr->OnItemExpire(owner, GetProto());
        owner->DestroyItem(GetBagSlot(), GetSlot(), true);
        return;
    }

    SetUInt32Value(ITEM_FIELD_DURATION, GetUInt32Value(ITEM_FIELD_DURATION) - diff);
    SetState(ITEM_CHANGED, owner);                          // save new time in database
}

void Item::SaveToDB(SQLTransaction& trans)
{
    uint32 guid = GetGUIDLow();
    switch (uState)
    {
        case ITEM_NEW:
        case ITEM_CHANGED:
        {
            uint8 index = 0;
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(uState == ITEM_NEW ? CHAR_ADD_ITEM_INSTANCE : CHAR_UPDATE_ITEM_INSTANCE);
            stmt->setUInt32(  index, GetEntry());
            stmt->setUInt32(++index, GUID_LOPART(GetOwnerGUID()));
            stmt->setUInt32(++index, GUID_LOPART(GetUInt64Value(ITEM_FIELD_CREATOR)));
            stmt->setUInt32(++index, GUID_LOPART(GetUInt64Value(ITEM_FIELD_GIFTCREATOR)));
            stmt->setUInt32(++index, GetCount());
            stmt->setUInt32(++index, GetUInt32Value(ITEM_FIELD_DURATION));

            std::ostringstream ssSpells;
            for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
                ssSpells << GetSpellCharges(i) << " ";
            stmt->setString(++index, ssSpells.str());

            stmt->setUInt32(++index, GetUInt32Value(ITEM_FIELD_FLAGS));

            // delete enchantments
            trans->PAppend("DELETE FROM item_enchantments where guid = %d", guid);

            stmt->setInt32 (++index, GetItemRandomPropertyId());
            stmt->setUInt32(++index, GetUInt32Value(ITEM_FIELD_DURABILITY));
            stmt->setUInt32(++index, GetPlayedTime());
            stmt->setString(++index, m_text);
            stmt->setUInt32(++index, guid);

            trans->Append(stmt);

            if ((uState == ITEM_CHANGED) && HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_WRAPPED))
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPDATE_GIFT_OWNER);
                stmt->setUInt32(0, GUID_LOPART(GetOwnerGUID()));
                stmt->setUInt32(1, guid);
                trans->Append(stmt);
            }

            // insert new enchantments
            for (uint32 i = 0; i < MAX_ENCHANTMENT_SLOT; ++i)
            {
                EnchantmentSlot slot = EnchantmentSlot(i);
                uint32 id = GetEnchantmentId(slot);
                if (id != 0)
                {
                    uint32 duration = GetEnchantmentDuration(slot);
                    uint32 charges = GetEnchantmentCharges(slot);

                    std::ostringstream ssEnchants;
                    ssEnchants << "(" << guid << ", ";
                    ssEnchants << i << ", ";
                    ssEnchants << id << ", ";
                    ssEnchants << duration << ", ";
                    ssEnchants << charges << ");";

                    trans->PAppend("INSERT INTO item_enchantments (guid, slot, id, duration, charges) "
                                   "VALUES %s", ssEnchants.str().c_str());
                }
            }

            break;
        }
        case ITEM_REMOVED:
        {
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE);
            stmt->setUInt32(0, guid);
            trans->Append(stmt);

            if (HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_WRAPPED))
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GIFT);
                stmt->setUInt32(0, guid);
                trans->Append(stmt);
            }
            delete this;
            return;
        }
        case ITEM_UNCHANGED:
            break;
    }
    SetState(ITEM_UNCHANGED);
}

bool Item::LoadFromDB(uint32 guid, uint64 owner_guid, Field* fields, uint32 entry)
{
    //                                                    0                1      2         3        4      5                 6           7           8     9
    //result = CharacterDatabase.PQuery("SELECT creatorGuid, giftCreatorGuid, count, duration, charges, flags, randomPropertyId, durability, playedTime, text FROM item_instance WHERE guid = '%u'", guid);

    // create item before any checks for store correct guid
    // and allow use "FSetState(ITEM_REMOVED); SaveToDB();" for deleting item from DB
    Object::_Create(guid, 0, HIGHGUID_ITEM);

    // Set entry, MUST be before proto check
    SetEntry(entry);
    SetFloatValue(OBJECT_FIELD_SCALE_X, 1.0f);

    ItemPrototype const* proto = GetProto();
    if (!proto)
        return false;

    // set owner (not if item is only loaded for gbank/auction/mail
    if (owner_guid != 0)
        SetOwnerGUID(owner_guid);

    bool need_save = false;                                 // need explicit save data at load fixes
    SetUInt64Value(ITEM_FIELD_CREATOR, MAKE_NEW_GUID(fields[0].GetUInt32(), 0, HIGHGUID_PLAYER));
    SetUInt64Value(ITEM_FIELD_GIFTCREATOR, MAKE_NEW_GUID(fields[1].GetUInt32(), 0, HIGHGUID_PLAYER));
    SetCount(fields[2].GetUInt32());

    uint32 duration = fields[3].GetUInt32();
    SetUInt32Value(ITEM_FIELD_DURATION, duration);
    // update duration if need, and remove if not need
    if ((proto->Duration == 0) != (duration == 0))
    {
        SetUInt32Value(ITEM_FIELD_DURATION, abs(proto->Duration));
        need_save = true;
    }

    Tokens tokens(fields[4].GetString(), ' ', MAX_ITEM_PROTO_SPELLS);
    if (tokens.size() == MAX_ITEM_PROTO_SPELLS)
        for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
            SetSpellCharges(i, atoi(tokens[i]));

    SetUInt32Value(ITEM_FIELD_FLAGS, fields[5].GetUInt32());
    // Remove bind flag for items vs NO_BIND set
    if (IsSoulBound() && proto->Bonding == NO_BIND)
    {
        ApplyModFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_SOULBOUND, false);
        need_save = true;
    }

    // load enchantments from table item_enchantments
    QueryResult enchantments = CharacterDatabase.PQuery("SELECT slot, id, duration, charges from item_enchantments "
                                                            "where guid = %d", guid);

    if (enchantments && enchantments->GetRowCount() > 0)
    {
        do 
        {
            Field* fields = enchantments->Fetch();
            uint32 slot = fields[0].GetInt32(),
                   id = fields[1].GetInt32(),
                   duration = fields[2].GetInt32(),
                   charges = fields[3].GetInt32();

            if (slot >= MAX_ENCHANTMENT_SLOT)
            {
                sLog->outError("Invalid enchantment slot (%d) for item %d.", slot, guid);
                continue;
            }
            m_uint32Values[ITEM_FIELD_ENCHANTMENT_1_1 + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_ID_OFFSET] = id;
            m_uint32Values[ITEM_FIELD_ENCHANTMENT_1_1 + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_DURATION_OFFSET] = duration;
            m_uint32Values[ITEM_FIELD_ENCHANTMENT_1_1 + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_CHARGES_OFFSET] = charges;
        } while (enchantments->NextRow());
    }

    // Reforge is stored in slot 8
    m_reforgeId = GetEnchantmentId(REFORGING_ENCHANTMENT_SLOT);
    SetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID, fields[6].GetInt32());
    // recalculate suffix factor
    if (GetItemRandomPropertyId() < 0)
        UpdateItemSuffixFactor();

    uint32 durability = fields[7].GetUInt32();
    SetUInt32Value(ITEM_FIELD_DURABILITY, durability);
    // update max durability (and durability) if need
    SetUInt32Value(ITEM_FIELD_MAXDURABILITY, proto->MaxDurability);
    if (durability > proto->MaxDurability)
    {
        SetUInt32Value(ITEM_FIELD_DURABILITY, proto->MaxDurability);
        need_save = true;
    }

    m_playedTime = fields[8].GetUInt32();
    SetText(fields[9].GetString());

    if (need_save)                                           // normal item changed state set not work at loading
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPDATE_ITEM_INSTANCE_ON_LOAD);
        stmt->setUInt32(0, GetUInt32Value(ITEM_FIELD_DURATION));
        stmt->setUInt32(1, GetUInt32Value(ITEM_FIELD_FLAGS));
        stmt->setUInt32(2, GetUInt32Value(ITEM_FIELD_DURABILITY));
        stmt->setUInt32(3, guid);
        CharacterDatabase.Execute(stmt);
    }

    return true;
}

void Item::DeleteFromDB(SQLTransaction& trans)
{
    trans->PAppend("DELETE FROM item_enchantments WHERE guid = %d;", GetGUIDLow());

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE);
    stmt->setUInt32(0, GetGUIDLow());
    trans->Append(stmt);
}

void Item::DeleteFromInventoryDB(SQLTransaction& trans)
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INVENTORY_ITEM);
    stmt->setUInt32(0, GetGUIDLow());
    trans->Append(stmt);
}

ItemPrototype const *Item::GetProto() const
{
    return sObjectMgr->GetItemPrototype(GetEntry());
}

Player* Item::GetOwner()const
{
    return sObjectMgr->GetPlayer(GetOwnerGUID());
}

uint32 Item::GetSkill()
{
    const static uint32 item_weapon_skills[MAX_ITEM_SUBCLASS_WEAPON] =
    {
        SKILL_AXES,     SKILL_2H_AXES,  SKILL_BOWS,          SKILL_GUNS,      SKILL_MACES,
        SKILL_2H_MACES, SKILL_POLEARMS, SKILL_SWORDS,        SKILL_2H_SWORDS, 0,
        SKILL_STAVES,   0,              0,                   SKILL_FIST_WEAPONS,   0,
        SKILL_DAGGERS,  SKILL_THROWN,   SKILL_ASSASSINATION, SKILL_CROSSBOWS, SKILL_WANDS,
        SKILL_FISHING
    };

    const static uint32 item_armor_skills[MAX_ITEM_SUBCLASS_ARMOR] =
    {
        0,SKILL_CLOTH,SKILL_LEATHER,SKILL_MAIL,SKILL_PLATE_MAIL,0,SKILL_SHIELD,0,0,0,0
    };

    ItemPrototype const* proto = GetProto();

    switch (proto->Class)
    {
        case ITEM_CLASS_WEAPON:
            if (proto->SubClass >= MAX_ITEM_SUBCLASS_WEAPON)
                return 0;
            else
                return item_weapon_skills[proto->SubClass];

        case ITEM_CLASS_ARMOR:
            if (proto->SubClass >= MAX_ITEM_SUBCLASS_ARMOR)
                return 0;
            else
                return item_armor_skills[proto->SubClass];

        default:
            return 0;
    }
}

uint32 Item::GetSpell()
{
    ItemPrototype const* proto = GetProto();

    switch (proto->Class)
    {
        case ITEM_CLASS_WEAPON:
            switch (proto->SubClass)
            {
                case ITEM_SUBCLASS_WEAPON_AXE:     return  196;
                case ITEM_SUBCLASS_WEAPON_AXE2:    return  197;
                case ITEM_SUBCLASS_WEAPON_BOW:     return  264;
                case ITEM_SUBCLASS_WEAPON_GUN:     return  266;
                case ITEM_SUBCLASS_WEAPON_MACE:    return  198;
                case ITEM_SUBCLASS_WEAPON_MACE2:   return  199;
                case ITEM_SUBCLASS_WEAPON_POLEARM: return  200;
                case ITEM_SUBCLASS_WEAPON_SWORD:   return  201;
                case ITEM_SUBCLASS_WEAPON_SWORD2:  return  202;
                case ITEM_SUBCLASS_WEAPON_STAFF:   return  227;
                case ITEM_SUBCLASS_WEAPON_DAGGER:  return 1180;
                case ITEM_SUBCLASS_WEAPON_THROWN:  return 2567;
                case ITEM_SUBCLASS_WEAPON_SPEAR:   return 3386;
                case ITEM_SUBCLASS_WEAPON_CROSSBOW:return 5011;
                case ITEM_SUBCLASS_WEAPON_WAND:    return 5009;
                default: return 0;
            }
        case ITEM_CLASS_ARMOR:
            switch (proto->SubClass)
            {
                case ITEM_SUBCLASS_ARMOR_CLOTH:    return 9078;
                case ITEM_SUBCLASS_ARMOR_LEATHER:  return 9077;
                case ITEM_SUBCLASS_ARMOR_MAIL:     return 8737;
                case ITEM_SUBCLASS_ARMOR_PLATE:    return  750;
                case ITEM_SUBCLASS_ARMOR_SHIELD:   return 9116;
                default: return 0;
            }
    }
    return 0;
}

int32 Item::GenerateItemRandomPropertyId(uint32 item_id)
{
    ItemPrototype const *itemProto = sItemStorage.LookupEntry<ItemPrototype>(item_id);

    if (!itemProto)
        return 0;

    // item must have one from this field values not null if it can have random enchantments
    if ((!itemProto->RandomProperty) && (!itemProto->RandomSuffix))
        return 0;

    // item can have not null only one from field values
    if ((itemProto->RandomProperty) && (itemProto->RandomSuffix))
    {
        sLog->outErrorDb("Item template %u have RandomProperty == %u and RandomSuffix == %u, but must have one from field =0",itemProto->ItemId,itemProto->RandomProperty,itemProto->RandomSuffix);
        return 0;
    }

    // RandomProperty case
    if (itemProto->RandomProperty)
    {
        uint32 randomPropId = GetItemEnchantMod(itemProto->RandomProperty);
        ItemRandomPropertiesEntry const *random_id = sItemRandomPropertiesStore.LookupEntry(randomPropId);
        if (!random_id)
        {
            sLog->outErrorDb("Enchantment id #%u used but it doesn't have records in 'ItemRandomProperties.dbc'",randomPropId);
            return 0;
        }

        return random_id->ID;
    }
    // RandomSuffix case
    else
    {
        uint32 randomPropId = GetItemEnchantMod(itemProto->RandomSuffix);
        ItemRandomSuffixEntry const *random_id = sItemRandomSuffixStore.LookupEntry(randomPropId);
        if (!random_id)
        {
            sLog->outErrorDb("Enchantment id #%u used but it doesn't have records in sItemRandomSuffixStore.",randomPropId);
            return 0;
        }

        return -int32(random_id->ID);
    }
}

void Item::SetItemRandomProperties(int32 randomPropId)
{
    if (!randomPropId)
        return;

    if (randomPropId > 0)
    {
        ItemRandomPropertiesEntry const *item_rand = sItemRandomPropertiesStore.LookupEntry(randomPropId);
        if (item_rand)
        {
            if (GetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID) != int32(item_rand->ID))
            {
                SetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID,item_rand->ID);
                SetState(ITEM_CHANGED, GetOwner());
            }
            for (uint32 i = PROP_ENCHANTMENT_SLOT_0; i < PROP_ENCHANTMENT_SLOT_0 + MAX_ITEM_ENCHANTMENT_RANDOM_ENTRIES; ++i)
                SetEnchantment(EnchantmentSlot(i),item_rand->enchant_id[i - PROP_ENCHANTMENT_SLOT_0],0,0);
        }
    }
    else
    {
        ItemRandomSuffixEntry const *item_rand = sItemRandomSuffixStore.LookupEntry(-randomPropId);
        if (item_rand)
        {
            if (GetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID) != -int32(item_rand->ID) ||
                !GetItemSuffixFactor())
            {
                SetInt32Value(ITEM_FIELD_RANDOM_PROPERTIES_ID,-int32(item_rand->ID));
                UpdateItemSuffixFactor();
                SetState(ITEM_CHANGED, GetOwner());
            }

            for (uint32 i = PROP_ENCHANTMENT_SLOT_0; i < PROP_ENCHANTMENT_SLOT_0 + MAX_ITEM_ENCHANTMENT_RANDOM_ENTRIES; ++i)
                SetEnchantment(EnchantmentSlot(i),item_rand->enchant_id[i - PROP_ENCHANTMENT_SLOT_0],0,0);
        }
    }
}

void Item::UpdateItemSuffixFactor()
{
    uint32 suffixFactor = GenerateEnchSuffixFactor(GetEntry());
    if (GetItemSuffixFactor() == suffixFactor)
        return;
    SetUInt32Value(ITEM_FIELD_PROPERTY_SEED, suffixFactor);
}

void Item::SetState(ItemUpdateState state, Player *forplayer)
{
    if (uState == ITEM_NEW && state == ITEM_REMOVED)
    {
        // pretend the item never existed
        RemoveFromUpdateQueueOf(forplayer);
        forplayer->DeleteRefundReference(GetGUIDLow());
        delete this;
        return;
    }
    if (state != ITEM_UNCHANGED)
    {
        // new items must stay in new state until saved
        if (uState != ITEM_NEW)
            uState = state;

        AddToUpdateQueueOf(forplayer);
    }
    else
    {
        // unset in queue
        // the item must be removed from the queue manually
        uQueuePos = -1;
        uState = ITEM_UNCHANGED;
    }
}

void Item::AddToUpdateQueueOf(Player *player)
{
    if (IsInUpdateQueue())
        return;

    ASSERT(player != NULL);

    if (player->GetGUID() != GetOwnerGUID())
    {
        sLog->outDebug("Item::AddToUpdateQueueOf - Owner's guid (%u) and player's guid (%u) don't match!", GUID_LOPART(GetOwnerGUID()), player->GetGUIDLow());
        return;
    }

    if (player->m_itemUpdateQueueBlocked)
        return;

    player->m_itemUpdateQueue.push_back(this);
    uQueuePos = player->m_itemUpdateQueue.size()-1;
}

void Item::RemoveFromUpdateQueueOf(Player *player)
{
    if (!IsInUpdateQueue())
        return;

    ASSERT(player != NULL)

    if (player->GetGUID() != GetOwnerGUID())
    {
        sLog->outDebug("Item::RemoveFromUpdateQueueOf - Owner's guid (%u) and player's guid (%u) don't match!", GUID_LOPART(GetOwnerGUID()), player->GetGUIDLow());
        return;
    }

    if (player->m_itemUpdateQueueBlocked)
        return;

    player->m_itemUpdateQueue[uQueuePos] = NULL;
    uQueuePos = -1;
}

uint8 Item::GetBagSlot() const
{
    return m_container ? m_container->GetSlot() : uint8(INVENTORY_SLOT_BAG_0);
}

bool Item::IsEquipped() const
{
    return !IsInBag() && m_slot < EQUIPMENT_SLOT_END;
}

bool Item::CanBeTraded(bool mail, bool trade) const
{
    if (m_lootGenerated)
        return false;

    if ((!mail || !IsBoundAccountWide()) && (IsSoulBound() && (!HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_BOP_TRADEABLE) || !trade)))
        return false;

    if (IsBag() && (Player::IsBagPos(GetPos()) || !((Bag const*)this)->IsEmpty()))
        return false;

    if (Player* owner = GetOwner())
    {
        if (owner->CanUnequipItem(GetPos(),false) !=  EQUIP_ERR_OK)
            return false;
        if (owner->GetLootGUID() == GetGUID())
            return false;
    }

    if (IsBoundByEnchant())
        return false;

    return true;
}


bool Item::HasEnchantRequiredSkill(const Player *pPlayer) const
{
    // Check all enchants for required skill
    for (uint32 enchant_slot = PERM_ENCHANTMENT_SLOT; enchant_slot < MAX_ENCHANTMENT_SLOT; ++enchant_slot)
        if (uint32 enchant_id = GetEnchantmentId(EnchantmentSlot(enchant_slot)))
            if (SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id))
                if (enchantEntry->requiredSkill && pPlayer->GetSkillValue(enchantEntry->requiredSkill) < enchantEntry->requiredSkillValue)
                    return false;

  return true;
}


uint32 Item::GetEnchantRequiredLevel() const
{
    uint32 level = 0;

    // Check all enchants for required level
    for (uint32 enchant_slot = PERM_ENCHANTMENT_SLOT; enchant_slot < MAX_ENCHANTMENT_SLOT; ++enchant_slot)
        if (uint32 enchant_id = GetEnchantmentId(EnchantmentSlot(enchant_slot)))
            if (SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id))
                if (enchantEntry->requiredLevel > level)
                    level = enchantEntry->requiredLevel;

    return level;
}

bool Item::IsBoundByEnchant() const
{
    // Check all enchants for soulbound
    for (uint32 enchant_slot = PERM_ENCHANTMENT_SLOT; enchant_slot < MAX_ENCHANTMENT_SLOT; ++enchant_slot)
        if (uint32 enchant_id = GetEnchantmentId(EnchantmentSlot(enchant_slot)))
            if (SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id))
                if (enchantEntry->slot & ENCHANTMENT_CAN_SOULBOUND)
                    return true;
    return false;
}

uint8 Item::CanBeMergedPartlyWith(ItemPrototype const* proto) const
{
    // not allow merge looting currently items
    if (m_lootGenerated)
        return EQUIP_ERR_ALREADY_LOOTED;

    // check item type
    if (GetEntry() != proto->ItemId)
        return EQUIP_ERR_ITEM_CANT_STACK;

    // check free space (full stacks can't be target of merge
    if (GetCount() >= proto->GetMaxStackSize())
        return EQUIP_ERR_ITEM_CANT_STACK;

    return EQUIP_ERR_OK;
}

bool Item::IsFitToSpellRequirements(SpellEntry const* spellInfo) const
{
    ItemPrototype const* proto = GetProto();
    
    if (spellInfo->EquippedItemClass != -1)                 // -1 == any item class
    {
        // Special case - accept vellum for armor/weapon requirements
        if ((spellInfo->EquippedItemClass == ITEM_CLASS_ARMOR ||
            spellInfo->EquippedItemClass == ITEM_CLASS_WEAPON) && proto->IsVellum())
                return true;

        if (spellInfo->EquippedItemClass != int32(proto->Class))
            return false;                                   //  wrong item class

        if (spellInfo->EquippedItemSubClassMask != 0)        // 0 == any subclass
        {
            if ((spellInfo->EquippedItemSubClassMask & (1 << proto->SubClass)) == 0)
                return false;                               // subclass not present in mask
        }
    }

    if (spellInfo->EquippedItemInventoryTypeMask != 0)       // 0 == any inventory type
    {
        // Special case - accept weapon type for main and offhand requirements
        if (proto->InventoryType == INVTYPE_WEAPON &&
            (spellInfo->EquippedItemInventoryTypeMask & (1 << INVTYPE_WEAPONMAINHAND) ||
             spellInfo->EquippedItemInventoryTypeMask & (1 << INVTYPE_WEAPONOFFHAND)))
            return true;
        else if ((spellInfo->EquippedItemInventoryTypeMask & (1 << proto->InventoryType)) == 0)
            return false;                                   // inventory type not present in mask
    }

    return true;
}

bool Item::IsTargetValidForItemUse(Unit* pUnitTarget)
{
    ConditionList conditions = sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_ITEM_REQUIRED_TARGET, GetProto()->ItemId);
    if (conditions.empty())
        return true;

    if (!pUnitTarget)
        return false;

    for (ConditionList::const_iterator itr = conditions.begin(); itr != conditions.end(); ++itr)
    {
        ACE_Auto_Ptr<ItemRequiredTarget> irt(new ItemRequiredTarget((ItemRequiredTargetType)(*itr)->mConditionValue1, (*itr)->mConditionValue2));
        if (irt->IsFitToRequirements(pUnitTarget))
            return true;
    }
    return false;
}

void Item::SetEnchantment(EnchantmentSlot slot, uint32 id, uint32 duration, uint32 charges)
{
    // Better lost small time at check in comparison lost time at item save to DB.
    if ((GetEnchantmentId(slot) == id) && (GetEnchantmentDuration(slot) == duration) && (GetEnchantmentCharges(slot) == charges))
        return;

    SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_ID_OFFSET,id);
    SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_DURATION_OFFSET,duration);
    SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_CHARGES_OFFSET,charges);
    SetState(ITEM_CHANGED, GetOwner());
}

void Item::SetEnchantmentDuration(EnchantmentSlot slot, uint32 duration, Player* owner)
{
    if (GetEnchantmentDuration(slot) == duration)
        return;

    SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_DURATION_OFFSET,duration);
    SetState(ITEM_CHANGED, owner);
    // Cannot use GetOwner() here, has to be passed as an argument to avoid freeze due to hashtable locking
}

void Item::SetEnchantmentCharges(EnchantmentSlot slot, uint32 charges)
{
    if (GetEnchantmentCharges(slot) == charges)
        return;

    SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_CHARGES_OFFSET,charges);
    SetState(ITEM_CHANGED, GetOwner());
}

void Item::ClearEnchantment(EnchantmentSlot slot)
{
    if (!GetEnchantmentId(slot))
        return;

    for (uint8 x = 0; x < MAX_ITEM_ENCHANTMENT_EFFECTS; ++x)
        SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + slot*MAX_ENCHANTMENT_OFFSET + x, 0);
    SetState(ITEM_CHANGED, GetOwner());
}


void Player::ApplyReforge(Item *item, bool apply)
{
    // Modify stats only if item is equipped
    if (!item || !item->IsEquipped())
        return;

    uint32 reforge_id = item->GetEnchantmentId(REFORGING_ENCHANTMENT_SLOT);
    if (!reforge_id)
        return;

    ItemReforgeEntry const *pReforge = sItemReforgeStore.LookupEntry(reforge_id);
    if (!pReforge)
        return;

    const ItemPrototype* proto = item->GetProto();
    if (!proto)
        return;

    // Only items with item level 200 or higher could be reforged
    if (proto->ItemLevel < 200)
        return;

    // Get source stat/rating amount from item
    uint32 srcstatamount = 0;
    for (int j = 0; j < MAX_ITEM_PROTO_STATS; j++)
    {
        if (proto->ItemStat[j].ItemStatType == pReforge->source_stat)
            srcstatamount += proto->ItemStat[j].ItemStatValue;
    }

    // for items with random enchantments (either randomproperties and randomsuffix) count also their amounts
    for (uint32 i = PROP_ENCHANTMENT_SLOT_0; i < PROP_ENCHANTMENT_SLOT_0 + MAX_ITEM_ENCHANTMENT_RANDOM_ENTRIES; ++i)
    {
        uint32 enchDisplayType = 0;
        uint32 enchAmount = 0;
        uint32 enchSpellId = 0;
        uint32 enchId = item->GetEnchantmentId(EnchantmentSlot(i));
        if (enchId)
        {
            SpellItemEnchantmentEntry const *ench = sSpellItemEnchantmentStore.LookupEntry(enchId);
            if (!ench)
                continue;

            for (int s = 0; s < MAX_ITEM_ENCHANTMENT_EFFECTS; ++s)
            {
                enchDisplayType = ench->type[s];
                enchAmount = ench->amount[s];
                enchSpellId = ench->spellid[s];

                // we fancy only STAT modifiers
                if (enchDisplayType != ITEM_ENCHANTMENT_TYPE_STAT)
                    continue;

                // if no implicit stat amount, retrieve it from suffix stuff
                if (!enchAmount)
                {
                    ItemRandomSuffixEntry const *item_rand_suffix = sItemRandomSuffixStore.LookupEntry(abs(item->GetItemRandomPropertyId()));
                    if (item_rand_suffix)
                    {
                        for (int k = 0; k < MAX_ITEM_ENCHANTMENT_RANDOM_ENTRIES; ++k)
                        {
                            if (item_rand_suffix->enchant_id[k] == enchId)
                            {
                                enchAmount = uint32((item_rand_suffix->prefix[k] * item->GetItemSuffixFactor()) / 10000);
                                break;
                            }
                        }
                    }
                }

                // add amount to total
                if (enchSpellId == pReforge->source_stat)
                    srcstatamount += enchAmount;
            }
        }
    }

    // Get stat/rating bonus
    float srcstat_mod = srcstatamount * pReforge->source_mod;
    float newstat_mod = srcstat_mod * pReforge->new_mod;

    // NOTE
    // Implemented only values present in ItemReforge.dbc
    // with new patch there is a chance for adding new entrys to that DBC
    // so they need to be implemented here!

    // Apply new stat/rating bonus
    switch (pReforge->new_stat)
    {
        case ITEM_MOD_SPIRIT:
            HandleStatModifier(UNIT_MOD_STAT_SPIRIT, TOTAL_VALUE, newstat_mod, apply);
            ApplyStatBuffMod(STAT_SPIRIT, newstat_mod, apply);
            break;
        case ITEM_MOD_DODGE_RATING:
            ApplyRatingMod(CR_DODGE, newstat_mod, apply);
            break;
        case ITEM_MOD_PARRY_RATING:
            ApplyRatingMod(CR_PARRY, newstat_mod, apply);
            break;
        case ITEM_MOD_HIT_RATING:
            ApplyRatingMod(CR_HIT_MELEE, newstat_mod, apply);
            ApplyRatingMod(CR_HIT_RANGED, newstat_mod, apply);
            ApplyRatingMod(CR_HIT_SPELL, newstat_mod, apply);
            break;
        case ITEM_MOD_CRIT_RATING:
            ApplyRatingMod(CR_CRIT_MELEE, newstat_mod, apply);
            ApplyRatingMod(CR_CRIT_RANGED, newstat_mod, apply);
            ApplyRatingMod(CR_CRIT_SPELL, newstat_mod, apply);
            break;
        case ITEM_MOD_HASTE_RATING:
            ApplyRatingMod(CR_HASTE_MELEE, newstat_mod, apply);
            ApplyRatingMod(CR_HASTE_RANGED, newstat_mod, apply);
            ApplyRatingMod(CR_HASTE_SPELL, newstat_mod, apply);
            break;
        case ITEM_MOD_EXPERTISE_RATING:
            ApplyRatingMod(CR_EXPERTISE, newstat_mod, apply);
            break;
        case ITEM_MOD_MASTERY_RATING:
            ApplyRatingMod(CR_MASTERY, newstat_mod, apply);
            break;
    }

    // And take source stat/rating bonus
    switch (pReforge->source_stat)
    {
        case ITEM_MOD_SPIRIT:
            HandleStatModifier(UNIT_MOD_STAT_SPIRIT, TOTAL_VALUE, -srcstat_mod, apply);
            ApplyStatBuffMod(STAT_SPIRIT, -srcstat_mod, apply);
            break;
        case ITEM_MOD_DODGE_RATING:
            ApplyRatingMod(CR_DODGE, -srcstat_mod, apply);
            break;
        case ITEM_MOD_PARRY_RATING:
            ApplyRatingMod(CR_PARRY, -srcstat_mod, apply);
            break;
        case ITEM_MOD_HIT_RATING:
            ApplyRatingMod(CR_HIT_MELEE, -srcstat_mod, apply);
            ApplyRatingMod(CR_HIT_RANGED, -srcstat_mod, apply);
            ApplyRatingMod(CR_HIT_SPELL, -srcstat_mod, apply);
            break;
        case ITEM_MOD_CRIT_RATING:
            ApplyRatingMod(CR_CRIT_MELEE, -srcstat_mod, apply);
            ApplyRatingMod(CR_CRIT_RANGED, -srcstat_mod, apply);
            ApplyRatingMod(CR_CRIT_SPELL, -srcstat_mod, apply);
            break;
        case ITEM_MOD_HASTE_RATING:
            ApplyRatingMod(CR_HASTE_MELEE, -srcstat_mod, apply);
            ApplyRatingMod(CR_HASTE_RANGED, -srcstat_mod, apply);
            ApplyRatingMod(CR_HASTE_SPELL, -srcstat_mod, apply);
            break;
        case ITEM_MOD_EXPERTISE_RATING:
            ApplyRatingMod(CR_EXPERTISE, -srcstat_mod, apply);
            break;
        case ITEM_MOD_MASTERY_RATING:
            ApplyRatingMod(CR_MASTERY, -srcstat_mod, apply);
            break;
    }
}

void Item::SetReforge(uint32 id)
{
    if (id == m_reforgeId)
        return;

    ItemReforgeEntry const* ref_info = sItemReforgeStore.LookupEntry(id);
    if (ref_info || id == 0)
    {
        if (id != 0)
            SetEnchantment(REFORGING_ENCHANTMENT_SLOT,id,0,0);

        if (GetOwner())
            GetOwner()->ApplyReforge(this, (id == 0)?false:true);

        if (id == 0)
            SetEnchantment(REFORGING_ENCHANTMENT_SLOT,id,0,0);

        SetState(ITEM_CHANGED, GetOwner());
        m_reforgeId = id;
    }
    else
        sLog->outError("Wrong reforge ID %u", id);
}

uint32 Item::GetReforge()
{
    return m_reforgeId;
}

void Item::ClearReforge()
{
    //TODO: send it to client
    m_reforgeId = 0;
}

bool Item::GemsFitSockets() const
{
    bool fits = true;
    for (uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT+MAX_GEM_SOCKETS; ++enchant_slot)
    {
        uint8 SocketColor = GetProto()->Socket[enchant_slot-SOCK_ENCHANTMENT_SLOT].Color;

        uint32 enchant_id = GetEnchantmentId(EnchantmentSlot(enchant_slot));
        if (!enchant_id)
        {
            if (SocketColor) fits &= false;
            continue;
        }

        SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if (!enchantEntry)
        {
            if (SocketColor) fits &= false;
            continue;
        }

        uint8 GemColor = 0;

        uint32 gemid = enchantEntry->GemID;
        if (gemid)
        {
            ItemPrototype const* gemProto = sItemStorage.LookupEntry<ItemPrototype>(gemid);
            if (gemProto)
            {
                GemPropertiesEntry const* gemProperty = sGemPropertiesStore.LookupEntry(gemProto->GemProperties);
                if (gemProperty)
                    GemColor = gemProperty->color;
            }
        }

        fits &= (GemColor & SocketColor) ? true : false;
    }
    return fits;
}

uint8 Item::GetGemCountWithID(uint32 GemID) const
{
    uint8 count = 0;
    for (uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT+MAX_GEM_SOCKETS; ++enchant_slot)
    {
        uint32 enchant_id = GetEnchantmentId(EnchantmentSlot(enchant_slot));
        if (!enchant_id)
            continue;

        SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if (!enchantEntry)
            continue;

        if (GemID == enchantEntry->GemID)
            ++count;
    }
    return count;
}

uint8 Item::GetGemCountWithLimitCategory(uint32 limitCategory) const
{
    uint8 count = 0;
    for (uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT+MAX_GEM_SOCKETS; ++enchant_slot)
    {
        uint32 enchant_id = GetEnchantmentId(EnchantmentSlot(enchant_slot));
        if (!enchant_id)
            continue;

        SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if (!enchantEntry)
            continue;

        ItemPrototype const* gemProto = ObjectMgr::GetItemPrototype(enchantEntry->GemID);
        if (!gemProto)
            continue;

        if (gemProto->ItemLimitCategory == limitCategory)
            ++count;
    }
    return count;
}

bool Item::IsLimitedToAnotherMapOrZone(uint32 cur_mapId, uint32 cur_zoneId) const
{
    ItemPrototype const* proto = GetProto();
    return proto && ((proto->Map && proto->Map != cur_mapId) || (proto->Area && proto->Area != cur_zoneId));
}

// Though the client has the information in the item's data field,
// we have to send SMSG_ITEM_TIME_UPDATE to display the remaining
// time.
void Item::SendTimeUpdate(Player* owner)
{
    uint32 duration = GetUInt32Value(ITEM_FIELD_DURATION);
    if (!duration)
        return;

    WorldPacket data(SMSG_ITEM_TIME_UPDATE, (8+4));
    data << uint64(GetGUID());
    data << uint32(duration);
    owner->GetSession()->SendPacket(&data);
}

Item* Item::CreateItem(uint32 item, uint32 count, Player const* player)
{
    if (count < 1)
        return NULL;                                        //don't create item at zero count

    ItemPrototype const *pProto = sObjectMgr->GetItemPrototype(item);
    if (pProto)
    {
        if (count > pProto->GetMaxStackSize())
            count = pProto->GetMaxStackSize();

        ASSERT(count !=0 && "pProto->Stackable == 0 but checked at loading already");

        Item *pItem = NewItemOrBag(pProto);
        if (pItem->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_ITEM), item, player))
        {
            pItem->SetCount(count);
            return pItem;
        }
        else
            delete pItem;
    }
    else
        ASSERT(false);
    return NULL;
}

Item* Item::CloneItem(uint32 count, Player const* player) const
{
    Item* newItem = CreateItem(GetEntry(), count, player);
    if (!newItem)
        return NULL;

    newItem->SetUInt32Value(ITEM_FIELD_CREATOR,      GetUInt32Value(ITEM_FIELD_CREATOR));
    newItem->SetUInt32Value(ITEM_FIELD_GIFTCREATOR,  GetUInt32Value(ITEM_FIELD_GIFTCREATOR));
    newItem->SetUInt32Value(ITEM_FIELD_FLAGS,        GetUInt32Value(ITEM_FIELD_FLAGS));
    newItem->SetUInt32Value(ITEM_FIELD_DURATION,     GetUInt32Value(ITEM_FIELD_DURATION));
    // player CAN be NULL in which case we must not update random properties because that accesses player's item update queue
    if (player)
        newItem->SetItemRandomProperties(GetItemRandomPropertyId());
    return newItem;
}

bool Item::IsBindedNotWith(Player const* player) const
{
    // not binded item
    if (!IsSoulBound())
        return false;

    // own item
    if (GetOwnerGUID() == player->GetGUID())
        return false;

    if (HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_BOP_TRADEABLE))
        if (allowedGUIDs.find(player->GetGUIDLow()) != allowedGUIDs.end())
            return false;

    // BOA item case
    if (IsBoundAccountWide())
        return false;

    return true;
}

bool ItemRequiredTarget::IsFitToRequirements(Unit* pUnitTarget) const
{
    if (pUnitTarget->GetTypeId() != TYPEID_UNIT)
        return false;

    if (pUnitTarget->GetEntry() != m_uiTargetEntry)
        return false;

    switch(m_uiType)
    {
        case ITEM_TARGET_TYPE_CREATURE:
            return pUnitTarget->IsAlive();
        case ITEM_TARGET_TYPE_DEAD:
            return !pUnitTarget->IsAlive();
        default:
            return false;
    }
}

void Item::BuildUpdate(UpdateDataMapType& data_map)
{
    if (Player *owner = GetOwner())
        BuildFieldsUpdate(owner, data_map);
    ClearUpdateMask(false);
}

void Item::SaveRefundDataToDB()
{
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    trans->PAppend("DELETE FROM item_refund_instance WHERE item_guid = '%u'", GetGUIDLow());
    trans->PAppend("INSERT INTO item_refund_instance (`item_guid`,`player_guid`,`paidMoney`,`paidExtendedCost`)"
    " VALUES('%u','%u','" UI64FMTD "','%u')", GetGUIDLow(), GetRefundRecipient(), GetPaidMoney(), GetPaidExtendedCost());
    CharacterDatabase.CommitTransaction(trans);
}

void Item::DeleteRefundDataFromDB()
{
    CharacterDatabase.PExecute("DELETE FROM item_refund_instance WHERE item_guid = '%u'", GetGUIDLow());
}

void Item::SetNotRefundable(Player *owner, bool changestate)
{
    if (!HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_REFUNDABLE))
        return;

    RemoveFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_REFUNDABLE);
    // Following is not applicable in the trading procedure
    if (changestate)
        SetState(ITEM_CHANGED, owner);

    SetRefundRecipient(0);
    SetPaidMoney(0);
    SetPaidExtendedCost(0);
    DeleteRefundDataFromDB();

    owner->DeleteRefundReference(GetGUIDLow());
}

void Item::UpdatePlayedTime(Player *owner)
{
    // Calculate time elapsed since last played time update
    time_t curtime = time(NULL);
    uint32 elapsed = uint32(curtime - m_lastPlayedTimeUpdate);
    m_playedTime += elapsed;
    m_lastPlayedTimeUpdate = curtime;

    if (IsRefundExpired())
        SetNotRefundable(owner);
}

uint32 Item::GetPlayedTime() const
{
    time_t curtime = time(NULL);
    uint32 elapsed = uint32(curtime - m_lastPlayedTimeUpdate);
    return m_playedTime + elapsed;
}

uint32 Item::GetPlayedTimeOfCreation() const
{
    return GetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME);
}

bool Item::IsRefundExpired() const
{
    return GetPlayedTime() > 2*HOUR;
}

void Item::SetSoulboundTradeable(AllowedLooterSet* allowedLooters, Player* currentOwner, bool apply)
{
    if (apply)
    {
        SetFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_BOP_TRADEABLE);
        allowedGUIDs = *allowedLooters;
    }
    else
    {
        RemoveFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_BOP_TRADEABLE);
        if (allowedGUIDs.empty())
            return;

        allowedGUIDs.clear();
        SetState(ITEM_CHANGED, currentOwner);
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_BOP_TRADE);
        stmt->setUInt32(0, GetGUIDLow());
        CharacterDatabase.Execute(stmt);
    }
}

void Item::ClearSoulboundTradeable(Player* currentOwner)
{
    RemoveFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_BOP_TRADEABLE);
    if (allowedGUIDs.empty())
        return;

    allowedGUIDs.clear();
    SetState(ITEM_CHANGED, currentOwner);
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_BOP_TRADE);
    stmt->setUInt32(0, GetGUIDLow());
    CharacterDatabase.Execute(stmt);
}

bool Item::CheckSoulboundTradeExpire()
{
    // called from owner's update - GetOwner() MUST be valid
    if (GetUInt32Value(ITEM_FIELD_CREATE_PLAYED_TIME) + 2*HOUR < GetOwner()->GetTotalPlayedTime())
    {
        SetSoulboundTradeable(NULL, GetOwner(), false);
        return true; // remove from tradeable list
    }

    return false;
}

bool Item::HasStats() const
{
    if (GetItemRandomPropertyId() != 0)
        return true;

    ItemPrototype const* pProto = GetProto();
    for (uint8 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
        if (pProto->ItemStat[i].ItemStatValue != 0)
            return true;

    return false;
}

bool Item::CanBeTransmogrified() const
{
    ItemPrototype const* pProto = GetProto();

    if (!pProto)
        return false;

    if (pProto->Flags2 & ITEM_FLAGS_EXTRA_CANNOT_BE_TRANSMOG)
        return false;

    if (pProto->Flags2 & ITEM_FLAGS_EXTRA_CAN_TRANSMOG)
        return true;

    if (pProto->Quality == ITEM_QUALITY_LEGENDARY)
        return false;

    if (pProto->Class != ITEM_CLASS_ARMOR &&
        pProto->Class != ITEM_CLASS_WEAPON)
        return false;

    if (pProto->Class == ITEM_CLASS_WEAPON && pProto->SubClass == ITEM_SUBCLASS_WEAPON_FISHING_POLE)
        return false;

    if (!HasStats())
        return false;

    return true;
}

bool Item::CanTransmogrify() const
{
    ItemPrototype const* pProto = GetProto();

    if (!pProto)
        return false;

    if (pProto->Flags2 & ITEM_FLAGS_EXTRA_CANNOT_TRANSMOG)
        return false;

    if (pProto->Flags2 & ITEM_FLAGS_EXTRA_CAN_TRANSMOG)
        return true;

    if (pProto->Quality == ITEM_QUALITY_LEGENDARY)
        return false;

    if (pProto->Class != ITEM_CLASS_ARMOR &&
        pProto->Class != ITEM_CLASS_WEAPON)
        return false;

    if (pProto->Class == ITEM_CLASS_WEAPON && pProto->SubClass == ITEM_SUBCLASS_WEAPON_FISHING_POLE)
        return false;

    if (!HasStats())
        return false;

    return true;
}
bool Item::CanTransmogrifyItemWithItem(Item const* transmogrified, Item const* transmogrifier)
{
    if (!transmogrifier || !transmogrified)
        return false;

    ItemPrototype const* proto1 = transmogrifier->GetProto(); // source
    ItemPrototype const* proto2 = transmogrified->GetProto(); // dest

    if (proto1->ItemId == proto2->ItemId)
        return false;

    if (!transmogrified->CanTransmogrify() || !transmogrifier->CanBeTransmogrified())
        return false;

    // special case exceptions: for bows, crossbows, guns <- -> gun, bow and crossbow can be transmogrified on gun, bow and crossbow.
    if (proto1->Class == ITEM_CLASS_WEAPON && proto2->Class == ITEM_CLASS_WEAPON
       && (proto1->SubClass == ITEM_SUBCLASS_WEAPON_GUN || proto1->SubClass == ITEM_SUBCLASS_WEAPON_BOW
       || proto1->SubClass == ITEM_SUBCLASS_WEAPON_CROSSBOW)
       && (proto2->SubClass == ITEM_SUBCLASS_WEAPON_BOW || proto2->SubClass == ITEM_SUBCLASS_WEAPON_CROSSBOW
       || proto2->SubClass == ITEM_SUBCLASS_WEAPON_GUN))
       return true;

    if (proto1->InventoryType == INVTYPE_BAG ||
        proto1->InventoryType == INVTYPE_RELIC ||
        proto1->InventoryType == INVTYPE_BODY ||
        proto1->InventoryType == INVTYPE_FINGER ||
        proto1->InventoryType == INVTYPE_TRINKET ||
        proto1->InventoryType == INVTYPE_AMMO ||
        proto1->InventoryType == INVTYPE_QUIVER)
        return false;

    if (proto1->SubClass != proto2->SubClass && (proto1->Class != ITEM_CLASS_WEAPON || !proto2->IsRangedWeapon() || !proto1->IsRangedWeapon()))
        return false;

    if (proto1->InventoryType != proto2->InventoryType
        && (((proto1->Class == INVTYPE_WEAPONMAINHAND || proto1->Class == INVTYPE_WEAPONOFFHAND) && (proto2->InventoryType != INVTYPE_WEAPON)) 
        || (((proto1->InventoryType == INVTYPE_CHEST && proto2->InventoryType != INVTYPE_ROBE) || (proto1->InventoryType == INVTYPE_ROBE && proto2->InventoryType != INVTYPE_CHEST)))))
        return false;

    return true;
}

// used by mail items, transmog cost, stationeryinfo, etc..
uint64 Item::GetSellPrice(ItemPrototype const* pProto, bool& normalSellPrice)
{
    normalSellPrice = true;

    if (pProto->Flags2 & ITEM_FLAGS_EXTRA_HAS_NORMAL_PRICE)
        return pProto->BuyPrice;
    else
    {
        ImportPriceQualityEntry const* qualityPrice = sImportPriceQualityStore.LookupEntry(pProto->Quality + 1);
        ItemPriceBaseEntry const* basePrice = sItemPriceBaseStore.LookupEntry(pProto->ItemLevel);

        if (!qualityPrice || !basePrice)
            return 0;

        float qualityFactor = qualityPrice->Factor;
        float baseFactor = 0.0f;

        uint32 inventoryType = pProto->InventoryType;

        if (inventoryType == INVTYPE_WEAPON ||
            inventoryType == INVTYPE_2HWEAPON ||
            inventoryType == INVTYPE_WEAPONMAINHAND ||
            inventoryType == INVTYPE_WEAPONOFFHAND ||
            inventoryType == INVTYPE_RANGED ||
            inventoryType == INVTYPE_THROWN ||
            inventoryType == INVTYPE_RANGEDRIGHT)
            baseFactor = basePrice->WeaponFactor;
        else
            baseFactor = basePrice->ArmorFactor;

        if (inventoryType == INVTYPE_ROBE)
            inventoryType = INVTYPE_CHEST;

        float typeFactor = 0.0f;
        uint8 wepType = -1;

        switch (inventoryType)
        {
            case INVTYPE_HEAD:
            case INVTYPE_SHOULDERS:
            case INVTYPE_CHEST:
            case INVTYPE_WAIST:
            case INVTYPE_LEGS:
            case INVTYPE_FEET:
            case INVTYPE_WRISTS:
            case INVTYPE_HANDS:
            case INVTYPE_CLOAK:
            {
                ImportPriceArmorEntry const* armorPrice = sImportPriceArmorStore.LookupEntry(inventoryType);
                if (!armorPrice)
                    return 0;

                switch (pProto->SubClass)
                {
                    case ITEM_SUBCLASS_ARMOR_MISC:
                    case ITEM_SUBCLASS_ARMOR_CLOTH:
                    {
                        typeFactor = armorPrice->ClothFactor;
                        break;
                    }
                    case ITEM_SUBCLASS_ARMOR_LEATHER:
                    {
                        typeFactor = armorPrice->ClothFactor;
                        break;
                    }
                    case ITEM_SUBCLASS_ARMOR_MAIL:
                    {
                        typeFactor = armorPrice->ClothFactor;
                        break;
                    }
                    case ITEM_SUBCLASS_ARMOR_PLATE:
                    {
                        typeFactor = armorPrice->ClothFactor;
                        break;
                    }
                    default:
                    {
                        return 0;
                    }
                }

                break;
            }
            case INVTYPE_SHIELD:
            {
                ImportPriceShieldEntry const* shieldPrice = sImportPriceShieldStore.LookupEntry(1); // it only has two rows, it's unclear which is the one used
                if (!shieldPrice)
                    return 0;

                typeFactor = shieldPrice->Factor;
                break;
            }
            case INVTYPE_WEAPONMAINHAND:
                wepType = 0;             // unk enum, fall back
            case INVTYPE_WEAPONOFFHAND:
                wepType = 1;             // unk enum, fall back
            case INVTYPE_WEAPON:
                wepType = 2;             // unk enum, fall back
            case INVTYPE_2HWEAPON:
                wepType = 3;             // unk enum, fall back
            case INVTYPE_RANGED:
            case INVTYPE_RANGEDRIGHT:
            case INVTYPE_RELIC:
            {
                wepType = 4;             // unk enum

                ImportPriceWeaponEntry const* weaponPrice = sImportPriceWeaponStore.LookupEntry(wepType + 1);
                if (!weaponPrice)
                    return 0;

                typeFactor = weaponPrice->Factor;
                break;
            }
            default:
                return pProto->BuyPrice;
        }

        normalSellPrice = false;
        return (uint64)(qualityFactor * typeFactor * baseFactor);
    }
}

uint64 Item::GetTransmogrifyCost() const
{
    ItemPrototype const* pProto = GetProto();
    uint64 cost = 0;

    if (pProto->Flags2 & ITEM_FLAGS_EXTRA_HAS_NORMAL_PRICE)
        cost = pProto->SellPrice;
    else
    {
        bool normalPrice;
        cost = GetSellPrice(pProto,normalPrice);
        
        if (!normalPrice)
        {
            if (pProto->BuyCount <= 1)
            {
                ItemClassEntry const* classEntry = sItemClassStore.LookupEntry(pProto->Class);
                if (classEntry)
                    cost *= classEntry->PriceFactor;
                else
                    cost = 0;
            }
            else
                cost /= 4 * pProto->BuyCount;
        }
        else
            cost = pProto->SellPrice;
    }
    
    if (cost < 10000)
        cost = 10000;
    
    return cost;
}
