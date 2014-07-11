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
#include "Opcodes.h"
#include "Log.h"
#include "UpdateMask.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "Chat.h"
#include "SkillExtraItems.h"
#include "Unit.h"
#include "Spell.h"
#include "DynamicObject.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "Group.h"
#include "Guild.h"
#include "UpdateData.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "SharedDefines.h"
#include "Pet.h"
#include "GameObject.h"
#include "GossipDef.h"
#include "Creature.h"
#include "Totem.h"
#include "CreatureAI.h"
#include "BattlegroundMgr.h"
#include "Battleground.h"
#include "BattlegroundEY.h"
#include "BattlegroundWS.h"
#include "BattlefieldMgr.h"
#include "Battlefield.h"
#include "BattlefieldWG.h"
#include "OutdoorPvPMgr.h"
#include "Language.h"
#include "SocialMgr.h"
#include "Util.h"
#include "VMapFactory.h"
#include "TemporarySummon.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "SkillDiscovery.h"
#include "Formulas.h"
#include "Vehicle.h"
#include "ScriptMgr.h"
#include "GameObjectAI.h"
#include "ScriptedCreature.h"
#include "MoveSpline.h"
#include "PathGenerator.h"

pEffect SpellEffects[TOTAL_SPELL_EFFECTS]=
{
    &Spell::EffectNULL,                                     //  0
    &Spell::EffectInstaKill,                                //  1 SPELL_EFFECT_INSTAKILL
    &Spell::EffectSchoolDMG,                                //  2 SPELL_EFFECT_SCHOOL_DAMAGE
    &Spell::EffectDummy,                                    //  3 SPELL_EFFECT_DUMMY
    &Spell::EffectUnused,                                   //  4 SPELL_EFFECT_PORTAL_TELEPORT          unused
    &Spell::EffectTeleportUnits,                            //  5 SPELL_EFFECT_TELEPORT_UNITS
    &Spell::EffectApplyAura,                                //  6 SPELL_EFFECT_APPLY_AURA
    &Spell::EffectEnvirinmentalDMG,                         //  7 SPELL_EFFECT_ENVIRONMENTAL_DAMAGE
    &Spell::EffectPowerDrain,                               //  8 SPELL_EFFECT_POWER_DRAIN
    &Spell::EffectHealthLeech,                              //  9 SPELL_EFFECT_HEALTH_LEECH
    &Spell::EffectHeal,                                     // 10 SPELL_EFFECT_HEAL
    &Spell::EffectBind,                                     // 11 SPELL_EFFECT_BIND
    &Spell::EffectNULL,                                     // 12 SPELL_EFFECT_PORTAL
    &Spell::EffectUnused,                                   // 13 SPELL_EFFECT_RITUAL_BASE              unused
    &Spell::EffectUnused,                                   // 14 SPELL_EFFECT_RITUAL_SPECIALIZE        unused
    &Spell::EffectUnused,                                   // 15 SPELL_EFFECT_RITUAL_ACTIVATE_PORTAL   unused
    &Spell::EffectQuestComplete,                            // 16 SPELL_EFFECT_QUEST_COMPLETE
    &Spell::EffectWeaponDmg,                                // 17 SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL
    &Spell::EffectResurrect,                                // 18 SPELL_EFFECT_RESURRECT
    &Spell::EffectAddExtraAttacks,                          // 19 SPELL_EFFECT_ADD_EXTRA_ATTACKS
    &Spell::EffectUnused,                                   // 20 SPELL_EFFECT_DODGE                    one spell: Dodge
    &Spell::EffectUnused,                                   // 21 SPELL_EFFECT_EVADE                    one spell: Evade (DND)
    &Spell::EffectParry,                                    // 22 SPELL_EFFECT_PARRY
    &Spell::EffectBlock,                                    // 23 SPELL_EFFECT_BLOCK                    one spell: Block
    &Spell::EffectCreateItem,                               // 24 SPELL_EFFECT_CREATE_ITEM
    &Spell::EffectUnused,                                   // 25 SPELL_EFFECT_WEAPON
    &Spell::EffectUnused,                                   // 26 SPELL_EFFECT_DEFENSE                  one spell: Defense
    &Spell::EffectPersistentAA,                             // 27 SPELL_EFFECT_PERSISTENT_AREA_AURA
    &Spell::EffectSummonType,                               // 28 SPELL_EFFECT_SUMMON
    &Spell::EffectLeap,                                     // 29 SPELL_EFFECT_LEAP
    &Spell::EffectEnergize,                                 // 30 SPELL_EFFECT_ENERGIZE
    &Spell::EffectWeaponDmg,                                // 31 SPELL_EFFECT_WEAPON_PERCENT_DAMAGE
    &Spell::EffectTriggerMissileSpell,                      // 32 SPELL_EFFECT_TRIGGER_MISSILE
    &Spell::EffectOpenLock,                                 // 33 SPELL_EFFECT_OPEN_LOCK
    &Spell::EffectSummonChangeItem,                         // 34 SPELL_EFFECT_SUMMON_CHANGE_ITEM
    &Spell::EffectApplyAreaAura,                            // 35 SPELL_EFFECT_APPLY_AREA_AURA_PARTY
    &Spell::EffectLearnSpell,                               // 36 SPELL_EFFECT_LEARN_SPELL
    &Spell::EffectUnused,                                   // 37 SPELL_EFFECT_SPELL_DEFENSE            one spell: SPELLDEFENSE (DND)
    &Spell::EffectDispel,                                   // 38 SPELL_EFFECT_DISPEL
    &Spell::EffectUnused,                                   // 39 SPELL_EFFECT_LANGUAGE
    &Spell::EffectDualWield,                                // 40 SPELL_EFFECT_DUAL_WIELD
    &Spell::EffectJump,                                     // 41 SPELL_EFFECT_JUMP
    &Spell::EffectJumpDest,                                 // 42 SPELL_EFFECT_JUMP_DEST
    &Spell::EffectTeleUnitsFaceCaster,                      // 43 SPELL_EFFECT_TELEPORT_UNITS_FACE_CASTER
    &Spell::EffectLearnSkill,                               // 44 SPELL_EFFECT_SKILL_STEP
    &Spell::EffectPlayMovie,                                // 45 SPELL_EFFECT_PLAY_MOVIE
    &Spell::EffectUnused,                                   // 46 SPELL_EFFECT_SPAWN clientside, unit appears as if it was just spawned
    &Spell::EffectTradeSkill,                               // 47 SPELL_EFFECT_TRADE_SKILL
    &Spell::EffectUnused,                                   // 48 SPELL_EFFECT_STEALTH                  one spell: Base Stealth
    &Spell::EffectUnused,                                   // 49 SPELL_EFFECT_DETECT                   one spell: Detect
    &Spell::EffectTransmitted,                              // 50 SPELL_EFFECT_TRANS_DOOR
    &Spell::EffectUnused,                                   // 51 SPELL_EFFECT_FORCE_CRITICAL_HIT       unused
    &Spell::EffectUnused,                                   // 52 SPELL_EFFECT_GUARANTEE_HIT            one spell: zzOLDCritical Shot
    &Spell::EffectEnchantItemPerm,                          // 53 SPELL_EFFECT_ENCHANT_ITEM
    &Spell::EffectEnchantItemTmp,                           // 54 SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY
    &Spell::EffectTameCreature,                             // 55 SPELL_EFFECT_TAMECREATURE
    &Spell::EffectSummonPet,                                // 56 SPELL_EFFECT_SUMMON_PET
    &Spell::EffectLearnPetSpell,                            // 57 SPELL_EFFECT_LEARN_PET_SPELL
    &Spell::EffectWeaponDmg,                                // 58 SPELL_EFFECT_WEAPON_DAMAGE
    &Spell::EffectCreateRandomItem,                         // 59 SPELL_EFFECT_CREATE_RANDOM_ITEM       create item base at spell specific loot
    &Spell::EffectProficiency,                              // 60 SPELL_EFFECT_PROFICIENCY
    &Spell::EffectSendEvent,                                // 61 SPELL_EFFECT_SEND_EVENT
    &Spell::EffectPowerBurn,                                // 62 SPELL_EFFECT_POWER_BURN
    &Spell::EffectThreat,                                   // 63 SPELL_EFFECT_THREAT
    &Spell::EffectTriggerSpell,                             // 64 SPELL_EFFECT_TRIGGER_SPELL
    &Spell::EffectApplyAreaAura,                            // 65 SPELL_EFFECT_APPLY_AREA_AURA_RAID
    &Spell::EffectRechargeManaGem,                          // 66 SPELL_EFFECT_CREATE_MANA_GEM          (possibly recharge it, misc - is item ID)
    &Spell::EffectHealMaxHealth,                            // 67 SPELL_EFFECT_HEAL_MAX_HEALTH
    &Spell::EffectInterruptCast,                            // 68 SPELL_EFFECT_INTERRUPT_CAST
    &Spell::EffectDistract,                                 // 69 SPELL_EFFECT_DISTRACT
    &Spell::EffectPull,                                     // 70 SPELL_EFFECT_PULL                     one spell: Distract Move
    &Spell::EffectPickPocket,                               // 71 SPELL_EFFECT_PICKPOCKET
    &Spell::EffectAddFarsight,                              // 72 SPELL_EFFECT_ADD_FARSIGHT
    &Spell::EffectUntrainTalents,                           // 73 SPELL_EFFECT_UNTRAIN_TALENTS
    &Spell::EffectApplyGlyph,                               // 74 SPELL_EFFECT_APPLY_GLYPH
    &Spell::EffectHealMechanical,                           // 75 SPELL_EFFECT_HEAL_MECHANICAL          one spell: Mechanical Patch Kit
    &Spell::EffectSummonObjectWild,                         // 76 SPELL_EFFECT_SUMMON_OBJECT_WILD
    &Spell::EffectScriptEffect,                             // 77 SPELL_EFFECT_SCRIPT_EFFECT
    &Spell::EffectUnused,                                   // 78 SPELL_EFFECT_ATTACK
    &Spell::EffectSanctuary,                                // 79 SPELL_EFFECT_SANCTUARY
    &Spell::EffectAddComboPoints,                           // 80 SPELL_EFFECT_ADD_COMBO_POINTS
    &Spell::EffectUnused,                                   // 81 SPELL_EFFECT_CREATE_HOUSE             one spell: Create House (TEST)
    &Spell::EffectNULL,                                     // 82 SPELL_EFFECT_BIND_SIGHT
    &Spell::EffectDuel,                                     // 83 SPELL_EFFECT_DUEL
    &Spell::EffectStuck,                                    // 84 SPELL_EFFECT_STUCK
    &Spell::EffectSummonPlayer,                             // 85 SPELL_EFFECT_SUMMON_PLAYER
    &Spell::EffectActivateObject,                           // 86 SPELL_EFFECT_ACTIVATE_OBJECT
    &Spell::EffectWMODamage,                                // 87 SPELL_EFFECT_WMO_DAMAGE
    &Spell::EffectWMORepair,                                // 88 SPELL_EFFECT_WMO_REPAIR
    &Spell::EffectWMOChange,                                // 89 SPELL_EFFECT_WMO_CHANGE // 0 intact // 1 damaged // 2 destroyed // 3 rebuilding
    &Spell::EffectKillCreditPersonal,                       // 90 SPELL_EFFECT_KILL_CREDIT              Kill credit but only for single person
    &Spell::EffectUnused,                                   // 91 SPELL_EFFECT_THREAT_ALL               one spell: zzOLDBrainwash
    &Spell::EffectEnchantHeldItem,                          // 92 SPELL_EFFECT_ENCHANT_HELD_ITEM
    &Spell::EffectForceDeselect,                            // 93 SPELL_EFFECT_FORCE_DESELECT
    &Spell::EffectSelfResurrect,                            // 94 SPELL_EFFECT_SELF_RESURRECT
    &Spell::EffectSkinning,                                 // 95 SPELL_EFFECT_SKINNING
    &Spell::EffectCharge,                                   // 96 SPELL_EFFECT_CHARGE
    &Spell::EffectCastButtons,                              // 97 SPELL_EFFECT_CAST_BUTTON (totem bar since 3.2.2a)
    &Spell::EffectKnockBack,                                // 98 SPELL_EFFECT_KNOCK_BACK
    &Spell::EffectDisEnchant,                               // 99 SPELL_EFFECT_DISENCHANT
    &Spell::EffectInebriate,                                //100 SPELL_EFFECT_INEBRIATE
    &Spell::EffectFeedPet,                                  //101 SPELL_EFFECT_FEED_PET
    &Spell::EffectDismissPet,                               //102 SPELL_EFFECT_DISMISS_PET
    &Spell::EffectReputation,                               //103 SPELL_EFFECT_REPUTATION
    &Spell::EffectSummonObject,                             //104 SPELL_EFFECT_SUMMON_OBJECT_SLOT1
    &Spell::EffectSummonObject,                             //105 SPELL_EFFECT_SUMMON_OBJECT_SLOT2
    &Spell::EffectSummonObject,                             //106 SPELL_EFFECT_SUMMON_OBJECT_SLOT3
    &Spell::EffectSummonObject,                             //107 SPELL_EFFECT_SUMMON_OBJECT_SLOT4
    &Spell::EffectDispelMechanic,                           //108 SPELL_EFFECT_DISPEL_MECHANIC
    &Spell::EffectSummonDeadPet,                            //109 SPELL_EFFECT_SUMMON_DEAD_PET
    &Spell::EffectDestroyAllTotems,                         //110 SPELL_EFFECT_DESTROY_ALL_TOTEMS
    &Spell::EffectDurabilityDamage,                         //111 SPELL_EFFECT_DURABILITY_DAMAGE
    &Spell::EffectUnused,                                   //112 SPELL_EFFECT_112
    &Spell::EffectResurrectNew,                             //113 SPELL_EFFECT_RESURRECT_NEW
    &Spell::EffectTaunt,                                    //114 SPELL_EFFECT_ATTACK_ME
    &Spell::EffectDurabilityDamagePCT,                      //115 SPELL_EFFECT_DURABILITY_DAMAGE_PCT
    &Spell::EffectSkinPlayerCorpse,                         //116 SPELL_EFFECT_SKIN_PLAYER_CORPSE       one spell: Remove Insignia, bg usage, required special corpse flags...
    &Spell::EffectSpiritHeal,                               //117 SPELL_EFFECT_SPIRIT_HEAL              one spell: Spirit Heal
    &Spell::EffectSkill,                                    //118 SPELL_EFFECT_SKILL                    professions and more
    &Spell::EffectApplyAreaAura,                            //119 SPELL_EFFECT_APPLY_AREA_AURA_PET
    &Spell::EffectUnused,                                   //120 SPELL_EFFECT_TELEPORT_GRAVEYARD       one spell: Graveyard Teleport Test
    &Spell::EffectWeaponDmg,                                //121 SPELL_EFFECT_NORMALIZED_WEAPON_DMG
    &Spell::EffectUnused,                                   //122 SPELL_EFFECT_122                      unused
    &Spell::EffectSendTaxi,                                 //123 SPELL_EFFECT_SEND_TAXI                taxi/flight related (misc value is taxi path id)
    &Spell::EffectPullTowards,                              //124 SPELL_EFFECT_PULL_TOWARDS
    &Spell::EffectModifyThreatPercent,                      //125 SPELL_EFFECT_MODIFY_THREAT_PERCENT
    &Spell::EffectStealBeneficialBuff,                      //126 SPELL_EFFECT_STEAL_BENEFICIAL_BUFF    spell steal effect?
    &Spell::EffectProspecting,                              //127 SPELL_EFFECT_PROSPECTING              Prospecting spell
    &Spell::EffectApplyAreaAura,                            //128 SPELL_EFFECT_APPLY_AREA_AURA_FRIEND
    &Spell::EffectApplyAreaAura,                            //129 SPELL_EFFECT_APPLY_AREA_AURA_ENEMY
    &Spell::EffectRedirectThreat,                           //130 SPELL_EFFECT_REDIRECT_THREAT
    &Spell::EffectPlayerNotification,                       //131 SPELL_EFFECT_PLAYER_NOTIFICATION
    &Spell::EffectPlayMusic,                                //132 SPELL_EFFECT_PLAY_MUSIC               sound id in misc value (SoundEntries.dbc)
    &Spell::EffectUnlearnSpecialization,                    //133 SPELL_EFFECT_UNLEARN_SPECIALIZATION   unlearn profession specialization
    &Spell::EffectKillCredit,                               //134 SPELL_EFFECT_KILL_CREDIT              misc value is creature entry
    &Spell::EffectNULL,                                     //135 SPELL_EFFECT_CALL_PET
    &Spell::EffectHealPct,                                  //136 SPELL_EFFECT_HEAL_PCT
    &Spell::EffectEnergizePct,                              //137 SPELL_EFFECT_ENERGIZE_PCT
    &Spell::EffectLeapBack,                                 //138 SPELL_EFFECT_LEAP_BACK                Leap back
    &Spell::EffectQuestClear,                               //139 SPELL_EFFECT_CLEAR_QUEST              Reset quest status (miscValue - quest ID)
    &Spell::EffectForceCast,                                //140 SPELL_EFFECT_FORCE_CAST
    &Spell::EffectForceCastWithValue,                       //141 SPELL_EFFECT_FORCE_CAST_WITH_VALUE
    &Spell::EffectTriggerSpellWithValue,                    //142 SPELL_EFFECT_TRIGGER_SPELL_WITH_VALUE
    &Spell::EffectApplyAreaAura,                            //143 SPELL_EFFECT_APPLY_AREA_AURA_OWNER
    &Spell::EffectKnockBack,                                //144 SPELL_EFFECT_KNOCK_BACK_DEST
    &Spell::EffectPullTowards,                              //145 SPELL_EFFECT_PULL_TOWARDS_DEST                      Black Hole Effect
    &Spell::EffectActivateRune,                             //146 SPELL_EFFECT_ACTIVATE_RUNE
    &Spell::EffectQuestFail,                                //147 SPELL_EFFECT_QUEST_FAIL               quest fail
    &Spell::EffectUnused,                                   //148 SPELL_EFFECT_148   1 spell - 43509
    &Spell::EffectChargeDest,                               //149 SPELL_EFFECT_CHARGE_DEST
    &Spell::EffectQuestStart,                               //150 SPELL_EFFECT_QUEST_START
    &Spell::EffectTriggerRitualOfSummoning,                 //151 SPELL_EFFECT_TRIGGER_SPELL_2
    &Spell::EffectNULL,                                     //152 SPELL_EFFECT_152                      summon Refer-a-Friend
    &Spell::EffectCreateTamedPet,                           //153 SPELL_EFFECT_CREATE_TAMED_PET         misc value is creature entry
    &Spell::EffectDiscoverTaxi,                             //154 SPELL_EFFECT_DISCOVER_TAXI
    &Spell::EffectTitanGrip,                                //155 SPELL_EFFECT_TITAN_GRIP Allows you to equip two-handed axes, maces and swords in one hand, but you attack $49152s1% slower than normal.
    &Spell::EffectEnchantItemPrismatic,                     //156 SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC
    &Spell::EffectCreateItem2,                              //157 SPELL_EFFECT_CREATE_ITEM_2            create item or create item template and replace by some randon spell loot item
    &Spell::EffectMilling,                                  //158 SPELL_EFFECT_MILLING                  milling
    &Spell::EffectRenamePet,                                //159 SPELL_EFFECT_ALLOW_RENAME_PET         allow rename pet once again
    &Spell::EffectNULL,                                     //160 SPELL_EFFECT_160                      1 spell - 45534
    &Spell::EffectSpecCount,                                //161 SPELL_EFFECT_TALENT_SPEC_COUNT        second talent spec (learn/revert)
    &Spell::EffectActivateSpec,                             //162 SPELL_EFFECT_TALENT_SPEC_SELECT       activate primary/secondary spec
    &Spell::EffectNULL,                                     //163 unused
    &Spell::EffectRemoveAura,                               //164 SPELL_EFFECT_REMOVE_AURA
    &Spell::EffectDamageFromMaxHealthPCT,                   //165 SPELL_EFFECT_DAMAGE_FROM_MAX_HEALTH_PCT
    &Spell::EffectNULL,                                     //166
    &Spell::EffectNULL,                                     //167
    &Spell::EffectNULL,                                     //168
    &Spell::EffectNULL,                                     //169
    &Spell::EffectNULL,                                     //170
    &Spell::EffectNULL,                                     //171
    &Spell::EffectResurrect,                                //172 SPELL_EFFECT_MASS_RESURRECT
    &Spell::EffectActivateGuildBankSlot,                    //173 SPELL_EFFECT_ACTIVATE_GUILD_BANK_SLOT
    &Spell::EffectApplyAura,                                //174 SPELL_EFFECT_APPLY_AURA_FORCED
    &Spell::EffectUnused,                                   //175 unused
    &Spell::EffectSanctuary,                                //176 SPELL_EFFECT_SANCTUARY_2
    &Spell::EffectNULL,                                     //177
    &Spell::EffectUnused,                                   //178 unused
    &Spell::EffectNULL,                                     //179
    &Spell::EffectUnused,                                   //180 unused
    &Spell::EffectUnused,                                   //181 unused
    &Spell::EffectNULL,                                     //182
};

void Spell::EffectNULL(SpellEffIndex /*effIndex*/)
{
    sLog->outDebug("WORLD: Spell Effect DUMMY");
}

void Spell::EffectUnused(SpellEffIndex /*effIndex*/)
{
    // NOT USED BY ANY SPELL OR USELESS OR IMPLEMENTED IN DIFFERENT WAY IN TRINITY
}

void Spell::EffectResurrectNew(SpellEffIndex effIndex)
{
    if (!unitTarget || unitTarget->isAlive())
        return;

    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    if (!unitTarget->IsInWorld())
        return;

    Player* pTarget = unitTarget->ToPlayer();

    if (pTarget->isRessurectRequested())       // already have one active request
        return;

    uint32 health = damage;
    uint32 mana = m_spellInfo->EffectMiscValue[effIndex];
    ExecuteLogEffectResurrect(effIndex, pTarget);
    pTarget->setResurrectRequestData(m_caster->GetGUID(), m_caster->GetMapId(), m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ(), health, mana);
    SendResurrectRequest(pTarget);
}

void Spell::EffectInstaKill(SpellEffIndex /*effIndex*/)
{
    if (!unitTarget || !unitTarget->isAlive())
        return;

    // Demonic Sacrifice
    if (m_spellInfo->Id == 18788 && unitTarget->GetTypeId() == TYPEID_UNIT)
    {
        uint32 entry = unitTarget->GetEntry();
        uint32 spellID;
        switch (entry)
        {
            case   416: spellID = 18789; break;               //imp
            case   417: spellID = 18792; break;               //fellhunter
            case  1860: spellID = 18790; break;               //void
            case  1863: spellID = 18791; break;               //succubus
            case 17252: spellID = 35701; break;               //fellguard
            default:
                sLog->outError("EffectInstaKill: Unhandled creature entry (%u) case.", entry);
                return;
        }

        m_caster->CastSpell(m_caster, spellID, true);
    }

    if (m_caster == unitTarget)                              // prevent interrupt message
        finish();

    WorldPacket data(SMSG_SPELLINSTAKILLLOG, 8+8+4);
    data << uint64(m_caster->GetGUID());
    data << uint64(unitTarget->GetGUID());
    data << uint32(m_spellInfo->Id);
    m_caster->SendMessageToSet(&data, true);

    m_caster->DealDamage(unitTarget, unitTarget->GetHealth(), NULL, NODAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
}

void Spell::EffectEnvirinmentalDMG(SpellEffIndex effIndex)
{
    uint32 absorb = 0;
    uint32 resist = 0;

    // Note: this hack with damage replace required until GO casting not implemented
    // environment damage spells already have around enemies targeting but this not help in case not existed GO casting support
    // currently each enemy selected explicitly and self cast damage, we prevent apply self casted spell bonuses/etc
    damage = SpellMgr::CalculateSpellEffectAmount(m_spellInfo, effIndex, m_caster);

    m_caster->CalcAbsorbResist(m_caster, GetSpellSchoolMask(m_spellInfo), SPELL_DIRECT_DAMAGE, damage, &absorb, &resist, m_spellInfo);

    m_caster->SendSpellNonMeleeDamageLog(m_caster, m_spellInfo->Id, damage, GetSpellSchoolMask(m_spellInfo), absorb, resist, false, 0, false);
    if (m_caster->GetTypeId() == TYPEID_PLAYER)
        m_caster->ToPlayer()->EnvironmentalDamage(DAMAGE_FIRE, damage);
}

void Spell::EffectSchoolDMG(SpellEffIndex /*effIndex*/)
{
}

void Spell::SpellDamageSchoolDmg(SpellEffIndex effIndex)
{
    bool apply_direct_bonus = true;

    if (unitTarget && unitTarget->isAlive())
    {
        switch (m_spellInfo->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
            {
                // Meteor like spells (divided damage to targets)
                if (m_customAttr & SPELL_ATTR0_CU_SHARE_DAMAGE)
                {
                    uint32 count = 0;
                    for (std::list<TargetInfo>::iterator ihit= m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                        if (ihit->effectMask & (1<<effIndex))
                            ++count;

                    damage /= count;                    // divide to all targets
                }

                switch(m_spellInfo->Id)                     // better way to check unknown
                {
                    // Positive/Negative Charge
                    case 28062:
                    case 28085:
                    case 39090:
                    case 39093:
                        if (!m_triggeredByAuraSpell)
                            break;
                        if (unitTarget == m_caster)
                        {
                            uint8 count = 0;
                            for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                                if (ihit->targetGUID != m_caster->GetGUID())
                                    if (Player *target = ObjectAccessor::GetPlayer(*m_caster, ihit->targetGUID))
                                        if (target->HasAura(m_triggeredByAuraSpell->Id))
                                            ++count;
                            if (count)
                            {
                                uint32 spellId = 0;
                                switch (m_spellInfo->Id)
                                {
                                    case 28062: spellId = 29659; break;
                                    case 28085: spellId = 29660; break;
                                    case 39090: spellId = 39089; break;
                                    case 39093: spellId = 39092; break;
                                }
                                m_caster->SetAuraStack(spellId, m_caster, count);
                            }
                        }

                        if (unitTarget->HasAura(m_triggeredByAuraSpell->Id))
                            damage = 0;
                        break;
                    // Consumption
                    case 28865:
                        damage = (((InstanceMap*)m_caster->GetMap())->GetDifficulty() == REGULAR_DIFFICULTY ? 2750 : 4250);
                        break;
                    // percent from health with min
                    case 25599:                             // Thundercrash
                    {
                        damage = unitTarget->GetHealth() / 2;
                        if (damage < 200)
                            damage = 200;
                        break;
                    }
                    // arcane charge. must only affect demons (also undead?)
                    case 45072:
                    {
                        if (unitTarget->GetCreatureType() != CREATURE_TYPE_DEMON
                            && unitTarget->GetCreatureType() != CREATURE_TYPE_UNDEAD)
                            return;
                        break;
                    }
                    case 98649: // Meltdown (Lord Rhyolith encounter)
                    case 101646:
                    case 101647:
                    case 101648:
                    {
                        damage = m_caster->GetHealth();
                        apply_direct_bonus = false;
                        break;
                    }
                    case 89667: // lightning rod (al'akir)
                    case 93293:
                    case 93294:
                    case 93295:
                    {
                        // damages all allies within 20yd horizontally and 5yd vertically
                        float dist_z = unitTarget->GetPositionZ() - m_caster->GetPositionZ();
                        if (dist_z < -5.0f || dist_z > 5.0f)
                            return;
                        break;
                    }
                    case 33671: // gruul's shatter
                    case 50811: // krystallus shatter ( Normal )
                    case 61547: // krystallus shatter ( Heroic )
                    {
                        // don't damage self and only players
                        if (unitTarget->GetGUID() == m_caster->GetGUID() || unitTarget->GetTypeId() != TYPEID_PLAYER)
                            return;

                        float radius = GetEffectRadius(0);
                        if (!radius) return;
                        float distance = m_caster->GetDistance2d(unitTarget);
                        damage = (distance > radius) ? 0 : int32(SpellMgr::CalculateSpellEffectAmount(m_spellInfo, 0) * ((radius - distance)/radius));
                        break;
                    }
                    // TODO: add spell specific target requirement hook for spells
                    // Shadowbolts only affects targets with Shadow Mark (Gothik)
                    case 27831:
                    case 55638:
                        if (!unitTarget->HasAura(27825))
                            return;
                        break;
                    // Cataclysmic Bolt
                    case 38441:
                    {
                        damage = unitTarget->CountPctFromMaxHealth(50);
                        break;
                    }
                    // Rocket Barrage
                    case 69041:
                    {
                        damage = uint32(1+ m_caster->getLevel() * 2);
                        break;
                    }
                    case 56578: // Rapid-Fire Harpoon
                    case 62775: // Tympanic Tantrum
                    {
                        damage = unitTarget->CountPctFromMaxHealth(damage);
                        break;
                    }
                    // Loken Pulsing Shockwave
                    case 59837:
                    case 52942:
                    {
                        // don't damage self and only players
                        if(unitTarget->GetGUID() == m_caster->GetGUID() || unitTarget->GetTypeId() != TYPEID_PLAYER)
                            return;

                        float radius = GetEffectRadius(0);
                        if (!radius)
                            return;
                        float distance = m_caster->GetDistance2d(unitTarget);
                        damage = (distance > radius) ? 0 : int32(SpellMgr::CalculateSpellEffectAmount(m_spellInfo, 0) * distance);
                        break;
                    }
                    // Burning Metal - multiply damage per stack
                    case 76002:
                    {
                        apply_direct_bonus = false;
                        // normal version - 1000 per stack
                        if (Aura* pAura = m_caster->GetAura(75846))
                        {
                            damage = 1000*pAura->GetStackAmount();
                        }
                        // HC version - 2000 per stack
                        else if (Aura* pAura = m_caster->GetAura(93567))
                        {
                            damage = 2000*pAura->GetStackAmount();
                        }
                        break;
                    }
                    case 99400: // Burning blast ( Ragnaros encounter)
                    case 101241:
                    case 101242:
                    case 101243:
                    {
                        if(Aura* pAura = m_caster->GetAura(99399)) // Burning wound
                        {
                            uint8 stacks = pAura->GetStackAmount();
                            damage *= stacks; // multiply damage by number Burning wound stacks
                            apply_direct_bonus = false;
                        }
                        break;
                    }
                    case 100271: // Combustion ( Ragnaros encounter)
                    case 100272:
                    {
                        Aura * a = m_caster->GetAura(100249);
                        if(!a)
                            a = m_caster->GetAura(100250);
                        if (a)
                            damage = 2000 * a->GetStackAmount();
                        apply_direct_bonus = false;
                        break;
                    }
                    case 98175: // Magma trap ( Ragnaros encounter )
                    case 100106:
                    case 100107:
                    case 100108:
                    {
                        damage = (damage * 170 ) / 100;
                        break;
                    }
                    // Paladin: Guardian of Ancient Kings: Ancient Fury
                    case 86704:
                    {
                        if(Aura* pAura = m_caster->GetAura(86700))
                        {
                            uint8 charges = pAura->GetCharges();
                            damage *= charges; // multiply damage by number of charges of Ancient Power
                        }
                        break;
                    }
                    // Tyrande's favorite doll - Release mana: aoe damage
                    case 92601:
                        if (AuraEffect* storedmana = m_caster->GetAuraEffect(92596, EFFECT_0))
                        {
                            damage = storedmana->GetAmount();
                            apply_direct_bonus = false;
                        }
                        else return;
                        break;
                    // Bane of Havoc damage proc spell
                    case 85455:
                        // Don't allow to modify damage with spellpower and so on
                        apply_direct_bonus = false;
                        break;
                    // Immolation (infernal)
                    case 31303:
                    {
                        Player* pOwner = m_caster->GetCharmerOrOwnerPlayerOrPlayerItself();
                        if (!pOwner)
                            break;

                        damage = 40 + pOwner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + SPELL_SCHOOL_SHADOW)*0.675f;

                        apply_direct_bonus = false;

                        int32 bonuspct = pOwner->GetTotalAuraModifierByAffectMask(SPELL_AURA_ADD_PCT_MODIFIER, m_spellInfo);
                        if (bonuspct != 0)
                            damage *= 1.0f+((float)bonuspct/100.0f);
                        break;
                    }
                    // Gargoyle Strike (gargoyle, dk)
                    case 51963:
                    {
                        Player* pOwner = m_caster->GetCharmerOrOwnerPlayerOrPlayerItself();
                        if (!pOwner)
                            break;

                        damage = 35 + pOwner->GetTotalAttackPowerValue(BASE_ATTACK)*0.453f;

                        apply_direct_bonus = false;

                        int32 bonuspct = pOwner->GetTotalAuraModifierByAffectMask(SPELL_AURA_ADD_PCT_MODIFIER, m_spellInfo);
                        if (bonuspct != 0)
                            damage *= 1.0f+((float)bonuspct/100.0f);
                        break;
                    }
                }
                break;
            }
            case SPELLFAMILY_MAGE:
            {
                // Arcane Missiles Aurastate
                // since 4.0.1, every spell has a 40% chance of enabling Arcane Missiles
                // but is canceled by presence of talents:
                if (!(m_caster->HasAura(44445) || // Hot Streak
                    m_caster->HasAura(44446) ||  // Improved Hot Streak #1 (overrides Hot Streak)
                    m_caster->HasAura(44448) ||  // Improved Hot Streak #2
                    m_caster->HasAura(44546) ||  // Mind Freeze #1
                    m_caster->HasAura(44548) ||  // Mind Freeze #2
                    m_caster->HasAura(44549) ))  // Mind Freeze #3
                {
                    if (m_caster->getClass() == CLASS_MAGE && m_caster->getLevel() >= 3
                        && m_spellInfo->Id != 7268 && roll_chance_i(40))
                    {
                        if (!m_caster->HasAura(79683))
                            m_caster->CastSpell(m_caster, 79683, true); // Arcane Missiles Aurastate
                    }
                }

                // spell Pyroblast! from Hot Streak talent - remove aura state
                if (m_spellInfo->Id == 92315 && m_caster->HasAura(48108))
                    m_caster->RemoveAurasDueToSpell(48108);

                // Frostbolt
                if (m_spellInfo->Id == 116)
                {
                    // Early Frost
                    if(m_caster->HasAura(83049) && !m_caster->HasAura(83162))
                        m_caster->CastSpell(m_caster,83162,true);
                    else if(m_caster->HasAura(83050) && !m_caster->HasAura(83239))
                        m_caster->CastSpell(m_caster,83239,true);
                }
                // Freeze (Water Elemental ability)
                else if (m_spellInfo->Id == 33395)
                {
                    Player* pOwner = m_caster->GetCharmerOrOwnerPlayerOrPlayerItself();
                    if (!pOwner)
                        break;

                    // Talent Improved Freeze
                    if ((pOwner->HasAura(86259) && roll_chance_i(33)) ||
                        (pOwner->HasAura(86260) && roll_chance_i(66)) ||
                        (pOwner->HasAura(86314)))
                    {
                        // 2x cast of Fingers of Frost (faster than verifying of
                        // aura presence / adding / modifying stack)
                        pOwner->CastSpell(pOwner, 44544, true);
                        pOwner->CastSpell(pOwner, 44544, true);
                    }
                }
                // Arcane Blast
                else if (m_spellInfo->Id == 30451)
                {
                    if (m_caster && unitTarget && m_caster->GetTypeId() == TYPEID_PLAYER)
                    {
                        // Nether Vortex
                        if (m_caster->HasAura(86209) || (m_caster->HasAura(86181) && roll_chance_i(50)))
                        {
                            Unit::AuraList const& myAuras = m_caster->GetSingleCastAuras();
                            bool found = false;
                            if (!myAuras.empty())
                            {
                                for (Unit::AuraList::const_iterator itr = myAuras.begin(); itr != myAuras.end(); ++itr)
                                {
                                    if ((*itr) && (*itr)->GetId() == 31589)
                                    {
                                        // search only on other targets than a current
                                        Unit *owner = (*itr)->GetUnitOwner();
                                        if (owner && unitTarget->GetGUID() != owner->GetGUID())
                                        {
                                            found = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            if (!found)
                                m_caster->CastSpell(unitTarget, 31589, true);
                        }
                    }
                }
                break;
            }
            case SPELLFAMILY_WARRIOR:
            {
                // Bloodthirst
                if (m_spellInfo->SpellFamilyFlags[1] & 0x400)
                    damage = uint32(damage * (m_caster->GetTotalAttackPowerValue(BASE_ATTACK)) / 100);
                // Victory Rush
                else if (m_spellInfo->SpellFamilyFlags[1] & 0x100)
                {
                    damage = uint32(damage * m_caster->GetTotalAttackPowerValue(BASE_ATTACK) / 100);
                    m_caster->ModifyAuraState(AURA_STATE_WARRIOR_VICTORY_RUSH, false);
                }
                // Heroic Strike
                else if (m_spellInfo->Id == 78)
                {
                    damage = uint32(8 + m_caster->GetTotalAttackPowerValue(BASE_ATTACK)* 60 / 100);
                }
                // Shockwave
                else if (m_spellInfo->Id == 46968)
                {
                    int32 pct = m_caster->CalculateSpellDamage(unitTarget, m_spellInfo, 2);
                    if (pct > 0)
                        damage+= int32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * pct / 100);
                    break;
                }
                // Cleave damage recalculate
                else if (m_spellInfo->Id == 845)
                {
                    damage = 6 + m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.45f;
                }
                // Execute
                else if (m_spellInfo->Id == 5308)
                {
                    float availableRage = m_caster->GetPower(POWER_RAGE);

                    // Sudden Death rage save
                    if (m_caster->HasAura(29723))
                        availableRage -= 50.0f;
                    else if (m_caster->HasAura(29725))
                        availableRage -= 100.0f;

                    if (availableRage < 0.0f)  // no bonus rage used (Sudden Death could set it to negative)
                        availableRage = 0.0f;

                    if (availableRage > 200.0f)     // maximum of 20 rage used
                        availableRage = 200.0f;

                    float usedRage = 10.0f + (availableRage / 10.0f);    // rage amount used for a computation (including 10 base)
                    // before: damage = % modifier for low levels, after: final damage
                    damage = uint32 (10 + m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.0437f * usedRage) * damage / 100;

                    m_caster->ModifyPower(POWER_RAGE, -int32(availableRage));
                }
                // Thunder Clap and talent Blood and Thunder
                else if (m_spellInfo->Id == 6343)
                {
                    if ((m_caster->HasAura(84614) && roll_chance_i(50)) || m_caster->HasAura(84615))
                    {
                        if (unitTarget->HasAura(94009) && m_caster->ToPlayer())
                        {
                            // apply Rend to all targets of Thunder Clap
                            for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                                if (Unit* pl = Unit::GetUnit(*m_caster, ihit->targetGUID))
                                    m_caster->CastSpell(pl, 772, true);
                        }
                    }
                }
                // Shield Slam
                else if (m_spellInfo->Id == 23922)
                {
                    // manual implementation due to avoid overpowering already overpowered class
                    damage += m_spellInfo->ap_bonus*m_caster->GetTotalAttackPowerValue(BASE_ATTACK);
                }
                // Glyph of Heroic throw
                else if (m_spellInfo->Id == 57755 && m_caster->HasAura(58357)&&unitTarget)
                {
                    m_caster->CastSpell(unitTarget,58567,true);
                }
                break;
            }
            case SPELLFAMILY_WARLOCK:
            {
                // Shadow Bolt, Incinerate and Hand of Gul'dan, Soul Fire
                if (m_caster->ToPlayer()
                    && (m_spellInfo->Id == 686 || m_spellInfo->Id == 29722 || m_spellInfo->Id == 71521 || m_spellInfo->Id == 6353))
                {
                    // Impending Doom talent
                    if ((m_caster->HasAura(85108) && roll_chance_i(15)) ||
                        (m_caster->HasAura(85107) && roll_chance_i(10)) ||
                        (m_caster->HasAura(85106) && roll_chance_i(5)))
                    {
                        // Modify cooldown of Metamorphosis and linked spells
                        m_caster->ToPlayer()->ModifySpellCooldown(47241, -15000, true);
                        m_caster->ToPlayer()->ModifySpellCooldown(54817, -15000, true);
                        m_caster->ToPlayer()->ModifySpellCooldown(54879, -15000, true);
                    }
                }

                // Conflagrate
                if (m_spellInfo->TargetAuraState == AURA_STATE_CONFLAGRATE)
                {
                    AuraEffect const* aura = NULL;                // found req. aura for damage calculation

                    Unit::AuraEffectList const &mPeriodic = unitTarget->GetAuraEffectsByType(SPELL_AURA_PERIODIC_DAMAGE);
                    for (Unit::AuraEffectList::const_iterator i = mPeriodic.begin(); i != mPeriodic.end(); ++i)
                    {
                        // for caster applied auras only
                        if ((*i)->GetSpellProto()->SpellFamilyName != SPELLFAMILY_WARLOCK ||
                            (*i)->GetCasterGUID() != m_caster->GetGUID())
                            continue;

                        // Immolate
                        if ((*i)->GetSpellProto()->SpellFamilyFlags[0] & 0x4)
                        {
                            aura = *i;                      // it selected always if exist
                            break;
                        }
                    }

                    // found Immolate
                    if (aura)
                    {
                        uint32 pdamage = aura->GetAmount() > 0 ? aura->GetAmount() : 0;
                        pdamage = m_caster->SpellDamageBonus(unitTarget, aura->GetSpellProto(), aura->GetEffIndex(), pdamage, DOT, aura->GetBase()->GetStackAmount());
                        uint32 pct_dir = m_caster->CalculateSpellDamage(unitTarget, m_spellInfo, (effIndex + 1));
                        uint8 baseTotalTicks = uint8(m_caster->CalcSpellDuration(aura->GetSpellProto()) / aura->GetSpellProto()->EffectAmplitude[2]);
                        damage += pdamage * baseTotalTicks * pct_dir / 100;

                        uint32 pct_dot = m_caster->CalculateSpellDamage(unitTarget, m_spellInfo, (effIndex + 2)) / 3;
                        m_spellValue->EffectBasePoints[1] = SpellMgr::CalculateSpellEffectBaseAmount(pdamage * baseTotalTicks * pct_dot / 100, m_spellInfo, 1);

                        apply_direct_bonus = false;
                        break;
                    }
                }
                // Shadow Bite
                else if (m_spellInfo->SpellFamilyFlags[1] & 0x400000)
                {
                    if (m_caster->GetTypeId() == TYPEID_UNIT && m_caster->ToCreature()->isPet())
                    {
                        damage += (m_caster->ToPet()->GetBonusDamage()/0.15f)*1.228f*0.5f;

                        // 30% damage increase per every caster's dot
                        damage *= 1.0f+unitTarget->GetDoTsByCaster(m_caster->GetOwnerGUID())*0.30f;

                        apply_direct_bonus = false;
                    }
                }
                // Searing Pain
                else if (m_spellInfo->Id == 5676)
                {
                    if (m_caster->HasAura(74434) && !m_caster->HasAura(79440))
                    {
                        //m_caster->CastSpell(m_caster, 79440, true);
                        Aura* pAura = m_caster->AddAura(79440, m_caster);
                        pAura->SetCharges(2);
                        m_caster->RemoveAura(74434);
                    }
                }
                else if (m_spellInfo->Id == 31117) // Unstable Affliction backfire dmg has been doubled at 4.3.4
                {
                    damage *= 2;
                }
                // Soul Swap
                else if (m_spellInfo->Id == 86121)
                {
                    int32 bp0 = 0;
                    std::list<uint32> DoTList;
                    Unit::AuraEffectList const &mPeriodic = unitTarget->GetAuraEffectsByType(SPELL_AURA_PERIODIC_DAMAGE);
                    for (Unit::AuraEffectList::const_iterator i = mPeriodic.begin(); i != mPeriodic.end(); ++i)
                    {
                        if ((*i)->GetCaster() == m_caster)
                        {
                            DoTList.push_back((*i)->GetId());
                            // Hardcoded values, sorry, our spellsystem doesnt support something like that
                            switch ((*i)->GetId())
                            {
                                case 172:   // Corruption
                                    bp0 |= 1 << 0;
                                    break;
                                case 603:   // Bane of Doom
                                    bp0 |= 1 << 1;
                                    break;
                                case 980:   // Bane of Agony
                                    bp0 |= 1 << 2;
                                    break;
                                case 27243: // Seed of Corruption
                                    bp0 |= 1 << 3;
                                    break;
                                case 27285: // Seed of Corruption #2
                                    bp0 |= 1 << 4;
                                    break;
                                case 30108: // Unstable Affliction
                                    bp0 |= 1 << 5;
                                    break;
                                //case 80240: // Bane of Havoc
                                //    bp0 |= 1 << 6;
                                //    break;
                            }
                        }
                    }
                    // Remove DoTs only if caster doesnt have Glyph of Soul Swap
                    if (!DoTList.empty() && !m_caster->HasAura(56226))
                    {
                        for (std::list<uint32>::const_iterator itr = DoTList.begin(); itr != DoTList.end(); ++itr)
                            unitTarget->RemoveAurasDueToSpell((*itr));
                    }
                    // Cast spell which changes AB spell to Exhale one
                    Aura* pMarkAura = unitTarget->AddAura(86211, m_caster);
                    if (pMarkAura && pMarkAura->GetEffect(0))
                        pMarkAura->GetEffect(0)->SetScriptedAmount(bp0);
                    // ..and cast visual
                    unitTarget->CastSpell(m_caster, 92795, true);

                    // Glyph of Soul Swap also makes this spell to have cooldown
                    if (m_caster->HasAura(56226))
                    {
                        //m_caster->CastSpell(m_caster, 94229, true); // Soul Swap Cooldown Marker
                        m_caster->ToPlayer()->AddSpellCooldown(86121, 0, 30000);

                        //Send cooldown manully
                        WorldPacket data(SMSG_SPELL_COOLDOWN, 8 + 1 + 4);
                        data << uint64(m_caster->GetGUID());
                        data << uint8(1);
                        data << uint32(86121);
                        data << uint32(30000); // 30 seconds
                        m_caster->ToPlayer()->GetSession()->SendPacket(&data);
                    }
                }
                // Soul Swap Exhale
                else if (m_spellInfo->Id == 86213)
                {
                    Aura* pMarkAura = m_caster->GetAura(86211);
                    if (!pMarkAura || !pMarkAura->GetEffect(0))
                        return;

                    if (pMarkAura->GetCaster() == unitTarget)
                        return;

                    // -100% cast time
                    m_caster->CastSpell(m_caster, 92794, true);

                    int32 bp0 = pMarkAura->GetEffect(0)->GetScriptedAmount();
                    if (bp0 & (1 << 0))
                        m_caster->CastSpell(unitTarget, 172, true);
                    if (bp0 & (1 << 1))
                        m_caster->CastSpell(unitTarget, 603, true);
                    if (bp0 & (1 << 2))
                        m_caster->CastSpell(unitTarget, 980, true);
                    if (bp0 & (1 << 3))
                        m_caster->CastSpell(unitTarget, 27243, true);
                    if (bp0 & (1 << 4))
                        m_caster->CastSpell(unitTarget, 27285, true);
                    if (bp0 & (1 << 5))
                        m_caster->CastSpell(unitTarget, 30108, true);

                    // And return back old Soul Swap spell
                    m_caster->RemoveAurasDueToSpell(86211);
                }
                // Imps Firebolt
                else if (m_spellInfo->Id == 3110)
                {
                    // Damage fix
                    if (m_caster->GetTypeId() == TYPEID_UNIT && m_caster->ToCreature()->isPet())
                        damage += (m_caster->ToPet()->GetBonusDamage()/0.15f)*0.657f*0.5f;
                    apply_direct_bonus = false;

                    Player* pOwner = m_caster->GetCharmerOrOwnerPlayerOrPlayerItself();
                    if (!pOwner)
                        break;

                    damage = pOwner->SpellDamageBonus(unitTarget, m_spellInfo, effIndex, damage, SPELL_DIRECT_DAMAGE);

                    // Burning Embers talent
                    int32 bp0 = 0;
                    if (pOwner->HasAura(85112))
                        bp0 = 50;
                    else if (pOwner->HasAura(91986))
                        bp0 = 25;

                    // We must calculate maximal basepoints manually
                    int32 maxbp = pOwner->SpellBaseDamageBonus(SPELL_SCHOOL_MASK_SPELL);
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
                        pScaling.Init(pOwner->getLevel(), pSpell);
                        maxbp = (maxbp + pScaling.avg[1])/7;
                        bp0 *= (damage/100)/7;
                        if (bp0 > maxbp)
                            bp0 = maxbp;
                        // If we already have aura like this, only refresh if damage is lower
                        if (unitTarget->HasAura(85421) && unitTarget->GetAura(85421)->GetEffect(0) &&
                            unitTarget->GetAura(85421)->GetEffect(0)->GetAmount() >= bp0)
                            unitTarget->GetAura(85421)->RefreshDuration();
                        else
                        pOwner->CastCustomSpell(unitTarget, 85421, &bp0, 0, 0, true);
                    }
                }
                // Lash of Pain (succubus)
                else if (m_spellInfo->Id == 7814 && m_caster->ToPet())
                {
                    damage += (m_caster->ToPet()->GetBonusDamage()/0.15f)*0.612f*0.5f;
                    apply_direct_bonus = false;

                    Player* pOwner = m_caster->GetCharmerOrOwnerPlayerOrPlayerItself();
                    if (!pOwner)
                        break;

                    int32 bonuspct = pOwner->GetTotalAuraModifierByAffectMask(SPELL_AURA_ADD_PCT_MODIFIER, m_spellInfo);
                    if (bonuspct != 0)
                        damage *= 1.0f+((float)bonuspct/100.0f);
                }
                // Whiplash (succubus)
                else if (m_spellInfo->Id == 6360 && m_caster->ToPet())
                {
                    damage += (m_caster->ToPet()->GetBonusDamage()/0.15f)*0.85f*0.5f;
                    apply_direct_bonus = false;
                }
                // Torment (voidwalker)
                else if (m_spellInfo->Id == 3716 && m_caster->ToPet())
                {
                    damage += (m_caster->ToPet()->GetBonusDamage()/0.15f)*0.512f*0.5f;
                    apply_direct_bonus = false;
                }
                // Doom Bolt (doomguard)
                else if (m_spellInfo->Id == 85692 && m_caster->ToTempSummon())
                {
                    Player* pOwner = m_caster->GetCharmerOrOwnerPlayerOrPlayerItself();
                    if (!pOwner)
                        break;

                    damage += pOwner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + SPELL_SCHOOL_SHADOW)*0.9f;
                    apply_direct_bonus = false;

                    int32 bonuspct = pOwner->GetTotalAuraModifierByAffectMask(SPELL_AURA_ADD_PCT_MODIFIER, m_spellInfo);
                    if (bonuspct != 0)
                        damage *= 1.0f+((float)bonuspct/100.0f);
                }
                break;
            }
            case SPELLFAMILY_PRIEST:
            {
                // Mind Blast
                if (m_spellInfo->Id == 8092)
                {
                    // Applies Mind Trauma effect if:
                    // We are in Shadow Form
                    if (m_caster->GetShapeshiftForm() == FORM_SHADOW)
                    {
                        float chance = 0.0f;

                        if (m_caster->HasAura(15273))
                            chance = 33.0f;
                        else if (m_caster->HasAura(15312))
                            chance = 66.0f;
                        else if (m_caster->HasAura(15313))
                            chance = 100.0f;

                        // We have Improved Mind Blast (chance > 0)
                        if (chance > 0)
                            // Chance has been successfully rolled
                            if (chance == 100.0f || roll_chance_f(chance))
                                m_caster->CastSpell(unitTarget, 48301, true);
                    }
                }
                // Improved Mind Blast (Mind Blast in shadow form bonus)
                else if (m_caster->GetShapeshiftForm() == FORM_SHADOW && (m_spellInfo->SpellFamilyFlags[0] & 0x00002000))
                {
                    Unit::AuraEffectList const& ImprMindBlast = m_caster->GetAuraEffectsByType(SPELL_AURA_ADD_FLAT_MODIFIER);
                    for (Unit::AuraEffectList::const_iterator i = ImprMindBlast.begin(); i != ImprMindBlast.end(); ++i)
                    {
                        if ((*i)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_PRIEST &&
                            ((*i)->GetSpellProto()->SpellIconID == 95))
                        {
                            int chance = SpellMgr::CalculateSpellEffectAmount((*i)->GetSpellProto(), 1, m_caster);
                            if (roll_chance_i(chance))
                                // Mind Trauma
                                m_caster->CastSpell(unitTarget, 48301, true, 0);
                            break;
                        }
                    }
                }
                // Reflective Shield
                else if (m_spellInfo->Id == 33619)
                {
                    apply_direct_bonus = false;
                }
                // Smite
                else if (m_spellInfo->Id == 585)
                {
                    // Train of Thought
                    if (m_caster->HasAura(92297) || (m_caster->HasAura(92295) && roll_chance_i(50)))
                        m_caster->ToPlayer()->ModifySpellCooldown(47540, -500, true);
                }
                break;
            }
            case SPELLFAMILY_DRUID:
            {
                if (m_caster->GetTypeId() == TYPEID_PLAYER)
                {
                    //Druid T12 Feral 4P Bonus
                    if (m_spellInfo->Id == 1079 || m_spellInfo->Id == 22568 || m_spellInfo->Id == 22570 || m_spellInfo->Id == 52610 )
                    {
                        int cp = (int)m_caster->ToPlayer()->GetComboPoints();

                        if (cp)
                        {
                            if (roll_chance_i(cp * 20)) // 20 % chance per combo point
                            {
                                if (Aura * berserk = m_caster->GetAura(50334)) // Berserk
                                {
                                    berserk->SetDuration(berserk->GetDuration() + 2000);
                                }
                            }
                        }
                    }
                }
                // Ferocious Bite
                if (m_caster->GetTypeId() == TYPEID_PLAYER && (m_spellInfo->SpellFamilyFlags[0] & 0x000800000) && m_spellInfo->SpellVisual[0] == 6587)
                {
                    // converts each extra point of energy into ($f1+$AP/410) additional damage
                    float ap = m_caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    // energy after using FB
                    int32 energy = m_caster->GetPower(POWER_ENERGY);

                    // Glyph of Ferocious Bite
                    if (m_caster->HasAura(67598))
                    {
                        //Healed for 1% of maximum health for each 10 energy spent
                        int32 bp = (m_caster->GetPower(POWER_ENERGY) + 25) / 10;
                        //Maximum of 50 energy can be spent (25 for FB + 25 for 100% dmg increase)
                        bp = (bp > 5) ? 5 : bp;

                        if(bp)
                            m_caster->CastCustomSpell(m_caster, 101024, &bp, 0, 0, true);
                    }

                    damage += int32(m_caster->ToPlayer()->GetComboPoints() * ap * 0.109f);

                    //If druid has 25 additional energy stored
                    if (energy - 25 >= 0)
                    {
                        m_caster->ModifyPower(POWER_ENERGY, -25);
                        // increase damage by up to 100%
                        damage *= 2;
                    }

                    // Done with calculation
                    apply_direct_bonus = false;
                }
                // Wrath
                else if (m_spellInfo->SpellFamilyFlags[0] & 0x00000001)
                {
                    // Improved Insect Swarm
                    if (AuraEffect const * aurEff = m_caster->GetDummyAuraEffect(SPELLFAMILY_DRUID, 1771, 0))
                        if (unitTarget->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DRUID, 0x00200000, 0, 0))
                            damage = int32(damage*(100.0f+aurEff->GetAmount())/100.0f);
                }
                // Starsurge
                else if (m_spellInfo->Id == 78674)
                {
                    // Glyph of Starsurge
                    if (m_caster && m_caster->ToPlayer() && m_caster->HasAura(62971))
                        m_caster->ToPlayer()->ModifySpellCooldown(48505, -5000, true);
                }
                // Starfire
                else if (m_spellInfo->Id == 2912)
                {
                    // Glyph of Starfire
                    if (m_caster->HasAura(54845))
                    {
                        uint32 dotId = 8921; // Moonfire dot 

                        Aura * moonFire = unitTarget->GetAura(8921, m_caster->GetGUID());
                        if (moonFire == NULL)
                        {
                            moonFire = unitTarget->GetAura(93402, m_caster->GetGUID()); // Or sunfire
                            dotId = 93402;
                        }

                        uint64 tempGUID = unitTarget->GetGUID();

                        if (moonFire)
                        {
                            UnitList targets;
                            {
                                float radius = 100.0f;

                                CellPair p(Trinity::ComputeCellPair(m_caster->GetPositionX(), m_caster->GetPositionY()));
                                Cell cell(p);
                                cell.data.Part.reserved = ALL_DISTRICT;

                                Trinity::AnyUnfriendlyVisibleUnitInObjectRangeCheck u_check(m_caster, m_caster, radius);
                                Trinity::UnitListSearcher<Trinity::AnyUnfriendlyVisibleUnitInObjectRangeCheck> checker(m_caster, targets, u_check);

                                TypeContainerVisitor<Trinity::UnitListSearcher<Trinity::AnyUnfriendlyVisibleUnitInObjectRangeCheck>, GridTypeMapContainer > grid_object_checker(checker);
                                TypeContainerVisitor<Trinity::UnitListSearcher<Trinity::AnyUnfriendlyVisibleUnitInObjectRangeCheck>, WorldTypeMapContainer > world_object_checker(checker);

                                cell.Visit(p, grid_object_checker, *m_caster->GetMap(), *m_caster, radius);
                                cell.Visit(p, world_object_checker, *m_caster->GetMap(), *m_caster, radius);
                            }

                            for (std::list<Unit*>::iterator itr = targets.begin(); itr != targets.end(); itr++)
                            {
                                if (Aura * mf = (*itr)->GetAura(dotId, m_caster->GetGUID()))
                                {
                                    //Only functions on the target with your most recently applied Moonfire.
                                    if (mf->GetApplyTime() > moonFire->GetApplyTime())
                                    {
                                        tempGUID = (*itr)->GetGUID();
                                        moonFire = mf;
                                    }
                                }
                            }

                            if (moonFire && unitTarget->GetGUID() == tempGUID)
                            {
                                // maximum of 3 renewals (9 seconds)
                                AuraEffect *dotEffect = moonFire->GetEffect(0);
                                if (dotEffect)
                                {
                                    int32 refreshCounter = dotEffect->GetScriptedAmount();
                                    if (refreshCounter < 3)
                                    {
                                        moonFire->SetDuration(moonFire->GetDuration() + 3000);
                                        dotEffect->SetScriptedAmount(refreshCounter + 1);
                                    }
                                }
                            }
                        }
                    }

                    // Fury of Stormrage removal
                    if (m_caster && m_caster->HasAura(81093))
                        m_caster->RemoveAurasDueToSpell(81093);
                }
                // Lacerate
                else if (m_spellInfo->Id == 33745)
                {
                    if (m_caster)
                        damage += m_caster->GetTotalAttackPowerValue(BASE_ATTACK)*0.0552f;
                }
                // Rake
                else if (m_spellInfo->Id == 1822)
                {
                    //Patch 4.2.0 (2011-06-28): Initial damage on hit now deals the same damage as each periodic tick 
                    // (and is treated the same for all combat calculations). Periodic damage now gains 14.7% of attack power per tick
                    damage += int32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK)*0.147f);
                }
                // Maul
                else if (m_spellInfo->Id == 6807)
                {
                    if (m_caster)
                    {
                        damage += m_caster->GetTotalAttackPowerValue(BASE_ATTACK)*0.19f;

                        if (unitTarget->HasAuraState(AURA_STATE_BLEEDING))
                        {
                            // Rend and Tear talent bonus
                            float coef = 0.0f;
                            if (m_caster->HasAura(48434))
                                coef = 1.20f;
                            else if (m_caster->HasAura(48433))
                                coef = 1.13f;
                            else if (m_caster->HasAura(48432))
                                coef = 1.07f;

                            if (coef)
                                damage *= coef;
                        }
                    }
                }
                // Trash (Bear form)
                else if (m_spellInfo->Id == 77758)
                {
                    if (m_caster)
                        damage += m_caster->GetTotalAttackPowerValue(BASE_ATTACK)*0.0982f;
                }
                break;
            }
            case SPELLFAMILY_ROGUE:
            {
                // Envenom
                if (m_caster->GetTypeId() == TYPEID_PLAYER && (m_spellInfo->SpellFamilyFlags[1] & 0x8))
                {
                    // consume from stack dozes not more that have combo-points
                    if (uint32 combo = m_caster->ToPlayer()->GetComboPoints())
                    {
                        // Lookup for Deadly poison (only attacker applied)
                        if (AuraEffect const * aurEff = unitTarget->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_ROGUE, 0x10000, 0, 0, m_caster->GetGUID()))
                        {
                            // count consumed deadly poison doses at target
                            bool needConsume = true;
                            uint32 spellId = aurEff->GetId();
                            uint32 doses = aurEff->GetBase()->GetStackAmount();
                            if (doses > combo)
                                doses = combo;
                            // Master Poisoner
                            Unit::AuraEffectList const& auraList = m_caster->ToPlayer()->GetAuraEffectsByType(SPELL_AURA_MOD_AURA_DURATION_BY_DISPEL_NOT_STACK);
                            for (Unit::AuraEffectList::const_iterator iter = auraList.begin(); iter != auraList.end(); ++iter)
                            {
                                if ((*iter)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_ROGUE && (*iter)->GetSpellProto()->SpellIconID == 1960)
                                {
                                    uint32 chance = SpellMgr::CalculateSpellEffectAmount((*iter)->GetSpellProto(), 2, m_caster);

                                    if (chance && roll_chance_i(chance))
                                        needConsume = false;

                                    break;
                                }
                            }

                            if (needConsume)
                                for (uint32 i = 0; i < doses; ++i)
                                    unitTarget->RemoveAuraFromStack(spellId);
                            damage *= doses;
                            damage += int32(((Player*)m_caster)->GetTotalAttackPowerValue(BASE_ATTACK) * 0.09f * doses);
                        }

                        // Relentless Strikes
                        uint32 chance = m_caster->ToPlayer()->GetComboPoints();
                        if(m_caster->HasAura(14179)) //rank 1
                        {
                            chance *= 7;
                        }
                        if(m_caster->HasAura(58422)) //rank 2
                        {
                            chance *= 14;
                        }
                        if(m_caster->HasAura(58423)) //rank 3
                        {
                            chance *= 20;
                        }
                        if(roll_chance_i(chance))
                        {
                            m_caster->CastSpell(m_caster, 14181 , true);
                        }
                        // Eviscerate and Envenom Bonus Damage (item set effect)
                        if (m_caster->HasAura(37169))
                            damage += ((Player*)m_caster)->GetComboPoints()*40;
                    }

                    // Revealing strike bonus damage
                    Aura* revealing = unitTarget->GetAura(84617, m_caster->GetGUID());
                    if (revealing)
                    {
                        float bonus = 0.35f;                     // adds 35% bonus
                        if (m_caster->HasAura(56814))           // glyph of revealing strike adds additional 10% bonus
                            bonus += 0.10f;

                        damage *= 1 + bonus;

                        revealing->Remove();
                    }

                    // Cut to the Chase
                    if (m_caster->HasAura(51667) ||
                        (m_caster->HasAura(51665) && roll_chance_i(67)) ||
                        (m_caster->HasAura(51664) && roll_chance_i(33)))
                    {
                        if (Aura* pAura = m_caster->GetAura(5171))
                        {
                            uint32 duration = 21000;

                            // Improved Slice and Dice
                            if (m_caster->HasAura(14166))
                                duration *= 1.5f;
                            else if (m_caster->HasAura(14165))
                                duration *= 1.25f;

                            // Glyph of Slice and Dice
                            if (m_caster->HasAura(56810))
                                duration += 6000;

                            pAura->SetMaxDuration(duration);
                            pAura->SetDuration(duration);
                        }
                    }
                }
                // Eviscerate
                else if ((m_spellInfo->SpellFamilyFlags[0] & 0x00020000) && m_caster->GetTypeId() == TYPEID_PLAYER)
                {
                    if (uint32 combo = ((Player*)m_caster)->GetComboPoints())
                    {
                        float ap = m_caster->GetTotalAttackPowerValue(BASE_ATTACK);
                        damage += int32(ap * combo * 0.091f);

                        // Eviscerate and Envenom Bonus Damage (item set effect)
                        if (m_caster->HasAura(37169))
                            damage += combo*40;

                        // Serrated Blades
                        if ((m_caster->HasAura(14171) && roll_chance_i(10*combo)) ||
                            (m_caster->HasAura(14172) && roll_chance_i(20*combo)) )
                        {
                            if (Aura* pAura = unitTarget->GetAura(1943))
                                pAura->RefreshDuration();
                        }
                    }

                    // Revealing strike bonus damage
                    Aura* revealing = unitTarget->GetAura(84617, m_caster->GetGUID());
                    if (revealing)
                    {
                        float bonus = 0.35f;                     // adds 35% bonus
                        if (m_caster->HasAura(56814))           // glyph of revealing strike adds additional 10% bonus
                            bonus += 0.10f;

                        damage *= 1 + bonus;

                        revealing->Remove();
                    }

                    // Cut to the Chase
                    if (m_caster->HasAura(51667) ||
                        (m_caster->HasAura(51665) && roll_chance_i(67)) ||
                        (m_caster->HasAura(51664) && roll_chance_i(33)))
                    {
                        if (Aura* pAura = m_caster->GetAura(5171))
                        {
                            uint32 duration = 21000;

                            // Improved Slice and Dice
                            if (m_caster->HasAura(14166))
                                duration *= 1.5f;
                            else if (m_caster->HasAura(14165))
                                duration *= 1.25f;

                            // Glyph of Slice and Dice
                            if (m_caster->HasAura(56810))
                                duration += 6000;

                            pAura->SetMaxDuration(duration);
                            pAura->SetDuration(duration);
                        }
                    }
                }
                else if (m_spellInfo->Id == 79136) // Venomous Wound
                {
                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                        damage += 0.176f*((Player*)m_caster)->GetTotalAttackPowerValue(BASE_ATTACK);
                }

                // Eviscerate and Envenom
                if (m_spellInfo->Id == 2098 || m_spellInfo->Id == 32645)
                {
                    // Restless Blades
                    if (m_caster)
                    {
                        int32 minusSecs = 0;
                        if (m_caster->HasAura(79096))
                            minusSecs = 2*m_caster->ToPlayer()->GetComboPoints();
                        else if (m_caster->HasAura(79095))
                            minusSecs = 1*m_caster->ToPlayer()->GetComboPoints();

                        // If present, modify cooldown of some spells
                        if (minusSecs)
                        {
                            m_caster->ToPlayer()->ModifySpellCooldown(13750, -minusSecs*1000, true); // Adrenaline Rush
                            m_caster->ToPlayer()->ModifySpellCooldown(2983 , -minusSecs*1000, true); // Sprint
                            m_caster->ToPlayer()->ModifySpellCooldown(51690, -minusSecs*1000, true); // Killing Spree
                            m_caster->ToPlayer()->ModifySpellCooldown(73981, -minusSecs*1000, true); // Redirect
                        }
                    }
                }
                // Gravity Strike (Corla's Zealot) + HC version
                if (m_spellInfo->Id == 76561 || m_spellInfo->Id == 93656)
                {
                    damage = unitTarget->CountPctFromMaxHealth(damage);
                }
                break;
            }
            case SPELLFAMILY_HUNTER:
            {   // Rapid Recuperation
                if (m_caster->HasAura(3045))
                {
                      if (m_caster->HasAura(53228))   // Rank 1
                          m_caster->CastSpell(m_caster,53230,true);
                      else if (m_caster->HasAura(53232))   // Rank 2
                          m_caster->CastSpell(m_caster,54227,true);
                }

                //Gore
                if (m_spellInfo->SpellIconID == 1578)
                {
                    if (m_caster->HasAura(57627))           // Charge 6 sec post-affect
                        damage *= 2;
                }
                //Kill Command
                else if (m_spellInfo->Id == 83381)
                {
                    if (m_caster->GetOwner() && m_caster->GetOwner()->ToPlayer())
                        damage += ceil(0.516f*float(m_caster->GetOwner()->ToPlayer()->GetTotalAttackPowerValue(RANGED_ATTACK)));
                }
                break;
            }
            case SPELLFAMILY_SHAMAN:
            {
                // Searing Bolt
                if (m_spellInfo->Id == 3606)
                {
                    Unit* m_owner = m_caster->GetOwner();
                    if(m_owner)
                    {
                        if(m_owner->HasAura(77657))
                            m_caster->CastSpell(unitTarget,77661,true);
                        else if (m_owner->HasAura(77656))
                        {
                            if(roll_chance_i(67))
                                m_caster->CastSpell(unitTarget,77661,true);
                        }
                        else if (m_owner->HasAura(77655))
                        {
                            if(roll_chance_i(33))
                                m_caster->CastSpell(unitTarget,77661,true);
                        }
                    }
                }
                // Earthquake triggered effect
                else if (m_spellInfo->Id == 77478)
                {
                    if (m_caster)
                        damage += m_caster->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + SPELL_SCHOOL_NATURE)*0.11f;
                }

                // Focused Insight - on every shock spell
                if (m_spellInfo->Id == 8056 || m_spellInfo->Id == 8050 || m_spellInfo->Id == 8042)
                {
                    int32 manamod = 0;
                    int32 effmod = 0;
                    if (m_caster->HasAura(77796))
                    {
                        manamod = -m_powerCost*0.75f;
                        effmod = 30;
                    }
                    else if (m_caster->HasAura(77795))
                    {
                        manamod = -m_powerCost*0.50f;
                        effmod = 20;
                    }
                    else if (m_caster->HasAura(77794))
                    {
                        manamod = -m_powerCost*0.25f;
                        effmod = 10;
                    }

                    if (manamod != 0 && effmod > 0)
                        m_caster->CastCustomSpell(m_caster, 77800, &manamod, &effmod, &effmod, true);
                }

                // Lightning Bolt -> Chain Lightning handled in SpellScript, cause should not count every single jump of spell
                if (m_spellInfo->Id == 403)
                {
                    // talent Feedback - modify cooldown of Elemental Mastery
                    if (m_caster->HasAura(86185))
                        m_caster->ToPlayer()->ModifySpellCooldown(16166, -3000, true);
                    else if (m_caster->HasAura(86184))
                        m_caster->ToPlayer()->ModifySpellCooldown(16166, -2000, true);
                    else if (m_caster->HasAura(86183))
                        m_caster->ToPlayer()->ModifySpellCooldown(16166, -1000, true);
                }

                // Frost Shock
                if (m_spellInfo->Id == 8056)
                {
                    // Frozen Power affects only targets further than 15y
                    if (m_caster && unitTarget && !unitTarget->IsWithinDistInMap(m_caster, 15.0f))
                    {
                        if (m_caster->HasAura(63374) || (m_caster->HasAura(63373) && roll_chance_i(50)))
                            m_caster->CastSpell(unitTarget, 63685, true);
                    }
                }

                // Fulmination
                if (m_spellInfo->Id == 88767)
                {
                    // disable damage bonus (already applied in spell script)
                    apply_direct_bonus = false;
                }

                // Implementation of Elemental Overload mastery profficiency
                if ((m_spellInfo->Id == 403 || m_spellInfo->Id == 51505 || m_spellInfo->Id == 421) &&
                    m_caster->ToPlayer() && m_caster->ToPlayer()->HasMastery() &&
                    m_caster->ToPlayer()->GetTalentBranchSpec(m_caster->ToPlayer()->GetActiveSpec()) == SPEC_SHAMAN_ELEMENTAL)
                {
                    int32 chance = m_caster->ToPlayer()->GetMasteryPoints()*2.0f;
                    if (roll_chance_i(chance))
                    {
                        switch (m_spellInfo->Id)
                        {
                            case 403:   // Lightning Bolt
                                m_caster->CastSpell(unitTarget,45284,true);
                                break;
                            case 51505: // Lava Burst
                                m_caster->CastSpell(unitTarget,77451,true);
                                break;
                            case 421:   // Chain Lightning
                                m_caster->CastSpell(unitTarget,45297,true);
                                break;
                            default:
                                break;
                        }
                    }
                }
                break;
            }
            case SPELLFAMILY_PALADIN:
            {
                // Hammer of the Righteous - AoE part
                if (m_spellInfo->Id == 88263)
                {
                    damage += m_spellInfo->ap_bonus*m_caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    apply_direct_bonus = false;
                }
                // Shield of the Righteous
                else if (m_spellInfo->Id == 53600)
                {
                    // silently added bonus from Blizzard
                    damage += 0.1f*m_caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    if (m_caster->GetPower(POWER_HOLY_POWER) > 0)
                    {
                        uint32 holypower = m_caster->GetPower(POWER_HOLY_POWER);
                        // Divine Purpose effect
                        if (m_caster->HasAura(90174))
                        {
                            holypower = 2;
                            m_caster->RemoveAurasDueToSpell(90174);
                        }
                        damage *= holypower*3.0f;
                        m_caster->SetPower(POWER_HOLY_POWER,0);
                    }
                    break;
                }
                // Exorcism (scales with AP / SP, chooses what's greater)
                else if (m_spellInfo->Id == 879)
                {
                    apply_direct_bonus = false;
                    uint32 sp = m_caster->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + SPELL_SCHOOL_HOLY);
                    uint32 ap = m_caster->GetTotalAttackPowerValue(BASE_ATTACK);

                    if (Player* modOwner = m_caster->GetSpellModOwner())
                        modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_DAMAGE, damage);

                    damage += 0.344f * ((sp > ap) ? sp : ap);
                    break;
                }
                // Hammer of Wrath
                else if (m_spellInfo->Id == 24275)
                {
                    apply_direct_bonus = false;

                    if (Player* modOwner = m_caster->GetSpellModOwner())
                        modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_DAMAGE, damage);

                    damage += 0.39f * m_caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    damage += 0.117f * m_caster->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + SPELL_SCHOOL_HOLY);
                }
                // Seal of Righteousness
                else if (m_spellInfo->Id == 25742 || m_spellInfo->Id == 101423)
                {
                    // damage formula is little wierd - divide weapon damage by 1.8 to be more accurent to tooltip
                    apply_direct_bonus = false;
                    damage = (m_caster->ToPlayer()->GetAttackTime(BASE_ATTACK)/1000.0f/1.8f)*0.011f*m_caster->GetUInt32Value(UNIT_FIELD_ATTACK_POWER)*0.022f*m_caster->GetStat(STAT_INTELLECT);
                }
                // Consecration triggered spell
                else if (m_spellInfo->Id == 81297)
                {
                    if (!m_caster)
                        break;

                    int32 ap = m_caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    int32 sph = m_caster->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + SPELL_SCHOOL_HOLY);
                    damage += int32(0.04f*(sph+ap)*0.675f); // 0.675 is magic number, present almost everywhere..

                    int32 bonuspct = m_caster->GetTotalAuraModifierByAffectMask(SPELL_AURA_ADD_PCT_MODIFIER, m_spellInfo);
                    if (bonuspct != 0)
                        damage *= 1.0f+((float)bonuspct /100.0f);

                    damage += m_caster->GetTotalAuraModifierByAffectMask(SPELL_AURA_ADD_FLAT_MODIFIER, m_spellInfo);

                    apply_direct_bonus = false;
                }
                break;
            }
            case SPELLFAMILY_DEATHKNIGHT:
            {
                // Blood Boil - bonus for diseased targets
                if (m_spellInfo->SpellFamilyFlags[0] & 0x00040000 && unitTarget->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DEATHKNIGHT, 0, 0, 0x00000002, m_caster->GetGUID()))
                {
                    damage += damage / 2;
                    damage += int32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK)* 0.035f);
                }
                //// Death Coil - disable adding bonus
                //else if (m_spellInfo->Id == 47632)
                //    apply_direct_bonus = false;
                // Howling Blast
                else if (m_spellInfo->Id == 49184)
                {
                    damage += int32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK)*0.44f);
                    if (this->m_targets.getUnitTargetGUID() != unitTarget->GetGUID()) // 50% of damage to non-primary targets
                        damage = int32(damage * 0.5f);
                }
                else if (m_spellInfo->Id == 48963 // Morbidity
                    || m_spellInfo->Id == 49564
                    || m_spellInfo->Id == 49565)
                {
                    if (effIndex != EFFECT_1)
                        break;

                    if (AuraEffect * aurEff = m_caster->GetAuraEffect(m_spellInfo->Id, EFFECT_1,0))
                    {
                        int bonusDmg = 100 + aurEff->GetAmount();
                        damage = (damage * bonusDmg) / 100;
                    }
                }
                break;
            }
        }

        if (m_originalCaster && damage > 0 && apply_direct_bonus)
            damage = m_originalCaster->SpellDamageBonus(unitTarget, m_spellInfo, effIndex, (uint32)damage, SPELL_DIRECT_DAMAGE);

        // Dont forget count damage reduction auras if damage was manually computed !!!
        if (apply_direct_bonus == false)
        {
            float DoneTotalMod = 1.0f;

            // Some spells don't benefit from pct done mods
            if (!(m_spellInfo->AttributesEx6 & SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS) && m_originalCaster)
            {
                Unit::AuraEffectList const &mModDamagePercentDone = m_originalCaster->GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
                for (Unit::AuraEffectList::const_iterator i = mModDamagePercentDone.begin(); i != mModDamagePercentDone.end(); ++i)
                if (((*i)->GetMiscValue() & GetSpellSchoolMask(m_spellInfo)) &&
                    (*i)->GetSpellProto()->EquippedItemClass == -1 &&          // -1 == any item class (not wand)
                    (*i)->GetSpellProto()->EquippedItemInventoryTypeMask == 0) // 0 == any inventory type (not wand)
                {
                    if (m_spellInfo->SpellFamilyName == SPELLFAMILY_PALADIN) // Restrict this bonus only for paladins damage abilities
                                                                             // otherwise in some spell is bonus counted twice
                        DoneTotalMod *= ((*i)->GetAmount() + 100.0f) / 100.0f;
                }
            }

            damage *= DoneTotalMod;


            float TakenTotalMod = 1.0f;
            TakenTotalMod *= unitTarget->GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN, m_spellInfo->SchoolMask);

            // Exception for Cheating Death ( dummy aura )
            Aura * cheatingDeathAura = unitTarget->GetAura(45182);
            if (cheatingDeathAura && cheatingDeathAura->GetEffect(EFFECT_0))
                AddPctN(TakenTotalMod, cheatingDeathAura->GetEffect(EFFECT_0)->GetAmount());

            damage *= TakenTotalMod;
        }

        m_damage += damage;
    }

    // Some special cases after damage recount
    switch (m_spellInfo->Id)
    {
        // Ice Lance - special case (drop charge of Fingers of Frost)
        case 30455:
            if(Aura* pAura = m_caster->GetAura(44544))
                if (pAura->ModStackAmount(-1))
                    m_caster->RemoveAurasDueToSpell(44544);
            break;
        case 8092:  // Mind Blast
        case 73510: // Mind Spike
        {
            // Increase damage done by every shadow orb stack
            if (Aura* pOrbs = m_caster->GetAura(77487))
                if (pOrbs->GetStackAmount() > 0)
                {
                    // 10% base bonus
                    float coef = 0.1f;
                    int32 es_bp0 = 10;
                    int32 es_bp1 = 0;

                    // Implementation of Shadow Orbs Power mastery proficiency
                    if (m_spellInfo->SpellFamilyName == SPELLFAMILY_PRIEST &&
                        m_caster->ToPlayer() && m_caster->ToPlayer()->HasMastery() &&
                        m_caster->ToPlayer()->GetTalentBranchSpec(m_caster->ToPlayer()->GetActiveSpec()) == SPEC_PRIEST_SHADOW &&
                        m_damage > 1)
                    {
                        coef += m_caster->ToPlayer()->GetMasteryPoints() * 1.45f / 100.0f;
                        es_bp0 += ceil(m_caster->ToPlayer()->GetMasteryPoints()*1.45f);
                    }
                    m_damage = float(m_damage) * (1.0f + (pOrbs->GetStackAmount() * coef));

                    es_bp1 = es_bp0;

                    // Empowered Shadows buff for increased DoT damage
                    m_caster->CastCustomSpell(m_caster, 95799, &es_bp0, &es_bp1, 0, true);

                    // Consume Shadow Orbs
                    m_caster->RemoveAurasDueToSpell(77487);
                }
            break;
        }
        // Shadow Word: Death
        case 32379:
        {
            if (unitTarget->GetHealthPct() <= 25.0f) // At or below 25 %
            {
                if(m_caster->HasAura(14910)) // Mind Melt (Rank 1)
                    m_damage *= 1.15f;

                if(m_caster->HasAura(33371)) // Mind Melt (Rank 2)
                    m_damage *= 1.30f;
            }

            // Deals three times as much damage to targets below 25%
            if (unitTarget->GetHealthPct() < 25.0f)
                m_damage *= 3;

            // Target is not killed
            if (m_damage < int32(unitTarget->GetHealth()))
            {
                // Deals damage equal to damage done to caster
                int32 back_damage = m_damage;
                // Pain and Suffering reduces damage
                if (AuraEffect * aurEff = m_caster->GetDummyAuraEffect(SPELLFAMILY_PRIEST, 2874, 1))
                    back_damage += aurEff->GetAmount() * back_damage / 100; // += due to - basepoint in spelleffect
                m_caster->CastCustomSpell(m_caster, 32409, &back_damage, 0, 0, true);

                if (unitTarget->GetHealthPct() <= 25.0f)
                {
                    // Glyph of Shadow Word: Death
                    if (!m_caster->ToPlayer()->HasSpellCooldown(55682) && m_caster->HasAura(55682))
                        m_caster->CastSpell(m_caster, 77691, true); // dummy hack!
                }
            }
            break;
        }
        // Frostbolt (mage)
        case 116:
        {
            // Frostbolt is involved in some other things, so we must ensure that mage is the caster
            if (!m_caster || !unitTarget || m_caster->getClass() != CLASS_MAGE)
                break;

            // talent Shatter - target must be frozen
            if (unitTarget->isFrozen())
            {
                if (m_caster->HasAura(11170))
                    m_damage *= 1.1f;
                else if (m_caster->HasAura(12982))
                    m_damage *= 1.2f;
            }
            break;
        }
        // Incinerate (warlock)
        case 29722:
        {
            if (!m_caster || !unitTarget || m_caster->getClass() != CLASS_WARLOCK)
                break;

            // If the target is affected by Immolate spell, let's increase damage by 1/6 (info by DBC tooltip)
            if (unitTarget->HasAura(348))
                m_damage += int32((float)m_damage/6.0f);
            break;
        }
        // Haunt
        case 48181:
        {
            if (!m_caster || m_caster->GetTypeId() != TYPEID_PLAYER)
                break;

            m_damage += m_caster->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + SPELL_SCHOOL_SHADOW) * 0.5577f * 1.25f;
            break;
        }
        // Shadowburn
        case 17877:
        {
            // Glyph of Shadowburn implementation
            if (m_caster && unitTarget->GetHealthPct() <= 20.0f // 20% hp
                && m_damage < int32(unitTarget->GetHealth()) // target is still alve
                && m_caster->HasAura(56229) // has glyph
                && !m_caster->ToPlayer()->HasSpellCooldown(56229)) // without cd
            {
                m_caster->CastSpell(m_caster, 77691, true); // dummy hack!
            }
            break;
        }
        // Soul Fire
        case 6353:
        {
            // Improved Soul Fire talent
            int32 bp0 = 0;
            if (m_caster->HasAura(18119))
                bp0 = 4;
            else if (m_caster->HasAura(18120))
                bp0 = 8;

            if (bp0)
                m_caster->CastCustomSpell(m_caster, 85383, &bp0, 0, 0, true);

            // Burning Embers talent
            bp0 = 0;
            if (m_caster->HasAura(85112))
                bp0 = 50;
            else if (m_caster->HasAura(91986))
                bp0 = 25;

            // We must calculate maximal basepoints manually
            int32 maxbp = m_caster->SpellBaseDamageBonus(SPELL_SCHOOL_MASK_SPELL);
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
                pScaling.Init(m_caster->getLevel(), pSpell);
                maxbp = (maxbp + pScaling.avg[1])/7;
                bp0 *= (damage/100)/7;
                if (bp0 > maxbp)
                    bp0 = maxbp;
                // If we already have aura like this, only refresh if damage is lower
                if (unitTarget->HasAura(85421) && unitTarget->GetAura(85421)->GetEffect(0) &&
                    unitTarget->GetAura(85421)->GetEffect(0)->GetAmount() >= bp0)
                    unitTarget->GetAura(85421)->RefreshDuration();
                else
                    m_caster->CastCustomSpell(unitTarget, 85421, &bp0, 0, 0, true);
            }
            break;
        }
        default:
            break;
    }

    // Implementation of demonolgy warlock mastery Master Demonologist proficiency
    if (m_spellInfo->SpellFamilyName == SPELLFAMILY_WARLOCK && m_caster->isPet())
    {
        Player* pOwner = m_caster->GetCharmerOrOwnerPlayerOrPlayerItself();
        if (pOwner && pOwner->HasMastery() &&
            pOwner->GetTalentBranchSpec(pOwner->GetActiveSpec()) == SPEC_WARLOCK_DEMONOLOGY)
        {
            m_damage = m_damage * (1+(pOwner->GetMasteryPoints()*2.3f/100.0f));
        }
    }

    if (m_spellInfo->SpellFamilyName == SPELLFAMILY_MAGE)
    {
        // Implementation of Mana Adept mage mastery proficiency
        if (m_caster->ToPlayer() && m_caster->ToPlayer()->HasMastery() &&
            m_caster->ToPlayer()->GetTalentBranchSpec(m_caster->ToPlayer()->GetActiveSpec()) == SPEC_MAGE_ARCANE &&
            damage > 1)
        {
            // Get mana percentage (0.0f - 1.0f)
            float manapct = float(m_caster->GetPower(POWER_MANA)) / float(m_caster->GetMaxPower(POWER_MANA));
            // multiplier formula: 1 + (mastery*1.5*(%mana remain)/100)
            m_damage = m_damage*(1.0f+m_caster->ToPlayer()->GetMasteryPoints()*1.5f*manapct/100.0f);
        }

        // Implementation of Frostburn mage mastery proficiency
        if (m_caster->ToPlayer() && m_caster->ToPlayer()->HasMastery() &&
            m_caster->ToPlayer()->GetTalentBranchSpec(m_caster->ToPlayer()->GetActiveSpec()) == SPEC_MAGE_FROST &&
            damage > 1)
        {
            if (unitTarget && unitTarget->isFrozen())
                m_damage = m_damage*(1.0f+m_caster->ToPlayer()->GetMasteryPoints()*2.5f/100.0f);
        }
    }

    if (m_caster && m_spellInfo)
    {
        // Implement SPELL_AURA_MOD_DAMAGE_MECHANIC
        Unit::AuraEffectList const& effList = m_caster->GetAuraEffectsByType(SPELL_AURA_MOD_DAMAGE_MECHANIC);
        for (Unit::AuraEffectList::const_iterator itr = effList.begin(); itr != effList.end(); ++itr)
        {
            if ((*itr) && (*itr)->GetMiscValue() == int32(m_spellInfo->Mechanic))
            {
                if ((*itr)->GetAmount())
                    m_damage *= (1+(*itr)->GetAmount()/100.0f);
            }
        }
    }
}

void Spell::EffectDummy(SpellEffIndex effIndex)
{
    if (!unitTarget && !gameObjTarget && !itemTarget)
        return;

    uint32 spell_id = 0;
    int32 bp = 0;
    bool triggered = true;
    SpellCastTargets targets;

    // selection by spell entry for dummy buffs
    switch(m_spellInfo->Id)
    {
        case 1459:          // Arcane Brillance
        case 61316:         // Dalaran Brillance
            m_caster->CastSpell(unitTarget,m_spellInfo->EffectBasePoints[effIndex],true);
            break;
    }

    // selection by spell family
    switch (m_spellInfo->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch (m_spellInfo->Id)
            {
                case 8593:                                  // Symbol of life (restore creature to life)
                case 31225:                                 // Shimmering Vessel (restore creature to life)
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                        return;
                    unitTarget->ToCreature()->setDeathState(JUST_ALIVED);
                    return;
                }
                case 12162:                                 // Deep wounds
                case 12850:                                 // (now good common check for this spells)
                case 12868:
                {
                    if (!unitTarget)
                        return;

                    float damage;
                    // DW should benefit of attack power, damage percent mods etc.
                    // TODO: check if using offhand damage is correct and if it should be divided by 2
                    if (m_caster->haveOffhandWeapon() && m_caster->getAttackTimer(BASE_ATTACK) > m_caster->getAttackTimer(OFF_ATTACK))
                        damage = (m_caster->GetFloatValue(UNIT_FIELD_MINOFFHANDDAMAGE) + m_caster->GetFloatValue(UNIT_FIELD_MAXOFFHANDDAMAGE))/2;
                    else
                        damage = (m_caster->GetFloatValue(UNIT_FIELD_MINDAMAGE) + m_caster->GetFloatValue(UNIT_FIELD_MAXDAMAGE))/2;

                    switch (m_spellInfo->Id)
                    {
                        case 12162: damage *= 0.16f; break; // Rank 1
                        case 12850: damage *= 0.32f; break; // Rank 2
                        case 12868: damage *= 0.48f; break; // Rank 3
                        default:
                            sLog->outError("Spell::EffectDummy: Spell %u not handled in DW",m_spellInfo->Id);
                            return;
                    };

                    // get remaining damage of old Deep Wound aura
                    AuraEffect* deepWound = unitTarget->GetAuraEffect(12721, 0, m_caster->GetGUID());
                    if (deepWound)
                    {
                        int32 remainingTicks = deepWound->GetBase()->GetDuration() / deepWound->GetAmplitude();
                        damage += remainingTicks * deepWound->GetAmount();
                    }

                    // 1 tick/sec * 6 sec = 6 ticks
                    int32 deepWoundsDotBasePoints0 = int32(damage / 6);
                    m_caster->CastCustomSpell(unitTarget, 12721, &deepWoundsDotBasePoints0, NULL, NULL, true, NULL);
                    return;
                }
                case 13567:                                 // Dummy Trigger
                {
                    // can be used for different aura triggering, so select by aura
                    if (!m_triggeredByAuraSpell || !unitTarget)
                        return;

                    switch (m_triggeredByAuraSpell->Id)
                    {
                        case 26467:                         // Persistent Shield
                            m_caster->CastCustomSpell(unitTarget, 26470, &damage, NULL, NULL, true);
                            break;
                        default:
                            sLog->outError("EffectDummy: Non-handled case for spell 13567 for triggered aura %u",m_triggeredByAuraSpell->Id);
                            break;
                    }
                    return;
                }
                case 82174: // Synapse Springs
                {
                    if (!unitTarget || !unitTarget->ToPlayer())
                        return;

                    float intel = unitTarget->ToPlayer()->GetStat(STAT_INTELLECT);
                    float agi = unitTarget->ToPlayer()->GetStat(STAT_AGILITY);
                    float str = unitTarget->ToPlayer()->GetStat(STAT_STRENGTH);

                    if (intel > agi && intel > str)
                        unitTarget->CastSpell(unitTarget, 96230, true);
                    else if (agi > intel && agi > str)
                        unitTarget->CastSpell(unitTarget, 96228, true);
                    else
                        unitTarget->CastSpell(unitTarget, 96229, true);
                    break;
                }
                case 84348: // Invisibility Field
                {
                    if (!unitTarget || unitTarget->isInCombat())
                        return;

                    unitTarget->CastSpell(unitTarget, 82820, true);
                    break;
                }
                case 82626: // Grounded Plasma Shield
                {
                    if (!unitTarget)
                        return;

                    unitTarget->CastSpell(unitTarget, 82627, true);
                    break;
                }
                case 17251:                                 // Spirit Healer Res
                {
                    if (!unitTarget || !m_originalCaster)
                        return;

                    if (m_originalCaster->GetTypeId() == TYPEID_PLAYER)
                    {
                        WorldPacket data(SMSG_SPIRIT_HEALER_CONFIRM, 8);
                        data << uint64(unitTarget->GetGUID());
                        m_originalCaster->ToPlayer()->GetSession()->SendPacket(&data);
                    }
                    return;
                }
                case 20577:                                 // Cannibalize
                    if (unitTarget)
                        m_caster->CastSpell(m_caster, 20578, false, NULL);
                    return;
                case 23019:                                 // Crystal Prison Dummy DND
                {
                    if (!unitTarget || !unitTarget->isAlive() || unitTarget->GetTypeId() != TYPEID_UNIT || unitTarget->ToCreature()->isPet())
                        return;

                    Creature* creatureTarget = unitTarget->ToCreature();

                    m_caster->SummonGameObject(179644, creatureTarget->GetPositionX(), creatureTarget->GetPositionY(), creatureTarget->GetPositionZ(), creatureTarget->GetOrientation(), 0, 0, 0, 0, uint32(creatureTarget->GetRespawnTime()-time(NULL)));
                    sLog->outDebug("SummonGameObject at SpellEfects.cpp EffectDummy for Spell 23019");

                    creatureTarget->ForcedDespawn();

                    return;
                }
                case 23448:                                 // Transporter Arrival - Ultrasafe Transporter: Gadgetzan - backfires
                {
                    int32 r = irand(0, 119);
                    if (r < 20)                           // Transporter Malfunction - 1/6 polymorph
                        m_caster->CastSpell(m_caster, 23444, true);
                    else if (r < 100)                     // Evil Twin               - 4/6 evil twin
                        m_caster->CastSpell(m_caster, 23445, true);
                    else                                    // Transporter Malfunction - 1/6 miss the target
                        m_caster->CastSpell(m_caster, 36902, true);
                    return;
                }
                case 23453:                                 // Gnomish Transporter - Ultrasafe Transporter: Gadgetzan
                    if (roll_chance_i(50))                // Gadgetzan Transporter         - success
                        m_caster->CastSpell(m_caster, 23441, true);
                    else                                    // Gadgetzan Transporter Failure - failure
                        m_caster->CastSpell(m_caster, 23446, true);
                    return;
                case 25860:                                 // Reindeer Transformation
                {
                    if (!m_caster->HasAuraType(SPELL_AURA_MOUNTED))
                        return;

                    float flyspeed = m_caster->GetSpeedRate(MOVE_FLIGHT);
                    float speed = m_caster->GetSpeedRate(MOVE_RUN);

                    m_caster->RemoveAurasByType(SPELL_AURA_MOUNTED);

                    //5 different spells used depending on mounted speed and if mount can fly or not
                    if (flyspeed >= 4.1f)
                        // Flying Reindeer
                        m_caster->CastSpell(m_caster, 44827, true); //310% flying Reindeer
                    else if (flyspeed >= 3.8f)
                        // Flying Reindeer
                        m_caster->CastSpell(m_caster, 44825, true); //280% flying Reindeer
                    else if (flyspeed >= 1.6f)
                        // Flying Reindeer
                        m_caster->CastSpell(m_caster, 44824, true); //60% flying Reindeer
                    else if (speed >= 2.0f)
                        // Reindeer
                        m_caster->CastSpell(m_caster, 25859, true); //100% ground Reindeer
                    else
                        // Reindeer
                        m_caster->CastSpell(m_caster, 25858, true); //60% ground Reindeer

                    return;
                }
                case 26074:                                 // Holiday Cheer
                    // implemented at client side
                    return;
                // Polarity Shift
                case 28089:
                    if (unitTarget)
                        unitTarget->CastSpell(unitTarget, roll_chance_i(50) ? 28059 : 28084, true, NULL, NULL, m_caster->GetGUID());
                    break;
                // Polarity Shift
                case 39096:
                    if (unitTarget)
                        unitTarget->CastSpell(unitTarget, roll_chance_i(50) ? 39088 : 39091, true, NULL, NULL, m_caster->GetGUID());
                    break;
                case 29200:                                 // Purify Helboar Meat
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                        return;

                    uint32 spell_id = roll_chance_i(50)
                        ? 29277                             // Summon Purified Helboar Meat
                        : 29278;                            // Summon Toxic Helboar Meat

                    m_caster->CastSpell(m_caster,spell_id,true,NULL);
                    return;
                }
                case 29858:                                 // Soulshatter
                    if (unitTarget && unitTarget->CanHaveThreatList()
                        && unitTarget->getThreatManager().getThreat(m_caster) > 0.0f)
                        m_caster->CastSpell(unitTarget,32835,true);
                    return;
                case 30458:                                 // Nigh Invulnerability
                    if (!m_CastItem) return;
                    if (roll_chance_i(86))                   // Nigh-Invulnerability   - success
                        m_caster->CastSpell(m_caster, 30456, true, m_CastItem);
                    else                                    // Complete Vulnerability - backfire in 14% casts
                        m_caster->CastSpell(m_caster, 30457, true, m_CastItem);
                    return;
                case 30507:                                 // Poultryizer
                    if (!m_CastItem) return;
                    if (roll_chance_i(80))                   // Poultryized! - success
                        m_caster->CastSpell(unitTarget, 30501, true, m_CastItem);
                    else                                    // Poultryized! - backfire 20%
                        m_caster->CastSpell(unitTarget, 30504, true, m_CastItem);
                    return;
                case 35745:                                 // Socrethar's Stone
                {
                    uint32 spell_id;
                    switch(m_caster->GetAreaId())
                    {
                        case 3900: spell_id = 35743; break; // Socrethar Portal
                        case 3742: spell_id = 35744; break; // Socrethar Portal
                        default: return;
                    }

                    m_caster->CastSpell(m_caster, spell_id, true);
                    return;
                }
                case 37674:                                 // Chaos Blast
                {
                    if (!unitTarget)
                        return;

                    int32 basepoints0 = 100;
                    m_caster->CastCustomSpell(unitTarget, 37675, &basepoints0, NULL, NULL, true);
                    return;
                }
                // Wrath of the Astromancer
                case 42784:
                {
                    uint32 count = 0;
                    for (std::list<TargetInfo>::iterator ihit= m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                        if (ihit->effectMask & (1<<effIndex))
                            ++count;

                    damage = 12000; // maybe wrong value
                    damage /= count;

                    SpellEntry const *spellInfo = sSpellStore.LookupEntry(42784);

                     // now deal the damage
                    for (std::list<TargetInfo>::iterator ihit= m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                        if (ihit->effectMask & (1<<effIndex))
                        {
                            if (Unit* casttarget = Unit::GetUnit((*unitTarget), ihit->targetGUID))
                                m_caster->DealDamage(casttarget, damage, NULL, SPELL_DIRECT_DAMAGE, SPELL_SCHOOL_MASK_ARCANE, spellInfo, false);
                        }

                    return;
                }
                // Demon Broiled Surprise
                case 43723:
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                        return;

                    Player *player = (Player*)m_caster;

                    if (player && player->GetQuestStatus(11379) == QUEST_STATUS_INCOMPLETE)
                    {
                        Creature *creature = player->FindNearestCreature(19973, 10, false);
                        if (!creature)
                        {
                            SendCastResult(SPELL_FAILED_NOT_HERE);
                            return;
                        }

                        player->CastSpell(player, 43753, false);
                    }
                    return;
                }
                case 44875:                                 // Complete Raptor Capture
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                        return;

                    unitTarget->ToCreature()->ForcedDespawn();

                    //cast spell Raptor Capture Credit
                    m_caster->CastSpell(m_caster, 42337, true, NULL);
                    return;
                }
                case 47170:                                 // Impale Leviroth
                {
                    if (!unitTarget && unitTarget->GetEntry() != 26452 && unitTarget->HealthAbovePct(95))
                        return;

                        m_caster->DealDamage(unitTarget, unitTarget->CountPctFromMaxHealth(93));
                        return;
                }
                case 49357:                                 // Brewfest Mount Transformation
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                        return;
                    if (!m_caster->HasAuraType(SPELL_AURA_MOUNTED))
                        return;
                    m_caster->RemoveAurasByType(SPELL_AURA_MOUNTED);
                    // Ram for Alliance, Kodo for Horde
                    if (m_caster->ToPlayer()->GetTeam() == ALLIANCE)
                    {
                        if (m_caster->GetSpeedRate(MOVE_RUN) >= 2.0f)
                            // 100% Ram
                            m_caster->CastSpell(m_caster, 43900, true);
                        else
                            // 60% Ram
                            m_caster->CastSpell(m_caster, 43899, true);
                    }
                    else
                    {
                        if (m_caster->ToPlayer()->GetSpeedRate(MOVE_RUN) >= 2.0f)
                            // 100% Kodo
                            m_caster->CastSpell(m_caster, 49379, true);
                        else
                            // 60% Kodo
                            m_caster->CastSpell(m_caster, 49378, true);
                    }
                    return;
                case 52845:                                 // Brewfest Mount Transformation (Faction Swap)
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                        return;
                    if (!m_caster->HasAuraType(SPELL_AURA_MOUNTED))
                        return;
                    m_caster->RemoveAurasByType(SPELL_AURA_MOUNTED);
                    // Ram for Horde, Kodo for Alliance
                    if (m_caster->ToPlayer()->GetTeam() == HORDE)
                    {
                        if (m_caster->GetSpeedRate(MOVE_RUN) >= 2.0f)
                            // 100% Ram
                            m_caster->CastSpell(m_caster, 43900, true);
                        else
                            // 60% Ram
                            m_caster->CastSpell(m_caster, 43899, true);
                    }
                    else
                    {
                        if (m_caster->ToPlayer()->GetSpeedRate(MOVE_RUN) >= 2.0f)
                            // 100% Kodo
                            m_caster->CastSpell(m_caster, 49379, true);
                        else
                            // 60% Kodo
                            m_caster->CastSpell(m_caster, 49378, true);
                    }
                    return;
                case 55004:                                 // Nitro Boosts
                    if (!m_CastItem)
                        return;
                    if (roll_chance_i(95))                  // Nitro Boosts - success
                        m_caster->CastSpell(m_caster, 54861, true, m_CastItem);
                    else                                    // Knocked Up   - backfire 5%
                        m_caster->CastSpell(m_caster, 46014, true, m_CastItem);
                    return;
                case 50243:                                 // Teach Language
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                        return;

                    // spell has a 1/3 chance to trigger one of the below
                    if (roll_chance_i(66))
                        return;
                    if (m_caster->ToPlayer()->GetTeam() == ALLIANCE)
                    {
                        // 1000001 - gnomish binary
                        m_caster->CastSpell(m_caster, 50242, true);
                    }
                    else
                    {
                        // 01001000 - goblin binary
                        m_caster->CastSpell(m_caster, 50246, true);
                    }

                    return;
                }
                case 51582:                                 //Rocket Boots Engaged (Rocket Boots Xtreme and Rocket Boots Xtreme Lite)
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                        return;

                    if (Battleground* bg = m_caster->ToPlayer()->GetBattleground())
                        bg->EventPlayerDroppedFlag(m_caster->ToPlayer());

                    m_caster->CastSpell(m_caster, 30452, true, NULL);
                    return;
                }
                case 52759:                                 // Ancestral Awakening
                    if (!unitTarget)
                        return;
                    m_caster->CastCustomSpell(unitTarget, 52752, &damage, NULL, NULL, true);
                    return;
                case 54171:                                   //Divine Storm
                {
                    m_caster->CastCustomSpell(unitTarget, 54172, &damage, 0, 0, true);
                    return;
                }
                case 58418:                                 // Portal to Orgrimmar
                case 58420:                                 // Portal to Stormwind
                    return;                                 // implemented in EffectScript[0]
                case 62324: // Throw Passenger
                {
                    if (m_targets.HasTraj())
                    {
                        if (Vehicle *vehicle = m_caster->GetVehicleKit())
                            if (Unit *passenger = vehicle->GetPassenger(damage - 1))
                            {
                                std::list<Unit*> unitList;
                                // use 99 because it is 3d search
                                SearchAreaTarget(unitList, 99, PUSH_DST_CENTER, SPELL_TARGETS_ENTRY, 33114);
                                float minDist = 99 * 99;
                                Vehicle *target = NULL;
                                for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end(); ++itr)
                                {
                                    if (Vehicle *seat = (*itr)->GetVehicleKit())
                                        if (!seat->GetPassenger(0))
                                            if (Unit *device = seat->GetPassenger(2))
                                                if (!device->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
                                                {
                                                    float dist = (*itr)->GetExactDistSq(&m_targets.m_dstPos);
                                                    if (dist < minDist)
                                                    {
                                                        minDist = dist;
                                                        target = seat;
                                                    }
                                                }
                                }
                                if (target && target->GetBase()->IsWithinDist2d(&m_targets.m_dstPos, GetEffectRadius(effIndex) * 2)) // now we use *2 because the location of the seat is not correct
                                    passenger->EnterVehicle(target, 0);
                                else
                                {
                                    passenger->ExitVehicle();
                                    float x, y, z;
                                    m_targets.m_dstPos.GetPosition(x, y, z);
                                    passenger->GetMotionMaster()->MoveJump(x, y, z, m_targets.GetSpeedXY(), m_targets.GetSpeedZ());
                                }
                            }
                    }
                    return;
                }
                case 64385:                                 // Unusual Compass
                {
                    m_caster->SetOrientation(float(urand(0,62832)) / 10000.0f);
                    WorldPacket data;
                    m_caster->BuildHeartBeatMsg(&data);
                    m_caster->SendMessageToSet(&data,true);
                    return;
                }
                case 68996:                                 // Two forms (worgen transformation spell)
                {
                    if(m_caster->GetTypeId() == TYPEID_PLAYER && !m_caster->isInCombat())
                        m_caster->ToPlayer()->toggleWorgenForm(!m_caster->ToPlayer()->isInWorgenForm(), true);
                    return;
                }
                case 53808:                                 // Pygmy Oil
                {
                    Aura *pAura = m_caster->GetAura(53806);
                    if (pAura)
                        pAura->RefreshDuration();
                    else
                    {
                        pAura = m_caster->GetAura(53805);
                        if (!pAura || pAura->GetStackAmount() < 5 || !roll_chance_i(50))
                             m_caster->CastSpell(m_caster, 53805, true);
                        else
                        {
                            pAura->Remove();
                            m_caster->CastSpell(m_caster, 53806, true);
                        }
                    }
                    return;
                }
                case 54577:                                 // U.D.E.D.
                {
                    if (unitTarget->GetEntry() != 29402)
                        return;

                    m_caster->SummonGameObject(192693, unitTarget->GetPositionX(), unitTarget->GetPositionY(), unitTarget->GetPositionZ(), unitTarget->GetOrientation(), 0, 0, 0, 0, 100);
                    for (uint8 i = 0; i < 4; ++i)
                        m_caster->SummonGameObject(191567, float(unitTarget->GetPositionX() + irand(-7, 7)), float(unitTarget->GetPositionY() + irand(-7, 7)), unitTarget->GetPositionZ(), unitTarget->GetOrientation(), 0, 0, 0, 0, 100);

                    unitTarget->Kill(unitTarget);
                    return;
                }
                case 51961:                                 // Captured Chicken Cover - Quest 12702 & 12532
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER
                        || !unitTarget->HasAura(51959)
                        || !(m_caster->ToPlayer()->GetQuestStatus(12702) == QUEST_STATUS_INCOMPLETE || m_caster->ToPlayer()->GetQuestStatus(12532) == QUEST_STATUS_INCOMPLETE))
                        return;

                    m_caster->CastSpell(m_caster, 51037, true);
                    unitTarget->Kill(unitTarget);
                    return;
                }
                case 78741: // Activated!
                {
                    // Triggered spells doesn't take energy, so we must handle it this way
                    if (m_caster)
                        m_caster->ModifyPower(POWER_ENERGY, -1);
                    return;
                }
                case 82674: // Teleport With Error
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                        return;
                    switch (urand(1,25))
                    {
                        case 1: // Silithus - Hive'Zora
                            m_caster->ToPlayer()->TeleportTo(1, -7423.328125f, 1825.405273f, -34.175117f, 3.881345f);
                            break;
                        case 2: // Duskwood - The Darken Bank
                            m_caster->ToPlayer()->TeleportTo(0, -10162.260742f, 328.837677f, 28.624191f, 4.182609f);
                            break;
                        case 3: // Anywhere in Wetlands
                            m_caster->ToPlayer()->TeleportTo(0, 4575.687500f, -3264.132324f, 175.662460f, 4.580755f);
                            break;
                        case 4: // Desolace - Slitherblade Shore
                            m_caster->ToPlayer()->TeleportTo(1, -220.936539f, 2773.268799f, 4.103652f, 3.592412f);
                            break;
                        case 5: // Ungoro - Golakka Hot Springs
                            m_caster->ToPlayer()->TeleportTo(1, -7361.037598f, -695.695496f, -269.397675f, 4.109206f);
                            break;
                        case 6: // Badlands - Crypt
                            m_caster->ToPlayer()->TeleportTo(0, 6569.775879f, -3485.206787f, 313.996948f, 1.531060f);
                            break;
                        case 7: // Western Plaguelands - Mardenholde Keep
                            m_caster->ToPlayer()->TeleportTo(0, 2938.456299f, -1399.236328f, 184.953110f, 4.183212f);
                            break;
                        case 8: // Tanaris - Steamwheedle Port
                            m_caster->ToPlayer()->TeleportTo(1, -6732.344238f, -4744.461426f, 10.367676f, 3.820166f);
                            break;
                        case 9: // Badlands - Fuselight-by-the-Sea
                            m_caster->ToPlayer()->TeleportTo(0, -6646.642578f, -4819.053711f, 9.158126f, 4.737045f);
                            break;
                        case 10: // Anywhere in Hellfire Peninsula
                            m_caster->ToPlayer()->TeleportTo(530, -127.998326f, 512.378113f, -31.104742f, 2.026362f);
                            break;
                        case 11: // Nothern Barrens - Fray Island
                            m_caster->ToPlayer()->TeleportTo(1, -1698.633911f, -4279.546875f, 11.451643f, 0.264871f);
                            break;
                        case 12: // Tanaris - Lost Rigger Cove
                            m_caster->ToPlayer()->TeleportTo(1, -8079.051758f, -5089.977539f, 28.107094f, 5.152199f);
                            break;
                        case 13: // Anywhere in Felwood
                            m_caster->ToPlayer()->TeleportTo(1, 5981.353027f, -1447.957642f, 434.463440f, 1.400303f);
                            break;
                        case 14: // Moonglade - Nighthaven
                            m_caster->ToPlayer()->TeleportTo(1, 7766.967773f, -2434.168457f, 487.532715f, 6.089598f);
                            break;
                        case 15: // Anywhere in Western Plaguelands
                            m_caster->ToPlayer()->TeleportTo(0, 2333.879395f, -1779.823486f, 96.284035f, 1.334569f);
                            break;
                        case 16: // Anywhere in Nothern Stranglethorn
                            m_caster->ToPlayer()->TeleportTo(0, -12889.212891f, -44.755123f, 20.455288f, 2.872535f);
                            break;
                        case 17: // Badlands - Scar of the Worldbreaker
                            m_caster->ToPlayer()->TeleportTo(0, -7003.808105f, -2646.0f, 301.0f, 0.191165f);
                            break;
                        case 18: // Anywhere in Arathi Highlands
                            m_caster->ToPlayer()->TeleportTo(0, -1161.0f, -2824.0f, 54.0f, 3.172512f);
                            break;
                        case 19: // Anywhere in Blasted Lands
                            m_caster->ToPlayer()->TeleportTo(0, -11846, -2419, 26, 4.817215f);
                            break;
                        case 20: // Cape of the Stranglethorn - Janeiro's Point
                            m_caster->ToPlayer()->TeleportTo(0, -14191, 711, 40, 2.142116f);
                            break;
                        case 21: // Nothern Barrens - Near Crossroad
                            m_caster->ToPlayer()->TeleportTo(1, -299, -2801, 93, 2.263477f);
                            break;
                        case 22: // Searing Gore - Iron Summit
                            m_caster->ToPlayer()->TeleportTo(0,-7095.874023f, -1201.917114f, 354.192505f, 6.186895f);
                            break;
                        case 23: // Anywhere in Wetlands
                            m_caster->ToPlayer()->TeleportTo(0 ,-4005, -1327, 146, 2.980112f);
                            break;
                        case 24: // Anywhere in Blasted Lands
                            m_caster->ToPlayer()->TeleportTo(0, -11948, -3559, 219, 6.252136f);
                            break;
                        case 25: // Dire Maul
                            m_caster->ToPlayer()->TeleportTo(1, -3660, 1092, 220, 3.053623f);
                            break;
                    }
                    break;
                }
                case 47468: // Claw (risen ghoul spell)
                {
                    if (!m_caster->GetCharmerOrOwnerPlayerOrPlayerItself() || m_caster->GetCharmerOrOwnerPlayerOrPlayerItself()->GetActiveTalentBranchSpec() != SPEC_DK_UNHOLY)
                        break;

                    if (m_caster->HasAura(63560))
                        m_caster->CastSpell(unitTarget, 91778, true);
                    else
                        m_caster->CastSpell(unitTarget, 91776, true);
                    break;
                }
                case 47484: // Huddle (risen ghoul spell)
                {
                    if (!m_caster->GetCharmerOrOwnerPlayerOrPlayerItself() || m_caster->GetCharmerOrOwnerPlayerOrPlayerItself()->GetActiveTalentBranchSpec() != SPEC_DK_UNHOLY)
                        break;

                    if (m_caster->HasAura(63560))
                        m_caster->CastSpell(unitTarget, 91837, true);
                    else
                        m_caster->CastSpell(unitTarget, 91838, true);
                    break;
                }
                case 47482: // Leap (risen ghoul spell)
                {
                    if (!m_caster->GetCharmerOrOwnerPlayerOrPlayerItself() || m_caster->GetCharmerOrOwnerPlayerOrPlayerItself()->GetActiveTalentBranchSpec() != SPEC_DK_UNHOLY)
                        break;

                    if (m_caster->HasAura(63560))
                    {
                        m_caster->CastSpell(unitTarget, 91807, true);
                        m_caster->CastSpell(unitTarget, 91802, true);
                    }
                    else
                        m_caster->CastSpell(unitTarget, 91809, true);
                    break;
                }
                case 99130: // Seeping Venom (Cinderweb Spiderling - Beth'tilac)
                {
                    m_caster->CastSpell(unitTarget, m_spellInfo->EffectBasePoints[effIndex], true);
                    break;
                }
            }

            break;
        }
        case SPELLFAMILY_HUNTER:
            // Steady Shot
            if (m_spellInfo->SpellFamilyFlags[1] & 0x1)
            {
                // focus effect (it has its own skill for this)
                SpellEntry const* energizeSpell = sSpellStore.LookupEntry(77443);
                if (energizeSpell)
                {
                    int32 bp0 = energizeSpell->EffectBasePoints[0];
                    if (unitTarget && unitTarget->GetHealthPct() <= 25.0f)
                    {
                        // Termination
                        if (m_caster->HasAura(83490))
                            bp0 += 6;
                        else if (m_caster->HasAura(83489))
                            bp0 += 3;
                    }
                    m_caster->CastCustomSpell(m_caster,77443,&bp0,0,0,true);
                }

                // Improved Steady Shot proc
                if (m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->ToPlayer()->GetHistorySpell(2) == 56641)
                {
                    int32 bp0 = 0;
                    if (AuraEffect* pEffect = m_caster->GetDummyAuraEffect(SPELLFAMILY_HUNTER, 3409, EFFECT_0))
                    {
                        bp0 = pEffect->GetAmount();
                        m_caster->CastCustomSpell(m_caster, 53220, &bp0, 0, 0, true);
                        // Avoid double proc (1-1-0-0 = proc, next cast 1-1-1-0 = proc, etc..)
                        m_caster->ToPlayer()->AddNonTriggeredSpellcastHistory(sSpellStore.LookupEntry(53220));
                    }
                }
            }
            // Camouflage
            if (m_spellInfo->SpellFamilyFlags[2] & 0x20)
            {
                m_caster->CastSpell(m_caster, 51755, true); // main aura
                m_caster->CastSpell(m_caster, 80326, true); // periodic stealth trigger aura
            }
            // Master's Call
            else if (m_spellInfo->Id == 53271)
            {
                if (m_caster)
                {
                    Unit* pPet = Unit::GetUnit(*m_caster, m_caster->GetPetGUID());
                    if (unitTarget && pPet)
                    {
                        pPet->RemoveMovementImpairingAuras();
                        unitTarget->RemoveMovementImpairingAuras();
                        pPet->CastSpell(unitTarget, 54216, true);
                        pPet->CastSpell(pPet, 62305, true);
                    }
                }
            }
            break;
        case SPELLFAMILY_MAGE:
        {
            if (!m_caster)
                break;

            uint8 caster_level = m_caster->getLevel();
            switch(m_spellInfo->Id)
            {
                // Conjure Refreshment
                case 42955:
                    {
                        uint32 n_spell = 0;
                        if (caster_level >= 85)
                            n_spell = 92727;
                        else if (caster_level >= 80)
                            n_spell = 42956;
                        else if (caster_level >= 74)
                            n_spell = 74625;
                        else if (caster_level >= 64)
                            n_spell = 92805;
                        else if (caster_level >= 54)
                            n_spell = 92802;
                        else if (caster_level >= 44)
                            n_spell = 92799;
                        else
                            n_spell = 92739;

                        m_caster->CastSpell(m_caster,n_spell,true);
                        break;
                    }
                case 1459: // Arcane Brilliance
                    {
                        if (m_caster->GetTypeId() == TYPEID_PLAYER)
                        {
                            std::list<Unit*> PartyMembers;
                            m_caster->GetPartyMembers(PartyMembers);
                            bool Continue = false;
                            uint32 player = 0;
                            for (std::list<Unit*>::iterator itr = PartyMembers.begin(); itr != PartyMembers.end(); ++itr) // If caster is in party with a player
                            {
                                ++player;
                                if (Continue == false && player > 1)
                                    Continue = true;
                            }
                            if (Continue == true)
                                m_caster->CastSpell(unitTarget, 79058, true); // Arcane Brilliance (For all)
                            else
                                m_caster->CastSpell(unitTarget, 79057, true); // Arcane Brilliance (Only for caster)
                        }
                        break;
                    }
                case 82731: // Flame Orb
                {
                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                        m_caster->CastSpell(m_caster, 84765, true); // Summon Flame Orb
                    break;
                }
                case 92283: // Frostfire Orb
                {
                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                        m_caster->CastSpell(m_caster, 84714, true); // Summon Frostfire Orb
                    break;
                }
                case 43987: // Ritual of Refreshment
                {
                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    {
                        m_caster->ToPlayer()->RemoveSpellCooldown(74650, true); // Rank 1
                        m_caster->ToPlayer()->RemoveSpellCooldown(92824, true); // Rank 2
                        m_caster->ToPlayer()->RemoveSpellCooldown(92827, true); // Rank 3
                        if (m_caster->getLevel() > 75 && m_caster->getLevel() < 80)
                            m_caster->CastSpell(m_caster, 74650, true);
                        if (m_caster->getLevel() > 80 && m_caster->getLevel() < 85)
                            m_caster->CastSpell(m_caster, 92824, true);
                        if (m_caster->getLevel() == 85)
                            m_caster->CastSpell(m_caster, 92827, true);
                    }
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_WARRIOR:
            // Charge
            if (m_spellInfo->SpellFamilyFlags & SPELLFAMILYFLAG_WARRIOR_CHARGE && m_spellInfo->SpellVisual[0] == 867)
            {
                int32 chargeBasePoints0 = damage;
                m_caster->CastCustomSpell(m_caster, 34846, &chargeBasePoints0, NULL, NULL, true);

                //Juggernaut crit bonus & Cooldown
                if (m_caster->HasAura(64976))
                {
                    m_caster->CastSpell(m_caster, 65156, true);
                    m_caster->CastSpell(m_caster, 96216, true);
                }
                return;
            }
            //Slam
            if (m_spellInfo->SpellFamilyFlags[0] & SPELLFAMILYFLAG_WARRIOR_SLAM && m_spellInfo->SpellIconID == 559)
            {
                m_caster->CastSpell(unitTarget,50783,true);
                // Single-minded fury (warrior): Slam off-hand
                if (m_caster->HasAura(81099) && m_caster->haveOffhandWeapon())
                    m_caster->CastSpell(unitTarget,97992,true);
                return;
            }
            // Concussion Blow
            if (m_spellInfo->SpellFamilyFlags[0] & SPELLFAMILYFLAG_WARRIOR_CONCUSSION_BLOW)
            {
                m_damage+= uint32(damage * m_caster->GetTotalAttackPowerValue(BASE_ATTACK) / 100);
                return;
            }
            switch(m_spellInfo->Id)
            {
                // Bloodthirst
                case 23881:
                {
                    m_caster->CastCustomSpell(unitTarget, 23885, &damage, NULL, NULL, true, NULL);
                    return;
                }
                // Intercept
                case 20252:
                {
                    //Juggernaut CD part
                    if (m_caster->HasAura(64976))
                        m_caster->CastSpell(m_caster, 96215, true);
                    return;
                }
                case 97462: // Rallying Cry
                {
                    int32 bp0 = unitTarget->GetMaxHealth()*(float)damage/100.0f;

                    m_caster->CastCustomSpell(unitTarget, 97463, &bp0, 0, 0, true);
                    break;
                }
            }
            break;
        case SPELLFAMILY_WARLOCK:
            // Life Tap
            if (m_spellInfo->SpellFamilyFlags[0] & SPELLFAMILYFLAG_WARLOCK_LIFETAP)
            {
                float coef = 1.2f;

                // Improved Life Tap
                if(m_caster->HasAura(18183))
                    coef += 0.1f;
                else if(m_caster->HasAura(18182))
                    coef += 0.2f;

                int32 damage = float(m_caster->GetMaxHealth())*0.15f;
                int32 mana = float(damage)*coef;

                if (unitTarget && (int32(unitTarget->GetHealth()) > damage))
                {
                    // Shouldn't Appear in Combat Log
                    unitTarget->ModifyHealth(-damage);

                    // Improved Life Tap mod
                    if (AuraEffect const * aurEff = m_caster->GetDummyAuraEffect(SPELLFAMILY_WARLOCK, 208, 0))
                        mana = (aurEff->GetAmount() + 100)* mana / 100;

                    m_caster->CastCustomSpell(unitTarget, 31818, &mana, NULL, NULL, true);

                    // Mana Feed
                    int32 manaFeedVal = 0;
                    if (AuraEffect const * aurEff = m_caster->GetAuraEffect(SPELL_AURA_ADD_FLAT_MODIFIER, SPELLFAMILY_WARLOCK, 1982, 0))
                        manaFeedVal = aurEff->GetAmount();

                    if (manaFeedVal > 0)
                    {
                        manaFeedVal = manaFeedVal * mana / 100;
                        m_caster->CastCustomSpell(m_caster, 32553, &manaFeedVal, NULL, NULL, true, NULL);
                    }
                }
                else
                    SendCastResult(SPELL_FAILED_FIZZLE);
                return;
            }
            // Soul Link activation
            else if (m_spellInfo->Id == 19028)
            {
                int32 bp0 = 20;

                if (m_caster->HasAura(63312)) // Glyph of Soul Link
                    bp0 = 25;

                 unitTarget->CastCustomSpell(unitTarget, 25228,&bp0, 0, 0, true);
            }
            break;
        case SPELLFAMILY_DRUID:
            // Starfall
            if (m_spellInfo->SpellFamilyFlags[2] & SPELLFAMILYFLAG2_DRUID_STARFALL)
            {
                //Mounting cancels Starfall the effect.
                if (m_caster->IsMounted())
                {
                    if (m_triggeredByAuraSpell)
                        m_caster->RemoveAurasDueToSpell(m_triggeredByAuraSpell->Id);
                    return;
                }

                //Any effect which causes you to lose control of your character will supress the starfall effect.
                if (m_caster->hasUnitState(UNIT_STAT_CONTROLLED))
                    return;

                //Max 20 stars
                if (AuraEffect* eff = m_caster->GetAuraEffect(48505,EFFECT_0)) //periodic trigger spell aura effect
                {
                    int32 count = eff->GetAmount();
                    if (count >= 20)
                        return;
                    else
                        eff->SetAmount(++count);
                }

                m_caster->CastSpell(unitTarget, damage, true);
                return;
            }

            switch(m_spellInfo->Id)
            {
                case 80964: // Skull Bash (bear)
                case 80965: // Skull Bash (cat)
                {
                    // Trigger spell for charge and for interrupt spellcast
                    m_caster->CastSpell(unitTarget, 93983, true);
                    m_caster->CastSpell(unitTarget, 93985, true);

                    // Brutal Impact talent
                    if (m_caster->HasAura(16941))
                        m_caster->CastSpell(unitTarget, 82365, true);
                    else if (m_caster->HasAura(16940))
                        m_caster->CastSpell(unitTarget, 82364, true);
                    break;
                }
                case 1126: // Mark of the Wild
                {
                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    {
                        std::list<Unit*> PartyMembers;
                        m_caster->GetPartyMembers(PartyMembers);
                        bool Continue = false;
                        uint32 player = 0;
                        for(std::list<Unit*>::iterator itr = PartyMembers.begin(); itr != PartyMembers.end(); ++itr) // If caster is in party with a player
                        {
                            ++player;
                            if (Continue == false && player > 1)
                                Continue = true;
                        }
                        if (Continue == true)
                            m_caster->CastSpell(unitTarget, 79061, true); // Mark of the Wild (Raid)
                        else
                            m_caster->CastSpell(unitTarget, 79060, true); // Mark of the Wild (Caster)
                    }
                    break;
                }
                case 88751: // Wild Mushroom: Detonate
                {
                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    {
                        Player* pPlayer = m_caster->ToPlayer();
                        Player::GUIDTimestampMap* Mushrooms = pPlayer ? pPlayer->GetSummonMapFor(47649) : NULL;
                        if (Mushrooms && !Mushrooms->empty())
                        {
                            uint32 counter = 0;
                            Creature* pTemp = NULL;
                            for (Player::GUIDTimestampMap::iterator itr = Mushrooms->begin(); itr != Mushrooms->end(); ++itr)
                            {
                                pTemp = Creature::GetCreature(*pPlayer, (*itr).first);
                                if (pTemp)
                                {
                                    if (counter <= 2)
                                        pTemp->CastSpell(pTemp, 78777, true, 0, 0, m_caster->GetGUID());
                                    pTemp->Kill(pTemp);
                                    counter++;
                                }
                            }
                            Mushrooms->clear();
                        }
                    }
                    break;
                }
            }
            break;
        case SPELLFAMILY_PALADIN:
            // Judgement (seal trigger)
            if (m_spellInfo->Id == 20271)
            {
                if (!unitTarget || !unitTarget->isAlive())
                    return;

                uint32 spellId = 54158; // Judgement

                // Seal of Righteousness and Seal of Truth have triggered spell ID in the third effect (dummy)
                Unit::AuraApplicationMap & sealAuras = m_caster->GetAppliedAuras();
                for (Unit::AuraApplicationMap::iterator iter = sealAuras.begin(); iter != sealAuras.end();)
                {
                    Aura * aura = iter->second->GetBase();
                    if (IsSealSpell(aura->GetSpellProto()))
                    {
                        if (AuraEffect * aureff = aura->GetEffect(2))
                            if (aureff->GetAuraType() == SPELL_AURA_DUMMY)
                            {
                                // Judgement of Righteousness, Judgement of Truth
                                if (sSpellStore.LookupEntry(aureff->GetAmount()))
                                    spellId = aureff->GetAmount();
                            }
                        break;
                    }
                    else
                        ++iter;
                }
                m_caster->CastSpell(unitTarget, spellId, true);

                // Long Arm of the Law
                if ((m_caster->HasAura(87168) && roll_chance_i(50)) || m_caster->HasAura(87172))
                {
                    // If target is at or further than 15 yards (6.0f 2D dist)
                    if (unitTarget->GetDistance2d(m_caster) >= 6.0f) 
                        m_caster->CastSpell(m_caster,87173,true);
                }

                // Communion
                if (m_caster->HasAura(31876))
                {
                    // replenishment
                    m_caster->CastSpell(m_caster, 57669, true);
                }
                return;
            }
            switch(m_spellInfo->Id)
            {
                case 31789:                                 // Righteous Defense (step 1)
                {
                    // Clear targets for eff 1
                    for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                        ihit->effectMask &= ~(1<<1);

                    // not empty (checked), copy
                    Unit::AttackerSet attackers = unitTarget->getAttackers();

                    // remove invalid attackers
                    for (Unit::AttackerSet::iterator aItr = attackers.begin(); aItr != attackers.end(); )
                        if (!(*aItr)->canAttack(m_caster))
                            attackers.erase(aItr++);
                        else
                            ++aItr;

                    // selected from list 3
                    uint32 maxTargets = std::min<uint32>(3, attackers.size());
                    for (uint32 i = 0; i < maxTargets; ++i)
                    {
                        Unit::AttackerSet::iterator aItr = attackers.begin();
                        std::advance(aItr, urand(0, attackers.size() - 1));
                        AddUnitTarget(*aItr, 1);
                        attackers.erase(aItr);
                    }

                    // now let next effect cast spell at each target.
                    return;
                }
                // Guardian of Ancient Kings
                case 86150:
                {
                    switch(m_caster->ToPlayer()->GetTalentBranchSpec(m_caster->ToPlayer()->GetActiveSpec()))
                    {
                    case 831: // holy
                        m_caster->CastSpell(m_caster, 86669, true);
                        break;
                    case 839: // protection
                        m_caster->CastSpell(m_caster, 86659, true);
                        break;
                    case 855: // retribution
                        m_caster->CastSpell(m_caster, 86698, true);
                        break;
                    default: break;
                    }
                    return;
                }
                case 19740: // Blessing of Might
                {
                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    {
                        std::list<Unit*> PartyMembers;
                        m_caster->GetPartyMembers(PartyMembers);
                        bool Continue = false;
                        uint32 player = 0;
                        for(std::list<Unit*>::iterator itr = PartyMembers.begin(); itr != PartyMembers.end(); ++itr) // If caster is in party with a player
                        {
                            ++player;
                            if (Continue == false && player > 1)
                                Continue = true;
                        }
                        if (Continue == true)
                            m_caster->CastSpell(unitTarget, 79102, true); // Blessing of Might (Raid)
                        else
                            m_caster->CastSpell(unitTarget, 79101, true); // Blessing of Might (Caster)
                    }
                    break;
                }
                case 20217: // Blessing of Kings
                {
                    if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    {
                        std::list<Unit*> PartyMembers;
                        m_caster->GetPartyMembers(PartyMembers);
                        bool Continue = false;
                        uint32 player = 0;
                        for(std::list<Unit*>::iterator itr = PartyMembers.begin(); itr != PartyMembers.end(); ++itr) // If caster is in party with a player
                        {
                            ++player;
                            if (Continue == false && player > 1)
                                Continue = true;
                        }
                        if (Continue == true)
                            m_caster->CastSpell(unitTarget, 79063, true); // Blessing of Kings (Raid)
                        else
                            m_caster->CastSpell(unitTarget, 79062, true); // Blessing of Kings (Caster)
                    }
                    break;
                }
            }
            break;
        case SPELLFAMILY_SHAMAN:
            // Cleansing Totem Pulse
            if (m_spellInfo->SpellFamilyFlags[0] & SPELLFAMILYFLAG_SHAMAN_TOTEM_EFFECTS && m_spellInfo->SpellIconID == 1673)
            {
                int32 bp1 = 1;
                // Cleansing Totem Effect
                if (unitTarget)
                    m_caster->CastCustomSpell(unitTarget, 52025, NULL, &bp1, NULL, true, NULL, NULL, m_originalCasterGUID);
                return;
            }
            // Healing Stream Totem
            if (m_spellInfo->SpellFamilyFlags[0] & SPELLFAMILYFLAG_SHAMAN_HEALING_STREAM)
            {
                if (!unitTarget)
                    return;

                m_caster->CastCustomSpell(unitTarget, 52042, &damage, 0, 0, true, 0, 0, m_originalCasterGUID);
                return;
            }
            // Mana Spring Totem
            if (m_spellInfo->SpellFamilyFlags[0] & SPELLFAMILYFLAG_SHAMAN_MANA_SPRING)
            {
                if (!unitTarget || unitTarget->getPowerType() != POWER_MANA)
                    return;
                m_caster->CastCustomSpell(unitTarget, 52032, &damage, 0, 0, true, 0, 0, m_originalCasterGUID);
                return;
            }
            // Lava Lash
            if (m_spellInfo->SpellFamilyFlags[2] & SPELLFAMILYFLAG2_SHAMAN_LAVA_LASH)
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    return;

                if (m_caster->ToPlayer()->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND))
                {
                    // Damage is increased by 25% if your off-hand weapon is enchanted with Flametongue.
                    if (m_caster->GetAuraEffect(SPELL_AURA_DUMMY, SPELLFAMILY_SHAMAN, 0x200000, 0, 0))
                        m_damage += m_damage * damage / 100;
                }
                return;
            }
            // Unleash Elements
            else if (m_spellInfo->Id == 73680 && m_caster->GetTypeId() == TYPEID_PLAYER)
            {
                Player* pPlayer = m_caster->ToPlayer();
                if (!pPlayer || !unitTarget)
                    return;

                Item* mainhanditem = pPlayer->GetItemByPos(INVENTORY_SLOT_BAG_0,EQUIPMENT_SLOT_MAINHAND);
                Item* offhanditem = pPlayer->GetItemByPos(INVENTORY_SLOT_BAG_0,EQUIPMENT_SLOT_OFFHAND);
                if (!mainhanditem && !offhanditem)
                    return;

                if (mainhanditem)
                {
                    bool done = false;
                    for (uint32 enchant_slot = PERM_ENCHANTMENT_SLOT; enchant_slot < MAX_ENCHANTMENT_SLOT; ++enchant_slot)
                        if (uint32 enchant_id = mainhanditem->GetEnchantmentId(EnchantmentSlot(enchant_slot)))
                            if (sSpellItemEnchantmentStore.LookupEntry(enchant_id) != NULL)
                            {
                                switch (enchant_id)
                                {
                                case 3021: //Rockbiter
                                    if (unitTarget->IsHostileTo(m_caster))
                                        m_caster->CastSpell(unitTarget,73684,true);
                                    done = true;
                                    break;
                                case 5:    //Flametongue
                                    if (unitTarget->IsHostileTo(m_caster))
                                        m_caster->CastSpell(unitTarget,73683,true);
                                    done = true;
                                    break;
                                case 283:  //Windfury
                                    if (unitTarget->IsHostileTo(m_caster))
                                        m_caster->CastSpell(unitTarget,73681,true);
                                    done = true;
                                    break;
                                case 3345: //Earthliving
                                    if (unitTarget->IsFriendlyTo(m_caster))
                                        m_caster->CastSpell(unitTarget,73685,true);
                                    done = true;
                                    break;
                                case 2:    //Frostbrand
                                    if (unitTarget->IsHostileTo(m_caster))
                                        m_caster->CastSpell(unitTarget,73682,true);
                                    done = true;
                                    break;
                                default:
                                    break;
                                }
                                if (done)
                                    break;
                            }
                }
                if (offhanditem)
                {
                    bool done = false;
                    for (uint32 enchant_slot = PERM_ENCHANTMENT_SLOT; enchant_slot < MAX_ENCHANTMENT_SLOT; ++enchant_slot)
                        if (uint32 enchant_id = offhanditem->GetEnchantmentId(EnchantmentSlot(enchant_slot)))
                            if (sSpellItemEnchantmentStore.LookupEntry(enchant_id) != NULL)
                            {
                                switch (enchant_id)
                                {
                                case 3021: //Rockbiter
                                    if (unitTarget->IsHostileTo(m_caster))
                                        m_caster->CastSpell(unitTarget,73684,true);
                                    done = true;
                                    break;
                                case 5:    //Flametongue
                                    if (unitTarget->IsHostileTo(m_caster))
                                        m_caster->CastSpell(unitTarget,73683,true);
                                    done = true;
                                    break;
                                case 283:  //Windfury
                                    if (unitTarget->IsHostileTo(m_caster))
                                        m_caster->CastSpell(unitTarget,73681,true);
                                    done = true;
                                    break;
                                case 3345: //Earthliving
                                    if (unitTarget->IsFriendlyTo(m_caster))
                                        m_caster->CastSpell(unitTarget,73685,true);
                                    done = true;
                                    break;
                                case 2:    //Frostbrand
                                    if (unitTarget->IsHostileTo(m_caster))
                                        m_caster->CastSpell(unitTarget,73682,true);
                                    done = true;
                                    break;
                                default:
                                    break;
                                }
                                if (done)
                                    break;
                            }
                }
            }
            // Fire Nova
            else if (m_spellInfo->Id == 1535)
            {
                // "ignites your flame shock on all nearby enemies"
                if (unitTarget && unitTarget->GetAura(8050,m_caster->GetGUID()) )
                    m_caster->CastSpell(unitTarget, 8349, true);
            }
            break;
        case SPELLFAMILY_PRIEST:
            // Leap of Faith
            if (m_spellInfo->Id == 73325)
            {
                if(!unitTarget)
                    return;

                Position pos;
                Unit* pTarget;
                GetSummonPosition(effIndex, pos);
                if (Unit *unit = unitTarget->GetVehicleBase())
                    pTarget = unit;
                else
                    pTarget = unitTarget;

                unitTarget->CastSpell(m_caster, 92832, true);   // cast movement spell

                // Body and Soul
                if (m_caster->HasAura(64127))                  // rank #1
                    m_caster->CastSpell(pTarget, 64128, true); // Increase speed of the target by 30%
                else if (m_caster->HasAura(64129))             // rank #2
                    m_caster->CastSpell(pTarget, 65081, true); // Increase speed of the target by 60%

                return;
            }
            if (m_spellInfo->Id == 21562) // Power Word : Fortitude
            {
                if (m_caster->GetTypeId() == TYPEID_PLAYER)
                {
                    std::list<Unit*> PartyMembers;
                    m_caster->GetPartyMembers(PartyMembers);
                    bool Continue = false;
                    uint32 player = 0;
                    for(std::list<Unit*>::iterator itr = PartyMembers.begin(); itr != PartyMembers.end(); ++itr) // If caster is in party with a player
                    {
                        ++player;
                        if (Continue == false && player > 1)
                            Continue = true;
                    }
                    if (Continue == true)
                        m_caster->CastSpell(unitTarget, 79105, true); // Power Word : Fortitude (Raid)
                    else
                        m_caster->CastSpell(unitTarget, 79104, true); // Power Word : Fortitude (Caster)
                }
                break;
            }
            if (m_spellInfo->Id == 27683) // Shadow Protection
            {
                if (m_caster->GetTypeId() == TYPEID_PLAYER)
                {
                    std::list<Unit*> PartyMembers;
                    m_caster->GetPartyMembers(PartyMembers);
                    if (PartyMembers.size() > 2)
                        m_caster->CastSpell(unitTarget, 79107, true); // Shadow Protection (Raid)
                    else
                        m_caster->CastSpell(unitTarget, 79106, true); // Shadow Protection (Caster)
                }
            }
            if (m_spellInfo->Id == 527) // Dispel Magic
            {
                if (!unitTarget)
                    return;
                if (m_caster->IsFriendlyTo(unitTarget))
                    bp = 2;     // 2 effects from friendly target
                else
                    bp = 1;     // 1 effect from enemy target

                spell_id = 97690;
            }
            break;
        case SPELLFAMILY_DEATHKNIGHT:
            // Chains of Ice
            if (m_spellInfo->SpellFamilyFlags[0] & 0x00000004)
            {
                if (m_caster->HasAura(50040))
                    m_caster->CastSpell(unitTarget, 96293, true);
                else if (m_caster->HasAura(50041))
                    m_caster->CastSpell(unitTarget, 96294, true);
            }
            // Death Coil
            if (m_spellInfo->SpellFamilyFlags[0] & SPELLFAMILYFLAG_DK_DEATH_COIL)
            {
                if (m_caster->IsFriendlyTo(unitTarget))
                {
                    int32 bp = (damage + 0.23f * m_caster->GetTotalAttackPowerValue(BASE_ATTACK)) * 3.5 + 1;
                    m_caster->CastCustomSpell(unitTarget, 47633, &bp, NULL, NULL, true);
                }
                else
                {
                    int32 bp = damage + 0.23f * m_caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    m_caster->CastCustomSpell(unitTarget, 47632, &bp, NULL, NULL, true);
                }
                return;
            }
            switch (m_spellInfo->Id)
            {
                case 49998: // Death Strike (main hand only)
                {
                    // Heal for 20% of the damage sustained in 5 preceding seconds
                    bp = int32(m_caster->GetDamageTakenHistory(5) * 20.0f / 100.0f);
                    // Minimum of 7% total health
                    int32 min = int32(m_caster->CountPctFromMaxHealth(7));
                    if (m_caster->HasAura(101568) // Glyph of Dark Succor enabler aura
                        && (m_caster->HasAura(48266) || m_caster->HasAura(48265)))
                    {
                        min = int32(m_caster->CountPctFromMaxHealth(20));
                        m_caster->RemoveAura(101568); // Remove enabler
                    }

                    bp = bp > min ? bp : min;
                    // Improved Death Strike
                    if (AuraEffect const * aurEff = m_caster->GetAuraEffect(SPELL_AURA_ADD_PCT_MODIFIER, SPELLFAMILY_DEATHKNIGHT, 2751, 0))
                        bp = int32(bp * (m_caster->CalculateSpellDamage(m_caster, aurEff->GetSpellProto(), 2) + 100.0f) / 100.0f);
                    m_caster->CastCustomSpell(m_caster, 45470, &bp, NULL, NULL, false);

                    // Implementation of Blood Shield mastery profficiency
                    if (m_caster->ToPlayer() && m_caster->ToPlayer()->HasMastery() &&
                        m_caster->ToPlayer()->GetTalentBranchSpec(m_caster->ToPlayer()->GetActiveSpec()) == SPEC_DK_BLOOD &&
                        m_caster->HasAura(48263)) // in patch 4.1 blood shield activated only in blood presence
                    {
                        // Blood Shield effect stacks
                        int32 bonus = 0;
                        if (AuraEffect *aurEff = m_caster->GetAuraEffect(77535, 0))
                            bonus = aurEff->GetAmount();

                        // 6.25% of amount healed per mastery point, so mastery*6.25 percent
                        int32 bp0 = bp*(m_caster->ToPlayer()->GetMasteryPoints()*6.25f/100.0f) + bonus;
                        // Max Health is a Cap
                        int32 max = m_caster->GetMaxHealth();
                        bp0 = bp0 > max ? max : bp0;
                        m_caster->CastCustomSpell(m_caster,77535,&bp0,0,0,true);
                    }
                    return;
                }
                break;
            case 49560: // Death Grip
                Position pos;
                GetSummonPosition(effIndex, pos);
                if (Unit *unit = unitTarget->GetVehicleBase()) // what is this for?
                    unit->CastSpell(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), damage, true);
                else if (!unitTarget->HasAuraType(SPELL_AURA_DEFLECT_SPELLS)) // Deterrence
                    unitTarget->CastSpell(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), damage, true);

                if (m_caster->GetTypeId() == TYPEID_PLAYER // is Player
                    && m_caster->HasAura(59309)) // Has Glyph of Resilient Grip
                {
                    if (unitTarget->IsImmunedToSpell(sSpellStore.LookupEntry(49560))) // if is target immune
                        m_caster->ToPlayer()->RemoveSpellCooldown(49560, true); // remove spell cooldown
                }
                return;
            case 46584: // Raise Dead
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    return;
                // 46584 has two dummy effects, we will handle only one of them
                if (effIndex != EFFECT_0)
                    return;

                // Do we have talent Master of Ghouls?
                if (m_caster->HasAura(52143))
                    // summon as pet
                    bp = 52150;
                else
                    // or guardian
                    bp = 46585;

                if (m_targets.HasDst())
                    targets.setDst(m_targets.m_dstPos);
                else
                {
                    targets.setDst(*m_caster);
                    triggered = true;
                }
                // Remove cooldown - summon spellls have category
                uint32 cd = m_caster->ToPlayer()->GetSpellCooldownDelay(46584);
                m_caster->ToPlayer()->RemoveSpellCategoryCooldown(m_spellInfo->Category);
                m_caster->ToPlayer()->AddSpellCooldown(46584, 0, cd);
                spell_id = 48289;
                break;
            }
            // Raise dead - take reagents and trigger summon spells
            case 48289:
                if (m_targets.HasDst())
                    targets.setDst(m_targets.m_dstPos);

                spell_id = CalculateDamage(0, NULL);
                break;
            // Raise Ally
            case 61999:
                if (unitTarget && unitTarget->GetTypeId() == TYPEID_PLAYER && m_caster->GetTypeId() == TYPEID_PLAYER &&
                    (unitTarget->ToPlayer()->IsInPartyWith(m_caster) || unitTarget->ToPlayer()->IsInRaidWith(m_caster)))
                    unitTarget->CastSpell(unitTarget, 46619, true);
                break;
            // Gnaw (risen ghoul spell)
            case 47481:
                if (!m_caster->GetCharmerOrOwnerPlayerOrPlayerItself() || m_caster->GetCharmerOrOwnerPlayerOrPlayerItself()->GetActiveTalentBranchSpec() != SPEC_DK_UNHOLY)
                    break;

                if (m_caster->HasAura(63560))
                    m_caster->CastSpell(unitTarget, 91797, true);
                else
                    m_caster->CastSpell(unitTarget, 91800, true);
                break;
            }
            break;
    }

    //spells triggered by dummy effect should not miss
    if (spell_id)
    {
        SpellEntry const *spellInfo = sSpellStore.LookupEntry(spell_id);

        if (!spellInfo)
        {
            sLog->outError("EffectDummy of spell %u: triggering unknown spell id %i\n", m_spellInfo->Id, spell_id);
            return;
        }

        targets.setUnitTarget(unitTarget);
        Spell* spell = new Spell(m_caster, spellInfo, triggered, m_originalCasterGUID, true);
        if (bp) spell->SetSpellValue(SPELLVALUE_BASE_POINT0, bp);
        spell->prepare(&targets);
    }

    // pet auras
    if (PetAura const* petSpell = sSpellMgr->GetPetAura(m_spellInfo->Id,effIndex))
    {
        m_caster->AddPetAura(petSpell);
        return;
    }

    // normal DB scripted effect
    sLog->outDebug("Spell ScriptStart spellid %u in EffectDummy(%u)", m_spellInfo->Id, effIndex);
    m_caster->GetMap()->ScriptsStart(sSpellScripts, uint32(m_spellInfo->Id | (effIndex << 24)), m_caster, unitTarget);

    // Script based implementation. Must be used only for not good for implementation in core spell effects
    // So called only for not proccessed cases
    if (gameObjTarget)
        sScriptMgr->OnDummyEffect(m_caster, m_spellInfo->Id, effIndex, gameObjTarget);
    else if (unitTarget && unitTarget->GetTypeId() == TYPEID_UNIT)
        sScriptMgr->OnDummyEffect(m_caster, m_spellInfo->Id, effIndex, unitTarget->ToCreature());
    else if (itemTarget)
        sScriptMgr->OnDummyEffect(m_caster, m_spellInfo->Id, effIndex, itemTarget);
}

void Spell::EffectTriggerSpellWithValue(SpellEffIndex effIndex)
{
    uint32 triggered_spell_id = m_spellInfo->EffectTriggerSpell[effIndex];

    // normal case
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(triggered_spell_id);

    if (!spellInfo)
    {
        sLog->outError("EffectTriggerSpellWithValue of spell %u: triggering unknown spell id %i", m_spellInfo->Id,triggered_spell_id);
        return;
    }

    int32 bp = damage;
    Unit * caster = GetTriggeredSpellCaster(spellInfo, m_caster, unitTarget);

    caster->CastCustomSpell(unitTarget,triggered_spell_id,&bp,&bp,&bp,true);
}

void Spell::EffectTriggerRitualOfSummoning(SpellEffIndex effIndex)
{
    uint32 triggered_spell_id = m_spellInfo->EffectTriggerSpell[effIndex];
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(triggered_spell_id);

    if (!spellInfo)
    {
        sLog->outError("EffectTriggerRitualOfSummoning of spell %u: triggering unknown spell id %i", m_spellInfo->Id,triggered_spell_id);
        return;
    }

    finish();

    // Have Group, Will Travel - exception, because our spellsystem doesn't support anything bigger
    // force it to be cast to self
    if (spellInfo->Id == 85592 && m_caster->GetTypeId() == TYPEID_PLAYER)
        unitTarget = m_caster;

    m_caster->CastSpell(unitTarget,spellInfo,false);
}

void Spell::EffectForceCast(SpellEffIndex effIndex)
{
    if (!unitTarget)
        return;

    uint32 triggered_spell_id = m_spellInfo->EffectTriggerSpell[effIndex];

    // Dalaran Sewers arena spell Flush triggering deleted spell
    if (triggered_spell_id == 61698)
    {
        if (unitTarget)
            unitTarget->KnockbackFrom(m_caster->GetPositionX(), m_caster->GetPositionY(), 20.0f, 20.0f);
        return;
    }

    // normal case
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(triggered_spell_id);

    if (!spellInfo)
    {
        sLog->outError("EffectForceCast of spell %u: triggering unknown spell id %i", m_spellInfo->Id,triggered_spell_id);
        return;
    }

    if (damage)
    {
        switch(m_spellInfo->Id)
        {
            case 52588: // Skeletal Gryphon Escape
            case 48598: // Ride Flamebringer Cue
                unitTarget->RemoveAura(damage);
                break;
            case 52463: // Hide In Mine Car
            case 52349: // Overtake
                unitTarget->CastCustomSpell(unitTarget, spellInfo->Id, &damage, NULL, NULL, true, NULL, NULL, m_originalCasterGUID);
                return;
            case 72378: // Blood Nova
            case 73058: // Blood Nova
                spellInfo = sSpellMgr->GetSpellForDifficultyFromSpell(spellInfo, m_caster);
                break;
        }
    }
    Unit * caster = GetTriggeredSpellCaster(spellInfo, m_caster, unitTarget);

    caster->CastSpell(unitTarget, spellInfo, true, NULL, NULL, m_originalCasterGUID);
}

void Spell::EffectForceCastWithValue(SpellEffIndex effIndex)
{
    if (!unitTarget)
        return;

    uint32 triggered_spell_id = m_spellInfo->EffectTriggerSpell[effIndex];

    // normal case
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(triggered_spell_id);

    if (!spellInfo)
    {
        sLog->outError("EffectForceCastWithValue of spell %u: triggering unknown spell id %i", m_spellInfo->Id,triggered_spell_id);
        return;
    }
    int32 bp = damage;
    Unit * caster = GetTriggeredSpellCaster(spellInfo, m_caster, unitTarget);

    caster->CastCustomSpell(unitTarget, spellInfo->Id, &bp, &bp, &bp, true, NULL, NULL, m_originalCasterGUID);
}


void Spell::EffectTriggerSpell(SpellEffIndex effIndex)
{
    // only unit case known
    if (!unitTarget)
    {
        if (gameObjTarget || itemTarget)
            sLog->outError("Spell::EffectTriggerSpell (Spell: %u): Unsupported non-unit case!",m_spellInfo->Id);
        return;
    }

    uint32 triggered_spell_id = m_spellInfo->EffectTriggerSpell[effIndex];
    Unit* originalCaster = NULL;
    Unit* triggerTarget = unitTarget;

    const SpellEntry* baseSpellInfo = this->GetSpellInfo();
    if (baseSpellInfo)
    {
        switch(baseSpellInfo->Id)
        {
        case 49376: // Feral Charge (Cat form)
            // Stampede
            if (m_caster->HasAura(78893))
            {
                m_caster->CastSpell(m_caster, 109881, true); // Ravage! (Stampede) enabler spell: Stampede Ravage Marker
                m_caster->CastSpell(m_caster, 81022, true); // rank 2
            }
            else if (m_caster->HasAura(78892))
            {
                m_caster->CastSpell(m_caster, 109881, true); // Ravage! (Stampede) enabler spell: Stampede Ravage Marker
                m_caster->CastSpell(m_caster, 81021, true); // rank 1
            }
            break;
        case 16979: // Feral Charge (Bear form)
            // Stampede
            if (m_caster->HasAura(78893))
                m_caster->CastSpell(m_caster, 81017, true); // rank 2
            else if (m_caster->HasAura(78892))
                m_caster->CastSpell(m_caster, 81016, true); // rank 1
            break;
        default:
            break;
        }
    }

    // special cases
    switch(triggered_spell_id)
    {
        // Mirror Image
        case 58832:
        {
            // Glyph of Mirror Image
            if (m_caster->HasAura(63093))
               m_caster->CastSpell(m_caster, 65047, true); // Mirror Image

            break;
        }
        case 20511: // Intimidating Shout
        {
            if (!m_caster->HasAura(63327)) // Glyph of Intimidating Shout
                return;
            break;
        }
        case 91565: // Feral Aggression
        {
            if (m_caster->GetTypeId() == TYPEID_PLAYER)
            {
                switch (m_caster->ToPlayer()->GetShapeshiftForm())
                {
                    // Should affect only Faerie Fire used in feral forms
                    case FORM_CAT:
                    case FORM_BEAR:
                    case FORM_DIREBEAR:
                    {
                        // more CastSpell is faster than finding an aura and modifying stack count
                        if (m_caster->HasAura(16859))
                        {
                            m_caster->CastSpell(unitTarget, 91565, true, 0, 0, (originalCaster ? originalCaster->GetGUID() : 0));
                            m_caster->CastSpell(unitTarget, 91565, true, 0, 0, (originalCaster ? originalCaster->GetGUID() : 0));
                        }
                        else if (m_caster->HasAura(16858))
                            m_caster->CastSpell(unitTarget, 91565, true, 0, 0, (originalCaster ? originalCaster->GetGUID() : 0));
                        break;
                    }
                    default:
                        break;
                }
            }
            break;
        }
        // Hammer of the Righteous
        case 88263:
        {
            // Trigger this spell to self - this will make the effect work properly
            triggerTarget = m_caster;
            break;
        }
        // Dark Intent
        case 85767:
        {
            // Cast also on self
            unitTarget->CastSpell(m_caster, 85767, true);

            // Mark aura as "casters aura"
            if (m_caster->GetAura(85767) && m_caster->GetAura(85767)->GetEffect(EFFECT_0))
                m_caster->GetAura(85767)->GetEffect(EFFECT_0)->SetScriptedAmount(1);
            break;
        }
        // Snake Trap
        case 57879:
        {
            originalCaster = m_originalCaster;
            // Entrapment
            if (m_originalCaster)
            {
                // Snake Trap and Ice Trap
                if (m_originalCaster->HasAura(19184))
                    m_originalCaster->CastSpell(m_caster,19185,true);
                else if (m_originalCaster->HasAura(19387))
                    m_originalCaster->CastSpell(m_caster,64803,true);
            }
            break;
        }
        // Vanish (not exist, do stuff needed for vanishing)
        case 18461:
        {
            unitTarget->CombatStop(true);
            unitTarget->RemoveMovementImpairingAuras();
            unitTarget->RemoveAurasByType(SPELL_AURA_MOD_STALKED);

            // If this spell is given to an NPC, it must handle the rest using its own AI
            if (unitTarget->GetTypeId() != TYPEID_PLAYER)
                return;

            triggered_spell_id = 11327;
            break;
        }
        // Demonic Empowerment -- succubus
        case 54437:
        {
            unitTarget->RemoveMovementImpairingAuras();
            unitTarget->RemoveAurasByType(SPELL_AURA_MOD_STALKED);
            unitTarget->RemoveAurasByType(SPELL_AURA_MOD_STUN);

            // Cast Lesser Invisibility
            triggered_spell_id = 7870;
            break;
        }
        // Ebon Plague
        case 40515:
            // not existing spell for increasing magic damage taken
            // TODO: find it or create it!
            return;
        // just skip
        case 23770:                                         // Sayge's Dark Fortune of *
            // not exist, common cooldown can be implemented in scripts if need.
            return;
        // Brittle Armor - (need add max stack of 24575 Brittle Armor)
        case 29284:
        {
            // Brittle Armor
            SpellEntry const* spell = sSpellStore.LookupEntry(24575);
            if (!spell)
                return;

            for (uint32 j = 0; j < spell->StackAmount; ++j)
                m_caster->CastSpell(unitTarget, spell->Id, true);
            return;
        }
        // Mercurial Shield - (need add max stack of 26464 Mercurial Shield)
        case 29286:
        {
            // Mercurial Shield
            SpellEntry const* spell = sSpellStore.LookupEntry(26464);
            if (!spell)
                return;

            for (uint32 j = 0; j < spell->StackAmount; ++j)
                m_caster->CastSpell(unitTarget, spell->Id, true);
            return;
        }
        // Righteous Defense
        case 31980:
        {
            m_caster->CastSpell(unitTarget, 31790, true);
            return;
        }
        // Cloak of Shadows
        case 35729:
        {
            Unit::AuraApplicationMap& Auras = unitTarget->GetAppliedAuras();
            for (Unit::AuraApplicationMap::iterator iter = Auras.begin(); iter != Auras.end();)
            {
                // remove all harmful spells on you...
                SpellEntry const* spell = iter->second->GetBase()->GetSpellProto();

                if (spell->SchoolMask & SPELL_SCHOOL_MASK_MAGIC // only magic spells
                    // ignore positive and passive auras
                    && !iter->second->IsPositive() && !iter->second->GetBase()->IsPassive())
                {
                    m_caster->RemoveAura(iter);
                }
                else
                    ++iter;
            }
            return;
        }
        // Priest Shadowfiend (34433) need apply mana gain trigger aura on pet
        case 41967:
        {
            if (Unit *pet = unitTarget->GetGuardianPet())
                pet->CastSpell(pet, 28305, true);
            return;
        }
        // Empower Rune Weapon
        case 53258:
            return; // skip, hack-added in spell effect
        // just make the core stfu
        case 33801: // Coldflame
        case 89024: // Pursuit of Justice
            return;
    }

    // normal case
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(triggered_spell_id);
    if (!spellInfo)
    {
        if (triggered_spell_id != 0) // stop spamming console if triggering was turned off by us
            sLog->outError("EffectTriggerSpell of spell %u: triggering unknown spell id %i", m_spellInfo->Id,triggered_spell_id);
        return;
    }

    // Remove spell cooldown (not category) if spell triggering spell with cooldown and same category
    // Needed by freezing arrow and few other spells
    if (m_caster->GetTypeId() == TYPEID_PLAYER && m_spellInfo->CategoryRecoveryTime && spellInfo->CategoryRecoveryTime
        && m_spellInfo->Category == spellInfo->Category)
        m_caster->ToPlayer()->RemoveSpellCooldown(spellInfo->Id);

    // Note: not exist spells with weapon req. and IsSpellHaveCasterSourceTargets == true
    // so this just for speedup places in else
    Unit * caster = GetTriggeredSpellCaster(spellInfo, m_caster, unitTarget);

    caster->CastSpell(triggerTarget,spellInfo,true, 0, 0, (originalCaster ? originalCaster->GetGUID() : 0));
}

void Spell::EffectTriggerMissileSpell(SpellEffIndex effIndex)
{
    uint32 triggered_spell_id = m_spellInfo->EffectTriggerSpell[effIndex];

    // normal case
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(triggered_spell_id);

    if (!spellInfo)
    {
        sLog->outError("EffectTriggerMissileSpell of spell %u (eff: %u): triggering unknown spell id %u",
            m_spellInfo->Id,effIndex,triggered_spell_id);
        return;
    }

    if (m_CastItem)
        sLog->outStaticDebug("WORLD: cast Item spellId - %i", spellInfo->Id);

    // Remove spell cooldown (not category) if spell triggering spell with cooldown and same category
    // Needed by freezing arrow and few other spells
    if (m_caster->GetTypeId() == TYPEID_PLAYER && m_spellInfo->CategoryRecoveryTime && spellInfo->CategoryRecoveryTime
        && m_spellInfo->Category == spellInfo->Category)
        m_caster->ToPlayer()->RemoveSpellCooldown(spellInfo->Id);

    float x, y, z;
    m_targets.m_dstPos.GetPosition(x, y, z);

    // Hunter talent Entrapment exception
    if (triggered_spell_id == 13810)
    {
        if (m_originalCaster)
        {
            // Snake Trap and Ice Trap
            if (m_originalCaster->HasAura(19184))
                m_originalCaster->CastSpell(x,y,z,19185,true);
            else if (m_originalCaster->HasAura(19387))
                m_originalCaster->CastSpell(x,y,z,64803,true);
        }
    }

    m_caster->CastSpell(x, y, z, spellInfo->Id, true, m_CastItem, 0, m_originalCasterGUID);
}

void Spell::EffectJump(SpellEffIndex effIndex)
{
    if (m_caster->isInFlight())
        return;

    float x, y, z;
    if (m_targets.getUnitTarget())
        m_targets.getUnitTarget()->GetContactPoint(m_caster,x,y,z,CONTACT_DISTANCE);
    else if (m_targets.getGOTarget())
        m_targets.getGOTarget()->GetContactPoint(m_caster,x,y,z,CONTACT_DISTANCE);
    else
    {
        sLog->outError("Spell::EffectJump - unsupported target mode for spell ID %u", m_spellInfo->Id);
        return;
    }

    // Imp's spell Flee - special case, master should be target for fleeing
    if (GetSpellInfo()->Id == 93282)
    {
        if (m_caster->isPet() && m_caster->ToPet())
            m_caster->ToPet()->GetOwner()->GetContactPoint(m_caster,x,y,z,CONTACT_DISTANCE);
    }

    float speedXY, speedZ;
    CalculateJumpSpeeds(effIndex, m_caster->GetExactDist2d(x, y), speedXY, speedZ);
    if (m_caster->GetTypeId() == TYPEID_PLAYER)
        m_caster->GetMotionMaster()->MovementExpired(false);
    m_caster->GetMotionMaster()->MoveJump(x, y, z, speedXY, speedZ);
}

void Spell::EffectJumpDest(SpellEffIndex effIndex)
{
    if (m_caster->isInFlight())
        return;

    // Init dest coordinates
    float x, y, z;
    if (m_targets.HasDst())
    {
        m_targets.m_dstPos.GetPosition(x, y, z);
    }
    else
    {
        sLog->outError("Spell::EffectJumpDest - unsupported target mode for spell ID %u", m_spellInfo->Id);
        return;
    }

    float speedXY, speedZ;
    CalculateJumpSpeeds(effIndex, m_caster->GetExactDist2d(x, y), speedXY, speedZ);
    if (m_caster->GetTypeId() == TYPEID_PLAYER)
        m_caster->GetMotionMaster()->MovementExpired(false);
    m_caster->GetMotionMaster()->MoveJump(x, y, z, speedXY, speedZ);
}

void Spell::CalculateJumpSpeeds(uint8 i, float dist, float & speedXY, float & speedZ)
{
    if (m_spellInfo->EffectMiscValue[i])
        speedZ = float(m_spellInfo->EffectMiscValue[i])/10;
    else if (m_spellInfo->EffectMiscValueB[i])
        speedZ = float(m_spellInfo->EffectMiscValueB[i])/10;
    else
        speedZ = 10.0f;
    speedXY = dist * Movement::gravity / speedZ;
}

void Spell::EffectTeleportUnits(SpellEffIndex /*effIndex*/)
{
    if (!unitTarget || unitTarget->isInFlight())
        return;

    // Pre effects
    uint8 uiMaxSafeLevel = 0;
    switch (m_spellInfo->Id)
    {
        case 36563:    // Shadowstep
            if (Unit * target = m_targets.getUnitTarget())
            {
                Position pos;
                target->GetFirstCollisionPosition(pos, 2.0f, M_PI);
                m_targets.setDst(pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ(),target->GetOrientation());
            }
            break;
        case 48129:  // Scroll of Recall
            uiMaxSafeLevel = 40;
        case 60320:  // Scroll of Recall II
            if (!uiMaxSafeLevel)
                uiMaxSafeLevel = 70;
        case 60321:  // Scroll of Recal III
            if (!uiMaxSafeLevel)
                uiMaxSafeLevel = 80;

            if (unitTarget->getLevel() > uiMaxSafeLevel)
            {
                unitTarget->AddAura(60444,unitTarget); //Apply Lost! Aura
                return;
            }
            break;
        case 66550: // teleports outside (Isle of Conquest)
            if (Player* pTarget = unitTarget->ToPlayer())
            {
                if (pTarget->GetTeamId() == TEAM_ALLIANCE)
                    m_targets.setDst(442.24f,-835.25f,44.30f,0.06f,628);
                else
                    m_targets.setDst(1120.43f,-762.11f,47.92f,2.94f,628);
            }
            break;
        case 66551: // teleports inside (Isle of Conquest)
            if (Player* pTarget = unitTarget->ToPlayer())
            {
                if (pTarget->GetTeamId() == TEAM_ALLIANCE)
                    m_targets.setDst(389.57f,-832.38f,48.65f,3.00f,628);
                else
                    m_targets.setDst(1174.85f,-763.24f,48.72f,6.26f,628);
            }
        case 46613: // Wintergrasp Defender Portal spell
        case 54640:
        case 54643:
            {
                if (Player* pTarget = unitTarget->ToPlayer())
                {
                    if (Battlefield* pBf = sBattlefieldMgr.GetBattlefieldToZoneId(pTarget->GetZoneId()))
                        if (pBf->GetTypeId() == BATTLEFIELD_WG)
                            if (GameObject* pPortal = GetClosestGameObjectWithEntry(pTarget, 190763, 10.0f))
                                ((BattlefieldWG*)pBf)->TeleportDefenderWithPortal(pTarget, pPortal->GetGUID());
                }
                // Let it BF handle, not this handler
                return;
            }
            break;
        case 99566: // Meteor Burn (teleport)
            {
                Position pos;
                unitTarget->GetPosition(&pos);
                pos.m_positionZ -= 2;   // fall through the ground
                m_targets.setDst(pos);
            }
            break;
    }

    // If not exist data for dest location - return
    if (!m_targets.HasDst())
    {
        sLog->outError("Spell::EffectTeleportUnits - does not have destination for spell ID %u\n", m_spellInfo->Id);
        return;
    }

    // Init dest coordinates
    uint32 mapid = m_targets.m_dstPos.GetMapId();
    if (mapid == MAPID_INVALID)
        mapid = unitTarget->GetMapId();
    float x, y, z, orientation;
    m_targets.m_dstPos.GetPosition(x, y, z, orientation);

    // In some raids, spells like Killing spree teleport player below floor ( try to add small amount to z axis)
    if (unitTarget->GetMap() && unitTarget->GetMap()->IsRaid())
        z += 0.75f;

    if (!orientation && m_targets.getUnitTarget())
        orientation = m_targets.getUnitTarget()->GetOrientation();
    sLog->outDebug("Spell::EffectTeleportUnits - teleport unit to %u %f %f %f %f\n", mapid, x, y, z, orientation);

    if (mapid == unitTarget->GetMapId())
        unitTarget->NearTeleportTo(x, y, z, orientation, unitTarget == m_caster);
    else if (unitTarget->GetTypeId() == TYPEID_PLAYER)
        unitTarget->ToPlayer()->TeleportTo(mapid, x, y, z, orientation, unitTarget == m_caster ? TELE_TO_SPELL : 0);

    // post effects for TARGET_DST_DB
    switch (m_spellInfo->Id)
    {
        // Dimensional Ripper - Everlook
        case 23442:
        {
            int32 r = irand(0, 119);
            if (r >= 70)                                  // 7/12 success
            {
                if (r < 100)                              // 4/12 evil twin
                    m_caster->CastSpell(m_caster, 23445, true);
                else                                        // 1/12 fire
                    m_caster->CastSpell(m_caster, 23449, true);
            }
            return;
        }
        // Ultrasafe Transporter: Toshley's Station
        case 36941:
        {
            if (roll_chance_i(50))                        // 50% success
            {
                int32 rand_eff = urand(1, 7);
                switch (rand_eff)
                {
                    case 1:
                        // soul split - evil
                        m_caster->CastSpell(m_caster, 36900, true);
                        break;
                    case 2:
                        // soul split - good
                        m_caster->CastSpell(m_caster, 36901, true);
                        break;
                    case 3:
                        // Increase the size
                        m_caster->CastSpell(m_caster, 36895, true);
                        break;
                    case 4:
                        // Decrease the size
                        m_caster->CastSpell(m_caster, 36893, true);
                        break;
                    case 5:
                    // Transform
                    {
                        if (m_caster->ToPlayer()->GetTeam() == ALLIANCE)
                            m_caster->CastSpell(m_caster, 36897, true);
                        else
                            m_caster->CastSpell(m_caster, 36899, true);
                        break;
                    }
                    case 6:
                        // chicken
                        m_caster->CastSpell(m_caster, 36940, true);
                        break;
                    case 7:
                        // evil twin
                        m_caster->CastSpell(m_caster, 23445, true);
                        break;
                }
            }
            return;
        }
        // Dimensional Ripper - Area 52
        case 36890:
        {
            if (roll_chance_i(50))                        // 50% success
            {
                int32 rand_eff = urand(1, 4);
                switch (rand_eff)
                {
                    case 1:
                        // soul split - evil
                        m_caster->CastSpell(m_caster, 36900, true);
                        break;
                    case 2:
                        // soul split - good
                        m_caster->CastSpell(m_caster, 36901, true);
                        break;
                    case 3:
                        // Increase the size
                        m_caster->CastSpell(m_caster, 36895, true);
                        break;
                    case 4:
                        // Transform
                    {
                        if (m_caster->ToPlayer()->GetTeam() == ALLIANCE)
                            m_caster->CastSpell(m_caster, 36897, true);
                        else
                            m_caster->CastSpell(m_caster, 36899, true);
                        break;
                    }
                }
            }
            return;
        }
    }
}

void Spell::EffectApplyAura(SpellEffIndex effIndex)
{
    if (!m_spellAura || !unitTarget)
        return;

    //For some funky reason, some spells have to be cast as a spell on the enemy even if they're supposed to apply an aura.
    switch (m_spellInfo->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            // Soulstone Resurrection - resurrecting dead player, needs handling just before aura would be added
            if (m_spellInfo->Id == 20707)
            {
                if (unitTarget && unitTarget->GetTypeId() == TYPEID_PLAYER && unitTarget->isDead())
                {
                    if (unitTarget->ToPlayer()->isRessurectRequested())       // already have one active request
                        return;

                    // Increase ressurection data after using soulstone
                    if (InstanceScript * pInstance = m_caster->GetInstanceScript())
                    {
                        if (pInstance->instance->IsRaid() && pInstance->IsEncounterInProgress())
                            pInstance->AddRessurectionData();
                    }

                    ExecuteLogEffectResurrect(effIndex, unitTarget);

                    unitTarget->ToPlayer()->setResurrectRequestData(m_caster->GetGUID(), m_caster->GetMapId(), m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ(), unitTarget->GetMaxHealth()*0.3f, unitTarget->GetMaxPower(POWER_MANA)*0.3f);
                    SendResurrectRequest(unitTarget->ToPlayer());

                    m_spellAura->Remove();
                    return;
                }
            }
            break;
        }
        case SPELLFAMILY_ROGUE:
        {
            if (m_spellInfo->SpellFamilyFlags[0] == 0x8)    //Gouge
            {
                if (!IsTriggered())
                    m_caster->CastSpell(unitTarget,1776,true);
                return;
            }
            if (m_spellInfo->Id == 408 || m_spellInfo->Id == 8647)   //Kidney Shot and Expose Armor
            {
                // check whether the target has a Revealing Strike debuff
                Aura *revealing = unitTarget->GetAura(84617, m_caster->GetGUID());

                if(revealing)
                {
                    int duration = m_spellAura->GetDuration();   // get original duration (without revealing strike)

                    float bonus = 0.35f;                          // adds 35% bonus
                    if(m_caster->HasAura(56814))                 // glyph of revealing strike adds additional 10% bonus
                        bonus += 0.10f;

                    duration *= 1 + bonus;
                    m_spellAura->SetDuration(duration);
                    m_spellAura->SetMaxDuration(duration);
                    revealing->Remove();                         // remove the revealing strike debuff
                }
            }
            break;
        }
        case SPELLFAMILY_MAGE:
        {
            // Impact - reset cooldown of Fire Blast
            if (m_spellInfo->Id == 64343)
            {
                m_caster->ToPlayer()->RemoveSpellCooldown(2136, true);
            }
            break;
        }
        case SPELLFAMILY_PRIEST:
        {
            // Inner Focus
            if (m_spellInfo->Id == 89485 && m_caster)
            {
                // Strength of Soul
                if (m_caster->HasAura(89488)) // rank #1
                    m_caster->CastSpell(m_caster, 96266, true);
                else if (m_caster->HasAura(89489)) // rank #2
                    m_caster->CastSpell(m_caster, 96267, true);
            }
            if(m_spellInfo->Id == 41635 && m_caster)
            {
                if (m_spellAura->GetCharges() == 5 && m_caster->HasAura(99134)) //Priest T12 Healer 2P Bonus ( hackfix for PoM triggering )
                {
                    m_caster->CastSpell(m_caster,99132,true); // Divine Fire
                }
            }
            // Implementation of Priests discipline mastery proficiency
            if (m_spellAura->HasEffectType(SPELL_AURA_SCHOOL_ABSORB) && m_spellInfo->Id != 47753) // exclude Divine Aegis (handled elsewhere)
            {
                if (m_caster->ToPlayer() && m_caster->ToPlayer()->HasMastery() &&
                    m_caster->ToPlayer()->GetTalentBranchSpec(m_caster->ToPlayer()->GetActiveSpec()) == SPEC_PRIEST_DISCIPLINE)
                {
                    if (AuraEffect* aurEff = m_spellAura->GetEffect(effIndex))
                        aurEff->ChangeAmount(aurEff->GetAmount()*(1.0f+m_caster->ToPlayer()->GetMasteryPoints()*2.5f/100.0f));
                }
            }
            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            if (m_spellInfo->Id == 17364 && m_caster->HasAura(99213)) // Stormstrike (Shaman T12 Enhancement 4P Bonus)
                m_caster->CastSpell(unitTarget,99212,true);
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            if (m_spellInfo->Id == 53490) // Bullheaded ( hunter's pet ability )
            {
                m_caster->RemoveMovementImpairingAuras();
                m_caster->RemoveAurasByType(SPELL_AURA_MOD_STUN);
                m_caster->CastSpell(m_caster,63896,true);
            }
            break;
        }

    }
    ASSERT(unitTarget == m_spellAura->GetOwner());
    m_spellAura->_ApplyEffectForTargets(effIndex);
}

void Spell::EffectApplyAreaAura(SpellEffIndex effIndex)
{
    if (!m_spellAura || !unitTarget)
        return;
    ASSERT (unitTarget == m_spellAura->GetOwner());
    m_spellAura->_ApplyEffectForTargets(effIndex);
}

void Spell::EffectUnlearnSpecialization(SpellEffIndex effIndex)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    Player *_player = (Player*)unitTarget;
    uint32 spellToUnlearn = m_spellInfo->EffectTriggerSpell[effIndex];

    _player->removeSpell(spellToUnlearn);

    sLog->outDebug("Spell: Player %u has unlearned spell %u from NpcGUID: %u", _player->GetGUIDLow(), spellToUnlearn, m_caster->GetGUIDLow());
}

void Spell::EffectPowerDrain(SpellEffIndex effIndex)
{
    if (m_spellInfo->EffectMiscValue[effIndex] < 0 || m_spellInfo->EffectMiscValue[effIndex] >= int8(MAX_POWERS))
        return;

    Powers powerType = Powers(m_spellInfo->EffectMiscValue[effIndex]);

    if (!unitTarget || !unitTarget->isAlive() || unitTarget->getPowerType() != powerType || damage < 0)
        return;

    // add spell damage bonus
    damage = m_caster->SpellDamageBonus(unitTarget, m_spellInfo, effIndex, uint32(damage), SPELL_DIRECT_DAMAGE);

    int32 power = damage;

    int32 newDamage = -(unitTarget->ModifyPower(powerType, -int32(power)));

    float gainMultiplier = 0.0f;

    // Don`t restore from self drain
    if (m_caster != unitTarget)
    {
        gainMultiplier = SpellMgr::CalculateSpellEffectValueMultiplier(m_spellInfo, effIndex, m_originalCaster, this);

        int32 gain = int32(newDamage * gainMultiplier);

        m_caster->EnergizeBySpell(m_caster, m_spellInfo->Id, gain, powerType);
    }
    ExecuteLogEffectTakeTargetPower(effIndex, unitTarget, powerType, newDamage, gainMultiplier);
}

void Spell::EffectSendEvent(SpellEffIndex effIndex)
{
    /*
    we do not handle a flag dropping or clicking on flag in battleground by sendevent system
    */
    sLog->outDebug("Spell ScriptStart %u for spellid %u in EffectSendEvent ", m_spellInfo->EffectMiscValue[effIndex], m_spellInfo->Id);

    Object *pTarget;
    if (focusObject)
        pTarget = focusObject;
    else if (unitTarget)
        pTarget = unitTarget;
    else if (gameObjTarget)
        pTarget = gameObjTarget;
    else
        pTarget = NULL;

    m_caster->GetMap()->ScriptsStart(sEventScripts, m_spellInfo->EffectMiscValue[effIndex], m_caster, pTarget);
}

void Spell::EffectPowerBurn(SpellEffIndex effIndex)
{
    if (m_spellInfo->EffectMiscValue[effIndex] < 0 || m_spellInfo->EffectMiscValue[effIndex] >= int8(MAX_POWERS))
        return;

    Powers powerType = Powers(m_spellInfo->EffectMiscValue[effIndex]);

    if (!unitTarget || !unitTarget->isAlive() || unitTarget->getPowerType() != powerType || damage < 0)
        return;

    // burn x% of target's mana, up to maximum of 2x% of caster's mana (Mana Burn)
    if (m_spellInfo->ManaCostPercentage)
    {
        int32 maxDamage = m_caster->GetMaxPower(powerType) * damage * 2 / 100;
        damage = unitTarget->GetMaxPower(powerType) * damage / 100;
        damage = std::min(damage, maxDamage);
    }

    int32 power = damage;

    int32 newDamage = -(unitTarget->ModifyPower(powerType, -power));

    // NO - Not a typo - EffectPowerBurn uses effect value multiplier - not effect damage multiplier
    float dmgMultiplier = SpellMgr::CalculateSpellEffectValueMultiplier(m_spellInfo, effIndex, m_originalCaster, this);

    // add log data before multiplication (need power amount, not damage)
    ExecuteLogEffectTakeTargetPower(effIndex, unitTarget, powerType, newDamage, 0.0f);

    newDamage = int32(newDamage * dmgMultiplier);

    m_damage += newDamage;
}

void Spell::EffectHeal(SpellEffIndex /*effIndex*/)
{
}

void Spell::SpellDamageHeal(SpellEffIndex effIndex)
{
    if (unitTarget && unitTarget->isAlive() && damage >= 0)
    {
        // Try to get original caster
        Unit *caster = m_originalCasterGUID ? m_originalCaster : m_caster;

        // Skip if m_originalCaster not available
        if (!caster)
            return;

        int32 addhealth = damage;

        // Empowered Touch talent (procs from Regrowth, Nourish and Healing Touch)
        if (unitTarget && m_caster && (m_spellInfo->Id == 8936 || m_spellInfo->Id == 50464 || m_spellInfo->Id == 5185)
            && unitTarget->HasAura(33763))
        {
            // Rank 1 (50%) and 2 (100%)
            if ((m_caster->HasAura(33879) && roll_chance_i(50))
                || m_caster->HasAura(33880))
            {
                Aura* pAura = unitTarget->GetAura(33763);
                if (pAura)
                    pAura->RefreshDuration();
            }
        }

        // Healing Touch + Glyph of Healing Touch
        if (m_spellInfo->Id == 5185 && m_caster && m_caster->ToPlayer() && m_caster->HasAura(54825))
        {
            // reduce cooldown of Nature's Swiftness
            if (m_caster->ToPlayer()->HasSpellCooldown(17116))
                m_caster->ToPlayer()->ModifySpellCooldown(17116, -10000, true);
        }

        if (m_spellInfo->Id == 96880) // Tipping of the Scales ( trinket )
        {
            if (AuraEffect* pEff = m_caster->GetAuraEffect(96881,EFFECT_0)) //  Weight of a Feather 
            {
                addhealth = pEff->GetAmount();
                pEff->GetBase()->Remove();
            }
        }

        // Vital spark implementation
        if(caster->GetMapId() == 720) // Firelands
        {
            uint32 spellId = 0;

            // Torment debuff
            if (unitTarget->HasAura(99256))
                spellId = 99256;
            if (unitTarget->HasAura(100230))
                spellId = 100230;
            if (unitTarget->HasAura(100231))
                spellId = 100231;
            if (unitTarget->HasAura(100232))
                spellId = 100232;

            if (spellId && m_spellInfo->Id != 52752 && m_spellInfo->Id != 379) // Ancestral Awakening and Earth shield
            {
                uint32 stacks = unitTarget->GetAuraCount(spellId);
                if (stacks && stacks >=3)
                {
                    uint32 sparks= 0;

                    if (spellId == 99256 || spellId == 100231) // On 10 man
                        sparks = stacks / 3;
                    else
                        sparks = stacks / 5;                   // On 25 man

                    if (!caster->HasAura(99263)) // Cant gain spark if caster has Vital flame active on him
                    {
                        for(uint32 i = 0; i < sparks; i++)
                            caster->CastSpell(caster,99262,true); // Vital Spark
                    }
                }
            }
        }

        // Vital flame triggering implementation
        if(caster->GetMapId() == 720) // Firelands
        {
            if (caster->HasAura(99262) && unitTarget->HasAura(99252) ) //Vital Spark, Blaze of Glory
            {
                if (m_spellInfo->Id != 52752 && m_spellInfo->Id != 379)
                {
                    uint32 stacks = caster->GetAuraCount(99262);
                    caster->CastSpell(caster,99263,true); // Vital flame
                    caster->RemoveAurasDueToSpell(99262); // Remove Vital Spark stacks
                    if (AuraEffect* aurEff = caster->GetAuraEffect(99263,EFFECT_0))
                    {
                        if(stacks)
                            aurEff->SetAmount( int32(5 * stacks)); // bonus healing is stored here
                    }
                }
            }
        }

        // Devour magic (Warlock pet heal ability)
        if (m_spellInfo->Id == 19658) 
        {
            if (Unit * pOwner = m_caster->GetOwner())
            {
                if (pOwner->GetTypeId() == TYPEID_PLAYER)
                {
                    addhealth += (int32(pOwner->ToPlayer()->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + SPELL_SCHOOL_SHADOW)) * 0.5f) * 0.3f;
                    addhealth = caster->SpellHealingBonus(unitTarget, m_spellInfo, effIndex, addhealth, HEAL);
                }
            }
        }
        // Word of Glory (paladin holy talent)
        else if (m_spellInfo->Id == 85673)
        {
            //multiply by amount of holy power
            int32 holypower = caster->GetPower(POWER_HOLY_POWER);
            int32 multiplier = holypower;
            bool takePower = true;

            // Divine Purpose effect
            if (caster->HasAura(90174))
            {
                takePower = false;
                multiplier = 3;
                caster->RemoveAurasDueToSpell(90174);
            }

            // This spell retains 0.198 of attack power as heal value
            addhealth += 0.198f*m_caster->GetTotalAttackPowerValue(BASE_ATTACK);

            // Apply spell power bonus
            addhealth = caster->SpellHealingBonus(unitTarget, m_spellInfo, effIndex, addhealth, HEAL);

            addhealth *= multiplier;

            // Selfless Healer
            if (caster != unitTarget)
            {
                int32 bpmod = 0;
                if (caster->HasAura(85804))
                    bpmod = 2;
                else if (caster->HasAura(85803))
                    bpmod = 1;

                if (bpmod)
                {
                    addhealth *= 1.0f+bpmod*0.25f;
                    bpmod *= 2*multiplier;
                    caster->CastCustomSpell(caster, 90811, &bpmod, 0, 0, true);
                }
            }

            // Guarded by Light talent (if healing self, increase health)
            if (caster == unitTarget)
            {
                if (caster->HasAura(85646))
                    addhealth *= 1.1f;
                else if (caster->HasAura(85639))
                    addhealth *= 1.05f;
            }
            // Guarded by Light also allows to proc Holy Shield from Word of Glory
            if ((caster->HasAura(85646) || caster->HasAura(85639)) && caster->HasAura(20925))
                caster->CastSpell(caster,87342,true);
            // and also Guarded by Light rank 2 procs shield that absorbs overhealed health
            if (caster->HasAura(85646))
            {
                int32 missinghealth = unitTarget->GetMaxHealth() - unitTarget->GetHealth();
                if (addhealth > missinghealth)
                {
                    int32 bp0 = addhealth-missinghealth;
                    caster->CastCustomSpell(unitTarget, 88063, &bp0, 0, 0, true);
                }
            }

            //and clear holy power
            if (takePower)
            {
                caster->SetPower(POWER_HOLY_POWER, 0);

                // Eternal Glory returns holy power back (15s cooldown)
                if ((caster->HasAura(87163) && roll_chance_i(15)) ||
                    (caster->HasAura(87164) && roll_chance_i(30)))
                {
                    Player *player = caster->ToPlayer();
                    if (player && !player->HasSpellCooldown(88676))
                    {
                        player->CastCustomSpell(caster, 88676, &holypower, NULL, NULL, true);
                        player->AddSpellAndCategoryCooldowns(sSpellStore.LookupEntry(88676), 0);
                    }
                }
            }
        }
        // Light of Dawn
        else if (m_spellInfo->Id == 85222)
        {
            uint32 holypower = caster->GetPower(POWER_HOLY_POWER) + 1;
            // Divine Purpose effect
            if (caster->HasAura(90174))
            {
                holypower = 3;
                caster->RemoveAurasDueToSpell(90174);
            }

            addhealth = caster->SpellHealingBonus(unitTarget, m_spellInfo, effIndex, addhealth, HEAL);
            addhealth *= holypower;
        }
        // Flash Heal
        else if (m_spellInfo->Id == 2061 && caster->HasAura(88688))
        {
            addhealth = caster->SpellHealingBonus(unitTarget, m_spellInfo, effIndex, addhealth, HEAL);
        }
        // Chain Heal and Riptide
        else if (m_spellInfo->Id == 1064 || m_spellInfo->Id == 61295)
        {
            // Tidal Waves
            if (caster->HasAura(51564) || caster->HasAura(51563) || caster->HasAura(51562))
            {
                int32 bp0 = 0;
                Aura* pAura = caster->GetAura(51564);
                if (!pAura)
                    pAura = caster->GetAura(51563);
                if (!pAura)
                    pAura = caster->GetAura(51562);
                if (pAura)
                    bp0 = -pAura->GetSpellProto()->EffectBasePoints[0];
                if (bp0)
                {
                    int32 bp1 = -bp0;
                    caster->CastCustomSpell(caster,53390,&bp0,&bp1,0,true);
                }
            }

            addhealth = caster->SpellHealingBonus(unitTarget, m_spellInfo, effIndex, addhealth, HEAL);
        }
        // Atonement
        else if (m_spellInfo->Id == 81751 || m_spellInfo->Id == 94472)
        {
            if (caster == unitTarget) // If the Priest is healed through Atonement, the effect is reduced in half
                addhealth = damage / 2;
        }
        // Vessel of the Naaru (Vial of the Sunwell trinket)
        else if (m_spellInfo->Id == 45064)
        {
            // Amount of heal - depends from stacked Holy Energy
            int damageAmount = 0;
            if (AuraEffect const * aurEff = m_caster->GetAuraEffect(45062, 0))
            {
                damageAmount+= aurEff->GetAmount();
                m_caster->RemoveAurasDueToSpell(45062);
            }

            addhealth += damageAmount;
        }
        // Swiftmend - consumes Regrowth or Rejuvenation
        else if (m_spellInfo->TargetAuraState == AURA_STATE_SWIFTMEND && unitTarget->HasAuraState(AURA_STATE_SWIFTMEND, m_spellInfo, m_caster))
        {
            addhealth = caster->SpellHealingBonus(unitTarget, m_spellInfo, effIndex, addhealth, HEAL);

            Unit::AuraEffectList const& RejorRegr = unitTarget->GetAuraEffectsByType(SPELL_AURA_PERIODIC_HEAL);
            // find most short by duration
            AuraEffect *targetAura = NULL;
            for (Unit::AuraEffectList::const_iterator i = RejorRegr.begin(); i != RejorRegr.end(); ++i)
            {
                if ((*i)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_DRUID
                    && (*i)->GetSpellProto()->SpellFamilyFlags[0] & 0x50)
                {
                    if (!targetAura || (*i)->GetBase()->GetDuration() < targetAura->GetBase()->GetDuration())
                        targetAura = *i;
                }
            }

            if (!targetAura)
            {
                sLog->outError("Target(GUID:" UI64FMTD ") has aurastate AURA_STATE_SWIFTMEND but no matching aura.", unitTarget->GetGUID());
                return;
            }

            // Glyph of Swiftmend
            if (!caster->HasAura(54824))
                unitTarget->RemoveAura(targetAura->GetId(), targetAura->GetCasterGUID());

            // T12 Restoration 4P Bonus
            if (caster->HasAura(99015))
                caster->CastCustomSpell(caster, 99017, &addhealth, 0, 0, true); // Firebloom

            // Efflorescence
            int32 bp0 = 0;
            if (caster->HasAura(34151))
                bp0 = addhealth*0.04f;
            else if (caster->HasAura(81274))
                bp0 = addhealth*0.08f;
            else if (caster->HasAura(81275))
                bp0 = addhealth*0.12f;

            if (bp0)
                unitTarget->CastCustomSpell(unitTarget, 81262, &bp0, &bp0, &bp0, true, 0, 0, caster->GetGUID());

            //addhealth += tickheal * tickcount;
            //addhealth = caster->SpellHealingBonus(m_spellInfo, addhealth,HEAL, unitTarget);
        }
        // Glyph of Nourish
        else if (m_spellInfo->SpellFamilyName == SPELLFAMILY_DRUID && m_spellInfo->SpellFamilyFlags[1] & 0x2000000)
        {
            addhealth = caster->SpellHealingBonus(unitTarget, m_spellInfo, effIndex, addhealth, HEAL);

            if (AuraEffect const* aurEff = m_caster->GetAuraEffect(62971, 0))
            {
                Unit::AuraEffectList const& Periodic = unitTarget->GetAuraEffectsByType(SPELL_AURA_PERIODIC_HEAL);
                for (Unit::AuraEffectList::const_iterator i = Periodic.begin(); i != Periodic.end(); ++i)
                {
                    if (m_caster->GetGUID() == (*i)->GetCasterGUID())
                        addhealth += addhealth * aurEff->GetAmount() / 100;
                }
            }
        }
        // Lifebloom - final heal coef multiplied by original DoT stack
        else if (m_spellInfo->Id == 33778)
        {
            addhealth = caster->SpellHealingBonus(unitTarget, m_spellInfo, effIndex, addhealth, HEAL, m_spellValue->EffectBasePoints[1]);

            // Gift of Nature 
            float GoNbonus = (caster->HasAura(87305)) ? 0.25f : 0.0f;

            // Gift of the Earthmother
            if (caster->HasAura(51181))
                addhealth *= (1.15f + GoNbonus);
            else if (caster->HasAura(51180))
                addhealth *= (1.10f + GoNbonus);
            else if (caster->HasAura(51179))
                addhealth *= (1.05f + GoNbonus);
            else if (caster->HasAura(87305)) // If player has only Gift of Nature
                addhealth *= 1.25f;
        }
        // Riptide - increase healing done by Chain Heal
        else if (m_spellInfo->SpellFamilyName == SPELLFAMILY_SHAMAN && m_spellInfo->SpellFamilyFlags[0] & 0x100)
        {
            addhealth = caster->SpellHealingBonus(unitTarget, m_spellInfo, effIndex, addhealth, HEAL);
            if (AuraEffect * aurEff = unitTarget->GetAuraEffect(SPELL_AURA_PERIODIC_HEAL, SPELLFAMILY_SHAMAN, 0, 0, 0x10, m_originalCasterGUID))
            {
                addhealth = int32(addhealth * 1.25f);
                // consume aura
                if (!caster->HasAura(99195)) // Shaman T12 Restoration 4P Bonus
                    unitTarget->RemoveAura(aurEff->GetBase());
            }
        }
        // Death Pact - return pct of max health to caster
        else if (m_spellInfo->SpellFamilyName == SPELLFAMILY_DEATHKNIGHT && m_spellInfo->SpellFamilyFlags[0] & 0x00080000)
            addhealth = caster->SpellHealingBonus(unitTarget, m_spellInfo, effIndex, int32(caster->CountPctFromMaxHealth(damage)), HEAL);
        // Healthstone
        else if (m_spellInfo->Id == 6262)
        {
            addhealth = (float(addhealth)/100.0f)*caster->GetCreateHealth();

            // Glyph of Healthstone
            if (caster->HasAura(56224))
                addhealth *= 1.3f;

            // Soulburn: Healthstone
            if (caster->HasAura(74434))
            {
                caster->CastSpell(caster, 79437, true);
                caster->RemoveAura(74434);
            }
        }
        // Divine Touch
        else if (m_spellInfo->Id == 63544)
        {
        }
        // Glyph of Power Word: Shield
        else if (m_spellInfo->Id == 56160)
        {
        }
        // Efflorescence
        else if (m_spellInfo->Id == 81269)
        {
        }
        // Glyph of Healing Wave
        else if (m_spellInfo->Id == 55533)
        {
        }
        // Spirit Link
        else if (m_spellInfo->Id == 98021)
        {
        }
        else
            addhealth = caster->SpellHealingBonus(unitTarget, m_spellInfo, effIndex, addhealth, HEAL);

        if (caster->GetMapId() == 720) // // Vital flame healing bonus implementation
        {
            if (caster->HasAura(99263) && unitTarget->HasAura(99252) ) //Vital flame, Blaze of Glory
            {
                if (AuraEffect* aurEff = caster->GetAuraEffect(99263,EFFECT_0))
                {
                    addhealth += (addhealth * aurEff->GetAmount()) / 100;
                }
            }
        }

        /**** Mastery System for healing spells**************/

        if (Player *player = m_caster->ToPlayer())
        {
            if (player->HasMastery() && addhealth > 1)
            {
                BranchSpec playerSpec = player->GetActiveTalentBranchSpec();

                // Implementation of Echo of Light mastery proficiency
                if (m_spellInfo->SpellFamilyName == SPELLFAMILY_PRIEST &&
                    playerSpec == SPEC_PRIEST_HOLY)
                {
                    // Echo of Light HoT effect
                    int32 bp0 = addhealth*(player->GetMasteryPoints()*1.25f/100.0f);

                    // stack with old aura
                    if (Aura* echo = unitTarget->GetAura(77489))
                        if (AuraEffect* hoteff = echo->GetEffect(EFFECT_0))
                            bp0 += hoteff->GetAmount()*((float)(hoteff->GetTotalTicks()-hoteff->GetTickNumber())/(float)hoteff->GetTotalTicks());

                    m_caster->CastCustomSpell(unitTarget, 77489, &bp0, NULL, NULL, true);
                }

                // Implementation of Illuminated Healing mastery proficiency
                if (m_spellInfo->SpellFamilyName == SPELLFAMILY_PALADIN &&
                    playerSpec == SPEC_PALADIN_HOLY)
                {
                    // Illuminated Healing absorb value and spellcast
                    int32 bp0 = addhealth*(player->GetMasteryPoints()*1.5f/100.0f);

                    // "The total absorption created can never exceed 1/3 of the casting paladin�s health."
                    if (bp0 > m_caster->GetHealth()*0.33f)
                        bp0 = m_caster->GetHealth()*0.33f;

                    if (Aura* pAura = unitTarget->GetAura(86273, m_caster->GetGUID()))
                    {
                        if (AuraEffect* pEff = pAura->GetEffect(EFFECT_0))
                        {
                            bp0 += pEff->GetAmount();
                            if (bp0 > m_caster->GetHealth()*0.33f)
                                bp0 = m_caster->GetHealth()*0.33f;

                            pEff->SetAmount(bp0);
                            pAura->SetNeedClientUpdateForTargets();
                            pAura->SetDuration(pAura->GetMaxDuration());
                        }
                    }
                    else
                        m_caster->CastCustomSpell(unitTarget, 86273, &bp0, 0, 0, true);
                }

                // Implementation of Harmony mastery proficiency
                if (m_spellInfo->SpellFamilyName == SPELLFAMILY_DRUID &&
                    playerSpec == SPEC_DRUID_RESTORATION && !m_IsTriggeredSpell)
                {
                    int32 bp = player->GetMasteryPoints() * 1.25f;
                    player->CastCustomSpell(player, 100977, &bp, &bp, NULL, true);
                }
            }
        }
        /****************************************************/

        // Remove Grievious bite if fully healed
        if (unitTarget->HasAura(48920) && (unitTarget->GetHealth() + addhealth >= unitTarget->GetMaxHealth()))
            unitTarget->RemoveAura(48920);

        // Nature's Blessing (after all calculating)
        if (m_spellInfo->SpellFamilyName == SPELLFAMILY_SHAMAN)
        {
            if (unitTarget->HasAura(974)) // must have Earth Shield
            {
                if (caster->HasAura(30869))
                    addhealth *= 1.18f;
                else if (caster->HasAura(30868))
                    addhealth *= 1.12f;
                else if (caster->HasAura(30867))
                    addhealth *= 1.06f;
            }

            // any shaman heal should trigger Ancestral Vigor if talent Ancestral Healing is present
            if (m_caster->HasAura(16235) || m_caster->HasAura(16176))
            {
                int32 bp0 = 5;
                if (m_caster->HasAura(16235))
                    bp0 = 10;

                uint32 maxHealth = unitTarget->GetMaxHealth();

                // the amount is 10% of healing done and cannot exceed 10% of targets total HP
                bp0 = (int32)(bp0*addhealth/100.0f);
                if (bp0 > maxHealth*0.1f)
                    bp0 = maxHealth*0.1f;

                Aura* pAura = unitTarget->GetAura(105284);
                AuraEffect* pEff = pAura ? pAura->GetEffect(EFFECT_0) : NULL;

                if (pAura && pEff)
                {
                    bp0 += pEff->GetAmount();

                    if (bp0 > pEff->GetScriptedAmount()*0.1f)
                        bp0 = pEff->GetScriptedAmount()*0.1f;

                    pEff->ChangeAmount(bp0);
                    pAura->RefreshDuration();
                    pAura->SetNeedClientUpdateForTargets();
                }
                else
                {
                    m_caster->CastCustomSpell(unitTarget, 105284, &bp0, 0, 0, true);

                    if ((pAura = unitTarget->GetAura(105284)) != NULL && pAura->GetEffect(0))
                        pAura->GetEffect(0)->SetScriptedAmount(maxHealth);
                }
            }
        }
        // Greater Heal
        else if (m_spellInfo->Id == 2060)
        {
            // Train of Thought
            if (m_caster->ToPlayer() && (m_caster->HasAura(92297) || (m_caster->HasAura(92295) && roll_chance_i(50))))
                m_caster->ToPlayer()->ModifySpellCooldown(89485, -5000, true);
        }
        // Flash Heal (the instant one from Surge of Light)
        else if (m_spellInfo->Id == 101062)
        {
            // Remove talent proc Surge of Light (instant cast)
            m_caster->RemoveAurasDueToSpell(88688);
        }
        else if (m_spellInfo->Id == 33110) // Glyph of Prayer of Mending
        {
            if (Aura * mending = unitTarget->GetAura(41635,caster->GetGUID()))
            {
                if (mending->GetCharges() == 5 && caster->HasAura(55685))
                    addhealth = addhealth * 1.6f;
            }
        }
        // Healing Stream Totem
        else if (m_spellInfo->Id == 52042)
        {
            if (Unit *owner = m_caster->GetCharmerOrOwnerPlayerOrPlayerItself())
            {
                // Soothing Rains
                if (owner->HasAura(16187))
                    addhealth *= 1.25f;
                else if (owner->HasAura(16205))
                    addhealth *= 1.50f;
            }
        }

        m_damage -= addhealth;
    }
}

void Spell::EffectHealPct(SpellEffIndex effIndex)
{
    if (!unitTarget || !unitTarget->isAlive() || damage < 0)
        return;

    // Skip if m_originalCaster not available
    if (!m_originalCaster)
        return;

    // Rune Tap - Party
    if (m_spellInfo->Id == 59754 && unitTarget == m_caster)
        return;

    // Victory Rush and healing reduction when proc from Impending Victory
    if (m_spellInfo->Id == 34428 && m_caster->HasAura(82368))
        damage = 5;

    // Drain Life (Health Energize) heals more when under or at 25% health and have talent Death's Embrace
    if (m_spellInfo->Id == 89653 && m_caster && m_caster->GetHealthPct() <= 25.0f)
    {
        // Death's Embrace
        if (m_caster->HasAura(47200))
            damage += 3;
        else if (m_caster->HasAura(47199))
            damage += 2;
        else if (m_caster->HasAura(47198))
            damage += 1;
    }

    m_healing += m_originalCaster->SpellHealingBonus(unitTarget, m_spellInfo, effIndex, unitTarget->CountPctFromMaxHealth(damage), HEAL);
}

void Spell::EffectHealMechanical(SpellEffIndex effIndex)
{
    if (!unitTarget || !unitTarget->isAlive() || damage < 0)
        return;

    // Skip if m_originalCaster not available
    if (!m_originalCaster)
        return;

    m_healing += m_originalCaster->SpellHealingBonus(unitTarget, m_spellInfo, effIndex, uint32(damage), HEAL);
}

void Spell::EffectHealthLeech(SpellEffIndex effIndex)
{
    if (!unitTarget || !unitTarget->isAlive() || damage < 0)
        return;

    sLog->outDebug("HealthLeech :%i", damage);

    float healMultiplier = SpellMgr::CalculateSpellEffectValueMultiplier(m_spellInfo, effIndex, m_originalCaster, this);

    // Leeching spells coming from player also counts spell power bonus
    if (m_caster->GetTypeId() == TYPEID_PLAYER)
        damage = m_caster->SpellDamageBonus(unitTarget, m_spellInfo, effIndex, damage, SPELL_DIRECT_DAMAGE);

    m_damage += damage;

    // get max possible damage, don't count overkill for heal
    uint32 healthGain = uint32(-unitTarget->GetHealthGain(-damage) * healMultiplier);

    if (m_caster->isAlive())
    {
        healthGain = m_caster->SpellHealingBonus(m_caster, m_spellInfo, effIndex, healthGain, HEAL);
        m_caster->HealBySpell(m_caster, m_spellInfo, uint32(healthGain));
    }
}

void Spell::DoCreateItem(uint32 /*i*/, uint32 itemtype)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* player = (Player*)unitTarget;

    uint32 newitemid = itemtype;
    ItemPrototype const *pProto = sObjectMgr->GetItemPrototype(newitemid);
    if (!pProto)
    {
        player->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, NULL, NULL);
        return;
    }

    // bg reward have some special in code work
    uint32 bgType = 0;
    switch(m_spellInfo->Id)
    {
        case SPELL_AV_MARK_WINNER:
        case SPELL_AV_MARK_LOSER:
            bgType = BATTLEGROUND_AV;
            break;
        case SPELL_WS_MARK_WINNER:
        case SPELL_WS_MARK_LOSER:
            bgType = BATTLEGROUND_WS;
            break;
        case SPELL_AB_MARK_WINNER:
        case SPELL_AB_MARK_LOSER:
            bgType = BATTLEGROUND_AB;
            break;
        default:
            break;
    }

    uint32 num_to_add = damage;

    if (num_to_add < 1)
        num_to_add = 1;
    if (num_to_add > pProto->GetMaxStackSize())
        num_to_add = pProto->GetMaxStackSize();

    // init items_count to 1, since 1 item will be created regardless of specialization
    int items_count=1;
    // the chance to create additional items
    float additionalCreateChance=0.0f;
    // the maximum number of created additional items
    uint8 additionalMaxNum=0;
    // get the chance and maximum number for creating extra items
    if (canCreateExtraItems(player, m_spellInfo->Id, additionalCreateChance, additionalMaxNum))
    {
        // roll with this chance till we roll not to create or we create the max num
        while (roll_chance_f(additionalCreateChance) && items_count <= additionalMaxNum)
            ++items_count;
    }

    // really will be created more items
    num_to_add *= items_count;

    // can the player store the new item?
    ItemPosCountVec dest;
    uint32 no_space = 0;
    uint8 msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, newitemid, num_to_add, &no_space);
    if (msg != EQUIP_ERR_OK)
    {
        // convert to possible store amount
        if (msg == EQUIP_ERR_INVENTORY_FULL || msg == EQUIP_ERR_CANT_CARRY_MORE_OF_THIS)
            num_to_add -= no_space;
        else
        {
            // if not created by another reason from full inventory or unique items amount limitation
            player->SendEquipError(msg, NULL, NULL, newitemid);
            return;
        }
    }

    if (num_to_add)
    {
        // create the new item and store it
        Item* pItem = player->StoreNewItem(dest, newitemid, true, Item::GenerateItemRandomPropertyId(newitemid));

        // was it successful? return error if not
        if (!pItem)
        {
            player->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, NULL, NULL);
            return;
        }

        // set the "Crafted by ..." property of the item
        if (pItem->GetProto()->Class != ITEM_CLASS_CONSUMABLE && pItem->GetProto()->Class != ITEM_CLASS_QUEST && newitemid != 6265 && newitemid != 6948)
            pItem->SetUInt32Value(ITEM_FIELD_CREATOR, player->GetGUIDLow());

        // send info to the client
        if (pItem)
            player->SendNewItem(pItem, num_to_add, true, bgType == 0);

        // we succeeded in creating at least one item, so a levelup is possible
        if (bgType == 0)
            player->UpdateCraftSkill(m_spellInfo->Id);

        // Add guild news for crafting epic things
        if (pItem->GetProto()->Quality == ITEM_QUALITY_EPIC)
            player->AddGuildNews(GUILD_NEWS_ITEM_CRAFT, pItem->GetEntry());

        player->UpdateGuildAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_CRAFT_ITEMS_GUILD, pItem->GetProto()->ItemId);

        // Check if it was archaeology project, and update proper criterias
        if (m_spellInfo->researchProjectId > 0)
        {
            ResearchProjectEntry const* pProj = sResearchProjectStore.LookupEntry(m_spellInfo->researchProjectId);
            if (pProj)
            {
                // We send miscvalue1 value 1 for uncommon and better items, 0 for poor and normal
                if (pItem->GetProto()->Quality > ITEM_QUALITY_NORMAL)
                    player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ARCHAEOLOGY, 1, pProj->researchBranch);
                else
                    player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ARCHAEOLOGY, 0, pProj->researchBranch);
            }
        }

        /* activate craft spell cooldown */
        std::vector<uint32> cd_spells;
        uint32 cd_time = 0;

        switch (m_spellInfo->Id) {

            /* 6 days + to midnight */
            case 75146:  // tailor: Dream of Azshara
            case 75142:  // tailor: Dream of Deepholm
            case 75144:  // tailor: Dream of Hyjal
            case 75145:  // tailor: Dream of Ragnaros
            case 75141:  // tailor: Dream of Skywall
                cd_spells.push_back(m_spellInfo->Id);
                cd_time = 518400 + sObjectMgr->SecsToMidnight();
                break;

            /* 2 days + to midnight */
            case 60893:  // alch: Northrend Alchemy Research
                cd_spells.push_back(m_spellInfo->Id);
                cd_time = 172800 + sObjectMgr->SecsToMidnight();
                break;

            /* at midnight */
            case 61288:  // inscr: Minor Inscription Research
            case 61177:  // inscr: Nothernd Inscription Research
            case 89244:  // inscr: Forged Documents
            case 73478:  // jewel: Fire Prism
            case 62242:  // jewel: Icy Prism
            case 47280:  // jewel: Brilliant Glass
                cd_spells.push_back(m_spellInfo->Id);
                cd_time = sObjectMgr->SecsToMidnight();
                break;

            /* special: alchemy transmute, shared cooldown */
            // vanilla - iron to gold, mithril to truesilver
            case 11479:  case 11480:
            // vanilla - elemental transmutes
            case 17564: case 17562: case 17563: case 17565:
            case 17560: case 17561: case 17566: case 17559:
            // tbc - primal transmutes
            case 28569: case 28568: case 28567: case 28566:
            case 28581: case 28580: case 28582: case 28584:
            case 28583: case 28585:
            // wotlk - eternal transmutes
            case 53784: case 53783: case 53780: case 53779:
            case 53771: case 53773: case 53774: case 53775:
            case 53782: case 53781: case 53776: case 53777:
            // wotlk - epic gems
            case 66659: case 66663: case 66660: case 66664:
            case 66662: case 66658:
            // cata - truegold, pyrium, living elements
            case 80243: case 80244: case 78866:
            {
                cd_spells.push_back(11479); cd_spells.push_back(11480);
                cd_spells.push_back(17564); cd_spells.push_back(17562);
                cd_spells.push_back(17563); cd_spells.push_back(17565);
                cd_spells.push_back(17560); cd_spells.push_back(17561);
                cd_spells.push_back(17566); cd_spells.push_back(17559);
                cd_spells.push_back(28569); cd_spells.push_back(28568);
                cd_spells.push_back(28567); cd_spells.push_back(28566);
                cd_spells.push_back(28581); cd_spells.push_back(28580);
                cd_spells.push_back(28582); cd_spells.push_back(28584);
                cd_spells.push_back(28583); cd_spells.push_back(28585);
                cd_spells.push_back(53784); cd_spells.push_back(53783);
                cd_spells.push_back(53780); cd_spells.push_back(53779);
                cd_spells.push_back(53771); cd_spells.push_back(53773);
                cd_spells.push_back(53774); cd_spells.push_back(53775);
                cd_spells.push_back(53782); cd_spells.push_back(53781);
                cd_spells.push_back(53776); cd_spells.push_back(53777);
                cd_spells.push_back(66659); cd_spells.push_back(66663);
                cd_spells.push_back(66660); cd_spells.push_back(66664);
                cd_spells.push_back(66662); cd_spells.push_back(66658);
                cd_spells.push_back(80243); cd_spells.push_back(80244);
                cd_spells.push_back(78866);
                cd_time = sObjectMgr->SecsToMidnight();
                break;
            }
            default:
                break;
        }

        while (!cd_spells.empty()) {
            player->AddSpellCooldown(cd_spells.back(), 0, cd_time * 1000);
            cd_spells.pop_back();
        }

        /* send only one packet for current spell */
        if (cd_time) {
            WorldPacket data(SMSG_SPELL_COOLDOWN, 8+1+4);
            data << uint64(player->GetGUID());
            data << uint8(1);
            data << uint32(m_spellInfo->Id);
            data << uint32(cd_time);    // 0 means full spell cooldown
            player->GetSession()->SendPacket(&data);
        }
    }

/*
    // for battleground marks send by mail if not add all expected
    if (no_space > 0 && bgType)
    {
        if (Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(BattlegroundTypeId(bgType)))
            bg->SendRewardMarkByMail(player, newitemid, no_space);
    }
*/
}

void Spell::EffectCreateItem(SpellEffIndex effIndex)
{
    if (m_spellInfo->EffectItemType[effIndex] == 6948 && unitTarget->ToPlayer()->GetItemByEntry(6948)) //dont create a new hearthstone on homebind if player already has
        return;
    DoCreateItem(effIndex,m_spellInfo->EffectItemType[effIndex]);
    ExecuteLogEffectCreateItem(effIndex, m_spellInfo->EffectItemType[effIndex]);
}

void Spell::EffectCreateItem2(SpellEffIndex effIndex)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;
    Player* player = (Player*)m_caster;

    uint32 item_id = m_spellInfo->EffectItemType[effIndex];

    if (item_id)
        DoCreateItem(effIndex, item_id);

    // special case: fake item replaced by generate using spell_loot_template
    if (IsLootCraftingSpell(m_spellInfo))
    {
        if (item_id)
        {
            if (!player->HasItemCount(item_id, damage))
                return;

            // remove reagent
            uint32 count = damage;
            player->DestroyItemCount(item_id, count, true);

            uint16 lootMode = LOOT_MODE_DEFAULT;

            // Special scripts
            switch(m_spellInfo->Id)
            {
            // Transmute: Living Elements
            case 78866:
                {
                    // Zone dependent loot
                    switch(m_caster->GetZoneId())
                    {
                    case 616:
                        lootMode = LOOT_MODE_HARD_MODE_1; // Hyjal - Fire only
                        break;
                    case 5146:
                        lootMode = LOOT_MODE_HARD_MODE_2; // Vashj'ir - Water only
                        break;
                    case 5042:
                        lootMode = LOOT_MODE_HARD_MODE_3; // Deepholme - Earth only
                        break;
                    case 5034:
                        lootMode = LOOT_MODE_HARD_MODE_4; // Uldum - Air only
                        break;
                    default: // all four 25% drop
                        break;
                    }
                    break;
                }
            default:
                break;
            }

            int i = 0;
            for(i = 0; i < damage; i++) // for each "placeholder" item created
            {
                // create some random items
                player->AutoStoreLoot(m_spellInfo->Id, LootTemplates_Spell, false, lootMode);
            }
        }
        else
            player->AutoStoreLoot(m_spellInfo->Id, LootTemplates_Spell);    // create some random items
    }
    // TODO: ExecuteLogEffectCreateItem(i, m_spellInfo->EffectItemType[i]);
}

void Spell::EffectCreateRandomItem(SpellEffIndex /*effIndex*/)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;
    Player* player = (Player*)m_caster;

    // create some random items
    LootTemplate const* tab = LootTemplates_Spell.GetLootFor(m_spellInfo->Id);
    if(tab)
        player->AutoStoreLoot(m_spellInfo->Id, LootTemplates_Spell);
    else if (m_CastItem)
        player->AutoStoreLoot(m_CastItem->GetEntry(), LootTemplates_Item);
    // TODO: ExecuteLogEffectCreateItem(i, m_spellInfo->EffectItemType[i]);
}

void Spell::EffectPersistentAA(SpellEffIndex effIndex)
{
    if (!m_spellAura)
    {
        float radius = GetEffectRadius(effIndex);

        Unit *caster = m_caster->GetEntry() == WORLD_TRIGGER ? m_originalCaster : m_caster;
        // Caster not in world, might be spell triggered from aura removal
        if (!caster->IsInWorld())
            return;
        DynamicObject* dynObj = new DynamicObject;
        if (!dynObj->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_DYNAMICOBJECT), caster, m_spellInfo, m_targets.m_dstPos, radius, false, DYNAMIC_OBJECT_AREA_SPELL))
        {
            delete dynObj;
            return;
        }
        dynObj->GetMap()->Add(dynObj);

        if (Aura * aura = Aura::TryCreate(m_spellInfo, dynObj, caster, &m_spellValue->EffectBasePoints[0]))
        {
            m_spellAura = aura;
            m_spellAura->_RegisterForTargets();
        }
        else
            return;
    }
    ASSERT(m_spellAura->GetDynobjOwner());
    m_spellAura->_ApplyEffectForTargets(effIndex);
}

void Spell::EffectEnergize(SpellEffIndex effIndex)
{
    if (!unitTarget)
        return;
    if (!unitTarget->isAlive())
        return;

    if (m_spellInfo->EffectMiscValue[effIndex] < 0 || m_spellInfo->EffectMiscValue[effIndex] >= int8(MAX_POWERS))
        return;

    Powers power = Powers(m_spellInfo->EffectMiscValue[effIndex]);

    if (unitTarget->getPowerType() != power && !(m_spellInfo->AttributesEx7 & SPELL_ATTR7_CAN_RESTORE_SECONDARY_POWER))
        return;

    // Some level depends spells
    int level_multiplier = 0;
    int level_diff = 0;
    switch (m_spellInfo->Id)
    {
        case 9512:                                          // Restore Energy
            level_diff = m_caster->getLevel() - 40;
            level_multiplier = 2;
            break;
        case 24571:                                         // Blood Fury
            level_diff = m_caster->getLevel() - 60;
            level_multiplier = 10;
            break;
        case 24532:                                         // Burst of Energy
            level_diff = m_caster->getLevel() - 60;
            level_multiplier = 4;
            break;
        case 63375:                                         // Improved Stormstrike
        case 68082:                                         // Glyph of Seal of Command
        case 20167:                                         // Seal of Insight
        case 99131:                                         // Divine Fire - Priest T12 Healer 2P Bonus
        case 99069:                                         // Fires of Heaven - Paladin T12 Healer 2P Bonus
        case 99007:                                         // Heartfire - Druid T12 Healer 2P bonus
        case 99189:                                         // Flametide - Shaman T12 Restoration 2P Bonus
            damage = damage * unitTarget->GetCreateMana() / 100;
            break;
        case 92601:                                         // Tyrande's favorite doll - Release mana: restore mana
            if (AuraEffect* storedmana = m_caster->GetAuraEffect(92596, EFFECT_0))
            {
                damage = storedmana->GetAmount();
                m_caster->RemoveAurasDueToSpell(92596);
            }
            else return;
            break;
        case 48542:                                         // Revitalize
            damage = damage * unitTarget->GetMaxPower(power) / 100;
            break;
        case 50422: // Scent of Blood
            if (Aura* pAura = unitTarget->GetAura(50421))
                if (pAura->ModStackAmount(-1))
                    pAura->Remove(AURA_REMOVE_BY_EXPIRE);
            break;
        case 5405:  // Replenish Mana (mage's Mana Gem)
        {
            // Improved Mana Gem
            int32 bp0 = 0;
            if (m_caster->HasAura(31584))
                bp0 = 0.01f*m_caster->GetMaxPower(POWER_MANA);
            else if (m_caster->HasAura(31585))
                bp0 = 0.02f*m_caster->GetMaxPower(POWER_MANA);

            if (bp0)
                m_caster->CastCustomSpell(m_caster, 83098, &bp0, &bp0, 0, true);
            break;
        }
        case 82726: // Fervor
        {
            // If caster is player and have pet, let him cast pet variant of spell Fervor (targetting system will do the trick for us)
            if (m_caster && m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->GetPetGUID())
                m_caster->CastSpell(m_caster, 82728, true);
            break;
        }
        case 88676: // Eternal Glory
        {
            m_caster->ModifyPower(power, damage);   // energize spell log shows wrong numbers here - don't send it until it is fixed
            return;
        }
        case 101033: // Resurgence
        {
            // we need to pass the values in scripted amount due to spell scaling overwriting our amount
            damage = m_spellValue->EffectScriptedPoints[0];
            break;
        }
        default:
            break;
    }

    if (level_diff > 0)
        damage -= level_multiplier * level_diff;

    // Some exceptions for POWER_SCRIPTED, because its universal and not-really power, only some kind of marker

    if (damage < 0 && power != POWER_SCRIPTED)
        return;

    if (power != POWER_SCRIPTED && unitTarget->GetMaxPower(power) == 0)
        return;

    m_caster->EnergizeBySpell(unitTarget, m_spellInfo->Id, damage, power);

    // Mad Alchemist's Potion
    if (m_spellInfo->Id == 45051)
    {
        // find elixirs on target
        bool guardianFound = false;
        bool battleFound = false;
        Unit::AuraApplicationMap& Auras = unitTarget->GetAppliedAuras();
        for (Unit::AuraApplicationMap::iterator itr = Auras.begin(); itr != Auras.end(); ++itr)
        {
            uint32 spell_id = itr->second->GetBase()->GetId();
            if (!guardianFound)
                if (sSpellMgr->IsSpellMemberOfSpellGroup(spell_id, SPELL_GROUP_ELIXIR_GUARDIAN))
                    guardianFound = true;
            if (!battleFound)
                if (sSpellMgr->IsSpellMemberOfSpellGroup(spell_id, SPELL_GROUP_ELIXIR_BATTLE))
                    battleFound = true;
            if (battleFound && guardianFound)
                break;
        }

        // get all available elixirs by mask and spell level
        std::set<uint32> avalibleElixirs;
        if (!guardianFound)
            sSpellMgr->GetSetOfSpellsInSpellGroup(SPELL_GROUP_ELIXIR_GUARDIAN, avalibleElixirs);
        if (!battleFound)
            sSpellMgr->GetSetOfSpellsInSpellGroup(SPELL_GROUP_ELIXIR_BATTLE, avalibleElixirs);
        for (std::set<uint32>::iterator itr = avalibleElixirs.begin(); itr != avalibleElixirs.end() ;)
        {
            SpellEntry const *spellInfo = sSpellStore.LookupEntry(*itr);
            if (spellInfo->spellLevel < m_spellInfo->spellLevel || spellInfo->spellLevel > unitTarget->getLevel())
                avalibleElixirs.erase(itr++);
            else if (sSpellMgr->IsSpellMemberOfSpellGroup(*itr, SPELL_GROUP_ELIXIR_SHATTRATH))
                avalibleElixirs.erase(itr++);
            else if (sSpellMgr->IsSpellMemberOfSpellGroup(*itr, SPELL_GROUP_ELIXIR_UNSTABLE))
                avalibleElixirs.erase(itr++);
            else
                ++itr;
        }

        if (!avalibleElixirs.empty())
        {
            // cast random elixir on target
            uint32 rand_spell = urand(0,avalibleElixirs.size()-1);
            std::set<uint32>::iterator itr = avalibleElixirs.begin();
            std::advance(itr, rand_spell);
            m_caster->CastSpell(unitTarget,*itr,true,m_CastItem);
        }
    }
}

void Spell::EffectEnergizePct(SpellEffIndex effIndex)
{
    if (!unitTarget)
        return;
    if (!unitTarget->isAlive())
        return;

    if (m_spellInfo->EffectMiscValue[effIndex] < 0 || m_spellInfo->EffectMiscValue[effIndex] >= int8(MAX_POWERS))
        return;

    Powers power = Powers(m_spellInfo->EffectMiscValue[effIndex]);

    if (unitTarget->getPowerType() != power && !(m_spellInfo->AttributesEx7 & SPELL_ATTR7_CAN_RESTORE_SECONDARY_POWER))
        return;

    uint32 maxPower = unitTarget->GetMaxPower(power);
    if (maxPower == 0)
        return;

    if (m_spellInfo->Id == 30294) // mana regen (Soul Leech)
    {
        if (m_caster->HasAura(30293)) // Soul Leech rank 1
            damage = 2;
        else if (m_caster->HasAura(30295)) // Soul Leech rank 2
            damage = 4;
    }

    int32 gain = damage * maxPower / 100;
    m_caster->EnergizeBySpell(unitTarget, m_spellInfo->Id, gain, power);
}

void Spell::SendLoot(uint64 guid, LootType loottype)
{
    Player* player = (Player*)m_caster;
    if (!player)
        return;

    if (gameObjTarget)
    {
        // Players shouldn't be able to loot gameobjects that are currently despawned
        if (!gameObjTarget->isSpawned() && !player->isGameMaster())
        {
            sLog->outError("Possible hacking attempt: Player %s [guid: %u] tried to loot a gameobject [entry: %u id: %u] which is on respawn time without being in GM mode!",
                            player->GetName(), player->GetGUIDLow(), gameObjTarget->GetEntry(), gameObjTarget->GetGUIDLow());
            return;
        }

        if (sScriptMgr->OnGossipHello(player, gameObjTarget))
            return;

        gameObjTarget->AI()->GossipHello(player);

        switch (gameObjTarget->GetGoType())
        {
            case GAMEOBJECT_TYPE_DOOR:
            case GAMEOBJECT_TYPE_BUTTON:
                gameObjTarget->UseDoorOrButton();
                player->GetMap()->ScriptsStart(sGameObjectScripts, gameObjTarget->GetDBTableGUIDLow(), player, gameObjTarget);
                return;

            case GAMEOBJECT_TYPE_QUESTGIVER:
                // start or end quest
                player->PrepareQuestMenu(guid);
                player->SendPreparedQuest(guid);
                return;

            case GAMEOBJECT_TYPE_SPELL_FOCUS:
                // triggering linked GO
                if (uint32 trapEntry = gameObjTarget->GetGOInfo()->spellFocus.linkedTrapId)
                    gameObjTarget->TriggeringLinkedGameObject(trapEntry,m_caster);
                return;

            case GAMEOBJECT_TYPE_GOOBER:
                gameObjTarget->Use(m_caster);
                return;

            case GAMEOBJECT_TYPE_CHEST:
                // TODO: possible must be moved to loot release (in different from linked triggering)
                if (gameObjTarget->GetGOInfo()->chest.eventId)
                {
                    sLog->outDebug("Chest ScriptStart id %u for GO %u", gameObjTarget->GetGOInfo()->chest.eventId,gameObjTarget->GetDBTableGUIDLow());
                    player->GetMap()->ScriptsStart(sEventScripts, gameObjTarget->GetGOInfo()->chest.eventId, player, gameObjTarget);
                }

                // triggering linked GO
                if (uint32 trapEntry = gameObjTarget->GetGOInfo()->chest.linkedTrapId)
                    gameObjTarget->TriggeringLinkedGameObject(trapEntry,m_caster);

                // Don't return, let loots been taken
            default:
                break;
        }
    }

    // Send loot
    player->SendLoot(guid, loottype);
}

void Spell::EffectOpenLock(SpellEffIndex effIndex)
{
    if (!m_caster || m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        sLog->outDebug("WORLD: Open Lock - No Player Caster!");
        return;
    }

    Player* player = (Player*)m_caster;

    uint32 lockId = 0;
    uint64 guid = 0;

    // Get lockId
    if (gameObjTarget)
    {
        GameObjectInfo const* goInfo = gameObjTarget->GetGOInfo();
        // Arathi Basin banner opening !
        if ((goInfo->type == GAMEOBJECT_TYPE_BUTTON && goInfo->button.noDamageImmune) ||
            (goInfo->type == GAMEOBJECT_TYPE_GOOBER && goInfo->goober.losOK))
        {
            //CanUseBattlegroundObject() already called in CheckCast()
            // in battleground check
            if (Battleground *bg = player->GetBattleground())
          {
        bg->EventPlayerClickedOnFlag(player, gameObjTarget);
        return;
          }
        }
        else if (goInfo->type == GAMEOBJECT_TYPE_FLAGSTAND)
        {
            //CanUseBattlegroundObject() already called in CheckCast()
            // in battleground check
            if (Battleground *bg = player->GetBattleground())
            {
                if (bg->GetTypeID(true) == BATTLEGROUND_EY)
                    bg->EventPlayerClickedOnFlag(player, gameObjTarget);
                return;
            }
        }else if (m_spellInfo->Id == 1842 && gameObjTarget->GetGOInfo()->type == GAMEOBJECT_TYPE_TRAP && gameObjTarget->GetOwner())
        {
            gameObjTarget->SetLootState(GO_JUST_DEACTIVATED);
            return;
        }
        // TODO: Add script for spell 41920 - Filling, becouse server it freze when use this spell
        // handle outdoor pvp object opening, return true if go was registered for handling
        // these objects must have been spawned by outdoorpvp!
        else if (gameObjTarget->GetGOInfo()->type == GAMEOBJECT_TYPE_GOOBER && sOutdoorPvPMgr->HandleOpenGo(player, gameObjTarget->GetGUID()))
            return;
        lockId = goInfo->GetLockId();
        guid = gameObjTarget->GetGUID();
    }
    else if (itemTarget)
    {
        lockId = itemTarget->GetProto()->LockID;
        guid = itemTarget->GetGUID();
    }
    else
    {
        sLog->outDebug("WORLD: Open Lock - No GameObject/Item Target!");
        return;
    }

    SkillType skillId = SKILL_NONE;
    int32 reqSkillValue = 0;
    int32 skillValue;

    SpellCastResult res = CanOpenLock(effIndex, lockId, skillId, reqSkillValue, skillValue);
    if (res != SPELL_CAST_OK)
    {
        SendCastResult(res);
        return;
    }

    if (gameObjTarget)
        SendLoot(guid, LOOT_SKINNING);
    else
        itemTarget->SetFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_UNLOCKED);

    // not allow use skill grow at item base open
    if (!m_CastItem && skillId != SKILL_NONE)
    {
        // update skill if really known
        if (uint32 pureSkillValue = player->GetPureSkillValue(skillId))
        {
            if (gameObjTarget)
            {
                // Allow one skill-up and one XP gain until respawned
                if (!gameObjTarget->IsInSkillupList(player->GetGUIDLow()) &&
                    player->UpdateGatherSkill(skillId, pureSkillValue, reqSkillValue))
                {
                    gameObjTarget->AddToSkillupList(player->GetGUIDLow());
                    if (skillId == SKILL_HERBALISM || skillId == SKILL_MINING || skillId == SKILL_ARCHAEOLOGY)
                    {
                        // give XP to player
                        Player *pl = m_caster->ToPlayer();
                        if (pl)
                        {
                            uint8 lvl = pl->getLevel();
                            uint32 skill = reqSkillValue;
                            uint32 id = gameObjTarget->GetGOInfo()->id;
                            uint32 xp = Trinity::GatherXP::Gain(lvl, skill, id);
                            xp += pl->GetXPRestBonus(xp);
                            pl->GiveXP(xp, NULL);
                        }
                    }
                }
            }
            else if (itemTarget)
            {
                // Do one skill-up
                player->UpdateGatherSkill(skillId, pureSkillValue, reqSkillValue);
            }
        }
    }
    ExecuteLogEffectOpenLock(effIndex, gameObjTarget ? (Object*)gameObjTarget : (Object*)itemTarget);
}

void Spell::EffectSummonChangeItem(SpellEffIndex effIndex)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player *player = (Player*)m_caster;

    // applied only to using item
    if (!m_CastItem)
        return;

    // ... only to item in own inventory/bank/equip_slot
    if (m_CastItem->GetOwnerGUID() != player->GetGUID())
        return;

    uint32 newitemid = m_spellInfo->EffectItemType[effIndex];
    if (!newitemid)
        return;

    uint16 pos = m_CastItem->GetPos();

    Item *pNewItem = Item::CreateItem(newitemid, 1, player);
    if (!pNewItem)
        return;

    for (uint8 j = PERM_ENCHANTMENT_SLOT; j <= TEMP_ENCHANTMENT_SLOT; ++j)
        if (m_CastItem->GetEnchantmentId(EnchantmentSlot(j)))
            pNewItem->SetEnchantment(EnchantmentSlot(j), m_CastItem->GetEnchantmentId(EnchantmentSlot(j)), m_CastItem->GetEnchantmentDuration(EnchantmentSlot(j)), m_CastItem->GetEnchantmentCharges(EnchantmentSlot(j)));

    if (m_CastItem->GetUInt32Value(ITEM_FIELD_DURABILITY) < m_CastItem->GetUInt32Value(ITEM_FIELD_MAXDURABILITY))
    {
        double loosePercent = 1 - m_CastItem->GetUInt32Value(ITEM_FIELD_DURABILITY) / double(m_CastItem->GetUInt32Value(ITEM_FIELD_MAXDURABILITY));
        player->DurabilityLoss(pNewItem, loosePercent);
    }

    if (player->IsInventoryPos(pos))
    {
        ItemPosCountVec dest;
        uint8 msg = player->CanStoreItem(m_CastItem->GetBagSlot(), m_CastItem->GetSlot(), dest, pNewItem, true);
        if (msg == EQUIP_ERR_OK)
        {
            player->DestroyItem(m_CastItem->GetBagSlot(), m_CastItem->GetSlot(), true);

            // prevent crash at access and unexpected charges counting with item update queue corrupt
            if (m_CastItem == m_targets.getItemTarget())
                m_targets.setItemTarget(NULL);

            m_CastItem = NULL;

            player->StoreItem(dest, pNewItem, true);
            return;
        }
    }
    else if (player->IsBankPos(pos))
    {
        ItemPosCountVec dest;
        uint8 msg = player->CanBankItem(m_CastItem->GetBagSlot(), m_CastItem->GetSlot(), dest, pNewItem, true);
        if (msg == EQUIP_ERR_OK)
        {
            player->DestroyItem(m_CastItem->GetBagSlot(), m_CastItem->GetSlot(), true);

            // prevent crash at access and unexpected charges counting with item update queue corrupt
            if (m_CastItem == m_targets.getItemTarget())
                m_targets.setItemTarget(NULL);

            m_CastItem = NULL;

            player->BankItem(dest, pNewItem, true);
            return;
        }
    }
    else if (player->IsEquipmentPos(pos))
    {
        uint16 dest;

        player->DestroyItem(m_CastItem->GetBagSlot(), m_CastItem->GetSlot(), true);

        uint8 msg = player->CanEquipItem(m_CastItem->GetSlot(), dest, pNewItem, true);

        if (msg == EQUIP_ERR_OK || msg == EQUIP_ERR_CANT_DO_RIGHT_NOW)
        {
            if (msg == EQUIP_ERR_CANT_DO_RIGHT_NOW) dest = EQUIPMENT_SLOT_MAINHAND;

            // prevent crash at access and unexpected charges counting with item update queue corrupt
            if (m_CastItem == m_targets.getItemTarget())
                m_targets.setItemTarget(NULL);

            m_CastItem = NULL;

            player->EquipItem(dest, pNewItem, true);
            player->AutoUnequipOffhandIfNeed();
            return;
        }
    }

    // fail
    delete pNewItem;
}

void Spell::EffectProficiency(SpellEffIndex /*effIndex*/)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;
    Player *p_target = (Player*)unitTarget;

    uint32 subClassMask = m_spellInfo->EquippedItemSubClassMask;
    if (m_spellInfo->EquippedItemClass == ITEM_CLASS_WEAPON && !(p_target->GetWeaponProficiency() & subClassMask))
    {
        p_target->AddWeaponProficiency(subClassMask);
        p_target->SendProficiency(ITEM_CLASS_WEAPON, p_target->GetWeaponProficiency());
    }
    if (m_spellInfo->EquippedItemClass == ITEM_CLASS_ARMOR && !(p_target->GetArmorProficiency() & subClassMask))
    {
        p_target->AddArmorProficiency(subClassMask);
        p_target->SendProficiency(ITEM_CLASS_ARMOR, p_target->GetArmorProficiency());
    }
}

void Spell::EffectSummonType(SpellEffIndex effIndex)
{
    uint32 entry = m_spellInfo->EffectMiscValue[effIndex];
    if (!entry)
        return;

    SummonPropertiesEntry const *properties = sSummonPropertiesStore.LookupEntry(m_spellInfo->EffectMiscValueB[effIndex]);
    if (!properties)
    {
        sLog->outError("EffectSummonType: Unhandled summon type %u", m_spellInfo->EffectMiscValueB[effIndex]);
        return;
    }

    if (!m_originalCaster)
        return;

    int32 duration = GetSpellDuration(m_spellInfo);
    if (Player* modOwner = m_originalCaster->GetSpellModOwner())
        modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_DURATION, duration);

    Position pos;
    GetSummonPosition(effIndex, pos);

    // Shadowy Apparition - summon pos is original casters pos
    if (entry == 46954 && m_originalCaster)
    {
        if (m_originalCaster->ToPlayer())
        {
            Player::GUIDTimestampMap* tsMap = m_originalCaster->ToPlayer()->GetSummonMapFor(entry);
            if (tsMap && tsMap->size() >= GetMaxActiveSummons(entry))
            {
                Player::GUIDTimestampMap::iterator ittd = tsMap->end();
                for (Player::GUIDTimestampMap::iterator itr2 = tsMap->begin(); itr2 != tsMap->end(); ++itr2)
                {
                    if (ittd == tsMap->end() || (*itr2).second < (*ittd).second)
                        if (!Creature::GetCreature(*m_originalCaster, (*itr2).first))
                            ittd = itr2;
                }

                if (ittd != tsMap->end())
                    tsMap->erase(ittd);
                else
                    return;
            }
        }
        m_originalCaster->GetPosition(&pos);
    }

    if (m_caster && m_caster->GetTypeId() == TYPEID_PLAYER && entry == 27893) // Dancing Rune Weapon
    {
        m_caster->CastSpell(m_caster,81256,true); // + 20 % parry chance
    }

    /*//totem must be at same Z in case swimming caster and etc.
        if (fabs(z - m_caster->GetPositionZ()) > 5)
            z = m_caster->GetPositionZ();

    uint8 level = m_caster->getLevel();

    // level of creature summoned using engineering item based at engineering skill level
    if (m_caster->GetTypeId() == TYPEID_PLAYER && m_CastItem)
    {
        ItemPrototype const *proto = m_CastItem->GetProto();
        if (proto && proto->RequiredSkill == SKILL_ENGINERING)
        {
            uint16 skill202 = m_caster->ToPlayer()->GetSkillValue(SKILL_ENGINERING);
            if (skill202)
                level = skill202/5;
        }
    }*/

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        Player::GUIDTimestampMap* tsMap = m_caster->ToPlayer()->GetSummonMapFor(entry);
        if (tsMap && tsMap->size() >= GetMaxActiveSummons(entry))
            m_caster->ToPlayer()->DespawnOldestSummon(entry);
    }

    // Shadowfiend should be summoned as pet
    if (entry == 19668 && m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        Pet* pet = m_caster->ToPlayer()->SummonPet(entry,pos.m_positionX,pos.m_positionY,pos.m_positionZ,pos.m_orientation,SUMMON_PET,15000,PET_SLOT_OTHER_PET);
        if (!pet)
            return;

        pet->SetReactState(REACT_AGGRESSIVE);
        pet->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);
        pet->SetCreatorGUID(m_originalCaster->GetGUID());
        ExecuteLogEffectSummonObject(effIndex, pet);

        if (Unit * victim = m_caster->getVictim())
        {
            if (victim->IsInWorld() && m_caster->IsHostileTo(victim))
                pet->GetMotionMaster()->MoveChase(victim);
        }

        return;
    }

    TempSummon *summon = NULL;

    switch (properties->Category)
    {
        default:
            if (properties->Flags & 512)
            {
                SummonGuardian(effIndex, entry, properties);
                break;
            }
            switch (properties->Type)
            {
                case SUMMON_TYPE_PET:
                case SUMMON_TYPE_GUARDIAN:
                case SUMMON_TYPE_GUARDIAN2:
                case SUMMON_TYPE_MINION:
                    SummonGuardian(effIndex, entry, properties);
                    break;
                case SUMMON_TYPE_VEHICLE:
                case SUMMON_TYPE_VEHICLE2:
                    if (m_originalCaster)
                        summon = m_caster->GetMap()->SummonCreature(entry, pos, properties, duration, m_originalCaster);
                    break;
                case SUMMON_TYPE_TOTEM:
                {
                    // we need to know the totem's GUID before it is actually created
                    uint32 lowGUID = sObjectMgr->GenerateLowGuidForUnit(true);
                    if (m_originalCaster->GetTypeId() == TYPEID_PLAYER
                        && properties->Slot >= SUMMON_SLOT_TOTEM
                        && properties->Slot < MAX_TOTEM_SLOT)
                    {
                        // This packet has to be received by the client before the actual unit creation
                        WorldPacket data(SMSG_TOTEM_CREATED, 1+8+4+4);

                        data << uint8(properties->Slot-1);
                        // guessing GUID that will be assigned to the totem
                        data << uint64(MAKE_NEW_GUID(lowGUID, entry, HIGHGUID_UNIT));
                        data << uint32(duration);
                        data << uint32(m_spellInfo->Id);
                        m_originalCaster->ToPlayer()->SendDirectMessage(&data);
                    }

                    summon = m_caster->GetMap()->SummonCreature(entry, pos, properties, duration, m_originalCaster, 0, lowGUID);
                    if (!summon || !summon->isTotem())
                        return;

                    // Mana Tide Totem
                    if (m_spellInfo->Id == 16190)
                        damage = m_caster->CountPctFromMaxHealth(10);

                    if (damage)                                            // if not spell info, DB values used
                    {
                        summon->SetMaxHealth(damage);
                        summon->SetHealth(damage);
                    }

                    //summon->SetUInt32Value(UNIT_CREATED_BY_SPELL,m_spellInfo->Id);

                    if (m_originalCaster->GetTypeId() == TYPEID_PLAYER
                        && properties->Slot >= SUMMON_SLOT_TOTEM
                        && properties->Slot < MAX_TOTEM_SLOT)
                    {
                        // set display id depending on race
                        uint32 displayId = m_originalCaster->GetModelForTotem(PlayerTotemType(properties->Id));
                        summon->SetNativeDisplayId(displayId);
                        summon->SetDisplayId(displayId);

                        //summon->SendUpdateToPlayerm_originalCaster->ToPlayer();
                       
                    }
                    break;
                }
                case SUMMON_TYPE_MINIPET:
                {
                    summon = m_caster->GetMap()->SummonCreature(entry, pos, properties, duration, m_originalCaster);
                    if (!summon || !summon->HasUnitTypeMask(UNIT_MASK_MINION))
                        return;

                    //summon->InitPetCreateSpells();                         // e.g. disgusting oozeling has a create spell as summon...
                    summon->SelectLevel(summon->GetCreatureInfo());       // some summoned creaters have different from 1 DB data for level/hp
                    summon->SetUInt32Value(UNIT_NPC_FLAGS, summon->GetCreatureInfo()->npcflag);

                    summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

                    summon->AI()->EnterEvadeMode();

                    std::string name = m_originalCaster->GetName();
                    name.append(petTypeSuffix[3]);
                    summon->SetName(name);
                    break;
                }
                default:
                {
                    float radius = GetEffectRadius(effIndex);

                    uint32 amount = damage > 0 ? damage : 1;

                    // Spell specific summon counts - "damage" is often set to maximum summon count at a time, so we need to set it manually
                    switch (m_spellInfo->Id)
                    {
                        case 18662: // Curse of Doom
                        case 88747: // Wild Mushroom
                        case 62618: // Power Word: Barrier
                            amount = 1;
                            break;
                        default:
                            break;
                    }

                    for (uint32 count = 0; count < amount; ++count)
                    {
                        GetSummonPosition(effIndex, pos, radius, count);

                        TempSummonType summonType = (duration == 0) ? TEMPSUMMON_DEAD_DESPAWN : TEMPSUMMON_TIMED_DESPAWN;

                        summon = m_originalCaster->SummonCreature(entry, pos, summonType, duration);
                        if (!summon)
                            continue;
                        if (properties->Category == SUMMON_CATEGORY_ALLY)
                        {
                            summon->SetOwnerGUID(m_originalCaster->GetGUID());
                            summon->setFaction(m_originalCaster->getFaction());
                            summon->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);
                        }
                        ExecuteLogEffectSummonObject(effIndex, summon);

                        if (m_caster->GetTypeId() == TYPEID_PLAYER)
                            m_caster->ToPlayer()->AddSummonToMap(entry, summon->GetGUID(),time(NULL));
                    }

                    return;
                }
            }//switch
            break;
        case SUMMON_CATEGORY_PET:
            SummonGuardian(effIndex, entry, properties);
            break;
        case SUMMON_CATEGORY_PUPPET:
            summon = m_caster->GetMap()->SummonCreature(entry, pos, properties, duration, m_originalCaster);
            break;
        case SUMMON_CATEGORY_VEHICLE:
        {
            float x, y, z;
            m_caster->GetClosePoint(x, y, z, DEFAULT_WORLD_OBJECT_SIZE);
            summon = m_caster->GetMap()->SummonCreature(entry, pos, properties, duration, m_caster);
            if (!summon || !summon->IsVehicle())
                return;

            if (m_spellInfo->EffectBasePoints[effIndex])
            {
                SpellEntry const *spellProto = sSpellStore.LookupEntry(SpellMgr::CalculateSpellEffectAmount(m_spellInfo, effIndex));
                if (spellProto)
                    m_caster->CastSpell(summon, spellProto, true);
            }

            m_caster->EnterVehicle(summon->GetVehicleKit());
            break;
        }
    }

    if (summon)
    {
        summon->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);
        summon->SetCreatorGUID(m_originalCaster->GetGUID());
        ExecuteLogEffectSummonObject(effIndex, summon);

        if (m_caster->GetTypeId() == TYPEID_PLAYER)
            m_caster->ToPlayer()->AddSummonToMap(entry, summon->GetGUID(),time(NULL));
    }
}

void Spell::EffectLearnSpell(SpellEffIndex effIndex)
{
    if (!unitTarget)
        return;

    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        if (m_caster->GetTypeId() == TYPEID_PLAYER)
            EffectLearnPetSpell(effIndex);

        return;
    }

    Player *player = (Player*)unitTarget;

    uint32 spellToLearn = (m_spellInfo->Id == 483 || m_spellInfo->Id == 55884) ? damage : m_spellInfo->EffectTriggerSpell[effIndex];
    player->learnSpell(spellToLearn, false);

    sLog->outDebug("Spell: Player %u has learned spell %u from NpcGUID=%u", player->GetGUIDLow(), spellToLearn, m_caster->GetGUIDLow());
}

typedef std::list< std::pair<uint32, uint64> > DispelList;
typedef std::list< std::pair<Aura *, uint8> > DispelChargesList;
void Spell::EffectDispel(SpellEffIndex effIndex)
{
    if (!unitTarget)
        return;

    DispelChargesList dispel_list;

    // Create dispel mask by dispel type
    uint32 dispel_type = m_spellInfo->EffectMiscValue[effIndex];
    uint32 dispelMask  = GetDispellMask(DispelType(dispel_type));

    // we should not be able to dispel diseases if the target is affected by unholy blight
    if (dispelMask & (1 << DISPEL_DISEASE) && unitTarget->HasAura(50536))
        dispelMask &= ~(1 << DISPEL_DISEASE);

    // Cleanse (paladin)
    if (m_spellInfo->Id == 4987 && effIndex == EFFECT_0)
    {
        // Acts of Sacrifice also removes one movement imparing effect
        if (m_caster->HasAura(85795) || m_caster->HasAura(85446))
        {
            uint32 mechanic_mask = (1<<MECHANIC_SNARE)|(1<<MECHANIC_ROOT);
            Unit::AuraApplicationMap const& auraMap = m_caster->GetAppliedAuras();
            for (Unit::AuraApplicationMap::const_iterator iter = auraMap.begin(); iter != auraMap.end(); ++iter)
            {
                Aura const* aura = iter->second->GetBase();
                if (GetAllSpellMechanicMask(aura->GetSpellProto()) & mechanic_mask)
                {
                    m_caster->RemoveAurasDueToSpell(aura->GetId());
                    break;
                }
            }
        }
    }

    Unit::AuraMap const& auras = unitTarget->GetOwnedAuras();
    for (Unit::AuraMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
    {
        Aura * aura = itr->second;
        AuraApplication * aurApp = aura->GetApplicationOfTarget(unitTarget->GetGUID());
        if (!aurApp)
            continue;

        // don't try to remove passive auras
        if (aura->IsPassive())
            continue;

        if ((1<<aura->GetSpellProto()->Dispel) & dispelMask)
        {
            if (aura->GetSpellProto()->Dispel == DISPEL_MAGIC)
            {
                bool positive = aurApp->IsPositive();

                // do not remove positive auras if friendly target
                //               negative auras if non-friendly target
                if (positive == unitTarget->IsFriendlyTo(m_caster))
                    continue;
            }

            // The charges / stack amounts don't count towards the total number of auras that can be dispelled.
            // Ie: A dispel on a target with 5 stacks of Winters Chill and a Polymorph has 1 / (1 + 1) -> 50% chance to dispell
            // Polymorph instead of 1 / (5 + 1) -> 16%.
            bool dispel_charges = aura->GetSpellProto()->AttributesEx7 & SPELL_ATTR7_DISPEL_CHARGES;
            uint8 charges = dispel_charges ? aura->GetCharges() : aura->GetStackAmount();
            if (charges > 0)
                dispel_list.push_back(std::make_pair(aura, charges));
        }
    }

    if (dispel_list.empty())
        return;

    // Ok if exist some buffs for dispel try dispel it
    uint32 failCount = 0;
    DispelList success_list;
    WorldPacket dataFail(SMSG_DISPEL_FAILED, 8+8+4+4+damage*4);
    // dispel N = damage buffs (or while exist buffs for dispel)
    for (int32 count = 0; count < damage && !dispel_list.empty();)
    {
        // Random select buff for dispel
        DispelChargesList::iterator itr = dispel_list.begin();
        std::advance(itr, urand(0, dispel_list.size() - 1));

        bool success = false;
        // 2.4.3 Patch Notes: "Dispel effects will no longer attempt to remove effects that have 100% dispel resistance."
        if (!GetDispelChance(itr->first->GetCaster(), unitTarget, itr->first->GetId(), !unitTarget->IsFriendlyTo(m_caster), &success))
        {
            dispel_list.erase(itr);
            continue;
        }
        else
        {
            if (success)
            {
                success_list.push_back(std::make_pair(itr->first->GetId(), itr->first->GetCasterGUID()));
                --itr->second;
                if (itr->second <= 0)
                    dispel_list.erase(itr);
            }
            else
            {
                if (!failCount)
                {
                    // Failed to dispell
                    dataFail << uint64(m_caster->GetGUID());            // Caster GUID
                    dataFail << uint64(unitTarget->GetGUID());          // Victim GUID
                    dataFail << uint32(m_spellInfo->Id);                // dispel spell id
                }
                ++failCount;
                dataFail << uint32(itr->first->GetId());                         // Spell Id
            }
            ++count;
        }
    }

    if (failCount)
        m_caster->SendMessageToSet(&dataFail, true);

    if (success_list.empty())
        return;

    // Cleanse Spirit
    if (m_spellInfo->Id == 51886)
    {
        // Cleansing Waters - set m_healing (originally not used for this purpose, will be set to 0 after handling)
        if (m_caster->HasAura(86959))
            m_healing = 86961;
        else if (m_caster->HasAura(86962))
            m_healing = 86958;
    }

    WorldPacket dataSuccess(SMSG_SPELLDISPELLOG, 8+8+4+1+4+damage*5);
    // Send packet header
    dataSuccess.append(unitTarget->GetPackGUID());         // Victim GUID
    dataSuccess.append(m_caster->GetPackGUID());           // Caster GUID
    dataSuccess << uint32(m_spellInfo->Id);                // dispel spell id
    dataSuccess << uint8(0);                               // not used
    dataSuccess << uint32(success_list.size());            // count
    for (DispelList::iterator itr = success_list.begin(); itr != success_list.end(); ++itr)
    {
        // Send dispelled spell info
        dataSuccess << uint32(itr->first);              // Spell Id
        dataSuccess << uint8(0);                        // 0 - dispelled !=0 cleansed
        unitTarget->RemoveAurasDueToSpellByDispel(itr->first, itr->second, m_caster);
    }
    m_caster->SendMessageToSet(&dataSuccess, true);

    // On success dispel
    // Devour Magic
    if (m_spellInfo->SpellFamilyName == SPELLFAMILY_WARLOCK && m_spellInfo->Category == SPELLCATEGORY_DEVOUR_MAGIC)
    {
        Unit * pOwner = m_caster->GetOwner();
        m_caster->CastSpell(m_caster, 19658,true);
        // Glyph of Felhunter
            if (pOwner && pOwner->HasAura(56249))
                m_caster->CastSpell(pOwner, 19658,true);
    }

    switch(m_spellInfo->Id)
    {
        // Priest Dispel Magic (real dispel)
        case 97690:
        {
            // Glyph of Dispel Magic
            if (m_caster->HasAura(55677) && unitTarget && unitTarget->IsFriendlyTo(m_caster) && !success_list.empty())
            {
                // Heal the taget for 3% of their max health
                int32 bp0 = unitTarget->CountPctFromMaxHealth(3);
                m_caster->CastCustomSpell(unitTarget, 56131, &bp0, NULL, NULL, true);
            }
            break;
        }
    default: break;
    }
}

void Spell::EffectDualWield(SpellEffIndex /*effIndex*/)
{
    unitTarget->SetCanDualWield(true);
    if (unitTarget->GetTypeId() == TYPEID_UNIT)
        unitTarget->ToCreature()->UpdateDamagePhysical(OFF_ATTACK);
}

void Spell::EffectPull(SpellEffIndex effIndex)
{
    // TODO: create a proper pull towards distract spell center for distract
    EffectNULL(effIndex);
}

void Spell::EffectDistract(SpellEffIndex /*effIndex*/)
{
    // Check for possible target
    if (!unitTarget || unitTarget->isInCombat())
        return;

    // target must be OK to do this
    if (unitTarget->hasUnitState(UNIT_STAT_CONFUSED | UNIT_STAT_STUNNED | UNIT_STAT_FLEEING))
        return;

    float angle = unitTarget->GetAngle(&m_targets.m_dstPos);

    unitTarget->SetFacingTo(angle);

    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
        unitTarget->GetMotionMaster()->MoveDistract(damage * IN_MILLISECONDS);
}

void Spell::EffectPickPocket(SpellEffIndex /*effIndex*/)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    // victim must be creature and attackable
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT || m_caster->IsFriendlyTo(unitTarget))
        return;

    // victim have to be alive and humanoid or undead
    if (unitTarget->isAlive() && (unitTarget->GetCreatureTypeMask() &CREATURE_TYPEMASK_HUMANOID_OR_UNDEAD) != 0)
        m_caster->ToPlayer()->SendLoot(unitTarget->GetGUID(),LOOT_PICKPOCKETING);
}

void Spell::EffectAddFarsight(SpellEffIndex effIndex)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    float radius = GetEffectRadius(effIndex);
    int32 duration = GetSpellDuration(m_spellInfo);
    // Caster not in world, might be spell triggered from aura removal
    if (!m_caster->IsInWorld())
        return;
    DynamicObject* dynObj = new DynamicObject;
    if (!dynObj->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_DYNAMICOBJECT), m_caster, m_spellInfo, m_targets.m_dstPos, radius, true, DYNAMIC_OBJECT_FARSIGHT_FOCUS))
    {
        delete dynObj;
        return;
    }
    dynObj->SetDuration(duration);
    
    dynObj->setActive(true);    //must before add to map to be put in world container
    dynObj->GetMap()->Add(dynObj); //grid will also be loaded

    dynObj->SetCasterViewpoint();
}

void Spell::EffectUntrainTalents(SpellEffIndex effIndex)
{
    if (!unitTarget || m_caster->GetTypeId() == TYPEID_PLAYER)
        return;

    if (uint64 guid = m_caster->GetGUID()) // the trainer is the caster
        unitTarget->ToPlayer()->SendTalentWipeConfirm(guid);
}

void Spell::EffectTeleUnitsFaceCaster(SpellEffIndex effIndex)
{
    if (!unitTarget)
        return;

    if (unitTarget->isInFlight())
        return;

    float fx,fy,fz;
    m_caster->GetClosePoint(fx,fy,fz,unitTarget->GetObjectSize());

    unitTarget->NearTeleportTo(fx,fy,fz,-m_caster->GetOrientation(),unitTarget == m_caster);
}

void Spell::EffectLearnSkill(SpellEffIndex effIndex)
{
    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    if (damage < 0)
        return;

    uint32 skillid =  m_spellInfo->EffectMiscValue[effIndex];

    // If we are learning Riding, we need to dismount to avoid unwanted auras on target
    // This should unmount, so also remove all old "movement increase and flying" auras
    if (skillid == SKILL_RIDING)
        unitTarget->Unmount();

    uint16 skillval = unitTarget->ToPlayer()->GetPureSkillValue(skillid);
    unitTarget->ToPlayer()->SetSkill(skillid, SpellMgr::CalculateSpellEffectAmount(m_spellInfo, effIndex), skillval?skillval:1, damage*75);
}

void Spell::EffectPlayMovie(SpellEffIndex effIndex)
{
    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    uint32 movieId = m_spellInfo->EffectMiscValue[effIndex];
    if (!sMovieStore.LookupEntry(movieId))
        return;

    unitTarget->ToPlayer()->SendMovieStart(movieId);
}

void Spell::EffectTradeSkill(SpellEffIndex /*effIndex*/)
{
    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;
    // uint32 skillid =  m_spellInfo->EffectMiscValue[i];
    // uint16 skillmax = unitTarget->ToPlayer()->(skillid);
    // unitTarget->ToPlayer()->SetSkill(skillid,skillval?skillval:1,skillmax+75);
}

void Spell::EffectEnchantItemPerm(SpellEffIndex effIndex)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;
    if (!itemTarget)
        return;

    Player* p_caster = (Player*)m_caster;

    // Handle vellums
    if (itemTarget->IsVellum())
    {
        // destroy one vellum from stack
        uint32 count = 1;
        p_caster->DestroyItemCount(itemTarget,count,true);
        unitTarget=p_caster;
        // and add a scroll
        DoCreateItem(effIndex,m_spellInfo->EffectItemType[effIndex]);
        itemTarget=NULL;
        m_targets.setItemTarget(NULL);
    }
    else
    {
        // do not increase skill if vellum used
        if (!(m_CastItem && m_CastItem->GetProto()->Flags & ITEM_PROTO_FLAG_TRIGGERED_CAST))
            p_caster->UpdateCraftSkill(m_spellInfo->Id);

        uint32 enchant_id = m_spellInfo->EffectMiscValue[effIndex];
        if (!enchant_id)
            return;

        SpellItemEnchantmentEntry const *pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if (!pEnchant)
            return;
        
        // item can be in trade slot and have owner diff. from caster
        Player* item_owner = itemTarget->GetOwner();
        if (!item_owner)
            return;

        if (item_owner != p_caster && p_caster->GetSession()->GetSecurity() > SEC_PLAYER && sWorld->getBoolConfig(CONFIG_GM_LOG_TRADE))
        {
            sLog->outCommand(p_caster->GetSession()->GetAccountId(),"GM %s (Account: %u) enchanting(perm): %s (Entry: %d) for player: %s (Account: %u)",
                p_caster->GetName(),p_caster->GetSession()->GetAccountId(),
                itemTarget->GetProto()->Name1,itemTarget->GetEntry(),
                item_owner->GetName(),item_owner->GetSession()->GetAccountId());
        }
        if (item_owner != p_caster)
        {
            sLog->outChar("IP:(%s) account:(%u) character:(%s) action:(%s) %s:(name:(%s) entry:(%u)) %s:(name:(%s) account:(%u))",
                         p_caster->GetSession()->GetRemoteAddress().c_str(),
                         p_caster->GetSession()->GetAccountId(),
                         p_caster->GetName(),
                         "enchanting (perm)",
                           "item",
                           itemTarget->GetProto()->Name1,
                           itemTarget->GetEntry(),
                           "for_player",
                           item_owner->GetName(),
                           item_owner->GetSession()->GetAccountId());
        }

        // remove old enchanting before applying new if equipped
        item_owner->ApplyEnchantment(itemTarget,PERM_ENCHANTMENT_SLOT,false);

        itemTarget->SetEnchantment(PERM_ENCHANTMENT_SLOT, enchant_id, 0, 0);

        // add new enchanting if equipped
        item_owner->ApplyEnchantment(itemTarget,PERM_ENCHANTMENT_SLOT,true);

        itemTarget->SetSoulboundTradeable(NULL, item_owner, false);
    }
}

void Spell::EffectEnchantItemPrismatic(SpellEffIndex effIndex)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;
    if (!itemTarget)
        return;

    Player* p_caster = (Player*)m_caster;

    uint32 enchant_id = m_spellInfo->EffectMiscValue[effIndex];
    if (!enchant_id)
        return;

    SpellItemEnchantmentEntry const *pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
    if (!pEnchant)
        return;

    // support only enchantings with add socket in this slot
    {
        bool add_socket = false;
        for (uint8 i = 0; i < MAX_ITEM_ENCHANTMENT_EFFECTS; ++i)
        {
            if (pEnchant->type[i] == ITEM_ENCHANTMENT_TYPE_PRISMATIC_SOCKET)
            {
                add_socket = true;
                break;
            }
        }
        if (!add_socket)
        {
            sLog->outError("Spell::EffectEnchantItemPrismatic: attempt apply enchant spell %u with SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC (%u) but without ITEM_ENCHANTMENT_TYPE_PRISMATIC_SOCKET (%u), not suppoted yet.",
                m_spellInfo->Id,SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC,ITEM_ENCHANTMENT_TYPE_PRISMATIC_SOCKET);
            return;
        }
    }

    // item can be in trade slot and have owner diff. from caster
    Player* item_owner = itemTarget->GetOwner();
    if (!item_owner)
        return;

    if (item_owner != p_caster && p_caster->GetSession()->GetSecurity() > SEC_PLAYER && sWorld->getBoolConfig(CONFIG_GM_LOG_TRADE))
    {
        sLog->outCommand(p_caster->GetSession()->GetAccountId(),"GM %s (Account: %u) enchanting(perm): %s (Entry: %d) for player: %s (Account: %u)",
            p_caster->GetName(),p_caster->GetSession()->GetAccountId(),
            itemTarget->GetProto()->Name1,itemTarget->GetEntry(),
            item_owner->GetName(),item_owner->GetSession()->GetAccountId());
    }
    if (item_owner != p_caster)
    {
        sLog->outChar("IP:(%s) account:(%u) character:(%s) action:(%s) %s:(name:(%s) entry:(%u)) %s:(name:(%s) account:(%u))",
                     p_caster->GetSession()->GetRemoteAddress().c_str(),
                     p_caster->GetSession()->GetAccountId(),
                     p_caster->GetName(),
                     "enchanting (perm)",
                       "item",
                       itemTarget->GetProto()->Name1,
                       itemTarget->GetEntry(),
                       "for_player",
                       item_owner->GetName(),
                       item_owner->GetSession()->GetAccountId());
    }

    // remove old enchanting before applying new if equipped
    item_owner->ApplyEnchantment(itemTarget,PRISMATIC_ENCHANTMENT_SLOT,false);

    itemTarget->SetEnchantment(PRISMATIC_ENCHANTMENT_SLOT, enchant_id, 0, 0);

    // add new enchanting if equipped
    item_owner->ApplyEnchantment(itemTarget,PRISMATIC_ENCHANTMENT_SLOT,true);

    itemTarget->SetSoulboundTradeable(NULL, item_owner, false);
}

void Spell::EffectEnchantItemTmp(SpellEffIndex effIndex)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* p_caster = (Player*)m_caster;

    if (!itemTarget)
        return;

    uint32 enchant_id = m_spellInfo->EffectMiscValue[effIndex];

    if (!enchant_id)
    {
        sLog->outError("Spell %u Effect %u (SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY) have 0 as enchanting id",m_spellInfo->Id,effIndex);
        return;
    }

    SpellItemEnchantmentEntry const *pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
    if (!pEnchant)
    {
        sLog->outError("Spell %u Effect %u (SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY) have not existed enchanting id %u ",m_spellInfo->Id,effIndex,enchant_id);
        return;
    }

    // select enchantment duration
    uint32 duration;

    // rogue family enchantments exception by duration
    if (m_spellInfo->Id == 38615)
        duration = 1800;                                    // 30 mins
    // other rogue family enchantments always 1 hour (some have spell damage=0, but some have wrong data in EffBasePoints)
    else if (m_spellInfo->SpellFamilyName == SPELLFAMILY_ROGUE)
        duration = 3600;                                    // 1 hour
    // shaman family enchantments
    else if (m_spellInfo->SpellFamilyName == SPELLFAMILY_SHAMAN)
        duration = 1800;                                    // 30 mins
    // other cases with this SpellVisual already selected
    else if (m_spellInfo->SpellVisual[0] == 215)
        duration = 1800;                                    // 30 mins
    // some fishing pole bonuses except Glow Worm which lasts full hour
    else if (m_spellInfo->SpellVisual[0] == 563 && m_spellInfo->Id != 64401)
        duration = 600;                                     // 10 mins
    // shaman rockbiter enchantments
    else if (m_spellInfo->SpellVisual[0] == 0)
        duration = 1800;                                    // 30 mins
    else if (m_spellInfo->Id == 29702)
        duration = 300;                                     // 5 mins
    else if (m_spellInfo->Id == 37360)
        duration = 300;                                     // 5 mins
    // default case
    else
        duration = 3600;                                    // 1 hour

    // item can be in trade slot and have owner diff. from caster
    Player* item_owner = itemTarget->GetOwner();
    if (!item_owner)
        return;

    if (item_owner != p_caster && p_caster->GetSession()->GetSecurity() > SEC_PLAYER && sWorld->getBoolConfig(CONFIG_GM_LOG_TRADE))
    {
        sLog->outCommand(p_caster->GetSession()->GetAccountId(),"GM %s (Account: %u) enchanting(temp): %s (Entry: %d) for player: %s (Account: %u)",
            p_caster->GetName(), p_caster->GetSession()->GetAccountId(),
            itemTarget->GetProto()->Name1, itemTarget->GetEntry(),
            item_owner->GetName(), item_owner->GetSession()->GetAccountId());
    }

    if (item_owner != p_caster)
    {
        sLog->outChar("IP:(%s) account:(%u) character:(%s) action:(%s) %s:(name:(%s) entry:(%u)) %s:(name:(%s) account:(%u))",
                     p_caster->GetSession()->GetRemoteAddress().c_str(),
                     p_caster->GetSession()->GetAccountId(),
                     p_caster->GetName(),
                     "enchanting (temp)",
                       "item",
                       itemTarget->GetProto()->Name1,
                       itemTarget->GetEntry(),
                       "for_player",
                       item_owner->GetName(),
                       item_owner->GetSession()->GetAccountId());
    }

    // remove old enchanting before applying new if equipped
    item_owner->ApplyEnchantment(itemTarget,TEMP_ENCHANTMENT_SLOT, false);

    itemTarget->SetEnchantment(TEMP_ENCHANTMENT_SLOT, enchant_id, duration * 1000, 0);

    // add new enchanting if equipped
    item_owner->ApplyEnchantment(itemTarget, TEMP_ENCHANTMENT_SLOT, true);
}

void Spell::EffectTameCreature(SpellEffIndex /*effIndex*/)
{
    if (m_caster->GetPetGUID())
        return;

    if (!unitTarget)
        return;

    if (unitTarget->GetTypeId() != TYPEID_UNIT)
        return;

    Creature* creatureTarget = unitTarget->ToCreature();

    if (creatureTarget->isPet())
        return;

    if (m_caster->getClass() != CLASS_HUNTER)
        return;

    // If we have a full list we shouldn't be able to create a new one.
    if (m_caster->ToPlayer()->getSlotForNewPet() == PET_SLOT_FULL_LIST)
    {
        // Need to get the right faluire numbers or maby a custom message to the screen ?
        return;
    }

    // cast finish successfully
    //SendChannelUpdate(0);
    finish();

    Pet* pet = m_caster->CreateTamedPetFrom(creatureTarget,m_spellInfo->Id);
    if (!pet)                                               // in very specific state like near world end/etc.
        return;

    // "kill" original creature
    creatureTarget->ForcedDespawn();

    uint8 level = (creatureTarget->getLevel() < (m_caster->getLevel() - 5)) ? (m_caster->getLevel() - 5) : creatureTarget->getLevel();

    // prepare visual effect for levelup
    pet->SetUInt32Value(UNIT_FIELD_LEVEL, level - 1);

    // add to world
    pet->GetMap()->Add(pet->ToCreature());

    // visual effect for levelup
    pet->SetUInt32Value(UNIT_FIELD_LEVEL, level);

    // caster have pet now
    m_caster->SetMinion(pet, true, m_caster->GetTypeId() == TYPEID_PLAYER ? m_caster->ToPlayer()->getSlotForNewPet() : PET_SLOT_UNK_SLOT);

    pet->InitTalentForLevel();

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        //slot is defined by SetMinion.
        m_caster->ToPlayer()->getSlotForNewPet();
        pet->SavePetToDB(m_caster->ToPlayer()->m_currentPetSlot);
        m_caster->ToPlayer()->PetSpellInitialize();
    }
}

void Spell::EffectSummonPet(SpellEffIndex effIndex)
{
    Player *owner = NULL;
    if (m_originalCaster)
    {
        if (m_originalCaster->GetTypeId() == TYPEID_PLAYER)
            owner = (Player*)m_originalCaster;
        else if (m_originalCaster->ToCreature()->isTotem())
            owner = m_originalCaster->GetCharmerOrOwnerPlayerOrPlayerItself();
    }

    uint32 petentry = m_spellInfo->EffectMiscValue[effIndex];
    
    PetSlot slot = (PetSlot)m_spellInfo->EffectBasePoints[effIndex];
    switch (petentry)
    {
        case 0:    // any hunter pet
            break;
        case 416:  // Imp
            slot = PET_SLOT_HUNTER_FIRST;
            break;
        case 1860: // Voidwalker
            slot = (PetSlot)(PET_SLOT_HUNTER_FIRST+1);
            break;
        case 1863: // Succubus
            slot = (PetSlot)(PET_SLOT_HUNTER_FIRST+2);
            break;
        case 417:  // Felhunter
            slot = (PetSlot)(PET_SLOT_HUNTER_FIRST+3);
            break;
        default:
            slot = PET_SLOT_UNK_SLOT;
            break;
    }
    
    if (!owner)
    {
        SummonPropertiesEntry const *properties = sSummonPropertiesStore.LookupEntry(67);
        if (properties)
            SummonGuardian(effIndex, petentry, properties);
        return;
    }

    Pet *OldSummon = owner->GetPet();

    // if pet requested type already exist
    if (OldSummon)
    {
        if (petentry == 0 || OldSummon->GetEntry() == petentry)
        {
            // pet in corpse state can't be summoned
            if (OldSummon->isDead())
                return;

            ASSERT(OldSummon->GetMap() == owner->GetMap());

            //OldSummon->GetMap()->Remove(OldSummon->ToCreature(),false);

            float px, py, pz;
            owner->GetClosePoint(px, py, pz, OldSummon->GetObjectSize());

            OldSummon->NearTeleportTo(px, py, pz, OldSummon->GetOrientation());
            //OldSummon->Relocate(px, py, pz, OldSummon->GetOrientation());
            //OldSummon->SetMap(owner->GetMap());
            //owner->GetMap()->Add(OldSummon->ToCreature());

            if (owner->GetTypeId() == TYPEID_PLAYER && OldSummon->isControlled())
                owner->ToPlayer()->PetSpellInitialize();

            return;
        }

        if (owner->GetTypeId() == TYPEID_PLAYER)
            owner->ToPlayer()->RemovePet(OldSummon,PET_SLOT_ACTUAL_PET_SLOT,false);
        else
            return;
    }

    float x, y, z;
    owner->GetClosePoint(x, y, z, owner->GetObjectSize());
    Pet* pet = owner->SummonPet(petentry, x, y, z, owner->GetOrientation(), SUMMON_PET, 0, slot);
    if (!pet)
        return;

    if (m_caster->GetTypeId() == TYPEID_UNIT)
    {
        if (m_caster->ToCreature()->isTotem())
            pet->SetReactState(REACT_AGGRESSIVE);
        else
            pet->SetReactState(REACT_DEFENSIVE);
    }

    pet->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);

    // generate new name for summon pet
    std::string new_name=sObjectMgr->GeneratePetName(petentry);
    if (!new_name.empty())
        pet->SetName(new_name);

    ExecuteLogEffectSummonObject(effIndex, pet);
}

void Spell::EffectLearnPetSpell(SpellEffIndex effIndex)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player *_player = (Player*)m_caster;

    Pet *pet = _player->GetPet();
    if (!pet)
        return;
    if (!pet->isAlive())
        return;

    SpellEntry const *learn_spellproto = sSpellStore.LookupEntry(m_spellInfo->EffectTriggerSpell[effIndex]);
    if (!learn_spellproto)
        return;

    pet->learnSpell(learn_spellproto->Id);

    pet->SavePetToDB(PET_SLOT_ACTUAL_PET_SLOT);
    _player->PetSpellInitialize();
}

void Spell::EffectTaunt(SpellEffIndex /*effIndex*/)
{
    if (!unitTarget)
        return;

    // this effect use before aura Taunt apply for prevent taunt already attacking target
    // for spell as marked "non effective at already attacking target"
    if (!unitTarget || !unitTarget->CanHaveThreatList()
        || unitTarget->getVictim() == m_caster)
    {
        SendCastResult(SPELL_FAILED_DONT_REPORT);
        return;
    }

    if (m_spellInfo->Id == 62124) // Hand of Reckoning
    {
        int32 damageDone = int32(1 + m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.5f);
        bool is_crit = m_caster->isSpellCrit(unitTarget, m_spellInfo, m_spellSchoolMask, m_attackType);
        if (is_crit)
            damageDone *= 2;
        m_caster->DealDamage(unitTarget, damageDone, NULL, SPELL_DIRECT_DAMAGE, SPELL_SCHOOL_MASK_HOLY, m_spellInfo, false);
        m_caster->SendSpellNonMeleeDamageLog(unitTarget, m_spellInfo->Id, damageDone, SPELL_SCHOOL_MASK_HOLY, 0, 0, false, false, is_crit);
    }

    // Also use this effect to set the taunter's threat to the taunted creature's highest value
    if (unitTarget->getThreatManager().getCurrentVictim())
    {
        float myThreat = unitTarget->getThreatManager().getThreat(m_caster);
        float itsThreat = unitTarget->getThreatManager().getCurrentVictim()->getThreat();
        if (itsThreat > myThreat)
            unitTarget->getThreatManager().addThreat(m_caster, itsThreat - myThreat);
    }

    //Set aggro victim to caster
    if (!unitTarget->getThreatManager().getOnlineContainer().empty())
        if (HostileReference* forcedVictim = unitTarget->getThreatManager().getOnlineContainer().getReferenceByTarget(m_caster))
            unitTarget->getThreatManager().setCurrentVictim(forcedVictim);

    if (unitTarget->ToCreature()->IsAIEnabled && !unitTarget->ToCreature()->HasReactState(REACT_PASSIVE))
        unitTarget->ToCreature()->AI()->AttackStart(m_caster);
}

void Spell::EffectWeaponDmg(SpellEffIndex /*effIndex*/)
{
}

void Spell::SpellDamageWeaponDmg(SpellEffIndex effIndex)
{
    if (!unitTarget)
        return;
    if (!unitTarget->isAlive())
        return;

    // multiple weapon dmg effect workaround
    // execute only the last weapon damage
    // and handle all effects at once
    for (uint32 j = effIndex + 1; j < MAX_SPELL_EFFECTS; ++j)
    {
        switch (m_spellInfo->Effect[j])
        {
            case SPELL_EFFECT_WEAPON_DAMAGE:
            case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
            case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
            case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
                return;     // we must calculate only at last weapon effect
            break;
        }
    }

    // some spell specific modifiers
    float totalDamagePercentMod  = 1.0f;                    // applied to final bonus+weapon damage
    int32 fixed_bonus = 0;
    int32 spell_bonus = 0;                                  // bonus specific for spell
    bool damage_bonus = true;                               // count damage bonus?

    switch (m_spellInfo->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch (m_spellInfo->Id)
            {
                case 69055:     // Saber Lash
                case 70814:     // Saber Lash
                {
                    uint32 count = 0;
                    for (std::list<TargetInfo>::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                        if (ihit->effectMask & (1 << effIndex))
                            ++count;

                    totalDamagePercentMod /= count;
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            // Devastate (player ones)
            if (m_spellInfo->SpellFamilyFlags[1] & 0x40)
            {
                // Player can apply only 58567 Sunder Armor effect.
                bool needCast = !unitTarget->HasAura(58567);
                if (needCast)
                    m_caster->CastSpell(unitTarget, 58567, true);

                if (Aura * aur = unitTarget->GetAura(58567))
                {
                    // 58388 - Glyph of Devastate dummy aura.
                    if (int32 num = (needCast ? 0 : 1) + (m_caster->HasAura(58388) ? 1 : 0))
                        aur->ModStackAmount(num);
                    fixed_bonus += (aur->GetStackAmount() - 1) * CalculateDamage(2, unitTarget);
                }
            }
            // Mortal Strike
            else if (m_spellInfo->Id == 12294)
            {
                // talent Lambs to the Slaughter
                if (m_caster->HasAura(84583) || m_caster->HasAura(84587) || m_caster->HasAura(84588))
                    if (unitTarget->HasAura(94009, m_caster->GetGUID())) // rend
                        unitTarget->GetAura(94009, m_caster->GetGUID())->RefreshDuration(); // refresh rend
            }
            break;
        }
        case SPELLFAMILY_ROGUE:
        {
            // Hemorrhage
            if (m_spellInfo->Id == 16511)
            {
                if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    m_caster->ToPlayer()->AddComboPoints(unitTarget, 1, this);

                // 1.447 damage multiplier with daggers
                if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    if (Item* item = m_caster->ToPlayer()->GetWeaponForAttack(m_attackType, true))
                        if (item->GetProto()->SubClass == ITEM_SUBCLASS_WEAPON_DAGGER)
                            totalDamagePercentMod *= 1.447f;
            }
            // Ambush
            else if (m_spellInfo->Id == 8676)
            {
                // 1.447 damage multiplier with daggers
                if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    if (Item* item = m_caster->ToPlayer()->GetWeaponForAttack(m_attackType, true))
                        if (item->GetProto()->SubClass == ITEM_SUBCLASS_WEAPON_DAGGER)
                            totalDamagePercentMod *= 1.447f;
            }
            // Mutilate (for each hand)
            else if (m_spellInfo->SpellFamilyFlags[1] & 0x6)
            {
                bool found = false;
                // fast check
                if (unitTarget->HasAuraState(AURA_STATE_DEADLY_POISON, m_spellInfo, m_caster))
                    found = true;
                // full aura scan
                else
                {
                    Unit::AuraApplicationMap const& auras = unitTarget->GetAppliedAuras();
                    for (Unit::AuraApplicationMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                    {
                        if (itr->second->GetBase()->GetSpellProto()->Dispel == DISPEL_POISON)
                        {
                            found = true;
                            break;
                        }
                    }
                }

                if (found)
                    totalDamagePercentMod *= 1.2f;          // 120% if poisoned
            }
            // Backstab
            else if (m_spellInfo->Id == 53)
            {
                if (m_caster && m_caster->ToPlayer() && unitTarget && unitTarget->GetHealthPct() < 35.0f)
                {
                    // Murderous Intent
                    int32 bp0 = 0;
                    if (m_caster->HasAura(14159))
                        bp0 = 30;
                    else if (m_caster->HasAura(14158))
                        bp0 = 15;

                    if (bp0)
                        m_caster->CastCustomSpell(m_caster, 79132, &bp0, 0, 0, true);
                }
            }
            // Main Gauche bonus attack
            else if (m_spellInfo->Id == 86392)
            {
                // Combat Potency energy gain chance
                if (m_caster && roll_chance_i(20))
                {
                    Aura* pPotency = m_caster->GetAura(35551);
                    if (!pPotency)
                        pPotency = m_caster->GetAura(35550);
                    if (!pPotency)
                        pPotency = m_caster->GetAura(35541);

                    if (pPotency)
                        m_caster->CastSpell(m_caster, pPotency->GetSpellProto()->EffectTriggerSpell[0], true);
                }
            }
            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            // Seal of Command Unleashed
            if (m_spellInfo->Id == 20467)
            {
                spell_bonus += int32(0.08f*m_caster->GetTotalAttackPowerValue(BASE_ATTACK));
                spell_bonus += int32(0.13f*m_caster->SpellBaseDamageBonus(GetSpellSchoolMask(m_spellInfo)));
            }
            // Templar's Verdict
            else if (m_spellInfo->Id == 85256)
            {
                uint32 realholypower = m_caster->GetPower(POWER_HOLY_POWER)+1;
                bool takePower = true;
                // Divine Purpose effect
                if (m_caster->HasAura(90174))
                {
                    takePower = false;
                    realholypower = 3;
                    m_caster->RemoveAurasDueToSpell(90174);
                }
                switch(realholypower)
                {
                    case 1:
                    default:
                        totalDamagePercentMod *= 1.0f;                // stay on 30%
                        break;
                    case 2:
                        totalDamagePercentMod *= 3.0f;                // 90%
                        break;
                    case 3:
                        totalDamagePercentMod *= 235.0f/30.0f;        // 235%
                        break;
                }
                if (takePower)
                    m_caster->SetPower(POWER_HOLY_POWER,0);
            }
            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            // Skyshatter Harness item set bonus
            // Stormstrike
            if (AuraEffect * aurEff = m_caster->IsScriptOverriden(m_spellInfo, 5634))
                m_caster->CastSpell(m_caster, 38430, true, NULL, aurEff);

            // Lava Lash
            if (m_spellInfo->Id == 60103)
            {
                // Improved Lava Lash
                if (unitTarget->HasAura(8050) && (m_caster->HasAura(77700) || m_caster->HasAura(77701)))
                    m_caster->CastSpell(unitTarget, 105792, true);
            }
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            // Mangle (Cat): CP
            if (m_spellInfo->SpellFamilyFlags[1] & 0x400)
            {
                if (m_caster->GetTypeId() == TYPEID_PLAYER)
                    m_caster->ToPlayer()->AddComboPoints(unitTarget,1, this);

                if (m_caster->HasAura(54815)) // Glyph of Bloodletting
                {
                    if (AuraEffect *ripEff = unitTarget->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DRUID, 0x00800000, 0x0, 0x0, m_caster->GetGUID()))
                    {
                        int32 refreshCounter = ripEff->GetScriptedAmount();
                        if (refreshCounter < 3)
                        {
                            ripEff->GetBase()->SetDuration(ripEff->GetBase()->GetDuration() + 2000);
                            ripEff->SetScriptedAmount(refreshCounter + 1);
                        }
                    }
                }
            }
            // Shred
            else if (m_spellInfo->Id == 5221)
            {
                if (unitTarget->HasAuraState(AURA_STATE_BLEEDING))
                {
                    // Rend and Tear talent bonus
                    if (m_caster->HasAura(48434))
                        totalDamagePercentMod *= 1.20f;
                    else if (m_caster->HasAura(48433))
                        totalDamagePercentMod *= 1.13f;
                    else if (m_caster->HasAura(48432))
                        totalDamagePercentMod *= 1.07f;
                }
            }
            // Ravage! (Stampede)
            else if (m_spellInfo->Id == 81170)
            {
                m_caster->RemoveAurasDueToSpell(89140); // enabler spell
                m_caster->RemoveAurasDueToSpell(81021); // Stampede buff rank 1
                m_caster->RemoveAurasDueToSpell(81022); // Stampede buff rank 2

                if (m_caster->HasAura(48483)) // Infected Wounds (Rank 1)
                    m_caster->CastSpell(unitTarget, 58179, true);
                else if (m_caster->HasAura(48484)) // Infected Wounds (Rank 2)
                    m_caster->CastSpell(unitTarget, 58180, true);

            }
            else if (m_spellInfo->Id == 6785) // Ravage
            {
                if (m_caster->HasAura(48483)) // Infected Wounds (Rank 1)
                    m_caster->CastSpell(unitTarget, 58179, true);
                else if (m_caster->HasAura(48484)) // Infected Wounds (Rank 2)
                    m_caster->CastSpell(unitTarget, 58180, true);
            }
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            float shotMod = 0;
            switch(m_spellInfo->Id)
            {
                case 53351: // Kill Shot
                {
                    // "You attempt to finish the wounded target off, firing a long range attack dealing % weapon damage plus RAP*0.30+543."
                    shotMod = 0.3f;
                    break;
                }
                case 56641: // Steady Shot
                {
                    // "A steady shot that causes % weapon damage plus RAP*0.021+280. Generates 9 Focus."
                    // focus effect done in dummy
                    shotMod = 0.021f;
                    break;
                }
                case 19434: // Aimed Shot
                case 82928: // Aimed Shot (Master Marksman)
                {
                    // "A powerful aimed shot that deals % ranged weapon damage plus (RAP * 0.724)+776."
                    shotMod = 0.724f;
                    // Remove Master Marksman proc spell "Fire!"
                    if (m_caster->HasAura(82926) && m_spellInfo->Id == 82928)
                        m_caster->RemoveAurasDueToSpell(82926);
                    break;
                }
                case 77767: // Cobra Shot
                {
                    // "Deals weapon damage plus (276 + (RAP * 0.017)) in the form of Nature damage and increases the duration of your Serpent Sting on the target by 6 sec. Generates 9 Focus."
                    shotMod = 0.017f;
                    break;
                }
                case 3044: // Arcane Shot
                case 53209: // Chimera Shot
                {
                    // "An instant shot that causes % weapon damage plus (RAP * 0.0483)+289 as Arcane damage."
                    if (m_spellInfo->SpellFamilyFlags[0] & 0x800)
                        shotMod = 0.0483f;

                    // "An instant shot that causes ranged weapon damage plus RAP*0.732+1620, refreshing the duration of  your Serpent Sting and healing you for 5% of your total health."
                    if (m_spellInfo->Id == 53209)
                        shotMod = 0.732f;

                    // Marked for Death 1,2
                    if(m_caster->HasAuraEffect(53241,0,0))
                        if(roll_chance_i(sSpellStore.LookupEntry(53241)->EffectBasePoints[0]))
                        {
                            m_caster->CastSpell(m_caster->ToPlayer()->GetSelectedUnit(),88691,true);
                            break;
                        }
                    if(m_caster->HasAuraEffect(53243,0,0))
                        if(roll_chance_i(sSpellStore.LookupEntry(53243)->EffectBasePoints[0]))
                        {
                            m_caster->CastSpell(m_caster->ToPlayer()->GetSelectedUnit(),88691,true);
                            break;
                        }
                    break;
                }
                default:
                    break;
            }
            spell_bonus += int32((shotMod*m_caster->GetTotalAttackPowerValue(RANGED_ATTACK)));
            break;
        }
        case SPELLFAMILY_WARLOCK:
        {
            // Felstorm (felguard)
            if (m_spellInfo->Id == 89753 && m_caster->ToPet())
                spell_bonus += (m_caster->ToPet()->GetBonusDamage()/0.15f)*0.231f;
            // Legion Strike (felguard)
            else if (m_spellInfo->Id == 30213 && m_caster->ToPet())
                spell_bonus += (m_caster->ToPet()->GetBonusDamage()/0.15f)*0.264f;
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            // Blood Strike
            if (m_spellInfo->SpellFamilyFlags[EFFECT_0] & 0x400000)
            {
                float diseaseBonus = SpellMgr::CalculateSpellEffectAmount(m_spellInfo, EFFECT_2) / 10.0f;   // damage bonus per 1 disease
                uint32 diseaseCount = unitTarget->GetDiseasesByCaster(m_caster->GetGUID());
                AddPctF(totalDamagePercentMod, diseaseBonus * diseaseCount);
                break;
            }
            // Death Strike
            if (m_spellInfo->SpellFamilyFlags[EFFECT_0] & 0x10)
            {
                // Glyph of Death Strike
                if (AuraEffect const * aurEff = m_caster->GetAuraEffect(59336, EFFECT_0))
                {
                    if (uint32 runic = (m_caster->GetPower(POWER_RUNIC_POWER) / 10))  // runic power is in range 0 - 1000 => need to divide by 10
                    {
                        runic /= 5;     // bonus is applied for every 5 runic power
                        float bonus = runic * SpellMgr::CalculateSpellEffectAmount(aurEff->GetSpellProto(), EFFECT_0);
                        bonus = std::min(bonus, (float)SpellMgr::CalculateSpellEffectAmount(aurEff->GetSpellProto(), EFFECT_1));
                        AddPctF(totalDamagePercentMod, bonus);
                    }
                }
                break;
            }
            // Obliterate (12.5% more damage per disease)
            if (m_spellInfo->SpellFamilyFlags[EFFECT_1] & 0x20000)
            {
                bool consumeDiseases = false;

                totalDamagePercentMod *= ((SpellMgr::CalculateSpellEffectAmount(m_spellInfo, EFFECT_2) * unitTarget->GetDiseasesByCaster(m_caster->GetGUID(), consumeDiseases) / 2.0f) + 100.0f) / 100.0f;
                break;
            }
            // Blood-Caked Strike - Blood-Caked Blade
            if (m_spellInfo->SpellIconID == 1736)
            {
                totalDamagePercentMod *= ((unitTarget->GetDiseasesByCaster(m_caster->GetGUID()) * 12.5f) + 100.0f) / 100.0f;
                break;
            }
            // Heart Strike
            if (m_spellInfo->SpellFamilyFlags[EFFECT_0] & 0x1000000)
            {
                totalDamagePercentMod *= ((SpellMgr::CalculateSpellEffectAmount(m_spellInfo, EFFECT_2) * unitTarget->GetDiseasesByCaster(m_caster->GetGUID())) + 100.0f) / 100.0f;
                break;
            }
            break;
        }
    }

    bool normalized = false;
    float weaponDamagePercentMod = 1.0;
    for (int j = 0; j < MAX_SPELL_EFFECTS; ++j)
    {
        switch(m_spellInfo->Effect[j])
        {
            case SPELL_EFFECT_WEAPON_DAMAGE:
            case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
                fixed_bonus += CalculateDamage(j, unitTarget);
                break;
            case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
                fixed_bonus += CalculateDamage(j, unitTarget);
                normalized = true;
                break;
            case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
                weaponDamagePercentMod *= float(CalculateDamage(j,unitTarget)) / 100.0f;
                break;
            default:
                break;                                      // not weapon damage effect, just skip
        }
    }

    // apply to non-weapon bonus weapon total pct effect, weapon total flat effect included in weapon damage
    if (fixed_bonus || spell_bonus)
    {
        UnitMods unitMod;
        switch(m_attackType)
        {
            default:
            case BASE_ATTACK:   unitMod = UNIT_MOD_DAMAGE_MAINHAND; break;
            case OFF_ATTACK:    unitMod = UNIT_MOD_DAMAGE_OFFHAND;  break;
            case RANGED_ATTACK: unitMod = UNIT_MOD_DAMAGE_RANGED;   break;
        }

        float weapon_total_pct = 1.0f;
        if (m_spellInfo->SchoolMask & SPELL_SCHOOL_MASK_NORMAL)
             weapon_total_pct = m_caster->GetModifierValue(unitMod, TOTAL_PCT);

        if (fixed_bonus)
            fixed_bonus = int32(fixed_bonus * weapon_total_pct);

        if (spell_bonus)
            spell_bonus = int32(spell_bonus * weapon_total_pct);
    }

    float weaponDamage = (float) m_caster->CalculateDamage(m_attackType, normalized, true);

    // Sequence is important
    for (int j = 0; j < MAX_SPELL_EFFECTS; ++j)
    {
        // We assume that a spell have at most one fixed_bonus
        // and at most one weaponDamagePercentMod
        switch(m_spellInfo->Effect[j])
        {
            case SPELL_EFFECT_WEAPON_DAMAGE:
            case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
            case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
                weaponDamage += fixed_bonus;
                break;
            case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
                weaponDamage = weaponDamage * weaponDamagePercentMod;
            default:
                break;                                      // not weapon damage effect, just skip
        }
    }

    if (spell_bonus)
        weaponDamage += spell_bonus;

    if (totalDamagePercentMod != 1.0f)
        weaponDamage = weaponDamage * totalDamagePercentMod;

    // prevent negative damage
    uint32 eff_damage = uint32(weaponDamage > 0 ? weaponDamage : 0);

    // Add melee damage bonuses (also check for negative)
    if (damage_bonus)
        m_caster->MeleeDamageBonus(unitTarget, &eff_damage, m_attackType, m_spellInfo);
    m_damage+= eff_damage;

    // Custom after damage calculation spell bonuses
    switch (m_spellInfo->Id)
    {
        case 53385: // Divine Storm
            {
                int32 dmg = m_damage * m_spellInfo->EffectBasePoints[1] / 100.0f;
                m_caster->CastCustomSpell(m_caster, 54171, &dmg, 0, 0, true);
            }
            break;
        case 53351: // Kill Shot
            // Glyph of Kill shot
            if (m_caster && m_caster->ToPlayer()          // is a player
                && m_caster->HasAura(63067)               // has the glyph
                && !m_caster->HasAura(90967)              // glyph is not on cooldown
                && int32(unitTarget->GetHealth()) > m_damage)      // target is not killed
            {
                m_caster->CastSpell(m_caster, 90967, true); // Cast cooldown reset dummy spell (+ cooldown)
                m_caster->CastSpell(m_caster, 77691, true); // dummy hack!
            }
            break;
    }
}

void Spell::EffectThreat(SpellEffIndex /*effIndex*/)
{
    if (!unitTarget || !unitTarget->isAlive() || !m_caster->isAlive())
        return;

    if (!unitTarget->CanHaveThreatList())
        return;

    unitTarget->AddThreat(m_caster, float(damage));
}

void Spell::EffectHealMaxHealth(SpellEffIndex effIndex)
{
    if (!unitTarget || !unitTarget->isAlive())
        return;

    int32 addhealth;
    if (m_spellInfo->SpellFamilyName == SPELLFAMILY_PALADIN) // Lay on Hands
    {
        if (m_caster->GetGUID() == unitTarget->GetGUID())
            m_caster->CastSpell(m_caster, 25771, true); // Forbearance

        if (m_caster->HasAura(54939)) // Glyph of Divinity
            m_caster->CastSpell(m_caster,54986,true);
    }

    // damage == 0 - heal for caster max health
    if (damage == 0)
        addhealth = m_caster->GetMaxHealth();
    else
        addhealth = unitTarget->GetMaxHealth() - unitTarget->GetHealth();

    if (m_originalCaster)
         m_healing += m_originalCaster->SpellHealingBonus(unitTarget,m_spellInfo, effIndex, addhealth, HEAL);
}

void Spell::EffectInterruptCast(SpellEffIndex effIndex)
{
    if (!unitTarget || !unitTarget->isAlive())
        return;

    // TODO: not all spells that used this effect apply cooldown at school spells
    // also exist case: apply cooldown to interrupted cast only and to all spells
    for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
    {
        if (Spell* spell = unitTarget->GetCurrentSpell(CurrentSpellTypes(i)))
        {
            SpellEntry const* curSpellInfo = spell->m_spellInfo;
            uint32 interruptFlags = (i == CURRENT_CHANNELED_SPELL) ? curSpellInfo->ChannelInterruptFlags : curSpellInfo->InterruptFlags;
            // check if we can interrupt spell
            if ((spell->getState() == SPELL_STATE_CASTING
                || (spell->getState() == SPELL_STATE_PREPARING && spell->GetCastTime() > 0.0f))
                && (interruptFlags & SPELL_INTERRUPT_FLAG_INTERRUPT) && curSpellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE)
            {
                if (m_originalCaster)
                {
                    int32 duration = m_originalCaster->ModSpellDuration(m_spellInfo, unitTarget, m_originalCaster->CalcSpellDuration(m_spellInfo), false);
                    unitTarget->ProhibitSpellScholl(GetSpellSchoolMask(curSpellInfo), duration/*GetSpellDuration(m_spellInfo)*/);
                }
                ExecuteLogEffectInterruptCast(effIndex, unitTarget, curSpellInfo->Id);
                unitTarget->InterruptSpell(CurrentSpellTypes(i), false);

                switch(m_spellInfo->Id)
                {
                    case 6552://Rude interruption
                        if(m_originalCaster->HasAura(61216))
                            m_originalCaster->CastSpell(m_originalCaster,86662,true);
                        else if(m_originalCaster->HasAura(61221))
                            m_originalCaster->CastSpell(m_originalCaster,86663,true);
                        break;
                    case 1766://Glyph of Kick
                        if(m_originalCaster->ToPlayer() && m_originalCaster->HasAura(56805))
                            m_originalCaster->ToPlayer()->ModifySpellCooldown(1766,-6000,true);
                        break;
                }
            }
        }
    }
}

void Spell::EffectSummonObjectWild(SpellEffIndex effIndex)
{
    uint32 gameobject_id = m_spellInfo->EffectMiscValue[effIndex];

    GameObject* pGameObj = new GameObject;

    WorldObject* target = focusObject;
    if (!target)
        target = m_caster;

    float x, y, z;
    if (m_targets.HasDst())
        m_targets.m_dstPos.GetPosition(x, y, z);
    else
        m_caster->GetClosePoint(x, y, z, DEFAULT_WORLD_OBJECT_SIZE);

    Map *map = target->GetMap();

    if (!pGameObj->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_GAMEOBJECT), gameobject_id, map,
        m_caster->GetPhaseMask(), x, y, z, target->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 100, GO_STATE_READY))
    {
        delete pGameObj;
        return;
    }

    int32 duration = GetSpellDuration(m_spellInfo);

    pGameObj->SetRespawnTime(duration > 0 ? duration/IN_MILLISECONDS : 0);
    pGameObj->SetSpellId(m_spellInfo->Id);

    ExecuteLogEffectSummonObject(effIndex, pGameObj);

    // Wild object not have owner and check clickable by players
    map->Add(pGameObj);

    if (pGameObj->GetGoType() == GAMEOBJECT_TYPE_FLAGDROP && m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        Player *pl = m_caster->ToPlayer();
        Battleground* bg = pl->GetBattleground();

        switch(pGameObj->GetMapId())
        {
            case 489:                                       //WS
            {
                if (bg && bg->GetTypeID(true) == BATTLEGROUND_WS && bg->GetStatus() == STATUS_IN_PROGRESS)
                {
                    uint32 team = ALLIANCE;

                    if (pl->GetTeam() == team)
                        team = HORDE;

                    ((BattlegroundWS*)bg)->SetDroppedFlagGUID(pGameObj->GetGUID(),team);
                }
                break;
            }
            case 566:                                       //EY
            {
                if (bg && bg->GetTypeID(true) == BATTLEGROUND_EY && bg->GetStatus() == STATUS_IN_PROGRESS)
                {
                    ((BattlegroundEY*)bg)->SetDroppedFlagGUID(pGameObj->GetGUID());
                }
                break;
            }
        }
    }

    if (uint32 linkedEntry = pGameObj->GetGOInfo()->GetLinkedGameObjectEntry())
    {
        GameObject* linkedGO = new GameObject;
        if (linkedGO->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_GAMEOBJECT), linkedEntry, map,
            m_caster->GetPhaseMask(), x, y, z, target->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 100, GO_STATE_READY))
        {
            linkedGO->SetRespawnTime(duration > 0 ? duration/IN_MILLISECONDS : 0);
            linkedGO->SetSpellId(m_spellInfo->Id);

            ExecuteLogEffectSummonObject(effIndex, linkedGO);

            // Wild object not have owner and check clickable by players
            map->Add(linkedGO);
        }
        else
        {
            delete linkedGO;
            linkedGO = NULL;
            return;
        }
    }
}

void Spell::EffectScriptEffect(SpellEffIndex effIndex)
{
    // TODO: we must implement hunter pet summon at login there (spell 6962)

    switch(m_spellInfo->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch(m_spellInfo->Id)
            {
                case 45204: // Clone Me!
                case 41055: // Copy Weapon
                case 45206: // Copy Off-hand Weapon
                    unitTarget->CastSpell(m_caster, damage, false);
                    break;
                case 45205: // Copy Offhand Weapon
                case 41054: // Copy Weapon
                    m_caster->CastSpell(unitTarget, damage, false);
                    break;
                case 87212: // Shadowy Apparition
                {
                    if (!unitTarget || !m_originalCaster)
                        break;

                    int32 bp0 = 1; // count = 1
                    m_originalCaster->CastCustomSpell(m_originalCaster, 87426, &bp0, 0, 0, true, 0, 0, unitTarget->GetGUID());
                    break;
                }
                case 55693:                                 // Remove Collapsing Cave Aura
                    if (!unitTarget)
                        return;
                    unitTarget->RemoveAurasDueToSpell(SpellMgr::CalculateSpellEffectAmount(m_spellInfo, effIndex));
                    break;
                // PX-238 Winter Wondervolt TRAP
                case 26275:
                {
                    uint32 spells[4] = { 26272, 26157, 26273, 26274 };

                    // check presence
                    for (uint8 j = 0; j < 4; ++j)
                        if (unitTarget->HasAuraEffect(spells[j],0))
                            return;

                    // select spell
                    uint32 iTmpSpellId = spells[urand(0,3)];

                    // cast
                    unitTarget->CastSpell(unitTarget, iTmpSpellId, true);
                    return;
                }
                // Bending Shinbone
                case 8856:
                {
                    if (!itemTarget && m_caster->GetTypeId() != TYPEID_PLAYER)
                        return;

                    uint32 spell_id = 0;
                    switch(urand(1, 5))
                    {
                    case 1:  spell_id = 8854; break;
                    default: spell_id = 8855; break;
                    }

                    m_caster->CastSpell(m_caster,spell_id,true,NULL);
                    return;
                }
                // Brittle Armor - need remove one 24575 Brittle Armor aura
                case 24590:
                    unitTarget->RemoveAuraFromStack(24575);
                    return;
                // Mercurial Shield - need remove one 26464 Mercurial Shield aura
                case 26465:
                    unitTarget->RemoveAuraFromStack(26464);
                    return;
                // Shadow Flame (All script effects, not just end ones to prevent player from dodging the last triggered spell)
                case 22539:
                case 22972:
                case 22975:
                case 22976:
                case 22977:
                case 22978:
                case 22979:
                case 22980:
                case 22981:
                case 22982:
                case 22983:
                case 22984:
                case 22985:
                {
                    if (!unitTarget || !unitTarget->isAlive())
                        return;

                    // Onyxia Scale Cloak
                    if (unitTarget->HasAura(22683))
                        return;

                    // Shadow Flame
                    m_caster->CastSpell(unitTarget, 22682, true);
                    return;
                }
                // Piccolo of the Flaming Fire
                case 17512:
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                        return;
                    unitTarget->HandleEmoteCommand(EMOTE_STATE_DANCE);
                    return;
                }
                // Escape artist
                case 20589:
                    m_caster->RemoveMovementImpairingAuras();
                    return;
                // Decimate
                case 28374:
                case 54426:
                    if (unitTarget)
                    {
                        int32 damage = int32(unitTarget->GetHealth()) - int32(unitTarget->CountPctFromMaxHealth(5));
                        if (damage > 0)
                            m_caster->CastCustomSpell(28375, SPELLVALUE_BASE_POINT0, damage, unitTarget);
                    }
                    return;
                // Mirren's Drinking Hat
                case 29830:
                {
                    uint32 item = 0;
                    switch (urand(1, 6))
                    {
                        case 1:
                        case 2:
                        case 3:
                            item = 23584; break;            // Loch Modan Lager
                        case 4:
                        case 5:
                            item = 23585; break;            // Stouthammer Lite
                        case 6:
                            item = 23586; break;            // Aerie Peak Pale Ale
                    }
                    if (item)
                        DoCreateItem(effIndex,item);
                    break;
                }
                // Improved Sprint
                case 30918:
                {
                    // Removes snares and roots.
                    unitTarget->RemoveMovementImpairingAuras();
                    break;
                }
                // Plant Warmaul Ogre Banner
                case 32307:
                {
                    Player *p_caster = dynamic_cast<Player*>(m_caster);
                    if (!p_caster)
                        break;
                    p_caster->RewardPlayerAndGroupAtEvent(18388, unitTarget);
                    Creature *cTarget = dynamic_cast<Creature*>(unitTarget);
                    if (!cTarget)
                        break;
                    cTarget->setDeathState(CORPSE);
                    cTarget->RemoveCorpse();
                    break;
                }
                case 48025:                                     // Headless Horseman's Mount
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                        return;

                    // Prevent stacking of mounts and client crashes upon dismounting
                    unitTarget->RemoveAurasByType(SPELL_AURA_MOUNTED);

                    // Triggered spell id dependent on riding skill and zone
                    bool canFly = true;
                    uint32 v_map = GetVirtualMapForMapAndZone(unitTarget->GetMapId(), unitTarget->GetZoneId());

                    if (!unitTarget->ToPlayer()->IsKnowHowFlyIn(unitTarget->GetMapId(), unitTarget->GetZoneId()))
                        canFly = false;
                    
                    if(canFly && v_map == 0 && !unitTarget->ToPlayer()->HasSpell(90267))
                        canFly = false;

                    float x, y, z;
                    unitTarget->GetPosition(x, y, z);
                    uint32 areaFlag = unitTarget->GetBaseMap()->GetAreaFlag(x, y, z);
                    AreaTableEntry const *pArea = sAreaStore.LookupEntry(areaFlag);
                    if (!pArea || (canFly && (pArea && pArea->flags & AREA_FLAG_NO_FLY_ZONE)))
                        canFly = false;

                    switch(unitTarget->ToPlayer()->GetBaseSkillValue(SKILL_RIDING))
                    {
                    case 75: unitTarget->CastSpell(unitTarget, 51621, true); break;
                    case 150: unitTarget->CastSpell(unitTarget, 48024, true); break;
                    case 225:
                        {
                            if (canFly)
                                unitTarget->CastSpell(unitTarget, 51617, true);
                            else
                                unitTarget->CastSpell(unitTarget, 48024, true);
                        }break;
                    case 300:
                        {
                            if (canFly)
                                unitTarget->CastSpell(unitTarget, 48023, true);
                            else
                                unitTarget->CastSpell(unitTarget, 48024, true);
                        }break;
                    }
                    return;
                }
                case 47977:                                     // Magic Broom
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                        return;

                    // Prevent stacking of mounts and client crashes upon dismounting
                    unitTarget->RemoveAurasByType(SPELL_AURA_MOUNTED);

                    // Triggered spell id dependent on riding skill and zone
                    bool canFly = true;
                    uint32 v_map = GetVirtualMapForMapAndZone(unitTarget->GetMapId(), unitTarget->GetZoneId());
                    if (v_map != 530 && v_map != 571 && v_map != 0)
                        canFly = false;

                    if (!unitTarget->ToPlayer()->IsKnowHowFlyIn(unitTarget->GetMapId(), unitTarget->GetZoneId()))
                        canFly = false;
                    
                    if(canFly && v_map == 0 && !unitTarget->ToPlayer()->HasSpell(90267))
                        canFly = false;

                    float x, y, z;
                    unitTarget->GetPosition(x, y, z);
                    uint32 areaFlag = unitTarget->GetBaseMap()->GetAreaFlag(x, y, z);
                    AreaTableEntry const *pArea = sAreaStore.LookupEntry(areaFlag);
                    if (!pArea || (canFly && (pArea && pArea->flags & AREA_FLAG_NO_FLY_ZONE)))
                        canFly = false;

                    switch(unitTarget->ToPlayer()->GetBaseSkillValue(SKILL_RIDING))
                    {
                    case 75: unitTarget->CastSpell(unitTarget, 42680, true); break;
                    case 150: unitTarget->CastSpell(unitTarget, 42683, true); break;
                    case 225:
                        {
                            if (canFly)
                                unitTarget->CastSpell(unitTarget, 42667, true);
                            else
                                unitTarget->CastSpell(unitTarget, 42683, true);
                        }break;
                    case 300:
                        {
                            if (canFly)
                                unitTarget->CastSpell(unitTarget, 42668, true);
                            else
                                unitTarget->CastSpell(unitTarget, 42683, true);
                        }break;
                    }
                    return;
                }
                // Mug Transformation
                case 41931:
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                        return;

                    uint8 bag = 19;
                    uint8 slot = 0;
                    Item *item = NULL;

                    while (bag) // 256 = 0 due to var type
                    {
                        item = m_caster->ToPlayer()->GetItemByPos(bag, slot);
                        if (item && item->GetEntry() == 38587) break;
                        ++slot;
                        if (slot == 39)
                        {
                            slot = 0;
                            ++bag;
                        }
                    }
                    if (bag)
                    {
                        if (m_caster->ToPlayer()->GetItemByPos(bag,slot)->GetCount() == 1) m_caster->ToPlayer()->RemoveItem(bag,slot,true);
                        else m_caster->ToPlayer()->GetItemByPos(bag,slot)->SetCount(m_caster->ToPlayer()->GetItemByPos(bag,slot)->GetCount()-1);
                        // Spell 42518 (Braufest - Gratisprobe des Braufest herstellen)
                        m_caster->CastSpell(m_caster, 42518, true);
                        return;
                    }
                    break;
                }
                // Brutallus - Burn
                case 45141:
                case 45151:
                {
                    //Workaround for Range ... should be global for every ScriptEffect
                    float radius = GetEffectRadius(effIndex);
                    if (unitTarget && unitTarget->GetTypeId() == TYPEID_PLAYER && unitTarget->GetDistance(m_caster) >= radius && !unitTarget->HasAura(46394) && unitTarget != m_caster)
                        unitTarget->CastSpell(unitTarget, 46394, true);

                    break;
                }
                // Goblin Weather Machine
                case 46203:
                {
                    if (!unitTarget)
                        return;

                    uint32 spellId = 0;
                    switch(rand() % 4)
                    {
                        case 0: spellId = 46740; break;
                        case 1: spellId = 46739; break;
                        case 2: spellId = 46738; break;
                        case 3: spellId = 46736; break;
                    }
                    unitTarget->CastSpell(unitTarget, spellId, true);
                    break;
                }
                // 5,000 Gold
                case 46642:
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                        return;

                    unitTarget->ToPlayer()->ModifyMoney(5000 * GOLD);

                    break;
                }
                // Roll Dice - Decahedral Dwarven Dice
                case 47770:
                {
                    char buf[128];
                    const char *gender = "his";
                    if (m_caster->getGender() > 0)
                        gender = "her";
                    sprintf(buf, "%s rubs %s [Decahedral Dwarven Dice] between %s hands and rolls. One %u and one %u.", m_caster->GetName(), gender, gender, urand(1,10), urand(1,10));
                    m_caster->MonsterTextEmote(buf, 0);
                    break;
                }
                // Roll 'dem Bones - Worn Troll Dice
                case 47776:
                {
                    char buf[128];
                    const char *gender = "his";
                    if (m_caster->getGender() > 0)
                        gender = "her";
                    sprintf(buf, "%s causually tosses %s [Worn Troll Dice]. One %u and one %u.", m_caster->GetName(), gender, urand(1,6), urand(1,6));
                    m_caster->MonsterTextEmote(buf, 0);
                    break;
                }
                // Vigilance
                case 50725:
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                        return;

                    // Remove Taunt cooldown
                    unitTarget->ToPlayer()->RemoveSpellCooldown(355, true);

                    return;
                }
                // Death Knight Initiate Visual
                case 51519:
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                        return;

                    uint32 iTmpSpellId = 0;
                    switch (unitTarget->GetDisplayId())
                    {
                        case 25369: iTmpSpellId = 51552; break; // bloodelf female
                        case 25373: iTmpSpellId = 51551; break; // bloodelf male
                        case 25363: iTmpSpellId = 51542; break; // draenei female
                        case 25357: iTmpSpellId = 51541; break; // draenei male
                        case 25361: iTmpSpellId = 51537; break; // dwarf female
                        case 25356: iTmpSpellId = 51538; break; // dwarf male
                        case 25372: iTmpSpellId = 51550; break; // forsaken female
                        case 25367: iTmpSpellId = 51549; break; // forsaken male
                        case 25362: iTmpSpellId = 51540; break; // gnome female
                        case 25359: iTmpSpellId = 51539; break; // gnome male
                        case 25355: iTmpSpellId = 51534; break; // human female
                        case 25354: iTmpSpellId = 51520; break; // human male
                        case 25360: iTmpSpellId = 51536; break; // nightelf female
                        case 25358: iTmpSpellId = 51535; break; // nightelf male
                        case 25368: iTmpSpellId = 51544; break; // orc female
                        case 25364: iTmpSpellId = 51543; break; // orc male
                        case 25371: iTmpSpellId = 51548; break; // tauren female
                        case 25366: iTmpSpellId = 51547; break; // tauren male
                        case 25370: iTmpSpellId = 51545; break; // troll female
                        case 25365: iTmpSpellId = 51546; break; // troll male
                        default: return;
                    }

                    unitTarget->CastSpell(unitTarget, iTmpSpellId, true);
                    Creature* npc = unitTarget->ToCreature();
                    npc->LoadEquipment(npc->GetEquipmentId());
                    return;
                }
                // Emblazon Runeblade
                case 51770:
                {
                    if (!m_originalCaster)
                        return;

                    m_originalCaster->CastSpell(m_originalCaster, damage, false);
                    break;
                }
                // Deathbolt from Thalgran Blightbringer
                // reflected by Freya's Ward
                // Retribution by Sevenfold Retribution
                case 51854:
                {
                    if (!m_caster || !unitTarget)
                        return;
                    if (unitTarget->HasAura(51845))
                        unitTarget->CastSpell(m_caster, 51856, true);
                    else
                        m_caster->CastSpell(unitTarget, 51855, true);
                    break;
                }
                // Summon Ghouls On Scarlet Crusade
                case 51904:
                {
                    if (!m_targets.HasDst())
                        return;

                    float x, y, z;
                    float radius = GetEffectRadius(effIndex);
                    for (uint8 i = 0; i < 15; ++i)
                    {
                        m_caster->GetRandomPoint(m_targets.m_dstPos, radius, x, y, z);
                        m_caster->CastSpell(x, y, z, 54522, true);
                    }
                    break;
                }
                case 52173: // Coyote Spirit Despawn
                case 60243: // Blood Parrot Despawn
                    if (unitTarget->GetTypeId() == TYPEID_UNIT && unitTarget->ToCreature()->isSummon())
                        unitTarget->ToTempSummon()->UnSummon();
                    return;
                case 52479: // Gift of the Harvester
                    if (unitTarget && m_originalCaster)
                        m_originalCaster->CastSpell(unitTarget, urand(0, 1) ? damage : 52505, true);
                    return;
                // Death Gate
                case 52751:
                {
                    if (!unitTarget || unitTarget->getClass() != CLASS_DEATH_KNIGHT)
                        return;
                    // triggered spell is stored in m_spellInfo->EffectBasePoints[0]
                    unitTarget->CastSpell(unitTarget, damage, false);
                    break;
                }
                case 53110: // Devour Humanoid
                    if (unitTarget)
                        unitTarget->CastSpell(m_caster, damage, true);
                    return;
                // Winged Steed of the Ebon Blade
                case 54729:
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                        return;

                    // Prevent stacking of mounts and client crashes upon dismounting
                    unitTarget->RemoveAurasByType(SPELL_AURA_MOUNTED);

                    // Triggered spell id dependent on riding skill
                    if (uint16 skillval = unitTarget->ToPlayer()->GetSkillValue(SKILL_RIDING))
                    {
                        if (skillval >= 300)
                            unitTarget->CastSpell(unitTarget, 54727, true);
                        else
                            unitTarget->CastSpell(unitTarget, 54726, true);
                    }
                    return;
                }
                case 58418:                                 // Portal to Orgrimmar
                case 58420:                                 // Portal to Stormwind
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER || effIndex != 0)
                        return;

                    uint32 spellID = SpellMgr::CalculateSpellEffectAmount(m_spellInfo, 0);
                    uint32 questID = SpellMgr::CalculateSpellEffectAmount(m_spellInfo, 1);

                    if (unitTarget->ToPlayer()->GetQuestStatus(questID) == QUEST_STATUS_COMPLETE && !unitTarget->ToPlayer()->GetQuestRewardStatus (questID))
                        unitTarget->CastSpell(unitTarget, spellID, true);

                    return;
                }
                case 58941:                                 // Rock Shards
                    if (unitTarget && m_originalCaster)
                    {
                        for (uint32 i = 0; i < 3; ++i)
                        {
                            m_originalCaster->CastSpell(unitTarget, 58689, true);
                            m_originalCaster->CastSpell(unitTarget, 58692, true);
                        }
                        if (((InstanceMap*)m_originalCaster->GetMap())->GetDifficulty() == REGULAR_DIFFICULTY)
                        {
                            m_originalCaster->CastSpell(unitTarget, 58695, true);
                            m_originalCaster->CastSpell(unitTarget, 58696, true);
                        }
                        else
                        {
                            m_originalCaster->CastSpell(unitTarget, 60883, true);
                            m_originalCaster->CastSpell(unitTarget, 60884, true);
                        }
                    }
                    return;
                case 58983: // Big Blizzard Bear
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                        return;

                    // Prevent stacking of mounts and client crashes upon dismounting
                    unitTarget->RemoveAurasByType(SPELL_AURA_MOUNTED);

                    // Triggered spell id dependent on riding skill
                    if (uint16 skillval = unitTarget->ToPlayer()->GetSkillValue(SKILL_RIDING))
                    {
                        if (skillval >= 150)
                            unitTarget->CastSpell(unitTarget, 58999, true);
                        else
                            unitTarget->CastSpell(unitTarget, 58997, true);
                    }
                    return;
                }
                case 63845: // Create Lance
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                        return;

                    if (m_caster->ToPlayer()->GetTeam() == ALLIANCE)
                        m_caster->CastSpell(m_caster, 63914, true);
                    else
                        m_caster->CastSpell(m_caster, 63919, true);
                    return;
                }
                case 71342:                                     // Big Love Rocket
                    {
                        if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                            return;

                        // Prevent stacking of mounts and client crashes upon dismounting
                        unitTarget->RemoveAurasByType(SPELL_AURA_MOUNTED);

                        // Triggered spell id dependent on riding skill and zone
                        bool canFly = true;
                        uint32 v_map = GetVirtualMapForMapAndZone(unitTarget->GetMapId(), unitTarget->GetZoneId());
                        if (v_map != 530 && v_map != 571 && v_map != 0)
                            canFly = false;

                        if (canFly && v_map == 571 && !unitTarget->ToPlayer()->HasSpell(54197))
                            canFly = false;

                        if(canFly && v_map == 0 && !unitTarget->ToPlayer()->HasSpell(90267))
                            canFly = false;

                        float x, y, z;
                        unitTarget->GetPosition(x, y, z);
                        uint32 areaFlag = unitTarget->GetBaseMap()->GetAreaFlag(x, y, z);
                        AreaTableEntry const *pArea = sAreaStore.LookupEntry(areaFlag);
                        if (!pArea || (canFly && (pArea->flags & AREA_FLAG_NO_FLY_ZONE)))
                            canFly = false;

                        switch(unitTarget->ToPlayer()->GetBaseSkillValue(SKILL_RIDING))
                        {
                        case 0: unitTarget->CastSpell(unitTarget, 71343, true); break;
                        case 75: unitTarget->CastSpell(unitTarget, 71344, true); break;
                        case 150: unitTarget->CastSpell(unitTarget, 71345, true); break;
                        case 225:
                            {
                                if (canFly)
                                    unitTarget->CastSpell(unitTarget, 71346, true);
                                else
                                    unitTarget->CastSpell(unitTarget, 71345, true);
                            }break;
                        case 300:
                            {
                                if (canFly)
                                    unitTarget->CastSpell(unitTarget, 71347, true);
                                else
                                    unitTarget->CastSpell(unitTarget, 71345, true);
                            }break;
                        }
                        return;
                    }
                case 72286:                                     // Invincible
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                        return;

                    // Prevent stacking of mounts and client crashes upon dismounting
                    unitTarget->RemoveAurasByType(SPELL_AURA_MOUNTED);

                    // Triggered spell id dependent on riding skill and zone
                    bool canFly = true;
                    uint32 v_map = GetVirtualMapForMapAndZone(unitTarget->GetMapId(), unitTarget->GetZoneId());
                    if (v_map != 530 && v_map != 571 && v_map != 0)
                        canFly = false;

                    if (!unitTarget->ToPlayer()->IsKnowHowFlyIn(unitTarget->GetMapId(), unitTarget->GetZoneId()))
                        canFly = false;

                    float x, y, z;
                    unitTarget->GetPosition(x, y, z);
                    uint32 areaFlag = unitTarget->GetBaseMap()->GetAreaFlag(x, y, z);
                    AreaTableEntry const *pArea = sAreaStore.LookupEntry(areaFlag);
                    if (canFly && pArea->flags & AREA_FLAG_NO_FLY_ZONE)
                        canFly = false;

                    switch(unitTarget->ToPlayer()->GetBaseSkillValue(SKILL_RIDING))
                    {
                    case 0: unitTarget->CastSpell(unitTarget, 71343, true); break;
                    case 75: unitTarget->CastSpell(unitTarget, 71344, true); break;
                    case 150: unitTarget->CastSpell(unitTarget, 71345, true); break;
                    case 225:
                        {
                        if (canFly)
                                unitTarget->CastSpell(unitTarget, 71346, true);
                            else
                                unitTarget->CastSpell(unitTarget, 71345, true);
                        }break;
                    case 300:
                        {
                        if (canFly)
                            unitTarget->CastSpell(unitTarget, 71347, true);
                        else
                            unitTarget->CastSpell(unitTarget, 71345, true);
                        }break;
                    }
                    return;
                }
                case 74856:                                     // Blazing Hippogryph
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                        return;

                    // Prevent stacking of mounts and client crashes upon dismounting
                    unitTarget->RemoveAurasByType(SPELL_AURA_MOUNTED);

                    // Triggered spell id dependent on riding skill
                    if (uint16 skillval = unitTarget->ToPlayer()->GetSkillValue(SKILL_RIDING))
                    {
                        if (skillval >= 300)
                            unitTarget->CastSpell(unitTarget, 74855, true);
                        else
                            unitTarget->CastSpell(unitTarget, 74854, true);
                    }
                    return;
                }
                case 75614:                                     // Celestial Steed
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                        return;

                    // Prevent stacking of mounts and client crashes upon dismounting
                    unitTarget->RemoveAurasByType(SPELL_AURA_MOUNTED);

                    // Triggered spell id dependent on riding skill and zone
                    bool canFly = true;
                    uint32 v_map = GetVirtualMapForMapAndZone(unitTarget->GetMapId(), unitTarget->GetZoneId());
                    if (v_map != 530 && v_map != 571 && v_map != 0)
                        canFly = false;

                    if (!unitTarget->ToPlayer()->IsKnowHowFlyIn(unitTarget->GetMapId(), unitTarget->GetZoneId()))
                        canFly = false;

                    if(canFly && v_map == 0 && !unitTarget->ToPlayer()->HasSpell(90267))
                        canFly = false;

                    float x, y, z;
                    unitTarget->GetPosition(x, y, z);
                    uint32 areaFlag = unitTarget->GetBaseMap()->GetAreaFlag(x, y, z);
                    AreaTableEntry const *pArea = sAreaStore.LookupEntry(areaFlag);
                    if (!pArea || (canFly && (pArea && pArea->flags & AREA_FLAG_NO_FLY_ZONE)))
                        canFly = false;

                    switch(unitTarget->ToPlayer()->GetBaseSkillValue(SKILL_RIDING))
                    {
                    case 75: unitTarget->CastSpell(unitTarget, 75619, true); break;
                    case 150: unitTarget->CastSpell(unitTarget, 75620, true); break;
                    case 225:
                        {
                            if (canFly)
                                unitTarget->CastSpell(unitTarget, 75617, true);
                            else
                                unitTarget->CastSpell(unitTarget, 75620, true);
                        }break;
                    case 300:
                        {
                            if (canFly)
                            {
                                if (unitTarget->ToPlayer()->Has310Flyer(false))
                                    unitTarget->CastSpell(unitTarget, 76153, true);
                                else
                                    unitTarget->CastSpell(unitTarget, 75618, true);
                            }
                            else
                                unitTarget->CastSpell(unitTarget, 75620, true);
                        }break;
                    }
                    return;
                }
                case 75973:                                     // X-53 Touring Rocket
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                        return;

                    // Prevent stacking of mounts
                    unitTarget->RemoveAurasByType(SPELL_AURA_MOUNTED);

                    // Triggered spell id dependent on riding skill
                    if (uint16 skillval = unitTarget->ToPlayer()->GetSkillValue(SKILL_RIDING))
                    {
                        if (skillval >= 300)
                        {
                            if (unitTarget->ToPlayer()->Has310Flyer(false))
                                unitTarget->CastSpell(unitTarget, 76154, true);
                            else
                                unitTarget->CastSpell(unitTarget, 75972, true);
                        }
                        else
                            unitTarget->CastSpell(unitTarget, 75957, true);
                    }
                    return;
                }
                case 59317:                                 // Teleporting
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
                        return;

                    // return from top
                    if (unitTarget->ToPlayer()->GetAreaId() == 4637)
                        unitTarget->CastSpell(unitTarget, 59316, true);
                    // teleport atop
                    else
                        unitTarget->CastSpell(unitTarget, 59314, true);

                    return;
                // random spell learn instead placeholder
                case 60893:                                 // Northrend Alchemy Research
                case 61177:                                 // Northrend Inscription Research
                case 61288:                                 // Minor Inscription Research
                case 61756:                                 // Northrend Inscription Research (FAST QA VERSION)
                case 64323:                                 // Book of Glyph Mastery
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                        return;

                    // learn random explicit discovery recipe (if any)
                    if (uint32 discoveredSpell = GetExplicitDiscoverySpell(m_spellInfo->Id, (Player*)m_caster))
                        m_caster->ToPlayer()->learnSpell(discoveredSpell, false);
                    return;
                }
                case 62428: // Load into Catapult
                {
                    if (Vehicle *seat = m_caster->GetVehicleKit())
                        if (Unit *passenger = seat->GetPassenger(0))
                            if (Unit *demolisher = m_caster->GetVehicleBase())
                                passenger->CastSpell(demolisher, damage, true);
                    return;
                }
                case 62482: // Grab Crate
                {
                    if (unitTarget)
                    {
                        if (Vehicle *seat = m_caster->GetVehicleKit())
                        {
                            if (Creature *oldContainer = dynamic_cast<Creature*>(seat->GetPassenger(1)))
                                oldContainer->DisappearAndDie();
                            // TODO: a hack, range = 11, should after some time cast, otherwise too far
                            m_caster->CastSpell(seat->GetBase(), 62496, true);
                            unitTarget->EnterVehicle(seat, 1);
                        }
                    }
                    return;
                }
                case 60123: // Lightwell
                {
                    if (m_caster->GetTypeId() != TYPEID_UNIT || !m_caster->ToCreature()->isSummon())
                        return;

                    Aura * chargesaura = m_caster->GetAura(59907);

                    if (chargesaura && chargesaura->GetCharges() > 1)
                    {
                        chargesaura->SetCharges(chargesaura->GetCharges() - 1);
                        m_caster->CastSpell(unitTarget, 7001, true, NULL, NULL, m_caster->ToTempSummon()->GetSummonerGUID());
                    }
                    else if (chargesaura && chargesaura->GetCharges() == 1)
                    {
                        m_caster->CastSpell(unitTarget, 7001, true, NULL, NULL, m_caster->ToTempSummon()->GetSummonerGUID());
                        m_caster->ToTempSummon()->UnSummon();
                    }
                    else
                        m_caster->ToTempSummon()->UnSummon();
                    return;
                }
                // Stoneclaw Totem
                case 55328: // Rank 1
                case 55329: // Rank 2
                case 55330: // Rank 3
                case 55332: // Rank 4
                case 55333: // Rank 5
                case 55335: // Rank 6
                case 55278: // Rank 7
                case 58589: // Rank 8
                case 58590: // Rank 9
                case 58591: // Rank 10
                {
                    int32 basepoints0 = damage;
                    // Cast Absorb on totems
                    for (uint8 slot = SUMMON_SLOT_TOTEM; slot < MAX_TOTEM_SLOT; ++slot)
                    {
                        if (!unitTarget->m_SummonSlot[slot])
                            continue;

                        Creature* totem = unitTarget->GetMap()->GetCreature(unitTarget->m_SummonSlot[slot]);
                        if (totem && totem->isTotem())
                        {
                            m_caster->CastCustomSpell(totem, 55277, &basepoints0, NULL, NULL, true);
                        }
                    }
                    // Glyph of Stoneclaw Totem
                    if (AuraEffect *aur=unitTarget->GetAuraEffect(63298, 0))
                    {
                        basepoints0 *= aur->GetAmount();
                        m_caster->CastCustomSpell(unitTarget, 55277, &basepoints0, NULL, NULL, true);
                    }
                    break;
                }
                case 66545: //Summon Memory
                {
                    uint8 uiRandom = urand(0,25);
                    uint32 uiSpells[26] = {66704,66705,66706,66707,66709,66710,66711,66712,66713,66714,66715,66708,66708,66691,66692,66694,66695,66696,66697,66698,66699,66700,66701,66702,66703,66543};

                    m_caster->CastSpell(m_caster,uiSpells[uiRandom],true);
                    break;
                }
                case 45668:                                 // Ultra-Advanced Proto-Typical Shortening Blaster
                {
                    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT)
                        return;

                    if (roll_chance_i(50))                  // chance unknown, using 50
                        return;

                    static uint32 const spellPlayer[5] =
                    {
                        45674,                            // Bigger!
                        45675,                            // Shrunk
                        45678,                            // Yellow
                        45682,                            // Ghost
                        45684                             // Polymorph
                    };

                    static uint32 const spellTarget[5] = {
                        45673,                            // Bigger!
                        45672,                            // Shrunk
                        45677,                            // Yellow
                        45681,                            // Ghost
                        45683                             // Polymorph
                    };

                    m_caster->CastSpell(m_caster, spellPlayer[urand(0,4)], true);
                    unitTarget->CastSpell(unitTarget, spellTarget[urand(0,4)], true);
                    break;
                }
                case 64142:                                 // Upper Deck - Create Foam Sword
                {
                    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
                        return;
                    Player *plr = unitTarget->ToPlayer();
                    static uint32 const itemId[] = {45061, 45176, 45177, 45178, 45179, 0};
                    // player can only have one of these items
                    for (uint32 const *itr = &itemId[0]; *itr; ++itr)
                        if (plr->HasItemCount(*itr, 1, true))
                            return;
                    DoCreateItem(effIndex, itemId[urand(0,4)]);
                    return;
                }
                case 14181:                                 // Relentless Strikes
                {
                    if (effIndex == 0)
                    {
                        unitTarget->EnergizeBySpell(unitTarget, m_spellInfo->Id, damage, POWER_ENERGY);
                        return;
                    }
                    break;
                }
                case 99304:                                 // Consume (Beth'tilac)
                {
                    if (m_caster->IsVehicle() && unitTarget)
                        unitTarget->CastSpell(m_caster, damage, false);  // Ride Vehicle
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            break;
        }
        case SPELLFAMILY_WARLOCK:
        {
            if (m_spellInfo->Id == 6201) // Create Healthstone
                m_caster->CastSpell(m_caster, 34130, true);
            else if (m_spellInfo->Id == 77801) // Demon Soul
            {
                Unit* pPet = Unit::GetUnit(*m_caster,m_caster->GetPetGUID());
                if (!pPet)
                    return;

                uint32 ChosenSpell = 0;

                if (pPet->GetEntry() == 416)        // Imp
                    ChosenSpell = 79459;
                else if (pPet->GetEntry() == 1860)  // Voidwalker
                    ChosenSpell = 79464;
                else if (pPet->GetEntry() == 1863)  // Succubus
                    ChosenSpell = 79463;
                else if (pPet->GetEntry() == 417)   // Felhunter
                    ChosenSpell = 79460;
                else if (pPet->GetEntry() == 17252) // Felguard
                    ChosenSpell = 79462;

                if (ChosenSpell)
                    m_caster->CastSpell(m_caster, ChosenSpell, true);
            }
            // Cremation (both ranks, triggered spell)
            else if (m_spellInfo->Id == 89603)
            {
                // chance calculated in spell effect (this spell is triggered with this chance)
                Aura* pImmolate = unitTarget ? unitTarget->GetAura(348, m_originalCasterGUID) : NULL;

                if (pImmolate)
                    pImmolate->RefreshDuration();
            }
            // Fel Flame (increase duration part)
            else if (m_spellInfo->Id == 77799)
            {
                Aura* pImmolate = unitTarget ? unitTarget->GetAura(348, m_originalCasterGUID) : NULL;
                Aura* pUnstableAffliction = unitTarget ? unitTarget->GetAura(30108, m_originalCasterGUID) : NULL;
                if (pUnstableAffliction)
                {
                    if (pUnstableAffliction->GetDuration()+6000 > pUnstableAffliction->GetMaxDuration())
                        pUnstableAffliction->RefreshDuration();
                    else
                        pUnstableAffliction->SetDuration(pUnstableAffliction->GetDuration()+6000);
                }
                if (pImmolate)
                {
                    if (pImmolate->GetDuration()+6000 > pImmolate->GetMaxDuration())
                        pImmolate->RefreshDuration();
                    else
                        pImmolate->SetDuration(pImmolate->GetDuration()+6000);
                }
            }
            return;
        }
        case SPELLFAMILY_HUNTER:
        {
            // cobra shot focus effect + add 6 seconds to serpent sting
            if (m_spellInfo->SpellFamilyFlags[2] & 0x400000)
            {
                SpellEntry const* energizeSpell = sSpellStore.LookupEntry(91954);
                if (energizeSpell)
                {
                    int32 bp0 = energizeSpell->EffectBasePoints[0];
                    if (unitTarget && unitTarget->GetHealthPct() <= 25.0f)
                    {
                        // Termination
                        if (m_caster->HasAura(83490))
                            bp0 += 6;
                        else if (m_caster->HasAura(83489))
                            bp0 += 3;
                    }
                    m_caster->CastCustomSpell(m_caster,91954,&bp0,0,0,true);
                }

                if (unitTarget->GetAura(1978, m_originalCasterGUID))
                    unitTarget->GetAura(1978, m_originalCasterGUID)->SetDuration((unitTarget->GetAura(1978, m_originalCasterGUID)->GetDuration() + 6000), true);
            }
            // chimera shot health effect + serpent sting refresh
            if (m_spellInfo->SpellFamilyFlags[2] & 0x1)
            {
                m_caster->CastSpell(m_caster,53353,true);
                if (unitTarget->GetAura(1978, m_originalCasterGUID))
                    unitTarget->GetAura(1978, m_originalCasterGUID)->RefreshDuration();
            }
            // Kill Command (caster fix)
            if (m_spellInfo->Id == 34026)
            {
                if (m_caster->GetPetGUID())
                {
                    Pet* pPet = (Pet*)Unit::GetUnit(*m_caster,m_caster->GetPetGUID());
                    if (pPet && pPet->getVictim())
                    {
                        Unit* victim = pPet->getVictim();
                        pPet->CastSpell(victim, 83381, true);

                        // Resistance is Futile refunds 100% of Kill Command focus cost
                        if (m_caster->HasAura(82897))
                        {
                            m_caster->RemoveAurasDueToSpell(82897);
                            m_caster->CastSpell(m_caster, 86316, true);
                        }
                    }
                }
            }
            return;
        }
        case SPELLFAMILY_POTION:
        {
            switch(m_spellInfo->Id)
            {
                // Netherbloom
                case 28702:
                {
                    if (!unitTarget)
                        return;
                    // 25% chance of casting a random buff
                    if (roll_chance_i(75))
                        return;

                    // triggered spells are 28703 to 28707
                    // Note: some sources say, that there was the possibility of
                    //       receiving a debuff. However, this seems to be removed by a patch.
                    const uint32 spellid = 28703;

                    // don't overwrite an existing aura
                    for (uint8 i = 0; i < 5; ++i)
                        if (unitTarget->HasAura(spellid + i))
                            return;
                    unitTarget->CastSpell(unitTarget, spellid+urand(0, 4), true);
                    break;
                }

                // Nightmare Vine
                case 28720:
                {
                    if (!unitTarget)
                        return;
                    // 25% chance of casting Nightmare Pollen
                    if (roll_chance_i(75))
                        return;
                    unitTarget->CastSpell(unitTarget, 28721, true);
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            // Pestilence
            if (m_spellInfo->SpellFamilyFlags[1]&0x10000)
            {
                // Get diseases on target of spell
                if (m_targets.getUnitTarget() != unitTarget)
                {
                    float dmgDecreased = 2.0f ;

                    if (m_caster->HasAura(91316) ) // Contagion (Rank 1)
                        dmgDecreased = 1.333f;

                    if (m_caster->HasAura(91319) ) // Contagion (Rank 2)
                        dmgDecreased = 1.0f;

                    m_caster->CastSpell(unitTarget, 91939, true); // Visual effect

                    // And spread them on target
                    // Blood Plague
                    if (m_targets.getUnitTarget()->GetAura(55078))
                    {
                        m_caster->CastSpell(unitTarget, 55078, true);

                        if (AuraEffect* aurEff = unitTarget->GetAuraEffect(55078,0,m_caster->GetGUID()) )
                            if ( aurEff->GetAmount() / dmgDecreased > 0)
                                aurEff->SetAmount(aurEff->GetAmount() / dmgDecreased);
                    }
                    // Frost Fever
                    if (m_targets.getUnitTarget()->GetAura(55095))
                    {
                        m_caster->CastSpell(unitTarget, 55095, true);
                        if (AuraEffect* aurEff = unitTarget->GetAuraEffect(55095,0,m_caster->GetGUID()))
                            if ( aurEff->GetAmount() / dmgDecreased > 0)
                                aurEff->SetAmount(aurEff->GetAmount() / dmgDecreased);
                    }
                    // Ebon Plague
                    if (m_targets.getUnitTarget()->GetAura(65142))
                    {
                        int32 bp0 = (m_caster->HasAura(51099)) ? 15 : 30;
                        m_caster->CastCustomSpell(unitTarget, 65142, &bp0, NULL, NULL, true);
                    }
                }
            }
            // Festering Strike
            else if (m_spellInfo->Id == 85948)
            {
                // increase duration of certain auras
                uint32 affectAuras[] = {65142, 55078, 55095, 45524};
                Aura* tmp = NULL;
                for (uint8 i = 0; i < sizeof(affectAuras)/sizeof(uint32); i++)
                {
                    tmp = unitTarget->GetAura(affectAuras[i], m_caster->GetGUID());
                    if (tmp)
                    {
                        int32 newduration = tmp->GetDuration() + m_spellInfo->EffectBasePoints[effIndex] * 1000;
                        tmp->SetDuration(newduration);
                        if (newduration > tmp->GetMaxDuration())
                            tmp->SetMaxDuration(newduration);
                    }
                }
            }
            // Acherus Deathcharger
            else if (m_spellInfo->Id == 48778)
            {
                m_caster->CastSpell(m_caster, 50772, true); // Summon Unholy Mount Visual
            }
            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            // Earthquake
            if (m_spellInfo->Id == 77478)
            {
                // 10% chance to stun (knockdown) enemies
                if (roll_chance_i(10))
                    m_caster->CastSpell(unitTarget, 77505, true);
            }
            // Improved Lava Lash
            else if (m_spellInfo->Id == 105792)
            {
                // Instantly spread flame shock to up to 4 nearby enemies
                if (m_caster && unitTarget)
                    m_caster->AddAura(8050, unitTarget);
            }
            else if (m_spellInfo->Id == 99202) // Taming the Flames ( T12 4P set bonus )
            {
                if (m_caster->ToPlayer())
                    m_caster->ToPlayer()->ModifySpellCooldown(2894, -4000, true); // Fire Elemental Totem
            }
            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            // Shattering Throw
            if (m_spellInfo->SpellFamilyFlags[1] & 0x00400000)
            {
                if (!unitTarget)
                    return;
                // remove shields, will still display immune to damage part
                unitTarget->RemoveAurasWithMechanic(1<<MECHANIC_IMMUNE_SHIELD, AURA_REMOVE_BY_ENEMY_SPELL);
                return;
            }
            break;
        }
        case SPELLFAMILY_MAGE:
        {
            // Mobile Banking - guild perk
            // Why the hell does it have SPELLFAMILY_MAGE family name?!
            if (m_spellInfo->Id == 83958)
            {
                // Allow only if player has friendly or better reputation rank
                if (m_caster && m_caster->ToPlayer() && m_caster->ToPlayer()->GetReputationRank(FACTION_GUILD) >= REP_FRIENDLY)
                {
                    // Cast summoning spell
                    if (m_caster->ToPlayer()->GetTeamId() == TEAM_ALLIANCE)
                        m_caster->CastSpell(m_caster, 88304, true);
                    else
                        m_caster->CastSpell(m_caster, 88306, true);
                }
            }

            // Combustion
            if (m_spellInfo->Id == 11129)
            {
                if (!m_caster || !unitTarget)
                    return;

                int32 dot_sum = 0;

                uint64 caster_guid = m_caster->GetGUID();
                Unit::AuraEffectList const &dot_list = unitTarget->GetAuraEffectsByType(SPELL_AURA_PERIODIC_DAMAGE);

                if (dot_list.empty())
                    return;

                // sumarize all the mages fire DoTs applied on the target
                for (Unit::AuraEffectList::const_iterator itr = dot_list.begin(); itr != dot_list.end(); ++itr)
                    if(AuraEffect const *dot_eff = *itr)
                    {
                        if (dot_eff->GetCasterGUID() != caster_guid)
                            continue;
                        SpellEntry const *dot_spell = dot_eff->GetSpellProto();
                        if (!dot_spell)
                            continue;
                        uint32 dot_schoolmask = dot_spell->SchoolMask;
                        if (!(dot_schoolmask &= SPELL_SCHOOL_MASK_FIRE))
                            continue;

                        uint32 dotEffIndex = dot_eff->GetEffIndex();
                        int32 dot_tick = m_caster->SpellDamageBonus(unitTarget, dot_spell, dotEffIndex, dot_eff->GetAmount(), DOT, dot_eff->GetBase()->GetStackAmount());
                        uint32 dot_amplitude = dot_spell->EffectAmplitude[dotEffIndex];

                        if (dot_amplitude > 0 && dot_tick > 0)
                            dot_sum += dot_tick * IN_MILLISECONDS / dot_amplitude;    // base damage per second (without haste)
                    }

                // apply a new DoT. amount is equal to the sum above
                if (dot_sum > 0)
                    m_caster->CastCustomSpell(unitTarget, 83853, &dot_sum, NULL, NULL, true);
            }
        }
        case SPELLFAMILY_PRIEST:
        {
            // Archangel
            if (m_spellInfo->Id == 87151)
            {
                if (!m_caster)
                    return;

                // Evangelism / Archangel
                if (Aura* aura = m_caster->GetAura(81661))
                {
                    int8 charges = aura->GetCharges();

                    // 3% healing bonus for each stack
                    int32 basepoints0_a = charges * 3;
                    m_caster->CastCustomSpell(m_caster, 81700, &basepoints0_a, NULL, NULL, true);

                    // Restore 1% of mana for each stack
                    int32 basepoints0_b = charges * 1;
                    m_caster->CastCustomSpell(m_caster, 87152, &basepoints0_b, NULL, NULL, true);

                    // Consume aura
                    m_caster->RemoveAurasDueToSpell(81661);
                    // Add cooldown
                    m_caster->ToPlayer()->AddSpellCooldown(87151, 0, 30000);
                }
                // Dark Evangelism / Dark Archangel
                else if (Aura* aura = m_caster->GetAura(87118))
                {
                    int8 charges = aura->GetCharges();

                    // 4% damage bonus for each stack for specific spells
                    int32 basepoints0_a = charges * 4;
                    int32 basepoints1_a = charges * 4;
                    m_caster->CastCustomSpell(m_caster, 87153, &basepoints0_a, &basepoints1_a, NULL, true);

                    // Restore mana manually (eff #3 unknown effect) --workaround: using Archangel
                    // Restore 5% of mana for each stack
                    int32 basepoints0_b = charges * 5;
                    m_caster->CastCustomSpell(m_caster, 87152, &basepoints0_b, NULL, NULL, true);

                    // Consume aura
                    m_caster->RemoveAurasDueToSpell(87118);
                    // Add cooldown
                    m_caster->ToPlayer()->AddSpellCooldown(87151, 0, 90000);
                }

                // Disable spell
                m_caster->RemoveAurasDueToSpell(87154);
                m_caster->RemoveAurasDueToSpell(94709);
            }

            // Chakra: Serenity - Refresh Renew
            if (m_spellInfo->Id == 81585)
            {
                if (!unitTarget)
                    return;

                // Direct heals condition handled implicitely

                if (Aura* renew = unitTarget->GetAura(139,m_caster->GetGUID()))
                    renew->RefreshDuration();
            }

            // Strength of Soul
            if (m_spellInfo->Id == 89490)
            {
                if (!unitTarget)
                    return;

                if (Aura* debuff = unitTarget->GetAura(6788)) // Decrease duration of Weakened Soul
                {                                                                   // Rank 2   Rank 1
                    int32 duration = debuff->GetDuration() - (m_caster->HasAura(89489) ? 4000 : 2000);
                    if (duration > 0)
                        debuff->SetDuration(duration);
                    else
                        debuff->Remove(AURA_REMOVE_BY_EXPIRE);
                }
            }
        }
    }

    // normal DB scripted effect
    sLog->outDebug("Spell ScriptStart spellid %u in EffectScriptEffect(%u)", m_spellInfo->Id, effIndex);
    m_caster->GetMap()->ScriptsStart(sSpellScripts, uint32(m_spellInfo->Id | (effIndex << 24)), m_caster, unitTarget);
}

void Spell::EffectSanctuary(SpellEffIndex /*effIndex*/)
{
    if (!unitTarget)
        return;

    std::list<Unit*> targets;
    Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(unitTarget, unitTarget, m_caster->GetMap()->GetVisibilityDistance());
    Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(unitTarget, targets, u_check);
    unitTarget->VisitNearbyObject(m_caster->GetMap()->GetVisibilityDistance(), searcher);
    for (std::list<Unit*>::iterator iter = targets.begin(); iter != targets.end(); ++iter)
    {
        if (!(*iter)->hasUnitState(UNIT_STAT_CASTING))
            continue;

        for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_MAX_SPELL; i++)
        {
            if ((*iter)->GetCurrentSpell(i)
            && (*iter)->GetCurrentSpell(i)->m_targets.getUnitTargetGUID() == unitTarget->GetGUID())
            {
                (*iter)->InterruptSpell(CurrentSpellTypes(i), false);
            }
        }
    }

    unitTarget->CombatStop();
    unitTarget->getHostileRefManager().deleteReferences();   // stop all fighting
    // Vanish allows to remove all threat and cast regular stealth so other spells can be used
    if (m_caster->GetTypeId() == TYPEID_PLAYER
        && m_spellInfo->SpellFamilyName == SPELLFAMILY_ROGUE
        && (m_spellInfo->SpellFamilyFlags[0] & SPELLFAMILYFLAG_ROGUE_VANISH))
    {
        m_caster->ToPlayer()->RemoveAurasByType(SPELL_AURA_MOD_ROOT);
        // Overkill
        if (m_caster->ToPlayer()->HasSpell(58426))
           m_caster->CastSpell(m_caster, 58427, true);
    }
}

void Spell::EffectAddComboPoints(SpellEffIndex /*effIndex*/)
{
    if (!unitTarget)
        return;

    if (!m_caster->m_movedPlayer)
        return;

    Player* plr = m_caster->m_movedPlayer;

    if (plr == NULL)
        return;

    if (damage > 0)
        plr->AddComboPoints(unitTarget, damage, this);
    else
    {
        // Rogue: Redirect
        if (GetSpellInfo()->Id == 73981 && plr->GetComboPoints() > 0)
        {
            Unit * comboTarget = ObjectAccessor::GetUnit(*plr, plr->GetComboTarget());
            if (comboTarget == NULL)
                return;

            Unit::AuraApplicationMap const& auras = comboTarget->GetAppliedAuras();
            for (Unit::AuraApplicationMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            {
                Aura * aura = itr->second->GetBase();
                uint32 auraId = aura->GetSpellProto()->Id;

                switch (auraId)
                {
                    // Transfers any existing combo points to the current enemy target, as well as any insight gained from Bandit's Guile
                    case 84745:
                    case 84746:
                    case 84747:
                    {
                        if (itr->second->GetBase()->GetCasterGUID() == plr->GetGUID())
                        {
                            if (Aura* insightCopy = plr->AddAura(auraId, unitTarget))
                                insightCopy->SetDuration(aura->GetDuration());
                        }
                        break;
                    }
                    default:
                        break;
                }
            }

            uint8 combopoints = plr->GetComboPoints();
            plr->ClearComboPoints();
            plr->AddComboPoints(unitTarget, combopoints, this);
            // Remove old rogue's insights
            comboTarget->RemoveAura(84745, plr->GetGUID());
            comboTarget->RemoveAura(84746, plr->GetGUID());
            comboTarget->RemoveAura(84747, plr->GetGUID());
        }
    }
}

void Spell::EffectDuel(SpellEffIndex effIndex)
{
    if (!m_caster || !unitTarget || m_caster->GetTypeId() != TYPEID_PLAYER || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    Player *caster = (Player*)m_caster;
    Player *target = (Player*)unitTarget;

    // caster or target already have requested duel
    if (caster->duel || target->duel || !target->GetSocial() || target->GetSocial()->HasIgnore(caster->GetGUIDLow()))
        return;

    // Players can only fight a duel with each other outside (=not inside dungeons and not in capital cities)
    // Don't have to check the target's map since you cannot challenge someone across maps
    if (caster->GetMap()->Instanceable())
    //if (mapid != 0 && mapid != 1 && mapid != 530 && mapid != 571 && mapid != 609)
    {
        SendCastResult(SPELL_FAILED_NO_DUELING);            // Dueling isn't allowed here
        return;
    }

    AreaTableEntry const* casterAreaEntry = GetAreaEntryByAreaID(caster->GetZoneId());
    if (casterAreaEntry && (casterAreaEntry->flags & AREA_FLAG_CAPITAL))
    {
        SendCastResult(SPELL_FAILED_NO_DUELING);            // Dueling isn't allowed here
        return;
    }

    AreaTableEntry const* targetAreaEntry = GetAreaEntryByAreaID(target->GetZoneId());
    if (targetAreaEntry && (targetAreaEntry->flags & AREA_FLAG_CAPITAL))
    {
        SendCastResult(SPELL_FAILED_NO_DUELING);            // Dueling isn't allowed here
        return;
    }

    //CREATE DUEL FLAG OBJECT
    GameObject* pGameObj = new GameObject;

    uint32 gameobject_id = m_spellInfo->EffectMiscValue[effIndex];

    Map *map = m_caster->GetMap();
    if (!pGameObj->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_GAMEOBJECT), gameobject_id,
        map, m_caster->GetPhaseMask(),
        m_caster->GetPositionX()+(unitTarget->GetPositionX()-m_caster->GetPositionX())/2 ,
        m_caster->GetPositionY()+(unitTarget->GetPositionY()-m_caster->GetPositionY())/2 ,
        m_caster->GetPositionZ(),
        m_caster->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 0, GO_STATE_READY))
    {
        delete pGameObj;
        return;
    }

    pGameObj->SetUInt32Value(GAMEOBJECT_FACTION, m_caster->getFaction());
    pGameObj->SetUInt32Value(GAMEOBJECT_LEVEL, m_caster->getLevel()+1);
    int32 duration = GetSpellDuration(m_spellInfo);
    pGameObj->SetRespawnTime(duration > 0 ? duration/IN_MILLISECONDS : 0);
    pGameObj->SetSpellId(m_spellInfo->Id);

    ExecuteLogEffectSummonObject(effIndex, pGameObj);

    m_caster->AddGameObject(pGameObj);
    map->Add(pGameObj);
    //END

    // Send request
    WorldPacket data(SMSG_DUEL_REQUESTED, 8 + 8, true);
    data << uint64(pGameObj->GetGUID());
    data << uint64(caster->GetGUID());
    caster->GetSession()->SendPacket(&data);
    target->GetSession()->SendPacket(&data);

    // create duel-info
    DuelInfo *duel   = new DuelInfo;
    duel->initiator  = caster;
    duel->opponent   = target;
    duel->startTime  = 0;
    duel->startTimer = 0;
    caster->duel     = duel;

    DuelInfo *duel2   = new DuelInfo;
    duel2->initiator  = caster;
    duel2->opponent   = caster;
    duel2->startTime  = 0;
    duel2->startTimer = 0;
    target->duel      = duel2;

    caster->SetUInt64Value(PLAYER_DUEL_ARBITER, pGameObj->GetGUID());
    target->SetUInt64Value(PLAYER_DUEL_ARBITER, pGameObj->GetGUID());

    sScriptMgr->OnPlayerDuelRequest(target, caster);
}

void Spell::EffectStuck(SpellEffIndex /*effIndex*/)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    if (!sWorld->getBoolConfig(CONFIG_CAST_UNSTUCK))
        return;

    Player* pTarget = (Player*)unitTarget;

    sLog->outDebug("Spell Effect: Stuck");
    sLog->outDetail("Player %s (guid %u) used auto-unstuck future at map %u (%f, %f, %f)", pTarget->GetName(), pTarget->GetGUIDLow(), m_caster->GetMapId(), m_caster->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ());

    if (pTarget->isInFlight())
        return;

    /* share cooldown with hearthstone and astral recall */
    if (pTarget->HasSpellCooldown(8690) && (!pTarget->HasSpell(556) || pTarget->HasSpellCooldown(556))) {
        SendCastResult(SPELL_FAILED_NOT_READY);
        return;
    }

    /* which spells to preserve through spell reset */
    uint32 preserve_spells_table[] = {
        15007,  // Ressurection Sickness
        26013,  // Deserter
    };
    std::vector<uint32> preserve_spells;

    for (uint32 i = 0; i < sizeof(preserve_spells_table)/sizeof(uint32); i++)
        if (pTarget->HasAura(preserve_spells_table[i]))
            preserve_spells.push_back(preserve_spells_table[i]);

    pTarget->SetShapeshiftForm(FORM_NONE);
    pTarget->RemoveAllAuras();

    if (!pTarget->isAlive()) {
        pTarget->ResurrectPlayer(0.1f);
        pTarget->SpawnCorpseBones();
    }

    while (!preserve_spells.empty()) {
        pTarget->CastSpell(pTarget, preserve_spells.back(), true);
        preserve_spells.pop_back();
    }

    /* disabled, causes server crash, probably due to unavailability of m_mapId
     * when the save occurs (and player is already log out) */
#if 0
    /* save the player, update new position
     * and force logout without saving current position
     * NOTE: sMapMgr->GetZoneId() has no "orientation", so we can't use WorldLocation */
    WorldLocation loc = pTarget->GetStartPosition();
    pTarget->SaveToDB();
    pTarget->SavePositionInDB(loc.m_mapId, loc.m_positionX, loc.m_positionY, loc.m_positionZ, loc.m_orientation, sMapMgr->GetZoneId(loc.m_mapId, loc.m_positionX, loc.m_positionY, loc.m_positionZ), pTarget->GetGUID());
    pTarget->GetSession()->LogoutPlayer(false);
#endif

    /* trigger 30min hearthstone cooldown, 15min astral recall */
    pTarget->AddSpellCooldown(8690, 0, 1800*1000);
    if (pTarget->HasSpell(556))
        pTarget->AddSpellCooldown(556, 0, 900*1000);

    /* doesn't work (??) */
    //WorldPacket data(SMSG_COOLDOWN_EVENT, 4 + 8);
    //data << uint32(8690);
    //data << uint64(pTarget->GetGUID());
    //pTarget->GetSession()->SendPacket(&data);

    pTarget->TeleportTo(pTarget->GetStartPosition(), unitTarget == m_caster ? TELE_TO_SPELL : 0);

    ChatHandler(pTarget->GetSession()).PSendSysMessage("Vsechny aury resetovany, pro obnoveni pasivnich abilit proved relog (napis /logout).\n");
    ChatHandler(pTarget->GetSession()).PSendSysMessage("All auras have been re-set, please relog the character (using /logout) to restore passive abilities.\n");
}

void Spell::EffectSummonPlayer(SpellEffIndex /*effIndex*/)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    // Evil Twin (ignore player summon, but hide this for summoner)
    if (unitTarget->HasAura(23445))
        return;

    float x, y, z;
    m_caster->GetClosePoint(x, y, z, unitTarget->GetObjectSize());

    unitTarget->ToPlayer()->SetSummonPoint(m_caster->GetMapId(),x,y,z);

    WorldPacket data(SMSG_SUMMON_REQUEST, 8+4+4);
    data << uint64(m_caster->GetGUID());                    // summoner guid
    data << uint32(m_caster->GetZoneId());                  // summoner zone
    data << uint32(MAX_PLAYER_SUMMON_DELAY*IN_MILLISECONDS); // auto decline after msecs
    unitTarget->ToPlayer()->GetSession()->SendPacket(&data);
}

static ScriptInfo generateActivateCommand()
{
    ScriptInfo si;
    si.command = SCRIPT_COMMAND_ACTIVATE_OBJECT;
    return si;
}

void Spell::EffectActivateObject(SpellEffIndex effIndex)
{
    if (!gameObjTarget)
        return;

    static ScriptInfo activateCommand = generateActivateCommand();

    int32 delay_secs = m_spellInfo->EffectMiscValue[effIndex];

    gameObjTarget->GetMap()->ScriptCommandStart(activateCommand, delay_secs, m_caster, gameObjTarget);
}

void Spell::EffectApplyGlyph(SpellEffIndex effIndex)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER || m_glyphIndex >= MAX_GLYPH_SLOT_INDEX)
        return;

    Player *player = (Player*)m_caster;

    // glyph sockets level requirement
    if (!(player->GetUInt32Value(PLAYER_GLYPHS_ENABLED) & (1 << m_glyphIndex)))
    {
        SendCastResult(SPELL_FAILED_GLYPH_SOCKET_LOCKED);
        return;
    }

    // apply new one
    if (uint32 glyph = m_spellInfo->EffectMiscValue[effIndex])
    {
        if (GlyphPropertiesEntry const *gp = sGlyphPropertiesStore.LookupEntry(glyph))
        {
            if (GlyphSlotEntry const *gs = sGlyphSlotStore.LookupEntry(player->GetGlyphSlot(m_glyphIndex)))
            {
                if (gp->TypeFlags != gs->TypeFlags)
                {
                    SendCastResult(SPELL_FAILED_INVALID_GLYPH);
                    return;                                 // glyph slot mismatch
                }
            }

            // remove old glyph
            if (uint32 oldglyph = player->GetGlyph(m_glyphIndex))
            {
                if (GlyphPropertiesEntry const *old_gp = sGlyphPropertiesStore.LookupEntry(oldglyph))
                {
                    player->RemoveAurasDueToSpell(old_gp->SpellId);
                    player->SetGlyph(m_glyphIndex, 0);
                }
            }

            player->SetGlyph(m_glyphIndex, glyph);
            player->SendTalentsInfoData(false);
            player->learnSpell(gp->SpellId, true);
        }
    }
    else
    {
        // Glyph removal
        if (uint32 oldglyph = player->GetGlyph(m_glyphIndex))
        {
            if (GlyphPropertiesEntry const *old_gp = sGlyphPropertiesStore.LookupEntry(oldglyph))
            {
                player->RemoveAurasDueToSpell(old_gp->SpellId);
                player->SetGlyph(m_glyphIndex, 0);
            }
        }
    }
}

void Spell::EffectEnchantHeldItem(SpellEffIndex effIndex)
{
    // this is only item spell effect applied to main-hand weapon of target player (players in area)
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* item_owner = (Player*)unitTarget;
    Item* item = item_owner->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);

    if (!item)
        return;

    // must be equipped
    if (!item ->IsEquipped())
        return;

    if (m_spellInfo->EffectMiscValue[effIndex])
    {
        uint32 enchant_id = m_spellInfo->EffectMiscValue[effIndex];
        int32 duration = GetSpellDuration(m_spellInfo);          //Try duration index first ..
        if (!duration)
            duration = damage;//+1;            //Base points after ..
        if (!duration)
            duration = 10;                                  //10 seconds for enchants which don't have listed duration

        SpellItemEnchantmentEntry const *pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if (!pEnchant)
            return;

        // Always go to temp enchantment slot
        EnchantmentSlot slot = TEMP_ENCHANTMENT_SLOT;

        // Enchantment will not be applied if a different one already exists
        if (item->GetEnchantmentId(slot) && item->GetEnchantmentId(slot) != enchant_id)
            return;

        // Apply the temporary enchantment
        item->SetEnchantment(slot, enchant_id, duration*IN_MILLISECONDS, 0);
        item_owner->ApplyEnchantment(item, slot, true);
    }
}

void Spell::EffectDisEnchant(SpellEffIndex /*effIndex*/)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* p_caster = (Player*)m_caster;
    if (!itemTarget || !itemTarget->GetProto()->DisenchantID)
        return;

    p_caster->UpdateCraftSkill(m_spellInfo->Id);

    m_caster->ToPlayer()->SendLoot(itemTarget->GetGUID(),LOOT_DISENCHANTING);

    // item will be removed at disenchanting end
}

void Spell::EffectInebriate(SpellEffIndex /*effIndex*/)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    Player *player = (Player*)unitTarget;
    uint16 currentDrunk = player->GetDrunkValue();

    // Drunken Vomit
    if (m_CastItem && currentDrunk >= 25000 && damage > 0)
    {
        if (roll_chance_i(damage))
        {
            SpellCastTargets targets;
            targets.setUnitTarget(player);
            SpellEntry const *spellInfo = sSpellStore.LookupEntry(67468);
            // not triggered because of 2s cast time (not instant)
            Spell *vomit = new Spell(player, spellInfo, false);
            vomit->prepare(&targets);
        }
    }
    
    int16 drunkMod = damage * 256;
    if (drunkMod < 0 && currentDrunk < (-drunkMod))
        currentDrunk = 0;
    else if (currentDrunk + drunkMod > 0xFFFF)
        currentDrunk = 0xFFFF;
    else
        currentDrunk += drunkMod;
    player->SetDrunkValue(currentDrunk, m_CastItem ? m_CastItem->GetEntry() : 0);
}

void Spell::EffectFeedPet(SpellEffIndex effIndex)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player *_player = (Player*)m_caster;

    Item* foodItem = m_targets.getItemTarget();
    if (!foodItem)
        return;

    Pet *pet = _player->GetPet();
    if (!pet)
        return;

    if (!pet->isAlive())
        return;

    int32 benefit = m_spellInfo->EffectBasePoints[effIndex];
    if (benefit <= 0)
        return;

    ExecuteLogEffectDestroyItem(effIndex, foodItem->GetEntry());

    uint32 count = 1;
    _player->DestroyItemCount(foodItem, count, true);
    // TODO: fix crash when a spell has two effects, both pointed at the same item target

    m_caster->CastCustomSpell(pet, m_spellInfo->EffectTriggerSpell[effIndex], &benefit, NULL, NULL, true);
}

void Spell::EffectDismissPet(SpellEffIndex effIndex)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Pet* pet = m_caster->ToPlayer()->GetPet();

    // not let dismiss dead pet
    if (!pet||!pet->isAlive())
        return;

    ExecuteLogEffectUnsummonObject(effIndex, pet);
    m_caster->ToPlayer()->RemovePet(pet, PET_SLOT_ACTUAL_PET_SLOT);
    m_caster->ToPlayer()->m_currentPetSlot = PET_SLOT_DELETED;
}

void Spell::EffectSummonObject(SpellEffIndex effIndex)
{
    uint32 go_id = m_spellInfo->EffectMiscValue[effIndex];

    uint8 slot = 0;
    switch(m_spellInfo->Effect[effIndex])
    {
        case SPELL_EFFECT_SUMMON_OBJECT_SLOT1: slot = 0; break;
        case SPELL_EFFECT_SUMMON_OBJECT_SLOT2: slot = 1; break;
        case SPELL_EFFECT_SUMMON_OBJECT_SLOT3: slot = 2; break;
        case SPELL_EFFECT_SUMMON_OBJECT_SLOT4: slot = 3; break;
        default: return;
    }

    // Hunter traps should have their own slots depending on type (frost / fire / nature)
    switch (m_spellInfo->Id)
    {
        case 13795: // Immolation trap
        case 82944: // Immolation trap (Trap launcher)
        case 13813: // Explosive trap
        case 82938: // Explosive trap (Trap launcher)
            slot = 0;
            break;
        case 1499:  // Freezing trap
        case 60202: // Freezing trap (Trap launcher)
        case 13809: // Ice Trap
        case 82940: // Ice trap (Trap launcher)
            slot = 1;
            break;
        case 34600: // Snake Trap
        case 82949: // Snake Trap (Trap launcher)
            slot = 2;
            break;
    }

    float x, y, z;
    // If dest location if present
    if (m_targets.HasDst())
        m_targets.m_dstPos.GetPosition(x, y, z);
    // Summon in random point all other units if location present
    else
        m_caster->GetClosePoint(x, y, z, DEFAULT_WORLD_OBJECT_SIZE);

    float o = m_caster->GetOrientation();
    int32 duration = GetSpellDuration(m_spellInfo);

    // Special case - Archaeology, spell Survey
    if (m_spellInfo->Id == 80451 && m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        duration = 60000; // 1 minute
        Creature* pNearest = NULL;
        float distance = -1.0f;
        uint64 guid = 0;
        for (uint8 i = 0; i < MAX_DIGSITES; i++)
        {
            guid = MAKE_NEW_GUID(m_caster->ToPlayer()->GetDigCreatureSlot(i), 2, HIGHGUID_UNIT);
            Creature* pTemp = Creature::GetCreature(*m_caster, guid);
            if (!pTemp || pTemp->GetMapId() != m_caster->GetMapId())
                continue;

            if (pTemp->GetDistance2d(m_caster) < distance || distance < 0.0f)
            {
                pNearest = pTemp;
                distance = pTemp->GetDistance2d(m_caster);
            }
        }

        if (pNearest)
        {
            // TODO: confirm distances
            if (distance < 5.0f)
            {
                // We digged out fragment
                QueryResult res = WorldDatabase.PQuery("SELECT type FROM research_site WHERE site IN (SELECT site_id \
                                                       FROM creature_archaeology_assign WHERE guid='%u');",pNearest->GetGUIDLow());
                if (res)
                {
                    uint32 type = (*res)[0].GetUInt32();
                    switch (type)
                    {
                        case 1:  // Dwarf
                            go_id = 204282;
                            break;
                        case 2:  // Draenei
                            go_id = 207188;
                            break;
                        case 3:  // Fossil
                            go_id = 206836;
                            break;
                        case 4:  // Night Elf
                            go_id = 203071;
                            break;
                        case 5:  // Nerubian
                            go_id = 203078;
                            break;
                        case 6:  // Orc
                            go_id = 207187;
                            break;
                        case 7:  // Tol'vir
                            go_id = 207190;
                            break;
                        case 8:  // Troll
                            go_id = 202655;
                            break;
                        case 27: // Vrykul
                            go_id = 207189;
                            break;
                        default: // Other or not listed
                            sLog->outError("Not listed siteId %u for creature %u", type, pNearest->GetGUIDLow());
                            break;
                    }
                }
                x = pNearest->GetPositionX();
                y = pNearest->GetPositionY();
                z = pNearest->GetPositionZ();
                m_caster->ToPlayer()->DiggedCreature(pNearest->GetGUIDLow());
            }
            else if (distance < 50.0f)
            {
                // Green tool
                int32 mult = int32(urand(0,2))-1;
                o = m_caster->GetAngle(pNearest) + mult*M_PI/16;
                go_id = 4510101;
            }
            else if (distance < 100.0f)
            {
                // Yellow tool
                int32 mult = int32(urand(0,6))-3;
                o = m_caster->GetAngle(pNearest) + mult*M_PI/16;
                go_id = 4510102;
            }
            else
            {
                // Red tool
                int32 mult = int32(urand(0,8))-4;
                o = m_caster->GetAngle(pNearest) + mult*M_PI/16;
                go_id = 4510103;
            }
        }
        else
        {
            // Use red tool if no creature in range (should not happen)
            go_id = 4510103;
        }
    }
    else if (m_spellInfo->Id >= 84996 && m_spellInfo->Id <= 85000) // raid markers
    {
        if (m_caster && m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->ToPlayer()->GetGroup())
        {
            if(m_caster->ToPlayer()->GetGroup()->GetLeaderGUID() == m_caster->GetGUID())
            {
                m_caster->ToPlayer()->GetGroup()->RemoveMarkerBySpellId(m_caster->ToPlayer(),m_spellInfo->Id);
                Creature* trigger = m_caster->SummonCreature(RAID_MARKER_TRIGGER, x, y, z, o, TEMPSUMMON_TIMED_DESPAWN, 300000); // 5 minutes
                if (trigger)
                {
                    trigger->setFaction(35);
                    m_caster->ToPlayer()->AddSummonToMap(RAID_MARKER_TRIGGER, trigger->GetGUID(),time(NULL));
                    trigger->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);
                    m_caster->ToPlayer()->GetGroup()->RefreshMarkerBySpellIdToGroup(m_spellInfo->Id);
                    return;
                }
            }
        }
    }

    uint64 guid = m_caster->m_ObjectSlot[slot];
    if (guid != 0)
    {
        GameObject* obj = NULL;
        if (m_caster)
            obj = m_caster->GetMap()->GetGameObject(guid);

        if (obj)
        {
            // Recast case - null spell id to make auras not be removed on object remove from world
            if (m_spellInfo->Id == obj->GetSpellId())
                obj->SetSpellId(0);
            m_caster->RemoveGameObject(obj, true);
        }
        m_caster->m_ObjectSlot[slot] = 0;
    }

    GameObject* pGameObj = new GameObject;

    Map *map = m_caster->GetMap();
    if (!pGameObj->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_GAMEOBJECT), go_id, map,
        m_caster->GetPhaseMask(), x, y, z, o, 0.0f, 0.0f, 0.0f, 0.0f, 0, GO_STATE_READY))
    {
        delete pGameObj;
        return;
    }

    //pGameObj->SetUInt32Value(GAMEOBJECT_LEVEL,m_caster->getLevel());
    pGameObj->SetRespawnTime(duration > 0 ? duration/IN_MILLISECONDS : 0);
    pGameObj->SetSpellId(m_spellInfo->Id);
    m_caster->AddGameObject(pGameObj);

    ExecuteLogEffectSummonObject(effIndex, pGameObj);

    map->Add(pGameObj);

    m_caster->m_ObjectSlot[slot] = pGameObj->GetGUID();
}

void Spell::EffectResurrect(SpellEffIndex effIndex)
{
    if (!unitTarget)
        return;
    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    if (unitTarget->isAlive())
        return;
    if (!unitTarget->IsInWorld())
        return;

    switch (m_spellInfo->Id)
    {
        // Defibrillate (Goblin Jumper Cables) have 33% chance on success
        case 8342:
            if (roll_chance_i(67))
            {
                m_caster->CastSpell(m_caster, 8338, true, m_CastItem);
                return;
            }
            break;
        // Defibrillate (Goblin Jumper Cables XL) have 50% chance on success
        case 22999:
            if (roll_chance_i(50))
            {
                m_caster->CastSpell(m_caster, 23055, true, m_CastItem);
                return;
            }
            break;
        // Defibrillate ( Gnomish Army Knife) have 67% chance on success_list
        case 54732:
            if (roll_chance_i(33))
            {
                return;
            }
            break;
        // Mass Resurrection
        case 83968:
            // debuff Recently Mass Resurrected
            if (unitTarget->HasAura(95223))
                return;
            m_caster->CastSpell(unitTarget, 95223, true);
        default:
            break;
    }

    Player* pTarget = unitTarget->ToPlayer();

    if (pTarget->isRessurectRequested())       // already have one active request
        return;

    uint32 percentMod = 0;
    // Auras which increases percentage of health and mana player is resurrected with
    if (pTarget && m_caster->ToPlayer())
    {
        if (pTarget->GetGuildId() == m_caster->ToPlayer()->GetGuildId())
            percentMod = m_caster->GetTotalAuraModifier(SPELL_AURA_MOD_RESURRECT_STATS);
    }

    uint32 health = pTarget->CountPctFromMaxHealth(damage + percentMod);
    uint32 mana   = pTarget->GetMaxPower(POWER_MANA) * (damage + percentMod) / 100;

    ExecuteLogEffectResurrect(effIndex, pTarget);

    pTarget->setResurrectRequestData(m_caster->GetGUID(), m_caster->GetMapId(), m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ(), health, mana);
    SendResurrectRequest(pTarget);
}

void Spell::EffectAddExtraAttacks(SpellEffIndex effIndex)
{
    if (!unitTarget || !unitTarget->isAlive() || !unitTarget->getVictim())
        return;

    if (unitTarget->m_extraAttacks)
        return;

    unitTarget->m_extraAttacks = damage;

    ExecuteLogEffectExtraAttacks(effIndex, unitTarget->getVictim(), damage);
}

void Spell::EffectParry(SpellEffIndex /*effIndex*/)
{
    if (unitTarget && unitTarget->GetTypeId() == TYPEID_PLAYER)
        unitTarget->ToPlayer()->SetCanParry(true);
}

void Spell::EffectBlock(SpellEffIndex /*effIndex*/)
{
    if (unitTarget && unitTarget->GetTypeId() == TYPEID_PLAYER)
        unitTarget->ToPlayer()->SetCanBlock(true);
}

void Spell::EffectLeap(SpellEffIndex /*effIndex*/)
{
    if (unitTarget->isInFlight())
        return;

    if (!m_targets.HasDst())
        return;

    unitTarget->NearTeleportTo(m_targets.m_dstPos.GetPositionX(), m_targets.m_dstPos.GetPositionY(), m_targets.m_dstPos.GetPositionZ(), m_targets.m_dstPos.GetOrientation(), unitTarget == m_caster);
}

void Spell::EffectReputation(SpellEffIndex effIndex)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    Player *_player = (Player*)unitTarget;

    int32  rep_change = damage;//+1;           // field store reputation change -1

    uint32 faction_id = m_spellInfo->EffectMiscValue[effIndex];

    FactionEntry const* factionEntry = sFactionStore.LookupEntry(faction_id);

    if (!factionEntry)
        return;

    if (RepRewardRate const * repData = sObjectMgr->GetRepRewardRate(faction_id))
    {
        rep_change = int32((float)rep_change * repData->spell_rate);
    }

    _player->GetReputationMgr().ModifyReputation(factionEntry, rep_change);
}

void Spell::EffectQuestComplete(SpellEffIndex effIndex)
{
    Player *pPlayer;

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
        pPlayer = (Player*)m_caster;
    else if (unitTarget && unitTarget->GetTypeId() == TYPEID_PLAYER)
        pPlayer = (Player*)unitTarget;
    else
        return;

    uint32 quest_id = m_spellInfo->EffectMiscValue[effIndex];
    if (quest_id)
    {
        uint16 log_slot = pPlayer->FindQuestSlot(quest_id);
        if (log_slot < MAX_QUEST_LOG_SIZE)
            pPlayer->AreaExploredOrEventHappens(quest_id);
        else if (!pPlayer->GetQuestRewardStatus(quest_id))   // never rewarded before
            pPlayer->CompleteQuest(quest_id);   // quest not in log - for internal use
    }
}

void Spell::EffectForceDeselect(SpellEffIndex /*effIndex*/)
{
    WorldPacket data(SMSG_CLEAR_TARGET, 8);
    data << uint64(m_caster->GetGUID());
    m_caster->SendMessageToSet(&data, true);
}

void Spell::EffectSelfResurrect(SpellEffIndex effIndex)
{
    if (!unitTarget || unitTarget->isAlive())
        return;
    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;
    if (!unitTarget->IsInWorld())
        return;

    uint32 health = 0;
    uint32 mana = 0;

    // flat case
    if (damage < 0)
    {
        health = uint32(-damage);
        mana = m_spellInfo->EffectMiscValue[effIndex];
    }
    // percent case
    else
    {
        health = unitTarget->CountPctFromMaxHealth(damage);
        if (unitTarget->GetMaxPower(POWER_MANA) > 0)
            mana = uint32(damage/100.0f*unitTarget->GetMaxPower(POWER_MANA));
    }

    Player *plr = unitTarget->ToPlayer();
    plr->ResurrectPlayer(0.0f);

    plr->SetHealth(health);
    plr->SetPower(POWER_MANA, mana);
    plr->SetPower(POWER_RAGE, 0);
    plr->SetPower(POWER_ENERGY, plr->GetMaxPower(POWER_ENERGY));

    plr->SpawnCorpseBones();
}

void Spell::EffectSkinning(SpellEffIndex /*effIndex*/)
{
    if (unitTarget->GetTypeId() != TYPEID_UNIT)
        return;
    if (!m_caster || m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Creature* creature = unitTarget->ToCreature();
    int32 targetLevel = creature->getLevel();

    uint32 skill = creature->GetCreatureInfo()->GetRequiredLootSkill();

    m_caster->ToPlayer()->SendLoot(creature->GetGUID(),LOOT_SKINNING);
    creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);

    int32 reqValue = targetLevel < 10 ? 0 : targetLevel < 20 ? (targetLevel-10)*10 : targetLevel*5;

    int32 skillValue = m_caster->ToPlayer()->GetPureSkillValue(skill);

    // Double chances for elites
    m_caster->ToPlayer()->UpdateGatherSkill(skill, skillValue, reqValue, creature->isElite() ? 2 : 1);
}

void Spell::EffectCharge(SpellEffIndex /*effIndex*/)
{
    Unit *target = m_targets.getUnitTarget();
    if (!target)
        return;

    float angle = target->GetRelativeAngle(m_caster);

    Position pos;
    target->GetContactPoint(m_caster, pos.m_positionX, pos.m_positionY, pos.m_positionZ);
    target->GetFirstCollisionPosition(pos, target->GetObjectSize(), angle);

    // Warriors Charge - Stun the target - must be implemented this way since charge doesn't have "Trigger spell" effect
    if (m_spellInfo->Id == 100 && !target->HasAuraType(SPELL_AURA_DEFLECT_SPELLS))
        m_caster->CastSpell(target, 96273, true);

    if (m_preGeneratedPath.GetPathType() & PATHFIND_NOPATH)
        m_caster->GetMotionMaster()->MoveCharge(pos.m_positionX, pos.m_positionY, pos.m_positionZ);
    else
        m_caster->GetMotionMaster()->MoveCharge(m_preGeneratedPath, SPEED_CHARGE, EVENT_CHARGE_PREPATH);

    // Juggernaut - share cooldown of Charge and Intercept
    if (m_caster->HasAura(64976))
    {
        if (m_spellInfo->Id == 100)
        {
            const SpellEntry* pSpell = sSpellStore.LookupEntry(20252);
            m_caster->ToPlayer()->AddSpellAndCategoryCooldowns(pSpell,0);
            //m_caster->ToPlayer()->SendCooldownEvent(pSpell);
            //Since SendCooldownEvent doesn't do what we want, we must send cooldown manually
            WorldPacket data(SMSG_SPELL_COOLDOWN, 8+1+4);
            data << uint64(m_caster->GetGUID());
            data << uint8(1);
            data << uint32(20252);
            data << uint32(15000); // 15 seconds cooldown (as Charge)
            m_caster->ToPlayer()->GetSession()->SendPacket(&data);
        }
        else if (m_spellInfo->Id == 20252)
        {
            const SpellEntry* pSpell = sSpellStore.LookupEntry(100);
            m_caster->ToPlayer()->AddSpellAndCategoryCooldowns(pSpell,0);
            //And again - manually send cooldown
            WorldPacket data(SMSG_SPELL_COOLDOWN, 8+1+4);
            data << uint64(m_caster->GetGUID());
            data << uint8(1);
            data << uint32(100);
            data << uint32(0);    // 0 means full spell cooldown
            m_caster->ToPlayer()->GetSession()->SendPacket(&data);
        }
    }

    // not all charge effects used in negative spells
    if (!IsPositiveSpell(m_spellInfo->Id) && m_caster->GetTypeId() == TYPEID_PLAYER)
        m_caster->Attack(target, true);
}

void Spell::EffectChargeDest(SpellEffIndex /*effIndex*/)
{
    if (m_targets.HasDst())
    {
        float x, y, z;
        m_targets.m_dstPos.GetPosition(x, y, z);
        m_caster->GetMotionMaster()->MoveCharge(x, y, z);
    }
}

void Spell::EffectKnockBack(SpellEffIndex effIndex)
{
    if (!unitTarget)
        return;

    if (unitTarget->HasAura(87856)) // player is inside Al'akir squall line vehicle
        return;

    if (Creature* creatureTarget = unitTarget->ToCreature()) // If Creature is WorldBoss or regular boss
    {
        CreatureInfo const *cinfo = sObjectMgr->GetCreatureTemplate(creatureTarget->GetEntry());
        if(cinfo)
            if (creatureTarget->isWorldBoss() ||  (cinfo->rank == 3) )
                return;
    }

    // Spells with SPELL_EFFECT_KNOCK_BACK (like Thunderstorm) can't knockback target if target has ROOT
    if (unitTarget->hasUnitState(UNIT_STAT_ROOT))
        return;

    // Instantly interrupt non melee spells being casted
    if (unitTarget->IsNonMeleeSpellCasted(true))
        unitTarget->InterruptNonMeleeSpells(true);

    // Typhoon
    if (m_spellInfo->SpellFamilyName == SPELLFAMILY_DRUID && m_spellInfo->SpellFamilyFlags[1] & 0x01000000)
    {
        // Glyph of Typhoon
        if (m_caster->HasAura(62135))
            return;
    }

    // Thunderstorm
    if (m_spellInfo->SpellFamilyName == SPELLFAMILY_SHAMAN && m_spellInfo->SpellFamilyFlags[1] & 0x00002000)
    {
        // Glyph of Thunderstorm
        if (m_caster->HasAura(62132))
            return;

        // movement slowing aura
        m_caster->CastSpell(unitTarget, 100955, true);
    }

    float ratio = 0.1f; // dbc value ratio
    float speedxy = float(m_spellInfo->EffectMiscValue[effIndex]) * ratio;
    float speedz = float(damage) * ratio;
    if (speedxy < 0.1f && speedz < 0.1f)
        return;

    float x, y;
    if (m_spellInfo->Effect[effIndex] == SPELL_EFFECT_KNOCK_BACK_DEST)
    {
        if (m_targets.HasDst())
            m_targets.m_dstPos.GetPosition(x, y);
        else
            return;
    }
    // Magic Wings - Darkmoon Faire Island spell
    else if (m_spellInfo->Id == 102116)
    {
        m_caster->GetNearPoint2D(x, y, 5.0f, m_caster->GetOrientation() - M_PI);
        speedxy *= 1.2f;
        speedz *= 1.25f;
    }
    // Ultrasafe Personnel Launcher
    else if (m_spellInfo->Id == 77393)
    {
        // adjust knockback position to always "knock back" from the same absolute angle
        float dstAngle = m_caster->GetAngle(-5692.6215f, -923.6963f);
        m_caster->GetNearPoint2D(x, y, 5.0f, dstAngle);
    }
    else //if (m_spellInfo->Effect[i] == SPELL_EFFECT_KNOCK_BACK)
    {
        m_caster->GetPosition(x, y);
    }

    unitTarget->KnockbackFrom(x, y, speedxy, speedz);
}

void Spell::EffectLeapBack(SpellEffIndex effIndex)
{
    // Dont jump if rooted
    if (m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->hasUnitState(UNIT_STAT_ROOT))
        return;

    float speedxy = float(m_spellInfo->EffectMiscValue[effIndex])/10;
    float speedz = float(damage/10);
    if (!speedxy)
    {
        if (m_targets.getUnitTarget())
            m_caster->JumpTo(m_targets.getUnitTarget(), speedz);
    }
    else
    {
        //1891: Disengage
        m_caster->JumpTo(speedxy, speedz, m_spellInfo->SpellIconID != 1891);
    }

    // Disengage, talent Posthaste
    if (m_spellInfo->Id == 781)
    {
        int32 bp0 = 0;
        if (m_caster->HasAura(83560))
            bp0 = 30;
        else if (m_caster->HasAura(83558))
            bp0 = 15;

        if (bp0)
            m_caster->CastCustomSpell(m_caster, 83559, &bp0, 0, 0, true);
    }
}

void Spell::EffectQuestClear(SpellEffIndex effIndex)
{
    Player *pPlayer = NULL;
    if (m_caster->GetTypeId() == TYPEID_PLAYER)
        pPlayer = m_caster->ToPlayer();
    else if (unitTarget && unitTarget->GetTypeId() == TYPEID_PLAYER)
        pPlayer = unitTarget->ToPlayer();

    if (!pPlayer)
        return;

    uint32 quest_id = m_spellInfo->EffectMiscValue[effIndex];

    Quest const* pQuest = sObjectMgr->GetQuestTemplate(quest_id);

    if (!pQuest)
        return;

    QuestStatusMap::iterator qs_itr = pPlayer->getQuestStatusMap().find(quest_id);
    // Player has never done this quest
    if (qs_itr == pPlayer->getQuestStatusMap().end())
        return;

    // remove all quest entries for 'entry' from quest log
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 quest = pPlayer->GetQuestSlotQuestId(slot);
        if (quest == quest_id)
        {
            pPlayer->SetQuestSlot(slot, 0);

            // we ignore unequippable quest items in this case, its' still be equipped
            pPlayer->TakeQuestSourceItem(quest, false);
        }
    }

    // set quest status to not started (will be updated in DB at next save)
    pPlayer->SetQuestStatus(quest_id, QUEST_STATUS_NONE);

    // reset rewarded for restart repeatable quest
    QuestStatusData &data = qs_itr->second;
    data.m_rewarded = false;
}

void Spell::EffectSendTaxi(SpellEffIndex effIndex)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    unitTarget->ToPlayer()->ActivateTaxiPathTo(m_spellInfo->EffectMiscValue[effIndex],m_spellInfo->Id);
}

void Spell::EffectPullTowards(SpellEffIndex effIndex)
{
    if (!unitTarget)
        return;

    float speedZ = (float)(SpellMgr::CalculateSpellEffectAmount(m_spellInfo, effIndex) / 10);
    float speedXY = (float)(m_spellInfo->EffectMiscValue[effIndex]/10);
    Position pos;
    if (m_spellInfo->Effect[effIndex] == SPELL_EFFECT_PULL_TOWARDS_DEST)
    {
        if (m_targets.HasDst())
            pos.Relocate(m_targets.m_dstPos);
        else
            return;
    }
    else //if (m_spellInfo->Effect[i] == SPELL_EFFECT_PULL_TOWARDS)
    {
        pos.Relocate(m_caster);
    }

    unitTarget->GetMotionMaster()->MoveJump(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), speedXY, speedZ);
}

void Spell::EffectDispelMechanic(SpellEffIndex effIndex)
{
    if (!unitTarget)
        return;

    uint32 mechanic = m_spellInfo->EffectMiscValue[effIndex];

    std::queue < std::pair < uint32, uint64 > > dispel_list;

    Unit::AuraMap const& auras = unitTarget->GetOwnedAuras();
    for (Unit::AuraMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
    {
        Aura * aura = itr->second;
        if (!aura->GetApplicationOfTarget(unitTarget->GetGUID()))
            continue;
        bool success = false;
        GetDispelChance(aura->GetCaster(), unitTarget, aura->GetId(), !unitTarget->IsFriendlyTo(m_caster), &success);
        if ((GetAllSpellMechanicMask(aura->GetSpellProto()) & (1 << mechanic)) && success)
            dispel_list.push(std::make_pair(aura->GetId(), aura->GetCasterGUID()));
    }

    for (; dispel_list.size(); dispel_list.pop())
    {
        unitTarget->RemoveAura(dispel_list.front().first, dispel_list.front().second, 0, AURA_REMOVE_BY_ENEMY_SPELL);
    }
}

void Spell::EffectSummonDeadPet(SpellEffIndex /*effIndex*/)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;
    Player *_player = (Player*)m_caster;
    Pet *pet = _player->GetPet();
    if (!pet)
        return;
    if (pet->isAlive())
        return;
    if (damage < 0)
        return;

    float x,y,z;
    _player->GetPosition(x, y, z);
    _player->GetMap()->CreatureRelocation(pet, x, y, z, _player->GetOrientation());

    pet->SetUInt32Value(UNIT_DYNAMIC_FLAGS, 0);
    pet->RemoveFlag (UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);
    pet->setDeathState(ALIVE);
    pet->clearUnitState(UNIT_STAT_ALL_STATE);
    pet->SetHealth(pet->CountPctFromMaxHealth(damage));

    //pet->AIM_Initialize();
    //_player->PetSpellInitialize();
    pet->SavePetToDB(PET_SLOT_ACTUAL_PET_SLOT);
}

void Spell::EffectDestroyAllTotems(SpellEffIndex /*effIndex*/)
{
    int32 mana = 0;
    for (uint8 slot = SUMMON_SLOT_TOTEM; slot < MAX_TOTEM_SLOT; ++slot)
    {
        if (!m_caster->m_SummonSlot[slot])
            continue;

        Creature* totem = m_caster->GetMap()->GetCreature(m_caster->m_SummonSlot[slot]);
        if (totem && totem->isTotem())
        {
            uint32 spell_id = totem->GetUInt32Value(UNIT_CREATED_BY_SPELL);
            SpellEntry const* spellInfo = sSpellStore.LookupEntry(spell_id);
            if (spellInfo)
            {
                mana += spellInfo->manaCost;
                mana += spellInfo->ManaCostPercentage * m_caster->GetCreateMana() / 100;
            }
            totem->ToTotem()->UnSummon();
        }
    }
    mana = mana * damage / 100;

    if (mana)
        m_caster->CastCustomSpell(m_caster, 39104, &mana, NULL, NULL, true);
}

void Spell::EffectDurabilityDamage(SpellEffIndex effIndex)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    int32 slot = m_spellInfo->EffectMiscValue[effIndex];

    // FIXME: some spells effects have value -1/-2
    // Possibly its mean -1 all player equipped items and -2 all items
    if (slot < 0)
    {
        unitTarget->ToPlayer()->DurabilityPointsLossAll(damage, (slot < -1));
        return;
    }

    // invalid slot value
    if (slot >= INVENTORY_SLOT_BAG_END)
        return;

    if (Item* item = unitTarget->ToPlayer()->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        unitTarget->ToPlayer()->DurabilityPointsLoss(item, damage);

    ExecuteLogEffectDurabilityDamage(effIndex, unitTarget, slot, damage);
}

void Spell::EffectDurabilityDamagePCT(SpellEffIndex effIndex)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    int32 slot = m_spellInfo->EffectMiscValue[effIndex];

    // FIXME: some spells effects have value -1/-2
    // Possibly its mean -1 all player equipped items and -2 all items
    if (slot < 0)
    {
        unitTarget->ToPlayer()->DurabilityLossAll(double(damage)/100.0f, (slot < -1));
        return;
    }

    // invalid slot value
    if (slot >= INVENTORY_SLOT_BAG_END)
        return;

    if (damage <= 0)
        return;

    if (Item* item = unitTarget->ToPlayer()->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        unitTarget->ToPlayer()->DurabilityLoss(item, double(damage)/100.0f);
}

void Spell::EffectModifyThreatPercent(SpellEffIndex /*effIndex*/)
{
    if (!unitTarget)
        return;

    unitTarget->getThreatManager().modifyThreatPercent(m_caster, damage);
}

void Spell::EffectTransmitted(SpellEffIndex effIndex)
{
    uint32 name_id = m_spellInfo->EffectMiscValue[effIndex];

    GameObjectInfo const* goinfo = sObjectMgr->GetGameObjectInfo(name_id);

    if (!goinfo)
    {
        sLog->outErrorDb("Gameobject (Entry: %u) not exist and not created at spell (ID: %u) cast",name_id, m_spellInfo->Id);
        return;
    }

    float fx, fy, fz;

    if (m_targets.HasDst())
        m_targets.m_dstPos.GetPosition(fx, fy, fz);
    //FIXME: this can be better check for most objects but still hack
    else if (m_spellInfo->EffectRadiusIndex[effIndex] && m_spellInfo->speed == 0)
    {
        float dis = GetEffectRadius(effIndex);
        m_caster->GetClosePoint(fx, fy, fz, DEFAULT_WORLD_OBJECT_SIZE, dis);
    }
    else
    {
        //GO is always friendly to it's creator, get range for friends
        float min_dis = GetSpellMinRangeForFriend(sSpellRangeStore.LookupEntry(m_spellInfo->rangeIndex));
        float max_dis = GetSpellMaxRangeForFriend(sSpellRangeStore.LookupEntry(m_spellInfo->rangeIndex));
        float dis = (float)rand_norm() * (max_dis - min_dis) + min_dis;

        m_caster->GetClosePoint(fx, fy, fz, DEFAULT_WORLD_OBJECT_SIZE, dis);
    }

    Map *cMap = m_caster->GetMap();
    if (goinfo->type == GAMEOBJECT_TYPE_FISHINGNODE || goinfo->type == GAMEOBJECT_TYPE_FISHINGHOLE)
    {
        LiquidData liqData;
        if ( !cMap->IsInWater(fx, fy, fz + 1.f/* -0.5f */, &liqData))             // Hack to prevent fishing bobber from failing to land on fishing hole
        { // but this is not proper, we really need to ignore not materialized objects
            SendCastResult(SPELL_FAILED_NOT_HERE);
            SendChannelUpdate(0);
            return;
        }

        // replace by water level in this case
        //fz = cMap->GetWaterLevel(fx, fy);
        fz = liqData.level;
    }
    // if gameobject is summoning object, it should be spawned right on caster's position
    else if (goinfo->type == GAMEOBJECT_TYPE_SUMMONING_RITUAL)
    {
        m_caster->GetPosition(fx, fy, fz);
    }

    GameObject* pGameObj = new GameObject;

    if (!pGameObj->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_GAMEOBJECT), name_id, cMap,
        m_caster->GetPhaseMask(), fx, fy, fz, m_caster->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 100, GO_STATE_READY))
    {
        delete pGameObj;
        return;
    }

    int32 duration = GetSpellDuration(m_spellInfo);

    switch(goinfo->type)
    {
        case GAMEOBJECT_TYPE_FISHINGNODE:
        {
            m_caster->SetUInt64Value(UNIT_FIELD_CHANNEL_OBJECT,pGameObj->GetGUID());
            m_caster->AddGameObject(pGameObj);              // will removed at spell cancel

            // end time of range when possible catch fish (FISHING_BOBBER_READY_TIME..GetDuration(m_spellInfo))
            // start time == fish-FISHING_BOBBER_READY_TIME (0..GetDuration(m_spellInfo)-FISHING_BOBBER_READY_TIME)
            int32 lastSec = 0;
            switch(urand(0, 3))
            {
                case 0: lastSec =  3; break;
                case 1: lastSec =  7; break;
                case 2: lastSec = 13; break;
                case 3: lastSec = 17; break;
            }

            duration = duration - lastSec*IN_MILLISECONDS + FISHING_BOBBER_READY_TIME*IN_MILLISECONDS;
            break;
        }
        case GAMEOBJECT_TYPE_SUMMONING_RITUAL:
        {
            if (m_caster->GetTypeId() == TYPEID_PLAYER)
            {
                pGameObj->AddUniqueUse(m_caster->ToPlayer());
                m_caster->AddGameObject(pGameObj);      // will removed at spell cancel
            }
            break;
        }
        case GAMEOBJECT_TYPE_DUEL_ARBITER: // 52991
            m_caster->AddGameObject(pGameObj);
            break;
        case GAMEOBJECT_TYPE_FISHINGHOLE:
        case GAMEOBJECT_TYPE_CHEST:
        default:
            break;
    }

    pGameObj->SetRespawnTime(duration > 0 ? duration/IN_MILLISECONDS : 0);

    pGameObj->SetOwnerGUID(m_caster->GetGUID());

    //pGameObj->SetUInt32Value(GAMEOBJECT_LEVEL, m_caster->getLevel());
    pGameObj->SetSpellId(m_spellInfo->Id);

    ExecuteLogEffectSummonObject(effIndex, pGameObj);

    sLog->outStaticDebug("AddObject at SpellEfects.cpp EffectTransmitted");
    //m_caster->AddGameObject(pGameObj);
    //m_ObjToDel.push_back(pGameObj);

    cMap->Add(pGameObj);

    if (uint32 linkedEntry = pGameObj->GetGOInfo()->GetLinkedGameObjectEntry())
    {
        GameObject* linkedGO = new GameObject;
        if (linkedGO->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_GAMEOBJECT), linkedEntry, cMap,
            m_caster->GetPhaseMask(), fx, fy, fz, m_caster->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 100, GO_STATE_READY))
        {
            linkedGO->SetRespawnTime(duration > 0 ? duration/IN_MILLISECONDS : 0);
            //linkedGO->SetUInt32Value(GAMEOBJECT_LEVEL, m_caster->getLevel());
            linkedGO->SetSpellId(m_spellInfo->Id);
            linkedGO->SetOwnerGUID(m_caster->GetGUID());

            ExecuteLogEffectSummonObject(effIndex, linkedGO);

            linkedGO->GetMap()->Add(linkedGO);
        }
        else
        {
            delete linkedGO;
            linkedGO = NULL;
            return;
        }
    }
}

void Spell::EffectProspecting(SpellEffIndex /*effIndex*/)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* p_caster = (Player*)m_caster;
    if (!itemTarget || !(itemTarget->GetProto()->Flags & ITEM_PROTO_FLAG_PROSPECTABLE))
        return;

    if (itemTarget->GetCount() < 5)
        return;

    if (sWorld->getBoolConfig(CONFIG_SKILL_PROSPECTING))
    {
        uint32 SkillValue = p_caster->GetPureSkillValue(SKILL_JEWELCRAFTING);
        uint32 reqSkillValue = itemTarget->GetProto()->RequiredSkillRank;
        p_caster->UpdateGatherSkill(SKILL_JEWELCRAFTING, SkillValue, reqSkillValue);
    }

    m_caster->ToPlayer()->SendLoot(itemTarget->GetGUID(), LOOT_PROSPECTING);
}

void Spell::EffectMilling(SpellEffIndex /*effIndex*/)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* p_caster = (Player*)m_caster;
    if (!itemTarget || !(itemTarget->GetProto()->Flags & ITEM_PROTO_FLAG_MILLABLE))
        return;

    if (itemTarget->GetCount() < 5)
        return;

    if (sWorld->getBoolConfig(CONFIG_SKILL_MILLING))
    {
        uint32 SkillValue = p_caster->GetPureSkillValue(SKILL_INSCRIPTION);
        uint32 reqSkillValue = itemTarget->GetProto()->RequiredSkillRank;
        p_caster->UpdateGatherSkill(SKILL_INSCRIPTION, SkillValue, reqSkillValue);
    }

    m_caster->ToPlayer()->SendLoot(itemTarget->GetGUID(), LOOT_MILLING);
}

void Spell::EffectSkill(SpellEffIndex /*effIndex*/)
{
    sLog->outDebug("WORLD: SkillEFFECT");
}

/* There is currently no need for this effect. We handle it in Battleground.cpp
   If we would handle the resurrection here, the spiritguide would instantly disappear as the
   player revives, and so we wouldn't see the spirit heal visual effect on the npc.
   This is why we use a half sec delay between the visual effect and the resurrection itself */
void Spell::EffectSpiritHeal(SpellEffIndex /*effIndex*/)
{
    /*
    if (!unitTarget || unitTarget->isAlive())
        return;
    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;
    if (!unitTarget->IsInWorld())
        return;

    //m_spellInfo->EffectBasePoints[i]; == 99 (percent?)
    //unitTarget->ToPlayer()->setResurrect(m_caster->GetGUID(), unitTarget->GetPositionX(), unitTarget->GetPositionY(), unitTarget->GetPositionZ(), unitTarget->GetMaxHealth(), unitTarget->GetMaxPower(POWER_MANA));
    unitTarget->ToPlayer()->ResurrectPlayer(1.0f);
    unitTarget->ToPlayer()->SpawnCorpseBones();
    */
}

// remove insignia spell effect
void Spell::EffectSkinPlayerCorpse(SpellEffIndex /*effIndex*/)
{
    sLog->outDebug("Effect: SkinPlayerCorpse");
    if ((m_caster->GetTypeId() != TYPEID_PLAYER) || (unitTarget->GetTypeId() != TYPEID_PLAYER) || (unitTarget->isAlive()))
        return;

    unitTarget->ToPlayer()->RemovedInsignia((Player*)m_caster);
}

void Spell::EffectStealBeneficialBuff(SpellEffIndex effIndex)
{
    sLog->outDebug("Effect: StealBeneficialBuff");

    if (!unitTarget || unitTarget == m_caster)                 // can't steal from self
        return;

    DispelChargesList steal_list;

    // Create dispel mask by dispel type
    uint32 dispelMask  = GetDispellMask(DispelType(m_spellInfo->EffectMiscValue[effIndex]));
    Unit::AuraMap const& auras = unitTarget->GetOwnedAuras();
    for (Unit::AuraMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
    {
        Aura * aura = itr->second;
        AuraApplication * aurApp = aura->GetApplicationOfTarget(unitTarget->GetGUID());
        if (!aurApp)
            continue;

        if ((1<<aura->GetSpellProto()->Dispel) & dispelMask)
        {
            // Need check for passive? this
            if (!aurApp->IsPositive() || aura->IsPassive() || aura->GetSpellProto()->AttributesEx4 & SPELL_ATTR4_NOT_STEALABLE)
                continue;

            // The charges / stack amounts don't count towards the total number of auras that can be dispelled.
            // Ie: A dispel on a target with 5 stacks of Winters Chill and a Polymorph has 1 / (1 + 1) -> 50% chance to dispell
            // Polymorph instead of 1 / (5 + 1) -> 16%.
            bool dispel_charges = aura->GetSpellProto()->AttributesEx7 & SPELL_ATTR7_DISPEL_CHARGES;
            uint8 charges = dispel_charges ? aura->GetCharges() : aura->GetStackAmount();
            if (charges > 0)
                steal_list.push_back(std::make_pair(aura, charges));
        }
    }

    if (steal_list.empty())
        return;

    // Ok if exist some buffs for dispel try dispel it
    uint32 failCount = 0;
    DispelList success_list;
    WorldPacket dataFail(SMSG_DISPEL_FAILED, 8+8+4+4+damage*4);
    // dispel N = damage buffs (or while exist buffs for dispel)
    for (int32 count = 0; count < damage && !steal_list.empty();)
    {
        // Random select buff for dispel
        DispelChargesList::iterator itr = steal_list.begin();
        std::advance(itr, urand(0, steal_list.size() - 1));

        bool success = false;
        // 2.4.3 Patch Notes: "Dispel effects will no longer attempt to remove effects that have 100% dispel resistance."
        if (!GetDispelChance(itr->first->GetCaster(), unitTarget, itr->first->GetId(), !unitTarget->IsFriendlyTo(m_caster), &success))
        {
            steal_list.erase(itr);
            continue;
        }
        else
        {
            if (success)
            {
                success_list.push_back(std::make_pair(itr->first->GetId(), itr->first->GetCasterGUID()));
                --itr->second;
                if (itr->second <= 0)
                    steal_list.erase(itr);
            }
            else
            {
                if (!failCount)
                {
                    // Failed to dispell
                    dataFail << uint64(m_caster->GetGUID());            // Caster GUID
                    dataFail << uint64(unitTarget->GetGUID());          // Victim GUID
                    dataFail << uint32(m_spellInfo->Id);                // dispel spell id
                }
                ++failCount;
                dataFail << uint32(itr->first->GetId());                         // Spell Id
            }
            ++count;
        }
    }

    if (failCount)
        m_caster->SendMessageToSet(&dataFail, true);

    if (success_list.empty())
        return;

    WorldPacket dataSuccess(SMSG_SPELLSTEALLOG, 8+8+4+1+4+damage*5);
    dataSuccess.append(unitTarget->GetPackGUID());  // Victim GUID
    dataSuccess.append(m_caster->GetPackGUID());    // Caster GUID
    dataSuccess << uint32(m_spellInfo->Id);         // dispel spell id
    dataSuccess << uint8(0);                        // not used
    dataSuccess << uint32(success_list.size());     // count
    for (DispelList::iterator itr = success_list.begin(); itr!=success_list.end(); ++itr)
    {
        dataSuccess << uint32(itr->first);          // Spell Id
        dataSuccess << uint8(0);                    // 0 - steals !=0 transfers
        unitTarget->RemoveAurasDueToSpellBySteal(itr->first, itr->second, m_caster);
    }
    m_caster->SendMessageToSet(&dataSuccess, true);
}

void Spell::EffectKillCreditPersonal(SpellEffIndex effIndex)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    unitTarget->ToPlayer()->KilledMonsterCredit(m_spellInfo->EffectMiscValue[effIndex], 0);
}

void Spell::EffectKillCredit(SpellEffIndex effIndex)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    int32 creatureEntry = m_spellInfo->EffectMiscValue[effIndex];
    if (!creatureEntry)
    {
        if (m_spellInfo->Id == 42793) // Burn Body
            creatureEntry = 24008; // Fallen Combatant
    }

    if (creatureEntry)
        unitTarget->ToPlayer()->RewardPlayerAndGroupAtEvent(creatureEntry, unitTarget);
}

void Spell::EffectQuestFail(SpellEffIndex effIndex)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    unitTarget->ToPlayer()->FailQuest(m_spellInfo->EffectMiscValue[effIndex]);
}

void Spell::EffectQuestStart(SpellEffIndex effIndex)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    Player * player = unitTarget->ToPlayer();
    if (Quest const* qInfo = sObjectMgr->GetQuestTemplate(m_spellInfo->EffectMiscValue[effIndex]))
    {
        if (player->CanTakeQuest(qInfo, false) && player->CanAddQuest(qInfo, false))
        {
            player->AddQuest(qInfo, NULL);
        }
    }
}

void Spell::EffectActivateRune(SpellEffIndex effIndex)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player *plr = (Player*)m_caster;

    if (plr->getClass() != CLASS_DEATH_KNIGHT)
        return;

    // needed later
    m_runesState = m_caster->ToPlayer()->GetRunesState();

    // Second effect of Empower Rune Weapon
    // excluded for two reasons:
    // 1) effect #1 does it all at once
    // 2) we use that spell for dirty unclean hacky way to fix restoring single runes
    if (m_spellInfo->Id == 89831)
        return;

    RuneType refreshType = RuneType(m_spellInfo->EffectMiscValue[effIndex]);

    // Blood Tap effect should refresh blood rune and convert it. Unfortunatelly, it does it backwards, so it doesn't work
    if (m_spellInfo->Id == 45529)
        refreshType = RUNE_BLOOD;

    int8 highestCDRune = -1;
    uint32 highestCooldown = 0;

    uint32 count = damage;
    if (count == 0) count = 1;

    for (uint32 r = 0; r < count; r++)
    {
        for (uint32 j = 0; j < MAX_RUNES; j++)
        {
            uint32 cd = plr->GetRuneCooldown(j);
            if (plr->GetCurrentRune(j) == refreshType)
            {
                if (highestCDRune == -1 || cd > highestCooldown)
                {
                    highestCDRune = j;
                    highestCooldown = cd;
                }
            }
        }

        if (highestCDRune >= 0)
            plr->SetRuneCooldown(highestCDRune, 0);
        else if (m_spellInfo->Id == 45529) // Blood Tap: No blood rune to refresh. Try death rune in first two slots.
        {
            for (uint32 k = 0; k < 2; k++)
            {
                uint32 cd = plr->GetRuneCooldown(k);
                if (cd && plr->GetCurrentRune(k) == RUNE_DEATH)
                {
                    if (highestCDRune == -1 || cd > highestCooldown)
                    {
                        highestCDRune = k;
                        highestCooldown = cd;
                    }
                }
            }
            if (highestCDRune >= 0)
                plr->SetRuneCooldown(highestCDRune, 0);
        }
    }

    // Empower rune weapon
    if (m_spellInfo->Id == 47568)
    {
        // Need to do this just once
        if (effIndex != 0)
            return;

        // generating 25 Runic Power
        int32 bp0 = 250;
        plr->CastCustomSpell(plr, 62458, &bp0, 0, 0, true); // Probably not correct spell

        for (uint32 i = 0; i < MAX_RUNES; ++i)
        {
            if (plr->GetRuneCooldown(i))
                plr->SetRuneCooldown(i, 0);
        }

        plr->ResyncRunes(MAX_RUNES);
    }
    // Any other spell activating a rune
    else
    {
        // hack alert !!
        // Also cast spell originally used for Empower Rune Weapon
        // It allows us to tell client that one single rune was refreshed
        m_caster->CastSpell(m_caster, 89831, true);
    }
}

void Spell::EffectCreateTamedPet(SpellEffIndex effIndex)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER || unitTarget->GetPetGUID() || unitTarget->getClass() != CLASS_HUNTER)
        return;

    uint32 creatureEntry = m_spellInfo->EffectMiscValue[effIndex];
    Pet * pet = unitTarget->CreateTamedPetFrom(creatureEntry, m_spellInfo->Id);
    if (!pet)
        return;

    // add to world
    pet->GetMap()->Add(pet->ToCreature());

    // unitTarget has pet now
    unitTarget->SetMinion(pet, true, unitTarget->ToPlayer()->getSlotForNewPet());

    pet->InitTalentForLevel();

    if (unitTarget->GetTypeId() == TYPEID_PLAYER)
    {
        pet->SavePetToDB(PET_SLOT_ACTUAL_PET_SLOT);
        unitTarget->ToPlayer()->PetSpellInitialize();
    }
}

void Spell::EffectDiscoverTaxi(SpellEffIndex effIndex)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;
    uint32 nodeid = m_spellInfo->EffectMiscValue[effIndex];
    if (sTaxiNodesStore.LookupEntry(nodeid))
        unitTarget->ToPlayer()->GetSession()->SendDiscoverNewTaxiNode(nodeid);
}

void Spell::EffectTitanGrip(SpellEffIndex /*effIndex*/)
{
    if (unitTarget && unitTarget->GetTypeId() == TYPEID_PLAYER)
        unitTarget->ToPlayer()->SetCanTitanGrip(true);
}

void Spell::EffectRedirectThreat(SpellEffIndex /*effIndex*/)
{
    if (unitTarget)
        m_caster->SetReducedThreatPercent((uint32)damage, unitTarget->GetGUID());
}

void Spell::EffectWMODamage(SpellEffIndex /*effIndex*/)
{
    if (!gameObjTarget)
        return;

    Unit* caster = m_originalCaster;
    if (!caster)
        return;

    FactionTemplateEntry const* casterFaction = caster->getFactionTemplateEntry();
    FactionTemplateEntry const* targetFaction = sFactionTemplateStore.LookupEntry(gameObjTarget->GetUInt32Value(GAMEOBJECT_FACTION));
    // Do not allow to damage GO's of friendly factions (ie: Wintergrasp Walls/Ulduar Storm Beacons)
    if ((casterFaction && targetFaction && !casterFaction->IsFriendlyTo(*targetFaction)) || !targetFaction)
        gameObjTarget->ModifyHealth(-damage, caster, GetSpellInfo()->Id);
}

void Spell::EffectWMORepair(SpellEffIndex /*effIndex*/)
{
    if (!gameObjTarget)
        return;

    gameObjTarget->ModifyHealth(damage, m_caster);
}

void Spell::EffectWMOChange(SpellEffIndex effIndex)
{
    if (!gameObjTarget || !m_originalCaster)
        return;

    Player* player = m_originalCaster->GetCharmerOrOwnerPlayerOrPlayerItself();
    gameObjTarget->SetDestructibleState(GameObjectDestructibleState(m_spellInfo->EffectMiscValue[effIndex]), player, true);
}

uint32 Spell::GetMaxActiveSummons(uint32 entry)
{
    switch (entry)
    {
        // Wild Mushroom
        case 47649:
            return 3;
        // Shadowy Apparition
        case 46954:
            return 4;
        default:
            return (uint32)(-1);
    }
}

void Spell::SummonGuardian(uint32 i, uint32 entry, SummonPropertiesEntry const *properties)
{
    Unit *caster = m_originalCaster;
    if (!caster)
        return;

    if (caster->isTotem())
      caster = caster->ToTotem()->GetOwner();

    // in another case summon new
    uint8 level = caster->getLevel();

    // level of pet summoned using engineering item based at engineering skill level
    if (m_CastItem && caster->GetTypeId() == TYPEID_PLAYER)
        if (ItemPrototype const *proto = m_CastItem->GetProto())
            if (proto->RequiredSkill == SKILL_ENGINERING)
                if (uint16 skill202 = caster->ToPlayer()->GetSkillValue(SKILL_ENGINERING))
                    level = skill202/5;

    //float radius = GetSpellRadiusForFriend(sSpellRadiusStore.LookupEntry(m_spellInfo->EffectRadiusIndex[i]));
    float radius = 5.0f;
    uint32 amount = damage > 0 ? damage : 1;
    int32 duration = GetSpellDuration(m_spellInfo);
    switch (m_spellInfo->Id)
    {
        case 1122: // Inferno
        case 46619: // Raise Ally
            amount = 1;
            break;
        case 94548: // Cardboard Assassin (Engineering enchant)
            amount = 1;
            break;
    }
    if (Player* modOwner = m_originalCaster->GetSpellModOwner())
        modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_DURATION, duration);

    //TempSummonType summonType = (duration == 0) ? TEMPSUMMON_DEAD_DESPAWN : TEMPSUMMON_TIMED_DESPAWN;
    Map *map = caster->GetMap();

    for (uint32 count = 0; count < amount; ++count)
    {
        Position pos;
        GetSummonPosition(i, pos, radius, count);

        // Shadowy Apparition - summon pos is original casters pos
        if (entry == 46954 && m_originalCaster)
            m_originalCaster->GetPosition(&pos);

        TempSummon *summon = map->SummonCreature(entry, pos, properties, duration, caster);
        if (!summon)
            return;
        if (summon->HasUnitTypeMask(UNIT_MASK_GUARDIAN))
            ((Guardian*)summon)->InitStatsForLevel(level);

        summon->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);
        if (summon->HasUnitTypeMask(UNIT_MASK_MINION) && m_targets.HasDst() && entry != 46954)
            ((Minion*)summon)->SetFollowAngle(m_caster->GetAngle(summon));

        if (summon->GetEntry() == 27893) // Rune Weapon
        {
            if (uint32 weapon = m_caster->GetUInt32Value(PLAYER_VISIBLE_ITEM_16_ENTRYID))
            {
                summon->SetDisplayId(11686);
                summon->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID, weapon);
            }
            else
                summon->SetDisplayId(1126);
        }

        summon->AI()->EnterEvadeMode();

        if (summon->GetEntry() == 46954) // Shadowy Apparition (here due to threat list)
        {
            if (m_originalCaster && m_caster && m_originalCaster->isAlive() && m_caster->isAlive())
            {
                summon->SetSpeed(MOVE_RUN, 0.5f, true);
                summon->CastSpell(summon, 87427, true);
                summon->SetDisplayId(m_originalCaster->GetDisplayId());
                // Have to clone the caster
                m_originalCaster->CastSpell(summon, 87213, true);
                summon->getThreatManager().clearReferences();
                summon->getThreatManager().addThreat(m_caster, 100000.0f);
                summon->Attack(m_caster, true);
                summon->GetMotionMaster()->MoveChase(m_caster);

                if (m_originalCaster->ToPlayer())
                    m_originalCaster->ToPlayer()->AddSummonToMap(entry, summon->GetGUID(),time(NULL));
            }
        }

        summon->CastSpell(summon,65220,true); // Every pet/guardian should have avoidance
        ExecuteLogEffectSummonObject(i, summon);
    }
}

void Spell::GetSummonPosition(uint32 i, Position &pos, float radius, uint32 count)
{
    pos.SetOrientation(m_caster->GetOrientation());

    if (m_targets.HasDst())
    {
        // Summon 1 unit in dest location
        if (count == 0)
            pos.Relocate(m_targets.m_dstPos);
        // Summon in random point all other units if location present
        else
        {
            //This is a workaround. Do not have time to write much about it
            switch (m_spellInfo->EffectImplicitTargetA[i])
            {
                case TARGET_MINION:
                case TARGET_DEST_CASTER_RANDOM:
                    m_caster->GetNearPosition(pos, radius * (float)rand_norm(), (float)rand_norm()*static_cast<float>(2*M_PI));
                    break;
                case TARGET_DEST_DEST_RANDOM:
                case TARGET_DEST_TARGET_RANDOM:
                    m_caster->GetRandomPoint(m_targets.m_dstPos, radius, pos);
                    break;
                default:
                    pos.Relocate(m_targets.m_dstPos);
                    break;
            }
        }
    }
    // Summon if dest location not present near caster
    else
    {
        float x, y, z;
        m_caster->GetClosePoint(x,y,z,3.0f);
        pos.Relocate(x, y, z);
    }
    // if totem is not in LOS ...relocate
    if ((m_spellInfo->EffectImplicitTargetA[i] == TARGET_DEST_CASTER_FRONT_LEFT // earth
        || m_spellInfo->EffectImplicitTargetA[i] == TARGET_DEST_CASTER_BACK_LEFT // water
        || m_spellInfo->EffectImplicitTargetA[i] == TARGET_DEST_CASTER_FRONT_RIGHT // fire
        || m_spellInfo->EffectImplicitTargetA[i] == TARGET_DEST_CASTER_BACK_RIGHT) // air
        && !m_caster->IsWithinLOS(pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ()))
        pos.Relocate(m_caster->GetPositionX(),m_caster->GetPositionY(),m_caster->GetPositionZ());
}

void Spell::EffectRenamePet(SpellEffIndex /*effIndex*/)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT ||
        !unitTarget->ToCreature()->isPet() || ((Pet*)unitTarget)->getPetType() != HUNTER_PET)
        return;

    unitTarget->SetByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED);
}

void Spell::EffectPlayMusic(SpellEffIndex effIndex)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    uint32 soundid = m_spellInfo->EffectMiscValue[effIndex];

    if (!sSoundEntriesStore.LookupEntry(soundid))
    {
        sLog->outError("EffectPlayMusic: Sound (Id: %u) not exist in spell %u.",soundid,m_spellInfo->Id);
        return;
    }

    WorldPacket data(SMSG_PLAY_MUSIC, 4);
    data << uint32(soundid);
    unitTarget->ToPlayer()->GetSession()->SendPacket(&data);
}

void Spell::EffectSpecCount(SpellEffIndex /*effIndex*/)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    unitTarget->ToPlayer()->UpdateSpecCount(damage);
}

void Spell::EffectActivateSpec(SpellEffIndex /*effIndex*/)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    unitTarget->ToPlayer()->ActivateSpec(damage-1);  // damage is 1 or 2, spec is 0 or 1
}

void Spell::EffectPlayerNotification(SpellEffIndex /*effIndex*/)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    switch(m_spellInfo->Id)
    {
        case 58730: // Restricted Flight Area
        case 58600: // Restricted Flight Area
            unitTarget->ToPlayer()->GetSession()->SendNotification(LANG_ZONE_NOFLYZONE);
            unitTarget->PlayDirectSound(9417); // Fel Reaver sound
            break;
    }
}

void Spell::EffectDamageFromMaxHealthPCT(SpellEffIndex effIndex)
{
    if (!unitTarget)
        return;

    m_damage += unitTarget->CountPctFromMaxHealth(damage);
}

void Spell::EffectRemoveAura(SpellEffIndex effIndex)
{
    if (!unitTarget)
        return;
    // there may be need of specifying casterguid of removed auras
    unitTarget->RemoveAurasDueToSpell(m_spellInfo->EffectTriggerSpell[effIndex]);
}

void Spell::EffectCastButtons(SpellEffIndex effIndex)
{
    if (!unitTarget || m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player *p_caster = (Player*)m_caster;
    uint32 button_id = m_spellInfo->EffectMiscValue[effIndex] + 132;
    uint32 n_buttons = m_spellInfo->EffectMiscValueB[effIndex];

    for (; n_buttons; n_buttons--, button_id++)
    {
        ActionButton const* ab = p_caster->GetActionButton(button_id);
        if (!ab || ab->GetType() != ACTION_BUTTON_SPELL)
            continue;

        uint32 spell_id = ab->GetAction();
        if (!spell_id)
            continue;

        SpellEntry const *spellInfo = sSpellStore.LookupEntry(spell_id);
        if (!spellInfo)
            continue;

        if (!p_caster->HasSpell(spell_id) || p_caster->HasSpellCooldown(spell_id))
            continue;

        // It may occur, that the same spell is selected
        // when we do it, it would cause indefinite recursion
        if (spell_id == m_spellInfo->Id)
            continue;

        // Valid totem spells only have the first TotemCategory field set, so only check this
        if (p_caster->getClass() == CLASS_SHAMAN)
            if (spellInfo->TotemCategory[0] < TC_EARTH_TOTEM || spellInfo->TotemCategory[0] > TC_WATER_TOTEM)
                continue;

        uint32 cost = CalculatePowerCost(spellInfo, m_caster, GetSpellSchoolMask(spellInfo));

        if (m_caster->GetPower(POWER_MANA) < cost)
            break;

        m_caster->CastSpell(unitTarget, spell_id, true);
        m_caster->ModifyPower(POWER_MANA, -(int32)cost);
        p_caster->AddSpellAndCategoryCooldowns(spellInfo, 0);
    }
}

void Spell::EffectRechargeManaGem(SpellEffIndex /*effIndex*/)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    Player *player = m_caster->ToPlayer();

    if (!player)
        return;

    uint32 item_id = m_spellInfo->EffectItemType[0];

    ItemPrototype const *pProto = sObjectMgr->GetItemPrototype(item_id);
    if (!pProto)
    {
        player->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, NULL, NULL);
        return;
    }

    if (Item* pItem = player->GetItemByEntry(item_id))
    {
        for (int x = 0; x < MAX_ITEM_PROTO_SPELLS; ++x)
            pItem->SetSpellCharges(x,pProto->Spells[x].SpellCharges);
        pItem->SetState(ITEM_CHANGED,player);
    }
}

void Spell::EffectActivateGuildBankSlot(SpellEffIndex effIndex)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
        return;

    // Some organization
    Player* pPlayer = m_caster->ToPlayer();
    if (!pPlayer || pPlayer->GetGuildId() == 0)
        return;

    // If not in guild, gtfo
    Guild* pGuild = sObjectMgr->GetGuildById(pPlayer->GetGuildId());
    if (!pGuild)
        return;

    // Finally.. buy it!
    // (minus 1 because of buying 7th slot which is in fact at 6th position in 0..6 array)
    pGuild->HandleBuyBankTab(pPlayer->GetSession(), GetSpellInfo()->EffectBasePoints[effIndex]-1);
}

void Spell::EffectBind(SpellEffIndex effIndex)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* player = unitTarget->ToPlayer();

    uint32 area_id;
    WorldLocation loc;
    if (m_spellInfo->EffectImplicitTargetA[effIndex] == TARGET_DST_DB || m_spellInfo->EffectImplicitTargetB[effIndex] == TARGET_DST_DB)
    {
        SpellTargetPosition const* st = sSpellMgr->GetSpellTargetPosition(m_spellInfo->Id);
        if (!st)
        {
            sLog->outError("Spell::EffectBind - unknown teleport coordinates for spell ID %u", m_spellInfo->Id);
            return;
        }

        loc.m_mapId       = st->target_mapId;
        loc.m_positionX   = st->target_X;
        loc.m_positionY   = st->target_Y;
        loc.m_positionZ   = st->target_Z;
        loc.m_orientation = st->target_Orientation;
        area_id = player->GetAreaId();
    }
    else
    {
        player->GetPosition(&loc);
        area_id = player->GetAreaId();
    }

    player->SetHomebind(loc, area_id);

    // binding
    WorldPacket data(SMSG_BINDPOINTUPDATE, (4+4+4+4+4));
    data << float(loc.m_positionX);
    data << float(loc.m_positionY);
    data << float(loc.m_positionZ);
    data << uint32(loc.m_mapId);
    data << uint32(area_id);
    player->SendDirectMessage(&data);

    sLog->outStaticDebug("New homebind X      : %f", loc.m_positionX);
    sLog->outStaticDebug("New homebind Y      : %f", loc.m_positionY);
    sLog->outStaticDebug("New homebind Z      : %f", loc.m_positionZ);
    sLog->outStaticDebug("New homebind MapId  : %u", loc.m_mapId);
    sLog->outStaticDebug("New homebind AreaId : %u", area_id);

    // zone update
    data.Initialize(SMSG_PLAYERBOUND, 8+4);
    data << uint64(player->GetGUID());
    data << uint32(area_id);
    player->SendDirectMessage( &data );
}
