#include "gamePCH.h"
#include "ScriptedCreature.h"
#include "GSCommands.h"
#include "GSMgr.h"

GSMgr::GSMgr()
{
    //
}

GSMgr::~GSMgr()
{
    //
}

void GSMgr::LoadScripts()
{
    // select everything for this creature from database
    QueryResult res = ScriptDatabase.PQuery("SELECT id, script FROM gscript_template");
    uint32 cnt = 0;

    for (auto itr = m_registeredAIs.begin(); itr != m_registeredAIs.end(); ++itr)
    {
        // unhook scripts, so we won't cause conflict or memory corruption
        (*itr)->DoAction(GSAI_SIGNAL_UNHOOK);
    }

    // retrieve string to be parsed
    std::vector<std::string> toparse;
    if (res)
    {
        do
        {
            Field* f = res->Fetch();
            if (f)
            {
                // cleanup existing script
                /*if (m_loadedScripts.find(f[0].GetInt32()) != m_loadedScripts.end())
                {
                    for (auto itr = m_loadedScripts[f[0].GetInt32()]->begin(); itr != m_loadedScripts[f[0].GetInt32()]->end(); ++itr)
                        delete *itr;
                    delete m_loadedScripts[f[0].GetInt32()];
                }*/

                // cut the input string by lines

                std::stringstream ss(f[1].GetCString());
                std::string to;

                while (std::getline(ss, to, '\n'))
                    toparse.push_back(std::string(to));

                // if there was something at input...
                if (toparse.size() > 0)
                {
                    // ...parse script into tokens...
                    CommandProtoVector* cpv = gscr_parseInput(toparse);
                    // ...and if succeeded, analyze command sequence and build command vector
                    if (cpv)
                    {
                        m_loadedScripts[f[0].GetInt32()] = gscr_analyseSequence(cpv, f[0].GetInt32());
                        cnt++;
                    }
                }

                toparse.clear();
            }
        } while (res->NextRow());
    }

    for (auto itr = m_registeredAIs.begin(); itr != m_registeredAIs.end(); ++itr)
    {
        // rehook scripts
        (*itr)->DoAction(GSAI_SIGNAL_REHOOK);
    }

    sLog->outString(">> Loaded %u G-Script scripts.", cnt);
    sLog->outString();
}

void GSMgr::RegisterAI(ScriptedAI* src)
{
    m_registeredAIs.insert(src);
}

void GSMgr::UnregisterAI(ScriptedAI* src)
{
    if (m_registeredAIs.find(src) != m_registeredAIs.end())
        m_registeredAIs.erase(src);
}

CommandVector* GSMgr::GetScript(int id)
{
    if (m_loadedScripts.find(id) == m_loadedScripts.end())
        return nullptr;

    return m_loadedScripts[id];
}
