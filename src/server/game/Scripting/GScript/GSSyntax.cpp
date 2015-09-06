#include "gamePCH.h"
#include "GSCommands.h"
#include <exception>
#include <stack>
#include <map>

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
// map of identifiers for timers
std::map<const char*, int, StrCompare> gscr_timer_map;
// last id assigned to timer (will be incremented before assigning)
int gscr_last_timer_id = -1;

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
    // validate numeric input
    for (int i = 0; src[i] != '\0'; i++)
    {
        if (src[i] < '0' || src[i] > '9')
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

    // numeric value - recognized by first character
    if (str[0] >= '0' && str[0] <= '9')
    {
        // return success only when valid integer was supplied, otherwise carry on parsing
        if (tryStrToInt(rr.value, str))
            return rr;
    }

    int lastpos = 0;
    size_t i;
    bool subject = false;
    // "less or equal" sign in combination with strlen may be considered dangerous, but
    // we need to catch even the last terminating zero character
    for (i = 0; i <= strlen(str); i++)
    {
        // dot character marks end of subject part, as well, as zero terminator when no subject specified
        if ((str[i] == '.' || str[i] == '\0') && !subject)
        {
            // decide about specifier target type
            std::string subid = std::string(str).substr(0, i);
            if (subid == "me")
                rr.subject_type = GSST_ME;
            else if (subid == "target")
                rr.subject_type = GSST_TARGET;
            else if (subid == "random")
                rr.subject_type = GSST_RANDOM;
            else if (subid == "implicit")
                rr.subject_type = GSST_IMPLICIT;
            else if (subid == "chance")
                rr.subject_type = GSST_CHANCE;
            else if (subid == "timer")
                rr.subject_type = GSST_TIMER;
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

            subject = true;
            lastpos = i;
        }
        // otherwise zero terminator signs that we jusr parsed the rest of the string, which is parameter
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

    switch (ret->type)
    {
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
            if (!tryStrToInt(ret->params.c_wait.delay, src->parameters[0].c_str()))
                CLEANUP_AND_THROW("invalid parameter 1 (delay) for WAIT");

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
            if (!tryStrToInt(ret->params.c_cast.spell, src->parameters[0].c_str()))
                CLEANUP_AND_THROW("invalid parameter 1 (spellId) for CAST");

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
        case GSCR_SAY:
        case GSCR_YELL:
            if (src->parameters.size() != 1)
                CLEANUP_AND_THROW("too few parameters for instruction SAY/YELL");

            // store string to be said / yelled
            ret->params.c_say_yell.tosay = src->parameters[0].c_str();

            break;
        // if instruction - standard control sequence; needs to be closed by endif
        // Syntax: if <specifier> [<operator> <specifier>]
        case GSCR_IF:
            if (src->parameters.size() == 0)
                CLEANUP_AND_THROW("too few parameters for instruction IF");

            // parse source specifier
            ret->params.c_if.source = gs_specifier::parse(src->parameters[0].c_str());
            // if the instruction has more parameters than one..
            if (src->parameters.size() > 1)
            {
                // .. it has to be 3, otherwise it's considered error
                if (src->parameters.size() == 3)
                {
                    ret->params.c_if.op = gs_parse_operator(src->parameters[1]);
                    ret->params.c_if.dest = gs_specifier::parse(src->parameters[2].c_str());
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
                ret->params.c_faction.faction = 0;
            // or set faction
            else if (!tryStrToInt(ret->params.c_faction.faction, src->parameters[0].c_str()))
                CLEANUP_AND_THROW("invalid non-numeric faction specifier for command FACTION");
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

            if (!tryStrToInt(ret->params.c_timer.value, src->parameters[2].c_str()))
                CLEANUP_AND_THROW("invalid time value parameter for instruction TIMER");

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
                ret->params.c_morph.morph_id = 0;
            // or set morph
            else if (!tryStrToInt(ret->params.c_morph.morph_id, src->parameters[0].c_str()))
                CLEANUP_AND_THROW("invalid non-numeric faction specifier for command MORPH");
            break;
        // summon instruction - summons NPC at position
        // Syntax: summon <entry>
        //         summon <entry> <x> <y> <z>
        case GSCR_SUMMON:
            if (src->parameters.size() == 0)
                CLEANUP_AND_THROW("too few parameters for instruction SUMMON");
            if (src->parameters.size() != 1 && src->parameters.size() != 4)
                CLEANUP_AND_THROW("invalid parameter count for instruction SUMMON - use 1 or 4 params");

            if (!tryStrToInt(ret->params.c_summon.creature_entry, src->parameters[0].c_str()))
                CLEANUP_AND_THROW("invalid creature entry identifier for instruction SUMMON");

            ret->params.c_summon.x = 0;
            ret->params.c_summon.y = 0;
            ret->params.c_summon.z = 0;
            if (src->parameters.size() == 4)
            {
                if (!tryStrToFloat(ret->params.c_summon.x, src->parameters[1].c_str()))
                    CLEANUP_AND_THROW("invalid X position specifier for instruction SUMMON");
                if (!tryStrToFloat(ret->params.c_summon.y, src->parameters[2].c_str()))
                    CLEANUP_AND_THROW("invalid Y position specifier for instruction SUMMON");
                if (!tryStrToFloat(ret->params.c_summon.z, src->parameters[3].c_str()))
                    CLEANUP_AND_THROW("invalid Z position specifier for instruction SUMMON");
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
                CLEANUP_AND_THROW("invalid parameter count for instruction SUMMON - use 3 params");

            if (!tryStrToFloat(ret->params.c_walk_run_teleport.x, src->parameters[0].c_str()))
                CLEANUP_AND_THROW("invalid X position specifier for instruction WALK/RUN/TELEPORT");
            if (!tryStrToFloat(ret->params.c_walk_run_teleport.y, src->parameters[1].c_str()))
                CLEANUP_AND_THROW("invalid Y position specifier for instruction WALK/RUN/TELEPORT");
            if (!tryStrToFloat(ret->params.c_walk_run_teleport.z, src->parameters[2].c_str()))
                CLEANUP_AND_THROW("invalid Z position specifier for instruction SUMMON");
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
    gscr_last_timer_id = -1;

    while (!gscr_if_stack.empty())
        gscr_if_stack.pop();

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
