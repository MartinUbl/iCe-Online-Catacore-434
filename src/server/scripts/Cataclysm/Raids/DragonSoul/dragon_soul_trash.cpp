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
                    me->CastSpell(pTwilightPortal, SPELL_TWILIGHT_PORTAL_BEAM, false);
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
    SPELL_PSYCHIC_SLICE             = 105671,
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
            blizzardTimer = urand(4000, 6000);
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
                blizzardTimer = 10000;
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
            pHagara->SetVisible(true);
            pHagara->AI()->DoAction(DATA_HAGARA_INTRO_TRASH);
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
    NPC_SKYFIRE                         = 56598,
    NPC_TWILIGHT_ASSAULT_DRAKE_LEFT     = 56855,
    NPC_TWILIGHT_ASSAULT_DRAKE_RIGHT    = 56587,
    NPC_TWILIGHT_ELITE_DREADBLADE       = 56854,
    NPC_TWILIGHT_ELITE_SLAYER           = 56848,
    NPC_TWILIGHT_SAPPER                 = 56923,
    NPC_HARPOON                         = 56681,
};

enum BlackhornTrashSpells
{
    // Twilight Assault Drake
    SPELL_TWILIGHT_BARRAGE          = 107286,
    SPELL_TWILIGHT_BARRAGE_DMG_1    = 107439,
    SPELL_TWILIGHT_BARRAGE_DUMMY    = 107287,
    SPELL_TWILIGHT_BARRAGE_DMG_2    = 107501,
    SPELL_HARPOON                   = 108038,

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
    SPELL_TWILIGHT_FLAMES_AURA      = 108053,
};

const Position harpoonPos[2] =
{
    { 13430.10f, -12161.81f, 154.11f, 1.51f }, // Left harpoon drake position
    { 13432.68f, -12103.32f, 154.11f, 4.66f }, // Right harpoon drake position
};

enum drakeSide
{
    LEFT_HARPOON_POSITION           = 0,
    RIGHT_HARPOON_POSITION          = 1,
};

enum GorionaDrakesPosition
{
    LEFT_DRAKE_SPAWN_POS            = 0,
    RIGHT_DRAKE_SPAWN_POS           = 1,
    LEFT_DRAKE_DROP_POS             = 2,
    RIGHT_DRAKE_DROP_POS            = 3,
    LEFT_DRAKE_END_FLY_POS          = 4,
    RIGHT_DRAKE_END_FLY_POS         = 5,
};

const Position assaultDrakePos[6] =
{
    { 13441.83f, -12184.15f, 172.05f, 1.49f }, // Left Spawn Drake Pos
    { 13447.38f, -12083.55f, 172.05f, 4.45f }, // Right Spawn Drake Pos
    { 13431.26f, -12125.28f, 172.05f, 3.08f }, // Left drake drop add position
    { 13430.35f, -12140.22f, 172.05f, 3.08f }, // Right drake drop add position
    { 13433.40f, -12082.54f, 172.05f, 4.65f }, // Left drake end fly position
    { 13429.12f, -12182.25f, 172.05f, 1.46f }, // Right drake end fly position
};

enum actionsSkyfire
{
    ACTION_START_WARMASTER_ENCOUNTER    = 0,
    ACTION_SHOOT_DRAKE                  = 1,
};

class npc_ds_twilight_assault_drake : public CreatureScript
{
public:
    npc_ds_twilight_assault_drake() : CreatureScript("npc_ds_twilight_assault_drake") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_twilight_assault_drakeAI(pCreature);
    }

    struct npc_ds_twilight_assault_drakeAI : public ScriptedAI
    {
        npc_ds_twilight_assault_drakeAI(Creature *creature) : ScriptedAI(creature) { }

        TaskScheduler scheduler;
        uint32 harpoonTimer;
        uint32 releaseTimer;
        uint32 twilightBarrageTimer;
        bool harpoon;

        void Reset() override
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetFlying(true);
            me->SetSpeed(MOVE_FLIGHT, 1.5f);
            me->SetInCombatWithZone();
            harpoonTimer = 30000;
            twilightBarrageTimer = urand(8000, 16000);
            harpoon = false;

            scheduler.Schedule(Seconds(7), [this](TaskContext /*task context*/)
            {
                if (Vehicle * veh = me->GetVehicleKit())
                {
                    if (Unit * passenger = veh->GetPassenger(0))
                    {
                        passenger->ExitVehicle();
                        passenger->GetMotionMaster()->MoveFall();
                    }
                }

                if (me->GetEntry() == NPC_TWILIGHT_ASSAULT_DRAKE_LEFT)
                    me->GetMotionMaster()->MovePoint(LEFT_DRAKE_END_FLY_POS, assaultDrakePos[LEFT_DRAKE_END_FLY_POS], true, false);
                else
                    me->GetMotionMaster()->MovePoint(RIGHT_DRAKE_END_FLY_POS, assaultDrakePos[RIGHT_DRAKE_END_FLY_POS], true, false);

                scheduler.Schedule(Seconds(5), [this](TaskContext /*task context*/)
                {
                    if (me->GetEntry() == NPC_TWILIGHT_ASSAULT_DRAKE_LEFT)
                        me->SetFacingTo(assaultDrakePos[LEFT_DRAKE_END_FLY_POS].GetOrientation());
                    else
                        me->SetFacingTo(assaultDrakePos[RIGHT_DRAKE_END_FLY_POS].GetOrientation());
                });
            });
        }

        void SpellHit(Unit* /*caster*/, SpellEntry const* spell) override
        {
            if (spell->Id == SPELL_HARPOON)
            {
                me->SetSpeed(MOVE_FLIGHT, 1.0f);
                if (me->GetEntry() == NPC_TWILIGHT_ASSAULT_DRAKE_LEFT)
                    me->GetMotionMaster()->MovePoint(0, harpoonPos[RIGHT_HARPOON_POSITION], true, true);
                else
                    me->GetMotionMaster()->MovePoint(0, harpoonPos[LEFT_HARPOON_POSITION], true, true);
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (!harpoon)
            {
                if (harpoonTimer <= diff)
                {
                    if (Creature * pHarpoon = me->FindNearestCreature(NPC_HARPOON, 200.0f, true))
                        pHarpoon->CastSpell(me, SPELL_HARPOON, true);

                    uint32 releaseTimer = IsHeroic() ? 20 : 25;
                    scheduler.Schedule(Seconds(releaseTimer), [this](TaskContext harpoon)
                    {
                        if (harpoon.GetRepeatCounter() == 0)
                        {
                            if (Creature * pHarpoon = me->FindNearestCreature(NPC_HARPOON, 200.0f, true))
                            {
                                pHarpoon->InterruptNonMeleeSpells(true);
                                me->SetSpeed(MOVE_FLIGHT, 2.0f);
                                if (me->GetEntry() == NPC_TWILIGHT_ASSAULT_DRAKE_LEFT)
                                    me->GetMotionMaster()->MovePoint(LEFT_DRAKE_END_FLY_POS, assaultDrakePos[LEFT_DRAKE_END_FLY_POS], true);
                                else me->GetMotionMaster()->MovePoint(RIGHT_DRAKE_END_FLY_POS, assaultDrakePos[RIGHT_DRAKE_END_FLY_POS], true);
                            }
                            harpoon.Repeat(Seconds(3));
                        }
                        else
                        {
                            if (me->GetEntry() == NPC_TWILIGHT_ASSAULT_DRAKE_LEFT)
                                me->SetFacingTo(assaultDrakePos[LEFT_DRAKE_END_FLY_POS].GetOrientation());
                            else
                                me->SetFacingTo(assaultDrakePos[RIGHT_DRAKE_END_FLY_POS].GetOrientation());
                        }
                    });
                    harpoon = true;
                }
                else harpoonTimer -= diff;
            }

            if (twilightBarrageTimer <= diff)
            {
                uint32 randPos = urand(0, 24);
                me->CastSpell(warmasterDamagePos[randPos].GetPositionX(), warmasterDamagePos[randPos].GetPositionY(), warmasterDamagePos[randPos].GetPositionZ(), SPELL_TWILIGHT_BARRAGE, false);
                twilightBarrageTimer = 8000;
            }
            else twilightBarrageTimer -= diff;

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

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

        void SpellHit(Unit* /*caster*/, SpellEntry const* spell) override
        {
            if (spell->Id == 99148)
            {
                me->CastSpell(me, SPELL_ENGINE_FIRE, false);
            }
        }
    };
};

class npc_ds_twilight_elite_dreadblade : public CreatureScript
{
public:
    npc_ds_twilight_elite_dreadblade() : CreatureScript("npc_ds_twilight_elite_dreadblade") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_twilight_elite_dreadbladeAI(pCreature);
    }

    struct npc_ds_twilight_elite_dreadbladeAI : public ScriptedAI
    {
        npc_ds_twilight_elite_dreadbladeAI(Creature *creature) : ScriptedAI(creature) { }

        uint32 bladeRushTimer;
        uint32 degenerationTimer;

        void Reset() override
        {
            bladeRushTimer = 10000;
            degenerationTimer = 15000;
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (bladeRushTimer <= diff)
            {

                bladeRushTimer = 10000;
            }
            else bladeRushTimer -= diff;

            if (degenerationTimer <= diff)
            {
                me->CastSpell(me->GetVictim(), SPELL_DEGENERATION, false);
                degenerationTimer = 10000;
            }
            else degenerationTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_ds_twilight_elite_slayer : public CreatureScript
{
public:
    npc_ds_twilight_elite_slayer() : CreatureScript("npc_ds_twilight_elite_slayer") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_twilight_elite_slayerAI(pCreature);
    }

    struct npc_ds_twilight_elite_slayerAI : public ScriptedAI
    {
        npc_ds_twilight_elite_slayerAI(Creature *creature) : ScriptedAI(creature) { }

        uint32 bladeRushTimer;
        uint32 brutalStrikeTimer;

        void Reset() override
        {
            bladeRushTimer = 10000;
            brutalStrikeTimer = 15000;
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (bladeRushTimer <= diff)
            {

                bladeRushTimer = 10000;
            }
            else bladeRushTimer -= diff;

            if (brutalStrikeTimer <= diff)
            {
                me->CastSpell(me->GetVictim(), SPELL_BRUTAL_STRIKE, false);
                brutalStrikeTimer = 10000;
            }
            else brutalStrikeTimer -= diff;

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
        npc_ds_twilight_sapperAI(Creature *creature) : ScriptedAI(creature) { }

        TaskScheduler scheduler;
        bool explosion;

        void Reset() override
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->GetMotionMaster()->MoveFall();

            scheduler.Schedule(Seconds(2), [this](TaskContext /*task context*/)
            {
                me->CastSpell(me, SPELL_SMOKE_BOMB, false);
                me->CastSpell(me, SPELL_EVADE, false);
                me->GetMotionMaster()->MovePoint(0, sapperEndPos, true, true);
            });
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (!explosion)
            {
                if (me->GetDistance2d(sapperEndPos.GetPositionX(), sapperEndPos.GetPositionY()) <= 2.0f)
                {
                    if (Creature* pShip = me->FindNearestCreature(NPC_SKYFIRE, 300.0f))
                        pShip->SetHealth(pShip->GetHealth() - pShip->GetMaxHealth()*0.2);
                    me->CastSpell(me, SPELL_DETONATE, false);
                    explosion = true;
                }
            }
        }
    };
};

class npc_ds_twilight_flames : public CreatureScript
{
public:
    npc_ds_twilight_flames() : CreatureScript("npc_ds_twilight_flames") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_twilight_flamesAI(pCreature);
    }

    struct npc_ds_twilight_flamesAI : public ScriptedAI
    {
        npc_ds_twilight_flamesAI(Creature *creature) : ScriptedAI(creature) { }

        TaskScheduler scheduler;

        void Reset() override
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetInCombatWithZone();

            scheduler.Schedule(Seconds(2), [this](TaskContext /*task context*/)
            {
                me->CastSpell(me, SPELL_TWILIGHT_FLAMES_AURA, false);
            });
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
        npc_ds_skyfire_deckAI(Creature *creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetMaxHealth(RAID_MODE(4000000, 10000000, 6000000, 15000000));
            me->SetHealth(RAID_MODE(4000000, 10000000, 6000000, 15000000));
        }

        void DoAction(const int32 action) override
        {
            if (action == ACTION_START_WARMASTER_ENCOUNTER)
                me->SetInCombatWithZone();
        }

        void UpdateAI(const uint32 diff) override {}
    };
};

///////////////////////////
///     Spine stuff     ///
///////////////////////////

enum NPC
{
    NPC_SPINE_OF_DEATHWING          = 53879,
    NPC_HIDEOUS_AMALGAMATION        = 53890,
    NPC_CORRUPTION                  = 53891,
    NPC_CORRUPTION_2                = 56161, // ?
    NPC_CORRUPTION_3                = 56162, // ?
    NPC_SPAWNER                     = 53888,
    NPC_CORRUPTED_BLOOD             = 53889,
    NPC_BURNING_TENDONS_LEFT        = 56341, // left
    NPC_BURNING_TENDONS_RIGHT       = 56575, // right
};

enum Spells
{
    // Corruption
    SPELL_FIERY_GRIP                = 105490,
    SPELL_FIERY_GRIP_25             = 109457,
    SPELL_FIERY_GRIP_10H            = 109458,
    SPELL_FIERY_GRIP_25H            = 109459,
    SPELL_SEARING_PLASMA_AOE        = 109379,
    SPELL_SEARING_PLASMA            = 105479,

    // Hideous Amalgamation
    SPELL_ZERO_REGEN                = 109121,
    SPELL_ABSORB_BLOOD_BAR          = 109329,
    SPELL_DEGRADATION               = 106005,
    SPELL_NUCLEAR_BLAST             = 105845,
    SPELL_NUCLEAR_BLAST_SCRIPT      = 105846,
    SPELL_SUPERHEATED_NUCLEUS       = 105834,
    SPELL_SUPERHEATED_NUCLEUS_DMG   = 106264,
    SPELL_ABSORB_BLOOD              = 105244,
    SPELL_ABSORB_BLOOD_DUMMY        = 105241, // target on 105223
    SPELL_ABSORBED_BLOOD            = 105248,

    // Spawner
    SPELL_GRASPING_TENDRILS         = 105510,
    SPELL_GRASPING_TENDRILS_10      = 105563,
    SPELL_GRASPING_TENDRILS_25      = 109454,
    SPELL_GRASPING_TENDRILS_10H     = 109455,
    SPELL_GRASPING_TENDRILS_25H     = 109456,

    // Corrupted Blood
    SPELL_RESIDUE                   = 105223,
    SPELL_BURST                     = 105219,

    // Burning Tendons
    SPELL_SEAL_ARMOR_BREACH_LEFT    = 105847, // 1698
    SPELL_SEAL_ARMOR_BREACH_RIGHT   = 105848, // 1699
    SPELL_BREACH_ARMOR_LEFT         = 105363, // 1677
    SPELL_BREACH_ARMOR_RIGHT        = 105385, // 1681
    SPELL_PLATE_FLY_OFF_LEFT        = 105366, // 1678
    SPELL_PLATE_FLY_OFF_RIGHT       = 105384, // 1680
    SPELL_SLOW                      = 110907,

    SPELL_BLOOD_CORRUPTION_DEATH    = 106199,
    SPELL_BLOOD_CORRUPTION_EARTH    = 106200,
    SPELL_BLOOD_OF_DEATHWING        = 106201,
    SPELL_BLOOD_OF_NELTHARION       = 106213,
};

enum SpineActions
{
    ACTION_CORRUPTED_DIED           = 0,
    ACTION_CORRUPTED_POSITION       = 1,
    ACTION_ABSORB_BLOOD             = 2,
    ACTION_TENDONS_POSITION         = 3,
    ACTION_OPEN_PLATE               = 4,
    ACTION_CLOSE_PLATE              = 5,
};

class npc_ds_spine_corruption : public CreatureScript
{
public:
    npc_ds_spine_corruption() : CreatureScript("npc_ds_spine_corruption") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_spine_corruptionAI(pCreature);
    }

    struct npc_ds_spine_corruptionAI : public ScriptedAI
    {
        npc_ds_spine_corruptionAI(Creature *creature) : ScriptedAI(creature) { }

        uint32 searingPlasmaTimer;
        uint32 fieryGripTimer;
        uint32 damageCounter;
        uint32 corruptedPosition;
        bool isGrip;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_AGGRESSIVE);

            isGrip = false;
            damageCounter = 0;
            corruptedPosition = 0;
            searingPlasmaTimer = urand(10000, 18000);
            fieryGripTimer = urand(10000, 25000);
        }

        void JustDied(Unit * /*who*/) override
        {
            if (Creature * pSpineOfDeathwing = me->FindNearestCreature(NPC_SPINE_OF_DEATHWING, 300.0f, true))
                pSpineOfDeathwing->AI()->SetData(ACTION_CORRUPTED_POSITION, corruptedPosition);
        }

        void SetData(uint32 type, uint32 data)
        {
            if (type == ACTION_CORRUPTED_POSITION)
                corruptedPosition = data;
        }

        void DamageTaken(Unit* /*who*/, uint32 &damage)
        {
            if (!isGrip)
                return;

            if (damageCounter <= damage)
            {
                damageCounter = 0;
                isGrip = false;
                fieryGripTimer = urand(30000, 35000);
                me->InterruptSpell(CURRENT_CHANNELED_SPELL);
            }
            else
                damageCounter -= damage;
        }

        void UpdateAI(const uint32 diff) override 
        {
            if (!UpdateVictim())
                return;

            if (searingPlasmaTimer <= diff)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    me->CastSpell(pTarget, SPELL_SEARING_PLASMA, false);
                searingPlasmaTimer = 8000;
            }
            else searingPlasmaTimer -= diff;

            if (fieryGripTimer <= diff)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true, -SPELL_FIERY_GRIP))
                {
                    me->CastCustomSpell(SPELL_FIERY_GRIP, SPELLVALUE_MAX_TARGETS, 1, pTarget, false);
                    damageCounter = me->CountPctFromMaxHealth(20);
                    isGrip = true;
                }
                fieryGripTimer = urand(30000, 35000);
            }
            else fieryGripTimer -= diff;
        }
    };
};

class npc_ds_spine_burning_tendons : public CreatureScript
{
public:
    npc_ds_spine_burning_tendons() : CreatureScript("npc_ds_spine_burning_tendons") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_spine_burning_tendonsAI(pCreature);
    }

    struct npc_ds_spine_burning_tendonsAI : public ScriptedAI
    {
        npc_ds_spine_burning_tendonsAI(Creature *creature) : ScriptedAI(creature) 
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 openTimer;
        uint32 tendonsPosition;
        bool isOpen;

        void Reset() override
        {
            me->SetVisible(false);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

            tendonsPosition = 0;
            openTimer = 0;
            isOpen = false;
        }

        void JustDied(Unit * /*who*/) override
        {
            if (me->GetEntry() == NPC_BURNING_TENDONS_LEFT)
            {
                if (Creature * pRightTendons = me->FindNearestCreature(NPC_BURNING_TENDONS_RIGHT, 100.0f, true))
                    pRightTendons->DespawnOrUnsummon();
            }
            else if (me->GetEntry() == NPC_BURNING_TENDONS_RIGHT)
            {
                if (Creature * pLeftTendons = me->FindNearestCreature(NPC_BURNING_TENDONS_LEFT, 100.0f, true))
                    pLeftTendons->DespawnOrUnsummon();
            }

            // Here is the part when Deathwing Plate should fly off - but whis spell doesn`t work
            me->CastSpell(me, ((tendonsPosition % 2) == 1) ? SPELL_PLATE_FLY_OFF_RIGHT : SPELL_PLATE_FLY_OFF_LEFT, false);

            if (instance)
                instance->SetData(DATA_SPINE_OF_DEATHWING_PLATES, 0);
        }

        void DoAction(const int32 action) override
        {
            switch (action)
            {
            case ACTION_OPEN_PLATE:
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetVisible(true);
                // Here plate should open - but whis spell doesn`t work
                me->CastSpell(me, ((tendonsPosition % 2) == 1) ? SPELL_BREACH_ARMOR_LEFT : SPELL_BREACH_ARMOR_RIGHT, false);
                openTimer = 23000;
                isOpen = true;
                break;
            case ACTION_CLOSE_PLATE:
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                // Here plate should seal again - but whis spell doesn`t work
                me->CastSpell(me, ((tendonsPosition % 2) == 1) ? SPELL_SEAL_ARMOR_BREACH_LEFT : SPELL_SEAL_ARMOR_BREACH_RIGHT, false);
                me->SetVisible(false);
                break;
            default:
                break;
            }
        }

        void SetData(uint32 type, uint32 data)
        {
            if (type == ACTION_TENDONS_POSITION)
                tendonsPosition = data;
        }

        void UpdateAI(const uint32 diff) override
        {
            if (isOpen)
            {
                if (openTimer <= diff)
                {
                    me->AI()->DoAction(ACTION_CLOSE_PLATE);
                    isOpen = false;
                }
                else openTimer -= diff;
            }
        }
    };
};

class npc_ds_spine_corrupted_blood : public CreatureScript
{
public:
    npc_ds_spine_corrupted_blood() : CreatureScript("npc_ds_spine_corrupted_blood") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_spine_corrupted_bloodAI(pCreature);
    }

    struct npc_ds_spine_corrupted_bloodAI : public ScriptedAI
    {
        npc_ds_spine_corrupted_bloodAI(Creature *creature) : ScriptedAI(creature) { }

        uint32 searingPlasmaTimer;
        uint32 fieryGripTimer;
        uint32 moveBackTimer;
        bool isDead;

        void Reset() override
        {
            me->SetWalk(true);
            me->SetSpeed(MOVE_WALK, 1.0f, true);
            isDead = false;
            moveBackTimer = 0;
            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
        }

        void JustReachedHome() override
        {
            isDead = false;
            DoResetThreat();
            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveAura(SPELL_RESIDUE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
        }

        void DamageTaken(Unit* /*who*/, uint32 & damage)
        {
            if (me->GetHealth() < damage)
            {
                damage = 0;
                if (!isDead)
                {
                    moveBackTimer = 5000;
                    isDead = true;
                    me->SetReactState(REACT_PASSIVE);
                    me->AttackStop();
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                    me->CastSpell(me, SPELL_BURST, true);
                    me->CastSpell(me, SPELL_RESIDUE, true);
                    me->SetWalk(true);
                    me->SetSpeed(MOVE_WALK, 0.1f, true);

                    std::list<Creature*> hideous_amalgamation;
                    GetCreatureListWithEntryInGrid(hideous_amalgamation, me, NPC_HIDEOUS_AMALGAMATION, 200.0f);
                    for (std::list<Creature*>::const_iterator itr = hideous_amalgamation.begin(); itr != hideous_amalgamation.end(); ++itr)
                    {
                        if (*itr)
                            (*itr)->AI()->DoAction(ACTION_ABSORB_BLOOD);
                    }
                }
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (isDead)
            {
                if (moveBackTimer <= diff)
                {
                    me->GetMotionMaster()->MoveTargetedHome();
                    isDead = false;
                }
                else moveBackTimer -= diff;
            }

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_ds_spine_hideous_amalgamation : public CreatureScript
{
public:
    npc_ds_spine_hideous_amalgamation() : CreatureScript("npc_ds_spine_hideous_amalgamation") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_spine_hideous_amalgamationAI(pCreature);
    }

    struct npc_ds_spine_hideous_amalgamationAI : public ScriptedAI
    {
        npc_ds_spine_hideous_amalgamationAI(Creature *creature) : ScriptedAI(creature) { }

        TaskScheduler scheduler;
        uint32 searingPlasmaTimer;
        uint32 fieryGripTimer;
        bool isExplode;
        int absorbedBloodCount = 0;

        void Reset() override
        {
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            isExplode = false;
        }

        void DoAction(const int32 action)
        {
            if (action == ACTION_ABSORB_BLOOD)
            {
                me->CastSpell(me, SPELL_ABSORBED_BLOOD, true);
                // the spell SPELL_ABSORBED_BLOOD has a travel time - can't look at its aura
                absorbedBloodCount++;
                if (absorbedBloodCount == 9)
                    me->CastSpell(me, SPELL_SUPERHEATED_NUCLEUS, true);
            }
        }

        void JustDied(Unit * /*who*/) override
        {
            if (IsHeroic())
            {
                Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                if (!PlList.isEmpty())
                {
                    for (Map::PlayerList::const_iterator itr = PlList.begin(); itr != PlList.end(); ++itr)
                    {
                        if (Player* pPlayer = itr->getSource())
                            pPlayer->AddAura(SPELL_DEGRADATION, pPlayer);
                    }
                }
            }
        }

        void DamageTaken(Unit* who, uint32& damage)
        {
            if (me->GetHealth() <= damage)
            {
                if (Aura * aur = me->GetAura(SPELL_SUPERHEATED_NUCLEUS))
                {
                    damage = 0;
                    if (!isExplode)
                    {
                        isExplode = true;
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        me->CastSpell(me, SPELL_NUCLEAR_BLAST, false);
                        scheduler.Schedule(Seconds(6), [this](TaskContext /* Task context */)
                        {
                            Creature * pBurningTendonsLeft = me->FindNearestCreature(NPC_BURNING_TENDONS_LEFT, 100.0f, true);
                            Creature * pBurningTendonsRight = me->FindNearestCreature(NPC_BURNING_TENDONS_RIGHT, 100.0f, true);
                            if (pBurningTendonsLeft && pBurningTendonsRight)
                            {
                                if (me->GetDistance2d(pBurningTendonsLeft->GetPositionX(), pBurningTendonsLeft->GetPositionY()) <=
                                    me->GetDistance2d(pBurningTendonsRight->GetPositionX(), pBurningTendonsRight->GetPositionY()))
                                {
                                    pBurningTendonsLeft->AI()->DoAction(ACTION_OPEN_PLATE);
                                }
                                else
                                    pBurningTendonsRight->AI()->DoAction(ACTION_OPEN_PLATE);
                            }
                            me->Kill(me);
                        });
                    }
                }
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_ds_spine_spawner : public CreatureScript
{
public:
    npc_ds_spine_spawner() : CreatureScript("npc_ds_spine_spawner") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_spine_spawnerAI(pCreature);
    }

    struct npc_ds_spine_spawnerAI : public ScriptedAI
    {
        npc_ds_spine_spawnerAI(Creature *creature) : ScriptedAI(creature) { }

        uint32 checkTimer;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            checkTimer = 1000;
        }

        void UpdateAI(const uint32 diff) override 
        {
            if (checkTimer <= diff)
            {
                if (me->FindNearestCreature(NPC_CORRUPTION, 5.0f, true))
                    me->RemoveAura(SPELL_GRASPING_TENDRILS);
                else
                    me->CastSpell(me, SPELL_GRASPING_TENDRILS, false);
                checkTimer = 2500;
            }
            else checkTimer -= diff;
        }
    };
};

///////////////////////////
///    Madness stuff    ///
///////////////////////////

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
                    pPlayer->NearTeleportTo(telePos[6].GetPositionX(), telePos[6].GetPositionY(), telePos[6].GetPositionZ(), telePos[6].GetOrientation());
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
            if (pSummoner)
            {
                summonerGuid = pSummoner->GetGUID();
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
                        if (Player* pPlayer = ObjectAccessor::FindPlayer(summonerGuid))
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
                            if (Player* pPlayer = ObjectAccessor::FindPlayer(summonerGuid))
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
                        if (summonDrakes.GetRepeatCounter() < 15)
                            summonDrakes.Repeat(Seconds(10));
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

enum SkyfireNpc
{
    NPC_BOSS_BLACKHORN                  = 56427,
    NPC_GORIONA                         = 56781,
};

enum gunshipActions
{
    ACTION_READY_TO_FOLLOW_DEATHWING = 0,
    ACTION_START_BLACKHORN_ENCOUNTER = 1,
    ACTION_SPINE_OF_DEATHWING        = 2,
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

        if (pCreature->GetInstanceScript()->GetData(TYPE_BOSS_BLACKHORN) == DONE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SPINE_OF_DEATHWING, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        }
        else if (pCreature->FindNearestCreature(NPC_BOSS_BLACKHORN, 200.0f, true))
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
                    scheduler.Schedule(Seconds(0), [this](TaskContext greetings)
                    {
                        uint32 repeatCount = greetings.GetRepeatCounter();
                        if (repeatCount == 0)
                        {
                            if (Creature * pGoriona = me->FindNearestCreature(NPC_GORIONA, 300.0f, true))
                                pGoriona->AI()->DoAction(ACTION_START_ENCOUNTER);
                            me->MonsterYell("All ahead full. Everything depends on our speed! We can't let the Destroyer get away.", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(26292, false);
                            greetings.Repeat(Seconds(8));
                        }
                        else if (repeatCount == 1)
                        {
                            instance->SetData(DATA_START_BLACKHORN_ENCOUNTER, 0);
                            greetings.Repeat(Seconds(15));
                        }
                        else if (repeatCount == 2)
                        {
                            me->MonsterYell("Our engines are damaged! We're sitting ducks up here!", LANG_UNIVERSAL, 0);
                            me->SendPlaySound(26303, false);
                            greetings.Repeat(Seconds(6));

                        }
                        else if (repeatCount == 3)
                        {
                            if (Creature * pWarmasterBlackhorn = me->FindNearestCreature(NPC_BOSS_BLACKHORN, 300.0f, true))
                            {
                                pWarmasterBlackhorn->MonsterYell("You won't get near the Master. Dragonriders, attack!", LANG_UNIVERSAL, 0);
                                pWarmasterBlackhorn->SendPlaySound(26210, false);
                            }
                            greetings.Repeat(Seconds(8));
                        }
                    });
                }
                else if (action == ACTION_SPINE_OF_DEATHWING)
                {
                    instance->SetData(DATA_PREPARE_SPINE_ENCOUNTER, 0);
                    PlayMovieToPlayers(74);
                    scheduler.Schedule(Seconds(19), [this](TaskContext /*Show Movie and teleport to Spine of Deathwing*/)
                    {
                        TeleportAllPlayers(ACTION_SPINE_OF_DEATHWING, true);
                    });
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
    new npc_ds_twilight_assault_drake();
    new npc_ds_twilight_elite_dreadblade();
    new npc_ds_twilight_elite_slayer();
    new npc_ds_twilight_flames();
    new npc_ds_engine_stalker();
    new npc_ds_skyfire_deck();
    new npc_ds_spine_corruption();
    new npc_ds_spine_corrupted_blood();
    new npc_ds_spine_hideous_amalgamation();
    new npc_ds_spine_burning_tendons();
    new npc_ds_spine_spawner();
    new npc_ds_instance_teleporter();
    new npc_ds_travel_with_drakes();
    new go_ds_instance_teleporter();
    new npc_ds_alliance_ship_crew();
    new npc_ds_deathwing_wyrmrest_temple();
}