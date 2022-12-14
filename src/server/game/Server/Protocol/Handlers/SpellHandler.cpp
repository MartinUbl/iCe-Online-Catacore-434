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
#include "DBCStores.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Log.h"
#include "Opcodes.h"
#include "Spell.h"
#include "Totem.h"
#include "TemporarySummon.h"
#include "SpellAuras.h"
#include "CreatureAI.h"
#include "ScriptMgr.h"
#include "GameObjectAI.h"
#include "SpellAuraEffects.h"

void WorldSession::HandleClientCastFlags(WorldPacket& recvPacket, uint8 castFlags, SpellCastTargets & targets)
{
    // some spell cast packet including more data (for projectiles?)
    if (castFlags & 0x02)
    {
        // not sure about these two
        recvPacket >> targets.m_elevation;
        recvPacket >> targets.m_speed;
        uint8 hasMovementData;
        recvPacket >> hasMovementData;
        if (hasMovementData)
        {
            recvPacket.rfinish();
            // movement packet for caster of the spell
            /*recvPacket.read_skip<uint32>(); // MSG_MOVE_STOP - hardcoded in client
            uint64 guid;
            recvPacket.readPackGUID(guid);

            MovementInfo movementInfo;
            movementInfo.guid = guid;
            ReadMovementInfo(recvPacket, &movementInfo);*/
        }
    }
}

void WorldSession::HandleUseItemOpcode(WorldPacket& recvPacket)
{
    // TODO: add targets.read() check
    Player* pUser = _player;

    // ignore for remote control state
    if (pUser->m_mover != pUser)
        return;

    if(pUser->GetEmoteState())
        pUser->SetEmoteState(0);

    uint8 bagIndex, slot, castFlags;
    uint8 castCount;                                       // next cast if exists (single or not)
    uint64 itemGUID;
    uint32 unk;                                     
    uint32 spellId;                                         // casted spell id

    recvPacket >> bagIndex >> slot >> castCount >> spellId >> itemGUID >> unk >> castFlags;

    Item *pItem = pUser->GetUseableItemByPos(bagIndex, slot);
    if (!pItem)
    {
        pUser->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, NULL, NULL);
        return;
    }

    if (pItem->GetGUID() != itemGUID)
    {
        pUser->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, NULL, NULL);
        return;
    }

    sLog->outDetail("WORLD: CMSG_USE_ITEM packet, bagIndex: %u, slot: %u, castCount: %u, spellId: %u, Item: %u, data length = %i", bagIndex, slot, castCount, spellId, pItem->GetEntry(), (uint32)recvPacket.size());

    ItemPrototype const *proto = pItem->GetProto();
    if (!proto)
    {
        pUser->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, pItem, NULL);
        return;
    }

    // some item classes can be used only in equipped state
    if (proto->InventoryType != INVTYPE_NON_EQUIP && !pItem->IsEquipped())
    {
        pUser->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, pItem, NULL);
        return;
    }

    uint8 msg = pUser->CanUseItem(pItem);
    if (msg != EQUIP_ERR_OK)
    {
        pUser->SendEquipError(msg, pItem, NULL);
        return;
    }

    // only allow conjured consumable, bandage, poisons (all should have the 2^21 item flag set in DB)
    if (proto->Class == ITEM_CLASS_CONSUMABLE && !(proto->Flags & ITEM_PROTO_FLAG_USEABLE_IN_ARENA) && pUser->InArena())
    {
        pUser->SendEquipError(EQUIP_ERR_NOT_DURING_ARENA_MATCH, pItem, NULL);
        return;
    }

    // don't allow items banned in arena
    if (proto->Flags & ITEM_PROTO_FLAG_NOT_USEABLE_IN_ARENA && pUser->InArena())
    {
        pUser->SendEquipError(EQUIP_ERR_NOT_DURING_ARENA_MATCH, pItem, NULL);
        return;
    }

    if (pUser->IsInCombat())
    {
        for (int i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
        {
            if (SpellEntry const *spellInfo = sSpellStore.LookupEntry(proto->Spells[i].SpellId))
            {
                if (IsNonCombatSpell(spellInfo))
                {
                    pUser->SendEquipError(EQUIP_ERR_NOT_IN_COMBAT,pItem,NULL);
                    return;
                }
            }
        }
    }

    // check also  BIND_WHEN_PICKED_UP and BIND_QUEST_ITEM for .additem or .additemset case by GM (not binded at adding to inventory)
    if (pItem->GetProto()->Bonding == BIND_WHEN_USE || pItem->GetProto()->Bonding == BIND_WHEN_PICKED_UP || pItem->GetProto()->Bonding == BIND_QUEST_ITEM)
    {
        if (!pItem->IsSoulBound())
        {
            pItem->SetState(ITEM_CHANGED, pUser);
            pItem->SetBinding(true);
        }
    }

    SpellCastTargets targets;
    targets.read(recvPacket, pUser);
    HandleClientCastFlags(recvPacket, castFlags, targets);

    if (!pItem->IsTargetValidForItemUse(targets.getUnitTarget()))
    {
        // free gray item after use fail
        pUser->SendEquipError(EQUIP_ERR_NONE, pItem, NULL);

        // send spell error
        if (SpellEntry const* spellInfo = sSpellStore.LookupEntry(spellId))
        {
            // for implicit area/coord target spells
            if (!targets.getUnitTarget())
                Spell::SendCastResult(_player,spellInfo,castCount,SPELL_FAILED_NO_VALID_TARGETS);
            // for explicit target spells
            else
                Spell::SendCastResult(_player,spellInfo,castCount,SPELL_FAILED_BAD_TARGETS);
        }
        return;
    }

    // Note: If script stop casting it must send appropriate data to client to prevent stuck item in gray state.
    if (!sScriptMgr->OnItemUse(pUser,pItem,targets))
    {
        // no script or script not process request by self
        pUser->CastItemUseSpell(pItem,targets,castCount);
    }
}

#define OPEN_CHEST 11437
#define OPEN_SAFE 11535
#define OPEN_CAGE 11792
#define OPEN_BOOTY_CHEST 5107
#define OPEN_STRONGBOX 8517

void WorldSession::HandleOpenItemOpcode(WorldPacket& recvPacket)
{
    sLog->outDetail("WORLD: CMSG_OPEN_ITEM packet, data length = %i",(uint32)recvPacket.size());

    Player* pUser = _player;

    // ignore for remote control state
    if (pUser->m_mover != pUser)
        return;

    uint8 bagIndex, slot;

    recvPacket >> bagIndex >> slot;

    sLog->outDetail("bagIndex: %u, slot: %u",bagIndex,slot);

    Item *pItem = pUser->GetItemByPos(bagIndex, slot);
    if (!pItem)
    {
        pUser->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, NULL, NULL);
        return;
    }

    ItemPrototype const *proto = pItem->GetProto();
    if (!proto)
    {
        pUser->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, pItem, NULL);
        return;
    }

    // Verify that the bag is an actual bag or wrapped item that can be used "normally"
    if(!(proto->Flags & ITEM_PROTO_FLAG_OPENABLE) && !pItem->HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_WRAPPED))
    {
        pUser->SendEquipError(EQUIP_ERR_CANT_DO_RIGHT_NOW, pItem, NULL);
        sLog->outError("Possible hacking attempt: Player %s [guid: %u] tried to open item [guid: %u, entry: %u] which is not openable!", 
                pUser->GetName(), pUser->GetGUIDLow(), pItem->GetGUIDLow(), proto->ItemId);
        return;
    }

    // locked item
    uint32 lockId = proto->LockID;
    if (lockId)
    {
        LockEntry const *lockInfo = sLockStore.LookupEntry(lockId);

        if (!lockInfo)
        {
            pUser->SendEquipError(EQUIP_ERR_ITEM_LOCKED, pItem, NULL);
            sLog->outError("WORLD::OpenItem: item [guid = %u] has an unknown lockId: %u!", pItem->GetGUIDLow(), lockId);
            return;
        }

        // was not unlocked yet
        if (pItem->IsLocked())
        {
            pUser->SendEquipError(EQUIP_ERR_ITEM_LOCKED, pItem, NULL);
            return;
        }
    }

    if (pItem->HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_WRAPPED))// wrapped?
    {
        QueryResult result = CharacterDatabase.PQuery("SELECT entry, flags FROM character_gifts WHERE item_guid = '%u'", pItem->GetGUIDLow());
        if (result)
        {
            Field *fields = result->Fetch();
            uint32 entry = fields[0].GetUInt32();
            uint32 flags = fields[1].GetUInt32();

            pItem->SetUInt64Value(ITEM_FIELD_GIFTCREATOR, 0);
            pItem->SetEntry(entry);
            pItem->SetUInt32Value(ITEM_FIELD_FLAGS, flags);
            pItem->SetState(ITEM_CHANGED, pUser);
        }
        else
        {
            sLog->outError("Wrapped item %u don't have record in character_gifts table and will deleted", pItem->GetGUIDLow());
            pUser->DestroyItem(pItem->GetBagSlot(), pItem->GetSlot(), true);
            return;
        }
        CharacterDatabase.PExecute("DELETE FROM character_gifts WHERE item_guid = '%u'", pItem->GetGUIDLow());
    }
    else
        pUser->SendLoot(pItem->GetGUID(),LOOT_CORPSE);
}

void WorldSession::HandleGameObjectUseOpcode(WorldPacket & recv_data)
{
    uint64 guid;

    recv_data >> guid;

    sLog->outDebug("WORLD: Recvd CMSG_GAMEOBJ_USE Message [guid=%u]", GUID_LOPART(guid));

    // ignore for remote control state
    if (_player->m_mover != _player)
        return;

    GameObject *obj = GetPlayer()->GetMap()->GetGameObject(guid);

    if (!obj)
        return;

    if (sScriptMgr->OnGossipHello(_player, obj))
        return;

    obj->AI()->GossipHello(_player);

    obj->Use(_player);
}

void WorldSession::HandleGameobjectReportUse(WorldPacket& recvPacket)
{
    uint64 guid;
    recvPacket >> guid;

    sLog->outDebug("WORLD: Recvd CMSG_GAMEOBJ_REPORT_USE Message [in game guid: %u]", GUID_LOPART(guid));

    // ignore for remote control state
    if (_player->m_mover != _player)
        return;

    GameObject* go = GetPlayer()->GetMap()->GetGameObject(guid);
    if (!go)
        return;

    if (!go->IsWithinDistInMap(_player,INTERACTION_DISTANCE))
        return;

    //if (go->GetGOInfo()->type == GAMEOBJECT_TYPE_MAILBOX)


    _player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT, go->GetEntry());
}

const uint32 ModABSpellMap[][3] = {
//   OriginalSpellID,  ModifyABAura,  SpellEffectIndex
    {11366,            48108,         0}, // Hot Streak
    {689,              74434,         1}, // Soulburn
    {6785,             81021,         0}, // Stampede (Cat Form) rank 1
    {6785,             81022,         0}, // Stampede (Cat Form) rank 2
    {1499,             77769,         0}, // Trap Launcher
    {13813,            77769,         1},
    {13809,            77769,         2},
    {13795,            82946,         0}, // Trap Launcher (second?)
    {34600,            82946,         1},
    {19434,            82926,         1}, // Fire! (Master Marksman proc)
    {6229,             687,           2}, // Demon Armor - condition for talent Nether Ward
    {6229,             28176,         2}, // Fel Armor
    {8921,             94338,         0}, // Eclipse (Solar) - condition for talent Sunfire
    {86121,            86211,         0}, // Soul Swap
    {77606,            77616,         0}, // Dark Simulacrum
    {88625,            81206,         2}, // Chakra: Sanctuary
    {88625,            81208,         2}, // Chakra: Sanctuary
    {2061,             88688,         0}, // Flash Light (talent Surge of Light)
    {82731,            84726,         1}, // Frostfire Orb rank 1
    {82731,            84727,         1}, // Frostfire Orb rank 1
};

void WorldSession::HandleCastSpellOpcode(WorldPacket& recvPacket)
{
    uint32 spellId, glyphIndex;
    uint8  castCount, castFlags;
    recvPacket >> castCount >> spellId >> glyphIndex >> castFlags;

    // Player don't know modified AB spell, so we must set the handler to ignore it
    // Also we can make some spells as triggered (i.e. Dark Simulacrum does it)
    bool IgnoreDontKnowSpell = false;
    bool SetTriggered = false;
    // Modify action button if aura is present
    // Client does NOT modify spell IDs, only spell icons
    for (uint16 i = 0; i < sizeof(ModABSpellMap)/(sizeof(uint32)*3); i++)
    {
        if (spellId == ModABSpellMap[i][0] && _player->HasAura(ModABSpellMap[i][1]))
        {
            if (_player->GetAura(ModABSpellMap[i][1])->IsModActionButton())
            {
                if (uint32 id = _player->GetAura(ModABSpellMap[i][1])->GetActionButtonSpellForEffect(ModABSpellMap[i][2]))
                {
                    spellId = id;
                    IgnoreDontKnowSpell = true;

                    // Dark Simulacrum makes spell cast as triggered
                    if (ModABSpellMap[i][1] == 77616)
                    {
                        SetTriggered = true;
                        _player->RemoveAurasDueToSpell(77616);
                    }
                }
            }
        }
        if (IgnoreDontKnowSpell)
            break;
    }

    sLog->outDebug("WORLD: got cast spell packet, castCount: %u, spellId: %u, glyphIndex: %u, castFlags: %u, data length = %u", castCount, spellId, glyphIndex, castFlags, (uint32)recvPacket.size());

    // ignore for remote control state (for player case)
    Unit* mover = _player->m_mover;

    if (!mover || !mover->IsInWorld())
    {
        recvPacket.rfinish(); // prevent spam at ignore packet
        return;
    }

    if (mover != _player && mover->GetTypeId() == TYPEID_PLAYER)
    {
        recvPacket.rfinish(); // prevent spam at ignore packet
        return;
    }

    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);

    if (!spellInfo)
    {
        sLog->outError("WORLD: unknown spell id %u", spellId);
        recvPacket.rfinish(); // prevent spam at ignore packet
        return;
    }

    if (mover->GetTypeId() == TYPEID_PLAYER)
    {
        if (mover->ToPlayer()->GetEmoteState())
            mover->ToPlayer()->SetEmoteState(0);

        // if the spell is in general knowledge of all players, ignore individual knowledge
        if (sObjectMgr->IsSpellKnownByAll(spellId))
            IgnoreDontKnowSpell = true;

        // allow cast even if the player don't know the spell
        if (spellInfo->AttributesEx8 & SPELL_ATTR8_RAID_MARKER)
            IgnoreDontKnowSpell = true;

        // Special aura (only one spell so far)
        if (mover->HasAuraType(SPELL_AURA_MOD_NEXT_SPELL))
        {
            mover->RemoveAurasByType(SPELL_AURA_MOD_NEXT_SPELL);
            mover->RemoveAura(107837); // Remove also Throw Totem aura (Echo of Baine encounter)
            IgnoreDontKnowSpell = true;
        }
        else if (spellInfo->HasSpellEffect(SPELL_EFFECT_TALENT_SPEC_SELECT)) // Switch talent branch spec, sanity checks are implemented elsewhere
            IgnoreDontKnowSpell = true;

        // not have spell in spellbook or spell passive and not casted by client
        if ((!mover->ToPlayer()->HasActiveSpell (spellId) || IsPassiveSpell(spellId)) && !IgnoreDontKnowSpell)
        {
            //cheater? kick? ban?
            recvPacket.rfinish(); // prevent spam at ignore packet
            return;
        }
    }
    else
    {
        // not have spell in spellbook or spell passive and not casted by client
        if ((mover->GetTypeId() == TYPEID_UNIT && !mover->ToCreature()->HasSpell(spellId)) || IsPassiveSpell(spellId))
        {
            //cheater? kick? ban?
            recvPacket.rfinish(); // prevent spam at ignore packet
            return;
        }
    }

    // Client is resending autoshot cast opcode when other spell is casted during shoot rotation
    // Skip it to prevent "interrupt" message
    if (IsAutoRepeatRangedSpell(spellInfo) && _player->GetCurrentSpell(CURRENT_AUTOREPEAT_SPELL)
        && _player->GetCurrentSpell(CURRENT_AUTOREPEAT_SPELL)->m_spellInfo == spellInfo)
    {
        recvPacket.rfinish();
        return;
    }

    // can't use our own spells when we're in possession of another unit,
    if (_player->IsPossessing())
    {
        recvPacket.rfinish(); // prevent spam at ignore packet
        return;
    }

    // client provided targets
    SpellCastTargets targets;
    targets.read(recvPacket, mover);
    HandleClientCastFlags(recvPacket, castFlags, targets);

    // Archaeology spellcasts defines also reagents used to solve archeology project
    uint32 keyStonesCount = 0;
    if (spellInfo->researchProjectId > 0)
    {
        uint32 reagentCount, reagentId, reagentTypeCount;
        uint8 reagentType;

        // Client sends all data from cast, we take only count of keystones he put into interface
        // reagentType 1 = currency, reagentType 2 = item
        recvPacket >> reagentCount;
        for (uint8 i = 0; i < reagentCount; i++)
        {
            recvPacket >> reagentType >> reagentId >> reagentTypeCount;
            if (reagentType == 2)
                keyStonesCount = reagentTypeCount;
        }
    }

    // auto-selection buff level base at target level (in spellInfo)
    if (targets.getUnitTarget())
    {
        SpellEntry const *actualSpellInfo = sSpellMgr->SelectAuraRankForPlayerLevel(spellInfo,targets.getUnitTarget()->getLevel());

        // if rank not found then function return NULL but in explicit cast case original spell can be casted and later failed with appropriate error message
        if (actualSpellInfo)
            spellInfo = actualSpellInfo;
    }

    Spell *spell = new Spell(mover, spellInfo, SetTriggered);
    spell->m_cast_count = castCount;                       // set count of casts
    spell->m_glyphIndex = glyphIndex;
    spell->m_keyStonesCount = keyStonesCount;
    spell->prepare(&targets);
}

void WorldSession::HandleCancelCastOpcode(WorldPacket& recvPacket)
{
    uint32 spellId;

    recvPacket.read_skip<uint8>();                          // counter, increments with every CANCEL packet, don't use for now
    recvPacket >> spellId;

    if (_player->IsNonMeleeSpellCasted(false))
        _player->InterruptNonMeleeSpells(false,spellId,false);

    // cancel queued spell, too
    _player->CancelQueuedSpell();
}

void WorldSession::HandleCancelAuraOpcode(WorldPacket& recvPacket)
{
    uint32 spellId;
    recvPacket >> spellId;

    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo)
        return;

    // not allow remove non positive spells and spells with attr SPELL_ATTR0_CANT_CANCEL
    if (spellInfo->Attributes & SPELL_ATTR0_CANT_CANCEL)
        return;

    if (!IsPositiveSpell(spellId) && !spellInfo->AppliesAuraType(SPELL_AURA_MOD_POSSESS))
        return;

    // don't allow cancelling passive auras (some of them are visible)
    if (IsPassiveSpell(spellInfo))
        return;

    // channeled spell case (it currently casted then)
    if (IsChanneledSpell(spellInfo))
    {
        if (Spell* curSpell = _player->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
            if (curSpell->m_spellInfo->Id == spellId)
                _player->InterruptSpell(CURRENT_CHANNELED_SPELL);
        return;
    }

    // non channeled case
    // maybe should only remove one buff when there are multiple?
    _player->RemoveOwnedAura(spellId, 0, 0, AURA_REMOVE_BY_CANCEL);
}

void WorldSession::HandlePetCancelAuraOpcode(WorldPacket& recvPacket)
{
    uint64 guid;
    uint32 spellId;

    recvPacket >> guid;
    recvPacket >> spellId;

    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo)
    {
        sLog->outError("WORLD: unknown PET spell id %u", spellId);
        return;
    }

    Creature* pet = ObjectAccessor::GetCreatureOrPetOrVehicle(*_player,guid);

    if (!pet)
    {
        sLog->outError("Pet %u not exist.", uint32(GUID_LOPART(guid)));
        return;
    }

    if (pet != GetPlayer()->GetGuardianPet() && pet != GetPlayer()->GetCharm())
    {
        sLog->outError("HandlePetCancelAura.Pet %u isn't pet of player %s", uint32(GUID_LOPART(guid)),GetPlayer()->GetName());
        return;
    }

    if (!pet->IsAlive())
    {
        pet->SendPetActionFeedback(FEEDBACK_PET_DEAD);
        return;
    }

    pet->RemoveOwnedAura(spellId, 0, 0, AURA_REMOVE_BY_CANCEL);

    pet->AddCreatureSpellCooldown(spellId);
}

void WorldSession::HandleCancelGrowthAuraOpcode(WorldPacket& /*recvPacket*/)
{
}

void WorldSession::HandleCancelAutoRepeatSpellOpcode(WorldPacket& /*recvPacket*/)
{
    // may be better send SMSG_CANCEL_AUTO_REPEAT?
    // cancel and prepare for deleting
    _player->InterruptSpell(CURRENT_AUTOREPEAT_SPELL);
}

void WorldSession::HandleCancelChanneling(WorldPacket & recv_data)
{
    recv_data.read_skip<uint32>();                          // spellid, not used

    // ignore for remote control state (for player case)
    Unit* mover = _player->m_mover;
    if (mover != _player && mover->GetTypeId() == TYPEID_PLAYER)
        return;

    mover->InterruptSpell(CURRENT_CHANNELED_SPELL);
}

void WorldSession::HandleTotemDestroyed(WorldPacket& recvPacket)
{
    // ignore for remote control state
    if (_player->m_mover != _player)
        return;

    uint8 slotId;
    uint64 totemGUID;

    recvPacket >> slotId;
    recvPacket >> totemGUID;

    // Unsummon druid's mushrooms
    if (Creature* totem = GetPlayer()->GetMap()->GetCreature(totemGUID))
    if (totem->IsMushroom() && totem->ToTempSummon())
        totem->ToTempSummon()->UnSummon();

    ++slotId;
    if (slotId >= MAX_TOTEM_SLOT)
        return;

    if (!_player->m_SummonSlot[slotId])
        return;

    Creature* totem = GetPlayer()->GetMap()->GetCreature(_player->m_SummonSlot[slotId]);

    // Don't unsummon sentry totem
    if (totem && totem->IsTotem() && totem->GetEntry() != SENTRY_TOTEM_ENTRY)
        totem->ToTotem()->UnSummon();
}

void WorldSession::HandleSelfResOpcode(WorldPacket & /*recv_data*/)
{
    sLog->outDebug("WORLD: CMSG_SELF_RES");                  // empty opcode

    if (_player->GetUInt32Value(PLAYER_SELF_RES_SPELL))
    {
        SpellEntry const *spellInfo = sSpellStore.LookupEntry(_player->GetUInt32Value(PLAYER_SELF_RES_SPELL));
        if (spellInfo)
            _player->CastSpell(_player,spellInfo,false,0);

        _player->SetUInt32Value(PLAYER_SELF_RES_SPELL, 0);
    }
}

void WorldSession::HandleSpellClick(WorldPacket & recv_data)
{
    uint64 guid;
    recv_data >> guid;

    // this will get something not in world. crash
    Creature *unit = ObjectAccessor::GetCreatureOrPetOrVehicle(*_player, guid);

    if (!unit)
        return;

    // TODO: Unit::SetCharmedBy: 28782 is not in world but 0 is trying to charm it! -> crash
    if (!unit->IsInWorld())
        return;

    SpellClickInfoMapBounds clickPair = sObjectMgr->GetSpellClickInfoMapBounds(unit->GetEntry());
    for (SpellClickInfoMap::const_iterator itr = clickPair.first; itr != clickPair.second; ++itr)
    {
        if (itr->second.IsFitToRequirements(_player, unit))
        {
            Unit *caster = (itr->second.castFlags & NPC_CLICK_CAST_CASTER_PLAYER) ? (Unit*)_player : (Unit*)unit;
            Unit *target = (itr->second.castFlags & NPC_CLICK_CAST_TARGET_PLAYER) ? (Unit*)_player : (Unit*)unit;
            uint64 origCasterGUID = (itr->second.castFlags & NPC_CLICK_CAST_ORIG_CASTER_OWNER) ? unit->GetOwnerGUID() : 0;
            caster->CastSpell(target, itr->second.spellId, true, NULL, NULL, origCasterGUID);
        }
    }

    if (unit->IsVehicle())
    {
        if (unit->CheckPlayerCondition(_player))
            _player->EnterVehicle(unit);
    }

    unit->AI()->DoAction(EVENT_SPELLCLICK);
}

void WorldSession::HandleMirrrorImageDataRequest(WorldPacket & recv_data)
{
    sLog->outDebug("WORLD: CMSG_GET_MIRRORIMAGE_DATA");
    uint64 guid;
    recv_data >> guid;

    // Get unit for which data is needed by client
    Unit* unit = ObjectAccessor::GetObjectInWorld(guid, (Unit*)NULL);
    if (!unit)
        return;

    if (!unit->HasAuraType(SPELL_AURA_CLONE_CASTER))
        return;

    Unit* creator = unit->GetAuraEffectsByType(SPELL_AURA_CLONE_CASTER).front()->GetCaster();
    if (!creator)
        return;

    WorldPacket data(SMSG_MIRRORIMAGE_DATA, 68);
    data << uint64(guid);
    data << uint32(creator->GetDisplayId());
    data << uint8(creator->getRace());
    data << uint8(creator->getGender());
    data << uint8(creator->getClass());

    if (creator->GetTypeId() == TYPEID_PLAYER)
    {
        Player* pCreator = creator->ToPlayer();

        data << uint8(pCreator->GetByteValue(PLAYER_BYTES, 0));   // skin
        data << uint8(pCreator->GetByteValue(PLAYER_BYTES, 1));   // face
        data << uint8(pCreator->GetByteValue(PLAYER_BYTES, 2));   // hair
        data << uint8(pCreator->GetByteValue(PLAYER_BYTES, 3));   // haircolor
        data << uint8(pCreator->GetByteValue(PLAYER_BYTES_2, 0)); // facialhair
        data << uint64(pCreator->GetGuildId() ? MAKE_NEW_GUID(HIGHGUID_GUILD, 0, pCreator->GetGuildId()) : 0);

        static const EquipmentSlots ItemSlots[] =
        {
            EQUIPMENT_SLOT_HEAD,
            EQUIPMENT_SLOT_SHOULDERS,
            EQUIPMENT_SLOT_BODY,
            EQUIPMENT_SLOT_CHEST,
            EQUIPMENT_SLOT_WAIST,
            EQUIPMENT_SLOT_LEGS,
            EQUIPMENT_SLOT_FEET,
            EQUIPMENT_SLOT_WRISTS,
            EQUIPMENT_SLOT_HANDS,
            EQUIPMENT_SLOT_BACK,
            EQUIPMENT_SLOT_TABARD,
            EQUIPMENT_SLOT_END
        };

        // Display items in visible slots
        for (EquipmentSlots const* itr = &ItemSlots[0]; *itr != EQUIPMENT_SLOT_END; ++itr)
        {
            if (*itr == EQUIPMENT_SLOT_HEAD && pCreator->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_HIDE_HELM))
                data << uint32(0);
            else if (*itr == EQUIPMENT_SLOT_BACK && pCreator->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_HIDE_CLOAK))
                data << uint32(0);
            else if (Item const *item = pCreator->GetItemByPos(INVENTORY_SLOT_BAG_0, *itr))
                data << uint32(item->GetProto()->DisplayInfoID);
            else
                data << uint32(0);
        }
    }
    else
    {
        // Skip player data for creatures
        data << uint8(0);
        data << uint32(0);
        data << uint64(0);
        data << uint32(0);
        data << uint32(0);
        data << uint32(0);
        data << uint32(0);
        data << uint32(0);
        data << uint32(0);
        data << uint32(0);
        data << uint32(0);
        data << uint32(0);
        data << uint32(0);
        data << uint32(0);
    }

    SendPacket(&data);
}

void WorldSession::HandleUpdateProjectilePosition(WorldPacket& recvPacket)
{
    sLog->outDebug("WORLD: CMSG_UPDATE_PROJECTILE_POSITION");

    uint64 casterGuid;
    uint32 spellId;
    uint8 castCount;
    float x, y, z;    // Position of missile hit

    recvPacket.readPackGUID(casterGuid);
    recvPacket >> spellId;
    recvPacket >> castCount;
    recvPacket >> x;
    recvPacket >> y;
    recvPacket >> z;

    WorldPacket data(SMSG_SET_PROJECTILE_POSITION, 21);
    data << uint64(casterGuid);
    data << uint8(castCount);
    data << float(x);
    data << float(y);
    data << float(z);
    SendPacket(&data);
}