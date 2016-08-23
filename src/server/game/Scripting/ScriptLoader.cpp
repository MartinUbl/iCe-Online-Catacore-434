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
#include "ScriptPCH.h"
#include "ScriptLoader.h"


//examples
void AddSC_example_creature();
void AddSC_example_escort();
void AddSC_example_gossip_codebox();
void AddSC_example_misc();
void AddSC_example_commandscript();

// spells
void AddSC_deathknight_spell_scripts();
void AddSC_druid_spell_scripts();
void AddSC_generic_spell_scripts();
void AddSC_hunter_spell_scripts();
void AddSC_mage_spell_scripts();
void AddSC_paladin_spell_scripts();
void AddSC_priest_spell_scripts();
void AddSC_rogue_spell_scripts();
void AddSC_shaman_spell_scripts();
void AddSC_warlock_spell_scripts();
void AddSC_warrior_spell_scripts();
void AddSC_quest_spell_scripts();
void AddSC_item_spell_scripts();
void AddSC_example_spell_scripts();

void AddSC_SmartSCripts();

#ifdef SCRIPTS
//world
void AddSC_areatrigger_scripts();
void AddSC_boss_emeriss();
void AddSC_boss_taerar();
void AddSC_boss_ysondre();
void AddSC_darkmoon_island();
void AddSC_generic_creature();
void AddSC_go_scripts();
void AddSC_guards();
void AddSC_guards_swog();
void AddSC_item_scripts();
void AddSC_npc_professions();
void AddSC_npc_innkeeper();
void AddSC_npc_spell_click_spells();
void AddSC_npcs_special();
void AddSC_questline_fangs_of_the_father();
void AddSC_npc_taxi();
void AddSC_achievement_scripts();
void AddSC_bf_commandscript();

//////////////////////////////////
// VANILLA - dungeons and raids //
//////////////////////////////////

// ***** KALIMDOR LOCATIONS ***** //
//********************************//

void AddSC_ashenvale();
void AddSC_azshara();
void AddSC_azuremyst_isle();
void AddSC_bloodmyst_isle();
void AddSC_darkshore();
void AddSC_desolace();
void AddSC_durotar();
void AddSC_dustwallow_marsh();
void AddSC_felwood();
void AddSC_feralas();
void AddSC_moonglade();
void AddSC_mulgore();
void AddSC_silithus();
void AddSC_stonetalon_mountains();
void AddSC_tanaris();
void AddSC_teldrassil();
void AddSC_the_barrens();
void AddSC_thousand_needles();
void AddSC_ungoro_crater();
void AddSC_winterspring();

// Cities
void AddSC_darnassus();
void AddSC_exodar();
void AddSC_orgrimmar();
void AddSC_thunder_bluff();

// ***** KALIMDOR DUNGEONS ***** //
//*******************************//

// Ragefire Chasm - 15-16
void AddSC_instance_ragefire_chasm();
void AddSC_boss_adarogg();
void AddSC_boss_dark_shaman_koranthal();
void AddSC_boss_slagmaw();
void AddSC_boss_lava_guard_gordoth();

// Wailing Caverns - 17-20
void AddSC_instance_wailing_caverns();
void AddSC_wailing_caverns();
void AddSC_boss_lady_anacondra();
void AddSC_boss_lord_cobrahn();
void AddSC_boss_kresh();
void AddSC_boss_lord_pythas();
void AddSC_boss_skum();
void AddSC_boss_lord_serpentis();
void AddSC_boss_verdan_the_everliving();
void AddSC_boss_mutanus_the_devourer();

// Blackfathom Deeps 22-25
void AddSC_instance_blackfathom_deeps();
void AddSC_blackfathom_deeps();
void AddSC_boss_ghamoo_ra();
void AddSC_boss_lady_sarevess();
void AddSC_boss_gelihast();
void AddSC_boss_lorgus_jett();
void AddSC_boss_baron_aquanis();
void AddSC_boss_kelris();
void AddSC_boss_old_serrakiss();
void AddSC_boss_aku_mai();

// Razorfen Kraul - 32-35
void AddSC_instance_razorfen_kraul();
void AddSC_razorfen_kraul();
void AddSC_boss_roogug();
void AddSC_boss_aggem_thorncurse();
void AddSC_boss_death_speaker_jargba();
void AddSC_boss_overlord_ramtusk();
void AddSC_boss_agathelos_the_raging();
void AddSC_boss_charlga_razorflank();

// Maraudon - 36-39
void AddSC_instance_maraudon();
void AddSC_boss_noxxion();
void AddSC_boss_razorlash();
void AddSC_boss_tinkerer_gizlock();
void AddSC_boss_lord_vyletongue();
void AddSC_boss_celebras_the_cursed();
void AddSC_boss_landslide();
void AddSC_boss_rotgrip();
void AddSC_boss_princess_theradras();

// Razorfen Downs - 42-45
void AddSC_instance_razorfen_downs();
void AddSC_razorfen_downs();
void AddSC_boss_tutenkash();
void AddSC_boss_mordresh_fire_eye();
void AddSC_boss_glutton();
void AddSC_boss_amnennar_the_coldbringer();

// Dire Maul - 44-47
void AddSC_instance_dire_maul();
void AddSC_boss_zevrim_thornhoof();
void AddSC_boss_hydrospawn();
void AddSC_boss_lethtendris();
void AddSC_boss_alzzin_the_wildshaper();
void AddSC_boss_guard_moldar();
void AddSC_boss_stomper_kreeg();
void AddSC_boss_guard_fengus();
void AddSC_boss_guard_slipkik();
void AddSC_boss_captain_kromcrush();
void AddSC_boss_king_gordok();
void AddSC_boss_chorush_the_observer();
void AddSC_boss_tendris_warpwood();
void AddSC_boss_illyanna_ravenoak();
void AddSC_boss_magister_kalendris();
void AddSC_boss_immolthar();
void AddSC_boss_prince_tortheldrin();

// Zul'Farrak - 46-49
void AddSC_instance_zulfarrak();
void AddSC_zulfarrak();
void AddSC_boss_antusul();
void AddSC_boss_theka_the_martyr();
void AddSC_boss_witch_doctor_zumrah();
void AddSC_boss_nekrum_gutchewer();
void AddSC_boss_hydromancer_velratha();
void AddSC_boss_gahzrilla();
void AddSC_boss_shadowpriest_sezzziz();
void AddSC_boss_ruuzlu();
void AddSC_boss_chief_ukorz_sandscalp();

// ***** KALIMDOR RAIDS ***** //
//****************************//

// Ruins of Ahn'Qiraj - 60
void AddSC_instance_ruins_of_ahnqiraj();
void AddSC_boss_kurinnaxx();
void AddSC_boss_rajaxx();
void AddSC_boss_moam();
void AddSC_boss_buru();
void AddSC_boss_ayamiss();
void AddSC_boss_ossirian();

// Temple of Ahn'Qiraj - 60
void AddSC_instance_temple_of_ahnqiraj();
void AddSC_boss_skeram();
void AddSC_boss_battleguard_sartura();
void AddSC_boss_fankriss();
void AddSC_boss_huhuran();
void AddSC_boss_twinemperors();
void AddSC_boss_cthun();
void AddSC_boss_bug_trio();
void AddSC_boss_viscidus();
void AddSC_boss_ouro();
void AddSC_mob_anubisath_sentinel();

// ***** EASTERN KINGDOM LOCATIONS ***** //
//***************************************//

void AddSC_arathi_highlands();
void AddSC_badlands();
void AddSC_blasted_lands();
void AddSC_burning_steppes();
void AddSC_deadwind_pass();
void AddSC_deeprun_tram();
void AddSC_dun_morogh();
void AddSC_duskwood();
void AddSC_eastern_plaguelands();
void AddSC_elwynn_forest();
void AddSC_eversong_woods();
void AddSC_ghostlands();
void AddSC_hillsbrad_foothills();
void AddSC_loch_modan();
void AddCS_plaguelands();
void AddSC_ruins_of_gilneas();
void AddSC_redridge_mountains();
void AddSC_searing_gorge();
void AddSC_silverpine_forest();
void AddSC_stranglethorn_vale();
void AddSC_swamp_of_sorrows();
void AddSC_the_cape_of_stranglethorn();
void AddSC_the_hinterlands();
void AddSC_tirisfal_glades();
void AddSC_western_plaguelands();
void AddSC_westfall();
void AddSC_wetlands();

// Cities
void AddSC_ironforge();
void AddSC_silvermoon_city();
void AddSC_stormwind_city();
void AddSC_undercity();

// ***** EASTERN KINGDOM DUNGEONS ***** //
//**************************************//

// Stockade - 22-25
void AddSC_instance_stockade();
void AddSC_boss_randolph_moloch();
void AddSC_boss_lord_overheat();
void AddSC_boss_hogger();

// Gnomeregan - 26-29
void AddSC_instance_gnomeregan();
void AddSC_gnomeregan();
void AddSC_boss_techbot();
void AddSC_boss_viscous_fallout();
void AddSC_boss_grubbis();
void AddSC_boss_electrocutioner_6000();
void AddSC_boss_crowd_pummeler();
void AddSC_boss_mekgineer_thermaplugg();

// Scarlet Monastery - 30-33
void AddSC_instance_scarlet_monastery();
void AddSC_boss_bloodmage_thalnos();
void AddSC_boss_arcanist_doan();
void AddSC_boss_azshir_the_sleepless();
void AddSC_boss_headless_horseman();
void AddSC_boss_herod();
void AddSC_boss_high_inquisitor_fairbanks();
void AddSC_boss_houndmaster_loksey();
void AddSC_boss_interrogator_vishas();
void AddSC_boss_scorn();
void AddSC_boss_mograine_and_whitemane();
void AddSC_the_scarlet_enclave();
void AddSC_the_scarlet_enclave_c1();
void AddSC_the_scarlet_enclave_c2();
void AddSC_the_scarlet_enclave_c5();

// Uldaman - 37-40
void AddSC_instance_uldaman();
void AddSC_uldaman();
void AddSC_boss_revelosh();
void AddSC_boss_baelog();
void AddSC_boss_ironaya();
void AddSC_boss_obsidian_sentinel();
void AddSC_boss_ancient_stone_keeper();
void AddSC_boss_galgann_firehammer();
void AddSC_boss_grimlok();
void AddSC_boss_archaedas();

// Scholomance - 40-43
void AddSC_instance_scholomance();
void AddSC_boss_jandice_barov();
void AddSC_boss_darkmaster_gandling();
void AddSC_boss_death_knight_darkreaver();
void AddSC_boss_theolen_krastinov();
void AddSC_boss_illucia_barov();
void AddSC_boss_instructor_malicia();
void AddSC_boss_kormok();
void AddSC_boss_lord_alexei_barov();
void AddSC_boss_lorekeeper_polkelt();
void AddSC_boss_ras_frostwhisper();
void AddSC_boss_the_ravenian();
void AddSC_boss_vectus();

// Stratholme - 48-51
void AddSC_instance_stratholme();
void AddSC_stratholme();
void AddSC_boss_the_unforgiven();
void AddSC_boss_hearthsinger_forresten();
void AddSC_boss_timmy_the_cruel();
void AddSC_boss_commander_malor();
void AddSC_boss_cannon_master_willey();
void AddSC_boss_instructor_galford();
void AddSC_boss_dathrohan_balnazzar();
void AddSC_boss_baroness_anastari();
void AddSC_boss_nerubenkan();
void AddSC_boss_maleki_the_pallid();
void AddSC_boss_magistrate_barthilas();
void AddSC_boss_ramstein_the_gorger();
void AddSC_boss_baron_rivendare();
void AddSC_boss_postmaster_malown();
void AddSC_boss_order_of_silver_hand();

// Sunken Temple - 52-55
void AddSC_instance_sunken_temple();
void AddSC_sunken_temple();
void AddSC_boss_jammalan_the_prophet();
void AddSC_boss_avatar_of_hakkar();
void AddSC_boss_shade_of_eranikus();

// Blackrock Depths - 53-56
void AddSC_instance_blackrock_depths();
void AddSC_blackrock_depths();
void AddSC_boss_lord_roccor();
void AddSC_boss_baelgar();
void AddSC_boss_houndmaster_grebmar();
void AddSC_boss_high_interrogator_gerstahn();
void AddSC_boss_high_justice_grimstone();
void AddSC_boss_gorosh_the_dervish();
void AddSC_boss_grizzle();
void AddSC_boss_eviscerator();
void AddSC_boss_okthor_the_breaker();
void AddSC_boss_anubshiah();
void AddSC_boss_hedrum_the_creeper();
void AddSC_boss_pyromancer_loregrain();
void AddSC_boss_general_angerforge();
void AddSC_boss_golem_lord_argelmach();
void AddSC_boss_ribbly_screwspigot();
void AddSC_boss_hurley_blackbreath();
void AddSC_boss_plugger_spazzring();
void AddSC_boss_phalanx();
void AddSC_boss_lord_incendius();
void AddSC_boss_fineous_darkvire();
void AddSC_boss_warden_stilgiss_and_verek();
void AddSC_boss_the_vault();
void AddSC_boss_ambassador_flamelash();
void AddSC_boss_tomb_of_seven();
void AddSC_boss_magmus();
void AddSC_boss_moira_bronzebeard();
void AddSC_boss_emperor_dragan_thaurissan();

// Blackrock Spire - 57-58
void AddSC_instance_blackrock_spire();
void AddSC_blackrock_spire();
void AddSC_boss_highlord_omokk();
void AddSC_boss_shadow_hunter_voshgajin();
void AddSC_boss_warmaster_voone();
void AddSC_boss_mother_smolderweb();
void AddSC_boss_urok_doomhowl();
void AddSC_boss_quatermaster_zigris();
void AddSC_boss_gizrul_the_slavener();
void AddSC_boss_halycon();
void AddSC_boss_overlord_wyrmthalak();
void AddSC_boss_pyroguard_emberseer();
void AddSC_boss_rend_blackhand();
void AddSC_boss_gyth();
void AddSC_boss_the_beast();
void AddSC_boss_drakkisath();

// ***** EASTERN KINGDOM RAIDS ***** //
//***********************************//

// Blackwing lair 60
void AddSC_instance_blackwing_lair();
void AddSC_boss_razorgore();
void AddSC_boss_vaelastrasz();
void AddSC_boss_broodlord_lashlayer();
void AddSC_boss_firemaw();
void AddSC_boss_ebonroc();
void AddSC_boss_flamegor();
void AddSC_boss_chromaggus();
void AddSC_boss_nefarian();
void AddSC_boss_victor_nefarius();

// Molten core - 60
void AddSC_instance_molten_core();
void AddSC_molten_core();
void AddSC_boss_lucifron();
void AddSC_boss_magmadar();
void AddSC_boss_gehennas();
void AddSC_boss_garr();
void AddSC_boss_shazzrah();
void AddSC_boss_baron_geddon();
void AddSC_boss_golemagg();
void AddSC_boss_sulfuron_harbinger();
void AddSC_boss_majordomo_exectus();
void AddSC_boss_ragnaros();

/////////////////////////////////////////////
// THE BURNING CRUSADE - dungeon and raids //
/////////////////////////////////////////////

// ***** OUTLAND LOCATIONS ****** //
//********************************//

void AddSC_blades_edge_mountains();
void AddSC_hellfire_peninsula();
void AddSC_nagrand();
void AddSC_netherstorm();
void AddSC_shadowmoon_valley();
void AddSC_terokkar_forest();
void AddSC_zangarmarsh();
void AddSC_isle_of_queldanas();

// City
void AddSC_shattrath_city();

// ***** OUTLAND DUNGEONS ***** //
//******************************//

// Hellfire Citacel: Hellfire Ramparts - 57-70 (70 HC)
void AddSC_instance_hellfire_ramparts();
void AddSC_boss_watchkeeper_gargolmar();
void AddSC_boss_omor_the_unscarred();
void AddSC_boss_vazruden_the_herald();

// Hellfire Citadel: The Blood Furnace - 58-70 (70 HC)
void AddSC_instance_blood_furnace();
void AddSC_boss_the_maker();
void AddSC_boss_broggok();
void AddSC_boss_kelidan_the_breaker();

// Hellfire Citadel: Shattered Halls - 65-70 (70 HC)
void AddSC_instance_shattered_halls();
void AddSC_boss_grand_warlock_nethekurse();
void AddSC_boss_blood_guard_porung();
void AddSC_boss_warbringer_omrogg();
void AddSC_boss_warchief_kargath_bladefist();

// Coilfang Reservoir: The Slave Pens - 59-70 (70 HC)
void AddSC_instance_slave_pens();
void AddSC_boss_mennu_the_betrayer();
void AddSC_boss_rokmar_the_crackler();
void AddSC_boss_quagmirran();

// Coilfang Reservoir: The Steamvault - 65-70 (70 HC)
void AddSC_instance_steam_vault();
void AddSC_boss_hydromancer_thespia();
void AddSC_boss_mekgineer_steamrigger();
void AddSC_boss_warlord_kalithresh();

// Coilfang Reservoir: The Underbog - 60-70 (70 HC)
void AddSC_instance_the_underbog();
void AddSC_boss_hungarfen();
void AddSC_boss_ghazan();
void AddSC_boss_swamplord_muselek();
void AddSC_boss_the_black_stalker();

// Auchindoun: Auchenai Crypts - 62-70 (70 HC)
void AddSC_instance_auchenai_crypts();
void AddSC_boss_shirrak_the_dead_watcher();
void AddSC_boss_exarch_maladaar();

// Auchindoun: Mana Tombs - 63-70 (70 HC)
void AddSC_instance_mana_tombs();
void AddSC_boss_pandemonius();
void AddSC_boss_tavarok();
void AddSC_boss_nexusprince_shaffar();
void AddSC_boss_yor();

// Auchindoun: Sethekk Halls - 63-70 (70 HC)
void AddSC_instance_sethekk_halls();
void AddSC_boss_darkweaver_syth();
void AddSC_boss_talon_king_ikiss();
void AddSC_boss_anzu();

// Auchindoun: Shadow Labyrinth - 65-70 (70 HC)
void AddSC_instance_shadow_labyrinth();
void AddSC_boss_ambassador_hellmaw();
void AddSC_boss_blackheart_the_inciter();
void AddSC_boss_grandmaster_vorpil();
void AddSC_boss_murmur();

// Tempest Keep: The Mechanar - 70 (70 HC)
void AddSC_instance_mechanar();
void AddSC_boss_mechano_lord_capacitus();
void AddSC_boss_gatewatcher_iron_hand();
void AddSC_boss_nethermancer_sepethrea();
void AddSC_boss_pathaleon_the_calculator();

// Tempest Keep: Botanica - 65-70 (70 HC)
void AddSC_instance_botanica();
void AddSC_boss_commander_sarannis();
void AddSC_boss_high_botanist_freywinn();
void AddSC_boss_thorngrin_the_tender();
void AddSC_boss_laj();
void AddSC_boss_warp_splinter();

// Tempest Keep: Arcatraz - 65-70 (70 HC)
void AddSC_instance_arcatraz();
void AddSC_arcatraz();
void AddSC_boss_zereketh_the_unbound();
void AddSC_boss_dalliah_the_doomsayer();
void AddSC_boss_wrath_scryer_soccothrates();
void AddSC_boss_harbinger_skyriss();

// Caverns of Time: Old Hillsbrad Foothills - 63-70 (70 HC)
void AddSC_instance_old_hillsbrad();
void AddSC_old_hillsbrad();
void AddSC_boss_lieutenant_drake();
void AddSC_boss_captain_skarloc();
void AddSC_boss_epoch_hunter();

// Caverns of Time: The Black Morass - 65-70 (70 HC)
void AddSC_instance_dark_portal();
void AddSC_dark_portal();
void AddSC_boss_chrono_lord_deja();
void AddSC_boss_temporus();
void AddSC_boss_aeonus();

// Magister's Terrace 65-70 (70 HC)
void AddSC_instance_magisters_terrace();
void AddSC_magisters_terrace();
void AddSC_boss_selin_fireheart();
void AddSC_boss_vexallus();
void AddSC_boss_priestess_delrissa();
void AddSC_boss_felblood_kaelthas();

// ***** OUTLAND RAIDS ***** //
//***************************//

// Karazhan - 70
void AddSC_instance_karazhan();
void AddSC_karazhan();
void AddSC_boss_servant_quarters();
void AddSC_boss_midnight();
void AddSC_boss_moroes();
void AddSC_bosses_opera();
void AddSC_boss_maiden_of_virtue();
void AddSC_boss_curator();
void AddSC_boss_chess_event();
void AddSC_boss_terestian_illhoof();
void AddSC_boss_shade_of_aran();
void AddSC_boss_netherspite();
void AddSC_boss_nightbane();
void AddSC_boss_prince_malchezaar();

// Gruul's Lair - 70
void AddSC_instance_gruuls_lair();
void AddSC_boss_gruul();
void AddSC_boss_high_king_maulgar();

// Hellfire Citadel: Magtheridon's Lair - 70
void AddSC_instance_magtheridons_lair();
void AddSC_boss_magtheridon();

// Tempest Keep: The Eye - 70
void AddSC_instance_the_eye();
void AddSC_the_eye();
void AddSC_boss_void_reaver();
void AddSC_boss_alar();
void AddSC_boss_high_astromancer_solarian();
void AddSC_boss_kaelthas();

// Coilfang Reservoir: Serpentshrine Cavern - 70
void AddSC_instance_serpentshrine_cavern();
void AddSC_boss_hydross_the_unstable();
void AddSC_boss_lurker_below();
void AddSC_boss_leotheras_the_blind();
void AddSC_boss_fathomlord_karathress();
void AddSC_boss_morogrim_tidewalker();
void AddSC_boss_lady_vashj();

// Caverns of Time: Hyjal Summit - 70
void AddSC_instance_hyjal_summit();
void AddSC_hyjal();
void AddSC_boss_rage_winterchill();
void AddSC_boss_anetheron();
void AddSC_boss_kazrogal();
void AddSC_boss_azgalor();
void AddSC_boss_archimonde();
void AddSC_hyjal_trash();

// Black Temple - 70
void AddSC_instance_black_temple();
void AddSC_black_temple();
void AddSC_boss_warlord_najentus();
void AddSC_boss_supremus();
void AddSC_boss_shade_of_akama();
void AddSC_boss_teron_gorefiend();
void AddSC_boss_gurtogg_bloodboil();
void AddSC_boss_reliquary_of_souls();
void AddSC_boss_mother_shahraz();
void AddSC_boss_illidari_council();
void AddSC_boss_illidan();

// Sunwell Plateau - 70
void AddSC_instance_sunwell_plateau();
void AddSC_sunwell_plateau();
void AddSC_boss_kalecgos();
void AddSC_boss_brutallus();
void AddSC_boss_felmyst();
void AddSC_boss_eredar_twins();
void AddSC_boss_muru();
void AddSC_boss_kiljaeden();

///////////////////////////////////////////////
// WRATH OF THE LICHKING - dungeon and raids //
///////////////////////////////////////////////

// ***** NORTHREND LOCATIONS ***** //
//*********************************//

void AddSC_borean_tundra();
void AddSC_crystalsong_forest();
void AddSC_dragonblight();
void AddSC_grizzly_hills();
void AddSC_howling_fjord();
void AddSC_icecrown();
void AddSC_sholazar_basin();
void AddSC_storm_peaks();
void AddSC_zuldrak();

// City
void AddSC_dalaran();

// ***** NORTHREND DUNGEONS ***** //
//********************************//

// Utgarde Keep: Utgarde Keep - 69-72 (80 HC)
void AddSC_instance_utgarde_keep();
void AddSC_utgarde_keep();
void AddSC_boss_keleseth();
void AddSC_boss_skarvald_dalronn();
void AddSC_boss_ingvar_the_plunderer();

// The Nexus: The Nexus - 71-73 (80 HC)
void AddSC_instance_nexus();
void AddSC_boss_magus_telestra();
void AddSC_boss_anomalus();
void AddSC_boss_ormorok();
void AddSC_boss_keristrasza();
void AddSC_boss_commander_stoutbeard();
void AddSC_boss_commander_kolurg();

// Azjol-Nerub: Azjol-Nerub - 72-74 (80 HC)
void AddSC_instance_azjol_nerub();
void AddSC_boss_krikthir_the_gatewatcher();
void AddSC_boss_hadronox();
void AddSC_boss_anubarak();

// Azjol-Nerub: Ahn'kahet - The Old Kingdom - 73-75 (80 HC)
void AddSC_instance_ahnkahet();
void AddSC_boss_elder_nadox();
void AddSC_boss_prince_taldaram();
void AddSC_boss_jedoga_shadowseeker();
void AddSC_boss_herald_volazj();
void AddSC_boss_amanitar();

// Drak'Tharon Keep - 74-76 (80 HC)
void AddSC_instance_drak_tharon_keep();
void AddSC_boss_trollgore();
void AddSC_boss_novos();
void AddSC_boss_dred();
void AddSC_boss_tharon_ja();

// The Violet Hold - 75-77 (80 HC)
void AddSC_instance_violet_hold();
void AddSC_violet_hold();
void AddSC_boss_erekem();
void AddSC_boss_moragg();
void AddSC_boss_ichoron();
void AddSC_boss_xevozz();
void AddSC_boss_lavanthor();
void AddSC_boss_zuramat();
void AddSC_boss_cyanigosa();

// Gundrak - 76-78 (80 HC)
void AddSC_instance_gundrak();
void AddSC_boss_slad_ran();
void AddSC_boss_drakkari_colossus();
void AddSC_boss_moorabi();
void AddSC_boss_gal_darah();
void AddSC_boss_eck();

// Ulduar: Halls of Stone - 77-78 (80 HC)
void AddSC_instance_halls_of_stone();
void AddSC_halls_of_stone();
void AddSC_boss_maiden_of_grief();
void AddSC_boss_krystallus();
void AddSC_boss_tribunal_of_the_ages();
void AddSC_boss_sjonnir();

// Crusaders' Coliseum: Trial of the Champion - 78-80 (80 HC)
void AddSC_instance_trial_of_the_champion();
void AddSC_trial_of_the_champion();
void AddSC_boss_grand_champions();
void AddSC_boss_argent_challenge();
void AddSC_boss_black_knight();

// Caverns of Time: The Culling of Stratholme - 79-80 (80 HC)
void AddSC_instance_culling_of_stratholme();
void AddSC_culling_of_stratholme();
void AddSC_boss_meathook();
void AddSC_boss_salramm();
void AddSC_boss_epoch();
void AddSC_boss_mal_ganis();
void AddSC_boss_infinite_corruptor();

// The Nexus: The Oculus - 79-80 (80 HC)
void AddSC_instance_oculus();
void AddSC_oculus();
void AddSC_boss_drakos();
void AddSC_boss_varos_cloudstrider();
void AddSC_boss_urom();
void AddSC_boss_ley_guardian_eregos();

// Utgarde Keep: Utgarde Pinnacle - 79-80 (80 HC)
void AddSC_instance_utgarde_pinnacle();
void AddSC_boss_svala();
void AddSC_boss_palehoof();
void AddSC_boss_skadi();
void AddSC_boss_ymiron();

// Ulduar: Halls of Lightning - 79-80 (80 HC)
void AddSC_instance_halls_of_lightning();
void AddSC_boss_bjarngrim();
void AddSC_boss_volkhan();
void AddSC_boss_ionar();
void AddSC_boss_loken();

// Icecrown Citadel: Pit of Saron - 80 (80 HC)
void AddSC_instance_pit_of_saron();
void AddSC_pit_of_saron();
void AddSC_boss_forgemaster_garfrost();
void AddSC_boss_krick_and_ick();
void AddSC_boss_scourgelord_tyrannus();

// Icecrown Citadel: Forge of Souls - 80 (80 HC)
void AddSC_instance_forge_of_souls();
void AddSC_forge_of_souls();
void AddSC_boss_bronjahm();
void AddSC_boss_devourer_of_souls();

// Icecrown Citadel: Halls of Reflection - 80 (80 HC)
void AddSC_instance_halls_of_reflection();
void AddSC_halls_of_reflection();
void AddSC_boss_falric();
void AddSC_boss_marwyn();
void AddSC_boss_wrath_of_the_lich_king();

// ***** NORTHREND RAIDS ***** //
//*****************************//

// Crusaders' Coliseum: Trial of the Crusader - 80 (80 HC)
void AddSC_instance_trial_of_the_crusader();
void AddSC_trial_of_the_crusader();
void AddSC_boss_northrend_beasts();
void AddSC_boss_lord_jaraxxus();
void AddSC_boss_faction_champions();
void AddSC_boss_twin_valkyr();
void AddSC_boss_anubarak_trial();

// The Nexus: The Eye of Eternity - 80
void AddSC_instance_eye_of_eternity();
void AddSC_boss_malygos();

// Onyxia's Lair - 80
void AddSC_instance_onyxias_lair();
void AddSC_boss_onyxia();

// Obsidian Sanctum - 80
void AddSC_instance_obsidian_sanctum();
void AddSC_boss_sartharion();

// Ruby Sanctum - 80
void AddSC_instance_ruby_sanctum();
void AddSC_boss_baltharus();
void AddSC_boss_zarithrian();
void AddSC_boss_ragefire();
void AddSC_boss_halion();

// Vault of Archavon - 80
void AddSC_instance_vault_of_archavon();
void AddSC_boss_archavon();
void AddSC_boss_emalon();
void AddSC_boss_koralon();
void AddSC_boss_toravon();

// Naxxramas - 80
void AddSC_instance_naxxramas();
void AddSC_boss_anubrekhan();
void AddSC_boss_faerlina();
void AddSC_boss_maexxna();
void AddSC_boss_noth();
void AddSC_boss_heigan();
void AddSC_boss_loatheb();
void AddSC_boss_razuvious();
void AddSC_boss_gothik();
void AddSC_boss_four_horsemen();
void AddSC_boss_patchwerk();
void AddSC_boss_grobbulus();
void AddSC_boss_gluth();
void AddSC_boss_thaddius();
void AddSC_boss_sapphiron();
void AddSC_boss_kelthuzad();

// Ulduar: Ulduar - 80
void AddSC_instance_ulduar();
void AddSC_boss_flame_leviathan();
void AddSC_boss_ignis();
void AddSC_boss_hodir();
void AddSC_boss_razorscale();
void AddSC_boss_xt002();
void AddSC_boss_assembly_of_iron();
void AddSC_boss_kologarn();
void AddSC_boss_auriaya();
void AddSC_boss_freya();
void AddSC_boss_mimiron();
void AddSC_boss_thorim();
void AddSC_boss_general_vezax();
void AddSC_boss_yogg_saron();
void AddSC_boss_algalon_the_observer();
void AddSC_ulduar_teleporter();

// Icecrown Citadel - 80
void AddSC_instance_icecrown_citadel();
void AddSC_boss_lord_marrowgar();
void AddSC_boss_lady_deathwhisper();
void AddSC_boss_gunship_battle();
void AddSC_boss_deathbringer_saurfang();
void AddSC_boss_festergut();
void AddSC_boss_rotface();
void AddSC_boss_professor_putricide();
void AddSC_boss_blood_prince_council();
void AddSC_boss_blood_queen_lanathel();
void AddSC_boss_valithria_dreamwalker();
void AddSC_boss_sindragosa();
void AddSC_boss_lich_king();
void AddSC_icecrown_citadel_teleport();

////////////////////////////////////////////////
///////// CATACLYSM dungeons and raids /////////
////////////////////////////////////////////////

// ***** CATACLYSM ZONES ***** //
//*****************************//

void AddSC_deepholm();
void AddSC_gilneas();
void AddSC_kezan();
void AddSC_lost_isles();
void AddSC_mount_hyjal();
void AddSC_tol_barad();
void AddSC_twilight_highlands();
void AddSC_uldum();
void AddSC_vashjir();

// ***** CATACLYSM DUNGEONS ***** //
//********************************//

// Blackrock Caverns - 80-82 (85 HC)
void AddSC_instance_blackrock_caverns();
void AddSC_boss_romogg();
void AddSC_boss_corla();
void AddSC_boss_karsh();
void AddSC_boss_beauty();
void AddSC_boss_obsidius();

// Throne of the Tides - 80-82 (85 HC)
void AddSC_instance_throne_of_the_tides();
void AddSC_boss_lady_nazjar();
void AddSC_boss_commander_ulthok();
void AddSC_boss_mindbender_ghursha();
void AddSC_boss_ozumat();

// Stonecore - 82-84 (85 HC)
void AddSC_instance_the_stonecore();
void AddSC_boss_corborus();
void AddSC_boss_slabhide();
void AddSC_boss_ozruk();
void AddSC_boss_high_priestess_azil();

// Vortex Pinnacle - 82-84 (85 HC)
void AddSC_instance_vortex_pinnacle();
void AddSC_boss_ertan();
void AddSC_boss_altairus();
void AddSC_boss_asaad();

// Grim Batol - 84-85 (85 HC)
void AddSC_instance_grim_batol();
void AddSC_boss_general_umbriss();
void AddSC_boss_forgemaster_throngus();
void AddSC_boss_drahga_shadowburner();
void AddSC_boss_erudax_husam();

// Lost City of Tol`Vir - 84-85 (85 HC)
void AddSC_instance_lost_city_of_the_tolvir();
void AddSC_boss_general_husam();
void AddSC_boss_high_prophet_barim();
void AddSC_boss_lockmaw_and_augh();
void AddSC_boss_siamat();

// Halls of Origination - 85 (85 HC)
void AddSC_instance_halls_of_origination();
void AddSC_boss_temple_guardian_anhuur();
void AddSC_boss_earthrager_ptah();
void AddSC_boss_anraphet();
void AddSC_boss_isiset();
void AddSC_boss_ammunae();
void AddSC_boss_setesh();
void AddSC_boss_rajh();

// Deadmines - 15-16 (85 HC)
void AddSC_instance_deadmines();
void AddSC_deadmines();
void AddSC_boss_mr_smite();
void AddSC_boss_glubtok();
void AddSC_boss_helix_gearbreaker();
void AddSC_boss_foe_reaper_5000();
void AddSC_boss_admiral_ripsnarl();
void AddSC_boss_captain_cookie();
void AddSC_boss_vanessa_vancleef();

// Shadowfang Keep - 18-21 (85 HC)
void AddSC_instance_shadowfang_keep();
void AddSC_shadowfang_keep();
void AddSC_boss_baron_ashbury();
void AddSC_boss_baron_silverlaine();
void AddSC_boss_commander_springvale();
void AddSC_boss_lord_walden();
void AddSC_boss_lord_godfrey();

// Zul`Gurub - 85
void AddSC_instance_zulgurub();
void AddSC_boss_high_priestess_venoxis();
void AddSC_boss_bloodlord_mandokir();
void AddSC_boss_grilek();
void AddSC_boss_hazzarah();
void AddSC_boss_renataki();
void AddSC_boss_wushoolay();
void AddSC_boss_high_priestess_kilnara();
void AddSC_boss_zanzil();
void AddSC_boss_jindo_the_godbreaker();

// Zul'Aman - 85
void AddSC_instance_zulaman();
void AddSC_zulaman();
void AddSC_boss_akilzon();
void AddSC_boss_nalorakk();
void AddSC_boss_janalai();
void AddSC_boss_halazzi();
void AddSC_boss_hex_lord_malacrass();
void AddSC_boss_zuljin();

// End Time - 85
void AddSC_instance_end_time();
void AddSC_boss_echo_of_jaina();
void AddSC_boss_echo_of_baine();
void AddSC_boss_echo_of_sylvanas();
void AddSC_boss_echo_of_tyrande();
void AddSC_boss_murozond();

// Well of Eternity - 85
void AddSC_instance_well_of_eternity();
void AddSC_boss_perotharn();
void AddSC_boss_queen_azshara();
void AddSC_boss_mannoroth();
void AddSC_mannoroth_intro();
void AddSC_well_of_eternity_trash();
void AddSC_woe_aspects_event();

// Hour of Twilight - 85
void AddSC_instance_hour_of_twilight();
void AddSC_boss_arcurion();
void AddSC_boss_asira_dawnslayer();
void AddSC_boss_archbishop_benedictus();

// ***** CATACLYSM RAIDS ***** //
//*****************************//

// Baradin Hold - 85
void AddSC_instance_baradin_hold();
void AddSC_boss_argaloth();
void AddSC_boss_occuthar();
void AddSC_boss_alizabal();

// The Bastion of Twilight - 85
void AddSC_instance_bastion_of_twilight();
void AddSC_boss_halfus_wyrmbreaker();
void AddSC_boss_valiona_and_theralion();
void AddSC_boss_twilight_council();
void AddSC_boss_chogall();
void AddSC_boss_sinestra();

// Blackwing Descent - 85
void AddSC_instance_blackwing_descent();
void AddSC_boss_magmaw();
void AddSC_boss_omnotron_defensing_system();
void AddSC_boss_chimaeron();
void AddSC_boss_maloriak();
void AddSC_boss_atramedes();
void AddSC_boss_lord_nefarian();

// Throne of the Four Winds - 85
void AddSC_instance_throne_of_the_four_winds();
void AddSC_boss_conclave_of_wind();
void AddSC_boss_alakir();

// Firelands - 85
void AddSC_instance_firelands();
void AddSC_boss_shannox();
void AddSC_boss_bethtilac();
void AddSC_boss_lord_rhyolith();
void AddSC_boss_alysrazor();
void AddSC_boss_baeloroc();
void AddSC_boss_majordomo_staghelm();
void AddSC_boss_ragnaros_fl();

// Dragon Soul - 85
void AddSC_instance_dragonsoul();
void AddSC_dragon_soul_trash();
void AddSC_dragon_soul_aspects();
void AddSC_boss_morchok();
void AddSC_boss_warlord_zonozz();
void AddSC_boss_yorsahj_the_unsleeping();
void AddSC_boss_hagara_the_stormbinder();
void AddSC_boss_ultraxion();
void AddSC_boss_warmaster_blackhorn();
void AddSC_boss_spine_of_deathwing();
void AddSC_boss_madness_of_deathwing();

////////////////////////////////////////////////
////////////////// PVP ZONES ///////////////////
////////////////////////////////////////////////

// Wintergrasp
void AddSC_wintergrasp();

// Isle of Conquest
void AddSC_isle_of_conquest();

// Alterac Valley
void AddSC_alterac_valley();
void AddSC_boss_balinda();
void AddSC_boss_drekthar();
void AddSC_boss_galvangar();
void AddSC_boss_vanndar();

// Outdoor Zones
void AddSC_outdoorpvp_ep();
void AddSC_outdoorpvp_hp();
void AddSC_outdoorpvp_na();
void AddSC_outdoorpvp_si();
void AddSC_outdoorpvp_tf();
void AddSC_outdoorpvp_zm();

// Player
void AddSC_chat_log();

// Custom
void AddSC_custom_events();
void AddSC_workarounds();
void AddSC_ice_vanoce();
void AddSC_soulwell_transfer();
void AddSC_GSAI();

// World Bosses
void AddSC_boss_kruul();
void AddSC_boss_azuregos();
void AddSC_boss_doomlordkazzak();
void AddSC_boss_doomwalker();

#endif

void AddScripts()
{
    AddExampleScripts();
    AddSpellScripts();
    AddSC_SmartSCripts();
#ifdef SCRIPTS
    AddWorldScripts();
    AddEasternKingdomsScripts();
    AddKalimdorScripts();
    AddOutlandScripts();
    AddNorthrendScripts();
    AddCataclysmScripts();
    AddBattlegroundScripts();
    AddOutdoorPvPScripts();
    AddCustomScripts();
#endif
}

void AddExampleScripts()
{
    AddSC_example_creature();
    AddSC_example_escort();
    AddSC_example_gossip_codebox();
    AddSC_example_misc();
    AddSC_example_commandscript();
}

void AddSpellScripts()
{
    AddSC_deathknight_spell_scripts();
    AddSC_druid_spell_scripts();
    AddSC_generic_spell_scripts();
    AddSC_hunter_spell_scripts();
    AddSC_mage_spell_scripts();
    AddSC_paladin_spell_scripts();
    AddSC_priest_spell_scripts();
    AddSC_rogue_spell_scripts();
    AddSC_shaman_spell_scripts();
    AddSC_warlock_spell_scripts();
    AddSC_warrior_spell_scripts();
    AddSC_quest_spell_scripts();
    AddSC_item_spell_scripts();
    AddSC_example_spell_scripts();
}

void AddWorldScripts()
{
#ifdef SCRIPTS
    AddSC_areatrigger_scripts();
    AddSC_boss_emeriss();
    AddSC_boss_taerar();
    AddSC_boss_ysondre();
    AddSC_darkmoon_island();
    AddSC_generic_creature();
    AddSC_go_scripts();
    AddSC_guards();
    AddSC_guards_swog();
    AddSC_item_scripts();
    AddSC_npc_professions();
    AddSC_npc_innkeeper();
    AddSC_npc_spell_click_spells();
    AddSC_questline_fangs_of_the_father();
    AddSC_npcs_special();
    AddSC_npc_taxi();
    AddSC_achievement_scripts();
    AddSC_chat_log();
    AddSC_bf_commandscript();
#endif
}

void AddEasternKingdomsScripts()
{
#ifdef SCRIPTS
    // Locations
    AddSC_arathi_highlands();
    AddSC_badlands();
    AddSC_blasted_lands();
    AddSC_burning_steppes();
    AddSC_deadwind_pass();
    AddSC_deeprun_tram();
    AddSC_dun_morogh();
    AddSC_duskwood();
    AddSC_eastern_plaguelands();
    AddSC_elwynn_forest();
    AddSC_eversong_woods();
    AddSC_ghostlands();
    AddSC_hillsbrad_foothills();
    AddSC_loch_modan();
    AddCS_plaguelands();
    AddSC_ruins_of_gilneas();
    AddSC_redridge_mountains();
    AddSC_searing_gorge();
    AddSC_silverpine_forest();
    AddSC_stranglethorn_vale();
    AddSC_swamp_of_sorrows();
    AddSC_the_cape_of_stranglethorn();
    AddSC_the_hinterlands();
    AddSC_tirisfal_glades();
    AddSC_western_plaguelands();
    AddSC_westfall();
    AddSC_wetlands();
    // Cities
    AddSC_ironforge();
    AddSC_silvermoon_city();
    AddSC_stormwind_city();
    AddSC_undercity();
    // Stockade
    AddSC_instance_stockade();
    AddSC_boss_randolph_moloch();
    AddSC_boss_lord_overheat();
    AddSC_boss_hogger();
    // Gnomeregan
    AddSC_instance_gnomeregan();
    AddSC_gnomeregan();
    AddSC_boss_techbot();
    AddSC_boss_viscous_fallout();
    AddSC_boss_grubbis();
    AddSC_boss_electrocutioner_6000();
    AddSC_boss_crowd_pummeler();
    AddSC_boss_mekgineer_thermaplugg();
    // Scarlet Enclave
    AddSC_the_scarlet_enclave();
    AddSC_the_scarlet_enclave_c1();
    AddSC_the_scarlet_enclave_c2();
    AddSC_the_scarlet_enclave_c5();
    // Scarlet Monastery
    AddSC_instance_scarlet_monastery();
    AddSC_boss_bloodmage_thalnos();
    AddSC_boss_arcanist_doan();
    AddSC_boss_azshir_the_sleepless();
    AddSC_boss_headless_horseman();
    AddSC_boss_herod();
    AddSC_boss_high_inquisitor_fairbanks();
    AddSC_boss_houndmaster_loksey();
    AddSC_boss_interrogator_vishas();
    AddSC_boss_scorn();
    AddSC_boss_mograine_and_whitemane();
    // Uldaman
    AddSC_instance_uldaman();
    AddSC_uldaman();
    AddSC_boss_revelosh();
    AddSC_boss_baelog();
    AddSC_boss_ironaya();
    AddSC_boss_obsidian_sentinel();
    AddSC_boss_ancient_stone_keeper();
    AddSC_boss_galgann_firehammer();
    AddSC_boss_grimlok();
    AddSC_boss_archaedas();
    // Scholomance
    AddSC_instance_scholomance();
    AddSC_boss_jandice_barov();
    AddSC_boss_darkmaster_gandling();
    AddSC_boss_death_knight_darkreaver();
    AddSC_boss_theolen_krastinov();
    AddSC_boss_illucia_barov();
    AddSC_boss_instructor_malicia();
    AddSC_boss_kormok();
    AddSC_boss_lord_alexei_barov();
    AddSC_boss_lorekeeper_polkelt();
    AddSC_boss_ras_frostwhisper();
    AddSC_boss_the_ravenian();
    AddSC_boss_vectus();
    // Stratholme
    AddSC_instance_stratholme();
    AddSC_stratholme();
    AddSC_boss_the_unforgiven();
    AddSC_boss_hearthsinger_forresten();
    AddSC_boss_timmy_the_cruel();
    AddSC_boss_commander_malor();
    AddSC_boss_cannon_master_willey();
    AddSC_boss_instructor_galford();
    AddSC_boss_dathrohan_balnazzar();
    AddSC_boss_baroness_anastari();
    AddSC_boss_nerubenkan();
    AddSC_boss_maleki_the_pallid();
    AddSC_boss_magistrate_barthilas();
    AddSC_boss_ramstein_the_gorger();
    AddSC_boss_baron_rivendare();
    AddSC_boss_postmaster_malown();
    AddSC_boss_order_of_silver_hand();
    // Sunken Temple
    AddSC_instance_sunken_temple();
    AddSC_sunken_temple();
    AddSC_boss_jammalan_the_prophet();
    AddSC_boss_avatar_of_hakkar();
    AddSC_boss_shade_of_eranikus();
    // Blackrock Depths
    AddSC_instance_blackrock_depths();
    AddSC_blackrock_depths();
    AddSC_boss_lord_roccor();
    AddSC_boss_baelgar();
    AddSC_boss_houndmaster_grebmar();
    AddSC_boss_high_interrogator_gerstahn();
    AddSC_boss_high_justice_grimstone();
    AddSC_boss_gorosh_the_dervish();
    AddSC_boss_grizzle();
    AddSC_boss_eviscerator();
    AddSC_boss_okthor_the_breaker();
    AddSC_boss_anubshiah();
    AddSC_boss_hedrum_the_creeper();
    AddSC_boss_pyromancer_loregrain();
    AddSC_boss_general_angerforge();
    AddSC_boss_golem_lord_argelmach();
    AddSC_boss_ribbly_screwspigot();
    AddSC_boss_hurley_blackbreath();
    AddSC_boss_plugger_spazzring();
    AddSC_boss_phalanx();
    AddSC_boss_lord_incendius();
    AddSC_boss_fineous_darkvire();
    AddSC_boss_warden_stilgiss_and_verek();
    AddSC_boss_the_vault();
    AddSC_boss_ambassador_flamelash();
    AddSC_boss_tomb_of_seven();
    AddSC_boss_magmus();
    AddSC_boss_moira_bronzebeard();
    AddSC_boss_emperor_dragan_thaurissan();
    // Blackrock Spire
    AddSC_instance_blackrock_spire();
    AddSC_blackrock_spire();
    AddSC_boss_highlord_omokk();
    AddSC_boss_shadow_hunter_voshgajin();
    AddSC_boss_warmaster_voone();
    AddSC_boss_mother_smolderweb();
    AddSC_boss_urok_doomhowl();
    AddSC_boss_quatermaster_zigris();
    AddSC_boss_gizrul_the_slavener();
    AddSC_boss_halycon();
    AddSC_boss_overlord_wyrmthalak();
    AddSC_boss_pyroguard_emberseer();
    AddSC_boss_rend_blackhand();
    AddSC_boss_gyth();
    AddSC_boss_the_beast();
    AddSC_boss_drakkisath();
    // Blackwing lair
    AddSC_instance_blackwing_lair();
    AddSC_boss_razorgore();
    AddSC_boss_vaelastrasz();
    AddSC_boss_broodlord_lashlayer();
    AddSC_boss_firemaw();
    AddSC_boss_ebonroc();
    AddSC_boss_flamegor();
    AddSC_boss_chromaggus();
    AddSC_boss_nefarian();
    AddSC_boss_victor_nefarius();
    // Molten core
    AddSC_instance_molten_core();
    AddSC_molten_core();
    AddSC_boss_lucifron();
    AddSC_boss_magmadar();
    AddSC_boss_gehennas();
    AddSC_boss_garr();
    AddSC_boss_shazzrah();
    AddSC_boss_baron_geddon();
    AddSC_boss_golemagg();
    AddSC_boss_sulfuron_harbinger();
    AddSC_boss_majordomo_exectus();
    AddSC_boss_ragnaros();
#endif
}

void AddKalimdorScripts()
{
#ifdef SCRIPTS
    // Locations
    AddSC_ashenvale();
    AddSC_azshara();
    AddSC_azuremyst_isle();
    AddSC_bloodmyst_isle();
    AddSC_darkshore();
    AddSC_desolace();
    AddSC_durotar();
    AddSC_dustwallow_marsh();
    AddSC_felwood();
    AddSC_feralas();
    AddSC_moonglade();
    AddSC_mulgore();
    AddSC_silithus();
    AddSC_stonetalon_mountains();
    AddSC_tanaris();
    AddSC_teldrassil();
    AddSC_the_barrens();
    AddSC_thousand_needles();
    AddSC_ungoro_crater();
    AddSC_winterspring();
    // Cities
    AddSC_darnassus();
    AddSC_exodar();
    AddSC_orgrimmar();
    AddSC_thunder_bluff();
    // Ragefire Chasm
    AddSC_instance_ragefire_chasm();
    AddSC_boss_adarogg();
    AddSC_boss_dark_shaman_koranthal();
    AddSC_boss_slagmaw();
    AddSC_boss_lava_guard_gordoth();
    // Wailing caverns
    AddSC_instance_wailing_caverns();
    AddSC_wailing_caverns();
    AddSC_boss_lady_anacondra();
    AddSC_boss_lord_cobrahn();
    AddSC_boss_kresh();
    AddSC_boss_lord_pythas();
    AddSC_boss_skum();
    AddSC_boss_lord_serpentis();
    AddSC_boss_verdan_the_everliving();
    AddSC_boss_mutanus_the_devourer();
    // Blackfathom Deeps
    AddSC_instance_blackfathom_deeps();
    AddSC_blackfathom_deeps();
    AddSC_boss_ghamoo_ra();
    AddSC_boss_lady_sarevess();
    AddSC_boss_gelihast();
    AddSC_boss_lorgus_jett();
    AddSC_boss_baron_aquanis();
    AddSC_boss_kelris();
    AddSC_boss_old_serrakiss();
    AddSC_boss_aku_mai();
    // Razorfen Kraul
    AddSC_instance_razorfen_kraul();
    AddSC_razorfen_kraul();
    AddSC_boss_roogug();
    AddSC_boss_aggem_thorncurse();
    AddSC_boss_death_speaker_jargba();
    AddSC_boss_overlord_ramtusk();
    AddSC_boss_agathelos_the_raging();
    AddSC_boss_charlga_razorflank();
    // Maraudon
    AddSC_instance_maraudon();
    AddSC_boss_noxxion();
    AddSC_boss_razorlash();
    AddSC_boss_tinkerer_gizlock();
    AddSC_boss_lord_vyletongue();
    AddSC_boss_celebras_the_cursed();
    AddSC_boss_landslide();
    AddSC_boss_rotgrip();
    AddSC_boss_princess_theradras();
    // Razorfen Downs
    AddSC_instance_razorfen_downs();
    AddSC_razorfen_downs();
    AddSC_boss_tutenkash();
    AddSC_boss_mordresh_fire_eye();
    AddSC_boss_glutton();
    AddSC_boss_amnennar_the_coldbringer();
    // Dire Maul
    AddSC_instance_dire_maul();
    AddSC_boss_zevrim_thornhoof();
    AddSC_boss_hydrospawn();
    AddSC_boss_lethtendris();
    AddSC_boss_alzzin_the_wildshaper();
    AddSC_boss_guard_moldar();
    AddSC_boss_stomper_kreeg();
    AddSC_boss_guard_fengus();
    AddSC_boss_guard_slipkik();
    AddSC_boss_captain_kromcrush();
    AddSC_boss_king_gordok();
    AddSC_boss_chorush_the_observer();
    AddSC_boss_tendris_warpwood();
    AddSC_boss_illyanna_ravenoak();
    AddSC_boss_magister_kalendris();
    AddSC_boss_immolthar();
    AddSC_boss_prince_tortheldrin();
    // Zul'Farrak
    AddSC_instance_zulfarrak();
    AddSC_zulfarrak();
    AddSC_boss_antusul();
    AddSC_boss_theka_the_martyr();
    AddSC_boss_witch_doctor_zumrah();
    AddSC_boss_nekrum_gutchewer();
    AddSC_boss_hydromancer_velratha();
    AddSC_boss_gahzrilla();
    AddSC_boss_shadowpriest_sezzziz();
    AddSC_boss_ruuzlu();
    AddSC_boss_chief_ukorz_sandscalp();
    // Ruins of Ahn'Qiraj
    AddSC_instance_ruins_of_ahnqiraj();
    AddSC_boss_kurinnaxx();
    AddSC_boss_rajaxx();
    AddSC_boss_moam();
    AddSC_boss_buru();
    AddSC_boss_ayamiss();
    AddSC_boss_ossirian();
    // Temple of Ahn'Qiraj
    AddSC_instance_temple_of_ahnqiraj();
    AddSC_boss_skeram();
    AddSC_boss_battleguard_sartura();
    AddSC_boss_fankriss();
    AddSC_boss_huhuran();
    AddSC_boss_twinemperors();
    AddSC_boss_cthun();
    AddSC_boss_bug_trio();
    AddSC_boss_viscidus();
    AddSC_boss_ouro();
    AddSC_mob_anubisath_sentinel();
#endif
}

void AddOutlandScripts()
{
#ifdef SCRIPTS
    // Locations
    AddSC_blades_edge_mountains();
    AddSC_hellfire_peninsula();
    AddSC_nagrand();
    AddSC_netherstorm();
    AddSC_shadowmoon_valley();
    AddSC_terokkar_forest();
    AddSC_zangarmarsh();
    AddSC_isle_of_queldanas();
    // City
    AddSC_shattrath_city();
    // Hellfire Citacel: Hellfire Ramparts
    AddSC_instance_hellfire_ramparts();
    AddSC_boss_watchkeeper_gargolmar();
    AddSC_boss_omor_the_unscarred();
    AddSC_boss_vazruden_the_herald();
    // Hellfire Citadel: The Blood Furnace
    AddSC_instance_blood_furnace();
    AddSC_boss_the_maker();
    AddSC_boss_broggok();
    AddSC_boss_kelidan_the_breaker();
    // Hellfire Citadel: Shattered Halls
    AddSC_instance_shattered_halls();
    AddSC_boss_grand_warlock_nethekurse();
    AddSC_boss_blood_guard_porung();
    AddSC_boss_warbringer_omrogg();
    AddSC_boss_warchief_kargath_bladefist();
    // Coilfang Reservoir: The Slave Pens
    AddSC_instance_slave_pens();
    AddSC_boss_mennu_the_betrayer();
    AddSC_boss_rokmar_the_crackler();
    AddSC_boss_quagmirran();
    // Coilfang Reservoir: The Steamvault
    AddSC_instance_steam_vault();
    AddSC_boss_hydromancer_thespia();
    AddSC_boss_mekgineer_steamrigger();
    AddSC_boss_warlord_kalithresh();
    // Coilfang Reservoir: The Underbog
    AddSC_instance_the_underbog();
    AddSC_boss_hungarfen();
    AddSC_boss_ghazan();
    AddSC_boss_swamplord_muselek();
    AddSC_boss_the_black_stalker();
    // Auchindoun: Auchenai Crypts
    AddSC_instance_auchenai_crypts();
    AddSC_boss_shirrak_the_dead_watcher();
    AddSC_boss_exarch_maladaar();
    // Auchindoun: Mana Tombs
    AddSC_instance_mana_tombs();
    AddSC_boss_pandemonius();
    AddSC_boss_tavarok();
    AddSC_boss_nexusprince_shaffar();
    AddSC_boss_yor();
    // Auchindoun: Sethekk Halls
    AddSC_instance_sethekk_halls();
    AddSC_boss_darkweaver_syth();
    AddSC_boss_talon_king_ikiss();
    AddSC_boss_anzu();
    // Auchindoun: Shadow Labyrinth
    AddSC_instance_shadow_labyrinth();
    AddSC_boss_ambassador_hellmaw();
    AddSC_boss_blackheart_the_inciter();
    AddSC_boss_grandmaster_vorpil();
    AddSC_boss_murmur();
    // Tempest Keep: The Mechanar
    AddSC_instance_mechanar();
    AddSC_boss_mechano_lord_capacitus();
    AddSC_boss_gatewatcher_iron_hand();
    AddSC_boss_nethermancer_sepethrea();
    AddSC_boss_pathaleon_the_calculator();
    // Tempest Keep: Botanica
    AddSC_instance_botanica();
    AddSC_boss_commander_sarannis();
    AddSC_boss_high_botanist_freywinn();
    AddSC_boss_thorngrin_the_tender();
    AddSC_boss_laj();
    AddSC_boss_warp_splinter();
    // Tempest Keep: Arcatraz
    AddSC_instance_arcatraz();
    AddSC_arcatraz();
    AddSC_boss_zereketh_the_unbound();
    AddSC_boss_dalliah_the_doomsayer();
    AddSC_boss_wrath_scryer_soccothrates();
    AddSC_boss_harbinger_skyriss();
    // Caverns of Time: Old Hillsbrad Foothills
    AddSC_instance_old_hillsbrad();
    AddSC_old_hillsbrad();
    AddSC_boss_lieutenant_drake();
    AddSC_boss_captain_skarloc();
    AddSC_boss_epoch_hunter();
    // Caverns of Time: The Black Morass
    AddSC_instance_dark_portal();
    AddSC_dark_portal();
    AddSC_boss_chrono_lord_deja();
    AddSC_boss_temporus();
    AddSC_boss_aeonus();
    // Magister's Terrace
    AddSC_instance_magisters_terrace();
    AddSC_magisters_terrace();
    AddSC_boss_selin_fireheart();
    AddSC_boss_vexallus();
    AddSC_boss_priestess_delrissa();
    AddSC_boss_felblood_kaelthas();
    // Karazhan
    AddSC_instance_karazhan();
    AddSC_karazhan();
    AddSC_boss_servant_quarters();
    AddSC_boss_midnight();
    AddSC_boss_moroes();
    AddSC_bosses_opera();
    AddSC_boss_maiden_of_virtue();
    AddSC_boss_curator();
    AddSC_boss_chess_event();
    AddSC_boss_terestian_illhoof();
    AddSC_boss_shade_of_aran();
    AddSC_boss_netherspite();
    AddSC_boss_nightbane();
    AddSC_boss_prince_malchezaar();
    // Gruul's Lair
    AddSC_instance_gruuls_lair();
    AddSC_boss_gruul();
    AddSC_boss_high_king_maulgar();
    // Hellfire Citadel: Magtheridon's Lair
    AddSC_instance_magtheridons_lair();
    AddSC_boss_magtheridon();
    // Tempest Keep: The Eye
    AddSC_instance_the_eye();
    AddSC_the_eye();
    AddSC_boss_void_reaver();
    AddSC_boss_alar();
    AddSC_boss_high_astromancer_solarian();
    AddSC_boss_kaelthas();
    // Coilfang Reservoir: Serpentshrine Cavern
    AddSC_instance_serpentshrine_cavern();
    AddSC_boss_hydross_the_unstable();
    AddSC_boss_lurker_below();
    AddSC_boss_leotheras_the_blind();
    AddSC_boss_fathomlord_karathress();
    AddSC_boss_morogrim_tidewalker();
    AddSC_boss_lady_vashj();
    // Caverns of Time: Hyjal Summit
    AddSC_instance_hyjal_summit();
    AddSC_hyjal();
    AddSC_boss_rage_winterchill();
    AddSC_boss_anetheron();
    AddSC_boss_kazrogal();
    AddSC_boss_azgalor();
    AddSC_boss_archimonde();
    AddSC_hyjal_trash();
    // Black Temple
    AddSC_instance_black_temple();
    AddSC_black_temple();
    AddSC_boss_warlord_najentus();
    AddSC_boss_supremus();
    AddSC_boss_shade_of_akama();
    AddSC_boss_teron_gorefiend();
    AddSC_boss_gurtogg_bloodboil();
    AddSC_boss_reliquary_of_souls();
    AddSC_boss_mother_shahraz();
    AddSC_boss_illidari_council();
    AddSC_boss_illidan();
    // Sunwell Plateau
    AddSC_instance_sunwell_plateau();
    AddSC_sunwell_plateau();
    AddSC_boss_kalecgos();
    AddSC_boss_brutallus();
    AddSC_boss_felmyst();
    AddSC_boss_eredar_twins();
    AddSC_boss_muru();
    AddSC_boss_kiljaeden();
#endif
}

void AddNorthrendScripts()
{
#ifdef SCRIPTS
    // Locations
    AddSC_borean_tundra();
    AddSC_crystalsong_forest();
    AddSC_dragonblight();
    AddSC_grizzly_hills();
    AddSC_howling_fjord();
    AddSC_icecrown();
    AddSC_sholazar_basin();
    AddSC_storm_peaks();
    AddSC_zuldrak();
    AddSC_wintergrasp();
    // City
    AddSC_dalaran();
    // Utgarde Keep: Utgarde Keep
    AddSC_instance_utgarde_keep();
    AddSC_boss_keleseth();
    AddSC_boss_skarvald_dalronn();
    AddSC_boss_ingvar_the_plunderer();
    // The Nexus: The Nexus
    AddSC_instance_nexus();
    AddSC_boss_magus_telestra();
    AddSC_boss_anomalus();
    AddSC_boss_ormorok();
    AddSC_boss_keristrasza();
    AddSC_boss_commander_stoutbeard();
    AddSC_boss_commander_kolurg();
    // Azjol-Nerub: Azjol-Nerub
    AddSC_instance_azjol_nerub();
    AddSC_boss_krikthir_the_gatewatcher();
    AddSC_boss_hadronox();
    AddSC_boss_anubarak();
    // Azjol-Nerub: Ahn'kahet - The Old Kingdom
    AddSC_instance_ahnkahet();
    AddSC_boss_elder_nadox();
    AddSC_boss_prince_taldaram();
    AddSC_boss_jedoga_shadowseeker();
    AddSC_boss_herald_volazj();
    AddSC_boss_amanitar();
    // Drak'Tharon Keep
    AddSC_instance_drak_tharon_keep();
    AddSC_boss_trollgore();
    AddSC_boss_novos();
    AddSC_boss_dred();
    AddSC_boss_tharon_ja();
    // The Violet Hold
    AddSC_instance_violet_hold();
    AddSC_violet_hold();
    AddSC_boss_erekem();
    AddSC_boss_moragg();
    AddSC_boss_ichoron();
    AddSC_boss_xevozz();
    AddSC_boss_lavanthor();
    AddSC_boss_zuramat();
    AddSC_boss_cyanigosa();
    // Gundrak - 76-78
    AddSC_instance_gundrak();
    AddSC_boss_slad_ran();
    AddSC_boss_drakkari_colossus();
    AddSC_boss_moorabi();
    AddSC_boss_gal_darah();
    AddSC_boss_eck();
    // Ulduar: Halls of Stone
    AddSC_instance_halls_of_stone();
    AddSC_halls_of_stone();
    AddSC_boss_maiden_of_grief();
    AddSC_boss_krystallus();
    AddSC_boss_tribunal_of_the_ages();
    AddSC_boss_sjonnir();
    // Crusaders' Coliseum: Trial of the Champion
    AddSC_instance_trial_of_the_champion();
    AddSC_trial_of_the_champion();
    AddSC_boss_grand_champions();
    AddSC_boss_argent_challenge();
    AddSC_boss_black_knight();
    // Caverns of Time: The Culling of Stratholme
    AddSC_instance_culling_of_stratholme();
    AddSC_culling_of_stratholme();
    AddSC_boss_meathook();
    AddSC_boss_salramm();
    AddSC_boss_epoch();
    AddSC_boss_mal_ganis();
    AddSC_boss_infinite_corruptor();
    // The Nexus: The Oculus
    AddSC_instance_oculus();
    AddSC_oculus();
    AddSC_boss_drakos();
    AddSC_boss_varos_cloudstrider();
    AddSC_boss_urom();
    AddSC_boss_ley_guardian_eregos();
    // Utgarde Keep: Utgarde Pinnacle
    AddSC_instance_utgarde_pinnacle();
    AddSC_utgarde_keep();
    AddSC_boss_svala();
    AddSC_boss_palehoof();
    AddSC_boss_skadi();
    AddSC_boss_ymiron();
    // Ulduar: Halls of Lightning
    AddSC_instance_halls_of_lightning();
    AddSC_boss_bjarngrim();
    AddSC_boss_volkhan();
    AddSC_boss_ionar();
    AddSC_boss_loken();
    // Icecrown Citadel: Pit of Saron
    AddSC_instance_pit_of_saron();
    AddSC_pit_of_saron();
    AddSC_boss_forgemaster_garfrost();
    AddSC_boss_krick_and_ick();
    AddSC_boss_scourgelord_tyrannus();
    // Icecrown Citadel: Forge of Souls
    AddSC_instance_forge_of_souls();
    AddSC_forge_of_souls();
    AddSC_boss_bronjahm();
    AddSC_boss_devourer_of_souls();
    // Icecrown Citadel: Halls of Reflection
    AddSC_instance_halls_of_reflection();
    AddSC_halls_of_reflection();
    AddSC_boss_falric();
    AddSC_boss_marwyn();
    AddSC_boss_wrath_of_the_lich_king();
    // Crusaders' Coliseum: Trial of the Crusader
    AddSC_instance_trial_of_the_crusader();
    AddSC_trial_of_the_crusader();
    AddSC_boss_northrend_beasts();
    AddSC_boss_lord_jaraxxus();
    AddSC_boss_faction_champions();
    AddSC_boss_twin_valkyr();
    AddSC_boss_anubarak_trial();
    // The Nexus: The Eye of Eternity
    AddSC_instance_eye_of_eternity();
    AddSC_boss_malygos();
    // Onyxia's Lair
    AddSC_instance_onyxias_lair();
    AddSC_boss_onyxia();
    // Obsidian Sanctum
    AddSC_instance_obsidian_sanctum();
    AddSC_boss_sartharion();
    // Ruby Sanctum
    AddSC_instance_ruby_sanctum();
    AddSC_boss_baltharus();
    AddSC_boss_zarithrian();
    AddSC_boss_ragefire();
    AddSC_boss_halion();
    // Vault of Archavon
    AddSC_instance_vault_of_archavon();
    AddSC_boss_archavon();
    AddSC_boss_emalon();
    AddSC_boss_koralon();
    AddSC_boss_toravon();
    // Naxxramas
    AddSC_instance_naxxramas();
    AddSC_boss_anubrekhan();
    AddSC_boss_faerlina();
    AddSC_boss_maexxna();
    AddSC_boss_noth();
    AddSC_boss_heigan();
    AddSC_boss_loatheb();
    AddSC_boss_razuvious();
    AddSC_boss_gothik();
    AddSC_boss_four_horsemen();
    AddSC_boss_patchwerk();
    AddSC_boss_grobbulus();
    AddSC_boss_gluth();
    AddSC_boss_thaddius();
    AddSC_boss_sapphiron();
    AddSC_boss_kelthuzad();
    // Ulduar: Ulduar
    AddSC_instance_ulduar();
    AddSC_boss_flame_leviathan();
    AddSC_boss_ignis();
    AddSC_boss_hodir();
    AddSC_boss_razorscale();
    AddSC_boss_xt002();
    AddSC_boss_assembly_of_iron();
    AddSC_boss_kologarn();
    AddSC_boss_auriaya();
    AddSC_boss_freya();
    AddSC_boss_mimiron();
    AddSC_boss_thorim();
    AddSC_boss_general_vezax();
    AddSC_boss_yogg_saron();
    AddSC_boss_algalon_the_observer();
    AddSC_ulduar_teleporter();
    // Icecrown Citadel
    AddSC_instance_icecrown_citadel();
    AddSC_boss_lord_marrowgar();
    AddSC_boss_lady_deathwhisper();
    AddSC_boss_gunship_battle();
    AddSC_boss_deathbringer_saurfang();
    AddSC_boss_festergut();
    AddSC_boss_rotface();
    AddSC_boss_professor_putricide();
    AddSC_boss_blood_prince_council();
    AddSC_boss_blood_queen_lanathel();
    AddSC_boss_valithria_dreamwalker();
    AddSC_boss_sindragosa();
    AddSC_boss_lich_king();
    AddSC_icecrown_citadel_teleport();
#endif
}

void AddCataclysmScripts()
{
#ifdef SCRIPTS
    // Locations
    AddSC_deepholm();
    AddSC_gilneas();
    AddSC_kezan();
    AddSC_lost_isles();
    AddSC_mount_hyjal();
    AddSC_tol_barad();
    AddSC_twilight_highlands();
    AddSC_uldum();
    AddSC_vashjir();
    // Blackrock Caverns
    AddSC_instance_blackrock_caverns();
    AddSC_boss_romogg();
    AddSC_boss_corla();
    AddSC_boss_karsh();
    AddSC_boss_beauty();
    AddSC_boss_obsidius();
    // Throne of the Tides
    AddSC_instance_throne_of_the_tides();
    AddSC_boss_lady_nazjar();
    AddSC_boss_commander_ulthok();
    AddSC_boss_mindbender_ghursha();
    AddSC_boss_ozumat();
    // Stonecore
    AddSC_instance_the_stonecore();
    AddSC_boss_corborus();
    AddSC_boss_slabhide();
    AddSC_boss_ozruk();
    AddSC_boss_high_priestess_azil();
    // Vortex Pinnacle
    AddSC_instance_vortex_pinnacle();
    AddSC_boss_ertan();
    AddSC_boss_altairus();
    AddSC_boss_asaad();
    // Grim Batol
    AddSC_instance_grim_batol();
    AddSC_boss_general_umbriss();
    AddSC_boss_forgemaster_throngus();
    AddSC_boss_drahga_shadowburner();
    AddSC_boss_erudax_husam();
    // Lost City of Tol`Vir
    AddSC_instance_lost_city_of_the_tolvir();
    AddSC_boss_general_husam();
    AddSC_boss_high_prophet_barim();
    AddSC_boss_lockmaw_and_augh();
    AddSC_boss_siamat();
    // Halls of Origination
    AddSC_instance_halls_of_origination();
    AddSC_boss_temple_guardian_anhuur();
    AddSC_boss_earthrager_ptah();
    AddSC_boss_anraphet();
    AddSC_boss_isiset();
    AddSC_boss_ammunae();
    AddSC_boss_setesh();
    AddSC_boss_rajh();
    // Deadmines
    AddSC_instance_deadmines();
    AddSC_deadmines();
    AddSC_boss_mr_smite();
    AddSC_boss_glubtok();
    AddSC_boss_helix_gearbreaker();
    AddSC_boss_foe_reaper_5000();
    AddSC_boss_admiral_ripsnarl();
    AddSC_boss_captain_cookie();
    AddSC_boss_vanessa_vancleef();
    // Shadowfang Keep
    AddSC_instance_shadowfang_keep();
    AddSC_shadowfang_keep();
    AddSC_boss_baron_ashbury();
    AddSC_boss_baron_silverlaine();
    AddSC_boss_commander_springvale();
    AddSC_boss_lord_walden();
    AddSC_boss_lord_godfrey();
    // Zul`Gurub
    AddSC_instance_zulgurub();
    AddSC_boss_high_priestess_venoxis();
    AddSC_boss_bloodlord_mandokir();
    AddSC_boss_grilek();
    AddSC_boss_hazzarah();
    AddSC_boss_renataki();
    AddSC_boss_wushoolay();
    AddSC_boss_high_priestess_kilnara();
    AddSC_boss_zanzil();
    AddSC_boss_jindo_the_godbreaker();
    // Zul'Aman
    AddSC_instance_zulaman();
    AddSC_zulaman();
    AddSC_boss_akilzon();
    AddSC_boss_nalorakk();
    AddSC_boss_janalai();
    AddSC_boss_halazzi();
    AddSC_boss_hex_lord_malacrass();
    AddSC_boss_zuljin();
    // End Time
    AddSC_instance_end_time();
    AddSC_boss_echo_of_jaina();
    AddSC_boss_echo_of_baine();
    AddSC_boss_echo_of_sylvanas();
    AddSC_boss_echo_of_tyrande();
    AddSC_boss_murozond();
    // Well of Eternity
    AddSC_instance_well_of_eternity();
    AddSC_boss_perotharn();
    AddSC_boss_queen_azshara();
    AddSC_boss_mannoroth();
    AddSC_mannoroth_intro();
    AddSC_well_of_eternity_trash();
    AddSC_woe_aspects_event();
    // Hour of Twilight
    AddSC_instance_hour_of_twilight();
    AddSC_boss_arcurion();
    AddSC_boss_asira_dawnslayer();
    AddSC_boss_archbishop_benedictus();
    // Baradin Hold
    AddSC_instance_baradin_hold();
    AddSC_boss_argaloth();
    AddSC_boss_occuthar();
    AddSC_boss_alizabal();
    // The Bastion of Twilight
    AddSC_instance_bastion_of_twilight();
    AddSC_boss_halfus_wyrmbreaker();
    AddSC_boss_valiona_and_theralion();
    AddSC_boss_twilight_council();
    AddSC_boss_chogall();
    AddSC_boss_sinestra();
    // Blackwing Descent
    AddSC_instance_blackwing_descent();
    AddSC_boss_magmaw();
    AddSC_boss_omnotron_defensing_system();
    AddSC_boss_chimaeron();
    AddSC_boss_maloriak();
    AddSC_boss_atramedes();
    AddSC_boss_lord_nefarian();
    // Throne of the Four Winds
    AddSC_instance_throne_of_the_four_winds();
    AddSC_boss_conclave_of_wind();
    AddSC_boss_alakir();
    // Firelands
    AddSC_instance_firelands();
    AddSC_boss_shannox();
    AddSC_boss_bethtilac();
    AddSC_boss_lord_rhyolith();
    AddSC_boss_alysrazor();
    AddSC_boss_baeloroc();
    AddSC_boss_majordomo_staghelm();
    AddSC_boss_ragnaros_fl();
    // Dragon Soul
    AddSC_instance_dragonsoul();
    AddSC_dragon_soul_trash();
    AddSC_dragon_soul_aspects();
    AddSC_boss_morchok();
    AddSC_boss_warlord_zonozz();
    AddSC_boss_yorsahj_the_unsleeping();
    AddSC_boss_hagara_the_stormbinder();
    AddSC_boss_ultraxion();
    AddSC_boss_warmaster_blackhorn();
    AddSC_boss_spine_of_deathwing();
    AddSC_boss_madness_of_deathwing();
#endif
}

void AddOutdoorPvPScripts()
{
#ifdef SCRIPTS
    AddSC_outdoorpvp_ep();
    AddSC_outdoorpvp_hp();
    AddSC_outdoorpvp_na();
    AddSC_outdoorpvp_si();
    AddSC_outdoorpvp_tf();
    AddSC_outdoorpvp_zm();
#endif
}

void AddBattlegroundScripts()
{
#ifdef SCRIPTS
#endif
}

#ifdef SCRIPTS
/* This is where custom scripts' loading functions should be declared. */

#endif

void AddCustomScripts()
{
#ifdef SCRIPTS
    // custom
    AddSC_custom_events();
    AddSC_workarounds();
    AddSC_ice_vanoce();
    AddSC_soulwell_transfer();
    AddSC_GSAI();
#endif
}
