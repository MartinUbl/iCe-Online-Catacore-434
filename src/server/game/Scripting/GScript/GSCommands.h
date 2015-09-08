#ifndef GSCR_COMMANDS_H
#define GSCR_COMMANDS_H

// parsed lexeme structure
struct gs_command_proto
{
    std::string instruction;
    std::vector<std::string> parameters;

    void addParameter(std::string par);
};

// vector of command prototypes
typedef std::vector<gs_command_proto*> CommandProtoVector;

struct gs_command;

// vector of commands
typedef std::vector<gs_command*> CommandVector;

// parse input lines into command prototype vector
CommandProtoVector* gscr_parseInput(std::vector<std::string> &lines);
// parse command prototypes into command vector to be executed
CommandVector* gscr_analyseSequence(CommandProtoVector* input, int scriptId);

// types of available commands
enum gs_command_type
{
    GSCR_NONE = 0,
    GSCR_LABEL = 1,
    GSCR_GOTO = 2,
    GSCR_WAIT = 3,
    GSCR_CAST = 4,
    GSCR_SAY = 5,
    GSCR_YELL = 6,
    GSCR_IF = 7,
    GSCR_ENDIF = 8,
    GSCR_COMBATSTOP = 9,
    GSCR_FACTION = 10,
    GSCR_REACT = 11,
    GSCR_KILL = 12,
    GSCR_COMBATSTART = 13,
    GSCR_TIMER = 14,
    GSCR_MORPH = 15,
    GSCR_SUMMON = 16,
    GSCR_WALK = 17,
    GSCR_RUN = 18,
    GSCR_TELEPORT = 19,
    GSCR_WAITFOR = 20,
    GSCR_LOCK = 21,
    GSCR_UNLOCK = 22,
    GSCR_SCALE = 23,
    GSCR_FLAGS = 24,
    GSCR_IMMUNITY = 25,
};

// string identifiers - index is matching the value of enum above
static std::string gscr_identifiers[] = {
    "nop",
    "label",
    "goto",
    "wait",
    "cast",
    "say",
    "yell",
    "if",
    "endif",
    "combatstop",
    "faction",
    "react",
    "kill",
    "combatstart",
    "timer",
    "morph",
    "summon",
    "walk",
    "run",
    "teleport",
    "waitfor",
    "lock",
    "unlock",
    "scale",
    "flags",
    "immunity",
};

enum gs_flag_operation
{
    GSFO_NONE = 0,
    GSFO_ADD = 1,               // add flag to current using OR instruction
    GSFO_REMOVE = 2,            // remove flag from current using AND on negated current and flag
    GSFO_SET = 3                // sets new value regardless of what was there previously
};

// structure for recognized values
struct gs_recognized_string
{
    const char* str;
    uint32 value;
};

static gs_recognized_string gs_recognized_npc_flags[] = {
    { "gossip", UNIT_NPC_FLAG_GOSSIP },
    { "questgiver", UNIT_NPC_FLAG_QUESTGIVER },
    { "trainer", UNIT_NPC_FLAG_TRAINER },
    { "trainer_class", UNIT_NPC_FLAG_TRAINER_CLASS },
    { "trainer_profession", UNIT_NPC_FLAG_TRAINER_PROFESSION },
    { "vendor", UNIT_NPC_FLAG_VENDOR },
    { "repair", UNIT_NPC_FLAG_REPAIR },
    { "flightmaster", UNIT_NPC_FLAG_FLIGHTMASTER },
    { "innkeeper", UNIT_NPC_FLAG_INNKEEPER },
    { "banker", UNIT_NPC_FLAG_BANKER },
    { "auctioneer", UNIT_NPC_FLAG_AUCTIONEER },
    { "spellclick", UNIT_NPC_FLAG_SPELLCLICK }
};

static gs_recognized_string gs_recognized_unit_flags[] = {
    { "not_attackable", UNIT_FLAG_NON_ATTACKABLE },
    { "disable_move", UNIT_FLAG_DISABLE_MOVE },
    { "pvp_attackable", UNIT_FLAG_PVP_ATTACKABLE },
    { "ooc_not_attackable", UNIT_FLAG_OOC_NOT_ATTACKABLE },
    { "pvp", UNIT_FLAG_PVP },
    { "silenced", UNIT_FLAG_SILENCED },
    { "pacified", UNIT_FLAG_PACIFIED },
    { "stunned", UNIT_FLAG_STUNNED },
    { "not_selectable", UNIT_FLAG_NOT_SELECTABLE },
    { "skinnable", UNIT_FLAG_SKINNABLE }
};

static gs_recognized_string gs_recognized_unit_flags_2[] = {
    { "feign_death", UNIT_FLAG2_FEIGN_DEATH }
};

static gs_recognized_string gs_recognized_unit_dynamic_flags[] = {
    { "lootable", UNIT_DYNFLAG_LOOTABLE },
    { "track_unit", UNIT_DYNFLAG_TRACK_UNIT },
    { "tapped", UNIT_DYNFLAG_TAPPED },
    { "tapped_by_player", UNIT_DYNFLAG_TAPPED_BY_PLAYER },
    { "specialinfo", UNIT_DYNFLAG_SPECIALINFO },
    { "dead", UNIT_DYNFLAG_DEAD },
    { "refer_a_friend", UNIT_DYNFLAG_REFER_A_FRIEND },
    { "tapped_by_all", UNIT_DYNFLAG_TAPPED_BY_ALL_THREAT_LIST }
};

static gs_recognized_string gs_recognized_mechanic_immunity[] = {
    { "charm", MECHANIC_CHARM },
    { "disorient", MECHANIC_DISORIENTED },
    { "disarm", MECHANIC_DISARM },
    { "distract", MECHANIC_DISTRACT },
    { "fear", MECHANIC_FEAR },
    { "grip", MECHANIC_GRIP },
    { "root", MECHANIC_ROOT },
    { "pacify", MECHANIC_PACIFY },
    { "silence", MECHANIC_SILENCE },
    { "sleep", MECHANIC_SLEEP },
    { "snare", MECHANIC_SNARE },
    { "stun", MECHANIC_STUN },
    { "freeze", MECHANIC_FREEZE },
    { "knockout", MECHANIC_KNOCKOUT },
    { "bleed", MECHANIC_BLEED },
    { "bandage", MECHANIC_BANDAGE },
    { "polymorph", MECHANIC_POLYMORPH },
    { "banish", MECHANIC_BANISH },
    { "shield", MECHANIC_SHIELD },
    { "shackle", MECHANIC_SHACKLE },
    { "mount", MECHANIC_MOUNT },
    { "infected", MECHANIC_INFECTED },
    { "turn", MECHANIC_TURN },
    { "horror", MECHANIC_HORROR },
    { "invulnerability", MECHANIC_INVULNERABILITY },
    { "interrupt", MECHANIC_INTERRUPT },
    { "daze", MECHANIC_DAZE },
    { "discovery", MECHANIC_DISCOVERY },
    { "immuneshield", MECHANIC_IMMUNE_SHIELD },
    { "sap", MECHANIC_SAPPED },
    { "enrage", MECHANIC_ENRAGED },
};

// flag for wait instruction
enum gs_wait_flag
{
    GSWF_NONE               = 0x00,     // none flag
    GSWF_MELEE_ATTACK       = 0x01,     // NPC can perform melee attacks during wait period
};

enum gs_event_type
{
    GSET_NONE = 0,              // no event
    GSET_MOVEMENT = 1,          // when movement is finished
    GSET_CAST = 2,              // when spell cast is finished
};

enum gs_state_value
{
    GSSV_NONE = 0,              // none specifier
    GSSV_READY = 1,             // is ready (i.e. timer passed, ..)
    GSSV_IN_PROGRESS = 2,       // is not ready, and is in progress
    GSSV_SUSPENDED = 3,         // is not ready and is suspended
};

// type of subject in specifier
enum gs_subject_type
{
    GSST_NONE = 0,              // none specifier
    GSST_ME = 1,                // the owner of script (NPC itself)
    GSST_TARGET = 2,            // current target
    GSST_RANDOM = 3,            // random target from threat list
    GSST_IMPLICIT = 4,          // implicit targetting (i.e. implicit spell target, etc.)
    GSST_CHANCE = 5,            // "chance" parameter i.e. for IF instruction
    GSST_TIMER = 6,             // timer stored, i.e. for IF instruction
    GSST_STATE = 7,             // constant state (i.e. for timer - ready)
};

// type of subject parameter in specifier
enum gs_subject_parameter
{
    GSSP_NONE = 0,              // none specifier
    GSSP_HEALTH = 1,            // subject health
    GSSP_HEALTH_PCT = 2,        // subject health percentage
    GSSP_FACTION = 3,           // subject faction (alliance / horde)
    GSSP_LEVEL = 4,             // subject level
    GSSP_IDENTIFIER = 5,        // subject identifier, i.e. timer.name_of_timer
    GSSP_STATE_VALUE = 6,       // subject state value, i.e. ready (for timer), etc.
};

// type of operator used in expression
enum gs_specifier_operator
{
    GSOP_NONE = 0,              // no operator
    GSOP_EQUALS = 1,            // ==
    GSOP_NOT_EQUAL = 2,         // !=
    GSOP_LESS_THAN = 3,         // <
    GSOP_LESS_OR_EQUAL = 4,     // <=
    GSOP_GREATER_THAN = 5,      // >
    GSOP_GREATER_OR_EQUAL = 6,  // >=
    GSOP_OF = 7,                // of   i.e. "if chance of 50"
    GSOP_IS = 8,                // is   (comparing states, not values)
};

// specifier structure - specifies target, target + target parameter or one-value
struct gs_specifier
{
    // type of subject
    gs_subject_type subject_type;
    // type of subject parameter
    gs_subject_parameter subject_parameter;
    // one-value stored
    int value;

    // parses input string into specifier structure
    static gs_specifier parse(const char* str);
    // creates empty specifier object prefilled with specified values
    static gs_specifier make_default_subject(gs_subject_type stype, gs_subject_parameter sparam = GSSP_NONE) { return{ stype, sparam, 0 }; };
    // creates empty specifier object prefilled with specified value
    static gs_specifier make_default_value(int value) { return{ GSST_NONE, GSSP_NONE, value }; };
};

// command structure
struct gs_command
{
    // type of command
    gs_command_type type;

    union
    {
        struct
        {
            int instruction_offset;
        } c_goto_label;

        struct
        {
            int delay;
            unsigned int flags;
        } c_wait;

        struct
        {
            int spell;
            gs_specifier target_type;
            bool triggered;
        } c_cast;

        struct
        {
            const char* tosay;
        } c_say_yell;

        struct
        {
            gs_specifier source;
            gs_specifier_operator op;
            gs_specifier dest;

            int endif_offset;
        } c_if;

        struct
        {
            int faction;
        } c_faction;

        struct
        {
            ReactStates reactstate;
        } c_react;

        struct
        {
            gs_specifier target;
        } c_kill;

        struct
        {
            int timer_id;
            int value;
        } c_timer;

        struct
        {
            int morph_id;
        } c_morph;

        struct
        {
            int creature_entry;
            float x, y, z;
        } c_summon;

        struct
        {
            float x, y, z;
        } c_walk_run_teleport;

        struct
        {
            gs_event_type eventtype;
        } c_waitfor;

        struct
        {
            bool restore;
            float scale;
        } c_scale;

        struct
        {
            uint32 field;
            gs_flag_operation op;
            int value;
        } c_flags;

        struct
        {
            gs_flag_operation op;
            int mask;
        } c_immunity;

    } params;

    // parses input command prototype into regular command
    static gs_command* parse(gs_command_proto* src, int offset);
};

#endif
