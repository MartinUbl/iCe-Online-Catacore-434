#include "ScriptPCH.h"
#include "GSCommands.h"
#include "GSMgr.h"
#include "CreatureTextMgr.h"

#define GS_FLOAT_EPSILON 0.00001

class GS_CreatureScript : public CreatureScript
{
    public:

        GS_CreatureScript() : CreatureScript("gscript")
        {
            //
        }

        struct event_data
        {
            union
            {
                uint32 spell_id;
                uint32 summon_entry;

                struct
                {
                    uint32 damage;
                    DamageEffectType damage_type;
                }damage_dealt;
            }misc;
        };

        struct GS_EventQueueItem
        {
            EventHookType ev_hook_type;
            int start_offset;
            int end_offset;
            int current_offset;
            uint64 target_guid;
            event_data ev_data;

            GS_EventQueueItem(EventHookType evHookType, int startOffset, int endOffset, uint64 targetGUID, event_data ev_data)
            {
                ev_hook_type = evHookType;
                start_offset = startOffset;
                current_offset = startOffset;
                end_offset = endOffset;
                target_guid = targetGUID;
                this->ev_data = ev_data;
            }
        };

        struct GS_QuestScriptRecord
        {
            int acceptScriptId;
            int completeScriptId;
        };

        enum GS_VariableType
        {
            GSVTYPE_INTEGER     = 0,
            GSVTYPE_FLOAT       = 1,
            GSVTYPE_UNIT        = 2,
            GSVTYPE_GAMEOBJECT  = 3
        };

        static void GetPoint2D(float &x, float &y, float &z, float srcX, float srcY, float srcZ, float objSize, float absAngle, float distance, Map const* srcMap)
        {
            x = srcX + (objSize + distance) * cos(absAngle);
            y = srcY + (objSize + distance) * sin(absAngle);

            Trinity::NormalizeMapCoord(x);
            Trinity::NormalizeMapCoord(y);

            z = srcZ;
            float max_z = srcMap->GetWaterOrGroundLevel(x, y, z, &srcZ, false);

            if (max_z > INVALID_HEIGHT)
                z = max_z;
        }

        struct GS_Variable
        {
            union
            {
                int32 asInteger;
                float asFloat;
                Unit* asUnitPointer;
                GameObject* asGoPointer;
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
            GS_Variable(GameObject* nvalue) { value.asGoPointer = nvalue; type = GSVTYPE_GAMEOBJECT; };

            bool operator==(GS_Variable &sec)
            {
                if (type == sec.type)
                {
                    // integer comparison can be done via ==
                    if (type == GSVTYPE_INTEGER)
                        return value.asInteger == sec.value.asInteger;
                    // compare floats using defined epsilon
                    else if (type == GSVTYPE_FLOAT)
                        return fabs(value.asFloat - sec.value.asFloat) < GS_FLOAT_EPSILON;

                    // backwards compatibility
                    return value.asInteger == sec.value.asInteger;
                }
                else
                {
                    // when comparing float and integer, convert both to float and perform epsilon comparison
                    if (type == GSVTYPE_INTEGER && sec.type == GSVTYPE_FLOAT)
                        return fabs(toFloat() - sec.value.asFloat) < GS_FLOAT_EPSILON;
                    else if (type == GSVTYPE_FLOAT && sec.type == GSVTYPE_INTEGER)
                        return fabs(value.asFloat - sec.toFloat()) < GS_FLOAT_EPSILON;
                }

                return false;
            }
            bool operator!=(GS_Variable &sec)
            {
                if (type == sec.type)
                {
                    // integer comparison can be done via ==
                    if (type == GSVTYPE_INTEGER)
                        return value.asInteger != sec.value.asInteger;
                    // compare floats using defined epsilon
                    else if (type == GSVTYPE_FLOAT)
                        return fabs(value.asFloat - sec.value.asFloat) > GS_FLOAT_EPSILON;

                    // backwards compatibility
                    return value.asInteger != sec.value.asInteger;
                }
                else
                {
                    // when comparing float and integer, convert both to float and perform epsilon comparison
                    if (type == GSVTYPE_INTEGER && sec.type == GSVTYPE_FLOAT)
                        return fabs(toFloat() - sec.value.asFloat) > GS_FLOAT_EPSILON;
                    else if (type == GSVTYPE_FLOAT && sec.type == GSVTYPE_INTEGER)
                        return fabs(value.asFloat - sec.toFloat()) > GS_FLOAT_EPSILON;
                }

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

        struct GS_ScriptSettings
        {
            uint32 flags;
            uint32 cooldown;
        };

        struct GS_ScriptQueueRecord
        {
            GS_ScriptQueueRecord() { };
            GS_ScriptQueueRecord(GScriptType ptype, int32 pparam = 0, uint64 pinvokerGUID = 0) : type(ptype), param(pparam), invokerGUID(pinvokerGUID) { };

            GScriptType type;
            int32 param;
            uint64 invokerGUID;
        };

        struct GS_ScriptedAI : public ScriptedAI
        {
            std::queue<GS_EventQueueItem> eventQueue;
            // vector of commands to be executed
            CommandContainer* com_container;
            // command counter (similar to program counter register); points to specific element in my_commands
            uint32 com_counter;
            // flag for waiting (not moving to next instruction)
            bool is_waiting;
            // flag for while command
            bool lock_move_counter = false;
            // snapshoted com_counter in case of event hook calls
            uint32 snapshot_com_counter;
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
            // script ID when player comes to range
            int m_playerInRangeScriptId = -1;
            // distance parameter for player in range script
            float m_playerInRangeScriptDistance = 0.0f;
            // map of script IDs for quests
            std::map<uint32, GS_QuestScriptRecord> m_questScripts;
            // map of script IDs for spellcasts received
            std::map<int32, int32> m_spellReceivedScripts;

            // map of script settings (key = script ID, value = settings struct)
            std::map<int, GS_ScriptSettings> m_scriptSettings;
            // current script settings we use
            GS_ScriptSettings* m_currScriptSettings = nullptr;

            // current type of script being executed
            GScriptType m_currentScriptType = GS_TYPE_NONE;
            // current param of script
            int32 m_currentScriptParam = 0;

            // primary key = TYPE:PARAM pair, secondary key = unit GUID, value = time of cooldown end
            std::map<uint64, std::map<uint64, uint32>> m_scriptCooldownMap;

            // script triggers waiting in queue to be started
            std::queue<GS_ScriptQueueRecord> m_scriptQueue;

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

            uint64 m_scriptInvokerGUID = 0;
            uint64 m_inheritedScriptInvokerGUID = 0;

            Unit* m_parentUnit = nullptr;
            Unit* m_lastSummonedUnit = nullptr;

            GS_ScriptedAI(Creature* cr) : ScriptedAI(cr)
            {
                com_container = nullptr;
                for (int i = 0; i < 10; i++)
                    m_gossipSelectScripts[i] = -1;

                sGSMgr->RegisterAI(this);

                GS_LoadMyScript();

                // ordinary reset
                com_counter = 0;
                snapshot_com_counter = 0;
                lock_move_counter = false;
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

            inline void ClearEventQueue() 
            {
                while (!eventQueue.empty())
                    eventQueue.pop(); 
            }

            void DoAction(const int32 action) override
            {
                if (action == GSAI_SIGNAL_UNHOOK)
                {
                    // while any other thread is inside UpdateAI, wait to leave
                    while (is_updating_lock)
                        ;
                    com_container = nullptr;
                    com_counter = 0;

                    snapshot_com_counter = 0;
                    ClearEventQueue();
                }
                else if (action == GSAI_SIGNAL_REHOOK)
                {
                    GS_LoadMyScript();
                    ResetState();

                    if (m_currentScriptType == GS_TYPE_COMBAT && m_combatScriptId >= 0)
                        com_container = sGSMgr->GetScript(m_combatScriptId);
                    else if (m_currentScriptType == GS_TYPE_OUT_OF_COMBAT && m_outOfCombatScriptId >= 0)
                        com_container = sGSMgr->GetScript(m_outOfCombatScriptId);
                }
                else if (action == GSAI_SIGNAL_INVOKER_FROM_CREATOR)
                {
                    Unit* creator = sObjectAccessor->GetUnit(*me, me->GetCreatorGUID());
                    if (creator)
                        m_scriptInvokerGUID = creator->GetGUID();
                }
                else if (action == GSAI_SIGNAL_INVOKER_FROM_OWNER)
                {
                    Unit* creator = sObjectAccessor->GetUnit(*me, me->GetOwnerGUID());
                    if (creator)
                        m_scriptInvokerGUID = creator->GetGUID();
                }
                else if (action == GSAI_SIGNAL_INVOKER_FROM_SUMMONER)
                {
                    if (me->ToTempSummon())
                        m_scriptInvokerGUID = me->ToTempSummon()->GetSummoner() ? me->ToTempSummon()->GetSummoner()->GetGUID() : 0;
                }
                else
                    ScriptedAI::DoAction(action);
            }

            void sGossipSelect(Player* player, uint32 sender, uint32 action)
            {
                player->CLOSE_GOSSIP_MENU();
                if (m_currentScriptType != GS_TYPE_COMBAT)
                {
                    if (GS_SelectScript(GS_TYPE_GOSSIP_SELECT, action, player))
                        ResetState();
                }
            }

            void sQuestAccept(Player* player, Quest const* quest)
            {
                if (m_currentScriptType != GS_TYPE_COMBAT)
                {
                    uint32 questId = quest->GetQuestId();
                    if (GS_SelectScript(GS_TYPE_QUEST_ACCEPT, questId, player))
                        ResetState();
                }
            }

            void sQuestComplete(Player* player, Quest const* quest)
            {
                if (m_currentScriptType != GS_TYPE_COMBAT)
                {
                    uint32 questId = quest->GetQuestId();
                    if (GS_SelectScript(GS_TYPE_QUEST_COMPLETE, questId, player))
                        ResetState();
                }
            }

            void PassengerBoarded(Unit* who, int8 seatId, bool apply) override
            {
                if (m_currentScriptType != GS_TYPE_COMBAT && m_currentScriptType != GS_TYPE_VEHICLE_ENTER && apply)
                {
                    if (GS_SelectScript(GS_TYPE_VEHICLE_ENTER, 0, who))
                        ResetState();
                }

                // if we are doing vehicle entered script, and passenger is leaving, reset state if it's last passenger
                if (m_currentScriptType == GS_TYPE_VEHICLE_ENTER && !apply)
                {
                    if (!me->GetVehicleKit()->HasPassengers())
                        EnterEvadeMode();
                }
            }

            void DamageDealt(Unit* victim, uint32& damage, DamageEffectType typeOfDamage) override
            {
                event_data ev_data;
                ev_data.misc.damage_dealt.damage = damage;
                ev_data.misc.damage_dealt.damage_type = typeOfDamage;
                AddEventToQueue(EVENT_HOOK_DAMAGE_DEALT, victim, ev_data);
            }

            void SpellHit(Unit* who, const SpellEntry* spell) override
            {
                if (who != me)
                {
                    event_data ev_data;
                    ev_data.misc.spell_id = spell->Id;
                    AddEventToQueue(EVENT_HOOK_SPELL_HIT, who, ev_data);
                }

                if (m_spellReceivedScripts.find(spell->Id) != m_spellReceivedScripts.end())
                {
                    if (m_currentScriptType != GS_TYPE_COMBAT)
                    {
                        if (GS_SelectScript(GS_TYPE_SPELL_RECEIVED, spell->Id, who))
                            ResetState();
                    }
                }
            }

            void SetInheritedInvoker(Unit* pl)
            {
                m_inheritedScriptInvokerGUID = pl->GetGUID();
            }

            void SetParentUnit(Unit* parent)
            {
                m_parentUnit = parent;
            }

            void GS_LoadMyScript()
            {
                m_spellReceivedScripts.clear();

                // select everything for this creature from database
                QueryResult res = ScriptDatabase.PQuery("SELECT script_type, script_type_param, script_cooldown, flags, script_id FROM creature_gscript WHERE creature_entry = %u", me->GetEntry());

                if (res)
                {
                    Field* f;
                    do
                    {
                        f = res->Fetch();

                        uint16 type = f[0].GetUInt16();
                        int32 cooldown = f[2].GetInt32();
                        uint32 flags = f[3].GetUInt32();
                        int32 scriptId = f[4].GetInt32();
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
                            case GS_TYPE_PLAYER_IN_RANGE:
                                m_playerInRangeScriptId = scriptId;
                                m_playerInRangeScriptDistance = scriptParam;
                            default:
                            case GS_TYPE_NONE:
                                break;
                        }

                        m_scriptSettings[scriptId].cooldown = cooldown;
                        m_scriptSettings[scriptId].flags = flags;

                    } while (res->NextRow());
                }
            }

            void ResetState()
            {
                // reset command counter, waiting and melee disable flags
                com_counter = 0;
                snapshot_com_counter = 0;
                ClearEventQueue();

                lock_move_counter = false;
                is_waiting = false;
                disable_melee = false;
                is_moving = false;
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
                    // will fall back to GS_TYPE_NONE if not found
                    GS_SelectScript(GS_TYPE_OUT_OF_COMBAT);
                }

                ScriptedAI::Reset();
            }

            void EnterEvadeMode() override
            {
                ExecuteCommandDirectly(EVENT_HOOK_EVADE, nullptr);

                if (!is_script_locked)
                {
                    ResetState();
                    GS_SelectScript(GS_TYPE_OUT_OF_COMBAT);
                }

                ScriptedAI::EnterEvadeMode();
            }

            void JustDied(Unit * killer) override
            {
                ExecuteCommandDirectly(EVENT_HOOK_DIED, killer);
                ScriptedAI::JustDied(killer);
            }

            // Used only by JustDied and EnterEvadeMode hooks
            void ExecuteCommandDirectly(EventHookType hookType, Unit* unit)
            {
                if (!AddEventToQueue(hookType, unit))
                    return;

                GS_EventQueueItem &eventItem = eventQueue.front();

                while (com_counter != (uint32)eventItem.end_offset)
                {
                    if (eventItem.current_offset == eventItem.start_offset)
                    {
                        com_counter = eventItem.start_offset + 1;
                    }

                    // fail safe check (caused by swapping scripts? -> need proper solution)
                    if (!com_container)
                        break;

                    ExecuteCommand(0, lock_move_counter, true);
                    eventItem.current_offset = com_counter;

                    if (eventItem.current_offset == eventItem.end_offset)
                        break;
                }

                ClearEventQueue();
            }

            void KilledUnit(Unit * victim) override
            {
                AddEventToQueue(EVENT_HOOK_KILLED, victim);
                ScriptedAI::KilledUnit(victim);
            }

            void EnterCombat(Unit* who) override
            {
                com_counter = 0;
                snapshot_com_counter = 0;
                lock_move_counter = false;
                is_waiting = false;
                disable_melee = false;
                is_moving = false;

                if (!is_script_locked)
                    GS_SelectScript(GS_TYPE_COMBAT);

                AddEventToQueue(EVENT_HOOK_COMBAT, who);

                ScriptedAI::EnterCombat(who);
            }

            bool AddEventToQueue(EventHookType hookType, Unit* unit, event_data ev_data = event_data())
            {
                // command container is nullptr if gscript was unsuccessfully reloaded (error in script)
                if (!com_container)
                    return false;

                // if event hook is registered, schedule its execution
                if (com_container->event_offset_map.find(hookType) != com_container->event_offset_map.end())
                {
                    int startOffset = com_container->event_offset_map[hookType].start_offset;
                    int endOffset = com_container->event_offset_map[hookType].end_offset;
                    auto eventItem = GS_EventQueueItem(hookType, startOffset, endOffset, unit ? unit->GetGUID() : 0, ev_data);
                    eventQueue.push(eventItem);
                    return true;
                }

                return false;
            }

            uint64 GS_MakeTypeParamPair(GScriptType type, int32 param)
            {
                return ((uint64)type << 32) | ((uint64)param);
            }

            uint32 GS_GetScriptCooldownFor(GScriptType type, int32 param = 0, Unit* target = nullptr)
            {
                int scriptId = GS_GetScriptByType(type, param);
                // script has no cooldown => no cooldown may be present in cooldown map
                if (m_scriptSettings.find(scriptId) == m_scriptSettings.end() || m_scriptSettings[scriptId].cooldown == 0)
                    return 0;

                uint64 tppair = GS_MakeTypeParamPair(type, param);

                if (m_scriptCooldownMap.find(tppair) == m_scriptCooldownMap.end())
                    return 0;

                uint32 cdval = 0;

                if ((m_scriptSettings[scriptId].flags & GSFLAG_COOLDOWN_PER_PLAYER) && target)
                {
                    if (m_scriptCooldownMap[tppair].find(target->GetGUID()) != m_scriptCooldownMap[tppair].end())
                        cdval = m_scriptCooldownMap[tppair][target->GetGUID()];
                }
                else
                {
                    if (m_scriptCooldownMap[tppair].find(0) != m_scriptCooldownMap[tppair].end())
                        cdval = m_scriptCooldownMap[tppair][0];
                }

                // if cooldown has already passed, no cooldown is returned
                if (cdval < time(nullptr))
                    cdval = 0;

                return cdval;
            }

            void GS_SetScriptCooldownFor(GScriptType type, int32 param = 0, Unit* target = nullptr)
            {
                int scriptId = GS_GetScriptByType(type, param);
                // script has no cooldown => no cooldown may be present in cooldown map
                if (m_scriptSettings.find(scriptId) == m_scriptSettings.end() || m_scriptSettings[scriptId].cooldown == 0)
                    return;

                uint64 tppair = GS_MakeTypeParamPair(type, param);

                if ((m_scriptSettings[scriptId].flags & GSFLAG_COOLDOWN_PER_PLAYER) && target)
                    m_scriptCooldownMap[tppair][target->GetGUID()] = time(nullptr) + m_scriptSettings[scriptId].cooldown;
                else
                    m_scriptCooldownMap[tppair][0] = time(nullptr) + m_scriptSettings[scriptId].cooldown;
            }

            bool GS_HasScriptCooldownFor(GScriptType type, int32 param = 0, Unit* target = nullptr)
            {
                return (GS_GetScriptCooldownFor(type, param, target) != 0);
            }

            int GS_GetScriptByType(GScriptType type, int32 param)
            {
                // select proper script
                switch (type)
                {
                    case GS_TYPE_COMBAT:
                        return m_combatScriptId;
                    case GS_TYPE_OUT_OF_COMBAT:
                        return m_outOfCombatScriptId;
                    case GS_TYPE_GOSSIP_SELECT:
                        if (param >= 0 && param < 10)
                            return m_gossipSelectScripts[param];
                        break;
                    case GS_TYPE_QUEST_ACCEPT:
                        if (m_questScripts.find(param) != m_questScripts.end())
                            return m_questScripts[param].acceptScriptId;
                        break;
                    case GS_TYPE_QUEST_COMPLETE:
                        if (m_questScripts.find(param) != m_questScripts.end())
                            return m_questScripts[param].completeScriptId;
                        break;
                    case GS_TYPE_VEHICLE_ENTER:
                        return m_vehicleEnterScriptId;
                    case GS_TYPE_SPELL_RECEIVED:
                        if (m_spellReceivedScripts.find(param) != m_spellReceivedScripts.end())
                            return m_spellReceivedScripts[param];
                        break;
                    case GS_TYPE_PLAYER_IN_RANGE:
                        return m_playerInRangeScriptId;
                    default:
                        break;
                }

                return -1;
            }

            bool GS_SelectScript(GScriptType type, int32 param = 0, Unit* invoker = nullptr, bool fromQueue = false)
            {
                // if not retaining script from queue and we are trying to start the same type
                if (!fromQueue && m_currentScriptType == type)
                {
                    // if even the same parameter, it is the same script, so determine shared cooldowns
                    if (m_currentScriptParam == param)
                    {
                        int scriptId = GS_GetScriptByType(type, param);
                        if (scriptId > -1)
                        {
                            uint32 needflags = (GSFLAG_SHARED_COOLDOWN | GSFLAG_COOLDOWN_PER_PLAYER);
                            if (invoker && (m_scriptSettings[scriptId].flags & needflags) == needflags)
                            {
                                GS_SetScriptCooldownFor(type, param, invoker);
                                return false;
                            }
                        }
                    }
                    m_scriptQueue.push(GS_ScriptQueueRecord(type, param, invoker ? invoker->GetGUID() : 0));
                    return false;
                }

                int scriptId = GS_GetScriptByType(type, param);

                // if the script is present, use it
                if (scriptId > -1)
                {
                    if (GS_HasScriptCooldownFor(type, param, invoker))
                        return false;

                    com_container = sGSMgr->GetScript(scriptId);
                    m_currentScriptType = type;
                    m_scriptInvokerGUID = invoker ? invoker->GetGUID() : 0;
                    m_currentScriptParam = param;

                    if (m_scriptSettings.find(scriptId) != m_scriptSettings.end())
                        m_currScriptSettings = &m_scriptSettings[scriptId];

                    GS_SetScriptCooldownFor(type, param, invoker);

                    return true;
                }
                else
                {
                    // reset state
                    com_container = nullptr;
                    m_currentScriptType = GS_TYPE_NONE;
                    m_currScriptSettings = nullptr;
                    m_scriptInvokerGUID = 0;
                    m_currentScriptParam = 0;
                }

                return false;
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

            Unit* GS_SelectUnitTarget(gs_specifier& spec)
            {
                WorldObject * obj = GS_SelectTarget(spec);
                if (obj && obj->GetTypeId() == TYPEID_UNIT)
                    return obj->ToUnit();
                return nullptr;
            }

            GameObject* GS_SelectGameObjectTarget(gs_specifier& spec)
            {
                WorldObject * obj = GS_SelectTarget(spec);
                if (obj && obj->GetTypeId() == TYPEID_GAMEOBJECT)
                    return obj->ToGameObject();
                return nullptr;
            }

            // select target using specifier supplied; does not work for one-values, just unit specifiers
            WorldObject* GS_SelectTarget(gs_specifier& spec)
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
                    {
                        uint64 invGUID = m_scriptInvokerGUID ? m_scriptInvokerGUID : m_inheritedScriptInvokerGUID;
                        return Unit::GetUnit(*me, invGUID);
                    }
                    // parent (summoner)
                    case GSST_PARENT:
                        return m_parentUnit ? m_parentUnit : me;
                    case GSST_CLOSEST_CREATURE:
                        return GetClosestCreatureWithEntry(me, spec.value, 300.0f, true);
                    case GSST_CLOSEST_GAMEOBJECT:
                        return me->FindNearestGameObject(spec.value, 300.0f);
                    case GSST_CLOSEST_PLAYER:
                        return me->FindNearestPlayer(300.0f, true);
                    case GSST_LAST_SUMMON:
                        return m_lastSummonedUnit;
                    case GSST_CLOSEST_CREATURE_GUID:
                        return GetClosestCreatureWithDBGuid(me, spec.value, 300.0f, true);
                    case GSST_CLOSEST_DEAD_CREATURE:
                        return GetClosestCreatureWithEntry(me, spec.value, 300.0f, false);
                    // may be also variable value, if the variable points to unit
                    case GSST_VARIABLE_VALUE:
                    {
                        // check if variable is from event context
                        if (!eventQueue.empty())
                        {
                            GS_EventQueueItem eventItem = eventQueue.front();

                            switch (spec.value)
                            {
                                case EVENT_VARIABLE_ENEMY_ID:
                                case EVENT_VARIABLE_KILLER_ID:
                                case EVENT_VARIABLE_VICTIM_ID:
                                case EVENT_VARIABLE_SUMMON_ID:
                                case EVENT_VARIABLE_CASTER_ID:
                                    return Unit::GetUnit(*me, eventItem.target_guid);
                            }
                        }

                        // find regular variable
                        if (variable_map.find(spec.value) != variable_map.end())
                        {
                            GS_Variable var = variable_map[spec.value];

                            if (var.type == GSVTYPE_UNIT)
                            {
                                return var.value.asUnitPointer;
                            }
                            else if (var.type == GSVTYPE_GAMEOBJECT)
                            {
                                return var.value.asGoPointer;
                            }
                        }
                        return nullptr;
                    }
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
                if (spec.subject_type == GSST_VARIABLE_VALUE && spec.subject_parameter == GSSP_NONE)
                {
                    if (!eventQueue.empty())
                    {
                        GS_EventQueueItem eventItem = eventQueue.front();

                        switch (spec.value)
                        {
                            case EVENT_VARIABLE_ENEMY_ID:
                            case EVENT_VARIABLE_KILLER_ID:
                            case EVENT_VARIABLE_VICTIM_ID:
                            case EVENT_VARIABLE_SUMMON_ID:
                            case EVENT_VARIABLE_CASTER_ID:
                            case EVENT_VARIABLE_TARGET_ID:
                                return GS_Variable(Unit::GetUnit(*me, eventItem.target_guid));
                            case EVENT_VARIABLE_SPELL_ID:
                                return GS_Variable((int32)eventItem.ev_data.misc.spell_id);
                            case EVENT_VARIABLE_SUMMON_ENTRY_ID:
                                return GS_Variable((int32)eventItem.ev_data.misc.summon_entry);
                            case EVENT_VARIABLE_DAMAGE_ID:
                                return GS_Variable((int32)eventItem.ev_data.misc.damage_dealt.damage);
                            case EVENT_VARIABLE_DAMAGE_TYPE_ID:
                                return GS_Variable((int32)eventItem.ev_data.misc.damage_dealt.damage_type);
                        }
                    }

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

                if (spec.subject_type == GSST_CLOSEST_GAMEOBJECT)
                    return GS_Variable(GS_SelectGameObjectTarget(spec));

                // there are only two meaningful subject types: me and target,
                // any other target is not so valuable here

                Unit* subject = GS_SelectUnitTarget(spec);
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
                    case GSSP_GUIDLOW:
                        return (subject->GetTypeId() == TYPEID_UNIT) ? GS_Variable((int32)subject->ToCreature()->GetDBTableGUIDLow()) : GS_Variable((int32)subject->GetGUIDLow());
                    case GSSP_ENTRY:
                        return (subject->GetTypeId() == TYPEID_UNIT && subject->ToCreature()->GetCreatureInfo()) ? GS_Variable((int32)subject->ToCreature()->GetCreatureInfo()->Entry) : GS_Variable((int32)subject->GetEntry());
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

                GS_Variable righthand = GS_GetValueFromSpecifier(cond.dest);

                // set operations (has, hasnt)
                if (cond.op == GSOP_HAS || cond.op == GSOP_HASNT)
                {
                    if (cond.source.subject_parameter == GSSP_AURAS)
                    {
                        Unit* source = GS_SelectUnitTarget(cond.source);
                        if (!source)
                            return false;

                        if (source->HasAura(righthand.toInteger()))
                            return (cond.op == GSOP_HAS);
                        else
                            return (cond.op == GSOP_HASNT);
                    }
                }

                // from now, we support just 3 parameter condition (lefthand, operator, righthand)
                GS_Variable lefthand = GS_GetValueFromSpecifier(cond.source);

                switch (cond.op)
                {
                    case GSOP_EQUALS:
                    case GSOP_IS:
                        return lefthand == righthand;
                    case GSOP_NOT_EQUAL:
                    case GSOP_ISNT:
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

            void MoveInLineOfSight(Unit* who) override
            {
                if (who && who->GetTypeId() == TYPEID_PLAYER && m_playerInRangeScriptId > -1 && me->GetDistance(who) <= m_playerInRangeScriptDistance && me->IsWithinLOSInMap(who, false))
                {
                    if (GS_SelectScript(GS_TYPE_PLAYER_IN_RANGE, (int32)m_playerInRangeScriptDistance, who))
                        ResetState();
                }
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
                    if (m_scriptInvokerGUID)
                        targetAI->SetInheritedInvoker(Unit::GetUnit(*me, m_scriptInvokerGUID));

                    // set me as parent of summoned unit
                    targetAI->SetParentUnit(me);
                }

                event_data ev_data;
                ev_data.misc.summon_entry = summoned->GetEntry();
                AddEventToQueue(EVENT_HOOK_JUST_SUMMONED, summoned, ev_data);

                m_lastSummonedUnit = summoned;
            }

            void AttackStart(Unit *who) override
            {
                ScriptedAI::AttackStart(who);
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
                lock_move_counter = false;

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
                if (m_currentScriptType == GS_TYPE_COMBAT && !me->IsInCombat() && (com_counter == 0 || com_counter == com_container->command_vector.size()))
                {
                    // if the script has been locked, and we reached end, there's no point in holding lock anymore
                    is_script_locked = false;

                    is_updating_lock = false;
                    return;
                }

                if (!com_container || com_counter == com_container->command_vector.size())
                {
                    // when no commands specified, or we just ended script block, there's no point in holding lock anymore
                    is_script_locked = false;

                    is_updating_lock = false;
                    if (me->GetVictim())
                        DoMeleeAttackIfReady();

                    // gossip/quest accept/quest complete script has passed to end
                    if (m_currentScriptType == GS_TYPE_GOSSIP_SELECT || m_currentScriptType == GS_TYPE_QUEST_ACCEPT || m_currentScriptType == GS_TYPE_QUEST_COMPLETE
                        || m_currentScriptType == GS_TYPE_VEHICLE_ENTER || m_currentScriptType == GS_TYPE_SPELL_RECEIVED || m_currentScriptType == GS_TYPE_PLAYER_IN_RANGE)
                    {
                        // if there's something in script queue
                        if (!m_scriptQueue.empty())
                        {
                            // retrieve last waiting script
                            GS_ScriptQueueRecord qscr = m_scriptQueue.front();
                            m_scriptQueue.pop();

                            // if there's invoker stored, retrieve pointer using its GUID
                            if (qscr.invokerGUID)
                            {
                                Unit* invoker = sObjectAccessor->FindUnit(qscr.invokerGUID);

                                // we have to be sure the invoker is present
                                if (invoker)
                                    if (GS_SelectScript(qscr.type, qscr.param, invoker, true))
                                        ResetState();
                            }
                            else
                            {
                                // otherwise start script without invoker
                                if (GS_SelectScript(qscr.type, qscr.param, nullptr, true))
                                    ResetState();
                            }
                        }
                        else
                            EnterEvadeMode();
                    }

                    return;
                }

                GS_UpdateTimers(diff);

                // handle event commands
                if (!eventQueue.empty())
                {
                    GS_EventQueueItem &eventItem = eventQueue.front();

                    if (eventItem.current_offset == eventItem.start_offset)
                    {
                        snapshot_com_counter = com_counter;
                        com_counter = eventItem.start_offset + 1;
                    }

                    ExecuteCommand(diff, lock_move_counter, true);
                    eventItem.current_offset = com_counter;

                    if (eventItem.current_offset == eventItem.end_offset)
                    {
                        eventQueue.pop();
                    }

                    if (eventQueue.empty())
                    {
                        com_counter = snapshot_com_counter;
                    }

                    is_updating_lock = false;
                    return;
                }

                // main flow of commands
                ExecuteCommand(diff, lock_move_counter, false);
                is_updating_lock = false;
            }

            void ExecuteCommand(const uint32 diff, bool &lock_move_counter, bool is_in_event_handler)
            {
                gs_command* curr = com_container->command_vector[com_counter];
                Unit* source = GS_SelectUnitTarget(curr->command_delegate);

                if (source)
                {
                    switch (curr->type)
                    {
                    case GSCR_LABEL:    // these instructions just marks current offset
                    case GSCR_ENDIF:
                    case GSCR_REPEAT:
                    case GSCR_ENDWHEN:
                    case GSCR_END_EVENT:
                        break;
                    case GSCR_EVENT:
                        com_counter = curr->params.c_event.end_offset;
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
                        Unit* target = GS_SelectUnitTarget(curr->params.c_cast.target_type);
                        source->CastSpell(target, GS_GetValueFromSpecifier(curr->params.c_cast.spell).toInteger(), curr->params.c_cast.triggered);
                        break;
                    }
                    case GSCR_SAY:
                    {
                        Unit* invoker = Unit::GetUnit(*me, m_scriptInvokerGUID ? m_scriptInvokerGUID : m_inheritedScriptInvokerGUID);
                        source->MonsterSay(curr->params.c_say_yell.tosay, LANG_UNIVERSAL, invoker ? invoker->GetGUID() : 0);
                        int sound = GS_GetValueFromSpecifier(curr->params.c_say_yell.sound_id).toInteger();
                        if (sound > 0)
                            source->PlayDirectSound(sound);

                        int emote = GS_GetValueFromSpecifier(curr->params.c_say_yell.emote_id).toInteger();
                        if (emote > 0)
                            source->HandleEmoteCommand(emote);
                        break;
                    }
                    case GSCR_YELL:
                    {
                        Unit* invoker = Unit::GetUnit(*me, m_scriptInvokerGUID ? m_scriptInvokerGUID : m_inheritedScriptInvokerGUID);
                        source->MonsterYell(curr->params.c_say_yell.tosay, LANG_UNIVERSAL, invoker ? invoker->GetGUID() : 0);
                        int sound = GS_GetValueFromSpecifier(curr->params.c_say_yell.sound_id).toInteger();
                        if (sound > 0)
                            source->PlayDirectSound(sound);

                        int emote = GS_GetValueFromSpecifier(curr->params.c_say_yell.emote_id).toInteger();
                        if (emote > 0)
                            source->HandleEmoteCommand(emote);
                        break;
                    }
                    case GSCR_TEXTEMOTE:
                    {
                        Unit* invoker = Unit::GetUnit(*me, m_scriptInvokerGUID ? m_scriptInvokerGUID : m_inheritedScriptInvokerGUID);
                        source->MonsterTextEmote(curr->params.c_say_yell.tosay, invoker ? invoker->GetGUID() : 0, false);
                        int sound = GS_GetValueFromSpecifier(curr->params.c_say_yell.sound_id).toInteger();
                        if (sound > 0)
                            source->PlayDirectSound(sound);

                        int emote = GS_GetValueFromSpecifier(curr->params.c_say_yell.emote_id).toInteger();
                        if (emote > 0)
                            source->HandleEmoteCommand(emote);
                        break;
                    }
                    case GSCR_BOSSEMOTE:
                    {
                        Unit* invoker = Unit::GetUnit(*me, m_scriptInvokerGUID ? m_scriptInvokerGUID : m_inheritedScriptInvokerGUID);
                        source->MonsterTextEmote(curr->params.c_say_yell.tosay, invoker ? invoker->GetGUID() : 0, true);
                        int sound = GS_GetValueFromSpecifier(curr->params.c_say_yell.sound_id).toInteger();
                        if (sound > 0)
                            source->PlayDirectSound(sound);

                        int emote = GS_GetValueFromSpecifier(curr->params.c_say_yell.emote_id).toInteger();
                        if (emote > 0)
                            source->HandleEmoteCommand(emote);
                        break;
                    }
                    case GSCR_WHISPER:
                    {
                        Unit* invoker = Unit::GetUnit(*me, m_scriptInvokerGUID ? m_scriptInvokerGUID : m_inheritedScriptInvokerGUID);
                        Unit* target;
                        if (curr->params.c_whisper.target.subject_type == GSST_NONE)
                            target = invoker;
                        else
                            target = GS_SelectUnitTarget(curr->params.c_whisper.target);

                        source->MonsterWhisper(curr->params.c_whisper.tosay, target ? target->GetGUID() : 0);
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
                        if (Unit* victim = GS_SelectUnitTarget(curr->params.c_kill.target))
                            source->Kill(victim);
                        break;
                    case GSCR_ATTACK:
                        disable_melee = !curr->params.c_attack.is_attack_enabled;
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
                        break;
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
                    case GSCR_SUMMONGO:
                    {
                        float x = GS_GetValueFromSpecifier(curr->params.c_summon_go.x).toFloat();
                        float y = GS_GetValueFromSpecifier(curr->params.c_summon_go.y).toFloat();
                        float z = GS_GetValueFromSpecifier(curr->params.c_summon_go.z).toFloat();
                        float o = source->GetOrientation();
                        if (x == 0 && y == 0 && z == 0)
                            source->GetPosition(x, y, z, o);

                        uint32 respawnTimer = 0;
                        if (curr->params.c_summon_go.respawn_timer.value > 0)
                            respawnTimer = (uint32)curr->params.c_summon_go.respawn_timer.value;

                        source->SummonGameObject(GS_GetValueFromSpecifier(curr->params.c_summon_go.go_entry).toInteger(), x, y, z, o, 0, 0, 0, 0, respawnTimer);
                        break;
                    }
                    case GSCR_GO:
                    {
                        GameObject* go = GS_SelectGameObjectTarget(curr->params.c_go.subject);
                        if (!go)
                            break;

                        switch (curr->params.c_go.op)
                        {
                        case GSGO_USE:
                            go->Use(source);
                            break;
                        case GSGO_RESET:
                            go->ResetDoorOrButton();
                            break;
                        case GSGO_TOGGLE:
                            go->SwitchDoorOrButton(!go->HasFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE));
                            break;
                        case GSGO_SET_STATE:
                            go->SetGoState((GOState)curr->params.c_go.value);
                            break;
                        default:
                            break;
                        }
                        break;
                    }
                    case GSCR_CALL_FOR_HELP:
                    {
                        float call_radius = GS_GetValueFromSpecifier(curr->params.c_call_for_help.radius).toFloat();
                        if (source->GetTypeId() == TYPEID_UNIT && call_radius < MAX_VISIBILITY_DISTANCE)
                            source->ToCreature()->CallForHelp(call_radius);
                        break;
                    }
                    case GSCR_SET_SHEATHE_STATE:
                    {
                        if (source->GetTypeId() == TYPEID_UNIT)
                            source->SetSheath((SheathState)curr->params.c_set_sheathe_state.state);
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
                        source->HandleEmoteCommand(GS_GetValueFromSpecifier(curr->params.c_emote.emote_id).toInteger());
                        break;
                    case GSCR_MOVIE:
                        if (Player* pl = source->ToPlayer())
                            pl->SendMovieStart(GS_GetValueFromSpecifier(curr->params.c_movie.movie_id).toInteger());
                        break;
                    case GSCR_AURA:
                        if (Unit* target = GS_SelectUnitTarget(curr->params.c_aura.subject))
                        {
                            if (curr->params.c_aura.op == GSFO_ADD)
                                target->AddAura(GS_GetValueFromSpecifier(curr->params.c_aura.aura_id).toInteger(), source);
                            else if (curr->params.c_aura.op == GSFO_REMOVE)
                                target->RemoveAurasDueToSpell(GS_GetValueFromSpecifier(curr->params.c_aura.aura_id).toInteger());
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
                    {
                        Unit* invoker = Unit::GetUnit(*me, m_scriptInvokerGUID ? m_scriptInvokerGUID : m_inheritedScriptInvokerGUID);
                        if (invoker && invoker->IsInWorld() && invoker->GetTypeId() == TYPEID_PLAYER)
                        {
                            if (curr->params.c_quest.op == GSQO_COMPLETE)
                                invoker->ToPlayer()->CompleteQuest(GS_GetValueFromSpecifier(curr->params.c_quest.quest_id).toInteger());
                            else if (curr->params.c_quest.op == GSQO_FAIL)
                                invoker->ToPlayer()->FailQuest(GS_GetValueFromSpecifier(curr->params.c_quest.quest_id).toInteger());
                            else if (curr->params.c_quest.op == GSQO_PROGRESS)
                                invoker->ToPlayer()->AddQuestObjectiveProgress(GS_GetValueFromSpecifier(curr->params.c_quest.quest_id).toInteger(), GS_GetValueFromSpecifier(curr->params.c_quest.objective_index).toInteger(), GS_GetValueFromSpecifier(curr->params.c_quest.value).toInteger());
                        }
                        break;
                    }
                    case GSCR_DESPAWN:
                        if (Unit* subj = GS_SelectUnitTarget(curr->params.c_despawn.subject))
                            if (subj->GetTypeId() == TYPEID_UNIT)
                                subj->ToCreature()->DespawnOrUnsummon();
                        break;
                    case GSCR_DESPAWNGO:
                        if (GameObject* go = GS_SelectGameObjectTarget(curr->params.c_despawn_go.subject))
                            go->Delete();
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
                        Unit* target = GS_SelectUnitTarget(curr->params.c_sound.target);
                        source->PlayDirectSound(GS_GetValueFromSpecifier(curr->params.c_sound.sound_id).toInteger(), target ? target->ToPlayer() : nullptr);
                        break;
                    }
                    case GSCR_TALK:
                    {
                        if (source->GetTypeId() == TYPEID_UNIT)
                        {
                            Unit* target = GS_SelectUnitTarget(curr->params.c_talk.talk_target);
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
                    case GSCR_FOLLOW:
                    {
                        if (Unit* target = GS_SelectUnitTarget(curr->params.c_follow.subject))
                            source->GetMotionMaster()->MoveFollow(target, GS_GetValueFromSpecifier(curr->params.c_follow.distance).toFloat(), GS_GetValueFromSpecifier(curr->params.c_follow.angle).toFloat());
                        else
                            source->GetMotionMaster()->MovementExpired();
                        break;
                    }
                    case GSCR_UNVEHICLE:
                    {
                        GS_Variable vehEntry = GS_GetValueFromSpecifier(curr->params.c_unvehicle.entry);
                        if (Unit* veh = source->GetVehicleBase())
                            if (vehEntry.value.asInteger == 0 || vehEntry.value.asInteger == (int32)veh->GetEntry())
                                source->ExitVehicle();
                        break;
                    }
                    case GSCR_VISIBILITY:
                        if (curr->params.c_visibility.set_visible)
                            source->SetVisibility(VISIBILITY_ON);
                        else
                            source->SetVisibility(VISIBILITY_OFF);
                        break;
                    case GSCR_EVADE:
                        EnterEvadeMode();
                        break;
                    case GSCR_RESET:
                    {
                        if (Creature* cr = source->ToCreature())
                        {
                            if (cr->GetAI())
                                cr->GetAI()->Reset();
                            cr->LoadCreaturesAddon(true);

                            cr->GetMotionMaster()->MovementExpired(true);
                            cr->GetMotionMaster()->MoveTargetedHome();
                        }
                        break;
                    }
                    case GSCR_RESOLVE:
                    {
                        if (curr->params.c_resolve.resolver_type == GSRT_VECTOR)
                        {
                            // initialize destination variables (so we can be sure they're there)
                            for (int i = 0; i < 3; i++)
                            {
                                auto itr = variable_map.find(curr->params.c_resolve.variable[i]);
                                if (itr == variable_map.end())
                                    variable_map[curr->params.c_resolve.variable[i]] = GS_Variable();
                            }

                            float tmpx, tmpy, tmpz;

                            // angle + distance vector resolver
                            if (curr->params.c_resolve.vector_resolve_type == GSVRT_ANGLE_DISTANCE)
                            {
                                float angle = GS_GetValueFromSpecifier(curr->params.c_resolve.angle).toFloat() * M_PI / 180.0f;
                                float distance = GS_GetValueFromSpecifier(curr->params.c_resolve.distance).toFloat();
                                // base = current position
                                if (curr->params.c_resolve.position_type == GSPTS_RELATIVE_CURRENT)
                                {
                                    GS_CreatureScript::GetPoint2D(tmpx, tmpy, tmpz, source->GetPositionX(), source->GetPositionY(), source->GetPositionZ(),
                                        source->GetObjectSize(), source->GetOrientation() + angle, distance, source->GetBaseMap());
                                }
                                // base = spawn position
                                else if (curr->params.c_resolve.position_type == GSPTS_RELATIVE_SPAWN)
                                {
                                    // only creatures have spawn position
                                    if (Creature* crsource = source->ToCreature())
                                    {
                                        GS_CreatureScript::GetPoint2D(tmpx, tmpy, tmpz, crsource->GetHomePosition().GetPositionX(), crsource->GetHomePosition().GetPositionY(), crsource->GetHomePosition().GetPositionZ(),
                                            source->GetObjectSize(), crsource->GetHomePosition().GetOrientation() + angle, distance, source->GetBaseMap());
                                    }
                                    else
                                    {
                                        // we cannot resolve "spawn" position of not spawned object (i.e. player)
                                        GS_CreatureScript::GetPoint2D(tmpx, tmpy, tmpz, source->GetPositionX(), source->GetPositionY(), source->GetPositionZ(),
                                            source->GetObjectSize(), source->GetOrientation() + angle, distance, source->GetBaseMap());
                                    }
                                }
                                // base = 0;0;0 (may come in handy in more complex scenarios)
                                else if (curr->params.c_resolve.position_type == GSPTS_ABSOLUTE)
                                {
                                    GS_CreatureScript::GetPoint2D(tmpx, tmpy, tmpz, 0.0f, 0.0f, 0.0f,
                                        source->GetObjectSize(), angle, distance, source->GetBaseMap());
                                }
                            }
                            // relative position vector resolver
                            else if (curr->params.c_resolve.vector_resolve_type == GSVRT_RELATIVE_POSITION)
                            {
                                float rel_x = GS_GetValueFromSpecifier(curr->params.c_resolve.rel_x).toFloat();
                                float rel_y = GS_GetValueFromSpecifier(curr->params.c_resolve.rel_y).toFloat();
                                float rel_z = GS_GetValueFromSpecifier(curr->params.c_resolve.rel_z).toFloat();

                                // base = current position
                                if (curr->params.c_resolve.position_type == GSPTS_RELATIVE_CURRENT)
                                {
                                    tmpx = source->GetPositionX() + rel_x;
                                    tmpy = source->GetPositionY() + rel_y;
                                    tmpz = source->GetPositionZ() + rel_z;
                                }
                                // base = spawn position
                                else if (curr->params.c_resolve.position_type == GSPTS_RELATIVE_SPAWN)
                                {
                                    // only creatures have spawn position
                                    if (Creature* crsource = source->ToCreature())
                                    {
                                        tmpx = crsource->GetHomePosition().GetPositionX() + rel_x;
                                        tmpy = crsource->GetHomePosition().GetPositionY() + rel_y;
                                        tmpz = crsource->GetHomePosition().GetPositionZ() + rel_z;
                                    }
                                    else
                                    {
                                        tmpx = source->GetPositionX() + rel_x;
                                        tmpy = source->GetPositionY() + rel_y;
                                        tmpz = source->GetPositionZ() + rel_z;
                                    }
                                }
                                // base = 0;0;0
                                else if (curr->params.c_resolve.position_type == GSPTS_ABSOLUTE)
                                {
                                    // i have no idea why would somebody want that, but this language should be complete in that way
                                    tmpx = rel_x;
                                    tmpy = rel_y;
                                    tmpz = rel_z;
                                }
                            }

                            // set variables to resolved values
                            variable_map[curr->params.c_resolve.variable[0]].value.asFloat = tmpx;
                            variable_map[curr->params.c_resolve.variable[0]].type = GSVTYPE_FLOAT;
                            variable_map[curr->params.c_resolve.variable[1]].value.asFloat = tmpy;
                            variable_map[curr->params.c_resolve.variable[1]].type = GSVTYPE_FLOAT;
                            variable_map[curr->params.c_resolve.variable[2]].value.asFloat = tmpz;
                            variable_map[curr->params.c_resolve.variable[2]].type = GSVTYPE_FLOAT;
                        }
                        break;
                    }
                    case GSCR_PHASE:
                    {
                        int32 mask = GS_GetValueFromSpecifier(curr->params.c_phase.phase_mask).toInteger();
                        if (mask > 0)
                            source->SetPhaseMask(mask, true);
                        else
                            source->SetPhaseMask(PHASEMASK_NORMAL, true);
                        break;
                    }
                    default:
                    case GSCR_NONE:
                        break;
                    }
                }

                bool command_incremented = false;

                // if not explicitly waiting, move counter to next instruction
                if (!is_waiting && !lock_move_counter && curr->type != GSCR_END_EVENT)
                {
                    command_incremented = true;
                    com_counter++;
                }

                // If we are in event hook -> ignore wait condition
                if (!command_incremented && is_in_event_handler && !lock_move_counter && curr->type != GSCR_END_EVENT)
                    com_counter++;

                // if not explicitly disabled melee attack, and is not waiting or has appropriate flag to attack during waiting,
                // proceed melee attack
                if (!disable_melee && (!is_waiting || (wait_flags & GSWF_MELEE_ATTACK)))
                    DoMeleeAttackIfReady();
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
