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
        boss_argalothAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            pInstance = pCreature->GetInstanceScript();
        }

        InstanceScript *pInstance;

        bool berserk;
        uint32 berserk_timer;
        uint32 darkness_timer;
        uint32 meteor_timer;
        uint32 update_timer;
        uint32 firestorm_progress;
        uint8 firestorm_count;


        void Reset()
        {
            berserk = false;
            berserk_timer = 300000; // 5 min Enrage Timer
            darkness_timer = 15000;
            meteor_timer = 9000;
            update_timer = 1000;
            firestorm_progress = 0;
            firestorm_count = 0;

            if (pInstance && pInstance->GetData(DATA_ARGALOTH_EVENT) != DONE)
                pInstance->SetData(DATA_ARGALOTH_EVENT, NOT_STARTED);
        }

        void EnterCombat(Unit* /*Ent*/)
        {
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
            } else firestorm_progress = 0;

            // Channel Fel Firestorm on 66 and 33 %
            if (update_timer < diff)
            {
                float health_pct = me->GetHealthPct();
                if ((health_pct < 66.0f && firestorm_count < 1)
                    || (health_pct < 33.0f && firestorm_count < 2))
                {
                    me->CastSpell(me, SPELL_FEL_FIRESTORM, false); // spell NEEDS FIX
                    firestorm_progress = 18000;
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
     };
};

void AddSC_boss_argaloth()
{
    new boss_argaloth();
}
