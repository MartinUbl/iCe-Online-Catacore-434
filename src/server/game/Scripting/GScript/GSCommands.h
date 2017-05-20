#ifndef GSCR_COMMANDS_H
#define GSCR_COMMANDS_H

// parsed lexeme structure
struct gs_command_proto
{
    std::string instruction;
    std::vector<std::string> parameters;
    int lineNum;

    void addParameter(std::string par);
};

// vector of command prototypes
typedef std::vector<gs_command_proto*> CommandProtoVector;

struct gs_command;
struct gs_commands_container;

// wrapper struct for command vector and event command vector
typedef gs_commands_container CommandContainer;

// parse input lines into command prototype vector
CommandProtoVector* gscr_parseInput(std::vector<std::string> &lines);
// parse command prototypes into command vector to be executed
CommandContainer* gscr_analyseSequence(CommandProtoVector* input, int scriptId);

// types of available commands
enum gs_command_type
{
    GSCR_NONE = 0,
    GSCR_LABEL,
    GSCR_GOTO,
    GSCR_WAIT,
    GSCR_CAST,
    GSCR_SAY,
    GSCR_YELL,
    GSCR_IF,
    GSCR_ENDIF,
    GSCR_COMBATSTOP,
    GSCR_FACTION,
    GSCR_REACT,
    GSCR_KILL,
    GSCR_COMBATSTART,
    GSCR_TIMER,
    GSCR_MORPH,
    GSCR_SUMMON,
    GSCR_SUMMONGO,
    GSCR_WALK,
    GSCR_RUN,
    GSCR_TELEPORT,
    GSCR_WAITFOR,
    GSCR_LOCK,
    GSCR_UNLOCK,
    GSCR_SCALE,
    GSCR_FLAGS,
    GSCR_IMMUNITY,
    GSCR_EMOTE,
    GSCR_MOVIE,
    GSCR_AURA,
    GSCR_SPEED,
    GSCR_MOVE,
    GSCR_MOUNT,
    GSCR_UNMOUNT,
    GSCR_QUEST,
    GSCR_DESPAWN,
    GSCR_DESPAWNGO,
    GSCR_REPEAT,
    GSCR_UNTIL,
    GSCR_WHILE,
    GSCR_ENDWHILE,
    GSCR_VAR,
    GSCR_SOUND,
    GSCR_WHEN,
    GSCR_ENDWHEN,
    GSCR_TALK,
    GSCR_TURN,
    GSCR_FOLLOW,
    GSCR_TEXTEMOTE,
    GSCR_BOSSEMOTE,
    GSCR_UNVEHICLE,
    GSCR_WHISPER,
    GSCR_VISIBLE,
    GSCR_INVISIBLE,
    GSCR_RESET,
    GSCR_RESOLVE,
    GSCR_PHASE,
    GSCR_EVENT,
    GSCR_END_EVENT,
    GSCR_GO,
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
    "summongo",
    "walk",
    "run",
    "teleport",
    "waitfor",
    "lock",
    "unlock",
    "scale",
    "flags",
    "immunity",
    "emote",
    "movie",
    "aura",
    "speed",
    "move",
    "mount",
    "unmount",
    "quest",
    "despawn",
    "despawngo",
    "repeat",
    "until",
    "while",
    "endwhile",
    "var",
    "sound",
    "when",
    "endwhen",
    "talk",
    "turn",
    "follow",
    "textemote",
    "bossemote",
    "unvehicle",
    "whisper",
    "visible",
    "invisible",
    "reset",
    "resolve",
    "phase",
    "event",
    "endevent",
    "go"
};

enum EventHookType : uint8
{
    EVENT_HOOK_COMBAT,
    EVENT_HOOK_EVADE,
    EVENT_HOOK_DIED,
    EVENT_HOOK_KILLED,
    EVENT_HOOK_JUST_SUMMONED,
    EVENT_HOOK_SPELL_HIT,
    EVENT_HOOK_DAMAGE_DEALT
};

static const std::map<std::string, EventHookType> gs_event_hook_names_map =
{
    { "combat_start",           EVENT_HOOK_COMBAT },            // Unit* ev_enemy
    { "enter_evade_mode",       EVENT_HOOK_EVADE },             // none
    { "just_died",              EVENT_HOOK_DIED },              // Unit* ev_killer
    { "killed_unit",            EVENT_HOOK_KILLED },            // Unit* ev_victim
    { "just_summoned",          EVENT_HOOK_JUST_SUMMONED },     // Unit* ev_summon
    { "spell_hit",              EVENT_HOOK_SPELL_HIT },         // Unit* ev_caster
    { "damage_dealt",           EVENT_HOOK_DAMAGE_DEALT },      // Unit* ev_target
};

#define GS_VARIABLE_MAX_LIMIT 10000

enum gs_event_variable_id : int32
{
    EVENT_VARIABLE_ENEMY_ID = GS_VARIABLE_MAX_LIMIT + 1,
    EVENT_VARIABLE_KILLER_ID,
    EVENT_VARIABLE_VICTIM_ID,
    EVENT_VARIABLE_SUMMON_ID,
    EVENT_VARIABLE_SUMMON_ENTRY_ID,
    EVENT_VARIABLE_CASTER_ID,
    EVENT_VARIABLE_SPELL_ID,
    EVENT_VARIABLE_TARGET_ID,
    EVENT_VARIABLE_DAMAGE_ID,
    EVENT_VARIABLE_DAMAGE_TYPE_ID,
};

static const std::map<std::string, gs_event_variable_id> gs_event_variable_id_mapping =
{
    { "ev_enemy",           EVENT_VARIABLE_ENEMY_ID },
    { "ev_killer",          EVENT_VARIABLE_KILLER_ID },
    { "ev_victim",          EVENT_VARIABLE_VICTIM_ID },
    { "ev_summon",          EVENT_VARIABLE_SUMMON_ID },
    { "ev_summon_entry",    EVENT_VARIABLE_SUMMON_ENTRY_ID },
    { "ev_caster",          EVENT_VARIABLE_CASTER_ID },
    { "ev_spell_id",        EVENT_VARIABLE_SPELL_ID },
    { "ev_target",          EVENT_VARIABLE_TARGET_ID },
    { "ev_damage",          EVENT_VARIABLE_DAMAGE_ID },
    { "ev_damage_type",     EVENT_VARIABLE_DAMAGE_TYPE_ID },
};

enum gs_quest_operation
{
    GSQO_NONE = 0,              // no operation
    GSQO_COMPLETE = 1,          // complete whole quest
    GSQO_FAIL = 2,              // fail quest (i.e. escort quest, where someone dies, etc.)
    GSQO_PROGRESS = 3           // move progress of quest objective
};

enum gs_flag_operation
{
    GSFO_NONE = 0,
    GSFO_ADD = 1,               // add flag to current using OR instruction
    GSFO_REMOVE = 2,            // remove flag from current using AND on negated current and flag
    GSFO_SET = 3                // sets new value regardless of what was there previously
};

enum gs_go_operation
{
    GSGO_NONE = 0,
    GSGO_USE = 1,               // use gameobject
    GSGO_TOGGLE = 2,            // switch go state
    GSGO_RESET = 3,             // reset go state
    GSGO_SET_STATE = 4          // manually set go state
};

// structure for recognized values
struct gs_recognized_string
{
    const char* str;
    uint32 value;
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
    GSST_NONE = 0,                  // none specifier
    GSST_ME = 1,                    // the owner of script (NPC itself)
    GSST_TARGET = 2,                // current target
    GSST_RANDOM = 3,                // random target from threat list
    GSST_IMPLICIT = 4,              // implicit targetting (i.e. implicit spell target, etc.)
    GSST_CHANCE = 5,                // "chance" parameter i.e. for IF instruction
    GSST_TIMER = 6,                 // timer stored, i.e. for IF instruction
    GSST_STATE = 7,                 // constant state (i.e. for timer - ready)
    GSST_RANDOM_NOTANK = 8,         // random target without current target
    GSST_INVOKER = 9,               // script invoker (gossip script, quest accept, etc.)
    GSST_PARENT = 10,               // parent unit (of summoned creature)
    GSST_VARIABLE_VALUE = 11,       // value of variable declared
    GSST_CLOSEST_CREATURE = 12,     // closest creature - expects parameter in round parenthesis
    GSST_CLOSEST_PLAYER = 13,       // closest player
    GSST_LAST_SUMMON = 14,          // last summoned unit
    GSST_CLOSEST_CREATURE_GUID = 15,// closest creature by GUID
    GSST_CLOSEST_DEAD_CREATURE = 16,// closest dead creature by entry
    GSST_CLOSEST_GAMEOBJECT = 17,   // closest gameoject - expects parameter in round parenthesis
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
    GSSP_COMBAT = 7,            // subject combat state (0 = ooc, 1 = combat)
    GSSP_MANA = 8,              // subject mana value
    GSSP_MANA_PCT = 9,          // subject mana percentage
    GSSP_DISTANCE = 10,         // subject distance from "me"
    GSSP_POS_X = 11,            // subject position X
    GSSP_POS_Y = 12,            // subject position Y
    GSSP_POS_Z = 13,            // subject position Z
    GSSP_ORIENTATION = 14,      // subject orientation
    GSSP_ALIVE = 15,            // subject alive state
    GSSP_GUIDLOW = 16,          // subject low GUID (mostly DB table GUID)
    GSSP_ENTRY = 17,            // subject entry (entry, ID from DB table)
    GSSP_AURAS = 18,            // subject aura set
};

enum gs_numeric_operatror
{
    GSNOP_NONE = 0,
    GSNOP_ASSIGN = 1,           // assign value to variable
    GSNOP_ADD = 2,              // add value to variable
    GSNOP_SUBTRACT = 3,         // subtract from variable
    GSNOP_MULTIPLY = 4,         // multiply variable
    GSNOP_DIVIDE = 5,           // divide variable value (float)
    GSNOP_DIVIDE_INT = 6,       // divide variable value (integer)
    GSNOP_MODULO = 7,           // assigns modulo of specified value
    GSNOP_INCREMENT = 8,        // increments value (integer, float)
    GSNOP_DECREMENT = 9,        // decrements value (integer, float)
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
    GSOP_HAS = 9,               // has  (is element contained in set?)
    GSOP_ISNT = 10,             // isnt (comparing states, not values) (negation of GSOP_IS)
    GSOP_HASNT = 11,            // hasnt (is element NOT contained in set?) (negation of GSOP_HAS)
};

enum gs_resolver_type
{
    GSRT_NONE = 0,              // no resolver type (or unresolved)
    GSRT_VECTOR = 1,            // vector resolver (using angle and distance, or relative position)
};

enum gs_vector_resolve_type
{
    GSVRT_ANGLE_DISTANCE = 1,   // resolve using angle and distance
    GSVRT_RELATIVE_POSITION = 2,// resolve using relative position
};

enum gs_position_type_specifier
{
    GSPTS_ABSOLUTE = 0,         // position is absolute, no need to add/subtract/multiply
    GSPTS_RELATIVE_CURRENT = 1, // position is relative to current position
    GSPTS_RELATIVE_SPAWN = 2,   // position is relative to spawn (summon) position
};

// specifier structure - specifies target, target + target parameter or one-value
struct gs_specifier
{
    // type of subject
    gs_subject_type subject_type;
    // type of subject parameter
    gs_subject_parameter subject_parameter;
    // one-value stored
    int32 value;
    // floating point representation of value
    float flValue;

    bool isFloat;

    // parses input string into specifier structure
    static gs_specifier parse(const char* str);
    // creates empty specifier object prefilled with specified values
    static gs_specifier make_default_subject(gs_subject_type stype, gs_subject_parameter sparam = GSSP_NONE) { return{ stype, sparam, 0, 0.0f, false }; };
    // creates empty specifier object prefilled with specified value
    static gs_specifier make_default_value(int32 value) { return{ GSST_NONE, GSSP_NONE, value, 0.0f, false }; };
    static gs_specifier make_default_float_value(float value) { return{ GSST_NONE, GSSP_NONE, 0, value, true }; };
};

// condition structure - consists of two subjects and one operator
struct gs_condition
{
    gs_specifier source;
    gs_specifier_operator op;
    gs_specifier dest;
};

struct gs_event_offsets
{
    uint32 start_offset;
    uint32 end_offset;
};

struct gs_commands_container
{
    std::vector<gs_command*> command_vector;
    std::map<EventHookType, gs_event_offsets> event_offset_map;
};

// command structure
struct gs_command
{
    // type of command
    gs_command_type type;
    // entity, which will play this command
    gs_specifier command_delegate;

    union
    {
        struct
        {
            int instruction_offset;
        } c_goto_label;

        struct
        {
            gs_specifier delay;
            unsigned int flags;
        } c_wait;

        struct
        {
            gs_specifier spell;
            gs_specifier target_type;
            bool triggered;
        } c_cast;

        struct
        {
            const char* tosay;
            gs_specifier sound_id;
            gs_specifier emote_id;
        } c_say_yell;

        struct
        {
            gs_condition condition;
            int endif_offset;
        } c_if;

        struct
        {
            gs_specifier faction;
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
            gs_specifier value;
        } c_timer;

        struct
        {
            gs_specifier morph_id;
        } c_morph;

        struct
        {
            gs_specifier creature_entry;
            gs_specifier x, y, z;
            bool nodespawn;
        } c_summon;

        struct
        {
            gs_specifier go_entry;
            gs_specifier x, y, z;
            gs_specifier respawn_timer;
        } c_summon_go;

        struct
        {
            gs_specifier x, y, z;
        } c_walk_run_teleport;

        struct
        {
            gs_event_type eventtype;
        } c_waitfor;

        struct
        {
            bool restore;
            gs_specifier scale;
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

        struct
        {
            gs_specifier emote_id;
        } c_emote;

        struct
        {
            gs_specifier movie_id;
        } c_movie;

        struct
        {
            gs_flag_operation op;
            gs_specifier aura_id;
            gs_specifier subject;
        } c_aura;

        struct
        {
            int movetype;
            gs_specifier speed;
        } c_speed;

        struct
        {
            int movetype; // -1 for idle
        } c_move;

        struct
        {
            gs_specifier mount_model_id;
        } c_mount;

        struct
        {
            gs_quest_operation op;
            gs_specifier quest_id;
            gs_specifier objective_index;
            gs_specifier value;
        } c_quest;

        struct
        {
            gs_specifier subject;
        } c_despawn;

        struct
        {
            gs_specifier subject;
        } c_despawn_go;

        struct
        {
            gs_go_operation op;
            gs_specifier subject;
            int value;
        } c_go;

        struct
        {
            gs_condition condition;
            int repeat_offset;
        } c_until;

        struct
        {
            int offset;
        } c_repeat;

        struct
        {
            gs_condition condition;
            int endwhile_offset;
            int my_offset;
        } c_while;

        struct
        {
            int while_offset;
        } c_endwhile;

        struct
        {
            gs_numeric_operatror op;
            int variable;

            gs_specifier spec;
        } c_var;

        struct
        {
            gs_specifier sound_id;
            gs_specifier target;
        } c_sound;

        struct
        {
            gs_condition condition;
            int endwhen_offset;
        } c_when;

        struct
        {
            gs_specifier talk_group_id;
            gs_specifier talk_target;
        } c_talk;

        struct
        {
            gs_specifier amount;
            bool relative;
        } c_turn;

        struct
        {
            gs_specifier subject;
            gs_specifier distance;
            gs_specifier angle;
        } c_follow;

        struct
        {
            gs_specifier entry;
        } c_unvehicle;

        struct
        {
            const char* tosay;
            gs_specifier target;
        } c_whisper;

        struct
        {
            gs_resolver_type resolver_type;
            int variable[3]; // for now, 3 should be sufficient
            gs_position_type_specifier position_type;
            gs_vector_resolve_type vector_resolve_type;
            gs_specifier angle;
            gs_specifier distance;
            gs_specifier rel_x, rel_y, rel_z;
        } c_resolve;

        struct
        {
            gs_specifier phase_mask;
        } c_phase;

        struct
        {
            int end_offset;
        } c_event;

    } params;

    // parses input command prototype into regular command
    static gs_command* parse(gs_command_proto* src, int offset);
};

#endif
