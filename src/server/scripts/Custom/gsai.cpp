#include "ScriptPCH.h"
#include "GSCommands.h"
#include "GSMgr.h"
#include "CreatureTextMgr.h"

class GS_CreatureScript : public CreatureScript
{
    public:

        GS_CreatureScript() : CreatureScript("gscript")
        {
            //
        }

        struct GS_QuestScriptRecord
        {
            int acceptScriptId;
            int completeScriptId;
        };

        enum GS_VariableType
        {
            GSVTYPE_INTEGER = 0,
            GSVTYPE_FLOAT = 1,
            GSVTYPE_UNIT = 2
        };

        struct GS_Variable
        {
            union
            {
                int32 asInteger;
                float asFloat;
                Unit* asUnitPointer;
            } value;

            // performs conversion to integer, if available (float, int)
            int32 toInteger()
            {
                if (type == GSVTYPE_INTEGER)
                    return value.asInteger;
                if (type == GSVTYPE_FLOAT)
                    return (int32)value.asFloat;
                else
                    return 0;
            };

            // performs conversion to float, if available (float, int)
            float toFloat()
            {
                if (type == GSVTYPE_INTEGER)
                    return (float)value.asInteger;
                if (type == GSVTYPE_FLOAT)
                    return value.asFloat;
                else
                    return 0.0f;
            };

            GS_VariableType type;

            GS_Variable() { value.asInteger = 0; type = GSVTYPE_INTEGER; };
            GS_Variable(int32 nvalue) { value.asInteger = nvalue; type = GSVTYPE_INTEGER; };
            GS_Variable(float nvalue) { value.asFloat = nvalue; type = GSVTYPE_FLOAT; };
            GS_Variable(Unit* nvalue) { value.asUnitPointer = nvalue; type = GSVTYPE_UNIT; };

            bool operator==(GS_Variable &sec)
            {
                // since they are both same types, we may afford that
                if (type == sec.type)
                    return value.asInteger == sec.value.asInteger;
                return false;
            }
            bool operator!=(GS_Variable &sec)
            {
                // since they are both same types, we may afford that
                if (type == sec.type)
                    return value.asInteger != sec.value.asInteger;
                return true;
            }
            bool operator>(GS_Variable &sec)
            {
                // same type case
                if (type == GSVTYPE_INTEGER && sec.type == GSVTYPE_INTEGER)
                    return value.asInteger > sec.value.asInteger;
                if (type == GSVTYPE_FLOAT && sec.type == GSVTYPE_FLOAT)
                    return value.asFloat > sec.value.asFloat;
                // different type case
                if (type == GSVTYPE_INTEGER && sec.type == GSVTYPE_FLOAT)
                    return (float)(value.asInteger) > sec.value.asFloat;
                if (type == GSVTYPE_FLOAT && sec.type == GSVTYPE_INTEGER)
                    return value.asFloat > (float)(sec.value.asInteger);

                // we can compare only numeric values - any other is considered false
                return false;
            }
            bool operator>=(GS_Variable &sec)
            {
                // same type case
                if (type == GSVTYPE_INTEGER && sec.type == GSVTYPE_INTEGER)
                    return value.asInteger >= sec.value.asInteger;
                if (type == GSVTYPE_FLOAT && sec.type == GSVTYPE_FLOAT)
                    return value.asFloat >= sec.value.asFloat;
                // different type case
                if (type == GSVTYPE_INTEGER && sec.type == GSVTYPE_FLOAT)
                    return (float)(value.asInteger) >= sec.value.asFloat;
                if (type == GSVTYPE_FLOAT && sec.type == GSVTYPE_INTEGER)
                    return value.asFloat >= (float)(sec.value.asInteger);

                // we can compare only numeric values - any other is considered false
                return false;
            }
            bool operator<(GS_Variable &sec)
            {
                // same type case
                if (type == GSVTYPE_INTEGER && sec.type == GSVTYPE_INTEGER)
                    return value.asInteger < sec.value.asInteger;
                if (type == GSVTYPE_FLOAT && sec.type == GSVTYPE_FLOAT)
                    return value.asFloat < sec.value.asFloat;
                // different type case
                if (type == GSVTYPE_INTEGER && sec.type == GSVTYPE_FLOAT)
                    return (float)(value.asInteger) < sec.value.asFloat;
                if (type == GSVTYPE_FLOAT && sec.type == GSVTYPE_INTEGER)
                    return value.asFloat < (float)(sec.value.asInteger);

                // we can compare only numeric values - any other is considered false
                return false;
            }
            bool operator<=(GS_Variable &sec)
            {
                // same type case
                if (type == GSVTYPE_INTEGER && sec.type == GSVTYPE_INTEGER)
                    return value.asInteger <= sec.value.asInteger;
                if (type == GSVTYPE_FLOAT && sec.type == GSVTYPE_FLOAT)
                    return value.asFloat <= sec.value.asFloat;
                // different type case
                if (type == GSVTYPE_INTEGER && sec.type == GSVTYPE_FLOAT)
                    return (float)(value.asInteger) <= sec.value.asFloat;
                if (type == GSVTYPE_FLOAT && sec.type == GSVTYPE_INTEGER)
                    return value.asFloat <= (float)(sec.value.asInteger);

                // we can compare only numeric values - any other is considered false
                return false;
            }
            GS_Variable& operator+=(const GS_Variable &sec)
            {
                if (type == GSVTYPE_INTEGER && sec.type == GSVTYPE_INTEGER)
                    value.asInteger += sec.value.asInteger;
                else if (type == GSVTYPE_FLOAT && sec.type == GSVTYPE_FLOAT)
                    value.asFloat += sec.value.asFloat;
                else if (type == GSVTYPE_FLOAT && sec.type == GSVTYPE_INTEGER)
                    value.asFloat += sec.value.asInteger;
                else if (type == GSVTYPE_INTEGER && sec.type == GSVTYPE_FLOAT)
                {
                    value.asFloat = ((float)value.asInteger) + sec.value.asFloat;
                    type = GSVTYPE_FLOAT;
                }
                else
                    sLog->outError("GSAI: compound operator += tried to work with non-numeric values");

                return *this;
            }
            GS_Variable& operator-=(const GS_Variable &sec)
            {
                if (type == GSVTYPE_INTEGER && sec.type == GSVTYPE_INTEGER)
                    value.asInteger -= sec.value.asInteger;
                else if (type == GSVTYPE_FLOAT && sec.type == GSVTYPE_FLOAT)
                    value.asFloat -= sec.value.asFloat;
                else if (type == GSVTYPE_FLOAT && sec.type == GSVTYPE_INTEGER)
                    value.asFloat -= sec.value.asInteger;
                else if (type == GSVTYPE_INTEGER && sec.type == GSVTYPE_FLOAT)
                {
                    value.asFloat = ((float)value.asInteger) - sec.value.asFloat;
                    type = GSVTYPE_FLOAT;
                }
                else
                    sLog->outError("GSAI: compound operator -= tried to work with non-numeric values");

                return *this;
            }
            GS_Variable& operator*=(const GS_Variable &sec)
            {
                if (type == GSVTYPE_INTEGER && sec.type == GSVTYPE_INTEGER)
                    value.asInteger *= sec.value.asInteger;
                else if (type == GSVTYPE_FLOAT && sec.type == GSVTYPE_FLOAT)
                    value.asFloat *= sec.value.asFloat;
                else if (type == GSVTYPE_FLOAT && sec.type == GSVTYPE_INTEGER)
                    value.asFloat *= sec.value.asInteger;
                else if (type == GSVTYPE_INTEGER && sec.type == GSVTYPE_FLOAT)
                {
                    value.asFloat = ((float)value.asInteger) * sec.value.asFloat;
                    type = GSVTYPE_FLOAT;
                }
                else
                    sLog->outError("GSAI: compound operator *= tried to work with non-numeric values");

                return *this;
            }
            GS_Variable& operator/=(const GS_Variable &sec)
            {
                // this may not be exactly what we would expect, but this performs float division,
                // when at least one parameter is of floating type. Otherwise performs integer
                // division

                if (type == GSVTYPE_INTEGER && sec.type == GSVTYPE_INTEGER)
                    value.asInteger /= sec.value.asInteger;
                else if (type == GSVTYPE_FLOAT && sec.type == GSVTYPE_FLOAT)
                    value.asFloat /= sec.value.asFloat;
                else if (type == GSVTYPE_FLOAT && sec.type == GSVTYPE_INTEGER)
                    value.asFloat /= (float)sec.value.asInteger;
                else if (type == GSVTYPE_INTEGER && sec.type == GSVTYPE_FLOAT)
                {
                    value.asFloat = ((float)value.asInteger) / sec.value.asFloat;
                    type = GSVTYPE_FLOAT;
                }
                else
                    sLog->outError("GSAI: compound operator /= tried to work with non-numeric values");

                return *this;
            }
            // result of modulo compount assignment is always integer
            GS_Variable& operator%=(const GS_Variable &sec)
            {
                if (type == GSVTYPE_INTEGER && sec.type == GSVTYPE_INTEGER)
                    value.asInteger %= sec.value.asInteger;
                else if (type == GSVTYPE_FLOAT && sec.type == GSVTYPE_FLOAT)
                {
                    value.asInteger = ((int32)value.asFloat) % ((int32)sec.value.asFloat);
                    type = GSVTYPE_INTEGER;
                }
                else if (type == GSVTYPE_FLOAT && sec.type == GSVTYPE_INTEGER)
                {
                    value.asInteger = ((int32)value.asFloat) % sec.value.asInteger;
                    type = GSVTYPE_INTEGER;
                }
                else if (type == GSVTYPE_INTEGER && sec.type == GSVTYPE_FLOAT)
                    value.asInteger = value.asInteger % ((int32)sec.value.asFloat);
                else
                    sLog->outError("GSAI: compound operator %%= tried to work with non-numeric values");

                return *this;
            }
        };

        struct GS_ScriptedAI : public ScriptedAI
        {
            // vector of commands to be executed
            CommandVector* my_commands;
            // command counter (similar to program counter register); points to specific element in my_commands
            uint32 com_counter;
            // flag for waiting (not moving to next instruction)
            bool is_waiting;
            // if waiting, this timer will then stop waiting loop
            uint32 wait_timer;
            // waiting flags
            uint32 wait_flags;

            // script ID when in combat
            int m_combatScriptId = -1;
            // script ID when not in combat
            int m_outOfCombatScriptId = -1;
            // script IDs for gossip options
            int m_gossipSelectScripts[10];
            // script ID when vehicle entered
            int m_vehicleEnterScriptId = -1;
            // map of script IDs for quests
            std::map<uint32, GS_QuestScriptRecord> m_questScripts;
            // map of script IDs for spellcasts received
            std::map<int32, int32> m_spellReceivedScripts;

            // current type of script being executed
            GScriptType m_currentScriptType = GS_TYPE_NONE;

            // flag for disabling melee combat
            bool disable_melee;

            // state flag for movement
            bool is_moving;
            // state flag for locked script change
            bool is_script_locked;

            // lock for update
            bool is_updating_lock;

            // stored faction in case of faction change
            uint32 stored_faction = 0;
            // stored model id in case of morph
            uint32 stored_modelid = 0;
            // stored scale value
            float stored_scale = -1.0f;
            // stored react state
            uint8 stored_react = 99;

            // map of all timers
            std::map<int, int32> timer_map;
            // map of all variables
            std::map<int, GS_Variable> variable_map;
            // set of all passed WHEN's offsets in script
            std::set<int> when_set;

            Unit* m_scriptInvoker = nullptr;
            Unit* m_inheritedScriptInvoker = nullptr;

            Unit* m_parentUnit = nullptr;
            Unit* m_lastSummonedUnit = nullptr;

            GS_ScriptedAI(Creature* cr) : ScriptedAI(cr)
            {
                my_commands = nullptr;
                for (int i = 0; i < 10; i++)
                    m_gossipSelectScripts[i] = -1;

                sGSMgr->RegisterAI(this);

                GS_LoadMyScript();

                // ordinary reset
                com_counter = 0;
                is_waiting = false;
                disable_melee = false;
                is_moving = false;
                is_updating_lock = false;
                is_script_locked = false;
                m_lastSummonedUnit = nullptr;
            }

            ~GS_ScriptedAI()
            {
                sGSMgr->UnregisterAI(this);
            }

            void DoAction(const int32 action) override
            {
                if (action == GSAI_SIGNAL_UNHOOK)
                {
                    // while any other thread is inside UpdateAI, wait to leave
                    while (is_updating_lock)
                        ;
                    my_commands = nullptr;
                    com_counter = 0;
                }
                else if (action == GSAI_SIGNAL_REHOOK)
                {
                    GS_LoadMyScript();
                    ResetState();

                    if (m_currentScriptType == GS_TYPE_COMBAT && m_combatScriptId >= 0)
                        my_commands = sGSMgr->GetScript(m_combatScriptId);
                    else if (m_currentScriptType == GS_TYPE_OUT_OF_COMBAT && m_outOfCombatScriptId >= 0)
                        my_commands = sGSMgr->GetScript(m_outOfCombatScriptId);
                }
                else if (action == GSAI_SIGNAL_INVOKER_FROM_CREATOR)
                {
                    Unit* creator = sObjectAccessor->GetUnit(*me, me->GetCreatorGUID());
                    if (creator)
                        m_scriptInvoker = creator;
                }
                else if (action == GSAI_SIGNAL_INVOKER_FROM_OWNER)
                {
                    Unit* creator = sObjectAccessor->GetUnit(*me, me->GetOwnerGUID());
                    if (creator)
                        m_scriptInvoker = creator;
                }
                else if (action == GSAI_SIGNAL_INVOKER_FROM_SUMMONER)
                {
                    if (me->ToTempSummon())
                        m_scriptInvoker = me->ToTempSummon()->GetSummoner();
                }
                else
                    ScriptedAI::DoAction(action);
            }

            void sGossipSelect(Player* player, uint32 sender, uint32 action)
            {
                player->CLOSE_GOSSIP_MENU();
                if (m_currentScriptType != GS_TYPE_COMBAT)
                {
                    if (m_gossipSelectScripts[action] >= 0)
                    {
                        ResetState();
                        m_scriptInvoker = player;
                        my_commands = sGSMgr->GetScript(m_gossipSelectScripts[action]);
                        m_currentScriptType = GS_TYPE_GOSSIP_SELECT;
                    }
                }
            }

            void sQuestAccept(Player* player, Quest const* quest)
            {
                if (m_currentScriptType != GS_TYPE_COMBAT)
                {
                    uint32 questId = quest->GetQuestId();
                    if (m_questScripts.find(questId) != m_questScripts.end() && m_questScripts[questId].acceptScriptId >= 0)
                    {
                        ResetState();
                        m_scriptInvoker = player;
                        my_commands = sGSMgr->GetScript(m_questScripts[questId].acceptScriptId);
                        m_currentScriptType = GS_TYPE_QUEST_ACCEPT;
                    }
                }
            }

            void sQuestComplete(Player* player, Quest const* quest)
            {
                if (m_currentScriptType != GS_TYPE_COMBAT)
                {
                    uint32 questId = quest->GetQuestId();
                    if (m_questScripts.find(questId) != m_questScripts.end() && m_questScripts[questId].completeScriptId >= 0)
                    {
                        ResetState();
                        m_scriptInvoker = player;
                        my_commands = sGSMgr->GetScript(m_questScripts[questId].completeScriptId);
                        m_currentScriptType = GS_TYPE_QUEST_COMPLETE;
                    }
                }
            }

            void PassengerBoarded(Unit* who, int8 seatId, bool apply) override
            {
                if (m_currentScriptType != GS_TYPE_COMBAT && m_currentScriptType != GS_TYPE_VEHICLE_ENTER && apply)
                {
                    if (m_vehicleEnterScriptId >= 0)
                    {
                        ResetState();
                        m_scriptInvoker = who;
                        my_commands = sGSMgr->GetScript(m_vehicleEnterScriptId);
                        m_currentScriptType = GS_TYPE_VEHICLE_ENTER;
                    }
                }

                // if we are doing vehicle entered script, and passenger is leaving, reset state if it's last passenger
                if (m_currentScriptType == GS_TYPE_VEHICLE_ENTER && !apply)
                {
                    if (!me->GetVehicleKit()->HasPassengers())
                        EnterEvadeMode();
                }
            }

            void SpellHit(Unit* who, const SpellEntry* spell) override
            {
                if (m_spellReceivedScripts.find(spell->Id) != m_spellReceivedScripts.end())
                {
                    if (m_currentScriptType != GS_TYPE_COMBAT)
                    {
                        if (m_spellReceivedScripts[spell->Id] >= 0)
                        {
                            ResetState();
                            m_scriptInvoker = who;
                            my_commands = sGSMgr->GetScript(m_spellReceivedScripts[spell->Id]);
                            m_currentScriptType = GS_TYPE_SPELL_RECEIVED;
                        }
                    }
                }
            }

            void SetInheritedInvoker(Unit* pl)
            {
                m_inheritedScriptInvoker = pl;
            }

            void SetParentUnit(Unit* parent)
            {
                m_parentUnit = parent;
            }

            void GS_LoadMyScript()
            {
                m_spellReceivedScripts.clear();

                // select everything for this creature from database
                QueryResult res = ScriptDatabase.PQuery("SELECT script_type, script_type_param, script_id FROM creature_gscript WHERE creature_entry = %u", me->GetEntry());

                if (res)
                {
                    Field* f;
                    do
                    {
                        f = res->Fetch();

                        uint16 type = f[0].GetUInt16();
                        int32 scriptId = f[2].GetInt32();
                        int32 scriptParam = f[1].GetInt32();

                        switch (type)
                        {
                            case GS_TYPE_COMBAT:
                                m_combatScriptId = scriptId;
                                break;
                            case GS_TYPE_OUT_OF_COMBAT:
                                m_outOfCombatScriptId = scriptId;
                                break;
                            case GS_TYPE_GOSSIP_SELECT:
                                if (scriptParam >= 0 && scriptParam <= 9)
                                    m_gossipSelectScripts[scriptParam] = scriptId;
                                break;
                            case GS_TYPE_QUEST_ACCEPT:
                                if (m_questScripts.find(scriptParam) == m_questScripts.end())
                                    m_questScripts[scriptParam] = GS_QuestScriptRecord();
                                m_questScripts[scriptParam].acceptScriptId = scriptId;
                                break;
                            case GS_TYPE_QUEST_COMPLETE:
                                if (m_questScripts.find(scriptParam) == m_questScripts.end())
                                    m_questScripts[scriptParam] = GS_QuestScriptRecord();
                                m_questScripts[scriptParam].completeScriptId = scriptId;
                                break;
                            case GS_TYPE_VEHICLE_ENTER:
                                m_vehicleEnterScriptId = scriptId;
                                break;
                            case GS_TYPE_SPELL_RECEIVED:
                                m_spellReceivedScripts[scriptParam] = scriptId;
                            default:
                            case GS_TYPE_NONE:
                                break;
                        }

                    } while (res->NextRow());
                }
            }

            void ResetState()
            {
                // reset command counter, waiting and melee disable flags
                com_counter = 0;
                is_waiting = false;
                disable_melee = false;
                is_moving = false;
                m_scriptInvoker = nullptr;
                m_lastSummonedUnit = nullptr;

                timer_map.clear();
                variable_map.clear();
                when_set.clear();

                if (stored_faction)
                {
                    me->setFaction(stored_faction);
                    stored_faction = 0;
                }
                if (stored_modelid)
                {
                    me->DeMorph();
                    stored_modelid = 0;
                }
                if (stored_scale > 0.0f)
                {
                    me->SetFloatValue(OBJECT_FIELD_SCALE_X, stored_scale);
                    stored_scale = -1.0f;
                }
                if (stored_react < 99)
                {
                    me->SetReactState((ReactStates)stored_react);
                    stored_react = 99;
                }

                me->ClearUnitState(UNIT_STATE_CANNOT_TURN);

                me->Unmount();
                me->LoadCreaturesAddon(true);

                me->UpdateEntry(me->GetEntry());

                me->GetMotionMaster()->MovementExpired(true);
                me->GetMotionMaster()->MoveTargetedHome();
            }

            void Reset() override
            {
                ResetState();

                if (!is_script_locked && m_currentScriptType != GS_TYPE_OUT_OF_COMBAT)
                {
                    if (m_outOfCombatScriptId >= 0)
                    {
                        my_commands = sGSMgr->GetScript(m_outOfCombatScriptId);
                        m_currentScriptType = GS_TYPE_OUT_OF_COMBAT;
                    }
                    else
                    {
                        my_commands = nullptr;
                        m_currentScriptType = GS_TYPE_NONE;
                    }
                }

                ScriptedAI::Reset();
            }

            void EnterEvadeMode() override
            {
                if (!is_script_locked)
                {
                    ResetState();

                    my_commands = nullptr;
                    m_currentScriptType = GS_TYPE_NONE;
                    if (m_outOfCombatScriptId >= 0)
                    {
                        my_commands = sGSMgr->GetScript(m_outOfCombatScriptId);
                        m_currentScriptType = GS_TYPE_OUT_OF_COMBAT;
                    }
                }

                ScriptedAI::EnterEvadeMode();
            }

            void EnterCombat(Unit* who) override
            {
                com_counter = 0;
                is_waiting = false;
                disable_melee = false;
                is_moving = false;
                if (!is_script_locked)
                {
                    my_commands = nullptr;
                    m_currentScriptType = GS_TYPE_NONE;
                    if (m_combatScriptId >= 0)
                    {
                        my_commands = sGSMgr->GetScript(m_combatScriptId);
                        m_currentScriptType = GS_TYPE_COMBAT;
                    }
                }

                ScriptedAI::EnterCombat(who);
            }

            void GS_SetTimer(int timer, uint32 time)
            {
                timer_map[timer] = time;
            }

            int GS_GetTimerState(int timer)
            {
                if (timer_map.find(timer) == timer_map.end())
                    return GSSV_READY;
                if (timer_map[timer] == 0)
                    return GSSV_READY;

                if (timer_map[timer] < 0)
                    return GSSV_SUSPENDED;

                return GSSV_IN_PROGRESS;
            }

            void GS_UpdateTimers(const uint32 diff)
            {
                for (auto itr = timer_map.begin(); itr != timer_map.end(); ++itr)
                {
                    // passed or suspended - do not update
                    if ((*itr).second <= 0)
                        continue;
                    if ((*itr).second < (int)diff)
                        (*itr).second = 0;
                    else
                        (*itr).second -= diff;
                }
            }

            // select target using specifier supplied; does not work for one-values, just unit specifiers
            Unit* GS_SelectTarget(gs_specifier& spec)
            {
                switch (spec.subject_type)
                {
                    // me - select script owner
                    case GSST_ME:
                    // implicit target is chosen then by emulator itself during handling - "me" is safe value here
                    case GSST_IMPLICIT:
                        return me;
                    // current target
                    case GSST_TARGET:
                        return me->GetVictim();
                    // random target from threat list
                    case GSST_RANDOM:
                    {
                        // get iterator
                        std::list<HostileReference*>::iterator itr = me->getThreatManager().getThreatList().begin();
                        // and advance by random value
                        int32 rr = urand(0, me->getThreatManager().getThreatList().size() - 1);
                        while (rr > 0)
                            itr++;
                        return (*itr)->getTarget();
                    }
                    // random target from threat list excluding tank
                    case GSST_RANDOM_NOTANK:
                    {
                        if (me->getThreatManager().getThreatList().size() <= 1)
                            return me->GetVictim();

                        // get iterator
                        std::list<HostileReference*>::iterator itr = me->getThreatManager().getThreatList().begin();
                        // and advance by random value + 1
                        int32 rr = urand(1, me->getThreatManager().getThreatList().size() - 1);
                        while (rr > 0)
                            itr++;
                        return (*itr)->getTarget();
                    }
                    // script invoker (active during gossip select, quest accept, etc.)
                    case GSST_INVOKER:
                        return m_scriptInvoker ? m_scriptInvoker : m_inheritedScriptInvoker;
                    // parent (summoner)
                    case GSST_PARENT:
                        return m_parentUnit ? m_parentUnit : me;
                    case GSST_CLOSEST_CREATURE:
                        return GetClosestCreatureWithEntry(me, spec.value, 300.0f, true);
                    case GSST_CLOSEST_PLAYER:
                        return me->FindNearestPlayer(300.0f, true);
                    case GSST_LAST_SUMMON:
                        return m_lastSummonedUnit;
                    case GSST_CLOSEST_CREATURE_GUID:
                    {
                        const CreatureData* crd = sObjectMgr->GetCreatureData(spec.value);
                        if (crd)
                            return me->GetMap()->GetCreature(MAKE_NEW_GUID(spec.value, crd->id, HIGHGUID_UNIT));
                        return nullptr;
                    }
                    // may be also variable value, if the variable points to unit
                    case GSST_VARIABLE_VALUE:
                        if (variable_map.find(spec.value) != variable_map.end())
                        {
                            GS_Variable var = variable_map[spec.value];
                            if (var.type == GSVTYPE_UNIT)
                                return var.value.asUnitPointer;
                        }
                        return nullptr;
                    // other non-handled cases - returns null as it's invalid in this context
                    default:
                    case GSST_STATE:
                    case GSST_CHANCE:
                    case GSST_NONE:
                        break;
                }

                return nullptr;
            }

            // retrieves numeric value using specifier supplied
            GS_Variable GS_GetValueFromSpecifier(gs_specifier& spec)
            {
                // variable has priority before anything else, due to its genericity
                if (spec.subject_type == GSST_VARIABLE_VALUE)
                {
                    if (variable_map.find(spec.value) == variable_map.end())
                        variable_map[spec.value] = GS_Variable((int32)0);

                    return variable_map[spec.value];
                }

                // if type and parameter were not specified, return one-value
                if (spec.subject_type == GSST_NONE)
                    return GS_Variable(spec.isFloat ? (float)spec.flValue : (int32)spec.value);

                if (spec.subject_type == GSST_TIMER && spec.subject_parameter == GSSP_IDENTIFIER)
                    return GS_Variable(GS_GetTimerState(spec.value));

                if (spec.subject_type == GSST_STATE && spec.subject_parameter == GSSP_STATE_VALUE)
                    return GS_Variable((int32)spec.value);

                // there are only two meaningful subject types: me and target,
                // any other target is not so valuable here

                Unit* subject = GS_SelectTarget(spec);
                // fallback to "me" if the target has not been found
                if (!subject)
                    subject = me;

                switch (spec.subject_parameter)
                {
                    case GSSP_HEALTH:
                        return GS_Variable((int32)subject->GetHealth());
                    case GSSP_HEALTH_PCT:
                        return GS_Variable(subject->GetHealthPct());
                    case GSSP_FACTION:
                        return GS_Variable((int32)subject->getFaction());
                    case GSSP_LEVEL:
                        return GS_Variable((int32)subject->getLevel());
                    case GSSP_COMBAT:
                        return GS_Variable((int32)(subject->IsInCombat() ? 1 : 0));
                    case GSSP_MANA:
                        return GS_Variable((int32)subject->GetPower(POWER_MANA));
                    case GSSP_MANA_PCT:
                        return GS_Variable((subject->GetMaxPower(POWER_MANA) > 0) ? (100.0f* (float)subject->GetPower(POWER_MANA) / (float)subject->GetMaxPower(POWER_MANA)) : 0);
                    case GSSP_DISTANCE:
                        return GS_Variable(me->GetDistance(subject));
                    case GSSP_POS_X:
                        return GS_Variable(subject->GetPositionX());
                    case GSSP_POS_Y:
                        return GS_Variable(subject->GetPositionY());
                    case GSSP_POS_Z:
                        return GS_Variable(subject->GetPositionZ());
                    case GSSP_ORIENTATION:
                        return GS_Variable(subject->GetOrientation());
                    case GSSP_ALIVE:
                        return GS_Variable((int32)(subject->IsAlive() ? 1 : 0));
                    case GSSP_NONE:
                        return GS_Variable(subject);
                    default:
                        break;
                }

                // fallback to value if no match
                return GS_Variable(spec.isFloat ? spec.flValue : (int32)spec.value);
            }

            bool GS_Meets(gs_condition &cond)
            {
                // chance is different, non-generic case
                if (cond.source.subject_type == GSST_CHANCE)
                {
                    if (cond.op == GSOP_OF && cond.dest.value > 0)
                        return urand(0, 100) < (uint32)cond.dest.value;
                    else
                        return false;
                }

                // from now, we support just 3 parameter condition (lefthand, operator, righthand)
                // so get those two values, and then apply operator
                GS_Variable lefthand = GS_GetValueFromSpecifier(cond.source);
                GS_Variable righthand = GS_GetValueFromSpecifier(cond.dest);

                switch (cond.op)
                {
                    case GSOP_EQUALS:
                    case GSOP_IS:
                        return lefthand == righthand;
                    case GSOP_NOT_EQUAL:
                        return lefthand != righthand;
                    case GSOP_LESS_THAN:
                        return lefthand < righthand;
                    case GSOP_LESS_OR_EQUAL:
                        return lefthand <= righthand;
                    case GSOP_GREATER_THAN:
                        return lefthand > righthand;
                    case GSOP_GREATER_OR_EQUAL:
                        return lefthand >= righthand;
                    default:
                    case GSOP_NONE:
                    case GSOP_OF:
                        break;
                }

                return false;
            }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (id == 100)
                    is_moving = false;
            }

            void JustSummoned(Creature* summoned) override
            {
                // only if script ID matches (if we have this AI, and it has its ID, then if the
                // summoned creature has the same ID, that means, it has the same AI as well)
                if (summoned->GetScriptId() == me->GetScriptId())
                {
                    GS_ScriptedAI* targetAI = ((GS_ScriptedAI*)summoned->AI());
                    // set summoned creature invoker same like ours invoker
                    if (m_scriptInvoker)
                        targetAI->SetInheritedInvoker(m_scriptInvoker);

                    // set me as parent of summoned unit
                    targetAI->SetParentUnit(me);
                }

                m_lastSummonedUnit = summoned;
            }

            void SummonedCreatureDespawn(Creature* summoned) override
            {
                if (m_lastSummonedUnit == summoned)
                    m_lastSummonedUnit = nullptr;

                // go through every variable, and check, if it's unit pointer, and if it points to our summoned creature
                // if yes, purge the pointer to avoid having dangling pointers in variables
                for (auto itr = variable_map.begin(); itr != variable_map.end(); ++itr)
                {
                    if (itr->second.type == GSVTYPE_UNIT && itr->second.value.asUnitPointer == summoned)
                        itr->second.value.asUnitPointer = nullptr;
                }
            }

            void UpdateAI(const uint32 diff) override
            {
                bool lock_move_counter = false;

                is_updating_lock = true;

                //UpdateVictim();
                // the block below modifies behaviour of UpdateVictim
                if (me->IsInCombat())
                {
                    if (!me->HasReactState(REACT_PASSIVE))
                    {
                        if (Unit *victim = me->SelectVictim(false))
                            AttackStart(victim);
                    }
                    if (me->getThreatManager().isThreatListEmpty() && !is_script_locked)
                    {
                        EnterEvadeMode();
                    }
                }

                // do not proceed script, if not in combat, and the script has not yet started/has already finished
                if (m_currentScriptType == GS_TYPE_COMBAT && !me->IsInCombat() && (com_counter == 0 || com_counter == my_commands->size()))
                {
                    // if the script has been locked, and we reached end, there's no point in holding lock anymore
                    is_script_locked = false;

                    is_updating_lock = false;
                    return;
                }

                if (!my_commands || com_counter == my_commands->size())
                {
                    // when no commands specified, or we just ended script block, there's no point in holding lock anymore
                    is_script_locked = false;

                    is_updating_lock = false;
                    if (me->GetVictim())
                        DoMeleeAttackIfReady();

                    // gossip/quest accept/quest complete script has passed to end
                    if (m_currentScriptType == GS_TYPE_GOSSIP_SELECT || m_currentScriptType == GS_TYPE_QUEST_ACCEPT || m_currentScriptType == GS_TYPE_QUEST_COMPLETE
                        || m_currentScriptType == GS_TYPE_VEHICLE_ENTER || m_currentScriptType == GS_TYPE_SPELL_RECEIVED)
                        EnterEvadeMode();

                    return;
                }

                GS_UpdateTimers(diff);

                gs_command* curr = (*my_commands)[com_counter];

                Unit* source = GS_SelectTarget(curr->command_delegate);
                if (source)
                {
                    switch (curr->type)
                    {
                        case GSCR_LABEL:    // these instructions just marks current offset
                        case GSCR_ENDIF:
                        case GSCR_REPEAT:
                        case GSCR_ENDWHEN:
                            break;
                        case GSCR_GOTO:
                            com_counter = curr->params.c_goto_label.instruction_offset;
                            break;
                        case GSCR_WAIT:
                            // if we are already waiting, loop in timer until the end is reached
                            if (is_waiting)
                            {
                                // if waiting ended, cancel wait flag
                                if (wait_timer < diff)
                                    is_waiting = false;
                                else
                                    wait_timer -= diff;
                            }
                            else // just stumbled upon wait instruction
                            {
                                // set waiting flag and store waiting parameters
                                is_waiting = true;
                                wait_timer = GS_GetValueFromSpecifier(curr->params.c_wait.delay).toInteger();
                                wait_flags = curr->params.c_wait.flags;
                            }
                            break;
                        case GSCR_CAST:
                        {
                            Unit* target = GS_SelectTarget(curr->params.c_cast.target_type);
                            source->CastSpell(target, GS_GetValueFromSpecifier(curr->params.c_cast.spell).toInteger(), curr->params.c_cast.triggered);
                            break;
                        }
                        case GSCR_SAY:
                        {
                            source->MonsterSay(curr->params.c_say_yell.tosay, LANG_UNIVERSAL, 0);
                            int sound = GS_GetValueFromSpecifier(curr->params.c_say_yell.sound_id).toInteger();
                            if (sound > 0)
                                source->PlayDirectSound(sound);
                            break;
                        }
                        case GSCR_YELL:
                        {
                            source->MonsterYell(curr->params.c_say_yell.tosay, LANG_UNIVERSAL, 0);
                            int sound = GS_GetValueFromSpecifier(curr->params.c_say_yell.sound_id).toInteger();
                            if (sound > 0)
                                source->PlayDirectSound(sound);
                            break;
                        }
                        case GSCR_IF:
                            // if script does not meet condition passed in, move to endif offset
                            if (!GS_Meets(curr->params.c_if.condition))
                                com_counter = curr->params.c_if.endif_offset;
                            break;
                        case GSCR_WHEN:
                            // if we already passed this when (condition was at least once true), or if the condition is not true,
                            // move to endwhen; otherwise pass to condition body and insert WHEN offset to set, to avoid passing again
                            if (when_set.find(com_counter) != when_set.end() || !GS_Meets(curr->params.c_when.condition))
                                com_counter = curr->params.c_when.endwhen_offset;
                            else
                                when_set.insert(com_counter);
                            break;
                        case GSCR_WHILE:
                            // if script does not meet condition passed in, move to endwhile offset
                            if (!GS_Meets(curr->params.c_while.condition))
                                com_counter = curr->params.c_while.endwhile_offset;
                            break;
                        case GSCR_ENDWHILE:
                            com_counter = curr->params.c_endwhile.while_offset;
                            // this avoids moving to next instruction after switch command (just this method call pass)
                            lock_move_counter = true;
                            break;
                        case GSCR_UNTIL:
                            // if script does not meet condition passed in, move to repeat offset
                            if (!GS_Meets(curr->params.c_until.condition))
                                com_counter = curr->params.c_until.repeat_offset;
                            break;
                        case GSCR_COMBATSTOP:
                            disable_melee = true;
                            source->AttackStop();
                            source->getThreatManager().clearReferences();
                            break;
                        case GSCR_FACTION:
                            // if the faction is larger than 0, that means set faction to specified value
                            if (curr->params.c_faction.faction.value > 0)
                            {
                                // store original faction, if not stored yet
                                if (stored_faction == 0)
                                    stored_faction = source->getFaction();
                                source->setFaction(GS_GetValueFromSpecifier(curr->params.c_faction.faction).toInteger());
                            }
                            // or if we are about to restore original faction and we already stored that value
                            else if (stored_faction != 0)
                            {
                                // restore faction
                                source->setFaction(stored_faction);
                                stored_faction = 0;
                            }
                            break;
                        case GSCR_REACT:
                            if (source->GetTypeId() == TYPEID_UNIT)
                            {
                                if (stored_react >= 99)
                                    stored_react = (uint8)source->ToCreature()->GetReactState();
                                source->ToCreature()->SetReactState(curr->params.c_react.reactstate);
                            }
                            break;
                        case GSCR_KILL:
                            if (Unit* victim = GS_SelectTarget(curr->params.c_kill.target))
                                source->Kill(victim);
                            break;
                        case GSCR_COMBATSTART:
                            disable_melee = false;
                            source->Attack(source->GetVictim(), true);
                            break;
                        case GSCR_TIMER:
                            GS_SetTimer(curr->params.c_timer.timer_id, GS_GetValueFromSpecifier(curr->params.c_timer.value).toInteger());
                            break;
                        case GSCR_MORPH:
                            if (curr->params.c_morph.morph_id.value > 0)
                            {
                                if (stored_modelid == 0)
                                    stored_modelid = source->GetDisplayId();
                                source->SetDisplayId(GS_GetValueFromSpecifier(curr->params.c_morph.morph_id).toInteger());
                            }
                            else if (stored_modelid > 0)
                            {
                                source->DeMorph();
                                stored_modelid = 0;
                            }
                        case GSCR_SUMMON:
                        {
                            float x = GS_GetValueFromSpecifier(curr->params.c_summon.x).toFloat();
                            float y = GS_GetValueFromSpecifier(curr->params.c_summon.y).toFloat();
                            float z = GS_GetValueFromSpecifier(curr->params.c_summon.z).toFloat();
                            float o = source->GetOrientation();
                            if (x == 0 && y == 0 && z == 0)
                                source->GetPosition(x, y, z, o);

                            TempSummonType sumtype = TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT;
                            if (curr->params.c_summon.nodespawn)
                                sumtype = TEMPSUMMON_MANUAL_DESPAWN;

                            source->SummonCreature(GS_GetValueFromSpecifier(curr->params.c_summon.creature_entry).toInteger(), x, y, z, o, sumtype, 15000);
                            break;
                        }
                        case GSCR_WALK:
                            source->SetWalk(true);
                            is_moving = true;
                            source->GetMotionMaster()->MovePoint(100,
                                GS_GetValueFromSpecifier(curr->params.c_walk_run_teleport.x).toFloat(),
                                GS_GetValueFromSpecifier(curr->params.c_walk_run_teleport.y).toFloat(),
                                GS_GetValueFromSpecifier(curr->params.c_walk_run_teleport.z).toFloat(),
                                true);
                            break;
                        case GSCR_RUN:
                            source->SetWalk(false);
                            is_moving = true;
                            source->GetMotionMaster()->MovePoint(100,
                                GS_GetValueFromSpecifier(curr->params.c_walk_run_teleport.x).toFloat(),
                                GS_GetValueFromSpecifier(curr->params.c_walk_run_teleport.y).toFloat(),
                                GS_GetValueFromSpecifier(curr->params.c_walk_run_teleport.z).toFloat(),
                                true);
                            break;
                        case GSCR_TELEPORT:
                            source->NearTeleportTo(GS_GetValueFromSpecifier(curr->params.c_walk_run_teleport.x).toFloat(),
                                GS_GetValueFromSpecifier(curr->params.c_walk_run_teleport.y).toFloat(),
                                GS_GetValueFromSpecifier(curr->params.c_walk_run_teleport.z).toFloat(),
                                source->GetOrientation());
                            break;
                        case GSCR_WAITFOR:
                            is_waiting = true;
                            wait_flags = 0;
                            if (curr->params.c_waitfor.eventtype == GSET_MOVEMENT && !is_moving)
                                is_waiting = false;
                            else if (curr->params.c_waitfor.eventtype == GSET_CAST && !source->IsNonMeleeSpellCasted(false))
                                is_waiting = false;
                            else if (curr->params.c_waitfor.eventtype == GSET_NONE)
                                is_waiting = false;
                            break;
                        case GSCR_LOCK:
                            is_script_locked = true;
                            break;
                        case GSCR_UNLOCK:
                            is_script_locked = false;
                            break;
                        case GSCR_SCALE:
                            if (!curr->params.c_scale.restore)
                            {
                                if (stored_scale < 0.0f)
                                    stored_scale = source->GetFloatValue(OBJECT_FIELD_SCALE_X);

                                source->SetFloatValue(OBJECT_FIELD_SCALE_X, GS_GetValueFromSpecifier(curr->params.c_scale.scale).toFloat());
                            }
                            else
                            {
                                if (stored_scale > 0.0f)
                                {
                                    source->SetFloatValue(OBJECT_FIELD_SCALE_X, stored_scale);
                                    stored_scale = -1.0f;
                                }
                            }
                        case GSCR_FLAGS:
                            if (curr->params.c_flags.op == GSFO_ADD)
                                source->SetFlag(curr->params.c_flags.field, curr->params.c_flags.value);
                            else if (curr->params.c_flags.op == GSFO_REMOVE)
                                source->RemoveFlag(curr->params.c_flags.field, curr->params.c_flags.value);
                            else if (curr->params.c_flags.op == GSFO_SET)
                                source->SetUInt32Value(curr->params.c_flags.field, curr->params.c_flags.value);
                            break;
                        case GSCR_IMMUNITY:
                            if (curr->params.c_immunity.op == GSFO_ADD)
                                source->ApplySpellImmune(0, IMMUNITY_MECHANIC, curr->params.c_immunity.mask, true);
                            else if (curr->params.c_immunity.op == GSFO_REMOVE)
                                source->ApplySpellImmune(0, IMMUNITY_MECHANIC, curr->params.c_immunity.mask, false);
                            break;
                        case GSCR_EMOTE:
                            if (Unit* source = GS_SelectTarget(curr->params.c_emote.subject))
                                source->HandleEmoteCommand(GS_GetValueFromSpecifier(curr->params.c_emote.emote_id).toInteger());
                            break;
                        case GSCR_MOVIE:
                            if (Unit* source = GS_SelectTarget(curr->params.c_emote.subject))
                                if (Player* pl = source->ToPlayer())
                                    pl->SendMovieStart(GS_GetValueFromSpecifier(curr->params.c_movie.movie_id).toInteger());
                            break;
                        case GSCR_AURA:
                            if (Unit* source = GS_SelectTarget(curr->params.c_aura.subject))
                            {
                                if (curr->params.c_aura.op == GSFO_ADD)
                                    source->AddAura(GS_GetValueFromSpecifier(curr->params.c_aura.aura_id).toInteger(), source);
                                else if (curr->params.c_aura.op == GSFO_REMOVE)
                                    source->RemoveAurasDueToSpell(GS_GetValueFromSpecifier(curr->params.c_aura.aura_id).toInteger());
                            }
                            break;
                        case GSCR_SPEED:
                            if (curr->params.c_speed.movetype >= 0 && curr->params.c_speed.movetype <= MAX_MOVE_TYPE)
                                source->SetSpeed((UnitMoveType)curr->params.c_speed.movetype, GS_GetValueFromSpecifier(curr->params.c_speed.speed).toFloat(), true);
                            break;
                        case GSCR_MOVE:
                            if (curr->params.c_move.movetype < 0)
                            {
                                source->GetMotionMaster()->MovementExpired();
                                source->GetMotionMaster()->MoveIdle();
                                source->StopMoving();

                                source->AddUnitState(UNIT_STATE_CANNOT_TURN);
                                source->SetOrientation(source->GetOrientation());
                                source->SendMovementFlagUpdate();
                            }
                            else
                            {
                                // if we are idling right now, start moving (if possible)
                                if (source->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE)
                                {
                                    source->ClearUnitState(UNIT_STATE_CANNOT_TURN);
                                    source->GetMotionMaster()->MovementExpired(true);
                                    if (source->GetVictim())
                                        source->GetMotionMaster()->MoveChase(source->GetVictim());
                                }

                                if (curr->params.c_move.movetype == MOVE_RUN)
                                    source->SetWalk(false);
                                if (curr->params.c_move.movetype == MOVE_WALK)
                                    source->SetWalk(true);
                            }
                            break;
                        case GSCR_MOUNT:
                            source->Mount(GS_GetValueFromSpecifier(curr->params.c_mount.mount_model_id).toInteger());
                            break;
                        case GSCR_UNMOUNT:
                            source->Unmount();
                            break;
                        case GSCR_QUEST:
                            if (m_scriptInvoker && m_scriptInvoker->IsInWorld() && m_scriptInvoker->GetTypeId() == TYPEID_PLAYER)
                            {
                                if (curr->params.c_quest.op == GSQO_COMPLETE)
                                    m_scriptInvoker->ToPlayer()->CompleteQuest(GS_GetValueFromSpecifier(curr->params.c_quest.quest_id).toInteger());
                                else if (curr->params.c_quest.op == GSQO_FAIL)
                                    m_scriptInvoker->ToPlayer()->FailQuest(GS_GetValueFromSpecifier(curr->params.c_quest.quest_id).toInteger());
                                else if (curr->params.c_quest.op == GSQO_PROGRESS)
                                    m_scriptInvoker->ToPlayer()->AddQuestObjectiveProgress(GS_GetValueFromSpecifier(curr->params.c_quest.quest_id).toInteger(), GS_GetValueFromSpecifier(curr->params.c_quest.objective_index).toInteger(), GS_GetValueFromSpecifier(curr->params.c_quest.value).toInteger());
                            }
                            break;
                        case GSCR_DESPAWN:
                            if (Unit* subj = GS_SelectTarget(curr->params.c_despawn.subject))
                                if (subj->GetTypeId() == TYPEID_UNIT)
                                    subj->ToCreature()->DespawnOrUnsummon();
                            break;
                        case GSCR_VAR:
                        {
                            auto itr = variable_map.find(curr->params.c_var.variable);
                            if (itr == variable_map.end())
                            {
                                variable_map[curr->params.c_var.variable] = GS_Variable();
                                itr = variable_map.find(curr->params.c_var.variable);
                            }

                            GS_Variable working = GS_GetValueFromSpecifier(curr->params.c_var.spec);

                            if (curr->params.c_var.op == GSNOP_ASSIGN)
                                itr->second = working;
                            else if (curr->params.c_var.op == GSNOP_ADD)
                                itr->second += working;
                            else if (curr->params.c_var.op == GSNOP_SUBTRACT)
                                itr->second -= working;
                            else if (curr->params.c_var.op == GSNOP_MULTIPLY)
                                itr->second *= working;
                            else if (curr->params.c_var.op == GSNOP_DIVIDE)
                            {
                                // change type of our variable to floating point number instead of integer
                                if (itr->second.type == GSVTYPE_INTEGER)
                                {
                                    itr->second.value.asFloat = (float)itr->second.value.asInteger;
                                    itr->second.type = GSVTYPE_FLOAT;
                                }
                                itr->second /= working;
                            }
                            else if (curr->params.c_var.op == GSNOP_DIVIDE_INT)
                            {
                                // change both types to integer, to be sure we do integer division
                                if (itr->second.type == GSVTYPE_FLOAT)
                                {
                                    itr->second.value.asInteger = (int32)itr->second.value.asFloat;
                                    itr->second.type = GSVTYPE_INTEGER;
                                }
                                if (working.type == GSVTYPE_FLOAT)
                                {
                                    working.value.asInteger = (int32)working.value.asFloat;
                                    working.type = GSVTYPE_INTEGER;
                                }
                                itr->second /= working;
                            }
                            else if (curr->params.c_var.op == GSNOP_MODULO)
                                itr->second %= working;
                            else if (curr->params.c_var.op == GSNOP_INCREMENT)
                                itr->second += GS_Variable((int32)1);
                            else if (curr->params.c_var.op == GSNOP_DECREMENT)
                                itr->second -= GS_Variable((int32)1);

                            break;
                        }
                        case GSCR_SOUND:
                        {
                            Unit* target = GS_SelectTarget(curr->params.c_sound.target);
                            source->PlayDirectSound(GS_GetValueFromSpecifier(curr->params.c_sound.sound_id).toInteger(), target->ToPlayer());
                            break;
                        }
                        case GSCR_TALK:
                        {
                            if (source->GetTypeId() == TYPEID_UNIT)
                            {
                                Unit* target = GS_SelectTarget(curr->params.c_talk.talk_target);
                                sCreatureTextMgr->SendChat(source->ToCreature(), GS_GetValueFromSpecifier(curr->params.c_talk.talk_group_id).toInteger(), target ? target->GetGUID() : 0);
                            }
                            break;
                        }
                        case GSCR_TURN:
                        {
                            GS_Variable turnpar = GS_GetValueFromSpecifier(curr->params.c_turn.amount);
                            if (curr->params.c_turn.relative && (turnpar.type == GSVTYPE_INTEGER || turnpar.type == GSVTYPE_FLOAT))
                                source->SetFacingTo(source->GetOrientation() + turnpar.toFloat());
                            else
                            {
                                if (turnpar.type == GSVTYPE_UNIT)
                                    source->SetFacingToObject(turnpar.value.asUnitPointer);
                                else
                                    source->SetFacingTo(turnpar.toFloat());
                            }
                            break;
                        }
                        default:
                        case GSCR_NONE:
                            break;
                    }
                }

                // if not explicitly waiting, move counter to next instruction
                if (!is_waiting && !lock_move_counter)
                    com_counter++;

                // if not explicitly disabled melee attack, and is not waiting or has appropriate flag to attack during waiting,
                // proceed melee attack
                if (!disable_melee && (!is_waiting || (wait_flags & GSWF_MELEE_ATTACK)))
                    DoMeleeAttackIfReady();

                is_updating_lock = false;
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new GS_ScriptedAI(c);
        }
};

void AddSC_GSAI()
{
    new GS_CreatureScript();
}
