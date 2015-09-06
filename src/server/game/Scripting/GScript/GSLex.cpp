#include "gamePCH.h"
#include "GSCommands.h"

// cuts supplied character sequence and retrieves wrapped substring as std::string
std::string getStringFromRange(char* str, int begin, int end)
{
    return std::string(str).substr(begin, end - begin);
}

void gs_command_proto::addParameter(std::string par)
{
    if (par.size() >= 2 && par.at(0) == '"' && par.at(par.size() - 1) == '"')
        parameters.push_back(par.substr(1, par.size() - 2));
    else
        parameters.push_back(par);
}

CommandProtoVector* gscr_parseInput(std::vector<std::string>& lines)
{
    CommandProtoVector* commands = new CommandProtoVector;
    gs_command_proto* tmp;
    char* line;
    int i, lastpos;
    bool instr, endbybreak, inquotes;

    // parser loop, limit line size to 1024 characters
    for (size_t linepos = 0; linepos < lines.size(); ++linepos)
    {
        line = (char*)lines[linepos].c_str();

        tmp = new gs_command_proto;
        lastpos = 0;
        instr = false;
        endbybreak = false;
        inquotes = false;

        // go char by char
        for (i = 0; i < 1024; i++)
        {
            // quotes encloses parameter - works just as switch between parameter and line context
            if (line[i] == '"')
            {
                inquotes = !inquotes;
            }
            // space/tab as delimiter
            else if ((line[i] == ' ' || line[i] == '\t') && !inquotes)
            {
                // in case of multiple consequent spaces/tabs
                if (lastpos == i)
                {
                    lastpos++;
                    continue;
                }

                // read instruction if not read yet
                if (!instr)
                {
                    tmp->instruction = getStringFromRange(line, lastpos, i);
                    instr = true;
                }
                else // otherwise it's parameter
                    tmp->addParameter(getStringFromRange(line, lastpos, i));

                lastpos = i + 1;
            }
            // break command by specified delimiter - # for comment, and any other line ending character
            else if (line[i] == '#' || line[i] == '\0' || line[i] == '\n' || line[i] == '\r')
            {
                endbybreak = true;
                break;
            }
        }

        // if the last character wasn't space/tab, and we ended by regular line break, parse
        // additional parameter from the rest of string
        if (endbybreak && lastpos != i - 1 && lastpos != i)
        {
            if (instr)
                tmp->addParameter(getStringFromRange(line, lastpos, i));
            else
            {
                tmp->instruction = getStringFromRange(line, lastpos, i);
                instr = true;
            }
        }

        // if no instruction has been read (empty line, line with just comment, etc.), delete lexeme
        if (tmp && !instr)
        {
            delete tmp;
            tmp = nullptr;
        }

        // for any other valid commands, push back lexeme structure
        if (tmp)
            commands->push_back(tmp);
    }

    return commands;
}
