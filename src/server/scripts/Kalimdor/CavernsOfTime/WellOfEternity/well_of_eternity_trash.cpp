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

#define NEVER  (0xffffffff)
#define GO_FIREWALL (207679)

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
                if (Creature * pGaurd = CAST_WOE_INSTANCE(pInstance)->GetGuardianByDirection(waveNumber,me))
                {
                    pGaurd->AI()->SetData(DATA_SET_WAVE_NUMBER, waveNumber);
                    pGaurd->AI()->DoAction(ACTION_PORTAL_CLOSING);
                }

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

        void SetData(uint32 type, uint32 data)
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

        void MovementInform(uint32 type, uint32 id)
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

        void Reset()
        {
            me->SetReactState(REACT_DEFENSIVE); // Temporary
            waitTimer = NEVER;
            guardMoveTimer = NEVER;
            distractionTimer = 10000;
        }

        void EnterEvadeMode()
        {
            // Nah ...
            me->ForcedDespawn();
        }

        void UpdateAI(const uint32 diff)
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
                    me->GetMotionMaster()->MovePoint(WP_END, leftEndPositions[DIRECTION_RIGHT]);
                else
                    me->GetMotionMaster()->MovePoint(WP_END, rightEndPositions[DIRECTION_RIGHT]);
            }

            // Second demon in line must wait for another demon , because his path is a bit longer
            if (waitTimer <= diff)
            {
                waitTimer = NEVER;

                if (direction == DIRECTION_STRAIGHT || waveNumber == WAVE_THREE)
                    return;

                if (direction == DIRECTION_LEFT)
                    me->GetMotionMaster()->MovePoint(WP_END, leftEndPositions[DIRECTION_LEFT]);
                else
                    me->GetMotionMaster()->MovePoint(WP_END, rightEndPositions[DIRECTION_LEFT]);
            }
            else waitTimer -= diff;

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
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

        void DoAction(const int32 action)
        {
            if (action == DATA_CRYSTAL_DESTROYED)
            {
                me->CastSpell(me, SPELL_CRYSTAL_MELTDOWN, false);
                me->RemoveAura(SPELL_CRYSTAL_PERIODIC);
                calm = false;
                colapseTimer = 3500;
            }
        }

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
            me->CastSpell(me, SPELL_CRYSTAL_PERIODIC, true);
            me->CastSpell(me, SPELL_FEL_CRYSTAL_STALKER_GLOW, true);
            beamTimer = 2000;
            colapsed = false;
            beamCasted = false;
        }

        void UpdateAI(const uint32 diff)
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

                uint32 connectorEntry = GetConnectorEntryByXCoord(me->GetPositionX());

                // Turn off connectors
                CAST_WOE_INSTANCE(pInstance)->TurnOffConnectors(connectorEntry, me);
                CAST_WOE_INSTANCE(pInstance)->CrystalDestroyed(me->GetPositionX());

                //Remove visual auras
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
            portalClosed = false;
        }

        uint32 entry;

        // guardian demon
        uint32 gripTimer;
        uint32 finalWalkTimer;
        DemonWave wave;
        uint32 lane;
        bool portalClosed;
        bool firstWP;
        // legion demon
        bool canMoveToPoint;
        uint32 leapTimer;
        uint32 strikeTimer;
        // arcanist
        uint32 arcaneTimer;
        // dreadlord
        uint32 swarmTimer;
        uint32 wardingTimer;

        void Reset()
        {
            finalWalkTimer = NEVER;
            wave = WAVE_ONE; // by default
            lane = 0;

            if (entry == GUARDIAN_DEMON_ENTRY)
                me->CastSpell(me, 90766, true); // Hovering Anim State

            firstWP = true;
            canMoveToPoint = false;
            gripTimer = 1000;
            leapTimer = 4000;
            strikeTimer = 5000;

            arcaneTimer = 2000;

            swarmTimer = urand(2000,5000);
            wardingTimer = 1000;
        }

        void SetData(uint32 type, uint32 data)
        {
            if (type == DATA_SET_WAVE_NUMBER)
                wave = (DemonWave)data;
            else if (type == DATA_SET_GUARDIAN_WAVE)
                lane = data;
        }

        uint32 GetData(uint32 type)
        {
            if (type == DATA_GET_GUARDIAN_WAVE && entry == GUARDIAN_DEMON_ENTRY)
                return lane;

            return 0;
        }

        void DoAction(const int32 action)
        {
            if (action == ACTION_PORTAL_CLOSING && portalClosed == false)
            {
                portalClosed = true;
                finalWalkTimer = 25000;
            }
        }

        void SpellHit(Unit* caster, const SpellEntry* pSpell)
        {
            if (caster == me && pSpell->Id == 107867)
                arcaneTimer = 500;
        }

        void SpellHitTarget(Unit *pTarget,const SpellEntry* spell)
        {
            if (spell->Id == SPELL_SUMMON_FIREWALL_DUMMY)
            {
                me->SummonGameObject(FIREWALL_ENTRY, 3193.9f, -4931.0f, 189.6f, 1.1f, 0, 0, 0, 0, 0);

                if (GameObject * pGo = me->FindNearestGameObject(FIREWALL_INVIS_ENTRY, 200.0f))
                    pGo->Delete();
            }
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (me->ToTempSummon() == NULL)
                return;

            if (id == 2 && entry == GUARDIAN_DEMON_ENTRY)
                finalWalkTimer = 1; // next update tick

            if (id == 3 && entry == GUARDIAN_DEMON_ENTRY)
            {
                if (Creature * pPortalDummy = me->FindNearestCreature(LEGION_PORTAL_DUMMY,50.0f,true))
                if (Aura * a = pPortalDummy->GetAura(SPELL_PORTAL_STATUS_SHUTTING_DOWN))
                    a->SetDuration(1000);
                me->ForcedDespawn();
            }

            if (id == 1 && entry == LEGION_DEMON_ENTRY)
            {
                me->CastSpell(me, SPELL_SUMMON_FIREWALL_DUMMY, false);
                me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
            }

            if (id != 0)
                return;

            if (entry == CORRUPTED_ARCANIST_ENTRY || DREADLORD_DEFFENDER_ENTRY)
                canMoveToPoint = true;
        }

        void JustDied(Unit * killer)
        {
            if (entry == LEGION_DEMON_ENTRY)
            {
                if (GameObject * pGo = me->FindNearestGameObject(FIREWALL_ENTRY, 200.0f))
                    pGo->Delete();
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (canMoveToPoint)
            {
                canMoveToPoint = false;
                if (entry == CORRUPTED_ARCANIST_ENTRY || DREADLORD_DEFFENDER_ENTRY)
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

                    me->GetMotionMaster()->MovePoint(2, guardPos[(uint32)wave].first);
                    finalWalkTimer = NEVER; // set in movement inform, when WP2 is reached
                    return;
                }

                me->GetMotionMaster()->MovePoint(3, guardPos[(uint32)wave].last);
                finalWalkTimer = NEVER;
            }
            else finalWalkTimer -= diff;

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
                        if (me->GetVictim())
                            me->CastSpell(me->GetVictim(), SPELL_DEMON_GRIP, true);
                        gripTimer = 2000;
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
                        }
                    }
                    else arcaneTimer -= diff;

                    float manaPct = me->GetPower(POWER_MANA) / me->GetMaxPower(POWER_MANA);
                    if (manaPct <= 0.12f && !me->IsNonMeleeSpellCasted(false))
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

        void Reset()
        {
            beamTimer = 5000;
            connected = false;
        }

        void UpdateAI(const uint32 diff)
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

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_woe_crystal_selector_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_AREA_ENTRY_DST);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_woe_crystal_selector_SpellScript();
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

        void Reset()
        {
            if (!pInstance)
                return;

            if (pInstance->GetData(DATA_PEROTHARN) == NOT_STARTED)
                me->CastSpell(me,SPELL_PORTAL_STATUS_ACTIVE,true);
        }

        void UpdateAI(const uint32 /*uiDiff*/)
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

        void Reset()
        {
        }

        void UpdateAI(const uint32 diff)
        {
        }
    };
};*/

void AddSC_well_of_eternity_trash()
{
    // NPC SCRIPTS
    new Legion_demon_WoE(); // 54500
    new fel_crystal_stalker_woe(); // 55965
    new npc_woe_generic(); // 55503,54927,55654,55656
    new npc_portal_connector(); // 55541,55542,55543
    new npc_legion_portal_woe(); // 54513
    // GO SCRIPTS
    new go_fel_crystal_woe();
    // SPELLSCRIPTS
    new spell_gen_woe_crystal_selector(); // 105018 + 105074 + 105004
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




*/