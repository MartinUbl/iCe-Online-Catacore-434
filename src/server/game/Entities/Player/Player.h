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

#ifndef _PLAYER_H
#define _PLAYER_H

#include <bitset>
#include "Common.h"
#include "ItemPrototype.h"
#include "Unit.h"
#include "Item.h"
#include "DatabaseEnv.h"
#include "NPCHandler.h"
#include "QuestDef.h"
#include "Group.h"
#include "Bag.h"
#include "WorldSession.h"
#include "Pet.h"
#include "MapReference.h"
#include "Util.h"                                           // for Tokens typedef
#include "AchievementMgr.h"
#include "ReputationMgr.h"
#include "Battleground.h"
#include "DBCEnums.h"
#include "MapInstanced.h"
#include "AntiHack.h"

#include<string>
#include<vector>

struct Mail;
class Channel;
class DynamicObject;
class Creature;
class Pet;
class PlayerMenu;
class UpdateMask;
class SpellCastTargets;
class PlayerSocial;
class OutdoorPvP;
class Guild;

typedef std::deque<Mail*> PlayerMails;

#define PLAYER_MAX_SKILLS           128
#define PLAYER_MAX_DAILY_QUESTS     25
#define PLAYER_EXPLORED_ZONES_SIZE  156

// Note: SPELLMOD_* values is aura types in fact
enum SpellModType
{
    SPELLMOD_FLAT         = 107,                            // SPELL_AURA_ADD_FLAT_MODIFIER
    SPELLMOD_PCT          = 108                             // SPELL_AURA_ADD_PCT_MODIFIER
};

// 2^n values, Player::m_isunderwater is a bitmask. These are Trinity internal values, they are never send to any client
enum PlayerUnderwaterState
{
    UNDERWATER_NONE                     = 0x00,
    UNDERWATER_INWATER                  = 0x01,             // terrain type is water and player is afflicted by it
    UNDERWATER_INLAVA                   = 0x02,             // terrain type is lava and player is afflicted by it
    UNDERWATER_INSLIME                  = 0x04,             // terrain type is lava and player is afflicted by it
    UNDERWARER_INDARKWATER              = 0x08,             // terrain type is dark water and player is afflicted by it

    UNDERWATER_EXIST_TIMERS             = 0x10
};

enum BuyBankSlotResult
{
    ERR_BANKSLOT_FAILED_TOO_MANY    = 0,
    ERR_BANKSLOT_INSUFFICIENT_FUNDS = 1,
    ERR_BANKSLOT_NOTBANKER          = 2,
    ERR_BANKSLOT_OK                 = 3
};

enum TrainerBuySpellResult
{
    ERR_TRAINER_UNAVAILABLE = 0,
    ERR_TRAINER_NOT_ENOUGH_MONEY = 1,
    ERR_TRAINER_OK = 2,
};

enum PlayerSpellState
{
    PLAYERSPELL_UNCHANGED = 0,
    PLAYERSPELL_CHANGED   = 1,
    PLAYERSPELL_NEW       = 2,
    PLAYERSPELL_REMOVED   = 3,
    PLAYERSPELL_TEMPORARY = 4
};

enum RatedBGStats
{
    RATED_BG_STAT_MATCHES_WON    = 0,
    RATED_BG_STAT_MATCHES_LOST   = 1,
    RATED_BG_STATS_MAX           = 2
};

struct PlayerSpell
{
    PlayerSpellState state : 8;
    bool active            : 1;                             // show in spellbook
    bool dependent         : 1;                             // learned as result another spell learn, skill grow, quest reward, etc
    bool disabled          : 1;                             // first rank has been learned in result talent learn but currently talent unlearned, save max learned ranks
};

struct PlayerTalent
{
    PlayerSpellState state : 8;
    uint8 spec             : 8;
};

enum PlayerChatTag
{
    CHAT_TAG_NONE       = 0x00,
    CHAT_TAG_AFK        = 0x01,
    CHAT_TAG_DND        = 0x02,
    CHAT_TAG_GM         = 0x04,
    CHAT_TAG_UNK        = 0x08, // Probably battleground commentator
    CHAT_TAG_DEV        = 0x10,
};

enum PlayerCurrencyState
{
    PLAYERCURRENCY_UNCHANGED = 0,
    PLAYERCURRENCY_CHANGED   = 1,
    PLAYERCURRENCY_NEW       = 2,
    PLAYERCURRENCY_REMOVED   = 3
};

enum CurrencySource
{
    CURRENCY_SOURCE_ALL    = 0,
    CURRENCY_SOURCE_ARENA  = 1,
    CURRENCY_SOURCE_BG     = 2,
    CURRENCY_SOURCE_OTHER  = 3,
    CURRENCY_SOURCE_MAX
};

typedef std::map<CurrencySource, uint32> CurrencySourceMap;

struct PlayerCurrency
{
    PlayerCurrencyState state;
    uint32 totalCount;
    uint32 seasonCount;

    CurrencySourceMap weekCap;
    CurrencySourceMap weekCount;
};

// Spell modifier (used for modify other spells)
struct SpellModifier
{
    SpellModifier(Aura * _ownerAura = NULL) : charges(0), ownerAura(_ownerAura) {}
    SpellModOp   op   : 8;
    SpellModType type : 8;
    int16 charges     : 16;
    int32 value;
    flag96 mask;
    uint32 spellId;
    Aura * const ownerAura;
};

typedef std::unordered_map<uint32, PlayerTalent*> PlayerTalentMap;
typedef std::unordered_map<uint32, PlayerSpell*> PlayerSpellMap;
typedef std::unordered_map<uint32, PlayerCurrency> PlayerCurrenciesMap;
typedef std::list<SpellModifier*> SpellModList;

/// Maximum number of CompactUnitFrames profiles
#define MAX_CUF_PROFILES 5

/// Bit index used in the many bool options of CompactUnitFrames
enum CUFBoolOptions
{
    CUF_KEEP_GROUPS_TOGETHER,
    CUF_DISPLAY_PETS,
    CUF_DISPLAY_MAIN_TANK_AND_ASSIST,
    CUF_DISPLAY_HEAL_PREDICTION,
    CUF_DISPLAY_AGGRO_HIGHLIGHT,
    CUF_DISPLAY_ONLY_DISPELLABLE_DEBUFFS,
    CUF_DISPLAY_POWER_BAR,
    CUF_DISPLAY_BORDER,
    CUF_USE_CLASS_COLORS,
    CUF_DISPLAY_NON_BOSS_DEBUFFS,
    CUF_DISPLAY_HORIZONTAL_GROUPS,
    CUF_LOCKED,
    CUF_SHOWN,
    CUF_AUTO_ACTIVATE_2_PLAYERS,
    CUF_AUTO_ACTIVATE_3_PLAYERS,
    CUF_AUTO_ACTIVATE_5_PLAYERS,
    CUF_AUTO_ACTIVATE_10_PLAYERS,
    CUF_AUTO_ACTIVATE_15_PLAYERS,
    CUF_AUTO_ACTIVATE_25_PLAYERS,
    CUF_AUTO_ACTIVATE_40_PLAYERS,
    CUF_AUTO_ACTIVATE_SPEC_1,
    CUF_AUTO_ACTIVATE_SPEC_2,
    CUF_AUTO_ACTIVATE_PVP,
    CUF_AUTO_ACTIVATE_PVE,
    CUF_UNK_145,
    CUF_UNK_156,
    CUF_UNK_157,

    CUF_BOOL_OPTIONS_COUNT,
};

/// Represents a CompactUnitFrame profile
struct CUFProfile
{
    CUFProfile() : ProfileName(), BoolOptions() // might want to change default value for options
    {
        FrameHeight = 0;
        FrameWidth  = 0;
        SortBy      = 0;
        HealthText  = 0;
        Unk146      = 0;
        Unk147      = 0;
        Unk148      = 0;
        Unk150      = 0;
        Unk152      = 0;
        Unk154      = 0;
    }

    CUFProfile(const std::string& name, uint16 frameHeight, uint16 frameWidth, uint8 sortBy, uint8 healthText, uint32 boolOptions,
        uint8 unk146, uint8 unk147, uint8 unk148, uint16 unk150, uint16 unk152, uint16 unk154)
        : ProfileName(name), BoolOptions((int)boolOptions)
    {
        FrameHeight = frameHeight;
        FrameWidth  = frameWidth;
        SortBy      = sortBy;
        HealthText  = healthText;
        Unk146      = unk146;
        Unk147      = unk147;
        Unk148      = unk148;
        Unk150      = unk150;
        Unk152      = unk152;
        Unk154      = unk154;
    }

    std::string ProfileName;
    uint16 FrameHeight;
    uint16 FrameWidth;
    uint8 SortBy;
    uint8 HealthText;

    uint8 Unk146;
    uint8 Unk147;
    uint8 Unk148;
    uint16 Unk150;
    uint16 Unk152;
    uint16 Unk154;

    std::bitset<CUF_BOOL_OPTIONS_COUNT> BoolOptions;

    // More fields can be added to BoolOptions without changing DB schema (up to 31, currently 24)
};

struct SpellCooldown
{
    uint32 end;     // cooldown is finished when m_LogonTimer reaches this value
    uint16 itemid;
};

typedef std::map<uint32, SpellCooldown> SpellCooldowns;

enum TrainerSpellState
{
    TRAINER_SPELL_ALREADY_LEARN = 00,
    TRAINER_SPELL_CAN_LEARN     = 01,
    TRAINER_SPELL_CANT_LEARN    = 02
};

enum ActionButtonUpdateState
{
    ACTIONBUTTON_UNCHANGED = 0,
    ACTIONBUTTON_CHANGED   = 1,
    ACTIONBUTTON_NEW       = 2,
    ACTIONBUTTON_DELETED   = 3
};

enum ActionButtonType
{
    ACTION_BUTTON_SPELL     = 0x00,
    ACTION_BUTTON_C         = 0x01,                         // click?
    ACTION_BUTTON_EQSET     = 0x20,
    ACTION_BUTTON_LIST      = 0x30,
    ACTION_BUTTON_MACRO     = 0x40,
    ACTION_BUTTON_CMACRO    = ACTION_BUTTON_C | ACTION_BUTTON_MACRO,
    ACTION_BUTTON_ITEM      = 0x80
};

#define ACTION_BUTTON_ACTION(X) (uint32(X) & 0x00FFFFFF)
#define ACTION_BUTTON_TYPE(X)   ((uint32(X) & 0xFF000000) >> 24)
#define MAX_ACTION_BUTTON_ACTION_VALUE (0x00FFFFFF+1)

struct ActionButton
{
    ActionButton() : packedData(0), uState(ACTIONBUTTON_NEW) {}

    uint32 packedData;
    ActionButtonUpdateState uState;

    // helpers
    ActionButtonType GetType() const { return ActionButtonType(ACTION_BUTTON_TYPE(packedData)); }
    uint32 GetAction() const { return ACTION_BUTTON_ACTION(packedData); }
    void SetActionAndType(uint32 action, ActionButtonType type)
    {
        uint32 newData = action | (uint32(type) << 24);
        if (newData != packedData || uState == ACTIONBUTTON_DELETED)
        {
            packedData = newData;
            if (uState != ACTIONBUTTON_NEW)
                uState = ACTIONBUTTON_CHANGED;
        }
    }
};

#define  MAX_ACTION_BUTTONS 144                             //checked in 3.2.0

typedef std::map<uint8,ActionButton> ActionButtonList;

struct PlayerCreateInfoItem
{
    PlayerCreateInfoItem(uint32 id, uint32 amount) : item_id(id), item_amount(amount) {}

    uint32 item_id;
    uint32 item_amount;
};

typedef std::list<PlayerCreateInfoItem> PlayerCreateInfoItems;

struct PlayerClassLevelInfo
{
    PlayerClassLevelInfo() : basehealth(0), basemana(0) {}
    uint16 basehealth;
    uint16 basemana;
};

struct PlayerClassInfo
{
    PlayerClassInfo() : levelInfo(NULL) { }

    PlayerClassLevelInfo* levelInfo;                        //[level-1] 0..MaxPlayerLevel-1
};

struct PlayerLevelInfo
{
    PlayerLevelInfo() { for (uint8 i=0; i < MAX_STATS; ++i) stats[i] = 0; }

    uint8 stats[MAX_STATS];
};

typedef std::list<uint32> PlayerCreateInfoSpells;

struct PlayerCreateInfoAction
{
    PlayerCreateInfoAction() : button(0), type(0), action(0) {}
    PlayerCreateInfoAction(uint8 _button, uint32 _action, uint8 _type) : button(_button), type(_type), action(_action) {}

    uint8 button;
    uint8 type;
    uint32 action;
};

typedef std::list<PlayerCreateInfoAction> PlayerCreateInfoActions;

struct PlayerInfo
{
                                                            // existence checked by displayId != 0
    PlayerInfo() : displayId_m(0),displayId_f(0),levelInfo(NULL)
    {
    }

    uint32 mapId;
    uint32 areaId;
    float positionX;
    float positionY;
    float positionZ;
    float orientation;
    uint16 displayId_m;
    uint16 displayId_f;
    PlayerCreateInfoItems item;
    PlayerCreateInfoSpells spell;
    PlayerCreateInfoActions action;

    PlayerLevelInfo* levelInfo;                             //[level-1] 0..MaxPlayerLevel-1
};

struct PvPInfo
{
    PvPInfo() : inHostileArea(false), inNoPvPArea(false), inFFAPvPArea(false), endTimer(0) {}

    bool inHostileArea;
    bool inNoPvPArea;
    bool inFFAPvPArea;
    time_t endTimer;
};

struct DuelInfo
{
    DuelInfo() : initiator(NULL), opponent(NULL), startTimer(0), startTime(0), outOfBound(0) {}

    Player *initiator;
    Player *opponent;
    time_t startTimer;
    time_t startTime;
    time_t outOfBound;
};

struct Areas
{
    uint32 areaID;
    uint32 areaFlag;
    float x1;
    float x2;
    float y1;
    float y2;
};

#define MAX_RUNES       6

enum RuneCooldowns
{
    RUNE_BASE_COOLDOWN  = 10000,
    RUNE_MISS_COOLDOWN  = 1500     // cooldown applied on runes when the spell misses
};

enum RuneType
{
    RUNE_BLOOD      = 0,
    RUNE_UNHOLY     = 1,
    RUNE_FROST      = 2,
    RUNE_DEATH      = 3,
    NUM_RUNE_TYPES  = 4
};

struct RuneInfo
{
    uint8 BaseRune;
    uint8 CurrentRune;
    uint32 Cooldown;
    AuraEffect const * ConvertAura;
};

struct SavedCastedAura
{
    uint64 targetGUID;
    uint32 spell;
};

struct Runes
{
    RuneInfo runes[MAX_RUNES];
    uint8 runeState;                                        // mask of available runes
    //RuneType lastUsedRune;
    uint8 lastUsedRunes;

    void SetRuneState(uint8 index, bool set = true)
    {
        if (set)
            runeState |= (1 << index);                      // usable
        else
            runeState &= ~(1 << index);                     // on cooldown
    }
};

struct EnchantDuration
{
    EnchantDuration() : item(NULL), slot(MAX_ENCHANTMENT_SLOT), leftduration(0) {};
    EnchantDuration(Item * _item, EnchantmentSlot _slot, uint32 _leftduration) : item(_item), slot(_slot),
        leftduration(_leftduration){ ASSERT(item); };

    Item * item;
    EnchantmentSlot slot;
    uint32 leftduration;
};

typedef std::list<EnchantDuration> EnchantDurationList;
typedef std::list<Item*> ItemDurationList;

enum PlayerMovementType
{
    MOVE_ROOT       = 1,
    MOVE_UNROOT     = 2,
    MOVE_WATER_WALK = 3,
    MOVE_LAND_WALK  = 4
};

enum DrunkenState
{
    DRUNKEN_SOBER   = 0,
    DRUNKEN_TIPSY   = 1,
    DRUNKEN_DRUNK   = 2,
    DRUNKEN_SMASHED = 3
};

#define MAX_DRUNKEN   4

enum PlayerFlags
{
    PLAYER_FLAGS_GROUP_LEADER   = 0x00000001,
    PLAYER_FLAGS_AFK            = 0x00000002,
    PLAYER_FLAGS_DND            = 0x00000004,
    PLAYER_FLAGS_GM             = 0x00000008,
    PLAYER_FLAGS_GHOST          = 0x00000010,
    PLAYER_FLAGS_RESTING        = 0x00000020,
    PLAYER_FLAGS_UNK7           = 0x00000040,
    PLAYER_FLAGS_UNK8           = 0x00000080,               // pre-3.0.3 PLAYER_FLAGS_FFA_PVP flag for FFA PVP state
    PLAYER_FLAGS_CONTESTED_PVP  = 0x00000100,               // Player has been involved in a PvP combat and will be attacked by contested guards
    PLAYER_FLAGS_IN_PVP         = 0x00000200,
    PLAYER_FLAGS_HIDE_HELM      = 0x00000400,
    PLAYER_FLAGS_HIDE_CLOAK     = 0x00000800,
    PLAYER_FLAGS_UNK13          = 0x00001000,               // played long time
    PLAYER_FLAGS_UNK14          = 0x00002000,               // played too long time
    PLAYER_FLAGS_UNK15          = 0x00004000,               // after logout, teleport using console and login, player will be frozen in place, no models displaying, on OLD location before teleport
    PLAYER_FLAGS_DEVELOPER      = 0x00008000,               // <Dev> prefix for something?
    PLAYER_FLAGS_UNK17          = 0x00010000,               // pre-3.0.3 PLAYER_FLAGS_SANCTUARY flag for player entered sanctuary
    PLAYER_FLAGS_UNK18          = 0x00020000,               // taxi benchmark mode (on/off) (2.0.1)
    PLAYER_FLAGS_PVP_TIMER      = 0x00040000,               // 3.0.2, pvp timer active (after you disable pvp manually)
    PLAYER_FLAGS_UNK20          = 0x00080000,
    PLAYER_FLAGS_UNK21          = 0x00100000,
    PLAYER_FLAGS_UNK22          = 0x00200000,
    PLAYER_FLAGS_UNK23          = 0x00400000,
    PLAYER_ALLOW_ONLY_ABILITY   = 0x00800000,                // used by bladestorm and killing spree
    PLAYER_FLAGS_UNK25          = 0x01000000,                // disabled all melee ability on tab include autoattack
    PLAYER_FLAGS_NO_XP_GAIN     = 0x02000000,
    PLAYER_FLAGS_UNK26          = 0x04000000,
    PLAYER_FLAGS_UNK27          = 0x08000000,
    PLAYER_FLAGS_GLEVEL_ENABLED = 0x10000000,
    PLAYER_FLAGS_VOID_UNLOCKED  = 0x20000000,       // void storage
    PLAYER_FLAGS_UNK30          = 0x40000000,
    PLAYER_FLAGS_UNK31          = 0x80000000
};

#define KNOWN_TITLES_SIZE   4
#define MAX_TITLE_INDEX     (KNOWN_TITLES_SIZE*64)          // 4 uint64 fields

// used in PLAYER_FIELD_BYTES values
enum PlayerFieldByteFlags
{
    PLAYER_FIELD_BYTE_TRACK_STEALTHED   = 0x00000002,
    PLAYER_FIELD_BYTE_RELEASE_TIMER     = 0x00000008,       // Display time till auto release spirit
    PLAYER_FIELD_BYTE_NO_RELEASE_WINDOW = 0x00000010        // Display no "release spirit" window at all
};

// used in PLAYER_FIELD_BYTES2 values
enum PlayerFieldByte2Flags
{
    PLAYER_FIELD_BYTE2_NONE                 = 0x00,
    PLAYER_FIELD_BYTE2_STEALTH              = 0x20,
    PLAYER_FIELD_BYTE2_INVISIBILITY_GLOW    = 0x40
};

enum ActivateTaxiReplies
{
    ERR_TAXIOK                      = 0,
    ERR_TAXIUNSPECIFIEDSERVERERROR  = 1,
    ERR_TAXINOSUCHPATH              = 2,
    ERR_TAXINOTENOUGHMONEY          = 3,
    ERR_TAXITOOFARAWAY              = 4,
    ERR_TAXINOVENDORNEARBY          = 5,
    ERR_TAXINOTVISITED              = 6,
    ERR_TAXIPLAYERBUSY              = 7,
    ERR_TAXIPLAYERALREADYMOUNTED    = 8,
    ERR_TAXIPLAYERSHAPESHIFTED      = 9,
    ERR_TAXIPLAYERMOVING            = 10,
    ERR_TAXISAMENODE                = 11,
    ERR_TAXINOTSTANDING             = 12
};

enum MirrorTimerType
{
    FATIGUE_TIMER      = 0,
    BREATH_TIMER       = 1,
    FIRE_TIMER         = 2
};
#define MAX_TIMERS      3
#define DISABLED_MIRROR_TIMER   -1

// 2^n values
enum PlayerExtraFlags
{
    // gm abilities
    PLAYER_EXTRA_GM_ON              = 0x0001,
    PLAYER_EXTRA_ACCEPT_WHISPERS    = 0x0004,
    PLAYER_EXTRA_TAXICHEAT          = 0x0008,
    PLAYER_EXTRA_GM_INVISIBLE       = 0x0010,
    PLAYER_EXTRA_GM_CHAT            = 0x0020,               // Show GM badge in chat messages
    PLAYER_EXTRA_HAS_310_FLYER      = 0x0040,               // Marks if player already has 310% speed flying mount
    
    // other states
    PLAYER_EXTRA_PVP_DEATH          = 0x0100,               // store PvP death status until corpse creating.
    PLAYER_EXTRA_WORGEN_FORM        = 0x0200,               // Player is in worgen form.
    PLAYER_EXTRA_PVPANNOUNCER       = 0x0400                // has subscribed to pvp announcer messages
};

// 2^n values
enum AtLoginFlags
{
    AT_LOGIN_NONE              = 0x00,
    AT_LOGIN_RENAME            = 0x01,
    AT_LOGIN_RESET_SPELLS      = 0x02,
    AT_LOGIN_RESET_TALENTS     = 0x04,
    AT_LOGIN_CUSTOMIZE         = 0x08,
    AT_LOGIN_RESET_PET_TALENTS = 0x10,
    AT_LOGIN_FIRST             = 0x20,
    AT_LOGIN_CHANGE_FACTION    = 0x40,
    AT_LOGIN_CHANGE_RACE       = 0x80,
    AT_LOGIN_LEARN_DEFAULT     = 0x100,
    AT_LOGIN_RESET_PET_SLOT    = 0x200,
    AT_LOGIN_REMOVE_PVP_TITLES = 0x400,
};

typedef std::map<uint32, QuestStatusData> QuestStatusMap;

enum QuestSlotOffsets
{
    QUEST_ID_OFFSET     = 0,
    QUEST_STATE_OFFSET  = 1,
    QUEST_COUNTS_OFFSET = 2,
    QUEST_TIME_OFFSET   = 4
};

#define MAX_QUEST_OFFSET 5

enum QuestSlotStateMask
{
    QUEST_STATE_NONE     = 0x0000,
    QUEST_STATE_COMPLETE = 0x0001,
    QUEST_STATE_FAIL     = 0x0002
};

enum SkillUpdateState
{
    SKILL_UNCHANGED     = 0,
    SKILL_CHANGED       = 1,
    SKILL_NEW           = 2,
    SKILL_DELETED       = 3
};

struct SkillStatusData
{
    SkillStatusData(uint8 _pos, SkillUpdateState _uState) : pos(_pos), uState(_uState)
    {
    }
    uint8 pos;
    SkillUpdateState uState;
};

typedef std::unordered_map<uint32, SkillStatusData> SkillStatusMap;

class Quest;
class Spell;
class Item;
class WorldSession;

enum PlayerSlots
{
    // first slot for item stored (in any way in player m_items data)
    PLAYER_SLOT_START           = 0,
    // last+1 slot for item stored (in any way in player m_items data)
    PLAYER_SLOT_END             = 86,
    PLAYER_SLOTS_COUNT          = (PLAYER_SLOT_END - PLAYER_SLOT_START)
};

#define INVENTORY_SLOT_BAG_0    255

enum EquipmentSlots                                         // 19 slots
{
    EQUIPMENT_SLOT_START        = 0,
    EQUIPMENT_SLOT_HEAD         = 0,
    EQUIPMENT_SLOT_NECK         = 1,
    EQUIPMENT_SLOT_SHOULDERS    = 2,
    EQUIPMENT_SLOT_BODY         = 3,
    EQUIPMENT_SLOT_CHEST        = 4,
    EQUIPMENT_SLOT_WAIST        = 5,
    EQUIPMENT_SLOT_LEGS         = 6,
    EQUIPMENT_SLOT_FEET         = 7,
    EQUIPMENT_SLOT_WRISTS       = 8,
    EQUIPMENT_SLOT_HANDS        = 9,
    EQUIPMENT_SLOT_FINGER1      = 10,
    EQUIPMENT_SLOT_FINGER2      = 11,
    EQUIPMENT_SLOT_TRINKET1     = 12,
    EQUIPMENT_SLOT_TRINKET2     = 13,
    EQUIPMENT_SLOT_BACK         = 14,
    EQUIPMENT_SLOT_MAINHAND     = 15,
    EQUIPMENT_SLOT_OFFHAND      = 16,
    EQUIPMENT_SLOT_RANGED       = 17,
    EQUIPMENT_SLOT_TABARD       = 18,
    EQUIPMENT_SLOT_END          = 19
};

enum InventorySlots                                         // 4 slots
{
    INVENTORY_SLOT_BAG_START    = 19,
    INVENTORY_SLOT_BAG_END      = 23
};

enum InventoryPackSlots                                     // 16 slots
{
    INVENTORY_SLOT_ITEM_START   = 23,
    INVENTORY_SLOT_ITEM_END     = 39
};

enum BankItemSlots                                          // 28 slots
{
    BANK_SLOT_ITEM_START        = 39,
    BANK_SLOT_ITEM_END          = 67
};

enum BankBagSlots                                           // 7 slots
{
    BANK_SLOT_BAG_START         = 67,
    BANK_SLOT_BAG_END           = 74
};

enum BuyBackSlots                                           // 12 slots
{
    // stored in m_buybackitems
    BUYBACK_SLOT_START          = 74,
    BUYBACK_SLOT_END            = 86
};

enum EquipmentSetUpdateState
{
    EQUIPMENT_SET_UNCHANGED = 0,
    EQUIPMENT_SET_CHANGED   = 1,
    EQUIPMENT_SET_NEW       = 2,
    EQUIPMENT_SET_DELETED   = 3
};

struct EquipmentSet
{
    EquipmentSet() : Guid(0), IgnoreMask(0), state(EQUIPMENT_SET_NEW)
    {
        for (uint8 i = 0; i < EQUIPMENT_SLOT_END; ++i)
            Items[i] = 0;
    }

    uint64 Guid;
    std::string Name;
    std::string IconName;
    uint32 IgnoreMask;
    uint32 Items[EQUIPMENT_SLOT_END];
    EquipmentSetUpdateState state;
};

#define MAX_EQUIPMENT_SET_INDEX 10                          // client limit

typedef std::map<uint32, EquipmentSet> EquipmentSets;

struct ItemPosCount
{
    ItemPosCount(uint16 _pos, uint32 _count) : pos(_pos), count(_count) {}
    bool isContainedIn(std::vector<ItemPosCount> const& vec) const;
    uint16 pos;
    uint32 count;
};
typedef std::vector<ItemPosCount> ItemPosCountVec;

enum TradeSlots
{
    TRADE_SLOT_COUNT            = 7,
    TRADE_SLOT_TRADED_COUNT     = 6,
    TRADE_SLOT_NONTRADED        = 6
};

#define MAX_DIGSITES 16

enum TransferAbortReason
{
    TRANSFER_ABORT_NONE                     = 0x00,
    TRANSFER_ABORT_ERROR                    = 0x01,
    TRANSFER_ABORT_MAX_PLAYERS              = 0x02,         // Transfer Aborted: instance is full
    TRANSFER_ABORT_NOT_FOUND                = 0x03,         // Transfer Aborted: instance not found
    TRANSFER_ABORT_TOO_MANY_INSTANCES       = 0x04,         // You have entered too many instances recently.
    TRANSFER_ABORT_ZONE_IN_COMBAT           = 0x06,         // Unable to zone in while an encounter is in progress.
    TRANSFER_ABORT_INSUF_EXPAN_LVL          = 0x07,         // You must have <TBC,WotLK> expansion installed to access this area.
    TRANSFER_ABORT_DIFFICULTY               = 0x08,         // <Normal,Heroic,Epic> difficulty mode is not available for %s.
    TRANSFER_ABORT_UNIQUE_MESSAGE           = 0x09,         // Until you've escaped TLK's grasp, you cannot leave this place!
    TRANSFER_ABORT_TOO_MANY_REALM_INSTANCES = 0x0A,         // Additional instances cannot be launched, please try again later.
    TRANSFER_ABORT_NEED_GROUP               = 0x0B,         // 3.1
    TRANSFER_ABORT_NOT_FOUND2               = 0x0C,         // 3.1
    TRANSFER_ABORT_NOT_FOUND3               = 0x0D,         // 3.1
    TRANSFER_ABORT_NOT_FOUND4               = 0x0E,         // 3.2
    TRANSFER_ABORT_REALM_ONLY               = 0x0F,         // All players on party must be from the same realm.
    TRANSFER_ABORT_MAP_NOT_ALLOWED          = 0x10,         // Map can't be entered at this time.
    TRANSFER_ABORT_LOCKED_TO_DIFFERENT_INSTANCE = 0x12,         // 4.2.2
    TRANSFER_ABORT_ALREADY_COMPLETED_ENCOUNTER  = 0x13         // 4.2.2
};

enum InstanceResetWarningType
{
    RAID_INSTANCE_WARNING_HOURS     = 1,                    // WARNING! %s is scheduled to reset in %d hour(s).
    RAID_INSTANCE_WARNING_MIN       = 2,                    // WARNING! %s is scheduled to reset in %d minute(s)!
    RAID_INSTANCE_WARNING_MIN_SOON  = 3,                    // WARNING! %s is scheduled to reset in %d minute(s). Please exit the zone or you will be returned to your bind location!
    RAID_INSTANCE_WELCOME           = 4,                    // Welcome to %s. This raid instance is scheduled to reset in %s.
    RAID_INSTANCE_EXPIRED           = 5
};

// PLAYER_FIELD_ARENA_TEAM_INFO_1_1 offsets
enum ArenaTeamInfoType
{
    ARENA_TEAM_ID                = 0,
    ARENA_TEAM_TYPE              = 1,                       // new in 3.2 - team type?
    ARENA_TEAM_MEMBER            = 2,                       // 0 - captain, 1 - member
    ARENA_TEAM_GAMES_WEEK        = 3,
    ARENA_TEAM_GAMES_SEASON      = 4,
    ARENA_TEAM_WINS_SEASON       = 5,
    ARENA_TEAM_PERSONAL_RATING   = 6,
    ARENA_TEAM_END               = 7
};

class InstanceSave;

enum RestType
{
    REST_TYPE_NO        = 0,
    REST_TYPE_IN_TAVERN = 1,
    REST_TYPE_IN_CITY   = 2
};

enum DuelCompleteType
{
    DUEL_INTERRUPTED = 0,
    DUEL_WON         = 1,
    DUEL_FLED        = 2
};

enum TeleportToOptions
{
    TELE_TO_GM_MODE             = 0x01,
    TELE_TO_NOT_LEAVE_TRANSPORT = 0x02,
    TELE_TO_NOT_LEAVE_COMBAT    = 0x04,
    TELE_TO_NOT_UNSUMMON_PET    = 0x08,
    TELE_TO_SPELL               = 0x10,
};

/// Type of environmental damages
enum EnviromentalDamage
{
    DAMAGE_EXHAUSTED = 0,
    DAMAGE_DROWNING  = 1,
    DAMAGE_FALL      = 2,
    DAMAGE_LAVA      = 3,
    DAMAGE_SLIME     = 4,
    DAMAGE_FIRE      = 5,
    DAMAGE_FALL_TO_VOID = 6                                 // custom case for fall without durability loss
};

enum PlayedTimeIndex
{
    PLAYED_TIME_TOTAL = 0,
    PLAYED_TIME_LEVEL = 1
};

#define MAX_PLAYED_TIME_INDEX 2

// used at player loading query list preparing, and later result selection
enum PlayerLoginQueryIndex
{
    PLAYER_LOGIN_QUERY_LOADFROM                 = 0,
    PLAYER_LOGIN_QUERY_LOADGROUP                = 1,
    PLAYER_LOGIN_QUERY_LOADBOUNDINSTANCES       = 2,
    PLAYER_LOGIN_QUERY_LOADAURAS                = 3,
    PLAYER_LOGIN_QUERY_LOADSPELLS               = 4,
    PLAYER_LOGIN_QUERY_LOADQUESTSTATUS          = 5,
    PLAYER_LOGIN_QUERY_LOADDAILYQUESTSTATUS     = 6,
    PLAYER_LOGIN_QUERY_LOADREPUTATION           = 7,
    PLAYER_LOGIN_QUERY_LOADINVENTORY            = 8,
    PLAYER_LOGIN_QUERY_LOADACTIONS              = 9,
    PLAYER_LOGIN_QUERY_LOADMAILCOUNT            = 10,
    PLAYER_LOGIN_QUERY_LOADMAILDATE             = 11,
    PLAYER_LOGIN_QUERY_LOADSOCIALLIST           = 12,
    PLAYER_LOGIN_QUERY_LOADHOMEBIND             = 13,
    PLAYER_LOGIN_QUERY_LOADSPELLCOOLDOWNS       = 14,
    PLAYER_LOGIN_QUERY_LOADDECLINEDNAMES        = 15,
    PLAYER_LOGIN_QUERY_LOADGUILD                = 16,
    PLAYER_LOGIN_QUERY_LOADARENAINFO            = 17,
    PLAYER_LOGIN_QUERY_LOADACHIEVEMENTS         = 18,
    PLAYER_LOGIN_QUERY_LOADCRITERIAPROGRESS     = 19,
    PLAYER_LOGIN_QUERY_LOADEQUIPMENTSETS        = 20,
    PLAYER_LOGIN_QUERY_LOADBGDATA               = 21,
    PLAYER_LOGIN_QUERY_LOADGLYPHS               = 22,
    PLAYER_LOGIN_QUERY_LOADTALENTS              = 23,
    PLAYER_LOGIN_QUERY_LOADACCOUNTDATA          = 24,
    PLAYER_LOGIN_QUERY_LOADSKILLS               = 25,
    PLAYER_LOGIN_QUERY_LOADWEKLYQUESTSTATUS     = 26,
    PLAYER_LOGIN_QUERY_LOADRANDOMBG             = 27,
    PLAYER_LOGIN_QUERY_LOADARENASTATS           = 28,
    PLAYER_LOGIN_QUERY_LOADBANNED               = 29,
    PLAYER_LOGIN_QUERY_LOADTALENTBRANCHSPECS    = 30,
    PLAYER_LOGIN_QUERY_LOADPETSLOT              = 31,
    PLAYER_LOGIN_QUERY_LOAD_CURRENCY            = 35,
    PLAYER_LOGIN_QUERY_LOAD_CURRENCY_WEEKCAP    = 36,
    PLAYER_LOGIN_QUERY_LOAD_CUF_PROFILES        = 37,
    PLAYER_LOGIN_QUERY_LOADVOIDSTORAGE          = 38,
    MAX_PLAYER_LOGIN_QUERY                      = 39
};

enum PlayerDelayedOperations
{
    DELAYED_SAVE_PLAYER             = 0x01,
    DELAYED_RESURRECT_PLAYER        = 0x02,
    DELAYED_SPELL_CAST_DESERTER     = 0x04,
    DELAYED_BG_MOUNT_RESTORE        = 0x08,                     ///< Flag to restore mount state after teleport from BG
    DELAYED_BG_TAXI_RESTORE         = 0x10,                     ///< Flag to restore taxi state after teleport from BG
    DELAYED_GROUP_MARKER_UPDATE     = 0x20,                     ///< Group need to get update packet for raid markers after teleport or login
    DELAYED_PVPANNOUNCE_JOIN        = 0x40,                     ///< PvP announcer join is done after login and world entering, but using flags from load time
    DELAYED_END
};

// Player summoning auto-decline time (in secs)
#define MAX_PLAYER_SUMMON_DELAY                   (2*MINUTE)
#define MAX_MONEY_AMOUNT                       9999999999 // 999'999g 99s 99c
// OLD_MAX_MONEY_AMOUNT                        2147483647

struct InstancePlayerBind
{
    InstanceSave *save;
    bool perm;
    /* permanent PlayerInstanceBinds are created in Raid/Heroic instances for players
       that aren't already permanently bound when they are inside when a boss is killed
       or when they enter an instance that the group leader is permanently bound to. */
    InstancePlayerBind() : save(NULL), perm(false) {}
};

enum DungeonStatusFlag
{
    DUNGEON_STATUSFLAG_NORMAL = 0x01,
    DUNGEON_STATUSFLAG_HEROIC = 0x02,

    RAID_STATUSFLAG_10MAN_NORMAL = 0x01,
    RAID_STATUSFLAG_25MAN_NORMAL = 0x02,
    RAID_STATUSFLAG_10MAN_HEROIC = 0x04,
    RAID_STATUSFLAG_25MAN_HEROIC = 0x08
};

struct AccessRequirement
{
    uint8  levelMin;
    uint8  levelMax;
    uint32 item;
    uint32 item2;
    uint32 quest_A;
    uint32 quest_H;
    uint32 achievement;
    std::string questFailedText;
};

enum CharDeleteMethod
{
    CHAR_DELETE_REMOVE = 0,                      // Completely remove from the database
    CHAR_DELETE_UNLINK = 1                       // The character gets unlinked from the account,
                                                 // the name gets freed up and appears as deleted ingame
};

enum CurrencyItems
{
    ITEM_HONOR_POINTS_ID    = 43308,
    ITEM_ARENA_POINTS_ID    = 43307
};

enum BranchSpec
{
    SPEC_NONE                 = 0,
    SPEC_DK_BLOOD             = 398,
    SPEC_DK_FROST             = 399,
    SPEC_DK_UNHOLY            = 400,
    SPEC_DRUID_BALANCE        = 752,
    SPEC_DRUID_FERAL          = 750,
    SPEC_DRUID_RESTORATION    = 748,
    SPEC_HUNTER_BEASTMASTERY  = 811,
    SPEC_HUNTER_MARKSMANSHIP  = 807,
    SPEC_HUNTER_SURVIVAL      = 809,
    SPEC_MAGE_ARCANE          = 799,
    SPEC_MAGE_FIRE            = 851,
    SPEC_MAGE_FROST           = 823,
    SPEC_PALADIN_HOLY         = 831,
    SPEC_PALADIN_PROTECTION   = 839,
    SPEC_PALADIN_RETRIBUTION  = 855,
    SPEC_PRIEST_DISCIPLINE    = 760,
    SPEC_PRIEST_HOLY          = 813,
    SPEC_PRIEST_SHADOW        = 795,
    SPEC_ROGUE_ASSASSINATION  = 182,
    SPEC_ROGUE_COMBAT         = 181,
    SPEC_ROGUE_SUBTLETY       = 183,
    SPEC_SHAMAN_ELEMENTAL     = 261,
    SPEC_SHAMAN_ENHANCEMENT   = 263,
    SPEC_SHAMAN_RESTORATION   = 262,
    SPEC_WARLOCK_AFFLICTION   = 871,
    SPEC_WARLOCK_DEMONOLOGY   = 867,
    SPEC_WARLOCK_DESTRUCTION  = 865,
    SPEC_WARRIOR_ARMS         = 746,
    SPEC_WARRIOR_FURY         = 815,
    SPEC_WARRIOR_PROTECTION   = 845,

    SPEC_PET_TENACITY         = 409,
    SPEC_PET_FEROCITY         = 410,
    SPEC_PET_CUNNING          = 411,
};

/*Player difficulty and merge progress in raids for Flexible raid locks rules*/
enum RaidPlayerStatus
{
    ONLY_NORMAL = 0,//all bosses killed only on normal difficulty (can enter any difficulty and ID)
    KILLED_HC,//killed 1 or more bosses on HC difficulty (cannot merge into other HC)
    KILLED_HC_N_MERGED,//killed some bosses on HC and then killed on normal difficulty or merged into other normal (cannot enter other than normal difficulty of that raid anymore)
};

class PlayerTaxi
{
public:
	PlayerTaxi();
	~PlayerTaxi() {}
	// Nodes
	void InitTaxiNodesForLevel(uint32 race, uint32 chrClass, uint8 level);
	void LoadTaxiMask(const char* data);

	bool IsTaximaskNodeKnown(uint32 nodeidx) const
	{
		uint8  field   = uint8((nodeidx - 1) / 32);
		uint32 submask = 1<<((nodeidx-1)%32);
		return (m_taximask[field] & submask) == submask;
	}
	bool SetTaximaskNode(uint32 nodeidx)
	{
		uint8  field   = uint8((nodeidx - 1) / 32);
		uint32 submask = 1<<((nodeidx-1)%32);
		if ((m_taximask[field] & submask) != submask)
		{
			m_taximask[field] |= submask;
			return true;
		}
		else
			return false;
	}
	void AppendTaximaskTo(ByteBuffer& data,bool all);
	bool AutomaticTaxiNodeLearn(uint32 nodeid);

	// Destinations
	bool LoadTaxiDestinationsFromString(const std::string& values, uint32 team);
	std::string SaveTaxiDestinationsToString();

	void ClearTaxiDestinations() { m_TaxiDestinations.clear(); }
	void AddTaxiDestination(uint32 dest) { m_TaxiDestinations.push_back(dest); }
	uint32 GetTaxiSource() const { return m_TaxiDestinations.empty() ? 0 : m_TaxiDestinations.front(); }
	uint32 GetTaxiDestination() const { return m_TaxiDestinations.size() < 2 ? 0 : m_TaxiDestinations[1]; }
	uint32 GetCurrentTaxiPath() const;
	uint32 NextTaxiDestination()
	{
		m_TaxiDestinations.pop_front();
		return GetTaxiDestination();
	}
	bool empty() const { return m_TaxiDestinations.empty(); }

	friend std::ostringstream& operator<< (std::ostringstream& ss, PlayerTaxi const& taxi);
private:
	TaxiMask m_taximask;
	std::deque<uint32> m_TaxiDestinations;
};

std::ostringstream& operator<< (std::ostringstream& ss, PlayerTaxi const& taxi);

class Player;

/// Holder for Battleground data
struct BGData
{
    BGData() : bgInstanceID(0), bgTypeID(BATTLEGROUND_TYPE_NONE), bgAfkReportedCount(0), bgAfkReportedTimer(0),
        bgTeam(0), mountSpell(0) { bgQueuesJoinedTime.clear(); ClearTaxiPath(); }

    uint32 bgInstanceID;                    ///< This variable is set to bg->m_InstanceID,
                                            ///  when player is teleported to BG - (it is battleground's GUID)
    BattlegroundTypeId bgTypeID;

    std::map<uint32, uint32> bgQueuesJoinedTime;

    std::set<uint32>   bgAfkReporter;
    uint8              bgAfkReportedCount;
    time_t             bgAfkReportedTimer;

    uint32 bgTeam;                          ///< What side the player will be added to

    uint32 mountSpell;
    uint32 taxiPath[2];

    WorldLocation joinPos;                  ///< From where player entered BG

    void ClearTaxiPath()     { taxiPath[0] = taxiPath[1] = 0; }
    bool HasTaxiPath() const { return taxiPath[0] && taxiPath[1]; }
};

struct VoidStorageItem
{
    VoidStorageItem()
    {
        ItemId = 0;
        ItemEntry = 0;
        CreatorGuid = 0;
        ItemRandomPropertyId = 0;
        ItemSuffixFactor = 0;
    }

    VoidStorageItem(uint64 id, uint32 entry, uint32 creator, uint32 randomPropertyId, uint32 suffixFactor)
    {
        ItemId = id;
        ItemEntry = entry;
        CreatorGuid = creator;
        ItemRandomPropertyId = randomPropertyId;
        ItemSuffixFactor = suffixFactor;
    }

    uint64 ItemId;
    uint32 ItemEntry;
    uint32 CreatorGuid;
    uint32 ItemRandomPropertyId;
    uint32 ItemSuffixFactor;
};

class TradeData
{
    public:                                                 // constructors
        TradeData(Player* player, Player* trader) :
            m_player(player),  m_trader(trader), m_accepted(false), m_acceptProccess(false),
            m_money(0), m_spell(0), m_packetExtCount(1), m_packetExtCountOther(1) {}

        Player* GetTrader() const { return m_trader; }
        TradeData* GetTraderData() const;

        Item* GetItem(TradeSlots slot) const;
        bool HasItem(uint64 item_guid) const;
        void SetItem(TradeSlots slot, Item* item);

        uint32 GetSpell() const { return m_spell; }
        void SetSpell(uint32 spell_id, Item* castItem = NULL);

        Item*  GetSpellCastItem() const;
        bool HasSpellCastItem() const { return m_spellCastItem != 0; }

        uint64 GetMoney() const { return m_money; }
        void SetMoney(uint64 money);

        bool IsAccepted() const { return m_accepted; }
        void SetAccepted(bool state, bool crosssend = false);

        bool IsInAcceptProcess() const { return m_acceptProccess; }
        void SetInAcceptProcess(bool state) { m_acceptProccess = state; }

        uint32 GetPacketCount(bool trader) const { return trader?m_packetExtCount:m_packetExtCountOther; };
        void SetPacketCount(uint32 count, bool trader) { if(trader) m_packetExtCount = count; else m_packetExtCountOther = count; };
        void IncrementPacketCount(bool trader) { if(trader) m_packetExtCount += 1; else m_packetExtCountOther += 1; };

    private:                                                // internal functions

        void Update(bool for_trader = true);

    private:                                                // fields

        Player*    m_player;                                // Player who own of this TradeData
        Player*    m_trader;                                // Player who trade with m_player

        bool       m_accepted;                              // m_player press accept for trade list
        bool       m_acceptProccess;                        // one from player/trader press accept and this processed

        uint64     m_money;                                 // m_player place money to trade

        uint32     m_spell;                                 // m_player apply spell to non-traded slot item
        uint64     m_spellCastItem;                         // applied spell casted by item use

        uint64     m_items[TRADE_SLOT_COUNT];               // traded itmes from m_player side including non-traded slot

        uint32     m_packetExtCount;                        // count of packets sent (needed by trade_status_extended)
        uint32     m_packetExtCountOther;                   // count of packets sent (needed by trade_status_extended)
};

class Player : public Unit, public GridObject<Player>
{
    friend class WorldSession;
    friend void Item::AddToUpdateQueueOf(Player *player);
    friend void Item::RemoveFromUpdateQueueOf(Player *player);
    friend class MapInstanced;
    friend class InstanceMap;
    public:
        explicit Player (WorldSession *session);
        ~Player ();

        void CleanupsBeforeDelete(bool finalCleanup = true);

        void AddToWorld();
        void RemoveFromWorld();

        bool TeleportTo(uint32 mapid, float x, float y, float z, float orientation, uint32 options = 0, bool stuckPort = false, TransportPositionContainer* newTransport = NULL);
        void TeleportOutOfMap(Map *oldMap);

        bool TeleportTo(WorldLocation const &loc, uint32 options = 0, bool stuckPort = false)
        {
            return TeleportTo(loc.GetMapId(), loc.GetPositionX(), loc.GetPositionY(), loc.GetPositionZ(), loc.GetOrientation(), options, stuckPort);
        }

        bool TeleportToBGEntryPoint();

        void SetSummonPoint(uint32 mapid, float x, float y, float z)
        {
            m_summon_expire = time(NULL) + MAX_PLAYER_SUMMON_DELAY;
            m_summon_mapid = mapid;
            m_summon_x = x;
            m_summon_y = y;
            m_summon_z = z;
        }
        void SummonIfPossible(bool agree);

        bool Create(uint32 guidlow, const std::string& name, uint8 race, uint8 class_, uint8 gender, uint8 skin, uint8 face, uint8 hairStyle, uint8 hairColor, uint8 facialHair, uint8 outfitId);

        void Update(uint32 time);

        static bool BuildEnumData(QueryResult result, ByteBuffer* dataBuffer, ByteBuffer* bitBuffer);

        void setClass(uint8 new_class) { m_class = new_class; };
        uint8 getClass() const { return m_class; };

        bool isDisabledPVPAnnounceStatus(){ return disabledPVPAnnounce;}
        void setDisabledPVPAnnounceStatus(bool disabled){ disabledPVPAnnounce = disabled;}

        void SetInWater(bool apply);

        bool IsInWater() const { return m_isInWater; }
        bool IsUnderWater() const;
        bool IsFalling() { return GetPositionZ() < m_lastFallZ; }

        void SendInitialPacketsBeforeAddToMap();
        void SendInitialPacketsAfterAddToMap();
        void SendTransferAborted(uint32 mapid, TransferAbortReason reason, uint8 arg = 0);
        void SendInstanceResetWarning(uint32 mapid, Difficulty difficulty, uint32 time);

        Creature* GetNPCIfCanInteractWith(uint64 guid, uint32 npcflagmask);
        GameObject* GetGameObjectIfCanInteractWith(uint64 guid, GameobjectTypes type) const;

        bool ToggleAFK();
        bool ToggleDND();
        bool isAFK() const { return HasFlag(PLAYER_FLAGS,PLAYER_FLAGS_AFK); }
        bool isDND() const { return HasFlag(PLAYER_FLAGS,PLAYER_FLAGS_DND); }
        uint8 GetChatTag() const;
        std::string afkMsg;
        std::string dndMsg;

        std::string krcma_lastmsg;

        uint32 GetBarberShopCost(uint8 newhairstyle, uint8 newhaircolor, uint8 newfacialhair, BarberShopStyleEntry const* newSkin=NULL);

        PlayerSocial *GetSocial() { return m_social; }

        PlayerTaxi m_taxi;
        void InitTaxiNodesForLevel() { m_taxi.InitTaxiNodesForLevel(getRace(), getClass(), getLevel()); }
        bool ActivateTaxiPathTo(std::vector<uint32> const& nodes, Creature* npc = NULL, uint32 spellid = 0);
        bool ActivateTaxiPathTo(uint32 taxi_path_id, uint32 spellid = 0);
        void CleanupAfterTaxiFlight();
        void ContinueTaxiFlight();
                                                            // mount_id can be used in scripting calls
        bool isAcceptWhispers() const { return m_ExtraFlags & PLAYER_EXTRA_ACCEPT_WHISPERS; }
        void SetAcceptWhispers(bool on) { if (on) m_ExtraFlags |= PLAYER_EXTRA_ACCEPT_WHISPERS; else m_ExtraFlags &= ~PLAYER_EXTRA_ACCEPT_WHISPERS; }
        bool IsGameMaster() const { return m_ExtraFlags & PLAYER_EXTRA_GM_ON; }
        void SetGameMaster(bool on);
        bool isGMChat() const { return GetSession()->GetSecurity() >= SEC_MODERATOR && (m_ExtraFlags & PLAYER_EXTRA_GM_CHAT); }
        void SetGMChat(bool on) { if (on) m_ExtraFlags |= PLAYER_EXTRA_GM_CHAT; else m_ExtraFlags &= ~PLAYER_EXTRA_GM_CHAT; }
        bool isTaxiCheater() const { return m_ExtraFlags & PLAYER_EXTRA_TAXICHEAT; }
        void SetTaxiCheater(bool on) { if (on) m_ExtraFlags |= PLAYER_EXTRA_TAXICHEAT; else m_ExtraFlags &= ~PLAYER_EXTRA_TAXICHEAT; }
        bool isGMVisible() const { return !(m_ExtraFlags & PLAYER_EXTRA_GM_INVISIBLE); }
        void SetGMVisible(bool on);
        bool Has310Flyer(bool checkAllSpells, uint32 excludeSpellId = 0);
        void SetHas310Flyer(bool on) { if (on) m_ExtraFlags |= PLAYER_EXTRA_HAS_310_FLYER; else m_ExtraFlags &= ~PLAYER_EXTRA_HAS_310_FLYER; }
        void SetSendFlyPacket(bool apply);
        void SetPvPDeath(bool on) { if (on) m_ExtraFlags |= PLAYER_EXTRA_PVP_DEATH; else m_ExtraFlags &= ~PLAYER_EXTRA_PVP_DEATH; }
        void SetPvPAnnouncerFlag(bool on) { if (on) m_ExtraFlags |= PLAYER_EXTRA_PVPANNOUNCER; else m_ExtraFlags &= ~PLAYER_EXTRA_PVPANNOUNCER; }

        void GiveXP(uint32 xp, Unit* victim, float group_rate=1.0f);
        void GiveLevel(uint8 level);

        void InitStatsForLevel(bool reapplyMods = false);

        // Played Time Stuff
        time_t m_logintime;
        time_t m_Last_tick;
        uint32 m_Played_time[MAX_PLAYED_TIME_INDEX];
        uint32 GetTotalPlayedTime() const { return m_Played_time[PLAYED_TIME_TOTAL]; }
        uint32 GetLevelPlayedTime() const { return m_Played_time[PLAYED_TIME_LEVEL]; }

        void setDeathState(DeathState s);                   // overwrite Unit::setDeathState

        void InnEnter (time_t time,uint32 mapid, float x,float y,float z)
        {
            inn_pos_mapid = mapid;
            inn_pos_x = x;
            inn_pos_y = y;
            inn_pos_z = z;
            time_inn_enter = time;
        }

        float GetRestBonus() const { return m_rest_bonus; }
        void SetRestBonus(float rest_bonus_new);

        RestType GetRestType() const { return rest_type; }
        void SetRestType(RestType n_r_type) { rest_type = n_r_type; }

        uint32 GetHonorableKills() { return GetUInt16Value(PLAYER_FIELD_KILLS, 0); }
        void BroadcastMessage(const char* str, ...);

        uint32 GetInnPosMapId() const { return inn_pos_mapid; }
        float GetInnPosX() const { return inn_pos_x; }
        float GetInnPosY() const { return inn_pos_y; }
        float GetInnPosZ() const { return inn_pos_z; }

        time_t GetTimeInnEnter() const { return time_inn_enter; }
        void UpdateInnerTime (time_t time) { time_inn_enter = time; }

        Pet* GetPet() const;
        Pet* SummonPet(uint32 entry, float x, float y, float z, float ang, PetType petType, uint32 despwtime, PetSlot slotID);
        void RemovePet(Pet* pet, PetRemoveMode mode = PetRemoveMode::REMOVE_AND_SAVE);
        uint32 GetPhaseMaskForSpawn() const;                // used for proper set phase for DB at GM-mode creature/GO spawn

        void Say(const std::string& text, const uint32 language);
        void Yell(const std::string& text, const uint32 language);
        void TextEmote(const std::string& text);
        void Whisper(const std::string& text, const uint32 language,uint64 receiver);
        void BuildPlayerChat(WorldPacket *data, uint8 msgtype, const std::string& text, uint32 language, const char* addonPrefix = NULL) const;

        void RemoveFakeDeath();

        /*********************************************************/
        /***                    STORAGE SYSTEM                 ***/
        /*********************************************************/

        void SetVirtualItemSlot(uint8 i, Item* item);
        void SetSheath(SheathState sheathed);             // overwrite Unit version
        uint8 FindEquipSlot(ItemPrototype const* proto, uint32 slot, bool swap) const;
        uint32 GetItemCount(uint32 item, bool inBankAlso = false, Item* skipItem = NULL) const;
        uint32 GetItemCountWithLimitCategory(uint32 limitCategory, Item* skipItem = NULL) const;
        Item* GetItemByGuid(uint64 guid) const;
        Item* GetItemByEntry(uint32 entry) const;
        Item* GetItemByPos(uint16 pos) const;
        Item* GetItemByPos(uint8 bag, uint8 slot) const;
        inline Item* GetUseableItemByPos(uint8 bag, uint8 slot) const //Does additional check for disarmed weapons
        {
            if (!CanUseAttackType(GetAttackBySlot(slot)))
                return NULL;
            return GetItemByPos(bag, slot);
        }
        Item* GetWeaponForAttack(WeaponAttackType attackType, bool useable = false) const;
        Item* GetShield(bool useable = false) const;
        static uint8 GetAttackBySlot(uint8 slot);        // MAX_ATTACK if not weapon slot
        std::vector<Item *> &GetItemUpdateQueue() { return m_itemUpdateQueue; }
        static bool IsInventoryPos(uint16 pos) { return IsInventoryPos(pos >> 8,pos & 255); }
        static bool IsInventoryPos(uint8 bag, uint8 slot);
        static bool IsEquipmentPos(uint16 pos) { return IsEquipmentPos(pos >> 8,pos & 255); }
        static bool IsEquipmentPos(uint8 bag, uint8 slot);
        static bool IsBagPos(uint16 pos);
        static bool IsBankPos(uint16 pos) { return IsBankPos(pos >> 8,pos & 255); }
        static bool IsBankPos(uint8 bag, uint8 slot);
        bool IsValidPos(uint16 pos, bool explicit_pos) { return IsValidPos(pos >> 8, pos & 255, explicit_pos); }
        bool IsValidPos(uint8 bag, uint8 slot, bool explicit_pos);
        uint8 GetBankBagSlotCount() const { return GetByteValue(PLAYER_BYTES_2, 2); }
        void SetBankBagSlotCount(uint8 count) { SetByteValue(PLAYER_BYTES_2, 2, count); }
        bool HasItemCount(uint32 item, uint32 count, bool inBankAlso = false) const;
        bool HasItemFitToSpellRequirements(SpellEntry const* spellInfo, Item const* ignoreItem = NULL) const;
        bool CanNoReagentCast(SpellEntry const* spellInfo) const;
        bool HasItemOrGemWithIdEquipped(uint32 item, uint32 count, uint8 except_slot = NULL_SLOT) const;
        bool HasItemOrGemWithLimitCategoryEquipped(uint32 limitCategory, uint32 count, uint8 except_slot = NULL_SLOT) const;
        uint8 CanTakeMoreSimilarItems(Item* pItem) const { return _CanTakeMoreSimilarItems(pItem->GetEntry(),pItem->GetCount(),pItem); }
        uint8 CanTakeMoreSimilarItems(uint32 entry, uint32 count) const { return _CanTakeMoreSimilarItems(entry,count,NULL); }
        uint8 CanStoreNewItem(uint8 bag, uint8 slot, ItemPosCountVec& dest, uint32 item, uint32 count, uint32* no_space_count = NULL) const
        {
            return _CanStoreItem(bag, slot, dest, item, count, NULL, false, no_space_count);
        }
        uint8 CanStoreItem(uint8 bag, uint8 slot, ItemPosCountVec& dest, Item *pItem, bool swap = false) const
        {
            if (!pItem)
                return EQUIP_ERR_ITEM_NOT_FOUND;
            uint32 count = pItem->GetCount();
            return _CanStoreItem(bag, slot, dest, pItem->GetEntry(), count, pItem, swap, NULL);

        }
        uint8 CanStoreItems(Item **pItem,int count) const;
        uint8 CanEquipNewItem(uint8 slot, uint16 &dest, uint32 item, bool swap) const;
        uint8 CanEquipItem(uint8 slot, uint16 &dest, Item *pItem, bool swap, bool not_loading = true) const;

        uint8 CanEquipUniqueItem(Item * pItem, uint8 except_slot = NULL_SLOT, uint32 limit_count = 1) const;
        uint8 CanEquipUniqueItem(ItemPrototype const* itemProto, uint8 except_slot = NULL_SLOT, uint32 limit_count = 1) const;
        uint8 CanUnequipItems(uint32 item, uint32 count) const;
        uint8 CanUnequipItem(uint16 src, bool swap) const;
        uint8 CanBankItem(uint8 bag, uint8 slot, ItemPosCountVec& dest, Item *pItem, bool swap, bool not_loading = true) const;
        uint8 CanUseItem(Item *pItem, bool not_loading = true) const;
        bool HasItemTotemCategory(uint32 TotemCategory) const;
        uint8 CanUseItem(ItemPrototype const *pItem) const;
        uint8 CanUseAmmo(uint32 item) const;
        Item* StoreNewItem(ItemPosCountVec const& pos, uint32 item, bool update, int32 randomPropertyId = 0, AllowedLooterSet* allowedLooters = NULL);
        Item* StoreItem(ItemPosCountVec const& pos, Item *pItem, bool update);
        Item* EquipNewItem(uint16 pos, uint32 item, bool update);
        Item* EquipItem(uint16 pos, Item *pItem, bool update);
        void AutoUnequipOffhandIfNeed(bool force = false);
        bool StoreNewItemInBestSlots(uint32 item_id, uint32 item_count);
        void AutoStoreLoot(uint8 bag, uint8 slot, uint32 loot_id, LootStore const& store, bool broadcast = false, uint16 lootMode = LOOT_MODE_DEFAULT);
        void AutoStoreLoot(uint32 loot_id, LootStore const& store, bool broadcast = false, uint16 lootMode = LOOT_MODE_DEFAULT) { AutoStoreLoot(NULL_BAG,NULL_SLOT,loot_id,store,broadcast,lootMode); }
        void StoreLootItem(uint8 lootSlot, Loot* loot);

        uint8 _CanTakeMoreSimilarItems(uint32 entry, uint32 count, Item* pItem, uint32* no_space_count = NULL) const;
        uint8 _CanStoreItem(uint8 bag, uint8 slot, ItemPosCountVec& dest, uint32 entry, uint32 count, Item *pItem = NULL, bool swap = false, uint32* no_space_count = NULL) const;

        void AddRefundReference(uint32 it);
        void DeleteRefundReference(uint32 it);

        void SendNewCurrency(uint32 id);
        void SendCurrencies();
        void SendUpdateCurrencyWeekCap(uint32 id, uint32 cap);
        uint32 GetCurrency(uint32 id, bool countprecision = false);
        bool HasCurrency(uint32 id, uint32 count, bool countprecision = false);
        void SetCurrency(uint32 id, uint32 count);
        uint32 GetCurrencyWeekCap(const CurrencyTypesEntry* currency, CurrencySource src);
        uint32 GetCurrencyWeekCap(uint32 id, CurrencySource src)
        {
            CurrencyTypesEntry const* curr = sCurrencyTypesStore.LookupEntry(id);
            if (curr)
                return GetCurrencyWeekCap(curr, src);

            return 0;
        }
        uint32 GetCurrencyWeekCount(uint32 id, CurrencySource src);
        uint32 GetCurrencySeasonCount(uint32 id);
        void SetCurrencyWeekCap(uint32 id, CurrencySource src, uint32 cap);
        void SetCurrencyWeekCount(uint32 id, CurrencySource src, uint32 count);
        void SetCurrencySeasonCount(uint32 id, uint32 count);
        void ModifyCurrency(uint32 id, int32 count, CurrencySource src = CURRENCY_SOURCE_ALL, bool ignoreweekcap = false, bool ignorebonuses = false);
        void SendConquestRewards();

        void SendWargameRequest(Player* target, uint64 battlegroundGuid) const;
        void SendWargameRequestSentNotify(Player* target) const;

        void ResetCurrencyWeekCount();

        void SendCompletedArtifacts();
        void DiggedCreature(uint64 guidlow);
        uint32 GetDigCreatureSlot(uint8 slot) { if (slot >= MAX_DIGSITES) return 0; return m_researchSites.site_creature[slot]; };
        void SetNewResearchProject(uint8 slot, bool completed = false);
        bool HasResearchProject(uint32 project);

        bool SaveCastedAuraApplyCondition(Unit* target, const SpellEntry* spell);
        void SaveCastedAuraApply(Aura* pAura);
        bool RemoveCastedAuraApply(Aura* pAura);
        void ProcessCastedAuraApplyMapChange();

        void ApplyEquipCooldown(Item * pItem);
        void SetAmmo(uint32 item);
        void RemoveAmmo();
        float GetAmmoDPS() const { return m_ammoDPS; }
        bool CheckAmmoCompatibility(const ItemPrototype *ammo_proto) const;
        void QuickEquipItem(uint16 pos, Item *pItem);
        void VisualizeItem(uint8 slot, Item *pItem);
        void SetVisibleItemSlot(uint8 slot, Item *pItem);
        Item* BankItem(ItemPosCountVec const& dest, Item *pItem, bool update)
        {
            return StoreItem(dest, pItem, update);
        }
        Item* BankItem(uint16 pos, Item *pItem, bool update);
        void RemoveItem(uint8 bag, uint8 slot, bool update);
        void MoveItemFromInventory(uint8 bag, uint8 slot, bool update);
                                                            // in trade, auction, guild bank, mail....
        void MoveItemToInventory(ItemPosCountVec const& dest, Item* pItem, bool update, bool in_characterInventoryDB = false);
                                                            // in trade, guild bank, mail....
        void RemoveItemDependentAurasAndCasts(Item * pItem);
        void DestroyItem(uint8 bag, uint8 slot, bool update, bool logAction = true);
        void DestroyItemCount(uint32 item, uint32 count, bool update, bool unequip_check = false);
        void DestroyItemCount(Item* item, uint32& count, bool update);
        void DestroyConjuredItems(bool update);
        void DestroyZoneLimitedItem(bool update, uint32 new_zone);
        void SplitItem(uint16 src, uint16 dst, uint32 count);
        void SwapItem(uint16 src, uint16 dst);
        void AddItemToBuyBackSlot(Item *pItem);
        Item* GetItemFromBuyBackSlot(uint32 slot);
        void RemoveItemFromBuyBackSlot(uint32 slot, bool del);
        void SendEquipError(uint8 msg, Item* pItem, Item *pItem2 = NULL, uint32 itemid = 0);
        void SendBuyError(uint8 msg, Creature* pCreature, uint32 item, uint32 param);
        void SendSellError(uint8 msg, Creature* pCreature, uint64 guid, uint32 param);
        void AddWeaponProficiency(uint32 newflag) { m_WeaponProficiency |= newflag; }
        void AddArmorProficiency(uint32 newflag) { m_ArmorProficiency |= newflag; }
        uint32 GetWeaponProficiency() const { return m_WeaponProficiency; }
        uint32 GetArmorProficiency() const { return m_ArmorProficiency; }
        bool IsUseEquipedWeapon(bool mainhand) const
        {
            // disarm applied only to mainhand weapon
            return !IsInFeralForm() && (!mainhand || !HasFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISARMED));
        }
        bool IsTwoHandUsed() const
        {
            Item* mainItem = GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
            return mainItem && mainItem->GetProto()->InventoryType == INVTYPE_2HWEAPON && !CanTitanGrip();
        }
        void SendNewItem(Item *item, uint32 count, bool received, bool created, bool broadcast = false);
        bool BuyItemFromVendorSlot(uint64 vendorguid, uint32 vendorslot, uint32 item, uint8 count, uint8 bag, uint8 slot);
        bool BuyCurrencyFromVendorSlot(uint64 vendorGuid, uint32 vendorSlot, uint32 currency, uint32 count);
        bool _StoreOrEquipNewItem(uint32 vendorslot, uint32 item, uint8 count, uint8 bag, uint8 slot, int64 price, ItemPrototype const *pProto, Creature *pVendor, VendorItem const* crItem, bool bStore);
        void CheckArmorSpecialization();

        float GetReputationPriceDiscount(Creature const* pCreature) const;

        Player* GetTrader() const { return m_trade ? m_trade->GetTrader() : NULL; }
        TradeData* GetTradeData() const { return m_trade; }
        void TradeCancel(bool sendback);

        void UpdateEnchantTime(uint32 time);
        void UpdateSoulboundTradeItems();
        void RemoveTradeableItem(Item* item);
        void UpdateItemDuration(uint32 time, bool realtimeonly = false);
        void AddEnchantmentDurations(Item *item);
        void RemoveEnchantmentDurations(Item *item);
        void RemoveArenaEnchantments(EnchantmentSlot slot);
        void AddEnchantmentDuration(Item *item, EnchantmentSlot slot, uint32 duration);
        void ApplyEnchantment(Item *item, EnchantmentSlot slot, bool apply, bool apply_dur = true, bool ignore_condition = false);
        void ApplyEnchantment(Item *item, bool apply);
        void ApplyReforge(Item *item, bool apply);
        void UpdateSkillEnchantments(uint16 skill_id, uint16 curr_value, uint16 new_value);
        void SendEnchantmentDurations();
        void BuildEnchantmentsInfoData(WorldPacket *data);
        void AddItemDurations(Item *item);
        void RemoveItemDurations(Item *item);
        void SendItemDurations();
        void LoadCorpse();
        void LoadPet();

        bool AddItem(uint32 itemId, uint32 count);

        struct phaseData_t
        {
            std::vector<uint32> worldMapAreas;
            std::vector<uint32> phaseIDs;
            std::vector<uint32> terrainSwapMaps;
        } activePhaseData;

        void UpdateActivePhaseData();

        /*********************************************************/
        /***                    GOSSIP SYSTEM                  ***/
        /*********************************************************/

        void PrepareGossipMenu(WorldObject *pSource, uint32 menuId = 0, bool showQuests = false);
        void SendPreparedGossip(WorldObject *pSource);
        void OnGossipSelect(WorldObject *pSource, uint32 gossipListId, uint32 menuId);

        uint32 GetGossipTextId(uint32 menuId);
        uint32 GetGossipTextId(WorldObject *pSource);
        uint32 GetDefaultGossipMenuForSource(WorldObject *pSource);

        /*********************************************************/
        /***                    QUEST SYSTEM                   ***/
        /*********************************************************/

        int32 GetQuestLevel(Quest const* pQuest) const { return pQuest && (pQuest->GetQuestLevel() > 0) ? pQuest->GetQuestLevel() : getLevel(); }

        void PrepareQuestMenu(uint64 guid);
        void SendPreparedQuest(uint64 guid);
        bool IsActiveQuest(uint32 quest_id) const;
        Quest const *GetNextQuest(uint64 guid, Quest const *pQuest);
        bool CanSeeStartQuest(Quest const *pQuest);
        bool CanTakeQuest(Quest const *pQuest, bool msg);
        bool CanAddQuest(Quest const *pQuest, bool msg);
        bool CanCompleteQuest(uint32 quest_id);
        bool CanCompleteRepeatableQuest(Quest const *pQuest);
        bool CanRewardQuest(Quest const *pQuest, bool msg);
        bool CanRewardQuest(Quest const *pQuest, uint32 reward, bool msg);
        void AddQuest(Quest const *pQuest, Object *questGiver);
        void CompleteQuest(uint32 quest_id);
        void IncompleteQuest(uint32 quest_id);
        void RewardQuest(Quest const *pQuest, uint32 reward, Object* questGiver, bool announce = true);
        void FailQuest(uint32 quest_id);
        bool SatisfyQuestSkillOrClass(Quest const* qInfo, bool msg);
        bool SatisfyQuestLevel(Quest const* qInfo, bool msg);
        bool SatisfyQuestLog(bool msg);
        bool SatisfyQuestPreviousQuest(Quest const* qInfo, bool msg);
        bool SatisfyQuestRace(Quest const* qInfo, bool msg);
        bool SatisfyQuestReputation(Quest const* qInfo, bool msg);
        bool SatisfyQuestStatus(Quest const* qInfo, bool msg);
        bool SatisfyQuestConditions(Quest const* qInfo, bool msg);
        bool SatisfyQuestTimed(Quest const* qInfo, bool msg);
        bool SatisfyQuestExclusiveGroup(Quest const* qInfo, bool msg);
        bool SatisfyQuestNextChain(Quest const* qInfo, bool msg);
        bool SatisfyQuestPrevChain(Quest const* qInfo, bool msg);
        bool SatisfyQuestDay(Quest const* qInfo, bool msg);
        bool SatisfyQuestWeek(Quest const* qInfo, bool msg);
        bool GiveQuestSourceItem(Quest const *pQuest);
        bool TakeQuestSourceItem(uint32 quest_id, bool msg);
        bool GetQuestRewardStatus(uint32 quest_id) const;
        QuestStatus GetQuestStatus(uint32 quest_id) const;
        void SetQuestStatus(uint32 quest_id, QuestStatus status);

        void SetDailyQuestStatus(uint32 quest_id);
        void SetWeeklyQuestStatus(uint32 quest_id);
        void ResetDailyQuestStatus();
        void ResetWeeklyQuestStatus();

        uint16 FindQuestSlot(uint32 quest_id) const;
        uint32 GetQuestSlotQuestId(uint16 slot) const { return GetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_ID_OFFSET); }
        uint32 GetQuestSlotState(uint16 slot)   const { return GetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_STATE_OFFSET); }
        uint16 GetQuestSlotCounter(uint16 slot, uint8 counter) const { return (uint16)(GetUInt64Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_COUNTS_OFFSET) >> (counter * 16)); }
        uint32 GetQuestSlotTime(uint16 slot)    const { return GetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_TIME_OFFSET); }
        void SetQuestSlot(uint16 slot, uint32 quest_id, uint32 timer = 0)
        {
            SetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_ID_OFFSET, quest_id);
            SetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_STATE_OFFSET, 0);
            SetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_COUNTS_OFFSET, 0);
            SetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_COUNTS_OFFSET + 1, 0);
            SetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_TIME_OFFSET, timer);
        }
        void SetQuestSlotCounter(uint16 slot, uint8 counter, uint16 count)
        {
            uint64 val = GetUInt64Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_COUNTS_OFFSET);
            val &= ~((uint64)0xFFFF << (counter * 16));
            val |= ((uint64)count << (counter * 16));
            SetUInt64Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_COUNTS_OFFSET, val);
        }
        void SetQuestSlotState(uint16 slot, uint32 state) { SetFlag(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_STATE_OFFSET, state); }
        void RemoveQuestSlotState(uint16 slot, uint32 state) { RemoveFlag(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_STATE_OFFSET, state); }
        void SetQuestSlotTimer(uint16 slot, uint32 timer) { SetUInt32Value(PLAYER_QUEST_LOG_1_1 + slot * MAX_QUEST_OFFSET + QUEST_TIME_OFFSET, timer); }
        void SwapQuestSlot(uint16 slot1, uint16 slot2)
        {
            for (int i = 0; i < MAX_QUEST_OFFSET; ++i)
            {
                uint32 temp1 = GetUInt32Value(PLAYER_QUEST_LOG_1_1 + MAX_QUEST_OFFSET * slot1 + i);
                uint32 temp2 = GetUInt32Value(PLAYER_QUEST_LOG_1_1 + MAX_QUEST_OFFSET * slot2 + i);

                SetUInt32Value(PLAYER_QUEST_LOG_1_1 + MAX_QUEST_OFFSET * slot1 + i, temp2);
                SetUInt32Value(PLAYER_QUEST_LOG_1_1 + MAX_QUEST_OFFSET * slot2 + i, temp1);
            }
        }
        uint16 GetReqKillOrCastCurrentCount(uint32 quest_id, int32 entry);
        void AreaExploredOrEventHappens(uint32 questId);
        void GroupEventHappens(uint32 questId, WorldObject const* pEventObject);
        void ItemAddedQuestCheck(uint32 entry, uint32 count);
        void ItemRemovedQuestCheck(uint32 entry, uint32 count);
        void CurrencyModifyQuestCheck(uint32 entry);
        void KilledMonster(CreatureInfo const* cInfo, uint64 guid);
        void KilledMonsterCredit(uint32 entry, uint64 guid);
        void CastedCreatureOrGO(uint32 entry, uint64 guid, uint32 spell_id);
        void TalkedToCreature(uint32 entry, uint64 guid);
        void AddQuestObjectiveProgress(uint32 quest_id, uint8 objective_index, uint16 addvalue);
        uint32 GetQuestObjectiveProgress(uint32 quest_id, uint8 objective_index);
        void MoneyChanged(uint64 value);
        void ReputationChanged(FactionEntry const* factionEntry);
        void ReputationChanged2(FactionEntry const* factionEntry);
        bool HasQuestForItem(uint32 itemid) const;
        bool HasQuestForGO(int32 GOId) const;
        void UpdateForQuestWorldObjects();
        bool CanShareQuest(uint32 quest_id) const;

        void SendQuestComplete(uint32 quest_id);
        void SendQuestReward(Quest const *pQuest, uint32 XP, Object* questGiver);
        void SendQuestFailed(uint32 quest_id);
        void SendQuestTimerFailed(uint32 quest_id);
        void SendCanTakeQuestResponse(uint32 msg);
        void SendQuestConfirmAccept(Quest const* pQuest, Player* pReceiver);
        void SendPushToPartyResponse(Player *pPlayer, uint32 msg);
        void SendQuestUpdateAddItem(Quest const* pQuest, uint32 item_idx, uint16 count);
        void SendQuestUpdateAddCreatureOrGo(Quest const* pQuest, uint64 guid, uint32 creatureOrGO_idx, uint16 old_count, uint16 add_count);

        void SendLightChange(uint32 originalLightId, uint32 targetLightId, uint32 transitionTimeMs);

        uint64 GetDivider() { return m_divider; }
        void SetDivider(uint64 guid) { m_divider = guid; }

        uint32 GetInGameTime() { return m_ingametime; }

        void SetInGameTime(uint32 time) { m_ingametime = time; }

        void AddTimedQuest(uint32 quest_id) { m_timedquests.insert(quest_id); }
        void RemoveTimedQuest(uint32 quest_id) { m_timedquests.erase(quest_id); }

        void SaveCUFProfile(uint8 id, CUFProfile* profile) { delete _CUFProfiles[id]; _CUFProfiles[id] = profile; } ///> Replaces a CUF profile at position 0-4
        CUFProfile* GetCUFProfile(uint8 id) const { return _CUFProfiles[id]; } ///> Retrieves a CUF profile at position 0-4
        uint8 GetCUFProfilesCount() const
        {
            uint8 count = 0;
            for (uint8 i = 0; i < MAX_CUF_PROFILES; ++i)
                if (_CUFProfiles[i])
                    ++count;
            return count;
        }

        /*********************************************************/
        /***                   LOAD SYSTEM                     ***/
        /*********************************************************/

        static Player* LoadFromDB(uint32 guid, SQLQueryHolder * holder, WorldSession * session);
        bool _LoadFromDB(uint32 guid, SQLQueryHolder * holder, PreparedQueryResult & result);
        bool isBeingLoaded() const { return GetSession()->PlayerLoading();}

        void Initialize(uint32 guid);
        static uint32 GetUInt32ValueFromArray(Tokens const& data, uint16 index);
        static float  GetFloatValueFromArray(Tokens const& data, uint16 index);
        static uint32 GetZoneIdFromDB(uint64 guid);
        static uint32 GetLevelFromDB(uint64 guid);
        static bool   LoadPositionFromDB(uint32& mapid, float& x,float& y,float& z,float& o, bool& in_flight, uint64 guid);

        void _LoadRatedBGData();

        /*********************************************************/
        /***                   SAVE SYSTEM                     ***/
        /*********************************************************/

        void SaveToDB();
        bool CreateInDB();
        void SaveInventoryAndGoldToDB(SQLTransaction& trans);                    // fast save function for item/money cheating preventing
        void SaveGoldToDB(SQLTransaction& trans);

        void _SaveRatedBGData();

        static void SetUInt32ValueInArray(Tokens& data,uint16 index, uint32 value);
        static void SetFloatValueInArray(Tokens& data,uint16 index, float value);
        static void Customize(uint64 guid, uint8 gender, uint8 skin, uint8 face, uint8 hairStyle, uint8 hairColor, uint8 facialHair);
        static void SavePositionInDB(uint32 mapid, float x,float y,float z,float o,uint32 zone,uint64 guid);

        static void DeleteFromDB(uint64 playerguid, uint32 accountId, bool updateRealmChars = true, bool deleteFinally = false);
        static void DeleteOldCharacters();
        static void DeleteOldCharacters(uint32 keepDays);

        bool m_mailsLoaded;
        bool m_mailsUpdated;

        void SetBindPoint(uint64 guid);
        void SendTalentWipeConfirm(uint64 guid);
        void ResetPetTalents();
        void CalcRage(uint32 damage,bool attacker);
        void RegenerateAll();
        void Regenerate(Powers power);
        void RegenerateHealth();
        void setRegenTimerCount(uint32 time) {m_regenTimerCount = time;}
        void setWeaponChangeTimer(uint32 time) {m_weaponChangeTimer = time;}

        uint64 GetMoney() const { return GetUInt64Value (PLAYER_FIELD_COINAGE); }
        void ModifyMoney(int64 d);
        bool HasEnoughMoney(uint64 amount) const { return (GetMoney() >= amount); }
        bool HasEnoughMoney(int64 amount) const
        {
            if (amount > 0)
                return (GetMoney() >= uint64(amount));
            return true;
        }

        void SetMoney(uint64 value)
        {
            SetUInt64Value (PLAYER_FIELD_COINAGE, value);
            MoneyChanged(value);
            UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED);
        }

        QuestStatusMap& getQuestStatusMap() { return mQuestStatus; };

        const uint64& GetSelection() const { return m_curSelection; }
        Unit *GetSelectedUnit() const;
        Player *GetSelectedPlayer() const;
        void SetSelection(const uint64 &guid) { m_curSelection = guid; SetUInt64Value(UNIT_FIELD_TARGET, guid); }

        uint8 GetComboPoints() { return m_comboPoints; }
        const uint64& GetComboTarget() const { return m_comboTarget; }

        void AddComboPoints(Unit* target, int8 count, Spell * spell = NULL);
        void GainSpellComboPoints(int8 count);
        void ClearComboPoints();
        void SendComboPoints();

        void SendMailResult(uint32 mailId, MailResponseType mailAction, MailResponseResult mailError, uint32 equipError = 0, uint32 item_guid = 0, uint32 item_count = 0);
        void SendNewMail();
        void UpdateNextMailTimeAndUnreads();
        void AddNewMailDeliverTime(time_t deliver_time);
        bool IsMailsLoaded() const { return m_mailsLoaded; }

        void RemoveMail(uint32 id);

        void AddMail(Mail* mail) { m_mail.push_front(mail);}// for call from WorldSession::SendMailTo
        uint32 GetMailSize() { return m_mail.size();}
        Mail* GetMail(uint32 id);

        PlayerMails::iterator GetMailBegin() { return m_mail.begin();}
        PlayerMails::iterator GetMailEnd() { return m_mail.end();}

        /*********************************************************/
        /*** MAILED ITEMS SYSTEM ***/
        /*********************************************************/

        uint8 unReadMails;
        time_t m_nextMailDelivereTime;

        typedef std::unordered_map<uint32, Item*> ItemMap;

        ItemMap mMitems;                                    //template defined in objectmgr.cpp

        Item* GetMItem(uint32 id)
        {
            ItemMap::const_iterator itr = mMitems.find(id);
            return itr != mMitems.end() ? itr->second : NULL;
        }

        void AddMItem(Item* it)
        {
            ASSERT(it);
            //ASSERT deleted, because items can be added before loading
            mMitems[it->GetGUIDLow()] = it;
        }

        bool RemoveMItem(uint32 id)
        {
            return mMitems.erase(id) ? true : false;
        }

        void PetSpellInitialize();
        void CharmSpellInitialize();
        void PossessSpellInitialize();
        void VehicleSpellInitialize();
        void SendRemoveControlBar();
        bool HasSpell(uint32 spell) const;
        bool HasActiveSpell(uint32 spell) const;            // show in spellbook
        TrainerSpellState GetTrainerSpellState(TrainerSpell const* trainer_spell) const;
        bool IsSpellFitByClassAndRace(uint32 spell_id) const;
        bool IsNeedCastPassiveSpellAtLearn(SpellEntry const* spellInfo) const;

        void SendProficiency(ItemClass itemClass, uint32 itemSubclassMask);
        void SendInitialSpells();
        bool AddSpell(uint32 spell_id, bool active, bool learning, bool dependent, bool disabled);
        void LearnSpell(uint32 spell_id, bool dependent);
        void RemoveSpell(uint32 spell_id, bool disabled = false, bool learn_low_rank = true);
        void ResetSpells(bool myClassOnly = false);
        void LearnDefaultSpells();
        void LearnQuestRewardedSpells();
        void LearnQuestRewardedSpells(Quest const* quest);
        void learnSpellHighRank(uint32 spellid);
        void AddTemporarySpell(uint32 spellId);
        void RemoveTemporarySpell(uint32 spellId);
        void SetReputation(uint32 factionentry, uint32 value);
        uint32 GetReputation(uint32 factionentry);
        std::string GetGuildName();
        uint32 GetFreeTalentPoints() const { return m_freeTalentPoints; }
        void SetFreeTalentPoints(uint32 points) { m_freeTalentPoints = points; }
        bool ResetTalents(bool no_cost = false);
        uint64 ResetTalentsCost() const;
        void InitTalentForLevel();
        void BuildPlayerTalentsInfoData(WorldPacket *data);
        void BuildPetTalentsInfoData(WorldPacket *data);
        void SendTalentsInfoData(bool pet);
        void LearnTalent(uint32 talentId, uint32 talentRank, bool one = true);
        void LearnPetTalent(uint64 petGuid, uint32 talentId, uint32 talentRank);

        bool AddTalent(uint32 spell, uint8 spec, bool learning);
        bool HasTalent(uint32 spell_id, uint8 spec) const;

        void SetTalentBranchSpec(uint32 branchSpec, uint8 spec) { m_branchSpec[spec] = branchSpec; }
        BranchSpec GetTalentBranchSpec(uint8 spec) const { return BranchSpec(m_branchSpec[spec]); }
        BranchSpec GetActiveTalentBranchSpec() const { return GetTalentBranchSpec(GetActiveSpec()); }
        bool HasTankSpec();
        bool HasHealingSpec();

        uint32 CalculateTalentsPoints() const;

        // Dual Spec
        void UpdateSpecCount(uint8 count);
        uint32 GetActiveSpec() const { return m_activeSpec; }
        void SetActiveSpec(uint8 spec){ m_activeSpec = spec; }
        uint8 GetSpecsCount() { return m_specsCount; }
        void SetSpecsCount(uint8 count) { m_specsCount = count; }
        void ActivateSpec(uint8 spec);

        void InitGlyphsForLevel();
        void SetGlyphSlot(uint8 slot, uint32 slottype)
        { 
            ASSERT(slot < MAX_GLYPH_SLOT_INDEX); // prevent updatefields corruption
            SetUInt32Value(PLAYER_FIELD_GLYPH_SLOTS_1 + slot, slottype);
        }
        uint32 GetGlyphSlot(uint8 slot) { return GetUInt32Value(PLAYER_FIELD_GLYPH_SLOTS_1 + slot); }
        void SetGlyph(uint8 slot, uint32 glyph)
        {
            m_Glyphs[m_activeSpec][slot] = glyph;
            SetUInt32Value(PLAYER_FIELD_GLYPHS_1 + slot, glyph);
        }
        uint32 GetGlyph(uint8 slot) { return m_Glyphs[m_activeSpec][slot]; }

        uint32 GetFreePrimaryProfessionPoints() const { return GetUInt32Value(PLAYER_CHARACTER_POINTS); }
        void SetFreePrimaryProfessions(uint16 profs) { SetUInt32Value(PLAYER_CHARACTER_POINTS, profs); }
        void InitPrimaryProfessions();

        // queued spells
        bool m_queuedSpell;
        void QueueSpell();
        void CancelQueuedSpell();
        bool HasQueuedSpell();

        PlayerSpellMap const& GetSpellMap() const { return m_spells; }
        PlayerSpellMap      & GetSpellMap()       { return m_spells; }

        SpellCooldowns const& GetSpellCooldownMap() const { return m_spellCooldowns; }

        void AddSpellMod(SpellModifier* mod, bool apply);
        bool IsAffectedBySpellmod(SpellEntry const *spellInfo, SpellModifier *mod, Spell * spell = NULL);
        template <class T> T ApplySpellMod(uint32 spellId, SpellModOp op, T &basevalue, Spell * spell = NULL);
        void RemoveSpellMods(Spell * spell);
        void RestoreSpellMods(Spell *spell, uint32 ownerAuraId=0);
        void DropModCharge(SpellModifier * mod, Spell * spell);
        void SetSpellModTakingSpell(Spell* spell, bool apply);

        static uint32 const infinityCooldownDelay = uint32(MONTH) * uint32(IN_MILLISECONDS);  // used for set "infinity cooldowns" for spells and check
        static uint32 const infinityCooldownDelayCheck = MONTH / 2 * IN_MILLISECONDS;
        bool HasSpellCooldown(uint32 spell_id) const;
        bool HasPetSpellCooldown(uint32 spell_id) const;
        uint32 GetSpellCooldownDelay(uint32 spell_id) const;    // remaining spell cooldown in milliseconds
        void AddSpellAndCategoryCooldowns(SpellEntry const* spellInfo, uint32 itemId, Spell* spell = NULL, bool infinityCooldown = false);
        void AddSpellCooldown(uint32 spell_id, uint32 itemid, uint32 msDuration);
        void AddPetSpellCooldown(uint32 spell_id, uint32 msDuration);
        void SendCooldownEvent(SpellEntry const *spellInfo, uint32 itemId = 0, Spell* spell = NULL);
        void ProhibitSpellScholl(SpellSchoolMask idSchoolMask, uint32 unTimeMs);
        void RemoveSpellCooldown(uint32 spell_id, bool update = false);
        void RemovePetSpellCooldown(uint32 spell_id);
        void RemoveSpellCategoryCooldown(uint32 cat, bool update = false);
        void SendClearCooldown(uint32 spell_id, Unit* target);
        void SendClearAllCooldowns(Unit* target);
        void ModifySpellCooldown(uint32 spell_id, int32 mod, bool update = true);

        void RemoveCategoryCooldown(uint32 cat);
        void RemoveArenaSpellCooldowns(bool removeActivePetCooldowns = false);
        void RemoveAllSpellCooldown();
        void _LoadSpellCooldowns(PreparedQueryResult result);
        void _SaveSpellCooldowns(SQLTransaction& trans);
        void SetLastPotionId(uint32 item_id) { m_lastPotionId = item_id; }
        void UpdatePotionCooldown(Spell* spell = NULL);

        // global cooldown
        void AddGlobalCooldown(SpellEntry const *spellInfo, Spell *spell);
        bool HasGlobalCooldown(SpellEntry const *spellInfo) const;
        void RemoveGlobalCooldown(SpellEntry const *spellInfo);
        uint32 GetGlobalCooldown(SpellEntry const *spellInfo);

        void SetResurrectRequestData(uint64 guid, uint32 mapId, float X, float Y, float Z, uint32 health, uint32 mana)
        {
            m_resurrectGUID = guid;
            m_resurrectMap = mapId;
            m_resurrectX = X;
            m_resurrectY = Y;
            m_resurrectZ = Z;
            m_resurrectHealth = health;
            m_resurrectMana = mana;
        }
        void ClearResurrectRequestData() { SetResurrectRequestData(0,0,0.0f,0.0f,0.0f,0,0); }
        bool IsRessurectRequestedBy(uint64 guid) const { return m_resurrectGUID == guid; }
        bool IsRessurectRequested() const { return m_resurrectGUID != 0; }
        void ResurectUsingRequestData();

        bool FallGround(uint8 FallMode);

        uint8 getCinematic()
        {
            return m_cinematic;
        }
        void setCinematic(uint8 cine)
        {
            m_cinematic = cine;
        }

        ActionButton* addActionButton(uint8 button, uint32 action, uint8 type);
        void removeActionButton(uint8 button);
        ActionButton const* GetActionButton(uint8 button);
        void SendInitialActionButtons() const { SendActionButtons(1); }
        void SendActionButtons(uint32 state) const;
        bool IsActionButtonDataValid(uint8 button, uint32 action, uint8 type);

        PvPInfo pvpInfo;
        void UpdatePvPState(bool onlyFFA = false);
        void SetPvP(bool state)
        {
            Unit::SetPvP(state);
            for (ControlList::iterator itr = m_Controlled.begin(); itr != m_Controlled.end(); ++itr)
                (*itr)->SetPvP(state);
        }
        void UpdatePvP(bool state, bool override=false);
        void UpdateZone(uint32 newZone,uint32 newArea);
        void UpdateArea(uint32 newArea);

        void UpdateZoneDependentAuras(uint32 zone_id);    // zones
        void UpdateAreaDependentAuras(uint32 area_id);    // subzones

        void UpdateAfkReport(time_t currTime);
        void UpdatePvPOnTimer(time_t diff);
        void UpdatePvPFlag(time_t currTime);
        void UpdateContestedPvP(uint32 currTime);
        void SetContestedPvPTimer(uint32 newTime) {m_contestedPvPTimer = newTime;}
        void ResetContestedPvP()
        {
            ClearUnitState(UNIT_STATE_ATTACK_PLAYER);
            RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_CONTESTED_PVP);
            m_contestedPvPTimer = 0;
        }

        /** todo: -maybe move UpdateDuelFlag+DuelComplete to independent DuelHandler.. **/
        DuelInfo *duel;
        void UpdateDuelFlag(time_t currTime);
        void CheckDuelDistance(time_t currTime);
        void DuelComplete(DuelCompleteType type);
        void SendDuelCountdown(uint32 counter);

        bool IsGroupVisibleFor(Player const* p) const;
        bool IsInSameGroupWith(Player const* p) const;
        bool IsInSameRaidWith(Player const* p) const { return p == this || (GetGroup() != NULL && GetGroup() == p->GetGroup()); }
        void UninviteFromGroup();
        static void RemoveFromGroup(Group* group, uint64 guid, RemoveMethod method = GROUP_REMOVEMETHOD_DEFAULT, uint64 kicker = 0 , const char* reason = NULL);
        void RemoveFromGroup(RemoveMethod method = GROUP_REMOVEMETHOD_DEFAULT) { RemoveFromGroup(GetGroup(),GetGUID(), method); }
        void SendUpdateToOutOfRangeGroupMembers();
        void RemoveMarkerForPlayer(Player * raidLeader);

        void SetInGuild(uint32 GuildId);
        void SetRank(uint8 rankId) { SetUInt32Value(PLAYER_GUILDRANK, rankId); }
        uint8 GetRank() { return uint8(GetUInt32Value(PLAYER_GUILDRANK)); }
        void SetGuildIdInvited(uint32 GuildId) { m_GuildIdInvited = GuildId; }
        uint32 GetGuildId() { return m_guildId; }
        Guild *GetGuild();
        static uint32 GetGuildIdFromDB(uint64 guid);
        static uint8 GetRankFromDB(uint64 guid);
        int GetGuildIdInvited() { return m_GuildIdInvited; }
        static void RemovePetitionsAndSigns(uint64 guid, uint32 type);

        void AddGuildNews(uint32 type, uint64 param);

        void SetLastBattlegroundTypeId(uint32 value) { m_lastBattlegroundTypeId = value; }
        uint32 GetLastBattlegroundTypeId() { return m_lastBattlegroundTypeId; }

        // Arena Team
        void SetInArenaTeam(uint32 ArenaTeamId, uint8 slot, uint8 type)
        {
            SetArenaTeamInfoField(slot, ARENA_TEAM_ID, ArenaTeamId);
            SetArenaTeamInfoField(slot, ARENA_TEAM_TYPE, type);
        }
        void SetArenaTeamInfoField(uint8 slot, ArenaTeamInfoType type, uint32 value)
        {
            SetUInt32Value(PLAYER_FIELD_ARENA_TEAM_INFO_1_1 + (slot * ARENA_TEAM_END) + type, value);
        }
        uint32 GetArenaTeamId(uint8 slot) { if (slot < 3) return GetUInt32Value(PLAYER_FIELD_ARENA_TEAM_INFO_1_1 + (slot * ARENA_TEAM_END) + ARENA_TEAM_ID); else return 0;}
        uint32 GetArenaPersonalRating(uint8 slot) { return GetUInt32Value(PLAYER_FIELD_ARENA_TEAM_INFO_1_1 + (slot * ARENA_TEAM_END) + ARENA_TEAM_PERSONAL_RATING); }
        static uint32 GetArenaTeamIdFromDB(uint64 guid, uint8 slot);
        void SetArenaTeamIdInvited(uint32 ArenaTeamId) { m_ArenaTeamIdInvited = ArenaTeamId; }
        uint32 GetArenaTeamIdInvited() { return m_ArenaTeamIdInvited; }
        static void LeaveAllArenaTeams(uint64 guid);

        // Research mechanisms (Archaeology)
        uint16 GetResearchSite(uint8 slot);
        void SetResearchSite(uint8 slot, uint16 siteId);

        Difficulty GetDifficulty(bool isRaid) const { return isRaid ? m_raidDifficulty : m_dungeonDifficulty; }
        Difficulty GetDungeonDifficulty() const { return m_dungeonDifficulty; }
        Difficulty GetRaidDifficulty() const { return m_raidDifficulty; }
        Difficulty GetStoredRaidDifficulty() const { return m_raidMapDifficulty; } // only for use in difficulty packet after exiting to raid map
        void SetDungeonDifficulty(Difficulty dungeon_difficulty) { m_dungeonDifficulty = dungeon_difficulty; }
        void SetRaidDifficulty(Difficulty raid_difficulty) { m_raidDifficulty = raid_difficulty; }
        void StoreRaidMapDifficulty() { m_raidMapDifficulty = GetMap()->GetDifficulty(); }

        bool UpdateSkill(uint32 skill_id, uint32 step);
        bool UpdateSkillPro(uint16 SkillId, int32 Chance, uint32 step);

        bool UpdateCraftSkill(uint32 spellid);
        bool UpdateGatherSkill(uint32 SkillId, uint32 SkillValue, uint32 RedLevel, uint32 Multiplicator = 1);
        bool UpdateFishingSkill();

        uint32 GetBaseDefenseSkillValue() const { return GetBaseSkillValue(SKILL_DEFENSE); }
        uint32 GetBaseWeaponSkillValue(WeaponAttackType attType) const;

        uint32 GetSpellByProto(ItemPrototype *proto);

        float GetHealthBonusFromStamina();
        float GetManaBonusFromIntellect();

        bool UpdateStats(Stats stat);
        bool UpdateAllStats();
        void UpdateResistances(uint32 school);
        void UpdateArmor();
        void UpdateSpellPower();
        void UpdateMaxHealth();
        void UpdateMaxPower(Powers power);
        void UpdateClassSpecificPowers();
        void ApplyFeralAPBonus(int32 amount, bool apply);
        void UpdateAttackPowerAndDamage(bool ranged = false);
        void UpdateShieldBlockValue();
        void UpdateDamagePhysical(WeaponAttackType attType);
        void ApplySpellPowerBonus(int32 amount, bool apply);
        void UpdateSpellDamageAndHealingBonus();
        void ApplyRatingMod(CombatRating cr, int32 value, bool apply);
        void UpdateRating(CombatRating cr);
        void UpdateAllRatings();

        bool IsInEclipse();
        bool IsEclipseDriverLeft() { return m_eclipseDriverLeft; };
        void TurnEclipseDriverLeft(bool left);
        void ClearEclipseState();

        void CalculateMinMaxDamage(WeaponAttackType attType, bool normalized, bool addTotalPct, float& min_damage, float& max_damage);

        void UpdateDefenseBonusesMod();
        inline void RecalculateRating(CombatRating cr) { ApplyRatingMod(cr, 0, true);}
        float GetMeleeCritFromAgility();
        void  GetDodgeFromAgility(float &diminishing, float &nondiminishing);
        float GetSpellCritFromIntellect();
        float OCTRegenMPPerSpirit();
        float OCTHpPerStamina();
        float GetRatingCoefficient(CombatRating cr) const;
        float GetRatingBonusValue(CombatRating cr) const;
        uint32 GetBaseSpellPowerBonus() const { return m_spellPowerFromIntellect + m_baseSpellPower; }
        int32 GetSpellPenetrationItemMod() const { return m_spellPenetrationItemMod; }

        float GetExpertiseDodgeOrParryReduction(WeaponAttackType attType) const;
        void UpdateBlockPercentage();
        void UpdateCritPercentage(WeaponAttackType attType);
        void UpdateAllCritPercentages();
        void UpdateParryPercentage();
        void UpdateDodgePercentage();
        void UpdateMeleeHitChances();
        void UpdateRangedHitChances();
        void UpdateSpellHitChances();

        void UpdateMastery();
        bool HasMastery();
        // Mastery points are always +8 - starting bonus
        float GetMasteryPoints() const { return GetRatingBonusValue(CR_MASTERY) + GetTotalAuraModifier(SPELL_AURA_MOD_MASTERY); }

        uint32 GetMountCapabilityIndex(uint32 amount);

        void UpdateAllSpellCritChances();
        void UpdateSpellCritChance(uint32 school);
        void UpdateArmorPenetration(int32 amount);
        void UpdateExpertise(WeaponAttackType attType);
        void ApplyManaRegenBonus(int32 amount, bool apply);
        void ApplyHealthRegenBonus(int32 amount, bool apply);
        void UpdateManaRegen();
        void UpdateHaste();

        const uint64& GetLootGUID() const { return m_lootGuid; }
        void SetLootGUID(const uint64 &guid) { m_lootGuid = guid; }

        void RemovedInsignia(Player* looterPlr);

        WorldSession* GetSession() const { return m_session; }

        void BuildCreateUpdateBlockForPlayer(UpdateData *data, Player *target) const;
        void DestroyForPlayer(Player *target, bool anim = false) const;
        void SendLogXPGain(uint32 GivenXP, Unit* victim, uint32 BonusXP, bool recruitAFriend = false, float group_rate=1.0f);

        // notifiers
        void SendAttackSwingCantAttack();
        void SendAttackSwingCancelAttack();
        void SendAttackSwingDeadTarget();
        void SendAttackSwingNotInRange();
        void SendAttackSwingBadFacingAttack();
        void SendAutoRepeatCancel(Unit *target);
        void SendExplorationExperience(uint32 Area, uint32 Experience);

        void SendDungeonDifficulty(bool IsInGroup);
        void SendRaidDifficulty(bool IsInGroup, int32 forcedDifficulty = -1);
        void ResetInstances(uint8 method, bool isRaid);
        void SendResetInstanceSuccess(uint32 MapId);
        void SendResetInstanceFailed(uint32 reason, uint32 MapId);
        void SendResetFailedNotify(uint32 mapid);

        virtual bool SetPosition(float x, float y, float z, float orientation, bool teleport = false);
        bool SetPosition(const Position &pos, bool teleport = false) { return SetPosition(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation(), teleport); }
        void UpdateUnderwaterState(Map * m, float x, float y, float z);

        void SendMessageToSet(WorldPacket *data, bool self);// overwrite Object::SendMessageToSet
        void SendMessageToSetInRange(WorldPacket *data, float fist, bool self);// overwrite Object::SendMessageToSetInRange
        void SendMessageToSetInRange(WorldPacket *data, float dist, bool self, bool own_team_only);
        void SendMessageToSet(WorldPacket *data, Player const* skipped_rcvr);

        void SendTeleportPacket(Position &oldPos);
        void SendTeleportAckPacket();

        Corpse *GetCorpse() const;
        void SpawnCorpseBones();
        void CreateCorpse();
        void KillPlayer();
        uint32 GetResurrectionSpellId();
        void ResurrectPlayer(float restore_percent, bool applySickness = false);
        void BuildPlayerRepop();
        void RepopAtGraveyard();
        void ReturnToGraveyard();

        void DurabilityLossAll(double percent, bool inventory);
        void DurabilityLoss(Item* item, double percent);
        void DurabilityPointsLossAll(int32 points, bool inventory);
        void DurabilityPointsLoss(Item* item, int32 points);
        void DurabilityPointLossForEquipSlot(EquipmentSlots slot);
        uint32 DurabilityRepairAll(bool cost, float discountMod, bool guildBank);
        uint32 DurabilityRepair(uint16 pos, bool cost, float discountMod, bool guildBank);

        void UpdateMirrorTimers();
        void StopMirrorTimers()
        {
            StopMirrorTimer(FATIGUE_TIMER);
            StopMirrorTimer(BREATH_TIMER);
            StopMirrorTimer(FIRE_TIMER);
        }

        void SendTotemCreateInfo(SpellEntry const * spell, uint32 duration, uint32 entry, uint32 slot, uint64 lowGUID);

        bool CanJoinConstantChannelInZone(ChatChannelsEntry const* channel, AreaTableEntry const* zone);

        void JoinedChannel(Channel *c);
        void LeftChannel(Channel *c);
        void CleanupChannels();
        void UpdateLocalChannels(uint32 newZone);
        void LeaveLFGChannel();

        void UpdateDefense();
        void UpdateWeaponSkill (WeaponAttackType attType);
        void UpdateCombatSkills(Unit *pVictim, WeaponAttackType attType, bool defence);

        void SetSkill(uint16 id, uint16 step, uint16 currVal, uint16 maxVal);
        uint16 GetMaxSkillValue(uint32 skill) const;        // max + perm. bonus + temp bonus
        uint16 GetPureMaxSkillValue(uint32 skill) const;    // max
        uint16 GetSkillValue(uint32 skill) const;           // skill value + perm. bonus + temp bonus
        uint16 GetBaseSkillValue(uint32 skill) const;       // skill value + perm. bonus
        uint16 GetPureSkillValue(uint32 skill) const;       // skill value
        int16 GetSkillPermBonusValue(uint32 skill) const;
        int16 GetSkillTempBonusValue(uint32 skill) const;
        uint16 GetSkillStep(uint16 skill) const;            // 0...6
        bool HasSkill(uint32 skill) const;
        void LearnSkillRewardedSpells(uint32 id, uint32 value);
        void RemoveSpecializationAuras(void);

        WorldLocation& GetTeleportDest() { return m_teleport_dest; }
        bool IsBeingTeleported() const { return mSemaphoreTeleport_Near || mSemaphoreTeleport_Far; }
        bool IsBeingTeleportedNear() const { return mSemaphoreTeleport_Near; }
        bool IsBeingTeleportedFar() const { return mSemaphoreTeleport_Far; }
        void SetSemaphoreTeleportNear(bool semphsetting) { mSemaphoreTeleport_Near = semphsetting; }
        void SetSemaphoreTeleportFar(bool semphsetting) { mSemaphoreTeleport_Far = semphsetting; }
        void ProcessDelayedOperations();

        void CheckAreaExploreAndOutdoor(void);

        static uint32 TeamForRace(uint8 race);
        uint32 GetTeam() const { return m_team; }
        TeamId GetTeamId() const { return m_team == ALLIANCE ? TEAM_ALLIANCE : TEAM_HORDE; }
        static uint32 getFactionForRace(uint8 race);
        void setFactionForRace(uint8 race);

        void InitDisplayIds();

        bool IsAtGroupRewardDistance(WorldObject const* pRewardSource) const;
        bool IsAtRecruitAFriendDistance(WorldObject const* pOther) const;
        bool RewardPlayerAndGroupAtKill(Unit* pVictim);
        bool CheckIfPlayersInInstanceAreInGroup(Group* group, const Map::PlayerList& pList);
        void RewardPlayerAndGroupAtEvent(uint32 creature_id,WorldObject* pRewardSource);
        bool isHonorOrXPTarget(Unit* pVictim);

        bool GetsRecruitAFriendBonus(bool forXP);
        uint8 GetGrantableLevels() { return GetByteValue(PLAYER_FIELD_BYTES, 1); }

        ReputationMgr&       GetReputationMgr()       { return m_reputationMgr; }
        ReputationMgr const& GetReputationMgr() const { return m_reputationMgr; }
        ReputationRank GetReputationRank(uint32 faction_id) const;
        void RewardReputation(Unit *pVictim, float rate);
        void RewardReputation(Quest const *pQuest);
        void RewardCurrency(Unit *pVictim);

        void UpdateSkillsForLevel();
        void UpdateSkillsToMaxSkillsForLevel();             // for .levelup
        void ModifySkillBonus(uint32 skillid,int32 val, bool talent);

        /*********************************************************/
        /***                  PVP SYSTEM                       ***/
        /*********************************************************/
        void UpdateHonorFields();
        bool RewardHonor(Unit *pVictim, uint32 groupsize, int32 honor = -1, bool pvptoken = false);
        uint32 GetMaxPersonalArenaRatingRequirement(uint32 minarenaslot);
        
		//End of PvP System

        inline SpellCooldowns GetSpellCooldowns() const { return m_spellCooldowns; }

        void SetDrunkValue(uint16 newDrunkValue, uint32 itemid=0);
        uint16 GetDrunkValue() const { return m_drunk; }
        static DrunkenState GetDrunkenstateByValue(uint16 value);

        uint32 GetDeathTimer() const { return m_deathTimer; }
        uint32 GetCorpseReclaimDelay(bool pvp) const;
        void UpdateCorpseReclaimDelay();
        void SendCorpseReclaimDelay(bool load = false);

        bool CanParry() const { return m_canParry; }
        void SetCanParry(bool value);
        bool CanBlock() const { return m_canBlock; }
        void SetCanBlock(bool value);
        bool CanTitanGrip() const { return m_canTitanGrip; }
        void SetCanTitanGrip(bool value) { m_canTitanGrip = value; }
        bool CanTameExoticPets() const { return IsGameMaster() || HasAuraType(SPELL_AURA_ALLOW_TAME_PET_TYPE); }

        void SetRegularAttackTime();
        void SetBaseModValue(BaseModGroup modGroup, BaseModType modType, float value) { m_auraBaseMod[modGroup][modType] = value; }
        void HandleBaseModValue(BaseModGroup modGroup, BaseModType modType, float amount, bool apply);
        float GetBaseModValue(BaseModGroup modGroup, BaseModType modType) const;
        float GetTotalBaseModValue(BaseModGroup modGroup) const;
        float GetTotalPercentageModValue(BaseModGroup modGroup) const { return m_auraBaseMod[modGroup][FLAT_MOD] + m_auraBaseMod[modGroup][PCT_MOD]; }
        void _ApplyAllStatBonuses();
        void _RemoveAllStatBonuses();

        void ResetAllPowers();

        void _ApplyWeaponDependentAuraMods(Item *item, WeaponAttackType attackType, bool apply);
        void _ApplyWeaponDependentAuraCritMod(Item *item, WeaponAttackType attackType, AuraEffect const * aura, bool apply);
        void _ApplyWeaponDependentAuraDamageMod(Item *item, WeaponAttackType attackType, AuraEffect const * aura, bool apply);

        void _ApplyItemMods(Item *item,uint8 slot,bool apply);
        void _RemoveAllItemMods();
        void _ApplyAllItemMods();
        void _ApplyAllLevelScaleItemMods(bool apply);
        void _ApplyItemBonuses(ItemPrototype const *proto,uint8 slot,bool apply, bool only_level_scale = false);
        void _ApplyWeaponDamage(uint8 slot, ItemPrototype const *proto, ScalingStatValuesEntry const *ssv, bool apply);
        void _ApplyAmmoBonuses();
        bool EnchantmentFitsRequirements(uint32 enchantmentcondition, int8 slot);
        void ToggleMetaGemsActive(uint8 exceptslot, bool apply);
        void CorrectMetaGemEnchants(uint8 slot, bool apply);
        void InitDataForForm(bool reapplyMods = false);

        void ApplyItemEquipSpell(Item *item, bool apply, bool form_change = false);
        void ApplyEquipSpell(SpellEntry const* spellInfo, Item* item, bool apply, bool form_change = false);
        void UpdateEquipSpellsAtFormChange();
        void CastItemCombatSpell(Unit *target, WeaponAttackType attType, uint32 procVictim, uint32 procEx, SpellEntry const * triggerSpell = NULL);
        void CastItemUseSpell(Item *item,SpellCastTargets const& targets,uint8 cast_count);
        void CastItemCombatSpell(Unit *target, WeaponAttackType attType, uint32 procVictim, uint32 procEx, Item *item, ItemPrototype const * proto, SpellEntry const * triggerSpell = NULL);

        void SendEquipmentSetList();
        void SetEquipmentSet(uint32 index, EquipmentSet eqset);
        void DeleteEquipmentSet(uint64 setGuid);

        void SetEmoteState(uint32 anim_id);
        uint32 GetEmoteState() { return m_emote; }

        void SendInitWorldStates(uint32 zone, uint32 area);
        void SendUpdateWorldState(uint32 Field, uint32 Value);
        void SendDirectMessage(WorldPacket *data);
        void SendBGWeekendWorldStates();

        void SendAurasForTarget(Unit *target);

        PlayerMenu* PlayerTalkClass;
        std::vector<ItemSetEffect *> ItemSetEff;

        void SendLoot(uint64 guid, LootType loot_type);
        void SendLootRelease(uint64 guid);
        void SendNotifyLootItemRemoved(uint8 lootSlot);
        void SendNotifyLootMoneyRemoved();

        /*********************************************************/
        /***               BATTLEGROUND SYSTEM                 ***/
        /*********************************************************/

        bool InBattleground()       const                { return m_bgData.bgInstanceID != 0; }
        bool InArena()              const;
        uint32 GetBattlegroundId()  const                { return m_bgData.bgInstanceID; }
        BattlegroundTypeId GetBattlegroundTypeId() const { return m_bgData.bgTypeID; }
        Battleground* GetBattleground() const;
        uint8 GetTwinkType()        const;

        uint32 GetBattlegroundQueueJoinTime(uint32 bgTypeId) const
        {
            std::map<uint32, uint32>::const_iterator itr = m_bgData.bgQueuesJoinedTime.find(bgTypeId);
            if (itr != m_bgData.bgQueuesJoinedTime.end())
                return itr->second;

            return 0;
        }
        void AddBattlegroundQueueJoinTime(uint32 bgTypeId, uint32 joinTime)
        {
            m_bgData.bgQueuesJoinedTime[bgTypeId] = joinTime;
        }
        void RemoveBattlegroundQueueJoinTime(uint32 bgTypeId)
        {
            m_bgData.bgQueuesJoinedTime.erase(m_bgData.bgQueuesJoinedTime.find(bgTypeId)->second);
        }

        bool InBattlegroundQueue() const
        {
            for (uint8 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
                if (m_bgBattlegroundQueueID[i].bgQueueTypeId != BATTLEGROUND_QUEUE_NONE)
                    return true;
            return false;
        }

        BattlegroundQueueTypeId GetBattlegroundQueueTypeId(uint32 index) const { return m_bgBattlegroundQueueID[index].bgQueueTypeId; }
        uint32 GetBattlegroundQueueIndex(BattlegroundQueueTypeId bgQueueTypeId) const
        {
            for (uint8 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
                if (m_bgBattlegroundQueueID[i].bgQueueTypeId == bgQueueTypeId)
                    return i;
            return PLAYER_MAX_BATTLEGROUND_QUEUES;
        }
        bool IsInvitedForBattlegroundQueueType(BattlegroundQueueTypeId bgQueueTypeId) const
        {
            for (uint8 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
                if (m_bgBattlegroundQueueID[i].bgQueueTypeId == bgQueueTypeId)
                    return m_bgBattlegroundQueueID[i].invitedToInstance != 0;
            return false;
        }
        bool IsWargameForBattlegroundQueueType(BattlegroundQueueTypeId bgQueueTypeId) const
        {
            for (uint8 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
                if (m_bgBattlegroundQueueID[i].bgQueueTypeId == bgQueueTypeId)
                    return m_bgBattlegroundQueueID[i].isWargame;
            return false;
        }
        bool InBattlegroundQueueForBattlegroundQueueType(BattlegroundQueueTypeId bgQueueTypeId) const
        {
            return GetBattlegroundQueueIndex(bgQueueTypeId) < PLAYER_MAX_BATTLEGROUND_QUEUES;
        }

        void SetBattlegroundId(uint32 val, BattlegroundTypeId bgTypeId)
        {
            m_bgData.bgInstanceID = val;
            m_bgData.bgTypeID = bgTypeId;
        }
        uint32 AddBattlegroundQueueId(BattlegroundQueueTypeId val, bool isWargame = false)
        {
            for (uint8 i=0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
            {
                if (m_bgBattlegroundQueueID[i].bgQueueTypeId == BATTLEGROUND_QUEUE_NONE || m_bgBattlegroundQueueID[i].bgQueueTypeId == val)
                {
                    m_bgBattlegroundQueueID[i].bgQueueTypeId = val;
                    m_bgBattlegroundQueueID[i].invitedToInstance = 0;
                    m_bgBattlegroundQueueID[i].isWargame = isWargame;
                    return i;
                }
            }
            return PLAYER_MAX_BATTLEGROUND_QUEUES;
        }
        bool HasFreeBattlegroundQueueId()
        {
            for (uint8 i=0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
                if (m_bgBattlegroundQueueID[i].bgQueueTypeId == BATTLEGROUND_QUEUE_NONE)
                    return true;
            return false;
        }
        void RemoveBattlegroundQueueId(BattlegroundQueueTypeId val)
        {
            for (uint8 i=0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
            {
                if (m_bgBattlegroundQueueID[i].bgQueueTypeId == val)
                {
                    m_bgBattlegroundQueueID[i].bgQueueTypeId = BATTLEGROUND_QUEUE_NONE;
                    m_bgBattlegroundQueueID[i].invitedToInstance = 0;
                    m_bgBattlegroundQueueID[i].isWargame = false;
                    return;
                }
            }
        }
        void SetInviteForBattlegroundQueueType(BattlegroundQueueTypeId bgQueueTypeId, uint32 instanceId)
        {
            for (uint8 i=0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
                if (m_bgBattlegroundQueueID[i].bgQueueTypeId == bgQueueTypeId)
                    m_bgBattlegroundQueueID[i].invitedToInstance = instanceId;
        }
        bool IsInvitedForBattlegroundInstance(uint32 instanceId) const
        {
            for (uint8 i=0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
                if (m_bgBattlegroundQueueID[i].invitedToInstance == instanceId)
                    return true;
            return false;
        }
        WorldLocation const& GetBattlegroundEntryPoint() const { return m_bgData.joinPos; }
        void SetBattlegroundEntryPoint();

        void SetBGTeam(uint32 team) { m_bgData.bgTeam = team; }
        uint32 GetBGTeam() const { return m_bgData.bgTeam ? m_bgData.bgTeam : GetTeam(); }

        void LeaveBattleground(bool teleportToEntryPoint = true, bool CastDeserter = true);
        bool CanJoinToBattleground() const;
        bool CanReportAfkDueToLimit();
        void ReportedAfkBy(Player* reporter);
        void ClearAfkReports() { m_bgData.bgAfkReporter.clear(); }

        bool GetBGAccessByLevel(BattlegroundTypeId bgTypeId) const;
        bool isTotalImmunity();
        bool CanUseBattlegroundObject();
        void RemoveAurasDueToBattlegroundObjectUse();
        bool isTotalImmune();
        bool CanCaptureTowerPoint();

        bool GetRandomWinner() { return m_IsBGRandomWinner; }
        void SetRandomWinner(bool isWinner);

        uint32 GetRatedBattlegroundStat(uint8 index) const { return m_ratedBgStats[index]; }
        uint32 GetRatedBattlegroundRating() const { return GetUInt32Value(PLAYER_FIELD_BATTLEGROUND_RATING); }

        void AddRatedBattlegroundStat(uint8 index) { m_ratedBgStats[index]++; }
        void SetRatedBattlegroundRating(uint32 value) { SetUInt32Value(PLAYER_FIELD_BATTLEGROUND_RATING, value); }

        /*********************************************************/
        /***               OUTDOOR PVP SYSTEM                  ***/
        /*********************************************************/

        OutdoorPvP * GetOutdoorPvP() const;
        // returns true if the player is in active state for outdoor pvp objective capturing, false otherwise
        bool IsOutdoorPvPActive();

        /*********************************************************/
        /***                    REST SYSTEM                    ***/
        /*********************************************************/

        bool isRested() const { return GetRestTime() >= 10*IN_MILLISECONDS; }
        uint32 GetXPRestBonus(uint32 xp);
        uint32 GetRestTime() const { return m_restTime;}
        void SetRestTime(uint32 v) { m_restTime = v;}

        /*********************************************************/
        /***              ENVIROMENTAL SYSTEM                  ***/
        /*********************************************************/

        uint32 EnvironmentalDamage(EnviromentalDamage type, uint32 damage);

        /*********************************************************/
        /***               FLOOD FILTER SYSTEM                 ***/
        /*********************************************************/

        void UpdateSpeakTime();
        bool CanSpeak() const;
        void ChangeSpeakTime(int utime);

        /*********************************************************/
        /***                 VARIOUS SYSTEMS                   ***/
        /*********************************************************/
        void UpdateFallInformationIfNeed(MovementInfo const& minfo, uint32 opcode);
        Unit *m_mover;
        WorldObject *m_seer;
        void SetFallInformation(uint32 time, float z)
        {
            m_lastFallTime = time;
            m_lastFallZ = z;
        }
        void HandleFall(MovementInfo const& movementInfo);

        bool IsKnowHowFlyIn(uint32 mapid, uint32 zone) const;

        void SetClientControl(Unit* target, uint8 allowMove);

        void SetMover(Unit* target);
        void SetSeer(WorldObject *target) { m_seer = target; }
        void SetViewpoint(WorldObject *target, bool apply);
        WorldObject* GetViewpoint() const;
        void StopCastingCharm();
        void StopCastingBindSight();

        uint32 GetSaveTimer() const { return m_nextSave; }
        void   SetSaveTimer(uint32 timer) { m_nextSave = timer; }

        time_t GetLastManualSave() const { return m_lastManualSave; }
        void   SetLastManualSave(time_t timer) { m_lastManualSave = timer; }

        // Recall position
        uint32 m_recallMap;
        float  m_recallX;
        float  m_recallY;
        float  m_recallZ;
        float  m_recallO;
        void   SaveRecallPosition();

        void SetHomebind(WorldLocation const& loc, uint32 area_id);

        uint32 m_ConditionErrorMsgId;

        // Homebind coordinates
        uint32 m_homebindMapId;
        uint16 m_homebindAreaId;
        float m_homebindX;
        float m_homebindY;
        float m_homebindZ;

        // in case of guild leave (preserve guild reputation)
        uint32 m_lastGuildId;
        time_t m_guildLeaveTime;

        void SetLeaveGuildData(uint32 guildId);
        static void SetOfflineLeaveGuildData(uint64 guid, uint32 guildId);

        WorldLocation GetStartPosition() const;

        PetSlot m_currentPetSlot;
        uint32 m_petSlotUsed;
    
        void setPetSlotUsed(PetSlot slot, bool used)
        {
            if(used)
                m_petSlotUsed |=  (1 << uint32(slot));
            else
                m_petSlotUsed &= ~(1 << uint32(slot));
        }
    
        PetSlot getSlotForNewPet()
        {
            // Some changes here.
            uint32 last_known = 0;
            // Call Pet Spells.
            // 883, 83242, 83243, 83244, 83245
            //  1     2      3      4      5
            if(HasSpell(83245))
                last_known = 5;
            else if(HasSpell(83244))
                last_known = 4;
            else if(HasSpell(83243))
                last_known = 3;
            else if(HasSpell(83242))
                last_known = 2;
            else if(HasSpell(883))
                last_known = 1;

            for(uint32 i = uint32(PET_SLOT_HUNTER_FIRST); i < last_known; i++)
            {   
                if((m_petSlotUsed & (1 << i)) == 0)
                    return PetSlot(i);
            }

            // If there is no slots available, then we should point that out
            return PET_SLOT_FULL_LIST; //(PetSlot)last_known;
        }
        void SendTooManyPets(Player *pl);

        // currently visible objects at player client
        typedef std::set<uint64> ClientGUIDs;
        ClientGUIDs m_clientGUIDs;

        bool HaveAtClient(WorldObject const* u) const { return u == this || m_clientGUIDs.find(u->GetGUID()) != m_clientGUIDs.end(); }

        bool canSeeOrDetect(Unit const* u, bool detect, bool inVisibleList = false, bool is3dDistance = true) const;
        bool IsVisibleInGridForPlayer(Player const* pl) const;
        bool IsVisibleGloballyFor(Player* pl) const;

        void SendInitialVisiblePackets(Unit* target);
        void UpdateObjectVisibility(bool forced = true);
        void UpdateVisibilityForPlayer();
        void UpdateVisibilityOf(WorldObject* target);
        void UpdateTriggerVisibility();
        void HandleDelayedUpdateForPlayer(Creature * target);

        template<class T>
            void UpdateVisibilityOf(T* target, UpdateData& data, std::set<Unit*>& visibleNow);

        // Stealth detection system
        void HandleStealthedUnitsDetection();

        uint8 m_forced_speed_changes[MAX_MOVE_TYPE];

        bool HasAtLoginFlag(AtLoginFlags f) const { return m_atLoginFlags & f; }
        void SetAtLoginFlag(AtLoginFlags f) { m_atLoginFlags |= f; }
        void RemoveAtLoginFlag(AtLoginFlags f, bool in_db_also = false);

        typedef std::unordered_map<uint64, uint64> GUIDTimestampMap;
        typedef std::unordered_map<uint32, GUIDTimestampMap> SummonMap;

        SummonMap m_summonMap;

        GUIDTimestampMap* GetSummonMapFor(uint32 entry);
        void AddSummonToMap(uint32 entry, uint64 guid, uint64 timestamp);
        uint32 DespawnOldestSummon(uint32 entry);
        void DespawnAllSummonsByEntry(uint32 entry);
        void DeleteSummonFromMapByGUID(uint32 entry, uint64 guid);

        void SetCombatReadinessTimer(uint32 _time) { m_combatReadinessTimer = _time; }

        bool isUsingLfg();

        typedef std::set<uint32> DFQuestsDoneList;
        DFQuestsDoneList m_DFQuests;

        // Temporarily removed pet cache
        uint32 GetTemporaryUnsummonedPetNumber() const { return m_temporaryUnsummonedPetNumber; }
        void SetTemporaryUnsummonedPetNumber(uint32 petnumber) { m_temporaryUnsummonedPetNumber = petnumber; }
        void UnsummonPetTemporaryIfAny();
        void ResummonPetTemporaryUnSummonedIfAny();
        bool IsPetNeedBeTemporaryUnsummoned() const { return !IsInWorld() || !IsAlive() || IsMounted() /*+in flight*/; }

        void SendCinematicStart(uint32 CinematicSequenceId);
        void SendMovieStart(uint32 MovieId);
        //Worgen Transformations

        void toggleWorgenForm(bool apply = true, bool with_anim = false);
        bool IsInWorgenForm() { return HasAura(97709); };
        void SetInWorgenForm(uint32 form);
    
        /*********************************************************/
        /***                 INSTANCE SYSTEM                   ***/
        /*********************************************************/

        typedef std::unordered_map< uint32 /*mapId*/, InstancePlayerBind > BoundInstancesMap;

        void UpdateHomebindTime(uint32 time);

        uint32 m_HomebindTimer;
        bool m_InstanceValid;
        // permanent binds and solo binds by difficulty
        BoundInstancesMap m_boundInstances[MAX_DIFFICULTY];
        InstancePlayerBind* GetBoundInstance(uint32 mapid, Difficulty difficulty);
        BoundInstancesMap& GetBoundInstances(Difficulty difficulty) { return m_boundInstances[difficulty]; }
        InstanceSave * GetInstanceSave(uint32 mapid, bool raid);
        void UnbindInstance(uint32 mapid, Difficulty difficulty, bool unload = false);
        void UnbindInstance(BoundInstancesMap::iterator &itr, Difficulty difficulty, bool unload = false);
        InstancePlayerBind* BindToInstance(InstanceSave *save, bool permanent, bool load = false);
        InstancePlayerBind* BindToInstanceRaid(uint32 instanceId,uint32 mapId);
        void SendRaidInfo();
        void SendSavedInstances();
        static void ConvertInstancesToGroup(Player *player, Group *group = NULL, uint64 player_guid = 0);
        bool Satisfy(AccessRequirement const* ar, uint32 target_map, bool report = false);
        bool CheckInstanceLoginValid();

        // last used pet number (for BG's)
        uint32 GetLastPetNumber() const { return m_lastpetnumber; }
        void SetLastPetNumber(uint32 petnumber) { m_lastpetnumber = petnumber; }

        /*********************************************************/
        /***                   GROUP SYSTEM                    ***/
        /*********************************************************/

        Group * GetGroupInvite() { return m_groupInvite; }
        void SetGroupInvite(Group *group) { m_groupInvite = group; }
        Group * GetGroup() { return m_group.getTarget(); }
        const Group * GetGroup() const { return (const Group*)m_group.getTarget(); }
        GroupReference& GetGroupRef() { return m_group; }
        void SetGroup(Group *group, int8 subgroup = -1);
        uint8 GetSubGroup() const { return m_group.getSubGroup(); }
        uint32 GetGroupUpdateFlag() const { return m_groupUpdateMask; }
        void SetGroupUpdateFlag(uint32 flag) { m_groupUpdateMask |= flag; }
        const uint64& GetAuraUpdateMaskForRaid() const { return m_auraRaidUpdateMask; }
        void SetAuraUpdateMaskForRaid(uint8 slot) { m_auraRaidUpdateMask |= (uint64(1) << slot); }
        Player* GetNextRandomRaidMember(float radius);
        PartyResult CanUninviteFromGroup() const;
        uint8 GetRoles()
        {
            if (Group* grp = GetGroup())
            {
                return grp->GetRoles(GetGUID());
            }
            return 0;
        }
        void SetRoles(uint8 _roles)
        {
            if (Group* grp = GetGroup())
            {
                grp->SetRoles(GetGUID(), _roles);
            }
        }

        // Battleground/Battlefield Group System
        void SetBattlegroundOrBattlefieldRaid(Group *group, int8 subgroup = -1);
        void RemoveFromBattlegroundOrBattlefieldRaid();
        Group * GetOriginalGroup() { return m_originalGroup.getTarget(); }
        GroupReference& GetOriginalGroupRef() { return m_originalGroup; }
        uint8 GetOriginalSubGroup() const { return m_originalGroup.getSubGroup(); }
        void SetOriginalGroup(Group *group, int8 subgroup = -1);

        void SetPassOnGroupLoot(bool bPassOnGroupLoot) { m_bPassOnGroupLoot = bPassOnGroupLoot; }
        bool GetPassOnGroupLoot() const { return m_bPassOnGroupLoot; }

        MapReference &GetMapRef() { return m_mapRef; }

        // Set map to player and add reference
        void SetMap(Map * map);
        void ResetMap();

        bool isAllowedToLoot(const Creature* creature);

        DeclinedName const* GetDeclinedNames() const { return m_declinedname; }
        uint8 GetRunesState() const { return m_runes->runeState; }
        RuneType GetBaseRune(uint8 index) const { return RuneType(m_runes->runes[index].BaseRune); }
        RuneType GetCurrentRune(uint8 index) const { return RuneType(m_runes->runes[index].CurrentRune); }
        uint32 GetRuneCooldown(uint8 index) const { return m_runes->runes[index].Cooldown; }
        uint32 GetRuneBaseCooldown(uint8 index);
        bool IsBaseRuneSlotsOnCooldown(RuneType runeType) const;
        //RuneType GetLastUsedRune() { return m_runes->lastUsedRune; }
        //void SetLastUsedRune(RuneType type) { m_runes->lastUsedRune = type; }
        void SetLastUsedRune(uint8 runeIndex) { m_runes->lastUsedRunes |= (1 << runeIndex); }
        void ClearLastUsedRuneList()          { m_runes->lastUsedRunes = 0; }
        bool HasLastUsedRune(uint8 runeIndex) { return m_runes->lastUsedRunes & (1 << runeIndex); }
        void SetBaseRune(uint8 index, RuneType baseRune) { m_runes->runes[index].BaseRune = baseRune; }
        void SetCurrentRune(uint8 index, RuneType currentRune) { m_runes->runes[index].CurrentRune = currentRune; }
        void SetRuneCooldown(uint8 index, uint32 cooldown);
        void SetRuneConvertAura(uint8 index, AuraEffect const * aura) { m_runes->runes[index].ConvertAura = aura; }
        void AddRuneByAuraEffect(uint8 index, RuneType newType, AuraEffect const * aura) { SetRuneConvertAura(index, aura); ConvertRune(index, newType); }
        void RemoveRunesByAuraEffect(AuraEffect const * aura);
        void RestoreBaseRune(uint8 index);
        void ConvertRune(uint8 index, RuneType newType);
        void ResyncRunes(uint8 count);
        void AddRunePower(uint8 index);
        void InitRunes();
        void SendConvertedRunes();
        bool HasPermanentDeathRuneInSlot(uint8 index) const;

        AchievementMgr& GetAchievementMgr() { return m_achievementMgr; }
        AchievementMgr const& GetAchievementMgr() const { return m_achievementMgr; }
        void UpdateAchievementCriteria(AchievementCriteriaTypes type, uint32 miscvalue1 = 0, uint32 miscvalue2 = 0, Unit *unit = NULL, uint32 time = 0);
        void UpdateGuildAchievementCriteria(AchievementCriteriaTypes type, uint32 miscvalue1 = 0, uint32 miscvalue2 = 0, Unit *unit = NULL, uint32 time = 0);
        void CompletedAchievement(AchievementEntry const* entry, bool ignoreGMAllowAchievementConfig = false);

        bool HasTitle(uint32 bitIndex);
        bool HasTitle(CharTitlesEntry const* title) { return HasTitle(title->bit_index); }
        void SetTitle(CharTitlesEntry const* title, bool lost = false);

        //bool isActiveObject() const { return true; }
        bool CanSeeSpellClickOn(Creature const* creature) const;

        uint32 GetChampioningFaction() const { return m_ChampioningFaction; }
        void SetChampioningFaction(uint32 faction) { m_ChampioningFaction = faction; }
        Spell * m_spellModTakingSpell;

        float GetAverageItemLevel();

        void AddNonTriggeredSpellcastHistory(const SpellEntry* spell, uint32 specialValue = 0);
        // Only for use in current Update tick! Pointer can (and probably will) expire with next spellcast
        uint64 GetHistorySpell(uint8 slot);

        void SetLastDirectAttackTarget(Unit* target);
        uint64 GetLastDirectAttackTargetGUID();

        void SetSpectatorData(uint32 instanceId, uint32 joinTime)
        {
            m_spectatorInstanceId = instanceId;
            m_spectatorJoinTime = joinTime;
        }
        uint32 GetSpectatorInstanceId() const { return m_spectatorInstanceId; };
        uint32 GetSpectatorJoinTime() const { return m_spectatorJoinTime; };
        void ViolateSpectatorWaitTime();

        AntiHackServant* GetAntiHackServant()
        {
            return &m_antiHackServant;
        }

        uint32 getRaidDiffProgr(uint32 id/*mapId*/)
        {
            return RaidDiffProgress[id];
        }

        void setRaidDiffProgr(uint32 id/*mapId*/, uint32 progr)
        {
            RaidDiffProgress[id]=progr;
        }

        uint32 getRaidId(uint32 mapId/*mapId*/)
        {
            return raidId[mapId];
        }

        void setRaidId(uint32 mapId/*mapId*/, uint32 instanceId)
        {
            raidId[mapId]=instanceId;
        }

        bool showInstanceBindQuery;

        // Void Storage
        bool IsVoidStorageUnlocked() const { return HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_VOID_UNLOCKED); }
        void UnlockVoidStorage() { SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_VOID_UNLOCKED); }
        void LockVoidStorage() { RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_VOID_UNLOCKED); }
        uint8 GetNextVoidStorageFreeSlot() const;
        uint8 GetNumOfVoidStorageFreeSlots() const;
        uint8 AddVoidStorageItem(const VoidStorageItem& item);
        void AddVoidStorageItemAtSlot(uint8 slot, const VoidStorageItem& item);
        void DeleteVoidStorageItem(uint8 slot);
        bool SwapVoidStorageItem(uint8 oldSlot, uint8 newSlot);
        VoidStorageItem* GetVoidStorageItem(uint8 slot) const;
        VoidStorageItem* GetVoidStorageItem(uint64 id, uint8& slot) const;

        void SetItemUpdateQueueState(bool state) { m_itemUpdateQueueBlocked = state; };
        bool GetItemUpdateQueueState() { return m_itemUpdateQueueBlocked; };

    protected:
        uint32 m_AreaID;
        uint32 m_regenTimerCount;
        float m_powerFraction[MAX_POWERS];
        uint32 m_contestedPvPTimer;

        uint32 m_spectatorInstanceId;
        uint32 m_spectatorJoinTime;

        AntiHackServant m_antiHackServant;

        //32bits for entry, 32bits for special cases
        uint64 m_nonTriggeredSpellcastHistory[4];

        uint64 m_lastDirectAttackTarget;

        std::list<SavedCastedAura*> m_myCastedAuras;

        uint32 m_ratedBgStats[RATED_BG_STATS_MAX]; // for won and lost games, rating is stored in PlayerFields

        /*********************************************************/
        /***               BATTLEGROUND SYSTEM                 ***/
        /*********************************************************/

        /*
        this is an array of BG queues (BgTypeIDs) in which is player
        */
        struct BgBattlegroundQueueID_Rec
        {
            BattlegroundQueueTypeId bgQueueTypeId;
            bool isWargame;
            uint32 invitedToInstance;
        };

        BgBattlegroundQueueID_Rec m_bgBattlegroundQueueID[PLAYER_MAX_BATTLEGROUND_QUEUES];
        BGData                    m_bgData;

        bool m_IsBGRandomWinner;

        /*********************************************************/
        /***                    QUEST SYSTEM                   ***/
        /*********************************************************/

        //We allow only one timed quest active at the same time. Below can then be simple value instead of set.
        typedef std::set<uint32> QuestSet;
        QuestSet m_timedquests;
        QuestSet m_weeklyquests;

        uint64 m_divider;
        uint32 m_ingametime;

        /*********************************************************/
        /***                   LOAD SYSTEM                     ***/
        /*********************************************************/

        void _LoadActions(PreparedQueryResult result);
        void _LoadAuras(PreparedQueryResult result, uint32 timediff);
        void _LoadGlyphAuras();
        void _LoadBoundInstances(PreparedQueryResult result);
        void _LoadBoundInstance(uint32 mapId);
        void _LoadInventory(PreparedQueryResult result, uint32 timediff);
        void _LoadVoidStorage(PreparedQueryResult result);
        void _LoadMailInit(PreparedQueryResult resultUnread, PreparedQueryResult resultDelivery);
        void _LoadMail();
        void _LoadMailedItems(Mail *mail);
        void _LoadQuestStatus(PreparedQueryResult result);
        void _LoadDailyQuestStatus(PreparedQueryResult result);
        void _LoadWeeklyQuestStatus(PreparedQueryResult result);
        void _LoadRandomBGStatus(PreparedQueryResult result);
        void _LoadGroup(PreparedQueryResult result);
        void _LoadSkills(PreparedQueryResult result);
        void _LoadSpells(PreparedQueryResult result);
        void _LoadFriendList(PreparedQueryResult result);
        bool _LoadHomeBind(PreparedQueryResult result);
        void _LoadDeclinedNames(PreparedQueryResult result);
        void _LoadArenaTeamInfo(PreparedQueryResult result);
        void _LoadArenaStatsInfo(PreparedQueryResult result);
        void _LoadEquipmentSets(PreparedQueryResult result);
        void _LoadBGData(PreparedQueryResult result);
        void _LoadGlyphs(PreparedQueryResult result);
        void _LoadTalents(PreparedQueryResult result);
        void _LoadTalentBranchSpecs(PreparedQueryResult result);
        void _LoadCurrency(PreparedQueryResult result);
        void _LoadCurrencyWeekcap(PreparedQueryResult result);
        void _LoadCUFProfiles(PreparedQueryResult result);
        void _LoadArchaeologyData();
        void _LoadPetSlots(PreparedQueryResult result);

        /*********************************************************/
        /***                   SAVE SYSTEM                     ***/
        /*********************************************************/

        void _SaveActions(SQLTransaction& trans);
        void _SaveAuras(SQLTransaction& trans);
        void _SaveInventory(SQLTransaction& trans);
        void _SaveVoidStorage(SQLTransaction& trans);
        void _SaveMail(SQLTransaction& trans);
        void _SaveQuestStatus(SQLTransaction& trans);
        void _SaveDailyQuestStatus(SQLTransaction& trans);
        void _SaveWeeklyQuestStatus(SQLTransaction& trans);
        void _SaveSkills(SQLTransaction& trans);
        void _SaveSpells(SQLTransaction& trans);
        void _SaveEquipmentSets(SQLTransaction& trans);
        void _SaveBGData(SQLTransaction& trans);
        void _SaveGlyphs(SQLTransaction& trans);
        void _SaveTalents(SQLTransaction& trans);
        void _SaveTalentBranchSpecs(SQLTransaction& trans);
        void _SaveCurrency();
        void _SaveCurrencyWeekcap();
        void _SaveStats(SQLTransaction& trans);
        void _SaveArchaeologyData();
        void _SaveCUFProfiles(SQLTransaction& trans);

        /*********************************************************/
        /***              ENVIRONMENTAL SYSTEM                 ***/
        /*********************************************************/
        void HandleSobering();
        void SendMirrorTimer(MirrorTimerType Type, uint32 MaxValue, uint32 CurrentValue, int32 Regen);
        void StopMirrorTimer(MirrorTimerType Type);
        void HandleDrowning(uint32 time_diff);
        int32 getMaxTimer(MirrorTimerType timer);

        /*********************************************************/
        /***                  HONOR SYSTEM                     ***/
        /*********************************************************/
        time_t m_lastHonorUpdateTime;
        
        void outDebugValues() const;
        uint64 m_lootGuid;

        uint32 m_team;
        uint32 m_nextSave;
        uint32 m_lastManualSave;
        time_t m_speakTime;
        uint32 m_speakCount;
        Difficulty m_dungeonDifficulty;
        Difficulty m_raidDifficulty;
        Difficulty m_raidMapDifficulty;

        uint32 m_atLoginFlags;

        Item* m_items[PLAYER_SLOTS_COUNT];
        uint32 m_currentBuybackSlot;
        PlayerCurrenciesMap m_currencies;

        VoidStorageItem* _voidStorageItems[VOID_STORAGE_MAX_SLOT];

        std::vector<Item*> m_itemUpdateQueue;
        bool m_itemUpdateQueueBlocked;

        uint32 m_ExtraFlags;
        uint64 m_curSelection;

        uint64 m_comboTarget;
        int8 m_comboPoints;

        bool m_eclipseDriverLeft;

        uint32 m_lastBattlegroundTypeId;

        WorldSafeLocsEntry* m_graveyard;

        QuestStatusMap mQuestStatus;

        SkillStatusMap mSkillStatus;

        uint32 m_GuildIdInvited;
        uint32 m_ArenaTeamIdInvited;

        uint32 m_ConquestPointCap;

        PlayerMails m_mail;
        PlayerSpellMap m_spells;
        PlayerTalentMap *m_talents[MAX_TALENT_SPECS];
        uint32 m_lastPotionId;                              // last used health/mana potion in combat, that block next potion use

        uint8 m_activeSpec;
        uint8 m_specsCount;
        uint32 m_branchSpec[MAX_TALENT_SPECS];
        uint32 m_freeTalentPoints;

        uint32 m_emote;

        uint32 m_Glyphs[MAX_TALENT_SPECS][MAX_GLYPH_SLOT_INDEX];

        ActionButtonList m_actionButtons;

        float m_auraBaseMod[BASEMOD_END][MOD_END];
        int16 m_baseRatingValue[MAX_COMBAT_RATING];
        uint32 m_baseFeralAP;
        uint32 m_baseManaRegen;
        uint32 m_baseHealthRegen;
        uint32 m_baseSpellPower;
        int32 m_spellPenetrationItemMod;

        uint32 m_spellPowerFromIntellect;
    
        SpellModList m_spellMods[MAX_SPELLMOD];
        //uint32 m_pad;
//        Spell * m_spellModTakingSpell;  // Spell for which charges are dropped in spell::finish

        EnchantDurationList m_enchantDuration;
        ItemDurationList m_itemDuration;
        ItemDurationList m_itemSoulboundTradeable;

        void ResetTimeSync();
        void SendTimeSync();

        uint64 m_resurrectGUID;
        uint32 m_resurrectMap;
        float m_resurrectX, m_resurrectY, m_resurrectZ;
        uint32 m_resurrectHealth, m_resurrectMana;

        WorldSession *m_session;

        typedef std::list<Channel*> JoinedChannelsList;
        JoinedChannelsList m_channels;

        /*********************************************************/
        /***                  Archaeology                      ***/
        /*********************************************************/
        typedef struct
        {
            uint32 site_creature[MAX_DIGSITES];
            uint8 site_dig_count[MAX_DIGSITES];
        } ResearchSites_t;
        ResearchSites_t m_researchSites;

        typedef struct
        {
            uint32 project_id;
            uint32 completed_count;
            uint64 completed_date;
            uint8 active;
        } ResearchProjectsElem;
        std::list<ResearchProjectsElem> m_researchProjects;

        uint8 m_cinematic;

        TradeData* m_trade;

        bool   m_DailyQuestChanged;
        bool   m_WeeklyQuestChanged;
        time_t m_lastDailyQuestTime;

        uint32 m_pvpOnTimer;

        uint32 m_drunkTimer;
        uint16 m_drunk;
        uint32 m_weaponChangeTimer;

        uint32 m_combatReadinessTimer;

        uint32 m_zoneUpdateId;
        uint32 m_zoneUpdateTimer;
        uint32 m_areaUpdateId;

        uint32 m_deathTimer;
        time_t m_deathExpireTime;

        uint32 m_restTime;

        uint32 m_WeaponProficiency;
        uint32 m_ArmorProficiency;
        bool m_canParry;
        bool m_canBlock;
        bool m_canTitanGrip;
        uint8 m_swingErrorMsg;
        float m_ammoDPS;

        ////////////////////Rest System/////////////////////
        time_t time_inn_enter;
        uint32 inn_pos_mapid;
        float  inn_pos_x;
        float  inn_pos_y;
        float  inn_pos_z;
        float m_rest_bonus;
        RestType rest_type;
        ////////////////////Rest System/////////////////////
        uint64 m_resetTalentsCost;
        time_t m_resetTalentsTime;
        uint32 m_usedTalentCount;
        uint32 m_questRewardTalentCount;

        // Social
        PlayerSocial *m_social;
        uint32 m_guildId;

        // Groups
        GroupReference m_group;
        GroupReference m_originalGroup;
        Group *m_groupInvite;
        uint32 m_groupUpdateMask;
        uint64 m_auraRaidUpdateMask;
        bool m_bPassOnGroupLoot;

        // last used pet number (for BG's)
        uint32 m_lastpetnumber;

        // Player summoning
        time_t m_summon_expire;
        uint32 m_summon_mapid;
        float  m_summon_x;
        float  m_summon_y;
        float  m_summon_z;

        DeclinedName *m_declinedname;
        Runes *m_runes;
        EquipmentSets m_EquipmentSets;

        CUFProfile* _CUFProfiles[MAX_CUF_PROFILES];
    private:
        // internal common parts for CanStore/StoreItem functions
        uint8 _CanStoreItem_InSpecificSlot(uint8 bag, uint8 slot, ItemPosCountVec& dest, ItemPrototype const *pProto, uint32& count, bool swap, Item *pSrcItem) const;
        uint8 _CanStoreItem_InBag(uint8 bag, ItemPosCountVec& dest, ItemPrototype const *pProto, uint32& count, bool merge, bool non_specialized, Item *pSrcItem, uint8 skip_bag, uint8 skip_slot) const;
        uint8 _CanStoreItem_InInventorySlots(uint8 slot_begin, uint8 slot_end, ItemPosCountVec& dest, ItemPrototype const *pProto, uint32& count, bool merge, Item *pSrcItem, uint8 skip_bag, uint8 skip_slot) const;
        Item* _StoreItem(uint16 pos, Item *pItem, uint32 count, bool clone, bool update);

        std::set<uint32> m_refundableItems;
        void SendRefundInfo(Item* item);
        void SendItemRefundResult(Item* item, ItemExtendedCostEntry const* iece, uint8 error);
        void RefundItem(Item* item);

        int32 CalculateReputationGain(uint32 creatureOrQuestLevel, int32 rep, int32 faction, bool for_quest, bool noQuestBonus = false);
        void AdjustQuestReqItemCount(Quest const* pQuest, QuestStatusData& questStatusData);

        bool IsCanDelayTeleport() const { return m_bCanDelayTeleport; }
        void SetCanDelayTeleport(bool setting) { m_bCanDelayTeleport = setting; }
        bool IsHasDelayedTeleport() const { return m_bHasDelayedTeleport; }
        void SetDelayedTeleportFlag(bool setting) { m_bHasDelayedTeleport = setting; }

        void ScheduleDelayedOperation(uint32 operation)
        {
            if (operation < DELAYED_END)
                m_DelayedOperations |= operation;
        }

        MapReference m_mapRef;
        uint8 m_class;

        void UpdateCharmedAI();

        uint32 m_lastFallTime;
        float  m_lastFallZ;

        LiquidTypeEntry const* m_lastLiquid;

        int32 m_MirrorTimer[MAX_TIMERS];
        uint8 m_MirrorTimerFlags;
        uint8 m_MirrorTimerFlagsLast;
        bool m_isInWater;

        bool disabledPVPAnnounce;

        // Current teleport data
        WorldLocation m_teleport_dest;
        uint32 m_teleport_options;
        bool mSemaphoreTeleport_Near;
        bool mSemaphoreTeleport_Far;

        uint32 m_DelayedOperations;
        bool m_bCanDelayTeleport;
        bool m_bHasDelayedTeleport;

        int m_DetectInvTimer;

        // Temporary removed pet cache
        uint32 m_temporaryUnsummonedPetNumber;
        uint32 m_oldpetspell;

        AchievementMgr m_achievementMgr;
        ReputationMgr  m_reputationMgr;

        SpellCooldowns m_spellCooldowns;
        SpellCooldowns m_petSpellCooldowns;
        std::map<uint32, uint32> m_globalCooldowns; // whole start recovery category stored in one

        uint32 m_ChampioningFaction;

        uint32 m_timeSyncCounter;
        uint32 m_timeSyncTimer;
        uint32 m_timeSyncClient;
        uint32 m_timeSyncServer;

        std::unordered_map<uint32 /*mapId*/, uint32 /*progress*/> RaidDiffProgress;//players difficulty progress in raid
        std::unordered_map<uint32 /*mapId*/, uint32 /*oldId*/> raidId;//players deafult IDs for his raids
};

void AddItemsSetItem(Player*player,Item *item);
void RemoveItemsSetItem(Player*player,ItemPrototype const *proto);

// "the bodies of template functions must be made available in a header file"
template <class T> T Player::ApplySpellMod(uint32 spellId, SpellModOp op, T &basevalue, Spell * spell)
{
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo) return 0;
    float totalmul = 1.0f;
    int32 totalflat = 0;

    // Drop charges for triggering spells instead of triggered ones
    if (m_spellModTakingSpell)
        spell = m_spellModTakingSpell;

    for (SpellModList::iterator itr = m_spellMods[op].begin(); itr != m_spellMods[op].end(); ++itr)
    {
        SpellModifier *mod = *itr;

        // Charges can be set only for mods with auras
        if (!mod->ownerAura)
            ASSERT(mod->charges == 0);

        if (!IsAffectedBySpellmod(spellInfo,mod,spell))
            continue;

        if (mod->type == SPELLMOD_FLAT)
            totalflat += mod->value;
        else if (mod->type == SPELLMOD_PCT)
        {
            // skip percent mods for null basevalue (most important for spell mods with charges)
            if (basevalue == T(0))
                continue;

            // special case (skip >10sec spell casts for instant cast setting)
            if (mod->op == SPELLMOD_CASTING_TIME  && basevalue >= T(10000) && mod->value <= -100)
                continue;

            // total multiplier bonuses are additive
            totalmul += (float)mod->value / 100.0f;
        }

        DropModCharge(mod, spell);
    }
    float diff = (float)basevalue * (totalmul - 1.0f) + (float)totalflat;
    basevalue = T((float)basevalue + diff);
    return T(diff);
}
#endif
