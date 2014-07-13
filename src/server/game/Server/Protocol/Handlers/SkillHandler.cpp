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
#include "DatabaseEnv.h"
#include "Opcodes.h"
#include "Log.h"
#include "Player.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "ObjectAccessor.h"
#include "UpdateMask.h"
#include "SpellMgr.h"

void WorldSession::HandleLearnTalentOpcode(WorldPacket & recv_data)
{
    uint32 talent_id, requested_rank;
    recv_data >> talent_id >> requested_rank;

    _player->LearnTalent(talent_id, requested_rank, true);
    _player->SendTalentsInfoData(false);
}

void WorldSession::HandleLearnPreviewTalents(WorldPacket& recvPacket)
{
    sLog->outDebug("CMSG_LEARN_PREVIEW_TALENTS");

    uint32 spec;
    uint32 talentsCount;
    recvPacket >> spec >> talentsCount;

    if(spec != ((uint32)-1))
    {
        uint32 specID = 0;
        for(uint32 i = 0; i < sTalentTabStore.GetNumRows(); i++)
        {
            TalentTabEntry const * entry = sTalentTabStore.LookupEntry(i);
            if(entry)
            {
                if(entry->ClassMask == _player->getClassMask() && entry->tabpage == spec)
                {
                    specID = entry->TalentTabID;
                    break;
                }
            }
        }
    
        if(_player->m_usedTalentCount == 0 || _player->GetTalentBranchSpec(_player->m_activeSpec) == 0)
        {
            if(_player->m_usedTalentCount != 0)
                _player->resetTalents();

            _player->SetTalentBranchSpec(specID, _player->m_activeSpec);
            for (uint32 i = 0; i < sTalentTreePrimarySpellsStore.GetNumRows(); ++i)
            {
                TalentTreePrimarySpellsEntry const *talentInfo = sTalentTreePrimarySpellsStore.LookupEntry(i);

                if (!talentInfo || talentInfo->TalentTabID != specID)
                    continue;

                _player->learnSpell(talentInfo->SpellID, false);
            }

            // Learn dummy mastery spells (mostly for displaying in client)
            TalentTabEntry const* tabEntry = sTalentTabStore.LookupEntry(specID);
            if (tabEntry)
            {
                for (uint8 i = 0; i < 2; i++)
                    if (tabEntry->masterySpells[i])
                        _player->learnSpell(tabEntry->masterySpells[i],false);
            }

            _player->UpdateMastery();
        }
        else if(uint32(_player->GetTalentBranchSpec(_player->m_activeSpec)) != specID) //cheat
            return;
    }

    _player->CheckArmorSpecialization();

    uint32 talentId, talentRank;

    for (uint32 i = 0; i < talentsCount; ++i)
    {
        recvPacket >> talentId >> talentRank;

        _player->LearnTalent(talentId, talentRank, false);
    }

    bool inOtherBranch = false;
    uint32 pointInBranchSpec = 0;
    for(PlayerTalentMap::iterator itr = _player->m_talents[_player->m_activeSpec]->begin(); itr != _player->m_talents[_player->m_activeSpec]->end(); itr++)
    {
        for(uint32 i = 0; i < sTalentStore.GetNumRows(); i++)
        {
            const TalentEntry * thisTalent = sTalentStore.LookupEntry(i);
            if(thisTalent) 
            {
                int thisrank = -1;
                for(int j = 0; j < 5; j++)
                    if(thisTalent->RankID[j] == itr->first)
                    {
                        thisrank = j;
                        break;
                    }
                if(thisrank != -1)
                {
                    if(thisTalent->TalentTab == uint32(_player->GetTalentBranchSpec(_player->m_activeSpec)))
                    {
                        int8 curtalent_maxrank = -1;
                        for (int8 rank = MAX_TALENT_RANK-1; rank >= 0; --rank)
                        {
                            if (thisTalent->RankID[rank] && _player->HasTalent(thisTalent->RankID[rank], _player->m_activeSpec))
                            {
                                curtalent_maxrank = rank;
                                break;
                            }
                        }
                        if(curtalent_maxrank != -1 && thisrank == curtalent_maxrank)
                            pointInBranchSpec += curtalent_maxrank + 1;
                    }
                    else
                        inOtherBranch = true;
                }
            }
        }
    }
    if(inOtherBranch && pointInBranchSpec < 31)
        _player->resetTalents();

    _player->SendTalentsInfoData(false);
}

void WorldSession::HandleTalentWipeConfirmOpcode(WorldPacket & recv_data)
{
    sLog->outDetail("MSG_TALENT_WIPE_CONFIRM");
    uint64 guid;
    recv_data >> guid;

    Creature *unit = GetPlayer()->GetNPCIfCanInteractWith(guid,UNIT_NPC_FLAG_TRAINER);
    if (!unit)
    {
        sLog->outDebug("WORLD: HandleTalentWipeConfirmOpcode - Unit (GUID: %u) not found or you can't interact with him.", uint32(GUID_LOPART(guid)));
        return;
    }

    GetPlayer()->RemoveFakeDeath();

    if (!(_player->resetTalents()))
    {
        WorldPacket data(MSG_TALENT_WIPE_CONFIRM, 8+4);    //you have not any talent
        data << uint64(0);
        data << uint32(0);
        SendPacket(&data);
        return;
    }

    _player->SendTalentsInfoData(false);
    unit->CastSpell(_player, 14867, true);                  //spell: "Untalent Visual Effect"
}

void WorldSession::HandleUnlearnSpecialization(WorldPacket& recv_data)
{
    uint32 id;

    recv_data >> id;

    // specialization id is received as new id, we need to manually decide, what to unlearn
    switch (id)
    {
        case 0: // Gnomish Engineer
            _player->RemoveAurasDueToSpell(20219);
            _player->removeSpell(20219);
            _player->IncompleteQuest(29477);
            break;
        case 1: // Goblin Engineer
            _player->RemoveAurasDueToSpell(20222);
            _player->removeSpell(20222);
            _player->IncompleteQuest(29475);
            break;
        case 2: // Elixir Master
            _player->RemoveAurasDueToSpell(28677);
            _player->removeSpell(28677);
            _player->IncompleteQuest(29481);
            break;
        case 3: // Potion Master
            _player->RemoveAurasDueToSpell(28675);
            _player->removeSpell(28675);
            _player->IncompleteQuest(29067);
            break;
        case 4: // Transmutation Master
            _player->RemoveAurasDueToSpell(28672);
            _player->removeSpell(28672);
            _player->IncompleteQuest(29482);
            break;
        default:
            sLog->outError("HandleUnlearnSpecialization: unknown spec id received: %u", id);
            break;
    }
}

void WorldSession::HandleUnlearnSkillOpcode(WorldPacket & recv_data)
{
    uint32 skill_id;
    recv_data >> skill_id;

    if (!IsPrimaryProfessionSkill(skill_id))
        return;

    GetPlayer()->SetSkill(skill_id, 0, 0, 0);
}

