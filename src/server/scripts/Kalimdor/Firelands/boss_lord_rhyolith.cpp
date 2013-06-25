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
#include "MapManager.h"
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
    SPELL_ERUPTION_DAMAGE   = 98492,
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
    DISPLAYID_DAMAGED_1      = 38669,
    DISPLAYID_DAMAGED_2      = 38676,
    DISPLAYID_SHATTERED      = 38594, // less than 25% HP

    DISPLAYID_VOLCANO_STILL  = 38054,
    DISPLAYID_VOLCANO_ACTIVE = 38848,
};

struct SoundTextEntry
{
    uint32 sound;
    const char* text;
};

enum SoundTexts
{
    ST_AGGRO = 0,
    ST_DEATH,
    ST_ARMOR_WEAKEN_1,
    ST_ARMOR_WEAKEN_2,
    ST_ARMOR_WEAKEN_3,
    ST_ARMOR_WEAKEN_4,
    ST_ARMOR_WEAKEN_5,
    ST_KILL_1,
    ST_KILL_2,
    ST_KILL_3,
    ST_VOLCANO_1,
    ST_VOLCANO_2,
    ST_VOLCANO_3,
    ST_VOLCANO_4,
    ST_VOLCANO_5,
    ST_STOMP_1,
    ST_STOMP_2,
    ST_SHATTER_ARMOR,
    ST_MAX
};

static const SoundTextEntry rhyolithQuotes[ST_MAX] = {
    // aggro
    {24537, "Hah? Hruumph? Soft little fleshy-things? Here? Nuisances, nuisances!"},
    // death
    {24545, "Broken. Mnngghhh... broken..."},
    // armor weaken
    {24540, "Oww now hey."},
    {24541, "Graaahh!"},
    {24542, "Augh - smooshy little pests, look what you've done!"},
    {24543, "Uurrghh now you... you infuriate me!"},
    {24544, "Oh you little beasts..."},
    // kill player
    {24546, "Finished."},
    {24547, "So soft!"},
    {24548, "Squeak, little pest."},
    // activating volcano
    {24550, "Buuurrrnn!"},
    {24551, "Succumb to living flame."},
    {24552, "My inner fire can never die!"},
    {24553, "Consuuuuuuume!"},
    {24554, "Flesh, buuurrrns."},
    // stomp
    {24556, "Stomp now."},
    {24557, "I'll crush you underfoot!"},
    // shatter armor
    {24558, "Eons I have slept undisturbed... Now this... Creatures of flesh, now you will BURN!"}
};

static void PlayAndYell(Unit* source, uint32 index)
{
    if (index >= ST_MAX)
        return;

    source->MonsterYell(rhyolithQuotes[index].text, LANG_UNIVERSAL, 0);
    source->PlayDistanceSound(rhyolithQuotes[index].sound);
}

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
                pFoot->EnterVehicle(me, 0, false);
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
                pFoot->EnterVehicle(me, 1, false);
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
        uint32 lastDamage;
        uint32 directionPower;
        float direction;
        float moveAngle;
        uint32 directionUpdateTimer;
        uint32 movementUpdateTimer;
        uint8 directionTimes;
        bool savedLeft;

        uint8 displayIdPhase;

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
            lastDamage  = 0;
            direction   = 0.0f;
            moveAngle   = me->GetOrientation();
            directionPower = 50;
            directionUpdateTimer = 1000;
            movementUpdateTimer  = 1000;

            concussiveStompTimer = 3000;
            activateVolcanoTimer = 8000;
            summonTimer          = 15000;
            lavaCheckTimer       = 1000;
            magmaDrinkCount      = 0;
            magmaDrinkTimer      = 0;
            displayIdPhase       = 0;

            me->SetWalk(true);

            savedLeft      = true;
            directionTimes = 0;

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

            if (pInstance)
                pInstance->SetData(TYPE_RHYOLITH, NOT_STARTED);
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
                pInstance->SetData(TYPE_RHYOLITH, IN_PROGRESS);
                pInstance->DoSetMaxScriptedPowerToPlayers(100);
                pInstance->DoSetScriptedPowerToPlayers(50);
                directionPower = 50;
            }

            PlayAndYell(me, ST_AGGRO);

            phase = 1;

            directionUpdateTimer = 1000;
            movementUpdateTimer  = 1000;
            concussiveStompTimer = 5000;
            activateVolcanoTimer = 10000;
            summonTimer          = 15000;
            lavaCheckTimer       = 1000;
            magmaDrinkCount      = 0;
            magmaDrinkTimer      = 0;
            moveAngle            = me->GetOrientation();
            lastDamage           = 0;
            displayIdPhase       = 0;

            me->SetWalk(true);

            savedLeft = true;
            directionTimes = 0;

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

        void KilledUnit(Unit* target)
        {
            PlayAndYell(me, ST_KILL_1+urand(0,2));
        }

        void EnterEvadeMode()
        {
            me->RemoveAurasDueToSpell(SPELL_BALANCE_BAR);
            if (pInstance)
                pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BALANCE_BAR);

            Unit* foot = Unit::GetUnit(*me, leftFootGUID);
            if (foot)
            {
                foot->SetVisibility(VISIBILITY_ON);
                foot->CombatStop();
                foot->CastSpell(foot, SPELL_OBSIDIAN_ARMOR, true);
                if (Aura* armor = foot->GetAura(SPELL_OBSIDIAN_ARMOR))
                    armor->SetStackAmount(80);
            }

            foot = Unit::GetUnit(*me, rightFootGUID);
            if (foot)
            {
                foot->SetVisibility(VISIBILITY_ON);
                foot->CombatStop();
                foot->CastSpell(foot, SPELL_OBSIDIAN_ARMOR, true);
                if (Aura* armor = foot->GetAura(SPELL_OBSIDIAN_ARMOR))
                    armor->SetStackAmount(80);
            }

            ScriptedAI::EnterEvadeMode();
        }

        void JustDied(Unit* killer)
        {
            if (pInstance)
                pInstance->SetData(TYPE_RHYOLITH, DONE);

            Unit* foot = Unit::GetUnit(*me, leftFootGUID);
            if (foot)
                foot->Kill(foot);

            foot = Unit::GetUnit(*me, rightFootGUID);
            if (foot)
                foot->Kill(foot);

            PlayAndYell(me, ST_DEATH);
        }

        void UpdateMovement()
        {
            moveAngle += direction*(M_PI/7);
            moveAngle = MapManager::NormalizeOrientation(moveAngle);

            // if too far away, move to the center of platform
            if (me->GetDistance2d(platformCenter.m_positionX, platformCenter.m_positionY) > 75.0f)
                moveAngle = me->GetAngle(&platformCenter);

            float x, y, z;
            //me->GetClosePoint(x, y, z, me->GetObjectSize(), 30.0f, moveAngle);
            me->GetNearPoint2D(x, y, 30.0f, moveAngle);
            z = me->GetPositionZ();

            me->SetOrientation(moveAngle);
            me->GetMotionMaster()->MovementExpired(false);
            //me->GetMotionMaster()->MovePoint(1, x, y, z);
            me->GetMotionMaster()->MoveCharge(x, y, z, 2.25f);
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

                me->GetMotionMaster()->MovementExpired(false);

                directionUpdateTimer = 3000;
                movementUpdateTimer = 500;

                // change orientation to allow movement update
                moveAngle = me->GetAngle(&platformCenter);
                me->SetFacingTo(moveAngle);

                Map::PlayerList const& plList = pInstance->instance->GetPlayers();
                for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
                    me->CastSpell((*itr).getSource(), SPELL_MAGMA_SPIT, true);

                magmaDrinkTimer = 1000;
                magmaDrinkCount = 3;
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim() || !me->getVictim())
                return;

            // displayid switch at 75% and 50%
            if (displayIdPhase < 1 && me->GetHealthPct() < 75.0f)
            {
                displayIdPhase = 1;
                me->SetDisplayId(DISPLAYID_DAMAGED_1);
            }
            else if (displayIdPhase < 2 && me->GetHealthPct() < 50.0f)
            {
                displayIdPhase = 2;
                me->SetDisplayId(DISPLAYID_DAMAGED_2);
            }

            // phase switch at 25%
            if (me->GetHealthPct() < 25.0f && phase != 2)
            {
                // back to normal movement
                me->GetMotionMaster()->MovementExpired(true);
                me->GetMotionMaster()->MoveChase(me->getVictim());

                displayIdPhase = 3;
                me->RemoveAurasDueToSpell(SPELL_OBSIDIAN_ARMOR);
                me->SetDisplayId(DISPLAYID_SHATTERED);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, false);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_THREAT, false);

                me->SetWalk(false);

                PlayAndYell(me, ST_SHATTER_ARMOR);

                me->RemoveAurasDueToSpell(SPELL_BALANCE_BAR);
                if (pInstance)
                    pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BALANCE_BAR);

                me->CastSpell(me, SPELL_IMMOLATION, true);

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
                PlayAndYell(me, ST_STOMP_1+urand(0,1));
                concussiveStompTimer = 30000;
            }
            else
                concussiveStompTimer -= diff;

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

                PlayAndYell(me, ST_VOLCANO_1+urand(0,4));

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
                        direction = 0.0f;
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

            if (phase == 1)
            {
                /* Direction stuff
                 * Update direction depending on which foot was damaged more
                 * if no foot was damaged, then converge to the middle (straight forward, energy 50/100)
                 */
                if (directionUpdateTimer <= diff)
                {
                    uint32 totaldmg = leftDamage+rightDamage+meDamage;

                    // moving left
                    if (direction > 0.0f)
                    {
                        // would turn more left, but total damage isn't at least 90% of last damage
                        if (leftDamage > rightDamage && totaldmg < lastDamage*0.9f)
                            totaldmg = 0;
                    }
                    else // moving right
                    {
                        if (rightDamage > leftDamage && totaldmg < lastDamage*0.9f)
                            totaldmg = 0;
                    }

                    if (totaldmg != 0)
                    {
                        if (leftDamage > rightDamage)
                        {
                            if (!savedLeft)
                            {
                                savedLeft = true;
                                directionTimes = 0;
                            }
                            direction += ((((float)leftDamage)/((float)totaldmg)))/10.0f;
                        }
                        else
                        {
                            if (savedLeft)
                            {
                                savedLeft = false;
                                directionTimes = 0;
                            }
                            direction += (-(((float)rightDamage)/((float)totaldmg)))/10.0f;
                        }
                    }
                    else
                    {
                        direction = 2.0f*direction/5.0f;
                        if (fabs(direction) <= 0.12f)
                            direction = 0.0f;
                    }

                    if (directionTimes < 20)
                        directionTimes++;

                    //direction *= (1.0f - ((float)directionTimes)/25.0f);

                    if (direction > 1.0f)
                        direction = 1.0f;
                    else if (direction < -1.0f)
                        direction = -1.0f;

                    if (lastDamage == 0)
                        lastDamage = totaldmg;
                    else
                        lastDamage = (lastDamage + totaldmg)/2.0f;

                    leftDamage = 0;
                    rightDamage = 0;
                    meDamage = 0;

                    RefreshPowerBar(50-direction*50, false);

                    directionUpdateTimer = 2000;
                }
                else
                    directionUpdateTimer -= diff;

                if (movementUpdateTimer <= diff)
                {
                    if (!me->hasUnitState(UNIT_STAT_CASTING))
                        UpdateMovement();

                    movementUpdateTimer = 800;
                }
                else
                    movementUpdateTimer -= diff;

                /* Phase 1 spell stuff */

                if (summonTimer <= diff)
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
                    summonTimer -= diff;
            }
            else if (phase == 2)
            {
                /* TODO: HC version spell */

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
            me->ApplySpellImmune(0, IMMUNITY_ID, 49560, true);
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

    struct npc_rhyolith_volcanoAI: public Scripted_NoMovementAI
    {
        npc_rhyolith_volcanoAI(Creature* c): Scripted_NoMovementAI(c)
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
            me->setFaction(14);
            me->SetReactState(REACT_PASSIVE);
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

        void SpellHitTarget(Unit* target, const SpellEntry* spell)
        {
            if (spell->Id == SPELL_LAVA_STRIKE)
                me->CastSpell(target, 98492, true);
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
                            me->CastSpell(*itr, SPELL_LAVA_STRIKE, false);
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

                        PlayAndYell(pBoss, ST_ARMOR_WEAKEN_1+urand(0,4));
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

/*
SQL:

UPDATE creature_template SET AIName='', ScriptName='boss_lord_rhyolith', vehicleId=1606 WHERE entry=52558;

UPDATE creature_template SET modelid1=38415, modelid2=0, modelid3=0, modelid4=0, ScriptName='npc_rhyolith_feet' WHERE entry=52577;
UPDATE creature_template SET modelid1=38416, modelid2=0, modelid3=0, modelid4=0, ScriptName='npc_rhyolith_feet' WHERE entry=53087;

UPDATE creature_template SET modelid1=38054, modelid2=0, modelid3=0, modelid4=0, ScriptName='npc_rhyolith_volcano' WHERE entry=52582;
UPDATE creature_template SET modelid1=38063, modelid2=0, modelid3=0, modelid4=0, ScriptName='npc_rhyolith_crater' WHERE entry=52866;

UPDATE creature_template SET AIName='', ScriptName='npc_rhyolith_spark' WHERE entry=53211;
UPDATE creature_template SET AIName='', ScriptName='npc_rhyolith_fragment' WHERE entry=52620;

UPDATE instance_template SET script='instance_firelands' WHERE map=720;

-- lava flow npc
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (950000, 0, 0, 0, 0, 0, 17188, 0, 0, 0, 'Lava Flow', '', '', 0, 88, 88, 0, 14, 14, 0, 0.428571, 1.2, 1, 0, 0, 0, 0, 0, 1, 2000, 2000, 1, 33554560, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 1060, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 76, 1, 0, 0, 0, 'npc_rhyolith_lava_flow', 15595);

INSERT INTO conditions VALUES (13, 0, 98493, 0, 18, 1, 52582, 0, 0, '', 'Lord Rhyolith - Heated Volcano');
*/
