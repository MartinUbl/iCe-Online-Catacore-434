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
#include "dragonsoul.h"
#include "TaskScheduler.h"

///////////////////////////
///    Morchok stuff    ///
///////////////////////////

enum MorchockTrashNPC
{
    NPC_ANCIENT_WATER_LORD          = 57160,
    NPC_EARTHEN_DESTROYER           = 57158,
    NPC_EARTHEN_SOLDIER             = 57159,
    NPC_TWILIGHT_SIEGE_CAPTAIN      = 57280,
    NPC_TWILIGHT_SIEGE_BREAKER      = 57259,
    NPC_TWILIGHT_PORTAL             = 57231,
};

enum MorchokTrashSpells
{
    SPELL_DRENCHED                  = 107801,
    SPELL_FLOOD_CHANNEL             = 107791,
    SPELL_FLOOD_DMG                 = 107792,
    SPELL_BOULDER_SMASH             = 107596,
    SPELL_BOULDER_SMASH_MISSILE     = 107597,
    SPELL_DUST_STORM                = 107682,
    SPELL_SHADOW_BOLT               = 95440,
    SPELL_TWILIGHT_RAGE             = 107872,
    SPELL_TWILIGHT_CORRUPTION       = 107852,
    SPELL_TWILIGHT_SUBMISSION       = 108183,
    SPELL_TWILIGHT_VOLLEY           = 108172,
    SPELL_TWILIGHT_PORTAL_BEAM      = 108096,
    SPELL_FIERY_EXPLOSION           = 110261,

    ACTION_DISABLE                  = 1,
};

class npc_ds_ancient_water_lord : public CreatureScript
{
public:
    npc_ds_ancient_water_lord() : CreatureScript("npc_ds_ancient_water_lord") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_ancient_water_lordAI(pCreature);
    }

    struct npc_ds_ancient_water_lordAI : public ScriptedAI
    {
        npc_ds_ancient_water_lordAI(Creature *creature) : ScriptedAI(creature) { }

        TaskScheduler scheduler;
        uint32 floodTimer;
        uint32 drenchedTimer;

        void Reset() override
        {
            floodTimer = urand(8000, 12000);
            drenchedTimer = urand(3000, 6000);
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (floodTimer <= diff)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    me->CastSpell(pTarget, SPELL_FLOOD_CHANNEL, false);
                scheduler.Schedule(Seconds(0), [this](TaskContext flood)
                {
                    if (flood.GetRepeatCounter() < 4)
                        flood.Repeat(Seconds(1));
                    me->CastSpell(me, SPELL_FLOOD_DMG, true);
                });
                floodTimer = (Is25ManRaid()) ? urand(10000, 12000) : urand(16000, 20000);
            }
            else floodTimer -= diff;

            if (drenchedTimer <= diff)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    me->CastSpell(pTarget, SPELL_DRENCHED, false);
                drenchedTimer = (Is25ManRaid()) ? urand(7000, 9000) : urand(13000, 15000);
            }
            else drenchedTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_ds_earthen_destroyer : public CreatureScript
{
public:
    npc_ds_earthen_destroyer() : CreatureScript("npc_ds_earthen_destroyer") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_earthen_destroyerAI(pCreature);
    }

    struct npc_ds_earthen_destroyerAI : public ScriptedAI
    {
        npc_ds_earthen_destroyerAI(Creature *creature) : ScriptedAI(creature) { }

        uint32 boulderSmashTimer;
        uint32 dustStormTimer;
        uint32 shadowBoltTimer;

        void Reset() override
        {
            boulderSmashTimer = urand(3000, 5000);
            dustStormTimer = urand(7000, 11000);
            shadowBoltTimer = urand(5000, 9000);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (boulderSmashTimer <= diff)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    me->CastSpell(pTarget, SPELL_BOULDER_SMASH_MISSILE, false);
                boulderSmashTimer = (Is25ManRaid()) ? urand(3000, 5000) : urand(6000, 7000);
            }
            else boulderSmashTimer -= diff;

            if (dustStormTimer <= diff)
            {
                me->CastSpell(me, SPELL_DUST_STORM, false);
                dustStormTimer = (Is25ManRaid()) ? urand(7000, 9000) : urand(15000, 20000);
            }
            else dustStormTimer -= diff;

            if (shadowBoltTimer <= diff)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    me->CastSpell(pTarget, SPELL_SHADOW_BOLT, true);
                shadowBoltTimer = (Is25ManRaid()) ? urand(5000, 7000) : urand(10000, 15000);
            }
            else shadowBoltTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_ds_earthen_soldier : public CreatureScript
{
public:
    npc_ds_earthen_soldier() : CreatureScript("npc_ds_earthen_soldier") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_earthen_soldierAI(pCreature);
    }

    struct npc_ds_earthen_soldierAI : public ScriptedAI
    {
        npc_ds_earthen_soldierAI(Creature *creature) : ScriptedAI(creature) { }

        uint32 twilightCorruptionTimer;
        uint32 twilightRageTimer;
        uint32 shadowBoltTimer;

        void Reset() override
        {
            twilightCorruptionTimer = urand(3000, 5000);
            twilightRageTimer = 10000;
            shadowBoltTimer = urand(5000, 9000);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (twilightCorruptionTimer <= diff)
            {
                me->CastSpell(me, SPELL_TWILIGHT_CORRUPTION, false);
                twilightCorruptionTimer = (Is25ManRaid()) ? urand(6000, 8000) : urand(10000, 12000);
            }
            else twilightCorruptionTimer -= diff;

            if (twilightRageTimer <= diff)
            {
                me->CastSpell(me, SPELL_TWILIGHT_RAGE, false);
                twilightRageTimer = 8000;
            }
            else twilightRageTimer -= diff;

            if (shadowBoltTimer <= diff)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    me->CastSpell(pTarget, SPELL_SHADOW_BOLT, true);
                shadowBoltTimer = (Is25ManRaid()) ? urand(5000, 7000) : urand(10000, 15000);
            }
            else shadowBoltTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_ds_twilight_siege_captain : public CreatureScript
{
public:
    npc_ds_twilight_siege_captain() : CreatureScript("npc_ds_twilight_siege_captain") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_twilight_siege_captainAI(pCreature);
    }

    struct npc_ds_twilight_siege_captainAI : public ScriptedAI
    {
        npc_ds_twilight_siege_captainAI(Creature *creature) : ScriptedAI(creature) { }

        TaskScheduler scheduler;
        uint32 twilightVolleyTimer;
        uint32 twilightSubmissionTimer;

        void Reset() override
        {
            scheduler.Schedule(Seconds(5), [this](TaskContext /* task context */)
            {
                if (Creature * pTwilightPortal = me->FindNearestCreature(NPC_TWILIGHT_PORTAL, 50.0f, true))
                {
                    pTwilightPortal->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                    me->CastSpell(pTwilightPortal, SPELL_TWILIGHT_PORTAL_BEAM, false);
                }
            });

            twilightVolleyTimer = urand(5000, 25000);
            twilightSubmissionTimer = urand(10000, 30000);
        }

        void EnterCombat(Unit * /*who*/) override
        {
            me->InterruptNonMeleeSpells(false, 0, true);
            if (Creature * pTwilightBreaker = me->FindNearestCreature(NPC_TWILIGHT_SIEGE_BREAKER, 100.0f, true))
                pTwilightBreaker->AI()->DoAction(ACTION_DISABLE);
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (!UpdateVictim())
                return;

            if (twilightVolleyTimer <= diff)
            {
                me->CastSpell(me, SPELL_TWILIGHT_VOLLEY, false);
                twilightVolleyTimer = (Is25ManRaid()) ? urand(10000, 25000) : urand(15000, 30000);
            }
            else twilightVolleyTimer -= diff;

            if (twilightSubmissionTimer <= diff)
            {
                me->CastSpell(me, SPELL_TWILIGHT_SUBMISSION, false);
                twilightSubmissionTimer = (Is25ManRaid()) ? urand(5000, 20000) : urand(10000, 20000);
            }
            else twilightSubmissionTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

const Position middlePos = { -1785.47f, -2393.37f, 45.58f, 0.0f };

class npc_ds_twilight_siege_breaker : public CreatureScript
{
public:
    npc_ds_twilight_siege_breaker() : CreatureScript("npc_ds_twilight_siege_breaker") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_twilight_siege_breakerAI(pCreature);
    }

    struct npc_ds_twilight_siege_breakerAI : public ScriptedAI
    {
        npc_ds_twilight_siege_breakerAI(Creature *creature) : ScriptedAI(creature) { }

        TaskScheduler scheduler;
        uint32 explosionTimer;
        bool explode,respawn;

        void Reset() override
        {
            me->SetWalk(true);
            me->SetSpeed(MOVE_WALK, 4.0f);
            me->GetMotionMaster()->MovePoint(0, middlePos, true, true);
            explosionTimer = 8500;
            respawn = false;
            explode = false;
        }

        void DoAction(const int32 param) override
        {
            if (param == ACTION_DISABLE)
            {
                explosionTimer = 10000;
                explode = true;
                respawn = false;
                me->Kill(me);
                me->SetRespawnTime(604800);
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (!explode)
            {
                if (explosionTimer <= diff)
                {
                    me->CastSpell(me, SPELL_FIERY_EXPLOSION, false);
                    explosionTimer = 300;
                    respawn = explode = true;
                }
                else explosionTimer -= diff;
            } else if (respawn)
            {
                if (explosionTimer <= diff)
                {
                    me->Kill(me);
                    me->Relocate(me->GetHomePosition());
                    me->Respawn(true);
                    respawn = false;
                }
                else explosionTimer -= diff;
            }

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

///////////////////////////
///    Zonozz stuff     ///
///////////////////////////

enum ZonozzTrashNPC
{
    NPC_EYE_OF_GORATH               = 57875,
    NPC_FLAIL_OF_GORATH             = 57877,
    NPC_CLAW_OF_GORATH              = 57890,
};

enum ZonozzTrashSpells
{
    SPELL_SHADOW_GAZE               = 109391,
    SPELL_SLUDGE_SPEW               = 110102,
    SPELL_WILD_FLAIL                = 109199,
    SPELL_OOZE_SPIT                 = 109396,
};

class npc_ds_eye_of_gorath : public CreatureScript
{
public:
    npc_ds_eye_of_gorath() : CreatureScript("npc_ds_eye_of_gorath") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_eye_of_gorathAI(pCreature);
    }

    struct npc_ds_eye_of_gorathAI : public ScriptedAI
    {
        npc_ds_eye_of_gorathAI(Creature *creature) : ScriptedAI(creature) { }

        uint32 shadowGazeTimer;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            shadowGazeTimer = urand(5000, 7000);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (shadowGazeTimer <= diff)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    me->CastSpell(pTarget, SPELL_SHADOW_GAZE, false);
                shadowGazeTimer = (Is25ManRaid()) ? 8000 : urand(10000, 13000);
            }
            else shadowGazeTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_ds_flail_of_gorath : public CreatureScript
{
public:
    npc_ds_flail_of_gorath() : CreatureScript("npc_ds_flail_of_gorath") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_flail_of_gorathAI(pCreature);
    }

    struct npc_ds_flail_of_gorathAI : public ScriptedAI
    {
        npc_ds_flail_of_gorathAI(Creature *creature) : ScriptedAI(creature) { }

        uint32 sludgeSpewTimer;
        uint32 wildFlailTimer;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            sludgeSpewTimer = urand(2000, 10000);
            wildFlailTimer = urand(5000, 30000);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (sludgeSpewTimer <= diff)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    me->CastSpell(pTarget, SPELL_SLUDGE_SPEW, false);
                sludgeSpewTimer = (Is25ManRaid()) ? urand(8000, 16000) : urand(15000, 20000);
            }
            else sludgeSpewTimer -= diff;

            if (wildFlailTimer <= diff)
            {
                me->CastSpell(me, SPELL_WILD_FLAIL, false);
                wildFlailTimer = urand(10000, 30000);
            }
            else wildFlailTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_ds_claw_of_gorath : public CreatureScript
{
public:
    npc_ds_claw_of_gorath() : CreatureScript("npc_ds_claw_of_gorath") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_claw_of_gorathAI(pCreature);
    }

    struct npc_ds_claw_of_gorathAI : public ScriptedAI
    {
        npc_ds_claw_of_gorathAI(Creature *creature) : ScriptedAI(creature) { }

        uint32 oozeSpitTimer;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            oozeSpitTimer = urand(10000, 25000);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (oozeSpitTimer <= diff)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    me->CastSpell(pTarget, SPELL_OOZE_SPIT, false);
                oozeSpitTimer = (Is25ManRaid()) ? urand(10000, 25000) : urand(15000, 30000);
            }
            else oozeSpitTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

///////////////////////////
///    Yorsahj stuff    ///
///////////////////////////

enum YorsahjTrashNPC
{
    NPC_ACID_GLOBULE                = 57333,
    NPC_SHADOWED_GLOBULE            = 57388,
    NPC_COBALT_GLOBULE              = 57384,
    NPC_CRIMSON_GLOBULE             = 57386,
    NPC_DARK_GLOBULE                = 57382,
    NPC_GLOWING_GLOBULE             = 57387,
};

enum YorsahjTrashSpells
{
    SPELL_DIGESTIVE_ACID            = 108419,
    SPELL_ACIDIC_BLOOD_OF_SHUMA     = 110743,
    SPELL_DEEP_CORRUPTION           = 108220,
    SPELL_SHADOWED_BLOOD_OF_SHUMA   = 110748,
    SPELL_MANA_VOID                 = 108223,
    SPELL_MANA_DIFFUSION            = 108228,
    SPELL_COBALT_BLOOD_OF_SHUMA     = 110747,
    SPELL_SEARING_BLOOD             = 108218,
    SPELL_CRIMSON_BLOOF_OF_SHUMA    = 110750,
    SPELL_PSYCHIC_SLICE             = 108922,
    SPELL_BLACK_BLOOD_OF_SHUMA      = 110746,
    SPELL_GLOWING_BLOOD             = 108221,
    SPELL_GLOWING_BLOOD_OF_SHUMA    = 110753,
};

class npc_ds_globule : public CreatureScript
{
public:
    npc_ds_globule() : CreatureScript("npc_ds_globule") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_globuleAI(pCreature);
    }

    struct npc_ds_globuleAI : public ScriptedAI
    {
        npc_ds_globuleAI(Creature *creature) : ScriptedAI(creature) { }

        uint32 randomSpellTimer;

        void Reset() override
        {
            randomSpellTimer = urand(5000, 15000);

            uint32 ownAura = 0;
            switch (me->GetEntry())
            {
            case NPC_ACID_GLOBULE:
                ownAura = SPELL_ACIDIC_BLOOD_OF_SHUMA;
                break;
            case NPC_COBALT_GLOBULE:
                ownAura = SPELL_COBALT_BLOOD_OF_SHUMA;
                break;
            case NPC_SHADOWED_GLOBULE:
                ownAura = SPELL_SHADOWED_BLOOD_OF_SHUMA;
                break;
            case NPC_GLOWING_GLOBULE:
                ownAura = SPELL_GLOWING_BLOOD_OF_SHUMA;
                break;
            case NPC_CRIMSON_GLOBULE:
                ownAura = SPELL_CRIMSON_BLOOF_OF_SHUMA;
                break;
            case NPC_DARK_GLOBULE:
                ownAura = SPELL_BLACK_BLOOD_OF_SHUMA;
                break;
            }
            me->CastSpell(me, ownAura, false);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (randomSpellTimer <= diff)
            {
                switch (me->GetEntry())
                {
                case NPC_ACID_GLOBULE:
                    me->CastSpell(me, SPELL_DIGESTIVE_ACID, false);
                    break;
                case NPC_COBALT_GLOBULE:
                    me->CastSpell(me,SPELL_MANA_VOID, false);
                    break;
                case NPC_SHADOWED_GLOBULE:
                    if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                        me->CastSpell(pTarget, SPELL_DEEP_CORRUPTION, false);
                    break;
                case NPC_GLOWING_GLOBULE:
                    me->CastSpell(me, SPELL_GLOWING_BLOOD, false);
                    break;
                case NPC_CRIMSON_GLOBULE:
                    me->CastSpell(me, SPELL_SEARING_BLOOD, false);
                    break;
                case NPC_DARK_GLOBULE:
                    if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                        me->CastSpell(pTarget, SPELL_PSYCHIC_SLICE, false);
                    break;
                }
                randomSpellTimer = me->HasAura(SPELL_GLOWING_BLOOD) ? 7000 : 14000;
            }
            else randomSpellTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

///////////////////////////
///    Hagara stuff     ///
///////////////////////////

enum HagaraTrashNPC
{
    NPC_TWILIGHT_FROST_EVOKER       = 57807,
    NPC_STORMBORN_MYRMIDON          = 57817,
    NPC_CORRUPTED_FRAGMENT          = 57819,
    NPC_STORMBINDER_ADEPT           = 57823,
    NPC_TORNADO_STALKER             = 57921,
    NPC_LIEUTENANT_SHARA            = 57821,
    NPC_FROZEN_GRASP_STALKER        = 57860,
    NPC_HAGARA_THE_STORMBINDER      = 55689,
};

enum HagaraTrashSpells
{
    SPELL_BLIZZARD                  = 109360,
    SPELL_FROSTBOLT                 = 109334,
    SPELL_SHACKLES_OF_ICE           = 109423,
    SPELL_CHAIN_LIGHTNING           = 109348,
    SPELL_SPARK                     = 109368,
    SPELL_TORNADO_SUMMON            = 109443,
    SPELL_TORNADO_VISUAL            = 109444,
    SPELL_TORNADO_DMG_AURA          = 109440,
    SPELL_FROST_CORRUPTION          = 109333,
    SPELL_FROZEN_GRASP_SUMMON       = 109305,
    SPELL_FROZEN_GRASP_AURA         = 109318,
    SPELL_FROZEN_GRASP_DMG_AURA     = 109295,
    SPELL_FROZEN_GRASP_GRIP         = 109307,
    SPELL_SHATTER                   = 109393,
};

class npc_ds_twilight_frost_evoker : public CreatureScript
{
public:
    npc_ds_twilight_frost_evoker() : CreatureScript("npc_ds_twilight_frost_evoker") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_twilight_frost_evokerAI(pCreature);
    }

    struct npc_ds_twilight_frost_evokerAI : public ScriptedAI
    {
        npc_ds_twilight_frost_evokerAI(Creature *creature) : ScriptedAI(creature) { }

        uint32 frostvoltTimer;
        uint32 blizzardTimer;
        uint32 shacklesOfIceTimer;

        void Reset() override
        {
            frostvoltTimer = 0;
            blizzardTimer = urand(5000, 15000);
            shacklesOfIceTimer = 0;
        }

        void JustDied(Unit * /*who*/) override
        {
            if (InstanceScript * instance = me->GetInstanceScript())
                instance->SetData(DATA_HAGARA_INTRO_TRASH, 0);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (frostvoltTimer <= diff)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    me->CastSpell(pTarget, SPELL_FROSTBOLT, false);
                frostvoltTimer = 3000;
            }
            else frostvoltTimer -= diff;

            if (blizzardTimer <= diff)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    me->CastSpell(pTarget, SPELL_BLIZZARD, false);
                blizzardTimer = urand(15000, 30000);
            }
            else blizzardTimer -= diff;

            if (shacklesOfIceTimer <= diff)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    me->CastSpell(pTarget, SPELL_SHACKLES_OF_ICE, false);
                shacklesOfIceTimer = urand(7000, 9000);
            }
            else shacklesOfIceTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_ds_stormborn_myrmidon : public CreatureScript
{
public:
    npc_ds_stormborn_myrmidon() : CreatureScript("npc_ds_stormborn_myrmidon") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_stormborn_myrmidonAI(pCreature);
    }

    struct npc_ds_stormborn_myrmidonAI : public ScriptedAI
    {
        npc_ds_stormborn_myrmidonAI(Creature *creature) : ScriptedAI(creature) { }

        uint32 chainLightningTimer;
        uint32 sparkTimer;

        void Reset() override
        {
            chainLightningTimer = urand(3000, 5000);
            sparkTimer = 4000;
        }

        void JustDied(Unit * /*who*/) override
        {
            if (InstanceScript * instance = me->GetInstanceScript())
                instance->SetData(DATA_HAGARA_INTRO_TRASH, 0);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (chainLightningTimer <= diff)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    me->CastSpell(pTarget, SPELL_CHAIN_LIGHTNING, false);
                chainLightningTimer = 2000;
            }
            else chainLightningTimer -= diff;

            if (sparkTimer <= diff)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    me->CastSpell(pTarget, SPELL_SPARK, true);
                sparkTimer = 4000;
            }
            else sparkTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_ds_stormbinder_adept : public CreatureScript
{
public:
    npc_ds_stormbinder_adept() : CreatureScript("npc_ds_stormbinder_adept") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_stormbinder_adeptAI(pCreature);
    }

    struct npc_ds_stormbinder_adeptAI : public ScriptedAI
    {
        npc_ds_stormbinder_adeptAI(Creature *creature) : ScriptedAI(creature) { }

        uint32 tornadoTimer;

        void Reset() override
        {
            tornadoTimer = 5000;
        }

        void JustDied(Unit * /*who*/) override
        {
            if (InstanceScript * instance = me->GetInstanceScript())
                instance->SetData(DATA_HAGARA_INTRO_TRASH, 0);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (tornadoTimer <= diff)
            {
                me->CastSpell(me, SPELL_TORNADO_SUMMON, false);
                tornadoTimer = urand(5000, 10000);
            }
            else tornadoTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_ds_corrupted_fragment : public CreatureScript
{
public:
    npc_ds_corrupted_fragment() : CreatureScript("npc_ds_corrupted_fragment") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_corrupted_fragmentAI(pCreature);
    }

    struct npc_ds_corrupted_fragmentAI : public ScriptedAI
    {
        npc_ds_corrupted_fragmentAI(Creature *creature) : ScriptedAI(creature) { }

        void JustDied(Unit * /*who*/) override
        {
            if (InstanceScript * instance = me->GetInstanceScript())
                instance->SetData(DATA_HAGARA_INTRO_TRASH, 0);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_ds_hagara_tornado : public CreatureScript
{
public:
    npc_ds_hagara_tornado() : CreatureScript("npc_ds_hagara_tornado") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_hagara_tornadoAI(pCreature);
    }

    struct npc_ds_hagara_tornadoAI : public ScriptedAI
    {
        npc_ds_hagara_tornadoAI(Creature *creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, SPELL_TORNADO_DMG_AURA, false);
            me->CastSpell(me, SPELL_TORNADO_VISUAL, false);
            me->SetSpeed(MOVE_WALK, 2.0f);
            me->SetWalk(true);
            me->GetMotionMaster()->MoveRandom(25.0f);
        }
    };
};

class npc_ds_lieutenant_shara : public CreatureScript
{
public:
    npc_ds_lieutenant_shara() : CreatureScript("npc_ds_lieutenant_shara") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_lieutenant_sharaAI(pCreature);
    }

    struct npc_ds_lieutenant_sharaAI : public ScriptedAI
    {
        npc_ds_lieutenant_sharaAI(Creature *creature) : ScriptedAI(creature) { }

        uint32 frostCorruptionTimer;
        uint32 frozenGraspTimer;
        uint32 shatterTimer;

        void Reset() override
        {
            frostCorruptionTimer = 5000;
            frozenGraspTimer = 9000;
            shatterTimer= 7000;
        }

        void JustDied(Unit * /*who*/) override
        {
            if (InstanceScript * instance = me->GetInstanceScript())
                instance->SetData(DATA_HAGARA_INTRO_TRASH, 0);

            std::list<Creature*> frozen_grasp_stalker;
            me->GetCreatureListWithEntryInGrid(frozen_grasp_stalker, NPC_FROZEN_GRASP_STALKER, 150.0f);
            for (std::list<Creature*>::iterator itr = frozen_grasp_stalker.begin(); itr != frozen_grasp_stalker.end(); ++itr)
            {
                if (*itr)
                    (*itr)->Kill((*itr));
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (frostCorruptionTimer <= diff)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    me->CastSpell(pTarget, SPELL_FROST_CORRUPTION, true);
                frostCorruptionTimer = 5000;
            }
            else frostCorruptionTimer -= diff;

            if (frozenGraspTimer <= diff)
            {
                me->CastSpell(me, SPELL_FROZEN_GRASP_SUMMON, false);
                frozenGraspTimer = 15000;
            }
            else frozenGraspTimer -= diff;

            if (shatterTimer <= diff)
            {
                me->CastSpell(me->GetVictim(), SPELL_SHATTER, true);
                shatterTimer = 7000;
            }
            else shatterTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_ds_frozen_grasp_stalker : public CreatureScript
{
public:
    npc_ds_frozen_grasp_stalker() : CreatureScript("npc_ds_frozen_grasp_stalker") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_frozen_grasp_stalkerAI(pCreature);
    }

    struct npc_ds_frozen_grasp_stalkerAI : public ScriptedAI
    {
        npc_ds_frozen_grasp_stalkerAI(Creature *creature) : ScriptedAI(creature) { }

       uint32 frozenGraspTimer;

        void Reset() override
        {
            me->CastSpell(me, SPELL_FROZEN_GRASP_DMG_AURA, false);
            me->CastSpell(me, SPELL_FROZEN_GRASP_AURA, false);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetInCombatWithZone();
            frozenGraspTimer = 2000;
        }

        void UpdateAI(const uint32 diff) override
        {
            if (frozenGraspTimer <= diff)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                {
                    me->CastSpell(pTarget, SPELL_FROZEN_GRASP_GRIP, false);
                    pTarget->GetMotionMaster()->MoveJump(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 15.0f, 5.0f);
                }
                frozenGraspTimer = 5000;
            }
            else frozenGraspTimer -= diff;
        }
    };
};

class npc_ds_hagara_trash_portal : public CreatureScript
{
public:
    npc_ds_hagara_trash_portal() : CreatureScript("npc_ds_hagara_trash_portal") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_hagara_trash_portalAI(pCreature);
    }

    struct npc_ds_hagara_trash_portalAI : public ScriptedAI
    {
        npc_ds_hagara_trash_portalAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript * instance;
        TaskScheduler scheduler;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);

            scheduler.Schedule(Seconds(2), [this](TaskContext flood)
            {
                if (instance && (instance->GetData(DATA_HAGARA_INTRO_TRASH) == 0))
                {
                    float angle = me->GetOrientation() - (2 * (M_PI / 8));
                    float angleAddition = (M_PI / 8);
                    float distance = 10.0f;

                    for (uint32 i = 0; i < 4; i++)
                    {
                        uint32 npcId = 0;
                        if (i == 0 || i == 3)
                            npcId = NPC_TWILIGHT_FROST_EVOKER;
                        else
                            npcId = NPC_STORMBORN_MYRMIDON;
                        me->SummonCreature(npcId, me->GetPositionX() + cos(angle)*distance, me->GetPositionY() + sin(angle)*distance, me->GetPositionZ(), angle, TEMPSUMMON_DEAD_DESPAWN);
                        angle += angleAddition;
                    }
                }
                else if (instance->GetData(DATA_HAGARA_INTRO_TRASH) == 4)
                {
                    float angle = me->GetOrientation() - (M_PI / 4);
                    float angleAddition = (M_PI / 4);
                    float distance = 10.0f;

                    for (uint32 i = 0; i < 7; i++)
                    {
                        if (i == 0)
                            me->SummonCreature(NPC_STORMBINDER_ADEPT, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), angle, TEMPSUMMON_DEAD_DESPAWN);
                        else
                        {
                            if (i == 4)
                            {
                                angle = me->GetOrientation() - (M_PI / 4);
                                distance = 14.0f;
                            }
                            me->SummonCreature(NPC_CORRUPTED_FRAGMENT, me->GetPositionX() + cos(angle)*distance, me->GetPositionY() + sin(angle)*distance, me->GetPositionZ(), angle, TEMPSUMMON_DEAD_DESPAWN);
                            angle += angleAddition;
                        }
                    }
                }
                else if (instance->GetData(DATA_HAGARA_INTRO_TRASH) == 18)
                {
                    float angle = me->GetOrientation() - (M_PI / 4);
                    float angleAddition = (M_PI / 4);
                    float distance = 10.0f;

                    for (uint32 i = 0; i < 3; i++)
                    {
                        uint32 npcId = 0;
                        if (i == 0)
                            npcId = NPC_TWILIGHT_FROST_EVOKER;
                        else if (i == 1)
                            npcId = NPC_STORMBORN_MYRMIDON;
                        else if (i == 2)
                            npcId = NPC_STORMBINDER_ADEPT;

                        me->SummonCreature(npcId, me->GetPositionX() + cos(angle)*distance, me->GetPositionY() + sin(angle)*distance, me->GetPositionZ(), angle, TEMPSUMMON_DEAD_DESPAWN);
                        angle += angleAddition;
                    }
                }
                else if (instance->GetData(DATA_HAGARA_INTRO_TRASH) == 24)
                {
                    float angle = me->GetOrientation() - (M_PI / 4);
                    float angleAddition = (M_PI / 8);
                    float distance = 10.0f;

                    for (uint32 i = 0; i < 11; i++)
                    {
                        if (i == 0)
                            me->SummonCreature(NPC_LIEUTENANT_SHARA, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN);
                        else
                        {
                            if (i == 6)
                            {
                                angle = me->GetOrientation() - (M_PI / 4);
                                distance = 14.0f;
                            }
                            me->SummonCreature(NPC_CORRUPTED_FRAGMENT, me->GetPositionX() + cos(angle)*distance, me->GetPositionY() + sin(angle)*distance, me->GetPositionZ(), angle, TEMPSUMMON_DEAD_DESPAWN);
                            angle += angleAddition;
                        }
                    }
                }
            });
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);
        }
    };
};

class go_ds_focusing_iris : public GameObjectScript
{
public:
    go_ds_focusing_iris() : GameObjectScript("go_ds_focusing_iris") { }

    bool OnGossipHello(Player* pPlayer, GameObject* pGo)
    {
        if (Creature * pHagara = pGo->FindNearestCreature(NPC_HAGARA_THE_STORMBINDER, 200.0f, true))
        {
            pHagara->SetFacingTo(6.27f);
            pHagara->AI()->DoAction(DATA_HAGARA_INTRO_TRASH);
            //pGo->Use(pPlayer);
            pGo->Delete();
        }
        return true;
    }
};

///////////////////////////
///   Ultraxion stuff   ///
///////////////////////////

enum ultraxionStuffSpells
{
    GIFT_OF_LIFE_MISSILE_10N        = 106042,
    GIFT_OF_LIFE_MISSILE_25N        = 109349,
    GIFT_OF_LIFE_MISSILE_10HC       = 109350,
    GIFT_OF_LIFE_MISSILE_25HC       = 109351,

    ESSENCE_OF_DREAMS_MISSILE_10N   = 106049,
    ESSENCE_OF_DREAMS_MISSILE_25N   = 109356,
    ESSENCE_OF_DREAMS_MISSILE_10HC  = 109357,
    ESSENCE_OF_DREAMS_MISSILE_25HC  = 109358,

    SOURCE_OF_MAGIC_MISSILE_10N     = 106050,
    SOURCE_OF_MAGIC_MISSILE_25N     = 109353,
    SOURCE_OF_MAGIC_MISSILE_10HC    = 109354,
    SOURCE_OF_MAGIC_MISSILE_25HC    = 109355,

    SPELL_SPAWN_ASSAULTER_VISUAL    = 109684,

    SPELL_TWILIGHT_BREATH           = 105858,
    SPELL_TWILIGHT_FLAMES           = 105700,
};

enum ultraxionGameobjects
{
    GO_GIFT_OF_LIFE                 = 209873,
    GO_ESSENCE_OF_DREAMS            = 209874,
    GO_SOURCE_OF_MAGIC              = 209875,
};

// NPC ultraxion go spawn - 119550, 119551, 119552
class npc_ultraxion_go_spawn : public CreatureScript
{
public:
    npc_ultraxion_go_spawn() : CreatureScript("npc_ultraxion_go_spawn") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ultraxion_go_spawnAI(pCreature);
    }

    struct npc_ultraxion_go_spawnAI : public ScriptedAI
    {
        npc_ultraxion_go_spawnAI(Creature *creature) : ScriptedAI(creature) {}

        void SpellHit(Unit* /*caster*/, SpellEntry const* spell) override
        {
            uint32 gameobjectId = 0;
            uint32 gameobjectCount;
            gameobjectCount = (getDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL || getDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC) ? 2 : 1;

            switch (spell->Id)
            {
            case GIFT_OF_LIFE_MISSILE_10N:
            case GIFT_OF_LIFE_MISSILE_25N:
            case GIFT_OF_LIFE_MISSILE_10HC:
            case GIFT_OF_LIFE_MISSILE_25HC:
                gameobjectId = GO_GIFT_OF_LIFE;
                break;
            case ESSENCE_OF_DREAMS_MISSILE_10N:
            case ESSENCE_OF_DREAMS_MISSILE_25N:
            case ESSENCE_OF_DREAMS_MISSILE_10HC:
            case ESSENCE_OF_DREAMS_MISSILE_25HC:
                gameobjectId = GO_ESSENCE_OF_DREAMS;
                break;
            case SOURCE_OF_MAGIC_MISSILE_10N:
            case SOURCE_OF_MAGIC_MISSILE_25N:
            case SOURCE_OF_MAGIC_MISSILE_10HC:
            case SOURCE_OF_MAGIC_MISSILE_25HC:
                gameobjectId = GO_SOURCE_OF_MAGIC;
                break;
            default:
                break;
            }

            for (uint32 i = 0; i < gameobjectCount; i++)
            {
                uint32 distance;
                distance = (i == 1) ? 3 : 0;
                me->SummonGameObject(gameobjectId, me->GetPositionX() + distance, me->GetPositionY() + distance, me->GetPositionZ() + 1, 0.0f, 0, 0, 0, 0, 300000);
            }
        }
    };
};

class npc_ds_twilight_assaulter : public CreatureScript
{
public:
    npc_ds_twilight_assaulter() : CreatureScript("npc_ds_twilight_assaulter") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_twilight_assaulterAI(pCreature);
    }

    struct npc_ds_twilight_assaulterAI : public ScriptedAI
    {
        npc_ds_twilight_assaulterAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();

            me->GetMotionMaster()->MoveRandom(20.0f);
            me->SetVisible(false);
        }

        InstanceScript * instance;
        TaskScheduler scheduler;
        uint32 specialAbilityTimer;
        bool canCast;

        void IsSummonedBy(Unit* pSummoner) override
        {
            if (pSummoner)
            {
                me->GetMotionMaster()->MoveIdle(MOTION_SLOT_IDLE);
                canCast = true;
                me->AI()->DoAction(DATA_ULTRAXION_DRAKES);
                me->SetInCombatWithZone();
                me->SetReactState(REACT_AGGRESSIVE);
            }
        }

        void Reset() override
        {
            me->SetFlying(true);
            canCast = false;
            specialAbilityTimer = urand(6000, 8000);
        }

        void DoAction(const int32 action) override
        {
            if (action == DATA_ULTRAXION_DRAKES)
            {
                me->SetVisible(true);
                me->CastSpell(me, SPELL_SPAWN_ASSAULTER_VISUAL, false);
            }
        }

        void JustDied(Unit * /*who*/) override
        {
            if (instance)
                instance->SetData(DATA_ULTRAXION_DRAKES, 1);
        }

        void EnterCombat(Unit * /*who*/) override
        {
            scheduler.Schedule(Seconds(2), [this](TaskContext fly)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    me->CastSpell(pTarget, SPELL_TWILIGHT_FLAMES, false);
            });
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (!UpdateVictim())
                return;

            if (canCast)
            {
                if (specialAbilityTimer <= diff)
                {
                    uint32 randSpell = urand(0, 1) ? SPELL_TWILIGHT_BREATH : SPELL_TWILIGHT_FLAMES;
                    if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                        me->CastSpell(pTarget, randSpell, false);
                    specialAbilityTimer = urand(6000, 8000);
                }
                else specialAbilityTimer -= diff;
            }

            DoMeleeAttackIfReady();
        }
    };
};

///////////////////////////
///   Blackhorn stuff   ///
///////////////////////////

enum BlackhornTrashNPC
{
    NPC_WARMASTER_BLACKHORN             = 56427,
    NPC_GORIONA                         = 56781,
    NPC_SKYFIRE                         = 56598,
    NPC_TWILIGHT_ASSAULT_DRAKE_LEFT     = 56855,
    NPC_TWILIGHT_ASSAULT_DRAKE_RIGHT    = 56587,
    NPC_TWILIGHT_ELITE_DREADBLADE       = 56854,
    NPC_TWILIGHT_ELITE_SLAYER           = 56848,
    NPC_TWILIGHT_SAPPER                 = 56923,
    NPC_TWILIGHT_FLAMES                 = 57268,
    NPC_FIRE_STALKER                    = 57852,
    NPC_BROADSIDE_TARGET                = 119559,
};

enum BlackhornTrashSpells
{
    // Twilight Elite Dreadblade & Slayer
    SPELL_BLADE_RUSH                = 107594,
    SPELL_BLADE_RUSH_DMG            = 107595,
    SPELL_BRUTAL_STRIKE             = 107567,
    SPELL_DEGENERATION              = 107558,

    // Twilight Sapper
    SPELL_EVADE                     = 107761,
    SPELL_DETONATE                  = 107518,
    SPELL_SMOKE_BOMB                = 107752,

    SPELL_ENGINE_FIRE               = 107799,
    SPELL_MASSIVE_EXPLOSION         = 108132,

    SPELL_TWILIGHT_FLAMES_AURA      = 108053,
    SPELL_SHIP_FIRE_VISUAL          = 109245,
    SPELL_DECK_FIRE_DMG             = 110095,
};

enum actionsSkyfire
{
    ACTION_START_WARMASTER_ENCOUNTER    = 0,
    ACTION_SHOOT_DRAKE                  = 1,
};

struct PlayableQuote skyfireSapper { 26296, "An Enemy Sapper has breached the Engine Room!" };
struct PlayableQuote skyfireLowHP { 26297, "The Skyfire can't take much more of this!" };
struct PlayableQuote skyfireGoingDown { 26298, "Were going down, abandon the ship!" };

class npc_ds_engine_stalker : public CreatureScript
{
public:
    npc_ds_engine_stalker() : CreatureScript("npc_ds_engine_stalker") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_engine_stalkerAI(pCreature);
    }

    struct npc_ds_engine_stalkerAI : public ScriptedAI
    {
        npc_ds_engine_stalkerAI(Creature *creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
        }
    };
};

class npc_ds_twilight_elite : public CreatureScript
{
public:
    npc_ds_twilight_elite() : CreatureScript("npc_ds_twilight_elite") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_twilight_eliteAI(pCreature);
    }

    struct npc_ds_twilight_eliteAI : public ScriptedAI
    {
        npc_ds_twilight_eliteAI(Creature *creature) : ScriptedAI(creature) { }

        TaskScheduler scheduler;
        uint32 bladeRushTimer;
        uint32 specialAbilityTimer;

        void Reset() override
        {
            bladeRushTimer = 10000;
            specialAbilityTimer = urand(5000, 15000);
        }

        inline bool IsCastingAllowed()
        {
            return !me->IsNonMeleeSpellCasted(false);
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (!UpdateVictim())
                return;

            if (bladeRushTimer <= diff)
            {
                me->SetInCombatWithZone();

                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 1, 100.0f, true))
                {
                    me->CastSpell(pTarget, SPELL_BLADE_RUSH, false);

                    // Deal damage manually to all players between me and Blade Rush target
                    scheduler.Schedule(Milliseconds(1900), [this, pTarget](TaskContext /*task context*/)
                    {
                        Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                        for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                        {
                            if (Player* pl = i->getSource())
                            {
                                if (pl->GetGUID() != pTarget->GetGUID())
                                {
                                    // Find out if any player is in between Ice Lance and player with Target buff
                                    if ((pl)->IsInBetween(me, pTarget, 2.0f) == true && pl->IsAlive())
                                    {
                                        me->CastSpell(pl, SPELL_BLADE_RUSH_DMG, true);
                                    }
                                }
                            }
                        }
                    });
                }
                bladeRushTimer = urand(20000, 25000);
            }
            else bladeRushTimer -= diff;

            if (specialAbilityTimer <= diff)
            {
                if (IsCastingAllowed())
                {
                    if (me->GetEntry() == NPC_TWILIGHT_ELITE_DREADBLADE)
                        me->CastSpell(me->GetVictim(), SPELL_DEGENERATION, false);
                    else if (me->GetEntry() == NPC_TWILIGHT_ELITE_SLAYER)
                        me->CastSpell(me->GetVictim(), SPELL_BRUTAL_STRIKE, false);
                    specialAbilityTimer = urand(7000, 15000);
                }
            }
            else specialAbilityTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_ds_twilight_infiltrator : public CreatureScript
{
public:
    npc_ds_twilight_infiltrator() : CreatureScript("npc_ds_twilight_infiltrator") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_twilight_infiltratorAI(pCreature);
    }

    struct npc_ds_twilight_infiltratorAI : public ScriptedAI
    {
        npc_ds_twilight_infiltratorAI(Creature *creature) : ScriptedAI(creature) { }

        TaskScheduler scheduler;

        void Reset() override
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetFlying(true);
            me->SetSpeed(MOVE_FLIGHT, 2.3f);

            scheduler.Schedule(Seconds(4), [this](TaskContext /*task context*/)
            {
                me->SummonCreature(NPC_TWILIGHT_SAPPER, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 2000);
            });
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);
        }
    };
};

const Position sapperEndPos = { 13469.45f, -12134.10f, 150.9f, 0.0f };

class npc_ds_twilight_sapper : public CreatureScript
{
public:
    npc_ds_twilight_sapper() : CreatureScript("npc_ds_twilight_sapper") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_twilight_sapperAI(pCreature);
    }

    struct npc_ds_twilight_sapperAI : public ScriptedAI
    {
        npc_ds_twilight_sapperAI(Creature *creature) : ScriptedAI(creature) 
        {
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
        }

        TaskScheduler scheduler;
        uint32 moveTimer;
        bool explosion;

        void Reset() override
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->GetMotionMaster()->MoveFall();
            moveTimer = 3000;
            explosion = false;

            scheduler.Schedule(Seconds(2), [this](TaskContext /*task context*/)
            {
                me->CastSpell(me, SPELL_SMOKE_BOMB, false);
                me->CastSpell(me, SPELL_EVADE, false);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->GetMotionMaster()->MovePoint(0, sapperEndPos, true, true);
            });
        }

        void RunPlayableQuote(PlayableQuote quote, uint32 npcEntry)
        {
            if (Creature * pNpcEntry = me->FindNearestCreature(npcEntry, 300.0f))
            {
                pNpcEntry->MonsterYell(quote.GetText(), LANG_UNIVERSAL, 0);
                pNpcEntry->SendPlaySound(quote.GetSoundId(), false);
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (moveTimer <= diff)
            {
                if (!me->HasAuraType(SPELL_AURA_MOD_STUN))
                    me->GetMotionMaster()->MovePoint(0, sapperEndPos, true, true);
                moveTimer = 1000;
            }
            else moveTimer -= diff;

            if (!explosion)
            {
                if (me->GetDistance2d(sapperEndPos.GetPositionX(), sapperEndPos.GetPositionY()) <= 2.0f)
                {
                    RunPlayableQuote(skyfireSapper, NPC_SKY_CAPTAIN_SWAYZE);
                    explosion = true;

                    // Deal dmg to the ship (20% Ship health) 
                    if (Creature* pShip = me->FindNearestCreature(NPC_SKYFIRE, 300.0f))
                        pShip->DealDamage(pShip, pShip->GetMaxHealth()*0.2);

                    me->CastSpell(me, SPELL_DETONATE, false);
                    me->DespawnOrUnsummon(1000);
                }
            }
        }
    };
};

class npc_ds_warmaster_triggers : public CreatureScript
{
public:
    npc_ds_warmaster_triggers() : CreatureScript("npc_ds_warmaster_triggers") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_warmaster_triggersAI(pCreature);
    }

    struct npc_ds_warmaster_triggersAI : public ScriptedAI
    {
        npc_ds_warmaster_triggersAI(Creature *creature) : ScriptedAI(creature) { }

        TaskScheduler scheduler;

        void Reset() override
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            if (me->GetEntry() != NPC_BROADSIDE_TARGET)
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            if (me->GetEntry() == NPC_TWILIGHT_FLAMES)
                me->CastSpell(me, SPELL_TWILIGHT_FLAMES_AURA, false);
            if (me->GetEntry() == NPC_FIRE_STALKER)
            {
                me->CastSpell(me, SPELL_SHIP_FIRE_VISUAL, false);

                scheduler.Schedule(Seconds(1), [this](TaskContext fireDamage)
                {
                    me->CastSpell(me, SPELL_DECK_FIRE_DMG, true);
                    fireDamage.Repeat(Seconds(1));
                });
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);
        }
    };
};

class npc_ds_skyfire_deck : public CreatureScript
{
public:
    npc_ds_skyfire_deck() : CreatureScript("npc_ds_skyfire_deck") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_skyfire_deckAI(pCreature);
    }

    struct npc_ds_skyfire_deckAI : public ScriptedAI
    {
        npc_ds_skyfire_deckAI(Creature *creature) : ScriptedAI(creature) 
        {
            me->SetMaxHealth(RAID_MODE(4000000, 10000000, 6000000, 15000000));
            me->SetHealth(RAID_MODE(4000000, 10000000, 6000000, 15000000));
        }

        TaskScheduler scheduler;
        bool lowHealth;
        bool canExplode;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            lowHealth = false;
            canExplode = false;
        }

        void RunPlayableQuote(PlayableQuote quote, uint32 npcEntry)
        {
            if (Creature * pNpcEntry = me->FindNearestCreature(npcEntry, 300.0f))
            {
                pNpcEntry->MonsterYell(quote.GetText(), LANG_UNIVERSAL, 0);
                pNpcEntry->SendPlaySound(quote.GetSoundId(), false);
            }
        }

        void DamageTaken(Unit* who, uint32 &damage) override
        {
            if (!lowHealth)
            {
                if (me->GetHealth() < (me->GetMaxHealth() * 0.4))
                {
                    RunPlayableQuote(skyfireLowHP, NPC_SKY_CAPTAIN_SWAYZE);
                    lowHealth = true;
                }
            }

            if (me->GetHealth() > damage)
                return;

            if (me->GetHealth() <= damage)
            {
                damage = me->GetHealth() - 1;

                if (canExplode == false)
                {
                    canExplode = true;
                    me->setFaction(14);
                    me->CastSpell(me, SPELL_MASSIVE_EXPLOSION, false);

                    scheduler.Schedule(Seconds(2), [this](TaskContext /*task context*/)
                    {
                        if (Creature* pWarmaster = me->FindNearestCreature(NPC_WARMASTER_BLACKHORN, 300.0f))
                            pWarmaster->AI()->Reset();

                        Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                        for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                        {
                            if (Player* pl = i->getSource())
                                pl->Kill(pl);
                        }
                    });

                    RunPlayableQuote(skyfireGoingDown, NPC_SKY_CAPTAIN_SWAYZE);
                }
            }
        }

        void UpdateAI(const uint32 diff) override 
        {
            scheduler.Update(diff);
        }
    };
};

///////////////////////////
///    Madness stuff    ///
///////////////////////////

enum MadnessNpc
{
    NPC_MADNESS_OF_DEATHWING            = 56173,
    NPC_ALEXSTRASZA_DRAGON              = 56099,
    NPC_NOZDORMU_DRAGON                 = 56102,
    NPC_YSERA_DRAGON                    = 56100,
    NPC_KALECGOS_DRAGON                 = 56101,
    NPC_JUMP_PAD                        = 56699,
    NPC_PLATFORM                        = 56307,
    NPC_TIMEZONE                        = 56311,
};

enum MadnessSpells
{
    // Aspects
    SPELL_ALEXSTRASZA_PRESENCE          = 105825,
    SPELL_ALEXSTRASZA_PRESENCE_AURA     = 106028,
    SPELL_ALEXSTRASZA_PRESENCE_AURA_25  = 109571,
    SPELL_ALEXSTRASZA_PRESENCE_AURA_10H = 109572,
    SPELL_ALEXSTRASZA_PRESENCE_AURA_25H = 109573,

    SPELL_CAUTERIZE_BLIST_TENTACLE_10N  = 105565, // Cast
    SPELL_CAUTERIZE_BLIST_TENTACLE_25N  = 109579,
    SPELL_CAUTERIZE_BLIST_TENTACLE_10HC = 109580,
    SPELL_CAUTERIZE_BLIST_TENTACLE_25HC = 109581,
    SPELL_CAUTERIZE_DMG_10N             = 105569, // Dmg
    SPELL_CAUTERIZE_DMG_25N             = 109576,
    SPELL_CAUTERIZE_DMG_10HC            = 109577,
    SPELL_CAUTERIZE_DMG_25HC            = 109578,

    SPELL_CAUTERIZE_2_1                 = 106860, // deathwing phase 2 5 sec
    SPELL_CAUTERIZE_2_2                 = 106888, // deathwing phase 2 10 sec
    SPELL_CAUTERIZE_2_3                 = 106889, // deathwing phase 2 15 sec
    SPELL_CAUTERIZE_2_DMG               = 106886,
    SPELL_CAUTERIZE_3                   = 108840, // parasite
    SPELL_CAUTERIZE_3_DMG               = 109045,

    SPELL_NOZDORMU_PRESENCE             = 105823,
    SPELL_NOZDORMU_PRESENCE_AURA        = 106027,
    SPELL_NOZDORMU_PRESENCE_AURA_25     = 109622,
    SPELL_NOZDORMU_PRESENCE_AURA_10H    = 109623,
    SPELL_NOZDORMU_PRESENCE_AURA_25H    = 109624,
    SPELL_TIME_ZONE_FORCE               = 106919, // first spell, there are npcs 'platform'
    SPELL_TIME_ZONE_SUMMON_TARGET       = 105793, // summon target
    SPELL_TIME_ZONE_MISSILE_1           = 105799,
    SPELL_TIME_ZONE_SUMMON_1            = 105802, // summon time zone
    SPELL_TIME_ZONE                     = 105831, // main aura on trigger
    SPELL_TIME_ZONE_AURA_1              = 105830, // aura (debuff)
    SPELL_TIME_ZONE_MISSILE_2           = 107055, // ? maybe heroic
    SPELL_TIME_ZONE_SUMMON_2            = 107057, // ? maybe heroic
    SPELL_TIME_ZONE_AURA_2              = 108646, // for parasite

    SPELL_YSERA_PRESENCE                = 106456,
    SPELL_YSERA_PRESENCE_AURA           = 106457,
    SPELL_YSERA_PRESENCE_AURA_25        = 109640,
    SPELL_YSERA_PRESENCE_AURA_10H       = 109641,
    SPELL_YSERA_PRESENCE_AURA_25H       = 109642,
    SPELL_THE_DREAMER                   = 106463,
    SPELL_ENTER_THE_DREAM               = 106464,

    SPELL_KALECGOS_PRESENCE             = 106026,
    SPELL_KALECGOS_PRESENCE_AURA        = 106029,
    SPELL_KALECGOS_PRESENCE_AURA_25     = 109606,
    SPELL_KALECGOS_PRESENCE_AURA_10H    = 109607,
    SPELL_KALECGOS_PRESENCE_AURA_25H    = 109608,
    SPELL_SPELLWEAVER                   = 106039,
    SPELL_SPELLWEAVING                  = 106040,

    SPELL_CONCENTRATION_ALEXSTRASZA     = 106641,
    SPELL_CONCENTRATION_NOZDORMU        = 106642,
    SPELL_CONCENTRATION_YSERA           = 106643,
    SPELL_CONCENTRATION_KALECGOS        = 106644,

    // Thrall
    SPELL_ASTRAL_RECALL                 = 108537,
    SPELL_ASTRAL_RECALL_2               = 101063,
    SPELL_TRIGGER_ASPECT_YELL_1         = 109708,
    SPELL_TRIGGER_ASPECT_YELL_2         = 109727,
    SPELL_FIRE_DRAGON_SOUL_ASPECTS      = 110067,
    SPELL_FIRE_DRAGON_SOUL              = 109971,
    SPELL_FIRE_DRAGON_SOUL_SCRIPT       = 110065,
    SPELL_HOVER                         = 57764,

    // Jump Pad
    SPELL_CARRYING_WINDS_DUMMY          = 106678,
    SPELL_CARRYING_WINDS_SPEED_10N      = 106664,
    SPELL_CARRYING_WINDS_SPEED_25N      = 109963,
    SPELL_CARRYING_WINDS_SPEED_10HC     = 109962,
    SPELL_CARRYING_WINDS_SPEED_25HC     = 109961,
};

const Position jumpPadSpawnPos[36] =
{
    // 1 to 2
    { -11990.8f, 12284.9f,  0.00f, 6.25f },
    { -11989.7f, 12271.9f,  0.00f, 5.27f },
    { -11987.3f, 12261.2f,  0.00f, 0.29f },
    { -11981.8f, 12251.7f,  0.00f, 0.32f },
    { -11975.7f, 12243.3f,  0.00f, 0.27f },
    { -11969.4f, 12233.7f,  0.00f, 5.13f },
    // 2 to 1
    { -12029.6f, 12261.1f, -4.90f, 1.00f },
    { -12021.3f, 12253.6f, -4.90f, 0.75f },
    { -12013.1f, 12247.1f, -5.55f, 5.42f },
    { -12009.6f, 12237.1f, -6.55f, 5.38f },
    { -12002.5f, 12228.5f, -6.55f, 6.13f },
    { -11996.7f, 12218.6f, -6.55f, 3.45f },
    // 2 to 3
    { -12076.4f, 12225.7f, -7.45f, 5.79f },
    { -12070.7f, 12216.5f, -7.45f, 0.62f },
    { -12067.6f, 12207.4f, -7.90f, 1.90f },
    { -12060.6f, 12199.2f, -7.45f, 0.01f },
    { -12054.4f, 12190.5f, -7.45f, 0.62f },
    { -12047.9f, 12181.2f, -7.45f, 5.42f },
    // 3 to 2
    { -12103.3f, 12194.7f, -4.36f, 6.11f },
    { -12091.9f, 12191.5f, -4.36f, 5.31f },
    { -12083.3f, 12183.7f, -4.36f, 6.00f },
    { -12072.4f, 12179.2f, -4.36f, 5.65f },
    { -12063.2f, 12172.2f, -4.36f, 4.23f },
    { -12054.1f, 12165.9f, -4.28f, 5.69f },
    // 3 to 4
    { -12131.4f, 12147.3f, -2.74f, 0.52f },
    { -12122.5f, 12140.1f, -2.74f, 1.07f },
    { -12113.2f, 12135.4f, -2.74f, 0.98f },
    { -12103.4f, 12132.3f, -2.74f, 1.08f },
    { -12093.5f, 12127.6f, -2.74f, 1.15f },
    { -12082.8f, 12124.7f, -3.04f, 5.84f },
    // 4 to 3
    { -12086.9f, 12093.6f,  0.75f, 2.77f },
    { -12095.4f, 12097.0f,  0.26f, 5.84f },
    { -12105.1f, 12101.5f,  0.48f, 4.51f },
    { -12115.7f, 12102.9f,  1.27f, 4.39f },
    { -12126.2f, 12105.1f,  1.20f, 6.12f },
    { -12135.4f, 12111.9f,  0.47f, 5.93f },
};

const Position jumpPadLandPos[6] =
{
    { -12022.9f, 12233.8f, -6.38f, 0.0f }, // from 1 to 2
    { -11970.9f, 12260.5f,  1.26f, 0.0f }, // from 2 to 1
    { -12086.9f, 12172.9f, -3.02f, 0.0f }, // from 2 to 3
    { -12053.5f, 12210.7f, -6.00f, 0.0f }, // from 3 to 2
    { -12114.6f, 12090.8f,  2.30f, 0.0f }, // from 3 to 4
    { -12098.0f, 12149.4f, -2.73f, 0.0f }, // from 4 to 3
};

const Position thrallPos = { -12061.8f, 12188.0f, 10.2f, 5.57f };
const Position thrallPosIsland = { -12133.3f, 12252.3f, 7.8f, 5.90f };

const Position alexstraszaPos = { -11957.3f, 12338.3f, 38.9f, 5.061f };
const Position nozdormuPos = { -12093.8f, 12312.0f, 38.9f, 5.427f };
const Position yseraPos = { -12157.4f, 12212.5f, 38.9f, 5.759f };
const Position kalecgosPos = { -12224.8f, 12128.5f, 38.9f, 5.846f };

// List of gossip texts
#define GOSSIP_DESTROYER_DEFEAT     "We defeated the Destroyer!"
#define GOSSIP_WHATS_THAT_NOICE     "Inconceivable! He coulnd't possibly be alive!"

enum ThrallActions
{
    ACTION_THRALL_INTRO_SPEECH         = 0,
    ACTION_SPAWN_JUMP_PADS             = 1,
    ACTION_START_MADNESS_ENCOUNTER     = 2,
    ACTION_NOZDORMU_PLATFORM_DESTROYED = 3,
    ACTION_TELEPORT_HOME               = 5,
    ACTION_ASPECTS_SECOND_PHASE        = 10,
    ACTION_ASPECTS_OUTRO               = 11,
    ACTION_ASPECTS_HIDE                = 12,
};

const int8 MAX_SECOND_PHASE_QUOTES = 8;

static const PlayableQuote secondPhaseQuotes[MAX_SECOND_PHASE_QUOTES] =
{
    { 26260, "Is he... coming apart?" }, // Kalecgos
    { 25895, "Aspects! Aid the heroes as best you can!" }, // Thrall
    { 26499, "He's completely mad!" }, // Alexstrasza
    { 25896, "These champions are our only hope for exposing Deathwing's weakness. Together, we can prevail." }, // Thrall
    { 25950, "Press the attack, heroes!" }, // Nozdormu
    { 25897, "Don't give up! We're counting on you to push Deathwing back." }, // Thrall
    { 26143, "Such rage I have never seen..." }, // Ysera
    { 25902, "Strike now, Heroes! Only you can give us our opening!" }, // Thrall
};

const int8 MAX_OUTRO_QUOTES = 4;

static const PlayableQuote outroQuotes[MAX_OUTRO_QUOTES] =
{
    { 26497, "Excellent work. The fire of my heart glows with a brilliant purity unmatched; every spark of it I will channel into the Dragon Soul." }, // Alexstrasza
    { 26258, "Well done! I will realign the flow of mana and fill the Dragon Soul with all my arcane might." }, // Kalecgos
    { 25948, "It is time. I will expend everything to bind every thread here, now, around the Dragon Soul. What comes to pass will NEVER be undone." }, // Nozdormu
    { 26141, "We are one step closer. The unknowable, transcendent power of the Emerald Dream I now give unto the Dragon Soul." }, // Ysera
};

class npc_ds_madness_thrall : public CreatureScript
{
public:
    npc_ds_madness_thrall() : CreatureScript("npc_ds_madness_thrall") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        if (pCreature->GetInstanceScript()->GetData(TYPE_BOSS_MADNESS_OF_DEATHWING) != 3)
        {
            if (pCreature->AI()->GetData(ACTION_THRALL_INTRO_SPEECH) == 0)
            {
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_DESTROYER_DEFEAT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                pPlayer->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, pCreature->GetGUID());
            }
            else
            {
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_WHATS_THAT_NOICE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                pPlayer->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, pCreature->GetGUID());
            }
        }
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction) override
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
        {
            pCreature->AI()->DoAction(ACTION_THRALL_INTRO_SPEECH);
            pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            pPlayer->CLOSE_GOSSIP_MENU();
        }
        else if (uiAction == GOSSIP_ACTION_INFO_DEF + 2)
        {
            pCreature->AI()->DoAction(ACTION_START_MADNESS_ENCOUNTER);
            pPlayer->CLOSE_GOSSIP_MENU();
        }
        return true;
    }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_madness_thrallAI(pCreature);
    }

    struct npc_ds_madness_thrallAI : public ScriptedAI
    {
        npc_ds_madness_thrallAI(Creature* pCreature) : ScriptedAI(pCreature), summons(pCreature)
        {
            instance = pCreature->GetInstanceScript();
            scheduler.Schedule(Seconds(2), [this, pCreature](TaskContext introSpeech)
            {
                pCreature->GetAI()->DoAction(ACTION_SPAWN_JUMP_PADS);

                me->SummonCreature(NPC_ALEXSTRASZA_DRAGON_FORM, alexstraszaPos, TEMPSUMMON_MANUAL_DESPAWN);
                me->SummonCreature(NPC_KALECGOS_DRAGON_FORM, kalecgosPos, TEMPSUMMON_MANUAL_DESPAWN);
                me->SummonCreature(NPC_NOZDORMU_DRAGON_FORM, nozdormuPos, TEMPSUMMON_MANUAL_DESPAWN);
                me->SummonCreature(NPC_YSERA_DRAGON_FORM, yseraPos, TEMPSUMMON_MANUAL_DESPAWN);
            });
        }

        InstanceScript* instance;
        TaskScheduler scheduler;
        SummonList summons;
        uint8 intro;
        uint8 quote;

        void Reset() override
        {
            intro = 0;
            quote = 0;

            Dragonsoul();
        }

        void Dragonsoul(bool summon = true)
        {
            if (Vehicle * veh = me->GetVehicleKit())
            {
                if (summon == true)
                {
                    if (Creature * vehiclePassenger = me->SummonCreature(NPC_THE_DRAGON_SOUL_MADNESS, 0, 0, 0))
                    {
                        vehiclePassenger->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        vehiclePassenger->AddAura(110489, vehiclePassenger); // Dragon Soul Cosmetic
                        vehiclePassenger->EnterVehicle(me, 0);
                    }
                }
                else
                {
                    if (Unit * vehiclePassenger = veh->GetPassenger(0))
                    {
                        vehiclePassenger->ExitVehicle();
                        vehiclePassenger->ToCreature()->DespawnOrUnsummon();
                    }
                }
            }
        }

        void JustSummoned(Creature* summon) override
        {
            summons.push_back(summon->GetGUID());
        }

        void DoAction(const int32 action) override
        {
            Creature* pAlexstrasza = me->FindNearestCreature(NPC_ALEXSTRASZA_DRAGON_FORM, 300.0f, true);
            Creature* pKalecgos = me->FindNearestCreature(NPC_KALECGOS_DRAGON_FORM, 300.0f, true);
            Creature* pNozdormu = me->FindNearestCreature(NPC_NOZDORMU_DRAGON_FORM, 300.0f, true);
            Creature* pYsera = me->FindNearestCreature(NPC_YSERA_DRAGON_FORM, 300.0f, true);

            switch (action)
            {
            case ACTION_THRALL_INTRO_SPEECH:
                intro = 1;
                me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                scheduler.Schedule(Seconds(0), [this, pAlexstrasza, pKalecgos, pYsera, pNozdormu](TaskContext introSpeech)
                {
                    if (pAlexstrasza && pKalecgos && pYsera && pNozdormu)
                    {
                        if (introSpeech.GetRepeatCounter() == 0)
                        {
                            me->MonsterYell("It is done; at last the Destroyer has met his end. Now, we can begin to heal our world.", LANG_UNIVERSAL, false);
                            me->SendPlaySound(25901, false);
                            introSpeech.Repeat(Seconds(10));
                        }
                        else if (introSpeech.GetRepeatCounter() == 1)
                        {
                            pAlexstrasza->MonsterYell("You have accomplished the impossible, succeeded where all Azeroth feared you would fail.", LANG_UNIVERSAL, false);
                            me->SendPlaySound(26501, false);
                            introSpeech.Repeat(Seconds(9));
                        }
                        else if (introSpeech.GetRepeatCounter() == 2)
                        {
                            pAlexstrasza->MonsterYell("Before you is the Cache of the Aspects. Take from it what you will, for you have more than earned the right.", LANG_UNIVERSAL, false);
                            me->SendPlaySound(26502, false);
                            introSpeech.Repeat(Seconds(11));
                        }
                        else if (introSpeech.GetRepeatCounter() == 3)
                        {
                            pNozdormu->MonsterYell("...although, you may want to hurry.", LANG_UNIVERSAL, false);
                            me->SendPlaySound(25952, false);
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        }
                    }
                });
                break;
            case ACTION_SPAWN_JUMP_PADS:
                for (uint8 i = 0; i < 36; i++)
                {
                    uint32 platformPos = 0;
                    if (i <= 5)
                        platformPos = 0;
                    else if (i <= 11)
                        platformPos = 1;
                    else if (i <= 17)
                        platformPos = 2;
                    else if (i <= 23)
                        platformPos = 3;
                    else if (i <= 29)
                        platformPos = 4;
                    else if (i <= 35)
                        platformPos = 5;

                    if (Creature * pJumpPad = me->SummonCreature(NPC_JUMP_PAD, jumpPadSpawnPos[i], TEMPSUMMON_MANUAL_DESPAWN))
                        pJumpPad->AI()->SetData(ACTION_SPAWN_JUMP_PADS, platformPos);
                }
                break;
            case ACTION_START_MADNESS_ENCOUNTER:
                me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->CastSpell(me, SPELL_ASTRAL_RECALL, false);
                quote = 0;

                scheduler.Schedule(Seconds(2), [this](TaskContext /*task context*/)
                {
                    if (Creature* pMadnessDeathwing = me->FindNearestCreature(NPC_MADNESS_OF_DEATHWING, 300.0f, true))
                    {
                        pMadnessDeathwing->SetInCombatWithZone();
                        pMadnessDeathwing->CastSpell(pMadnessDeathwing, 75764, false); // Emerge
                    }

                    me->GetMotionMaster()->MoveJump(thrallPos.GetPositionX(), thrallPos.GetPositionY(), thrallPos.GetPositionZ(), 100.0f, 100.0f);
                    me->NearTeleportTo(thrallPos.GetPositionX(), thrallPos.GetPositionY(), thrallPos.GetPositionZ(), thrallPos.GetOrientation());
                    me->CastSpell(me, SPELL_HOVER, true);

                    scheduler.Schedule(Seconds(1), [this](TaskContext /*task context*/)
                    {
                        me->SetFacingTo(thrallPos.GetOrientation());
                        me->SetUnitMovementFlags(MOVEMENTFLAG_HOVER | MOVEMENTFLAG_FLYING);
                    });
                });
                break;
            case ACTION_TELEPORT_HOME:
                {
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    me->RemoveAura(SPELL_HOVER);
                    quote = 0;
                    me->GetMotionMaster()->MoveJump(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY(), me->GetHomePosition().GetPositionZ(), 100.0f, 100.0f);
                    me->NearTeleportTo(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY(), me->GetHomePosition().GetPositionZ(), me->GetHomePosition().GetOrientation());
                    scheduler.Schedule(Seconds(2), [this](TaskContext /*task context*/)
                    {
                        me->SetFacingTo(me->GetHomePosition().GetOrientation());
                        scheduler.CancelAll();
                    });

                    if (pAlexstrasza && pKalecgos && pYsera && pNozdormu)
                    {
                        pAlexstrasza->CastSpell(pAlexstrasza, SPELL_ALEXSTRASZA_PRESENCE, false);
                        pNozdormu->CastSpell(pNozdormu, SPELL_NOZDORMU_PRESENCE, false);
                        pYsera->CastSpell(pYsera, SPELL_THE_DREAMER, false);
                        pYsera->CastSpell(pYsera, SPELL_YSERA_PRESENCE, false);
                        pKalecgos->CastSpell(pKalecgos, SPELL_SPELLWEAVER, false);
                        pKalecgos->CastSpell(pKalecgos, SPELL_KALECGOS_PRESENCE, false);
                    }
                }
                break;
            case ACTION_ASPECTS_SECOND_PHASE:
                Dragonsoul(false);
                me->SetVisible(false);
                me->GetMotionMaster()->MovePoint(0, thrallPosIsland.GetPositionX(), thrallPosIsland.GetPositionY(), thrallPosIsland.GetPositionZ());

                scheduler.Schedule(Seconds(1), [this](TaskContext /*task context*/)
                {
                    Dragonsoul();
                    me->SetVisible(true);
                    me->SetFacingTo(thrallPos.GetOrientation());
                });

                if (pAlexstrasza && pKalecgos && pYsera && pNozdormu)
                {
                    // Schedule raid buffs
                    scheduler.Schedule(Seconds(5), [this, pAlexstrasza, pYsera, pNozdormu, pKalecgos](TaskContext quote)
                    {
                        pAlexstrasza->InterruptNonMeleeSpells(true, 0, true);
                        pYsera->InterruptNonMeleeSpells(true, 0, true);
                        pNozdormu->InterruptNonMeleeSpells(true, 0, true);
                        pKalecgos->InterruptNonMeleeSpells(true, 0, true);

                        pAlexstrasza->CastSpell(pAlexstrasza, SPELL_ALEXSTRASZA_PRESENCE, false);
                        pNozdormu->CastSpell(pNozdormu, SPELL_NOZDORMU_PRESENCE, false);
                        pYsera->CastSpell(pYsera, SPELL_THE_DREAMER, false);
                        pYsera->CastSpell(pYsera, SPELL_YSERA_PRESENCE, false);
                        pKalecgos->CastSpell(pKalecgos, SPELL_SPELLWEAVER, false);
                        pKalecgos->CastSpell(pKalecgos, SPELL_KALECGOS_PRESENCE, false);
                    });
                    // Schedule Quotes
                    scheduler.Schedule(Seconds(30), 1, [this, pAlexstrasza, pYsera, pNozdormu, pKalecgos](TaskContext quote)
                    {
                        if (quote.GetRepeatCounter() % 2 == 1)
                            RunPlayableQuote(secondPhaseQuotes[quote.GetRepeatCounter()]);
                        else
                        {
                            if (quote.GetRepeatCounter() == 0)
                                pKalecgos->MonsterYell(secondPhaseQuotes[quote.GetRepeatCounter()].GetText(), LANG_UNIVERSAL, 0);
                            if (quote.GetRepeatCounter() == 2)
                                pAlexstrasza->MonsterYell(secondPhaseQuotes[quote.GetRepeatCounter()].GetText(), LANG_UNIVERSAL, 0);
                            if (quote.GetRepeatCounter() == 4)
                                pNozdormu->MonsterYell(secondPhaseQuotes[quote.GetRepeatCounter()].GetText(), LANG_UNIVERSAL, 0);
                            if (quote.GetRepeatCounter() == 6)
                                pYsera->MonsterYell(secondPhaseQuotes[quote.GetRepeatCounter()].GetText(), LANG_UNIVERSAL, 0);

                            me->SendPlaySound(secondPhaseQuotes[quote.GetRepeatCounter()].GetSoundId(), false);
                        }
                        if (quote.GetRepeatCounter() < MAX_SECOND_PHASE_QUOTES - 1)
                            quote.Repeat(Seconds(30));
                    });
                }
                break;
            case ACTION_ASPECTS_OUTRO:
                Dragonsoul(false);
                me->SetVisible(false);
                me->GetMotionMaster()->MovePoint(0, thrallPos.GetPositionX(), thrallPos.GetPositionY(), thrallPos.GetPositionZ());

                scheduler.Schedule(Seconds(1), [this](TaskContext /*task context*/)
                {
                    Dragonsoul();
                    me->SetVisible(true);
                    me->SetFacingTo(thrallPos.GetOrientation());
                });

                scheduler.CancelGroup(1);
                scheduler.Schedule(Seconds(5), [this, pAlexstrasza, pYsera, pNozdormu, pKalecgos](TaskContext quoteOutro)
                {
                    if (pAlexstrasza && pKalecgos && pYsera && pNozdormu)
                    {
                        if (quoteOutro.GetRepeatCounter() < MAX_OUTRO_QUOTES)
                            me->SendPlaySound(outroQuotes[quoteOutro.GetRepeatCounter()].GetSoundId(), false);

                        if (quoteOutro.GetRepeatCounter() == 0)
                        {
                            pAlexstrasza->CastSpell(me, SPELL_CONCENTRATION_ALEXSTRASZA, true);
                            pAlexstrasza->MonsterYell(outroQuotes[quoteOutro.GetRepeatCounter()].GetText(), LANG_UNIVERSAL, 0);
                            quoteOutro.Repeat(Seconds(10));
                        }
                        else if (quoteOutro.GetRepeatCounter() == 1)
                        {
                            pKalecgos->CastSpell(me, SPELL_CONCENTRATION_KALECGOS, true);
                            pKalecgos->MonsterYell(outroQuotes[quoteOutro.GetRepeatCounter()].GetText(), LANG_UNIVERSAL, 0);
                            quoteOutro.Repeat(Seconds(9));
                        }
                        else if (quoteOutro.GetRepeatCounter() == 2)
                        {
                            pNozdormu->CastSpell(me, SPELL_CONCENTRATION_NOZDORMU, true);
                            pNozdormu->MonsterYell(outroQuotes[quoteOutro.GetRepeatCounter()].GetText(), LANG_UNIVERSAL, 0);
                            quoteOutro.Repeat(Seconds(15));
                        }
                        else if (quoteOutro.GetRepeatCounter() == 3)
                        {
                            pYsera->CastSpell(me, SPELL_CONCENTRATION_YSERA, true);
                            pYsera->MonsterYell(outroQuotes[quoteOutro.GetRepeatCounter()].GetText(), LANG_UNIVERSAL, 0);
                            quoteOutro.Repeat(Seconds(11));
                        }
                        else if (quoteOutro.GetRepeatCounter() == 4)
                        {
                            me->CastSpell(me, SPELL_FIRE_DRAGON_SOUL, false);
                            quoteOutro.Repeat(Seconds(15));
                        }
                        else if (quoteOutro.GetRepeatCounter() == 5)
                        {
                            pAlexstrasza->InterruptNonMeleeSpells(true, 0, true);
                            pNozdormu->InterruptNonMeleeSpells(true, 0, true);
                            pYsera->InterruptNonMeleeSpells(true, 0, true);
                            pKalecgos->InterruptNonMeleeSpells(true, 0, true);
                        }
                    }
                });
                break;
            case ACTION_ASPECTS_HIDE:
                summons.DespawnAll();
                me->DespawnOrUnsummon();
                break;
            default:
                break;
            }
        }

        uint32 GetData(uint32 data)
        {
            return intro;
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);
        }
    };
};

class npc_ds_madness_aspects_dragon_form : public CreatureScript
{
public:
    npc_ds_madness_aspects_dragon_form() : CreatureScript("npc_ds_madness_aspects_dragon_form") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_madness_aspects_dragon_formAI(pCreature);
    }

    struct npc_ds_madness_aspects_dragon_formAI : public ScriptedAI
    {
        npc_ds_madness_aspects_dragon_formAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        void Reset() override
        {
            me->SetFlying(true);
            switch (me->GetEntry())
            {
            case NPC_ALEXSTRASZA_DRAGON_FORM:
                me->CastSpell(me, SPELL_ALEXSTRASZA_PRESENCE, false);
                break;
            case NPC_NOZDORMU_DRAGON_FORM:
                me->CastSpell(me, SPELL_NOZDORMU_PRESENCE, false);
                break;
            case NPC_KALECGOS_DRAGON_FORM:
                me->CastSpell(me, SPELL_KALECGOS_PRESENCE, false);
                me->CastSpell(me, SPELL_SPELLWEAVER, false);
                break;
            case NPC_YSERA_DRAGON_FORM:
                me->CastSpell(me, SPELL_YSERA_PRESENCE, false);
                me->CastSpell(me, SPELL_THE_DREAMER, false);
                break;
            default:
                break;
            }
        }

        void UpdateAI(const uint32 diff) override { }
    };
};

enum JumpPads
{
    JUMP_PAD_1_TO_2                     = 0,
    JUMP_PAD_2_TO_1                     = 1,
    JUMP_PAD_2_TO_3                     = 2,
    JUMP_PAD_3_TO_2                     = 3,
    JUMP_PAD_3_TO_4                     = 4,
    JUMP_PAD_4_TO_3                     = 5,
};

class npc_ds_madness_jump_pad : public CreatureScript
{
public:
    npc_ds_madness_jump_pad() : CreatureScript("npc_ds_madness_jump_pad") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_madness_jump_padAI(pCreature);
    }

    struct npc_ds_madness_jump_padAI : public ScriptedAI
    {
        npc_ds_madness_jump_padAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 jumpPadPos;

        void Reset() override
        {
            jumpPadPos = 0;
            me->SetVisible(false);
        }

        void SetData(uint32 type, uint32 data) override
        {
            if (type == ACTION_SPAWN_JUMP_PADS)
            {
                jumpPadPos = data;
            }
        }

        void MoveInLineOfSight(Unit *who)
        {
            if (!who)
                return;

            if (who->GetTypeId() == TYPEID_PLAYER && !who->ToPlayer()->IsGameMaster() && who->GetExactDist2d(me) < 9.0f)
            {
                uint8 jumpLandPos = 0;
                switch (jumpPadPos)
                {
                case JUMP_PAD_1_TO_2:
                    jumpLandPos = JUMP_PAD_1_TO_2;
                    break;
                case JUMP_PAD_2_TO_1:
                    jumpLandPos = JUMP_PAD_2_TO_1;
                    break;
                case JUMP_PAD_2_TO_3:
                    jumpLandPos = JUMP_PAD_2_TO_3;
                    break;
                case JUMP_PAD_3_TO_2:
                    jumpLandPos = JUMP_PAD_3_TO_2;
                    break;
                case JUMP_PAD_3_TO_4:
                    jumpLandPos = JUMP_PAD_3_TO_4;
                    break;
                case JUMP_PAD_4_TO_3:
                    jumpLandPos = JUMP_PAD_4_TO_3;
                    break;
                default:
                    break;
                }
                if (!who->HasAura(SPELL_CARRYING_WINDS_SPEED_10N) && !who->HasAura(SPELL_CARRYING_WINDS_SPEED_25N)
                    && !who->HasAura(SPELL_CARRYING_WINDS_SPEED_10HC) && !who->HasAura(SPELL_CARRYING_WINDS_SPEED_25HC))
                {
                    who->GetMotionMaster()->MoveJump(jumpPadLandPos[jumpLandPos].GetPositionX(), jumpPadLandPos[jumpLandPos].GetPositionY(), jumpPadLandPos[jumpLandPos].GetPositionZ(), 25.0f, 15.0f);
                    who->CastSpell(who, SPELL_CARRYING_WINDS_SPEED_10N, true);
                }
            }
        }
    };
};

////////////////////////////
/// Other instance stuff ///
////////////////////////////

static const Position telePos[6] =
{
    {  -1779.50f,  -2393.43f,  45.61f, 3.20f }, // Wyrmrest Temple/Base
    {  -1804.10f,  -2402.60f, 341.35f, 0.48f }, // Wyrmrest Summit
    {  13629.35f,  13612.09f, 123.49f, 3.14f }, // Hagara
    {  13444.90f, -12133.30f, 151.21f, 0.00f }, // Skifire (Ship)
    { -13852.50f, -13665.38f, 297.37f, 1.53f }, // Spine of Deathwing
    { -12081.39f,  12160.05f,  30.60f, 6.03f }, // Madness of Deathwing
};

class go_ds_instance_teleporter : public GameObjectScript
{
public:
    go_ds_instance_teleporter() : GameObjectScript("go_ds_instance_teleporter") { }

    bool OnGossipHello(Player* pPlayer, GameObject* pGameObject)
    {
        if (InstanceScript* pInstance = pGameObject->GetInstanceScript())
        {
            // Disable teleport if encounter in progress
            for (uint8 i = 0; i < MAX_ENCOUNTER; i++)
            {
                if (pInstance->GetData(i) == IN_PROGRESS)
                    return true;
            }

            switch (pGameObject->GetEntry())
            {
                // Base Teleporter near Entrance
            case GO_TRAVEL_TO_WYRMREST_TEMPLE:
                if (pInstance->GetData(TYPE_BOSS_SPINE_OF_DEATHWING) == DONE)
                    pPlayer->NearTeleportTo(telePos[5].GetPositionX(), telePos[5].GetPositionY(), telePos[5].GetPositionZ(), telePos[5].GetOrientation());
                else if (pInstance->GetData(TYPE_BOSS_BLACKHORN) == DONE || pInstance->GetData(TYPE_BOSS_ULTRAXION) == DONE)
                    pPlayer->NearTeleportTo(telePos[3].GetPositionX(), telePos[3].GetPositionY(), telePos[3].GetPositionZ(), telePos[3].GetOrientation());
                else if (pInstance->GetData(TYPE_BOSS_HAGARA) == DONE
                    || (pInstance->GetData(TYPE_BOSS_ZONOZZ) == DONE && pInstance->GetData(TYPE_BOSS_YORSAHJ) == DONE))
                    pPlayer->NearTeleportTo(telePos[1].GetPositionX(), telePos[1].GetPositionY(), telePos[1].GetPositionZ(), telePos[1].GetOrientation());
                else if (pInstance->GetData(TYPE_BOSS_MORCHOK) == DONE)
                    pPlayer->NearTeleportTo(telePos[0].GetPositionX(), telePos[0].GetPositionY(), telePos[0].GetPositionZ(), telePos[0].GetOrientation());
                break;
                // Zonozz and Yorsahj teleport back to wyrmrest temple
            case GO_TRAVEL_TO_WYRMREST_BASE:
                pPlayer->NearTeleportTo(telePos[0].GetPositionX(), telePos[0].GetPositionY(), telePos[0].GetPositionZ(), telePos[0].GetOrientation());
                break;
                // Hagara back teleporter
            case GO_TRAVEL_TO_WYRMREST_SUMMIT:
                pPlayer->NearTeleportTo(telePos[1].GetPositionX(), telePos[1].GetPositionY(), telePos[1].GetPositionZ(), telePos[1].GetOrientation());
                break;
                // Hagara teleporter
            case GO_TRAVEL_TO_EYE_OF_ETERNITY:
                if (pInstance->GetData(TYPE_BOSS_ZONOZZ) == DONE)
                    pPlayer->NearTeleportTo(telePos[2].GetPositionX(), telePos[2].GetPositionY(), telePos[2].GetPositionZ(), telePos[2].GetOrientation());
                break;
                // Skifire teleporter
            case GO_TRAVEL_TO_DECK:
                if (pInstance->GetData(TYPE_BOSS_ULTRAXION) == DONE)
                    pPlayer->NearTeleportTo(telePos[3].GetPositionX(), telePos[3].GetPositionY(), telePos[3].GetPositionZ(), telePos[3].GetOrientation());
                break;
                // Maelstrom teleporter
            case GO_TRAVEL_TO_MAELSTROM:
                if (pInstance->GetData(TYPE_BOSS_SPINE_OF_DEATHWING) == DONE)
                {
                    pPlayer->NearTeleportTo(telePos[6].GetPositionX(), telePos[6].GetPositionY(), telePos[6].GetPositionZ(), telePos[6].GetOrientation());
                    pPlayer->CastSpell(pPlayer, SPELL_PARACHUTE, true);
                }
                break;
            default:
                break;
            }

        }
        return true;
    }
};

class npc_ds_instance_teleporter : public CreatureScript
{
public:
    npc_ds_instance_teleporter() : CreatureScript("npc_ds_instance_teleporter") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_instance_teleporterAI(pCreature);
    }

    struct npc_ds_instance_teleporterAI : public ScriptedAI
    {
        npc_ds_instance_teleporterAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();

            if (instance && !(instance->GetData(TYPE_BOSS_MORCHOK) == DONE))
                me->SetVisible(false);
            if (instance && (instance->GetData(TYPE_BOSS_HAGARA) == DONE) && (me->GetEntry() == NPC_TRAVEL_TO_EYE_OF_ETERNITY))
                me->SetVisible(true);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        }

        InstanceScript * instance;

        void SpellHit(Unit* /*caster*/, SpellEntry const* spell) override
        {
            if (spell->Id == SPELL_OPEN_EYE_OF_ETERNITY_PORTAL)
            {
                me->SetVisible(true);
                me->SummonGameObject(GO_TRAVEL_TO_EYE_OF_ETERNITY, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() - 3, me->GetOrientation(), 0, 0, 0, 0, 604800);
            }
        }
    };
};

enum Drakes
{
    NPC_RED_DRAKE                   = 119553,
    NPC_EMERALD_DRAKE               = 119554,
    NPC_BRONZE_DRAKE                = 119555,

    SPELL_PARACHUTE_DROP            = 97443,
};

static const Position travelPos[12] =
{
    { -1794.43f, -2362.44f,  50.28f, 1.49f }, // Valeera spawn drake position
    { -1777.35f, -2423.51f,  50.28f, 4.68f }, // Eiendormi spawn drake position
    { -1756.18f, -2386.45f,  50.28f, 0.04f }, // Nethestrasz spawn drake position
    { -1765.21f, -1921.51f, 130.78f, 1.44f }, // Zonozz fly position
    { -1769.78f, -3032.23f, 126.20f, 4.35f }, // Yorsahj fly position
    { -1745.26f, -1845.62f,-221.30f, 1.27f }, // Zonozz landing position
    { -1853.23f, -3070.28f,-178.30f, 0.40f }, // Yorshaj landing position
    { -1681.13f, -2382.85f,  78.50f, 0.07f }, // 1st fly up position
    { -1649.47f, -2289.83f, 145.60f, 2.40f }, // 2nd fly up position
    { -1734.94f, -2243.27f, 258.05f, 3.59f }, // 3rd fly up position
    { -1804.51f, -2313.50f, 339.84f, 4.57f }, // 4th fly up position
    { -1799.54f, -2367.86f, 345.08f, 5.20f }, // Landing top of the wyrmrest temple
};

const uint8 ACTION_REMOVE_FLAG                 = 0;

class npc_ds_travel_with_drakes : public CreatureScript
{
public:
    npc_ds_travel_with_drakes() : CreatureScript("npc_ds_travel_with_drakes") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        if (pCreature->GetEntry() == NPC_VALEERA)
            pPlayer->SummonCreature(NPC_EMERALD_DRAKE, travelPos[0], TEMPSUMMON_MANUAL_DESPAWN);
        else if (pCreature->GetEntry() == NPC_EIENDORMI)
            pPlayer->SummonCreature(NPC_BRONZE_DRAKE, travelPos[1], TEMPSUMMON_MANUAL_DESPAWN);
        else if (pCreature->GetEntry() == NPC_NETHESTRASZ)
            pPlayer->SummonCreature(NPC_RED_DRAKE, travelPos[2], TEMPSUMMON_MANUAL_DESPAWN);

        pCreature->AI()->DoAction(ACTION_REMOVE_FLAG);
        pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        return true;
    }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_travel_with_drakesAI(pCreature);
    }

    struct npc_ds_travel_with_drakesAI : public ScriptedAI
    {
        npc_ds_travel_with_drakesAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            if ((me->GetEntry() == NPC_VALEERA && instance->GetData(TYPE_BOSS_MORCHOK) == DONE) ||
                (me->GetEntry() == NPC_EIENDORMI && instance->GetData(TYPE_BOSS_ZONOZZ) == DONE) ||
                (me->GetEntry() == NPC_NETHESTRASZ && instance->GetData(TYPE_BOSS_YORSAHJ) == DONE))
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            }

            summonerGuid = 0;
            ejectPassanger = false;
        }

        InstanceScript * instance;
        TaskScheduler scheduler;
        uint64 summonerGuid;
        uint32 ejectTimer;
        bool ejectPassanger;

        void IsSummonedBy(Unit* pSummoner) override
        {
            summonerGuid = pSummoner->GetGUID();
        }

        void DoAction(const int32 action) override
        {
            if (action == ACTION_REMOVE_FLAG)
            {
                scheduler.Schedule(Seconds(5), [this](TaskContext removeFlag)
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                });
            }
        }

        void Reset() override
        {
            me->SetFlying(true);
            me->SetSpeed(MOVE_FLIGHT, 4.0f);

            if (me->GetEntry() != NPC_VALEERA && me->GetEntry() != NPC_EIENDORMI && me->GetEntry() != NPC_NETHESTRASZ)
            {
                scheduler.Schedule(Seconds(2), [this](TaskContext fly)
                {
                    if (fly.GetRepeatCounter() == 0)
                    {
                        if (Player* pPlayer = ObjectAccessor::GetPlayer(*me, summonerGuid))
                            pPlayer->EnterVehicle(me, 0, nullptr);
                        fly.Repeat(Seconds(3));
                    }
                    else
                    {
                        if (me->GetEntry() == NPC_EMERALD_DRAKE)
                        {
                            me->GetMotionMaster()->MovePoint(0, travelPos[3]);
                            ejectTimer = 17000;
                        }
                        else if (me->GetEntry() == NPC_BRONZE_DRAKE)
                        {
                            me->GetMotionMaster()->MovePoint(0, travelPos[4]);
                            ejectTimer = 23000;
                        }
                        else if (me->GetEntry() == NPC_RED_DRAKE)
                        {
                            scheduler.Schedule(Seconds(0), [this](TaskContext fly)
                            {
                                uint32 flyPosition = 0;
                                uint32 repeatTime = 60000;

                                switch (fly.GetRepeatCounter())
                                {
                                case 0:
                                    flyPosition = 7;
                                    repeatTime = 3;
                                    break;
                                case 1:
                                    flyPosition = 8;
                                    repeatTime = 4;
                                    break;
                                case 2:
                                    flyPosition = 9;
                                    repeatTime = 4;
                                    break;
                                case 3:
                                    flyPosition = 10;
                                    repeatTime = 5;
                                    break;
                                case 4:
                                    flyPosition = 11;
                                    ejectTimer = 3000;
                                    ejectPassanger = true;
                                    break;
                                default:
                                    break;
                                }
                                me->GetMotionMaster()->MovePoint(0, travelPos[flyPosition], true, true);
                                if (fly.GetRepeatCounter() != 4)
                                    fly.Repeat(Seconds(repeatTime));
                            });
                        }
                        if (me->GetEntry() != NPC_RED_DRAKE)
                            ejectPassanger = true;
                    }
                });
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (ejectPassanger)
            {
                if (ejectTimer <= diff)
                {
                    if (Vehicle * veh = me->GetVehicleKit())
                    {
                        if (Unit * passenger = veh->GetPassenger(0))
                        {
                            passenger->ExitVehicle();
                        }
                    }
                    ejectPassanger = false;
                    scheduler.Schedule(Seconds(2), [this](TaskContext hide)
                    {
                        if (hide.GetRepeatCounter() == 0)
                        {
                            me->SetVisible(false);
                            if (me->GetEntry() != NPC_RED_DRAKE)
                                hide.Repeat(Seconds(3));
                        }
                        else
                        {
                            if (Player* pPlayer = ObjectAccessor::GetPlayer(*me, summonerGuid))
                            {
                                if (me->GetEntry() == NPC_EMERALD_DRAKE)
                                    pPlayer->GetMotionMaster()->MoveJump(travelPos[5].GetPositionX(), travelPos[5].GetPositionY(), travelPos[5].GetPositionZ(), 20.0f, 10.0f);
                                else if (me->GetEntry() == NPC_BRONZE_DRAKE)
                                    pPlayer->GetMotionMaster()->MoveJump(travelPos[6].GetPositionX(), travelPos[6].GetPositionY(), travelPos[6].GetPositionZ(), 30.0f, 10.0f);
                                pPlayer->CastSpell(pPlayer, SPELL_PARACHUTE_DROP, true);
                            }
                            me->Kill(me);
                        }
                    });
                }
                else ejectTimer -= diff;
            }
        }
    };
};

// Dethwings position from where he talks to players and aspects
const Position greetingsPos = { -1690.0f, -2384.0f, 358.0f, 0.0f };
const Position leavePos = { -1729.0f, -2165.0f, 372.0f, 0.0f };

static const Position spawnDrakePos[8] =
{
    { -1774.57f, -2352.91f, 352.62f, 4.38f }, // 0
    { -1804.68f, -2358.17f, 352.62f, 5.20f }, // 1
    { -1824.35f, -2382.29f, 352.62f, 6.00f }, // 2
    { -1819.90f, -2411.20f, 352.62f, 0.51f }, // 3
    { -1797.69f, -2428.33f, 352.62f, 1.31f }, // 4
    { -1769.29f, -2425.72f, 352.62f, 2.07f }, // 5
    { -1749.33f, -2402.71f, 352.62f, 2.78f }, // 6
    { -1752.28f, -2376.94f, 352.62f, 3.63f }, // 7
};

class npc_ds_deathwing_wyrmrest_temple : public CreatureScript
{
public:
    npc_ds_deathwing_wyrmrest_temple() : CreatureScript("npc_ds_deathwing_wyrmrest_temple") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_deathwing_wyrmrest_templeAI(pCreature);
    }

    struct npc_ds_deathwing_wyrmrest_templeAI : public ScriptedAI
    {
        npc_ds_deathwing_wyrmrest_templeAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();

            me->GetMotionMaster()->MoveRandom(70.0f);
        }

        InstanceScript * instance;
        TaskScheduler scheduler;

        void Reset() override
        {
            me->SetFlying(true);
            me->SetSpeed(MOVE_FLIGHT, 1.5f);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }

        void JustReachedHome() override
        {
            me->GetMotionMaster()->MoveRandom(70.0f);
        }

        void DoAction(const int32 action) override
        {
            if (instance)
            {
                switch (action)
                {
                case DATA_DEATHWING_GREETINGS:
                    instance->SetData(DATA_ULTRAXION_DRAKES, 0);
                    me->GetMotionMaster()->MovePoint(0, greetingsPos);
                    scheduler.Schedule(Seconds(13), [this](TaskContext greetings)
                    {
                        uint32 repeatCount = greetings.GetRepeatCounter();
                        if (repeatCount == 0)
                        {
                            me->GetMotionMaster()->MoveIdle(MOTION_SLOT_IDLE);
                            me->SetFacingTo(3.18f);
                            me->MonsterYell("It's good to see you again, Alexstrasza. I have been busy in my absence.", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(26360, false);
                            greetings.Repeat(Seconds(8));
                        }
                        else if (repeatCount == 1)
                        {
                            me->MonsterYell("Twisting your pitiful whelps into mindless abominations, bent only to my will. It was a very...painful process.", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(26361, false);
                            greetings.Repeat(Seconds(25));
                        }
                        else if (repeatCount == 2)
                        {
                            me->GetMotionMaster()->MoveTargetedHome();
                        }
                    });
                    break;
                case DATA_SUMMON_ULTRAXION:
                    me->GetMotionMaster()->MovePoint(0, greetingsPos);
                    scheduler.Schedule(Seconds(11), [this](TaskContext greetings)
                    {
                        uint32 repeatCount = greetings.GetRepeatCounter();
                        if (repeatCount == 0)
                        {
                            me->GetMotionMaster()->MoveIdle(MOTION_SLOT_IDLE);
                            me->SetFacingTo(3.18f);
                            me->MonsterYell("Mere whelps, experiments, a means to a greater end. You will see what the research of my clutch has yielded.", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(26362, false);
                            greetings.Repeat(Seconds(11));
                        }
                        else if (repeatCount == 1)
                        {
                            me->MonsterYell("Nefarian? Onyxia? Sinestra? They were nothing. Now you face my ultimate creation.", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(26363, false);
                            greetings.Repeat(Seconds(10));
                        }
                        else if (repeatCount == 2)
                        {
                            me->MonsterYell("The Hour of Twilight is nigh; the sun sets on your pitiful, mortal existence!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(26364, false);
                            greetings.Repeat(Seconds(7));

                        }
                        else if (repeatCount == 3)
                        {
                            me->GetMotionMaster()->MovePoint(0, leavePos);
                            scheduler.Schedule(Seconds(10), [this](TaskContext /* Summon Ultraxion*/)
                            {
                                instance->SetData(DATA_SUMMON_ULTRAXION, 0);
                            });
                        }
                    });
                    break;
                case DATA_ULTRAXION_DRAKES:
                    scheduler.Schedule(Seconds(0), [this](TaskContext summonDrakes)
                    {
                        uint32 npcId = (urand(0, 1) == 0) ? NPC_TWILIGHT_ASSAULTER : NPC_TWILIGHT_ASSAULTERESS;
                        uint32 randPos = urand(0, 7);
                        me->SummonCreature(npcId, spawnDrakePos[randPos], TEMPSUMMON_DEAD_DESPAWN);
                        if (summonDrakes.GetRepeatCounter() < 14)
                            summonDrakes.Repeat(Seconds(15));
                    });
                    break;
                default:
                    break;
                }
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);
        }
    };
};

#define GOSSIP_GUNSHIP_READY                       "We are the Alliance. We are always ready."
#define GOSSIP_START_BLACKHORN_ENCOUNTER           "Bring us in closer!"
#define GOSSIP_SPINE_OF_DEATHWING                  "JUSTICE AND GLORY!"
#define GOSSIP_TELEPORT_TO_MAELSTROM               "Teleport to Maelstrom."

enum gunshipActions
{
    ACTION_READY_TO_FOLLOW_DEATHWING = 0,
    ACTION_START_BLACKHORN_ENCOUNTER = 1,
    ACTION_SPINE_OF_DEATHWING        = 2,
    ACTION_TELEPORT_TO_MAELSTROM     = 3,
    ACTION_START_ENCOUNTER           = 0,
};

class npc_ds_alliance_ship_crew : public CreatureScript
{
public:
    npc_ds_alliance_ship_crew() : CreatureScript("npc_ds_alliance_ship_crew") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        if (pCreature->IsQuestGiver())
            pPlayer->PrepareQuestMenu(pCreature->GetGUID());

        if (pCreature->GetInstanceScript()->GetData(TYPE_BOSS_SPINE_OF_DEATHWING) == DONE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_TELEPORT_TO_MAELSTROM, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
        }
        if (pCreature->GetInstanceScript()->GetData(TYPE_BOSS_BLACKHORN) == DONE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SPINE_OF_DEATHWING, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        }
        else if (pCreature->FindNearestCreature(NPC_WARMASTER_BLACKHORN, 200.0f, true))
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_START_BLACKHORN_ENCOUNTER, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
        }
        else if (pCreature->GetInstanceScript()->GetData(DATA_ASPECTS_PREPARE_TO_CHANNEL) == 3)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_GUNSHIP_READY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        }
        pPlayer->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, pCreature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction) override
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
        {
            if (pCreature->GetInstanceScript()->GetData(DATA_ASPECTS_PREPARE_TO_CHANNEL) == 3)
                pCreature->AI()->DoAction(ACTION_READY_TO_FOLLOW_DEATHWING);
            else
                pPlayer->NearTeleportTo(telePos[3].GetPositionX(), telePos[3].GetPositionY(), telePos[3].GetPositionZ(), telePos[3].GetOrientation());
        }
        else if (uiAction == GOSSIP_ACTION_INFO_DEF + 2)
        {
            pCreature->AI()->DoAction(ACTION_START_BLACKHORN_ENCOUNTER);
        }
        else if (uiAction == GOSSIP_ACTION_INFO_DEF + 3)
        {
            pCreature->AI()->DoAction(ACTION_SPINE_OF_DEATHWING);
            pCreature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        }
        else if (uiAction == GOSSIP_ACTION_INFO_DEF + 4)
        {
            pCreature->AI()->DoAction(ACTION_TELEPORT_TO_MAELSTROM);
        }
        pPlayer->CLOSE_GOSSIP_MENU();
        return true;
    }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_alliance_ship_crewAI(pCreature);
    }

    struct npc_ds_alliance_ship_crewAI : public ScriptedAI
    {
        npc_ds_alliance_ship_crewAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();

            if (instance && instance->GetData(TYPE_BOSS_ULTRAXION) != DONE)
                me->SetVisible(false);
        }

        InstanceScript * instance;
        TaskScheduler scheduler;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);

            if (me->GetEntry() == NPC_SKY_CAPTAIN_SWAYZE)
                me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        }

        void TeleportAllPlayers(uint32 action, bool withParachute)
        {
            Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
            {
                if (Player* player = i->getSource())
                {
                    if (!player->IsGameMaster())
                    {
                        if (action == ACTION_READY_TO_FOLLOW_DEATHWING)
                            player->NearTeleportTo(telePos[3].GetPositionX(), telePos[3].GetPositionY(), telePos[3].GetPositionZ(), telePos[3].GetOrientation());
                        else if (action == ACTION_SPINE_OF_DEATHWING)
                            player->NearTeleportTo(telePos[4].GetPositionX(), telePos[4].GetPositionY(), telePos[4].GetPositionZ(), telePos[4].GetOrientation());
                        else if (action == ACTION_TELEPORT_TO_MAELSTROM)
                            player->NearTeleportTo(telePos[5].GetPositionX(), telePos[5].GetPositionY(), telePos[5].GetPositionZ(), telePos[5].GetOrientation());

                        if (withParachute == true)
                            player->AddAura(SPELL_PARACHUTE, player);
                    }
                }
            }
        }

        void PlayMovieToPlayers(uint32 movieId)
        {
            if (!instance)
                return;

            Map::PlayerList const& plList = instance->instance->GetPlayers();
            if (plList.isEmpty())
                return;

            for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
            {
                if (Player * pPlayer = itr->getSource())
                    pPlayer->SendMovieStart(movieId);
            }
        }

        void DoAction(const int32 action) override
        {
            if (instance)
            {
                if (action == ACTION_READY_TO_FOLLOW_DEATHWING)
                {
                    TeleportAllPlayers(action, false);
                    instance->SetData(DATA_ASPECTS_PREPARE_TO_CHANNEL, 4);
                }
                else if (action == ACTION_START_BLACKHORN_ENCOUNTER)
                {
                    me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    scheduler.Schedule(Seconds(0), [this](TaskContext greetings)
                    {
                        uint32 repeatCount = greetings.GetRepeatCounter();
                        if (repeatCount == 0)
                        {
                            me->MonsterYell("All ahead full. Everything depends on our speed! We can't let the Destroyer get away.", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(26292, false);
                            greetings.Repeat(Seconds(10));
                        }
                        else if (repeatCount == 1)
                        {
                            if (Creature * pGoriona = me->FindNearestCreature(NPC_GORIONA, 300.0f, true))
                                pGoriona->AI()->DoAction(ACTION_START_ENCOUNTER);
                            greetings.Repeat(Seconds(8));
                        }
                        else if (repeatCount == 2)
                        {
                            me->MonsterYell("Our engines are damaged! We're sitting ducks up here!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(26303, false);
                        }
                    });
                }
                else if (action == ACTION_SPINE_OF_DEATHWING)
                {
                    PlayMovieToPlayers(74);
                    scheduler.Schedule(Seconds(19), [this](TaskContext /*Show Movie and teleport to Spine of Deathwing*/)
                    {
                        TeleportAllPlayers(ACTION_SPINE_OF_DEATHWING, true);
                        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    });
                }
                else if (action == ACTION_TELEPORT_TO_MAELSTROM)
                {
                    TeleportAllPlayers(ACTION_TELEPORT_TO_MAELSTROM, true);
                }
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);
        }
    };
};

void AddSC_dragon_soul_trash()
{
    new npc_ds_ancient_water_lord();
    new npc_ds_earthen_destroyer();
    new npc_ds_earthen_soldier();
    new npc_ds_twilight_siege_captain();
    new npc_ds_twilight_siege_breaker();
    new npc_ds_eye_of_gorath();
    new npc_ds_claw_of_gorath();
    new npc_ds_flail_of_gorath();
    new npc_ds_globule();
    new npc_ds_twilight_frost_evoker();
    new npc_ds_stormborn_myrmidon();
    new npc_ds_stormbinder_adept();
    new npc_ds_corrupted_fragment();
    new npc_ds_hagara_tornado();
    new npc_ds_frozen_grasp_stalker();
    new npc_ds_lieutenant_shara();
    new npc_ds_hagara_trash_portal();
    new npc_ultraxion_go_spawn();
    new npc_ds_twilight_assaulter();
    new go_ds_focusing_iris();
    new npc_ds_twilight_sapper();
    new npc_ds_twilight_infiltrator();
    new npc_ds_twilight_elite();
    new npc_ds_warmaster_triggers();
    new npc_ds_engine_stalker();
    new npc_ds_skyfire_deck();
    new npc_ds_madness_thrall();
    new npc_ds_madness_aspects_dragon_form();
    new npc_ds_madness_jump_pad();
    new npc_ds_instance_teleporter();
    new npc_ds_travel_with_drakes();
    new go_ds_instance_teleporter();
    new npc_ds_alliance_ship_crew();
    new npc_ds_deathwing_wyrmrest_temple();
}