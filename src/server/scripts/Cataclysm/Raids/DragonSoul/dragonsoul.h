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
    NPC_THE_DRAGON_SOUL_SPINE           = 56694, // After spine

    // Thrall
    NPC_THRALL                          = 56667, // Ultraxion - Top of the Wyrmrest Temple
    NPC_THRALL_SPINE                    = 56103, // After Spine of Deathwing
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
};

const Position warmasterDamagePos[25] =
{
    { 13454.01f, -12145.91f, 150.84f, 3.16f },
    { 13450.81f, -12145.89f, 150.83f, 3.05f },
    { 13438.05f, -12144.99f, 150.79f, 3.09f },
    { 13427.59f, -12144.45f, 150.82f, 2.87f },
    { 13421.81f, -12139.19f, 150.86f, 1.95f },
    { 13421.55f, -12133.15f, 150.90f, 1.50f },
    { 13425.70f, -12121.36f, 150.84f, 0.84f },
    { 13432.72f, -12117.58f, 150.78f, 0.21f },
    { 13444.88f, -12116.27f, 150.79f, 6.26f },
    { 13453.38f, -12117.73f, 150.83f, 5.69f },
    { 13458.96f, -12125.79f, 150.89f, 5.06f },
    { 13459.80f, -12135.03f, 151.00f, 4.66f },
    { 13453.10f, -12139.57f, 150.84f, 2.88f },
    { 13449.03f, -12138.92f, 150.83f, 3.06f },
    { 13439.47f, -12138.54f, 150.81f, 3.01f },
    { 13437.73f, -12138.22f, 150.81f, 1.61f },
    { 13437.64f, -12130.64f, 150.83f, 1.55f },
    { 13437.73f, -12125.13f, 150.80f, 1.55f },
    { 13437.73f, -12125.13f, 150.80f, 1.51f },
    { 13442.06f, -12120.55f, 150.79f, 6.22f },
    { 13446.12f, -12120.87f, 150.81f, 6.17f },
    { 13451.08f, -12122.58f, 150.83f, 4.38f },
    { 13444.46f, -12131.01f, 150.83f, 4.15f },
    { 13444.04f, -12136.78f, 150.83f, 4.72f },
    { 13444.69f, -12140.80f, 150.82f, 4.87f },
};
#endif