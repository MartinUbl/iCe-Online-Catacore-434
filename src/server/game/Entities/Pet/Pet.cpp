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
#include "Log.h"
#include "WorldPacket.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Pet.h"
#include "Formulas.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "CreatureAI.h"
#include "Unit.h"
#include "Util.h"

char const* petTypeSuffix[MAX_PET_TYPE] =
{
    "'s Minion",                                            // SUMMON_PET
    "'s Pet",                                               // HUNTER_PET
    "'s Guardian",                                          // GUARDIAN_PET
    "'s Companion"                                          // MINI_PET
};

#define PET_XP_FACTOR 0.05f

Pet::Pet(Player *owner, PetType type) : Guardian(NULL, owner),
m_resetTalentsCost(0), m_resetTalentsTime(0), m_usedTalentCount(0),
m_removed(false), m_owner(owner), m_happinessTimer(7500), m_petType(type),
m_duration(0), m_auraRaidUpdateMask(0), m_loading(false), m_declinedname(NULL)
{
    m_unitTypeMask |= UNIT_MASK_PET;
    if (type == HUNTER_PET)
        m_unitTypeMask |= UNIT_MASK_HUNTER_PET;

    if (!(m_unitTypeMask & UNIT_MASK_CONTROLABLE_GUARDIAN))
    {
        m_unitTypeMask |= UNIT_MASK_CONTROLABLE_GUARDIAN;
        InitCharmInfo();
    }

    m_name = "Pet";

    m_isWorldObject = true;
}

Pet::~Pet()
{
    delete m_declinedname;
}

void Pet::AddToWorld()
{
    ///- Register the pet for guid lookup
    if (!IsInWorld())
    {
        ///- Register the pet for guid lookup
        sObjectAccessor->AddObject(this);
        Unit::AddToWorld();
        AIM_Initialize();
    }

    // Prevent stuck pets when zoning. Pets default to "follow" when added to world
    // so we'll reset flags and let the AI handle things
    if (GetCharmInfo() && GetCharmInfo()->HasCommandState(COMMAND_FOLLOW))
    {
        GetCharmInfo()->SetIsCommandAttack(false);
        GetCharmInfo()->SetIsAtStay(false);
        GetCharmInfo()->SetIsFollowing(false);
        GetCharmInfo()->SetIsReturning(false);
    }

}

void Pet::RemoveFromWorld()
{
    ///- Remove the pet from the accessor
    if (IsInWorld())
    {
        ///- Don't call the function for Creature, normal mobs + totems go in a different storage
        Unit::RemoveFromWorld();
        sObjectAccessor->RemoveObject(this);
    }
}

bool Pet::LoadPetFromDB(Player* owner, uint32 petentry, uint32 petnumber, bool current, PetSlot slotID)
{
    m_loading = true;

    if(slotID == PET_SLOT_ACTUAL_PET_SLOT)
        slotID = owner->m_currentPetSlot;

    uint32 ownerid = owner->GetGUIDLow();

    QueryResult result;

    if (petnumber)
        // known petnumber entry                  0   1      2(?)   3        4      5    6           7     8     9        10         11       12            13      14        15                 16                 17              18
        result = CharacterDatabase.PQuery("SELECT id, entry, owner, modelid, level, exp, Reactstate, slot, name, renamed, curhealth, curmana, curhappiness, abdata, savetime, resettalents_cost, resettalents_time, CreatedBySpell, PetType "
            "FROM character_pet WHERE owner = '%u' AND id = '%u'",
            ownerid, petnumber);
    else if (current && slotID != PET_SLOT_UNK_SLOT)
        // current pet                            0   1      2(?)   3        4      5    6           7     8     9        10         11       12            13      14        15                 16                 17              18
        result = CharacterDatabase.PQuery("SELECT id, entry, owner, modelid, level, exp, Reactstate, slot, name, renamed, curhealth, curmana, curhappiness, abdata, savetime, resettalents_cost, resettalents_time, CreatedBySpell, PetType "
            "FROM character_pet WHERE owner = '%u' AND slot = '%u'",
            ownerid, slotID);
    else if (petentry)
        // known petentry entry (unique for summoned pet, but non unique for hunter pet (only from current or not stabled pets)
        //                                        0   1      2(?)   3        4      5    6           7     8     9        10         11       12           13       14        15                 16                 17              18
        result = CharacterDatabase.PQuery("SELECT id, entry, owner, modelid, level, exp, Reactstate, slot, name, renamed, curhealth, curmana, curhappiness, abdata, savetime, resettalents_cost, resettalents_time, CreatedBySpell, PetType "
            "FROM character_pet WHERE owner = '%u' AND entry = '%u' AND ((slot >= '%u' AND slot <= '%u') OR slot > '%u')",
            ownerid, petentry,PET_SLOT_HUNTER_FIRST,PET_SLOT_HUNTER_LAST,PET_SLOT_STABLE_LAST);
    else
        // any current or other non-stabled pet (for hunter "call pet")
        //                                        0   1      2(?)   3        4      5    6           7     8     9        10         11       12            13      14        15                 16                 17              18
        result = CharacterDatabase.PQuery("SELECT id, entry, owner, modelid, level, exp, Reactstate, slot, name, renamed, curhealth, curmana, curhappiness, abdata, savetime, resettalents_cost, resettalents_time, CreatedBySpell, PetType "
            "FROM character_pet WHERE owner = '%u' AND ((slot >= '%u' AND slot <= '%u') OR slot > '%u')",
            ownerid,PET_SLOT_HUNTER_FIRST,PET_SLOT_HUNTER_LAST,PET_SLOT_STABLE_LAST);

    if (!result)
    {
        m_loading = false;
        return false;
    }

    Field *fields = result->Fetch();

    // update for case of current pet "slot = 0"
    petentry = fields[1].GetUInt32();
    if (!petentry)
    {
        m_loading = false;
        return false;
    }

    uint32 summon_spell_id = fields[17].GetUInt32();
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(summon_spell_id);

    bool is_temporary_summoned = spellInfo && GetSpellDuration(spellInfo) > 0;

    // check temporary summoned pets like mage water elemental
    if (current && is_temporary_summoned)
    {
        m_loading = false;
        return false;
    }

    PetType pet_type = PetType(fields[18].GetUInt8());
    if (pet_type == HUNTER_PET)
    {
        CreatureInfo const* creatureInfo = sObjectMgr->GetCreatureTemplate(petentry);
        if (!creatureInfo || !creatureInfo->isTameable(owner->CanTameExoticPets()))
        {
            m_loading = false;
            return false;
        }
    }
    else if (pet_type == SUMMON_PET && summon_spell_id && !owner->HasSpell(summon_spell_id))
    {
        // Raise Dead exception - pet summoned by triggered spell
        if (summon_spell_id != 52150)
        {
            // pet is summon but owner has no summon spell (e.g.: Water Elemental)
            m_loading = false;
            return false;
        }
        else if(!owner->HasAura(52143))//Raise dead but not in Unholy talent specialization
        {
            m_loading = false;
            return false;
        }
    }

    // exception for Summon Water Elemental (mage) being summoned due to summon spell id not being saved
    if (pet_type == SUMMON_PET && summon_spell_id == 0 && petentry == 510)
    {
        if (!owner->HasSpell(31687))
        {
            m_loading = false;
            return false;
        }
    }

    uint32 pet_number = fields[0].GetUInt32();

    if (current && owner->IsPetNeedBeTemporaryUnsummoned())
    {
        owner->SetTemporaryUnsummonedPetNumber(pet_number);
        m_loading = false;
        return false;
    }

    Map *map = owner->GetMap();
    if (!Create(pet_number, map, owner->GetPhaseMask(), petentry))
    {
        m_loading = false;
        return false;
    }

    float px, py, pz;
    owner->GetClosePoint(px, py, pz, GetObjectSize(), PET_FOLLOW_DIST, GetFollowAngle());
    Relocate(px, py, pz, owner->GetOrientation());

    if (!IsPositionValid())
    {
        sLog->outError("Pet (guidlow %d, entry %d) not loaded. Suggested coordinates isn't valid (X: %f Y: %f)",
            GetGUIDLow(), GetEntry(), GetPositionX(), GetPositionY());
        m_loading = false;
        return false;
    }

    setPetType(pet_type);
    setFaction(owner->getFaction());
    SetUInt32Value(UNIT_CREATED_BY_SPELL, summon_spell_id);

    CreatureInfo const *cinfo = GetCreatureInfo();
    if (cinfo->type == CREATURE_TYPE_CRITTER)
    {
        m_loading = false;
        map->Add(this->ToCreature());
        return true;
    }

    m_charmInfo->SetPetNumber(pet_number, IsPermanentPetFor(owner));

    SetDisplayId(fields[3].GetUInt32());
    SetNativeDisplayId(fields[3].GetUInt32());
    uint32 petlevel = fields[4].GetUInt32();
    SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);
    SetName(fields[8].GetString());

    switch (getPetType())
    {
        case SUMMON_PET:
            petlevel=owner->getLevel();

            if (IsPetGhoul())
                SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_ROGUE); // technically, it's a rogue (has energy)
            else
                SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_MAGE);

            SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
                                                            // this enables popup window (pet dismiss, cancel)
            break;
        case HUNTER_PET:
            SetUInt32Value(UNIT_FIELD_BYTES_0, 0x02020100); //class=warrior,gender=none,power=focus
            SetSheath(SHEATH_STATE_MELEE);
            SetByteFlag(UNIT_FIELD_BYTES_2, 2, fields[9].GetBool() ? UNIT_CAN_BE_ABANDONED : UNIT_CAN_BE_RENAMED | UNIT_CAN_BE_ABANDONED);

            SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
                                                            // this enables popup window (pet abandon, cancel)
            setPowerType(POWER_FOCUS);
            break;
        default:
            if (!IsPetGhoul())
                sLog->outError("Pet have incorrect type (%u) for pet loading.", getPetType());
            break;
    }

    SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(time(NULL))); // cast can't be helped here
    SetCreatorGUID(owner->GetGUID());

    InitStatsForLevel(petlevel);
    SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, fields[5].GetUInt32());

    SynchronizeLevelWithOwner();

    SetReactState(ReactStates(fields[6].GetUInt8()));
    SetCanModifyStats(true);

    if (getPetType() == SUMMON_PET && !current)              //all (?) summon pets come with full health when called, but not when they are current
    {
        if (IsPetGhoul())
            SetPower(POWER_ENERGY, GetMaxPower(POWER_ENERGY));
        else
            SetPower(POWER_MANA, GetMaxPower(POWER_MANA));
    }
    else
    {
        uint32 savedhealth = fields[10].GetUInt32();
        uint32 savedmana = fields[11].GetUInt32();
        if (!savedhealth && getPetType() == HUNTER_PET)
            setDeathState(JUST_DIED);
        else
        {
            SetHealth(savedhealth > GetMaxHealth() ? GetMaxHealth() : savedhealth);
            SetPower(POWER_MANA, savedmana > GetMaxPower(POWER_MANA) ? GetMaxPower(POWER_MANA) : savedmana);
        }
    }

    // Send fake summon spell cast - this is needed for correct cooldown application for spells
    // Example: 46584 - without this cooldown (which should be set always when pet is loaded) isn't set clientside
    // TODO: pets should be summoned from real cast instead of just faking it?
    if (GetUInt32Value(UNIT_CREATED_BY_SPELL))
    {
        // don't summon water elemental if we haven't frost specs
        if (!owner->HasSpell(31687) && GetUInt32Value(UNIT_CREATED_BY_SPELL) != 31687)
        {
            WorldPacket data(SMSG_SPELL_GO, (8+8+4+4+2));
            data.append(owner->GetPackGUID());
            data.append(owner->GetPackGUID());
            data << uint8(0);
            data << uint32(GetUInt32Value(UNIT_CREATED_BY_SPELL));
            data << uint32(256); // CAST_FLAG_UNKNOWN3
            data << uint32(0);
            data << uint32(0);
            owner->SendMessageToSet(&data, true);
        }
    }

    owner->SetMinion(this, true, slotID == PET_SLOT_UNK_SLOT ? PET_SLOT_OTHER_PET : slotID);
    map->Add(this->ToCreature());

    m_resetTalentsCost = fields[15].GetUInt32();
    m_resetTalentsTime = fields[16].GetUInt64();
    InitTalentForLevel();                                   // set original talents points before spell loading

    uint32 timediff = uint32(time(NULL) - fields[14].GetUInt32());
    _LoadAuras(timediff);

    // load action bar, if data broken will fill later by default spells.
    if (!is_temporary_summoned)
    {
        m_charmInfo->LoadPetActionBar(fields[13].GetString());

        _LoadSpells();
        InitTalentForLevel();                               // re-init to check talent count
        _LoadSpellCooldowns();
        LearnPetPassives();
        InitLevelupSpellsForLevel();
        CastPetAuras(current);
    }

    CleanupActionBar();                                     // remove unknown spells from action bar after load

    sLog->outDebug("New Pet has guid %u", GetGUIDLow());

    owner->PetSpellInitialize();

    if (owner->GetGroup())
        owner->SetGroupUpdateFlag(GROUP_UPDATE_PET);

    owner->SendTalentsInfoData(true);

    if (getPetType() == HUNTER_PET)
    {
        result = CharacterDatabase.PQuery("SELECT genitive, dative, accusative, instrumental, prepositional FROM character_pet_declinedname WHERE owner = '%u' AND id = '%u'", owner->GetGUIDLow(), GetCharmInfo()->GetPetNumber());

        if (result)
        {
            delete m_declinedname;
            m_declinedname = new DeclinedName;
            Field *fields2 = result->Fetch();
            for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
            {
                m_declinedname->name[i] = fields2[i].GetString();
            }
        }
    }

    //set last used pet number (for use in BG's)
    if (owner->GetTypeId() == TYPEID_PLAYER && isControlled() && !isTemporarySummoned() && (getPetType() == SUMMON_PET || getPetType() == HUNTER_PET || IsWarlockPet()))
        owner->ToPlayer()->SetLastPetNumber(pet_number);

    m_loading = false;

    return true;
}

void Pet::SavePetToDB(PetSlot mode)
{
    if (!GetEntry())
        return;

    // save only fully controlled creature
    if (!isControlled())
        return;

    // not save not player pets
    if (!IS_PLAYER_GUID(GetOwnerGUID()))
        return;

    Player* pOwner = (Player*)GetOwner();
    if (!pOwner)
        return;

    if(mode == PET_SLOT_ACTUAL_PET_SLOT)
        mode = pOwner->m_currentPetSlot;
    //if(mode >= PET_SLOT_HUNTER_FIRST && mode <= PET_SLOT_HUNTER_LAST && getPetType() != HUNTER_PET)
    //    assert(false);
    //if(mode == PET_SLOT_OTHER_PET && getPetType() == HUNTER_PET)
    //    assert(false);
    
    // not save pet as current if another pet temporary unsummoned
    if (mode == pOwner->m_currentPetSlot && pOwner->GetTemporaryUnsummonedPetNumber() &&
        pOwner->GetTemporaryUnsummonedPetNumber() != m_charmInfo->GetPetNumber())
    {
        // pet will lost anyway at restore temporary unsummoned
        if (getPetType() == HUNTER_PET)
            return;
    }

    uint32 curhealth = GetHealth();
    uint32 curmana = GetPower(POWER_MANA);

    // current/stable/not_in_slot
    if (mode >= PET_SLOT_HUNTER_FIRST)
    {
        uint32 owner = GUID_LOPART(GetOwnerGUID());
        std::string name = m_name;
        CharacterDatabase.escape_string(name);

        SQLTransaction trans = CharacterDatabase.BeginTransaction();

        // save pet
        std::ostringstream ss;
        ss  << "REPLACE INTO character_pet (id, entry,  owner, modelid, level, exp, Reactstate, slot, name, renamed, curhealth, curmana, curhappiness, abdata, savetime, resettalents_cost, resettalents_time, CreatedBySpell, PetType) "
            << "VALUES ("
            << m_charmInfo->GetPetNumber() << ", "
            << GetEntry() << ", "
            << owner << ", "
            << GetNativeDisplayId() << ", "
            << uint32(getLevel()) << ", "
            << GetUInt32Value(UNIT_FIELD_PETEXPERIENCE) << ", "
            << uint32(GetReactState()) << ", "
            << uint32(mode) << ", '"
            << name.c_str() << "', "
            << uint32(HasByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED) ? 0 : 1) << ", "
            << curhealth << ", "
            << curmana << ", "
            << "0, '";

        for (uint32 i = ACTION_BAR_INDEX_START; i < ACTION_BAR_INDEX_END; ++i)
        {
            ss << uint32(m_charmInfo->GetActionBarEntry(i)->GetType()) << " "
               << uint32(m_charmInfo->GetActionBarEntry(i)->GetAction()) << " ";
        };

        ss  << "', "
            << time(NULL) << ", "
            << uint32(m_resetTalentsCost) << ", "
            << uint64(m_resetTalentsTime) << ", "
            << GetUInt32Value(UNIT_CREATED_BY_SPELL) << ", "
            << uint32(getPetType()) << ")";

        trans->Append(ss.str().c_str());

        // save auras before possibly removing them
        _SaveAuras(trans);

        // stable and not in slot saves
        if (mode > PET_SLOT_HUNTER_LAST && getPetType() == HUNTER_PET)
            RemoveAllAuras();

        _SaveSpells(trans);
        _SaveSpellCooldowns(trans);

        CharacterDatabase.CommitTransaction(trans);
    }
    // delete
    else
    {
        // warlocks pets doesn't get deleted
        if (pOwner->getClass() != CLASS_WARLOCK)
        {
            if(pOwner->m_currentPetSlot >= PET_SLOT_HUNTER_FIRST && pOwner->m_currentPetSlot <= PET_SLOT_HUNTER_LAST)
                pOwner->setPetSlotUsed(pOwner->m_currentPetSlot, false);
            RemoveAllAuras();
            DeleteFromDB(m_charmInfo->GetPetNumber());
        }
    }
}

void Pet::DeleteFromDB(uint32 guidlow)
{
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    trans->PAppend("DELETE FROM character_pet_declinedname WHERE id = '%u'", guidlow);
    trans->PAppend("DELETE FROM pet_aura WHERE guid = '%u'", guidlow);
    trans->PAppend("DELETE FROM pet_spell WHERE guid = '%u'", guidlow);
    trans->PAppend("DELETE FROM pet_spell_cooldown WHERE guid = '%u'", guidlow);
    trans->PAppend("DELETE FROM character_pet WHERE id = '%u'", guidlow);
    CharacterDatabase.CommitTransaction(trans);
}

void Pet::setDeathState(DeathState s)                       // overwrite virtual Creature::setDeathState and Unit::setDeathState
{
    Creature::setDeathState(s);
    if (getDeathState() == CORPSE)
    {
        if (getPetType() == HUNTER_PET)
        {
            // pet corpse non lootable and non skinnable
            SetUInt32Value(UNIT_DYNAMIC_FLAGS, 0x00);
            RemoveFlag (UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);

            //SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
        }
    }
    else if (getDeathState() == ALIVE)
    {
        //RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
        CastPetAuras(true);
    }
}

void Pet::Update(uint32 diff)
{
    if (m_removed)                                           // pet already removed, just wait in remove queue, no updates
        return;

    if (m_loading)
        return;

    switch(m_deathState)
    {
        case CORPSE:
        {
            if (getPetType() != HUNTER_PET || m_corpseRemoveTime <= time(NULL))
            {
                Remove(PET_SLOT_ACTUAL_PET_SLOT);               //hunters' pets never get removed because of death, NEVER!
                return;
            }
            break;
        }
        case ALIVE:
        {
            // unsummon pet that lost owner
            Player* owner = GetOwner();
            if (!owner || (!IsWithinDistInMap(owner, GetMap()->GetVisibilityDistance()) && !IsPossessed()) || (isControlled() && !owner->GetPetGUID()))
            //if (!owner || (!IsWithinDistInMap(owner, GetMap()->GetVisibilityDistance()) && (owner->GetCharmGUID() && (owner->GetCharmGUID() != GetGUID()))) || (isControlled() && !owner->GetPetGUID()))
            {
                Remove(PET_SLOT_ACTUAL_PET_SLOT, true);
                return;
            }

            if (isControlled())
            {
                if (owner->GetPetGUID() != GetGUID())
                {
                    sLog->outError("Pet %u is not pet of owner %s, removed", GetEntry(), m_owner->GetName());
                    Remove(PET_SLOT_ACTUAL_PET_SLOT);
                    return;
                }
            }

            if (m_duration > 0)
            {
                if (uint32(m_duration) > diff)
                    m_duration -= diff;
                else
                {
                    Remove(PET_SLOT_ACTUAL_PET_SLOT);
                    return;
                }
            }

            //regenerate focus for hunter pets or energy for deathknight's ghoul
            // in Creature::Update
            /*
            if (m_regenTimer)
            {
                if (m_regenTimer > int32(diff))
                    m_regenTimer -= diff;
                else
                {
                    switch (getPowerType())
                    {
                        case POWER_FOCUS:
                            Regenerate(POWER_FOCUS);
                            m_regenTimer += PET_FOCUS_REGEN_INTERVAL - diff;
                            if (m_regenTimer <= 0) m_regenTimer = 1;
                            break;
                        case POWER_ENERGY:
                            Regenerate(POWER_ENERGY);
                            m_regenTimer += CREATURE_REGEN_INTERVAL - diff;
                            if (!m_regenTimer) ++m_regenTimer;
                            break;
                        default:
                            m_regenTimer = 0;
                            break;
                    }
                }
            }
            */

            break;
        }
        default:
            break;
    }
    Creature::Update(diff);
}

void Creature::Regenerate(Powers power, uint32 diff)
{
    int32 curValue = GetPower(power);
    int32 maxValue = GetMaxPower(power);

    uint32 powerIndex = GetPowerIndex(power);
    if (power == MAX_POWERS)
        return;

    float addvalue = 0.0f;

    switch (power)
    {
        case POWER_FOCUS:
        {
            // For hunter pets.
            addvalue = 5.0f * diff / IN_MILLISECONDS * sWorld->getRate(RATE_POWER_FOCUS);      // 5 per second
            break;
        }
        case POWER_ENERGY:
        {
            // For deathknight's ghoul.
            addvalue = 10.0f * diff / IN_MILLISECONDS * sWorld->getRate(RATE_POWER_ENERGY);      // 10 per second
            break;
        }
        case POWER_MANA:
        {
            // TODO: use correct formula, this one gives really tiny regeneration
            // TODO: use UNIT_FIELD_POWER_REGEN_FLAT_MODIFIER and UNIT_FIELD_POWER_REGEN_INTERRUPTED_FLAT_MODIFIER
                // so client will display the regeneration smoothly
            if (IsInCombat() || GetCharmerOrOwnerGUID())
            {
                float ManaIncreaseRate = sWorld->getRate(RATE_POWER_MANA);
                float Spirit = GetStat(STAT_SPIRIT);

                addvalue = (Spirit/5.0f + 17.0f) * ManaIncreaseRate * diff;
            }
            else
                addvalue = maxValue * float(diff) / 3;

            addvalue /= CREATURE_REGEN_INTERVAL;
            break;
        }
        default:
            return;
    }

    // Apply modifiers (if any).
    AuraEffectList const& ModPowerRegenPCTAuras = GetAuraEffectsByType(SPELL_AURA_MOD_POWER_REGEN_PERCENT);
    for (AuraEffectList::const_iterator i = ModPowerRegenPCTAuras.begin(); i != ModPowerRegenPCTAuras.end(); ++i)
        if (Powers((*i)->GetMiscValue()) == power)
            addvalue *= ((*i)->GetAmount() + 100) / 100.0f;

    addvalue += GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_POWER_REGEN, power) * diff / (5 * IN_MILLISECONDS);

    addvalue += m_powerFraction;
    int32 addIntValue = floor(addvalue);
    m_powerFraction = addvalue - addIntValue;

    int32 newValue = curValue + addIntValue;

    if (newValue < 0)
        newValue = 0;
    else if (newValue >= maxValue)
    {
        newValue = maxValue;
        m_powerFraction = 0;
    }

    if (m_regenTimer >= CREATURE_REGEN_INTERVAL)
        ModifyPower(power, newValue - curValue);
    else
        UpdateUInt32Value(UNIT_FIELD_POWER1 + powerIndex, newValue);
}

bool Pet::CanTakeMoreActiveSpells(uint32 spellid)
{
    uint8  activecount = 1;
    uint32 chainstartstore[ACTIVE_SPELLS_MAX];

    if (IsPassiveSpell(spellid))
        return true;

    chainstartstore[0] = sSpellMgr->GetFirstSpellInChain(spellid);

    for (PetSpellMap::const_iterator itr = m_spells.begin(); itr != m_spells.end(); ++itr)
    {
        if (itr->second.state == PETSPELL_REMOVED)
            continue;

        if (IsPassiveSpell(itr->first))
            continue;

        uint32 chainstart = sSpellMgr->GetFirstSpellInChain(itr->first);

        uint8 x;

        for (x = 0; x < activecount; x++)
        {
            if (chainstart == chainstartstore[x])
                break;
        }

        if (x == activecount)                                //spellchain not yet saved -> add active count
        {
            ++activecount;
            if (activecount > ACTIVE_SPELLS_MAX)
                return false;
            chainstartstore[x] = chainstart;
        }
    }
    return true;
}

void Pet::Remove(PetSlot mode, bool returnreagent)
{
    m_owner->RemovePet(this,mode,returnreagent);
}

void Pet::GivePetXP(uint32 xp)
{
    if (getPetType() != HUNTER_PET)
        return;

    if (xp < 1)
        return;

    if (!IsAlive())
        return;
    
    uint8 maxlevel = std::min((uint8)sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL), GetOwner()->getLevel());
    uint8 petlevel = getLevel();

    // If pet is detected to be at, or above(?) the players level, don't hand out XP
    if (petlevel >= maxlevel)
       return;

    uint32 nextLvlXP = GetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP);
    uint32 curXP = GetUInt32Value(UNIT_FIELD_PETEXPERIENCE);
    uint32 newXP = curXP + xp;

    // Check how much XP the pet should receive, and hand off have any left from previous levelups
    while (newXP >= nextLvlXP && petlevel < maxlevel)
    {
        // Subtract newXP from amount needed for nextlevel, and give pet the level
        newXP -= nextLvlXP;
        ++petlevel;

        GivePetLevel(petlevel);

        nextLvlXP = GetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP);
    }
    // Not affected by special conditions - give it new XP
    SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, petlevel < maxlevel ? newXP : 0);
}

void Pet::GivePetLevel(uint8 level)
{
    if (!level || level == getLevel())
        return;

    if (getPetType()==HUNTER_PET)
    {
        SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, 0);
        SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, uint32(sObjectMgr->GetXPForLevel(level)*PET_XP_FACTOR));
    }

    InitStatsForLevel(level);
    InitLevelupSpellsForLevel();
    InitTalentForLevel();
}

bool Pet::CreateBaseAtCreature(Creature* creature)
{
    ASSERT(creature);

    if (!CreateBaseAtTamed(creature->GetCreatureInfo(), creature->GetMap(), creature->GetPhaseMask()))
        return false;

    Relocate(creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ(), creature->GetOrientation());

    if (!IsPositionValid())
    {
        sLog->outError("Pet (guidlow %d, entry %d) not created base at creature. Suggested coordinates isn't valid (X: %f Y: %f)",
            GetGUIDLow(), GetEntry(), GetPositionX(), GetPositionY());
        return false;
    }

    CreatureInfo const *cinfo = GetCreatureInfo();
    if (!cinfo)
    {
        sLog->outError("CreateBaseAtCreature() failed, creatureInfo is missing!");
        return false;
    }

    SetDisplayId(creature->GetDisplayId());

    if (CreatureFamilyEntry const* cFamily = sCreatureFamilyStore.LookupEntry(cinfo->family))
        SetName(cFamily->Name);
    else
        SetName(creature->GetNameForLocaleIdx(sObjectMgr->GetDBCLocaleIndex()));

    return true;
}

bool Pet::CreateBaseAtCreatureInfo(CreatureInfo const* cinfo, Unit * owner)
{
    if (!CreateBaseAtTamed(cinfo, owner->GetMap(), owner->GetPhaseMask()))
        return false;

    if (CreatureFamilyEntry const* cFamily = sCreatureFamilyStore.LookupEntry(cinfo->family))
        SetName(cFamily->Name);

    Relocate(owner->GetPositionX(), owner->GetPositionY(), owner->GetPositionZ(), owner->GetOrientation());

    return true;
}

bool Pet::CreateBaseAtTamed(CreatureInfo const * cinfo, Map * map, uint32 phaseMask)
{
    sLog->outDebug("Pet::CreateBaseForTamed");
    uint32 pet_number = sObjectMgr->GeneratePetNumber();
    if (!Create(pet_number, map, phaseMask, cinfo->Entry))
        return false;

    SetMaxPower(POWER_HAPPINESS, GetCreatePowers(POWER_HAPPINESS));
    SetPower(POWER_HAPPINESS, 166500);
    setPowerType(POWER_FOCUS);
    SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, 0);
    SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, 0);
    SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, uint32(sObjectMgr->GetXPForLevel(getLevel()+1)*PET_XP_FACTOR));
    SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);

    if (cinfo->type == CREATURE_TYPE_BEAST)
    {
        SetUInt32Value(UNIT_FIELD_BYTES_0, 0x02020100);
        SetSheath(SHEATH_STATE_MELEE);
        SetByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED | UNIT_CAN_BE_ABANDONED);
    }

    return true;
}

// TODO: Move stat mods code to pet passive auras
bool Guardian::InitStatsForLevel(uint8 petlevel)
{
    CreatureInfo const *cinfo = GetCreatureInfo();
    ASSERT(cinfo);

    SetLevel(petlevel);

    //Determine pet type
    PetType petType = MAX_PET_TYPE;
    if (IsPet() && m_owner->GetTypeId() == TYPEID_PLAYER)
    {
        if ((m_owner->getClass() == CLASS_WARLOCK)
            || (m_owner->getClass() == CLASS_SHAMAN)        // Fire Elemental
            || (m_owner->getClass() == CLASS_PRIEST))        // Shadowfiend
            petType = SUMMON_PET;
        else if (m_owner->getClass() == CLASS_HUNTER)
        {
            petType = HUNTER_PET;
            m_unitTypeMask |= UNIT_MASK_HUNTER_PET;
        }
        else if (m_owner->getClass() == CLASS_WARLOCK)
        {
            m_unitTypeMask |= UNIT_MASK_WARLOCK_PET;
        }
        else if (m_owner->getClass() == CLASS_DEATH_KNIGHT) // Ghoul
        {
            // It's there also to avoid showing error
            // Not all ghouls are guardians :)
            setPowerType(POWER_ENERGY);
            SetMaxPower(POWER_ENERGY, 100);
            SetPower(POWER_ENERGY, 100);
        }
        else if (m_owner->getClass() == CLASS_MAGE && GetEntry() == 510) // Water elemental
        {
            // no additional things needed?
        }
        else
            sLog->outError("Unknown type pet %u is summoned by player class %u", GetEntry(), m_owner->getClass());
    }

    uint32 creature_ID = (petType == HUNTER_PET) ? 1 : cinfo->Entry;

    SetMeleeDamageSchool(SpellSchools(cinfo->dmgschool));

    SetModifierValue(UNIT_MOD_ARMOR, BASE_VALUE, float(petlevel*50));

    SetAttackTime(BASE_ATTACK, BASE_ATTACK_TIME);
    SetAttackTime(OFF_ATTACK, BASE_ATTACK_TIME);
    SetAttackTime(RANGED_ATTACK, BASE_ATTACK_TIME);

    SetFloatValue(UNIT_MOD_CAST_SPEED, 1.0f);
    SetFloatValue(UNIT_MOD_CAST_HASTE, 1.0f);

    //scale
    CreatureFamilyEntry const* cFamily = sCreatureFamilyStore.LookupEntry(cinfo->family);
    if (cFamily && cFamily->minScale > 0.0f && petType == HUNTER_PET)
    {
        float scale;
        if (getLevel() >= cFamily->maxScaleLevel)
            scale = cFamily->maxScale;
        else if (getLevel() <= cFamily->minScaleLevel)
            scale = cFamily->minScale;
        else
            scale = cFamily->minScale + float(getLevel() - cFamily->minScaleLevel) / cFamily->maxScaleLevel * (cFamily->maxScale - cFamily->minScale);

        SetFloatValue(OBJECT_FIELD_SCALE_X, scale);
    }

    //resistance
    int32 createResistance[MAX_SPELL_SCHOOL] = {0,0,0,0,0,0,0};
    if (cinfo && petType != HUNTER_PET)
    {
        createResistance[SPELL_SCHOOL_HOLY]   = cinfo->resistance1;
        createResistance[SPELL_SCHOOL_FIRE]   = cinfo->resistance2;
        createResistance[SPELL_SCHOOL_NATURE] = cinfo->resistance3;
        createResistance[SPELL_SCHOOL_FROST]  = cinfo->resistance4;
        createResistance[SPELL_SCHOOL_SHADOW] = cinfo->resistance5;
        createResistance[SPELL_SCHOOL_ARCANE] = cinfo->resistance6;
    }
    for (uint8 i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
        SetModifierValue(UnitMods(UNIT_MOD_RESISTANCE_START + i), BASE_VALUE, float(createResistance[i]));

    //health, mana, armor and resistance
    PetLevelInfo const* pInfo = sObjectMgr->GetPetLevelInfo(creature_ID, petlevel);
    if (pInfo)                                      // exist in DB
    {
        SetCreateHealth(pInfo->health);
        if (petType != HUNTER_PET) //hunter pet use focus
            SetCreateMana(pInfo->mana);

        if (pInfo->armor > 0)
            SetModifierValue(UNIT_MOD_ARMOR, BASE_VALUE, float(pInfo->armor));

        for (uint8 stat = 0; stat < MAX_STATS; ++stat)
            SetCreateStat(Stats(stat), float(pInfo->stats[stat]));
    }
    else                                            // not exist in DB, use some default fake data
    {
        // remove elite bonuses included in DB values
        CreatureBaseStats const* stats = sObjectMgr->GetCreatureBaseStats(petlevel, cinfo->unit_class);
        SetCreateHealth(stats->BaseHealth[cinfo->expansion]);
        SetCreateMana(stats->BaseMana);

        SetCreateStat(STAT_STRENGTH, 22);
        SetCreateStat(STAT_AGILITY, 22);
        SetCreateStat(STAT_STAMINA, 25);
        SetCreateStat(STAT_INTELLECT, 28);
        SetCreateStat(STAT_SPIRIT, 27);
    }

    SetBonusDamage(0);
    switch (petType)
    {
        case SUMMON_PET:
        {
            //the damage bonus used for pets is either fire or shadow damage, whatever is higher
            uint32 fire  = m_owner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + SPELL_SCHOOL_FIRE);
            uint32 shadow = m_owner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + SPELL_SCHOOL_SHADOW);
            uint32 val  = (fire > shadow) ? fire : shadow;
            SetBonusDamage(int32 (val * 0.15f));
            //bonusAP += val * 0.57;

            SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float(petlevel - (petlevel / 4)));
            SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float(petlevel + (petlevel / 4)));

            //SetModifierValue(UNIT_MOD_ATTACK_POWER, BASE_VALUE, float(cinfo->attackpower));

            switch (GetEntry())
            {
                case 19668: // Shadowfiend
                {
                    if (!pInfo)
                    {
                        SetCreateMana(28 + 10 * petlevel);
                        SetCreateHealth(28 + 30 * petlevel);
                    }
                    // 1.5s attack time
                    SetAttackTime(BASE_ATTACK, cinfo->baseattacktime);
                    int32 bonus_dmg = (int32(m_owner->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_SHADOW)* 0.3f));
                    SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float((petlevel * 4 - petlevel) + bonus_dmg));
                    SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float((petlevel * 4 + petlevel) + bonus_dmg));
                    break;
                }
            }
            break;
        }
        case HUNTER_PET:
        {
            SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, uint32(sObjectMgr->GetXPForLevel(petlevel)*PET_XP_FACTOR));
            //these formula may not be correct; however, it is designed to be close to what it should be
            //this makes dps 0.5 of pets level
            SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float(petlevel - (petlevel / 4)));
            //damage range is then petlevel / 2
            SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float(petlevel + (petlevel / 4)));
            //damage is increased afterwards as strength and pet scaling modify attack power
            break;
        }
        default:
        {
            switch (GetEntry())
            {
                case 510: // mage Water Elemental
                {
                    if (m_owner->getLevel() > 80)
                    {
                        SetCreateHealth(14900+9200.0f*(m_owner->getLevel()-80)/5.0f);
                        SetCreateMana(9800+6200.0f*(m_owner->getLevel()-80)/5.0f);
                    }
                    SetBonusDamage(int32(m_owner->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_FROST) * 0.33f));
                    SetModifierValue(UNIT_MOD_STAT_STAMINA, BASE_VALUE, float(m_owner->GetStat(STAT_STAMINA)) * 0.3f);  //  Bonus Stamina (30% of player stamina)
                    SetModifierValue(UNIT_MOD_STAT_INTELLECT, BASE_VALUE, float(m_owner->GetStat(STAT_INTELLECT)) * 0.3f);  //  Bonus Stamina (30% of player stamina)
                    SetModifierValue(UNIT_MOD_ARMOR, BASE_VALUE, float(m_owner->GetArmor()) * 0.35f);  //  Bonus Armor (35% of player armor)
                    break;
                }
                case 1964: //force of nature
                {
                    if (!pInfo)
                        SetCreateHealth(30 + 30*petlevel);
                    float bonusDmg = m_owner->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_NATURE) * 0.15f;
                    SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float(petlevel * 2.5f - (petlevel / 2) + bonusDmg));
                    SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float(petlevel * 2.5f + (petlevel / 2) + bonusDmg));
                    break;
                }
                case 15352: //earth elemental 36213
                {
                    if (!pInfo)
                        SetCreateHealth(100 + 120*petlevel);
                    SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float(petlevel - (petlevel / 4)));
                    SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float(petlevel + (petlevel / 4)));
                    break;
                }
                case 15438: //fire elemental
                {
                    if (!pInfo)
                    {
                        SetCreateHealth(40*petlevel);
                        SetCreateMana(28 + 10*petlevel);
                    }
                    SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float(petlevel * 4 - petlevel));
                    SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float(petlevel * 4 + petlevel));
                    break;
                }
                case 28017: // Bloodworms
                {
                    SetCreateHealth(m_owner->GetMaxHealth()*0.20f);
                    SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float(petlevel - 30 - (petlevel / 4)) + m_owner->GetTotalAttackPowerValue(BASE_ATTACK) * 0.006f);
                    SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float(petlevel - 30 + (petlevel / 4)) + m_owner->GetTotalAttackPowerValue(BASE_ATTACK) * 0.006f);
                    break;
                }
                case 19833: //Snake Trap - Venomous Snake
                {
                    SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float((petlevel / 2) - 25));
                    SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float((petlevel / 2) - 18));
                    break;
                }
                case 19921: //Snake Trap - Viper
                {
                    SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float(petlevel / 2 - 10));
                    SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float(petlevel / 2));
                    break;
                }
                case 26125: // Risen Ghoul
                {
                    SetFloatValue(UNIT_MOD_CAST_SPEED, m_owner->GetFloatValue(UNIT_MOD_CAST_SPEED));
                    SetFloatValue(UNIT_MOD_CAST_HASTE, m_owner->GetFloatValue(UNIT_MOD_CAST_HASTE));
                    m_modMeleeHitChance = (float)((int32)m_owner->m_modMeleeHitChance);
                    m_modRangedHitChance = (float)((int32)m_owner->m_modRangedHitChance);
                    break;
                }
                case 29264: // Feral Spirit
                {
                    if (!pInfo)
                        SetCreateHealth(30*petlevel);

                    float dmg_multiplier = 0.3f;
                    if (m_owner->GetAuraEffect(63271, 0)) // Glyph of Feral Spirit
                        dmg_multiplier = 0.6f;

                    // wolf attack speed is 1.5s
                    SetAttackTime(BASE_ATTACK, cinfo->baseattacktime);
                    SetBonusDamage(int32(m_owner->GetTotalAttackPowerValue(BASE_ATTACK) * dmg_multiplier));

                    // 14AP == 1dps, wolf's strike speed == 2s so dmg = basedmg + AP / 14 * 2
                    SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE,float((petlevel * 4 - petlevel) + (m_owner->GetTotalAttackPowerValue(BASE_ATTACK) * dmg_multiplier * 2 / 14)));
                    SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE,float((petlevel * 4 + petlevel) + (m_owner->GetTotalAttackPowerValue(BASE_ATTACK) * dmg_multiplier * 2 / 14)));

                    SetModifierValue(UNIT_MOD_ARMOR, BASE_VALUE, float(m_owner->GetArmor()) * 0.35f);  //  Bonus Armor (35% of player armor)
                    SetModifierValue(UNIT_MOD_STAT_STAMINA, BASE_VALUE,float(m_owner->GetStat(STAT_STAMINA)) * 0.3f);  //  Bonus Stamina (30% of player stamina)
                    if (!HasAura(58877))//prevent apply twice for the 2 wolves
                        AddAura(58877, this);//Spirit Hunt, passive, Spirit Wolves' attacks heal them and their master for 150% of damage done.
                    break;
                }
                case 31216: // Mirror Image
                case 53438: // Mirror Image from T12 2P set
                {
                    SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_MAGE);

                    SpellSchoolMask srcSchool = SPELL_SCHOOL_MASK_FROST;
                    if (m_owner && m_owner->GetTypeId() == TYPEID_PLAYER)
                    {
                        if (m_owner->ToPlayer()->GetActiveTalentBranchSpec() == SPEC_MAGE_ARCANE)
                            srcSchool = SPELL_SCHOOL_MASK_ARCANE;
                        else if (m_owner->ToPlayer()->GetActiveTalentBranchSpec() == SPEC_MAGE_FIRE)
                            srcSchool = SPELL_SCHOOL_MASK_FIRE;
                    }
                    SetBonusDamage(int32(m_owner->SpellBaseDamageBonusDone(srcSchool) * 0.33f));

                    SetDisplayId(m_owner->GetDisplayId());
                    if (!pInfo)
                    {
                        SetCreateMana(28 + 30*petlevel);
                        SetCreateHealth(28 + 10*petlevel);
                    }
                    break;
                }
                case 27829: // Ebon Gargoyle
                {
                    if (!pInfo)
                    {
                        SetCreateMana(28 + 10 * petlevel);
                        SetCreateHealth(28 + 30 * petlevel);
                    }

                    if (Player *owner = m_owner->ToPlayer()) // get 100% of owning player's haste
                    {
                        float bonus = owner->GetRatingBonusValue(CR_HASTE_MELEE);
                        bonus += owner->GetTotalAuraModifier(SPELL_AURA_MOD_MELEE_HASTE) + owner->GetTotalAuraModifier(SPELL_AURA_MOD_MELEE_RANGED_HASTE);
                        bonus = bonus >= 25 ? 25 : bonus; // Limit to maxium 1.5s cast time
                        ApplyCastTimePercentMod(bonus, true);
                        SetCreateHealth(uint32(owner->GetMaxHealth()*0.8)); // hp must be 0.8x of DK hp
                    }

                    SetBonusDamage(int32(m_owner->GetTotalAttackPowerValue(BASE_ATTACK) * 0.5f));
                    SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float(petlevel - (petlevel / 4)));
                    SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float(petlevel + (petlevel / 4)));
                    break;
                }
                case 50675: // Ebon Imp (from Bane of Doom)
                {
                    // Mostly custom values, who cares about these stupid little imps
                    SetCreateHealth(petlevel*petlevel/2);
                    SetBonusDamage(int32(m_owner->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_FIRE) * 0.33f));
                    SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float(int32(m_owner->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_FIRE) * 0.05f)));
                    SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float(int32(m_owner->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_FIRE) * 0.10f)));
                    break;
                }
            }
            break;
        }
    }

    UpdateAllStats();

    SetFullHealth();
    SetPower(POWER_MANA, GetMaxPower(POWER_MANA));
    return true;
}

bool Pet::HaveInDiet(ItemPrototype const* item) const
{
    if (!item->FoodType)
        return false;

    CreatureInfo const* cInfo = GetCreatureInfo();
    if (!cInfo)
        return false;

    CreatureFamilyEntry const* cFamily = sCreatureFamilyStore.LookupEntry(cInfo->family);
    if (!cFamily)
        return false;

    uint32 diet = cFamily->petFoodMask;
    uint32 FoodMask = 1 << (item->FoodType-1);
    return diet & FoodMask;
}

uint32 Pet::GetCurrentFoodBenefitLevel(uint32 itemlevel)
{
    // -5 or greater food level
    if (getLevel() <= itemlevel + 5)                         //possible to feed level 60 pet with level 55 level food for full effect
        return 35000;
    // -10..-6
    else if (getLevel() <= itemlevel + 10)                   //pure guess, but sounds good
        return 17000;
    // -14..-11
    else if (getLevel() <= itemlevel + 14)                   //level 55 food gets green on 70, makes sense to me
        return 8000;
    // -15 or less
    else
        return 0;                                           //food too low level
}

void Pet::_LoadSpellCooldowns()
{
    m_CreatureSpellCooldowns.clear();
    m_CreatureCategoryCooldowns.clear();

    QueryResult result = CharacterDatabase.PQuery("SELECT spell,time FROM pet_spell_cooldown WHERE guid = '%u'",m_charmInfo->GetPetNumber());

    if (result)
    {
        time_t curTime = time(NULL);

        WorldPacket data(SMSG_SPELL_COOLDOWN, size_t(8+1+result->GetRowCount()*8));
        data << GetGUID();
        data << uint8(0x0);                                 // flags (0x1, 0x2)

        do
        {
            Field *fields = result->Fetch();

            uint32 spell_id = fields[0].GetUInt32();
            time_t db_time  = (time_t)fields[1].GetUInt64();

            if (!sSpellStore.LookupEntry(spell_id))
            {
                sLog->outError("Pet %u have unknown spell %u in `pet_spell_cooldown`, skipping.",m_charmInfo->GetPetNumber(),spell_id);
                continue;
            }

            // skip outdated cooldown
            if (db_time <= curTime)
                continue;

            uint32 duration = uint32(db_time - curTime) * IN_MILLISECONDS;
            data << uint32(spell_id);
            data << uint32(duration);

            _AddCreatureSpellCooldown(spell_id, duration);

            sLog->outDebug("Pet (Number: %u) spell %u cooldown loaded (%u secs).", m_charmInfo->GetPetNumber(), spell_id, uint32(db_time-curTime));
        }
        while (result->NextRow());

        if (!m_CreatureSpellCooldowns.empty() && GetOwner())
            ((Player*)GetOwner())->GetSession()->SendPacket(&data);
    }
}

void Pet::_SaveSpellCooldowns(SQLTransaction& trans)
{
    trans->PAppend("DELETE FROM pet_spell_cooldown WHERE guid = '%u'", m_charmInfo->GetPetNumber());

    time_t curTime = time(NULL);

    // remove oudated and save active
    for (CreatureSpellCooldowns::iterator itr = m_CreatureSpellCooldowns.begin(); itr != m_CreatureSpellCooldowns.end();)
    {
        if (itr->second <= m_LogonTimer)
            m_CreatureSpellCooldowns.erase(itr++);
        else
        {
            time_t cdEnd = curTime + (itr->second - m_LogonTimer) / IN_MILLISECONDS;
            trans->PAppend("INSERT INTO pet_spell_cooldown (guid,spell,time) VALUES ('%u', '%u', '" UI64FMTD "')", m_charmInfo->GetPetNumber(), itr->first, uint64(cdEnd));
            ++itr;
        }
    }
}

void Pet::_LoadSpells()
{
    QueryResult result = CharacterDatabase.PQuery("SELECT spell,active FROM pet_spell WHERE guid = '%u'",m_charmInfo->GetPetNumber());

    if (result)
    {
        do
        {
            Field *fields = result->Fetch();

            AddSpell(fields[0].GetUInt32(), ActiveStates(fields[1].GetUInt8()), PETSPELL_UNCHANGED);
        }
        while (result->NextRow());
    }
}

void Pet::_SaveSpells(SQLTransaction& trans)
{
    for (PetSpellMap::iterator itr = m_spells.begin(), next = m_spells.begin(); itr != m_spells.end(); itr = next)
    {
        ++next;

        // prevent saving family passives to DB
        if (itr->second.type == PETSPELL_FAMILY)
            continue;

        switch (itr->second.state)
        {
            case PETSPELL_REMOVED:
                trans->PAppend("DELETE FROM pet_spell WHERE guid = '%u' and spell = '%u'", m_charmInfo->GetPetNumber(), itr->first);
                m_spells.erase(itr);
                continue;
            case PETSPELL_CHANGED:
                trans->PAppend("DELETE FROM pet_spell WHERE guid = '%u' and spell = '%u'", m_charmInfo->GetPetNumber(), itr->first);
                trans->PAppend("INSERT INTO pet_spell (guid,spell,active) VALUES ('%u', '%u', '%u')", m_charmInfo->GetPetNumber(), itr->first, itr->second.active);
                break;
            case PETSPELL_NEW:
                trans->PAppend("INSERT INTO pet_spell (guid,spell,active) VALUES ('%u', '%u', '%u')", m_charmInfo->GetPetNumber(), itr->first, itr->second.active);
                break;
            case PETSPELL_UNCHANGED:
                continue;
        }
        itr->second.state = PETSPELL_UNCHANGED;
    }
}

void Pet::_LoadAuras(uint32 timediff)
{
    sLog->outDebug("Loading auras for pet %u",GetGUIDLow());

    QueryResult result = CharacterDatabase.PQuery("SELECT caster_guid,spell,effect_mask,recalculate_mask,stackcount,amount0,amount1,amount2,base_amount0,base_amount1,base_amount2,maxduration,remaintime,remaincharges FROM pet_aura WHERE guid = '%u'",m_charmInfo->GetPetNumber());

    if (result)
    {
        do
        {
            int32 damage[3];
            int32 baseDamage[3];
            Field *fields = result->Fetch();
            uint64 caster_guid = fields[0].GetUInt64();
            uint32 spellid = fields[1].GetUInt32();
            uint8 effmask = fields[2].GetUInt8();
            uint8 recalculatemask = fields[3].GetUInt8();
            uint8 stackcount = fields[4].GetUInt8();
            damage[0] = fields[5].GetInt32();
            damage[1] = fields[6].GetInt32();
            damage[2] = fields[7].GetInt32();
            baseDamage[0] = fields[8].GetInt32();
            baseDamage[1] = fields[9].GetInt32();
            baseDamage[2] = fields[10].GetInt32();
            int32 maxduration = fields[11].GetInt32();
            int32 remaintime = fields[12].GetInt32();
            uint8 remaincharges = fields[13].GetUInt8();

            SpellEntry const* spellproto = sSpellStore.LookupEntry(spellid);
            if (!spellproto)
            {
                sLog->outError("Unknown aura (spellid %u), ignore.",spellid);
                continue;
            }

            // negative effects should continue counting down after logout
            if (remaintime != -1 && !IsPositiveSpell(spellid))
            {
                if (remaintime/IN_MILLISECONDS <= int32(timediff))
                    continue;

                remaintime -= timediff*IN_MILLISECONDS;
            }

            // prevent wrong values of remaincharges
            if (spellproto->procCharges)
            {
                if (remaincharges <= 0 || remaincharges > spellproto->procCharges)
                    remaincharges = spellproto->procCharges;
            }
            else
                remaincharges = 0;

            if (Aura * aura = Aura::TryCreate(spellproto, effmask, this, NULL, &baseDamage[0], NULL, NULL, caster_guid))
            {
                if (!aura->CanBeSaved())
                {
                    aura->Remove();
                    continue;
                }
                aura->SetLoadedState(maxduration,remaintime,remaincharges,stackcount,recalculatemask,&damage[0]);
                aura->ApplyForTargets();
                sLog->outDetail("Added aura spellid %u, effectmask %u", spellproto->Id, effmask);
            }
        }
        while (result->NextRow());
    }
}

void Pet::_SaveAuras(SQLTransaction& trans)
{
    trans->PAppend("DELETE FROM pet_aura WHERE guid = '%u'", m_charmInfo->GetPetNumber());

    for (AuraMap::const_iterator itr = m_ownedAuras.begin(); itr != m_ownedAuras.end() ; ++itr)
    {
        // check if the aura has to be saved
        if (!itr->second->CanBeSaved() || IsPetAura(itr->second))
            continue;

        Aura * aura = itr->second;

        int32 damage[MAX_SPELL_EFFECTS];
        int32 baseDamage[MAX_SPELL_EFFECTS];
        uint8 effMask = 0;
        uint8 recalculateMask = 0;
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (aura->GetEffect(i))
            {
                baseDamage[i] = aura->GetEffect(i)->GetBaseAmount();
                damage[i] = aura->GetEffect(i)->GetAmount();
                effMask |= (1<<i);
                if (aura->GetEffect(i)->CanBeRecalculated())
                    recalculateMask |= (1<<i);
            }
            else
            {
                baseDamage[i] = 0;
                damage[i] = 0;
            }
        }

        trans->PAppend("INSERT INTO pet_aura (guid,caster_guid,spell,effect_mask,recalculate_mask,stackcount,amount0,amount1,amount2,base_amount0,base_amount1,base_amount2,maxduration,remaintime,remaincharges) "
            "VALUES ('%u', '" UI64FMTD "', '%u', '%u', '%u', '%u', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%u')",
            m_charmInfo->GetPetNumber(), itr->second->GetCasterGUID(), itr->second->GetId(), effMask, recalculateMask,
            itr->second->GetStackAmount(), damage[0], damage[1], damage[2], baseDamage[0], baseDamage[1], baseDamage[2],
            itr->second->GetMaxDuration(), itr->second->GetDuration(),itr->second->GetCharges());
    }
}

bool Pet::AddSpell(uint32 spell_id,ActiveStates active /*= ACT_DECIDE*/, PetSpellState state /*= PETSPELL_NEW*/, PetSpellType type /*= PETSPELL_NORMAL*/)
{
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spell_id);
    if (!spellInfo)
    {
        // do pet spell book cleanup
        if (state == PETSPELL_UNCHANGED)                    // spell load case
        {
            sLog->outError("Pet::addSpell: Non-existed in SpellStore spell #%u request, deleting for all pets in `pet_spell`.",spell_id);
            CharacterDatabase.PExecute("DELETE FROM pet_spell WHERE spell = '%u'",spell_id);
        }
        else
            sLog->outError("Pet::addSpell: Non-existed in SpellStore spell #%u request.",spell_id);

        return false;
    }

    PetSpellMap::iterator itr = m_spells.find(spell_id);
    if (itr != m_spells.end())
    {
        if (itr->second.state == PETSPELL_REMOVED)
        {
            m_spells.erase(itr);
            state = PETSPELL_CHANGED;
        }
        else if (state == PETSPELL_UNCHANGED && itr->second.state != PETSPELL_UNCHANGED)
        {
            // can be in case spell loading but learned at some previous spell loading
            itr->second.state = PETSPELL_UNCHANGED;

            if (active == ACT_ENABLED)
                ToggleAutocast(spell_id, true);
            else if (active == ACT_DISABLED)
                ToggleAutocast(spell_id, false);

            return false;
        }
        else
            return false;
    }

    PetSpell newspell;
    newspell.state = state;
    newspell.type = type;

    if (active == ACT_DECIDE)                               // active was not used before, so we save it's autocast/passive state here
    {
        if (IsAutocastableSpell(spell_id))
            newspell.active = ACT_DISABLED;
        else
            newspell.active = ACT_PASSIVE;
    }
    else
        newspell.active = active;

    // talent: unlearn all other talent ranks (high and low)
    if (TalentSpellPos const* talentPos = GetTalentSpellPos(spell_id))
    {
        if (TalentEntry const *talentInfo = sTalentStore.LookupEntry(talentPos->talent_id))
        {
            for (uint8 i = 0; i < MAX_TALENT_RANK; ++i)
            {
                // skip learning spell and no rank spell case
                uint32 rankSpellId = talentInfo->RankID[i];
                if (!rankSpellId || rankSpellId == spell_id)
                    continue;

                // skip unknown ranks
                if (!HasSpell(rankSpellId))
                    continue;
                RemoveSpell(rankSpellId,false,false);
            }
        }
    }
    else if (sSpellMgr->GetSpellRank(spell_id) != 0)
    {
        for (PetSpellMap::const_iterator itr2 = m_spells.begin(); itr2 != m_spells.end(); ++itr2)
        {
            if (itr2->second.state == PETSPELL_REMOVED) continue;

            if (sSpellMgr->IsRankSpellDueToSpell(spellInfo,itr2->first))
            {
                // replace by new high rank
                if (sSpellMgr->IsHighRankOfSpell(spell_id,itr2->first))
                {
                    newspell.active = itr2->second.active;

                    if (newspell.active == ACT_ENABLED)
                        ToggleAutocast(itr2->first, false);

                    unLearnSpell(itr2->first,false,false);
                    break;
                }
                // ignore new lesser rank
                else if (sSpellMgr->IsHighRankOfSpell(itr2->first,spell_id))
                    return false;
            }
        }
    }

    m_spells[spell_id] = newspell;

    if (IsPassiveSpell(spell_id) && (!spellInfo->CasterAuraState || HasAuraState(AuraState(spellInfo->CasterAuraState))))
        CastSpell(this, spell_id, true);
    else
        m_charmInfo->AddSpellToActionBar(spell_id);

    if (newspell.active == ACT_ENABLED)
        ToggleAutocast(spell_id, true);

    uint32 talentCost = GetTalentSpellCost(spell_id);
    if (talentCost)
    {
        int32 free_points = GetMaxTalentPointsForLevel(getLevel());
        m_usedTalentCount += talentCost;
        // update free talent points
        free_points-=m_usedTalentCount;
        SetFreeTalentPoints(free_points > 0 ? free_points : 0);
    }
    return true;
}

bool Pet::LearnSpell(uint32 spell_id)
{
    // prevent duplicated entires in spell book
    if (!AddSpell(spell_id))
        return false;

    if (!m_loading)
    {
        WorldPacket data(SMSG_PET_LEARNED_SPELL, 4);
        data << uint32(spell_id);
        m_owner->GetSession()->SendPacket(&data);
        m_owner->PetSpellInitialize();
    }
    return true;
}

void Pet::InitLevelupSpellsForLevel()
{
    uint8 level = getLevel();

    // Death Knight's ghoul
    if (GetEntry() == 26125)
    {
        LearnSpell(47468); // Claw
        LearnSpell(47481); // Gnaw
        LearnSpell(47484); // Huddle
        LearnSpell(47482); // Leap
    }

    if (PetLevelupSpellSet const *levelupSpells = GetCreatureInfo()->family ? sSpellMgr->GetPetLevelupSpellList(GetCreatureInfo()->family) : NULL)
    {
        // PetLevelupSpellSet ordered by levels, process in reversed order
        for (PetLevelupSpellSet::const_reverse_iterator itr = levelupSpells->rbegin(); itr != levelupSpells->rend(); ++itr)
        {
            // will called first if level down
            if (itr->first > level)
                unLearnSpell(itr->second,true);                 // will learn prev rank if any
            // will called if level up
            else
                LearnSpell(itr->second);                        // will unlearn prev rank if any
        }
    }

    int32 petSpellsId = GetCreatureInfo()->PetSpellDataId ? -(int32)GetCreatureInfo()->PetSpellDataId : GetEntry();

    // default spells (can be not learned if pet level (as owner level decrease result for example) less first possible in normal game)
    if (PetDefaultSpellsEntry const *defSpells = sSpellMgr->GetPetDefaultSpellsEntry(petSpellsId))
    {
        for (uint8 i = 0; i < MAX_CREATURE_SPELL_DATA_SLOT; ++i)
        {
            SpellEntry const* spellEntry = sSpellStore.LookupEntry(defSpells->spellid[i]);
            if (!spellEntry)
                continue;

            // will called first if level down
            if (spellEntry->spellLevel > level)
                unLearnSpell(spellEntry->Id,true);
            // will called if level up
            else
                LearnSpell(spellEntry->Id);
        }
    }
}

bool Pet::unLearnSpell(uint32 spell_id, bool learn_prev, bool clear_ab)
{
    if (RemoveSpell(spell_id,learn_prev,clear_ab))
    {
        if (!m_loading)
        {
            WorldPacket data(SMSG_PET_REMOVED_SPELL, 4);
            data << uint32(spell_id);
            m_owner->GetSession()->SendPacket(&data);
        }
        return true;
    }
    return false;
}

bool Pet::RemoveSpell(uint32 spell_id, bool learn_prev, bool clear_ab)
{
    PetSpellMap::iterator itr = m_spells.find(spell_id);
    if (itr == m_spells.end())
        return false;

    if (itr->second.state == PETSPELL_REMOVED)
        return false;

    if (itr->second.state == PETSPELL_NEW)
        m_spells.erase(itr);
    else
        itr->second.state = PETSPELL_REMOVED;

    RemoveAurasDueToSpell(spell_id);

    uint32 talentCost = GetTalentSpellCost(spell_id);
    if (talentCost > 0)
    {
        if (m_usedTalentCount > talentCost)
            m_usedTalentCount -= talentCost;
        else
            m_usedTalentCount = 0;
        // update free talent points
        int32 free_points = GetMaxTalentPointsForLevel(getLevel()) - m_usedTalentCount;
        SetFreeTalentPoints(free_points > 0 ? free_points : 0);
    }

    if (learn_prev)
    {
        if (uint32 prev_id = sSpellMgr->GetPrevSpellInChain (spell_id))
            LearnSpell(prev_id);
        else
            learn_prev = false;
    }

    // if remove last rank or non-ranked then update action bar at server and client if need
    if (clear_ab && !learn_prev && m_charmInfo->RemoveSpellFromActionBar(spell_id))
    {
        if (!m_loading)
        {
            // need update action bar for last removed rank
            if (Unit* owner = GetOwner())
                if (owner->GetTypeId() == TYPEID_PLAYER)
                    owner->ToPlayer()->PetSpellInitialize();
        }
    }

    return true;
}

void Pet::CleanupActionBar()
{
    for (uint8 i = 0; i < MAX_UNIT_ACTION_BAR_INDEX; ++i)
        if (UnitActionBarEntry const* ab = m_charmInfo->GetActionBarEntry(i))
            if (ab->GetAction() && ab->IsActionBarForSpell())
            {
                if (!HasSpell(ab->GetAction()))
                    m_charmInfo->SetActionBar(i, 0, ACT_PASSIVE);
                else if (ab->GetType() == ACT_ENABLED)
                    ToggleAutocast(ab->GetAction(), true);
            }
}

void Pet::InitPetCreateSpells()
{
    m_charmInfo->InitPetActionBar();
    m_spells.clear();

    // this is there to avoid massive "your pet has learned ..." spam
    bool loadingState = m_loading;
    m_loading = true;

    LearnPetPassives();
    InitLevelupSpellsForLevel();

    m_loading = loadingState;

    CastPetAuras(false);
}

bool Pet::ResetTalents(bool no_cost)
{
    Unit *owner = GetOwner();
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return false;

    // not need after this call
    if (owner->ToPlayer()->HasAtLoginFlag(AT_LOGIN_RESET_PET_TALENTS))
        owner->ToPlayer()->RemoveAtLoginFlag(AT_LOGIN_RESET_PET_TALENTS,true);

    CreatureInfo const * ci = GetCreatureInfo();
    if (!ci)
        return false;
    // Check pet talent type
    CreatureFamilyEntry const *pet_family = sCreatureFamilyStore.LookupEntry(ci->family);
    if (!pet_family || pet_family->petTalentType == PET_TALENT_TYPE_NOT_HUNTER_PET)
        return false;

    Player *player = owner->ToPlayer();

    uint8 level = getLevel();
    uint32 talentPointsForLevel = GetMaxTalentPointsForLevel(level);

    if (m_usedTalentCount == 0)
    {
        SetFreeTalentPoints(talentPointsForLevel);
        return false;
    }

    uint32 cost = 0;

    if (!no_cost)
    {
        cost = ResetTalentsCost();

        if (!player->HasEnoughMoney(cost))
        {
            player->SendBuyError(BUY_ERR_NOT_ENOUGHT_MONEY, 0, 0, 0);
            return false;
        }
    }

    for (uint32 i = 0; i < sTalentStore.GetNumRows(); ++i)
    {
        TalentEntry const *talentInfo = sTalentStore.LookupEntry(i);

        if (!talentInfo)
            continue;

        TalentTabEntry const *talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TalentTab);

        if (!talentTabInfo)
            continue;

        // unlearn only talents for pets family talent type
        if (!((1 << pet_family->petTalentType) & talentTabInfo->petTalentMask))
            continue;

        for (uint8 j = 0; j < MAX_TALENT_RANK; ++j)
        {
            for (PetSpellMap::const_iterator itr = m_spells.begin(); itr != m_spells.end();)
            {
                if (itr->second.state == PETSPELL_REMOVED)
                {
                    ++itr;
                    continue;
                }
                // remove learned spells (all ranks)
                uint32 itrFirstId = sSpellMgr->GetFirstSpellInChain(itr->first);

                // unlearn if first rank is talent or learned by talent
                if (itrFirstId == talentInfo->RankID[j] || sSpellMgr->IsSpellLearnToSpell(talentInfo->RankID[j],itrFirstId))
                {
                    unLearnSpell(itr->first,false);
                    itr = m_spells.begin();
                    continue;
                }
                else
                    ++itr;
            }
        }
    }

    SetFreeTalentPoints(talentPointsForLevel);

    if (!no_cost)
    {
        player->ModifyMoney(-(int32)cost);

        m_resetTalentsCost = cost;
        m_resetTalentsTime = time(NULL);
    }
    if (!m_loading)
        player->PetSpellInitialize();
    return true;
}

void Pet::resetTalentsForAllPetsOf(Player* owner, Pet* online_pet /*= NULL*/)
{
    // not need after this call
    if (owner->ToPlayer()->HasAtLoginFlag(AT_LOGIN_RESET_PET_TALENTS))
        owner->ToPlayer()->RemoveAtLoginFlag(AT_LOGIN_RESET_PET_TALENTS,true);

    // reset for online
    if (online_pet)
        online_pet->ResetTalents(true);

    // now need only reset for offline pets (all pets except online case)
    uint32 except_petnumber = online_pet ? online_pet->GetCharmInfo()->GetPetNumber() : 0;

    QueryResult resultPets = CharacterDatabase.PQuery(
        "SELECT id FROM character_pet WHERE owner = '%u' AND id <> '%u'",
        owner->GetGUIDLow(),except_petnumber);

    // no offline pets
    if (!resultPets)
        return;

    QueryResult result = CharacterDatabase.PQuery(
        "SELECT DISTINCT pet_spell.spell FROM pet_spell, character_pet "
        "WHERE character_pet.owner = '%u' AND character_pet.id = pet_spell.guid AND character_pet.id <> %u",
        owner->GetGUIDLow(),except_petnumber);

    if (!result)
        return;

    bool need_comma = false;
    std::ostringstream ss;
    ss << "DELETE FROM pet_spell WHERE guid IN (";

    do
    {
        Field *fields = resultPets->Fetch();

        uint32 id = fields[0].GetUInt32();

        if (need_comma)
            ss << ",";

        ss << id;

        need_comma = true;
    }
    while (resultPets->NextRow());

    ss << ") AND spell IN (";

    bool need_execute = false;
    do
    {
        Field *fields = result->Fetch();

        uint32 spell = fields[0].GetUInt32();

        if (!GetTalentSpellCost(spell))
            continue;

        if (need_execute)
            ss << ",";

        ss << spell;

        need_execute = true;
    }
    while (result->NextRow());

    if (!need_execute)
        return;

    ss << ")";

    CharacterDatabase.Execute(ss.str().c_str());
}

void Pet::InitTalentForLevel()
{
    uint8 level = getLevel();
    uint32 talentPointsForLevel = GetMaxTalentPointsForLevel(level);
    // Reset talents in case low level (on level down) or wrong points for level (hunter can unlearn TP increase talent)
    if (talentPointsForLevel == 0 || m_usedTalentCount > talentPointsForLevel)
        ResetTalents(true); // Remove all talent points

    SetFreeTalentPoints(talentPointsForLevel - m_usedTalentCount);

    Unit *owner = GetOwner();
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return;

    if (!m_loading)
        owner->ToPlayer()->SendTalentsInfoData(true);
}

uint32 Pet::ResetTalentsCost() const
{
    int64 days = int64(sWorld->GetGameTime() - m_resetTalentsTime)/DAY;

    // The first time reset costs 10 silver; after 1 day cost is reset to 10 silver
    if (m_resetTalentsCost < 10*SILVER || days > 0)
        return 10*SILVER;
    // then 50 silver
    else if (m_resetTalentsCost < 50*SILVER)
        return 50*SILVER;
    // then 1 gold
    else if (m_resetTalentsCost < 1*GOLD)
        return 1*GOLD;
    // then increasing at a rate of 1 gold; cap 10 gold
    else
        return (m_resetTalentsCost + 1*GOLD > 10*GOLD ? 10*GOLD : m_resetTalentsCost + 1*GOLD);
}

uint8 Pet::GetMaxTalentPointsForLevel(uint8 level)
{
    uint8 points = (level >= 20) ? ((level - 16) / 4) : 0;
    // Mod points from owner SPELL_AURA_MOD_PET_TALENT_POINTS
    if (Unit *owner = GetOwner())
        points+=owner->GetTotalAuraModifier(SPELL_AURA_MOD_PET_TALENT_POINTS);
    return points;
}

void Pet::ToggleAutocast(uint32 spellid, bool apply)
{
    if (!IsAutocastableSpell(spellid))
        return;

    PetSpellMap::iterator itr = m_spells.find(spellid);
    if (itr == m_spells.end())
        return;

    uint32 i;

    if (apply)
    {
        for (i = 0; i < m_autospells.size() && m_autospells[i] != spellid; ++i)
            ;                                               // just search

        if (i == m_autospells.size())
        {
            m_autospells.push_back(spellid);

            if (itr->second.active != ACT_ENABLED)
            {
                itr->second.active = ACT_ENABLED;
                if (itr->second.state != PETSPELL_NEW)
                    itr->second.state = PETSPELL_CHANGED;
            }
        }
    }
    else
    {
        AutoSpellList::iterator itr2 = m_autospells.begin();
        for (i = 0; i < m_autospells.size() && m_autospells[i] != spellid; ++i, ++itr2)
            ;                                               // just search

        if (i < m_autospells.size())
        {
            m_autospells.erase(itr2);
            if (itr->second.active != ACT_DISABLED)
            {
                itr->second.active = ACT_DISABLED;
                if (itr->second.state != PETSPELL_NEW)
                    itr->second.state = PETSPELL_CHANGED;
            }
        }
    }
}

bool Pet::IsPermanentPetFor(Player* owner)
{
    switch (getPetType())
    {
        case SUMMON_PET:
            switch (owner->getClass())
            {
                case CLASS_WARLOCK:
                    return GetCreatureInfo()->type == CREATURE_TYPE_DEMON;
                case CLASS_DEATH_KNIGHT:
                    return GetCreatureInfo()->type == CREATURE_TYPE_UNDEAD;
                case CLASS_MAGE:
                    return GetCreatureInfo()->type == CREATURE_TYPE_ELEMENTAL;
                default:
                    return false;
            }
        case HUNTER_PET:
            return true;
        default:
            return false;
    }
}

bool Pet::Create(uint32 pet_number, Map *map, uint32 phaseMask, uint32 Entry)
{
    ASSERT(map);
    SetMap(map);

    SetPhaseMask(phaseMask,false);
    Object::_Create(pet_number, pet_number, HIGHGUID_PET);

    m_DBTableGuid = pet_number;
    m_originalEntry = Entry;

    if (!InitEntry(Entry))
        return false;

    SetSheath(SHEATH_STATE_MELEE);

    return true;
}

bool Pet::HasSpell(uint32 spell) const
{
    PetSpellMap::const_iterator itr = m_spells.find(spell);
    return itr != m_spells.end() && itr->second.state != PETSPELL_REMOVED;
}

// Get all passive spells in our skill line
void Pet::LearnPetPassives()
{
    CreatureInfo const* cInfo = GetCreatureInfo();
    if (!cInfo)
        return;

    // Passive AoE avoidances for hunter pets, DK ghouls, warlock demons and priests shadowfiend
    switch (GetEntry())
    {
        case 416:   // Imp
        case 1860:  // Voidwalker
        case 1863:  // Succubus
        case 417:   // Felhunter
        case 17252: // Felguard
            CastSpell(this, 32233, true);
            break;
        case 19668: // Shadowfiend
            CastSpell(this, 63623, true);
            break;
        case 510:   // Water Elemental
            CastSpell(this, 65220, true);
            break;
        default:
        {
            if (IsPetGhoul())
            {
                int bp0 = -90;
                CastCustomSpell(this, 62137, &bp0, 0, 0, true);
            }
            if (IsHunterPet())
                CastSpell(this, 65220, true);
            break;
        }
    }

    CreatureFamilyEntry const* cFamily = sCreatureFamilyStore.LookupEntry(cInfo->family);
    if (!cFamily)
        return;

    PetFamilySpellsStore::const_iterator petStore = sPetFamilySpellsStore.find(cFamily->ID);
    if (petStore != sPetFamilySpellsStore.end())
    {
        // For general hunter pets skill 270
        // Passive 01~10, Passive 00 (20782, not used), Ferocious Inspiration (34457)
        // Scale 01~03 (34902~34904, bonus from owner, not used)
        for (PetFamilySpellsSet::const_iterator petSet = petStore->second.begin(); petSet != petStore->second.end(); ++petSet)
            AddSpell(*petSet, ACT_DECIDE, PETSPELL_NEW, PETSPELL_FAMILY);
    }
}

void Pet::CastPetAuras(bool current)
{
    Unit* owner = GetOwner();
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return;

    if (!IsPermanentPetFor(owner->ToPlayer()))
        return;

    for (PetAuraSet::const_iterator itr = owner->m_petAuras.begin(); itr != owner->m_petAuras.end();)
    {
        PetAura const* pa = *itr;
        ++itr;

        if (!current && pa->IsRemovedOnChangePet())
            owner->RemovePetAura(pa);
        else
            CastPetAura(pa);
    }

    // Demonic Pact raid-wide buff cast, just to make sure we (un)apply it
    if (owner->HasAura(47236))
        CastSpell(this, 53646, true);
    else
        RemoveAurasDueToSpell(53646);
}

void Pet::CastPetAura(PetAura const* aura)
{
    uint32 auraId = aura->GetAura(GetEntry());
    if (!auraId)
        return;

    if (auraId == 35696)                                      // Demonic Knowledge
    {
        int32 basePoints = int32(aura->GetDamage() * (GetStat(STAT_STAMINA) + GetStat(STAT_INTELLECT)) / 100);
        CastCustomSpell(this, auraId, &basePoints, NULL, NULL, true);
    }
    else
        CastSpell(this, auraId, true);
}

bool Pet::IsPetAura(Aura const* aura)
{
    Unit* owner = GetOwner();

    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return false;

    // if the owner has that pet aura, return true
    for (PetAuraSet::const_iterator itr = owner->m_petAuras.begin(); itr != owner->m_petAuras.end(); ++itr)
    {
        if ((*itr)->GetAura(GetEntry()) == aura->GetId())
            return true;
    }
    return false;
}

void Pet::learnSpellHighRank(uint32 spellid)
{
    LearnSpell(spellid);

    if (uint32 next = sSpellMgr->GetNextSpellInChain(spellid))
        learnSpellHighRank(next);
}

void Pet::SynchronizeLevelWithOwner()
{
    Unit* owner = GetOwner();
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return;

    switch (getPetType())
    {
        // always same level
        case SUMMON_PET:
        case HUNTER_PET:
            GivePetLevel(owner->getLevel());
            break;
        default:
            break;
    }
}

PetTalentType Pet::GetTalentType()
{
    CreatureInfo const *ci = GetCreatureInfo();
    if (!ci)
        return PET_TALENT_TYPE_NOT_HUNTER_PET;
    
    CreatureFamilyEntry const *pet_family = sCreatureFamilyStore.LookupEntry(ci->family);
    if (!pet_family)
        return PET_TALENT_TYPE_NOT_HUNTER_PET;
    
    return (PetTalentType)pet_family->petTalentType;
}
