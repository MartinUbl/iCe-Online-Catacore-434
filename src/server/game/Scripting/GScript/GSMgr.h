#ifndef SC_GSMGR_H
#define SC_GSMGR_H

#include <ace/Singleton.h>

#include "GSCommands.h"
#include "ScriptedCreature.h"

enum GScriptType
{
    GS_TYPE_NONE = 0,
    GS_TYPE_COMBAT = 1,
    GS_TYPE_OUT_OF_COMBAT = 2,
    GS_TYPE_GOSSIP_SELECT = 3,
    GS_TYPE_QUEST_ACCEPT = 4,
    GS_TYPE_QUEST_COMPLETE = 5,
    GS_TYPE_VEHICLE_ENTER = 6,
    GS_TYPE_SPELL_RECEIVED = 7,
};

enum GSAISignals
{
    GSAI_SIGNAL_UNHOOK = 1,
    GSAI_SIGNAL_REHOOK = 2,
    GSAI_SIGNAL_INVOKER_FROM_CREATOR = 3,
    GSAI_SIGNAL_INVOKER_FROM_OWNER = 4,
    GSAI_SIGNAL_INVOKER_FROM_SUMMONER = 4,
};

class GSMgr
{
    friend class ACE_Singleton<GSMgr, ACE_Null_Mutex>;

    GSMgr();
    ~GSMgr();

    public:
        void LoadScripts();
        void RegisterAI(ScriptedAI* src);
        void UnregisterAI(ScriptedAI* src);
        CommandVector* GetScript(int id);

    private:
        std::map<int, CommandVector*> m_loadedScripts;
        std::set<ScriptedAI*> m_registeredAIs;
};

#define sGSMgr ACE_Singleton<GSMgr, ACE_Null_Mutex>::instance()

#endif
