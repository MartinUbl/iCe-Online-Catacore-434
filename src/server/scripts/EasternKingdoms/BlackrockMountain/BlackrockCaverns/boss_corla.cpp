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
#include "blackrock_caverns.h"

enum Spells
{
    SPELL_AURA_OF_ACCELERATION      = 75817,
    SPELL_AURA_OF_ACCELERATION_H    = 87376,
    SPELL_DARK_COMMAND              = 75823,
    SPELL_DARK_COMMAND_H            = 93462,
    SPELL_CORLA_VISUAL              = 87655,
    SPELL_CORLA_VISUAL_CASTING      = 75645,
    SPELL_DUMMY_VISUAL              = 75649,

    SPELL_EVOLUTION                 = 75697,
    SPELL_EVOLUTION_H               = 87378,
    SPELL_TWILIGHT_EVOLUTION        = 75732,
    SPELL_KNEELING_IN_SUPPLICATION  = 75608,
    SPELL_ZEALOT_VISUAL             = 95564,
    SPELL_FORCE_BLAST               = 76522,
    SPELL_FORCE_BLAST_H             = 93648,
    SPELL_GRAVITY_STRIKE            = 76561,
    SPELL_GRAVITY_STRIKE_H          = 93656,
    SPELL_SHADOW_STRIKE             = 82362,
    SPELL_SHADOW_STRIKE_H           = 87374,
    SPELL_GRIEVOUS_WHIRL            = 76524,
    SPELL_GRIEVOUS_WHIRL_H          = 93658
};

#define DUMMY_X 573.64f
#define DUMMY_Y 904.49f
#define DUMMY_Z 178.27f

class boss_corla: public CreatureScript
{
public:
    boss_corla(): CreatureScript("boss_corla") {}

    struct corlaAI: public ScriptedAI
    {
        corlaAI(Creature* pCreature): ScriptedAI(pCreature)
        {
            pInstance = me->GetInstanceScript();
            me->CastSpell(me, SPELL_CORLA_VISUAL_CASTING, true);
        }

        InstanceScript* pInstance;
        uint64 dummyGUID;
        uint32 m_uiBeamVisual_Timer;
        uint32 m_uiDarkCommand_Timer;

        void Reset()
        {
            dummyGUID = 0;
            Creature* pDummy = me->SummonTrigger(DUMMY_X, DUMMY_Y, DUMMY_Z, 0, 0, 0); // Prepare dummy for the beam
            if(!pDummy)
                return;

            dummyGUID = pDummy->GetGUID();
            pDummy->SetFlying(true);
            pDummy->GetMotionMaster()->MovePoint(0, DUMMY_X, DUMMY_Y, DUMMY_Z);
            pDummy->CastSpell(pDummy, SPELL_DUMMY_VISUAL, true);
            pDummy->CastSpell(me, SPELL_CORLA_VISUAL, true);

            m_uiBeamVisual_Timer = 30000;
            m_uiDarkCommand_Timer = 15000;
        }

        void EnterCombat(Unit* who)
        {
            me->RemoveAurasDueToSpell(SPELL_CORLA_VISUAL_CASTING);
            me->CastSpell(me, DUNGEON_MODE(SPELL_AURA_OF_ACCELERATION, SPELL_AURA_OF_ACCELERATION_H), true);

            // Remove dummy on EnterCombat
            if(dummyGUID)
                if(Creature* pDummy = me->GetCreature(*me, dummyGUID))
                    pDummy->ForcedDespawn(0);

            // Activate Zealots
            uint64 ZealotGUID;
            for(int i = 0; i < 3; i++)
            {
                ZealotGUID = 0;
                if((ZealotGUID = pInstance->GetData64(DATA_CORLA_ZEALOT_1+i)) != 0)
                {
                    Creature* pZealot = me->GetCreature(*me, ZealotGUID);
                    if(pZealot)
                        pZealot->CastSpell(pZealot, SPELL_KNEELING_IN_SUPPLICATION, true);
                }
            }
        }

        void EnterEvadeMode() // Wipe cleanup
        {
            CreatureAI::EnterEvadeMode();
            ClearPlayers(false);
        }

        void JustReachedHome()
        {
            me->CastSpell(me, SPELL_CORLA_VISUAL_CASTING, true);

            if(!pInstance)
                return;

            // Reset Zealots
            uint64 ZealotGUID;
            for(int i = 0; i < 3; i++)
            {
                ZealotGUID = 0;
                if((ZealotGUID = pInstance->GetData64(DATA_CORLA_ZEALOT_1+i)) != 0)
                {
                    Creature* pZealot = me->GetCreature(*me, ZealotGUID);
                    if(pZealot)
                    {
                        pZealot->Respawn();
                        pZealot->AI()->EnterEvadeMode();
                    }
                }
            }
        }

        void JustDied(Unit* /*pKiller*/)
        {
            if(!pInstance)
                return;

            // Achievement when three Evolved Twilight Zealots have been killed
            bool achiev = DUNGEON_MODE(false,true); // Heroic only

            // Kill Zealots
            uint64 ZealotGUID;
            for(int i = 0; i < 3; i++)
            {
                ZealotGUID = 0;
                if((ZealotGUID = pInstance->GetData64(DATA_CORLA_ZEALOT_1+i)) != 0)
                {
                    Creature* pZealot = me->GetCreature(*me, ZealotGUID);
                    if(pZealot)
                    {
                        if(pZealot->isAlive())
                        {
                            achiev = false;
                            pZealot->Kill(pZealot);
                        }
                        pZealot->ForcedDespawn();
                    }
                }
            }
            ClearPlayers(achiev);
        }

        void UpdateAI(const uint32 diff)
        {
            if(!UpdateVictim()) // Update Beam OoC only
            {
                if(m_uiBeamVisual_Timer < diff)
                {
                    if(dummyGUID)
                        if(Creature* pDummy = me->GetCreature(*me, dummyGUID))
                            pDummy->CastSpell(me, SPELL_CORLA_VISUAL, true);

                    m_uiBeamVisual_Timer = 30000;
                } else m_uiBeamVisual_Timer -= diff;
                return;
            }

            if(m_uiDarkCommand_Timer < diff)
            {
                Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0);
                if(pTarget)
                    me->CastSpell(pTarget, DUNGEON_MODE(SPELL_DARK_COMMAND, SPELL_DARK_COMMAND_H), false);

                m_uiDarkCommand_Timer = 12000;
            } else m_uiDarkCommand_Timer -= diff;

            DoMeleeAttackIfReady();
        }

        void ClearPlayers(bool achiev) // Remove MC debuff from players
        {
            if(!pInstance)
                return;

            Map::PlayerList const& plList = pInstance->instance->GetPlayers();
            if (!plList.isEmpty())
            {
                for(Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
                {
                    if(Player* pPlayer = itr->getSource())
                    {
                        pPlayer->RemoveAurasDueToSpell(SPELL_TWILIGHT_EVOLUTION);
                        // Give achievement Arrested Development
                        if(achiev)
                            pPlayer->GetAchievementMgr().CompletedAchievement(sAchievementStore.LookupEntry(5282));
                    }
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new corlaAI(c);
    }
};

// Twilight Zealot AI
class mob_twilight_zealot_corla : public CreatureScript
{
public:
    mob_twilight_zealot_corla() : CreatureScript("mob_twilight_zealot_corla") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_twilight_zealot_corlaAI(pCreature);
    }

    struct mob_twilight_zealot_corlaAI : public ScriptedAI
    {
        mob_twilight_zealot_corlaAI(Creature *pCreature) : ScriptedAI(pCreature)
        {
            pInstance = pCreature->GetInstanceScript();
        }

        InstanceScript* pInstance;
        Unit* pBeamTarget;
        uint64 m_uiDummyGUID;
        uint32 m_uiBeam_Timer;
        uint32 m_uiEvolution_Timer;
        uint32 m_uiAbility_Timer;
        bool m_bKneeling;

        void Reset()
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

            pBeamTarget = me->ToUnit();
            m_uiDummyGUID = 0;
            Creature* pDummy = me->SummonTrigger(DUMMY_X, DUMMY_Y, DUMMY_Z, 0, 0, 0); // Prepare beam dummy
            if(!pDummy)
                return;

            m_uiDummyGUID = pDummy->GetGUID();
            pDummy->SetFlying(true);
            pDummy->GetMotionMaster()->MovePoint(0, DUMMY_X, DUMMY_Y, DUMMY_Z);
            pDummy->CastSpell(pDummy, SPELL_DUMMY_VISUAL, true);

            m_uiBeam_Timer = 30000;
            m_uiAbility_Timer = 6000;
            m_uiEvolution_Timer = 300;
            m_bKneeling = false;
        }

        void EnterEvadeMode()
        {
            if(m_uiDummyGUID)
                if(Creature* pDummy = me->GetCreature(*me, m_uiDummyGUID))
                    pDummy->ForcedDespawn();

            me->RemoveAurasDueToSpell(SPELL_KNEELING_IN_SUPPLICATION);
            me->RemoveAurasDueToSpell(SPELL_TWILIGHT_EVOLUTION);

            CreatureAI::EnterEvadeMode();
        }

        void JustDied(Unit* /*pKiller*/)
        {
            me->RemoveAurasDueToSpell(SPELL_KNEELING_IN_SUPPLICATION);
            if(m_uiDummyGUID)
                if(Creature* pDummy = me->GetCreature(*me, m_uiDummyGUID))
                    pDummy->ForcedDespawn();
        }

        void SpellHit(Unit* /*pCaster*/, const SpellEntry* spellInfo)
        {
            // Activate me when hit by Corla's Kneeling in Supplication
            if(spellInfo && (spellInfo->Id == SPELL_KNEELING_IN_SUPPLICATION))
            {
                me->SetReactState(REACT_AGGRESSIVE);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

                m_bKneeling = true;

                // Activate beam
                if(m_uiDummyGUID && pBeamTarget)
                    if(Creature* pDummy = me->GetCreature(*me, m_uiDummyGUID))
                        pDummy->CastSpell(pBeamTarget, SPELL_ZEALOT_VISUAL, true);
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if(m_bKneeling)
            {

                if(UpdateBeam()) // Beam has been updatetd along with new target
                    m_uiBeam_Timer = 30000;

                if(m_uiBeam_Timer < diff)
                {
                    if(m_uiDummyGUID && pBeamTarget)
                        if(Creature* pDummy = me->GetCreature(*me, m_uiDummyGUID))
                            pDummy->CastSpell(pBeamTarget, SPELL_ZEALOT_VISUAL, true);

                    m_uiBeam_Timer = 30000;
                } else m_uiBeam_Timer -= diff;

                if(m_uiEvolution_Timer < diff)
                {
                    if(pBeamTarget)
                    {
                        // Add evolution aura Charge to preselected target (target of the beam)
                        Aura* aura = pBeamTarget->GetAura(SPELL_EVOLUTION);
                        if(!aura)
                            aura = pBeamTarget->GetAura(SPELL_EVOLUTION_H);

                        if(aura)
                        {
                            aura->SetCharges(aura->GetCharges() + 1);
                            aura->RefreshDuration();

                            // Twilight Evolution after taking 100 charges
                            if(aura->GetCharges() == 100)
                            {
                                pBeamTarget->RemoveAurasDueToSpell(SPELL_EVOLUTION);
                                // Is me evolved ?
                                if(pBeamTarget->ToCreature())
                                {
                                    m_bKneeling = false;
                                    me->CastSpell(pBeamTarget, SPELL_TWILIGHT_EVOLUTION, true);
                                    me->RemoveAurasDueToSpell(SPELL_KNEELING_IN_SUPPLICATION);

                                    // Deactivate beam
                                    if(m_uiDummyGUID)
                                        if(Creature* pDummy = me->GetCreature(*me, m_uiDummyGUID))
                                            pDummy->ForcedDespawn();
                                }
                                // Is a player evolved
                                else
                                {
                                    // Corla will put on Charm effect
                                    if(pInstance)
                                        if(Creature* pCorla = me->GetCreature(*me, pInstance->GetData64(DATA_CORLA)))
                                            pCorla->CastSpell(pBeamTarget, SPELL_TWILIGHT_EVOLUTION, true);
                                }
                            }
                        }
                        else pBeamTarget->CastSpell(pBeamTarget, SPELL_EVOLUTION, true);
                    }

                    int32 new_timer = 200 - (diff - m_uiEvolution_Timer); // Should not get delayed
                    m_uiEvolution_Timer = (new_timer > 0) ? (uint32)new_timer : 0;
                } else m_uiEvolution_Timer -= diff;

                return;
            }

            if(!UpdateVictim())
                return;

            // Evolved Zealot's abilities
            if(m_uiAbility_Timer < diff)
            {
                Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0);
                switch(urand(1,6))
                {
                case 1:
                    if(pTarget)
                        me->CastSpell(pTarget, DUNGEON_MODE(SPELL_FORCE_BLAST, SPELL_FORCE_BLAST_H), false);
                    break;
                case 2:
                    if(pTarget)
                        me->CastSpell(pTarget, DUNGEON_MODE(SPELL_SHADOW_STRIKE, SPELL_SHADOW_STRIKE_H), false);
                    break;
                case 3:
                case 4:
                case 5:
                    me->CastSpell(me->getVictim(), DUNGEON_MODE(SPELL_GRAVITY_STRIKE, SPELL_GRAVITY_STRIKE_H), false);
                    break;
                case 6:
                    me->CastSpell(me, DUNGEON_MODE(SPELL_GRIEVOUS_WHIRL, SPELL_GRIEVOUS_WHIRL_H), false);
                    break;
                default:
                    break;
                }
                m_uiAbility_Timer = 9000;
            } else m_uiAbility_Timer -= diff;

            DoMeleeAttackIfReady();
        }

        bool UpdateBeam() // Select correct target for the beam
        {
            if(!pInstance)
                return false;

            Unit* pNewTarget = NULL;
            if(m_uiDummyGUID)
            {
                if(Creature* pDummy = me->GetCreature(*me, m_uiDummyGUID))
                {
                    Map::PlayerList const& plList = pInstance->instance->GetPlayers();
                    if (!plList.isEmpty())
                    {
                        float angle = me->GetAngle(pDummy->GetPositionX(), pDummy->GetPositionY());
                        for(Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
                        {
                            Unit* pUnit;
                            pUnit = itr->getSource()->ToUnit();
                            if(pUnit->HasAura(SPELL_TWILIGHT_EVOLUTION)) // Player is charmed
                                break;

                            float ang = angle - me->GetAngle(pUnit->GetPositionX(), pUnit->GetPositionY());
                            ang < 0 ? ang = -ang : ang = ang;
                            if(me->GetDistance(pUnit) < 3.00f // Player is near enough
                                && ang < (3.14f / 6.00f)) // Player stands in the way of the beam
                            {
                                // The nearest player to be selected
                                if(pNewTarget)
                                {
                                    if(me->GetDistance(pUnit) < me->GetDistance(pNewTarget))
                                        pNewTarget = pUnit;
                                }
                                else pNewTarget = pUnit;
                            }
                        }
                    }
                    if(!pNewTarget && !pBeamTarget->ToCreature())
                    {
                        // old target was a player but now there is no player matching conditions. targetting self
                        pNewTarget = me->ToUnit();
                    }
                    if(pNewTarget && pNewTarget != pBeamTarget)
                    {
                        // Target has changed, need update
                        pBeamTarget = pNewTarget;
                        pDummy->CastSpell(pBeamTarget, SPELL_ZEALOT_VISUAL,  true);
                        return true;
                    }
                }
            }
            // Not updated
            return false;
        }
    };

};

void AddSC_corla()
{
    new boss_corla();
    new mob_twilight_zealot_corla();
}
