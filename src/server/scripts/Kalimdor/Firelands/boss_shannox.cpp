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
    NPC_CRYSTAL_TRAP                = 53713,
    NPC_CRYSTAL_PRISON              = 53819,
    NPC_IMMOLATION_TRAP             = 53724,
    NPC_IMMOLATION_TRAP_BUNNY       = 537240,
    NPC_RAGEFACE                    = 53695,
    NPC_RIPLIMB                     = 53694,
    NPC_HURL_SPEAR_WEAPON           = 53752,
};

enum Spells
{
    SPELL_ARCING_SLASH              = 99931,
    SPELL_IMMOLATION_TRAP           = 99838,
    SPELL_IMMOLATION_TRAP_SUMMON    = 99839,
    SPELL_SPEAR_TARGET_VISUAL       = 99988,
    SPELL_MAGMA_RUPTURE_IMMOLATION  = 99841,
    SPELL_MAGMA_FLARE_VISUAL        = 100495,
    SPELL_MAGMA_RUPTURE             = 99840, // to do: add radius index for 45 yd.
    SPELL_CRYSTAL_PRISON_EFFECT     = 99837,
    SPELL_CRYSTAL_PRISON_TRAP_SUMMON = 99836,
    SPELL_HURL_SPEAR                = 100002,
    SPELL_SUMMON_SPEAR              = 99978,
    SPELL_SPEAR_VISUAL              = 100026,
    SPELL_FRENZY                    = 100522,
    SPELL_LIMB_RIP                  = 99832,
    SPELL_FRENZIED_DEVOTION         = 100064,
    SPELL_FEEDING_FRENZY            = 100655,
    SPELL_FACE_RAGE                 = 99945,
    SPELL_BERSERK                   = 26662,
    SPELL_DOGGED_DETERMINATION      = 101111,
    SPELL_WARY                      = 100167,
    SPELL_SEPARATION_ANXIETY        = 99835,
};

# define NEVER  (4294967295) // used as "delayed" timer ( max uint32 value)

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

                Reset();
            }

            Creature* pRiplimb;
            Creature* pSummonSpear;
            std::list<uint64> summonedTraps;
            Creature* pRageface;
            InstanceScript* pInstance;
            uint32 ArcingSlashTimer;
            uint32 CrystalTrapTimer;
            uint32 ImmolationTrapTimer;
            uint32 HurlSpearTimer;
            Position SpearPos;
            uint32 separationCheckTimer;
            uint32 aggroTime;
            uint32 riplimbRespawnTimer;
            uint32 enrageTimer;
            bool aggroBool;

            void Reset()
            {
                pRiplimb = NULL;
                pRageface = NULL;

                aggroTime = 500;
                aggroBool = false;

                separationCheckTimer = 1000;
                ArcingSlashTimer = 7000;
                CrystalTrapTimer = 8000;
                ImmolationTrapTimer = 10000;
                HurlSpearTimer = 20000;
                riplimbRespawnTimer = 30000;
                enrageTimer = 600 * IN_MILLISECONDS; // 10 minutes enrage timer

                pSummonSpear = NULL;

                me->SetReactState(REACT_PASSIVE);

                DespawnTraps();
                summonedTraps.clear();

                if (pInstance)
                {
                    if(pInstance->GetData(TYPE_SHANNOX)!=DONE)
                        pInstance->SetData(TYPE_SHANNOX, NOT_STARTED);
                    pRiplimb = me->GetCreature((*me), pInstance->GetData64(DATA_RIPLIMB_GUID));
                    pRageface = me->GetCreature((*me), pInstance->GetData64(DATA_RAGEFACE_GUID));
                }

                SetEquipmentSlots(false, 71557, EQUIP_NO_CHANGE, EQUIP_NO_CHANGE);
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

                std::list<Creature*> cr;
                cr.clear();
                GetCreatureListWithEntryInGrid(cr, me, NPC_CRYSTAL_PRISON, 50000.0f);

                if (!cr.empty())
                {
                    for (std::list<Creature*>::const_iterator itr = cr.begin(); itr != cr.end(); itr++)
                        me->Kill((*itr)); // we must kill'em, because we don't know if we have prisoner.
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
                    case NPC_HURL_SPEAR_WEAPON: // Hurl Spear weapon
                    {
                        SetEquipmentSlots(false, 0, EQUIP_NO_CHANGE, EQUIP_NO_CHANGE);
                        pSummon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
                        pSummon->setFaction(35);
                        summonedTraps.push_back(pSummon->GetGUID());
                        break;
                    }
                    default:
                        break;
                }
            }

            void SummonedCreatureDespawn(Creature* pSummon)
            {
                switch (pSummon->GetEntry())
                {
                    case NPC_HURL_SPEAR_WEAPON:
                    {
                        if (!summonedTraps.empty())
                        {
                            for (std::list<uint64>::const_iterator itr = summonedTraps.begin(); itr != summonedTraps.end(); itr++)
                            {
                                Creature* bunny = NULL;

                                if ((bunny = me->GetMap()->GetCreature((*itr))) != NULL)
                                    if (bunny->GetEntry() == NPC_IMMOLATION_TRAP_BUNNY)
                                        bunny->ForcedDespawn();
                            }
                        }
                    }
                        break;
                }
            }

            void JustDied(Unit* killer)
            {
                if (pInstance)
                    pInstance->SetData(TYPE_SHANNOX, DONE);

                DespawnTraps();
                summonedTraps.clear();

                DoYell("The pain... Lord of fire, it hurts...", 24568);
            }

            void KilledUnit(Unit* pTarget)
            {
                if (pTarget->GetTypeId() == TYPEID_PLAYER)
                {
                    switch (urand(0,3))
                    {
                        case 0:
                            DoYell("Yes... oh yes!", 24581);
                            break;
                        case 1:
                            DoYell("The Firelord will be most pleased!", 24580);
                            break;
                        case 2:
                            DoYell("Now you stay dead!", 24579);
                            break;
                        case 3:
                            DoYell("Dog food!", 24578);
                            break;
                    }
                }
            }

            void DoYell(const char* text, uint32 soundid)
            {
                if (text)
                    me->MonsterYell(text, LANG_UNIVERSAL, 0);

                if (soundid)
                    me->PlayDirectSound(soundid);
            }

            void EnterCombat(Unit* pWho)
            {
                DoYell("Aha! The interlopers... Kill them! EAT THEM!", 24565);
                if (pInstance)
                {
                    pInstance->SetData(TYPE_SHANNOX, IN_PROGRESS);

                    pRiplimb = me->GetCreature((*me), pInstance->GetData64(DATA_RIPLIMB_GUID));
                    pRageface = me->GetCreature((*me), pInstance->GetData64(DATA_RAGEFACE_GUID));

                    if (pRiplimb && !pRiplimb->isAlive())
                    {
                        pRiplimb->Respawn();
                        pRiplimb->RemoveAllAuras();
                    }

                    if (pRageface && !pRiplimb->isAlive())
                    {
                        pRageface->Respawn();
                        pRiplimb->RemoveAllAuras();
                    }
                }

                me->SetReactState(REACT_AGGRESSIVE);
            }

            void EnterEvadeMode()
            {
                if (pRiplimb && !pRiplimb->isAlive())
                {
                    pRiplimb->setDeathState(DEAD);
                    pRiplimb->Respawn(true);
                    pRiplimb->RemoveAllAuras();
                }

                if (pRageface && !pRageface->isAlive())
                {
                    pRageface->setDeathState(DEAD);
                    pRageface->Respawn(true);
                    pRiplimb->RemoveAllAuras();
                }

                ScriptedAI::EnterEvadeMode();
            }

            void DoAction(const int32 action)
            {
                // Riblimb death
                if (action == 1)
                {
                    SetEquipmentSlots(false, 71557, EQUIP_NO_CHANGE, EQUIP_NO_CHANGE);
                    me->CastSpell(me, SPELL_FRENZY, false);

                    if (IsHeroic())
                    {
                        me->MonsterTextEmote("Riplimb collapses into a smouldering heap...",0,true);
                        riplimbRespawnTimer = 30000;
                    }
                    else
                        DoYell("Riplimb! No... no! Oh, you terrible little beasts! HOW COULD YOU?!", 24574);
                }
                // Rageface death
                if (action == 2)
                {
                    me->CastSpell(me, SPELL_FRENZY, false);
                    DoYell("You murderers! Why... why would you kill such a noble animal?!", 24575);
                }

                // equip spear
                if (action == 4)
                    SetEquipmentSlots(false, 71557, EQUIP_NO_CHANGE, EQUIP_NO_CHANGE);
            }

            void SpellHit(Unit* pTarget, const SpellEntry* spell)
            {
                if (!spell)
                    return;

                for (uint32 i = 0; i < 3; i++)
                {
                    if(spell->Effect[i] == SPELL_EFFECT_ATTACK_ME)
                    {
                        // Taunt sounds..why not use..
                        switch (urand(0, 3))
                        {
                            case 0:
                                me->PlayDirectSound(24595);
                                break;
                            case 1:
                                me->PlayDirectSound(24594);
                                break;
                            case 2:
                                me->PlayDirectSound(24593);
                                break;
                            case 3:
                                me->PlayDirectSound(24592);
                                break;
                        }
                    }
                }
            }

            void SpellHitTarget(Unit* pTarget, const SpellEntry* spell)
            {
                if (!spell)
                    return;

                if (spell->Id == SPELL_MAGMA_RUPTURE)
                {
                    if (pTarget != me)
                        return;

                    float angle, x, y, z, cx, cy;
                    pTarget->GetPosition(cx,cy,z);

                    for (uint32 i = 35; i < 120; i += 1)
                    {
                        angle = i * 0.15f;

                        x = cx + (0 + 1.5f * angle) * cos(angle);
                        y = cy + (0 + 1.5f * angle) * sin(angle);
                        z = me->GetMap()->GetHeight(x, y, z + 5.0f);
                        me->CastSpell(x,y,z, SPELL_MAGMA_RUPTURE_IMMOLATION, true);
                    }
                }

                if (spell->Id == SPELL_HURL_SPEAR)
                {
                    if (pSummonSpear)
                        pSummonSpear->SetVisibility(VISIBILITY_ON);

                    if (pRiplimb)
                        pRiplimb->AI()->DoAction(1);

                    Creature* pSummon = me->SummonCreature(NPC_IMMOLATION_TRAP_BUNNY, SpearPos.GetPositionX(), SpearPos.GetPositionY(), SpearPos.GetPositionZ());
                    if (pSummon)
                    {
                        pSummon->SetVisibility(VISIBILITY_ON);
                        pSummon->CastSpell(pSummon, SPELL_MAGMA_FLARE_VISUAL, true);
                        pSummon->setFaction(35);
                        float x, y, z, angle, xc, yc;
                        pSummon->GetPosition(xc, yc, z);

                        uint32 i;
                        for (i = 0; i < 24; i += 1)
                        {
                            angle = i * 0.4f;

                            x = xc + (1 + 2.0f * angle) * cos(angle);
                            y = yc + (1 + 2.0f * angle) * sin(angle);
                            z = me->GetMap()->GetHeight(x, y, z + 5.0f);
                            me->CastSpell(x,y,z, SPELL_MAGMA_RUPTURE_IMMOLATION, true);
                        }

                        for (i = 48; i < 140; i += 1)
                        {
                            angle = i * 0.15f;

                            x = xc + (1 + 2.0f * angle) * cos(angle);
                            y = yc + (1 + 2.0f * angle) * sin(angle);
                            z = me->GetMap()->GetHeight(x, y, z + 5.0f);
                            me->CastSpell(x,y,z, SPELL_MAGMA_RUPTURE_IMMOLATION, true);
                        }
                        pSummon->ForcedDespawn(1000);
                    }
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!me->isInCombat())
                {
                    if (pRageface && pRageface->getVictim())
                        AttackStart(pRageface->getVictim());

                    if (pRiplimb && pRiplimb->getVictim())
                        AttackStart(pRiplimb->getVictim());
                }

                if (!UpdateVictim())
                    return;

                if (!aggroBool)
                {
                    if (aggroTime <= diff)
                    {
                        if (pRiplimb)
                        {
                            if (Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0))
                            {
                                pRiplimb->SetReactState(REACT_AGGRESSIVE);
                                pRiplimb->AI()->AttackStart(pTarget);
                                pRiplimb->GetMotionMaster()->Clear(false);
                                pRiplimb->GetMotionMaster()->MoveChase(pTarget);
                            }
                        }

                        if (pRageface)
                        {
                            if (Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0))
                            {
                                pRageface->AddThreat(pTarget, 50000.0f);
                                pRageface->AI()->AttackStart(pTarget);
                                pRageface->GetMotionMaster()->Clear(false);
                                pRageface->GetMotionMaster()->MoveChase(pTarget);
                            }
                        }
                        aggroTime = 500;
                        aggroBool = true;
                    }
                    else
                        aggroTime -= diff;
                }

                if (enrageTimer <= diff)
                {
                    me->CastSpell(me,SPELL_BERSERK,true);
                    enrageTimer = 999999999;
                }
                else enrageTimer -= diff;

                if (ArcingSlashTimer <= diff)
                {
                    if (!me->IsNonMeleeSpellCasted(false))
                    {
                        if(me->GetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID) == 71557) // Only if Shannox has spear equiped (in main hand)
                            me->CastSpell(me->getVictim(), SPELL_ARCING_SLASH, false);
                        ArcingSlashTimer = 11000;
                    }
                }
                else
                    ArcingSlashTimer -= diff;

                if (CrystalTrapTimer <= diff)
                {
                    if (!me->IsNonMeleeSpellCasted(false))
                    {
                        Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 1000, true);

                        if (pTarget)
                            me->CastSpell(pTarget, SPELL_CRYSTAL_PRISON_TRAP_SUMMON, false);

                        CrystalTrapTimer = 25500;
                    }
                }
                else
                    CrystalTrapTimer -= diff;

                if (ImmolationTrapTimer <= diff)
                {
                    if (!me->IsNonMeleeSpellCasted(false))
                    {
                        Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 1000, true);

                        if (pTarget)
                            me->CastSpell(pTarget, SPELL_IMMOLATION_TRAP_SUMMON, false);

                        ImmolationTrapTimer = 10000;
                    }

                }
                else
                    ImmolationTrapTimer -= diff;

                if (HurlSpearTimer <= diff)
                {
                    if (!me->IsNonMeleeSpellCasted(false))
                    {
                        if (pRiplimb->isAlive())
                        {
                            switch (urand(0, 3))
                            {
                                case 0:
                                    me->PlayDirectSound(24588);
                                    break;
                                case 1:
                                    me->PlayDirectSound(24587);
                                    break;
                                case 2:
                                    me->PlayDirectSound(24586);
                                    break;
                                case 3:
                                    me->PlayDirectSound(24585);
                                    break;
                            }

                            float x,y,z,ripangle;
                            pRiplimb->GetPosition(x, y, z);
                            ripangle = me->GetAngle(x, y);
                            pRiplimb->GetNearPoint2D(x, y, ((float)urand(100,150))/10.0f, ripangle);
                            SpearPos.Relocate(x, y, z);
                            Creature* pSpear = me->SummonCreature(NPC_IMMOLATION_TRAP_BUNNY, x, y, z);
                            if (((pSummonSpear = me->SummonCreature(NPC_HURL_SPEAR_WEAPON, x, y, z))) != NULL)
                            {
                                pSummonSpear->SetVisibility(VISIBILITY_OFF);
                            }
                            if (pSpear)
                            {
                                pSpear->setFaction(35);
                                pSpear->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
                                pSpear->SetFloatValue(OBJECT_FIELD_SCALE_X, 1.0f);
                                pSpear->CastSpell(pSpear, SPELL_SPEAR_TARGET_VISUAL, true);
                                summonedTraps.push_back(pSpear->GetGUID());
                            }
                            me->CastSpell(pSpear, SPELL_HURL_SPEAR, false);
                            HurlSpearTimer = 42000;
                        }
                        else
                        {
                            switch (urand(1,2))
                            {
                                case 1:
                                    DoYell("Now you BURN!!", 24576);
                                    break;
                                case 2:
                                    DoYell("Twist in flames, interlopers!", 24577);
                                    break;
                            }
                            me->CastSpell(me, SPELL_MAGMA_RUPTURE, false);

                            HurlSpearTimer = 15000;
                        }
                    }
                }
                else
                    HurlSpearTimer -= diff;

                if (separationCheckTimer <= diff)
                {
                    if (pRiplimb && pRiplimb->isAlive() && me->GetDistance(pRiplimb) >= 60.0f)
                    {
                        me->CastSpell(me, SPELL_SEPARATION_ANXIETY, true);
                        pRiplimb->CastSpell(pRiplimb, SPELL_SEPARATION_ANXIETY, true);
                    }

                    if (pRageface && pRageface->isAlive() && me->GetDistance(pRageface) >= 60.0f)
                    {
                        me->CastSpell(me, SPELL_SEPARATION_ANXIETY, true);
                        pRageface->CastSpell(pRageface, SPELL_SEPARATION_ANXIETY, true);
                    }

                    // also check for trapped players here - if everybody is frozen in prison, wipe the prisons
                    if (pInstance)
                    {
                        Map::PlayerList const& plList = pInstance->instance->GetPlayers();
                        bool everybody = true;
                        for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
                        {
                            if (!itr->getSource()->HasAura(SPELL_CRYSTAL_PRISON_EFFECT))
                            {
                                everybody = false;
                                break;
                            }
                        }
                        if (everybody)
                        {
                            std::list<Creature*> cr;
                            GetCreatureListWithEntryInGrid(cr, me, NPC_CRYSTAL_PRISON, 50000.0f);

                            if (!cr.empty())
                            {
                                for (std::list<Creature*>::const_iterator itr = cr.begin(); itr != cr.end(); itr++)
                                    me->Kill((*itr));
                            }
                        }
                    }

                    separationCheckTimer = 1000;
                }
                else
                    separationCheckTimer -= diff;

                if (IsHeroic() && pRiplimb && pRiplimb->isDead())
                {
                    if (riplimbRespawnTimer <= diff)
                    {
                        pRiplimb->Respawn(true);
                        pRiplimb->RemoveAllAuras();
                        me->RemoveAuraFromStack(SPELL_FRENZY);

                        pRiplimb->SetReactState(REACT_AGGRESSIVE);
                        pRiplimb->SetInCombatWithZone();

                        pRiplimb->CastSpell(pRiplimb,SPELL_FEEDING_FRENZY,true); // Feeding Frenzy
                        me->MonsterTextEmote("Riplimb reanimates and surges into battle to protect his master!",0,true);

                        //pRiplimb->SetPosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), true);    WHAT,WHY ???

                        Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 1,250.0f,true);
                        if (pTarget == NULL)
                            pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0,250.0f,true);

                        if (pTarget)
                        {
                            pRiplimb->AI()->AttackStart(pTarget);
                            pRiplimb->GetMotionMaster()->Clear(false);
                            pRiplimb->GetMotionMaster()->MoveChase(pTarget);
                        }
                        riplimbRespawnTimer = 30000;
                    }
                    else riplimbRespawnTimer -= diff;
                }

                DoMeleeAttackIfReady();

            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new boss_shannoxAI(c);
        }
};

class npc_riplimb : public CreatureScript
{
    public:
        npc_riplimb() : CreatureScript("npc_riplimb") { }

        struct npc_riplimbAI : public ScriptedAI
        {
            npc_riplimbAI(Creature* c) : ScriptedAI(c)
            {
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_THREAT, true);
                me->ApplySpellImmune(0, IMMUNITY_ID, 49560, true);
                Reset();
            }

            InstanceScript* pInstance;
            bool spearDropped;
            bool shannoxHP;
            bool canBeTrapped;
            uint32 spearDelayTimer;
            uint32 limbRipTimer;
            uint8 bossWait;
            Unit* pSpear;
            Creature* pBoss;
            uint32 DoggedDeterminationTimer;

            void Reset()
            {
                me->SetReactState(REACT_PASSIVE);
                pSpear = NULL;
                pBoss = NULL;
                spearDropped = false;
                shannoxHP = false;
                canBeTrapped = true;
                spearDelayTimer = 3000;
                limbRipTimer = urand(8000, 10000);
                bossWait = 0;
                DoggedDeterminationTimer = 0;

                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SNARE, true);

                pInstance = me->GetInstanceScript();
                if (pInstance)
                    pBoss = Unit::GetCreature((*me), pInstance->GetData64(TYPE_SHANNOX));
            }

            void EnterCombat(Unit* target)
            {
                if (IsHeroic())
                    me->CastSpell(me,SPELL_FEEDING_FRENZY,true); // Feeding Frenzy

                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SNARE, true);

                if (pInstance)
                    pBoss = Unit::GetCreature((*me), pInstance->GetData64(TYPE_SHANNOX));

                if (pBoss)
                    pBoss->SetReactState(REACT_AGGRESSIVE);
            }

            void MovementInform(uint32 type, uint32 id)
            {
                if (type != POINT_MOTION_TYPE)
                    return;

                switch (id)
                {
                    case 1:
                        if (pSpear)
                            pSpear->ToCreature()->ForcedDespawn();
                        me->CastSpell(me, SPELL_SPEAR_VISUAL, true);
                        me->SetSpeed(MOVE_RUN, 1.7f);
                        bossWait = 1;
                        break;
                    case 2:
                        bossWait = 2;
                        me->SetSpeed(MOVE_RUN, 3.2f);
                        canBeTrapped = true;
                        break;
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (bossWait == 1)
                {
                    if (pBoss)
                        me->GetMotionMaster()->MovePoint(2, pBoss->GetPositionX(), pBoss->GetPositionY(), pBoss->GetPositionZ());

                    bossWait = 5;
                }

                if (bossWait == 2)
                {
                    me->GetMotionMaster()->Clear(true);
                    me->GetMotionMaster()->MoveChase(me->getVictim());
                    me->RemoveAurasDueToSpell(SPELL_SPEAR_VISUAL);

                    if (pBoss)
                        pBoss->AI()->DoAction(4); // equip spear

                    me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SNARE, true);

                    DoggedDeterminationTimer = 0;
                    me->RemoveAurasDueToSpell(SPELL_DOGGED_DETERMINATION);

                    bossWait = 0;
                }

                if (DoggedDeterminationTimer)
                {
                    if (DoggedDeterminationTimer > 0)
                    {
                        if (DoggedDeterminationTimer <= diff)
                        {
                            me->CastSpell(me, SPELL_DOGGED_DETERMINATION, true);
                            DoggedDeterminationTimer = 0;
                        }
                        else
                            DoggedDeterminationTimer -= diff;
                    }
                }

                if (spearDropped)
                {
                    if (spearDelayTimer <= diff)
                    {
                        if ((pSpear = GetClosestCreatureWithEntry(me, NPC_HURL_SPEAR_WEAPON, 5000.0f)) != NULL)
                        {
                            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SNARE, false);
                            me->GetMotionMaster()->MovePoint(1, pSpear->GetPositionX(), pSpear->GetPositionY(), pSpear->GetPositionZ());
                            canBeTrapped = false;
                            spearDropped = false;
                            bossWait = 5;
                            spearDelayTimer = 3000;
                            DoggedDeterminationTimer = urand(2000,4000);
                        }
                    }
                    else
                        spearDelayTimer -= diff;
                }

                if (limbRipTimer <= diff)
                {
                    me->CastSpell(me->getVictim(), SPELL_LIMB_RIP, false);
                    limbRipTimer = urand(8000, 10000);
                }
                else
                    limbRipTimer -= diff;

                if (pBoss && pBoss->HealthBelowPct(30) && !shannoxHP)
                {
                    me->CastSpell(me, SPELL_FRENZIED_DEVOTION, true);
                    shannoxHP = true;
                }

                DoMeleeAttackIfReady();
            }

            void DoAction(const int32 action)
            {
                if (action == 1)
                    spearDropped = true;
            }

            void DamageTaken(Unit* attacker, uint32& damage)
            {
                if ( damage >= me->GetHealth() && me->GetMotionMaster()->GetCurrentMovementGeneratorType() == POINT_MOTION_TYPE)
                    damage = 0;
            }

            void JustDied(Unit* pKiller)
            {
                if (pBoss)
                {
                    if (IsHeroic() == false)
                        pBoss->MonsterTextEmote("Shannox become enraged at seeing one of his companions fall",0,true);
                    pBoss->AI()->DoAction(1);
                }
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_riplimbAI(c);
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
                Reset();
            }

            InstanceScript* pInstance;
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

                pInstance = me->GetInstanceScript();

                if (pInstance)
                {
                    pBoss = Unit::GetCreature((*me), me->GetInstanceScript()->GetData64(TYPE_SHANNOX));

                    if (pBoss)
                        armtrapTimer = 2000 + uint32(me->GetDistance2d(pBoss) / 100) * 1000;
                }
                else
                    armtrapTimer = 2000;
            }

            void MoveInLineOfSight(Unit* pWho)
            {
                if (!pWho || pWho->GetDistance(me) > 0.3f || !armTrap || trapActivated || pWho->HasAura(SPELL_WARY) || pWho->HasAura(SPELL_DOGGED_DETERMINATION))
                    return;

                if ( pWho->isPet() || pWho->isGuardian())
                    return;

                if (pWho->GetTypeId() == TYPEID_UNIT && pWho->GetEntry() == NPC_RIPLIMB && !((npc_riplimb::npc_riplimbAI*)pWho->GetAI())->canBeTrapped)
                    return;

                if (pWho->ToCreature() && pWho->ToCreature()->GetEntry() == NPC_RAGEFACE)
                {
                    if (!pWho->IsNonMeleeSpellCasted(false) || pWho->ToCreature()->AI()->GetData(0) == 1) // If casting or jumping (1), dont allow go into trap
                        return;
                }

                if (pWho->GetTypeId() == TYPEID_PLAYER || pWho->GetCharmerOrOwnerPlayerOrPlayerItself() != NULL ||
                    (!pWho->HasAura(SPELL_SPEAR_VISUAL) && (pWho->GetEntry() == NPC_RIPLIMB || pWho->GetEntry() == NPC_RAGEFACE)))
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
                        DoUpdateObjectVisibility();
                    }

                    me->SetVisibility(VISIBILITY_OFF);
                }
            }

            void DoUpdateObjectVisibility()
            {
                Map::PlayerList const& plrList = me->GetMap()->GetPlayers();

                if (!plrList.isEmpty())
                {
                    for (Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
                    {
                        itr->getSource()->DestroyForNearbyPlayers();
                        itr->getSource()->UpdateObjectVisibility();
                    }
                }
            }

            Unit* GetPrisoner()
            {
                return Unit::GetUnit((*me), prisonerGuid);
            }

            void DoAction(const int32 id)
            {
                if (id == 1)
                {
                    if (prisonerGuid != 0)
                    {
                        if (GetPrisoner())
                        {
                            if (GetPrisoner()->GetEntry() == NPC_RIPLIMB || GetPrisoner()->GetEntry() == NPC_RAGEFACE)
                                GetPrisoner()->CastSpell(GetPrisoner(), SPELL_WARY, true);

                            GetPrisoner()->RemoveAurasDueToSpell(SPELL_CRYSTAL_PRISON_EFFECT);
                            me->ForcedDespawn();
                        }
                    }
                }
            }

            void SummonedCreatureDespawn(Creature* trg)
            {
                if (Unit* prisoner = GetPrisoner())
                    prisoner->RemoveAurasDueToSpell(SPELL_CRYSTAL_PRISON_EFFECT);
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
                Reset();
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
                if (Creature* pTrap = Unit::GetCreature((*me), summonerGuid))
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
                if (Creature* pTrap = Unit::GetCreature((*me), summonerGuid))
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

class npc_immolation_trap : public CreatureScript
{
    public:
        npc_immolation_trap() : CreatureScript("npc_immolation_trap") { }

        struct npc_immolation_trapAI : public ScriptedAI
        {
            npc_immolation_trapAI(Creature* c) : ScriptedAI(c)
            {
                pInstance = me->GetInstanceScript();
                pBoss = NULL;
                c->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
                c->setFaction(35);
                Reset();
            }

            bool can_trigger;
            bool passive;
            InstanceScript* pInstance;
            uint32 armtrapTimer;
            Creature* pBoss;

            void Reset()
            {
                can_trigger = passive = false;
                me->RemoveAllAuras(); // because "Hurl Spear" visual in his mouth

                if (pInstance)
                {
                    pBoss = Unit::GetCreature((*me), pInstance->GetData64(TYPE_SHANNOX));

                    if (pBoss)
                        armtrapTimer = 2000 + (me->GetDistance2d(pBoss)* 10);
                }
                else
                    armtrapTimer = 2000;
            }

            void MoveInLineOfSight(Unit* pWho)
            {
                if (!pWho || can_trigger == false || passive || pWho->GetDistance(me) > 0.3f || pWho->HasAura(SPELL_WARY))
                    return;

                if (pWho->GetTypeId() == TYPEID_PLAYER || pWho->GetCharmerOrOwnerPlayerOrPlayerItself() != NULL || (pWho->GetEntry() == NPC_RIPLIMB || pWho->GetEntry() == NPC_RAGEFACE))
                {
                    passive = true;
                    me->CastSpell(pWho, SPELL_IMMOLATION_TRAP, true);

                    if(pWho->ToCreature())
                        pWho->CastSpell(pWho, SPELL_WARY, true);

                    me->SetVisible(false);
                }

                ScriptedAI::MoveInLineOfSight(pWho);
            }

            void UpdateAI(const uint32 diff)
            {
                if(can_trigger)
                    return;

                if (armtrapTimer <= diff)
                {
                    can_trigger = true;
                }
                else
                    armtrapTimer -= diff;
            }

        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_immolation_trapAI(c);
        }
};

class npc_rageface : public CreatureScript
{
    public:
        npc_rageface() : CreatureScript("npc_rageface") { }

        struct npc_ragefaceAI : public ScriptedAI
        {
            npc_ragefaceAI(Creature* c) : ScriptedAI(c)
            {
                pInstance = me->GetInstanceScript();
                c->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
                c->ApplySpellImmune(0, IMMUNITY_ID, 49560, true); // Death Grip jump effect
                c->ApplySpellImmune(0, IMMUNITY_ID, 81261, true); // Solar Beam
                c->ApplySpellImmune(0, IMMUNITY_ID, 88625, true); // Chastise
                Reset();
            }

            InstanceScript* pInstance;
            Creature* pBoss;
            uint32 changeTargetTimer;
            bool shannoxHP;
            bool jumping;

            Position jumpPos;

            uint32 jumpTimer;
            uint32 debuffTimer;

            void Reset()
            {
                debuffTimer = 28000;

                jumpTimer = 30000;
                changeTargetTimer = urand(5000,8000);

                me->RemoveAllAuras();
                me->SetReactState(REACT_PASSIVE);
                shannoxHP = true;
                pBoss = NULL;
                if (pInstance)
                    pBoss = Unit::GetCreature((*me), pInstance->GetData64(TYPE_SHANNOX));
            }

            uint32 GetData(uint32 type)
            {
                return ((jumping) ? 1 : 0); // // Is jumping ?
            }

            void DamageTaken(Unit* attacker, uint32& damage)
            {
                if (!me->IsNonMeleeSpellCasted(false)) // Must casting face rage
                    return;

                uint32 distractDamage = (Is25ManRaid()) ? 45000 : 30000;

                if ( damage >= distractDamage)
                {
                    me->InterruptNonMeleeSpells(false);
                    me->RemoveAura(100129); // Crit buff on rageface
                    me->RemoveAura(101212);
                    me->RemoveAura(101213);
                    me->RemoveAura(101214);
                    changeTargetTimer = 100;
                }
            }

            void EnterCombat(Unit* target)
            {
                if (IsHeroic())
                    me->CastSpell(me,SPELL_FEEDING_FRENZY,true); // Feeding Frenzy

                if (pBoss)
                    pBoss->SetReactState(REACT_AGGRESSIVE);

                me->SetInCombatWithZone();
            }

            void EnterEvadeMode()
            {
                jumping = false;
                me->RemoveAllAuras();
                ScriptedAI::EnterEvadeMode();
            }

            void ChangeTarget (bool ragefacing)
            {
                Map::PlayerList const& plrList = me->GetMap()->GetPlayers();
                std::list<Player*> targets;
                targets.clear();

                if (!plrList.isEmpty())
                {
                    for (Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
                    {
                        Player* plr = itr->getSource();

                        if (plr && plr->isAlive() && plr->isGameMaster() == false && plr->GetExactDist2d(me) < 55.0f)
                        {
                            if (!plr->HasTankSpec() && !plr->HasAura(5487)) // Not on tanks
                                if (!plr->HasAura(SPELL_CRYSTAL_PRISON_EFFECT)) // Not on players in crystal
                                    targets.push_back(plr);
                        }
                    }

                    if (!targets.empty())
                    {
                        std::list<Player*>::iterator itr = targets.begin();
                        uint32 randPos = urand(0, targets.size() - 1);
                        std::advance(itr, randPos);
                        if ((*itr) && (*itr)->IsInWorld())
                        {
                            DoResetThreat();
                            me->SetInCombatWithZone();
                            me->AddThreat(*itr,500000.0f);
                            if (ragefacing) // Time for rageface
                            {
                                jumping = true;
                                me->CastSpell((*itr), SPELL_FACE_RAGE, false); // Jump to target
                                debuffTimer = 3000;
                            }
                        }
                    }
                }
            }

            void UnstuckRageFace(void)
            {
                jumping = false;
                DoResetThreat();
                me->SetInCombatWithZone();
                me->InterruptNonMeleeSpells(false);
                if (Unit * player = SelectTarget(SELECT_TARGET_RANDOM,0,100.0f,true))
                {
                    me->AddThreat(player,20.0f);
                    me->GetMotionMaster()->MoveChase(player);
                }
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() != TYPEID_PLAYER)
                    return;

                UnstuckRageFace();
            }

            void SpellHit(Unit* pSrc, const SpellEntry* spell)
            {
                if (spell->Id == SPELL_CRYSTAL_PRISON_EFFECT) // If Rageface was caught in trap
                {
                    debuffTimer = 15000;
                    jumpTimer += 10000; // 10 second trap
                    changeTargetTimer += 10000;
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (jumping)
                {
                    if (Unit * victim = me->getVictim())
                    {
                        if (victim->IsInWorld() && me->IsWithinMeleeRange(victim))
                        {
                            me->StopMoving();
                            me->CastSpell(victim,99947,false); // Stunning and mauling enemy for 30 seconds with increasing damage
                            jumping = false;
                            changeTargetTimer = 31000;
                        }
                    }
                    return;
                }

                if (debuffTimer <= diff)
                {
                    // I will use this timer also for rageface stuck 
                    if (Unit * victim = me->getVictim())
                    {
                        if (victim->IsInWorld() && victim->isDead())
                            UnstuckRageFace();
                    }

                    if (me->IsNonMeleeSpellCasted(false))
                    {
                        me->RemoveAura(100129); // Crit buff on rageface
                        me->RemoveAura(101212);
                        me->RemoveAura(101213);
                        me->RemoveAura(101214);
                        jumping = false;
                    }
                    debuffTimer = 2000;
                }
                else debuffTimer -= diff;

                if (jumpTimer <= diff)
                {
                    if (me->IsNonMeleeSpellCasted(false))
                    {
                        ChangeTarget(true); // Jump on target which will be faceraged
                        changeTargetTimer = 33000;
                        jumpTimer = 30000;
                    }
                }
                else jumpTimer -= diff;

                if (changeTargetTimer <= diff)
                {
                    if (me->IsNonMeleeSpellCasted(false))
                    {
                        ChangeTarget(false);
                        changeTargetTimer = urand(8000, 10000);
                    }
                }
                else changeTargetTimer -= diff;

                if (!shannoxHP && pBoss && pBoss->HealthBelowPct(30))
                {
                    me->CastSpell(me, SPELL_FRENZIED_DEVOTION, true);
                    shannoxHP = true;
                }

                DoMeleeAttackIfReady();
            }

            void JustDied(Unit* pKiller)
            {
                if (Creature* pBoss = Unit::GetCreature((*me), pInstance->GetData64(TYPE_SHANNOX)))
                {
                    pBoss->MonsterTextEmote("Shannox become enraged at seeing one of his companions fall",0,true);
                    pBoss->AI()->DoAction(2);
                }
            }
        };

    CreatureAI* GetAI(Creature* c) const
    {
        return new npc_ragefaceAI(c);
    }
};

class spell_gen_face_rage : public SpellScriptLoader
{
    public:
        spell_gen_face_rage() : SpellScriptLoader("spell_gen_face_rage") { }

        class spell_gen_face_rage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_face_rage_SpellScript);

            void HandleHit(SpellEffIndex /*effIndex*/)
            {
                Unit * unit = GetHitUnit();
                Aura * rf = unit->GetAura(99947);
                if(!unit || !rf)
                    return;

                uint32 ticks = 60 - (rf->GetDuration() / 500); // hits every 500 ms, 30 s full duration -> 30000/500 = 60

                if (ticks != 0)
                    ticks -= 1; // Don't count bonus fo initial tick

                if (ticks)
                {
                    uint32 addition = ticks * 8000;
                    SetHitDamage(GetHitDamage() + addition);
                }
                else
                    SetHitDamage(8000);
            }

            void Register()
            {
                OnEffect += SpellEffectFn(spell_gen_face_rage_SpellScript::HandleHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_face_rage_SpellScript();
        }
};

void AddSC_boss_shannox()
{
    new boss_shannox();
    new npc_crystal_trap();
    new npc_crystal_prison();
    new npc_immolation_trap();
    new npc_riplimb();
    new npc_rageface();
    new spell_gen_face_rage();
}

/*
    Heroic SQLs
    DELETE FROM `spell_script_names` WHERE  `spell_id`=99948 AND `ScriptName`='spell_gen_face_rage' LIMIT 1;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (99948, 'spell_gen_face_rage');
*/

/*
SQL:
UPDATE creature_template SET scriptname = "npc_crystal_trap" WHERE entry = 53713;
UPDATE creature_template SET scriptname = "npc_crystal_prison" WHERE entry = 53819;
UPDATE creature_template SET scriptname = "npc_immolation_trap" WHERE entry = 53724;
UPDATE creature_template SET scriptname = "npc_riplimb" WHERE entry = 53694;
UPDATE creature_template SET scriptname = "npc_rageface" WHERE entry = 53695;
UPDATE creature_template SET scriptname = "boss_shannox" WHERE entry = 53691;

INSERT INTO creature_template
  (entry, difficulty_entry_1, difficulty_entry_2, difficulty_entry_3, KillCredit1, KillCredit2, modelid1, modelid2, modelid3, modelid4, name, subname, IconName, gossip_menu_id, minlevel, maxlevel, exp, faction_A, faction_H, npcflag, speed_walk, speed_run, scale, rank, mindmg, maxdmg, dmgschool, attackpower, dmg_multiplier, baseattacktime, rangeattacktime, unit_class, unit_flags, dynamicflags, family, trainer_type, trainer_spell, trainer_class, trainer_race, minrangedmg, maxrangedmg, rangedattackpower, type, type_flags, lootid, pickpocketloot, skinloot, resistance1, resistance2, resistance3, resistance4, resistance5, resistance6, spell1, spell2, spell3, spell4, spell5, spell6, spell7, spell8, PetSpellDataId, VehicleId, mingold, maxgold, AIName, MovementType, InhabitType, Health_mod, Mana_mod, Armor_mod, RacialLeader, questItem1, questItem2, questItem3, questItem4, questItem5, questItem6, movementId, RegenHealth, equipment_id, mechanic_immune_mask, flags_extra, ScriptName, WDBVerified)
VALUES
  (537240, 0, 0, 0, 0, 0, 11686, 0, 0, 0, "Immolation Trap trigger", "NULL", "NULL", 0, 1, 1, 3, 35, 35, 0, 1, "1,14286", "0,3", 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "", 0, 3, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, "", 15595);

*/
