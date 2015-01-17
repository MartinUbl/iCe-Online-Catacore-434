/*
 * Copyright (C) 2006-2013 iCe Online <http://ice-wow.eu>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptPCH.h"
#include "well_of_eternity.h"

#define CAST_WOE_INSTANCE(i)     (dynamic_cast<instance_well_of_eternity::instance_well_of_eternity_InstanceMapScript*>(i))

static void PlayQuote (Creature * source, SimpleQuote quote, bool yell = false)
{
    source->PlayDirectSound(quote.soundID);

    if (yell)
        source->MonsterYell(quote.text, LANG_UNIVERSAL,0,200.0f);
    else
        source->MonsterSay(quote.text, LANG_UNIVERSAL,0,200.0f);
}

const Position leftEndPositions[2] =
{
    {3260.16f,-4943.4f, 181.7f, 3.7f},     //LEFT
    {3257.5f,-4938.6f, 181.7f, 3.7f},     //RIGHT
};

const Position rightEndPositions[2] =
{
    {3411.3f,-4841.3f, 181.7f, 0.65f},     //LEFT
    {3413.7f,-4845.3f, 181.7f, 0.65f},     //RIGHT
};

namespace Illidan
{
    static const SimpleQuote explainQuotes[3] = // used all
    {
        {26080, "Explain your presence." },
        {26523, "Talk with me now." },
        {26524, "Are you here to help?" },
    };

    static const SimpleQuote introQuotes[3] = // used all
    {
        {26076, "Over here! In the shadows!" },
        {26054, "We now hide in shadows, hidden from our enemies." },
        {26525, "I think we stand a better chance fighting along side one another"}
    };

    static const SimpleQuote distractQuote[2] = // used all
    {
        {26063, "I'll hold them back so we get past. Be ready." },
        {26064, "My magic is fading. I'm going through!" }
    };

    static const SimpleQuote escortQuotes[11] =
    {
        {26065, "Come with me if you like to live long enough to see me save this world!" },
        // random from these 2
        {26066, "So many demons. Not enough time." },
        {26068, "I've seen a Guardian Demon slaughter a hundred elves. Tread lightly." },
        // random from these 2
        {26069, "They come endlessly from the palace." },
        {26067, "They will get what they deserve. In due time." },

        {26070, "The stench of sulfur and brimstone. These portals are as foul as the demons themselves." },
        {26071, "Cut this one down from the shadows." },
        {26072, "Let us shut down this final portal and finish this." },
        {26073, "The demons should be leaving. We'll be in the palace in no time." },
        {26074, "The demons are no longer pouring from the palace. We can move ahead." },
        {26075, "Too easy." }
    };

    static const SimpleQuote onEventStart [2] =
    {
        {26050, "Another demon ready to be slaughtered" },
        {26127, "Who shut down the portals!? Clever. Little worms."},
    }; 

    static const SimpleQuote waitingAttack[4] = // used all
    {
        {26100, "Waitng to attack." },
        {26081, "Attack, I don't like to be kept waiting." },
        {26082, "I'll let you have the first kill. Don't make me regret that." },
        {26083, "Patience is not one of my virtues." }
    };

    static const SimpleQuote onAggro[3] = // used all
    {
        {26056, "Death to the Legion!" },
        {26057, "My blades hunger." },
        {26058, "This won't hurt a bit." }
    };

    static const SimpleQuote crystalOrder[4] =
    {
        {26106, "Shut down the portal, and we'll continue." },
        {26103, "Destroy the crystal so we can move on." },
        {26105, "Destroy the portal energy focus!" },
        {26104, "Smash the crystal. We need to move." }
    };
    static const SimpleQuote onLeaving = {26055, "Were leaving, stay close."};

    const char * GOSSIP_ITEM_ILLIDAN_1 = "I am ready to be hidden by your shadowcloak.";
    const char * GOSSIP_ITEM_ILLIDAN_2 = "Let's go!";

    #define ILLIDAN_GOSSIP_MENU_TEXT_ID_1 555001
    #define ILLIDAN_GOSSIP_MENU_TEXT_ID_2 555002
}

namespace GuardianDemon
{
    static const SimpleQuote portalClosing[2] =
    {
        {26037, "The portal is closing! Hurry!" },
        {26038, "There! Go! GO!!!" }
    };

    static const SimpleQuote onKill[3] =
    {
        {26040, "Their fate is sealed." },
        {26039, "Such fragile creatures. Worthless!" },
        {26041, "Leave nothing but smoldering husks."}
    };
}

class Legion_demon_WoE : public CreatureScript
{
public:
    Legion_demon_WoE() : CreatureScript("legion_demon_WoE") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new Legion_demon_WoEAI(creature);
    }

    struct Legion_demon_WoEAI : public ScriptedAI
    {
        Legion_demon_WoEAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = me->GetInstanceScript();
            canMoveToNextPoint = false;
            me->SetWalk(true);
            me->SetSpeed(MOVE_WALK, 1.28571f, true);
            me->ForcedDespawn(5 * MINUTE * IN_MILLISECONDS); // default
        }

        InstanceScript * pInstance;
        DemonDirection direction;
        DemonWave waveNumber;
        bool canMoveToNextPoint;
        uint32 distractionTimer;
        uint32 guardMoveTimer;
        uint32 waitTimer;

        void CheckPortalStatus()
        {
            if (CAST_WOE_INSTANCE(pInstance)->IsPortalClosing(direction))
            {
                if (Creature * pGaurd = CAST_WOE_INSTANCE(pInstance)->GetGuardianByDirection(direction))
                    pGaurd->AI()->DoAction(ACTION_PORTAL_CLOSING);

                if (Creature * pPortalDummy = me->FindNearestCreature(LEGION_PORTAL_DUMMY,50.0f,true))
                {
                    // Prevent multiple application of auras from rest demons
                    if (pPortalDummy->HasAura(SPELL_PORTAL_STATUS_SHUTTING_DOWN))
                        return;

                    // We need to cast it, beacause it will also remove active aura
                    pPortalDummy->CastSpell(pPortalDummy, SPELL_PORTAL_STATUS_SHUTTING_DOWN, true);

                    if (Aura * portalAura = pPortalDummy->GetAura(SPELL_PORTAL_STATUS_SHUTTING_DOWN))
                        portalAura->SetDuration(60000);
                }
            }
        }

        void SetData(uint32 type, uint32 data)  override
        {
            if (type == DEMON_DATA_DIRECTION)
            {
                direction = (DemonDirection)data;
            }
            else if (type == DEMON_DATA_WAVE)
            {
                waveNumber = (DemonWave)data;
            }
        }

        void MovementInform(uint32 type, uint32 id)  override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == WP_END)
            {
                if (pInstance)
                    CheckPortalStatus();

                me->ForcedDespawn();
                return;
            }

            switch (direction)
            {
                case DIRECTION_LEFT: // Going to Left Corner
                {
                    if (waveNumber == WAVE_ONE)
                        waitTimer = 1900;
                    else if (waveNumber == WAVE_TWO)
                        canMoveToNextPoint = true;

                    break;
                }
                case DIRECTION_RIGHT: // Going to Right Corner
                {
                    if (waveNumber == WAVE_ONE)
                        waitTimer = 1900;
                    else if (waveNumber == WAVE_TWO)
                        canMoveToNextPoint = true;
                    break;
                }
                case DIRECTION_STRAIGHT: // Just go straight till end
                {
                    break;
                }
            }
        }

        void Reset()  override
        {
            me->SetReactState(REACT_AGGRESSIVE);
            waitTimer = MAX_TIMER;
            guardMoveTimer = MAX_TIMER;
            distractionTimer = 10000;
        }

        void EnterEvadeMode()  override
        {
            // Nah ...
            me->ForcedDespawn();
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (me->GetExactDist2d(who) > 5.0f || who->GetTypeId() != TYPEID_PLAYER)
                return;

            ScriptedAI::MoveInLineOfSight(who);
        }

        void UpdateAI(const uint32 diff)  override
        {
            if (!pInstance)
                return;

            if (distractionTimer <= diff)
            {
                if (Creature * stalker = me->FindNearestCreature(DISTRACION_STALKER_ENTRY,10.0f,true))
                    if (stalker->HasAura(SPELL_DISTRACTION_AURA))
                    {
                        me->ForcedDespawn();
                        return;
                    }
                distractionTimer = 500;
            }
            else distractionTimer -= diff;

            if (canMoveToNextPoint)
            {
                canMoveToNextPoint = false;

                if (direction == DIRECTION_LEFT)
                    me->GetMotionMaster()->MovePoint(WP_END, leftEndPositions[DIRECTION_RIGHT],true);
                else
                    me->GetMotionMaster()->MovePoint(WP_END, rightEndPositions[DIRECTION_RIGHT],true);
            }

            // Second demon in line must wait for another demon , because his path is a bit longer
            if (waitTimer <= diff)
            {
                waitTimer = MAX_TIMER;

                if (direction == DIRECTION_STRAIGHT || waveNumber == WAVE_THREE)
                    return;

                if (direction == DIRECTION_LEFT)
                    me->GetMotionMaster()->MovePoint(WP_END, leftEndPositions[DIRECTION_LEFT],true);
                else
                    me->GetMotionMaster()->MovePoint(WP_END, rightEndPositions[DIRECTION_LEFT],true);
            }
            else waitTimer -= diff;

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady(); // Probably only beat the shit out of players with bare hands
        }
    };
};

class fel_crystal_stalker_woe : public CreatureScript
{
public:
    fel_crystal_stalker_woe() : CreatureScript("fel_crystal_stalker_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new fel_crystal_stalker_woeAI(creature);
    }

    struct fel_crystal_stalker_woeAI : public ScriptedAI
    {
        fel_crystal_stalker_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = me->GetInstanceScript();
            me->SetDisplayId(17612); // invis model
            calm = true;
        }

        uint32 GetConnectorEntryByXCoord(uint32 xCoord)
        {
            switch (xCoord)
            {
                case FIRST_CRYSTAL_X_COORD:
                    return PORTAL_CONNECTOR_1_ENTRY;
                case SECOND_CRYSTAL_X_COORD:
                    return PORTAL_CONNECTOR_2_ENTRY;
                case THIRD_CRYSTAL_X_COORD:
                    return PORTAL_CONNECTOR_3_ENTRY;
            }
            return 0;
        }

        InstanceScript * pInstance;
        uint32 beamTimer;
        uint32 colapseTimer;
        bool beamCasted;
        bool colapsed;
        bool calm;

        void DoAction(const int32 action)  override
        {
            if (action == DATA_CRYSTAL_DESTROYED)
            {
                me->CastSpell(me, SPELL_CRYSTAL_MELTDOWN, false);
                me->RemoveAura(SPELL_CRYSTAL_PERIODIC);
                calm = false;
                colapseTimer = 3500;

                if (Creature * pIllidan = me->FindNearestCreature(ILLIDAN_STORMRAGE_ENTRY, 200.0f, true))
                {
                    pIllidan->AI()->DoAction(ACTION_ILLIDAN_DELAY_MOVE);
                    PlayQuote(pIllidan, Illidan::onLeaving);
                }
            }
        }

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
            me->CastSpell(me, SPELL_CRYSTAL_PERIODIC, true);
            me->CastSpell(me, SPELL_FEL_CRYSTAL_STALKER_GLOW, true);
            beamTimer = 2000;
            colapsed = false;
            beamCasted = false;
        }

        void UpdateAI(const uint32 diff) override
        {
            if (beamCasted == false && beamTimer <= diff)
            {
                beamCasted = true;
                uint32 entry = GetConnectorEntryByXCoord(me->GetPositionX());
                if (Creature * pNearConnector = me->FindNearestCreature(entry, 20.0f, true))
                    me->CastSpell(pNearConnector, SPELL_PORTAL_BEAM_CONNECTOR_GREEN, false);
            }
            else beamTimer -= diff;

            if (colapsed || calm || !pInstance)
                return;

            if (colapseTimer <= diff)
            {
                me->CastSpell(me, SPELL_CRYSTAL_DESTRUCTION, true);
                me->CastSpell(me, SPELL_CRYSTAL_FEL_GROUND, true);

                if (GameObject * goCrystal = me->FindNearestGameObject(PORTAL_ENERGY_FOCUS_ENTRY, 20.0f))
                    goCrystal->Delete();
                // This will trigger die anim at crystals and freeze anim after 1.5s cca -> exactly what we need
                me->CastSpell(me, SPELL_SHATTER_CRYSTALS , true);

                if (Creature * pGuardian = me->FindNearestCreature(GUARDIAN_DEMON_ENTRY, 200.0f, true))
                {
                    pGuardian->AI()->DoAction(ACTION_PORTAL_HURRY);
                    PlayQuote(pGuardian, GuardianDemon::portalClosing[1],true);
                }

                uint32 connectorEntry = GetConnectorEntryByXCoord(me->GetPositionX());

                // Turn off connectors
                CAST_WOE_INSTANCE(pInstance)->TurnOffConnectors(connectorEntry, me);
                CAST_WOE_INSTANCE(pInstance)->CrystalDestroyed(me->GetPositionX());

                // Remove visual auras
                me->RemoveAura(SPELL_FEL_CRYSTAL_STALKER_GLOW);
                colapsed = true;
            }
            else colapseTimer -= diff;
        }
    };
};

class npc_woe_generic : public CreatureScript
{
public:
    npc_woe_generic() : CreatureScript("npc_woe_generic") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_woe_genericAI(creature);
    }

    struct npc_woe_genericAI : public ScriptedAI
    {
        npc_woe_genericAI(Creature* creature) : ScriptedAI(creature)
        {
            entry = me->GetEntry();
            pInstance = me->GetInstanceScript();
            portalClosed = false;

            if ((entry == CORRUPTED_ARCANIST_ENTRY || entry == DREADLORD_DEFFENDER_ENTRY) && me->ToTempSummon())
            {
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            }
        }

        uint32 entry;
        InstanceScript * pInstance;

        // guardian demon
        uint32 gripTimer;
        uint32 finalWalkTimer;
        uint32 hurryTimer;
        uint32 lane;
        bool portalClosed;
        bool firstWP;
        bool textSaid;
        // legion demon
        bool canMoveToPoint;
        uint32 leapTimer;
        uint32 strikeTimer;
        // arcanist
        uint32 arcaneTimer;
        // dreadlord
        uint32 swarmTimer;
        uint32 wardingTimer;

        void Reset() override
        {
            finalWalkTimer = MAX_TIMER;
            hurryTimer = MAX_TIMER;
            lane = 0;

            if (entry == GUARDIAN_DEMON_ENTRY)
                me->CastSpell(me, 90766, true); // Hovering Anim State

            firstWP = true;
            canMoveToPoint = false;
            textSaid = false;
            gripTimer = 4000;
            leapTimer = 4000;
            strikeTimer = 5000;

            arcaneTimer = 2000;

            swarmTimer = urand(2000,5000);
            wardingTimer = 1000;
        }

        void SetData(uint32 type, uint32 data) override
        {
            if (type == DATA_SET_GUARDIAN_LANE)
                lane = data;
        }

        uint32 GetData(uint32 type) override
        {
            if (type == DATA_GET_GUARDIAN_LANE && entry == GUARDIAN_DEMON_ENTRY)
                return lane;

            return 0;
        }

        void DoAction(const int32 action) override
        {
            if (action == ACTION_PORTAL_CLOSING && portalClosed == false)
            {
                portalClosed = true;
                finalWalkTimer = 25000;
            }

            if (action == ACTION_PORTAL_HURRY)
                hurryTimer = 5000;
        }

        void SpellHit(Unit*, const SpellEntry* pSpell) override
        {
            if (pSpell->Id == SPELL_ARCANE_ANNIHILATION)
                arcaneTimer = 500;
        }

        void SpellHitTarget(Unit *pTarget,const SpellEntry* spell) override
        {
            if (spell->Id == SPELL_SUMMON_FIREWALL_DUMMY)
            {
                me->SummonGameObject(FIREWALL_ENTRY, 3193.9f, -4931.0f, 189.6f, 1.1f, 0, 0, 0, 0, 0);

                if (GameObject * pGo = me->FindNearestGameObject(FIREWALL_INVIS_ENTRY, 200.0f))
                    pGo->Delete();
            }

            if (spell->Id == SPELL_DEMON_GRIP && pTarget->GetTypeId() == TYPEID_PLAYER)
                pTarget->CastSpell(pTarget, SPELL_DEMON_GRIP_ROOT, true); // must root manualy
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (me->ToTempSummon() == nullptr)
                return;

            if (id == 2 && entry == GUARDIAN_DEMON_ENTRY)
                finalWalkTimer = 1; // next update tick

            if (id == 3 && entry == GUARDIAN_DEMON_ENTRY)
            {
                if (Creature * pPortalDummy = me->FindNearestCreature(LEGION_PORTAL_DUMMY,60.0f,true))
                if (Aura * a = pPortalDummy->GetAura(SPELL_PORTAL_STATUS_SHUTTING_DOWN))
                    a->SetDuration(4000);
                me->ForcedDespawn();
            }

            if (id == 1 && entry == LEGION_DEMON_ENTRY)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_AGGRESSIVE);
                me->CastSpell(me, SPELL_SUMMON_FIREWALL_DUMMY, false);
                me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
            }

            if (id != 0)
                return;

            if (entry == CORRUPTED_ARCANIST_ENTRY || DREADLORD_DEFFENDER_ENTRY)
                canMoveToPoint = true;
        }

        void JustDied(Unit * killer) override
        {
            if (entry == LEGION_DEMON_ENTRY)
            {
                if (GameObject * pGo = me->FindNearestGameObject(FIREWALL_ENTRY, 200.0f))
                    pGo->Delete();

                if (me->ToTempSummon() == NULL && pInstance)
                {
                    if (Creature * pIllidan = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_ILLIDAN)))
                        pIllidan->AI()->DoAction(ACTION_ILLIDAN_DELAY_MOVE);
                }
            }

            switch (entry)
            {
                case CORRUPTED_ARCANIST_ENTRY:
                case DREADLORD_DEFFENDER_ENTRY:
                {
                    if (pInstance)
                    {
                        CAST_WOE_INSTANCE(pInstance)->OnDeffenderDeath(me);
                        CAST_WOE_INSTANCE(pInstance)->UnRegisterIllidanVictim(me->GetGUID());
                    }
                    break;
                }
            }
        }

        void KilledUnit(Unit * victim) override
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                if (entry == GUARDIAN_DEMON_ENTRY)
                    PlayQuote(me, GuardianDemon::onKill[urand(0, 2)]);
        }

        void EnterEvadeMode() override
        {
            if (!pInstance)
                return;

            if (entry == CORRUPTED_ARCANIST_ENTRY || entry == DREADLORD_DEFFENDER_ENTRY)
                CAST_WOE_INSTANCE(pInstance)->UnRegisterIllidanVictim(me->GetGUID());

            if (CAST_WOE_INSTANCE(pInstance)->IsPortalClosing((DemonDirection)lane) && entry == GUARDIAN_DEMON_ENTRY)
                me->ForcedDespawn();

            ScriptedAI::EnterEvadeMode();
        }

        void EnterCombat(Unit * /*who*/) override
        {
            if (pInstance && (entry == CORRUPTED_ARCANIST_ENTRY || entry == DREADLORD_DEFFENDER_ENTRY))
            {
                // Remove stealth auras
                pInstance->DoRemoveAurasDueToSpellOnPlayers(102994); // TODO: is this safe ? -> we should kill vehicle ...
                CAST_WOE_INSTANCE(pInstance)->RegisterIllidanVictim(me->GetGUID());

                if (Creature * pIllidan = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_ILLIDAN)))
                {
                    if (textSaid == false)
                    {
                        textSaid = true;
                        PlayQuote(pIllidan, Illidan::onAggro[urand(0, 2)]);
                    }
                }
            }
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (entry == GUARDIAN_DEMON_ENTRY)
            if (me->GetExactDist2d(who) > 5.0f || who->GetTypeId() != TYPEID_PLAYER)
                return;

            ScriptedAI::MoveInLineOfSight(who);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (canMoveToPoint)
            {
                canMoveToPoint = false;
                if (entry == CORRUPTED_ARCANIST_ENTRY || entry == DREADLORD_DEFFENDER_ENTRY)
                {
                    me->GetMotionMaster()->MovePoint(1, COURTYARD_X,COURTYARD_Y, COURTYARD_Z, true);
                    me->ForcedDespawn(5000);
                }
            }

            if (finalWalkTimer <= diff) // GUARDIAN_DEMON_ENTRY only 
            {
                if (firstWP)
                {
                    firstWP = false;
                    me->SetWalk(true);
                    me->SetSpeed(MOVE_WALK, 3.0f, true);

                    me->GetMotionMaster()->MovePoint(2, guardPos[(uint32)lane].first,true);
                    finalWalkTimer = MAX_TIMER; // set in movement inform, when WP2 is reached
                    return;
                }

                me->GetMotionMaster()->MovePoint(3, guardPos[(uint32)lane].last,true);
                finalWalkTimer = MAX_TIMER;
            }
            else finalWalkTimer -= diff;

            // GUARDIAN DEMON TIMER
            if (hurryTimer <= diff)
            {
                PlayQuote(me, GuardianDemon::portalClosing[0], true);
                hurryTimer = MAX_TIMER;
            }
            else hurryTimer -= diff;

            if (!UpdateVictim())
                return;

            switch (entry)
            {
                case LEGION_DEMON_ENTRY:
                {
                    if (leapTimer <= diff)
                    { 
                        if (!me->IsNonMeleeSpellCasted(false))
                        {
                            if (Unit * player = SelectTarget(SELECT_TARGET_RANDOM,0,40.0f,true))
                            {
                                me->CastSpell(player, SPELL_CRUSHING_LEAP_JUMP, true);
                                //me->CastSpell(player, SPELL_CRUSHING_LEAP_DAMAGE, true);
                                me->CastSpell(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), SPELL_CRUSHING_LEAP_DAMAGE, true);
                            }
                            leapTimer = 8000;
                        }
                    }
                    else leapTimer -= diff;

                    if (strikeTimer <= diff)
                    {
                        if (!me->HasUnitState(UNIT_STATE_JUMPING))
                        {
                            me->CastSpell(me, SPELL_STRIKE_FEAR, false);
                            strikeTimer = urand(6000,7000);
                        }
                    }
                    else strikeTimer -= diff;
                }
                break;
                case GUARDIAN_DEMON_ENTRY:
                {
                    if (gripTimer <= diff)
                    {
                        if (!me->IsNonMeleeSpellCasted(false))
                        {
                            if (urand(0,100) < 50)
                            {
                                me->CastSpell(me, SPELL_DEMON_GRIP, false); // TODO: not triggering 105720 ?
                                // me->CastSpell(me, SPELL_DEMON_GRIP_ROOT, true);
                            }
                            else 
                                me->CastSpell(me, SPELL_INCINERATE, false);
                        }
                        gripTimer = 7000;
                    }
                    else gripTimer -= diff;
                }
                break;
                case CORRUPTED_ARCANIST_ENTRY:
                {
                    if (arcaneTimer <= diff)
                    {
                        if (!me->IsNonMeleeSpellCasted(false))
                        {
                            if (me->GetVictim())
                                me->CastSpell(me->GetVictim(), SPELL_ARCANE_ANNIHILATION, false);
                            // timer set in SpellHit -> 500 ms delay after finish casting of previous one
                            // canot simply set timer to cast time of spell + 500 ms because spell is triggering casting speed stack aura on caster
                            arcaneTimer = MAX_TIMER;
                        }
                    }
                    else arcaneTimer -= diff;

                    const SpellEntry * spellProto = GetSpellStore()->LookupEntry(SPELL_ARCANE_ANNIHILATION);
                    uint32 manaCost = CalculatePowerCost(spellProto, me, GetSpellSchoolMask(spellProto));
                    if (manaCost >= me->GetPower(POWER_MANA) && !me->IsNonMeleeSpellCasted(false))
                    {
                        me->CastSpell(me, SPELL_INFINITE_MANA, false);
                        arcaneTimer = 5500;
                    }
                }
                break;
                case DREADLORD_DEFFENDER_ENTRY:
                {
                    if (swarmTimer <= diff)
                    {
                        if (!me->IsNonMeleeSpellCasted(false))
                        {
                            if (me->GetVictim())
                                me->CastSpell(me->GetVictim(), SPELL_CARRION_SWARM, false);
                            swarmTimer = urand(8000,13000);
                        }
                    }
                    else swarmTimer -= diff;

                    if (wardingTimer <= diff)
                    {
                        if (!me->IsNonMeleeSpellCasted(false))
                        {
                            if (Creature * pArcanist = me->FindNearestCreature(CORRUPTED_ARCANIST_ENTRY,40.0f,true))
                            if (Creature * pDeffender = me->FindNearestCreature(DREADLORD_DEFFENDER_ENTRY,40.0f,true))
                            if (!pDeffender->IsNonMeleeSpellCasted(false))
                                me->CastSpell(pArcanist,SPELL_DEMONIC_WARDING, false);
                            wardingTimer = urand(9000,12000);
                        }
                    }
                    else wardingTimer -= diff;
                }
                break;

            }

            DoMeleeAttackIfReady();
        }
    };
};

class npc_portal_connector : public CreatureScript
{
public:
    npc_portal_connector() : CreatureScript("npc_portal_connector") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_portal_connectorAI(creature);
    }

    struct npc_portal_connectorAI : public ScriptedAI
    {
        npc_portal_connectorAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_DISABLE_MOVE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->CastSpell(me, SPELL_FEL_CRYSTAL_CONNECTOR_COSMETIC, true);
        }

        uint32 beamTimer;
        bool connected;

        void Reset() override
        {
            beamTimer = 5000;
            connected = false;
        }

        void UpdateAI(const uint32 diff) override
        {
            if (connected)
                return;

            if (beamTimer <= diff)
            {
                connected = true;

                std::list<Creature*> connectors;
                me->GetCreatureListWithEntryInGrid(connectors, me->GetEntry(), 30.0f);
                connectors.sort(Trinity::ObjectDistanceOrderPred(me));

                for (std::list<Creature*>::iterator itr = connectors.begin(); itr != connectors.end(); itr++)
                {
                    if (*itr)
                        if (me->HasInArc(M_PI,*itr) && (*itr) != me)
                        {
                            me->CastSpell(*itr, SPELL_PORTAL_BEAM_CONNECTOR_GREEN, false);
                            break;
                        }
                }
            }
            else beamTimer -= diff;
        }
    };
};

class go_fel_crystal_woe : public GameObjectScript
{
public:
    go_fel_crystal_woe() : GameObjectScript("go_fel_crystal_woe") { }

    bool OnGossipHello(Player *pPlayer, GameObject * pGO)
    {
        if (!pPlayer || pPlayer->IsInCombat())
            return false;

        Creature  * pStalker = pPlayer->FindNearestCreature(FEL_CRYSTAL_STALKER_ENTRY, 35.0f, true);
        Creature  * pArcanist = pPlayer->FindNearestCreature(CORRUPTED_ARCANIST_ENTRY, 35.0f, true);
        Creature  * pDreadLord = pPlayer->FindNearestCreature(DREADLORD_DEFFENDER_ENTRY, 35.0f, true);
        InstanceScript  * pInstScript = pPlayer->GetInstanceScript();

        if (pStalker && !pArcanist && !pDreadLord && pInstScript->GetData(DATA_PEROTHARN) != DONE)
        {
            pStalker->AI()->DoAction(DATA_CRYSTAL_DESTROYED);
            pGO->SetFlag(GAMEOBJECT_FLAGS,GO_FLAG_NOT_SELECTABLE);
        }
        return true;
    }
};

// 105018 + 105074 + 105004
class spell_gen_woe_crystal_selector : public SpellScriptLoader
{
    public:
        spell_gen_woe_crystal_selector() : SpellScriptLoader("spell_gen_woe_crystal_selector") { }

        class spell_gen_woe_crystal_selector_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_woe_crystal_selector_SpellScript);

            void FilterTargets(std::list<Unit*>& unitList)
            {
                unitList.clear();
                std::list<Creature*> crystals;
                GetCreatureListWithEntryInGrid(crystals, GetCaster(), FEL_CRYSTAL_ENTRY, 20.0f);
                for (std::list<Creature*>::const_iterator itr = crystals.begin(); itr != crystals.end(); ++itr)
                    if (*itr)
                        unitList.push_back(*itr);
            }

            void Register() override
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_woe_crystal_selector_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_AREA_ENTRY_DST);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_woe_crystal_selector_SpellScript();
        }
};

class spell_gen_woe_distract_selector : public SpellScriptLoader
{
    public:
        spell_gen_woe_distract_selector() : SpellScriptLoader("spell_gen_woe_distract_selector") { }

        class spell_gen_woe_distract_selector_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_woe_distract_selector_SpellScript);

            void FilterTargets(std::list<Unit*>& unitList)
            {
                unitList.clear();
                std::list<Creature*> crystals;
                GetCreatureListWithEntryInGrid(crystals, GetCaster(), DISTRACION_STALKER_ENTRY, 100.0f);
                unitList.sort(Trinity::ObjectDistanceOrderPred(GetCaster()));

                for (std::list<Creature*>::const_iterator itr = crystals.begin(); itr != crystals.end(); ++itr)
                    if (!(*itr)->HasAura(SPELL_DISTRACTION_AURA))
                    {
                        (*itr)->CastSpell((*itr), SPELL_DISTRACTION_AURA, true);

                        if (Aura * aura = (*itr)->GetAura(SPELL_DISTRACTION_AURA))
                            aura->SetDuration(15000);

                        unitList.push_back(*itr);
                        break;
                    }
            }

            void Register() override
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_woe_distract_selector_SpellScript::FilterTargets, EFFECT_0, TARGET_DST_NEARBY_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_woe_distract_selector_SpellScript();
        }
};

class npc_legion_portal_woe : public CreatureScript
{
public:
    npc_legion_portal_woe() : CreatureScript("npc_legion_portal_woe") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_legion_portal_woeAI(pCreature);
    }

    struct npc_legion_portal_woeAI : public Scripted_NoMovementAI
    {
        npc_legion_portal_woeAI(Creature* pCreature) : Scripted_NoMovementAI(pCreature)
        {
            pInstance = me->GetInstanceScript();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);
        }

        InstanceScript * pInstance;

        void Reset() override
        {
            if (!pInstance)
                return;

            if (pInstance->GetData(DATA_PEROTHARN) == NOT_STARTED)
                me->CastSpell(me,SPELL_PORTAL_STATUS_ACTIVE,true);
        }

        void UpdateAI(const uint32 /*uiDiff*/) override
        {
            if (!UpdateVictim())
                return;
        }
    };

};

// BASIC TEMPLATE FOR CREATURE SCRIPTS
/*class Legion_demon_WoE : public CreatureScript
{
public:
    Legion_demon_WoE() : CreatureScript("Legion_demon_WoE") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new Legion_demon_WoEAI(creature);
    }

    struct Legion_demon_WoEAI : public ScriptedAI
    {
        Legion_demon_WoEAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = me->GetInstanceScript();
        }

        InstanceScript * pInstance;

        void Reset() override
        {
        }

        void UpdateAI(const uint32 diff) override
        {
        }
    };
};*/

namespace Illidan
{
    static const SimpleQuote afterPerotharnDeathQuote[2] = // used all
    {
        { 26051, "The hunter became the prey." },
        { 26052, "You did well, but for now I must continue alone. Good hunting." }
    };

    enum IllidanSpells
    {
        SPELL_ILLIDAN_GREEN_ARROW = 105924,
        SPELL_SHADOW_CLOAK_ILLIDAN = 105915,    // good one ?
        SPELL_SHADOW_CLOAK_CIRCLE = 102951,      // visual circle
        SPELL_DISTRACT_MISSILE_AURA = 110100,

        // Shadow system on players
        SPELL_SHADOW_WALK_STEALTH = 102994, // stealth + speed + SPELL_AURA_SET_VEHICLE_ID -> misc value 1763)
        SPELL_SHADOW_CLOAK_TRIGGERER = 103004, // Triggering two spells every second (THESE SHOULD TRIGGER SHADOW WALK OUT OF COMBAT)
        SPELL_SHADOW_WALK_STACK_AURA = 103020, // just dummy stacking aura
        SPELL_SHADOW_CLOAK_FAKE_STEALTH = 105915,
        SPELL_SHADOW_WALK_ILLIDAN_KIT = 105915
    };

    class npc_illidan_intro_woe : public CreatureScript
    {
        public:
            npc_illidan_intro_woe() : CreatureScript("npc_illidan_intro_woe") { }
        struct npc_illidan_intro_woeAI : public ScriptedAI
        {
            npc_illidan_intro_woeAI(Creature* c) : ScriptedAI(c)
            {
                pInstance = c->GetInstanceScript();
                ResetEvent(true);
                playerSpotted = false;
            }

        // Call this function with false parameter in case of wipe
        void ResetEvent(bool init)
        {
            if (init)
            {
                me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                me->CastSpell(me, SPELL_ILLIDAN_GREEN_ARROW, true);
                me->CastSpell(me, SPELL_SHADOW_CLOAK_ILLIDAN, true);
            }
            else
            {
                me->SetHomePosition(illidanPos[ILLIDAN_FIRST_STEP]);
                me->AI()->EnterEvadeMode();
                me->GetMotionMaster()->MovePoint(0, illidanPos[ILLIDAN_FIRST_STEP],true);
            }
            if (pInstance)
                CAST_WOE_INSTANCE(pInstance)->SetIllidanStep(ILLIDAN_STEP_NULL);

            attackTimer = 2000;
            eventTimer = MAX_TIMER;
            explainTimer = MAX_TIMER;
            delayMoveTimer = MAX_TIMER;
            orderTimer = MAX_TIMER;
            waitingTimer = MAX_TIMER;
            sayDelayTimer = MAX_TIMER;
            explainCounter = 0;
            eventStarted = false;
            gossipDone = false;
            specialAction = SPECIAL_ACTION_NONE;
            me->SetWalk(true);
            me->SetSpeed(MOVE_WALK, 3.0f, true);
        }

        InstanceScript * pInstance;
        SpecialActions specialAction;
        uint32 eventTimer;
        uint32 explainTimer;
        uint32 delayMoveTimer;
        uint32 waitingTimer;
        uint32 orderTimer;
        uint8 explainCounter;
        uint32 attackTimer;
        uint32 sayDelayTimer;
        bool weaponSwitcher;
        bool eventStarted;
        bool gossipDone;
        bool playerSpotted;

        void Reset() override
        {
        }

        void EnterEvadeMode() override
        {
            me->RemoveAllAuras();
            me->DeleteThreatList();
            me->CombatStop(true);
        }
        void EnterCombat(Unit * /*who*/) override {}

        void MoveInLineOfSight(Unit* who) override
        {
            if (playerSpotted == true)
                return;

            if (who->GetTypeId() == TYPEID_PLAYER && me->GetDistance(who) < 40.0f)
            {
                playerSpotted = true;
                me->SetHomePosition(illidanPos[ILLIDAN_FIRST_STEP]);
                me->GetMotionMaster()->MovePoint(0, illidanPos[ILLIDAN_FIRST_STEP],true);
                PlayQuote(me, introQuotes[0]);
            }
        }

        void DoAction(const int32 action) override
        {
            if (action == ACTION_ILLIDAN_DELAY_MOVE)
                delayMoveTimer = 5000;
            else if (action == ACTION_ON_ALL_DEFFENDERS_KILLED)
                orderTimer = 6000;
            else if (action == ACTION_ILLIDAN_REMOVE_VEHICLE)
                HandleVehicle(false);
            else if (action == ACTION_ILLIDAN_CREATE_VEHICLE)
                HandleVehicle(true);
            else  if (action == ACTION_AFTER_PEROTHARN_DEATH)
                sayDelayTimer = 3000;
        }

        void MovementInform(uint32 type, uint32 wpID) override
        {
            if (!pInstance)
                return;

            int32 id = (int32)wpID; // Stop compiler bitching about unsigned / signed comparsion

            if (type == EFFECT_MOTION_TYPE && id == ILLIDAN_PALACE_JUMP_WP) // jump point
            {
                eventTimer = 100;
                CAST_WOE_INSTANCE(pInstance)->SetIllidanStep(IllidanSteps(id));
                return;
            }

            if (type != POINT_MOTION_TYPE)
                return;

            if ((id >= ILLIDAN_FIRST_STEP && id < ILLIDAN_MAX_STEPS) || id > ILLIDAN_STEP_NULL) // or post wps...
                CAST_WOE_INSTANCE(pInstance)->SetIllidanStep(IllidanSteps(id));

            if ((id >= ILLIDAN_FIRST_STEP && id <= ILLIDAN_THIRD_PACK_STEP) || id == ILLIDAN_PEROTHARN_STEP)
                me->SetFacingTo(illidanPos[id].m_orientation);

            switch (id)
            {
                case ILLIDAN_FIRST_STEP: // first move -> players can start event
                    explainTimer = 30000;
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                    PlayQuote(me, introQuotes[2]);
                    me->RemoveAllAuras();
                    break;
                case ILLIDAN_FIRST_LOOK_STEP:
                {
                    PlayQuote(me, escortQuotes[urand(1,2)]);
                    eventTimer = 1000; // move to distract point in 1 second
                    break;
                }
                case ILLIDAN_DISTRACT_STEP:
                    PlayQuote(me, distractQuote[0]);
                    eventTimer = 4000;
                    break;
                case ILLIDAN_FIRST_PACK_STEP:
                    eventTimer = 10000; // wait 10 seconds, then say waiting quote
                    break;
                case ILLIDAN_STAIRS_1_STEP:
                    PlayQuote(me, escortQuotes[urand(3, 4)]);
                    eventTimer = 100; // move to next point
                    break;
                case ILLIDAN_STAIRS_2_STEP:
                    eventTimer = 100;
                    break;
                case ILLIDAN_SECOND_PACK_STEP:
                    eventTimer = 15000;
                    break;
                case ILLIDAN_WAIT_SECOND_PORTAL_STEP:
                    PlayQuote(me, escortQuotes[5]);
                    eventTimer = 32000;
                    break;
                case ILLIDAN_BEFORE_STAIRS_STEP:
                    PlayQuote(me, escortQuotes[6]);
                    eventTimer = 500;
                    break;
                case ILLIDAN_UPSTAIRS_STEP:
                    // next move after legion demon kill ->
                    break;
                case ILLIDAN_THIRD_PACK_STEP:
                    break;
                case ILLIDAN_BACK_STEP_1:
                    PlayQuote(me, escortQuotes[8]);
                    eventTimer = 4000;
                    break;
                case ILLIDAN_BACK_STEP_2:
                {
                    PlayQuote(me, escortQuotes[9]);
                    eventTimer = 6000;
                    break;
                }
                case ILLIDAN_TOO_EASY_STEP:
                {
                    PlayQuote(me, escortQuotes[10]);
                    eventTimer = 2000;
                    break;
                }
                case ILLIDAN_PEROTHARN_STEP:
                    eventTimer = 1000;
                    break;
                default:
                    break;
            }

            // After Perotharn kill way points
            switch (id)
            {
                case ILLIDAN_PEROTHARN_DEFEATED_STAIRS_WP:
                {
                    PlayQuote(me, afterPerotharnDeathQuote[1]);
                    eventTimer = 5000;
                    me->SetFacingTo(afterPerotharnDeathWP[0].m_orientation);
                }
                break;
                case ILLIDAN_HANDRAIL_WP:
                case ILLIDAN_RUNAWAY_WP:
                    eventTimer = 100;
                break;
                default:
                    break;
            }
        }

        /*
            TODO: Remove this after TESTING !!!
        */
        void ReceiveEmote(Player* player, uint32 uiTextEmote) override
        {
            if (uiTextEmote == TEXTEMOTE_KNEEL && player->IsGameMaster())
            {
                me->GetMotionMaster()->MovePoint(ILLIDAN_PEROTHARN_STEP, illidanPos[ILLIDAN_PEROTHARN_STEP],true);
                CAST_WOE_INSTANCE(pInstance)->crystalsRemaining = 0;
                return;
            }

            uint32 id = uint32(CAST_WOE_INSTANCE(pInstance)->GetIllidanMoveStep()) + 1;
            me->GetMotionMaster()->MovePoint(id, illidanPos[id],true);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!pInstance)
                return;

            if (sayDelayTimer <= diff)
            {
                PlayQuote(me, afterPerotharnDeathQuote[0]);
                sayDelayTimer = MAX_TIMER;
            }
            else sayDelayTimer -= diff;

            // Order to smash crystals
            if (orderTimer <= diff)
            {
                if (GameObject * pfocusCrsytal = me->FindNearestGameObject(PORTAL_ENERGY_FOCUS_ENTRY,50.0f))
                    if (!pfocusCrsytal->HasFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE))
                        PlayQuote(me, crystalOrder[urand(0,3)]);
                orderTimer = MAX_TIMER;
            }
            else orderTimer -= diff;

            // Tell wait quote if group is not attacking too long
            if (waitingTimer <= diff)
            {
                if (Creature * pArcanist = me->FindNearestCreature(CORRUPTED_ARCANIST_ENTRY,50.0f,true))
                    if (pArcanist->IsInCombat() == false)
                        PlayQuote(me, waitingAttack[urand(0, 3)]);
                waitingTimer = MAX_TIMER;
            }
            else waitingTimer -= diff;

            if (delayMoveTimer <= diff)
            {
                // Next step point
                uint32 id = uint32(CAST_WOE_INSTANCE(pInstance)->GetIllidanMoveStep()) + 1;
                me->GetMotionMaster()->MovePoint(id, illidanPos[id],true);

                if (id == ILLIDAN_THIRD_PACK_STEP)
                    PlayQuote(me, escortQuotes[7]);

                delayMoveTimer = MAX_TIMER;
            }
            else delayMoveTimer -= diff;

            // If players dont start event after some time, Illidan will speak to them
            if (explainTimer <= diff)
            {
                IllidanSteps step = CAST_WOE_INSTANCE(pInstance)->GetIllidanMoveStep();
                if (explainCounter < 3 && step == ILLIDAN_FIRST_STEP)
                {
                    PlayQuote(me, explainQuotes[explainCounter]);
                    explainCounter++;
                }
                explainTimer = urand(10000, 30000);
            }
            else explainTimer -= diff;

            if (eventTimer <= diff)
            {
                // Check special action first (shared timer), so we must be sure, that no action will come before special action occur
                switch (specialAction)
                {
                    // No special action set
                    case SPECIAL_ACTION_NONE:
                        break;
                    case SPECIAL_ACTION_MOVE_TO_DEFFENDERS:
                    {
                        uint32 id = (uint32)ILLIDAN_FIRST_PACK_STEP;
                        me->GetMotionMaster()->MovePoint(id, illidanPos[id],true);
                        PlayQuote(me, distractQuote[1]);
                        // This need to be done after every special action ...
                        specialAction = SPECIAL_ACTION_NONE; eventTimer = MAX_TIMER; return;
                    }
                    case SPECIAL_ACTION_WAIT_INTRO:
                    {
                        // ILLIDAN_FIRST_LOOK_STEP
                        uint32 id = (uint32)ILLIDAN_FIRST_LOOK_STEP;
                        me->GetMotionMaster()->MovePoint(id, illidanPos[id],true);
                        PlayQuote(me, escortQuotes[0]);
                        specialAction = SPECIAL_ACTION_NONE; eventTimer = MAX_TIMER; return;
                    }
                }

                IllidanSteps step = CAST_WOE_INSTANCE(pInstance)->GetIllidanMoveStep();
                eventTimer = MAX_TIMER;
                specialAction = SPECIAL_ACTION_NONE;

                switch (step)
                {
                    case ILLIDAN_FIRST_STEP: // nothing to do
                        break;
                    case ILLIDAN_FIRST_LOOK_STEP:
                    {
                        uint32 id = (uint32)ILLIDAN_DISTRACT_STEP;
                        me->GetMotionMaster()->MovePoint(id, illidanPos[id],true);
                        break;
                    }
                    case ILLIDAN_DISTRACT_STEP:
                        me->CastSpell(me, SPELL_DISTRACT_MISSILE_AURA, true);
                        specialAction = SPECIAL_ACTION_MOVE_TO_DEFFENDERS;
                        eventTimer = 12000;
                        break;
                    case ILLIDAN_FIRST_PACK_STEP:
                        waitingTimer = 8000; // say quote after 8 seconds
                        break;
                    case ILLIDAN_STAIRS_1_STEP:
                    {
                        uint32 id = (uint32)step + 1;
                        me->GetMotionMaster()->MovePoint(id, illidanPos[id],true);
                        break;
                    }
                    case ILLIDAN_STAIRS_2_STEP:
                    {
                        uint32 id = (uint32)step + 1;
                        me->GetMotionMaster()->MovePoint(id, illidanPos[id],true);
                        break;
                    }
                    case ILLIDAN_SECOND_PACK_STEP:
                        break;
                    case ILLIDAN_WAIT_SECOND_PORTAL_STEP:
                        me->GetMotionMaster()->MovePoint(ILLIDAN_BEFORE_STAIRS_STEP, illidanPos[ILLIDAN_BEFORE_STAIRS_STEP],true);
                        break;
                    case ILLIDAN_BEFORE_STAIRS_STEP:
                        me->GetMotionMaster()->MovePoint(ILLIDAN_UPSTAIRS_STEP, illidanPos[ILLIDAN_UPSTAIRS_STEP],true);
                        break;
                    case ILLIDAN_UPSTAIRS_STEP:
                        break;
                    case ILLIDAN_THIRD_PACK_STEP:
                        me->GetMotionMaster()->MovePoint(ILLIDAN_BACK_STEP_1, illidanPos[ILLIDAN_BACK_STEP_1],true);
                        break;
                    case ILLIDAN_BACK_STEP_1:
                        me->GetMotionMaster()->MovePoint(ILLIDAN_BACK_STEP_2, illidanPos[ILLIDAN_BACK_STEP_2],true);
                        break;
                    case ILLIDAN_BACK_STEP_2:
                    {
                        if (Creature * pPerotharn = me->FindNearestCreature(PEROTHARN_ENTRY, 100.0f, true))
                            PlayQuote(pPerotharn, Illidan::onEventStart[1]);
                        me->GetMotionMaster()->MovePoint(ILLIDAN_TOO_EASY_STEP, illidanPos[ILLIDAN_TOO_EASY_STEP],true);
                        break;
                    }
                    case ILLIDAN_TOO_EASY_STEP:
                        me->GetMotionMaster()->MovePoint(ILLIDAN_PEROTHARN_STEP, illidanPos[ILLIDAN_PEROTHARN_STEP],true);
                        break;
                    case ILLIDAN_PEROTHARN_STEP:
                    {
                        PlayQuote(me, Illidan::onEventStart[0]);
                        if (Creature * pPerotharn = me->FindNearestCreature(PEROTHARN_ENTRY, 100.0f, true))
                            pPerotharn->AI()->DoAction(ACTION_PEROTHRAN_PREPARE_TO_AGGRO);
                        break;
                    }
                    default:
                        break;
                }

                // After Perotharn kill way points
                switch ((uint32)step)
                {
                    case ILLIDAN_PEROTHARN_DEFEATED_STAIRS_WP:
                        me->GetMotionMaster()->MovePoint(ILLIDAN_HANDRAIL_WP, afterPerotharnDeathWP[1],true);
                        break;
                    case ILLIDAN_HANDRAIL_WP:
                        me->GetMotionMaster()->MoveJump(afterPerotharnDeathWP[2].GetPositionX(),
                                                        afterPerotharnDeathWP[2].GetPositionY(),
                                                        afterPerotharnDeathWP[2].GetPositionZ(),
                                                        10.0f, 30.0f, ILLIDAN_PALACE_JUMP_WP);
                        break;
                    case ILLIDAN_PALACE_JUMP_WP:
                        me->GetMotionMaster()->MovePoint(ILLIDAN_RUNAWAY_WP, afterPerotharnDeathWP[3],true);
                        break;
                    case ILLIDAN_RUNAWAY_WP:
                        me->SetVisible(false);
                        me->Kill(me);
                        break;
                    default:
                        break;
                }
            }
            else eventTimer -= diff;

            if (attackTimer <= diff)
            {
                bool hideAndSeekPhase = false;
                if (Creature * pPerotharn = me->FindNearestCreature(PEROTHARN_ENTRY,200.0f, true))
                if (pPerotharn->AI()->GetData(DATA_GET_PEROTHARN_PHASE) == 1) // 1 -> PHASE_HIDE_AND_SEEK
                    hideAndSeekPhase = true;

                // if (eventTimer > NEVER / 2) -> This should prvent skipping event pathing issuses ..., dont attack if we dont arrive to WP point yet)
                if (pInstance && gossipDone && (eventTimer > MAX_TIMER / 2) && !me->IsNonMeleeSpellCasted(false) && !hideAndSeekPhase)
                {
                    if (Creature * victim = CAST_WOE_INSTANCE(pInstance)->GetIllidanVictim())
                    {
                        HandleVehicle(false);

                        if (me->IsWithinMeleeRange(victim))
                        {
                            if (weaponSwitcher == true)
                                me->HandleEmoteCommand(EMOTE_ONESHOT_ATTACK1H);
                            else
                                me->HandleEmoteCommand(EMOTE_ONESHOT_ATTACKOFF);

                            me->SetFacingTo(me->GetAngle(victim));
                            weaponSwitcher = !weaponSwitcher;
                        }
                        else
                        {
                            float x, y, z;
                            float range = me->GetExactDist(victim) - victim->GetCombatReach();
                            if (range < 5.0f) // just for sure
                                range = 5.0f;
                            me->GetNearPoint(me, x, y, z, range, 0, me->GetAngle(victim));
                            me->GetMotionMaster()->MovePoint(ILLIDAN_ATTACK_START_WP, x, y, z,true);
                        }
                    }
                    else
                    {
                        HandleVehicle(true);
                    }

                }
                attackTimer = 1000;
            }
            else attackTimer -= diff;

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }

        void HandleVehicle(bool enter)
        {
            if (enter)
            {
                if (!me->HasAura(SPELL_SHADOW_WALK_ILLIDAN_KIT))
                {
                    me->AddAura(SPELL_SHADOW_WALK_ILLIDAN_KIT, me); // Create vehicle kit
                    if (Unit * stalker = me->SummonCreature(ILLIDAN_SHADOWCLOAK_VEHICLE_ENTRY,me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0))
                    {
                        stalker->EnterVehicle(me,0);
                        stalker->AddAura(102951, stalker); // add visual circle aura
                        stalker->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE); // for sure
                    }
                }
            }
            else
            {
                Creature* vehPassenger = me->GetVehicleCreatureBase();
                if (vehPassenger == NULL)
                    vehPassenger = me->FindNearestCreature(ILLIDAN_SHADOWCLOAK_VEHICLE_ENTRY, 10.0f, true);

                if (vehPassenger)
                {
                    vehPassenger->Kill(vehPassenger);
                    vehPassenger->ForcedDespawn();
                }
                me->RemoveAura(SPELL_SHADOW_WALK_ILLIDAN_KIT);
            }
        }

        };

        bool OnGossipSelect(Player* player, Creature* cr, uint32 /*Sender*/, uint32 action) override
        {
            player->PlayerTalkClass->ClearMenus();
            npc_illidan_intro_woe::npc_illidan_intro_woeAI* pAI = (npc_illidan_intro_woe::npc_illidan_intro_woeAI*)(cr->GetAI());

            if (!pAI)
                return false;

            switch (action)
            {
                case GOSSIP_ACTION_INFO_DEF + 1:
                    if (pAI->gossipDone == false)
                        PlayQuote(cr,introQuotes[1]);

                    pAI->explainTimer = MAX_TIMER;
                    pAI->HandleVehicle(true);
                    if (pAI->pInstance)
                    {
                        pAI->pInstance->DoUpdateWorldState(WORLD_STATE_DEMONS_IN_COURTYARD, 1); // lets see how it looks like
                        pAI->pInstance->DoAddAuraOnPlayers(NULL, SPELL_SHADOW_CLOAK_TRIGGERER);
                    }

                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_ILLIDAN_2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2); // bubble
                    player->SEND_GOSSIP_MENU(ILLIDAN_GOSSIP_MENU_TEXT_ID_2, cr->GetGUID()); // text

                    if (pAI)
                        pAI->gossipDone = true;
                    break;
                case GOSSIP_ACTION_INFO_DEF + 2:
                {
                    if (pAI->eventStarted == true)
                        return false;
                    cr->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    cr->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                    pAI->specialAction = SPECIAL_ACTION_WAIT_INTRO;
                    pAI->eventTimer = 6000;
                    pAI->eventStarted = true;
                    break;
                }
            }
            return true;
        }
        bool OnGossipHello(Player* player, Creature* creature) override
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_ILLIDAN_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1); // bubble
            player->SEND_GOSSIP_MENU(ILLIDAN_GOSSIP_MENU_TEXT_ID_1, creature->GetGUID()); // text
            return true;
        }

        CreatureAI* GetAI(Creature* pCreature) const
        {
            return new npc_illidan_intro_woeAI(pCreature);
        }
    };
}

enum eyeSpells
{
    SPELL_FEL_FLAMES = 102356,
    SPELL_FEL_LIGHTNING = 102361 
};

class npc_eye_of_the_legion_trash : public CreatureScript
{
public:
    npc_eye_of_the_legion_trash() : CreatureScript("npc_eye_of_the_legion_trash") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_eye_of_the_legion_trashAI(creature);
    }

    struct npc_eye_of_the_legion_trashAI : public ScriptedAI
    {
        npc_eye_of_the_legion_trashAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 felFlamesTimer;
        uint32 felLightningTimer;

        void Reset() override
        {
            felFlamesTimer = urand(6000, 8000);
            felLightningTimer = urand(4000, 5000);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (felFlamesTimer <= diff)
            {
                me->CastSpell(me->GetVictim(),SPELL_FEL_FLAMES , true);
                felFlamesTimer = urand(9000,12000);
            }
            else felFlamesTimer -= diff;

            if (felLightningTimer <= diff)
            {
                if (Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 20.0f, true))
                    me->CastSpell(target, SPELL_FEL_LIGHTNING, false);
                felLightningTimer = urand(4000,5000);
            }
            else felLightningTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

enum championSpells
{
    SPELL_QUEENS_BLADE = 102260,
    SPELL_SHIMMERING_STRIKE_AOE = 102257
};

class npc_eternal_champion_trash_woe : public CreatureScript
{
public:
    npc_eternal_champion_trash_woe() : CreatureScript("npc_eternal_champion_trash_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_eternal_champion_trash_woeAI(creature);
    }

    struct npc_eternal_champion_trash_woeAI : public ScriptedAI
    {
        npc_eternal_champion_trash_woeAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 bladeTimer;
        uint32 strikeTimer;

        void Reset() override
        {
            bladeTimer = urand(2000, 4000);
            strikeTimer = urand(8000, 10000);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (bladeTimer <= diff)
            {
                me->CastSpell(me->GetVictim(),SPELL_QUEENS_BLADE , true);
                bladeTimer = urand(9000,12000);
            }
            else bladeTimer -= diff;

            if (strikeTimer <= diff)
            {
                me->CastSpell(me, SPELL_SHIMMERING_STRIKE_AOE, true);
                strikeTimer = urand(4000,5000);
            }
            else strikeTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

enum EnchantedMistressSpells
{
    SPELL_BLIZZARD = 102267,
    SPELL_FROSTBOLT = 102266,
    SPELL_FIREBALL = 102265,
    SPELL_FIREBOMB = 102263
};

enum EnchantedMistressEntry
{
    ENTRY_FROST_MISTRESS = 54589,
    ENTRY_FIRE_MISTRESS = 56579
};

enum events
{
    EVENT_FROSTBOLT,
    EVENT_FIREBALL,
    EVENT_BLIZZARD,
    EVENT_FIREBOMB
};

class npc_enchanted_mistress_woe : public CreatureScript
{
public:
    npc_enchanted_mistress_woe() : CreatureScript("npc_enchanted_mistress_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_enchanted_mistress_woeAI(creature);
    }

    struct npc_enchanted_mistress_woeAI : public ScriptedAI
    {
        npc_enchanted_mistress_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            entry = me->GetEntry();
        }

        uint32 entry;
        EventMap events;

        void Reset() override
        {
            events.Reset();
        }

        void EnterCombat(Unit * /*who*/) override
        {
            if (entry == ENTRY_FROST_MISTRESS)
            {
                events.ScheduleEvent(EVENT_FROSTBOLT, 1000);
                events.ScheduleEvent(EVENT_BLIZZARD, 8000);
            }
            else if (entry == ENTRY_FIRE_MISTRESS)
            {
                events.ScheduleEvent(EVENT_FIREBALL, 1000);
                events.ScheduleEvent(EVENT_FIREBOMB, 6000);
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case EVENT_FROSTBOLT:
                        if (!me->IsNonMeleeSpellCasted(false))
                        {
                            me->CastSpell(me->GetVictim(), SPELL_FROSTBOLT, false);
                            events.ScheduleEvent(EVENT_FROSTBOLT, 2500);
                        }
                        break;
                    case EVENT_FIREBALL:
                        if (!me->IsNonMeleeSpellCasted(false))
                        {
                            me->CastSpell(me->GetVictim(), SPELL_FIREBALL, false);
                            events.ScheduleEvent(EVENT_FIREBALL, 2500);
                        }
                        break;
                    case EVENT_BLIZZARD:
                        if (!me->IsNonMeleeSpellCasted(false))
                        {
                            if (Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 30.0f, true))
                                me->CastSpell(target, SPELL_BLIZZARD, false);
                            events.ScheduleEvent(EVENT_BLIZZARD, urand(8000,10000));
                        }
                        break;
                    case EVENT_FIREBOMB:
                        if (!me->IsNonMeleeSpellCasted(false))
                        {
                            if (Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 40.0f, true))
                                me->CastSpell(target, SPELL_FIREBOMB, false);
                            events.ScheduleEvent(EVENT_FIREBOMB, urand(8000,10000));
                        }
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_well_of_eternity_trash()
{
    // NPC SCRIPTS
    new Legion_demon_WoE(); // 54500
    new fel_crystal_stalker_woe(); // 55965
    new npc_woe_generic(); // 55503,54927,55654,55656
    new npc_portal_connector(); // 55541,55542,55543
    new npc_legion_portal_woe(); // 54513
    new Illidan::npc_illidan_intro_woe();
    new npc_eye_of_the_legion_trash(); // 54747 ?
    new npc_eternal_champion_trash_woe(); // 54612 ?
    new npc_enchanted_mistress_woe(); // 54589 + 56579 ?
    // GO SCRIPTS
    new go_fel_crystal_woe();
    // SPELLSCRIPTS
    new spell_gen_woe_crystal_selector(); // 105018 + 105074 + 105004
    new spell_gen_woe_distract_selector(); // 110121
}

/*
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105018, 'spell_gen_woe_crystal_selector');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105074, 'spell_gen_woe_crystal_selector');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105004, 'spell_gen_woe_crystal_selector');

UPDATE `gameobject_template` SET `ScriptName`='go_fel_crystal_woe' WHERE  `entry`=209366 LIMIT 1;

UPDATE `creature_template` SET `ScriptName`='fel_crystal_stalker_woe' WHERE  `entry`=55965 LIMIT 1;
UPDATE `creature_template` SET `ScriptName`='npc_woe_generic' WHERE  `entry`=54927 LIMIT 1;
UPDATE `creature_template` SET `ScriptName`='npc_woe_generic' WHERE  `entry`=55503 LIMIT 1;
UPDATE `creature_template` SET `ScriptName`='npc_woe_generic' WHERE  `entry`=55654 LIMIT 1;
UPDATE `creature_template` SET `ScriptName`='npc_woe_generic' WHERE  `entry`=55656 LIMIT 1;
UPDATE `creature_template` SET `ScriptName`='npc_legion_portal_woe' WHERE  `entry`=54513 LIMIT 1;

select * from creature_template where entry in (55503,54927,55654,55656);


REPLACE INTO `npc_text` (`ID`, `text0_0`, `text0_1`, `lang0`, `prob0`, `em0_0`, `em0_1`, `em0_2`, `em0_3`, `em0_4`, `em0_5`, `text1_0`, `text1_1`, `lang1`, `prob1`, `em1_0`, `em1_1`, `em1_2`, `em1_3`, `em1_4`, `em1_5`, `text2_0`, `text2_1`, `lang2`, `prob2`, `em2_0`, `em2_1`, `em2_2`, `em2_3`, `em2_4`, `em2_5`, `text3_0`, `text3_1`, `lang3`, `prob3`, `em3_0`, `em3_1`, `em3_2`, `em3_3`, `em3_4`, `em3_5`, `text4_0`, `text4_1`, `lang4`, `prob4`, `em4_0`, `em4_1`, `em4_2`, `em4_3`, `em4_4`, `em4_5`, `text5_0`, `text5_1`, `lang5`, `prob5`, `em5_0`, `em5_1`, `em5_2`, `em5_3`, `em5_4`, `em5_5`, `text6_0`, `text6_1`, `lang6`, `prob6`, `em6_0`, `em6_1`, `em6_2`, `em6_3`, `em6_4`, `em6_5`, `text7_0`, `text7_1`, `lang7`, `prob7`, `em7_0`, `em7_1`, `em7_2`, `em7_3`, `em7_4`, `em7_5`, `WDBVerified`) VALUES (555001, 'The shadows will hide us well.I am concealed by this shadowcloak, and can imbue its magics onto you.It will not make us invisible, but as long as we keep our distance from our foes we will remain hidden.', NULL, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 1);
REPLACE INTO `npc_text` (`ID`, `text0_0`, `text0_1`, `lang0`, `prob0`, `em0_0`, `em0_1`, `em0_2`, `em0_3`, `em0_4`, `em0_5`, `text1_0`, `text1_1`, `lang1`, `prob1`, `em1_0`, `em1_1`, `em1_2`, `em1_3`, `em1_4`, `em1_5`, `text2_0`, `text2_1`, `lang2`, `prob2`, `em2_0`, `em2_1`, `em2_2`, `em2_3`, `em2_4`, `em2_5`, `text3_0`, `text3_1`, `lang3`, `prob3`, `em3_0`, `em3_1`, `em3_2`, `em3_3`, `em3_4`, `em3_5`, `text4_0`, `text4_1`, `lang4`, `prob4`, `em4_0`, `em4_1`, `em4_2`, `em4_3`, `em4_4`, `em4_5`, `text5_0`, `text5_1`, `lang5`, `prob5`, `em5_0`, `em5_1`, `em5_2`, `em5_3`, `em5_4`, `em5_5`, `text6_0`, `text6_1`, `lang6`, `prob6`, `em6_0`, `em6_1`, `em6_2`, `em6_3`, `em6_4`, `em6_5`, `text7_0`, `text7_1`, `lang7`, `prob7`, `em7_0`, `em7_1`, `em7_2`, `em7_3`, `em7_4`, `em7_5`, `WDBVerified`) VALUES (555002, 'Anything outside of that circle around you will not be able to see you, but the illusion will fade if you attack or take damage.I suggest you remain hidden until I say otherwise, these demons are too powerful to face head on and in full force.', NULL, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 1);




*/