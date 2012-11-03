/*
* Copyright (C) 2010-2011 Project SkyFire <http://www.projectskyfire.org/>
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

#include"ScriptPCH.h"
#include"baradin_hold.h"

enum Spells
{
    SPELL_BERSERK              = 47008,
    SPELL_CONSUMING_DARKNESS   = 88954,
//  H_SPELL_CONSUMING_DARKNESS = 95173, // unused
    SPELL_FEL_FIRESTORM        = 88972,
    SPELL_METEOR_SLASH         = 88942,
    H_SPELL_METEOR_SLASH       = 95172,
};

class boss_argaloth: public CreatureScript
{
public:
    boss_argaloth() : CreatureScript("boss_argaloth") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
       return new boss_argalothAI(pCreature);
    }
    struct boss_argalothAI: public ScriptedAI
    {
        boss_argalothAI(Creature* pCreature) : ScriptedAI(pCreature), Summons(pCreature)
        {
            pInstance = pCreature->GetInstanceScript();

            pCreature->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            pCreature->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
            pCreature->ApplySpellImmune(0, IMMUNITY_ID, 49560, true); // Death Grip jump effect
            pCreature->ApplySpellImmune(0, IMMUNITY_ID, 81261, true); // Solar Beam
            pCreature->ApplySpellImmune(0, IMMUNITY_ID, 88625, true); // Chastise
        }

        InstanceScript *pInstance;

        bool berserk;
        uint32 berserk_timer;
        uint32 darkness_timer;
        uint32 meteor_timer;
        uint32 update_timer;
        uint32 firestorm_progress;
        uint8 firestorm_count;
        SummonList Summons;


        void Reset()
        {
            berserk = false;
            berserk_timer = 300000; // 5 min Enrage Timer
            darkness_timer = 15000;
            meteor_timer = 9000;
            update_timer = 1000;
            firestorm_progress = 0;
            firestorm_count = 0;
            Summons.DespawnAll();

            if (pInstance && pInstance->GetData(DATA_ARGALOTH_EVENT) != DONE)
                pInstance->SetData(DATA_ARGALOTH_EVENT, NOT_STARTED);
        }

        void EnterCombat(Unit* /*Ent*/)
        {
            DoZoneInCombat(me);
            if (pInstance)
                pInstance->SetData(DATA_ARGALOTH_EVENT, IN_PROGRESS);
        }

        void JustDied(Unit* /*Kill*/)
        {
            if (pInstance)
                pInstance->SetData(DATA_ARGALOTH_EVENT, DONE);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            // pure butchery
            if (berserk)
            {
                DoMeleeAttackIfReady();
                return;
            }

            // Berserk timer
            if (berserk_timer < diff)
            {
                berserk = true;
                me->CastSpell(me, SPELL_BERSERK, true);
            } else berserk_timer -= diff;

            // Stop the rest of AI while channeling
            if (firestorm_progress > diff)
            {
                firestorm_progress -= diff;
                return;
            } else
            {
                firestorm_progress = 0;
                Summons.DespawnAll();
            }

            // Channel Fel Firestorm on 66 and 33 %
            if (update_timer < diff)
            {
                float health_pct = me->GetHealthPct();
                if ((health_pct < 66.0f && firestorm_count < 1)
                    || (health_pct < 33.0f && firestorm_count < 2))
                {
                    me->CastSpell(me, SPELL_FEL_FIRESTORM, false); // spell NEEDS FIX
                    firestorm_progress = 19500;
                    firestorm_count++;
                    update_timer = 1000;
                    return;
                }
                update_timer = 1000;
            } else update_timer -= diff;

            // Meteor Slash timer
            if (meteor_timer < diff)
            {
                me->CastSpell(me, RAID_MODE(SPELL_METEOR_SLASH,H_SPELL_METEOR_SLASH), false);
                meteor_timer = 15000;
            } else meteor_timer -= diff;

            if (me->hasUnitState(UNIT_STAT_CASTING))
                return;

            // Consuming Darknes timer. RaidMode-dependent count of DoTs
            if (darkness_timer < diff)
            {
                for (int i = 0; i < RAID_MODE(3,8); i++) // add check not to afflict the same target (does not stack) ?
                {
                    if (Unit *pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0))
                    {
                        me->CastSpell(pTarget, SPELL_CONSUMING_DARKNESS, false);
                        me->AddAura(SPELL_CONSUMING_DARKNESS, pTarget); // Aura needs to be applied manually
                    }
                }
                darkness_timer = urand(12000,14000);
            } else darkness_timer -= diff;

            // White damage
            DoMeleeAttackIfReady();
        }

        void JustSummoned(Creature* pSummon)
        {
            if (!pSummon)
                return;

            Summons.Summon(pSummon);
        }

        void DoAction(const int32 /*param*/)
        {
            if (Unit *pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0))
                pTarget->CastSpell(pTarget, 88996, true, 0, 0, me->GetGUID());
        }
     };
};

class mob_argaloth_fel_flames: public CreatureScript
{
public:
    mob_argaloth_fel_flames() : CreatureScript("mob_argaloth_fel_flames") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
       return new mob_argaloth_fel_flamesAI(pCreature);
    }
    struct mob_argaloth_fel_flamesAI: public Scripted_NoMovementAI
    {
        mob_argaloth_fel_flamesAI(Creature* pCreature) : Scripted_NoMovementAI(pCreature)
        {
            pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }

        bool fire;
        uint32 aura_timer;

        void InitializeAI() { Scripted_NoMovementAI::InitializeAI(); }

        void Reset()
        {
            fire = false;
            aura_timer = 1000;
        }

        void EnterCombat(Unit* /*pWho*/) { }
        void DamageTaken(Unit* /*pDoneBy*/, uint32 &damage) { damage = 0; }

        void UpdateAI(const uint32 diff)
        {
            if (fire)
                return;

            if (aura_timer < diff)
            {
                me->AddAura(88999, me);
                fire = true;
            } else aura_timer -= diff;
        }
    };
};

/* SQL
UPDATE `creature_template` SET ScriptName='mob_argaloth_fel_flames', scale=0.5, modelid1=16946, faction_A=14, faction_H=14, minlevel=85, maxlevel=85 WHERE entry=47829;
*/

void AddSC_boss_argaloth()
{
    new boss_argaloth();
    new mob_argaloth_fel_flames();
}
