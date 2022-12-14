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

#ifndef TRINITY_DBCSFRM_H
#define TRINITY_DBCSFRM_H

const char Achievementfmt[]="niixsxiixixxii";
//const std::string CustomAchievementfmt="pppaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaapapaaaaaaaaaaaaaaaaaapp";
//const std::string CustomAchievementIndex = "ID";
const char AchievementCriteriafmt[]="niiiixiiiisiiiiixiiiiii";
const char AreaTableEntryfmt[] = "iiinixxxxxisiiiiiffixxxxxx";
const char AreaGroupEntryfmt[]="niiiiiii";
const char AreaPOIEntryfmt[]="niiiiiiiiiiiffixixxixx";
const char AreaTriggerEntryfmt[]="nifffxxxfffff";
const char ArmorLocationfmt[]="nfffff";
const char AuctionHouseEntryfmt[]="niiix";
const char BankBagSlotPricesEntryfmt[]="ni";
const char BarberShopStyleEntryfmt[]="nixxxiii";
const char BattlemasterListEntryfmt[]="niiiiiiiiixsiiiiiiii";
const char CharStartOutfitEntryfmt[]="diiiiiiiiiiiiiiiiiiiiiiiiixxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
const char CharTitlesEntryfmt[]="nxsxix";
const char ChatChannelsEntryfmt[]="nixsx";
const char ChrClassesEntryfmt[]="nixsxxxixiixxx";
const char ChrRacesEntryfmt[]="nxixiixixxxxixsxxxxxixxx";
const char ChrClassesXPowerTypesfmt[]="nii";
const char CinematicSequencesEntryfmt[]="nxxxxxxxxx";
const char CreatureDisplayInfofmt[]="nxxxfxxxxxxxxxxxx";
const char CreatureFamilyfmt[]="nfifiiiiixsx";
const char CreatureSpellDatafmt[]="niiiixxxx";
const char CreatureTypefmt[]="nxx";
const char CurrencyTypesfmt[]="nisxxxiiiix";
const char DestructibleModelDatafmt[]="nxxixxxixxxixxxixxx";
const char DurabilityCostsfmt[]="niiiiiiiiiiiiiiiiiiiiiiiiiiiii";
const char DurabilityQualityfmt[]="nf";
const char DungeonEncountersfmt[]="niiiisii";
const char EmotesEntryfmt[]="nxxiiixx";
const char EmotesTextEntryfmt[]="nxixxxxxxxxxxxxxxxx";
const char FactionEntryfmt[]="niiiiiiiiiiiiiiiiiiffixsxx";
const char FactionTemplateEntryfmt[]="niiiiiiiiiiiii";
const char GameObjectDisplayInfofmt[]="nsxxxxxxxxxxffffffxxx";
const char GemPropertiesEntryfmt[]="nixxii";
const char GlyphPropertiesfmt[]="niii";
const char GlyphSlotfmt[]="nii";
const char GtBarberShopCostBasefmt[]="xf";
const char GtCombatRatingsfmt[]="xf";
const char GtChanceToMeleeCritBasefmt[]="xf";
const char GtChanceToMeleeCritfmt[]="xf";
const char GtChanceToSpellCritBasefmt[]="xf";
const char GtChanceToSpellCritfmt[]="xf";
const char GtOCTBaseHPByClassfmt[]="nf";
const char GtOCTBaseMPByClassfmt[]="nf";
const char GtOCTClassCombatRatingScalarfmt[]="nf";
const char GtOCTHpPerStaminafmt[]="nf";
const char GtOCTRegenHPfmt[]="xf";
//const char GtOCTRegenMPfmt[]="f";
const char GtRegenHPPerSptfmt[]="xf";
const char GtRegenMPPerSptfmt[]="xf";
const char GtSpellScalingfmt[]="nf";
const char Holidaysfmt[] = "niiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiixxsiix";
//const char Itemfmt[]="niiiiiii";
const char ItemArmorQualityfmt[]="nfffffffi";
const char ItemArmorShieldfmt[]="nifffffff";
const char ItemArmorTotalfmt[]="niffff";
const char ImportPriceArmorfmt[]="nffff";
const char ImportPriceQualityfmt[]="nf";
const char ImportPriceShieldfmt[]="nf";
const char ImportPriceWeaponfmt[]="nf";
const char ItemPriceBasefmt[]="diff";
const char ItemBagFamilyfmt[]="nx";
const char ItemClassfmt[]="dixxfx";
//const char ItemDisplayTemplateEntryfmt[]="nxxxxxxxxxxixxxxxxxxxxx";
//const char ItemCondExtCostsEntryfmt[]="xiii";
const char ItemDamagefmt[]="nfffffffi";
const char ItemLimitCategoryEntryfmt[]="nxii";
const char ItemRandomPropertiesfmt[]="nxiiiiis";
const char ItemRandomSuffixfmt[]="nsxiiiiiiiiii";
const char ItemReforgefmt[]="nifif";
const char ItemSetEntryfmt[]="dsiiiiiiiiiixxxxxxxiiiiiiiiiiiiiiiiii"; /// "dsxxxxxxxxxxxxxxxxxiiiiiiiiiiiiiiiiii"
const char LFGDungeonEntryfmt[]="nxiiiiiiixixxixixxxxx";
const char LiquidTypeEntryfmt[] = "nxxixixxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
const char LockEntryfmt[]="niiiiiiiiiiiiiiiiiiiiiiiixxxxxxxx";
const char MailTemplateEntryfmt[]="nxs";
const char MapEntryfmt[]="nxixxxsixxixiffxiixx";
const char MapDifficultyEntryfmt[]="diisiix";
const char MovieEntryfmt[]="nxxx";
const char MountCapabilityfmt[]="niixxiii";
const char MountTypefmt[]="niiiiiiiiiiiiiiiiixxxxxxx";
const char NumTalentsAtLevelfmt[]="nf";
const char OverrideSpellDatafmt[]="niiiiiiiiiixx";
const char QuestFactionRewardfmt[]="niiiiiiiiii";
const char QuestSortEntryfmt[]="nx";
const char QuestXPfmt[]="niiiiiiiiii";
const char Phasefmt[]="nsi";
const char PvPDifficultyfmt[]="diiiii";
const char RandomPropertiesPointsfmt[]="niiiiiiiiiiiiiii";
const char ScalingStatDistributionfmt[]="niiiiiiiiiiiiiiiiiiiiii";
const char ScalingStatValuesfmt[]="iniiiiiiiiiiiiiiiiiiiixxxxxxxxxxxxxxxxxxxxxxxxx";
const char SkillLinefmt[]="nisxixi";
const char SkillLineAbilityfmt[]="niiiixxiiiiiix";
const char SoundEntriesfmt[]="nxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
const char ResearchBranchfmt[]="nsxixi";
const char ResearchProjects_Truefmt[]="nsxxiiixi";
const char ResearchProjectsfmt[]     ="nsxxiiixii";
const char ResearchSitesfmt[]="nixsx";
const char SpellCastTimefmt[]="nixx";
const std::string CustomSpellCastTimeEntryfmt="ppaa";
const std::string CustomSpellCastTimeEntryIndex="id";
const char SpellDifficultyfmt[]="niiii";
const std::string CustomSpellDifficultyfmt="ppppp";
const std::string CustomSpellDifficultyIndex="id";
const char SpellDurationfmt[]="niii";
const std::string CustomDurationEntryfmt="pppp";
const std::string CustomDurationEntryIndex="id";
//const std::string CustomSpellEntryfmt="pppppppppaappapaaaaaaaaapaaaaaaaaaaaaaaaaaaaaaappppaaaaaaaaapaappppappppaaappppppppppppppppppaaapppaaapppppppppaaappppppppppppppppppppppppaaaaaappppaapppaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaappppapaaaa";
const char True_SpellEntryfmt[]=      "iiiiiiiiiiiiiiiifiiiisxxxiixxifiiiiiiixiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiifffiiiiiiiiifffiiiiiiiiiiiiiiiiiifffiiifffiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiffffffffffiiiiiiiiiii";
const std::string CustomSpellEntryIndex = "Id";
const char SpellEntryfmt[]=           "niiiiiiiiiiiiiiifiiiisxxxiixxifiiiiiiixiiiiiiiii";
const std::string CustomSpellEntryfmt="ppppppppppapppppppppaaaappaapapppppppappppppppa";
const char SpellAuraOptionsfmt[]="niiii";
const std::string CustomSpellAuraOptionsEntryfmt="ppppp";
const std::string CustomSpellAuraOptionsEntryIndex="id";
const char SpellAuraRestrictionsfmt[]="niiiiiiii";
const std::string CustomSpellAuraRestrictionsEntryfmt="ppppppppp";
const std::string CustomSpellAuraRestrictionsEntryIndex="id";
const char SpellCastingRequirementsfmt[]="nixxixi";
const std::string CustomSpellCastingRequirementsEntryfmt="ppaapap";
const std::string CustomSpellCastingRequirementsEntryIndex="id";
const char SpellCategoriesfmt[]="niiiiii";
const std::string CustomSpellCategoriesEntryfmt="ppppppp";
const std::string CustomSpellCategoriesEntryIndex="id";
const char SpellClassOptionsfmt[]="nxiiiix";
const std::string CustomSpellClassOptionsEntryfmt="pappppa";
const std::string CustomSpellClassOptionsEntryIndex="id";
const char SpellCooldownsfmt[]="niii";
const std::string CustomSpellCooldownsEntryfmt="pppp";
const std::string CustomSpellCooldownsEntryIndex="id";
const char SpellEffectfmt[]="nifiiiffiiiiiifiifiiiiiiiii";
const std::string CustomSpellEffectEntryfmt="ppppppapppppppppapppppppppa";
const std::string CustomSpellEffectEntryIndex="id";
const char SpellEquippedItemsfmt[]="niii";
const std::string CustomSpellEquippedItemsEntryfmt="pppp";
const std::string CustomSpellEquippedItemsEntryIndex="id";
const char SpellInterruptsfmt[]="nixixi";
const std::string CustomSpellInterruptsEntryfmt="ppapap";
const std::string CustomSpellInterruptsEntryIndex="id";
const char SpellLevelsfmt[]="niii";
const std::string CustomSpellLevelsEntryfmt="pppp";
const std::string CustomSpellLevelsEntryIndex="id";
const char SpellPowerfmt[]="niiiixxx";
const std::string CustomSpellPowerEntryfmt="pppppaaa";
const std::string CustomSpellPowerEntryIndex="id";
const char SpellReagentsfmt[]="niiiiiiiiiiiiiiii";
const std::string CustomSpellReagentsEntryfmt="ppppppppppppppppp";
const std::string CustomSpellReagentsEntryIndex="id";
const char SpellScalingfmt[]="niiiiffffffffffi";
const std::string CustomSpellScalingEntryfmt="nppppppppppppppp";
const std::string CustomSpellScalingEntryIndex="id";
const char SpellShapeshiftfmt[]="nixixx";
const std::string CustomSpellShapeshiftEntryfmt="ppapaa";
const std::string CustomSpellShapeshiftEntryIndex="id";
const char SpellTargetRestrictionsfmt[]="nxiiii";
const std::string CustomSpellTargetRestrictionsEntryfmt="ppppp";
const std::string CustomSpellTargetRestrictionsEntryIndex="id";
const char SpellTotemsfmt[]="niiii";
const std::string CustomSpellTotemsEntryfmt="ppppp";
const std::string CustomSpellTotemsEntryIndex="id";
const char SpellFocusObjectfmt[]="nx";
const char SpellItemEnchantmentfmt[]="nxiiiiiixxxiiisiiiiiiix";
const char SpellItemEnchantmentConditionfmt[]="nbbbbbxxxxxbbbbbbbbbbiiiiiXXXXX";
const char SpellRadiusfmt[]="nfff";
const char SpellRangefmt[]="nffffixx";
const char SpellRuneCostfmt[]="niiii";
const char SpellShapeshiftFormfmt[]="nxxiixiiixxiiiiiiiiix";
const char SummonPropertiesfmt[] = "niiiii";
const char GuildPerksfmt[] = "nii";
const char TalentEntryfmt[]="niiiiiiiiixxixxxxxx";
const char TalentTabEntryfmt[]="nxxiiixxxii";
const char TalentTreePrimarySpellsfmt[]="niix";
const char TaxiNodesEntryfmt[]="nifffsiiixx";
const char TaxiPathEntryfmt[]="niii";
const char TaxiPathNodeEntryfmt[]="diiifffiiii";
const char TotemCategoryEntryfmt[]="nxii";
char const TransportAnimationfmt[] = "diifffx";
char const TransportRotationfmt[] = "diiffff";
const char UnitPowerBarEntryfmt[]="nxiiixxxxxxxxxxxxxxxxsxxxxx";
const char VehicleEntryfmt[]="niffffiiiiiiiifffffffffffffffssssfifiixx";
const char VehicleSeatEntryfmt[]="niiffffffffffiiiiiifffffffiiifffiiiiiiiffiiiiixxxxxxxxxxxxxxxxxxxx";
const char WMOAreaTableEntryfmt[]="niiixxxxxiixxxx";
const char WorldMapAreaEntryfmt[]="xinxffffixxxii";
const char WorldMapOverlayEntryfmt[]="nxiiiixxxxxxxxx";
const char WorldSafeLocsEntryfmt[]="nifffx";

#endif
