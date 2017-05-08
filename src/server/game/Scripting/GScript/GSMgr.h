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
    GS_TYPE_PLAYER_IN_RANGE = 8,
};

enum GScriptFlags
{
    GSFLAG_COOLDOWN_PER_PLAYER  = 0x00000001,   // cooldown is not global, but per player
    GSFLAG_SHARED_COOLDOWN      = 0x00000002,   // script is run for group of players that "would start it" only once within cooldown period
    GSFLAG_QUEUE_PERSISTENT     = 0x00000004,   // if the player no longer fits the conditions to start the script, but is in queue for starting it, leave him here
};

enum GSAISignals
{
    GSAI_SIGNAL_UNHOOK = 1,
    GSAI_SIGNAL_REHOOK = 2,
    GSAI_SIGNAL_INVOKER_FROM_CREATOR = 3,
    GSAI_SIGNAL_INVOKER_FROM_OWNER = 4,
    GSAI_SIGNAL_INVOKER_FROM_SUMMONER = 5,
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
        CommandContainer* GetScript(int id);

        void AddError(int scriptId, std::string err);
        std::list<std::string>* GetErrorList();

    private:
        std::map<int, CommandContainer*> m_loadedScripts;
        std::set<ScriptedAI*> m_registeredAIs;
        std::list<std::string> m_errorList;
};

#define sGSMgr ACE_Singleton<GSMgr, ACE_Null_Mutex>::instance()

#endif
