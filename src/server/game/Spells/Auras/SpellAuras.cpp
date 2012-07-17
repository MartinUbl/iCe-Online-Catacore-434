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
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "Unit.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "DynamicObject.h"
#include "ObjectAccessor.h"
#include "Util.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "ScriptMgr.h"
#include "SpellScript.h"

AuraApplication::AuraApplication(Unit * target, Unit * caster, Aura * aura, uint8 effMask):
m_target(target), m_base(aura), m_slot(MAX_AURAS), m_flags(AFLAG_NONE),
m_effectsToApply(effMask), m_removeMode(AURA_REMOVE_NONE), m_needClientUpdate(false)
{
    ASSERT(GetTarget() && GetBase());

    if (GetBase()->IsVisible())
    {
        // Try find slot for aura
        uint8 slot = MAX_AURAS;
        // Lookup for auras already applied from spell
        if (AuraApplication * foundAura = GetTarget()->GetAuraApplication(GetBase()->GetId(), GetBase()->GetCasterGUID(), GetBase()->GetCastItemGUID()))
        {
            // allow use single slot only by auras from same caster
            slot = foundAura->GetSlot();
        }
        else
        {
            Unit::VisibleAuraMap const * visibleAuras = GetTarget()->GetVisibleAuras();
            // lookup for free slots in units visibleAuras
            Unit::VisibleAuraMap::const_iterator itr = visibleAuras->find(0);
            for (uint32 freeSlot = 0; freeSlot < MAX_AURAS; ++itr , ++freeSlot)
            {
                if (itr == visibleAuras->end() || itr->first != freeSlot)
                {
                    slot = freeSlot;
                    break;
                }
            }
        }

        // Register Visible Aura
        if (slot < MAX_AURAS)
        {
            m_slot = slot;
            GetTarget()->SetVisibleAura(slot, this);
            SetNeedClientUpdate();
            sLog->outDebug("Aura: %u Effect: %d put to unit visible auras slot: %u", GetBase()->GetId(), GetEffectMask(), slot);
        }
        else
            sLog->outDebug("Aura: %u Effect: %d could not find empty unit visible slot", GetBase()->GetId(), GetEffectMask());
    }

    _InitFlags(caster, effMask);
}

void AuraApplication::_Remove()
{
    uint8 slot = GetSlot();

    if (slot >= MAX_AURAS)
        return;

    if (AuraApplication * foundAura = m_target->GetAuraApplication(GetBase()->GetId(), GetBase()->GetCasterGUID(), GetBase()->GetCastItemGUID()))
    {
        // Reuse visible aura slot by aura which is still applied - prevent storing dead pointers
        if (slot == foundAura->GetSlot())
        {
            if (GetTarget()->GetVisibleAura(slot) == this)
            {
                GetTarget()->SetVisibleAura(slot, foundAura);
                foundAura->SetNeedClientUpdate();
            }
            // set not valid slot for aura - prevent removing other visible aura
            slot = MAX_AURAS;
        }
    }

    // update for out of range group members
    if (slot < MAX_AURAS)
    {
        GetTarget()->RemoveVisibleAura(slot);
        ClientUpdate(true);
    }
}

void AuraApplication::_InitFlags(Unit * caster, uint8 effMask)
{
    // mark as selfcasted if needed
    m_flags |= (GetBase()->GetCasterGUID() == GetTarget()->GetGUID()) ? AFLAG_CASTER : AFLAG_NONE;

    // aura is casted by self or an enemy
    // one negative effect and we know aura is negative
    if (IsSelfcasted() || !caster || !caster->IsFriendlyTo(GetTarget()))
    {
        bool negativeFound = false;
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (((1<<i) & effMask) && !IsPositiveEffect(GetBase()->GetId(), i))
            {
                negativeFound = true;
                break;
            }
        }
        m_flags |= negativeFound ? AFLAG_NEGATIVE : AFLAG_POSITIVE;
    }
    // aura is casted by friend
    // one positive effect and we know aura is positive
    else
    {
        bool positiveFound = false;
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (((1<<i) & effMask) && IsPositiveEffect(GetBase()->GetId(), i))
            {
                positiveFound = true;
                break;
            }
        }
        m_flags |= positiveFound ? AFLAG_POSITIVE : AFLAG_NEGATIVE;
    }
}

void AuraApplication::_HandleEffect(uint8 effIndex, bool apply)
{
    AuraEffect * aurEff = GetBase()->GetEffect(effIndex);
    ASSERT(aurEff);
    ASSERT(HasEffect(effIndex) == (!apply));
    ASSERT((1<<effIndex) & m_effectsToApply);
    sLog->outDebug("AuraApplication::_HandleEffect: %u, apply: %u: amount: %u", aurEff->GetAuraType(), apply, aurEff->GetAmount());

    if (apply)
    {
        m_flags |= 1<<effIndex;
        GetTarget()->_RegisterAuraEffect(aurEff, true);
        aurEff->HandleEffect(this, AURA_EFFECT_HANDLE_REAL, true);
    }
    else
    {
        m_flags &= ~(1<<effIndex);

        // remove from list before mods removing (prevent cyclic calls, mods added before including to aura list - use reverse order)
        GetTarget()->_RegisterAuraEffect(aurEff, false);
        aurEff->HandleEffect(this, AURA_EFFECT_HANDLE_REAL, false);

        // Remove all triggered by aura spells vs unlimited duration
        aurEff->CleanupTriggeredSpells(GetTarget());
    }
    SetNeedClientUpdate();
}

void AuraApplication::ClientUpdate(bool remove)
{
    m_needClientUpdate = false;

    WorldPacket data(SMSG_AURA_UPDATE);
    data.append(GetTarget()->GetPackGUID());
    data << uint8(m_slot);

    if (remove)
    {
        ASSERT(!m_target->GetVisibleAura(m_slot));
        data << uint32(0);
        sLog->outDebug("Aura %u removed slot %u",GetBase()->GetId(), m_slot);
        m_target->SendMessageToSet(&data, true);
        return;
    }
    ASSERT(m_target->GetVisibleAura(m_slot));

    Aura const * aura = GetBase();

    data << uint32(aura->GetId());
    uint32 flags = m_flags;
    if (aura->GetMaxDuration() > 0)
        flags |= AFLAG_DURATION;

    AuraBasepointType bptype = BP_NONE;

    int32 idx = 0;
    if (aura->IsModActionButton())
    {
        bptype = BP_MOD_ACTION_BUTTON;
        flags |= AFLAG_BASEPOINT;
    }
    else
    {
        idx = aura->GetSpellProto()->GetEffectMiscValueB(0);
        if (aura->GetCasterGUID() == GetTarget()->GetGUID())
        {
            if (m_target->GetTypeId() == TYPEID_PLAYER)
            {
                if (aura->GetSpellProto()->Id == 87840)
                    idx = m_target->ToPlayer()->GetMountCapabilityIndex(230);
                else
                    idx = m_target->ToPlayer()->GetMountCapabilityIndex(idx);

                if (idx)
                {
                    bptype = BP_MOUNT_CAPATIBILITY;
                    flags |= AFLAG_BASEPOINT;
                }
            }
        }
    }

    // If we still haven't got basepoint replacement
    if (bptype == BP_NONE)
    {
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; i++)
        {
            if (aura->GetEffect(i) && aura->GetEffect(i)->GetAmount())
            {
                bptype = BP_AMOUNT;
                flags |= AFLAG_BASEPOINT;
            }
        }
    }

    data << uint8(flags);
    data << uint8(aura->GetCasterLevel());
    data << uint8(aura->GetStackAmount() > 1 ? aura->GetStackAmount() : (aura->GetCharges()) ? aura->GetCharges() : 1);

    if (!(flags & AFLAG_CASTER))
        data.appendPackGUID(aura->GetCasterGUID());

    if (flags & AFLAG_DURATION)
    {
        data << uint32(aura->GetMaxDuration());
        data << uint32(aura->GetDuration());
    }

    if (flags & AFLAG_BASEPOINT)
    {
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; i++)
        {
            if ( (1 << i) & flags )
            {
                switch (bptype)
                {
                    case BP_MOD_ACTION_BUTTON:
                        data << uint32(aura->GetActionButtonSpellForEffect(i));
                        break;
                    case BP_MOUNT_CAPATIBILITY:
                        data << uint32(idx);
                        break;
                    case BP_AMOUNT:
                        data << uint32(aura->GetEffect(i) ? aura->GetEffect(i)->GetAmount() : 0);
                        break;
                    default:
                        data << uint32(0);
                        break;
                }
            }
        }
    }

    m_target->SendMessageToSet(&data, true);
}

Aura * Aura::TryCreate(SpellEntry const* spellproto, uint8 tryEffMask, WorldObject * owner, Unit * caster, int32 *baseAmount, Item * castItem, uint64 casterGUID)
{
    ASSERT(spellproto);
    ASSERT(owner);
    ASSERT(caster || casterGUID);
    ASSERT(tryEffMask <= MAX_EFFECT_MASK);
    uint8 effMask = 0;
    switch(owner->GetTypeId())
    {
        case TYPEID_UNIT:
        case TYPEID_PLAYER:
            for (uint8 i = 0; i< MAX_SPELL_EFFECTS; ++i)
            {
                if (IsUnitOwnedAuraEffect(spellproto->Effect[i]))
                    effMask |= 1 << i;
            }
            break;
        case TYPEID_DYNAMICOBJECT:
            for (uint8 i = 0; i< MAX_SPELL_EFFECTS; ++i)
            {
                if (spellproto->Effect[i] == SPELL_EFFECT_PERSISTENT_AREA_AURA)
                    effMask |= 1 << i;
            }
            break;
        default:
            break;
    }
    if (uint8 realMask = effMask & tryEffMask)
        return Create(spellproto,realMask,owner,caster,baseAmount,castItem,casterGUID);
    return NULL;
}

Aura * Aura::TryCreate(SpellEntry const* spellproto, WorldObject * owner, Unit * caster, int32 *baseAmount, Item * castItem, uint64 casterGUID)
{
    ASSERT(spellproto);
    ASSERT(owner);
    ASSERT(caster || casterGUID);
    uint8 effMask = 0;
    switch(owner->GetTypeId())
    {
        case TYPEID_UNIT:
        case TYPEID_PLAYER:
            for (uint8 i = 0; i< MAX_SPELL_EFFECTS; ++i)
            {
                if (IsUnitOwnedAuraEffect(spellproto->Effect[i]))
                    effMask |= 1 << i;
            }
            break;
        case TYPEID_DYNAMICOBJECT:
            for (uint8 i = 0; i< MAX_SPELL_EFFECTS; ++i)
            {
                if (spellproto->Effect[i] == SPELL_EFFECT_PERSISTENT_AREA_AURA)
                    effMask |= 1 << i;
            }
            break;
        default:
            break;
    }
    if (effMask)
        return Create(spellproto,effMask,owner,caster,baseAmount,castItem,casterGUID);
    return NULL;
}

Aura * Aura::Create(SpellEntry const* spellproto, uint8 effMask, WorldObject * owner, Unit * caster, int32 *baseAmount, Item * castItem, uint64 casterGUID)
{
    ASSERT(effMask);
    ASSERT(spellproto);
    ASSERT(owner);
    ASSERT(caster || casterGUID);
    ASSERT(effMask <= MAX_EFFECT_MASK);
    // try to get caster of aura
    if (casterGUID)
    {
        if (owner->GetGUID() == casterGUID)
            caster = (Unit *)owner;
        else
            caster = ObjectAccessor::GetUnit(*owner, casterGUID);
    }
    else
    {
        casterGUID = caster->GetGUID();
    }
    // check if aura can be owned by owner
    if (owner->isType(TYPEMASK_UNIT))
    {
        if (!owner->IsInWorld() || ((Unit*)owner)->IsDuringRemoveFromWorld())
        {
            // owner not in world so
            // don't allow to own not self casted single target auras
            if (casterGUID != owner->GetGUID() && IsSingleTargetSpell(spellproto))
                return NULL;
        }
    }
    Aura * aura = NULL;
    switch(owner->GetTypeId())
    {
        case TYPEID_UNIT:
        case TYPEID_PLAYER:
            aura = new UnitAura(spellproto,effMask,owner,caster,baseAmount,castItem, casterGUID);
            break;
        case TYPEID_DYNAMICOBJECT:
            aura = new DynObjAura(spellproto,effMask,owner,caster,baseAmount,castItem, casterGUID);
            break;
        default:
            ASSERT(false);
            return NULL;
    }
    // aura can be removed in Unit::_AddAura call
    if (aura->IsRemoved())
        return NULL;
    return aura;
}

Aura::Aura(SpellEntry const* spellproto, uint8 effMask, WorldObject * owner, Unit * caster, int32 *baseAmount, Item * castItem, uint64 casterGUID):
m_spellProto(spellproto), m_casterGuid(casterGUID ? casterGUID : caster->GetGUID()),
m_castItemGuid(castItem ? castItem->GetGUID() : 0), m_applyTime(time(NULL)),
m_owner(owner), m_timeCla(0), m_updateTargetMapInterval(0),
m_casterLevel(caster ? caster->getLevel() : m_spellProto->spellLevel), m_procCharges(0), m_stackAmount(1),
m_isRemoved(false), m_isSingleTarget(false)
{
    if (m_spellProto->manaPerSecond)
        m_timeCla = 1 * IN_MILLISECONDS;

    Player* modOwner = NULL;

    if (caster)
    {
        modOwner = caster->GetSpellModOwner();
        m_maxDuration = caster->CalcSpellDuration(m_spellProto);
    }
    else
        m_maxDuration = GetSpellDuration(m_spellProto);

    if (IsPassive() && m_spellProto->DurationIndex == 0)
        m_maxDuration = -1;

    if (!IsPermanent() && modOwner)
        modOwner->ApplySpellMod(GetId(), SPELLMOD_DURATION, m_maxDuration);

    m_duration = m_maxDuration;

    m_procCharges = m_spellProto->procCharges;
    if (modOwner)
        modOwner->ApplySpellMod(GetId(), SPELLMOD_CHARGES, m_procCharges);
}
void Aura::_InitEffects(uint8 effMask, Unit * caster, int32 *baseAmount)
{
    // shouldn't be in constructor - functions in AuraEffect::AuraEffect use polymorphism
    for (uint8 i=0 ; i<MAX_SPELL_EFFECTS; ++i)
    {
        if (effMask & (uint8(1) << i))
            m_effects[i] = new AuraEffect(this, i, baseAmount ? baseAmount + i : NULL, caster);
        else
            m_effects[i] = NULL;
    }
}

Aura::~Aura()
{
    // unload scripts
    while(!m_loadedScripts.empty())
    {
        std::list<AuraScript *>::iterator itr = m_loadedScripts.begin();
        (*itr)->_Unload();
        delete (*itr);
        m_loadedScripts.erase(itr);
    }

    // free effects memory
    for (uint8 i = 0 ; i < MAX_SPELL_EFFECTS; ++i)
         delete m_effects[i];

    ASSERT(m_applications.empty());
    _DeleteRemovedApplications();
}

Unit* Aura::GetCaster() const
{
    if (GetOwner()->GetGUID() == GetCasterGUID())
        return GetUnitOwner();
    if (AuraApplication const * aurApp = GetApplicationOfTarget(GetCasterGUID()))
        return aurApp->GetTarget();

    return ObjectAccessor::GetUnit(*GetOwner(), GetCasterGUID());
}

AuraObjectType Aura::GetType() const
{
    return (m_owner->GetTypeId() == TYPEID_DYNAMICOBJECT) ? DYNOBJ_AURA_TYPE : UNIT_AURA_TYPE;
}

void Aura::_ApplyForTarget(Unit * target, Unit * caster, AuraApplication * auraApp)
{
    ASSERT(target);
    ASSERT(auraApp);
    // aura mustn't be already applied
    ASSERT (m_applications.find(target->GetGUID()) == m_applications.end());

    m_applications[target->GetGUID()] = auraApp;

    // set infinity cooldown state for spells
    if (caster && caster->GetTypeId() == TYPEID_PLAYER)
    {
        if (m_spellProto->Attributes & SPELL_ATTR0_DISABLED_WHILE_ACTIVE)
        {
            Item* castItem = m_castItemGuid ? caster->ToPlayer()->GetItemByGuid(m_castItemGuid) : NULL;
            caster->ToPlayer()->AddSpellAndCategoryCooldowns(m_spellProto,castItem ? castItem->GetEntry() : 0, NULL,true);
        }
    }
}

void Aura::_UnapplyForTarget(Unit * target, Unit * caster, AuraApplication * auraApp)
{
    ASSERT(target);
    ASSERT(auraApp->GetRemoveMode());
    ASSERT(auraApp);

    ApplicationMap::iterator itr = m_applications.find(target->GetGUID());
    // TODO: Figure out why this happens.
    if (itr == m_applications.end())
    {
        sLog->outError("Aura::_UnapplyForTarget, target:%u, caster:%u, spell:%u was not found in owners application map!",
        target->GetGUIDLow(), caster->GetGUIDLow(), auraApp->GetBase()->GetSpellProto()->Id);
    }
    else
        m_applications.erase(itr);

    // aura has to be already applied
    //ASSERT(itr->second == auraApp);
    m_removedApplications.push_back(auraApp);

    // reset cooldown state for spells
    if (caster && caster->GetTypeId() == TYPEID_PLAYER)
    {
        if (GetSpellProto()->Attributes & SPELL_ATTR0_DISABLED_WHILE_ACTIVE)
            // note: item based cooldowns and cooldown spell mods with charges ignored (unknown existed cases)
            caster->ToPlayer()->SendCooldownEvent(GetSpellProto());
    }
}

// removes aura from all targets
// and marks aura as removed
void Aura::_Remove(AuraRemoveMode removeMode)
{
    ASSERT (!m_isRemoved);
    m_isRemoved = true;
    ApplicationMap::iterator appItr = m_applications.begin();
    for (appItr = m_applications.begin(); appItr != m_applications.end();)
    {
        AuraApplication * aurApp =  appItr->second;
        Unit * target = aurApp->GetTarget();
        target->_UnapplyAura(aurApp, removeMode);
        appItr = m_applications.begin();
    }
}

void Aura::UpdateTargetMap(Unit * caster, bool apply)
{
    if (IsRemoved())
        return;

    m_updateTargetMapInterval = UPDATE_TARGET_MAP_INTERVAL;

    // Wierd exception for Gouge application not-applying immediately
    if (m_spellProto->Id == 1776)
        m_updateTargetMapInterval = 0;

    // fill up to date target list
    //       target, effMask
    std::map<Unit *, uint8> targets;

    FillTargetMap(targets, caster);

    UnitList targetsToRemove;

    // mark all auras as ready to remove
    for (ApplicationMap::iterator appIter = m_applications.begin(); appIter != m_applications.end();++appIter)
    {
        std::map<Unit *, uint8>::iterator existing = targets.find(appIter->second->GetTarget());
        // not found in current area - remove the aura
        if (existing == targets.end())
            targetsToRemove.push_back(appIter->second->GetTarget());
        else
        {
            // needs readding - remove now, will be applied in next update cycle
            // (dbcs do not have auras which apply on same type of targets but have different radius, so this is not really needed)
            if (appIter->second->GetEffectMask() != existing->second || !CanBeAppliedOn(existing->first))
                targetsToRemove.push_back(appIter->second->GetTarget());
            // nothing todo - aura already applied
            // remove from auras to register list
            targets.erase(existing);
        }
    }

    // register auras for units
    for (std::map<Unit *, uint8>::iterator itr = targets.begin(); itr!= targets.end();)
    {
        // aura mustn't be already applied on target
        if (AuraApplication * aurApp = GetApplicationOfTarget(itr->first->GetGUID()))
        {
            // the core created 2 different units with same guid
            // this is a major failure, which i can't fix right now
            // let's remove one unit from aura list
            // this may cause area aura "bouncing" between 2 units after each update
            // but because we know the reason of a crash we can remove the assertion for now
            if (aurApp->GetTarget() != itr->first)
            {
                // remove from auras to register list
                targets.erase(itr++);
                continue;
            }
        }

        bool addUnit = true;
        // check target immunities
        if (itr->first->IsImmunedToSpell(GetSpellProto())
            || !CanBeAppliedOn(itr->first))
            addUnit = false;

        if (addUnit)
        {
            // persistent area aura does not hit flying targets
            if (GetType() == DYNOBJ_AURA_TYPE)
            {
                if (itr->first->isInFlight())
                    addUnit = false;
            }
            // unit auras can not stack with each other
            else // (GetType() == UNIT_AURA_TYPE)
            {
                // Allow to remove by stack when aura is going to be applied on owner
                if (itr->first != GetOwner())
                {
                    // check if not stacking aura already on target
                    // this one prevents unwanted usefull buff loss because of stacking and prevents overriding auras periodicaly by 2 near area aura owners
                    for (Unit::AuraApplicationMap::iterator iter = itr->first->GetAppliedAuras().begin(); iter != itr->first->GetAppliedAuras().end(); ++iter)
                    {
                        Aura const * aura = iter->second->GetBase();
                        if (!sSpellMgr->CanAurasStack(this, aura, aura->GetCasterGUID() == GetCasterGUID()))
                        {
                            addUnit = false;
                            break;
                        }
                    }
                }
            }
        }
        if (!addUnit)
            targets.erase(itr++);
        else
        {
            // owner has to be in world, or effect has to be applied to self
            ASSERT((!GetOwner()->IsInWorld() && GetOwner() == itr->first) || GetOwner()->IsInMap(itr->first));
            itr->first->_CreateAuraApplication(this, itr->second);
            ++itr;
        }
    }

    // remove auras from units no longer needing them
    for (UnitList::iterator itr = targetsToRemove.begin(); itr != targetsToRemove.end();++itr)
        if (AuraApplication * aurApp = GetApplicationOfTarget((*itr)->GetGUID()))
            (*itr)->_UnapplyAura(aurApp, AURA_REMOVE_BY_DEFAULT);

    if (!apply)
        return;

    // apply aura effects for units
    for (std::map<Unit *, uint8>::iterator itr = targets.begin(); itr!= targets.end();++itr)
    {
        if (AuraApplication * aurApp = GetApplicationOfTarget(itr->first->GetGUID()))
        {
            // owner has to be in world, or effect has to be applied to self
            ASSERT((!GetOwner()->IsInWorld() && GetOwner() == itr->first) || GetOwner()->IsInMap(itr->first));
            itr->first->_ApplyAura(aurApp, itr->second);
        }
    }
}

// targets have to be registered and not have effect applied yet to use this function
void Aura::_ApplyEffectForTargets(uint8 effIndex)
{
    //Unit * caster = GetCaster();
    // prepare list of aura targets
    UnitList targetList;
    for (ApplicationMap::iterator appIter = m_applications.begin(); appIter != m_applications.end(); ++appIter)
    {
        if ((appIter->second->GetEffectsToApply() & (1<<effIndex)) && !appIter->second->HasEffect(effIndex))
            targetList.push_back(appIter->second->GetTarget());
    }

    // apply effect to targets
    for (UnitList::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
    {
        if (GetApplicationOfTarget((*itr)->GetGUID()))
        {
            // owner has to be in world, or effect has to be applied to self
            if (!((!GetOwner()->IsInWorld() && GetOwner() == *itr) || GetOwner()->IsInMap(*itr)))
                return;

            (*itr)->_ApplyAuraEffect(this, effIndex);
        }
    }
}
void Aura::UpdateOwner(uint32 diff, WorldObject * owner)
{
    ASSERT(owner == m_owner);

    Unit * caster = GetCaster();
    // Apply spellmods for channeled auras
    // used for example when triggered spell of spell:10 is modded
    Spell * modSpell = NULL;
    Player * modOwner = NULL;
    if (caster)
    {
        modOwner = caster->GetSpellModOwner();
        if (modOwner)
        {
            modSpell = modOwner->FindCurrentSpellBySpellId(GetId());
            if (modSpell)
                modOwner->SetSpellModTakingSpell(modSpell, true);
        }
    }

    Update(diff, caster);

    if (m_updateTargetMapInterval <= int32(diff))
        UpdateTargetMap(caster);
    else
        m_updateTargetMapInterval -= diff;

    // update aura effects
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        if (m_effects[i])
            m_effects[i]->Update(diff, caster);

    // remove spellmods after effects update
    if (modSpell)
        modOwner->SetSpellModTakingSpell(modSpell, false);

    _DeleteRemovedApplications();
}

void Aura::Update(uint32 diff, Unit * caster)
{
    if (m_duration > 0)
    {
        m_duration -= diff;
        if (m_duration < 0)
            m_duration = 0;

        // handle manaPerSecond/manaPerSecondPerLevel
        if (m_timeCla)
        {
            if (m_timeCla > int32(diff))
                m_timeCla -= diff;
            else if (caster)
            {
                if (int32 manaPerSecond = m_spellProto->manaPerSecond)
                {
                    m_timeCla += 1000 - diff;

                    Powers powertype = Powers(m_spellProto->powerType);
                    if (powertype == POWER_HEALTH)
                    {
                        if (int32(caster->GetHealth()) > manaPerSecond)
                            caster->ModifyHealth(-manaPerSecond);
                        else
                        {
                            Remove();
                            return;
                        }
                    }
                    else
                    {
                        if (int32(caster->GetPower(powertype)) >= manaPerSecond)
                            caster->ModifyPower(powertype, -manaPerSecond);
                        else
                        {
                            Remove();
                            return;
                        }
                    }
                }
            }
        }
    }
}

void Aura::SetDuration(int32 duration, bool withMods)
{
    if (withMods)
    {
        if (Unit * caster = GetCaster())
            if (Player * modOwner = caster->GetSpellModOwner())
                modOwner->ApplySpellMod(GetId(), SPELLMOD_DURATION, duration);
    }
    m_duration = duration;
    SetNeedClientUpdateForTargets();
}

void Aura::RefreshDuration()
{
    SetDuration(GetMaxDuration());
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        if (m_effects[i])
            m_effects[i]->ResetPeriodic();

    if (m_spellProto->manaPerSecond)
        m_timeCla = 1 * IN_MILLISECONDS;
}

void Aura::SetCharges(uint8 charges)
{
    if (m_procCharges == charges)
        return;
    m_procCharges = charges;
    SetNeedClientUpdateForTargets();
}

bool Aura::DropCharge()
{
    if (m_procCharges) //auras without charges always have charge = 0
    {
        if (--m_procCharges) // Send charge change
            SetNeedClientUpdateForTargets();
        else              // Last charge dropped
        {
            Remove(AURA_REMOVE_BY_EXPIRE);
            return true;
        }
    }
    return false;
}

void Aura::SetStackAmount(uint8 stackAmount, bool /*applied*/)
{
    if (stackAmount != m_stackAmount)
    {
        m_stackAmount = stackAmount;
        RecalculateAmountOfEffects();
    }
    SetNeedClientUpdateForTargets();
}

bool Aura::ModStackAmount(int32 num)
{
    // Can`t mod
    if (!m_spellProto->StackAmount || !GetStackAmount())
        return true;

    // Modify stack but limit it
    int32 stackAmount = m_stackAmount + num;
    if (stackAmount > int32(m_spellProto->StackAmount))
        stackAmount = m_spellProto->StackAmount;
    else if (stackAmount <= 0) // Last aura from stack removed
    {
        m_stackAmount = 0;
        return true; // need remove aura
    }
    bool refresh = stackAmount >= GetStackAmount();

    // Update stack amount
    SetStackAmount(stackAmount);

    if (refresh)
        RefreshDuration();
    SetNeedClientUpdateForTargets();

    return false;
}

bool Aura::IsModActionButton() const
{
    // Mostly spells with aura #332 or #333
    // Some spells with these auras does not modify action bar spells, no idea how to decide which does.
    // Otherwise some spells without these aura types do modify AB

    // IMPORTANT NOTE:
    // Don't forget to add new spell ID that modifies spell bar to field in WorldSession::HandleCastSpellOpcode !

    switch (GetSpellProto()->Id)
    {
        case 48108: // Hot Streak
        case 74434: // Soulburn
        case 81021: // Stampede (Cat Form)
        case 81022: // Stampede (Bear Form)
        case 77769: // Trap Launcher
        case 82946: // Trap Launcher (second?)
        case 82926: // Fire! (Master Marksman proc)
        case 86211: // Soul Swap (marker)
        case 77616: // Dark Simulacrum
        case 81206: // Chakra: Sanctuary
        case 81208: // Chakra: Serenity
            return true;
        case 687:   // Demon Armor - condition for talent Nether Ward
        case 28176: // Fel Armor
            return true;
        case 94338: // Eclipse (Solar) - condition for talent Sunfire
            if (GetCaster()->HasAura(93401))
                return true;
            else
                return false;
        case 92294: // Frostfire Orb Override - condition for talent Frostfire Orb
            if (GetCaster()->HasAura(84726) || GetCaster()->HasAura(84727) || GetCaster()->HasAura(84728))
                return true;
            else
                return false;
        default:
            return false;
    }

    return false;
}

uint8 Aura::GetModActionButtonEffectMask() const
{
    uint8 effMask = 0;
    // Effect mask for mod action button procedure
    // every OR has bit mover equal to its spell effect index (even spell effect index 0)
    switch (GetSpellProto()->Id)
    {
        case 48108: // Hot Streak
        case 94338: // Eclipse (Solar)
        case 91713: // Nether Ward
        case 92294: // Frostfire Orb Override
        case 86211: // Soul Swap (marker)
        case 77616: // Dark Simulacrum
        case 81021: // Stampede (Cat Form)
        case 81022: // Stampede (Bear Form)
            effMask |= 1 << 0;
            break;
        case 74434: // Soulburn
        case 82926: // Fire! (Master Marksman proc)
            effMask |= 1 << 1;
            break;
        case 82946: // Trap Launcher (second?)
            effMask |= (1 << 0) | (1 << 1);
            break;
        case 77769: // Trap Launcher
            effMask |= (1 << 0) | (1 << 1) | (1 << 2);
            break;
        case 687:   // Demon Armor
        case 28176: // Fel Armor
        case 81206: // Chakra: Sanctuary
        case 81208: // Chakra: Serenity
            effMask |= 1 << 2;
            break;
        default:
            return 0;
    }

    return effMask;
}

uint32 Aura::GetActionButtonSpellForEffect(uint8 effIndex) const
{
    // Some spells has new spell ID in their BasePoints for specific effect index
    // Some not...

    // If supplied effIndex doesn't modify action button, skip
    if (!((1 << effIndex) & GetModActionButtonEffectMask()))
        return 0;

    switch (GetSpellProto()->Id)
    {
        case 48108: // Hot Streak
        case 74434: // Soulburn
        case 81021: // Stampede (Cat Form)
        case 81022: // Stampede (Bear Form)
        case 77769: // Trap Launcher
        case 82946: // Trap Launcher (second?)
        case 82926: // Fire! (Master Marksman proc)
        case 84728: // Frostfire Orb Override
        case 86211: // Soul Swap (marker)
            return GetSpellProto()->EffectBasePoints[effIndex];
        case 77616: // Dark Simulacrum
            return (m_effects[effIndex] != NULL) ? m_effects[effIndex]->GetBaseAmount() : 0;
        case 94338: // Eclipse (Solar)
            return 93402;
        case 687: // Demon Armor
        case 28176: // Fel Armor
            if (GetCaster()->HasAura(91713))
                return 91711;
            else
                return 6229;
        case 81206: // Chakra: Sanctuary
        case 81208: // Chakra: Serenity
            // talent Revelations
            if (!GetCaster() || !GetCaster()->HasAura(88627))
                return 88625;
            return GetSpellProto()->EffectBasePoints[effIndex];
        default:
            return 0;
    }

    return 0;
}

bool Aura::IsPassive() const
{
    return IsPassiveSpell(GetSpellProto());
}

bool Aura::IsDeathPersistent() const
{
    return IsDeathPersistentSpell(GetSpellProto());
}

bool Aura::CanBeSaved() const
{
    if (IsPassive())
        return false;

    if (GetCasterGUID() != GetOwner()->GetGUID())
        if (IsSingleTargetSpell(GetSpellProto()))
            return false;

    // Can't be saved - aura handler relies on calculated amount and changes it
    if (HasEffectType(SPELL_AURA_CONVERT_RUNE))
        return false;

    // No point in saving this, since the stable dialog can't be open on aura load anyway.
    if (HasEffectType(SPELL_AURA_OPEN_STABLE))
        return false;

    return true;
}

bool Aura::IsVisible() const
{
    return !IsPassive() || HasAreaAuraEffect(GetSpellProto()) || HasEffectType(SPELL_AURA_ABILITY_IGNORE_AURASTATE);
}

void Aura::UnregisterSingleTarget()
{
    ASSERT(m_isSingleTarget);
    Unit * caster = GetCaster();
    // TODO: find a better way to do this.
    if (!caster)
        caster = ObjectAccessor::GetObjectInOrOutOfWorld(GetCasterGUID(), (Unit*)NULL);
    ASSERT(caster);
    caster->GetSingleCastAuras().remove(this);
    SetIsSingleTarget(false);
}

void Aura::SetLoadedState(int32 maxduration, int32 duration, int32 charges, uint8 stackamount, uint8 recalculateMask, int32 * amount)
{
    m_maxDuration = maxduration;
    m_duration = duration;
    m_procCharges = charges;
    m_stackAmount = stackamount;
    Unit * caster = GetCaster();
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        if (m_effects[i])
        {
            m_effects[i]->SetAmount(amount[i]);
            m_effects[i]->SetCanBeRecalculated(recalculateMask & (1<<i));
            m_effects[i]->CalculatePeriodic(caster);
            m_effects[i]->CalculateSpellMod();
            m_effects[i]->RecalculateAmount(caster);
        }
}

bool Aura::HasEffectType(AuraType type) const
{
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (m_effects[i] && m_effects[i]->GetAuraType() == type)
            return true;
    }
    return false;
}

void Aura::RecalculateAmountOfEffects()
{
    ASSERT (!IsRemoved());
    Unit * caster = GetCaster();
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        if (m_effects[i])
            m_effects[i]->RecalculateAmount(caster);
}

void Aura::HandleAllEffects(AuraApplication const * aurApp, uint8 mode, bool apply)
{
    ASSERT (!IsRemoved());
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        if (m_effects[i] && !IsRemoved())
            m_effects[i]->HandleEffect(aurApp, mode, apply);
}

void Aura::SetNeedClientUpdateForTargets() const
{
    for (ApplicationMap::const_iterator appIter = m_applications.begin(); appIter != m_applications.end(); ++appIter)
        appIter->second->SetNeedClientUpdate();
}

// trigger effects on real aura apply/remove
void Aura::HandleAuraSpecificMods(AuraApplication const * aurApp, Unit * caster, bool apply)
{
    Unit * target = aurApp->GetTarget();
    AuraRemoveMode removeMode = aurApp->GetRemoveMode();
    // spell_area table
    SpellAreaForAreaMapBounds saBounds = sSpellMgr->GetSpellAreaForAuraMapBounds(GetId());
    if (saBounds.first != saBounds.second)
    {
        uint32 zone, area;
        target->GetZoneAndAreaId(zone,area);

        for (SpellAreaForAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
        {
            // some auras remove at aura remove
            if (!itr->second->IsFitToRequirements((Player*)target,zone,area))
                target->RemoveAurasDueToSpell(itr->second->spellId);
            // some auras applied at aura apply
            else if (itr->second->autocast)
            {
                if (!target->HasAura(itr->second->spellId))
                    target->CastSpell(target,itr->second->spellId,true);
            }
        }
    }
    // mods at aura apply
    if (apply)
    {
        // Apply linked auras (On first aura apply)
        if (sSpellMgr->GetSpellCustomAttr(GetId()) & SPELL_ATTR0_CU_LINK_AURA)
        {
            if (const std::vector<int32> *spell_triggered = sSpellMgr->GetSpellLinked(GetId() + SPELL_LINK_AURA))
                for (std::vector<int32>::const_iterator itr = spell_triggered->begin(); itr != spell_triggered->end(); ++itr)
                {
                    if (*itr < 0)
                        target->ApplySpellImmune(GetId(), IMMUNITY_ID, -(*itr), true);
                    else if (caster)
                        caster->AddAura(*itr, target);
                }
        }
        switch (GetSpellProto()->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
                switch(GetId())
                {
                    case 32474: // Buffeting Winds of Susurrus
                        if (target->GetTypeId() == TYPEID_PLAYER)
                            target->ToPlayer()->ActivateTaxiPathTo(506, GetId());
                        break;
                    case 33572: // Gronn Lord's Grasp, becomes stoned
                        if (GetStackAmount() >= 5 && !target->HasAura(33652))
                            target->CastSpell(target, 33652, true);
                        break;
                    case 50836: //Petrifying Grip, becomes stoned
                        if (GetStackAmount() >= 5 && !target->HasAura(50812))
                            target->CastSpell(target, 50812, true);
                        break;
                    case 60970: // Heroic Fury (remove Intercept cooldown)
                        if (target->GetTypeId() == TYPEID_PLAYER)
                            target->ToPlayer()->RemoveSpellCooldown(20252, true);
                        break;
                    case 85474: // [DND] Hide text (unused)
                        if (target->GetTypeId() == TYPEID_UNIT)
                        {
                            SetMaxDuration(-1);   // duration: persistant
                            RefreshDuration();
                        }
                        break;
                    case 45182: // Cheating Death (from rogue talent Cheat Death)
                        if (target->GetTypeId() == TYPEID_PLAYER)
                            target->SetHealth(target->GetMaxHealth()*0.1f); // Set health to 10% of maximum
                }
                break;
            case SPELLFAMILY_MAGE:
                if (!caster)
                    break;
                if (GetSpellProto()->SpellFamilyFlags[0] & 0x00000001 && GetSpellProto()->SpellFamilyFlags[2] & 0x00000008)
                {
                    // Glyph of Fireball
                    if (caster->HasAura(56368))
                        SetDuration(0);
                }
                // Todo: This should be moved to similar function in spell::hit
                else if (GetSpellProto()->SpellFamilyFlags[0] & 0x01000000)
                {
                    // Polymorph Sound - Sheep && Penguin
                    if (GetSpellProto()->SpellIconID == 82 && GetSpellProto()->SpellVisual[0] == 12978)
                    {
                        // Glyph of the Penguin
                        if (caster->HasAura(52648))
                            caster->CastSpell(target,61635,true);
                        else
                            caster->CastSpell(target,61634,true);
                    }
                }
                switch(GetId())
                {
                    case 12536: // Clearcasting
                    case 12043: // Presence of Mind
                        // Arcane Potency
                        if (AuraEffect const * aurEff = caster->GetAuraEffect(SPELL_AURA_DUMMY, SPELLFAMILY_MAGE, 2120, 0))
                        {
                            uint32 spellId = 0;

                            switch (aurEff->GetId())
                            {
                                case 31571: spellId = 57529; break;
                                case 31572: spellId = 57531; break;
                                default:
                                    sLog->outError("Aura::HandleAuraSpecificMods: Unknown rank of Arcane Potency (%d) found", aurEff->GetId());
                            }
                            if (spellId)
                                caster->CastSpell(caster, spellId, true);
                        }
                        break;
                    case 45438: // Ice Block
                        // Glyph of Ice Block
                        if (caster && caster->ToPlayer() && caster->HasAura(56372))
                            caster->ToPlayer()->RemoveSpellCooldown(122, true);
                        break;
                    case 12472: // Icy Veins
                        // Glyph of Icy Veins
                        if (caster->HasAura(56374))
                        {
                            caster->RemoveAurasWithMechanic((1 << MECHANIC_SNARE));
                            Unit::AuraEffectList const& auraList = caster->GetAuraEffectsByType(SPELL_AURA_HASTE_SPELLS);
                            if (!auraList.empty())
                            {
                                for (Unit::AuraEffectList::const_iterator itr = auraList.begin(); itr != auraList.end();)
                                {
                                    Aura* aura = (*itr)->GetBase();
                                    int32 amount = (*itr)->GetAmount();

                                    ++itr;
                                    if (aura && amount < 0)
                                        aura->Remove();
                                }
                            }
                        }
                        break;
                    case 120: // Cone of Cold
                        // Improved Cone of Cold
                        if(caster->HasAura(12489))
                            target->CastSpell(target,83302,true);
                        else if(caster->HasAura(11190))
                            target->CastSpell(target,83301,true);
                        break;
                    default:
                        break;
                }
                break;
            case SPELLFAMILY_WARLOCK:
                switch(GetId())
                {
                    case 48020: // Demonic Circle
                        if (target->GetTypeId() == TYPEID_PLAYER)
                            if (GameObject* obj = target->GetGameObject(48018))
                            {
                                // Soulburn: Demonic Circle
                                if (target->HasAura(74434))
                                    target->CastSpell(target, 79438, true);

                                target->ToPlayer()->TeleportTo(obj->GetMapId(),obj->GetPositionX(),obj->GetPositionY(),obj->GetPositionZ(),obj->GetOrientation());
                                target->ToPlayer()->RemoveMovementImpairingAuras();
                            }
                        break;
                    case 6358: // Seduction (succubus)
                        // Glyph of Seduction
                        if (caster && caster->GetCharmerOrOwnerPlayerOrPlayerItself() && caster->GetCharmerOrOwnerPlayerOrPlayerItself()->HasAura(56250))
                        {
                            target->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
                            target->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE_PERCENT);
                        }
                        break;
                    case 1490: // Curse of the Elements
                        if (!caster || !caster->ToPlayer())
                            break;

                        // Jinx, casting Jinx: Curse of the Elements
                        if (caster->HasAura(85479))
                            caster->CastSpell(target, 86105, true);
                        else if (caster->HasAura(18179))
                            caster->CastSpell(target, 85547, true);
                        break;
                }
                break;
            case SPELLFAMILY_PRIEST:
                if (!caster)
                    break;
                // Devouring Plague
                if (GetSpellProto()->SpellFamilyFlags[0] & 0x02000000 && GetEffect(0))
                {
                    // Improved Devouring Plague
                    if (AuraEffect const * aurEff = caster->GetDummyAuraEffect(SPELLFAMILY_PRIEST, 3790, 0))
                    {
                        int32 basepoints0 = aurEff->GetAmount() * GetEffect(0)->GetTotalTicks() * caster->SpellDamageBonus(target, GetSpellProto(), 0, GetEffect(0)->GetAmount(), DOT) / 100;
                        caster->CastCustomSpell(target, 63675, &basepoints0, NULL, NULL, true, NULL, GetEffect(0));
                    }
                }
                // Renew
                else if (GetSpellProto()->SpellFamilyFlags[0] & 0x00000040 && GetEffect(0))
                {
                    // Divine Touch (old Empowered Renew)
                    if (AuraEffect const * aurEff = caster->GetDummyAuraEffect(SPELLFAMILY_PRIEST, 3021, 0))
                    {
                        int32 basepoints0 = aurEff->GetAmount() * GetEffect(0)->GetTotalTicks() * caster->SpellHealingBonus(target, GetSpellProto(), 0, GetEffect(0)->GetAmount(), HEAL) / 100;
                        caster->CastCustomSpell(target, 63544, &basepoints0, NULL, NULL, true, NULL, GetEffect(0));
                    }
                }
                switch(GetId())
                {
                // Fade
                case 586:
                    {
                        if (caster && caster->ToPlayer())
                        {
                            // Phantasm
                            if (caster->HasAura(47570) || (caster->HasAura(47569) && roll_chance_i(50)))
                                caster->RemoveMovementImpairingAuras();
                        }
                    }
                    break;
                // Dispersion
                case 47585:
                    {
                        // apply snare and movement impairing effects immunity
                        caster->CastSpell(caster, 63230, true);
                        break;
                    }
                // Power Word: Shield
                case 17:
                    {
                        if (GetEffect(0))
                        {
                            // Glyph of Power Word: Shield
                            if (AuraEffect* glyph = caster->GetAuraEffect(55672,0))
                            {
                                // instantly heal m_amount% of the absorb-value
                                int32 heal = glyph->GetAmount() * GetEffect(0)->GetAmount() / 100;
                                caster->CastCustomSpell(target, 56160, &heal, NULL, NULL, true, 0, GetEffect(0));
                            }
                        }
                        if (!target)
                            break;

                        // Body and Soul
                        if (caster->HasAura(64127))                 // rank #1
                            caster->CastSpell(target, 64128, true); // Increase speed of the target by 30%
                        else if (caster->HasAura(64129))            // rank #2
                            caster->CastSpell(target, 65081, true); // Increase speed of the target by 60%

                        // Holy Walk (Gladiator 4/5 set bonus)
                        if (caster->HasAura(33333) && caster == target)
                            target->CastSpell(target, 96219, true);      // Suppress movement speed reduction when cast on self

                        break;
                    }
                // Resurrection Sickness for special iCe purposes
                // weirdly SPELLFAMILY_PRIEST
                case 15007:
                    if (target->GetMapId() == 746)  // Banana Plantation (jail map)
                    {
                        SetMaxDuration(-1);         // duration: persistant
                        RefreshDuration();
                    }
                    break;
                default: break;
                }
                break;
            case SPELLFAMILY_ROGUE:
                // Sprint (skip non player casted spells by category)
                if (GetSpellProto()->SpellFamilyFlags[0] & 0x40 && GetSpellProto()->Category == 44)
                    // in official maybe there is only one icon?
                    if (target->HasAura(58039)) // Glyph of Blurred Speed
                        target->CastSpell(target, 61922, true); // Sprint (waterwalk)

                // Deadly Momentum
                if (GetId() == 84590)
                {
                    if (Aura* snd = target->GetAura(5171))
                        snd->RefreshDuration();
                    if (Aura* rec = target->GetAura(73651))
                        rec->RefreshDuration();
                }
                // Killing Spree
                else if (GetId() == 51690)
                {
                    if (caster)
                    {
                        caster->CastSpell(caster, 69107, true); // invisibility aura
                        caster->CastSpell(caster, 61851, true); // + damage aura
                    }
                }
                // Cheap Shot, Garrote
                if (apply && (GetSpellProto()->Id == 1833 || GetSpellProto()->Id == 703)) //1330
                {
                    if (Unit* caster = aurApp->GetBase()->GetCaster())
                    {
                        int32 bp0 = 0;
                        // Find Weakness
                        if (caster->HasAura(51632)) // Rank #1
                            bp0 = 35;
                        else if (caster->HasAura(91023)) // Rank #2
                            bp0 = 70;

                        if (bp0) // Apply debuff
                            caster->CastCustomSpell(target, 91021, &bp0, NULL, NULL, true);
                    }
                }
                // Blind
                if (apply && (GetSpellProto()->Id == 2094))
                {
                    if (Unit* caster = aurApp->GetBase()->GetCaster())
                    {
                        // Glyph of Blind
                        if (caster->HasAura(91299))
                        {
                            target->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
                            target->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE_PERCENT);
                        }
                    }
                }
                // Rupture
                if (GetSpellProto()->Id == 1943)
                {
                    // Restless Blades
                    if (Unit* caster = aurApp->GetBase()->GetCaster())
                    {
                        int32 minusSecs = 0;
                        if (caster->HasAura(79096))
                            minusSecs = 2*caster->ToPlayer()->GetComboPoints();
                        else if (caster->HasAura(79095))
                            minusSecs = 1*caster->ToPlayer()->GetComboPoints();

                        // If present, modify cooldown of some spells
                        if (minusSecs)
                        {
                            caster->ToPlayer()->ModifySpellCooldown(13750, -minusSecs*1000, true); // Adrenaline Rush
                            caster->ToPlayer()->ModifySpellCooldown(2983 , -minusSecs*1000, true); // Sprint
                            caster->ToPlayer()->ModifySpellCooldown(51690, -minusSecs*1000, true); // Killing Spree
                            caster->ToPlayer()->ModifySpellCooldown(73981, -minusSecs*1000, true); // Redirect
                        }
                    }
                }
                break;
            case SPELLFAMILY_PALADIN: // Speed of Light (talent)
                if(GetId() == 82327)
                {
                    if(target->HasAura(85495)) // r1
                    {
                        target->CastSpell(target, 85497, true);
                        target->SetSpeed(MOVE_RUN, 1.2f, true);
                    }
                    if(target->HasAura(85498)) // r2
                    {
                        target->CastSpell(target, 85497, true);
                        target->SetSpeed(MOVE_RUN, 1.4f, true);
                    }
                    if(target->HasAura(85499)) // r3
                    {
                        target->CastSpell(target, 85497, true);
                        target->SetSpeed(MOVE_RUN, 1.6f, true);
                    }
                }
                // Inquisition
                else if(GetId() == 84963)
                {
                    if(caster)
                    {
                        uint32 holypower = caster->GetPower(POWER_HOLY_POWER) + 1;
                        if (caster->HasAura(90174))
                        {
                            holypower = 3;
                            caster->RemoveAurasDueToSpell(90174);
                        }
                        SetDuration(GetMaxDuration() * (caster->GetPower(POWER_HOLY_POWER) + 1));
                        caster->SetPower(POWER_HOLY_POWER, 0);
                    }
                }
                // Avenging Wrath
                else if (GetId() == 31884)
                {
                    if (caster)
                    {
                        if (apply)
                        {
                            // Sanctified Wrath
                            if (caster->HasAura(53376) || caster->HasAura(90286) || caster->HasAura(53375))
                                caster->CastSpell(caster, 57318, true);
                        }
                        else
                            caster->RemoveAurasDueToSpell(57318);
                    }
                }
                break;
            case SPELLFAMILY_SHAMAN:
                {
                    // Spirit Walk
                    if (caster && target && GetId() == 58875)
                    {
                        // Removes snares and roots.
                        target->RemoveMovementImpairingAuras();
                        target->CastSpell(target, 58876, true);
                        break;
                    }
                }
                break;
            case SPELLFAMILY_HUNTER:
                {
                    // Serpent Sting
                    if (caster && GetId() == 1978 && (caster->HasAura(82834) || caster->HasAura(19464)))
                    {
                        if (AuraEffect* pEff = GetEffect(0))
                        {
                            int32 bp0 = pEff->GetAmount() * (float(GetMaxDuration()) / float(pEff->GetAmplitude()));
                            // Improved Serpent Sting - deal % of total damage done
                            if (caster->HasAura(82834))      // rank 2
                                bp0 *= 0.3f;
                            else if (caster->HasAura(19464)) // rank 1
                                bp0 *= 0.15f;

                            if (bp0)
                                caster->CastCustomSpell(target, 83077, &bp0, 0, 0, true);
                        }
                    }
                    // Misdirection
                    else if (GetId() == 34477 &&
                        caster && target &&
                        caster->GetTypeId() == TYPEID_PLAYER &&
                        caster->HasAura(56829) &&                   // Glyph of Misdirection
                        caster->GetPetGUID() == target->GetGUID())  // casters pet is the target
                    {
                        caster->ToPlayer()->RemoveSpellCooldown(34477);
                    }
                }
                break;
            case SPELLFAMILY_DRUID:
                {
                    // Rejuvenation
                    if (caster && target && GetId() == 774)
                    {
                        // Gift of the Earthmother
                        int32 bp0 = 0;
                        if (caster->HasAura(51181))
                            bp0 = 15;
                        else if (caster->HasAura(51180))
                            bp0 = 10;
                        else if (caster->HasAura(51179))
                            bp0 = 5;

                        AuraEffect* pEff = aurApp->GetBase()->GetEffect(0);

                        if (!pEff || bp0 <= 0)
                            break;

                        uint32 pheal = pEff->GetAmount() > 0 ? pEff->GetAmount()*4 : 0;
                        pheal = caster->SpellHealingBonus(target, GetSpellProto(), 0, pheal, DOT);
                        bp0 = bp0*pheal/100;
                        caster->CastCustomSpell(target, 64801, &bp0, 0, 0, true);
                        break;
                    }
                    // Shooting Stars - reset cooldown of Starsurge
                    if (GetId() == 93400 && caster && caster->ToPlayer())
                        caster->ToPlayer()->RemoveSpellCooldown(78674, true);
                }
                break;
            case SPELLFAMILY_DEATHKNIGHT:
                if (!caster)
                    break;
                // Frost Fever and Blood Plague
                if (GetSpellProto()->SpellFamilyFlags[2] & 0x2)
                {
                    // Can't proc on self
                    if (GetCasterGUID() == target->GetGUID())
                        break;

                    AuraEffect * aurEff = NULL;
                    // Ebon Plaguebringer / Crypt Fever
                    Unit::AuraEffectList const& TalentAuras = caster->GetAuraEffectsByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
                    for (Unit::AuraEffectList::const_iterator itr = TalentAuras.begin(); itr != TalentAuras.end(); ++itr)
                    {
                        if ((*itr)->GetMiscValue() == 7282)
                        {
                            aurEff = *itr;
                            // Ebon Plaguebringer - end search if found
                            if ((*itr)->GetSpellProto()->SpellIconID == 1766)
                                break;
                        }
                    }
                    if (aurEff)
                    {
                        uint32 spellId = 0;
                        switch (aurEff->GetId())
                        {
                            // Crypt Fever
                            case 49632: spellId = 50510; break;
                            case 49631: spellId = 50509; break;
                            case 49032: spellId = 50508; break;
                            default:
                                sLog->outError("Aura::HandleAuraSpecificMods: Unknown rank of Crypt Fever/Ebon Plague (%d) found", aurEff->GetId());
                        }
                        caster->CastSpell(target, spellId, true, 0, GetEffect(0));
                    }
                }
                break;
        }

        // Paladin's talent Pursuit of Justice
        // Must be written out of switch, because of different spell families of source spells
        if (target && ((target->HasAura(26022) && roll_chance_i(50)) || target->HasAura(26023)))
        {
            if (aurApp->GetBase() && aurApp->GetBase()->GetSpellProto())
            {
                SpellEntry const* spellInfo = aurApp->GetBase()->GetSpellProto();
                if (spellInfo->AppliesAuraType(SPELL_AURA_MOD_STUN) || spellInfo->AppliesAuraType(SPELL_AURA_MOD_FEAR) || spellInfo->AppliesAuraType(SPELL_AURA_MOD_ROOT))
                    target->CastSpell(target, 89024, true);
            }
        }
    }
    // mods at aura remove
    else
    {
        // Remove Linked Auras
        if (removeMode != AURA_REMOVE_BY_STACK && removeMode != AURA_REMOVE_BY_DEATH)
        {
            if (uint32 customAttr = sSpellMgr->GetSpellCustomAttr(GetId()))
            {
                if (customAttr & SPELL_ATTR0_CU_LINK_REMOVE)
                {
                    if (const std::vector<int32> *spell_triggered = sSpellMgr->GetSpellLinked(-(int32)GetId()))
                        for (std::vector<int32>::const_iterator itr = spell_triggered->begin(); itr != spell_triggered->end(); ++itr)
                        {
                            if (*itr < 0)
                                target->RemoveAurasDueToSpell(-(*itr));
                            else if (removeMode != AURA_REMOVE_BY_DEFAULT)
                                target->CastSpell(target, *itr, true, NULL, NULL, GetCasterGUID());
                        }
                }
                if (customAttr & SPELL_ATTR0_CU_LINK_AURA)
                {
                    if (const std::vector<int32> *spell_triggered = sSpellMgr->GetSpellLinked(GetId() + SPELL_LINK_AURA))
                        for (std::vector<int32>::const_iterator itr = spell_triggered->begin(); itr != spell_triggered->end(); ++itr)
                        {
                            if (*itr < 0)
                                target->ApplySpellImmune(GetId(), IMMUNITY_ID, -(*itr), false);
                            else
                                target->RemoveAurasDueToSpell(*itr);
                        }
                }
            }
        }
        switch(GetSpellProto()->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
                switch(GetId())
                {
                    case 61987: // Avenging Wrath
                        // Remove the immunity shield marker on Avenging Wrath removal if Forbearance is not present
                        if (target->HasAura(61988) && !target->HasAura(25771))
                            target->RemoveAura(61988);
                        break;
                    case 72368: // Shared Suffering
                    case 72369:
                        if (caster)
                        {
                            if (AuraEffect* aurEff = GetEffect(0))
                            {
                                int32 remainingDamage = aurEff->GetAmount() * (aurEff->GetTotalTicks() - aurEff->GetTickNumber());
                                if (remainingDamage > 0)
                                    caster->CastCustomSpell(caster, 72373, NULL, &remainingDamage, NULL, true);
                            }
                        }
                        break;
                    case 91296: case 91308: // Corrupted Egg Shell (trinket)
                        if (target->getPowerType() != POWER_MANA)
                            break;
                        if (caster)
                        {
                            uint32 base_points = (GetId() == 91296 ? 5040 : 5700); // normal / heroic
                            caster->EnergizeBySpell(target, GetId(), base_points, POWER_MANA);
                        }
                        break;
                    case 43681: // Inactive (Report AFK)
                        if (removeMode == AURA_REMOVE_BY_EXPIRE)
                            caster->ToPlayer()->LeaveBattleground(); // Leave Battleground
                        break;
                }
                break;
            case SPELLFAMILY_MAGE:
                switch(GetId())
                {
                    case 66: // Invisibility
                        if (removeMode != AURA_REMOVE_BY_EXPIRE)
                            break;
                        // Force Water Elemental to also cast his Invisibility
                        if (target->GetPetGUID())
                            if (Unit* pPet = Unit::GetUnit(*target, target->GetPetGUID()))
                                if (pPet && pPet->ToPet() && pPet->ToPet()->GetEntry() == 510)
                                    pPet->CastSpell(pPet, 96243, true);
                        target->CastSpell(target, 32612, true, NULL, GetEffect(1));
                        target->CombatStop(); // Drop all threat
                        break;
                    case 32612: // Invisibility (real invisible mod aura)
                        // Force Water Elemental to also remove his Invisibility
                        if (target->GetPetGUID())
                            if (Unit* pPet = Unit::GetUnit(*target, target->GetPetGUID()))
                                if (pPet && pPet->ToPet() && pPet->ToPet()->GetEntry() == 510)
                                    pPet->RemoveAurasDueToSpell(96243);
                        break;
                    case 44401: //Missile Barrage
                    case 48108: //Hot Streak
                    case 57761: //Fireball!
                        if (removeMode != AURA_REMOVE_BY_EXPIRE || aurApp->GetBase()->IsExpired())
                            break;
                        if (target->HasAura(70752)) //Item - Mage T10 2P Bonus
                            target->CastSpell(target, 70753, true);
                        break;
                    default:
                        break;
                }
                break;
            case SPELLFAMILY_WARRIOR:
                if (!caster)
                    break;
                // Spell Reflection
                if (GetSpellProto()->SpellFamilyFlags[1] & 0x2)
                {
                    if (removeMode != AURA_REMOVE_BY_DEFAULT)
                    {
                        // Improved Spell Reflection
                        if (caster->GetDummyAuraEffect(SPELLFAMILY_WARRIOR,1935, 1))
                        {
                            // aura remove - remove auras from all party members
                            std::list<Unit*> PartyMembers;
                            target->GetPartyMembers(PartyMembers);
                            for (std::list<Unit*>::iterator itr = PartyMembers.begin(); itr != PartyMembers.end(); ++itr)
                            {
                                if ((*itr)!= target)
                                    (*itr)->RemoveAurasWithFamily(SPELLFAMILY_WARRIOR, 0, 0x2, 0, GetCasterGUID());
                            }
                        }
                    }
                }
                break;
            case SPELLFAMILY_WARLOCK:
                if (!caster)
                    break;
                // Curse of Doom
                if (GetSpellProto()->SpellFamilyFlags[1] & 0x02)
                {
                    if (removeMode == AURA_REMOVE_BY_DEATH)
                    {
                        if (caster->GetTypeId() == TYPEID_PLAYER && caster->ToPlayer()->isHonorOrXPTarget(target))
                            caster->CastSpell(target, 18662, true, NULL, GetEffect(0));
                    }
                }
                // Improved Fear
                else if (GetSpellProto()->SpellFamilyFlags[1] & 0x00000400)
                {
                    if (AuraEffect* aurEff = caster->GetAuraEffect(SPELL_AURA_DUMMY, SPELLFAMILY_WARLOCK, 98, 0))
                    {
                        uint32 spellId = 0;
                        switch (aurEff->GetId())
                        {
                            case 53759: spellId = 60947; break;
                            case 53754: spellId = 60946; break;
                            default:
                                sLog->outError("Aura::HandleAuraSpecificMods: Unknown rank of Improved Fear (%d) found", aurEff->GetId());
                        }
                        if (spellId)
                            caster->CastSpell(target, spellId, true);
                    }
                }
                switch(GetId())
                {
                    case 48018: // Demonic Circle
                        // Do not remove GO when aura is removed by stack
                        // to prevent remove GO added by new spell
                        // old one is already removed
                        if (removeMode != AURA_REMOVE_BY_STACK)
                            target->RemoveGameObject(GetId(), true);
                        target->RemoveAura(62388);
                        break;
                    default:
                        break;
                }
                break;
            case SPELLFAMILY_PRIEST:
                if (!caster)
                    break;
                // Shadow word: Pain // Vampiric Touch
                if (removeMode == AURA_REMOVE_BY_ENEMY_SPELL && (GetSpellProto()->SpellFamilyFlags[0] & 0x00008000 || GetSpellProto()->SpellFamilyFlags[1] & 0x00000400))
                {
                    // Shadow Affinity
                    if (AuraEffect const * aurEff = caster->GetDummyAuraEffect(SPELLFAMILY_PRIEST, 178, 1))
                    {
                        int32 basepoints0 = aurEff->GetAmount() * caster->GetCreateMana() / 100;
                        caster->CastCustomSpell(caster, 64103, &basepoints0, NULL, NULL, true, NULL, GetEffect(0));
                    }
                }
                // Power word: shield
                else if (removeMode == AURA_REMOVE_BY_ENEMY_SPELL && GetSpellProto()->SpellFamilyFlags[0] & 0x00000001)
                {
                    // Rapture
                    if (Aura const * aura = caster->GetAuraOfRankedSpell(47535))
                    {
                        // check cooldown
                        if (caster->HasAura(63853))
                            break;
                        // and add if needed
                        caster->AddAura(63853, caster);

                        // energize the caster
                        if (AuraEffect const * aurEff = aura->GetEffect(0))
                        {
                            float multiplier = (float)aurEff->GetAmount();

                            int32 basepoints0 = int32(multiplier * caster->GetMaxPower(POWER_MANA) / 100);
                            caster->CastCustomSpell(caster, 47755, &basepoints0, NULL, NULL, true);
                        }
                    }
                }
                switch(GetId())
                {
                    /*
                    case 47788: // Guardian Spirit --no longer needed
                        if (removeMode != AURA_REMOVE_BY_EXPIRE)
                            break;
                        if (caster->GetTypeId() != TYPEID_PLAYER)
                            break;

                        Player *player = caster->ToPlayer();
                        // Glyph of Guardian Spirit
                        if (AuraEffect * aurEff = player->GetAuraEffect(63231, 0))
                        {
                            if (!player->HasSpellCooldown(47788))
                                break;

                            player->RemoveSpellCooldown(GetSpellProto()->Id, true);
                            player->AddSpellCooldown(GetSpellProto()->Id, 0, uint32(time(NULL) + aurEff->GetAmount()));

                            WorldPacket data(SMSG_SPELL_COOLDOWN, 8+1+4+4);
                            data << uint64(player->GetGUID());
                            data << uint8(0x0);                                     // flags (0x1, 0x2)
                            data << uint32(GetSpellProto()->Id);
                            data << uint32(aurEff->GetAmount()*IN_MILLISECONDS);
                            player->SendDirectMessage(&data);
                        }
                        break;
                    */
                    // Spirit of Redemption - linked auras
                    case 27792:
                    case 27795:
                    case 27827:
                        {
                            caster->RemoveAurasDueToSpell(27792);
                            caster->RemoveAurasDueToSpell(27795);
                            break;
                        }
                    // Evangelism / Dark Evangelism rank 2
                    case 81661:
                    case 87118:
                        {
                            // Disable spell
                            caster->RemoveAurasDueToSpell(87154);
                            break;
                        }
                    default: break;
                }
                break;
            case SPELLFAMILY_ROGUE:
                // Remove Vanish on stealth remove
                if (GetId() == 1784)
                    target->RemoveAurasWithFamily(SPELLFAMILY_ROGUE, 0x0000800, 0, 0, target->GetGUID());
                else if (GetId() == 6770) // On-sap removal - blackjack talent
                {    
                    if (caster->HasAura(79125)) // rank 2
                        caster->CastSpell(target, 79126, true);
                    else if (caster->HasAura(79123)) // rank 1
                        caster->CastSpell(target, 79124, true);
                }
                else if (GetId() == 84748) // Bandit's Guile
                {
                    if (caster && removeMode != AURA_REMOVE_BY_STACK)
                    {
                        // Remove also dummy auras from player when removing main effect
                        caster->RemoveAurasDueToSpell(84745);
                        caster->RemoveAurasDueToSpell(84746);
                        caster->RemoveAurasDueToSpell(84747);
                    }
                }
                break;
            case SPELLFAMILY_PALADIN:
                // Remove the immunity shield marker on Forbearance removal if AW marker is not present
                if (GetId() == 25771 && target->HasAura(61988) && !target->HasAura(61987))
                    target->RemoveAura(61988);
                // Guardian of Ancient Kings - Retribution
                else if (GetId() == 86698)
                {
                    target->RemoveAurasDueToSpell(86701);
                    if(target->HasAura(86700)) // Ancient Power
                    {
                        target->CastSpell(target, 86704, true); // Ancient Fury
                        target->RemoveAurasDueToSpell(86700);
                    }
                }
                // Guardian of Ancient Kings (Ancient Healer)
                else if (GetId() == 86674)
                {
                    target->RemoveAurasDueToSpell(86669);
                }
                break;
            case SPELLFAMILY_DEATHKNIGHT:
                // Blood of the North
                // Reaping
                // Death Rune Mastery
                if (GetSpellProto()->SpellIconID == 3041 || GetSpellProto()->SpellIconID == 22 || GetSpellProto()->SpellIconID == 2622)
                {
                    if (!GetEffect(0) || GetEffect(0)->GetAuraType() != SPELL_AURA_PERIODIC_DUMMY)
                        break;
                    if (target->GetTypeId() != TYPEID_PLAYER)
                        break;
                    if (target->ToPlayer()->getClass() != CLASS_DEATH_KNIGHT)
                        break;

                     // aura removed - remove death runes
                    target->ToPlayer()->RemoveRunesByAuraEffect(GetEffect(0));
                }
                switch(GetId())
                {
                    case 50514: // Summon Gargoyle
                        if (removeMode != AURA_REMOVE_BY_EXPIRE)
                            break;
                        target->CastSpell(target, GetEffect(0)->GetAmount(), true, NULL, GetEffect(0));
                        break;
                }
                break;
            case SPELLFAMILY_HUNTER:
                // Glyph of Freezing Trap
                if (GetSpellProto()->SpellFamilyFlags[0] & 0x00000008)
                    if (caster && caster->HasAura(56845))
                        target->CastSpell(target, 61394, true);
                break;
        }
    }

    // mods at aura apply or remove
    switch (GetSpellProto()->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
            switch (GetId())
            {
                case 50720: // Vigilance
                    if (apply)
                        target->CastSpell(caster, 59665, true, 0, 0, caster->GetGUID());
                    else
                        target->SetReducedThreatPercent(0,0);
                    break;
            }
            break;
        case SPELLFAMILY_ROGUE:
            // Stealth
            if (GetSpellProto()->SpellFamilyFlags[0] & 0x00400000)
            {
                // Master of subtlety
                if (AuraEffect const * aurEff = target->GetAuraEffect(31223, 0))
                {
                    if (!apply)
                        target->CastSpell(target,31666,true);
                    else
                    {
                        int32 basepoints0 = aurEff->GetAmount();
                        target->CastCustomSpell(target,31665, &basepoints0, NULL, NULL ,true);
                    }
                }
                // Overkill
                if (target->HasAura(58426))
                {
                    if (!apply)
                        target->CastSpell(target,58428,true);
                    else
                        target->CastSpell(target,58427,true);
                }
                break;
            }
            break;
        case SPELLFAMILY_DRUID:
            {
                // Tree of Life, passive auras
                if (GetId() == 33891)
                {
                    // Tree of Life (passive)
                    if (apply)
                    {
                        // +15% heal, +120% armor
                        target->CastSpell(target, 5420, true);
                        // instant Regrowth and Ent. Roots, +30% dmg Wrath, +200% dmg Ent. Roots
                        target->CastSpell(target, 81097, true);
                        // -50% Wrath cast time, and dummy Lifebloom aura, don't care.. :-P
                        target->CastSpell(target, 81098, true);
                    }
                    else
                    {
                        target->RemoveAurasDueToSpell(5420);
                        target->RemoveAurasDueToSpell(81097);
                        target->RemoveAurasDueToSpell(81098);
                    }
                }
                // Enrage and King of the Jungle talent
                else if (GetId() == 5229)
                {
                    if (apply)
                    {
                        int32 bp0 = 0;
                        if (caster->HasAura(48495))
                            bp0 = 15;
                        else if (caster->HasAura(48494))
                            bp0 = 10;
                        else if (caster->HasAura(48492))
                            bp0 = 5;

                        if (bp0)
                            caster->CastCustomSpell(caster, 51185, &bp0, 0, 0, true);
                    }
                    else
                        caster->RemoveAurasDueToSpell(51185);
                }
                // Tiger's Fury and King of the Jungle talent
                else if (GetId() == 5217)
                {
                    if (apply)
                    {
                        int32 bp0 = 0;
                        if (caster->HasAura(48495))
                            bp0 = 60;
                        else if (caster->HasAura(48494))
                            bp0 = 40;
                        else if (caster->HasAura(48492))
                            bp0 = 20;

                        if (bp0)
                            caster->CastCustomSpell(caster, 51178, &bp0, 0, 0, true);
                    }
                }
                // Eclipse (Solar)
                else if (GetId() == 48517)
                {
                    // Cast spell, which adds aura to morph Starfire into Sunfire
                    // Condition to talent Sunfire
                    if (apply && caster->HasAura(93401))
                        caster->CastSpell(caster, 94338, true);
                    else
                        caster->RemoveAurasDueToSpell(94338);
                }

                // Vengeance for druids - remove at bear form unapply
                if (GetId() == 5487 && !apply && caster && caster->HasAura(76691))
                    caster->RemoveAurasDueToSpell(76691);

                // Tiger's Fury, Berserk with Primal Madness talent
                if (GetId() == 5217 || GetId() == 50334)
                {
                    if (apply)
                    {
                        if (caster->HasAura(80317))
                            caster->CastSpell(caster, 80886, true);
                        else if (caster->HasAura(80316))
                            caster->CastSpell(caster, 80879, true);
                    }
                    else
                    {
                        if ((GetId() == 5217 && !caster->HasAura(50334)) ||
                            (GetId() == 50334 && !caster->HasAura(5217)))
                        {
                            caster->RemoveAurasDueToSpell(80879);
                            caster->RemoveAurasDueToSpell(80886);
                        }
                    }
                }
            }
        case SPELLFAMILY_HUNTER:
            switch(GetId())
            {
                case 19574: // Bestial Wrath
                    // The Beast Within cast on owner if talent present
                    if (Unit* owner = target->GetOwner())
                    {
                        // Search talent
                        if (owner->HasAura(34692))
                        {
                            if (apply)
                                owner->CastSpell(owner, 34471, true, 0, GetEffect(0));
                            else
                                owner->RemoveAurasDueToSpell(34471);
                        }
                    }
                    break;
            }
            break;
        case SPELLFAMILY_PALADIN:
            switch(GetId())
            {
                case 19746:
                case 31821:
                    // Aura Mastery Triggered Spell Handler
                    // If apply Concentration Aura -> trigger -> apply Aura Mastery Immunity
                    // If remove Concentration Aura -> trigger -> remove Aura Mastery Immunity
                    // If remove Aura Mastery -> trigger -> remove Aura Mastery Immunity
                    // Do effects only on aura owner
                    if (GetCasterGUID() != target->GetGUID())
                        break;
                    if (apply)
                    {
                        if ((GetSpellProto()->Id == 31821 && target->HasAura(19746, GetCasterGUID())) || (GetSpellProto()->Id == 19746 && target->HasAura(31821)))
                            target->CastSpell(target,64364,true);
                    }
                    else
                        target->RemoveAurasDueToSpell(64364, GetCasterGUID());
                    break;
            }
            break;
        case SPELLFAMILY_DEATHKNIGHT:
            // Blood Presence
            if (GetId() == 48263)
            {
                // Blood Presence enables using of various abilities such as Rune Strike
                caster->ModifyAuraState(AURA_STATE_DEFENSE, apply);
            }

            // Blood and Frost Presence
            if (GetId() == 48263 || GetId() == 48266)
            {
                if (apply)
                {
                    // Improved Unholy Presence - increase movement speed in other stances
                    int32 bp0 = 0;
                    if (caster && caster->HasAura(50392))
                        bp0 = 15;
                    else if (caster && caster->HasAura(50391))
                        bp0 = 8;

                    if (bp0)
                        caster->CastCustomSpell(caster, 63622, &bp0, 0, 0, true);
                }
                else
                    caster->RemoveAurasDueToSpell(63622);
            }

            if (GetSpellSpecific(GetSpellProto()) == SPELL_SPECIFIC_PRESENCE)
            {
                AuraEffect *bloodPresenceAura=0;  // healing by damage done
                AuraEffect *frostPresenceAura=0;  // increased health
                AuraEffect *unholyPresenceAura=0; // increased movement speed, faster rune recovery

                // Consume all runic power when switching presences
                caster->SetPower(POWER_RUNIC_POWER, 0);

                // Improved Presences
                Unit::AuraEffectList const& vDummyAuras = target->GetAuraEffectsByType(SPELL_AURA_DUMMY);
                for (Unit::AuraEffectList::const_iterator itr = vDummyAuras.begin(); itr != vDummyAuras.end(); ++itr)
                {
                    switch((*itr)->GetId())
                    {
                        // Improved Blood Presence
                        case 50365:
                        case 50371:
                        {
                            bloodPresenceAura = (*itr);
                            break;
                        }
                        // Improved Frost Presence
                        case 50384:
                        case 50385:
                        {
                            frostPresenceAura = (*itr);
                            break;
                        }
                        // Improved Unholy Presence
                        case 50391:
                        case 50392:
                        {
                            unholyPresenceAura = (*itr);
                            break;
                        }
                    }
                }

                uint32 presence=GetId();
                if (apply)
                {
                    // Blood Presence bonus
                    if (presence == 48266)
                        target->CastSpell(target, 63611, true);
                    else if (bloodPresenceAura)
                    {
                        int32 basePoints1=bloodPresenceAura->GetAmount();
                        target->CastCustomSpell(target,63611,NULL,&basePoints1,NULL,true,0,bloodPresenceAura);
                    }
                    // Frost Presence bonus
                    if (presence == 48263)
                        target->CastSpell(target, 61261, true);
                    else if (frostPresenceAura)
                    {
                        int32 basePoints0=frostPresenceAura->GetAmount();
                        target->CastCustomSpell(target,61261,&basePoints0,NULL,NULL,true,0,frostPresenceAura);
                    }
                    // Unholy Presence bonus
                    if (presence == 48265)
                    {
                        if (unholyPresenceAura)
                        {
                            // Not listed as any effect, only base points set
                            int32 basePoints0 = SpellMgr::CalculateSpellEffectAmount(unholyPresenceAura->GetSpellProto(), 1);
                            target->CastCustomSpell(target,63622,&basePoints0 ,&basePoints0,&basePoints0,true,0,unholyPresenceAura);
                            target->CastCustomSpell(target,65095,&basePoints0 ,NULL,NULL,true,0,unholyPresenceAura);
                        }
                        target->CastSpell(target,49772, true);
                    }
                    else if (unholyPresenceAura)
                    {
                        int32 basePoints0=unholyPresenceAura->GetAmount();
                        target->CastCustomSpell(target,49772,&basePoints0,NULL,NULL,true,0,unholyPresenceAura);
                    }
                }
                else
                {
                    // Remove passive auras
                    if (presence == 48266 || bloodPresenceAura)
                        target->RemoveAurasDueToSpell(63611);
                    if (presence == 48263 || frostPresenceAura)
                        target->RemoveAurasDueToSpell(61261);
                    if (presence == 48265 || unholyPresenceAura)
                    {
                        if (presence == 48265 && unholyPresenceAura)
                        {
                            target->RemoveAurasDueToSpell(63622);
                            target->RemoveAurasDueToSpell(65095);
                        }
                        target->RemoveAurasDueToSpell(49772);
                    }
                }
            }
            break;
        case SPELLFAMILY_WARRIOR:
            {
                uint32 entry = GetId();
                // Enrage
                if (entry == 12880 || entry == 14201 || entry == 14202)
                    caster->ModifyAuraState(AURA_STATE_ENRAGE, apply);
            }
            break;
        case SPELLFAMILY_WARLOCK:
            // Drain Soul - If the target is at or below 25% health, Drain Soul causes four times the normal damage
            if (GetSpellProto()->SpellFamilyFlags[0] & 0x00004000)
            {
                if (!caster)
                    break;
                if (apply)
                {
                    if (target != caster && !target->HealthAbovePct(25))
                        caster->CastSpell(caster, 200000, true);
                }
                else
                {
                    if (target != caster)
                        caster->RemoveAurasDueToSpell(GetId());
                    else
                        caster->RemoveAurasDueToSpell(200000);
                }
            }
            // Curse of Weakness
            else if (GetSpellProto()->Id == 702 && caster && caster->ToPlayer() && target)
            {
                if (apply)
                {
                    // Jinx, casting CotW bonus
                    int32 bp0 = 0;
                    if (caster->HasAura(85479))
                        bp0 = -10;
                    else if (caster->HasAura(18179))
                        bp0 = -5;

                    if (bp0 != 0)
                    {
                        if (target->getPowerType() == POWER_RUNIC_POWER)
                            caster->CastCustomSpell(target, 85541, &bp0, 0, 0, true);
                        else if (target->getPowerType() == POWER_FOCUS)
                            caster->CastCustomSpell(target, 85542, &bp0, 0, 0, true);
                        else if (target->getPowerType() == POWER_ENERGY)
                            caster->CastCustomSpell(target, 85540, &bp0, 0, 0, true);
                        else if (target->getPowerType() == POWER_RAGE)
                            caster->CastCustomSpell(target, 85539, &bp0, 0, 0, true);
                    }
                }
                else
                {
                    // Remove all possible Jinxes
                    target->RemoveAurasDueToSpell(85539);
                    target->RemoveAurasDueToSpell(85540);
                    target->RemoveAurasDueToSpell(85541);
                    target->RemoveAurasDueToSpell(85542);
                }
            }
            break;
    }
}

bool Aura::CanBeAppliedOn(Unit *target)
{
    // unit not in world or during remove from world
    if (!target->IsInWorld() || target->IsDuringRemoveFromWorld())
    {
        // area auras mustn't be applied
        if (GetOwner() != target)
            return false;
        // not selfcasted single target auras mustn't be applied
        if (GetCasterGUID() != GetOwner()->GetGUID() && IsSingleTargetSpell(GetSpellProto()))
            return false;
    }
    else if (GetOwner() != target)
        return CheckAreaTarget(target);
    return true;
}

bool Aura::CheckAreaTarget(Unit *target)
{
    // for owner check use Spell::CheckTarget
    ASSERT(GetOwner() != target);

    // some special cases
    switch(GetId())
    {
        case 45828: // AV Marshal's HP/DMG auras
        case 45829:
        case 45830:
        case 45821:
        case 45822: // AV Warmaster's HP/DMG auras
        case 45823:
        case 45824:
        case 45826:
            switch(target->GetEntry())
            {
                // alliance
                case 14762: // Dun Baldar North Marshal
                case 14763: // Dun Baldar South Marshal
                case 14764: // Icewing Marshal
                case 14765: // Stonehearth Marshal
                case 11948: // Vandar Stormspike
                // horde
                case 14772: // East Frostwolf Warmaster
                case 14776: // Tower Point Warmaster
                case 14773: // Iceblood Warmaster
                case 14777: // West Frostwolf Warmaster
                case 11946: // Drek'thar
                    return true;
                default:
                    return false;
                    break;
            }
            break;
    }
    return true;
}

void Aura::_DeleteRemovedApplications()
{
    while (!m_removedApplications.empty())
    {
        delete m_removedApplications.front();
        m_removedApplications.pop_front();
    }
}

void Aura::LoadScripts()
{
    sScriptMgr->CreateAuraScripts(m_spellProto->Id, m_loadedScripts);
    for(std::list<AuraScript *>::iterator itr = m_loadedScripts.begin(); itr != m_loadedScripts.end() ;)
    {
        if (!(*itr)->_Load(this))
        {
            std::list<AuraScript *>::iterator bitr = itr;
            ++itr;
            m_loadedScripts.erase(bitr);
            continue;
        }
        (*itr)->Register();
        ++itr;
    }
}

bool Aura::CallScriptEffectApplyHandlers(AuraEffect const * aurEff, AuraApplication const * aurApp, AuraEffectHandleModes mode)
{
    bool preventDefault = false;
    for(std::list<AuraScript *>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end() ; ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_APPLY, aurApp);
        std::list<AuraScript::EffectApplyHandler>::iterator effEndItr = (*scritr)->OnEffectApply.end(), effItr = (*scritr)->OnEffectApply.begin();
        for(; effItr != effEndItr ; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellProto, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, mode);
        }
        if (!preventDefault)
            preventDefault = (*scritr)->_IsDefaultActionPrevented();
        (*scritr)->_FinishScriptCall();
    }
    return preventDefault;
}

bool Aura::CallScriptEffectRemoveHandlers(AuraEffect const * aurEff, AuraApplication const * aurApp, AuraEffectHandleModes mode)
{
    bool preventDefault = false;
    for(std::list<AuraScript *>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end() ; ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_REMOVE, aurApp);
        std::list<AuraScript::EffectApplyHandler>::iterator effEndItr = (*scritr)->OnEffectRemove.end(), effItr = (*scritr)->OnEffectRemove.begin();
        for(; effItr != effEndItr ; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellProto, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, mode);
        }
        if (!preventDefault)
            preventDefault = (*scritr)->_IsDefaultActionPrevented();
        (*scritr)->_FinishScriptCall();
    }
    return preventDefault;
}

bool Aura::CallScriptEffectPeriodicHandlers(AuraEffect const * aurEff, AuraApplication const * aurApp)
{
    bool preventDefault = false;
    for(std::list<AuraScript *>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end() ; ++scritr)
    {
        if (!(*scritr))
            continue;

        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_PERIODIC, aurApp);
        std::list<AuraScript::EffectPeriodicHandler>::iterator effEndItr = (*scritr)->OnEffectPeriodic.end(), effItr = (*scritr)->OnEffectPeriodic.begin();
        for(; effItr != effEndItr ; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellProto, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff);
        }
        if (!preventDefault)
            preventDefault = (*scritr)->_IsDefaultActionPrevented();
        (*scritr)->_FinishScriptCall();
    }
    return preventDefault;
}

void Aura::CallScriptEffectUpdatePeriodicHandlers(AuraEffect * aurEff)
{
    for(std::list<AuraScript *>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end() ; ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_UPDATE_PERIODIC);
        std::list<AuraScript::EffectUpdatePeriodicHandler>::iterator effEndItr = (*scritr)->OnEffectUpdatePeriodic.end(), effItr = (*scritr)->OnEffectUpdatePeriodic.begin();
        for(; effItr != effEndItr ; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellProto, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff);
        }
        (*scritr)->_FinishScriptCall();
    }
}

void Aura::CallScriptEffectCalcAmountHandlers(AuraEffect const * aurEff, int32 & amount, bool & canBeRecalculated)
{
    for(std::list<AuraScript *>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end() ; ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_CALC_AMOUNT);
        std::list<AuraScript::EffectCalcAmountHandler>::iterator effEndItr = (*scritr)->DoEffectCalcAmount.end(), effItr = (*scritr)->DoEffectCalcAmount.begin();
        for(; effItr != effEndItr ; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellProto, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, amount, canBeRecalculated);
        }
        (*scritr)->_FinishScriptCall();
    }
}

void Aura::CallScriptEffectCalcPeriodicHandlers(AuraEffect const * aurEff, bool & isPeriodic, int32 & amplitude)
{
    for(std::list<AuraScript *>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end() ; ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_CALC_PERIODIC);
        std::list<AuraScript::EffectCalcPeriodicHandler>::iterator effEndItr = (*scritr)->DoEffectCalcPeriodic.end(), effItr = (*scritr)->DoEffectCalcPeriodic.begin();
        for(; effItr != effEndItr ; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellProto, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, isPeriodic, amplitude);
        }
        (*scritr)->_FinishScriptCall();
    }
}

void Aura::CallScriptEffectCalcSpellModHandlers(AuraEffect const * aurEff, SpellModifier *& spellMod)
{
    for(std::list<AuraScript *>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end() ; ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_CALC_SPELLMOD);
        std::list<AuraScript::EffectCalcSpellModHandler>::iterator effEndItr = (*scritr)->DoEffectCalcSpellMod.end(), effItr = (*scritr)->DoEffectCalcSpellMod.begin();
        for(; effItr != effEndItr ; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellProto, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, spellMod);
        }
        (*scritr)->_FinishScriptCall();
    }
}

void Aura::CallScriptEffectAbsorbHandlers(AuraEffect * aurEff, AuraApplication const * aurApp, DamageInfo & dmgInfo, uint32 & absorbAmount, bool & defaultPrevented)
{
    for(std::list<AuraScript *>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end() ; ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_ABSORB, aurApp);
        std::list<AuraScript::EffectAbsorbHandler>::iterator effEndItr = (*scritr)->OnEffectAbsorb.end(), effItr = (*scritr)->OnEffectAbsorb.begin();
        for(; effItr != effEndItr ; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellProto, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, dmgInfo, absorbAmount);
        }
        if (!defaultPrevented)
            defaultPrevented = (*scritr)->_IsDefaultActionPrevented();
        (*scritr)->_FinishScriptCall();
    }
}

void Aura::CallScriptEffectAfterAbsorbHandlers(AuraEffect * aurEff, AuraApplication const * aurApp, DamageInfo & dmgInfo, uint32 & absorbAmount)
{
    for(std::list<AuraScript *>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end() ; ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_AFTER_ABSORB, aurApp);
        std::list<AuraScript::EffectAbsorbHandler>::iterator effEndItr = (*scritr)->AfterEffectAbsorb.end(), effItr = (*scritr)->AfterEffectAbsorb.begin();
        for(; effItr != effEndItr ; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellProto, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, dmgInfo, absorbAmount);
        }
        (*scritr)->_FinishScriptCall();
    }
}


void Aura::CallScriptEffectManaShieldHandlers(AuraEffect * aurEff, AuraApplication const * aurApp, DamageInfo & dmgInfo, uint32 & absorbAmount, bool & defaultPrevented)
{
    for(std::list<AuraScript *>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end() ; ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_MANASHIELD, aurApp);
        std::list<AuraScript::EffectManaShieldHandler>::iterator effEndItr = (*scritr)->OnEffectManaShield.end(), effItr = (*scritr)->OnEffectManaShield.begin();
        for(; effItr != effEndItr ; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellProto, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, dmgInfo, absorbAmount);
        }
        if (!defaultPrevented)
            defaultPrevented = (*scritr)->_IsDefaultActionPrevented();
        (*scritr)->_FinishScriptCall();
    }
}

void Aura::CallScriptEffectAfterManaShieldHandlers(AuraEffect * aurEff, AuraApplication const * aurApp, DamageInfo & dmgInfo, uint32 & absorbAmount)
{
    for(std::list<AuraScript *>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end() ; ++scritr)
    {
        (*scritr)->_PrepareScriptCall(AURA_SCRIPT_HOOK_EFFECT_AFTER_MANASHIELD, aurApp);
        std::list<AuraScript::EffectManaShieldHandler>::iterator effEndItr = (*scritr)->AfterEffectManaShield.end(), effItr = (*scritr)->AfterEffectManaShield.begin();
        for(; effItr != effEndItr ; ++effItr)
        {
            if ((*effItr).IsEffectAffected(m_spellProto, aurEff->GetEffIndex()))
                (*effItr).Call(*scritr, aurEff, dmgInfo, absorbAmount);
        }
        (*scritr)->_FinishScriptCall();
    }
}

UnitAura::UnitAura(SpellEntry const* spellproto, uint8 effMask, WorldObject * owner, Unit * caster, int32 *baseAmount, Item * castItem, uint64 casterGUID)
    : Aura(spellproto, effMask, owner, caster, baseAmount, castItem, casterGUID)
{
    m_AuraDRGroup = DIMINISHING_NONE;
    LoadScripts();
    _InitEffects(effMask, caster, baseAmount);
    GetUnitOwner()->_AddAura(this, caster);
   if (GetUnitOwner()->GetTypeId() == TYPEID_PLAYER)
       sScriptMgr->OnPlayerAura(GetUnitOwner()->ToPlayer(), spellproto);
};

void UnitAura::_ApplyForTarget(Unit * target, Unit * caster, AuraApplication * aurApp)
{
    Aura::_ApplyForTarget(target, caster, aurApp);

    // register aura diminishing on apply
    if (DiminishingGroup group = GetDiminishGroup())
        target->ApplyDiminishingAura(group,true);
}

void UnitAura::_UnapplyForTarget(Unit * target, Unit * caster, AuraApplication * aurApp)
{
    Aura::_UnapplyForTarget(target, caster, aurApp);

    // unregister aura diminishing (and store last time)
    if (DiminishingGroup group = GetDiminishGroup())
        target->ApplyDiminishingAura(group,false);
}

void UnitAura::Remove(AuraRemoveMode removeMode)
{
    if (IsRemoved())
        return;
    GetUnitOwner()->RemoveOwnedAura(this, removeMode);
}

void UnitAura::FillTargetMap(std::map<Unit *, uint8> & targets, Unit * caster)
{
    Player * modOwner = NULL;
    if (caster)
        modOwner = caster->GetSpellModOwner();

    for (uint8 effIndex = 0; effIndex < MAX_SPELL_EFFECTS ; ++effIndex)
    {
        if (!HasEffect(effIndex))
            continue;
        UnitList targetList;
        // non-area aura
        if (GetSpellProto()->Effect[effIndex] == SPELL_EFFECT_APPLY_AURA)
        {
            targetList.push_back(GetUnitOwner());
        }
        else
        {
            float radius;
            if (GetSpellProto()->Effect[effIndex] == SPELL_EFFECT_APPLY_AREA_AURA_ENEMY)
                radius = GetSpellRadiusForHostile(sSpellRadiusStore.LookupEntry(GetSpellProto()->EffectRadiusIndex[effIndex]));
            else
                radius = GetSpellRadiusForFriend(sSpellRadiusStore.LookupEntry(GetSpellProto()->EffectRadiusIndex[effIndex]));

            if (modOwner)
                modOwner->ApplySpellMod(GetId(), SPELLMOD_RADIUS, radius);

            if (!GetUnitOwner()->hasUnitState(UNIT_STAT_ISOLATED))
            {
                switch(GetSpellProto()->Effect[effIndex])
                {
                    case SPELL_EFFECT_APPLY_AREA_AURA_PARTY:
                        targetList.push_back(GetUnitOwner());
                        GetUnitOwner()->GetPartyMemberInDist(targetList, radius);
                        break;
                    case SPELL_EFFECT_APPLY_AREA_AURA_RAID:
                        targetList.push_back(GetUnitOwner());
                        GetUnitOwner()->GetRaidMember(targetList, radius);
                        // Strength of Earth totem
                        if (GetSpellProto()->SpellIconID == 691)
                            GetUnitOwner()->GetRaidMember(targetList, GetSpellRadiusForFriend(sSpellRadiusStore.LookupEntry(GetSpellProto()->EffectRadiusIndex[1])));
                        break;
                    case SPELL_EFFECT_APPLY_AREA_AURA_FRIEND:
                    {
                        targetList.push_back(GetUnitOwner());
                        Trinity::AnyFriendlyUnitInObjectRangeCheck u_check(GetUnitOwner(), GetUnitOwner(), radius);
                        Trinity::UnitListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(GetUnitOwner(), targetList, u_check);
                        GetUnitOwner()->VisitNearbyObject(radius, searcher);
                        break;
                    }
                    case SPELL_EFFECT_APPLY_AREA_AURA_ENEMY:
                    {
                        Trinity::AnyAoETargetUnitInObjectRangeCheck u_check(GetUnitOwner(), GetUnitOwner(), radius); // No GetCharmer in searcher
                        Trinity::UnitListSearcher<Trinity::AnyAoETargetUnitInObjectRangeCheck> searcher(GetUnitOwner(), targetList, u_check);
                        GetUnitOwner()->VisitNearbyObject(radius, searcher);
                        break;
                    }
                    case SPELL_EFFECT_APPLY_AREA_AURA_PET:
                        targetList.push_back(GetUnitOwner());
                    case SPELL_EFFECT_APPLY_AREA_AURA_OWNER:
                    {
                        if (Unit *owner = GetUnitOwner()->GetCharmerOrOwner())
                            if (GetUnitOwner()->IsWithinDistInMap(owner, radius))
                                targetList.push_back(owner);
                        break;
                    }
                }
            }
        }

        for (UnitList::iterator itr = targetList.begin(); itr!= targetList.end();++itr)
        {
            std::map<Unit *, uint8>::iterator existing = targets.find(*itr);
            if (existing != targets.end())
                existing->second |= 1<<effIndex;
            else
                targets[*itr] = 1<<effIndex;
        }
    }
}

DynObjAura::DynObjAura(SpellEntry const* spellproto, uint8 effMask, WorldObject * owner, Unit * caster, int32 *baseAmount, Item * castItem, uint64 casterGUID)
    : Aura(spellproto, effMask, owner, caster, baseAmount, castItem, casterGUID)
{
    LoadScripts();
    ASSERT(GetDynobjOwner());
    ASSERT(GetDynobjOwner()->IsInWorld());
    ASSERT(GetDynobjOwner()->GetMap() == caster->GetMap());
    _InitEffects(effMask, caster, baseAmount);
    GetDynobjOwner()->SetAura(this);
}

void DynObjAura::Remove(AuraRemoveMode removeMode)
{
    if (IsRemoved())
        return;
    _Remove(removeMode);
}

void DynObjAura::FillTargetMap(std::map<Unit *, uint8> & targets, Unit * /*caster*/)
{
    Unit * dynObjOwnerCaster = GetDynobjOwner()->GetCaster();
    float radius = GetDynobjOwner()->GetRadius();

    for (uint8 effIndex = 0; effIndex < MAX_SPELL_EFFECTS; ++effIndex)
    {
        if (!HasEffect(effIndex))
            continue;
        UnitList targetList;
        if (GetSpellProto()->EffectImplicitTargetB[effIndex] == TARGET_DEST_DYNOBJ_ALLY
            || GetSpellProto()->EffectImplicitTargetB[effIndex] == TARGET_UNIT_AREA_ALLY_DST)
        {
            Trinity::AnyFriendlyUnitInObjectRangeCheck u_check(GetDynobjOwner(), dynObjOwnerCaster, radius);
            Trinity::UnitListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(GetDynobjOwner(), targetList, u_check);
            GetDynobjOwner()->VisitNearbyObject(radius, searcher);
        }
        else if (GetSpellProto()->EffectImplicitTargetB[effIndex] == TARGET_DEST_DYNOBJ_ALL_UNITS)
        {
            Trinity::AnyUnitInObjectRangeCheck u_check(GetDynobjOwner(), radius);
            Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(GetDynobjOwner(), targetList, u_check);
            GetDynobjOwner()->VisitNearbyObject(radius, searcher);
        }
        else
        {
            Trinity::AnyAoETargetUnitInObjectRangeCheck u_check(GetDynobjOwner(), dynObjOwnerCaster, radius);
            Trinity::UnitListSearcher<Trinity::AnyAoETargetUnitInObjectRangeCheck> searcher(GetDynobjOwner(), targetList, u_check);
            GetDynobjOwner()->VisitNearbyObject(radius, searcher);
        }

        for (UnitList::iterator itr = targetList.begin(); itr!= targetList.end();++itr)
        {
            std::map<Unit *, uint8>::iterator existing = targets.find(*itr);
            if (existing != targets.end())
                existing->second |= 1<<effIndex;
            else
                targets[*itr] = 1<<effIndex;
        }
    }
}

