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
#include "SpellMgr.h"
#include "ObjectMgr.h"
#include "SpellAuras.h"
#include "SpellAuraDefines.h"

#include "DBCStores.h"
#include "World.h"
#include "Chat.h"
#include "Spell.h"
#include "BattlegroundMgr.h"
#include "BattlefieldMgr.h"
#include "CreatureAI.h"
#include "MapManager.h"
#include "BattlegroundIC.h"

bool IsAreaEffectTarget[TOTAL_SPELL_TARGETS];
SpellEffectTargetTypes EffectTargetType[TOTAL_SPELL_EFFECTS];
SpellSelectTargetTypes SpellTargetType[TOTAL_SPELL_TARGETS];

SpellMgr::SpellMgr()
{
    for (int i = 0; i < TOTAL_SPELL_EFFECTS; ++i)
    {
        switch(i)
        {
            case SPELL_EFFECT_PERSISTENT_AREA_AURA: //27
            case SPELL_EFFECT_SUMMON:               //28
            case SPELL_EFFECT_TRIGGER_MISSILE:      //32
            case SPELL_EFFECT_TRANS_DOOR:           //50 summon object
            case SPELL_EFFECT_SUMMON_PET:           //56
            case SPELL_EFFECT_ADD_FARSIGHT:         //72
            case SPELL_EFFECT_SUMMON_OBJECT_WILD:   //76
            //case SPELL_EFFECT_SUMMON_CRITTER:       //97 not 303
            case SPELL_EFFECT_SUMMON_OBJECT_SLOT1:  //104
            case SPELL_EFFECT_SUMMON_OBJECT_SLOT2:  //105
            case SPELL_EFFECT_SUMMON_OBJECT_SLOT3:  //106
            case SPELL_EFFECT_SUMMON_OBJECT_SLOT4:  //107
            case SPELL_EFFECT_SUMMON_DEAD_PET:      //109
            case SPELL_EFFECT_TRIGGER_SPELL_2:      //151 ritual of summon
                EffectTargetType[i] = SPELL_REQUIRE_DEST;
                break;
            case SPELL_EFFECT_PARRY: // 0
            case SPELL_EFFECT_BLOCK: // 0
            case SPELL_EFFECT_SKILL: // always with dummy 3 as A
            //case SPELL_EFFECT_LEARN_SPELL: // 0 may be 5 pet
            case SPELL_EFFECT_TRADE_SKILL: // 0 or 1
            case SPELL_EFFECT_PROFICIENCY: // 0
                EffectTargetType[i] = SPELL_REQUIRE_NONE;
                break;
            case SPELL_EFFECT_ENCHANT_ITEM:
            case SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY:
            case SPELL_EFFECT_DISENCHANT:
            //in 243 this is 0, in 309 it is 1
            //so both item target and unit target is pushed, and cause crash
            //case SPELL_EFFECT_FEED_PET:
            case SPELL_EFFECT_PROSPECTING:
            case SPELL_EFFECT_MILLING:
            case SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC:
                EffectTargetType[i] = SPELL_REQUIRE_ITEM;
                break;
            //caster must be pushed otherwise no sound
            case SPELL_EFFECT_APPLY_AREA_AURA_PARTY:
            case SPELL_EFFECT_APPLY_AREA_AURA_FRIEND:
            case SPELL_EFFECT_APPLY_AREA_AURA_ENEMY:
            case SPELL_EFFECT_APPLY_AREA_AURA_PET:
            case SPELL_EFFECT_APPLY_AREA_AURA_OWNER:
            case SPELL_EFFECT_APPLY_AREA_AURA_RAID:
            case SPELL_EFFECT_CHARGE:
            case SPELL_EFFECT_CHARGE_DEST:
            case SPELL_EFFECT_JUMP:
            case SPELL_EFFECT_JUMP_DEST:
            case SPELL_EFFECT_LEAP_BACK:
                EffectTargetType[i] = SPELL_REQUIRE_CASTER;
                break;
            //case SPELL_EFFECT_WMO_DAMAGE:
            //case SPELL_EFFECT_WMO_REPAIR:
            //case SPELL_EFFECT_WMO_CHANGE:
            //    EffectTargetType[i] = SPELL_REQUIRE_GOBJECT;
            //    break;
            default:
                EffectTargetType[i] = SPELL_REQUIRE_UNIT;
                break;
        }
    }

    for (int i = 0; i < TOTAL_SPELL_TARGETS; ++i)
    {
        switch(i)
        {
            case TARGET_UNIT_CASTER:
            case TARGET_UNIT_CASTER_FISHING:
            case TARGET_UNIT_CASTER_UNKNOWN:
            case TARGET_UNIT_MASTER:
            case TARGET_UNIT_PET:
            case TARGET_UNIT_PARTY_CASTER:
            case TARGET_UNIT_RAID_CASTER:
            case TARGET_UNIT_VEHICLE:
            case TARGET_UNIT_PASSENGER_0:
            case TARGET_UNIT_PASSENGER_1:
            case TARGET_UNIT_PASSENGER_2:
            case TARGET_UNIT_PASSENGER_3:
            case TARGET_UNIT_PASSENGER_4:
            case TARGET_UNIT_PASSENGER_5:
            case TARGET_UNIT_PASSENGER_6:
            case TARGET_UNIT_PASSENGER_7:
            case TARGET_UNIT_SUMMONER:
                SpellTargetType[i] = TARGET_TYPE_UNIT_CASTER;
                break;
            case TARGET_UNIT_TARGET_PUPPET:
            case TARGET_UNIT_TARGET_ALLY:
            case TARGET_UNIT_TARGET_RAID:
            case TARGET_UNIT_TARGET_ANY:
            case TARGET_UNIT_TARGET_ENEMY:
            case TARGET_UNIT_TARGET_PARTY:
            case TARGET_UNIT_PARTY_TARGET:
            case TARGET_UNIT_CLASS_TARGET:
            case TARGET_UNIT_CHAINHEAL:
                SpellTargetType[i] = TARGET_TYPE_UNIT_TARGET;
                break;
            case TARGET_UNIT_NEARBY_ENEMY:
            case TARGET_UNIT_NEARBY_ALLY:
            case TARGET_UNIT_NEARBY_ALLY_UNK:
            case TARGET_UNIT_NEARBY_ENTRY:
            case TARGET_UNIT_NEARBY_RAID:
            case TARGET_GAMEOBJECT_NEARBY_ENTRY:
                SpellTargetType[i] = TARGET_TYPE_UNIT_NEARBY;
                break;
            case TARGET_UNIT_AREA_ENEMY_SRC:
            case TARGET_UNIT_AREA_ALLY_SRC:
            case TARGET_UNIT_AREA_ENTRY_SRC:
            case TARGET_UNIT_AREA_PARTY_SRC:
            case TARGET_UNIT_AREA_PARTY_SRC_2:
            case TARGET_GAMEOBJECT_AREA_SRC:
                SpellTargetType[i] = TARGET_TYPE_AREA_SRC;
                break;
            case TARGET_UNIT_AREA_ENEMY_DST:
            case TARGET_UNIT_AREA_ALLY_DST:
            case TARGET_UNIT_AREA_ENTRY_DST:
            case TARGET_UNIT_AREA_PARTY_DST:
            case TARGET_GAMEOBJECT_AREA_DST:
                SpellTargetType[i] = TARGET_TYPE_AREA_DST;
                break;
            case TARGET_UNIT_CONE_ENEMY:
            case TARGET_UNIT_CONE_ALLY:
            case TARGET_UNIT_CONE_ENTRY:
            case TARGET_UNIT_CONE_ENEMY_UNKNOWN:
            case TARGET_UNIT_AREA_PATH:
            case TARGET_GAMEOBJECT_AREA_PATH:
            case TARGET_DEST_UNK_110: // Unknown target selecting, surely cone, but probably cone modified by some criterias
                SpellTargetType[i] = TARGET_TYPE_AREA_CONE;
                break;
            case TARGET_DST_CASTER:
            case TARGET_SRC_CASTER:
            case TARGET_MINION:
            case TARGET_DEST_CASTER_FRONT_LEAP:
            case TARGET_DEST_CASTER_FRONT:
            case TARGET_DEST_CASTER_BACK:
            case TARGET_DEST_CASTER_RIGHT:
            case TARGET_DEST_CASTER_LEFT:
            case TARGET_DEST_CASTER_FRONT_LEFT:
            case TARGET_DEST_CASTER_BACK_LEFT:
            case TARGET_DEST_CASTER_BACK_RIGHT:
            case TARGET_DEST_CASTER_FRONT_RIGHT:
            case TARGET_DEST_CASTER_RANDOM:
            case TARGET_DEST_CASTER_RADIUS:
                SpellTargetType[i] = TARGET_TYPE_DEST_CASTER;
                break;
            case TARGET_DST_TARGET_ENEMY:
            case TARGET_DEST_TARGET_ANY:
            case TARGET_DEST_TARGET_FRONT:
            case TARGET_DEST_TARGET_BACK:
            case TARGET_DEST_TARGET_RIGHT:
            case TARGET_DEST_TARGET_LEFT:
            case TARGET_DEST_TARGET_FRONT_LEFT:
            case TARGET_DEST_TARGET_BACK_LEFT:
            case TARGET_DEST_TARGET_BACK_RIGHT:
            case TARGET_DEST_TARGET_FRONT_RIGHT:
            case TARGET_DEST_TARGET_RANDOM:
            case TARGET_DEST_TARGET_RADIUS:
                SpellTargetType[i] = TARGET_TYPE_DEST_TARGET;
                break;
            case TARGET_DEST_DYNOBJ_ENEMY:
            case TARGET_DEST_DYNOBJ_ALLY:
            case TARGET_DEST_DYNOBJ_ALL_UNITS:
            case TARGET_DEST_DEST:
            case TARGET_DEST_TRAJ:
            case TARGET_DEST_DEST_FRONT_LEFT:
            case TARGET_DEST_DEST_BACK_LEFT:
            case TARGET_DEST_DEST_BACK_RIGHT:
            case TARGET_DEST_DEST_FRONT_RIGHT:
            case TARGET_DEST_DEST_FRONT:
            case TARGET_DEST_DEST_BACK:
            case TARGET_DEST_DEST_RIGHT:
            case TARGET_DEST_DEST_LEFT:
            case TARGET_DEST_DEST_RANDOM:
            case TARGET_DEST_DEST_RANDOM_DIR_DIST:
                SpellTargetType[i] = TARGET_TYPE_DEST_DEST;
                break;
            case TARGET_DST_DB:
            case TARGET_DST_HOME:
            case TARGET_DST_NEARBY_ENTRY:
                SpellTargetType[i] = TARGET_TYPE_DEST_SPECIAL;
                break;
            case TARGET_UNIT_CHANNEL_TARGET:
            case TARGET_DEST_CHANNEL_TARGET:
            case TARGET_DEST_CHANNEL_CASTER:
                SpellTargetType[i] = TARGET_TYPE_CHANNEL;
                break;
            default:
                SpellTargetType[i] = TARGET_TYPE_DEFAULT;
        }
    }

    for (int32 i = 0; i < TOTAL_SPELL_TARGETS; ++i)
    {
        switch(i)
        {
            case TARGET_UNIT_AREA_ENEMY_DST:
            case TARGET_UNIT_AREA_ENEMY_SRC:
            case TARGET_UNIT_AREA_ALLY_DST:
            case TARGET_UNIT_AREA_ALLY_SRC:
            case TARGET_UNIT_AREA_ENTRY_DST:
            case TARGET_UNIT_AREA_ENTRY_SRC:
            case TARGET_UNIT_AREA_PARTY_DST:
            case TARGET_UNIT_AREA_PARTY_SRC:
            case TARGET_UNIT_AREA_PARTY_SRC_2:
            case TARGET_UNIT_PARTY_TARGET:
            case TARGET_UNIT_PARTY_CASTER:
            case TARGET_UNIT_CONE_ENEMY:
            case TARGET_UNIT_CONE_ALLY:
            case TARGET_UNIT_CONE_ENEMY_UNKNOWN:
            case TARGET_UNIT_AREA_PATH:
            case TARGET_GAMEOBJECT_AREA_PATH:
            case TARGET_UNIT_RAID_CASTER:
            case TARGET_DEST_UNK_110:
                IsAreaEffectTarget[i] = true;
                break;
            default:
                IsAreaEffectTarget[i] = false;
                break;
        }
    }
}

SpellMgr::~SpellMgr()
{
}

SpellMgr& SpellMgr::Instance()
{
    static SpellMgr spellMgr;
    return spellMgr;
}

SpellScaling::SpellScaling()
{
    for(int i = 0; i < 3; i++)
    {
        avg[i] = 0.f;
        min[i] = 0.f;
        max[i] = 0.f;
        pts[i] = 0.f;
    }
    cast = 0;
    canScale = false;

    playerLevel = 1;
    spellEntry = NULL;
}

void SpellScaling::Init(uint8 playerLevel_, const SpellEntry * spellEntry_)
{
    playerLevel = playerLevel_;
    spellEntry = spellEntry_;

    if(!spellEntry->SpellScalingId)
        return;

    if(!spellEntry->SpellScaling_class)
        return;

    float base_coef = spellEntry->base_coef;
    uint8 base_level = spellEntry->base_level_coef;

    int32 ct_min = spellEntry->ct_min;
    int32 ct_max = spellEntry->ct_max;
    uint8 ct_level = spellEntry->ct_max_level;

    int8 class_ = spellEntry->SpellScaling_class;

    float gtCoef = GetGtSpellScalingValue(class_, playerLevel_);

    if(gtCoef == -1.0f)
        return;

    gtCoef *= ( std::min(playerLevel,base_level) + ( base_coef * std::max(0,playerLevel-base_level) ) )/playerLevel;

    //cast time
    cast = 0;
    if(ct_max>0 && playerLevel_>1 && ct_level>1)
        cast = ct_min+(((playerLevel-1)*(ct_max-ct_min))/(ct_level-1));
    else
        cast = ct_min;

    if(ct_min < ct_max)     // cast time increasing with level
    {
        if(cast > ct_max)
            cast = ct_max;
    }
    else                    // cast time decreasing with level (e.g. Starfire)
    {
        if (cast < ct_max)
            cast = ct_max;
    }

    //effects
    for(uint8 effIndex = 0; effIndex < 3; effIndex++)
    {
        float mult = spellEntry->coefMultiplier[effIndex];
        float randommult = spellEntry->coefRandomMultiplier[effIndex];
        float othermult = spellEntry->coefOther[effIndex];

        avg[effIndex] = mult*gtCoef;
        if(ct_max > 0)
            avg[effIndex] *= float(cast)/float(ct_max);

        min[effIndex]=roundf(avg[effIndex])-std::floor(avg[effIndex]*randommult/2);
        max[effIndex]=roundf(avg[effIndex])+std::floor(avg[effIndex]*randommult/2);
        pts[effIndex]=roundf(othermult*gtCoef);
        avg[effIndex]=std::max((float)ceil(mult),roundf(avg[effIndex]));
    }
    canScale = true;
}

bool SpellMgr::IsSrcTargetSpell(SpellEntry const *spellInfo) const
{
    for (uint8 i = 0; i< MAX_SPELL_EFFECTS; ++i)
    {
        if (SpellTargetType[spellInfo->EffectImplicitTargetA[i]] == TARGET_TYPE_AREA_SRC || SpellTargetType[spellInfo->EffectImplicitTargetB[i]] == TARGET_TYPE_AREA_SRC)
            return true;
    }
    return false;
}

int32 GetSpellDuration(SpellEntry const *spellInfo)
{
    if (!spellInfo)
        return 0;
    SpellDurationEntry const *du = sSpellDurationStore.LookupEntry(spellInfo->DurationIndex);
    if (!du)
        return 0;
    return (du->Duration[0] == -1) ? -1 : abs(du->Duration[0]);
}

int32 GetSpellMaxDuration(SpellEntry const *spellInfo)
{
    if (!spellInfo)
        return 0;
    SpellDurationEntry const *du = sSpellDurationStore.LookupEntry(spellInfo->DurationIndex);
    if (!du)
        return 0;
    return (du->Duration[2] == -1) ? -1 : abs(du->Duration[2]);
}

uint32 GetDispelChance(Unit* auraCaster, Unit* target, uint32 spellId, bool offensive, bool *result)
{
    // we assume that aura dispel chance is 100% on start
    // need formula for level difference based chance
    int32 resist_chance = 0;

    // Apply dispel mod from aura caster
    if (auraCaster)
        if (Player* modOwner = auraCaster->GetSpellModOwner())
            modOwner->ApplySpellMod(spellId, SPELLMOD_RESIST_DISPEL_CHANCE, resist_chance);

    // Dispel resistance from target SPELL_AURA_MOD_DISPEL_RESIST
    // Only affects offensive dispels
    if (offensive && target)
        resist_chance += target->GetTotalAuraModifier(SPELL_AURA_MOD_DISPEL_RESIST);

    // Try dispel
    if (result)
        *result = !roll_chance_i(resist_chance);

    resist_chance = resist_chance < 0 ? 0 : resist_chance;
    resist_chance = resist_chance > 100 ? 100 : resist_chance;
    return 100 - resist_chance;
}

uint32 GetSpellCastTime(SpellEntry const* spellInfo, Spell * spell)
{
    SpellCastTimesEntry const *spellCastTimeEntry = sSpellCastTimesStore.LookupEntry(spellInfo->CastingTimeIndex);

    // not all spells have cast time index and this is all is pasiive abilities
    if (!spellCastTimeEntry)
        return 0;

    int32 castTime = spellCastTimeEntry->CastTime;

    if (spell && spell->GetCaster() && spellInfo->CastingTimeIndex != 1)
    {
        SpellScaling values;
        values.Init(spell->GetCaster()->getLevel(), spellInfo);

        if(values.canScale)
        {
            castTime = values.cast;
        }

        // Tauren racial Cultivation - doesn't have effect for that
        // ..all ranks of Herb Gathering
        if (spellInfo->Id == 2366 ||
            spellInfo->Id == 2368 ||
            spellInfo->Id == 3570 ||
            spellInfo->Id == 11993 ||
            spellInfo->Id == 28695 ||
            spellInfo->Id == 50300 ||
            spellInfo->Id == 74519)
        {
            // Comments on Wowhead says, that cast time with that racial is aprox. 0.5s
            if (spell->GetCaster()->HasAura(20552))
                castTime = 500;
        }
    }
    
    if (spell && spell->GetCaster())
        spell->GetCaster()->ModSpellCastTime(spellInfo, castTime, spell);

    return (castTime > 0) ? uint32(castTime) : 0;
}

bool IsPassiveSpell(uint32 spellId)
{
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo)
        return false;
    return IsPassiveSpell(spellInfo);
}

bool IsPassiveSpell(SpellEntry const * spellInfo)
{
    if (spellInfo->Attributes & SPELL_ATTR0_PASSIVE)
        return true;
    return false;
}

bool IsAutocastableSpell(uint32 spellId)
{
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo)
        return false;
    if (spellInfo->Attributes & SPELL_ATTR0_PASSIVE)
        return false;
    if (spellInfo->AttributesEx & SPELL_ATTR1_UNAUTOCASTABLE_BY_PET)
        return false;
    return true;
}

bool IsHigherHankOfSpell(uint32 spellId_1, uint32 spellId_2)
{
    return sSpellMgr->GetSpellRank(spellId_1)<sSpellMgr->GetSpellRank(spellId_2);
}

uint32 CalculatePowerCost(SpellEntry const * spellInfo, Unit const * caster, SpellSchoolMask schoolMask)
{
    // Spell drain all exist power on cast (Only paladin lay of Hands)
    if (spellInfo->AttributesEx & SPELL_ATTR1_DRAIN_ALL_POWER)
    {
        // If power type - health drain all
        if (spellInfo->powerType == POWER_HEALTH)
            return caster->GetHealth();
        // Else drain all power
        if (spellInfo->powerType < MAX_POWERS)
            return caster->GetPower(Powers(spellInfo->powerType));
        sLog->outError("CalculateManaCost: Unknown power type '%d' in spell %d", spellInfo->powerType, spellInfo->Id);
        return 0;
    }

    // Base powerCost
    int32 powerCost = spellInfo->manaCost;
    // PCT cost from total amount
    if (spellInfo->ManaCostPercentage)
    {
        switch (spellInfo->powerType)
        {
            // health as power used
            case POWER_HEALTH:
                powerCost += spellInfo->ManaCostPercentage * caster->GetCreateHealth() / 100;
                break;
            case POWER_MANA:
                powerCost += spellInfo->ManaCostPercentage * caster->GetCreateMana() / 100;
                break;
            case POWER_RAGE:
            case POWER_FOCUS:
            case POWER_ENERGY:
            case POWER_HAPPINESS:
                powerCost += spellInfo->ManaCostPercentage * caster->GetMaxPower(Powers(spellInfo->powerType)) / 100;
                break;
            case POWER_RUNE:
            case POWER_RUNIC_POWER:
                sLog->outDebug("CalculateManaCost: Not implemented yet!");
                break;
            default:
                sLog->outError("CalculateManaCost: Unknown power type '%d' in spell %d", spellInfo->powerType, spellInfo->Id);
                return 0;
        }
    }
    SpellSchools school = GetFirstSchoolInMask(schoolMask);
    // Flat mod from caster auras by spell school
    powerCost += caster->GetInt32Value(UNIT_FIELD_POWER_COST_MODIFIER + school);
    // Shiv - costs 20 + weaponSpeed*10 energy (apply only to non-triggered spell with energy cost)
    if (spellInfo->AttributesEx4 & SPELL_ATTR4_SPELL_VS_EXTEND_COST)
        powerCost += caster->GetAttackTime(OFF_ATTACK)/100;
    // Apply cost mod by spell
    if (Player* modOwner = caster->GetSpellModOwner())
        modOwner->ApplySpellMod(spellInfo->Id, SPELLMOD_COST, powerCost);

    if (spellInfo->Attributes & SPELL_ATTR0_LEVEL_DAMAGE_CALCULATION)
        powerCost = int32(powerCost/ (1.117f* spellInfo->spellLevel / caster->getLevel() -0.1327f));

    // PCT mod from user auras by school
    if (powerCost > 0)
        powerCost = int32(powerCost * (1.0f+caster->GetFloatValue(UNIT_FIELD_POWER_COST_MULTIPLIER+school)));
    else
        powerCost = 0;

    return powerCost;
}

Unit* GetTriggeredSpellCaster(SpellEntry const * spellInfo, Unit * caster, Unit * target)
{
    for (uint8 i = 0 ; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (SpellTargetType[spellInfo->EffectImplicitTargetA[i]] == TARGET_TYPE_UNIT_TARGET
            || SpellTargetType[spellInfo->EffectImplicitTargetB[i]] == TARGET_TYPE_UNIT_TARGET
            || SpellTargetType[spellInfo->EffectImplicitTargetA[i]] == TARGET_TYPE_CHANNEL
            || SpellTargetType[spellInfo->EffectImplicitTargetB[i]] == TARGET_TYPE_CHANNEL
            || SpellTargetType[spellInfo->EffectImplicitTargetA[i]] == TARGET_TYPE_DEST_TARGET
            || SpellTargetType[spellInfo->EffectImplicitTargetB[i]] == TARGET_TYPE_DEST_TARGET)
            return caster;
    }
    return target;
}

AuraState GetSpellAuraState(SpellEntry const * spellInfo)
{
    // Seals
    if (IsSealSpell(spellInfo))
        return AURA_STATE_JUDGEMENT;

    // Conflagrate aura state on Immolate
    if (spellInfo->SpellFamilyName == SPELLFAMILY_WARLOCK && (spellInfo->SpellFamilyFlags[0] & 4))
        return AURA_STATE_CONFLAGRATE;

    // Faerie Fire (druid versions)
    if (spellInfo->SpellFamilyName == SPELLFAMILY_DRUID && spellInfo->SpellFamilyFlags[0] & 0x400)
        return AURA_STATE_FAERIE_FIRE;

    // Sting (hunter's pet ability)
    if (spellInfo->Category == 1133)
        return AURA_STATE_FAERIE_FIRE;

    // Victorious
    if (spellInfo->SpellFamilyName == SPELLFAMILY_WARRIOR &&  spellInfo->SpellFamilyFlags[1] & 0x00040000)
        return AURA_STATE_WARRIOR_VICTORY_RUSH;

    // Swiftmend state on Regrowth & Rejuvenation
    if (spellInfo->SpellFamilyName == SPELLFAMILY_DRUID && spellInfo->SpellFamilyFlags[0] & 0x50)
        return AURA_STATE_SWIFTMEND;

    // Deadly poison aura state
    if (spellInfo->SpellFamilyName == SPELLFAMILY_ROGUE && spellInfo->SpellFamilyFlags[0] & 0x10000)
        return AURA_STATE_DEADLY_POISON;

    // Enrage aura state
    if (spellInfo->Dispel == DISPEL_ENRAGE)
        return AURA_STATE_ENRAGE;

    // Bleeding aura state
    if (GetAllSpellMechanicMask(spellInfo) & 1<<MECHANIC_BLEED)
        return AURA_STATE_BLEEDING;

    if (GetSpellSchoolMask(spellInfo) & SPELL_SCHOOL_MASK_FROST)
    {
        for (uint8 i = 0; i<MAX_SPELL_EFFECTS; ++i)
        {
            if (spellInfo->EffectApplyAuraName[i] == SPELL_AURA_MOD_STUN
                || spellInfo->EffectApplyAuraName[i] == SPELL_AURA_MOD_ROOT)
                return AURA_STATE_FROZEN;
        }
    }

     switch (spellInfo->Id)
     {
         case 99390: // Imprinted
         case 71465: // Divine Surge
         case 50241: // Evasive Charges
             return AURA_STATE_UNKNOWN22;
             break;
         case 100360: // Imprinted2
             return AURA_STATE_UNKNOWN20;
             break;
         default:
             break;
     }

    return AURA_STATE_NONE;
}

SpellSpecific GetSpellSpecific(SpellEntry const * spellInfo)
{
    switch(spellInfo->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            // Food / Drinks (mostly)
            if (spellInfo->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED)
            {
                bool food = false;
                bool drink = false;
                for (int i = 0; i < MAX_SPELL_EFFECTS; ++i)
                {
                    switch(spellInfo->EffectApplyAuraName[i])
                    {
                        // Food
                        case SPELL_AURA_MOD_REGEN:
                        case SPELL_AURA_OBS_MOD_HEALTH:
                            food = true;
                            break;
                        // Drink
                        case SPELL_AURA_MOD_POWER_REGEN:
                        case SPELL_AURA_OBS_MOD_POWER:
                            drink = true;
                            break;
                        default:
                            break;
                    }
                }

                if (food && drink)
                    return SPELL_SPECIFIC_FOOD_AND_DRINK;
                else if (food)
                    return SPELL_SPECIFIC_FOOD;
                else if (drink)
                    return SPELL_SPECIFIC_DRINK;
            }
            // scrolls effects
            else
            {
                uint32 firstSpell = sSpellMgr->GetFirstSpellInChain(spellInfo->Id);
                switch (firstSpell)
                {
                    case 12880: // Enrage (Enrage)
                    case 57518: // Enrage (Wrecking Crew)
                        return SPELL_SPECIFIC_WARRIOR_ENRAGE;
                }
            }
            break;
        }
        case SPELLFAMILY_MAGE:
        {
            // family flags 18(Molten), 25(Frost/Ice), 28(Mage)
            if (spellInfo->SpellFamilyFlags[0] & 0x12040000)
                return SPELL_SPECIFIC_MAGE_ARMOR;

            // Arcane brillance and Arcane intelect (normal check fails because of flags difference)
            if (spellInfo->SpellFamilyFlags[0] & 0x400)
                return SPELL_SPECIFIC_MAGE_ARCANE_BRILLANCE;

            if ((spellInfo->SpellFamilyFlags[0] & 0x1000000) && spellInfo->EffectApplyAuraName[0] == SPELL_AURA_MOD_CONFUSE)
                return SPELL_SPECIFIC_MAGE_POLYMORPH;

            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            if (spellInfo->Id == 12292) // Death Wish
                return SPELL_SPECIFIC_WARRIOR_ENRAGE;

            break;
        }
        case SPELLFAMILY_WARLOCK:
        {
            // only warlock curses have this
            if (spellInfo->Dispel == DISPEL_CURSE)
            {
                if (spellInfo->Id == 980 || spellInfo->Id == 603 || spellInfo->Id == 80240)
                    return SPELL_SPECIFIC_WARLOCK_BANE;
                else
                    return SPELL_SPECIFIC_CURSE;
            }

            // Warlock (Demon Armor | Demon Skin | Fel Armor)
            if (spellInfo->SpellFamilyFlags[1] & 0x20000020 || spellInfo->SpellFamilyFlags[2] & 0x00000010)
                return SPELL_SPECIFIC_WARLOCK_ARMOR;

            //seed of corruption and corruption
            if (spellInfo->SpellFamilyFlags[1] & 0x10 || spellInfo->SpellFamilyFlags[0] & 0x2)
                return SPELL_SPECIFIC_WARLOCK_CORRUPTION;
            break;
        }
        case SPELLFAMILY_PRIEST:
        {
            // Divine Spirit and Prayer of Spirit
            if (spellInfo->SpellFamilyFlags[0] & 0x20)
                return SPELL_SPECIFIC_PRIEST_DIVINE_SPIRIT;

            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            // only hunter stings have this
            if (spellInfo->Dispel == DISPEL_POISON)
                return SPELL_SPECIFIC_STING;

            // only hunter aspects have this (but not all aspects in hunter family)
            if (spellInfo->SpellFamilyFlags.HasFlag(0x00380000, 0x00400000, 0x00001010))
                return SPELL_SPECIFIC_ASPECT;

            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            if (IsSealSpell(spellInfo))
                return SPELL_SPECIFIC_SEAL;

            if (spellInfo->SpellFamilyFlags[0] & 0x00002190)
                return SPELL_SPECIFIC_HAND;

            // Judgement of Wisdom, Judgement of Light, Judgement of Justice
            if (spellInfo->Id == 20184 || spellInfo->Id == 20185 || spellInfo->Id == 20186)
                return SPELL_SPECIFIC_JUDGEMENT;

            // only paladin auras have this (for palaldin class family)
            if (spellInfo->SpellFamilyFlags[2] & 0x00000020)
                return SPELL_SPECIFIC_AURA;

            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            if (IsElementalShield(spellInfo))
                return SPELL_SPECIFIC_ELEMENTAL_SHIELD;

            break;
        }

        case SPELLFAMILY_DEATHKNIGHT:
            if (spellInfo->Id == 48266 || spellInfo->Id == 48263 || spellInfo->Id == 48265)
            //if (spellInfo->Category == 47)
                return SPELL_SPECIFIC_PRESENCE;
            break;
    }

    for (int i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (spellInfo->Effect[i] == SPELL_EFFECT_APPLY_AURA)
        {
            switch(spellInfo->EffectApplyAuraName[i])
            {
                case SPELL_AURA_MOD_CHARM:
                case SPELL_AURA_MOD_POSSESS_PET:
                case SPELL_AURA_MOD_POSSESS:
                case SPELL_AURA_AOE_CHARM:
                    return SPELL_SPECIFIC_CHARM;
                case SPELL_AURA_TRACK_CREATURES:
                case SPELL_AURA_TRACK_STEALTHED:
                    return SPELL_SPECIFIC_TRACKER;
                case SPELL_AURA_PHASE:
                    return SPELL_SPECIFIC_PHASE;
            }
        }
    }

    return SPELL_SPECIFIC_NORMAL;
}

// target not allow have more one spell specific from same caster
bool IsSingleFromSpellSpecificPerCaster(SpellSpecific spellSpec1,SpellSpecific spellSpec2)
{
    switch(spellSpec1)
    {
        case SPELL_SPECIFIC_SEAL:
        case SPELL_SPECIFIC_HAND:
        case SPELL_SPECIFIC_AURA:
        case SPELL_SPECIFIC_STING:
        case SPELL_SPECIFIC_CURSE:
        case SPELL_SPECIFIC_ASPECT:
        case SPELL_SPECIFIC_JUDGEMENT:
        case SPELL_SPECIFIC_WARLOCK_CORRUPTION:
        case SPELL_SPECIFIC_WARLOCK_BANE:
            return spellSpec1 == spellSpec2;
        default:
            return false;
    }
}

bool IsSingleFromSpellSpecificPerTarget(SpellSpecific spellSpec1, SpellSpecific spellSpec2)
{
    switch(spellSpec1)
    {
        case SPELL_SPECIFIC_PHASE:
        case SPELL_SPECIFIC_TRACKER:
        case SPELL_SPECIFIC_WARLOCK_ARMOR:
        case SPELL_SPECIFIC_MAGE_ARMOR:
        case SPELL_SPECIFIC_ELEMENTAL_SHIELD:
        case SPELL_SPECIFIC_MAGE_POLYMORPH:
        case SPELL_SPECIFIC_PRESENCE:
        case SPELL_SPECIFIC_CHARM:
        case SPELL_SPECIFIC_SCROLL:
        case SPELL_SPECIFIC_WARRIOR_ENRAGE:
        case SPELL_SPECIFIC_MAGE_ARCANE_BRILLANCE:
        case SPELL_SPECIFIC_PRIEST_DIVINE_SPIRIT:
            return spellSpec1 == spellSpec2;
        case SPELL_SPECIFIC_FOOD:
            return spellSpec2 == SPELL_SPECIFIC_FOOD
                || spellSpec2 == SPELL_SPECIFIC_FOOD_AND_DRINK;
        case SPELL_SPECIFIC_DRINK:
            return spellSpec2 == SPELL_SPECIFIC_DRINK
                || spellSpec2 == SPELL_SPECIFIC_FOOD_AND_DRINK;
        case SPELL_SPECIFIC_FOOD_AND_DRINK:
            return spellSpec2 == SPELL_SPECIFIC_FOOD
                || spellSpec2 == SPELL_SPECIFIC_DRINK
                || spellSpec2 == SPELL_SPECIFIC_FOOD_AND_DRINK;
        default:
            return false;
    }
}

bool IsPositiveTarget(uint32 targetA, uint32 targetB)
{
    // non-positive targets
    switch(targetA)
    {
        case TARGET_UNIT_NEARBY_ENEMY:
        case TARGET_UNIT_TARGET_ENEMY:
        case TARGET_UNIT_AREA_ENEMY_SRC:
        case TARGET_UNIT_AREA_ENEMY_DST:
        case TARGET_UNIT_CONE_ENEMY:
        case TARGET_DEST_DYNOBJ_ENEMY:
        case TARGET_DST_TARGET_ENEMY:
            return false;
        default:
            break;
    }
    if (targetB)
        return IsPositiveTarget(targetB, 0);
    return true;
}

bool SpellMgr::_isPositiveEffect(uint32 spellId, uint32 effIndex, bool deep) const
{
    SpellEntry const *spellproto = sSpellStore.LookupEntry(spellId);
    if (!spellproto) return false;

    // not found a single positive spell with this attribute
    if (spellproto->Attributes & SPELL_ATTR0_NEGATIVE_1)
        return false;

    switch (spellproto->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
            switch (spellId)
            {
                case 34700: // Allergic Reaction
                    return false;
                case 30877: // Tag Murloc
                    return true;
                default:
                    break;
            }
            break;
        case SPELLFAMILY_MAGE:
            // Amplify Magic, Dampen Magic
            if (spellproto->SpellFamilyFlags[0] == 0x00002000)
                return true;
            // Ignite    
            if (spellproto->SpellIconID == 45)
                return true;
            // Dragon's Breath
            if (spellproto->Id == 31661)
                return false;
            break;
        case SPELLFAMILY_WARRIOR:
            // Shockwave
            if (spellId == 46968)
                return false;
            break;
        case SPELLFAMILY_PRIEST:
            switch (spellId)
            {
                case 64844: // Divine Hymn
                case 64904: // Hymn of Hope
                case 47585: // Dispersion
                    return true;
                default:
                    break;
            }
            break;
        case SPELLFAMILY_HUNTER:
            // Aspect of the Viper
            if (spellId == 34074)
                return true;
            break;
        case SPELLFAMILY_ROGUE:
            // Envenom (buff is positive effect but because of the damage effect its being counted as a negative effect)
            if (spellId == 32645)
                return true;
            break;
        default:
            break;
    }

    switch (spellproto->Mechanic)
    {
        case MECHANIC_IMMUNE_SHIELD:
            return true;
        default:
            break;
    }

    // Special case: effects which determine positivity of whole spell
    for (uint8 i = 0; i<MAX_SPELL_EFFECTS; ++i)
    {
        if (spellproto->EffectApplyAuraName[i] == SPELL_AURA_MOD_STEALTH)
            return true;
    }

    switch(spellproto->Effect[effIndex])
    {
        case SPELL_EFFECT_DUMMY:
            // some explicitly required dummy effect sets
            switch(spellId)
            {
                case 28441: return false;                   // AB Effect 000
                default:
                    break;
            }
            break;
        // always positive effects (check before target checks that provided non-positive result in some case for positive effects)
        case SPELL_EFFECT_HEAL:
        case SPELL_EFFECT_LEARN_SPELL:
        case SPELL_EFFECT_SKILL_STEP:
        case SPELL_EFFECT_HEAL_PCT:
        case SPELL_EFFECT_ENERGIZE_PCT:
            return true;

            // non-positive aura use
        case SPELL_EFFECT_APPLY_AURA:
        case SPELL_EFFECT_APPLY_AREA_AURA_FRIEND:
        {
            switch(spellproto->EffectApplyAuraName[effIndex])
            {
                case SPELL_AURA_MOD_DAMAGE_DONE:            // dependent from bas point sign (negative -> negative)
                case SPELL_AURA_MOD_STAT:
                case SPELL_AURA_MOD_SKILL:
                case SPELL_AURA_MOD_DODGE_PERCENT:
                case SPELL_AURA_MOD_HEALING_PCT:
                case SPELL_AURA_MOD_HEALING_DONE:
                case SPELL_AURA_MOD_DAMAGE_PERCENT_DONE:
                    if (SpellMgr::CalculateSpellEffectAmount(spellproto, effIndex) < 0)
                        return false;
                    break;
                case SPELL_AURA_MOD_DAMAGE_TAKEN:           // dependent from bas point sign (positive -> negative)
                    if (SpellMgr::CalculateSpellEffectAmount(spellproto, effIndex) > 0)
                        return false;
                    break;
                case SPELL_AURA_MOD_CRIT_PCT:
                case SPELL_AURA_MOD_SPELL_CRIT_CHANCE:
                    if (SpellMgr::CalculateSpellEffectAmount(spellproto, effIndex) > 0)
                        return true;                        // some expected positive spells have SPELL_ATTR1_NEGATIVE
                    break;
                case SPELL_AURA_ADD_TARGET_TRIGGER:
                    return true;
                case SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
                case SPELL_AURA_PERIODIC_TRIGGER_SPELL:
                    if (!deep)
                    {
                        uint32 spellTriggeredId = spellproto->EffectTriggerSpell[effIndex];
                        SpellEntry const *spellTriggeredProto = sSpellStore.LookupEntry(spellTriggeredId);

                        if (spellTriggeredProto)
                        {
                            // non-positive targets of main spell return early
                            for (int i = 0; i < MAX_SPELL_EFFECTS; ++i)
                            {
                                if (!spellTriggeredProto->Effect[i])
                                    continue;
                                // if non-positive trigger cast targeted to positive target this main cast is non-positive
                                // this will place this spell auras as debuffs
                                if (IsPositiveTarget(spellTriggeredProto->EffectImplicitTargetA[effIndex],spellTriggeredProto->EffectImplicitTargetB[effIndex]) && !_isPositiveEffect(spellTriggeredId,i, true))
                                    return false;
                            }
                        }
                    }
                case SPELL_AURA_PROC_TRIGGER_SPELL:
                    // many positive auras have negative triggered spells at damage for example and this not make it negative (it can be canceled for example)
                    break;
                case SPELL_AURA_MOD_STUN:                   //have positive and negative spells, we can't sort its correctly at this moment.
                    if (effIndex == 0 && spellproto->Effect[1] == 0 && spellproto->Effect[2] == 0)
                        return false;                       // but all single stun aura spells is negative
                    break;
                case SPELL_AURA_MOD_PACIFY_SILENCE:
                    if (spellproto->Id == 24740)             // Wisp Costume
                        return true;
                    return false;
                case SPELL_AURA_MOD_ROOT:
                case SPELL_AURA_MOD_SILENCE:
                case SPELL_AURA_GHOST:
                case SPELL_AURA_PERIODIC_LEECH:
                case SPELL_AURA_MOD_STALKED:
                case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
                case SPELL_AURA_PREVENT_RESSURECTION:
                    return false;
                case SPELL_AURA_PERIODIC_DAMAGE:            // used in positive spells also.
                    // part of negative spell if casted at self (prevent cancel)
                    if (spellproto->EffectImplicitTargetA[effIndex] == TARGET_UNIT_CASTER)
                        return false;
                    break;
                case SPELL_AURA_MOD_DECREASE_SPEED:         // used in positive spells also
                    // part of positive spell if casted at self
                    if (spellproto->EffectImplicitTargetA[effIndex] != TARGET_UNIT_CASTER)
                        return false;
                    // but not this if this first effect (didn't find better check)
                    if (spellproto->Attributes & SPELL_ATTR0_NEGATIVE_1 && effIndex == 0)
                        return false;
                    break;
                case SPELL_AURA_MECHANIC_IMMUNITY:
                {
                    // non-positive immunities
                    switch(spellproto->EffectMiscValue[effIndex])
                    {
                        case MECHANIC_BANDAGE:
                        case MECHANIC_SHIELD:
                        case MECHANIC_MOUNT:
                        case MECHANIC_INVULNERABILITY:
                            return false;
                        default:
                            break;
                    }
                }   break;
                case SPELL_AURA_ADD_FLAT_MODIFIER:          // mods
                case SPELL_AURA_ADD_PCT_MODIFIER:
                {
                    // non-positive mods
                    switch(spellproto->EffectMiscValue[effIndex])
                    {
                        case SPELLMOD_COST:                 // dependent from bas point sign (negative -> positive)
                            if (SpellMgr::CalculateSpellEffectAmount(spellproto, effIndex) > 0)
                            {
                                if (!deep)
                                {
                                    bool negative = true;
                                    for (uint8 i=0; i<MAX_SPELL_EFFECTS; ++i)
                                    {
                                        if (i != effIndex)
                                            if (_isPositiveEffect(spellId, i, true))
                                            {
                                                negative = false;
                                                break;
                                            }
                                    }
                                    if (negative)
                                        return false;
                                }
                            }
                            break;
                        default:
                            break;
                    }
                }   break;
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }

    // non-positive targets
    if (!IsPositiveTarget(spellproto->EffectImplicitTargetA[effIndex],spellproto->EffectImplicitTargetB[effIndex]))
        return false;

    // spells which can't be reflected are negative by default
    if (spellproto->AttributesEx & SPELL_ATTR1_CANT_BE_REFLECTED)
        return false;

    if (!deep && spellproto->EffectTriggerSpell[effIndex]
        && !spellproto->EffectApplyAuraName[effIndex]
        && IsPositiveTarget(spellproto->EffectImplicitTargetA[effIndex],spellproto->EffectImplicitTargetB[effIndex])
        && !_isPositiveSpell(spellproto->EffectTriggerSpell[effIndex], true))
        return false;

    // ok, positive
    return true;
}

bool IsPositiveSpell(uint32 spellId)
{
    if (!sSpellStore.LookupEntry(spellId)) // non-existing spells
        return false;
    return !(sSpellMgr->GetSpellCustomAttr(spellId) & SPELL_ATTR0_CU_NEGATIVE);
}

bool IsPositiveEffect(uint32 spellId, uint32 effIndex)
{
    if (!sSpellStore.LookupEntry(spellId))
        return false;
    switch(effIndex)
    {
        default:
        case 0: return !(sSpellMgr->GetSpellCustomAttr(spellId) & SPELL_ATTR0_CU_NEGATIVE_EFF0);
        case 1: return !(sSpellMgr->GetSpellCustomAttr(spellId) & SPELL_ATTR0_CU_NEGATIVE_EFF1);
        case 2: return !(sSpellMgr->GetSpellCustomAttr(spellId) & SPELL_ATTR0_CU_NEGATIVE_EFF2);
    }
}

bool SpellMgr::_isPositiveSpell(uint32 spellId, bool deep) const
{
    SpellEntry const *spellproto = sSpellStore.LookupEntry(spellId);
    if (!spellproto) return false;

    // spells with at least one negative effect are considered negative
    // some self-applied spells have negative effects but in self casting case negative check ignored.
    for (int i = 0; i < MAX_SPELL_EFFECTS; ++i)
        if (!_isPositiveEffect(spellId, i, deep))
            return false;
    return true;
}

bool IsSingleTargetSpell(SpellEntry const *spellInfo)
{
    // all other single target spells have if it has AttributesEx5
    if (spellInfo->AttributesEx5 & SPELL_ATTR5_SINGLE_TARGET_SPELL)
        return true;

    switch(GetSpellSpecific(spellInfo))
    {
        case SPELL_SPECIFIC_JUDGEMENT:
            return true;
        default:
            break;
    }

    return false;
}

bool IsSingleTargetSpells(SpellEntry const *spellInfo1, SpellEntry const *spellInfo2)
{
    // TODO - need better check
    // Equal icon and spellfamily
    if (spellInfo1->SpellFamilyName == spellInfo2->SpellFamilyName &&
        spellInfo1->SpellIconID == spellInfo2->SpellIconID)
        return true;

    // TODO - need found Judgements rule
    SpellSpecific spec1 = GetSpellSpecific(spellInfo1);
    // spell with single target specific types
    switch(spec1)
    {
        case SPELL_SPECIFIC_JUDGEMENT:
        case SPELL_SPECIFIC_MAGE_POLYMORPH:
            if (GetSpellSpecific(spellInfo2) == spec1)
                return true;
            break;
        default:
            break;
    }

    return false;
}

SpellCastResult GetErrorAtShapeshiftedCast (SpellEntry const *spellInfo, uint32 form)
{
    // talents that learn spells can have stance requirements that need ignore
    // (this requirement only for client-side stance show in talent description)
    if (GetTalentSpellCost(spellInfo->Id) > 0 &&
        (spellInfo->Effect[0] == SPELL_EFFECT_LEARN_SPELL || spellInfo->Effect[1] == SPELL_EFFECT_LEARN_SPELL || spellInfo->Effect[2] == SPELL_EFFECT_LEARN_SPELL))
        return SPELL_CAST_OK;

    uint32 stanceMask = (form ? 1 << (form - 1) : 0);

    if (stanceMask & spellInfo->StancesNot)                 // can explicitly not be casted in this stance
        return SPELL_FAILED_NOT_SHAPESHIFT;

    if (stanceMask & spellInfo->Stances)                    // can explicitly be casted in this stance
        return SPELL_CAST_OK;

    bool actAsShifted = false;
    SpellShapeshiftFormEntry const *shapeInfo = NULL;
    if (form > 0)
    {
        shapeInfo = sSpellShapeshiftFormStore.LookupEntry(form);
        if (!shapeInfo)
        {
            sLog->outError("GetErrorAtShapeshiftedCast: unknown shapeshift %u", form);
            return SPELL_CAST_OK;
        }
        actAsShifted = !(shapeInfo->flags1 & 1);            // shapeshift acts as normal form for spells
    }

    if (actAsShifted)
    {
        if (spellInfo->Attributes & SPELL_ATTR0_NOT_SHAPESHIFT) // not while shapeshifted
            return SPELL_FAILED_NOT_SHAPESHIFT;
        else if (spellInfo->Stances != 0)                   // needs other shapeshift
            return SPELL_FAILED_ONLY_SHAPESHIFT;
    }
    else
    {
       /* // needs shapeshift
        if (!(spellInfo->AttributesEx2 & SPELL_ATTR2_NOT_NEED_SHAPESHIFT) && spellInfo->Stances != 0)
            return SPELL_FAILED_ONLY_SHAPESHIFT;*/
    }

    // Check if stance disables cast of not-stance spells
    // Example: cannot cast any other spells in zombie or ghoul form
    // TODO: Find a way to disable use of these spells clientside
    if (shapeInfo && shapeInfo->flags1 & 0x400)
    {
        if (!(stanceMask & spellInfo->Stances))
            return SPELL_FAILED_ONLY_SHAPESHIFT;
    }

    return SPELL_CAST_OK;
}

void SpellMgr::LoadSpellTargetPositions()
{
    mSpellTargetPositions.clear();                                // need for reload case

    uint32 count = 0;

    //                                               0   1           2                  3                  4                  5
    QueryResult result = WorldDatabase.Query("SELECT id, target_map, target_position_x, target_position_y, target_position_z, target_orientation FROM spell_target_position");
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> Loaded %u spell target coordinates", count);
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        uint32 Spell_ID = fields[0].GetUInt32();

        SpellTargetPosition st;

        st.target_mapId       = fields[1].GetUInt32();
        st.target_X           = fields[2].GetFloat();
        st.target_Y           = fields[3].GetFloat();
        st.target_Z           = fields[4].GetFloat();
        st.target_Orientation = fields[5].GetFloat();

        MapEntry const* mapEntry = sMapStore.LookupEntry(st.target_mapId);
        if (!mapEntry)
        {
            sLog->outErrorDb("Spell (ID:%u) target map (ID: %u) does not exist in `Map.dbc`.",Spell_ID,st.target_mapId);
            continue;
        }

        if (st.target_X==0 && st.target_Y==0 && st.target_Z==0)
        {
            sLog->outErrorDb("Spell (ID:%u) target coordinates not provided.",Spell_ID);
            continue;
        }

        SpellEntry const* spellInfo = sSpellStore.LookupEntry(Spell_ID);
        if (!spellInfo)
        {
            sLog->outErrorDb("Spell (ID:%u) listed in `spell_target_position` does not exist.",Spell_ID);
            continue;
        }

        bool found = false;
        for (int i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (spellInfo->EffectImplicitTargetA[i] == TARGET_DST_DB || spellInfo->EffectImplicitTargetB[i] == TARGET_DST_DB)
            {
                // additional requirements
                if (spellInfo->Effect[i]==SPELL_EFFECT_BIND && spellInfo->EffectMiscValue[i])
                {
                    uint32 area_id = sMapMgr->GetAreaId(st.target_mapId, st.target_X, st.target_Y, st.target_Z);
                    if (area_id != uint32(spellInfo->EffectMiscValue[i]))
                    {
                        sLog->outErrorDb("Spell (Id: %u) listed in `spell_target_position` expected point to zone %u bit point to zone %u.",Spell_ID, spellInfo->EffectMiscValue[i], area_id);
                        break;
                    }
                }

                found = true;
                break;
            }
        }
        if (!found)
        {
            sLog->outErrorDb("Spell (Id: %u) listed in `spell_target_position` does not have target TARGET_DST_DB (17).",Spell_ID);
            continue;
        }

        mSpellTargetPositions[Spell_ID] = st;
        ++count;

    } while (result->NextRow());

    // Check all spells
    for (uint32 i = 1; i < sSpellStore.GetNumRows(); ++i)
    {
        SpellEntry const * spellInfo = sSpellStore.LookupEntry(i);
        if (!spellInfo)
            continue;

        bool found = false;
        for (int j = 0; j < MAX_SPELL_EFFECTS; ++j)
        {
            switch(spellInfo->EffectImplicitTargetA[j])
            {
                case TARGET_DST_DB:
                    found = true;
                    break;
            }
            if (found)
                break;
            switch(spellInfo->EffectImplicitTargetB[j])
            {
                case TARGET_DST_DB:
                    found = true;
                    break;
            }
            if (found)
                break;
        }
        if (found)
        {
//            if (!sSpellMgr->GetSpellTargetPosition(i))
//                sLog->outDebug("Spell (ID: %u) does not have record in `spell_target_position`", i);
        }
    }

    sLog->outString();
    sLog->outString(">> Loaded %u spell teleport coordinates", count);
}

bool SpellMgr::IsAffectedByMod(SpellEntry const *spellInfo, SpellModifier *mod) const
{
    // false for spellInfo == NULL
    if (!spellInfo || !mod)
        return false;

    SpellEntry const *affect_spell = sSpellStore.LookupEntry(mod->spellId);
    // False if affect_spell == NULL or spellFamily not equal
    if (!affect_spell || affect_spell->SpellFamilyName != spellInfo->SpellFamilyName)
        return false;

    // true
    if (mod->mask  & spellInfo->SpellFamilyFlags)
        return true;

    return false;
}

void SpellMgr::LoadSpellProcEvents()
{
    mSpellProcEventMap.clear();                             // need for reload case

    uint32 count = 0;

    //                                               0      1           2                3                 4                 5                 6          7       8        9             10
    QueryResult result = WorldDatabase.Query("SELECT entry, SchoolMask, SpellFamilyName, SpellFamilyMask0, SpellFamilyMask1, SpellFamilyMask2, procFlags, procEx, ppmRate, CustomChance, Cooldown FROM spell_proc_event");
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> Loaded %u spell proc event conditions", count);
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        const SpellEntry *spell = sSpellStore.LookupEntry(entry);
        if (!spell)
        {
            sLog->outErrorDb("Spell %u listed in `spell_proc_event` does not exist", entry);
            continue;
        }

        SpellProcEventEntry spe;

        spe.schoolMask      = fields[1].GetUInt32();
        spe.spellFamilyName = fields[2].GetUInt32();
        spe.spellFamilyMask[0] = fields[3].GetUInt32();
        spe.spellFamilyMask[1] = fields[4].GetUInt32();
        spe.spellFamilyMask[2] = fields[5].GetUInt32();
        spe.procFlags       = fields[6].GetUInt32();
        spe.procEx          = fields[7].GetUInt32();
        spe.ppmRate         = fields[8].GetFloat();
        spe.customChance    = fields[9].GetFloat();
        spe.cooldown        = fields[10].GetUInt32();

        mSpellProcEventMap[entry] = spe;

        ++count;
    } while (result->NextRow());

    sLog->outString();
    sLog->outString(">> Loaded %u extra spell proc event conditions", count);
}

void SpellMgr::LoadSpellBonusess()
{
    mSpellBonusMap.clear();                             // need for reload case
    uint32 count = 0;
    //                                               0      1             2          3         4
    QueryResult result = WorldDatabase.Query("SELECT entry, direct_bonus, dot_bonus, ap_bonus, ap_dot_bonus FROM spell_bonus_data");
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> Loaded %u spell bonus data", count);
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        const SpellEntry *spell = sSpellStore.LookupEntry(entry);
        if (!spell)
        {
            sLog->outErrorDb("Spell %u listed in `spell_bonus_data` does not exist", entry);
            continue;
        }

        SpellBonusEntry& sbe = mSpellBonusMap[entry];
        sbe.direct_damage = fields[1].GetFloat();
        sbe.dot_damage    = fields[2].GetFloat();
        sbe.ap_bonus      = fields[3].GetFloat();
        sbe.ap_dot_bonus   = fields[4].GetFloat();

        // compare with DBC coefficient
        float DBCcoef = 0;
        for(uint32 i=0;i<3;i++)
        {
            if(spell->EffectBonusCoefficient[i] == sbe.direct_damage ||
                sbe.direct_damage < 0)
            {
                break;
            }

            if(DBCcoef == 0)
            {
                DBCcoef = spell->EffectBonusCoefficient[i];
            }
        }
        if(DBCcoef > 0)
        {
            sLog->outError("SPC: DB SP coef doesn't match DBC. ID: %d, DB: %f, DBC: %f, Name: %s", entry, sbe.direct_damage, DBCcoef, spell->SpellName);
        }

        ++count;
    } while (result->NextRow());

    sLog->outString();
    sLog->outString(">> Loaded %u extra spell bonus data",  count);
}

bool SpellMgr::IsSpellProcEventCanTriggeredBy(SpellProcEventEntry const* spellProcEvent, uint32 EventProcFlag, SpellEntry const * procSpell, uint32 procFlags, uint32 procExtra, bool active)
{
    // No extra req need
    uint32 procEvent_procEx = PROC_EX_NONE;

    // check prockFlags for condition
    if ((procFlags & EventProcFlag) == 0)
        return false;

    bool hasFamilyMask = false;

    /* Check Periodic Auras

    *Dots can trigger if spell has no PROC_FLAG_SUCCESSFUL_NEGATIVE_MAGIC_SPELL
        nor PROC_FLAG_TAKEN_POSITIVE_MAGIC_SPELL

    *Only Hots can trigger if spell has PROC_FLAG_TAKEN_POSITIVE_MAGIC_SPELL

    *Only dots can trigger if spell has both positivity flags or PROC_FLAG_SUCCESSFUL_NEGATIVE_MAGIC_SPELL

    *Aura has to have PROC_FLAG_TAKEN_POSITIVE_MAGIC_SPELL or spellfamily specified to trigger from Hot

    */

    if (procFlags & PROC_FLAG_DONE_PERIODIC)
    {
        if (EventProcFlag & PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG)
        {
            if (!(procExtra & PROC_EX_INTERNAL_DOT))
                return false;
        }
        else if (procExtra & PROC_EX_INTERNAL_HOT)
            procExtra |= PROC_EX_INTERNAL_REQ_FAMILY;
        else if (EventProcFlag & PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_POS)
            return false;
    }

    if (procFlags & PROC_FLAG_TAKEN_PERIODIC)
    {
        if (EventProcFlag & PROC_FLAG_TAKEN_SPELL_MAGIC_DMG_CLASS_POS)
        {
            if (!(procExtra & PROC_EX_INTERNAL_DOT))
                return false;
        }
        else if (procExtra & PROC_EX_INTERNAL_HOT)
            procExtra |= PROC_EX_INTERNAL_REQ_FAMILY;
        else if (EventProcFlag & PROC_FLAG_TAKEN_SPELL_NONE_DMG_CLASS_POS)
            return false;
    }
    // Trap casts are active by default
    if (procFlags & PROC_FLAG_DONE_TRAP_ACTIVATION)
        active = true;

    // Always trigger for this
    if (procFlags & (PROC_FLAG_KILLED | PROC_FLAG_KILL | PROC_FLAG_DEATH))
        return true;

    if (spellProcEvent)     // Exist event data
    {
        // Store extra req
        procEvent_procEx = spellProcEvent->procEx;

        // For melee triggers
        if (procSpell == NULL)
        {
            // Check (if set) for school (melee attack have Normal school)
            if (spellProcEvent->schoolMask && (spellProcEvent->schoolMask & SPELL_SCHOOL_MASK_NORMAL) == 0)
                return false;
        }
        else // For spells need check school/spell family/family mask
        {
            // Check (if set) for school
            if (spellProcEvent->schoolMask && (spellProcEvent->schoolMask & procSpell->SchoolMask) == 0)
                return false;

            // Check (if set) for spellFamilyName
            if (spellProcEvent->spellFamilyName && (spellProcEvent->spellFamilyName != procSpell->SpellFamilyName))
                return false;

            // spellFamilyName is Ok need check for spellFamilyMask if present
            if (spellProcEvent->spellFamilyMask)
            {
                if ((spellProcEvent->spellFamilyMask & procSpell->SpellFamilyFlags) == 0)
                    return false;
                hasFamilyMask = true;
                // Some spells are not considered as active even with have spellfamilyflags
                if (!(procEvent_procEx & PROC_EX_ONLY_ACTIVE_SPELL))
                    active = true;
            }
        }
    }

    if (procExtra & (PROC_EX_INTERNAL_REQ_FAMILY))
    {
        if (!hasFamilyMask)
            return false;
    }

    // Check for extra req (if none) and hit/crit
    if (procEvent_procEx == PROC_EX_NONE)
    {
        // No extra req, so can trigger only for hit/crit - spell has to be active
        if ((procExtra & (PROC_EX_NORMAL_HIT|PROC_EX_CRITICAL_HIT)) && active)
            return true;
    }
    else // Passive spells hits here only if resist/reflect/immune/evade
    {
        if (procExtra & AURA_SPELL_PROC_EX_MASK)
        {
            // if spell marked as procing only from not active spells
            if (active && procEvent_procEx & PROC_EX_NOT_ACTIVE_SPELL)
                return false;
            // if spell marked as procing only from active spells
            if (!active && procEvent_procEx & PROC_EX_ONLY_ACTIVE_SPELL)
                return false;
            // Exist req for PROC_EX_EX_TRIGGER_ALWAYS
            if (procEvent_procEx & PROC_EX_EX_TRIGGER_ALWAYS)
                return true;
            // PROC_EX_NOT_ACTIVE_SPELL and PROC_EX_ONLY_ACTIVE_SPELL flags handle: if passed checks before
            if ((procExtra & (PROC_EX_NORMAL_HIT|PROC_EX_CRITICAL_HIT)) && ((procEvent_procEx & (AURA_SPELL_PROC_EX_MASK)) == 0))
                return true;
        }
        // Check Extra Requirement like (hit/crit/miss/resist/parry/dodge/block/immune/reflect/absorb and other)
        if (procEvent_procEx & procExtra)
            return true;
    }
    return false;
}

void SpellMgr::LoadSpellGroups()
{
    mSpellSpellGroup.clear();                                  // need for reload case
    mSpellGroupSpell.clear();

    uint32 count = 0;

    //                                               0   1
    QueryResult result = WorldDatabase.Query("SELECT id, spell_id FROM spell_group");
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> Loaded %u spell group definitions", count);
        return;
    }

    std::set<uint32> groups;

    do
    {
        Field *fields = result->Fetch();

        uint32 group_id = fields[0].GetUInt32();
        if (group_id <= SPELL_GROUP_DB_RANGE_MIN && group_id >= SPELL_GROUP_CORE_RANGE_MAX)
        {
            sLog->outErrorDb("SpellGroup id %u listed in `spell_groups` is in core range, but is not defined in core!", group_id);
            continue;
        }
        int32 spell_id = fields[1].GetInt32();

        groups.insert(std::set<uint32>::value_type(group_id));
        mSpellGroupSpell.insert(SpellGroupSpellMap::value_type((SpellGroup)group_id, spell_id));

    } while (result->NextRow());

    for (SpellGroupSpellMap::iterator itr = mSpellGroupSpell.begin(); itr!= mSpellGroupSpell.end() ;)
    {
        if (itr->second < 0)
        {
            if (groups.find(abs(itr->second)) == groups.end())
            {
                sLog->outErrorDb("SpellGroup id %u listed in `spell_groups` does not exist", abs(itr->second));
                mSpellGroupSpell.erase(itr++);
            }
            else
                ++itr;
        }
        else
        {
            SpellEntry const* spellInfo = sSpellStore.LookupEntry(itr->second);

            if (!spellInfo)
            {
                sLog->outErrorDb("Spell %u listed in `spell_group` does not exist", itr->second);
                mSpellGroupSpell.erase(itr++);
            }
            else if (GetSpellRank(itr->second) > 1)
            {
                sLog->outErrorDb("Spell %u listed in `spell_group` is not first rank of spell", itr->second);
                mSpellGroupSpell.erase(itr++);
            }
            else
                ++itr;
        }
    }

    for (std::set<uint32>::iterator groupItr = groups.begin() ; groupItr != groups.end() ; ++groupItr)
    {
        std::set<uint32> spells;
        GetSetOfSpellsInSpellGroup(SpellGroup(*groupItr), spells);

        for (std::set<uint32>::iterator spellItr = spells.begin() ; spellItr != spells.end() ; ++spellItr)
        {
            ++count;
            mSpellSpellGroup.insert(SpellSpellGroupMap::value_type(*spellItr, SpellGroup(*groupItr)));
        }
    }

    sLog->outString();
    sLog->outString(">> Loaded %u spell group definitions", count);
}

void SpellMgr::LoadSpellGroupStackRules()
{
    mSpellGroupStack.clear();                                  // need for reload case

    uint32 count = 0;

    //                                               0         1
    QueryResult result = WorldDatabase.Query("SELECT group_id, stack_rule FROM spell_group_stack_rules");
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> Loaded %u spell group stack rules", count);
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        uint32 group_id = fields[0].GetUInt32();
        uint8 stack_rule = fields[1].GetUInt32();
        if (stack_rule >= SPELL_GROUP_STACK_RULE_MAX)
        {
            sLog->outErrorDb("SpellGroupStackRule %u listed in `spell_group_stack_rules` does not exist", stack_rule);
            continue;
        }

        SpellGroupSpellMapBounds spellGroup = GetSpellGroupSpellMapBounds((SpellGroup)group_id);

        if (spellGroup.first == spellGroup.second)
        {
            sLog->outErrorDb("SpellGroup id %u listed in `spell_group_stack_rules` does not exist", group_id);
            continue;
        }

        mSpellGroupStack[(SpellGroup)group_id] = (SpellGroupStackRule)stack_rule;

        ++count;
    } while (result->NextRow());

    sLog->outString();
    sLog->outString(">> Loaded %u spell group stack rules", count);
}

void SpellMgr::LoadSpellThreats()
{
    mSpellThreatMap.clear();                                // need for reload case

    uint32 count = 0;

    //                                               0      1
    QueryResult result = WorldDatabase.Query("SELECT entry, Threat FROM spell_threat");
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> Loaded %u aggro generating spells", count);
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();
        uint16 Threat = fields[1].GetUInt16();

        if (!sSpellStore.LookupEntry(entry))
        {
            sLog->outErrorDb("Spell %u listed in `spell_threat` does not exist", entry);
            continue;
        }

        mSpellThreatMap[entry] = Threat;

        ++count;
    } while (result->NextRow());

    sLog->outString();
    sLog->outString(">> Loaded %u aggro generating spells", count);
}

bool SpellMgr::IsRankSpellDueToSpell(SpellEntry const *spellInfo_1,uint32 spellId_2) const
{
    SpellEntry const *spellInfo_2 = sSpellStore.LookupEntry(spellId_2);
    if (!spellInfo_1 || !spellInfo_2) return false;
    if (spellInfo_1->Id == spellId_2) return false;

    return GetFirstSpellInChain(spellInfo_1->Id) == GetFirstSpellInChain(spellId_2);
}

bool SpellMgr::canStackSpellRanks(SpellEntry const *spellInfo)
{
    if (IsPassiveSpell(spellInfo->Id))                       // ranked passive spell
        return false;
    if (spellInfo->powerType != POWER_MANA && spellInfo->powerType != POWER_HEALTH)
        return false;
    if (IsProfessionOrRidingSpell(spellInfo->Id))
        return false;

    if (sSpellMgr->IsSkillBonusSpell(spellInfo->Id))
        return false;

    // All stance spells. if any better way, change it.
    for (int i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        switch(spellInfo->SpellFamilyName)
        {
            case SPELLFAMILY_PALADIN:
                // Paladin aura Spell
                if (spellInfo->Effect[i] == SPELL_EFFECT_APPLY_AREA_AURA_RAID)
                    return false;
                break;
            case SPELLFAMILY_DRUID:
                // Druid form Spell
                if (spellInfo->Effect[i] == SPELL_EFFECT_APPLY_AURA &&
                    spellInfo->EffectApplyAuraName[i] == SPELL_AURA_MOD_SHAPESHIFT)
                    return false;
                break;
            case SPELLFAMILY_ROGUE:
                // Rogue Stealth
                if (spellInfo->Effect[i] == SPELL_EFFECT_APPLY_AURA &&
                    spellInfo->EffectApplyAuraName[i] == SPELL_AURA_MOD_SHAPESHIFT)
                    return false;
        }
    }
    return true;
}

bool SpellMgr::IsProfessionOrRidingSpell(uint32 spellId)
{
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo)
        return false;

    for (uint8 i = 0 ; i < MAX_SPELL_EFFECTS ; ++i)
    {
        if (spellInfo->Effect[i] == SPELL_EFFECT_SKILL)
        {
            uint32 skill = spellInfo->EffectMiscValue[i];

            bool found = IsProfessionOrRidingSkill(skill);
            if (found)
                return true;
        }
    }
    return false;
}

bool SpellMgr::IsProfessionSpell(uint32 spellId)
{
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo)
        return false;

    for (uint8 i = 0 ; i < MAX_SPELL_EFFECTS ; ++i)
    {
        if (spellInfo->Effect[i] == SPELL_EFFECT_SKILL)
        {
            uint32 skill = spellInfo->EffectMiscValue[i];

            bool found = IsProfessionSkill(skill);
            if (found)
                return true;
        }
    }
    return false;
}

bool SpellMgr::IsPrimaryProfessionSpell(uint32 spellId)
{
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo)
        return false;

    for (uint8 i = 0 ; i < MAX_SPELL_EFFECTS ; ++i)
    {
        if (spellInfo->Effect[i] == SPELL_EFFECT_SKILL)
        {
            uint32 skill = spellInfo->EffectMiscValue[i];

            bool found = IsPrimaryProfessionSkill(skill);
            if (found)
                return true;
        }
    }
    return false;
}

bool SpellMgr::IsPrimaryProfessionFirstRankSpell(uint32 spellId) const
{
    return IsPrimaryProfessionSpell(spellId) && GetSpellRank(spellId) == 1;
}

bool SpellMgr::IsSkillBonusSpell(uint32 spellId) const
{
    SkillLineAbilityMapBounds bounds = GetSkillLineAbilityMapBounds(spellId);

    for (SkillLineAbilityMap::const_iterator _spell_idx = bounds.first; _spell_idx != bounds.second; ++_spell_idx)
    {
        SkillLineAbilityEntry const *pAbility = _spell_idx->second;
        if (!pAbility || pAbility->learnOnGetSkill != ABILITY_LEARNED_ON_GET_PROFESSION_SKILL)
            continue;

        if (pAbility->req_skill_value > 0)
            return true;
    }

    return false;
}

bool SpellMgr::IsSkillTypeSpell(uint32 spellId, SkillType type) const
{
    SkillLineAbilityMapBounds bounds = GetSkillLineAbilityMapBounds(spellId);

    for (SkillLineAbilityMap::const_iterator _spell_idx = bounds.first; _spell_idx != bounds.second; ++_spell_idx)
        if (_spell_idx->second->skillId == uint32(type))
            return true;

    return false;
}

// basepoints provided here have to be valid basepoints (use SpellMgr::CalculateSpellEffectBaseAmount)
int32 SpellMgr::CalculateSpellEffectAmount(SpellEntry const * spellEntry, uint8 effIndex, Unit const * caster, int32 const * effBasePoints, Unit const * target)
{
    float basePointsPerLevel = spellEntry->EffectRealPointsPerLevel[effIndex];
    int32 basePoints = effBasePoints ? *effBasePoints : spellEntry->EffectBasePoints[effIndex];
    int32 randomPoints = int32(spellEntry->EffectDieSides[effIndex]);

    float maxPoints = 0.00f;
    float comboPointScaling = 0.00f;
    if (caster)
    {
        uint8 level = caster->getLevel();

        // There are several spells, such as buffs, which should scale by target level, not caster
        // Unfortunately, we haven't found any generic way to implement that
        if (target
            && (spellEntry->Id == 79104    // Power Word: Fortitude
                || spellEntry->Id == 79105 // Power Word: Fortitude - raid-wide
                || spellEntry->Id == 79057 // Arcane Brillance
                || spellEntry->Id == 79058 // Arcane Brillance - raid-wide
                || spellEntry->Id == 79060 // Mark of the Wild
                || spellEntry->Id == 79061 // Mark of the Wild - raid-wide
                || spellEntry->Id == 79101 // Blessing of Might
                || spellEntry->Id == 79102 // Blessing of Might - raid-wide
                || spellEntry->Id == 79062 // Blessing of Kings
                || spellEntry->Id == 79063 // Blessing of Kings - raid-wide
                )
            )
            level = target->getLevel();

        SpellScaling values;
        values.Init(level, spellEntry);
        if (values.canScale && (int32)values.min[effIndex] != 0)
        {
            basePoints = (int32)values.min[effIndex];
            maxPoints = values.max[effIndex];
            comboPointScaling = values.pts[effIndex];
        }
        else
        {
            int32 level = int32(caster->getLevel());
            if (level > int32(spellEntry->maxLevel) && spellEntry->maxLevel > 0)
                level = int32(spellEntry->maxLevel);
            else if (level < int32(spellEntry->baseLevel))
                level = int32(spellEntry->baseLevel);
            level -= int32(spellEntry->spellLevel);
            basePoints += int32(level * basePointsPerLevel);
        }

        // Mixology - amount boost
        if (caster && target && caster->GetTypeId() == TYPEID_PLAYER)
        {
            if (spellEntry->SpellFamilyName == SPELLFAMILY_POTION && (
                sSpellMgr->IsSpellMemberOfSpellGroup(spellEntry->Id, SPELL_GROUP_ELIXIR_BATTLE) ||
                sSpellMgr->IsSpellMemberOfSpellGroup(spellEntry->Id, SPELL_GROUP_ELIXIR_GUARDIAN) ||
                sSpellMgr->IsSpellMemberOfSpellGroup(spellEntry->Id, SPELL_GROUP_FLASK)))
            {
                if (caster->HasAura(53042) && caster->HasSpell(spellEntry->EffectTriggerSpell[0]))
                    basePoints *= 1.266667;
            }
        }
    }

    if (maxPoints != 0.00f)
        basePoints = irand(basePoints, (int32)maxPoints);
    else
    {
        // not sure for Cataclysm.
        // roll in a range <1;EffectDieSides> as of patch 3.3.3
        switch(randomPoints)
        {
            case 0: break;
            case 1: basePoints += 1; break;                     // range 1..1
            default:
                // range can have positive (1..rand) and negative (rand..1) values, so order its for irand
                int32 randvalue = (randomPoints >= 1)
                    ? irand(1, randomPoints)
                    : irand(randomPoints, 1);

                basePoints += randvalue;
                break;
        }
    }
    int32 value = basePoints;

    // random damage
    if (caster)
    {
        Player* pl = caster->GetCharmerOrOwnerPlayerOrPlayerItself();
        if (pl)
        {
            // Pet abilities, probably hack, but who knows
            switch (spellEntry->Id)
            {
                case 17253: // Bite
                case 16827: // Claw
                case 49966: // Smack
                {
                    uint32 rap = pl->GetTotalAttackPowerValue(RANGED_ATTACK);
                    value += rap*0.4f*0.2f;
                    break;
                }
                case 53508: // Wolverine Bite
                {
                    uint32 rap = pl->GetTotalAttackPowerValue(RANGED_ATTACK);
                    value += rap*0.4f*0.1f;
                    break;
                }
                case 90361: // Spirit Mend
                {
                    uint32 rap = pl->GetTotalAttackPowerValue(RANGED_ATTACK);
                    if (effIndex == EFFECT_0)
                        value += rap*0.35f*0.5f;
                    else if (effIndex == EFFECT_1)
                        value += rap*0.35f*0.335f;
                    break;
                }
            }
        }


        // bonus amount from combo points
        if (caster->m_movedPlayer)
        {
            if (uint8 comboPoints = caster->m_movedPlayer->GetComboPoints())
            {
                if (float comboDamage = spellEntry->EffectPointsPerComboPoint[effIndex])
                {
                    if (comboPointScaling != 0.00f)
                        comboDamage = comboPointScaling;

                    value += int32(comboDamage * comboPoints);
                }
            }
        }

        value = caster->ApplyEffectModifiers(spellEntry, effIndex, value);

        // amount multiplication based on caster's level
        if (!basePointsPerLevel && (spellEntry->Attributes & SPELL_ATTR0_LEVEL_DAMAGE_CALCULATION && spellEntry->spellLevel) &&
            spellEntry->Effect[effIndex] != SPELL_EFFECT_WEAPON_PERCENT_DAMAGE &&
            spellEntry->Effect[effIndex] != SPELL_EFFECT_KNOCK_BACK &&
            spellEntry->EffectApplyAuraName[effIndex] != SPELL_AURA_MOD_SPEED_ALWAYS &&
            spellEntry->EffectApplyAuraName[effIndex] != SPELL_AURA_MOD_SPEED_NOT_STACK &&
            spellEntry->EffectApplyAuraName[effIndex] != SPELL_AURA_MOD_INCREASE_SPEED &&
            spellEntry->EffectApplyAuraName[effIndex] != SPELL_AURA_MOD_DECREASE_SPEED &&
            spellEntry->EffectApplyAuraName[effIndex] != SPELL_AURA_MOD_MELEE_SPEED_PCT &&
            spellEntry->EffectApplyAuraName[effIndex] != SPELL_AURA_MOD_RANGED_SPEED_PCT)
            //there are many more: slow speed, -healing pct
            value = int32(value*0.25f*exp(caster->getLevel()*(70 - spellEntry->spellLevel) / 1000.0f));
        //value = int32(value * (int32)getLevel() / (int32)(spellProto->spellLevel ? spellProto->spellLevel : 1));

    }
    // Skinning, step for 525 maxskill
    if (spellEntry->Id == 74522)
        value = 7;

    return value;
}

int32 SpellMgr::CalculateSpellEffectBaseAmount(int32 value, SpellEntry const * spellEntry, uint8 effIndex)
{
    if (spellEntry->EffectDieSides[effIndex] == 0)
        return value;
    else
        return value - 1;
}

float SpellMgr::CalculateSpellEffectValueMultiplier(SpellEntry const * spellEntry, uint8 effIndex, Unit * caster, Spell * spell)
{
    float multiplier = spellEntry->EffectValueMultiplier[effIndex];

    if (caster)
        if (Player * modOwner = caster->GetSpellModOwner())
            modOwner->ApplySpellMod(spellEntry->Id, SPELLMOD_VALUE_MULTIPLIER, multiplier, spell);
    return multiplier;
}

float SpellMgr::CalculateSpellEffectDamageMultiplier(SpellEntry const * spellEntry, uint8 effIndex, Unit * caster, Spell * spell)
{
    float multiplier = spellEntry->EffectDamageMultiplier[effIndex];

    if (caster)
        if (Player * modOwner = caster->GetSpellModOwner())
            modOwner->ApplySpellMod(spellEntry->Id, SPELLMOD_DAMAGE_MULTIPLIER, multiplier, spell);
    return multiplier;
}

SpellEntry const* SpellMgr::SelectAuraRankForPlayerLevel(SpellEntry const* spellInfo, uint32 playerLevel) const
{
    // ignore passive spells
    if (IsPassiveSpell(spellInfo->Id))
        return spellInfo;

    bool needRankSelection = false;
    for (int i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (IsPositiveEffect(spellInfo->Id, i) && (
            spellInfo->Effect[i] == SPELL_EFFECT_APPLY_AURA ||
            spellInfo->Effect[i] == SPELL_EFFECT_APPLY_AREA_AURA_PARTY ||
            spellInfo->Effect[i] == SPELL_EFFECT_APPLY_AREA_AURA_RAID))
        {
            needRankSelection = true;
            break;
        }
    }

    // not required
    if (!needRankSelection)
        return spellInfo;

    for (uint32 nextSpellId = spellInfo->Id; nextSpellId != 0; nextSpellId = GetPrevSpellInChain(nextSpellId))
    {
        SpellEntry const *nextSpellInfo = sSpellStore.LookupEntry(nextSpellId);
        if (!nextSpellInfo)
            break;

        // if found appropriate level
        if (playerLevel + 10 >= nextSpellInfo->spellLevel)
            return nextSpellInfo;

        // one rank less then
    }

    // not found
    return NULL;
}

void SpellMgr::LoadSpellLearnSkills()
{
    mSpellLearnSkills.clear();                              // need for reload case

    // search auto-learned skills and add its to map also for use in unlearn spells/talents
    uint32 dbc_count = 0;
    
    for (uint32 spell = 0; spell < sSpellStore.GetNumRows(); ++spell)
    {
        
        SpellEntry const* entry = sSpellStore.LookupEntry(spell);

        if (!entry)
            continue;

        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (entry->Effect[i] == SPELL_EFFECT_SKILL)
            {
                SpellLearnSkillNode dbc_node;
                dbc_node.skill = entry->EffectMiscValue[i];
                dbc_node.step  = SpellMgr::CalculateSpellEffectAmount(entry, i);
                if (dbc_node.skill != SKILL_RIDING)
                    dbc_node.value = 1;
                else
                    dbc_node.value = dbc_node.step * 75;
                dbc_node.maxvalue = dbc_node.step * 75;
                mSpellLearnSkills[spell] = dbc_node;
                ++dbc_count;
                break;
            }
        }
    }

    sLog->outString();
    sLog->outString(">> Loaded %u Spell Learn Skills from DBC", dbc_count);
}

void SpellMgr::LoadSpellLearnSpells()
{
    mSpellLearnSpells.clear();                              // need for reload case

    //                                               0      1        2
    QueryResult result = WorldDatabase.Query("SELECT entry, SpellID, Active FROM spell_learn_spell");
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> Loaded 0 spell learn spells");
        sLog->outErrorDb("`spell_learn_spell` table is empty!");
        return;
    }

    uint32 count = 0;

    do
    {
        Field *fields = result->Fetch();

        uint32 spell_id = fields[0].GetUInt32();

        SpellLearnSpellNode node;
        node.spell      = fields[1].GetUInt32();
        node.active     = fields[2].GetBool();
        node.autoLearned= false;

        if (!sSpellStore.LookupEntry(spell_id))
        {
            sLog->outErrorDb("Spell %u listed in `spell_learn_spell` does not exist", spell_id);
            continue;
        }

        if (!sSpellStore.LookupEntry(node.spell))
        {
            sLog->outErrorDb("Spell %u listed in `spell_learn_spell` learning not existed spell %u", spell_id, node.spell);
            continue;
        }

        if (GetTalentSpellCost(node.spell))
        {
            sLog->outErrorDb("Spell %u listed in `spell_learn_spell` attempt learning talent spell %u, skipped", spell_id, node.spell);
            continue;
        }

        mSpellLearnSpells.insert(SpellLearnSpellMap::value_type(spell_id,node));

        ++count;
    } while (result->NextRow());

    // search auto-learned spells and add its to map also for use in unlearn spells/talents
    uint32 dbc_count = 0;
    for (uint32 spell = 0; spell < sSpellStore.GetNumRows(); ++spell)
    {
        SpellEntry const* entry = sSpellStore.LookupEntry(spell);

        if (!entry)
            continue;

        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (entry->Effect[i] == SPELL_EFFECT_LEARN_SPELL)
            {
                SpellLearnSpellNode dbc_node;
                dbc_node.spell = entry->EffectTriggerSpell[i];
                dbc_node.active = true;                     // all dbc based learned spells is active (show in spell book or hide by client itself)

                // ignore learning not existed spells (broken/outdated/or generic learnig spell 483
                if (!sSpellStore.LookupEntry(dbc_node.spell))
                    continue;

                // talent or passive spells or skill-step spells auto-casted and not need dependent learning,
                // pet teaching spells don't must be dependent learning (casted)
                // other required explicit dependent learning
                dbc_node.autoLearned = entry->EffectImplicitTargetA[i] == TARGET_UNIT_PET || GetTalentSpellCost(spell) > 0 || IsPassiveSpell(spell) || IsSpellHaveEffect(entry,SPELL_EFFECT_SKILL_STEP);

                SpellLearnSpellMapBounds db_node_bounds = GetSpellLearnSpellMapBounds(spell);

                bool found = false;
                for (SpellLearnSpellMap::const_iterator itr = db_node_bounds.first; itr != db_node_bounds.second; ++itr)
                {
                    if (itr->second.spell == dbc_node.spell)
                    {
                        sLog->outErrorDb("Spell %u auto-learn spell %u in spell.dbc then the record in `spell_learn_spell` is redundant, please fix DB.",
                            spell,dbc_node.spell);
                        found = true;
                        break;
                    }
                }

                if (!found)                                  // add new spell-spell pair if not found
                {
                    mSpellLearnSpells.insert(SpellLearnSpellMap::value_type(spell,dbc_node));
                    ++dbc_count;
                }
            }
        }
    }

    sLog->outString();
    sLog->outString(">> Loaded %u spell learn spells + %u found in DBC", count, dbc_count);
}

void SpellMgr::LoadSpellPetAuras()
{
    mSpellPetAuraMap.clear();                                  // need for reload case

    uint32 count = 0;

    //                                                  0       1       2    3
    QueryResult result = WorldDatabase.Query("SELECT spell, effectId, pet, aura FROM spell_pet_auras");
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> Loaded %u spell pet auras", count);
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        uint32 spell = fields[0].GetUInt32();
        uint8 eff = fields[1].GetUInt8();
        uint32 pet = fields[2].GetUInt32();
        uint32 aura = fields[3].GetUInt32();

        SpellPetAuraMap::iterator itr = mSpellPetAuraMap.find((spell<<8) + eff);
        if (itr != mSpellPetAuraMap.end())
            itr->second.AddAura(pet, aura);
        else
        {
            SpellEntry const* spellInfo = sSpellStore.LookupEntry(spell);
            if (!spellInfo)
            {
                sLog->outErrorDb("Spell %u listed in `spell_pet_auras` does not exist", spell);
                continue;
            }
            if (spellInfo->Effect[eff] != SPELL_EFFECT_DUMMY &&
               (spellInfo->Effect[eff] != SPELL_EFFECT_APPLY_AURA ||
                spellInfo->EffectApplyAuraName[eff] != SPELL_AURA_DUMMY))
            {
                sLog->outError("Spell %u listed in `spell_pet_auras` does not have dummy aura or dummy effect", spell);
                continue;
            }

            SpellEntry const* spellInfo2 = sSpellStore.LookupEntry(aura);
            if (!spellInfo2)
            {
                sLog->outErrorDb("Aura %u listed in `spell_pet_auras` does not exist", aura);
                continue;
            }

            PetAura pa(pet, aura, spellInfo->EffectImplicitTargetA[eff] == TARGET_UNIT_PET, SpellMgr::CalculateSpellEffectAmount(spellInfo, eff));
            mSpellPetAuraMap[(spell<<8) + eff] = pa;
        }

        ++count;
    } while (result->NextRow());

    sLog->outString();
    sLog->outString(">> Loaded %u spell pet auras", count);
}

void SpellMgr::LoadPetLevelupSpellMap()
{
    mPetLevelupSpellMap.clear();                                   // need for reload case

    uint32 count = 0;
    uint32 family_count = 0;

    for (uint32 i = 0; i < sCreatureFamilyStore.GetNumRows(); ++i)
    {
        CreatureFamilyEntry const *creatureFamily = sCreatureFamilyStore.LookupEntry(i);
        if (!creatureFamily)                                     // not exist
            continue;

        for (uint8 j = 0; j < 2; ++j)
        {
            if (!creatureFamily->skillLine[j])
                continue;

            for (uint32 k = 0; k < sSkillLineAbilityStore.GetNumRows(); ++k)
            {
                SkillLineAbilityEntry const *skillLine = sSkillLineAbilityStore.LookupEntry(k);
                if (!skillLine)
                    continue;

                //if (skillLine->skillId != creatureFamily->skillLine[0] &&
                //    (!creatureFamily->skillLine[1] || skillLine->skillId != creatureFamily->skillLine[1]))
                //    continue;

                if (skillLine->skillId != creatureFamily->skillLine[j])
                    continue;

                if (skillLine->learnOnGetSkill != ABILITY_LEARNED_ON_GET_RACE_OR_CLASS_SKILL)
                    continue;

                SpellEntry const *spell = sSpellStore.LookupEntry(skillLine->spellId);
                if (!spell) // not exist or triggered or talent
                    continue;

                if (!spell->spellLevel)
                    continue;

                PetLevelupSpellSet& spellSet = mPetLevelupSpellMap[creatureFamily->ID];
                if (spellSet.empty())
                    ++family_count;

                spellSet.insert(PetLevelupSpellSet::value_type(spell->spellLevel,spell->Id));
                ++count;
            }
        }
    }

    sLog->outString();
    sLog->outString(">> Loaded %u pet levelup and default spells for %u families", count, family_count);
}

bool LoadPetDefaultSpells_helper(CreatureInfo const* cInfo, PetDefaultSpellsEntry& petDefSpells)
{
    // skip empty list;
    bool have_spell = false;
    for (uint8 j = 0; j < MAX_CREATURE_SPELL_DATA_SLOT; ++j)
    {
        if (petDefSpells.spellid[j])
        {
            have_spell = true;
            break;
        }
    }
    if (!have_spell)
        return false;

    // remove duplicates with levelupSpells if any
    if (PetLevelupSpellSet const *levelupSpells = cInfo->family ? sSpellMgr->GetPetLevelupSpellList(cInfo->family) : NULL)
    {
        for (uint8 j = 0; j < MAX_CREATURE_SPELL_DATA_SLOT; ++j)
        {
            if (!petDefSpells.spellid[j])
                continue;

            for (PetLevelupSpellSet::const_iterator itr = levelupSpells->begin(); itr != levelupSpells->end(); ++itr)
            {
                if (itr->second == petDefSpells.spellid[j])
                {
                    petDefSpells.spellid[j] = 0;
                    break;
                }
            }
        }
    }

    // skip empty list;
    have_spell = false;
    for (uint8 j = 0; j < MAX_CREATURE_SPELL_DATA_SLOT; ++j)
    {
        if (petDefSpells.spellid[j])
        {
            have_spell = true;
            break;
        }
    }

    return have_spell;
}

void SpellMgr::LoadPetDefaultSpells()
{
    mPetDefaultSpellsMap.clear();

    uint32 countCreature = 0;
    uint32 countData = 0;

    for (uint32 i = 0; i < sCreatureStorage.MaxEntry; ++i)
    {
        CreatureInfo const* cInfo = sCreatureStorage.LookupEntry<CreatureInfo>(i);
        if (!cInfo)
            continue;

        if (!cInfo->PetSpellDataId)
            continue;

        // for creature with PetSpellDataId get default pet spells from dbc
        CreatureSpellDataEntry const* spellDataEntry = sCreatureSpellDataStore.LookupEntry(cInfo->PetSpellDataId);
        if (!spellDataEntry)
            continue;

        int32 petSpellsId = -int32(cInfo->PetSpellDataId);
        PetDefaultSpellsEntry petDefSpells;
        for (uint8 j = 0; j < MAX_CREATURE_SPELL_DATA_SLOT; ++j)
            petDefSpells.spellid[j] = spellDataEntry->spellId[j];

        if (LoadPetDefaultSpells_helper(cInfo, petDefSpells))
        {
            mPetDefaultSpellsMap[petSpellsId] = petDefSpells;
            ++countData;
        }
    }

    // different summon spells
    for (uint32 i = 0; i < sSpellStore.GetNumRows(); ++i)
    {
        SpellEntry const* spellEntry = sSpellStore.LookupEntry(i);
        if (!spellEntry)
            continue;

        for (uint8 k = 0; k < MAX_SPELL_EFFECTS; ++k)
        {
            if (spellEntry->Effect[k] == SPELL_EFFECT_SUMMON || spellEntry->Effect[k] == SPELL_EFFECT_SUMMON_PET)
            {
                uint32 creature_id = spellEntry->EffectMiscValue[k];
                CreatureInfo const *cInfo = sCreatureStorage.LookupEntry<CreatureInfo>(creature_id);
                if (!cInfo)
                    continue;

                // already loaded
                if (cInfo->PetSpellDataId)
                    continue;

                // for creature without PetSpellDataId get default pet spells from creature_template
                int32 petSpellsId = cInfo->Entry;
                if (mPetDefaultSpellsMap.find(cInfo->Entry) != mPetDefaultSpellsMap.end())
                    continue;

                PetDefaultSpellsEntry petDefSpells;
                for (uint8 j = 0; j < MAX_CREATURE_SPELL_DATA_SLOT; ++j)
                    petDefSpells.spellid[j] = cInfo->spells[j];

                if (LoadPetDefaultSpells_helper(cInfo, petDefSpells))
                {
                    mPetDefaultSpellsMap[petSpellsId] = petDefSpells;
                    ++countCreature;
                }
            }
        }
    }

    sLog->outString();
    sLog->outString(">> Loaded addition spells for %u pet spell data entries.", countData);
    sLog->outString(">> Loaded %u summonable creature templates.", countCreature);
}

/// Some checks for spells, to prevent adding deprecated/broken spells for trainers, spell book, etc
bool SpellMgr::IsSpellValid(SpellEntry const *spellInfo, Player *pl, bool msg)
{
    // not exist
    if (!spellInfo)
        return false;

    bool need_check_reagents = false;

    // check effects
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        switch (spellInfo->Effect[i])
        {
            case 0:
                continue;

            // craft spell for crafting non-existed item (break client recipes list show)
            case SPELL_EFFECT_CREATE_ITEM:
            case SPELL_EFFECT_CREATE_ITEM_2:
            {
                if (spellInfo->EffectItemType[i] == 0)
                {
                    // skip auto-loot crafting spells, its not need explicit item info (but have special fake items sometime)
                    if (!IsLootCraftingSpell(spellInfo))
                    {
                        if (msg)
                        {
                            if (pl)
                                ChatHandler(pl).PSendSysMessage("Craft spell %u not have create item entry.",spellInfo->Id);
                            else
                                sLog->outErrorDb("Craft spell %u not have create item entry.",spellInfo->Id);
                        }
                        return false;
                    }

                }
                // also possible IsLootCraftingSpell case but fake item must exist anyway
                else if (!ObjectMgr::GetItemPrototype(spellInfo->EffectItemType[i]))
                {
                    if (msg)
                    {
                        if (pl)
                            ChatHandler(pl).PSendSysMessage("Craft spell %u create not-exist in DB item (Entry: %u) and then...",spellInfo->Id,spellInfo->EffectItemType[i]);
                        else
                            sLog->outErrorDb("Craft spell %u create not-exist in DB item (Entry: %u) and then...",spellInfo->Id,spellInfo->EffectItemType[i]);
                    }
                    return false;
                }

                need_check_reagents = true;
                break;
            }
            case SPELL_EFFECT_LEARN_SPELL:
            {
                SpellEntry const *spellInfo2 = sSpellStore.LookupEntry(spellInfo->EffectTriggerSpell[i]);
                if (!IsSpellValid(spellInfo2,pl,msg))
                {
                    if (msg)
                    {
                        if (pl)
                            ChatHandler(pl).PSendSysMessage("Spell %u learn to broken spell %u, and then...",spellInfo->Id,spellInfo->EffectTriggerSpell[i]);
                        else
                            sLog->outErrorDb("Spell %u learn to invalid spell %u, and then...",spellInfo->Id,spellInfo->EffectTriggerSpell[i]);
                    }
                    return false;
                }
                break;
            }
        }
    }

    if (need_check_reagents)
    {
        for (uint8 j = 0; j < MAX_SPELL_REAGENTS; ++j)
        {
            if (spellInfo->Reagent[j] > 0 && !ObjectMgr::GetItemPrototype(spellInfo->Reagent[j]))
            {
                if (msg)
                {
                    if (pl)
                        ChatHandler(pl).PSendSysMessage("Craft spell %u have not-exist reagent in DB item (Entry: %u) and then...",spellInfo->Id,spellInfo->Reagent[j]);
                    else
                        sLog->outErrorDb("Craft spell %u have not-exist reagent in DB item (Entry: %u) and then...",spellInfo->Id,spellInfo->Reagent[j]);
                }
                return false;
            }
        }
    }

    return true;
}

void SpellMgr::LoadSpellAreas()
{
    mSpellAreaMap.clear();                                  // need for reload case
    mSpellAreaForQuestMap.clear();
    mSpellAreaForActiveQuestMap.clear();
    mSpellAreaForQuestEndMap.clear();
    mSpellAreaForAuraMap.clear();

    uint32 count = 0;

    //                                               0      1     2            3                   4          5           6         7       8
    QueryResult result = WorldDatabase.Query("SELECT spell, area, quest_start, quest_start_active, quest_end, aura_spell, racemask, gender, autocast FROM spell_area");

    if (!result)
    {
        sLog->outString();
        sLog->outString(">> Loaded %u spell area requirements", count);
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        uint32 spell = fields[0].GetUInt32();
        SpellArea spellArea;
        spellArea.spellId             = spell;
        spellArea.areaId              = fields[1].GetUInt32();
        spellArea.questStart          = fields[2].GetUInt32();
        spellArea.questStartCanActive = fields[3].GetBool();
        spellArea.questEnd            = fields[4].GetUInt32();
        spellArea.auraSpell           = fields[5].GetInt32();
        spellArea.raceMask            = fields[6].GetUInt32();
        spellArea.gender              = Gender(fields[7].GetUInt8());
        spellArea.autocast            = fields[8].GetBool();

        if (const SpellEntry* spellInfo = sSpellStore.LookupEntry(spell))
        {
            if (spellArea.autocast)
                const_cast<SpellEntry*>(spellInfo)->Attributes |= SPELL_ATTR0_CANT_CANCEL;
        }
        else
        {
            sLog->outErrorDb("Spell %u listed in `spell_area` does not exist", spell);
            continue;
        }

        {
            bool ok = true;
            SpellAreaMapBounds sa_bounds = GetSpellAreaMapBounds(spellArea.spellId);
            for (SpellAreaMap::const_iterator itr = sa_bounds.first; itr != sa_bounds.second; ++itr)
            {
                if (spellArea.spellId != itr->second.spellId)
                    continue;
                if (spellArea.areaId != itr->second.areaId)
                    continue;
                if (spellArea.questStart != itr->second.questStart)
                    continue;
                if (spellArea.auraSpell != itr->second.auraSpell)
                    continue;
                if ((spellArea.raceMask & itr->second.raceMask) == 0)
                    continue;
                if (spellArea.gender != itr->second.gender)
                    continue;

                // duplicate by requirements
                ok =false;
                break;
            }

            if (!ok)
            {
                sLog->outErrorDb("Spell %u listed in `spell_area` already listed with similar requirements.", spell);
                continue;
            }
        }

        if (spellArea.areaId && !GetAreaEntryByAreaID(spellArea.areaId))
        {
            sLog->outErrorDb("Spell %u listed in `spell_area` have wrong area (%u) requirement", spell,spellArea.areaId);
            continue;
        }

        if (spellArea.questStart && !sObjectMgr->GetQuestTemplate(spellArea.questStart))
        {
            sLog->outErrorDb("Spell %u listed in `spell_area` have wrong start quest (%u) requirement", spell,spellArea.questStart);
            continue;
        }

        if (spellArea.questEnd)
        {
            if (!sObjectMgr->GetQuestTemplate(spellArea.questEnd))
            {
                sLog->outErrorDb("Spell %u listed in `spell_area` have wrong end quest (%u) requirement", spell,spellArea.questEnd);
                continue;
            }

            if (spellArea.questEnd == spellArea.questStart && !spellArea.questStartCanActive)
            {
                sLog->outErrorDb("Spell %u listed in `spell_area` have quest (%u) requirement for start and end in same time", spell,spellArea.questEnd);
                continue;
            }
        }

        if (spellArea.auraSpell)
        {
            SpellEntry const* spellInfo = sSpellStore.LookupEntry(abs(spellArea.auraSpell));
            if (!spellInfo)
            {
                sLog->outErrorDb("Spell %u listed in `spell_area` have wrong aura spell (%u) requirement", spell,abs(spellArea.auraSpell));
                continue;
            }

            if (uint32(abs(spellArea.auraSpell)) == spellArea.spellId)
            {
                sLog->outErrorDb("Spell %u listed in `spell_area` have aura spell (%u) requirement for itself", spell,abs(spellArea.auraSpell));
                continue;
            }

            // not allow autocast chains by auraSpell field (but allow use as alternative if not present)
            if (spellArea.autocast && spellArea.auraSpell > 0)
            {
                bool chain = false;
                SpellAreaForAuraMapBounds saBound = GetSpellAreaForAuraMapBounds(spellArea.spellId);
                for (SpellAreaForAuraMap::const_iterator itr = saBound.first; itr != saBound.second; ++itr)
                {
                    if (itr->second->autocast && itr->second->auraSpell > 0)
                    {
                        chain = true;
                        break;
                    }
                }

                if (chain)
                {
                    sLog->outErrorDb("Spell %u listed in `spell_area` have aura spell (%u) requirement that itself autocast from aura", spell,spellArea.auraSpell);
                    continue;
                }

                SpellAreaMapBounds saBound2 = GetSpellAreaMapBounds(spellArea.auraSpell);
                for (SpellAreaMap::const_iterator itr2 = saBound2.first; itr2 != saBound2.second; ++itr2)
                {
                    if (itr2->second.autocast && itr2->second.auraSpell > 0)
                    {
                        chain = true;
                        break;
                    }
                }

                if (chain)
                {
                    sLog->outErrorDb("Spell %u listed in `spell_area` have aura spell (%u) requirement that itself autocast from aura", spell,spellArea.auraSpell);
                    continue;
                }
            }
        }

        if (spellArea.raceMask && (spellArea.raceMask & RACEMASK_ALL_PLAYABLE) == 0)
        {
            sLog->outErrorDb("Spell %u listed in `spell_area` have wrong race mask (%u) requirement", spell,spellArea.raceMask);
            continue;
        }

        if (spellArea.gender != GENDER_NONE && spellArea.gender != GENDER_FEMALE && spellArea.gender != GENDER_MALE)
        {
            sLog->outErrorDb("Spell %u listed in `spell_area` have wrong gender (%u) requirement", spell, spellArea.gender);
            continue;
        }

        SpellArea const* sa = &mSpellAreaMap.insert(SpellAreaMap::value_type(spell,spellArea))->second;

        // for search by current zone/subzone at zone/subzone change
        if (spellArea.areaId)
            mSpellAreaForAreaMap.insert(SpellAreaForAreaMap::value_type(spellArea.areaId,sa));

        // for search at quest start/reward
        if (spellArea.questStart)
        {
            if (spellArea.questStartCanActive)
                mSpellAreaForActiveQuestMap.insert(SpellAreaForQuestMap::value_type(spellArea.questStart,sa));
            else
                mSpellAreaForQuestMap.insert(SpellAreaForQuestMap::value_type(spellArea.questStart,sa));
        }

        // for search at quest start/reward
        if (spellArea.questEnd)
            mSpellAreaForQuestEndMap.insert(SpellAreaForQuestMap::value_type(spellArea.questEnd,sa));

        // for search at aura apply
        if (spellArea.auraSpell)
            mSpellAreaForAuraMap.insert(SpellAreaForAuraMap::value_type(abs(spellArea.auraSpell),sa));

        ++count;
    } while (result->NextRow());

    sLog->outString();
    sLog->outString(">> Loaded %u spell area requirements", count);
}

SpellCastResult SpellMgr::GetSpellAllowedInLocationError(SpellEntry const *spellInfo, uint32 map_id, uint32 zone_id, uint32 area_id, Player const* player)
{
    if (spellInfo->Id == 98619) // Ugly ugly hack ...
        return SPELL_CAST_OK;

    // normal case
    if (spellInfo->AreaGroupId > 0)
    {
        bool found = false;
        AreaGroupEntry const* groupEntry = sAreaGroupStore.LookupEntry(spellInfo->AreaGroupId);
        while (groupEntry)
        {
            for (uint8 i = 0; i < MAX_GROUP_AREA_IDS; ++i)
                if (groupEntry->AreaId[i] == zone_id || groupEntry->AreaId[i] == area_id)
                    found = true;
            if (found || !groupEntry->nextGroup)
                break;
            // Try search in next group
            groupEntry = sAreaGroupStore.LookupEntry(groupEntry->nextGroup);
        }

        if (!found)
            return SPELL_FAILED_INCORRECT_AREA;
    }

    // continent limitation (virtual continent)
    if (spellInfo->AttributesEx4 & SPELL_ATTR4_CAST_ONLY_IN_OUTLAND)
    {
        uint32 v_map = GetVirtualMapForMapAndZone(map_id, zone_id);
        MapEntry const *mapEntry = sMapStore.LookupEntry(v_map);
        if (!mapEntry || mapEntry->addon < 1 || !mapEntry->IsContinent())
            return SPELL_FAILED_INCORRECT_AREA;
    }

    // raid instance limitation
    if (spellInfo->AttributesEx6 & SPELL_ATTR6_NOT_IN_RAID_INSTANCE)
    {
        MapEntry const *mapEntry = sMapStore.LookupEntry(map_id);
        if (!mapEntry || mapEntry->IsRaid())
            return SPELL_FAILED_NOT_IN_RAID_INSTANCE;
    }

    // DB base check (if non empty then must fit at least single for allow)
    SpellAreaMapBounds saBounds = sSpellMgr->GetSpellAreaMapBounds(spellInfo->Id);
    if (saBounds.first != saBounds.second)
    {
        for (SpellAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
        {
            if (itr->second.IsFitToRequirements(player,zone_id,area_id))
                return SPELL_CAST_OK;
        }
        return SPELL_FAILED_INCORRECT_AREA;
    }

    // bg spell checks
    switch(spellInfo->Id)
    {
        case 23333:                                         // Horde Flag
        case 23335:                                         // Alliance Flag
            return (map_id == 489 || map_id == 726) && player && player->InBattleground() ? SPELL_CAST_OK : SPELL_FAILED_REQUIRES_AREA;
        case 34976:                                         // Netherstorm Flag
            return map_id == 566 && player && player->InBattleground() ? SPELL_CAST_OK : SPELL_FAILED_REQUIRES_AREA;
        case 2584:                                          // Waiting to Resurrect
        case 22011:                                         // Spirit Heal Channel
        case 22012:                                         // Spirit Heal
        case 24171:                                         // Resurrection Impact Visual
        case 42792:                                         // Recently Dropped Flag
        case 43681:                                         // Inactive
        case 44535:                                         // Spirit Heal (mana)
        {
            MapEntry const* mapEntry = sMapStore.LookupEntry(map_id);
            if (!mapEntry)
                return SPELL_FAILED_INCORRECT_AREA;

            return zone_id == 4197 || (mapEntry->IsBattleground() && player && player->InBattleground()) ? SPELL_CAST_OK : SPELL_FAILED_REQUIRES_AREA;
        }
        case 44521:                                         // Preparation
        {
            if (!player)
                return SPELL_FAILED_REQUIRES_AREA;

            MapEntry const *mapEntry = sMapStore.LookupEntry(map_id);
            if (!mapEntry)
                return SPELL_FAILED_INCORRECT_AREA;

            if (!mapEntry->IsBattleground())
                return SPELL_FAILED_REQUIRES_AREA;

            Battleground* bg = player->GetBattleground();
            return bg && bg->GetStatus() == STATUS_WAIT_JOIN ? SPELL_CAST_OK : SPELL_FAILED_REQUIRES_AREA;
        }
        case 32724:                                         // Gold Team (Alliance)
        case 32725:                                         // Green Team (Alliance)
        case 35774:                                         // Gold Team (Horde)
        case 35775:                                         // Green Team (Horde)
        {
            MapEntry const *mapEntry = sMapStore.LookupEntry(map_id);
            if (!mapEntry)
                return SPELL_FAILED_INCORRECT_AREA;

            return mapEntry->IsBattleArena() && player && player->InBattleground() ? SPELL_CAST_OK : SPELL_FAILED_REQUIRES_AREA;
        }
        case 32727:                                         // Arena Preparation
        {
            if (!player)
                return SPELL_FAILED_REQUIRES_AREA;

            MapEntry const *mapEntry = sMapStore.LookupEntry(map_id);
            if (!mapEntry)
                return SPELL_FAILED_INCORRECT_AREA;

            if (!mapEntry->IsBattleArena())
                return SPELL_FAILED_REQUIRES_AREA;

            Battleground *bg = player->GetBattleground();
            return bg && bg->GetStatus() == STATUS_WAIT_JOIN ? SPELL_CAST_OK : SPELL_FAILED_REQUIRES_AREA;
        }
    }

    // aura limitations
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        switch (spellInfo->EffectApplyAuraName[i])
        {
            case SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED:
            case SPELL_AURA_FLY:
            {
                if (spellInfo->Id == 82724 && map_id == 754) // Eye of the Storm spell - TotFW instance
                    break;
                if (player && !player->IsKnowHowFlyIn(map_id, zone_id))
                    return SPELL_FAILED_INCORRECT_AREA;
            }
        }
    }

    return SPELL_CAST_OK;
}

void SpellMgr::LoadSkillLineAbilityMap()
{
    mSkillLineAbilityMap.clear();

    uint32 count = 0;

    for (uint32 i = 0; i < sSkillLineAbilityStore.GetNumRows(); ++i)
    {
        SkillLineAbilityEntry const *SkillInfo = sSkillLineAbilityStore.LookupEntry(i);
        if (!SkillInfo)
            continue;

        mSkillLineAbilityMap.insert(SkillLineAbilityMap::value_type(SkillInfo->spellId,SkillInfo));
        ++count;
    }

    sLog->outString();
    sLog->outString(">> Loaded %u SkillLineAbility MultiMap Data", count);
}

DiminishingGroup GetDiminishingReturnsGroupForSpell(SpellEntry const* spellproto, bool triggered)
{
    if (IsPositiveSpell(spellproto->Id))
        return DIMINISHING_NONE;

    // Explicit Diminishing Groups
    switch (spellproto->SpellFamilyName)
    {
        // Event spells
        case SPELLFAMILY_UNK1:
            return DIMINISHING_NONE;
        case SPELLFAMILY_GENERIC:
            // some generic arena related spells have by some strange reason MECHANIC_TURN
            if  (spellproto->Mechanic == MECHANIC_TURN)
                return DIMINISHING_NONE;
            break;
        case SPELLFAMILY_MAGE:
        {
            // Improved Cone of Cold
            if (spellproto->SpellFamilyFlags[1] & 0x80000000 && spellproto->SpellIconID == 35)
                return DIMINISHING_CONTROL_ROOT;
            // Shattered Barrier: only flag SpellFamilyFlags[0] = 0x00080000 shared
            // by most frost spells, using id instead
            if ((spellproto->Id == 55080) || (spellproto->Id == 83073))
                return DIMINISHING_TRIGGER_ROOT;
            // Frost Nova / Freeze (Water Elemental)
            if ((spellproto->SpellIconID == 193) && (spellproto->Id != 55080) && (spellproto->Id != 83073))
                return DIMINISHING_CONTROL_ROOT;
            // Ring of Frost
            if (spellproto->Id == 82691)
                return DIMINISHING_DISORIENT;
            // Dragon's Breath
            if (spellproto->SpellFamilyFlags[0] & 0x00800000)
                return DIMINISHING_DISORIENT_SPECIAL;
            break;
        }
        case SPELLFAMILY_ROGUE:
        {
            // Sap 0x80 Gouge 0x8
            if (spellproto->SpellFamilyFlags[0] & 0x88)
                return DIMINISHING_DISORIENT;
            // Blind
            if (spellproto->SpellFamilyFlags[0] & 0x1000000)
                return DIMINISHING_FEAR_BLIND;
            // Cheap Shot
            if (spellproto->SpellFamilyFlags[0] & 0x400)
                return DIMINISHING_CONTROL_STUN;
            // Kidney Shot
            if (spellproto->SpellFamilyFlags[0] & 0x200000)
                return DIMINISHING_CONTROL_STUN;
            // Crippling poison - Limit to 10 seconds in PvP (No SpellFamilyFlags)
            if (spellproto->SpellIconID == 163)
                return DIMINISHING_LIMITONLY;
            break;
        }
        case SPELLFAMILY_WARLOCK:
        {
            // Death Coil
            if (spellproto->SpellFamilyFlags[0] & 0x80000)
                return DIMINISHING_DEATHCOIL;
            // Curses/etc
            if (spellproto->SpellFamilyFlags[0] & 0x80000000)
                return DIMINISHING_LIMITONLY;
            // Curse of Exhaustion
            if (spellproto->Id == 18223)
                return DIMINISHING_LIMITONLY;
            // Howl of Terror
            if (spellproto->SpellFamilyFlags[1] & 0x8)
                return DIMINISHING_FEAR_BLIND;
            // Seduction
            if (spellproto->SpellFamilyFlags[1] & 0x10000000)
                return DIMINISHING_FEAR_BLIND;
            // Unstable Affliction + Sin and Punishment should not have DR
            if (spellproto->Id == 31117 || spellproto->Id == 87204 ) // Sin and Punishment -> SPELLFAMILY_WARLOCK ??? mistake in DB ?
                return DIMINISHING_NONE;
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            // Solar beam should not be affected be DR
            if (spellproto->Id == 81261)
                return DIMINISHING_NONE;
            // Pounce
            if (spellproto->SpellFamilyFlags[0] & 0x20000)
                return DIMINISHING_CONTROL_STUN;
            // Cyclone
            if (spellproto->SpellFamilyFlags[1] & 0x20)
                return DIMINISHING_CYCLONE;
            // Entangling Roots: to force natures grasp proc to be control root
            if (spellproto->SpellFamilyFlags[0] & 0x00000200)
                return DIMINISHING_CONTROL_ROOT;
            // Faerie Fire
            if (spellproto->SpellFamilyFlags[0] & 0x400)
                return DIMINISHING_LIMITONLY;
            // Hibernate
            if (spellproto->Id == 2637)
                return DIMINISHING_DISORIENT;
            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            // Earthgrab
            if (spellproto->SpellFamilyFlags[2] & 0x00004000)
                return DIMINISHING_CONTROL_ROOT;
            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            // Hamstring - limit duration to 10s in PvP
            if (spellproto->SpellFamilyFlags[0] & 0x2)
                return DIMINISHING_LIMITONLY;
            // Intimidating Shout
            if (spellproto->SpellFamilyFlags[0] & 0x40000)
                return DIMINISHING_FEAR_BLIND;
            // Intercept + charge stun
            if ((spellproto->SpellFamilyFlags[0] & 0x01000000 || spellproto->Id == 96273) || spellproto->Id == 20253)
                return DIMINISHING_NONE;
            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            // Repentance
            if (spellproto->SpellFamilyFlags[0] & 0x4)
                return DIMINISHING_DISORIENT;
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            // Hungering Cold
            if ((spellproto->SpellFamilyFlags[1] & 0x00009000) && spellproto->SpellIconID == 2797)
                return DIMINISHING_DISORIENT;
            // Mark of Blood
            if ((spellproto->SpellFamilyFlags[0] & 0x10000000) && spellproto->SpellIconID == 2285)
                return DIMINISHING_LIMITONLY;
            break;
        }
        case SPELLFAMILY_PRIEST:
        {
            // Shackle Undead
            if ((spellproto->SpellFamilyFlags[1] & 0x04001000) && spellproto->SpellIconID == 27)
                return DIMINISHING_DISORIENT;
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            // Hunter's mark or Widow Venom
            if (((spellproto->SpellFamilyFlags[0] & 0x400) && spellproto->SpellIconID == 538) || spellproto->Id == 82654)
                return DIMINISHING_LIMITONLY;
            // Scatter Shot
            if ((spellproto->SpellFamilyFlags[0] & 0x40000) && spellproto->SpellIconID == 132)
                return DIMINISHING_DISORIENT_SPECIAL;
            // Freezing Trap
            if ((spellproto->SpellFamilyFlags[0] & 0x00000008) && spellproto->SpellIconID == 180)
                return DIMINISHING_DISORIENT;
            // Wyvern Sting
            if ((spellproto->SpellFamilyFlags[1] & 0x00001000) && spellproto->SpellIconID == 1721)
                return DIMINISHING_DISORIENT;
            // Bad Manner (Monkey)
            if (spellproto->SpellFamilyFlags[1] & 0x10000000)
                return DIMINISHING_CONTROL_STUN;
            break;
        }
        default:
            break;
    }

    // Get by mechanic
    uint32 mechanic = GetAllSpellMechanicMask(spellproto);
    if (mechanic == MECHANIC_NONE)          return DIMINISHING_NONE;
    if (mechanic & ((1<<MECHANIC_STUN) |
                    (1<<MECHANIC_SHACKLE))) return triggered ? DIMINISHING_TRIGGER_STUN : DIMINISHING_CONTROL_STUN;
    if (mechanic & ((1<<MECHANIC_SLEEP) |
                    (1<<MECHANIC_FREEZE)))  return DIMINISHING_FREEZE_SLEEP;
    if (mechanic & (1<<MECHANIC_POLYMORPH)) return DIMINISHING_DISORIENT;
    if (mechanic & (1<<MECHANIC_ROOT))      return triggered ? DIMINISHING_TRIGGER_ROOT : DIMINISHING_CONTROL_ROOT;
    if (mechanic & ((1<<MECHANIC_FEAR) |
                    (1<<MECHANIC_TURN)))    return DIMINISHING_FEAR_BLIND;
    if (mechanic & (1<<MECHANIC_CHARM))     return DIMINISHING_CHARM;
    if (mechanic & (1<<MECHANIC_SILENCE))   return DIMINISHING_SILENCE;
    if (mechanic & (1<<MECHANIC_DISARM))    return DIMINISHING_DISARM;
    if (mechanic & (1<<MECHANIC_FREEZE))    return DIMINISHING_FREEZE_SLEEP;
    if (mechanic & ((1<<MECHANIC_KNOCKOUT) |
                    (1<<MECHANIC_SAPPED)))  return DIMINISHING_KNOCKOUT;
    if (mechanic & (1<<MECHANIC_BANISH))    return DIMINISHING_BANISH;
    if (mechanic & (1<<MECHANIC_HORROR))    return DIMINISHING_DEATHCOIL;

    // Get by effect
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (spellproto->EffectApplyAuraName[i] == SPELL_AURA_MOD_TAUNT)
            return DIMINISHING_TAUNT;
    }
    return DIMINISHING_NONE;
}

int32 GetDiminishingReturnsLimitDuration(DiminishingGroup group, SpellEntry const* spellproto)
{
    if (!IsDiminishingReturnsGroupDurationLimited(group))
        return 0;

    // Explicit diminishing duration
    switch(spellproto->SpellFamilyName)
    {
        case SPELLFAMILY_HUNTER:
        {
            // Wyvern Sting
            if (spellproto->SpellFamilyFlags[1] & 0x1000)
                return 6 * IN_MILLISECONDS;
            // Hunter's Mark
            if (spellproto->SpellFamilyFlags[0] & 0x400)
                return 120 * IN_MILLISECONDS;
            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            // Repentance - limit to 6 seconds in PvP
            if (spellproto->SpellFamilyFlags[0] & 0x4)
                return 6 * IN_MILLISECONDS;
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            // Faerie Fire - limit to 40 seconds in PvP (3.1)
            if (spellproto->SpellFamilyFlags[0] & 0x400)
                return 40 * IN_MILLISECONDS;
            break;
        }
        case SPELLFAMILY_WARLOCK:
        {
            // Curse of Elements ( Max 2 minutes in PvP )
            if (spellproto->Id == 1490)
                return 120 * IN_MILLISECONDS;
            break;
        }
        default:
            break;
    }

    return 8 * IN_MILLISECONDS;
}

bool IsDiminishingReturnsGroupDurationLimited(DiminishingGroup group)
{
    switch(group)
    {
        case DIMINISHING_CONTROL_STUN:
        case DIMINISHING_TRIGGER_STUN:
        case DIMINISHING_FREEZE_SLEEP:
        case DIMINISHING_CONTROL_ROOT:
        case DIMINISHING_TRIGGER_ROOT:
        case DIMINISHING_FEAR_BLIND:
        case DIMINISHING_CHARM:
        case DIMINISHING_DISORIENT:
        case DIMINISHING_KNOCKOUT:
        case DIMINISHING_CYCLONE:
        case DIMINISHING_BANISH:
        case DIMINISHING_DISORIENT_SPECIAL:
        case DIMINISHING_LIMITONLY:
            return true;
        default:
            return false;
    }
}

DiminishingLevels GetDiminishingReturnsMaxLevel(DiminishingGroup group)
{
    switch(group)
    {
        case DIMINISHING_TAUNT:
            return DIMINISHING_LEVEL_TAUNT_IMMUNE;
        default:
            return DIMINISHING_LEVEL_IMMUNE;
    }
}

DiminishingReturnsType GetDiminishingReturnsGroupType(DiminishingGroup group)
{
    switch(group)
    {
        case DIMINISHING_TAUNT:
        case DIMINISHING_CONTROL_STUN:
        case DIMINISHING_TRIGGER_STUN:
        case DIMINISHING_CYCLONE:
            return DRTYPE_ALL;
        case DIMINISHING_FEAR_BLIND:
        case DIMINISHING_CONTROL_ROOT:
        case DIMINISHING_TRIGGER_ROOT:
        case DIMINISHING_CHARM:
        case DIMINISHING_DISORIENT:
        case DIMINISHING_SILENCE:
        case DIMINISHING_DISARM:
        case DIMINISHING_DEATHCOIL:
        case DIMINISHING_FREEZE_SLEEP:
        case DIMINISHING_BANISH:
        case DIMINISHING_DISORIENT_SPECIAL:
        case DIMINISHING_KNOCKOUT:
            return DRTYPE_PLAYER;
        default:
            break;
    }

    return DRTYPE_NONE;
}

bool IsPartOfSkillLine(uint32 skillId, uint32 spellId)
{
    SkillLineAbilityMapBounds skillBounds = sSpellMgr->GetSkillLineAbilityMapBounds(spellId);
    for (SkillLineAbilityMap::const_iterator itr = skillBounds.first; itr != skillBounds.second; ++itr)
        if (itr->second->skillId == skillId)
            return true;

    return false;
}

bool SpellArea::IsFitToRequirements(Player const* player, uint32 newZone, uint32 newArea) const
{
    if (gender != GENDER_NONE)                   // not in expected gender
        if (!player || gender != player->getGender())
            return false;

    if (raceMask)                                // not in expected race
        if (!player || !(raceMask & player->getRaceMask()))
            return false;

    if (areaId)                                  // not in expected zone
        if (newZone != areaId && newArea != areaId)
            return false;

    if (questStart)                              // not in expected required quest state
        if (!player || ((!questStartCanActive || !player->IsActiveQuest(questStart)) && !player->GetQuestRewardStatus(questStart)))
            return false;

    if (questEnd)                                // not in expected forbidden quest state
        if (!player || player->GetQuestRewardStatus(questEnd))
            return false;

    if (auraSpell)                               // not have expected aura
        if (!player || (auraSpell > 0 && !player->HasAura(auraSpell)) || (auraSpell < 0 && player->HasAura(-auraSpell)))
            return false;

    // Extra conditions -- leaving the possibility add extra conditions...
    switch(spellId)
    {
        case 58600: // No fly Zone - Dalaran
            {
                if (!player)
                    return false;

                AreaTableEntry const* pArea = GetAreaEntryByAreaID(player->GetAreaId());
                if (!(pArea && pArea->flags & AREA_FLAG_NO_FLY_ZONE))
                    return false;
                if (!player->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED) && !player->HasAuraType(SPELL_AURA_FLY))
                    return false;
                break;
            }
        case 58730: // No fly Zone - Wintergrasp
        {
            if (!player)
                return false;

            Battlefield* Bf = sBattlefieldMgr.GetBattlefieldToZoneId(player->GetZoneId());
            if (!Bf || Bf->CanFlyIn() || (!player->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED) && !player->HasAuraType(SPELL_AURA_FLY)))
                return false;
            break;
        }
        case 68719: // Oil Refinery - Isle of Conquest.
        case 68720: // Quarry - Isle of Conquest.
            {
                if (player->GetBattlegroundTypeId() != BATTLEGROUND_IC || !player->GetBattleground())
                    return false;

                uint8 nodeType = spellId == 68719 ? NODE_TYPE_REFINERY : NODE_TYPE_QUARRY;
                uint8 nodeState = player->GetTeamId() == TEAM_ALLIANCE ? NODE_STATE_CONTROLLED_A : NODE_STATE_CONTROLLED_H;

                BattlegroundIC* pIC = static_cast<BattlegroundIC*>(player->GetBattleground());
                if (pIC->GetNodeState(nodeType) == nodeState)
                    return true;

                return false;
            }
    }

    return true;
}

//-----------TRINITY-------------

bool SpellMgr::CanAurasStack(Aura const *aura1, Aura const *aura2, bool sameCaster) const
{
    SpellEntry const *spellInfo_1 = aura1->GetSpellProto();
    SpellEntry const *spellInfo_2 = aura2->GetSpellProto();
    SpellSpecific spellSpec_1 = GetSpellSpecific(spellInfo_1);
    SpellSpecific spellSpec_2 = GetSpellSpecific(spellInfo_2);
    if (spellSpec_1 && spellSpec_2)
        if (IsSingleFromSpellSpecificPerTarget(spellSpec_1, spellSpec_2)
            || (sameCaster && IsSingleFromSpellSpecificPerCaster(spellSpec_1, spellSpec_2)))
            return false;

    SpellGroupStackRule stackRule = CheckSpellGroupStackRules(spellInfo_1->Id, spellInfo_2->Id);
    if (stackRule)
    {
        if (stackRule == SPELL_GROUP_STACK_RULE_EXCLUSIVE)
            return false;
        if (sameCaster && stackRule == SPELL_GROUP_STACK_RULE_EXCLUSIVE_FROM_SAME_CASTER)
            return false;
    }

    if (spellInfo_1->SpellFamilyName != spellInfo_2->SpellFamilyName)
        return true;

    if (!sameCaster)
    {
        if (spellInfo_1->AttributesEx & SPELL_ATTR1_STACK_FOR_DIFF_CASTERS
            || spellInfo_1->AttributesEx3 & SPELL_ATTR3_STACK_FOR_DIFF_CASTERS)
            return true;

        // check same periodic auras
        for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            switch(spellInfo_1->EffectApplyAuraName[i])
            {
                // DOT or HOT from different casters will stack
                case SPELL_AURA_PERIODIC_DAMAGE:
                case SPELL_AURA_PERIODIC_DUMMY:
                case SPELL_AURA_PERIODIC_HEAL:
                case SPELL_AURA_PERIODIC_TRIGGER_SPELL:
                case SPELL_AURA_PERIODIC_ENERGIZE:
                case SPELL_AURA_PERIODIC_MANA_LEECH:
                case SPELL_AURA_PERIODIC_LEECH:
                case SPELL_AURA_POWER_BURN_MANA:
                case SPELL_AURA_OBS_MOD_POWER:
                case SPELL_AURA_OBS_MOD_HEALTH:
                case SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
                    // periodic auras which target areas are not allowed to stack this way (replenishment for example)
                    if (IsAreaOfEffectSpellEffect(spellInfo_1, i) || IsAreaOfEffectSpellEffect(spellInfo_2, i))
                        break;
                    return true;
                default:
                    break;
            }
        }
    }

    uint32 spellId_1 = GetLastSpellInChain(spellInfo_1->Id);
    uint32 spellId_2 = GetLastSpellInChain(spellInfo_2->Id);

    // same spell
    if (spellId_1 == spellId_2)
    {
        // Hack for Incanter's Absorption
        if (spellId_1 == 44413)
            return true;
        if (aura1->GetCastItemGUID() && aura2->GetCastItemGUID())
            if (aura1->GetCastItemGUID() != aura2->GetCastItemGUID() && (GetSpellCustomAttr(spellId_1) & SPELL_ATTR0_CU_ENCHANT_PROC))
                return true;
        // two auras with same id but different effects to apply can stack
        // this solves a problem of persistent area aura dropping other effects from caster
        if (!(aura1->GetEffectMask() & aura2->GetEffectMask()))
            return true;
        // same spell with same caster should not stack
        return false;
    }

    return true;
}

bool CanSpellDispelAura(SpellEntry const * dispelSpell, SpellEntry const * aura)
{
    // These auras (like ressurection sickness) can't be dispelled
    if (aura->Attributes & SPELL_ATTR0_NEGATIVE_1)
        return false;

    // These spells (like Mass Dispel) can dispell all auras
    if (dispelSpell->Attributes & SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY)
        return true;

    // These auras (like Divine Shield) can't be dispelled
    if (aura->Attributes & SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY)
        return false;

    // These auras (Cyclone for example) are not dispelable
    if (aura->AttributesEx & SPELL_ATTR1_UNAFFECTED_BY_SCHOOL_IMMUNE)
        return false;

    return true;
}

bool CanSpellPierceImmuneAura(SpellEntry const * pierceSpell, SpellEntry const * aura)
{
    // these spells pierce all avalible spells (Resurrection Sickness for example)
    if (pierceSpell->Attributes & SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY)
        return true;

    // these spells (Cyclone for example) can pierce all...
    if ((pierceSpell->AttributesEx & SPELL_ATTR1_UNAFFECTED_BY_SCHOOL_IMMUNE)
        // ...but not these (Divine shield, multicyclone for example)
        && !(aura && (aura->Mechanic == MECHANIC_IMMUNE_SHIELD || aura->Mechanic == MECHANIC_INVULNERABILITY || aura->Mechanic == MECHANIC_BANISH)))
        return true;

    return false;
}

void SpellMgr::LoadSpellEnchantProcData()
{
    mSpellEnchantProcEventMap.clear();                             // need for reload case

    uint32 count = 0;

    //                                               0      1             2          3
    QueryResult result = WorldDatabase.Query("SELECT entry, customChance, PPMChance, procEx FROM spell_enchant_proc_data");
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> Loaded %u spell enchant proc event conditions", count);
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        uint32 enchantId = fields[0].GetUInt32();

        SpellItemEnchantmentEntry const *ench = sSpellItemEnchantmentStore.LookupEntry(enchantId);
        if (!ench)
        {
            sLog->outErrorDb("Enchancment %u listed in `spell_enchant_proc_data` does not exist", enchantId);
            continue;
        }

        SpellEnchantProcEntry spe;

        spe.customChance = fields[1].GetUInt32();
        spe.PPMChance = fields[2].GetFloat();
        spe.procEx = fields[3].GetUInt32();

        mSpellEnchantProcEventMap[enchantId] = spe;

        ++count;
    } while (result->NextRow());

    sLog->outString();
    sLog->outString(">> Loaded %u enchant proc data definitions", count);
}

void SpellMgr::LoadSpellRequired()
{
    mSpellsReqSpell.clear();                                   // need for reload case
    mSpellReq.clear();                                         // need for reload case

    //                                               0         1
    QueryResult result = WorldDatabase.Query("SELECT spell_id, req_spell from spell_required");

    if (!result)
    {
        sLog->outString();
        sLog->outString(">> Loaded 0 spell required records");
        sLog->outErrorDb("`spell_required` table is empty!");
        return;
    }
    uint32 rows = 0;

    do
    {
        Field *fields = result->Fetch();

        uint32 spell_id =  fields[0].GetUInt32();
        uint32 spell_req = fields[1].GetUInt32();
        // check if chain is made with valid first spell
        SpellEntry const * spell = sSpellStore.LookupEntry(spell_id);
        if (!spell)
        {
            sLog->outErrorDb("spell_id %u in `spell_required` table is not found in dbcs, skipped", spell_id);
            continue;
        }
        SpellEntry const * req_spell = sSpellStore.LookupEntry(spell_req);
        if (!req_spell)
        {
            sLog->outErrorDb("req_spell %u in `spell_required` table is not found in dbcs, skipped", spell_req);
            continue;
        }
        if (GetFirstSpellInChain(spell_id) == GetFirstSpellInChain(spell_req))
        {
            sLog->outErrorDb("req_spell %u and spell_id %u in `spell_required` table are ranks of the same spell, entry not needed, skipped", spell_req, spell_id);
            continue;
        }
        if (IsSpellRequiringSpell(spell_id, spell_req))
        {
            sLog->outErrorDb("duplicated entry of req_spell %u and spell_id %u in `spell_required`, skipped", spell_req, spell_id);
            continue;
        }
        if (IsSpellRequiringSpell(spell_req, spell_id))
        {
            sLog->outErrorDb("avoiding indefinite recursion - entry of req_spell %u and spell_id %u in `spell_required` - skipped", spell_req, spell_id);
            continue;
        }

        mSpellReq.insert (std::pair<uint32, uint32>(spell_id, spell_req));
        mSpellsReqSpell.insert (std::pair<uint32, uint32>(spell_req, spell_id));
        ++rows;
    } while (result->NextRow());

    sLog->outString();
    sLog->outString(">> Loaded %u spell required records", rows);
}

void SpellMgr::LoadSpellRanks()
{
    mSpellChains.clear();                                   // need for reload case

    //                                               0               1         2
    QueryResult result = WorldDatabase.Query("SELECT first_spell_id, spell_id, rank from spell_ranks ORDER BY first_spell_id , rank");

    if (!result)
    {
        sLog->outString(">> Loaded 0 spell rank records");
        sLog->outString();
        sLog->outErrorDb("`spell_ranks` table is empty!");
        return;
    }

    uint32 rows = 0;
    bool finished = false;

    do
    {
        // spellid, rank
        std::list < std::pair < int32, int32 > > rankChain;
        int32 currentSpell = -1;
        int32 lastSpell = -1;

        // fill one chain
        while (currentSpell == lastSpell && !finished)
        {
            Field *fields = result->Fetch();

            currentSpell = fields[0].GetUInt32();
            if (lastSpell == -1)
                lastSpell = currentSpell;
            uint32 spell_id = fields[1].GetUInt32();
            uint32 rank = fields[2].GetUInt32();

            // don't drop the row if we're moving to the next rank
            if (currentSpell == lastSpell)
            {
                rankChain.push_back(std::make_pair(spell_id, rank));
                if (!result->NextRow())
                    finished = true;
            }
            else
                break;
        }
        // check if chain is made with valid first spell
        SpellEntry const * first = sSpellStore.LookupEntry(lastSpell);
        if (!first)
        {
            sLog->outErrorDb("Spell rank identifier(first_spell_id) %u listed in `spell_ranks` does not exist!", lastSpell);
            continue;
        }
        // check if chain is long enough
        if (rankChain.size() < 2)
        {
            sLog->outErrorDb("There is only 1 spell rank for identifier(first_spell_id) %u in `spell_ranks`, entry is not needed!", lastSpell);
            continue;
        }
        int32 curRank = 0;
        bool valid = true;
        // check spells in chain
        for (std::list<std::pair<int32, int32> >::iterator itr = rankChain.begin() ; itr!= rankChain.end(); ++itr)
        {
            SpellEntry const * spell = sSpellStore.LookupEntry(itr->first);
            if (!spell)
            {
                sLog->outErrorDb("Spell %u (rank %u) listed in `spell_ranks` for chain %u does not exist!", itr->first, itr->second, lastSpell);
                valid = false;
                break;
            }
            ++curRank;
            if (itr->second != curRank)
            {
                sLog->outErrorDb("Spell %u (rank %u) listed in `spell_ranks` for chain %u does not have proper rank value(should be %u)!", itr->first, itr->second, lastSpell, curRank);
                valid = false;
                break;
            }
        }
        if (!valid)
            continue;
        int32 prevRank = 0;
        // insert the chain
        std::list<std::pair<int32, int32> >::iterator itr = rankChain.begin();
        do
        {
            ++rows;
            int32 addedSpell = itr->first;
            mSpellChains[addedSpell].first = lastSpell;
            mSpellChains[addedSpell].last = rankChain.back().first;
            mSpellChains[addedSpell].rank = itr->second;
            mSpellChains[addedSpell].prev = prevRank;
            prevRank = addedSpell;
            ++itr;
            if (itr == rankChain.end())
            {
                mSpellChains[addedSpell].next = 0;
                break;
            }
            else
                mSpellChains[addedSpell].next = itr->first;
        }
        while (true);
    } while (!finished);

    sLog->outString(">> Loaded %u spell rank records", rows);
    sLog->outString();
}

#define INIT_NEW_SPELLENTRY(a,b) {a=new SpellEntry_n();memset(a,0,sizeof(SpellEntry_n));a->Id=b;a->SpellName="Custom Spell";}
#define INSERT_SPELLENTRY(a) {sTrueSpellStore.indexTable[a->Id]=a;}

//create new spells that isn't present in DBC and is needed for gameplay
void SpellMgr::LoadCustomSpells()
{
    SpellEntry_n* pSpell;

    /*
        HOW DOES IT WORK?

        INIT_NEW_SPELLENTRY(a,b) - inits new space for spell to variable a and sets id from variable b
        INSERT_SPELLENTRY(a) - inserts new spell to array of all spells

        in most cases, additional things are needed to set, because this is only SpellEntry_n structure
        which is raw structure from DBC. The whole SpellEntry structure access is available in
        function LoadSpellCustomAttr
    */

    // Demonic Circle: Summon (caster aura state, allows casting of teleporting spell)
    INIT_NEW_SPELLENTRY(pSpell,62388);
    pSpell->SpellName = "Demonic Circle: Summon (enabler)";
    pSpell->CastingTimeIndex = 1;
    pSpell->DurationIndex = 21;
    pSpell->rangeIndex = 1;
    INSERT_SPELLENTRY(pSpell);

    // Ozumat Heroic Kill Credit (for satisfying achievement criteria)
    INIT_NEW_SPELLENTRY(pSpell,95673);
    pSpell->SpellName = "Ozumat Heroic Kill Credit";
    pSpell->CastingTimeIndex = 1;
    pSpell->DurationIndex = 21;
    pSpell->rangeIndex = 1;
    INSERT_SPELLENTRY(pSpell);
    
    // Tribunal of Ages Kill Credit (for satisfying achievement criteria)
    
    INIT_NEW_SPELLENTRY(pSpell,59046);
    pSpell->SpellName = "Tribunal of Ages Kill Credit";
    pSpell->CastingTimeIndex = 1;
    pSpell->DurationIndex = 21;
    pSpell->rangeIndex = 1;
    INSERT_SPELLENTRY(pSpell);

    // spells for various (mostly Molten Front) achievements
    static const uint32 dummySpellsList[] = {101174, 101175, 101173, 101170, 101176, 101177, 101179, 101182, 100979, 101098, 101097, 101099, 101100, 101101, 101103, 98102, 101181, 101167};
    for (uint32 i = 0; i < sizeof(dummySpellsList)/sizeof(uint32); i++)
    {
        INIT_NEW_SPELLENTRY(pSpell, dummySpellsList[i]);
        pSpell->SpellName = "Achievement Credit - Custom";
        pSpell->CastingTimeIndex = 1;
        pSpell->DurationIndex = 21;
        pSpell->rangeIndex = 1;
        INSERT_SPELLENTRY(pSpell);
    }
}

void SpellMgr::LoadSpellCustomCrafts(uint32 i, SpellEntry* spellInfo)
{
    // ice-like preparation spell custom craft items - we need 358 (?) ilvl, not 370, it's TEMPORARY!

    for (uint8 j = 0; j < MAX_SPELL_EFFECTS; j++)
    {
        if (spellInfo->Effect[j] == SPELL_EFFECT_CREATE_ITEM)
        {
            switch (i)
            {
                case 78473:
                    spellInfo->EffectItemType[j] = 70036;
                    break;
                case 78458:
                    spellInfo->EffectItemType[j] = 70037;
                    break;
                case 78450:
                    spellInfo->EffectItemType[j] = 70038;
                    break;
                case 78459:
                    spellInfo->EffectItemType[j] = 70040;
                    break;
                case 78474:
                    spellInfo->EffectItemType[j] = 70041;
                    break;
                case 78486:
                    spellInfo->EffectItemType[j] = 70039;
                    break;
                case 78485:
                    spellInfo->EffectItemType[j] = 70042;
                    break;
                case 78451:
                    spellInfo->EffectItemType[j] = 70043;
                    break;
                case 75293:
                    spellInfo->EffectItemType[j] = 70062;
                    break;
                case 75297:
                    spellInfo->EffectItemType[j] = 70061;
                    break;
                case 75270:
                    spellInfo->EffectItemType[j] = 70063;
                    break;
                case 75306:
                    spellInfo->EffectItemType[j] = 70067;
                    break;
                case 75295:
                    spellInfo->EffectItemType[j] = 70065;
                    break;
                case 75307:
                    spellInfo->EffectItemType[j] = 70066;
                    break;
                case 75305:
                    spellInfo->EffectItemType[j] = 70060;
                    break;
                case 75291:
                    spellInfo->EffectItemType[j] = 70064;
                    break;
                case 75269:
                    spellInfo->EffectItemType[j] = 70052;
                    break;
                case 75294:
                    spellInfo->EffectItemType[j] = 70053;
                    break;
                case 75290:
                    spellInfo->EffectItemType[j] = 70054;
                    break;
                case 75304:
                    spellInfo->EffectItemType[j] = 70055;
                    break;
                case 75296:
                    spellInfo->EffectItemType[j] = 70056;
                    break;
                case 75302:
                    spellInfo->EffectItemType[j] = 70057;
                    break;
                case 75303:
                    spellInfo->EffectItemType[j] = 70058;
                    break;
                case 75292:
                    spellInfo->EffectItemType[j] = 70059;
                    break;
                case 78457:
                    spellInfo->EffectItemType[j] = 70044;
                    break;
                case 78456:
                    spellInfo->EffectItemType[j] = 70045;
                    break;
                case 78448:
                    spellInfo->EffectItemType[j] = 70046;
                    break;
                case 78449:
                    spellInfo->EffectItemType[j] = 70048;
                    break;
                case 78484:
                    spellInfo->EffectItemType[j] = 70049;
                    break;
                case 78483:
                    spellInfo->EffectItemType[j] = 70047;
                    break;
                case 78471:
                    spellInfo->EffectItemType[j] = 70050;
                    break;
                case 78470:
                    spellInfo->EffectItemType[j] = 70051;
                    break;
                case 78468:
                    spellInfo->EffectItemType[j] = 70020;
                    break;
                case 78454:
                    spellInfo->EffectItemType[j] = 70021;
                    break;
                case 78446:
                    spellInfo->EffectItemType[j] = 70022;
                    break;
                case 78447:
                    spellInfo->EffectItemType[j] = 70024;
                    break;
                case 78469:
                    spellInfo->EffectItemType[j] = 70025;
                    break;
                case 78481:
                    spellInfo->EffectItemType[j] = 70023;
                    break;
                case 78482:
                    spellInfo->EffectItemType[j] = 70026;
                    break;
                case 78455:
                    spellInfo->EffectItemType[j] = 70027;
                    break;
                case 76467:
                    spellInfo->EffectItemType[j] = 70018;
                    break;
                case 76468:
                    spellInfo->EffectItemType[j] = 70013;
                    break;
                case 76465:
                    spellInfo->EffectItemType[j] = 70019;
                    break;
                case 76472:
                    spellInfo->EffectItemType[j] = 70012;
                    break;
                case 76466:
                    spellInfo->EffectItemType[j] = 70014;
                    break;
                case 76471:
                    spellInfo->EffectItemType[j] = 70015;
                    break;
                case 76470:
                    spellInfo->EffectItemType[j] = 70016;
                    break;
                case 76469:
                    spellInfo->EffectItemType[j] = 70017;
                    break;
                case 76458:
                    spellInfo->EffectItemType[j] = 70010;
                    break;
                case 76459:
                    spellInfo->EffectItemType[j] = 70005;
                    break;
                case 76456:
                    spellInfo->EffectItemType[j] = 70011;
                    break;
                case 76464:
                    spellInfo->EffectItemType[j] = 70004;
                    break;
                case 76457:
                    spellInfo->EffectItemType[j] = 70006;
                    break;
                case 76463:
                    spellInfo->EffectItemType[j] = 70007;
                    break;
                case 76462:
                    spellInfo->EffectItemType[j] = 70008;
                    break;
                case 76461:
                    spellInfo->EffectItemType[j] = 70009;
                    break;
                case 78445:
                    spellInfo->EffectItemType[j] = 70028;
                    break;
                case 78453:
                    spellInfo->EffectItemType[j] = 70029;
                    break;
                case 78444:
                    spellInfo->EffectItemType[j] = 70030;
                    break;
                case 78452:
                    spellInfo->EffectItemType[j] = 70032;
                    break;
                case 78480:
                    spellInfo->EffectItemType[j] = 70033;
                    break;
                case 78467:
                    spellInfo->EffectItemType[j] = 70031;
                    break;
                case 78479:
                    spellInfo->EffectItemType[j] = 70034;
                    break;
                case 78464:
                    spellInfo->EffectItemType[j] = 70035;
                    break;
                default:
                    break;
            }
        }
     }
}

// set data in core for now
void SpellMgr::LoadSpellCustomAttr()
{
    mSpellCustomAttr.resize(GetSpellStore()->GetNumRows(), 0);  // initialize with 0 values;

    uint32 count = 0;

    SpellEntry* spellInfo = NULL;
    for (uint32 i = 0; i < sSpellStore.GetNumRows(); ++i)
    {
        spellInfo = (SpellEntry*)sSpellStore.LookupEntry(i);
        if (!spellInfo)
            continue;

        LoadSpellCustomCrafts(i, spellInfo);

        for (uint8 j = 0; j < MAX_SPELL_EFFECTS; ++j)
        {
            switch (spellInfo->Effect[j])
            {
                case SPELL_EFFECT_SCHOOL_DAMAGE:
                case SPELL_EFFECT_WEAPON_DAMAGE:
                case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
                case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
                case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
                case SPELL_EFFECT_HEAL:
                    mSpellCustomAttr[i] |= SPELL_ATTR0_CU_DIRECT_DAMAGE;
                    count++;
                    break;
                case SPELL_EFFECT_CHARGE:
                case SPELL_EFFECT_CHARGE_DEST:
                case SPELL_EFFECT_JUMP:
                case SPELL_EFFECT_JUMP_DEST:
                case SPELL_EFFECT_LEAP_BACK:
                    if (!spellInfo->speed && !spellInfo->SpellFamilyName)
                        spellInfo->speed = SPEED_CHARGE;
                    mSpellCustomAttr[i] |= SPELL_ATTR0_CU_CHARGE;
                    count++;
                    break;
                case SPELL_EFFECT_PICKPOCKET:
                    mSpellCustomAttr[i] |= SPELL_ATTR0_CU_PICKPOCKET;
                    break;
                case SPELL_EFFECT_TRIGGER_SPELL:
                    if (IsPositionTarget(spellInfo->EffectImplicitTargetA[j]) ||
                        spellInfo->Targets & (TARGET_FLAG_SOURCE_LOCATION|TARGET_FLAG_DEST_LOCATION))
                        spellInfo->Effect[j] = SPELL_EFFECT_TRIGGER_MISSILE;
                    count++;
                    break;
                case SPELL_EFFECT_ENCHANT_ITEM:
                case SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY:
                case SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC:
                case SPELL_EFFECT_ENCHANT_HELD_ITEM:
                {
                    // only enchanting profession enchantments procs can stack
                    if (IsPartOfSkillLine(SKILL_ENCHANTING, i))
                    {
                        uint32 enchantId = spellInfo->EffectMiscValue[j];
                        SpellItemEnchantmentEntry const *enchant = sSpellItemEnchantmentStore.LookupEntry(enchantId);
                        for (uint8 s = 0; s < MAX_ITEM_ENCHANTMENT_EFFECTS; ++s)
                        {
                            if (enchant->type[s] != ITEM_ENCHANTMENT_TYPE_COMBAT_SPELL)
                                continue;

                            SpellEntry const *procInfo = sSpellStore.LookupEntry(enchant->spellid[s]);
                            if (!procInfo)
                                continue;

                            // if proced directly from enchantment, not via proc aura
                            // NOTE: Enchant Weapon - Blade Ward also has proc aura spell and is proced directly
                            // however its not expected to stack so this check is good
                            if (IsSpellHaveAura(procInfo, SPELL_AURA_PROC_TRIGGER_SPELL))
                                continue;

                            mSpellCustomAttr[enchant->spellid[s]] |= SPELL_ATTR0_CU_ENCHANT_PROC;
                        }
                    }
                    break;
                }
            }

            switch (SpellTargetType[spellInfo->EffectImplicitTargetA[j]])
            {
                case TARGET_TYPE_UNIT_TARGET:
                case TARGET_TYPE_DEST_TARGET:
                    spellInfo->Targets |= TARGET_FLAG_UNIT;
                    count++;
                    break;
                default:
                    break;
            }
        }

        for (uint8 j = 0; j < MAX_SPELL_EFFECTS; ++j)
        {
            switch (spellInfo->EffectApplyAuraName[j])
            {
                case SPELL_AURA_MOD_POSSESS:
                case SPELL_AURA_MOD_CONFUSE:
                case SPELL_AURA_MOD_CHARM:
                case SPELL_AURA_AOE_CHARM:
                case SPELL_AURA_MOD_FEAR:
                case SPELL_AURA_MOD_STUN:
                    mSpellCustomAttr[i] |= SPELL_ATTR0_CU_AURA_CC;
                    count++;
                    break;
            }
        }

        if (!_isPositiveEffect(i, 0, false))
        {
            mSpellCustomAttr[i] |= SPELL_ATTR0_CU_NEGATIVE_EFF0;
            count++;
        }
        if (!_isPositiveEffect(i, 1, false))
        {
            mSpellCustomAttr[i] |= SPELL_ATTR0_CU_NEGATIVE_EFF1;
            count++;
        }
        if (!_isPositiveEffect(i, 2, false))
        {
            mSpellCustomAttr[i] |= SPELL_ATTR0_CU_NEGATIVE_EFF2;
            count++;
        }

        if (spellInfo->SpellVisual[0] == 3879)
        {
            mSpellCustomAttr[i] |= SPELL_ATTR0_CU_CONE_BACK;
            count++;
        }

        if (spellInfo->activeIconID == 2158)  // flight
        {
            spellInfo->Attributes |= SPELL_ATTR0_PASSIVE;
            count++;
        }

        switch (i)
        {
        case 118:   // Polymorph
        case 61305: // Polymorph (other animal)
        case 28272: // polymorph (other animal)
        case 61721: // Polymorph (other animal)
        case 61780: // Polymorph (other animal)
        case 28271: // Polymorph (other animal)
            spellInfo->AuraInterruptFlags = AURA_INTERRUPT_FLAG_TAKE_DAMAGE;
            count++;
            break;
        case 2643: // Multi-Shot no-target Effect 0 fix.
            spellInfo->EffectImplicitTargetA[0] = TARGET_DST_TARGET_ENEMY;
            count++;
            break;
        case 82928: // Aimed Shot! casting time fix
            spellInfo->CastingTimeIndex = 1;
            count++;
            break;
        case 82661: //Aspect of the Fox
            spellInfo->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            count++;
            break;
        case 77223: // Enhaced Elements (bad spellfamily name and flags)
            // probably Blizzard's mistake
            spellInfo->SpellClassOptionsId = 7443;
            spellInfo->EffectSpellClassMask[0].Set(0x06200000,0x00001106,0xB610020F);
            count++;
            break;
        case 85421: // Burning Ember ( warlock )
            spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
            break;
        case 49206: // Summon Gargoyle
            spellInfo->DurationIndex = 9; // 30 s
            break;
        case 54424: // Fel Intelligence (warlock)
            spellInfo->EffectValueMultiplier[1] = 362.05f;
            break;
        case 62388: //Demonic Circle: Summon (caster aura spell)
            spellInfo->EquippedItemClass = -1;
            spellInfo->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_CASTER;
            spellInfo->EffectApplyAuraName[0] = SPELL_AURA_DUMMY;
            break;
        case 95673: // Ozumat Heroic Kill Credit
            spellInfo->EquippedItemClass = -1;
            spellInfo->Effect[0] = SPELL_EFFECT_DUMMY;
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_CASTER;
            break;
        case 59046: // Tribunal of Ages Kill Credit
            spellInfo->EquippedItemClass = -1;
            spellInfo->Effect[0] = SPELL_EFFECT_DUMMY;
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_CASTER;
            break;
        case 101174: // spells for various (mostly Molten Front) achievements
        case 101175:
        case 101173:
        case 101170:
        case 101176:
        case 101177:
        case 101179:
        case 101182:
        case 100979:
        case 101098:
        case 101097:
        case 101099:
        case 101100:
        case 101101:
        case 101103:
        case 98102:
        case 101181:
        case 101167:
            spellInfo->EquippedItemClass = -1;
            spellInfo->Effect[0] = SPELL_EFFECT_DUMMY;
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_TARGET_ANY;
            break;
        case 87934: //Serpent Spread
        case 87935:
            spellInfo->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            spellInfo->EffectApplyAuraName[0] = SPELL_AURA_PROC_TRIGGER_SPELL;
            count++;
            break;
        case 63093: // Glyph of Mirror Image
            spellInfo->Effect[0] = SPELL_EFFECT_APPLY_AURA;
            spellInfo->EffectApplyAuraName[0] = SPELL_AURA_DUMMY;
            count++;
            break;
        case 87427: // Shadowy Apparition Visual
            spellInfo->AttributesEx3 |= SPELL_ATTR3_DEATH_PERSISTENT;
            count++;
            break;
        case 84726: // Frostfire Orb (rank 1)
        case 84727: // Frostfire Orb (rank 2)
            // we need to send amount in order to override spell in action bar
            spellInfo->AttributesEx8 |= SPELL_ATTR8_AURA_SEND_AMOUNT;
            count++;
            break;
        case 16213: // Purification (passive)
            // 4.0.6a Blizzard hotfix, note in client not present, also DBC data wrong!
            spellInfo->EffectBasePoints[0] = 25;
            spellInfo->EffectBasePoints[1] = 25;
            count++;
            break;
        case 49224: // Magic Suppression (Ranks 1, 2 and 3)
        case 49610:
        case 49611:
            // these spells have charges, which causes their drop after first trigger (which is wrong)
            spellInfo->procCharges = 0;
            break;
        case 55095: // Frost Fever
            // originally MECHANIC_PACIFY, but that causes the effect to be dispelled with other pacify spells,
            // and that's wrong
            spellInfo->EffectMechanic[1] = MECHANIC_INFECTED;
            break;
        /**************************** ALYSRAZOR  **********************************/

        case 100555: // Smouldering Roots
            spellInfo->Effect[1] = 0;
        break;
        case 99432: //Burnout
            spellInfo->EffectTriggerSpell[1] = 0;
        break;
        case 99388: // Imprinted
        case 99389:
        case 100359:
        case 100358:
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_TARGET_ENEMY;
        break;
        case 101223: // Fieroblast
        case 101294:
        case 101295:
        case 101296:
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_TARGET_ENEMY;
            spellInfo->EffectImplicitTargetA[1] = TARGET_UNIT_TARGET_ENEMY;
        break;
        case 99816: // Fiery Tornado
        case 100733:
        case 100734:
        case 100735:
            spellInfo->EffectRadiusIndex[EFFECT_0] = 26;
            spellInfo->EffectRadiusIndex[EFFECT_1] = 26;
            break;
        case 99844: // Blazing Claw
        case 101729:
        case 101730:
        case 101731:
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_CONE_ENEMY;
            spellInfo->EffectImplicitTargetA[1] = TARGET_UNIT_CONE_ENEMY;
            spellInfo->EffectImplicitTargetA[2] = TARGET_UNIT_CONE_ENEMY;

            spellInfo->Effect[2] = SPELL_EFFECT_APPLY_AURA;
            spellInfo->EffectApplyAuraName[2] = SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN;
            spellInfo->EffectValueMultiplier[2] = 10;
            spellInfo->EffectBasePoints[2] = 10;
            spellInfo->EffectMiscValue[2] = 1;
            spellInfo->EffectMiscValueB[2] = 0;
            break;
/************************ END OF ALYSRAZOR *******************************/


/************************ MAJORDOMO STAGHELM *******************************/

        case 98583: // Burning orb
        case 99629: // Reckless Leap
            spellInfo->EffectTriggerSpell[0] = 0;
            break;
        case 98584: // Burning orb damage
        case 100209:
        case 100210:
        case 100211:
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_TARGET_ENEMY;// should be single target not chain spell
            spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
            break;
        case 98450:// Searing Seeds
            spellInfo->AttributesEx3 |= SPELL_ATTR3_PLAYERS_ONLY;
            break;

/********************* END OF MAORDOMO STAGHELM ****************************/

/*************************  RAGNAROS FIRELANDS *****************************/

        case 98928: // Lava wave leap back effect is handling incorrect -> fixed in AI
        case 100292:
        case 100293:
        case 100294:
            spellInfo->Effect[2] = SPELL_EFFECT_NONE;
            spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_INITIAL_AGGRO;
            spellInfo->AttributesEx |= SPELL_ATTR1_NO_THREAT;
            if ( i == 100292)
                spellInfo->excludeTargetAuraSpell = 100292;
            else if ( i == 100293)
                spellInfo->excludeTargetAuraSpell = 100293;
            else if ( i == 100294)
                spellInfo->excludeTargetAuraSpell = 100294;
            break;
        case 100455:
        case 101229:
        case 101230:
        case 101231:
            spellInfo->AttributesEx3 |= SPELL_ATTR3_PLAYERS_ONLY;
            break;
        case 101088: // Lavalogged
        case 101102:
            spellInfo->EffectImplicitTargetA[0] = TARGET_SRC_CASTER;
            spellInfo->EffectImplicitTargetB[0] = TARGET_UNIT_AREA_ALLY_SRC;
            break;
        case 99849: // Fixate debuff
            spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
            break;
        case 99296: // Missing triggering spell
        case 100282:
        case 100283:
        case 100284:
            spellInfo->EffectTriggerSpell[0] = 99303;
            break;
        case 98518: // Molten Inferno
        case 100252:
        case 100253:
        case 100254:
            spellInfo->EffectRadiusIndex[0] = 22;
            break;
        case 99126: // Blazing Heat
        case 100984:
        case 100985:
        case 100986:
            spellInfo->DurationIndex = 1; // 10 seconds ( pre nerfed value )
            break;
        case 99414: // Burning speed - speed handling in G-core is incorrect ( SPELL_AURA_MOD_INCREASE_SPEED )
        case 100306:// after application ignore speed_walk,speed_run from DB and start from 1.0
        case 100307:
        case 100308:
            spellInfo->EffectApplyAuraName[1] = SPELL_AURA_DUMMY;
            break;
        case 98710: // Sulfuras smash
        case 100890:
        case 100891:
        case 100892:
        spellInfo->AttributesEx5 &= ~SPELL_ATTR5_UNK19;// SPELL_ATTR5_DONT_TURN_DURING_CAST
            break;
        case 98951: // Splitting blow
        case 100883:
        case 100884:
        case 100885:
        spellInfo->AttributesEx5 &= ~SPELL_ATTR5_UNK19;// SPELL_ATTR5_DONT_TURN_DURING_CAST
            break;
        case 99172: // Engulfing flames
        case 100175:
        case 100176:
        case 100177:
            spellInfo->EffectTriggerSpell[0] = 0;
            break;
        case 100182: // Englufing flames (HC), visual only
        case 100183:
            spellInfo->Effect[0] = SPELL_EFFECT_DUMMY;
            break;
        case 100171: // World in flames  (HC)
        case 100190:
            spellInfo->AttributesEx &= ~SPELL_ATTR1_CHANNELED_1;
            break;
        case 98982: // Submerge - disable lava bolts from triggering
        case 100295:
        case 100296:
        case 100297:
            spellInfo->EffectTriggerSpell[0] = 0;
            break;
        case 100158: // Molten Power
        case 100302:
            spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
            break;
        case 98870: // Scorched ground
        case 100122:
        case 100123:
        case 100124:
            spellInfo->AttributesEx3 |= SPELL_ATTR3_PLAYERS_ONLY;
            spellInfo->EffectImplicitTargetB[0] = TARGET_UNIT_AREA_ALLY_DST;
            break;
        case 100342: // Draw Out Firelord
        case 100344:
        case 100345:
        case 100606: // Empower Sulfuras Visual Missile
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_TARGET_ANY;
            if (i == 100345)
                spellInfo->EffectImplicitTargetA[0] = TARGET_NONE;
            break;
        case 100250: // Combustion - bad targeting, handled in AI
        case 100249:
            mSpellCustomAttr[i] |= SPELL_ATTR0_CU_NEGATIVE_EFF0;
            spellInfo->Effect[0] = 0;
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_CASTER;
        break;
        case 100756: // Cloudburst summon effect
            spellInfo->Effect[0] = 0;
            break;
        case 100647: // Entrapping Roots
            spellInfo->EffectTriggerSpell[0] = 0;
            break;
        case 100876: // Summon dreadflame
            spellInfo->DurationIndex = 21; // unlimited
            break;

/*************************  END OF RAGNAROS FIRELANDS *****************************/

        case 88691: //Marked for Death Tracking
            spellInfo->EffectApplyAuraName[0] = SPELL_AURA_MOD_STALKED;
            count++;
            break;
        case 42402: // Surge
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_TARGET_ENEMY;
            spellInfo->EffectImplicitTargetA[1] = TARGET_UNIT_TARGET_ENEMY;
            spellInfo->EffectImplicitTargetA[2] = TARGET_UNIT_TARGET_ENEMY;
            break;
        case 81170: // Stampede
            spellInfo->Attributes &= ~SPELL_ATTR0_ONLY_STEALTHED;
            count++;
            break;
        case 49838: // Stop Time
        case 50526: // Wandering Plague
        case 52916: // Honor Among Thieves
            spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_INITIAL_AGGRO;
            count++;
            break;
        case 76189: // Crepuscular Veil
        case 1978:  // Serpent Sting
        case 2818:  // Deadly Poison
        case 84617: // Revealing Strike
        case 84748: // Bandit's Guile
        case 86346: // Colossus Smash
        case 91021: // Find Weakness
        case 89299: case 92953: // Twilight Spit
        case 96886: // Occu'thar: Focused Fire eyebeam visual dummy aura
            spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
            count++;
            break;
        case 97358: // Gaping Wound + difficulty entries
        case 97357:
            spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
            spellInfo->EffectRadiusIndex[0] = 28;
            spellInfo->EffectRadiusIndex[1] = 28;
            break;
        case 31700: // Black Qiraji Battle Tank
        case 44824: // Flying Reindeer
            // iCelike casting time fix - 1.5seconds like other mounts
            spellInfo->CastingTimeIndex = 16;
            count++;
            break;
        case 101010: // [DND] Target Indicator - Quick Shot (Darkmoon faire island spell)
            spellInfo->DurationIndex = 27; // lenghten duration to 3 seconds from 1.25s, we need it to last longer
            break;
        case 81281: // Fungal growth
        case 81288:
            spellInfo->EffectRadiusIndex[0] = 14; // 8 yards
        break;
        case 99233: // Burning Rage T12 warrior dps bonus
            spellInfo->DurationIndex = 29; // 12 s 
        break;
        case 74410: // Arena - Dampening
        case 74411: // Battleground - Dampening
            spellInfo->EffectBasePoints[0] = -10; // - 10 % healing done
        break;
        case 98552: // Summon Spark of Rhyolith
        case 98136: // Summon Fragment of Rhyolith
        case 100392:
            spellInfo->DurationIndex = 21; // unlimited - despawned by AI
            break;
        case 99845: // Immolation (Rhyolith, Firelands)
            spellInfo->EffectRadiusIndex[0] = 12;
            break;
        case 98649: // Meltdown ( Rhyolith) - not working as intendet
        case 101646:
        case 101647:
        case 101648:
            spellInfo->EffectTriggerSpell[1] = 0;
            break;
        case 99875: // Fuse
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_TARGET_ANY;
            spellInfo->EffectImplicitTargetB[0] = TARGET_NONE;
            spellInfo->EffectImplicitTargetA[2] = TARGET_UNIT_TARGET_ANY;
            break;
        case 86956: // Focused laser
            spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
            break;
        case 1680: // Whirlwind  (Fury)
            spellInfo->EffectRadiusIndex[0] = 8;
            spellInfo->EffectRadiusIndex[1] = 8;
            count++;
            break;
        case 44949: // Whirlwind Off-Hand (Fury)
            spellInfo->EffectRadiusIndex[0] = 8;
            spellInfo->EffectRadiusIndex[1] = 8;
            count++;
            break;
        case 50622: // Whirlwind (triggered by Bladestorm)
            spellInfo->EffectRadiusIndex[0] = 8;
            spellInfo->EffectRadiusIndex[1] = 8;
            spellInfo->EffectRadiusIndex[2] = 8;
            count++;
            break;
        case 77486: // Shadow Orb Power (disable spell proc, done in other way)
            spellInfo->Effect[0] = 0;
            break;
        case 24576: // Chromatic Mount (disable slow fall effect)
            spellInfo->Effect[2] = 0;
            break;
        case 44543: //Fingers of Frost rank 1
            spellInfo->procChance = 7;
            count++;
            break;
        case 44545: //Fingers of Frost rank 2
            spellInfo->procChance = 14;
            count++;
            break;
        case 83074: //Fingers of Frost rank 3
            spellInfo->procChance = 20;
            count++;
            break;
        case 44544: // Fingers of Frost proc'd effect
            spellInfo->EffectSpellClassMask[0][0] |= 0x00020000; // add Ice Lance
            count++;
            break;
        case 81782: // Power Word : Barrier
            spellInfo->DurationIndex = 39;
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_TARGET_ALLY;
            spellInfo->EffectImplicitTargetA[1] = TARGET_UNIT_TARGET_ALLY;
            spellInfo->EffectImplicitTargetB[0] = TARGET_NONE;
            spellInfo->EffectImplicitTargetB[1] = TARGET_NONE;
            count++;
            break;
        // Bind
        case 3286:
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_TARGET_ENEMY;
            spellInfo->EffectImplicitTargetA[1] = TARGET_UNIT_TARGET_ENEMY;
            count++;
            break;
        case 8494: // Mana Shield (rank 2)
            // because of bug in dbc
            spellInfo->procChance = 0;
            ++count;
            break;
        case 97499: // Water Totem
        case 97500: // Water Totem
        case 43302: // Lightning Totem
        case 97492: // Lightning Totem
            spellInfo->EffectBasePoints[0] = 1;
            break;
        // Sweeping Strikes
        case 12328:
            // this allows keeping spell even when changing stances
            spellInfo->AttributesEx2 |= SPELL_ATTR2_NOT_NEED_SHAPESHIFT;
            break;
        // Heroism
        case 32182:
            spellInfo->excludeCasterAuraSpell = 57723; // Exhaustion
            count++;
            break;
        // Holy Radiance
        case 82327:
            spellInfo->MaxAffectedTargets = 6;
            break;
        // Blazing Harpoon
        case 61588:
            spellInfo->MaxAffectedTargets = 1;
            count++;
            break;
        // (Improved) Lava Lash
        case 105792:
            spellInfo->MaxAffectedTargets = 4;
            count++;
            break;
        // Bloodlust
        case 2825:
            spellInfo->excludeCasterAuraSpell = 57724; // Sated
            count++;
            break;
        // Time Warp
        case 80353:
            spellInfo->excludeCasterAuraSpell = 80354; // Temporal Displacement
            count++;
            break;
        // Ancient Hysteria
        case 90355:
            spellInfo->excludeCasterAuraSpell = 95809; // Insanity
            count++;
            break;
        // Heart of the Crusader
        case 20335:
        case 20336:
        case 20337:
        // Entries were not updated after spell effect change, we have to do that manually :/
            spellInfo->AttributesEx3 |= SPELL_ATTR3_CAN_PROC_TRIGGERED;
            count++;
            break;
        case 16007: // Draco-Incarcinatrix 900
            // was 46, but effect is aura effect
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_NEARBY_ENTRY;
            spellInfo->EffectImplicitTargetB[0] = TARGET_DST_NEARBY_ENTRY;
            count++;
            break;
        case 24131:                             // Wyvern Sting (rank 1)
        case 24134:                             // Wyvern Sting (rank 2)
        case 24135:                             // Wyvern Sting (rank 3)
            // something wrong and it applied as positive buff
            mSpellCustomAttr[i] |= SPELL_ATTR0_CU_NEGATIVE_EFF0;
            count++;
            break;
        case 88287: // Massive Crash (boss spell) and ranks
        case 91914:
        case 91921:
        case 91922:
            mSpellCustomAttr[i] |= SPELL_ATTR0_CU_NEGATIVE_EFF0;
            mSpellCustomAttr[i] |= SPELL_ATTR0_CU_NEGATIVE_EFF1;
            count++;
            break;
        case 74522: // Skinning (Grandmaster)
            // Because of bug in DBC, we need to set basepoints manually
            spellInfo->EffectBasePoints[1] = 7;
            count++;
            break;
        case 86303: // Reactive barrier (rank 1)
        case 86304: // Reactvie barrier (rank 2)
            spellInfo->EffectApplyAuraName[0] = SPELL_AURA_SCHOOL_ABSORB;
            spellInfo->EffectMiscValue[0] = 127;
            break;
        case 46841: // Escape to the Isle of Quel'Danas
            // not sure why this is needed
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_TARGET_ANY;
            spellInfo->EffectImplicitTargetB[0] = TARGET_DST_DB;
            count++;
            break;
        case 26029: // dark glare
        case 37433: // spout
        case 43140: case 43215: // flame breath
        case 99510: // Lava
            mSpellCustomAttr[i] |= SPELL_ATTR0_CU_CONE_LINE;
            count++;
            break;
        case 24340: case 26558: case 28884:     // Meteor
        case 36837: case 38903: case 41276:     // Meteor
        case 57467:                             // Meteor
        case 26789:                             // Shard of the Fallen Star
        case 31436:                             // Malevolent Cleave
        case 35181:                             // Dive Bomb
        case 40810: case 43267: case 43268:     // Saber Lash
        case 42384:                             // Brutal Swipe
        case 45150:                             // Meteor Slash
        case 88942: case 95172:                 // Meteor Slash (Argaloth)
        case 89348: case 95178:                 // Demon Repellent Ray (BH trash)
        case 64422: case 64688:                 // Sonic Screech
        case 72373:                             // Shared Suffering
        case 71904:                             // Chaos Bane
        case 70492: case 72505:                 // Ooze Eruption
        case 72624: case 72625:                 // Ooze Eruption
        case 86704:                             // Paladin: Guardian of Ancient Kings: Ancient Fury
        case 86367: case 93135: case 93136:     // Conclave of Wind: Nezir:
        case 93137:                             // Sleet Storm (4 heroic entries)
        case 77679: case 92968: case 92969:     // Scorching Blast
        case 92970:                             // (4 heroic entries)
        case 88917: case 88916: case 88915:     // Caustic Slime
        case 82935:                             // (4 heroic entries)
        case 86014: case 92863: case 92864: case 92865: // Twilight Meteorite (Valiona)
        case 86825: case 92879: case 92880: case 92881: // Blackout (Valiona)
        case 98474: case 100212: case 100213: case 100214: // Flame Scythe (Majordomo Staghelm)
        case 100431:                            // Flaming Cleave ( Fireland's trash )
            // ONLY SPELLS WITH SPELLFAMILY_GENERIC and EFFECT_SCHOOL_DAMAGE
            mSpellCustomAttr[i] |= SPELL_ATTR0_CU_SHARE_DAMAGE;
            count++;
            break;
        case 59725:                             // Improved Spell Reflection - aoe aura
            // Target entry seems to be wrong for this spell :/
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_PARTY_CASTER;
            spellInfo->EffectRadiusIndex[0] = 45;
            count++;
            break;
        case 27820:                             // Mana Detonation
        //case 28062: case 39090:                 // Positive/Negative Charge
        //case 28085: case 39093:
        case 69782: case 69796:                 // Ooze Flood
        case 69798: case 69801:                 // Ooze Flood
        case 69538: case 69553: case 69610:     // Ooze Combine
            mSpellCustomAttr[i] |= SPELL_ATTR0_CU_EXCLUDE_SELF;
            count++;
            break;
        case 44978: case 45001: case 45002:     // Wild Magic
        case 45004: case 45006: case 45010:     // Wild Magic
        case 31347: // Doom
        case 41635: // Prayer of Mending
        case 44869: // Spectral Blast
        case 45027: // Revitalize
        case 45976: // Muru Portal Channel
        case 39365: // Thundering Storm
        case 41071: // Raise Dead (HACK)
        case 52124: // Sky Darkener Assault
        case 42442: // Vengeance Landing Cannonfire
        case 45863: // Cosmetic - Incinerate to Random Target
        case 25425: // Shoot
        case 45761: // Shoot
        case 42611: // Shoot
        case 62374: // Pursued
            spellInfo->MaxAffectedTargets = 1;
            count++;
            break;
        case 85425: // Conclave of Wind: Anshal: Nurture triggered summoning spells
            spellInfo->AttributesEx &= ~SPELL_ATTR1_CHANNELED_2;
            count++;
            break;
        case 86282: // Conclave of Wind: Anshal:
        case 93120: // Ravenous Creeper:
        case 93121: // Toxic Spores
        case 93122: // 4 difficulty entries
            spellInfo->AttributesEx |= SPELL_ATTR1_STACK_FOR_DIFF_CASTERS;
            count++;
            break;
        case 85483: // Conclave of Wind:
        case 93138: // Rohash:
        case 93139: // Wind Blast
        case 93140: // 4 difficulty entries
            // target 104
            spellInfo->EffectImplicitTargetA[1] = spellInfo->EffectImplicitTargetA[0];
            count++;
            break;
        case 52479: // Gift of the Harvester
            spellInfo->MaxAffectedTargets = 1;
            // a trap always has dst = src?
            spellInfo->EffectImplicitTargetA[0] = TARGET_DST_CASTER;
            spellInfo->EffectImplicitTargetA[1] = TARGET_DST_CASTER;
            count++;
            break;
        case 89024: // Pursuit of Justice
        case 86698: // Guardian of Ancient Kings
        case 89023: // Blessed Life
            spellInfo->Effect[1] = 0; // removed non-exist triggered spell
            break;
        case 23257: // Demonic Enrage
            spellInfo->Effect[2] = 0; // removed non-exist triggered spell
            break;
        case 41376: // Spite
        case 39992: // Needle Spine
        case 29576: // Multi-Shot
        case 40816: // Saber Lash
        case 37790: // Spread Shot
        case 46771: // Flame Sear
        case 45248: // Shadow Blades
        case 41303: // Soul Drain
        case 54172: // Divine Storm (heal)
        case 29213: // Curse of the Plaguebringer - Noth
        case 28542: // Life Drain - Sapphiron
        case 66588: // Flaming Spear
        case 54171: // Divine Storm
            spellInfo->MaxAffectedTargets = 3;
            count++;
            break;
        case 38310: // Multi-Shot
            spellInfo->MaxAffectedTargets = 4;
            count++;
            break;
        case 42005: // Bloodboil
        case 38296: // Spitfire Totem
        case 37676: // Insidious Whisper
        case 46008: // Negative Energy
        case 45641: // Fire Bloom
        case 55665: // Life Drain - Sapphiron (H)
        case 28796: // Poison Bolt Volly - Faerlina
            spellInfo->MaxAffectedTargets = 5;
            count++;
            break;
        case 40827: // Sinful Beam
        case 40859: // Sinister Beam
        case 40860: // Vile Beam
        case 40861: // Wicked Beam
        case 54835: // Curse of the Plaguebringer - Noth (H)
        case 54098: // Poison Bolt Volly - Faerlina (H)
            spellInfo->MaxAffectedTargets = 10;
            count++;
            break;
        case 50312: // Unholy Frenzy
            spellInfo->MaxAffectedTargets = 15;
            count++;
            break;
        case 38794: case 33711: //Murmur's Touch
            spellInfo->MaxAffectedTargets = 1;
            spellInfo->EffectTriggerSpell[0] = 33760;
            count++;
            break;
        case 17941:    // Shadow Trance
        case 22008:    // Netherwind Focus
        case 31834:    // Light's Grace
        case 34754:    // Clearcasting
        case 34936:    // Backlash
        case 48108:    // Hot Streak
        case 51124:    // Killing Machine
        case 54741:    // Firestarter
        case 57761:    // Fireball!
        case 39805:    // Lightning Overload
        case 64823:    // Item - Druid T8 Balance 4P Bonus
        case 44401:
            spellInfo->procCharges = 1;
            count++;
            break;
        case 53390: // Tidal Wave
            spellInfo->procCharges = 2;
            count++;
            break;
        case 28200:    // Ascendance (Talisman of Ascendance trinket)
            spellInfo->procCharges = 6;
            count++;
            break;
        case 47201:    // Everlasting Affliction
        case 47202:
        case 47203:
        case 47204:
        case 47205:
            // add corruption to affected spells
            spellInfo->EffectSpellClassMask[1][0] |= 2;
            count++;
            break;
        case 49305:
            spellInfo->EffectImplicitTargetB[0] = 1;
            count++;
            break;
        case 76665:    // Little Big Flame Breath
        case 93667:
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_AREA_PATH;
            spellInfo->EffectImplicitTargetB[0] = TARGET_UNIT_AREA_PATH;
            spellInfo->EffectRadiusIndex[0] = 13;
            spellInfo->AttributesEx |= SPELL_ATTR0_NEGATIVE_1;
            break;
        case 51852:    // The Eye of Acherus (no spawn in phase 2 in db)
            spellInfo->EffectMiscValue[0] |= 1;
            count++;
            break;
        case 52025:    // Cleansing Totem Effect
            spellInfo->EffectDieSides[1] = 1;
            count++;
            break;
        case 51904:     // Summon Ghouls On Scarlet Crusade (core does not know the triggered spell is summon spell)
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_CASTER;
            count++;
            break;
        case 29809:     // Desecration Arm - 36 instead of 37 - typo? :/
            spellInfo->EffectRadiusIndex[0] = 37;
            count++;
            break;
        // Master Shapeshifter: missing stance data for forms other than bear - bear version has correct data
        // To prevent aura staying on target after talent unlearned
        case 48420:
            spellInfo->Stances = 1 << (FORM_CAT - 1);
            count++;
            break;
        case 48421:
            spellInfo->Stances = 1 << (FORM_MOONKIN - 1);
            count++;
            break;
        case 48422:
            spellInfo->Stances = 1 << (FORM_TREE - 1);
            count++;
            break;
        case 47569: // Improved Shadowform (Rank 1)
            // with this spell atrribute aura can be stacked several times
            spellInfo->Attributes &= ~SPELL_ATTR0_NOT_SHAPESHIFT;
            ++count;
            break;
        case 8145: // Tremor Totem (instant pulse)
        case 6474: // Earthbind Totem (instant pulse)
            spellInfo->AttributesEx5 |= SPELL_ATTR5_START_PERIODIC_AT_APPLY;
            count++;
            break;
        case 30421:     // Nether Portal - Perseverence
            spellInfo->EffectBasePoints[2] += 30000;
            count++;
            break;
        case 42650: // Army of the Dead - can be interrupted
            spellInfo->InterruptFlags = SPELL_INTERRUPT_FLAG_INTERRUPT;
            count++;
            break;
        // interruptable npc abilities:
        case 75823:             // Dark Command
        case 76813: case 91437: // Healing Wave
        case 82362: case 87374: // Shadow Strike
        case 82632:             // Rising Flames
        case 88357: case 93988: // Lightning Blast
        case 82752: case 92509: // Hydro Lance
        case 92510: case 92511:
        case 83718: case 92541: // Harden Skin
        case 92542: case 92543:
            spellInfo->InterruptFlags |= SPELL_INTERRUPT_FLAG_INTERRUPT;
            count++;
            break;
        case 61607: // Mark of Blood
            spellInfo->AttributesEx |= SPELL_ATTR1_NO_THREAT;
            ++count;
            break;
        // some dummy spell only has dest, should push caster in this case
        case 62324: // Throw Passenger
            spellInfo->Targets |= TARGET_FLAG_UNIT_CASTER;
            count++;
            break;
        case 16834: // Natural shapeshifter
        case 16835:
            spellInfo->DurationIndex = 21;
            count++;
            break;
        case 65142: // Ebon Plague
            spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
            spellInfo->SpellFamilyFlags[2] = 0x10;
            spellInfo->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            spellInfo->EffectApplyAuraName[1] = SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN;
            spellInfo->EffectMiscValue[1] = SPELL_SCHOOL_MASK_MAGIC;
            spellInfo->EffectBasePoints[1] = 8;
            count++;
            break;
        case 41013:     // Parasitic Shadowfiend Passive
            spellInfo->EffectApplyAuraName[0] = 4; // proc debuff, and summon infinite fiends
            count++;
            break;
        case 27892:     // To Anchor 1
        case 27928:     // To Anchor 1
        case 27935:     // To Anchor 1
        case 27915:     // Anchor to Skulls
        case 27931:     // Anchor to Skulls
        case 27937:     // Anchor to Skulls
            spellInfo->rangeIndex = 13;
            count++;
            break;
        case 48743: // Death Pact
            spellInfo->AttributesEx &= ~SPELL_ATTR1_CANT_TARGET_SELF;
            count++;
            break;
        // target allys instead of enemies, target A is src_caster, spells with effect like that have ally target
        // this is the only known exception, probably just wrong data
        case 29214: // Wrath of the Plaguebringer
        case 54836: // Wrath of the Plaguebringer
            spellInfo->EffectImplicitTargetB[0] = TARGET_UNIT_AREA_ALLY_SRC;
            spellInfo->EffectImplicitTargetB[1] = TARGET_UNIT_AREA_ALLY_SRC;
            count++;
            break;
        case 31687: // Summon Water Elemental
            // 322-330 switch - effect changed to dummy, target entry not changed in client:(
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_CASTER;
            count++;
            break;
        case 31228: // Cheat Death (rank 1)
        case 31229: // Cheat Death (rank 2)
        case 31230: // Cheat Death (rank 3)
            spellInfo->SpellCooldownsId = 398;
            break;
        case 1953: // Blink
            spellInfo->DurationIndex = 37;
            break;
        case 25771: // Forbearance - wrong mechanic immunity in DBC since 3.0.x
            spellInfo->EffectMiscValue[0] = MECHANIC_IMMUNE_SHIELD;
            count++;
            break;
        case 64321: // Potent Pheromones
            // spell should dispel area aura, but doesn't have the attribute
            // may be db data bug, or blizz may keep reapplying area auras every update with checking immunity
            // that will be clear if we get more spells with problem like this
            spellInfo->AttributesEx |= SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY;
            count++;
            break;
        case 69055:     // Saber Lash
        case 70814:     // Saber Lash
            spellInfo->EffectRadiusIndex[0] = 8;
            count++;
            break;
        case 69075:     // Bone Storm
        case 70834:     // Bone Storm
        case 70835:     // Bone Storm
        case 70836:     // Bone Storm
            spellInfo->EffectRadiusIndex[0] = 12;
            count++;
            break;
        case 18500: // Wing Buffet
        case 33086: // Wild Bite
        case 49749: // Piercing Blow
        case 52890: // Penetrating Strike
        case 53454: // Impale
        case 59446: // Impale
        case 62383: // Shatter
        case 64777: // Machine Gun
        case 65239: // Machine Gun
        case 65919: // Impale
        case 67858: // Impale
        case 67859: // Impale
        case 67860: // Impale
        case 69293: // Wing Buffet
        case 74439: // Machine Gun
            mSpellCustomAttr[i] |= SPELL_ATTR0_CU_IGNORE_ARMOR;
            count++;
            break;
        // Strength of the Pack
        case 64381:
            spellInfo->StackAmount = 4;
            count++;
            break;
        // THESE SPELLS ARE WORKING CORRECTLY EVEN WITHOUT THIS HACK
        // THE ONLY REASON ITS HERE IS THAT CURRENT GRID SYSTEM
        // DOES NOT ALLOW FAR OBJECT SELECTION (dist > 333)
        case 70781: // Light's Hammer Teleport
        case 70856: // Oratory of the Damned Teleport
        case 70857: // Rampart of Skulls Teleport
        case 70858: // Deathbringer's Rise Teleport
        case 70859: // Upper Spire Teleport
        case 70860: // Frozen Throne Teleport
        case 70861: // Sindragosa's Lair Teleport
            spellInfo->EffectImplicitTargetA[0] = TARGET_DST_DB;
            count++;
            break;
        // Deathbringer Saurfang achievement (must be cast on players, cannot do that with ENTRY target)
        case 72928:
            spellInfo->EffectImplicitTargetB[0] = TARGET_UNIT_AREA_ENEMY_SRC;
            count++;
            break;
        case 63675: // Improved Devouring Plague
        case 83853: // Combustion
        case 12654: // Ignite
            spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_DONE_BONUS;
            count++;
            break;
        case 33206: // Pain Suppression
            spellInfo->AttributesEx5 &= ~SPELL_ATTR5_USABLE_WHILE_STUNNED;
            count++;
            break;
        // this is here until targetAuraSpell and alike support SpellDifficulty.dbc
        case 70459: // Ooze Eruption Search Effect
            spellInfo->targetAuraSpell = 0;
            count++;
            break;
        case 70728: // Exploit Weakness
        case 70840: // Devious Minds
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_CASTER;
            spellInfo->EffectImplicitTargetB[0] = TARGET_UNIT_PET;
            count++;
            break;
        case 70893: // Culling The Herd
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_CASTER;
            spellInfo->EffectImplicitTargetB[0] = TARGET_UNIT_MASTER;
            count++;
            break;
        case 71413: // Green Ooze Summon
        case 71414: // Orange Ooze Summon
            spellInfo->EffectImplicitTargetA[0] = TARGET_DEST_DEST;
            count++;
            break;
        // THIS IS HERE BECAUSE COOLDOWN ON CREATURE PROCS IS NOT IMPLEMENTED
        case 71604: // Mutated Strength
        case 72673: // Mutated Strength
        case 72674: // Mutated Strength
        case 72675: // Mutated Strength
            spellInfo->Effect[1] = 0;
            count++;
            break;
        case 70447: // Volatile Ooze Adhesive
        case 72836: // Volatile Ooze Adhesive
        case 72837: // Volatile Ooze Adhesive
        case 72838: // Volatile Ooze Adhesive
        case 70672: // Gaseous Bloat
        case 72455: // Gaseous Bloat
        case 72832: // Gaseous Bloat
        case 72833: // Gaseous Bloat
            spellInfo->EffectImplicitTargetB[0] = TARGET_UNIT_TARGET_ENEMY;
            spellInfo->EffectImplicitTargetB[1] = TARGET_UNIT_TARGET_ENEMY;
            spellInfo->EffectImplicitTargetB[2] = TARGET_UNIT_TARGET_ENEMY;
            count++;
            break;
        case 70911: // Unbound Plague
        case 72854: // Unbound Plague
        case 72855: // Unbound Plague
        case 72856: // Unbound Plague
            spellInfo->EffectImplicitTargetB[0] = TARGET_UNIT_TARGET_ENEMY;
            count++;
            break;
        case 71708: // Empowered Flare
        case 72785: // Empowered Flare
        case 72786: // Empowered Flare
        case 72787: // Empowered Flare
            spellInfo->AttributesEx3 |= SPELL_ATTR3_NO_DONE_BONUS;
            count++;
            break;
        case 44614: // Frostfire Bolt
            spellInfo->StackAmount = 0; //TODO: remove when stacking of Decrease Run Speed % aura is fixed
            count++;
            break;
        case 15237: // Holy Nova
        case 23455: // Holy Nova - heal
            spellInfo->EffectRadiusIndex[0] = 13;
            count++;
            break;
        case 87904:  // Al'akir:
        case 101458: // Feedback
        case 101459: // 4 difficulty entries
        case 101460:
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_CASTER;
            count++;
            break;
        case 87770: // Al'akir:
        case 93261: // Wind Burst
        case 93262: // 4 difficulty entries
        case 93263:
            spellInfo->PreventionType = SPELL_PREVENTION_TYPE_NONE; // remove SPELL_PREVENTION_TYPE_SILENCE
            count++;
            break;
        case 88835: // Conclave of Wind kill credit
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_CASTER;
            spellInfo->EffectImplicitTargetB[0] = TARGET_NONE;
            count++;
            break;
        case 96872: // Occu'thar Focused Fire ability, triggers summoning spell
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_TARGET_ENEMY;
            spellInfo->EffectImplicitTargetB[0] = TARGET_NONE;
            count++;
            break;
        case 74434: // Soulburn
            spellInfo->procCharges = 1;
            count++;
            break;
        case 85547:// Jinx: Curse of the Elements
        case 86105:
            spellInfo->excludeTargetAuraSpell = 1490; // Dont affect targets with original CoE
            spellInfo->DurationIndex = 5; // 30 s same as original CoE
            spellInfo->MaxAffectedTargets = 15;
           break;
        case 23126: // World Enlarger
            spellInfo->AuraInterruptFlags |= AURA_INTERRUPT_FLAG_SPELL_ATTACK;
            count++;
            break;
        case 19975: // Entangling Roots -- Nature's Grasp Proc
            spellInfo->CastingTimeIndex = 1;
            count++;
            break;
        case 18395: // Dismounting Shot
            spellInfo->rangeIndex = 13;    /* unlimited range */
            spellInfo->DurationIndex = 1;  /* 10s duration */
            spellInfo->Effect[1] = 0;      /* no stun */
            count++;
            break;
        case 95529: // Bounce Achievement Aura
            spellInfo->StackAmount = 20;
            count++;
            break;
        case 24347: // Master Angler
            // Because of bug in DBC, we need to set MiscValue manually
            spellInfo->EffectMiscValue[0] = 11374; // Changing fish instead pig :D
            count++;
            break;
        case 43681: // Inactive (Spell for AFK Reporter)
            spellInfo->AttributesEx |= SPELL_ATTR1_CANT_BE_REFLECTED;
            count++;
            break;
        case 79361: // Twilight Phoenix (changing model from Twilight Phoenix to Dark Phoenix)
            spellInfo->EffectMiscValue[0] = 47841; // Dark Phoenix models
            count++;
            break;
        case 54726: // Winged Steed of the Ebon Blade
            spellInfo->EffectMiscValue[0] = 119965;// used custom model
            break;
        case 54727: // Winged Steed of the Ebon Blade
            spellInfo->EffectMiscValue[0] = 119964; // used custom model
            break;
        case 47037: // Swift War Elekk
            spellInfo->EffectMiscValue[0] = 119966; // used custom model
            break;
        case 18363: // Riding Kodo
            spellInfo->EffectMiscValue[0] = 119967; // used custom model
            break;
        case 121805: // RAF Mount III
            spellInfo->EffectMiscValue[0] = 119968; // used custom model
            break;
        case 55293: // Amani War Bear
            spellInfo->EffectMiscValue[0] = 55293; // used custom model
            break;
        case 89485: // Inner Focus
            spellInfo->procCharges = 1; // only one charge
            count++;
            break;
        case 74854: // Blazzing Hippogryph
            spellInfo->EffectMiscValue[0] = 29767;
            spellInfo->CastingTimeIndex = 16; // 1,5s casting time
            break;
        case 74855: // Blazing Hippogryph
            spellInfo->EffectMiscValue[0] = 4862;
            spellInfo->CastingTimeIndex = 16; // 1,5s casting time
            break;
        case 88172: // Faction Override -> Horde
        case 88174: // Faction Override -> Alliance
            spellInfo->Attributes |= SPELL_ATTR0_CANT_CANCEL;
            spellInfo->AttributesEx3 |= SPELL_ATTR3_DEATH_PERSISTENT;
            break;
        case 89912: // Chakra Flow (Priest T11 4p bonus)
            spellInfo->Attributes |= SPELL_ATTR0_CANT_CANCEL; // active during Chakra state - do not remove by right-click
            break;
        case 43810: // Frost Wyrm
            spellInfo->Attributes |= SPELL_ATTR0_NOT_SHAPESHIFT | SPELL_ATTR0_CANT_USED_IN_COMBAT | SPELL_ATTR0_UNK18;
            count++;
            break;
        case 63531: //  Retribution Aura Overflow ( Communion ) retribution paladin talent, missing basepoint
            spellInfo->EffectBasePoints[1] = 3;
            break;
        case 82415: // Dampening Wave
        case 92650: // Dampening Wave (heroic difficulty)
            // because of bug in dbc
            spellInfo->EffectRadiusIndex[0] = 48; // 60 yards
            spellInfo->EffectRadiusIndex[1] = 48; // 60 yards
            break;
        case 81828: // Thrashing Charge
        case 92651: // Thrashing Charge (heroic difficulty)
            // because bug of dbc we must set corrected target manually
            spellInfo->EffectRadiusIndex[0] = 18; // 10 yards
            spellInfo->EffectRadiusIndex[1] = 18; // 10 yards
            spellInfo->CastingTimeIndex = 1; // instant cast
            break;
        case 81629: // Submerge
            spellInfo->EffectTriggerSpell[0] = 0; // Summon effect, summoned in AI
            break;
        case 81008: // Quake
        case 92631: // Quake (Heroic difficulty)
            // because of bug in dbc
            spellInfo->EffectRadiusIndex[0] = 31; // 80 yards
            break;
       case 82699: // Water bomb
            spellInfo->EffectImplicitTargetA[0] = TARGET_SRC_CASTER;
            spellInfo->EffectImplicitTargetB[0] = TARGET_UNIT_AREA_ALLY_SRC;
            spellInfo->EffectRadiusIndex[0] = 22;
            break;
        case 82746: // Glaciate
        case 92506:
        case 92507:
        case 92508:
            spellInfo->EffectRadiusIndex[0] = 13; // 10 yardov ???
            break;
        case 82700: // Water bomb
        case 88579: // Inferno rush aoe
        case 82860:
            spellInfo->AttributesEx5 |= SPELL_ATTR5_USABLE_WHILE_STUNNED;
            break;
        case 92212: // Flamestrike ( Ignacious pure visual)
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_CASTER;
            spellInfo->EffectImplicitTargetB[0] = TARGET_NONE;
            break;
        case 92214:  // Flamestrike aoe
            spellInfo->EffectRadiusIndex[0] = 29;
            break;
        case 92270: // Summon effekt u Frozen orbu
            spellInfo->Effect[0] = 0;
            break;
        case 97318: // Plucked
            spellInfo->Effect[0] = 0;
            spellInfo->EffectImplicitTargetA[1] = TARGET_UNIT_TARGET_ENEMY;
            spellInfo->AttributesEx &= ~SPELL_ATTR1_UNK26;
            break;
        case 93362: // Flamestrike summon effekt
        case 93383:
            spellInfo->Effect[1] = 0;
            break;
        case 92303: // Zvysovanie rychlosti u Frozen orbu
            spellInfo->Effect[1] = 0;
            break;
        case 84915: // Liquid ice
        case 92497:
        case 92498:
        case 92499:
            spellInfo->EffectRadiusIndex[0] =15; 
            break;
        case 83070: // Lightning blast -  Zmena z aoe na direct dmg spell
        case 92454:
        case 92455:
        case 92456:
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_TARGET_ENEMY;
            spellInfo->EffectImplicitTargetB[0] = TARGET_NONE;
            spellInfo->Effect[1] = 0;// vypnem effekt 1
            break;
        case 83565: // Quake
        case 92544:
        case 92545:
        case 92546:
            spellInfo->excludeTargetAuraSpell=83500; // Ak ma hrac "levitate" debuff tak sa mu nic nestane
            spellInfo->EffectRadiusIndex[0] = 22;
            break;
        case 83067: //Thundershock
        case 92469:
        case 92470:
        case 92471:
            spellInfo->excludeTargetAuraSpell=83581; // Ak ma hrac grounded debuff tak sa mu nic nestane
            spellInfo->EffectRadiusIndex[0] = 22;
            break;
        case 92548: // Instant glaciate u Frozen orbu
            spellInfo->EffectRadiusIndex[0] = 22;
            break;
        case 84948: // Gravity crush ( na 10 mane maju affektovat iba 1 hraca )
        case 92487:
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_TARGET_ENEMY;
            spellInfo->EffectImplicitTargetB[0] = TARGET_NONE;
            spellInfo->EffectImplicitTargetA[1] = TARGET_UNIT_TARGET_ENEMY;
            spellInfo->EffectImplicitTargetB[1] = TARGET_NONE;
            spellInfo->Effect[2] = 0;// vypnem effekt 2 triggerroval neexistujuci spell v DB
            break;
        case 92486: //Gravity crush 25 man ( nerfli to z 5 na 3 ) Takze skusime 4 ak to bude prilis hard da sa to spat na 3 :D
        case 92488:
            spellInfo->Effect[2] = 0;// vypnem effekt 2 triggerroval neexistujuci spell v DB
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_TARGET_ENEMY;
            spellInfo->EffectImplicitTargetB[0] = TARGET_NONE;
            spellInfo->EffectImplicitTargetA[1] = TARGET_UNIT_TARGET_ENEMY;
            spellInfo->EffectImplicitTargetB[1] = TARGET_NONE;
            spellInfo->MaxAffectedTargets = 3;
            spellInfo->EffectChainTarget[0] = 3; 
            spellInfo->EffectRadiusIndex[0] = 22; 
            spellInfo->EffectChainTarget[1] = 3; 
            spellInfo->EffectRadiusIndex[1] = 22;
            break;
        case 92076: // Gravity core chybajuci radius index
        case 92537:
        case 92538:
        case 92539:
            spellInfo->EffectRadiusIndex[1] = 13; // 10 yardov
            break;
        case 92075:
            spellInfo->rangeIndex=6; // 100 yards
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_TARGET_ENEMY;
            spellInfo->EffectImplicitTargetB[0] = TARGET_NONE;
            spellInfo->EffectRadiusIndex[1] = 13; // 10 yardov
            spellInfo->DurationIndex = 1;
            break;
        case 92067: // Static overload
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_TARGET_ENEMY;
            spellInfo->EffectImplicitTargetB[0] = TARGET_NONE;
            spellInfo->EffectRadiusIndex[1] = 13; // 10 yardov
            spellInfo->DurationIndex = 1;
            break;
        case 84529: // Electric Instability by mal ignorovat resist hracov
        case 92480:
        case 92481:
        case 92482:
            spellInfo->EffectRadiusIndex[0] = 84;  // 8 yardov
            break;
        case 86838: // Impending dooooom
            spellInfo->EffectTriggerSpell[0] = 0;
            spellInfo->DurationIndex = 21; // unlimited duration
            break;
        case 81441: // Shadowfury
        case 92644: // Shadowfury (heroic difficulty)
            spellInfo->EffectRadiusIndex[0] = 13; // 10 yards
            spellInfo->EffectRadiusIndex[1] = 13;
            break;
        case 81440: // Frostbolt Volley
        case 92642: // Frostbolt Volley (heroic difficulty)
            spellInfo->EffectRadiusIndex[0] = 23; // 40 yards
            spellInfo->EffectRadiusIndex[1] = 23;
            break;
        case 81569: // Spining Slash
        case 92623: // Spining Slash (heroic difficulty)
            spellInfo->EffectRadiusIndex[0] = 8; // 5 yards
            break;
        case 81508: // Dust Storm
        case 92624: // Dust Storm (heoric difficulty)
            spellInfo->EffectRadiusIndex[0] = 11; // 40 yards
            spellInfo->EffectRadiusIndex[1] = 11; // 40 yards
            spellInfo->EffectRadiusIndex[2] = 11; // 40 yards
            break;
        case 81272: // Electrocute radius index ( Nefarian Encounter in BWD )
        case 94088:
        case 94089:
        case 94090:
            spellInfo->EffectRadiusIndex[0] = 22;
            break;
        case 77987: // Growth Catalyst
        case 101440: // + difficulty entries
        case 101441:
        case 101442:
            spellInfo->AttributesEx3 |= SPELL_ATTR3_STACK_FOR_DIFF_CASTERS;
            spellInfo->Effect[2] = 0;
            break;
        case 78095: // Magma Jets + difficulty entries
        case 93014:
        case 93015:
        case 93016:
            spellInfo->EffectRadiusIndex[0] = 15; // 3yd
            spellInfo->EffectRadiusIndex[1] = 13; // 10yd
            break;
        case 99517: // Countdown
        case 99489:
            mSpellCustomAttr[i] |= SPELL_ATTR0_CU_EXCLUDE_SELF;
            spellInfo->EffectRadiusIndex[0] = 7; // 2 yd
            break;
        case 99353: // Decimating Strike ( Baleroc )
            spellInfo->AttributesEx4 |= SPELL_ATTR4_IGNORE_RESISTANCES;
            break;
        case 99842: // Magma Rupture
        case 101205: // + difficulty entries
        case 101206:
        case 101207:
            spellInfo->EffectRadiusIndex[0] = 15;
            break;
        case 100002: // Hurl Spear
        case 99840: // Magma Rupture (Shannox' spell)
            spellInfo->EffectRadiusIndex[1] = 11; // 45yd
            break;
        case 99945: // Handled in AI
            spellInfo->EffectTriggerSpell[0] = 0;
            break;
        case 51466: // Elemental Oath (Rank 1)
        case 51470: // Elemental Oath (Rank 2)
            spellInfo->Effect[1] = SPELL_EFFECT_APPLY_AURA;
            spellInfo->EffectApplyAuraName[1] = SPELL_AURA_ADD_FLAT_MODIFIER;
            spellInfo->EffectMiscValue[1] = SPELLMOD_EFFECT2;
            spellInfo->EffectSpellClassMask[1] = flag96(0x00000000, 0x00004000, 0x00000000);
            break;
        case 56222: // Dark command
        case 6795:  // Growl
        case 20736: // Distracting shot
        case 62124: // Hand of Reckoning
        case 355:   // Taunt
        case 21008: // Mocking blow
            spellInfo->Attributes |= SPELL_ATTR0_IMPOSSIBLE_DODGE_PARRY_BLOCK;
            count++;
            break;
        case 51723: //Fan of Knives
        case 26679: //Deadly Throw
            spellInfo->excludeCasterAuraSpell=0;
            count++;
            break;
        case 96466: // Whispers of Hethiss
            spellInfo->EffectImplicitTargetA[1] = TARGET_UNIT_TARGET_ENEMY;
            break;
        case 96560: // Word of Hethiss
             spellInfo->EffectRadiusIndex[0] = 13; // 10 yd
             spellInfo->EffectRadiusIndex[1] = 13; // 10 yd
             break;
        case 96685: // Venomous Effusion
             spellInfo->EffectRadiusIndex[0] = 7; // 2 yd
             break;
        case 96521: // Pool of Acrid Tears
        case 97089:
            spellInfo->EffectRadiusIndex[0] = 26; // 4 yd
            break;
        case 96755: // Pool of Arcid Tears (again)
        case 97085:
            spellInfo->EffectRadiusIndex[0] = 13; // 10 yd
            break;
        case 96335: // Zanzil's Graveyard Gas
        case 96434:
            spellInfo->EffectImplicitTargetA[0] = TARGET_SRC_CASTER;
            spellInfo->EffectImplicitTargetB[0] = TARGET_UNIT_AREA_ENEMY_SRC;
            spellInfo->EffectImplicitTargetA[1] = TARGET_SRC_CASTER;
            spellInfo->EffectImplicitTargetB[1] = TARGET_UNIT_AREA_ENEMY_SRC;
            spellInfo->EffectRadiusIndex[0] = 22; // 200 yd
            spellInfo->EffectRadiusIndex[1] = 22; // 200 yd
            spellInfo->AttributesEx3 &= ~SPELL_ATTR3_DEATH_PERSISTENT;
            break;
        case 97016: // Big Bad Voodoo
            spellInfo->EffectRadiusIndex[0] = 66; // 100 yd
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_AREA_ALLY_SRC;
            break;
        case 77569: // Release Aberration
            spellInfo->EffectRadiusIndex[0] = 12;
            break;
        case 82848: // Massacre
            spellInfo->EffectRadiusIndex[0] = 28;
            break;
        case 91307: // Mocking Shadows
            mSpellCustomAttr[i] |= SPELL_ATTR0_CU_NEGATIVE_EFF0;
            break;
        case 82881: // Break
        case 82890: // Mortality
            mSpellCustomAttr[i] |= SPELL_ATTR0_CU_NEGATIVE_EFF0;
            mSpellCustomAttr[i] |= SPELL_ATTR0_CU_NEGATIVE_EFF1;
            break;
        case 96644: // Thousand Blades
        case 96645:
            spellInfo->EffectRadiusIndex[0] = 21; // 35 yd
            break;
        case 83154: // Piercing Chill
            spellInfo->EffectRadiusIndex[0] = 9; // 20 yd
            spellInfo->EffectRadiusIndex[1] = 9; // 20 yd
            break;
        case 30213: // Legion Strike
            spellInfo->EffectRadiusIndex[1] = 29; // 6 yd
            break;
        case 95799: // Empowered shadow
            spellInfo->EffectApplyAuraName[0] = SPELL_AURA_DUMMY; // disable applying spell modifier for direct damage
            count++;
            break;
        case 82691: // Ring of Frost
            spellInfo->EffectImplicitTargetA[0] = TARGET_UNIT_TARGET_ENEMY; // disable spreading of freeze from frozen target to his allies
            spellInfo->EffectImplicitTargetB[0] = TARGET_NONE;
            count++;
            break;
        case 82772: // Frozen (council)
        case 92503:
        case 92504:
        case 92505:
        case 98229: // Concentration ( Majordomo Staghelm HC)
            spellInfo->procFlags = 0;
            break;
        case 93495:// Wake
            mSpellCustomAttr[i] |= SPELL_ATTR0_CU_CONE_BACK;
            spellInfo->EffectImplicitTargetB[0] = TARGET_UNIT_CONE_ENEMY; 
            spellInfo->EffectImplicitTargetB[1] = TARGET_UNIT_CONE_ENEMY; 
            count++;
            break;
        case 83703: // Shadow Nova (Halfus Wyrmbreaker)
            spellInfo->CastingTimeIndex = 2;
            count++;
            break;
        case 83487: // Chains (Halfus Wyrmbreaker)
            spellInfo->AttributesEx |= SPELL_ATTR1_STACK_FOR_DIFF_CASTERS;
            count++;
            break;
        case 77758: // Thrash
        case 779:   // Swipe
            spellInfo->EffectBonusCoefficient[0] = 0;   // no spell power bonus
            count++;
            break;

/***************** SINESTRA ***************************************/
        case 92958:// Twilight Pulse
        case 92959:
            spellInfo->AttributesEx3 |= SPELL_ATTR3_PLAYERS_ONLY;
            spellInfo->EffectRadiusIndex[0] = 77; // 6 yards
            break;
        case 92851: // Twilight Slicer
            spellInfo->EffectTriggerSpell[1] = 0;
            break;
        case 95822: // Twilight flames
            spellInfo->AttributesEx3 |= SPELL_ATTR3_PLAYERS_ONLY;
            break;
        case 87229 : // Fiery barrier summon effekt
            spellInfo->Effect[1] = 0;
            break;
        case 87655: // Purple beam
        case 35371: // White beam
            spellInfo->AttributesEx |= SPELL_ATTR1_STACK_FOR_DIFF_CASTERS;
        break;
        case 90045: // Indomitable
        case 92946:
            spellInfo->AttributesEx3 |= SPELL_ATTR3_PLAYERS_ONLY;
            break;
        case 90028: // Unleash essence
        case 92947:
            spellInfo->AttributesEx3 |= SPELL_ATTR3_PLAYERS_ONLY;
            break;
        case 89284: // Twilight essence (visual)
            spellInfo->EffectTriggerSpell[0] = 0;
            break;
        case 88146: // Twilight essence aoe dmg
        case 92950:
            spellInfo->EffectRadiusIndex[0] = 26; // 4 y
            spellInfo->EffectRadiusIndex[1] = 26; // 4 y
            spellInfo->AttributesEx3 |= SPELL_ATTR3_PLAYERS_ONLY;
            break;
        case 89435: // Aoe Wracks
        case 92956:
            spellInfo->EffectRadiusIndex[0] = 22; // 200 yards
            spellInfo->EffectRadiusIndex[1] = 22;
            break;

        // Beth'tilac
        case 99219: // Sticky Webbing
            spellInfo->EffectAmplitude[0] = 500; // Lowered, cause players sometimes fall on the ground
            break;
        case 99223: // Sticky Webbing ( triggered)
            spellInfo->DurationIndex = 66;  // 2.5s
            spellInfo->AttributesEx |= SPELL_ATTR1_STACK_FOR_DIFF_CASTERS;
            spellInfo->AttributesEx2 &= ~(uint32)SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS; // don't ignore LoS
            break;
        case 99506: // The Widow's Kiss
            mSpellCustomAttr[i] |= SPELL_ATTR0_CU_EXCLUDE_SELF; // ignore the owner of aura
            break;
        case 98471: // Burning Acid
        case 100826:
        case 100827:
        case 100828:
            spellInfo->AttributesEx2 &= ~(uint32)SPELL_ATTR2_CAN_TARGET_NOT_IN_LOS; // don't ignore LoS
            break;

        //Deatbringer Saurfang
        case 7202: // Blood link
            spellInfo->EffectApplyAuraName[0]=SPELL_EFFECT_DUMMY;
            break;

        case 88314: // Twisting Winds
            spellInfo->EffectRadiusIndex[0] = 16; // 3 yd
            spellInfo->EffectRadiusIndex[1] = 16; // 3 yd
            break;
        case 89133: // Solar Fire (Normal)
        case 89878: // Solar Fire (Heroic)
            spellInfo->EffectRadiusIndex[0] = 7; // 2 yd
            break;
        case 74108: // Solar Winds (Normal)
        case 89130: // Solar Winds (Heroic)
            spellInfo->Effect[1] = 0; // Disable second effect
            break;
        case 93564: // Pistol Barrage (Normal)
        case 93784: // Pistol Barrage (Heroic)
            spellInfo->Effect[1] = 0; // Disable second effect
            break;
        default:
            break;
        }


        switch(spellInfo->SpellFamilyName)
        {
            case SPELLFAMILY_DRUID:
                // Starfall Target Selection
                if (spellInfo->SpellFamilyFlags[2] & 0x100)
                    spellInfo->MaxAffectedTargets = 2;
                // Starfall AOE Damage
                else if (spellInfo->SpellFamilyFlags[2] & 0x800000)
                    mSpellCustomAttr[i] |= SPELL_ATTR0_CU_EXCLUDE_SELF;
                else
                    break;
                count++;
                break;
                // Do not allow Deadly throw to proc twice
            case SPELLFAMILY_ROGUE:
                if (spellInfo->SpellFamilyFlags[1] & 0x1)
                    spellInfo->AttributesEx4 |= SPELL_ATTR4_CANT_PROC_FROM_SELFCAST;
                else
                    break;
                count++;
                break;
            case SPELLFAMILY_PALADIN:
                // Seals of the Pure should affect Seal of Righteousness
                if (spellInfo->SpellIconID == 25 && spellInfo->Attributes & SPELL_ATTR0_PASSIVE)
                    spellInfo->EffectSpellClassMask[0][1] |= 0x20000000;
                else
                    break;
                count++;
                break;
            case SPELLFAMILY_DEATHKNIGHT:
                // Icy Touch - extend FamilyFlags (unused value) for Sigil of the Frozen Conscience to use
                if (spellInfo->SpellIconID == 2721 && spellInfo->SpellFamilyFlags[0] & 0x2)
                    spellInfo->SpellFamilyFlags[0] |= 0x40;
                count++;
                break;
        }
    }

    SummonPropertiesEntry *properties = const_cast<SummonPropertiesEntry*>(sSummonPropertiesStore.LookupEntry(121));
    properties->Type = SUMMON_TYPE_TOTEM;
    properties = const_cast<SummonPropertiesEntry*>(sSummonPropertiesStore.LookupEntry(647)); // 52893
    properties->Type = SUMMON_TYPE_TOTEM;

    CreatureAI::FillAISpellInfo();

    sLog->outString();
    sLog->outString(">> Loaded %u custom spell attributes", count);
}

// Fill custom data about enchancments
void SpellMgr::LoadEnchantCustomAttr()
{
    uint32 size = sSpellItemEnchantmentStore.GetNumRows();
    mEnchantCustomAttr.resize(size);

    uint32 count = 0;

    for (uint32 i = 0; i < size; ++i)
       mEnchantCustomAttr[i] = NULL;

    for (uint32 i = 0; i < GetSpellStore()->GetNumRows(); ++i)
    {
        SpellEntry * spellInfo = (SpellEntry*)GetSpellStore()->LookupEntry(i);
        if (!spellInfo)
            continue;

        // only rogue's poisons and shaman's weapon enchant
        if (spellInfo->SpellFamilyName != SPELLFAMILY_ROGUE && spellInfo->SpellFamilyName != SPELLFAMILY_SHAMAN)
            continue;

        for (uint32 j = 0; j < MAX_SPELL_EFFECTS; ++j)
        {
            if (spellInfo->Effect[j] == SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY)
            {
                uint32 enchId = spellInfo->EffectMiscValue[j];
                SpellItemEnchantmentEntry const *ench = sSpellItemEnchantmentStore.LookupEntry(enchId);
                if (!ench)
                    continue;

                // create structure
                if (!mEnchantCustomAttr[enchId])
                {
                    mEnchantCustomAttr[enchId] = new EnchantCustomProperties;
                    mEnchantCustomAttr[enchId]->classOrigins = 0;
                    mEnchantCustomAttr[enchId]->allowedInBattleground = true;
                }

                // add class, who can cast that temp enchantment, to classOrigins bitfield
                if (spellInfo->SpellFamilyName < 16)
                    mEnchantCustomAttr[enchId]->classOrigins |= 1 << (GetClassFromSpellFamily(spellInfo->SpellFamilyName));

                count++;
                break;
            }
        }
    }

    sLog->outString();
    sLog->outString(">> Loaded %u custom enchant attributes", count);
}

void SpellMgr::LoadSpellLinked()
{
    mSpellLinkedMap.clear();    // need for reload case
    uint32 count = 0;

    //                                               0              1             2
    QueryResult result = WorldDatabase.Query("SELECT spell_trigger, spell_effect, type FROM spell_linked_spell");
    if (!result)
    {
        sLog->outString();
        sLog->outString(">> Loaded %u linked spells", count);
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        int32 trigger = fields[0].GetInt32();
        int32 effect =  fields[1].GetInt32();
        int32 type =    fields[2].GetInt32();

        SpellEntry const* spellInfo = sSpellStore.LookupEntry(abs(trigger));
        if (!spellInfo)
        {
            sLog->outErrorDb("Spell %u listed in `spell_linked_spell` does not exist", abs(trigger));
            continue;
        }
        spellInfo = sSpellStore.LookupEntry(abs(effect));
        if (!spellInfo)
        {
            sLog->outErrorDb("Spell %u listed in `spell_linked_spell` does not exist", abs(effect));
            continue;
        }

        if (trigger > 0)
        {
            switch(type)
            {
                case 0: mSpellCustomAttr[trigger] |= SPELL_ATTR0_CU_LINK_CAST; break;
                case 1: mSpellCustomAttr[trigger] |= SPELL_ATTR0_CU_LINK_HIT;  break;
                case 2: mSpellCustomAttr[trigger] |= SPELL_ATTR0_CU_LINK_AURA; break;
            }
        }
        else
        {
            mSpellCustomAttr[-trigger] |= SPELL_ATTR0_CU_LINK_REMOVE;
        }

        if (type) //we will find a better way when more types are needed
        {
            if (trigger > 0)
                trigger += SPELL_LINKED_MAX_SPELLS * type;
            else
                trigger -= SPELL_LINKED_MAX_SPELLS * type;
        }
        mSpellLinkedMap[trigger].push_back(effect);

        ++count;
    } while (result->NextRow());

    sLog->outString();
    sLog->outString(">> Loaded %u linked spells", count);
}
