/*
 * Copyright (C) 2006-2013 iCe Online <http://ice-wow.eu>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INSTANCE_DRAGONSOUL_H
#define INSTANCE_DRAGONSOUL_H

struct FacelessQuote : public PlayableQuote
{
private:
    std::string whisperText;

public:
    FacelessQuote(uint32 soundId, std::string text, std::string whisperText) : PlayableQuote(soundId, text)
    {
        this->whisperText = whisperText;
    }

    const char * GetWhisperText() const
    {
        return whisperText.c_str();
    }
};

enum Encounters
{
    // Bosses
    TYPE_BOSS_MORCHOK                   = 0,
    TYPE_BOSS_ZONOZZ                    = 1,
    TYPE_BOSS_YORSAHJ                   = 2,
    TYPE_BOSS_HAGARA                    = 3,
    TYPE_BOSS_ULTRAXION                 = 4,
    TYPE_BOSS_BLACKHORN                 = 5,
    TYPE_BOSS_SPINE_OF_DEATHWING        = 6,
    TYPE_BOSS_MADNESS_OF_DEATHWING      = 7,
    MAX_ENCOUNTER                       = 8,
};

enum AdditionalEvents
{
    DATA_HEROIC_KILLS                   = 9,

    DATA_PLAYERS_SUMMIT_ARRIVAL         = 10,
    DATA_START_ULTRAXION_ASPECTS_EVENT  = 11,
    DATA_ASPECTS_PREPARE_TO_CHANNEL     = 12,
    DATA_CHANNEL_DRAGONSOUL             = 13,
    DATA_DEATHWING_GREETINGS            = 14,
    DATA_ULTRAXION_DRAKES               = 15,
    DATA_SUMMON_ULTRAXION               = 16,
    DATA_HELP_AGAINST_ULTRAXION         = 17,
    DATA_ULTRAXION_DEFEATED             = 18,
    DATA_START_BLACKHORN_ENCOUNTER      = 19,
    DATA_HAGARA_INTRO_TRASH             = 20,
    DATA_PREPARE_SPINE_ENCOUNTER        = 21,
    DATA_SPINE_OF_DEATHWING_PLATES      = 22,
    DATA_ULTRAXION_RESET                = 23,
    DATA_WARMASTER_BOARD_SPOT           = 24,
};

enum AchievEvents
{
    //
};

enum DragonsoulCreatureIds
{
    NPC_VALEERA                         = 57289, // Travel to Zon'ozz
    NPC_EIENDORMI                       = 57288, // Travel to Yor'sahj
    NPC_NETHESTRASZ                     = 57287, // Travel at the top of the Wyrmrest Temple

    // Dragon Soul
    NPC_THE_DRAGON_SOUL                 = 56668, // Ultraxion - Top of the Wyrmrest Temple
    NPC_THE_DRAGON_SOUL_MADNESS         = 56694, // After spine

    // Thrall
    NPC_THRALL                          = 56667, // Ultraxion - Top of the Wyrmrest Temple
    NPC_THRALL_MADNESS_START            = 56103, // After Spine of Deathwing
    NPC_THRALL_MADNESS                  = 58232, // After killing Madness of Deathwing - outro

    // Aspects
    NPC_ALEXSTRASZA_THE_LIFE_BINDER     = 56630, // Ultraxion - Top of the Wyrmrest Temple
    NPC_ALEXSTRASZA_DRAGON_FORM         = 56099, // Madness of Deathwing - 1st platform
    NPC_ALEXSTRASZA_MADNESS             = 58207, // After killing Madness of Deathwing - outro

    NPC_NOZDORMU_THE_TIMELESS_ONE       = 56666, // Ultraxion - Top of the Wyrmrest Temple
    NPC_NOZDORMU_DRAGON_FORM            = 56102, // Madness of Deathwing - 2nd platform
    NPC_NOZDORMU_MADNESS                = 58208, // After killing Madness of Deathwing - outro

    NPC_YSERA_THE_AWAKENED              = 56665, // Ultraxion - Top of the Wyrmrest Temple
    NPC_YSERA_DRAGON_FORM               = 56100, // Madness of Deathwing - 3rd platform
    NPC_YSERA_MADNESS                   = 58209, // After killing Madness of Deathwing - outro

    NPC_KALECGOS                        = 56664, // Ultraxion - Top of the Wyrmrest Temple
    NPC_KALECGOS_DRAGON_FORM            = 56101, // Madness of Deathwing - 4th platform
    NPC_KALECGOS_MADNESS                = 58210, // After killing Madness of Deathwing - outro

    NPC_TRAVEL_TO_WYRMREST_TEMPLE       = 57328,
    NPC_TRAVEL_TO_WYRMREST_BASE         = 57882,
    NPC_TRAVEL_TO_WYRMREST_SUMMIT       = 57379, // Teleport at the top of the Wyrmrest Temple
    NPC_TRAVEL_TO_EYE_OF_ETERNITY       = 57377, // Teleport to Hagara
    NPC_TRAVEL_TO_DECK                  = 57378, // Teleport to Aliance Ship
    NPC_TRAVEL_TO_MAELSTORM             = 57443, // Teleport to Madness of Deathwing
    NPC_ANDORGOS                        = 15502,

    NPC_SKY_CAPTAIN_SWAYZE              = 55870,
    NPC_KAANU_REEVS                     = 55891,

    NPC_DEATHWING_WYRMREST_TEMPLE       = 55971,
    NPC_TWILIGHT_ASSAULTER              = 56252,
    NPC_TWILIGHT_ASSAULTERESS           = 56250,
};

enum Currencies
{
    CURRENCY_VALOR_POINTS                   = 396,
    CURRENCY_MOTE_OF_DARKNESS               = 614,
    CURRENCY_ESSENCE_OF_CORRUPTED_DEATHWING = 615,
};

enum SharedSpells
{
    SPELL_TELEPORT_VISUAL_ACTIVE                = 108203,
    SPELL_TELEPORT_VISUAL_DISABLED              = 108227,
    SPELL_OPEN_EYE_OF_ETERNITY_PORTAL           = 109527,
    SPELL_DRAGON_SOUL_PARATROOPER_KIT_1         = 104953, // Swayze has it while jumping to spine of deathwing
    SPELL_DRAGON_SOUL_PARATROOPER_KIT_2         = 105008, // Reevs has it while jumping to spine of deathwing
    SPELL_PARACHUTE                             = 110660, // Used by players
};

enum DragonsoulGameobjectsId
{
    GO_ALLIANCE_SHIP                    = 210081,
    GO_ALLIANCE_SHIP2                   = 210083,
    GO_HORDE_SHIP                       = 210082,
    GO_TRAVEL_TO_WYRMREST_TEMPLE        = 210085,
    GO_TRAVEL_TO_WYRMREST_BASE          = 210086,
    GO_TRAVEL_TO_WYRMREST_SUMMIT        = 210087,
    GO_TRAVEL_TO_EYE_OF_ETERNITY        = 210088,
    GO_TRAVEL_TO_DECK                   = 210089,
    GO_TRAVEL_TO_MAELSTROM              = 210090,
    GO_DEATHWING_BACK_PLATE_1           = 209623,
    GO_DEATHWING_BACK_PLATE_2           = 209631,
    GO_DEATHWING_BACK_PLATE_3           = 209632,
};
#endif