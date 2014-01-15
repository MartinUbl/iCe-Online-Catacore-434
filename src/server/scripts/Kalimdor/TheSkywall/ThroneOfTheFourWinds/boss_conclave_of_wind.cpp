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


struct npc_conclave_bossAI : public ScriptedAI
{
public:
    npc_conclave_bossAI(Creature *c) : ScriptedAI(c)
    {
        c->SetReactState(REACT_DEFENSIVE);
        pInstance = c->GetInstanceScript();

        // immunity to Toxic Spores
        c->ApplySpellImmune(0, IMMUNITY_ID, 86282, true);
        c->ApplySpellImmune(0, IMMUNITY_ID, 93120, true);
        c->ApplySpellImmune(0, IMMUNITY_ID, 93121, true);
        c->ApplySpellImmune(0, IMMUNITY_ID, 93122, true);

        c->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
        c->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
        c->ApplySpellImmune(0, IMMUNITY_ID, 49560, true); // Death Grip jump effect
        c->ApplySpellImmune(0, IMMUNITY_ID, 81261, true); // Solar Beam
        c->ApplySpellImmune(0, IMMUNITY_ID, 88625, true); // Chastise
    }

    bool alive;
    bool valid_targets;
    bool lock;  // locks UpdateAI fuction from ScriptMgr calls dependent on grid system
                // unlocked and called via Instnce Script
    uint32 platform_check_timer;
    uint32 platform_spell_timer;
    InstanceScript *pInstance;

    void conclave_NoPlayersOnPlatform()
    {
        TeleHome();
        me->GetMotionMaster()->Clear(true);
        uint32 spellID = 0;
        int32 textID = 0;
        switch(me->GetEntry())
        {
        case NPC_ANSHAL: spellID = 85576; textID = -1850513; break; // spell Withering Winds
        case NPC_NEZIR:  spellID = 85578; textID = -1850523; break; // spell Chilling Winds
        case NPC_ROHASH: spellID = 85573; textID = -1850517; break; // spell Deafening Winds
        default: break;
        }
        me->CastSpell(me, spellID, false);
        DoScriptText(textID, me);
    }

    void conclave_SetPower(int32 value)
    {
        me->SetPower(me->getPowerType(), value);
    }

    void Reset()
    {
        valid_targets = true;
        platform_spell_timer = 1000;
        platform_check_timer = 3000;
        alive = true;
        lock = true;

        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        AurasCleanup();
    }

    void EnterCombat(Unit* /*who*/)
    {
        if(!valid_targets)
            return;

        if(pInstance)
            pInstance->SetData(TYPE_CONCLAVE, IN_PROGRESS);

        switch(me->GetEntry())
        {
        case NPC_ANSHAL: DoScriptText(-1850510, me); break;
        case NPC_NEZIR:  DoScriptText(-1850520, me); break;
        case NPC_ROHASH: DoScriptText(-1850516, me); break;
        default: break;
        }
    }

    void KilledUnit(Unit* /*who*/)
    {
        switch(me->GetEntry())
        {
        case NPC_ANSHAL: DoScriptText((urand(0,1) ? -1850511 : -1850512), me); break;
        case NPC_NEZIR:  DoScriptText((urand(0,1) ? -1850521 : -1850522), me); break;
        default: break;
        }
    }

    void EnterEvadeMode()
    {
        // not to evade on my own. (start ultimate ability while no targets present on my platform)
        if(pInstance)
        {
            uint32 data = pInstance->GetData(TYPE_CONCLAVE);
            if(data == IN_PROGRESS || data == SPECIAL)
                return;
        }
        ScriptedAI::EnterEvadeMode();
    }

    void TeleHome()
    {
        float x, y, z, o;
        me->GetRespawnCoord(x, y, z, &o);

        //me->NearTeleportTo(x, y, z, o);
        //me->Relocate(x, y, z, o);
        //WorldPacket heart;
        //me->BuildHeartBeatMsg(&heart);
        //me->SendMessageToSet(&heart, false);
        DoTeleportTo(x, y, z, 0);
    }

    void AurasCleanup()
    {
        me->RemoveAurasDueToSpell(85576);
        me->RemoveAurasDueToSpell(93181);
        me->RemoveAurasDueToSpell(93182);
        me->RemoveAurasDueToSpell(93183);

        me->RemoveAurasDueToSpell(85573);
        me->RemoveAurasDueToSpell(93190);
        me->RemoveAurasDueToSpell(93191);
        me->RemoveAurasDueToSpell(93192);

        me->RemoveAurasDueToSpell(85578);
        me->RemoveAurasDueToSpell(93147);
        me->RemoveAurasDueToSpell(93148);
        me->RemoveAurasDueToSpell(93149);
    }

    void DamageTaken(Unit* pDoneBy, uint32 &damage)
    {
        if(!alive)
        {
            damage = 0;
        }
        else if(damage >= me->GetHealth())
        {
            damage = 0;
            if(pInstance)
            {
                if(pInstance->GetData(TYPE_CONCL_DEAD)+1 == 3) // two of conclave have already died and have not ressed yet
                                                               // third will be added on SetData DONE
                {
                    // instance binding + loot
                    if(Map* map = me->GetMap())
                    {
                        // kill all bosses (they are Gathering Strenght)
                        if(Creature* anshal = map->GetCreature(pInstance->GetData64(NPC_ANSHAL)))
                            pDoneBy->Kill(anshal);
                        if(Creature* nezir = map->GetCreature(pInstance->GetData64(NPC_NEZIR)))
                            pDoneBy->Kill(nezir);
                        if(Creature* rohash = map->GetCreature(pInstance->GetData64(NPC_ROHASH)))
                            pDoneBy->Kill(rohash);
                    }

                    pInstance->SetData(TYPE_CONCLAVE, DONE);
                    return;
                }
                else
                {
                    TeleHome();
                    AurasCleanup();

                    switch(me->GetEntry())
                    {
                    case NPC_ANSHAL: DoScriptText(-1850515, me); break;
                    case NPC_NEZIR:  DoScriptText(-1850525, me); break;
                    case NPC_ROHASH: DoScriptText(-1850519, me); break;
                    default: break;
                    }
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    me->CastSpell(me, 86307, false); // Gather Strength (1 minute ressurection timer)
                    me->SetHealth(1);
                    me->AI()->DoAction(0); // set power to 0

                    alive = false;
                    pInstance->SetData(TYPE_CONCL_DEAD, SPECIAL); // may call EnterEvadeMode() and Reset() in effect
                }
            }
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if(!alive)
            return;

        if(platform_spell_timer < diff)
        {
            me->CastSpell(me, 85763, true); // Distance Checker spell
            platform_spell_timer = 1000;
        } else platform_spell_timer -= diff;

        if(!valid_targets)
            return;

        if(platform_check_timer < diff)
        {
            // no target has been hit by Distance Checker spell. no valid targets are present on the platform then.
            valid_targets = false;
            conclave_NoPlayersOnPlatform();
        } else platform_check_timer -= diff;
    }

    void SpellHitTarget(Unit* pTarget, const SpellEntry* spellInfo)
    {
        if(spellInfo && spellInfo->Id == 85763) // Distance Checker spell
        {
            if(!valid_targets)
            {
                if(pTarget && me->GetEntry() != NPC_ROHASH)
                    me->GetMotionMaster()->MoveChase(pTarget);

                AurasCleanup();
                valid_targets = true;
            }
            // reset / extend timer of valid targets expiration
            platform_check_timer = 3000;
        }

        if(pTarget == (Unit *)me && spellInfo && spellInfo->Id == 86307) // Gather Strength
        {
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            alive = true;
            if(pInstance)
                pInstance->SetData(TYPE_CONCL_DEAD, IN_PROGRESS);
        }
    }

    bool CheckLock()
    {
        if(lock)
            return false;
        else
        {
            lock = true;
            return true;
        }
    }

    bool UpdateVictim()
    {
        if(!me->isInCombat())
            return false;

        // not to attack invalid target out of the platform
        // (could happen if there still were valid targets but top aggro target changed platform etc.)

        for(int i = 0; i < 10; i++) // max 10 times not to freeze here
                                    // 10 should be sufficient. if not, we could loose 1 or 2 updates
        {
            if(Unit *victim = me->SelectVictim())
            {
                if(me->GetDistance(victim) > 60.0f)
                {
                    // select someone else on next cycle
                    me->getThreatManager().modifyThreatPercent(victim, -99.99f);
                    me->RemoveAurasByType(SPELL_AURA_MOD_TAUNT, victim->GetGUID());
                }
                // target is on our platform
                else
                {
                    AttackStart(victim);
                    return me->getVictim();
                }
            }
            else
                return false;
        }
        return false;
    }

};

// overloaded AIs
class boss_anshal_conclave : public CreatureScript
{
public:
    boss_anshal_conclave() : CreatureScript("boss_anshal_conclave") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_anshal_conclaveAI (pCreature);
    }

    struct boss_anshal_conclaveAI : public npc_conclave_bossAI
    {
        boss_anshal_conclaveAI(Creature *c) : npc_conclave_bossAI(c)
        {
            c->setPowerType(POWER_ENERGY);
            c->SetMaxPower(POWER_ENERGY, 90);
        }
/*
abilities
Nurture - summons       _OK
Soothing Breeze         _OK
Zephyr (ultimate)       _OK
*/

        void conclave_SetPower(int32 value)
        {
            npc_conclave_bossAI::conclave_SetPower(value);

            // energy removing steps by three (87, 84, 81, .. , 33, 30, 27, .. , 6, 3, 0)
            if(value == 19)
            {
                // nurture
                me->CastSpell(me, 85422, false); // triggers 85425 5 times (summons triggers)
            }
            else if(value == 8 || value == 71)
            {
                // soothing breeze
                me->CastSpell(me, 86205, false); // triggers summoning spell
            }
        }

        void conclave_DoUltimate()
        {
            // Zephyr
            me->CastSpell(me, 84638, true);
            DoScriptText(-1850514, me);
        }

        void Reset()
        {
            npc_conclave_bossAI::Reset();
            conclave_SetPower(10);
        }

        void UpdateAI(const uint32 diff)
        {
            if(!CheckLock())
                return;

            npc_conclave_bossAI::UpdateAI(diff);

            if(!UpdateVictim() || !alive || !valid_targets/* || me->hasUnitState(UNIT_STAT_CASTING)*/)
                return;

            DoMeleeAttackIfReady();
        }

        void DoAction(const int32 param)
        {
            if(!alive)
                return;

            switch(param)
            {
            case 666: // Do Ultimate Ability
                TeleHome();
                conclave_DoUltimate();
                break;
            case 667: // unlock UpdateAI()
                lock = false;
                break;
            default:
                if(param <= 90 && param >= 0)
                    conclave_SetPower(param);
                break;
            }
        }

    };
};

class boss_nezir_conclave : public CreatureScript
{
public:
    boss_nezir_conclave() : CreatureScript("boss_nezir_conclave") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_nezir_conclaveAI (pCreature);
    }

    struct boss_nezir_conclaveAI : public npc_conclave_bossAI
    {
        boss_nezir_conclaveAI(Creature *c) : npc_conclave_bossAI(c)
        {
            c->setPowerType(POWER_RUNIC_POWER);
            c->SetMaxPower(POWER_RUNIC_POWER, 900); // sets 1/10 of the value for runic power
        }
/*
abilities
Wind Chill              _OK
Permafrost              _OK
Ice Patch               _OK
Sleet Storm (ultimate)  _OK
*/

        uint32 spell_windchill_timer;
        uint32 spell_permafrost_timer;
        uint32 spell_icepatch_timer;

        void conclave_DoUltimate()
        {
            // Sleet Storm
            me->CastSpell(me, 84644, true);
            DoScriptText(-1850524, me);
        }

        void conclave_SetPower(int32 value)
        {
            me->SetPower(me->getPowerType(), value*10); // sets 1/10 of the value for runic power
        }

        void Reset()
        {
            npc_conclave_bossAI::Reset();
            conclave_SetPower(10);

            spell_windchill_timer = 4000;
            spell_permafrost_timer = 6000;
            spell_icepatch_timer = 8000;
        }

        void UpdateAI(const uint32 diff)
        {
            if(!CheckLock())
                return;

            npc_conclave_bossAI::UpdateAI(diff);

            if(!UpdateVictim() || !alive || !valid_targets || me->hasUnitState(UNIT_STAT_CASTING))
                return;

            if(spell_windchill_timer < diff)
            {
                me->CastSpell(me, 84645, false);
                spell_windchill_timer = 10000;
            } else spell_windchill_timer -= diff;

            if(spell_permafrost_timer < diff)
            {
                me->CastSpell(me, 86082, true);
                spell_permafrost_timer = 11000;
            } else spell_permafrost_timer -= diff;

            if(spell_icepatch_timer < diff)
            {
                if(Unit* pTarget = me->AI()->SelectTarget(SELECT_TARGET_RANDOM, 0, 40.0f))
                {
                    me->CastSpell(pTarget, 86122, false); // summons Ice Patch npc
                    spell_icepatch_timer = 15000;
                } else spell_icepatch_timer = 4000;
            } else spell_icepatch_timer -= diff;


            DoMeleeAttackIfReady();
        }

        void DoAction(const int32 param)
        {
            if(!alive)
                return;

            switch(param)
            {
            case 666: // Do Ultimate Ability
                TeleHome();
                conclave_DoUltimate();
                break;
            case 667: // unlock UpdateAI()
                lock = false;
                break;
            default:
                if(param <= 90 && param >= 0)
                    conclave_SetPower(param);
                break;
            }
        }
    };
};

class boss_rohash_conclave : public CreatureScript
{
public:
    boss_rohash_conclave() : CreatureScript("boss_rohash_conclave") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_rohash_conclaveAI (pCreature);
    }

    struct boss_rohash_conclaveAI : public npc_conclave_bossAI
    {
        boss_rohash_conclaveAI(Creature *c) : npc_conclave_bossAI(c)
        {
            c->setPowerType(POWER_FOCUS);
            c->SetMaxPower(POWER_FOCUS, 90);
        }
/*
abilities:
adds (tornados)         _OK
Slicing Gale            _OK
Wind Blast              _OK
Storm Shiled (HC)       _OK
Hurricane (ultimate)    _effects OK, visual workaround OK
*/

        uint32 hurricane_end_timer;
        uint32 windblast_update_timer;
        uint8 m_ability_phase; /* 0 = none; 1-13 Wind Blast; 20 Tornados; 30 Storm Shield; 40-42 Hurricane*/

        void AttackStart(Unit* who)
        {
            if(who)
            {
                AttackStartNoMove(who);
                me->SetFacingTo(0.0f); // permanent (turns and always faces victiom)
                me->Attack(who, false);
            }
        }

        void conclave_DoUltimate()
        {
            // Hurricane
            me->CastSpell(me, 84643, true);
            DoScriptText(-1850518, me);
        }

        void conclave_SetPower(int32 value)
        {
            me->SetPower(me->getPowerType(), value);
            // energy removing steps by three (87, 84, 81, .. , 33, 30, 27, .. , 6, 3, 0)
            if(value == 11)
                m_ability_phase = 20;
            else if(value == 28)
            {
                // HC-only Storm Shield
                m_ability_phase = 30;
            }
            else if(value == 55)
            {
                // begin Wind Blast
                m_ability_phase = 1;
            }
            else if(value == 90)
            {
                // begin Hurricane
                m_ability_phase = 40;
            }
        }

        void Reset()
        {
            npc_conclave_bossAI::Reset();
            conclave_SetPower(10);

            hurricane_end_timer = 120000;
            windblast_update_timer = 120000;
            m_ability_phase = 0;
        }

        void DoHurricane()
        {
            // begin
            if(m_ability_phase == 41)
            {
                // Prepare dummy for farsight
                if(Creature* pDummy = me->SummonCreature(WORLD_TRIGGER, me->GetPositionX(), me->GetPositionY()+1, 240.04f, 0, TEMPSUMMON_TIMED_DESPAWN, 15000))
                {
                    //pDummy->SetVisibility(VISIBILITY_OFF);
                    pDummy->setFaction(14);
                    pDummy->SetFacingToObject(me);
                    pDummy->SetFlying(true);
                    pDummy->GetMotionMaster()->MovePoint(0, pDummy->GetPositionX(), pDummy->GetPositionY(), pDummy->GetPositionZ());

                    if(Map* pMap = me->GetMap())
                    {
                        Map::PlayerList const &lPlayers = pMap->GetPlayers();
                        for(Map::PlayerList::const_iterator itr = lPlayers.begin(); itr != lPlayers.end(); ++itr)
                        {
                            if(Player* pPlayer = itr->getSource())
                            {
                                if(me->GetDistance2d(pPlayer) < 80.0f && pPlayer->isAlive())
                                {
                                    pPlayer->SetUInt64Value(PLAYER_FARSIGHT, pDummy->GetGUID());
                                    pPlayer->NearTeleportTo(me->GetPositionX(), me->GetPositionY(), 200.04f, pPlayer->GetOrientation());
                                }
                            }
                        }
                    }
                }
                m_ability_phase = 42;
            }
            // end
            else if(m_ability_phase == 42)
            {
                if(Map* pMap = me->GetMap())
                {
                    Map::PlayerList const &lPlayers = pMap->GetPlayers();
                    for(Map::PlayerList::const_iterator itr = lPlayers.begin(); itr != lPlayers.end(); ++itr)
                    {
                        if(Player* pPlayer = itr->getSource())
                        {
                            if(me->GetDistance2d(pPlayer) < 80.0f)
                            {
                                if(pPlayer->isAlive())
                                {
                                    float a = ((float)urand(0,628))/100;
                                    pPlayer->NearTeleportTo(me->GetPositionX()+10.0f*cos(a), me->GetPositionY()+10.0f*sin(a), 220.04f, pPlayer->GetOrientation());
                                }
                            }
                            pPlayer->SetUInt64Value(PLAYER_FARSIGHT, 0);
                        }
                    }
                }
                m_ability_phase = 0;
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if(!CheckLock())
                return;

            npc_conclave_bossAI::UpdateAI(diff);

            if(!alive || !valid_targets)
                return;

            if(m_ability_phase == 0 || m_ability_phase > 15) // Not to call UpdateVictim() while casting Wind Blast
                if(!UpdateVictim())
                    return;

            // Hurricane end
            if(hurricane_end_timer < diff)
            {
                DoHurricane();
                hurricane_end_timer = 120000;
            } else hurricane_end_timer -= diff;

            // Wind Blast update
            if(windblast_update_timer < diff)
            {
                if(m_ability_phase == 2) // begin Wind Blast attack
                {
                    me->CastSpell(me, 85480, false);
                    windblast_update_timer = 500;
                    m_ability_phase++;
                }
                else if(m_ability_phase == 13) // end Wind Blast
                {
                    if(Unit* victim = me->SelectVictim())
                        AttackStart(victim);
                    windblast_update_timer = 120000;
                    m_ability_phase = 0;
                }
                else if (m_ability_phase > 2 && m_ability_phase < 13) // spin around the platform
                {
                    float o = me->GetOrientation();
                    // 120deg over 6sec
                    o -= 0.175f;
                    (o < 0) ? o += 6.28f : o = o;

                    // one-shot
                    me->SetOrientation(o);
                    WorldPacket data;
                    me->BuildHeartBeatMsg(&data);
                    me->SendMessageToSet(&data, false);

                    windblast_update_timer = 510;
                    m_ability_phase++;
                }
            } else windblast_update_timer -= diff;

            if(!me->isAttackReady()) // not to interrupt current spell attack
                return;

            if(m_ability_phase == 1)
            {
                me->AttackStop();
                me->GetMotionMaster()->MoveIdle();

                if(Player* target = SelectRandomPlayer(50.0f))
                {
                    // one-shot
                    me->SetFacingToObject(target);
                }
                else
                {
                    // one-shot
                    me->SetOrientation(urand(0,628)/100.0f);
                    WorldPacket data;
                    me->BuildHeartBeatMsg(&data);
                    me->SendMessageToSet(&data, false);
                }
                me->CastSpell(me, 86193, false); // Wind Blast init spell
                windblast_update_timer = 5000;
                m_ability_phase = 2;
                return;
            }
            else if(m_ability_phase == 20)
            {
                me->CastSpell(me, 86192, false); // summon three tornados
                m_ability_phase = 0;
            }
            else if(m_ability_phase == 30)
            {
                switch(this->getDifficulty()) // Storm Shield (HC-only ability)
                {
                case RAID_DIFFICULTY_10MAN_HEROIC:
                    me->CastSpell(me, 93059, true);
                    break;
                case RAID_DIFFICULTY_25MAN_HEROIC:
                    me->CastSpell(me, 95865, true);
                    break;
                default: break;
                }
                m_ability_phase = 0;
            }
            else if(m_ability_phase == 40)
            {
                m_ability_phase = 41;
                hurricane_end_timer = 15000;
                DoHurricane();
            }

            // DoSpellAttackIfReady(86182); // changed to random target:
            if (!me->hasUnitState(UNIT_STAT_CASTING) && me->isAttackReady())
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 80.0f))
                {
                    me->Attack(target, false);
                    me->CastSpell(target, 86182, false); // standard spell attack Slicing Gale
                    me->resetAttackTimer();
                }
            }

        }

        void DoAction(const int32 param)
        {
            if(!alive)
                return;

            switch(param)
            {
            case 666: // Do Ultimate Ability
                TeleHome();
                conclave_DoUltimate();
                break;
            case 667: // unlock UpdateAI()
                lock = false;
                break;
            default:
                if(param <= 90 && param >= 0)
                    conclave_SetPower(param);
                break;
            }
        }
    };
};

// summon AIs:
// Ice Patch
class npc_icepatch_conclave : public CreatureScript
{
public:
    npc_icepatch_conclave() : CreatureScript("npc_icepatch_conclave") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_icepatch_conclaveAI (pCreature);
    }

    struct npc_icepatch_conclaveAI : public Scripted_NoMovementAI
    {
        npc_icepatch_conclaveAI(Creature *c) : Scripted_NoMovementAI(c) { kill_timer = 30000; }

        uint32 kill_timer;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            me->CastSpell(me, 86107, true);
            if(TempSummon* summon = me->ToTempSummon())
                summon->SetTempSummonType(TEMPSUMMON_DEAD_DESPAWN);
        }

        void EnterCombat(Unit* /*who*/) { }
        void AttackStart(Unit* /*who*/) { }

        void UpdateAI(const uint32 diff)
        {
            if(kill_timer < diff)
                me->Kill(me);
            else kill_timer -= diff;
        }
    };
};

// Soothing Breeze
class npc_breeze_conclave : public CreatureScript
{
public:
    npc_breeze_conclave() : CreatureScript("npc_breeze_conclave") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_breeze_conclaveAI (pCreature);
    }

    struct npc_breeze_conclaveAI : public Scripted_NoMovementAI
    {
        npc_breeze_conclaveAI(Creature *c) : Scripted_NoMovementAI(c) { kill_timer = 30000; }

        uint32 kill_timer;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            me->CastSpell(me, 86206, true); // triggers healing spells
            me->CastSpell(me, 86207, true); // triggers silence spell
            me->CastSpell(me, 86208, true); // visual
            if(TempSummon* summon = me->ToTempSummon())
                summon->SetTempSummonType(TEMPSUMMON_DEAD_DESPAWN);
        }

        void EnterCombat(Unit* /*who*/) { }
        void AttackStart(Unit* /*who*/) { }

        void UpdateAI(const uint32 diff)
        {
            if(kill_timer < diff)
                me->Kill(me);
            else kill_timer -= diff;
        }
    };
};

// Tornado
class npc_tornado_conclave : public CreatureScript
{
public:
    npc_tornado_conclave() : CreatureScript("npc_tornado_conclave") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_tornado_conclaveAI (pCreature);
    }

    struct npc_tornado_conclaveAI : public PassiveAI
    {
        npc_tornado_conclaveAI(Creature *c) : PassiveAI(c) { kill_timer = 40000; }

        uint32 kill_timer;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            me->CastSpell(me, 86134, true); // aura
            if(TempSummon* summon = me->ToTempSummon())
                summon->SetTempSummonType(TEMPSUMMON_DEAD_DESPAWN);

            me->GetMotionMaster()->MoveRandom(25.0f);
        }

        void MovementInform(uint32, uint32)
        {
            me->GetMotionMaster()->MoveRandom(25.0f);
        }

        void UpdateAI(const uint32 diff)
        {
            if(kill_timer < diff)
                me->Kill(me);
            else kill_timer -= diff;
        }

    };
};

// Ravenous Creeper TRIGGER
class npc_creeper_trigger_conclave : public CreatureScript
{
public:
    npc_creeper_trigger_conclave() : CreatureScript("npc_creeper_trigger_conclave") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_creeper_trigger_conclaveAI (pCreature);
    }

    struct npc_creeper_trigger_conclaveAI : public Scripted_NoMovementAI
    {
        npc_creeper_trigger_conclaveAI(Creature *c) : Scripted_NoMovementAI(c) { }

        uint32 summon_timer;
        // no timed kill needed. summoned for 10 sec duration

        void Reset()
        {
            summon_timer = 9000;
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            me->CastSpell(me, 85428, true); // visual dummy aura
            if(TempSummon* summon = me->ToTempSummon())
                summon->SetTempSummonType(TEMPSUMMON_DEAD_DESPAWN);
        }

        void EnterCombat(Unit* /*who*/) { }
        void AttackStart(Unit* /*who*/) { }

        void UpdateAI(const uint32 diff)
        {
            if(summon_timer < diff)
            {
                me->CastSpell(me, 85429, true); // summon Ravenous Creeper
                summon_timer = 20000;
            } else summon_timer -= diff;
        }
    };
};

// Ravenous Creeper
class npc_ravenous_creeper_conclave : public CreatureScript
{
public:
    npc_ravenous_creeper_conclave() : CreatureScript("npc_ravenous_creeper_conclave") { }

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_ravenous_creeper_conclaveAI(creature);
    }

    struct npc_ravenous_creeper_conclaveAI : public ScriptedAI
    {
        npc_ravenous_creeper_conclaveAI(Creature *c) : ScriptedAI(c)
        {
            if(TempSummon* summon = me->ToTempSummon())
                summon->SetTempSummonType(TEMPSUMMON_DEAD_DESPAWN);
        }

        uint32 spores_timer;

        void Reset()
        {
            spores_timer = 5000;
            me->HandleEmoteCommand(EMOTE_ONESHOT_EMERGE);
        }

        void UpdateAI(const uint32 diff)
        {
            if(spores_timer < diff)
            {
                // Toxic Spores aura
                me->CastSpell(me, 86281, true);
                spores_timer = 20000;
            } else spores_timer -= diff;

            if(!UpdateVictim())
                return;

            if(me->GetDistance2d(me->getVictim()) > 50.0f)
            {
                me->getThreatManager().resetAllAggro();
                me->GetMotionMaster()->MoveTargetedHome();
                return;
            }

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_conclave_of_wind()
{
    new boss_anshal_conclave();
    new boss_nezir_conclave();
    new boss_rohash_conclave();

    new npc_icepatch_conclave();
    new npc_breeze_conclave();
    new npc_tornado_conclave();
    new npc_creeper_trigger_conclave();
    new npc_ravenous_creeper_conclave();
}

/* SQL
UPDATE `creature_template` SET Scriptname="boss_anshal_conclave" WHERE entry=45870;
UPDATE `creature_template` SET Scriptname="boss_nezir_conclave" WHERE entry=45871;
UPDATE `creature_template` SET Scriptname="boss_rohash_conclave" WHERE entry=45872;

UPDATE `creature_template` SET Scriptname="npc_icepatch_conclave", faction_A=14, faction_H=14 WHERE entry=46186;
UPDATE `creature_template` SET Scriptname="npc_breeze_conclave", faction_A=14, faction_H=14 WHERE entry=46246;
UPDATE `creature_template` SET Scriptname="npc_tornado_conclave", faction_A=14, faction_H=14 WHERE entry=46207;
UPDATE `creature_template` SET Scriptname="npc_creeper_trigger_conclave", faction_A=14, faction_H=14 WHERE entry=45813;
UPDATE `creature_template` SET Scriptname="npc_ravenous_creeper_conclave", faction_A=14, faction_H=14 WHERE entry=45812;

INSERT INTO `creature_template_addon` (entry, path_id, mount, bytes1, bytes2, emote, auras) VALUES
(45870, 0, 0, 0, 0, 417, NULL), (45871, 0, 0, 0, 0, 417, NULL), (45872, 0, 0, 0, 0, 417, NULL);

UPDATE `creature_template` SET flags_extra=1 WHERE entry IN (45871, 50098, 50108, 50118, 45872, 50095, 50105, 50115, 45870, 50103, 50113, 50123);
UPDATE `creature_template` SET mechanic_immune_mask=653213695 WHERE entry IN (45871, 50098, 50108, 50118, 45872, 50095, 50105, 50115, 45870, 50103, 50113, 50123);


INSERT INTO `script_texts`
(`npc_entry`, `entry`, `content_default`, `content_loc1`, `content_loc2`, `content_loc3`, `content_loc4`, `content_loc5`, `content_loc6`, `content_loc7`, `content_loc8`, `sound`, `type`, `language`, `emote`, `comment`)
VALUES
-- anshal
(45870, -1850510, "It shall be I that earns the favor of our lord by casting out the intruders. My calmest wind shall prove too much for them!", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_AGGRO'),
(45870, -1850511, "Begone, outsiders!", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 1, 0, 0, 'SAY_KILL_1'),
(45870, -1850512, "Your presence shall no longer defile our home!", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 1, 0, 0, 'SAY_KILL_2'),
(45870, -1850513, "You think to outrun the wind? A fatal mistake.", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_NO_PLAYERS'),
(45870, -1850514, "The power of our winds, UNLEASHED!", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_FULL_POWER'),
(45870, -1850515, "My power grows feeble, brothers. I shamefully must rely on you for a time.", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_DEATH'),
-- rohash
(45872, -1850516, "As I am the strongest wind, it shall be I that tears the invaders apart!", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_AGGRO'),
(45872, -1850517, "Why do you flee, mortals? There is nowhere you can run or hide here!", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_NO_PLAYERS'),
(45872, -1850518, "The power of our winds, UNLEASHED!", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_FULL_POWER'),
(45872, -1850519, "The intruders stand fast, brothers. I cannot break them. Allow me a brief respite to strengthen my winds.", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_DEATH'),
-- nezir
(45871, -1850520, "The honor of slaying the interlopers shall be mine, brothers! Their feeble bodies will freeze solid from my wind's icy chill!", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_AGGRO'),
(45871, -1850521, "Frozen solid.", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 1, 0, 0, 'SAY_KILL_1'),
(45871, -1850522, "Another mortal has taken their last breath!", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 1, 0, 0, 'SAY_KILL_2'),
(45871, -1850523, "You throw away your honor and flee as cowards? Then die!", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_NO_PLAYERS'),
(45871, -1850524, "The power of our winds, UNLEASHED!", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_FULL_POWER'),
(45871, -1850525, "The intruders stand fast, brothers. I cannot break them. Allow me a brief respite to strengthen my winds.", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_DEATH'),
-- alakir
(46753, -1850526, "The Conclave of Wind has dissipated. Your honorable conduct and determination have earned you the right to face me in battle, mortals. I await your assault on my platform! Come!", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 6, 0, 0, 'SAY_CONCL_DEFEAT');

*/
