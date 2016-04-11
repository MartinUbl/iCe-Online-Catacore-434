#include "gamePCH.h"
#include "GSCommands.h"
#include <exception>
#include <stack>
#include <map>

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

static gs_recognized_string gs_recognized_movetype[] = {
    { "walk", MOVE_WALK },
    { "run", MOVE_RUN },
    { "swim", MOVE_SWIM },
    { "flight", MOVE_FLIGHT }
};

static gs_recognized_string gs_recognized_emote[] = {
    { "wave", EMOTE_ONESHOT_WAVE },
    { "dance", EMOTE_STATE_DANCE },
    { "point", EMOTE_ONESHOT_POINT },
    { "cheer", EMOTE_ONESHOT_CHEER },
    { "bow", EMOTE_ONESHOT_BOW },
    { "question", EMOTE_ONESHOT_QUESTION },
    { "exclamation", EMOTE_ONESHOT_EXCLAMATION },
    { "laugh", EMOTE_ONESHOT_LAUGH },
    { "sleep", EMOTE_STATE_SLEEP },
    { "sit", EMOTE_STATE_SIT },
    { "rude", EMOTE_ONESHOT_RUDE },
    { "roar", EMOTE_ONESHOT_ROAR },
    { "kneel", EMOTE_ONESHOT_KNEEL },
    { "kiss", EMOTE_ONESHOT_KISS },
    { "cry", EMOTE_ONESHOT_CRY },
    { "chicken", EMOTE_ONESHOT_CHICKEN },
    { "beg", EMOTE_ONESHOT_BEG },
    { "applaud", EMOTE_ONESHOT_APPLAUD },
    { "shout", EMOTE_ONESHOT_SHOUT },
    { "shy", EMOTE_ONESHOT_SHY },
};

// string comparator for map
struct StrCompare : public std::binary_function<const char*, const char*, bool>
{
public:
    bool operator() (const char* str1, const char* str2) const
    {
        return std::strcmp(str1, str2) < 0;
    }
};

// stores label offsets (key = label name, value = label offset)
std::map<const char*, int, StrCompare> gscr_label_offset_map;
// list of all gotos (pairs of origin gs_command pointer, and label name it calls)
std::list<std::pair<gs_command*, const char*> > gscr_gotos_list;
// stack of IF commands (to match appropriate ENDIF)
std::stack<gs_command*> gscr_if_stack;
// stack of WHEN commands (to match appropriate ENDWHEN)
std::stack<gs_command*> gscr_when_stack;
// stack of REPEAT commands (to match appropriate UNTIL)
std::stack<gs_command*> gscr_repeat_stack;
// stack of WHILE commands (to match appropriate ENDWHILE)
std::stack<gs_command*> gscr_while_stack;
// map of identifiers for timers
std::map<const char*, int, StrCompare> gscr_timer_map;
// map of identifiers for variables
std::map<const char*, int, StrCompare> gscr_variable_map;
// last id assigned to timer (will be incremented before assigning)
int gscr_last_timer_id = -1;
// last id assigned to variable
int gscr_last_variable_id = -1;

#define CLEANUP_AND_THROW(s) { delete ret; throw SyntaxErrorException(s); }

// exception class we will catch in parsing loop
class SyntaxErrorException : public std::exception
{
    protected:
        std::string msgstr;

    public:
        SyntaxErrorException(const char* message)
        {
            msgstr = std::string("SyntaxErrorException: ") + std::string(message);
        }

        SyntaxErrorException(std::string message)
        {
            msgstr = std::string("SyntaxErrorException: ") + message;
        }

        virtual const char* what() const throw()
        {
            return msgstr.c_str();
        }
};

// attempts to parse string to int, and returns true/false of success/fail
static bool tryStrToInt(int& dest, const char* src)
{
    // parsing hexadecimal number
    if (strlen(src) > 2 && src[0] == '0' && src[1] == 'x')
    {
        // validate hexa-numeric input
        for (int i = 0; src[i] != '\0'; i++)
        {
            if ((src[i] < '0' || src[i] > '9') && (src[i] < 'a' || src[i] > 'f') && (src[i] < 'A' || src[i] > 'F'))
                return false;
        }

        char* endptr;
        dest = strtol(src+2, &endptr, 16);
        if (endptr == src+2)
            return false;

        return true;
    }

    // validate numeric input
    for (int i = 0; src[i] != '\0'; i++)
    {
        if ((src[i] != '-' && (src[i] < '0' || src[i] > '9')) || (src[i] == '-' && i != 0))
            return false;
    }

    // use standard function (strtol) to convert numeric input
    char* endptr;
    dest = strtol(src, &endptr, 10);
    if (endptr == src)
        return false;

    return true;
}

// attempts to parse string to float, and returns true/false of success/fail
static bool tryStrToFloat(float& dest, const char* src)
{
    // validate numeric input
    for (int i = 0; src[i] != '\0'; i++)
    {
        if ((src[i] < '0' || src[i] > '9') && src[i] != '.' && src[i] != '-')
            return false;
    }

    // use standard function (strtof) to convert numeric input
    char* endptr;
    dest = strtof(src, &endptr);
    if (endptr == src)
        return false;

    return true;
}

static bool tryRecognizeFlag(int& target, uint32 field, std::string& str)
{
    gs_recognized_string* arr = nullptr;
    int count = 0;

    if (field == UNIT_FIELD_FLAGS)
    {
        arr = gs_recognized_unit_flags;
        count = sizeof(gs_recognized_unit_flags) / sizeof(gs_recognized_string);
    }
    else if (field == UNIT_FIELD_FLAGS_2)
    {
        arr = gs_recognized_unit_flags_2;
        count = sizeof(gs_recognized_unit_flags_2) / sizeof(gs_recognized_string);
    }
    else if (field == UNIT_NPC_FLAGS)
    {
        arr = gs_recognized_npc_flags;
        count = sizeof(gs_recognized_npc_flags) / sizeof(gs_recognized_string);
    }
    else if (field == UNIT_DYNAMIC_FLAGS)
    {
        arr = gs_recognized_unit_dynamic_flags;
        count = sizeof(gs_recognized_unit_dynamic_flags) / sizeof(gs_recognized_string);
    }
    else if (field == UNIT_FIELD_RESISTANCEBUFFMODSPOSITIVE)
    {
        arr = gs_recognized_mechanic_immunity;
        count = sizeof(gs_recognized_mechanic_immunity) / sizeof(gs_recognized_string);
    }
    else if (field == UNIT_FIELD_HOVERHEIGHT)
    {
        arr = gs_recognized_movetype;
        count = sizeof(gs_recognized_movetype) / sizeof(gs_recognized_string);
    }
    else if (field == UNIT_NPC_EMOTESTATE)
    {
        arr = gs_recognized_emote;
        count = sizeof(gs_recognized_emote) / sizeof(gs_recognized_string);
    }
    else
        return false;

    for (int i = 0; i < count; i++)
    {
        if (str == arr[i].str)
        {
            target = (int)(arr[i].value);
            return true;
        }
    }

    return false;
}

// parses operator from input string
static gs_specifier_operator gs_parse_operator(std::string& src)
{
    if (src == "==")
        return GSOP_EQUALS;
    else if (src == "!=")
        return GSOP_NOT_EQUAL;
    else if (src == "<")
        return GSOP_LESS_THAN;
    else if (src == "<=")
        return GSOP_LESS_OR_EQUAL;
    else if (src == ">")
        return GSOP_GREATER_THAN;
    else if (src == ">=")
        return GSOP_GREATER_OR_EQUAL;
    else if (src == "of")
        return GSOP_OF;
    else if (src == "is")
        return GSOP_IS;
    return GSOP_NONE;
}

// parses react state from input string
static ReactStates gs_parse_react_state(std::string& src)
{
    if (src == "aggressive")
        return REACT_AGGRESSIVE;
    else if (src == "defensive")
        return REACT_DEFENSIVE;
    else if (src == "passive")
        return REACT_PASSIVE;
    return REACT_AGGRESSIVE;
}

// proceeds parsing of specifier
gs_specifier gs_specifier::parse(const char* str)
{
    gs_specifier rr;

    rr.subject_type = GSST_NONE;
    rr.subject_parameter = GSSP_NONE;
    rr.value = 0;
    rr.flValue = 0.0f;
    rr.isFloat = false;

    // numeric value - recognized by first character
    if ((str[0] >= '0' && str[0] <= '9') || (str[0] == '-' && str[1] >= '0' && str[1] <= '9'))
    {
        // return success only when valid integer was supplied, otherwise carry on parsing
        if (tryStrToInt(rr.value, str))
            return rr;
        // next, try float
        if (tryStrToFloat(rr.flValue, str))
        {
            rr.isFloat = true;
            return rr;
        }
    }

    int lastpos = 0;
    size_t i;
    bool subject = false;
    // "less or equal" sign in combination with strlen may be considered dangerous, but
    // we need to catch even the last terminating zero character
    for (i = 0; i <= strlen(str); i++)
    {
        // dot character marks end of subject part, as well, as zero terminator when no subject specified
        if ((str[i] == '.' || str[i] == '\0' || str[i] == '(') && !subject)
        {
            // decide about specifier target type
            std::string subid = std::string(str).substr(0, i);
            // variable name - recognized by first character
            if (str[0] == '$')
            {
                std::string varname = subid.substr(1, subid.size()-1);
                auto itr = gscr_variable_map.find(varname.c_str());
                if (itr == gscr_variable_map.end())
                    return rr;

                rr.value = (*itr).second;
                rr.subject_type = GSST_VARIABLE_VALUE;
            }
            else if (subid == "me")
                rr.subject_type = GSST_ME;
            else if (subid == "target")
                rr.subject_type = GSST_TARGET;
            else if (subid == "random")
                rr.subject_type = GSST_RANDOM;
            else if (subid == "random_notank")
                rr.subject_type = GSST_RANDOM_NOTANK;
            else if (subid == "implicit")
                rr.subject_type = GSST_IMPLICIT;
            else if (subid == "chance")
                rr.subject_type = GSST_CHANCE;
            else if (subid == "timer")
                rr.subject_type = GSST_TIMER;
            else if (subid == "invoker")
                rr.subject_type = GSST_INVOKER;
            else if (subid == "parent")
                rr.subject_type = GSST_PARENT;
            else if (subid == "closest_creature")
                rr.subject_type = GSST_CLOSEST_CREATURE;
            else if (subid == "closest_player")
                rr.subject_type = GSST_CLOSEST_PLAYER;
            else if (subid == "last_summon")
                rr.subject_type = GSST_LAST_SUMMON;
            else if (subid == "creature_guid")
                rr.subject_type = GSST_CLOSEST_CREATURE_GUID;
            else if (subid == "ready")
            {
                rr.subject_type = GSST_STATE;
                rr.subject_parameter = GSSP_STATE_VALUE;
                rr.value = GSSV_READY;
            }
            else if (subid == "inprogress")
            {
                rr.subject_type = GSST_STATE;
                rr.subject_parameter = GSSP_STATE_VALUE;
                rr.value = GSSV_IN_PROGRESS;
            }
            else if (subid == "suspended")
            {
                rr.subject_type = GSST_STATE;
                rr.subject_parameter = GSSP_STATE_VALUE;
                rr.value = GSSV_SUSPENDED;
            }

            if (str[i] == '(')
            {
                lastpos = i;
                // find the end of parenthesis part
                while (str[i] != ')' && str[i] != '\0')
                    i++;
                std::string subpar = std::string(str).substr(lastpos + 1, i - lastpos - 1);

                if (rr.subject_type == GSST_CLOSEST_CREATURE || rr.subject_type == GSST_CLOSEST_CREATURE_GUID)
                    tryStrToInt(rr.value, subpar.c_str());
            }

            subject = true;
            lastpos = i;
        }
        // otherwise zero terminator signs that we just parsed the rest of the string, which is parameter
        else if (str[i] == '\0')
        {
            std::string parid = std::string(str).substr(lastpos + 1, i - lastpos);

            if (rr.subject_type == GSST_TIMER)
            {
                if (gscr_timer_map.find(parid.c_str()) == gscr_timer_map.end())
                    gscr_timer_map[parid.c_str()] = (++gscr_last_timer_id);

                rr.value = gscr_timer_map[parid.c_str()];
                rr.subject_parameter = GSSP_IDENTIFIER;
            }
            else if (parid == "health")
                rr.subject_parameter = GSSP_HEALTH;
            else if (parid == "health_pct")
                rr.subject_parameter = GSSP_HEALTH_PCT;
            else if (parid == "faction")
                rr.subject_parameter = GSSP_FACTION;
            else if (parid == "level")
                rr.subject_parameter = GSSP_LEVEL;
            else if (parid == "combat")
                rr.subject_parameter = GSSP_COMBAT;
            else if (parid == "mana")
                rr.subject_parameter = GSSP_MANA;
            else if (parid == "mana_pct")
                rr.subject_parameter = GSSP_MANA_PCT;
            else if (parid == "distance")
                rr.subject_parameter = GSSP_DISTANCE;
            else if (parid == "position_x")
                rr.subject_parameter = GSSP_POS_X;
            else if (parid == "position_y")
                rr.subject_parameter = GSSP_POS_Y;
            else if (parid == "position_z")
                rr.subject_parameter = GSSP_POS_Z;
            else if (parid == "orientation")
                rr.subject_parameter = GSSP_ORIENTATION;
            else if (parid == "alive")
                rr.subject_parameter = GSSP_ALIVE;
            else if (parid == "guid")
                rr.subject_parameter = GSSP_GUIDLOW;
            else if (parid == "entry")
                rr.subject_parameter = GSSP_ENTRY;
        }
    }

    return rr;
}

// proceeds parsing of command
gs_command* gs_command::parse(gs_command_proto* src, int offset)
{
    size_t i;
    gs_command* ret = new gs_command;

    // find instruction type for string identifier
    ret->type = GSCR_NONE;
    for (i = 0; i < sizeof(gscr_identifiers)/sizeof(std::string); i++)
    {
        if (src->instruction == gscr_identifiers[i])
        {
            ret->type = (gs_command_type)i;
            break;
        }
    }

    // if we haven't found valid instruction, end parsing
    if (ret->type == GSCR_NONE)
        CLEANUP_AND_THROW(std::string("invalid instruction: ") + src->instruction);

    ret->command_delegate = gs_specifier::make_default_subject(GSST_ME);
    // parse command delegate if any
    if (src->parameters.size() > 0)
    {
        std::string lastpar = src->parameters[src->parameters.size() - 1];
        // it has to contain at least one character inside [ ] parenthesis
        if (lastpar.size() > 2 && lastpar[0] == '[' && lastpar[lastpar.size() - 1] == ']')
        {
            ret->command_delegate = gs_specifier::parse(lastpar.substr(1, lastpar.size() - 2).c_str());
            // delegate is not considered parameter for specific instruction, remove it
            src->parameters.erase(src->parameters.begin() + src->parameters.size() - 1);
        }
    }

    switch (ret->type)
    {
        default:
        case GSCR_NONE:
            break;
        // label instruction - just store to map
        // Syntax: label <name>
        case GSCR_LABEL:
            if (src->parameters.size() == 0)
                CLEANUP_AND_THROW("no label name specified for instruction LABEL");
            if (src->parameters.size() > 1)
                CLEANUP_AND_THROW("specify just label name for instruction LABEL");

            if (gscr_label_offset_map.find(src->parameters[0].c_str()) != gscr_label_offset_map.end())
                CLEANUP_AND_THROW(std::string("label '") + src->parameters[0] + std::string("' already exists!"));

            // store to map to be later resolved by gotos
            gscr_label_offset_map[src->parameters[0].c_str()] = offset;
            break;
        // goto instruction - push to list to be resolved later
        // Syntax: goto <label name>
        case GSCR_GOTO:
            if (src->parameters.size() == 0)
                CLEANUP_AND_THROW("no label name specified for instruction LABEL");
            if (src->parameters.size() > 1)
                CLEANUP_AND_THROW("specify just label name for instruction LABEL");

            gscr_gotos_list.push_back(std::make_pair(ret, src->parameters[0].c_str()));
            break;
        // wait instruction - to stop script for some time
        // Syntax: wait <time> [flag] [flag] ..
        case GSCR_WAIT:
            if (src->parameters.size() == 0)
                CLEANUP_AND_THROW("too few parameters for instruction WAIT");

            ret->params.c_wait.delay = gs_specifier::parse(src->parameters[0].c_str());

            ret->params.c_wait.flags = 0;
            // any more parameters are considered wait flags
            if (src->parameters.size() > 1)
            {
                for (i = 1; i < src->parameters.size(); i++)
                {
                    if (src->parameters[i] == "melee")
                        ret->params.c_wait.flags |= GSWF_MELEE_ATTACK;
                    //else if (src->parameters[i] == " ... ")
                    //    ret->params.c_wait.flags |= GSWF_...;
                    else
                        CLEANUP_AND_THROW(std::string("unknown flag '") + src->parameters[i] + std::string("' for WAIT"));
                }
            }
            break;
        // cast instruction for spellcast
        // Syntax: cast <spellid> [target specifier] [triggered]
        case GSCR_CAST:
            if (src->parameters.size() == 0)
                CLEANUP_AND_THROW("too few parameters for instruction CAST");
            if (src->parameters.size() > 3)
                CLEANUP_AND_THROW("too many parameters for instruction CAST");

            ret->params.c_cast.spell = gs_specifier::parse(src->parameters[0].c_str());

            // by default the spell is not triggered
            ret->params.c_cast.triggered = false;
            // if it has second parameter, it's target type specifier
            if (src->parameters.size() > 1)
            {
                ret->params.c_cast.target_type = gs_specifier::parse(src->parameters[1].c_str());
                // if it has third parameter, it's triggered flag
                if (src->parameters.size() > 2)
                {
                    if (src->parameters[2] == "triggered")
                        ret->params.c_cast.triggered = true;
                    else
                        CLEANUP_AND_THROW("invalid parameter 3 (triggered flag) for CAST");
                }
            }
            else // use implicit targetting as fallback
                ret->params.c_cast.target_type = gs_specifier::make_default_subject(GSST_IMPLICIT);

            break;
        // say and yell instructions are basically the same
        // Syntax: say "Hello world!"
        //         yell "Hello universe!"
        //         textemote "Hello Milky Way!"
        //         bossemote "Hello everything!"
        case GSCR_SAY:
        case GSCR_YELL:
        case GSCR_TEXTEMOTE:
        case GSCR_BOSSEMOTE:
            if (src->parameters.size() < 1)
                CLEANUP_AND_THROW("too few parameters for instruction SAY/YELL/TEXTEMOTE/BOSSEMOTE");
            if (src->parameters.size() > 2)
                CLEANUP_AND_THROW("too many parameters for instruction SAY/YELL/TEXTEMOTE/BOSSEMOTE");

            // store string to be said / yelled
            ret->params.c_say_yell.tosay = src->parameters[0].c_str();
            // store sound ID if supplied
            if (src->parameters.size() == 2)
                ret->params.c_say_yell.sound_id = gs_specifier::parse(src->parameters[1].c_str());
            else
                ret->params.c_say_yell.sound_id = gs_specifier::make_default_value(0);

            break;
        // if instruction - standard control sequence; needs to be closed by endif
        // Syntax: if <specifier> [<operator> <specifier>]
        case GSCR_IF:
            if (src->parameters.size() == 0)
                CLEANUP_AND_THROW("too few parameters for instruction IF");

            // parse source specifier
            ret->params.c_if.condition.source = gs_specifier::parse(src->parameters[0].c_str());
            // if the instruction has more parameters than one..
            if (src->parameters.size() > 1)
            {
                // .. it has to be 3, otherwise it's considered error
                if (src->parameters.size() == 3)
                {
                    ret->params.c_if.condition.op = gs_parse_operator(src->parameters[1]);
                    ret->params.c_if.condition.dest = gs_specifier::parse(src->parameters[2].c_str());
                }
                else
                    CLEANUP_AND_THROW("invalid IF statement, use subject, or subject + operator + subject");
            }

            gscr_if_stack.push(ret);

            break;
        // endif instruction - just pops latest IF from stack and sets its offset there
        // Syntax: endif
        case GSCR_ENDIF:
        {
            if (gscr_if_stack.empty())
                CLEANUP_AND_THROW("invalid ENDIF - no matching IF");

            // pop matching IF from if-stack
            gs_command* matching = gscr_if_stack.top();
            gscr_if_stack.pop();

            // sets offset of this instruction (for possible jumping) to the if statement command
            matching->params.c_if.endif_offset = offset;
            break;
        }
        // combatstop instruction - stops combat (melee attack, threat, ..)
        // Syntax: combatstop
        case GSCR_COMBATSTOP:
            if (src->parameters.size() != 0)
                CLEANUP_AND_THROW("too many parameters for instruction COMBATSTOP");
            break;
        // faction instruction - changes faction of script owner
        // Syntax: faction <factionId>
        //         faction restore
        case GSCR_FACTION:
            if (src->parameters.size() == 0)
                CLEANUP_AND_THROW("too few parameters for instruction FACTION");
            if (src->parameters.size() > 1)
                CLEANUP_AND_THROW("too many parameters for instruction FACTION");

            // restore faction
            if (src->parameters[0] == "restore")
                ret->params.c_faction.faction = gs_specifier::make_default_value(0);
            // or set faction
            else
                ret->params.c_faction.faction = gs_specifier::parse(src->parameters[0].c_str());

            break;
        // react instruction - sets react state of creature
        // Syntax: react <react state string>
        // react state string: aggressive, defensive, passive
        case GSCR_REACT:
            if (src->parameters.size() == 0)
                CLEANUP_AND_THROW("too few parameters for instruction REACT");
            if (src->parameters.size() > 1)
                CLEANUP_AND_THROW("too many parameters for instruction REACT");

            ret->params.c_react.reactstate = gs_parse_react_state(src->parameters[0]);
            break;
        // kill instruction - kills target specified
        // Syntax: kill <target specifier>
        case GSCR_KILL:
            if (src->parameters.size() == 0)
                CLEANUP_AND_THROW("too few parameters for instruction KILL");
            if (src->parameters.size() > 1)
                CLEANUP_AND_THROW("too many parameters for instruction KILL");

            ret->params.c_kill.target = gs_specifier::parse(src->parameters[0].c_str());
            break;
        // combatstart instruction - starts combat (melee attack, threat, ..)
        // Syntax: combatstart
        case GSCR_COMBATSTART:
            if (src->parameters.size() != 0)
                CLEANUP_AND_THROW("too many parameters for instruction COMBATSTART");
            break;
        // timer instruction - sets timer, etc.
        // Syntax: timer <timer name> set <timer value>
        case GSCR_TIMER:
        {
            if (src->parameters.size() < 3)
                CLEANUP_AND_THROW("too few parameters for instruction TIMER");
            if (src->parameters.size() > 3)
                CLEANUP_AND_THROW("too many parameters for instruction TIMER");

            std::string& timname = src->parameters[0];

            if (src->parameters[1] != "set")
                CLEANUP_AND_THROW("invalid operation in instruction TIMER");

            ret->params.c_timer.value = gs_specifier::parse(src->parameters[2].c_str());

            if (gscr_timer_map.find(timname.c_str()) == gscr_timer_map.end())
                gscr_timer_map[timname.c_str()] = (++gscr_last_timer_id);

            ret->params.c_timer.timer_id = gscr_timer_map[timname.c_str()];

            break;
        }
        // morph instruction - changes model id of script owner
        // Syntax: morph <modelId>
        //         morph restore
        case GSCR_MORPH:
            if (src->parameters.size() == 0)
                CLEANUP_AND_THROW("too few parameters for instruction MORPH");
            if (src->parameters.size() > 1)
                CLEANUP_AND_THROW("too many parameters for instruction MORPH");

            // restore morph
            if (src->parameters[0] == "restore")
                ret->params.c_morph.morph_id = gs_specifier::make_default_value(0);
            // or set morph
            else
                ret->params.c_morph.morph_id = gs_specifier::parse(src->parameters[0].c_str());

            break;
        // summon instruction - summons NPC at position
        // Syntax: summon <entry>
        //         summon <entry> <x> <y> <z>
        case GSCR_SUMMON:
            if (src->parameters.size() == 0)
                CLEANUP_AND_THROW("too few parameters for instruction SUMMON");
            if (src->parameters.size() != 1 && src->parameters.size() != 4 && src->parameters.size() != 5)
                CLEANUP_AND_THROW("invalid parameter count for instruction SUMMON - use 1, 4 or 5 params");

            ret->params.c_summon.creature_entry = gs_specifier::parse(src->parameters[0].c_str());
            ret->params.c_summon.nodespawn = false;

            if (src->parameters.size() >= 4)
            {
                ret->params.c_summon.x = gs_specifier::parse(src->parameters[1].c_str());
                ret->params.c_summon.y = gs_specifier::parse(src->parameters[2].c_str());
                ret->params.c_summon.z = gs_specifier::parse(src->parameters[3].c_str());

                if (src->parameters.size() == 5)
                {
                    if (src->parameters[4] == "nodespawn")
                        ret->params.c_summon.nodespawn = true;
                }
            }
            else
            {
                ret->params.c_summon.x = gs_specifier::make_default_float_value(0.0f);
                ret->params.c_summon.y = gs_specifier::make_default_float_value(0.0f);
                ret->params.c_summon.z = gs_specifier::make_default_float_value(0.0f);
            }
            break;
        // walk, run and teleport instructions - move to destination position with specified type
        // Syntax: walk <x> <y> <z>
        //         run <x> <y> <z>
        //         teleport <x> <y> <z>
        case GSCR_WALK:
        case GSCR_RUN:
        case GSCR_TELEPORT:
            if (src->parameters.size() != 3)
                CLEANUP_AND_THROW("invalid parameter count for instruction WALK / RUN / TELEPORT - use 3 params");

            ret->params.c_walk_run_teleport.x = gs_specifier::parse(src->parameters[0].c_str());
            ret->params.c_walk_run_teleport.y = gs_specifier::parse(src->parameters[1].c_str());
            ret->params.c_walk_run_teleport.z = gs_specifier::parse(src->parameters[2].c_str());
            break;
        // waitfor instruction - waits for event to occur
        // Syntax: waitfor <event type>
        case GSCR_WAITFOR:
            if (src->parameters.size() != 1)
                CLEANUP_AND_THROW("invalid parameter count for instruction WAITFOR - use 1 parameter");

            if (src->parameters[0] == "movement")
                ret->params.c_waitfor.eventtype = GSET_MOVEMENT;
            else if (src->parameters[0] == "cast")
                ret->params.c_waitfor.eventtype = GSET_CAST;
            else
                CLEANUP_AND_THROW("invalid event specifier for instruction WAITFOR");

            break;
        // lock / unlock instruction - locks or unlocks script change
        // Syntax: lock
        //         unlock
        case GSCR_LOCK:
        case GSCR_UNLOCK:
            if (src->parameters.size() != 0)
                CLEANUP_AND_THROW("invalid parameter count for instruction LOCK / UNLOCK - do not supply parameters");
            break;
        // scale instruction - changes scale of NPC
        // Syntax: scale <scale value>
        case GSCR_SCALE:
            if (src->parameters.size() != 1)
                CLEANUP_AND_THROW("invalid parameter count for instruction SCALE - use 1 parameter");

            ret->params.c_scale.restore = false;
            if (src->parameters[0] == "restore")
            {
                ret->params.c_scale.restore = true;
                ret->params.c_scale.scale = gs_specifier::make_default_float_value(0.0f);
            }
            else
                ret->params.c_scale.scale = gs_specifier::parse(src->parameters[0].c_str());

            break;
        // flags instruction - sets/adds/removes flag
        // Syntax: flags <field identifier> add <flag or name>
        //         flags <field identifier> remove <flag or name>
        //         flags <field identifier> set <flag or name>
        case GSCR_FLAGS:
            if (src->parameters.size() < 3)
                CLEANUP_AND_THROW("too few parameters for instruction FLAGS");
            if (src->parameters.size() > 3)
                CLEANUP_AND_THROW("too many parameters for instruction FLAGS");

            if (src->parameters[1] == "add")
                ret->params.c_flags.op = GSFO_ADD;
            else if (src->parameters[1] == "remove")
                ret->params.c_flags.op = GSFO_REMOVE;
            else if (src->parameters[1] == "set")
                ret->params.c_flags.op = GSFO_SET;
            else
                CLEANUP_AND_THROW("invalid operation for instruction FLAGS");

            if (src->parameters[0] == "unit_flags")
                ret->params.c_flags.field = UNIT_FIELD_FLAGS;
            else if (src->parameters[0] == "unit_flags2")
                ret->params.c_flags.field = UNIT_FIELD_FLAGS_2;
            else if (src->parameters[0] == "dynamic_flags")
                ret->params.c_flags.field = UNIT_DYNAMIC_FLAGS;
            else if (src->parameters[0] == "npc_flags")
                ret->params.c_flags.field = UNIT_NPC_FLAGS;
            else
                CLEANUP_AND_THROW("unknown flags identifier for instruction FLAGS");

            if (!tryStrToInt(ret->params.c_flags.value, src->parameters[2].c_str()))
            {
                if (!tryRecognizeFlag(ret->params.c_flags.value, ret->params.c_flags.field, src->parameters[2]))
                    CLEANUP_AND_THROW("could not recognize flag name / identifier in FLAGS instruction");
            }

            break;
        // immunity instruction - change immunity mask of owner
        // Syntax: immunity add <flag or name>
        //         immunity remove <flag or name>
        //         immunity set <flag or name>
        case GSCR_IMMUNITY:
            if (src->parameters.size() < 2)
                CLEANUP_AND_THROW("too few parameters for instruction IMMUNITY");
            if (src->parameters.size() > 2)
                CLEANUP_AND_THROW("too many parameters for instruction IMMUNITY");

            if (src->parameters[0] == "add")
                ret->params.c_immunity.op = GSFO_ADD;
            else if (src->parameters[0] == "remove")
                ret->params.c_immunity.op = GSFO_REMOVE;
            else
                CLEANUP_AND_THROW("invalid operation for instruction IMMUNITY, use add or remove");

            if (!tryStrToInt(ret->params.c_immunity.mask, src->parameters[1].c_str()))
            {
                if (!tryRecognizeFlag(ret->params.c_immunity.mask, UNIT_FIELD_RESISTANCEBUFFMODSPOSITIVE /* dummy, just to not be in conflict */, src->parameters[1]))
                    CLEANUP_AND_THROW("could not recognize flag name / identifier in IMMUNITY instruction");
            }

            break;
        // emote instruction - plays emote on subject
        // Syntax: emote <emote id> [<subject>]
        case GSCR_EMOTE:
        {
            if (src->parameters.size() < 1)
                CLEANUP_AND_THROW("too few parameters for instruction EMOTE");
            if (src->parameters.size() > 2)
                CLEANUP_AND_THROW("too many parameters for instruction EMOTE");

            int32 emote;
            if (!tryRecognizeFlag(emote, UNIT_NPC_EMOTESTATE, src->parameters[0]))
                ret->params.c_emote.emote_id = gs_specifier::parse(src->parameters[0].c_str());
            else
                ret->params.c_emote.emote_id = gs_specifier::make_default_value(emote);

            if (src->parameters.size() == 2)
                ret->params.c_emote.subject = gs_specifier::parse(src->parameters[1].c_str());
            else
                ret->params.c_emote.subject.subject_type = GSST_ME;

            break;
        }
        // movie instruction - plays movie to subject
        // Syntax: movie <movie id> <subject>
        case GSCR_MOVIE:
            if (src->parameters.size() < 2)
                CLEANUP_AND_THROW("too few parameters for instruction MOVIE");
            if (src->parameters.size() > 2)
                CLEANUP_AND_THROW("too many parameters for instruction MOVIE");

            ret->params.c_movie.movie_id = gs_specifier::parse(src->parameters[0].c_str());
            ret->params.c_movie.subject = gs_specifier::parse(src->parameters[1].c_str());

            break;
        // aura instruction - adds/removes aura to subject
        // Syntax: aura add <spell id> [<subject>]
        //         aura remove <spell id> [<subject>]
        case GSCR_AURA:
            if (src->parameters.size() < 2)
                CLEANUP_AND_THROW("too few parameters for instruction AURA");
            if (src->parameters.size() > 3)
                CLEANUP_AND_THROW("too many parameters for instruction AURA");

            if (src->parameters[0] == "add")
                ret->params.c_aura.op = GSFO_ADD;
            else if (src->parameters[0] == "remove")
                ret->params.c_aura.op = GSFO_REMOVE;
            else
                CLEANUP_AND_THROW("invalid operation for instruction AURA, use add or remove");

            ret->params.c_aura.aura_id = gs_specifier::parse(src->parameters[1].c_str());

            if (src->parameters.size() == 3)
                ret->params.c_aura.subject = gs_specifier::parse(src->parameters[2].c_str());
            else
                ret->params.c_aura.subject.subject_type = GSST_ME;

            break;
        // speed instruction - sets movement speed of specified move type
        // Syntax: speed <movetype> <value>
        case GSCR_SPEED:
            if (src->parameters.size() < 2)
                CLEANUP_AND_THROW("too few parameters for instruction SPEED");
            if (src->parameters.size() > 2)
                CLEANUP_AND_THROW("too many parameters for instruction SPEED");

            if (!tryStrToInt(ret->params.c_speed.movetype, src->parameters[0].c_str()))
            {
                if (!tryRecognizeFlag(ret->params.c_speed.movetype, UNIT_FIELD_HOVERHEIGHT /* dummy, to avoid conflict */, src->parameters[0]))
                    CLEANUP_AND_THROW("could not recognize move type name / identifier in SPEED instruction");
            }

            ret->params.c_speed.speed = gs_specifier::parse(src->parameters[1].c_str());

            break;
        // move instruction - sets implicit move type
        // Syntax: move <movetype>
        case GSCR_MOVE:
            if (src->parameters.size() < 1)
                CLEANUP_AND_THROW("too few parameters for instruction MOVE");
            if (src->parameters.size() > 1)
                CLEANUP_AND_THROW("too many parameters for instruction MOVE");

            if (src->parameters[0] == "idle")
            {
                ret->params.c_move.movetype = -1;
            }
            else
            {
                if (!tryStrToInt(ret->params.c_move.movetype, src->parameters[0].c_str()))
                {
                    if (!tryRecognizeFlag(ret->params.c_move.movetype, UNIT_FIELD_HOVERHEIGHT /* dummy, to avoid conflict */, src->parameters[0]))
                        CLEANUP_AND_THROW("could not recognize move type name / identifier in MOVE instruction");
                }
            }

            break;
        // mount instruction - sets mounted state for script owner
        // Syntax: mount <display id>
        case GSCR_MOUNT:
            if (src->parameters.size() < 1)
                CLEANUP_AND_THROW("too few parameters for instruction MOUNT");
            if (src->parameters.size() > 1)
                CLEANUP_AND_THROW("too many parameters for instruction MOUNT");
            ret->params.c_mount.mount_model_id = gs_specifier::parse(src->parameters[0].c_str());
            break;
        // unmount instruction - dismounts script owner from mount
        // Syntax: unmount
        case GSCR_UNMOUNT:
            if (src->parameters.size() != 0)
                CLEANUP_AND_THROW("invalid parameter count for instruction UNMOUNT - do not supply parameters");
            break;
        // quest instruction - manipulates with quest state and objectives
        // Syntax: quest complete <quest id>
        //         quest failed <quest id>
        //         quest progress <quest id> <objective> <add value>
        case GSCR_QUEST:
            if (src->parameters.size() < 2)
                CLEANUP_AND_THROW("too few parameters for instruction QUEST");
            if (src->parameters.size() > 4)
                CLEANUP_AND_THROW("too many parameters for instruction QUEST");

            ret->params.c_quest.op = GSQO_NONE;
            if (src->parameters[0] == "complete")
                ret->params.c_quest.op = GSQO_COMPLETE;
            else if (src->parameters[0] == "failed")
                ret->params.c_quest.op = GSQO_FAIL;
            else if (src->parameters[0] == "progress")
            {
                ret->params.c_quest.op = GSQO_PROGRESS;
                if (src->parameters.size() < 3)
                    CLEANUP_AND_THROW("missing objective parameter for instruction QUEST, operation PROGRESS");
            }
            else
                CLEANUP_AND_THROW("invalid quest operation for instruction QUEST");

            ret->params.c_quest.quest_id = gs_specifier::parse(src->parameters[1].c_str());

            if (ret->params.c_quest.op == GSQO_PROGRESS)
            {
                ret->params.c_quest.objective_index = gs_specifier::parse(src->parameters[2].c_str());

                if (ret->params.c_quest.objective_index.value < 0 || ret->params.c_quest.objective_index.value > 3)
                    CLEANUP_AND_THROW("objective index is out of 0..3 range for instruction QUEST");

                ret->params.c_quest.value = gs_specifier::make_default_value(1);

                if (src->parameters.size() == 4)
                    ret->params.c_quest.value = gs_specifier::parse(src->parameters[3].c_str());
            }

            break;
        // unmount instruction - dismounts script owner from mount
        // Syntax: unmount
        case GSCR_DESPAWN:
            if (src->parameters.size() > 1)
                CLEANUP_AND_THROW("invalid parameter count for instruction DESPAWN - do not supply parameters, or supply one");

            if (src->parameters.size() == 0)
                ret->params.c_despawn.subject = gs_specifier::make_default_subject(GSST_ME, GSSP_NONE);
            else
                ret->params.c_despawn.subject = gs_specifier::parse(src->parameters[0].c_str());

            break;
        // repeat instruction - standard control sequence; needs to be closed by until
        // Syntax: repeat
        case GSCR_REPEAT:
            if (src->parameters.size() != 0)
                CLEANUP_AND_THROW("too many parameters for instruction REPEAT");

            ret->params.c_repeat.offset = offset;

            gscr_repeat_stack.push(ret);
            break;
        // until instruction - just pops latest REPEAT from stack and sets its offset there; also declares condition
        // Syntax: until <subject> <operator> <subject>
        case GSCR_UNTIL:
        {
            if (gscr_repeat_stack.empty())
                CLEANUP_AND_THROW("invalid UNTIL - no matching REPEAT");

            if (src->parameters.size() == 0)
                CLEANUP_AND_THROW("too few parameters for instruction UNTIL");

            // parse source specifier
            ret->params.c_until.condition.source = gs_specifier::parse(src->parameters[0].c_str());
            // if the instruction has more parameters than one..
            if (src->parameters.size() > 1)
            {
                // .. it has to be 3, otherwise it's considered error
                if (src->parameters.size() == 3)
                {
                    ret->params.c_until.condition.op = gs_parse_operator(src->parameters[1]);
                    ret->params.c_until.condition.dest = gs_specifier::parse(src->parameters[2].c_str());
                }
                else
                    CLEANUP_AND_THROW("invalid UNTIL condition statement, use subject, or subject + operator + subject");
            }

            // pop matching REPEAT from if-stack
            gs_command* matching = gscr_repeat_stack.top();
            gscr_repeat_stack.pop();

            // sets offset of this instruction (for possible jumping) to the repeat statement command
            ret->params.c_until.repeat_offset = matching->params.c_repeat.offset;
            break;
        }
        // while instruction - standard control sequence; needs to be closed by endwhile
        // Syntax: while <specifier> [<operator> <specifier>]
        case GSCR_WHILE:
            if (src->parameters.size() == 0)
                CLEANUP_AND_THROW("too few parameters for instruction WHILE");

            // parse source specifier
            ret->params.c_while.condition.source = gs_specifier::parse(src->parameters[0].c_str());
            // if the instruction has more parameters than one..
            if (src->parameters.size() > 1)
            {
                // .. it has to be 3, otherwise it's considered error
                if (src->parameters.size() == 3)
                {
                    ret->params.c_while.condition.op = gs_parse_operator(src->parameters[1]);
                    ret->params.c_while.condition.dest = gs_specifier::parse(src->parameters[2].c_str());
                }
                else
                    CLEANUP_AND_THROW("invalid WHILE statement, use subject, or subject + operator + subject");
            }

            ret->params.c_while.my_offset = offset;

            gscr_while_stack.push(ret);

            break;
        // endwhile instruction - just pops latest WHILE from stack and sets its offset there
        // Syntax: endwhile
        case GSCR_ENDWHILE:
        {
            if (gscr_while_stack.empty())
                CLEANUP_AND_THROW("invalid ENDWHILE - no matching WHILE");

            // pop matching IF from if-stack
            gs_command* matching = gscr_while_stack.top();
            gscr_while_stack.pop();

            // sets offset of this instruction (for possible jumping) to the if statement command
            matching->params.c_while.endwhile_offset = offset;
            ret->params.c_endwhile.while_offset = matching->params.c_while.my_offset;
            break;
        }
        // var instruction - working with variables
        // Syntax: var <operation> <variable name> [<value to work with>]
        case GSCR_VAR:
        {
            if (src->parameters.size() < 2)
                CLEANUP_AND_THROW("too few parameters for VAR instruction");

            std::string operation = src->parameters[0];
            if (operation == "set")
                ret->params.c_var.op = GSNOP_ASSIGN;
            else if (operation == "add")
                ret->params.c_var.op = GSNOP_ADD;
            else if (operation == "sub")
                ret->params.c_var.op = GSNOP_SUBTRACT;
            else if (operation == "mul")
                ret->params.c_var.op = GSNOP_MULTIPLY;
            else if (operation == "div")
                ret->params.c_var.op = GSNOP_DIVIDE;
            else if (operation == "ndiv")
                ret->params.c_var.op = GSNOP_DIVIDE_INT;
            else if (operation == "mod")
                ret->params.c_var.op = GSNOP_MODULO;
            else if (operation == "inc")
                ret->params.c_var.op = GSNOP_INCREMENT;
            else if (operation == "dec")
                ret->params.c_var.op = GSNOP_DECREMENT;
            else
                CLEANUP_AND_THROW("invalid operation for VAR instruction");

            if (gscr_variable_map.find(src->parameters[1].c_str()) == gscr_variable_map.end())
                gscr_variable_map[src->parameters[1].c_str()] = ++gscr_last_variable_id;

            ret->params.c_var.variable = gscr_variable_map[src->parameters[1].c_str()];

            if (ret->params.c_var.op != GSNOP_INCREMENT && ret->params.c_var.op != GSNOP_DECREMENT)
            {
                if (src->parameters.size() != 3)
                    CLEANUP_AND_THROW("not enough parameters for instruction VAR");

                ret->params.c_var.spec = gs_specifier::parse(src->parameters[2].c_str());
            }

            break;
        }
        // sound instruction - plays sound directly to player, or to environment
        // Syntax: sound <sound id> [<target>]
        case GSCR_SOUND:
            if (src->parameters.size() < 1)
                CLEANUP_AND_THROW("too few parameters for instruction SOUND");
            if (src->parameters.size() > 2)
                CLEANUP_AND_THROW("too many parameters for instruction SOUND");

            ret->params.c_sound.sound_id = gs_specifier::parse(src->parameters[0].c_str());

            if (src->parameters.size() == 2)
                ret->params.c_sound.target = gs_specifier::parse(src->parameters[1].c_str());
            else
                ret->params.c_sound.target.subject_type = GSST_NONE;
            break;
        // when instruction - standard control sequence, but entered just once; needs to be closed by endwhen
        // Syntax: when <specifier> [<operator> <specifier>]
        case GSCR_WHEN:
            if (src->parameters.size() == 0)
                CLEANUP_AND_THROW("too few parameters for instruction WHEN");

            // parse source specifier
            ret->params.c_when.condition.source = gs_specifier::parse(src->parameters[0].c_str());
            // if the instruction has more parameters than one..
            if (src->parameters.size() > 1)
            {
                // .. it has to be 3, otherwise it's considered error
                if (src->parameters.size() == 3)
                {
                    ret->params.c_when.condition.op = gs_parse_operator(src->parameters[1]);
                    ret->params.c_when.condition.dest = gs_specifier::parse(src->parameters[2].c_str());
                }
                else
                    CLEANUP_AND_THROW("invalid WHEN statement, use subject, or subject + operator + subject");
            }

            gscr_when_stack.push(ret);

            break;
        // endwhen instruction - just pops latest WHEN from stack and sets its offset there
        // Syntax: endwhen
        case GSCR_ENDWHEN:
        {
            if (gscr_when_stack.empty())
                CLEANUP_AND_THROW("invalid ENDWHEN - no matching WHEN");

            // pop matching WHEN from if-stack
            gs_command* matching = gscr_when_stack.top();
            gscr_when_stack.pop();

            // sets offset of this instruction (for possible jumping) to the when statement command
            matching->params.c_when.endwhen_offset = offset;
            break;
        }
        // talk instruction - says something from creature_text table assigned with current NPC
        // Syntax: talk <group id> [<target>]
        case GSCR_TALK:
            if (src->parameters.size() < 1)
                CLEANUP_AND_THROW("too few parameters for instruction TALK");
            if (src->parameters.size() > 2)
                CLEANUP_AND_THROW("too many parameters for instruction TALK");

            ret->params.c_talk.talk_group_id = gs_specifier::parse(src->parameters[0].c_str());

            if (src->parameters.size() == 2)
                ret->params.c_talk.talk_target = gs_specifier::parse(src->parameters[1].c_str());
            else
                ret->params.c_talk.talk_target.subject_type = GSST_NONE;
            break;
        // turn instruction - changes orientation of script owner
        // Syntax: turn <orientation or unit>
        //         turn by <relative orientation>
        case GSCR_TURN:
            if (src->parameters.size() < 1)
                CLEANUP_AND_THROW("too few parameters for instruction TURN");
            if (src->parameters.size() > 2)
                CLEANUP_AND_THROW("too many parameters for instruction TURN");

            if (src->parameters.size() == 2 && src->parameters[0] == "by")
            {
                ret->params.c_turn.relative = true;
                ret->params.c_turn.amount = gs_specifier::parse(src->parameters[1].c_str());
            }
            else if (src->parameters.size() == 1)
            {
                ret->params.c_turn.relative = false;
                ret->params.c_turn.amount = gs_specifier::parse(src->parameters[0].c_str());
            }
            else
                CLEANUP_AND_THROW("invalid parameters for instruction TURN");

            break;
        // follow instruction - follows specifier target
        // Syntax: follow <who> [<distance> [<angle>]]
        //         follow stop
        case GSCR_FOLLOW:
            if (src->parameters.size() < 1)
                CLEANUP_AND_THROW("too few parameters for instruction FOLLOW");
            if (src->parameters.size() > 3)
                CLEANUP_AND_THROW("too many parameters for instruction FOLLOW");

            if (src->parameters[0] == "stop")
                ret->params.c_follow.subject.subject_type = GSST_NONE;
            else
            {
                ret->params.c_follow.subject = gs_specifier::parse(src->parameters[0].c_str());
                if (src->parameters.size() > 1)
                {
                    ret->params.c_follow.distance = gs_specifier::parse(src->parameters[1].c_str());
                    if (src->parameters.size() == 3)
                        ret->params.c_follow.angle = gs_specifier::parse(src->parameters[2].c_str());
                    else
                        ret->params.c_follow.angle = gs_specifier::make_default_float_value(M_PI);
                }
                else
                {
                    ret->params.c_follow.distance = gs_specifier::make_default_float_value(INTERACTION_DISTANCE);
                    ret->params.c_follow.angle = gs_specifier::make_default_float_value(M_PI);
                }
            }

            break;
    }

    return ret;
}

// analyzes sequence of command prototypes and parses lines of input to output CommandVector
CommandVector* gscr_analyseSequence(CommandProtoVector* input, int scriptId)
{
    CommandVector* cv = new CommandVector;
    gs_command* tmp;
    size_t i;

    gscr_label_offset_map.clear();
    gscr_gotos_list.clear();
    gscr_timer_map.clear();
    gscr_variable_map.clear();
    gscr_last_timer_id = -1;
    gscr_last_variable_id = -1;

    while (!gscr_if_stack.empty())
        gscr_if_stack.pop();
    while (!gscr_when_stack.empty())
        gscr_when_stack.pop();
    while (!gscr_repeat_stack.empty())
        gscr_repeat_stack.pop();
    while (!gscr_while_stack.empty())
        gscr_while_stack.pop();

    try
    {
        // parse every input line
        for (i = 0; i < input->size(); i++)
        {
            tmp = gs_command::parse((*input)[i], i);
            if (tmp)
                cv->push_back(tmp);
        }

        // pair gotos with labels
        for (auto itr = gscr_gotos_list.begin(); itr != gscr_gotos_list.end(); ++itr)
        {
            if (gscr_label_offset_map.find((*itr).second) == gscr_label_offset_map.end())
                throw SyntaxErrorException(std::string("no matching label '") + (*itr).second + std::string("' for goto"));

            (*itr).first->params.c_goto_label.instruction_offset = gscr_label_offset_map[(*itr).second];
        }
    }
    catch (std::exception& e)
    {
        sLog->outError("GSAI ID %u Exception: %s", scriptId, e.what());
        delete cv;
        cv = nullptr;
    }

    return cv;
}
