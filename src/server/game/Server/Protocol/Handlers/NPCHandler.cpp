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
#include "Language.h"
#include "DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "GossipDef.h"
#include "UpdateMask.h"
#include "ObjectAccessor.h"
#include "Creature.h"
#include "Pet.h"
#include "BattlegroundMgr.h"
#include "Battleground.h"
#include "ScriptMgr.h"
#include "CreatureAI.h"

enum StableResultCode
{
    STABLE_ERR_MONEY        = 0x01,                         // "you don't have enough money"
    STABLE_ERR_STABLE       = 0x06,                         // currently used in most fail cases
    STABLE_SUCCESS_STABLE   = 0x08,                         // stable success
    STABLE_SUCCESS_UNSTABLE = 0x09,                         // unstable/swap success
    STABLE_SUCCESS_BUY_SLOT = 0x0A,                         // buy slot success
    STABLE_ERR_EXOTIC       = 0x0C,                         // "you are unable to control exotic creatures"
};

void WorldSession::HandleTabardVendorActivateOpcode(WorldPacket & recv_data)
{
    uint64 guid;
    recv_data >> guid;

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(guid,UNIT_NPC_FLAG_TABARDDESIGNER);
    if (!unit)
    {
        sLog->outDebug("WORLD: HandleTabardVendorActivateOpcode - Unit (GUID: %u) not found or you can not interact with him.", uint32(GUID_LOPART(guid)));
        return;
    }

    GetPlayer()->RemoveFakeDeath();

    SendTabardVendorActivate(guid);
}

void WorldSession::SendTabardVendorActivate(uint64 guid)
{
    WorldPacket data(MSG_TABARDVENDOR_ACTIVATE, 8);
    data << guid;
    SendPacket(&data);
}

void WorldSession::HandleBankerActivateOpcode(WorldPacket & recv_data)
{
    uint64 guid;

    sLog->outDebug("WORLD: Received CMSG_BANKER_ACTIVATE");

    recv_data >> guid;

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(guid,UNIT_NPC_FLAG_BANKER);
    if (!unit)
    {
        sLog->outDebug("WORLD: HandleBankerActivateOpcode - Unit (GUID: %u) not found or you can not interact with him.", uint32(GUID_LOPART(guid)));
        return;
    }

    GetPlayer()->RemoveFakeDeath();

    SendShowBank(guid);
}

void WorldSession::SendShowBank(uint64 guid)
{
    WorldPacket data(SMSG_SHOW_BANK, 8);
    data << guid;
    SendPacket(&data);
}

void WorldSession::SendShowMail(uint64 guid)
{
    WorldPacket data(SMSG_SHOW_MAILBOX, 8);
    data << guid;
    SendPacket(&data);
}

void WorldSession::HandleTrainerListOpcode(WorldPacket & recv_data)
{
    uint64 guid;
    uint32 spellId;

    recv_data >> guid >> spellId;
    SendTrainerList(guid);
}

void WorldSession::SendTrainerList(uint64 guid)
{
    std::string str = GetTrinityString(LANG_NPC_TAINER_HELLO);
    SendTrainerList(guid, str);
}

void WorldSession::SendTrainerList(uint64 guid, const std::string& strTitle)
{
    sLog->outDebug("WORLD: SendTrainerList");

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(guid,UNIT_NPC_FLAG_TRAINER);
    if (!unit)
    {
        sLog->outDebug("WORLD: SendTrainerList - Unit (GUID: %u) not found or you can not interact with him.", uint32(GUID_LOPART(guid)));
        return;
    }

    GetPlayer()->RemoveFakeDeath();

    // trainer list loaded at check;
    if (!unit->isCanTrainingOf(_player,true))
        return;

    CreatureInfo const *ci = unit->GetCreatureInfo();

    if (!ci)
    {
        sLog->outDebug("WORLD: SendTrainerList - (GUID: %u) NO CREATUREINFO!",GUID_LOPART(guid));
        return;
    }

    TrainerSpellData const* trainer_spells = unit->GetTrainerSpells();
    if (!trainer_spells)
    {
        sLog->outDebug("WORLD: SendTrainerList - Training spells not found for creature (GUID: %u Entry: %u)",
            GUID_LOPART(guid), unit->GetEntry());
        return;
    }

    WorldPacket data(SMSG_TRAINER_LIST, 8 + 4 + 4 + trainer_spells->spellList.size() * 38 + strTitle.size() + 1);
    data << guid;
    data << uint32(trainer_spells->trainerType);
    data << uint32(1); // this value is only sent back in TrainerBuySpell opcode

    size_t count_pos = data.wpos();
    data << uint32(trainer_spells->spellList.size());

    // reputation discount
    float fDiscountMod = _player->GetReputationPriceDiscount(unit);

    uint32 count = 0;
    for (TrainerSpellMap::const_iterator itr = trainer_spells->spellList.begin(); itr != trainer_spells->spellList.end(); ++itr)
    {
        TrainerSpell const* tSpell = &itr->second;
        TrainerSpellState state = _player->GetTrainerSpellState(tSpell);

        data << uint32(tSpell->spell);                      // learned spell (or cast-spell in profession case)
        data << uint8(state);
        data << uint32(floor(tSpell->spellCost * fDiscountMod));

        data << uint8(tSpell->reqLevel);
        data << uint32(tSpell->reqSkill);
        data << uint32(tSpell->reqSkillValue);
        for (int i = 0; i < 3; i++)
            data << uint32(tSpell->reqSpell[i]);
        data << uint32(0);

        ++count;
    }

    data << strTitle;

    data.put<uint32>(count_pos,count);
    SendPacket(&data);
}

void WorldSession::HandleTrainerBuySpellOpcode(WorldPacket & recv_data)
{
    uint64 guid;
    uint32 spellId = 0, trainerId, result = ERR_TRAINER_OK;

    // unk uint32 added in 4.0.1
    recv_data >> guid >> trainerId >> spellId;
    sLog->outDebug("WORLD: Received CMSG_TRAINER_BUY_SPELL NpcGUID=%u, learn spell id is: %u",uint32(GUID_LOPART(guid)), spellId);

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_TRAINER);
    if (!unit)
    {
        sLog->outDebug("WORLD: HandleTrainerBuySpellOpcode - Unit (GUID: %u) not found or you can not interact with him.", uint32(GUID_LOPART(guid)));
        return;
    }

    GetPlayer()->RemoveFakeDeath();

    if (!unit->isCanTrainingOf(_player,true)) {
        SendTrainerBuyFailed(guid, spellId, 0);
        return;
    }

    // check present spell in trainer spell list
    TrainerSpellData const* trainer_spells = unit->GetTrainerSpells();
    if (!trainer_spells) {
        SendTrainerBuyFailed(guid, spellId, 0);
        return;
    }

    // not found, cheat?
    TrainerSpell const* trainer_spell = trainer_spells->Find(spellId);
    if (!trainer_spell) {
        SendTrainerBuyFailed(guid, spellId, 0);
        return;
    }

    // can't be learn, cheat? Or double learn with lags...
    if (_player->GetTrainerSpellState(trainer_spell) != TRAINER_SPELL_CAN_LEARN) {
        SendTrainerBuyFailed(guid, spellId, 0);
        return;
    }

    // apply reputation discount
    uint64 nSpellCost = uint64(floor(trainer_spell->spellCost * _player->GetReputationPriceDiscount(unit)));

    // check money requirement
    if (!_player->HasEnoughMoney(nSpellCost)) {
        SendTrainerBuyFailed(guid, spellId, 1);
        result = ERR_TRAINER_NOT_ENOUGH_MONEY;
        return;
    }

    if(result == ERR_TRAINER_OK)
    {
        _player->ModifyMoney(-int64(nSpellCost));

        unit->SendPlaySpellVisualKit(179, 0, 0);       // 53 SpellCastDirected
        _player->SendPlaySpellVisualKit(362, 1, 0);    // 113 EmoteSalute

        // learn explicitly or cast explicitly
        if (trainer_spell->IsCastable())
            _player->CastSpell(_player,trainer_spell->spell,true);
        else
            _player->LearnSpell(spellId, false);
    }

    WorldPacket data(SMSG_TRAINER_BUY_SUCCEEDED, 12);
    data << uint64(guid);
    data << uint32(spellId);
    SendPacket(&data);
}

void WorldSession::SendTrainerBuyFailed(uint64 guid, uint32 spellId, uint32 reason)
{
    WorldPacket data(SMSG_TRAINER_BUY_FAILED, 16);
    data << uint64(guid);
    data << uint32(spellId);        // should be same as in packet from client
    data << uint32(reason);         // 1 == "Not enough money for trainer service." 0 == "Trainer service %d unavailable."
    SendPacket(&data);
}

void WorldSession::HandleGossipHelloOpcode(WorldPacket & recv_data)
{
    sLog->outDebug("WORLD: Received CMSG_GOSSIP_HELLO");

    uint64 guid;
    recv_data >> guid;

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE);
    if (!unit)
    {
        sLog->outDebug("WORLD: HandleGossipHelloOpcode - Unit (GUID: %u) not found or you can not interact with him.", uint32(GUID_LOPART(guid)));
        return;
    }

    GetPlayer()->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TALK);
    // GetPlayer()->RemoveFakeDeath();

    if (unit->IsArmorer() || unit->IsCivilian() || unit->IsQuestGiver() || unit->IsServiceProvider())
    {
        unit->StopMoving();
    }

    // iCelike quest workaround
    _player->KilledMonsterCredit(unit->GetEntry(),unit->GetGUID());

    // If spiritguide, no need for gossip menu, just put player into resurrect queue
    if (unit->IsSpiritGuide())
    {
        Battleground *bg = _player->GetBattleground();
        if (bg)
        {
            bg->AddPlayerToResurrectQueue(unit->GetGUID(), _player->GetGUID());
            sBattlegroundMgr->SendAreaSpiritHealerQueryOpcode(_player, bg, unit->GetGUID());
            return;
        }
    }

    if (!sScriptMgr->OnGossipHello(_player, unit))
    {
//        _player->TalkedToCreature(unit->GetEntry(), unit->GetGUID());
        _player->PrepareGossipMenu(unit, unit->GetCreatureInfo()->GossipMenuId, true);
        _player->SendPreparedGossip(unit);
    }
    unit->AI()->sGossipHello(_player);
}

/*void WorldSession::HandleGossipSelectOptionOpcode(WorldPacket & recv_data)
{
    sLog->outDebug("WORLD: CMSG_GOSSIP_SELECT_OPTION");

    uint32 option;
    uint32 unk;
    uint64 guid;
    std::string code = "";

    recv_data >> guid >> unk >> option;

    if (_player->PlayerTalkClass->GossipOptionCoded(option))
    {
        sLog->outDebug("reading string");
        recv_data >> code;
        sLog->outDebug("string read: %s", code.c_str());
    }

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE);
    if (!unit)
    {
        sLog->outDebug("WORLD: HandleGossipSelectOptionOpcode - Unit (GUID: %u) not found or you can't interact with him.", uint32(GUID_LOPART(guid)));
        return;
    }

    GetPlayer()->RemoveFakeDeath();

    if (!code.empty())
    {
        if (!Script->GossipSelectWithCode(_player, unit, _player->PlayerTalkClass->GossipOptionSender (option), _player->PlayerTalkClass->GossipOptionAction(option), code.c_str()))
            unit->OnGossipSelect (_player, option);
    }
    else
    {
        if (!Script->OnGossipSelect (_player, unit, _player->PlayerTalkClass->GossipOptionSender (option), _player->PlayerTalkClass->GossipOptionAction (option)))
           unit->OnGossipSelect (_player, option);
    }
}*/

void WorldSession::HandleSpiritHealerActivateOpcode(WorldPacket & recv_data)
{
    sLog->outDebug("WORLD: CMSG_SPIRIT_HEALER_ACTIVATE");

    uint64 guid;

    recv_data >> guid;

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_SPIRITHEALER);
    if (!unit)
    {
        sLog->outDebug("WORLD: HandleSpiritHealerActivateOpcode - Unit (GUID: %u) not found or you can not interact with him.", uint32(GUID_LOPART(guid)));
        return;
    }

    GetPlayer()->RemoveFakeDeath();

    SendSpiritResurrect();
}

void WorldSession::SendSpiritResurrect()
{
    _player->ResurrectPlayer(0.5f, true);

    _player->DurabilityLossAll(0.25f,true);

    // get corpse nearest graveyard
    WorldSafeLocsEntry const *corpseGrave = NULL;
    Corpse *corpse = _player->GetCorpse();
    if (corpse)
        corpseGrave = sObjectMgr->GetClosestGraveYard(
            corpse->GetPositionX(), corpse->GetPositionY(), corpse->GetPositionZ(), corpse->GetMapId(), _player->GetTeam());

    // now can spawn bones
    _player->SpawnCorpseBones();

    // teleport to nearest from corpse graveyard, if different from nearest to player ghost
    if (corpseGrave)
    {
        WorldSafeLocsEntry const *ghostGrave = sObjectMgr->GetClosestGraveYard(
            _player->GetPositionX(), _player->GetPositionY(), _player->GetPositionZ(), _player->GetMapId(), _player->GetTeam());

        if (corpseGrave != ghostGrave)
            _player->TeleportTo(corpseGrave->map_id, corpseGrave->x, corpseGrave->y, corpseGrave->z, _player->GetOrientation());
        // or update at original position
        else
            _player->UpdateObjectVisibility();
    }
    // or update at original position
    else
        _player->UpdateObjectVisibility();
}

void WorldSession::HandleBinderActivateOpcode(WorldPacket & recv_data)
{
    uint64 npcGUID;
    recv_data >> npcGUID;

    if (!GetPlayer()->IsInWorld() || !GetPlayer()->IsAlive())
        return;

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(npcGUID,UNIT_NPC_FLAG_INNKEEPER);
    if (!unit)
    {
        sLog->outDebug("WORLD: HandleBinderActivateOpcode - Unit (GUID: %u) not found or you can not interact with him.", uint32(GUID_LOPART(npcGUID)));
        return;
    }

    GetPlayer()->RemoveFakeDeath();

    SendBindPoint(unit);
}

void WorldSession::SendBindPoint(Creature *npc)
{
    // prevent set homebind to instances in any case
    if (GetPlayer()->GetMap()->Instanceable())
        return;

    uint32 bindspell = 3286;

    // update sql homebind
    CharacterDatabase.PExecute("UPDATE character_homebind SET map = '%u', zone = '%u', position_x = '%f', position_y = '%f', position_z = '%f' WHERE guid = '%u'",
        _player->GetMapId(), _player->GetAreaId(), _player->GetPositionX(), _player->GetPositionY(), _player->GetPositionZ(), _player->GetGUIDLow());
    _player->m_homebindMapId = _player->GetMapId();
    _player->m_homebindAreaId = _player->GetAreaId();
    _player->m_homebindX = _player->GetPositionX();
    _player->m_homebindY = _player->GetPositionY();
    _player->m_homebindZ = _player->GetPositionZ();

    // send spell for homebinding (3286)
    npc->CastSpell(_player, bindspell, true);


    WorldPacket data(SMSG_TRAINER_BUY_SUCCEEDED, 12);
    data << uint64(npc->GetGUID());
    data << uint32(bindspell);
    SendPacket(&data);

    _player->PlayerTalkClass->CloseGossip();
}

void WorldSession::HandleListStabledPetsOpcode(WorldPacket & recv_data)
{
    sLog->outDebug("WORLD: Recv MSG_LIST_STABLED_PETS");
    uint64 npcGUID;

    recv_data >> npcGUID;

    if (!CheckStableMaster(npcGUID))
        return;

    GetPlayer()->RemoveFakeDeath();

    // remove mounts this fix bug where getting pet from stable while mounted deletes pet.
    if (GetPlayer()->IsMounted())
        GetPlayer()->RemoveAurasByType(SPELL_AURA_MOUNTED);

    SendStablePet(npcGUID);
}

void WorldSession::SendStablePet(uint64 guid)
{
    m_sendStabledPetCallback.SetParam(guid);
    m_sendStabledPetCallback.SetFutureResult(
        CharacterDatabase.AsyncPQuery("SELECT owner, slot, id, entry, level, name FROM character_pet WHERE owner = '%u' AND slot >= '%u' AND slot <= '%u' ORDER BY slot",
            _player->GetGUIDLow(), PET_SLOT_HUNTER_FIRST, PET_SLOT_STABLE_LAST)
        );
}

void WorldSession::SendStablePetCallback(QueryResult result, uint64 guid)
{
    if (!GetPlayer())
        return;

    sLog->outDebug("WORLD: Recv MSG_LIST_STABLED_PETS Send.");

    WorldPacket data(MSG_LIST_STABLED_PETS, 200);           // guess size

    data << uint64 (guid);

    size_t wpos = data.wpos();
    data << uint8(0);                                       // place holder for slot show number

    data << uint8(20);                                      //

    uint8 num = 0;                                          // counter for place holder

    if (result)
    {
        do
        {
            Field *fields = result->Fetch();

            data << uint32(fields[1].GetUInt32());                  // slot
            data << uint32(fields[2].GetUInt32());                  // petnumber
            data << uint32(fields[3].GetUInt32());                  // creature entry
            data << uint32(fields[4].GetUInt32());                  // level
            data << fields[5].GetString();                          // name
            data << uint8(fields[1].GetUInt32() <= PET_SLOT_STABLE_FIRST ? 1 : 2);   // 1 = current, 2/3 = in stable (any from 4,5,... create problems with proper show)

            ++num;
        }
        while (result->NextRow());
    }

    data.put<uint8>(wpos, num);                             // set real data to placeholder
    SendPacket(&data);
}

void WorldSession::SendStableResult(uint8 res)
{
    WorldPacket data(SMSG_STABLE_RESULT, 1);
    data << uint8(res);
    SendPacket(&data);
}

void WorldSession::HandleStableRevivePet(WorldPacket &/* recv_data */)
{
    sLog->outDebug("HandleStableRevivePet: Not implemented");
}

void WorldSession::HandleSetPetSlotOpcode(WorldPacket& recvPacket)
{
    uint32 petnumber;
    uint8 slot;
    ObjectGuid guid;

    recvPacket >> petnumber;
    recvPacket >> slot;

    guid[3] = recvPacket.ReadBit();
    guid[2] = recvPacket.ReadBit();
    guid[0] = recvPacket.ReadBit();
    guid[7] = recvPacket.ReadBit();
    guid[5] = recvPacket.ReadBit();
    guid[6] = recvPacket.ReadBit();
    guid[1] = recvPacket.ReadBit();
    guid[4] = recvPacket.ReadBit();

    recvPacket.ReadByteSeq(guid[5]);
    recvPacket.ReadByteSeq(guid[3]);
    recvPacket.ReadByteSeq(guid[1]);
    recvPacket.ReadByteSeq(guid[7]);
    recvPacket.ReadByteSeq(guid[4]);
    recvPacket.ReadByteSeq(guid[0]);
    recvPacket.ReadByteSeq(guid[6]);
    recvPacket.ReadByteSeq(guid[2]);

    if (!CheckStableMaster((uint64)guid))
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    if (slot > MAX_PET_STABLES)
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    GetPlayer()->RemoveFakeDeath();

    Pet* pet = _player->GetPet();

    //If we move the pet already summoned...
    if (pet && pet->GetCharmInfo() && pet->GetCharmInfo()->GetPetNumber() == petnumber)
        _player->RemovePet(pet);

    //If we move to the pet already summoned...
    if (pet && GetPlayer()->m_currentPetSlot == slot)
        _player->RemovePet(pet);

    m_stableChangeSlotCallback.SetParam(slot);
    m_stableChangeSlotCallback.SetFutureResult(CharacterDatabase.PQuery("SELECT slot,entry,id FROM character_pet WHERE owner = '%u' AND id = '%u'", _player->GetGUIDLow(), petnumber));
}

void WorldSession::HandleStableChangeSlotCallback(QueryResult result, uint8 new_slot)
{
    if (!GetPlayer())
        return;

    if (!result)
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    Field *fields = result->Fetch();

    uint32 slot        = fields[0].GetUInt32();
    uint32 creature_id = fields[1].GetUInt32();
    uint32 pet_number  = fields[2].GetUInt32();

    if (!creature_id)
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    CreatureInfo const* creatureInfo = sObjectMgr->GetCreatureTemplate(creature_id);
    if (!creatureInfo || !creatureInfo->isTameable(_player->CanTameExoticPets()))
    {
        // if problem in exotic pet
        if (creatureInfo && creatureInfo->isTameable(true))
            SendStableResult(STABLE_ERR_EXOTIC);
        else
            SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    CharacterDatabase.PExecute("UPDATE character_pet SET slot = '%u' WHERE slot = '%u' AND owner='%u'",                      new_slot, slot, GetPlayer()->GetGUIDLow());
    CharacterDatabase.PExecute("UPDATE character_pet SET slot = '%u' WHERE slot = '%u' AND owner='%u' AND id<>'%u'", slot, new_slot, GetPlayer()->GetGUIDLow(), pet_number);

    // Update if its a Hunter pet
    if (new_slot != 100)
    {
        // We need to remove and add the new pet to there diffrent slots
        GetPlayer()->setPetSlotUsed((PetSlot)slot, false); 
        GetPlayer()->setPetSlotUsed((PetSlot)new_slot, true); 
    }

    SendStableResult(STABLE_SUCCESS_STABLE);
}

void WorldSession::HandleRepairItemOpcode(WorldPacket & recv_data)
{
    sLog->outDebug("WORLD: CMSG_REPAIR_ITEM");

    uint64 npcGUID, itemGUID;
    uint8 guildBank;                                        // new in 2.3.2, bool that means from guild bank money

    recv_data >> npcGUID >> itemGUID >> guildBank;

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(npcGUID, UNIT_NPC_FLAG_REPAIR);
    if (!unit)
    {
        sLog->outDebug("WORLD: HandleRepairItemOpcode - Unit (GUID: %u) not found or you can not interact with him.", uint32(GUID_LOPART(npcGUID)));
        return;
    }

    GetPlayer()->RemoveFakeDeath();

    // reputation discount
    float discountMod = _player->GetReputationPriceDiscount(unit);

    if (itemGUID)
    {
        sLog->outDebug("ITEM: Repair item, itemGUID = %u, npcGUID = %u", GUID_LOPART(itemGUID), GUID_LOPART(npcGUID));

        Item* item = _player->GetItemByGuid(itemGUID);
        if (item)
            _player->DurabilityRepair(item->GetPos(), true, discountMod, guildBank);
    }
    else
    {
        sLog->outDebug("ITEM: Repair all items, npcGUID = %u", GUID_LOPART(npcGUID));
        _player->DurabilityRepairAll(true, discountMod, guildBank);
    }
}

