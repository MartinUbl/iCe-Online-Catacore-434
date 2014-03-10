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
    NPC_LORD_RHYOLITH   = 52558,
    NPC_LEFT_FOOT       = 52577,
    NPC_RIGHT_FOOT      = 53087,

    NPC_VOLCANO         = 52582,
    NPC_CRATER          = 52866,
    NPC_LAVA_FLOW       = 950000, // custom NPC

    NPC_SPARK           = 53211,
    NPC_FRAGMENT        = 52620,

    // Heroic
    NPC_LIQUID_OBSIDIAN = 52619,
    NPC_UNLEASHED_FLAME = 54347

};

static const uint32 npcListUnsummonAtReset[] = {NPC_VOLCANO, NPC_CRATER, NPC_SPARK, NPC_FRAGMENT, NPC_LAVA_FLOW,NPC_LIQUID_OBSIDIAN,NPC_UNLEASHED_FLAME};

static const Position platformCenter = {-374.337006f, -318.489990f, 100.413002f, 0.0f};

# define NEVER  (0xffffffff) // used as "delayed" timer ( max uint32 value)

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
    SPELL_SIT_DOWN          = 84119,

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
    SPELL_SPARK_IMMOLATION      = 98597,
    SPELL_SPARK_INFERNAL_RAGE   = 98596,
    SPELL_FRAGMENT_MELTDOWN     = 98646,

    // heroic spells
    SPELL_FUSE                  = 99875

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

            c->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
            c->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            c->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);

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

        uint32 leftDamage, rightDamage;
        uint32 lastDamage;
        float direction;
        float moveAngle;
        uint32 directionUpdateTimer;
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
        uint32 EruptionTimer;
        uint32 DPSTimer;
        uint32 sitTimer;
        bool   canAttackIn2Phase;
        uint32 sitAnimTimer;
        uint32 position_counter;
        uint32 lDamage_field[4];
        uint32 rDamage_field[4];
        uint32 directionPower;

        uint32 enrageTimer;
        bool beam;

        void Reset()
        {
            beam = false;

            me->SetDisplayId(DISPLAYID_NORMAL); // "dressed up"
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE); // Rhyolith can't be attack directly in phase 1

            me->InterruptNonMeleeSpells(false);
            me->RemoveAura(SPELL_SIT_DOWN);
            me->SetRooted(false);
            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            canAttackIn2Phase = false;

            position_counter = 0;
            leftDamage  = 0;
            rightDamage = 0;
            lastDamage  = 0;
            direction   = 0.0f;
            moveAngle   = me->GetOrientation();
            directionUpdateTimer = 1000;
            DPSTimer = 1000;
            concussiveStompTimer = 15000;
            activateVolcanoTimer = 25000;
            summonTimer          = 22000;
            lavaCheckTimer       = 1000;
            EruptionTimer        = 20000;
            sitTimer             = NEVER;
            sitAnimTimer         = NEVER;
            magmaDrinkCount      = 0;
            magmaDrinkTimer      = 0;
            displayIdPhase       = 0;
            directionPower = 50;

            memset (lDamage_field,0,4*sizeof(uint32));
            memset (rDamage_field,0,4*sizeof(uint32));

            savedLeft      = true;
            canAttackIn2Phase = false;
            directionTimes = 0;

            if (!pInstance || !pInstance->instance)
                return;

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
                if(pInstance->GetData(TYPE_RHYOLITH)!=DONE)
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
            }
        }

        void EnterCombat(Unit* pWho)
        {
            me->CastSpell(me, SPELL_BALANCE_BAR, true);
            me->SetUInt64Value(UNIT_FIELD_TARGET,0);

            if (pInstance)
            {
                pInstance->SetData(TYPE_RHYOLITH, IN_PROGRESS);
                pInstance->DoSetMaxScriptedPowerToPlayers(100);
                pInstance->DoSetScriptedPowerToPlayers(50);
            }

            PlayAndYell(me, ST_AGGRO);

            phase = 1;

            directionUpdateTimer = 1000;
            concussiveStompTimer = 15000;
            activateVolcanoTimer = 25000;
            summonTimer          = 22000;
            lavaCheckTimer       = 1000;
            magmaDrinkCount      = 0;
            magmaDrinkTimer      = 0;
            moveAngle            = me->GetOrientation();
            lastDamage           = 0;
            displayIdPhase       = 0;
            directionPower       = 50;

            me->SetWalk(false);
            me->SetSpeed(MOVE_WALK,1.2f,true);

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

            me->RemoveAurasDueToSpell(SPELL_MOLTEN_ARMOR);
            me->RemoveAurasDueToSpell(101157);
            me->RemoveAurasDueToSpell(101158);
            me->RemoveAurasDueToSpell(101159);

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
            me->RemoveAurasDueToSpell(SPELL_MOLTEN_ARMOR);
            me->RemoveAurasDueToSpell(101157);
            me->RemoveAurasDueToSpell(101158);
            me->RemoveAurasDueToSpell(101159);

            if (pInstance)
                pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BALANCE_BAR);

            Unit* foot = Unit::GetUnit(*me, leftFootGUID);
            if (foot)
            {
                if (pInstance)
                    pInstance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, foot);
                foot->SetVisibility(VISIBILITY_ON);
                foot->CombatStop();
                foot->ToCreature()->AI()->EnterEvadeMode();
                foot->RemoveAllAuras();
                foot->SetFullHealth();
                foot->CastSpell(foot, SPELL_OBSIDIAN_ARMOR, true);
                if (Aura* armor = foot->GetAura(SPELL_OBSIDIAN_ARMOR))
                    armor->SetStackAmount(80);
            }

            foot = Unit::GetUnit(*me, rightFootGUID);
            if (foot)
            {
                if (pInstance)
                    pInstance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, foot);
                foot->SetVisibility(VISIBILITY_ON);
                foot->CombatStop();
                foot->ToCreature()->AI()->EnterEvadeMode();
                foot->RemoveAllAuras();
                foot->SetFullHealth();
                foot->CastSpell(foot, SPELL_OBSIDIAN_ARMOR, true);
                if (Aura* armor = foot->GetAura(SPELL_OBSIDIAN_ARMOR))
                    armor->SetStackAmount(80);
            }

            if (pInstance)
                    pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BALANCE_BAR);

            ScriptedAI::EnterEvadeMode();
        }

        void JustDied(Unit* killer)
        {
            if (pInstance)
                    pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BALANCE_BAR);

            if (pInstance)
                pInstance->SetData(TYPE_RHYOLITH, DONE);

            Unit* foot = Unit::GetUnit(*me, leftFootGUID);
            if (foot)
                foot->Kill(foot);

            foot = Unit::GetUnit(*me, rightFootGUID);
            if (foot)
                foot->Kill(foot);


            std::list<Creature*> crList;
            for (uint32 i = 0; i < sizeof(npcListUnsummonAtReset) / sizeof(uint32); i++)
            {
                GetCreatureListWithEntryInGrid(crList, me, npcListUnsummonAtReset[i], 200.0f);
                for (std::list<Creature*>::iterator itr = crList.begin(); itr != crList.end(); ++itr)
                    (*itr)->ForcedDespawn();
            }

            PlayAndYell(me, ST_DEATH);
        }

        void UpdateMovement(void)
        {
            // if too far away, move to the center of platform
            if (me->GetDistance2d(platformCenter.m_positionX, platformCenter.m_positionY) > 75.0f)
                moveAngle = me->GetAngle(&platformCenter);

            float x, y, z;
            me->GetNearPoint2D(x, y, 30.0f, moveAngle);
            z = me->GetPositionZ();

            me->SetOrientation(moveAngle);
            me->GetMotionMaster()->MovementExpired(false);
            me->SetWalk(true);
            me->SetSpeed(MOVE_WALK,1.2f,true);
            me->GetMotionMaster()->MovePoint(0, x, y, z);
            me->SetSpeed(MOVE_WALK,1.2f,true);
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
            if (amount >= 0) // Add armor
            {
                for ( int32 i = 0 ; i < amount; i++)
                {
                    Aura * a = me->GetAura(SPELL_MOLTEN_ARMOR);
                    if (!a)
                        me->GetAura(101157);
                    if (!a)
                        me->GetAura(101158);
                    if (!a)
                        me->GetAura(101159);
                    if (a)
                        a->SetStackAmount(a->GetStackAmount() + 1);
                    else
                        me->AddAura(SPELL_MOLTEN_ARMOR,me);
                }
            }
            else    // Reduce armor
            {
                amount *= -1;
                for (int32 i = 0; i < amount; i++)
                {
                    if (me->HasAura(SPELL_MOLTEN_ARMOR))
                        me->RemoveAuraFromStack(SPELL_MOLTEN_ARMOR);
                    else
                    if (me->HasAura(101157))
                        me->RemoveAuraFromStack(101157);
                    else
                    if (me->HasAura(101158))
                        me->RemoveAuraFromStack(101158);
                    else
                    if (me->HasAura(101159))
                        me->RemoveAuraFromStack(101159);
                }
            }
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

                if ( phase == 1) // Only in first phase
                {
                    uint32 cnt = urand(2,3);
                    for (uint32 i = 0; i < cnt; i++)
                        me->CastSpell(me, SPELL_VOLCANIC_BIRTH, true);
                }
            }
            else if (spell->Id == SPELL_DRINK_MAGMA)
            {
                if (!pInstance)
                    return;

                me->GetMotionMaster()->MovementExpired(false);

                directionUpdateTimer = 3000;

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

        uint32 GetExactDamage(uint32 damage) // Return exact dmg before reduction from Obsidian Armor
        {
            uint32 stacks = 0;

            Aura* pArmor = me->GetAura(SPELL_OBSIDIAN_ARMOR);
            if (pArmor)
                stacks = pArmor->GetStackAmount();

            if(stacks)
                return ((damage * 100)/(100 - stacks));
            else
                return damage;
        }

        inline uint32 GetTotalDamage(void)
        {
            return GetTotalRightDamage() + GetTotalLeftDamage();
        }

        inline uint32 GetTotalLeftDamage(void)
        {
            uint32 totalDamage = 0;

            for (uint32 i = 0 ; i < 4; i++)
                totalDamage  += lDamage_field[i];

            return totalDamage;
        }

        inline uint32 GetTotalRightDamage(void)
        {
            uint32 totalDamage = 0;

            for (uint32 i = 0 ; i < 4; i++)
                totalDamage  += rDamage_field[i];

            return totalDamage;
        }

        inline bool isTurningLeft(void)
        {
            return (GetTotalLeftDamage() > GetTotalRightDamage()) ? true : false;
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
                //reset damage count dealt by players for getting loot
                me->ResetPlayerDamageReq();
                //me->SetReactState(REACT_AGGRESSIVE);
                //me->SetUInt64Value(UNIT_FIELD_TARGET,me->getVictim()->GetGUID());

                // back to normal movement

                //me->GetMotionMaster()->MovementExpired(true);
                //me->GetMotionMaster()->MoveChase(me->getVictim());
                me->SetWalk(false);
                me->SetSpeed(MOVE_RUN,1.0f,true);

                displayIdPhase = 3;
                me->RemoveAurasDueToSpell(SPELL_OBSIDIAN_ARMOR);
                me->SetDisplayId(DISPLAYID_SHATTERED);

                PlayAndYell(me, ST_SHATTER_ARMOR);

                me->RemoveAurasDueToSpell(SPELL_BALANCE_BAR);
                if (pInstance)
                    pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BALANCE_BAR);

                me->CastSpell(me, SPELL_IMMOLATION, true);

                // In transition Rhyolith will sit down for a while
                me->AddAura(SPELL_SIT_DOWN, me); // Sit animation
                me->StopMoving();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                me->SetRooted(true);
                canAttackIn2Phase = false;
                sitTimer = 2000;
                sitAnimTimer = 6000;

                Unit* foot = Unit::GetUnit(*me, leftFootGUID);
                if (foot)
                {
                    if (pInstance)
                        pInstance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, foot);
                    foot->SetVisibility(VISIBILITY_OFF);
                }

                foot = Unit::GetUnit(*me, rightFootGUID);
                if (foot)
                {
                    if (pInstance)
                        pInstance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, foot);
                    foot->SetVisibility(VISIBILITY_OFF);
                }
                phase = 2;
                concussiveStompTimer = 13000;
                me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE);
            }

            /* both phases */
            if (concussiveStompTimer <= diff)
            {
                me->StopMoving(); // Dont move during casting
                directionUpdateTimer = 3100; // 3s cast time
                me->CastSpell(me, SPELL_CONCUSSIVE_STOMP, false);
                PlayAndYell(me, ST_STOMP_1+urand(0,1));
                if ( phase == 1)
                    concussiveStompTimer = 30500;
                else
                    concussiveStompTimer = 13000; // In second phase stomp every 13 seconds, but dont spawn volcanos
            }
            else
                concussiveStompTimer -= diff;

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
                me->SetUInt64Value(UNIT_FIELD_TARGET, 0);

                if (DPSTimer <= diff)
                {
                    if(position_counter == 4) // Fill field from start
                        position_counter = 0;

                    // We need to get real damage before reduction from Obsidian armor
                    lDamage_field[position_counter] = GetExactDamage(leftDamage);
                    rDamage_field[position_counter] = GetExactDamage(rightDamage);

                    leftDamage = 0; // Rest last dps for both legs
                    rightDamage = 0;

                    position_counter++;

                    DPSTimer = 1000;
                }
                else DPSTimer -= diff;

                /* Direction stuff
                 * Update direction depending on which foot was damaged more
                 */
                if (directionUpdateTimer <= diff)
                {
                    uint32 powerPoint = (Is25ManRaid()) ? 5000 : 2000; // These numbers control sensibility of stearing
                    bool noDmgIncoming = false;

                    if (isTurningLeft())
                    {
                        uint32 leftDamage = GetTotalLeftDamage() - GetTotalRightDamage();
                        if(leftDamage != 0)
                        {
                            uint32 powerDamage = (leftDamage/powerPoint >= 25) ? 25 : (leftDamage/powerPoint);
                            float angle = powerDamage * 0.0084f; // Comes from my calculation ( Maximum 30 s to make one turn around)
                            direction = 25.0f / powerDamage;
                            moveAngle += angle;
                            moveAngle = MapManager::NormalizeOrientation(moveAngle);
                        }
                        else noDmgIncoming = true;
                    }
                    else // Is turning right
                    {
                        uint32 rightDamage = GetTotalRightDamage() - GetTotalLeftDamage();
                        if(rightDamage != 0)
                        {
                            uint32 powerDamage = (rightDamage/powerPoint >= 25) ? 25 : (rightDamage/powerPoint);
                            float angle = powerDamage * 0.0084f;
                            direction = 25.0f / powerDamage;
                            direction *= -1.0f;
                            moveAngle -= angle;
                            moveAngle = MapManager::NormalizeOrientation(moveAngle);
                        }
                        else noDmgIncoming = true;
                    }

                    if (noDmgIncoming)
                        RefreshPowerBar(50, false);
                    else
                        RefreshPowerBar(50 - direction * 50, false);

                    if (!me->hasUnitState(UNIT_STAT_CASTING))
                        UpdateMovement();


                    directionUpdateTimer = 1000;
                }
                else
                    directionUpdateTimer -= diff;

                /* Phase 1 spell stuff */
                if (summonTimer <= diff)
                {
                    // summon 1 spark OR 5 fragments, 50/50 chances
                    if (urand(0,100) > 50)
                    {
                        for (uint32 i = 0; i < 5; i++)
                            me->CastSpell(me, SPELL_SUMMON_FRAGMENT, true);
                    }
                    else
                        me->CastSpell(me, SPELL_SUMMON_SPARK, true);

                    ModMoltenArmorStack(-1);

                    summonTimer = urand(22000, 24000);
                }
                else
                    summonTimer -= diff;

                if (activateVolcanoTimer <= diff)
                {
                    std::list<Creature*> volcanoList;
                    GetCreatureListWithEntryInGrid(volcanoList, me, NPC_VOLCANO, 200.0f);
                    for (std::list<Creature*>::iterator itr = volcanoList.begin(); itr != volcanoList.end(); )
                    {
                        if ((*itr)->HasAura(SPELL_VOLCANO_SMOKE)) // Only on non active volcano
                            itr++;
                        else
                            itr = volcanoList.erase(itr);
                    }

                    bool found = false;

                    if (volcanoList.size() > 0)
                    {
                        for (std::list<Creature*>::iterator iter = volcanoList.begin(); iter != volcanoList.end(); ++iter)
                            if ( (*iter) && me->HasInArc(M_PI,(*iter)) ) // Pick volcano in front of boss if possible
                            {
                                me->CastSpell((*iter), SPELL_ACTIVATE_VOLCANO, true);
                                volcanoList.erase(iter); // This volcano is not interesting anymore
                                found = true;
                                break;
                            }
                    }

                    if (volcanoList.size() > 0 && found == false) // If we did not found volcano in front of boss, pick random
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

                if (EruptionTimer <= diff)
                {
                    std::list<Creature*> craterList;
                    GetCreatureListWithEntryInGrid(craterList, me, NPC_CRATER, 200.0f);

                    if (craterList.size() > 0 )
                    {
                        uint32 randpos = urand(0,craterList.size()-1);

                        std::list<Creature*>::iterator itr = craterList.begin();
                        std::advance(itr, randpos);
                        if (*itr)
                            (*itr)->AI()->DoAction(0); // Erupt
                    }

                    EruptionTimer = urand(10000,30000);
                }
                else EruptionTimer -= diff;

            }
            else if (phase == 2)
            {
                if (canAttackIn2Phase == false)
                {
                    me->SetReactState(REACT_PASSIVE);
                    me->SetUInt64Value(UNIT_FIELD_TARGET, 0);
                }

                if (sitTimer <= diff)
                {
                    me->RemoveAura(SPELL_SIT_DOWN);
                    sitTimer = NEVER;
                }
                else sitTimer -= diff;

                if (sitAnimTimer <= diff)
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                    me->SetRooted(false);
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->SetUInt64Value(UNIT_FIELD_TARGET, me->getVictim()->GetGUID());
                    me->GetMotionMaster()->MovementExpired(true);
                    me->GetMotionMaster()->MoveChase(me->getVictim());
                    canAttackIn2Phase = true;
                    sitAnimTimer = NEVER;
                }
                else sitAnimTimer -= diff;

                if (beam == false)
                {
                    beam = true;

                    // Despawn all summons
                    std::list<Creature*> crList;
                    for (uint32 i = 0; i < sizeof(npcListUnsummonAtReset)/sizeof(uint32); i++)
                    {
                        GetCreatureListWithEntryInGrid(crList, me, npcListUnsummonAtReset[i], 200.0f);
                        for (std::list<Creature*>::iterator itr = crList.begin(); itr != crList.end(); ++itr)
                            (*itr)->ForcedDespawn();
                    }

                    // The Eruption debuff is now cleared when transitioning into phase 2 of the fight.
                    if (pInstance)
                        pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_ERUPTION_DAMAGE);

                    if (IsHeroic())
                    {
                        //me->CastSpell(me->getVictim(),101324,true); // Summon spell

                        Unit * pl1 = SelectTarget(SELECT_TARGET_RANDOM,0,100.0f,true);
                        Unit * pl2 = SelectTarget(SELECT_TARGET_RANDOM,0,100.0f,true);
                        Unit * pl3 = SelectTarget(SELECT_TARGET_RANDOM,0,100.0f,true);

                        if (!pl1 || !pl2 || !pl3)
                            return;

                        me->SummonCreature(NPC_UNLEASHED_FLAME,pl1->GetPositionX()+5,pl1->GetPositionY()+10,pl1->GetPositionZ(),0.0f);
                        me->SummonCreature(NPC_UNLEASHED_FLAME,pl2->GetPositionX()+10,pl2->GetPositionY()+5,pl2->GetPositionZ(),0.0f);
                        me->SummonCreature(NPC_UNLEASHED_FLAME,pl3->GetPositionX()-5,pl3->GetPositionY()-10,pl3->GetPositionZ(),0.0f);
                    }
                }

                if (canAttackIn2Phase)
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
            c->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
            c->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            c->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
            instance = c->GetInstanceScript();
        }

        uint32 SynchronizeTimer;
        InstanceScript * instance;

        void Reset()
        {
            SynchronizeTimer = 3000;
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

         void SpellHit(Unit* pSrc, const SpellEntry* spell)
         {
            if (spell->Id == SPELL_FUSE)
            {
                me->CastSpell(me,SPELL_OBSIDIAN_ARMOR,true);

                if (me->GetEntry() == NPC_LEFT_FOOT)
                {
                    if (Creature * rightFoot = me->FindNearestCreature(NPC_RIGHT_FOOT,100.0f,true))
                        rightFoot->CastSpell(rightFoot,SPELL_OBSIDIAN_ARMOR,true);
                }
                else if (Creature * leftFoot = me->FindNearestCreature(NPC_LEFT_FOOT,100.0f,true))
                        leftFoot->CastSpell(leftFoot,SPELL_OBSIDIAN_ARMOR,true);
            }
         }

        void EnterCombat(Unit* /*who*/)
        {
            me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS,18.0f);

            if (instance)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            me->SetUInt64Value(UNIT_FIELD_TARGET,0);
        }

        void EnterEvadeMode()
        {
            ScriptedAI::EnterEvadeMode();
        }

        void JustDied(Unit* killer)
        {
            if (instance)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (SynchronizeTimer <= diff)
            {
                if (me->GetEntry() == NPC_LEFT_FOOT)
                {
                    if (Creature * rightFoot = me->FindNearestCreature(NPC_RIGHT_FOOT,50.0f,true))
                    {
                        uint32 shareHealth = me->GetHealth() + rightFoot->GetHealth();
                        shareHealth /= 2;
                        me->SetHealth(shareHealth);
                        rightFoot->SetHealth(shareHealth);
                    }
                }

                SynchronizeTimer = 1000;
            }
            else SynchronizeTimer -= diff;
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

            if (eruptTimer && pInstance)
            {
                if (eruptTimer <= diff)
                {
                    std::list<Player*> targetList;

                    Map::PlayerList const& plList = pInstance->instance->GetPlayers();
                    for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
                    {
                        if ((*itr).getSource()->isAlive() && (*itr).getSource()->isGameMaster() == false) 
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
                Creature* pBoss = GetClosestCreatureWithEntry(me, NPC_LORD_RHYOLITH, 200.0f, true);

                if (pBoss && me->GetExactDist2d(pBoss) <= 17.0f)
                {
                    // animation
                    me->RemoveAurasDueToSpell(SPELL_ERUPTION);
                    pBoss->SummonCreature(NPC_CRATER, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN,60000);

                    // If the volcano was dormant, increase stacks of Molten Armor by one
                    if (me->HasAura(SPELL_VOLCANO_SMOKE))
                    {
                        if (boss_rhyolith::boss_rhyolithAI* pAI = (boss_rhyolith::boss_rhyolithAI*)(pBoss->GetAI()))
                            pAI->ModMoltenArmorStack(urand(2,3));
                    }
                    // Every time he steps on an active volcano, he loses 8 stacks of the buff ( 16 was nerfed on 4.3.4)
                    else
                    {
                        if (IsHeroic())
                        {
                            float angle,dist;

                            for ( uint8 i = 0; i < 5; i++ ) // Summon 5 Liquid Obsidians from the edge of platform
                            {
                                angle = (float)(urand(0, 628)) / 100.0f;
                                dist = 58.0f;
                                pBoss->SummonCreature(NPC_LIQUID_OBSIDIAN, -374.337006f + cos(angle) * dist, -318.489990f + sin(angle) * dist, 102.0f ,0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                            }
                        }

                        pBoss->MonsterTextEmote("Lord Rhyolith's armor is weakened by the active volcano.", 0, true);
                        if (boss_rhyolith::boss_rhyolithAI* pAI = (boss_rhyolith::boss_rhyolithAI*)(pBoss->GetAI()))
                            pAI->ModObsidianArmorStack(-10);

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
        bool erupted;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            flowTimer = 0;
            flowStep  = 0;
            lastCreatures.clear();
            tendency  = 0.0f;
            erupted = false;
        }

        void DoAction(const int32 param) // Used for eruption
        {
            if (erupted == false)
            {
                flowTimer = 1;
                erupted = true;

                if(Creature* centerStumb = me->SummonCreature(NPC_LAVA_FLOW, me->GetPositionX(),me->GetPositionY(),me->GetPositionZ(),0.0f,TEMPSUMMON_MANUAL_DESPAWN, 0))
                {
                    centerStumb->CastSpell(centerStumb,SPELL_LAVA_TUBE,true);
                    centerStumb->SetFloatValue(OBJECT_FIELD_SCALE_X, 5.0f);
                }
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (flowTimer)
            {
                if (flowTimer <= diff)
                {
                    if (lastCreatures.empty())
                    {
                        tendency = ((float)urand(0,100.0f*M_PI/3))/100.0f - M_PI/6;

                        lastCreatures.resize(urand(4,6));
                        Position pos;
                        for (uint32 i = 0; i < lastCreatures.size(); i++)
                        {
                            me->GetNearPosition(pos, 5.0f, i*(2*M_PI/lastCreatures.size()));
                            lastCreatures[i] = me->SummonCreature(NPC_LAVA_FLOW, pos, TEMPSUMMON_MANUAL_DESPAWN, 0);
                        }
                    }

                    Position pos;
                    for (uint32 i = 0; i < lastCreatures.size(); i++)
                    {
                        lastCreatures[i]->GetNearPosition(pos, FLOW_DIST, i*(2*M_PI/lastCreatures.size())+tendency);
                        lastCreatures[i] = me->SummonCreature(NPC_LAVA_FLOW, pos, TEMPSUMMON_MANUAL_DESPAWN, 0);
                        lastCreatures[i]->GetAI()->DoAction(10000 - 160*flowStep);
                    }
                    flowStep++;

                    tendency *= 3.0f/4.0f;
                    if (flowStep % 10 == 0)
                        tendency = ((float)urand(0,100.0f*M_PI/3))/100.0f - M_PI/6;

                    if (flowStep >= FLOW_LENGTH)
                    {
                        me->RemoveAurasDueToSpell(SPELL_LAVA_TUBE);
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
                    if (!me->HasAura(SPELL_LAVA_TUBE))
                        me->CastSpell(me, SPELL_MAGMA_FLOW_BOOM, true);
                    me->ForcedDespawn(7000);
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
        uint32 aggressiveTimer;

        void Reset()
        {
            aggressiveTimer = 4000;
            infernalRageTimer = aggressiveTimer + 5000;
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, false);
        }

        void UpdateAI(const uint32 diff)
        {
            if (aggressiveTimer <= diff)
            {
                me->CastSpell(me, SPELL_SPARK_IMMOLATION, true);
                me->SetInCombatWithZone();
                aggressiveTimer = NEVER;
                infernalRageTimer = 5000;
            }
            else aggressiveTimer -= diff;

            if (!UpdateVictim())
                return;

            if (infernalRageTimer <= diff)
            {
                me->CastSpell(me, SPELL_SPARK_INFERNAL_RAGE, true);
                infernalRageTimer = 5000;
            }
            else
                infernalRageTimer -= diff;

            DoMeleeAttackIfReady();
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
            victimGUID = 0;
        }

        uint32 meltdownTimer;
        uint32 aggressiveTimer;
        uint64 victimGUID;

        void Reset()
        {
            aggressiveTimer = 4000;
            meltdownTimer = aggressiveTimer + 30000;
        }

        void UpdateAI(const uint32 diff)
        {
            if (aggressiveTimer <= diff)
            {
                me->CastSpell(me, SPELL_FRAGMENT_MELTDOWN, true);
                me->SetInCombatWithZone();
                aggressiveTimer = NEVER;
                meltdownTimer = 30000;
            }
            else aggressiveTimer -= diff;

            if (!UpdateVictim())
                return;

            if (meltdownTimer <= diff)
            {
                if ( Unit * target = SelectTarget(SELECT_TARGET_RANDOM,0,100.0f,true))
                {
                    victimGUID = target->GetGUID();
                    me->GetMotionMaster()->MoveCharge(target->GetPositionX(),target->GetPositionY(),target->GetPositionZ());
                }
                meltdownTimer = NEVER;
            }
            else
                meltdownTimer -= diff;

            if(victimGUID)
            {
                Unit * vic = Unit::GetUnit(*me,victimGUID);
                if(vic && me->IsWithinMeleeRange(vic))
                {
                    me->CastSpell(me,98649,true);
                    victimGUID = 0;
                    me->Kill(me);
                }
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new npc_rhyolith_fragmentAI(c);
    }
};

class npc_Liquid_Obsidian : public CreatureScript
{
public:
   npc_Liquid_Obsidian() : CreatureScript("npc_Liquid_Obsidian") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_Liquid_ObsidianAI (creature);
    }

    struct npc_Liquid_ObsidianAI : public ScriptedAI
    {
        npc_Liquid_ObsidianAI(Creature* creature) : ScriptedAI(creature)
        {
            summonerGUID = 0;
        }

        uint32 Path_correction_timer;
        uint64 summonerGUID;
        bool arrived;

        void Reset()
        {
            arrived = false;
            Path_correction_timer = 500;
            me->SetReactState(REACT_PASSIVE);
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, false);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);
            me->SetSpeed(MOVE_RUN,0.3f,true);
        }

        void IsSummonedBy(Unit* pSummoner)
        {
            if (pSummoner && pSummoner->ToCreature())
                summonerGUID = pSummoner->GetGUID();
        }

        void UpdateAI ( const uint32 diff)
        {
            if (me->HasAuraType(SPELL_AURA_MOD_STUN))
            {
                me->StopMoving();
                return;
            }

            // WE MUST CLACULATE ACTUAL MOVEMENT SPEED MANULLY !
            int32 speedReduction = me->GetTotalAuraModifier(SPELL_AURA_MOD_DECREASE_SPEED);
            speedReduction = (speedReduction < -50) ? -50 : speedReduction; // Maximum 50 % movement speed reduction
            speedReduction *= -1;

            float speed = 0.3f; // Base speed
            if (speedReduction)
                speed = (speed * speedReduction) / 100;
            me->SetSpeed(MOVE_RUN, speed, true);

            if (Path_correction_timer <= diff && arrived == false)
            {
                if(Unit * pLord = Unit::GetUnit(*me,summonerGUID))
                {
                    if (me->GetExactDist2d(pLord) <= 8.0f)
                    {
                        me->StopMoving();
                        arrived = true;

                        Creature * lFoot = me->FindNearestCreature(NPC_LEFT_FOOT,100.0f,true);
                        Creature * rFoot = me->FindNearestCreature(NPC_RIGHT_FOOT,100.0f,true);

                        if (urand(0,1) && lFoot)
                            me->CastSpell(lFoot,SPELL_FUSE,true);
                        else if (rFoot)
                            me->CastSpell(rFoot,SPELL_FUSE,true);

                        if (pLord->GetDisplayId() != 38594) // Phase 2
                            pLord->CastSpell(pLord,SPELL_OBSIDIAN_ARMOR,false);
                    }
                    else
                        me->GetMotionMaster()->MovePoint(0,pLord->GetPositionX(),pLord->GetPositionY(),pLord->GetPositionZ());
                }

                Path_correction_timer = 500;
            }
            else Path_correction_timer -= diff;

        }
    };
};

class npc_Unleashed_flame : public CreatureScript
{
public:
   npc_Unleashed_flame() : CreatureScript("npc_Unleashed_flame") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_Unleashed_flameAI (creature);
    }

    struct npc_Unleashed_flameAI : public ScriptedAI
    {
        npc_Unleashed_flameAI(Creature* creature) : ScriptedAI(creature)
        {
            summonerGUID = 0;
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE);
        }

        uint32 motionTimer;
        uint64 summonerGUID;

        void Reset()
        {
            me->setFaction(14);
            motionTimer = 200;
            me->SetReactState(REACT_PASSIVE);
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, false);

            me->SetSpeed(MOVE_RUN,0.8f,true);

            me->CastSpell(me,101313,true); // Laser visual on ground + aoe damage trigerring
        }

        void IsSummonedBy(Unit* pSummoner)
        {
            if (pSummoner && pSummoner->ToCreature())
                summonerGUID = pSummoner->GetGUID();

            if(Unit * pLord = Unit::GetUnit(*me,summonerGUID))
                me->CastSpell(pLord,86956,true); // Visual beam to Rhyolith
        }

        void UpdateAI ( const uint32 diff)
        {
            if (motionTimer <= diff)
            {
                    if (urand(0,100) >= 50 ) //  55% chance to go at random position on platform
                    {
                        float angle =(float)urand(0,6) + 0.28f;
                        Position pos;
                        me->GetNearPosition(pos, 34.0f,angle);
                        me->GetMotionMaster()->MovePoint(0,pos);
                    }
                    else // go to random player position
                    {
                        if(Creature * pLord = Creature::GetCreature(*me,summonerGUID))
                        {
                            Unit * target = pLord->AI()->SelectTarget(SELECT_TARGET_RANDOM,0,100.0f,true);
                            if (target)
                                me->GetMotionMaster()->MovePoint(0,target->GetPositionX(),target->GetPositionY(),target->GetPositionZ());

                        }
                    }
                motionTimer = 3000;
            }
            else motionTimer -= diff;

        }
    };
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
    // Heroic
    new npc_Liquid_Obsidian();
    new npc_Unleashed_flame();
}

/* HEROIC SQLs

UPDATE `creature_template` SET `ScriptName`='npc_Liquid_Obsidian' WHERE  `entry`=52619 LIMIT 1;
UPDATE `creature_template` SET `ScriptName`='npc_Unleashed_flame' WHERE  `entry`=54347 LIMIT 1;
UPDATE `creature_template` SET `modelid2`=0 WHERE  `entry`=54347 LIMIT 1;

*/

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
