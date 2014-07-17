/*
 * Copyright (C) 2008-2010 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
#include "throne_of_the_four_winds.h"

/*
 TODO:
critical:
 --
to be done:
 correct timers based on test feedback
obsolete:
 lightning clouds dummy visual range to cca 20 (spells: 89583, 89592)
 Squall Line: wrong Vehicle Kit ID
 destroy platform animation
workarond-ed:
 return to platform - port
 Relentless Storm
*/

enum Stuff
{
// phase 1
    SPELL_LIGHTNING_STRIKE_DAMAGE = 88214,
    SPELL_LIGHTNING_STRIKE_DAMAGE_10H = 93255,
    SPELL_LIGHTNING_STRIKE_DAMAGE_25N = 93256,
    SPELL_LIGHTNING_STRIKE_DAMAGE_25H = 93257,
    SPELL_LIGHTNING_STRIKE_VISUAL = 88230,
    SPELL_STATIC_SHOCK_HC_ONLY    = 87873,
    SPELL_ELECTROCUTE             = 88427,
    SPELL_WIND_BURST              = 87770,
    SPELL_ICE_STORM_GROUND_AURA   = 87469,
    SPELL_ICE_STORM_TRIGGER       = 87472,
    SPELL_ICE_STORM_SUMMON        = 87055,
    SPELL_SQUALL_LINE_DUMMY_ALERT = 91115, // dummy spell for addons
    SPELL_SQUALL_LINE_INIT        = 87652,
    SPELL_SQUALL_LINE_VISUAL      = 87621,
    SPELL_SQUALL_LINE_VEHICLE     = 87856,
// phase 2
    //SPELL_ACID_RAIN_TRIGGER       = 88290,
    SPELL_ACID_RAIN_DOT           = 88301,
    NPC_STORMLING                 = 47175,
    SPELL_STORMLING_DAMAGE        = 87905,
    SPELL_STORMLING_VISUAL        = 87906,
    SPELL_FEEDBACK                = 87904,
// phase 3
    SPELL_LIGHTNING               = 89641,
    SPELL_LIGHTNING_ROD           = 89668,
    SPELL_WIND_BURST_THIRD_PHASE  = 88858,
    SPELL_EYE_OF_THE_STORM_AURA   = 82724,
    SPELL_EYE_OF_THE_STORM_TRIGGER= 89107,
    SPELL_RELENTLESS_STORM_CHANNEL= 88875,
    SPELL_RELENTLESS_STORM_VISUAL = 88866,
    SPELL_LIGHTNING_CLOUDS_ALERT  = 89628, // dummy spell for addons
    SPELL_LIGHTNING_CLOUDS        = 89564,
    SPELL_LIGHTNING_CLOUDS_DMG    = 89587,
    NPC_ALAKIR_LARGE_MODEL        = 48233,
    //// Relentless Storm ? - workaround-ed
    //SPELL_RELENTLESS_STORM_VEHICLE= 89104,
    //NPC_RELENTLESS_STORM          = 47807,

    GO_HEART_OF_WIND              = 207891,
};

#define SL_DIST_FIRST 29.0f
#define SL_DIST_NEXT  5.32f
#define SL_COUNT 8
#define SL_RAD_PER_STEP (M_PI/32.0f)
#define SL_FURTH_SPEED (1.3f * (SL_DIST_FIRST+SL_DIST_NEXT * (SL_COUNT-1)))

class boss_alakir: public CreatureScript
{
    public:
        boss_alakir(): CreatureScript("boss_alakir") {}

        struct boss_alakirAI: public Scripted_NoMovementAI
        {
            boss_alakirAI(Creature* c): Scripted_NoMovementAI(c)
            {
                c->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
                c->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
                c->ApplySpellImmune(0, IMMUNITY_ID, 49560, true); // Death Grip jump effect
                c->ApplySpellImmune(0, IMMUNITY_ID, 81261, true); // Solar Beam
                c->ApplySpellImmune(0, IMMUNITY_ID, 88625, true); // Chastise
                c->SetReactState(REACT_DEFENSIVE);

                instance = c->GetInstanceScript();
                map = c->GetMap();

                m_stormlings.clear();
                m_relentless_guid = 0;
                Reset();
            }

            InstanceScript* instance;
            Map* map;
            uint32 m_phase;

            std::list<uint64> m_stormlings;

            uint32 checkHealthTimer;

            uint32 squallLineTimer;
            uint32 iceStormTimer;
            uint32 lightningStrikeTimer;
            uint32 windBurstTimer;
            uint32 staticShockTimer;
            uint32 acidRainTimer;
            uint32 stormlingTimer;

            uint64 m_relentless_guid;
            uint64 m_large_model_guid;

            uint16 vortexready[2][SL_COUNT]; // 2 slots for simultaneous Squall Lines
            bool vortexslot;

            void Reset()
            {
                m_phase = 0;

                checkHealthTimer = 2000;

                iceStormTimer = 5000;
                squallLineTimer = 15000;
                lightningStrikeTimer = 8000;
                windBurstTimer = 11000;
                staticShockTimer = 2000;
                acidRainTimer = 2000;
                stormlingTimer = 6000;

                m_large_model_guid = 0;
                DespawnStormlings();

                vortexslot = false;
                for (uint32 i = 0; i < SL_COUNT; i++)
                {
                    vortexready[0][i] = 0;
                    vortexready[1][i] = 0;
                }

                if (instance)
                    instance->SetData(TYPE_ALAKIR, NOT_STARTED);

                if (GameObject* skywall = GetClosestGameObjectWithEntry(me, 4510369, 55555.0f))
                    skywall->EnableCollision(false);

                me->SetFlying(false);
                me->SetVisibility(VISIBILITY_ON);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);

            }

            void EnterCombat(Unit* /*who*/)
            {
                if (instance)
                    instance->SetData(TYPE_ALAKIR, IN_PROGRESS);

                DoScriptText(-1850527, me);

                m_phase = 1;
            }

            void JustDied(Unit* /*pKiller*/)
            {
                if (instance)
                    instance->SetData(TYPE_ALAKIR, DONE);

                DespawnStormlings();

                if (!map)
                    return;

                // summon loot chest
                uint32 go_entry = GO_HEART_OF_WIND + map->GetDifficulty(); // 207891 + 0/1/2/3
                GameObject* pGO = NULL;
                pGO = me->SummonGameObject(go_entry, me->GetPositionX(), me->GetPositionY(), 243.04f, 0, 0, 0, 0, 0, 0);

                Map::PlayerList const& plrList = map->GetPlayers();
                if (plrList.isEmpty())
                    return;
                for(Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
                {
                    if(Player* pPlayer = itr->getSource())
                    {
                        // kill credit to all players
                        pPlayer->KilledMonsterCredit(me->GetEntry(), me->GetGUID());
                        // GO client upate
                        if (pGO)
                            pGO->SendUpdateToPlayer(pPlayer);
                    }
                }
            }

            void KilledUnit(Unit* /*victim*/)
            {
                DoScriptText((urand(0,1) ? -1850532 : -1850533), me);
            }

            void SpellHitTarget(Unit* pTarget, const SpellEntry* spell)
            {
                if (!spell || !pTarget)
                    return;

                // Cast visual when target hit by Lightning Strike
                if (spell->Id == SPELL_LIGHTNING_STRIKE_DAMAGE || spell->Id == SPELL_LIGHTNING_STRIKE_DAMAGE_10H
                    || spell->Id == SPELL_LIGHTNING_STRIKE_DAMAGE_25N || spell->Id == SPELL_LIGHTNING_STRIKE_DAMAGE_25H)
                    me->CastSpell(pTarget, SPELL_LIGHTNING_STRIKE_VISUAL, true);
            }

            void DamageTaken(Unit* /*pDoneBy*/, uint32 &damage)
            {
                if (m_phase == 3)
                    damage = 0;
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                // ######## Switch Phases ########
                if (checkHealthTimer <= diff)
                {
                    if (m_phase == 2 && me->GetHealthPct() < 25.0f)
                    {
                        DoScriptText(-1850531, me);
                        m_phase = 3;

                        if(!map)
                            return;
                    // shatter platform, initialize
                        if (instance)
                            instance->SetData(TYPE_ALAKIR, SPECIAL);

                    // let players fly
                        me->CastSpell(me, SPELL_WIND_BURST_THIRD_PHASE, false);

                    // let myself fly
                        me->SetFlying(true);


                    // set invisible
                        me->SetVisibility(VISIBILITY_OFF);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);

                    // summon large model boss
                        if (Creature* tmp_large_model = me->SummonCreature(NPC_ALAKIR_LARGE_MODEL, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0))
                        {
                            m_large_model_guid = tmp_large_model->GetGUID();
                            tmp_large_model->SetMaxHealth(me->GetMaxHealth());
                            tmp_large_model->SetHealth(me->GetHealth());
                            if (Unit* victim = me->getVictim())
                                tmp_large_model->AI()->AttackStart(victim);
                        }

                    // let Stormlings fly
                        if (!m_stormlings.empty())
                        {
                            Creature *summon = NULL;
                            for (std::list<uint64>::const_iterator itr = m_stormlings.begin(); itr != m_stormlings.end(); ++itr)
                            {
                                if ((summon = map->GetCreature(*itr)) != NULL)
                                    me->AddAura(SPELL_EYE_OF_THE_STORM_AURA, summon);
                            }
                        }

                    // Relentless Storm visual on trigger npc
                        if (Creature* summon = me->SummonTrigger(0, 0, 0, 0, 0, 0))
                        {
                            m_relentless_guid = summon->GetGUID();

                            summon->SetFlying(true);
                            summon->GetMotionMaster()->MovePoint(666,me->GetPositionX(),me->GetPositionY(),me->GetPositionZ());
                            WorldPacket hbt;
                            me->BuildHeartBeatMsg(&hbt);
                            me->SendMessageToSet(&hbt, false);

                            summon->SetReactState(REACT_PASSIVE);
                            summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                            me->AddAura(SPELL_RELENTLESS_STORM_VISUAL, summon);
                        }
                    }
                    else if (m_phase == 1 && me->GetHealthPct() < 80.0f)
                    {
                        DoScriptText(-1850529, me);
                        m_phase = 2;
                    }
                    else if (m_phase == 3)
                    {
                        // check 2D distance of players -- Relentless Strom workaround
                        Map::PlayerList const &plList = map->GetPlayers();
                        if (!plList.isEmpty())
                            for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
                            {
                                if (Player *player = itr->getSource())
                                {
                                    if (me->GetDistance2d(player) > 130.0f // is too far
                                        && !player->HasAura(89104))        // has not internal CD aura
                                    {
                                        float dx = player->GetPositionX() - me->GetPositionX();
                                        float dy = player->GetPositionY() - me->GetPositionY();
                                        float fx = me->GetPositionX() + (dx/2);
                                        float fy = me->GetPositionY() + (dy/2);
                                        player->GetMotionMaster()->MoveJump(fx, fy, player->GetPositionZ()-10.0f, 25.0f, 8.0f);
                                        me->AddAura(89104, player);
                                    }
                                }
                            }
                    }

                    checkHealthTimer = 2000;

                } else checkHealthTimer -= diff;

                // ######## Abilities Timers ########
                switch (m_phase)
                {
                case 1:
                    {
                    // Squall Line
                        if (squallLineTimer <= diff)
                        {
                            squallLineTimer = 47000; // 50s duration
                            DoSquallLine();

                        } else squallLineTimer -= diff;

                    // Ice Storm
                        if (iceStormTimer <= diff)
                        {
                            DoIceStorm();
                            iceStormTimer = 25000;

                        } else iceStormTimer -= diff;

                    // Static Shock -- HC only
                        switch (getDifficulty())
                        {
                            case RAID_DIFFICULTY_10MAN_HEROIC:
                            case RAID_DIFFICULTY_25MAN_HEROIC:
                                if (staticShockTimer <= diff)
                                {
                                    me->InterruptSpell(CURRENT_CHANNELED_SPELL);
                                    if (me->HasUnitState(UNIT_STATE_CASTING))
                                        break;
                                    me->CastSpell(me, SPELL_STATIC_SHOCK_HC_ONLY, false);
                                    staticShockTimer = 2000;

                                } else staticShockTimer -= diff;
                                break;
                            default: break;
                        }

                    // Wind Burst
                        if (windBurstTimer <= diff)
                        {
                            DoScriptText(-1850528, me);

                            me->InterruptSpell(CURRENT_CHANNELED_SPELL);
                            if (me->HasUnitState(UNIT_STATE_CASTING))
                                break;
                            me->CastSpell(me, SPELL_WIND_BURST, false);

                            windBurstTimer = 19000;
                            lightningStrikeTimer += 5000;

                            return;

                        } else windBurstTimer -= diff;

                    // Lightning Strike
                        if (lightningStrikeTimer <= diff)
                        {
                            Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0);
                            if (target)
                            {
                                me->InterruptSpell(CURRENT_CHANNELED_SPELL);
                                if (me->HasUnitState(UNIT_STATE_CASTING))
                                    break;
                                me->CastSpell(target, SPELL_LIGHTNING_STRIKE_DAMAGE, false);
                                float ori = me->GetAngle(target->GetPositionX(), target->GetPositionY());
                                DoLightningVisual(ori);
                            }
                            lightningStrikeTimer = 5000;

                            return;

                        } else lightningStrikeTimer -= diff;

                    // attack
                        if (me->IsWithinMeleeRange(me->getVictim()))
                            me->InterruptSpell(CURRENT_CHANNELED_SPELL);
                        DoMeleeAttackIfReady();
                        DoSpellAttackIfReady(SPELL_ELECTROCUTE); // case primary target is not in melee range

                    }
                    break;
                case 2:
                    {
                    // Squall Line
                        if (squallLineTimer <= diff)
                        {
                            squallLineTimer = 43000;
                            DoSquallLine();

                        } else squallLineTimer -= diff;

                    // Static Shock -- HC only
                        switch (getDifficulty())
                        {
                            case RAID_DIFFICULTY_10MAN_HEROIC:
                            case RAID_DIFFICULTY_25MAN_HEROIC:
                                if (staticShockTimer <= diff)
                                {
                                    me->InterruptSpell(CURRENT_CHANNELED_SPELL);
                                    if (me->HasUnitState(UNIT_STATE_CASTING))
                                        break;
                                    me->CastSpell(me, SPELL_STATIC_SHOCK_HC_ONLY, false);
                                    staticShockTimer = 2000;

                                } else staticShockTimer -= diff;
                                break;
                            default: break;
                        }

                    // acid rain
                        if (acidRainTimer <= diff)
                        {
                            me->CastSpell(me, SPELL_ACID_RAIN_DOT, true);
                            acidRainTimer = 14500;

                        } else acidRainTimer -= diff;

                    // summon stormling
                        if (stormlingTimer <= diff)
                        {
                            Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0);
                            if (target)
                            {
                                Creature* summon = me->SummonCreature(NPC_STORMLING, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN, 0);
                                if (summon)
                                {
                                    summon->setFaction(14);
                                    summon->AddThreat(target, 1.0f);
                                    summon->AI()->AttackStart(target);
                                    summon->CastSpell(summon, SPELL_STORMLING_DAMAGE, true);
                                    summon->CastSpell(summon, SPELL_STORMLING_VISUAL, true);

                                    m_stormlings.push_back(summon->GetGUID());
                                }
                            }
                            stormlingTimer = 20000;

                        } else stormlingTimer -= diff;

                    // attack
                        if (me->IsWithinMeleeRange(me->getVictim()))
                            me->InterruptSpell(CURRENT_CHANNELED_SPELL);
                        DoMeleeAttackIfReady();
                        DoSpellAttackIfReady(SPELL_ELECTROCUTE); // case primary target is not in melee range

                    }
                    break;
                case 3:
                    // large model npc has its own AI
                    break;
                default:
                    break;
               }
            }

            void SummonedCreatureDies(Creature* summon, Unit* killer)
            {
                // Stomrling dies --> apply feedback debuff
                if (summon && summon->GetEntry() == NPC_STORMLING)
                {
                    if (m_phase == 3 && map != NULL)
                    {
                        if (Creature* tmp = map->GetCreature(m_large_model_guid))
                            tmp->CastSpell(tmp, SPELL_FEEDBACK, true);
                    }
                    else
                        me->CastSpell(me, SPELL_FEEDBACK, true);

                    summon->ForcedDespawn();
                    m_stormlings.remove(summon->GetGUID());
                }
                // large model for phase three dies --> die
                else if (summon && summon->GetEntry() == NPC_ALAKIR_LARGE_MODEL && killer)
                {
                    m_phase = 4;
                    killer->Kill(me);
                }
            }

            void SetData(uint32 id, uint32 data)
            {
                uint32 pos = id/10;
                uint32 slot = id%10;
                vortexready[slot][pos] = (uint16)data;
            }

            uint32 GetData(uint32 id)
            {
                uint32 pos = id/10;
                uint32 slot = id%10;
                return vortexready[slot][pos];
            }

            void DoSquallLine()
            {
                DoScriptText(-1850530, me);
                me->CastSpell(me, SPELL_SQUALL_LINE_DUMMY_ALERT, true);

                Creature* pTemp = NULL;
                float mx,my,mz;
                uint32 mstep = urand(0,uint32((2*M_PI)/(SL_RAD_PER_STEP))-1);
                uint32 skip = urand(0,SL_COUNT-2);

                uint32 slot;
                (vortexslot) ? slot = 1 : slot = 0;
                vortexslot ^= true;

                uint32 clockwise = urand(0,1);
                float direction;
                (!clockwise) ? direction=1.0f : direction=-1.0f;

                for (uint32 i = 0; i < SL_COUNT; i++)
                {
                    vortexready[slot][i] = 0;

                    if (i == skip || i == skip+1)
                        continue;

                    mx = me->GetPositionX() + (SL_DIST_FIRST+SL_DIST_NEXT * i) * cos(mstep * SL_RAD_PER_STEP) * direction;
                    my = me->GetPositionY() + (SL_DIST_FIRST+SL_DIST_NEXT * i) * sin(mstep * SL_RAD_PER_STEP);
                    mz = me->GetPositionZ();
                    pTemp = me->SummonCreature(48855, mx,my,mz,0,TEMPSUMMON_MANUAL_DESPAWN,0);
                    if (!pTemp)
                        continue;

                    pTemp->SetSpeed(MOVE_RUN, SL_FURTH_SPEED/(SL_DIST_FIRST+SL_DIST_NEXT * (SL_COUNT-i)), true);
                    pTemp->AI()->SetData(1, clockwise); // direction
                    pTemp->AI()->SetData(2, slot); // slot

                    mstep += 1;
                    mx = me->GetPositionX() + (SL_DIST_FIRST+SL_DIST_NEXT * i) * cos(mstep * SL_RAD_PER_STEP) * direction;
                    my = me->GetPositionY() + (SL_DIST_FIRST+SL_DIST_NEXT * i) * sin(mstep * SL_RAD_PER_STEP);

                    // Define actual point as 1000*position+step
                    pTemp->GetMotionMaster()->MovePoint(mstep+1000*i, mx, my, mz);
                }
            }

            void DoIceStorm()
            {
                Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0);
                if (pTarget)
                {
                    Creature* pStorm = me->SummonCreature(46973, pTarget->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ());
                    if (pStorm)
                    {
                        pStorm->AddThreat(pTarget, 50000.0f);
                        pStorm->Attack(pTarget, true);
                        pStorm->GetMotionMaster()->MoveChase(pTarget);
                    }
                }
            }

            void DoLightningVisual(float ori) // 48977 Lightning Strike Trigger
            {
                float r = 67.0f;
                for (int i = 0; i < 12; i++)
                {
                    float add = urand(0,157);
                    float angle = ori + (add/100) - (M_PI/4); // 90 degrees cone
                    if (i == 1)
                        angle = ori - (M_PI/4);
                    else if (i == 2)
                        angle = ori + (M_PI/4);
                    float x = me->GetPositionX() + cos(angle) * r;
                    float y = me->GetPositionY() + sin(angle) * r;
                    float z = me->GetPositionZ();
                    Creature* pLightning = me->SummonCreature(48977, x, y, z, 0, TEMPSUMMON_TIMED_DESPAWN, 3000);
                    if (pLightning)
                    {
                        pLightning->SetVisibility(VISIBILITY_OFF);
                        pLightning->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                        me->CastSpell(pLightning, SPELL_LIGHTNING_STRIKE_VISUAL, true); // 88230 visual spell
                    }
                }
            }

            void DespawnStormlings()
            {
                if (map)
                {
                    while (!m_stormlings.empty())
                    {
                        Creature *summon = map->GetCreature(*m_stormlings.begin());
                        if (summon)
                            summon->DisappearAndDie();
                        m_stormlings.erase(m_stormlings.begin());
                    }
                    if (m_relentless_guid)
                    {
                        Creature *trigger = map->GetCreature(m_relentless_guid);
                        if (trigger)
                            trigger->DisappearAndDie();
                        m_relentless_guid = 0;
                    }
                }
            }

        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new boss_alakirAI(c);
        }
};

class npc_ice_storm: public CreatureScript
{
    public:
        npc_ice_storm(): CreatureScript("npc_ice_storm") {}

        struct npc_ice_stormAI: public ScriptedAI
        {
            npc_ice_stormAI(Creature* c): ScriptedAI(c)
            {
                c->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                c->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                c->setFaction(14);
                Reset();
            }

            uint32 disappearTimer;
            uint32 dropTimer;

            void Reset()
            {
                me->CastSpell(me, SPELL_ICE_STORM_TRIGGER, true);
                me->SetSpeed(MOVE_RUN, 0.5f, true);
                disappearTimer = 20000;
                dropTimer = 2000;
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (dropTimer <= diff)
                {
                    Unit* summon = me->SummonCreature(46766, 0, 0, 0);
                    summon->SetDisplayId(me->GetDisplayId());
                    me->CastSpell(me, SPELL_ICE_STORM_TRIGGER, true); // visual
                    dropTimer = 2000;
                } else dropTimer -= diff;

                if (disappearTimer <= diff)
                {
                    disappearTimer = 10000;
                    me->Kill(me, false);
                    me->ForcedDespawn();
                } else disappearTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_ice_stormAI(c);
        }
};

class npc_ice_storm_ground: public CreatureScript
{
    public:
        npc_ice_storm_ground(): CreatureScript("npc_ice_storm_ground") {}

        struct npc_ice_storm_groundAI: public Scripted_NoMovementAI
        {
            npc_ice_storm_groundAI(Creature* c): Scripted_NoMovementAI(c)
            {
                c->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                c->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                c->SetReactState(REACT_PASSIVE);
                c->setFaction(14);
                Reset();
            }

            uint32 disappearTimer;

            void Reset()
            {
                me->CastSpell(me, SPELL_ICE_STORM_GROUND_AURA, false);
                disappearTimer = 20000;
            }

            void UpdateAI(const uint32 diff)
            {
                if (!me->HasAura(SPELL_ICE_STORM_GROUND_AURA) && !me->IsNonMeleeSpellCasted(true))
                    me->CastSpell(me, SPELL_ICE_STORM_GROUND_AURA, false);

                if (disappearTimer <= diff)
                {
                    disappearTimer = 10000;
                    me->Kill(me, false);
                    me->ForcedDespawn();
                } else disappearTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_ice_storm_groundAI(c);
        }
};

class npc_squall_vortex: public CreatureScript
{
    public:
        npc_squall_vortex(): CreatureScript("npc_squall_vortex") {}

        struct npc_squall_vortexAI: public ScriptedAI
        {
            npc_squall_vortexAI(Creature* c): ScriptedAI(c)
            {
                c->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                c->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                c->setFaction(14);
                direction = 1.0f;
                slot = 0;
                Reset();
            }

            uint32 phaseTimer;
            uint64 boss_guid;
            std::list<Unit*> pVehicle_list;

            float nextx,nexty,nextz,nextpoint;
            uint8 nextpos;
            bool needstomove;
            uint8 phase;
            float direction;
            uint32 slot;

            void Reset()
            {
                me->AddAura(SPELL_SQUALL_LINE_INIT, me); // triggers SPELL_SQUALL_LINE_VISUAL after 5 sec
                phaseTimer = 5000;
                pVehicle_list.clear();
                needstomove = false;
                phase = 1;
                if (TempSummon* summon = me->ToTempSummon())
                    boss_guid = summon->GetSummonerGUID();
                else
                    boss_guid = 0;
            }

            void SetData(uint32 id, uint32 value)
            {
                if (id == 1)
                {
                    if (value) // if clockwise (counter-natural sense)
                        direction = -1.0f;
                }
                else if (id == 2)
                    slot = value;
            }

            void AttackStart(Unit* /*who*/) { }

            void MoveInLineOfSight(Unit* pWho)
            {
                if (!pWho || pWho->GetVehicle() || pWho->GetTypeId() != TYPEID_PLAYER || pWho->isDead() || (phase != 2) || pWho->GetDistance2d(me) > 2.5f)
                    return;

                Player* pPassenger = pWho->ToPlayer();
                Unit* pVehicle = me->SummonCreature(48852, 0, 0, 0);
                pVehicle->SetDisplayId(me->GetDisplayId());
                pVehicle->setFaction(14);
                pVehicle->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_SCHOOL_DAMAGE, true);
                pVehicle->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
                pVehicle->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                Vehicle* pVeh = pVehicle->GetVehicleKit();
                if (pVeh)
                {
                    if (Aura* aura = pPassenger->AddAura(SPELL_SQUALL_LINE_VEHICLE, pPassenger))
                        aura->SetDuration(phaseTimer);
                    pPassenger->EnterVehicle(pVehicle,0);
                    pPassenger->ClearUnitState(UNIT_STATE_UNATTACKABLE); // applied when entering vehicle
                    pVehicle->GetMotionMaster()->MoveFollow(me, 0.0f, 0.0f);
                    pVehicle->SetSpeed(MOVE_RUN, me->GetSpeed(MOVE_RUN), true);
                }
                pVehicle_list.push_back(pVehicle);
            }

            void MovementInform(uint32 type, uint32 id)
            {
                // Get actual position
                uint32 mypos = id/1000;
                uint32 mypoint = id%1000;

                Creature* pBoss = NULL;
                if (boss_guid)
                    pBoss = me->GetCreature(*me, boss_guid);
                if (!pBoss)
                    return;

                // And do the trick
                mypoint += 1;

                nextx = pBoss->GetPositionX() + (SL_DIST_FIRST+SL_DIST_NEXT * mypos) * cos(mypoint * SL_RAD_PER_STEP) * direction;
                nexty = pBoss->GetPositionY() + (SL_DIST_FIRST+SL_DIST_NEXT * mypos) * sin(mypoint * SL_RAD_PER_STEP);
                nextz = pBoss->GetPositionZ();

                nextpos = mypos;
                nextpoint = mypoint;
                needstomove = true;
                pBoss->AI()->SetData(mypos*10+slot, mypoint);
            }

            void UpdateAI(const uint32 diff)
            {
                if (phaseTimer <= diff)
                {
                    switch (phase)
                    {
                    case 1:
                        phaseTimer = 40000;
                        phase++;
                        break;
                    case 2:
                        me->RemoveAurasDueToSpell(SPELL_SQUALL_LINE_VISUAL);
                        phase++;
                        for(std::list<Unit*>::const_iterator itr = pVehicle_list.begin(); itr != pVehicle_list.end(); ++itr)
                        {
                            (*itr)->Kill(*itr, false);
                            if ((*itr)->ToCreature())
                                (*itr)->ToCreature()->ForcedDespawn();
                        }
                        pVehicle_list.clear();
                        phaseTimer = 5000;
                        break;
                    case 3:
                        me->Kill(me, false);
                        me->ForcedDespawn();
                        // For being sure when doing multithreading
                        // should not happen, but who knows
                        phaseTimer = 10000;
                        phase++;
                        break;
                    default: break;
                    }
                }
                else phaseTimer -= diff;

                if (needstomove)
                {
                    Creature* pBoss = NULL;
                    if (boss_guid)
                        pBoss = me->GetCreature(*me, boss_guid);
                    if (!pBoss)
                        return;

                    uint32 sum = 0;
                    for (uint32 i = 0; i < SL_COUNT; i++)
                        sum += pBoss->AI()->GetData(i*10+slot);

                    if (sum >= nextpoint*(SL_COUNT-2))
                    {
                        me->GetMotionMaster()->MovePoint(nextpos*1000+nextpoint, nextx, nexty, nextz);
                        if (!pVehicle_list.empty())
                            for(std::list<Unit*>::const_iterator itr = pVehicle_list.begin(); itr != pVehicle_list.end(); ++itr)
                                (*itr)->GetMotionMaster()->MovePoint(nextpos*1000+nextpoint, nextx, nexty, nextz);
                        needstomove = false;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_squall_vortexAI(c);
        }
};

class npc_lightning_clouds: public CreatureScript
{
    public:
        npc_lightning_clouds(): CreatureScript("npc_lightning_clouds") {}

        struct npc_lightning_cloudsAI: public Scripted_NoMovementAI
        {
            npc_lightning_cloudsAI(Creature* c): Scripted_NoMovementAI(c)
            {
                c->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                c->SetReactState(REACT_PASSIVE);
                c->setFaction(14);

                c->SetFlying(true);
                c->GetMotionMaster()->MovePoint(666, c->GetPositionX(), c->GetPositionY(), c->GetPositionZ());
                WorldPacket data;
                c->BuildHeartBeatMsg(&data);
                c->SendMessageToSet(&data, false);

                c->CastSpell(me, SPELL_LIGHTNING_CLOUDS, true);

                Reset();
            }

            void Reset() { }
            void EnterEvadeMode() { }
            void DamageTaken(Unit* /*who*/, uint32 &damage) { damage = 0; }

            void SpellHit(Unit* /*caster*/, const SpellEntry* spell_info)
            {
                if (spell_info && spell_info->Id == 89567) // not working blizz aura
                {
                    me->AddAura(SPELL_LIGHTNING_CLOUDS_DMG, me);

                    for (int i = 0; i < 4; i++)
                    {
                        float x, y, z, d, r, a;

                        d = float(urand(0,100)) / 100.0f;
                        r = 9.75f * sqrt(d); // todo: increase along with range of visual dummy spells

                        a = float(urand(0,628)) / 100.0f;

                        x = me->GetPositionX() + sin(a) * r;
                        y = me->GetPositionY() + cos(a) * r;
                        z = me->GetPositionZ() + float(urand(0,4)) - 2.0f;

                        if (Creature * summon = me->SummonCreature(SUMMON_LIGHTNING_CLOUDS_EX, x, y, z, 0))
                        {
                            summon->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_SCHOOL_DAMAGE, true);
                            summon->SetReactState(REACT_PASSIVE);
                            summon->setFaction(14);

                            summon->SetFlying(true);
                            summon->GetMotionMaster()->MovePoint(666, summon->GetPositionX(), summon->GetPositionY(), summon->GetPositionZ());
                            WorldPacket heartbeat;
                            summon->BuildHeartBeatMsg(&heartbeat);
                            summon->SendMessageToSet(&heartbeat, false);
                        }
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_lightning_cloudsAI(c);
        }
};

class npc_alakir_large_model: public CreatureScript
{
    public:
        npc_alakir_large_model(): CreatureScript("npc_alakir_large_model") {}

        struct npc_alakir_large_modelAI: public Scripted_NoMovementAI
        {
            npc_alakir_large_modelAI(Creature* c): Scripted_NoMovementAI(c)
            {
                c->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
                c->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
                c->ApplySpellImmune(0, IMMUNITY_ID, 49560, true); // Death Grip jump effect
                c->ApplySpellImmune(0, IMMUNITY_ID, 81261, true); // Solar Beam
                c->ApplySpellImmune(0, IMMUNITY_ID, 88625, true); // Chastise

                Reset();
            }

            uint32 lightningTimer;
            uint32 windBurstTimer;
            uint32 lightningRodTimer;
            uint32 lightningCloudTimer;

            void Reset()
            {
                me->AddAura(SPELL_RELENTLESS_STORM_CHANNEL, me);
                me->setFaction(14);

                me->SetFlying(true);
                me->GetMotionMaster()->MovePoint(666,me->GetPositionX(),me->GetPositionY(),me->GetPositionZ());
                WorldPacket data;
                me->BuildHeartBeatMsg(&data);
                me->SendMessageToSet(&data, false);

                lightningTimer = 3000;
                windBurstTimer = 20000;
                lightningRodTimer = 9000;
                lightningCloudTimer = 30000;

                // do initial Lightning Clouds at the bottom
                DoLightningCloud(188.0f);
            }

            void EnterEvadeMode()
            {
                me->ForcedDespawn();
            }

            void JustDied(Unit* /*killer*/)
            {
                DoScriptText(-1850535, me);
            }

            void DamageTaken(Unit*, uint32 &damage)
            {
                if (damage > me->GetHealth())
                {
                    // Four Play
                    if (me->HasAura(SPELL_FEEDBACK) ||
                        me->HasAura(101458) ||
                        me->HasAura(101459) ||
                        me->HasAura(101460))
                    {
                        Map* map = me->GetMap();
                        const AchievementEntry* achiev = sAchievementStore.LookupEntry(5305);
                        if (map && achiev)
                        {
                            Map::PlayerList const &plList = map->GetPlayers();
                            if (!plList.isEmpty())
                                for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
                                    if (Player *player = itr->getSource())
                                        player->GetAchievementMgr().CompletedAchievement(achiev);
                        }
                    }
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

            // Lightning
                if (lightningTimer <= diff)
                {
                    Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0);
                    if (target)
                        me->CastSpell(target, SPELL_LIGHTNING, false);
                    lightningTimer = 3000;

                } else lightningTimer -= diff;
            // Wind Burst
                if (windBurstTimer <= diff)
                {
                    DoScriptText(-1850534, me);

                    me->CastSpell(me, SPELL_WIND_BURST_THIRD_PHASE, false);
                    windBurstTimer = 20000;

                } else windBurstTimer -= diff;
            // Lightning Rod
                if (lightningRodTimer <= diff)
                {
                    Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0);
                    if (target)
                        me->AddAura(SPELL_LIGHTNING_ROD, target);
                    lightningRodTimer = 15000;

                } else lightningRodTimer -= diff;
            // Lightning Cloud
                if (lightningCloudTimer < diff)
                {
                    me->CastSpell(me, SPELL_LIGHTNING_CLOUDS_ALERT, true);
                    Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0);
                    if (target)
                    {
                        float altitude = target->GetPositionZ();
                        DoLightningCloud(altitude);
                    }
                    lightningCloudTimer = 30000;

                } else lightningCloudTimer -= diff;
            }

            void DoLightningCloud(float z)
            {
                // fill the given altitude with clouds
                for (int i = 0; i < 50; i++)
                {
                    float x, y, d, r, a;

                    // circle will be filled even
                    d = float(urand(0,100)) / 100.0f;
                    r = 125.0f * sqrt(d);

                    a = float(urand(0,628)) / 100.0f;

                    x = me->GetPositionX() + sin(a) * r;
                    y = me->GetPositionY() + cos(a) * r;
                    z += float(urand(0,8)) - 4.0f;

                    me->SummonCreature(SUMMON_LIGHTNING_CLOUDS, x, y, z, 0);
                }
            }


        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_alakir_large_modelAI(c);
        }
};


void AddSC_alakir()
{
    new boss_alakir();
    new npc_ice_storm();
    new npc_ice_storm_ground();
    new npc_squall_vortex();
    new npc_lightning_clouds();
    new npc_alakir_large_model();
}

/*
SQL:

UPDATE creature_template SET vehicleId=342,AIName='NullAI' WHERE entry=48852;
UPDATE creature_template SET vehicleId=0,ScriptName='npc_squall_vortex' WHERE entry=48855;
UPDATE creature_template SET ScriptName='npc_ice_storm' WHERE entry=46973;
UPDATE creature_template SET ScriptName='npc_ice_storm_ground' WHERE entry=46766;
UPDATE creature_template SET ScriptName='boss_alakir', modelid1=35248 WHERE entry=46753;
UPDATE creature_template SET ScriptName='npc_lightning_clouds' WHERE entry=48190;
UPDATE creature_template SET ScriptName='npc_alakir_large_model', name="Al'akir" WHERE entry=48233;
UPDATE creature_model_info SET bounding_radius=35.0, combat_reach=45.0 WHERE modelid=35248;
UPDATE creature_model_info SET bounding_radius=250.0, combat_reach=250.0 WHERE modelid=36062;

DELETE FROM `script_texts` WHERE entry IN (-1850527,-1850528,-1850529,-1850530,-1850531,-1850532,-1850533,-1850534,-1850535);
INSERT INTO `script_texts`
(`npc_entry`, `entry`, `content_default`, `content_loc1`, `content_loc2`, `content_loc3`, `content_loc4`, `content_loc5`, `content_loc6`, `content_loc7`, `content_loc8`, `sound`, `type`, `language`, `emote`, `comment`)
VALUES
(46753, -1850527, "Your challenge is accepted, mortals! Know that you face Al'Akir, Elemental Lord of Air! You have no hope of defeating me!", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_AGGRO'),
(46753, -1850528, "Winds! Obey my command!", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_WIND_BURST'),
(46753, -1850529, "Your futile persistence angers me.", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_PHASE_TWO'),
(46753, -1850530, "Storms! I summon you to my side!", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_SQUALL_LINE'),
(46753, -1850531, "ENOUGH! I WILL NO LONGER BE CONTAINED!", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_PHASE_THREE'),
(46753, -1850532, "Like swatting insects...", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_KILL_1'),
(46753, -1850533, "This little one will vex me no longer.", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_KILL_2'),
(48233, -1850534, "Winds! Obey my command!", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_WIND_BURST'),
(48233, -1850535, "After every storm... comes the calm...", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_DEATH');

UPDATE `gameobject_template` SET data0=43 WHERE entry=207891;

*/
