#include "ScriptPCH.h"
#include "GScript\GSCommands.h"

class GS_CreatureScript : public CreatureScript
{
    public:

        GS_CreatureScript() : CreatureScript("gscript")
        {
            //
        }

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

            // flag for disabling melee combat
            bool disable_melee;

            // state flag for movement
            bool is_moving;

            // stored faction in case of faction change
            uint32 stored_faction = 0;
            // stored model id in case of morph
            uint32 stored_modelid = 0;

            // map of all timers
            std::map<int, uint32> timer_map;

            GS_ScriptedAI(Creature* cr) : ScriptedAI(cr)
            {
                my_commands = nullptr;

                GS_LoadMyScript();

                // ordinary reset
                com_counter = 0;
                is_waiting = false;
                disable_melee = false;
                is_moving = false;
            }

            void GS_LoadMyScript()
            {
                if (my_commands)
                {
                    for (auto itr = my_commands->begin(); itr != my_commands->end(); ++itr)
                        delete *itr;
                    delete my_commands;
                }
                my_commands = nullptr;

                // select everything for this creature from database
                QueryResult res = ScriptDatabase.PQuery("SELECT script_type, script FROM creature_gscript WHERE creature_entry = %u", me->GetEntry());

                // retrieve string to be parsed
                std::vector<std::string> toparse;
                if (res)
                {
                    Field* f = res->Fetch();
                    if (f)
                    {
                        // cut the input string by lines

                        std::stringstream ss(f[1].GetCString());
                        std::string to;

                        while (std::getline(ss, to, '\n'))
                            toparse.push_back(std::string(to));
                    }
                }

                // if there was something at input...
                if (toparse.size() > 0)
                {
                    // ...parse script into tokens...
                    CommandProtoVector* cpv = gscr_parseInput(toparse);
                    // ...and if succeeded, analyze command sequence and build command vector
                    if (cpv)
                        my_commands = gscr_analyseSequence(cpv);
                }
            }

            void Reset()
            {
                // reset command counter, waiting and melee disable flags
                com_counter = 0;
                is_waiting = false;
                disable_melee = false;
                is_moving = false;

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
                    if ((*itr).second < diff)
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
                    // other non-handled cases - returns null as it's invalid in this context
                    default:
                    case GSST_CHANCE:
                    case GSST_NONE:
                        break;
                }

                return nullptr;
            }

            // retrieves numeric value using specifier supplied
            int32 GS_GetValueFromSpecifier(gs_specifier& spec)
            {
                // if type and parameter were not specified, return one-value
                if (spec.subject_type == GSST_NONE || spec.subject_parameter == GSSP_NONE)
                    return spec.value;

                if (spec.subject_type == GSST_TIMER && spec.subject_parameter == GSSP_IDENTIFIER)
                    return GS_GetTimerState(spec.value);

                if (spec.subject_type == GSST_STATE && spec.subject_parameter == GSSP_STATE_VALUE)
                    return spec.value;

                // there are only two meaningful subject types: me and target,
                // any other target is not so valuable here

                bool isVictim = (spec.subject_type == GSST_TARGET && me->GetVictim());

                switch (spec.subject_parameter)
                {
                    case GSSP_HEALTH:
                        return isVictim ? me->GetVictim()->GetHealth() : me->GetHealth();
                    case GSSP_HEALTH_PCT:
                        return isVictim ? (int32)me->GetVictim()->GetHealthPct() : (int32)me->GetHealthPct();
                    case GSSP_FACTION:
                        return isVictim ? me->GetVictim()->getFaction() : me->getFaction();
                    case GSSP_LEVEL:
                        return isVictim ? me->GetVictim()->getLevel() : me->getLevel();
                    default:
                    case GSSP_NONE:
                        break;
                }

                // fallback to value if no match
                return spec.value;
            }

            bool GS_Meets(gs_command* ifcmd)
            {
                // chance is different, non-generic case
                if (ifcmd->params.c_if.source.subject_type == GSST_CHANCE)
                {
                    if (ifcmd->params.c_if.op == GSOP_OF && ifcmd->params.c_if.dest.value > 0)
                        return urand(0, 100) < (uint32)ifcmd->params.c_if.dest.value;
                    else
                        return false;
                }

                // from now, we support just 3 parameter condition (lefthand, operator, righthand)
                // so get those two values, and then apply operator
                int32 lefthand = GS_GetValueFromSpecifier(ifcmd->params.c_if.source);
                int32 righthand = GS_GetValueFromSpecifier(ifcmd->params.c_if.dest);

                switch (ifcmd->params.c_if.op)
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

            void MovementInform(uint32 type, uint32 id)
            {
                if (id == 100)
                    is_moving = false;
            }

            void UpdateAI(const uint32 diff)
            {
                // do not proceed script, if not in combat, and the script has not yet started/has already finished
                if (!me->IsInCombat() && (com_counter == 0 || com_counter == my_commands->size()))
                    return;

                if (!my_commands || com_counter == my_commands->size())
                {
                    DoMeleeAttackIfReady();
                    return;
                }

                GS_UpdateTimers(diff);

                gs_command* curr = (*my_commands)[com_counter];

                switch (curr->type)
                {
                    case GSCR_LABEL:    // these instructions just marks current offset
                    case GSCR_ENDIF:
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
                        else // just stubled upon wait instruction
                        {
                            // set waiting flag and store waiting parameters
                            is_waiting = true;
                            wait_timer = curr->params.c_wait.delay;
                            wait_flags = curr->params.c_wait.flags;
                        }
                        break;
                    case GSCR_CAST:
                    {
                        Unit* target = GS_SelectTarget(curr->params.c_cast.target_type);
                        me->CastSpell(target, curr->params.c_cast.spell, curr->params.c_cast.triggered);
                        break;
                    }
                    case GSCR_SAY:
                        me->MonsterSay(curr->params.c_say_yell.tosay, LANG_UNIVERSAL, 0);
                        break;
                    case GSCR_YELL:
                        me->MonsterYell(curr->params.c_say_yell.tosay, LANG_UNIVERSAL, 0);
                        break;
                    case GSCR_IF:
                        // if script does not meed condition passed in, move to endif offset
                        if (!GS_Meets(curr))
                            com_counter = curr->params.c_if.endif_offset;
                        break;
                    case GSCR_COMBATSTOP:
                        disable_melee = true;
                        me->AttackStop();
                        me->SetStandState(UNIT_STAND_STATE_STAND);
                        break;
                    case GSCR_FACTION:
                        // if the faction is larger than 0, that means set faction to specified value
                        if (curr->params.c_faction.faction > 0)
                        {
                            // store original faction, if not stored yet
                            if (stored_faction == 0)
                                stored_faction = me->getFaction();
                            me->setFaction(curr->params.c_faction.faction);
                        }
                        // or if we are about to restore original faction and we already stored that value
                        else if (stored_faction != 0)
                        {
                            // restore faction
                            me->setFaction(stored_faction);
                            stored_faction = 0;
                        }
                        break;
                    case GSCR_REACT:
                        me->SetReactState(curr->params.c_react.reactstate);
                        break;
                    case GSCR_KILL:
                        if (Unit* victim = GS_SelectTarget(curr->params.c_kill.target))
                            me->Kill(victim);
                        break;
                    case GSCR_COMBATSTART:
                        disable_melee = false;
                        me->Attack(me->GetVictim(), true);
                        break;
                    case GSCR_TIMER:
                        GS_SetTimer(curr->params.c_timer.timer_id, curr->params.c_timer.value);
                        break;
                    case GSCR_MORPH:
                        if (curr->params.c_morph.morph_id > 0)
                        {
                            if (stored_modelid == 0)
                                stored_modelid = me->GetDisplayId();
                            me->SetDisplayId(curr->params.c_morph.morph_id);
                        }
                        else if (stored_modelid > 0)
                        {
                            me->DeMorph();
                            stored_modelid = 0;
                        }
                    case GSCR_SUMMON:
                    {
                        float x = curr->params.c_summon.x;
                        float y = curr->params.c_summon.y;
                        float z = curr->params.c_summon.z;
                        float o = me->GetOrientation();
                        if (x == 0 && y == 0 && z == 0)
                            me->GetPosition(x, y, z, o);

                        me->SummonCreature(curr->params.c_summon.creature_entry, x, y, z, o, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 15000);
                        break;
                    }
                    case GSCR_WALK:
                        me->SetWalk(true);
                        is_moving = true;
                        me->GetMotionMaster()->MovePoint(100, curr->params.c_walk_run_teleport.x, curr->params.c_walk_run_teleport.y, curr->params.c_walk_run_teleport.z, true);
                        break;
                    case GSCR_RUN:
                        me->SetWalk(false);
                        is_moving = true;
                        me->GetMotionMaster()->MovePoint(100, curr->params.c_walk_run_teleport.x, curr->params.c_walk_run_teleport.y, curr->params.c_walk_run_teleport.z, true);
                        break;
                    case GSCR_TELEPORT:
                        me->NearTeleportTo(curr->params.c_walk_run_teleport.x, curr->params.c_walk_run_teleport.y, curr->params.c_walk_run_teleport.z, me->GetOrientation());
                        break;
                    case GSCR_WAITFOR:
                        is_waiting = true;
                        wait_flags = 0;
                        if (curr->params.c_waitfor.eventtype == GSET_MOVEMENT && !is_moving)
                            is_waiting = false;
                        else if (curr->params.c_waitfor.eventtype == GSET_CAST && !me->IsNonMeleeSpellCasted(false))
                            is_waiting = false;
                        else if (curr->params.c_waitfor.eventtype == GSET_NONE)
                            is_waiting = false;
                        break;
                    default:
                    case GSCR_NONE:
                        break;
                }

                // if not explicitly waiting, move counter to next instruction
                if (!is_waiting)
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
