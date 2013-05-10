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
#include "firelands.h"

enum NPCs
{
    NPC_LORD_RHYOLITH = 52558,
    NPC_LEFT_FOOT     = 52577,
    NPC_RIGHT_FOOT    = 53087,

    NPC_VOLCANO       = 52582,
    NPC_CRATER        = 52866,
    NPC_LAVA_FLOW     = 950000, // custom NPC

    NPC_SPARK         = 53211,
    NPC_FRAGMENT      = 52620,
};

static const uint32 npcListUnsummonAtReset[] = {NPC_VOLCANO, NPC_CRATER, NPC_SPARK, NPC_FRAGMENT, NPC_LAVA_FLOW};

static const Position platformCenter = {-374.337006f, -318.489990f, 100.413002f, 0.0f};

enum Spells
{
    SPELL_BALANCE_BAR       = 98226, // power bar for power 10

    // phase one
    SPELL_OBSIDIAN_ARMOR    = 98632,
    SPELL_CONCUSSIVE_STOMP  = 97282,
    SPELL_ACTIVATE_VOLCANO  = 98493, // spell Heated Volcano
    SPELL_MOLTEN_ARMOR      = 98255,
    SPELL_SUMMON_SPARK      = 98553,
    SPELL_SUMMON_FRAGMENT   = 98135,
    SPELL_DRINK_MAGMA       = 98034, // dummy cast
    SPELL_MAGMA_SPIT        = 78359,

    // phase two
    SPELL_IMMOLATION        = 99846,

    SPELL_SUPERHEATED       = 101304, // enrage spell

    // volcanoes
    SPELL_VOLCANIC_BIRTH    = 98010, // spawn at nearby location
    SPELL_ERUPTION          = 98264, // the fire coming out of a volcano top // at unapply it explodes
    SPELL_LAVA_STRIKE       = 98491, // trigger damaging missile
    SPELL_VOLCANIC_WRATH    = 93354, // missile of lava
    SPELL_VOLCANO_SMOKE     = 97699,
    SPELL_LAVA_TUBE         = 98265, // creates a tube, after stepping on active volcano

    // lava from volcano
    SPELL_MAGMA_FLOW_GROUND = 97225,
    SPELL_MAGMA_FLOW_BOOM   = 97230,

    // boss spawns abilities
    SPELL_SPARK_IMMOLATION  = 98597,
    SPELL_SPARK_INFERNAL_RAGE = 98596,
    SPELL_FRAGMENT_MELTDOWN = 98646,
};

enum DisplayIDs
{
    DISPLAYID_NORMAL         = 38414, // more than 25% HP
    DISPLAYID_SHATTERED      = 38594, // less than 25% HP

    DISPLAYID_VOLCANO_STILL  = 38054,
    DISPLAYID_VOLCANO_ACTIVE = 38848,
};

/*

TODOs (Gregory):

- spell 98276 - chance to trigger is 0, increase to 100
- limit targets of spell 98276 and make it trigger spell 98491 (stored in base points)

*/

class boss_rhyolith: public CreatureScript
{
public:
    boss_rhyolith(): CreatureScript("boss_lord_rhyolith") {}

    struct boss_rhyolithAI: public ScriptedAI
    {
        boss_rhyolithAI(Creature* c): ScriptedAI(c)
        {
            pInstance = c->GetInstanceScript();
            leftFootGUID = 0;
            rightFootGUID = 0;

            Creature* pFoot = me->SummonCreature(NPC_LEFT_FOOT, 0,0,0);
            if (pFoot)
            {
                leftFootGUID = pFoot->GetGUID();
                pFoot->SetMaxHealth(me->GetMaxHealth() / 2);
                pFoot->EnterVehicle(me, -1, false);
                pFoot->clearUnitState(UNIT_STAT_UNATTACKABLE);

                pFoot->CastSpell(pFoot, SPELL_OBSIDIAN_ARMOR, true);
                if (Aura* armor = pFoot->GetAura(SPELL_OBSIDIAN_ARMOR))
                    armor->SetStackAmount(80);
            }

            pFoot = me->SummonCreature(NPC_RIGHT_FOOT, 0,0,0);
            if (pFoot)
            {
                rightFootGUID = pFoot->GetGUID();
                pFoot->SetMaxHealth(me->GetMaxHealth() / 2);
                pFoot->EnterVehicle(me, -1, false);
                pFoot->clearUnitState(UNIT_STAT_UNATTACKABLE);

                pFoot->CastSpell(pFoot, SPELL_OBSIDIAN_ARMOR, true);
                if (Aura* armor = pFoot->GetAura(SPELL_OBSIDIAN_ARMOR))
                    armor->SetStackAmount(80);
            }

            Reset();
        }

        InstanceScript *pInstance;
        uint64 leftFootGUID, rightFootGUID;

        uint32 leftDamage, rightDamage, meDamage;
        uint32 directionPower;
        float direction;
        uint32 directionUpdateTimer;

        uint8 phase;
        uint32 concussiveStompTimer;
        uint32 activateVolcanoTimer;
        uint32 summonTimer;
        uint32 lavaCheckTimer;
        uint8 magmaDrinkCount;
        uint32 magmaDrinkTimer;

        uint32 enrageTimer;

        void Reset()
        {
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_THREAT, true);

            me->SetDisplayId(DISPLAYID_NORMAL); // "dressed up"

            leftDamage  = 0;
            rightDamage = 0;
            meDamage    = 0;
            direction   = 0.0f;
            directionPower = 50;
            directionUpdateTimer = 1000;

            concussiveStompTimer = 3000;
            activateVolcanoTimer = 8000;
            summonTimer          = 15000;
            lavaCheckTimer       = 1000;
            magmaDrinkCount      = 0;
            magmaDrinkTimer      = 0;

            uint32 mode = pInstance->instance->GetDifficulty();
            if (mode == RAID_DIFFICULTY_10MAN_HEROIC || mode == RAID_DIFFICULTY_25MAN_HEROIC)
                enrageTimer = 300000;
            else
                enrageTimer = 360000;

            std::list<Creature*> crList;
            for (uint32 i = 0; i < sizeof(npcListUnsummonAtReset)/sizeof(uint32); i++)
            {
                GetCreatureListWithEntryInGrid(crList, me, npcListUnsummonAtReset[i], 200.0f);
                for (std::list<Creature*>::iterator itr = crList.begin(); itr != crList.end(); ++itr)
                    (*itr)->ForcedDespawn();
            }

            phase = 1;
        }

        void FootDamaged(uint64 guid, uint32 damage)
        {
            if (guid == leftFootGUID)
                leftDamage  += damage;
            else if (guid == rightFootGUID)
                rightDamage += damage;
            else
                return;

            if (Unit* foot = Unit::GetUnit(*me, guid))
                foot->DealDamage(me, damage);
        }

        void DamageTaken(Unit* pWho, uint32 &damage)
        {
            if (pWho->GetGUID() != me->GetGUID() && pWho->GetGUID() != leftFootGUID && pWho->GetGUID() != rightFootGUID)
            {
                Unit* foot = Unit::GetUnit(*me, leftFootGUID);
                if (foot)
                    me->DealDamage(foot, damage/2);

                foot = Unit::GetUnit(*me, rightFootGUID);
                if (foot)
                    me->DealDamage(foot, damage/2);

                meDamage += damage;
            }
        }

        void EnterCombat(Unit* pWho)
        {
            me->CastSpell(me, SPELL_BALANCE_BAR, true);
            if (pInstance)
            {
                pInstance->DoSetMaxScriptedPowerToPlayers(100);
                pInstance->DoSetScriptedPowerToPlayers(50);
                directionPower = 50;
            }

            phase = 1;

            concussiveStompTimer = 5000;
            activateVolcanoTimer = 10000;
            summonTimer          = 15000;
            lavaCheckTimer       = 1000;
            magmaDrinkCount      = 0;
            magmaDrinkTimer      = 0;

            Unit* foot = Unit::GetUnit(*me, leftFootGUID);
            if (foot)
                foot->SetVisibility(VISIBILITY_ON);

            foot = Unit::GetUnit(*me, rightFootGUID);
            if (foot)
                foot->SetVisibility(VISIBILITY_ON);

            uint32 mode = pInstance->instance->GetDifficulty();
            if (mode == RAID_DIFFICULTY_10MAN_HEROIC || mode == RAID_DIFFICULTY_25MAN_HEROIC)
                enrageTimer = 300000;
            else
                enrageTimer = 360000;

            me->CastSpell(me, SPELL_OBSIDIAN_ARMOR, true);
            if (Aura* armor = me->GetAura(SPELL_OBSIDIAN_ARMOR))
                armor->SetStackAmount(80);
        }

        void EnterEvadeMode()
        {
            me->RemoveAurasDueToSpell(SPELL_BALANCE_BAR);
            if (pInstance)
                pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BALANCE_BAR);

            ScriptedAI::EnterEvadeMode();
        }

        void RefreshPowerBar(uint32 now, bool removal)
        {
            if (removal)
            {
                me->RemoveAurasDueToSpell(SPELL_BALANCE_BAR);
                if (pInstance)
                    pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BALANCE_BAR);

                me->CastSpell(me, SPELL_BALANCE_BAR, true);
            }

            if (pInstance)
            {
                uint32 delta = (uint32)((int32)now - (int32)directionPower);

                if (delta != 0)
                {
                    /* Client needs to get energize packet to proper power amount shown in power bar */
                    Map::PlayerList const& plList = pInstance->instance->GetPlayers();
                    for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
                        (*itr).getSource()->SendEnergizeSpellLog((*itr).getSource(), SPELL_BALANCE_BAR, delta, POWER_SCRIPTED);

                    pInstance->DoSetScriptedPowerToPlayers(now);

                    directionPower = now;
                }
            }
        }

        void ModMoltenArmorStack(int32 amount)
        {
            Aura* pArmor = me->GetAura(SPELL_MOLTEN_ARMOR);
            if (!pArmor)
            {
                if (amount <= 0)
                    return;

                me->AddAura(SPELL_MOLTEN_ARMOR, me);
                amount -= 1; // 1 stack is applied on aura apply by default

                pArmor = me->GetAura(SPELL_MOLTEN_ARMOR);
                if (!pArmor)
                    return;
            }

            pArmor->ModStackAmount(amount);
        }

        void ModObsidianArmorStack(int32 amount)
        {
            Aura* pArmor = me->GetAura(SPELL_OBSIDIAN_ARMOR);
            if (pArmor)
                pArmor->ModStackAmount(amount);

            Creature* pFoot = Creature::GetCreature(*me, leftFootGUID);
            if (pFoot)
            {
                pArmor = pFoot->GetAura(SPELL_OBSIDIAN_ARMOR);
                if (pArmor)
                    pArmor->ModStackAmount(amount);
            }

            pFoot = Creature::GetCreature(*me, rightFootGUID);
            if (pFoot)
            {
                pArmor = pFoot->GetAura(SPELL_OBSIDIAN_ARMOR);
                if (pArmor)
                    pArmor->ModStackAmount(amount);
            }
        }

        void SpellHit(Unit* pSrc, const SpellEntry* spell)
        {
            if (spell->Id == SPELL_CONCUSSIVE_STOMP ||
                spell->Id == 100411 ||
                spell->Id == 100968 ||
                spell->Id == 100969)
            {
                ModMoltenArmorStack(-1);

                uint32 cnt = urand(2,3);
                for (uint32 i = 0; i < cnt; i++)
                    me->CastSpell(me, SPELL_VOLCANIC_BIRTH, true);
            }
            else if (spell->Id == SPELL_DRINK_MAGMA)
            {
                if (!pInstance)
                    return;

                Map::PlayerList const& plList = pInstance->instance->GetPlayers();
                for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
                    me->CastSpell((*itr).getSource(), SPELL_MAGMA_SPIT, true);

                magmaDrinkTimer = 1000;
                magmaDrinkCount = 3;
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            // phase switch at 25%
            if (me->GetHealthPct() < 25.0f && phase != 2)
            {
                me->RemoveAurasDueToSpell(SPELL_OBSIDIAN_ARMOR);
                me->SetDisplayId(DISPLAYID_SHATTERED);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, false);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_THREAT, false);
                Unit* foot = Unit::GetUnit(*me, leftFootGUID);
                if (foot)
                    foot->SetVisibility(VISIBILITY_OFF);

                foot = Unit::GetUnit(*me, rightFootGUID);
                if (foot)
                    foot->SetVisibility(VISIBILITY_OFF);
                phase = 2;
            }

            /* both phases */
            if (concussiveStompTimer <= diff)
            {
                me->CastSpell(me, SPELL_CONCUSSIVE_STOMP, false);
                concussiveStompTimer = 30000;
            }
            else
                concussiveStompTimer -= diff;

            if (phase == 1)
            {
                /* Direction stuff
                 * Update direction depending on which foot was damaged more
                 * if no foot was damaged, then converge to the middle (straight forward, energy 50/100)
                 */
                if (directionUpdateTimer <= diff)
                {
                    uint32 totaldmg = leftDamage+rightDamage+meDamage;
                    if (totaldmg != 0)
                    {
                        if (leftDamage > rightDamage)
                            direction += (-(((float)leftDamage)/((float)totaldmg)))/10.0f;
                        else
                            direction += ((((float)rightDamage)/((float)totaldmg)))/10.0f;
                    }
                    else
                    {
                        direction = 2.0f*direction/3.0f;
                        if (fabs(direction) <= 0.12f)
                            direction = 0.0f;
                    }

                    if (direction > 1.0f)
                        direction = 1.0f;
                    else if (direction < -1.0f)
                        direction = -1.0f;

                    leftDamage = 0;
                    rightDamage = 0;
                    meDamage = 0;

                    RefreshPowerBar(50+direction*50, false);

                    directionUpdateTimer = 2000;
                }
                else
                    directionUpdateTimer -= diff;

                /* Phase 1 spell stuff */

                /* ENABLE AT DEVELOPMENT END !!!!!! */
                /*if (summonTimer <= diff)
                {
                    // summon 1 spark OR 5 fragments, 50/50 chances
                    if (urand(1,4) > 2)
                    {
                        for (uint32 i = 0; i < 5; i++)
                            me->CastSpell(me, SPELL_SUMMON_FRAGMENT, true);
                    }
                    else
                        me->CastSpell(me, SPELL_SUMMON_SPARK, true);

                    ModMoltenArmorStack(-1);

                    summonTimer = urand(20000, 25000);
                }
                else
                    summonTimer -= diff;*/

                if (activateVolcanoTimer <= diff)
                {
                    std::list<Creature*> volcanoList;
                    GetCreatureListWithEntryInGrid(volcanoList, me, NPC_VOLCANO, 200.0f);
                    for (std::list<Creature*>::iterator itr = volcanoList.begin(); itr != volcanoList.end(); )
                    {
                        if ((*itr)->HasAura(SPELL_VOLCANO_SMOKE))
                            itr++;
                        else
                            itr = volcanoList.erase(itr);
                    }

                    if (volcanoList.size() > 0)
                    {
                        uint32 randpos = urand(0,volcanoList.size()-1);

                        std::list<Creature*>::iterator itr = volcanoList.begin();
                        std::advance(itr, randpos);
                        if (*itr)
                            me->CastSpell(*itr, SPELL_ACTIVATE_VOLCANO, true);
                    }

                    activateVolcanoTimer = 25000;
                }
                else
                    activateVolcanoTimer -= diff;

                if (lavaCheckTimer <= diff)
                {
                    lavaCheckTimer = 1000;

                    if (pInstance)
                    {
                        ZLiquidStatus res = pInstance->instance->getLiquidStatus(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), MAP_ALL_LIQUIDS);
                        if (res != 0)
                        {
                            me->CastSpell(me, SPELL_DRINK_MAGMA, true);
                            lavaCheckTimer = 6000;
                        }
                    }
                }
                else
                    lavaCheckTimer -= diff;

                if (magmaDrinkTimer)
                {
                    if (magmaDrinkTimer <= diff)
                    {
                        Map::PlayerList const& plList = pInstance->instance->GetPlayers();
                        for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
                            me->CastSpell((*itr).getSource(), SPELL_MAGMA_SPIT, true);
                        magmaDrinkTimer = 1000;
                        magmaDrinkCount--;

                        if (magmaDrinkCount == 0)
                            magmaDrinkTimer = 0;
                    }
                    else
                        magmaDrinkTimer -= diff;
                }
            }
            else if (phase == 2)
            {
                /* */
                DoMeleeAttackIfReady();
            }

            if (enrageTimer <= diff)
            {
                me->CastSpell(me, SPELL_SUPERHEATED, true);
                enrageTimer = 10000;
            }
            else
                enrageTimer -= diff;
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new boss_rhyolithAI(c);
    }
};

class npc_rhyolith_feet: public CreatureScript
{
public:
    npc_rhyolith_feet(): CreatureScript("npc_rhyolith_feet") {}

    struct npc_rhyolith_feetAI: public ScriptedAI
    {
        npc_rhyolith_feetAI(Creature* c): ScriptedAI(c)
        {
        }

        void DamageTaken(Unit* attacker, uint32& damage)
        {
            if (Unit* pBoss = me->GetVehicleBase())
            {
                if (attacker->GetGUID() != pBoss->GetGUID())
                    if (boss_rhyolith::boss_rhyolithAI* pAI = (boss_rhyolith::boss_rhyolithAI*)(pBoss->GetAI()))
                        pAI->FootDamaged(me->GetGUID(), damage);
            }

            if (damage > me->GetHealth())
                damage = 0;
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            //
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new npc_rhyolith_feetAI(c);
    }
};

class npc_rhyolith_volcano: public CreatureScript
{
public:
    npc_rhyolith_volcano(): CreatureScript("npc_rhyolith_volcano") {}

    struct npc_rhyolith_volcanoAI: public ScriptedAI
    {
        npc_rhyolith_volcanoAI(Creature* c): ScriptedAI(c)
        {
            pInstance = me->GetInstanceScript();
            Reset();
        }

        uint32 bossRangeCheckTimer;
        uint32 eruptTimer;
        uint32 deactivateTimer;
        bool destroyed;
        InstanceScript *pInstance;

        void Reset()
        {
            me->CastSpell(me, SPELL_VOLCANO_SMOKE, true);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

            bossRangeCheckTimer = 1000;
            eruptTimer = 0;
            deactivateTimer = 0;
            destroyed = false;
        }

        void SpellHit(Unit* pAttacker, const SpellEntry* spell)
        {
            if (spell->Id == SPELL_ACTIVATE_VOLCANO)
            {
                me->RemoveAurasDueToSpell(SPELL_VOLCANO_SMOKE);
                me->SetDisplayId(DISPLAYID_VOLCANO_ACTIVE);
                me->CastSpell(me, SPELL_ERUPTION, true);
                eruptTimer = 2000;
                deactivateTimer = 21000;
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (destroyed)
                return;

            if (deactivateTimer)
            {
                if (deactivateTimer <= diff)
                {
                    me->RemoveAurasDueToSpell(SPELL_ERUPTION);
                    me->SetDisplayId(DISPLAYID_VOLCANO_STILL);
                    me->CastSpell(me, SPELL_VOLCANO_SMOKE, true);
                    eruptTimer = 0;
                    deactivateTimer = 0;
                }
                else
                    deactivateTimer -= diff;
            }

            if (eruptTimer && pInstance)
            {
                if (eruptTimer <= diff)
                {
                    std::list<Player*> targetList;

                    Map::PlayerList const& plList = pInstance->instance->GetPlayers();
                    for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
                    {
                        if ((*itr).getSource()->isAlive())
                            targetList.push_back((*itr).getSource());
                    }

                    uint32 mode = pInstance->instance->GetDifficulty();
                    uint32 count = 3;
                    if (mode == RAID_DIFFICULTY_25MAN_NORMAL || mode == RAID_DIFFICULTY_25MAN_HEROIC)
                        count = 6;

                    if (!targetList.empty())
                    {
                        std::list<Player*>::iterator itr;
                        for (uint32 i = 0; i < count && !targetList.empty(); i++)
                        {
                            itr = targetList.begin();
                            std::advance(itr, urand(0, targetList.size()-1));
                            me->CastSpell(*itr, SPELL_LAVA_STRIKE, true);
                            targetList.erase(itr);
                        }
                    }

                    eruptTimer = 2000;
                }
                else
                    eruptTimer -= diff;
            }

            if (bossRangeCheckTimer <= diff)
            {
                // 7y range? TODO: Check
                if (Creature* pBoss = GetClosestCreatureWithEntry(me, NPC_LORD_RHYOLITH, 7.0f, true))
                {
                    // animation
                    me->RemoveAurasDueToSpell(SPELL_ERUPTION);

                    // if it's in range, spawn crater and despawn us
                    /*Creature* pCrater = */pBoss->SummonCreature(NPC_CRATER, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0);
                    /*if (pCrater)
                        pCrater->CastSpell(pCrater, SPELL_VOLCANIC_ERUPTION, true);*/

                    // If the volcano was dormant, increase stacks of Molten Armor buff on boss by 2 to 4
                    if (me->HasAura(SPELL_VOLCANO_SMOKE))
                    {
                        if (boss_rhyolith::boss_rhyolithAI* pAI = (boss_rhyolith::boss_rhyolithAI*)(pBoss->GetAI()))
                            pAI->ModMoltenArmorStack(urand(2,4));
                    }
                    // Every time he steps on an active volcano, he loses 16 stacks of the buff
                    else
                    {
                        if (boss_rhyolith::boss_rhyolithAI* pAI = (boss_rhyolith::boss_rhyolithAI*)(pBoss->GetAI()))
                            pAI->ModObsidianArmorStack(-16);
                    }

                    me->ForcedDespawn(2000);
                    destroyed = true;
                }
                bossRangeCheckTimer = 1000;
            }
            else
                bossRangeCheckTimer -= diff;
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new npc_rhyolith_volcanoAI(c);
    }
};

#define FLOW_LENGTH 30
#define FLOW_DIST   2.0f

class npc_rhyolith_crater: public CreatureScript
{
public:
    npc_rhyolith_crater(): CreatureScript("npc_rhyolith_crater") {}

    struct npc_rhyolith_craterAI: public Scripted_NoMovementAI
    {
        npc_rhyolith_craterAI(Creature* c): Scripted_NoMovementAI(c)
        {
            Reset();
        }

        uint32 flowTimer;
        std::vector<Creature*> lastCreatures;
        uint32 flowStep;
        float tendency;

        Creature* centerStumb;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            flowTimer = 5000;
            flowStep  = 0;
            lastCreatures.clear();
            centerStumb = NULL;
            tendency  = 0.0f;
        }

        void UpdateAI(const uint32 diff)
        {
            if (flowTimer)
            {
                if (flowTimer <= diff)
                {
                    if (lastCreatures.empty())
                    {
                        centerStumb = me->SummonCreature(NPC_LAVA_FLOW, 0,0, 0, 0, TEMPSUMMON_MANUAL_DESPAWN, 0);
                        centerStumb->CastSpell(me, SPELL_LAVA_TUBE, true);
                        centerStumb->SetFloatValue(OBJECT_FIELD_SCALE_X, 4.0f);
                        tendency = ((float)urand(0,100.0f*M_PI/3))/100.0f - M_PI/6;

                        lastCreatures.resize(urand(4,6));
                        Position pos;
                        for (uint32 i = 0; i < lastCreatures.size(); i++)
                        {
                            me->GetNearPosition(pos, 5.0f, i*(2*M_PI/lastCreatures.size()));
                            lastCreatures[i] = me->SummonCreature(NPC_LAVA_FLOW, pos, TEMPSUMMON_TIMED_DESPAWN, 15000);
                        }
                    }

                    Position pos;
                    for (uint32 i = 0; i < lastCreatures.size(); i++)
                    {
                        lastCreatures[i]->GetNearPosition(pos, FLOW_DIST, i*(2*M_PI/lastCreatures.size())+tendency);
                        lastCreatures[i] = me->SummonCreature(NPC_LAVA_FLOW, pos, TEMPSUMMON_TIMED_DESPAWN, 15000);
                        lastCreatures[i]->GetAI()->DoAction(10000 - 160*flowStep);
                    }
                    flowStep++;

                    tendency *= 3.0f/4.0f;
                    if (flowStep % 10 == 0)
                        tendency = ((float)urand(0,100.0f*M_PI/3))/100.0f - M_PI/6;

                    if (flowStep >= FLOW_LENGTH)
                    {
                        centerStumb->RemoveAurasDueToSpell(SPELL_LAVA_TUBE);
                        centerStumb->ForcedDespawn();
                        flowTimer = 0;
                    }
                    else
                        flowTimer = 160;
                }
                else
                    flowTimer -= diff;
            }
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new npc_rhyolith_craterAI(c);
    }
};

class npc_rhyolith_lava_flow: public CreatureScript
{
public:
    npc_rhyolith_lava_flow(): CreatureScript("npc_rhyolith_lava_flow") {}

    struct npc_rhyolith_lava_flowAI: public Scripted_NoMovementAI
    {
        npc_rhyolith_lava_flowAI(Creature* c): Scripted_NoMovementAI(c)
        {
            Reset();
        }

        uint32 boomTimer;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->CastSpell(me, SPELL_MAGMA_FLOW_GROUND, true);
            boomTimer = 10000;
        }

        void UpdateAI(const uint32 diff)
        {
            if (boomTimer)
            {
                if (boomTimer <= diff)
                {
                    me->CastSpell(me, SPELL_MAGMA_FLOW_BOOM, true);
                    boomTimer = 0;
                }
                else
                    boomTimer -= diff;
            }
        }

        void DamageTaken(Unit* who, uint32 &damage)
        {
            damage = 0;
        }

        void AttackStart(Unit* who)
        {
            return;
        }

        void DoAction(const int32 act)
        {
            if (act > 0)
                boomTimer = act;
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new npc_rhyolith_lava_flowAI(c);
    }
};

class npc_rhyolith_spark: public CreatureScript
{
public:
    npc_rhyolith_spark(): CreatureScript("npc_rhyolith_spark") {}

    struct npc_rhyolith_sparkAI: public ScriptedAI
    {
        npc_rhyolith_sparkAI(Creature* c): ScriptedAI(c)
        {
            Reset();
        }

        uint32 infernalRageTimer;

        void Reset()
        {
            infernalRageTimer = 5000;
        }

        void EnterCombat(Unit* with)
        {
            me->CastSpell(me, SPELL_SPARK_IMMOLATION, true);
            infernalRageTimer = 5000;
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (infernalRageTimer <= diff)
            {
                me->CastSpell(me, SPELL_SPARK_INFERNAL_RAGE, true);
                infernalRageTimer = 5000;
            }
            else
                infernalRageTimer -= diff;
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new npc_rhyolith_sparkAI(c);
    }
};

class npc_rhyolith_fragment: public CreatureScript
{
public:
    npc_rhyolith_fragment(): CreatureScript("npc_rhyolith_fragment") {}

    struct npc_rhyolith_fragmentAI: public ScriptedAI
    {
        npc_rhyolith_fragmentAI(Creature* c): ScriptedAI(c)
        {
            Reset();
        }

        uint32 meltdownTimer;

        void Reset()
        {
            meltdownTimer = 30000;
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (meltdownTimer <= diff)
            {
                std::list<Player*> targetList;
                Map::PlayerList const& plList = me->GetInstanceScript()->instance->GetPlayers();
                for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
                {
                    if ((*itr).getSource()->isAlive() && (*itr).getSource()->IsWithinCombatRange(me, 100.0f))
                        targetList.push_back((*itr).getSource());
                }

                if (targetList.empty())
                    me->ForcedDespawn();
                else
                {
                    std::list<Player*>::iterator itr = targetList.begin();
                    std::advance(itr, urand(0, targetList.size()-1));
                    me->CastSpell(*itr, SPELL_FRAGMENT_MELTDOWN, true);
                }

                meltdownTimer = 30000;
            }
            else
                meltdownTimer -= diff;
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new npc_rhyolith_fragmentAI(c);
    }
};

void AddSC_boss_lord_rhyolith()
{
    new boss_rhyolith();
    new npc_rhyolith_feet();
    new npc_rhyolith_volcano();
    new npc_rhyolith_crater();
    new npc_rhyolith_lava_flow();
    new npc_rhyolith_spark();
    new npc_rhyolith_fragment();
}
