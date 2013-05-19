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

enum stuffs
{
    SPELL_ARCING_SLASH = 99931,
    SPELL_IMMOLATION_TRAP = 99838, // triggered spell by trap?
    SPELL_IMMOLATION_TRAP_SUMMON = 52606,
    SPELL_CRYSTAL_PRISON_EFFECT = 99837,
    SPELL_CRYSTAL_PRISON_TRAP_SUMMON = 99836, // summon NPC (trap) - need AI!!!
    SPELL_HURL_SPEAR  = 100002,
    SPELL_FRENZY = 100522,
    SPELL_MAGMA_RUPTURE = 99840, // to do: add radius index for 45 yd.
    SPELL_LIMB_RIP = 99832,
    SPELL_FRENZIED_DEVOTION = 100064,
    SPELL_FEEDING_FRENZY = 100655,
    SPELL_FACE_RAGE = 99945,
    SPELL_BERSERK = 26662,

    NPC_CRYSTAL_TRAP = 53713,
    NPC_CRYSTAL_PRISON = 53819,
    NPC_IMMOLATION_TRAP = 53724,
    NPC_RAGEFACE = 53695,
    NPC_RIPLIMB = 53694,
};

class boss_shannox : public CreatureScript
{
    public:
        boss_shannox() : CreatureScript("boss_shannox") { }

    struct boss_shannoxAI : public ScriptedAI
    {
        boss_shannoxAI(Creature* c) : ScriptedAI(c)
        {
            if (c->GetInstanceScript())
                pInstance = c->GetInstanceScript();
        }

        Creature* pRiblimb;
        std::list<uint64> summonedTraps;
        Creature* pRageface;
        InstanceScript* pInstance;
        uint32 ArcingSlashTimer;
        uint32 CrystalTrapTimer;

        void Reset()
        {
            pRiblimb = NULL;
            pRageface = NULL;
            ArcingSlashTimer = 7000;
            CrystalTrapTimer = 8000;

            me->SetReactState(REACT_PASSIVE);

            DespawnTraps();
            summonedTraps.clear();
        }

        void DespawnTraps()
        {
        
            if (!summonedTraps.empty())
            {
                for (std::list<uint64>::const_iterator itr = summonedTraps.begin(); itr != summonedTraps.end(); itr++)
                {
                    Creature* pTraps = NULL;

                    if ((pTraps = me->GetMap()->GetCreature((*itr))) != NULL)
                        pTraps->ForcedDespawn(5000);
                }
            }
        }

        void JustSummoned(Creature* pSummon)
        {
            switch (pSummon->GetEntry())
            {
                case NPC_CRYSTAL_TRAP:
                case NPC_IMMOLATION_TRAP:
                    summonedTraps.push_back(pSummon->GetGUID());
                    break;
                default:
                    break;
            }
        }

        void EnterCombat(Unit* pWho)
        {
            if (pInstance)
            {
                if (pRiblimb == NULL)
                    pRiblimb = me->GetCreature(*me, pInstance->GetData64(DATA_RIPLIMB_GUID));

                if (pRageface == NULL)
                    pRageface = me->GetCreature(*me, pInstance->GetData64(DATA_RAGEFACE_GUID));
            }
            me->SetReactState(REACT_AGGRESSIVE);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (ArcingSlashTimer <= diff)
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    me->CastSpell(me->getVictim(), SPELL_ARCING_SLASH, false);
                    ArcingSlashTimer = 11000;
                }
            } else ArcingSlashTimer -= diff;

            if (CrystalTrapTimer <= diff)
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0);

                    if (pTarget)
                        me->CastSpell(pTarget, SPELL_CRYSTAL_PRISON_TRAP_SUMMON, false);

                    CrystalTrapTimer = 12000;
                }
            } else CrystalTrapTimer -= diff;

            DoMeleeAttackIfReady();

        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new boss_shannoxAI(c);
    }
};

class npc_crystal_trap : public CreatureScript
{
    public:
        npc_crystal_trap() : CreatureScript("npc_crystal_trap") { }

    struct npc_crystal_trapAI : public ScriptedAI
    {
        npc_crystal_trapAI(Creature* c) : ScriptedAI(c)
        {
            c->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
            c->setFaction(35);
        }

        bool armTrap;
        bool trapActivated;
        uint32 armtrapTimer;
        uint64 prisonerGuid;
        Creature* pBoss;

        void Reset()
        {
            armTrap = false;
            trapActivated = false;
            prisonerGuid = 0;
            pBoss = NULL;

            if (pBoss == NULL)
                pBoss = Unit::GetCreature(*me, me->GetInstanceScript()->GetData64(TYPE_SHANNOX));

            if (pBoss)
                armtrapTimer = 2000 + uint32(me->GetDistance2d(pBoss) / 100) * 1000;

        }

        void MoveInLineOfSight(Unit* pWho)
        {
            if (!pWho || pWho->GetDistance(me) > 0.3f || armTrap == false || trapActivated == true)
                return;

            if (pWho->GetTypeId() == TYPEID_PLAYER || (pWho->GetCharmerOrOwnerPlayerOrPlayerItself() == NULL
                && (pWho->GetEntry() == NPC_RIPLIMB || pWho->GetEntry() == NPC_RAGEFACE)))
                ActivateTrap(pWho);

            ScriptedAI::MoveInLineOfSight(pWho);
        }

        void ActivateTrap(Unit* pWho)
        {
            if (pWho)
            {
                prisonerGuid = pWho->GetGUID();
                trapActivated = true;

                if (pBoss)
                {
                    pBoss->CastSpell(pWho, SPELL_CRYSTAL_PRISON_EFFECT, false);
                    float x,y,z;
                    pWho->GetPosition(x, y, z);
                    pWho->NearTeleportTo(x,y,z,0,false);
                    me->SummonCreature(NPC_CRYSTAL_PRISON, x, y, z, 0, TEMPSUMMON_CORPSE_DESPAWN);
                    DoDestroyForPlayer(pWho);
                }

                me->SetVisibility(VISIBILITY_OFF);
            }
        }

        void DoDestroyForPlayer(Unit* pWho)
        {
            Map::PlayerList const& plrList = me->GetMap()->GetPlayers();

            if (!plrList.isEmpty())
            {
                for (Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
                {
                    pWho->DestroyForPlayer(itr->getSource(), true);
                }
            }
        }

        Unit* GetPrisoner()
        {
            if (Unit* prisoner = Unit::GetUnit(*me, prisonerGuid))
                return prisoner;
            else
                return NULL;
        }

        void DoAction(const int32 id)
        {
            if (id == 1)
            {
                if (prisonerGuid != 0)
                {
                    if (GetPrisoner())
                    {
                        GetPrisoner()->RemoveAurasDueToSpell(SPELL_CRYSTAL_PRISON_EFFECT);
                        me->ForcedDespawn();
                    }
                }
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (armTrap == false && trapActivated == false) // done
            {
                if (armtrapTimer <= diff)
                {
                    armTrap = true;
                    armtrapTimer = 2000;
                } else armtrapTimer -= diff;
            }
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new npc_crystal_trapAI(c);
    }
};

class npc_crystal_prison : public CreatureScript
{
    public:
        npc_crystal_prison() : CreatureScript("npc_crystal_prison") { }

    struct npc_crystal_prisonAI : public ScriptedAI
    {
        npc_crystal_prisonAI(Creature* c) : ScriptedAI(c)
        {
            c->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            c->SetReactState(REACT_PASSIVE);
            summonerGuid = 0;
        }

        uint32 selfKillTimer;
        uint64 summonerGuid;

        void Reset()
        {
            selfKillTimer = 1000;
        }

        void IsSummonedBy(Unit* pSummoner)
        {
            if (pSummoner)
                summonerGuid = pSummoner->GetGUID();
        }

        void UpdateAI(const uint32 diff)
        {
            if (Creature* pTrap = Unit::GetCreature(*me, summonerGuid))
            {
                if (((npc_crystal_trap::npc_crystal_trapAI*)pTrap->GetAI())->GetPrisoner())
                {
                    if (!((npc_crystal_trap::npc_crystal_trapAI*)pTrap->GetAI())->GetPrisoner()->isAlive())
                        me->Kill(me);

                    if (((npc_crystal_trap::npc_crystal_trapAI*)pTrap->GetAI())->GetPrisoner()->GetTypeId() == TYPEID_UNIT &&
                        (((npc_crystal_trap::npc_crystal_trapAI*)pTrap->GetAI())->GetPrisoner()->GetEntry() == NPC_RIPLIMB ||
                        ((npc_crystal_trap::npc_crystal_trapAI*)pTrap->GetAI())->GetPrisoner()->GetEntry() == NPC_RAGEFACE))
                    {
                        if (selfKillTimer <= diff)
                        {
                            me->DealDamage(me, me->CountPctFromMaxHealth(10), 0, DIRECT_DAMAGE);
                            selfKillTimer = 1000;
                        } else selfKillTimer -= diff;
                    }
                }

            }
        }

        void JustDied(Unit* pKiller)
        {
            if (Creature* pTrap = Unit::GetCreature(*me, summonerGuid))
            {
                pTrap->AI()->DoAction(1);
                me->ForcedDespawn();
                summonerGuid = 0;
            }
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new npc_crystal_prisonAI(c);
    }
};

void AddSC_boss_shannox()
{
    new boss_shannox();
    new npc_crystal_trap();
    new npc_crystal_prison();
}
