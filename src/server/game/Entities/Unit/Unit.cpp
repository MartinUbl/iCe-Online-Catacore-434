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
#include "CreatureAIImpl.h"
#include "Log.h"
#include "Opcodes.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Unit.h"
#include "QuestDef.h"
#include "Player.h"
#include "Creature.h"
#include "Spell.h"
#include "Group.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "CreatureAI.h"
#include "Formulas.h"
#include "Pet.h"
#include "Util.h"
#include "Totem.h"
#include "Battleground.h"
#include "BattlefieldMgr.h"
#include "OutdoorPvP.h"
#include "InstanceSaveMgr.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "CreatureGroups.h"
#include "PetAI.h"
#include "PassiveAI.h"
#include "TemporarySummon.h"
#include "Vehicle.h"
#include "Transport.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "UpdateFieldFlags.h"
#include "InstanceScript.h"
#include "MapInstanced.h"
#include "ScriptDatabase.h"

#include <math.h>

float baseMoveSpeed[MAX_MOVE_TYPE] =
{
    2.5f,                  // MOVE_WALK
    7.0f,                  // MOVE_RUN
    4.5f,                  // MOVE_RUN_BACK
    4.722222f,             // MOVE_SWIM
    4.5f,                  // MOVE_SWIM_BACK
    3.141594f,             // MOVE_TURN_RATE
    7.0f,                  // MOVE_FLIGHT
    4.5f,                  // MOVE_FLIGHT_BACK
    3.14f                  // MOVE_PITCH_RATE
};
float playerBaseMoveSpeed[MAX_MOVE_TYPE] = {
    2.5f,                  // MOVE_WALK
    7.0f,                  // MOVE_RUN
    4.5f,                  // MOVE_RUN_BACK
    4.722222f,             // MOVE_SWIM
    4.5f,                  // MOVE_SWIM_BACK
    3.141594f,             // MOVE_TURN_RATE
    7.0f,                  // MOVE_FLIGHT
    4.5f,                  // MOVE_FLIGHT_BACK
    3.14f                  // MOVE_PITCH_RATE
};

uint32 preserve_spell_table[] = { // Perserve spell table
    15007,  // Ressurection Sickness
    26013,  // Deserter
    25771,  // Forbearance (Paladin debuff)
    11196,  // Recently Bandaged (First Aid)
    6788,   // Weakened Soul (Priest debuff)
    80354,  // Temporal Displacement (Time Wrap debuff)
    57724,  // Sated (Bloodlust debuff)
    57723,  // Exhaustion (Heroism debuff)
    95223,  // Recently Mass Resurrected (Mass Resurrection debuff)
    95809,  // Insanity (Ancient Histeria debuff)
};

// Used for prepare can/can`t triggr aura
static bool InitTriggerAuraData();
// Define can trigger auras
static bool isTriggerAura[TOTAL_AURAS];
// Define can`t trigger auras (need for disable second trigger)
static bool isNonTriggerAura[TOTAL_AURAS];
// Triggered always, even from triggered spells
static bool isAlwaysTriggeredAura[TOTAL_AURAS];
// Prepare lists
static bool procPrepared = InitTriggerAuraData();

// we can disable this warning for this since it only
// causes undefined behavior when passed to the base class constructor
#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif
Unit::Unit(): WorldObject(),
m_movedPlayer(NULL), IsAIEnabled(false), NeedChangeAI(false),
m_ControlledByPlayer(false), movespline(new Movement::MoveSpline()), i_AI(NULL), i_disabledAI(NULL), m_procDeep(0),
m_removedAurasCount(0), i_motionMaster(this), m_ThreatManager(this), m_vehicle(NULL),
m_vehicleKit(NULL), m_unitTypeMask(UNIT_MASK_NONE), m_HostileRefManager(this)
{
#ifdef _MSC_VER
#pragma warning(default:4355)
#endif
    m_objectType |= TYPEMASK_UNIT;
    m_objectTypeId = TYPEID_UNIT;

    m_updateFlag = UPDATEFLAG_HAS_LIVING;

    m_attackTimer[BASE_ATTACK] = 0;
    m_attackTimer[OFF_ATTACK] = 0;
    m_attackTimer[RANGED_ATTACK] = 0;
    m_modAttackSpeedPct[BASE_ATTACK] = 1.0f;
    m_modAttackSpeedPct[OFF_ATTACK] = 1.0f;
    m_modAttackSpeedPct[RANGED_ATTACK] = 1.0f;

    m_extraAttacks = 0;
    m_canDualWield = false;

    m_rootTimes = 0;

    m_state = 0;
    m_deathState = ALIVE;

    for (uint8 i = 0; i < CURRENT_MAX_SPELL; ++i)
        m_currentSpells[i] = NULL;

    m_addDmgOnce = 0;

    for (uint8 i = 0; i < MAX_SUMMON_SLOT; ++i)
        m_SummonSlot[i] = 0;

    m_ObjectSlot[0] = m_ObjectSlot[1] = m_ObjectSlot[2] = m_ObjectSlot[3] = 0;

    m_auraUpdateIterator = m_ownedAuras.end();
    m_Visibility = VISIBILITY_ON;

    m_interruptMask = 0;
    m_detectInvisibilityMask = 0;
    m_invisibilityMask = 0;
    m_transform = 0;
    m_canModifyStats = false;

    for (uint8 i = 0; i < MAX_SPELL_IMMUNITY; ++i)
        m_spellImmune[i].clear();
    for (uint8 i = 0; i < UNIT_MOD_END; ++i)
    {
        m_auraModifiersGroup[i][BASE_VALUE] = 0.0f;
        m_auraModifiersGroup[i][BASE_PCT] = 1.0f;
        m_auraModifiersGroup[i][TOTAL_VALUE] = 0.0f;
        m_auraModifiersGroup[i][TOTAL_PCT] = 1.0f;
    }
                                                            // implement 50% base damage from offhand
    m_auraModifiersGroup[UNIT_MOD_DAMAGE_OFFHAND][TOTAL_PCT] = 0.5f;

    for (uint8 i = 0; i < MAX_ATTACK; ++i)
    {
        m_weaponDamage[i][MINDAMAGE] = BASE_MINDAMAGE;
        m_weaponDamage[i][MAXDAMAGE] = BASE_MAXDAMAGE;
    }
    for (uint8 i = 0; i < MAX_STATS; ++i)
        m_createStats[i] = 0.0f;

    m_attacking = NULL;
    m_modMeleeHitChance = 0.0f;
    m_modRangedHitChance = 0.0f;
    m_modSpellHitChance = 0.0f;
    m_baseSpellCritChance = 5;

    m_CombatTimer = 0;
    m_LogonTimer = 0;

    m_personalPlayer = 0;

    //m_victimThreat = 0.0f;
    for (uint8 i = 0; i < MAX_SPELL_SCHOOL; ++i)
        m_threatModifier[i] = 1.0f;
    m_isSorted = true;
    for (uint8 i = 0; i < MAX_MOVE_TYPE; ++i)
        m_speed_rate[i] = 1.0f;

    m_charmInfo = NULL;
    //m_unit_movement_flags = 0;
    m_reducedThreatPercent = 0;
    m_misdirectionTargetGUID = 0;

    // remove aurastates allowing special moves
    for (uint8 i = 0; i < MAX_REACTIVE; ++i)
        m_reactiveTimer[i] = 0;

    m_cleanupDone = false;
    m_duringRemoveFromWorld = false;

    m_lDamageTakenHistory.clear();
}

Unit::~Unit()
{
    // set current spells as deletable
    for (uint8 i = 0; i < CURRENT_MAX_SPELL; ++i)
    {
        if (m_currentSpells[i])
        {
            m_currentSpells[i]->SetReferencedFromCurrent(false);
            m_currentSpells[i] = NULL;
        }
    }

    _DeleteRemovedAuras();

    delete m_charmInfo;
    delete m_vehicleKit;
    delete movespline;

    ASSERT(!m_duringRemoveFromWorld);
    ASSERT(!m_attacking);
}

void Unit::Update(uint32 p_time)
{
    // WARNING! Order of execution here is important, do not change.
    // Spells must be processed with event system BEFORE they go to _UpdateSpells.
    // Or else we may have some SPELL_STATE_FINISHED spells stalled in pointers, that is bad.
    m_LogonTimer += p_time;
    m_Events.Update(p_time);

    if (!IsInWorld())
        return;

    _UpdateSpells(p_time);

    // If this is set during update SetCantProc(false) call is missing somewhere in the code
    // Having this would prevent spells from being proced, so let's crash
    ASSERT(!m_procDeep);

    if (CanHaveThreatList() && getThreatManager().isNeedUpdateToClient(p_time))
        SendThreatListUpdate();

    // update combat timer only for players and pets (only pets with PetAI)
    if (IsInCombat() && (GetTypeId() == TYPEID_PLAYER || (ToCreature()->IsPet() && IsControlledByPlayer())))
    {
        // Check UNIT_STATE_MELEE_ATTACKING or UNIT_STATE_CHASE (without UNIT_STATE_FOLLOW in this case) so pets can reach far away
        // targets without stopping half way there and running off.
        // These flags are reset after target dies or another command is given.
        if (m_HostileRefManager.isEmpty())
        {
            // m_CombatTimer set at aura start and it will be freeze until aura removing
            if (m_CombatTimer <= p_time)
                ClearInCombat();
            else
                m_CombatTimer -= p_time;
        }
    }

    if (uint32 base_att = getAttackTimer(BASE_ATTACK))
        setAttackTimer(BASE_ATTACK, (p_time >= base_att ? 0 : base_att - p_time));
    if (uint32 ranged_att = getAttackTimer(RANGED_ATTACK))
        setAttackTimer(RANGED_ATTACK, (p_time >= ranged_att ? 0 : ranged_att - p_time));
    if (uint32 off_att = getAttackTimer(OFF_ATTACK))
        setAttackTimer(OFF_ATTACK, (p_time >= off_att ? 0 : off_att - p_time));

    // update abilities available only for fraction of time
    UpdateReactives(p_time);

    ModifyAuraState(AURA_STATE_HEALTHLESS_20_PERCENT, HealthBelowPct(20));
    ModifyAuraState(AURA_STATE_HEALTHLESS_35_PERCENT, HealthBelowPct(35));
    ModifyAuraState(AURA_STATE_HEALTH_ABOVE_75_PERCENT, HealthAbovePct(75));

    UpdateSplineMovement(p_time);
    i_motionMaster.UpdateMotion(p_time);
}

bool Unit::haveOffhandWeapon() const
{
    if (GetTypeId() == TYPEID_PLAYER)
        return this->ToPlayer()->GetWeaponForAttack(OFF_ATTACK,true);
    else
        return m_canDualWield;
}

void Unit::MonsterMoveWithSpeed(float x, float y, float z, float speed, bool generatePath, bool forceDestination)
{
    Movement::MoveSplineInit init(this);
    init.MoveTo(x, y, z, generatePath, forceDestination);
    init.SetVelocity(speed);
    init.Launch();
}

enum MovementIntervals
{
    POSITION_UPDATE_DELAY = 400,
};

void Unit::UpdateSplineMovement(uint32 t_diff)
{
    if (movespline->Finalized())
        return;

    movespline->updateState(t_diff);
    bool arrived = movespline->Finalized();

    if (arrived)
        DisableSpline();

    m_movesplineTimer.Update(t_diff);
    if (m_movesplineTimer.Passed() || arrived)
        UpdateSplinePosition();
}

void Unit::UpdateSplinePosition()
{
    uint32 const positionUpdateDelay = 400;

    m_movesplineTimer.Reset(positionUpdateDelay);
    Movement::Location loc = movespline->ComputePosition();
    if (GetTransGUID())
    {
        Position& pos = m_movementInfo.t_pos;
        pos.m_positionX = loc.x;
        pos.m_positionY = loc.y;
        pos.m_positionZ = loc.z;
        pos.SetOrientation(loc.orientation);

        if (Unit* vehicle = GetVehicleBase())
        {
            loc.x += vehicle->GetPositionX();
            loc.y += vehicle->GetPositionY();
            loc.z += vehicle->GetPositionZMinusOffset();
            loc.orientation = vehicle->GetOrientation();
        }
        else if (TransportBase* transport = GetDirectTransport())
                transport->CalculatePassengerPosition(loc.x, loc.y, loc.z, loc.orientation);
    }

    if (HasUnitState(UNIT_STATE_CANNOT_TURN))
        loc.orientation = GetOrientation();

    SetPosition(loc.x, loc.y, loc.z, loc.orientation);
}

bool Unit::IsSplineEnabled() const
{
    return movespline->Initialized() && !movespline->Finalized();
}

void Unit::DisableSpline()
{
    m_movementInfo.RemoveMovementFlag(MOVEMENTFLAG_FORWARD);
    movespline->_Interrupt();
}

void Unit::SendMonsterMoveTransport(Unit *vehicleOwner)
{
    // TODO: Turn into BuildMonsterMoveTransport packet and allow certain variables (for npc movement aboard vehicles)
    WorldPacket data(SMSG_MONSTER_MOVE_TRANSPORT, GetPackGUID().size()+vehicleOwner->GetPackGUID().size());
    data.append(GetPackGUID());
    data.append(vehicleOwner->GetPackGUID());
    data << int8(GetTransSeat());
    data << uint8(0);   // unk boolean
    data << GetPositionX() - vehicleOwner->GetPositionX();
    data << GetPositionY() - vehicleOwner->GetPositionY();
    data << GetPositionZ() - vehicleOwner->GetPositionZ();
    data << uint32(getMSTime());            // should be an increasing constant that indicates movement packet count
    data << uint8(SPLINETYPE_FACING_ANGLE); 
    data << GetOrientation();              // facing angle?
    data << uint32(SPLINEFLAG_TRANSPORT);
    data << uint32(GetTransTime());        // move time
    data << uint32(1);                     // amount of waypoints
    data << GetTransOffsetX();
    data << GetTransOffsetY();
    data << GetTransOffsetZ();
    SendMessageToSet(&data, true);
}

void Unit::resetAttackTimer(WeaponAttackType type)
{
    m_attackTimer[type] = uint32(GetAttackTime(type) * m_modAttackSpeedPct[type]);
}

bool Unit::IsWithinCombatRange(const Unit *obj, float dist2compare) const
{
    if (!obj || !IsInMap(obj)) return false;

    float dx = GetPositionX() - obj->GetPositionX();
    float dy = GetPositionY() - obj->GetPositionY();
    float dz = GetPositionZ() - obj->GetPositionZ();
    float distsq = dx*dx + dy*dy + dz*dz;

    float sizefactor = GetCombatReach() + obj->GetCombatReach();
    float maxdist = dist2compare + sizefactor;

    return distsq < maxdist * maxdist;
}

bool Unit::IsWithinMeleeRange(const Unit *obj, float dist) const
{
    if (!obj || !IsInMap(obj)) return false;

    float dx = GetPositionX() - obj->GetPositionX();
    float dy = GetPositionY() - obj->GetPositionY();
    float dz = GetPositionZ() - obj->GetPositionZ();
    float distsq = dx*dx + dy*dy + dz*dz;

    if (GetTypeId() == TYPEID_PLAYER && obj->GetTypeId() == TYPEID_UNIT)
    {
        if (obj->ToCreature()->HasInternalCombatReachSet())
        if (ToPlayer()->GetExactDist(obj) <= obj->ToCreature()->GetSavedCombatReach())
            return true;
    }

    float sizefactor = GetMeleeReach() + obj->GetMeleeReach();

    float maxdist = dist + sizefactor;

    return distsq < maxdist * maxdist;
}

void Unit::GetRandomContactPoint(const Unit* obj, float &x, float &y, float &z, float distance2dMin, float distance2dMax) const
{
    float combat_reach = GetCombatReach();
    if (combat_reach < 0.1) // sometimes bugged for players
    {
        //sLog->outError("Unit %u (Type: %u) has invalid combat_reach %f",GetGUIDLow(),GetTypeId(),combat_reach);
        //if (GetTypeId() == TYPEID_UNIT)
        //    sLog->outError("Creature entry %u has invalid combat_reach", this->ToCreature()->GetEntry());
        combat_reach = DEFAULT_COMBAT_REACH;
    }
    uint32 attacker_number = getAttackers().size();
    if (attacker_number > 0)
        --attacker_number;
    GetNearPoint(obj,x,y,z,obj->GetCombatReach(), distance2dMin+(distance2dMax-distance2dMin)*(float)rand_norm()
        , GetAngle(obj) + (attacker_number ? (static_cast<float>(M_PI/2) - static_cast<float>(M_PI) * (float)rand_norm()) * float(attacker_number) / combat_reach * 0.3f : 0));
}

void Unit::UpdateInterruptMask()
{
    m_interruptMask = 0;
    for (AuraApplicationList::const_iterator i = m_interruptableAuras.begin(); i != m_interruptableAuras.end(); ++i)
        m_interruptMask |= (*i)->GetBase()->GetSpellProto()->AuraInterruptFlags;

    if (Spell* spell = m_currentSpells[CURRENT_CHANNELED_SPELL])
        if (spell->getState() == SPELL_STATE_CASTING)
            m_interruptMask |= spell->m_spellInfo->ChannelInterruptFlags;
}

bool Unit::HasAuraTypeWithFamilyFlags(AuraType auraType, uint32 familyName, uint32 familyFlags) const
{
    if (!HasAuraType(auraType))
        return false;
    AuraEffectList const &auras = GetAuraEffectsByType(auraType);
    for (AuraEffectList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
        if (SpellEntry const *iterSpellProto = (*itr)->GetSpellProto())
            if (iterSpellProto->SpellFamilyName == familyName && iterSpellProto->SpellFamilyFlags[0] & familyFlags)
                return true;
    return false;
}

void Unit::DealDamageMods(Unit *pVictim, uint32 &damage, uint32* absorb)
{
    if (!pVictim->IsAlive() || pVictim->HasUnitState(UNIT_STATE_UNATTACKABLE) || (pVictim->GetTypeId() == TYPEID_UNIT && pVictim->ToCreature()->IsInEvadeMode()))
    {
        if (absorb)
            *absorb += damage;
        damage = 0;
        return;
    }

    //You don't lose health from damage taken from another player while in a sanctuary
    //You still see it in the combat log though
    if (pVictim != this && IsControlledByPlayer() && pVictim->IsControlledByPlayer())
    {
        const AreaTableEntry *area = GetAreaEntryByAreaID(pVictim->GetAreaId());
        if (area && area->IsSanctuary())      //sanctuary
        {
            if (absorb)
                *absorb += damage;
            damage = 0;
        }
    }

    uint32 originalDamage = damage;

    if (absorb && originalDamage > damage)
        *absorb += (originalDamage - damage);
}

uint32 Unit::DealDamage(Unit *pVictim, uint32 damage, CleanDamage const* cleanDamage, DamageEffectType damagetype, SpellSchoolMask damageSchoolMask, SpellEntry const *spellProto, bool durabilityLoss)
{
    if (pVictim->IsAIEnabled)
        pVictim->GetAI()->DamageTaken(this, damage);

    if (IsAIEnabled)
        GetAI()->DamageDealt(pVictim, damage, damagetype);

    if (damagetype != NODAMAGE)
    {
        // Improved Polymorph - needs to be handled before Polymorph removal
        if (GetTypeId() == TYPEID_PLAYER && (HasAura(11210) || HasAura(12592)) && pVictim->HasAura(118) && !HasAura(87515))
        {
            CastSpell(this, 87515, true); // Cooldown marker
            if (HasAura(11210))
                pVictim->CastSpell(pVictim, 83046, true); // stun #1
            else if (HasAura(12592))
                pVictim->CastSpell(pVictim, 83047, true); // stun #2
        }

        if (Aura* roar = pVictim->GetAura(53480)) // Roar of Sacrifice
        {
            Unit * pet = roar->GetCaster(); // Caster should be hunter's pet

            if (pet && pet->IsAlive())
            {
                Unit* pHunter = Unit::GetUnit(*pet, pet->GetOwnerGUID());
                int32 bp0 = (int32) ((damage * 20) / 100 ); // 20 % of damage caused is shared to pet

                if (pHunter && pHunter->IsAlive())
                    pHunter->CastCustomSpell(pet, 67481, &bp0, NULL, NULL, true);
            }
        }

        // Camouflage is removed by any damage delat to hunter or any damage done by pet or hunter itself
        if (pVictim->isCamouflaged())
            pVictim->RemoveCamouflage();
        else if (isCamouflaged())
            RemoveCamouflage();

        // Tricks of the Trade
        if (pVictim->HasAura(57934) && !pVictim->HasAura(59628) && !HasAura(57933))
        {
            if (!(spellProto && (spellProto->AttributesEx & SPELL_ATTR1_NO_THREAT)))
            {
                pVictim->CastSpell(pVictim, 59628, true);
                CastSpell(this, 57933, true);
            }
        }

        // interrupting auras with AURA_INTERRUPT_FLAG_DAMAGE before checking !damage (absorbed damage breaks that type of auras)
        pVictim->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TAKE_DAMAGE, spellProto ? spellProto->Id : 0);

        // We're going to call functions which can modify content of the list during iteration over it's elements
        // Let's copy the list so we can prevent iterator invalidation
        AuraEffectList vCopyDamageCopy(pVictim->GetAuraEffectsByType(SPELL_AURA_SHARE_DAMAGE_PCT));
        // copy damage to casters of this aura
        for (AuraEffectList::iterator i = vCopyDamageCopy.begin(); i != vCopyDamageCopy.end(); ++i)
        {
            // Check if aura was removed during iteration - we don't need to work on such auras
            if (!((*i)->GetBase()->IsAppliedOnTarget(pVictim->GetGUID())))
                continue;
            // check damage school mask
            if (((*i)->GetMiscValue() & damageSchoolMask) == 0)
                continue;

            Unit * shareDamageTarget = (*i)->GetCaster();
            if (!shareDamageTarget)
                continue;
            SpellEntry const * spell = (*i)->GetSpellProto();

            uint32 share = uint32(damage * (float((*i)->GetAmount()) / 100.0f));

            // TODO: check packets if damage is done by pVictim, or by attacker of pVictim
            DealDamageMods(shareDamageTarget, share, NULL);
            DealDamage(shareDamageTarget, share, NULL, NODAMAGE, GetSpellSchoolMask(spell), spell, false);
        }
    }

    // Implementation of "extra attacks" mastery proficiencies
    if (cleanDamage && (damagetype == DIRECT_DAMAGE || damagetype == SPELL_DIRECT_DAMAGE)
                    && (!spellProto || !(spellProto->AttributesEx4 & SPELL_ATTR4_TRIGGERED)))
    {
        uint32 spellId = 0;
        if (spellProto)
            spellId = spellProto->Id;       // do not allow to proc from itself

        if (!spellProto || spellProto->DmgClass == SPELL_DAMAGE_CLASS_MELEE)
        {
            if (ToPlayer() && ToPlayer()->HasMastery() && cleanDamage->attackType == BASE_ATTACK && cleanDamage->hitOutCome != MELEE_HIT_MISS)
            {
                // Implementation of Main Gauche rogue combat mastery proficiency
                if (ToPlayer()->GetTalentBranchSpec(ToPlayer()->GetActiveSpec()) == SPEC_ROGUE_COMBAT && spellId != 86392)
                {
                    if (roll_chance_f(ToPlayer()->GetMasteryPoints()*2.0f))
                        CastSpell(pVictim, 86392, true);
                }

                // Implementation of Strikes of Opportunity warrior arms mastery proficiency
                if (ToPlayer()->GetTalentBranchSpec(ToPlayer()->GetActiveSpec()) == SPEC_WARRIOR_ARMS && spellId != 76858)
                {
                    if ((!spellProto || spellProto->Id != 26654) && roll_chance_f(ToPlayer()->GetMasteryPoints()*2.2f))
                        CastSpell(pVictim, 76858, true);
                }
            }
        }
    }

    // Rage from Damage made (only from direct weapon damage)
    if (cleanDamage && damagetype == DIRECT_DAMAGE && this != pVictim && getPowerType() == POWER_RAGE)
    {
        float weaponSpeedHitFactor;
        uint32 rage_damage = damage + cleanDamage->absorbed_damage;

        switch(cleanDamage->attackType)
        {
            case BASE_ATTACK:
            {
                weaponSpeedHitFactor = GetAttackTime(cleanDamage->attackType) / 1000.0f * 6.5f;

                if (cleanDamage->hitOutCome != MELEE_HIT_MISS)
                    RewardRage(rage_damage, weaponSpeedHitFactor, true);

                break;
            }
            case OFF_ATTACK:
            {
                weaponSpeedHitFactor = GetAttackTime(cleanDamage->attackType) / 1000.0f * 3.25f;

                if (cleanDamage->hitOutCome != MELEE_HIT_MISS)
                    RewardRage(rage_damage, weaponSpeedHitFactor, true);

                break;
            }
            case RANGED_ATTACK:
                break;
            default:
                break;
        }
    }

    // interrupt player in BG if he is trying to take a flag
    if (Player *pl = pVictim->ToPlayer())
    {
        if (pl->InBattleground() &&
            (damagetype == DIRECT_DAMAGE || damagetype == SPELL_DIRECT_DAMAGE))
        {
            Spell *activeSpell = pl->GetCurrentSpell(CURRENT_GENERIC_SPELL);
            if (activeSpell && activeSpell->m_spellInfo && activeSpell->m_spellInfo->Id == 21651)
                pl->InterruptNonMeleeSpells(false);
        }
    }

    if (!damage)
    {
        // Rage from absorbed and mitigated damage
        if (cleanDamage && pVictim->getPowerType() == POWER_RAGE)
        {
            uint32 rage_damage = cleanDamage->absorbed_damage;
            if (damagetype != SPELL_DIRECT_DAMAGE)
                rage_damage += cleanDamage->mitigated_damage;
            if (rage_damage > 0)
                pVictim->RewardRage(rage_damage, 0, false, damagetype);
        }

        return 0;
    }

    sLog->outStaticDebug("DealDamageStart");

    uint32 health = pVictim->GetHealth();
    sLog->outDetail("deal dmg:%d to health:%d ",damage,health);

    // write clean damage to damagetaken map
    pVictim->DamageTakenByUnit(this, (damage>health)?health:damage);

    // duel ends when player has 1 or less hp
    bool duel_hasEnded = false;
    if (pVictim->GetTypeId() == TYPEID_PLAYER && pVictim->ToPlayer()->duel && damage >= (health-1))
    {
        // prevent kill only if killed in duel and killed by opponent or opponent controlled creature
        if (pVictim->ToPlayer()->duel->opponent == this || pVictim->ToPlayer()->duel->opponent->GetGUID() == GetOwnerGUID())
            damage = health - 1;

        duel_hasEnded = true;
    }

    if (GetTypeId() == TYPEID_PLAYER && this != pVictim)
    {
        Player *killer = this->ToPlayer();

        // in bg, count dmg if victim is also a player
        if (pVictim->GetTypeId() == TYPEID_PLAYER)
            if (Battleground *bg = killer->GetBattleground())
                bg->UpdatePlayerScore(killer, SCORE_DAMAGE_DONE, damage);

        killer->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE, damage, 0, pVictim);
        killer->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HIT_DEALT, damage);
    }

    if (pVictim->GetTypeId() == TYPEID_PLAYER)
        pVictim->ToPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HIT_RECEIVED, damage);
    else if (!pVictim->IsControlledByPlayer() || pVictim->IsVehicle())
    {
        if (!pVictim->ToCreature()->hasLootRecipient())
            pVictim->ToCreature()->SetLootRecipient(this);

        if (IsControlledByPlayer())
            pVictim->ToCreature()->LowerPlayerDamageReq(health < damage ?  health : damage);
    }

    // Bane of Havoc
    if (GetTypeId() == TYPEID_PLAYER)
    {
        if (HasAura(85466))
        {
            if (Aura* pAura = GetAura(85466))
            {
                if (Unit* pCaster = pAura->GetCaster())
                {
                    if (pVictim != pCaster)
                    {
                        int32 basepoints0 = damage*0.15f;
                        if(basepoints0 > 1)
                            CastCustomSpell(pCaster, 85455, &basepoints0, 0, 0, true);
                    }
                }
            }
        }
    }

    if (health <= damage)
    {
        sLog->outStaticDebug("DealDamage: victim just died");

        if (pVictim->GetTypeId() == TYPEID_PLAYER)
        {
            pVictim->ToPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_TOTAL_DAMAGE_RECEIVED, health);

            // call before auras are removed
            if (Player* killer = this->ToPlayer()) // keep the this-> for clarity
                killer->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL, 1, 0, pVictim);
        }

        Kill(pVictim, durabilityLoss);
    }
    else
    {
        sLog->outStaticDebug("DealDamageAlive");

        if (pVictim->GetTypeId() == TYPEID_PLAYER)
            pVictim->ToPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_TOTAL_DAMAGE_RECEIVED, damage);

        // Blood Craze
        if (pVictim->GetTypeId() == TYPEID_PLAYER)
        {
            if (pVictim->HasAura(16487)) // Rank 1
                if (roll_chance_f(10.0f))
                    pVictim->CastSpell(pVictim, 16488, true);

            if (pVictim->HasAura(16489)) // Rank 2
                if (roll_chance_f(10.0f))
                    pVictim->CastSpell(pVictim, 16490, true);

            if (pVictim->HasAura(16492)) // Rank 3
                if (roll_chance_f(10.0f))
                    pVictim->CastSpell(pVictim, 16491, true);
        }

        // Brain Freeze
        if (GetTypeId() == TYPEID_PLAYER)
        {
            if (damagetype == SPELL_DIRECT_DAMAGE)
            {
                if(GetSpellSchoolMask(spellProto) == SPELL_SCHOOL_MASK_FROST &&
                   spellProto->AppliesAuraType(SPELL_AURA_MOD_DECREASE_SPEED))
                {
                    if (this->ToPlayer()->HasAura(44546))
                        if (roll_chance_f(5.0f))
                            this->CastSpell(this, 57761, true);

                    if (this->ToPlayer()->HasAura(44548))
                        if (roll_chance_f(10.0f))
                            this->CastSpell(this, 57761, true);

                    if (this->ToPlayer()->HasAura(44549))
                        if (roll_chance_f(15.0f))
                            this->CastSpell(this, 57761, true);
                }
            }
        }

        // Nature's Ward
        if (pVictim->GetTypeId() == TYPEID_PLAYER && pVictim->ToPlayer()->getClass() == CLASS_DRUID
            && pVictim->ToPlayer()->GetSpellCooldownDelay(45281) == 0
            && (pVictim->GetMaxHealth()*0.5f) >= pVictim->GetHealth()-damage)
        {
            if (pVictim->HasAura(33882) || (pVictim->HasAura(33881) && roll_chance_i(50) ))
            {
                pVictim->CastSpell(pVictim, 774, true);
                pVictim->ToPlayer()->AddSpellCooldown(45281, 0, 6000);
            }
        }

        pVictim->ModifyHealth(- (int32)damage);

        if (damagetype == DIRECT_DAMAGE || damagetype == SPELL_DIRECT_DAMAGE)
            pVictim->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_DIRECT_DAMAGE, spellProto ? spellProto->Id : 0);

        if (pVictim->GetTypeId() != TYPEID_PLAYER)
        {
            if (spellProto && IsDamageToThreatSpell(spellProto))
                pVictim->AddThreat(this, damage * 2.0f, damageSchoolMask, spellProto);
            else
                pVictim->AddThreat(this, (float)damage, damageSchoolMask, spellProto);
        }
        else                                                // victim is a player
        {
            // random durability for items (HIT TAKEN)
            if (roll_chance_f(sWorld->getRate(RATE_DURABILITY_LOSS_DAMAGE)))
            {
                EquipmentSlots slot = EquipmentSlots(urand(0,EQUIPMENT_SLOT_END-1));
                pVictim->ToPlayer()->DurabilityPointLossForEquipSlot(slot);
            }
        }

        // Rage from damage received
        if (this != pVictim && pVictim->getPowerType() == POWER_RAGE)
        {
            uint32 rage_damage = damage;
            if (cleanDamage)
            {
                rage_damage += cleanDamage->absorbed_damage;
                if (damagetype != SPELL_DIRECT_DAMAGE)
                    rage_damage += cleanDamage->mitigated_damage;
            }

            pVictim->RewardRage(rage_damage, 0, false, damagetype);
        }

        if (GetTypeId() == TYPEID_PLAYER)
        {
            // random durability for items (HIT DONE)
            if (roll_chance_f(sWorld->getRate(RATE_DURABILITY_LOSS_DAMAGE)))
            {
                EquipmentSlots slot = EquipmentSlots(urand(0,EQUIPMENT_SLOT_END-1));
                this->ToPlayer()->DurabilityPointLossForEquipSlot(slot);
            }
        }

        if (damagetype != NODAMAGE && damage)
        {
            if (pVictim != this && pVictim->GetTypeId() == TYPEID_PLAYER) // does not support creature push_back
            {
                if (damagetype != DOT)
                {
                    if (Spell* spell = pVictim->m_currentSpells[CURRENT_GENERIC_SPELL])
                    {
                        if (spell->getState() == SPELL_STATE_PREPARING)
                        {
                            uint32 interruptFlags = spell->m_spellInfo->InterruptFlags;
                            if (interruptFlags & SPELL_INTERRUPT_FLAG_ABORT_ON_DMG)
                                pVictim->InterruptNonMeleeSpells(false);
                            else if (interruptFlags & SPELL_INTERRUPT_FLAG_PUSH_BACK)
                                spell->Delayed();
                        }
                    }
                }

                if (Spell* spell = pVictim->m_currentSpells[CURRENT_CHANNELED_SPELL])
                {
                    if (spell->getState() == SPELL_STATE_CASTING)
                    {
                        uint32 channelInterruptFlags = spell->m_spellInfo->ChannelInterruptFlags;
                        if (((channelInterruptFlags & CHANNEL_FLAG_DELAY) != 0) && (damagetype != DOT))
                            spell->DelayedChannel();
                    }
                }
            }
        }

        // last damage from duel opponent
        if (duel_hasEnded)
        {
            ASSERT(pVictim->GetTypeId() == TYPEID_PLAYER);
            Player *he = pVictim->ToPlayer();

            ASSERT(he->duel);

            he->SetHealth(1);

            he->duel->opponent->CombatStopWithPets(true);
            he->CombatStopWithPets(true);

            he->CastSpell(he, 7267, true);                  // beg
            he->DuelComplete(DUEL_WON);
        }
    }

    pVictim->SaveDamageTakenHistory(damage);

    sLog->outStaticDebug("DealDamageEnd returned %d damage", damage);

    return damage;
}

uint64 Unit::GetDamageTakenByUnit(uint64 guid)
{
    if (m_damageTakenMap.empty() || m_damageTakenMap.find(guid) == m_damageTakenMap.end())
        return 0;

    return m_damageTakenMap[guid];
}

uint64 Unit::GetDamageTakenByUnit(Unit* dealer)
{
    if (!dealer)
        return 0;

    return GetDamageTakenByUnit(dealer->GetGUID());
}

void Unit::DamageTakenByUnit(uint64 guid, uint64 damage)
{
    // just for creatures (for now)
    if (GetTypeId() == TYPEID_PLAYER)
        return;

    DamageMap::iterator itr = m_damageTakenMap.find(guid);
    if (itr == m_damageTakenMap.end())
        m_damageTakenMap[guid] = damage;
    else
        m_damageTakenMap[guid] += damage;
}

void Unit::DamageTakenByUnit(Unit* dealer, uint64 damage)
{
    if (dealer)
        DamageTakenByUnit(dealer->GetGUID(), damage);
}

void Unit::ClearDamageTakenByUnit(uint64 guid)
{
    if (m_damageTakenMap.find(guid) != m_damageTakenMap.end())
        m_damageTakenMap.erase(guid);
}

void Unit::ClearDamageTakenByUnit(Unit* dealer)
{
    if (dealer)
        ClearDamageTakenByUnit(dealer->GetGUID());
}

uint64 Unit::GetTotalDamageTaken()
{
    if (m_damageTakenMap.empty())
        return 0;

    uint64 sum = 0;
    for (DamageMap::const_iterator itr = m_damageTakenMap.begin(); itr != m_damageTakenMap.end(); ++itr)
        sum += (*itr).second;

    return sum;
}

void Unit::CastStop(uint32 except_spellid)
{
    for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_MAX_SPELL; i++)
        if (m_currentSpells[i] && m_currentSpells[i]->m_spellInfo->Id != except_spellid)
            InterruptSpell(CurrentSpellTypes(i),false);
}

void Unit::CastSpell(Unit* Victim, uint32 spellId, bool triggered, Item *castItem, AuraEffect const * triggeredByAura, uint64 originalCaster)
{
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);

    if (!spellInfo)
    {
        sLog->outError("CastSpell: unknown spell id %i by caster: %s %u)", spellId,(GetTypeId() == TYPEID_PLAYER ? "player (GUID:" : "creature (Entry:"),(GetTypeId() == TYPEID_PLAYER ? GetGUIDLow() : GetEntry()));
        return;
    }

    CastSpell(Victim,spellInfo,triggered,castItem,triggeredByAura, originalCaster);
}

void Unit::CastSpell(Unit* Victim,SpellEntry const *spellInfo, bool triggered, Item *castItem, AuraEffect const * triggeredByAura, uint64 originalCaster)
{
    if (!spellInfo)
    {
        sLog->outError("CastSpell: unknown spell by caster: %s %u)", (GetTypeId() == TYPEID_PLAYER ? "player (GUID:" : "creature (Entry:"),(GetTypeId() == TYPEID_PLAYER ? GetGUIDLow() : GetEntry()));
        return;
    }

    if (!originalCaster && GetTypeId() == TYPEID_UNIT && this->ToCreature()->IsTotem() && IsControlledByPlayer())
        if (Unit * owner = GetOwner())
            originalCaster=owner->GetGUID();

    SpellCastTargets targets;
    targets.setUnitTarget(Victim);

    if (castItem)
        sLog->outStaticDebug("WORLD: cast Item spellId - %i", spellInfo->Id);

    if (!originalCaster && triggeredByAura)
        originalCaster = triggeredByAura->GetCasterGUID();

    Spell *spell = new Spell(this, spellInfo, triggered, originalCaster);

    spell->m_CastItem = castItem;
    spell->prepare(&targets, triggeredByAura);
}

void Unit::CastCustomSpell(Unit* target, uint32 spellId, int32 const* bp0, int32 const* bp1, int32 const* bp2, bool triggered, Item *castItem, AuraEffect const * triggeredByAura, uint64 originalCaster)
{
    CustomSpellValues values;
    if (bp0)
        values.AddSpellMod(SPELLVALUE_BASE_POINT0, *bp0);
    if (bp1)
        values.AddSpellMod(SPELLVALUE_BASE_POINT1, *bp1);
    if (bp2)
        values.AddSpellMod(SPELLVALUE_BASE_POINT2, *bp2);
    CastCustomSpell(spellId, values, target, triggered, castItem, triggeredByAura, originalCaster);
}

void Unit::CastCustomSpell(Unit* target, uint32 spellId, int32 const* bp0, int32 const* bp1, int32 const* bp2, int32 const* sp0, int32 const* sp1, int32 const* sp2, bool triggered, Item *castItem, AuraEffect const * triggeredByAura, uint64 originalCaster)
{
    CustomSpellValues values;
    if (bp0)
        values.AddSpellMod(SPELLVALUE_BASE_POINT0, *bp0);
    if (bp1)
        values.AddSpellMod(SPELLVALUE_BASE_POINT1, *bp1);
    if (bp2)
        values.AddSpellMod(SPELLVALUE_BASE_POINT2, *bp2);

    if (sp0)
        values.AddSpellMod(SPELLVALUE_SCRIPTED_POINT0, *sp0);
    if (sp1)
        values.AddSpellMod(SPELLVALUE_SCRIPTED_POINT1, *sp1);
    if (sp2)
        values.AddSpellMod(SPELLVALUE_SCRIPTED_POINT2, *sp2);

    CastCustomSpell(spellId, values, target, triggered, castItem, triggeredByAura, originalCaster);
}

void Unit::CastCustomSpell(uint32 spellId, SpellValueMod mod, int32 value, Unit* target, bool triggered, Item *castItem, AuraEffect const * triggeredByAura, uint64 originalCaster)
{
    CustomSpellValues values;
    values.AddSpellMod(mod, value);
    CastCustomSpell(spellId, values, target, triggered, castItem, triggeredByAura, originalCaster);
}

void Unit::CastCustomSpell(uint32 spellId, CustomSpellValues const &value, Unit* Victim, bool triggered, Item *castItem, AuraEffect const * triggeredByAura, uint64 originalCaster)
{
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo)
    {
        sLog->outError("CastSpell: unknown spell id %i by caster: %s %u)", spellId,(GetTypeId() == TYPEID_PLAYER ? "player (GUID:" : "creature (Entry:"),(GetTypeId() == TYPEID_PLAYER ? GetGUIDLow() : GetEntry()));
        return;
    }

    SpellCastTargets targets;
    targets.setUnitTarget(Victim);

    if (!originalCaster && triggeredByAura)
        originalCaster = triggeredByAura->GetCasterGUID();

    Spell *spell = new Spell(this, spellInfo, triggered, originalCaster);

    if (castItem)
    {
        sLog->outStaticDebug("WORLD: cast Item spellId - %i", spellInfo->Id);
        spell->m_CastItem = castItem;
    }

    for (CustomSpellValues::const_iterator itr = value.begin(); itr != value.end(); ++itr)
        spell->SetSpellValue(itr->first, itr->second);

    spell->prepare(&targets, triggeredByAura);
}

void Unit::CastCustomSpell(uint32 spellId, CustomSpellValues const &value, Position &pos, bool triggered, Item *castItem, AuraEffect const * triggeredByAura, uint64 originalCaster)
{
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo)
    {
        sLog->outError("CastSpell: unknown spell id %i by caster: %s %u)", spellId,(GetTypeId() == TYPEID_PLAYER ? "player (GUID:" : "creature (Entry:"),(GetTypeId() == TYPEID_PLAYER ? GetGUIDLow() : GetEntry()));
        return;
    }

    SpellCastTargets targets;
    targets.setDst(pos);

    if (!originalCaster && triggeredByAura)
        originalCaster = triggeredByAura->GetCasterGUID();

    Spell *spell = new Spell(this, spellInfo, triggered, originalCaster);

    if (castItem)
    {
        sLog->outStaticDebug("WORLD: cast Item spellId - %i", spellInfo->Id);
        spell->m_CastItem = castItem;
    }

    for (CustomSpellValues::const_iterator itr = value.begin(); itr != value.end(); ++itr)
        spell->SetSpellValue(itr->first, itr->second);

    spell->prepare(&targets, triggeredByAura);
}

// used for scripting
void Unit::CastSpell(float x, float y, float z, uint32 spellId, bool triggered, Item *castItem, AuraEffect const * triggeredByAura, uint64 originalCaster, Unit* OriginalVictim)
{
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);

    if (!spellInfo)
    {
        sLog->outError("CastSpell(x,y,z): unknown spell id %i by caster: %s %u)", spellId,(GetTypeId() == TYPEID_PLAYER ? "player (GUID:" : "creature (Entry:"),(GetTypeId() == TYPEID_PLAYER ? GetGUIDLow() : GetEntry()));
        return;
    }

    if (castItem)
        sLog->outStaticDebug("WORLD: cast Item spellId - %i", spellInfo->Id);

    if (!originalCaster && triggeredByAura)
        originalCaster = triggeredByAura->GetCasterGUID();

    Spell *spell = new Spell(this, spellInfo, triggered, originalCaster);

    SpellCastTargets targets;
    targets.setDst(x, y, z, GetOrientation());
    if (OriginalVictim)
        targets.setUnitTarget(OriginalVictim);
    spell->m_CastItem = castItem;
    spell->prepare(&targets, triggeredByAura);
}

// used for scripting
void Unit::CastSpell(GameObject *go, uint32 spellId, bool triggered, Item *castItem, AuraEffect* triggeredByAura, uint64 originalCaster)
{
    if (!go)
        return;

    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);

    if (!spellInfo)
    {
        sLog->outError("CastSpell(x,y,z): unknown spell id %i by caster: %s %u)", spellId,(GetTypeId() == TYPEID_PLAYER ? "player (GUID:" : "creature (Entry:"),(GetTypeId() == TYPEID_PLAYER ? GetGUIDLow() : GetEntry()));
        return;
    }

    if (!(spellInfo->Targets & (TARGET_FLAG_OBJECT | TARGET_FLAG_OBJECT_CASTER)))
    {
        sLog->outError("CastSpell: spell id %i by caster: %s %u) is not gameobject spell", spellId,(GetTypeId() == TYPEID_PLAYER ? "player (GUID:" : "creature (Entry:"),(GetTypeId() == TYPEID_PLAYER ? GetGUIDLow() : GetEntry()));
        return;
    }

    if (castItem)
        sLog->outStaticDebug("WORLD: cast Item spellId - %i", spellInfo->Id);

    if (!originalCaster && triggeredByAura)
        originalCaster = triggeredByAura->GetCasterGUID();

    Spell *spell = new Spell(this, spellInfo, triggered, originalCaster);

    SpellCastTargets targets;
    targets.setGOTarget(go);
    spell->m_CastItem = castItem;
    spell->prepare(&targets, triggeredByAura);
}

// Obsolete func need remove, here only for comotability vs another patches
uint32 Unit::SpellNonMeleeDamageLog(Unit *pVictim, uint32 spellID, uint32 damage)
{
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellID);
    SpellNonMeleeDamage damageInfo(this, pVictim, spellInfo->Id, spellInfo->SchoolMask);
    damage = SpellDamageBonus(pVictim, spellInfo, EFFECT_0, damage, SPELL_DIRECT_DAMAGE);
    CalculateSpellDamageTaken(&damageInfo, damage, spellInfo);
    DealDamageMods(damageInfo.target,damageInfo.damage,&damageInfo.absorb);
    SendSpellNonMeleeDamageLog(&damageInfo);
    DealSpellDamage(&damageInfo, true);
    return damageInfo.damage;
}

void Unit::CalculateSpellDamageTaken(SpellNonMeleeDamage *damageInfo, int32 damage, SpellEntry const *spellInfo, WeaponAttackType attackType, bool crit)
{
    if (damage < 0)
        return;

    if (spellInfo->AttributesEx4 & SPELL_ATTR4_FIXED_DAMAGE)
    {
        Unit *pVictim = damageInfo->target;
        if (!pVictim || !pVictim->IsAlive())
            return;

        SpellSchoolMask damageSchoolMask = SpellSchoolMask(damageInfo->schoolMask);

        // Calculate absorb resist
        if (damage > 0)
        {
            CalcAbsorbResist(pVictim, damageSchoolMask, SPELL_DIRECT_DAMAGE, damage, &damageInfo->absorb, &damageInfo->resist, spellInfo);
            damage -= damageInfo->absorb + damageInfo->resist;
        }
        else
            damage = 0;

        damageInfo->damage = damage;
        return;
    }

    Unit *pVictim = damageInfo->target;
    if (!pVictim || !pVictim->IsAlive())
        return;

    SpellSchoolMask damageSchoolMask = SpellSchoolMask(damageInfo->schoolMask);
    uint32 crTypeMask = pVictim->GetCreatureTypeMask();

    if (IsDamageReducedByArmor(damageSchoolMask, spellInfo))
        damage = CalcArmorReducedDamage(pVictim, damage, spellInfo, attackType);

    bool blocked = false;
    // Per-school calc
    switch (spellInfo->DmgClass)
    {
        // Melee and Ranged Spells
        case SPELL_DAMAGE_CLASS_RANGED:
        case SPELL_DAMAGE_CLASS_MELEE:
            {
                // Physical Damage
                if (damageSchoolMask & SPELL_SCHOOL_MASK_NORMAL)
                {
                    // Get blocked status
                    blocked = isSpellBlocked(pVictim, spellInfo, attackType);
                }

                if (crit)
                {
                    damageInfo->HitInfo |= SPELL_HIT_TYPE_CRIT;

                    // Calculate crit bonus
                    uint32 crit_bonus = damage;
                    // Apply crit_damage bonus for melee spells
                    if (Player* modOwner = GetSpellModOwner())
                        modOwner->ApplySpellMod(spellInfo->Id, SPELLMOD_CRIT_DAMAGE_BONUS, crit_bonus);
                    damage += crit_bonus;

                    // Apply SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_DAMAGE or SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_DAMAGE
                    int32 critPctDamageMod = 0;
                    if (attackType == RANGED_ATTACK)
                        critPctDamageMod += pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_DAMAGE);
                    else
                    {
                        critPctDamageMod += pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_DAMAGE);
                        critPctDamageMod += GetTotalAuraModifier(SPELL_AURA_MOD_CRIT_DAMAGE_BONUS);
                    }
                    // Increase crit damage from SPELL_AURA_MOD_CRIT_PERCENT_VERSUS
                    critPctDamageMod += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_CRIT_PERCENT_VERSUS, crTypeMask);

                    if (critPctDamageMod != 0)
                        damage = int32(damage * float((100.0f + critPctDamageMod)/100.0f));
                }

                // Spell weapon based damage CAN BE crit & blocked at same time
                if (blocked)
                {
                    damageInfo->blocked = pVictim->CalculateBlockedAmount(damage);
                    damage -= damageInfo->blocked;
                }

                ApplyResilience(pVictim, &damage);
            }
            break;
        // Magical Attacks
        case SPELL_DAMAGE_CLASS_NONE:
        case SPELL_DAMAGE_CLASS_MAGIC:
            {
                // If crit add critical bonus
                if (crit)
                {
                    damageInfo->HitInfo |= SPELL_HIT_TYPE_CRIT;
                    damage = SpellCriticalDamageBonus(spellInfo, damage, pVictim);
                }

                ApplyResilience(pVictim, &damage);
            }
            break;
    }

    // Calculate absorb resist
    if (damage > 0)
    {
        CalcAbsorbResist(pVictim, damageSchoolMask, SPELL_DIRECT_DAMAGE, damage, &damageInfo->absorb, &damageInfo->resist, spellInfo);
        damage -= damageInfo->absorb + damageInfo->resist;
    }
    else
        damage = 0;


    damage = AfterAllSpellDamageComputation(spellInfo, damage, pVictim);
    damageInfo->damage = damage;
}

void Unit::DealSpellDamage(SpellNonMeleeDamage *damageInfo, bool durabilityLoss)
{
    if (damageInfo == 0)
        return;

    Unit *pVictim = damageInfo->target;

    if (!pVictim)
        return;

    if (!pVictim->IsAlive() || pVictim->HasUnitState(UNIT_STATE_UNATTACKABLE) || (pVictim->GetTypeId() == TYPEID_UNIT && pVictim->ToCreature()->IsInEvadeMode()))
        return;

    SpellEntry const *spellProto = sSpellStore.LookupEntry(damageInfo->SpellID);
    if (spellProto == NULL)
    {
        sLog->outDebug("Unit::DealSpellDamage have wrong damageInfo->SpellID: %u", damageInfo->SpellID);
        return;
    }

    //You don't lose health from damage taken from another player while in a sanctuary
    //You still see it in the combat log though
    if (pVictim != this && IsControlledByPlayer() && pVictim->IsControlledByPlayer())
    {
        const AreaTableEntry *area = GetAreaEntryByAreaID(pVictim->GetAreaId());

        if (area && area->IsSanctuary())       // sanctuary
            return;
    }

    // Implementation of Wild Quiver hunter marksmanship mastery proficiency
    if ((damageInfo->SpellID == 75 ||    // Auto-shot spell
         damageInfo->SpellID == 3044 ||  // Arcane Shot
         damageInfo->SpellID == 56641 || // Steady Shot
         damageInfo->SpellID == 19434 || // Aimed Shot
         damageInfo->SpellID == 82928 || // Aimed Shot (Master Marksman)
         damageInfo->SpellID == 53209 || // Chimera Shot
         damageInfo->SpellID == 2643)    // Multi-Shot
        &&
        ToPlayer() && ToPlayer()->HasMastery() &&
        ToPlayer()->GetTalentBranchSpec(ToPlayer()->GetActiveSpec()) == SPEC_HUNTER_MARKSMANSHIP)
    {
        if (roll_chance_f(ToPlayer()->GetMasteryPoints()*2.1f))
        {
            CastSpell(pVictim, 76663, true);
        }
    }

    // Implementation of Hand of Light mastery proficiency
    if (spellProto)
    {
        uint32 spellId = spellProto->Id;
        // Divine Storm, Crusader Strike, Templar's Verdict
        if (spellId == 53385 || spellId == 35395 || spellId == 85256)
        {
            Player *player = ToPlayer();
            if (player && player->HasMastery() &&
                player->GetTalentBranchSpec(player->GetActiveSpec()) == SPEC_PALADIN_RETRIBUTION)
            {
                uint32 damage = damageInfo->damage;
                float mastp = player->GetMasteryPoints();
                float coef = mastp*2.1f/100.0f;
                int32 bp0 = coef*float(damage);//float(damage)*(player->GetMasteryPoints()*2.1f/100.0f);
                CastCustomSpell(pVictim,96172,&bp0,0,0,true);
            }
        }
    }

    // Call default DealDamage
    CleanDamage cleanDamage(damageInfo->cleanDamage, damageInfo->absorb, BASE_ATTACK, MELEE_HIT_NORMAL);
    DealDamage(pVictim, damageInfo->damage, &cleanDamage, SPELL_DIRECT_DAMAGE, SpellSchoolMask(damageInfo->schoolMask), spellProto, durabilityLoss);
}

//TODO for melee need create structure as in
void Unit::CalculateMeleeDamage(Unit *pVictim, uint32 damage, CalcDamageInfo *damageInfo, WeaponAttackType attackType)
{
    damageInfo->attacker         = this;
    damageInfo->target           = pVictim;
    damageInfo->damageSchoolMask = GetMeleeDamageSchoolMask();
    damageInfo->attackType       = attackType;
    damageInfo->damage           = 0;
    damageInfo->cleanDamage      = 0;
    damageInfo->absorb           = 0;
    damageInfo->resist           = 0;
    damageInfo->blocked_amount   = 0;

    damageInfo->TargetState      = 0;
    damageInfo->HitInfo          = 0;
    damageInfo->procAttacker     = PROC_FLAG_NONE;
    damageInfo->procVictim       = PROC_FLAG_NONE;
    damageInfo->procEx           = PROC_EX_NONE;
    damageInfo->hitOutCome       = MELEE_HIT_EVADE;

    if (!pVictim)
        return;
    if (!IsAlive() || !pVictim->IsAlive())
        return;

    // Select HitInfo/procAttacker/procVictim flag based on attack type
    switch (attackType)
    {
        case BASE_ATTACK:
            damageInfo->procAttacker = PROC_FLAG_DONE_MELEE_AUTO_ATTACK | PROC_FLAG_DONE_MAINHAND_ATTACK;
            damageInfo->procVictim   = PROC_FLAG_TAKEN_MELEE_AUTO_ATTACK;
            damageInfo->HitInfo      = HITINFO_NORMALSWING2;
            break;
        case OFF_ATTACK:
            damageInfo->procAttacker = PROC_FLAG_DONE_MELEE_AUTO_ATTACK | PROC_FLAG_DONE_OFFHAND_ATTACK;
            damageInfo->procVictim   = PROC_FLAG_TAKEN_MELEE_AUTO_ATTACK;
            damageInfo->HitInfo = HITINFO_LEFTSWING;
            break;
        default:
            return;
    }

    // Physical Immune check
    if (damageInfo->target->IsImmunedToDamage(SpellSchoolMask(damageInfo->damageSchoolMask)))
    {
       damageInfo->HitInfo       |= HITINFO_NORMALSWING;
       damageInfo->TargetState    = VICTIMSTATE_IS_IMMUNE;

       damageInfo->procEx        |= PROC_EX_IMMUNE;
       damageInfo->damage         = 0;
       damageInfo->cleanDamage    = 0;
       return;
    }

    damage += CalculateDamage(damageInfo->attackType, false, true);
    // Add melee damage bonus
    MeleeDamageBonus(damageInfo->target, &damage, damageInfo->attackType);

    // Calculate armor reduction
    if (IsDamageReducedByArmor((SpellSchoolMask)(damageInfo->damageSchoolMask)))
    {
        damageInfo->damage = CalcArmorReducedDamage(damageInfo->target, damage, NULL , damageInfo->attackType);
        damageInfo->cleanDamage += damage - damageInfo->damage;
    }
    else
        damageInfo->damage = damage;

    damageInfo->hitOutCome = RollMeleeOutcomeAgainst(damageInfo->target, damageInfo->attackType);

    switch (damageInfo->hitOutCome)
    {
        case MELEE_HIT_EVADE:
            {
                damageInfo->HitInfo    |= HITINFO_MISS|HITINFO_SWINGNOHITSOUND;
                damageInfo->TargetState = VICTIMSTATE_EVADES;

                damageInfo->procEx|=PROC_EX_EVADE;
                damageInfo->damage = 0;
                damageInfo->cleanDamage = 0;
            }
            return;
        case MELEE_HIT_MISS:
            {
                damageInfo->HitInfo    |= HITINFO_MISS;
                damageInfo->TargetState = VICTIMSTATE_INTACT;

                damageInfo->procEx |= PROC_EX_MISS;
                damageInfo->damage  = 0;
                damageInfo->cleanDamage = 0;
            }
            break;
        case MELEE_HIT_NORMAL:
            damageInfo->TargetState = VICTIMSTATE_HIT;
            damageInfo->procEx|=PROC_EX_NORMAL_HIT;
            break;
        case MELEE_HIT_CRIT:
            {
                damageInfo->HitInfo     |= HITINFO_CRITICALHIT;
                damageInfo->TargetState  = VICTIMSTATE_HIT;

                damageInfo->procEx      |= PROC_EX_CRITICAL_HIT;
                // Crit bonus calc
                damageInfo->damage += damageInfo->damage;
                damageInfo->cleanDamage += damageInfo->cleanDamage;
                int32 mod = 0;
                // Apply SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_DAMAGE or SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_DAMAGE
                if (damageInfo->attackType == RANGED_ATTACK)
                    mod += damageInfo->target->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_DAMAGE);
                else
                {
                    mod += damageInfo->target->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_DAMAGE);
                    mod += GetTotalAuraModifier(SPELL_AURA_MOD_CRIT_DAMAGE_BONUS);
                }

                uint32 crTypeMask = damageInfo->target->GetCreatureTypeMask();

                // Increase crit damage from SPELL_AURA_MOD_CRIT_PERCENT_VERSUS
                mod += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_CRIT_PERCENT_VERSUS, crTypeMask);
                if (mod != 0)
                {
                    float coeff = float((100.0f + mod)/100.0f);
                    damageInfo->damage = int32((damageInfo->damage) * coeff);
                    damageInfo->cleanDamage = int32((damageInfo->cleanDamage) * coeff);
                }
            }
            break;
        case MELEE_HIT_PARRY:
            damageInfo->TargetState  = VICTIMSTATE_PARRY;
            damageInfo->procEx      |= PROC_EX_PARRY;
            damageInfo->cleanDamage += damageInfo->damage;
            damageInfo->damage = 0;
            break;
        case MELEE_HIT_DODGE:
            damageInfo->TargetState  = VICTIMSTATE_DODGE;
            damageInfo->procEx      |= PROC_EX_DODGE;
            damageInfo->cleanDamage += damageInfo->damage;
            damageInfo->damage = 0;
            break;
        case MELEE_HIT_BLOCK:
            {
                damageInfo->TargetState = VICTIMSTATE_HIT;
                damageInfo->HitInfo    |= HITINFO_BLOCK;
                damageInfo->procEx     |= PROC_EX_BLOCK;
                damageInfo->blocked_amount = damageInfo->target->CalculateBlockedAmount(damageInfo->damage);

                damageInfo->procEx  |= PROC_EX_NORMAL_HIT;
                damageInfo->damage      -= damageInfo->blocked_amount;
                damageInfo->cleanDamage += damageInfo->blocked_amount;
            }
            break;
        case MELEE_HIT_GLANCING:
            {
                damageInfo->HitInfo     |= HITINFO_GLANCING;
                damageInfo->TargetState  = VICTIMSTATE_HIT;
                damageInfo->procEx      |= PROC_EX_NORMAL_HIT;
                int32 leveldif = int32(pVictim->getLevel()) - int32(getLevel());
                if (leveldif > 3)
                    leveldif = 3;
                float reducePercent = 1 - leveldif * 0.1f;
                damageInfo->cleanDamage += damageInfo->damage-uint32(reducePercent * damageInfo->damage);
                damageInfo->damage   = uint32(reducePercent * damageInfo->damage);
            }
            break;
        case MELEE_HIT_CRUSHING:
            {
                damageInfo->HitInfo     |= HITINFO_CRUSHING;
                damageInfo->TargetState  = VICTIMSTATE_HIT;
                damageInfo->procEx      |= PROC_EX_NORMAL_HIT;
                // 150% normal damage
                damageInfo->damage += (damageInfo->damage / 2);
            }
        break;
        default:
            break;
    }

    int32 resilienceReduction = damageInfo->damage;
    ApplyResilience(pVictim, &resilienceReduction);
    resilienceReduction = damageInfo->damage - resilienceReduction;
    damageInfo->damage      -= resilienceReduction;
    damageInfo->cleanDamage += resilienceReduction;

    // Calculate absorb resist
    if (int32(damageInfo->damage) > 0)
    {
        damageInfo->procVictim |= PROC_FLAG_TAKEN_DAMAGE;
        // Calculate absorb & resists
        CalcAbsorbResist(damageInfo->target, SpellSchoolMask(damageInfo->damageSchoolMask), DIRECT_DAMAGE, damageInfo->damage, &damageInfo->absorb, &damageInfo->resist);
        damageInfo->damage -= damageInfo->absorb + damageInfo->resist;
        if (damageInfo->absorb)
        {
            damageInfo->HitInfo |= HITINFO_ABSORB;
            damageInfo->procEx  |= PROC_EX_ABSORB;
        }
        if (damageInfo->resist)
            damageInfo->HitInfo |= HITINFO_RESIST;
    }
    else // Impossible get negative result but....
        damageInfo->damage = 0;
}

void Unit::DealMeleeDamage(CalcDamageInfo *damageInfo, bool durabilityLoss)
{
    Unit *pVictim = damageInfo->target;

    if (!pVictim->IsAlive() || pVictim->HasUnitState(UNIT_STATE_UNATTACKABLE) || (pVictim->GetTypeId() == TYPEID_UNIT && pVictim->ToCreature()->IsInEvadeMode()))
        return;

    //You don't lose health from damage taken from another player while in a sanctuary
    //You still see it in the combat log though
    if (pVictim != this && IsControlledByPlayer() && pVictim->IsControlledByPlayer())
    {
        const AreaTableEntry *area = GetAreaEntryByAreaID(pVictim->GetAreaId());
        if (area && area->IsSanctuary())      // sanctuary
            return;
    }

    // Hmmmm dont like this emotes client must by self do all animations
    if (damageInfo->HitInfo&HITINFO_CRITICALHIT)
        pVictim->HandleEmoteCommand(EMOTE_ONESHOT_WOUNDCRITICAL);
    if (damageInfo->blocked_amount && damageInfo->TargetState != VICTIMSTATE_BLOCKS)
        pVictim->HandleEmoteCommand(EMOTE_ONESHOT_PARRYSHIELD);

    if (damageInfo->TargetState == VICTIMSTATE_PARRY)
    {
        // Get attack timers
        float offtime  = float(pVictim->getAttackTimer(OFF_ATTACK));
        float basetime = float(pVictim->getAttackTimer(BASE_ATTACK));
        // Reduce attack time
        if (pVictim->haveOffhandWeapon() && offtime < basetime)
        {
            float percent20 = pVictim->GetAttackTime(OFF_ATTACK) * 0.20f;
            float percent60 = 3.0f * percent20;
            if (offtime > percent20 && offtime <= percent60)
                pVictim->setAttackTimer(OFF_ATTACK, uint32(percent20));
            else if (offtime > percent60)
            {
                offtime -= 2.0f * percent20;
                pVictim->setAttackTimer(OFF_ATTACK, uint32(offtime));
            }
        }
        else
        {
            float percent20 = pVictim->GetAttackTime(BASE_ATTACK) * 0.20f;
            float percent60 = 3.0f * percent20;
            if (basetime > percent20 && basetime <= percent60)
                pVictim->setAttackTimer(BASE_ATTACK, uint32(percent20));
            else if (basetime > percent60)
            {
                basetime -= 2.0f * percent20;
                pVictim->setAttackTimer(BASE_ATTACK, uint32(basetime));
            }
        }
    }

    // Call default DealDamage
    CleanDamage cleanDamage(damageInfo->cleanDamage,damageInfo->absorb,damageInfo->attackType,damageInfo->hitOutCome);
    DealDamage(pVictim, damageInfo->damage, &cleanDamage, DIRECT_DAMAGE, SpellSchoolMask(damageInfo->damageSchoolMask), NULL, durabilityLoss);

    // If this is a creature and it attacks from behind it has a probability to daze it's victim
    if ((damageInfo->hitOutCome == MELEE_HIT_CRIT || damageInfo->hitOutCome == MELEE_HIT_CRUSHING || damageInfo->hitOutCome == MELEE_HIT_NORMAL || damageInfo->hitOutCome == MELEE_HIT_GLANCING) &&
        GetTypeId() != TYPEID_PLAYER && !this->ToCreature()->IsControlledByPlayer() && !pVictim->HasInArc(M_PI, this)
        && (pVictim->GetTypeId() == TYPEID_PLAYER || !pVictim->ToCreature()->isWorldBoss()))
    {
        // -probability is between 0% and 40%
        // 20% base chance
        float Probability = 20.0f;

        //there is a newbie protection, at level 10 just 7% base chance; assuming linear function
        if (pVictim->getLevel() < 30)
            Probability = 0.65f * pVictim->getLevel() + 0.5f;

        Probability *= ((float) getLevel()) / pVictim->getLevel();

        if (Probability > 40.0f)
            Probability = 40.0f;

        if (roll_chance_f(Probability))
            CastSpell(pVictim, 1604, true);
    }

    if (GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->CastItemCombatSpell(pVictim, damageInfo->attackType, damageInfo->procVictim, damageInfo->procEx);

    // Unleash Elements - Windfury part, drop charges after melee attacks
    if(Aura *uw = GetAura(73681))
        uw->DropCharge();

    // Do effect if any damage done to target
    if (damageInfo->damage)
    {
        // We're going to call functions which can modify content of the list during iteration over it's elements
        // Let's copy the list so we can prevent iterator invalidation
        AuraEffectList vDamageShieldsCopy(pVictim->GetAuraEffectsByType(SPELL_AURA_DAMAGE_SHIELD));
        for (AuraEffectList::const_iterator dmgShieldItr = vDamageShieldsCopy.begin(); dmgShieldItr != vDamageShieldsCopy.end(); ++dmgShieldItr)
        {
            SpellEntry const *i_spellProto = (*dmgShieldItr)->GetSpellProto();
            // Damage shield can be resisted...
            if (SpellMissInfo missInfo = pVictim->SpellHitResult(this, i_spellProto ,false))
            {
                pVictim->SendSpellMiss(this, i_spellProto->Id, missInfo);
                continue;
            }

            // ...or immuned
            if (IsImmunedToDamage(i_spellProto))
            {
                pVictim->SendSpellDamageImmune(this, i_spellProto->Id);
                continue;
            }

            uint32 damage = (*dmgShieldItr)->GetAmount();

            // No Unit::CalcAbsorbResist here - opcode doesn't send that data - this damage is probably not affected by that
            pVictim->DealDamageMods(this,damage,NULL);

            // TODO: Move this to a packet handler
            WorldPacket data(SMSG_SPELLDAMAGESHIELD,(8 + 8 + 4 + 4 + 4 + 4));
            data << uint64(pVictim->GetGUID());
            data << uint64(GetGUID());
            data << uint32(i_spellProto->Id);
            data << uint32(damage);                         // Damage
            int32 overkill = int32(damage) - int32(GetHealth());
            data << uint32(overkill > 0 ? overkill : 0);    // Overkill
            data << uint32(i_spellProto->SchoolMask);
            data << uint32(0);                              // 4.0.6

            pVictim->SendMessageToSet(&data, true);

            pVictim->DealDamage(this, damage, 0, SPELL_DIRECT_DAMAGE, GetSpellSchoolMask(i_spellProto), i_spellProto, true);
        }
    }
}

void Unit::HandleEmoteCommand(uint32 anim_id)
{
    WorldPacket data(SMSG_EMOTE, 4 + 8);
    data << uint32(anim_id);
    data << uint64(GetGUID());
    SendMessageToSet(&data, true);
}

bool Unit::IsDamageReducedByArmor(SpellSchoolMask schoolMask, SpellEntry const *spellInfo, uint8 effIndex)
{
    // only physical spells damage gets reduced by armor
    if ((schoolMask & SPELL_SCHOOL_MASK_NORMAL) == 0)
        return false;
    if (spellInfo)
    {
        // there are spells with no specific attribute but they have "ignores armor" in tooltip
        if (sSpellMgr->GetSpellCustomAttr(spellInfo->Id) & SPELL_ATTR0_CU_IGNORE_ARMOR)
            return false;

        // bleeding effects are not reduced by armor
        // as well as direct damage spells with bleed mechanic mask
        if (effIndex != MAX_SPELL_EFFECTS)
            if (spellInfo->EffectApplyAuraName[effIndex] == SPELL_AURA_PERIODIC_DAMAGE ||
                spellInfo->Effect[effIndex] == SPELL_EFFECT_SCHOOL_DAMAGE)
                if (GetSpellMechanicMask(spellInfo, effIndex) & (1<<MECHANIC_BLEED))
                    return false;
    }
    return true;
}

uint32 Unit::CalcArmorReducedDamage(Unit* pVictim, const uint32 damage, SpellEntry const *spellInfo, WeaponAttackType /*attackType*/)
{
    uint32 newdamage = 0;
    float armor = float(pVictim->GetArmor());

    // decrease enemy armor effectiveness by SPELL_AURA_BYPASS_ARMOR
    int32 auraEffectivenessReduction = 0;
    AuraEffectList const & reductionAuras = pVictim->GetAuraEffectsByType(SPELL_AURA_BYPASS_ARMOR);
    for (AuraEffectList::const_iterator i = reductionAuras.begin(); i != reductionAuras.end(); ++i)
        if ((*i)->GetCasterGUID() == GetGUID())
            auraEffectivenessReduction += (*i)->GetAmount();
    armor = CalculatePctN(armor, 100 - std::min(auraEffectivenessReduction, 100));

    // Ignore enemy armor by SPELL_AURA_MOD_TARGET_RESISTANCE aura
    armor += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_TARGET_RESISTANCE, SPELL_SCHOOL_MASK_NORMAL);

    if (spellInfo)
        if (Player *modOwner = GetSpellModOwner())
            modOwner->ApplySpellMod(spellInfo->Id, SPELLMOD_IGNORE_ARMOR, armor);

    AuraEffectList const& ResIgnoreAurasAb = GetAuraEffectsByType(SPELL_AURA_MOD_ABILITY_IGNORE_TARGET_RESIST);
    for (AuraEffectList::const_iterator j = ResIgnoreAurasAb.begin(); j != ResIgnoreAurasAb.end(); ++j)
    {
        if ((*j)->GetMiscValue() & SPELL_SCHOOL_MASK_NORMAL
            && (*j)->IsAffectedOnSpell(spellInfo))
            armor = floor(float(armor) * (float(100 - (*j)->GetAmount()) / 100.0f));
    }

    AuraEffectList const& ResIgnoreAuras = GetAuraEffectsByType(SPELL_AURA_MOD_IGNORE_TARGET_RESIST);
    for (AuraEffectList::const_iterator j = ResIgnoreAuras.begin(); j != ResIgnoreAuras.end(); ++j)
    {
        if ((*j)->GetMiscValue() & SPELL_SCHOOL_MASK_NORMAL)
            armor = floor(float(armor) * (float(100 - (*j)->GetAmount()) / 100.0f));
    }

    if (GetTypeId() == TYPEID_PLAYER)
    {
        AuraEffectList const& ResIgnoreAuras = GetAuraEffectsByType(SPELL_AURA_MOD_ARMOR_PENETRATION_PCT);
        for (AuraEffectList::const_iterator itr = ResIgnoreAuras.begin(); itr != ResIgnoreAuras.end(); ++itr)
        {
            // item neutral spell
            if ((*itr)->GetSpellProto()->EquippedItemClass == -1)
            {
                armor = floor(float(armor) * (float(100 - (*itr)->GetAmount()) / 100.0f));
                continue;
            }

            // item dependent spell - check curent weapons
            for (int i = 0; i < MAX_ATTACK; ++i)
            {
                Item *weapon = ToPlayer()->GetWeaponForAttack(WeaponAttackType(i), true);

                if (weapon && weapon->IsFitToSpellRequirements((*itr)->GetSpellProto()))
                {
                    armor = floor(float(armor) * (float(100 - (*itr)->GetAmount()) / 100.0f));
                    break;
                }
            }
        }
    }

    // Apply Player CR_ARMOR_PENETRATION rating
    if (GetTypeId() == TYPEID_PLAYER)
    {
        float maxArmorPen=0;
        if (getLevel() < 60)
            maxArmorPen = float(400 + 85 * pVictim->getLevel());
        else
            maxArmorPen = 400 + 85 * pVictim->getLevel() + 4.5f * 85 * (pVictim->getLevel() - 59);
        // Cap armor penetration to this number
        maxArmorPen = std::min(((armor+maxArmorPen) / 3),armor);
        // Figure out how much armor do we ignore
        float armorPen = maxArmorPen * this->ToPlayer()->GetRatingBonusValue(CR_ARMOR_PENETRATION) / 100.0f;
        // Got the value, apply it
        armor -= armorPen;
    }

    if (armor < 0.0f)
        armor = 0.0f;

    float armorReduction = armor / (armor + 85.f * getLevel() + 400.f);
    if(getLevel() > 59)
        armorReduction =   armor / (armor + 467.5f * getLevel() - 22167.5f);
    if(getLevel() > 80)
        armorReduction =   armor / (armor + 2167.5f * getLevel() - 158167.5f);

    if (armorReduction < 0.0f)
        armorReduction = 0.0f;
    if (armorReduction > 0.75f)
        armorReduction = 0.75f;

    newdamage = uint32(damage - (damage * armorReduction));

    return (newdamage > 1) ? newdamage : 1;
}

uint32 Unit::CalcSpellResistance(Unit* victim, SpellSchoolMask schoolMask, SpellEntry const* spellInfo) const
{
    // Magic damage, check for resists
    if (!(schoolMask & SPELL_SCHOOL_MASK_SPELL))
        return 0;

    // Ignore spells that can't be resisted
    if (spellInfo && spellInfo->AttributesEx4 & SPELL_ATTR4_IGNORE_RESISTANCES)
        return 0;

    uint8 const bossLevel = 85;
    uint32 const bossResistanceConstant = 510;
    uint32 resistanceConstant = 0;
    uint8 level = victim->getLevel();

    if (level == bossLevel)
        resistanceConstant = bossResistanceConstant;
    else
        resistanceConstant = level * 5;

    int32 baseVictimResistance = victim->GetResistance(schoolMask);
    baseVictimResistance += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_TARGET_RESISTANCE, schoolMask);

    if (Player const* player = ToPlayer())
        baseVictimResistance -= player->GetSpellPenetrationItemMod();

    // Resistance can't be lower then 0
    int32 victimResistance = std::max<int32>(baseVictimResistance, 0);

    if (victimResistance > 0)
    {
        int32 ignoredResistance = 0;

        AuraEffectList const& ResIgnoreAuras = GetAuraEffectsByType(SPELL_AURA_MOD_IGNORE_TARGET_RESIST);
        for (AuraEffectList::const_iterator itr = ResIgnoreAuras.begin(); itr != ResIgnoreAuras.end(); ++itr)
            if ((*itr)->GetMiscValue() & schoolMask)
                ignoredResistance += (*itr)->GetAmount();

        ignoredResistance = std::min<int32>(ignoredResistance, 100);
        ApplyPctN(victimResistance, 100 - ignoredResistance);
    }

    if (victimResistance <= 0)
        return 0;

    float averageResist = float(victimResistance) / float(victimResistance + resistanceConstant);

    float discreteResistProbability[11];
    for (uint32 i = 0; i < 11; ++i)
    {
        discreteResistProbability[i] = 0.5f - 2.5f * fabs(0.1f * i - averageResist);
        if (discreteResistProbability[i] < 0.0f)
            discreteResistProbability[i] = 0.0f;
    }

    if (averageResist <= 0.1f)
    {
        discreteResistProbability[0] = 1.0f - 7.5f * averageResist;
        discreteResistProbability[1] = 5.0f * averageResist;
        discreteResistProbability[2] = 2.5f * averageResist;
    }

    uint32 resistance = 0;
    float r = float(rand_norm());
    float probabilitySum = discreteResistProbability[0];

    while (r >= probabilitySum && resistance < 10)
        probabilitySum += discreteResistProbability[++resistance];

    return resistance * 10;
}

uint32 Unit::GetAbsorbedDamageFromSpell(Unit* pVictim, SpellEntry const* spellInfo) const
{
    if (!pVictim || !pVictim->IsAlive() || !spellInfo)
        return 0;

    SpellSchoolMask schoolMask = GetSpellSchoolMask(spellInfo);

    uint32 totalDamageAbsorbed = 0;

    AuraEffectList const & AbsIgnoreAurasB = GetAuraEffectsByType(SPELL_AURA_MOD_TARGET_ABILITY_ABSORB_SCHOOL);
    for (AuraEffectList::const_iterator itr = AbsIgnoreAurasB.begin(); itr != AbsIgnoreAurasB.end(); ++itr)
    {
        if (!((*itr)->GetMiscValue() & schoolMask))
            continue;
        // Quick check - > only one spell (58284 Chaos Bolt Passive)
        if (((*itr)->GetAmount() > 0) && (*itr)->IsAffectedOnSpell(spellInfo))
            return 0;
    }

    // We're going to call functions which can modify content of the list during iteration over it's elements
    // Let's copy the list so we can prevent iterator invalidation
    AuraEffectList vSchoolAbsorbCopy(pVictim->GetAuraEffectsByType(SPELL_AURA_SCHOOL_ABSORB));
    vSchoolAbsorbCopy.sort(Trinity::AbsorbAuraOrderPred());

    // absorb without mana cost
    for (AuraEffectList::iterator itr = vSchoolAbsorbCopy.begin(); itr != vSchoolAbsorbCopy.end(); ++itr)
    {
        AuraEffect * absorbAurEff = (*itr);
        // Check if aura was removed during iteration - we don't need to work on such auras
        AuraApplication * aurApp = absorbAurEff->GetBase()->GetApplicationOfTarget(pVictim->GetGUID());
        if (!aurApp)
            continue;
        if (!(absorbAurEff->GetMiscValue() & schoolMask))
            continue;

        // get amount which can be still absorbed by the aura
        int32 currentAbsorb = absorbAurEff->GetAmount();
        // aura with infinite absorb amount - let the scripts handle absorbtion amount, set here to 0 for safety
        if (currentAbsorb < 0)
            currentAbsorb = 0;

        totalDamageAbsorbed += currentAbsorb;
    }

    // absorb by mana cost
    AuraEffectList vManaShieldCopy(pVictim->GetAuraEffectsByType(SPELL_AURA_MANA_SHIELD));
    for (AuraEffectList::const_iterator itr = vManaShieldCopy.begin(); itr != vManaShieldCopy.end(); ++itr)
    {
        AuraEffect * absorbAurEff = (*itr);
        // Check if aura was removed during iteration - we don't need to work on such auras
        AuraApplication * aurApp = absorbAurEff->GetBase()->GetApplicationOfTarget(pVictim->GetGUID());
        if (!aurApp)
            continue;
        // check damage school mask
        if (!(absorbAurEff->GetMiscValue() & schoolMask))
            continue;

        // get amount which can be still absorbed by the aura
        int32 currentAbsorb = absorbAurEff->GetAmount();
        // aura with infinite absorb amount - let the scripts handle absorbtion amount, set here to 0 for safety
        if (currentAbsorb < 0)
            currentAbsorb = 0;

        totalDamageAbsorbed += currentAbsorb;
    }

    return totalDamageAbsorbed;
}

void Unit::CalcAbsorbResist(Unit *pVictim, SpellSchoolMask schoolMask, DamageEffectType damagetype, const uint32 damage, uint32 *absorb, uint32 *resist, SpellEntry const *spellInfo)
{
    if (!pVictim || !pVictim->IsAlive() || !damage)
        return;

    DamageInfo dmgInfo = DamageInfo(this, pVictim, damage, spellInfo, schoolMask, damagetype);

    uint32 spellResistance = CalcSpellResistance(pVictim, schoolMask, spellInfo);
    dmgInfo.ResistDamage(CalculatePctN(damage, spellResistance));

    // Ignore Absorption Auras
    float auraAbsorbMod = 0;
    AuraEffectList const & AbsIgnoreAurasA = GetAuraEffectsByType(SPELL_AURA_MOD_TARGET_ABSORB_SCHOOL);
    for (AuraEffectList::const_iterator itr = AbsIgnoreAurasA.begin(); itr != AbsIgnoreAurasA.end(); ++itr)
    {
        if (!((*itr)->GetMiscValue() & schoolMask))
            continue;

        if ((*itr)->GetAmount() > auraAbsorbMod)
            auraAbsorbMod = float((*itr)->GetAmount());
    }

    AuraEffectList const & AbsIgnoreAurasB = GetAuraEffectsByType(SPELL_AURA_MOD_TARGET_ABILITY_ABSORB_SCHOOL);
    for (AuraEffectList::const_iterator itr = AbsIgnoreAurasB.begin(); itr != AbsIgnoreAurasB.end(); ++itr)
    {
        if (!((*itr)->GetMiscValue() & schoolMask))
            continue;

        if (((*itr)->GetAmount() > auraAbsorbMod) && (*itr)->IsAffectedOnSpell(spellInfo))
            auraAbsorbMod = float((*itr)->GetAmount());
    }
    RoundToInterval(auraAbsorbMod, 0.0f, 100.0f);

    // We're going to call functions which can modify content of the list during iteration over it's elements
    // Let's copy the list so we can prevent iterator invalidation
    AuraEffectList vSchoolAbsorbCopy(pVictim->GetAuraEffectsByType(SPELL_AURA_SCHOOL_ABSORB));
    vSchoolAbsorbCopy.sort(Trinity::AbsorbAuraOrderPred());

    // absorb without mana cost
    for (AuraEffectList::iterator itr = vSchoolAbsorbCopy.begin(); (itr != vSchoolAbsorbCopy.end()) && (dmgInfo.GetDamage() > 0); ++itr)
    {
        AuraEffect * absorbAurEff = (*itr);
        // Check if aura was removed during iteration - we don't need to work on such auras
        AuraApplication * aurApp = absorbAurEff->GetBase()->GetApplicationOfTarget(pVictim->GetGUID());
        if (!aurApp)
            continue;
        if (!(absorbAurEff->GetMiscValue() & schoolMask))
            continue;

        // get amount which can be still absorbed by the aura
        int32 currentAbsorb = absorbAurEff->GetAmount();
        // aura with infinite absorb amount - let the scripts handle absorbtion amount, set here to 0 for safety
        if (currentAbsorb < 0)
            currentAbsorb = 0;

        uint32 absorb = currentAbsorb;

        bool defaultPrevented = false;

        absorbAurEff->GetBase()->CallScriptEffectAbsorbHandlers(absorbAurEff, aurApp, dmgInfo, absorb, defaultPrevented);
        currentAbsorb = absorb;

        if (defaultPrevented)
            continue;

        // Apply absorb mod auras
        AddPctF(currentAbsorb, -auraAbsorbMod);

        // absorb must be smaller than the damage itself
        currentAbsorb = RoundToInterval(currentAbsorb, 0, int32(dmgInfo.GetDamage()));

        dmgInfo.AbsorbDamage(currentAbsorb);

        // All warlocks absorb auras should trigger Nether Protection if talent present
        if (currentAbsorb && pVictim && pVictim->GetTypeId() == TYPEID_PLAYER && aurApp->GetBase()->GetSpellProto() && spellInfo
            && aurApp->GetBase()->GetSpellProto()->AppliesAuraType(SPELL_AURA_SCHOOL_ABSORB)
            && aurApp->GetBase()->GetSpellProto()->SpellFamilyName == SPELLFAMILY_WARLOCK)
        {
            int32 bp0 = 0;
            if (pVictim->HasAura(30301))
                bp0 = -30;
            else if (pVictim->HasAura(30299))
                bp0 = -15;

            if (bp0)
            {
                if (spellInfo->SchoolMask & (1 << SPELL_SCHOOL_ARCANE))
                    pVictim->CastCustomSpell(pVictim, 54373, &bp0, 0, 0, true);
                if (spellInfo->SchoolMask & (1 << SPELL_SCHOOL_FIRE))
                    pVictim->CastCustomSpell(pVictim, 54371, &bp0, 0, 0, true);
                if (spellInfo->SchoolMask & (1 << SPELL_SCHOOL_FROST))
                    pVictim->CastCustomSpell(pVictim, 54372, &bp0, 0, 0, true);
                if (spellInfo->SchoolMask & (1 << SPELL_SCHOOL_HOLY))
                    pVictim->CastCustomSpell(pVictim, 54370, &bp0, 0, 0, true);
                if (spellInfo->SchoolMask & (1 << SPELL_SCHOOL_NATURE))
                    pVictim->CastCustomSpell(pVictim, 54375, &bp0, 0, 0, true);
                if (spellInfo->SchoolMask & (1 << SPELL_SCHOOL_SHADOW))
                    pVictim->CastCustomSpell(pVictim, 54374, &bp0, 0, 0, true);
            }
        }

        absorb = currentAbsorb;
        absorbAurEff->GetBase()->CallScriptEffectAfterAbsorbHandlers(absorbAurEff, aurApp, dmgInfo, absorb);

        // Check if our aura is using amount to count damage
        if (absorbAurEff->GetAmount() >= 0)
        {
            // Reduce shield amount
            absorbAurEff->ChangeAmount(absorbAurEff->GetAmount() - currentAbsorb);
            // Aura cannot absorb anything more - remove it
            if (absorbAurEff->GetAmount() <= 0)
            {
                absorbAurEff->SetAmount(0);
                if (aurApp->GetSlot() != MAX_AURAS)
                    aurApp->ClientUpdate(false);
                absorbAurEff->GetBase()->Remove(AURA_REMOVE_BY_ENEMY_SPELL);
            }
            else     // Aura can absorb more - update value in client tooltip
                absorbAurEff->GetBase()->SetNeedClientUpdateForTargets();
        }
    }

    // absorb by mana cost
    AuraEffectList vManaShieldCopy(pVictim->GetAuraEffectsByType(SPELL_AURA_MANA_SHIELD));
    for (AuraEffectList::const_iterator itr = vManaShieldCopy.begin(); (itr != vManaShieldCopy.end()) && (dmgInfo.GetDamage() > 0); ++itr)
    {
        AuraEffect * absorbAurEff = (*itr);
        // Check if aura was removed during iteration - we don't need to work on such auras
        AuraApplication * aurApp = absorbAurEff->GetBase()->GetApplicationOfTarget(pVictim->GetGUID());
        if (!aurApp)
            continue;
        // check damage school mask
        if (!(absorbAurEff->GetMiscValue() & schoolMask))
            continue;

        // get amount which can be still absorbed by the aura
        int32 currentAbsorb = absorbAurEff->GetAmount();
        // aura with infinite absorb amount - let the scripts handle absorbtion amount, set here to 0 for safety
        if (currentAbsorb < 0)
            currentAbsorb = 0;

        uint32 absorb = currentAbsorb;

        bool defaultPrevented = false;

        absorbAurEff->GetBase()->CallScriptEffectManaShieldHandlers(absorbAurEff, aurApp, dmgInfo, absorb, defaultPrevented);
        currentAbsorb = absorb;

        if (defaultPrevented)
            continue;

        AddPctF(currentAbsorb, -auraAbsorbMod);

        // absorb must be smaller than the damage itself
        currentAbsorb = RoundToInterval(currentAbsorb, 0, int32(dmgInfo.GetDamage()));

        int32 manaReduction = currentAbsorb;

        // lower absorb amount by talents
        if (float manaMultiplier = SpellMgr::CalculateSpellEffectValueMultiplier(absorbAurEff->GetSpellProto(), absorbAurEff->GetEffIndex(), absorbAurEff->GetCaster()))
            manaReduction = int32(float(manaReduction) * manaMultiplier);

        int32 manaTaken = -pVictim->ModifyPower(POWER_MANA, -manaReduction);

        // take case when mana has ended up into account
        currentAbsorb = currentAbsorb ? int32(float(currentAbsorb)*(float(manaTaken) / float(manaReduction))) : 0;

        dmgInfo.AbsorbDamage(currentAbsorb);

        absorb = currentAbsorb;
        absorbAurEff->GetBase()->CallScriptEffectAfterManaShieldHandlers(absorbAurEff, aurApp, dmgInfo, absorb);

        // Check if our aura is using amount to count damage
        if (absorbAurEff->GetAmount() >= 0)
        {
            absorbAurEff->ChangeAmount(absorbAurEff->GetAmount() - currentAbsorb);
            if ((absorbAurEff->GetAmount() <= 0))
            {
                absorbAurEff->SetAmount(0);
                if (aurApp->GetSlot() != MAX_AURAS)
                    aurApp->ClientUpdate(false);
                absorbAurEff->GetBase()->Remove(AURA_REMOVE_BY_ENEMY_SPELL);
            }
            else
                absorbAurEff->GetBase()->SetNeedClientUpdateForTargets();
        }
    }

    // split damage auras - only when not damaging self
    if (pVictim != this)
    {
        // We're going to call functions which can modify content of the list during iteration over it's elements
        // Let's copy the list so we can prevent iterator invalidation
        AuraEffectList vSplitDamageFlatCopy(pVictim->GetAuraEffectsByType(SPELL_AURA_SPLIT_DAMAGE_FLAT));
        for (AuraEffectList::iterator itr = vSplitDamageFlatCopy.begin(); (itr != vSplitDamageFlatCopy.end()) && (dmgInfo.GetDamage() > 0); ++itr)
        {
            // Check if aura was removed during iteration - we don't need to work on such auras
            if (!((*itr)->GetBase()->IsAppliedOnTarget(pVictim->GetGUID())))
                continue;
            // check damage school mask
            if (!((*itr)->GetMiscValue() & schoolMask))
                continue;

            // Damage can be splitted only if aura has an alive caster
            Unit * caster = (*itr)->GetCaster();
            if (!caster || (caster == pVictim) || !caster->IsInWorld() || !caster->IsAlive())
                continue;

            int32 splitDamage = (*itr)->GetAmount();

            // absorb must be smaller than the damage itself
            splitDamage = RoundToInterval(splitDamage, 0, int32(dmgInfo.GetDamage()));

            dmgInfo.AbsorbDamage(splitDamage);

            uint32 splitted = splitDamage;
            uint32 splitted_absorb = 0;
            DealDamageMods(caster, splitted, &splitted_absorb);

            SendSpellNonMeleeDamageLog(caster, (*itr)->GetSpellProto()->Id, splitted, schoolMask, splitted_absorb, 0, false, 0, false);

            CleanDamage cleanDamage = CleanDamage(splitted, 0, BASE_ATTACK, MELEE_HIT_NORMAL);
            DealDamage(caster, splitted, &cleanDamage, DIRECT_DAMAGE, schoolMask, (*itr)->GetSpellProto(), false);
        }

        // We're going to call functions which can modify content of the list during iteration over it's elements
        // Let's copy the list so we can prevent iterator invalidation
        AuraEffectList vSplitDamagePctCopy(pVictim->GetAuraEffectsByType(SPELL_AURA_SPLIT_DAMAGE_PCT));
        for (AuraEffectList::iterator itr = vSplitDamagePctCopy.begin(), next; (itr != vSplitDamagePctCopy.end()) &&  (dmgInfo.GetDamage() > 0); ++itr)
        {
            // Check if aura was removed during iteration - we don't need to work on such auras
            if (!((*itr)->GetBase()->IsAppliedOnTarget(pVictim->GetGUID())))
                continue;
            // check damage school mask
            if (!((*itr)->GetMiscValue() & schoolMask))
                continue;

            // Damage can be splitted only if aura has an alive caster
            Unit * caster = (*itr)->GetCaster();
            if (!caster || (caster == pVictim) || !caster->IsInWorld() || !caster->IsAlive())
                continue;

            int32 splitDamage = CalculatePctN(dmgInfo.GetDamage(), (*itr)->GetAmount());

            // absorb must be smaller than the damage itself
            splitDamage = RoundToInterval(splitDamage, 0, int32(dmgInfo.GetDamage()));

            dmgInfo.AbsorbDamage(splitDamage);

            uint32 splitted = splitDamage;
            uint32 split_absorb = 0;
            DealDamageMods(caster, splitted, &split_absorb);

            SendSpellNonMeleeDamageLog(caster, (*itr)->GetSpellProto()->Id, splitted, schoolMask, split_absorb, 0, false, 0, false);

            CleanDamage cleanDamage = CleanDamage(splitted, 0, BASE_ATTACK, MELEE_HIT_NORMAL);
            DealDamage(caster, splitted, &cleanDamage, DIRECT_DAMAGE, schoolMask, (*itr)->GetSpellProto(), false);
        }
    }

    *resist = dmgInfo.GetResist();
    *absorb = dmgInfo.GetAbsorb();
}

void Unit::CalcHealAbsorb(Unit *pVictim, const SpellEntry *healSpell, uint32 &healAmount, uint32 &absorb)
{
    if (!healAmount)
        return;

    int32 RemainingHeal = healAmount;

    // Need remove expired auras after
    bool existExpired = false;

    // absorb without mana cost
    AuraEffectList const& vHealAbsorb = pVictim->GetAuraEffectsByType(SPELL_AURA_SCHOOL_HEAL_ABSORB);
    for (AuraEffectList::const_iterator i = vHealAbsorb.begin(); i != vHealAbsorb.end() && RemainingHeal > 0; ++i)
    {
        if (!((*i)->GetMiscValue() & healSpell->SchoolMask))
            continue;

        // Max Amount can be absorbed by this aura
        int32 currentAbsorb = (*i)->GetAmount();

        // Found empty aura (impossible but..)
        if (currentAbsorb <= 0)
        {
            existExpired = true;
            continue;
        }

        // currentAbsorb - damage can be absorbed by shield
        // If need absorb less damage
        if (RemainingHeal < currentAbsorb)
            currentAbsorb = RemainingHeal;

        RemainingHeal -= currentAbsorb;

        // Reduce shield amount
        (*i)->SetAmount((*i)->GetAmount() - currentAbsorb);
        // Need remove it later
        if ((*i)->GetAmount() <= 0)
            existExpired = true;

        if ((*i)->GetBase() && existExpired == false)
            (*i)->GetBase()->SetNeedClientUpdateForTargets();
    }

    // Remove all expired absorb auras
    if (existExpired)
    {
        for (AuraEffectList::const_iterator i = vHealAbsorb.begin(); i != vHealAbsorb.end();)
        {
            AuraEffect *auraEff = *i;
            ++i;
            if (auraEff->GetAmount() <= 0)
            {
                uint32 removedAuras = pVictim->m_removedAurasCount;
                auraEff->GetBase()->Remove(AURA_REMOVE_BY_ENEMY_SPELL);
                if (removedAuras+1 < pVictim->m_removedAurasCount)
                    i = vHealAbsorb.begin();
            }
        }
    }

    absorb = RemainingHeal > 0 ? (healAmount - RemainingHeal) : healAmount;
    healAmount = RemainingHeal;
}

void Unit::AttackerStateUpdate (Unit *pVictim, WeaponAttackType attType, bool extra)
{
    if (HasUnitState(UNIT_STATE_CANNOT_AUTOATTACK) || HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED))
        return;

    if (!pVictim->IsAlive())
        return;

    if ((attType == BASE_ATTACK || attType == OFF_ATTACK) && !this->IsWithinLOSInMap(pVictim))
        return;

    CombatStart(pVictim);
    RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_MELEE_ATTACK);

    if (attType != BASE_ATTACK && attType != OFF_ATTACK)
        return;                                             // ignore ranged case

    // melee attack spell casted at main hand attack only - no normal melee dmg dealt
    if (attType == BASE_ATTACK && m_currentSpells[CURRENT_MELEE_SPELL])
        m_currentSpells[CURRENT_MELEE_SPELL]->cast();
    else
    {
        // attack can be redirected to another target
        pVictim = SelectMagnetTarget(pVictim);

        CalcDamageInfo damageInfo;
        CalculateMeleeDamage(pVictim, 0, &damageInfo, attType);
        // Send log damage message to client
        DealDamageMods(pVictim, damageInfo.damage, &damageInfo.absorb);
        SendAttackStateUpdate(&damageInfo);

        ProcDamageAndSpell(damageInfo.target, damageInfo.procAttacker, damageInfo.procVictim, damageInfo.procEx, damageInfo.damage, damageInfo.attackType);
        DealMeleeDamage(&damageInfo,true);

        if (GetTypeId() == TYPEID_PLAYER)
        {
            if (damageInfo.target)
                ToPlayer()->SetLastDirectAttackTarget(damageInfo.target);

            sLog->outStaticDebug("AttackerStateUpdate: (Player) %u attacked %u (TypeId: %u) for %u dmg, absorbed %u, blocked %u, resisted %u.",
                GetGUIDLow(), pVictim->GetGUIDLow(), pVictim->GetTypeId(), damageInfo.damage, damageInfo.absorb, damageInfo.blocked_amount, damageInfo.resist);
        }
        else
            sLog->outStaticDebug("AttackerStateUpdate: (NPC)    %u attacked %u (TypeId: %u) for %u dmg, absorbed %u, blocked %u, resisted %u.",
                GetGUIDLow(), pVictim->GetGUIDLow(), pVictim->GetTypeId(), damageInfo.damage, damageInfo.absorb, damageInfo.blocked_amount, damageInfo.resist);
    }

    if(!extra && m_extraAttacks)
    {
        while(m_extraAttacks)
        {
            AttackerStateUpdate(pVictim, BASE_ATTACK, true);
            if(m_extraAttacks > 0)
                --m_extraAttacks;
        }
    }
}

MeleeHitOutcome Unit::RollMeleeOutcomeAgainst(const Unit *pVictim, WeaponAttackType attType) const
{
    // This is only wrapper

    // Miss chance based on melee
    //float miss_chance = MeleeMissChanceCalc(pVictim, attType);
    float miss_chance = MeleeSpellMissChance(pVictim, attType, 0);

    // Critical hit chance
    float crit_chance = GetUnitCriticalChance(attType, pVictim);

    // stunned target cannot dodge and this is check in GetUnitDodgeChance() (returned 0 in this case)
    float dodge_chance = pVictim->GetUnitDodgeChance();
    float block_chance = pVictim->GetUnitBlockChance();
    float parry_chance = pVictim->GetUnitParryChance();

    // Useful if want to specify crit & miss chances for melee, else it could be removed
    sLog->outStaticDebug("MELEE OUTCOME: miss %f crit %f dodge %f parry %f block %f", miss_chance,crit_chance,dodge_chance,parry_chance,block_chance);

    return RollMeleeOutcomeAgainst(pVictim, attType, int32(crit_chance*100), int32(miss_chance*100), int32(dodge_chance*100),int32(parry_chance*100),int32(block_chance*100));
}

MeleeHitOutcome Unit::RollMeleeOutcomeAgainst (const Unit *pVictim, WeaponAttackType attType, int32 crit_chance, int32 miss_chance, int32 dodge_chance, int32 parry_chance, int32 block_chance) const
{
    if (pVictim->GetTypeId() == TYPEID_UNIT && pVictim->ToCreature()->IsInEvadeMode())
        return MELEE_HIT_EVADE;

    int32    sum = 0, tmp = 0;
    int32    roll = urand (0, 10000);

    int32    levelDiff = pVictim->getLevel() - getLevel();

    sLog->outStaticDebug ("RollMeleeOutcomeAgainst: level difference %d for attacker", -levelDiff);
    sLog->outStaticDebug ("RollMeleeOutcomeAgainst: rolled %d, miss %d, dodge %d, parry %d, block %d, crit %d",
        roll, miss_chance, dodge_chance, parry_chance, block_chance, crit_chance);

    tmp = miss_chance;

    if (tmp > 0 && roll < (sum += tmp))
    {
        sLog->outStaticDebug ("RollMeleeOutcomeAgainst: MISS");
        return MELEE_HIT_MISS;
    }

    // always crit against a sitting target (except 0 crit chance)
    if (pVictim->GetTypeId() == TYPEID_PLAYER && crit_chance > 0 && !pVictim->IsStandState())
    {
        sLog->outStaticDebug ("RollMeleeOutcomeAgainst: CRIT (sitting victim)");
        return MELEE_HIT_CRIT;
    }

    // Dodge chance

    // only players can't dodge if attacker is behind
    if (pVictim->GetTypeId() == TYPEID_PLAYER && !pVictim->HasInArc(M_PI,this) && !pVictim->HasAuraType(SPELL_AURA_IGNORE_HIT_DIRECTION))
    {
        sLog->outStaticDebug ("RollMeleeOutcomeAgainst: attack came from behind and victim was a player.");
    }
    else
    {
        // Modify dodge chance by level difference
        if (dodge_chance > 0)
            dodge_chance += 50 * levelDiff;

        // Reduce dodge chance by attacker expertise rating
        if (GetTypeId() == TYPEID_PLAYER)
            dodge_chance -= int32(this->ToPlayer()->GetExpertiseDodgeOrParryReduction(attType)*100);
        else
            dodge_chance -= GetTotalAuraModifier(SPELL_AURA_MOD_EXPERTISE)*25;

        // Modify dodge chance by attacker SPELL_AURA_MOD_COMBAT_RESULT_CHANCE
        dodge_chance+= GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_COMBAT_RESULT_CHANCE, VICTIMSTATE_DODGE)*100;
        dodge_chance = int32 (float (dodge_chance) * GetTotalAuraMultiplier(SPELL_AURA_MOD_ENEMY_DODGE));

        tmp = dodge_chance;
        if ((tmp > 0)                                        // check if unit _can_ dodge
            && roll < (sum += tmp))
        {
            sLog->outStaticDebug ("RollMeleeOutcomeAgainst: DODGE <%d, %d)", sum-tmp, sum);
            return MELEE_HIT_DODGE;
        }
    }

    // parry & block chances

    // check if attack comes from behind, nobody can parry or block if attacker is behind
    if (!pVictim->HasInArc(M_PI, this) && !pVictim->HasAuraType(SPELL_AURA_IGNORE_HIT_DIRECTION))
        sLog->outStaticDebug ("RollMeleeOutcomeAgainst: attack came from behind.");
    else
    {
        // Modify parry chance by level difference
        if (parry_chance > 0)
        {
            if (levelDiff > 2)
                parry_chance += 100 + 800 * (levelDiff - 2);
            else
                parry_chance += 50 * levelDiff;
        }

        // Reduce parry chance by attacker expertise rating
        if (GetTypeId() == TYPEID_PLAYER)
            parry_chance -= int32(this->ToPlayer()->GetExpertiseDodgeOrParryReduction(attType)*100);
        else
            parry_chance -= GetTotalAuraModifier(SPELL_AURA_MOD_EXPERTISE)*25;

        if (pVictim->GetTypeId() == TYPEID_PLAYER || !(pVictim->ToCreature()->GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_NO_PARRY))
        {
            int32 tmp2 = int32(parry_chance);
            if (tmp2 > 0                                         // check if unit _can_ parry
                && roll < (sum += tmp2))
            {
                sLog->outStaticDebug ("RollMeleeOutcomeAgainst: PARRY <%d, %d)", sum-tmp2, sum);
                return MELEE_HIT_PARRY;
            }
        }

        if (pVictim->GetTypeId() == TYPEID_PLAYER || !(pVictim->ToCreature()->GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_NO_BLOCK))
        {
            tmp = block_chance;
            if (tmp > 0                                          // check if unit _can_ block
                && roll < (sum += tmp))
            {
                sLog->outStaticDebug ("RollMeleeOutcomeAgainst: BLOCK <%d, %d)", sum-tmp, sum);
                return MELEE_HIT_BLOCK;
            }
        }
    }

    // Critical chance
    tmp = crit_chance;

    if (tmp > 0 && roll < (sum += tmp))
    {
        sLog->outStaticDebug ("RollMeleeOutcomeAgainst: CRIT <%d, %d)", sum-tmp, sum);
        if (GetTypeId() == TYPEID_UNIT && (this->ToCreature()->GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_NO_CRIT))
            sLog->outStaticDebug ("RollMeleeOutcomeAgainst: CRIT DISABLED)");
        else
            return MELEE_HIT_CRIT;
    }

    // Max 40% chance to score a glancing blow against mobs that are higher level (can do only players and pets and not with ranged weapon)
    if (attType != RANGED_ATTACK &&
        (GetTypeId() == TYPEID_PLAYER || this->ToCreature()->IsPet()) &&
        pVictim->GetTypeId() != TYPEID_PLAYER && !pVictim->ToCreature()->IsPet() &&
        getLevel() < pVictim->getLevelForTarget(this))
    {
        // cap possible value (with bonuses > max skill)
        tmp = (2 + levelDiff) * 500;
        tmp = tmp > 4000 ? 4000 : tmp;
        if (tmp > 0 && roll < (sum += tmp))
        {
            sLog->outStaticDebug ("RollMeleeOutcomeAgainst: GLANCING <%d, %d)", sum-4000, sum);
            return MELEE_HIT_GLANCING;
        }
    }

    // mobs can score crushing blows if they're 4 or more levels above victim
    if (getLevelForTarget(pVictim) >= pVictim->getLevelForTarget(this) + 4 &&
        // can be from by creature (if can) or from controlled player that considered as creature
        !IsControlledByPlayer() &&
        !(GetTypeId() == TYPEID_UNIT && this->ToCreature()->GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_NO_CRUSH))
    {
        tmp = levelDiff * 1000 - 1500;
        if (tmp > 0 && roll < (sum += tmp))
        {
            sLog->outStaticDebug ("RollMeleeOutcomeAgainst: CRUSHING <%d, %d)", sum-tmp, sum);
            return MELEE_HIT_CRUSHING;
        }
    }

    sLog->outStaticDebug ("RollMeleeOutcomeAgainst: NORMAL");
    return MELEE_HIT_NORMAL;
}

uint32 Unit::CalculateDamage(WeaponAttackType attType, bool normalized, bool addTotalPct)
{
    float min_damage, max_damage;

    if (GetTypeId() == TYPEID_PLAYER && (normalized || !addTotalPct))
        this->ToPlayer()->CalculateMinMaxDamage(attType,normalized,addTotalPct,min_damage, max_damage);
    else
    {
        switch (attType)
        {
            case RANGED_ATTACK:
                min_damage = GetFloatValue(UNIT_FIELD_MINRANGEDDAMAGE);
                max_damage = GetFloatValue(UNIT_FIELD_MAXRANGEDDAMAGE);
                break;
            case BASE_ATTACK:
                min_damage = GetFloatValue(UNIT_FIELD_MINDAMAGE);
                max_damage = GetFloatValue(UNIT_FIELD_MAXDAMAGE);
                break;
            case OFF_ATTACK:
                min_damage = GetFloatValue(UNIT_FIELD_MINOFFHANDDAMAGE);
                max_damage = GetFloatValue(UNIT_FIELD_MAXOFFHANDDAMAGE);
                break;
                // Just for good manner
            default:
                min_damage = 0.0f;
                max_damage = 0.0f;
                break;
        }
    }

    if (min_damage > max_damage)
        std::swap(min_damage,max_damage);

    if (max_damage == 0.0f)
        max_damage = 5.0f;

    return urand((uint32)min_damage, (uint32)max_damage);
}

float Unit::CalculateLevelPenalty(SpellEntry const* spellProto) const
{
    if (spellProto->spellLevel <= 0 || spellProto->spellLevel >= spellProto->maxLevel)
        return 1.0f;

    float LvlPenalty = 0.0f;

    if (spellProto->spellLevel < 20)
        LvlPenalty = 20.0f - spellProto->spellLevel * 3.75f;
    float LvlFactor = (float(spellProto->spellLevel) + 6.0f) / float(getLevel());
    if (LvlFactor > 1.0f)
        LvlFactor = 1.0f;

    return (100.0f - LvlPenalty) * LvlFactor / 100.0f;
}

void Unit::SendMeleeAttackStart(Unit* pVictim)
{
    WorldPacket data(SMSG_ATTACKSTART, 8 + 8);
    data << uint64(GetGUID());
    data << uint64(pVictim->GetGUID());

    SendMessageToSet(&data, true);
    sLog->outStaticDebug("WORLD: Sent SMSG_ATTACKSTART");
}

void Unit::SendMeleeAttackStop(Unit* victim)
{
    if (!victim)
        return;

    WorldPacket data(SMSG_ATTACKSTOP, (8+8+4));            // we guess size
    data.append(GetPackGUID());
    data.append(victim->GetPackGUID());                     // can be 0x00...
    data << uint32(0);                                      // can be 0x1
    SendMessageToSet(&data, true);
    sLog->outDetail("%s %u stopped attacking %s %u", (GetTypeId() == TYPEID_PLAYER ? "player" : "creature"), GetGUIDLow(), (victim->GetTypeId() == TYPEID_PLAYER ? "player" : "creature"),victim->GetGUIDLow());
}

bool Unit::isSpellBlocked(Unit *pVictim, SpellEntry const * /*spellProto*/, WeaponAttackType attackType)
{
    if (pVictim->HasInArc(M_PI,this) || pVictim->HasAuraType(SPELL_AURA_IGNORE_HIT_DIRECTION))
    {
        // Check creatures flags_extra for disable block
        if (pVictim->GetTypeId() == TYPEID_UNIT &&
           pVictim->ToCreature()->GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_NO_BLOCK)
                return false;

        float blockChance = pVictim->GetUnitBlockChance();
        blockChance += (pVictim->getLevel() - getLevel()) * 0.2f;
        if (roll_chance_f(blockChance))
            return true;
    }
    return false;
}

bool Unit::isBlockCritical()
{
    if (roll_chance_i(GetTotalAuraModifier(SPELL_AURA_MOD_BLOCK_CRIT_CHANCE)))
        return true;
    return false;
}

int32 Unit::GetMechanicResistChance(const SpellEntry *spell)
{
    if (!spell)
        return 0;
    int32 resist_mech = 0;
    for (uint8 eff = 0; eff < MAX_SPELL_EFFECTS; ++eff)
    {
        if (spell->Effect[eff] == 0)
           break;
        int32 effect_mech = GetEffectMechanic(spell, eff);
        if (effect_mech)
        {
            int32 temp = GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_MECHANIC_RESISTANCE, effect_mech);
            if (resist_mech < temp)
                resist_mech = temp;
        }
    }
    return resist_mech;
}

// Melee based spells hit result calculations
SpellMissInfo Unit::MeleeSpellHitResult(Unit *pVictim, SpellEntry const *spell)
{
    // Spells with SPELL_ATTR3_IGNORE_HIT_RESULT will additionally fully ignore
    // resist and deflect chances
    if (spell->AttributesEx3 & SPELL_ATTR3_IGNORE_HIT_RESULT)
        return SPELL_MISS_NONE;

    // All non-damaging interrupts off the global cooldown will now always hit the target.
    if ((spell->HasSpellEffect(SPELL_EFFECT_INTERRUPT_CAST) || (spell->AttributesEx7 & SPELL_ATTR7_INTERRUPT_ONLY_NONPLAYER)) && !spell->HasSpellEffect(SPELL_EFFECT_SCHOOL_DAMAGE))
        return SPELL_MISS_NONE;

    bool cannotMiss = false;

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; i++)
        if (spell->Effect[i] == SPELL_EFFECT_ATTACK_ME)
            cannotMiss = true;

    WeaponAttackType attType = BASE_ATTACK;

    // Check damage class instead of attack type to correctly handle judgements
    // - they are meele, but can't be dodged/parried/deflected because of ranged dmg class
    if (spell->DmgClass == SPELL_DAMAGE_CLASS_RANGED)
        attType = RANGED_ATTACK;

    int32 levelDiff = pVictim->getLevel() - this->getLevel();

    uint32 roll = urand (0, 10000);

    uint32 missChance = uint32(MeleeSpellMissChance(pVictim, attType, spell->Id)*100.0f);
    // Roll miss
    uint32 tmp = missChance;
    if (!cannotMiss && roll <= tmp)
        return SPELL_MISS_MISS;

    // Chance resist mechanic (select max value from every mechanic spell effect)
    int32 resist_mech = 0;
    // Get effects mechanic and chance
    for (uint8 eff = 0; eff < MAX_SPELL_EFFECTS; ++eff)
    {
        int32 effect_mech = GetEffectMechanic(spell, eff);
        if (effect_mech)
        {
            int32 temp = pVictim->GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_MECHANIC_RESISTANCE, effect_mech);
            if (resist_mech < temp*100)
                resist_mech = temp*100;
        }
    }
    // Roll chance
    tmp += resist_mech;
    if (roll < tmp)
        return SPELL_MISS_RESIST;

    bool canDodge = true;
    bool canParry = true;

    // spells with this attribute can be fully blocked (they usually have other effects than doing damage)
    bool canBlock = spell->AttributesEx3 & SPELL_ATTR3_BLOCKABLE_SPELL;

    // Same spells cannot be parry/dodge
    if (spell->Attributes & SPELL_ATTR0_IMPOSSIBLE_DODGE_PARRY_BLOCK)
        return SPELL_MISS_NONE;

    // Chance resist mechanic
    int32 resist_chance = pVictim->GetMechanicResistChance(spell)*100;
    tmp += resist_chance;
    if (roll < tmp)
        return SPELL_MISS_RESIST;

    // Ranged attacks can only miss, resist and deflect
    if (attType == RANGED_ATTACK)
    {
        // only if in front
        if (pVictim->HasInArc(M_PI,this) || pVictim->HasAuraType(SPELL_AURA_IGNORE_HIT_DIRECTION))
        {
            int32 deflect_chance = pVictim->GetTotalAuraModifier(SPELL_AURA_DEFLECT_SPELLS)*100;

            if (deflect_chance >= 10000) // If AuraEffect has 100 bp -> secure 100 % deflect chance
                return SPELL_MISS_DEFLECT;

            tmp+=deflect_chance;
            if (roll < tmp)
                return SPELL_MISS_DEFLECT;
        }
        return SPELL_MISS_NONE;
    }

    // Check for attack from behind
    if (!pVictim->HasInArc(M_PI,this) && !pVictim->HasAuraType(SPELL_AURA_IGNORE_HIT_DIRECTION))
    {
        // Can`t dodge from behind in PvP (but its possible in PvE)
        if (pVictim->GetTypeId() == TYPEID_PLAYER)
            canDodge = false;
        // Can`t parry or block
        canParry = false;
        canBlock = false;
    }
    // Check creatures flags_extra for disable parry
    if (pVictim->GetTypeId() == TYPEID_UNIT)
    {
        uint32 flagEx = pVictim->ToCreature()->GetCreatureInfo()->flags_extra;
        if (flagEx & CREATURE_FLAG_EXTRA_NO_PARRY)
            canParry = false;
        // Check creatures flags_extra for disable block
        if (flagEx & CREATURE_FLAG_EXTRA_NO_BLOCK)
            canBlock = false;
    }
    // Ignore combat result aura
    AuraEffectList const &ignore = GetAuraEffectsByType(SPELL_AURA_IGNORE_COMBAT_RESULT);
    for (AuraEffectList::const_iterator i = ignore.begin(); i != ignore.end(); ++i)
    {
        if (!(*i)->IsAffectedOnSpell(spell))
            continue;
        switch ((*i)->GetMiscValue())
        {
            case MELEE_HIT_DODGE: canDodge = false; break;
            case MELEE_HIT_BLOCK: canBlock = false; break;
            case MELEE_HIT_PARRY: canParry = false; break;
            default:
                sLog->outStaticDebug("Spell %u SPELL_AURA_IGNORE_COMBAT_RESULT have unhandled state %d", (*i)->GetId(), (*i)->GetMiscValue());
                break;
        }
    }

    if (canDodge)
    {
        // Roll dodge
        int32 dodgeChance = int32(pVictim->GetUnitDodgeChance()*100.0f);
        if (dodgeChance > 0)
            dodgeChance += levelDiff * 50;
        // Reduce enemy dodge chance by SPELL_AURA_MOD_COMBAT_RESULT_CHANCE
        dodgeChance += GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_COMBAT_RESULT_CHANCE, VICTIMSTATE_DODGE)*100;
        dodgeChance = int32(float(dodgeChance) * GetTotalAuraMultiplier(SPELL_AURA_MOD_ENEMY_DODGE));
        // Reduce dodge chance by attacker expertise rating
        if (GetTypeId() == TYPEID_PLAYER)
            dodgeChance -= int32(this->ToPlayer()->GetExpertiseDodgeOrParryReduction(attType) * 100.0f);
        else
            dodgeChance -= GetTotalAuraModifier(SPELL_AURA_MOD_EXPERTISE)*25;
        if (dodgeChance < 0)
            dodgeChance = 0;

        if (roll < (tmp += dodgeChance))
            return SPELL_MISS_DODGE;
    }

    if (canParry)
    {
        // Roll parry
        int32 parryChance = int32(pVictim->GetUnitParryChance()*100.0f);
        if (parryChance > 0)
            parryChance += levelDiff > 2 ? 100 + ( levelDiff - 2 ) * 800 : levelDiff * 50;
        // Reduce parry chance by attacker expertise rating
        if (GetTypeId() == TYPEID_PLAYER)
            parryChance -= int32(this->ToPlayer()->GetExpertiseDodgeOrParryReduction(attType) * 100.0f);
        else
            parryChance -= GetTotalAuraModifier(SPELL_AURA_MOD_EXPERTISE)*25;
        if (parryChance < 0)
            parryChance = 0;

        tmp += parryChance;
        if (roll < tmp)
            return SPELL_MISS_PARRY;
    }

    if (canBlock)
    {
        int32 blockChance = int32(pVictim->GetUnitBlockChance()*100.0f);
        if (blockChance < 0)
            blockChance = 0;
        tmp += blockChance;

        if (roll < tmp)
            return SPELL_MISS_BLOCK;
    }

    return SPELL_MISS_NONE;
}

// TODO need use unit spell resistances in calculations
SpellMissInfo Unit::MagicSpellHitResult(Unit *pVictim, SpellEntry const *spell)
{
    // Can`t miss on dead target (on skinning for example)
    if ((!pVictim->IsAlive() && pVictim->GetTypeId() != TYPEID_PLAYER) || spell->AttributesEx3 & SPELL_ATTR3_IGNORE_HIT_RESULT)
        return SPELL_MISS_NONE;

    // All non-damaging interrupts off the global cooldown will now always hit the target.
    if ((spell->HasSpellEffect(SPELL_EFFECT_INTERRUPT_CAST) || (spell->AttributesEx7 & SPELL_ATTR7_INTERRUPT_ONLY_NONPLAYER)) && !spell->HasSpellEffect(SPELL_EFFECT_SCHOOL_DAMAGE))
        return SPELL_MISS_NONE;

    SpellSchoolMask schoolMask = GetSpellSchoolMask(spell);
    // PvP - PvE spell misschances per leveldif > 2
    int32 lchance = pVictim->GetTypeId() == TYPEID_PLAYER ? 7 : 11;
    int32 leveldif = int32(pVictim->getLevelForTarget(this)) - int32(getLevelForTarget(pVictim));

    // Base hit chance from attacker and victim levels
    int32 modHitChance;
    if (leveldif < 3)
        modHitChance = 96 - leveldif;
    else
        modHitChance = 94 - (leveldif - 2) * lchance;

    // Spellmod from SPELLMOD_RESIST_MISS_CHANCE
    if (Player *modOwner = GetSpellModOwner())
        modOwner->ApplySpellMod(spell->Id, SPELLMOD_RESIST_MISS_CHANCE, modHitChance);

    // Chance hit from victim SPELL_AURA_MOD_ATTACKER_SPELL_HIT_CHANCE auras
    modHitChance += pVictim->GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_ATTACKER_SPELL_HIT_CHANCE, schoolMask);

    // Increase from attacker SPELL_AURA_MOD_INCREASES_SPELL_PCT_TO_HIT auras
    modHitChance += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_INCREASES_SPELL_PCT_TO_HIT, schoolMask);
    // Reduce spell hit chance for Area of effect spells from victim SPELL_AURA_MOD_AOE_AVOIDANCE aura
    if (IsAreaOfEffectSpell(spell))
        modHitChance -= pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_AOE_AVOIDANCE);

    int32 HitChance = modHitChance * 100;
    // Increase hit chance from attacker SPELL_AURA_MOD_SPELL_HIT_CHANCE and attacker ratings
    HitChance += int32(m_modSpellHitChance * 100.0f);

    // Decrease hit chance from victim rating bonus
    if (pVictim->GetTypeId() == TYPEID_PLAYER)
        HitChance -= int32(pVictim->ToPlayer()->GetRatingBonusValue(CR_HIT_TAKEN_SPELL) * 100.0f);

    if (HitChance < 100)
        HitChance = 100;
    else if (HitChance > 10000)
        HitChance = 10000;

    int32 tmp = 10000 - HitChance;

    int32 rand = irand(0, 10000);

    bool cannotMiss = false;

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; i++)
        if (spell->Effect[i] == SPELL_EFFECT_ATTACK_ME)
            cannotMiss = true;

    if (!cannotMiss && rand < tmp)
        return SPELL_MISS_MISS;

    // Spells with SPELL_ATTR3_IGNORE_HIT_RESULT will additionally fully ignore
    // resist and deflect chances
    if (spell->AttributesEx3 & SPELL_ATTR3_IGNORE_HIT_RESULT)
        return SPELL_MISS_NONE;

    // Chance resist mechanic (select max value from every mechanic spell effect)
    int32 resist_chance = pVictim->GetMechanicResistChance(spell) * 100;
    tmp += resist_chance;

    // Chance resist debuff
    if (!IsPositiveSpell(spell->Id))
    {
        bool bNegativeAura = false;
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (spell->EffectApplyAuraName[i] != 0)
            {
                bNegativeAura = true;
                break;
            }
        }

        if (bNegativeAura)
        {
            tmp += pVictim->GetMaxPositiveAuraModifierByMiscValue(SPELL_AURA_MOD_DEBUFF_RESISTANCE, int32(spell->Dispel)) * 100;
            tmp += pVictim->GetMaxNegativeAuraModifierByMiscValue(SPELL_AURA_MOD_DEBUFF_RESISTANCE, int32(spell->Dispel)) * 100;
        }
    }

   // Roll chance
    if (rand < tmp)
        return SPELL_MISS_RESIST;

    // cast by caster in front of victim
    if (pVictim->HasInArc(M_PI, this) || pVictim->HasAuraType(SPELL_AURA_IGNORE_HIT_DIRECTION))
    {
        int32 deflect_chance = pVictim->GetTotalAuraModifier(SPELL_AURA_DEFLECT_SPELLS) * 100;
        tmp += deflect_chance;
        if (rand < tmp)
            return SPELL_MISS_DEFLECT;
    }

    return SPELL_MISS_NONE;
}

// Calculate spell hit result can be:
// Every spell can: Evade/Immune/Reflect/Sucesful hit
// For melee based spells:
//   Miss
//   Dodge
//   Parry
// For spells
//   Resist
SpellMissInfo Unit::SpellHitResult(Unit *pVictim, SpellEntry const *spell, bool CanReflect, uint32 effectMask)
{
    // Return evade for units in evade mode
    if (pVictim->GetTypeId() == TYPEID_UNIT && pVictim->ToCreature()->IsInEvadeMode() && this != pVictim)
        return SPELL_MISS_EVADE;

    // Check for immune
    if (pVictim->IsImmunedToSpell(spell, effectMask))
        return SPELL_MISS_IMMUNE;

    // All positive spells can`t miss
    // TODO: client not show miss log for this spells - so need find info for this in dbc and use it!
    if (IsPositiveSpell(spell->Id)
        &&(!IsHostileTo(pVictim)))  //prevent from affecting enemy by "positive" spell
        return SPELL_MISS_NONE;

    if (this == pVictim)
        return SPELL_MISS_NONE;

    // Try victim reflect spell
    if (CanReflect)
    {
        int32 reflectchance = pVictim->GetTotalAuraModifier(SPELL_AURA_REFLECT_SPELLS);
        Unit::AuraEffectList const& mReflectSpellsSchool = pVictim->GetAuraEffectsByType(SPELL_AURA_REFLECT_SPELLS_SCHOOL);
        for (Unit::AuraEffectList::const_iterator i = mReflectSpellsSchool.begin(); i != mReflectSpellsSchool.end(); ++i)
            if ((*i)->GetMiscValue() & GetSpellSchoolMask(spell))
                reflectchance += (*i)->GetAmount();
        if (reflectchance > 0 && roll_chance_i(reflectchance))
        {
            // Start triggers for remove charges if need (trigger only for victim, and mark as active spell)
            ProcDamageAndSpell(pVictim, PROC_FLAG_NONE, PROC_FLAG_TAKEN_SPELL_MAGIC_DMG_CLASS_NEG, PROC_EX_REFLECT, 1, BASE_ATTACK, spell);
            return SPELL_MISS_REFLECT;
        }
    }

    switch (spell->DmgClass)
    {
        case SPELL_DAMAGE_CLASS_RANGED:
        case SPELL_DAMAGE_CLASS_MELEE:
            return MeleeSpellHitResult(pVictim, spell);
        case SPELL_DAMAGE_CLASS_NONE:
            return SPELL_MISS_NONE;
        case SPELL_DAMAGE_CLASS_MAGIC:
            return MagicSpellHitResult(pVictim, spell);
    }
    return SPELL_MISS_NONE;
}

uint32 Unit::GetShieldBlockValue() const
{
    using namespace std;

    int32 ratio = 30 + GetTotalAuraModifier(SPELL_AURA_MOD_SHIELD_BLOCKVALUE_PCT);
    ratio = max(0, ratio);
    ratio = min(100, ratio);
    return (uint32) ratio;
}

uint32 Unit::CalculateBlockedAmount(uint32 damage)
{
    float blockRatio = GetShieldBlockValue() / 100.0f;

    //double blocked amount if block is critical
    if (isBlockCritical())
        blockRatio *= 2;

    if (blockRatio > 1)
        blockRatio = 1;

    return uint32(blockRatio * damage);
}

uint32 Unit::GetDefenseSkillValue(Unit const* target) const
{
    if (GetTypeId() == TYPEID_PLAYER)
    {
        // in PvP use full skill instead current skill value
        uint32 value = (target && target->GetTypeId() == TYPEID_PLAYER)
            ? this->ToPlayer()->GetMaxSkillValue(SKILL_DEFENSE)
            : this->ToPlayer()->GetSkillValue(SKILL_DEFENSE);
        value += uint32(this->ToPlayer()->GetRatingBonusValue(CR_DEFENSE_SKILL));
        return value;
    }
    else
        return GetUnitMeleeSkill(target);
}

float Unit::GetUnitDodgeChance() const
{
    if (IsNonMeleeSpellCasted(false) || HasUnitState(UNIT_STATE_STUNNED))
        return 0.0f;

    if (GetTypeId() == TYPEID_PLAYER)
        return GetFloatValue(PLAYER_DODGE_PERCENTAGE);
    else
    {
        if (((Creature const*)this)->IsTotem())
            return 0.0f;
        else
        {
            float dodge = 5.0f;
            dodge += GetTotalAuraModifier(SPELL_AURA_MOD_DODGE_PERCENT);
            return dodge > 0.0f ? dodge : 0.0f;
        }
    }
}

float Unit::GetUnitParryChance() const
{
    if (IsNonMeleeSpellCasted(false) || HasUnitState(UNIT_STATE_STUNNED))
        return 0.0f;

    float chance = 0.0f;

    if (GetTypeId() == TYPEID_PLAYER)
    {
        Player const* player = (Player const*)this;
        if (player->CanParry())
        {
            Item *tmpitem = player->GetWeaponForAttack(BASE_ATTACK,true);
            if (!tmpitem)
                tmpitem = player->GetWeaponForAttack(OFF_ATTACK,true);

            if (tmpitem)
                chance = GetFloatValue(PLAYER_PARRY_PERCENTAGE);
        }
    }
    else if (GetTypeId() == TYPEID_UNIT)
    {
        if (GetCreatureType() == CREATURE_TYPE_HUMANOID || (GetTypeId() == TYPEID_UNIT && ToCreature()->isWorldBoss()))
        {
            chance = 5.0f;
            chance += GetTotalAuraModifier(SPELL_AURA_MOD_PARRY_PERCENT);
        }
    }

    return chance > 0.0f ? chance : 0.0f;
}

float Unit::GetUnitBlockChance() const
{
    if (IsNonMeleeSpellCasted(false) || HasUnitState(UNIT_STATE_STUNNED))
        return 0.0f;

    if (GetTypeId() == TYPEID_PLAYER)
    {
        Player const* player = (Player const*)this;
        if (player->CanBlock())
        {
            Item *tmpitem = player->GetUseableItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
            if(tmpitem && !tmpitem->IsBroken())
                return GetFloatValue(PLAYER_BLOCK_PERCENTAGE);
        }
        // is player but has no block ability or no not broken shield equipped
        return 0.0f;
    }
    else
    {
        if (((Creature const*)this)->IsTotem())
            return 0.0f;
        else
        {
            float block = 5.0f;
            block += GetTotalAuraModifier(SPELL_AURA_MOD_BLOCK_PERCENT);
            return block > 0.0f ? block : 0.0f;
        }
    }
}

float Unit::GetUnitCriticalChance(WeaponAttackType attackType, const Unit *pVictim, SpellEntry const *spell) const
{
    float crit;

    if (GetTypeId() == TYPEID_PLAYER)
    {
        switch(attackType)
        {
            case BASE_ATTACK:
                crit = GetFloatValue(PLAYER_CRIT_PERCENTAGE);
                break;
            case OFF_ATTACK:
                crit = GetFloatValue(PLAYER_OFFHAND_CRIT_PERCENTAGE);
                break;
            case RANGED_ATTACK:
                crit = GetFloatValue(PLAYER_RANGED_CRIT_PERCENTAGE);
                break;
                // Just for good manner
            default:
                crit = 0.0f;
                break;
        }
    }
    else
    {
        crit = 5.0f;
        crit += GetTotalAuraModifier(SPELL_AURA_MOD_WEAPON_CRIT_PERCENT);
        crit += GetTotalAuraModifier(SPELL_AURA_MOD_CRIT_PCT);
    }

    // flat aura mods
    if (attackType == RANGED_ATTACK)
        crit += pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_CHANCE);
    else
        crit += pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_CHANCE);

    crit += pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_SPELL_AND_WEAPON_CRIT_CHANCE);

    if (!spell)// If not spell (pure melee / ranged attack)
    {
        if (pVictim)
        {
            // Blood Presence
            if (pVictim->HasAura(48263))
            {
                // Improved Blood Presence, both ranks
                if (pVictim->HasAura(50365))
                    crit -= 3.0f;
                else if (pVictim->HasAura(50371))
                    crit -= 6.0f;
            }
        }
    }

    if (GetTypeId() == TYPEID_PLAYER && spell)
    {
        Player* pCaster = (Player*)this;
        // Careful Aim hunter's talent
        if (pCaster->HasAura(34482) || pCaster->HasAura(34483))
        {
            if (AuraEffect * aurEff = pCaster->GetDummyAuraEffect(SPELLFAMILY_HUNTER, 2222, EFFECT_1))
            if (pVictim->GetHealthPct() > aurEff->GetAmount())
            {
                // Steady Shot, Cobra Shot, Aimed Shot and Aimed Shot (Master Marksman)
                if (spell->Id == 56641 || spell->Id == 77767 || spell->Id == 19434 || spell->Id == 82928)
                {
                    if (pCaster->HasAura(34482))
                        crit += 30.0f;
                    else
                        crit += 60.0f;
                }
            }
        }
    }

    // Hand of Gul'dan increases crit chance of warlocks minions
    if (pVictim->HasAura(86000) && GetTypeId() == TYPEID_UNIT)
    {
        Player* pOwner = GetCharmerOrOwnerPlayerOrPlayerItself();
        if (pOwner && pOwner->getClass() == CLASS_WARLOCK)
            crit += 10.0f;
    }

    // Apply crit chance from defence skill
    crit += (getLevel() - pVictim->getLevel()) * 0.2f;

    if (crit < 0.0f)
        crit = 0.0f;
    return crit;
}

void Unit::_DeleteRemovedAuras()
{
    while (!m_removedAuras.empty())
    {
        delete m_removedAuras.front();
        m_removedAuras.pop_front();
    }
}

void Unit::_UpdateSpells(uint32 time)
{
    if (m_currentSpells[CURRENT_AUTOREPEAT_SPELL])
        _UpdateAutoRepeatSpell();

    // remove finished spells from current pointers
    for (uint32 i = 0; i < CURRENT_MAX_SPELL; ++i)
    {
        if (m_currentSpells[i] && m_currentSpells[i]->getState() == SPELL_STATE_FINISHED)
        {
            m_currentSpells[i]->SetReferencedFromCurrent(false);
            m_currentSpells[i] = NULL;                      // remove pointer
        }
    }

    // m_auraUpdateIterator can be updated in indirect called code at aura remove to skip next planned to update but removed auras
    for (m_auraUpdateIterator = m_ownedAuras.begin(); m_auraUpdateIterator != m_ownedAuras.end();)
    {
        Aura * i_aura = m_auraUpdateIterator->second;
        ++m_auraUpdateIterator;                            // need shift to next for allow update if need into aura update
        i_aura->UpdateOwner(time, this);
    }

    // remove expired auras - do that after updates(used in scripts?)
    for (AuraMap::iterator i = m_ownedAuras.begin(); i != m_ownedAuras.end();)
    {
        if (i->second->IsExpired())
            RemoveOwnedAura(i, AURA_REMOVE_BY_EXPIRE);
        else
            ++i;
    }

    for (VisibleAuraMap::iterator itr = m_visibleAuras.begin(); itr != m_visibleAuras.end(); ++itr)
        if (itr->second->IsNeedClientUpdate())
            itr->second->ClientUpdate();

    _DeleteRemovedAuras();

    if (!m_gameObj.empty())
    {
        GameObjectList::iterator itr;
        for (itr = m_gameObj.begin(); itr != m_gameObj.end();)
        {
            if (!(*itr)->isSpawned())
            {
                (*itr)->SetOwnerGUID(0);
                (*itr)->SetRespawnTime(0);
                (*itr)->Delete();
                m_gameObj.erase(itr++);
            }
            else
                ++itr;
        }
    }
}

void Unit::_UpdateAutoRepeatSpell()
{
    //check "realtime" interrupts
    if ((GetTypeId() == TYPEID_PLAYER && ((Player*)this)->isMoving()) || IsNonMeleeSpellCasted(false,false,true,m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_spellInfo->Id == 75))
    {
        // cancel wand shoot (exception for auto shot - it can be casted while moving
        if (m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_spellInfo->Id != 75)
        {
            InterruptSpell(CURRENT_AUTOREPEAT_SPELL);
            m_AutoRepeatFirstCast = true;
            return;
        }
    }

    // this aura type will avoid all attacks made by player
    if (HasAuraType(SPELL_AURA_CANNOT_ATTACK))
        return;

    //apply delay (Auto Shot (spellID 75) not affected)
    if (m_AutoRepeatFirstCast && getAttackTimer(RANGED_ATTACK) < 500 && m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_spellInfo->Id != 75)
        setAttackTimer(RANGED_ATTACK,500);
    m_AutoRepeatFirstCast = false;

    //castroutine
    if (isAttackReady(RANGED_ATTACK))
    {
        // Check if able to cast
        if (m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->CheckCast(true) != SPELL_CAST_OK)
        {
            InterruptSpell(CURRENT_AUTOREPEAT_SPELL);
            return;
        }

        // we want to shoot
        Spell* spell = new Spell(this, m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_spellInfo, true);
        spell->prepare(&(m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_targets));

        // all went good, reset attack
        resetAttackTimer(RANGED_ATTACK);

        // and save last direct attack target (autorepeat routine spell is counted also as direct attack)
        if (ToPlayer() && m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_targets.getUnitTarget())
            ToPlayer()->SetLastDirectAttackTarget(m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_targets.getUnitTarget());
    }
}

void Unit::SetCurrentCastedSpell(Spell * pSpell)
{
    ASSERT(pSpell);                                         // NULL may be never passed here, use InterruptSpell or InterruptNonMeleeSpells

    CurrentSpellTypes CSpellType = pSpell->GetCurrentContainer();

    if (pSpell == m_currentSpells[CSpellType]) return;      // avoid breaking self

    // break same type spell if it is not delayed
    InterruptSpell(CSpellType,false);

    // special breakage effects:
    switch (CSpellType)
    {
        case CURRENT_GENERIC_SPELL:
        {
            // generic spells always break channeled not delayed spells
            InterruptSpell(CURRENT_CHANNELED_SPELL,false);

            // autorepeat breaking
            if (m_currentSpells[CURRENT_AUTOREPEAT_SPELL])
            {
                // break autorepeat if not Auto Shot
                if (m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_spellInfo->Id != 75)
                    InterruptSpell(CURRENT_AUTOREPEAT_SPELL);
                m_AutoRepeatFirstCast = true;
            }
            AddUnitState(UNIT_STATE_CASTING);
        } break;

        case CURRENT_CHANNELED_SPELL:
        {
            // channel spells always break generic non-delayed and any channeled spells
            InterruptSpell(CURRENT_GENERIC_SPELL,false);
            InterruptSpell(CURRENT_CHANNELED_SPELL);

            // it also does break autorepeat if not Auto Shot
            if (m_currentSpells[CURRENT_AUTOREPEAT_SPELL] &&
                m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_spellInfo->Id != 75)
                InterruptSpell(CURRENT_AUTOREPEAT_SPELL);
            AddUnitState(UNIT_STATE_CASTING);
        } break;

        case CURRENT_AUTOREPEAT_SPELL:
        {
            // only Auto Shoot does not break anything
            if (pSpell->m_spellInfo->Id != 75)
            {
                // generic autorepeats break generic non-delayed and channeled non-delayed spells
                InterruptSpell(CURRENT_GENERIC_SPELL,false);
                InterruptSpell(CURRENT_CHANNELED_SPELL,false);
            }
            // special action: set first cast flag
            m_AutoRepeatFirstCast = true;
        } break;

        default:
        {
            // other spell types don't break anything now
        } break;
    }

    // current spell (if it is still here) may be safely deleted now
    if (m_currentSpells[CSpellType])
        m_currentSpells[CSpellType]->SetReferencedFromCurrent(false);

    // set new current spell
    m_currentSpells[CSpellType] = pSpell;
    pSpell->SetReferencedFromCurrent(true);

    pSpell->m_selfContainer = &(m_currentSpells[pSpell->GetCurrentContainer()]);
}

void Unit::InterruptSpell(CurrentSpellTypes spellType, bool withDelayed, bool withInstant)
{
    ASSERT(spellType < CURRENT_MAX_SPELL);

    //sLog->outDebug("Interrupt spell for unit %u.", GetEntry());
    Spell *spell = m_currentSpells[spellType];
    if (spell
        && (withDelayed || spell->getState() != SPELL_STATE_DELAYED)
        && (withInstant || spell->GetCastTime() > 0))
    {
        // for example, do not let self-stun aura interrupt itself
        if (!spell->IsInterruptable())
            return;

        m_currentSpells[spellType] = NULL;

        // send autorepeat cancel message for autorepeat spells
        if (spellType == CURRENT_AUTOREPEAT_SPELL)
        {
            if (GetTypeId() == TYPEID_PLAYER)
                this->ToPlayer()->SendAutoRepeatCancel(this);
        }

        if (spell->getState() != SPELL_STATE_FINISHED)
            spell->cancel();
        spell->SetReferencedFromCurrent(false);
    }
}

void Unit::FinishSpell(CurrentSpellTypes spellType, bool ok /*= true*/)
{
    Spell* spell = m_currentSpells[spellType];
    if (!spell)
        return;

    if (spellType == CURRENT_CHANNELED_SPELL)
        spell->SendChannelUpdate(0);

    spell->finish(ok);
}

bool Unit::IsNonMeleeSpellCasted(bool withDelayed, bool skipChanneled, bool skipAutorepeat, bool isAutoshoot, bool skipInstant) const
{
    // We don't do loop here to explicitly show that melee spell is excluded.
    // Maybe later some special spells will be excluded too.

    // generic spells are cast when they are not finished and not delayed
    if (m_currentSpells[CURRENT_GENERIC_SPELL] &&
        (m_currentSpells[CURRENT_GENERIC_SPELL]->getState() != SPELL_STATE_FINISHED) &&
        (withDelayed || m_currentSpells[CURRENT_GENERIC_SPELL]->getState() != SPELL_STATE_DELAYED))
    {
        if (!skipInstant || m_currentSpells[CURRENT_GENERIC_SPELL]->GetCastTime())
        {
            if (!isAutoshoot || !(m_currentSpells[CURRENT_GENERIC_SPELL]->m_spellInfo->AttributesEx2 & SPELL_ATTR2_NOT_RESET_AUTO_ACTIONS))
                return true;
        }
    }
    // channeled spells may be delayed, but they are still considered cast
    if (!skipChanneled && m_currentSpells[CURRENT_CHANNELED_SPELL] &&
        (m_currentSpells[CURRENT_CHANNELED_SPELL]->getState() != SPELL_STATE_FINISHED))
    {
        if (!isAutoshoot || !(m_currentSpells[CURRENT_CHANNELED_SPELL]->m_spellInfo->AttributesEx2 & SPELL_ATTR2_NOT_RESET_AUTO_ACTIONS))
            return true;
    }
    // autorepeat spells may be finished or delayed, but they are still considered cast
    if (!skipAutorepeat && m_currentSpells[CURRENT_AUTOREPEAT_SPELL])
        return true;

    return false;
}

void Unit::InterruptNonMeleeSpells(bool withDelayed, uint32 spell_id, bool withInstant)
{
    // generic spells are interrupted if they are not finished or delayed
    if (m_currentSpells[CURRENT_GENERIC_SPELL] && (!spell_id || m_currentSpells[CURRENT_GENERIC_SPELL]->m_spellInfo->Id == spell_id))
        InterruptSpell(CURRENT_GENERIC_SPELL,withDelayed,withInstant);

    // autorepeat spells are interrupted if they are not finished or delayed
    if (m_currentSpells[CURRENT_AUTOREPEAT_SPELL] && (!spell_id || m_currentSpells[CURRENT_AUTOREPEAT_SPELL]->m_spellInfo->Id == spell_id))
        InterruptSpell(CURRENT_AUTOREPEAT_SPELL,withDelayed,withInstant);

    // channeled spells are interrupted if they are not finished, even if they are delayed
    if (m_currentSpells[CURRENT_CHANNELED_SPELL] && (!spell_id || m_currentSpells[CURRENT_CHANNELED_SPELL]->m_spellInfo->Id == spell_id))
        InterruptSpell(CURRENT_CHANNELED_SPELL,true,true);
}

bool Unit::CanCastWhileWalking(const SpellEntry * const sp)
{
    if (!sp)
        return false;

    // Auto Shot should not be interrupted nor forbidden to cast when moving
    if (sp->Id == 75)
        return true;

    AuraEffectList alist = GetAuraEffectsByType(SPELL_AURA_WALK_AND_CAST);
    for (AuraEffectList::const_iterator i = alist.begin(); i != alist.end(); ++i)
    {
        // check that spell mask matches
        if(!((*i)->GetSpellProto()->EffectSpellClassMask[(*i)->GetEffIndex()] & 
            sp->SpellFamilyFlags))
            continue;
        return true;
    }
    return false;
}

Spell* Unit::FindCurrentSpellBySpellId(uint32 spell_id) const
{
    for (uint32 i = 0; i < CURRENT_MAX_SPELL; i++)
        if (m_currentSpells[i] && m_currentSpells[i]->m_spellInfo->Id == spell_id)
            return m_currentSpells[i];
    return NULL;
}

int32 Unit::GetCurrentSpellCastTime(uint32 spell_id) const
{
    if (Spell const * spell = FindCurrentSpellBySpellId(spell_id))
        return spell->GetCastTime();
    return 0;
}

bool Unit::isInFrontInMap(Unit const* target, float distance,  float arc) const
{
    return IsWithinDistInMap(target, distance) && HasInArc(arc, target);
}

bool Unit::isInBackInMap(Unit const* target, float distance, float arc) const
{
    return IsWithinDistInMap(target, distance) && !HasInArc(2 * M_PI - arc, target);
}

bool Unit::isInAccessiblePlaceFor(Creature const* c) const
{
    if (IsInWater())
        return c->CanSwim();
    else
        return c->CanWalk() || c->CanFly();
}

bool Unit::IsInWater() const
{
    return GetBaseMap()->IsInWater(GetPositionX(),GetPositionY(), GetPositionZ());
}

bool Unit::IsUnderWater() const
{
    return GetBaseMap()->IsUnderWater(GetPositionX(),GetPositionY(),GetPositionZ());
}

void Unit::DeMorph()
{
    SetDisplayId(GetNativeDisplayId());
}

bool Unit::_OnAuraReapply(Aura* oldAura, Aura* newAura)
{
    /* Called when trying to apply aura with same ID as already existing aura
     * Return values:
     *    true = drop old aura
     *   false = drop new aura (do not apply new, apply "need client update" flag, refresh aura duration)
     */

    SpellEntry const* spellProto = newAura->GetSpellProto();
    //Unit* oldCaster = oldAura->GetCaster();
    Unit* newCaster = newAura->GetCaster();

    // Non stackable auras with this attribute should not be reapplied, just silently refreshed with update
    if ((spellProto->AttributesEx5 & SPELL_ATTR5_HIDE_DURATION)
        && spellProto->StackAmount == 0)
        return false;

    switch (spellProto->Id)
    {
        // Necrotic Strike heal absorb part
        case 73975:
        {
            uint32 oldAmount = oldAura->GetEffect(EFFECT_0)->GetAmount();

            if (oldAmount > 0) // only positive values
                oldAura->GetEffect(EFFECT_0)->SetAmount(oldAmount + newAura->GetEffect(EFFECT_0)->GetAmount());

            return false;
        }
        // Hamstring
        case 1715:
        {
            // Improved Hamstring
            if (newCaster && newCaster->GetTypeId()== TYPEID_PLAYER && !newCaster->ToPlayer()->HasSpellCooldown(23694) && (newCaster->HasAura(12289) || newCaster->HasAura(12668)))
            {
                // rooting part
                newCaster->CastSpell(this, 23694, true);

                // Add internal cooldown
                // Rank 1 - 60 seconds, Rank 2 - 30 seconds
                if (newCaster->HasAura(12289))
                    newCaster->ToPlayer()->AddSpellCooldown(23694,0,60000);
                else
                    newCaster->ToPlayer()->AddSpellCooldown(23694,0,30000);
            }
            return true;
        }
    }

    return true;
}

void Unit::_AddAura(UnitAura * aura, Unit * caster)
{
    ASSERT(!m_cleanupDone);
    m_ownedAuras.insert(AuraMap::value_type(aura->GetId(), aura));

    bool preventRemoval = false;

    // passive and Incanter's Absorption and auras with different type can stack with themselves any number of times
    if (!aura->IsPassive() && aura->GetId() != 44413)
    {
        // find current aura from spell and change it's stackamount
        if (Aura * foundAura = GetOwnedAura(aura->GetId(), aura->GetCasterGUID(), (sSpellMgr->GetSpellCustomAttr(aura->GetId()) & SPELL_ATTR0_CU_ENCHANT_PROC) ? aura->GetCastItemGUID() : 0, 0, aura))
        {
            // do not get stacks from aura which is being removed
            if (!foundAura->IsRemoved())
            {
                if (aura->GetSpellProto()->StackAmount)
                {
                    bool canAddStack = true;

                    switch (aura->GetId())
                    {
                        //FrostFire Bolt only stacks if caster has glyph
                        case 44614:
                            if (!caster->HasAura(61205))
                                canAddStack = false;
                            break;
                    }
                    if (canAddStack)
                        aura->ModStackAmount(foundAura->GetStackAmount());
                }

                // Update periodic timers from the previous aura
                // ToDo Fix me
                for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                {
                    AuraEffect *existingEff = foundAura->GetEffect(i);
                    AuraEffect *newEff = aura->GetEffect(i);
                    if (!existingEff || !newEff)
                        continue;
                    if (existingEff->IsPeriodic() && newEff->IsPeriodic())
                        newEff->SetPeriodicTimer(existingEff->GetPeriodicTimer());
                }

                if (_OnAuraReapply(foundAura, aura))
                {
                    // Use the new one to replace the old one
                    // This is the only place where AURA_REMOVE_BY_STACK should be used
                    RemoveOwnedAura(foundAura, AURA_REMOVE_BY_STACK);
                }
                else
                {
                    // Use the modified old one instead of new application
                    foundAura->RefreshDuration();
                    foundAura->SetNeedClientUpdateForTargets();
                    aura->Remove(AURA_REMOVE_BY_DEFAULT);
                    preventRemoval = true;
                }
            }
        }
    }

    if (!preventRemoval)
        _RemoveNoStackAurasDueToAura(aura);

    if (aura->IsRemoved())
        return;

    if (caster && IsInWorld())
    {
        if (caster->GetTypeId() == TYPEID_PLAYER)
        {
            Player* plcaster = caster->ToPlayer();
            if (plcaster->SaveCastedAuraApplyCondition(this, aura->GetSpellProto()))
            {
                plcaster->SaveCastedAuraApply(aura);
                plcaster->ProcessCastedAuraApplyMapChange();
            }
        }

        bool isSingleTarget = false;
        switch (aura->GetSpellProto()->Id)
        {
            case 33763:     // Lifebloom - only on 1 target if caster is not in Tree of life form
                isSingleTarget = !caster->HasAura(33891);
                break;
            default:
                isSingleTarget = IsSingleTargetSpell(aura->GetSpellProto());
                break;
        }

        aura->SetIsSingleTarget(isSingleTarget);

        if (isSingleTarget)
        {
            ASSERT((!IsDuringRemoveFromWorld()) || (aura->GetCasterGUID() == GetGUID()));
            // register single target aura
            caster->GetSingleCastAuras().push_back(aura);
            // remove other single target auras
            Unit::AuraList& scAuras = caster->GetSingleCastAuras();
            for (Unit::AuraList::iterator itr = scAuras.begin(); itr != scAuras.end();)
            {
                if ((*itr) != aura &&
                    IsSingleTargetSpells((*itr)->GetSpellProto(), aura->GetSpellProto()))
                {
                    (*itr)->Remove();
                    itr = scAuras.begin();
                }
                else
                    ++itr;
            }
        }
    }
    else
    {
        aura->SetIsSingleTarget(false);
    }
}

// creates aura application instance and registers it in lists
// aura application effects are handled separately to prevent aura list corruption
AuraApplication * Unit::_CreateAuraApplication(Aura * aura, uint8 effMask)
{
    // can't apply aura on unit which is going to be deleted - to not create a memory leak
    ASSERT(!m_cleanupDone);
    // aura musn't be removed
    ASSERT(!aura->IsRemoved());

    SpellEntry const* aurSpellInfo = aura->GetSpellProto();
    uint32 aurId = aurSpellInfo->Id;

    // ghost spell check, allow apply any auras at player loading in ghost mode (will be cleanup after load)
    if (!IsAlive() && !IsDeathPersistentSpell(aurSpellInfo) &&
        (GetTypeId() != TYPEID_PLAYER || !this->ToPlayer()->GetSession()->PlayerLoading()))
        return NULL;

    Unit * caster = aura->GetCaster();

    AuraApplication * aurApp = new AuraApplication(this, caster, aura, effMask);
    m_appliedAuras.insert(AuraApplicationMap::value_type(aurId, aurApp));

    if (aurSpellInfo->AuraInterruptFlags)
    {
        m_interruptableAuras.push_back(aurApp);
        AddInterruptMask(aurSpellInfo->AuraInterruptFlags);
    }

    if (AuraState aState = GetSpellAuraState(aura->GetSpellProto()))
        m_auraStateAuras.insert(AuraStateAurasMap::value_type(aState, aurApp));

    aura->_ApplyForTarget(this, caster, aurApp);
    return aurApp;
}

void Unit::_ApplyAuraEffect(Aura * aura, uint8 effIndex)
{
    ASSERT(aura);
    ASSERT(aura->HasEffect(effIndex));
    AuraApplication * aurApp = aura->GetApplicationOfTarget(GetGUID());
    ASSERT(aurApp);
    if (!aurApp->GetEffectMask())
        _ApplyAura(aurApp, 1<<effIndex);
    else
        aurApp->_HandleEffect(effIndex, true);
}

// handles effects of aura application
// should be done after registering aura in lists
void Unit::_ApplyAura(AuraApplication * aurApp, uint8 effMask)
{
    Aura * aura = aurApp->GetBase();

    _RemoveNoStackAurasDueToAura(aura);

    if (aurApp->GetRemoveMode())
        return;

    // Update target aura state flag
    if (AuraState aState = GetSpellAuraState(aura->GetSpellProto()))
        ModifyAuraState(aState, true);

    if (aurApp->GetRemoveMode())
        return;

    // Sitdown on apply aura req seated
    if (aura->GetSpellProto()->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED && !IsSitState())
        SetStandState(UNIT_STAND_STATE_SIT);

    Unit * caster = aura->GetCaster();

    if (aurApp->GetRemoveMode())
        return;

    aura->HandleAuraSpecificMods(aurApp, caster, true);
    aura->HandleAuraSpecificPeriodics(aurApp, caster);

    // apply effects of the aura
    for (uint8 i = 0 ; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (effMask & 1<<i && (!aurApp->GetRemoveMode()))
            aurApp->_HandleEffect(i, true);
    }
}

// removes aura application from lists and unapplies effects
void Unit::_UnapplyAura(AuraApplicationMap::iterator &i, AuraRemoveMode removeMode)
{
    AuraApplication * aurApp = i->second;
    ASSERT(aurApp);
    ASSERT(aurApp->GetTarget() == this);
    aurApp->SetRemoveMode(removeMode);
    Aura * aura = aurApp->GetBase();

    // dead loop is killing the server probably
    ASSERT(m_removedAurasCount < 0xFFFFFFFF);

    ++m_removedAurasCount;

    Unit * caster = aura->GetCaster();

    // Remove all pointers from lists here to prevent possible pointer invalidation on spellcast/auraapply/auraremove
    m_appliedAuras.erase(i);

    if (aura->GetSpellProto()->AuraInterruptFlags)
    {
        m_interruptableAuras.remove(aurApp);
        UpdateInterruptMask();
    }

    bool auraStateFound = false;
    AuraState auraState = GetSpellAuraState(aura->GetSpellProto());
    if (auraState)
    {
        bool canBreak = false;
        // Get mask of all aurastates from remaining auras
        for (AuraStateAurasMap::iterator itr = m_auraStateAuras.lower_bound(auraState); itr != m_auraStateAuras.upper_bound(auraState) && !(auraStateFound && canBreak);)
        {
            if (itr->second == aurApp)
            {
                m_auraStateAuras.erase(itr);
                itr = m_auraStateAuras.lower_bound(auraState);
                canBreak = true;
                continue;
            }
            auraStateFound = true;
            ++itr;
        }
    }

    aurApp->_Remove();
    aura->_UnapplyForTarget(this, caster, aurApp);

    // remove effects of the spell - needs to be done after removing aura from lists
    for (uint8 itr = 0 ; itr < MAX_SPELL_EFFECTS; ++itr)
    {
        if (aurApp->HasEffect(itr))
            aurApp->_HandleEffect(itr, false);
    }

    // all effect mustn't be applied
    ASSERT(!aurApp->GetEffectMask());

    // Remove totem at next update if totem looses its aura
    if (aurApp->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE && GetTypeId() == TYPEID_UNIT && this->ToCreature()->IsTotem()&& this->ToTotem()->GetSummonerGUID() == aura->GetCasterGUID())
    {
        if (this->ToTotem()->GetSpell() == aura->GetId() && this->ToTotem()->GetTotemType() == TOTEM_PASSIVE)
            this->ToTotem()->setDeathState(JUST_DIED);
    }

    // Remove aurastates only if were not found
    if (!auraStateFound)
        ModifyAuraState(auraState, false);

    aura->HandleAuraSpecificMods(aurApp, caster, false);

    // only way correctly remove all auras from list
    //if (removedAuras != m_removedAurasCount) new aura may be added
        i = m_appliedAuras.begin();
}

void Unit::_UnapplyAura(AuraApplication * aurApp, AuraRemoveMode removeMode)
{
    // aura can be removed from unit only if it's applied on it, shouldn't happen
    if (aurApp->GetBase()->GetApplicationOfTarget(GetGUID()) != aurApp)
        return;
    uint32 spellId = aurApp->GetBase()->GetId();
    for (AuraApplicationMap::iterator iter = m_appliedAuras.lower_bound(spellId); iter != m_appliedAuras.upper_bound(spellId);)
    {
        if (iter->second == aurApp)
        {
            _UnapplyAura(iter, removeMode);
            return;
        }
        else
            ++iter;
    }
    //ASSERT(false);
}

void Unit::_RemoveNoStackAuraApplicationsDueToAura(Aura * aura)
{
    // dynobj auras can stack infinite number of times
    if (aura->GetType() == DYNOBJ_AURA_TYPE)
        return;

    SpellEntry const* spellProto = aura->GetSpellProto();

    uint32 spellId = spellProto->Id;

    // passive spell special case (only non stackable with ranks)
    if (IsPassiveSpell(spellId) && IsPassiveSpellStackableWithRanks(spellProto))
        return;

    bool remove = false;
    for (AuraApplicationMap::iterator i = m_appliedAuras.begin(); i != m_appliedAuras.end(); ++i)
    {
        if (remove)
        {
            remove = false;
            i = m_appliedAuras.begin();
        }

        if (!_IsNoStackAuraDueToAura(aura, i->second->GetBase()))
            continue;

        RemoveAura(i, AURA_REMOVE_BY_DEFAULT);
        if (i == m_appliedAuras.end())
            break;
        remove = true;
    }
}

void Unit::_RemoveNoStackAurasDueToAura(Aura * aura)
{
    SpellEntry const* spellProto = aura->GetSpellProto();

    uint32 spellId = spellProto->Id;

    // passive spell special case (only non stackable with ranks)
    if (IsPassiveSpell(spellId) && IsPassiveSpellStackableWithRanks(spellProto))
        return;

    bool remove = false;
    for (AuraMap::iterator i = m_ownedAuras.begin(); i != m_ownedAuras.end(); ++i)
    {
        if (remove)
        {
            remove = false;
            i = m_ownedAuras.begin();
        }

        if (!_IsNoStackAuraDueToAura(aura, i->second))
            continue;

        RemoveOwnedAura(i, AURA_REMOVE_BY_DEFAULT);
        if (i == m_ownedAuras.end())
            break;
        remove = true;
    }
}

bool Unit::_IsNoStackAuraDueToAura(Aura * appliedAura, Aura * existingAura) const
{
    SpellEntry const* spellProto = appliedAura->GetSpellProto();
    // Do not check already applied aura
    if (existingAura == appliedAura)
        return false;

    // Do not check dynobj auras for stacking
    if (existingAura->GetType() != UNIT_AURA_TYPE)
        return false;

    SpellEntry const* i_spellProto = existingAura->GetSpellProto();
    uint32 i_spellId = i_spellProto->Id;
    bool sameCaster = appliedAura->GetCasterGUID() == existingAura->GetCasterGUID();

    if (IsPassiveSpell(i_spellId))
    {
        // passive non-stackable spells not stackable only for same caster
        if (!sameCaster)
            return false;

        // passive non-stackable spells not stackable only with another rank of same spell
        if (!sSpellMgr->IsRankSpellDueToSpell(spellProto, i_spellId))
            return false;
    }

    bool is_triggered_by_spell = false;
    // prevent triggering aura of removing aura that triggered it
    // prevent triggered aura of removing aura that triggering it (triggered effect early some aura of parent spell
    for (uint8 j = 0; j < MAX_SPELL_EFFECTS; ++j)
    {
        if (i_spellProto->EffectTriggerSpell[j] == spellProto->Id
            || spellProto->EffectTriggerSpell[j] == i_spellProto->Id) // I do not know what is this for
        {
            is_triggered_by_spell = true;
            break;
        }
    }

    if (is_triggered_by_spell)
        return false;

    if (sSpellMgr->CanAurasStack(appliedAura, existingAura, sameCaster))
        return false;
    return true;
}

void Unit::_RegisterAuraEffect(AuraEffect * aurEff, bool apply)
{
    if (apply)
        m_modAuras[aurEff->GetAuraType()].push_back(aurEff);
    else
        m_modAuras[aurEff->GetAuraType()].remove(aurEff);
}

// All aura base removes should go through this function!
void Unit::RemoveOwnedAura(AuraMap::iterator &i, AuraRemoveMode removeMode)
{
    Aura * aura = i->second;
    ASSERT(!aura->IsRemoved());

    // if unit currently update aura list then make safe update iterator shift to next
    if (m_auraUpdateIterator == i)
        ++m_auraUpdateIterator;

    m_ownedAuras.erase(i);
    m_removedAuras.push_back(aura);

    // Unregister single target aura
    if (aura->IsSingleTarget())
        aura->UnregisterSingleTarget();

    if (aura->GetCaster() && aura->GetCaster()->GetTypeId() == TYPEID_PLAYER)
    {
        Player* plcaster = aura->GetCaster()->ToPlayer();
        if (plcaster->RemoveCastedAuraApply(aura))
            plcaster->ProcessCastedAuraApplyMapChange();
    }

    aura->_Remove(removeMode);

    i = m_ownedAuras.begin();
}

void Unit::RemoveOwnedAura(uint32 spellId, uint64 caster, uint8 reqEffMask, AuraRemoveMode removeMode)
{
    for (AuraMap::iterator itr = m_ownedAuras.lower_bound(spellId); itr != m_ownedAuras.upper_bound(spellId);)
        if (((itr->second->GetEffectMask() & reqEffMask) == reqEffMask) && (!caster || itr->second->GetCasterGUID() == caster))
        {
            RemoveOwnedAura(itr, removeMode);
            itr = m_ownedAuras.lower_bound(spellId);
        }
        else
            ++itr;
}

void Unit::RemoveOwnedAura(Aura * aura, AuraRemoveMode removeMode)
{
    if (aura->IsRemoved())
        return;

    ASSERT(aura->GetOwner() == this);

    uint32 spellId = aura->GetId();
    for (AuraMap::iterator itr = m_ownedAuras.lower_bound(spellId); itr != m_ownedAuras.upper_bound(spellId); ++itr)
        if (itr->second == aura)
        {
            RemoveOwnedAura(itr, removeMode);
            return;
        }
    ASSERT(false);
}

Aura * Unit::GetOwnedAura(uint32 spellId, uint64 casterGUID, uint64 itemCasterGUID, uint8 reqEffMask, Aura* except) const
{
    for (AuraMap::const_iterator itr = m_ownedAuras.lower_bound(spellId); itr != m_ownedAuras.upper_bound(spellId); ++itr)
        if (((itr->second->GetEffectMask() & reqEffMask) == reqEffMask) && (!casterGUID || itr->second->GetCasterGUID() == casterGUID) && (!itemCasterGUID || itr->second->GetCastItemGUID() == itemCasterGUID) && (!except || except != itr->second))
            return itr->second;
    return NULL;
}

void Unit::RemoveAura(AuraApplicationMap::iterator &i, AuraRemoveMode mode)
{
    AuraApplication * aurApp = i->second;
    // Do not remove aura which is already being removed
    if (aurApp->GetRemoveMode())
        return;
    Aura * aura = aurApp->GetBase();
    _UnapplyAura(i, mode);
    // Remove aura - for Area and Target auras
    if (aura->GetOwner() == this)
        aura->Remove(mode);
}

void Unit::RemoveAura(uint32 spellId, uint64 caster, uint8 reqEffMask, AuraRemoveMode removeMode)
{
    for (AuraApplicationMap::iterator iter = m_appliedAuras.lower_bound(spellId); iter != m_appliedAuras.upper_bound(spellId);)
    {
        Aura const * aura = iter->second->GetBase();
        if (((aura->GetEffectMask() & reqEffMask) == reqEffMask)
            && (!caster || aura->GetCasterGUID() == caster))
        {
            RemoveAura(iter, removeMode);
            return;
        }
        else
            ++iter;
    }
}

void Unit::RemoveAura(AuraApplication * aurApp, AuraRemoveMode mode)
{
    ASSERT(aurApp->GetBase()->GetApplicationOfTarget(GetGUID()) == aurApp);
    // no need to remove
    if (aurApp->GetRemoveMode() || aurApp->GetBase()->IsRemoved())
        return;
    uint32 spellId = aurApp->GetBase()->GetId();
    for (AuraApplicationMap::iterator iter = m_appliedAuras.lower_bound(spellId); iter != m_appliedAuras.upper_bound(spellId);)
    {
        if (aurApp == iter->second)
        {
            RemoveAura(iter, mode);
            return;
        }
        else
            ++iter;
    }
}

void Unit::RemoveAura(Aura * aura, AuraRemoveMode mode)
{
    if (aura->IsRemoved())
        return;
    if (AuraApplication * aurApp = aura->GetApplicationOfTarget(GetGUID()))
        RemoveAura(aurApp, mode);
    else
        ASSERT(false);
}

void Unit::RemoveAurasDueToSpell(uint32 spellId, uint64 caster, uint8 reqEffMask, AuraRemoveMode removeMode)
{
    for (AuraApplicationMap::iterator iter = m_appliedAuras.lower_bound(spellId); iter != m_appliedAuras.upper_bound(spellId);)
    {
        Aura const * aura = iter->second->GetBase();
        if (((aura->GetEffectMask() & reqEffMask) == reqEffMask)
            && (!caster || aura->GetCasterGUID() == caster))
        {
            RemoveAura(iter, removeMode);
            iter = m_appliedAuras.lower_bound(spellId);
        }
        else
            ++iter;
    }
}

void Unit::RemoveAuraFromStack(uint32 spellId, uint64 caster, AuraRemoveMode removeMode)
{
    for (AuraMap::iterator iter = m_ownedAuras.lower_bound(spellId); iter != m_ownedAuras.upper_bound(spellId);)
    {
        Aura const * aura = iter->second;
        if ((aura->GetType() == UNIT_AURA_TYPE)
            && (!caster || aura->GetCasterGUID() == caster))
        {
            RemoveAuraFromStack(iter, removeMode);
            return;
        }
        else
            ++iter;
    }
}

inline void Unit::RemoveAuraFromStack(AuraMap::iterator &iter, AuraRemoveMode removeMode)
{
    if (iter->second->ModStackAmount(-1))
        RemoveOwnedAura(iter, removeMode);
}

void Unit::RemoveAurasDueToSpellByDispel(uint32 spellId, uint64 casterGUID, Unit *dispeller)
{
    for (AuraMap::iterator iter = m_ownedAuras.lower_bound(spellId); iter != m_ownedAuras.upper_bound(spellId);)
    {
        Aura * aura = iter->second;
        if (aura->GetCasterGUID() == casterGUID)
        {
            if (aura->GetSpellProto()->AttributesEx7 & SPELL_ATTR7_DISPEL_CHARGES)
                aura->DropCharge();
            else
                RemoveAuraFromStack(iter, AURA_REMOVE_BY_ENEMY_SPELL);

            // Unstable Affliction (crash if before removeaura?)
            if (aura->GetSpellProto()->SpellFamilyName == SPELLFAMILY_WARLOCK && (aura->GetSpellProto()->Id == 30108))
            {
                // backfire damage and silence
                dispeller->CastSpell(dispeller, 31117, true, NULL, NULL, aura->GetCasterGUID());
            }
            // Vampiric Touch (+ Sin and Punishment talent of original caster)
            else if (aura->GetSpellProto()->Id == 34914 && aura->GetCaster())
            {
                Player* caster = aura->GetCaster()->ToPlayer();
                if (caster)
                {
                    // Sin and Punishment
                    if ((caster->HasAura(87099) && roll_chance_i(50)) || caster->HasAura(87100))
                        dispeller->CastSpell(dispeller, 87204, true);
                }
            }
            // Lifebloom
            else if (aura->GetSpellProto()->Id == 33763 && aura->GetCaster())
            {
                if (AuraEffect* pEff = aura->GetEffect(EFFECT_1))
                {
                    int32 amount = pEff->GetAmount() / (aura->GetStackAmount() ? aura->GetStackAmount() : 1);
                    int32 stack = 1;
                    CastCustomSpell(this, 33778, &amount, &stack, NULL, true, NULL, pEff, casterGUID);
                }
            }
            // Frost Fever
            else if (aura->GetSpellProto()->Id == 55095)
            {
                if (aura->GetCaster() && aura->GetCaster()->GetTypeId() == TYPEID_PLAYER)
                {
                    Player* plcaster = aura->GetCaster()->ToPlayer();
                    // talent Resilient Infection
                    if (plcaster->HasAura(81339) || (plcaster->HasAura(81338) && roll_chance_i(50)))
                    {
                        plcaster->CastSpell(plcaster, 81168, true);  // refresh frost rune
                        plcaster->CastSpell(plcaster, 89831, true);  // and again this ugh-ugly hack (Empower Rune Weapon 
                    }
                }
            }
            // Blood Plague
            else if (aura->GetSpellProto()->Id == 55078)
            {
                if (aura->GetCaster() && aura->GetCaster()->GetTypeId() == TYPEID_PLAYER)
                {
                    Player* plcaster = aura->GetCaster()->ToPlayer();
                    // talent Resilient Infection
                    if (plcaster->HasAura(81339) || (plcaster->HasAura(81338) && roll_chance_i(50)))
                    {
                        plcaster->CastSpell(plcaster, 81169, true);  // refresh unholy rune
                        plcaster->CastSpell(plcaster, 89831, true);  // and again this ugh-ugly hack (Empower Rune Weapon 
                    }
                }
            }
            // Flame Shock
            if (aura->GetSpellProto()->SpellFamilyName == SPELLFAMILY_SHAMAN && (aura->GetSpellProto()->SpellFamilyFlags[0] & 0x10000000))
            {
                Unit * caster = aura->GetCaster();
                if (caster)
                {
                    int32 bp0 = 0;
                    // Lava Flows
                    if (AuraEffect const * aurEff = caster->GetDummyAuraEffect(SPELLFAMILY_SHAMAN, 3087, 0))
                    {
                        switch(aurEff->GetId())
                        {
                            case 51482: // Rank 3
                            case 51481: // Rank 2
                            case 51480: // Rank 1
                                bp0 = aurEff->GetAmount();
                                break;
                            default:
                                break;
                        }
                    }
                    if (bp0)
                        caster->CastCustomSpell(caster, 65264, &bp0, 0, 0, true);
                }
            }
            return;
        }
        else
            ++iter;
    }
}

void Unit::RemoveAurasDueToSpellBySteal(uint32 spellId, uint64 casterGUID, Unit *stealer)
{
    for (AuraMap::iterator iter = m_ownedAuras.lower_bound(spellId); iter != m_ownedAuras.upper_bound(spellId);)
    {
        Aura * aura = iter->second;
        if (aura->GetCasterGUID() == casterGUID)
        {
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

            bool stealCharge = aura->GetSpellProto()->AttributesEx7 & SPELL_ATTR7_DISPEL_CHARGES;

            if (stealCharge)
                aura->DropCharge();
            else
                RemoveAuraFromStack(iter, AURA_REMOVE_BY_ENEMY_SPELL);


            if (Aura * newAura = stealCharge ? stealer->GetAura(aura->GetId(), aura->GetCasterGUID()) : NULL)
            {
                uint8 newCharges = newAura->GetCharges() + 1;
                uint8 maxCharges = newAura->GetSpellProto()->procCharges;
                // We must be able to steal as much charges as original caster can have
                if (Unit * caster = newAura->GetCaster())
                    if (Player* modOwner = caster->GetSpellModOwner())
                        modOwner->ApplySpellMod(aura->GetId(), SPELLMOD_CHARGES, maxCharges);
                newAura->SetCharges(maxCharges < newCharges ? maxCharges : newCharges);
            }
            else
            {
                int32 dur = (2*MINUTE*IN_MILLISECONDS < aura->GetDuration() || aura->GetDuration() < 0) ? 2*MINUTE*IN_MILLISECONDS : aura->GetDuration();

                newAura = Aura::TryCreate(aura->GetSpellProto(), effMask, stealer, NULL, &baseDamage[0], NULL, NULL, aura->GetCasterGUID());
                if (!newAura)
                    return;
                // strange but intended behaviour: Stolen single target auras won't be treated as single targeted
                if (newAura->IsSingleTarget())
                    newAura->UnregisterSingleTarget();
                newAura->SetLoadedState(dur, dur, stealCharge ? 1 : aura->GetCharges(), aura->GetStackAmount(), recalculateMask, &damage[0]);
                newAura->ApplyForTargets();
            }
            return;
        }
        else
            ++iter;
    }
}

void Unit::RemoveAurasDueToItemSpell(Item* castItem,uint32 spellId)
{
    for (AuraApplicationMap::iterator iter = m_appliedAuras.lower_bound(spellId); iter != m_appliedAuras.upper_bound(spellId);)
    {
        if (!castItem || iter->second->GetBase()->GetCastItemGUID() == castItem->GetGUID())
        {
            RemoveAura(iter);
            iter = m_appliedAuras.upper_bound(spellId);          // overwrite by more appropriate
        }
        else
            ++iter;
    }
}

void Unit::RemoveAurasByType(AuraType auraType, uint64 casterGUID, Aura * except, bool negative, bool positive)
{
    // Hack for Running Wild spell not removing
    if (auraType == SPELL_AURA_MOUNTED && HasAura(87840))
        RemoveAurasDueToSpell(87840);

    for (AuraEffectList::iterator iter = m_modAuras[auraType].begin(); iter != m_modAuras[auraType].end();)
    {
        Aura * aura = (*iter)->GetBase();
        AuraApplication * aurApp = aura ->GetApplicationOfTarget(GetGUID());

        ++iter;
        if (aura != except && (!casterGUID || aura->GetCasterGUID() == casterGUID)
            && ((negative && !aurApp->IsPositive()) || (positive && aurApp->IsPositive())))
        {
            uint32 removedAuras = m_removedAurasCount;
            RemoveAura(aurApp);
            if (m_removedAurasCount > removedAuras + 1)
                iter = m_modAuras[auraType].begin();
        }
    }
}

void Unit::RemoveAurasWithAttribute(uint32 flags)
{
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end();)
    {
        SpellEntry const *spell = iter->second->GetBase()->GetSpellProto();
        if (spell->Attributes & flags)
            RemoveAura(iter);
        else
            ++iter;
    }
}

void Unit::RemoveNotOwnSingleTargetAuras(uint32 newPhase)
{
    // single target auras from other casters
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end();)
    {
        AuraApplication const * aurApp = iter->second;
        Aura const * aura = aurApp->GetBase();

        if (aura->GetCasterGUID() != GetGUID() && IsSingleTargetSpell(aura->GetSpellProto()))
        {
            if (!newPhase)
                RemoveAura(iter);
            else
            {
                Unit* caster = aura->GetCaster();
                if (!caster || !caster->InSamePhase(newPhase))
                    RemoveAura(iter);
                else
                    ++iter;
            }
        }
        else
            ++iter;
    }

    // single target auras at other targets
    AuraList& scAuras = GetSingleCastAuras();
    for (AuraList::iterator iter = scAuras.begin(); iter != scAuras.end();)
    {
        Aura * aura = *iter;
        if (aura->GetUnitOwner() != this && !aura->GetUnitOwner()->InSamePhase(newPhase))
        {
            aura->Remove();
            iter = scAuras.begin();
        }
        else
            ++iter;
    }
}


void Unit::RemoveAurasWithInterruptFlags(uint32 flag, uint32 except)
{
    if (!(m_interruptMask & flag))
        return;

    // interrupt auras
    for (AuraApplicationList::iterator iter = m_interruptableAuras.begin(); iter != m_interruptableAuras.end();)
    {
        Aura * aura = (*iter)->GetBase();
        ++iter;
        if ((aura->GetSpellProto()->AuraInterruptFlags & flag) && (!except || aura->GetId() != except))
        {
            uint32 removedAuras = m_removedAurasCount;
            RemoveAura(aura);
            if (m_removedAurasCount > removedAuras + 1)
                iter = m_interruptableAuras.begin();
        }
    }

    // interrupt channeled spell
    if (Spell* spell = m_currentSpells[CURRENT_CHANNELED_SPELL])
        if (spell->getState() == SPELL_STATE_CASTING
            && (spell->m_spellInfo->ChannelInterruptFlags & flag)
            && spell->m_spellInfo->Id != except)
        {
            if (!CanCastWhileWalking(spell->GetSpellInfo()))
                InterruptNonMeleeSpells(false);
        }

    UpdateInterruptMask();
}

void Unit::RemoveAurasWithFamily(SpellFamilyNames family, uint32 familyFlag1, uint32 familyFlag2, uint32 familyFlag3, uint64 casterGUID)
{
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end();)
    {
        Aura const * aura = iter->second->GetBase();
        if (!casterGUID || aura->GetCasterGUID() == casterGUID)
        {
            SpellEntry const *spell = aura->GetSpellProto();
            if (spell->SpellFamilyName == uint32(family) && spell->SpellFamilyFlags.HasFlag(familyFlag1, familyFlag2, familyFlag3))
            {
                RemoveAura(iter);
                continue;
            }
        }
        ++iter;
    }
}

void Unit::RemoveMovementImpairingAuras()
{
    RemoveAurasWithMechanic((1<<MECHANIC_SNARE)|(1<<MECHANIC_ROOT));
}

void Unit::RemoveAurasWithMechanic(uint32 mechanic_mask, AuraRemoveMode removemode, uint32 except)
{
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end();)
    {
        Aura const * aura = iter->second->GetBase();
        if (!except || aura->GetId() != except)
        {
            if (GetAllSpellMechanicMask(aura->GetSpellProto()) & mechanic_mask)
            {
                RemoveAura(iter, removemode);
                continue;
            }
        }
        ++iter;
    }
}

void Unit::RemoveAreaAurasDueToLeaveWorld()
{
    // make sure that all area auras not applied on self are removed - prevent access to deleted pointer later
    for (AuraMap::iterator iter = m_ownedAuras.begin(); iter != m_ownedAuras.end();)
    {
        Aura * aura = iter->second;
        ++iter;
        Aura::ApplicationMap const & appMap = aura->GetApplicationMap();
        for (Aura::ApplicationMap::const_iterator itr = appMap.begin(); itr!= appMap.end();)
        {
            AuraApplication * aurApp = itr->second;
            ++itr;
            Unit * target = aurApp->GetTarget();
            if (target == this)
                continue;
            target->RemoveAura(aurApp);
            // things linked on aura remove may apply new area aura - so start from the beginning
            iter = m_ownedAuras.begin();
        }
    }

    // remove area auras owned by others
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end();)
    {
        if (iter->second->GetBase()->GetOwner() != this)
        {
            RemoveAura(iter);
        }
        else
            ++iter;
    }
}

void Unit::RemovePlayerAurasWithSameAuraTypeDueToStack(AuraType aurType, AuraEffect * aurEff, uint64 casterGUID, uint32 exceptSpellId)
{
    if (aurEff == NULL)
        return;

    if (aurEff->GetAmount() == 0)
        return;

    int32 miscValue = aurEff->GetMiscValue();

     // Earth and Moon (hack this, because we dont want to erase CoE from warlock, just drop all other EaM debuffs)
    if (aurType == SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN && exceptSpellId == 60433)
    {
        for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end();)
        {
            if (iter->second->GetBase()->GetId() == 60433 && iter->second->GetBase()->GetCasterGUID() != casterGUID)
              RemoveAura(iter);
            else
                ++iter;
        }
        return;
    }

    // Make a copy so we can prevent iterator invalidation
    AuraEffectList aurasCopy(GetAuraEffectsByType(aurType));

    for (AuraEffectList::iterator iter = aurasCopy.begin(); iter != aurasCopy.end(); ++iter)
    {
        // For sure
        if ((*iter)->GetBase() == NULL)
            continue;
        if ((*iter)->GetBase()->IsPassive() || (*iter)->GetBase()->IsPermanent())
            continue;
        if ((*iter)->GetMiscValue() != miscValue)
            continue;
        if ((*iter)->GetCasterGUID() == casterGUID && (*iter)->GetId() == exceptSpellId)
            continue;
        if ((*iter)->GetSpellProto()->AttributesEx7 & SPELL_ATTR7_CONSOLIDATED_RAID_BUFF) // Frost Armor ...
            continue;

        if (aurType == SPELL_AURA_MOD_MELEE_RANGED_HASTE_2 && (*iter)->GetId() == 55095) // Dont drop Frost Fever
            continue;

        if (Unit * caster = (*iter)->GetCaster())
        if (caster->GetTypeId() == TYPEID_PLAYER || caster->IsHunterPet())
            (*iter)->GetBase()->Remove();
    }
}

void Unit::RemoveAllAuras()
{
    // this may be a dead loop if some events on aura remove will continiously apply aura on remove
    // we want to have all auras removed, so use your brain when linking events
    while (!m_appliedAuras.empty() || !m_ownedAuras.empty())
    {
        int i;

        AuraApplicationMap::iterator aurAppIter;
        for (aurAppIter = m_appliedAuras.begin(), i = 0; aurAppIter != m_appliedAuras.end() && i < 1000; i++)
            _UnapplyAura(aurAppIter, AURA_REMOVE_BY_DEFAULT);

        AuraMap::iterator aurIter;
        for (aurIter = m_ownedAuras.begin(), i = 0; aurIter != m_ownedAuras.end() && i < 1000; i++)
            RemoveOwnedAura(aurIter);
    }
}

void Unit::RemoveAllPositiveAuras()
{
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end();)
    {
        AuraApplication const* aurApp = iter->second;

        if (aurApp->IsPositive() && !aurApp->GetBase()->IsPassive())
            RemoveAura(iter);
        else
            ++iter;
    }
}

void Unit::RemoveAllNegativeAuras()
{
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end();)
    {
        AuraApplication const* aurApp = iter->second;

        bool IsPreserved = false;

        for (uint32 i = 0;i < sizeof(preserve_spell_table) / sizeof(uint32); i++)
        {
            if (aurApp->GetBase()->GetId() == preserve_spell_table[i])
            {
                IsPreserved = true;
                break;
            }
        }
        if (IsPreserved)
        {
            ++iter;
            continue;
        }

        if (aurApp->IsNegative())
            RemoveAura(iter);
        else
            ++iter;
    }
}

void Unit::RemoveArenaAuras(bool onLeave, bool isArena)
{
    // in join, remove positive buffs, on end, remove negative
    // used to remove positive visible auras in arenas
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end();)
    {
        AuraApplication const * aurApp = iter->second;
        Aura const * aura = aurApp->GetBase();
        const SpellEntry * spellProto = aura->GetSpellProto();

        // Do not removed these auras !!!
        if (spellProto->AttributesEx4 & SPELL_ATTR4_UNK21 // don't remove stances, shadowform, pally/hunter auras
            || (spellProto->AttributesEx4 & SPELL_ATTR4_USABLE_IN_ARENA)
            || aura->IsPassive()
            || (isArena == false && spellProto->SpellFamilyName == SPELLFAMILY_POTION) // remove potion auras only if entering/leaving arena
            || (aurApp->IsPositive() && onLeave)
            || spellProto->AppliesAuraType(SPELL_AURA_MOD_SHAPESHIFT) // shapeshifts are removed later, after adding to battleground (due to falling under map for unknown reason)
            || (spellProto->Attributes & SPELL_ATTR0_CANT_CANCEL)) // mostly guild tabards
            ++iter;
        else
            RemoveAura(iter);
    }
}

void Unit::RemoveAllAurasOnDeath()
{
    // used just after dieing to remove all visible auras
    // and disable the mods for the passive ones
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end();)
    {
        Aura const * aura = iter->second->GetBase();
        if (!aura->IsPassive() && !aura->IsDeathPersistent())
            _UnapplyAura(iter, AURA_REMOVE_BY_DEATH);
        else
            ++iter;
    }

    for (AuraMap::iterator iter = m_ownedAuras.begin(); iter != m_ownedAuras.end();)
    {
        Aura * aura = iter->second;
        if (!aura->IsPassive() && !aura->IsDeathPersistent())
            RemoveOwnedAura(iter, AURA_REMOVE_BY_DEATH);
        else
            ++iter;
    }
}

void Unit::RemoveAllAurasRequiringDeadTarget()
{
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end();)
    {
        Aura const * aura = iter->second->GetBase();
        if (!aura->IsPassive() && IsRequiringDeadTargetSpell(aura->GetSpellProto()))
            _UnapplyAura(iter, AURA_REMOVE_BY_DEFAULT);
        else
            ++iter;
    }

    for (AuraMap::iterator iter = m_ownedAuras.begin(); iter != m_ownedAuras.end();)
    {
        Aura * aura = iter->second;
        if (!aura->IsPassive() && IsRequiringDeadTargetSpell(aura->GetSpellProto()))
            RemoveOwnedAura(iter, AURA_REMOVE_BY_DEFAULT);
        else
            ++iter;
    }
}

void Unit::DelayOwnedAuras(uint32 spellId, uint64 caster, int32 delaytime)
{
    for (AuraMap::iterator iter = m_ownedAuras.lower_bound(spellId); iter != m_ownedAuras.upper_bound(spellId);++iter)
    {
        Aura * aura = iter->second;
        if (!caster || aura->GetCasterGUID() == caster)
        {
            if (aura->GetDuration() < delaytime)
                aura->SetDuration(0);
            else
                aura->SetDuration(aura->GetDuration() - delaytime);

            // update for out of range group members (on 1 slot use)
            aura->SetNeedClientUpdateForTargets();
            sLog->outDebug("Aura %u partially interrupted on unit %u, new duration: %u ms",aura->GetId() , GetGUIDLow(), aura->GetDuration());
        }
    }
}

void Unit::_RemoveAllAuraStatMods()
{
    for (AuraApplicationMap::iterator i = m_appliedAuras.begin(); i != m_appliedAuras.end(); ++i)
        (*i).second->GetBase()->HandleAllEffects(i->second, AURA_EFFECT_HANDLE_STAT, false);
}

void Unit::_ApplyAllAuraStatMods()
{
    for (AuraApplicationMap::iterator i = m_appliedAuras.begin(); i != m_appliedAuras.end(); ++i)
        (*i).second->GetBase()->HandleAllEffects(i->second, AURA_EFFECT_HANDLE_STAT, true);
}

AuraEffect * Unit::GetAuraEffect(uint32 spellId, uint8 effIndex, uint64 caster) const
{
    for (AuraApplicationMap::const_iterator itr = m_appliedAuras.lower_bound(spellId); itr != m_appliedAuras.upper_bound(spellId); ++itr)
        if (itr->second->HasEffect(effIndex) && (!caster || itr->second->GetBase()->GetCasterGUID() == caster))
            return itr->second->GetBase()->GetEffect(effIndex);
    return NULL;
}

AuraEffect * Unit::GetAuraEffectOfRankedSpell(uint32 spellId, uint8 effIndex, uint64 caster) const
{
    uint32 rankSpell = sSpellMgr->GetFirstSpellInChain(spellId);
    while (true)
    {
        if (AuraEffect * aurEff = GetAuraEffect(rankSpell, effIndex, caster))
            return aurEff;
        SpellChainNode const * chainNode = sSpellMgr->GetSpellChainNode(rankSpell);
        if (!chainNode)
            break;
        else
            rankSpell = chainNode->next;
    }
    return NULL;
}

AuraEffect* Unit::GetAuraEffect(AuraType type, SpellFamilyNames name, uint32 iconId, uint8 effIndex) const
{
    AuraEffectList const& auras = GetAuraEffectsByType(type);
    for (Unit::AuraEffectList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
    {
        if (effIndex != (*itr)->GetEffIndex())
            continue;
        SpellEntry const * spell = (*itr)->GetSpellProto();
        if (spell->SpellIconID == iconId && spell->SpellFamilyName == uint32(name))
            return *itr;
    }
    return NULL;
}

AuraEffect* Unit::GetAuraEffect(AuraType type, SpellFamilyNames family, uint32 familyFlag1, uint32 familyFlag2, uint32 familyFlag3, uint64 casterGUID) const
{
    AuraEffectList const& auras = GetAuraEffectsByType(type);
    for (AuraEffectList::const_iterator i = auras.begin(); i != auras.end(); ++i)
    {
        SpellEntry const *spell = (*i)->GetSpellProto();
        if (spell->SpellFamilyName == uint32(family) && spell->SpellFamilyFlags.HasFlag(familyFlag1, familyFlag2, familyFlag3))
        {
            if (casterGUID && (*i)->GetCasterGUID() != casterGUID)
                continue;
            return (*i);
        }
    }
    return NULL;
}

AuraApplication * Unit::GetAuraApplication(uint32 spellId, uint64 casterGUID, uint64 itemCasterGUID, uint8 reqEffMask, AuraApplication * except) const
{
    for (AuraApplicationMap::const_iterator itr = m_appliedAuras.lower_bound(spellId); itr != m_appliedAuras.upper_bound(spellId); ++itr)
    {
        Aura const * aura = itr->second->GetBase();
        if (((aura->GetEffectMask() & reqEffMask) == reqEffMask) && (!casterGUID || aura->GetCasterGUID() == casterGUID) && (!itemCasterGUID || aura->GetCastItemGUID() == itemCasterGUID) && (!except || except != itr->second))
            return itr->second;
    }
    return NULL;
}

Aura * Unit::GetAura(uint32 spellId, uint64 casterGUID, uint64 itemCasterGUID, uint8 reqEffMask) const
{
    AuraApplication * aurApp = GetAuraApplication(spellId, casterGUID, itemCasterGUID, reqEffMask);
    return aurApp ? aurApp->GetBase() : NULL;
}

AuraApplication * Unit::GetAuraApplicationOfRankedSpell(uint32 spellId, uint64 casterGUID, uint64 itemCasterGUID, uint8 reqEffMask, AuraApplication* except) const
{
    uint32 rankSpell = sSpellMgr->GetFirstSpellInChain(spellId);
    while (true)
    {
        if (AuraApplication * aurApp = GetAuraApplication(rankSpell, casterGUID, itemCasterGUID, reqEffMask, except))
            return aurApp;
        SpellChainNode const * chainNode = sSpellMgr->GetSpellChainNode(rankSpell);
        if (!chainNode)
            break;
        else
            rankSpell = chainNode->next;
    }
    return NULL;
}

Aura * Unit::GetAuraOfRankedSpell(uint32 spellId, uint64 casterGUID, uint64 itemCasterGUID, uint8 reqEffMask) const
{
    AuraApplication * aurApp = GetAuraApplicationOfRankedSpell(spellId, casterGUID, itemCasterGUID, reqEffMask);
    return aurApp ? aurApp->GetBase() : NULL;
}

bool Unit::HasAuraEffect(uint32 spellId, uint8 effIndex, uint64 caster) const
{
    for (AuraApplicationMap::const_iterator itr = m_appliedAuras.lower_bound(spellId); itr != m_appliedAuras.upper_bound(spellId); ++itr)
        if (itr->second->HasEffect(effIndex) && (!caster || itr->second->GetBase()->GetCasterGUID() == caster))
            return true;
    return false;
}

uint32 Unit::GetAuraCount(uint32 spellId) const
{
    uint32 count = 0;
    for (AuraApplicationMap::const_iterator itr = m_appliedAuras.lower_bound(spellId); itr != m_appliedAuras.upper_bound(spellId); ++itr)
    {
        if (!itr->second->GetBase()->GetStackAmount())
            count++;
        else
            count += (uint32)itr->second->GetBase()->GetStackAmount();
    }
    return count;
}

bool Unit::HasAura(uint32 spellId, uint64 casterGUID, uint64 itemCasterGUID, uint8 reqEffMask) const
{
    if (GetAuraApplication(spellId, casterGUID, itemCasterGUID, reqEffMask))
        return true;
    return false;
}

bool Unit::HasAuraType(AuraType auraType) const
{
    return (!m_modAuras[auraType].empty());
}

bool Unit::HasAuraTypeWithCaster(AuraType auratype, uint64 caster) const
{
    AuraEffectList const& mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
        if (caster == (*i)->GetCasterGUID())
            return true;
    return false;
}

bool Unit::HasAuraTypeWithMiscvalue(AuraType auratype, int32 miscvalue) const
{
    AuraEffectList const& mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
        if (miscvalue == (*i)->GetMiscValue())
            return true;
    return false;
}

bool Unit::HasAuraTypeWithAffectMask(AuraType auratype, SpellEntry const * affectedSpell) const
{
    AuraEffectList const& mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
        if ((*i)->IsAffectedOnSpell(affectedSpell))
            return true;
    return false;
}

bool Unit::HasAuraTypeWithValue(AuraType auratype, int32 value) const
{
    AuraEffectList const& mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
        if (value == (*i)->GetAmount())
            return true;
    return false;
}

bool Unit::HasNegativeAuraWithInterruptFlag(uint32 flag, uint64 guid)
{
    if (!(m_interruptMask & flag))
        return false;
    for (AuraApplicationList::iterator iter = m_interruptableAuras.begin(); iter != m_interruptableAuras.end(); ++iter)
    {
        if (!(*iter)->IsPositive() && (*iter)->GetBase()->GetSpellProto()->AuraInterruptFlags & flag && (!guid || (*iter)->GetBase()->GetCasterGUID() == guid))
            return true;
    }
    return false;
}

bool Unit::HasNegativeAuraWithAttribute(uint32 flag, uint64 guid)
{
    for (AuraApplicationMap::iterator iter = m_appliedAuras.begin(); iter != m_appliedAuras.end(); ++iter)
    {
        Aura const *aura = iter->second->GetBase();
        if (!iter->second->IsPositive() && aura->GetSpellProto()->Attributes & flag && (!guid || aura->GetCasterGUID() == guid))
            return true;
    }
    return false;
}

AuraEffect * Unit::IsScriptOverriden(SpellEntry const * spell, int32 script) const
{
    AuraEffectList const& auras = GetAuraEffectsByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
    for (AuraEffectList::const_iterator i = auras.begin(); i != auras.end(); ++i)
    {
        if ((*i)->GetMiscValue() == script)
            if ((*i)->IsAffectedOnSpell(spell))
                return (*i);
    }
    return NULL;
}

uint32 Unit::GetDiseasesByCaster(uint64 casterGUID, bool remove)
{
    static const AuraType diseaseAuraTypes[] =
    {
        SPELL_AURA_PERIODIC_DAMAGE, // Frost Fever and Blood Plague
        //SPELL_AURA_LINKED,          // Crypt Fever and Ebon Plague
        SPELL_AURA_MOD_DAMAGE_FROM_CASTER, // Ebon Plague
        SPELL_AURA_NONE
    };

    uint32 diseases=0;
    for (AuraType const* itr = &diseaseAuraTypes[0]; itr && itr[0] != SPELL_AURA_NONE; ++itr)
    {
        for (AuraEffectList::iterator i = m_modAuras[*itr].begin(); i != m_modAuras[*itr].end();)
        {
            // Get auras with disease dispel type by caster
            if ((*i)->GetSpellProto()->Dispel == DISPEL_DISEASE
                && (*i)->GetCasterGUID() == casterGUID)
            {
                ++diseases;

                if (remove)
                {
                    RemoveAura((*i)->GetId(), (*i)->GetCasterGUID());
                    i = m_modAuras[*itr].begin();
                    continue;
                }
            }
            ++i;
        }
    }
    return diseases;
}

uint32 Unit::GetDoTsByCaster(uint64 casterGUID) const
{
    static const AuraType diseaseAuraTypes[] =
    {
        SPELL_AURA_PERIODIC_DAMAGE,
        SPELL_AURA_PERIODIC_DAMAGE_PERCENT,
        SPELL_AURA_NONE
    };

    uint32 dots=0;
    for (AuraType const* itr = &diseaseAuraTypes[0]; itr && itr[0] != SPELL_AURA_NONE; ++itr)
    {
        Unit::AuraEffectList const& auras = GetAuraEffectsByType(*itr);
        for (AuraEffectList::const_iterator i = auras.begin(); i != auras.end(); ++i)
        {
            // Get auras by caster
            if ((*i)->GetCasterGUID() == casterGUID)
                ++dots;
        }
    }
    return dots;
}

int32 Unit::GetTotalAuraModifier(AuraType auratype) const
{
    int32 modifier = 0;

    AuraEffectList const& mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
        modifier += (*i)->GetAmount();

    return modifier;
}

float Unit::GetTotalAuraMultiplier(AuraType auratype) const
{
    float multiplier = 1.0f;

    AuraEffectList const& mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
        multiplier *= (100.0f + (*i)->GetAmount())/100.0f;

    return multiplier;
}

int32 Unit::GetMaxPositiveAuraModifier(AuraType auratype) const
{
    int32 modifier = 0;

    AuraEffectList const& mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        if ((*i)->GetAmount() > modifier)
            modifier = (*i)->GetAmount();
    }

    return modifier;
}

int32 Unit::GetMaxNegativeAuraModifier(AuraType auratype) const
{
    int32 modifier = 0;

    AuraEffectList const& mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
        if ((*i)->GetAmount() < modifier)
            modifier = (*i)->GetAmount();

    return modifier;
}

int32 Unit::GetTotalAuraModifierByMiscMask(AuraType auratype, uint32 misc_mask) const
{
    int32 modifier = 0;

    AuraEffectList const& mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        if ((*i)->GetMiscValue()& misc_mask)
            modifier += (*i)->GetAmount();
    }
    return modifier;
}

float Unit::GetTotalAuraMultiplierByMiscMask(AuraType auratype, uint32 misc_mask) const
{
    float multiplier = 1.0f;

    AuraEffectList const& mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        if ((*i)->GetMiscValue()& misc_mask)
            multiplier *= (100.0f + (*i)->GetAmount())/100.0f;
    }
    return multiplier;
}

int32 Unit::GetMaxPositiveAuraModifierByMiscMask(AuraType auratype, uint32 misc_mask, const AuraEffect* except) const
{
    int32 modifier = 0;

    AuraEffectList const& mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        if (except != (*i) && (*i)->GetMiscValue()& misc_mask && (*i)->GetAmount() > modifier)
            modifier = (*i)->GetAmount();
    }

    return modifier;
}

int32 Unit::GetMaxNegativeAuraModifierByMiscMask(AuraType auratype, uint32 misc_mask) const
{
    int32 modifier = 0;

    AuraEffectList const& mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        if ((*i)->GetMiscValue()& misc_mask && (*i)->GetAmount() < modifier)
            modifier = (*i)->GetAmount();
    }

    return modifier;
}

int32 Unit::GetTotalAuraModifierByMiscValue(AuraType auratype, int32 misc_value) const
{
    int32 modifier = 0;

    AuraEffectList const& mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        if ((*i)->GetMiscValue() == misc_value)
            modifier += (*i)->GetAmount();
    }
    return modifier;
}

float Unit::GetTotalAuraMultiplierByMiscValue(AuraType auratype, int32 misc_value) const
{
    float multiplier = 1.0f;

    AuraEffectList const& mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        if ((*i)->GetMiscValue() == misc_value)
            multiplier *= (100.0f + (*i)->GetAmount())/100.0f;
    }
    return multiplier;
}

int32 Unit::GetMaxPositiveAuraModifierByMiscValue(AuraType auratype, int32 misc_value) const
{
    int32 modifier = 0;

    AuraEffectList const& mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        if ((*i)->GetMiscValue() == misc_value && (*i)->GetAmount() > modifier)
            modifier = (*i)->GetAmount();
    }

    return modifier;
}

int32 Unit::GetMaxNegativeAuraModifierByMiscValue(AuraType auratype, int32 misc_value) const
{
    int32 modifier = 0;

    AuraEffectList const& mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        if ((*i)->GetMiscValue() == misc_value && (*i)->GetAmount() < modifier)
            modifier = (*i)->GetAmount();
    }

    return modifier;
}

int32 Unit::GetTotalAuraModifierByAffectMask(AuraType auratype, SpellEntry const * affectedSpell, SpellModOp op) const
{
    int32 modifier = 0;

    AuraEffectList const& mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        if (op == SPELLMOD_NONE)
        {
            if ((*i)->IsAffectedOnSpell(affectedSpell))
                modifier += (*i)->GetAmount();
        }
        else
        {
            if ((*i)->IsAffectedOnSpell(affectedSpell) && (*i)->GetSpellModType() == op)
                modifier += (*i)->GetAmount();
        }
    }
    return modifier;
}

float Unit::GetTotalAuraMultiplierByAffectMask(AuraType auratype, SpellEntry const * affectedSpell, SpellModOp op) const
{
    float multiplier = 1.0f;

    AuraEffectList const& mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        if (op == SPELLMOD_NONE)
        {
            if ((*i)->IsAffectedOnSpell(affectedSpell))
                multiplier *= (100.0f + (*i)->GetAmount())/100.0f;
        }
        else
        {
            if ((*i)->IsAffectedOnSpell(affectedSpell) && (*i)->GetSpellModType() == op)
                multiplier *= (100.0f + (*i)->GetAmount())/100.0f;
        }
    }
    return multiplier;
}

int32 Unit::GetMaxPositiveAuraModifierByAffectMask(AuraType auratype, SpellEntry const * affectedSpell, SpellModOp op) const
{
    int32 modifier = 0;

    AuraEffectList const& mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        if (op == SPELLMOD_NONE)
        {
            if ((*i)->IsAffectedOnSpell(affectedSpell) && (*i)->GetAmount() > modifier)
                modifier = (*i)->GetAmount();
        }
        else
        {
            if ((*i)->IsAffectedOnSpell(affectedSpell) && (*i)->GetAmount() > modifier && (*i)->GetSpellModType() == op)
                modifier = (*i)->GetAmount();
        }
    }

    return modifier;
}

int32 Unit::GetMaxNegativeAuraModifierByAffectMask(AuraType auratype, SpellEntry const * affectedSpell, SpellModOp op) const
{
    int32 modifier = 0;

    AuraEffectList const& mTotalAuraList = GetAuraEffectsByType(auratype);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
    {
        if (op == SPELLMOD_NONE)
        {
            if ((*i)->IsAffectedOnSpell(affectedSpell) && (*i)->GetAmount() < modifier)
                modifier = (*i)->GetAmount();
        }
        else
        {
            if ((*i)->IsAffectedOnSpell(affectedSpell) && (*i)->GetAmount() < modifier && (*i)->GetSpellModType() == op)
                modifier = (*i)->GetAmount();
        }
    }

    return modifier;
}

void Unit::_RegisterDynObject(DynamicObject* dynObj)
{
    m_dynObj.push_back(dynObj);
}

void Unit::_UnregisterDynObject(DynamicObject* dynObj)
{
    m_dynObj.remove(dynObj);
}

DynamicObject * Unit::GetDynObject(uint32 spellId)
{
    if (m_dynObj.empty())
        return NULL;
    for (DynObjectList::const_iterator i = m_dynObj.begin(); i != m_dynObj.end();++i)
    {
        DynamicObject* dynObj = *i;

        if (dynObj->GetSpellId() == spellId)
            return dynObj;
    }
    return NULL;
}

void Unit::RemoveDynObject(uint32 spellId)
{
    if (m_dynObj.empty())
        return;
    for (DynObjectList::iterator i = m_dynObj.begin(); i != m_dynObj.end();)
    {
        DynamicObject* dynObj = *i;
        if (dynObj->GetSpellId() == spellId)
        {
            dynObj->Remove();
            i = m_dynObj.begin();
        }
        else
            ++i;
    }
}

void Unit::RemoveAllDynObjects()
{
    while (!m_dynObj.empty())
        m_dynObj.front()->Remove();
}


GameObject* Unit::GetGameObject(uint32 spellId) const
{
    for (GameObjectList::const_iterator i = m_gameObj.begin(); i != m_gameObj.end(); ++i)
        if ((*i)->GetSpellId() == spellId)
            return *i;

    return NULL;
}

void Unit::AddGameObject(GameObject* gameObj)
{
    if (!gameObj || !gameObj->GetOwnerGUID() == 0) return;
    m_gameObj.push_back(gameObj);
    gameObj->SetOwnerGUID(GetGUID());

    if (GetTypeId() == TYPEID_PLAYER && gameObj->GetSpellId())
    {
        SpellEntry const* createBySpell = sSpellStore.LookupEntry(gameObj->GetSpellId());
        // Need disable spell use for owner
        if (createBySpell && createBySpell->Attributes & SPELL_ATTR0_DISABLED_WHILE_ACTIVE)
            // note: item based cooldowns and cooldown spell mods with charges ignored (unknown existed cases)
            this->ToPlayer()->AddSpellAndCategoryCooldowns(createBySpell,0,NULL,true);
    }
}

void Unit::RemoveGameObject(GameObject* gameObj, bool del)
{
    if (!gameObj || !gameObj->GetOwnerGUID() == GetGUID()) return;

    gameObj->SetOwnerGUID(0);

    for (uint32 i = 0; i < 4; ++i)
    {
        if (m_ObjectSlot[i] == gameObj->GetGUID())
        {
            m_ObjectSlot[i] = 0;
            break;
        }
    }

    // GO created by some spell
    if (uint32 spellid = gameObj->GetSpellId())
    {
        RemoveAurasDueToSpell(spellid);

        if (GetTypeId() == TYPEID_PLAYER)
        {
            SpellEntry const* createBySpell = sSpellStore.LookupEntry(spellid);
            // Need activate spell use for owner
            if (createBySpell && createBySpell->Attributes & SPELL_ATTR0_DISABLED_WHILE_ACTIVE)
                // note: item based cooldowns and cooldown spell mods with charges ignored (unknown existed cases)
                this->ToPlayer()->SendCooldownEvent(createBySpell);
        }
    }

    m_gameObj.remove(gameObj);

    if (del)
    {
        gameObj->SetRespawnTime(0);
        gameObj->Delete();
    }
}

void Unit::RemoveGameObject(uint32 spellid, bool del)
{
    if (m_gameObj.empty())
        return;
    GameObjectList::iterator i, next;
    for (i = m_gameObj.begin(); i != m_gameObj.end(); i = next)
    {
        next = i;
        if (spellid == 0 || (*i)->GetSpellId() == spellid)
        {
            (*i)->SetOwnerGUID(0);
            if (del)
            {
                (*i)->SetRespawnTime(0);
                (*i)->Delete();
            }

            next = m_gameObj.erase(i);
        }
        else
            ++next;
    }
}

void Unit::RemoveAllGameObjects()
{
    // remove references to unit
    for (GameObjectList::iterator i = m_gameObj.begin(); i != m_gameObj.end();)
    {
        (*i)->SetOwnerGUID(0);
        (*i)->SetRespawnTime(0);
        (*i)->Delete();
        i = m_gameObj.erase(i);
    }
}

void Unit::SendSpellNonMeleeDamageLog(SpellNonMeleeDamage *log)
{
    WorldPacket data(SMSG_SPELLNONMELEEDAMAGELOG, (16+4+4+4+1+4+4+1+1+4+4+1)); // we guess size
    data.append(log->target->GetPackGUID());
    data.append(log->attacker->GetPackGUID());
    data << uint32(log->SpellID);
    data << uint32(log->damage);                            // damage amount
    int32 overkill = log->damage - log->target->GetHealth();
    data << uint32(overkill > 0 ? overkill : -1);           // overkill (-1 if no overkill?!)
    data << uint8 (log->schoolMask);                        // damage school
    data << uint32(log->absorb);                            // AbsorbedDamage
    data << uint32(log->resist);                            // resist
    data << uint8 (log->physicalLog);                       // if 1, then client show spell name (example: %s's ranged shot hit %s for %u school or %s suffers %u school damage from %s's spell_name
    data << uint8 (log->unused);                            // unused
    data << uint32(log->blocked);                           // blocked
    data << uint32(log->HitInfo);
    data << uint8 (0);                                      // flag to use extend data
    SendMessageToSet(&data, true);
}

void Unit::SendSpellNonMeleeDamageLog(Unit *target, uint32 SpellID, uint32 Damage, SpellSchoolMask damageSchoolMask, uint32 AbsorbedDamage, uint32 Resist, bool PhysicalDamage, uint32 Blocked, bool CriticalHit)
{
    SpellNonMeleeDamage log(this, target, SpellID, damageSchoolMask);
    log.damage = Damage - AbsorbedDamage - Resist - Blocked;
    log.absorb = AbsorbedDamage;
    log.resist = Resist;
    log.physicalLog = PhysicalDamage;
    log.blocked = Blocked;
    log.HitInfo = SPELL_HIT_TYPE_UNK1 | SPELL_HIT_TYPE_UNK3 | SPELL_HIT_TYPE_UNK6;
    if (CriticalHit)
        log.HitInfo |= SPELL_HIT_TYPE_CRIT;
    SendSpellNonMeleeDamageLog(&log);
}

void Unit::ProcDamageAndSpell(Unit *pVictim, uint32 procAttacker, uint32 procVictim, uint32 procExtra, uint32 amount, WeaponAttackType attType, SpellEntry const *procSpell, SpellEntry const * procAura)
{
     // Not much to do if no flags are set.
    if (procAttacker)
        ProcDamageAndSpellFor(false, pVictim,procAttacker, procExtra,attType, procSpell, amount, procAura);
    // Now go on with a victim's events'n'auras
    // Not much to do if no flags are set or there is no victim
    if (pVictim && pVictim->IsAlive() && procVictim)
        pVictim->ProcDamageAndSpellFor(true, this, procVictim, procExtra, attType, procSpell, amount, procAura);
}

void Unit::SendPeriodicAuraLog(SpellPeriodicAuraLogInfo *pInfo)
{
    AuraEffect const * aura = pInfo->auraEff;

    WorldPacket data(SMSG_PERIODICAURALOG, 8+8+4+4+4+4*5+1);
    data.append(GetPackGUID());
    data.appendPackGUID(aura->GetCasterGUID());
    data << uint32(aura->GetId());                          // spellId
    data << uint32(1);                                      // count
    data << uint32(aura->GetAuraType());                    // auraId
    switch(aura->GetAuraType())
    {
        case SPELL_AURA_PERIODIC_DAMAGE:
        case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
            data << uint32(pInfo->damage);                  // damage
            data << uint32(pInfo->overDamage);              // overkill?
            data << uint32(GetSpellSchoolMask(aura->GetSpellProto()));
            data << uint32(pInfo->absorb);                  // absorb
            data << uint32(pInfo->resist);                  // resist
            data << uint8(pInfo->critical);                 // new 3.1.2 critical tick
            break;
        case SPELL_AURA_PERIODIC_HEAL:
        case SPELL_AURA_OBS_MOD_HEALTH:
            data << uint32(pInfo->damage);                  // damage
            data << uint32(pInfo->overDamage);              // overheal
            data << uint32(pInfo->absorb);                  // absorb
            data << uint8(pInfo->critical);                 // new 3.1.2 critical tick
            break;
        case SPELL_AURA_OBS_MOD_POWER:
        case SPELL_AURA_PERIODIC_ENERGIZE:
            data << uint32(aura->GetMiscValue());           // power type
            data << uint32(pInfo->damage);                  // damage
            break;
        case SPELL_AURA_PERIODIC_MANA_LEECH:
            data << uint32(aura->GetMiscValue());           // power type
            data << uint32(pInfo->damage);                  // amount
            data << float(pInfo->multiplier);               // gain multiplier
            break;
        default:
            sLog->outError("Unit::SendPeriodicAuraLog: unknown aura %u", uint32(aura->GetAuraType()));
            return;
    }

    SendMessageToSet(&data, true);
}

void Unit::SendSpellMiss(Unit *target, uint32 spellID, SpellMissInfo missInfo)
{
    WorldPacket data(SMSG_SPELLLOGMISS, 4+8+1+4+8+1);
    data << uint32(spellID);
    data << uint64(GetGUID());
    data << uint8(0);                                       // can be 0 or 1
    data << uint32(1);                                      // target count
    // for (i = 0; i < target count; ++i)
    data << uint64(target->GetGUID());                      // target GUID
    data << uint8(missInfo);
    // end loop
    SendMessageToSet(&data, true);
}

void Unit::SendSpellDamageImmune(Unit * target, uint32 spellId)
{
    WorldPacket data(SMSG_SPELLORDAMAGE_IMMUNE, 8+8+4+1);
    data << uint64(GetGUID());
    data << uint64(target->GetGUID());
    data << uint32(spellId);
    data << uint8(0); // bool - log format: 0-default, 1-debug
    SendMessageToSet(&data,true);
}

void Unit::SendAttackStateUpdate(CalcDamageInfo *damageInfo)
{
    sLog->outDebug("WORLD: Sending SMSG_ATTACKERSTATEUPDATE");

    uint32 count = 1;
    size_t maxsize = 4+5+5+4+4+1+4+4+4+4+4+1+4+4+4+4+4*12;
    WorldPacket data(SMSG_ATTACKERSTATEUPDATE, maxsize);    // we guess size
    data << uint32(damageInfo->HitInfo);
    data.append(damageInfo->attacker->GetPackGUID());
    data.append(damageInfo->target->GetPackGUID());
    data << uint32(damageInfo->damage);                     // Full damage
    int32 overkill = damageInfo->damage - damageInfo->target->GetHealth();
    data << uint32(overkill < 0 ? 0 : overkill);            // Overkill
    data << uint8(count);                                   // Sub damage count

    for (uint32 i = 0; i < count; ++i)
    {
        data << uint32(damageInfo->damageSchoolMask);       // School of sub damage
        data << float(damageInfo->damage);                  // sub damage
        data << uint32(damageInfo->damage);                 // Sub Damage
    }

    if (damageInfo->HitInfo & (HITINFO_ABSORB | HITINFO_ABSORB2))
    {
        for (uint32 i = 0; i < count; ++i)
            data << uint32(damageInfo->absorb);             // Absorb
    }

    if (damageInfo->HitInfo & (HITINFO_RESIST | HITINFO_RESIST2))
    {
        for (uint32 i = 0; i < count; ++i)
            data << uint32(damageInfo->resist);             // Resist
    }

    data << uint8(damageInfo->TargetState);
    data << uint32(0);
    data << uint32(0);

    if (damageInfo->HitInfo & HITINFO_BLOCK)
        data << uint32(damageInfo->blocked_amount);

    if (damageInfo->HitInfo & HITINFO_UNK3)
        data << uint32(0);

    if (damageInfo->HitInfo & HITINFO_UNK1)
    {
        data << uint32(0);
        data << float(0);
        data << float(0);
        data << float(0);
        data << float(0);
        data << float(0);
        data << float(0);
        data << float(0);
        data << float(0);
        data << float(0);       // Found in a loop with 1 iteration
        data << float(0);       // ditto ^
        data << uint32(0);
    }

    SendMessageToSet(&data, true);
}

void Unit::SendAttackStateUpdate(uint32 HitInfo, Unit *target, uint8 /*SwingType*/, SpellSchoolMask damageSchoolMask, uint32 Damage, uint32 AbsorbDamage, uint32 Resist, VictimState TargetState, uint32 BlockedAmount)
{
    CalcDamageInfo dmgInfo;
    dmgInfo.HitInfo = HitInfo;
    dmgInfo.attacker = this;
    dmgInfo.target = target;
    dmgInfo.damage = Damage - AbsorbDamage - Resist - BlockedAmount;
    dmgInfo.damageSchoolMask = damageSchoolMask;
    dmgInfo.absorb = AbsorbDamage;
    dmgInfo.resist = Resist;
    dmgInfo.TargetState = TargetState;
    dmgInfo.blocked_amount = BlockedAmount;
    SendAttackStateUpdate(&dmgInfo);
}

bool Unit::HandleModPowerRegenAuraProc(Unit *pVictim, uint32 damage, AuraEffect* triggeredByAura, SpellEntry const * /*procSpell*/, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 cooldown)
{
    SpellEntry const *hasteSpell = triggeredByAura->GetSpellProto();

    Item* castItem = triggeredByAura->GetBase()->GetCastItemGUID() && GetTypeId() == TYPEID_PLAYER
        ? this->ToPlayer()->GetItemByGuid(triggeredByAura->GetBase()->GetCastItemGUID()) : NULL;

    uint32 triggered_spell_id = 0;
    Unit* target = pVictim;
    int32 basepoints0 = 0;

    switch(hasteSpell->SpellFamilyName)
    {
        case SPELLFAMILY_ROGUE:
        {
            switch(hasteSpell->Id)
            {
                // Blade Flurry
                case 13877:
                //case 33735:
                {
                    target = SelectNearbyTarget();
                    if (!target || target == pVictim)
                        return false;
                    basepoints0 = damage;
                    triggered_spell_id = 22482;
                    break;
                }
            }
            break;
        }
    }

    // processed charge only counting case
    if (!triggered_spell_id)
        return true;

    SpellEntry const* triggerEntry = sSpellStore.LookupEntry(triggered_spell_id);

    if (!triggerEntry)
    {
        sLog->outError("Unit::HandleHasteAuraProc: Spell %u have not existed triggered spell %u",hasteSpell->Id,triggered_spell_id);
        return false;
    }

    // default case
    if ((!target && !sSpellMgr->IsSrcTargetSpell(triggerEntry)) || (target && target != this && !target->IsAlive()))
        return false;

    if (cooldown && GetTypeId() == TYPEID_PLAYER && this->ToPlayer()->HasSpellCooldown(triggered_spell_id))
        return false;

    if (basepoints0)
        CastCustomSpell(target,triggered_spell_id,&basepoints0,NULL,NULL,true,castItem,triggeredByAura);
    else
        CastSpell(target,triggered_spell_id,true,castItem,triggeredByAura);

    if (cooldown && GetTypeId() == TYPEID_PLAYER)
        this->ToPlayer()->AddSpellCooldown(triggered_spell_id,0,cooldown*1000);

    return true;
}

bool Unit::HandleSpellCritChanceAuraProc(Unit *pVictim, uint32 /*damage*/, AuraEffect* triggeredByAura, SpellEntry const * /*procSpell*/, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 cooldown)
{
    SpellEntry const *triggeredByAuraSpell = triggeredByAura->GetSpellProto();

    Item* castItem = triggeredByAura->GetBase()->GetCastItemGUID() && GetTypeId() == TYPEID_PLAYER
        ? this->ToPlayer()->GetItemByGuid(triggeredByAura->GetBase()->GetCastItemGUID()) : NULL;

    uint32 triggered_spell_id = 0;
    Unit* target = pVictim;
    int32 basepoints0 = 0;

    /*switch(triggeredByAuraSpell->SpellFamilyName)
    {
        case SPELLFAMILY_MAGE:
        {
            switch(triggeredByAuraSpell->Id)
            {
                // Focus Magic
                case 54646:
                {
                    Unit* caster = triggeredByAura->GetCaster();
                    if (!caster)
                        return false;

                    triggered_spell_id = 54648;
                    target = caster;
                    break;
                }
            }
        }
    }*/

    // processed charge only counting case
    if (!triggered_spell_id)
        return true;

    SpellEntry const* triggerEntry = sSpellStore.LookupEntry(triggered_spell_id);

    if (!triggerEntry)
    {
        sLog->outError("Unit::HandleHasteAuraProc: Spell %u have not existed triggered spell %u",triggeredByAuraSpell->Id,triggered_spell_id);
        return false;
    }

    // default case
    if (!target || (target != this && !target->IsAlive()))
        return false;

    if (cooldown && GetTypeId() == TYPEID_PLAYER && this->ToPlayer()->HasSpellCooldown(triggered_spell_id))
        return false;

    if (basepoints0)
        CastCustomSpell(target,triggered_spell_id,&basepoints0,NULL,NULL,true,castItem,triggeredByAura);
    else
        CastSpell(target,triggered_spell_id,true,castItem,triggeredByAura);

    if (cooldown && GetTypeId() == TYPEID_PLAYER)
        this->ToPlayer()->AddSpellCooldown(triggered_spell_id,0,cooldown*1000);

    return true;
}

bool Unit::HandleDummyAuraProc(Unit *pVictim, uint32 damage, AuraEffect* triggeredByAura, SpellEntry const * procSpell, uint32 procFlag, uint32 procEx, uint32 cooldown)
{
    SpellEntry const *dummySpell = triggeredByAura->GetSpellProto();
    uint32 effIndex = triggeredByAura->GetEffIndex();
    int32  triggerAmount = triggeredByAura->GetAmount();

    Item* castItem = triggeredByAura->GetBase()->GetCastItemGUID() && GetTypeId() == TYPEID_PLAYER
        ? this->ToPlayer()->GetItemByGuid(triggeredByAura->GetBase()->GetCastItemGUID()) : NULL;

    uint32 triggered_spell_id = 0;
    uint32 cooldown_spell_id = 0; // for random trigger, will be one of the triggered spell to avoid repeatable triggers
                                  // otherwise, it's the triggered_spell_id by default
    Unit* target = pVictim;
    int32 basepoints0 = 0;
    uint64 originalCaster = 0;

    switch(dummySpell->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch (dummySpell->Id)
            {
                // Bloodworms Health Leech
                case 50453:
                {
                    if (Unit *owner = this->GetOwner())
                    {
                        basepoints0 = int32(damage*1.50);
                        target = owner;
                        triggered_spell_id = 50454;
                        break;
                    }
                    return false;
                }
                // Scales of Life
                case 96879: // normal
                case 97117: // heroic
                {
                    uint32 heal = 0;

                    if (IsFullHealth()) // Pure overheal
                        heal = damage;
                    else
                    {
                        uint32 missing_hp = GetMaxHealth() - GetHealth();
                        if (missing_hp >= damage) // No overheal no deal
                            heal = 0;
                        else
                            heal = damage - missing_hp; // Add only real amount of overheal
                    }

                    if (heal == 0)
                        break;

                    int32 bp0 = 0;
                    int32 max_heal = (dummySpell->Id == 96879) ? 17096 : 19284; // Stored also in auraEffect value of dummySpell -> nah :P

                    if (AuraEffect* pEff = GetAuraEffect(96881,EFFECT_0)) // Weight of a Feather
                    {
                        bp0 = ((pEff->GetAmount() + (int32)heal) > max_heal) ? (int32)max_heal : (pEff->GetAmount() + (int32)heal);
                        pEff->SetAmount(bp0);
                        pEff->GetBase()->RefreshDuration();
                    }
                    else
                    {
                        bp0 = int32(heal);
                        CastCustomSpell(this, 96881, &bp0, 0, 0, true);
                    }
                    break;
                }
                // Sweeping Strikes
                case 18765:
                case 35429:
                {
                    target = SelectNearbyTarget();
                    if (!target)
                        return false;

                    triggered_spell_id = 26654;
                    break;
                }
                case 108124 : // Fel Decay (Perotharn encounter)
                {
                    // Proc only from heals
                    if((procFlag & PROC_FLAG_TAKEN_SPELL_NONE_DMG_CLASS_POS) || (procFlag & PROC_FLAG_TAKEN_SPELL_MAGIC_DMG_CLASS_POS))
                    {
                        if (Unit * caster = triggeredByAura->GetCaster())
                        {
                            int32 bp0 = (damage / 2) + 1; // Deal half damage from heal to healer
                            caster->CastCustomSpell(caster, 108128, &bp0, 0, 0, true); // Fel Surge
                        }
                    }
                    break;
                }
                // Unstable Power
                case 24658:
                {
                    if (!procSpell || procSpell->Id == 24659)
                        return false;
                    // Need remove one 24659 aura
                    RemoveAuraFromStack(24659);
                    return true;
                }
                // Restless Strength
                case 24661:
                {
                    // Need remove one 24662 aura
                    RemoveAuraFromStack(24662);
                    return true;
                }
                // Adaptive Warding (Frostfire Regalia set)
                case 28764:
                {
                    if (!procSpell)
                        return false;

                    // find Mage Armor
                    if (!GetAuraEffect(SPELL_AURA_MOD_MANA_REGEN_INTERRUPT, SPELLFAMILY_MAGE, 0x10000000, 0, 0))
                        return false;

                    switch(GetFirstSchoolInMask(GetSpellSchoolMask(procSpell)))
                    {
                        case SPELL_SCHOOL_NORMAL:
                        case SPELL_SCHOOL_HOLY:
                            return false;                   // ignored
                        case SPELL_SCHOOL_FIRE:   triggered_spell_id = 28765; break;
                        case SPELL_SCHOOL_NATURE: triggered_spell_id = 28768; break;
                        case SPELL_SCHOOL_FROST:  triggered_spell_id = 28766; break;
                        case SPELL_SCHOOL_SHADOW: triggered_spell_id = 28769; break;
                        case SPELL_SCHOOL_ARCANE: triggered_spell_id = 28770; break;
                        default:
                            return false;
                    }

                    target = this;
                    break;
                }
                // Obsidian Armor (Justice Bearer`s Pauldrons shoulder)
                case 27539:
                {
                    if (!procSpell)
                        return false;

                    switch(GetFirstSchoolInMask(GetSpellSchoolMask(procSpell)))
                    {
                        case SPELL_SCHOOL_NORMAL:
                            return false;                   // ignore
                        case SPELL_SCHOOL_HOLY:   triggered_spell_id = 27536; break;
                        case SPELL_SCHOOL_FIRE:   triggered_spell_id = 27533; break;
                        case SPELL_SCHOOL_NATURE: triggered_spell_id = 27538; break;
                        case SPELL_SCHOOL_FROST:  triggered_spell_id = 27534; break;
                        case SPELL_SCHOOL_SHADOW: triggered_spell_id = 27535; break;
                        case SPELL_SCHOOL_ARCANE: triggered_spell_id = 27540; break;
                        default:
                            return false;
                    }

                    target = this;
                    break;
                }
                // Mana Leech (Passive) (Priest Pet Aura)
                case 28305:
                {
                    // Cast on owner
                    target = GetOwner();
                    if (!target)
                        return false;

                    triggered_spell_id = 34650;
                    break;
                }
                // Mark of Malice
                case 33493:
                {
                    // Cast finish spell at last charge
                    if (triggeredByAura->GetBase()->GetCharges() > 1)
                        return false;

                    target = this;
                    triggered_spell_id = 33494;
                    break;
                }
                // Twisted Reflection (boss spell)
                case 21063:
                    triggered_spell_id = 21064;
                    break;
                // Vampiric Aura (boss spell)
                case 38196:
                {
                    basepoints0 = 3 * damage;               // 300%
                    if (basepoints0 < 0)
                        return false;

                    triggered_spell_id = 31285;
                    target = this;
                    break;
                }
                // Aura of Madness (Darkmoon Card: Madness trinket)
                //=====================================================
                // 39511 Sociopath: +35 strength (Paladin, Rogue, Druid, Warrior)
                // 40997 Delusional: +70 attack power (Rogue, Hunter, Paladin, Warrior, Druid)
                // 40998 Kleptomania: +35 agility (Warrior, Rogue, Paladin, Hunter, Druid)
                // 40999 Megalomania: +41 damage/healing (Druid, Shaman, Priest, Warlock, Mage, Paladin)
                // 41002 Paranoia: +35 spell/melee/ranged crit strike rating (All classes)
                // 41005 Manic: +35 haste (spell, melee and ranged) (All classes)
                // 41009 Narcissism: +35 intellect (Druid, Shaman, Priest, Warlock, Mage, Paladin, Hunter)
                // 41011 Martyr Complex: +35 stamina (All classes)
                // 41406 Dementia: Every 5 seconds either gives you +5% damage/healing. (Druid, Shaman, Priest, Warlock, Mage, Paladin)
                // 41409 Dementia: Every 5 seconds either gives you -5% damage/healing. (Druid, Shaman, Priest, Warlock, Mage, Paladin)
                case 39446:
                {
                    if (GetTypeId() != TYPEID_PLAYER || !this->IsAlive())
                        return false;

                    // Select class defined buff
                    switch (getClass())
                    {
                        case CLASS_PALADIN:                 // 39511,40997,40998,40999,41002,41005,41009,41011,41409
                        case CLASS_DRUID:                   // 39511,40997,40998,40999,41002,41005,41009,41011,41409
                            triggered_spell_id = RAND(39511,40997,40998,40999,41002,41005,41009,41011,41409);
                            cooldown_spell_id = 39511;
                            break;
                        case CLASS_ROGUE:                   // 39511,40997,40998,41002,41005,41011
                        case CLASS_WARRIOR:                 // 39511,40997,40998,41002,41005,41011
                            triggered_spell_id = RAND(39511,40997,40998,41002,41005,41011);
                            cooldown_spell_id = 39511;
                            break;
                        case CLASS_PRIEST:                  // 40999,41002,41005,41009,41011,41406,41409
                        case CLASS_SHAMAN:                  // 40999,41002,41005,41009,41011,41406,41409
                        case CLASS_MAGE:                    // 40999,41002,41005,41009,41011,41406,41409
                        case CLASS_WARLOCK:                 // 40999,41002,41005,41009,41011,41406,41409
                            triggered_spell_id = RAND(40999,41002,41005,41009,41011,41406,41409);
                            cooldown_spell_id = 40999;
                            break;
                        case CLASS_HUNTER:                  // 40997,40999,41002,41005,41009,41011,41406,41409
                            triggered_spell_id = RAND(40997,40999,41002,41005,41009,41011,41406,41409);
                            cooldown_spell_id = 40997;
                            break;
                        default:
                            return false;
                    }

                    target = this;
                    if (roll_chance_i(10))
                        this->ToPlayer()->Say("This is Madness!", LANG_UNIVERSAL);
                    break;
                }
                // Sunwell Exalted Caster Neck (??? neck)
                // cast ??? Light's Wrath if Exalted by Aldor
                // cast ??? Arcane Bolt if Exalted by Scryers
                case 46569:
                    return false;                           // old unused version
                // Sunwell Exalted Caster Neck (Shattered Sun Pendant of Acumen neck)
                // cast 45479 Light's Wrath if Exalted by Aldor
                // cast 45429 Arcane Bolt if Exalted by Scryers
                case 45481:
                {
                    if (GetTypeId() != TYPEID_PLAYER)
                        return false;

                    // Get Aldor reputation rank
                    if (ToPlayer()->GetReputationRank(932) == REP_EXALTED)
                    {
                        target = this;
                        triggered_spell_id = 45479;
                        break;
                    }
                    // Get Scryers reputation rank
                    if (ToPlayer()->GetReputationRank(934) == REP_EXALTED)
                    {
                        // triggered at positive/self casts also, current attack target used then
                        if (IsFriendlyTo(target))
                        {
                            target = GetVictim();
                            if (!target)
                            {
                                uint64 selected_guid = ToPlayer()->GetSelection();
                                target = ObjectAccessor::GetUnit(*this,selected_guid);
                                if (!target)
                                    return false;
                            }
                            if (IsFriendlyTo(target))
                                return false;
                        }

                        triggered_spell_id = 45429;
                        break;
                    }
                    return false;
                }
                // Sunwell Exalted Melee Neck (Shattered Sun Pendant of Might neck)
                // cast 45480 Light's Strength if Exalted by Aldor
                // cast 45428 Arcane Strike if Exalted by Scryers
                case 45482:
                {
                    if (GetTypeId() != TYPEID_PLAYER)
                        return false;

                    // Get Aldor reputation rank
                    if (ToPlayer()->GetReputationRank(932) == REP_EXALTED)
                    {
                        target = this;
                        triggered_spell_id = 45480;
                        break;
                    }
                    // Get Scryers reputation rank
                    if (ToPlayer()->GetReputationRank(934) == REP_EXALTED)
                    {
                        triggered_spell_id = 45428;
                        break;
                    }
                    return false;
                }
                // Sunwell Exalted Tank Neck (Shattered Sun Pendant of Resolve neck)
                // cast 45431 Arcane Insight if Exalted by Aldor
                // cast 45432 Light's Ward if Exalted by Scryers
                case 45483:
                {
                    if (GetTypeId() != TYPEID_PLAYER)
                        return false;

                    // Get Aldor reputation rank
                    if (ToPlayer()->GetReputationRank(932) == REP_EXALTED)
                    {
                        target = this;
                        triggered_spell_id = 45432;
                        break;
                    }
                    // Get Scryers reputation rank
                    if (ToPlayer()->GetReputationRank(934) == REP_EXALTED)
                    {
                        target = this;
                        triggered_spell_id = 45431;
                        break;
                    }
                    return false;
                }
                // Sunwell Exalted Healer Neck (Shattered Sun Pendant of Restoration neck)
                // cast 45478 Light's Salvation if Exalted by Aldor
                // cast 45430 Arcane Surge if Exalted by Scryers
                case 45484:
                {
                    if (GetTypeId() != TYPEID_PLAYER)
                        return false;

                    // Get Aldor reputation rank
                    if (ToPlayer()->GetReputationRank(932) == REP_EXALTED)
                    {
                        target = this;
                        triggered_spell_id = 45478;
                        break;
                    }
                    // Get Scryers reputation rank
                    if (ToPlayer()->GetReputationRank(934) == REP_EXALTED)
                    {
                        triggered_spell_id = 45430;
                        break;
                    }
                    return false;
                }
                // Living Seed
                case 48504:
                {
                    triggered_spell_id = 48503;
                    basepoints0 = triggerAmount;
                    target = this;
                    break;
                }
                // Kill command
                case 58914:
                {
                    // Remove aura stack from pet
                    RemoveAuraFromStack(58914);
                    Unit* owner = GetOwner();
                    if (!owner)
                        return true;
                    // reduce the owner's aura stack
                    owner->RemoveAuraFromStack(34027);
                    return true;
                }
                // Vampiric Touch (generic, used by some boss)
                case 52723:
                case 60501:
                {
                    triggered_spell_id = 52724;
                    basepoints0 = damage / 2;
                    target = this;
                    break;
                }
                // Shadowfiend Death (Gain mana if pet dies with Glyph of Shadowfiend)
                case 57989:
                {
                    Unit *owner = GetOwner();
                    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
                        return false;
                    // Glyph of Shadowfiend (need cast as self cast for owner, no hidden cooldown)
                    owner->CastSpell(owner,58227,true,castItem,triggeredByAura);
                    return true;
                }
                case 71519: // Deathbringer's Will Normal
                {
                    if (GetTypeId() != TYPEID_PLAYER)
                        return false;

                    std::vector<uint32> RandomSpells;
                    switch (getClass())
                    {
                        case CLASS_WARRIOR:
                        case CLASS_PALADIN:
                        case CLASS_DEATH_KNIGHT:
                            RandomSpells.push_back(71484);
                            RandomSpells.push_back(71491);
                            RandomSpells.push_back(71492);
                            break;
                        case CLASS_SHAMAN:
                        case CLASS_ROGUE:
                            RandomSpells.push_back(71486);
                            RandomSpells.push_back(71485);
                            RandomSpells.push_back(71492);
                            break;
                        case CLASS_DRUID:
                            RandomSpells.push_back(71484);
                            RandomSpells.push_back(71485);
                            RandomSpells.push_back(71486);
                            break;
                        case CLASS_HUNTER:
                            RandomSpells.push_back(71486);
                            RandomSpells.push_back(71491);
                            RandomSpells.push_back(71485);
                            break;
                        default:
                            return false;
                    }
                    if (RandomSpells.empty()) //shouldn't happen
                        return false;

                    uint8 rand_spell = irand(0,(RandomSpells.size() - 1));
                    CastSpell(target,RandomSpells[rand_spell],true,castItem,triggeredByAura, originalCaster);
                    for (std::vector<uint32>::iterator itr = RandomSpells.begin(); itr != RandomSpells.end(); ++itr)
                    {
                        if (!ToPlayer()->HasSpellCooldown(*itr))
                            ToPlayer()->AddSpellCooldown(*itr,0,cooldown*1000);
                    }
                    break;
                }
                case 71562: // Deathbringer's Will Heroic
                {
                    if (GetTypeId() != TYPEID_PLAYER)
                        return false;

                    std::vector<uint32> RandomSpells;
                    switch (getClass())
                    {
                        case CLASS_WARRIOR:
                        case CLASS_PALADIN:
                        case CLASS_DEATH_KNIGHT:
                            RandomSpells.push_back(71561);
                            RandomSpells.push_back(71559);
                            RandomSpells.push_back(71560);
                            break;
                        case CLASS_SHAMAN:
                        case CLASS_ROGUE:
                            RandomSpells.push_back(71558);
                            RandomSpells.push_back(71556);
                            RandomSpells.push_back(71560);
                            break;
                        case CLASS_DRUID:
                            RandomSpells.push_back(71561);
                            RandomSpells.push_back(71556);
                            RandomSpells.push_back(71558);
                            break;
                        case CLASS_HUNTER:
                            RandomSpells.push_back(71558);
                            RandomSpells.push_back(71559);
                            RandomSpells.push_back(71556);
                            break;
                        default:
                            return false;
                    }
                    if (RandomSpells.empty()) //shouldn't happen
                        return false;

                    uint8 rand_spell = irand(0,(RandomSpells.size() - 1));
                    CastSpell(target,RandomSpells[rand_spell],true,castItem,triggeredByAura, originalCaster);
                    for (std::vector<uint32>::iterator itr = RandomSpells.begin(); itr != RandomSpells.end(); ++itr)
                    {
                        if (!ToPlayer()->HasSpellCooldown(*itr))
                            ToPlayer()->AddSpellCooldown(*itr,0,cooldown*1000);
                    }
                    break;
                }
                // Item - Shadowmourne Legendary
                case 71903:
                {
                    if (!pVictim || !pVictim->IsAlive() || HasAura(73422))  // cant collect shards while under effect of Chaos Bane buff
                        return false;

                    CastSpell(this, 71905, true, NULL, triggeredByAura);

                    // this can't be handled in AuraScript because we need to know pVictim
                    Aura const* dummy = GetAura(71905);
                    if (!dummy || dummy->GetStackAmount() < 10)
                        return false;

                    RemoveAurasDueToSpell(71905);
                    triggered_spell_id = 71904;
                    target = pVictim;
                    break;
                }
                // Shadow's Fate (Shadowmourne questline)
                case 71169:
                {
                    triggered_spell_id = 71203;
                    target = triggeredByAura->GetCaster();
                    break;
                }
                // Gaseous Bloat (Professor Putricide add)
                case 70215:
                case 72858:
                case 72859:
                case 72860:
                {
                    target = GetVictim();
                    triggered_spell_id = 70701;
                    break;
                }

                //Matrix Restabilizer
                case 96976:
                case 97138:
                {
                    Player* owner=this->ToPlayer();
                    if (owner->HasSpellCooldown(dummySpell->Id))
                        return false;
                    owner->AddSpellAndCategoryCooldowns(sSpellStore.LookupEntry(dummySpell->Id), castItem->GetProto()->ItemId);
                    uint32 crit=owner->GetUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + CR_CRIT_MELEE);
                    uint32 haste=owner->GetUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + CR_HASTE_MELEE);
                    uint32 mastery=owner->GetUInt32Value(PLAYER_FIELD_COMBAT_RATING_1 + CR_MASTERY);
                    if(crit>=haste&&crit>=mastery)
                    {
                        if(dummySpell->Id==96976)
                        {
                            triggered_spell_id = 96978;
                            target = triggeredByAura->GetCaster();
                            break;
                        }
                        else
                        {
                            triggered_spell_id = 97140;
                            target = triggeredByAura->GetCaster();
                            break;
                        }
                    }
                    if(haste>=crit&&haste>=mastery)
                    {
                        if(dummySpell->Id==96976)
                        {
                            triggered_spell_id = 96977;
                            target = triggeredByAura->GetCaster();
                            break;
                        }
                        else
                        {
                            triggered_spell_id = 97139;
                            target = triggeredByAura->GetCaster();
                            break;
                        }
                    }
                    if(mastery>=crit&&mastery>=haste)
                    {
                        if(dummySpell->Id==96976)
                        {
                            triggered_spell_id = 96979;
                            target = triggeredByAura->GetCaster();
                            break;
                        }
                        else
                        {
                            triggered_spell_id = 97141;
                            target = triggeredByAura->GetCaster();
                            break;
                        }
                    }
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_MAGE:
        {
            // Magic Absorption
            if (dummySpell->SpellIconID == 459)             // only this spell have SpellIconID == 459 and dummy aura
            {
                if (getPowerType() != POWER_MANA)
                    return false;

                // mana reward
                basepoints0 = (triggerAmount * GetMaxPower(POWER_MANA) / 100);
                target = this;
                triggered_spell_id = 29442;
                break;
            }
            // Master of Elements
            if (dummySpell->SpellIconID == 1920)
            {
                if (!procSpell)
                    return false;

                // mana cost save
                int32 cost = procSpell->manaCost + procSpell->ManaCostPercentage * GetCreateMana() / 100;
                basepoints0 = cost * triggerAmount/100;
                if (basepoints0 <= 0)
                    return false;

                target = this;
                triggered_spell_id = 29077;
                break;
            }
            // Arcane Potency
            if (dummySpell->SpellIconID == 2120)
            {
                if (!procSpell)
                    return false;

                target = this;
                switch (dummySpell->Id)
                {
                    case 31571: triggered_spell_id = 57529; break;
                    case 31572: triggered_spell_id = 57531; break;
                    default:
                        sLog->outError("Unit::HandleDummyAuraProc: non handled spell id: %u",dummySpell->Id);
                        return false;
                }
                break;
            }

            // Permafrost healing spell proc
            if (dummySpell->Id == 12571 || dummySpell->Id == 12569 || dummySpell->Id == 11175)
            {
                if (damage > 0 && triggeredByAura && triggeredByAura->GetAmount() > 0)
                {
                    int32 bp0 = damage * triggeredByAura->GetAmount() / 100.0f;
                    CastCustomSpell(this, 91394, &bp0, 0, 0, true);
                }

                if (HasAura(11175)) // Permafrost (Rank 1)
                    CastCustomSpell(68391, SPELLVALUE_BASE_POINT0, -3, pVictim, true);
                else if (HasAura(12569)) // Permafrost (Rank 2)
                    CastCustomSpell(68391, SPELLVALUE_BASE_POINT0, -7, pVictim, true);
                else if (HasAura(12571)) // Permafrost (Rank 3)
                    CastCustomSpell(68391, SPELLVALUE_BASE_POINT0, -10, pVictim, true); 
            }
            // Hot Streak
            if (dummySpell->Id == 44445)
            {
                if (effIndex != 0)
                    return false;

                if (procEx & PROC_EX_CRITICAL_HIT)
                {
                    float critChance = GetFloatValue(PLAYER_SPELL_CRIT_PERCENTAGE1 + SPELL_SCHOOL_FIRE);
                    float chance = 78.93f - 1.7106f * critChance;
                    if (GetTypeId() == TYPEID_PLAYER)
                        ToPlayer()->ApplySpellMod(44549, SPELLMOD_CHANCE_OF_SUCCESS, chance, NULL); // T12 4p set bonus

                    if (roll_chance_f(chance))
                        CastSpell(this, 48108, true);
                }
                return true;
            }
            // Improved Hot Streak
            if (dummySpell->Id == 44446 || dummySpell->Id == 44448)
            {
                if (effIndex != 0)
                    return false;
                AuraEffect *counter = triggeredByAura->GetBase()->GetEffect(1);
                if (!counter)
                    return true;

                // Count spell criticals in a row in second aura
                if ((procEx & PROC_EX_CRITICAL_HIT) && !HasAura(48108))
                {
                    counter->SetAmount(counter->GetAmount()*2);
                    if (counter->GetAmount() < 100) // not enough
                        return true;
                    // Crititcal counted -> roll chance
                    if (roll_chance_i(triggerAmount))
                       CastSpell(this, 48108, true, castItem, triggeredByAura);
                }
                counter->SetAmount(25);
                return true;
            }
            // Incanter's Regalia set (add trigger chance to Mana Shield)
            if (dummySpell->SpellFamilyFlags[0] & 0x8000)
            {
                if (GetTypeId() != TYPEID_PLAYER)
                    return false;

                target = this;
                triggered_spell_id = 37436;
                break;
            }
            switch(dummySpell->Id)
            {
                // Glyph of Icy Veins
                case 56374:
                {
                    RemoveAurasByType(SPELL_AURA_HASTE_SPELLS, 0, 0, true, false);
                    RemoveAurasByType(SPELL_AURA_MOD_DECREASE_SPEED);
                    return true;
                }
                // Ignite talent aura
                case 11119:
                case 11120:
                case 12846:
                {
                    AuraEffect * aurEff = GetAuraEffect(dummySpell->Id, EFFECT_0, GetGUID());

                    if (!aurEff)
                        return false;

                    basepoints0 = damage * aurEff->GetAmount() / 100;
                    triggered_spell_id = 12654;

                    // Ignite should profit from mastery
                    if (GetTypeId() == TYPEID_PLAYER)
                        ToPlayer()->ApplySpellMod(12654, SPELLMOD_DOT, basepoints0);

                    basepoints0 = basepoints0 / 2;
                    basepoints0 += pVictim->GetRemainingPeriodicAmount(GetGUID(), triggered_spell_id,SPELL_AURA_PERIODIC_DAMAGE,EFFECT_0);
                    break;
                }
                // Glyph of Ice Block
                case 56372:
                {
                    if (GetTypeId() != TYPEID_PLAYER)
                        return false;

                    // remove cooldown from Frost Nova
                    ToPlayer()->RemoveSpellCooldown(122, true);
                    break;
                }
                // Blessing of Ancient Kings (Val'anyr, Hammer of Ancient Kings)
                case 64411:
                {
                    basepoints0 = int32(CalculatePctN(damage, 15));
                    if (AuraEffect* aurEff = pVictim->GetAuraEffect(64413, 0, GetGUID()))
                    {
                        // The shield can grow to a maximum size of 20,000 damage absorbtion
                        aurEff->SetAmount(std::max<int32>(aurEff->GetAmount() + basepoints0, 20000));

                        // Refresh and return to prevent replacing the aura
                        aurEff->GetBase()->RefreshDuration();
                        return true;
                    }
                    target = pVictim;
                    triggered_spell_id = 64413;
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            switch(dummySpell->Id)
            {
                // Sweeping Strikes
                case 12328:
                {
                    target = SelectNearbyTarget();
                    // If player select something else during SS ( self for example ) SS can still proc, which is wrong
                    if (!target || target == this || target->IsFriendlyTo(this) || !target->IsWithinMeleeRange(this))
                        return false;

                    triggered_spell_id = 26654;
                    break;
                }
                // Improved Spell Reflection
                case 59088:
                case 59089:
                {
                    triggered_spell_id = 59725;
                    target = this;
                    break;
                }
            }

            // Retaliation
            if (dummySpell->SpellFamilyFlags[1] & 0x8)
            {
                // check attack comes not from behind
                if (!HasInArc(M_PI, pVictim))
                    return false;

                triggered_spell_id = 22858;
                break;
            }
            if (dummySpell->Id == 99239) // Warior T12 Protection 2P Bonus
            {
                if(procSpell->Id == 23922) // Shield Slam
                {
                    CastCustomSpell(99240, SPELLVALUE_BASE_POINT0,damage * 0.1, pVictim, true); // Combust
                    break;
                }
                else return (false);
            }
            // Second Wind
            if (dummySpell->SpellIconID == 1697)
            {
                // only for spells and hit/crit (trigger start always) and not start from self casted spells (5530 Mace Stun Effect for example)
                if (procSpell == 0 || !(procEx & (PROC_EX_NORMAL_HIT|PROC_EX_CRITICAL_HIT)) || this == pVictim)
                    return false;
                // Need stun or root mechanic
                if (!(GetAllSpellMechanicMask(procSpell) & ((1<<MECHANIC_ROOT)|(1<<MECHANIC_STUN))))
                    return false;

                switch (dummySpell->Id)
                {
                    case 29838: triggered_spell_id=29842; break;
                    case 29834: triggered_spell_id=29841; break;
                    case 42770: triggered_spell_id=42771; break;
                    default:
                        sLog->outError("Unit::HandleDummyAuraProc: non handled spell id: %u (SW)",dummySpell->Id);
                    return false;
                }

                target = this;
                break;
            }
            // Glyph of Sunder Armor
            if (dummySpell->Id == 58387)
            {
                if (!pVictim || !pVictim->IsAlive() || !procSpell)
                    return false;

                target = SelectNearbyTarget();
                if (!target || target == pVictim)
                    return false;

                CastSpell(target, 58567, true);
                return true;
            }
            break;
        }
        case SPELLFAMILY_WARLOCK:
        {
            // Seed of Corruption
            if (dummySpell->SpellFamilyFlags[1] & 0x00000010)
            {
                if (procSpell && (procSpell->Id == 27243 || procSpell->Id == 27285))
                    return false;
                // if damage is more than need or target die from damage deal finish spell
                if (triggeredByAura->GetAmount() <= int32(damage) || GetHealth() <= damage)
                {
                    // remember guid before aura delete
                    uint64 casterGuid = triggeredByAura->GetCasterGUID();

                    // Remove aura (before cast for prevent infinite loop handlers)
                    RemoveAurasDueToSpell(triggeredByAura->GetId());

                    // Cast finish spell (triggeredByAura already not exist!)
                    if (Unit* caster = GetUnit(*this, casterGuid))
                        caster->CastSpell(this, 27285, true, castItem);
                    return true;                            // no hidden cooldown
                }

                // Damage counting
                triggeredByAura->SetAmount(triggeredByAura->GetAmount() - damage);
                return true;
            }
            // Seed of Corruption (Mobs cast) - no die req
            if (dummySpell->SpellFamilyFlags.IsEqual(0, 0, 0) && dummySpell->SpellIconID == 1932)
            {
                // if damage is more than need deal finish spell
                if (triggeredByAura->GetAmount() <= int32(damage))
                {
                    // remember guid before aura delete
                    uint64 casterGuid = triggeredByAura->GetCasterGUID();

                    // Remove aura (before cast for prevent infinite loop handlers)
                    RemoveAurasDueToSpell(triggeredByAura->GetId());

                    // Cast finish spell (triggeredByAura already not exist!)
                    if (Unit* caster = GetUnit(*this, casterGuid))
                        caster->CastSpell(this, 32865, true, castItem);
                    return true;                            // no hidden cooldown
                }
                // Damage counting
                triggeredByAura->SetAmount(triggeredByAura->GetAmount() - damage);
                return true;
            }
            // Fel Synergy
            if (dummySpell->SpellIconID == 3222)
            {
                target = GetGuardianPet();
                if (!target)
                    return false;
                basepoints0 = damage * triggerAmount / 100;
                triggered_spell_id = 54181;
                break;
            }
            switch (dummySpell->Id)
            {
                // Siphon Life
                case 63108:
                {
                    triggered_spell_id = 63106;
                    target = this;
                    basepoints0 = int32(damage*triggerAmount/100);
                    break;
                }
                // Glyph of Shadowflame
                case 63310:
                {
                    triggered_spell_id = 63311;
                    break;
                }
                // Nightfall
                case 18094:
                case 18095:
                // Glyph of corruption
                case 56218:
                {
                    target = this;
                    triggered_spell_id = 17941;
                    break;
                }
                // Shadowflame (Voidheart Raiment set bonus)
                case 37377:
                {
                    triggered_spell_id = 37379;
                    break;
                }
                // Pet Healing (Corruptor Raiment or Rift Stalker Armor)
                case 37381:
                {
                    target = GetGuardianPet();
                    if (!target)
                        return false;

                    // heal amount
                    basepoints0 = damage * triggerAmount/100;
                    triggered_spell_id = 37382;
                    break;
                }
                // Shadowflame Hellfire (Voidheart Raiment set bonus)
                case 39437:
                {
                    triggered_spell_id = 37378;
                    break;
                }
                // Fel Armor
                case 28176:
                {
                    target = this;

                    triggerAmount = sSpellMgr->CalculateSpellEffectAmount(dummySpell, EFFECT_1, this);
                    // heal amount
                    basepoints0 = damage * triggerAmount/100.0f;
                    triggered_spell_id = 96379;
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_PRIEST:
        {
            // Vampiric Touch
            if (dummySpell->SpellFamilyFlags[1] & 0x00000400)
            {
                if (!pVictim || !pVictim->IsAlive())
                    return false;

                if (effIndex != 0)
                    return false;

                // pVictim is caster of aura
                if (triggeredByAura->GetCasterGUID() != pVictim->GetGUID())
                    return false;

                // Energize 0.25% of max. mana
                pVictim->CastSpell(pVictim,57669,true,castItem,triggeredByAura);
                return true;                                // no hidden cooldown
            }
            // Glyph of Spirit Tap
            if (dummySpell->Id == 63237)
            {
                target = this;
                triggered_spell_id = 81301;
            }
            // Divine Aegis
            if (dummySpell->SpellIconID == 2820)
            {
                bool crit = (procEx & PROC_EX_CRITICAL_HIT);
                bool PoH = (procSpell->Id == 596);

                if(!crit && !PoH)
                    return false;

                if(crit && PoH)
                    triggerAmount *= 2;

                if (ToPlayer() && ToPlayer()->HasMastery() &&
                    ToPlayer()->GetTalentBranchSpec(ToPlayer()->GetActiveSpec()) == SPEC_PRIEST_DISCIPLINE)
                {
                    // Calculate amount with mastery also - needs to be there due to problems with absorb cap of this talent
                   triggerAmount *= 1.0f+ToPlayer()->GetMasteryPoints()*2.5f/100.0f;
                }

                // Multiple effects stack, so let's try to find this aura.
                int32 bonus = 0;
                if (AuraEffect *aurEff = target->GetAuraEffect(47753, 0))
                    bonus = aurEff->GetAmount();

                basepoints0 = damage * triggerAmount/100 + bonus;

                if (basepoints0 > (0.4f * GetMaxHealth()))
                    basepoints0 = 0.4f * GetMaxHealth();

                triggered_spell_id = 47753;
                break;
            }
            // Body and Soul
            if (dummySpell->SpellIconID == 2218)
            {
                // Proc only from Abolish desease on self cast
                if (procSpell->Id != 552 || pVictim != this || !roll_chance_i(triggerAmount))
                    return false;
                triggered_spell_id = 64136;
                target = this;
                break;
            }
            switch(dummySpell->Id)
            {
                // Vampiric Embrace
                case 15286:
                {
                    if (!pVictim || !pVictim->IsAlive() || procSpell->SpellFamilyFlags[1] & 0x80000)
                        return false;

                    // heal amount
                    int32 team = triggerAmount*damage/500;
                    int32 self = triggerAmount*damage/100 - team;
                    CastCustomSpell(this,15290,&team,&self,NULL,true,castItem,triggeredByAura);
                    return true;                                // no hidden cooldown
                }
                // Priest Tier 6 Trinket (Ashtongue Talisman of Acumen)
                case 40438:
                {
                    // Shadow Word: Pain
                    if (procSpell->SpellFamilyFlags[0] & 0x8000)
                        triggered_spell_id = 40441;
                    // Renew
                    else if (procSpell->SpellFamilyFlags[0] & 0x40)
                        triggered_spell_id = 40440;
                    else
                        return false;

                    target = this;
                    break;
                }
                // Glyph of Prayer of Healing
                case 55680:
                {
                    triggered_spell_id = 56161;

                    SpellEntry const* GoPoH = sSpellStore.LookupEntry(triggered_spell_id);
                    if (!GoPoH)
                        return false;

                    int EffIndex = 0;
                    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; i++)
                    {
                        if (GoPoH->Effect[i] == SPELL_EFFECT_APPLY_AURA)
                        {
                            EffIndex = i;
                            break;
                        }
                    }
                    int32 tickcount = GetSpellMaxDuration(GoPoH) / GoPoH->EffectAmplitude[EffIndex];
                    if (!tickcount)
                        return false;

                    basepoints0 = damage * triggerAmount / tickcount / 100;
                    break;
                }
                // Improved Shadowform
                case 47570:
                case 47569:
                {
                    if (!roll_chance_i(triggerAmount))
                        return false;

                    RemoveMovementImpairingAuras();
                    break;
                }
                // Glyph of Dispel Magic
                case 55677:
                {
                    // Dispel Magic shares spellfamilyflag with abolish disease
                    if (procSpell->SpellIconID != 74)
                        return false;
                    if (!target || !target->IsFriendlyTo(this))
                        return false;

                    basepoints0 = int32(target->CountPctFromMaxHealth(triggerAmount));
                    triggered_spell_id = 56131;
                    break;
                }
                // Oracle Healing Bonus ("Garments of the Oracle" set)
                case 26169:
                {
                    // heal amount
                    basepoints0 = int32(damage * 10/100);
                    target = this;
                    triggered_spell_id = 26170;
                    break;
                }
                // Frozen Shadoweave (Shadow's Embrace set) warning! its not only priest set
                case 39372:
                {
                    if (!procSpell || (GetSpellSchoolMask(procSpell) & (SPELL_SCHOOL_MASK_FROST | SPELL_SCHOOL_MASK_SHADOW)) == 0)
                        return false;

                    // heal amount
                    basepoints0 = damage * triggerAmount/100;
                    target = this;
                    triggered_spell_id = 39373;
                    break;
                }
                // Greater Heal (Vestments of Faith (Priest Tier 3) - 4 pieces bonus)
                case 28809:
                {
                    triggered_spell_id = 28810;
                    break;
                }
                // Priest T10 Healer 2P Bonus
                case 70770:
                    // Flash Heal
                    if (procSpell->SpellFamilyFlags[0] & 0x800)
                    {
                        triggered_spell_id = 70772;
                        SpellEntry const* blessHealing = sSpellStore.LookupEntry(triggered_spell_id);
                        if (!blessHealing)
                            return false;
                        basepoints0 = int32(triggerAmount * damage / 100 / (GetSpellMaxDuration(blessHealing) / blessHealing->EffectAmplitude[0]));
                    }
                    break;
                // Evangelism
                case 81659:
                case 81662:
                {
                    // Smite and Holy Fire
                    if (!(procSpell->Id == 585) && !(procSpell->Id == 14914))
                        return false;
                    
                    // Proc only from  direct dmg of Holy Fire
                    if (procSpell->Id == 14914 && (procFlag & PROC_FLAG_DONE_PERIODIC))
                        return false;

                    // Done to someone else
                    if (!target || target == this)
                        return false;

                    // Done hit / crit
                    if (!(procEx & (PROC_EX_NORMAL_HIT | PROC_EX_CRITICAL_HIT)))
                        return false;

                    uint32 auraid = dummySpell->Id == 81659 ? 81660 : 81661; // Rank 1 or Rank 2

                    // Remove Dark Evangelism
                    RemoveAurasDueToSpell(87117);
                    RemoveAurasDueToSpell(87118);

                    // Aura is already active
                    if (Aura* pEvangelism = GetAura(auraid))
                    {
                        uint8 charges = pEvangelism->GetCharges();
                        if (charges < 5)
                        {
                            // Add charge
                            pEvangelism->SetCharges(++charges);
                            pEvangelism->SetStackAmount(charges);
                        }

                        // cast marker aura
                        if (charges >= 5 && !HasAura(94709))
                            CastSpell(this, 94709, true);

                        // Refresh duration not considering number of charges
                        pEvangelism->RefreshDuration();
                    }
                    else
                    {
                        // Cast a new one
                        CastSpell(this, auraid, true);

                        // Fresh aura has 0 charges, add one
                        if (Aura* aura = GetAura(auraid))
                        {
                            aura->SetCharges(1);
                            aura->SetStackAmount(1);
                        }
                    }

                    // Enable Archangel spell (87151)
                    if (Aura* pAura = GetAura(87154))
                        pAura->RefreshDuration();
                    else
                        CastSpell(this, 87154, true);

                    return false;
                }
                // Atonement
                case 14523:
                case 81749:
                {
                    if (!pVictim || (procSpell->Id != 585 && procSpell->Id != 14914)) // Smite, Holy Fire
                        return false;

                    int32 bp0 = damage * dummySpell->GetSpellEffect(0)->EffectBasePoints / 100;

                    CastCustomSpell(pVictim, 94472, &bp0, NULL, NULL, true, 0, triggeredByAura, GetGUID());
                    // Fails when cast on dummy target
                    return false;
                }
            }
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            switch(dummySpell->Id)
            {
                // Glyph of Innervate
                case 54832:
                {
                    if (procSpell->SpellIconID != 62)
                        return false;

                    int32 mana_perc = SpellMgr::CalculateSpellEffectAmount(triggeredByAura->GetSpellProto(), triggeredByAura->GetEffIndex());
                    basepoints0 = uint32((GetCreatePowers(POWER_MANA) * mana_perc / 100) / 10);
                    triggered_spell_id = 54833;
                    target = this;
                    break;
                }
                // Glyph of Bloodletting
                case 54815:
                {
                    if (procSpell->Id != 5221) // Shred
                        return false;

                    // Mangle ( cat form ) -> is handled in void Spell::SpellDamageWeaponDmg , because from unknown reason mangle will never get here

                    // Try to find spell Rip on the target
                    if (AuraEffect *ripEff = target->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DRUID, 0x00800000, 0x0, 0x0, GetGUID()))
                    {
                        int32 refreshCounter = ripEff->GetScriptedAmount();
                        if (refreshCounter < 3)
                        {
                            ripEff->GetBase()->SetDuration(ripEff->GetBase()->GetDuration() + 2000);
                            ripEff->SetScriptedAmount(refreshCounter + 1);
                            return true;
                        }
                        else
                            return false;
                    }
                    // if not found Rip
                    return false;
                }
                case 99001: // Druid T12 Feral 2P Bonus
                {
                    if(procSpell->SpellIconID == 2312 || procSpell->Id == 6807 || procSpell->Id == 5221  ) // Mangle, Maul, Shred
                    {
                        CastCustomSpell(99002, SPELLVALUE_BASE_POINT0,damage * 0.05, pVictim, true); // Fiery Claws + 10 % dmg as fire dot, but 5% per tick ( 2 ticks )
                        break;
                    }
                    else return (false);
                }
                // Leader of the Pack
                case 24932:
                {
                   if (triggerAmount <= 0)
                        return false;
                    basepoints0 = int32(CountPctFromMaxHealth(triggerAmount));
                    target = this;
                    triggered_spell_id = 34299;
                    if (triggeredByAura->GetCasterGUID() != GetGUID())
                        break;
                    int32 basepoints1 = triggerAmount * 2;
                    // Mana Part
                    CastCustomSpell(this,68285,&basepoints1,0,0,true,0,triggeredByAura);
                    break;
                }
                // Healing Touch (Dreamwalker Raiment set)
                case 28719:
                {
                    // mana back
                    basepoints0 = int32(procSpell->manaCost * 30 / 100);
                    target = this;
                    triggered_spell_id = 28742;
                    break;
                }
                // Healing Touch Refund (Idol of Longevity trinket)
                case 28847:
                {
                    target = this;
                    triggered_spell_id = 28848;
                    break;
                }
                // Mana Restore (Malorne Raiment set / Malorne Regalia set)
                case 37288:
                case 37295:
                {
                    target = this;
                    triggered_spell_id = 37238;
                    break;
                }
                // Druid Tier 6 Trinket
                case 40442:
                {
                    float  chance;

                    // Starfire
                    if (procSpell->SpellFamilyFlags[0] & 0x4)
                    {
                        triggered_spell_id = 40445;
                        chance = 25.0f;
                    }
                    // Rejuvenation
                    else if (procSpell->SpellFamilyFlags[0] & 0x10)
                    {
                        triggered_spell_id = 40446;
                        chance = 25.0f;
                    }
                    // Mangle (Bear) and Mangle (Cat)
                    else if (procSpell->SpellFamilyFlags[1] & 0x00000440)
                    {
                        triggered_spell_id = 40452;
                        chance = 40.0f;
                    }
                    else
                        return false;

                    if (!roll_chance_f(chance))
                        return false;

                    target = this;
                    break;
                }
                // Maim Interrupt
                case 44835:
                {
                    // Deadly Interrupt Effect
                    triggered_spell_id = 32747;
                    break;
                }
                case 99174: //  Rogue T12 2P Bonus ( SPELLFAMILY_DRUID seriously blizz ??? )
                {
                    if (procEx & PROC_EX_CRITICAL_HIT) // Only from melee critical strikes 
                    {
                        int32 bp0 = int32((damage*3)/100);
                        CastCustomSpell(pVictim,99173, &bp0, 0, 0, true); // Burning wounds
                    }
                    break;
                }
                // Item - Druid T10 Balance 4P Bonus
                case 70723:
                {
                    // Wrath & Starfire
                    if ((procSpell->SpellFamilyFlags[0] & 0x5) && (procEx & PROC_EX_CRITICAL_HIT))
                    {
                        triggered_spell_id = 71023;
                        SpellEntry const* triggeredSpell = sSpellStore.LookupEntry(triggered_spell_id);
                        if (!triggeredSpell)
                            return false;
                        basepoints0 = int32(triggerAmount * damage / 100 / (GetSpellMaxDuration(triggeredSpell) / triggeredSpell->EffectAmplitude[0]));
                    }
                    break;
                }
                // Item - Druid T10 Restoration 4P Bonus (Rejuvenation)
                case 70664:
                {
                    // Proc only from normal Rejuvenation
                    if (procSpell->SpellVisual[0] != 32)
                        return false;

                    Player* caster = ToPlayer();
                    if (!caster)
                        return false;
                    if (!caster->GetGroup() && pVictim == this)
                        return false;

                    CastCustomSpell(70691, SPELLVALUE_BASE_POINT0, damage, pVictim, true);
                    return true;
                }
                // PvP balance 4p bonus(Sudden Eclipse)
                case 46832:
                {
                    Player *caster = ToPlayer();
                    if (!caster || caster->IsInEclipse())
                        break;
                    if (caster->HasSpellCooldown(95746))
                        break;
                    if (caster->GetActiveTalentBranchSpec() != SPEC_DRUID_BALANCE)
                        break;
                    caster->AddSpellAndCategoryCooldowns(sSpellStore.LookupEntry(95746), 0);
                    int32 change = caster->IsEclipseDriverLeft() ? -13 : 20;
                    caster->ModifyPower(POWER_ECLIPSE, change);
                    return true;
                }
            }
            // Eclipse
            if (dummySpell->SpellIconID == 2856 && GetTypeId() == TYPEID_PLAYER)
            {
                if (!procSpell || effIndex != 0)
                    return false;

                bool isWrathSpell = (procSpell->SpellFamilyFlags[0] & 1);

                if (!roll_chance_f(dummySpell->procChance * (isWrathSpell ? 0.6f : 1.0f)))
                    return false;

                target = this;
                if (target->HasAura(isWrathSpell ? 48517 : 48518))
                    return false;

                triggered_spell_id = isWrathSpell ? 48518 : 48517;
                break;
            }
            // Living Seed
            else if (dummySpell->SpellIconID == 2860)
            {
                triggered_spell_id = 48504;
                basepoints0 = triggerAmount * damage / 100;
                break;
            }
            // King of the Jungle
            else if (dummySpell->SpellIconID == 2850)
            {
                // Effect 0 - mod damage while having Enrage
                if (effIndex == 0)
                {
                    if (!(procSpell->SpellFamilyFlags[0] & 0x00080000))
                        return false;
                    triggered_spell_id = 51185;
                    basepoints0 = triggerAmount;
                    target = this;
                    break;
                }
                // Effect 1 - Tiger's Fury restore energy
                else if (effIndex == 1)
                {
                    if (!(procSpell->SpellFamilyFlags[2] & 0x00000800))
                        return false;
                    triggered_spell_id = 51178;
                    basepoints0 = triggerAmount;
                    target = this;
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_ROGUE:
        {
            switch(dummySpell->Id)
            {
                // Deadly Throw Interrupt
                case 32748:
                {
                    // Prevent cast Deadly Throw Interrupt on self from last effect (apply dummy) of Deadly Throw
                    if (this == pVictim)
                        return false;

                    triggered_spell_id = 32747;
                    break;
                }
                // Honor Among Thieves
                case 51701:
                case 51700:
                case 51698:
                {
                    uint32 pchance = SpellMgr::CalculateSpellEffectAmount(dummySpell, EFFECT_0);

                    if (roll_chance_i(pchance))
                        if (triggeredByAura && triggeredByAura->GetCaster() && triggeredByAura->GetCaster()->IsAlive()
                            && triggeredByAura->GetCaster()->GetVictim())
                        {
                            if (triggeredByAura->GetCaster()->GetTypeId() == TYPEID_PLAYER)
                            {
                                if (!triggeredByAura->GetCaster()->ToPlayer()->HasSpellCooldown(51699))
                                {
                                    triggeredByAura->GetCaster()->CastSpell(triggeredByAura->GetCaster()->GetVictim(), 51699, true);
                                    triggeredByAura->GetCaster()->ToPlayer()->AddSpellAndCategoryCooldowns(sSpellStore.LookupEntry(51699), 0);
                                }
                            }
                        }

                    break;
                }
                // Glyph of Hemorrhage
                case 56807:
                {
                    // proc only from Hemorrhage
                    if (procSpell && procSpell->Id != 16511)
                        return false;

                    triggered_spell_id = 89775;
                    target = pVictim;
                    basepoints0 = damage*0.4f / 8.0f; // 40% damage in 8 ticks (24sec duration, 3sec amplitude)

                    break;
                }
                // Bandit's Guile
                case 84652:
                case 84653:
                case 84654:
                {
                    // should proc only from Sinister Strike and Revealing Strike
                    if (procSpell && (procSpell->Id == 1752 || procSpell->Id == 84617))
                    {
                        triggered_spell_id = 84748;
                        target = pVictim;
                        originalCaster = GetGUID();
                    }
                    else
                        return false;

                    break;
                }
            }
            // Cut to the Chase
            if (dummySpell->SpellIconID == 2909)
            {
                // "refresh your Slice and Dice duration to its 5 combo point maximum"
                // lookup Slice and Dice
                if (AuraEffect const* aur = GetAuraEffect(SPELL_AURA_MOD_MELEE_HASTE, SPELLFAMILY_ROGUE, 0x40000, 0, 0))
                {
                    aur->GetBase()->SetDuration(GetSpellMaxDuration(aur->GetSpellProto()), true);
                    return true;
                }
                return false;
            }
            // Deadly Brew
            else if (dummySpell->SpellIconID == 2963)
            {
                triggered_spell_id = 3409;
                break;
            }
            // Quick Recovery
            else if (dummySpell->SpellIconID == 2116)
            {
                if (!procSpell)
                    return false;

                // energy cost save
                basepoints0 = procSpell->manaCost * triggerAmount/100;
                if (basepoints0 <= 0)
                    return false;

                target = this;
                triggered_spell_id = 31663;
                break;
            }
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            // Tamed Pet Passive  ( Listening for procs )
            // Here should be handled all custom procs from pets !!!
            if (dummySpell->Id == 83727)
            {
                Unit * pet = triggeredByAura->GetCaster(); // Hunter's Pet
                if (!pet)
                    return false;

                Unit* pHunter = Unit::GetUnit(*pet, pet->GetOwnerGUID());

                if (!pHunter)
                    return false;

                if (Pet::IsPetBasicAttackSpell(procSpell->Id))
                {
                    // Remove Sic 'em buff after casting basic attack from Hunter
                    pHunter->RemoveAura(83359);
                    pHunter->RemoveAura(89388);

                    if (procEx & PROC_EX_CRITICAL_HIT) // Invigoration talent proc only from critical basic attacks
                    {
                        if (pHunter->HasAura(53257))
                            pHunter->RemoveAuraFromStack(53257); // Drop 1 charge of Cobra strike when pet critical strike with his ability

                        if (AuraEffect * aurEff = pHunter->GetDummyAuraEffect(SPELLFAMILY_HUNTER, 3487, 0)) // Has Invigoration talent
                        {
                            int32 bp0 = aurEff->GetAmount();
                            pHunter->CastCustomSpell(pHunter, 53398, &bp0, NULL, NULL, true); // Focus gain
                        }
                    }
                }
                else // Procs from non basic attacks of pet
                {

                }

                break;
            }
            // Thrill of the Hunt
            if (dummySpell->SpellIconID == 2236)
            {
                if (!procSpell)
                    return false;

                Spell * spell = ToPlayer()->m_spellModTakingSpell;

                // Disable charge drop because of Lock and Load
                ToPlayer()->SetSpellModTakingSpell(spell, false);

                // Explosive Shot
                if (procSpell->SpellFamilyFlags[2] & 0x200)
                {
                    if (AuraEffect const* pEff = pVictim->GetAuraEffect(SPELL_AURA_PERIODIC_DUMMY, SPELLFAMILY_HUNTER, 0x0, 0x80000000, 0x0, GetGUID()))
                        basepoints0 = CalculatePowerCost(pEff->GetSpellProto(), this, SpellSchoolMask(pEff->GetSpellProto()->SchoolMask)) * 4/10/3;
                }
                else
                    basepoints0 = CalculatePowerCost(procSpell, this, SpellSchoolMask(procSpell->SchoolMask)) * 4/10;

                ToPlayer()->SetSpellModTakingSpell(spell, true);

                if (basepoints0 <= 0)
                    return false;

                target = this;
                triggered_spell_id = 34720;
                break;
            }
            // Hunting Party
            if (dummySpell->SpellIconID == 3406)
            {
                triggered_spell_id = 57669;
                target = this;
                break;
            }
            if (dummySpell->Id == 83340 || dummySpell->Id == 83356) // Sic 'Em! (Rank 1/2)
            {
                //Proc only from critical Arcane Shot, Aimed Shot or Explosive Shot 
                if ((procSpell->Id == 3044 || procSpell->Id == 19434 || procSpell->Id == 53301) && (procEx & PROC_EX_CRITICAL_HIT))
                {
                    // Cast Sic 'em buff on hunter
                    this->CastSpell(this, (dummySpell->Id == 83340) ? 83359 : 89388, true);
                    break;
                }
                else
                    return false;
            }
            // Crouching Tiger, Hidden Chimera 
            if (dummySpell->SpellIconID == 4752 && GetTypeId() == TYPEID_PLAYER)
            {
                if (ToPlayer()->HasSpellCooldown(82898) || ToPlayer()->HasSpellCooldown(82899))
                    return false;

                // Whenever you are hit by a melee attack, the cooldown of your Disengage is instantly reduced by 2/4 seconds
                if ((procFlag & PROC_FLAG_TAKEN_MELEE_AUTO_ATTACK) || (procFlag & PROC_FLAG_TAKEN_SPELL_MELEE_DMG_CLASS))
                {
                    if (dummySpell->Id == 82898) // Rank 1
                        ToPlayer()->ModifySpellCooldown(781,-2000,true);
                    else                         // Rank 2
                        ToPlayer()->ModifySpellCooldown(781,-4000,true);

                    ToPlayer()->AddSpellCooldown(82898,0,2000);
                }
                // Whenever you are hit by a ranged attack or spell, the cooldown of your Deterrence is instantly reduced by 4/8 seconds
                else
                {
                    if (dummySpell->Id == 82898) // Rank 1
                        ToPlayer()->ModifySpellCooldown(19263,-4000,true);
                    else                         // Rank 2
                        ToPlayer()->ModifySpellCooldown(19263,-8000,true);

                    ToPlayer()->AddSpellCooldown(82899,0,2000);
                }
                break;
            }
            // Rapid Recuperation
            if (dummySpell->SpellIconID == 3560)
            {
                // This effect only from Rapid Killing (mana regen)
                if (!(procSpell->SpellFamilyFlags[1] & 0x01000000))
                    return false;
                triggered_spell_id = 56654;

                target = this;

                switch(dummySpell->Id)
                {
                    case 53228:                             // Rank 1
                        triggered_spell_id = 56654;
                        break;
                    case 53232:                             // Rank 2
                        triggered_spell_id = 58882;
                        break;
                }
                break;
            }
            // Glyph of Mend Pet
            if(dummySpell->Id == 57870)
            {
                pVictim->CastSpell(pVictim, 57894, true, NULL, NULL, GetGUID());
                return true;
            }
            // Misdirection
            if(dummySpell->Id == 34477)
            {
                // does not DoT periodic tick
                if(!(procFlag & PROC_FLAG_DONE_PERIODIC) &&
                // is attacking
                procFlag & (PROC_FLAG_DONE_MELEE_AUTO_ATTACK|
                            PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS|
                            PROC_FLAG_DONE_RANGED_AUTO_ATTACK|
                            PROC_FLAG_DONE_SPELL_RANGED_DMG_CLASS) &&
                // deals damage
                damage > 0)
                {
                    if(Aura *pMisdirectAura = GetAura(34477))
                    {
                        // aura has not procced yet - 0 (infinte) set natively
                        if(pMisdirectAura->GetCharges() == 0)
                        {
                            // set the aura duration up/down to 4 sec
                            pMisdirectAura->SetDuration(4000);

                            // aura has procced
                            pMisdirectAura->SetCharges(1);
                        }
                    }
                }
                // not to remove charges, no spell needed to trigger
                return false;
            }
            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            // Seal of Righteousness
            if (dummySpell->Id == 20154)
            {
                // Do not allow this Seal proc from spells which can trigger this again -> break unwanted recursion
                if (procSpell && (procSpell->Id == 101423 || procSpell->Id == 25742 || procSpell->Id == 20424))
                    return false;

                // Prevent multiple procs (dunno why happend twice)
                if (ToPlayer() && ToPlayer()->HasSpellCooldown(20154))
                    return false;
                else
                {
                    if (ToPlayer())
                        ToPlayer()->AddSpellCooldown(20154, 0, 500);

                    // (Seals of Command) In addition, your Seal of Righteousness now hits all enemy targets within melee range.
                    CastSpell(pVictim, HasAura(85126) ? 101423 : 25742, true);
                }

                return false;
            }

            // Tower of Radiance rank 3 is for some reason listed as "dummy" instead of "trigger spell" like previous ranks
            if (dummySpell->Id == 85512)
            {
                if (!procSpell)
                    return false;

                triggered_spell_id = 88852;
                // only for spells Flash of Light and Divine Light
                if (procSpell->Id != 19750 && procSpell->Id != 82326)
                    return false;
                // and also if target has aura Beacon of Light of this caster
                if (pVictim->GetAura(53563, GetGUID()))
                {
                    target = this;
                    break;
                }

                return false;
            }
            // Light's Beacon - Beacon of Light
            if (dummySpell->Id == 53651)
            {
                // Get target of beacon of light
                if (Unit * beaconTarget = triggeredByAura->GetBase()->GetCaster())
                {
                    // do not proc when target of beacon of light is healed
                    if (beaconTarget == this)
                        return false;
                    // check if it was heal by paladin which casted this beacon of light
                    if (beaconTarget->GetAura(53563, pVictim->GetGUID()))
                    {
                        if (beaconTarget->IsWithinLOSInMap(pVictim))
                        {
                            basepoints0 = damage * triggerAmount / 100;
                            // Holy Light heals for double value
                            if (procSpell->Id == 635)
                                basepoints0 *= 2;

                            triggered_spell_id = 53652;
                            target = beaconTarget;
                            originalCaster = pVictim->GetGUID();
                            break;
                        }
                    }
                }
                return false;
            }
            // Divine Purpose
            if (dummySpell->Id == 86172 || dummySpell->Id == 85117)
            {
                if (procSpell)
                {
                    uint32 id = procSpell->Id;
                    // Judgement, Exorcism, Templar's Verdict, Divine Storm,
                    // Inquisition, Holy Wrath, Hammer of Wrath
                    if (id == 20271 || id == 879 || id == 85256 || id == 53385 ||
                        id == 84963 || id == 2812 || id == 24275)
                    {
                        target = this;
                        triggered_spell_id = 90174;
                        break;
                    }
                }
                return false;
            }
            // Judgements of the Bold
            if (dummySpell->Id == 89901)
            {
                target = this;
                triggered_spell_id = 89906;
            }
            if (dummySpell->Id == 99093) // Paladin T12 Retribution 2P Bonus
            {
                if(procSpell->Id == 35395) // Crusader Strike
                {
                    CastCustomSpell(99092, SPELLVALUE_BASE_POINT0,damage * 0.075, pVictim, true); // Flames of the Faithful
                    break;
                }
                else return (false);
            }
            if (dummySpell->Id == 99074) // Paladin T12 Protection 2P Bonus
            {
                if(procSpell->Id == 53600) // Shield of the Righteous
                {
                    CastCustomSpell(99075, SPELLVALUE_BASE_POINT0,damage * 0.1, pVictim, true); // Righteous Flames
                    break;
                }
                else return (false);
            }
            if (dummySpell->Id == 99070) // Paladin T12 Holy 4P Bonus
            {
                if(procSpell->Id == 82326 || procSpell->Id == 19750 || procSpell->Id == 635 ) // Divine Light, Flash of Light, and Holy Light 
                {
                    CastCustomSpell(54968, SPELLVALUE_BASE_POINT0,damage * 0.10, this, true); // Divine Flame
                    break;
                }
                else return (false);
            }
            // Judgements of the Wise
            if (dummySpell->Id == 31878)
            {
                target = this;
                triggered_spell_id = 31930;
            }
            // Sanctified Wrath
            if (dummySpell->SpellIconID == 3029)
            {
                triggered_spell_id = 57318;
                target = this;
                basepoints0 = triggerAmount;
                CastCustomSpell(target,triggered_spell_id,&basepoints0,&basepoints0,NULL,true,castItem,triggeredByAura);
                return true;
            }
            // Sheath of Light
            if (dummySpell->SpellIconID == 3030)
            {
                // 4 healing tick
                basepoints0 = triggerAmount*damage/400;
                triggered_spell_id = 54203;
                break;
            }
            switch (dummySpell->Id)
            {
                // Variable Pulse Lightning Capacitor (for some reason listed under SPELLFAMILY_PALADIN, u mad Blizzard?)
                case 96887:
                case 97119:
                {
                    // trigger Electrical Charge
                    triggered_spell_id = 96890;

                    Aura* electricAura = GetAura(96890);
                    if (!electricAura)
                        break;

                    int8 stackamount = electricAura->GetStackAmount();

                    int chance = stackamount * 18;
                    if (chance > 100)
                        chance = 100;

                    if (roll_chance_i(chance))
                    {
                        Unit* target = pVictim;
                        if (!target || target->IsFriendlyTo(this))
                        {
                            target = GetVictim();
                            if (!target || target->IsFriendlyTo(this))
                            {
                                Unit::AttackerSet const& atts = getAttackers();
                                Unit::AttackerSet::iterator itr = atts.begin();
                                if (itr != atts.end())
                                    target = (*itr);
                            }
                        }

                        if (target)
                        {
                            int32 bp0 = dummySpell->EffectBasePoints[0] * stackamount;

                            CastCustomSpell(target, 96891, &bp0, NULL, NULL, true);
                            RemoveAurasDueToSpell(96890);
                            return true;
                        }
                    }
                    break;
                }
                // Selfless Healer - handled in Word of Glory code
                case 85804:
                case 85803:
                    return false;
                // Heart of the Crusader
                case 20335: // rank 1
                    triggered_spell_id = 21183;
                    break;
                case 20336: // rank 2
                    triggered_spell_id = 54498;
                    break;
                case 20337: // rank 3
                    triggered_spell_id = 54499;
                    break;
                // Judgement of Light
                case 20185:
                {
                    // 2% of base mana
                    basepoints0 = int32(pVictim->CountPctFromMaxHealth(2));
                    pVictim->CastCustomSpell(pVictim, 20267, &basepoints0, 0, 0, true, 0, triggeredByAura);
                    return true;
                }
                // Judgement of Wisdom
                case 20186:
                {
                    if (pVictim && pVictim->IsAlive() && pVictim->getPowerType() == POWER_MANA)
                    {
                        // 2% of base mana
                        basepoints0 = int32(pVictim->GetCreateMana() * 2 / 100);
                        pVictim->CastCustomSpell(pVictim, 20268, &basepoints0, NULL, NULL, true, 0, triggeredByAura);
                    }
                    return true;
                }
                // Holy Power (Redemption Armor set)
                case 28789:
                {
                    if (!pVictim)
                        return false;

                    // Set class defined buff
                    switch (pVictim->getClass())
                    {
                        case CLASS_PALADIN:
                        case CLASS_PRIEST:
                        case CLASS_SHAMAN:
                        case CLASS_DRUID:
                            triggered_spell_id = 28795;     // Increases the friendly target's mana regeneration by $s1 per 5 sec. for $d.
                            break;
                        case CLASS_MAGE:
                        case CLASS_WARLOCK:
                            triggered_spell_id = 28793;     // Increases the friendly target's spell damage and healing by up to $s1 for $d.
                            break;
                        case CLASS_HUNTER:
                        case CLASS_ROGUE:
                            triggered_spell_id = 28791;     // Increases the friendly target's attack power by $s1 for $d.
                            break;
                        case CLASS_WARRIOR:
                            triggered_spell_id = 28790;     // Increases the friendly target's armor
                            break;
                        default:
                            return false;
                    }
                    break;
                }
                // Seal of Truth
                case 31801:
                {
                    if (effIndex != 0 || pVictim == this)                       // effect 1,2 used by seal unleashing code
                        return false;

                    bool singleTarget = false;
                    if (procFlag & PROC_FLAG_DONE_MELEE_AUTO_ATTACK)
                        singleTarget = true;
                    else if (procSpell)
                    {
                        if (IsSingleTargetSpell(procSpell))
                            singleTarget = true;
                        else
                        {
                            switch (procSpell->Id)
                            {
                                case 853: // Hammer of Justice
                                case 879: // Exorcism
                                case 20271: // Judgement
                                case 24275: // Hammer of Wrath
                                case 35395: // Crusader Strike
                                case 85256: // Templar's Verdict
                                    singleTarget = true;
                                    break;
                            }
                        }
                    }

                    // At melee attack or any single target ability
                    if (singleTarget)
                    {
                        if (pVictim)
                        {
                            // Talent says that we need 5 stacks to proc, but according to video, damage is scaling with number of Censure stacks
                            if (Aura * censure = pVictim->GetAura(31803,GetGUID()))
                            {
                                int32 bp0 = censure->GetStackAmount() * 3;
                                CastCustomSpell(pVictim, 42463, &bp0, 0, 0, true);
                            }
                        }

                        triggered_spell_id = 31803;
                    }
                    break;
                }
                // Seal of Corruption
                case 53736:
                {
                    if (effIndex != 0)                       // effect 1,2 used by seal unleashing code
                        return false;

                    // At melee attack or Hammer of the Righteous spell damage considered as melee attack
                    if ((procFlag & PROC_FLAG_DONE_MELEE_AUTO_ATTACK) || (procSpell && procSpell->Id == 53595))
                        triggered_spell_id = 53742;
                    // On target with 5 stacks of Blood Corruption direct damage is done
                    if (Aura * aur = pVictim->GetAura(triggered_spell_id, GetGUID()))
                    {
                        if (aur->GetStackAmount() == 5)
                        {
                            aur->RefreshDuration();
                            CastSpell(pVictim, 53739, true);
                            return true;
                        }
                    }

                    // Only Autoattack can stack debuff
                    if (procFlag & PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS)
                        return false;
                    break;
                }
                // Spiritual Attunement
                case 31785:
                case 33776:
                {
                    // if healed by another unit (pVictim)
                    if (this == pVictim)
                        return false;

                    // heal amount
                    basepoints0 = triggerAmount*(std::min(damage,GetMaxHealth() - GetHealth()))/100;
                    target = this;

                    if (basepoints0)
                        triggered_spell_id = 31786;
                    break;
                }
                // Paladin Tier 6 Trinket (Ashtongue Talisman of Zeal)
                case 40470:
                {
                    if (!procSpell)
                        return false;

                    float  chance;

                    // Flash of light/Holy light
                    if (procSpell->SpellFamilyFlags[0] & 0xC0000000)
                    {
                        triggered_spell_id = 40471;
                        chance = 15.0f;
                    }
                    // Judgement (any)
                    else if (GetSpellSpecific(procSpell) == SPELL_SPECIFIC_JUDGEMENT)
                    {
                        triggered_spell_id = 40472;
                        chance = 50.0f;
                    }
                    else
                        return false;

                    if (!roll_chance_f(chance))
                        return false;

                    break;
                }
                case 71406: // Tiny Abomination in a Jar
                {
                    if (!pVictim || !pVictim->IsAlive())
                        return false;

                    CastSpell(this, 71432, true, NULL, triggeredByAura);

                    Aura const* dummy = GetAura(71432);
                    if (!dummy || dummy->GetStackAmount() < 8)
                        return false;

                    RemoveAurasDueToSpell(71432);
                    triggered_spell_id = 71433;  // default main hand attack
                    // roll if offhand
                    if (Player const* player = ToPlayer())
                        if (player->GetWeaponForAttack(OFF_ATTACK, true) && urand(0, 1))
                            triggered_spell_id = 71434;
                    target = pVictim;
                    break;
                }
                // Guardian of Ancient Kings (Ancient Crusader)
                case 86701:
                {
                    if (this == pVictim)
                        return false;

                    if (procFlag & (PROC_FLAG_DONE_MELEE_AUTO_ATTACK | PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS))
                    {
                        if(Aura* aura = GetAura(86700))
                        {
                            aura->RefreshDuration();
                            uint8 charges = aura->GetCharges();
                            if(charges < 20)
                            {
                                if(charges < 1)
                                    ++charges;
                                aura->SetCharges(++charges);
                                aura->SetStackAmount(charges);
                            }
                        }
                        else CastSpell(this, 86700, true);
                    }
                    return false;
                    break;
                }
                // Guardian of Ancient Kings (Ancient Healer)
                case 86674:
                {
                    if(!ToPlayer())
                        return false;

                    bool unsummon = false;
                    if(Aura* aura = GetAura(86674))
                    {
                        uint8 charges = aura->GetCharges();
                        if (--charges)
                            aura->SetCharges(charges);
                        else
                        {
                            RemoveAurasDueToSpell(86674);
                            unsummon = true;
                        }
                    }
                    std::list<Unit*> minionList;
                    ToPlayer()->GetAllMinionsByEntry(minionList, 46499);
                    if (!minionList.empty())
                    {
                        for(std::list<Unit*>::const_iterator itr = minionList.begin(); itr != minionList.end(); ++itr)
                        {
                            if(Unit* guardian = *itr)
                            {
                                int32 bp0 = damage;         // Heal the same target for the same amount
                                int32 bp1 = damage / 10;    // Heal nearby targets by 10%
                                guardian->CastCustomSpell(pVictim, 86678, &bp0, &bp1, NULL, true); // Light of the Ancient Kings
                                if(unsummon)
                                    guardian->ToTempSummon()->UnSummon();
                            }
                        }
                    }
                    return false;
                    break;
                }
                // Tyrande's favorite doll - capture mana
                case 92272:
                {
                    if (effIndex != EFFECT_0 || !procSpell || getPowerType() != POWER_MANA)
                        return false;

                    // maximum of mana stored
                    int32 maxmana = 0;
                    if (AuraEffect* maximum = GetAuraEffect(92272, EFFECT_1))
                        maxmana = maximum->GetAmount();

                    // mana to store (% of manacost)
                    int32 addmana = (procSpell->GetManaCostPercentage() * GetCreateMana() / 100) * triggerAmount / 100;
                    addmana = addmana > maxmana ? maxmana : addmana;
                    if (addmana <= 0)
                        return false;

                    // some mana already stored
                    if (AuraEffect* manastored = GetAuraEffect(92596, EFFECT_0))
                    {
                        int32 newmana = manastored->GetAmount() + addmana;
                        newmana = newmana > maxmana ? maxmana : newmana;
                        manastored->ChangeAmount(newmana);
                        // update value at client
                        manastored->GetBase()->SetNeedClientUpdateForTargets();
                    }
                    else // new application needed
                        CastCustomSpell(this, 92596, &addmana, NULL, NULL, true);

                    break;
                }
                case 71545: // Tiny Abomination in a Jar (Heroic)
                {
                    if (!pVictim || !pVictim->IsAlive())
                        return false;

                    CastSpell(this, 71432, true, NULL, triggeredByAura);

                    Aura const* dummy = GetAura(71432);
                    if (!dummy || dummy->GetStackAmount() < 7)
                        return false;

                    RemoveAurasDueToSpell(71432);
                    triggered_spell_id = 71433;  // default main hand attack
                    // roll if offhand
                    if (Player const* player = ToPlayer())
                        if (player->GetWeaponForAttack(OFF_ATTACK, true) && urand(0, 1))
                            triggered_spell_id = 71434;
                    target = pVictim;
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            switch(dummySpell->Id)
            {
                // Earthen Power (Rank 1, 2)
                case 51523:
                case 51524:
                {
                    // Totem itself must be a caster of this spell
                    Unit* caster = NULL;
                    for (ControlList::iterator itr = m_Controlled.begin(); itr != m_Controlled.end(); ++itr) {
                        if ((*itr)->GetEntry() != 2630)
                            continue;

                        caster = (*itr);
                        break;
                    }

                    if (!caster)
                        return false;

                    caster->CastSpell(caster, 59566, true, castItem, triggeredByAura, originalCaster);
                    return true;
                }
                // Tidal Force
                case 55198:
                {
                    // Remove aura stack from  caster
                    RemoveAuraFromStack(55166);
                    // drop charges
                    return false;
                }
                // Totemic Power (The Earthshatterer set)
                case 28823:
                {
                    if (!pVictim)
                        return false;

                    // Set class defined buff
                    switch (pVictim->getClass())
                    {
                        case CLASS_PALADIN:
                        case CLASS_PRIEST:
                        case CLASS_SHAMAN:
                        case CLASS_DRUID:
                            triggered_spell_id = 28824;     // Increases the friendly target's mana regeneration by $s1 per 5 sec. for $d.
                            break;
                        case CLASS_MAGE:
                        case CLASS_WARLOCK:
                            triggered_spell_id = 28825;     // Increases the friendly target's spell damage and healing by up to $s1 for $d.
                            break;
                        case CLASS_HUNTER:
                        case CLASS_ROGUE:
                            triggered_spell_id = 28826;     // Increases the friendly target's attack power by $s1 for $d.
                            break;
                        case CLASS_WARRIOR:
                            triggered_spell_id = 28827;     // Increases the friendly target's armor
                            break;
                        default:
                            return false;
                    }
                    break;
                }
                // Lesser Healing Wave (Totem of Flowing Water Relic)
                case 28849:
                {
                    target = this;
                    triggered_spell_id = 28850;
                    break;
                }
                // Windfury Weapon (Passive) 1-5 Ranks
                case 33757:
                {
                    if (GetTypeId() != TYPEID_PLAYER || !castItem || !castItem->IsEquipped()  || !pVictim || !pVictim->IsAlive())
                        return false;

                    // custom cooldown processing case
                    if (cooldown && ToPlayer()->HasSpellCooldown(dummySpell->Id))
                        return false;

                    if (triggeredByAura->GetBase() && castItem->GetGUID() != triggeredByAura->GetBase()->GetCastItemGUID())
                        return false;

                    WeaponAttackType attType = WeaponAttackType(this->ToPlayer()->GetAttackBySlot(castItem->GetSlot()));
                    if ((attType != BASE_ATTACK && attType != OFF_ATTACK) || !isAttackReady(attType))
                        return false;

                    // Now compute real proc chance...
                    uint32 chance = 20;
                    this->ToPlayer()->ApplySpellMod(dummySpell->Id,SPELLMOD_CHANCE_OF_SUCCESS,chance);

                    if (!roll_chance_i(chance))
                        return false;

                    // Now amount of extra power stored in 1 effect of Enchant spell
                    // Get it by item enchant id
                    uint32 spellId;
                    switch (castItem->GetEnchantmentId(EnchantmentSlot(TEMP_ENCHANTMENT_SLOT)))
                    {
                        case 283: spellId =  8232; break;
                        default:
                        {
                            sLog->outError("Unit::HandleDummyAuraProc: non handled item enchantment (rank?) %u for spell id: %u (Windfury)",
                                castItem->GetEnchantmentId(EnchantmentSlot(TEMP_ENCHANTMENT_SLOT)),dummySpell->Id);
                            return false;
                        }
                    }

                    SpellEntry const* windfurySpellEntry = sSpellStore.LookupEntry(spellId);
                    if (!windfurySpellEntry)
                    {
                        sLog->outError("Unit::HandleDummyAuraProc: non existed spell id: %u (Windfury)",spellId);
                        return false;
                    }

                    int32 extra_attack_power = CalculateSpellDamage(pVictim, windfurySpellEntry, 1);

                    // Value gained from additional AP
                    basepoints0 = int32(extra_attack_power/14.0f * GetAttackTime(BASE_ATTACK)/1000);
                    if (attType == BASE_ATTACK)
                        triggered_spell_id = 25504;
                    else    // off-hand attack
                        triggered_spell_id = 33750;

                    // apply cooldown before cast to prevent processing itself
                    if (cooldown)
                        ToPlayer()->AddSpellCooldown(dummySpell->Id,0,cooldown*1000);

                    // triggering three extra attacks
                    for (uint32 i = 0; i < 3; ++i)
                        CastCustomSpell(pVictim,triggered_spell_id,&basepoints0,NULL,NULL,true,castItem,triggeredByAura);

                    return true;
                }
                // Shaman Tier 6 Trinket
                case 40463:
                {
                    if (!procSpell)
                        return false;

                    float chance;
                    if (procSpell->SpellFamilyFlags[0] & 0x1)
                    {
                        triggered_spell_id = 40465;         // Lightning Bolt
                        chance = 15.0f;
                    }
                    else if (procSpell->SpellFamilyFlags[0] & 0x80)
                    {
                        triggered_spell_id = 40465;         // Lesser Healing Wave
                        chance = 10.0f;
                    }
                    else if (procSpell->SpellFamilyFlags[1] & 0x00000010)
                    {
                        triggered_spell_id = 40466;         // Stormstrike
                        chance = 50.0f;
                    }
                    else
                        return false;

                    if (!roll_chance_f(chance))
                        return false;

                    target = this;
                    break;
                }
                // Glyph of Healing Wave
                case 55440:
                {
                    // Not proc from self heals
                    if (this == pVictim)
                        return false;
                    basepoints0 = triggerAmount * damage / 100;
                    target = this;
                    triggered_spell_id = 55533;
                    break;
                }
                // Spirit Hunt
                case 58877:
                {
                    // Cast on owner
                    target = GetOwner();
                    if (!target)
                        return false;
                    basepoints0 = triggerAmount * damage / 100;
                    triggered_spell_id = 58879;
                    break;
                }
                // Shaman T8 Elemental 4P Bonus
                case 64928:
                {
                    basepoints0 = int32(triggerAmount * damage / 100);
                    triggered_spell_id = 64930;            // Electrified
                    break;
                }
                // Shaman T9 Elemental 4P Bonus
                case 67228:
                {
                    // Lava Burst
                    if (procSpell->SpellFamilyFlags[1] & 0x1000)
                    {
                        triggered_spell_id = 71824;
                        SpellEntry const* triggeredSpell = sSpellStore.LookupEntry(triggered_spell_id);
                        if (!triggeredSpell)
                            return false;
                        basepoints0 = int32(triggerAmount * damage / 100 / (GetSpellMaxDuration(triggeredSpell) / triggeredSpell->EffectAmplitude[0]));
                    }
                    break;
                }
                // Item - Shaman T10 Restoration 4P Bonus
                case 70808:
                {
                    // Chain Heal
                    if((procSpell->SpellFamilyFlags[0] & 0x100) && (procEx & PROC_EX_CRITICAL_HIT))
                    {
                        triggered_spell_id = 70809;
                        SpellEntry const* triggeredSpell = sSpellStore.LookupEntry(triggered_spell_id);
                        if (!triggeredSpell)
                            return false;
                        basepoints0 = int32(triggerAmount * damage / 100 / (GetSpellMaxDuration(triggeredSpell) / triggeredSpell->EffectAmplitude[0]));
                    }
                    break;
                }
                // Item - Shaman T10 Elemental 2P Bonus
                case 70811:
                {
                    // Lightning Bolt & Chain Lightning
                    if (GetTypeId() == TYPEID_PLAYER && (procSpell->SpellFamilyFlags[0] & 0x3))
                    {
                        // reduce the cooldown of Elemental Mastery by 1 second
                        ToPlayer()->ModifySpellCooldown(16116, -1000, true);
                        return true;
                    }
                    return false;
                }
                // Telluric Currents
                case 82984:
                case 82988:
                {
                    // proc only from Lightning Bolt
                    if (procSpell->Id != 403)
                        return false;

                    // amount of mana restored equals 20/40 percent of damage done
                    int32 bp0 = 0;
                    bp0 = triggerAmount * damage / 100;
                    if (bp0)
                        CastCustomSpell(this, 82987, &bp0, 0, 0, true);

                    return false;
                }
                break;
            }
            // Frozen Power
            if (dummySpell->SpellIconID == 3780)
            {
                if (this->GetDistance(target) < 15.0f)
                    return false;
                float chance = (float)triggerAmount;
                if (!roll_chance_f(chance))
                    return false;

                triggered_spell_id = 63685;
                break;
            }
            // Storm, Earth and Fire
            if (dummySpell->SpellIconID == 3063)
            {
                // Earthbind Totem summon only
                if (procSpell->Id != 2484)
                    return false;

                float chance = (float)triggerAmount;
                if (!roll_chance_f(chance))
                    return false;

                triggered_spell_id = 64695;
                break;
            }
            // Ancestral Awakening
            if (dummySpell->SpellIconID == 3065)
            {
                triggered_spell_id = 52759;
                basepoints0 = triggerAmount * damage / 100;
                target = this;
                break;
            }
            // Earth Shield
            if (dummySpell->SpellFamilyFlags[1] & 0x00000400)
            {
                // 3.0.8: Now correctly uses the Shaman's own spell critical strike chance to determine the chance of a critical heal.
                originalCaster = triggeredByAura->GetCasterGUID();
                target = this;
                basepoints0 = triggerAmount;

                // Glyph of Earth Shield
                if (AuraEffect* aur = GetAuraEffect(63279,0))
                {
                    int32 aur_mod = aur->GetAmount();
                    basepoints0 = int32(basepoints0 * (aur_mod + 100.0f) / 100.0f);
                }
                triggered_spell_id = 379;
                break;
            }
            // Flametongue Weapon (Passive)
            if (dummySpell->SpellFamilyFlags[0] & 0x200000)
            {
                if (GetTypeId() != TYPEID_PLAYER  || !pVictim || !pVictim->IsAlive() || !castItem || !castItem->IsEquipped())
                    return false;

                float fire_onhit = (float)(SpellMgr::CalculateSpellEffectAmount(dummySpell, 0) / 100.0);

                float add_spellpower = (float)(SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_FIRE)
                                     + pVictim->SpellBaseDamageBonusTaken(SPELL_SCHOOL_MASK_FIRE));

                // 1.3speed = 5%, 2.6speed = 10%, 4.0 speed = 15%, so, 1.0speed = 3.84%
                add_spellpower= add_spellpower / 100.0f * 3.84f;

                // Enchant on Off-Hand and ready?
                if (castItem->GetSlot() == EQUIPMENT_SLOT_OFFHAND && isAttackReady(OFF_ATTACK))
                {
                    float BaseWeaponSpeed = GetAttackTime(OFF_ATTACK)/1000.0f;

                    // Value1: add the tooltip damage by swingspeed + Value2: add spelldmg by swingspeed
                    basepoints0 = int32((fire_onhit * BaseWeaponSpeed) + (add_spellpower * BaseWeaponSpeed));
                    triggered_spell_id = 10444;
                }

                // Enchant on Main-Hand and ready?
                else if (castItem->GetSlot() == EQUIPMENT_SLOT_MAINHAND && isAttackReady(BASE_ATTACK))
                {
                    float BaseWeaponSpeed = GetAttackTime(BASE_ATTACK)/1000.0f;

                    // Value1: add the tooltip damage by swingspeed +  Value2: add spelldmg by swingspeed
                    basepoints0 = int32((fire_onhit * BaseWeaponSpeed) + (add_spellpower * BaseWeaponSpeed));
                    triggered_spell_id = 10444;
                }

                // If not ready, we should  return, shouldn't we?!
                else
                    return false;

                CastCustomSpell(pVictim,triggered_spell_id,&basepoints0,NULL,NULL,true,castItem,triggeredByAura);
                return true;
            }
            // Resurgence
            if (dummySpell->Id == 16196 || dummySpell->Id == 16180)
            {
                // Water Shield
                if (HasAura(52127) && procSpell)
                {
                    // spell Resurgence, that is responsible for mana gain
                    SpellEntry const* spell = sSpellStore.LookupEntry(101033);
                    if (spell)
                    {
                        int32 amount = SpellMgr::CalculateSpellEffectAmount(spell, EFFECT_0, this);

                        // Healing Wave and Greater Healing Wave - 100% of amount
                        if (procSpell->Id == 331 || procSpell->Id == 77472)
                            amount *= 1.0f;
                        // Healing Surge, Riptide, Unleash Life - 60%
                        else if (procSpell->Id == 8004 || procSpell->Id == 61295 || procSpell->Id == 73685)
                            amount *= 0.6f;
                        // Chain Heal
                        else if (procSpell->Id == 1064)
                            amount *= 0.33f;

                        CastCustomSpell(this, 101033, &amount, 0, 0, &amount, 0, 0, true);
                        return true;
                    }
                }
                return false;
            }
            // Static Shock
            if (dummySpell->SpellIconID == 3059)
            {
                // proc only from Primal Strike, Stormstrike (+offhand) and Lava Lash
                if (procSpell->Id != 73899 && procSpell->Id != 17364 && procSpell->Id != 32176 && procSpell->Id != 60103)
                    return false;

                // Lightning Shield
                if (GetAuraEffect(SPELL_AURA_PROC_TRIGGER_SPELL, SPELLFAMILY_SHAMAN, 0x400, 0, 0) != NULL)
                {
                    // custom cooldown processing case
                    if (GetTypeId() == TYPEID_PLAYER && ToPlayer()->HasSpellCooldown(26364))
                        ToPlayer()->RemoveSpellCooldown(26364);

                    CastSpell(target, 26364, true, castItem, triggeredByAura);
                    return true;
                }
                return false;
            }
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            // Ebon Plaguebringer
            if (dummySpell->Id == 51160 || dummySpell->Id == 51099)
            {
                if (!target || !target->IsAlive() || !procSpell)
                    return false;

                // Proc from Plague Strike, Icy Touch, Chains of Ice and Outbreak
                if (procSpell->Id != 45462 && procSpell->Id != 45477 && procSpell->Id != 45524 && procSpell->Id != 77575)
                    return false;

                basepoints0 = triggeredByAura->GetAmount();
                triggered_spell_id = 65142;
            }
            if (dummySpell->Id == 98996) // Death Knight T12 DPS 4P Bonus
            {
                if(procSpell->Id == 49020 || procSpell->Id == 55090) // Obliterate, Scourge strike
                {
                    CastCustomSpell(99000, SPELLVALUE_BASE_POINT0,damage * 0.07, pVictim, true); // Flaming torrent ( 7 % dmg as fire dmg)
                }
                else return (false);
            }
            // Runic Empowerment
            else if (dummySpell->Id == 81229)
            {
                if (GetTypeId() != TYPEID_PLAYER || getClass() != CLASS_DEATH_KNIGHT)
                    return false;

                // Runic Corruption talent
                // It suppresses the main function of that passive ability and replaces it by rune regen speed bonus
                if (HasAura(51462) || HasAura(51459))
                {
                    // Due to exception in spellsystem we only apply the aura in order to Player::UpdateHaste be called
                    // also use that as "dummy indicator" aura
                    int32 bp = 0;
                    if (Aura* pAura = GetAura(51460))
                    {
                        pAura->SetMaxDuration(pAura->GetDuration() + 3000);
                        pAura->RefreshDuration();
                    }
                    else
                        CastCustomSpell(this, 51460, &bp, 0, 0, true);
                    return true;
                }

                uint8 depletedRunes[3];
                uint8 dsize = 0;
                uint8 runes = ToPlayer()->GetRunesState();

                for (uint32 i = 0; i < MAX_RUNES/2; i++)
                {
                    // if both runes of this kind are depleted
                    if ((!(runes & (1 << (i*2)))) && (!(runes & (1 << ((i*2)+1)))))
                    {
                        // set its bit
                        depletedRunes[dsize++] = i;
                    }
                }

                if (dsize != 0)
                {
                    switch(depletedRunes[urand(1,dsize)-1])
                    {
                        case 0: // blood
                            CastSpell(this, 81166, true);
                            break;
                        case 2: // frost
                            CastSpell(this, 81168, true);
                            break;
                        case 1: // unholy
                            CastSpell(this, 81169, true);
                            break;
                        default:
                            break;
                    }
                    // hack alert !!
                    // Also cast spell originally used for Empower Rune Weapon
                    // It allows us to tell client that one single rune was refreshed
                    CastSpell(this, 89831, true);
                }
                return true;
            }
            // Blood-Caked Strike - Blood-Caked Blade
            if (dummySpell->SpellIconID == 138)
            {
                if (!target || !target->IsAlive())
                    return false;

                triggered_spell_id = dummySpell->EffectTriggerSpell[effIndex];
                break;
            }
            // Improved Blood Presence
            if (dummySpell->SpellIconID == 2636)
            {
                if (GetTypeId() != TYPEID_PLAYER)
                    return false;
                basepoints0 = triggerAmount * damage / 100;
                break;
            }
            // Butchery
            if (dummySpell->SpellIconID == 2664)
            {
                basepoints0 = triggerAmount;
                triggered_spell_id = 50163;
                target = this;
                break;
            }
            // Dancing Rune Weapon
            if (dummySpell->Id == 49028)
            {
                // 1 dummy aura for dismiss rune blade
                if (effIndex != 1)
                    return false;

                Unit* pPet = NULL;
                for (ControlList::const_iterator itr = m_Controlled.begin(); itr != m_Controlled.end(); ++itr) //Find Rune Weapon
                    if ((*itr)->GetEntry() == 27893)
                    {
                        pPet = (*itr);
                        break;
                    }

                if (pPet && pPet->GetVictim() && damage && procSpell)
                {
                    uint32 procDmg = damage / 2;
                    pPet->SendSpellNonMeleeDamageLog(pPet->GetVictim(),procSpell->Id,procDmg,GetSpellSchoolMask(procSpell),0,0,false,0,false);
                    pPet->DealDamage(pPet->GetVictim(),procDmg,NULL,SPELL_DIRECT_DAMAGE,GetSpellSchoolMask(procSpell),procSpell,true);
                    break;
                }
                else
                    return false;
            }
            // Mark of Blood
            if (dummySpell->Id == 49005)
            {
                // TODO: need more info (cooldowns/PPM)
                triggered_spell_id = 61607;
                break;
            }
            // Unholy Blight
            if (dummySpell->Id == 49194)
            {
                basepoints0 = (triggerAmount * damage / 100) / 10;
                triggered_spell_id = 50536;
                basepoints0 += pVictim->GetRemainingPeriodicAmount(GetGUID(), triggered_spell_id, SPELL_AURA_PERIODIC_DAMAGE,EFFECT_0);
                break;
            }
            // Vendetta
            if (dummySpell->SpellFamilyFlags[0] & 0x10000)
            {
                basepoints0 = int32(CountPctFromMaxHealth(triggerAmount));
                triggered_spell_id = 50181;
                target = this;
                break;
            }
            // Necrosis
            if (dummySpell->SpellIconID == 2709)
            {
                basepoints0 = triggerAmount * damage / 100;
                triggered_spell_id = 51460;
                break;
            }
            // Threat of Thassarian
            if (dummySpell->SpellIconID == 2023)
            {
                // Must Dual Wield
                if (!procSpell || !haveOffhandWeapon())
                    return false;
                // Chance as basepoints for dummy aura
                if (!roll_chance_i(triggerAmount))
                    return false;

                switch (procSpell->Id)
                {
                    case 49020: triggered_spell_id = 66198; break; // Obliterate
                    case 49143: triggered_spell_id = 66196; break; // Frost Strike
                    case 45462: triggered_spell_id = 66216; break; // Plague Strike
                    case 49998: triggered_spell_id = 66188; break; // Death Strike
                    case 56815: triggered_spell_id = 66217; break; // Rune Strike
                    case 45902: triggered_spell_id = 66215; break; // Blood Strike
                    default:
                        return false;
                }
                break;
            }
            // Runic Power Back on Snare/Root
            if (dummySpell->Id == 61257)
            {
                // only for spells and hit/crit (trigger start always) and not start from self casted spells
                if (procSpell == 0 || !(procEx & (PROC_EX_NORMAL_HIT|PROC_EX_CRITICAL_HIT)) || this == pVictim)
                    return false;
                // Need snare or root mechanic
                if (!(GetAllSpellMechanicMask(procSpell) & ((1<<MECHANIC_ROOT)|(1<<MECHANIC_SNARE))))
                    return false;
                triggered_spell_id = 61258;
                target = this;
                break;
            }
            // Wandering Plague
            if (dummySpell->SpellIconID == 1614)
            {
                if (!roll_chance_f(GetUnitCriticalChance(BASE_ATTACK, pVictim, dummySpell)))
                    return false;
                basepoints0 = triggerAmount * damage / 100;
                triggered_spell_id = 50526;
                break;
            }
            // Item - Death Knight T10 Melee 4P Bonus
            if (dummySpell->Id == 70656)
            {
                if (!this->ToPlayer())
                    return false;

                for (uint32 i = 0; i < MAX_RUNES; ++i)
                    if (this->ToPlayer()->GetRuneCooldown(i) == 0)
                        return false;
            }
            break;
        }
        case SPELLFAMILY_POTION:
        {
            // alchemist's stone
            if (dummySpell->Id == 17619)
            {
                if (procSpell->SpellFamilyName == SPELLFAMILY_POTION)
                {
                    for (uint8 i=0; i<MAX_SPELL_EFFECTS; i++)
                    {
                        if (procSpell->Effect[i] == SPELL_EFFECT_HEAL)
                        {
                            triggered_spell_id = 21399;
                        }
                        else if (procSpell->Effect[i] == SPELL_EFFECT_ENERGIZE)
                        {
                            triggered_spell_id = 21400;
                        }
                        else
                            continue;

                        basepoints0 = int32(CalculateSpellDamage(this, procSpell,i) * 0.4f);
                        CastCustomSpell(this,triggered_spell_id,&basepoints0,NULL,NULL,true,NULL,triggeredByAura);
                    }
                    return true;
                }
            }
            break;
        }
        case SPELLFAMILY_PET:
        {
            switch (dummySpell->SpellIconID)
            {
                // Guard Dog
                case 201:
                {
                    float addThreat = SpellMgr::CalculateSpellEffectAmount(procSpell, 0, this) * triggerAmount / 100.0f;
                    pVictim->AddThreat(this, addThreat);
                    break;
                }
                // Silverback
                case 1582:
                    triggered_spell_id = dummySpell->Id == 62765 ? 62801 : 62800;
                    target = this;
                    break;
            }
            break;
        }
        default:
            break;
    }

    // Vengeance (shared handler for 4 spells)
    if (dummySpell && (dummySpell->Id == 93098 || dummySpell->Id == 93099 || dummySpell->Id == 84840 || dummySpell->Id == 84839))
    {
        if (pVictim && pVictim->GetTypeId() != TYPEID_PLAYER && !pVictim->IsPet())       // pVictim is attacker actually
        {
            if (Aura* pVengeance = GetAura(76691))
            {
                AuraEffect* pFrst = pVengeance->GetEffect(EFFECT_0);
                AuraEffect* pScnd = pVengeance->GetEffect(EFFECT_1);
                AuraEffect* pThrd = pVengeance->GetEffect(EFFECT_2);
                if (!pFrst || !pScnd || !pThrd)
                    return true;

                int32 bp = damage*0.05f;

                if (pFrst->GetAmount()+bp >= GetMaxHealth()*0.1f)
                    bp = GetMaxHealth()*0.1f - pFrst->GetAmount();

                pFrst->ChangeAmount(pFrst->GetAmount()+bp);
                pScnd->ChangeAmount(pScnd->GetAmount()+bp);

                if (pFrst->GetAmount() > pThrd->GetAmount())
                    pThrd->SetAmount(pFrst->GetAmount());

                pVengeance->SetNeedClientUpdateForTargets();
            }
            else
            {
                int32 bp = damage*0.05f;
                CastCustomSpell(this, 76691, &bp, &bp, &bp, true);
            }

            ToPlayer()->AddSpellCooldown(76691,0,cooldown*1000);
        }

        // we handled everything, leave
        return true;
    }

    // if not handled by custom case, get triggered spell from dummySpell proto
    if (!triggered_spell_id)
        triggered_spell_id = dummySpell->EffectTriggerSpell[triggeredByAura->GetEffIndex()];

    // processed charge only counting case
    if (!triggered_spell_id)
        return true;

    SpellEntry const* triggerEntry = sSpellStore.LookupEntry(triggered_spell_id);
    if (!triggerEntry)
    {
        sLog->outError("Unit::HandleDummyAuraProc: Spell %u have not existed triggered spell %u",dummySpell->Id,triggered_spell_id);
        return false;
    }

    // default case
    if ((!target && !sSpellMgr->IsSrcTargetSpell(triggerEntry)) || (target && target != this && !target->IsAlive()))
        return false;

    if (cooldown_spell_id == 0)
        cooldown_spell_id = triggered_spell_id;

    if (cooldown && GetTypeId() == TYPEID_PLAYER && ToPlayer()->HasSpellCooldown(cooldown_spell_id))
        return false;

    if (basepoints0)
        CastCustomSpell(target,triggered_spell_id,&basepoints0,NULL,NULL,true,castItem,triggeredByAura, originalCaster);
    else
        CastSpell(target,triggered_spell_id,true,castItem,triggeredByAura, originalCaster);

    if (cooldown && GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->AddSpellCooldown(cooldown_spell_id,0,cooldown*1000);

    return true;
}
bool Unit::HandleObsModEnergyAuraProc(Unit *pVictim, uint32 /*damage*/, AuraEffect* triggeredByAura, SpellEntry const * /*procSpell*/, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 cooldown)
{
    SpellEntry const *dummySpell = triggeredByAura->GetSpellProto ();
    //uint32 effIndex = triggeredByAura->GetEffIndex();
    //int32  triggerAmount = triggeredByAura->GetAmount();

    Item* castItem = triggeredByAura->GetBase()->GetCastItemGUID() && GetTypeId() == TYPEID_PLAYER
        ? this->ToPlayer()->GetItemByGuid(triggeredByAura->GetBase()->GetCastItemGUID()) : NULL;

    uint32 triggered_spell_id = 0;
    Unit* target = pVictim;
    int32 basepoints0 = 0;

    switch(dummySpell->SpellFamilyName)
    {
        case SPELLFAMILY_HUNTER:
        {
            // Aspect of the Viper
            if (dummySpell->SpellFamilyFlags[1] & 0x40000)
            {
                uint32 maxmana = GetMaxPower(POWER_MANA);
                basepoints0 = uint32(maxmana* GetAttackTime(RANGED_ATTACK)/1000.0f/100.0f);
                target = this;
                triggered_spell_id = 34075;
                break;
            }
            break;
        }
    }
    // processed charge only counting case
    if (!triggered_spell_id)
        return true;

    SpellEntry const* triggerEntry = sSpellStore.LookupEntry(triggered_spell_id);

    // Try handle unknown trigger spells
    if (!triggerEntry)
    {
        sLog->outError("Unit::HandleObsModEnergyAuraProc: Spell %u have not existed triggered spell %u",dummySpell->Id,triggered_spell_id);
        return false;
    }

    // default case
    if ((!target && !sSpellMgr->IsSrcTargetSpell(triggerEntry)) || (target && target != this && !target->IsAlive()))
        return false;

    if (cooldown && GetTypeId() == TYPEID_PLAYER && this->ToPlayer()->HasSpellCooldown(triggered_spell_id))
        return false;
    if (basepoints0)
        CastCustomSpell(target,triggered_spell_id,&basepoints0,NULL,NULL,true,castItem,triggeredByAura);
    else
        CastSpell(target,triggered_spell_id,true,castItem,triggeredByAura);

    if (cooldown && GetTypeId() == TYPEID_PLAYER)
        this->ToPlayer()->AddSpellCooldown(triggered_spell_id,0,cooldown*1000);
    return true;
}
bool Unit::HandleModDamagePctTakenAuraProc(Unit *pVictim, uint32 /*damage*/, AuraEffect* triggeredByAura, SpellEntry const * /*procSpell*/, uint32 /*procFlag*/, uint32 /*procEx*/, uint32 cooldown)
{
    SpellEntry const *dummySpell = triggeredByAura->GetSpellProto ();
    //uint32 effIndex = triggeredByAura->GetEffIndex();
    //int32  triggerAmount = triggeredByAura->GetAmount();

    Item* castItem = triggeredByAura->GetBase()->GetCastItemGUID() && GetTypeId() == TYPEID_PLAYER
        ? this->ToPlayer()->GetItemByGuid(triggeredByAura->GetBase()->GetCastItemGUID()) : NULL;

    uint32 triggered_spell_id = 0;
    Unit* target = pVictim;
    int32 basepoints0 = 0;

    switch(dummySpell->SpellFamilyName)
    {
        case SPELLFAMILY_PALADIN:
        {
            // Blessing of Sanctuary
            if (dummySpell->SpellFamilyFlags[0] & 0x10000000)
            {
                switch (getPowerType())
                {
                    case POWER_MANA:   triggered_spell_id = 57319; break;
                    default:
                        return false;
                }
            }
            break;
        }
    }
    // processed charge only counting case
    if (!triggered_spell_id)
        return true;

    SpellEntry const* triggerEntry = sSpellStore.LookupEntry(triggered_spell_id);

    if (!triggerEntry)
    {
        sLog->outError("Unit::HandleModDamagePctTakenAuraProc: Spell %u have not existed triggered spell %u",dummySpell->Id,triggered_spell_id);
        return false;
    }

    // default case
    if ((!target && !sSpellMgr->IsSrcTargetSpell(triggerEntry)) || (target && target != this && !target->IsAlive()))
        return false;

    if (cooldown && GetTypeId() == TYPEID_PLAYER && this->ToPlayer()->HasSpellCooldown(triggered_spell_id))
        return false;

    if (basepoints0)
        CastCustomSpell(target,triggered_spell_id,&basepoints0,NULL,NULL,true,castItem,triggeredByAura);
    else
        CastSpell(target,triggered_spell_id,true,castItem,triggeredByAura);

    if (cooldown && GetTypeId() == TYPEID_PLAYER)
        this->ToPlayer()->AddSpellCooldown(triggered_spell_id,0,cooldown*1000);

    return true;
}

// Used in case when access to whole aura is needed
// All procs should be handled like this...
bool Unit::HandleAuraProc(Unit * pVictim, uint32 damage, Aura * triggeredByAura, SpellEntry const * procSpell, uint32 procFlag, uint32 procEx, uint32 /*cooldown*/, bool * handled)
{
    SpellEntry const *dummySpell = triggeredByAura->GetSpellProto();

    switch(dummySpell->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
            switch (dummySpell->Id)
            {
                // Nevermelting Ice Crystal
                case 71564:
                    RemoveAuraFromStack(71564);
                    *handled = true;
                    break;
                case 71756:
                case 72782:
                case 72783:
                case 72784:
                    RemoveAuraFromStack(dummySpell->Id);
                    *handled = true;
                    break;
                // Discerning Eye of the Beast
                case 59915:
                { 
                    CastSpell(this, 59914, true);   // 59914 already has correct basepoints in DBC, no need for custom bp
                    *handled = true;
                    break;
                }
                // Swift Hand of Justice
                case 59906:
                {
                    int32 bp0 = CalculatePctN(GetMaxHealth(), SpellMgr::CalculateSpellEffectAmount(dummySpell, 0));
                    CastCustomSpell(this, 59913, &bp0, NULL, NULL, true);
                    *handled = true;
                    break;
                }
            }
            break;
        case SPELLFAMILY_PALADIN:
        {
            // Judgements of the Just
            if (dummySpell->SpellIconID == 3015)
            {
                *handled = true;
                if (procSpell->Category == SPELLCATEGORY_JUDGEMENT)
                {
                    CastSpell(pVictim, 68055, true);
                    return true;
                }
            }
            // Glyph of Divinity
            else if (dummySpell->Id == 54939)
            {
                *handled = true;
                // Check if we are the target and prevent mana gain
                if (triggeredByAura->GetCasterGUID() == pVictim->GetGUID())
                    return false;
                // Lookup base amount mana restore
                for (uint8 i=0; i<MAX_SPELL_EFFECTS; i++)
                {
                    if (procSpell->Effect[i] == SPELL_EFFECT_ENERGIZE)
                    {
                        // value multiplied by 2 because you should get twice amount
                        int32 mana = SpellMgr::CalculateSpellEffectAmount(procSpell, i) * 2;
                        CastCustomSpell(this, 54986, 0, &mana, NULL, true);
                    }
                }
                return true;
            }
            break;
        }
        case SPELLFAMILY_MAGE:
        {
            switch (dummySpell->Id)
            {
                // Empowered Fire
                case 31656:
                case 31657:
                case 31658:
                {
                    *handled = true;

                    SpellEntry const *spInfo = sSpellStore.LookupEntry(67545);
                    if (!spInfo)
                        return false;

                    int32 bp0 = this->GetCreateMana() * SpellMgr::CalculateSpellEffectAmount(spInfo, 0) / 100;
                    this->CastCustomSpell(this, 67545, &bp0, NULL, NULL, true, NULL, triggeredByAura->GetEffect(0), this->GetGUID());
                    return true;
                }
            }
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            // Reaping
            // Blood Rites
            if (dummySpell->Id == 56835 || dummySpell->Id == 50034)
            {
                *handled = true;
                // Convert recently used runes to Death runes
                if (GetTypeId() == TYPEID_PLAYER)
                {
                    Player *player = ToPlayer();
                    if (player->getClass() != CLASS_DEATH_KNIGHT)
                        return false;

                    AuraEffect * aurEff = triggeredByAura->GetEffect(0);
                    if (!aurEff)
                        return false;

                    // Reset amplitude - set death rune remove timer to 30s
                    aurEff->ResetPeriodic(true);

                    for (uint8 i = 0; i < MAX_RUNES; i++)
                    {
                        if (player->HasLastUsedRune(i))
                            player->AddRuneByAuraEffect(i, RUNE_DEATH, aurEff);
                    }

                    return true;
                }
                return false;
            }

            switch(dummySpell->Id)
            {
                // Hungering Cold aura drop
                case 49203:
                    *handled = true;
                    // Drop only in not disease case
                    if (procSpell && procSpell->Dispel == DISPEL_DISEASE)
                        return false;
                    return true;
                // Cinderglacier
                case 53386:
                    *handled = true;
                    // Drop only in non-periodic case
                    if (procFlag & PROC_FLAG_DONE_PERIODIC)
                        return false;
                    return true;
            }
            break;
        }
        case SPELLFAMILY_ROGUE:
        {
            switch(dummySpell->Id)
            {
                // Find Weakness
                case 51632: // rank #1
                case 91023: // rank #2
                {
                    *handled = true;
                    // Ambush
                    if (!procSpell || !pVictim || procSpell->Id != 8676)
                        return false;

                    if (!(procEx & (PROC_EX_NORMAL_HIT | PROC_EX_CRITICAL_HIT)))
                         return false;

                    // Rank #1 35%, Rank #2 70%
                    int32 bp0 = dummySpell->Id == 51632 ? 35 : 70;
                    // Apply debuff
                    CastCustomSpell(pVictim, 91021, &bp0, NULL, NULL, true);
                    return false;
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            // Do not allow Fire! to be dropped by another spell than Aimed Shot (handled elsewhere)
            if (triggeredByAura && triggeredByAura->GetId() == 82926)
            {
                *handled = true;
                break;
            }
            break;
        }
    }
    return false;
}

void Unit::HandleProcTriggerSpellCopy(Unit *pVictim, uint32 damage, AuraEffect* triggeredByAura, SpellEntry const *procSpell, uint32 procFlags)
{
    if (pVictim == NULL || !pVictim->IsAlive() || !procSpell)
        return;

    if (GetTypeId() != TYPEID_PLAYER)
        return;

    // not allow proc extra attack spell at extra attack
    if (m_extraAttacks && IsSpellHaveEffect(sSpellStore.LookupEntry(procSpell->Id), SPELL_EFFECT_ADD_EXTRA_ATTACKS))
        return;

    if (procFlags & PROC_FLAG_DONE_PERIODIC) // If triggered by dot
    {
        // Cast Wrath of Tarecgosa spell ( arcane damage )
        if (roll_chance_f(8.0f))
            CastCustomSpell(101085, SPELLVALUE_BASE_POINT0, damage, pVictim, true);
    }
    else     // Direct negative spell
    {
        if (roll_chance_f(7.0f)) // Not sure, can't find proof of proc chance after 4.3 nerf
        {
            if (!ToPlayer()->HasSpellCooldown(101056)) 
            {
                // We have to force 1 second CD, because aoe spells can hit many targets => higher chance to proc
                // Proc chance should be derived from single cast of spell not from units hits
                ToPlayer()->AddSpellCooldown(101056, 0, 1000);

                // Special condition for Improved Devouring plague
                Aura * aPlague = pVictim->GetAura(2944);
                AuraEffect const * aurEff = this->GetDummyAuraEffect(SPELLFAMILY_PRIEST, 3790, 0);

                if (procSpell->Id == 63675 && aPlague && aurEff && aPlague->GetEffect(0))
                {
                    uint32 damage = aPlague->GetEffect(0)->GetAmount();
                    damage = this->SpellDamageBonusDone(pVictim, aPlague->GetSpellProto(),EFFECT_0, (uint32)damage, DOT);
                    damage = pVictim->SpellDamageBonusTaken(pVictim, aPlague->GetSpellProto(),EFFECT_0, (uint32)damage, DOT);

                    int32 basepoints0 = aurEff->GetAmount() * aPlague->GetEffect(0)->GetTotalTicks() * int32(damage) / 100;

                    CastCustomSpell(pVictim, 63675, &basepoints0, NULL, NULL, true, NULL, aPlague->GetEffect(0));
                }
                else
                    CastSpell(pVictim, procSpell->Id, true); // Cast copy of that spell
            }
            else
                return;
        }

    }
}

bool Unit::HandleProcTriggerSpell(Unit *pVictim, uint32 damage, AuraEffect* triggeredByAura, SpellEntry const *procSpell, uint32 procFlags, uint32 procEx, uint32 cooldown)
{
    // Get triggered aura spell info
    SpellEntry const* auraSpellInfo = triggeredByAura->GetSpellProto();

    // Basepoints of trigger aura
    int32 triggerAmount = triggeredByAura->GetAmount();

    // Set trigger spell id, target, custom basepoints
    uint32 trigger_spell_id = auraSpellInfo->EffectTriggerSpell[triggeredByAura->GetEffIndex()];

    Unit*  target = NULL;
    int32  basepoints0 = 0;

    if (triggeredByAura->GetAuraType() == SPELL_AURA_PROC_TRIGGER_SPELL_WITH_VALUE)
        basepoints0 = triggerAmount;

    Item* castItem = triggeredByAura->GetBase()->GetCastItemGUID() && GetTypeId() == TYPEID_PLAYER
        ? this->ToPlayer()->GetItemByGuid(triggeredByAura->GetBase()->GetCastItemGUID()) : NULL;

    // Try handle unknown trigger spells
    if (sSpellStore.LookupEntry(trigger_spell_id) == NULL)
    {
        switch (auraSpellInfo->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
                switch (auraSpellInfo->Id)
                {
                    case 23780:             // Aegis of Preservation (Aegis of Preservation trinket)
                        trigger_spell_id = 23781;
                        break;
                    case 33896:             // Desperate Defense (Stonescythe Whelp, Stonescythe Alpha, Stonescythe Ambusher)
                        trigger_spell_id = 33898;
                        break;
                    case 43820:             // Charm of the Witch Doctor (Amani Charm of the Witch Doctor trinket)
                        // Pct value stored in dummy
                        basepoints0 = pVictim->GetCreateHealth() * SpellMgr::CalculateSpellEffectAmount(auraSpellInfo, 1) / 100;
                        target = pVictim;
                        break;
                    case 57345:             // Darkmoon Card: Greatness
                    {
                        float stat = 0.0f;
                        // strength
                        if (GetStat(STAT_STRENGTH) > stat) { trigger_spell_id = 60229;stat = GetStat(STAT_STRENGTH); }
                        // agility
                        if (GetStat(STAT_AGILITY)  > stat) { trigger_spell_id = 60233;stat = GetStat(STAT_AGILITY);  }
                        // intellect
                        if (GetStat(STAT_INTELLECT)> stat) { trigger_spell_id = 60234;stat = GetStat(STAT_INTELLECT);}
                        // spirit
                        if (GetStat(STAT_SPIRIT)   > stat) { trigger_spell_id = 60235;                               }
                        break;
                    }
                    case 103004: // Shadowcloak (Well of Eternity)
                    {
                        // First remove passenger !
                        if (Creature* vehPassenger = GetVehicleCreatureBase())
                        {
                            vehPassenger->Kill(vehPassenger);
                            vehPassenger->ForcedDespawn();
                        }

                        RemoveAura(102994); // Remove Stealth + vehicle kit

                        uint32 stacks = 1;
                        if (Aura * aur = GetAura(103020))
                            stacks = aur->GetStackAmount();

                        RemoveAura(103020); // Remove stacks of dummy aura

                        if (!HasAura(103018) && stacks > 1)
                        if (Aura * ambusher = AddAura(103018,this)) // Scale duration of Shadow Ambusher with number of dummy stacks
                            ambusher->SetDuration(stacks * 1000);

                        break;
                    }
                    case 64568:             // Blood Reserve
                    {
                        if (GetHealth() - damage < GetMaxHealth() * 0.35)
                        {
                            basepoints0 = triggerAmount;
                            trigger_spell_id = 64569;
                            RemoveAura(64568);
                        }
                        break;
                    }
                    case 67702:             // Death's Choice, Item - Coliseum 25 Normal Melee Trinket
                    {
                        float stat = 0.0f;
                        // strength
                        if (GetStat(STAT_STRENGTH) > stat) { trigger_spell_id = 67708;stat = GetStat(STAT_STRENGTH); }
                        // agility
                        if (GetStat(STAT_AGILITY)  > stat) { trigger_spell_id = 67703;                               }
                        break;
                    }
                    case 67771:             // Death's Choice (heroic), Item - Coliseum 25 Heroic Melee Trinket
                    {
                        float stat = 0.0f;
                        // strength
                        if (GetStat(STAT_STRENGTH) > stat) { trigger_spell_id = 67773;stat = GetStat(STAT_STRENGTH); }
                        // agility
                        if (GetStat(STAT_AGILITY)  > stat) { trigger_spell_id = 67772;                               }
                        break;
                    }
                    // Mana Drain Trigger
                    case 27522:
                    case 40336:
                    {
                        // On successful melee or ranged attack gain $29471s1 mana and if possible drain $27526s1 mana from the target.
                        if (this && this->IsAlive())
                            CastSpell(this, 29471, true, castItem, triggeredByAura);
                        if (pVictim && pVictim->IsAlive())
                            CastSpell(pVictim, 27526, true, castItem, triggeredByAura);
                        return true;
                    }
                    // Eye for an Eye
                    case 9799:
                    case 25988:
                    {
                        // return damage % back to attacker
                        basepoints0 = int32((triggerAmount * damage) / 100.0f);
                        trigger_spell_id = 25997;
                        break;
                    }
                }
                break;
            case SPELLFAMILY_MAGE:
                if (auraSpellInfo->SpellIconID == 2127)     // Blazing Speed
                {
                    switch (auraSpellInfo->Id)
                    {
                        case 31641:  // Rank 1
                        case 31642:  // Rank 2
                            trigger_spell_id = 31643;
                            break;
                        default:
                            sLog->outError("Unit::HandleProcTriggerSpell: Spell %u miss posibly Blazing Speed",auraSpellInfo->Id);
                            return false;
                    }
                }
                if(auraSpellInfo->Id == 99061) // Mage T12 2P Bonus
                {
                    CastSpell(this,58836,true); // Initialize image
                    break;
                }
                break;
            case SPELLFAMILY_WARRIOR:
                if (auraSpellInfo->Id == 50421)             // Scent of Blood
                    trigger_spell_id = 50422;
                break;
            case SPELLFAMILY_WARLOCK:
            {
                // Drain Soul
                if (auraSpellInfo->SpellFamilyFlags[0] & 0x4000)
                {
                    // Improved Drain Soul
                    Unit::AuraEffectList const& mAddFlatModifier = GetAuraEffectsByType(SPELL_AURA_DUMMY);
                    for (Unit::AuraEffectList::const_iterator i = mAddFlatModifier.begin(); i != mAddFlatModifier.end(); ++i)
                    {
                        if ((*i)->GetMiscValue() == SPELLMOD_CHANCE_OF_SUCCESS && (*i)->GetSpellProto()->SpellIconID == 113)
                        {
                            int32 value2 = CalculateSpellDamage(this, (*i)->GetSpellProto(),2);
                            basepoints0 = value2 * GetMaxPower(POWER_MANA) / 100;
                            // Drain Soul
                            CastCustomSpell(this, 18371, &basepoints0, NULL, NULL, true, castItem, triggeredByAura);
                            break;
                        }
                    }
                    // Not remove charge (aura removed on death in any cases)
                    // Need for correct work Drain Soul SPELL_AURA_CHANNEL_DEATH_ITEM aura
                    return false;
                }
                break;
            }
            case SPELLFAMILY_PRIEST:
            {
                // Greater Heal Refund
                if (auraSpellInfo->Id == 37594)
                    trigger_spell_id = 37595;
                // Blessed Recovery
                else if (auraSpellInfo->SpellIconID == 1875)
                {
                    switch (auraSpellInfo->Id)
                    {
                        case 27811: trigger_spell_id = 27813; break;
                        case 27815: trigger_spell_id = 27817; break;
                        case 27816: trigger_spell_id = 27818; break;
                        default:
                            sLog->outError("Unit::HandleProcTriggerSpell: Spell %u not handled in BR", auraSpellInfo->Id);
                        return false;
                    }
                    basepoints0 = damage * triggerAmount / 100 / 3;
                    target = this;
                }
                break;
            }
            case SPELLFAMILY_DRUID:
            {
                // Druid Forms Trinket
                if (auraSpellInfo->Id == 37336)
                {
                    switch (GetShapeshiftForm())
                    {
                        case FORM_NONE:     trigger_spell_id = 37344;break;
                        case FORM_CAT:      trigger_spell_id = 37341;break;
                        case FORM_BEAR:
                        case FORM_DIREBEAR: trigger_spell_id = 37340;break;
                        case FORM_TREE:     trigger_spell_id = 37342;break;
                        case FORM_MOONKIN:  trigger_spell_id = 37343;break;
                        default:
                            return false;
                    }
                }
                // Druid T9 Feral Relic (Lacerate, Swipe, Mangle, and Shred)
                else if (auraSpellInfo->Id == 67353)
                {
                    switch (GetShapeshiftForm())
                    {
                        case FORM_CAT:      trigger_spell_id = 67355; break;
                        case FORM_BEAR:
                        case FORM_DIREBEAR: trigger_spell_id = 67354; break;
                        default:
                            return false;
                    }
                }
                break;
            }
            case SPELLFAMILY_HUNTER:
            {
                if (auraSpellInfo->SpellIconID == 3247)     // Piercing Shots 1,2,3
                {
                    trigger_spell_id = 63468;
                    SpellEntry const *TriggerPS = sSpellStore.LookupEntry(trigger_spell_id);
                    if (!TriggerPS)
                        return false;

                    basepoints0 = 0;
                    if (triggeredByAura->GetSpellProto()->Id == 53234) // Rank 1
                        basepoints0 = damage * 0.1f;
                    else if (triggeredByAura->GetSpellProto()->Id == 53237) // Rank 2
                        basepoints0 = damage * 0.2f;
                    else if (triggeredByAura->GetSpellProto()->Id == 53238) // Rank 3
                        basepoints0 = damage * 0.3f;

                    basepoints0 = basepoints0 / (GetSpellMaxDuration(TriggerPS) / 1000);
                    basepoints0 += pVictim->GetRemainingPeriodicAmount(GetGUID(), trigger_spell_id,SPELL_AURA_PERIODIC_DAMAGE,EFFECT_0);
                    break;
                }
                if (auraSpellInfo->SpellIconID == 2225)     // Serpent Spread 1,2
                {
                    if ( !(auraSpellInfo->procFlags == 0x1140) )
                        return false;

                    // Allow proc only from Multi-Shot
                    if (procSpell->Id != 2643)
                        return false;

                    switch (auraSpellInfo->Id)
                    {
                        case 87934:     trigger_spell_id = 88453; break;
                        case 87935:     trigger_spell_id = 88466; break;
                        default:
                            return false;
                    }
                    break;
                }
                if (auraSpellInfo->Id == 82661)       // Aspect of the Fox: Focus bonus
                {
                    if ( !((auraSpellInfo->procFlags & PROC_FLAG_TAKEN_MELEE_AUTO_ATTACK) || (auraSpellInfo->procFlags & PROC_FLAG_TAKEN_SPELL_MELEE_DMG_CLASS)) )
                        return false;
                    target = this;
                    basepoints0 = auraSpellInfo->EffectBasePoints[0];

                    // One with Nature talent  + 1/2/3 focus
                    if (HasAura(82682))
                        basepoints0++;
                    else if (HasAura(82683))
                        basepoints0 += 2;
                    else if (HasAura(82684))
                        basepoints0 += 3;

                    trigger_spell_id = 82716;
                    break;
                }
                break;
            }
            case SPELLFAMILY_PALADIN:
            {
                switch (auraSpellInfo->Id)
                {
                    // Healing Discount
                    case 37705:
                    {
                        trigger_spell_id = 37706;
                        target = this;
                        break;
                    }
                    // Soul Preserver
                    case 60510:
                    {
                        trigger_spell_id = 60515;
                        target = this;
                        break;
                    }
                    // Lightning Capacitor
                    case 37657:
                    {
                        if (!pVictim || !pVictim->IsAlive())
                            return false;
                        // stacking
                        CastSpell(this, 37658, true, NULL, triggeredByAura);

                        Aura * dummy = GetAura(37658);
                        // release at 3 aura in stack (cont contain in basepoint of trigger aura)
                        if (!dummy || dummy->GetStackAmount() < triggerAmount)
                            return false;

                        RemoveAurasDueToSpell(37658);
                        trigger_spell_id = 37661;
                        target = pVictim;
                        break;
                    }
                    // Thunder Capacitor
                    case 54841:
                    {
                        if (!pVictim || !pVictim->IsAlive())
                            return false;
                        // stacking
                        CastSpell(this, 54842, true, NULL, triggeredByAura);

                        // counting
                        Aura * dummy = GetAura(54842);
                        // release at 3 aura in stack (cont contain in basepoint of trigger aura)
                        if (!dummy || dummy->GetStackAmount() < triggerAmount)
                            return false;

                        RemoveAurasDueToSpell(54842);
                        trigger_spell_id = 54843;
                        target = pVictim;
                        break;
                    }
                    //Item - Coliseum 25 Normal Caster Trinket
                    case 67712:
                    {
                        if(!pVictim || !pVictim->IsAlive())
                            return false;
                        // stacking
                        CastSpell(this, 67713, true, NULL, triggeredByAura);

                        Aura * dummy = GetAura(67713);
                        // release at 3 aura in stack (cont contain in basepoint of trigger aura)
                        if(!dummy || dummy->GetStackAmount() < triggerAmount)
                            return false;

                        RemoveAurasDueToSpell(67713);
                        trigger_spell_id = 67714;
                        target = pVictim;
                        break;
                    }
                    //Item - Coliseum 25 Heroic Caster Trinket
                    case 67758:
                    {
                        if(!pVictim || !pVictim->IsAlive())
                            return false;
                        // stacking
                        CastSpell(this, 67759, true, NULL, triggeredByAura);

                        Aura * dummy = GetAura(67759);
                        // release at 3 aura in stack (cont contain in basepoint of trigger aura)
                        if(!dummy || dummy->GetStackAmount() < triggerAmount)
                            return false;

                        RemoveAurasDueToSpell(67759);
                        trigger_spell_id = 67760;
                        target = pVictim;
                        break;
                    }
                    // Blessed Life
                    case 31828:
                    case 31829:
                    {
                        cooldown = 8;
                        break;
                    }
                    default:
                        // Illumination
                        if (auraSpellInfo->SpellIconID == 241)
                        {
                            if (!procSpell)
                                return false;
                            // procspell is triggered spell but we need mana cost of original casted spell
                            uint32 originalSpellId = procSpell->Id;
                            // Holy Shock heal
                            if (procSpell->SpellFamilyFlags[1] & 0x00010000)
                            {
                                switch(procSpell->Id)
                                {
                                    case 25914: originalSpellId = 20473; break;
                                    case 25913: originalSpellId = 20929; break;
                                    case 25903: originalSpellId = 20930; break;
                                    case 27175: originalSpellId = 27174; break;
                                    case 33074: originalSpellId = 33072; break;
                                    case 48820: originalSpellId = 48824; break;
                                    case 48821: originalSpellId = 48825; break;
                                    default:
                                        sLog->outError("Unit::HandleProcTriggerSpell: Spell %u not handled in HShock",procSpell->Id);
                                       return false;
                                }
                            }
                            SpellEntry const *originalSpell = sSpellStore.LookupEntry(originalSpellId);
                            if (!originalSpell)
                            {
                                sLog->outError("Unit::HandleProcTriggerSpell: Spell %u unknown but selected as original in Illu",originalSpellId);
                                return false;
                            }
                            // percent stored in effect 1 (class scripts) base points
                            int32 cost = originalSpell->manaCost + originalSpell->ManaCostPercentage * GetCreateMana() / 100;
                            basepoints0 = cost*SpellMgr::CalculateSpellEffectAmount(auraSpellInfo, 1)/100;
                            trigger_spell_id = 20272;
                            target = this;
                        }
                        break;
                }
                break;
            }
            case SPELLFAMILY_SHAMAN:
            {
                switch (auraSpellInfo->Id)
                {
                    // Lightning Shield (The Ten Storms set)
                    case 23551:
                    {
                        trigger_spell_id = 23552;
                        target = pVictim;
                        break;
                    }
                    // Damage from Lightning Shield (The Ten Storms set)
                    case 23552:
                    {
                        trigger_spell_id = 27635;
                        break;
                    }
                    // Mana Surge (The Earthfury set)
                    case 23572:
                    {
                        if (!procSpell)
                            return false;
                        basepoints0 = procSpell->manaCost * 35 / 100;
                        trigger_spell_id = 23571;
                        target = this;
                        break;
                    }
                    default:
                    {
                        // Lightning Shield (overwrite non existing triggered spell call in spell.dbc
                        if (auraSpellInfo->SpellFamilyFlags[0] & 0x400)
                        {
                            trigger_spell_id = sSpellMgr->GetSpellWithRank(26364, sSpellMgr->GetSpellRank(auraSpellInfo->Id));
                        }
                        // Nature's Guardian
                        else if (auraSpellInfo->SpellIconID == 2013)
                        {
                            // Check health condition - should drop to less 30% (damage deal after this!)
                            if (!HealthBelowPctDamaged(30, damage))
                                return false;

                             if (pVictim && pVictim->IsAlive())
                                 pVictim->getThreatManager().modifyThreatPercent(this,-10);

                            basepoints0 = int32(CountPctFromMaxHealth(triggerAmount));
                            trigger_spell_id = 31616;
                            target = this;
                        }
                    }
                }
                break;
            }
            case SPELLFAMILY_DEATHKNIGHT:
            {
                // Acclimation
                if (auraSpellInfo->SpellIconID == 1930)
                {
                    if (!procSpell)
                        return false;
                    switch(GetFirstSchoolInMask(GetSpellSchoolMask(procSpell)))
                    {
                        case SPELL_SCHOOL_NORMAL:
                            return false;                   // ignore
                        case SPELL_SCHOOL_HOLY:   trigger_spell_id = 50490; break;
                        case SPELL_SCHOOL_FIRE:   trigger_spell_id = 50362; break;
                        case SPELL_SCHOOL_NATURE: trigger_spell_id = 50488; break;
                        case SPELL_SCHOOL_FROST:  trigger_spell_id = 50485; break;
                        case SPELL_SCHOOL_SHADOW: trigger_spell_id = 50489; break;
                        case SPELL_SCHOOL_ARCANE: trigger_spell_id = 50486; break;
                        default:
                            return false;
                    }
                }
                // Blood Presence (Improved)
                else if (auraSpellInfo->Id == 63611)
                {
                    if (GetTypeId() != TYPEID_PLAYER)
                        return false;

                    trigger_spell_id = 50475;
                    basepoints0 = damage * triggerAmount / 100;
                }
                break;
            }
            default:
                 break;
        }
    }

    // All ok. Check current trigger spell
    SpellEntry const* triggerEntry = sSpellStore.LookupEntry(trigger_spell_id);
    if (triggerEntry == NULL)
    {
        // Not cast unknown spell
        // sLog->outError("Unit::HandleProcTriggerSpell: Spell %u have 0 in EffectTriggered[%d], not handled custom case?",auraSpellInfo->Id,triggeredByAura->GetEffIndex());
        return false;
    }

    // not allow proc extra attack spell at extra attack
    if (m_extraAttacks && IsSpellHaveEffect(triggerEntry, SPELL_EFFECT_ADD_EXTRA_ATTACKS))
        return false;

    if (procSpell)
    {
        // Handle some custom things - little hacky, but safe and quick
        switch (procSpell->Id)
        {
            // These spells should not trigger anything - they used to trigger each other, and the triggered spell
            // triggered the original spell, so we were trapped in a loop
            case 20424: // Seals of Command
            case 101423: // Seals of Command aoe
            case 51292: // Soulthirst
            case 82366: // Consecration
                return false;
        }
    }

    // Proc required some HP condition (mostly below PCT) to proc
    // Melee attacks which reduce you below x% health ...
    switch (auraSpellInfo->Id)
    {
        // Leaden Despair
        case 92180:
        case 92185:
        // Spidersilk Spindle
        case 96947:
        case 97130:
        // Symbiotic Worm
        case 92236:
        case 92356:
        // Bedrock Talisman
        case 92234:
            if (GetHealth() - damage < GetMaxHealth() * 0.35)
                break;
            else return false;
    }

    // Custom requirements (not listed in procEx) Warning! damage dealing after this
    // Custom triggered spells
    switch (auraSpellInfo->Id)
    {
        // Persistent Shield (Scarab Brooch trinket)
        // This spell originally trigger 13567 - Dummy Trigger (vs dummy efect)
        case 26467:
        {
            basepoints0 = damage * 15 / 100;
            target = pVictim;
            trigger_spell_id = 26470;
            break;
        }
        case 74001: // Combat Readiness
        {
            if (GetTypeId() == TYPEID_PLAYER)
                ToPlayer()->SetCombatReadinessTimer(10 * IN_MILLISECONDS);
            break;
        }
        case 12298: // Shield Specialization 
        case 12724:
        case 12725:
        {
            if (procEx & PROC_EX_REFLECT)
            {
                uint32 auraId = auraSpellInfo->Id;

                if (auraId == 12298)
                    ModifyPower(POWER_RAGE,20);
                else
                    ModifyPower(POWER_RAGE,(auraId == 12724) ? 40 : 60);

                return false;
            }
        }
        break;
        // Lock and Load
        case 56342:
        case 56343:
        {
            #define LOCK_AND_LOAD_COOLDOWN_MARKER 67544 // 10 s iCD

            if (GetTypeId() != TYPEID_PLAYER || !procSpell || HasAura(LOCK_AND_LOAD_COOLDOWN_MARKER))
                return false;

            if (procFlags & PROC_FLAG_DONE_PERIODIC)
            {
                // T.N.T -> allow to proc from  periodic effects
                AuraEffect * aurEff = GetAuraEffect(SPELL_AURA_DUMMY, SPELLFAMILY_HUNTER, 355, EFFECT_0);

                // Immolation Trap, Explosive Trap or Black Arrow
                if (aurEff && (procSpell->Id == 13797 || procSpell->Id == 13812 || procSpell->Id == 3674) && roll_chance_i(aurEff->GetAmount()))
                    CastSpell(this, LOCK_AND_LOAD_COOLDOWN_MARKER, true);
                else
                    return false;
            }
            else if (procFlags & PROC_FLAG_DONE_TRAP_ACTIVATION) // Only Lock and Load talent
            {
                // This flag with combination with PROC_FLAG_DONE_PERIODIC doesnt work, have no idea why ...
                // This part is hacked in Spell::prepareDataForTriggerSystem
                /*if (roll_chance_i(triggerAmount) && (procSpell->SchoolMask & SPELL_SCHOOL_MASK_FROST))
                    CastSpell(this, LOCK_AND_LOAD_COOLDOWN_MARKER, true);
                else return false;*/
                return false;
            }
            break;
        }
        case 26016: // Vindication
        {
            if (this == pVictim)
                return false;
            break;
        }
        // Unyielding Knights (item exploit 29108\29109)
        case 38164:
        {
            if (!pVictim || pVictim->GetEntry() != 19457)  // Proc only if your target is Grillok
                return false;
            break;
        }
        // Glyph of Backstab (proc only from Backstab)
        case 56800:
        {
            if (procSpell->Id != 53)
                return false;
            break;
        }
        case 99134:// Priest T12 Healer 2P Bonus
        {
            // Proc only from Flash Heal, Heal, Greater Heal, Prayer of Mending proc handled in Spell::EffectApplyAura
            if (procSpell->Id != 2050 && procSpell->Id != 2060 && procSpell->Id != 2061)
                return false;
            break;
        }
        case 88687: // Surge of Light
        case 88690:
        {
            //Proc only from Smite, Heal, Flash Heal, Binding Heal or Greater Heal 
            if (procSpell->Id != 2050 && procSpell->Id != 2060 && procSpell->Id != 2061 && procSpell->Id != 32546 && procSpell->Id != 585)
                return false;
            break;
        }
        case 16880: // Nature's Grace
        case 61345:
        case 61346:
        {
            if (HasAura(93432)) //Nature's Torment Cooldown
                return false;
            // Moonfire ( Sunfire) , Regrowth, or Insect Swarm
            // Insect Swarm proc implemented in spell_druid.cpp
            if (procSpell->Id != 8921 && procSpell->Id != 93402 && procSpell->Id != 8936)
                return false;
            break;
        }
        case 99204: // Shaman T12 Elemental 2P Bonus
        {
            if (procSpell->Id != 403) // Lightning Bolt
                return false;
            break;
        }
        case 53256: // Cobra Strikes
        case 53259:
        case 53260:
        {
            if (procSpell->Id != 3044) // Arcane Shot
                return false;

            this->CastSpell(this,53257,true); // Generate second stack of Cobra strikes buff
            break;
        }
        case 85043: // Grand Crusader
        case 75806:
        {
            // Proc only from  Crusader Strike or Hammer of the Righteous
            if (procSpell->Id != 35395 && procSpell->Id != 53595)
                return false;
            break;
        }
        case 85126: // Seals of Command
        {
            // Only from Seal of Righteousness, Seal of Truth, and Seal of Justice 
            if (HasAura(20154) || HasAura(31801) || HasAura(20164))
                break;
            else
                return false;
        }
        case 20784: // Tamed Pet Passive ( Frenzy only)
        {
            // Proc only from basic attacks abilities
            if (procSpell && !Pet::IsPetBasicAttackSpell(procSpell->Id))
                return false;

            if (Unit * pet = triggeredByAura->GetCaster()) // Hunter's Pet
            {
                if (Unit* pHunter = Unit::GetUnit(*pet, pet->GetOwnerGUID()))
                {
                    int32 amount = 0;
                    if (pHunter->HasAura(19621)) // Frenzy (Rank 1)
                        amount = 2;
                    else if (pHunter->HasAura(19622)) // Frenzy (Rank 2)
                        amount = 4;
                    else if (pHunter->HasAura(19623)) // Frenzy (Rank 3)
                        amount = 6;

                    if (amount)
                        pet->CastCustomSpell(pet, 19615, &amount, 0, 0, true);
                }
            }
            return false;
        }
        // Vigilance
        case 50720:
            if (triggeredByAura)
            {
                // change target
                target = triggeredByAura->GetCaster();

                // trigger Vengeance if present
                if (target && target->HasSpell(93098))
                {
                    if (Aura* pVengeance = target->GetAura(76691))
                    {
                        AuraEffect* pFrst = pVengeance->GetEffect(EFFECT_0);
                        AuraEffect* pScnd = pVengeance->GetEffect(EFFECT_1);
                        AuraEffect* pThrd = pVengeance->GetEffect(EFFECT_2);
                        if (!pFrst || !pScnd || !pThrd)
                            return true;

                        int32 bp = (damage*0.2f)*0.05f;

                        if (pFrst->GetAmount()+bp >= GetMaxHealth()*0.1f)
                            bp = GetMaxHealth()*0.1f - pFrst->GetAmount();

                        pFrst->ChangeAmount(pFrst->GetAmount()+bp);
                        pScnd->ChangeAmount(pScnd->GetAmount()+bp);

                        if (pFrst->GetAmount() > pThrd->GetAmount())
                            pThrd->SetAmount(pFrst->GetAmount());

                        pVengeance->SetNeedClientUpdateForTargets();
                    }
                    else
                    {
                        int32 bp = (damage*0.2f)*0.05f;
                        target->CastCustomSpell(this, 76691, &bp, &bp, &bp, true);
                    }

                    target->ToPlayer()->AddSpellCooldown(76691,0,cooldown*1000);
                }
            }
            break;
        // Entrapment
        case 19387:
        case 19184:
            // Hacked elsewhere
            return false;
            break;
        // Shadow Infusion
        case 49572:
        {
            // Should proc only from Death Coil (dmg and heal)
            if (procSpell->Id != 47632 && procSpell->Id != 47633)
                return false;
            Pet* plPet = ToPlayer() ? ToPlayer()->GetPet() : NULL;
            // and only if player pet doesn't have Dark Transformation on
            if (!plPet || plPet->HasAura(63560))
                return false;
            break;
        }
        // Incite
        case 50685:
        case 50686:
        case 50687:
            // Should proc only on critical hit
            if (!(procEx & PROC_EX_CRITICAL_HIT))
                return false;
            // Should proc only from Heroic Strike
            if (!procSpell || procSpell->Id != 78)
                return false;
            // And also cannot proc from self bonus
            if (HasAura(86627))
                return false;
            break;
        // Hold the Line
        case 84604:
        case 84621:
            if (!(procEx & PROC_EX_PARRY))
                return false;
            break;
        // Protector of the Innocent
        case 20138:
        case 20139:
        case 20140:
            // Cannot proc when healing self
            if (this == pVictim)
                return false;
            break;
        // Deflection
        case 52420:
        {
            if (!HealthBelowPct(35))
                return false;
            break;
        }
        // Battle Trance
        case 12322:
        case 85741:
        case 85742:
        {
            // should proc only from Bloodthirst, Mortal Strike and Shield Slam
            if (procSpell->Id != 23881 && procSpell->Id != 12294 && procSpell->Id != 23922)
                return false;
            break;
        }
        // Brambles
        case 50419:
        {
            if (!roll_chance_i(triggerAmount))
                return false;
            break;
        }
        // Cheat Death
        case 28845:
        {
            // When your health drops below 20%
            if (HealthBelowPctDamaged(20, damage) || HealthBelowPct(20))
                return false;
            break;
        }
        // Deadly Swiftness (Rank 1)
        case 31255:
        {
            // whenever you deal damage to a target who is below 20% health.
            if (!pVictim || !pVictim->IsAlive() || pVictim->HealthAbovePct(20))
                return false;

            target = this;
            trigger_spell_id = 22588;
        }
        // Greater Heal Refund (Avatar Raiment set)
        case 37594:
        {
            if (!pVictim || !pVictim->IsAlive())
                return false;

            // Not give if target already have full health
            if (pVictim->IsFullHealth())
                return false;
            // If your Greater Heal brings the target to full health, you gain $37595s1 mana.
            if (pVictim->GetHealth() + damage < pVictim->GetMaxHealth())
                return false;
            break;
        }
        // Blessed Life
        case 31828:
        case 31829:
        {
            // Check for player origin and for spell cooldown
            if(!ToPlayer() || ToPlayer()->HasSpellCooldown(auraSpellInfo->Id))
                return false;

            // Add internal cooldown (8 seconds hardcoded value)
            ToPlayer()->AddSpellCooldown(auraSpellInfo->Id,0,8000);
            break;
        }
        // Will of the Necropolis
        case 52284:
        case 81163:
        case 81164:
        {
            // Check for player origin, health percentage and for spell cooldown
            if(!HealthBelowPct(30) || !ToPlayer() || ToPlayer()->HasSpellCooldown(auraSpellInfo->Id))
                return false;

            // Add internal cooldown (45 seconds hardcoded value)
            ToPlayer()->AddSpellCooldown(auraSpellInfo->Id,0,45000);

            // Also reset cooldown on Rune Tap if any
            ToPlayer()->RemoveSpellCooldown(48982, true);
            CastSpell(this, 96171, true);
            break;
        }
        // Improved Hamstring
        case 12289:
        case 12668:
        {
            // handled elsewhere
            return false;
        }
        // Bonus Healing (Crystal Spire of Karabor mace)
        case 40971:
        {
            // If your target is below $s1% health
            if (!pVictim || !pVictim->IsAlive() || pVictim->HealthAbovePct(triggerAmount))
                return false;
            break;
        }
        // Evasive Maneuvers (Commendation of Kael`thas trinket)
        case 45057:
        {
            // reduce you below $s1% health
            if (GetHealth() - damage > GetMaxHealth() * triggerAmount / 100)
                return false;
            break;
        }
        // Rapid Recuperation
        case 53228:
        case 53232:
        {
            // This effect only from Rapid Fire (ability cast)
            if (!(procSpell->SpellFamilyFlags[0] & 0x20))
                return false;
            break;
        }
        // Glyph of Silencing Shot
        case 56836:
        {
            // Silencing shot never trigger this -> handled in AuraEffect::HandleAuraModSilence
            return false;
        }
        // Decimation
        case 63156:
        case 63158:
            // Can proc only if target has hp below 25%
            if (!pVictim->HealthBelowPct(26))
                return false;
            break;
        // Deep Freeze Immunity State (hack)
        case 71761:
            return false;
            break;
        // Deathbringer Saurfang - Rune of Blood
        case 72408:
            // can proc only if target is marked with rune
            if (!pVictim->HasAura(72410))
                return false;
            break;
        // Deathbringer Saurfang - Blood Beast's Blood Link
        case 72176:
            basepoints0 = 3;
            break;
        case 15337: // Improved Spirit Tap (Rank 1)
        case 15338: // Improved Spirit Tap (Rank 2)
        {
            if (procSpell->SpellFamilyFlags[0] & 0x800000)
                if ((procSpell->Id != 58381) || !roll_chance_i(50))
                    return false;

            target = pVictim;
            break;
        }
        // Aftermath
        case 85113: // Rank 1
        case 85114: // Rank 2
            // Conflagrate
            if (procSpell->Id == 17962)
            {
                // Should not proc on caster (owner of Aftermath proc aura), only for "sychr py�o"
                if (this == target)
                    return false;
            }
            // Rain of Fire
            else if (procSpell->Id == 42223)
            {
                if ((roll_chance_i(12) && auraSpellInfo->Id == 85114) ||
                    (roll_chance_i(6)  && auraSpellInfo->Id == 85113) )
                    trigger_spell_id = 85387;
                else
                    return false;
            }
            else
                return false;
            break;
        // Executioner
        case 20502:
        case 20503:
            // Should proc only from spell Execute
            if (procSpell->Id != 5308)
                return false;
            break;
        // Lambs to the Slaughter
        case 84583:
        case 84587:
        case 84588:
            // Should proc only from spell Mortal Strike
            if (procSpell->Id != 12294)
                return false;
            break;
        //Impending Victory
        case 80128:
        case 80129:
            // Should proc only from Devastate
            if (procSpell->Id != 20243 || pVictim->GetHealthPct() > 20.0f)
                return false;
            break;
        // Rude Interruption
        case 61216:
        case 61221:
        {
            // Should proc only from Pummel
            if (procSpell->Id != 6552)
                return false;
            break;
        }
        case 56414: // Glyph of Dazing Shield
            // only proc from Avenger's Shield
            if (procSpell->Id != 31935)
                return false;
            break;
        // Soul Leech
        case 30293:     // rank 1
        case 30295:     // rank 2
            // Should proc only from Shadowburn, Soul Fire, Chaos Bolt
            if (procSpell->Id != 17877 && procSpell->Id != 6353 && procSpell->Id != 50796)
                return false;
            // Replenishment "proc"
            CastSpell(this, 57669, true);
            break;
        // Paladin T12 Holy 2P bonus
        case 99067:
            // Should proc only from Holy Shock heals
            if (procSpell->Id != 25914)
                return false;
            break;
        // Priest - Masochism
        case 88994:
        case 88995:
        {
            if (damage < GetMaxHealth()*0.1f)
                return false;
            break;
        }
        // Priest - Harnessed Shadows
        case 33191:
        case 78228:
        {
            // Shadow Orbs should proc from crit only
            if (!(procEx & PROC_EX_CRITICAL_HIT))
                return false;
            break;
        }
        // Glyph of Aimed Shot (crits restore focus)
        case 56824:
        {
            // Should proc from crit only
            if (!(procEx & PROC_EX_CRITICAL_HIT))
                return false;
            // Shoul proc from Aimed Shot and Aimed Shot! (Master Marksman) only
            if (procSpell->Id != 19434 && procSpell->Id != 82928)
                return false;
            break;
        }
        // Die by the Sword
        case 81913:
        case 81914:
        {
            if (GetHealthPct() > 20.0f)
                return false;
            break;
        }
        // Strength of Soul (Discipline Priest)
        case 89488:
        case 89489:
        {
            if (procSpell->SpellIconID == 2818) // Do not proc from Penance
                return false;
            break;
        }
        case 91702: // Mana Feed (Tamed Pet Passive 07 (DND))
        {
            if (!(procEx & PROC_EX_CRITICAL_HIT) || GetTypeId() != TYPEID_UNIT) // Proc only from pet critical basic attacks
                return false;

             // Felstorm + Pursuit + Whiplash are not basic attack spells
            if (procSpell->Id == 89753 || procSpell->Id == 30153 || procSpell->Id == 6360)
                return false;

            if (Player * pWarlock = (Player*)GetOwner()) // Get Warlock
            {
                if (AuraEffect const * aurEff = pWarlock->GetDummyAuraEffect(SPELLFAMILY_WARLOCK, 1982, EFFECT_2)) // Mana feed talent
                {
                    basepoints0 = aurEff->GetAmount();

                    #define ENTRY_FELHUNTER (417)
                    #define ENTRY_FELGUARD   (17252)

                    // Now restores more mana (four times as much) when the warlock is using a Felguard or Felhunter.
                    if (ToCreature()->GetEntry() == ENTRY_FELHUNTER || ToCreature()->GetEntry() == ENTRY_FELGUARD)
                        basepoints0 *= 4;
                }
                else return false;
            }
            else return false;
            break;
        }
        // Sudden Doom
        case 49018:
        case 49529:
        case 49530:
        {
            if(GetTypeId() != TYPEID_PLAYER)
                return false;
            if (!ToPlayer()->GetWeaponForAttack(BASE_ATTACK))
                return false;

            // Select chance based on weapon speed
            float speed = ToPlayer()->GetWeaponForAttack(BASE_ATTACK)->GetProto()->Delay / 1000.0f;

            int32 modifier = 1;

            if(auraSpellInfo->Id == 49530) // Rank 3
                modifier = 4;
            else if(auraSpellInfo->Id == 49529) // Rank 2
                modifier = 3;

            // ToDo: Check this, its based on a wowhead comment
            if(!roll_chance_f(speed * modifier))
                return false;
            break;
        }
        case 44543: // Fingers of Frost
        case 44545:
        case 83074: 
        {
            if(procSpell->Id == 30455) // should not proc from Ice Lance
                return false;
            break;
        }
        case 44561: // Enduring Winter (Rank 1)
        case 86500: // Enduring Winter (Rank 2)
        case 86508: // Enduring Winter (Rank 3)
        {
            if(procSpell->Id == 30455) // should not proc from Ice Lance
                return false;
            break;
        }
        case 85103: //Cremation
        case 85104:
        case 89603:
            if(procSpell->Id != 71521) //shoul proc only from Hand of Gul'dan
                return false;
            break;
        case 24943: // Fury of Stormrage
        case 17104:
            // Proc only from Wrath
            if (procSpell->Id != 5176)
                return false;
            break;
        case 53556: // Enlightened Judgements
        case 53557:
            // Proc only from Judgements
            if (procSpell->Id != 54158 && procSpell->Id != 31804 && procSpell->Id != 20187)
                return false;
            break;
        case 46945: // Safeguard
        case 46949:
            // do not apply to self (second effect of Intervene)
            if (pVictim == this)
                return false;
            break;
        default:
            break;
    }

    // Cataclysm trinket proc internal cooldown
    if (ToPlayer())
    {
        switch (auraSpellInfo->Id)
        {
            // 60 second cooldown
            case 91137: // Tears of Blood
            case 91140: // Tears of Blood (Heroic)
            case 92056: // Gear Detector
            case 90892: // Stonemother's Kiss
            case 92054: // Grace of the Herald, Hearth of the Vile
            case 92088: // Grace of the Herald (Heroic)
            case 92164: // Porcelain Crab, Harrison's Insignia of Panache, Schnottz's Medallion of Command
            case 92175: // Porcelain Crab (Heroic)
            case 90886: // Witching Hourglass
            case 90888: // Witching Hourglass (Heroic)
            case 92070: // Key to the Endless Chamber
            case 92093: // Key to the Endless Chamber (Heroic)
            case 91142: // Rainsong
            case 91144: // Rainsong (Heroic)
            case 90897: // Tendrils of Burrowing Dark
            case 90899: // Tendrils of Burrowing Dark (Heroic)
            case 91353: // Shrine-Cleansing Purifier, Tank-Commander Insignia
            case 95878: // Talisman of Sinister Order
            case 90990: // Anhuur's Hymnal
            case 90993: // Anhuur's Hymnal (Heroic)
            case 91148: // Blood of Isiset
            case 91150: // Blood of Isiset (Heroic)
            case 91361: // Heart of Solace
            case 91365: // Heart of Solace (Heroic)
            case 92097: // Left Eye of Rajh
            case 92095: // Left Eye of Rajh (Heroic)
            case 91369: // Right Eye of Rajh
            case 91366: // Right Eye of Rajh (Heroic)
            case 92209: // Throngus's Finger
            case 92207: // Throngus's Finger (Heroic)
            case 92217: // Gladiator's Insignia of Victory
            case 92219: // Gladiator's Insignia of Dominance
            case 92221: // Gladiator's Insignia of Conquest
            case 85024: // Gladiator's Insignia of Dominance
            case 85011: // Gladiator's Insignia of Conquest
            case 99722: // Gladiator's Insignia of Victory
            case 99720: // Gladiator's Insignia of Dominance
            case 99718: // Gladiator's Insignia of Conquest
            case 99749: // Gladiator's Insignia of Conquest
            case 99743: // Gladiator's Insignia of Dominance
            case 99747: // Gladiator's Insignia of Victory
            case 91011: // Bell of Enraging Resonance
            case 91822: // Crushing Weight
            case 92343: // Crushing Weight (Heroic)
            case 92127: // Essence of the Cyclone
            case 92353: // Essence of the Cyclone (Heroic)
            case 91186: // Fall of Mortality
            case 92333: // Fall of Mortality (Heroic)
            case 91817: // Heart of Rage
            case 92346: // Heart of Rage (Heroic)
            case 91193: // Mandala of Stirring Patterns
            case 92125: // Prestor's Talisman of Machination
            case 92350: // Prestor's Talisman of Machination (Heroic)
            case 91048: // Stump of Time
            case 91025: // Theralion's Mirror
            case 92322: // Theralion's Mirror (Heroic)
            case 92114: // Unheeded Warning
            case 92319: // Bell of Enraging Resonance
            case 96947: // Spidersilk Spindle
            case 97130: // Spidersilk Spindle (Heroic)
            case 96910: // The Hungerer
            case 97126: // The Hungerer (Heroic)
            case 101288:// Coren's Chilled Chromium Coaster
            case 100309:// Dwyer's Caber
            case 101292:// Mithril Stopwatch
            case 101290:// Petrified Pickled Egg
            case 85034: // Vicious Gladiator's Insignia of Victory
                // If has cooldown, do not proc. Little hack maybe, but can proc from other things
                if (ToPlayer()->HasSpellCooldown(auraSpellInfo->Id))
                    return false;

                ToPlayer()->AddSpellCooldown(auraSpellInfo->Id, 0, 60000);
                break;
            //45 second cooldown
            case 96967: // Eye of Blazing Power
            case 97137: // Eye of Blazing Power (Heroic)       
                if (ToPlayer()->HasSpellCooldown(auraSpellInfo->Id))
                    return false;

                ToPlayer()->AddSpellCooldown(auraSpellInfo->Id, 0, 45000);
                break;
            // 30 second cooldown
            case 92180: // Leaden Despair
            case 92185: // Leaden Despair (Heroic)
            case 91080: // Harmlight Totem
            case 92234: // Bedrock Talisman
            case 91833: // Fury of Angerforge
            case 92236: // Symbiotic Worm
            case 92356: // Symbiotic Worm (Heroic)
                if (ToPlayer()->HasSpellCooldown(auraSpellInfo->Id))
                    return false;

                ToPlayer()->AddSpellCooldown(auraSpellInfo->Id, 0, 30000);
                break;
            // 20 second cooldown and hp check
            case 90998: // Sorrowsong
            case 91003: // Sorrowsong (Heroic)
                if (ToPlayer()->HasSpellCooldown(auraSpellInfo->Id) || !pVictim || pVictim->GetHealthPct() > 35.0f)
                    return false;

                ToPlayer()->AddSpellCooldown(auraSpellInfo->Id, 0, 20000);
                break;
            case 99622://Flintlocke's Woodchucker
                {
                    if (ToPlayer()->HasSpellCooldown(auraSpellInfo->Id))
                        return false;
                    ToPlayer()->AddSpellCooldown(auraSpellInfo->Id, 0, 45000);
                    break;
                }
            case 89183: // Darkmoon card: Tsunami
            {
                if (ToPlayer()->HasSpellCooldown(auraSpellInfo->Id))
                    return false;

                if (ToPlayer()->GetAura(89183)->GetStackAmount() == 5)
                    ToPlayer()->AddSpellCooldown(auraSpellInfo->Id, 0, 30000);
                break;
            }
        }
    }

    // Sword Specialization
    if (auraSpellInfo->SpellFamilyName == SPELLFAMILY_GENERIC && auraSpellInfo->SpellIconID == 1462 && procSpell)
    {
        if (Player * plr = ToPlayer())
        {
            if (cooldown && plr->HasSpellCooldown(16459))
                return false;

            // this required for attacks like Mortal Strike
            plr->RemoveSpellCooldown(procSpell->Id);

            CastSpell(pVictim, procSpell->Id, true);

            if (cooldown)
                plr->AddSpellCooldown(16459, 0, cooldown*1000);
            return true;
        }
    }

    // Blade Barrier
    if (auraSpellInfo->SpellFamilyName == SPELLFAMILY_DEATHKNIGHT && auraSpellInfo->SpellIconID == 85)
    {
        Player * plr = this->ToPlayer();
        if (this->GetTypeId() != TYPEID_PLAYER || !plr || plr->getClass() != CLASS_DEATH_KNIGHT)
            return false;

        if (!plr->IsBaseRuneSlotsOnCooldown(RUNE_BLOOD))
            return false;
    }

    // Custom basepoints/target for exist spell
    // dummy basepoints or other customs
    switch(trigger_spell_id)
    {
        // Auras which should proc on area aura source (caster in this case):
        // Turn the Tables
        case 52914:
        case 52915:
        case 52910:
        // Honor Among Thieves
        case 52916:
        {
            target = triggeredByAura->GetBase()->GetCaster();
            if (!target)
                return false;

            if (cooldown && target->GetTypeId() == TYPEID_PLAYER && target->ToPlayer()->HasSpellCooldown(trigger_spell_id))
                return false;

            target->CastSpell(target,trigger_spell_id,true,castItem,triggeredByAura);

            if (cooldown && GetTypeId() == TYPEID_PLAYER)
                this->ToPlayer()->AddSpellCooldown(trigger_spell_id,0,cooldown*1000);
            return true;
        }
        // Cast positive spell on enemy target
        case 7099:  // Curse of Mending
        case 39647: // Curse of Mending
        case 29494: // Temptation
        {
            target = pVictim;
            break;
        }
        // Death's Advance proc
        case 96268:
        {
            if (HasAura(96268))
                return false;

            // can't trigger without talent
            if (!HasAura(96269) && !HasAura(96270))
                return false;

            Player *pl = ToPlayer();
            if (!pl || pl->getClass() != CLASS_DEATH_KNIGHT)
                break;

            for (uint32 i = 0; i < MAX_RUNES; ++i)
            {
                if (pl->GetCurrentRune(i) != RUNE_UNHOLY)
                    continue;
                if (pl->GetRuneCooldown(i) == 0)    // non-depleted unholy rune found - do nothing
                    return false;
            }
            break;
        }
        case 54648: // Focus Magic
        {
            if (triggeredByAura && triggeredByAura->GetCaster() && (procEx & PROC_EX_CRITICAL_HIT))
            {
                target = triggeredByAura->GetCaster();
                target->CastSpell(target, trigger_spell_id, true);
                return true;
            }
            else
                return false;
            break;
        }
        // Revitalize
        case 81094:
        {
            // Disallow proc -> Handled in AuraEffect::PeriodicTick, due to procing bug
            // It seems that if aura has only PROC_FLAG_DONE_PERIODIC it wont proc from SPELL_AURA_PERIODIC_HEAL effects.
            // TODO: Global problem -> Find reason why !!!
            return false;
        }
        // Ready, Steady, Aim...
        case 82925:
        {
            // Only when Steady-shooting
            if (procSpell->Id != 56641)
                return false;
            break;
        }
        // Denounce
        case 85509:
        {
            if (procSpell->Id != 879) // Exorcism only
                return false;
            break;
        }
        // Combo points add triggers (need add combopoint only for main target, and after possible combopoints reset)
        case 15250: // Rogue Setup
        {
            if (!pVictim || pVictim != GetVictim())   // applied only for main target
                return false;
            break;                                   // continue normal case
        }
        // Finish movies that add combo
        case 14189: // Seal Fate (Netherblade set)
        case 14157: // Ruthlessness
        {
            if (!pVictim || pVictim == this)
                return false;
            // Need add combopoint AFTER finish movie (or they dropped in finish phase)
            break;
        }
        // Bloodthirst (($m/100)% of max health)
        case 23880:
        {
            basepoints0 = int32(CountPctFromMaxHealth(triggerAmount) / 1000);
            break;
        }
        // Shamanistic Rage triggered spell
        case 30824:
        {
            basepoints0 = int32(GetTotalAttackPowerValue(BASE_ATTACK) * triggerAmount / 100);
            break;
        }
        // Enlightenment (trigger only from mana cost spells)
        case 35095:
        {
            if (!procSpell || procSpell->powerType != POWER_MANA || (procSpell->manaCost == 0 && procSpell->ManaCostPercentage == 0 && procSpell->manaCostPerlevel == 0))
                return false;
            break;
        }
        // Sword and Board
        case 50227:
        {
            // Remove cooldown on Shield Slam
            if (GetTypeId() == TYPEID_PLAYER)
                this->ToPlayer()->RemoveSpellCategoryCooldown(971, true);
            break;
        }
        // Maelstrom Weapon
        case 53817:
        {
            // Item - Shaman T10 Enhancement 4P Bonus
            if (AuraEffect const* aurEff = GetAuraEffect(70832, 0))
                if (Aura const* maelstrom = GetAura(53817))
                    if ((maelstrom->GetStackAmount() == maelstrom->GetSpellProto()->StackAmount) && roll_chance_i(aurEff->GetAmount()))               
                        CastSpell(this, 70831, true, castItem, triggeredByAura);

            break;
        }
        // Tower of Radiance proc spell
        case 88852:
        {
            // only for spells Flash of Light and Divine Light
            if (procSpell->Id != 19750 && procSpell->Id != 82326)
                return false;
            // and also if target has aura Beacon of Light of this caster
            if (pVictim->GetAura(53563, GetGUID()))
                break;

            return false;
        }
        // Rolling Thunder
        case 88765:
        {
            if (Aura * lightningShield = GetAura(324))
            {
                uint8 lsCharges = lightningShield->GetCharges();
                if(lsCharges < 9)
                {
                    lightningShield->SetCharges(lsCharges + 1);
                    lsCharges++;
                }

                if (lsCharges == 9)
                    this->CastSpell(this,95774,true); // Fulmination marker
            }
            break;
        }
        // Astral Shift
        case 52179:
        {
            if (procSpell == 0 || !(procEx & (PROC_EX_NORMAL_HIT|PROC_EX_CRITICAL_HIT)) || this == pVictim)
                return false;

            // Need stun, fear or silence mechanic
            if (!(GetAllSpellMechanicMask(procSpell) & ((1<<MECHANIC_SILENCE)|(1<<MECHANIC_STUN)|(1<<MECHANIC_FEAR))))
                return false;
            break;
        }
        // Glyph of Death's Embrace
        case 58679:
        {
            // Proc only from healing part of Death Coil. Check is essential as all Death Coil spells have 0x2000 mask in SpellFamilyFlags
            if (!procSpell || !(procSpell->SpellFamilyName == SPELLFAMILY_DEATHKNIGHT && procSpell->SpellFamilyFlags[0] == 0x80002000))
                return false;

            if (!pVictim || !pVictim->IsAlive())
                return false;

            // Glyph of Death's Embrace no longer refunds Runic Power when self-healing via Lichborne.
            if ((HasAura(49039) || HasAura(50397)) && pVictim->ToPlayer())
                return false;
            break;
        }
        // Savage Defense
        case 62606:
        {
            basepoints0 = int32(GetTotalAttackPowerValue(BASE_ATTACK) * triggerAmount / 100.0f);

            Player* pl = ToPlayer();
            // Savage Defender druid's feral mastery proficiency
            if (pl && pl->HasMastery() &&
                pl->GetTalentBranchSpec(pl->GetActiveSpec()) == SPEC_DRUID_FERAL)
            {
                // Each points increases damage absorbed by 4.0%
                basepoints0 *= (1+float(pl->GetMasteryPoints())*4.0f/100.0f);
            }
            break;
        }
        // Efflorescence
        case 81262:
        {
            if (procSpell->Id != 18562) // Only from Swiftmend
                return false;

            int32 bp = 0;

            if (AuraEffect * aurEff = GetAuraEffect(SPELL_AURA_PROC_TRIGGER_SPELL,SPELLFAMILY_DRUID,2886,EFFECT_0))
            {
                bp = (damage * aurEff->GetAmount()) / 100;
                if (pVictim)
                    pVictim->CastCustomSpell(pVictim, 81262, &bp, &bp, &bp, true, 0, 0, GetGUID());
            }
            return false;
        }

        // Body and Soul
        case 64128:
        case 65081:
        {
            // Proc only from PW:S cast
            if (!(procSpell->SpellFamilyFlags[0] & 0x00000001))
                return false;
            break;
        }
        // Culling the Herd
        case 70893:
        {
            // check if we're doing a critical hit
            if (!(procSpell->SpellFamilyFlags[1] & 0x10000000) && (procEx != PROC_EX_CRITICAL_HIT) )
                return false;
            // check if it was procced from pet basic attack
            if (!Pet::IsPetBasicAttackSpell(procSpell->Id))
                return false;
            break;
        }
        // Deathbringer Saurfang - Blood Link
        case 72202:
            target = FindNearestCreature(37813, 75.0f); // NPC_DEATHBRINGER_SAURFANG = 37813
            break;
        // Shadow's Fate (Shadowmourne questline)
        case 71169:
            if (GetTypeId() != TYPEID_PLAYER)
                return false;
            if (ToPlayer()->GetQuestStatus(24547) != QUEST_STATUS_INCOMPLETE)   // A Feast of Souls
                return false;
            if (pVictim->GetTypeId() != TYPEID_UNIT)
                return false;
            // critters are not allowed
            if (pVictim->GetCreatureType() == CREATURE_TYPE_CRITTER)
                return false;
            break;
        // Sacred Shield
        case 96263:
            // Not allow proc when target has under 30% health, or that damage would not reduce him under 30% health
            if (GetHealthPct() < 30.0f || GetMaxHealth()*0.3f < (GetHealth()-damage))
                return false;
            // Also if cooldown isn't ready, dont allow
            if (ToPlayer()->HasSpellCooldown(85285))
                return false;
            // Add custom cooldown (proc cooldown doesn't work)
            ToPlayer()->AddSpellCooldown(85285,0,60000);
            // And set absorb points by formula
            basepoints0 = 1+ToPlayer()->GetUInt32Value(UNIT_FIELD_ATTACK_POWER)*2.5f;
            break;
    }

    if (cooldown && GetTypeId() == TYPEID_PLAYER && ToPlayer()->HasSpellCooldown(trigger_spell_id))
        return false;

    if (target == NULL)
    {
        // If there are no procflags and it is generic spell with damage part -> target should be victim, not caster.
        if (procFlags == PROC_FLAG_NONE && auraSpellInfo->SpellFamilyName == SPELLFAMILY_GENERIC &&
            (auraSpellInfo->HasSpellEffect(SPELL_EFFECT_SCHOOL_DAMAGE) || auraSpellInfo->AppliesAuraType(SPELL_AURA_PERIODIC_DAMAGE)))
            target = pVictim;
    }


    // try detect target manually if not set
    if (target == NULL)
       target = !(procFlags & (PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS | PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_POS)) && IsPositiveSpell(trigger_spell_id) ? this : pVictim;

    // default case
    if ((!target && !sSpellMgr->IsSrcTargetSpell(triggerEntry)) || (target && target != this && !target->IsAlive()))
        return false;

    if (basepoints0)
        CastCustomSpell(target,trigger_spell_id,&basepoints0,NULL,NULL,true,castItem,triggeredByAura);
    else
        CastSpell(target,trigger_spell_id,true,castItem,triggeredByAura);

    if (cooldown && GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->AddSpellCooldown(trigger_spell_id,0,cooldown*1000);

    return true;
}

bool Unit::HandleOverrideClassScriptAuraProc(Unit *pVictim, uint32 /*damage*/, AuraEffect *triggeredByAura, SpellEntry const *procSpell, uint32 cooldown)
{
    int32 scriptId = triggeredByAura->GetMiscValue();

    if (!pVictim || !pVictim->IsAlive())
        return false;

    Item* castItem = triggeredByAura->GetBase()->GetCastItemGUID() && GetTypeId() == TYPEID_PLAYER
        ? this->ToPlayer()->GetItemByGuid(triggeredByAura->GetBase()->GetCastItemGUID()) : NULL;

    uint32 triggered_spell_id = 0;

    switch(scriptId)
    {
        case 836:                                           // Ice Shards  (Rank 1)
        {
            if (!procSpell || procSpell->SpellVisual[0] != 9487)
                return false;
            triggered_spell_id = 12484;
            break;
        }
        case 988:                                           // Ice Shards (Rank 2)
        {
            if (!procSpell || procSpell->SpellVisual[0] != 9487)
                return false;
            triggered_spell_id = 12485;
            break;
        }
        case 989:                                           // Ice Shards (Rank 3)
        {
            if (!procSpell || procSpell->SpellVisual[0] != 9487)
                return false;
            triggered_spell_id = 12486;
            break;
        }
        case 4533:                                          // Dreamwalker Raiment 2 pieces bonus
        {
            // Chance 50%
            if (!roll_chance_i(50))
                return false;

            switch (pVictim->getPowerType())
            {
                case POWER_MANA:   triggered_spell_id = 28722; break;
                case POWER_RAGE:   triggered_spell_id = 28723; break;
                case POWER_ENERGY: triggered_spell_id = 28724; break;
                default:
                    return false;
            }
            break;
        }
        case 4537:                                          // Dreamwalker Raiment 6 pieces bonus
            triggered_spell_id = 28750;                     // Blessing of the Claw
            break;
        case 5497:                                          // Improved Mana Gems
            triggered_spell_id = 37445;                     // Mana Surge
            break;
        default:
            break;
    }

    // not processed
    if (!triggered_spell_id)
        return false;

    // standard non-dummy case
    SpellEntry const* triggerEntry = sSpellStore.LookupEntry(triggered_spell_id);

    if (!triggerEntry)
    {
        sLog->outError("Unit::HandleOverrideClassScriptAuraProc: Spell %u triggering for class script id %u",triggered_spell_id,scriptId);
        return false;
    }

    if (cooldown && GetTypeId() == TYPEID_PLAYER && this->ToPlayer()->HasSpellCooldown(triggered_spell_id))
        return false;

    CastSpell(pVictim, triggered_spell_id, true, castItem, triggeredByAura);

    if (cooldown && GetTypeId() == TYPEID_PLAYER)
        this->ToPlayer()->AddSpellCooldown(triggered_spell_id,0,cooldown*1000);

    return true;
}

void Unit::setPowerType(Powers new_powertype)
{
    SetByteValue(UNIT_FIELD_BYTES_0, 3, new_powertype);

    if (GetTypeId() == TYPEID_PLAYER)
    {
        if (this->ToPlayer()->GetGroup())
            this->ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_POWER_TYPE);
    }
    else if (this->ToCreature()->IsPet())
    {
        Pet *pet = ((Pet*)this);
        if (pet->isControlled())
        {
            Unit *owner = GetOwner();
            if (owner && (owner->GetTypeId() == TYPEID_PLAYER) && owner->ToPlayer()->GetGroup())
                owner->ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_POWER_TYPE);
        }
    }

    switch(new_powertype)
    {
        default:
        case POWER_MANA:
            break;
        case POWER_RAGE:
            SetMaxPower(POWER_RAGE, GetCreatePowers(POWER_RAGE));
            SetPower(POWER_RAGE, 0);
            break;
        case POWER_FOCUS:
            SetMaxPower(POWER_FOCUS, GetCreatePowers(POWER_FOCUS));
            SetPower(POWER_FOCUS, GetCreatePowers(POWER_FOCUS));
            break;
        case POWER_ENERGY:
            SetMaxPower(POWER_ENERGY, GetCreatePowers(POWER_ENERGY));
            break;
        case POWER_HAPPINESS:
            SetMaxPower(POWER_HAPPINESS, GetCreatePowers(POWER_HAPPINESS));
            SetPower(POWER_HAPPINESS, GetCreatePowers(POWER_HAPPINESS));
            break;
    }
}

FactionTemplateEntry const* Unit::getFactionTemplateEntry() const
{
    FactionTemplateEntry const* entry = sFactionTemplateStore.LookupEntry(getFaction());
    if (!entry)
    {
        static uint64 guid = 0;                             // prevent repeating spam same faction problem

        if (GetGUID() != guid)
        {
            if (const Player *player = ToPlayer())
                sLog->outError("Player %s has invalid faction (faction template id) #%u", player->GetName(), getFaction());
            else if (const Creature *creature = ToCreature())
                sLog->outError("Creature (template id: %u) has invalid faction (faction template id) #%u", creature->GetCreatureInfo()->Entry, getFaction());
            else
                sLog->outError("Unit (name=%s, type=%u) has invalid faction (faction template id) #%u", GetName(), uint32(GetTypeId()), getFaction());

            guid = GetGUID();
        }
    }
    return entry;
}

bool Unit::IsHostileTo(Unit const* unit) const
{
    if (!unit)
        return false;
    // always non-hostile to self
    if (unit == this)
        return false;

    // always non-hostile to GM in GM mode
    if (unit->GetTypeId() == TYPEID_PLAYER && ((Player const*)unit)->IsGameMaster())
        return false;

    if (GetTypeId() == TYPEID_PLAYER && unit->GetTypeId() == TYPEID_PLAYER && unit->ToPlayer()->InArena() && ToPlayer()->InArena())
    {
        uint32 cmpSpectId = unit->ToPlayer()->GetSpectatorInstanceId();
        uint32 thisSpectId = ToPlayer()->GetSpectatorInstanceId();

        // arena player friendly to spectator
        if (cmpSpectId > 0 && thisSpectId == 0)
            return false;

        // spectators are friendly too
        if (cmpSpectId > 0 && thisSpectId > 0)
            return false;

        // spectators are friendly for arena players too
        if (cmpSpectId == 0 && thisSpectId > 0)
            return false;
    }

    // always hostile to enemy
    if (GetVictim() == unit || unit->GetVictim() == this)
        return true;

    // test pet/charm masters instead pers/charmeds
    Unit const* testerOwner = GetCharmerOrOwner();
    Unit const* targetOwner = unit->GetCharmerOrOwner();

    // always hostile to owner's enemy
    if (testerOwner && (testerOwner->GetVictim() == unit || unit->GetVictim() == testerOwner))
        return true;

    // always hostile to enemy owner
    if (targetOwner && (GetVictim() == targetOwner || targetOwner->GetVictim() == this))
        return true;

    // always hostile to owner of owner's enemy
    if (testerOwner && targetOwner && (testerOwner->GetVictim() == targetOwner || targetOwner->GetVictim() == testerOwner))
        return true;

    Unit const* tester = testerOwner ? testerOwner : this;
    Unit const* target = targetOwner ? targetOwner : unit;

    // always non-hostile to target with common owner, or to owner/pet
    if (tester == target)
        return false;

    // special cases (Duel, etc)
    if (tester->GetTypeId() == TYPEID_PLAYER && target->GetTypeId() == TYPEID_PLAYER)
    {
        Player const* pTester = (Player const*)tester;
        Player const* pTarget = (Player const*)target;

        // Duel
        if (pTester->duel && pTester->duel->opponent == pTarget && pTester->duel->startTime != 0)
            return true;

        // Group
        if (pTester->GetGroup() && pTester->GetGroup() == pTarget->GetGroup())
            return false;

        // Sanctuary
        if (pTarget->HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_SANCTUARY) && pTester->HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_SANCTUARY))
            return false;

        // PvP FFA state
        if (pTester->HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP) && pTarget->HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP))
            return true;

        //= PvP states
        // Green/Blue (can't attack)
        if (!pTester->HasAuraType(SPELL_AURA_MOD_FACTION) && !pTarget->HasAuraType(SPELL_AURA_MOD_FACTION))
        {
            if (pTester->GetTeam() == pTarget->GetTeam())
                return false;

            // Red (can attack) if true, Blue/Yellow (can't attack) in another case
            return pTester->IsPvP() && pTarget->IsPvP();
        }
    }

    // faction base cases
    FactionTemplateEntry const*tester_faction = tester->getFactionTemplateEntry();
    FactionTemplateEntry const*target_faction = target->getFactionTemplateEntry();
    if (!tester_faction || !target_faction)
        return false;

    if (target->isAttackingPlayer() && tester->IsContestedGuard())
        return true;

    // PvC forced reaction and reputation case
    if (tester->GetTypeId() == TYPEID_PLAYER && !tester->HasAuraType(SPELL_AURA_MOD_FACTION))
    {
        // forced reaction
        if (target_faction->faction)
        {
            if (ReputationRank const* force =tester->ToPlayer()->GetReputationMgr().GetForcedRankIfAny(target_faction))
                return *force <= REP_HOSTILE;

            // if faction have reputation then hostile state for tester at 100% dependent from at_war state
            if (FactionEntry const* raw_target_faction = sFactionStore.LookupEntry(target_faction->faction))
                if (FactionState const* factionState = tester->ToPlayer()->GetReputationMgr().GetState(raw_target_faction))
                    return (factionState->Flags & FACTION_FLAG_AT_WAR);
        }
    }
    // CvP forced reaction and reputation case
    else if (target->GetTypeId() == TYPEID_PLAYER && !target->HasAuraType(SPELL_AURA_MOD_FACTION))
    {
        // forced reaction
        if (tester_faction->faction)
        {
            if (ReputationRank const* force = target->ToPlayer()->GetReputationMgr().GetForcedRankIfAny(tester_faction))
                return *force <= REP_HOSTILE;

            // apply reputation state
            FactionEntry const* raw_tester_faction = sFactionStore.LookupEntry(tester_faction->faction);
            if (raw_tester_faction && raw_tester_faction->reputationListID >=0)
                return ((Player const*)target)->GetReputationMgr().GetRank(raw_tester_faction) <= REP_HOSTILE;
        }
    }

    // common faction based case (CvC,PvC,CvP)
    return tester_faction->IsHostileTo(*target_faction);
}

bool Unit::IsFriendlyTo(Unit const* unit) const
{
    // just for the case...
    if (!unit)
        return true;

    // always friendly to self
    if (unit == this)
        return true;

    // always friendly to GM in GM mode
    if (unit->GetTypeId() == TYPEID_PLAYER && ((Player const*)unit)->IsGameMaster())
        return true;

    if (GetTypeId() == TYPEID_PLAYER && unit->GetTypeId() == TYPEID_PLAYER && unit->ToPlayer()->InArena() && ToPlayer()->InArena())
    {
        uint32 cmpSpectId = unit->ToPlayer()->GetSpectatorInstanceId();
        uint32 thisSpectId = ToPlayer()->GetSpectatorInstanceId();

        // arena player friendly to spectator
        if (cmpSpectId > 0 && thisSpectId == 0)
            return true;

        // spectators are friendly too
        if (cmpSpectId > 0 && thisSpectId > 0)
            return true;

        // spectators are friendly for arena players too
        if (cmpSpectId == 0 && thisSpectId > 0)
            return true;
    }

    // always non-friendly to enemy
    if (GetVictim() == unit || unit->GetVictim() == this)
        return false;

    // test pet/charm masters instead pers/charmeds
    Unit const* testerOwner = GetCharmerOrOwner();
    Unit const* targetOwner = unit->GetCharmerOrOwner();

    // always non-friendly to owner's enemy
    if (testerOwner && (testerOwner->GetVictim() == unit || unit->GetVictim() == testerOwner))
        return false;

    // always non-friendly to enemy owner
    if (targetOwner && (GetVictim() == targetOwner || targetOwner->GetVictim() == this))
        return false;

    // always non-friendly to owner of owner's enemy
    if (testerOwner && targetOwner && (testerOwner->GetVictim() == targetOwner || targetOwner->GetVictim() == testerOwner))
        return false;

    Unit const* tester = testerOwner ? testerOwner : this;
    Unit const* target = targetOwner ? targetOwner : unit;

    // always friendly to target with common owner, or to owner/pet
    if (tester == target)
        return true;

    // special cases (Duel)
    if (tester->GetTypeId() == TYPEID_PLAYER && target->GetTypeId() == TYPEID_PLAYER)
    {
        Player const* pTester = (Player const*)tester;
        Player const* pTarget = (Player const*)target;

        // Duel
        if (pTester->duel && pTester->duel->opponent == target && pTester->duel->startTime != 0)
            return false;

        // Group
        if (pTester->GetGroup() && pTester->GetGroup() == pTarget->GetGroup())
            return true;

        // Sanctuary
        if (pTarget->HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_SANCTUARY) && pTester->HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_SANCTUARY))
            return true;

        // PvP FFA state
        if (pTester->HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP) && pTarget->HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP))
            return false;

        //= PvP states
        // Green/Blue (non-attackable)
        if (!pTester->HasAuraType(SPELL_AURA_MOD_FACTION) && !pTarget->HasAuraType(SPELL_AURA_MOD_FACTION))
        {
            if (pTester->GetTeam() == pTarget->GetTeam())
                return true;

            // Blue (friendly/non-attackable) if not PVP, or Yellow/Red in another case (attackable)
            return !pTarget->IsPvP();
        }
    }

    // faction base cases
    FactionTemplateEntry const *tester_faction = tester->getFactionTemplateEntry();
    FactionTemplateEntry const *target_faction = target->getFactionTemplateEntry();
    if (!tester_faction || !target_faction)
        return false;

    if (target->isAttackingPlayer() && tester->IsContestedGuard())
        return false;

    // PvC forced reaction and reputation case
    if (tester->GetTypeId() == TYPEID_PLAYER && !tester->HasAuraType(SPELL_AURA_MOD_FACTION))
    {
        // forced reaction
        if (target_faction->faction)
        {
            if (ReputationRank const *force =tester->ToPlayer()->GetReputationMgr().GetForcedRankIfAny(target_faction))
                return *force >= REP_FRIENDLY;

            // if faction have reputation then friendly state for tester at 100% dependent from at_war state
            if (FactionEntry const *raw_target_faction = sFactionStore.LookupEntry(target_faction->faction))
                if (FactionState const *factionState = tester->ToPlayer()->GetReputationMgr().GetState(raw_target_faction))
                    return !(factionState->Flags & FACTION_FLAG_AT_WAR);
        }
    }
    // CvP forced reaction and reputation case
    else if (target->GetTypeId() == TYPEID_PLAYER && !target->HasAuraType(SPELL_AURA_MOD_FACTION))
    {
        // forced reaction
        if (tester_faction->faction)
        {
            if (ReputationRank const *force =target->ToPlayer()->GetReputationMgr().GetForcedRankIfAny(tester_faction))
                return *force >= REP_FRIENDLY;

            // apply reputation state
            if (FactionEntry const *raw_tester_faction = sFactionStore.LookupEntry(tester_faction->faction))
                if (raw_tester_faction->reputationListID >= 0)
                    return ((Player const*)target)->GetReputationMgr().GetRank(raw_tester_faction) >= REP_FRIENDLY;
        }
    }

    // common faction based case (CvC,PvC,CvP)
    return tester_faction->IsFriendlyTo(*target_faction);
}

bool Unit::IsHostileToPlayers() const
{
    FactionTemplateEntry const *my_faction = getFactionTemplateEntry();
    if (!my_faction || !my_faction->faction)
        return false;

    FactionEntry const *raw_faction = sFactionStore.LookupEntry(my_faction->faction);
    if (raw_faction && raw_faction->reputationListID >= 0)
        return false;

    return my_faction->IsHostileToPlayers();
}

bool Unit::IsNeutralToAll() const
{
    FactionTemplateEntry const *my_faction = getFactionTemplateEntry();
    if (!my_faction || !my_faction->faction)
        return true;

    FactionEntry const *raw_faction = sFactionStore.LookupEntry(my_faction->faction);
    if (raw_faction && raw_faction->reputationListID >= 0)
        return false;

    return my_faction->IsNeutralToAll();
}

bool Unit::Attack(Unit *victim, bool meleeAttack)
{
    if (!victim || victim == this)
        return false;

    // dead units can neither attack nor be attacked
    if (!IsAlive() || !victim->IsInWorld() || !victim->IsAlive())
        return false;

    // player cannot attack in mount state
    if (GetTypeId() == TYPEID_PLAYER && IsMounted())
        return false;

    // nobody can attack GM in GM-mode
    if (victim->GetTypeId() == TYPEID_PLAYER)
    {
        if (victim->ToPlayer()->IsGameMaster())
            return false;
    }
    else
    {
        if (victim->ToCreature()->IsInEvadeMode())
            return false;
    }

    // remove SPELL_AURA_MOD_UNATTACKABLE at attack (in case non-interruptible spells stun aura applied also that not let attack)
    if (HasAuraType(SPELL_AURA_MOD_UNATTACKABLE))
        RemoveAurasByType(SPELL_AURA_MOD_UNATTACKABLE);

    if (m_attacking)
    {
        if (m_attacking == victim)
        {
            // switch to melee attack from ranged/magic
            if (meleeAttack)
            {
                if (!HasUnitState(UNIT_STATE_MELEE_ATTACKING))
                {
                    AddUnitState(UNIT_STATE_MELEE_ATTACKING);
                    SendMeleeAttackStart(victim);
                    return true;
                }
            }
            else if (HasUnitState(UNIT_STATE_MELEE_ATTACKING))
            {
                ClearUnitState(UNIT_STATE_MELEE_ATTACKING);
                SendMeleeAttackStop(victim);
                return true;
            }
            return false;
        }

        //switch target
        InterruptSpell(CURRENT_MELEE_SPELL);
        if (!meleeAttack)
            ClearUnitState(UNIT_STATE_MELEE_ATTACKING);
    }

    if (m_attacking)
        m_attacking->_removeAttacker(this);

    m_attacking = victim;
    m_attacking->_addAttacker(this);

    // Set our target
    SetUInt64Value(UNIT_FIELD_TARGET, victim->GetGUID());

    if (meleeAttack)
        AddUnitState(UNIT_STATE_MELEE_ATTACKING);

    // set position before any AI calls/assistance
    //if (GetTypeId() == TYPEID_UNIT)
    //    this->ToCreature()->SetCombatStartPosition(GetPositionX(), GetPositionY(), GetPositionZ());

    if (GetTypeId() == TYPEID_UNIT && !this->ToCreature()->IsPet())
    {
        // should not let player enter combat by right clicking target
        SetInCombatWith(victim);
        if (victim->GetTypeId() == TYPEID_PLAYER)
            victim->SetInCombatWith(this);
        AddThreat(victim, 0.0f);

        this->ToCreature()->SendAIReaction(AI_REACTION_HOSTILE);
        this->ToCreature()->CallAssistance();
    }

    if (GetTypeId() == TYPEID_PLAYER && ToPlayer()->GetEmoteState() != 0)
        ToPlayer()->SetEmoteState(0);

    // delay offhand weapon attack to next attack time
    if (haveOffhandWeapon())
        resetAttackTimer(OFF_ATTACK);

    if (meleeAttack)
        SendMeleeAttackStart(victim);

    return true;
}

bool Unit::AttackStop()
{
    if (!m_attacking)
        return false;

    Unit* victim = m_attacking;

    m_attacking->_removeAttacker(this);
    m_attacking = NULL;

    // Clear our target
    SetUInt64Value(UNIT_FIELD_TARGET, 0);

    ClearUnitState(UNIT_STATE_MELEE_ATTACKING);

    InterruptSpell(CURRENT_MELEE_SPELL);

    // reset only at real combat stop
    if (GetTypeId() == TYPEID_UNIT)
    {
        this->ToCreature()->SetNoCallAssistance(false);

        if (this->ToCreature()->HasSearchedAssistance())
        {
            this->ToCreature()->SetNoSearchAssistance(false);
            UpdateSpeed(MOVE_RUN, false);
        }
    }

    SendMeleeAttackStop(victim);

    if (GetTypeId() == TYPEID_PLAYER && ToPlayer()->GetEmoteState() != 0)
        ToPlayer()->SetEmoteState(0);

    return true;
}

void Unit::CombatStop(bool includingCast)
{
    if (includingCast && IsNonMeleeSpellCasted(false))
        InterruptNonMeleeSpells(false);

    AttackStop();
    RemoveAllAttackers();
    if (GetTypeId() == TYPEID_PLAYER)
        this->ToPlayer()->SendAttackSwingCancelAttack();     // melee and ranged forced attack cancel
    ClearInCombat();
}

void Unit::CombatStopWithPets(bool includingCast)
{
    CombatStop(includingCast);

    for (ControlList::const_iterator itr = m_Controlled.begin(); itr != m_Controlled.end(); ++itr)
        (*itr)->CombatStop(includingCast);
}

bool Unit::isAttackingPlayer() const
{
    if (HasUnitState(UNIT_STATE_ATTACK_PLAYER))
        return true;

    for (ControlList::const_iterator itr = m_Controlled.begin(); itr != m_Controlled.end(); ++itr)
        if ((*itr)->isAttackingPlayer())
            return true;

    for (uint8 i = 0; i < MAX_SUMMON_SLOT; ++i)
        if (m_SummonSlot[i])
            if (Creature *summon = GetMap()->GetCreature(m_SummonSlot[i]))
                if (summon->isAttackingPlayer())
                    return true;

    return false;
}

void Unit::RemoveAllAttackers()
{
    while (!m_attackers.empty())
    {
        AttackerSet::iterator iter = m_attackers.begin();
        if (!(*iter)->AttackStop())
        {
            sLog->outError("WORLD: Unit has an attacker that isn't attacking it!");
            m_attackers.erase(iter);
        }
    }
}

void Unit::ModifyAuraState(AuraState flag, bool apply)
{
    if (apply)
    {
        if (!HasFlag(UNIT_FIELD_AURASTATE, 1<<(flag-1)))
        {
            SetFlag(UNIT_FIELD_AURASTATE, 1<<(flag-1));
            if (GetTypeId() == TYPEID_PLAYER)
            {
                PlayerSpellMap const& sp_list = this->ToPlayer()->GetSpellMap();
                for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
                {
                    if (itr->second->state == PLAYERSPELL_REMOVED || itr->second->disabled) continue;
                    SpellEntry const *spellInfo = sSpellStore.LookupEntry(itr->first);
                    if (!spellInfo || !IsPassiveSpell(itr->first)) continue;
                    if (spellInfo->CasterAuraState == uint32(flag))
                        CastSpell(this, itr->first, true, NULL);
                }
            }
            else if (this->ToCreature()->IsPet())
            {
                Pet *pet = ((Pet*)this);
                for (PetSpellMap::const_iterator itr = pet->m_spells.begin(); itr != pet->m_spells.end(); ++itr)
                {
                    if (itr->second.state == PETSPELL_REMOVED) continue;
                    SpellEntry const *spellInfo = sSpellStore.LookupEntry(itr->first);
                    if (!spellInfo || !IsPassiveSpell(itr->first)) continue;
                    if (spellInfo->CasterAuraState == uint32(flag))
                        CastSpell(this, itr->first, true, NULL);
                }
            }
        }
    }
    else
    {
        if (HasFlag(UNIT_FIELD_AURASTATE,1<<(flag-1)))
        {
            RemoveFlag(UNIT_FIELD_AURASTATE, 1<<(flag-1));

            if (flag != AURA_STATE_ENRAGE)                  // enrage aura state triggering continues auras
            {
                Unit::AuraApplicationMap& tAuras = GetAppliedAuras();
                for (Unit::AuraApplicationMap::iterator itr = tAuras.begin(); itr != tAuras.end();)
                {
                    SpellEntry const* spellProto = (*itr).second->GetBase()->GetSpellProto();
                    if (spellProto->CasterAuraState == uint32(flag))
                        RemoveAura(itr);
                    else
                        ++itr;
                }
            }
        }
    }
}

uint32 Unit::BuildAuraStateUpdateForTarget(Unit * target) const
{
    uint32 auraStates = GetUInt32Value(UNIT_FIELD_AURASTATE) &~(PER_CASTER_AURA_STATE_MASK);
    for (AuraStateAurasMap::const_iterator itr = m_auraStateAuras.begin(); itr != m_auraStateAuras.end(); ++itr)
        if ((1<<(itr->first-1)) & PER_CASTER_AURA_STATE_MASK)
            if (itr->second->GetBase()->GetCasterGUID() == target->GetGUID())
                auraStates |= (1<<(itr->first-1));

    return auraStates;
}

bool Unit::HasAuraState(AuraState flag, SpellEntry const *spellProto, Unit const * Caster) const
{
    if (Caster)
    {
        if (spellProto)
        {
            AuraEffectList const& stateAuras = Caster->GetAuraEffectsByType(SPELL_AURA_ABILITY_IGNORE_AURASTATE);
            for (AuraEffectList::const_iterator j = stateAuras.begin(); j != stateAuras.end(); ++j)
                if ((*j)->IsAffectedOnSpell(spellProto))
                    return true;
        }
        // Check per caster aura state
        // If aura with aurastate by caster not found return false
        if ((1<<(flag-1)) & PER_CASTER_AURA_STATE_MASK)
        {
            for (AuraStateAurasMap::const_iterator itr = m_auraStateAuras.lower_bound(flag); itr != m_auraStateAuras.upper_bound(flag); ++itr)
                if (itr->second->GetBase()->GetCasterGUID() == Caster->GetGUID())
                    return true;
            return false;
        }
    }

    return HasFlag(UNIT_FIELD_AURASTATE, 1<<(flag-1));
}

void Unit::SetOwnerGUID(uint64 owner)
{
    if (GetOwnerGUID() == owner)
        return;

    SetUInt64Value(UNIT_FIELD_SUMMONEDBY, owner);
    if (!owner)
        return;

    // Update owner dependent fields
    Player* player = ObjectAccessor::GetPlayer(*this, owner);
    if (!player || !player->HaveAtClient(this)) // if player cannot see this unit yet, he will receive needed data with create object
        return;

    SetFieldNotifyFlag(UF_FLAG_OWNER);

    UpdateData udata(player->GetMapId());
    WorldPacket packet;
    BuildValuesUpdateBlockForPlayer(&udata, player);
    udata.BuildPacket(&packet);
    player->SendDirectMessage(&packet);

    RemoveFieldNotifyFlag(UF_FLAG_OWNER);
}

Unit* Unit::GetOwner() const
{
    if (uint64 ownerid = GetOwnerGUID())
    {
        return ObjectAccessor::GetUnit(*this, ownerid);
    }
    return NULL;
}

Unit *Unit::GetCharmer() const
{
    if (uint64 charmerid = GetCharmerGUID())
        return ObjectAccessor::GetUnit(*this, charmerid);
    return NULL;
}

Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const
{
    uint64 guid = GetCharmerOrOwnerGUID();
    if (IS_PLAYER_GUID(guid))
        return ObjectAccessor::GetPlayer(*this, guid);

    return GetTypeId() == TYPEID_PLAYER ? (Player*)this : NULL;
}

Minion *Unit::GetFirstMinion() const
{
    if (uint64 pet_guid = GetMinionGUID())
    {
        if (Creature* pet = ObjectAccessor::GetCreatureOrPetOrVehicle(*this, pet_guid))
            if (pet->HasUnitTypeMask(UNIT_MASK_MINION))
                return (Minion*)pet;

        sLog->outError("Unit::GetFirstMinion: Minion %u not exist.",GUID_LOPART(pet_guid));
        const_cast<Unit*>(this)->SetMinionGUID(0);
    }

    return NULL;
}

Guardian* Unit::GetGuardianPet() const
{
    if (uint64 pet_guid = GetPetGUID())
    {
        if (Creature* pet = ObjectAccessor::GetCreatureOrPetOrVehicle(*this, pet_guid))
            if (pet->HasUnitTypeMask(UNIT_MASK_GUARDIAN))
                return (Guardian*)pet;

        sLog->outCrash("Unit::GetGuardianPet: Guardian " UI64FMTD " not exist.", pet_guid);
        const_cast<Unit*>(this)->SetPetGUID(0);
    }

    return NULL;
}

Unit* Unit::GetCharm() const
{
    if (uint64 charm_guid = GetCharmGUID())
    {
        if (Unit* pet = ObjectAccessor::GetUnit(*this, charm_guid))
            return pet;

        sLog->outError("Unit::GetCharm: Charmed creature %u not exist.",GUID_LOPART(charm_guid));
        const_cast<Unit*>(this)->SetUInt64Value(UNIT_FIELD_CHARM, 0);
    }

    return NULL;
}

void Unit::SetMinion(Minion *minion, bool apply)
{
    sLog->outDebug("SetMinion %u for %u, apply %u", minion->GetEntry(), GetEntry(), apply);
    
    if (apply)
    {
        if (minion->GetOwnerGUID())
        {
            sLog->outCrash("SetMinion: Minion %u is not the minion of owner %u", minion->GetEntry(), GetEntry());
            return;
        }

        minion->SetOwnerGUID(GetGUID());

        m_Controlled.insert(minion);

        if (GetTypeId() == TYPEID_PLAYER)
        {
            minion->m_ControlledByPlayer = true;
            minion->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
        }

        // Can only have one pet. If a new one is summoned, dismiss the old one.
        if (minion->IsGuardianPet())
        {
            if (Guardian* oldPet = GetGuardianPet())
            {
                if (oldPet != minion && (oldPet->IsPet() || minion->IsPet() || oldPet->GetEntry() != minion->GetEntry()))
                {
                    // remove existing minion pet
                    if (oldPet->IsPet())
                        ((Pet*)oldPet)->Remove();
                    else
                        oldPet->UnSummon();
                    SetPetGUID(minion->GetGUID());
                    SetMinionGUID(0);
                }
            }
            else
            {
                SetPetGUID(minion->GetGUID());
                SetMinionGUID(0);
            }
            
            if (GetTypeId() == TYPEID_PLAYER)
            {
                Player *player = ToPlayer();
                player->m_currentPetSlot = minion->GetSlot();

                if (Pet *pet = minion->ToPet())
                {
                    if (pet->IsHunterPet() && !pet->IsInStable())
                        player->setPetSlotUsed(pet->GetSlot(), true);
                }
            }
        }

        if (minion->HasUnitTypeMask(UNIT_MASK_CONTROLABLE_GUARDIAN))
        {
            if (AddUInt64Value(UNIT_FIELD_SUMMON, minion->GetGUID()))
            {
            }
        }

        if (minion->m_Properties && minion->m_Properties->Type == SUMMON_TYPE_MINIPET)
        {
            SetCritterGUID(minion->GetGUID());
        }

        // PvP, FFAPvP
        minion->SetByteValue(UNIT_FIELD_BYTES_2, 1, GetByteValue(UNIT_FIELD_BYTES_2, 1));

        // FIXME: hack, speed must be set only at follow
        if (GetTypeId() == TYPEID_PLAYER && minion->IsPet())
            for (uint8 i = 0; i < MAX_MOVE_TYPE; ++i)
                minion->SetSpeed(UnitMoveType(i), m_speed_rate[i], true);

        if (GetTypeId() == TYPEID_PLAYER)
        {
            // Send infinity cooldown - client does that automatically but after relog cooldown needs to be set again
            SpellEntry const *spellInfo = sSpellStore.LookupEntry(minion->GetUInt32Value(UNIT_CREATED_BY_SPELL));
            if (spellInfo && (spellInfo->Attributes & SPELL_ATTR0_DISABLED_WHILE_ACTIVE))
                this->ToPlayer()->AddSpellAndCategoryCooldowns(spellInfo, 0, NULL, true);
        }
    }
    else
    {
        if (minion->GetOwnerGUID() != GetGUID())
        {
            sLog->outCrash("SetMinion: Minion %u is not the minion of owner %u", minion->GetEntry(), GetEntry());
            return;
        }

        m_Controlled.erase(minion);

        if (minion->m_Properties && minion->m_Properties->Type == SUMMON_TYPE_MINIPET)
        {
            if (GetCritterGUID() == minion->GetGUID())
                SetCritterGUID(0);
        }

        if (minion->IsGuardianPet())
        {
            if (GetPetGUID() == minion->GetGUID())
                SetPetGUID(0);
        }
        else if (minion->IsTotem())
        {
            // All summoned by totem minions must disappear when it is removed.
        if (const SpellEntry* spInfo = sSpellStore.LookupEntry(minion->ToTotem()->GetSpell()))
            for (int i = 0; i < MAX_SPELL_EFFECTS; ++i)
            {
                if (spInfo->Effect[i] != SPELL_EFFECT_SUMMON)
                    continue;

                this->RemoveAllMinionsByEntry(spInfo->EffectMiscValue[i]);
            }
        }

        if (GetTypeId() == TYPEID_PLAYER)
        {
            SpellEntry const *spellInfo = sSpellStore.LookupEntry(minion->GetUInt32Value(UNIT_CREATED_BY_SPELL));
            // Remove infinity cooldown
            if (spellInfo && (spellInfo->Attributes & SPELL_ATTR0_DISABLED_WHILE_ACTIVE))
                this->ToPlayer()->SendCooldownEvent(spellInfo);
        }

        //if (minion->HasUnitTypeMask(UNIT_MASK_GUARDIAN))
        {
            if (RemoveUInt64Value(UNIT_FIELD_SUMMON, minion->GetGUID()))
            {
                //Check if there is another minion
                for (ControlList::iterator itr = m_Controlled.begin(); itr != m_Controlled.end(); ++itr)
                {
                    // do not use this check, creature do not have charm guid
                    //if (GetCharmGUID() == (*itr)->GetGUID())
                    if (GetGUID() == (*itr)->GetCharmerGUID())
                        continue;

                    //ASSERT((*itr)->GetOwnerGUID() == GetGUID());
                    if ((*itr)->GetOwnerGUID() != GetGUID())
                    {
                        OutDebugInfo();
                        (*itr)->OutDebugInfo();
                        ASSERT(false);
                    }
                    ASSERT((*itr)->GetTypeId() == TYPEID_UNIT);

                    if (!(*itr)->HasUnitTypeMask(UNIT_MASK_CONTROLABLE_GUARDIAN))
                        continue;

                    if (AddUInt64Value(UNIT_FIELD_SUMMON, (*itr)->GetGUID()))
                    {
                        //show another pet bar if there is no charm bar
                        if (GetTypeId() == TYPEID_PLAYER && !GetCharmGUID())
                        {
                            if ((*itr)->IsPet())
                                this->ToPlayer()->PetSpellInitialize();
                            else
                                this->ToPlayer()->CharmSpellInitialize();
                        }
                    }
                    break;
                }
            }
        }
    }
}

void Unit::GetAllMinionsByEntry(std::list<Unit*>& Minions, uint32 entry)
{
    for (Unit::ControlList::iterator itr = m_Controlled.begin(); itr != m_Controlled.end();)
    {
        Unit *unit = *itr; 
        ++itr;
        if (unit->GetEntry() == entry && unit->GetTypeId() == TYPEID_UNIT
            && unit->ToCreature()->IsSummon()) // minion, actually
            Minions.push_back(unit);
    }
}

void Unit::RemoveAllMinionsByEntry(uint32 entry)
{
    for (Unit::ControlList::iterator itr = m_Controlled.begin(); itr != m_Controlled.end();)
    {
        Unit *unit = *itr;
        ++itr;
        if (unit->GetEntry() == entry && unit->GetTypeId() == TYPEID_UNIT
            && unit->ToCreature()->IsSummon()) // minion, actually
            unit->ToTempSummon()->UnSummon();
        // i think this is safe because i have never heard that a despawned minion will trigger a same minion
    }
}

void Unit::SetCharm(Unit* charm, bool apply)
{
    if (apply)
    {
        if (GetTypeId() == TYPEID_PLAYER)
        {
            if (!AddUInt64Value(UNIT_FIELD_CHARM, charm->GetGUID()))
                sLog->outCrash("Player %s is trying to charm unit %u, but it already has a charmed unit " UI64FMTD "", GetName(), charm->GetEntry(), GetCharmGUID());

            charm->m_ControlledByPlayer = true;
            // TODO: maybe we can use this flag to check if controlled by player
            charm->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
        }
        else
            charm->m_ControlledByPlayer = false;

        // PvP, FFAPvP
        charm->SetByteValue(UNIT_FIELD_BYTES_2, 1, GetByteValue(UNIT_FIELD_BYTES_2, 1));

        if (!charm->AddUInt64Value(UNIT_FIELD_CHARMEDBY, GetGUID()))
            sLog->outCrash("Unit %u is being charmed, but it already has a charmer " UI64FMTD "", charm->GetEntry(), charm->GetCharmerGUID());

        if (charm->HasUnitMovementFlag(MOVEMENTFLAG_WALKING))
        {
            charm->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
            charm->SendMovementFlagUpdate();
        }

        m_Controlled.insert(charm);
    }
    else
    {
        if (GetTypeId() == TYPEID_PLAYER)
        {
            if (!RemoveUInt64Value(UNIT_FIELD_CHARM, charm->GetGUID()))
                sLog->outCrash("Player %s is trying to uncharm unit %u, but it has another charmed unit " UI64FMTD "", GetName(), charm->GetEntry(), GetCharmGUID());
        }

        if (!charm->RemoveUInt64Value(UNIT_FIELD_CHARMEDBY, GetGUID()))
            sLog->outCrash("Unit %u is being uncharmed, but it has another charmer " UI64FMTD "", charm->GetEntry(), charm->GetCharmerGUID());

        if (charm->GetTypeId() == TYPEID_PLAYER)
        {
            charm->m_ControlledByPlayer = true;
            charm->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
            charm->ToPlayer()->UpdatePvPState();
        }
        else if (Player *player = charm->GetCharmerOrOwnerPlayerOrPlayerItself())
        {
            charm->m_ControlledByPlayer = true;
            charm->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
            charm->SetByteValue(UNIT_FIELD_BYTES_2, 1, player->GetByteValue(UNIT_FIELD_BYTES_2, 1));
        }
        else
        {
            charm->m_ControlledByPlayer = false;
            charm->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
            charm->SetByteValue(UNIT_FIELD_BYTES_2, 1, 0);
        }

        if (charm->GetTypeId() == TYPEID_PLAYER
            || !charm->ToCreature()->HasUnitTypeMask(UNIT_MASK_MINION)
            || charm->GetOwnerGUID() != GetGUID())
            m_Controlled.erase(charm);
    }
}

int32 Unit::DealHeal(Unit *pVictim, uint32 addhealth)
{
    int32 gain = 0;

    if (pVictim->IsAIEnabled)
        pVictim->GetAI()->HealReceived(this, addhealth);

    if (IsAIEnabled)
        GetAI()->HealDone(pVictim, addhealth);

    if (addhealth)
        gain = pVictim->ModifyHealth(int32(addhealth));

    Unit* unit = this;

    if (GetTypeId() == TYPEID_UNIT && this->ToCreature()->IsTotem())
        unit = GetOwner();

    if (unit->GetTypeId() == TYPEID_PLAYER)
    {
        if (Battleground *bg = unit->ToPlayer()->GetBattleground())
            bg->UpdatePlayerScore((Player*)unit, SCORE_HEALING_DONE, gain);

        // use the actual gain, as the overheal shall not be counted, skip gain 0 (it ignored anyway in to criteria)
        if (gain)
            unit->ToPlayer()->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE, gain, 0, pVictim);

        unit->ToPlayer()->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEAL_CASTED, addhealth);
    }

    if (pVictim->GetTypeId() == TYPEID_PLAYER)
    {
        pVictim->ToPlayer()->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_TOTAL_HEALING_RECEIVED, gain);
        pVictim->ToPlayer()->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEALING_RECEIVED, addhealth);
    }

    return gain;
}

Unit* Unit::SelectMagnetTarget(Unit *victim, SpellEntry const *spellInfo)
{
    if (!victim)
        return NULL;

    // Magic case
    if (spellInfo && (spellInfo->DmgClass == SPELL_DAMAGE_CLASS_NONE || spellInfo->DmgClass == SPELL_DAMAGE_CLASS_MAGIC))
    {
        //I am not sure if this should be redirected.
        if (spellInfo->DmgClass == SPELL_DAMAGE_CLASS_NONE)
            return victim;

        Unit::AuraEffectList const& magnetAuras = victim->GetAuraEffectsByType(SPELL_AURA_SPELL_MAGNET);
        for (Unit::AuraEffectList::const_iterator itr = magnetAuras.begin(); itr != magnetAuras.end(); ++itr)
            if (Unit* magnet = (*itr)->GetBase()->GetUnitOwner())
                if (magnet->IsAlive())
                if (Spell* spell = FindCurrentSpellBySpellId(spellInfo->Id))
                    {
                        // Store magnet aura to drop charge on hit
                        spell->SetMagnetingAura((*itr)->GetBase());
                        return magnet;
                    }
    }
    // Melee && ranged case
    else
    {
        AuraEffectList const& hitTriggerAuras = victim->GetAuraEffectsByType(SPELL_AURA_ADD_CASTER_HIT_TRIGGER);
        for (AuraEffectList::const_iterator i = hitTriggerAuras.begin(); i != hitTriggerAuras.end(); ++i)
            if (Unit* magnet = (*i)->GetBase()->GetCaster())
                if (magnet->IsAlive() && magnet->IsWithinLOSInMap(this))
                    if (roll_chance_i((*i)->GetAmount()))
                    {
                        (*i)->GetBase()->DropCharge();
                        return magnet;
                    }
    }

    return victim;
}

Unit* Unit::GetFirstControlled() const
{
    //Sequence: charmed, pet, other guardians
    Unit *unit = GetCharm();
    if (!unit)
        if (uint64 guid = GetUInt64Value(UNIT_FIELD_SUMMON))
            unit = ObjectAccessor::GetUnit(*this, guid);

    return unit;
}

void Unit::RemoveAllControlled()
{
    //possessed pet and vehicle
    if (GetTypeId() == TYPEID_PLAYER)
        this->ToPlayer()->StopCastingCharm();

    while (!m_Controlled.empty())
    {
        Unit *target = *m_Controlled.begin();
        m_Controlled.erase(m_Controlled.begin());
        if (target->GetCharmerGUID() == GetGUID())
            target->RemoveCharmAuras();
        else if (target->GetOwnerGUID() == GetGUID() && target->IsSummon())
            target->ToTempSummon()->UnSummon();
        else
            sLog->outError("Unit %u is trying to release unit %u which is neither charmed nor owned by it", GetEntry(), target->GetEntry());
    }
    if (GetPetGUID())
        sLog->outCrash("Unit %u is not able to release its pet " UI64FMTD, GetEntry(), GetPetGUID());
    if (GetMinionGUID())
        sLog->outCrash("Unit %u is not able to release its minion " UI64FMTD, GetEntry(), GetMinionGUID());
    if (GetCharmGUID())
        sLog->outCrash("Unit %u is not able to release its charm " UI64FMTD, GetEntry(), GetCharmGUID());
}

Unit* Unit::GetNextRandomRaidMemberOrPet(float radius)
{
    Player* player = NULL;
    if (GetTypeId() == TYPEID_PLAYER)
        player = (Player*)this;
    // Should we enable this also for charmed units?
    else if (GetTypeId() == TYPEID_UNIT && this->ToCreature()->IsPet())
        player = (Player*)GetOwner();

    if (!player)
        return NULL;
    Group *pGroup = player->GetGroup();
    //When there is no group check pet presence
    if (!pGroup)
    {
        // We are pet now, return owner
        if (player != this)
            return IsWithinDistInMap(player, radius) ? player : NULL;
        Unit * pet = GetGuardianPet();
        //No pet, no group, nothing to return
        if (!pet)
            return NULL;
        // We are owner now, return pet
        return IsWithinDistInMap(pet, radius) ? pet : NULL;
    }

    std::vector<Unit*> nearMembers;
    //reserve place for players and pets because resizing vector every unit push is unefficient (vector is reallocated then)
    nearMembers.reserve(pGroup->GetMembersCount()*2);

    for (GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        if (Player *Target = itr->getSource())
        {
            // IsHostileTo check duel and controlled by enemy
            if (Target !=this && Target->IsAlive() && IsWithinDistInMap(Target, radius) && !IsHostileTo(Target))
                nearMembers.push_back(Target);

        // Push player's pet to vector
        if (Unit *pet = Target->GetGuardianPet())
            if (pet != this && pet->IsAlive() && IsWithinDistInMap(pet, radius) && !IsHostileTo(pet))
                nearMembers.push_back(pet);
        }

    if (nearMembers.empty())
        return NULL;

    uint32 randTarget = urand(0,nearMembers.size()-1);
    return nearMembers[randTarget];
}

//only called in Player::SetSeer
// so move it to Player?
void Unit::AddPlayerToVision(Player* plr)
{
    if (m_sharedVision.empty())
    {
        setActive(true);
        SetWorldObject(true);
    }
    m_sharedVision.push_back(plr);
}

//only called in Player::SetSeer
void Unit::RemovePlayerFromVision(Player* plr)
{
    m_sharedVision.remove(plr);
    if (m_sharedVision.empty())
    {
        setActive(false);
        SetWorldObject(false);
    }
}

void Unit::RemoveBindSightAuras()
{
    RemoveAurasByType(SPELL_AURA_BIND_SIGHT);
}

void Unit::RemoveCharmAuras()
{
    RemoveAurasByType(SPELL_AURA_MOD_CHARM);
    RemoveAurasByType(SPELL_AURA_MOD_POSSESS_PET);
    RemoveAurasByType(SPELL_AURA_MOD_POSSESS);
    RemoveAurasByType(SPELL_AURA_AOE_CHARM);
}

void Unit::UnsummonAllTotems()
{
    for (uint8 i = 0; i < MAX_SUMMON_SLOT; ++i)
    {
        if (!m_SummonSlot[i])
            continue;

        if (Creature *OldTotem = GetMap()->GetCreature(m_SummonSlot[i]))
            if (OldTotem->IsSummon())
                OldTotem->ToTempSummon()->UnSummon();
    }
}

void Unit::SendHealSpellLog(Unit *pVictim, uint32 SpellID, uint32 Damage, uint32 OverHeal, uint32 Absorb, bool critical)
{
    // we guess size
    WorldPacket data(SMSG_SPELLHEALLOG, 8+8+4+4+4+4+1+1);
    data.append(pVictim->GetPackGUID());
    data.append(GetPackGUID());
    data << uint32(SpellID);
    data << uint32(Damage);
    data << uint32(OverHeal);
    data << uint32(Absorb); // Absorb amount
    data << uint8(critical ? 1 : 0);
    data << uint8(0); // unused
    SendMessageToSet(&data, true);
}

int32 Unit::HealBySpell(Unit * pVictim, SpellEntry const * spellInfo, uint32 addHealth, bool critical)
{
    uint32 absorb = 0;
    // calculate heal absorb and reduce healing
    CalcHealAbsorb(pVictim, spellInfo, addHealth, absorb);

    int32 gain = DealHeal(pVictim, addHealth);
    SendHealSpellLog(pVictim, spellInfo->Id, addHealth, uint32(addHealth - gain), absorb, critical);
    return gain;
}

void Unit::SendEnergizeSpellLog(Unit *pVictim, uint32 SpellID, int32 Damage, Powers powertype)
{
    WorldPacket data(SMSG_SPELLENERGIZELOG, 8+8+4+4+4+1);
    data.append(pVictim->GetPackGUID());
    data.append(GetPackGUID());
    data << uint32(SpellID);
    data << uint32(powertype);
    data << int32(Damage);
    SendMessageToSet(&data, true);
}

void Unit::EnergizeBySpell(Unit *pVictim, uint32 SpellID, int32 Damage, Powers powertype)
{
    SendEnergizeSpellLog(pVictim, SpellID, Damage, powertype);
    // needs to be called after sending spell log
    pVictim->ModifyPower(powertype, Damage);
}

uint32 Unit::SpellDamageBonus(Unit *pVictim, SpellEntry const *spellProto, uint32 effIndex, uint32 damage, DamageEffectType damagetype,uint32 stack)
{
    if (!pVictim)
        return damage;

    damage = this->SpellDamageBonusDone(pVictim, spellProto, effIndex, (uint32)damage, damagetype, stack);

    damage = pVictim->SpellDamageBonusTaken(this, spellProto,effIndex, (uint32)damage, damagetype, stack);

    return damage;
}

// OLD COMPUTATION METHOD -> remove this later
/*uint32 Unit::SpellDamageBonus(Unit *pVictim, SpellEntry const *spellProto, uint32 effIndex, uint32 pdamage, DamageEffectType damagetype, uint32 stack)
{
    if (!spellProto || !pVictim || damagetype == DIRECT_DAMAGE)
        return pdamage;

    // Some spells should not gain any spell damage bonus, because their damage was computed from (mostly percentage) damage of other spells
    // There is no special attribute or any general rule ...
    switch (spellProto->Id)
    {
        case 63675: // Improved Devouring Plague
        case 83853: // Combustion
        case 12654: // Ignite
            return pdamage;
    }

    // For totems get damage bonus from owner
    if (GetTypeId() == TYPEID_UNIT && this->ToCreature()->IsTotem())
        if (Unit *owner = GetOwner())
            return owner->SpellDamageBonus(pVictim, spellProto, effIndex, pdamage, damagetype);

    // Taken/Done total percent damage auras
    float DoneTotalMod = 1.0f;
    float ApCoeffMod = 1.0f;
    int32 DoneTotal = 0;
    int32 TakenTotal = 0;

    // ..done
    // Pet damage
    if (GetTypeId() == TYPEID_UNIT && !this->ToCreature()->IsPet())
        DoneTotalMod *= this->ToCreature()->GetSpellDamageMod(this->ToCreature()->GetCreatureInfo()->rank);

    // Some spells don't benefit from pct done mods
    if (!(spellProto->AttributesEx6 & SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS))
    {
        AuraEffectList const &mModDamagePercentDone = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
        for (AuraEffectList::const_iterator i = mModDamagePercentDone.begin(); i != mModDamagePercentDone.end(); ++i)
            if (((*i)->GetMiscValue() & GetSpellSchoolMask(spellProto)) &&
                (*i)->GetSpellProto()->EquippedItemClass == -1 &&          // -1 == any item class (not wand)
                (*i)->GetSpellProto()->EquippedItemInventoryTypeMask == 0) // 0 == any inventory type (not wand)
                DoneTotalMod *= ((*i)->GetAmount()+100.0f)/100.0f;
    }

    uint32 creatureTypeMask = pVictim->GetCreatureTypeMask();
    // Add flat bonus from spell damage versus
    DoneTotal += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_FLAT_SPELL_DAMAGE_VERSUS, creatureTypeMask);
    AuraEffectList const &mDamageDoneVersus = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_DONE_VERSUS);
    for (AuraEffectList::const_iterator i = mDamageDoneVersus.begin(); i != mDamageDoneVersus.end(); ++i)
        if (creatureTypeMask & uint32((*i)->GetMiscValue()))
            DoneTotalMod *= ((*i)->GetAmount()+100.0f)/100.0f;

    // bonus against aurastate
    AuraEffectList const &mDamageDoneVersusAurastate = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_DONE_VERSUS_AURASTATE);
    for (AuraEffectList::const_iterator i = mDamageDoneVersusAurastate.begin(); i != mDamageDoneVersusAurastate.end(); ++i)
        if (pVictim->HasAuraState(AuraState((*i)->GetMiscValue())))
            DoneTotalMod *= ((*i)->GetAmount() + 100.0f) / 100.0f;

    // done scripted mod (take it from owner)
    Unit * owner = GetOwner() ? GetOwner() : this;
    AuraEffectList const &mOverrideClassScript= owner->GetAuraEffectsByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
    for (AuraEffectList::const_iterator i = mOverrideClassScript.begin(); i != mOverrideClassScript.end(); ++i)
    {
        if (!(*i)->IsAffectedOnSpell(spellProto))
            continue;

        switch ((*i)->GetMiscValue())
        {
            case 12368: // Molten Fury
            case 4919:
            {
                if (pVictim->HasAuraState(AURA_STATE_HEALTHLESS_35_PERCENT, spellProto, this))
                    DoneTotalMod *= (100.0f+(*i)->GetAmount())/100.0f;
                break;
            }
            case 6917: // Death's Embrace
            case 6926:
            case 6928:
            {
                if (pVictim->GetHealthPct() < 25.0f)
                    DoneTotalMod *= (100.0f+(*i)->GetAmount())/100.0f;
                break;
            }
            // Soul Siphon
            case 4992:
            case 4993:
            {
                // effect 1 m_amount
                int32 maxPercent = (*i)->GetAmount();
                // effect 0 m_amount
                int32 stepPercent = CalculateSpellDamage(this, (*i)->GetSpellProto(), 0);
                // count affliction effects and calc additional damage in percentage
                int32 modPercent = 0;
                AuraApplicationMap const &victimAuras = pVictim->GetAppliedAuras();
                for (AuraApplicationMap::const_iterator itr = victimAuras.begin(); itr != victimAuras.end(); ++itr)
                {
                    Aura const * aura = itr->second->GetBase();
                    SpellEntry const *m_spell = aura->GetSpellProto();
                    //  Unstable affliction, Corruption, Bane of agony, Bane of doom
                    if (m_spell->SpellFamilyName != SPELLFAMILY_WARLOCK || !(m_spell->Id == 30108  || m_spell->Id == 172 || m_spell->Id == 980 || m_spell->Id == 603))
                        continue;
                    modPercent += stepPercent * aura->GetStackAmount();
                    if (modPercent >= maxPercent)
                    {
                        modPercent = maxPercent;
                        break;
                    }
                }
                DoneTotalMod *= (modPercent+100.0f)/100.0f;
                break;
            }
            case 6916: // Death's Embrace
            case 6925:
            case 6927:
                if (HasAuraState(AURA_STATE_HEALTHLESS_20_PERCENT, spellProto, this))
                    DoneTotalMod *= (100.0f+(*i)->GetAmount())/100.0f;
                break;
            case 5481: // Starfire Bonus
            {
                if (pVictim->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DRUID, 0x200002, 0, 0))
                    DoneTotalMod *= ((*i)->GetAmount()+100.0f)/100.0f;
                break;
            }
            case 4418: // Increased Shock Damage
            case 4554: // Increased Lightning Damage
            case 4555: // Improved Moonfire
            case 5142: // Increased Lightning Damage
            case 5147: // Improved Consecration / Libram of Resurgence
            case 5148: // Idol of the Shooting Star
            case 6008: // Increased Lightning Damage
            case 8627: // Totem of Hex
            {
                DoneTotal += (*i)->GetAmount();
                break;
            }
            // Rage of Rivendare
            case 7293:
            {
                if (pVictim->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DEATHKNIGHT, 0,0x02000000,0))
                {
                    if (SpellChainNode const *chain = sSpellMgr->GetSpellChainNode((*i)->GetId()))
                        DoneTotalMod *= (chain->rank * 2.0f + 100.0f)/100.0f;
                }
                break;
            }
            // Twisted Faith
            case 7377:
            {
                if (pVictim->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST, 0x8000, 0,0, GetGUID()))
                    DoneTotalMod *= ((*i)->GetAmount()+100.0f)/100.0f;
                break;
            }
        }
    }

    // Custom scripted damage
    switch(spellProto->SpellFamilyName)
    {
        case SPELLFAMILY_MAGE:
            // Ice Lance
            if (spellProto->Id == 30455)
            {
                if (pVictim->HasAuraState(AURA_STATE_FROZEN, spellProto, this))
                {
                    DoneTotalMod *= 2.0f;

                    // Fingers of Frost increases damage by another 25%
                    if (HasAura(44544))
                        DoneTotalMod *= 1.25f;
                }
            }

            // Torment the weak
            if (spellProto->SchoolMask & SPELL_SCHOOL_MASK_ARCANE)
                if (pVictim->HasAuraType(SPELL_AURA_MOD_DECREASE_SPEED))
                {
                    AuraEffectList const& mDumyAuras = GetAuraEffectsByType(SPELL_AURA_DUMMY);
                    for (AuraEffectList::const_iterator i = mDumyAuras.begin(); i != mDumyAuras.end(); ++i)
                        if ((*i)->GetSpellProto()->SpellIconID == 2215)
                        {
                            DoneTotalMod *= float((*i)->GetAmount() + 100.f) / 100.f;
                            break;
                        }
                }
        break;
        case SPELLFAMILY_PRIEST:
            // Smite
            if (spellProto->SpellFamilyFlags[0] & 0x80)
            {
                // Glyph of Smite
                if (AuraEffect * aurEff = GetAuraEffect(55692, 0))
                    if (pVictim->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST, 0x100000, 0, 0, GetGUID()))
                        AddPctN(DoneTotalMod, aurEff->GetAmount());
            }
        break;
        case SPELLFAMILY_PALADIN:
            // Judgement of Vengeance/Judgement of Corruption
            if ((spellProto->SpellFamilyFlags[1] & 0x400000) && spellProto->SpellIconID == 2292)
            {
                // Get stack of Holy Vengeance/Blood Corruption on the target added by caster
                uint32 stacks = 0;
                Unit::AuraEffectList const& auras = pVictim->GetAuraEffectsByType(SPELL_AURA_PERIODIC_DAMAGE);
                for (Unit::AuraEffectList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                    if (((*itr)->GetId() == 31803 || (*itr)->GetId() == 53742) && (*itr)->GetCasterGUID() == GetGUID())
                    {
                        stacks = (*itr)->GetBase()->GetStackAmount();
                        break;
                    }
                // + 10% for each application of Holy Vengeance/Blood Corruption on the target
                if (stacks)
                    DoneTotalMod *= (10.0f + float(stacks)) / 10.0f;
            }

            if (spellProto->Id == 96172) // Hand of Light
            {
                if (AuraEffect * aurEff = GetAuraEffect(84963,0)) // Inquisition
                {
                    DoneTotalMod *= ((aurEff)->GetAmount() + 100.0f) / 100.0f;
                }
            }
        break;
        case SPELLFAMILY_WARLOCK:
            //Fire and Brimstone
            if (spellProto->SpellFamilyFlags[1] & 0x00020040)
                if (pVictim->HasAuraState(AURA_STATE_CONFLAGRATE))
                {
                    AuraEffectList const& mDumyAuras = GetAuraEffectsByType(SPELL_AURA_DUMMY);
                    for (AuraEffectList::const_iterator i = mDumyAuras.begin(); i != mDumyAuras.end(); ++i)
                        if ((*i)->GetSpellProto()->SpellIconID == 3173)
                        {
                            DoneTotalMod *= float((*i)->GetAmount() + 100.f) / 100.f;
                            break;
                        }
                }
        break;
        case SPELLFAMILY_HUNTER:
        break;
        case SPELLFAMILY_DEATHKNIGHT:
            // Merciless Combat
            if (pVictim->HealthBelowPct(35))
            {
                // Icy Touch, Howling Blast
                if (spellProto->Id == 45477 || spellProto->Id == 49184)
                {
                    if (AuraEffect * aurEff = GetAuraEffect(SPELL_AURA_DUMMY, SPELLFAMILY_DEATHKNIGHT, 2656, EFFECT_0))
                        AddPctF(DoneTotalMod, aurEff->GetAmount());
                }
            }

            // Improved Icy Touch
            if (spellProto->SpellFamilyFlags[0] & 0x2)
                if (AuraEffect * aurEff = GetDummyAuraEffect(SPELLFAMILY_DEATHKNIGHT, 2721, 0))
                    DoneTotalMod *= (100.0f + aurEff->GetAmount()) / 100.0f;

            // Glacier Rot
            if (spellProto->SpellFamilyFlags[0] & 0x2 || spellProto->SpellFamilyFlags[1] & 0x6)
                if (AuraEffect * aurEff = GetDummyAuraEffect(SPELLFAMILY_DEATHKNIGHT, 196, 0))
                    if (pVictim->GetDiseasesByCaster(owner->GetGUID()) > 0)
                        DoneTotalMod *= (100.0f + aurEff->GetAmount()) / 100.0f;
        break;
        case SPELLFAMILY_GENERIC:
            // The Widow's Kiss
            if (spellProto->Id == 99506)
            {
                // multiply the damage done by stack amount
                if (Aura *aura = this->GetAura(spellProto->Id, GetGUID()))
                    pdamage *= aura->GetStackAmount();
            }
        break;
    }

    // ..taken
    float TakenTotalMod = 1.0f;
    TakenTotalMod *= pVictim->GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN, spellProto->SchoolMask);

    //Ebon plague (need special handling, cause missing spell effect data)
    if (Aura * aura = pVictim->GetAura(65142))
    {
        int32 amount = aura->GetSpellProto()->EffectBasePoints[EFFECT_1];
        bool applyBonus = true;
        // Dont add bonus taken damage if target has same aura with same amount (prevent stacking)
        AuraEffectList const& damageTakenList = pVictim->GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN);
        for (AuraEffectList::const_iterator i = damageTakenList.begin(); i != damageTakenList.end(); ++i)
            if (amount == (*i)->GetAmount() && (SPELL_SCHOOL_MASK_MAGIC & GetSpellSchoolMask((*i)->GetSpellProto())))
                applyBonus = false;

        if (applyBonus && (SPELL_SCHOOL_MASK_MAGIC & GetSpellSchoolMask(spellProto)))
                AddPctN(TakenTotalMod, amount);
    }

    // .. taken pct: dummy auras
    AuraEffectList const& mDummyAuras = pVictim->GetAuraEffectsByType(SPELL_AURA_DUMMY);
    for (AuraEffectList::const_iterator i = mDummyAuras.begin(); i != mDummyAuras.end(); ++i)
    {
        switch((*i)->GetSpellProto()->SpellIconID)
        {
            // Cheat Death
            case 2109:
                if ((*i)->GetMiscValue() & SPELL_SCHOOL_MASK_NORMAL)
                {
                    if (pVictim->GetTypeId() != TYPEID_PLAYER)
                        continue;
                    AddPctN(TakenTotalMod, (*i)->GetAmount());
                }
                break;
        }
    }

    // From caster spells
    AuraEffectList const& mOwnerTaken = pVictim->GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_FROM_CASTER);
    for (AuraEffectList::const_iterator i = mOwnerTaken.begin(); i != mOwnerTaken.end(); ++i)
        if ((*i)->GetCasterGUID() == GetGUID() && (*i)->IsAffectedOnSpell(spellProto))
            AddPctN(TakenTotalMod, (*i)->GetAmount());

    // Mod damage from spell mechanic
    uint32 mechanicMask = GetAllSpellMechanicMask(spellProto);

    // Shred, Maul - "Effects which increase Bleed damage also increase Shred damage"
    if (spellProto->SpellFamilyName == SPELLFAMILY_DRUID && spellProto->SpellFamilyFlags[0] & 0x00008800)
        mechanicMask |= (1<<MECHANIC_BLEED);

    if (mechanicMask)
    {
        AuraEffectList const& mDamageDoneMechanic = pVictim->GetAuraEffectsByType(SPELL_AURA_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT);
        for (AuraEffectList::const_iterator i = mDamageDoneMechanic.begin(); i != mDamageDoneMechanic.end(); ++i)
            if (mechanicMask & uint32(1<<((*i)->GetMiscValue())))
                AddPctN(TakenTotalMod, (*i)->GetAmount());
    }

    // Taken/Done fixed damage bonus auras
    int32 DoneAdvertisedBenefit  = SpellBaseDamageBonusDone(GetSpellSchoolMask(spellProto));
    int32 TakenAdvertisedBenefit = pVictim->SpellBaseDamageBonusTaken(GetSpellSchoolMask(spellProto));
    // Pets just add their bonus damage to their spell damage
    // note that their spell damage is just gain of their own auras
    if (HasUnitTypeMask(UNIT_MASK_GUARDIAN))
        DoneAdvertisedBenefit += ((Guardian*)this)->GetBonusDamage();

    // Check for table values
    float coeff = 0;
    SpellBonusEntry const *bonus = sSpellMgr->GetSpellBonusData(spellProto->Id);
    if (bonus)
    {
        if (damagetype == DOT)
        {
            coeff = bonus->dot_damage;
            if (bonus->ap_dot_bonus > 0)
            {
                WeaponAttackType attType = (IsRangedWeaponSpell(spellProto) && spellProto->DmgClass != SPELL_DAMAGE_CLASS_MELEE) ? RANGED_ATTACK : BASE_ATTACK;
                float APbonus = attType == BASE_ATTACK ? pVictim->GetTotalAuraModifier(SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS) : pVictim->GetTotalAuraModifier(SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS);
                APbonus += GetTotalAttackPowerValue(attType);
                DoneTotal += int32(bonus->ap_dot_bonus * stack * ApCoeffMod * APbonus);
            }
        }
        else
        {
            coeff = bonus->direct_damage;
            if (bonus->ap_bonus > 0)
            {
                WeaponAttackType attType = (IsRangedWeaponSpell(spellProto) && spellProto->DmgClass != SPELL_DAMAGE_CLASS_MELEE) ? RANGED_ATTACK : BASE_ATTACK;
                float APbonus = attType == BASE_ATTACK ? pVictim->GetTotalAuraModifier(SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS) : pVictim->GetTotalAuraModifier(SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS);
                APbonus += GetTotalAttackPowerValue(attType);
                DoneTotal += int32(bonus->ap_bonus * stack * ApCoeffMod * APbonus);
            }
        }
    }
    // Default calculation
    if (DoneAdvertisedBenefit || TakenAdvertisedBenefit)
    {
        if(coeff <= 0)
        {
            if(effIndex < 4)
            {
                coeff = spellProto->EffectBonusCoefficient[effIndex];
            }
            else
            {   // should never happen
                coeff = 0;
            }

        }
        float coeff2 = CalculateLevelPenalty(spellProto) * stack;
        if (spellProto->SpellFamilyName) //TODO: fix this
            TakenTotal+= int32(TakenAdvertisedBenefit * coeff * coeff2);
        if (Player* modOwner = GetSpellModOwner())
        {
            coeff *= 100.0f;
            modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_BONUS_MULTIPLIER, coeff);
            coeff /= 100.0f;
        }
        DoneTotal += int32(DoneAdvertisedBenefit * coeff * coeff2);
    }

    // Some spells don't benefit from done mods
    if (spellProto->AttributesEx3 & SPELL_ATTR3_NO_DONE_BONUS)
    {
        DoneTotal = 0;
        DoneTotalMod = 1.0f;
    }

    float tmpDamage = (int32(pdamage) + DoneTotal) * DoneTotalMod;

    // apply spellmod to Done damage (flat and pct)
    if (Player* modOwner = GetSpellModOwner())
        modOwner->ApplySpellMod(spellProto->Id, damagetype == DOT ? SPELLMOD_DOT : SPELLMOD_DAMAGE, tmpDamage);

    tmpDamage = (tmpDamage + TakenTotal) * TakenTotalMod;

    return uint32(std::max(tmpDamage, 0.0f));
}*/

uint32 Unit::SpellDamageBonusDone(Unit* victim, SpellEntry const* spellProto, uint32 effIndex, uint32 pdamage, DamageEffectType damagetype, uint32 stack) const
{
    if (!spellProto || !victim || damagetype == DIRECT_DAMAGE)
        return pdamage;

    // Some spells don't benefit from done mods at all
    if (spellProto->AttributesEx3 & SPELL_ATTR3_NO_DONE_BONUS)
        return pdamage;

    // Some spells should not gain any spell damage bonus, because their damage was computed from (mostly percentage) damage of other spells
    // There is no special attribute or any general rule ...
    switch (spellProto->Id)
    {
        case 63675: // Improved Devouring Plague
        case 83853: // Combustion
        case 12654: // Ignite
        case 83077: // Improved Serpent Sting
        case 63468: // Piercing Shots
            return pdamage;
        case 44614: // Frostfire Bolt
            if (damagetype == DOT)
                return pdamage;
    }

    // For totems get damage bonus from owner
    if (GetTypeId() == TYPEID_UNIT && ToCreature()->IsTotem())
        if (Unit* owner = GetOwner())
            return owner->SpellDamageBonusDone(victim, spellProto, effIndex, pdamage, damagetype);

    float ApCoeffMod = 1.0f;
    int32 DoneTotal = 0;

    // done scripted mod (take it from owner)
    Unit const* owner = GetOwner() ? GetOwner() : this;
    AuraEffectList const& mOverrideClassScript = owner->GetAuraEffectsByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
    for (AuraEffectList::const_iterator i = mOverrideClassScript.begin(); i != mOverrideClassScript.end(); ++i)
    {
        if (!(*i)->IsAffectedOnSpell(spellProto))
            continue;

        switch ((*i)->GetMiscValue())
        {
            case 4418: // Increased Shock Damage
            case 4554: // Increased Lightning Damage
            case 4555: // Improved Moonfire
            case 5142: // Increased Lightning Damage
            case 5147: // Improved Consecration / Libram of Resurgence
            case 5148: // Idol of the Shooting Star
            case 6008: // Increased Lightning Damage
            case 8627: // Totem of Hex
            {
                DoneTotal += (*i)->GetAmount();
                break;
            }
        }
    }

    // Custom scripted damage
    switch (spellProto->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
            // The Widow's Kiss
            if (spellProto->Id == 99506)
            {
                // multiply the damage done by stack amount
                if (Aura *aura = this->GetAura(spellProto->Id, GetGUID()))
                    pdamage *= aura->GetStackAmount();
            }
        break;
    }

    // Done fixed damage bonus auras
    int32 DoneAdvertisedBenefit  = SpellBaseDamageBonusDone(GetSpellSchoolMask(spellProto));
    // Pets just add their bonus damage to their spell damage
    // note that their spell damage is just gain of their own auras
    if (HasUnitTypeMask(UNIT_MASK_GUARDIAN))
        DoneAdvertisedBenefit += ((Guardian*)this)->GetBonusDamage();

    // Check for table values
    float coeff = 0;
    SpellBonusEntry const* bonus = sSpellMgr->GetSpellBonusData(spellProto->Id);
    if (bonus)
    {
        if (damagetype == DOT)
        {
            coeff = bonus->dot_damage;
            if (bonus->ap_dot_bonus > 0)
            {
                WeaponAttackType attType = (IsRangedWeaponSpell(spellProto) && spellProto->DmgClass != SPELL_DAMAGE_CLASS_MELEE) ? RANGED_ATTACK : BASE_ATTACK;
                float APbonus = float(victim->GetTotalAuraModifier(attType == BASE_ATTACK ? SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS : SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS));
                APbonus += GetTotalAttackPowerValue(attType);
                DoneTotal += int32(bonus->ap_dot_bonus * stack * ApCoeffMod * APbonus);
            }
        }
        else
        {
            coeff = bonus->direct_damage;
            if (bonus->ap_bonus > 0)
            {
                WeaponAttackType attType = (IsRangedWeaponSpell(spellProto) && spellProto->DmgClass != SPELL_DAMAGE_CLASS_MELEE) ? RANGED_ATTACK : BASE_ATTACK;
                float APbonus = float(victim->GetTotalAuraModifier(attType == BASE_ATTACK ? SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS : SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS));
                APbonus += GetTotalAttackPowerValue(attType);
                DoneTotal += int32(bonus->ap_bonus * stack * ApCoeffMod * APbonus);
            }
        }
    }

    // Default calculation
    if (DoneAdvertisedBenefit)
    {
        if(coeff <= 0)
        {
            if(effIndex < MAX_SPELL_EFFECTS)
                coeff = spellProto->EffectBonusCoefficient[effIndex]; // Spell power coeficient
            else
                coeff = 0; // should never happen
        }

        float factorMod = CalculateLevelPenalty(spellProto) * stack;

        if (Player* modOwner = GetSpellModOwner())
        {
            coeff *= 100.0f;
            modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_BONUS_MULTIPLIER, coeff);
            coeff /= 100.0f;
        }
        DoneTotal += int32(DoneAdvertisedBenefit * coeff * factorMod);
    }

    // Done Percentage for DOT is already calculated, no need to do it again. The percentage mod is applied in Aura::HandleAuraSpecificMods.
    float tmpDamage = (int32(pdamage) + DoneTotal) * (damagetype == DOT ? 1.0f : SpellDamagePctDone(victim, spellProto, damagetype));
    // apply spellmod to Done damage (flat and pct)
    if (Player* modOwner = GetSpellModOwner())
        modOwner->ApplySpellMod(spellProto->Id, damagetype == DOT ? SPELLMOD_DOT : SPELLMOD_DAMAGE, tmpDamage);

    return uint32(std::max(tmpDamage, 0.0f));
}

float Unit::SpellDamagePctDone(Unit* victim, SpellEntry const* spellProto, DamageEffectType damagetype) const
{
    if (!spellProto || !victim || damagetype == DIRECT_DAMAGE)
        return 1.0f;

     // Hand of Light should profit from Inquisition, despite SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS -> was hotfixed in 4.3.0
    if (spellProto->Id == 96172)
    if (AuraEffect * aurEff = GetAuraEffect(84963,EFFECT_0)) // Inquisition
        return (aurEff->GetAmount() + 100.0f) / 100.0f;

    // Some spells don't benefit from pct done mods
    if (spellProto->AttributesEx6 & SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS)
        return 1.0f;

    // Some spells don't benefit from done bonuses at all
    if (spellProto->AttributesEx3 & SPELL_ATTR3_NO_DONE_BONUS)
        return 1.0f;

    // Some spells should not gain any spell damage bonus, because their damage was computed from (mostly percentage) damage of other spells
    // There is no special attribute or any general rule ...
    switch (spellProto->Id)
    {
        case 63675: // Improved Devouring Plague
        case 83853: // Combustion
        case 12654: // Ignite
        case 83077: // Improved Serpent Sting
        case 63468: // Piercing Shots
            return 1.0f;
        case 44614: // Frostfire Bolt
            if (damagetype == DOT)
                return 1.0f;
    }

    // For totems pct done mods are calculated when its calculation is run on the player in SpellDamageBonusDone.
    if (GetTypeId() == TYPEID_UNIT && ToCreature()->IsTotem())
        return 1.0f;

    // Done total percent damage auras
    float DoneTotalMod = 1.0f;

    // Pet damage?
    if (GetTypeId() == TYPEID_UNIT && !ToCreature()->IsPet())
        DoneTotalMod *= ToCreature()->GetSpellDamageMod(ToCreature()->GetCreatureInfo()->rank);

    AuraEffectList const& mModDamagePercentDone = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
    for (AuraEffectList::const_iterator i = mModDamagePercentDone.begin(); i != mModDamagePercentDone.end(); ++i)
    {
        if (spellProto->EquippedItemClass == -1 && (*i)->GetSpellProto()->EquippedItemClass != -1)    //prevent apply mods from weapon specific case to non weapon specific spells (Example: thunder clap and two-handed weapon specialization)
            continue;

        if ((*i)->GetMiscValue() & GetSpellSchoolMask(spellProto))
        {
            if ((*i)->GetSpellProto()->EquippedItemClass == -1)
                DoneTotalMod *= ((*i)->GetAmount()+100.0f)/100.0f;
            else if (!((*i)->GetSpellProto()->AttributesEx5 & SPELL_ATTR5_SPECIAL_ITEM_CLASS_CHECK) && ((*i)->GetSpellProto()->EquippedItemSubClassMask == 0))
                DoneTotalMod *= ((*i)->GetAmount()+100.0f)/100.0f;
            else if (ToPlayer() && ToPlayer()->HasItemFitToSpellRequirements((*i)->GetSpellProto()))
                DoneTotalMod *= ((*i)->GetAmount()+100.0f)/100.0f;
        }
    }

    uint32 creatureTypeMask = victim->GetCreatureTypeMask();

    AuraEffectList const& mDamageDoneVersus = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_DONE_VERSUS);
    for (AuraEffectList::const_iterator i = mDamageDoneVersus.begin(); i != mDamageDoneVersus.end(); ++i)
        if (creatureTypeMask & uint32((*i)->GetMiscValue()))
            DoneTotalMod *= ((*i)->GetAmount() + 100.0f) / 100.0f;

    // bonus against aurastate
    AuraEffectList const& mDamageDoneVersusAurastate = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_DONE_VERSUS_AURASTATE);
    for (AuraEffectList::const_iterator i = mDamageDoneVersusAurastate.begin(); i != mDamageDoneVersusAurastate.end(); ++i)
        if (victim->HasAuraState(AuraState((*i)->GetMiscValue())))
            DoneTotalMod *= ((*i)->GetAmount() + 100.0f) / 100.0f;

    // Add SPELL_AURA_MOD_DAMAGE_DONE_FOR_MECHANIC percent bonus
    AddPctN(DoneTotalMod, GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_DAMAGE_MECHANIC, spellProto->Mechanic));

    // done scripted mod (take it from owner)
    const Unit * owner = GetOwner() ? GetOwner() : this;
    AuraEffectList const &mOverrideClassScript= owner->GetAuraEffectsByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
    for (AuraEffectList::const_iterator i = mOverrideClassScript.begin(); i != mOverrideClassScript.end(); ++i)
    {
        if (!(*i)->IsAffectedOnSpell(spellProto))
            continue;

        switch ((*i)->GetMiscValue())
        {
            case 12368: // Molten Fury
            case 4919:
            {
                if (victim->HasAuraState(AURA_STATE_HEALTHLESS_35_PERCENT, spellProto, this))
                    DoneTotalMod *= (100.0f+(*i)->GetAmount())/100.0f;
                break;
            }
            case 6917: // Death's Embrace
            case 6926:
            case 6928:
            {
                if (victim->GetHealthPct() < 25.0f)
                    DoneTotalMod *= (100.0f+(*i)->GetAmount())/100.0f;
                break;
            }
            // Soul Siphon
            case 4992:
            case 4993:
            {
                // effect 1 m_amount
                int32 maxPercent = (*i)->GetAmount();
                // effect 0 m_amount
                int32 stepPercent = CalculateSpellDamage(this, (*i)->GetSpellProto(), 0);
                // count affliction effects and calc additional damage in percentage
                int32 modPercent = 0;
                AuraApplicationMap const &victimAuras = victim->GetAppliedAuras();
                for (AuraApplicationMap::const_iterator itr = victimAuras.begin(); itr != victimAuras.end(); ++itr)
                {
                    Aura const * aura = itr->second->GetBase();
                    SpellEntry const *m_spell = aura->GetSpellProto();
                    //  Unstable affliction, Corruption, Bane of agony, Bane of doom
                    if (m_spell->SpellFamilyName != SPELLFAMILY_WARLOCK || !(m_spell->Id == 30108  || m_spell->Id == 172 || m_spell->Id == 980 || m_spell->Id == 603))
                        continue;
                    modPercent += stepPercent * aura->GetStackAmount();
                    if (modPercent >= maxPercent)
                    {
                        modPercent = maxPercent;
                        break;
                    }
                }
                DoneTotalMod *= (modPercent+100.0f)/100.0f;
                break;
            }
            case 6916: // Death's Embrace
            case 6925:
            case 6927:
                if (HasAuraState(AURA_STATE_HEALTHLESS_20_PERCENT, spellProto, this))
                    DoneTotalMod *= (100.0f+(*i)->GetAmount())/100.0f;
                break;
            case 5481: // Starfire Bonus
            {
                if (victim->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DRUID, 0x200002, 0, 0))
                    DoneTotalMod *= ((*i)->GetAmount()+100.0f)/100.0f;
                break;
            }
            // Twisted Faith
            case 7377:
            {
                if (victim->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST, 0x8000, 0,0, GetGUID()))
                    DoneTotalMod *= ((*i)->GetAmount()+100.0f)/100.0f;
                break;
            }
        }
    }

    // Custom scripted damage
    switch(spellProto->SpellFamilyName)
    {
        case SPELLFAMILY_MAGE:
            // Ice Lance + Deep Freeze
            if (spellProto->Id == 30455 || spellProto->Id == 71757)
            {
                if (victim->HasAuraState(AURA_STATE_FROZEN, spellProto, this))
                {
                    DoneTotalMod *= 2.0f;

                    // Fingers of Frost increases damage by another 25%
                    if (HasAura(44544))
                        DoneTotalMod *= 1.25f;
                }
            }

            // Torment the weak
            if (spellProto->SchoolMask & SPELL_SCHOOL_MASK_ARCANE)
                if (victim->HasAuraType(SPELL_AURA_MOD_DECREASE_SPEED))
                {
                    AuraEffectList const& mDumyAuras = GetAuraEffectsByType(SPELL_AURA_DUMMY);
                    for (AuraEffectList::const_iterator i = mDumyAuras.begin(); i != mDumyAuras.end(); ++i)
                        if ((*i)->GetSpellProto()->SpellIconID == 2215)
                        {
                            DoneTotalMod *= float((*i)->GetAmount() + 100.f) / 100.f;
                            break;
                        }
                }
        break;
        case SPELLFAMILY_PRIEST:
            // Smite
            if (spellProto->SpellFamilyFlags[0] & 0x80)
            {
                // Glyph of Smite
                if (AuraEffect * aurEff = GetAuraEffect(55692, 0))
                    if (victim->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST, 0x100000, 0, 0, GetGUID()))
                        AddPctN(DoneTotalMod, aurEff->GetAmount());
            }
            else if (spellProto->Id == 32379) // Shadow Word: Death
            {
                if (victim->GetHealthPct() <= 25.0f) // At or below 25 %
                {
                    if (AuraEffect * aurEff = GetAuraEffect(SPELL_AURA_DUMMY,SPELLFAMILY_PRIEST,3139,EFFECT_0)) // Mind Melt
                        AddPctN(DoneTotalMod, aurEff->GetAmount());
                }
            }
        break;
        case SPELLFAMILY_PALADIN:
            // Judgement of Vengeance/Judgement of Corruption
            if ((spellProto->SpellFamilyFlags[1] & 0x400000) && spellProto->SpellIconID == 2292)
            {
                // Get stack of Holy Vengeance/Blood Corruption on the target added by caster
                uint32 stacks = 0;
                Unit::AuraEffectList const& auras = victim->GetAuraEffectsByType(SPELL_AURA_PERIODIC_DAMAGE);
                for (Unit::AuraEffectList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                    if (((*itr)->GetId() == 31803 || (*itr)->GetId() == 53742) && (*itr)->GetCasterGUID() == GetGUID())
                    {
                        stacks = (*itr)->GetBase()->GetStackAmount();
                        break;
                    }
                // + 10% for each application of Holy Vengeance/Blood Corruption on the target
                if (stacks)
                    DoneTotalMod *= (10.0f + float(stacks)) / 10.0f;
            }
        break;
        case SPELLFAMILY_WARLOCK:
            //Fire and Brimstone
            if (spellProto->SpellFamilyFlags[1] & 0x00020040)
                if (victim->HasAuraState(AURA_STATE_CONFLAGRATE))
                {
                    AuraEffectList const& mDumyAuras = GetAuraEffectsByType(SPELL_AURA_DUMMY);
                    for (AuraEffectList::const_iterator i = mDumyAuras.begin(); i != mDumyAuras.end(); ++i)
                        if ((*i)->GetSpellProto()->SpellIconID == 3173)
                        {
                            DoneTotalMod *= float((*i)->GetAmount() + 100.f) / 100.f;
                            break;
                        }
                }
        break;
        case SPELLFAMILY_HUNTER:
        {
            // Implementation of Beast Mastery hunter mastery proficiency for spell damage
            if (Unit * owner  = GetOwner())
            {
                if (owner->GetTypeId() == TYPEID_PLAYER)
                {
                    if (owner->ToPlayer()->GetActiveTalentBranchSpec() == SPEC_HUNTER_BEASTMASTERY)
                        DoneTotalMod *= 1.0f + (owner->ToPlayer()->GetMasteryPoints() * 1.67f / 100.0f);
                }
            }

            if (Pet::IsPetBasicAttackSpell(spellProto->Id))
            {
                // Spiked Collar 
                if (AuraEffect * aurEff = GetDummyAuraEffect(SPELLFAMILY_HUNTER,2934,EFFECT_0))
                    DoneTotalMod *= float(aurEff->GetAmount() + 100.f) / 100.f;

                // Wild hunt
                if (AuraEffect * aurEff = GetDummyAuraEffect(SPELLFAMILY_PET, 3748, EFFECT_0))
                {
                    uint32 cost = CalculatePowerCost(spellProto, this, GetSpellSchoolMask(spellProto));

                    // When your pet is at or above 50 Focus
                    if (cost + GetPower(POWER_FOCUS) >= 50)
                    {
                        // Rank 1 -> + 50 % of original cost
                        // Rank 2 -> + 100 % of original cost
                        int32 additionalFocusTaken = aurEff->GetAmount() == 60 ? (cost / 2) : cost;
                        const_cast<Unit*>(this)->ModifyPower(POWER_FOCUS, -additionalFocusTaken);
                        // Your pet's Basic Attacks will deal 60/120% more damage, 
                        DoneTotalMod *= float(aurEff->GetAmount() + 100.f) / 100.f;
                    }
                }
            }
        }
        break;
        case SPELLFAMILY_DEATHKNIGHT:
            // Merciless Combat
            if (victim->HealthBelowPct(35))
            {
                // Icy Touch, Howling Blast
                if (spellProto->Id == 45477 || spellProto->Id == 49184)
                {
                    if (AuraEffect * aurEff = GetAuraEffect(SPELL_AURA_DUMMY, SPELLFAMILY_DEATHKNIGHT, 2656, EFFECT_0))
                        AddPctF(DoneTotalMod, aurEff->GetAmount());
                }
            }

            // Improved Icy Touch
            if (spellProto->SpellFamilyFlags[0] & 0x2)
                if (AuraEffect * aurEff = GetDummyAuraEffect(SPELLFAMILY_DEATHKNIGHT, 2721, 0))
                    DoneTotalMod *= (100.0f + aurEff->GetAmount()) / 100.0f;

            // Glacier Rot
            if (spellProto->SpellFamilyFlags[0] & 0x2 || spellProto->SpellFamilyFlags[1] & 0x6)
                if (AuraEffect * aurEff = GetDummyAuraEffect(SPELLFAMILY_DEATHKNIGHT, 196, 0))
                    if (victim->GetDiseasesByCaster(owner->GetGUID()) > 0)
                        DoneTotalMod *= (100.0f + aurEff->GetAmount()) / 100.0f;
        break;
    }

    return DoneTotalMod;
}

uint32 Unit::SpellDamageBonusTaken(Unit* caster, SpellEntry const* spellProto, uint32 effIndex, uint32 pdamage, DamageEffectType damagetype, uint32 stack) const
{
    if (!spellProto || damagetype == DIRECT_DAMAGE)
        return pdamage;

    int32 TakenTotal = 0;
    float TakenTotalMod = 1.0f;
    float TakenTotalCasterMod = 0.0f;

    TakenTotalMod *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN, spellProto->SchoolMask);

    // Mod damage from spell mechanic
    if (uint32 mechanicMask = GetAllSpellMechanicMask(spellProto))
    {
        AuraEffectList const& mDamageDoneMechanic = GetAuraEffectsByType(SPELL_AURA_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT);
        for (AuraEffectList::const_iterator i = mDamageDoneMechanic.begin(); i != mDamageDoneMechanic.end(); ++i)
            if (mechanicMask & uint32(1 << ((*i)->GetMiscValue())))
                AddPctN(TakenTotalMod, (*i)->GetAmount());
    }

    //Ebon plague (need special handling)
    if (Aura * aura = GetAura(65142))
    {
        int32 amount = aura->GetSpellProto()->EffectBasePoints[EFFECT_1];
        bool applyBonus = true;
        // Dont add bonus taken damage if target has same aura with same amount (prevent stacking)
        AuraEffectList const& damageTakenList = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN);
        for (AuraEffectList::const_iterator i = damageTakenList.begin(); i != damageTakenList.end(); ++i)
            if (amount == (*i)->GetAmount() && (SPELL_SCHOOL_MASK_MAGIC & GetSpellSchoolMask((*i)->GetSpellProto())))
                applyBonus = false;

        if (applyBonus && (SPELL_SCHOOL_MASK_MAGIC & GetSpellSchoolMask(spellProto)))
                AddPctN(TakenTotalMod, amount);
    }

    //.. taken pct: dummy auras
    AuraEffectList const& mDummyAuras = GetAuraEffectsByType(SPELL_AURA_DUMMY);
    for (AuraEffectList::const_iterator i = mDummyAuras.begin(); i != mDummyAuras.end(); ++i)
    {
        switch ((*i)->GetSpellProto()->SpellIconID)
        {
            // Cheat Death
            case 2109:
                if ((*i)->GetMiscValue() & SPELL_SCHOOL_MASK_NORMAL)
                    AddPctN(TakenTotalMod, (*i)->GetAmount());
                break;
        }
    }
        // Spells with SPELL_ATTR4_FIXED_DAMAGE should only benefit from mechanic damage mod auras.
    if (!(spellProto->AttributesEx4 & SPELL_ATTR4_FIXED_DAMAGE))
    {
        // get all auras from caster that allow the spell to ignore resistance (sanctified wrath)
        AuraEffectList const& IgnoreResistAuras = caster->GetAuraEffectsByType(SPELL_AURA_MOD_IGNORE_TARGET_RESIST);
        for (AuraEffectList::const_iterator i = IgnoreResistAuras.begin(); i != IgnoreResistAuras.end(); ++i)
        {
            if ((*i)->GetMiscValue() & GetSpellSchoolMask(spellProto))
                TakenTotalCasterMod += (float((*i)->GetAmount()));
        }

        // From caster spells
        AuraEffectList const& mOwnerTaken = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_FROM_CASTER);
        for (AuraEffectList::const_iterator i = mOwnerTaken.begin(); i != mOwnerTaken.end(); ++i)
            if ((*i)->GetCasterGUID() == caster->GetGUID() && (*i)->IsAffectedOnSpell(spellProto))
                AddPctN(TakenTotalMod, (*i)->GetAmount());

        int32 TakenAdvertisedBenefit = SpellBaseDamageBonusTaken(GetSpellSchoolMask(spellProto));

        // Check for table values
        float coeff = 0;
        SpellBonusEntry const* bonus = sSpellMgr->GetSpellBonusData(spellProto->Id);
        if (bonus)
            coeff = (damagetype == DOT) ? bonus->dot_damage : bonus->direct_damage;

        // Default calculation
        if (TakenAdvertisedBenefit)
        {
            if(coeff <= 0 || !bonus)
            {
                if(effIndex < MAX_SPELL_EFFECTS)
                    coeff = spellProto->EffectBonusCoefficient[effIndex]; // Spell power coeficient
                else
                    coeff = 0; // should never happen
            }

            float factorMod = CalculateLevelPenalty(spellProto) * stack;
            // level penalty still applied on Taken bonus - is it blizzlike?
            if (Player* modOwner = GetSpellModOwner())
            {
                coeff *= 100.0f;
                modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_BONUS_MULTIPLIER, coeff);
                coeff /= 100.0f;
            }
            TakenTotal += int32(TakenAdvertisedBenefit * coeff * factorMod);
        }
    }

    float tmpDamage = 0.0f;

    if (TakenTotalCasterMod)
    {
        if (TakenTotal < 0)
        {
            if (TakenTotalMod < 1)
                tmpDamage = ((float(CalculatePctF(pdamage, TakenTotalCasterMod) + TakenTotal) * TakenTotalMod) + CalculatePctF(pdamage, TakenTotalCasterMod));
            else
                tmpDamage = ((float(CalculatePctF(pdamage, TakenTotalCasterMod) + TakenTotal) + CalculatePctF(pdamage, TakenTotalCasterMod)) * TakenTotalMod);
        }
        else if (TakenTotalMod < 1)
            tmpDamage = ((CalculatePctF(float(pdamage) + TakenTotal, TakenTotalCasterMod) * TakenTotalMod) + CalculatePctF(float(pdamage) + TakenTotal, TakenTotalCasterMod));
    }
    if (!tmpDamage)
        tmpDamage = (float(pdamage) + TakenTotal) * TakenTotalMod;

    return uint32(std::max(tmpDamage, 0.0f));
}

int32 Unit::SpellBaseDamageBonusDone(SpellSchoolMask schoolMask) const
{
    // SPELL_AURA_OVERRIDE_SPELL_POWER_BY_AP_PCT:
    // Your spell power is now equal to ??% of your attack power, and you no longer benefit from other sources of spell power
    AuraEffectList const& mDamageDonebyAP = GetAuraEffectsByType(SPELL_AURA_OVERRIDE_SPELL_POWER_BY_AP_PCT);
    if (!mDamageDonebyAP.empty())
    {
        float AP = GetTotalAttackPowerValue(BASE_ATTACK);
        int32 amount = 0;
        for (AuraEffectList::const_iterator i =mDamageDonebyAP.begin(); i != mDamageDonebyAP.end(); ++i)
            if ((*i)->GetMiscValue() & schoolMask)
                amount += (*i)->GetAmount();

        return  uint32(AP * amount / 100.0f);
    }


    int32 DoneAdvertisedBenefit = 0;

    // ..done
    AuraEffectList const& mDamageDone = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_DONE);
    for (AuraEffectList::const_iterator i = mDamageDone.begin(); i != mDamageDone.end(); ++i)
        if (((*i)->GetMiscValue() & schoolMask) != 0 &&
        (*i)->GetSpellProto()->EquippedItemClass == -1 &&
                                                            // -1 == any item class (not wand then)
        (*i)->GetSpellProto()->EquippedItemInventoryTypeMask == 0)
                                                            // 0 == any inventory type (not wand then)
            DoneAdvertisedBenefit += (*i)->GetAmount();

    if (GetTypeId() == TYPEID_PLAYER)
    {
        // Base value
        DoneAdvertisedBenefit += ToPlayer()->GetBaseSpellPowerBonus();

        // Damage bonus from stats
        AuraEffectList const& mDamageDoneOfStatPercent = GetAuraEffectsByType(SPELL_AURA_MOD_SPELL_DAMAGE_OF_STAT_PERCENT);
        for (AuraEffectList::const_iterator i = mDamageDoneOfStatPercent.begin(); i != mDamageDoneOfStatPercent.end(); ++i)
        {
            if ((*i)->GetMiscValue() & schoolMask)
            {
                // stat used stored in miscValueB for this aura
                Stats usedStat = Stats((*i)->GetMiscValueB());
                DoneAdvertisedBenefit += int32(GetStat(usedStat) * (*i)->GetAmount() / 100.0f);
            }
        }
        // ... and attack power
        AuraEffectList const& mDamageDonebyAP = GetAuraEffectsByType(SPELL_AURA_MOD_SPELL_DAMAGE_OF_ATTACK_POWER);
        for (AuraEffectList::const_iterator i =mDamageDonebyAP.begin(); i != mDamageDonebyAP.end(); ++i)
            if ((*i)->GetMiscValue() & schoolMask)
                DoneAdvertisedBenefit += int32(GetTotalAttackPowerValue(BASE_ATTACK) * (*i)->GetAmount() / 100.0f);

        // TODO this should modify PLAYER_FIELD_MOD_SPELL_POWER_PCT instead of all the separate power fields
        int32 spModPct = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_SPELL_POWER_PCT);
        // should it apply to non players as well?
        DoneAdvertisedBenefit += DoneAdvertisedBenefit * spModPct / 100;
    }
    return DoneAdvertisedBenefit > 0 ? DoneAdvertisedBenefit : 0;
}

int32 Unit::SpellBaseDamageBonusTaken(SpellSchoolMask schoolMask) const
{
    int32 TakenAdvertisedBenefit = 0;

    // ..done (for creature type by mask) in taken
    AuraEffectList const& mDamageDoneCreature = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_DONE_CREATURE);
    for (AuraEffectList::const_iterator i = mDamageDoneCreature.begin(); i != mDamageDoneCreature.end(); ++i)
        if (GetCreatureTypeMask() & uint32((*i)->GetMiscValue()))
            TakenAdvertisedBenefit += (*i)->GetAmount();

    // ..taken
    AuraEffectList const& mDamageTaken = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_TAKEN);
    for (AuraEffectList::const_iterator i = mDamageTaken.begin(); i != mDamageTaken.end(); ++i)
        if (((*i)->GetMiscValue() & schoolMask) != 0)
            TakenAdvertisedBenefit += (*i)->GetAmount();

    return TakenAdvertisedBenefit > 0 ? TakenAdvertisedBenefit : 0;
}

bool Unit::IsSpellCrit(Unit* victim, SpellEntry const* spellProto, SpellSchoolMask schoolMask, WeaponAttackType attackType) const
{
    return roll_chance_f(GetUnitSpellCriticalChance(victim, spellProto, schoolMask, attackType));
}

float Unit::GetUnitSpellCriticalChance(Unit *pVictim, SpellEntry const *spellProto, SpellSchoolMask schoolMask, WeaponAttackType attackType) const
{
    // Mobs can't crit with spells.
    if (GetTypeId() == TYPEID_UNIT && !(GetOwner() && GetOwner()->GetTypeId() == TYPEID_PLAYER))
        return 0.0f;

    // not critting spell
    if ((spellProto->AttributesEx2 & SPELL_ATTR2_CANT_CRIT))
        return 0.0f;

    float crit_chance = 0.0f;
    switch(spellProto->DmgClass)
    {
        case SPELL_DAMAGE_CLASS_NONE:
            // We need more spells to find a general way (if there is any)
            switch (spellProto->Id)
            {
                case 379:   // Earth Shield
                case 33778: // Lifebloom Final Bloom
                case 64844: // Divine Hymn
                case 85222: // Light of Dawn
                case 94286: // Protector of the Innocent r1
                case 94288: // Protector of the Innocent r2
                case 94289: // Protector of the Innocent r3
                    break;
                default:
                    return 0.0f;
            }
        case SPELL_DAMAGE_CLASS_MAGIC:
        {
            if (schoolMask & SPELL_SCHOOL_MASK_NORMAL)
                crit_chance = 0.0f;
            // For other schools
            else if (GetTypeId() == TYPEID_PLAYER)
                crit_chance = GetFloatValue(PLAYER_SPELL_CRIT_PERCENTAGE1 + GetFirstSchoolInMask(schoolMask));
            else
            {
                crit_chance = (float)m_baseSpellCritChance;
                crit_chance += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_SPELL_CRIT_CHANCE_SCHOOL, schoolMask);
            }
            // Calculate increased critical strike chance for specific spells by relevant auras
            if (spellProto)
            {
                AuraEffectList const& critAuras = pVictim->GetAuraEffectsByType(SPELL_AURA_MOD_CRITICAL_CHANCE_SPECIFIC);
                for (AuraEffectList::const_iterator i = critAuras.begin(); i != critAuras.end(); ++i)
                    if ((*i)->GetCasterGUID() == GetGUID() && (*i)->IsAffectedOnSpell(spellProto))
                        crit_chance += (*i)->GetAmount();
            }
            // taken
            if (pVictim)
            {
                if (!IsPositiveSpell(spellProto->Id))
                {
                    // Modify critical chance by victim SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE
                    crit_chance += pVictim->GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE, schoolMask);
                    // Modify critical chance by victim SPELL_AURA_MOD_ATTACKER_SPELL_AND_WEAPON_CRIT_CHANCE
                    crit_chance += pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_SPELL_AND_WEAPON_CRIT_CHANCE);
                }
                // scripted (increase crit chance ... against ... target by x%
                AuraEffectList const& mOverrideClassScript = GetAuraEffectsByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
                for (AuraEffectList::const_iterator i = mOverrideClassScript.begin(); i != mOverrideClassScript.end(); ++i)
                {
                    if (!((*i)->IsAffectedOnSpell(spellProto)))
                        continue;

                    switch((*i)->GetMiscValue())
                    {
                        // Shatter
                        case  911:
                            if (!pVictim->HasAuraState(AURA_STATE_FROZEN, spellProto, this))
                                break;
                            if ((*i)->GetSpellProto())
                            {
                                if ((*i)->GetSpellProto()->Id == 11170)
                                    crit_chance *= 2;
                                else if ((*i)->GetSpellProto()->Id == 12982)
                                    crit_chance *= 3;
                            }
                            break;
                        case 7997: // Renewed Hope
                        case 7998:
                            if (pVictim->HasAura(6788))
                                crit_chance+=(*i)->GetAmount();
                            break;
                        case   21: // Test of Faith
                        case 6935:
                        case 6918:
                            if (pVictim->HealthBelowPct(50))
                                crit_chance+=(*i)->GetAmount();
                            break;
                        default:
                            break;
                    }
                }
                // Custom crit by class
                switch (spellProto->SpellFamilyName)
                {
                    case SPELLFAMILY_DRUID:
                        // cumulative effect - don't break

                        // Starfire
                        if (spellProto->SpellFamilyFlags[0] & 0x4 && spellProto->SpellIconID == 1485)
                        {
                            // Improved Insect Swarm
                            if (AuraEffect const * aurEff = GetDummyAuraEffect(SPELLFAMILY_DRUID, 1771, 0))
                                if (pVictim->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DRUID, 0x00000002, 0, 0))
                                    crit_chance+=aurEff->GetAmount();
                           break;
                        }
                    break;
                    case SPELLFAMILY_ROGUE:
                        // Shiv-applied poisons can't crit
                        if (FindCurrentSpellBySpellId(5938))
                            crit_chance = 0.0f;
                        break;
                    case SPELLFAMILY_PALADIN:
                        // Exorcism
                        if (spellProto->Category == 19)
                        {
                            if (pVictim->GetCreatureTypeMask() & CREATURE_TYPEMASK_DEMON_OR_UNDEAD)
                                return 100.0f;
                            break;
                        }
                        // Word of Glory (Last Word talent)
                        else if (spellProto->Id == 85673)
                        {
                            if (pVictim->GetHealthPct() < 35.0f)
                            {
                                if (HasAura(20234))
                                    crit_chance += 30.0f;
                                else if (HasAura(20235))
                                    crit_chance += 60.0f;
                            }
                        }
                    break;
                    case SPELLFAMILY_SHAMAN:
                        // Lava Burst
                        if (spellProto->Id == 51505 || spellProto->Id == 77451) // Original or "fake" Lava burst triggered from Elemental Overload
                        {
                            if (pVictim->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_SHAMAN, 0x10000000, 0,0, GetGUID()))
                                if (pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_SPELL_AND_WEAPON_CRIT_CHANCE) > -100)
                                    return 100.0f;
                            break;
                        }
                    break;
                    case SPELLFAMILY_PRIEST:
                        // remove Mind Spike debuff after casting Mind Blast
                        if (spellProto->Id == 8092)
                            pVictim->RemoveAurasDueToSpell(87178, GetGUID());
                    break;
                    case SPELLFAMILY_WARLOCK:
                        // Searing Pain
                        if (spellProto->Id == 5676 && ToPlayer())
                        {
                            // Soulburn proficiency
                            if (HasAura(74434) && HasAura(79440))
                            {
                                if (Aura* pAura = GetAura(79440))
                                {
                                    if (pAura->GetCharges() == 2)
                                    {
                                        pAura->SetCharges(1);
                                        return true;
                                    }
                                }
                            }

                            // Improved Searing Pain
                            if (HasAura(17927) && pVictim->GetHealthPct() <= 50.0f) // rank 1
                                crit_chance += 20.0f;
                            if (HasAura(17929) && pVictim->GetHealthPct() <= 50.0f) // rank 2
                                crit_chance += 40.0f;
                        }
                        // Hand of Gul'dan increases crit chance of warlocks minions
                        if (pVictim->HasAura(86000) && GetTypeId() == TYPEID_UNIT)
                        {
                            Player* pOwner = GetCharmerOrOwnerPlayerOrPlayerItself();
                            if (pOwner && pOwner->getClass() == CLASS_WARLOCK)
                                crit_chance += 10.0f;
                        }
                    break;
                }
            }
            break;
        }
        case SPELL_DAMAGE_CLASS_MELEE:
            if (pVictim)
            {
                // Custom crit by class
                switch(spellProto->SpellFamilyName)
                {
                    case SPELLFAMILY_DRUID:
                        // Rend and Tear - bonus crit chance for Ferocious Bite on bleeding targets
                        if (spellProto->SpellFamilyFlags[0] & 0x00800000
                            && spellProto->SpellIconID == 1680
                            && pVictim->HasAuraState(AURA_STATE_BLEEDING))
                        {
                            if (AuraEffect const *rendAndTear = GetDummyAuraEffect(SPELLFAMILY_DRUID, 2859, 1))
                                crit_chance += rendAndTear->GetAmount();
                            break;
                        }
                        // Ravage
                        else if (spellProto->Id == 6785 || spellProto->Id == 81170)
                        {
                            // Predatory Strikes talent
                            if (pVictim && pVictim->GetHealthPct() > 80.0f)
                            {
                                if (HasAura(16974))       // Rank 2
                                    crit_chance += 50.0f;
                                else if (HasAura(16972))  // Rank 1
                                    crit_chance += 25.0f;
                            }
                        }
                    break;
                    case SPELLFAMILY_PALADIN:
                    {
                        // Judgement of Command proc always crits on stunned target
                        if (spellProto->SpellFamilyName == SPELLFAMILY_PALADIN)
                        if (spellProto->SpellFamilyFlags[0] & 0x0000000000800000LL && spellProto->SpellIconID == 561)
                        if (pVictim->HasUnitState(UNIT_STATE_STUNNED))
                            return 100.0f;
                        break;
                    }
                    case SPELLFAMILY_HUNTER:
                    {
                        if (spellProto->Id == 83381) // Kill Command 
                        {
                            Unit * owner = GetOwner();

                            if (owner && owner->GetTypeId() == TYPEID_PLAYER)
                            {
                                // Improved Kill Command 
                                if (owner->HasAura(35029))       // Rank 1
                                    crit_chance += 5.0f;
                                else if (owner->HasAura(35030))  // Rank 2
                                    crit_chance += 10.0f;
                            }
                        }
                        break;
                    }

                }
            }
        case SPELL_DAMAGE_CLASS_RANGED:
        {
            if (pVictim)
            {
                crit_chance += GetUnitCriticalChance(attackType, pVictim, spellProto);
                crit_chance += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_SPELL_CRIT_CHANCE_SCHOOL, schoolMask);
            }
            break;
        }
        default:
            return 0.0f;
    }
    // percent done
    // only players use intelligence for critical chance computations
    if (Player* modOwner = GetSpellModOwner())
        modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_CRITICAL_CHANCE, crit_chance);

    // debug mode for spell crits (it's fine thing to debug spellcrit procs)
    if (sObjectMgr->GetSpellCritDebug())
        return 100.0f;

    return crit_chance > 0.0f ? crit_chance : 0.0f;
}

// After all healing was computed -> after all bonuses (done,taken,crit,absorb,...)
uint32 Unit::AfterAllSpellHealingCalculation(SpellEntry const *spellProto, uint32 heal, Unit *pVictim)
{
    if (spellProto == NULL || !pVictim)
        return heal;

    Unit * caster = this;

    /**** Mastery System for healing spells**************/

    if (Player *player = caster->ToPlayer())
    {
        if (player->HasMastery() && heal > 1)
        {
            BranchSpec playerSpec = player->GetActiveTalentBranchSpec();

            // Implementation of Echo of Light mastery proficiency
            if (spellProto->SpellFamilyName == SPELLFAMILY_PRIEST &&
                playerSpec == SPEC_PRIEST_HOLY)
            {
                // Echo of Light HoT effect
                int32 bp0 = heal*(player->GetMasteryPoints()*1.25f/100.0f);

                // stack with old aura
                if (Aura* echo = pVictim->GetAura(77489))
                    if (AuraEffect* hoteff = echo->GetEffect(EFFECT_0))
                        bp0 += hoteff->GetAmount()*((float)(hoteff->GetTotalTicks()-hoteff->GetTickNumber())/(float)hoteff->GetTotalTicks());

                caster->CastCustomSpell(pVictim, 77489, &bp0, NULL, NULL, true);
            }

            // Implementation of Illuminated Healing mastery proficiency
            if (spellProto->SpellFamilyName == SPELLFAMILY_PALADIN &&
                playerSpec == SPEC_PALADIN_HOLY)
            {
                // Illuminated Healing absorb value and spellcast
                int32 bp0 = heal*(player->GetMasteryPoints()*1.5f/100.0f);

                // "The total absorption created can never exceed 1/3 of the casting paladin�s health."
                if (bp0 > caster->GetHealth()*0.33f)
                    bp0 = caster->GetHealth()*0.33f;

                if (Aura* pAura = pVictim->GetAura(86273, caster->GetGUID()))
                {
                    if (AuraEffect* pEff = pAura->GetEffect(EFFECT_0))
                    {
                        bp0 += pEff->GetAmount();
                        if (bp0 > caster->GetHealth()*0.33f)
                            bp0 = caster->GetHealth()*0.33f;

                        pEff->SetAmount(bp0);
                        pAura->SetNeedClientUpdateForTargets();
                        pAura->SetDuration(pAura->GetMaxDuration());
                    }
                }
                else
                    caster->CastCustomSpell(pVictim, 86273, &bp0, 0, 0, true);
            }
        }
    }
    return heal;
}

// After all damage was computed -> after all bonuses (done,taken,crit,absorb,resil,block,...)
uint32 Unit::AfterAllSpellDamageComputation(SpellEntry const *spellProto, uint32 damage, Unit *pVictim)
{
    if (spellProto == NULL)
        return damage;

    Unit * caster = this;
    Unit * unitTarget = pVictim;

    if (!caster || !pVictim)
        return damage;

    // Some special cases after damage recount
    switch (spellProto->Id)
    {
        case 879:
        {
            // Glyph of Exorcism (remember damage which direct damage of Exorcism done)
            if (AuraEffect * aurEff = caster->GetAuraEffect(54934, EFFECT_0))
                aurEff->SetScriptedAmount(damage);
            break;
        }
        case 44614:
        {
            // Glyph of Frostfire (remember damage which direct damage of Frostfire Bolt done)
            if (AuraEffect * aurEff = caster->GetAuraEffect(61205, EFFECT_2))
                aurEff->SetScriptedAmount(damage * aurEff->GetAmount() / 100);
            break;
        }
        // Blood Boil
        case 48721:
        {
            // bonus for diseased targets
            if (unitTarget->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DEATHKNIGHT, 0, 0, 0x00000002, caster->GetGUID()))
                damage += damage / 2;
            break;
        }
        case 8092:  // Mind Blast
        case 73510: // Mind Spike
        {
            // Increase damage done by every shadow orb stack
            if (Aura* pOrbs = caster->GetAura(77487))
                if (pOrbs->GetStackAmount() > 0)
                {
                    // 10% base bonus
                    float coef = 0.1f;
                    int32 es_bp0 = 10;
                    int32 es_bp1 = 0;

                    // Implementation of Shadow Orbs Power mastery proficiency
                    if (spellProto->SpellFamilyName == SPELLFAMILY_PRIEST &&
                        caster->ToPlayer() && caster->ToPlayer()->HasMastery() &&
                        caster->ToPlayer()->GetTalentBranchSpec(caster->ToPlayer()->GetActiveSpec()) == SPEC_PRIEST_SHADOW &&
                        damage > 1)
                    {
                        coef += caster->ToPlayer()->GetMasteryPoints() * 1.45f / 100.0f;
                        es_bp0 += ceil(caster->ToPlayer()->GetMasteryPoints()*1.45f);
                    }
                    damage = float(damage) * (1.0f + (pOrbs->GetStackAmount() * coef));

                    es_bp1 = es_bp0;

                    // Empowered Shadows buff for increased DoT damage
                    caster->CastCustomSpell(caster, 95799, &es_bp0, &es_bp1, 0, true);

                    // Consume Shadow Orbs
                    caster->RemoveAurasDueToSpell(77487);
                }
            break;
        }
        // Shadow Word: Death
        case 32379:
        {
            // Target is not killed
            if (damage < unitTarget->GetHealth())
            {
                // Deals damage equal to damage done to caster
                int32 back_damage = damage;
                // Pain and Suffering reduces damage
                if (AuraEffect * aurEff = caster->GetDummyAuraEffect(SPELLFAMILY_PRIEST, 2874, 1))
                    back_damage += aurEff->GetAmount() * back_damage / 100; // += due to - basepoint in spelleffect
                caster->CastCustomSpell(caster, 32409, &back_damage, 0, 0, true);

                if (unitTarget->GetHealthPct() <= 25.0f)
                {
                    // Glyph of Shadow Word: Death
                    if (!caster->ToPlayer()->HasSpellCooldown(55682) && caster->HasAura(55682))
                        caster->CastSpell(caster, 77691, true); // dummy hack!
                }
            }
            break;
        }
        // Frostbolt (mage)
        case 116:
        {
            // Frostbolt is involved in some other things, so we must ensure that mage is the caster
            if (!unitTarget || caster->getClass() != CLASS_MAGE)
                break;

            // talent Shatter - target must be frozen
            if (unitTarget->HasAuraState(AURA_STATE_FROZEN, spellProto, caster))
            {
                if (caster->HasAura(11170))
                    damage *= 1.1f;
                else if (caster->HasAura(12982))
                    damage *= 1.2f;
            }
            break;
        }
        // Incinerate (warlock)
        case 29722:
        {
            if (caster->getClass() != CLASS_WARLOCK)
                break;

            // If the target is affected by Immolate spell, let's increase damage by 1/6 (info by DBC tooltip)
            if (unitTarget->HasAura(348))
                damage += damage / 6;
            break;
        }
        // Soul Fire
        case 6353:
        {
            // Improved Soul Fire talent
            int32 bp0 = 0;
            if (caster->HasAura(18119))
                bp0 = 4;
            else if (caster->HasAura(18120))
                bp0 = 8;

            if (bp0)
                caster->CastCustomSpell(caster, 85383, &bp0, 0, 0, true);

            // Burning Embers talent
            bp0 = 0;
            if (caster->HasAura(85112))
                bp0 = 50;
            else if (caster->HasAura(91986))
                bp0 = 25;

            // We must calculate maximal basepoints manually
            int32 maxbp = caster->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_SPELL);
            if (bp0 == 50)
                maxbp *= 1.4f;
            else
                maxbp *= 0.7f;
            SpellEntry const* pSpell = sSpellStore.LookupEntry((bp0 == 50)?85112:((bp0 == 25)?91986:0));
            if (pSpell)
            {
                // Burning Embers inflicts 25/50% of damage dealt OR damage based by formula
                // - the lower of them
                SpellScaling pScaling;
                pScaling.Init(caster->getLevel(), pSpell);
                maxbp = (maxbp + pScaling.avg[1])/7;
                bp0 *= (damage/100)/7;
                if (bp0 > maxbp)
                    bp0 = maxbp;
                // If we already have aura like this, only refresh if damage is lower
                if (unitTarget->HasAura(85421) && unitTarget->GetAura(85421)->GetEffect(0) &&
                    unitTarget->GetAura(85421)->GetEffect(0)->GetAmount() >= bp0)
                    unitTarget->GetAura(85421)->RefreshDuration();
                else
                    caster->CastCustomSpell(unitTarget, 85421, &bp0, 0, 0, true);
            }
            break;
        }
        default:
            break;
    }

    // Implementation of demonolgy warlock mastery Master Demonologist proficiency
    if (spellProto->SpellFamilyName == SPELLFAMILY_WARLOCK && caster->IsPet())
    {
        Player* pOwner = caster->GetCharmerOrOwnerPlayerOrPlayerItself();
        if (pOwner && pOwner->HasMastery() &&
            pOwner->GetTalentBranchSpec(pOwner->GetActiveSpec()) == SPEC_WARLOCK_DEMONOLOGY)
        {
            damage = damage * (1+(pOwner->GetMasteryPoints()*2.3f/100.0f));
        }
    }

    if (spellProto->SpellFamilyName == SPELLFAMILY_MAGE)
    {
        // Implementation of Mana Adept mage mastery proficiency
        if (caster->ToPlayer() && caster->ToPlayer()->HasMastery() &&
            caster->ToPlayer()->GetTalentBranchSpec(caster->ToPlayer()->GetActiveSpec()) == SPEC_MAGE_ARCANE &&
            damage > 1)
        {
            // Get mana percentage (0.0f - 1.0f)
            float manapct = float(caster->GetPower(POWER_MANA)) / float(caster->GetMaxPower(POWER_MANA));
            // multiplier formula: 1 + (mastery*1.5*(%mana remain)/100)
            damage = damage*(1.0f+caster->ToPlayer()->GetMasteryPoints()*1.5f*manapct/100.0f);
        }

        // Implementation of Frostburn mage mastery proficiency
        if (caster->ToPlayer() && caster->ToPlayer()->HasMastery() &&
            caster->ToPlayer()->GetTalentBranchSpec(caster->ToPlayer()->GetActiveSpec()) == SPEC_MAGE_FROST &&
            damage > 1)
        {
            if (unitTarget->HasAuraState(AURA_STATE_FROZEN, spellProto, caster))
                damage = damage*(1.0f+caster->ToPlayer()->GetMasteryPoints()*2.5f/100.0f);
        }
    }

    return damage;
}

uint32 Unit::SpellCriticalDamageBonus(SpellEntry const *spellProto, uint32 damage, Unit *pVictim)
{
    // Calculate critical bonus
    int32 crit_bonus;

    Player* modOwner = GetSpellModOwner();
    switch(spellProto->DmgClass)
    {
        // 100% for melee spells
        case SPELL_DAMAGE_CLASS_MELEE:
            crit_bonus = damage;
            break;
        case SPELL_DAMAGE_CLASS_RANGED:
            // 100% for ranged spell with SPELL_SCHOOL_MASK_NORMAL
            if ((GetSpellSchoolMask(spellProto) & SPELL_SCHOOL_MASK_NORMAL) || spellProto->Id == 53301)
                crit_bonus = damage;
            else
            // 50% for ranged spells with magic like spell school -> hunter's traps, DoTs (black arrow, serpent sting ...)
                crit_bonus = damage / 2;
            break;
        default:
            if(modOwner && (modOwner->getClass() == CLASS_MAGE || modOwner->getClass() == CLASS_WARLOCK))
                crit_bonus = damage; // 100% bonus for Mages and Warlocks in Cataclysm
            else 
                crit_bonus = damage / 2;                        // for spells is 50%
            break;
    }

    // All healing spells crit for 200% instead of 150%.
    if (spellProto->AttributesEx8 & SPELL_ATTR8_HEALING_SPELL)
        crit_bonus = damage;

    // adds additional damage to crit_bonus (from talents)
    if (modOwner)
        modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_CRIT_DAMAGE_BONUS, crit_bonus);

    if (pVictim)
    {
        uint32 creatureTypeMask = pVictim->GetCreatureTypeMask();
        crit_bonus = int32(crit_bonus * GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_CRIT_PERCENT_VERSUS, creatureTypeMask));
    }

    if (crit_bonus > 0)
        damage += crit_bonus;

    damage = int32(float(damage) * GetTotalAuraMultiplier(SPELL_AURA_MOD_CRIT_DAMAGE_BONUS));

    if (spellProto->Id == 116)
    {
        if (HasAura(83156))
            CastCustomSpell(83154, SPELLVALUE_MAX_TARGETS,1, pVictim, true);// Piercing chill to 1 additional target
        else if (HasAura(83157))
            CastCustomSpell(83154, SPELLVALUE_MAX_TARGETS, 2 , pVictim, true); // Piercing chill to 2 additional target
    }

    return damage;
}

uint32 Unit::SpellCriticalHealingBonus(SpellEntry const *spellProto, uint32 damage, Unit *pVictim)
{
    // Victory Rush - effect 2 - heal should not crit. Cannot be made in other way. Hack.
    if (spellProto->Id == 34428)
        return damage;

    // Calculate critical bonus
    int32 crit_bonus = damage;

    if (pVictim)
    {
        uint32 creatureTypeMask = pVictim->GetCreatureTypeMask();
        crit_bonus = int32(crit_bonus * GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_CRIT_PERCENT_VERSUS, creatureTypeMask));
    }

    if (crit_bonus > 0)
        damage += crit_bonus;

    damage = int32(float(damage) * GetTotalAuraMultiplier(SPELL_AURA_MOD_CRITICAL_HEALING_AMOUNT));

    return damage;
}

uint32 Unit::SpellHealingBonus(Unit *pVictim, SpellEntry const *spellProto, uint32 effIndex, uint32 healamount, DamageEffectType damagetype, uint32 stack)
{
    if (pVictim == NULL)
        return healamount;

    healamount = this->SpellHealingBonusDone(pVictim, spellProto, effIndex, healamount, damagetype, stack);

    healamount = pVictim->SpellHealingBonusTaken(this, spellProto, effIndex, healamount, damagetype, stack);

    return healamount;
}

// TODO -> Remove this later
/*uint32 Unit::SpellHealingBonus(Unit *pVictim, SpellEntry const *spellProto, uint32 effIndex, uint32 healamount, DamageEffectType damagetype, uint32 stack)
{
    // For totems get healing bonus from owner (statue isn't totem in fact)
    if (GetTypeId() == TYPEID_UNIT && this->ToCreature()->IsTotem())
        if (Unit* owner = GetOwner())
            return owner->SpellHealingBonus(pVictim, spellProto, effIndex, healamount, damagetype, stack);

    // no bonus for heal potions/bandages
    if (spellProto->SpellFamilyName == SPELLFAMILY_POTION)
        return healamount;
    // and Warlock's Healthstones
    if (spellProto->SpellFamilyName == SPELLFAMILY_WARLOCK && (spellProto->SpellFamilyFlags[0] & 0x10000))
        return healamount;

    // Healing Done
    // Taken/Done total percent damage auras
    float  DoneTotalMod = 1.0f;
    float  TakenTotalMod = 1.0f;
    int32  DoneTotal = 0;
    int32  TakenTotal = 0;

    // Healing done percent
    AuraEffectList const& mHealingDonePct = GetAuraEffectsByType(SPELL_AURA_MOD_HEALING_DONE_PERCENT);
    for (AuraEffectList::const_iterator i = mHealingDonePct.begin(); i != mHealingDonePct.end(); ++i)
        DoneTotalMod *= (100.0f + (*i)->GetAmount()) / 100.0f;

    // done scripted mod (take it from owner)
    Unit *owner = GetOwner() ? GetOwner() : this;
    AuraEffectList const &mOverrideClassScript= owner->GetAuraEffectsByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
    for (AuraEffectList::const_iterator i = mOverrideClassScript.begin(); i != mOverrideClassScript.end(); ++i)
    {
        if (!(*i)->IsAffectedOnSpell(spellProto))
            continue;
        switch((*i)->GetMiscValue())
        {
            case 4415: // Increased Rejuvenation Healing
            case 4953:
            case 3736: // Hateful Totem of the Third Wind / Increased Lesser Healing Wave / LK Arena (4/5/6) Totem of the Third Wind / Savage Totem of the Third Wind
                DoneTotal+=(*i)->GetAmount();
                break;
            case 7997: // Renewed Hope
            case 7998:
                if (pVictim->HasAura(6788))
                    DoneTotalMod *=((*i)->GetAmount() + 100.0f)/100.0f;
                break;
            case   21: // Test of Faith
            case 6935:
            case 6918:
                if (pVictim->HealthBelowPct(50))
                    DoneTotalMod *=((*i)->GetAmount() + 100.0f)/100.0f;
                break;
            case 8477: // Nourish Heal Boost
            {
                int32 stepPercent = (*i)->GetAmount();
                int32 modPercent = 0;
                AuraApplicationMap const& victimAuras = pVictim->GetAppliedAuras();
                for (AuraApplicationMap::const_iterator itr = victimAuras.begin(); itr != victimAuras.end(); ++itr)
                {
                    Aura const * aura = itr->second->GetBase();
                    if (aura->GetCasterGUID() != GetGUID())
                        continue;
                    SpellEntry const* m_spell = aura->GetSpellProto();
                    if (m_spell->SpellFamilyName != SPELLFAMILY_DRUID ||
                        !(m_spell->SpellFamilyFlags[1] & 0x00000010 || m_spell->SpellFamilyFlags[0] & 0x50))
                        continue;
                    modPercent += stepPercent * aura->GetStackAmount();
                }
                DoneTotalMod *= (modPercent+100.0f)/100.0f;
                break;
            }
            default:
                break;
        }
    }

    // Taken/Done fixed damage bonus auras
    int32 DoneAdvertisedBenefit  = SpellBaseHealingBonusDone(GetSpellSchoolMask(spellProto));
    int32 TakenAdvertisedBenefit = pVictim->SpellBaseHealingBonusTaken(GetSpellSchoolMask(spellProto));

    // ignored now cause DBC should have the right info for nearly all effects
    // delte if all goes well
    bool scripted = false;

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        switch (spellProto->EffectApplyAuraName[i])
        {
            // These auras do not use healing coeff
            case SPELL_AURA_PERIODIC_LEECH:
            case SPELL_AURA_PERIODIC_HEALTH_FUNNEL:
                scripted = true;
                break;
        }
    }

    // Check for table values
    SpellBonusEntry const* bonus = !scripted ? sSpellMgr->GetSpellBonusData(spellProto->Id) : NULL;
    float coeff = 0;
    float factorMod = 1.0f;
    if (bonus)
    {
        if (damagetype == DOT)
        {
            coeff = bonus->dot_damage;
            if (bonus->ap_dot_bonus > 0)
                DoneTotal += int32(bonus->ap_dot_bonus * stack * GetTotalAttackPowerValue(
                    (IsRangedWeaponSpell(spellProto) && spellProto->DmgClass !=SPELL_DAMAGE_CLASS_MELEE)? RANGED_ATTACK : BASE_ATTACK));
        }
        else
        {
            coeff = bonus->direct_damage;
            if (bonus->ap_bonus > 0)
                DoneTotal += int32(bonus->ap_bonus * stack * GetTotalAttackPowerValue(
                    (IsRangedWeaponSpell(spellProto) && spellProto->DmgClass !=SPELL_DAMAGE_CLASS_MELEE)? RANGED_ATTACK : BASE_ATTACK));
        }
    }
    else // scripted bonus
    {
        // Gift of the Naaru
        if (spellProto->SpellFamilyFlags[2] & 0x80000000 && spellProto->SpellIconID == 329)
        {
            scripted = true;
            int32 apBonus = int32(std::max(GetTotalAttackPowerValue(BASE_ATTACK), GetTotalAttackPowerValue(RANGED_ATTACK)));
            if (apBonus > DoneAdvertisedBenefit)
                DoneTotal += int32(apBonus * 0.22f); // 22% of AP per tick
            else
                DoneTotal += int32(DoneAdvertisedBenefit * 0.377f); //37.7% of BH per tick
        }
        // Earthliving - 0.45% of normal hot coeff
        else if (spellProto->SpellFamilyName == SPELLFAMILY_SHAMAN && spellProto->SpellFamilyFlags[1] & 0x80000)
            factorMod *= 0.45f;
        // Already set to scripted? so not uses healing bonus coefficient
        // No heal coeff for SPELL_DAMAGE_CLASS_NONE class spells by default
        else if (scripted || spellProto->DmgClass == SPELL_DAMAGE_CLASS_NONE)
        {
            scripted = true;
            coeff = 0.0f;
        }
    }

    // Default calculation
    if (DoneAdvertisedBenefit || TakenAdvertisedBenefit)
    {
        if(coeff <= 0)
        {
            if(effIndex < 4)
            {
                coeff = spellProto->EffectBonusCoefficient[effIndex];
            }
            else
            {   // should never happen
                coeff = 0;
            }
            
        }
        factorMod *= CalculateLevelPenalty(spellProto)* stack;
        TakenTotal += int32(TakenAdvertisedBenefit * coeff * factorMod);
        if (Player* modOwner = GetSpellModOwner())
        {
            coeff *= 100.0f;
            modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_BONUS_MULTIPLIER, coeff);
            coeff /= 100.0f;
        }
        DoneTotal += int32(DoneAdvertisedBenefit * coeff * factorMod);
    }

    // use float as more appropriate for negative values and percent applying
    float heal = (int32(healamount) + DoneTotal) * DoneTotalMod;
    // apply spellmod to Done amount
    if (Player* modOwner = GetSpellModOwner())
        modOwner->ApplySpellMod(spellProto->Id, damagetype == DOT ? SPELLMOD_DOT : SPELLMOD_DAMAGE, heal);

    // Nourish cast
    if (spellProto->SpellFamilyName == SPELLFAMILY_DRUID && spellProto->SpellFamilyFlags[1] & 0x2000000)
    {
        // Rejuvenation, Regrowth, Lifebloom, or Wild Growth
        if (pVictim->GetAuraEffect(SPELL_AURA_PERIODIC_HEAL, SPELLFAMILY_DRUID, 0x50, 0x4000010, 0))
            //increase healing by 20%
            TakenTotalMod *= 1.2f;
    }

    // Implementation of Deep Healing mastery proficiency
    if (Player *player = ToPlayer())
    {
        if (spellProto->SpellFamilyName == SPELLFAMILY_SHAMAN &&
            player->GetActiveTalentBranchSpec() == SPEC_SHAMAN_RESTORATION)
        {
            // Mastery gives 3% per mastery point
            float masterybonus = player->GetMasteryPoints() * 3.0f / 100.0f;
            // Healing scales linearly down (1% of bonus at 100% health, 100% of bonus at 0% health (hypotheticaly))
            float healthcoef = (100.0f - pVictim->GetHealthPct() * 0.99f) / 100.0f;

            TakenTotalMod *= 1.0f + masterybonus * healthcoef;
        }
    }

    // Taken mods

    // Healing Wave
    if (spellProto->SpellFamilyName == SPELLFAMILY_SHAMAN && spellProto->SpellFamilyFlags[0] & 0x40)
    {
        // Search for Healing Way on Victim
        if (AuraEffect const* HealingWay = pVictim->GetAuraEffect(29203, 0))
            TakenTotalMod *= (HealingWay->GetAmount() + 100.0f) / 100.0f;
    }

    // Tenacity increase healing % taken
    if (AuraEffect const* Tenacity = pVictim->GetAuraEffect(58549, 0))
        TakenTotalMod *= (Tenacity->GetAmount() + 100.0f) / 100.0f;


    // Healing taken percent
    float minval = (float)pVictim->GetMaxNegativeAuraModifier(SPELL_AURA_MOD_HEALING_PCT);
    if (minval)
        TakenTotalMod *= (100.0f + minval) / 100.0f;

    float maxval = (float)pVictim->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_HEALING_PCT);
    if (maxval)
        TakenTotalMod *= (100.0f + maxval) / 100.0f;

    if (damagetype == DOT)
    {
        // Healing over time taken percent
        float minval_hot = (float)pVictim->GetMaxNegativeAuraModifier(SPELL_AURA_MOD_HOT_PCT);
        if (minval_hot)
            TakenTotalMod *= (100.0f + minval_hot) / 100.0f;

        float maxval_hot = (float)pVictim->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_HOT_PCT);
        if (maxval_hot)
            TakenTotalMod *= (100.0f + maxval_hot) / 100.0f;
    }

    AuraEffectList const& mHealingGet= pVictim->GetAuraEffectsByType(SPELL_AURA_MOD_HEALING_RECEIVED);
    for (AuraEffectList::const_iterator i = mHealingGet.begin(); i != mHealingGet.end(); ++i)
        if (GetGUID() == (*i)->GetCasterGUID() && (*i)->IsAffectedOnSpell(spellProto))
            TakenTotalMod *= ((*i)->GetAmount() + 100.0f) / 100.0f;

    heal = (int32(heal) + TakenTotal) * TakenTotalMod;

    return uint32(std::max(heal, 0.0f));
}*/

uint32 Unit::SpellHealingBonusDone(Unit* victim, SpellEntry const* spellProto, uint32 effIndex, uint32 healamount, DamageEffectType damagetype, uint32 stack) const
{
    // For totems get healing bonus from owner (statue isn't totem in fact)
    if (GetTypeId() == TYPEID_UNIT && this->ToCreature()->IsTotem())
        if (Unit* owner = GetOwner())
            return owner->SpellHealingBonusDone(victim, spellProto,effIndex, healamount, damagetype, stack);

    // No bonus healing for potion spells
    if (spellProto->SpellFamilyName == SPELLFAMILY_POTION)
        return healamount;

    // and Warlock's Healthstones
    if (spellProto->SpellFamilyName == SPELLFAMILY_WARLOCK && (spellProto->SpellFamilyFlags[0] & 0x10000))
        return healamount;

    int32 DoneTotal = 0;

    // done scripted mod (take it from owner)
    Unit const* owner = GetOwner() ? GetOwner() : this;
    AuraEffectList const& mOverrideClassScript= owner->GetAuraEffectsByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
    for (AuraEffectList::const_iterator i = mOverrideClassScript.begin(); i != mOverrideClassScript.end(); ++i)
    {
        if (!(*i)->IsAffectedOnSpell(spellProto))
            continue;
        switch ((*i)->GetMiscValue())
        {
            case 4415: // Increased Rejuvenation Healing
            case 4953:
            case 3736: // Hateful Totem of the Third Wind / Increased Lesser Healing Wave / LK Arena (4/5/6) Totem of the Third Wind / Savage Totem of the Third Wind
                DoneTotal += (*i)->GetAmount();
                break;
            default:
                break;
        }
    }

    // Done fixed damage bonus auras
    int32 DoneAdvertisedBenefit = SpellBaseHealingBonusDone(GetSpellSchoolMask(spellProto));

    // Check for table values
    SpellBonusEntry const* bonus = sSpellMgr->GetSpellBonusData(spellProto->Id);
    float coeff = 0;
    float factorMod = 1.0f;
    if (bonus)
    {
        if (damagetype == DOT)
        {
            coeff = bonus->dot_damage;
            if (bonus->ap_dot_bonus > 0)
                DoneTotal += int32(bonus->ap_dot_bonus * stack * GetTotalAttackPowerValue(
                    (IsRangedWeaponSpell(spellProto) && spellProto->DmgClass !=SPELL_DAMAGE_CLASS_MELEE) ? RANGED_ATTACK : BASE_ATTACK));
        }
        else
        {
            coeff = bonus->direct_damage;
            if (bonus->ap_bonus > 0)
                DoneTotal += int32(bonus->ap_bonus * stack * GetTotalAttackPowerValue(
                    (IsRangedWeaponSpell(spellProto) && spellProto->DmgClass !=SPELL_DAMAGE_CLASS_MELEE) ? RANGED_ATTACK : BASE_ATTACK));
        }
    }
    else
    {
        // Earthliving - 0.45% of normal hot coeff
        if (spellProto->SpellFamilyName == SPELLFAMILY_SHAMAN && spellProto->SpellFamilyFlags[1] & 0x80000)
            factorMod *= 0.45f;

        // No bonus healing for SPELL_DAMAGE_CLASS_NONE class spells by default
        if (spellProto->DmgClass == SPELL_DAMAGE_CLASS_NONE)
            coeff = 0.0f;
    }

    // Default calculation
    if (DoneAdvertisedBenefit)
    {
        if(coeff <= 0 || !bonus)
        {
            if(effIndex < MAX_SPELL_EFFECTS)
                coeff = spellProto->EffectBonusCoefficient[effIndex]; // Spell power coeficient
            else
                coeff = 0; // should never happen
        }

        factorMod *= CalculateLevelPenalty(spellProto) * stack;

        if (Player* modOwner = GetSpellModOwner())
        {
            coeff *= 100.0f;
            modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_BONUS_MULTIPLIER, coeff);
            coeff /= 100.0f;
        }

        DoneTotal += int32(DoneAdvertisedBenefit * coeff * factorMod);
    }

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        switch (spellProto->EffectApplyAuraName[i])
        {
            // Bonus healing does not apply to these spells
            case SPELL_AURA_PERIODIC_LEECH:
            case SPELL_AURA_PERIODIC_HEALTH_FUNNEL:
                DoneTotal = 0;
                break;
        }
        if (spellProto->Effect[i] == SPELL_EFFECT_HEALTH_LEECH)
            DoneTotal = 0;
    }

    // Done Percentage for DOT is already calculated, no need to do it again. The percentage mod is applied in Aura::HandleAuraSpecificMods.
    float heal = float(int32(healamount) + DoneTotal) * (damagetype == DOT ? 1.0f : SpellHealingPctDone(victim, spellProto));
    // apply spellmod to Done amount
    if (Player* modOwner = GetSpellModOwner())
        modOwner->ApplySpellMod(spellProto->Id, damagetype == DOT ? SPELLMOD_DOT : SPELLMOD_DAMAGE, heal);

    return uint32(std::max(heal, 0.0f));
}

float Unit::SpellHealingPctDone(Unit* victim, SpellEntry const* spellProto) const
{
    // For totems pct done mods are calculated when its calculation is run on the player in SpellHealingBonusDone.
    if (GetTypeId() == TYPEID_UNIT && IsTotem())
        return 1.0f;

    // No bonus healing for potion spells
    if (spellProto->SpellFamilyName == SPELLFAMILY_POTION)
        return 1.0f;

    // and Warlock's Healthstones
    if (spellProto->SpellFamilyName == SPELLFAMILY_WARLOCK && (spellProto->SpellFamilyFlags[0] & 0x10000))
        return 1.0f;

    float DoneTotalMod = 1.0f;

    // Healing done percent
    AuraEffectList const& mHealingDonePct = GetAuraEffectsByType(SPELL_AURA_MOD_HEALING_DONE_PERCENT);
    for (AuraEffectList::const_iterator i = mHealingDonePct.begin(); i != mHealingDonePct.end(); ++i)
        AddPctN(DoneTotalMod, (*i)->GetAmount());

    // done scripted mod (take it from owner)
    Unit const* owner = GetOwner() ? GetOwner() : this;
    AuraEffectList const& mOverrideClassScript= owner->GetAuraEffectsByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
    for (AuraEffectList::const_iterator i = mOverrideClassScript.begin(); i != mOverrideClassScript.end(); ++i)
    {
        if (!(*i)->IsAffectedOnSpell(spellProto))
            continue;
        switch ((*i)->GetMiscValue())
        {
            case   21: // Test of Faith
            case 6935:
            case 6918:
                if (victim->HealthBelowPct(50))
                    AddPctN(DoneTotalMod, (*i)->GetAmount());
                break;
            case 7798: // Glyph of Regrowth
            {
                if (victim->GetAuraEffect(SPELL_AURA_PERIODIC_HEAL, SPELLFAMILY_DRUID, 0x40, 0, 0))
                    AddPctN(DoneTotalMod, (*i)->GetAmount());
                break;
            }
            case 7997: // Renewed Hope
            case 7998:
                if (victim->HasAura(6788))
                    DoneTotalMod *=((*i)->GetAmount() + 100.0f)/100.0f;
                break;
            case 8477: // Nourish Heal Boost
            {
                int32 stepPercent = (*i)->GetAmount();
                int32 modPercent = 0;
                AuraApplicationMap const& victimAuras = victim->GetAppliedAuras();
                for (AuraApplicationMap::const_iterator itr = victimAuras.begin(); itr != victimAuras.end(); ++itr)
                {
                    Aura const* aura = itr->second->GetBase();
                    if (aura->GetCasterGUID() != GetGUID())
                        continue;
                    SpellEntry const* m_spell = aura->GetSpellProto();
                    if (m_spell->SpellFamilyName != SPELLFAMILY_DRUID ||
                        !(m_spell->SpellFamilyFlags[1] & 0x00000010 || m_spell->SpellFamilyFlags[0] & 0x50))
                        continue;
                    modPercent += stepPercent * aura->GetStackAmount();
                }
                AddPctN(DoneTotalMod, modPercent);
                break;
            }
            default:
                break;
        }
    }

    return DoneTotalMod;
}

uint32 Unit::SpellHealingBonusTaken(Unit* caster, SpellEntry const* spellProto, uint32 effIndex, uint32 healamount, DamageEffectType damagetype, uint32 stack) const
{
    float TakenTotalMod = 1.0f;

    // Healing taken percent
    float minval = float(GetMaxNegativeAuraModifier(SPELL_AURA_MOD_HEALING_PCT));
    if (minval)
        AddPctN(TakenTotalMod, minval);

    float maxval = float(GetMaxPositiveAuraModifier(SPELL_AURA_MOD_HEALING_PCT));
    if (maxval)
        AddPctN(TakenTotalMod, maxval);

    // Tenacity increase healing % taken
    if (AuraEffect const* Tenacity = GetAuraEffect(58549, 0))
        AddPctN(TakenTotalMod, Tenacity->GetAmount());

    // Healing Done
    int32 TakenTotal = 0;

    // Taken fixed damage bonus auras
    int32 TakenAdvertisedBenefit = SpellBaseHealingBonusTaken(GetSpellSchoolMask(spellProto));

    // Nourish cast
    if (spellProto->SpellFamilyName == SPELLFAMILY_DRUID && spellProto->SpellFamilyFlags[1] & 0x2000000)
    {
        // Rejuvenation, Regrowth, Lifebloom, or Wild Growth
        if (GetAuraEffect(SPELL_AURA_PERIODIC_HEAL, SPELLFAMILY_DRUID, 0x50, 0x4000010, 0))
            // increase healing by 20%
            TakenTotalMod *= 1.2f;
    }

    if (damagetype == DOT)
    {
        // Healing over time taken percent
        float minval_hot = float(GetMaxNegativeAuraModifier(SPELL_AURA_MOD_HOT_PCT));
        if (minval_hot)
            AddPctN(TakenTotalMod, minval_hot);

        float maxval_hot = float(GetMaxPositiveAuraModifier(SPELL_AURA_MOD_HOT_PCT));
        if (maxval_hot)
            AddPctN(TakenTotalMod, maxval_hot);
    }

    // Check for table values
    SpellBonusEntry const* bonus = sSpellMgr->GetSpellBonusData(spellProto->Id);
    float coeff = 0;
    float factorMod = 1.0f;
    if (bonus)
        coeff = (damagetype == DOT) ? bonus->dot_damage : bonus->direct_damage;
    else
    {
        // No bonus healing for SPELL_DAMAGE_CLASS_NONE class spells by default
        if (spellProto->DmgClass == SPELL_DAMAGE_CLASS_NONE)
          coeff = 0.0f;
    }

    // Default calculation
    if (TakenAdvertisedBenefit)
    {
        if(coeff <= 0 || !bonus)
        {
            if(effIndex < MAX_SPELL_EFFECTS)
                coeff = spellProto->EffectBonusCoefficient[effIndex]; // Spell power coeficient
            else
                coeff = 0.0f; // should never happen
        }

        factorMod *= CalculateLevelPenalty(spellProto) * int32(stack);
        if (Player* modOwner = GetSpellModOwner())
        {
            coeff *= 100.0f;
            modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_BONUS_MULTIPLIER, coeff);
            coeff /= 100.0f;
        }

        // Earthliving - 0.45% of normal hot coeff
        if (spellProto->SpellFamilyName == SPELLFAMILY_SHAMAN && spellProto->SpellFamilyFlags[1] & 0x80000)
            factorMod *= 0.45f;

        TakenTotal += int32(TakenAdvertisedBenefit * coeff * factorMod);
    }

    AuraEffectList const& mHealingGet= GetAuraEffectsByType(SPELL_AURA_MOD_HEALING_RECEIVED);
    for (AuraEffectList::const_iterator i = mHealingGet.begin(); i != mHealingGet.end(); ++i)
        if (caster->GetGUID() == (*i)->GetCasterGUID() && (*i)->IsAffectedOnSpell(spellProto))
            AddPctN(TakenTotalMod, (*i)->GetAmount());

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        switch (spellProto->EffectApplyAuraName[i])
        {
            // Bonus healing does not apply to these spells
            case SPELL_AURA_PERIODIC_LEECH:
            case SPELL_AURA_PERIODIC_HEALTH_FUNNEL:
                TakenTotal = 0;
                break;
        }
        if (spellProto->Effect[i] == SPELL_EFFECT_HEALTH_LEECH)
            TakenTotal = 0;
    }

    // Implementation of Deep Healing mastery proficiency
    if (const Player *player = caster->ToPlayer())
    {
        if (spellProto->SpellFamilyName == SPELLFAMILY_SHAMAN &&
            player->GetActiveTalentBranchSpec() == SPEC_SHAMAN_RESTORATION)
        {
            // Mastery gives 3% per mastery point
            float masterybonus = player->GetMasteryPoints() * 3.0f / 100.0f;
            // Healing scales linearly down (1% of bonus at 100% health, 100% of bonus at 0% health (hypotheticaly))
            float healthcoef = (100.0f - GetHealthPct() * 0.99f) / 100.0f;

            TakenTotalMod *= 1.0f + masterybonus * healthcoef;
        }
    }

    float heal = float(int32(healamount) + TakenTotal) * TakenTotalMod;

    return uint32(std::max(heal, 0.0f));
}

int32 Unit::SpellBaseHealingBonusDone(SpellSchoolMask schoolMask) const
{
    // SPELL_AURA_OVERRIDE_SPELL_POWER_BY_AP_PCT:
    // Your spell power is now equal to ??% of your attack power, and you no longer benefit from other sources of spell power
    AuraEffectList const& mDamageDonebyAP = GetAuraEffectsByType(SPELL_AURA_OVERRIDE_SPELL_POWER_BY_AP_PCT);
    if (!mDamageDonebyAP.empty())
    {
        float AP = GetTotalAttackPowerValue(BASE_ATTACK);
        int32 amount = 0;
        for (AuraEffectList::const_iterator i =mDamageDonebyAP.begin(); i != mDamageDonebyAP.end(); ++i)
            if ((*i)->GetMiscValue() & schoolMask)
                amount += (*i)->GetAmount();

        return  uint32(AP * amount / 100.0f);
    }


    int32 AdvertisedBenefit = 0;

    AuraEffectList const& mHealingDone = GetAuraEffectsByType(SPELL_AURA_MOD_HEALING_DONE);
    for (AuraEffectList::const_iterator i = mHealingDone.begin(); i != mHealingDone.end(); ++i)
        if (!(*i)->GetMiscValue() || ((*i)->GetMiscValue() & schoolMask) != 0)
            AdvertisedBenefit += (*i)->GetAmount();

    // Healing bonus of spirit, intellect and strength
    if (GetTypeId() == TYPEID_PLAYER)
    {
        // Base value
        AdvertisedBenefit +=this->ToPlayer()->GetBaseSpellPowerBonus();

        // Healing bonus from stats
        AuraEffectList const& mHealingDoneOfStatPercent = GetAuraEffectsByType(SPELL_AURA_MOD_SPELL_HEALING_OF_STAT_PERCENT);
        for (AuraEffectList::const_iterator i = mHealingDoneOfStatPercent.begin(); i != mHealingDoneOfStatPercent.end(); ++i)
        {
            // stat used dependent from misc value (stat index)
            Stats usedStat = Stats((*i)->GetSpellProto()->EffectMiscValue[(*i)->GetEffIndex()]);
            AdvertisedBenefit += int32(GetStat(usedStat) * (*i)->GetAmount() / 100.0f);
        }

        // ... and attack power
        AuraEffectList const& mHealingDonebyAP = GetAuraEffectsByType(SPELL_AURA_MOD_SPELL_HEALING_OF_ATTACK_POWER);
        for (AuraEffectList::const_iterator i = mHealingDonebyAP.begin(); i != mHealingDonebyAP.end(); ++i)
            if ((*i)->GetMiscValue() & schoolMask)
                AdvertisedBenefit += int32(GetTotalAttackPowerValue(BASE_ATTACK) * (*i)->GetAmount() / 100.0f);

        // TODO this should modify PLAYER_FIELD_MOD_SPELL_POWER_PCT instead of all the separate power fields
        int32 spModPct = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_SPELL_POWER_PCT);
        // should it apply to non players as well?
        AdvertisedBenefit += AdvertisedBenefit * spModPct / 100;
    }
    return AdvertisedBenefit;
}

int32 Unit::SpellBaseHealingBonusTaken(SpellSchoolMask schoolMask) const
{
    int32 AdvertisedBenefit = 0;
    AuraEffectList const& mDamageTaken = GetAuraEffectsByType(SPELL_AURA_MOD_HEALING);
    for (AuraEffectList::const_iterator i = mDamageTaken.begin(); i != mDamageTaken.end(); ++i)
        if (((*i)->GetMiscValue() & schoolMask) != 0)
            AdvertisedBenefit += (*i)->GetAmount();
    return AdvertisedBenefit;
}

bool Unit::IsImmunedToDamage(SpellSchoolMask shoolMask)
{
    //If m_immuneToSchool type contain this school type, IMMUNE damage.
    SpellImmuneList const& schoolList = m_spellImmune[IMMUNITY_SCHOOL];
    for (SpellImmuneList::const_iterator itr = schoolList.begin(); itr != schoolList.end(); ++itr)
        if (itr->type & shoolMask)
            return true;

    //If m_immuneToDamage type contain magic, IMMUNE damage.
    SpellImmuneList const& damageList = m_spellImmune[IMMUNITY_DAMAGE];
    for (SpellImmuneList::const_iterator itr = damageList.begin(); itr != damageList.end(); ++itr)
        if (itr->type & shoolMask)
            return true;

    return false;
}

bool Unit::IsImmunedToDamage(SpellEntry const* spellInfo)
{
    if (spellInfo->Attributes & SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY)
        return false;

    uint32 shoolMask = GetSpellSchoolMask(spellInfo);
    if (spellInfo->Id != 42292 && spellInfo->Id !=59752)
    {
        //If m_immuneToSchool type contain this school type, IMMUNE damage.
        SpellImmuneList const& schoolList = m_spellImmune[IMMUNITY_SCHOOL];
        for (SpellImmuneList::const_iterator itr = schoolList.begin(); itr != schoolList.end(); ++itr)
            if (itr->type & shoolMask && !CanSpellPierceImmuneAura(spellInfo, sSpellStore.LookupEntry(itr->spellId)))
                return true;
    }

    //If m_immuneToDamage type contain magic, IMMUNE damage.
    SpellImmuneList const& damageList = m_spellImmune[IMMUNITY_DAMAGE];
    for (SpellImmuneList::const_iterator itr = damageList.begin(); itr != damageList.end(); ++itr)
        if (itr->type & shoolMask)
            return true;

    return false;
}

bool Unit::IsImmunedToSpell(SpellEntry const* spellInfo, uint32 effectMask)
{
    if (!spellInfo)
        return false;

    // Single spell immunity.
    SpellImmuneList const& idList = m_spellImmune[IMMUNITY_ID];
    for (SpellImmuneList::const_iterator itr = idList.begin(); itr != idList.end(); ++itr)
        if (itr->type == spellInfo->Id)
            return true;

    if (spellInfo->Attributes & SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY)
        return false;

    if (spellInfo->Dispel)
    {
        SpellImmuneList const& dispelList = m_spellImmune[IMMUNITY_DISPEL];
        for (SpellImmuneList::const_iterator itr = dispelList.begin(); itr != dispelList.end(); ++itr)
            if (itr->type == spellInfo->Dispel)
                return true;
    }

    if (spellInfo->Mechanic)
    {
        SpellImmuneList const& mechanicList = m_spellImmune[IMMUNITY_MECHANIC];
        for (SpellImmuneList::const_iterator itr = mechanicList.begin(); itr != mechanicList.end(); ++itr)
            if (itr->type == spellInfo->Mechanic)
            {
                // Exception for Ice Block and Forbearance (same immunity type, but they can be active at once!)
                if (itr->spellId != 25771 || spellInfo->Id != 45438)
                    return true;
            }
    }

    for (int i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        // State/effect immunities applied by aura expect full spell immunity
        // Ignore effects with mechanic, they are supposed to be checked separately
        if (!spellInfo->EffectMechanic[i])
            if (IsImmunedToSpellEffect(spellInfo, i))
                return true;
    }

    if (spellInfo->Id != 42292 && spellInfo->Id !=59752)
    {
        SpellImmuneList const& schoolList = m_spellImmune[IMMUNITY_SCHOOL];
        for (SpellImmuneList::const_iterator itr = schoolList.begin(); itr != schoolList.end(); ++itr)
        {
            const SpellImmune &immune = *itr;
            const SpellEntry *immuneAura = sSpellStore.LookupEntry(immune.spellId);
            if (!immuneAura)
                continue;

            if ((immune.type & GetSpellSchoolMask(spellInfo))
                && !CanSpellPierceImmuneAura(spellInfo, immuneAura))
            {
                bool positiveImmuneAura = IsPositiveSpell(immuneAura->Id);
                for (int i = 0; i < MAX_SPELL_EFFECTS; i++)
                    if ((effectMask & (1 << i)) && (!IsPositiveEffect(spellInfo->Id, i) || !positiveImmuneAura))
                        return true;
            }
        }
    }

    return false;
}

bool Unit::IsImmunedToSpellEffect(SpellEntry const* spellInfo, uint32 index) const
{
    if (!spellInfo)
        return false;
    //If m_immuneToEffect type contain this effect type, IMMUNE effect.
    uint32 effect = spellInfo->Effect[index];
    SpellImmuneList const& effectList = m_spellImmune[IMMUNITY_EFFECT];
    for (SpellImmuneList::const_iterator itr = effectList.begin(); itr != effectList.end(); ++itr)
        if (itr->type == effect)
            return true;

    if (uint32 mechanic = spellInfo->EffectMechanic[index])
    {
        SpellImmuneList const& mechanicList = m_spellImmune[IMMUNITY_MECHANIC];
        for (SpellImmuneList::const_iterator itr = mechanicList.begin(); itr != mechanicList.end(); ++itr)
            if (itr->type == mechanic)
                return true;
    }

    if (uint32 aura = spellInfo->EffectApplyAuraName[index])
    {
        SpellImmuneList const& list = m_spellImmune[IMMUNITY_STATE];
        for (SpellImmuneList::const_iterator itr = list.begin(); itr != list.end(); ++itr)
            if (itr->type == aura)
                if (!(spellInfo->AttributesEx3 & SPELL_ATTR3_IGNORE_HIT_RESULT))
                    return true;
        // Check for immune to application of harmful magical effects
        AuraEffectList const& immuneAuraApply = GetAuraEffectsByType(SPELL_AURA_MOD_IMMUNE_AURA_APPLY_SCHOOL);
        for (AuraEffectList::const_iterator iter = immuneAuraApply.begin(); iter != immuneAuraApply.end(); ++iter)
        if (spellInfo->DmgClass == SPELL_DAMAGE_CLASS_MAGIC &&   // Magic debuff
                ((*iter)->GetMiscValue() & GetSpellSchoolMask(spellInfo)) &&  // Check school
                !IsPositiveEffect(spellInfo->Id, index))                                  // Harmful
                return true;
    }

    return false;
}

bool Unit::IsDamageToThreatSpell(SpellEntry const * spellInfo) const
{
    if (!spellInfo)
        return false;

    switch(spellInfo->SpellFamilyName)
    {
        case SPELLFAMILY_WARLOCK:
            if (spellInfo->SpellFamilyFlags[0] == 0x100) // Searing Pain
                return true;
            break;
        case SPELLFAMILY_SHAMAN:
            if (spellInfo->SpellFamilyFlags[0] == SPELLFAMILYFLAG_SHAMAN_FROST_SHOCK)
                return true;
            break;
        case SPELLFAMILY_DEATHKNIGHT:
            if (spellInfo->SpellFamilyFlags[1] == 0x20000000) // Rune Strike
                return true;
            if (spellInfo->SpellFamilyFlags[2] == 0x8) // Death and Decay
                return true;
            break;
        case SPELLFAMILY_WARRIOR:
            if (spellInfo->SpellFamilyFlags[0] == 0x80) // Thunder Clap
                return true;
            break;
    }

    return false;
}

void Unit::MeleeDamageBonus(Unit *pVictim, uint32 *pdamage, WeaponAttackType attType, SpellEntry const *spellProto)
{
    if (pVictim == NULL)
        return;

    uint32 damage = *pdamage;

    damage = this->MeleeDamageBonusDone(pVictim, damage, attType, spellProto);
    damage = pVictim->MeleeDamageBonusTaken(this, damage, attType, spellProto);

    *pdamage = damage;
}

// TODO -> Remove this later
/*void Unit::MeleeDamageBonus(Unit *pVictim, uint32 *pdamage, WeaponAttackType attType, SpellEntry const *spellProto)
{
    if (!pVictim)
        return;

    if (*pdamage == 0)
        return;

    uint32 creatureTypeMask = pVictim->GetCreatureTypeMask();

    // Taken/Done fixed damage bonus auras
    int32 DoneFlatBenefit = 0;
    int32 TakenFlatBenefit = 0;

    // ..done (for creature type by mask) in taken
    AuraEffectList const& mDamageDoneCreature = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_DONE_CREATURE);
    for (AuraEffectList::const_iterator i = mDamageDoneCreature.begin(); i != mDamageDoneCreature.end(); ++i)
        if (creatureTypeMask & uint32((*i)->GetMiscValue()))
            DoneFlatBenefit += (*i)->GetAmount();

    // ..done
    // SPELL_AURA_MOD_DAMAGE_DONE included in weapon damage

    // ..done (base at attack power for marked target and base at attack power for creature type)
    int32 APbonus = 0;

    if (attType == RANGED_ATTACK)
    {
        APbonus += pVictim->GetTotalAuraModifier(SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS);

        // ..done (base at attack power and creature type)
        AuraEffectList const& mCreatureAttackPower = GetAuraEffectsByType(SPELL_AURA_MOD_RANGED_ATTACK_POWER_VERSUS);
        for (AuraEffectList::const_iterator i = mCreatureAttackPower.begin(); i != mCreatureAttackPower.end(); ++i)
            if (creatureTypeMask & uint32((*i)->GetMiscValue()))
                APbonus += (*i)->GetAmount();
    }
    else
    {
        APbonus += pVictim->GetTotalAuraModifier(SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS);

        // ..done (base at attack power and creature type)
        AuraEffectList const& mCreatureAttackPower = GetAuraEffectsByType(SPELL_AURA_MOD_MELEE_ATTACK_POWER_VERSUS);
        for (AuraEffectList::const_iterator i = mCreatureAttackPower.begin(); i != mCreatureAttackPower.end(); ++i)
            if (creatureTypeMask & uint32((*i)->GetMiscValue()))
                APbonus += (*i)->GetAmount();
    }

    if (APbonus != 0)                                       // Can be negative
    {
        bool normalized = false;
        if (spellProto)
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                if (spellProto->Effect[i] == SPELL_EFFECT_NORMALIZED_WEAPON_DMG)
                {
                    normalized = true;
                    break;
                }
        DoneFlatBenefit += int32(APbonus/14.0f * GetAPMultiplier(attType,normalized));
    }

    // ..taken
    AuraEffectList const& mDamageTaken = pVictim->GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_TAKEN);
    for (AuraEffectList::const_iterator i = mDamageTaken.begin(); i != mDamageTaken.end(); ++i)
        if ((*i)->GetMiscValue() & GetMeleeDamageSchoolMask())
            TakenFlatBenefit += (*i)->GetAmount();

    if (attType != RANGED_ATTACK)
        TakenFlatBenefit += pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN);
    else
        TakenFlatBenefit += pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN);

    // Done/Taken total percent damage auras
    float DoneTotalMod = 1.0f;
    float TakenTotalMod = 1.0f;

    // ..done
    // SPELL_AURA_MOD_AUTOATTACK_DAMAGE_1, SPELL_AURA_MOD_AUTOATTACK_DAMAGE_2
    if (!spellProto)
    {
        if (pVictim)
        {
            AuraEffectList const & autoattackDamage = pVictim->GetAuraEffectsByType(SPELL_AURA_MOD_AUTOATTACK_DAMAGE_1);
            for (AuraEffectList::const_iterator i = autoattackDamage.begin(); i != autoattackDamage.end(); ++i)
                if ((*i)->GetCasterGUID() == GetGUID())
                    AddPctN(DoneTotalMod, (*i)->GetAmount());
        }

        AuraEffectList const & autoattackDamage2 = GetAuraEffectsByType(SPELL_AURA_MOD_AUTOATTACK_DAMAGE_2);
        for (AuraEffectList::const_iterator i = autoattackDamage2.begin(); i != autoattackDamage2.end(); ++i)
        {
            int32 amount = (*i)->GetAmount();
            if ((*i)->GetSpellProto()->EquippedItemClass == -1)
                AddPctN(DoneTotalMod, amount);
            else if (!((*i)->GetSpellProto()->AttributesEx5 & SPELL_ATTR5_SPECIAL_ITEM_CLASS_CHECK) && ((*i)->GetSpellProto()->EquippedItemSubClassMask == 0))
                AddPctN(DoneTotalMod, amount);
            else if (ToPlayer() && ToPlayer()->HasItemFitToSpellRequirements((*i)->GetSpellProto()))
                AddPctN(DoneTotalMod, (*i)->GetAmount());
        }
    }


    // SPELL_AURA_MOD_DAMAGE_PERCENT_DONE included in weapon damage
    // SPELL_AURA_MOD_OFFHAND_DAMAGE_PCT  included in weapon damage

    // SPELL_AURA_MOD_DAMAGE_PERCENT_DONE for non-physical spells like Scourge Strike, Frost Strike, this is NOT included in weapon damage
    // Some spells don't benefit from pct done mods
    if (spellProto)
        if (!(spellProto->AttributesEx6 & SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS))
        {
            AuraEffectList const & mModDamagePercentDone = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
            for (AuraEffectList::const_iterator i = mModDamagePercentDone.begin(); i != mModDamagePercentDone.end(); ++i)
            {
                if ((*i)->GetMiscValue() & spellProto->SchoolMask && !(spellProto->SchoolMask & SPELL_SCHOOL_MASK_NORMAL))
                {
                    if ((*i)->GetSpellProto()->EquippedItemClass == -1)
                        AddPctN(DoneTotalMod, (*i)->GetAmount());
                    else if (!((*i)->GetSpellProto()->AttributesEx5 & SPELL_ATTR5_SPECIAL_ITEM_CLASS_CHECK) && ((*i)->GetSpellProto()->EquippedItemSubClassMask == 0))
                        AddPctN(DoneTotalMod, (*i)->GetAmount());
                    else if (ToPlayer() && ToPlayer()->HasItemFitToSpellRequirements((*i)->GetSpellProto()))
                        AddPctN(DoneTotalMod, (*i)->GetAmount());
                }
            }
        }

    AuraEffectList const &mDamageDoneVersus = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_DONE_VERSUS);
    for (AuraEffectList::const_iterator i = mDamageDoneVersus.begin(); i != mDamageDoneVersus.end(); ++i)
        if (creatureTypeMask & uint32((*i)->GetMiscValue()))
            DoneTotalMod *= ((*i)->GetAmount()+100.0f)/100.0f;

    // bonus against aurastate
    AuraEffectList const &mDamageDoneVersusAurastate = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_DONE_VERSUS_AURASTATE);
    for (AuraEffectList::const_iterator i = mDamageDoneVersusAurastate.begin(); i != mDamageDoneVersusAurastate.end(); ++i)
        if (pVictim->HasAuraState(AuraState((*i)->GetMiscValue())))
            DoneTotalMod *= ((*i)->GetAmount() + 100.0f) / 100.0f;

    // done scripted mod (take it from owner)
    Unit * owner = GetOwner() ? GetOwner() : this;
    AuraEffectList const &mOverrideClassScript= owner->GetAuraEffectsByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
    for (AuraEffectList::const_iterator i = mOverrideClassScript.begin(); i != mOverrideClassScript.end(); ++i)
    {
        if (!(*i)->IsAffectedOnSpell(spellProto))
            continue;

        switch ((*i)->GetMiscValue())
        {
            // Rage of Rivendare
            case 7293:
            {
                if (pVictim->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DEATHKNIGHT, 0,0x02000000,0))
                    if (SpellChainNode const *chain = sSpellMgr->GetSpellChainNode((*i)->GetId()))
                        DoneTotalMod *= (chain->rank * 2.0f + 100.0f)/100.0f;
                break;
            }
        }
    }

    // Custom scripted damage
    if (spellProto)
    {
        switch(spellProto->SpellFamilyName)
        {
            case SPELLFAMILY_WARRIOR:
            {
                if (spellProto->SpellFamilyFlags[0] & SPELLFAMILYFLAG_WARRIOR_SLAM)
                {
                    // Bloodsurge: Increases damage of Slam by +20%
                    if(HasAura(46916))
                        DoneTotalMod *= 1.20f;
                }
                break;
            }
            case SPELLFAMILY_SHAMAN:
            {
                // Improved Lava Lash - damage increase by X% for every stack of Searing Flames
                if (spellProto->Id == 60103)
                {
                    float bonusApp = (HasAura(99209)) ? 0.05f : 0.0f; // Shaman T12 Enhancement 2P Bonus

                    if (HasAura(77701))
                        bonusApp += 0.2f;
                    else if (HasAura(77700))
                        bonusApp += 0.1f;

                    if (Aura* pAura = pVictim->GetAura(77661, GetGUID()))
                    {
                        DoneTotalMod += bonusApp * pAura->GetStackAmount();
                        pAura->Remove();
                    }
                }
                break;
            }
            case SPELLFAMILY_DEATHKNIGHT:
            {
                // Glacier Rot
                if (spellProto->SpellFamilyFlags[0] & 0x2 || spellProto->SpellFamilyFlags[1] & 0x6)
                    if (AuraEffect * aurEff = GetDummyAuraEffect(SPELLFAMILY_DEATHKNIGHT, 196, 0))
                        if (pVictim->GetDiseasesByCaster(owner->GetGUID()) > 0)
                            DoneTotalMod *= (100.0f + aurEff->GetAmount()) / 100.0f;

                // Merciless Combat for melee spells
                if (pVictim->HealthBelowPct(35))
                {
                    if (spellProto->Id == 49020 || spellProto->Id == 66198 || // Obliterate
                        spellProto->Id == 49143 || spellProto->Id == 66196)   // Frost Strike
                    {
                        if (AuraEffect * aurEff = GetAuraEffect(SPELL_AURA_DUMMY, SPELLFAMILY_DEATHKNIGHT, 2656, EFFECT_0))
                            AddPctF(DoneTotalMod, aurEff->GetAmount());
                    }
                }
                break;
            }
        }
    }

    // ..taken
    AuraEffectList const& mModDamagePercentTaken = pVictim->GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN);
    for (AuraEffectList::const_iterator i = mModDamagePercentTaken.begin(); i != mModDamagePercentTaken.end(); ++i)
        if ((*i)->GetMiscValue() & GetMeleeDamageSchoolMask())
            TakenTotalMod *= ((*i)->GetAmount()+100.0f)/100.0f;

    // From caster spells
    AuraEffectList const& mOwnerTaken = pVictim->GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_FROM_CASTER);
    for (AuraEffectList::const_iterator i = mOwnerTaken.begin(); i != mOwnerTaken.end(); ++i)
        if ((*i)->GetCasterGUID() == GetGUID() && (*i)->IsAffectedOnSpell(spellProto))
            TakenTotalMod *= ((*i)->GetAmount() + 100.0f) / 100.0f;

    // .. taken pct (special attacks)
    if (spellProto)
    {
        // Mod damage from spell mechanic
        uint32 mechanicMask = GetAllSpellMechanicMask(spellProto);

        // Shred, Maul - "Effects which increase Bleed damage also increase Shred damage"
        if (spellProto->SpellFamilyName == SPELLFAMILY_DRUID && spellProto->SpellFamilyFlags[0] & 0x00008800)
            mechanicMask |= (1<<MECHANIC_BLEED);

        if (mechanicMask)
        {
            AuraEffectList const& mDamageDoneMechanic = pVictim->GetAuraEffectsByType(SPELL_AURA_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT);
            for (AuraEffectList::const_iterator i = mDamageDoneMechanic.begin(); i != mDamageDoneMechanic.end(); ++i)
                if (mechanicMask & uint32(1<<((*i)->GetMiscValue())))
                    TakenTotalMod *= ((*i)->GetAmount()+100.0f)/100.0f;
        }
    }

    // .. taken pct: dummy auras
    AuraEffectList const& mDummyAuras = pVictim->GetAuraEffectsByType(SPELL_AURA_DUMMY);
    for (AuraEffectList::const_iterator i = mDummyAuras.begin(); i != mDummyAuras.end(); ++i)
    {
        switch((*i)->GetSpellProto()->SpellIconID)
        {
            //Cheat Death
            case 2109:
                if ((*i)->GetMiscValue() & SPELL_SCHOOL_MASK_NORMAL)
                {
                    if (pVictim->GetTypeId() != TYPEID_PLAYER)
                        continue;
                    if ((*i)->GetAmount())
                        TakenTotalMod *= ((*i)->GetAmount()+100.0f)/100.0f;
                }
                break;
            // Blessing of Sanctuary
            // Greater Blessing of Sanctuary
            case 19:
            case 1804:
            {
                if ((*i)->GetSpellProto()->SpellFamilyName != SPELLFAMILY_PALADIN)
                    continue;

                if ((*i)->GetMiscValue() & (spellProto ? GetSpellSchoolMask(spellProto) : 0))
                    TakenTotalMod *= ((*i)->GetAmount() + 100.0f) / 100.0f;
                break;
            }
            // Ebon Plague
            case 1933:
                if ((*i)->GetMiscValue() & (spellProto ? GetSpellSchoolMask(spellProto) : 0))
                    TakenTotalMod *= ((*i)->GetAmount()+100.0f)/100.0f;
                break;
        }
    }

    // .. taken pct: class scripts
    AuraEffectList const& mclassScritAuras = GetAuraEffectsByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
    for (AuraEffectList::const_iterator i = mclassScritAuras.begin(); i != mclassScritAuras.end(); ++i)
    {
        switch((*i)->GetMiscValue())
        {
            case 6427: case 6428:                           // Dirty Deeds
                if (pVictim->HasAuraState(AURA_STATE_HEALTHLESS_35_PERCENT, spellProto, this))
                {
                    AuraEffect* eff0 = (*i)->GetBase()->GetEffect(0);
                    if (!eff0 || (*i)->GetEffIndex() != 1)
                    {
                        sLog->outError("Spell structure of DD (%u) changed.",(*i)->GetId());
                        continue;
                    }

                    // effect 0 have expected value but in negative state
                    TakenTotalMod *= (-eff0->GetAmount()+100.0f)/100.0f;
                }
                break;
        }
    }

    if (attType != RANGED_ATTACK)
    {
        AuraEffectList const& mModMeleeDamageTakenPercent = pVictim->GetAuraEffectsByType(SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN_PCT);
        for (AuraEffectList::const_iterator i = mModMeleeDamageTakenPercent.begin(); i != mModMeleeDamageTakenPercent.end(); ++i)
            TakenTotalMod *= ((*i)->GetAmount()+100.0f)/100.0f;
    }
    else
    {
        AuraEffectList const& mModRangedDamageTakenPercent = pVictim->GetAuraEffectsByType(SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN_PCT);
        for (AuraEffectList::const_iterator i = mModRangedDamageTakenPercent.begin(); i != mModRangedDamageTakenPercent.end(); ++i)
            TakenTotalMod *= ((*i)->GetAmount()+100.0f)/100.0f;
    }

    float tmpDamage = float(int32(*pdamage) + DoneFlatBenefit) * DoneTotalMod;

    // apply spellmod to Done damage
    if (spellProto)
        if (Player* modOwner = GetSpellModOwner())
            modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_DAMAGE, tmpDamage);

    tmpDamage = (tmpDamage + TakenFlatBenefit)*TakenTotalMod;

    // bonus result can be negative
    *pdamage = uint32(std::max(tmpDamage, 0.0f));
}*/

uint32 Unit::MeleeDamageBonusDone(Unit* victim, uint32 pdamage, WeaponAttackType attType, SpellEntry const* spellProto)
{
    if (!victim || pdamage == 0)
        return 0;

    uint32 creatureTypeMask = victim->GetCreatureTypeMask();

    // Done fixed damage bonus auras
    int32 DoneFlatBenefit = 0;

    // ..done
    AuraEffectList const& mDamageDoneCreature = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_DONE_CREATURE);
    for (AuraEffectList::const_iterator i = mDamageDoneCreature.begin(); i != mDamageDoneCreature.end(); ++i)
        if (creatureTypeMask & uint32((*i)->GetMiscValue()))
            DoneFlatBenefit += (*i)->GetAmount();

    // ..done
    // SPELL_AURA_MOD_DAMAGE_DONE included in weapon damage

    // ..done (base at attack power for marked target and base at attack power for creature type)
    int32 APbonus = 0;

    if (attType == RANGED_ATTACK)
    {
        APbonus += victim->GetTotalAuraModifier(SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS);

        // ..done (base at attack power and creature type)
        AuraEffectList const& mCreatureAttackPower = GetAuraEffectsByType(SPELL_AURA_MOD_RANGED_ATTACK_POWER_VERSUS);
        for (AuraEffectList::const_iterator i = mCreatureAttackPower.begin(); i != mCreatureAttackPower.end(); ++i)
            if (creatureTypeMask & uint32((*i)->GetMiscValue()))
                APbonus += (*i)->GetAmount();
    }
    else
    {
        APbonus += victim->GetTotalAuraModifier(SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS);

        // ..done (base at attack power and creature type)
        AuraEffectList const& mCreatureAttackPower = GetAuraEffectsByType(SPELL_AURA_MOD_MELEE_ATTACK_POWER_VERSUS);
        for (AuraEffectList::const_iterator i = mCreatureAttackPower.begin(); i != mCreatureAttackPower.end(); ++i)
            if (creatureTypeMask & uint32((*i)->GetMiscValue()))
                APbonus += (*i)->GetAmount();
    }

    if (APbonus != 0)                                       // Can be negative
    {
        bool normalized = false;
        if (spellProto)
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                if (spellProto->Effect[i] == SPELL_EFFECT_NORMALIZED_WEAPON_DMG)
                {
                    normalized = true;
                    break;
                }
        DoneFlatBenefit += int32(APbonus/14.0f * GetAPMultiplier(attType, normalized));
    }

    // Done total percent damage auras
    float DoneTotalMod = 1.0f;

    // SPELL_AURA_MOD_AUTOATTACK_DAMAGE_1, SPELL_AURA_MOD_AUTOATTACK_DAMAGE_2
    if (!spellProto)
    {
        if (victim)
        {
            AuraEffectList const & autoattackDamage = victim->GetAuraEffectsByType(SPELL_AURA_MOD_AUTOATTACK_DAMAGE_1);
            for (AuraEffectList::const_iterator i = autoattackDamage.begin(); i != autoattackDamage.end(); ++i)
                if ((*i)->GetCasterGUID() == GetGUID())
                    AddPctN(DoneTotalMod, (*i)->GetAmount());
        }

        AuraEffectList const & autoattackDamage2 = GetAuraEffectsByType(SPELL_AURA_MOD_AUTOATTACK_DAMAGE_2);
        for (AuraEffectList::const_iterator i = autoattackDamage2.begin(); i != autoattackDamage2.end(); ++i)
        {
            int32 amount = (*i)->GetAmount();
            if ((*i)->GetSpellProto()->EquippedItemClass == -1)
                AddPctN(DoneTotalMod, amount);
            else if (!((*i)->GetSpellProto()->AttributesEx5 & SPELL_ATTR5_SPECIAL_ITEM_CLASS_CHECK) && ((*i)->GetSpellProto()->EquippedItemSubClassMask == 0))
                AddPctN(DoneTotalMod, amount);
            else if (ToPlayer() && ToPlayer()->HasItemFitToSpellRequirements((*i)->GetSpellProto()))
                AddPctN(DoneTotalMod, (*i)->GetAmount());
        }
    }

    // Some spells don't benefit from pct done mods
    if (spellProto)
        if (!(spellProto->AttributesEx6 & SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS))
        {
            AuraEffectList const& mModDamagePercentDone = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
            for (AuraEffectList::const_iterator i = mModDamagePercentDone.begin(); i != mModDamagePercentDone.end(); ++i)
            {
                if ((*i)->GetMiscValue() & GetSpellSchoolMask(spellProto) && !(GetSpellSchoolMask(spellProto) & SPELL_SCHOOL_MASK_NORMAL))
                {
                    if ((*i)->GetSpellProto()->EquippedItemClass == -1)
                        AddPctN(DoneTotalMod, (*i)->GetAmount());
                    else if (!((*i)->GetSpellProto()->AttributesEx5 & SPELL_ATTR5_SPECIAL_ITEM_CLASS_CHECK) && ((*i)->GetSpellProto()->EquippedItemSubClassMask == 0))
                        AddPctN(DoneTotalMod, (*i)->GetAmount());
                    else if (ToPlayer() && ToPlayer()->HasItemFitToSpellRequirements((*i)->GetSpellProto()))
                        AddPctN(DoneTotalMod, (*i)->GetAmount());
                }
            }
        }

    AuraEffectList const& mDamageDoneVersus = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_DONE_VERSUS);
    for (AuraEffectList::const_iterator i = mDamageDoneVersus.begin(); i != mDamageDoneVersus.end(); ++i)
        if (creatureTypeMask & uint32((*i)->GetMiscValue()))
            AddPctN(DoneTotalMod, (*i)->GetAmount());

    // bonus against aurastate
    AuraEffectList const& mDamageDoneVersusAurastate = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_DONE_VERSUS_AURASTATE);
    for (AuraEffectList::const_iterator i = mDamageDoneVersusAurastate.begin(); i != mDamageDoneVersusAurastate.end(); ++i)
        if (victim->HasAuraState(AuraState((*i)->GetMiscValue())))
            AddPctN(DoneTotalMod, (*i)->GetAmount());

    // Add SPELL_AURA_MOD_DAMAGE_DONE_FOR_MECHANIC percent bonus
    if (spellProto)
        AddPctN(DoneTotalMod, GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_DAMAGE_MECHANIC, spellProto->Mechanic));

    // done scripted mod (take it from owner)
    //Unit * owner = GetOwner() ? GetOwner() : this;

    // Custom scripted damage
    if (spellProto)
    {
        switch(spellProto->SpellFamilyName)
        {
            case SPELLFAMILY_WARRIOR:
            {
                if (spellProto->SpellFamilyFlags[0] & SPELLFAMILYFLAG_WARRIOR_SLAM)
                {
                    // Bloodsurge: Increases damage of Slam by +20%
                    if(HasAura(46916))
                        DoneTotalMod *= 1.20f;
                }
                break;
            }
            case SPELLFAMILY_SHAMAN:
            {
                // Improved Lava Lash - damage increase by X% for every stack of Searing Flames
                if (spellProto->Id == 60103)
                {
                    float bonusApp = (HasAura(99209)) ? 0.05f : 0.0f; // Shaman T12 Enhancement 2P Bonus

                    if (HasAura(77701))
                        bonusApp += 0.2f;
                    else if (HasAura(77700))
                        bonusApp += 0.1f;

                    if (Aura* pAura = victim->GetAura(77661, GetGUID()))
                    {
                        DoneTotalMod += bonusApp * pAura->GetStackAmount();
                        pAura->Remove();
                    }
                }
                break;
            }
            case SPELLFAMILY_DEATHKNIGHT:
            {
                // Merciless Combat for melee spells
                if (victim->HealthBelowPct(35))
                {
                    if (spellProto->Id == 49020 || spellProto->Id == 66198 || // Obliterate
                        spellProto->Id == 49143 || spellProto->Id == 66196)   // Frost Strike
                    {
                        if (AuraEffect * aurEff = GetAuraEffect(SPELL_AURA_DUMMY, SPELLFAMILY_DEATHKNIGHT, 2656, EFFECT_0))
                            AddPctF(DoneTotalMod, aurEff->GetAmount());
                    }
                }
                break;
            }
        }
    }

    float tmpDamage = float(int32(pdamage) + DoneFlatBenefit) * DoneTotalMod;

    // apply spellmod to Done damage
    if (spellProto)
        if (Player* modOwner = GetSpellModOwner())
            modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_DAMAGE, tmpDamage);

    // bonus result can be negative
    return uint32(std::max(tmpDamage, 0.0f));
}

uint32 Unit::MeleeDamageBonusTaken(Unit* attacker, uint32 pdamage, WeaponAttackType attType, SpellEntry const* spellProto)
{
    if (pdamage == 0)
        return 0;

    int32 TakenFlatBenefit = 0;
    float TakenTotalCasterMod = 0.0f;

    // get all auras from caster that allow the spell to ignore resistance (sanctified wrath)
    SpellSchoolMask attackSchoolMask = spellProto ? GetSpellSchoolMask(spellProto) : SPELL_SCHOOL_MASK_NORMAL;
    AuraEffectList const& IgnoreResistAuras = attacker->GetAuraEffectsByType(SPELL_AURA_MOD_IGNORE_TARGET_RESIST);
    for (AuraEffectList::const_iterator i = IgnoreResistAuras.begin(); i != IgnoreResistAuras.end(); ++i)
    {
        if ((*i)->GetMiscValue() & attackSchoolMask)
            TakenTotalCasterMod += (float((*i)->GetAmount()));
    }

    // ..taken
    AuraEffectList const& mDamageTaken = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_TAKEN);
    for (AuraEffectList::const_iterator i = mDamageTaken.begin(); i != mDamageTaken.end(); ++i)
        if ((*i)->GetMiscValue() & attacker->GetMeleeDamageSchoolMask())
            TakenFlatBenefit += (*i)->GetAmount();

    if (attType != RANGED_ATTACK)
        TakenFlatBenefit += GetTotalAuraModifier(SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN);
    else
        TakenFlatBenefit += GetTotalAuraModifier(SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN);

    // Taken total percent damage auras
    float TakenTotalMod = 1.0f;

    // ..taken
    TakenTotalMod *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN, attacker->GetMeleeDamageSchoolMask());

    // .. taken pct (special attacks)
    if (spellProto)
    {
        // From caster spells
        AuraEffectList const& mOwnerTaken = GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_FROM_CASTER);
        for (AuraEffectList::const_iterator i = mOwnerTaken.begin(); i != mOwnerTaken.end(); ++i)
            if ((*i)->GetCasterGUID() == attacker->GetGUID() && (*i)->IsAffectedOnSpell(spellProto))
                AddPctN(TakenTotalMod, (*i)->GetAmount());

        // Mod damage from spell mechanic
        uint32 mechanicMask = GetAllSpellMechanicMask(spellProto);

        // Shred, Maul - "Effects which increase Bleed damage also increase Shred damage"
        if (spellProto->SpellFamilyName == SPELLFAMILY_DRUID && spellProto->SpellFamilyFlags[0] & 0x00008800)
            mechanicMask |= (1<<MECHANIC_BLEED);

        if (mechanicMask)
        {
            AuraEffectList const& mDamageDoneMechanic = GetAuraEffectsByType(SPELL_AURA_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT);
            for (AuraEffectList::const_iterator i = mDamageDoneMechanic.begin(); i != mDamageDoneMechanic.end(); ++i)
                if (mechanicMask & uint32(1<<((*i)->GetMiscValue())))
                    AddPctN(TakenTotalMod, (*i)->GetAmount());
        }
    }

    // .. taken pct: dummy auras
    AuraEffectList const& mDummyAuras = GetAuraEffectsByType(SPELL_AURA_DUMMY);
    for (AuraEffectList::const_iterator i = mDummyAuras.begin(); i != mDummyAuras.end(); ++i)
    {
        switch ((*i)->GetSpellProto()->SpellIconID)
        {
            // Cheat Death
            case 2109:
                if ((*i)->GetMiscValue() & SPELL_SCHOOL_MASK_NORMAL)
                {
                    if (Player* player = ToPlayer())
                    {
                        float mod = player->GetRatingBonusValue(CR_RESILIENCE_PLAYER_DAMAGE_TAKEN) * (-8.0f);
                        AddPctN(TakenTotalMod, std::max(mod, float((*i)->GetAmount())));
                    }
                }
                break;
        }
    }

    // .. taken pct: class scripts
    AuraEffectList const& mclassScritAuras = GetAuraEffectsByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
    for (AuraEffectList::const_iterator i = mclassScritAuras.begin(); i != mclassScritAuras.end(); ++i)
    {
        switch((*i)->GetMiscValue())
        {
            case 6427: case 6428:                           // Dirty Deeds
                if (HasAuraState(AURA_STATE_HEALTHLESS_35_PERCENT, spellProto, this))
                {
                    AuraEffect* eff0 = (*i)->GetBase()->GetEffect(0);
                    if (!eff0 || (*i)->GetEffIndex() != 1)
                    {
                        sLog->outError("Spell structure of DD (%u) changed.",(*i)->GetId());
                        continue;
                    }

                    // effect 0 have expected value but in negative state
                    TakenTotalMod *= (-eff0->GetAmount()+100.0f)/100.0f;
                }
                break;
        }
    }

    if (attType != RANGED_ATTACK)
    {
        AuraEffectList const& mModMeleeDamageTakenPercent = GetAuraEffectsByType(SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN_PCT);
        for (AuraEffectList::const_iterator i = mModMeleeDamageTakenPercent.begin(); i != mModMeleeDamageTakenPercent.end(); ++i)
            AddPctN(TakenTotalMod, (*i)->GetAmount());
    }
    else
    {
        AuraEffectList const& mModRangedDamageTakenPercent = GetAuraEffectsByType(SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN_PCT);
        for (AuraEffectList::const_iterator i = mModRangedDamageTakenPercent.begin(); i != mModRangedDamageTakenPercent.end(); ++i)
            AddPctN(TakenTotalMod, (*i)->GetAmount());
    }

    float tmpDamage = 0.0f;

    if (TakenTotalCasterMod)
    {
        if (TakenFlatBenefit < 0)
        {
            if (TakenTotalMod < 1)
                tmpDamage = ((float(CalculatePctF(pdamage, TakenTotalCasterMod) + TakenFlatBenefit) * TakenTotalMod) + CalculatePctF(pdamage, TakenTotalCasterMod));
            else
                tmpDamage = ((float(CalculatePctF(pdamage, TakenTotalCasterMod) + TakenFlatBenefit) + CalculatePctF(pdamage, TakenTotalCasterMod)) * TakenTotalMod);
        }
        else if (TakenTotalMod < 1)
            tmpDamage = ((CalculatePctF(float(pdamage) + TakenFlatBenefit, TakenTotalCasterMod) * TakenTotalMod) + CalculatePctF(float(pdamage) + TakenFlatBenefit, TakenTotalCasterMod));
    }
    if (!tmpDamage)
        tmpDamage = (float(pdamage) + TakenFlatBenefit) * TakenTotalMod;

    // bonus result can be negative
    return uint32(std::max(tmpDamage, 0.0f));
}

void Unit::ApplySpellImmune(uint32 spellId, uint32 op, uint32 type, bool apply)
{
    if (apply)
    {
        for (SpellImmuneList::iterator itr = m_spellImmune[op].begin(), next; itr != m_spellImmune[op].end(); itr = next)
        {
            next = itr; ++next;
            if (itr->type == type)
            {
                m_spellImmune[op].erase(itr);
                next = m_spellImmune[op].begin();
            }
        }
        SpellImmune Immune;
        Immune.spellId = spellId;
        Immune.type = type;
        m_spellImmune[op].push_back(Immune);
    }
    else
    {
        for (SpellImmuneList::iterator itr = m_spellImmune[op].begin(); itr != m_spellImmune[op].end(); ++itr)
        {
            if (itr->spellId == spellId)
            {
                m_spellImmune[op].erase(itr);
                break;
            }
        }
    }
}

void Unit::ApplySpellDispelImmunity(const SpellEntry * spellProto, DispelType type, bool apply)
{
    ApplySpellImmune(spellProto->Id,IMMUNITY_DISPEL, type, apply);

    if (apply && spellProto->AttributesEx & SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY)
    {
        // Create dispel mask by dispel type
        uint32 dispelMask = GetDispellMask(type);
        // Dispel all existing auras vs current dispel type
        AuraApplicationMap& auras = GetAppliedAuras();
        for (AuraApplicationMap::iterator itr = auras.begin(); itr != auras.end();)
        {
            SpellEntry const* spell = itr->second->GetBase()->GetSpellProto();
            if ((1<<spell->Dispel) & dispelMask)
            {
                // Dispel aura
                RemoveAura(itr);
            }
            else
                ++itr;
        }
    }
}

float Unit::GetWeaponProcChance() const
{
    // normalized proc chance for weapon attack speed
    // (odd formula...)
    if (isAttackReady(BASE_ATTACK))
        return (GetAttackTime(BASE_ATTACK) * 1.8f / 1000.0f);
    else if (haveOffhandWeapon() && isAttackReady(OFF_ATTACK))
        return (GetAttackTime(OFF_ATTACK) * 1.6f / 1000.0f);
    return 0;
}

float Unit::GetPPMProcChance(uint32 WeaponSpeed, float PPM, const SpellEntry * spellProto) const
{
    // proc per minute chance calculation
    if (PPM <= 0) return 0.0f;
    // Apply chance modifer aura
    if (spellProto)
        if (Player* modOwner = GetSpellModOwner())
            modOwner->ApplySpellMod(spellProto->Id,SPELLMOD_PROC_PER_MINUTE,PPM);

    return (WeaponSpeed * PPM) / 600.0f;   // result is chance in percents (probability = Speed_in_sec * (PPM / 60))
}

void Unit::Mount(uint32 mount, uint32 VehicleId, uint32 creatureEntry)
{
    RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_MOUNT);

    if (mount)
        SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, mount);

    SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_MOUNT);

    // unsummon pet
    if (Player* plr = ToPlayer())
    {
        Pet* pet = plr->GetPet();
        if (pet)
        {
            Battleground *bg = ToPlayer()->GetBattleground();
            // don't unsummon pet in arena but SetFlag UNIT_FLAG_STUNNED to disable pet's interface
            if (bg && bg->isArena())
                pet->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
            else
                plr->UnsummonPetTemporaryIfAny();
        }

        if (plr->GetEmoteState())
            plr->SetEmoteState(0);

        if (VehicleId)
        {
            if (CreateVehicleKit(VehicleId))
            {
                GetVehicleKit()->Reset();

                // Send others that we now have a vehicle
                WorldPacket data(SMSG_PLAYER_VEHICLE_DATA, GetPackGUID().size()+4);
                data.appendPackGUID(GetGUID());
                data << uint32(VehicleId);
                SendMessageToSet(&data,true);

                data.Initialize(SMSG_ON_CANCEL_EXPECTED_RIDE_VEHICLE_AURA, 0);
                plr->GetSession()->SendPacket(&data);

                // mounts can also have accessories
                GetVehicleKit()->InstallAllAccessories(creatureEntry);
            }
        }
    }

}

void Unit::Unmount()
{
    if (!IsMounted() && !IsMountedShape())
        return;

    /* remove druid flying/travel forms
     * (needs to be called before IsMounted) */
    switch (GetShapeshiftForm()) {
        case FORM_FLIGHT:
        case FORM_FLIGHT_EPIC:
        case FORM_TRAVEL:
            RemoveAurasByType(SPELL_AURA_MOD_SHAPESHIFT);
            return;
        default:
            break;
    }

    RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_NOT_MOUNTED);

    SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, 0);
    RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_MOUNT);

    WorldPacket data(SMSG_DISMOUNT, 8);
    data.appendPackGUID(GetGUID());
    SendMessageToSet(&data, true);

    // only resummon old pet if the player is already added to a map
    // this prevents adding a pet to a not created map which would otherwise cause a crash
    // (it could probably happen when logging in after a previous crash)
    if (GetTypeId() == TYPEID_PLAYER)
    {
        if (Pet *pPet = this->ToPlayer()->GetPet())
        {
            if (pPet && pPet->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED) && !pPet->HasUnitState(UNIT_STATE_STUNNED))
                pPet->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
        }
        else
            this->ToPlayer()->ResummonPetTemporaryUnSummonedIfAny();
    }
    if (GetTypeId() == TYPEID_PLAYER && GetVehicleKit())
    {
        // Send other players that we are no longer a vehicle
        WorldPacket data(SMSG_PLAYER_VEHICLE_DATA, 8+4);
        data.appendPackGUID(GetGUID());
        data << uint32(0);
        this->ToPlayer()->SendMessageToSet(&data, true);
        // Remove vehicle class from player
        RemoveVehicleKit();
    }
}

void Unit::SetInCombatWith(Unit* enemy)
{
    Unit* eOwner = enemy->GetCharmerOrOwnerOrSelf();
    if (eOwner->IsPvP())
    {
        SetInCombatState(true,enemy);
        return;
    }

    //check for duel
    if (eOwner->GetTypeId() == TYPEID_PLAYER && eOwner->ToPlayer()->duel)
    {
        Unit const* myOwner = GetCharmerOrOwnerOrSelf();
        if (((Player const*)eOwner)->duel->opponent == myOwner)
        {
            SetInCombatState(true,enemy);
            return;
        }
    }
    SetInCombatState(false,enemy);
}

void Unit::CombatStart(Unit* target, bool initialAggro)
{
    if (initialAggro)
    {
        if (!target->IsStandState())
            target->SetStandState(UNIT_STAND_STATE_STAND);

        if (!target->IsInCombat() && target->GetTypeId() != TYPEID_PLAYER
            && !target->ToCreature()->HasReactState(REACT_PASSIVE) && target->ToCreature()->IsAIEnabled)
        {
            target->ToCreature()->AI()->AttackStart(this);
        }

        SetInCombatWith(target);
        target->SetInCombatWith(this);
    }
    Unit *who = target->GetCharmerOrOwnerOrSelf();
    if (who->GetTypeId() == TYPEID_PLAYER)
      SetContestedPvP(who->ToPlayer());

    Player *me = GetCharmerOrOwnerPlayerOrPlayerItself();
    if (me && who->IsPvP()
        && (who->GetTypeId() != TYPEID_PLAYER
        || !me->duel || me->duel->opponent != who))
    {
        me->UpdatePvP(true);
        me->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
    }
}

void Unit::SetInCombatState(bool PvP, Unit* enemy)
{
    // only alive units can be in combat
    if (!IsAlive())
        return;

    if (PvP)
        m_CombatTimer = 5000;

    if (IsInCombat() || HasUnitState(UNIT_STATE_EVADE))
        return;

    SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);

    if (GetTypeId() != TYPEID_PLAYER)
    {
        // Set home position at place of engaging combat for escorted creatures
        if ((IsAIEnabled && this->ToCreature()->AI()->IsEscorted()) ||
            GetMotionMaster()->GetCurrentMovementGeneratorType() == WAYPOINT_MOTION_TYPE ||
            GetMotionMaster()->GetCurrentMovementGeneratorType() == POINT_MOTION_TYPE)
            this->ToCreature()->SetHomePosition(GetPositionX(), GetPositionY(), GetPositionZ(), GetOrientation());

        if (enemy)
        {
            Creature* crea = this->ToCreature();
            if (IsAIEnabled)
            {
                InstanceMap* map = GetMap() ? GetMap()->ToInstanceMap() : NULL;
                if (map && map->GetInstanceScript() && crea->GetCreatureInfo()->rank == 3)
                {
                    map->GetInstanceScript()->ResetRessurectionData();
                    map->GetInstanceScript()->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, this);
                }

                crea->AI()->EnterCombat(enemy);
                RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);//always remove Out of Combat Non Attackable flag if we enter combat and AI is enabled

            }
            if (crea->GetFormation())
                crea->GetFormation()->MemberAttackStart(crea, enemy);
        }

        if (IsPet())
        {
            UpdateSpeed(MOVE_RUN, true);
            UpdateSpeed(MOVE_SWIM, true);
            UpdateSpeed(MOVE_FLIGHT, true);
        }

        if (!(ToCreature()->GetCreatureInfo()->type_flags & CREATURE_TYPEFLAGS_MOUNTED_COMBAT))
            Unmount();
    }
    
    if(GetTypeId() == TYPEID_PLAYER)
    {
        // Add Visaual effect of Flame Druid Aura when entering combat with Fandral's Flamescythe
        if ((ToPlayer()->HasAura(768)) && ((ToPlayer()->HasItemOrGemWithIdEquipped(69897, 1) || ToPlayer()->HasItemOrGemWithIdEquipped(71466, 1))))
            ToPlayer()->CastSpell(ToPlayer(), 99244, true);

        // Interrupt mount spell cast while entering to combat
        Spell* currSpell = ToPlayer()->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        for (uint32 i = 0; i < MAX_SPELL_EFFECTS; i++)
        {
            if (currSpell && currSpell->m_spellInfo && currSpell->m_spellInfo->EffectApplyAuraName[i] == SPELL_AURA_MOUNTED)
                ToPlayer()->InterruptSpell(CURRENT_GENERIC_SPELL);
        }

        if (ToPlayer()->getRace() == RACE_WORGEN
        && ToPlayer()->HasSpell(68996) /* Two forms */)
        {
            if (ToPlayer()->HasAuraType(SPELL_AURA_MOD_SHAPESHIFT))
                ToPlayer()->SetInWorgenForm(UNIT_FLAG2_WORGEN_TRANSFORM3);
            else
                ToPlayer()->toggleWorgenForm();
        }

        // entering combat will break spectator wait time
        ToPlayer()->ViolateSpectatorWaitTime();
    }

    for (Unit::ControlList::iterator itr = m_Controlled.begin(); itr != m_Controlled.end(); ++itr)
    {
        (*itr)->SetInCombatState(PvP, enemy);
        (*itr)->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PET_IN_COMBAT);
    }
}

void Unit::ClearInCombat()
{
    m_CombatTimer = 0;
    RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);

    // Player's state will be cleared in Player::UpdateContestedPvP
    if (GetTypeId() != TYPEID_PLAYER)
    {
        Creature* creature = this->ToCreature();
        if (creature->GetCreatureInfo() && creature->GetCreatureInfo()->unit_flags & UNIT_FLAG_OOC_NOT_ATTACKABLE)
            SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);//re-apply Out of Combat Non Attackable flag if we leave combat, can be overriden in scripts in EnterEvadeMode()

        ClearUnitState(UNIT_STATE_ATTACK_PLAYER);
        if (HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TAPPED))
            SetUInt32Value(UNIT_DYNAMIC_FLAGS, ((Creature*)this)->GetCreatureInfo()->dynamicflags);
    }
    else
    {
        // Remove Visaual effect of Flame Druid Aura when leaving combat
        if (ToPlayer()->HasAura(99244))
            ToPlayer()->CastSpell(ToPlayer(), 768, true);
        this->ToPlayer()->UpdatePotionCooldown();
    }

    if (GetTypeId() != TYPEID_PLAYER && ((Creature*)this)->IsPet())
    {
        if (Unit *owner = GetOwner())
            for (uint8 i = 0; i < MAX_MOVE_TYPE; ++i)
                if (owner->GetSpeedRate(UnitMoveType(i)) > GetSpeedRate(UnitMoveType(i)))
                    SetSpeed(UnitMoveType(i), owner->GetSpeedRate(UnitMoveType(i)), true);
    }
    else if (!IsCharmed())
        return;

    RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PET_IN_COMBAT);
}

//TODO: remove this function
bool Unit::isTargetableForAttack() const
{
    return isAttackableByAOE() && !HasUnitState(UNIT_STATE_DIED);
}

bool Unit::canAttack(Unit const* target, bool force) const
{
    ASSERT(target);

    if (force)
    {
        if (IsFriendlyTo(target))
            return false;

        if (GetTypeId()!=TYPEID_PLAYER)
        {
            if (IsPet())
            {
                if (Unit *owner = GetOwner())
                    if (!(owner->canAttack(target)))
                        return false;
            }
            else if (!IsHostileTo(target))
                return false;
        }
    }
    else if (!IsHostileTo(target))
        return false;

    if (!target->isAttackableByAOE() || target->HasUnitState(UNIT_STATE_DIED))
        return false;

    // shaman totem quests: spell 8898, shaman can detect elementals but elementals cannot see shaman
    if (m_invisibilityMask || target->m_invisibilityMask)
        if (!canDetectInvisibilityOf(target) && !target->canDetectInvisibilityOf(this))
            return false;

    if (target->GetVisibility() == VISIBILITY_GROUP_STEALTH && !canDetectStealthOf(target, GetDistance(target)))
        return false;

    if (m_vehicle)
        if (IsOnVehicle(target) || m_vehicle->GetBase()->IsOnVehicle(target))
            return false;

    if (!canSeeOrDetect(target,true))
        return false;

    return true;
}

bool Unit::isAttackableByAOE(bool requireDeadTarget, SpellEntry const *spellProto) const
{
    if (IsAlive() == requireDeadTarget)
        return false;

    if (HasFlag(UNIT_FIELD_FLAGS,
        UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_OOC_NOT_ATTACKABLE))
        return false;

    if (HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE)
        && (!spellProto || !(spellProto->AttributesEx6 & SPELL_ATTR6_CAN_TARGET_UNTARGETABLE)))
        return false;

    if (GetTypeId() == TYPEID_PLAYER && ToPlayer()->IsGameMaster())
        return false;

    return !HasUnitState(UNIT_STATE_UNATTACKABLE);
}

int32 Unit::ModifyHealth(int32 dVal)
{
    int32 gain = 0;

    if (dVal == 0)
        return 0;

    int32 curHealth = (int32)GetHealth();

    int32 val = dVal + curHealth;
    if (val <= 0)
    {
        SetHealth(0);
        return -curHealth;
    }

    int32 maxHealth = (int32)GetMaxHealth();

    if (val < maxHealth)
    {
        SetHealth(val);
        gain = val - curHealth;
    }
    else if (curHealth != maxHealth)
    {
        SetHealth(maxHealth);
        gain = maxHealth - curHealth;
    }

    return gain;
}

int32 Unit::GetHealthGain(int32 dVal)
{
    int32 gain = 0;

    if (dVal == 0)
        return 0;

    int32 curHealth = (int32)GetHealth();

    int32 val = dVal + curHealth;
    if (val <= 0)
    {
        return -curHealth;
    }

    int32 maxHealth = (int32)GetMaxHealth();

    if (val < maxHealth)
        gain = dVal;
    else if (curHealth != maxHealth)
        gain = maxHealth - curHealth;

    return gain;
}

int32 Unit::ModifyPower(Powers power, int32 dVal)
{
    int32 gain = 0;

    int32 curPower = (int32)GetPower(power);

    // exception for eclipse, only one power which can be < 0
    if (power == POWER_ECLIPSE)
    {
        SetPower(power, dVal + curPower);
        return 0;
    }

    if (dVal == 0)
        return 0;

    int32 val = dVal + curPower;
    if (val <= 0)
    {
        SetPower(power,0);
        return -curPower;
    }

    int32 maxPower = (int32)GetMaxPower(power);

    if (val < maxPower)
    {
        SetPower(power,val);
        gain = val - curPower;
    }
    else if (curPower != maxPower)
    {
        SetPower(power,maxPower);
        gain = maxPower - curPower;
    }

    return gain;
}

bool Unit::isVisibleForOrDetect(Unit const* u, bool detect, bool inVisibleList, bool is3dDistance) const
{
    if (!u || !IsInMap(u))
        return false;

    return u->canSeeOrDetect(this, detect, inVisibleList, is3dDistance);
}

bool Unit::canSeeOrDetect(Unit const* /*u*/, bool /*detect*/, bool /*inVisibleList*/, bool /*is3dDistance*/) const
{
    return true;
}

bool Unit::canDetectInvisibilityOf(Unit const* u) const
{
    if (m_invisibilityMask & u->m_invisibilityMask) // same group
    {
        // Mage's Invisibility spell is exception - they cannot detect each other
        if (HasAura(32612) && u->HasAura(32612))
            return false;

        return true;
    }

    AuraEffectList const& auras = u->GetAuraEffectsByType(SPELL_AURA_MOD_STALKED); // Hunter mark
    for (AuraEffectList::const_iterator iter = auras.begin(); iter != auras.end(); ++iter)
        if ((*iter)->GetCasterGUID() == GetGUID())
            return true;

    if (uint32 mask = (m_detectInvisibilityMask & u->m_invisibilityMask))
    {
        for (uint8 i = 0; i < 10; ++i)
        {
            if (((1 << i) & mask) == 0)
                continue;

            // find invisibility level
            uint32 invLevel = 0;
            Unit::AuraEffectList const& iAuras = u->GetAuraEffectsByType(SPELL_AURA_MOD_INVISIBILITY);
            for (Unit::AuraEffectList::const_iterator itr = iAuras.begin(); itr != iAuras.end(); ++itr)
                if (uint8((*itr)->GetMiscValue()) == i && int32(invLevel) < (*itr)->GetAmount())
                    invLevel = (*itr)->GetAmount();

            // find invisibility detect level
            uint32 detectLevel = 0;
            if (i == 6 && GetTypeId() == TYPEID_PLAYER)          // special drunk detection case
            {
                detectLevel = this->ToPlayer()->GetDrunkValue();
            }
            else
            {
                Unit::AuraEffectList const& dAuras = GetAuraEffectsByType(SPELL_AURA_MOD_INVISIBILITY_DETECTION);
                for (Unit::AuraEffectList::const_iterator itr = dAuras.begin(); itr != dAuras.end(); ++itr)
                    if (uint8((*itr)->GetMiscValue()) == i && int32(detectLevel) < (*itr)->GetAmount())
                        detectLevel = (*itr)->GetAmount();
            }

            if (invLevel <= detectLevel)
                return true;
        }
    }

    return false;
}

bool Unit::canDetectStealthOf(Unit const* target, float distance) const
{
    if (HasUnitState(UNIT_STATE_STUNNED))
        return false;
    if (distance < 0.24f) //collision
        return true;
    if (!HasInArc(M_PI, target)) //behind
        return false;
    if (HasAuraType(SPELL_AURA_DETECT_STEALTH))
        return true;

    AuraEffectList const &auras = target->GetAuraEffectsByType(SPELL_AURA_MOD_STALKED); // Hunter mark
    for (AuraEffectList::const_iterator iter = auras.begin(); iter != auras.end(); ++iter)
        if ((*iter)->GetCasterGUID() == GetGUID())
            return true;

    //Visible distance based on stealth value (stealth rank 4 300MOD, 10.5 - 3 = 7.5)
    float visibleDistance = 7.5f;
    //Visible distance is modified by -Level Diff (every level diff = 1.0f in visible distance)
    visibleDistance += float(getLevelForTarget(target)) - target->GetTotalAuraModifier(SPELL_AURA_MOD_STEALTH)/5.0f;
    //-Stealth Mod(positive like Master of Deception) and Stealth Detection(negative like paranoia)
    //based on wowwiki every 5 mod we have 1 more level diff in calculation
    visibleDistance += (float)(GetTotalAuraModifier(SPELL_AURA_MOD_DETECT) - target->GetTotalAuraModifier(SPELL_AURA_MOD_STEALTH_LEVEL)) / 5.0f;
    visibleDistance = visibleDistance > MAX_PLAYER_STEALTH_DETECT_RANGE ? MAX_PLAYER_STEALTH_DETECT_RANGE : visibleDistance;

    return distance < visibleDistance;
}

void Unit::SetVisibility(UnitVisibility x)
{
    m_Visibility = x;

    if (m_Visibility == VISIBILITY_GROUP_STEALTH)
        HideFromNonDetectingTargets();

    UpdateObjectVisibility();
}

void Unit::HideFromNonDetectingTargets()
{
    using namespace Trinity;

    std::list<Player*> targets;
    auto visDistance = GetMap()->GetVisibilityDistance();
    AnyPlayerInObjectRangeCheck check(this, visDistance);
    PlayerListSearcher<AnyPlayerInObjectRangeCheck> searcher(this, targets, check);
    VisitNearbyWorldObject(visDistance, searcher);

    for (Player *player : targets)
    {
        if (player == this || !player->HaveAtClient(this))
            continue;

        if (GetCharmer() == player)
            continue;

        if (player->canSeeOrDetect(this, true))
            continue;

        HideForPlayer(player);
        player->m_clientGUIDs.erase(GetGUID());
    }
}

bool Unit::SetWalk(bool enable)
{
    if (enable == IsWalking())
        return false;

    if (enable)
        AddUnitMovementFlag(MOVEMENTFLAG_WALKING);
    else
        RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);

    return true;
}

void Unit::UpdateSpeed(UnitMoveType mtype, bool forced)
{
    int32 main_speed_mod  = 0;
    float stack_bonus     = 1.0f;
    float non_stack_bonus = 1.0f;

    switch(mtype)
    {
        // Only apply debuffs
        case MOVE_FLIGHT_BACK:
        case MOVE_RUN_BACK:
        case MOVE_SWIM_BACK:
            break;
        case MOVE_WALK:
            return;
        case MOVE_RUN:
        {
            if (IsMounted()) // Use on mount auras
            {
                main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED);
                stack_bonus     = GetTotalAuraMultiplier(SPELL_AURA_MOD_MOUNTED_SPEED_ALWAYS);
                non_stack_bonus = (100.0f + GetMaxPositiveAuraModifier(SPELL_AURA_MOD_MOUNTED_SPEED_NOT_STACK))/100.0f;
            }
            else
            {
                main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_SPEED) + GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_SPEED_SPECIAL);
                stack_bonus     = GetTotalAuraMultiplier(SPELL_AURA_MOD_SPEED_ALWAYS);
                non_stack_bonus = (100.0f + GetMaxPositiveAuraModifier(SPELL_AURA_MOD_SPEED_NOT_STACK))/100.0f;
            }
            break;
        }
        case MOVE_SWIM:
        {
            main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_SWIM_SPEED);
            break;
        }
        case MOVE_FLIGHT:
        {
            if (GetTypeId() == TYPEID_UNIT && IsControlledByPlayer()) // not sure if good for pet
            {
                main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED);
                stack_bonus     = GetTotalAuraMultiplier(SPELL_AURA_MOD_VEHICLE_SPEED_ALWAYS);

                // for some spells this mod is applied on vehicle owner
                int32 owner_speed_mod = 0;

                if (Unit * owner = GetCharmer())
                    owner_speed_mod = owner->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED);

                main_speed_mod = std::max(main_speed_mod, owner_speed_mod);
            }
            else if (IsMounted() || GetShapeshiftForm() > FORM_NONE)
            {
                main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED);
                stack_bonus     = GetTotalAuraMultiplier(SPELL_AURA_MOD_MOUNTED_FLIGHT_SPEED_ALWAYS);
            }
            else             // Use not mount nor shapeshift auras (should stack)
                main_speed_mod  = GetTotalAuraModifier(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED) + GetTotalAuraModifier(SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED);

            non_stack_bonus = (100.0f + GetMaxPositiveAuraModifier(SPELL_AURA_MOD_FLIGHT_SPEED_NOT_STACK))/100.0f;

            // Update speed for vehicle if available
            if (GetTypeId() == TYPEID_PLAYER && GetVehicle())
                GetVehicleBase()->UpdateSpeed(MOVE_FLIGHT, true);
            break;
        }
        default:
            sLog->outError("Unit::UpdateSpeed: Unsupported move type (%d)", mtype);
            return;
    }

    float bonus = non_stack_bonus > stack_bonus ? non_stack_bonus : stack_bonus;

    // Elusiveness (Racial Passive)
    if (AuraEffect * aurEff = GetAuraEffect(21009,EFFECT_0))
    if (HasStealthAura())
        main_speed_mod += aurEff->GetAmount();

    // now we ready for speed calculation
    float speed  = main_speed_mod ? bonus*(100.0f + main_speed_mod)/100.0f : bonus;

    switch(mtype)
    {
        case MOVE_RUN:
        case MOVE_SWIM:
        case MOVE_FLIGHT:
        {
            // Set creature speed rate from CreatureInfo
            if (GetTypeId() == TYPEID_UNIT)
                speed *= this->ToCreature()->GetCreatureInfo()->speed_walk;

            // Normalize speed by 191 aura SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED if need
            // TODO: possible affect only on MOVE_RUN
            if (int32 normalization = GetMaxPositiveAuraModifier(SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED))
            {
                // Use speed from aura
                float max_speed = normalization / (IsControlledByPlayer() ? playerBaseMoveSpeed[mtype] : baseMoveSpeed[mtype]);
                if (speed > max_speed)
                    speed = max_speed;
            }
            break;
        }
        default:
            break;
    }

    // for creature case, we check explicit if mob searched for assistance
    if (GetTypeId() == TYPEID_UNIT)
    {
        if (this->ToCreature()->HasSearchedAssistance())
            speed *= 0.66f;                                 // best guessed value, so this will be 33% reduction. Based off initial speed, mob can then "run", "walk fast" or "walk".
    }

    // Apply strongest slow aura mod to speed
    int32 slow = GetMaxNegativeAuraModifier(SPELL_AURA_MOD_DECREASE_SPEED);
    if (slow)
    {
        speed *=(100.0f + slow)/100.0f;
        if (float minSpeedMod = (float)GetMaxPositiveAuraModifier(SPELL_AURA_MOD_MINIMUM_SPEED))
        {
            float min_speed = minSpeedMod / 100.0f;
            if (speed < min_speed)
                speed = min_speed;
        }
    }
    SetSpeed(mtype, speed, forced);
}

float Unit::GetSpeed(UnitMoveType mtype) const
{
    return m_speed_rate[mtype]*(IsControlledByPlayer() ? playerBaseMoveSpeed[mtype] : baseMoveSpeed[mtype]);
}

void Unit::SetSpeed(UnitMoveType mtype, float rate, bool forced)
{
    if (rate < 0.0001f)
        rate = 0.0001f;

    // Update speed only on change
    if (m_speed_rate[mtype] == rate && !forced)
        return;

    m_speed_rate[mtype] = rate;

    propagateSpeedChange();

    WorldPacket data;
    ObjectGuid guid = GetGUID();
    if (!forced)
    {
        switch (mtype)
        {
            case MOVE_WALK:
                data.Initialize(SMSG_SPLINE_MOVE_SET_WALK_SPEED, 8+4+2+4+4+4+4+4+4+4);
                data.WriteBit(guid[0]);
                data.WriteBit(guid[6]);
                data.WriteBit(guid[7]);
                data.WriteBit(guid[3]);
                data.WriteBit(guid[5]);
                data.WriteBit(guid[1]);
                data.WriteBit(guid[2]);
                data.WriteBit(guid[4]);
                data.FlushBits();
                data.WriteByteSeq(guid[0]);
                data.WriteByteSeq(guid[4]);
                data.WriteByteSeq(guid[7]);
                data.WriteByteSeq(guid[1]);
                data.WriteByteSeq(guid[5]);
                data.WriteByteSeq(guid[3]);
                data << float(GetSpeed(mtype));
                data.WriteByteSeq(guid[2]);
                data.WriteByteSeq(guid[5]);
                break;
            case MOVE_RUN:
                data.Initialize(SMSG_SPLINE_MOVE_SET_RUN_SPEED, 1 + 8 + 4);
                data.WriteBit(guid[4]);
                data.WriteBit(guid[0]);
                data.WriteBit(guid[5]);
                data.WriteBit(guid[7]);
                data.WriteBit(guid[6]);
                data.WriteBit(guid[3]);
                data.WriteBit(guid[1]);
                data.WriteBit(guid[2]);
                data.FlushBits();
                data.WriteByteSeq(guid[0]);
                data.WriteByteSeq(guid[7]);
                data.WriteByteSeq(guid[6]);
                data.WriteByteSeq(guid[5]);
                data.WriteByteSeq(guid[3]);
                data.WriteByteSeq(guid[4]);
                data << float(GetSpeed(mtype));
                data.WriteByteSeq(guid[2]);
                data.WriteByteSeq(guid[1]);
                break;
            case MOVE_RUN_BACK:
                data.Initialize(SMSG_SPLINE_MOVE_SET_RUN_BACK_SPEED, 1 + 8 + 4);
                data.WriteBit(guid[1]);
                data.WriteBit(guid[2]);
                data.WriteBit(guid[6]);
                data.WriteBit(guid[0]);
                data.WriteBit(guid[3]);
                data.WriteBit(guid[7]);
                data.WriteBit(guid[5]);
                data.WriteBit(guid[4]);
                data.FlushBits();
                data.WriteByteSeq(guid[1]);
                data << float(GetSpeed(mtype));
                data.WriteByteSeq(guid[2]);
                data.WriteByteSeq(guid[4]);
                data.WriteByteSeq(guid[0]);
                data.WriteByteSeq(guid[3]);
                data.WriteByteSeq(guid[6]);
                data.WriteByteSeq(guid[5]);
                data.WriteByteSeq(guid[7]);
                break;
            case MOVE_SWIM:
                data.Initialize(SMSG_SPLINE_MOVE_SET_SWIM_SPEED, 1 + 8 + 4);
                data.WriteBit(guid[4]);
                data.WriteBit(guid[2]);
                data.WriteBit(guid[5]);
                data.WriteBit(guid[0]);
                data.WriteBit(guid[7]);
                data.WriteBit(guid[6]);
                data.WriteBit(guid[3]);
                data.WriteBit(guid[1]);
                data.FlushBits();
                data.WriteByteSeq(guid[5]);
                data.WriteByteSeq(guid[6]);
                data.WriteByteSeq(guid[1]);
                data.WriteByteSeq(guid[0]);
                data.WriteByteSeq(guid[2]);
                data.WriteByteSeq(guid[4]);
                data << float(GetSpeed(mtype));
                data.WriteByteSeq(guid[7]);
                data.WriteByteSeq(guid[3]);
                break;
            case MOVE_SWIM_BACK:
                data.Initialize(SMSG_SPLINE_MOVE_SET_SWIM_BACK_SPEED, 1 + 8 + 4);
                data.WriteBit(guid[0]);
                data.WriteBit(guid[1]);
                data.WriteBit(guid[3]);
                data.WriteBit(guid[6]);
                data.WriteBit(guid[4]);
                data.WriteBit(guid[5]);
                data.WriteBit(guid[7]);
                data.WriteBit(guid[2]);
                data.FlushBits();
                data.WriteByteSeq(guid[5]);
                data.WriteByteSeq(guid[3]);
                data.WriteByteSeq(guid[1]);
                data.WriteByteSeq(guid[0]);
                data.WriteByteSeq(guid[7]);
                data.WriteByteSeq(guid[6]);
                data << float(GetSpeed(mtype));
                data.WriteByteSeq(guid[4]);
                data.WriteByteSeq(guid[2]);
                break;
            case MOVE_TURN_RATE:
                data.Initialize(SMSG_SPLINE_MOVE_SET_TURN_RATE, 1 + 8 + 4);
                data.WriteBit(guid[2]);
                data.WriteBit(guid[4]);
                data.WriteBit(guid[6]);
                data.WriteBit(guid[1]);
                data.WriteBit(guid[3]);
                data.WriteBit(guid[5]);
                data.WriteBit(guid[7]);
                data.WriteBit(guid[0]);
                data.FlushBits();
                data << float(GetSpeed(mtype));
                data.WriteByteSeq(guid[1]);
                data.WriteByteSeq(guid[5]);
                data.WriteByteSeq(guid[3]);
                data.WriteByteSeq(guid[2]);
                data.WriteByteSeq(guid[7]);
                data.WriteByteSeq(guid[4]);
                data.WriteByteSeq(guid[6]);
                data.WriteByteSeq(guid[0]);
                break;
             case MOVE_FLIGHT:
                data.Initialize(SMSG_SPLINE_MOVE_SET_FLIGHT_SPEED, 1 + 8 + 4);
                data.WriteBit(guid[7]);
                data.WriteBit(guid[4]);
                data.WriteBit(guid[0]);
                data.WriteBit(guid[1]);
                data.WriteBit(guid[3]);
                data.WriteBit(guid[6]);
                data.WriteBit(guid[5]);
                data.WriteBit(guid[2]);
                data.FlushBits();
                data.WriteByteSeq(guid[0]);
                data.WriteByteSeq(guid[5]);
                data.WriteByteSeq(guid[4]);
                data.WriteByteSeq(guid[7]);
                data.WriteByteSeq(guid[3]);
                data.WriteByteSeq(guid[2]);
                data.WriteByteSeq(guid[1]);
                data.WriteByteSeq(guid[6]);
                data << float(GetSpeed(mtype));
                break;
            case MOVE_FLIGHT_BACK:
                data.Initialize(SMSG_SPLINE_MOVE_SET_FLIGHT_BACK_SPEED, 1 + 8 + 4);
                data.WriteBit(guid[2]);
                data.WriteBit(guid[1]);
                data.WriteBit(guid[6]);
                data.WriteBit(guid[5]);
                data.WriteBit(guid[0]);
                data.WriteBit(guid[3]);
                data.WriteBit(guid[4]);
                data.WriteBit(guid[7]);
                data.FlushBits();
                data.WriteByteSeq(guid[5]);
                data << float(GetSpeed(mtype));
                data.WriteByteSeq(guid[6]);
                data.WriteByteSeq(guid[1]);
                data.WriteByteSeq(guid[0]);
                data.WriteByteSeq(guid[2]);
                data.WriteByteSeq(guid[3]);
                data.WriteByteSeq(guid[7]);
                data.WriteByteSeq(guid[4]);
                break;
            case MOVE_PITCH_RATE:
                data.Initialize(SMSG_SPLINE_MOVE_SET_PITCH_RATE, 1 + 8 + 4);
                data.WriteBit(guid[3]);
                data.WriteBit(guid[5]);
                data.WriteBit(guid[6]);
                data.WriteBit(guid[1]);
                data.WriteBit(guid[0]);
                data.WriteBit(guid[4]);
                data.WriteBit(guid[7]);
                data.WriteBit(guid[2]);
                data.FlushBits();
                data.WriteByteSeq(guid[1]);
                data.WriteByteSeq(guid[5]);
                data.WriteByteSeq(guid[7]);
                data.WriteByteSeq(guid[0]);
                data.WriteByteSeq(guid[6]);
                data.WriteByteSeq(guid[3]);
                data.WriteByteSeq(guid[2]);
                data << float(GetSpeed(mtype));
                data.WriteByteSeq(guid[4]);
                break;
            default:
                sLog->outError("Unit::SetSpeed: Unsupported move type (%d), data not sent to client.",mtype);
                return;
        }

        SendMessageToSet(&data, true);
    }
    else
    {
        if (GetTypeId() == TYPEID_PLAYER)
        {
            // register forced speed changes for WorldSession::HandleForceSpeedChangeAck
            // and do it only for real sent packets and use run for run/mounted as client expected
            ++this->ToPlayer()->m_forced_speed_changes[mtype];

            if (!IsInCombat())
                if (Pet* pet = this->ToPlayer()->GetPet())
                    pet->SetSpeed(mtype, m_speed_rate[mtype], forced);
        }

        switch (mtype)
        {
            case MOVE_WALK:
                data.Initialize(MSG_MOVE_SET_WALK_SPEED, 1 + 8 + 4 + 4);
                data.WriteBit(guid[0]);
                data.WriteBit(guid[4]);
                data.WriteBit(guid[5]);
                data.WriteBit(guid[2]);
                data.WriteBit(guid[3]);
                data.WriteBit(guid[1]);
                data.WriteBit(guid[6]);
                data.WriteBit(guid[7]);
                data.WriteByteSeq(guid[6]);
                data.WriteByteSeq(guid[1]);
                data.WriteByteSeq(guid[5]);
                data << float(GetSpeed(mtype));
                data.WriteByteSeq(guid[2]);
                data << uint32(0);
                data.WriteByteSeq(guid[4]);
                data.WriteByteSeq(guid[0]);
                data.WriteByteSeq(guid[7]);
                data.WriteByteSeq(guid[3]);
                break;
            case MOVE_RUN:
                data.Initialize(MSG_MOVE_SET_RUN_SPEED, 1 + 8 + 4 + 4);
                data.WriteBit(guid[6]);
                data.WriteBit(guid[1]);
                data.WriteBit(guid[5]);
                data.WriteBit(guid[2]);
                data.WriteBit(guid[7]);
                data.WriteBit(guid[0]);
                data.WriteBit(guid[3]);
                data.WriteBit(guid[4]);
                data.WriteByteSeq(guid[5]);
                data.WriteByteSeq(guid[3]);
                data.WriteByteSeq(guid[1]);
                data.WriteByteSeq(guid[4]);
                data << uint32(0);
                data << float(GetSpeed(mtype));
                data.WriteByteSeq(guid[6]);
                data.WriteByteSeq(guid[0]);
                data.WriteByteSeq(guid[7]);
                data.WriteByteSeq(guid[2]);
                break;
            case MOVE_RUN_BACK:
                data.Initialize(MSG_MOVE_SET_RUN_BACK_SPEED, 1 + 8 + 4 + 4);
                data.WriteBit(guid[0]);
                data.WriteBit(guid[6]);
                data.WriteBit(guid[2]);
                data.WriteBit(guid[1]);
                data.WriteBit(guid[3]);
                data.WriteBit(guid[4]);
                data.WriteBit(guid[5]);
                data.WriteBit(guid[7]);
                data.WriteByteSeq(guid[5]);
                data << uint32(0);
                data << float(GetSpeed(mtype));
                data.WriteByteSeq(guid[0]);
                data.WriteByteSeq(guid[4]);
                data.WriteByteSeq(guid[7]);
                data.WriteByteSeq(guid[3]);
                data.WriteByteSeq(guid[1]);
                data.WriteByteSeq(guid[2]);
                data.WriteByteSeq(guid[6]);
                break;
            case MOVE_SWIM:
                data.Initialize(MSG_MOVE_SET_SWIM_SPEED, 1 + 8 + 4 + 4);
                data.WriteBit(guid[5]);
                data.WriteBit(guid[4]);
                data.WriteBit(guid[7]);
                data.WriteBit(guid[3]);
                data.WriteBit(guid[2]);
                data.WriteBit(guid[0]);
                data.WriteBit(guid[1]);
                data.WriteBit(guid[6]);
                data.WriteByteSeq(guid[0]);
                data << uint32(0);
                data.WriteByteSeq(guid[6]);
                data.WriteByteSeq(guid[3]);
                data.WriteByteSeq(guid[5]);
                data.WriteByteSeq(guid[2]);
                data << float(GetSpeed(mtype));
                data.WriteByteSeq(guid[1]);
                data.WriteByteSeq(guid[7]);
                data.WriteByteSeq(guid[4]);
                break;
            case MOVE_SWIM_BACK:
                data.Initialize(MSG_MOVE_SET_SWIM_BACK_SPEED, 1 + 8 + 4 + 4);
                data.WriteBit(guid[4]);
                data.WriteBit(guid[2]);
                data.WriteBit(guid[3]);
                data.WriteBit(guid[6]);
                data.WriteBit(guid[5]);
                data.WriteBit(guid[1]);
                data.WriteBit(guid[0]);
                data.WriteBit(guid[7]);
                data << uint32(0);
                data.WriteByteSeq(guid[0]);
                data.WriteByteSeq(guid[3]);
                data.WriteByteSeq(guid[4]);
                data.WriteByteSeq(guid[6]);
                data.WriteByteSeq(guid[5]);
                data.WriteByteSeq(guid[1]);
                data << float(GetSpeed(mtype));
                data.WriteByteSeq(guid[7]);
                data.WriteByteSeq(guid[2]);
                break;
            case MOVE_TURN_RATE:
                data.Initialize(MSG_MOVE_SET_TURN_RATE, 1 + 8 + 4 + 4);
                data.WriteBit(guid[7]);
                data.WriteBit(guid[2]);
                data.WriteBit(guid[1]);
                data.WriteBit(guid[0]);
                data.WriteBit(guid[4]);
                data.WriteBit(guid[5]);
                data.WriteBit(guid[6]);
                data.WriteBit(guid[3]);
                data.WriteByteSeq(guid[5]);
                data.WriteByteSeq(guid[7]);
                data.WriteByteSeq(guid[2]);
                data << float(GetSpeed(mtype));
                data.WriteByteSeq(guid[3]);
                data.WriteByteSeq(guid[1]);
                data.WriteByteSeq(guid[0]);
                data << uint32(0);
                data.WriteByteSeq(guid[6]);
                data.WriteByteSeq(guid[4]);
                break;
            case MOVE_FLIGHT:
                data.Initialize(MSG_MOVE_SET_FLIGHT_SPEED, 1 + 8 + 4 + 4);
                data.WriteBit(guid[0]);
                data.WriteBit(guid[5]);
                data.WriteBit(guid[1]);
                data.WriteBit(guid[6]);
                data.WriteBit(guid[3]);
                data.WriteBit(guid[2]);
                data.WriteBit(guid[7]);
                data.WriteBit(guid[4]);
                data.WriteByteSeq(guid[0]);
                data.WriteByteSeq(guid[1]);
                data.WriteByteSeq(guid[7]);
                data.WriteByteSeq(guid[5]);
                data << float(GetSpeed(mtype));
                data << uint32(0);
                data.WriteByteSeq(guid[2]);
                data.WriteByteSeq(guid[6]);
                data.WriteByteSeq(guid[3]);
                data.WriteByteSeq(guid[4]);
                break;
            case MOVE_FLIGHT_BACK:
                data.Initialize(MSG_MOVE_SET_FLIGHT_BACK_SPEED, 1 + 8 + 4 + 4);
                data.WriteBit(guid[1]);
                data.WriteBit(guid[2]);
                data.WriteBit(guid[6]);
                data.WriteBit(guid[4]);
                data.WriteBit(guid[7]);
                data.WriteBit(guid[3]);
                data.WriteBit(guid[0]);
                data.WriteBit(guid[5]);
                data.WriteByteSeq(guid[3]);
                data << uint32(0);
                data.WriteByteSeq(guid[6]);
                data << float(GetSpeed(mtype));
                data.WriteByteSeq(guid[1]);
                data.WriteByteSeq(guid[2]);
                data.WriteByteSeq(guid[4]);
                data.WriteByteSeq(guid[0]);
                data.WriteByteSeq(guid[5]);
                data.WriteByteSeq(guid[7]);
                break;
            case MOVE_PITCH_RATE:
                data.Initialize(MSG_MOVE_SET_PITCH_RATE, 1 + 8 + 4 + 4);
                data.WriteBit(guid[1]);
                data.WriteBit(guid[2]);
                data.WriteBit(guid[6]);
                data.WriteBit(guid[7]);
                data.WriteBit(guid[0]);
                data.WriteBit(guid[3]);
                data.WriteBit(guid[5]);
                data.WriteBit(guid[4]);
                data << float(GetSpeed(mtype));
                data.WriteByteSeq(guid[6]);
                data.WriteByteSeq(guid[4]);
                data.WriteByteSeq(guid[0]);
                data << uint32(0);
                data.WriteByteSeq(guid[1]);
                data.WriteByteSeq(guid[2]);
                data.WriteByteSeq(guid[7]);
                data.WriteByteSeq(guid[3]);
                data.WriteByteSeq(guid[5]);
                break;
            default:
                sLog->outError("Unit::SetSpeed: Unsupported move type (%d), data not sent to client.",mtype);
                return;
        }
        SendMessageToSet(&data, true);
    }
}

void Unit::SetHover(bool on)
{
    if (on)
        CastSpell(this, 11010, true);
    else
        RemoveAurasDueToSpell(11010);
}

void Unit::RemoveCamouflage()
{
    RemoveAurasDueToSpell(80325);
    RemoveAurasDueToSpell(51755);
    RemoveAurasDueToSpell(80326);

    if (GetTypeId() == TYPEID_PLAYER)
    {
        if (Unit* pPet = Unit::GetUnit(*this,GetPetGUID()))
        {
            pPet->RemoveAurasDueToSpell(51755);
            pPet->RemoveAurasDueToSpell(80326);
            pPet->RemoveAurasDueToSpell(80325);
        }
    }
    else
    {
        if (Unit* pHunter = Unit::GetUnit(*this,GetOwnerGUID()))
        {
            pHunter->RemoveAurasDueToSpell(51755);
            pHunter->RemoveAurasDueToSpell(80326);
            pHunter->RemoveAurasDueToSpell(80325);
        }
    }
}

void Unit::setDeathState(DeathState s)
{
    // death state needs to be updated before RemoveAllAurasOnDeath() calls HandleChannelDeathItem(..) so that
    // it can be used to check creation of death items (such as soul shards).
    DeathState oldDeathState = m_deathState;
    m_deathState = s;

    if (s != ALIVE && s != JUST_ALIVED)
    {
        CombatStop();
        DeleteThreatList();
        getHostileRefManager().deleteReferences();

        if (IsNonMeleeSpellCasted(false))
            InterruptNonMeleeSpells(false);

        ExitVehicle();                                      // Exit vehicle before calling RemoveAllControlled
                                                            // vehicles use special type of charm that is not removed by the next function
                                                            // triggering an assert

        UnsummonAllTotems();
        RemoveAllControlled();
        RemoveAllAurasOnDeath();
    }
    if (s == ALIVE)
    {
        ClearComboPointHolders();                           // any combo points pointed to unit lost at it death
        ClearAllDamageTaken();
    }

    if (s == JUST_DIED)
    {
        ModifyAuraState(AURA_STATE_HEALTHLESS_20_PERCENT, false);
        ModifyAuraState(AURA_STATE_HEALTHLESS_35_PERCENT, false);
        // remove aurastates allowing special moves
        ClearAllReactives();
        ClearDiminishings();
        GetMotionMaster()->Clear(false);
        GetMotionMaster()->MoveIdle();
        if (m_vehicleKit)
            m_vehicleKit->Die();
        StopMoving();
        //without this when removing IncreaseMaxHealth aura player may stuck with 1 hp
        //do not why since in IncreaseMaxHealth currenthealth is checked
        SetHealth(0);
        DisableSpline();
        SetPower(getPowerType(),0);
    }
    else if (s == JUST_ALIVED)
        RemoveFlag (UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE); // clear skinnable for creature and player (at battleground)

    if (oldDeathState != ALIVE && s == ALIVE)
    {
        // Reset display id on resurection - needed by corpse explosion to cleanup after display change
        // TODO: fix this
        if (!HasAuraType(SPELL_AURA_TRANSFORM))
            SetDisplayId(GetNativeDisplayId());
    }
}

/*########################################
########                          ########
########       AGGRO SYSTEM       ########
########                          ########
########################################*/
bool Unit::CanHaveThreatList() const
{
    // only creatures can have threat list
    if (GetTypeId() != TYPEID_UNIT)
        return false;

    // only alive units can have threat list
    if (!IsAlive() || isDying())
        return false;

    // totems can not have threat list
    if (this->ToCreature()->IsTotem())
        return false;

    // vehicles can not have threat list
    //if (this->ToCreature()->IsVehicle())
    //    return false;

    // summons can not have a threat list, unless they are controlled by a creature
    if (HasUnitTypeMask(UNIT_MASK_MINION))
    {
        assert(dynamic_cast<Minion*>(const_cast<Unit*>(this)));
        if(IS_PLAYER_GUID(((Minion*)this)->GetOwnerGUID()))
            return false;
    }

    if(HasUnitTypeMask(UNIT_MASK_GUARDIAN | UNIT_MASK_CONTROLABLE_GUARDIAN))
    {
        assert(dynamic_cast<Guardian*>(const_cast<Unit*>(this)));
        if(IS_PLAYER_GUID(((Guardian*)this)->GetOwnerGUID()))
            return false;
    }

    if(this->ToCreature()->IsPet())
    {
        assert(dynamic_cast<Pet*>(const_cast<Unit*>(this)));
        if(IS_PLAYER_GUID(((Pet*)this)->GetOwnerGUID()))
            return false;
    }

    return true;
}

//======================================================================

float Unit::ApplyTotalThreatModifier(float fThreat, SpellSchoolMask schoolMask)
{
    if (!HasAuraType(SPELL_AURA_MOD_THREAT) || fThreat < 0)
        return fThreat;

    SpellSchools school = GetFirstSchoolInMask(schoolMask);

    return fThreat * m_threatModifier[school];
}

//======================================================================

void Unit::AddThreat(Unit* pVictim, float fThreat, SpellSchoolMask schoolMask, SpellEntry const *threatSpell)
{
    // Only mobs can manage threat lists
    if (CanHaveThreatList())
        m_ThreatManager.addThreat(pVictim, fThreat, schoolMask, threatSpell);
}

//======================================================================

void Unit::DeleteThreatList()
{
    if (CanHaveThreatList() && !m_ThreatManager.isThreatListEmpty())
        SendClearThreatListOpcode();
    m_ThreatManager.clearReferences();
}

//======================================================================

void Unit::TauntApply(Unit* taunter)
{
    ASSERT(GetTypeId() == TYPEID_UNIT);

    if (!taunter || (taunter->GetTypeId() == TYPEID_PLAYER && taunter->ToPlayer()->IsGameMaster()))
        return;

    if (!CanHaveThreatList())
        return;

    if (this->ToCreature()->HasReactState(REACT_PASSIVE))
        return;

    Unit *target = GetVictim();
    if (target && target == taunter)
        return;

    SetInFront(taunter);
    if (this->ToCreature()->IsAIEnabled)
        this->ToCreature()->AI()->AttackStart(taunter);

    //m_ThreatManager.tauntApply(taunter);
}

//======================================================================

void Unit::TauntFadeOut(Unit *taunter)
{
    ASSERT(GetTypeId() == TYPEID_UNIT);

    if (!taunter || (taunter->GetTypeId() == TYPEID_PLAYER && taunter->ToPlayer()->IsGameMaster()))
        return;

    if (!CanHaveThreatList())
        return;

    if (this->ToCreature()->HasReactState(REACT_PASSIVE))
        return;

    Unit *target = GetVictim();
    if (!target || target != taunter)
        return;

    if (m_ThreatManager.isThreatListEmpty())
    {
        if (this->ToCreature()->IsAIEnabled)
            this->ToCreature()->AI()->EnterEvadeMode();
        return;
    }

    //m_ThreatManager.tauntFadeOut(taunter);
    target = m_ThreatManager.getHostilTarget();

    if (target && target != taunter)
    {
        SetInFront(target);
        if (this->ToCreature()->IsAIEnabled)
            this->ToCreature()->AI()->AttackStart(target);
    }
}

//======================================================================

Unit* Creature::SelectVictim()
{
    //function provides main threat functionality
    //next-victim-selection algorithm and evade mode are called
    //threat list sorting etc.

    Unit* target = NULL;
    // First checking if we have some taunt on us
    const AuraEffectList& tauntAuras = GetAuraEffectsByType(SPELL_AURA_MOD_TAUNT);
    if (!tauntAuras.empty())
    {
        Unit* caster = tauntAuras.back()->GetCaster();

        // The last taunt aura caster is alive an we are happy to attack him
        if (caster && caster->IsAlive())
            return GetVictim();
        else if (tauntAuras.size() > 1)
        {
            // We do not have last taunt aura caster but we have more taunt auras,
            // so find first available target

            // Auras are pushed_back, last caster will be on the end
            AuraEffectList::const_iterator aura = --tauntAuras.end();
            do
            {
                --aura;
                caster = (*aura)->GetCaster();
                if (caster && caster->IsInMap(this) && canAttack(caster) && caster->isInAccessiblePlaceFor(this->ToCreature()))
                {
                    target = caster;
                    break;
                }
            } while (aura != tauntAuras.begin());
        }
        else
            target = GetVictim();
    }

    if (CanHaveThreatList())
    {
        if (!target && !m_ThreatManager.isThreatListEmpty())
            // No taunt aura or taunt aura caster is dead standard target selection
            target = m_ThreatManager.getHostilTarget();
    }
    else if (!HasReactState(REACT_PASSIVE))
    {
        // We have player pet probably
        target = getAttackerForHelper();
        if (!target && IsSummon())
        {
            if (Unit * owner = this->ToTempSummon()->GetOwner())
            {
                if (owner->IsInCombat())
                    target = owner->getAttackerForHelper();
                if (!target)
                {
                    for (ControlList::const_iterator itr = owner->m_Controlled.begin(); itr != owner->m_Controlled.end(); ++itr)
                    {
                        if ((*itr)->IsInCombat())
                        {
                            target = (*itr)->getAttackerForHelper();
                            if (target) break;
                        }
                    }
                }
            }
        }
    }
    else
        return NULL;

    if (target && _IsTargetAcceptable(target))
    {
        SetInFront(target);
        return target;
    }

    // last case when creature don't must go to evade mode:
    // it in combat but attacker not make any damage and not enter to aggro radius to have record in threat list
    // for example at owner command to pet attack some far away creature
    // Note: creature not have targeted movement generator but have attacker in this case
    for (AttackerSet::const_iterator itr = m_attackers.begin(); itr != m_attackers.end(); ++itr)
    {
        if ((*itr) && !CanCreatureAttack(*itr) && (*itr)->GetTypeId() != TYPEID_PLAYER
        && !(*itr)->ToCreature()->HasUnitTypeMask(UNIT_MASK_CONTROLABLE_GUARDIAN))
            return NULL;
    }

    // TODO: a vehicle may eat some mob, so mob should not evade
    if (GetVehicle())
        return NULL;

    // search nearby enemy before enter evade mode
    if (HasReactState(REACT_AGGRESSIVE))
    {
        target = SelectNearestTargetInAttackDistance();
        if (target && _IsTargetAcceptable(target))
                return target;
    }
    
    // Killing spree retarget (if no any other target found)
    target = ((Unit*)this)->getAttackerForHelper();
    if(target && target->HasAura(69107))
        return target;
    else
        target = NULL;

    if (m_invisibilityMask)
    {
        Unit::AuraEffectList const& iAuras = GetAuraEffectsByType(SPELL_AURA_MOD_INVISIBILITY);
        for (Unit::AuraEffectList::const_iterator itr = iAuras.begin(); itr != iAuras.end(); ++itr)
            if ((*itr)->GetBase()->IsPermanent())
            {
                AI()->EnterEvadeMode();
                break;
            }
        return NULL;
    }

    // enter in evade mode in other case
    AI()->EnterEvadeMode();

    Creature* crea = ToCreature();
    InstanceMap* map = GetMap() ? GetMap()->ToInstanceMap() : NULL;
    if (crea && crea->IsAIEnabled && map && map->GetInstanceScript() && crea->GetCreatureInfo()->rank == 3)
    {
        map->GetInstanceScript()->ResetRessurectionData();
        map->GetInstanceScript()->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, this);
    }

    return NULL;
}

//======================================================================
//======================================================================
//======================================================================

int32 Unit::ApplyEffectModifiers(SpellEntry const* spellProto, uint8 effect_index, int32 value) const
{
    if (Player* modOwner = GetSpellModOwner())
    {
        modOwner->ApplySpellMod(spellProto->Id,SPELLMOD_ALL_EFFECTS, value);
        switch (effect_index)
        {
            case 0:
                modOwner->ApplySpellMod(spellProto->Id,SPELLMOD_EFFECT1, value);
                break;
            case 1:
                modOwner->ApplySpellMod(spellProto->Id,SPELLMOD_EFFECT2, value);
                break;
            case 2:
                modOwner->ApplySpellMod(spellProto->Id,SPELLMOD_EFFECT3, value);
                break;
        }
    }
    return value;
}

// function uses real base points (typically value - 1)
int32 Unit::CalculateSpellDamage(Unit const* target, SpellEntry const* spellProto, uint8 effect_index, int32 const* basePoints) const
{
    return SpellMgr::CalculateSpellEffectAmount(spellProto, effect_index, this, basePoints, target);
}

int32 Unit::CalcSpellDuration(SpellEntry const* spellProto)
{
    uint8 comboPoints = m_movedPlayer ? m_movedPlayer->GetComboPoints() : 0;

    int32 minduration = GetSpellDuration(spellProto);
    int32 maxduration = GetSpellMaxDuration(spellProto);

    int32 duration;

    if (comboPoints && minduration != -1 && minduration != maxduration)
        duration = minduration + int32((maxduration - minduration) * comboPoints / 5);
    else
        duration = minduration;

    return duration;
}

int32 Unit::ModSpellDuration(SpellEntry const* spellProto, Unit const* target, int32 duration, bool positive)
{
    //don't mod permament auras duration
    if (duration < 0)
        return duration;

    //cut duration only of negative effects
    if (!positive)
    {
        int32 mechanic = GetAllSpellMechanicMask(spellProto);

        int32 durationMod;
        int32 durationMod_always = 0;
        int32 durationMod_not_stack = 0;

        for (uint8 i = 1; i <= MECHANIC_ENRAGED; ++i)
        {
            if (!(mechanic & 1<<i))
                continue;
            // Find total mod value (negative bonus)
            int32 new_durationMod_always = target->GetTotalAuraModifierByMiscValue(SPELL_AURA_MECHANIC_DURATION_MOD, i);
            // Find max mod (negative bonus)
            int32 new_durationMod_not_stack = target->GetMaxNegativeAuraModifierByMiscValue(SPELL_AURA_MECHANIC_DURATION_MOD_NOT_STACK, i);
            // Check if mods applied before were weaker
            if (new_durationMod_always < durationMod_always)
                durationMod_always = new_durationMod_always;
            if (new_durationMod_not_stack < durationMod_not_stack)
                durationMod_not_stack = new_durationMod_not_stack;
        }

        // Select strongest negative mod
        if (durationMod_always > durationMod_not_stack)
            durationMod = durationMod_not_stack;
        else
            durationMod = durationMod_always;

        if (durationMod != 0)
            duration = int32(float(duration) * float(100.0f+durationMod) / 100.0f);

        // there are only negative mods currently
        durationMod_always = target->GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_AURA_DURATION_BY_DISPEL, spellProto->Dispel);
        durationMod_not_stack = target->GetMaxNegativeAuraModifierByMiscValue(SPELL_AURA_MOD_AURA_DURATION_BY_DISPEL_NOT_STACK, spellProto->Dispel);

        durationMod = 0;
        if (durationMod_always > durationMod_not_stack)
            durationMod += durationMod_not_stack;
        else
            durationMod += durationMod_always;

        if (durationMod != 0)
            duration = int32(float(duration) * float(100.0f+durationMod) / 100.0f);
    }
    else
    {
        //else positive mods here, there are no currently
        //when there will be, change GetTotalAuraModifierByMiscValue to GetTotalPositiveAuraModifierByMiscValue

        // Mixology - duration boost
        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            if (spellProto->SpellFamilyName == SPELLFAMILY_POTION && (
                sSpellMgr->IsSpellMemberOfSpellGroup(spellProto->Id, SPELL_GROUP_ELIXIR_BATTLE) ||
                sSpellMgr->IsSpellMemberOfSpellGroup(spellProto->Id, SPELL_GROUP_ELIXIR_GUARDIAN) ||
                sSpellMgr->IsSpellMemberOfSpellGroup(spellProto->Id, SPELL_GROUP_FLASK)))
            {
                if (target->HasAura(53042) && target->HasSpell(spellProto->EffectTriggerSpell[0]))
                    duration *= 2;
            }
        }
    }

    return duration > 0 ? duration : 0;
}

void Unit::ModSpellCastTime(SpellEntry const* spellProto, int32 & castTime, Spell * spell)
{
    if (!spellProto || castTime < 0)
        return;
    //called from caster
    if (Player* modOwner = GetSpellModOwner())
        modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_CASTING_TIME, castTime, spell);

    if (!(spellProto->Attributes & (SPELL_ATTR0_ABILITY|SPELL_ATTR0_TRADESPELL)) && ((GetTypeId() == TYPEID_PLAYER && spellProto->SpellFamilyName) || GetTypeId() == TYPEID_UNIT))
        castTime = int32(float(castTime) * GetFloatValue(UNIT_MOD_CAST_SPEED));
    else if (spellProto->Attributes & SPELL_ATTR0_REQ_AMMO && !(spellProto->AttributesEx2 & SPELL_ATTR2_AUTOREPEAT_FLAG))
        castTime = int32(float(castTime) * m_modAttackSpeedPct[RANGED_ATTACK]);
    else if (spellProto->SpellVisual[0] == 3881 && HasAura(67556)) // cooking with Chef Hat.
        castTime = 500;
}

DiminishingLevels Unit::GetDiminishing(DiminishingGroup group)
{
    for (Diminishing::iterator i = m_Diminishing.begin(); i != m_Diminishing.end(); ++i)
    {
        if (i->DRGroup != group)
            continue;

        if (!i->hitCount)
            return DIMINISHING_LEVEL_1;

        if (!i->hitTime)
            return DIMINISHING_LEVEL_1;

        // If last spell was casted more than 15 seconds ago - reset the count.
        if (i->stack == 0 && getMSTimeDiff(i->hitTime,getMSTime()) > 15000)
        {
            i->hitCount = DIMINISHING_LEVEL_1;
            return DIMINISHING_LEVEL_1;
        }
        // or else increase the count.
        else
            return DiminishingLevels(i->hitCount);
    }
    return DIMINISHING_LEVEL_1;
}

void Unit::IncrDiminishing(DiminishingGroup group)
{
    // Checking for existing in the table
    for (Diminishing::iterator i = m_Diminishing.begin(); i != m_Diminishing.end(); ++i)
    {
        if (i->DRGroup != group)
            continue;
        if (int32(i->hitCount) < GetDiminishingReturnsMaxLevel(group))
            i->hitCount += 1;
        return;
    }
    m_Diminishing.push_back(DiminishingReturn(group,getMSTime(),DIMINISHING_LEVEL_2));
}

float Unit::ApplyDiminishingToDuration(DiminishingGroup group, int32 &duration, Unit *caster, DiminishingLevels Level, int32 limitduration)
{
    if (duration == -1 || group == DIMINISHING_NONE)
        return 1.0f;

    // test pet/charm masters instead pets/charmeds
    Unit const* targetOwner = GetCharmerOrOwner();
    Unit const* casterOwner = caster->GetCharmerOrOwner();

    // Duration of crowd control abilities on pvp target is limited by 10 sec. (2.2.0)
    if (limitduration > 0 && duration > limitduration)
    {
        Unit const* target = targetOwner ? targetOwner : this;
        Unit const* source = casterOwner ? casterOwner : caster;

        if ((target->GetTypeId() == TYPEID_PLAYER
            || ((Creature*)target)->GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_ALL_DIMINISH)
            && source->GetTypeId() == TYPEID_PLAYER)
            duration = limitduration;
    }

    float mod = 1.0f;

    if (group == DIMINISHING_TAUNT)
    {
        if (GetTypeId() == TYPEID_UNIT && (((Creature*)this)->GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_TAUNT_DIMINISH))
        {
            DiminishingLevels diminish = Level;
            switch(diminish)
            {
                case DIMINISHING_LEVEL_1: break;
                case DIMINISHING_LEVEL_2: mod = 0.65f; break;
                case DIMINISHING_LEVEL_3: mod = 0.4225f; break;
                case DIMINISHING_LEVEL_4: mod = 0.274625f; break;
                case DIMINISHING_LEVEL_TAUNT_IMMUNE: mod = 0.0f; break;
                default: break;
            }
        }
    }
    // Some diminishings applies to mobs too (for example, Stun)
    else if ((GetDiminishingReturnsGroupType(group) == DRTYPE_PLAYER
        && ((targetOwner ? (targetOwner->GetTypeId() == TYPEID_PLAYER) : (GetTypeId() == TYPEID_PLAYER))
        || (GetTypeId() == TYPEID_UNIT && ((Creature*)this)->GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_ALL_DIMINISH)))
        || GetDiminishingReturnsGroupType(group) == DRTYPE_ALL)
    {
        DiminishingLevels diminish = Level;
        switch(diminish)
        {
            case DIMINISHING_LEVEL_1: break;
            case DIMINISHING_LEVEL_2: mod = 0.5f; break;
            case DIMINISHING_LEVEL_3: mod = 0.25f; break;
            case DIMINISHING_LEVEL_IMMUNE: mod = 0.0f; break;
            default: break;
        }
    }

    duration = int32(duration * mod);
    return mod;
}

void Unit::ApplyDiminishingAura(DiminishingGroup group, bool apply)
{
    // Checking for existing in the table
    for (Diminishing::iterator i = m_Diminishing.begin(); i != m_Diminishing.end(); ++i)
    {
        if (i->DRGroup != group)
            continue;

        if (apply)
            i->stack += 1;
        else if (i->stack)
        {
            i->stack -= 1;
            // Remember time after last aura from group removed
            if (i->stack == 0)
                i->hitTime = getMSTime();
        }
        break;
    }
}

uint32 Unit::GetSpellMaxRangeForTarget(Unit* target,const SpellRangeEntry * rangeEntry)
{
    if (!rangeEntry)
        return 0;
    if (rangeEntry->maxRangeHostile == rangeEntry->maxRangeFriend)
        return uint32(rangeEntry->maxRangeFriend);
    if (IsHostileTo(target))
        return uint32(rangeEntry->maxRangeHostile);
    return uint32(rangeEntry->maxRangeFriend);
};
uint32 Unit::GetSpellMinRangeForTarget(Unit* target,const SpellRangeEntry * rangeEntry)
{
    if (!rangeEntry)
        return 0;
    if (rangeEntry->minRangeHostile == rangeEntry->minRangeFriend)
        return uint32(rangeEntry->minRangeFriend);
    if (IsHostileTo(target))
        return uint32(rangeEntry->minRangeHostile);
    return uint32(rangeEntry->minRangeFriend);
};

Unit* Unit::GetUnit(WorldObject& object, uint64 guid)
{
    return ObjectAccessor::GetUnit(object,guid);
}

Player* Unit::GetPlayer(WorldObject& object, uint64 guid)
{
    return ObjectAccessor::GetPlayer(object,guid);
}

Creature* Unit::GetCreature(WorldObject& object, uint64 guid)
{
    return object.GetMap()->GetCreature(guid);
}

bool Unit::isVisibleForInState(Player const* u, bool inVisibleList) const
{
    return u->canSeeOrDetect(this, true, inVisibleList, false);
}

uint32 Unit::GetCreatureType() const
{
    if (GetTypeId() == TYPEID_PLAYER)
    {
        ShapeshiftForm form = GetShapeshiftForm();
        SpellShapeshiftFormEntry const* ssEntry = sSpellShapeshiftFormStore.LookupEntry(form);
        if (ssEntry && ssEntry->creatureType > 0)
            return ssEntry->creatureType;
        else
            return CREATURE_TYPE_HUMANOID;
    }
    else
        return this->ToCreature()->GetCreatureInfo()->type;
}

/*#######################################
########                         ########
########       STAT SYSTEM       ########
########                         ########
#######################################*/

bool Unit::HandleStatModifier(UnitMods unitMod, UnitModifierType modifierType, float amount, bool apply)
{
    if (unitMod >= UNIT_MOD_END || modifierType >= MODIFIER_TYPE_END)
    {
        sLog->outError("ERROR in HandleStatModifier(): non existed UnitMods or wrong UnitModifierType!");
        return false;
    }

    switch (modifierType)
    {
        case BASE_VALUE:
        case TOTAL_VALUE:
            m_auraModifiersGroup[unitMod][modifierType] += apply ? amount : -amount;
            break;
        case BASE_PCT:
        case TOTAL_PCT:
            ApplyPercentModFloatVar(m_auraModifiersGroup[unitMod][modifierType], amount, apply);
            break;
        default:
            break;
    }

    if (!CanModifyStats())
        return false;

    switch(unitMod)
    {
        case UNIT_MOD_STAT_STRENGTH:
        case UNIT_MOD_STAT_AGILITY:
        case UNIT_MOD_STAT_STAMINA:
        case UNIT_MOD_STAT_INTELLECT:
        case UNIT_MOD_STAT_SPIRIT:         UpdateStats(GetStatByAuraGroup(unitMod));  break;

        case UNIT_MOD_ARMOR:               UpdateArmor();           break;
        case UNIT_MOD_HEALTH:              UpdateMaxHealth();       break;

        case UNIT_MOD_MANA:
        case UNIT_MOD_RAGE:
        case UNIT_MOD_FOCUS:
        case UNIT_MOD_ENERGY:
        case UNIT_MOD_HAPPINESS:
        case UNIT_MOD_RUNE:
        case UNIT_MOD_RUNIC_POWER:          UpdateMaxPower(GetPowerTypeByAuraGroup(unitMod));          break;

        case UNIT_MOD_RESISTANCE_HOLY:
        case UNIT_MOD_RESISTANCE_FIRE:
        case UNIT_MOD_RESISTANCE_NATURE:
        case UNIT_MOD_RESISTANCE_FROST:
        case UNIT_MOD_RESISTANCE_SHADOW:
        case UNIT_MOD_RESISTANCE_ARCANE:   UpdateResistances(GetSpellSchoolByAuraGroup(unitMod));      break;

        case UNIT_MOD_ATTACK_POWER_POS:    
        case UNIT_MOD_ATTACK_POWER_NEG:
                                           UpdateAttackPowerAndDamage();         break;
        case UNIT_MOD_ATTACK_POWER_RANGED_POS: 
        case UNIT_MOD_ATTACK_POWER_RANGED_NEG:
                                           UpdateAttackPowerAndDamage(true);     break;

        case UNIT_MOD_DAMAGE_MAINHAND:     UpdateDamagePhysical(BASE_ATTACK);    break;
        case UNIT_MOD_DAMAGE_OFFHAND:      UpdateDamagePhysical(OFF_ATTACK);     break;
        case UNIT_MOD_DAMAGE_RANGED:       UpdateDamagePhysical(RANGED_ATTACK);  break;

        default:
            break;
    }

    return true;
}

float Unit::GetModifierValue(UnitMods unitMod, UnitModifierType modifierType) const
{
    if (unitMod >= UNIT_MOD_END || modifierType >= MODIFIER_TYPE_END)
    {
        sLog->outError("trial to access non existed modifier value from UnitMods!");
        return 0.0f;
    }

    if (modifierType == TOTAL_PCT && m_auraModifiersGroup[unitMod][modifierType] <= 0.0f)
        return 0.0f;

    return m_auraModifiersGroup[unitMod][modifierType];
}

float Unit::GetTotalStatValue(Stats stat) const
{
    UnitMods unitMod = UnitMods(UNIT_MOD_STAT_START + stat);

    if (m_auraModifiersGroup[unitMod][TOTAL_PCT] <= 0.0f)
        return 0.0f;

    // value = ((base_value * base_pct) + total_value) * total_pct
    float value  = m_auraModifiersGroup[unitMod][BASE_VALUE] + GetCreateStat(stat);
    value *= m_auraModifiersGroup[unitMod][BASE_PCT];
    value += m_auraModifiersGroup[unitMod][TOTAL_VALUE];
    value *= m_auraModifiersGroup[unitMod][TOTAL_PCT];

    return value;
}

float Unit::GetTotalAuraModValue(UnitMods unitMod) const
{
    if (unitMod >= UNIT_MOD_END)
    {
        sLog->outError("trial to access non existed UnitMods in GetTotalAuraModValue()!");
        return 0.0f;
    }

    if (m_auraModifiersGroup[unitMod][TOTAL_PCT] <= 0.0f)
        return 0.0f;

    float value = m_auraModifiersGroup[unitMod][BASE_VALUE];
    value *= m_auraModifiersGroup[unitMod][BASE_PCT];
    value += m_auraModifiersGroup[unitMod][TOTAL_VALUE];
    value *= m_auraModifiersGroup[unitMod][TOTAL_PCT];

    return value;
}

SpellSchools Unit::GetSpellSchoolByAuraGroup(UnitMods unitMod) const
{
    SpellSchools school = SPELL_SCHOOL_NORMAL;

    switch(unitMod)
    {
        case UNIT_MOD_RESISTANCE_HOLY:     school = SPELL_SCHOOL_HOLY;          break;
        case UNIT_MOD_RESISTANCE_FIRE:     school = SPELL_SCHOOL_FIRE;          break;
        case UNIT_MOD_RESISTANCE_NATURE:   school = SPELL_SCHOOL_NATURE;        break;
        case UNIT_MOD_RESISTANCE_FROST:    school = SPELL_SCHOOL_FROST;         break;
        case UNIT_MOD_RESISTANCE_SHADOW:   school = SPELL_SCHOOL_SHADOW;        break;
        case UNIT_MOD_RESISTANCE_ARCANE:   school = SPELL_SCHOOL_ARCANE;        break;

        default:
            break;
    }

    return school;
}

Stats Unit::GetStatByAuraGroup(UnitMods unitMod) const
{
    Stats stat = STAT_STRENGTH;

    switch(unitMod)
    {
        case UNIT_MOD_STAT_STRENGTH:    stat = STAT_STRENGTH;      break;
        case UNIT_MOD_STAT_AGILITY:     stat = STAT_AGILITY;       break;
        case UNIT_MOD_STAT_STAMINA:     stat = STAT_STAMINA;       break;
        case UNIT_MOD_STAT_INTELLECT:   stat = STAT_INTELLECT;     break;
        case UNIT_MOD_STAT_SPIRIT:      stat = STAT_SPIRIT;        break;

        default:
            break;
    }

    return stat;
}

Powers Unit::GetPowerTypeByAuraGroup(UnitMods unitMod) const
{
    switch (unitMod)
    {
        case UNIT_MOD_RAGE:        return POWER_RAGE;
        case UNIT_MOD_FOCUS:       return POWER_FOCUS;
        case UNIT_MOD_ENERGY:      return POWER_ENERGY;
        case UNIT_MOD_HAPPINESS:   return POWER_HAPPINESS;
        case UNIT_MOD_RUNE:        return POWER_RUNE;
        case UNIT_MOD_RUNIC_POWER: return POWER_RUNIC_POWER;
        default:
        case UNIT_MOD_MANA:        return POWER_MANA;
    }
}

float Unit::GetTotalAttackPowerValue(WeaponAttackType attType) const
{
    if (attType == RANGED_ATTACK)
    {
        // wands do not benefit from attack power
        if (getClassMask() & CLASSMASK_WAND_USERS)     // TODO: check the equipped item instead of this
            return 0.0f;

        int32 ap = GetInt32Value(UNIT_FIELD_RANGED_ATTACK_POWER) + GetInt32Value(UNIT_FIELD_RANGED_ATTACK_POWER_MOD_POS) - GetInt32Value(UNIT_FIELD_RANGED_ATTACK_POWER_MOD_NEG);
        if (ap < 0)
            return 0.0f;
        return ap * (1.0f + GetFloatValue(UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER));
    }
    else
    {
        int32 ap = GetInt32Value(UNIT_FIELD_ATTACK_POWER) + GetInt32Value(UNIT_FIELD_ATTACK_POWER_MOD_POS) - GetInt32Value(UNIT_FIELD_ATTACK_POWER_MOD_NEG);
        if (ap < 0)
            return 0.0f;
        return ap * (1.0f + GetFloatValue(UNIT_FIELD_ATTACK_POWER_MULTIPLIER));
    }
}

float Unit::GetWeaponDamageRange(WeaponAttackType attType ,WeaponDamageRange type) const
{
    if (attType == OFF_ATTACK && !haveOffhandWeapon())
        return 0.0f;

    return m_weaponDamage[attType][type];
}

void Unit::SetLevel(uint8 lvl)
{
    SetUInt32Value(UNIT_FIELD_LEVEL, lvl);

    // group update
    if (GetTypeId() == TYPEID_PLAYER && this->ToPlayer()->GetGroup())
        this->ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_LEVEL);

    if (GetTypeId() == TYPEID_PLAYER) 
        sWorld->UpdateCharacterNameDataLevel(ToPlayer()->GetGUIDLow(), lvl);
}

void Unit::SetHealth(uint32 val)
{
    if (getDeathState() == JUST_DIED)
        val = 0;
    else if (GetTypeId() == TYPEID_PLAYER && getDeathState() == DEAD)
        val = 1;
    else
    {
        uint32 maxHealth = GetMaxHealth();
        if (maxHealth < val)
            val = maxHealth;
    }

    SetUInt32Value(UNIT_FIELD_HEALTH, val);

    // group update
    if (GetTypeId() == TYPEID_PLAYER)
    {
        if (this->ToPlayer()->GetGroup())
            this->ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_CUR_HP);
    }
    else if (this->ToCreature()->IsPet())
    {
        Pet *pet = ((Pet*)this);
        if (pet->isControlled())
        {
            Unit *owner = GetOwner();
            if (owner && (owner->GetTypeId() == TYPEID_PLAYER) && owner->ToPlayer()->GetGroup())
                owner->ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_CUR_HP);
        }
    }
}

void Unit::SetMaxHealth(uint32 val)
{
    if (!val)
        val = 1;

    uint32 health = GetHealth();
    SetUInt32Value(UNIT_FIELD_MAXHEALTH, val);

    // group update
    if (GetTypeId() == TYPEID_PLAYER)
    {
        if (this->ToPlayer()->GetGroup())
            this->ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_MAX_HP);
    }
    else if (this->ToCreature()->IsPet())
    {
        Pet *pet = ((Pet*)this);
        if (pet->isControlled())
        {
            Unit *owner = GetOwner();
            if (owner && (owner->GetTypeId() == TYPEID_PLAYER) && owner->ToPlayer()->GetGroup())
                owner->ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_MAX_HP);
        }
    }

    if (val < health)
        SetHealth(val);
}

uint32 Unit::GetPower(Powers power) const
{
    uint32 powerIndex = GetPowerIndex(power);
    if (powerIndex == MAX_POWERS)
        return 0;

    return GetUInt32Value(UNIT_FIELD_POWER1 + powerIndex);
}

uint32 Unit::GetMaxPower(Powers power) const
{
    uint32 powerIndex = GetPowerIndex(power);
    if (powerIndex == MAX_POWERS)
        return 0;

    return GetUInt32Value(UNIT_FIELD_MAXPOWER1 + powerIndex);
}

void Unit::SetPower(Powers power, int32 val)
{
    uint32 powerIndex = GetPowerIndex(power);
    if (powerIndex == MAX_POWERS)
        return;

    if (GetPower(power) == uint32(val))
        return;

    // Special case for eclipse power (druid system)
    if(power == POWER_ECLIPSE && getClass() == CLASS_DRUID)
    {
        // some checks
        if(val > 100)
            val = 100;
        if(val < -100)
            val = -100;

        // The visual part
        int32 actualPower = (int32)GetPower(POWER_ECLIPSE);
        int32 deltaPower = abs(val-actualPower);
        int32 realModify = 0;
        if(val > actualPower)
        {
            realModify = deltaPower;
            ToPlayer()->TurnEclipseDriver(false);
        }
        else
        {
            realModify = -deltaPower;
            ToPlayer()->TurnEclipseDriver(true);
        }

        SendEnergizeSpellLog(this,89265,realModify,POWER_ECLIPSE);
        // end of visual part

        // function part
        // little hack (convert value < 0 to uint32), because stats are all defined as unsigned
        SetUInt32Value(UNIT_FIELD_POWER1 + powerIndex, (uint32)val);
        if(val >= 100 && !HasAura(48517))
        {
            //run solar eclipse
            CastSpell(this, 48517, true);
            ToPlayer()->TurnEclipseDriver(true);

            if (HasAura(16880) || HasAura(61345) || HasAura(61346)) // If player has Nature's Grace talent
                RemoveAura(93432); // Remove Cooldown marker

            // Euphoria
            int32 bp0 = 0;
            if (HasAura(81062))
                bp0 = 16;
            else if (HasAura(81061))
                bp0 = 8;

            if (bp0)
                CastCustomSpell(this, 81070, &bp0, 0, 0, true);
        }
        else if(val <= -100 && !HasAura(48518))
        {
            //run lunar eclipse
            CastSpell(this, 48518, true);
            ToPlayer()->TurnEclipseDriver(false);

            if (HasAura(16880) || HasAura(61345) || HasAura(61346)) // If player has Nature's Grace talent
                RemoveAura(93432); // Remove Cooldown marker

            // Euphoria
            int32 bp0 = 0;
            if (HasAura(81062))
                bp0 = 16;
            else if (HasAura(81061))
                bp0 = 8;

            if (bp0)
                CastCustomSpell(this, 81070, &bp0, 0, 0, true);
        }
        else if(val >= 0 && !ToPlayer()->IsEclipseDriverLeft() && HasAura(48518))
        {
            //cancel lunar eclipse
            RemoveAurasDueToSpell(48518);
        }
        else if(val <= 0 && ToPlayer()->IsEclipseDriverLeft() && HasAura(48517))
        {
            //cancel solar eclipse
            RemoveAurasDueToSpell(48517);
        }

        return;
    }

    // for now, we need positive value of power (only eclipse is exception)
    if(val < 0)
        val = abs(val);

    // Client limits
    switch(power)
    {
        case POWER_SOUL_SHARDS:
        case POWER_HOLY_POWER:
            if(val > 3)
                val = 3;
            break;
        default:
            break;
    }

    uint32 maxPower = GetMaxPower(power);
    if (maxPower < (uint32)val)
        val = maxPower;

    SetStatInt32Value(UNIT_FIELD_POWER1 + powerIndex, val);

    if (IsInWorld())
    {
        WorldPacket data(SMSG_POWER_UPDATE);
        data.append(GetPackGUID());
        data << uint32(1);  // count of updates. uint8 and uint32 for each
        data << uint8(powerIndex);
        data << int32(val);
        SendMessageToSet(&data, GetTypeId() == TYPEID_PLAYER ? true : false);
    }

    // group update
    if (GetTypeId() == TYPEID_PLAYER)
    {
        if (this->ToPlayer()->GetGroup())
            this->ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_CUR_POWER);
    }
    else if (this->ToCreature()->IsPet())
    {
        Pet *pet = ((Pet*)this);
        if (pet->isControlled())
        {
            Unit *owner = GetOwner();
            if (owner && (owner->GetTypeId() == TYPEID_PLAYER) && owner->ToPlayer()->GetGroup())
                owner->ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_CUR_POWER);
        }

        // Update the pet's character sheet with happiness damage bonus
        if (pet->getPetType() == HUNTER_PET)
            pet->UpdateDamagePhysical(BASE_ATTACK);
    }
}

uint32 Unit::GetPowerIndex(Powers power) const
{
    uint8 classId = getClass();
    if (IsPet() && ToPet()->getPetType() == HUNTER_PET)
        classId = CLASS_HUNTER;

    if (GetTypeId() != TYPEID_PLAYER)
        if (power == getPowerType())
            return 0;

    uint32 powerIndex = sObjectMgr->GetPowerIndexByClass(power, classId);
    if (powerIndex != MAX_POWERS)
        return powerIndex;

    return MAX_POWERS;
}

void Unit::SetMaxPower(Powers power, uint32 val)
{
    uint32 powerIndex = GetPowerIndex(power);
    if (powerIndex == MAX_POWERS)
        return;

    uint32 cur_power = GetPower(power);
    SetStatInt32Value(UNIT_FIELD_MAXPOWER1 + powerIndex, val);

    // group update
    if (GetTypeId() == TYPEID_PLAYER)
    {
        if (this->ToPlayer()->GetGroup())
            this->ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_MAX_POWER);
    }
    else if (this->ToCreature()->IsPet())
    {
        Pet *pet = ((Pet*)this);
        if (pet->isControlled())
        {
            Unit *owner = GetOwner();
            if (owner && (owner->GetTypeId() == TYPEID_PLAYER) && owner->ToPlayer()->GetGroup())
                owner->ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_MAX_POWER);
        }
    }

    if (val < cur_power)
        SetPower(power, val);
}

uint32 Unit::GetCreatePowers(Powers power) const
{
    // POWER_FOCUS and POWER_HAPPINESS only have hunter pet
    switch (power)
    {
        case POWER_MANA:
            if (getClass() == CLASS_HUNTER || getClass() == CLASS_WARRIOR || getClass() == CLASS_ROGUE || getClass() == CLASS_DEATH_KNIGHT)
                return false;
            else
                return GetCreateMana();

        case POWER_RAGE:        return 1000;
        case POWER_FOCUS:
            if(GetTypeId() == TYPEID_PLAYER && (ToPlayer()->getClass() == CLASS_HUNTER))
                return 100;
            return (GetTypeId() == TYPEID_PLAYER || !((Creature const*)this)->IsPet() || ((Pet const*)this)->getPetType() != HUNTER_PET ? 0 : 100);
        case POWER_ENERGY:      return 100;
        case POWER_HAPPINESS:   return (GetTypeId() == TYPEID_PLAYER || !((Creature const*)this)->IsPet() || ((Pet const*)this)->getPetType() != HUNTER_PET ? 0 : 1050000);
        case POWER_RUNE:        return (GetTypeId() == TYPEID_PLAYER && ((Player const*)this)->getClass() == CLASS_DEATH_KNIGHT ? 8 : 0);
        case POWER_RUNIC_POWER: return (GetTypeId() == TYPEID_PLAYER && ((Player const*)this)->getClass() == CLASS_DEATH_KNIGHT ? 1000 : 0);
        case POWER_SOUL_SHARDS: return (GetTypeId() == TYPEID_PLAYER && ((Player const*)this)->getClass() == CLASS_WARLOCK ? 3 : 0);
        case POWER_ECLIPSE:     return (GetTypeId() == TYPEID_PLAYER && ((Player const*)this)->getClass() == CLASS_DRUID ? 100 : 0);
        case POWER_HOLY_POWER:  return (GetTypeId() == TYPEID_PLAYER && ((Player const*)this)->getClass() == CLASS_PALADIN ? 3 : 0);
        case POWER_HEALTH:      return 0;
        default:
            break;
    }

    return 0;
}

void Unit::AddToWorld()
{
    if (!IsInWorld())
    {
        WorldObject::AddToWorld();
    }
}

void Unit::RemoveFromWorld()
{
    // cleanup
    ASSERT(GetGUID());

    if (IsInWorld())
    {
        m_duringRemoveFromWorld = true;
        if (IsVehicle())
            GetVehicleKit()->Uninstall();

        RemoveCharmAuras();
        RemoveBindSightAuras();
        RemoveNotOwnSingleTargetAuras();

        RemoveAllGameObjects();
        RemoveAllDynObjects();

        ExitVehicle();
        UnsummonAllTotems();
        RemoveAllControlled();

        RemoveAreaAurasDueToLeaveWorld();

        if (GetCharmerGUID())
        {
            sLog->outCrash("Unit %u has charmer guid when removed from world", GetEntry());
            ASSERT(false);
        }

        if (Unit *owner = GetOwner())
        {
            if (owner->m_Controlled.find(this) != owner->m_Controlled.end())
            {
                sLog->outCrash("Unit %u is in controlled list of %u when removed from world", GetEntry(), owner->GetEntry());
                ASSERT(false);
            }
        }

        if (m_movedPlayer && m_movedPlayer != this && m_movedPlayer->m_mover == this)
        {
            sLog->outError("Player %s has mover %u who is being removed from world", m_movedPlayer->GetName(), GetEntry());
            m_movedPlayer->SetMover(m_movedPlayer);
        }

        WorldObject::RemoveFromWorld();
        m_duringRemoveFromWorld = false;
    }
}

void Unit::CleanupsBeforeDelete(bool finalCleanup)
{
    // This needs to be before RemoveFromWorld to make GetCaster() return a valid pointer on aura removal
    InterruptNonMeleeSpells(true);

    if (IsInWorld())
        RemoveFromWorld();

    ASSERT(GetGUID());

    //A unit may be in removelist and not in world, but it is still in grid
    //and may have some references during delete
    RemoveAllAuras();
    RemoveAllGameObjects();

    if (finalCleanup)
        m_cleanupDone = true;

    m_Events.KillAllEvents(false);                      // non-delatable (currently casted spells) will not deleted now but it will deleted at call in Map::RemoveAllObjectsInRemoveList
    CombatStop();
    ClearComboPointHolders();
    DeleteThreatList();
    getHostileRefManager().setOnlineOfflineState(false);
    GetMotionMaster()->Clear(false);                    // remove different non-standard movement generators.

    if (Creature* thisCreature = ToCreature())
        if (GetTransport())
            GetTransport()->RemovePassenger(thisCreature);
}

void Unit::UpdateCharmAI()
{
    if (GetTypeId() == TYPEID_PLAYER)
        return;

    if (i_disabledAI) // disabled AI must be primary AI
    {
        if (!IsCharmed())
        {
            delete i_AI;
            i_AI = i_disabledAI;
            i_disabledAI = NULL;
        }
    }
    else
    {
        if (IsCharmed())
        {
            i_disabledAI = i_AI;
            if (IsPossessed() || IsVehicle())
                i_AI = new PossessedAI(this->ToCreature());
            else
                i_AI = new PetAI(this->ToCreature());
        }
    }
}

CharmInfo* Unit::InitCharmInfo()
{
    if (!m_charmInfo)
        m_charmInfo = new CharmInfo(this);

    return m_charmInfo;
}

void Unit::DeleteCharmInfo()
{
    if (!m_charmInfo)
        return;

    m_charmInfo->RestoreState();
    delete m_charmInfo;
    m_charmInfo = NULL;
}

CharmInfo::CharmInfo(Unit* unit)
: m_unit(unit), m_CommandState(COMMAND_FOLLOW), m_petnumber(0), m_barInit(false),
  m_isCommandAttack(false), m_isAtStay(false), m_isFollowing(false), m_isReturning(false)
{
    for (uint8 i = 0; i < MAX_SPELL_CHARM; ++i)
        m_charmspells[i].SetActionAndType(0,ACT_DISABLED);

    if (m_unit->GetTypeId() == TYPEID_UNIT)
    {
        m_oldReactState = m_unit->ToCreature()->GetReactState();
        m_unit->ToCreature()->SetReactState(REACT_PASSIVE);
    }

}

CharmInfo::~CharmInfo()
{
}

void CharmInfo::RestoreState()
{
    if (m_unit->GetTypeId() == TYPEID_UNIT)
        if (Creature *pCreature = m_unit->ToCreature())
            pCreature->SetReactState(m_oldReactState);
}

void CharmInfo::InitPetActionBar()
{
    // the first 3 SpellOrActions are attack, follow and stay
    for (uint32 i = 0; i < ACTION_BAR_INDEX_PET_SPELL_START - ACTION_BAR_INDEX_START; ++i)
        SetActionBar(ACTION_BAR_INDEX_START + i,COMMAND_ATTACK - i,ACT_COMMAND);

    // middle 4 SpellOrActions are spells/special attacks/abilities
    for (uint32 i = 0; i < ACTION_BAR_INDEX_PET_SPELL_END-ACTION_BAR_INDEX_PET_SPELL_START; ++i)
        SetActionBar(ACTION_BAR_INDEX_PET_SPELL_START + i,0,ACT_PASSIVE);

    // last 3 SpellOrActions are reactions
    SetActionBar(ACTION_BAR_INDEX_PET_SPELL_END + 0, REACT_PASSIVE, ACT_REACTION);
    SetActionBar(ACTION_BAR_INDEX_PET_SPELL_END + 1, REACT_DEFENSIVE, ACT_REACTION);
    SetActionBar(ACTION_BAR_INDEX_PET_SPELL_END + 2, REACT_ASSIST, ACT_REACTION);
}

void CharmInfo::InitEmptyActionBar(bool withAttack)
{
    if (withAttack)
        SetActionBar(ACTION_BAR_INDEX_START,COMMAND_ATTACK,ACT_COMMAND);
    else
        SetActionBar(ACTION_BAR_INDEX_START,0,ACT_PASSIVE);
    for (uint32 x = ACTION_BAR_INDEX_START+1; x < ACTION_BAR_INDEX_END; ++x)
        SetActionBar(x,0,ACT_PASSIVE);
}

void CharmInfo::InitPossessCreateSpells()
{
    InitEmptyActionBar();
    if (m_unit->GetTypeId() == TYPEID_UNIT)
    {
        for (uint32 i = 0; i < CREATURE_MAX_SPELLS; ++i)
        {
            uint32 spellId = m_unit->ToCreature()->m_spells[i];
            SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);
            if (spellInfo && spellInfo->Attributes & SPELL_ATTR0_CASTABLE_WHILE_DEAD)
                spellId = 0;
            if (IsPassiveSpell(spellId))
                m_unit->CastSpell(m_unit, spellId, true);
            else
                AddSpellToActionBar(m_unit->ToCreature()->m_spells[i], ACT_PASSIVE);
        }
    }
}

void CharmInfo::InitCharmCreateSpells()
{
    if (m_unit->GetTypeId() == TYPEID_PLAYER)                //charmed players don't have spells
    {
        InitEmptyActionBar();
        return;
    }

    InitPetActionBar();

    for (uint32 x = 0; x < MAX_SPELL_CHARM; ++x)
    {
        uint32 spellId = m_unit->ToCreature()->m_spells[x];
        SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);
        if (spellInfo && spellInfo->Attributes & SPELL_ATTR0_CASTABLE_WHILE_DEAD)
            spellId = 0;

        if (!spellId)
        {
            m_charmspells[x].SetActionAndType(spellId,ACT_DISABLED);
            continue;
        }

        if (IsPassiveSpell(spellId))
        {
            m_unit->CastSpell(m_unit, spellId, true);
            m_charmspells[x].SetActionAndType(spellId,ACT_PASSIVE);
        }
        else
        {
            m_charmspells[x].SetActionAndType(spellId,ACT_DISABLED);

            ActiveStates newstate = ACT_PASSIVE;
            if (spellInfo)
            {
                if (!IsAutocastableSpell(spellId))
                    newstate = ACT_PASSIVE;
                else
                {
                    bool autocast = false;
                    for (uint32 i = 0; i < MAX_SPELL_EFFECTS && !autocast; ++i)
                        if (SpellTargetType[spellInfo->EffectImplicitTargetA[i]] == TARGET_TYPE_UNIT_TARGET)
                            autocast = true;

                    if (autocast)
                    {
                        newstate = ACT_ENABLED;
                        ToggleCreatureAutocast(spellId, true);
                    }
                    else
                        newstate = ACT_DISABLED;
                }
            }

            AddSpellToActionBar(spellId, newstate);
        }
    }
}

bool CharmInfo::AddSpellToActionBar(uint32 spell_id, ActiveStates newstate)
{
    uint32 first_id = sSpellMgr->GetFirstSpellInChain(spell_id);

    // new spell rank can be already listed
    for (uint8 i = 0; i < MAX_UNIT_ACTION_BAR_INDEX; ++i)
    {
        if (uint32 action = PetActionBar[i].GetAction())
        {
            if (PetActionBar[i].IsActionBarForSpell() && sSpellMgr->GetFirstSpellInChain(action) == first_id)
            {
                PetActionBar[i].SetAction(spell_id);
                return true;
            }
        }
    }

    // or use empty slot in other case
    for (uint8 i = 0; i < MAX_UNIT_ACTION_BAR_INDEX; ++i)
    {
        if (!PetActionBar[i].GetAction() && PetActionBar[i].IsActionBarForSpell())
        {
            SetActionBar(i,spell_id,newstate == ACT_DECIDE ? IsAutocastableSpell(spell_id) ? ACT_DISABLED : ACT_PASSIVE : newstate);
            return true;
        }
    }
    return false;
}

bool CharmInfo::RemoveSpellFromActionBar(uint32 spell_id)
{
    uint32 first_id = sSpellMgr->GetFirstSpellInChain(spell_id);

    for (uint8 i = 0; i < MAX_UNIT_ACTION_BAR_INDEX; ++i)
    {
        if (uint32 action = PetActionBar[i].GetAction())
        {
            if (PetActionBar[i].IsActionBarForSpell() && sSpellMgr->GetFirstSpellInChain(action) == first_id)
            {
                SetActionBar(i,0,ACT_PASSIVE);
                return true;
            }
        }
    }

    return false;
}

void CharmInfo::ToggleCreatureAutocast(uint32 spellid, bool apply)
{
    if (IsPassiveSpell(spellid))
        return;

    for (uint32 x = 0; x < MAX_SPELL_CHARM; ++x)
        if (spellid == m_charmspells[x].GetAction())
            m_charmspells[x].SetType(apply ? ACT_ENABLED : ACT_DISABLED);
}

void CharmInfo::SetPetNumber(uint32 petnumber, bool statwindow)
{
    m_petnumber = petnumber;
    if (statwindow)
        m_unit->SetUInt32Value(UNIT_FIELD_PETNUMBER, m_petnumber);
    else
        m_unit->SetUInt32Value(UNIT_FIELD_PETNUMBER, 0);
}

void CharmInfo::LoadPetActionBar(const std::string& data)
{
    InitPetActionBar();

    Tokens tokens(data, ' ');

    if (tokens.size() != (ACTION_BAR_INDEX_END-ACTION_BAR_INDEX_START)*2)
        return;                                             // non critical, will reset to default

    uint8 index;
    Tokens::iterator iter;
    for (iter = tokens.begin(), index = ACTION_BAR_INDEX_START; index < ACTION_BAR_INDEX_END; ++iter, ++index)
    {
        // use unsigned cast to avoid sign negative format use at long-> ActiveStates (int) conversion
        ActiveStates type  = ActiveStates(atol(*iter));
        ++iter;
        uint32 action = uint32(atol(*iter));

        // "fix" the react states - this is necessary due to client automation
        if (type == ACT_REACTION && action == REACT_AGGRESSIVE)
            action = REACT_ASSIST;

        PetActionBar[index].SetActionAndType(action, type);

        // check correctness
        if (PetActionBar[index].IsActionBarForSpell())
        {
            if (!sSpellStore.LookupEntry(PetActionBar[index].GetAction()))
                SetActionBar(index, 0, ACT_PASSIVE);
            else if (!IsAutocastableSpell(PetActionBar[index].GetAction()))
                SetActionBar(index, PetActionBar[index].GetAction(), ACT_PASSIVE);
        }
    }
}

void CharmInfo::BuildActionBar(WorldPacket* data)
{
    for (uint32 i = 0; i < MAX_UNIT_ACTION_BAR_INDEX; ++i)
        *data << uint32(PetActionBar[i].packedData);
}

void CharmInfo::SetSpellAutocast(uint32 spell_id, bool state)
{
    for (uint8 i = 0; i < MAX_UNIT_ACTION_BAR_INDEX; ++i)
    {
        if (spell_id == PetActionBar[i].GetAction() && PetActionBar[i].IsActionBarForSpell())
        {
            PetActionBar[i].SetType(state ? ACT_ENABLED : ACT_DISABLED);
            break;
        }
    }
}

bool Unit::isFrozen() const
{
    return HasAuraState(AURA_STATE_FROZEN);
}

struct ProcTriggeredData
{
    ProcTriggeredData(Aura* _aura)
        : aura(_aura)
    {
        effMask = 0;
        spellProcEvent = NULL;
        byHack = false;
    }
    SpellProcEventEntry const *spellProcEvent;
    Aura * aura;
    uint32 effMask;
    bool byHack;
};

typedef std::list< ProcTriggeredData > ProcTriggeredList;

// List of auras that CAN be trigger but may not exist in spell_proc_event
// in most case need for drop charges
// in some types of aura need do additional check
// for example SPELL_AURA_MECHANIC_IMMUNITY - need check for mechanic
bool InitTriggerAuraData()
{
    for (uint16 i = 0; i < TOTAL_AURAS; ++i)
    {
        isTriggerAura[i] = false;
        isNonTriggerAura[i] = false;
        isAlwaysTriggeredAura[i] = false;
    }
    isTriggerAura[SPELL_AURA_DUMMY] = true;
    isTriggerAura[SPELL_AURA_MOD_CONFUSE] = true;
    isTriggerAura[SPELL_AURA_MOD_THREAT] = true;
    isTriggerAura[SPELL_AURA_MOD_STUN] = true; // Aura not have charges but need remove him on trigger
    isTriggerAura[SPELL_AURA_MOD_DAMAGE_DONE] = true;
    isTriggerAura[SPELL_AURA_MOD_DAMAGE_TAKEN] = true;
    isTriggerAura[SPELL_AURA_MOD_RESISTANCE] = true;
    isTriggerAura[SPELL_AURA_MOD_STEALTH] = true;
    isTriggerAura[SPELL_AURA_MOD_FEAR] = true; // Aura not have charges but need remove him on trigger
    isTriggerAura[SPELL_AURA_MOD_ROOT] = true;
    isTriggerAura[SPELL_AURA_TRANSFORM] = true;
    isTriggerAura[SPELL_AURA_REFLECT_SPELLS] = true;
    isTriggerAura[SPELL_AURA_DAMAGE_IMMUNITY] = true;
    isTriggerAura[SPELL_AURA_PROC_TRIGGER_SPELL] = true;
    isTriggerAura[SPELL_AURA_PROC_TRIGGER_SPELL_COPY] = true;
    isTriggerAura[SPELL_AURA_PROC_TRIGGER_DAMAGE] = true;
    isTriggerAura[SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK] = true;
    isTriggerAura[SPELL_AURA_SCHOOL_ABSORB] = true; // Savage Defense untested
    isTriggerAura[SPELL_AURA_MOD_POWER_COST_SCHOOL_PCT] = true;
    isTriggerAura[SPELL_AURA_MOD_POWER_COST_SCHOOL] = true;
    isTriggerAura[SPELL_AURA_REFLECT_SPELLS_SCHOOL] = true;
    isTriggerAura[SPELL_AURA_MECHANIC_IMMUNITY] = true;
    isTriggerAura[SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN] = true;
    isTriggerAura[SPELL_AURA_SPELL_MAGNET] = true;
    isTriggerAura[SPELL_AURA_MOD_ATTACK_POWER] = true;
    isTriggerAura[SPELL_AURA_ADD_CASTER_HIT_TRIGGER] = true;
    isTriggerAura[SPELL_AURA_OVERRIDE_CLASS_SCRIPTS] = true;
    isTriggerAura[SPELL_AURA_MOD_MECHANIC_RESISTANCE] = true;
    isTriggerAura[SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS] = true;
    isTriggerAura[SPELL_AURA_MOD_MELEE_HASTE] = true;
    isTriggerAura[SPELL_AURA_MOD_ATTACKER_MELEE_HIT_CHANCE]=true;
    isTriggerAura[SPELL_AURA_RAID_PROC_FROM_CHARGE] = true;
    isTriggerAura[SPELL_AURA_RAID_PROC_FROM_CHARGE_WITH_VALUE] = true;
    isTriggerAura[SPELL_AURA_PROC_TRIGGER_SPELL_WITH_VALUE] = true;
    isTriggerAura[SPELL_AURA_MOD_DAMAGE_FROM_CASTER] = true;
    isTriggerAura[SPELL_AURA_MOD_SPELL_CRIT_CHANCE] = true;
    isTriggerAura[SPELL_AURA_ABILITY_IGNORE_AURASTATE] = true;

    isNonTriggerAura[SPELL_AURA_MOD_POWER_REGEN] = true;
    isNonTriggerAura[SPELL_AURA_REDUCE_PUSHBACK] = true;

    isAlwaysTriggeredAura[SPELL_AURA_OVERRIDE_CLASS_SCRIPTS] = true;
    isAlwaysTriggeredAura[SPELL_AURA_MOD_FEAR] = true;
    isAlwaysTriggeredAura[SPELL_AURA_MOD_ROOT] = true;
    isAlwaysTriggeredAura[SPELL_AURA_MOD_STUN] = true;
    isAlwaysTriggeredAura[SPELL_AURA_TRANSFORM] = true;
    isAlwaysTriggeredAura[SPELL_AURA_SPELL_MAGNET] = true;
    isAlwaysTriggeredAura[SPELL_AURA_SCHOOL_ABSORB] = true;
    isAlwaysTriggeredAura[SPELL_AURA_MOD_STEALTH] = true;

    return true;
}

uint32 createProcExtendMask(SpellNonMeleeDamage *damageInfo, SpellMissInfo missCondition)
{
    uint32 procEx = PROC_EX_NONE;
    // Check victim state
    if (missCondition != SPELL_MISS_NONE)
        switch (missCondition)
        {
            case SPELL_MISS_MISS:    procEx|=PROC_EX_MISS;   break;
            case SPELL_MISS_RESIST:  procEx|=PROC_EX_RESIST; break;
            case SPELL_MISS_DODGE:   procEx|=PROC_EX_DODGE;  break;
            case SPELL_MISS_PARRY:   procEx|=PROC_EX_PARRY;  break;
            case SPELL_MISS_BLOCK:   procEx|=PROC_EX_BLOCK;  break;
            case SPELL_MISS_EVADE:   procEx|=PROC_EX_EVADE;  break;
            case SPELL_MISS_IMMUNE:  procEx|=PROC_EX_IMMUNE; break;
            case SPELL_MISS_IMMUNE2: procEx|=PROC_EX_IMMUNE; break;
            case SPELL_MISS_DEFLECT: procEx|=PROC_EX_DEFLECT;break;
            case SPELL_MISS_ABSORB:  procEx|=PROC_EX_ABSORB; break;
            case SPELL_MISS_REFLECT: procEx|=PROC_EX_REFLECT;break;
            default:
                break;
        }
    else
    {
        // On block
        if (damageInfo->blocked)
            procEx|=PROC_EX_BLOCK;
        // On absorb
        if (damageInfo->absorb)
            procEx|=PROC_EX_ABSORB;
        // On crit
        if (damageInfo->HitInfo & SPELL_HIT_TYPE_CRIT)
            procEx|=PROC_EX_CRITICAL_HIT;
        else
            procEx|=PROC_EX_NORMAL_HIT;
    }
    return procEx;
}

void Unit::ProcDamageAndSpellFor(bool isVictim, Unit * pTarget, uint32 procFlag, uint32 procExtra, WeaponAttackType attType, SpellEntry const * procSpell, uint32 damage, SpellEntry const * procAura)
{
    // Player is loading now - do not allow passive spell casts to proc
    if (GetTypeId() == TYPEID_PLAYER && this->ToPlayer()->GetSession()->PlayerLoading())
        return;
    // For melee/ranged based attack need update skills and set some Aura states if victim present
    if (procFlag & MELEE_BASED_TRIGGER_MASK && pTarget)
    {
        // Update skills here for players
        if (GetTypeId() == TYPEID_PLAYER)
        {
            // On melee based hit/miss/resist need update skill (for victim and attacker)
            if (procExtra&(PROC_EX_NORMAL_HIT|PROC_EX_MISS|PROC_EX_RESIST))
            {
                if (pTarget->GetTypeId() != TYPEID_PLAYER && pTarget->GetCreatureType() != CREATURE_TYPE_CRITTER)
                    this->ToPlayer()->UpdateCombatSkills(pTarget, attType, isVictim);
            }
            // Update defence if player is victim and parry/dodge/block
            else if (isVictim && procExtra&(PROC_EX_DODGE|PROC_EX_PARRY|PROC_EX_BLOCK))
                this->ToPlayer()->UpdateCombatSkills(pTarget, attType, true);
        }
        // If exist crit/parry/dodge/block need update aura state (for victim and attacker)
        if (procExtra & (PROC_EX_CRITICAL_HIT|PROC_EX_PARRY|PROC_EX_DODGE|PROC_EX_BLOCK))
        {
            // for victim
            if (isVictim)
            {
                // if victim and dodge attack
                if (procExtra&PROC_EX_DODGE)
                {
                    //Update AURA_STATE on dodge
                    if (getClass() != CLASS_ROGUE) // skip Rogue Riposte
                    {
                        ModifyAuraState(AURA_STATE_DEFENSE, true);
                        StartReactiveTimer(REACTIVE_DEFENSE);
                    }
                }
                // if victim and parry attack
                if (procExtra & PROC_EX_PARRY)
                {
                    // For Hunters only Counterattack (skip Mongoose bite)
                    if (getClass() == CLASS_HUNTER)
                    {
                        ModifyAuraState(AURA_STATE_HUNTER_PARRY, true);
                        StartReactiveTimer(REACTIVE_HUNTER_PARRY);
                    }
                    else
                    {
                        ModifyAuraState(AURA_STATE_DEFENSE, true);
                        StartReactiveTimer(REACTIVE_DEFENSE);
                    }

                    if (procExtra & PROC_EX_PARRY && GetTypeId() == TYPEID_PLAYER)
                    {
                        // Hold the Line "on parry" proc
                        if (HasAura(84604))
                            CastSpell(this, 84619, true);
                        else if (HasAura(84621))
                            CastSpell(this, 84620, true);
                    }
                }
                // if and victim block attack
                if (procExtra & PROC_EX_BLOCK)
                {
                    ModifyAuraState(AURA_STATE_DEFENSE,true);
                    StartReactiveTimer(REACTIVE_DEFENSE);
                }
            }
            else //For attacker
            {
                // Overpower on victim dodge
                if (procExtra&PROC_EX_DODGE && GetTypeId() == TYPEID_PLAYER && getClass() == CLASS_WARRIOR)
                {
                    this->ToPlayer()->AddComboPoints(pTarget, 1);
                    StartReactiveTimer(REACTIVE_OVERPOWER);
                }

                // Leader of the Pack
                if (procExtra & PROC_EX_CRITICAL_HIT && GetTypeId() == TYPEID_PLAYER)
                {
                    if (HasAura(17007)
                        && (GetShapeshiftForm() == FORM_CAT || GetShapeshiftForm() == FORM_BEAR || GetShapeshiftForm() == FORM_DIREBEAR)
                        && !ToPlayer()->HasSpellCooldown(34299))
                    {
                        int32 bp0 = GetMaxHealth()*0.04f;
                        CastCustomSpell(this, 34299, &bp0, 0, 0, true);
                        bp0 = GetMaxPower(POWER_MANA)*0.08f;
                        CastCustomSpell(this, 68285, &bp0, 0, 0, true);

                        ToPlayer()->AddSpellCooldown(34299, 0, 6000);
                    }
                }

                if (procExtra & PROC_EX_PARRY && GetTypeId() == TYPEID_PLAYER)
                {
                    // Hold the Line "on parry" proc
                    if (HasAura(84604))
                        CastSpell(this, 84619, true);
                    else if (HasAura(84621))
                        CastSpell(this, 84620, true);
                }
            }
        }
    }

    ProcTriggeredList procTriggered;
    // Fill procTriggered list
    for (AuraApplicationMap::const_iterator itr = GetAppliedAuras().begin(); itr!= GetAppliedAuras().end(); ++itr)
    {
        // Do not allow auras to proc from effect triggered by itself
        if (procAura && procAura->Id == itr->first)
            continue;
        ProcTriggeredData triggerData(itr->second->GetBase());
        // Defensive procs are active on absorbs (so absorption effects are not a hindrance)
        bool active = (damage > 0) || (procExtra & (PROC_EX_ABSORB|PROC_EX_BLOCK) && isVictim);
        if (isVictim)
            procExtra &= ~PROC_EX_INTERNAL_REQ_FAMILY;
        if (!itr->second || !itr->second->GetBase())
            continue;
        SpellEntry const* spellProto = itr->second->GetBase()->GetSpellProto();

        // Call hack to handle aura proc of non triggering auras if required
        if (IsHackTriggeredAura(pTarget, triggerData.aura, procSpell, procFlag, procExtra, attType, isVictim, active))
            triggerData.byHack = true;
        else if(!IsTriggeredAtSpellProcEvent(pTarget, triggerData.aura, procSpell, procFlag, procExtra, attType, isVictim, active, triggerData.spellProcEvent))
            continue;

        // Triggered spells not triggering additional spells
        bool triggered = !(spellProto->AttributesEx3 & SPELL_ATTR3_CAN_PROC_TRIGGERED) ?
            (procExtra & PROC_EX_INTERNAL_TRIGGERED && !(procFlag & PROC_FLAG_DONE_TRAP_ACTIVATION)) : false;

        // If hacked, immediatelly push front and do not continue in tick
        if (triggerData.byHack)
        {
            procTriggered.push_front(triggerData);
            continue;
        }

        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (itr->second->HasEffect(i))
            {
                AuraEffect * aurEff = itr->second->GetBase()->GetEffect(i);
                // Skip this auras
                if (isNonTriggerAura[aurEff->GetAuraType()])
                    continue;
                // If not trigger by default and spellProcEvent == NULL - skip
                if (!isTriggerAura[aurEff->GetAuraType()] && triggerData.spellProcEvent == NULL)
                    continue;
                // Some spells must always trigger
                if (!triggered || (isAlwaysTriggeredAura[aurEff->GetAuraType()] && procPrepared))
                    triggerData.effMask |= 1 << i;
            }
        }

        if (triggerData.effMask)
            procTriggered.push_front(triggerData);
    }

    // Nothing found
    if (procTriggered.empty())
        return;

    if (procExtra & (PROC_EX_INTERNAL_TRIGGERED | PROC_EX_INTERNAL_CANT_PROC))
        SetCantProc(true);

    // Handle effects proceed this time
    for (ProcTriggeredList::const_iterator i = procTriggered.begin(); i != procTriggered.end(); ++i)
    {
        // look for aura in auras list, it may be removed while proc event processing
        if (i->aura->IsRemoved())
            continue;

        // Handle our hacks
        if (i->byHack)
        {
            HandleAuraProcHack(pTarget, i->aura, procSpell, procFlag, procExtra, attType, isVictim, ((damage > 0) || (procExtra & (PROC_EX_ABSORB|PROC_EX_BLOCK) && isVictim)));
            continue;
        }

        bool useCharges= i->aura->GetCharges()>0;
        bool takeCharges = false;
        SpellEntry const *spellInfo = i->aura->GetSpellProto();
        uint32 Id = i->aura->GetId();

        // For players set spell cooldown if need
        uint32 cooldown = 0;
        if (GetTypeId() == TYPEID_PLAYER && i->spellProcEvent && i->spellProcEvent->cooldown)
            cooldown = i->spellProcEvent->cooldown;

        if (spellInfo->AttributesEx3 & SPELL_ATTR3_DISABLE_PROC)
            SetCantProc(true);

        // This bool is needed till separate aura effect procs are still here
        bool handled = false;
        if (HandleAuraProc(pTarget, damage, i->aura, procSpell, procFlag, procExtra, cooldown, &handled))
        {
            sLog->outDebug("ProcDamageAndSpell: casting spell %u (triggered with value by %s aura of spell %u)", spellInfo->Id,(isVictim?"a victim's":"an attacker's"), Id);
            takeCharges = true;
        }

        if (!handled)
        for (uint8 effIndex = 0; effIndex<MAX_SPELL_EFFECTS; ++effIndex)
        {
            if (!(i->effMask & (1<<effIndex)))
                continue;

            AuraEffect *triggeredByAura = i->aura->GetEffect(effIndex);
            ASSERT(triggeredByAura);

            switch(triggeredByAura->GetAuraType())
            {
                case SPELL_AURA_PROC_TRIGGER_SPELL:
                {
                    sLog->outDebug("ProcDamageAndSpell: casting spell %u (triggered by %s aura of spell %u)", spellInfo->Id,(isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId());
                    // Don`t drop charge or add cooldown for not started trigger
                    if (HandleProcTriggerSpell(pTarget, damage, triggeredByAura, procSpell, procFlag, procExtra, cooldown))
                        takeCharges = true;
                    break;
                }
                case SPELL_AURA_PROC_TRIGGER_SPELL_COPY:
                {
                    sLog->outDebug("ProcDamageAndSpell: casting spell copy %u (triggered by %s aura of spell %u)", spellInfo->Id, (isVictim ? "a victim's" : "an attacker's"), triggeredByAura->GetId());
                    HandleProcTriggerSpellCopy(pTarget, damage, triggeredByAura, procSpell, procFlag);
                    break;
                }
                case SPELL_AURA_PROC_TRIGGER_DAMAGE:
                {
                    sLog->outDebug("ProcDamageAndSpell: doing %u damage from spell id %u (triggered by %s aura of spell %u)", triggeredByAura->GetAmount() , spellInfo->Id, (isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId());
                    SpellNonMeleeDamage damageInfo(this, pTarget, spellInfo->Id, spellInfo->SchoolMask);
                    uint32 damage = SpellDamageBonus(pTarget, spellInfo, triggeredByAura->GetEffIndex(), triggeredByAura->GetAmount(), SPELL_DIRECT_DAMAGE);
                    CalculateSpellDamageTaken(&damageInfo, damage, spellInfo);
                    DealDamageMods(damageInfo.target,damageInfo.damage,&damageInfo.absorb);
                    SendSpellNonMeleeDamageLog(&damageInfo);
                    DealSpellDamage(&damageInfo, true);
                    takeCharges = true;
                    break;
                }
                case SPELL_AURA_MANA_SHIELD:
                case SPELL_AURA_DUMMY:
                {
                    sLog->outDebug("ProcDamageAndSpell: casting spell id %u (triggered by %s dummy aura of spell %u)", spellInfo->Id,(isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId());
                    if (HandleDummyAuraProc(pTarget, damage, triggeredByAura, procSpell, procFlag, procExtra, cooldown))
                        takeCharges = true;
                    break;
                }
                case SPELL_AURA_OBS_MOD_POWER:
                    sLog->outDebug("ProcDamageAndSpell: casting spell id %u (triggered by %s aura of spell %u)", spellInfo->Id,(isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId());
                    if (HandleObsModEnergyAuraProc(pTarget, damage, triggeredByAura, procSpell, procFlag, procExtra, cooldown))
                        takeCharges = true;
                    break;
                case SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN:
                    sLog->outDebug("ProcDamageAndSpell: casting spell id %u (triggered by %s aura of spell %u)", spellInfo->Id,(isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId());
                    if (HandleModDamagePctTakenAuraProc(pTarget, damage, triggeredByAura, procSpell, procFlag, procExtra, cooldown))
                        takeCharges = true;
                    break;
                case SPELL_AURA_MOD_POWER_REGEN_PERCENT:
                {
                    sLog->outDebug("ProcDamageAndSpell: casting spell id %u (triggered by %s ModPowerRegenPCT of spell %u)", spellInfo->Id,(isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId());
                    if (HandleModPowerRegenAuraProc(pTarget, damage, triggeredByAura, procSpell, procFlag, procExtra, cooldown))
                        takeCharges = true;
                    break;
                }
                case SPELL_AURA_OVERRIDE_CLASS_SCRIPTS:
                {
                    sLog->outDebug("ProcDamageAndSpell: casting spell id %u (triggered by %s aura of spell %u)", spellInfo->Id,(isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId());
                    if (HandleOverrideClassScriptAuraProc(pTarget, damage, triggeredByAura, procSpell, cooldown))
                        takeCharges = true;
                    break;
                }
                case SPELL_AURA_RAID_PROC_FROM_CHARGE_WITH_VALUE:
                {
                    sLog->outDebug("ProcDamageAndSpell: casting mending (triggered by %s dummy aura of spell %u)",
                        (isVictim?"a victim's":"an attacker's"),triggeredByAura->GetId());

                    HandleAuraRaidProcFromChargeWithValue(triggeredByAura);
                    takeCharges = true;
                    break;
                }
                case SPELL_AURA_RAID_PROC_FROM_CHARGE:
                {
                    sLog->outDebug("ProcDamageAndSpell: casting mending (triggered by %s dummy aura of spell %u)",
                        (isVictim?"a victim's":"an attacker's"),triggeredByAura->GetId());

                    HandleAuraRaidProcFromCharge(triggeredByAura);
                    takeCharges = true;
                    break;
                }
                case SPELL_AURA_PROC_TRIGGER_SPELL_WITH_VALUE:
                {
                    sLog->outDebug("ProcDamageAndSpell: casting spell %u (triggered with value by %s aura of spell %u)", spellInfo->Id,(isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId());

                    if (HandleProcTriggerSpell(pTarget, damage, triggeredByAura, procSpell, procFlag, procExtra, cooldown))
                        takeCharges = true;
                    break;
                }
                case SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK:
                    // Skip melee hits or instant cast spells
                    if (procSpell && GetSpellCastTime(procSpell) != 0)
                        takeCharges = true;
                    break;
                case SPELL_AURA_REFLECT_SPELLS_SCHOOL:
                    // Skip Melee hits and spells ws wrong school
                    if (procSpell && (triggeredByAura->GetMiscValue() & procSpell->SchoolMask))         // School check
                        takeCharges = true;
                    break;
                case SPELL_AURA_MOD_POWER_COST_SCHOOL_PCT:
                case SPELL_AURA_MOD_POWER_COST_SCHOOL:
                    // Skip melee hits and spells ws wrong school or zero cost
                    if (procSpell &&
                        (procSpell->manaCost != 0 || procSpell->ManaCostPercentage != 0) && // Cost check
                        (triggeredByAura->GetMiscValue() & procSpell->SchoolMask) == 0)         // School check
                        takeCharges = true;
                    break;
                case SPELL_AURA_MECHANIC_IMMUNITY:
                    // Compare mechanic
                    if (procSpell && procSpell->Mechanic == uint32(triggeredByAura->GetMiscValue()))
                        takeCharges = true;
                    break;
                case SPELL_AURA_MOD_MECHANIC_RESISTANCE:
                    // Compare mechanic
                    if (procSpell && procSpell->Mechanic == uint32(triggeredByAura->GetMiscValue()))
                        takeCharges = true;
                    break;
                case SPELL_AURA_MOD_DAMAGE_FROM_CASTER:
                    // Compare casters
                    if (triggeredByAura->GetCasterGUID() == pTarget->GetGUID())
                        takeCharges = true;
                    break;
                case SPELL_AURA_SPELL_MAGNET:
                    if (procSpell && procAura && procAura->Id == 8178)
                        if (IsAreaOfEffectSpell(procSpell)) // Don't allow AoE spells to consume grounding totem
                            return;
                    break;
                case SPELL_AURA_MOD_SPELL_CRIT_CHANCE:
                    sLog->outDebug("ProcDamageAndSpell: casting spell id %u (triggered by %s spell crit chance aura of spell %u)", spellInfo->Id,(isVictim?"a victim's":"an attacker's"), triggeredByAura->GetId());
                    if (procSpell && HandleSpellCritChanceAuraProc(pTarget, damage, triggeredByAura, procSpell, procFlag, procExtra, cooldown))
                        takeCharges = true;
                    break;
                // CC Auras which use their amount amount to drop
                // Are there any more auras which need this?
                case SPELL_AURA_MOD_CONFUSE:
                case SPELL_AURA_MOD_FEAR:
                case SPELL_AURA_MOD_STUN:
                case SPELL_AURA_MOD_ROOT:
                case SPELL_AURA_TRANSFORM:
                case SPELL_AURA_MOD_PACIFY_SILENCE:
                {
                    if (triggeredByAura->GetAuraType() == SPELL_AURA_MOD_PACIFY_SILENCE && triggeredByAura->GetSpellProto()->Id != 51514)
                    {
                        takeCharges = true;
                        break;
                    }

                    // chargeable mods are breaking on hit
                    if (useCharges)
                        takeCharges = true;
                    else
                    {
                        // Spell own direct damage at apply wont break the CC
                        if (procSpell && (procSpell->Id == triggeredByAura->GetId()))
                        {
                            Aura * aura = triggeredByAura->GetBase();
                            // called from spellcast, should not have ticked yet
                            if (aura->GetDuration() == aura->GetMaxDuration())
                                break;
                        }
                        int32 damageLeft = triggeredByAura->GetAmount();
                        // No damage left
                        if (damageLeft < int32(damage))
                            i->aura->Remove();
                        else
                            triggeredByAura->SetAmount(damageLeft - damage);
                    }
                    break;
                }
                //case SPELL_AURA_ADD_FLAT_MODIFIER:
                //case SPELL_AURA_ADD_PCT_MODIFIER:
                    // HandleSpellModAuraProc
                    //break;
                default:
                    // nothing do, just charges counter
                    takeCharges = true;
                    break;
            }
        }
        // Vanish - does not break stealth if has triggered aura
        if (i->aura->GetSpellProto() && i->aura->GetSpellProto()->Id == 11327)
            takeCharges = false;
        if (i->aura->GetSpellProto() && i->aura->GetSpellProto()->Id == 1784 && (HasAura(11327) || pTarget->HasAura(11327)))
            takeCharges = false;

        // Glyph of Lightning Shield - don't allow dropping under 3 charges of Lightning Shield
        if (i->aura->GetSpellProto() && i->aura->GetSpellProto()->Id == 324 && HasAura(55448) && i->aura->GetCharges() <= 3)
            takeCharges = false;

        // 4P PvP bonus for Elemental shaman's
        if (i->aura->GetSpellProto() && i->aura->GetSpellProto()->Id == 324 && HasAura(100956)) // // Improved Lightning Shield aura
        {
            uint8 maxCharges = 3;

            if (HasAura(88756)) // Rolling Thunder (Rank 1)
                maxCharges = 6;

            if (HasAura(88764)) // Rolling Thunder (Rank 2)
                maxCharges = 9;

            Aura * lsAura = this->GetAura(324,this->GetGUID());

            if(lsAura && ToPlayer() && !ToPlayer()->HasSpellCooldown(100956))
            {
                uint8 lsCharges = lsAura->GetCharges();

                if(lsCharges < maxCharges)
                {
                    lsAura->SetCharges(lsCharges + 1);
                    ToPlayer()->AddSpellCooldown(100956,0,3000);
                }
            }

            takeCharges = false;
        }

        // Sanguinary Vein - do not unapply Gouge if attempting from bleed effect
        if (pTarget && ((pTarget->HasAura(79146) && roll_chance_i(50)) || pTarget->HasAura(79147)) && i->aura->GetSpellProto() && i->aura->GetSpellProto()->Id == 1776
            && procSpell && ((procSpell->Mechanic == MECHANIC_BLEED && procSpell->AppliesAuraType(SPELL_AURA_PERIODIC_DAMAGE)) || procSpell->Id == 89775))
            takeCharges = false;

        // Repentance is not broken by Censure DoT ticks (only one explicit exception)
        if (procSpell && procSpell->Id == 31803 && i->aura->GetSpellProto()->Id == 20066)
            takeCharges = false;

        // Remove charge (aura can be removed by triggers)
        if (useCharges && takeCharges)
            i->aura->DropCharge();

        if (spellInfo->AttributesEx3 & SPELL_ATTR3_DISABLE_PROC)
            SetCantProc(false);
    }

    // Cleanup proc requirements
    if (procExtra & (PROC_EX_INTERNAL_TRIGGERED | PROC_EX_INTERNAL_CANT_PROC))
        SetCantProc(false);
}

SpellSchoolMask Unit::GetMeleeDamageSchoolMask() const
{
    return SPELL_SCHOOL_MASK_NORMAL;
}

Player* Unit::GetSpellModOwner() const
{
    if (GetTypeId() == TYPEID_PLAYER)
        return (Player*)this;

    /*if (GetTypeId() == TYPEID_DYNAMICOBJECT)
    {
        if (Unit * dynOwner = ToDynamicObject()->GetCaster())
        {
            if (dynOwner->GetTypeId() == TYPEID_PLAYER)
                return dynOwner->ToPlayer();
        }
    }*/

    if (this->ToCreature()->IsPet() || this->ToCreature()->IsTotem())
    {
        Unit* owner = GetOwner();
        if (owner && owner->GetTypeId() == TYPEID_PLAYER)
            return (Player*)owner;
    }
    return NULL;
}

///----------Pet responses methods-----------------
void Unit::SendPetCastFail(uint32 spellid, SpellCastResult msg)
{
    if (msg == SPELL_CAST_OK)
        return;

    Unit *owner = GetCharmerOrOwner();
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return;

    WorldPacket data(SMSG_PET_CAST_FAILED, 1 + 4 + 1);
    data << uint8(0);                                       // cast count?
    data << uint32(spellid);
    data << uint8(msg);
    // uint32 for some reason
    // uint32 for some reason
    owner->ToPlayer()->GetSession()->SendPacket(&data);
}

void Unit::SendPetActionFeedback (uint8 msg)
{
    Unit* owner = GetOwner();
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return;

    WorldPacket data(SMSG_PET_ACTION_FEEDBACK, 1);
    data << uint8(msg);
    owner->ToPlayer()->GetSession()->SendPacket(&data);
}

void Unit::SendPetTalk (uint32 pettalk)
{
    Unit* owner = GetOwner();
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return;

    WorldPacket data(SMSG_PET_ACTION_SOUND, 8 + 4);
    data << uint64(GetGUID());
    data << uint32(pettalk);
    owner->ToPlayer()->GetSession()->SendPacket(&data);
}

void Unit::SendPetAIReaction(uint64 guid)
{
    Unit* owner = GetOwner();
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
        return;

    WorldPacket data(SMSG_AI_REACTION, 8 + 4);
    data << uint64(guid);
    data << uint32(AI_REACTION_HOSTILE);
    owner->ToPlayer()->GetSession()->SendPacket(&data);
}

///----------End of Pet responses methods----------

void Unit::StopMoving()
{
    ClearUnitState(UNIT_STATE_MOVING);

    // not need send any packets if not in world or not moving
    if (!IsInWorld() || movespline->Finalized())
        return;

    // Update position using old spline
    UpdateSplinePosition();
    Movement::MoveSplineInit(this).Stop();
}

void Unit::SendMovementFlagUpdate()
{
    WorldPacket data;
    BuildHeartBeatMsg(&data);
    SendMessageToSet(&data, true);
}

bool Unit::IsSitState() const
{
    uint8 s = getStandState();
    return
        s == UNIT_STAND_STATE_SIT_CHAIR        || s == UNIT_STAND_STATE_SIT_LOW_CHAIR  ||
        s == UNIT_STAND_STATE_SIT_MEDIUM_CHAIR || s == UNIT_STAND_STATE_SIT_HIGH_CHAIR ||
        s == UNIT_STAND_STATE_SIT;
}

bool Unit::IsStandState() const
{
    uint8 s = getStandState();
    return !IsSitState() && s != UNIT_STAND_STATE_SLEEP && s != UNIT_STAND_STATE_KNEEL;
}

void Unit::SetStandState(uint8 state)
{
    SetByteValue(UNIT_FIELD_BYTES_1, 0, state);

    if (IsStandState())
       RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_NOT_SEATED);

    if (GetTypeId() == TYPEID_PLAYER)
    {
        WorldPacket data(SMSG_STANDSTATE_UPDATE, 1);
        data << (uint8)state;
        this->ToPlayer()->GetSession()->SendPacket(&data);
    }
}

bool Unit::IsPolymorphed() const
{
    uint32 transformId = getTransForm();
    if (!transformId)
        return false;

    const SpellEntry *spellInfo=sSpellStore.LookupEntry(transformId);
    if (!spellInfo)
        return false;

    return GetSpellSpecific(spellInfo) == SPELL_SPECIFIC_MAGE_POLYMORPH;
}

void Unit::SetDisplayId(uint32 modelId)
{
    SetUInt32Value(UNIT_FIELD_DISPLAYID, modelId);

    if (GetTypeId() == TYPEID_UNIT && this->ToCreature()->IsPet())
    {
        Pet *pet = ((Pet*)this);
        if (!pet->isControlled())
            return;
        Unit *owner = GetOwner();
        if (owner && (owner->GetTypeId() == TYPEID_PLAYER) && owner->ToPlayer()->GetGroup())
            owner->ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_MODEL_ID);
    }
}

void Unit::ClearComboPointHolders()
{
    while (!m_ComboPointHolders.empty())
    {
        uint32 lowguid = *m_ComboPointHolders.begin();

        Player* plr = sObjectMgr->GetPlayer(MAKE_NEW_GUID(lowguid, 0, HIGHGUID_PLAYER));
        if (plr && plr->GetComboTarget() == GetGUID())         // recheck for safe
            plr->ClearComboPoints();                        // remove also guid from m_ComboPointHolders;
        else
            m_ComboPointHolders.erase(lowguid);             // or remove manually
    }
}

void Unit::ClearAllReactives()
{
    for (uint8 i=0; i < MAX_REACTIVE; ++i)
        m_reactiveTimer[i] = 0;

    if (HasAuraState(AURA_STATE_DEFENSE))
        ModifyAuraState(AURA_STATE_DEFENSE, false);
    if (getClass() == CLASS_HUNTER && HasAuraState(AURA_STATE_HUNTER_PARRY))
        ModifyAuraState(AURA_STATE_HUNTER_PARRY, false);
    if (getClass() == CLASS_WARRIOR && GetTypeId() == TYPEID_PLAYER)
        this->ToPlayer()->ClearComboPoints();
}

void Unit::UpdateReactives(uint32 p_time)
{
    for (uint8 i = 0; i < MAX_REACTIVE; ++i)
    {
        ReactiveType reactive = ReactiveType(i);

        if (!m_reactiveTimer[reactive])
            continue;

        if (m_reactiveTimer[reactive] <= p_time)
        {
            m_reactiveTimer[reactive] = 0;

            switch (reactive)
            {
                case REACTIVE_DEFENSE:
                    if (HasAuraState(AURA_STATE_DEFENSE))
                        ModifyAuraState(AURA_STATE_DEFENSE, false);
                    break;
                case REACTIVE_HUNTER_PARRY:
                    if (getClass() == CLASS_HUNTER && HasAuraState(AURA_STATE_HUNTER_PARRY))
                        ModifyAuraState(AURA_STATE_HUNTER_PARRY, false);
                    break;
                case REACTIVE_OVERPOWER:
                    if (getClass() == CLASS_WARRIOR && GetTypeId() == TYPEID_PLAYER)
                        this->ToPlayer()->ClearComboPoints();
                    break;
                default:
                    break;
            }
        }
        else
        {
            m_reactiveTimer[reactive] -= p_time;
        }
    }
}

Unit* Unit::SelectNearbyTarget(float dist) const
{
    std::list<Unit *> targets;
    Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(this, this, dist);
    Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(this, targets, u_check);
    VisitNearbyObject(dist, searcher);

    // remove current target
    if (GetVictim())
        targets.remove(GetVictim());

    // remove not LoS targets
    for (std::list<Unit*>::iterator tIter = targets.begin(); tIter != targets.end();)
    {
        if (!IsWithinLOSInMap(*tIter) || (*tIter)->IsTotem() || (*tIter)->IsSpiritService() || (*tIter)->GetCreatureType() == CREATURE_TYPE_CRITTER)
            targets.erase(tIter++);
        else
            ++tIter;
    }

    // no appropriate targets
    if (targets.empty())
        return NULL;

    // select random
    std::list<Unit*>::const_iterator tcIter = targets.begin();
    std::advance(tcIter, urand(0, targets.size()-1));
    return *tcIter;
}

void Unit::ApplyAttackTimePercentMod(WeaponAttackType att,float val, bool apply)
{
    float remainingTimePct = (float)m_attackTimer[att] / (GetAttackTime(att) * m_modAttackSpeedPct[att]);
    if (val > 0)
    {
        ApplyPercentModFloatVar(m_modAttackSpeedPct[att], val, !apply);
        ApplyPercentModFloatValue(UNIT_FIELD_BASEATTACKTIME+att,val,!apply);
    }
    else
    {
        ApplyPercentModFloatVar(m_modAttackSpeedPct[att], -val, apply);
        ApplyPercentModFloatValue(UNIT_FIELD_BASEATTACKTIME+att,-val,apply);
    }
    m_attackTimer[att] = uint32(GetAttackTime(att) * m_modAttackSpeedPct[att] * remainingTimePct);

    if (Player *pl = ToPlayer())
        pl->UpdateHaste();
}

void Unit::ApplyCastTimePercentMod(float val, bool apply)
{
    if (val > 0)
    {
        ApplyPercentModFloatValue(UNIT_MOD_CAST_SPEED, val, !apply);
        ApplyPercentModFloatValue(UNIT_MOD_CAST_HASTE, val, !apply);
    }
    else
    {
        ApplyPercentModFloatValue(UNIT_MOD_CAST_SPEED, -val, apply);
        ApplyPercentModFloatValue(UNIT_MOD_CAST_HASTE, -val, apply);
    }

    if (Player *pl = ToPlayer())
        pl->UpdateHaste();
}

uint32 Unit::GetCastingTimeForBonus(SpellEntry const *spellProto, DamageEffectType damagetype, uint32 CastingTime)
{
    // Not apply this to creature casted spells with casttime == 0
    if (CastingTime == 0 && GetTypeId() == TYPEID_UNIT && !this->ToCreature()->IsPet())
        return 3500;

    if (CastingTime > 7000) CastingTime = 7000;
    if (CastingTime < 1500) CastingTime = 1500;

    if (damagetype == DOT && !IsChanneledSpell(spellProto))
        CastingTime = 3500;

    int32 overTime    = 0;
    uint8 effects     = 0;
    bool DirectDamage = false;
    bool AreaEffect   = false;

    for (uint32 i=0; i<MAX_SPELL_EFFECTS; i++)
    {
        switch (spellProto->Effect[i])
        {
            case SPELL_EFFECT_SCHOOL_DAMAGE:
            case SPELL_EFFECT_POWER_DRAIN:
            case SPELL_EFFECT_HEALTH_LEECH:
            case SPELL_EFFECT_ENVIRONMENTAL_DAMAGE:
            case SPELL_EFFECT_POWER_BURN:
            case SPELL_EFFECT_HEAL:
                DirectDamage = true;
                break;
            case SPELL_EFFECT_APPLY_AURA:
                switch (spellProto->EffectApplyAuraName[i])
                {
                    case SPELL_AURA_PERIODIC_DAMAGE:
                    case SPELL_AURA_PERIODIC_HEAL:
                    case SPELL_AURA_PERIODIC_LEECH:
                        if (GetSpellDuration(spellProto))
                            overTime = GetSpellDuration(spellProto);
                        break;
                    default:
                        // -5% per additional effect
                        ++effects;
                        break;
                }
            default:
                break;
        }

        if (IsAreaEffectTarget[spellProto->EffectImplicitTargetA[i]] || IsAreaEffectTarget[spellProto->EffectImplicitTargetB[i]])
            AreaEffect = true;
    }

    // Combined Spells with Both Over Time and Direct Damage
    if (overTime > 0 && CastingTime > 0 && DirectDamage)
    {
        // mainly for DoTs which are 3500 here otherwise
        uint32 OriginalCastTime = GetSpellCastTime(spellProto);
        if (OriginalCastTime > 7000) OriginalCastTime = 7000;
        if (OriginalCastTime < 1500) OriginalCastTime = 1500;
        // Portion to Over Time
        float PtOT = (overTime / 15000.0f) / ((overTime / 15000.0f) + (OriginalCastTime / 3500.0f));

        if (damagetype == DOT)
            CastingTime = uint32(CastingTime * PtOT);
        else if (PtOT < 1.0f)
            CastingTime  = uint32(CastingTime * (1 - PtOT));
        else
            CastingTime = 0;
    }

    // Area Effect Spells receive only half of bonus
    if (AreaEffect)
        CastingTime /= 2;

    // -5% of total per any additional effect
    for (uint8 i=0; i<effects; ++i)
    {
        if (CastingTime > 175)
        {
            CastingTime -= 175;
        }
        else
        {
            CastingTime = 0;
            break;
        }
    }

    return CastingTime;
}

void Unit::UpdateAuraForGroup(uint8 slot)
{
    if (slot >= MAX_AURAS)                        // slot not found, return
        return;
    if (GetTypeId() == TYPEID_PLAYER)
    {
        Player* player = (Player*)this;
        if (player->GetGroup())
        {
            player->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_AURAS);
            player->SetAuraUpdateMaskForRaid(slot);
        }
    }
    else if (GetTypeId() == TYPEID_UNIT && this->ToCreature()->IsPet())
    {
        Pet *pet = ((Pet*)this);
        if (pet->isControlled())
        {
            Unit *owner = GetOwner();
            if (owner && (owner->GetTypeId() == TYPEID_PLAYER) && owner->ToPlayer()->GetGroup())
            {
                owner->ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_AURAS);
                pet->SetAuraUpdateMaskForRaid(slot);
            }
        }
    }
}

float Unit::GetAPMultiplier(WeaponAttackType attType, bool normalized)
{
    if (!normalized || GetTypeId() != TYPEID_PLAYER)
        return float(GetAttackTime(attType))/1000.0f;

    Item *Weapon = this->ToPlayer()->GetWeaponForAttack(attType, true);
    if (!Weapon)
        return 2.4f;                                         // fist attack

    switch (Weapon->GetProto()->InventoryType)
    {
        case INVTYPE_2HWEAPON:
            return 3.3f;
        case INVTYPE_RANGED:
        case INVTYPE_RANGEDRIGHT:
        case INVTYPE_THROWN:
            return 2.8f;
        case INVTYPE_WEAPON:
        case INVTYPE_WEAPONMAINHAND:
        case INVTYPE_WEAPONOFFHAND:
        default:
            return Weapon->GetProto()->SubClass == ITEM_SUBCLASS_WEAPON_DAGGER ? 1.7f : 2.4f;
    }
}

void Unit::SetContestedPvP(Player *attackedPlayer)
{
    Player* player = GetCharmerOrOwnerPlayerOrPlayerItself();

    if (!player || (attackedPlayer && (attackedPlayer == player || (player->duel && player->duel->opponent == attackedPlayer))))
        return;

    player->SetContestedPvPTimer(30000);
    if (!player->HasUnitState(UNIT_STATE_ATTACK_PLAYER))
    {
        player->AddUnitState(UNIT_STATE_ATTACK_PLAYER);
        player->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_CONTESTED_PVP);
        // call MoveInLineOfSight for nearby contested guards
        UpdateObjectVisibility();
    }
    if (!HasUnitState(UNIT_STATE_ATTACK_PLAYER))
    {
        AddUnitState(UNIT_STATE_ATTACK_PLAYER);
        // call MoveInLineOfSight for nearby contested guards
        UpdateObjectVisibility();
    }
}

void Unit::AddPetAura(PetAura const* petSpell)
{
    if (GetTypeId() != TYPEID_PLAYER)
        return;

    m_petAuras.insert(petSpell);
    if (Pet* pet = this->ToPlayer()->GetPet())
        pet->CastPetAura(petSpell);
}

void Unit::RemovePetAura(PetAura const* petSpell)
{
    if (GetTypeId() != TYPEID_PLAYER)
        return;

    m_petAuras.erase(petSpell);
    if (Pet* pet = this->ToPlayer()->GetPet())
        pet->RemoveAurasDueToSpell(petSpell->GetAura(pet->GetEntry()));
}

Pet* Unit::CreateTamedPetFrom(Creature* creatureTarget, PetSlot slot, uint32 spell_id)
{
    if (GetTypeId() != TYPEID_PLAYER)
        return NULL;

    Pet* pet = new Pet((Player*)this, HUNTER_PET);
    pet->SetSlot(slot);

    if (!pet->CreateBaseAtCreature(creatureTarget))
    {
        delete pet;
        return NULL;
    }

    uint8 level = creatureTarget->getLevel() + 5 < getLevel() ? (getLevel() - 5) : creatureTarget->getLevel();

    InitTamedPet(pet, level, spell_id);

    return pet;
}

Pet* Unit::CreateTamedPetFrom(uint32 creatureEntry, PetSlot slot, uint32 spell_id)
{
    if (GetTypeId() != TYPEID_PLAYER)
        return NULL;

    CreatureInfo const* creatureInfo = sObjectMgr->GetCreatureTemplate(creatureEntry);
    if (!creatureInfo)
        return NULL;

    Pet* pet = new Pet((Player*)this, HUNTER_PET);
    pet->SetSlot(slot);

    if (!pet->CreateBaseAtCreatureInfo(creatureInfo, this) || !InitTamedPet(pet, getLevel(), spell_id))
    {
        delete pet;
        return NULL;
    }

    return pet;
}

bool Unit::InitTamedPet(Pet * pet, uint8 level, uint32 spell_id)
{
    pet->SetCreatorGUID(GetGUID());
    pet->setFaction(getFaction());
    pet->SetUInt32Value(UNIT_CREATED_BY_SPELL, spell_id);

    if (GetTypeId() == TYPEID_PLAYER)
        pet->SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);

    if (!pet->InitStatsForLevel(level))
    {
        sLog->outError("Pet::InitStatsForLevel() failed for creature (Entry: %u)!",pet->GetEntry());
        return false;
    }

    pet->GetCharmInfo()->SetPetNumber(sObjectMgr->GeneratePetNumber(), true);
    // this enables pet details window (Shift+P)
    pet->InitPetCreateSpells();
    //pet->InitLevelupSpellsForLevel();
    pet->SetFullHealth();
    return true;
}

bool Unit::CanReachByPath(Unit const* target) const
{
    if (!IsInWorld() || !target || !target->IsInWorld())
        return false;

    // to test reachability, lets generate path
    PathGenerator path(this);
    bool result = path.CalculatePath(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), false, false);

    // when not using pathfinding, the target is always reachable
    if (path.GetPathType() & PATHFIND_NOT_USING_PATH)
        return true;

    // the target is not reachable when error occured, when no path found, or when incomplete path has been found
    if (!result || (path.GetPathType() & (PATHFIND_NOPATH | PATHFIND_INCOMPLETE)))
        return false;

    // otherwise is reachable
    return true;
}

bool Unit::IsTriggeredAtSpellProcEvent(Unit *pVictim, Aura * aura, SpellEntry const* procSpell, uint32 procFlag, uint32 procExtra, WeaponAttackType attType, bool isVictim, bool active, SpellProcEventEntry const *& spellProcEvent)
{
    if (!aura || !aura->GetSpellProto())
        return false;

    SpellEntry const *spellProto = aura->GetSpellProto();

    // Get proc Event Entry
    spellProcEvent = sSpellMgr->GetSpellProcEvent(spellProto->Id);

    // Get EventProcFlag
    uint32 EventProcFlag;
    if (spellProcEvent && spellProcEvent->procFlags) // if exist get custom spellProcEvent->procFlags
        EventProcFlag = spellProcEvent->procFlags;
    else
        EventProcFlag = spellProto->procFlags;       // else get from spell proto
    // Continue if no trigger exist
    if (!EventProcFlag)
        return false;

    // Additional checks for triggered spells (ignore trap casts)
    if (procExtra & PROC_EX_INTERNAL_TRIGGERED && !(procFlag & PROC_FLAG_DONE_TRAP_ACTIVATION))
    {
        if (!(spellProto->AttributesEx3 & SPELL_ATTR3_CAN_PROC_TRIGGERED))
            return false;
    }

    // Check spellProcEvent data requirements
    if (!sSpellMgr->IsSpellProcEventCanTriggeredBy(spellProto, spellProcEvent, EventProcFlag, procSpell, procFlag, procExtra, active))
        return false;

    // In most cases req get honor or XP from kill
    if (EventProcFlag & PROC_FLAG_KILL && GetTypeId() == TYPEID_PLAYER)
    {
        bool allow = false;

        if (pVictim)
            allow = ToPlayer()->isHonorOrXPTarget(pVictim);

        // Shadow Word: Death - can trigger from every kill
        if (aura->GetId() == 32409)
            allow = true;
        if (!allow)
            return false;
    }
    // Aura added by spell can`t trigger from self (prevent drop charges/do triggers)
    // But except periodic and kill triggers (can triggered from self)
    if (procSpell && procSpell->Id == spellProto->Id
        && !(spellProto->procFlags&(PROC_FLAG_TAKEN_PERIODIC | PROC_FLAG_KILL)))
        return false;

    // Check if current equipment allows aura to proc
    if (!isVictim && GetTypeId() == TYPEID_PLAYER)
    {
        if (spellProto->EquippedItemClass == ITEM_CLASS_WEAPON)
        {
            Item *item = NULL;
            if (attType == BASE_ATTACK)
                item = this->ToPlayer()->GetUseableItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
            else if (attType == OFF_ATTACK)
                item = this->ToPlayer()->GetUseableItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
            else
                item = this->ToPlayer()->GetUseableItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED);

            if (this->ToPlayer()->IsInFeralForm())
                return false;

            if (!item || item->IsBroken() || item->GetProto()->Class != ITEM_CLASS_WEAPON || !((1<<item->GetProto()->SubClass) & spellProto->EquippedItemSubClassMask))
                return false;
        }
        else if (spellProto->EquippedItemClass == ITEM_CLASS_ARMOR)
        {
            // Check if player is wearing shield
            Item *item = this->ToPlayer()->GetUseableItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
            if (!item || item->IsBroken() || item->GetProto()->Class != ITEM_CLASS_ARMOR || !((1<<item->GetProto()->SubClass) & spellProto->EquippedItemSubClassMask))
                return false;
        }
    }
    // Get chance from spell
    float chance = float(spellProto->procChance);
    // If in spellProcEvent exist custom chance, chance = spellProcEvent->customChance;
    if (spellProcEvent && spellProcEvent->customChance)
        chance = spellProcEvent->customChance;
    // If PPM exist calculate chance from PPM
    if (spellProcEvent && spellProcEvent->ppmRate != 0)
    {
        if (!isVictim)
        {
            uint32 WeaponSpeed = GetAttackTime(attType);
            chance = GetPPMProcChance(WeaponSpeed, spellProcEvent->ppmRate, spellProto);
        }
        else
        {
            uint32 WeaponSpeed = pVictim->GetAttackTime(attType);
            chance = pVictim->GetPPMProcChance(WeaponSpeed, spellProcEvent->ppmRate, spellProto);
        }
    }
    // Apply chance modifer aura
    if (Player* modOwner = GetSpellModOwner())
    {
        modOwner->ApplySpellMod(spellProto->Id,SPELLMOD_CHANCE_OF_SUCCESS,chance);
    }
    return roll_chance_f(chance);
}

bool Unit::HandleAuraRaidProcFromChargeWithValue(AuraEffect *triggeredByAura)
{
    // aura can be deleted at casts
    SpellEntry const *spellProto = triggeredByAura->GetSpellProto();
    uint32 effIdx = triggeredByAura->GetEffIndex();
    int32 heal = triggeredByAura->GetAmount();
    uint64 caster_guid = triggeredByAura->GetCasterGUID();

    //Currently only Prayer of Mending
    if (!(spellProto->SpellFamilyName == SPELLFAMILY_PRIEST && spellProto->SpellFamilyFlags[1] & 0x20))
    {
        sLog->outDebug("Unit::HandleAuraRaidProcFromChargeWithValue, received not handled spell: %u", spellProto->Id);
        return false;
    }

    // jumps
    int32 jumps = triggeredByAura->GetBase()->GetCharges()-1;

    // current aura expire
    triggeredByAura->GetBase()->SetCharges(1);             // will removed at next charges decrease

    // next target selection
    if (jumps > 0)
    {
        if (Unit * caster = triggeredByAura->GetCaster())
        {
            float radius = triggeredByAura->GetSpellProto()->GetSpellRadius(caster, effIdx);

            if (Unit *target = GetNextRandomRaidMemberOrPet(radius))
            {
                CastCustomSpell(target, spellProto->Id, &heal, NULL, NULL, true, NULL, triggeredByAura, caster_guid);
                if (Aura * aura = target->GetAura(spellProto->Id, caster->GetGUID()))
                    aura->SetCharges(jumps);
            }
        }
    }

    // heal
    CastCustomSpell(this, 33110, &heal, NULL, NULL, true, NULL, NULL, caster_guid);
    return true;

}
bool Unit::HandleAuraRaidProcFromCharge(AuraEffect* triggeredByAura)
{
    // aura can be deleted at casts
    SpellEntry const* spellProto = triggeredByAura->GetSpellProto();

    uint32 damageSpellId;
    switch (spellProto->Id)
    {
        case 57949:            //shiver
            damageSpellId = 57952;
            //animationSpellId = 57951; dummy effects for jump spell have unknown use (see also 41637)
            break;
        case 59978:            //shiver
            damageSpellId = 59979;
            break;
        case 43593:            //Cold Stare
            damageSpellId = 43594;
            break;
        default:
            sLog->outError("Unit::HandleAuraRaidProcFromCharge, received not handled spell: %u", spellProto->Id);
            return false;
    }

    uint64 caster_guid = triggeredByAura->GetCasterGUID();
    uint32 effIdx = triggeredByAura->GetEffIndex();

    // jumps
    int32 jumps = triggeredByAura->GetBase()->GetCharges()-1;

    // current aura expire
    triggeredByAura->GetBase()->SetCharges(1);             // will removed at next charges decrease

    // next target selection
    if (jumps > 0)
    {
        if (Unit * caster = triggeredByAura->GetCaster())
        {
            float radius = triggeredByAura->GetSpellProto()->GetSpellRadius(caster, effIdx);
            
            if (Unit* target= GetNextRandomRaidMemberOrPet(radius))
            {
                CastSpell(target, spellProto, true,NULL,triggeredByAura,caster_guid);
                if (Aura * aura = target->GetAura(spellProto->Id, caster->GetGUID()))
                    aura->SetCharges(jumps);
            }
        }
    }

    CastSpell(this, damageSpellId, true,NULL,triggeredByAura,caster_guid);

    return true;
}

void Unit::PlayOneShotAnimKit(uint32 id)
{
    WorldPacket data(SMSG_PLAY_ONE_SHOT_ANIM_KIT, 8+1);
    data.appendPackGUID(GetGUID());
    data << uint16(id);
    SendMessageToSet(&data, true);
}

void Unit::Kill(Unit *pVictim, bool durabilityLoss)
{
    // Prevent killing unit twice (and giving reward from kill twice)
    if (!pVictim->GetHealth())
        return;

    // Inform pets (if any) when player kills target)
    if (this->ToPlayer())
    {
        Pet *pPet = this->ToPlayer()->GetPet();
        if (pPet && pPet->IsAlive() && pPet->isControlled())
            pPet->AI()->KilledUnit(pVictim);
    }

    // find player: owner of controlled `this` or `this` itself maybe
    Player *player = GetCharmerOrOwnerPlayerOrPlayerItself();
    Creature *creature = pVictim->ToCreature();

    bool bRewardIsAllowed = true;
    if (creature)
    {
        bRewardIsAllowed = creature->IsDamageEnoughForLootingAndReward();
        if (!bRewardIsAllowed)
            creature->SetLootRecipient(NULL);
    }

    if (bRewardIsAllowed && creature && creature->GetLootRecipient())
        player = creature->GetLootRecipient();

    // Reward player, his pets, and group/raid members
    // call kill spell proc event (before real die and combat stop to triggering auras removed at death/combat stop)
    if (bRewardIsAllowed && player && player != pVictim)
    {
        WorldPacket data(SMSG_PARTYKILLLOG, (8+8)); //send event PARTY_KILL
        data << uint64(player->GetGUID()); //player with killing blow
        data << uint64(pVictim->GetGUID()); //victim

        Player* pLooter = player;

        if (Group *group = player->GetGroup())
        {
            group->BroadcastPacket(&data, group->GetMemberGroup(player->GetGUID()));

            if (creature)
            {
                group->UpdateLooterGuid(creature, true);
                if (group->GetLooterGuid())
                {
                    pLooter = sObjectMgr->GetPlayer(group->GetLooterGuid());
                    if (pLooter)
                    {
                        creature->SetLootRecipient(pLooter);   // update creature loot recipient to the allowed looter.
                        group->SendLooter(creature, pLooter);
                    }
                    else
                        group->SendLooter(creature, NULL);
                }
                else
                    group->SendLooter(creature, NULL);

                group->UpdateLooterGuid(creature);
            }
        }
        else
        {
            player->SendDirectMessage(&data);

            if (creature)
            {
                WorldPacket data2(SMSG_LOOT_LIST, (8+1+1));
                data2 << uint64(creature->GetGUID());
                data2 << uint8(0); // unk1
                data2 << uint8(0); // no group looter
                player->SendMessageToSet(&data2, true);
            }
        }

        if (creature)
        {
            Loot* loot = &creature->loot;
            if (creature->lootForPickPocketed)
                creature->lootForPickPocketed = false;

            loot->clear();
            if (uint32 lootid = creature->GetCreatureInfo()->lootid)
            {
                loot->setMap(creature->GetMap());
                loot->FillLoot(lootid, LootTemplates_Creature, pLooter, false, false, creature->GetLootMode());
            }

            loot->generateMoneyLoot(creature->GetCreatureInfo()->mingold,creature->GetCreatureInfo()->maxgold);
        }

        player->RewardPlayerAndGroupAtKill(pVictim);
    }

    // Do KILL and KILLED procs. KILL proc is called only for the unit who landed the killing blow (and its owner - for pets and totems) regardless of who tapped the victim
    if (IsPet() || IsTotem())
        if (Unit *owner = GetOwner())
            owner->ProcDamageAndSpell(pVictim, PROC_FLAG_KILL, PROC_FLAG_NONE, PROC_EX_NONE, 0);

    if (pVictim->GetCreatureType() != CREATURE_TYPE_CRITTER)
        ProcDamageAndSpell(pVictim, PROC_FLAG_KILL, PROC_FLAG_KILLED, PROC_EX_NONE, 0);

    // Proc auras on death - must be before aura/combat remove
    pVictim->ProcDamageAndSpell(NULL, PROC_FLAG_DEATH, PROC_FLAG_NONE, PROC_EX_NONE, 0, BASE_ATTACK, 0);

    // Rogues talent Venomous Wounds shall grant Rupture ability to regain energy on targets death
    if (pVictim->HasAura(1943))
    {
        Aura* pAura = pVictim->GetAura(1943);
        if (pAura)
        {
            Unit* pCaster = pAura->GetCaster();
            if (pCaster && (pCaster->HasAura(79133) || pCaster->HasAura(79134)))
            {
                float duration_mod = float(pAura->GetDuration()) / float(pAura->GetMaxDuration());
                int32 bp0 = 25.0f*duration_mod;
                pCaster->CastCustomSpell(pCaster, 51637, &bp0, 0, 0, true);
            }
        }
    }

    if (GetTypeId() == TYPEID_PLAYER && ToPlayer()->isHonorOrXPTarget(pVictim))
    {
        if (HasAura(49588)) // Unholy Command (Rank 1)
        {
            if (roll_chance_f(50.0f))
                ToPlayer()->RemoveSpellCooldown(49576,true);
        }
        else if(HasAura(49589)) // Unholy Command (Rank 2)
            ToPlayer()->RemoveSpellCooldown(49576,true);
    }

    sLog->outStaticDebug("SET JUST_DIED");
    pVictim->setDeathState(JUST_DIED);

    // 10% durability loss on death
    // clean InHateListOf
    if (pVictim->GetTypeId() == TYPEID_PLAYER)
    {
        // remember victim PvP death for corpse type and corpse reclaim delay
        // at original death (not at SpiritOfRedemtionTalent timeout)
        pVictim->ToPlayer()->SetPvPDeath(player != NULL);

        // only if not player and not controlled by player pet. And not at BG
        if ((durabilityLoss && !player && !pVictim->ToPlayer()->InBattleground()) || (player && sWorld->getBoolConfig(CONFIG_DURABILITY_LOSS_IN_PVP)))
        {
            sLog->outStaticDebug("We are dead, losing %f percent durability", sWorld->getRate(RATE_DURABILITY_LOSS_ON_DEATH));
            pVictim->ToPlayer()->DurabilityLossAll(sWorld->getRate(RATE_DURABILITY_LOSS_ON_DEATH),false);
            // durability lost message
            WorldPacket data(SMSG_DURABILITY_DAMAGE_DEATH, 0);
            pVictim->ToPlayer()->GetSession()->SendPacket(&data);
        }
        // Call KilledUnit for creatures
        if (GetTypeId() == TYPEID_UNIT && this->ToCreature()->IsAIEnabled)
            this->ToCreature()->AI()->KilledUnit(pVictim);

        // last damage from non duel opponent or opponent controlled creature
        if (pVictim->ToPlayer()->duel)
        {
            pVictim->ToPlayer()->duel->opponent->CombatStopWithPets(true);
            pVictim->ToPlayer()->CombatStopWithPets(true);
            pVictim->ToPlayer()->DuelComplete(DUEL_INTERRUPTED);
        }
    }
    else                                                // creature died
    {
        sLog->outStaticDebug("DealDamageNotPlayer");

        if (!creature->IsPet())
        {
            creature->DeleteThreatList();
            CreatureInfo const* cInfo = creature->GetCreatureInfo();
            if (cInfo && (cInfo->lootid || cInfo->maxgold > 0))
                creature->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
        }

        // Call KilledUnit for creatures, this needs to be called after the lootable flag is set
        if (GetTypeId() == TYPEID_UNIT && this->ToCreature()->IsAIEnabled)
            this->ToCreature()->AI()->KilledUnit(pVictim);

        // Call creature just died function
        if (creature->IsAIEnabled)
        {
            std::ostringstream ssPrint;
            creature->AI()->JustDied(this);

            InstanceMap* map = GetMap() ? GetMap()->ToInstanceMap() : NULL;
            if(map && map->GetInstanceScript() && creature->GetCreatureInfo()->rank == 3)
            {
                uint32 mapId=map->GetId();
                map->GetInstanceScript()->ResetRessurectionData();
                map->GetInstanceScript()->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, creature);

                /*Flexible raid locks rules- boss killed on normal or HC*/
                if(map->IsRaid() && sInstanceSaveMgr->isFlexibleEnabled(mapId))
                {
                    Map::PlayerList const &PlayerList = map->GetInstanceScript()->instance->GetPlayers();//do it for all players
                    uint32 playNumber=map->GetPlayersCountExceptGMs();
                    bool log=true;

                    /*Kill log*/
                    if(creature->GetEntry() == 45993/*Theralion*/ || (player && player->GetSession()->GetSecurity() > SEC_PLAYER && !playNumber)) //GM just killed boss alone
                        log=false;
                    else
                        ssPrint << "Players: ";

                    if (!PlayerList.isEmpty())
                    {
                        for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                        {
                            Player* pPlayer = i->getSource();
                            if(pPlayer && pPlayer->GetSession()->GetSecurity() == SEC_PLAYER)
                            {
                                map->doDifficultyStaff(pPlayer, mapId, map->GetInstanceId());
                                map->copyDeadUnitsFromLeader(pPlayer, mapId, map->GetInstanceId(),creature->GetDBTableGUIDLow());//kill units in all IDs of group members   
                                if(log)
                                    ssPrint << pPlayer->GetName() << "(" << pPlayer->GetGUIDLow() << "), ";
                            }
                        }
                    }
                    if(log)
                    {
                        ssPrint << "just killed Boss: " << creature->GetName() << "(" << creature->GetEntry() << ") on " << map->GetDifficulty() << " difficulty. Number of players in raid: " << playNumber;
                        if(player && player->GetGroup())
                            ssPrint << " in group: " << player->GetGroup()->GetMembersCount();
                        if(playNumber < map->GetMaxPlayers() || playNumber > map->GetMaxPlayers())
                            ssPrint << " POSSIBLE BUGGING!";
                        sLog->outChar("%s", ssPrint.str().c_str());
                    }
                }
            }

        }
        
        if (creature->ToTempSummon())
        {
            if (Unit* pSummoner = creature->ToTempSummon()->GetSummoner())
            {
                if (pSummoner->ToCreature() && pSummoner->ToCreature()->IsAIEnabled)
                    pSummoner->ToCreature()->AI()->SummonedCreatureDies(creature, this);
            }
        }

        if (creature->ToPet() && creature->GetOwner())
        {
            Unit* pOwner = creature->GetOwner();
            int32 bp0 = 0;
            switch(creature->GetEntry())
            {
                // Warlock Pets and Demonic Rebirth case
                case 416:   // Imp
                case 1860:  // Voidwalker
                case 1863:  // Succubus
                case 417:   // Felhunter
                case 17252: // Felguard
                case 11859: // Doomguard
                case 89:    // Infernal
                    if (pOwner->HasAura(88447))
                        bp0 = -100;
                    if (pOwner->HasAura(88446))
                        bp0 = -50;

                    if(bp0)
                    {
                        if (!pOwner->ToPlayer()->HasSpellCooldown(88448))
                        {
                            pOwner->CastCustomSpell(pOwner, 88448, &bp0, 0, 0, true);
                            pOwner->ToPlayer()->AddSpellCooldown(88448, 0, 120000); // add 2 min cd
                        }
                    }
                    break;
            }
        }

        // Dungeon specific stuff, only applies to players killing creatures
        if (creature->GetInstanceId())
        {
            Map *m = creature->GetMap();
            Player *creditedPlayer = GetCharmerOrOwnerPlayerOrPlayerItself();
            // TODO: do instance binding anyway if the charmer/owner is offline

            if (m->IsDungeon() && creditedPlayer)
            {
                if (m->IsRaidOrHeroicDungeon())
                {
                    if (creature->GetCreatureInfo()->flags_extra & CREATURE_FLAG_EXTRA_INSTANCE_BIND)
                        ((InstanceMap *)m)->PermBindAllPlayers(creditedPlayer, creature);
                }
                else
                {
                    // the reset time is set but not added to the scheduler
                    // until the players leave the instance
                    time_t resettime = creature->GetRespawnTimeEx() + 2 * HOUR;
                    if (InstanceSave *save = sInstanceSaveMgr->GetInstanceSave(creature->GetInstanceId()))
                        if (save->GetResetTime() < resettime) save->SetResetTime(resettime);
                }
            }
        }
    }

    // outdoor pvp things, do these after setting the death state, else the player activity notify won't work... doh...
    // handle player kill only if not suicide (spirit of redemption for example)
    if (player && this != pVictim)
    {
        if (OutdoorPvP * pvp = player->GetOutdoorPvP())
            pvp->HandleKill(player, pVictim);
        if (Battlefield* bf = sBattlefieldMgr.GetBattlefieldToZoneId(player->GetZoneId()))
            bf->HandleKill(player, pVictim);
    }

    //if (pVictim->GetTypeId() == TYPEID_PLAYER)
    //    if (OutdoorPvP * pvp = pVictim->ToPlayer()->GetOutdoorPvP())
    //        pvp->HandlePlayerActivityChangedpVictim->ToPlayer();

    // battleground things (do this at the end, so the death state flag will be properly set to handle in the bg->handlekill)
    if (player && player->InBattleground())
    {
        if (Battleground *bg = player->GetBattleground())
        {
            if (pVictim->GetTypeId() == TYPEID_PLAYER)
                bg->HandleKillPlayer((Player*)pVictim, player);
            else
                bg->HandleKillUnit(pVictim->ToCreature(), player);
        }
    }

    // achievement stuff
    if (pVictim->GetTypeId() == TYPEID_PLAYER)
    {
        if (GetTypeId() == TYPEID_UNIT)
            pVictim->ToPlayer()->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE, GetEntry());
        else if (GetTypeId() == TYPEID_PLAYER && pVictim != this)
            pVictim->ToPlayer()->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_PLAYER, 1, this->ToPlayer()->GetTeam());
    }

    //Hook for OnPVPKill Event
    if (this->GetTypeId() == TYPEID_PLAYER)
    {
        if (pVictim->GetTypeId() == TYPEID_PLAYER)
        {
            Player *killer = this->ToPlayer();
            Player *killed = pVictim->ToPlayer();
            sScriptMgr->OnPVPKill(killer, killed);
        }
        else if (pVictim->GetTypeId() == TYPEID_UNIT)
        {
            Player *killer = this->ToPlayer();
            Creature *killed = pVictim->ToCreature();
            sScriptMgr->OnCreatureKill(killer, killed);
        }
    }
    else if (this->GetTypeId() == TYPEID_UNIT)
    {
        if (pVictim->GetTypeId() == TYPEID_PLAYER)
        {
            Creature *killer = this->ToCreature();
            Player *killed = pVictim->ToPlayer();
            sScriptMgr->OnPlayerKilledByCreature(killer, killed);
        }
    }
}

void Unit::SetControlled(bool apply, UnitState state)
{
    if (apply)
    {
        if (HasUnitState(state))
            return;

        AddUnitState(state);
        switch(state)
        {
            case UNIT_STATE_STUNNED:
                SetStunned(true);
                CastStop();
                break;
            case UNIT_STATE_ROOT:
                if (!HasUnitState(UNIT_STATE_STUNNED))
                    SetRooted(true);
                break;
            case UNIT_STATE_CONFUSED:
                if (!HasUnitState(UNIT_STATE_STUNNED))
                {
                    ClearUnitState(UNIT_STATE_MELEE_ATTACKING);
                    SendMeleeAttackStop(m_attacking);
                    SetConfused(true);
                    CastStop();
                }
                break;
            case UNIT_STATE_FLEEING:
                if (!HasUnitState(UNIT_STATE_STUNNED | UNIT_STATE_CONFUSED))
                {
                    ClearUnitState(UNIT_STATE_MELEE_ATTACKING);
                    SendMeleeAttackStop(m_attacking);
                    SetFeared(true);
                    CastStop();
                }
                break;
            default:
                break;
        }
    }
    else
    {
        switch(state)
        {
            case UNIT_STATE_STUNNED: if (HasAuraType(SPELL_AURA_MOD_STUN))    return;
                                    else    SetStunned(false);    break;
            case UNIT_STATE_ROOT:    if (HasAuraType(SPELL_AURA_MOD_ROOT) || GetVehicle())    return;
                                    else    SetRooted(false);     break;
            case UNIT_STATE_CONFUSED:if (HasAuraType(SPELL_AURA_MOD_CONFUSE)) return;
                                    else    SetConfused(false);   break;
            case UNIT_STATE_FLEEING: if (HasAuraType(SPELL_AURA_MOD_FEAR))    return;
                                    else    SetFeared(false);     break;
            default: return;
        }

        ClearUnitState(state);

        if (HasUnitState(UNIT_STATE_STUNNED))
            SetStunned(true);
        else
        {
            if (HasUnitState(UNIT_STATE_ROOT))
                SetRooted(true);

            if (HasUnitState(UNIT_STATE_CONFUSED))
                SetConfused(true);
            else if (HasUnitState(UNIT_STATE_FLEEING))
                SetFeared(true);
        }
    }
}

void Unit::SendMoveRoot(uint32 value)
{
    ObjectGuid guid = GetGUID();
    WorldPacket data(SMSG_MOVE_ROOT, 1 + 8 + 4);
    data.WriteBit(guid[2]);
    data.WriteBit(guid[7]);
    data.WriteBit(guid[6]);
    data.WriteBit(guid[0]);
    data.WriteBit(guid[5]);
    data.WriteBit(guid[4]);
    data.WriteBit(guid[1]);
    data.WriteBit(guid[3]);

    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[0]);
    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[5]);

    data << uint32(value);


    data.WriteByteSeq(guid[3]);
    data.WriteByteSeq(guid[4]);
    data.WriteByteSeq(guid[7]);
    data.WriteByteSeq(guid[6]);

    SendMessageToSet(&data, true);
}

void Unit::SendMoveUnroot(uint32 value)
{
    ObjectGuid guid = GetGUID();
    WorldPacket data(SMSG_MOVE_UNROOT, 1 + 8 + 4);
    data.WriteBit(guid[0]);
    data.WriteBit(guid[1]);
    data.WriteBit(guid[3]);
    data.WriteBit(guid[7]);
    data.WriteBit(guid[5]);
    data.WriteBit(guid[2]);
    data.WriteBit(guid[4]);
    data.WriteBit(guid[6]);

    data.WriteByteSeq(guid[3]);
    data.WriteByteSeq(guid[6]);
    data.WriteByteSeq(guid[1]);

    data << uint32(value);

    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[0]);
    data.WriteByteSeq(guid[7]);
    data.WriteByteSeq(guid[4]);
    data.WriteByteSeq(guid[5]);

    SendMessageToSet(&data, true);
}

void Unit::SetStunned(bool apply)
{
    if (apply)
    {
        SetUInt64Value(UNIT_FIELD_TARGET, 0);
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
        CastStop();
        RemoveUnitMovementFlag(MOVEMENTFLAG_MASK_MOVING);
        AddUnitMovementFlag(MOVEMENTFLAG_ROOT);

        // Creature specific
        if (GetTypeId() != TYPEID_PLAYER)
            this->ToCreature()->StopMoving();
        else
            SetStandState(UNIT_STAND_STATE_STAND);

        SendMoveRoot(0);
    }
    else
    {
        if (IsAlive() && GetVictim())
            SetUInt64Value(UNIT_FIELD_TARGET, GetVictim()->GetGUID());

        // don't remove UNIT_FLAG_STUNNED for pet when owner is mounted (disabled pet's interface)
        Unit *pOwner = GetOwner();
        if (!pOwner || (pOwner->GetTypeId() == TYPEID_PLAYER && !pOwner->ToPlayer()->IsMounted()))
            RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);

        if (!HasUnitState(UNIT_STATE_ROOT))         // prevent allow move if have also root effect
        {
            SendMoveUnroot(0);
            RemoveUnitMovementFlag(MOVEMENTFLAG_ROOT);
        }
    }
}

void Unit::SetWaterWalk(bool apply)
{
    WorldPacket data;
    ObjectGuid guid = GetGUID();

    if (apply)
    {
        data.Initialize(SMSG_MOVE_WATER_WALK, 1 + 4 + 8);
        data.WriteBit(guid[4]);
        data.WriteBit(guid[7]);
        data.WriteBit(guid[6]);
        data.WriteBit(guid[0]);
        data.WriteBit(guid[1]);
        data.WriteBit(guid[3]);
        data.WriteBit(guid[5]);
        data.WriteBit(guid[2]);

        data.WriteByteSeq(guid[0]);
        data.WriteByteSeq(guid[5]);
        data.WriteByteSeq(guid[2]);
        data << uint32(0);          //! movement counter
        data.WriteByteSeq(guid[7]);
        data.WriteByteSeq(guid[3]);
        data.WriteByteSeq(guid[4]);
        data.WriteByteSeq(guid[1]);
        data.WriteByteSeq(guid[6]);
    }
    else
    {
        data.Initialize(SMSG_MOVE_LAND_WALK, 1 + 4 + 8);
        data.WriteBit(guid[5]);
        data.WriteBit(guid[1]);
        data.WriteBit(guid[6]);
        data.WriteBit(guid[2]);
        data.WriteBit(guid[3]);
        data.WriteBit(guid[4]);
        data.WriteBit(guid[0]);
        data.WriteBit(guid[7]);

        data.WriteByteSeq(guid[6]);
        data.WriteByteSeq(guid[1]);
        data.WriteByteSeq(guid[7]);
        data.WriteByteSeq(guid[5]);
        data.WriteByteSeq(guid[4]);
        data.WriteByteSeq(guid[0]);
        data.WriteByteSeq(guid[3]);
        data.WriteByteSeq(guid[2]);
        data << uint32(0);          //! movement counter
    }

    SendMessageToSet(&data, true);
}

void Unit::SetRooted(bool apply)
{
    if (apply)
    {
        if (m_rootTimes > 0) //blizzard internal check?
            m_rootTimes++;

        RemoveUnitMovementFlag(MOVEMENTFLAG_MASK_MOVING);
        AddUnitMovementFlag(MOVEMENTFLAG_ROOT);

        if(Player *plr = ToPlayer())
            plr->SendMoveRoot(m_rootTimes);
        else
        {
            // this check should catch not initialized target
            if (m_uint32Values)
            {
                ObjectGuid guid = GetGUID();
                WorldPacket data(SMSG_SPLINE_MOVE_ROOT, 8);
                data.WriteBit(guid[5]);
                data.WriteBit(guid[4]);
                data.WriteBit(guid[6]);
                data.WriteBit(guid[1]);
                data.WriteBit(guid[3]);
                data.WriteBit(guid[7]);
                data.WriteBit(guid[2]);
                data.WriteBit(guid[0]);
                data.FlushBits();
                data.WriteByteSeq(guid[2]);
                data.WriteByteSeq(guid[1]);
                data.WriteByteSeq(guid[7]);
                data.WriteByteSeq(guid[3]);
                data.WriteByteSeq(guid[5]);
                data.WriteByteSeq(guid[0]);
                data.WriteByteSeq(guid[6]);
                data.WriteByteSeq(guid[4]);
                SendMessageToSet(&data, true);
                StopMoving();
            }
        }
    }
    else
    {
        if (!HasUnitState(UNIT_STATE_STUNNED))      // prevent allow move if have also stun effect
        {
            m_rootTimes++; //blizzard internal check?

            if(Player* plr = ToPlayer())
                plr->SendMoveUnroot(m_rootTimes);
            else
            {
                ObjectGuid guid = GetGUID();
                WorldPacket data(SMSG_SPLINE_MOVE_UNROOT, 8);
                data.WriteBit(guid[0]);
                data.WriteBit(guid[1]);
                data.WriteBit(guid[6]);
                data.WriteBit(guid[5]);
                data.WriteBit(guid[3]);
                data.WriteBit(guid[2]);
                data.WriteBit(guid[7]);
                data.WriteBit(guid[4]);

                data.FlushBits();

                data.WriteByteSeq(guid[6]);
                data.WriteByteSeq(guid[3]);
                data.WriteByteSeq(guid[1]);
                data.WriteByteSeq(guid[5]);
                data.WriteByteSeq(guid[2]);
                data.WriteByteSeq(guid[0]);
                data.WriteByteSeq(guid[7]);
                data.WriteByteSeq(guid[4]);

                SendMessageToSet(&data, true);
            }
        }
    }
}

void Unit::SetFeared(bool apply)
{
    if (apply)
    {
        SetUInt64Value(UNIT_FIELD_TARGET, 0);

        Unit *caster = NULL;
        Unit::AuraEffectList const& fearAuras = GetAuraEffectsByType(SPELL_AURA_MOD_FEAR);
        if (!fearAuras.empty())
            caster = ObjectAccessor::GetUnit(*this, fearAuras.front()->GetCasterGUID());
        if (!caster)
            caster = getAttackerForHelper();
        GetMotionMaster()->MoveFleeing(caster, fearAuras.empty() ? sWorld->getIntConfig(CONFIG_CREATURE_FAMILY_FLEE_DELAY) : 0);             // caster == NULL processed in MoveFleeing
    }
    else
    {
        if (IsAlive())
        {
            if (GetMotionMaster()->GetCurrentMovementGeneratorType() == FLEEING_MOTION_TYPE)
                GetMotionMaster()->MovementExpired();
            if (GetVictim())
                SetUInt64Value(UNIT_FIELD_TARGET, GetVictim()->GetGUID());
        }
    }

    if (GetTypeId() == TYPEID_PLAYER)
        this->ToPlayer()->SetClientControl(this, !apply);
}

void Unit::SetConfused(bool apply)
{
    if (apply)
    {
        SetUInt64Value(UNIT_FIELD_TARGET, 0);
        GetMotionMaster()->MoveConfused();
    }
    else
    {
        if (IsAlive())
        {
            if (GetMotionMaster()->GetCurrentMovementGeneratorType() == CONFUSED_MOTION_TYPE)
                GetMotionMaster()->MovementExpired();
            if (GetVictim())
                SetUInt64Value(UNIT_FIELD_TARGET, GetVictim()->GetGUID());
        }
    }

    if (GetTypeId() == TYPEID_PLAYER)
        this->ToPlayer()->SetClientControl(this, !apply);
}

bool Unit::SetCharmedBy(Unit* charmer, CharmType type, AuraApplication const * aurApp)
{
    if (!charmer)
        return false;

    // unmount players when charmed
    if (GetTypeId() == TYPEID_PLAYER)
        Unmount();

    ASSERT(type != CHARM_TYPE_POSSESS || charmer->GetTypeId() == TYPEID_PLAYER);
    ASSERT((type == CHARM_TYPE_VEHICLE) == IsVehicle());

    sLog->outDebug("SetCharmedBy: charmer %u (GUID %u), charmed %u (GUID %u), type %u.", charmer->GetEntry(), charmer->GetGUIDLow(), GetEntry(), GetGUIDLow(), uint32(type));

    if (this == charmer)
    {
        sLog->outCrash("Unit::SetCharmedBy: Unit %u (GUID %u) is trying to charm itself!", GetEntry(), GetGUIDLow());
        return false;
    }

    //if (HasUnitState(UNIT_STATE_UNATTACKABLE))
    //    return false;

    if (GetTypeId() == TYPEID_PLAYER && this->ToPlayer()->GetTransport())
    {
        sLog->outCrash("Unit::SetCharmedBy: Player on transport is trying to charm %u (GUID %u)", GetEntry(), GetGUIDLow());
        return false;
    }

    // Already charmed
    if (GetCharmerGUID())
    {
        sLog->outCrash("Unit::SetCharmedBy: %u (GUID %u) has already been charmed but %u (GUID %u) is trying to charm it!", GetEntry(), GetGUIDLow(), charmer->GetEntry(), charmer->GetGUIDLow());
        return false;
    }

    CastStop();
    CombatStop(); //TODO: CombatStop(true) may cause crash (interrupt spells)
    DeleteThreatList();

    // Charmer stop charming
    if (charmer->GetTypeId() == TYPEID_PLAYER)
    {
        charmer->ToPlayer()->StopCastingCharm();
        charmer->ToPlayer()->StopCastingBindSight();
    }

    // Charmed stop charming
    if (GetTypeId() == TYPEID_PLAYER)
    {
        this->ToPlayer()->StopCastingCharm();
        this->ToPlayer()->StopCastingBindSight();
    }

    // StopCastingCharm may remove a possessed pet?
    if (!IsInWorld())
    {
        sLog->outCrash("Unit::SetCharmedBy: %u (GUID %u) is not in world but %u (GUID %u) is trying to charm it!", GetEntry(), GetGUIDLow(), charmer->GetEntry(), charmer->GetGUIDLow());
        return false;
    }

    // charm is set by aura, and aura effect remove handler was called during apply handler execution
    // prevent undefined behaviour
    if (aurApp && aurApp->GetRemoveMode())
        return false;

    // Set charmed
    Map* pMap = GetMap();
    if (!IsVehicle() || (IsVehicle() && pMap && !pMap->IsBattleground()))
        setFaction(charmer->getFaction());

    charmer->SetCharm(this, true);

    if (GetTypeId() == TYPEID_UNIT)
    {
        this->ToCreature()->AI()->OnCharmed(true);
        GetMotionMaster()->MoveIdle();
    }
    else
    {
        if (this->ToPlayer()->isAFK())
            this->ToPlayer()->ToggleAFK();
        this->ToPlayer()->SetClientControl(this, 0);
    }

    // charm is set by aura, and aura effect remove handler was called during apply handler execution
    // prevent undefined behaviour
    if (aurApp && aurApp->GetRemoveMode())
        return false;

    // Pets already have a properly initialized CharmInfo, don't overwrite it.
    if (type != CHARM_TYPE_VEHICLE && !GetCharmInfo())
    {
        CharmInfo *charmInfo = InitCharmInfo();
        if (type == CHARM_TYPE_POSSESS)
            charmInfo->InitPossessCreateSpells();
        else
            charmInfo->InitCharmCreateSpells();
    }

    if (charmer->GetTypeId() == TYPEID_PLAYER)
    {
        switch(type)
        {
            case CHARM_TYPE_VEHICLE:
                SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
                charmer->ToPlayer()->SetClientControl(this, 1);
                charmer->ToPlayer()->SetMover(this);
                charmer->ToPlayer()->SetViewpoint(this, true);
                charmer->ToPlayer()->VehicleSpellInitialize();
                break;
            case CHARM_TYPE_POSSESS:
                AddUnitState(UNIT_STATE_POSSESSED);
                SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
                charmer->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                charmer->ToPlayer()->SetClientControl(this, 1);
                charmer->ToPlayer()->SetViewpoint(this, true);
                charmer->ToPlayer()->PossessSpellInitialize();
                break;
            case CHARM_TYPE_CHARM:
                if (GetTypeId() == TYPEID_UNIT && charmer->getClass() == CLASS_WARLOCK)
                {
                    CreatureInfo const *cinfo = this->ToCreature()->GetCreatureInfo();
                    if (cinfo && cinfo->type == CREATURE_TYPE_DEMON)
                    {
                        //to prevent client crash
                        SetByteValue(UNIT_FIELD_BYTES_0, 1, (uint8)CLASS_MAGE);

                        //just to enable stat window
                        if (GetCharmInfo())
                            GetCharmInfo()->SetPetNumber(sObjectMgr->GeneratePetNumber(), true);

                        //if charmed two demons the same session, the 2nd gets the 1st one's name
                        SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(time(NULL))); // cast can't be helped
                    }
                }
                charmer->ToPlayer()->CharmSpellInitialize();
                break;
            default:
            case CHARM_TYPE_CONVERT:
                break;
        }
    }
    return true;
}

void Unit::RemoveCharmedBy(Unit *charmer)
{
    if (!IsCharmed())
        return;

    if (!charmer)
        charmer = GetCharmer();
    if (charmer != GetCharmer()) // one aura overrides another?
    {
//        sLog->outCrash("Unit::RemoveCharmedBy: this: " UI64FMTD " true charmer: " UI64FMTD " false charmer: " UI64FMTD,
//            GetGUID(), GetCharmerGUID(), charmer->GetGUID());
//        ASSERT(false);
        return;
    }

    CharmType type;
    if (HasUnitState(UNIT_STATE_POSSESSED))
        type = CHARM_TYPE_POSSESS;
    else if (charmer->IsOnVehicle(this))
        type = CHARM_TYPE_VEHICLE;
    else
        type = CHARM_TYPE_CHARM;

    CastStop();
    CombatStop(); //TODO: CombatStop(true) may cause crash (interrupt spells)
    getHostileRefManager().deleteReferences();
    DeleteThreatList();
    Map* pMap = GetMap();
    if (!IsVehicle() || (IsVehicle() && pMap && !pMap->IsBattleground()))
        RestoreFaction();
    GetMotionMaster()->InitDefault();

    if (type == CHARM_TYPE_POSSESS)
    {
        ClearUnitState(UNIT_STATE_POSSESSED);
        RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
    }

    if (GetTypeId() == TYPEID_UNIT)
    {
        this->ToCreature()->AI()->OnCharmed(false);

        if (type != CHARM_TYPE_VEHICLE)//Vehicles' AI is never modified
        {
            this->ToCreature()->AIM_Initialize();

            if (this->ToCreature()->AI() && charmer && charmer->IsAlive())
                this->ToCreature()->AI()->AttackStart(charmer);
        }
    }
    else
        this->ToPlayer()->SetClientControl(this, 1);

    // If charmer still exists
    if (!charmer)
        return;

    ASSERT(type != CHARM_TYPE_POSSESS || charmer->GetTypeId() == TYPEID_PLAYER);
    ASSERT(type != CHARM_TYPE_VEHICLE || (GetTypeId() == TYPEID_UNIT && IsVehicle()));

    charmer->SetCharm(this, false);

    if (charmer->GetTypeId() == TYPEID_PLAYER)
    {
        switch(type)
        {
            case CHARM_TYPE_VEHICLE:
                charmer->ToPlayer()->SetClientControl(charmer, 1);
                charmer->ToPlayer()->SetViewpoint(this, false);
                break;
            case CHARM_TYPE_POSSESS:
                charmer->ToPlayer()->SetClientControl(charmer, 1);
                charmer->ToPlayer()->SetViewpoint(this, false);
                charmer->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                break;
            case CHARM_TYPE_CHARM:
                if (GetTypeId() == TYPEID_UNIT && charmer->getClass() == CLASS_WARLOCK)
                {
                    CreatureInfo const *cinfo = this->ToCreature()->GetCreatureInfo();
                    if (cinfo && cinfo->type == CREATURE_TYPE_DEMON)
                    {
                        SetByteValue(UNIT_FIELD_BYTES_0, 1, uint8(cinfo->unit_class));
                        if (GetCharmInfo())
                            GetCharmInfo()->SetPetNumber(0, true);
                        else
                            sLog->outError("Aura::HandleModCharm: target=" UI64FMTD " with typeid=%d has a charm aura but no charm info!", GetGUID(), GetTypeId());
                    }
                }
                break;
            default:
            case CHARM_TYPE_CONVERT:
                break;
        }
    }

    // reapply all faction force auras
    std::list<uint32> modFactionAuras;
    AuraEffectList const &modFact = GetAuraEffectsByType(SPELL_AURA_MOD_FACTION);
    if (!modFact.empty())
    {
        for (AuraEffectList::const_iterator itr = modFact.begin(); itr != modFact.end(); ++itr)
            modFactionAuras.push_back((*itr)->GetBase()->GetId());

        if (!modFactionAuras.empty())
        {
            for (std::list<uint32>::iterator itr = modFactionAuras.begin(); itr != modFactionAuras.end(); ++itr)
            {
                RemoveAurasDueToSpell(*itr);
                CastSpell(this, *itr, true);
            }
        }
    }

    //a guardian should always have charminfo
    if (charmer->GetTypeId() == TYPEID_PLAYER && this != charmer->GetFirstControlled())
        charmer->ToPlayer()->SendRemoveControlBar();
    else if (GetTypeId() == TYPEID_PLAYER || (GetTypeId() == TYPEID_UNIT && !this->ToCreature()->IsGuardian()))
        DeleteCharmInfo();
}

void Unit::RestoreFaction()
{
    if (GetTypeId() == TYPEID_PLAYER)
        this->ToPlayer()->setFactionForRace(getRace());
    else
    {
        if (HasUnitTypeMask(UNIT_MASK_MINION))
        {
            if (Unit* owner = GetOwner())
            {
                setFaction(owner->getFaction());
                return;
            }
        }

        if (CreatureInfo const *cinfo = this->ToCreature()->GetCreatureInfo())  // normal creature
        {
            FactionTemplateEntry const *faction = getFactionTemplateEntry();
            setFaction((faction && faction->friendlyMask & 0x004) ? cinfo->faction_H : cinfo->faction_A);
        }
    }
}

bool Unit::CreateVehicleKit(uint32 id)
{
    VehicleEntry const *vehInfo = sVehicleStore.LookupEntry(id);
    if (!vehInfo)
        return false;

    m_vehicleKit = new Vehicle(this, vehInfo);
    m_updateFlag |= UPDATEFLAG_HAS_VEHICLE;
    m_unitTypeMask |= UNIT_MASK_VEHICLE;
    return true;
}

void Unit::RemoveVehicleKit()
{
    if (!m_vehicleKit)
        return;

    m_vehicleKit->Uninstall();
    delete m_vehicleKit;

    m_vehicleKit = NULL;

    m_updateFlag &= ~UPDATEFLAG_HAS_VEHICLE;
    m_unitTypeMask &= ~UNIT_MASK_VEHICLE;
    RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
    RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_PLAYER_VEHICLE);
}

Unit *Unit::GetVehicleBase() const
{
    return m_vehicle ? m_vehicle->GetBase() : NULL;
}

Creature *Unit::GetVehicleCreatureBase() const
{
    if (Unit *veh = GetVehicleBase())
        if (Creature *c = veh->ToCreature())
            return c;

    return NULL;
}

uint64 Unit::GetTransGUID() const
{
    if (GetVehicle())
        return GetVehicle()->GetBase()->GetGUID();
    if (GetTransport())
        return GetTransport()->GetGUID();

    return 0;
}

TransportBase* Unit::GetDirectTransport() const
{
    if (Vehicle* veh = GetVehicle())
        return veh;
    return (TransportBase*)GetTransport();
}

bool Unit::IsInPartyWith(Unit const *unit) const
{
    if (this == unit)
        return true;

    const Unit *u1 = GetCharmerOrOwnerOrSelf();
    const Unit *u2 = unit->GetCharmerOrOwnerOrSelf();
    if (u1 == u2)
        return true;

    if (u1->GetTypeId() == TYPEID_PLAYER && u2->GetTypeId() == TYPEID_PLAYER)
      return u1->ToPlayer()->IsInSameGroupWith(u2->ToPlayer());
    else
        return false;
}

bool Unit::IsInRaidWith(Unit const *unit) const
{
    if (this == unit)
        return true;

    const Unit *u1 = GetCharmerOrOwnerOrSelf();
    const Unit *u2 = unit->GetCharmerOrOwnerOrSelf();
    if (u1 == u2)
        return true;

    if (u1->GetTypeId() == TYPEID_PLAYER && u2->GetTypeId() == TYPEID_PLAYER)
      return u1->ToPlayer()->IsInSameRaidWith(u2->ToPlayer());
    else
        return false;
}

void Unit::GetRaidMember(std::list<Unit*> &nearMembers, float radius)
{
    Player *owner = GetCharmerOrOwnerPlayerOrPlayerItself();
    if (!owner)
        return;

    Group *pGroup = owner->GetGroup();
    if (pGroup)
    {
        for (GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* Target = itr->getSource();

            if (Target && !IsHostileTo(Target))
            {
                if (Target->IsAlive() && IsWithinDistInMap(Target, radius))
                    nearMembers.push_back(Target);

                if (Guardian* pet = Target->GetGuardianPet())
                    if (pet->IsAlive() &&  IsWithinDistInMap(pet, radius))
                        nearMembers.push_back(pet);
            }
        }
    }
    else
    {
        if (owner->IsAlive() && (owner == this || IsWithinDistInMap(owner, radius)))
            nearMembers.push_back(owner);
        if (Guardian* pet = owner->GetGuardianPet())
            if (pet->IsAlive() && (pet == this && IsWithinDistInMap(pet, radius)))
                nearMembers.push_back(pet);
    }
}

void Unit::GetRaidMemberDead(std::list<Unit*> &nearMembers, float radius)
{
    Player *owner = GetCharmerOrOwnerPlayerOrPlayerItself();
    if (!owner)
        return;

    Group *pGroup = owner->GetGroup();
    if (pGroup)
    {
        for (GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* Target = itr->getSource();

            if (Target && !IsHostileTo(Target))
            {
                if (Target->isDead() && IsWithinDistInMap(Target, radius))
                    nearMembers.push_back(Target);
            }
        }
    }
}

void Unit::GetPartyMemberInDist(std::list<Unit*> &TagUnitMap, float radius)
{
    Unit *owner = GetCharmerOrOwnerOrSelf();
    Group *pGroup = NULL;
    if (owner->GetTypeId() == TYPEID_PLAYER)
        pGroup = owner->ToPlayer()->GetGroup();

    if (pGroup)
    {
        uint8 subgroup = owner->ToPlayer()->GetSubGroup();

        for (GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* Target = itr->getSource();

            // IsHostileTo check duel and controlled by enemy
            if (Target && Target->GetSubGroup() == subgroup && !IsHostileTo(Target))
            {
                if (Target->IsAlive() && IsWithinDistInMap(Target, radius))
                    TagUnitMap.push_back(Target);

                if (Guardian* pet = Target->GetGuardianPet())
                    if (pet->IsAlive() &&  IsWithinDistInMap(pet, radius))
                        TagUnitMap.push_back(pet);
            }
        }
    }
    else
    {
        if (owner->IsAlive() && (owner == this || IsWithinDistInMap(owner, radius)))
            TagUnitMap.push_back(owner);
        if (Guardian* pet = owner->GetGuardianPet())
            if (pet->IsAlive() && (pet == this && IsWithinDistInMap(pet, radius)))
                TagUnitMap.push_back(pet);
    }
}

void Unit::GetPartyMembers(std::list<Unit*> &TagUnitMap)
{
    Unit *owner = GetCharmerOrOwnerOrSelf();
    Group *pGroup = NULL;
    if (owner->GetTypeId() == TYPEID_PLAYER)
        pGroup = owner->ToPlayer()->GetGroup();

    if (pGroup)
    {
        uint8 subgroup = owner->ToPlayer()->GetSubGroup();

        for (GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* Target = itr->getSource();

            // IsHostileTo check duel and controlled by enemy
            if (Target && Target->GetSubGroup() == subgroup && !IsHostileTo(Target))
            {
                if (Target->IsAlive() && IsInMap(Target))
                    TagUnitMap.push_back(Target);

                if (Guardian* pet = Target->GetGuardianPet())
                    if (pet->IsAlive() && IsInMap(Target))
                        TagUnitMap.push_back(pet);
            }
        }
    }
    else
    {
        if (owner->IsAlive() && (owner == this || IsInMap(owner)))
            TagUnitMap.push_back(owner);
        if (Guardian* pet = owner->GetGuardianPet())
            if (pet->IsAlive() && (pet == this || IsInMap(pet)))
                TagUnitMap.push_back(pet);
    }
}

Aura * Unit::AddAura(uint32 spellId, Unit *target)
{
    if (!target)
        return NULL;

    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo)
        return NULL;

    if (!target->IsAlive() && !(spellInfo->Attributes & SPELL_ATTR0_PASSIVE) && !(spellInfo->AttributesEx2 & SPELL_ATTR2_ALLOW_DEAD_TARGET))
        return NULL;

    return AddAura(spellInfo, MAX_EFFECT_MASK, target);
}

Aura * Unit::AddAura(SpellEntry const *spellInfo, uint8 effMask, Unit *target)
{
    if (!spellInfo)
        return NULL;

    if (target->IsImmunedToSpell(spellInfo))
        return NULL;

    for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (!(effMask & (1<<i)))
            continue;
        if (target->IsImmunedToSpellEffect(spellInfo, i))
            effMask &= ~(1<<i);
    }

    if (Aura * aura = Aura::TryCreate(spellInfo, effMask, target, this))
    {
        aura->ApplyForTargets();
        return aura;
    }
    return NULL;
}

void Unit::SetAuraStack(uint32 spellId, Unit *target, uint32 stack)
{
    Aura *aura = target->GetAura(spellId, GetGUID());
    if (!aura)
        aura = AddAura(spellId, target);
    if (aura && stack)
        aura->SetStackAmount(stack);
}

void Unit::ApplyResilience(const Unit *pVictim, int32 *damage) const
{
    if (IsVehicle() || pVictim->IsVehicle())
        return;

    const Unit *source = ToPlayer();
    if (!source)
    {
        source = ToCreature();
        if (source)
        {
            source = source->ToCreature()->GetOwner();
            if (source)
                source = source->ToPlayer();
        }
    }

    const Unit *target = pVictim->ToPlayer();
    if (!target)
    {
        target = pVictim->ToCreature();
        if (target)
        {
            target = target->ToCreature()->GetOwner();
            if (target)
                target = target->ToPlayer();
        }
    }

    if (!target)
        return;

    if (source && damage)
    {
        *damage -= target->ToPlayer()->GetPlayerDamageReduction(*damage);
    }
}

// Melee based spells can be miss, parry or dodge on this step
// Crit or block - determined on damage calculation phase! (and can be both in some time)
float Unit::MeleeSpellMissChance(const Unit *pVictim, WeaponAttackType attType, uint32 spellId) const
{
    // Calculate hit chance (more correct for chance mod)
    int32 HitChance;

    // PvP - PvE melee chances
    if (spellId || attType == RANGED_ATTACK || !haveOffhandWeapon())
        HitChance = 95;
    else
        HitChance = 76;

    // Hit chance depends from victim auras
    if (attType == RANGED_ATTACK)
        HitChance += pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_RANGED_HIT_CHANCE);
    else
    {
        int32 totalHitMod = pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_MELEE_HIT_CHANCE);

        if (totalHitMod <= -100) // AuraEffects with -100 basepoint should secure 100% miss chance
            return 100.0f;

        HitChance += totalHitMod;
    }


    // Spellmod from SPELLMOD_RESIST_MISS_CHANCE
    if (spellId)
    {
        if (Player *modOwner = GetSpellModOwner())
            modOwner->ApplySpellMod(spellId, SPELLMOD_RESIST_MISS_CHANCE, HitChance);
    }

    // Miss = 100 - hit
    float miss_chance = 100.0f - HitChance;

    // Bonuses from attacker aura and ratings
    if (attType == RANGED_ATTACK)
        miss_chance -= m_modRangedHitChance;
    else
        miss_chance -= m_modMeleeHitChance;

    // Level difference affects miss chance
    int32 levelDiff = pVictim->getLevel() - this->getLevel();
    miss_chance += levelDiff > 2 ? 1.0f + (levelDiff - 2) * 2.0f : levelDiff * 0.5f;

    // Limit miss chance from 0 to 60%
    if (miss_chance < 0.0f)
        return 0.0f;
    if (miss_chance > 60.0f)
        return 60.0f;
    return miss_chance;
}

void Unit::SetPhaseMask(uint32 newPhaseMask, bool update)
{
    if (newPhaseMask == GetPhaseMask())
        return;

    if (IsInWorld())
        RemoveNotOwnSingleTargetAuras(newPhaseMask);        // we can lost access to caster or target

    WorldObject::SetPhaseMask(newPhaseMask,update);

    // update terrain swap and map data override
    if (GetTypeId() == TYPEID_PLAYER)
        ToPlayer()->UpdateActivePhaseData();

    if (!IsInWorld())
        return;

    if (!m_Controlled.empty())
    {
        for (ControlList::const_iterator itr = m_Controlled.begin(); itr != m_Controlled.end(); ++itr)
            if ((*itr) && (*itr)->GetTypeId() == TYPEID_UNIT)
                (*itr)->SetPhaseMask(newPhaseMask,true);
    }

    for (uint8 i = 0; i < MAX_SUMMON_SLOT; ++i)
        if (m_SummonSlot[i])
            if (Creature *summon = GetMap()->GetCreature(m_SummonSlot[i]))
                summon->SetPhaseMask(newPhaseMask,true);
}

void Unit::UpdateObjectVisibility(bool forced)
{
    if (!forced)
        AddToNotify(NOTIFY_VISIBILITY_CHANGED);
    else
    {
        WorldObject::UpdateObjectVisibility(true);
        // call MoveInLineOfSight for nearby creatures
        Trinity::AIRelocationNotifier notifier(*this);
        VisitNearbyObject(GetMap()->GetVisibilityDistance(), notifier);
    }
}

void Unit::SendMoveKnockBack(Player* player, float speedXY, float speedZ, float vcos, float vsin)
{
    ObjectGuid guid = GetGUID();
    WorldPacket data(SMSG_MOVE_KNOCK_BACK, (1+8+4+4+4+4+4));
    data.WriteBit(guid[0]);
    data.WriteBit(guid[3]);
    data.WriteBit(guid[6]);
    data.WriteBit(guid[7]);
    data.WriteBit(guid[2]);
    data.WriteBit(guid[5]);
    data.WriteBit(guid[1]);
    data.WriteBit(guid[4]);

    data.WriteByteSeq(guid[1]);

    data << float(vsin);
    data << uint32(0);

    data.WriteByteSeq(guid[6]);
    data.WriteByteSeq(guid[7]);

    data << float(speedXY);

    data.WriteByteSeq(guid[4]);
    data.WriteByteSeq(guid[5]);
    data.WriteByteSeq(guid[3]);

    data << float(speedZ);
    data << float(vcos);

    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[0]);

    player->SendMessageToSet(&data, true);
}

void Unit::KnockbackFrom(float x, float y, float speedXY, float speedZ)
{
    Player *player = NULL;
    if (GetTypeId() == TYPEID_PLAYER)
        player = (Player*)this;
    else
    {
        player = dynamic_cast<Player*>(GetCharmer());
        if (player && player->m_mover != this)
            player = NULL;
    }

    if (!player)
    {
        GetMotionMaster()->MoveKnockbackFrom(x, y, speedXY, speedZ);
    }
    else
    {
        float vcos, vsin;
        GetSinCos(x, y, vsin, vcos);

        SendMoveKnockBack(player, speedXY, -speedZ, vcos, vsin);
    }
}

float Unit::GetCombatRatingReduction(CombatRating cr) const
{
    const Player *player = NULL;
    if (GetTypeId() == TYPEID_PLAYER)
    {
        player = ToPlayer();
    }
    else if (IsPet())
    {
        if (Unit *owner = GetOwner())
            player = owner->ToPlayer();
    }

    if (!player || !player->IsInWorld())
        return 0.0f;

    float coeff = player->GetRatingBonusValue(cr);
    return 100.0f - 100.0f * pow(0.99f, coeff);
}

uint32 Unit::GetCombatRatingDamageReduction(CombatRating cr, float rate, float cap, uint32 damage) const
{
    float percent = GetCombatRatingReduction(cr) * rate;
    if (percent > cap)
        percent = cap;
    return uint32 (percent * damage / 100.0f);
}

uint32 Unit::GetModelForForm(ShapeshiftForm form)
{
    switch(form)
    {
        case FORM_CAT:
            // Based on Hair color
            if (getRace() == RACE_NIGHTELF)
            {
                uint8 hairColor = GetByteValue(PLAYER_BYTES, 3);
                switch (hairColor)
                {
                    case 7: // Violet
                    case 8:
                        return 29405;
                    case 3: // Light Blue
                        return 29406;
                    case 0: // Green
                    case 1: // Light Green
                    case 2: // Dark Green
                        return 29407;
                    case 4: // White
                        return 29408;
                    default: // original - Dark Blue
                        return 892;
                }
            }
            // Based on Skin color
            else if (getRace() == RACE_TAUREN)
            {
                uint8 skinColor = GetByteValue(PLAYER_BYTES, 0);
                // Male
                if (getGender() == GENDER_MALE)
                {
                    switch(skinColor)
                    {
                        case 12: // White
                        case 13:
                        case 14:
                        case 18: // Completly White
                            return 29409;
                        case 9: // Light Brown
                        case 10:
                        case 11:
                            return 29410;
                        case 6: // Brown
                        case 7:
                        case 8:
                            return 29411;
                        case 0: // Dark
                        case 1:
                        case 2:
                        case 3: // Dark Grey
                        case 4:
                        case 5:
                            return 29412;
                        default: // original - Grey
                            return 8571;
                    }
                }
                // Female
                else switch (skinColor)
                {
                    case 10: // White
                        return 29409;
                    case 6: // Light Brown
                    case 7:
                        return 29410;
                    case 4: // Brown
                    case 5:
                        return 29411;
                    case 0: // Dark
                    case 1:
                    case 2:
                    case 3:
                        return 29412;
                    default: // original - Grey
                        return 8571;
                }
            }
            else if (getRace() == RACE_WORGEN)
            {
                // model based on skin color
                uint8 skinColor = GetByteValue(PLAYER_BYTES, 0);
                switch (skinColor)
                {
                    case 0: // Blue-gray
                    case 2: // Black
                        return 33661;
                    case 1: // Brown
                        return 33662;
                    case 3: // White
                        return 33663;
                    case 4: // Dark-Brown
                        return 33664;
                    default:
                        return 33660;
                }
            }
            else if (getRace() == RACE_TROLL)
            {
                // model based on hair color
                uint8 hairColor = GetByteValue(PLAYER_BYTES, 3);
                switch (hairColor)
                {
                    case 0: // Dark Red
                    case 1: // Red
                        return 33668;
                    case 2: // Dark Orange
                    case 3: // Orange
                        return 33667;
                    case 4: // Green
                    case 5: // Blue-green
                    case 6: // Blue
                        return 33666;
                    case 7: // Dark Blue
                        return 33665;
                    case 8: // Gray
                    case 9: // White
                        return 33669;
                    default:
                        return 33665;
                }
            }
            else if (Player::TeamForRace(getRace()) == ALLIANCE)
                return 892;
            else
                return 8571;
        case FORM_DIREBEAR:
        case FORM_BEAR:
            // Based on Hair color
            if (getRace() == RACE_NIGHTELF)
            {
                uint8 hairColor = GetByteValue(PLAYER_BYTES, 3);
                switch (hairColor)
                {
                    case 0: // Green
                    case 1: // Light Green
                    case 2: // Dark Green
                        return 29413; // 29415?
                    case 6: // Dark Blue
                        return 29414;
                    case 4: // White
                        return 29416;
                    case 3: // Light Blue
                        return 29417;
                    default: // original - Violet
                        return 2281;
                }
            }
            // Based on Skin color
            else if (getRace() == RACE_TAUREN)
            {
                uint8 skinColor = GetByteValue(PLAYER_BYTES, 0);
                // Male
                if (getGender() == GENDER_MALE)
                {
                    switch (skinColor)
                    {
                        case 0: // Dark (Black)
                        case 1:
                        case 2:
                            return 29418;
                        case 3: // White
                        case 4:
                        case 5:
                        case 12:
                        case 13:
                        case 14:
                            return 29419;
                        case 9: // Light Brown/Grey
                        case 10:
                        case 11:
                        case 15:
                        case 16:
                        case 17:
                            return 29420;
                        case 18: // Completly White
                            return 29421;
                        default: // original - Brown
                            return 2289;
                    }
                }
                // Female
                else switch (skinColor)
                {
                    case 0: // Dark (Black)
                    case 1:
                        return 29418;
                    case 2: // White
                    case 3:
                        return 29419;
                    case 6: // Light Brown/Grey
                    case 7:
                    case 8:
                    case 9:
                        return 29420;
                    case 10: // Completly White
                        return 29421;
                    default: // original - Brown
                        return 2289;
                }
            }
            else if (getRace() == RACE_WORGEN)
            {
                // model based on skin color
                uint8 skinColor = GetByteValue(PLAYER_BYTES, 0);
                switch (skinColor)
                {
                    case 0: // Blue-gray
                        return 33650;
                    case 1: // Brown
                        return 33652;
                    case 2: // Black
                        return 33651;
                    case 3: // White
                        return 33654;
                    case 4: // Dark-Brown
                        return 33654;
                    default:
                        return 33653;
                }
            }
            else if (getRace() == RACE_TROLL)
            {
                // model based on hair color
                uint8 hairColor = GetByteValue(PLAYER_BYTES, 3);
                switch (hairColor)
                {
                    case 0: // Dark Red
                    case 1: // Red
                        return 33657;
                    case 2: // Dark Orange
                    case 3: // Orange
                        return 33659;
                    case 4: // Green
                    case 5: // Blue-green
                    case 6: // Blue
                        return 33655;
                    case 7: // Dark Blue
                        return 33656;
                    case 8: // Gray
                    case 9: // White
                        return 33658;
                    default:
                        return 33655;
                }
            }
            else if (Player::TeamForRace(getRace()) == ALLIANCE)
                return 2281;
            else
                return 2289;
        case FORM_MOONKIN:
            switch (getRace())
            {
                case RACE_TROLL:
                    return 37174;
                case RACE_WORGEN:
                    return 37173;
                case RACE_NIGHTELF:
                    return 15374;
                case RACE_TAUREN:
                default:
                    return 15375;
            }
            break;
        case FORM_FLIGHT:
            if (getRace() == RACE_NIGHTELF || getRace() == RACE_WORGEN)
            {
                return 20857;
            }
            else if (getRace() == RACE_TAUREN)
            {
                return 20872;
            }
            else if (getRace() == RACE_TROLL)
            {
                return 37728;
            }
        case FORM_FLIGHT_EPIC:
            if (getRace() == RACE_NIGHTELF)
            {
                return 21243;
            }
            else if (getRace() == RACE_TAUREN)
            {
                return 21244;
            }
            else if (getRace() == RACE_WORGEN)
            {
                return 37729;
            }
            else if (getRace() == RACE_TROLL)
            {
                return 37730;
            }
        default:
        {
            uint32 modelid = 0;
            SpellShapeshiftFormEntry const* formEntry = sSpellShapeshiftFormStore.LookupEntry(form);
            if (formEntry && formEntry->modelID_A)
            {
                // Take the alliance modelid as default
                if (GetTypeId() != TYPEID_PLAYER)
                    return formEntry->modelID_A;
                else
                {
                    if (Player::TeamForRace(getRace()) == ALLIANCE)
                        modelid = formEntry->modelID_A;
                    else
                        modelid = formEntry->modelID_H;

                    // If the player is horde but there are no values for the horde modelid - take the alliance modelid
                    if (!modelid && Player::TeamForRace(getRace()) == HORDE)
                        modelid = formEntry->modelID_A;
                }
            }

            // Glyph of Arctic Wolf
            if (form == FORM_GHOSTWOLF && HasAura(58135))
                modelid = 27312;

            // Glyph of the Treant
            if (form == FORM_TREE && HasAura(95212))
                modelid = 864;

            return modelid;
        }
    }
    return 0;
}

uint32 Unit::GetModelForTotem(PlayerTotemType totemType)
{
    switch(getRace())
    {
        case RACE_ORC:
        {
            switch(totemType)
            {
                case SUMMON_TYPE_TOTEM_FIRE:    //fire
                    return 30758;
                case SUMMON_TYPE_TOTEM_EARTH:   //earth
                    return 30757;
                case SUMMON_TYPE_TOTEM_WATER:   //water
                    return 30759;
                case SUMMON_TYPE_TOTEM_AIR:     //air
                    return 30756;
            }
            break;
        }
        case RACE_DWARF:
        {
            switch(totemType)
            {
                case SUMMON_TYPE_TOTEM_FIRE:    //fire
                    return 30754;
                case SUMMON_TYPE_TOTEM_EARTH:   //earth
                    return 30753;
                case SUMMON_TYPE_TOTEM_WATER:   //water
                    return 30755;
                case SUMMON_TYPE_TOTEM_AIR:     //air
                    return 30736;
            }
            break;
        }
        case RACE_TROLL:
        {
            switch(totemType)
            {
                case SUMMON_TYPE_TOTEM_FIRE:    //fire
                    return 30762;
                case SUMMON_TYPE_TOTEM_EARTH:   //earth
                    return 30761;
                case SUMMON_TYPE_TOTEM_WATER:   //water
                    return 30763;
                case SUMMON_TYPE_TOTEM_AIR:     //air
                    return 30760;
            }
            break;
        }
        case RACE_TAUREN:
        {
            switch(totemType)
            {
                case SUMMON_TYPE_TOTEM_FIRE:    //fire
                    return 4589;
                case SUMMON_TYPE_TOTEM_EARTH:   //earth
                    return 4588;
                case SUMMON_TYPE_TOTEM_WATER:   //water
                    return 4587;
                case SUMMON_TYPE_TOTEM_AIR:     //air
                    return 4590;
            }
            break;
        }
        case RACE_DRAENEI:
        {
            switch(totemType)
            {
                case SUMMON_TYPE_TOTEM_FIRE:    //fire
                    return 19074;
                case SUMMON_TYPE_TOTEM_EARTH:   //earth
                    return 19073;
                case SUMMON_TYPE_TOTEM_WATER:   //water
                    return 19075;
                case SUMMON_TYPE_TOTEM_AIR:     //air
                    return 19071;
            }
            break;
        }
        case RACE_GOBLIN:
        {
            switch(totemType)
            {
                case SUMMON_TYPE_TOTEM_FIRE:    //fire
                    return 30783;
                case SUMMON_TYPE_TOTEM_EARTH:   //earth
                    return 30782;
                case SUMMON_TYPE_TOTEM_WATER:   //water
                    return 30784;
                case SUMMON_TYPE_TOTEM_AIR:     //air
                    return 30781;
            }
            break;
        }
    }
    return 0;
}

void Unit::JumpTo(float speedXY, float speedZ, bool forward)
{
    float angle = forward ? 0 : M_PI;
    if (GetTypeId() == TYPEID_UNIT)
        GetMotionMaster()->MoveJumpTo(angle, speedXY, speedZ);
    else
    {
        float vcos = cos(angle+GetOrientation());
        float vsin = sin(angle+GetOrientation());

        SendMoveKnockBack(ToPlayer(), speedXY, -speedZ, vcos, vsin);
    }
}

void Unit::JumpTo(WorldObject *obj, float speedZ)
{
    float x, y, z;
    obj->GetContactPoint(this, x, y, z);
    float speedXY = GetExactDist2d(x, y) * 10.0f / speedZ;
    GetMotionMaster()->MoveJump(x, y, z, speedXY, speedZ);
}

bool Unit::CheckPlayerCondition(Player* pPlayer)
{
    switch(GetEntry())
    {
            case 35644: //Argent Warhorse
            case 36558: //Argent Battleworg
                if (!pPlayer->HasItemOrGemWithIdEquipped(46106,1)) //Check item Argent Lance
                    return false;
            default:
                return true;
    }
}

bool Unit::IsPersonalUnit() const
{
    if (GetTypeId() != TYPEID_UNIT)
        return false;

    return sScriptDatabase->IsPersonalCreature(GetEntry());
}

void Unit::SetPersonalPlayer(uint64 guid)
{
    if (!IsPersonalUnit())
        return;

    m_personalPlayer = guid;
}

uint64 Unit::GetPersonalPlayer() const
{
    return m_personalPlayer;
}

void Unit::EnterVehicle(Vehicle *vehicle, int8 seatId, AuraApplication const* aurApp)
{
    if (!IsAlive() || GetVehicleKit() == vehicle)
        return;

    bool byAura = aurApp == NULL ? false : true;

    if (m_vehicle)
    {
        if (m_vehicle == vehicle)
        {
            if (seatId >= 0 && seatId != GetTransSeat())
            {
                sLog->outDebug("EnterVehicle: %u leave vehicle %u seat %d and enter %d.", GetEntry(), m_vehicle->GetBase()->GetEntry(), GetTransSeat(), seatId);
                ChangeSeat(seatId, byAura);
            }
            return;
        }
        else
        {
            sLog->outDebug("EnterVehicle: %u exit %u and enter %u.", GetEntry(), m_vehicle->GetBase()->GetEntry(), vehicle->GetBase()->GetEntry());
            ExitVehicle();
        }
    }

    if (Player* plr = ToPlayer())
    {
        if (vehicle->GetBase()->GetTypeId() == TYPEID_PLAYER && plr->IsInCombat())
        {
            if (aurApp)
                vehicle->GetBase()->RemoveAura(const_cast<AuraApplication*>(aurApp));
            return;
        }

        InterruptNonMeleeSpells(false);
        plr->StopCastingCharm();
        plr->StopCastingBindSight();
        Unmount();
        RemoveAurasByType(SPELL_AURA_MOUNTED);

        // drop flag at invisible in bg
        if (Battleground *bg = plr->GetBattleground())
            bg->EventPlayerDroppedFlag(plr);
    }

    ASSERT(!m_vehicle);
    m_vehicle = vehicle;
    if (!m_vehicle->AddPassenger(this, seatId, byAura))
    {
        m_vehicle = NULL;
        return;
    }

    if (Player* thisPlr = this->ToPlayer())
    {
        WorldPacket data(SMSG_ON_CANCEL_EXPECTED_RIDE_VEHICLE_AURA, 0);
        thisPlr->GetSession()->SendPacket(&data);

        data.Initialize(SMSG_BREAK_TARGET, 7);
        data.append(vehicle->GetBase()->GetPackGUID());
        thisPlr->GetSession()->SendPacket(&data);
    }

    SetControlled(true, UNIT_STATE_ROOT);
    //movementInfo is set in AddPassenger
    //packets are sent in AddPassenger
}

void Unit::ChangeSeat(int8 seatId, bool next, bool byAura)
{
    if (!m_vehicle)
        return;

    if (seatId < 0)
    {
        seatId = m_vehicle->GetNextEmptySeat(GetTransSeat(), next, byAura);
        if (seatId < 0)
            return;
    }
    else if (seatId == GetTransSeat() || !m_vehicle->HasEmptySeat(seatId))
        return;

    m_vehicle->RemovePassenger(this);
    if (!m_vehicle->AddPassenger(this, seatId, byAura))
        ASSERT(false);
}

void Unit::ExitVehicle()
{
    if (!m_vehicle || !this)
        return;

    Unit *vehicleBase = m_vehicle->GetBase();
    if(!vehicleBase)
        return;
    const AuraEffectList &modAuras = vehicleBase->GetAuraEffectsByType(SPELL_AURA_CONTROL_VEHICLE);
    for (AuraEffectList::const_iterator itr = modAuras.begin(); itr != modAuras.end(); ++itr)
    {
        if ((*itr) && (*itr)->GetBase() && (*itr)->GetBase()->GetOwner() == this)
        {
            vehicleBase->RemoveAura((*itr)->GetBase());
            break; // there should be no case that a vehicle has two auras for one owner
        }
    }

    //sLog->outError("exit vehicle");

    m_vehicle->RemovePassenger(this);

    // This should be done before dismiss, because there may be some aura removal
    Vehicle *vehicle = m_vehicle;
    m_vehicle = NULL;

    if(!vehicle)
        return;
    vehicleBase = vehicle->GetBase();
    if(!vehicleBase)
        return;

    SetControlled(false, UNIT_STATE_ROOT);

    Position pos;
    vehicleBase->GetPosition(&pos);

    //Send leave vehicle, not correct
    if (GetTypeId() == TYPEID_PLAYER)
    {
        //this->ToPlayer()->SetClientControl(this, 1);
        this->ToPlayer()->SetFallInformation(0, GetPositionZ());
    }

    Movement::MoveSplineInit init(this);
    init.MoveTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), false);
    init.SetFacing(GetOrientation());
    init.SetTransportExit();
    init.Launch();

    if (vehicleBase->GetTypeId() == TYPEID_UNIT)
        sScriptMgr->OnRemovePassenger(vehicle, this);

    if (vehicleBase->HasUnitTypeMask(UNIT_MASK_MINION))
        if (((Minion*)vehicleBase)->GetOwner() == this)
            vehicle->Dismiss();
}

void Unit::BuildMovementPacket(ByteBuffer *data) const
{
    *data << uint32(GetUnitMovementFlags());            // movement flags
    *data << uint16(GetExtraUnitMovementFlags());       // 2.3.0
    *data << uint32(getMSTime());                       // time / counter
    *data << GetPositionX();
    *data << GetPositionY();
    *data << GetPositionZMinusOffset();
    *data << GetOrientation();

    bool onTransport = m_movementInfo.t_guid != 0;
    bool hasInterpolatedMovement = m_movementInfo.flags2 & MOVEMENTFLAG2_INTERPOLATED_MOVEMENT;
    bool time3 = false;
    bool swimming = ((GetUnitMovementFlags() & (MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING))
        || (m_movementInfo.flags2 & MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING));
    bool interPolatedTurning = m_movementInfo.flags2 & MOVEMENTFLAG2_INTERPOLATED_TURNING;
    bool jumping = GetUnitMovementFlags() & MOVEMENTFLAG_FALLING;
    bool splineElevation = GetUnitMovementFlags() & MOVEMENTFLAG_SPLINE_ELEVATION;
    bool splineData = false;

    data->WriteBits(GetUnitMovementFlags(), 30);
    data->WriteBits(m_movementInfo.flags2, 12);
    data->WriteBit(onTransport);
    if (onTransport)
    {
        data->WriteBit(hasInterpolatedMovement);
        data->WriteBit(time3);
    }

    data->WriteBit(swimming);
    data->WriteBit(interPolatedTurning);
    if (interPolatedTurning)
        data->WriteBit(jumping);

    data->WriteBit(splineElevation);
    data->WriteBit(splineData);

    data->FlushBits(); // reset bit stream

    *data << uint64(GetGUID());
    *data << uint32(getMSTime());
    *data << float(GetPositionX());
    *data << float(GetPositionY());
    *data << float(GetPositionZ());
    *data << float(GetOrientation());

    if (onTransport)
    {
        if (m_vehicle)
            *data << uint64(m_vehicle->GetBase()->GetGUID());
        else if (GetTransport())
            *data << uint64(GetTransport()->GetGUID());
        else // probably should never happen
            *data << (uint64)0;

        *data << float (GetTransOffsetX());
        *data << float (GetTransOffsetY());
        *data << float (GetTransOffsetZ());
        *data << float (GetTransOffsetO());
        *data << uint8 (GetTransSeat());
        *data << uint32(GetTransTime());
        if (hasInterpolatedMovement)
            *data << int32(0); // Transport Time 2
        if (time3)
            *data << int32(0); // Transport Time 3
    }

    if (swimming)
        *data << (float)m_movementInfo.pitch;

    if (interPolatedTurning)
    {
        *data << (uint32)m_movementInfo.fallTime;
        *data << (float)m_movementInfo.j_zspeed;
        if (jumping)
        {
            *data << (float)m_movementInfo.j_sinAngle;
            *data << (float)m_movementInfo.j_cosAngle;
            *data << (float)m_movementInfo.j_xyspeed;
        }
    }

    if (splineElevation)
        *data << (float)m_movementInfo.splineElevation;
}

void Unit::SetFlying(bool apply)
{
    if (apply)
    {
        SetByteFlag(UNIT_FIELD_BYTES_1, 3, 0x02);
        AddUnitMovementFlag(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING);
    }
    else
    {
        RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, 0x02);
        RemoveUnitMovementFlag(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING);
    }
}

void Unit::NearTeleportTo(float x, float y, float z, float orientation, bool casting /*= false*/)
{
    DisableSpline();
    if (GetTypeId() == TYPEID_PLAYER)
        this->ToPlayer()->TeleportTo(GetMapId(), x, y, z, orientation, TELE_TO_NOT_LEAVE_TRANSPORT | TELE_TO_NOT_LEAVE_COMBAT | TELE_TO_NOT_UNSUMMON_PET | (casting ? TELE_TO_SPELL : 0));
    else
    {
        SetPosition(x, y, z, orientation, true);
        SendMovementFlagUpdate();
        UpdateObjectVisibility();
    }
}

bool Unit::SetPosition(float x, float y, float z, float orientation, bool teleport)
{
    // prevent crash when a bad coord is sent by the client
    if (!Trinity::IsValidMapCoord(x,y,z,orientation))
    {
        sLog->outDebug("Unit::SetPosition(%f, %f, %f) .. bad coordinates!",x,y,z);
        return false;
    }

    orientation = MapManager::NormalizeOrientation(orientation);
    bool turn = (GetOrientation() != orientation);
    bool relocated = (teleport || GetPositionX() != x || GetPositionY() != y || GetPositionZ() != z);

    if (turn)
        RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TURNING);

    if (relocated)
    {
        RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_MOVE);

        // move and update visible state if need
        if (GetTypeId() == TYPEID_PLAYER)
            GetMap()->PlayerRelocation((Player*)this, x, y, z, orientation);
        else
            GetMap()->CreatureRelocation(this->ToCreature(), x, y, z, orientation);
    }
    else if (turn)
        SetOrientation(orientation);

    if ((relocated || turn) && IsVehicle())
        GetVehicleKit()->RelocatePassengers(x,y,z,orientation);

    return (relocated || turn);
}

void Unit::SendThreatListUpdate()
{
    if (uint32 count = getThreatManager().getThreatList().size())
    {
        //sLog->outDebug("WORLD: Send SMSG_THREAT_UPDATE Message");
        WorldPacket data(SMSG_THREAT_UPDATE, 8 + count * 8);
        data.append(GetPackGUID());
        data << uint32(count);
        std::list<HostileReference*>& tlist = getThreatManager().getThreatList();
        for (std::list<HostileReference*>::const_iterator itr = tlist.begin(); itr != tlist.end(); ++itr)
        {
            data.appendPackGUID((*itr)->getUnitGuid());
            data << uint32((*itr)->getThreat()*100);
        }
        SendMessageToSet(&data, false);
    }
}

void Unit::SendChangeCurrentVictimOpcode(HostileReference* pHostileReference)
{
    if (uint32 count = getThreatManager().getThreatList().size())
    {
        sLog->outDebug("WORLD: Send SMSG_HIGHEST_THREAT_UPDATE Message");
        WorldPacket data(SMSG_HIGHEST_THREAT_UPDATE, 8 + 8 + count * 8);
        data.append(GetPackGUID());
        data.appendPackGUID(pHostileReference->getUnitGuid());
        data << uint32(count);
        std::list<HostileReference*>& tlist = getThreatManager().getThreatList();
        for (std::list<HostileReference*>::const_iterator itr = tlist.begin(); itr != tlist.end(); ++itr)
        {
            data.appendPackGUID((*itr)->getUnitGuid());
            data << uint32((*itr)->getThreat());
        }
        SendMessageToSet(&data, false);
    }
}

void Unit::SendClearThreatListOpcode()
{
    sLog->outDebug("WORLD: Send SMSG_THREAT_CLEAR Message");
    WorldPacket data(SMSG_THREAT_CLEAR, 8);
    data.append(GetPackGUID());
    SendMessageToSet(&data, false);
}

void Unit::SendRemoveFromThreatListOpcode(HostileReference* pHostileReference)
{
    sLog->outDebug("WORLD: Send SMSG_THREAT_REMOVE Message");
    WorldPacket data(SMSG_THREAT_REMOVE, 8 + 8);
    data.append(GetPackGUID());
    data.appendPackGUID(pHostileReference->getUnitGuid());
    SendMessageToSet(&data, false);
}

void Unit::RewardRage(uint32 damage, float weaponSpeedHitFactor, bool attacker, DamageEffectType damageType)
{
    float addRage = 0;

    if (attacker)
    {
        addRage = weaponSpeedHitFactor;

        // talent who gave more rage on attack
        addRage *= 1.0f + GetTotalAuraModifier(SPELL_AURA_MOD_RAGE_FROM_DAMAGE_DEALT) / 100.0f;
    }
    else
    {
        float coeff = 18.92f;
        if (damageType == SPELL_DIRECT_DAMAGE)
            coeff = 20.25f;

        addRage = coeff * damage / GetMaxHealth();

        // Berserker Rage effect
        if (HasAura(18499))
            addRage *= 2.0f;
    }

    if (addRage < 1.0f)
        addRage = 1.0f;

    addRage *= sWorld->getRate(RATE_POWER_RAGE_INCOME);

    ModifyPower(POWER_RAGE, uint32(addRage*10));
}

void Unit::StopAttackFaction(uint32 faction_id)
{
    if (Unit* victim = GetVictim())
    {
        if (victim->getFactionTemplateEntry()->faction == faction_id)
        {
            AttackStop();
            if (IsNonMeleeSpellCasted(false))
                InterruptNonMeleeSpells(false);

            // melee and ranged forced attack cancel
            if (GetTypeId() == TYPEID_PLAYER)
                this->ToPlayer()->SendAttackSwingCancelAttack();
        }
    }

    AttackerSet const& attackers = getAttackers();
    for (AttackerSet::const_iterator itr = attackers.begin(); itr != attackers.end();)
    {
        if ((*itr)->getFactionTemplateEntry()->faction == faction_id)
        {
            (*itr)->AttackStop();
            itr = attackers.begin();
        }
        else
            ++itr;
    }

    getHostileRefManager().deleteReferencesForFaction(faction_id);

    for (ControlList::const_iterator itr = m_Controlled.begin(); itr != m_Controlled.end(); ++itr)
            (*itr)->StopAttackFaction(faction_id);
}

void Unit::OutDebugInfo() const
{
    sLog->outError("Unit::OutDebugInfo");
    sLog->outString("GUID " UI64FMTD ", entry %u, type %u, name %s", GetGUID(), GetEntry(), (uint32)GetTypeId(), GetName());
    sLog->outString("OwnerGUID " UI64FMTD ", MinionGUID " UI64FMTD ", CharmerGUID " UI64FMTD ", CharmedGUID " UI64FMTD, GetOwnerGUID(), GetMinionGUID(), GetCharmerGUID(), GetCharmGUID());
    sLog->outString("In world %u, unit type mask %u", (uint32)(IsInWorld() ? 1 : 0), m_unitTypeMask);
    if (IsInWorld())
        sLog->outString("Mapid %u", GetMapId());

    sLog->outStringInLine("Summon Slot: ");
    for (uint32 i = 0; i < MAX_SUMMON_SLOT; ++i)
        sLog->outStringInLine(UI64FMTD", ", m_SummonSlot[i]);
    sLog->outString();

    sLog->outStringInLine("Controlled List: ");
    for (ControlList::const_iterator itr = m_Controlled.begin(); itr != m_Controlled.end(); ++itr)
        sLog->outStringInLine(UI64FMTD", ", (*itr)->GetGUID());
    sLog->outString();

    sLog->outStringInLine("Aura List: ");
    for (AuraApplicationMap::const_iterator itr = m_appliedAuras.begin(); itr != m_appliedAuras.end(); ++itr)
        sLog->outStringInLine("%u, ", itr->first);
    sLog->outString();

    if (IsVehicle())
    {
        sLog->outStringInLine("Passenger List: ");
        for (SeatMap::iterator itr = GetVehicleKit()->m_Seats.begin(); itr != GetVehicleKit()->m_Seats.end(); ++itr)
            if (Unit *passenger = itr->second.passenger)
                sLog->outStringInLine(UI64FMTD", ", passenger->GetGUID());
        sLog->outString();
    }

    if (GetVehicle())
        sLog->outString("On vehicle %u.", GetVehicleBase()->GetEntry());
}

uint32 Unit::GetRemainingPeriodicAmount(uint64 casterGUID, uint32 spellId, AuraType auraType, uint8 effectIndex) const
{
    uint32 amount = 0;

    // Make a copy so we can prevent iterator invalidation
    AuraEffectList periodicAuras(GetAuraEffectsByType(auraType));
    for (AuraEffectList::const_iterator i = periodicAuras.begin(); i != periodicAuras.end(); ++i)
    {
        if ((*i)->GetCasterGUID() != casterGUID || (*i)->GetId() != spellId || (*i)->GetEffIndex() != effectIndex || !(*i)->GetTotalTicks())
            continue;
        int32 ticksRemaining = std::max<int32>((*i)->GetTotalTicks() - int32((*i)->GetTickNumber()), 0);
        uint32 damage = (*i)->GetAmount();

        uint32 auraTypeId = uint32(auraType);

        switch (auraTypeId)
        {
            case SPELL_AURA_PERIODIC_HEAL:
            case SPELL_AURA_OBS_MOD_HEALTH:
            case SPELL_AURA_PERIODIC_DAMAGE:
            case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
            case SPELL_AURA_PERIODIC_LEECH:
                damage = (*i)->GetDamage();
                break;
        }
        /*char buffer[100];
        sprintf(buffer, "Remaining damage is %d", damage);
        const_cast<Unit*>(this)->MonsterSay(buffer, LANG_UNIVERSAL, 0);*/

        amount += uint32((damage * ticksRemaining) / (*i)->GetTotalTicks());
        break;
    }
    return amount;
}

bool Unit::IsVisionObscured(Unit* pVictim)
{
    Aura* victimAura = NULL;
    Aura* myAura = NULL;
    Unit* victimCaster = NULL;
    Unit* myCaster = NULL;

    AuraEffectList const& vAuras = pVictim->GetAuraEffectsByType(SPELL_AURA_INTERFERE_TARGETTING);
    for (AuraEffectList::const_iterator i = vAuras.begin(); i != vAuras.end(); ++i)
    {
        victimAura = (*i)->GetBase();
        victimCaster = victimAura->GetCaster();
        break;
    }
    AuraEffectList const& myAuras = GetAuraEffectsByType(SPELL_AURA_INTERFERE_TARGETTING);
    for (AuraEffectList::const_iterator i = myAuras.begin(); i != myAuras.end(); ++i)
    {
        myAura = (*i)->GetBase();
        myCaster = myAura->GetCaster();
        break;
    }

    if ((myAura != NULL && myCaster == NULL) || (victimAura != NULL && victimCaster == NULL))
        return false; // Failed auras, will result in crash

    // E.G. Victim is in smoke bomb, and I'm not
    // Spells fail unless I'm friendly to the caster of victim's smoke bomb
    if (victimAura != NULL && myAura == NULL)
    {
        if (IsFriendlyTo(victimCaster))
            return false;
        else 
            return true;
    }
    // Victim is not in smoke bomb, while I am
    // Spells fail if my smoke bomb aura's caster is my enemy
    else if (myAura != NULL && victimAura == NULL)
    {
        if (IsFriendlyTo(myCaster))
            return false;
        else
            return true;
    }
    return false;
}

uint32 Unit::GetResistance(SpellSchoolMask mask) const
{
    int32 resist = -1;
    for (int i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
        if (mask & (1 << i) && (resist < 0 || resist > int32(GetResistance(SpellSchools(i)))))
            resist = int32(GetResistance(SpellSchools(i)));

    // resist value will never be negative here
    return uint32(resist);
}

void CharmInfo::SetIsCommandAttack(bool val)
{
    m_isCommandAttack = val;
}

bool CharmInfo::IsCommandAttack()
{
    return m_isCommandAttack;
}

void CharmInfo::SaveStayPosition()
{
    m_unit->GetPosition(m_stayX, m_stayY, m_stayZ);
}

void CharmInfo::GetStayPosition(float &x, float &y, float &z)
{
    x = m_stayX;
    y = m_stayY;
    z = m_stayZ;
}

void CharmInfo::SetIsAtStay(bool val)
{
    m_isAtStay = val;
}

bool CharmInfo::IsAtStay()
{
    return m_isAtStay;
}

void CharmInfo::SetIsFollowing(bool val)
{
    m_isFollowing = val;
}

bool CharmInfo::IsFollowing()
{
    return m_isFollowing;
}

void CharmInfo::SetIsReturning(bool val)
{
    m_isReturning = val;
}

bool CharmInfo::IsReturning()
{
    return m_isReturning;
}

void Unit::SetInFront(Unit const* target)
{
    if (!HasUnitState(UNIT_STATE_CANNOT_TURN))
        SetOrientation(GetAngle(target));
}

void Unit::SetFacingTo(float ori)
{
    Movement::MoveSplineInit init(this);
    init.MoveTo(GetPositionX(), GetPositionY(), GetPositionZMinusOffset());
    init.SetFacing(ori);
    init.Launch();
}

void Unit::SetFacingToObject(WorldObject* pObject)
{
    // never face when already moving
    if (!IsStopped())
        return;

    // TODO: figure out under what conditions creature will move towards object instead of facing it where it currently is.
    SetFacingTo(GetAngle(pObject));
}

bool Unit::IsHackTriggeredAura(Unit *pVictim, Aura * aura, SpellEntry const* procSpell, uint32 procFlag, uint32 procExtra, WeaponAttackType attType, bool isVictim, bool active)
{
    // Return value: false - continue normal handling : true - do not continue

    // Check for ordinal things
    if (!aura || !aura->GetSpellProto())
        return false;

    SpellEntry const* dummySpell = aura->GetSpellProto();

    // BE CAREFUL ! no foregoing checks.
    // on melee procSpell = NULL, auras trigger themselves by other aura effects, etc..

    // All aura IDs to be registered here to prevent mistakes and not to handle regular spells
    switch(dummySpell->Id)
    {
        // add case
        case 14751:
        case 53576:
        case 53569:
        case 85767:
        case 77769:
        case 79683:
        case 89523:
            return true; // Continue handling
    }

    return false; // Will not be processed as triggered aura
}

bool Unit::HandleAuraProcHack(Unit *pVictim, Aura * aura, SpellEntry const* procSpell, uint32 procFlag, uint32 procExtra, WeaponAttackType attType, bool isVictim, bool active)
{
    // Return value: false - continue normal handling : true - do not continue

    // Check for ordinal things
    if (!aura || !aura->GetSpellProto())
        return false;

    SpellEntry const* dummySpell = aura->GetSpellProto();

    // BE CAREFUL ! no foregoing checks.
    // on melee procSpell = NULL, auras trigger themselves by other aura effects, etc..

    switch (dummySpell->SpellFamilyName)
    {
        case SPELLFAMILY_PRIEST:
        {
            switch (dummySpell->Id)
            {
                // Chakra
                case 14751:
                    {
                        if (!procSpell)
                            break;

                        // is caster
                        if (procFlag &
                            (PROC_FLAG_DONE_MELEE_AUTO_ATTACK |
                            PROC_FLAG_DONE_SPELL_MELEE_DMG_CLASS |
                            PROC_FLAG_DONE_RANGED_AUTO_ATTACK |
                            PROC_FLAG_DONE_SPELL_RANGED_DMG_CLASS |
                            PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_POS |
                            PROC_FLAG_DONE_SPELL_NONE_DMG_CLASS_NEG |
                            PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS |
                            PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG))
                        {
                            // these spells trigger different chakra states
                            switch (procSpell->Id)
                            {
                                // Chakra: Serenity
                                case 2050:  // Heal
                                case 2061:  // Flash Heal
                                case 2060:  // Greater Heal
                                case 32546: // Binding Heal
                                    RemoveAurasDueToSpell(81206);
                                    RemoveAurasDueToSpell(81207);
                                    RemoveAurasDueToSpell(81209);
                                    RemoveAurasDueToSpell(14751);
                                    CastSpell(this, 81208, true); // Refreshing Renew spell (81585) has its script effect handler
                                    return true;
                                // Chakra: Sanctuary
                                case 596:   // Prayer of Healing
                                case 33076: // Prayer of Mending
                                    RemoveAurasDueToSpell(81208);
                                    RemoveAurasDueToSpell(81209);
                                    RemoveAurasDueToSpell(14751);
                                    CastSpell(this, 81206, true);
                                    CastSpell(this, 81207, true);
                                    return true;
                                // Chakra: Chastise
                                case 585:   // Smite
                                case 73510: // Mind Spike
                                    RemoveAurasDueToSpell(81206);
                                    RemoveAurasDueToSpell(81207);
                                    RemoveAurasDueToSpell(81208);
                                    RemoveAurasDueToSpell(14751);
                                    CastSpell(this, 81209, true);
                                    return true;
                                default:
                                    break;
                            }
                        }
                        break;
                    }
                default:
                    break;
            }
        }
        case SPELLFAMILY_PALADIN:
        {
            // Infusion of Light
            if ((dummySpell->Id == 53576 || dummySpell->Id == 53569) && procExtra & PROC_EX_CRITICAL_HIT && procSpell && (procSpell->Id == 25914 || procSpell->Id == 25912))
            {
                CastSpell(this, dummySpell->EffectTriggerSpell[0], true);
                return true;
            }
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            // Trap Launcher - drop aura after trap usage
            if (dummySpell->Id == 77769 && procSpell)
            {
                if (procSpell->casterAuraSpell == 77769)
                {
                    RemoveAurasDueToSpell(77769);
                    RemoveAurasDueToSpell(82946);
                }
            }
            break;
        }
        case SPELLFAMILY_WARLOCK:
            // Dark Intent
            if (dummySpell->Id == 85767)
            {
                // proc only on periodic crit (heal / damage)
                if ((procExtra & PROC_EX_CRITICAL_HIT) && (procFlag & PROC_FLAG_DONE_PERIODIC))
                {
                    Unit* triggerTarget = NULL;
                    int32 bp0 = 0;
                    uint32 triggeredSpellId = 0;

                    if (aura->GetCaster())
                    {
                        triggerTarget = aura->GetCaster();

                        // Caster's spell is different. Caster get 3%, target gets only 1% bonus
                        if (aura->GetEffect(EFFECT_0) && aura->GetEffect(EFFECT_0)->GetScriptedAmount() > 0)
                            bp0 = 1;
                        else
                            bp0 = 3;

                        if (triggerTarget->GetTypeId() == TYPEID_PLAYER)
                        {
                            switch (triggerTarget->getClass())
                            {
                                case CLASS_WARLOCK:
                                    triggeredSpellId = 94310;
                                    break;
                                case CLASS_MAGE:
                                    triggeredSpellId = 85759;
                                    break;
                                case CLASS_PRIEST:
                                    triggeredSpellId = 94311;
                                    break;
                                case CLASS_DEATH_KNIGHT:
                                    triggeredSpellId = 94312;
                                    break;
                                case CLASS_WARRIOR:
                                    triggeredSpellId = 94313;
                                    break;
                                case CLASS_DRUID:
                                    triggeredSpellId = 94318;
                                    break;
                                case CLASS_SHAMAN:
                                    triggeredSpellId = 94319;
                                    break;
                                case CLASS_HUNTER:
                                    triggeredSpellId = 94320;
                                    break;
                                case CLASS_PALADIN:
                                    triggeredSpellId = 94323;
                                    break;
                                case CLASS_ROGUE:
                                    triggeredSpellId = 94324;
                                    break;
                            }
                        }
                        else
                            triggeredSpellId = 94310;
                    }

                    if (triggerTarget && bp0 && triggeredSpellId)
                        triggerTarget->CastCustomSpell(triggerTarget, triggeredSpellId, &bp0, NULL, NULL, true);
                }
            }
            break;
        case SPELLFAMILY_WARRIOR:
            // Grounding Totem glyphed spell, dunno why spellfamily warrior
            if (dummySpell->Id == 89523 && ((procExtra & PROC_EX_REFLECT) != 0))
            {
                if(IsAreaOfEffectSpell(procSpell) == false) // Don't allow AoE spells to consume grounding totem
                {
                    // reflect one spell, then die
                    if (Unit* caster = aura->GetUnitOwner())
                        if (caster->IsTotem())
                            caster->Kill(caster);
                }
            }
        default:
            break;
    }

    return false;
}

void Unit::SaveDamageTakenHistory(uint32 damage)
{
    uint32 time = getMSTime();
    if (m_lDamageTakenHistory.empty())
    {
        DamageTakenRecord data = {time, damage};
        m_lDamageTakenHistory.push_back(data);
    }
    else
    {
        // Is last record still up-to-date
        std::list<DamageTakenRecord>::iterator last = m_lDamageTakenHistory.end();
        --last; // Get back to the beginning of the last record
        uint32 timestamp = last->timestamp;
        if (timestamp > time - 1000)
        {
            // Add damage
            last->damage += damage;
        }
        else
        {
            // Fresh record
            DamageTakenRecord data = {time, damage};
            m_lDamageTakenHistory.push_back(data);
        }

        // Search existing records for outdated ones to delete
        for (std::list<DamageTakenRecord>::iterator itr = m_lDamageTakenHistory.begin(); itr != m_lDamageTakenHistory.end(); )
        {
            uint32 timestamp = itr->timestamp;
            if (timestamp < time - 10500)
                itr = m_lDamageTakenHistory.erase(itr);
            else
                itr++;
        }
    }
}

uint32 Unit::GetDamageTakenHistory(uint32 seconds)
{
    // Asked time rounded to a middle of a second
    uint32 duration = seconds * 1000 + 500;
    uint32 time = getMSTime();
    uint32 damage_history = 0;
    if (m_lDamageTakenHistory.empty())
        return 0;
    else
    {
        for(std::list<DamageTakenRecord>::const_iterator itr = m_lDamageTakenHistory.begin(); itr != m_lDamageTakenHistory.end(); ++itr)
        {
            DamageTakenRecord record = *itr;
            // For each record within searched duration
            uint32 timestamp = record.timestamp;
            if (timestamp > (time - duration))
            {
                // Add damage taken
                damage_history += record.damage;
            }
        }
    }
    // Return summary of the damage history withing given time
    return damage_history;
}

void Unit::SendPlaySpellVisualKit(uint32 id, uint32 unkParam, uint32 unkParam2)
{
    ObjectGuid guid = GetGUID();

    WorldPacket data(SMSG_PLAY_SPELL_VISUAL_KIT, 4 + 4+ 4 + 8);
    data << uint32(unkParam2);
    data << uint32(id);     // SpellVisualKit.dbc index
    data << uint32(unkParam);
    data.WriteBit(guid[4]);
    data.WriteBit(guid[7]);
    data.WriteBit(guid[5]);
    data.WriteBit(guid[3]);
    data.WriteBit(guid[1]);
    data.WriteBit(guid[2]);
    data.WriteBit(guid[0]);
    data.WriteBit(guid[6]);
    data.FlushBits();
    data.WriteByteSeq(guid[0]);
    data.WriteByteSeq(guid[4]);
    data.WriteByteSeq(guid[1]);
    data.WriteByteSeq(guid[6]);
    data.WriteByteSeq(guid[7]);
    data.WriteByteSeq(guid[2]);
    data.WriteByteSeq(guid[3]);
    data.WriteByteSeq(guid[5]);
    SendMessageToSet(&data, true);
}
