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
#include "WorldPacket.h"
#include "WorldSession.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Opcodes.h"
#include "Log.h"
#include "UpdateMask.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "Pet.h"
#include "Unit.h"
#include "Totem.h"
#include "Spell.h"
#include "DynamicObject.h"
#include "Group.h"
#include "Guild.h"
#include "UpdateData.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "CellImpl.h"
#include "SharedDefines.h"
#include "LootMgr.h"
#include "VMapFactory.h"
#include "Battleground.h"
#include "Util.h"
#include "TemporarySummon.h"
#include "Vehicle.h"
#include "SpellAuraEffects.h"
#include "ScriptMgr.h"
#include "ConditionMgr.h"
#include "DisableMgr.h"
#include "SpellScript.h"
#include "DB2Structure.h"
#include "DB2Stores.h"
#include "BattlefieldMgr.h"
#include "InstanceScript.h"

#define SPELL_CHANNEL_UPDATE_INTERVAL (1 * IN_MILLISECONDS)

extern pEffect SpellEffects[TOTAL_SPELL_EFFECTS];

bool IsQuestTameSpell(uint32 spellId)
{
    SpellEntry const *spellproto = sSpellStore.LookupEntry(spellId);
    if (!spellproto) return false;

    return spellproto->Effect[0] == SPELL_EFFECT_THREAT
        && spellproto->Effect[1] == SPELL_EFFECT_APPLY_AURA && spellproto->EffectApplyAuraName[1] == SPELL_AURA_DUMMY;
}

SpellCastTargets::SpellCastTargets() : m_elevation(0), m_speed(0)
{
    m_unitTarget = NULL;
    m_itemTarget = NULL;
    m_GOTarget   = NULL;

    m_unitTargetGUID   = 0;
    m_GOTargetGUID     = 0;
    m_CorpseTargetGUID = 0;
    m_itemTargetGUID   = 0;
    m_itemTargetEntry  = 0;

    m_srcTransGUID = 0;
    m_srcTransOffset.Relocate(0, 0, 0, 0);
    m_srcPos.Relocate(0, 0, 0, 0);
    m_dstTransGUID = 0;
    m_dstTransOffset.Relocate(0, 0, 0, 0);
    m_dstPos.Relocate(0, 0, 0, 0);
    m_strTarget = "";
    m_targetMask = 0;
}

SpellCastTargets::~SpellCastTargets()
{
}

void SpellCastTargets::setUnitTarget(Unit *target)
{
    if (!target)
        return;

    m_unitTarget = target;
    m_unitTargetGUID = target->GetGUID();
    m_targetMask |= TARGET_FLAG_UNIT;
}

void SpellCastTargets::setSrc(float x, float y, float z)
{
    m_srcPos.Relocate(x, y, z);
    m_srcTransGUID = 0;
    m_targetMask |= TARGET_FLAG_SOURCE_LOCATION;
}

void SpellCastTargets::setSrc(Position &pos)
{
    m_srcPos.Relocate(pos);
    m_srcTransGUID = 0;
    m_targetMask |= TARGET_FLAG_SOURCE_LOCATION;
}

void SpellCastTargets::setSrc(WorldObject &wObj)
{
    uint64 guid = wObj.GetTransGUID();
    m_srcTransGUID = guid;
    m_srcTransOffset.Relocate(wObj.GetTransOffsetX(), wObj.GetTransOffsetY(), wObj.GetTransOffsetZ(), wObj.GetTransOffsetO());
    m_srcPos.Relocate(wObj);
    m_targetMask |= TARGET_FLAG_SOURCE_LOCATION;
}

void SpellCastTargets::modSrc(Position &pos)
{
    ASSERT(m_targetMask & TARGET_FLAG_SOURCE_LOCATION);

    if (m_srcTransGUID)
    {
        Position offset;
        m_srcPos.GetPositionOffsetTo(pos, offset);
        m_srcTransOffset.RelocateOffset(offset);
    }
    m_srcPos.Relocate(pos);
}

void SpellCastTargets::setDst(float x, float y, float z, float orientation, uint32 mapId)
{
    m_dstPos.Relocate(x, y, z, orientation);
    m_dstTransGUID = 0;
    m_targetMask |= TARGET_FLAG_DEST_LOCATION;
    if (mapId != MAPID_INVALID)
        m_dstPos.m_mapId = mapId;
}

void SpellCastTargets::setDst(Position &pos)
{
    m_dstPos.Relocate(pos);
    m_dstTransGUID = 0;
    m_targetMask |= TARGET_FLAG_DEST_LOCATION;
}

void SpellCastTargets::setDst(WorldObject &wObj)
{
    uint64 guid = wObj.GetTransGUID();
    m_dstTransGUID = guid;
    m_dstTransOffset.Relocate(wObj.GetTransOffsetX(), wObj.GetTransOffsetY(), wObj.GetTransOffsetZ(), wObj.GetTransOffsetO());
    m_dstPos.Relocate(wObj);
    m_targetMask |= TARGET_FLAG_DEST_LOCATION;
}

void SpellCastTargets::setDst(SpellCastTargets &spellTargets)
{
    m_dstTransGUID = spellTargets.m_dstTransGUID;
    m_dstTransOffset.Relocate(spellTargets.m_dstTransOffset);
    m_dstPos.Relocate(spellTargets.m_dstPos);
    m_targetMask |= TARGET_FLAG_DEST_LOCATION;
}

void SpellCastTargets::modDst(Position &pos)
{
    ASSERT(m_targetMask & TARGET_FLAG_DEST_LOCATION);

    if (m_dstTransGUID)
    {
        Position offset;
        m_dstPos.GetPositionOffsetTo(pos, offset);
        m_dstTransOffset.RelocateOffset(offset);
    }
    m_dstPos.Relocate(pos);
}

void SpellCastTargets::setGOTarget(GameObject *target)
{
    m_GOTarget = target;
    m_GOTargetGUID = target->GetGUID();
    m_targetMask |= TARGET_FLAG_OBJECT;
}

void SpellCastTargets::setItemTarget(Item* item)
{
    if (!item)
        return;

    m_itemTarget = item;
    m_itemTargetGUID = item->GetGUID();
    m_itemTargetEntry = item->GetEntry();
    m_targetMask |= TARGET_FLAG_ITEM;
}

void SpellCastTargets::setTradeItemTarget(Player* caster)
{
    m_itemTargetGUID = uint64(TRADE_SLOT_NONTRADED);
    m_itemTargetEntry = 0;
    m_targetMask |= TARGET_FLAG_TRADE_ITEM;

    Update(caster);
}

void SpellCastTargets::setCorpseTarget(Corpse* corpse)
{
    m_CorpseTargetGUID = corpse->GetGUID();
}

void SpellCastTargets::Update(Unit* caster)
{
    m_GOTarget   = m_GOTargetGUID ? caster->GetMap()->GetGameObject(m_GOTargetGUID) : NULL;
    m_unitTarget = m_unitTargetGUID ?
        (m_unitTargetGUID == caster->GetGUID() ? caster : ObjectAccessor::GetUnit(*caster, m_unitTargetGUID)) :
    NULL;

    m_itemTarget = NULL;
    if (caster->GetTypeId() == TYPEID_PLAYER)
    {
        Player *player = caster->ToPlayer();
        if (m_targetMask & TARGET_FLAG_ITEM)
            m_itemTarget = player->GetItemByGuid(m_itemTargetGUID);
        else if (m_targetMask & TARGET_FLAG_TRADE_ITEM)
            if (m_itemTargetGUID == TRADE_SLOT_NONTRADED) // here it is not guid but slot. Also prevents hacking slots
                if (TradeData* pTrade = player->GetTradeData())
                    m_itemTarget = pTrade->GetTraderData()->GetItem(TRADE_SLOT_NONTRADED);

        if (m_itemTarget)
            m_itemTargetEntry = m_itemTarget->GetEntry();
    }
    // update positions by transport move
    if (HasSrc() && m_srcTransGUID)
    {
        if (WorldObject * transport = ObjectAccessor::GetWorldObject(*caster, m_srcTransGUID))
        {
            m_srcPos.Relocate(transport);
            m_srcPos.RelocateOffset(m_srcTransOffset);
        }
    }
    if (HasDst() && m_dstTransGUID)
    {
        if (WorldObject * transport = ObjectAccessor::GetWorldObject(*caster, m_dstTransGUID))
        {
            m_dstPos.Relocate(transport);
            m_dstPos.RelocateOffset(m_dstTransOffset);
        }
    }
}

void SpellCastTargets::OutDebug()
{
    if (!m_targetMask)
        sLog->outString("TARGET_FLAG_SELF");

    if (m_targetMask & TARGET_FLAG_UNIT)
    {
        sLog->outString("TARGET_FLAG_UNIT: " UI64FMTD, m_unitTargetGUID);
    }
    if (m_targetMask & TARGET_FLAG_UNIT_MINIPET)
    {
        sLog->outString("TARGET_FLAG_UNIT_MINIPET: " UI64FMTD, m_unitTargetGUID);
    }
    if (m_targetMask & TARGET_FLAG_OBJECT)
    {
        sLog->outString("TARGET_FLAG_OBJECT: " UI64FMTD, m_GOTargetGUID);
    }
    if (m_targetMask & TARGET_FLAG_CORPSE)
    {
        sLog->outString("TARGET_FLAG_CORPSE: " UI64FMTD, m_CorpseTargetGUID);
    }
    if (m_targetMask & TARGET_FLAG_PVP_CORPSE)
    {
        sLog->outString("TARGET_FLAG_PVP_CORPSE: " UI64FMTD, m_CorpseTargetGUID);
    }
    if (m_targetMask & TARGET_FLAG_ITEM)
    {
        sLog->outString("TARGET_FLAG_ITEM: " UI64FMTD, m_itemTargetGUID);
    }
    if (m_targetMask & TARGET_FLAG_TRADE_ITEM)
    {
        sLog->outString("TARGET_FLAG_TRADE_ITEM: " UI64FMTD, m_itemTargetGUID);
    }
    if (m_targetMask & TARGET_FLAG_SOURCE_LOCATION)
    {
        sLog->outString("TARGET_FLAG_SOURCE_LOCATION: transport guid:" UI64FMTD " trans offset: %s position: %s", m_srcTransGUID, m_srcTransOffset.ToString().c_str(), m_srcPos.ToString().c_str());
    }
    if (m_targetMask & TARGET_FLAG_DEST_LOCATION)
    {
        sLog->outString("TARGET_FLAG_DEST_LOCATION: transport guid:" UI64FMTD " trans offset: %s position: %s", m_dstTransGUID, m_dstTransOffset.ToString().c_str(), m_dstPos.ToString().c_str());
    }
    if (m_targetMask & TARGET_FLAG_STRING)
    {
        sLog->outString("TARGET_FLAG_STRING: %s", m_strTarget.c_str());
    }
    sLog->outString("speed: %f", m_speed);
    sLog->outString("elevation: %f", m_elevation);
}

void SpellCastTargets::read (ByteBuffer & data, Unit * caster)
{
    data >> m_targetMask;

    if (m_targetMask == TARGET_FLAG_SELF)
        return;

    if (m_targetMask & (TARGET_FLAG_UNIT | TARGET_FLAG_UNIT_MINIPET))
        data.readPackGUID(m_unitTargetGUID);

    if (m_targetMask & (TARGET_FLAG_OBJECT))
        data.readPackGUID(m_GOTargetGUID);

    if(m_targetMask & (TARGET_FLAG_ITEM | TARGET_FLAG_TRADE_ITEM))
        data.readPackGUID(m_itemTargetGUID);

    if(m_targetMask & (TARGET_FLAG_CORPSE | TARGET_FLAG_PVP_CORPSE))
        data.readPackGUID(m_CorpseTargetGUID);

    if (m_targetMask & TARGET_FLAG_SOURCE_LOCATION)
    {
        data.readPackGUID(m_srcTransGUID);
        if (m_srcTransGUID)
            data >> m_srcTransOffset.PositionXYZStream();
        else
            data >> m_srcPos.PositionXYZStream();
    }
    else
    {
        m_srcTransGUID = caster->GetTransGUID();
        if (m_srcTransGUID)
            m_srcTransOffset.Relocate(caster->GetTransOffsetX(), caster->GetTransOffsetY(), caster->GetTransOffsetZ(), caster->GetTransOffsetO());
        else
            m_srcPos.Relocate(caster);
    }

    if (m_targetMask & TARGET_FLAG_DEST_LOCATION)
    {
        data.readPackGUID(m_dstTransGUID);
        if (m_dstTransGUID)
            data >> m_dstTransOffset.PositionXYZStream();
        else
            data >> m_dstPos.PositionXYZStream();
    }
    else
    {
        m_dstTransGUID = caster->GetTransGUID();
        if (m_dstTransGUID)
            m_dstTransOffset.Relocate(caster->GetTransOffsetX(), caster->GetTransOffsetY(), caster->GetTransOffsetZ(), caster->GetTransOffsetO());
        else
            m_dstPos.Relocate(caster);
    }

    if(m_targetMask & TARGET_FLAG_STRING)
        data >> m_strTarget;

    Update(caster);
}

void SpellCastTargets::write (ByteBuffer & data)
{
    data << uint32(m_targetMask);

    if (m_targetMask & (TARGET_FLAG_UNIT | TARGET_FLAG_PVP_CORPSE | TARGET_FLAG_OBJECT | TARGET_FLAG_CORPSE | TARGET_FLAG_UNIT_MINIPET))
    {
        if (m_targetMask & TARGET_FLAG_UNIT)
        {
            if (m_unitTarget)
                data.append(m_unitTarget->GetPackGUID());
            else
                data << uint8(0);
        }
        else if (m_targetMask & TARGET_FLAG_OBJECT)
        {
            if(m_GOTarget)
                data.append(m_GOTarget->GetPackGUID());
            else
                data << uint8(0);
        }
        else if (m_targetMask & ( TARGET_FLAG_CORPSE | TARGET_FLAG_PVP_CORPSE))
            data.appendPackGUID(m_CorpseTargetGUID);
        else
            data << uint8(0);
    }

    if (m_targetMask & ( TARGET_FLAG_ITEM | TARGET_FLAG_TRADE_ITEM))
    {
        if(m_itemTarget)
            data.append(m_itemTarget->GetPackGUID());
        else
            data << uint8(0);
    }

    if (m_targetMask & TARGET_FLAG_SOURCE_LOCATION)
    {
        data.appendPackGUID(m_srcTransGUID); // relative position guid here - transport for example
        if (m_srcTransGUID)
            data << m_srcTransOffset.PositionXYZStream();
        else
            data << m_srcPos.PositionXYZStream();
    }

    if (m_targetMask & TARGET_FLAG_DEST_LOCATION)
    {
        data.appendPackGUID(m_dstTransGUID); // relative position guid here - transport for example
        if (m_dstTransGUID)
            data << m_dstTransOffset.PositionXYZStream();
        else
            data << m_dstPos.PositionXYZStream();
    }

    if (m_targetMask & TARGET_FLAG_STRING)
        data << m_strTarget;
}

Spell::Spell(Unit* Caster, SpellEntry const *info, bool triggered, uint64 originalCasterGUID, bool skipCheck):
m_spellInfo(sSpellMgr->GetSpellForDifficultyFromSpell(info, Caster)),
m_caster(Caster), m_spellValue(new SpellValue(m_spellInfo)), m_preGeneratedPath(PathGenerator(m_caster))
{
    m_customAttr = sSpellMgr->GetSpellCustomAttr(m_spellInfo->Id);
    m_skipCheck = skipCheck;
    m_selfContainer = NULL;
    m_referencedFromCurrentSpell = false;
    m_executedCurrently = false;
    m_needComboPoints = NeedsComboPoints(m_spellInfo);
    m_comboPointGain = 0;
    m_delayStart = 0;
    m_delayAtDamageCount = 0;

    m_applyMultiplierMask = 0;
    m_handledEffectMask = 0;
    m_auraScaleMask = 0;

    // Get data for type of attack
    switch (m_spellInfo->DmgClass)
    {
        case SPELL_DAMAGE_CLASS_MELEE:
            if (m_spellInfo->AttributesEx3 & SPELL_ATTR3_REQ_OFFHAND)
                m_attackType = OFF_ATTACK;
            else
                m_attackType = BASE_ATTACK;
            break;
        case SPELL_DAMAGE_CLASS_RANGED:
            m_attackType = IsRangedWeaponSpell(m_spellInfo) ? RANGED_ATTACK : BASE_ATTACK;
            break;
        default:
                                                            // Wands
            if (m_spellInfo->AttributesEx2 & SPELL_ATTR2_AUTOREPEAT_FLAG)
                m_attackType = RANGED_ATTACK;
            else
                m_attackType = BASE_ATTACK;
            break;
    }

    m_spellSchoolMask = GetSpellSchoolMask(info);           // Can be override for some spell (wand shoot for example)

    if (m_attackType == RANGED_ATTACK)
    {
        // wand case
        if ((m_caster->getClassMask() & CLASSMASK_WAND_USERS) != 0 && m_caster->GetTypeId() == TYPEID_PLAYER)
        {
            if (Item* pItem = m_caster->ToPlayer()->GetWeaponForAttack(RANGED_ATTACK))
                m_spellSchoolMask = SpellSchoolMask(1 << pItem->GetProto()->damageType);
        }
    }

    if (originalCasterGUID)
        m_originalCasterGUID = originalCasterGUID;
    else
        m_originalCasterGUID = m_caster->GetGUID();

    if (m_originalCasterGUID == m_caster->GetGUID())
        m_originalCaster = m_caster;
    else
    {
        m_originalCaster = ObjectAccessor::GetUnit(*m_caster, m_originalCasterGUID);
        if (m_originalCaster && !m_originalCaster->IsInWorld()) m_originalCaster = NULL;
    }

    m_spellState = SPELL_STATE_NULL;

    m_IsTriggeredSpell = bool(triggered || (info->AttributesEx4 & SPELL_ATTR4_TRIGGERED));
    m_CastItem = NULL;

    unitTarget = NULL;
    itemTarget = NULL;
    gameObjTarget = NULL;
    focusObject = NULL;
    m_cast_count = 0;
    m_glyphIndex = 0;
    m_keyStonesCount = 0;
    m_preCastSpell = 0;
    m_triggeredByAuraSpell  = NULL;
    m_spellAura = NULL;
    m_magnetingAura = NULL;

    //Auto Shot & Shoot (wand)
    m_autoRepeat = IsAutoRepeatRangedSpell(m_spellInfo);

    m_runesState = 0;
    m_powerCost = CalculatePowerCost(m_spellInfo, m_caster, m_spellSchoolMask); // this will be nullified in Spell::prepare if cast item will be set
    m_casttime = 0;                                         // setup to correct value in Spell::prepare, don't must be used before.
    m_timer = 0;                                            // will set to castime in prepare

    m_channelTargetEffectMask = 0;

    // determine reflection
    m_canReflect = m_spellInfo->DmgClass == SPELL_DAMAGE_CLASS_MAGIC && !(m_spellInfo->Attributes & SPELL_ATTR0_ABILITY)
        && !(m_spellInfo->AttributesEx & SPELL_ATTR1_CANT_BE_REFLECTED) && !(m_spellInfo->Attributes & SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY)
        && !IsPassiveSpell(m_spellInfo) && !IsPositiveSpell(m_spellInfo->Id);

    CleanupTargetList();
    CleanupEffectExecuteData();
}

Spell::~Spell()
{
    // unload scripts
    while(!m_loadedScripts.empty())
    {
        std::list<SpellScript *>::iterator itr = m_loadedScripts.begin();
        (*itr)->_Unload();
        delete (*itr);
        m_loadedScripts.erase(itr);
    }

    if (m_referencedFromCurrentSpell && m_selfContainer && *m_selfContainer == this)
    {
        // Clean the reference to avoid later crash.
        // If this error is repeating, we may have to add an ASSERT to better track down how we get into this case.
        sLog->outError("SPELL: deleting spell for spell ID %u. However, spell still referenced.", m_spellInfo->Id);
        *m_selfContainer = NULL;
    }

    if (m_caster && m_caster->GetTypeId() == TYPEID_PLAYER)
        ASSERT(m_caster->ToPlayer()->m_spellModTakingSpell != this);

    delete m_spellValue;

    CheckEffectExecuteData();
}

template<typename T>
WorldObject* Spell::FindCorpseUsing()
{
    // non-standard target selection
    float max_range = GetSpellMaxRange(m_spellInfo, false);

    CellPair p(Trinity::ComputeCellPair(m_caster->GetPositionX(), m_caster->GetPositionY()));
    Cell cell(p);
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();

    WorldObject* result = NULL;

    T u_check(m_caster, max_range);
    Trinity::WorldObjectSearcher<T> searcher(m_caster, result, u_check);

    TypeContainerVisitor<Trinity::WorldObjectSearcher<T>, GridTypeMapContainer > grid_searcher(searcher);
    cell.Visit(p, grid_searcher, *m_caster->GetMap(), *m_caster, max_range);

    if (!result)
    {
        TypeContainerVisitor<Trinity::WorldObjectSearcher<T>, WorldTypeMapContainer > world_searcher(searcher);
        cell.Visit(p, world_searcher, *m_caster->GetMap(), *m_caster, max_range);
    }

    return result;
}

void Spell::SelectSpellTargets()
{
    for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        // not call for empty effect.
        // Also some spells use not used effect targets for store targets for dummy effect in triggered spells
        if (!m_spellInfo->Effect[i])
            continue;

        // Check if effect is prevented by some custom reasons
        if (!ApplyEffectCondition(SpellEffIndex(i)))
            continue;

        uint32 effectTargetType = EffectTargetType[m_spellInfo->Effect[i]];

        // is it possible that areaaura is not applied to caster?
        if (effectTargetType == SPELL_REQUIRE_NONE)
            continue;

        uint32 targetA = m_spellInfo->EffectImplicitTargetA[i];
        uint32 targetB = m_spellInfo->EffectImplicitTargetB[i];

        if (targetA)
            SelectEffectTargets(i, targetA);
        if (targetB) // In very rare case !A && B
            SelectEffectTargets(i, targetB);

        if (effectTargetType != SPELL_REQUIRE_UNIT)
        {
            if (effectTargetType == SPELL_REQUIRE_CASTER)
                AddUnitTarget(m_caster, i);
            else if (effectTargetType == SPELL_REQUIRE_ITEM)
            {
                if (m_targets.getItemTarget())
                    AddItemTarget(m_targets.getItemTarget(), i);
            }
            continue;
        }

        if (!targetA && !targetB)
        {
            if (!GetSpellMaxRangeForFriend(sSpellRangeStore.LookupEntry(m_spellInfo->rangeIndex)))
            {
                AddUnitTarget(m_caster, i);
                continue;
            }

            // add here custom effects that need default target.
            // FOR EVERY TARGET TYPE THERE IS A DIFFERENT FILL!!
            switch(m_spellInfo->Effect[i])
            {
                case SPELL_EFFECT_DUMMY:
                {
                    switch(m_spellInfo->Id)
                    {
                        case 20577:                         // Cannibalize
                        case 54044:                         // Carrion Feeder
                        {
                            WorldObject* result = NULL;
                            if (m_spellInfo->Id == 20577)
                                result = FindCorpseUsing<Trinity::CannibalizeObjectCheck>();
                            else
                                result = FindCorpseUsing<Trinity::CarrionFeederObjectCheck>();

                            if (result)
                            {
                                switch(result->GetTypeId())
                                {
                                    case TYPEID_UNIT:
                                    case TYPEID_PLAYER:
                                        AddUnitTarget((Unit*)result, i);
                                        break;
                                    case TYPEID_CORPSE:
                                        m_targets.setCorpseTarget((Corpse*)result);
                                        if (Player* owner = ObjectAccessor::FindPlayer(((Corpse*)result)->GetOwnerGUID()))
                                            AddUnitTarget(owner, i);
                                        break;
                                    default:
                                        break;
                                }
                            }
                            else
                            {
                                // clear cooldown at fail
                                if (m_caster->GetTypeId() == TYPEID_PLAYER)
                                    m_caster->ToPlayer()->RemoveSpellCooldown(m_spellInfo->Id, true);
                                SendCastResult(SPELL_FAILED_NO_EDIBLE_CORPSES);
                                finish(false);
                            }
                            break;
                        }
                        default:
                            if (m_targets.getUnitTarget())
                                AddUnitTarget(m_targets.getUnitTarget(), i);
                            else
                                AddUnitTarget(m_caster, i);
                            break;
                    }
                    break;
                }
                case SPELL_EFFECT_BIND:
                case SPELL_EFFECT_CREATE_ITEM:
                case SPELL_EFFECT_TRIGGER_SPELL:
                case SPELL_EFFECT_SKILL_STEP:
                case SPELL_EFFECT_PROFICIENCY:
                case SPELL_EFFECT_SUMMON_OBJECT_WILD:
                case SPELL_EFFECT_SELF_RESURRECT:
                case SPELL_EFFECT_REPUTATION:
                case SPELL_EFFECT_LEARN_SPELL:
                case SPELL_EFFECT_SEND_TAXI:
                    if (m_targets.getUnitTarget())
                        AddUnitTarget(m_targets.getUnitTarget(), i);
                    // Triggered spells have additional spell targets - cast them even if no explicit unit target is given (required for spell 50516 for example)
                    else if (m_spellInfo->Effect[i] == SPELL_EFFECT_TRIGGER_SPELL)
                        AddUnitTarget(m_caster, i);
                    break;
                case SPELL_EFFECT_SUMMON_PLAYER:
                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    {
                        // Have Group, Will Travel - target all party/raid members
                        if (m_spellInfo->Id == 85592)
                        {
                            Group *pGroup = m_caster->ToPlayer()->GetGroup();
                            if (pGroup)
                            {
                                for (GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
                                    if (itr->getSource() != m_caster)
                                        AddUnitTarget(itr->getSource(), i);
                            }
                        }
                        else if (m_caster->ToPlayer()->GetSelection())
                        {
                            Player* target = sObjectMgr->GetPlayer(m_caster->ToPlayer()->GetSelection());
                            if (target)
                                AddUnitTarget(target, i);
                        }
                    }
                    break;
                case SPELL_EFFECT_RESURRECT:
                case SPELL_EFFECT_RESURRECT_NEW:
                    if (m_targets.getUnitTarget())
                        AddUnitTarget(m_targets.getUnitTarget(), i);
                    if (m_targets.getCorpseTargetGUID())
                    {
                        Corpse *corpse = ObjectAccessor::GetCorpse(*m_caster, m_targets.getCorpseTargetGUID());
                        if (corpse)
                        {
                            Player* owner = ObjectAccessor::FindPlayer(corpse->GetOwnerGUID());
                            if (owner)
                                AddUnitTarget(owner, i);
                        }
                    }
                    break;
                case SPELL_EFFECT_SUMMON_CHANGE_ITEM:
                case SPELL_EFFECT_ADD_FARSIGHT:
                case SPELL_EFFECT_APPLY_GLYPH:
                case SPELL_EFFECT_STUCK:
                case SPELL_EFFECT_FEED_PET:
                case SPELL_EFFECT_DESTROY_ALL_TOTEMS:
                case SPELL_EFFECT_KILL_CREDIT2: // only one spell: 42793
                    AddUnitTarget(m_caster, i);
                    break;
                case SPELL_EFFECT_LEARN_PET_SPELL:
                    if (Guardian* pet = m_caster->GetGuardianPet())
                        AddUnitTarget(pet, i);
                    break;
                /*case SPELL_EFFECT_ENCHANT_ITEM:
                case SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY:
                case SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC:
                case SPELL_EFFECT_DISENCHANT:
                case SPELL_EFFECT_PROSPECTING:
                case SPELL_EFFECT_MILLING:
                    if (m_targets.getItemTarget())
                        AddItemTarget(m_targets.getItemTarget(), i);
                    break;*/
                case SPELL_EFFECT_APPLY_AURA:
                    switch(m_spellInfo->EffectApplyAuraName[i])
                    {
                        case SPELL_AURA_ADD_FLAT_MODIFIER:  // some spell mods auras have 0 target modes instead expected TARGET_UNIT_CASTER(1) (and present for other ranks for same spell for example)
                        case SPELL_AURA_ADD_PCT_MODIFIER:
                            AddUnitTarget(m_caster, i);
                            break;
                        default:                            // apply to target in other case
                            break;
                    }
                    break;
                case SPELL_EFFECT_APPLY_AREA_AURA_PARTY:
                                                            // AreaAura
                    if (m_spellInfo->Attributes & (SPELL_ATTR0_CASTABLE_WHILE_SITTING | SPELL_ATTR0_CASTABLE_WHILE_MOUNTED | SPELL_ATTR0_DONT_AFFECT_SHEATH_STATE | SPELL_ATTR0_NOT_SHAPESHIFT) || m_spellInfo->Attributes == SPELL_ATTR0_NOT_SHAPESHIFT)
                        SelectEffectTargets(i, TARGET_UNIT_PARTY_TARGET);
                    break;
                case SPELL_EFFECT_SKIN_PLAYER_CORPSE:
                    if (m_targets.getUnitTarget())
                    {
                        AddUnitTarget(m_targets.getUnitTarget(), i);
                    }
                    else if (m_targets.getCorpseTargetGUID())
                    {
                        Corpse *corpse = ObjectAccessor::GetCorpse(*m_caster,m_targets.getCorpseTargetGUID());
                        if (corpse)
                        {
                            Player* owner = ObjectAccessor::FindPlayer(corpse->GetOwnerGUID());
                            if (owner)
                                AddUnitTarget(owner, i);
                        }
                    }
                    break;
                default:
                    AddUnitTarget(m_caster, i);
                    break;
            }
        }
        if (IsChanneledSpell(m_spellInfo))
        {
            uint8 mask = (1<<i);
            for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
            {
                if (ihit->effectMask & mask)
                {
                    m_channelTargetEffectMask |= mask;
                    break;
                }
            }
        }
        else if (m_auraScaleMask)
        {
            bool checkLvl = !m_UniqueTargetInfo.empty();
            for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end();)
            {
                // remove targets which did not pass min level check
                if (m_auraScaleMask && ihit->effectMask == m_auraScaleMask)
                {
                    // Do not check for selfcast
                    if (!ihit->scaleAura && ihit->targetGUID != m_caster->GetGUID())
                    {
                         m_UniqueTargetInfo.erase(ihit++);
                         continue;
                    }
                }
                ++ihit;
            }
            if (checkLvl && m_UniqueTargetInfo.empty())
            {
                SendCastResult(SPELL_FAILED_LOWLEVEL);
                finish(false);
            }
        }
    }

    /* explicit target conditions here
     * typically spells with "when more that X targets are hit, do XYZ" */

    // Divine Storm
    if (m_spellInfo->Id == 53385)
    {
        uint32 targetCount = 0;
        for (std::list<TargetInfo>::iterator itr = m_UniqueTargetInfo.begin(); itr != m_UniqueTargetInfo.end(); ++itr)
            if ((*itr).effectMask & (1 << EFFECT_2))
                targetCount++;

        if (targetCount > 0)
        {
            // Divine Purpose talent 1/2
            if ((m_caster->HasAura(85117) && roll_chance_i(7)) ||
                (m_caster->HasAura(86172) && roll_chance_i(14)))
                m_caster->CastSpell(m_caster, 90174, true); // Divine Purpose
        }

        // if didn't hit more than 4 targets, do not energize
        if (targetCount < 4)
        {
            for (std::list<TargetInfo>::iterator itr = m_UniqueTargetInfo.begin(); itr != m_UniqueTargetInfo.end(); ++itr)
                if ((*itr).effectMask & (1 << EFFECT_0))
                    (*itr).effectMask &= ~(1 << EFFECT_0);
        }
    }
    // Whirlwind
    else if (m_spellInfo->Id == 1680)
    {
        if (m_UniqueTargetInfo.size() >= 4)
        {
            int32 bp = 1680;
            m_caster->CastCustomSpell(m_caster, 77691, &bp, NULL, NULL, true); // dummy cooldown modifier
        }
    }

    if (m_targets.HasDst())
    {
        if (m_targets.HasTraj())
        {
            float speed = m_targets.GetSpeedXY();
            if (speed > 0.0f)
                m_delayMoment = (uint64)floor(m_targets.GetDist2d() / speed * 1000.0f);
        }
        else if (m_spellInfo->speed > 0.0f)
        {
            if (!(m_spellInfo->AttributesEx9 & SPELL_ATTR9_SPECIAL_DELAY_CALCULATION))
            {
                float dist = m_caster->GetDistance(m_targets.m_dstPos);
                m_delayMoment = (uint64)floor(dist / m_spellInfo->speed * 1000.0f);
            }
            else
            {
                m_delayMoment = (uint64)(m_spellInfo->speed * 1000.0f);
            }
        }
    }
}

void Spell::prepareDataForTriggerSystem(AuraEffect const * /*triggeredByAura*/)
{
    //==========================================================================================
    // Now fill data for trigger system, need know:
    // can spell trigger another or not (m_canTrigger)
    // Create base triggers flags for Attacker and Victim (m_procAttacker, m_procVictim and m_procEx)
    //==========================================================================================

    m_procVictim = m_procAttacker = 0;
    // Get data for type of attack and fill base info for trigger
    switch (m_spellInfo->DmgClass)
    {
        case SPELL_DAMAGE_CLASS_MELEE:
            m_procAttacker = PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS;
            if (m_attackType == OFF_ATTACK)
                m_procAttacker |= PROC_FLAG_DONE_OFFHAND_ATTACK;
            else
                m_procAttacker |= PROC_FLAG_DONE_MAINHAND_ATTACK;
            m_procVictim   = PROC_FLAG_TAKEN_MELEE_AUTO_ATTACK;
            break;
        case SPELL_DAMAGE_CLASS_RANGED:
            // Auto attack
            if (m_spellInfo->AttributesEx2 & SPELL_ATTR2_AUTOREPEAT_FLAG)
            {
                m_procAttacker = PROC_FLAG_DONE_RANGED_AUTO_ATTACK;
                m_procVictim   = PROC_FLAG_TAKEN_RANGED_AUTO_ATTACK;
            }
            else // Ranged spell attack
            {
                m_procAttacker = PROC_FLAG_DONE_SPELL_RANGED_DMG_CLASS;
                m_procVictim   = PROC_FLAG_TAKEN_SPELL_RANGED_DMG_CLASS;
            }
            break;
        default:
            if (m_spellInfo->EquippedItemClass == ITEM_CLASS_WEAPON &&
                m_spellInfo->EquippedItemSubClassMask & (1<<ITEM_SUBCLASS_WEAPON_WAND)
                && m_spellInfo->AttributesEx2 & SPELL_ATTR2_AUTOREPEAT_FLAG) // Wands auto attack
            {
                m_procAttacker = PROC_FLAG_DONE_RANGED_AUTO_ATTACK;
                m_procVictim   = PROC_FLAG_TAKEN_RANGED_AUTO_ATTACK;
            }
            // For other spells trigger procflags are set in Spell::DoAllEffectOnTarget
            // Because spell positivity is dependant on target
    }
    m_procEx= PROC_EX_NONE;

    // Hunter trap spells - activation proc for Lock and Load, Entrapment and Misdirection
    if (IsTrapActivationSpell(m_spellInfo))
    {
        if (m_originalCaster)
        {
            // Lock and Load hack fix !!! -> very ugly solution
            if (AuraEffect * aurEff = m_originalCaster->GetAuraEffect(SPELL_AURA_PROC_TRIGGER_SPELL, SPELLFAMILY_HUNTER, 3579, EFFECT_0))
            if (roll_chance_i(aurEff->GetAmount()) && (m_spellInfo->SchoolMask & SPELL_SCHOOL_MASK_FROST))
                if (!m_originalCaster->HasAura(67544))
                {
                    m_originalCaster->CastSpell(m_originalCaster, 56453 , true); // buff
                    m_originalCaster->CastSpell(m_originalCaster, 67544 , true); // iCD
                }
        }
        m_procAttacker |= PROC_FLAG_DONE_TRAP_ACTIVATION;
    }

    /*
        Effects which are result of aura proc from triggered spell cannot proc
        to prevent chain proc of these spells
    */

    // Hellfire Effect - trigger as DOT
    if (m_spellInfo->SpellFamilyName == SPELLFAMILY_WARLOCK && m_spellInfo->SpellFamilyFlags[0] & 0x00000040)
    {
        m_procAttacker = PROC_FLAG_DONE_PERIODIC;
        m_procVictim   = PROC_FLAG_TAKEN_PERIODIC;
    }

    // Ranged autorepeat attack is set as triggered spell - ignore it
    if (!(m_procAttacker & PROC_FLAG_DONE_RANGED_AUTO_ATTACK))
    {
        if (m_IsTriggeredSpell &&
            (m_spellInfo->AttributesEx2 & SPELL_ATTR2_TRIGGERED_CAN_TRIGGER ||
            m_spellInfo->AttributesEx3 & SPELL_ATTR3_TRIGGERED_CAN_TRIGGER_2))
            m_procEx |= PROC_EX_INTERNAL_CANT_PROC;
        else if (m_IsTriggeredSpell)
            m_procEx |= PROC_EX_INTERNAL_TRIGGERED;
    }
    // Totem casts require spellfamilymask defined in spell_proc_event to proc
    if (m_originalCaster && m_caster != m_originalCaster && m_caster->GetTypeId() == TYPEID_UNIT && m_caster->ToCreature()->IsTotem() && m_caster->IsControlledByPlayer())
    {
        m_procEx |= PROC_EX_INTERNAL_REQ_FAMILY;
    }
}

void Spell::CleanupTargetList()
{
    m_UniqueTargetInfo.clear();
    m_UniqueGOTargetInfo.clear();
    m_UniqueItemInfo.clear();
    m_delayMoment = 0;
}

void Spell::AddUnitTarget(Unit* pVictim, uint32 effIndex)
{
    if (m_spellInfo->Effect[effIndex] == 0)
        return;

    if (!CheckTarget(pVictim, effIndex))
        return;

    // Check for effect immune skip if immuned
    bool immuned = pVictim->IsImmunedToSpellEffect(m_spellInfo, effIndex, false);

    uint64 targetGUID = pVictim->GetGUID();

    // Lookup target in already in list
    for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
    {
        if (targetGUID == ihit->targetGUID)                 // Found in list
        {
            if (!immuned)
                ihit->effectMask |= 1 << effIndex;          // Add only effect mask if not immuned
            ihit->scaleAura = false;
            if (m_auraScaleMask && ihit->effectMask == m_auraScaleMask && m_caster != pVictim)
            {
                SpellEntry const * auraSpell = sSpellStore.LookupEntry(sSpellMgr->GetFirstSpellInChain(m_spellInfo->Id));
                if (uint32(pVictim->getLevel() + 10) >= auraSpell->spellLevel)
                    ihit->scaleAura = true;
            }
            return;
        }
    }

    // This is new target calculate data for him

    // Get spell hit result on target
    TargetInfo target;
    target.targetGUID = targetGUID;                         // Store target GUID
    target.effectMask = immuned ? 0 : 1 << effIndex;        // Store index of effect if not immuned
    target.processed  = false;                              // Effects not apply on target
    target.alive      = pVictim->IsAlive();
    target.damage     = 0;
    target.crit       = false;
    target.scaleAura  = false;
    if (m_auraScaleMask && target.effectMask == m_auraScaleMask && m_caster != pVictim)
    {
        SpellEntry const * auraSpell = sSpellStore.LookupEntry(sSpellMgr->GetFirstSpellInChain(m_spellInfo->Id));
        if (uint32(pVictim->getLevel() + 10) >= auraSpell->spellLevel)
            target.scaleAura = true;
    }

    // Calculate hit result
    if (m_originalCaster)
    {
        target.missCondition = m_originalCaster->SpellHitResult(pVictim, m_spellInfo, m_canReflect, target.effectMask);
        if (m_skipCheck && target.missCondition != SPELL_MISS_IMMUNE)
            target.missCondition = SPELL_MISS_NONE;
    }
    else
        target.missCondition = SPELL_MISS_EVADE; //SPELL_MISS_NONE;

    // Spell have speed - need calculate incoming time
    // Incoming time is zero for self casts. At least I think so.
    if (m_spellInfo->speed > 0.0f && m_caster != pVictim)
    {
        // calculate spell incoming interval
        // TODO: this is a hack
        float dist = m_caster->GetDistance(pVictim->GetPositionX(), pVictim->GetPositionY(), pVictim->GetPositionZ());

        if (dist < 5.0f)
            dist = 5.0f;

        if (!(m_spellInfo->AttributesEx9 & SPELL_ATTR9_SPECIAL_DELAY_CALCULATION))
            target.timeDelay = uint64(floor(dist / m_spellInfo->speed * 1000.0f));
        else
            target.timeDelay = uint64(m_spellInfo->speed * 1000.0f);

        // Calculate minimum incoming time
        if (m_delayMoment == 0 || m_delayMoment>target.timeDelay)
            m_delayMoment = target.timeDelay;

        m_delay = true;
    }
    else
    {
        // PvP CC Delay
        bool isCasterPlayer = m_caster->GetTypeId() == TYPEID_PLAYER || m_caster->ToPet();
        bool isTargetPlayer = pVictim->GetTypeId() == TYPEID_PLAYER || pVictim->ToPet();
        if(isCasterPlayer && isTargetPlayer)
        {
            target.timeDelay = GetCCDelay(m_spellInfo);
            if (target.timeDelay > 0)
            {
                if (m_delayMoment == 0 || m_delayMoment > target.timeDelay)
                    m_delayMoment = target.timeDelay;

                m_delay = true;
            }
        }
        else
        {
            target.timeDelay = 0LL;
        }
    }

    // If target reflect spell back to caster
    if (target.missCondition == SPELL_MISS_REFLECT)
    {
        // Calculate reflected spell result on caster
        target.reflectResult =  m_caster->SpellHitResult(m_caster, m_spellInfo, m_canReflect);

        if (target.reflectResult == SPELL_MISS_REFLECT)     // Impossible reflect again, so simply deflect spell
            target.reflectResult = SPELL_MISS_PARRY;

        // Increase time interval for reflected spells by 1.5
        target.timeDelay += target.timeDelay >> 1;
    }
    else
        target.reflectResult = SPELL_MISS_NONE;

    // Add target to list
    m_UniqueTargetInfo.push_back(target);
}

void Spell::AddUnitTarget(uint64 unitGUID, uint32 effIndex)
{
    if (Unit* unit = m_caster->GetGUID() == unitGUID ? m_caster : ObjectAccessor::GetUnit(*m_caster, unitGUID))
        AddUnitTarget(unit, effIndex);
}

void Spell::AddGOTarget(GameObject* pVictim, uint32 effIndex)
{
    if (m_spellInfo->Effect[effIndex] == 0)
        return;

    switch (m_spellInfo->Effect[effIndex])
    {
        case SPELL_EFFECT_WMO_DAMAGE:
        case SPELL_EFFECT_WMO_REPAIR:
        case SPELL_EFFECT_WMO_CHANGE:
            if (pVictim->GetGoType() != GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING)
                return;
            break;
        default:
            break;
    }

    uint64 targetGUID = pVictim->GetGUID();

    // Lookup target in already in list
    for (std::list<GOTargetInfo>::iterator ihit = m_UniqueGOTargetInfo.begin(); ihit != m_UniqueGOTargetInfo.end(); ++ihit)
    {
        if (targetGUID == ihit->targetGUID)                 // Found in list
        {
            ihit->effectMask |= 1 << effIndex;              // Add only effect mask
            return;
        }
    }

    // This is new target calculate data for him

    GOTargetInfo target;
    target.targetGUID = targetGUID;
    target.effectMask = 1 << effIndex;
    target.processed  = false;                              // Effects not apply on target

    // Spell have speed - need calculate incoming time
    if (m_spellInfo->speed > 0.0f)
    {
        // calculate spell incoming interval
        float dist = m_caster->GetDistance(pVictim->GetPositionX(), pVictim->GetPositionY(), pVictim->GetPositionZ());
        if (dist < 5.0f)
            dist = 5.0f;

        if (!(m_spellInfo->AttributesEx9 & SPELL_ATTR9_SPECIAL_DELAY_CALCULATION))
            target.timeDelay = uint64(floor(dist / m_spellInfo->speed * 1000.0f));
        else
            target.timeDelay = uint64(m_spellInfo->speed * 1000.0f);

        if (m_delayMoment == 0 || m_delayMoment>target.timeDelay)
            m_delayMoment = target.timeDelay;
    }
    else
        target.timeDelay = 0LL;

    // Add target to list
    m_UniqueGOTargetInfo.push_back(target);
}

void Spell::AddGOTarget(uint64 goGUID, uint32 effIndex)
{
    GameObject* go = m_caster->GetMap()->GetGameObject(goGUID);
    if (go)
        AddGOTarget(go, effIndex);
}

void Spell::AddItemTarget(Item* pitem, uint32 effIndex)
{
    if (m_spellInfo->Effect[effIndex] == 0)
        return;

    // Lookup target in already in list
    for (std::list<ItemTargetInfo>::iterator ihit = m_UniqueItemInfo.begin(); ihit != m_UniqueItemInfo.end(); ++ihit)
    {
        if (pitem == ihit->item)                            // Found in list
        {
            ihit->effectMask |= 1<<effIndex;                // Add only effect mask
            return;
        }
    }

    // This is new target add data

    ItemTargetInfo target;
    target.item       = pitem;
    target.effectMask = 1 << effIndex;
    m_UniqueItemInfo.push_back(target);
}

void Spell::DoAllEffectOnTarget(TargetInfo *target)
{
    if (!target || target->processed)
        return;

    target->processed = true;                               // Target checked in apply effects procedure

    // Get mask of effects for target
    uint8 mask = target->effectMask;

    Unit* unit = m_caster->GetGUID() == target->targetGUID ? m_caster : ObjectAccessor::GetUnit(*m_caster,target->targetGUID);
    if (!unit)
    {
        uint8 farMask = 0;
        // create far target mask
        for(uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (IsFarUnitTargetEffect(m_spellInfo->Effect[i]))
                if ((1<<i) & mask)
                    farMask |= (1<<i);
        }
        if (!farMask)
            return;
        // find unit in world
        unit = ObjectAccessor::FindUnit(target->targetGUID);
        if (!unit)
            return;
        // do far effects on the unit
        // can't use default call because of threading, do stuff as fast as possible
        for(uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (farMask & (1<<i))
                HandleEffects(unit,NULL,NULL,i);
        }
        return;
    }

    if (unit->IsAlive() != target->alive)
        return;

    // Get original caster (if exist) and calculate damage/healing from him data
    Unit *caster = m_originalCaster ? m_originalCaster : m_caster;

    // Skip if m_originalCaster not avaiable
    if (!caster)
        return;

    SpellMissInfo missInfo = target->missCondition;

    // Need init unitTarget by default unit (can changed in code on reflect)
    // Or on missInfo != SPELL_MISS_NONE unitTarget undefined (but need in trigger subsystem)
    unitTarget = unit;

    // Reset damage/healing counter
    m_damage = target->damage;
    m_healing = -target->damage;

    // Fill base trigger info
    uint32 procAttacker = m_procAttacker;
    uint32 procVictim   = m_procVictim;
    uint32 procEx = m_procEx;

    m_spellAura = NULL; // Set aura to null for every target-make sure that pointer is not used for unit without aura applied

                            //Spells with this flag cannot trigger if effect is casted on self
                            // Slice and Dice, relentless strikes, eviscerate
    bool canEffectTrigger = unitTarget->CanProc() && (m_spellInfo->AttributesEx4 & (SPELL_ATTR4_CANT_PROC_FROM_SELFCAST) ? m_caster != unitTarget : true);
    Unit * spellHitTarget = NULL;

    if (missInfo == SPELL_MISS_NONE)                          // In case spell hit target, do all effect on that target
        spellHitTarget = unit;
    else if (missInfo == SPELL_MISS_REFLECT)                // In case spell reflect from target, do all effect on caster (if hit)
    {
        if (target->reflectResult == SPELL_MISS_NONE)       // If reflected spell hit caster -> do all effect on him
        {
            spellHitTarget = m_caster;
            if (m_caster->GetTypeId() == TYPEID_UNIT)
                m_caster->ToCreature()->LowerPlayerDamageReq(target->damage);
        }
    }

    if (spellHitTarget)
    {
        SpellMissInfo missInfo = DoSpellHitOnUnit(spellHitTarget, mask, target->scaleAura);
        if (missInfo != SPELL_MISS_NONE)
        {
            if (missInfo != SPELL_MISS_MISS)
                m_caster->SendSpellMiss(unit, m_spellInfo->Id, missInfo);
            m_damage = 0;
            spellHitTarget = NULL;
        }
    }

    // Do not take combo points on dodge and miss and some special cases
    if (m_needComboPoints && m_targets.getUnitTargetGUID() == target->targetGUID)
    {
        // When miss or dodge
        if (missInfo != SPELL_MISS_NONE)
        {
            m_needComboPoints = false;
            // Restore spell mods for a miss/dodge/parry Cold Blood
            // TODO: check how broad this rule should be
            if (m_caster->GetTypeId() == TYPEID_PLAYER)
                if ((missInfo == SPELL_MISS_MISS) ||
                    (missInfo == SPELL_MISS_DODGE) ||
                    (missInfo == SPELL_MISS_PARRY))
                    m_caster->ToPlayer()->RestoreSpellMods(this, 14177);
        }

        // Special cases
        // Improved Expose Armor
        if (m_caster->GetTypeId() == TYPEID_PLAYER && m_spellInfo->Id == 8647 &&
            ((m_caster->HasAura(14168) && roll_chance_i(50)) || m_caster->HasAura(14169)))
            m_needComboPoints = false;
    }

    // Trigger info was not filled in spell::preparedatafortriggersystem - we do it now
    if (canEffectTrigger && !procAttacker && !procVictim)
    {
        bool positive = true;
        if (m_damage > 0)
            positive = false;
        else if (!m_healing)
        {
            for (uint8 i = 0; i< MAX_SPELL_EFFECTS; ++i)
                // If at least one effect negative spell is negative hit
                if (mask & (1<<i) && !IsPositiveEffect(m_spellInfo->Id, i))
                {
                    positive = false;
                    break;
                }
        }
        switch(m_spellInfo->DmgClass)
        {
            case SPELL_DAMAGE_CLASS_MAGIC:
                if (positive)
                {
                    procAttacker |= PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS;
                    procVictim   |= PROC_FLAG_TAKEN_SPELL_MAGIC_DMG_CLASS_POS;
                }
                else
                {
                    procAttacker |= PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG;
                    procVictim   |= PROC_FLAG_TAKEN_SPELL_MAGIC_DMG_CLASS_NEG;
                }
            break;
            case SPELL_DAMAGE_CLASS_NONE:
                if (positive)
                {
                    procAttacker |= PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_POS;
                    procVictim   |= PROC_FLAG_TAKEN_SPELL_NONE_DMG_CLASS_POS;
                }
                else
                {
                    procAttacker |= PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG;
                    procVictim   |= PROC_FLAG_TAKEN_SPELL_NONE_DMG_CLASS_NEG;
                }
            break;
        }
    }
    CallScriptOnHitHandlers();

    // Cleanse Spirit - heal after dispel with Cleansing Waters talent
    if (m_spellInfo->Id == 51886 && m_healing)
    {
        if (m_caster && m_caster->GetTypeId() == TYPEID_PLAYER && !m_caster->ToPlayer()->HasSpellCooldown(m_healing))
        {
            m_caster->CastSpell(unitTarget, m_healing, true);
            m_caster->ToPlayer()->AddSpellCooldown(m_healing,0,6000);
        }

        m_healing = 0;
    }

    // All calculated do it!
    // Do healing and triggers
    if (m_healing > 0)
    {
        bool crit = caster->IsSpellCrit(unitTarget, m_spellInfo, m_spellSchoolMask);
        uint32 addhealth = m_healing;
        if (crit)
        {
            procEx |= PROC_EX_CRITICAL_HIT;
            addhealth = caster->SpellCriticalHealingBonus(m_spellInfo, addhealth, NULL);
        }
        else
            procEx |= PROC_EX_NORMAL_HIT;

        addhealth = caster->AfterAllSpellHealingCalculation(m_spellInfo,addhealth,unitTarget);

        // Do triggers for unit (reflect triggers passed on hit phase for correct drop charge)
        if (canEffectTrigger && missInfo != SPELL_MISS_REFLECT)
            caster->ProcDamageAndSpell(unitTarget, procAttacker, procVictim, procEx, addhealth, m_attackType, m_spellInfo, m_triggeredByAuraSpell);

        int32 gain = caster->HealBySpell(unitTarget, m_spellInfo, addhealth, crit);
        unitTarget->getHostileRefManager().threatAssist(caster, float(gain) * 0.5f, m_spellInfo);
    }
    // Do damage and triggers
    else if (m_damage > 0)
    {
        // Fill base damage struct (unitTarget - is real spell target)
        SpellNonMeleeDamage damageInfo(caster, unitTarget, m_spellInfo->Id, m_spellSchoolMask);

        // Add bonuses and fill damageInfo struct
        caster->CalculateSpellDamageTaken(&damageInfo, m_damage, m_spellInfo, m_attackType,  target->crit);
        caster->DealDamageMods(damageInfo.target,damageInfo.damage,&damageInfo.absorb);

        // Send log damage message to client
        caster->SendSpellNonMeleeDamageLog(&damageInfo);

        procEx |= createProcExtendMask(&damageInfo, missInfo);
        procVictim |= PROC_FLAG_TAKEN_DAMAGE;

        // Do triggers for unit (reflect triggers passed on hit phase for correct drop charge)
        if (canEffectTrigger && missInfo != SPELL_MISS_REFLECT)
        {
            caster->ProcDamageAndSpell(unitTarget, procAttacker, procVictim, procEx, damageInfo.damage, m_attackType, m_spellInfo, m_triggeredByAuraSpell);
            if (caster->GetTypeId() == TYPEID_PLAYER && (m_spellInfo->Attributes & SPELL_ATTR0_STOP_ATTACK_TARGET) == 0 &&
               (m_spellInfo->DmgClass == SPELL_DAMAGE_CLASS_MELEE || m_spellInfo->DmgClass == SPELL_DAMAGE_CLASS_RANGED))
                caster->ToPlayer()->CastItemCombatSpell(unitTarget, m_attackType, procVictim, procEx, m_spellInfo);
        }

        caster->DealSpellDamage(&damageInfo, true);

        // Used in spell scripts
        m_final_damage = damageInfo.damage;

        // Haunt
        if (m_spellInfo->SpellFamilyName == SPELLFAMILY_WARLOCK && m_spellInfo->SpellFamilyFlags[1] & 0x40000 && m_spellAura && m_spellAura->GetEffect(1))
        {
            AuraEffect * aurEff = m_spellAura->GetEffect(1);
            aurEff->SetAmount(aurEff->GetAmount() * damageInfo.damage / 100);
        }
    }
    // Passive spell hits/misses or active spells only misses (only triggers)
    else
    {
        // Fill base damage struct (unitTarget - is real spell target)
        SpellNonMeleeDamage damageInfo(caster, unitTarget, m_spellInfo->Id, m_spellSchoolMask);
        procEx |= createProcExtendMask(&damageInfo, missInfo);
        // Do triggers for unit (reflect triggers passed on hit phase for correct drop charge)
        if (canEffectTrigger && missInfo != SPELL_MISS_REFLECT)
            caster->ProcDamageAndSpell(unit, procAttacker, procVictim, procEx, 0, m_attackType, m_spellInfo, m_triggeredByAuraSpell);

        // Failed Pickpocket, reveal rogue
        if (missInfo == SPELL_MISS_RESIST
            && m_customAttr & SPELL_ATTR0_CU_PICKPOCKET
            && unitTarget->GetTypeId() == TYPEID_UNIT)
        {
            m_caster->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TALK);
            if (unitTarget->ToCreature()->IsAIEnabled)
                unitTarget->ToCreature()->AI()->AttackStart(m_caster);
        }
    }

    // Drop charge of magnet auras on hit
    if (m_magnetingAura)
    {
        if (!m_magnetingAura->IsRemoved() && m_magnetingAura->GetCharges()>0)
            m_magnetingAura->DropCharge();
        m_magnetingAura = NULL;
    }
	
    if (m_caster && !m_caster->IsFriendlyTo(unit) && !IsPositiveSpell(m_spellInfo->Id))
    {
        m_caster->CombatStart(unit, !(m_spellInfo->AttributesEx3 & SPELL_ATTR3_NO_INITIAL_AGGRO));

        if (m_customAttr & SPELL_ATTR0_CU_AURA_CC)
            if (!unit->IsStandState())
                unit->SetStandState(UNIT_STAND_STATE_STAND);
    }

    if (spellHitTarget)
    {
        //AI functions
        if (spellHitTarget->GetTypeId() == TYPEID_UNIT)
        {
            if (spellHitTarget->ToCreature()->IsAIEnabled)
                spellHitTarget->ToCreature()->AI()->SpellHit(m_caster, m_spellInfo);

            // cast at creature (or GO) quest objectives update at successful cast finished (+channel finished)
            // ignore pets or autorepeat/melee casts for speed (not exist quest for spells (hm...)
            if (m_originalCaster && m_originalCaster->IsControlledByPlayer() && !spellHitTarget->ToCreature()->IsPet() && !IsAutoRepeat() && !IsNextMeleeSwingSpell() && !IsChannelActive())
                if (Player* p = m_originalCaster->GetCharmerOrOwnerPlayerOrPlayerItself())
                    p->CastedCreatureOrGO(spellHitTarget->GetEntry(),spellHitTarget->GetGUID(),m_spellInfo->Id);
        }

        if (m_caster && m_caster->GetTypeId() == TYPEID_UNIT && m_caster->ToCreature()->IsAIEnabled)
            m_caster->ToCreature()->AI()->SpellHitTarget(spellHitTarget, m_spellInfo);

        // Needs to be called after dealing damage/healing to not remove breaking on damage auras
        DoTriggersOnSpellHit(spellHitTarget);

        // if target is fallged for pvp also flag caster if a player
        if (unit->IsPvP())
        {
            if (m_caster->GetTypeId() == TYPEID_PLAYER)
            m_caster->ToPlayer()->UpdatePvP(true);
        }

        CallScriptAfterHitHandlers();
    }
}

SpellMissInfo Spell::DoSpellHitOnUnit(Unit *unit, const uint32 effectMask, bool scaleAura)
{
    if (!unit || !effectMask)
        return SPELL_MISS_EVADE;

    // Recheck immune (only for delayed spells)
    if (m_spellInfo->speed && (unit->IsImmunedToDamage(m_spellInfo) || unit->IsImmunedToSpell(m_spellInfo)))
        return SPELL_MISS_IMMUNE;

    PrepareScriptHitHandlers();
    CallScriptBeforeHitHandlers();

    if (unit->GetTypeId() == TYPEID_PLAYER)
    {
        unit->ToPlayer()->GetAchievementMgr().StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_SPELL_TARGET, m_spellInfo->Id);
        unit->ToPlayer()->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET, m_spellInfo->Id, 0, m_caster);
        unit->ToPlayer()->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2, m_spellInfo->Id);
    }

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        m_caster->ToPlayer()->GetAchievementMgr().StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_SPELL_CASTER, m_spellInfo->Id);
        m_caster->ToPlayer()->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2, m_spellInfo->Id, 0, unit);
    }

    if (m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->getClass() == CLASS_DRUID && ((Player*)m_caster)->GetTalentBranchSpec(((Player*)m_caster)->GetActiveSpec()) == 752)
    {
        bool EclipseLeft = m_caster->ToPlayer()->IsEclipseDriverLeft();
        switch(m_spellInfo->Id)
        {
            // Moonfire - Lunar / Solar energy under Lunar Shower effect
            case 8921:
                {
                    // If player is under lunar shower effekt
                    if (m_caster->HasAura(81006) || m_caster->HasAura(81191) || m_caster->HasAura(81192))
                    {
                        int32 change = 0;
                        // If player is after eclipse reset and under lunar shower effect 
                        // his first Moonfire under lunar shower should be towards lunar eclipse
                        if (!m_caster->HasAura(67483) && !m_caster->HasAura(67484))
                        {
                            if (m_caster->GetPower(POWER_ECLIPSE) <= 0)
                                change = -8;
                            else
                                change = 8;
                        }
                        // If player is moving towards Solar eclipse
                        else if (!EclipseLeft)
                            change = 8;
                        // If player is moving towards Lunar eclipse
                        else
                            // (Hotfix) - Moonfire will now generate Lunar Energy when crossing from Solar to Lunar on the Eclipse bar.
                            change = -8;
                        // Modify energy
                        if (change != 0)
                            m_caster->ModifyPower(POWER_ECLIPSE, change);
                    }
                }
                break;

            // Sunfire - Lunar energy
            case 93402:
                // If player is under lunar shower effekt
                if (m_caster->HasAura(81006) || m_caster->HasAura(81191) || m_caster->HasAura(81192))
                {
                    // If we are moving towards lunar eclipse
                    if (EclipseLeft)
                    {
                        int32 bp0 = -8;
                        m_caster->ModifyPower(POWER_ECLIPSE, bp0);
                    }
                }
                break;

            // Wrath - Lunar energy
            case 5176:
                // Decrease energy towards lunar eclipse only when moving towards lunar eclipse or after eclipse reset
                if (EclipseLeft || (!m_caster->HasAura(67483) && !m_caster->HasAura(67484)))
                {
                    int32 bp0 = -13; // Wrath's base amount of energy

                    // T12 Balance 4P Bonus ( While not in an Eclipse state your Wrath generates 3 additional Lunar Energy )
                    if (m_caster->HasAura(99049) && !m_caster->HasAura(48517) && !m_caster->HasAura(48518)) 
                        bp0 -= 3;

                    // Talent Euphoria generates the same amount of energy again as a bonus
                    if (!m_caster->HasAura(48518) && !m_caster->HasAura(48517) &&
                        ((m_caster->HasAura(81062) && roll_chance_i(24))
                        || (m_caster->HasAura(81061) && roll_chance_i(12))))
                    {
                        // Gain doubled amount of base enrgy
                        m_caster->ModifyPower(POWER_ECLIPSE, 2*bp0);
                    }
                    else // Gain base amount of energy
                        m_caster->ModifyPower(POWER_ECLIPSE, bp0);

                }
                break;

            // Starfire - Solar energy
            case 2912:
                // Increase energy towards solar eclipse only when moving towards solar eclipse or after eclipse reset
                if (!EclipseLeft || (!m_caster->HasAura(67483) && !m_caster->HasAura(67484)))
                {
                    int32 bp0 = 20; // Starfire's base amount of energy

                    // T12 Balance 4P Bonus ( While not in an Eclipse state your Starfire generates 5 additional Solar Energy )
                    if (m_caster->HasAura(99049) && !m_caster->HasAura(48517) && !m_caster->HasAura(48518))
                        bp0 += 5;

                    // Talent Euphoria generates the same amount of energy again as a bonus
                    if (!m_caster->HasAura(48518) && !m_caster->HasAura(48517) &&
                        ((m_caster->HasAura(81062) && roll_chance_i(24))
                        || (m_caster->HasAura(81061) && roll_chance_i(12))))
                    {
                        // Gain doubled amount of base energy
                        m_caster->ModifyPower(POWER_ECLIPSE, 2*bp0);
                    }
                    else // Gain base amount of energy
                        m_caster->ModifyPower(POWER_ECLIPSE, bp0);
                }
                break;

            // Starsurge - Lunar or Solar energy
            case 78674:
                // Add amount of energy
                int32 change = 0;
                // If we are after eclipse reset Starsurge should be able to move energy both ways
                // depends if player is closer to lunar or solar eclipse, if he has 0 energy it moves towards solar
                if (!m_caster->HasAura(67483) && !m_caster->HasAura(67484))
                {
                    if (m_caster->GetPower(POWER_ECLIPSE) >= 0)
                        change = 15;
                    else
                        change = -15;
                }
                // If player is moving towards lunar eclipse decrease our energy
                else if (EclipseLeft)
                    change = -15;
                // If player is moving towards solar eclipse increase our energy
                else 
                    change = 15;
                // Finally modify energy
                if (change != 0)
                    m_caster->ModifyPower(POWER_ECLIPSE, change);
                break;
        }
    }

    // Banish cancells all the effect if reapplied
    if (m_caster && unit && unit->HasAura(710))
    {
        unit->RemoveAurasDueToSpell(710, m_caster->GetGUID());
        return SPELL_MISS_NONE;
    }

    if (m_caster != unit)
    {
        // Recheck  UNIT_FLAG_NON_ATTACKABLE for delayed spells
        if (m_spellInfo->speed > 0.0f &&
            unit->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE) &&
            unit->GetCharmerOrOwnerGUID() != m_caster->GetGUID())
        {
            return SPELL_MISS_EVADE;
        }

        if (!m_caster->IsFriendlyTo(unit))
        {
            // reset damage to 0 if target has Invisibility and isn't visible for caster
            // I do not think this is a correct way to fix it. Sanctuary effect should make all delayed spells invalid
            // for delayed spells ignore not visible explicit target
            if (m_spellInfo->speed > 0.0f && unit == m_targets.getUnitTarget()
                && (unit->m_invisibilityMask || m_caster->m_invisibilityMask)
                && !m_caster->canSeeOrDetect(unit, true))
            {
                // that was causing CombatLog errors
                // return SPELL_MISS_EVADE;
                return SPELL_MISS_MISS; // miss = do not send anything here
            }

            unit->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_HITBYSPELL);
            //TODO: This is a hack. But we do not know what types of stealth should be interrupted by CC
            if ((m_customAttr & SPELL_ATTR0_CU_AURA_CC) && unit->IsControlledByPlayer())
                unit->RemoveAurasByType(SPELL_AURA_MOD_STEALTH);
        }
        else
        {
            // for delayed spells ignore negative spells (after duel end) for friendly targets
            // TODO: this cause soul transfer bugged
            if (m_spellInfo->speed > 0.0f && unit->GetTypeId() == TYPEID_PLAYER && !IsPositiveSpell(m_spellInfo->Id))
            {
                return SPELL_MISS_EVADE;
            }

            // assisting case, healing and resurrection
            if (unit->HasUnitState(UNIT_STATE_ATTACK_PLAYER))
            {
                m_caster->SetContestedPvP();
                if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    m_caster->ToPlayer()->UpdatePvP(true);
            }
            if (unit->IsInCombat() && !(m_spellInfo->AttributesEx3 & SPELL_ATTR3_NO_INITIAL_AGGRO))
            {
                m_caster->SetInCombatState(unit->GetCombatTimer() > 0, unit);
                unit->getHostileRefManager().threatAssist(m_caster, 0.0f);
            }
        }
    }

    // Get Data Needed for Diminishing Returns, some effects may have multiple auras, so this must be done on spell hit, not aura add
    m_diminishGroup = GetDiminishingReturnsGroupForSpell(m_spellInfo,m_triggeredByAuraSpell);
    if (m_diminishGroup)
    {
        m_diminishLevel = unit->GetDiminishing(m_diminishGroup);
        DiminishingReturnsType type = GetDiminishingReturnsGroupType(m_diminishGroup);
        // Increase Diminishing on unit, current informations for actually casts will use values above
        if ((type == DRTYPE_PLAYER && unit->GetCharmerOrOwnerPlayerOrPlayerItself()) || type == DRTYPE_ALL)
            unit->IncrDiminishing(m_diminishGroup);
    }

    uint8 aura_effmask = 0;
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        if (effectMask & (1<<i) && IsUnitOwnedAuraEffect(m_spellInfo->Effect[i]))
            aura_effmask |= 1<<i;

    if (aura_effmask)
    {
        // Select rank for aura with level requirements only in specific cases
        // Unit has to be target only of aura effect, both caster and target have to be players, target has to be other than unit target
        SpellEntry const * aurSpellInfo = m_spellInfo;
        int32 basePoints[3];
        if (scaleAura)
        {
            aurSpellInfo = sSpellMgr->SelectAuraRankForPlayerLevel(m_spellInfo,unitTarget->getLevel());
            ASSERT(aurSpellInfo);
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            {
                basePoints[i] = aurSpellInfo->EffectBasePoints[i];
                if (m_spellInfo->Effect[i] != aurSpellInfo->Effect[i])
                {
                    aurSpellInfo = m_spellInfo;
                    break;
                }
            }
        }

        if (m_originalCaster)
        {
            m_spellAura = Aura::TryCreate(aurSpellInfo, (uint8)effectMask, (WorldObject*)unit,
                (Unit*)m_originalCaster, (aurSpellInfo == m_spellInfo)? &m_spellValue->EffectBasePoints[0] : &basePoints[0], &m_spellValue->EffectScriptedPoints[0], m_CastItem);
            if (m_spellAura)
            {
                // Now Reduce spell duration using data received at spell hit
                int32 duration = m_spellAura->GetMaxDuration();
                int32 limitduration = GetDiminishingReturnsLimitDuration(m_diminishGroup,aurSpellInfo);
                float diminishMod = unit->ApplyDiminishingToDuration(m_diminishGroup, duration, m_originalCaster, m_diminishLevel,limitduration);

                // unit is immune to aura if it was diminished to 0 duration
                if (diminishMod == 0.0f)
                {
                    m_spellAura->Remove();
                    return SPELL_MISS_IMMUNE;
                }

                ((UnitAura*)m_spellAura)->SetDiminishGroup(m_diminishGroup);

                bool positive = IsPositiveSpell(m_spellAura->GetId());
                AuraApplication * aurApp = m_spellAura->GetApplicationOfTarget(m_originalCaster->GetGUID());
                if (aurApp)
                    positive = aurApp->IsPositive();

                duration = m_originalCaster->ModSpellDuration(aurSpellInfo, unit, duration, positive);

                // Haste modifies duration of channeled spells
                if (IsChanneledSpell(m_spellInfo))
                {
                    if (m_spellInfo->AttributesEx5 & SPELL_ATTR5_HASTE_AFFECT_DURATION)

                        m_originalCaster->ModSpellCastTime(aurSpellInfo, duration, this);
                }

                if (duration != m_spellAura->GetMaxDuration())
                {
                    m_spellAura->SetMaxDuration(duration);
                    m_spellAura->SetDuration(duration);
                }
                m_spellAura->_RegisterForTargets();
            }
        }
    }

    for (uint32 effectNumber = 0; effectNumber < MAX_SPELL_EFFECTS; ++effectNumber)
    {
        if (effectMask & (1<<effectNumber))
            HandleEffects(unit,NULL,NULL,effectNumber);
    }

    return SPELL_MISS_NONE;
}

void Spell::DoTriggersOnSpellHit(Unit *unit)
{
    // Apply additional spell effects to target
    if (m_preCastSpell)
    {
        if (sSpellStore.LookupEntry(m_preCastSpell))
            // Blizz seems to just apply aura without bothering to cast
            m_caster->AddAura(m_preCastSpell, unit);
    }

    // spells with this flag can trigger only if not selfcast (eviscerate for example)
    if (m_ChanceTriggerSpells.size() && (!((m_spellInfo->AttributesEx4 & SPELL_ATTR4_CANT_PROC_FROM_SELFCAST) && unit == m_caster)))
    {
        int _duration=0;
        for (ChanceTriggerSpells::const_iterator i = m_ChanceTriggerSpells.begin(); i != m_ChanceTriggerSpells.end(); ++i)
        {
            // SPELL_AURA_ADD_TARGET_TRIGGER auras shouldn't trigger auras without duration
            // set duration equal to triggering spell
            if (roll_chance_i(i->second))
            {
                // Disallow applying on self (i.e. when logging in)
                if (   i->first->Id == 81325 || i->first->Id == 81326  // Brittle Bones
                    || i->first->Id == 50434 || i->first->Id == 50435  // Chilblains
                    || i->first->Id == 81130 || i->first->Id == 96265 // Scarlet Fever
                    || i->first->Id == 30069 || i->first->Id == 30070) // Blood Frenzy 
                {
                    if (unit == m_caster)
                        continue;
                }
                m_caster->CastSpell(unit, i->first, true);
                sLog->outDebug("Spell %d triggered spell %d by SPELL_AURA_ADD_TARGET_TRIGGER aura", m_spellInfo->Id, i->first->Id);
            }
            if (GetSpellDuration(i->first) == -1)
            {
                if (Aura * triggeredAur = unit->GetAura(i->first->Id, m_caster->GetGUID()))
                {
                    // get duration from aura-only once
                    if (!_duration)
                    {
                        Aura * aur = unit->GetAura(m_spellInfo->Id, m_caster->GetGUID());
                        _duration = aur ? aur->GetDuration() : -1;
                    }
                    triggeredAur->SetDuration(_duration);
                }
            }
        }
    }

    if (m_customAttr & SPELL_ATTR0_CU_LINK_HIT)
    {
        if (const std::vector<int32> *spell_triggered = sSpellMgr->GetSpellLinked(m_spellInfo->Id + SPELL_LINK_HIT))
        {
            for (std::vector<int32>::const_iterator i = spell_triggered->begin(); i != spell_triggered->end(); ++i)
            {
                if (*i < 0)
                    unit->RemoveAurasDueToSpell(-(*i));
                else
                    unit->CastSpell(unit, *i, true, 0, 0, m_caster->GetGUID());
            }
        }
    }
}

void Spell::DoAllEffectOnTarget(GOTargetInfo *target)
{
    if (target->processed)                                  // Check target
        return;
    target->processed = true;                               // Target checked in apply effects procedure

    uint32 effectMask = target->effectMask;
    if (!effectMask)
        return;

    GameObject* go = m_caster->GetMap()->GetGameObject(target->targetGUID);
    if (!go)
        return;

    PrepareScriptHitHandlers();
    CallScriptBeforeHitHandlers();

    for (uint32 effectNumber = 0; effectNumber < MAX_SPELL_EFFECTS; ++effectNumber)
        if (effectMask & (1 << effectNumber))
            HandleEffects(NULL, NULL, go, effectNumber);

    CallScriptOnHitHandlers();

    // cast at creature (or GO) quest objectives update at successful cast finished (+channel finished)
    // ignore autorepeat/melee casts for speed (not exist quest for spells (hm...)
    if (m_originalCaster && m_originalCaster->IsControlledByPlayer() && !IsAutoRepeat() && !IsNextMeleeSwingSpell() && !IsChannelActive())
    {
        if (Player* p = m_originalCaster->GetCharmerOrOwnerPlayerOrPlayerItself())
            p->CastedCreatureOrGO(go->GetEntry(),go->GetGUID(),m_spellInfo->Id);
    }
    CallScriptAfterHitHandlers();
}

void Spell::DoAllEffectOnTarget(ItemTargetInfo *target)
{
    uint32 effectMask = target->effectMask;
    if (!target->item || !effectMask)
        return;

    PrepareScriptHitHandlers();
    CallScriptBeforeHitHandlers();

    for (uint32 effectNumber = 0; effectNumber < MAX_SPELL_EFFECTS; ++effectNumber)
        if (effectMask & (1 << effectNumber))
            HandleEffects(NULL, target->item, NULL, effectNumber);

    CallScriptOnHitHandlers();

    CallScriptAfterHitHandlers();
}

bool Spell::UpdateChanneledTargetList()
{
    // Not need check return true
    if (m_channelTargetEffectMask == 0)
        return true;

    uint8 channelTargetEffectMask = m_channelTargetEffectMask;
    uint8 channelAuraMask = 0;
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        if (m_spellInfo->Effect[i] == SPELL_EFFECT_APPLY_AURA)
            channelAuraMask |= 1<<i;

    channelAuraMask &= channelTargetEffectMask;

    float range = 0;
    if (channelAuraMask)
    {
        range = GetSpellMaxRange(m_spellInfo, IsPositiveSpell(m_spellInfo->Id));
        if (Player * modOwner = m_caster->GetSpellModOwner())
            modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_RANGE, range, this);
    }

    for (std::list<TargetInfo>::iterator ihit= m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
    {
        if (ihit->missCondition == SPELL_MISS_NONE && (channelTargetEffectMask & ihit->effectMask))
        {
            Unit *unit = m_caster->GetGUID() == ihit->targetGUID ? m_caster : ObjectAccessor::GetUnit(*m_caster, ihit->targetGUID);

            if (!unit)
                continue;

            if (IsValidDeadOrAliveTarget(unit))
            {
                if (channelAuraMask & ihit->effectMask)
                {
                    if (AuraApplication * aurApp = unit->GetAuraApplication(m_spellInfo->Id, m_originalCasterGUID))
                    {
                        if (m_caster != unit && !m_caster->IsWithinDistInMap(unit,range))
                        {
                            ihit->effectMask &= ~aurApp->GetEffectMask();
                            unit->RemoveAura(aurApp);
                            continue;
                        }
                    }
                    else // aura is dispelled
                        continue;
                }

                channelTargetEffectMask &= ~ihit->effectMask;   // remove from need alive mask effect that have alive target
            }
        }
    }

    // is all effects from m_needAliveTargetMask have alive targets
    return channelTargetEffectMask == 0;
}

// Helper for Chain Healing
// Spell target first
// Raidmates then descending by injury suffered (MaxHealth - Health)
// Other players/mobs then descending by injury suffered (MaxHealth - Health)
struct ChainHealingOrder : public std::binary_function<const Unit*, const Unit*, bool>
{
    const Unit* MainTarget;
    ChainHealingOrder(Unit const* Target) : MainTarget(Target) {};
    // functor for operator ">"
    bool operator()(Unit const* _Left, Unit const* _Right) const
    {
        return (ChainHealingHash(_Left) < ChainHealingHash(_Right));
    }

    int32 ChainHealingHash(Unit const* Target) const
    {
        /*if (Target == MainTarget)
            return 0;
        else*/ if (Target->GetTypeId() == TYPEID_PLAYER && MainTarget->GetTypeId() == TYPEID_PLAYER &&
           Target->ToPlayer()->IsInSameRaidWith(MainTarget->ToPlayer()))
        {
            if (Target->IsFullHealth())
                return 40000;
            else
                return 20000 - Target->GetMaxHealth() + Target->GetHealth();
        }
        else
            return 40000 - Target->GetMaxHealth() + Target->GetHealth();
    }
};

void Spell::SearchChainTarget(std::list<Unit*> &TagUnitMap, float max_range, uint32 num, SpellTargets TargetType)
{
    Unit *cur = m_targets.getUnitTarget();
    if (!cur)
        return;

    // Get spell max affected targets
    /*uint32 unMaxTargets = m_spellInfo->MaxAffectedTargets;
    Unit::AuraList const& mod = m_caster->GetAuraEffectsByType(SPELL_AURA_MOD_MAX_AFFECTED_TARGETS);
    for (Unit::AuraList::const_iterator m = mod.begin(); m != mod.end(); ++m)
    {
        if (!(*m)->IsAffectedOnSpell(m_spellInfo))
            continue;
        unMaxTargets+=(*m)->GetAmount();
    }*/

    //FIXME: This very like horrible hack and wrong for most spells
    if (m_spellInfo->DmgClass != SPELL_DAMAGE_CLASS_MELEE)
        max_range += num * CHAIN_SPELL_JUMP_RADIUS;

    std::list<Unit*> tempUnitMap;
    if (TargetType == SPELL_TARGETS_CHAINHEAL)
    {
        SearchAreaTarget(tempUnitMap, max_range, PUSH_CHAIN, SPELL_TARGETS_ALLY);
        tempUnitMap.sort(ChainHealingOrder(m_caster));
        //if (cur->IsFullHealth() && tempUnitMap.size())
        //    cur = tempUnitMap.front();
    }
    else
        SearchAreaTarget(tempUnitMap, max_range, PUSH_CHAIN, TargetType);
    tempUnitMap.remove(cur);

    while (num)
    {
        TagUnitMap.push_back(cur);
        --num;

        if (tempUnitMap.empty())
            break;

        std::list<Unit*>::iterator next;

        if (TargetType == SPELL_TARGETS_CHAINHEAL)
        {
            next = tempUnitMap.begin();
            while (cur->GetDistance(*next) > CHAIN_SPELL_JUMP_RADIUS
                || !cur->IsWithinLOSInMap(*next))
            {
                ++next;
                if (next == tempUnitMap.end())
                    return;
            }
        }
        else
        {
            tempUnitMap.sort(Trinity::ObjectDistanceOrderPred(cur));
            next = tempUnitMap.begin();

            if (cur->GetDistance(*next) > CHAIN_SPELL_JUMP_RADIUS)      // Don't search beyond the max jump radius
                break;
    
            // Check if (*next) is a valid chain target. If not, don't add to TagUnitMap, and repeat loop.
            // If you want to add any conditions to exclude a target from TagUnitMap, add condition in this while() loop.    
            while ((m_spellInfo->DmgClass == SPELL_DAMAGE_CLASS_MELEE  
                && !m_caster->isInFrontInMap(*next, max_range))
                || !m_caster->canSeeOrDetect(*next, false)
                || !cur->IsWithinLOSInMap(*next)
                || (*next)->GetCreatureType() == CREATURE_TYPE_CRITTER
                || ((GetSpellInfo()->AttributesEx6 & SPELL_ATTR6_IGNORE_CROWD_CONTROL_TARGETS) && !(*next)->CanFreeMove()))
            {
                ++next;
                if (next == tempUnitMap.end() || cur->GetDistance(*next) > CHAIN_SPELL_JUMP_RADIUS) // Don't search beyond the max jump radius
                    return;
            }
        }

        cur = *next;
        tempUnitMap.erase(next);
    }
}

float Spell::GetEffectRadius(uint32 effIndex)
{
    return m_spellInfo->GetSpellRadius(m_caster, effIndex);
}

void Spell::SearchAreaTarget(std::list<Unit*> &TagUnitMap, float radius, SpellNotifyPushType type, SpellTargets TargetType, uint32 entry, bool extendRadius)
{
    if (TargetType == SPELL_TARGETS_GO)
        return;

    Position *pos;
    switch(type)
    {
        case PUSH_DST_CENTER:
            CheckDst();
            pos = &m_targets.m_dstPos;
            break;
        case PUSH_SRC_CENTER:
            CheckSrc();
            pos = &m_targets.m_srcPos;
            break;
        case PUSH_CHAIN:
        {
            Unit *target = m_targets.getUnitTarget();
            if (!target)
            {
                sLog->outError("SPELL: cannot find unit target for spell ID %u\n", m_spellInfo->Id);
                return;
            }
            pos = target;
            break;
        }
        default:
            pos = m_caster;
            break;
    }

    // This stuff is reqired to properly add units (mostly creatures) with huge combat reach to unitmap
    // Radius must be extended and then we need to cut units which are not in (original radius + combat reach of unit) range
    // This system is allowed only in dungeon/raid maps and only for players AoE (enemy) spells
    bool radiusExtensionAllowed = false;
    Map * map = m_caster->GetMap();
    #define RADIUS_ADDITION (50.0f)

    if (extendRadius && map)
    {
        if (map->IsRaid() || map->IsNonRaidDungeon())
        {
            radius = radius + RADIUS_ADDITION;
            radiusExtensionAllowed = true;
        }
    }

    bool requireDeadTarget = bool(m_spellInfo->AttributesEx3 & SPELL_ATTR3_REQUIRE_DEAD_TARGET);
    Trinity::SpellNotifierCreatureAndPlayer notifier(m_caster, TagUnitMap, radius, type, m_spellInfo, TargetType, pos, entry, requireDeadTarget);
    if ((m_spellInfo->AttributesEx3 & SPELL_ATTR3_PLAYERS_ONLY)
        || (TargetType == SPELL_TARGETS_ENTRY && !entry))
        m_caster->GetMap()->VisitWorld(pos->m_positionX, pos->m_positionY, radius, notifier);
    else
        m_caster->GetMap()->VisitAll(pos->m_positionX, pos->m_positionY, radius, notifier);

    if (m_customAttr & SPELL_ATTR0_CU_EXCLUDE_SELF)
        TagUnitMap.remove(m_caster);

    if (radiusExtensionAllowed)
    {
        for (std::list<Unit*>::iterator itr = TagUnitMap.begin(); itr != TagUnitMap.end();)
        {
            if ((*itr)->GetExactDist(pos) <= (radius - RADIUS_ADDITION) + (*itr)->GetObjectSize())
                itr++;
            else
                itr = TagUnitMap.erase(itr);
        }
    }
}

void Spell::SearchGOAreaTarget(std::list<GameObject*> &TagGOMap, float radius, SpellNotifyPushType type, SpellTargets TargetType, uint32 entry)
{
    if (TargetType != SPELL_TARGETS_GO)
        return;

    Position *pos;
    switch (type)
    {
        case PUSH_DST_CENTER:
            CheckDst();
            pos = &m_targets.m_dstPos;
            break;
        case PUSH_SRC_CENTER:
            CheckSrc();
            pos = &m_targets.m_srcPos;
            break;
        default:
            pos = m_caster;
            break;
    }

    Trinity::GameObjectInRangeCheck check(pos->m_positionX, pos->m_positionY, pos->m_positionZ, radius, entry);
    Trinity::GameObjectListSearcher<Trinity::GameObjectInRangeCheck> searcher(m_caster, TagGOMap, check);
    m_caster->GetMap()->VisitGrid(pos->m_positionX, pos->m_positionY, radius, searcher);
}

WorldObject* Spell::SearchNearbyTarget(float range, SpellTargets TargetType, SpellEffIndex effIndex)
{
    switch(TargetType)
    {
        case SPELL_TARGETS_ENTRY:
        {
            ConditionList conditions = sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_SPELL_SCRIPT_TARGET, m_spellInfo->Id);
            if (conditions.empty())
            {
                sLog->outDebug("Spell (ID: %u) (caster Entry: %u) does not have record in `conditions` for spell script target (ConditionSourceType 13)", m_spellInfo->Id, m_caster->GetEntry());
                if (IsPositiveSpell(m_spellInfo->Id))
                    return SearchNearbyTarget(range, SPELL_TARGETS_ALLY, effIndex);
                else
                    return SearchNearbyTarget(range, SPELL_TARGETS_ENEMY, effIndex);
            }

            Creature* creatureScriptTarget = NULL;
            GameObject* goScriptTarget = NULL;

            for (ConditionList::const_iterator i_spellST = conditions.begin(); i_spellST != conditions.end(); ++i_spellST)
            {
                if ((*i_spellST)->mConditionType != CONDITION_SPELL_SCRIPT_TARGET)
                    continue;
                if ((*i_spellST)->mConditionValue3 && !((*i_spellST)->mConditionValue3 & (1 << uint32(effIndex))))
                    continue;
                switch((*i_spellST)->mConditionValue1)
                {
                    case SPELL_TARGET_TYPE_CONTROLLED:
                        for (Unit::ControlList::iterator itr = m_caster->m_Controlled.begin(); itr != m_caster->m_Controlled.end(); ++itr)
                            if ((*itr)->GetEntry() == (*i_spellST)->mConditionValue2 && (*itr)->IsWithinDistInMap(m_caster, range))
                            {
                                goScriptTarget = NULL;
                                creatureScriptTarget = (*itr)->ToCreature();
                                range = m_caster->GetDistance(creatureScriptTarget);
                            }
                        break;
                    case SPELL_TARGET_TYPE_GAMEOBJECT:
                        if ((*i_spellST)->mConditionValue2)
                        {
                            if (GameObject *go = m_caster->FindNearestGameObject((*i_spellST)->mConditionValue2, range))
                            {
                                // remember found target and range, next attempt will find more near target with another entry
                                goScriptTarget = go;
                                creatureScriptTarget = NULL;
                                range = m_caster->GetDistance(goScriptTarget);
                            }
                        }
                        else if (focusObject)          //Focus Object
                        {
                            float frange = m_caster->GetDistance(focusObject);
                            if (range >= frange)
                            {
                                creatureScriptTarget = NULL;
                                goScriptTarget = focusObject;
                                range = frange;
                            }
                        }
                        break;
                    case SPELL_TARGET_TYPE_CREATURE:
                        if (m_targets.getUnitTarget() && m_targets.getUnitTarget()->GetEntry() == (*i_spellST)->mConditionValue2)
                            return m_targets.getUnitTarget();
                    case SPELL_TARGET_TYPE_DEAD:
                    default:
                        if (Creature *cre = m_caster->FindNearestCreature((*i_spellST)->mConditionValue2, range, (*i_spellST)->mConditionValue1 != SPELL_TARGET_TYPE_DEAD))
                        {
                            creatureScriptTarget = cre;
                            goScriptTarget = NULL;
                            range = m_caster->GetDistance(creatureScriptTarget);
                        }
                        break;
                }
            }

            if (creatureScriptTarget)
                return creatureScriptTarget;
            else
                return goScriptTarget;
        }
        default:
        case SPELL_TARGETS_ENEMY:
        {
            Unit *target = NULL;
            Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(m_caster, m_caster, range);
            Trinity::UnitLastSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(m_caster, target, u_check);
            m_caster->VisitNearbyObject(range, searcher);
            return target;
        }
        case SPELL_TARGETS_ALLY:
        {
            Unit *target = NULL;
            Trinity::AnyFriendlyUnitInObjectRangeCheck u_check(m_caster, m_caster, range);
            Trinity::UnitLastSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(m_caster, target, u_check);
            m_caster->VisitNearbyObject(range, searcher);
            return target;
        }
    }
}

void Spell::SelectEffectTargets(uint32 i, uint32 cur)
{
    SpellNotifyPushType pushType = PUSH_NONE;
    Player *modOwner = NULL;
    if (m_originalCaster)
        modOwner = m_originalCaster->GetSpellModOwner();

    switch(SpellTargetType[cur])
    {
        case TARGET_TYPE_UNIT_CASTER:
        {
            switch(cur)
            {
                case TARGET_UNIT_CASTER:
                case TARGET_UNIT_CASTER_UNKNOWN:
                    AddUnitTarget(m_caster, i);
                    break;
                case TARGET_UNIT_CASTER_FISHING:
                {
                    float min_dis = GetSpellMinRange(m_spellInfo, true);
                    float max_dis = GetSpellMaxRange(m_spellInfo, true);
                    float dis = (float)rand_norm() * (max_dis - min_dis) + min_dis;
                    float x, y, z;
                    m_caster->GetClosePoint(x, y, z, DEFAULT_WORLD_OBJECT_SIZE, dis);
                    m_targets.setDst(x, y, z, m_caster->GetOrientation());
                    break;
                }
                case TARGET_UNIT_MASTER:
                    if (Unit* owner = m_caster->GetCharmerOrOwner())
                        AddUnitTarget(owner, i);
                    break;
                case TARGET_UNIT_PET:
                    if (Guardian* pet = m_caster->GetGuardianPet())
                        AddUnitTarget(pet, i);
                    break;
                case TARGET_UNIT_SUMMONER:
                    if (m_caster->IsSummon())
                        if (Unit* unit = m_caster->ToTempSummon()->GetSummoner())
                            AddUnitTarget(unit, i);
                    break;
                case TARGET_UNIT_PARTY_CASTER:
                case TARGET_UNIT_RAID_CASTER:
                    pushType = PUSH_CASTER_CENTER;
                    break;
                case TARGET_UNIT_VEHICLE:
                    if (Unit *vehicle = m_caster->GetVehicleBase())
                        AddUnitTarget(vehicle, i);
                    break;
                case TARGET_UNIT_PASSENGER_0:
                case TARGET_UNIT_PASSENGER_1:
                case TARGET_UNIT_PASSENGER_2:
                case TARGET_UNIT_PASSENGER_3:
                case TARGET_UNIT_PASSENGER_4:
                case TARGET_UNIT_PASSENGER_5:
                case TARGET_UNIT_PASSENGER_6:
                case TARGET_UNIT_PASSENGER_7:
                    if (m_caster->GetTypeId() == TYPEID_UNIT && m_caster->ToCreature()->IsVehicle())
                        if (Unit *unit = m_caster->GetVehicleKit()->GetPassenger(cur - TARGET_UNIT_PASSENGER_0))
                            AddUnitTarget(unit, i);
                    break;
            }
            break;
        }

        case TARGET_TYPE_UNIT_TARGET:
        {
            Unit *target = m_targets.getUnitTarget();
            if (!target)
            {
                sLog->outError("SPELL: no unit target for spell ID %u", m_spellInfo->Id);
                break;
            }

            switch(cur)
            {
                case TARGET_UNIT_TARGET_ENEMY:
                    if (Unit *magnet = m_caster->SelectMagnetTarget(target, m_spellInfo))
                        if (magnet != target)
                            m_targets.setUnitTarget(magnet);
                    pushType = PUSH_CHAIN;
                    break;
                case TARGET_UNIT_TARGET_ANY:
                    if (!IsPositiveSpell(m_spellInfo->Id))
                        if (Unit *magnet = m_caster->SelectMagnetTarget(target, m_spellInfo))
                            if (magnet != target)
                                m_targets.setUnitTarget(magnet);
                    pushType = PUSH_CHAIN;
                    break;
                case TARGET_UNIT_CHAINHEAL:
                    pushType = PUSH_CHAIN;
                    break;
                case TARGET_UNIT_TARGET_ALLY:
                case TARGET_UNIT_TARGET_RAID:
                case TARGET_UNIT_TARGET_PARTY:
                case TARGET_UNIT_TARGET_PUPPET:
                    AddUnitTarget(target, i);
                    break;
                case TARGET_UNIT_PARTY_TARGET:
                case TARGET_UNIT_CLASS_TARGET:
                    pushType = PUSH_CASTER_CENTER; // not real
                    break;
            }
            break;
        }

        case TARGET_TYPE_UNIT_NEARBY:
        {
            WorldObject *target = NULL;
            float range;

            switch(cur)
            {
                case TARGET_UNIT_NEARBY_ENEMY:
                    range = GetSpellMaxRange(m_spellInfo, false);
                    if (modOwner) modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_RANGE, range, this);
                    target = SearchNearbyTarget(range, SPELL_TARGETS_ENEMY, SpellEffIndex(i));
                    break;
                case TARGET_UNIT_NEARBY_ALLY:
                case TARGET_UNIT_NEARBY_ALLY_UNK:
                case TARGET_UNIT_NEARBY_RAID:
                    range = GetSpellMaxRange(m_spellInfo, true);
                    if (modOwner) modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_RANGE, range, this);
                    target = SearchNearbyTarget(range, SPELL_TARGETS_ALLY, SpellEffIndex(i));
                    break;
                case TARGET_UNIT_NEARBY_ENTRY:
                case TARGET_GAMEOBJECT_NEARBY_ENTRY:
                    range = GetSpellMaxRange(m_spellInfo, IsPositiveSpell(m_spellInfo->Id));
                    if (modOwner) modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_RANGE, range, this);
                    target = SearchNearbyTarget(range, SPELL_TARGETS_ENTRY, SpellEffIndex(i));
                    break;
            }

            if (!target)
                return;
            else if (target->GetTypeId() == TYPEID_GAMEOBJECT)
                AddGOTarget((GameObject*)target, i);
            else
            {
                pushType = PUSH_CHAIN;

                if (m_targets.getUnitTarget() != target)
                    m_targets.setUnitTarget((Unit*)target);
            }

            break;
        }

        case TARGET_TYPE_AREA_SRC:
            pushType = PUSH_SRC_CENTER;
            break;

        case TARGET_TYPE_AREA_DST:
            pushType = PUSH_DST_CENTER;
            break;

        case TARGET_TYPE_AREA_CONE:
            if (m_customAttr & SPELL_ATTR0_CU_CONE_BACK)
                pushType = PUSH_IN_BACK;
            else if (m_customAttr & SPELL_ATTR0_CU_CONE_LINE)
                pushType = PUSH_IN_LINE;
            else
                pushType = PUSH_IN_FRONT;
            break;

        case TARGET_TYPE_DEST_CASTER: //4+8+2
        {
            if (cur == TARGET_SRC_CASTER)
            {
                m_targets.setSrc(*m_caster);
                break;
            }
            else if (cur == TARGET_DST_CASTER)
            {
                m_targets.setDst(*m_caster);

                // This is countermeasure for spells with non-traditional spell targetting specified in primary target
                // This implicitly selects the caster as primary target
                if (EffectTargetType[m_spellInfo->Effect[i]] == SPELL_REQUIRE_UNIT && m_spellInfo->EffectImplicitTargetB[i] == TARGET_NONE)
                    AddUnitTarget(m_caster, i);
                break;
            }

            float angle, dist;

            float objSize = m_caster->GetObjectSize();
            dist = GetEffectRadius(i);

            if (dist < objSize)
                dist = objSize;
            else if (cur == TARGET_DEST_CASTER_RANDOM)
                dist = objSize + (dist - objSize) * (float)rand_norm();

            switch(cur)
            {
                case TARGET_DEST_CASTER_FRONT_LEFT: angle = static_cast<float>(-M_PI/4);                   break;
                case TARGET_DEST_CASTER_BACK_LEFT:  angle = static_cast<float>(-3*M_PI/4);                 break;
                case TARGET_DEST_CASTER_BACK_RIGHT: angle = static_cast<float>(3*M_PI/4);                  break;
                case TARGET_DEST_CASTER_FRONT_RIGHT:angle = static_cast<float>(M_PI/4);                    break;
                case TARGET_MINION:
                case TARGET_DEST_CASTER_FRONT_LEAP:
                case TARGET_DEST_CASTER_FRONT:      angle = 0.0f;                                          break;
                case TARGET_DEST_CASTER_BACK:       angle = static_cast<float>(M_PI);                      break;
                case TARGET_DEST_CASTER_RIGHT:      angle = static_cast<float>(M_PI/2);                    break;
                case TARGET_DEST_CASTER_LEFT:       angle = static_cast<float>(-M_PI/2);                   break;
                default:                            angle = (float)rand_norm()*static_cast<float>(2*M_PI); break;
            }

            Position pos;
            if (cur == TARGET_DEST_CASTER_FRONT_LEAP)
                m_caster->GetFirstIncrementalCollisionPosition(pos, dist, angle);
            else
                m_caster->GetNearPosition(pos, dist, angle);

            if (cur == TARGET_DEST_CASTER_FRONT)
            {
                while (!m_caster->IsWithinLOS(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ()))
                {
                    dist -= 1.0f;
                    if (dist <= 1.0f)
                        break;

                    m_caster->GetFirstCollisionPosition(pos, dist, angle);
                }

                pos.m_positionZ = m_caster->GetMap()->GetHeight2(pos.m_positionX, pos.m_positionY, pos.m_positionZ+2.0f);
                m_caster->UpdateAllowedPositionZ(pos.m_positionX, pos.m_positionY, pos.m_positionZ);
            }

            m_targets.setDst(*m_caster);
            m_targets.modDst(pos);
            break;
        }

        case TARGET_TYPE_DEST_TARGET: //2+8+2
        {
            Unit *target = m_targets.getUnitTarget();
            if (!target)
            {
                sLog->outError("SPELL: no unit target for spell ID %u", m_spellInfo->Id);
                break;
            }

            if (cur == TARGET_DST_TARGET_ENEMY || cur == TARGET_DEST_TARGET_ANY)
            {
                m_targets.setDst(*target);
                break;
            }

            float angle, dist;

            float objSize = target->GetObjectSize();
            dist = GetEffectRadius(i);

            if (dist < objSize)
                dist = objSize;
            else if (cur == TARGET_DEST_TARGET_RANDOM)
                dist = objSize + (dist - objSize) * (float)rand_norm();

            switch(cur)
            {
                case TARGET_DEST_TARGET_FRONT:      angle = 0.0f;                                          break;
                case TARGET_DEST_TARGET_BACK:       angle = static_cast<float>(M_PI);                      break;
                case TARGET_DEST_TARGET_RIGHT:      angle = static_cast<float>(M_PI/2);                    break;
                case TARGET_DEST_TARGET_LEFT:       angle = static_cast<float>(-M_PI/2);                   break;
                case TARGET_DEST_TARGET_FRONT_LEFT: angle = static_cast<float>(-M_PI/4);                   break;
                case TARGET_DEST_TARGET_BACK_LEFT:  angle = static_cast<float>(-3*M_PI/4);                 break;
                case TARGET_DEST_TARGET_BACK_RIGHT: angle = static_cast<float>(3*M_PI/4);                  break;
                case TARGET_DEST_TARGET_FRONT_RIGHT:angle = static_cast<float>(M_PI/4);                    break;
                default:                            angle = (float)rand_norm()*static_cast<float>(2*M_PI); break;
            }

            Position pos;
            target->GetNearPosition(pos, dist, angle);
            m_targets.setDst(*target);
            m_targets.modDst(pos);
            break;
        }

        case TARGET_TYPE_DEST_DEST: //5+8+1
        {
            if (!m_targets.HasDst())
            {
                sLog->outError("SPELL: no destination for spell ID %u", m_spellInfo->Id);
                break;
            }

            float angle;
            switch(cur)
            {
                case TARGET_DEST_DYNOBJ_ENEMY:
                case TARGET_DEST_DYNOBJ_ALLY:
                case TARGET_DEST_DYNOBJ_ALL_UNITS:
                case TARGET_DEST_DEST:
                    return;
                case TARGET_DEST_TRAJ:
                    SelectTrajTargets();
                    return;
                case TARGET_DEST_DEST_FRONT:      angle = 0.0f;                                          break;
                case TARGET_DEST_DEST_BACK:       angle = static_cast<float>(M_PI);                      break;
                case TARGET_DEST_DEST_RIGHT:      angle = static_cast<float>(M_PI/2);                    break;
                case TARGET_DEST_DEST_LEFT:       angle = static_cast<float>(-M_PI/2);                   break;
                case TARGET_DEST_DEST_FRONT_LEFT: angle = static_cast<float>(-M_PI/4);                   break;
                case TARGET_DEST_DEST_BACK_LEFT:  angle = static_cast<float>(-3*M_PI/4);                 break;
                case TARGET_DEST_DEST_BACK_RIGHT: angle = static_cast<float>(3*M_PI/4);                  break;
                case TARGET_DEST_DEST_FRONT_RIGHT:angle = static_cast<float>(M_PI/4);                    break;
                default:                          angle = (float)rand_norm()*static_cast<float>(2*M_PI); break;
            }

            float dist;
            dist = GetEffectRadius(i);
            if (cur == TARGET_DEST_DEST_RANDOM || cur == TARGET_DEST_DEST_RANDOM_DIR_DIST)
                dist *= (float)rand_norm();

            // must has dst, no need to set flag
            Position pos = m_targets.m_dstPos;

            if (m_spellInfo->Id == 98010) // Volcanic Birth (Firelands)
            {
                // summon creature somewhere on his platform (not in our direction)
                angle = frand(M_PI / 4.0f,(M_PI));
                angle = (urand(0,1)) ? angle : -angle;
                dist  = frand(0.0f,42.0f);

                pos.m_positionX = -374.337006f;
                pos.m_positionY = -318.489990f;
                pos.m_positionZ = 100.413002f;
            }

            m_caster->MovePosition(pos, dist, angle);
            m_targets.modDst(pos);
            break;
        }

        case TARGET_TYPE_DEST_SPECIAL:
        {
            switch(cur)
            {
                case TARGET_DST_DB:
                    if (SpellTargetPosition const* st = sSpellMgr->GetSpellTargetPosition(m_spellInfo->Id))
                    {
                        //TODO: fix this check
                        if (m_spellInfo->Effect[0] == SPELL_EFFECT_TELEPORT_UNITS
                            || m_spellInfo->Effect[1] == SPELL_EFFECT_TELEPORT_UNITS
                            || m_spellInfo->Effect[2] == SPELL_EFFECT_TELEPORT_UNITS)
                            m_targets.setDst(st->target_X, st->target_Y, st->target_Z, st->target_Orientation, (int32)st->target_mapId);
                        else if (st->target_mapId == m_caster->GetMapId())
                            m_targets.setDst(st->target_X, st->target_Y, st->target_Z, st->target_Orientation);
                    }
                    else
                    {
                        sLog->outDebug("SPELL: unknown target coordinates for spell ID %u", m_spellInfo->Id);
                        Unit *target = NULL;
                        if (uint64 guid = m_caster->GetUInt64Value(UNIT_FIELD_TARGET))
                            target = ObjectAccessor::GetUnit(*m_caster, guid);
                        m_targets.setDst(target ? *target : *m_caster);
                    }
                    break;
                case TARGET_DST_HOME:
                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                        m_targets.setDst(m_caster->ToPlayer()->m_homebindX,m_caster->ToPlayer()->m_homebindY,m_caster->ToPlayer()->m_homebindZ, m_caster->ToPlayer()->GetOrientation(), m_caster->ToPlayer()->m_homebindMapId);
                    break;
                case TARGET_DST_NEARBY_ENTRY:
                {
                    float range = GetSpellMaxRange(m_spellInfo, IsPositiveSpell(m_spellInfo->Id));
                    if (modOwner) modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_RANGE, range, this);

                    // Allow AfterUnitTargetSelectHandlers force push some units in case of TARGET_DST_NEARBY_ENTRY
                    std::list<Unit*> unitList;
                    CallScriptAfterUnitTargetSelectHandlers(unitList, SpellEffIndex(i));

                    // Ring Toss - Darkmoon Faire
                    // explicit randomization since missile handling is little bit fucked up
                    // and also the spell target is set to be "nearby_entry", which is, obviously, wrong for us and our implementation
                    if (m_spellInfo->Id == 101699)
                    {
                        if (WorldObject *target = SearchNearbyTarget(range, SPELL_TARGETS_ENTRY, SpellEffIndex(i)))
                        {
                            float x, y, z;
                            target->GetPosition(x, y, z);
                            float radius = (((float)urand(10,100)) / 100.0f)*3.0f;
                            float angle = ((float)urand(0, 2*M_PI*100.0f))/100.0f;
                            x += radius * cos(angle);
                            y += radius * sin(angle);
                            m_targets.setDst(x, y, z, 0.0f);
                        }
                    }
                    else if (!unitList.empty()) // Add first target in list (in case unit was added explicitly via SpellScript)
                    {
                        m_targets.setDst(*(unitList.front()));
                    }
                    // Standard case
                    else if (WorldObject *target = SearchNearbyTarget(range, SPELL_TARGETS_ENTRY, SpellEffIndex(i)))
                        m_targets.setDst(*target);
                    break;
                }
            }
            break;
        }

        case TARGET_TYPE_CHANNEL:
        {
            if (!m_originalCaster || !m_originalCaster->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
            {
                sLog->outDebug("SPELL: no current channeled spell for spell ID %u - spell triggering this spell was interrupted.", m_spellInfo->Id);
                break;
            }

            switch (cur)
            {
                case TARGET_UNIT_CHANNEL_TARGET:
                    // unit target may be no longer avalible - teleported out of map for example
                    if (Unit* target = Unit::GetUnit(*m_caster, m_originalCaster->GetCurrentSpell(CURRENT_CHANNELED_SPELL)->m_targets.getUnitTargetGUID()))
                        AddUnitTarget(target, i);
                    else
                        sLog->outError("SPELL: cannot find channel spell target for spell ID %u", m_spellInfo->Id);
                    break;
                case TARGET_DEST_CHANNEL_TARGET:
                    if (m_originalCaster->GetCurrentSpell(CURRENT_CHANNELED_SPELL)->m_targets.HasDst())
                        m_targets.setDst(m_originalCaster->GetCurrentSpell(CURRENT_CHANNELED_SPELL)->m_targets);
                    else if (Unit* target = Unit::GetUnit(*m_caster, m_originalCaster->GetCurrentSpell(CURRENT_CHANNELED_SPELL)->m_targets.getUnitTargetGUID()))
                        m_targets.setDst(*target);
                    else
                        sLog->outError("SPELL: cannot find channel spell destination for spell ID %u", m_spellInfo->Id);
                    break;
                case TARGET_DEST_CHANNEL_CASTER:
                    m_targets.setDst(*m_originalCaster->GetCurrentSpell(CURRENT_CHANNELED_SPELL)->GetCaster());
                    break;
            }
            break;
        }

        default:
        {
            switch (cur)
            {
                case TARGET_GAMEOBJECT:
                    if (m_targets.getGOTarget())
                        AddGOTarget(m_targets.getGOTarget(), i);
                    break;
                case TARGET_GAMEOBJECT_ITEM:
                    if (m_targets.getGOTargetGUID())
                        AddGOTarget(m_targets.getGOTarget(), i);
                    else if (m_targets.getItemTarget())
                        AddItemTarget(m_targets.getItemTarget(), i);
                    break;
                case TARGET_UNIT_DRIVER:
                    if (Unit * driver = m_targets.getUnitTarget())
                        if (driver->IsOnVehicle(m_caster))
                            AddUnitTarget(driver, i);
                    break;
                default:
                    sLog->outError("Unhandled spell target %u", cur);
                    break;
            }
            break;
        }
    }

    if (pushType == PUSH_CHAIN) // Chain
    {
        Unit *target = m_targets.getUnitTarget();
        if (!target)
        {
            sLog->outError("SPELL: no chain unit target for spell ID %u", m_spellInfo->Id);
            return;
        }

        //Chain: 2, 6, 22, 25, 45, 77
        uint32 maxTargets = m_spellInfo->EffectChainTarget[i];
        if (modOwner)
            modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_JUMP_TARGETS, maxTargets, this);

        if (maxTargets > 1)
        {
            //otherwise, this multiplier is used for something else
            m_damageMultipliers[i] = 1.0f;
            m_applyMultiplierMask |= 1 << i;

            float range;
            std::list<Unit*> unitList;

            switch (cur)
            {
                case TARGET_UNIT_NEARBY_ENEMY:
                case TARGET_UNIT_TARGET_ENEMY:
                case TARGET_UNIT_NEARBY_ENTRY: // fix me
                    range = GetSpellMaxRange(m_spellInfo, false);
                    if (modOwner) modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_RANGE, range, this);
                    SearchChainTarget(unitList, range, maxTargets, SPELL_TARGETS_ENEMY);
                    break;
                case TARGET_UNIT_CHAINHEAL:
                case TARGET_UNIT_NEARBY_ALLY:  // fix me
                case TARGET_UNIT_NEARBY_ALLY_UNK:
                case TARGET_UNIT_NEARBY_RAID:
                    range = GetSpellMaxRange(m_spellInfo, true);
                    if (modOwner) modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_RANGE, range, this);
                    SearchChainTarget(unitList, range, maxTargets, SPELL_TARGETS_CHAINHEAL);
                    break;
            }

            CallScriptAfterUnitTargetSelectHandlers(unitList, SpellEffIndex(i));
            
            for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end(); ++itr)
                AddUnitTarget(*itr, i);
        }
        else
            AddUnitTarget(target, i);
    }
    else if (pushType)
    {
        // Dummy, just for client, except Have Group, Will Travel spell
        if (EffectTargetType[m_spellInfo->Effect[i]] != SPELL_REQUIRE_UNIT && m_spellInfo->Id != 83967)
            return;

        float radius;
        SpellTargets targetType;
        switch(cur)
        {
            case TARGET_UNIT_AREA_ENEMY_SRC:
            case TARGET_UNIT_AREA_ENEMY_DST:
            case TARGET_UNIT_CONE_ENEMY:
            case TARGET_UNIT_CONE_ENEMY_UNKNOWN:
            case TARGET_UNIT_AREA_PATH:
                if (m_spellInfo->Id == 2643      // Multi-Shot
                    || m_spellInfo->Id == 81297  // Consecration
                    || m_spellInfo->Id == 779    // Swipe  (Bear form)
                    || m_spellInfo->Id == 77758  // Thrash (Bear form)
                    || m_spellInfo->Id == 76524  // Grievous Whirl (Corla's Zealots)
                    || m_spellInfo->Id == 93658) // Grievous Whirl heroic version
                {
                    radius = 8; // 8yd
                    targetType = SPELL_TARGETS_ENEMY;
                    break;
                }
                if (m_spellInfo->Id == 122) // Frost Nova
                {
                    radius = 12; // 12yd
                    targetType = SPELL_TARGETS_ENEMY;
                    break;
                }
                if (m_spellInfo->Id == 89000 || m_spellInfo->Id == 95177) // Argaloth Fel Flames
                {
                    radius = 1.7f;
                    targetType = SPELL_TARGETS_ENEMY;
                    break;
                }
                if (m_spellInfo->Id == 86133 // Tornado (conclave: rohash)
                    || m_spellInfo->Id == 93141
                    || m_spellInfo->Id == 93142
                    || m_spellInfo->Id == 93143)
                {
                    radius = 4.0f;
                    targetType = SPELL_TARGETS_ENEMY;
                    break;
                }
                if (m_spellInfo->Id == 86367 // Sleet Storm (conclave: nezir)
                    || m_spellInfo->Id == 93135
                    || m_spellInfo->Id == 93136
                    || m_spellInfo->Id == 93137
                    || m_spellInfo->Id == 86487 // Hurricane (conclave: rohash)
                    || m_spellInfo->Id == 93144
                    || m_spellInfo->Id == 93145
                    || m_spellInfo->Id == 93146
                    || m_spellInfo->Id == 93060 // Storm Shield (conclave: rohash) damage spells
                    || m_spellInfo->Id == 95866
                    || m_spellInfo->Id == 87770 // Wind Burst (al'akir)
                    || m_spellInfo->Id == 93261
                    || m_spellInfo->Id == 93262
                    || m_spellInfo->Id == 93263
                    || m_spellInfo->Id == 88301 // Acid Rain (al'akir) DoT
                    || m_spellInfo->Id == 93279
                    || m_spellInfo->Id == 93280
                    || m_spellInfo->Id == 93281)
                {
                    radius = 90.0f;
                    targetType = SPELL_TARGETS_ENEMY;
                    break;
                }
                if (m_spellInfo->Id == 87873) // Static Shock (al'akir)
                {
                    radius = 45.0f;
                    targetType = SPELL_TARGETS_ENEMY;
                    break;
                }
                if (m_spellInfo->Id == 86111 // Ice Patch AOE (conclave: nezir)
                    || m_spellInfo->Id == 93129
                    || m_spellInfo->Id == 93130
                    || m_spellInfo->Id == 93131
                    || m_spellInfo->Id == 86282 // Toxic Spores (conclave: anshal: ravenous creeper)
                    || m_spellInfo->Id == 93120
                    || m_spellInfo->Id == 93121
                    || m_spellInfo->Id == 93122
                    || m_spellInfo->Id == 87908 // Stormling debuff (al'akir)
                    || m_spellInfo->Id == 93270
                    || m_spellInfo->Id == 93271
                    || m_spellInfo->Id == 93272)
                {
                    radius = 10.0f;
                    targetType = SPELL_TARGETS_ENEMY;
                    break;
                }
                if (m_spellInfo->Id == 89588 // Lightning Clouds damage (al'akir)
                    || m_spellInfo->Id == 93297
                    || m_spellInfo->Id == 93298
                    || m_spellInfo->Id == 93299)
                {
                    radius = 20.0f;
                    targetType = SPELL_TARGETS_ENEMY;
                    break;
                }
                if (m_spellInfo->Id == 78777) // Wild Mushroom: Detonate
                {
                    radius = 6.0f;
                    targetType = SPELL_TARGETS_ENEMY;
                    break;
                }
                if (m_spellInfo->Id == 88824) // Sound Bar
                {
                    radius = 100.0f;
                    targetType = SPELL_TARGETS_ENEMY;
                    break;
                }
                if (m_spellInfo->Id == 93041 // Absolute Zero
                    || m_spellInfo->Id == 78201
                    || m_spellInfo->Id == 78206
                    || m_spellInfo->Id == 78208
                    || m_spellInfo->Id == 93042
                    || m_spellInfo->Id == 93043)
                {
                    radius = 5.0f;
                    targetType = SPELL_TARGETS_ENEMY;
                    break;
                }
                if (m_spellInfo->Id == 77615 // Debilitating Slime
                    || m_spellInfo->Id == 77602
                    || m_spellInfo->Id == 77948
                    || m_spellInfo->Id == 92910)
                {
                    radius = 80.0f;
                    targetType = SPELL_TARGETS_ANY;
                    break;
                }
                if (m_spellInfo->Id == 77908 // Arcane Storm
                    || m_spellInfo->Id == 57473
                    || m_spellInfo->Id == 77908
                    || m_spellInfo->Id == 89160
                    || m_spellInfo->Id == 89161
                    || m_spellInfo->Id == 92961
                    || m_spellInfo->Id == 92962
                    || m_spellInfo->Id == 92963
                    || m_spellInfo->Id == 78225 // Acid Nova
                    || m_spellInfo->Id == 93013)
                {
                    radius = 80.0f;
                    targetType = SPELL_TARGETS_ENEMY;
                    break;
                }
                if (m_spellInfo->Id == 88858
                    || m_spellInfo->Id == 93286
                    || m_spellInfo->Id == 93287
                    || m_spellInfo->Id == 93288)
                {
                    radius = 200.0f;
                    targetType = SPELL_TARGETS_ENEMY;
                    break;
                }
                if (m_spellInfo->Id == 83619) // Fire Power (Mage: Flame Orb explosion)
                {
                    radius = 10.0f;
                    targetType = SPELL_TARGETS_ENEMY;
                    break;
                }
                // Massive Seaforium Charge - Seaforium Blast
                if (m_spellInfo->Id == 52408)
                {
                    radius = GetEffectRadius(i);
                    targetType = SPELL_TARGETS_ANY;
                    break;
                }
                radius = GetEffectRadius(i);
                targetType = SPELL_TARGETS_ENEMY;
                break;
            case TARGET_UNIT_AREA_ALLY_SRC:
            case TARGET_UNIT_AREA_ALLY_DST:
            case TARGET_UNIT_CONE_ALLY:
                if (m_spellInfo->Id == 84651 // Zephyr (conclave: anshal)
                    || m_spellInfo->Id == 93117
                    || m_spellInfo->Id == 93118
                    || m_spellInfo->Id == 93119)
                {
                    radius = 90.0f;
                    targetType = SPELL_TARGETS_ALLY;
                    break;
                }
                if (m_spellInfo->Id == 86452  // Holy Radiance
                    || m_spellInfo->Id == 89667 // Lightning Rod damage (Al'akir)
                    || m_spellInfo->Id == 93293
                    || m_spellInfo->Id == 93294
                    || m_spellInfo->Id == 93295)
                {
                    radius = 10.0f;
                    targetType = SPELL_TARGETS_ALLY;
                    break;
                }
                if (m_spellInfo->Id == 94472) // Atonement
                {
                    targetType = SPELL_TARGETS_ALLY;

                    if (Unit * target = m_targets.getUnitTarget())
                        radius = target->GetCombatReach() + 15.0f; // Increase radius by enemy combat reach
                    else
                        radius = GetEffectRadius(i);
                    break;
                }
                radius = GetEffectRadius(i);
                targetType = SPELL_TARGETS_ALLY;
                break;
            case TARGET_UNIT_AREA_ENTRY_DST:
            case TARGET_UNIT_AREA_ENTRY_SRC:
            {
                if (m_spellInfo->AttributesEx3 & SPELL_ATTR3_PLAYERS_ONLY)
                {
                    radius = GetEffectRadius(i);
                    targetType = SPELL_TARGETS_ANY;
                    break;
                }
            }
            case TARGET_UNIT_CONE_ENTRY: // fix me
                if (m_spellInfo->Id == 91858 // Overcharged Power Generator
                    || m_spellInfo->Id == 79629 // Power Generator
                    || m_spellInfo->Id == 91555 // (+ difficulty entries)
                    || m_spellInfo->Id == 91556
                    || m_spellInfo->Id == 91557)
                {
                    // AI purposes
                    radius = 12.0f;
                    targetType = SPELL_TARGETS_ANY;
                    break;
                }
                radius = GetEffectRadius(i);
                targetType = SPELL_TARGETS_ENTRY;
                break;
            case TARGET_GAMEOBJECT_AREA_SRC:
            case TARGET_GAMEOBJECT_AREA_DST:
            case TARGET_GAMEOBJECT_AREA_PATH:
                radius = GetEffectRadius(i);
                targetType = SPELL_TARGETS_GO;
                break;
            default:
                radius = GetEffectRadius(i);
                targetType = SPELL_TARGETS_NONE;
                break;
        }

        radius *= m_spellValue->RadiusMod;

        std::list<Unit*> unitList;
        std::list<GameObject*> gobjectList;
        switch (targetType)
        {
            case SPELL_TARGETS_ENTRY:
            {
                ConditionList conditions = sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_SPELL_SCRIPT_TARGET, m_spellInfo->Id);
                if (!conditions.empty())
                {
                    for (ConditionList::const_iterator i_spellST = conditions.begin(); i_spellST != conditions.end(); ++i_spellST)
                    {
                        if ((*i_spellST)->mConditionType != CONDITION_SPELL_SCRIPT_TARGET)
                            continue;
                        if ((*i_spellST)->mConditionValue3 && !((*i_spellST)->mConditionValue3 & (1<<i)))
                            continue;
                        if ((*i_spellST)->mConditionValue1 == SPELL_TARGET_TYPE_CREATURE)
                            SearchAreaTarget(unitList, radius, pushType, SPELL_TARGETS_ENTRY, (*i_spellST)->mConditionValue2);
                        else if ((*i_spellST)->mConditionValue1 == SPELL_TARGET_TYPE_CONTROLLED)
                        {
                            for (Unit::ControlList::iterator itr = m_caster->m_Controlled.begin(); itr != m_caster->m_Controlled.end(); ++itr)
                                if ((*itr)->GetEntry() == (*i_spellST)->mConditionValue2 &&
                                    /*(*itr)->IsWithinDistInMap(m_caster, radius)*/ (*itr)->IsInMap(m_caster)) // For 60243 and 52173 need skip radius check or use range (no radius entry for effect)
                                    unitList.push_back(*itr);
                        }
                    }
                }
                else
                {
                    // Custom entries
                    // TODO: move these to sql
                    switch (m_spellInfo->Id)
                    {
                        case 46584: // Raise Dead
                        {
                            if (WorldObject* result = FindCorpseUsing<Trinity::RaiseDeadObjectCheck> ())
                            {
                                switch(result->GetTypeId())
                                {
                                    case TYPEID_UNIT:
                                        m_targets.setDst(*result);
                                        break;
                                    default:
                                        break;
                                }
                            }
                            break;
                        }
                        // Corpse Explosion
                        case 49158:
                        case 51325:
                        case 51326:
                        case 51327:
                        case 51328:
                            // Search for ghoul if our ghoul or dead body not valid unit target
                            if (!(m_targets.getUnitTarget() && ((m_targets.getUnitTarget()->GetEntry() == 26125 && m_targets.getUnitTarget()->GetOwnerGUID() == m_caster->GetGUID())
                                || (m_targets.getUnitTarget()->getDeathState() == CORPSE
                                    && m_targets.getUnitTarget()->GetDisplayId() == m_targets.getUnitTarget()->GetNativeDisplayId()
                                    && m_targets.getUnitTarget()->GetTypeId() == TYPEID_UNIT
                                    && !m_targets.getUnitTarget()->ToCreature()->isDeadByDefault()
                                    && !(m_targets.getUnitTarget()->GetCreatureTypeMask() & CREATURE_TYPEMASK_MECHANICAL_OR_ELEMENTAL)
                                    && m_targets.getUnitTarget()->GetDisplayId() == m_targets.getUnitTarget()->GetNativeDisplayId()))))
                            {
                                CleanupTargetList();

                                WorldObject* result = FindCorpseUsing <Trinity::ExplodeCorpseObjectCheck> ();

                                if (result)
                                {
                                    switch (result->GetTypeId())
                                    {
                                        case TYPEID_UNIT:
                                        case TYPEID_PLAYER:
                                            m_targets.setUnitTarget((Unit*)result);
                                            break;
                                        default:
                                            break;
                                    }
                                }
                                else
                                {
                                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                                        m_caster->ToPlayer()->RemoveSpellCooldown(m_spellInfo->Id,true);
                                    SendCastResult(SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW);
                                    finish(false);
                                }
                            }
                            break;
                        case 97505: // Refreshing Stream
                            SearchAreaTarget(unitList, radius, pushType, SPELL_TARGETS_ANY);
                        break;
                        case 99083: // Meteor Burn (Beth'tilac) - teleport part
                            SearchAreaTarget(unitList, radius, pushType, SPELL_TARGETS_ENEMY);
                        break;

                        default:
                            sLog->outDebug("Spell (ID: %u) (caster Entry: %u) does not have type CONDITION_SOURCE_TYPE_SPELL_SCRIPT_TARGET record in `conditions` table.", m_spellInfo->Id, m_caster->GetEntry());

                            if (m_spellInfo->Effect[i] == SPELL_EFFECT_TELEPORT_UNITS)
                                SearchAreaTarget(unitList, radius, pushType, SPELL_TARGETS_ENTRY, 0);
                            else if (IsPositiveEffect(m_spellInfo->Id, i))
                                SearchAreaTarget(unitList, radius, pushType, SPELL_TARGETS_ALLY);
                            else
                                SearchAreaTarget(unitList, radius, pushType, SPELL_TARGETS_ENEMY);
                    }
                }
                break;
            }
            case SPELL_TARGETS_GO:
            {
                ConditionList conditions = sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_SPELL_SCRIPT_TARGET, m_spellInfo->Id);
                if (!conditions.empty())
                {
                    for (ConditionList::const_iterator i_spellST = conditions.begin(); i_spellST != conditions.end(); ++i_spellST)
                    {
                        if ((*i_spellST)->mConditionType != CONDITION_SPELL_SCRIPT_TARGET)
                            continue;
                        if ((*i_spellST)->mConditionValue3 && !((*i_spellST)->mConditionValue3 & (1<<i)))
                            continue;
                        if ((*i_spellST)->mConditionValue1 == SPELL_TARGET_TYPE_GAMEOBJECT)
                            SearchGOAreaTarget(gobjectList, radius, pushType, SPELL_TARGETS_GO, (*i_spellST)->mConditionValue2);
                    }
                }
                else
                {
                    if (m_spellInfo->Effect[i] == SPELL_EFFECT_ACTIVATE_OBJECT)
                        sLog->outDebug("Spell (ID: %u) (caster Entry: %u) with SPELL_EFFECT_ACTIVATE_OBJECT does not have type CONDITION_SOURCE_TYPE_SPELL_SCRIPT_TARGET record in `conditions` table.", m_spellInfo->Id, m_caster->GetEntry());
                    SearchGOAreaTarget(gobjectList, radius, pushType, SPELL_TARGETS_GO);
                }
                break;
            }
            case SPELL_TARGETS_ALLY:
            case SPELL_TARGETS_ENEMY:
            case SPELL_TARGETS_CHAINHEAL:
            case SPELL_TARGETS_ANY:
                // radius extension in case of players spells which are targetting enemy units
                SearchAreaTarget(unitList, radius, pushType, targetType, 0, (m_caster->GetTypeId() == TYPEID_PLAYER && targetType == SPELL_TARGETS_ENEMY));
                break;
            default:
                switch (cur)
                {
                    case TARGET_UNIT_AREA_PARTY_SRC:
                    case TARGET_UNIT_AREA_PARTY_DST:
                        m_caster->GetPartyMemberInDist(unitList, radius); //fix me
                        break;
                    case TARGET_UNIT_AREA_PARTY_SRC_2:
                        m_caster->GetRaidMemberDead(unitList, radius); // dead party and raid members
                        break;
                    case TARGET_UNIT_PARTY_TARGET:
                        m_targets.getUnitTarget()->GetPartyMemberInDist(unitList, radius);
                        break;
                    case TARGET_UNIT_PARTY_CASTER:
                        m_caster->GetPartyMemberInDist(unitList, radius);
                        break;
                    case TARGET_UNIT_RAID_CASTER:
                        // One huge exception - Have Group, Will Travel guild perk
                        if (m_spellInfo->Id == 83967 && m_caster->GetTypeId() == TYPEID_PLAYER)
                            unitList.push_back(m_caster);
                        else
                            m_caster->GetRaidMember(unitList, radius);

                        // Shield Fortress, Mass Regeneration, Mass Regeneration effect,
                        // Vampiric Brood T13 effect - exclude self
                        // TODO: generic fix based on SPELL_ATTR1_CANT_TARGET_SELF
                        if (m_spellInfo->Id == 105914 || m_spellInfo->Id == 105737 || m_spellInfo->Id == 105739 ||
                            m_spellInfo->Id == 105588)
                        {
                            for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end();)
                            {
                                if (*itr == m_caster)
                                    itr = unitList.erase(itr);
                                else
                                    ++itr;
                            }
                        }

                        // Mass Regeneration healing spell - exclude targets not having Mass Regeneration base spell
                        if (m_spellInfo->Id == 105739)
                        {
                            for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end();)
                            {
                                if (!(*itr)->HasAura(105737))
                                    itr = unitList.erase(itr);
                                else
                                    ++itr;
                            }
                        }

                        break;
                    case TARGET_UNIT_CLASS_TARGET:
                    {
                        Player* targetPlayer = m_targets.getUnitTarget() && m_targets.getUnitTarget()->GetTypeId() == TYPEID_PLAYER
                            ? (Player*)m_targets.getUnitTarget() : NULL;

                        Group* pGroup = targetPlayer ? targetPlayer->GetGroup() : NULL;
                        if (pGroup)
                        {
                            for (GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
                            {
                                Player* Target = itr->getSource();

                                // IsHostileTo check duel and controlled by enemy
                                if (Target && targetPlayer->IsWithinDistInMap(Target, radius) &&
                                    targetPlayer->getClass() == Target->getClass() &&
                                    !m_caster->IsHostileTo(Target))
                                {
                                    AddUnitTarget(Target, i);
                                }
                            }
                        }
                        else if (m_targets.getUnitTarget())
                            AddUnitTarget(m_targets.getUnitTarget(), i);
                        break;
                    }
                }
                break;
            }

            switch (m_spellInfo->SpellFamilyName)
            {
                case SPELLFAMILY_PALADIN:
                {
                    // Holy Radiance - include target in target list also (he is somehow ignored)
                    if (m_spellInfo->Id == 82327 || m_spellInfo->Id == 86452)
                        unitList.push_back(m_targets.getUnitTarget());
                    break;
                }
                case SPELLFAMILY_DRUID:
                {
                    // Stampeding Roar cat / bear form - include caster IF around is not any friendly target.
                    if (m_spellInfo->Id == 77764 || m_spellInfo->Id == 77761) {
                        if (unitList.empty())
                            unitList.push_back(m_caster);
                    }
                    break;
                }
            }

        if (!unitList.empty())
        {
            // Special target selection for smart heals and energizes
            uint32 maxSize = 0;
            int32 power = -1;
            switch (m_spellInfo->SpellFamilyName)
            {
                case SPELLFAMILY_GENERIC:
                    switch (m_spellInfo->Id)
                    {
                        case 26073: // Fire Nova (Quest spell)
                        {
                            for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end();)
                            {
                                if ((*itr)->GetTypeId() == TYPEID_UNIT && (*itr)->ToCreature()->GetEntry() == 46394)
                                    ++itr;
                                else
                                    itr = unitList.erase(itr);
                            }
                            break;
                        }
                        case 81008: // Quake
                        case 92631: // Quake (Heroic difficulty)
                        {
                            for (std::list<Unit*>::iterator itr = unitList.begin() ; itr != unitList.end();)
                            {
                                if ((*itr)->HasUnitMovementFlag(MOVEMENTFLAG_FALLING))
                                    itr = unitList.erase(itr);
                                else
                                    ++itr;
                            }
                            break;
                        }
                        case 101872: // Shoot - Darkmoon faire minigame spell
                        {
                            // choose the target, that is closest to the orientation of player
                            std::list<Unit*>::iterator itr = unitList.begin();
                            Unit* best = *itr;
                            itr++;
                            float casterOr = GetCaster()->GetOrientation();
                            for (; itr != unitList.end(); ++itr)
                            {
                                if (fabs(GetCaster()->GetAngle((*itr)->GetPositionX(), (*itr)->GetPositionY())-casterOr) < fabs(GetCaster()->GetAngle(best->GetPositionX(), best->GetPositionY())-casterOr))
                                    best = *itr;
                            }
                            unitList.clear();

                            // add best target only if it's in 30deg cone in front (15deg to every side)
                            if (fabs(GetCaster()->GetAngle(best->GetPositionX(), best->GetPositionY())-casterOr) < M_PI / 12.0f)
                            {
                                unitList.push_back(best);
                                m_targets.setUnitTarget(best);
                            }
                            break;
                        }
                        case 77679: // Scorching Blast + difficulty entries
                        case 92968:
                        case 92969:
                        case 92970:
                        {
                            for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end();)
                            {
                                if ((*itr)->GetTypeId()== TYPEID_UNIT && (*itr)->ToCreature()->IsPet())
                                    itr = unitList.erase(itr);
                                else
                                    ++itr;
                            }
                            break;
                        }
                        case 77569: // Release Aberration
                            for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end();)
                            {
                                if ((*itr)->GetTypeId()== TYPEID_UNIT && ((*itr)->ToCreature()->GetEntry() == 415055
                                    || (*itr)->ToCreature()->GetEntry() == 41378))
                                    ++itr;
                                else
                                    itr = unitList.erase(itr);
                            }
                            break;
                        case 92910: // Debilitating Slime
                            for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end();)
                           {
                                if ((*itr)->GetTypeId()== TYPEID_UNIT && ((*itr)->ToCreature()->GetEntry() == 415055 ||
                                    (*itr)->ToCreature()->GetEntry() == 415050 || (*itr)->ToCreature()->GetEntry() == 41505))
                                    itr = unitList.erase(itr);
                                else
                                    ++itr;
                            }
                            break;
                        case 43657: // Electrical Storm
                        case 97300:
                            for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end();)
                            {
                                if ((*itr)->GetDistance(m_caster) < 20.0f)
                                    itr = unitList.erase(itr);
                                else
                                    ++itr;
                            }
                            break;
                        case 98493: // Heated Volcano
                            unitList.resize(1);
                            break;
                        case 52759: // Ancestral Awakening
                        case 71610: // Echoes of Light (Althor's Abacus normal version)
                        case 71641: // Echoes of Light (Althor's Abacus heroic version)
                        case 96966: // Blaze of Life ( normal trinket )
                        case 97136: // Blaze of Life ( heroic trinket )
                        case 99152: // Cauterizing Flame
                        case 54968: // Divine Flame 
                            maxSize = 1;
                            power = POWER_HEALTH;
                            break;
                        case 57669: // Replenishment
                            // In arenas Replenishment may only affect the caster
                            if (m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->ToPlayer()->InArena())
                            {
                                unitList.clear();
                                unitList.push_back(m_caster);
                                break;
                            }
                            maxSize = 10;
                            power = POWER_MANA;
                            break;
                        case 88824: // Sound Bar
                        {
                            // Exclude targets with Sound Bar aura already applied
                            for (std::list<Unit*>::iterator itr = unitList.begin() ; itr != unitList.end();)
                            {
                                if ((*itr)->HasAura(88824))
                                    itr = unitList.erase(itr);
                                else
                                    ++itr;
                            }
                            break;
                        }
                        case 58836: // Initialize Images (Mirror Image)
                        {
                            if (!m_caster || m_caster->GetTypeId() != TYPEID_PLAYER)
                            {
                                unitList.clear();
                                break;
                            }

                            // Initialize Images spell should be applied only on summoned images
                            for (std::list<Unit*>::iterator itr = unitList.begin() ; itr != unitList.end();)
                            {
                                if ((*itr)->GetTypeId() != TYPEID_UNIT || (*itr)->GetEntry() != 31216  || (*itr)->GetCharmerOrOwnerGUID() != m_caster->GetGUID())
                                    itr = unitList.erase(itr);
                                else
                                    ++itr;
                            }
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                case SPELLFAMILY_PRIEST:
                    if (m_spellInfo->SpellFamilyFlags[0] == 0x10000000) // Circle of Healing
                    {
                        maxSize = m_caster->HasAura(55675) ? 6 : 5; // Glyph of Circle of Healing
                        power = POWER_HEALTH;
                    }
                    else if (m_spellInfo->Id == 64844) // Divine Hymn
                    {
                        maxSize = 5;
                        power = POWER_HEALTH;
                    }
                    else if (m_spellInfo->Id == 64904) // Hymn of Hope
                    {
                        maxSize = 3;
                        power = POWER_MANA;
                    }
                    else if (m_spellInfo->Id == 81751 || m_spellInfo->Id == 94472) // Atonement
                    {
                        maxSize = 1;
                        power = POWER_HEALTH;
                    }
                    else
                        break;

                    // Remove targets outside caster's raid
                    for (std::list<Unit*>::iterator itr = unitList.begin() ; itr != unitList.end();)
                    {
                        if (!(*itr)->IsInRaidWith(m_caster))
                            itr = unitList.erase(itr);
                        else
                            ++itr;
                    }
                    break;
                case SPELLFAMILY_DRUID:
                    if (m_spellInfo->SpellFamilyFlags[1] == 0x04000000) // Wild Growth
                    {
                        maxSize = m_caster->HasAura(62970) ? 6 : 5; // Glyph of Wild Growth

                        if (m_caster->HasAura(33891)) // Tree of Life boost
                            maxSize += 2;

                        power = POWER_HEALTH;
                    }
                    else if (m_spellInfo->SpellFamilyFlags[2] == 0x0100) // Starfall
                    {
                        // Remove targets not in LoS or in stealth
                        for (std::list<Unit*>::iterator itr = unitList.begin() ; itr != unitList.end();)
                        {
                            if ((*itr)->HasStealthAura() || (*itr)->HasInvisibilityAura() || !(*itr)->IsWithinLOSInMap(m_caster)
                               || ((*itr)->GetCreatureType() == CREATURE_TYPE_CRITTER ) || !(*itr)->IsInCombat() || (*itr)->IsFullHealth())
                                itr = unitList.erase(itr);
                            else
                                ++itr;
                        }
                        break;
                    }
                    // Stampeding Roar (bear form, cat form) - add speed bonus to caster also
                    else if (m_spellInfo->Id == 77761 || m_spellInfo->Id == 77764)
                    {
                        unitList.push_back(m_caster);
                        break;
                    }
                    // Efflorescence
                    else if (m_spellInfo->Id == 81269)
                    {
                        maxSize = 3;
                        power = POWER_HEALTH;
                        break;
                    }
                    // Firebloom
                    else if (m_spellInfo->Id == 99017)
                    {
                        maxSize = 1;
                        power = POWER_HEALTH;
                        break;
                    }
                    else
                        break;

                    // Remove targets outside caster's raid
                    for (std::list<Unit*>::iterator itr = unitList.begin() ; itr != unitList.end();)
                    {
                        if (!(*itr)->IsInRaidWith(m_caster))
                            itr = unitList.erase(itr);
                        else
                            ++itr;
                    }
                    break;
                case SPELLFAMILY_PALADIN:
                    // Holy Wrath - affect only demons and undeads (with glyph also dragonkins and elementals)
                    if (m_spellInfo->Id == 2812)
                    {
                        // Modify unit target list only for stun effect, damage should be dealt to anyone
                        if (m_spellInfo->EffectApplyAuraName[i] != SPELL_AURA_MOD_STUN)
                            break;

                        // Iterate through target list and exclude specific
                        for (std::list<Unit*>::iterator itr = unitList.begin() ; itr != unitList.end();)
                        {
                            if ((*itr)->GetTypeId() == TYPEID_PLAYER)
                                itr = unitList.erase(itr);
                            else if ((*itr)->GetTypeId() == TYPEID_UNIT)
                            {
                                switch ((*itr)->ToCreature()->GetCreatureType())
                                {
                                    // Demons and undeads should be affected always
                                    case CREATURE_TYPE_DEMON:
                                    case CREATURE_TYPE_UNDEAD:
                                        ++itr;
                                        continue;
                                    // Dragonkins and elementals only if player has Glyph of Holy Wrath
                                    case CREATURE_TYPE_DRAGONKIN:
                                    case CREATURE_TYPE_ELEMENTAL:
                                        if (m_caster->HasAura(56420))
                                        {
                                            ++itr;
                                            continue;
                                        }
                                        else
                                        {
                                            itr = unitList.erase(itr);
                                            continue;
                                        }
                                    default:
                                        itr = unitList.erase(itr);
                                        continue;
                                }
                            }
                            else
                                ++itr;
                        }
                    }
                    if (m_spellInfo->Id == 85222) // Light of Dawn
                    {
                        // Exclude targets which aren't in party with caster
                        // and limit count to 5 (the caster is healed by separate effect making 6 targets total)

                        maxSize = 5;
                        if (m_caster->HasAura(54940))
                            maxSize -= 2;

                        power = POWER_HEALTH;

                        for (std::list<Unit*>::iterator itr = unitList.begin() ; itr != unitList.end();)
                        {
                            Player *target = (*itr)->ToPlayer();
                            if (!target || target == m_caster || !(target->IsInPartyWith(m_caster) || target->IsInRaidWith(m_caster)))
                                itr = unitList.erase(itr);
                            else
                                ++itr;
                        }
                        break;
                    }
                    break;
                case SPELLFAMILY_SHAMAN:
                    // Spirit Link targetting and secondary effect
                    // we have to do it this way, cause the dummy raid aura select targets for us, which have to be used for calculation
                    if (m_spellInfo->Id == 98020)
                    {
                        uint32 curHealthSum = 0, maxHealthSum = 0;
                        for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end(); ++itr)
                        {
                            Unit *unit = *itr;
                            curHealthSum += unit->GetHealth();
                            maxHealthSum += unit->GetMaxHealth();
                        }

                        float avgHealthPct = 100.0f * float(curHealthSum) / maxHealthSum;

                        int32 bp = 0;
                        for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end(); ++itr)
                        {
                            Unit *target = *itr;
                            float curHealthPct = target->GetHealthPct();

                            // effect 0 = damage
                            if (curHealthPct > avgHealthPct)
                            {
                                bp = (curHealthPct - avgHealthPct) * target->GetMaxHealth() / 100.0f;
                                m_caster->CastCustomSpell(target, 98021, &bp, 0, 0, true);
                            }
                            // effect 1 = heal
                            else
                            {
                                bp = (avgHealthPct - curHealthPct) * target->GetMaxHealth() / 100.0f;
                                m_caster->CastCustomSpell(target, 98021, 0, &bp, 0, true);
                            }
                        }
                    }
                    break;
                case SPELLFAMILY_ROGUE:
                    // Fan of Knives
                    if (m_spellInfo->Id == 51723)
                    {
                        float roll = 100.0f;
                        if (m_caster->HasAura(16513))
                            roll = 33.0f;
                        else if (m_caster->HasAura(16514))
                            roll = 67.0f;

                        roll /= 2; // Let say, that all posions have 50 % chance to proc :)

                        // Vile Poisons - second part of talent
                        if (m_caster->HasAura(16513) || m_caster->HasAura(16514) || m_caster->HasAura(16515))
                        {
                            for (uint32 i = EQUIPMENT_SLOT_MAINHAND; i < EQUIPMENT_SLOT_RANGED;i++)
                            {
                                // Get weapon
                                Item* item = m_caster->ToPlayer()->GetItemByPos(INVENTORY_SLOT_BAG_0, i);

                                if (item)
                                {
                                    // find enchant
                                    SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(item->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT));

                                    if (enchantEntry && enchantEntry->spellid[0])
                                    {
                                        if (SpellEntry const* poison = sSpellStore.LookupEntry(enchantEntry->spellid[0]))
                                        {
                                            // find poisons
                                            if (poison && poison->Dispel == DISPEL_POISON && roll_chance_f(roll))
                                            {
                                                // apply to all targets in radius
                                                for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end(); ++itr)
                                                {
                                                    if ((*itr))
                                                    {
                                                        m_caster->CastSpell((*itr), poison->Id, true);
                                                        itr = unitList.erase(itr);
                                                    }
                                                }
                                            }
                                        }
                                     }
                                }
                            }
                        }
                    }
                    break;
                default:
                    break;
            }

            if (maxSize && power != -1)
            {
                if (Powers(power) == POWER_HEALTH)
                {
                    if (unitList.size() > maxSize)
                    {
                        unitList.sort(Trinity::HealthPctOrderPred());
                        unitList.resize(maxSize);
                    }
                }
                else
                {
                    for (std::list<Unit*>::iterator itr = unitList.begin() ; itr != unitList.end();)
                    {
                        if ((*itr)->getPowerType() != (Powers)power)
                            itr = unitList.erase(itr);
                        else
                            ++itr;
                    }
                    if (unitList.size() > maxSize)
                    {
                        unitList.sort(Trinity::PowerPctOrderPred((Powers)power));
                        unitList.resize(maxSize);
                    }
                }
            }

            // Other special target selection goes here
            if (uint32 maxTargets = m_spellValue->MaxAffectedTargets)
            {
                Unit::AuraEffectList const& Auras = m_caster->GetAuraEffectsByType(SPELL_AURA_MOD_MAX_AFFECTED_TARGETS);
                for (Unit::AuraEffectList::const_iterator j = Auras.begin(); j != Auras.end(); ++j)
                    if ((*j)->IsAffectedOnSpell(m_spellInfo))
                        maxTargets += (*j)->GetAmount();

                Trinity::RandomResizeList(unitList, maxTargets);

                // if the target list can contain only 1 target and is selected from many (i.e. TARGET_UNIT_CONE_ENTRY), then we probably search for only one target)
                // may be wrong for several spells
                if (maxTargets == 1 && unitList.size() == 1)
                {
                    m_targets.setUnitTarget(*(unitList.begin()));
                }

                switch (m_spellInfo->Id)
                {
                    case 85547:
                    case 86105:
                    // Jinx: Curse of the Elements
                    for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end();)
                    {
                        if ((*itr)->HasAuraTypeWithValue(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN,8))
                            itr = unitList.erase(itr);
                        else
                            ++itr;
                    }
                    break;
                }
            }
            else
            {
                switch (m_spellInfo->Id)
                {
                    case 27285: // Seed of Corruption proc spell
                    case 49821: // Mind Sear proc spell Rank 1
                    case 53022: // Mind Sear proc spell Rank 2
                        unitList.remove(m_targets.getUnitTarget());
                        break;
                    case 55789: // Improved Icy Talons
                    case 59725: // Improved Spell Reflection - aoe aura
                    case 97016: // Big Bad Voodoo
                        unitList.remove(m_caster);
                        break;
                    case 72255: // Mark of the Fallen Champion (Deathbringer Saurfang)
                    case 72444:
                    case 72445:
                    case 72446:
                        for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end();)
                        {
                            if (!(*itr)->HasAura(72293))
                                itr = unitList.erase(itr);
                            else
                                ++itr;
                        }
                        break;
                    case 69782: case 69796:                 // Ooze Flood
                    case 69798: case 69801:                 // Ooze Flood
                        // get 2 targets except 2 nearest
                        unitList.sort(Trinity::ObjectDistanceOrderPred(m_caster));
                        unitList.resize(4);
                        while (unitList.size() > 2)
                            unitList.pop_front();
                        // crashfix
                        if (unitList.empty())
                            return;
                        break;
                    case 68921: case 69049:                 // Soulstorm
                        for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end();)
                        {
                            Position pos;
                            (*itr)->GetPosition(&pos);
                            if (m_caster->GetExactDist2d(&pos) <= 10.0f)
                                itr = unitList.erase(itr);
                            else
                                ++itr;
                        }
                        break;
                }
                // Death Pact
                if (m_spellInfo->SpellFamilyName == SPELLFAMILY_DEATHKNIGHT && m_spellInfo->SpellFamilyFlags[0] & 0x00080000 && m_caster->GetTypeId() == TYPEID_PLAYER)
                {
                    Unit * unit_to_add = NULL;

                    for (std::list<Unit*>::iterator itr = unitList.begin() ; itr != unitList.end(); ++itr)
                    {
                        if ((*itr)->GetTypeId() == TYPEID_UNIT
                            && (*itr)->GetOwnerGUID() == m_caster->GetGUID()
                            && (*itr)->ToCreature()->GetCreatureInfo()->type == CREATURE_TYPE_UNDEAD)
                        {
                            unit_to_add = (*itr);
                            break;
                        }
                    }
                    if (unit_to_add)
                    {
                        unitList.clear();
                        unitList.push_back(unit_to_add);
                    }
                    // Pet not found - remove cooldown
                    else
                    {
                        if (modOwner->GetTypeId() == TYPEID_PLAYER)
                            modOwner->RemoveSpellCooldown(m_spellInfo->Id,true);
                        SendCastResult(SPELL_FAILED_NO_PET);
                        finish(false);
                    }
                }
            }

            /*CallScriptAfterUnitTargetSelectHandlers(unitList, SpellEffIndex(i));

            for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end(); ++itr)
                AddUnitTarget(*itr, i);*/
        }

        // Call this script hook, even if no targets in list -> we can force push custom units in SpellScripts
        CallScriptAfterUnitTargetSelectHandlers(unitList, SpellEffIndex(i));

        for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end(); ++itr)
            AddUnitTarget(*itr, i);

        if (!gobjectList.empty())
        {
            if (uint32 maxTargets = m_spellValue->MaxAffectedTargets)
            {
                Unit::AuraEffectList const& Auras = m_caster->GetAuraEffectsByType(SPELL_AURA_MOD_MAX_AFFECTED_TARGETS);
                for (Unit::AuraEffectList::const_iterator j = Auras.begin(); j != Auras.end(); ++j)
                    if ((*j)->IsAffectedOnSpell(m_spellInfo))
                        maxTargets += (*j)->GetAmount();

                Trinity::RandomResizeList(gobjectList, maxTargets);
            }
            for (std::list<GameObject*>::iterator itr = gobjectList.begin(); itr != gobjectList.end(); ++itr)
                AddGOTarget(*itr, i);
        }
    }
}

void Spell::prepare(SpellCastTargets const* targets, AuraEffect const * triggeredByAura)
{
    if (m_CastItem)
        m_castItemGUID = m_CastItem->GetGUID();
    else
        m_castItemGUID = 0;

    m_targets = *targets;

    if (!m_targets.getUnitTargetGUID() && m_spellInfo->Targets & TARGET_FLAG_UNIT)
    {
        Unit *target = NULL;
        if (m_caster->GetTypeId() == TYPEID_UNIT)
            target = m_caster->GetVictim();
        else
            target = ObjectAccessor::GetUnit(*m_caster, m_caster->ToPlayer()->GetSelection());

        if (target && IsValidSingleTargetSpell(target))
            m_targets.setUnitTarget(target);
        else
        {
            SendCastResult(SPELL_FAILED_BAD_TARGETS);
            finish(false);
            return;
        }
    }
    if (Player* plrCaster = m_caster->GetCharmerOrOwnerPlayerOrPlayerItself())
    {
        //check for special spell conditions
        ConditionList conditions = sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_SPELL, m_spellInfo->Id);
        if (!conditions.empty())
        {
            if (!sConditionMgr->IsPlayerMeetToConditions(plrCaster, conditions))
            {
                //SendCastResult(SPELL_FAILED_DONT_REPORT);
                SendCastResult(plrCaster, m_spellInfo, m_cast_count, SPELL_FAILED_DONT_REPORT);
                finish(false);
                return;
            }
        }
    }
    if (!m_targets.HasSrc() && m_spellInfo->Targets & TARGET_FLAG_SOURCE_LOCATION)
        m_targets.setSrc(*m_caster);

    if (!m_targets.HasDst() && m_spellInfo->Targets & TARGET_FLAG_DEST_LOCATION)
    {
        Unit *target = m_targets.getUnitTarget();
        if (!target)
        {
            if (m_caster->GetTypeId() == TYPEID_UNIT)
                target = m_caster->GetVictim();
            else
                target = ObjectAccessor::GetUnit(*m_caster, m_caster->ToPlayer()->GetSelection());
        }

        if (target)
            m_targets.setDst(*target);
        else
        {
            SendCastResult(SPELL_FAILED_BAD_TARGETS);
            finish(false);
            return;
        }
    }

    // Fill aura scaling information
    if (m_caster->IsControlledByPlayer() && !IsPassiveSpell(m_spellInfo->Id) && m_spellInfo->spellLevel && !IsChanneledSpell(m_spellInfo) && !m_IsTriggeredSpell)
    {
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (m_spellInfo->Effect[i] == SPELL_EFFECT_APPLY_AURA)
            {
                // Change aura with ranks only if basepoints are taken from spellInfo and aura is positive
                if (IsPositiveEffect(m_spellInfo->Id, i))
                {
                    m_auraScaleMask |= (1<<i);
                    if (m_spellValue->EffectBasePoints[i] != m_spellInfo->EffectBasePoints[i])
                    {
                        m_auraScaleMask = 0;
                        break;
                    }
                }
            }
        }
    }

    m_spellState = SPELL_STATE_PREPARING;

    if (triggeredByAura)
        m_triggeredByAuraSpell  = triggeredByAura->GetSpellProto();

    // create and add update event for this spell
    SpellEvent* Event = new SpellEvent(this);
    m_caster->m_Events.AddEvent(Event, m_caster->m_Events.CalculateTime(1));

    // queue casting at the cast of another spell
    Player *player = m_caster->ToPlayer();
    if (!m_IsTriggeredSpell && m_caster->IsNonMeleeSpellCasted(false, true, true) && m_cast_count)
    {
        if (!player || player->HasQueuedSpell())
        {
            SendCastResult(SPELL_FAILED_SPELL_IN_PROGRESS);
            finish(false);
            return;
        }
        player->QueueSpell();
        m_spellState = SPELL_STATE_QUEUED;
        return;
    }

    // queue casting at global cooldown
    if (!m_IsTriggeredSpell && !IsAutoRepeat() && player && player->HasGlobalCooldown(m_spellInfo))
    {
        if (!player->HasQueuedSpell())
        {
            player->QueueSpell();
            m_spellState = SPELL_STATE_QUEUED;
            return;
        }
        SendCastResult(SPELL_FAILED_NOT_READY);
        finish(false);
        return;
    }

    prepareFinish(triggeredByAura);
}

void Spell::prepareFinish(AuraEffect const * triggeredByAura)
{
    m_spellState = SPELL_STATE_PREPARING;
    if (sDisableMgr->IsDisabledFor(DISABLE_TYPE_SPELL, m_spellInfo->Id, m_caster))
    {
        SendCastResult(SPELL_FAILED_SPELL_UNAVAILABLE);
        finish(false);
        return;
    }
    LoadScripts();

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
        m_caster->ToPlayer()->SetSpellModTakingSpell(this, true);
    // Fill cost data (not use power for item casts
    m_powerCost = m_CastItem ? 0 : CalculatePowerCost(m_spellInfo, m_caster, m_spellSchoolMask);
    if (m_caster->GetTypeId() == TYPEID_PLAYER)
        m_caster->ToPlayer()->SetSpellModTakingSpell(this, false);

    // Set combo point requirement
    if (m_IsTriggeredSpell || m_CastItem || !m_caster->m_movedPlayer)
        m_needComboPoints = false;

    SpellCastResult result = CheckCast(true);
    if (result != SPELL_CAST_OK && !IsAutoRepeat())          //always cast autorepeat dummy for triggering
    {
        if (triggeredByAura && !triggeredByAura->GetBase()->IsPassive())
        {
            SendChannelUpdate(0);
            triggeredByAura->GetBase()->SetDuration(0);
        }
        SendCastResult(result);

        finish(false);
        return;
    }

    // Prepare data for triggers
    prepareDataForTriggerSystem(triggeredByAura);

    // calculate cast time (calculated after first CheckCast check to prevent charge counting for first CheckCast fail)
    m_casttime = GetSpellCastTime(m_spellInfo, this);
    //m_caster->ModSpellCastTime(m_spellInfo, m_casttime, this);

    // set timer base at cast time
    ReSetTimer();

    //Containers for channeled spells have to be set
    //TODO:Apply this to all casted spells if needed
    // Why check duration? 29350: channelled triggers channelled
    if (m_IsTriggeredSpell && (!IsChanneledSpell(m_spellInfo) || !GetSpellMaxDuration(m_spellInfo)))
        cast(true);
    else
    {
        // don't allow channeled spells / spells with cast time to be casted while moving
        // (even if they are interrupted on moving, spells with almost immediate effect get to have their effect processed before movement interrupter kicks in)
        if ((IsChanneledSpell(m_spellInfo) || m_casttime)
            && m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->isMoving()
            && m_spellInfo->InterruptFlags & SPELL_INTERRUPT_FLAG_MOVEMENT
            && !m_caster->CanCastWhileWalking(m_spellInfo))
        {
            SendCastResult(SPELL_FAILED_MOVING);
            finish(false);
            return;
        }

        // stealth must be removed at cast starting (at show channel bar)
        // skip triggered spell (item equip spell casting and other not explicit character casts/item uses)
        if (!m_IsTriggeredSpell && isSpellBreakStealth(m_spellInfo))
        {
            #define CAMOUFLAGE_EXCEPTION_SPELLID 51755 // Hunter ability exception (should be removed after damage done)

            m_caster->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_CAST, CAMOUFLAGE_EXCEPTION_SPELLID);

            for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            {
                if (EffectTargetType[m_spellInfo->Effect[i]] == SPELL_REQUIRE_UNIT)
                {
                    m_caster->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_SPELL_ATTACK);
                    break;
                }
            }
        }

        m_caster->SetCurrentCastedSpell(this);
        SendSpellStart();

        if (m_caster->GetTypeId() == TYPEID_PLAYER)
            m_caster->ToPlayer()->AddGlobalCooldown(m_spellInfo,this);

        if (!m_casttime && !m_spellInfo->StartRecoveryTime
            && !m_castItemGUID     //item: first cast may destroy item and second cast causes crash
            && GetCurrentContainer() == CURRENT_GENERIC_SPELL)
            cast(true);
    }
}
// TODO: Remove this !
bool Spell::CanCancel()
{
    // HACK FIX
    // Channeled spells are interrupted sometimes cause moving ???
    // TODO: Find reason and fix it !
    switch (m_spellInfo->Id)
    {
        // Lord Rhyolith spells (this is not actually channeled but still interrupting sometimes for no reason)
        case 97282:
        case 100411:
        case 100968:
        case 100969:
        // Morchok spells
        case 103851:
        case 103821:
        case 103785:
        case 108570:
        case 110288:
        case 110287:
        // Warlord spell
        case 103946:
        // Peroth'arn
        case 104905:
        // Archbishop Benedictus
        case 103754:
        case 103565:
        case 103767:
        // Argaloth
        case 88972:
        // Hagara the Stormbinder
        case 105256:
        case 109552:
        case 109553:
        case 109554:
        case 105409:
        case 109560:
        case 109561:
        case 109562:
        // Warmaster Blackhorn
        case 108046:
        // Lord Hiram Creed
        case 109669:
            return false;
    }
    return true;
}

void Spell::cancel()
{
    if (m_spellState == SPELL_STATE_FINISHED)
        return;

    if (CanCancel() == false)
        return;

    SetReferencedFromCurrent(false);
    if (m_selfContainer && *m_selfContainer == this)
        *m_selfContainer = NULL;

    uint32 oldState = m_spellState;
    m_spellState = SPELL_STATE_FINISHED;

    m_autoRepeat = false;
    switch (oldState)
    {
        case SPELL_STATE_PREPARING:
            if (m_caster->GetTypeId() == TYPEID_PLAYER)
                m_caster->ToPlayer()->RestoreSpellMods(this);
        case SPELL_STATE_DELAYED:
            SendInterrupted(0);
            SendCastResult(SPELL_FAILED_INTERRUPTED);
            break;

        case SPELL_STATE_CASTING:
            for (std::list<TargetInfo>::const_iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                if ((*ihit).missCondition == SPELL_MISS_NONE)
                    if (Unit* unit = m_caster->GetGUID() == ihit->targetGUID ? m_caster : ObjectAccessor::GetUnit(*m_caster, ihit->targetGUID))
                        unit->RemoveOwnedAura(m_spellInfo->Id, m_originalCasterGUID, 0, AURA_REMOVE_BY_CANCEL);

            SendChannelUpdate(0);
            SendInterrupted(0);
            SendCastResult(SPELL_FAILED_INTERRUPTED);

            // spell is canceled-take mods and clear list
            if (m_caster->GetTypeId() == TYPEID_PLAYER)
                m_caster->ToPlayer()->RemoveSpellMods(this);

            m_appliedMods.clear();
            break;

        default:
            break;
    }

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
        m_caster->ToPlayer()->RemoveGlobalCooldown(m_spellInfo);

    // signal the AI of caster, if any
    if (m_caster->GetTypeId() == TYPEID_UNIT && m_caster->ToCreature()->IsAIEnabled)
        m_caster->ToCreature()->AI()->SpellCastFailed(m_spellInfo);

    m_caster->RemoveDynObject(m_spellInfo->Id);
    m_caster->RemoveGameObject(m_spellInfo->Id,true);

    //set state back so finish will be processed
    m_spellState = oldState;

    finish(false);
}

void Spell::cast(bool skipCheck)
{
    // update pointers base at GUIDs to prevent access to non-existed already object
    UpdatePointers();

    if (Unit *target = m_targets.getUnitTarget())
    {
        // three check: prepare, cast (m_casttime > 0), hit (delayed)
        if (m_casttime && target->IsAlive()
            && (target->m_invisibilityMask || m_caster->m_invisibilityMask
            || target->GetVisibility() == VISIBILITY_GROUP_STEALTH)
            && !target->IsFriendlyTo(m_caster) && !m_caster->canSeeOrDetect(target, true))
        {
            SendCastResult(SPELL_FAILED_BAD_TARGETS);
            SendInterrupted(0);
            finish(false);
            return;
        }
    }
    else
    {
        // cancel at lost main target unit
        if (m_targets.getUnitTargetGUID() && m_targets.getUnitTargetGUID() != m_caster->GetGUID())
        {
            cancel();
            return;
        }
    }

    // now that we've done the basic check, now run the scripts
    // should be done before the spell is actually executed
    if (Player *playerCaster = m_caster->ToPlayer())
        sScriptMgr->OnPlayerSpellCast(playerCaster, this, skipCheck);

    SetExecutedCurrently(true);

    if (m_caster->GetTypeId() != TYPEID_PLAYER && m_targets.getUnitTarget() && m_targets.getUnitTarget() != m_caster)
        m_caster->SetInFront(m_targets.getUnitTarget());

    // Should this be done for original caster?
    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        // Set spell which will drop charges for triggered cast spells
        // if not successfully casted, will be remove in finish(false)
        m_caster->ToPlayer()->SetSpellModTakingSpell(this, true);
    }

    // triggered cast called from Spell::prepare where it was already checked
    if (!m_IsTriggeredSpell || !skipCheck)
    {
        SpellCastResult castResult = CheckCast(false);
        if (castResult != SPELL_CAST_OK)
        {
            SendCastResult(castResult);
            SendInterrupted(0);
            //restore spell mods
            if (m_caster->GetTypeId() == TYPEID_PLAYER)
            {
                m_caster->ToPlayer()->RestoreSpellMods(this);
                // cleanup after mod system
                // triggered spell pointer can be not removed in some cases
                m_caster->ToPlayer()->SetSpellModTakingSpell(this, false);
            }
            finish(false);
            SetExecutedCurrently(false);
            return;
        }

        // additional check after cast bar completes (must not be in CheckCast)
        // if trade not complete then remember it in trade data
        if (m_targets.getTargetMask() & TARGET_FLAG_TRADE_ITEM)
        {
            if (m_caster->GetTypeId() == TYPEID_PLAYER)
            {
                if (TradeData* my_trade = m_caster->ToPlayer()->GetTradeData())
                {
                    if (!my_trade->IsInAcceptProcess())
                    {
                        // Spell will be casted at completing the trade. Silently ignore at this place
                        my_trade->SetSpell(m_spellInfo->Id, m_CastItem);
                        SendCastResult(SPELL_FAILED_DONT_REPORT);
                        SendInterrupted(0);
                        m_caster->ToPlayer()->RestoreSpellMods(this);
                        // cleanup after mod system
                        // triggered spell pointer can be not removed in some cases
                        m_caster->ToPlayer()->SetSpellModTakingSpell(this, false);
                        finish(false);
                        SetExecutedCurrently(false);
                        return;
                    }
                }
            }
        }
    }

    m_delay = m_spellInfo->speed > 0.0f && !IsChanneledSpell(m_spellInfo);
    SelectSpellTargets();

    // Spell may be finished after target map check
    if (m_spellState == SPELL_STATE_FINISHED)
    {
        SendInterrupted(0);
        //restore spell mods
        if (m_caster->GetTypeId() == TYPEID_PLAYER)
        {
            m_caster->ToPlayer()->RestoreSpellMods(this);
            // cleanup after mod system
            // triggered spell pointer can be not removed in some cases
            m_caster->ToPlayer()->SetSpellModTakingSpell(this, false);
        }
        finish(false);
        SetExecutedCurrently(false);
        return;
    }

    if (m_spellInfo->SpellFamilyName)
    {
        if (m_spellInfo->excludeCasterAuraSpell && !IsPositiveSpell(m_spellInfo->excludeCasterAuraSpell))
            m_preCastSpell = m_spellInfo->excludeCasterAuraSpell;
        else if (m_spellInfo->excludeTargetAuraSpell && !IsPositiveSpell(m_spellInfo->excludeTargetAuraSpell))
            m_preCastSpell = m_spellInfo->excludeTargetAuraSpell;
    }

    switch (m_spellInfo->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            // Bandages and Recently Bandaged debuff (strcmp only for being sure sice MECHANIC_BANDAGE was probably removed)
            if (m_spellInfo->PreventionType == SPELL_PREVENTION_TYPE_UNK1 && strcmp(m_spellInfo->SpellName,"First Aid") == 0) // Bandages
            {
                if (m_targets.getUnitTarget() && m_targets.getUnitTarget()->HasAura(11196))
                {
                    SendCastResult(SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW);
                    SendInterrupted(0);
                    finish(false);
                    SetExecutedCurrently(false);
                    return;
                }
                else
                    m_preCastSpell = 11196;                                // Recently Bandaged
            }
            break;
        }
        case SPELLFAMILY_MAGE:
        {
             // Permafrost
             if (m_spellInfo->SpellFamilyFlags[1] & 0x00001000 ||  m_spellInfo->SpellFamilyFlags[0] & 0x00100220)
              m_preCastSpell = 68391;
             break;
        }
    }

    // traded items have trade slot instead of guid in m_itemTargetGUID
    // set to real guid to be sent later to the client
    m_targets.updateTradeSlotItem();

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        if (!m_IsTriggeredSpell && m_CastItem)
        {
            m_caster->ToPlayer()->GetAchievementMgr().StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_ITEM, m_CastItem->GetEntry());
            m_caster->ToPlayer()->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM, m_CastItem->GetEntry());
        }

        m_caster->ToPlayer()->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL, m_spellInfo->Id);
    }

    if (!m_IsTriggeredSpell)
    {
        // Powers have to be taken before SendSpellGo
        TakePower();
        TakeReagents();                                         // we must remove reagents before HandleEffects to allow place crafted item in same slot

        if (m_caster && m_caster->GetTypeId() == TYPEID_PLAYER)
        {
            // Also add spellcast history to player class
            // TODO: add hit outcome (crit/miss/parry/..) to specialValue
            m_caster->ToPlayer()->AddNonTriggeredSpellcastHistory(GetSpellInfo(), 0);

            // Whenever we cast non-triggered negative direct-damaging spell, save the last target
            if (m_targets.getUnitTarget() && !IsPositiveSpell(GetSpellInfo()->Id) && (GetSpellInfo()->AttributesEx3 & SPELL_ATTR3_NO_INITIAL_AGGRO) == 0)
                m_caster->ToPlayer()->SetLastDirectAttackTarget(m_targets.getUnitTarget());
        }
    }
    else if (Item* targetItem = m_targets.getItemTarget())
    {
        /// Not own traded item (in trader trade slot) req. reagents including triggered spell case
        if (targetItem->GetOwnerGUID() != m_caster->GetGUID())
            TakeReagents();
    }

    // are there any spells need to be triggered after hit?
    // handle SPELL_AURA_ADD_TARGET_TRIGGER auras
    Unit::AuraEffectList const& targetTriggers = m_caster->GetAuraEffectsByType(SPELL_AURA_ADD_TARGET_TRIGGER);
    for (Unit::AuraEffectList::const_iterator i = targetTriggers.begin(); i != targetTriggers.end(); ++i)
    {
        if (!(*i)->IsAffectedOnSpell(m_spellInfo))
            continue;
        SpellEntry const *auraSpellInfo = (*i)->GetSpellProto();
        uint32 auraSpellIdx = (*i)->GetEffIndex();
        if (SpellEntry const *spellInfo = sSpellStore.LookupEntry(auraSpellInfo->EffectTriggerSpell[auraSpellIdx]))
        {
            int32 auraBaseAmount = (*i)->GetBaseAmount();
            int32 chance = m_caster->CalculateSpellDamage(NULL, auraSpellInfo, auraSpellIdx, &auraBaseAmount);
            m_ChanceTriggerSpells.push_back(std::make_pair(spellInfo, chance * (*i)->GetBase()->GetStackAmount()));
        }
    }

    if (m_customAttr & SPELL_ATTR0_CU_DIRECT_DAMAGE)
        CalculateDamageDoneForAllTargets();

    // CAST SPELL
    PrepareScriptHitHandlers();

    for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        switch(m_spellInfo->Effect[i])
        {
            case SPELL_EFFECT_CHARGE:
            case SPELL_EFFECT_CHARGE_DEST:
            case SPELL_EFFECT_JUMP:
            case SPELL_EFFECT_JUMP_DEST:
            case SPELL_EFFECT_LEAP_BACK:
            case SPELL_EFFECT_ACTIVATE_RUNE:
                HandleEffects(NULL,NULL,NULL,i);
                m_handledEffectMask |= (1<<i);
                break;
        }
    }

    // we must send smsg_spell_go packet before m_castItem delete in TakeCastItem()...
    SendSpellGo();

    SendSpellCooldown();

    // Okay, everything is prepared. Now we need to distinguish between immediate and evented delayed spells
    if (m_delay || m_spellInfo->Id == 14157)
    {
        // Remove used for cast item if need (it can be already NULL after TakeReagents call
        // in case delayed spell remove item at cast delay start
        TakeCastItem();

        // Okay, maps created, now prepare flags
        m_immediateHandled = false;
        m_spellState = SPELL_STATE_DELAYED;
        SetDelayStart(0);

        if (m_caster->HasUnitState(UNIT_STATE_CASTING) && !m_caster->IsNonMeleeSpellCasted(false, false, true))
            m_caster->ClearUnitState(UNIT_STATE_CASTING);
    }
    else
    {
        // Immediate spell, no big deal
        handle_immediate();
    }

    HandeAfterCast();

    if (m_customAttr & SPELL_ATTR0_CU_LINK_CAST)
    {
        if (const std::vector<int32> *spell_triggered = sSpellMgr->GetSpellLinked(m_spellInfo->Id))
        {
            for (std::vector<int32>::const_iterator i = spell_triggered->begin(); i != spell_triggered->end(); ++i)
                if (*i < 0)
                    m_caster->RemoveAurasDueToSpell(-(*i));
                else
                    m_caster->CastSpell(m_targets.getUnitTarget() ? m_targets.getUnitTarget() : m_caster, *i, true);
        }
    }

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
        m_caster->ToPlayer()->SetSpellModTakingSpell(this, false);

    SetExecutedCurrently(false);
}

void Spell::handle_immediate()
{
    // start channeling if applicable
    if (IsChanneledSpell(m_spellInfo))
    {
        int32 duration = GetSpellDuration(m_spellInfo);
        if (duration)
        {
            // First mod_duration then haste - see Missile Barrage
            // Apply duration mod
            if (Player* modOwner = m_caster->GetSpellModOwner())
                modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_DURATION, duration);
            // Apply haste mods
            if (m_spellInfo->AttributesEx5 & SPELL_ATTR5_HASTE_AFFECT_DURATION)
                m_caster->ModSpellCastTime(m_spellInfo, duration, this);

            m_spellState = SPELL_STATE_CASTING;
            m_caster->AddInterruptMask(m_spellInfo->ChannelInterruptFlags);
            SendChannelStart(duration);
        }
    }

    PrepareTargetProcessing();

    // process immediate effects (items, ground, etc.) also initialize some variables
    _handle_immediate_phase();

    for (std::list<TargetInfo>::iterator ihit= m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
        DoAllEffectOnTarget(&(*ihit));

    for (std::list<GOTargetInfo>::iterator ihit= m_UniqueGOTargetInfo.begin(); ihit != m_UniqueGOTargetInfo.end(); ++ihit)
        DoAllEffectOnTarget(&(*ihit));

    FinishTargetProcessing();

    // spell is finished, perform some last features of the spell here
    _handle_finish_phase();

    // Remove used for cast item if need (it can be already NULL after TakeReagents call
    TakeCastItem();

    if (m_spellState != SPELL_STATE_CASTING)
        finish(true);                                       // successfully finish spell cast (not last in case autorepeat or channel spell)
}

uint64 Spell::handle_delayed(uint64 t_offset)
{
    UpdatePointers();

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
        m_caster->ToPlayer()->SetSpellModTakingSpell(this, true);

    uint64 next_time = 0;

    PrepareTargetProcessing();

    if (!m_immediateHandled)
    {
        _handle_immediate_phase();
        m_immediateHandled = true;
    }

    bool single_missile = (m_targets.HasDst());

    // now recheck units targeting correctness (need before any effects apply to prevent adding immunity at first effect not allow apply second spell effect and similar cases)
    for (std::list<TargetInfo>::iterator ihit= m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
    {
        if (ihit->processed == false)
        {
            if (single_missile || ihit->timeDelay <= t_offset)
                DoAllEffectOnTarget(&(*ihit));
            else if (next_time == 0 || ihit->timeDelay < next_time)
                next_time = ihit->timeDelay;
        }
    }

    // now recheck gameobject targeting correctness
    for (std::list<GOTargetInfo>::iterator ighit= m_UniqueGOTargetInfo.begin(); ighit != m_UniqueGOTargetInfo.end(); ++ighit)
    {
        if (ighit->processed == false)
        {
            if (single_missile || ighit->timeDelay <= t_offset)
                DoAllEffectOnTarget(&(*ighit));
            else if (next_time == 0 || ighit->timeDelay < next_time)
                next_time = ighit->timeDelay;
        }
    }

    FinishTargetProcessing();

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
        m_caster->ToPlayer()->SetSpellModTakingSpell(this, false);

    // All targets passed - need finish phase
    if (next_time == 0)
    {
        // spell is finished, perform some last features of the spell here
        _handle_finish_phase();

        finish(true);                                       // successfully finish spell cast

        // return zero, spell is finished now
        return 0;
    }
    else
    {
        // spell is unfinished, return next execution time
        return next_time;
    }
}

void Spell::_handle_immediate_phase()
{
    m_spellAura = NULL;
    // handle some immediate features of the spell here
    HandleThreatSpells(m_spellInfo->Id);

    PrepareScriptHitHandlers();

    for (uint32 j = 0; j < MAX_SPELL_EFFECTS; ++j)
    {
        if (m_spellInfo->Effect[j] == 0)
            continue;

        // apply Send Event effect to ground in case empty target lists
        if (m_spellInfo->Effect[j] == SPELL_EFFECT_SEND_EVENT && !HaveTargetsForEffect(j))
        {
            HandleEffects(NULL, NULL, NULL, j);
            continue;
        }
    }

    // initialize Diminishing Returns Data
    m_diminishLevel = DIMINISHING_LEVEL_1;
    m_diminishGroup = DIMINISHING_NONE;

    // process items
    for (std::list<ItemTargetInfo>::iterator ihit= m_UniqueItemInfo.begin(); ihit != m_UniqueItemInfo.end(); ++ihit)
        DoAllEffectOnTarget(&(*ihit));

    if (!m_originalCaster)
        return;
    uint8 oldEffMask = m_handledEffectMask;
    // process ground
    for (uint32 j = 0; j < MAX_SPELL_EFFECTS; ++j)
    {
        if (m_spellInfo->Effect[j] == 0)
            continue;

        if (EffectTargetType[m_spellInfo->Effect[j]] == SPELL_REQUIRE_DEST)
        {
            if (!m_targets.HasDst()) // FIXME: this will ignore dest set in effect
                m_targets.setDst(*m_caster);
            HandleEffects(m_originalCaster, NULL, NULL, j);
            m_handledEffectMask |= (1<<j);
        }
        else if (EffectTargetType[m_spellInfo->Effect[j]] == SPELL_REQUIRE_NONE)
        {
            HandleEffects(m_originalCaster, NULL, NULL, j);
            m_handledEffectMask |= (1<<j);
        }
    }
    if (oldEffMask != m_handledEffectMask && m_UniqueTargetInfo.empty())
    {
        uint32 procAttacker = m_procAttacker;
        if (!procAttacker)
        {
            bool positive = true;
            for (uint8 i = 0; i< MAX_SPELL_EFFECTS; ++i)
                // If at least one effect negative spell is negative hit
                if (m_handledEffectMask & (1<<i) && !IsPositiveEffect(m_spellInfo->Id, i))
                {
                    positive = false;
                    break;
                }
            switch(m_spellInfo->DmgClass)
            {
                case SPELL_DAMAGE_CLASS_MAGIC:
                    if (positive)
                        procAttacker |= PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS;
                    else
                        procAttacker |= PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG;
                break;
                case SPELL_DAMAGE_CLASS_NONE:
                    if (positive)
                        procAttacker |= PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_POS;
                    else
                        procAttacker |= PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG;
                break;
            }
        }
        // Proc damage for spells which have only dest targets (2484 should proc 51486 for example)
        m_originalCaster->ProcDamageAndSpell(0, procAttacker, 0, m_procEx | PROC_EX_NORMAL_HIT, 0, BASE_ATTACK, m_spellInfo, m_triggeredByAuraSpell);
    }
}

void Spell::_handle_finish_phase()
{
    if (m_caster->m_movedPlayer)
    {
        // Take for real after all targets are processed
        if (m_needComboPoints)
            m_caster->m_movedPlayer->ClearComboPoints();

        // Real add combo points from effects
        if (m_comboPointGain)
            m_caster->m_movedPlayer->GainSpellComboPoints(m_comboPointGain);
    }
}

void Spell::HandeAfterCast()
{
    // Increase ressurection data after using combat res
    if (InstanceScript * pInstance = m_caster->GetInstanceScript())
        if (pInstance->instance->IsRaid() && pInstance->IsEncounterInProgress())
            if (m_spellInfo->AttributesEx8 & SPELL_ATTR8_BATTLE_RESURRECTION)
                pInstance->AddRessurectionData();

    switch (m_spellInfo->Id)
    {
        // Blood Boil
        case 48721:
            m_caster->CastSpell(m_caster, 65658, true); // + 10 runic power
            break;
        // Ice Lance - special case (drop charge of Fingers of Frost)
        case 30455:
            if(Aura* pAura = m_caster->GetAura(44544))
                if (pAura->ModStackAmount(-1))
                    m_caster->RemoveAurasDueToSpell(44544);
            break;
        // Deep freeze
        case 44572:
        {
            if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                break;
            // Deals damage to targets that are permanently immune to stuns
            if (unitTarget->ToCreature()->GetCreatureInfo()->MechanicImmuneMask & (1 << (MECHANIC_STUN - 1)))
                m_caster->CastSpell(unitTarget, 71757, true);
            break;
        }
        // Shadowburn
        case 17877:
        {
            // Glyph of Shadowburn implementation
            if (unitTarget && unitTarget->GetHealthPct() <= 20.0f // 20% hp
                && damage < int32(unitTarget->GetHealth()) // target is still alve
                && m_caster->HasAura(56229) // has glyph
                && !m_caster->ToPlayer()->HasSpellCooldown(56229)) // without CD
            {
                m_caster->CastSpell(m_caster, 77691, true); // dummy hack!
            }
            break;
        }
    }
}

void Spell::SendSpellCooldown()
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* _player = (Player*)m_caster;

    // mana/health/etc potions, disabled by client (until combat out as declarate)
    if (m_CastItem && m_CastItem->IsPotion())
    {
        // need in some way provided data for Spell::finish SendCooldownEvent
        _player->SetLastPotionId(m_CastItem->GetEntry());
        return;
    }

    // have infinity cooldown but set at aura apply                  // do not set cooldown for triggered spells (needed by reincarnation)
    if (m_spellInfo->Attributes & (SPELL_ATTR0_DISABLED_WHILE_ACTIVE | SPELL_ATTR0_PASSIVE) || m_IsTriggeredSpell)
        return;


    if (m_spellInfo->Id == 78674 && _player->HasAura(93400))
    {
        if (m_casttime > 0) // Do not trigger cooldown for Starsurge if player was casting normal one (without inst casting boost)
        {
            _player->RemoveSpellCooldown(78674, true); // From some reason cooldown is set anyway, so clear it manually
            return;
        }
        // Shooting Stars
        _player->RemoveAurasDueToSpell(93400);
    }
    _player->AddSpellAndCategoryCooldowns(m_spellInfo,m_CastItem ? m_CastItem->GetEntry() : 0, this);
}

void Spell::update(uint32 difftime)
{
    // update pointers based at it's GUIDs
    UpdatePointers();

    if (m_targets.getUnitTargetGUID() && !m_targets.getUnitTarget())
    {
        sLog->outDebug("Spell %u is cancelled due to removal of target.", m_spellInfo->Id);
        cancel();
        return;
    }

    // check if the player caster has moved before the spell finished
    if ((m_caster->GetTypeId() == TYPEID_PLAYER && m_timer != 0) &&
        m_caster->isMoving() && (m_spellInfo->InterruptFlags & SPELL_INTERRUPT_FLAG_MOVEMENT) &&
        (m_spellInfo->Effect[0] != SPELL_EFFECT_STUCK || !m_caster->HasUnitMovementFlag(MOVEMENTFLAG_FALLING)))
    {
        // don't cancel for melee, autorepeat, triggered and instant spells
        if (!IsNextMeleeSwingSpell() && !IsAutoRepeat() && !m_IsTriggeredSpell &&
                !m_caster->CanCastWhileWalking(m_spellInfo))
            cancel();
    }

    switch(m_spellState)
    {
        case SPELL_STATE_PREPARING:
        {
            if (m_timer)
            {
                if (difftime >= m_timer)
                    m_timer = 0;
                else
                    m_timer -= difftime;
            }

            if (m_timer == 0 && !IsNextMeleeSwingSpell() && !IsAutoRepeat())
                cast(m_spellInfo->CastingTimeIndex == 1);
        } break;
        case SPELL_STATE_CASTING:
        {
            if (m_timer > 0)
            {
                // check if there are alive targets left
                if (!UpdateChanneledTargetList())
                {
                    sLog->outDebug("Channeled spell %d is removed due to lack of targets", m_spellInfo->Id);
                    SendChannelUpdate(0);
                    finish();
                }

                if (difftime >= m_timer)
                    m_timer = 0;
                else
                    m_timer -= difftime;
            }

            if (m_timer == 0)
            {
                SendChannelUpdate(0);

                // channeled spell processed independently for quest targeting
                // cast at creature (or GO) quest objectives update at successful cast channel finished
                // ignore autorepeat/melee casts for speed (not exist quest for spells (hm...)
                if (!IsAutoRepeat() && !IsNextMeleeSwingSpell())
                {
                    if (Player* p = m_caster->GetCharmerOrOwnerPlayerOrPlayerItself())
                    {
                        for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                        {
                            TargetInfo* target = &*ihit;
                            if (!IS_CRE_OR_VEH_GUID(target->targetGUID))
                                continue;

                            Unit* unit = m_caster->GetGUID() == target->targetGUID ? m_caster : ObjectAccessor::GetUnit(*m_caster, target->targetGUID);
                            if (unit == NULL)
                                continue;

                            p->CastedCreatureOrGO(unit->GetEntry(), unit->GetGUID(), m_spellInfo->Id);
                        }

                        for (std::list<GOTargetInfo>::iterator ihit = m_UniqueGOTargetInfo.begin(); ihit != m_UniqueGOTargetInfo.end(); ++ihit)
                        {
                            GOTargetInfo* target = &*ihit;

                            GameObject* go = m_caster->GetMap()->GetGameObject(target->targetGUID);
                            if (!go)
                                continue;

                            p->CastedCreatureOrGO(go->GetEntry(), go->GetGUID(), m_spellInfo->Id);
                        }
                    }
                }

                finish();
            }
        } break;
        default:
        {
        }break;
    }
}

void Spell::finish(bool ok)
{
    if (!m_caster)
        return;

    if (m_spellState == SPELL_STATE_FINISHED)
        return;
    m_spellState = SPELL_STATE_FINISHED;

    if (IsChanneledSpell(m_spellInfo))
        m_caster->UpdateInterruptMask();

    if (m_caster->HasUnitState(UNIT_STATE_CASTING) && !m_caster->IsNonMeleeSpellCasted(false, false, true))
        m_caster->ClearUnitState(UNIT_STATE_CASTING);

    // Unsummon summon as possessed creatures on spell cancel
    if (IsChanneledSpell(m_spellInfo) && m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        if (Unit *charm = m_caster->GetCharm())
            if (charm->GetTypeId() == TYPEID_UNIT
                && charm->ToCreature()->HasUnitTypeMask(UNIT_MASK_PUPPET)
                && charm->GetUInt32Value(UNIT_CREATED_BY_SPELL) == m_spellInfo->Id)
                ((Puppet*)charm)->UnSummon();
    }

    if (!ok)
        return;

    if (m_caster->GetTypeId() == TYPEID_UNIT && m_caster->ToCreature()->IsSummon())
    {
        // Unsummon statue
        uint32 spell = m_caster->GetUInt32Value(UNIT_CREATED_BY_SPELL);
        SpellEntry const *spellInfo = sSpellStore.LookupEntry(spell);
        if (spellInfo && spellInfo->SpellIconID == 2056)
        {
            sLog->outDebug("Statue %d is unsummoned in spell %d finish", m_caster->GetGUIDLow(), m_spellInfo->Id);
            m_caster->setDeathState(JUST_DIED);
            return;
        }
    }

    if (IsAutoActionResetSpell())
    {
        bool found = false;
        Unit::AuraEffectList const& vIgnoreReset = m_caster->GetAuraEffectsByType(SPELL_AURA_IGNORE_MELEE_RESET);
        for (Unit::AuraEffectList::const_iterator i = vIgnoreReset.begin(); i != vIgnoreReset.end(); ++i)
        {
            if ((*i)->IsAffectedOnSpell(m_spellInfo))
            {
                found = true;
                break;
            }
        }
        if (!found && !(m_spellInfo->AttributesEx2 & SPELL_ATTR2_NOT_RESET_AUTO_ACTIONS))
        {
            m_caster->resetAttackTimer(BASE_ATTACK);
            if (m_caster->haveOffhandWeapon())
                m_caster->resetAttackTimer(OFF_ATTACK);
            m_caster->resetAttackTimer(RANGED_ATTACK);
        }
    }

    // potions disabled by client, send event "not in combat" if need
    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        if (!m_triggeredByAuraSpell)
            m_caster->ToPlayer()->UpdatePotionCooldown(this);

        // triggered spell pointer can be not set in some cases
        // this is needed for proper apply of triggered spell mods
        m_caster->ToPlayer()->SetSpellModTakingSpell(this, true);
    }

    // Take mods after trigger spell (needed for 14177 to affect 48664)
    // mods are taken only on succesfull cast and independantly from targets of the spell
    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        m_caster->ToPlayer()->RemoveSpellMods(this);
        m_caster->ToPlayer()->SetSpellModTakingSpell(this, false);
    }

    //Case for Holy Power taking spells which takes all charges for greater healing and for dropping of Empowered imp buff
    switch (m_spellInfo->Id)
    {
        case 85222: // Light of Dawn
            m_caster->SetPower(POWER_HOLY_POWER, 0);
            break;
        case 6353: //Soul Fire
            m_caster->RemoveAurasDueToSpell(47283);
            break;
        default:
            break;
    }

    // Stop Attack for some spells
    if (m_spellInfo->Attributes & SPELL_ATTR0_STOP_ATTACK_TARGET)
        m_caster->AttackStop();
}

void Spell::SendCastResult(SpellCastResult result)
{
    if (result == SPELL_CAST_OK)
        return;

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    if (m_caster->ToPlayer()->GetSession()->PlayerLoading())  // don't send cast results at loading time
        return;

    SendCastResult((Player*)m_caster,m_spellInfo,m_cast_count,result);
}

void Spell::SendCastResult(Player* caster, SpellEntry const* spellInfo, uint8 cast_count, SpellCastResult result)
{
    if (result == SPELL_CAST_OK)
        return;

    WorldPacket data(SMSG_CAST_FAILED, (4+1+1));
    data << uint8(cast_count);                              // single cast or multi 2.3 (0/1)
    data << uint32(spellInfo->Id);
    data << uint8(result);                                  // problem
    switch (result)
    {
        case SPELL_FAILED_REQUIRES_SPELL_FOCUS:
            data << uint32(spellInfo->RequiresSpellFocus);
            break;
        case SPELL_FAILED_REQUIRES_AREA:
            // hardcode areas limitation case
            switch(spellInfo->Id)
            {
                case 41617:                                 // Cenarion Mana Salve
                case 41619:                                 // Cenarion Healing Salve
                    data << uint32(3905);
                    break;
                case 41618:                                 // Bottled Nethergon Energy
                case 41620:                                 // Bottled Nethergon Vapor
                    data << uint32(3842);
                    break;
                case 45373:                                 // Bloodberry Elixir
                    data << uint32(4075);
                    break;
                default:                                    // default case (don't must be)
                    data << uint32(0);
                    break;
            }
            break;
        case SPELL_FAILED_TOTEMS:
            if (spellInfo->Totem[0])
                data << uint32(spellInfo->Totem[0]);
            if (spellInfo->Totem[1])
                data << uint32(spellInfo->Totem[1]);
            break;
        case SPELL_FAILED_TOTEM_CATEGORY:
            if (spellInfo->TotemCategory[0])
                data << uint32(spellInfo->TotemCategory[0]);
            if (spellInfo->TotemCategory[1])
                data << uint32(spellInfo->TotemCategory[1]);
            break;
        case SPELL_FAILED_EQUIPPED_ITEM_CLASS:
            data << uint32(spellInfo->EquippedItemClass);
            data << uint32(spellInfo->EquippedItemSubClassMask);
            //data << uint32(spellInfo->EquippedItemInventoryTypeMask);
            break;
        case SPELL_FAILED_TOO_MANY_OF_ITEM:
        {
             uint32 item = 0;
             for (int8 x=0;x < 3;x++)
                 if (spellInfo->EffectItemType[x])
                     item = spellInfo->EffectItemType[x];
             ItemPrototype const *pProto = sObjectMgr->GetItemPrototype(item);
             if (pProto && pProto->ItemLimitCategory)
                 data << uint32(pProto->ItemLimitCategory);
             break;
        }
        default:
            break;
    }
    caster->GetSession()->SendPacket(&data);
}

void Spell::SendSpellStart()
{
    if (!IsNeedSendToClient())
        return;

    //sLog->outDebug("Sending SMSG_SPELL_START id=%u", m_spellInfo->Id);

    uint32 castFlags = CAST_FLAG_UNKNOWN_2;
    if ((m_IsTriggeredSpell && !IsAutoRepeatRangedSpell(m_spellInfo)) || m_triggeredByAuraSpell) 
         castFlags |= CAST_FLAG_PENDING;

    if ((m_caster->GetTypeId() == TYPEID_PLAYER ||
        (m_caster->GetTypeId() == TYPEID_UNIT && m_caster->ToCreature()->IsPet()))
         && m_spellInfo->powerType != POWER_HEALTH)
         castFlags |= CAST_FLAG_NO_GCD;

    if (m_spellInfo->runeCostID && m_spellInfo->powerType == POWER_RUNE)
        castFlags |= CAST_FLAG_NO_GCD; // // not needed, but Blizzard sends it

    // Check whether spell has effect HEAL
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; i++)
        if (m_spellInfo->Effect[i] == SPELL_EFFECT_HEAL)
        {
            castFlags |= CAST_FLAG_HEAL_VALUE;
            break;
        }

    WorldPacket data(SMSG_SPELL_START, (8+8+4+4+2));
    if (m_CastItem)
        data.append(m_CastItem->GetPackGUID());
    else
        data.append(m_caster->GetPackGUID());

    data.append(m_caster->GetPackGUID());
    data << uint8(m_cast_count);                            // pending spell cast?
    data << uint32(m_spellInfo->Id);                        // spellId
    data << uint32(castFlags);                              // cast flags
    data << uint32(0);                                      // unk 4.3.4
    data << int32(m_timer);                                 // delay? signed int?

    //data << int32(0);                                       // unk 4.3.4

    m_targets.write(data);

    if (castFlags & CAST_FLAG_POWER_LEFT_SELF)
        data << uint32(m_caster->GetPower((Powers)m_spellInfo->powerType));

    if (castFlags & CAST_FLAG_AMMO)
        WriteAmmoToPacket(&data);

    if (castFlags & CAST_FLAG_IMMUNITY)
    {
        data << uint32(0);
        data << uint32(0);

        data << uint32(0);
        data << uint8(2);       // if ==2
        data << uint64(0);      //GUID
    }

    if (castFlags & CAST_FLAG_HEAL_VALUE)
    {
        uint32 healvalue = 0;
        if (m_targets.getUnitTarget())
        {
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; i++)
                if (m_spellInfo->Effect[i] == SPELL_EFFECT_HEAL)
                {
                    healvalue = uint32(CalculateDamage(i, m_targets.getUnitTarget()));
                    break;
                }
        }
        data << uint32(healvalue);
        data << uint8(0);
    }

    m_caster->SendMessageToSet(&data, true);
}

void Spell::SendSpellGo()
{
    // not send invisible spell casting
    // One exception for rune refresh spell used for that - unfortunatelly we have to solve it that way
    // because we probably miss some opcodes for that
    if (!IsNeedSendToClient() && m_spellInfo->Id != 89831)
        return;

    //sLog->outDebug("Sending SMSG_SPELL_GO id=%u", m_spellInfo->Id);

    uint32 castFlags = CAST_FLAG_UNKNOWN_9;

    // triggered spells should be marked as pending only when some non-triggered spell is being casted right now
    // (being casted = has cast time)
    if (m_caster && m_caster->IsNonMeleeSpellCasted(true, false, true, true, true) && ((m_IsTriggeredSpell && !IsAutoRepeatRangedSpell(m_spellInfo)) || m_triggeredByAuraSpell))
        castFlags |= CAST_FLAG_PENDING;

    if (m_spellInfo->Attributes & SPELL_ATTR0_REQ_AMMO)
        castFlags |= CAST_FLAG_AMMO;                        // arrows/bullets visual
    if ((m_caster->GetTypeId() == TYPEID_PLAYER ||
        (m_caster->GetTypeId() == TYPEID_UNIT && m_caster->ToCreature()->IsPet()))
        && m_spellInfo->powerType != POWER_HEALTH)
        castFlags |= CAST_FLAG_POWER_LEFT_SELF; // should only be sent to self, but the current messaging doesn't make that possible

    if ((m_caster->GetTypeId() == TYPEID_PLAYER)
        && (m_caster->getClass() == CLASS_DEATH_KNIGHT)
        && m_spellInfo->runeCostID
        && m_spellInfo->powerType == POWER_RUNE)
    {
        castFlags |= CAST_FLAG_NO_GCD;                       // not needed, but Blizzard sends it
        castFlags |= CAST_FLAG_RUNE_LIST;                    // rune cooldowns list
        castFlags |= CAST_FLAG_UNKNOWN_9;                    // ??
    }

    if (IsSpellHaveEffect(m_spellInfo, SPELL_EFFECT_ACTIVATE_RUNE))
    {
        castFlags |= CAST_FLAG_RUNE_LIST;                    // rune cooldowns list
        castFlags |= CAST_FLAG_NO_GCD;                       // not needed, but Blizzard sends it
    }

    if (m_targets.HasTraj())
        castFlags |= CAST_FLAG_ADJUST_MISSILE;

    if (!m_spellInfo->StartRecoveryTime)
        castFlags |= CAST_FLAG_NO_GCD;

    WorldPacket data(SMSG_SPELL_GO, 50);                    // guess size

    if (m_CastItem)
        data.append(m_CastItem->GetPackGUID());
    else
        data.append(m_caster->GetPackGUID());

    data.append(m_caster->GetPackGUID());
    data << uint8(m_cast_count);                            // pending spell cast?
    data << uint32(m_spellInfo->Id);                        // spellId
    data << uint32(castFlags);                              // cast flags
    data << uint32(m_timer);                                // unk 4.3.4
    data << uint32(getMSTime());                            // timestamp

    /*
    // statement below seems to be wrong - i've seen spells with both unit and dest target
    // Can't have TARGET_FLAG_UNIT when *_LOCATION is present - it breaks missile visuals
    if (m_targets.getTargetMask() & (TARGET_FLAG_SOURCE_LOCATION | TARGET_FLAG_DEST_LOCATION))
        m_targets.setTargetMask(m_targets.getTargetMask() & ~TARGET_FLAG_UNIT);
    else if (m_targets.getIntTargetFlags() & FLAG_INT_UNIT)
        m_targets.setTargetMask(m_targets.getTargetMask() | TARGET_FLAG_UNIT);
    */

    WriteSpellGoTargets(&data);

    m_targets.write(data);

    if (castFlags & CAST_FLAG_POWER_LEFT_SELF)
        data << uint32(m_caster->GetPower((Powers)m_spellInfo->powerType));

    if (castFlags & CAST_FLAG_RUNE_LIST)                   // rune cooldowns list
    {
        if (Player* player = m_caster->ToPlayer())
        {
            data << uint8(m_runesState);                          // runes state before
            data << uint8(m_caster->ToPlayer()->GetRunesState()); // runes state after
            for (uint8 i = 0; i < MAX_RUNES; ++i)
            {
                float baseCd = float(player->GetRuneBaseCooldown(i));
                float val = (baseCd - float(player->GetRuneCooldown(i))) / baseCd * 255;
                if (val > 255)
                    val = 255;
                if (val < 0)
                    val = 0;

                data << uint8(val); // rune cooldown passed
            }
        }
    }

    if (castFlags & CAST_FLAG_ADJUST_MISSILE)
    {
        /*
        // Data from Trinity
        data << m_targets.GetElevation();
        data << uint32(m_delayMoment);
        */
        data << uint32(m_targets.m_elevation);
        data << uint32(0);
    }

    if (castFlags & CAST_FLAG_AMMO)
        WriteAmmoToPacket(&data);

    if (castFlags & CAST_FLAG_VISUAL_CHAIN)
    {
        data << uint32(0);
        data << uint32(0);
    }

    if (m_targets.getTargetMask() & TARGET_FLAG_DEST_LOCATION)
    {
        data << uint8(0);
    }

    if (m_targets.getTargetMask() & TARGET_FLAG_EXTRA_TARGETS)
    {
        data << uint8(0); // Extra targets count
        /*
        for (uint8 i = 0; i < count; ++i)
        {
            data << float(0);   // Target Position X
            data << float(0);   // Target Position Y
            data << float(0);   // Target Position Z
            data << uint64(0);  // Target Guid
        }
        */
    }

    m_caster->SendMessageToSet(&data, true);
}

void Spell::WriteAmmoToPacket(WorldPacket * data)
{
    /*uint32 ammoInventoryType = 0;
    uint32 ammoDisplayID = 0;

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        Item *pItem = m_caster->ToPlayer()->GetWeaponForAttack(RANGED_ATTACK);
        if (pItem)
        {
            ammoInventoryType = pItem->GetProto()->InventoryType;
            if (ammoInventoryType == INVTYPE_THROWN)
                ammoDisplayID = pItem->GetProto()->DisplayInfoID;
            else
            {
                uint32 ammoID = m_caster->ToPlayer()->GetUInt32Value(PLAYER_AMMO_ID);
                if (ammoID)
                {
                    ItemPrototype const *pProto = sObjectMgr->GetItemPrototype(ammoID);
                    if (pProto)
                    {
                        ammoDisplayID = pProto->DisplayInfoID;
                        ammoInventoryType = pProto->InventoryType;
                    }
                }
                else if (m_caster->HasAura(46699))      // Requires No Ammo
                {
                    ammoDisplayID = 5996;                   // normal arrow
                    ammoInventoryType = INVTYPE_AMMO;
                }
            }
        }
    }
    else
    {
        for (uint8 i = 0; i < 3; ++i)
        {
            if (uint32 item_id = m_caster->GetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + i))
            {
                if (ItemEntry const * itemEntry = sItemStore.LookupEntry(item_id))
                {
                    if (itemEntry->Class == ITEM_CLASS_WEAPON)
                    {
                        switch(itemEntry->SubClass)
                        {
                            case ITEM_SUBCLASS_WEAPON_THROWN:
                                ammoDisplayID = itemEntry->DisplayId;
                                ammoInventoryType = itemEntry->InventoryType;
                                break;
                            case ITEM_SUBCLASS_WEAPON_BOW:
                            case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                                ammoDisplayID = 5996;       // is this need fixing?
                                ammoInventoryType = INVTYPE_AMMO;
                                break;
                            case ITEM_SUBCLASS_WEAPON_GUN:
                                ammoDisplayID = 5998;       // is this need fixing?
                                ammoInventoryType = INVTYPE_AMMO;
                                break;
                        }

                        if (ammoDisplayID)
                            break;
                    }
                }
            }
        }
    }

    *data << uint32(ammoDisplayID);
    *data << uint32(ammoInventoryType);*/
}
// Writes miss and hit targets for a SMSG_SPELL_GO packet
void Spell::WriteSpellGoTargets(WorldPacket * data)
{
    // This function also fill data for channeled spells:
    // m_needAliveTargetMask req for stop channelig if one target die
    for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
    {
        if ((*ihit).effectMask == 0)                  // No effect apply - all immuned add state
            // possibly SPELL_MISS_IMMUNE2 for this??
            ihit->missCondition = SPELL_MISS_IMMUNE2;
    }

    /** Hit and miss target counts are both uint8, that limits us to 255 targets for each
    *   sending more than 255 targets crashes the client (since count sent would be wrong)
    *   Spells like 40647 (with a huge radius) can easily reach this limit (spell might need
    *   target conditions but we still need to limit the number of targets sent and keeping
    *   correct count for both hit and miss). **/

    uint32 hit = 0; 
    size_t hitPos = data->wpos(); 
    *data << (uint8)0; // placeholder 

    for (std::list<TargetInfo>::const_iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end() && hit < 255; ++ihit)
    {
        if ((*ihit).missCondition == SPELL_MISS_NONE) // Add only hits
        {
            *data << uint64(ihit->targetGUID);
            m_channelTargetEffectMask |=ihit->effectMask;
            ++hit;
        }
    }

    for (std::list<GOTargetInfo>::const_iterator ighit = m_UniqueGOTargetInfo.begin(); ighit != m_UniqueGOTargetInfo.end() && hit < 255; ++ighit)
    {
        *data << uint64(ighit->targetGUID); // Always hits
        ++hit;
    }

    uint32 miss = 0;

    size_t missPos = data->wpos();

    *data << (uint8)0; // placeholder 

    for (std::list<TargetInfo>::const_iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end() && miss <= 255; ++ihit)
    {
        if (ihit->missCondition != SPELL_MISS_NONE)        // Add only miss
        {
            *data << uint64(ihit->targetGUID);
            *data << uint8(ihit->missCondition);
            if (ihit->missCondition == SPELL_MISS_REFLECT)
                *data << uint8(ihit->reflectResult);
            ++miss;
        }
    }
    // Reset m_needAliveTargetMask for non channeled spell
    if (!IsChanneledSpell(m_spellInfo))
        m_channelTargetEffectMask = 0;

    data->put<uint8>(hitPos, (uint8)hit);
    data->put<uint8>(missPos, (uint8)miss);
}

void Spell::SendLogExecute()
{
	WorldPacket data(SMSG_SPELLLOGEXECUTE, 8+4+4+4+4+8);

    data.append(m_caster->GetPackGUID());

    data << uint32(m_spellInfo->Id);

    uint8 effCount = 0;
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (m_effectExecuteData[i])
            ++effCount;
    }

    if (!effCount)
        return;

    data << uint32(effCount);
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (!m_effectExecuteData[i])
            continue;

        data << uint32(m_spellInfo->Effect[i]);             // spell effect

        data.append(*m_effectExecuteData[i]);

        delete m_effectExecuteData[i];
        m_effectExecuteData[i] = NULL;
    }
    m_caster->SendMessageToSet(&data, true);
}

void Spell::ExecuteLogEffectTakeTargetPower(uint8 effIndex, Unit * target, uint32 powerType, uint32 powerTaken, float gainMultiplier)
{
    InitEffectExecuteData(effIndex);
    m_effectExecuteData[effIndex]->append(target->GetPackGUID());
    *m_effectExecuteData[effIndex] << uint32(powerTaken);
    *m_effectExecuteData[effIndex] << uint32(powerType);
    *m_effectExecuteData[effIndex] << float(gainMultiplier);
}

void Spell::ExecuteLogEffectExtraAttacks(uint8 effIndex, Unit * victim, uint32 attCount)
{
    InitEffectExecuteData(effIndex);
    m_effectExecuteData[effIndex]->append(victim->GetPackGUID());
    *m_effectExecuteData[effIndex] << uint32(attCount);
}

void Spell::ExecuteLogEffectInterruptCast(uint8 effIndex, Unit * victim, uint32 spellId)
{
    InitEffectExecuteData(effIndex);
    m_effectExecuteData[effIndex]->append(victim->GetPackGUID());
    *m_effectExecuteData[effIndex] << uint32(spellId);
}

void Spell::ExecuteLogEffectDurabilityDamage(uint8 effIndex, Unit * victim, uint32 /*itemslot*/, uint32 damage)
{
    InitEffectExecuteData(effIndex);
    m_effectExecuteData[effIndex]->append(victim->GetPackGUID());
    *m_effectExecuteData[effIndex] << uint32(m_spellInfo->Id);
    *m_effectExecuteData[effIndex] << uint32(damage);
}

void Spell::ExecuteLogEffectOpenLock(uint8 effIndex, Object * obj)
{
    InitEffectExecuteData(effIndex);
    m_effectExecuteData[effIndex]->append(obj->GetPackGUID());
}

void Spell::ExecuteLogEffectCreateItem(uint8 effIndex, uint32 entry)
{
    InitEffectExecuteData(effIndex);
    *m_effectExecuteData[effIndex] << uint32(entry);
}

void Spell::ExecuteLogEffectDestroyItem(uint8 effIndex, uint32 entry)
{
    InitEffectExecuteData(effIndex);
    *m_effectExecuteData[effIndex] << uint32(entry);
}

void Spell::ExecuteLogEffectSummonObject(uint8 effIndex, WorldObject * obj)
{
    InitEffectExecuteData(effIndex);
    m_effectExecuteData[effIndex]->append(obj->GetPackGUID());
}

void Spell::ExecuteLogEffectUnsummonObject(uint8 effIndex, WorldObject * obj)
{
    InitEffectExecuteData(effIndex);
    m_effectExecuteData[effIndex]->append(obj->GetPackGUID());
}

void Spell::ExecuteLogEffectResurrect(uint8 effIndex, Unit * target)
{
    InitEffectExecuteData(effIndex);
    m_effectExecuteData[effIndex]->append(target->GetPackGUID());
}

void Spell::SendInterrupted(uint8 result)
{
    WorldPacket data(SMSG_SPELL_FAILURE, (8+4+1));
    data.append(m_caster->GetPackGUID());
    data << uint8(m_cast_count);
    data << uint32(m_spellInfo->Id);
    data << uint8(result);
    m_caster->SendMessageToSet(&data, true);

    data.Initialize(SMSG_SPELL_FAILED_OTHER, (8+4));
    data.append(m_caster->GetPackGUID());
    data << uint8(m_cast_count);
    data << uint32(m_spellInfo->Id);
    data << uint8(result);
    m_caster->SendMessageToSet(&data, true);
}

void Spell::SendChannelUpdate(uint32 time)
{
    if (time == 0)
    {
        m_caster->SetUInt64Value(UNIT_FIELD_CHANNEL_OBJECT, 0);
        m_caster->SetUInt32Value(UNIT_CHANNEL_SPELL, 0);
    }

    WorldPacket data(SMSG_CHANNEL_UPDATE, 8+4);
    data.append(m_caster->GetPackGUID());
    data << uint32(time);

    m_caster->SendMessageToSet(&data, true);
}

void Spell::SendChannelStart(uint32 duration)
{
    WorldObject* target = NULL;

    // select first not resisted target from target list for _0_ effect
    if (!m_UniqueTargetInfo.empty())
    {
        for (std::list<TargetInfo>::const_iterator itr = m_UniqueTargetInfo.begin(); itr != m_UniqueTargetInfo.end(); ++itr)
        {
            for (uint8 effIndex = EFFECT_0; effIndex < MAX_SPELL_EFFECTS; effIndex++)
            {
                if ((itr->effectMask & (1 << effIndex)) && itr->reflectResult == SPELL_MISS_NONE && itr->targetGUID != m_caster->GetGUID())
                {
                    target = ObjectAccessor::GetUnit(*m_caster, itr->targetGUID);
                    break;
                }
            }
        }
    }
    else if (!m_UniqueGOTargetInfo.empty())
    {
        for (std::list<GOTargetInfo>::const_iterator itr = m_UniqueGOTargetInfo.begin(); itr != m_UniqueGOTargetInfo.end(); ++itr)
        {
            for (uint8 effIndex = EFFECT_0; effIndex < MAX_SPELL_EFFECTS; effIndex++)
            {
                if (itr->effectMask & (1 << effIndex))
                {
                    target = m_caster->GetMap()->GetGameObject(itr->targetGUID);
                    break;
                }
            }
        }
    }

    WorldPacket data(SMSG_CHANNEL_START, (8+4+4+1+1));
    data.append(m_caster->GetPackGUID());
    data << uint32(m_spellInfo->Id);
    data << uint32(duration);
    data << uint8(0);                           // immunity (castflag & 0x04000000)
    /*
    if (immunity)
    {
        data << uint32();                       // CastSchoolImmunities
        data << uint32();                       // CastImmunities
    }
    */
    data << uint8(0);                           // healPrediction (castflag & 0x40000000)
    /*
    if (healPrediction)
    {
        data.appendPackGUID(channelTarget);     // target packguid
        data << uint32();                       // spellid
        data << uint8(0);                       // unk3
        if (unk3 == 2)
            data.append();                      // unk packed guid (unused ?)
    }
    */
    m_caster->SendMessageToSet(&data, true);

    m_timer = duration;
    if (target)
        m_caster->SetUInt64Value(UNIT_FIELD_CHANNEL_OBJECT, target->GetGUID());
    m_caster->SetUInt32Value(UNIT_CHANNEL_SPELL, m_spellInfo->Id);
}

void Spell::SendResurrectRequest(Player* target)
{
    // Both players and NPCs can resurrect using spells - have a look at creature 28487 for example
    // However, the packet structure differs slightly

    const char* sentName = m_caster->GetTypeId() == TYPEID_PLAYER ? "" : m_caster->GetNameForLocaleIdx(target->GetSession()->GetSessionDbLocaleIndex());
    uint32 spellId = target->GetResurrectionSpellId();

    WorldPacket data(SMSG_RESURRECT_REQUEST, (8+4+strlen(sentName)+1+1+1));
    data << uint64(m_caster->GetGUID());
    data << uint32(strlen(sentName) + 1);

    data << sentName;
    data << uint8(0);

    data << uint8(m_caster->GetTypeId() == TYPEID_PLAYER ? 0 : 1);
    data << uint32(spellId);
    target->GetSession()->SendPacket(&data);
}

void Spell::TakeCastItem()
{
    if (!m_CastItem || m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    // not remove cast item at triggered spell (equipping, weapon damage, etc)
    if (m_IsTriggeredSpell)
        return;

    ItemPrototype const *proto = m_CastItem->GetProto();

    if (!proto)
    {
        // This code is to avoid a crash
        // I'm not sure, if this is really an error, but I guess every item needs a prototype
        sLog->outError("Cast item has no item prototype highId=%d, lowId=%d",m_CastItem->GetGUIDHigh(), m_CastItem->GetGUIDLow());
        return;
    }

    bool expendable = false;
    bool withoutCharges = false;

    for (int i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
    {
        if (proto->Spells[i].SpellId)
        {
            // item has limited charges
            if (proto->Spells[i].SpellCharges)
            {
                if (proto->Spells[i].SpellCharges < 0)
                    expendable = true;

                int32 charges = m_CastItem->GetSpellCharges(i);

                // item has charges left
                if (charges)
                {
                    (charges > 0) ? --charges : ++charges;  // abs(charges) less at 1 after use
                    if (proto->Stackable == 1)
                        m_CastItem->SetSpellCharges(i, charges);
                    m_CastItem->SetState(ITEM_CHANGED, (Player*)m_caster);
                }

                // all charges used
                withoutCharges = (charges == 0);
            }
        }
    }

    if (expendable && withoutCharges)
    {
        uint32 count = 1;
        m_caster->ToPlayer()->DestroyItemCount(m_CastItem, count, true);

        // prevent crash at access to deleted m_targets.getItemTarget
        if (m_CastItem == m_targets.getItemTarget())
            m_targets.setItemTarget(NULL);

        m_CastItem = NULL;
    }
}

void Spell::TakePower()
{
    if (m_CastItem || m_triggeredByAuraSpell)
        return;

    bool hit = true;
    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        if (m_spellInfo->powerType == POWER_RAGE || m_spellInfo->powerType == POWER_ENERGY || m_spellInfo->powerType == POWER_RUNE)
            if (uint64 targetGUID = m_targets.getUnitTargetGUID())
                for (std::list<TargetInfo>::iterator ihit= m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                    if (ihit->targetGUID == targetGUID)
                    {
                        if (ihit->missCondition != SPELL_MISS_NONE && ihit->missCondition != SPELL_MISS_ABSORB)
                        {
                            hit = false;
                            //lower spell cost on fail (by talent aura)
                            if (Player *modOwner = m_caster->ToPlayer()->GetSpellModOwner())
                                modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_SPELL_COST_REFUND_ON_FAIL, m_powerCost);
                        }
                        break;
                    }
    }

    Powers powerType = Powers(m_spellInfo->powerType);

    if (powerType == POWER_RUNE)
    {
        TakeRunePower(hit);
        return;
    }

    if (!m_powerCost)
        return;

    // health as power used
    if (m_spellInfo->powerType == POWER_HEALTH)
    {
        m_caster->ModifyHealth(-(int32)m_powerCost);
        return;
    }

    if (m_spellInfo->powerType >= MAX_POWERS)
    {
        sLog->outError("Spell::TakePower: Unknown power type '%d'", m_spellInfo->powerType);
        return;
    }

    // Terrible hack for Word of Glory not taking holy power when Eternal Glory procs
    // Holy power is taken in Word of Glory handler (Spell::SpellDamageHeal)
    if (GetSpellInfo()->Id == 85673 && m_caster->GetPower(POWER_HOLY_POWER) > 0)
        return;

    // spell Zealotry doesnt consume holypower, only needs 3 od them to be cast
    if (GetSpellInfo()->Id == 85696)
        return;

    // If have Divine Purpose buff and spell is supposed to take holy power, return. Buff removal handled in every spell handler
    if (powerType == POWER_HOLY_POWER && m_caster->HasAura(90174))
        return;

    // Dark Simulacrum - proc on any mana-taking spell ( except any kind of form e.g. cat/bear/shadow form )
    if (powerType == POWER_MANA && m_caster->HasAura(77606) && !GetSpellInfo()->AppliesAuraType(SPELL_AURA_MOD_SHAPESHIFT))
    {
        Aura* pAura = m_caster->GetAura(77606);
        if (pAura && pAura->GetCaster() && !pAura->GetCaster()->HasAura(77616))
        {
            int32 bp0 = GetSpellInfo()->Id;
            pAura->GetCaster()->CastCustomSpell(pAura->GetCaster(), 77616, &bp0, 0, 0, true);
            m_caster->RemoveAurasDueToSpell(77606);
        }
    }

    if (hit)
        m_caster->ModifyPower(powerType, -m_powerCost);
    else
        m_caster->ModifyPower(powerType, -irand(0, m_powerCost/4));
}

void Spell::TakeAmmo()
{
    if (m_attackType == RANGED_ATTACK && m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        Item *pItem = m_caster->ToPlayer()->GetWeaponForAttack(RANGED_ATTACK);

        // wands don't have ammo
        if (!pItem  || pItem->IsBroken() || pItem->GetProto()->SubClass == ITEM_SUBCLASS_WEAPON_WAND)
            return;

        if (pItem->GetProto()->InventoryType == INVTYPE_THROWN)
        {
            if (pItem->GetMaxStackCount() == 1)
            {
                // decrease durability for non-stackable throw weapon
                m_caster->ToPlayer()->DurabilityPointLossForEquipSlot(EQUIPMENT_SLOT_RANGED);
            }
            else
            {
                // decrease items amount for stackable throw weapon
                uint32 count = 1;
                m_caster->ToPlayer()->DestroyItemCount(pItem, count, true);
            }
        }
        //else if (uint32 ammo = m_caster->ToPlayer()->GetUInt32Value(PLAYER_AMMO_ID))
        //    m_caster->ToPlayer()->DestroyItemCount(ammo, 1, true);
    }
}

SpellCastResult Spell::CheckRuneCost(uint32 runeCostID)
{
    if (m_spellInfo->powerType != POWER_RUNE || !runeCostID)
        return SPELL_CAST_OK;

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return SPELL_CAST_OK;

    Player *plr = (Player*)m_caster;

    if (plr->getClass() != CLASS_DEATH_KNIGHT)
        return SPELL_CAST_OK;

    SpellRuneCostEntry const *src = sSpellRuneCostStore.LookupEntry(runeCostID);

    if (!src)
        return SPELL_CAST_OK;

    if (src->NoRuneCost())
        return SPELL_CAST_OK;

    int32 runeCost[NUM_RUNE_TYPES];                         // blood, frost, unholy, death

    for (uint32 i = 0; i < RUNE_DEATH; ++i)
    {
        runeCost[i] = src->RuneCost[i];
        if (Player* modOwner = m_caster->GetSpellModOwner())
            modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_COST, runeCost[i], this);
    }

    runeCost[RUNE_DEATH] = MAX_RUNES;                       // calculated later

    for (uint32 i = 0; i < MAX_RUNES; ++i)
    {
        RuneType rune = plr->GetCurrentRune(i);
        if ((plr->GetRuneCooldown(i) == 0) && (runeCost[rune] > 0))
            runeCost[rune]--;
    }

    for (uint32 i = 0; i < RUNE_DEATH; ++i)
        if (runeCost[i] > 0)
            runeCost[RUNE_DEATH] += runeCost[i];

    if (runeCost[RUNE_DEATH] > MAX_RUNES)
        return SPELL_FAILED_NO_POWER;                       // not sure if result code is correct

    return SPELL_CAST_OK;
}

void Spell::TakeRunePower(bool didhit)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player *plr = (Player*)m_caster;

    if (plr->getClass() != CLASS_DEATH_KNIGHT)
        return;

    SpellRuneCostEntry const *src = sSpellRuneCostStore.LookupEntry(m_spellInfo->runeCostID);

    if (!src || (src->NoRuneCost() && src->NoRunicPowerGain()))
        return;

    m_runesState = plr->GetRunesState();                    // store previous state

    int32 runeCost[NUM_RUNE_TYPES];                         // blood, frost, unholy, death

    bool anyRuneUsed = false;
    for (uint32 i = 0; i < RUNE_DEATH; ++i)
    {
        runeCost[i] = src->RuneCost[i];
        if (Player* modOwner = m_caster->GetSpellModOwner())
            modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_COST, runeCost[i], this);
        if (runeCost[i] > 0)
            anyRuneUsed = true;
        runeCost[i] = std::max(runeCost[i], 0);
    }

    runeCost[RUNE_DEATH] = 0;                               // calculated later
    plr->ClearLastUsedRuneList();

    if (!anyRuneUsed) // if no runes are to be used no runic power is generated => no further processing is needed
        if (GetSpellInfo() && GetSpellInfo()->Id != 57330) // Horn of Winter is exception
            return;

    for (uint32 i = 0; i < MAX_RUNES; ++i)
    {
        RuneType rune = plr->GetCurrentRune(i);
        if ((plr->GetRuneCooldown(i) == 0) && (runeCost[rune] > 0))
        {
            plr->SetRuneCooldown(i, didhit ? plr->GetRuneBaseCooldown(i) : uint32(RUNE_MISS_COOLDOWN));
            plr->SetLastUsedRune(i);
            runeCost[rune]--;
        }
    }

    runeCost[RUNE_DEATH] = runeCost[RUNE_BLOOD] + runeCost[RUNE_UNHOLY] + runeCost[RUNE_FROST];

    if (runeCost[RUNE_DEATH] > 0)
    {
        for (uint32 i = 0; i < MAX_RUNES; ++i)
        {
            RuneType rune = plr->GetCurrentRune(i);
            if ((plr->GetRuneCooldown(i) == 0) && (rune == RUNE_DEATH))
            {
                plr->SetRuneCooldown(i, didhit ? plr->GetRuneBaseCooldown(i) : uint32(RUNE_MISS_COOLDOWN));
                plr->SetLastUsedRune(i);
                runeCost[rune]--;

                // keep Death Rune type if missed
                if (didhit && !plr->HasPermanentDeathRuneInSlot(i))
                    plr->RestoreBaseRune(i);

                if (runeCost[RUNE_DEATH] == 0)
                    break;
            }
        }
    }

    // you can gain some runic power when use runes
    if (didhit)
    {
        float rp = (float)src->runePowerGain;
        rp *= sWorld->getRate(RATE_POWER_RUNICPOWER_INCOME);

        if (plr->HasAuraType(SPELL_AURA_MOD_RUNIC_POWER_FROM_DAMAGE_DEALT))
            rp *= 1.0f + plr->GetTotalAuraModifier(SPELL_AURA_MOD_RUNIC_POWER_FROM_DAMAGE_DEALT) / 100.0f;

        plr->ModifyPower(POWER_RUNIC_POWER, (int32)rp);
    }
}

void Spell::TakeReagents()
{
    if (m_IsTriggeredSpell)                                  // reagents used in triggered spell removed by original spell or don't must be removed.
    {
        Item* targetItem = m_targets.getItemTarget();
        /// Not own traded item (in trader trade slot) req. reagents including triggered spell case
        if (!(targetItem && targetItem->GetOwnerGUID() != m_caster->GetGUID()))
            return;
    }

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    // do not take reagents for these item casts
    if (m_CastItem && m_CastItem->GetProto()->Flags & ITEM_PROTO_FLAG_TRIGGERED_CAST)
        return;

    Player* p_caster = (Player*)m_caster;
    if (p_caster->CanNoReagentCast(m_spellInfo))
        return;

    // Archaeology project check - reagents / keystones
    if (m_spellInfo->researchProjectId)
    {
        // Use "invalid target" to disallow casting of spells with bad data
        // should not happen, but who knows...
        ResearchProjectEntry const* pProject = sResearchProjectStore.LookupEntry(m_spellInfo->researchProjectId);
        if (!pProject)
            return;

        ResearchBranchEntry const* pBranch = sResearchBranchStore.LookupEntry(pProject->researchBranch);
        if (!pBranch)
            return;

        // Disallow using more than maximum keystones
        if (pProject->keyStonesNeeded < m_keyStonesCount)
            m_keyStonesCount = pProject->keyStonesNeeded;

        // Calculate real currency cost of solving
        // each keystone supersedes 12 fragments
        uint32 realCurrencyCost = pProject->fragmentsNeeded - m_keyStonesCount*12;

        m_caster->ToPlayer()->ModifyCurrency(pBranch->currencyId, -int32(realCurrencyCost));
        m_caster->ToPlayer()->DestroyItemCount(pBranch->keyStoneId, m_keyStonesCount, true);

        // Change project in archaeology interface and in our stores
        // Also increment count of times project completed and write data
        uint8 slot = 255;
        for (uint8 i = 0; i < 16; i++) // iterate through all 16 project data fields (2x8)
        {
            if (i < 8)
            {
                if (m_caster->ToPlayer()->GetUInt16Value(PLAYER_FIELD_RESEARCHING_1+i, 0) == pProject->Id)
                {
                    slot = i;
                    break;
                }
            }
            else //if (i >= 8 && i < 16)
            {
                if (m_caster->ToPlayer()->GetUInt16Value(PLAYER_FIELD_RESEARCHING_1+(i-8), 1) == pProject->Id)
                {
                    slot = i;
                    break;
                }
            }
        }

        if (slot < 16)
            m_caster->ToPlayer()->SetNewResearchProject(slot, true);
    }

    for (uint32 x = 0; x < MAX_SPELL_REAGENTS; ++x)
    {
        if (m_spellInfo->Reagent[x] <= 0)
            continue;

        uint32 itemid = m_spellInfo->Reagent[x];
        uint32 itemcount = m_spellInfo->ReagentCount[x];

        // if CastItem is also spell reagent
        if (m_CastItem)
        {
            ItemPrototype const *proto = m_CastItem->GetProto();
            if (proto && proto->ItemId == itemid)
            {
                for (int s = 0; s < MAX_ITEM_PROTO_SPELLS; ++s)
                {
                    // CastItem will be used up and does not count as reagent
                    int32 charges = m_CastItem->GetSpellCharges(s);
                    if (proto->Spells[s].SpellCharges < 0 && abs(charges) < 2)
                    {
                        ++itemcount;
                        break;
                    }
                }

                m_CastItem = NULL;
            }
        }

        // if getItemTarget is also spell reagent
        if (m_targets.getItemTargetEntry() == itemid)
            m_targets.setItemTarget(NULL);

        p_caster->DestroyItemCount(itemid, itemcount, true);
    }
}

void Spell::HandleThreatSpells(uint32 spellId)
{
    if (!spellId)
        return;

    if (!m_targets.getUnitTarget() && m_UniqueTargetInfo.empty())
        return;

    uint16 threat = sSpellMgr->GetSpellThreat(spellId);

    if (!threat)
        return;

    if ((m_spellInfo->AttributesEx  & SPELL_ATTR1_NO_THREAT) ||
        (m_spellInfo->AttributesEx3 & SPELL_ATTR3_NO_INITIAL_AGGRO))
        return;

    if (m_targets.getUnitTarget())
    {
        if (m_targets.getUnitTarget()->CanHaveThreatList())
            m_targets.getUnitTarget()->AddThreat(m_caster, float(threat));
    }

    if (!m_UniqueTargetInfo.empty())
    {
        for (std::list<Spell::TargetInfo>::const_iterator itr = m_UniqueTargetInfo.begin(); itr != m_UniqueTargetInfo.end(); ++itr)
        {
            if (Unit* target = Unit::GetUnit(*m_caster, (*itr).targetGUID))
                if (target->CanHaveThreatList())
                    target->AddThreat(m_caster, float(threat));
        }
    }
}

void Spell::HandleEffects(Unit *pUnitTarget,Item *pItemTarget,GameObject *pGOTarget,uint32 effectIndex)
{
    //effect has been handled, skip it
    if (m_handledEffectMask & (1<<effectIndex))
        return;

    unitTarget = pUnitTarget;
    itemTarget = pItemTarget;
    gameObjTarget = pGOTarget;

    uint8 eff = m_spellInfo->Effect[effectIndex];

    // we do not need DamageMultiplier here.
    // we will also supply at least unit target, because of spell scaling and buff scaling by target, not caster
    damage = CalculateDamage(effectIndex, pUnitTarget);

    bool preventDefault = CallScriptEffectHandlers((SpellEffIndex)effectIndex);

    if (!preventDefault && eff < TOTAL_SPELL_EFFECTS)
    {
        (this->*SpellEffects[eff])((SpellEffIndex)effectIndex);
    }
}

bool Spell::ApplyEffectCondition(SpellEffIndex effIndex)
{
    // Since Cataclysm, Blizzard implemented spells, which has glyph effects built in
    // so we have to use some mechanism to prevent/allow applying those effects if player (don't) have glyph

    // returns true to continue handling effect
    // returns false to prevent applying this effect

    bool result = true;

    // I haven't found any not-player case (i.e. pets as casters and player owners as glyph holders)
    if (!m_caster || m_caster->GetTypeId() != TYPEID_PLAYER)
        return result;

    switch (m_spellInfo->Id)
    {
        case 5782: // Fear
            // Glyph of Fear - apply effect #3, rooting effect
            if (effIndex == EFFECT_2 && !m_caster->HasAura(56244))
                result = false;
            if (m_caster->HasAura(56244) && effIndex == EFFECT_2)
            {
                m_caster->ToPlayer()->AddSpellCooldown(m_spellInfo->Id, 0, 5000);
                // Send cooldown manully
                WorldPacket data(SMSG_SPELL_COOLDOWN, 8 + 1 + 4);
                data << uint64(m_caster->GetGUID());
                data << uint8(1);
                data << uint32(m_spellInfo->Id);
                data << uint32(5000); // 30 seconds
                m_caster->ToPlayer()->GetSession()->SendPacket(&data);
            }
            break;
        case 8122: // Psychic Scream
            // Glyph of Psychic Scream - apply effect #3, rooting effect
            if (effIndex == EFFECT_2 && !m_caster->HasAura(55676))
                result = false;
            break;
        case 85673: // Word of Glory
            // Glyph of the Long Word - apply effect #2, HoT effect
            if (effIndex == EFFECT_1 && !m_caster->HasAura(93466))
                result = false;
            break;
        case 879: // Exorcism
            // DoT from glyph - apply only if caster has the glyph
            if (effIndex == EFFECT_1 && !m_caster->HasAura(54934))
                result = false;
            break;
        case 44614: // Frostfire Bolt
            // Without Glyph of Frostfire - apply effect 0, with glyph - apply effect 2
            if (m_caster->HasAura(61205))
                result = (effIndex != EFFECT_0);
            else
                result = (effIndex != EFFECT_2);
            break;
        case 60433: // Earth and Moon (hack)
            if (Unit * target = m_targets.getUnitTarget())
            {
                // Dont apply -> dont stack with other player auras
                if (target->HasAuraTypeWithValue(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN, 8)
                    && !target->GetAura(60433))
                    result = false;
            }
            break;
    }

    return result;
}

SpellCastResult Spell::CheckCast(bool strict)
{
    // HACK
    // Allow some spell to be cast without additional checks or some explicit checks
    switch (m_spellInfo->Id)
    {
        case 30452: // Rocket Boots Engaged (bypass GCD triggered from use of boot item)
        case 98619: // Wings of Flame 
            return SPELL_CAST_OK;
        case 52610: // Savage Roar
            if (m_caster->GetShapeshiftForm() != FORM_CAT)
               return SPELL_FAILED_NOT_SHAPESHIFT;
            break;
        case 91836: // Fury of Angerforge
            if (!m_caster->HasAura(91832) || m_caster->GetAura(91832)->GetStackAmount() < m_spellInfo->StackAmount)
                return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
            break;
        case 106466: // Dream
            if (!m_caster->HasAura(106464)) // Enter the Dream - enabler
                return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
            break;
    }

    // Combat ressurections per encounter are limited since Cataclysm
    if (InstanceScript * pInstance = m_caster->GetInstanceScript())
    {
        if (pInstance->instance->IsRaid())
        {
            if (m_spellInfo->AttributesEx8 & SPELL_ATTR8_BATTLE_RESURRECTION)
            {
                if (!pInstance->CanUseCombatRessurrection())
                    return SPELL_FAILED_IN_COMBAT_RES_LIMIT_REACHED;
            }
        }
    }

    // Arena spectators are not able to cast spells while in arena
    if (m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->ToPlayer()->InArena() && m_caster->ToPlayer()->GetSpectatorInstanceId() > 0)
        return SPELL_FAILED_NO_VALID_TARGETS;

    // Client allow players to cast destination spells to places which are not in LoS with them -> caused by camera angle ?
    if (m_caster->GetTypeId() == TYPEID_PLAYER && m_spellInfo->SpellFamilyName != SPELLFAMILY_GENERIC
        && m_targets.HasDst() && (m_spellInfo->Targets & TARGET_FLAG_DEST_LOCATION))
    {
        float x, y, z;
        m_targets.m_dstPos.GetPosition(x, y, z);
        if (VMAP::VMapFactory::checkAreaForLoS(m_caster->GetAreaId()) && !m_caster->IsWithinLOS(x,y,z))
            return SPELL_FAILED_LINE_OF_SIGHT;
    }

    // the teleporting spell used for lowlevels in Cata-locations should work only on 
    if (m_spellInfo->Id == 84513 && (m_caster->GetMapId() != 0 && m_caster->GetMapId() != 1 && m_caster->GetMapId() != 646))
        return SPELL_FAILED_NOT_HERE;

    Unit* Target = m_targets.getUnitTarget();
    if (m_spellInfo->Id == 30449 && Target) // Spellsteal Check
    {
        if (Target != m_caster && !Target->IsFriendlyTo(m_caster))
        {
            int SpellSteal = 0;
            Unit::AuraMap const& auras = Target->GetOwnedAuras();
            for (Unit::AuraMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            {
                Aura * aura = itr->second;
                AuraApplication * aurApp = aura->GetApplicationOfTarget(Target->GetGUID());
                if (!aurApp)
                    continue;
                if ((1<<aura->GetSpellProto()->Dispel))
                {
                    if (!aurApp->IsPositive() || aura->IsPassive() || aura->GetSpellProto()->AttributesEx4 & SPELL_ATTR4_NOT_STEALABLE)
                        continue;
                    bool dispel_charges = aura->GetSpellProto()->AttributesEx7 & SPELL_ATTR7_DISPEL_CHARGES;
                    uint8 charges = dispel_charges ? aura->GetCharges() : aura->GetStackAmount();
                    if (charges > 0)
                        if (aura->GetDuration() > 0) // If spell has a duration
                            if (aura->GetSpellProto()->Dispel == DISPEL_MAGIC) // If spell type is magic
                                ++SpellSteal;
                }
            }
            if (SpellSteal == 0)
                return SPELL_FAILED_NOTHING_TO_STEAL;
        }
    }

    // Cobra shot and Chimera shot can't be cast while hunter has detterance
    if ((m_spellInfo->Id == 77767 || m_spellInfo->Id == 53209) && m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        if (m_caster->HasAuraType(SPELL_AURA_DEFLECT_SPELLS))
            return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
    }

    // Killing Spree check - at least one enemy in range
    if (m_spellInfo->Id == 51690)
    {
        if (!SearchNearbyTarget(10.0f, SPELL_TARGETS_ENEMY, EFFECT_1))
            return SPELL_FAILED_NO_VALID_TARGETS;
    }

    // Redirect - player needs to have some combo points on some target
    if (m_spellInfo->Id == 73981 && m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        if (m_caster->ToPlayer()->GetComboPoints() == 0 || m_caster->ToPlayer()->GetComboTarget() == 0)
            return SPELL_FAILED_NO_VALID_TARGETS;
    }

    // Sap
    if (m_spellInfo->Id == 30980 || m_spellInfo->Id == 6770)
    {
        if (Target && Target->HasAura(51755)) // Camouflage
            return SPELL_FAILED_NO_VALID_TARGETS;
    }

    // Magic Broom (instant mount spell) cannot be cast in battleground (and arena)
    if (m_spellInfo->Id == 47977 && m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->ToPlayer()->GetMap() && m_caster->ToPlayer()->GetMap()->IsBattlegroundOrArena())
        return SPELL_FAILED_NOT_IN_BATTLEGROUND;

    // Dont let players cast beg spell by themselves ( learned via /run PickupSpell(7267) macro)
    if (m_spellInfo->Id == 7267 && !m_IsTriggeredSpell)
        return SPELL_FAILED_SPELL_UNAVAILABLE;

    if (m_spellInfo->Id == 53271 && m_caster->GetTypeId() == TYPEID_PLAYER) // Master's Call
    {
        Unit* pPet = Unit::GetUnit(*m_caster, m_caster->GetPetGUID());

        // Can't be cast if pet is dead and also check if pet is able to cast
        if (!pPet || pPet->isDead() || pPet->isFeared() || pPet->HasUnitState(UNIT_STATE_STUNNED) || pPet->HasUnitState(UNIT_STATE_CONFUSED))
            return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
    }

    if (m_spellInfo->Id == 34026) // Kill Command -> cast only if Hunter has target and target is in 5y range from pet
    {
        Pet* pPet = (Pet*)Unit::GetUnit(*m_caster, m_caster->GetPetGUID());

        if (pPet && pPet->IsAlive())
        {
            if (pPet->GetVictim() == NULL) // Pet must has valid target
                return SPELL_FAILED_NO_VALID_TARGETS;

            if (pPet->GetDistance(pPet->GetVictim()) > 5.0f) // Pet need to be in 5 yard range from target
                return SPELL_FAILED_OUT_OF_RANGE;
        }
        else
            return SPELL_FAILED_NO_PET;
    }

    // Wild Mushrooms: Detonate - can be cast only if there are some Mushrooms and all mushrooms must be max 40 yards from caster
    if (m_spellInfo->Id == 88751)
    {
        if (m_caster && m_caster->GetTypeId() == TYPEID_PLAYER)
        {
            Player::GUIDTimestampMap* summonMap = m_caster->ToPlayer()->GetSummonMapFor(47649);
            if (!summonMap || summonMap->empty())
                return SPELL_FAILED_NO_VALID_TARGETS;
            else
            {
                for (Player::GUIDTimestampMap::iterator itr = summonMap->begin(); itr != summonMap->end();++itr)
                    if (Creature * cr = Creature::GetCreature(*m_caster, (*itr).first))
                        if (m_caster->GetDistance(cr) > 40.0f)
                            return SPELL_FAILED_OUT_OF_RANGE;
            }
        }
    }

    // Dispel Magic - can be cast on friend target only when caster has aura Absolution
    // also check if target has anything to dispel
    if (m_spellInfo->Id == 527)
    {
        // check if can dispel target
        bool enemy = !m_caster->IsFriendlyTo(Target);
        if (!Target || (!enemy && m_caster != Target))      // without aura caster can dispel only self and enemies
        {
            if (!m_caster->HasAura(33167))
                return SPELL_FAILED_TARGET_FRIENDLY;
        }
    }

    // Archaeology project check - reagents / keystones
    if (m_spellInfo->researchProjectId)
    {
        if (m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->HasSpell(m_spellInfo->Id))
        {
            // Use "invalid target" to disallow casting of spells with bad data
            // should not happen, but who knows...
            ResearchProjectEntry const* pProject = sResearchProjectStore.LookupEntry(m_spellInfo->researchProjectId);
            if (!pProject)
                return SPELL_FAILED_BAD_TARGETS;

            ResearchBranchEntry const* pBranch = sResearchBranchStore.LookupEntry(pProject->researchBranch);
            if (!pBranch)
                return SPELL_FAILED_BAD_TARGETS;

            if (!m_caster->ToPlayer()->HasResearchProject(m_spellInfo->researchProjectId))
                return SPELL_FAILED_NOT_READY;

            // Disallow using more than maximum keystones
            if (pProject->keyStonesNeeded < m_keyStonesCount)
                m_keyStonesCount = pProject->keyStonesNeeded;

            // Calculate real currency cost of solving
            // each keystone supersedes 12 fragments
            uint32 realCurrencyCost = pProject->fragmentsNeeded - m_keyStonesCount*12;

            // Continue only if caster has needed items and currencies
            if (m_caster->ToPlayer()->GetCurrency(pBranch->currencyId) < realCurrencyCost ||
                m_caster->ToPlayer()->GetItemCount(pBranch->keyStoneId, false) < m_keyStonesCount)
                return SPELL_FAILED_REAGENTS;
        }
        else
            return SPELL_FAILED_ERROR;
    }

    // Disallow using any spell in map "759"
    if (m_caster->GetMapId() == 759
        && ((m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->ToPlayer()->GetSession()->GetSecurity() == SEC_PLAYER)
        || m_caster->ToPet()))
    {
        static const uint32 allowed_spells[] = // Hearthstone, Stuck, All mage portals
        {
            8690, 7355, 53142, 11419, 32266, 11416, 11417, 35717, 33691, 32267, 49361, 10059,
            49360, 88345, 88346, 11418,53141,17608, 32268, 44089, 17609, 184594, 33728, 32270,
            49363, 17334, 49362, 17610, 88339, 88341, 17611, 8326, 15007
        };

        for (uint32 i = 0;i < sizeof(allowed_spells) / sizeof(uint32); i++)
        {
            if (m_spellInfo->Id == allowed_spells[i])
                return SPELL_CAST_OK;
        }

         return SPELL_FAILED_NOT_HERE;
    }

    // Check Noggenfogger Elixir cast in BG or Arenas (Special effect Dummy)
    if (m_spellInfo->Id == 16589 && m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->GetMap()->IsBattlegroundOrArena())
        return SPELL_FAILED_NOT_HERE;

    // Explicitly allow usage of spell Dropping Heavy Bomb used for quest Mission: Abyssal Shelf only when riding taxi
    if (m_spellInfo->Id == 33836 && (m_caster->GetTypeId() != TYPEID_PLAYER || !m_caster->ToPlayer()->IsInFlight()))
        return SPELL_FAILED_NOT_HERE;

    // Do not allow to use gameobject which was already used and is going to disappear on next update
    GameObject *go = m_targets.getGOTarget();
    if (go && go->GetGoType() == GAMEOBJECT_TYPE_GOOBER && go->getLootState() != GO_READY)
        return SPELL_FAILED_CHEST_IN_USE;

    // Check add guild bank condition
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; i++)
    {
        if (m_spellInfo->Effect[i] == SPELL_EFFECT_ACTIVATE_GUILD_BANK_SLOT)
        {
            // Caster needs to be player
            if (!m_caster->ToPlayer())
                return SPELL_FAILED_ERROR;

            // ..and also is needed to be in a guild
            if (m_caster->ToPlayer()->GetGuildId() == 0)
                return SPELL_FAILED_ERROR;

            // ..and also the guild must exist
            Guild* pGuild = sObjectMgr->GetGuildById(m_caster->ToPlayer()->GetGuildId());
            if (!pGuild)
                return SPELL_FAILED_ERROR;

            // ..and also the caster has to be guild master (rank 0)
            int32 rankId = pGuild->GetMemberRankId(m_caster->GetGUID());
            if (rankId != 0)
                return SPELL_FAILED_ERROR;

            // ..and also can not buy slot which is already bought
            // (minus 1 because of buying 7th slot which is in fact at 6th position in 0..6 array)
            if (pGuild->_GetPurchasedTabsSize() != m_spellInfo->EffectBasePoints[i]-1)
                return SPELL_FAILED_ERROR;
        }
    }

    // Check for valid hunter pet spells
    if (m_caster && m_caster->IsPet() && m_caster->ToPet() && m_caster->ToPet()->IsHunterPet())
    {
        if (m_spellInfo->SpellFamilyName != SPELLFAMILY_GENERIC && m_spellInfo->SpellFamilyName != SPELLFAMILY_HUNTER && m_spellInfo->SpellFamilyName != SPELLFAMILY_PET)
            return SPELL_FAILED_NOT_KNOWN;
    }

    // check death state
    if (!m_IsTriggeredSpell && !m_caster->IsAlive() && !(m_spellInfo->Attributes & SPELL_ATTR0_PASSIVE) && !(m_spellInfo->Attributes & SPELL_ATTR0_CASTABLE_WHILE_DEAD))
        return SPELL_FAILED_CASTER_DEAD;

    // check cooldowns to prevent cheating
    if (m_caster->GetTypeId() == TYPEID_PLAYER && !(m_spellInfo->Attributes & SPELL_ATTR0_PASSIVE))
    {
        //can cast triggered (by aura only?) spells while have this flag
        if (!m_IsTriggeredSpell && m_caster->ToPlayer()->HasFlag(PLAYER_FLAGS, PLAYER_ALLOW_ONLY_ABILITY))
            return SPELL_FAILED_SPELL_IN_PROGRESS;

        if (m_caster->ToPlayer()->HasSpellCooldown(m_spellInfo->Id) ||
            (strict && !m_IsTriggeredSpell && m_caster->ToPlayer()->HasGlobalCooldown(m_spellInfo)))
        {
            if (m_triggeredByAuraSpell)
                return SPELL_FAILED_DONT_REPORT;
            else
                return SPELL_FAILED_NOT_READY;
        }
    }

    // only allow triggered spells if at an ended battleground
    if (!m_IsTriggeredSpell && m_caster->GetTypeId() == TYPEID_PLAYER)
        if (Battleground * bg = m_caster->ToPlayer()->GetBattleground())
            if (bg->GetStatus() == STATUS_WAIT_LEAVE)
                return SPELL_FAILED_DONT_REPORT;

    if(m_caster->GetTypeId() == TYPEID_PLAYER && VMAP::VMapFactory::createOrGetVMapManager()->isLineOfSightCalcEnabled())
    {
        if(m_spellInfo->Attributes & SPELL_ATTR0_OUTDOORS_ONLY &&
                !m_caster->GetMap()->IsOutdoors(m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ()))
            return SPELL_FAILED_ONLY_OUTDOORS;

        if(m_spellInfo->Attributes & SPELL_ATTR0_INDOORS_ONLY &&
                m_caster->GetMap()->IsOutdoors(m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ()))
            return SPELL_FAILED_ONLY_INDOORS;
    }

    // only check at first call, Stealth auras are already removed at second call
    // for now, ignore triggered spells
    if (strict && !m_IsTriggeredSpell)
    {
        bool checkForm = true;
        // Ignore form req aura
        Unit::AuraEffectList const& ignore = m_caster->GetAuraEffectsByType(SPELL_AURA_MOD_IGNORE_SHAPESHIFT);
        for (Unit::AuraEffectList::const_iterator i = ignore.begin(); i != ignore.end(); ++i)
        {
            if (!(*i)->IsAffectedOnSpell(m_spellInfo))
                continue;
            checkForm = false;
            break;
        }
        if (checkForm)
        {
            // Cannot be used in this stance/form
            SpellCastResult shapeError = GetErrorAtShapeshiftedCast(m_spellInfo, m_caster->GetShapeshiftForm());
            if (shapeError != SPELL_CAST_OK)
                return shapeError;

            if ((m_spellInfo->Attributes & SPELL_ATTR0_ONLY_STEALTHED) && !(m_caster->HasStealthAura()))
                return SPELL_FAILED_ONLY_STEALTHED;
        }
    }

    bool reqCombat=true;
    Unit::AuraEffectList const& stateAuras = m_caster->GetAuraEffectsByType(SPELL_AURA_ABILITY_IGNORE_AURASTATE);
    for (Unit::AuraEffectList::const_iterator j = stateAuras.begin(); j != stateAuras.end(); ++j)
    {
        if ((*j)->IsAffectedOnSpell(m_spellInfo))
        {
            m_needComboPoints = false;
            if ((*j)->GetMiscValue() == 1)
            {
                reqCombat=false;
                break;
            }
        }
    }

    // caster state requirements
    // not for triggered spells (needed by execute)
    if (!m_IsTriggeredSpell)
    {
        if (m_spellInfo->CasterAuraState && !m_caster->HasAuraState(AuraState(m_spellInfo->CasterAuraState), m_spellInfo, m_caster))
            return SPELL_FAILED_CASTER_AURASTATE;
        if (m_spellInfo->CasterAuraStateNot && m_caster->HasAuraState(AuraState(m_spellInfo->CasterAuraStateNot), m_spellInfo, m_caster))
            return SPELL_FAILED_CASTER_AURASTATE;

        // Note: spell 62473 requres casterAuraSpell = triggering spell
        if (m_spellInfo->casterAuraSpell && !m_caster->HasAura(m_spellInfo->casterAuraSpell))
            return SPELL_FAILED_CASTER_AURASTATE;
        if (m_spellInfo->excludeCasterAuraSpell && m_caster->HasAura(m_spellInfo->excludeCasterAuraSpell))
            return SPELL_FAILED_CASTER_AURASTATE;

        if (reqCombat && m_caster->IsInCombat() && IsNonCombatSpell(m_spellInfo))
            return SPELL_FAILED_AFFECTING_COMBAT;
    }

    // cancel autorepeat spells if cast start when moving
    // (not wand currently autorepeat cast delayed to moving stop anyway in spell update code)
    if (m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->ToPlayer()->isMoving())
    {
        // skip stuck spell to allow use it in falling case and apply spell limitations at movement
        if ((!m_caster->HasUnitMovementFlag(MOVEMENTFLAG_FALLING) || m_spellInfo->Effect[0] != SPELL_EFFECT_STUCK) &&
            (IsAutoRepeat() || (m_spellInfo->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED) != 0) && 
                !m_caster->CanCastWhileWalking(m_spellInfo))
            return SPELL_FAILED_MOVING;
    }

    Unit *target = m_targets.getUnitTarget();

    // In pure self-cast spells, the client won't send any unit target
    if (!target && (m_targets.getTargetMask() == TARGET_FLAG_SELF || m_targets.getTargetMask() & TARGET_FLAG_UNIT_CASTER)) // TARGET_FLAG_SELF == 0, remember!
        target = m_caster;

    if (target)
    {
        // target state requirements (not allowed state), apply to self also
        if (!m_IsTriggeredSpell && m_spellInfo->TargetAuraStateNot && target->HasAuraState(AuraState(m_spellInfo->TargetAuraStateNot), m_spellInfo, m_caster))
            return SPELL_FAILED_TARGET_AURASTATE;

        if (m_spellInfo->targetAuraSpell && !target->HasAura(m_spellInfo->targetAuraSpell))
            return SPELL_FAILED_TARGET_AURASTATE;

        if (m_spellInfo->excludeTargetAuraSpell && target->HasAura(m_spellInfo->excludeTargetAuraSpell))
            return SPELL_FAILED_TARGET_AURASTATE;

        if (!m_IsTriggeredSpell && target == m_caster && m_spellInfo->AttributesEx & SPELL_ATTR1_CANT_TARGET_SELF && m_spellInfo->Id != 70940)
            return SPELL_FAILED_BAD_TARGETS;

        bool non_caster_target = target != m_caster && !sSpellMgr->IsSpellWithCasterSourceTargetsOnly(m_spellInfo);

        if (non_caster_target)
        {
            // target state requirements (apply to non-self only), to allow cast affects to self like Dirty Deeds
            if (!m_IsTriggeredSpell && m_spellInfo->TargetAuraState && !target->HasAuraState(AuraState(m_spellInfo->TargetAuraState), m_spellInfo, m_caster))
                return SPELL_FAILED_TARGET_AURASTATE;

            // Not allow casting on flying player
            if (target->HasUnitState(UNIT_STATE_UNATTACKABLE))
                return SPELL_FAILED_BAD_TARGETS;

            if (!m_IsTriggeredSpell && (target->HasAuraType(SPELL_AURA_MOD_STEALTH)
                || target->m_invisibilityMask) && !m_caster->canSeeOrDetect(target, true))
                return SPELL_FAILED_BAD_TARGETS;

            if (m_caster->GetTypeId() == TYPEID_PLAYER)
            {
                // Do not allow to banish target tapped by someone not in caster's group
                if (m_spellInfo->Mechanic == MECHANIC_BANISH)
                    if (Creature *targetCreature = target->ToCreature())
                        if (targetCreature->hasLootRecipient() && !targetCreature->isTappedBy(m_caster->ToPlayer()))
                            return SPELL_FAILED_CANT_CAST_ON_TAPPED;

                if (m_customAttr & SPELL_ATTR0_CU_PICKPOCKET)
                {
                     if (target->GetTypeId() == TYPEID_PLAYER)
                         return SPELL_FAILED_BAD_TARGETS;
                     else if ((target->GetCreatureTypeMask() & CREATURE_TYPEMASK_HUMANOID_OR_UNDEAD) == 0)
                         return SPELL_FAILED_TARGET_NO_POCKETS;
                }

                // Not allow disarm unarmed player
                if (m_spellInfo->Mechanic == MECHANIC_DISARM)
                {
                    if (target->GetTypeId() == TYPEID_PLAYER)
                    {
                        Player *player = target->ToPlayer();
                        if (!player->GetWeaponForAttack(BASE_ATTACK) || !player->IsUseEquipedWeapon(true))
                            return SPELL_FAILED_TARGET_NO_WEAPONS;
                    }
                    else if (!target->GetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID))
                        return SPELL_FAILED_TARGET_NO_WEAPONS;
                }
            }
            
            if (!m_IsTriggeredSpell && !(m_spellInfo->AttributesEx2 & SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS))
            {
                if (m_caster->GetEntry() != WORLD_TRIGGER) // Ignore LOS for gameobjects casts (wrongly casted by a trigger)
                {
                    if (VMAP::VMapFactory::checkSpellForLoS(m_spellInfo->Id) && VMAP::VMapFactory::checkAreaForLoS(m_caster->GetAreaId()))
                    {
                        bool inLoS;

                        // If the player is in battleground, we use reduced LoS check to avoid striking through walls
                        if (m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->ToPlayer()->GetBattleground())
                            inLoS = m_caster->IsWithinLOSInMap(target, true);
                        else
                            inLoS = m_caster->IsWithinLOSInMap(target, false);

                        if (!inLoS)
                            return SPELL_FAILED_LINE_OF_SIGHT;
                    }
                }
                if (m_caster->IsVisionObscured(target))
                {
                    // Camouflage should be ripped by melee abilities
                    if (m_spellInfo->DmgClass != SPELL_DAMAGE_CLASS_MELEE || ((m_spellInfo->EquippedItemSubClassMask & ITEM_SUBCLASS_MASK_WEAPON_RANGED) != 0) || !target->isCamouflaged())
                        return SPELL_FAILED_VISION_OBSCURED; // smoke bomb, camouflage...
                }
            }

        }
        else if (m_caster == target)
        {
            if (m_caster->GetTypeId() == TYPEID_PLAYER) // Target - is player caster
            {
                // Additional check for some spells
                // If 0 spell effect empty - client not send target data (need use selection)
                // TODO: check it on next client version
                if (m_targets.getTargetMask() == TARGET_FLAG_SELF &&
                    m_spellInfo->EffectImplicitTargetA[1] == TARGET_UNIT_TARGET_ENEMY)
                {
                    target = m_caster->GetUnit(*m_caster, m_caster->ToPlayer()->GetSelection());
                    if (target)
                        m_targets.setUnitTarget(target);
                    else
                        return SPELL_FAILED_BAD_TARGETS;
                }
                // Lay on Hands - cannot be self-cast on paladin with Forbearance or after using Avenging Wrath
                if (m_spellInfo->SpellFamilyName == SPELLFAMILY_PALADIN && m_spellInfo->SpellFamilyFlags[0] & 0x0008000)
                    if (target->HasAura(25771)) // Immunity shield marker
                        return SPELL_FAILED_TARGET_AURASTATE;
            }
        }

        // check pet presents
        for (int j = 0; j < MAX_SPELL_EFFECTS; ++j)
        {
            if (m_spellInfo->EffectImplicitTargetA[j] == TARGET_UNIT_PET)
            {
                target = m_caster->GetGuardianPet();
                if (!target)
                {
                    if (m_triggeredByAuraSpell)              // not report pet not existence for triggered spells
                        return SPELL_FAILED_DONT_REPORT;
                    else
                        return SPELL_FAILED_NO_PET;
                }
                break;
            }
        }

        //check creature type
        //ignore self casts (including area casts when caster selected as target)
        if (non_caster_target)
        {
            if (!CheckTargetCreatureType(target))
            {
                if (target->GetTypeId() == TYPEID_PLAYER)
                    return SPELL_FAILED_TARGET_IS_PLAYER;
                else
                    return SPELL_FAILED_BAD_TARGETS;
            }
        }

        // who can give me an example to show what is the use of this
        // even if we need check, check by effect rather than whole spell, otherwise 57108,57143 are broken
        /*
        // TODO: this check can be applied and for player to prevent cheating when IsPositiveSpell will return always correct result.
        // check target for pet/charmed casts (not self targeted), self targeted cast used for area effects and etc
        if (non_caster_target && m_caster->GetTypeId() == TYPEID_UNIT && m_caster->GetCharmerOrOwnerGUID())
        {
            // check correctness positive/negative cast target (pet cast real check and cheating check)
            if (IsPositiveSpell(m_spellInfo->Id))
            {
                                                     //dispel positivity is dependant on target, don't check it
                if (m_caster->IsHostileTo(target) && !IsDispel(m_spellInfo))
                    return SPELL_FAILED_BAD_TARGETS;
            }
            else
            {
                if (m_caster->IsFriendlyTo(target))
                    return SPELL_FAILED_BAD_TARGETS;
            }
        }
        */

        if (target)
            if (IsPositiveSpell(m_spellInfo->Id))
                if (target->IsImmunedToSpell(m_spellInfo))
                    return SPELL_FAILED_TARGET_AURASTATE;

        //Must be behind the target.
        if (m_spellInfo->AttributesEx2 == SPELL_ATTR2_UNK20 && m_spellInfo->AttributesEx & SPELL_ATTR1_MELEE_COMBAT_START && target->HasInArc(static_cast<float>(M_PI), m_caster)
            //Exclusion for Pounce: Facing Limitation was removed in 2.0.1, but it still uses the same, old Ex-Flags
            && (!(m_spellInfo->SpellFamilyName == SPELLFAMILY_DRUID && m_spellInfo->SpellFamilyFlags.IsEqual(0x20000,0,0)))
            //Mutilate no longer requires you be behind the target as of patch 3.0.3
            && (!(m_spellInfo->SpellFamilyName == SPELLFAMILY_ROGUE && m_spellInfo->SpellFamilyFlags[1] & 0x200000))
            //Exclusion for Throw: Facing limitation was added in 3.2.x, but that shouldn't be
            && (!(m_spellInfo->SpellFamilyName == SPELLFAMILY_ROGUE && m_spellInfo->SpellFamilyFlags[0] & 0x00000001))
            //Ravage! (Stampede) - feral druid
            && (!(m_spellInfo->Id == 81170)))
        {
            SendInterrupted(2);
            return SPELL_FAILED_NOT_BEHIND;
        }

        //Target must be facing you.
        if (((m_spellInfo->Attributes == (SPELL_ATTR0_ABILITY | SPELL_ATTR0_NOT_SHAPESHIFT | SPELL_ATTR0_DONT_AFFECT_SHEATH_STATE | SPELL_ATTR0_STOP_ATTACK_TARGET)) || (m_spellInfo->Id == 1776 && !m_caster->HasAura(56809))) && !target->HasInArc(static_cast<float>(M_PI), m_caster))
        {
            SendInterrupted(2);
            return SPELL_FAILED_NOT_INFRONT;
        }

        // check if target is in combat
        if (non_caster_target && (m_spellInfo->AttributesEx & SPELL_ATTR1_NOT_IN_COMBAT_TARGET) && target->IsInCombat())
            return SPELL_FAILED_TARGET_AFFECTING_COMBAT;
    }

    // Spell casted only on battleground
    if ((m_spellInfo->AttributesEx3 & SPELL_ATTR3_BATTLEGROUND) &&  m_caster->GetTypeId() == TYPEID_PLAYER)
        if (!m_caster->ToPlayer()->InBattleground())
            return SPELL_FAILED_ONLY_BATTLEGROUNDS;

    // do not allow spells to be cast in arenas
    // - with greater than 10 min CD without SPELL_ATTR4_USABLE_IN_ARENA flag
    // - with SPELL_ATTR4_NOT_USABLE_IN_ARENA flag
    if ((m_spellInfo->AttributesEx4 & SPELL_ATTR4_NOT_USABLE_IN_ARENA) ||
        (GetSpellRecoveryTime(m_spellInfo) > 10 * MINUTE * IN_MILLISECONDS && !(m_spellInfo->AttributesEx4 & SPELL_ATTR4_USABLE_IN_ARENA)))
        if (MapEntry const* mapEntry = sMapStore.LookupEntry(m_caster->GetMapId()))
            if (mapEntry->IsBattleArena())
                return SPELL_FAILED_NOT_IN_ARENA;

    // Have Group, Will Travel - do not use inside instances
    if (m_spellInfo->Id == 83967)
        if (MapEntry const* mapEntry = sMapStore.LookupEntry(m_caster->GetMapId()))
            if (mapEntry->IsDungeon() || mapEntry->IsRaid() || mapEntry->Instanceable())
                return SPELL_FAILED_NOT_HERE;

    // Special cases for spells which cannot be casted in arenas
    if (MapEntry const* mapEntry = sMapStore.LookupEntry(m_caster->GetMapId()))
    {
        if (mapEntry->IsBattleArena())
        {
            switch (m_spellInfo->Id)
            {
                case 2825:  // Bloodlust
                case 32182: // Heroism
                case 80353: // Time Warp
                case 90355: // Ancient Hysteria
                    return SPELL_FAILED_NOT_IN_ARENA;
                default:
                    break;
            }
        }
        else if (mapEntry->IsBattleground())
        {
            switch (m_spellInfo->Id)
            {
                case 698: // Ritual of Summoning
                    return SPELL_FAILED_NOT_IN_BATTLEGROUND;
            }
        }
    }

    // zone check
    if (m_caster->GetTypeId() == TYPEID_UNIT || !m_caster->ToPlayer()->IsGameMaster())
    {
        uint32 zone, area;
        m_caster->GetZoneAndAreaId(zone,area);

        SpellCastResult locRes= sSpellMgr->GetSpellAllowedInLocationError(m_spellInfo,m_caster->GetMapId(),zone,area,
            m_caster->GetTypeId() == TYPEID_PLAYER ? m_caster->ToPlayer() : NULL);
        if (locRes != SPELL_CAST_OK)
            return locRes;
    }

    // not let players cast spells at mount (and let do it to creatures)
    if (m_caster->IsMounted() && m_caster->GetTypeId() == TYPEID_PLAYER && !m_IsTriggeredSpell &&
        !IsPassiveSpell(m_spellInfo->Id) && !(m_spellInfo->Attributes & SPELL_ATTR0_CASTABLE_WHILE_MOUNTED))
    {
        if (m_caster->IsInFlight())
            return SPELL_FAILED_NOT_ON_TAXI;
        else
            return SPELL_FAILED_NOT_MOUNTED;
    }

    SpellCastResult castResult = SPELL_CAST_OK;

    // always (except passive spells) check items (focus object can be required for any type casts)
    if (!IsPassiveSpell(m_spellInfo->Id))
    {
        castResult = CheckItems();
        if (castResult != SPELL_CAST_OK)
            return castResult;
    }

    // Triggered spells also have range check
    // TODO: determine if there is some flag to enable/disable the check
    castResult = CheckRange(strict);
    if (castResult != SPELL_CAST_OK)
        return castResult;

    if (!m_IsTriggeredSpell)
    {
        castResult = CheckPower();
        if (castResult != SPELL_CAST_OK)
            return castResult;

        castResult = CheckCasterAuras();
        if (castResult != SPELL_CAST_OK)
            return castResult;
    }

    for (int i = 0; i < MAX_SPELL_EFFECTS; i++)
    {
        // for effects of spells that have only one target
        switch(m_spellInfo->Effect[i])
        {
            case SPELL_EFFECT_DUMMY:
            {
                if (m_spellInfo->Id == 51582)          // Rocket Boots Engaged
                {
                    if (m_caster->IsInWater())
                        return SPELL_FAILED_ONLY_ABOVEWATER;
                }
                else if (m_spellInfo->SpellIconID == 156)    // Holy Shock
                {
                    // spell different for friends and enemies
                    // hurt version required facing
                    if (m_targets.getUnitTarget() && !m_caster->IsFriendlyTo(m_targets.getUnitTarget()) && !m_caster->HasInArc(static_cast<float>(M_PI), m_targets.getUnitTarget()))
                        return SPELL_FAILED_UNIT_NOT_INFRONT;
                }
                else if (m_spellInfo->SpellIconID == 33 && m_spellInfo->SpellFamilyName == SPELLFAMILY_SHAMAN && m_spellInfo->SpellFamilyFlags[0] & SPELLFAMILYFLAG_SHAMAN_FIRE_NOVA)
                {
                    if (!m_caster->m_SummonSlot[1])
                        return SPELL_FAILED_SUCCESS;
                }
                else if (m_spellInfo->SpellFamilyName == SPELLFAMILY_DEATHKNIGHT && m_spellInfo->SpellFamilyFlags[0] == 0x2000) // Death Coil (DeathKnight)
                {
                    Unit* target = m_targets.getUnitTarget();
                    if (!target || (target->IsFriendlyTo(m_caster) && target->GetCreatureType() != CREATURE_TYPE_UNDEAD))
                        return SPELL_FAILED_BAD_TARGETS;
                    if (!target->IsFriendlyTo(m_caster) && !m_caster->HasInArc(static_cast<float>(M_PI), target))
                        return SPELL_FAILED_UNIT_NOT_INFRONT;
                }
                else if (m_spellInfo->Id == 19938)          // Awaken Peon
                {
                    Unit *unit = m_targets.getUnitTarget();
                    if (!unit || !unit->HasAura(17743))
                        return SPELL_FAILED_BAD_TARGETS;
                }
                else if (m_spellInfo->Id == 52264)          // Deliver Stolen Horse
                {
                    if (!m_caster->FindNearestCreature(28653,5))
                        return SPELL_FAILED_OUT_OF_RANGE;
                }
                else if (m_spellInfo->Id == 31789)          // Righteous Defense
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                        return SPELL_FAILED_DONT_REPORT;

                    Unit* target = m_targets.getUnitTarget();
                    if (!target || !target->IsFriendlyTo(m_caster) || target->getAttackers().empty())
                        return SPELL_FAILED_BAD_TARGETS;

                }
                else if (m_spellInfo->Id == 61999)          // Raise Ally
                {
                    Unit* target = m_targets.getUnitTarget();
                    if (!(target && target->isDead() && target->GetTypeId() == TYPEID_PLAYER && m_caster->GetTypeId() == TYPEID_PLAYER &&
                        (target->ToPlayer()->IsInPartyWith(m_caster) || target->ToPlayer()->IsInRaidWith(m_caster))))
                        return SPELL_FAILED_BAD_TARGETS;
                }
                break;
            }
            case SPELL_EFFECT_LEARN_SPELL:
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    return SPELL_FAILED_BAD_TARGETS;

                if (m_spellInfo->EffectImplicitTargetA[i] != TARGET_UNIT_PET)
                    break;

                Pet* pet = m_caster->ToPlayer()->GetPet();

                if (!pet)
                    return SPELL_FAILED_NO_PET;

                SpellEntry const *learn_spellproto = sSpellStore.LookupEntry(m_spellInfo->EffectTriggerSpell[i]);

                if (!learn_spellproto)
                    return SPELL_FAILED_NOT_KNOWN;

                if (m_spellInfo->spellLevel > pet->getLevel())
                    return SPELL_FAILED_LOWLEVEL;

                break;
            }
            case SPELL_EFFECT_LEARN_PET_SPELL:
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    return SPELL_FAILED_BAD_TARGETS;

                Pet* pet = m_caster->ToPlayer()->GetPet();
                if (!pet)
                    return SPELL_FAILED_NO_PET;

                SpellEntry const *learn_spellproto = sSpellStore.LookupEntry(m_spellInfo->EffectTriggerSpell[i]);

                if (!learn_spellproto)
                    return SPELL_FAILED_NOT_KNOWN;

                if (m_spellInfo->spellLevel > pet->getLevel())
                    return SPELL_FAILED_LOWLEVEL;

                break;
            }
            case SPELL_EFFECT_APPLY_GLYPH:
            {
                uint32 glyphId = m_spellInfo->EffectMiscValue[i];
                if (GlyphPropertiesEntry const *gp = sGlyphPropertiesStore.LookupEntry(glyphId))
                    if (m_caster->HasAura(gp->SpellId))
                        return SPELL_FAILED_UNIQUE_GLYPH;
                break;
            }
            case SPELL_EFFECT_FEED_PET:
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    return SPELL_FAILED_BAD_TARGETS;

                Item* foodItem = m_targets.getItemTarget();
                if (!foodItem)
                    return SPELL_FAILED_BAD_TARGETS;

                Pet* pet = m_caster->ToPlayer()->GetPet();

                if (!pet)
                    return SPELL_FAILED_NO_PET;

                if (!pet->HaveInDiet(foodItem->GetProto()))
                    return SPELL_FAILED_WRONG_PET_FOOD;

                if (!pet->GetCurrentFoodBenefitLevel(foodItem->GetProto()->ItemLevel))
                    return SPELL_FAILED_FOOD_LOWLEVEL;

                if (m_caster->IsInCombat() || pet->IsInCombat())
                    return SPELL_FAILED_AFFECTING_COMBAT;

                break;
            }
            case SPELL_EFFECT_POWER_BURN:
            case SPELL_EFFECT_POWER_DRAIN:
            {
                // Can be area effect, Check only for players and not check if target - caster (spell can have multiply drain/burn effects)
                if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    if (Unit* target = m_targets.getUnitTarget())
                        if (target != m_caster && target->getPowerType() != Powers(m_spellInfo->EffectMiscValue[i]))
                            return SPELL_FAILED_BAD_TARGETS;
                break;
            }
            case SPELL_EFFECT_CHARGE:
            {
                if (m_spellInfo->SpellFamilyName == SPELLFAMILY_WARRIOR)
                {
                    // Warbringer - can't be handled in proc system - should be done before checkcast root check and charge effect process
                    if (strict && m_caster->IsScriptOverriden(m_spellInfo, 6953))
                        m_caster->RemoveMovementImpairingAuras();
                }

                if (m_caster->HasUnitState(UNIT_STATE_ROOT))
                    return SPELL_FAILED_ROOTED;

                Unit* target = m_targets.getUnitTarget();

                if (!target)
                    return SPELL_FAILED_DONT_REPORT;

                Position pos;
                target->GetFirstCollisionPosition(pos, CONTACT_DISTANCE, target->GetRelativeAngle(m_caster));

                m_preGeneratedPath.SetPathLengthLimit(GetSpellMaxRange(m_spellInfo, true) * 12.0f);
                bool result = m_preGeneratedPath.CalculatePath(pos.m_positionX, pos.m_positionY, pos.m_positionZ, false, false);
                if (m_preGeneratedPath.GetPathType() & PATHFIND_SHORT)
                    return SPELL_FAILED_OUT_OF_RANGE;
                else if (!result)
                    return SPELL_FAILED_NOPATH;
                break;
            }
            case SPELL_EFFECT_SKINNING:
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER || !m_targets.getUnitTarget() || m_targets.getUnitTarget()->GetTypeId() != TYPEID_UNIT)
                    return SPELL_FAILED_BAD_TARGETS;

                if (!(m_targets.getUnitTarget()->GetUInt32Value(UNIT_FIELD_FLAGS) & UNIT_FLAG_SKINNABLE))
                    return SPELL_FAILED_TARGET_UNSKINNABLE;

                Creature* creature = m_targets.getUnitTarget()->ToCreature();
                if (creature->GetCreatureType() != CREATURE_TYPE_CRITTER && !creature->loot.isLooted())
                    return SPELL_FAILED_TARGET_NOT_LOOTED;

                uint32 skill = creature->GetCreatureInfo()->GetRequiredLootSkill();

                int32 skillValue = m_caster->ToPlayer()->GetSkillValue(skill);
                int32 TargetLevel = m_targets.getUnitTarget()->getLevel();
                int32 ReqValue = (skillValue < 100 ? (TargetLevel-10) * 10 : TargetLevel * 5);
                if (ReqValue > skillValue)
                    return SPELL_FAILED_LOW_CASTLEVEL;

                // chance for fail at orange skinning attempt
                if ((m_selfContainer && (*m_selfContainer) == this) &&
                    skillValue < sWorld->GetConfigMaxSkillValue() &&
                    (ReqValue < 0 ? 0 : ReqValue) > irand(skillValue - 25, skillValue + 37))
                    return SPELL_FAILED_TRY_AGAIN;

                break;
            }
            case SPELL_EFFECT_OPEN_LOCK:
            {
                if (m_spellInfo->EffectImplicitTargetA[i] != TARGET_GAMEOBJECT &&
                    m_spellInfo->EffectImplicitTargetA[i] != TARGET_GAMEOBJECT_ITEM)
                    break;

                if (m_caster->GetTypeId() != TYPEID_PLAYER  // only players can open locks, gather etc.
                    // we need a go target in case of TARGET_GAMEOBJECT
                    || (m_spellInfo->EffectImplicitTargetA[i] == TARGET_GAMEOBJECT && !m_targets.getGOTarget()))
                    return SPELL_FAILED_BAD_TARGETS;

                Item *pTempItem = NULL;
                if (m_targets.getTargetMask() & TARGET_FLAG_TRADE_ITEM)
                {
                    if (TradeData* pTrade = m_caster->ToPlayer()->GetTradeData())
                        pTempItem = pTrade->GetTraderData()->GetItem(TradeSlots(m_targets.getItemTargetGUID()));
                }
                else if (m_targets.getTargetMask() & TARGET_FLAG_ITEM)
                    pTempItem = m_caster->ToPlayer()->GetItemByGuid(m_targets.getItemTargetGUID());

                // we need a go target, or an openable item target in case of TARGET_GAMEOBJECT_ITEM
                if (m_spellInfo->EffectImplicitTargetA[i] == TARGET_GAMEOBJECT_ITEM &&
                    !m_targets.getGOTarget() &&
                    (!pTempItem || !pTempItem->GetProto()->LockID || !pTempItem->IsLocked()))
                    return SPELL_FAILED_BAD_TARGETS;

                if (m_spellInfo->Id != 1842 || (m_targets.getGOTarget() &&
                    m_targets.getGOTarget()->GetGOInfo()->type != GAMEOBJECT_TYPE_TRAP))
                    if (m_caster->ToPlayer()->InBattleground())
                    {
                        m_caster->ToPlayer()->RemoveAurasDueToBattlegroundObjectUse();
                        // In Battleground players can use only flags and banners
                        if (!m_caster->ToPlayer()->CanUseBattlegroundObject())
                            return SPELL_FAILED_TRY_AGAIN;
                    }

                // get the lock entry
                uint32 lockId = 0;
                if (GameObject* go = m_targets.getGOTarget())
                {
                    lockId = go->GetGOInfo()->GetLockId();
                    if (!lockId)
                        return SPELL_FAILED_BAD_TARGETS;
                }
                else if (Item* itm = m_targets.getItemTarget())
                    lockId = itm->GetProto()->LockID;

                SkillType skillId = SKILL_NONE;
                int32 reqSkillValue = 0;
                int32 skillValue = 0;

                // check lock compatibility
                SpellCastResult res = CanOpenLock(i, lockId, skillId, reqSkillValue, skillValue);
                if (res != SPELL_CAST_OK)
                    return res;

                // chance for fail at orange mining/herb/LockPicking gathering attempt
                // second check prevent fail at rechecks
                if (skillId != SKILL_NONE && (!m_selfContainer || ((*m_selfContainer) != this)))
                {
                    bool canFailAtMax = skillId != SKILL_HERBALISM && skillId != SKILL_MINING;

                    // chance for failure in orange gather / lockpick (gathering skill can't fail at maxskill)
                    if ((canFailAtMax || skillValue < sWorld->GetConfigMaxSkillValue()) && reqSkillValue > irand(skillValue - 25, skillValue + 37))
                        return SPELL_FAILED_TRY_AGAIN;
                }
                break;
            }
            case SPELL_EFFECT_SUMMON_DEAD_PET:
            {
                Creature *pet = m_caster->GetGuardianPet();
                if (!pet)
                    return SPELL_FAILED_NO_PET;

                if (pet->IsAlive())
                    return SPELL_FAILED_ALREADY_HAVE_SUMMON;

                break;
            }
            // This is generic summon effect
            case SPELL_EFFECT_SUMMON:
            {
                SummonPropertiesEntry const *SummonProperties = sSummonPropertiesStore.LookupEntry(m_spellInfo->EffectMiscValueB[i]);
                if (!SummonProperties)
                    break;
                switch(SummonProperties->Category)
                {
                    case SUMMON_CATEGORY_PET:
                        m_caster->RemoveAllMinionsByEntry(m_spellInfo->EffectMiscValue[i]);
                        break;
                    case SUMMON_CATEGORY_PUPPET:
                        if (m_caster->GetCharmGUID())
                            return SPELL_FAILED_ALREADY_HAVE_CHARM;
                        break;
                }
                break;
            }
            case SPELL_EFFECT_CREATE_TAMED_PET:
            {
                if (m_targets.getUnitTarget())
                {
                    if (m_targets.getUnitTarget()->GetTypeId() != TYPEID_PLAYER)
                        return SPELL_FAILED_BAD_TARGETS;
                    if (m_targets.getUnitTarget()->GetPetGUID())
                        return SPELL_FAILED_ALREADY_HAVE_SUMMON;
                }
                break;
            }
            case SPELL_EFFECT_SUMMON_PET:
            {
                if (m_caster->GetPetGUID())                  //let warlock do a replacement summon
                {
                    if (m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->getClass() == CLASS_WARLOCK)
                    {
                        if (strict)                         //starting cast, trigger pet stun (cast by pet so it doesn't attack player)
                            if (Pet* pet = m_caster->ToPlayer()->GetPet())
                                pet->CastSpell(pet, 32752, true, NULL, NULL, pet->GetGUID());
                    }
                    else
                        return SPELL_FAILED_ALREADY_HAVE_SUMMON;
                }

                if (m_caster->GetCharmGUID())
                    return SPELL_FAILED_ALREADY_HAVE_CHARM;

                break;
            }
            case SPELL_EFFECT_SUMMON_PLAYER:
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    return SPELL_FAILED_BAD_TARGETS;
                if (!m_caster->ToPlayer()->GetSelection() && m_spellInfo->Id != 85592)
                    return SPELL_FAILED_BAD_TARGETS;

                Player* target = sObjectMgr->GetPlayer(m_caster->ToPlayer()->GetSelection());
                if ((!target || m_caster->ToPlayer() == target || !target->IsInSameRaidWith(m_caster->ToPlayer())) && m_spellInfo->Id != 85592)
                    return SPELL_FAILED_BAD_TARGETS;

                // check if our map is dungeon
                if (sMapStore.LookupEntry(m_caster->GetMapId())->IsDungeon())
                {
                    Map const* pMap = m_caster->GetMap();
                    InstanceTemplate const* instance = ObjectMgr::GetInstanceTemplate(pMap->GetId());
                    if (!instance)
                        return SPELL_FAILED_TARGET_NOT_IN_INSTANCE;
                    if (!target || !target->Satisfy(sObjectMgr->GetAccessRequirement(pMap->GetId(), pMap->GetDifficulty()), pMap->GetId()))
                        return SPELL_FAILED_BAD_TARGETS;
                }
                break;
            }
            case SPELL_EFFECT_LEAP:
            case SPELL_EFFECT_TELEPORT_UNITS_FACE_CASTER:
            {
              //Do not allow to cast it before BG starts.
                if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    if (Battleground const *bg = m_caster->ToPlayer()->GetBattleground())
                        if (bg->GetStatus() != STATUS_IN_PROGRESS)
                            return SPELL_FAILED_TRY_AGAIN;
                break;
            }
            case SPELL_EFFECT_TELEPORT_UNITS:
            {
                // Mage teleports should not be able to cast in battleground
                if (m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->ToPlayer()->getClass() == CLASS_MAGE )
                    if (m_caster->ToPlayer()->GetBattleground())
                            return SPELL_FAILED_NOT_IN_BATTLEGROUND;
                break;
            }
            case SPELL_EFFECT_STEAL_BENEFICIAL_BUFF:
            {
                if (m_targets.getUnitTarget() == m_caster)
                    return SPELL_FAILED_BAD_TARGETS;
                break;
            }
            case SPELL_EFFECT_REDIRECT_THREAT:
            {
                if (m_spellInfo->Id == 57934 && m_targets.getUnitTarget() &&
                    m_targets.getUnitTarget()->GetTypeId() != TYPEID_PLAYER)
                    return SPELL_FAILED_BAD_TARGETS;
                break;
            }
            case SPELL_EFFECT_LEAP_BACK:
            {
                // Spell 781 (Disengage) requires player to be in combat
                if (m_caster->GetTypeId() == TYPEID_PLAYER && m_spellInfo->Id == 781 && !m_caster->IsInCombat())
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

                Unit* target = m_targets.getUnitTarget();
                if (m_caster == target && m_caster->HasUnitState(UNIT_STATE_ROOT))
                {
                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                        return SPELL_FAILED_ROOTED;
                    else
                        return SPELL_FAILED_DONT_REPORT;
                }
                break;
            }
            case SPELL_EFFECT_TALENT_SPEC_SELECT:
                // can't change during already started arena/battleground
                if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    if (Battleground const* bg = m_caster->ToPlayer()->GetBattleground())
                        if (bg->GetStatus() == STATUS_IN_PROGRESS)
                            return SPELL_FAILED_NOT_IN_BATTLEGROUND;
                break;
            default:
                break;
        }

        /* custom check switch */
        switch(m_spellInfo->Effect[i])
        {
            case SPELL_EFFECT_JUMP:
            case SPELL_EFFECT_JUMP_DEST:
            case SPELL_EFFECT_CHARGE_DEST:
            {
                /* allow quest-based jumps */
                if (m_spellInfo->Id == 4336 || m_spellInfo->Id == 101987)
                    break;

                /* do these checks only on players */
                if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    break;

                /* get destination position */
                Position *dpos;
                if (m_targets.HasDst())
                    dpos = dynamic_cast<Position*>(&m_targets.m_dstPos);
                else if (m_targets.getUnitTarget())
                {
                    dpos = dynamic_cast<Position*>(m_targets.getUnitTarget());
                    if (Unit* tu = m_targets.getUnitTarget())
                    {
                        // Antipersonnel Cannon cannot be charged or jumped to
                        if (tu->GetEntry() == 27894)
                            return SPELL_FAILED_BAD_TARGETS;
                    }
                }
                else if (m_targets.getGOTarget())
                    dpos = dynamic_cast<Position*>(m_targets.getGOTarget());
                else
                    break;

                /* allow only within-LOS jumps */
                if (!m_caster->IsWithinLOS(dpos->GetPositionX(), dpos->GetPositionY(), dpos->GetPositionZ()))
                    return SPELL_FAILED_LINE_OF_SIGHT;

                /* not before BG starts */
                if (Battleground const* bg = m_caster->ToPlayer()->GetBattleground())
                    if (bg->GetStatus() != STATUS_IN_PROGRESS)
                        return SPELL_FAILED_TRY_AGAIN;

                // Disabled, but left there for future developers:
                // http://wow.gamepedia.com/Charge (5.6.2015) - "It is possible to Charge mid-air, while jumping, falling or suffering knockback."

                /* not while falling (exclude triggered spells - they are probably casted by another player like Death Grip while jumping) */
                //if (m_caster->HasUnitMovementFlag(MOVEMENTFLAG_FALLING) && !IsTriggered() && m_spellInfo->Id != 6544)
                //    return SPELL_FAILED_FALLING;

                /* exclude flat ground for charge when not in BG */
                if (!m_caster->ToPlayer()->InBattleground()) {
                    if (m_spellInfo->Effect[i] == SPELL_EFFECT_CHARGE ||
                        m_spellInfo->Effect[i] == SPELL_EFFECT_CHARGE_DEST)
                        break;
                }

                /* if the spell has custom target (Heroic Leap, ..), proceed to flat ground test */
                if (m_spellInfo->EffectImplicitTargetA[i] != TARGET_DEST_DEST)
                    break;

                /* Ignore next triggered jump like checks if spell was triggered*/
                if (IsTriggered())
                    break;

                float zdelta = m_caster->GetPositionZ() - dpos->GetPositionZ();
                /* automatically allow jumps up to 2y above and any lower level (so everything under player is allowed) */
                if (zdelta > -2.0f)
                    break;

                /* destination must stand on flat ground (in players map) */
                if (!sObjectMgr->IsFlatGround(m_caster->GetMap(), dpos->GetPositionX(), dpos->GetPositionY(), dpos->GetPositionZ()))
                    return SPELL_FAILED_OUT_OF_RANGE;

                /* flat path between player and destination required */
                if (!m_caster->HasFlatPathTo(dpos))
                    return SPELL_FAILED_OUT_OF_RANGE;

                break;
            }
            default:
                break;
        }
    }

    for (int i = 0; i < MAX_SPELL_EFFECTS; i++)
    {
        switch(m_spellInfo->EffectApplyAuraName[i])
        {
            case SPELL_AURA_DUMMY:
            {
                //custom check
                switch(m_spellInfo->Id)
                {
                    // Tag Murloc
                    case 30877:
                    {
                        Unit* target = m_targets.getUnitTarget();
                        if (!target || target->GetEntry() != 17326)
                            return SPELL_FAILED_BAD_TARGETS;
                        break;
                    }
                    case 61336:
                        if (m_caster->GetTypeId() != TYPEID_PLAYER || !m_caster->ToPlayer()->IsInFeralForm())
                            return SPELL_FAILED_ONLY_SHAPESHIFT;
                        break;
                    case 1515:
                    {
                        if (m_caster->GetTypeId() != TYPEID_PLAYER)
                            return SPELL_FAILED_BAD_TARGETS;

                        if (!m_targets.getUnitTarget() || m_targets.getUnitTarget()->GetTypeId() == TYPEID_PLAYER)
                            return SPELL_FAILED_BAD_IMPLICIT_TARGETS;

                        // Check if there are to many so that you don't get mixed with pets
                        // being there from the begining
                        if (m_caster->ToPlayer()->getSlotForNewPet() == PET_SLOT_FULL_LIST)
                        {
                            m_caster->ToPlayer()->SendTooManyPets(m_caster->ToPlayer());
                            return SPELL_FAILED_NO_ACTIONS; // i havent found the right error message to use so this need to be changed
                        }

                        Creature* target = m_targets.getUnitTarget()->ToCreature();

                        if (target->getLevel() > m_caster->getLevel())
                            return SPELL_FAILED_HIGHLEVEL;

                        // use SMSG_PET_TAME_FAILURE?
                        if (!target->GetCreatureInfo()->isTameable (m_caster->ToPlayer()->CanTameExoticPets()))
                            return SPELL_FAILED_BAD_TARGETS;

                        if (m_caster->GetPetGUID())
                            return SPELL_FAILED_ALREADY_HAVE_SUMMON;

                        if (m_caster->GetCharmGUID())
                            return SPELL_FAILED_ALREADY_HAVE_CHARM;

                        break;
                    }
                    case 44795: // Parachute
                    {
                        float x, y, z;
                        m_caster->GetPosition(x, y, z);
                        float ground_Z = m_caster->GetMap()->GetHeight(m_caster->GetPhaseMask(), x, y, z);
                        if (fabs(ground_Z - z) < 0.1f)
                            return SPELL_FAILED_DONT_REPORT;
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case SPELL_AURA_MOD_POSSESS_PET:
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    return SPELL_FAILED_NO_PET;

                Pet *pet = m_caster->ToPlayer()->GetPet();
                if (!pet)
                    return SPELL_FAILED_NO_PET;

                if (pet->GetCharmerGUID())
                    return SPELL_FAILED_CHARMED;
                break;
            }
            case SPELL_AURA_MOD_POSSESS:
            case SPELL_AURA_MOD_CHARM:
            case SPELL_AURA_AOE_CHARM:
            {
                if (m_caster->GetCharmerGUID())
                    return SPELL_FAILED_CHARMED;

                if (m_spellInfo->EffectApplyAuraName[i] == SPELL_AURA_MOD_CHARM
                    || m_spellInfo->EffectApplyAuraName[i] == SPELL_AURA_MOD_POSSESS)
                {
                    if (m_caster->GetPetGUID())
                        return SPELL_FAILED_ALREADY_HAVE_SUMMON;

                    if (m_caster->GetCharmGUID())
                        return SPELL_FAILED_ALREADY_HAVE_CHARM;
                }

                if (Unit *target = m_targets.getUnitTarget())
                {
                    if (target->GetTypeId() == TYPEID_UNIT && target->ToCreature()->IsVehicle())
                        return SPELL_FAILED_BAD_IMPLICIT_TARGETS;

                    if (target->IsMounted())
                        return SPELL_FAILED_CANT_BE_CHARMED;

                    if (target->GetCharmerGUID())
                        return SPELL_FAILED_CHARMED;

                    int32 damage = CalculateDamage(i, target);
                    if (damage && int32(target->getLevel()) > damage)
                        return SPELL_FAILED_HIGHLEVEL;
                }

                break;
            }
            case SPELL_AURA_MOUNTED:
            {
                // Not castable while swimming - exception for MountCapabilities 231 and 232 - these are underwater mounts
                if (m_caster->IsInWater() && m_spellInfo->EffectMiscValueB[i] != 231 && m_spellInfo->EffectMiscValueB[i] != 232)
                    return SPELL_FAILED_ONLY_ABOVEWATER;

                // Ignore map check if spell have AreaId. AreaId already checked and this prevent special mount spells
                bool AllowMount = !m_caster->GetMap()->IsDungeon() || m_caster->GetMap()->IsBattlegroundOrArena();
                InstanceTemplate const *it = sObjectMgr->GetInstanceTemplate(m_caster->GetMapId());
                if (it)
                    AllowMount = it->allowMount;
                if (m_caster->GetTypeId() == TYPEID_PLAYER && !AllowMount && !m_IsTriggeredSpell && !m_spellInfo->AreaGroupId)
                    return SPELL_FAILED_NO_MOUNTS_ALLOWED;

                if (m_caster->IsInDisallowedMountForm())
                    return SPELL_FAILED_NOT_SHAPESHIFT;

                break;
            }
            case SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS:
            {
                if (!m_targets.getUnitTarget())
                    return SPELL_FAILED_BAD_IMPLICIT_TARGETS;

                // can be casted at non-friendly unit or own pet/charm
                if (m_caster->IsFriendlyTo(m_targets.getUnitTarget()))
                    return SPELL_FAILED_TARGET_FRIENDLY;

                break;
            }
            case SPELL_AURA_FLY:
            case SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED:
            {
                // not allow cast fly spells if not have req. skills  (all spells is self target)
                // allow always ghost flight spells
                if (m_originalCaster && m_originalCaster->GetTypeId() == TYPEID_PLAYER && m_originalCaster->IsAlive())
                {
                    Battlefield* Bf = sBattlefieldMgr.GetBattlefieldToZoneId(m_originalCaster->GetZoneId());

                    if (AreaTableEntry const* pArea = GetAreaEntryByAreaID(m_originalCaster->GetAreaId()))
                        if (m_caster->HasAuraType(SPELL_AURA_MOD_MOUNT_TYPE) || (pArea->flags & AREA_FLAG_NO_FLY_ZONE) || (Bf && !Bf->CanFlyIn()))
                            return m_IsTriggeredSpell ? SPELL_FAILED_DONT_REPORT : SPELL_FAILED_NOT_HERE;
                }
                break;
            }
            case SPELL_AURA_PERIODIC_MANA_LEECH:
            {
                if (!m_targets.getUnitTarget())
                    return SPELL_FAILED_BAD_IMPLICIT_TARGETS;

                if (m_caster->GetTypeId() != TYPEID_PLAYER || m_CastItem)
                    break;

                if (m_targets.getUnitTarget()->getPowerType() != POWER_MANA)
                    return SPELL_FAILED_BAD_TARGETS;

                break;
            }
            case SPELL_AURA_MOD_SHAPESHIFT:
            {
                /* disallow druid fly/travel forms while having mount immunity */
                ShapeshiftForm form = ShapeshiftForm(m_spellInfo->EffectMiscValue[i]);
                switch (form) {
                    case FORM_FLIGHT:
                    case FORM_FLIGHT_EPIC:
                    case FORM_TRAVEL:
                        {    
                        SpellImmuneList const& mechanicList = m_caster->m_spellImmune[IMMUNITY_MECHANIC];
                        for (SpellImmuneList::const_iterator itr = mechanicList.begin(); itr != mechanicList.end(); ++itr)
                            if (itr->type == MECHANIC_MOUNT)
                                return SPELL_FAILED_IMMUNE;
                        }
                        break;
                    default:
                        break;
                }
            }
            default:
                break;
        }
    }

    // check trade slot case (last, for allow catch any another cast problems)
    if (m_targets.getTargetMask() & TARGET_FLAG_TRADE_ITEM)
    {
        if (m_CastItem)
            return SPELL_FAILED_ITEM_ENCHANT_TRADE_WINDOW;

        if (m_caster->GetTypeId() != TYPEID_PLAYER)
            return SPELL_FAILED_NOT_TRADING;

        TradeData* my_trade = m_caster->ToPlayer()->GetTradeData();

        if (!my_trade)
            return SPELL_FAILED_NOT_TRADING;

        TradeSlots slot = TradeSlots(m_targets.getItemTargetGUID());
        if (slot != TRADE_SLOT_NONTRADED)
            return SPELL_FAILED_BAD_TARGETS;

        if (!m_IsTriggeredSpell)
            if (my_trade->GetSpell())
                return SPELL_FAILED_ITEM_ALREADY_ENCHANTED;
    }

    // check if caster has at least 1 combo point for spells that require combo points
    if (m_needComboPoints)
        if (Player* plrCaster = m_caster->ToPlayer())
            if (!plrCaster->GetComboPoints())
                return SPELL_FAILED_NO_COMBO_POINTS;

    // all ok
    return SPELL_CAST_OK;
}

SpellCastResult Spell::CheckPetCast(Unit* target)
{
    if (!m_caster->IsAlive() && !(m_spellInfo->Attributes & SPELL_ATTR0_CASTABLE_WHILE_DEAD))
        return SPELL_FAILED_CASTER_DEAD;

    if (m_caster->HasUnitState(UNIT_STATE_CASTING) && !m_IsTriggeredSpell)              //prevent spellcast interruption by another spellcast
        return SPELL_FAILED_SPELL_IN_PROGRESS;
    if (m_caster->IsInCombat() && IsNonCombatSpell(m_spellInfo))
        return SPELL_FAILED_AFFECTING_COMBAT;

                                                            //dead owner (pets still alive when owners ressed?)
    if (Unit *owner = m_caster->GetCharmerOrOwner())
        if (!owner->IsAlive())
            return SPELL_FAILED_CASTER_DEAD;

    if (!target && m_targets.getUnitTarget())
        target = m_targets.getUnitTarget();

    for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (SpellTargetType[m_spellInfo->EffectImplicitTargetA[i]] == TARGET_TYPE_UNIT_TARGET
            || SpellTargetType[m_spellInfo->EffectImplicitTargetA[i]] == TARGET_TYPE_DEST_TARGET)
        {
            if (!target)
                return SPELL_FAILED_BAD_IMPLICIT_TARGETS;
            m_targets.setUnitTarget(target);
            break;
        }
    }

    Unit* _target = m_targets.getUnitTarget();

    if (_target)                                         //for target dead/target not valid
    {
        if (!_target->IsAlive())
            return SPELL_FAILED_BAD_TARGETS;

        if (!IsValidSingleTargetSpell(_target))
            return SPELL_FAILED_BAD_TARGETS;
    }
                                                        //cooldown
    if (m_caster->ToCreature()->HasSpellCooldown(m_spellInfo->Id))
        return SPELL_FAILED_NOT_READY;

    if (m_caster->IsPet())
        if (Unit* owner = m_caster->GetOwner())
            if (owner->GetTypeId() == TYPEID_PLAYER && owner->ToPlayer()->HasPetSpellCooldown(m_spellInfo->Id))
                return SPELL_FAILED_NOT_READY;

    return CheckCast(true);
}

uint32 Spell::GetCCDelay(SpellEntry const* _spell)
{
    // CCD for spell with auras
    AuraType auraWithCCD[] =
    {
        SPELL_AURA_MOD_STUN,
        SPELL_AURA_MOD_CONFUSE,
        SPELL_AURA_MOD_FEAR,
        SPELL_AURA_MOD_SILENCE,
        SPELL_AURA_MOD_DISARM,
        SPELL_AURA_MOD_POSSESS,
        SPELL_AURA_MOD_ROOT
    };
    const uint8 CCDArraySize = sizeof(auraWithCCD) / sizeof(auraWithCCD[0]);

    const uint32 ccDelay = 100;

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; i++)
    {
        switch (_spell->EffectApplyAuraName[i])
        {
            case SPELL_AURA_MOD_STUN:
                return ccDelay + 50;
        }
    }

    switch(_spell->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
            // Entrapment
            if (_spell->SpellIconID == 20)
                return 0;
            break;
        case SPELLFAMILY_HUNTER:
            // Traps
            if (_spell->SpellFamilyFlags[0] & 0x8 ||      // Frozen trap
                _spell->Id == 57879 ||                    // Snake Trap
                _spell->SpellFamilyFlags[2] & 0x00024000) // Explosive and Immolation Trap
                return 0;
            break;
        case SPELLFAMILY_DEATHKNIGHT:
            // Death Grip
            if (_spell->Id == 49576)
                return ccDelay;
            break;
        case SPELLFAMILY_ROGUE:
            // Blind
            if (_spell->Id == 2094)
                return ccDelay;
            // Smoke Bomb
            if (_spell->Id == 76577)
                return ccDelay + 350;
            // Vanish
            if (_spell->Id == 18461 || _spell->Id == 11327)
                return ccDelay + 50;
            break;
        case SPELLFAMILY_PRIEST:
            // Mind Control - for both caster and victim
            if(_spell->Id == 605)
                return ccDelay;
            break;
        case SPELLFAMILY_WARRIOR:
            // Charge + intercept
            if (_spell->Id == 96273 || _spell->Id == 20253)
                return ccDelay + 275;
            break;
        case SPELLFAMILY_DRUID:
            // Feral Charge (cat + bear form)
            if (_spell->Id == 49376 || _spell->Id == 16979)
                return ccDelay + 275;
            break;
    }

    for (uint8 i = 0; i < CCDArraySize; ++i)
        if (_spell->AppliesAuraType(auraWithCCD[i]))
            return ccDelay;

    return 0;
}

SpellCastResult Spell::CheckCasterAuras() const
{
    // spells totally immuned to caster auras (wsg flag drop, give marks etc)
    if (m_spellInfo->AttributesEx6& SPELL_ATTR6_IGNORE_CASTER_AURAS)
        return SPELL_CAST_OK;

    uint8 school_immune = 0;
    uint32 mechanic_immune = 0;
    uint32 dispel_immune = 0;

    // Check if the spell grants school or mechanic immunity.
    // We use bitmasks so the loop is done only once and not on every aura check below.
    if (m_spellInfo->AttributesEx & SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY)
    {
        for (int i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (m_spellInfo->EffectApplyAuraName[i] == SPELL_AURA_SCHOOL_IMMUNITY)
                school_immune |= uint32(m_spellInfo->EffectMiscValue[i]);
            else if (m_spellInfo->EffectApplyAuraName[i] == SPELL_AURA_MECHANIC_IMMUNITY)
                mechanic_immune |= 1 << uint32(m_spellInfo->EffectMiscValue[i]);
            else if (m_spellInfo->EffectApplyAuraName[i] == SPELL_AURA_DISPEL_IMMUNITY)
                dispel_immune |= GetDispellMask(DispelType(m_spellInfo->EffectMiscValue[i]));
        }
        // immune movement impairment and loss of control
        if (m_spellInfo->Id == 42292 || m_spellInfo->Id == 59752)
            mechanic_immune = IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK;
    }

    // Glyph of Pain Suppression
    if (m_spellInfo->SpellFamilyName == SPELLFAMILY_PRIEST && m_spellInfo->SpellIconID == 2178)
        if (m_caster->HasAuraEffect(63248, 0))      // no SpellFamilyFlags or SpellIconID to identify this
            mechanic_immune = 1 << MECHANIC_STUN;   // "immune" to stun only for this cast

    // Check whether the cast should be prevented by any state you might have.
    SpellCastResult prevented_reason = SPELL_CAST_OK;
    // Have to check if there is a stun aura. Otherwise will have problems with ghost aura apply while logging out
    uint32 unitflag = m_caster->GetUInt32Value(UNIT_FIELD_FLAGS);     // Get unit state
    if (unitflag & UNIT_FLAG_STUNNED && !(m_spellInfo->AttributesEx5 & SPELL_ATTR5_USABLE_WHILE_STUNNED))
        prevented_reason = SPELL_FAILED_STUNNED;
    else if (unitflag & UNIT_FLAG_CONFUSED && !(m_spellInfo->AttributesEx5 & SPELL_ATTR5_USABLE_WHILE_CONFUSED))
        prevented_reason = SPELL_FAILED_CONFUSED;
    else if (unitflag & UNIT_FLAG_FLEEING && !(m_spellInfo->AttributesEx5 & SPELL_ATTR5_USABLE_WHILE_FEARED))
        prevented_reason = SPELL_FAILED_FLEEING;
    else if (unitflag & UNIT_FLAG_SILENCED && m_spellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE)
        prevented_reason = SPELL_FAILED_SILENCED;
    else if (unitflag & UNIT_FLAG_PACIFIED && m_spellInfo->PreventionType == SPELL_PREVENTION_TYPE_PACIFY)
        prevented_reason = SPELL_FAILED_PACIFIED;

    // this aura type disallows any damaging attacks coming from player
    if (m_caster && m_caster->HasAuraType(SPELL_AURA_CANNOT_ATTACK))
    {
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; i++)
        {
            switch (m_spellInfo->Effect[i])
            {
                case SPELL_EFFECT_APPLY_AURA:
                    if (!(m_spellInfo->EffectApplyAuraName[i] == SPELL_AURA_PERIODIC_DAMAGE ||
                        m_spellInfo->EffectApplyAuraName[i] == SPELL_AURA_PERIODIC_DAMAGE_PERCENT ||
                        m_spellInfo->EffectApplyAuraName[i] == SPELL_AURA_PERIODIC_TRIGGER_SPELL ||
                        m_spellInfo->EffectApplyAuraName[i] == SPELL_AURA_PERIODIC_LEECH ||
                        m_spellInfo->EffectApplyAuraName[i] == SPELL_AURA_PERIODIC_DUMMY))
                        break;
                case SPELL_EFFECT_SCHOOL_DAMAGE:
                case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
                case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
                case SPELL_EFFECT_WEAPON_DAMAGE:
                case SPELL_EFFECT_DAMAGE_FROM_MAX_HEALTH_PCT:
                    return SPELL_FAILED_PACIFIED;
                    break;
                default:
                    break;
            }
        }
    }

    // Attr must make flag drop spell totally immune from all effects
    if (prevented_reason != SPELL_CAST_OK)
    {
        if (school_immune || mechanic_immune || dispel_immune)
        {
            //Checking auras is needed now, because you are prevented by some state but the spell grants immunity.
            Unit::AuraApplicationMap const& auras = m_caster->GetAppliedAuras();
            for (Unit::AuraApplicationMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            {
                Aura const * aura = itr->second->GetBase();
                if (GetAllSpellMechanicMask(aura->GetSpellProto()) & mechanic_immune)
                    continue;
                if (GetSpellSchoolMask(aura->GetSpellProto()) & school_immune)
                    continue;
                if ((1<<(aura->GetSpellProto()->Dispel)) & dispel_immune)
                    continue;

                //Make a second check for spell failed so the right SPELL_FAILED message is returned.
                //That is needed when your casting is prevented by multiple states and you are only immune to some of them.
                for (uint8 i=0; i<MAX_SPELL_EFFECTS; ++i)
                {
                    if (AuraEffect * part = aura->GetEffect(i))
                    {
                        switch(part->GetAuraType())
                        {
                            case SPELL_AURA_MOD_STUN:
                                if (!(m_spellInfo->AttributesEx5 & SPELL_ATTR5_USABLE_WHILE_STUNNED))
                                    return SPELL_FAILED_STUNNED;
                                break;
                            case SPELL_AURA_MOD_CONFUSE:
                                if (!(m_spellInfo->AttributesEx5 & SPELL_ATTR5_USABLE_WHILE_CONFUSED))
                                    return SPELL_FAILED_CONFUSED;
                                break;
                            case SPELL_AURA_MOD_FEAR:
                                if (!(m_spellInfo->AttributesEx5 & SPELL_ATTR5_USABLE_WHILE_FEARED))
                                    return SPELL_FAILED_FLEEING;
                                break;
                            case SPELL_AURA_MOD_SILENCE:
                            case SPELL_AURA_MOD_PACIFY:
                            case SPELL_AURA_MOD_PACIFY_SILENCE:
                                if (m_spellInfo->PreventionType == SPELL_PREVENTION_TYPE_PACIFY)
                                    return SPELL_FAILED_PACIFIED;
                                else if (m_spellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE)
                                    return SPELL_FAILED_SILENCED;
                                break;
                            default: break;
                        }
                    }
                }
            }
        }
        // You are prevented from casting and the spell casted does not grant immunity. Return a failed error.
        else
            return prevented_reason;
    }
    return SPELL_CAST_OK;
}

bool Spell::CanAutoCast(Unit* target)
{
    uint64 targetguid = target->GetGUID();

    for (uint32 j = 0; j < MAX_SPELL_EFFECTS; ++j)
    {
        if (m_spellInfo->Effect[j] == SPELL_EFFECT_APPLY_AURA)
        {
            if (m_spellInfo->StackAmount <= 1)
            {
                if (target->HasAuraEffect(m_spellInfo->Id, j))
                    return false;
            }
            else
            {
                if (AuraEffect * aureff = target->GetAuraEffect(m_spellInfo->Id, j))
                    if (aureff->GetBase()->GetStackAmount() >= m_spellInfo->StackAmount)
                        return false;
            }
        }
        else if (IsAreaAuraEffect(m_spellInfo->Effect[j]))
        {
            if (target->HasAuraEffect(m_spellInfo->Id, j))
                return false;
        }
    }

    SpellCastResult result = CheckPetCast(target);

    if (result == SPELL_CAST_OK || result == SPELL_FAILED_UNIT_NOT_INFRONT)
    {
        SelectSpellTargets();
        //check if among target units, our WANTED target is as well (->only self cast spells return false)
        for (std::list<TargetInfo>::iterator ihit= m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
            if (ihit->targetGUID == targetguid)
                return true;
    }
    return false;                                           //target invalid
}

SpellCastResult Spell::CheckRange(bool strict)
{
    // self cast doesn't need range checking -- also for Starshards fix
    if (m_spellInfo->rangeIndex == 1)
        return SPELL_CAST_OK;

    // Don't check for instant cast spells
    if (!strict && m_casttime == 0)
        return SPELL_CAST_OK;

    // Have Group, Will Travel spell do not check for range
    if (m_spellInfo->Id == 85592)
        return SPELL_CAST_OK;

    SpellRangeEntry const* srange = sSpellRangeStore.LookupEntry(m_spellInfo->rangeIndex);

    Unit *target = m_targets.getUnitTarget();
    float max_range = (float)m_caster->GetSpellMaxRangeForTarget(target, srange);
    float min_range = (float)m_caster->GetSpellMinRangeForTarget(target, srange);
    uint32 range_type = GetSpellRangeType(srange);

    if (Player* modOwner = m_caster->GetSpellModOwner())
        modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_RANGE, max_range, this);

    if (target && target != m_caster)
    {
        if (range_type == SPELL_RANGE_MELEE)
        {
            // Because of lag, we can not check too strictly here.
            if (!m_caster->IsWithinMeleeRange(target, max_range))
                return !m_IsTriggeredSpell ? SPELL_FAILED_OUT_OF_RANGE : SPELL_FAILED_DONT_REPORT;
        }
        else if (!m_caster->IsWithinCombatRange(target, max_range))
            return !m_IsTriggeredSpell ? SPELL_FAILED_OUT_OF_RANGE : SPELL_FAILED_DONT_REPORT; //0x5A;

        if (range_type == SPELL_RANGE_RANGED)
        {
            if (m_caster->IsWithinMeleeRange(target))
                return !m_IsTriggeredSpell ? SPELL_FAILED_TOO_CLOSE : SPELL_FAILED_DONT_REPORT;
        }
        else if (min_range && m_caster->IsWithinCombatRange(target, min_range)) // skip this check if min_range = 0
            return !m_IsTriggeredSpell ? SPELL_FAILED_TOO_CLOSE : SPELL_FAILED_DONT_REPORT;

        if (m_caster->GetTypeId() == TYPEID_PLAYER &&
            (m_spellInfo->FacingCasterFlags & SPELL_FACING_FLAG_INFRONT) && !m_caster->HasInArc(static_cast<float>(M_PI), target))
            return !m_IsTriggeredSpell ? SPELL_FAILED_UNIT_NOT_INFRONT : SPELL_FAILED_DONT_REPORT;
    }

    if (m_targets.HasDst() && !m_targets.HasTraj())
    {
        if (!m_caster->IsWithinDist3d(&m_targets.m_dstPos, max_range))
            return SPELL_FAILED_OUT_OF_RANGE;
        if (min_range && m_caster->IsWithinDist3d(&m_targets.m_dstPos, min_range))
            return SPELL_FAILED_TOO_CLOSE;
    }

    return SPELL_CAST_OK;
}

SpellCastResult Spell::CheckPower()
{
    // item cast not used power
    if (m_CastItem)
        return SPELL_CAST_OK;

    // health as power used - need check health amount
    if (m_spellInfo->powerType == POWER_HEALTH)
    {
        if (int32(m_caster->GetHealth()) <= m_powerCost)
            return SPELL_FAILED_CASTER_AURASTATE;
        return SPELL_CAST_OK;
    }
    // Check valid power type
    if (m_spellInfo->powerType >= MAX_POWERS)
    {
        sLog->outError("Spell::CheckPower: Unknown power type '%d'", m_spellInfo->powerType);
        return SPELL_FAILED_UNKNOWN;
    }

    //check rune cost only if a spell has PowerType == POWER_RUNE
    if (m_spellInfo->powerType == POWER_RUNE)
    {
        SpellCastResult failReason = CheckRuneCost(m_spellInfo->runeCostID);
        if (failReason != SPELL_CAST_OK)
            return failReason;
    }

    // Check power amount
    Powers powerType = Powers(m_spellInfo->powerType);

    if (int32(m_caster->GetPower(powerType)) < m_powerCost)
        return SPELL_FAILED_NO_POWER;
    else
        return SPELL_CAST_OK;
}

SpellCastResult Spell::CheckItems()
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return SPELL_CAST_OK;

    Player* p_caster = (Player*)m_caster;

    if (!m_CastItem)
    {
        if (m_castItemGUID)
            return SPELL_FAILED_ITEM_NOT_READY;
    }
    else
    {
        uint32 itemid = m_CastItem->GetEntry();
        if (!p_caster->HasItemCount(itemid, 1))
            return SPELL_FAILED_ITEM_NOT_READY;

        ItemPrototype const *proto = m_CastItem->GetProto();
        if (!proto)
            return SPELL_FAILED_ITEM_NOT_READY;

        for (int i = 0; i < MAX_ITEM_SPELLS; ++i)
            if (proto->Spells[i].SpellCharges)
                if (m_CastItem->GetSpellCharges(i) == 0)
                    return SPELL_FAILED_NO_CHARGES_REMAIN;

        // consumable cast item checks
        if (proto->Class == ITEM_CLASS_CONSUMABLE && m_targets.getUnitTarget())
        {
            // such items should only fail if there is no suitable effect at all - see Rejuvenation Potions for example
            SpellCastResult failReason = SPELL_CAST_OK;
            for (int i = 0; i < MAX_SPELL_EFFECTS; i++)
            {
                    // skip check, pet not required like checks, and for TARGET_UNIT_PET m_targets.getUnitTarget() is not the real target but the caster
                    if (m_spellInfo->EffectImplicitTargetA[i] == TARGET_UNIT_PET)
                    continue;

                if (m_spellInfo->Effect[i] == SPELL_EFFECT_HEAL)
                {
                    if (m_targets.getUnitTarget()->IsFullHealth())
                    {
                        failReason = SPELL_FAILED_ALREADY_AT_FULL_HEALTH;
                        continue;
                    }
                    else
                    {
                        failReason = SPELL_CAST_OK;
                        break;
                    }
                }

                // Mana Potion, Rage Potion, Thistle Tea(Rogue), ...
                if (m_spellInfo->Effect[i] == SPELL_EFFECT_ENERGIZE)
                {
                    if (m_spellInfo->EffectMiscValue[i] < 0 || m_spellInfo->EffectMiscValue[i] >= int8(MAX_POWERS))
                    {
                        failReason = SPELL_FAILED_ALREADY_AT_FULL_POWER;
                        continue;
                    }

                    Powers power = Powers(m_spellInfo->EffectMiscValue[i]);
                    if (m_targets.getUnitTarget()->GetPower(power) == (int32)m_targets.getUnitTarget()->GetMaxPower(power))
                    {
                        failReason = SPELL_FAILED_ALREADY_AT_FULL_POWER;
                        continue;
                    }
                    else
                    {
                        failReason = SPELL_CAST_OK;
                        break;
                    }
                }
            }
            if (failReason != SPELL_CAST_OK)
                return failReason;
        }
    }

    // check target item
    if (m_targets.getItemTargetGUID())
    {
        if (m_caster->GetTypeId() != TYPEID_PLAYER)
            return SPELL_FAILED_BAD_TARGETS;

        if (!m_targets.getItemTarget())
            return SPELL_FAILED_ITEM_GONE;

        if (!m_targets.getItemTarget()->IsFitToSpellRequirements(m_spellInfo))
            return SPELL_FAILED_EQUIPPED_ITEM_CLASS;
    }
    // if not item target then required item must be equipped
    else
    {
        if (m_caster->GetTypeId() == TYPEID_PLAYER && !m_caster->ToPlayer()->HasItemFitToSpellRequirements(m_spellInfo))
            return SPELL_FAILED_EQUIPPED_ITEM_CLASS;
    }

    // check spell focus object
    if (m_spellInfo->RequiresSpellFocus)
    {
        CellPair p(Trinity::ComputeCellPair(m_caster->GetPositionX(), m_caster->GetPositionY()));
        Cell cell(p);
        cell.data.Part.reserved = ALL_DISTRICT;

        GameObject* ok = NULL;
        Trinity::GameObjectFocusCheck go_check(m_caster,m_spellInfo->RequiresSpellFocus);
        Trinity::GameObjectSearcher<Trinity::GameObjectFocusCheck> checker(m_caster, ok, go_check);

        TypeContainerVisitor<Trinity::GameObjectSearcher<Trinity::GameObjectFocusCheck>, GridTypeMapContainer > object_checker(checker);
        Map& map = *m_caster->GetMap();
        cell.Visit(p, object_checker, map, *m_caster, map.GetVisibilityDistance());

        if (!ok)
            return SPELL_FAILED_REQUIRES_SPELL_FOCUS;

        focusObject = ok;                                   // game object found in range
    }

    // do not take reagents for these item casts
    if (!(m_CastItem && m_CastItem->GetProto()->Flags & ITEM_PROTO_FLAG_TRIGGERED_CAST))
    {
        // check reagents (ignore triggered spells with reagents processed by original spell) and special reagent ignore case.
        if (!m_IsTriggeredSpell && !p_caster->CanNoReagentCast(m_spellInfo))
        {
            for (uint32 i = 0; i < MAX_SPELL_REAGENTS; i++)
            {
                if (m_spellInfo->Reagent[i] <= 0)
                    continue;

                uint32 itemid    = m_spellInfo->Reagent[i];
                uint32 itemcount = m_spellInfo->ReagentCount[i];

                // if CastItem is also spell reagent
                if (m_CastItem && m_CastItem->GetEntry() == itemid)
                {
                    ItemPrototype const *proto = m_CastItem->GetProto();
                    if (!proto)
                        return SPELL_FAILED_ITEM_NOT_READY;
                    for (int s=0; s < MAX_ITEM_PROTO_SPELLS; ++s)
                    {
                        // CastItem will be used up and does not count as reagent
                        int32 charges = m_CastItem->GetSpellCharges(s);
                        if (proto->Spells[s].SpellCharges < 0 && abs(charges) < 2)
                        {
                            ++itemcount;
                            break;
                        }
                    }
                }
                if (!p_caster->HasItemCount(itemid,itemcount))
                    return SPELL_FAILED_ITEM_NOT_READY;         //0x54
            }
        }
/*
        //deprecated. in cataclysm totem items arent required to cast totem spells

        // check totem-item requirements (items presence in inventory)
        uint32 totems = 2;
        for (int i = 0; i < 2 ; ++i)
        {
            if (m_spellInfo->Totem[i] != 0)
            {
                if (p_caster->HasItemCount(m_spellInfo->Totem[i],1))
                {
                    totems -= 1;
                    continue;
                }
            }else
            totems -= 1;
        }
        if (totems != 0)
            return SPELL_FAILED_TOTEMS;                         //0x7C

        // Check items for TotemCategory  (items presence in inventory)
        uint32 TotemCategory = 2;
        for (int i= 0; i < 2; ++i)
        {
            if (m_spellInfo->TotemCategory[i] != 0)
            {
                if (p_caster->HasItemTotemCategory(m_spellInfo->TotemCategory[i]))
                {
                    TotemCategory -= 1;
                    continue;
                }
            }
            else
                TotemCategory -= 1;
        }
        if (TotemCategory != 0)
            return SPELL_FAILED_TOTEM_CATEGORY;                 //0x7B
        */
    }

    // special checks for spell effects
    for (int i = 0; i < MAX_SPELL_EFFECTS; i++)
    {
        switch (m_spellInfo->Effect[i])
        {
            case SPELL_EFFECT_CREATE_ITEM:
            case SPELL_EFFECT_CREATE_ITEM_2:
            {
                if (!m_IsTriggeredSpell && m_spellInfo->EffectItemType[i])
                {
                    ItemPosCountVec dest;
                    uint8 msg = p_caster->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, m_spellInfo->EffectItemType[i], 1);
                    if (msg != EQUIP_ERR_OK)
                    {
                        ItemPrototype const *pProto = sObjectMgr->GetItemPrototype(m_spellInfo->EffectItemType[i]);
                        // TODO: Needs review
                        if (pProto && !(pProto->ItemLimitCategory))
                        {
                            p_caster->SendEquipError(msg, NULL, NULL, m_spellInfo->EffectItemType[i]);
                            return SPELL_FAILED_DONT_REPORT;
                        }
                        else
                        {
                            if (!(m_spellInfo->SpellFamilyName == SPELLFAMILY_MAGE && (m_spellInfo->SpellFamilyFlags[0] & 0x40000000)))
                                return SPELL_FAILED_TOO_MANY_OF_ITEM;
                            else if (!(p_caster->HasItemCount(m_spellInfo->EffectItemType[i],1)))
                                return SPELL_FAILED_TOO_MANY_OF_ITEM;
                            else
                                p_caster->CastSpell(m_caster,SpellMgr::CalculateSpellEffectAmount(m_spellInfo, 1),false);        // move this to anywhere
                            return SPELL_FAILED_DONT_REPORT;
                        }
                    }
                }
                break;
            }
            case SPELL_EFFECT_ENCHANT_ITEM:
                if (m_spellInfo->EffectItemType[i] && m_targets.getItemTarget()
                    && m_targets.getItemTarget()->IsVellum())
                {
                    // cannot enchant vellum for other player
                    if (m_targets.getItemTarget()->GetOwner() != m_caster)
                        return SPELL_FAILED_NOT_TRADEABLE;
                    // do not allow to enchant vellum from scroll made by vellum-prevent exploit
                    if (m_CastItem && m_CastItem->GetProto()->Flags & ITEM_PROTO_FLAG_TRIGGERED_CAST)
                        return SPELL_FAILED_TOTEM_CATEGORY;
                    ItemPosCountVec dest;
                    uint8 msg = p_caster->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, m_spellInfo->EffectItemType[i], 1);
                    if (msg != EQUIP_ERR_OK)
                    {
                        p_caster->SendEquipError(msg, NULL, NULL, m_spellInfo->EffectItemType[i]);
                        return SPELL_FAILED_DONT_REPORT;
                    }
                }
            case SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC:
            {
                Item* targetItem = m_targets.getItemTarget();
                if (!targetItem)
                    return SPELL_FAILED_ITEM_NOT_FOUND;

                if (targetItem->GetProto()->ItemLevel < m_spellInfo->baseLevel)
                    return SPELL_FAILED_LOWLEVEL;

                bool isItemUsable = false;
                for (uint8 e = 0; e < MAX_ITEM_PROTO_SPELLS; ++e)
                {
                    ItemPrototype const *proto = targetItem->GetProto();
                    if (proto->Spells[e].SpellId && (
                        proto->Spells[e].SpellTrigger == ITEM_SPELLTRIGGER_ON_USE ||
                        proto->Spells[e].SpellTrigger == ITEM_SPELLTRIGGER_ON_NO_DELAY_USE))
                    {
                        isItemUsable = true;
                        break;
                    }
                }

                SpellItemEnchantmentEntry const *pEnchant = sSpellItemEnchantmentStore.LookupEntry(m_spellInfo->EffectMiscValue[i]);
                // do not allow adding usable enchantments to items that have use effect already
                if (pEnchant && isItemUsable)
                    for (uint8 s = 0; s < MAX_ITEM_ENCHANTMENT_EFFECTS; ++s)
                        if (pEnchant->type[s] == ITEM_ENCHANTMENT_TYPE_USE_SPELL)
                            return SPELL_FAILED_ON_USE_ENCHANT;

                // Not allow enchant in trade slot for some enchant type
                if (targetItem->GetOwner() != m_caster)
                {
                    if (!pEnchant)
                        return SPELL_FAILED_ERROR;
                    if (pEnchant->slot & ENCHANTMENT_CAN_SOULBOUND)
                        return SPELL_FAILED_NOT_TRADEABLE;
                }
                break;
            }
            case SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY:
            {
                Item *item = m_targets.getItemTarget();
                if (!item)
                    return SPELL_FAILED_ITEM_NOT_FOUND;
                // Not allow enchant in trade slot for some enchant type
                if (item->GetOwner() != m_caster)
                {
                    uint32 enchant_id = m_spellInfo->EffectMiscValue[i];
                    SpellItemEnchantmentEntry const *pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
                    if (!pEnchant)
                        return SPELL_FAILED_ERROR;
                    if (pEnchant->slot & ENCHANTMENT_CAN_SOULBOUND)
                        return SPELL_FAILED_NOT_TRADEABLE;
                }
                break;
            }
            case SPELL_EFFECT_ENCHANT_HELD_ITEM:
                // check item existence in effect code (not output errors at offhand hold item effect to main hand for example
                break;
            case SPELL_EFFECT_DISENCHANT:
            {
                if (!m_targets.getItemTarget())
                    return SPELL_FAILED_CANT_BE_DISENCHANTED;

                // prevent disenchanting in trade slot
                if (m_targets.getItemTarget()->GetOwnerGUID() != m_caster->GetGUID())
                    return SPELL_FAILED_CANT_BE_DISENCHANTED;

                ItemPrototype const* itemProto = m_targets.getItemTarget()->GetProto();
                if (!itemProto)
                    return SPELL_FAILED_CANT_BE_DISENCHANTED;

                // must have disenchant loot (other static req. checked at item prototype loading)
                if (!itemProto->DisenchantID)
                    return SPELL_FAILED_CANT_BE_DISENCHANTED;

                // 2.0.x addon: Check player enchanting level against the item disenchanting requirements
                int32 item_disenchantskilllevel = itemProto->RequiredDisenchantSkill;
                if (item_disenchantskilllevel > int32(p_caster->GetSkillValue(SKILL_ENCHANTING)))
                    return SPELL_FAILED_LOW_CASTLEVEL;
                break;
            }
            case SPELL_EFFECT_PROSPECTING:
            {
                if (!m_targets.getItemTarget())
                    return SPELL_FAILED_CANT_BE_PROSPECTED;
                //ensure item is a prospectable ore
                if (!(m_targets.getItemTarget()->GetProto()->Flags & ITEM_PROTO_FLAG_PROSPECTABLE))
                    return SPELL_FAILED_CANT_BE_PROSPECTED;
                //prevent prospecting in trade slot
                if (m_targets.getItemTarget()->GetOwnerGUID() != m_caster->GetGUID())
                    return SPELL_FAILED_CANT_BE_PROSPECTED;
                //Check for enough skill in jewelcrafting
                uint32 item_prospectingskilllevel = m_targets.getItemTarget()->GetProto()->RequiredSkillRank;
                if (item_prospectingskilllevel >p_caster->GetSkillValue(SKILL_JEWELCRAFTING))
                    return SPELL_FAILED_LOW_CASTLEVEL;
                //make sure the player has the required ores in inventory
                if (m_targets.getItemTarget()->GetCount() < 5)
                    return SPELL_FAILED_NEED_MORE_ITEMS;

                if (!LootTemplates_Prospecting.HaveLootFor(m_targets.getItemTargetEntry()))
                    return SPELL_FAILED_CANT_BE_PROSPECTED;

                break;
            }
            case SPELL_EFFECT_MILLING:
            {
                if (!m_targets.getItemTarget())
                    return SPELL_FAILED_CANT_BE_MILLED;
                //ensure item is a millable herb
                if (!(m_targets.getItemTarget()->GetProto()->Flags & ITEM_PROTO_FLAG_MILLABLE))
                    return SPELL_FAILED_CANT_BE_MILLED;
                //prevent milling in trade slot
                if (m_targets.getItemTarget()->GetOwnerGUID() != m_caster->GetGUID())
                    return SPELL_FAILED_CANT_BE_MILLED;
                //Check for enough skill in inscription
                uint32 item_millingskilllevel = m_targets.getItemTarget()->GetProto()->RequiredSkillRank;
                if (item_millingskilllevel >p_caster->GetSkillValue(SKILL_INSCRIPTION))
                    return SPELL_FAILED_LOW_CASTLEVEL;
                //make sure the player has the required herbs in inventory
                if (m_targets.getItemTarget()->GetCount() < 5)
                    return SPELL_FAILED_NEED_MORE_ITEMS;

                if (!LootTemplates_Milling.HaveLootFor(m_targets.getItemTargetEntry()))
                    return SPELL_FAILED_CANT_BE_MILLED;

                break;
            }
            case SPELL_EFFECT_WEAPON_DAMAGE:
            case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER) return SPELL_FAILED_TARGET_NOT_PLAYER;
                if (m_attackType != RANGED_ATTACK)
                    break;
                Item *pItem = m_caster->ToPlayer()->GetWeaponForAttack(m_attackType);
                if (!pItem || pItem->IsBroken())
                    return SPELL_FAILED_EQUIPPED_ITEM;

                switch(pItem->GetProto()->SubClass)
                {
                    case ITEM_SUBCLASS_WEAPON_THROWN:
                    {
                        uint32 ammo = pItem->GetEntry();
                        if (!m_caster->ToPlayer()->HasItemCount(ammo, 1))
                            return SPELL_FAILED_NO_AMMO;
                    };  break;
                    /*case ITEM_SUBCLASS_WEAPON_GUN:
                    case ITEM_SUBCLASS_WEAPON_BOW:
                    case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                    {
                        uint32 ammo = m_caster->ToPlayer()->GetUInt32Value(PLAYER_AMMO_ID);
                        if (!ammo)
                        {
                            // Requires No Ammo
                            if (m_caster->HasAura(46699))
                                break;                      // skip other checks

                            return SPELL_FAILED_NO_AMMO;
                        }

                        ItemPrototype const *ammoProto = sObjectMgr->GetItemPrototype(ammo);
                        if (!ammoProto)
                            return SPELL_FAILED_NO_AMMO;

                        if (ammoProto->Class != ITEM_CLASS_PROJECTILE)
                            return SPELL_FAILED_NO_AMMO;

                        // check ammo ws. weapon compatibility
                        switch(pItem->GetProto()->SubClass)
                        {
                            case ITEM_SUBCLASS_WEAPON_BOW:
                            case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                                if (ammoProto->SubClass != ITEM_SUBCLASS_ARROW)
                                    return SPELL_FAILED_NO_AMMO;
                                break;
                            case ITEM_SUBCLASS_WEAPON_GUN:
                                if (ammoProto->SubClass != ITEM_SUBCLASS_BULLET)
                                    return SPELL_FAILED_NO_AMMO;
                                break;
                            default:
                                return SPELL_FAILED_NO_AMMO;
                        }

                        if (!m_caster->ToPlayer()->HasItemCount(ammo, 1))
                        {
                            m_caster->ToPlayer()->SetUInt32Value(PLAYER_AMMO_ID, 0);
                            return SPELL_FAILED_NO_AMMO;
                        }
                    };  break;*/
                    case ITEM_SUBCLASS_WEAPON_WAND:
                        break;
                    default:
                        break;
                }
                break;
            }
            case SPELL_EFFECT_CREATE_MANA_GEM:
            {
                 uint32 item_id = m_spellInfo->EffectItemType[i];
                 ItemPrototype const *pProto = sObjectMgr->GetItemPrototype(item_id);

                 if (!pProto)
                     return SPELL_FAILED_ITEM_AT_MAX_CHARGES;

                 if (Item* pitem = p_caster->GetItemByEntry(item_id))
                 {
                     for (int x = 0; x < MAX_ITEM_PROTO_SPELLS; ++x)
                         if (pProto->Spells[x].SpellCharges != 0 && pitem->GetSpellCharges(x) == pProto->Spells[x].SpellCharges)
                             return SPELL_FAILED_ITEM_AT_MAX_CHARGES;
                 }
                 break;
            }
            default:
                break;
        }
    }

    // check weapon presence in slots for main/offhand weapons
    if (m_spellInfo->EquippedItemClass >=0)
    {
        // main hand weapon required
        if (m_spellInfo->AttributesEx3 & SPELL_ATTR3_MAIN_HAND)
        {
            Item* item = m_caster->ToPlayer()->GetWeaponForAttack(BASE_ATTACK);

            // skip spell if no weapon in slot or broken
            if (!item || item->IsBroken())
                return m_IsTriggeredSpell? SPELL_FAILED_DONT_REPORT : SPELL_FAILED_EQUIPPED_ITEM_CLASS;

            // skip spell if weapon not fit to triggered spell
            if (!item->IsFitToSpellRequirements(m_spellInfo))
                return m_IsTriggeredSpell? SPELL_FAILED_DONT_REPORT : SPELL_FAILED_EQUIPPED_ITEM_CLASS;
        }

        // offhand hand weapon required
        if (m_spellInfo->AttributesEx3 & SPELL_ATTR3_REQ_OFFHAND)
        {
            Item* item = m_caster->ToPlayer()->GetWeaponForAttack(OFF_ATTACK);

            // skip spell if no weapon in slot or broken
            if (!item || item->IsBroken())
                return m_IsTriggeredSpell? SPELL_FAILED_DONT_REPORT : SPELL_FAILED_EQUIPPED_ITEM_CLASS;

            // skip spell if weapon not fit to triggered spell
            if (!item->IsFitToSpellRequirements(m_spellInfo))
                return m_IsTriggeredSpell? SPELL_FAILED_DONT_REPORT : SPELL_FAILED_EQUIPPED_ITEM_CLASS;
        }
    }

    return SPELL_CAST_OK;
}

void Spell::Delayed() // only called in DealDamage()
{
    if (!m_caster)// || m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    //if (m_spellState == SPELL_STATE_DELAYED)
    //    return;                                             // spell is active and can't be time-backed

    if (isDelayableNoMore())                                 // Spells may only be delayed twice
        return;

    // spells not loosing casting time (slam, dynamites, bombs..)
    //if (!(m_spellInfo->InterruptFlags & SPELL_INTERRUPT_FLAG_DAMAGE))
    //    return;

    //check pushback reduce
    int32 delaytime = 500;                                  // spellcasting delay is normally 500ms
    int32 delayReduce = 100;                                // must be initialized to 100 for percent modifiers
    m_caster->ToPlayer()->ApplySpellMod(m_spellInfo->Id, SPELLMOD_NOT_LOSE_CASTING_TIME, delayReduce, this);
    delayReduce += m_caster->GetTotalAuraModifier(SPELL_AURA_REDUCE_PUSHBACK) - 100;
    if (delayReduce >= 100)
        return;

    delaytime = delaytime * (100 - delayReduce) / 100;

    if (int32(m_timer) + delaytime > m_casttime)
    {
        delaytime = m_casttime - m_timer;
        m_timer = m_casttime;
    }
    else
        m_timer += delaytime;

    sLog->outDetail("Spell %u partially interrupted for (%d) ms at damage", m_spellInfo->Id, delaytime);

    WorldPacket data(SMSG_SPELL_DELAYED, 8+4);
    data.append(m_caster->GetPackGUID());
    data << uint32(delaytime);

    m_caster->SendMessageToSet(&data, true);
}

void Spell::DelayedChannel()
{
    if (!m_caster || m_caster->GetTypeId() != TYPEID_PLAYER || getState() != SPELL_STATE_CASTING)
        return;

    if (isDelayableNoMore())                                    // Spells may only be delayed twice
        return;

    //check pushback reduce
    int32 delaytime = GetSpellDuration(m_spellInfo) * 25 / 100; // channeling delay is normally 25% of its time per hit
    int32 delayReduce = 100;                                    // must be initialized to 100 for percent modifiers
    m_caster->ToPlayer()->ApplySpellMod(m_spellInfo->Id, SPELLMOD_NOT_LOSE_CASTING_TIME, delayReduce, this);
    delayReduce += m_caster->GetTotalAuraModifier(SPELL_AURA_REDUCE_PUSHBACK) - 100;
    if (delayReduce >= 100)
        return;

    delaytime = delaytime * (100 - delayReduce) / 100;

    if (int32(m_timer) <= delaytime)
    {
        delaytime = m_timer;
        m_timer = 0;
    }
    else
        m_timer -= delaytime;

    sLog->outDebug("Spell %u partially interrupted for %i ms, new duration: %u ms", m_spellInfo->Id, delaytime, m_timer);

    for (std::list<TargetInfo>::const_iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
        if ((*ihit).missCondition == SPELL_MISS_NONE)
            if (Unit* unit = (m_caster->GetGUID() == ihit->targetGUID) ? m_caster : ObjectAccessor::GetUnit(*m_caster, ihit->targetGUID))
                unit->DelayOwnedAuras(m_spellInfo->Id, m_originalCasterGUID, delaytime);

    // partially interrupt persistent area auras
    if (DynamicObject* dynObj = m_caster->GetDynObject(m_spellInfo->Id))
        dynObj->Delay(delaytime);

    SendChannelUpdate(m_timer);
}

void Spell::UpdatePointers()
{
    if (m_originalCasterGUID == m_caster->GetGUID())
        m_originalCaster = m_caster;
    else
    {
        m_originalCaster = ObjectAccessor::GetUnit(*m_caster,m_originalCasterGUID);
        if (m_originalCaster && !m_originalCaster->IsInWorld())
            m_originalCaster = NULL;
    }

    if (m_castItemGUID && m_caster->GetTypeId() == TYPEID_PLAYER)
        m_CastItem = m_caster->ToPlayer()->GetItemByGuid(m_castItemGUID);

    m_targets.Update(m_caster);
}

bool Spell::CheckTargetCreatureType(Unit* target) const
{
    uint32 spellCreatureTargetMask = m_spellInfo->TargetCreatureType;

    // Curse of Doom & Exorcism: not find another way to fix spell target check :/
    if (m_spellInfo->SpellFamilyName == SPELLFAMILY_WARLOCK && m_spellInfo->Category == 1179)
    {
        // not allow cast at player
        if (target->GetTypeId() == TYPEID_PLAYER)
            return false;

        spellCreatureTargetMask = 0x7FF;
    }

    // Dismiss Pet and Taming Lesson skipped
    if (m_spellInfo->Id == 2641 || m_spellInfo->Id == 23356)
        spellCreatureTargetMask =  0;

    // Polymorph and Grounding Totem
    if (target->GetEntry() == 5925 && m_spellInfo->SpellFamilyName == SPELLFAMILY_MAGE && (m_spellInfo->SpellFamilyFlags[0] & 0x1000000) && m_spellInfo->EffectApplyAuraName[0] == SPELL_AURA_MOD_CONFUSE)
        return true;

    if (spellCreatureTargetMask)
    {
        uint32 TargetCreatureType = target->GetCreatureTypeMask();

        return !TargetCreatureType || (spellCreatureTargetMask & TargetCreatureType);
    }
    return true;
}

CurrentSpellTypes Spell::GetCurrentContainer()
{
    if (IsNextMeleeSwingSpell())
        return(CURRENT_MELEE_SPELL);
    else if (IsAutoRepeat())
        return(CURRENT_AUTOREPEAT_SPELL);
    else if (IsChanneledSpell(m_spellInfo))
        return(CURRENT_CHANNELED_SPELL);
    else
        return(CURRENT_GENERIC_SPELL);
}

bool Spell::CheckTarget(Unit* target, uint32 eff)
{
    // Check targets for creature type mask and remove not appropriate (skip explicit self target case, maybe need other explicit targets)
    if (m_spellInfo->EffectImplicitTargetA[eff] != TARGET_UNIT_CASTER)
    {
        if (!CheckTargetCreatureType(target))
            return false;
    }

    // Check Aura spell req (need for AoE spells)
    if (m_spellInfo->targetAuraSpell && !target->HasAura(m_spellInfo->targetAuraSpell))
        return false;
    if (m_spellInfo->excludeTargetAuraSpell && target->HasAura(m_spellInfo->excludeTargetAuraSpell))
        return false;

    // Check targets for not_selectable unit flag and remove
    // A player can cast spells on his pet (or other controlled unit) though in any state
    if (target != m_caster && target->GetCharmerOrOwnerGUID() != m_caster->GetGUID())
    {
        // any unattackable target skipped
        if (target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
            return false;

        // unselectable targets skipped in all cases except TARGET_UNIT_NEARBY_ENTRY targeting
        // in case TARGET_UNIT_NEARBY_ENTRY target selected by server always and can't be cheated
        /*if (target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE) &&
            m_spellInfo->EffectImplicitTargetA[eff] != TARGET_UNIT_NEARBY_ENTRY &&
            m_spellInfo->EffectImplicitTargetB[eff] != TARGET_UNIT_NEARBY_ENTRY)
            return false;*/
    }

    //Check player targets and remove if in GM mode or GM invisibility (for not self casting case)
    if (target != m_caster && target->GetTypeId() == TYPEID_PLAYER)
    {
        if (target->ToPlayer()->GetVisibility() == VISIBILITY_OFF)
            return false;

        if (target->ToPlayer()->IsGameMaster() && !IsPositiveSpell(m_spellInfo->Id))
            return false;
    }

    switch(m_spellInfo->EffectApplyAuraName[eff])
    {
        case SPELL_AURA_NONE:
        default:
            break;
        case SPELL_AURA_MOD_POSSESS:
        case SPELL_AURA_MOD_CHARM:
        case SPELL_AURA_MOD_POSSESS_PET:
        case SPELL_AURA_AOE_CHARM:
            if (target->GetTypeId() == TYPEID_UNIT && target->IsVehicle())
                return false;
            if (target->IsMounted())
                return false;
            if (target->GetCharmerGUID())
                return false;
            if (int32 damage = CalculateDamage(eff, target))
                if ((int32)target->getLevel() > damage)
                    return false;
            break;
    }

    //Do not do further checks for triggered spells
    if (m_IsTriggeredSpell)
        return true;

    //Check targets for LOS visibility (except spells without range limitations)
    switch(m_spellInfo->Effect[eff])
    {
        case SPELL_EFFECT_SUMMON_PLAYER:                    // from anywhere
            break;
        case SPELL_EFFECT_DUMMY:
            if (m_spellInfo->Id != 20577)                    // Cannibalize
                break;
            //fall through
        case SPELL_EFFECT_RESURRECT:
        case SPELL_EFFECT_RESURRECT_NEW:
            // player far away, maybe his corpse near?
            if (target != m_caster && !target->IsWithinLOSInMap(m_caster))
            {
                if (!m_targets.getCorpseTargetGUID())
                    return false;

                Corpse *corpse = ObjectAccessor::GetCorpse(*m_caster, m_targets.getCorpseTargetGUID());
                if (!corpse)
                    return false;

                if (target->GetGUID() != corpse->GetOwnerGUID())
                    return false;

                if (!corpse->IsWithinLOSInMap(m_caster))
                    return false;
            }

            // all ok by some way or another, skip normal check
            break;
        default:                                            // normal case
            // Get GO cast coordinates if original caster -> GO
            WorldObject *caster = NULL;
            if (IS_GAMEOBJECT_GUID(m_originalCasterGUID))
                caster = m_caster->GetMap()->GetGameObject(m_originalCasterGUID);
            if (!caster)
                caster = m_caster;
            if (target->GetEntry() == 5925)
                return true;
            // check LoS
            if (target != m_caster && !(m_spellInfo->AttributesEx2 & SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS) &&
                !target->IsWithinLOSInMap(caster))
                return false;
            break;
    }

    return true;
}

bool Spell::IsNeedSendToClient() const
{
    return m_spellInfo->SpellVisual[0] || m_spellInfo->SpellVisual[1] || IsChanneledSpell(m_spellInfo) ||
        (m_spellInfo->AttributesEx8 & SPELL_ATTR8_AURA_SEND_AMOUNT) || m_spellInfo->speed > 0.0f || (!m_triggeredByAuraSpell && !m_IsTriggeredSpell);
}

bool Spell::HaveTargetsForEffect(uint8 effect) const
{
    for (std::list<TargetInfo>::const_iterator itr = m_UniqueTargetInfo.begin(); itr != m_UniqueTargetInfo.end(); ++itr)
        if (itr->effectMask & (1 << effect))
            return true;

    for (std::list<GOTargetInfo>::const_iterator itr = m_UniqueGOTargetInfo.begin(); itr != m_UniqueGOTargetInfo.end(); ++itr)
        if (itr->effectMask & (1 << effect))
            return true;

    for (std::list<ItemTargetInfo>::const_iterator itr = m_UniqueItemInfo.begin(); itr != m_UniqueItemInfo.end(); ++itr)
        if (itr->effectMask & (1 << effect))
            return true;

    return false;
}

SpellEvent::SpellEvent(Spell* spell) : BasicEvent()
{
    m_Spell = spell;
}

SpellEvent::~SpellEvent()
{
    if (m_Spell->getState() != SPELL_STATE_FINISHED)
        m_Spell->cancel();

    if (m_Spell->IsDeletable())
    {
        delete m_Spell;
    }
    else
    {
        sLog->outError("~SpellEvent: %s %u tried to delete non-deletable spell %u. Was not deleted, causes memory leak.",
            (m_Spell->GetCaster()->GetTypeId() == TYPEID_PLAYER ? "Player" : "Creature"), m_Spell->GetCaster()->GetGUIDLow(), m_Spell->m_spellInfo->Id);
        ASSERT(false);
    }
}

bool SpellEvent::Execute(uint64 e_time, uint32 p_time)
{
    // update spell if it is not finished
    if (m_Spell->getState() != SPELL_STATE_FINISHED)
        m_Spell->update(p_time);

    // check spell state to process
    switch (m_Spell->getState())
    {
        case SPELL_STATE_FINISHED:
        {
            // spell was finished, check deletable state
            if (m_Spell->IsDeletable())
            {
                // check, if we do have unfinished triggered spells
                return true;                                // spell is deletable, finish event
            }
            // event will be re-added automatically at the end of routine)
        } break;

        case SPELL_STATE_DELAYED:
        {
            // first, check, if we have just started
            if (m_Spell->GetDelayStart() != 0)
            {
                // no, we aren't, do the typical update
                // check, if we have channeled spell on our hands
                if (IsChanneledSpell(m_Spell->m_spellInfo))
                {
                    // evented channeled spell is processed separately, casted once after delay, and not destroyed till finish
                    // check, if we have casting anything else except this channeled spell and autorepeat
                    if (m_Spell->GetCaster()->IsNonMeleeSpellCasted(false, true, true))
                    {
                        // another non-melee non-delayed spell is casted now, abort
                        m_Spell->cancel();
                    }
                    else
                    {
                        // Set last not triggered spell for apply spellmods
                        if (m_Spell->GetCaster() && m_Spell->GetCaster()->GetTypeId() == TYPEID_PLAYER)
                            ((Player*)m_Spell->GetCaster())->SetSpellModTakingSpell(m_Spell, true);

                        // do the action (pass spell to channeling state)
                        m_Spell->handle_immediate();

                        // And remove after effect handling
                        if (m_Spell->GetCaster() && m_Spell->GetCaster()->GetTypeId() == TYPEID_PLAYER)
                            ((Player*)m_Spell->GetCaster())->SetSpellModTakingSpell(m_Spell, false);
                    }
                    // event will be re-added automatically at the end of routine)
                }
                else
                {
                    // run the spell handler and think about what we can do next
                    uint64 t_offset = e_time - m_Spell->GetDelayStart();
                    uint64 n_offset = m_Spell->handle_delayed(t_offset);
                    if (n_offset)
                    {
                        // re-add us to the queue
                        m_Spell->GetCaster()->m_Events.AddEvent(this, m_Spell->GetDelayStart() + n_offset, false);
                        return false;                       // event not complete
                    }
                    // event complete
                    // finish update event will be re-added automatically at the end of routine)
                }
            }
            else
            {
                // delaying had just started, record the moment
                m_Spell->SetDelayStart(e_time);
                // re-plan the event for the delay moment
                m_Spell->GetCaster()->m_Events.AddEvent(this, e_time + m_Spell->GetDelayMoment(), false);
                return false;                               // event not complete
            }
        } break;

        case SPELL_STATE_QUEUED:
        {
            const SpellEntry *spellInfo = m_Spell->m_spellInfo;
            Player *player = ((Player*)m_Spell->GetCaster());
            if (!spellInfo || !player)
                return true;

            if (!player->HasQueuedSpell())      // interrupted by Esc key, ...
            {
                m_Spell->SendCastResult(SPELL_FAILED_DONT_REPORT);
                m_Spell->finish(false);
                break;
            }

            // don't cast another spell while looting, disable for archaeology (it would fail because of project change)
            if (player->GetLootGUID() || spellInfo->researchProjectId || m_Spell->m_targets.getGOTarget())
            {
                m_Spell->SendCastResult(SPELL_FAILED_NOT_READY);
                m_Spell->finish(false);
                player->CancelQueuedSpell();
                break;
            }
            
            SpellCastResult result = m_Spell->CheckCast(true);
            // can't cast because something other than CD and cast is preventing casting (e.g. crowd control)
            if (result != SPELL_CAST_OK && result != SPELL_FAILED_NOT_READY && result != SPELL_FAILED_SPELL_IN_PROGRESS)
            {
                m_Spell->SendCastResult(result);
                m_Spell->finish(false);
                player->CancelQueuedSpell();
                break;
            }

            // remaining cooldown and global cooldown on queued spell
            uint32 cd = player->GetSpellCooldownDelay(spellInfo->Id);
            uint32 gcd = player->GetGlobalCooldown(spellInfo);

            if (cd > gcd)
            {
                m_Spell->SendCastResult(SPELL_FAILED_NOT_READY);
                m_Spell->finish(false);
                player->CancelQueuedSpell();
                break;
            }

            // not ready or another action in progress - wait more
            if (result != SPELL_CAST_OK || player->IsNonMeleeSpellCasted(false, true, true))
            {
                // cast time of current spell
                uint32 cast = 0;
                Spell *current = player->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                if (current)
                    cast = current->GetRemainingCastTime();

                uint32 available = std::max(cast, gcd) + 1;      // 1ms after previous spell cast
                if (available <= 0)
                    available = 1;

                // maximum of 400 ms allowed
                 // this sometimes happens when player tries to cast the spell twice on one global cooldown
                if (available > 400)
                {
                    m_Spell->SendCastResult(SPELL_FAILED_NOT_READY);
                    m_Spell->finish(false);
                    player->CancelQueuedSpell();
                    break;
                }

                player->m_Events.AddEvent(this, e_time + available, false);
                return false;
            }

            // cast queued spell
            m_Spell->setState(SPELL_STATE_PREPARING);
            m_Spell->prepareFinish(NULL);
            player->CancelQueuedSpell();
            
            // send global cooldown to client
            // without this the GDC on client would start immediately when pressed spell on action bar
            //  instead of when the cast is started
            // TODO: invent something better, this sometimes causes the GCD on client
            //  jumping up (finished cast) & down (started cast of this)
            //  one of the possibilities is to cast this spell immediately, not as separated event
            WorldPacket data(SMSG_SPELL_COOLDOWN, 8+1+4);
            data << uint64(player->GetGUID());
            data << uint8(3);
            data << uint32(spellInfo->Id);
            data << uint32(player->GetGlobalCooldown(spellInfo));
            player->GetSession()->SendPacket(&data);
            break;
        } break;

        default:
        {
            // all other states
            // event will be re-added automatically at the end of routine)
        } break;
    }

    // spell processing not complete, plan event on the next update interval
    m_Spell->GetCaster()->m_Events.AddEvent(this, e_time + 1, false);
    return false;                                           // event not complete
}

void SpellEvent::Abort(uint64 /*e_time*/)
{
    // oops, the spell we try to do is aborted
    if (m_Spell->getState() != SPELL_STATE_FINISHED)
        m_Spell->cancel();
}

bool SpellEvent::IsDeletable() const
{
    return m_Spell->IsDeletable();
}

bool Spell::IsValidSingleTargetEffect(Unit const* target, Targets type) const
{
    switch (type)
    {
        case TARGET_UNIT_TARGET_ENEMY:
            return !m_caster->IsFriendlyTo(target);
        case TARGET_UNIT_TARGET_ALLY:
        case TARGET_UNIT_PARTY_TARGET:
            return m_caster->IsFriendlyTo(target);
        case TARGET_UNIT_TARGET_PARTY:
            return m_caster != target && m_caster->IsInPartyWith(target);
        case TARGET_UNIT_TARGET_RAID:
            return m_caster->IsInRaidWith(target);
        case TARGET_UNIT_TARGET_PUPPET:
            return target->HasUnitTypeMask(UNIT_MASK_PUPPET) && m_caster == target->GetOwner();
        default:
            break;
    }
    return true;
}

bool Spell::IsValidSingleTargetSpell(Unit const* target) const
{
    if (target->GetMapId() == MAPID_INVALID)
    {
        sLog->outDebug("Spell::IsValidSingleTargetSpell - a spell was cast on '%s' (GUIDLow: %u), but they have an invalid map id!", target->GetName(), target->GetGUIDLow());
        return false;
    }
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (!IsValidSingleTargetEffect(target, Targets(m_spellInfo->EffectImplicitTargetA[i])))
            return false;
        // Need to check B?
        //if (!IsValidSingleTargetEffect(m_spellInfo->EffectImplicitTargetB[i], target)
        //    return false;
    }
    return true;
}

bool Spell::IsValidDeadOrAliveTarget(Unit const* target) const
{
    if (target->IsAlive())
        return !IsRequiringDeadTargetSpell(m_spellInfo);
    if (IsAllowingDeadTargetSpell(m_spellInfo))
        return true;
    return false;
}

void Spell::CalculateDamageDoneForAllTargets()
{
    float multiplier[MAX_SPELL_EFFECTS];
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        if (m_applyMultiplierMask & (1 << i))
            multiplier[i] = SpellMgr::CalculateSpellEffectDamageMultiplier(m_spellInfo, i, m_originalCaster, this);

    bool usesAmmo = true;
    Unit::AuraEffectList const& Auras = m_caster->GetAuraEffectsByType(SPELL_AURA_ABILITY_CONSUME_NO_AMMO);
    for (Unit::AuraEffectList::const_iterator j = Auras.begin(); j != Auras.end(); ++j)
    {
        if ((*j)->IsAffectedOnSpell(m_spellInfo))
            usesAmmo=false;
    }

    for (std::list<TargetInfo>::iterator ihit= m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
    {
        TargetInfo &target = *ihit;

        uint32 mask = target.effectMask;
        if (!mask)
            continue;

        Unit* unit = m_caster->GetGUID() == target.targetGUID ? m_caster : ObjectAccessor::GetUnit(*m_caster, target.targetGUID);
        if (!unit) // || !unit->IsAlive()) do we need to check alive here?
            continue;

        if (usesAmmo)
        {
            bool ammoTaken = false;
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; i++)
            {
                if (!(mask & 1<<i))
                    continue;
                switch (m_spellInfo->Effect[i])
                {
                    case SPELL_EFFECT_SCHOOL_DAMAGE:
                    case SPELL_EFFECT_WEAPON_DAMAGE:
                    case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
                    case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
                    case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
                    ammoTaken=true;
                    TakeAmmo();
                }
                if (ammoTaken)
                    break;
            }
        }

        if (target.missCondition == SPELL_MISS_NONE)                          // In case spell hit target, do all effect on that target
        {
            target.damage += CalculateDamageDone(unit, mask, multiplier);
            target.crit = m_caster->IsSpellCrit(unit, m_spellInfo, m_spellSchoolMask, m_attackType);
        }
        else if (target.missCondition == SPELL_MISS_REFLECT)                // In case spell reflect from target, do all effect on caster (if hit)
        {
            if (target.reflectResult == SPELL_MISS_NONE)       // If reflected spell hit caster -> do all effect on him
            {
                target.damage += CalculateDamageDone(m_caster, mask, multiplier);
                target.crit = m_caster->IsSpellCrit(m_caster, m_spellInfo, m_spellSchoolMask, m_attackType);
            }
        }
    }
}

int32 Spell::CalculateDamageDone(Unit *unit, const uint32 effectMask, float * multiplier)
{
    int32 damageDone = 0;
    unitTarget = unit;
    for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (effectMask & (1<<i))
        {
            m_damage = 0;
            damage = CalculateDamage(i, NULL);

            switch(m_spellInfo->Effect[i])
            {
                case SPELL_EFFECT_SCHOOL_DAMAGE:
                    SpellDamageSchoolDmg((SpellEffIndex)i);
                    break;
                case SPELL_EFFECT_WEAPON_DAMAGE:
                case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
                case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
                case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
                    SpellDamageWeaponDmg((SpellEffIndex)i);
                    break;
                case SPELL_EFFECT_HEAL:
                    SpellDamageHeal((SpellEffIndex)i);
                    break;
            }

            if (m_damage > 0 && !(m_spellInfo->Attributes & SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY))
            {
                if (IsAreaEffectTarget[m_spellInfo->EffectImplicitTargetA[i]] || IsAreaEffectTarget[m_spellInfo->EffectImplicitTargetB[i]])
                {
                    m_damage = int32(float(m_damage) * unit->GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_AOE_DAMAGE_AVOIDANCE, m_spellInfo->SchoolMask));
                    if (m_caster->GetTypeId() == TYPEID_UNIT)
                        m_damage = int32(float(m_damage) * unit->GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_CREATURE_AOE_DAMAGE_AVOIDANCE, m_spellInfo->SchoolMask));

                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    {
                        uint32 targetAmount = m_UniqueTargetInfo.size();
                        if (targetAmount > 10)
                            m_damage = m_damage * 10/targetAmount;
                    }
                }
            }

            if (m_applyMultiplierMask & (1 << i))
            {
                m_damage = int32(m_damage * m_damageMultipliers[i]);
                m_damageMultipliers[i] *= multiplier[i];
            }

            damageDone += m_damage;
        }
    }

    return damageDone;
}

SpellCastResult Spell::CanOpenLock(uint32 effIndex, uint32 lockId, SkillType& skillId, int32& reqSkillValue, int32& skillValue)
{
    if (!lockId)                                             // possible case for GO and maybe for items.
        return SPELL_CAST_OK;

    // Get LockInfo
    LockEntry const *lockInfo = sLockStore.LookupEntry(lockId);

    if (!lockInfo)
        return SPELL_FAILED_BAD_TARGETS;

    bool reqKey = false;                                    // some locks not have reqs

    for (int j = 0; j < MAX_LOCK_CASE; ++j)
    {
        switch(lockInfo->Type[j])
        {
            // check key item (many fit cases can be)
            case LOCK_KEY_ITEM:
                if (lockInfo->Index[j] && m_CastItem && m_CastItem->GetEntry() == lockInfo->Index[j])
                    return SPELL_CAST_OK;
                reqKey = true;
                break;
                // check key skill (only single first fit case can be)
            case LOCK_KEY_SKILL:
            {
                reqKey = true;

                // wrong locktype, skip
                if (uint32(m_spellInfo->EffectMiscValue[effIndex]) != lockInfo->Index[j])
                    continue;

                skillId = SkillByLockType(LockType(lockInfo->Index[j]));

                if (skillId != SKILL_NONE)
                {
                    // skill bonus provided by casting spell (mostly item spells)
                    // add the damage modifier from the spell casted (cheat lock / skeleton key etc.)
                    uint32 spellSkillBonus = uint32(CalculateDamage(effIndex, NULL));
                    reqSkillValue = lockInfo->Skill[j];

                    // castitem check: rogue using skeleton keys. the skill values should not be added in this case.
                    skillValue = m_CastItem || m_caster->GetTypeId()!= TYPEID_PLAYER ?
                        0 : m_caster->ToPlayer()->GetSkillValue(skillId);

                    if(skillId == SKILL_LOCKPICKING)
                    {
                        skillValue += 100; //Add lockpicking constant base bonus for first 20 levels because of spellEntry->spellLevel set to 20 => base points -= (20 * base points to level == 100)
                    }

                    skillValue += spellSkillBonus;

                    if (skillValue < reqSkillValue)
                        return SPELL_FAILED_LOW_CASTLEVEL;
                }

                return SPELL_CAST_OK;
            }
        }
    }

    if (reqKey)
        return SPELL_FAILED_BAD_TARGETS;

    return SPELL_CAST_OK;
}

void Spell::SetSpellValue(SpellValueMod mod, int32 value)
{
    switch(mod)
    {
        case SPELLVALUE_BASE_POINT0:
            m_spellValue->EffectBasePoints[0] = SpellMgr::CalculateSpellEffectBaseAmount(value, m_spellInfo, 0);
            break;
        case SPELLVALUE_BASE_POINT1:
            m_spellValue->EffectBasePoints[1] = SpellMgr::CalculateSpellEffectBaseAmount(value, m_spellInfo, 1);
            break;
        case SPELLVALUE_BASE_POINT2:
            m_spellValue->EffectBasePoints[2] = SpellMgr::CalculateSpellEffectBaseAmount(value, m_spellInfo, 2);
            break;
        case SPELLVALUE_SCRIPTED_POINT0:
            m_spellValue->EffectScriptedPoints[0] = value;
            break;
        case SPELLVALUE_SCRIPTED_POINT1:
            m_spellValue->EffectScriptedPoints[1] = value;
            break;
        case SPELLVALUE_SCRIPTED_POINT2:
            m_spellValue->EffectScriptedPoints[2] = value;
            break;
        case SPELLVALUE_RADIUS_MOD:
            m_spellValue->RadiusMod = (float)value / 10000;
            break;
        case SPELLVALUE_MAX_TARGETS:
            m_spellValue->MaxAffectedTargets = (uint32)value;
            break;
    }
}

float tangent(float x)
{
    x = tan(x);
    //if (x < std::numeric_limits<float>::max() && x > -std::numeric_limits<float>::max()) return x;
    //if (x >= std::numeric_limits<float>::max()) return std::numeric_limits<float>::max();
    //if (x <= -std::numeric_limits<float>::max()) return -std::numeric_limits<float>::max();
    if (x < 100000.0f && x > -100000.0f) return x;
    if (x >= 100000.0f) return 100000.0f;
    if (x <= 100000.0f) return -100000.0f;
    return 0.0f;
}

#define DEBUG_TRAJ(a) //a

void Spell::SelectTrajTargets()
{
    if (!m_targets.HasTraj())
        return;

    float dist2d = m_targets.GetDist2d();
    if (!dist2d)
        return;

    float dz = m_targets.m_dstPos.m_positionZ - m_targets.m_srcPos.m_positionZ;

    UnitList unitList;
    SearchAreaTarget(unitList, dist2d, PUSH_IN_THIN_LINE, SPELL_TARGETS_ANY);
    if (unitList.empty())
        return;

    unitList.sort(Trinity::ObjectDistanceOrderPred(m_caster));

    float b = tangent(m_targets.m_elevation);
    float a = (dz - dist2d * b) / (dist2d * dist2d);
    if (a > -0.0001f) a = 0;
    DEBUG_TRAJ(sLog->outError("Spell::SelectTrajTargets: a %f b %f", a, b);)

    float bestDist = GetSpellMaxRange(m_spellInfo, false);

    UnitList::const_iterator itr = unitList.begin();
    for (; itr != unitList.end(); ++itr)
    {
        if (m_caster == *itr || m_caster->IsOnVehicle(*itr) || (*itr)->GetVehicle())//(*itr)->IsOnVehicle(m_caster))
            continue;

        const float size = std::max((*itr)->GetObjectSize() * 0.7f, 1.0f); // 1/sqrt(3)
        // TODO: all calculation should be based on src instead of m_caster
        const float objDist2d = m_targets.m_srcPos.GetExactDist2d(*itr) * cos(m_targets.m_srcPos.GetRelativeAngle(*itr));
        const float dz = (*itr)->GetPositionZ() - m_targets.m_srcPos.m_positionZ;

        DEBUG_TRAJ(sLog->outError("Spell::SelectTrajTargets: check %u, dist between %f %f, height between %f %f.", (*itr)->GetEntry(), objDist2d - size, objDist2d + size, dz - size, dz + size);)

        float dist = objDist2d - size;
        float height = dist * (a * dist + b);
        DEBUG_TRAJ(sLog->outError("Spell::SelectTrajTargets: dist %f, height %f.", dist, height);)
        if (dist < bestDist && height < dz + size && height > dz - size)
        {
            bestDist = dist > 0 ? dist : 0;
            break;
        }

#define CHECK_DIST {\
    DEBUG_TRAJ(sLog->outError("Spell::SelectTrajTargets: dist %f, height %f.", dist, height);)\
    if (dist > bestDist) continue;\
    if (dist < objDist2d + size && dist > objDist2d - size) { bestDist = dist; break; }\
        }

        if (!a)
        {
            height = dz - size;
            dist = height / b;
            CHECK_DIST;

            height = dz + size;
            dist = height / b;
            CHECK_DIST;

            continue;
        }

        height = dz - size;
        float sqrt1 = b * b + 4 * a * height;
        if (sqrt1 > 0)
        {
            sqrt1 = sqrt(sqrt1);
            dist = (sqrt1 - b) / (2 * a);
            CHECK_DIST;
        }

        height = dz + size;
        float sqrt2 = b * b + 4 * a * height;
        if (sqrt2 > 0)
        {
            sqrt2 = sqrt(sqrt2);
            dist = (sqrt2 - b) / (2 * a);
            CHECK_DIST;

            dist = (-sqrt2 - b) / (2 * a);
            CHECK_DIST;
        }

        if (sqrt1 > 0)
        {
            dist = (-sqrt1 - b) / (2 * a);
            CHECK_DIST;
        }
    }

    if (m_targets.m_srcPos.GetExactDist2d(&m_targets.m_dstPos) > bestDist)
    {
        float x = m_targets.m_srcPos.m_positionX + cos(m_caster->GetOrientation()) * bestDist;
        float y = m_targets.m_srcPos.m_positionY + sin(m_caster->GetOrientation()) * bestDist;
        float z = m_targets.m_srcPos.m_positionZ + bestDist * (a * bestDist + b);

        if (itr != unitList.end())
        {
            float distSq = (*itr)->GetExactDistSq(x, y, z);
            float sizeSq = (*itr)->GetObjectSize();
            sizeSq *= sizeSq;
            DEBUG_TRAJ(sLog->outError("Initial %f %f %f %f %f", x, y, z, distSq, sizeSq);)
            if (distSq > sizeSq)
            {
                float factor = 1 - sqrt(sizeSq / distSq);
                x += factor * ((*itr)->GetPositionX() - x);
                y += factor * ((*itr)->GetPositionY() - y);
                z += factor * ((*itr)->GetPositionZ() - z);

                distSq = (*itr)->GetExactDistSq(x, y, z);
                DEBUG_TRAJ(sLog->outError("Initial %f %f %f %f %f", x, y, z, distSq, sizeSq);)
            }
        }

        Position trajDst;
        trajDst.Relocate(x, y, z, m_caster->GetOrientation());
        m_targets.modDst(trajDst);
    }
}

void Spell::PrepareTargetProcessing()
{
    CheckEffectExecuteData();
}

void Spell::FinishTargetProcessing()
{
    SendLogExecute();
}

void Spell::InitEffectExecuteData(uint8 effIndex)
{
    ASSERT(effIndex < MAX_SPELL_EFFECTS);
    if (!m_effectExecuteData[effIndex])
    {
        m_effectExecuteData[effIndex] = new ByteBuffer(0x20);
        // first dword - target counter
        *m_effectExecuteData[effIndex] << uint32(1);
    }
    else
    {
        // increase target counter by one
        uint32 count = (*m_effectExecuteData[effIndex]).read<uint32>(0);
        (*m_effectExecuteData[effIndex]).put<uint32>(0, ++count);
    }
}

void Spell::CleanupEffectExecuteData()
{
    for(uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        m_effectExecuteData[i] = NULL;
}

void Spell::CheckEffectExecuteData()
{
    for(uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        ASSERT(!m_effectExecuteData[i]);
}

void Spell::LoadScripts()
{
    sScriptMgr->CreateSpellScripts(m_spellInfo->Id, m_loadedScripts);
    for(std::list<SpellScript *>::iterator itr = m_loadedScripts.begin(); itr != m_loadedScripts.end() ;)
    {
        if (!(*itr)->_Load(this))
        {
            std::list<SpellScript *>::iterator bitr = itr;
            ++itr;
            m_loadedScripts.erase(bitr);
            continue;
        }
        (*itr)->Register();
        ++itr;
    }
}

void Spell::PrepareScriptHitHandlers()
{
    for(std::list<SpellScript *>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end() ; ++scritr)
    {
        (*scritr)->_InitHit();
    }
}

bool Spell::CallScriptEffectHandlers(SpellEffIndex effIndex)
{
    // execute script effect handler hooks and check if effects was prevented
    bool preventDefault = false;
    for(std::list<SpellScript *>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end() ; ++scritr)
    {
        (*scritr)->_PrepareScriptCall(SPELL_SCRIPT_HOOK_EFFECT);
        std::list<SpellScript::EffectHandler>::iterator effEndItr = (*scritr)->OnEffect.end(), effItr = (*scritr)->OnEffect.begin();
        for(; effItr != effEndItr ; ++effItr)
        {
            // effect execution can be prevented
            if (!(*scritr)->_IsEffectPrevented(effIndex) && (*effItr).IsEffectAffected(m_spellInfo, effIndex))
                (*effItr).Call(*scritr, effIndex);
        }
        if (!preventDefault)
            preventDefault = (*scritr)->_IsDefaultEffectPrevented(effIndex);
        (*scritr)->_FinishScriptCall();
    }
    return preventDefault;
}

void Spell::CallScriptBeforeHitHandlers()
{
    for(std::list<SpellScript *>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end() ; ++scritr)
    {
        (*scritr)->_PrepareScriptCall(SPELL_SCRIPT_HOOK_BEFORE_HIT);
        std::list<SpellScript::HitHandler>::iterator hookItrEnd = (*scritr)->BeforeHit.end(), hookItr = (*scritr)->BeforeHit.begin();
        for(; hookItr != hookItrEnd ; ++hookItr)
        {
            (*hookItr).Call(*scritr);
        }
        (*scritr)->_FinishScriptCall();
    }
}

void Spell::CallScriptOnHitHandlers()
{
    for(std::list<SpellScript *>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end() ; ++scritr)
    {
        (*scritr)->_PrepareScriptCall(SPELL_SCRIPT_HOOK_HIT);
        std::list<SpellScript::HitHandler>::iterator hookItrEnd = (*scritr)->OnHit.end(), hookItr = (*scritr)->OnHit.begin();
        for(; hookItr != hookItrEnd ; ++hookItr)
        {
            (*hookItr).Call(*scritr);
        }
        (*scritr)->_FinishScriptCall();
    }
}

void Spell::CallScriptAfterHitHandlers()
{
    for(std::list<SpellScript *>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end() ; ++scritr)
    {
        (*scritr)->_PrepareScriptCall(SPELL_SCRIPT_HOOK_AFTER_HIT);
        std::list<SpellScript::HitHandler>::iterator hookItrEnd = (*scritr)->AfterHit.end(), hookItr = (*scritr)->AfterHit.begin();
        for(; hookItr != hookItrEnd ; ++hookItr)
        {
            (*hookItr).Call(*scritr);
        }
        (*scritr)->_FinishScriptCall();
    }
}

void Spell::CallScriptAfterUnitTargetSelectHandlers(std::list<Unit*>& unitTargets, SpellEffIndex effIndex)
{
    for(std::list<SpellScript *>::iterator scritr = m_loadedScripts.begin(); scritr != m_loadedScripts.end() ; ++scritr)
    {
        (*scritr)->_PrepareScriptCall(SPELL_SCRIPT_HOOK_UNIT_TARGET_SELECT);
        std::list<SpellScript::UnitTargetHandler>::iterator hookItrEnd = (*scritr)->OnUnitTargetSelect.end(), hookItr = (*scritr)->OnUnitTargetSelect.begin();
        for(; hookItr != hookItrEnd ; ++hookItr)
            if ((*hookItr).IsEffectAffected(m_spellInfo, effIndex))
            (*hookItr).Call(*scritr, unitTargets);

        (*scritr)->_FinishScriptCall();
    }
}
